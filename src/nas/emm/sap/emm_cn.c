/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under 
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.  
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*****************************************************************************

  Source      emm_cn.c

  Version     0.1

  Date        2013/12/05

  Product     NAS stack

  Subsystem   EPS Core Network

  Author      Sebastien Roux, Lionel GAUTHIER

  Description

*****************************************************************************/

#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "log.h"
#include "msc.h"
#include "gcc_diag.h"
#include "dynamic_memory_check.h"
#include "assertions.h"
#include "common_defs.h"
#include "common_types.h"
#include "common_defs.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "mme_app_ue_context.h"
#include "assertions.h"
#include "3gpp_requirements_24.301.h"
#include "mme_app_defs.h"
#include "mme_app_apn_selection.h"

#include "emm_cause.h"
#include "emm_sap.h"
#include "emm_data.h"
#include "emm_proc.h"
#include "bstrlib.h"
#include "emm_cn.h"

#include "nas_itti_messaging.h"

extern int emm_cn_wrapper_tracking_area_update_accept (emm_data_context_t * emm_context);

static int _emm_cn_authentication_res (emm_cn_auth_res_t * const msg);
static int _emm_cn_authentication_fail (const emm_cn_auth_fail_t * msg);
static int _emm_cn_deregister_ue (const mme_ue_s1ap_id_t ue_id);
static int _emm_cn_context_res (const emm_cn_context_res_t * const msg);
static int _emm_cn_context_fail (const emm_cn_context_fail_t * msg);

/** Messages sent to both EMM and ESM layer by the MME_APP depending on the state of the UE context. */
static int _emm_cn_pdn_config_res (emm_cn_pdn_config_res_t * msg_pP);
static int _emm_cn_pdn_config_fail (const emm_cn_pdn_config_fail_t * msg_pP);

/*
 * String representation of EMMCN-SAP primitives
*/
static const char                      *_emm_cn_primitive_str[] = {
  "EMM_CN_AUTHENTICATION_PARAM_RES",
  "EMM_CN_AUTHENTICATION_PARAM_FAIL",
  "EMMCN_CONTEXT_RES",
  "EMMCN_CONTEXT_FAIL",
  "EMMCN_PDN_CONFIG_RES",
  "EMMCN_PDN_CONFIG_FAIL",
  "EMM_CN_DEREGISTER_UE",
  "EMMCN_IMPLICIT_DETACH_UE",
  "EMMCN_SMC_PROC_FAIL",
};

//------------------------------------------------------------------------------
static int _emm_cn_authentication_res (emm_cn_auth_res_t * const msg)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_data_context_t                          *emm_context = NULL;
  int                                     rc = RETURNerror;

  /*
   * We received security vector from HSS. Try to setup security with UE
   */
  emm_context = emm_data_context_get(&_emm_data, msg->ue_id);
  if (emm_context) {
    nas_auth_info_proc_t * auth_info_proc = get_nas_cn_procedure_auth_info(emm_context);

    if (auth_info_proc) {
      for (int i = 0; i < msg->nb_vectors; i++) {
        auth_info_proc->vector[i] = msg->vector[i];
        msg->vector[i] = NULL;
      }
      auth_info_proc->nb_vectors = msg->nb_vectors;
      rc = (*auth_info_proc->success_notif)(emm_context);
    } else {
      OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Failed to find Auth_info procedure associated to UE id " MME_UE_S1AP_ID_FMT "...\n", msg->ue_id);
    }
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_cn_authentication_fail (const emm_cn_auth_fail_t * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_data_context_t                     *emm_context = NULL;
  int                                     rc = RETURNerror;

  /*
   * We received security vector from HSS. Try to setup security with UE
   */
  emm_context = emm_data_context_get(&_emm_data, msg->ue_id);

  nas_auth_info_proc_t * auth_info_proc = get_nas_cn_procedure_auth_info(emm_context);

  if (auth_info_proc) {
    auth_info_proc->nas_cause = msg->cause;
    rc = (*auth_info_proc->failure_notif)(emm_context);
  } else {
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Failed to find Auth_info procedure associated to UE id " MME_UE_S1AP_ID_FMT "...\n", msg->ue_id);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_cn_smc_fail (const emm_cn_smc_fail_t * msg)
{
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  rc = emm_proc_attach_reject (msg->ue_id, msg->emm_cause);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_cn_pdn_config_res (emm_cn_pdn_config_res_t * msg_pP)
{
  emm_data_context_t                       *emm_context = NULL;
  ue_context_t                             *ue_context  = NULL;
  int                                       rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_context = emm_data_context_get (&_emm_data, msg_pP->ue_id);
  if (!emm_context) { /**< We assume that an MME_APP UE context also should not exist here. */
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Failed to find UE associated to id " MME_UE_S1AP_ID_FMT ". "
        "Not further processing PDN config response (not removing it)...\n", msg_pP->ue_id);
    /** No procedure to remove. */
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, msg_pP->ue_id);
  if(!ue_context){
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  ATTACH - For ueId " MME_UE_S1AP_ID_FMT " no UE context exists. \n", msg_pP->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }
  /*
   * We already have an ESM PDN connectivity (due handover).
   * Like it is the case in PDN_CONNECTIVITY_RES, check the status and respond.
   */
  if (!is_nas_specific_procedure_tau_running(emm_context)){
    /*
     * It should be sent to the ESM layer else.
     * Not processing it if not due Tracking Request.
     */
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "PDN configuration response not processed in EMM layer only due TAU. Ignoring (should be sent to ESM).. for id " MME_UE_S1AP_ID_FMT ". \n", msg_pP->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }

  /*
   * The UE context will already be updated by the S6a ULA.
   * Get the APN configuration information.
   * For each APN configuration, update the subscription and ctx-id information in the already established PDN contexts.
   */
  subscription_data_t *subscription_data = mme_ue_subscription_data_exists_imsi(&mme_app_desc.mme_ue_contexts, emm_context->_imsi64);
  if(!subscription_data){
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "PDN configuration response not processed in EMM layer only due TAU. Ignoring (should be sent to ESM).. for id " MME_UE_S1AP_ID_FMT ". \n", msg_pP->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }
  /**
   * Update each existing PDN context with the received APN configuration.
   * Remove all sessions of all PDN connections where no subscription is received implicitly.
   * For each change in the pdn context, we have to traverse the tree from the beginning.
   */
  pdn_context_t * pdn_context = NULL, * pdn_context_safe = NULL;
  bool changed = false;
  /** Check if an APN configuration exists. */
  apn_configuration_t * apn_configuration = NULL;
  do {
    RB_FOREACH_SAFE(pdn_context, PdnContexts, &ue_context->pdn_contexts, pdn_context_safe) {
      /** Remove the pdn context. */
      changed = false;
      mme_app_select_apn(emm_context->_imsi64, pdn_context->apn_subscribed, &apn_configuration);
      if(apn_configuration){
        /**
         * We found an APN configuration. Updating it. Might traverse the list new.
         * We check if anything (ctxId) will be changed, that might alter the position of the element in the list.
         */
        OAILOG_INFO(LOG_NAS_EMM, "EMMCN-SAP  - " "Updating the created PDN context from the subscription profile for UE " MME_UE_S1AP_ID_FMT". \n", msg_pP->ue_id);
        changed = pdn_context->context_identifier >= PDN_CONTEXT_IDENTIFIER_UNASSIGNED;
        if(mme_app_update_pdn_context(ue_context->mme_ue_s1ap_id, pdn_context->context_identifier, pdn_context->default_ebi, pdn_context->apn_subscribed, apn_configuration) == RETURNerror){
          OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Error while processing the created PDN context for APN \"%s\" for ue_id " MME_UE_S1AP_ID_FMT ". "
              "Aborting TAU procedure. \n", bdata(pdn_context->apn_subscribed), msg_pP->ue_id);
          // todo: better error handlign..
          OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
        }
        if(changed)
          break;
        /** Nothing changed, continue processing the elements. */
        continue;
      }else {
        /**
         * Remove the PDN context and trigger a DSR.
         * Set in the flags, that it should not be signaled back to the EMM layer. Might not need to traverse the list new.
         */
        OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "PDN context for APN \"%s\" could not be found in subscription profile for UE "
            "with ue_id " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT ". Triggering deactivation of PDN context. \n",
            bdata(pdn_context->apn_subscribed), emm_context->ue_id, emm_context->_imsi64);
        nas_itti_pdn_disconnect_req(emm_context->ue_id, pdn_context->default_ebi, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, false,
            pdn_context->s_gw_address_s11_s4.address.ipv4_address, pdn_context->s_gw_teid_s11_s4, pdn_context->context_identifier);
        /** No response is expected. */
        // todo: handle the discrepance in the UE/eNB contexts..
        mme_app_esm_delete_pdn_context(emm_context->ue_id, pdn_context->apn_subscribed, pdn_context->context_identifier, pdn_context->default_ebi);
        OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Invalid PDN context removed successfully ue_id " MME_UE_S1AP_ID_FMT ". \n", msg_pP->ue_id);
        continue;
      }
    }
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - " "Completed the checking of PDN contexts for ue_id " MME_UE_S1AP_ID_FMT ". \n", msg_pP->ue_id);
  } while(changed);

  /*
   * Send tracking area update accept message to the UE
   */
  rc = emm_cn_wrapper_tracking_area_update_accept(emm_context);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc );

}

//------------------------------------------------------------------------------
static int
_emm_cn_pdn_config_fail (const emm_cn_pdn_config_fail_t * msg_pP){
  int                                     rc = RETURNok;
  struct emm_data_context_s              *emm_context = NULL;
  ESM_msg                                 esm_msg = {.header = {0}};
  int                                     esm_cause;

  OAILOG_FUNC_IN (LOG_NAS_ESM);
  emm_context = emm_data_context_get (&_emm_data, msg_pP->ue_id);
  if (!emm_context) {
    OAILOG_ERROR (LOG_NAS_ESM, "EMMCN-SAP  - " "Failed to find UE associated to id " MME_UE_S1AP_ID_FMT "...\n", msg_pP->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
  }
  if (!is_nas_specific_procedure_tau_running(emm_context)){
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "PDN configuration reject not processed in EMM layer only due TAU. Ignoring (should be sent to ESM).. for id " MME_UE_S1AP_ID_FMT ". \n", msg_pP->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
  }
  /*
   * Send tracking area update reject message to the UE, no matther the mobiltiy.
   */
  //    emm_sap_t                               emm_sap = {0};
  //    memset(&emm_sap, 0, sizeof(emm_sap_t));
  //    emm_sap.primitive = EMMREG_TAU_REJ;
  //    emm_sap.u.emm_reg.ue_id = emm_ctx_p->ue_id;
  //    emm_sap.u.emm_reg.ctx  = emm_ctx_p;
  //    emm_sap.u.emm_reg.u.tau.proc = tau_proc;
  //    emm_sap.u.emm_reg.notify= true;
  //    emm_sap.u.emm_reg.free_proc = true;
  //
  //    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_COMMON_PROC_REQ ue id " MME_UE_S1AP_ID_FMT " ", emm_ctx_p->ue_id);
  //    rc = emm_proc_attach_reject (msg->ue_id, EMM_CAUSE_ESM_FAILURE);
  //    rc = emm_sap_send (&emm_sap);
  rc = emm_proc_tracking_area_update_reject(msg_pP->ue_id, EMM_CAUSE_ESM_FAILURE);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
static int _emm_cn_deregister_ue (const mme_ue_s1ap_id_t ue_id)
{
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - " "TODO deregister UE " MME_UE_S1AP_ID_FMT ", following procedure is a test\n", ue_id);
  emm_detach_request_ies_t * params = calloc(1, sizeof(*params));
  params->type         = EMM_DETACH_TYPE_EPS;
  params->switch_off   = false;
  params->is_native_sc = false;
  params->ksi          = 0;
  emm_proc_detach_request (ue_id, params);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_cn_implicit_detach_ue (const uint32_t ue_id, const emm_proc_detach_type_t detach_type, const int emm_cause)
{
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC Implicit Detach udId " MME_UE_S1AP_ID_FMT "\n", ue_id);
  emm_detach_request_ies_t  params = {0};
//  //params.decode_status
//  //params.guti = NULL;
//  //params.imei = NULL;
//  //params.imsi = NULL;
//  params.is_native_sc = true;
//  params.ksi = 0;
//  params.switch_off = true;
//  params.type = EMM_DETACH_TYPE_EPS;

  /** No detach procedure for implicit detach. Set the UE state into EMM-DEREGISTER-INITIATED state. */
  emm_proc_detach(ue_id, detach_type, emm_cause);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_cn_context_res (const emm_cn_context_res_t * msg)
{
  emm_data_context_t                     *emm_context = NULL;
  int                                     rc = RETURNerror;
  /**
   * We received security vector from source MME.
   * Directly using received security parameters.
   * If we received already security parameters like UE network capability, ignoring the parameters received from the source MME and using the UE parameters.
   */
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_context = emm_data_context_get (&_emm_data, msg->ue_id);
  if (emm_context == NULL) { /**< We assume that an MME_APP UE context also should not exist here. */
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Failed to find UE associated to id " MME_UE_S1AP_ID_FMT "...\n", msg->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }

  nas_ctx_req_proc_t * ctx_req_proc = get_nas_cn_procedure_ctx_req(emm_context);
  /** Update the context request procedure. */
  if (!ctx_req_proc) {
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Failed to find context request procedure associated to UE id " MME_UE_S1AP_ID_FMT "...\n", msg->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  } else {
    // todo: PDN Connections IE from here or else?
    rc = (*ctx_req_proc->success_notif)(emm_context);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_cn_context_fail (const emm_cn_context_fail_t * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_data_context_t                     *emm_ctx = NULL;
  int                                     rc = RETURNerror;

  /**
   * An UE could or could not exist. We need to check. If it exists, it needs to be purged.
   * Since no UE context is established yet, we don't have security/no bearers.0
   * If the message is received after the timeout, the MME_APP context also should not exist.
   * If the MME_APP context existed at that point in time, it will later be removed.
   * Just discard the message then.
   */
  emm_ctx = emm_data_context_get (&_emm_data, msg->ue_id);
  if (emm_ctx == NULL) {
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Failed to find UE associated to id " MME_UE_S1AP_ID_FMT " (S10 Ctx fail)...\n", msg->ue_id);
    /**
     * In this case, don't wait for the timer to remove the rest! Assume no timers exist.
     * Purge the rest of the UE context (MME_APP etc.).
     */
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }

  nas_ctx_req_proc_t * ctx_req_proc = get_nas_cn_procedure_ctx_req(emm_ctx);

  if (ctx_req_proc) {
    ctx_req_proc->nas_cause = msg->cause; // todo: better handling
    rc = (*ctx_req_proc->failure_notif)(emm_ctx);
    nas_delete_cn_procedure(emm_ctx, ctx_req_proc);

  } else {
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Failed to find context request procedure associated to UE id " MME_UE_S1AP_ID_FMT "...\n", msg->ue_id);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

//------------------------------------------------------------------------------
int emm_cn_send (const emm_cn_t * msg)
{
  int                                     rc = RETURNerror;
  emm_cn_primitive_t                      primitive = msg->primitive;

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  OAILOG_INFO (LOG_NAS_EMM, "EMMCN-SAP - Received primitive %s (%d)\n", _emm_cn_primitive_str[primitive - _EMMCN_START - 1], primitive);

  switch (primitive) {
  case EMMCN_AUTHENTICATION_PARAM_RES:
    rc = _emm_cn_authentication_res (msg->u.auth_res);
    break;

  case EMMCN_AUTHENTICATION_PARAM_FAIL:
    rc = _emm_cn_authentication_fail (msg->u.auth_fail);
    break;

  case EMMCN_DEREGISTER_UE:
    rc = _emm_cn_deregister_ue (msg->u.deregister.ue_id);
    break;

  case EMMCN_IMPLICIT_DETACH_UE:
    rc = _emm_cn_implicit_detach_ue (msg->u.emm_cn_implicit_detach.ue_id, msg->u.emm_cn_implicit_detach.detach_type, msg->u.emm_cn_implicit_detach.emm_cause);
    break;

  case EMMCN_SMC_PROC_FAIL:
    rc = _emm_cn_smc_fail (msg->u.smc_fail);
    break;

  case EMMCN_PDN_CONFIG_RES:
    rc = _emm_cn_pdn_config_res(msg->u.pdn_cfg_res);
    break;

  case EMMCN_PDN_CONFIG_FAIL:
    rc = _emm_cn_pdn_config_fail(msg->u.pdn_cfg_fail);
    break;

    /** S10 Context Response information. */
  case EMMCN_CONTEXT_RES:
    rc = _emm_cn_context_res (msg->u.context_res);
    break;
  case EMMCN_CONTEXT_FAIL:
    rc = _emm_cn_context_fail (msg->u.context_fail);
    break;

  default:
    /*
     * Other primitives are forwarded to the Access Stratum
     */
    rc = RETURNerror;
    break;
  }

  if (rc != RETURNok) {
    OAILOG_ERROR (LOG_NAS_EMM, "EMMCN-SAP - Failed to process primitive %s (%d)\n", _emm_cn_primitive_str[primitive - _EMMCN_START - 1], primitive);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

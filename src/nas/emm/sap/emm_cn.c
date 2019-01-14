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
//#include "esm_proc.h"
//#include "esm_cause.h"
//#include "esm_sap.h"
//#include "esm_send.h"
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
  int                                       rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  // todo: multi apn handover ?!
  emm_context = emm_data_context_get (&_emm_data, msg_pP->ue_id);
  if (!emm_context) { /**< We assume that an MME_APP UE context also should not exist here. */
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Failed to find UE associated to id " MME_UE_S1AP_ID_FMT ". "
        "Not further processing PDN config response (not removing it)...\n", msg_pP->ue_id);
    /** No procedure to remove. */
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
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
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
  }
  if (!msg_pP->mobility) {
//    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - " "No mobility procedure is running (initial TAU) for UE " MME_UE_S1AP_ID_FMT. "Continuing with PDN connectivity. \n", msg_pP->ue_id);
    // todo: process the APN-Configuration!
    // todo: create a new PDN context with the received PDN configuration!
    // todo: send an S11 message
    // todo:
//    nas_itti_pdn_connectivity_req (PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, msg_pP->ue_id, pdn_cid, default_ebi, emm_context->_imsi64, &emm_context->_imsi,
//        emm_context->esm_ctx.esm_proc_data, emm_context->esm_ctx.esm_proc_data->request_type);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
  }else {
    /*
     * Send tracking area update accept message to the UE
     */
//    rc = esm_cn_wrapper_tracking_area_update_accept(emm_context);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc );
  }
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
static int _emm_cn_pdn_connectivity_res (emm_cn_pdn_connectivity_resp_t * msg_pP)
{
  int                                     rc = RETURNerror;
  struct emm_data_context_s              *emm_context = NULL;
  struct pdn_context_s                   *pdn_context = NULL;
  esm_proc_pdn_type_t                     esm_pdn_type = ESM_PDN_TYPE_IPV4;
  ESM_msg                                 esm_msg = {.header = {0}};
  EpsQualityOfService                     qos = {0};
  bool                                    is_standalone = false;    // warning hardcoded
  bool                                    triggered_by_ue = true;  // warning hardcoded
//  esm_sap_t                               esm_sap = {0};

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  //  clear_protocol_configuration_options(&msg_pP->pco); // todo: here or in the ITTI_FREE function
  /** Only enter this method for standalone TAU Request case. */
  if (!emm_context) {
    OAILOG_ERROR (LOG_NAS_ESM, "EMMCN-SAP  - " "Failed to find UE associated to id " MME_UE_S1AP_ID_FMT "...\n", msg_pP->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
  }
  if (!is_nas_specific_procedure_tau_running(emm_context)){
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "PDN connectivity reject not processed in EMM layer only due TAU. Ignoring (should be sent to ESM).. for id " MME_UE_S1AP_ID_FMT ". \n", msg_pP->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
  }

  // todo: update the UE/EMM context with the subscription data..
  rc = emm_cn_wrapper_tracking_area_update_accept(emm_context);
  /** We will set the UE into COMMON-PROCEDURE-INITIATED state inside this method. */
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int
_emm_cn_pdn_connectivity_fail (const emm_cn_pdn_connectivity_fail_t * msg)
{
//  OAILOG_FUNC_IN (LOG_NAS_ESM);
//  int                                     rc = RETURNok;
//  struct emm_data_context_s              *emm_context = NULL;
//  ESM_msg                                 esm_msg = {.header = {0}};
//  int                                     esm_cause;
//  emm_context = emm_data_context_get (&_emm_data, msg->ue_id);
//  if (emm_context == NULL) {
//    OAILOG_ERROR (LOG_NAS_ESM, "EMMCN-SAP  - " "Failed to find UE associated to id " MME_UE_S1AP_ID_FMT "...\n", msg->ue_id);
//    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
//  }
//  memset (&esm_msg, 0, sizeof (ESM_msg));
//
//  // Map S11 cause to ESM cause
//  switch (msg->cause) {
//    case CAUSE_CONTEXT_NOT_FOUND:
//      esm_cause = ESM_CAUSE_REQUEST_REJECTED_BY_GW;
//      break;
//    case CAUSE_INVALID_MESSAGE_FORMAT:
//      esm_cause = ESM_CAUSE_REQUEST_REJECTED_BY_GW;
//      break;
//    case CAUSE_SERVICE_NOT_SUPPORTED:
//      esm_cause = ESM_CAUSE_SERVICE_OPTION_NOT_SUPPORTED;
//      break;
//    case CAUSE_SYSTEM_FAILURE:
//      esm_cause = ESM_CAUSE_NETWORK_FAILURE;
//      break;
//    case CAUSE_NO_RESOURCES_AVAILABLE:
//      esm_cause = ESM_CAUSE_INSUFFICIENT_RESOURCES;
//      break;
//    case CAUSE_ALL_DYNAMIC_ADDRESSES_OCCUPIED:
//      esm_cause = ESM_CAUSE_INSUFFICIENT_RESOURCES;
//      break;
//    default:
//      esm_cause = ESM_CAUSE_REQUEST_REJECTED_BY_GW;
//      break;
//  }
//
//  if(is_nas_specific_procedure_attach_running(emm_context)){
//    OAILOG_ERROR (LOG_NAS_ESM, "EMMCN-SAP  - " "Sending Attach/PDN Connectivity Reject message to id " MME_UE_S1AP_ID_FMT "...\n", msg->ue_id);
//
//    rc = esm_send_pdn_connectivity_reject (msg->pti, &esm_msg.pdn_connectivity_reject, esm_cause);
//    /*
//     * Encode the returned ESM response message
//     */
//    uint8_t                             esm_cn_sap_buffer[esm_cn_SAP_BUFFER_SIZE];
//    int size = esm_msg_encode (&esm_msg, esm_cn_sap_buffer, esm_cn_SAP_BUFFER_SIZE);
//    OAILOG_INFO (LOG_NAS_ESM, "ESM encoded MSG size %d\n", size);
//
//    if (size > 0) {
//      nas_emm_attach_proc_t  *attach_proc = get_nas_specific_procedure_attach(emm_context);
//      /*
//       * Setup the ESM message container
//       */
//      if(attach_proc){
//        /** Sending the PDN connection reject inside a Attach Reject to the UE. */
//        attach_proc->esm_msg_out = blk2bstr(esm_cn_sap_buffer, size);
//        rc = emm_proc_attach_reject (msg->ue_id, EMM_CAUSE_ESM_FAILURE);
//      }else{
//        // todo: send the pdn disconnect reject as a standalone message to the UE.
//        // todo: must clean the created pdn_context elements (no bearers should exist).
//      }
//    }
//  }else if (is_nas_specific_procedure_tau_running(emm_context)){
//    OAILOG_ERROR (LOG_NAS_ESM, "EMMCN-SAP  - " "Sending TAU Reject message to id " MME_UE_S1AP_ID_FMT "..\n", msg->ue_id);
//
//    nas_emm_tau_proc_t  *tau_proc = get_nas_specific_procedure_tau(emm_context);
//    rc = emm_proc_tracking_area_update_reject(msg->ue_id, EMM_CAUSE_ESM_FAILURE);
//  }else{
//    /** Forward the PDN Connectivity to the ESM layer. */
//    OAILOG_DEBUG(LOG_NAS_ESM, "EMMCN-SAP  - " "Forwarding PDN Connectivity Failure for mmeUeS1apId " MME_UE_S1AP_ID_FMT " and enbId %d ..\n", msg->ue_id, msg->linked_ebi);
//    esm_sap_t                               esm_sap = {0};
//    esm_sap.primitive = ESM_PDN_CONNECTIVITY_REJ;
//    esm_sap.ue_id = msg->ue_id;
//    esm_sap.ctx = emm_context;
//    esm_sap.recv = NULL;
//    // todo: how to verify that esm proc is unique?!
//    esm_sap.data.pdn_connect.linked_ebi = msg->linked_ebi; // pdn_context->default_ebi; /**< Default Bearer Id of default APN. */
//// todo:   esm_sap.data.pdn_connect.cid = emm_context->esm_ctx.esm_proc_data->pdn_cid;
//    esm_sap.data.pdn_connect.esm_cause = ESM_CAUSE_NETWORK_FAILURE;
//    rc = esm_sap_send(&esm_sap);
//    OAILOG_FUNC_RETURN(LOG_NAS_ESM, rc);
//  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
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
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Failed to find UE associated to id " MME_UE_S1AP_ID_FMT "...\n", msg->ue_id);
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
  case _EMMCN_AUTHENTICATION_PARAM_RES:
    rc = _emm_cn_authentication_res (msg->u.auth_res);
    break;

  case _EMMCN_AUTHENTICATION_PARAM_FAIL:
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

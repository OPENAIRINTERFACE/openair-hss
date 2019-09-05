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

static int _emm_cn_authentication_res (emm_cn_auth_res_t * const msg);
static int _emm_cn_authentication_fail (const emm_cn_auth_fail_t * msg);
static int _emm_cn_deregister_ue (const mme_ue_s1ap_id_t ue_id);
static int _emm_cn_context_res (const emm_cn_context_res_t * const msg);
static int _emm_cn_context_fail (const emm_cn_context_fail_t * msg);

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
  "EMMCN_REPLACE_UE",
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
static int _emm_cn_implicit_detach_ue (const uint32_t ue_id, const emm_proc_detach_type_t detach_type, const int emm_cause, const bool clr)
{
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC Implicit Detach ue_id " MME_UE_S1AP_ID_FMT "\n", ue_id);
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
  emm_proc_detach(ue_id, detach_type, emm_cause, clr);
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

/*
 *
 * Name:    _emm_wrapper_esm_accept()
 *
 * Description: Checks if an attach or TAU procedure is ongoing and proceeds with the accept.
 *
 * Inputs:  args:      UE context data
 *      Others:    None
 *
 * Outputs:     None
 *      Return:    RETURNok, RETURNerror
 *      Others:    None
 *
 */
int _emm_wrapper_esm_accept(mme_ue_s1ap_id_t ue_id, bstring *esm_rsp, eps_bearer_context_status_t ebr_status)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_data_context_t                     * emm_context = NULL;
  int                                     rc = RETURNerror;

  // todo: get the EMM_CONTEXT, if exists lock it!!
  emm_context = emm_data_context_get(&_emm_data, ue_id);
  if(!emm_context){
    OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - No EMM context was found for ue_id " MME_UE_S1AP_ID_FMT ". Ignoring ESM accept. \n", ue_id);
    bdestroy_wrapper(esm_rsp);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNerror);
  }
  if(is_nas_specific_procedure_attach_running(emm_context)){
	  if(esm_rsp){
		  rc = _emm_wrapper_attach_accept(ue_id, *esm_rsp);
		  OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
	  }
  } else if(is_nas_specific_procedure_tau_running(emm_context)) {
	  if(esm_rsp)
		  bdestroy_wrapper(esm_rsp);
	  rc = emm_wrapper_tracking_area_update_accept(ue_id, ebr_status);
	  OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
  }
  OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - Neither attach nor TAU procedure is ongoing for ue_id " MME_UE_S1AP_ID_FMT ". Ignoring the received ESM reject. \n", ue_id);
  bdestroy_wrapper(esm_rsp);
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNerror);
}

/*
 *
 * Name:    _emm_wrapper_esm_reject()
 *
 * Description: Checks if an attach or TAU procedure is ongoing and proceeds with the reject.
 *
 * Inputs:  args:      UE context data
 *      Others:    None
 *
 * Outputs:     None
 *      Return:    RETURNok, RETURNerror
 *      Others:    None
 *
 */
int _emm_wrapper_esm_reject(mme_ue_s1ap_id_t ue_id, bstring *esm_rsp)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_data_context_t                     * emm_context = NULL;
  int                                     rc = RETURNerror;

  // todo: get the EMM_CONTEXT, if exists lock it!!
  emm_context = emm_data_context_get(&_emm_data, ue_id);
  if(!emm_context){
    OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - No EMM context was found for ue_id " MME_UE_S1AP_ID_FMT ". Ignoring ESM reject. \n", ue_id);
    bdestroy_wrapper(esm_rsp);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNerror);
  }
  if(is_nas_specific_procedure_attach_running(emm_context)){
	  rc = _emm_wrapper_attach_reject(ue_id, *esm_rsp);
	  OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
  } else if(is_nas_specific_procedure_tau_running(emm_context)) {
	  bdestroy_wrapper(esm_rsp);
	  rc = emm_wrapper_tracking_area_update_reject(ue_id, EMM_CAUSE_ESM_FAILURE);
	  OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
  }
  OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - Neither attach nor TAU procedure is ongoing for ue_id " MME_UE_S1AP_ID_FMT ". Ignoring the received ESM reject. \n", ue_id);
  bdestroy_wrapper(esm_rsp);
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNerror);
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
    rc = _emm_cn_implicit_detach_ue (msg->u.emm_cn_implicit_detach.ue_id, msg->u.emm_cn_implicit_detach.detach_type, msg->u.emm_cn_implicit_detach.emm_cause,
    		msg->u.emm_cn_implicit_detach.clr);
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

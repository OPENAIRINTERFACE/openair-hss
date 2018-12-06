/*
 * esm_cn.c
 *
 *  Created on: Dec 3, 2018
 *      Author: root
 */

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

#include "esm_proc.h"
#include "esm_cause.h"
#include "esm_sap.h"
#include "esm_send.h"
#include "bstrlib.h"
#include "emm_cn.h"

#include "nas_itti_messaging.h"

static int _esm_cn_pdn_config_res (esm_cn_pdn_config_res_t * msg_pP);
static int _esm_cn_pdn_config_fail (const esm_cn_pdn_config_fail_t * msg_pP);
static int _esm_cn_pdn_connectivity_res (esm_cn_pdn_res_t * msg_pP);
static int _esm_cn_pdn_config_fail (const esm_cn_pdn_config_fail_t * msg);
static int _esm_cn_pdn_disconnect_res(const esm_cn_pdn_disconnect_res_t * msg);

static int _esm_cn_activate_dedicated_bearer_req (esm_eps_activate_eps_bearer_ctx_req_t * msg);

//------------------------------------------------------------------------------
static int _esm_cn_pdn_config_res (esm_cn_pdn_config_res_t * msg_pP)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  struct esm_context_s                   *esm_context = NULL;
  ue_context_t                           *ue_context;
  pdn_context_t                          *pdn_context = NULL;
  bool                                    is_pdn_connectivity = false;
  esm_sap_t                               esm_sap = {0};
  pdn_cid_t                               pdn_cid = 0;
  ebi_t                                   default_ebi = 0;

//  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, msg_pP->ue_id);
//  emm_context = emm_data_context_get(&_emm_data, msg_pP->ue_id);

  if (esm_context == NULL || ue_context == NULL) {
    OAILOG_ERROR (LOG_NAS_ESM, "ESMCN-SAP  - " "Failed to find ESM/UE context associated to id " MME_UE_S1AP_ID_FMT "...\n", msg_pP->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
  }

  /*
   * Get the APN from the UE requested data or the first APN from handover
   * Todo: multiple APN handover!
   */
  bstring apn = NULL;

  // todo: esm context
//  if(emm_context->esm_ctx.esm_proc_data){ /**< In case of S10 TAU the PDN Contexts should already be established. */
//    apn = emm_context->esm_ctx.esm_proc_data->apn;
//    if(!apn){
//      OAILOG_INFO(LOG_NAS_ESM, "EMMCN-SAP  - " "No APN set in the ESM proc data for UE Id " MME_UE_S1AP_ID_FMT ". Taking default APN. ...\n", msg_pP->ue_id);
//      /** Check if any PDN contexts exists (handover/idle tau). */
//      if(RB_EMPTY(&ue_context->pdn_contexts)){
//        /** Neither a PDN context exists nor an APN is set. */
//        apn_configuration_t *default_apn_config = &ue_context->apn_config_profile.apn_configuration[ue_context->apn_config_profile.context_identifier];
//        apn = blk2bstr(default_apn_config->service_selection, default_apn_config->service_selection_length);
//      }
//    }
//  }else{
//    /** Check if any PDN contexts exists (handover/idle tau). */
//    if(RB_EMPTY(&ue_context->pdn_contexts)){
//      /** Neither a PDN context exists nor an APN is set. */
//      apn_configuration_t *default_apn_config = &ue_context->apn_config_profile.apn_configuration[ue_context->apn_config_profile.context_identifier];
//      apn = blk2bstr(default_apn_config->service_selection, default_apn_config->service_selection_length);
//    }
//  }

  /** Inform the ESM about the PDN Config Response. */
  esm_sap.primitive = ESM_PDN_CONFIG_RES;
  esm_sap.ue_id = emm_context->ue_id;
  esm_sap.ctx = emm_context;
  esm_sap.recv = NULL;
  esm_sap.data.pdn_config_res.pdn_cid             = &pdn_cid;     /**< Only used for initial attach and initial TAU. */
  esm_sap.data.pdn_config_res.default_ebi         = &default_ebi; /**< Only used for initial attach and initial TAU. */
  esm_sap.data.pdn_config_res.is_pdn_connectivity = &is_pdn_connectivity; /**< Default Bearer Id of default APN. */
  esm_sap.data.pdn_config_res.imsi                = msg_pP->imsi64; /**< Context Identifier of default APN. */
  esm_sap.data.pdn_config_res.apn                 = apn; /**< Context Identifier of default APN. Will be NULL in case of multi APN or none given. */

  rc = esm_sap_send(&esm_sap);

  if(rc != RETURNok){
    // todo: checking if TAU_ACCEPT/TAU_REJECT is already sent or not..
    OAILOG_INFO(LOG_NAS_ESM, "ESMCN-SAP  - " "Error processing PDN Config response for UE Id " MME_UE_S1AP_ID_FMT "...\n", msg_pP->ue_id);
    /** Error processing PDN Config Response. */
    esm_cn_t esm_cn = {0};
    esm_cn_pdn_config_fail_t esm_cn_pdn_config_fail = {0};
    esm_cn.u.esm_cn_pdn_config_fail = &esm_cn_pdn_config_fail;
    esm_cn.primitive = EMMCN_PDN_CONFIG_FAIL;
    esm_cn.u.esm_cn_pdn_config_fail->ue_id = msg_pP->ue_id;
    rc = esm_cn_send(&esm_cn);
    OAILOG_FUNC_RETURN(LOG_NAS_ESM, rc);
  }

  /** PDN Context and Bearer Contexts (default) are created with this. */

  if (!is_pdn_connectivity) { /**< We may have just created  a PDN context, but no connectivity yet. Assume pdn_connectivity is established when a PDN_Context already existed. */
  // todo: esm
//    nas_itti_pdn_connectivity_req (emm_context->esm_ctx.esm_proc_data->pti, msg_pP->ue_id, pdn_cid, default_ebi, emm_context->_imsi64, &emm_context->_imsi,
//        emm_context->esm_ctx.esm_proc_data, emm_context->esm_ctx.esm_proc_data->request_type);
  } else {
    /*
     * We already have an ESM PDN connectivity (due handover).
     * Like it is the case in PDN_CONNECTIVITY_RES, check the status and respond.
     */
    if (is_nas_specific_procedure_tau_running(emm_context)){


      OAILOG_INFO(LOG_NAS_ESM, "EMMCN-SAP  - " "Tracking Area Update Procedure is running. PDN Connectivity is already established with ULA. Continuing with the accept procedure for id " MME_UE_S1AP_ID_FMT "...\n", msg_pP->ue_id);
      // todo: checking if TAU_ACCEPT/TAU_REJECT is already sent or not..
      /*
       * Send tracking area update accept message to the UE
       */
      rc = esm_cn_wrapper_tracking_area_update_accept(emm_context);

      /** We will set the UE into COMMON-PROCEDURE-INITIATED state inside this method. */
      OAILOG_FUNC_RETURN(LOG_NAS_ESM, rc);
    }else{
      OAILOG_ERROR (LOG_NAS_ESM, "EMMCN-SAP  - " "TAU procedure is not running for UE associated to id " MME_UE_S1AP_ID_FMT ". ULA received and PDN Connectivity is there. Performing an implicit detach..\n", msg_pP->ue_id);
//     todo: better way to handle this? _esm_cn_implicit_detach_ue(msg_pP->ue_id);
      // todo: rejecting any specific procedures?
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
    }
  }
  // todo: unlock UE contexts
//    unlock_ue_contexts(ue_context);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
}

//------------------------------------------------------------------------------
static int _esm_cn_pdn_config_fail (const esm_cn_pdn_config_fail_t * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNok;
  struct emm_data_context_s              *emm_ctx_p = NULL;
  ESM_msg                                 esm_msg = {.header = {0}};
  int                                     esm_cause;
  emm_ctx_p = emm_data_context_get (&_emm_data, msg->ue_id);
  if (emm_ctx_p == NULL) {
    OAILOG_ERROR (LOG_NAS_ESM, "EMMCN-SAP  - " "Failed to find UE associated to id " MME_UE_S1AP_ID_FMT "...\n", msg->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
  }
  memset (&esm_msg, 0, sizeof (ESM_msg));

  // Map S11 cause to ESM cause
  esm_cause = ESM_CAUSE_NETWORK_FAILURE; // todo: error for SAEGW error and HSS ULA error?

  if(is_nas_specific_procedure_attach_running(emm_ctx_p)){
    // todo: ESM CTX?!
//    if(emm_ctx_p->esm_ctx.esm_proc_data){
//      rc = esm_send_pdn_connectivity_reject (emm_ctx_p->esm_ctx.esm_proc_data->pti, &esm_msg.pdn_connectivity_reject, esm_cause);
//      /*
//       * Encode the returned ESM response message
//       */
//      uint8_t                             esm_cn_sap_buffer[esm_cn_SAP_BUFFER_SIZE];
//      int size = esm_msg_encode (&esm_msg, esm_cn_sap_buffer, esm_cn_SAP_BUFFER_SIZE);
//      OAILOG_INFO (LOG_NAS_ESM, "ESM encoded MSG size %d\n", size);
//
//      if (size > 0) {
//        nas_emm_attach_proc_t  *attach_proc = get_nas_specific_procedure_attach(emm_ctx_p);
//        /*
//         * Setup the ESM message container
//         */
//        attach_proc->esm_msg_out = blk2bstr(esm_cn_sap_buffer, size);
//        rc = emm_proc_attach_reject (msg->ue_id, EMM_CAUSE_ESM_FAILURE);
//       }else{
//         // todo: wtf
//         DevAssert(0);
//       }
//       emm_context_unlock(emm_ctx_p);
//       OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
//
//    }
  }else if (is_nas_specific_procedure_tau_running(emm_ctx_p)){
      /** Trigger a TAU Reject. */
    nas_emm_tau_proc_t  *tau_proc = get_nas_specific_procedure_tau(emm_ctx_p);

//    emm_sap_t                               emm_sap = {0};
//
//    memset(&emm_sap, 0, sizeof(emm_sap_t));
//
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

    rc = emm_proc_tracking_area_update_reject(msg->ue_id, EMM_CAUSE_ESM_FAILURE);

    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
 }else{
    // tood: checking for ESM procedure or some other better way?
    DevAssert(0);
  }
  //  emm_context_unlock(emm_ctx_p);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
static int _esm_cn_pdn_connectivity_res (esm_cn_pdn_res_t * msg_pP)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  struct emm_data_context_s              *emm_context = NULL;
  struct pdn_context_s                   *pdn_context = NULL;
  esm_proc_pdn_type_t                     esm_pdn_type = ESM_PDN_TYPE_IPV4;
  ESM_msg                                 esm_msg = {.header = {0}};
  EpsQualityOfService                     qos = {0};
  bool                                    is_standalone = false;    // warning hardcoded
  bool                                    triggered_by_ue = true;  // warning hardcoded
  esm_sap_t                               esm_sap = {0};

  ue_context_t *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, msg_pP->ue_id);
  emm_context = emm_data_context_get(&_emm_data, msg_pP->ue_id);

  if (ue_context == NULL || emm_context == NULL) {
    OAILOG_ERROR (LOG_NAS_ESM, "EMMCN-SAP  - " "Failed to find UE associated to id " MME_UE_S1AP_ID_FMT "...\n", msg_pP->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
  }

  /** Get the PDN context (should exist right now). */
  /** Check if a tunnel already exists depending on the flag. */
//  pdn_context_t pdn_context_key = {.context_identifier = msg_pP->pdn_cid}; // todo: should be also retrievable from pdn_cid alone (or else get esm_proc_data --> apn stands there, too).
//    keyTunnel.ipv4AddrRemote = pUlpReq->u_api_info.initialReqInfo.peerIp;
  mme_app_get_pdn_context(ue_context, msg_pP->pdn_cid, msg_pP->ebi, NULL, &pdn_context);
  DevAssert(pdn_context);
  nas_emm_attach_proc_t                  *attach_proc = get_nas_specific_procedure_attach(emm_context);

  if(!attach_proc){
    /** Check if it is a TAU procedure, else it is standalone. */
    if (is_nas_specific_procedure_tau_running(emm_context)){
      /** Continue with TAU Accept. */
      rc = esm_cn_wrapper_tracking_area_update_accept(emm_context);
      /** We will set the UE into COMMON-PROCEDURE-INITIATED state inside this method. */
      OAILOG_FUNC_RETURN(LOG_NAS_ESM, rc);
    }
    OAILOG_INFO (LOG_NAS_ESM, "EMM  -  Neither TAU nor ATTACH procedure is running for ue " MME_UE_S1AP_ID_FMT ". Assuming standalone. \n", emm_context->ue_id);
    is_standalone = true;
  }
  /** Trigger default EPS Bearer Activation. */
  esm_sap.primitive = ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REQ;
  esm_sap.ctx           = emm_context;
  esm_sap.ue_id         = msg_pP->ue_id;
  esm_sap.data.pdn_connect.cid = msg_pP->pdn_cid;
  esm_sap.data.pdn_connect.pdn_type = msg_pP->pdn_type;
  esm_sap.data.pdn_connect.apn = pdn_context->apn_subscribed;
  esm_sap.data.pdn_connect.pco = &msg_pP->pco;
  esm_sap.data.pdn_connect.linked_ebi = msg_pP->ebi;
  esm_sap.data.pdn_connect.apn_ambr = msg_pP->apn_ambr; /**< Updated with the value from the PCRF. */
  esm_sap.data.pdn_connect.pdn_addr = msg_pP->pdn_addr; /**< Not storing the IP information. */
  /** Set the QoS. */
  esm_sap.data.pdn_connect.qci = msg_pP->qci;
  MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REQ ue id " MME_UE_S1AP_ID_FMT, esm_sap.ue_id);
  rc = esm_sap_send (&esm_sap);
  clear_protocol_configuration_options(&msg_pP->pco); // todo: here or in the ITTI_FREE function
  /*************************************************************************/
  /*
   * END OF CODE THAT WAS IN esm_sap.c/_esm_sap_recv()
   */
  /*************************************************************************/
  if (attach_proc) {
    /*
     * Setup the ESM message container
     */
    attach_proc->esm_msg_out = esm_sap.send;

    /*
     * Send attach accept message to the UE
     */
    rc = esm_cn_wrapper_attach_accept (emm_context);

    if (rc != RETURNerror) {
      if (IS_EMM_CTXT_PRESENT_OLD_GUTI(emm_context) &&
          (memcmp(&emm_context->_old_guti, &emm_context->_guti, sizeof(emm_context->_guti)))) {
        /*
         * Implicit GUTI reallocation;
         * Notify EMM that common procedure has been initiated
         * LG: TODO check this, seems very suspicious
         */
        emm_sap_t                               emm_sap = {0};

        emm_sap.primitive = EMMREG_COMMON_PROC_REQ;
        emm_sap.u.emm_reg.ue_id = msg_pP->ue_id;
        emm_sap.u.emm_reg.ctx  = emm_context;

        MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_COMMON_PROC_REQ ue id " MME_UE_S1AP_ID_FMT " ", msg_pP->ue_id);

        rc = emm_sap_send (&emm_sap);
      }
    }
  } else if (is_standalone) {
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
  }else {
    OAILOG_ERROR(LOG_NAS_ESM, "This is an error, for no other procedure NAS layer should be informed about the PDN Connectivity. It happens before NAS for UE Id " MME_UE_S1AP_ID_FMT". \n", msg_pP->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
  }
//  unlock_ue_contexts(ue_context);

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
static int _esm_cn_pdn_connectivity_fail (const esm_cn_pdn_fail_t * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNok;
  struct emm_data_context_s              *emm_context = NULL;
  ESM_msg                                 esm_msg = {.header = {0}};
  int                                     esm_cause;
  emm_context = emm_data_context_get (&_emm_data, msg->ue_id);
  if (emm_context == NULL) {
    OAILOG_ERROR (LOG_NAS_ESM, "EMMCN-SAP  - " "Failed to find UE associated to id " MME_UE_S1AP_ID_FMT "...\n", msg->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
  }
  memset (&esm_msg, 0, sizeof (ESM_msg));

  // Map S11 cause to ESM cause
  switch (msg->cause) {
    case CAUSE_CONTEXT_NOT_FOUND:
      esm_cause = ESM_CAUSE_REQUEST_REJECTED_BY_GW;
      break;
    case CAUSE_INVALID_MESSAGE_FORMAT:
      esm_cause = ESM_CAUSE_REQUEST_REJECTED_BY_GW;
      break;
    case CAUSE_SERVICE_NOT_SUPPORTED:
      esm_cause = ESM_CAUSE_SERVICE_OPTION_NOT_SUPPORTED;
      break;
    case CAUSE_SYSTEM_FAILURE:
      esm_cause = ESM_CAUSE_NETWORK_FAILURE;
      break;
    case CAUSE_NO_RESOURCES_AVAILABLE:
      esm_cause = ESM_CAUSE_INSUFFICIENT_RESOURCES;
      break;
    case CAUSE_ALL_DYNAMIC_ADDRESSES_OCCUPIED:
      esm_cause = ESM_CAUSE_INSUFFICIENT_RESOURCES;
      break;
    default:
      esm_cause = ESM_CAUSE_REQUEST_REJECTED_BY_GW;
      break;
  }

  if(is_nas_specific_procedure_attach_running(emm_context)){
    OAILOG_ERROR (LOG_NAS_ESM, "EMMCN-SAP  - " "Sending Attach/PDN Connectivity Reject message to id " MME_UE_S1AP_ID_FMT "...\n", msg->ue_id);

    rc = esm_send_pdn_connectivity_reject (msg->pti, &esm_msg.pdn_connectivity_reject, esm_cause);
    /*
     * Encode the returned ESM response message
     */
    uint8_t                             esm_cn_sap_buffer[esm_cn_SAP_BUFFER_SIZE];
    int size = esm_msg_encode (&esm_msg, esm_cn_sap_buffer, esm_cn_SAP_BUFFER_SIZE);
    OAILOG_INFO (LOG_NAS_ESM, "ESM encoded MSG size %d\n", size);

    if (size > 0) {
      nas_emm_attach_proc_t  *attach_proc = get_nas_specific_procedure_attach(emm_context);
      /*
       * Setup the ESM message container
       */
      if(attach_proc){
        /** Sending the PDN connection reject inside a Attach Reject to the UE. */
        attach_proc->esm_msg_out = blk2bstr(esm_cn_sap_buffer, size);
        rc = emm_proc_attach_reject (msg->ue_id, EMM_CAUSE_ESM_FAILURE);
      }else{
        // todo: send the pdn disconnect reject as a standalone message to the UE.
        // todo: must clean the created pdn_context elements (no bearers should exist).
      }
    }
  }else if (is_nas_specific_procedure_tau_running(emm_context)){
    OAILOG_ERROR (LOG_NAS_ESM, "EMMCN-SAP  - " "Sending TAU Reject message to id " MME_UE_S1AP_ID_FMT "..\n", msg->ue_id);

    nas_emm_tau_proc_t  *tau_proc = get_nas_specific_procedure_tau(emm_context);
    rc = emm_proc_tracking_area_update_reject(msg->ue_id, EMM_CAUSE_ESM_FAILURE);
  }else{
    /** Forward the PDN Connectivity to the ESM layer. */
    OAILOG_DEBUG(LOG_NAS_ESM, "EMMCN-SAP  - " "Forwarding PDN Connectivity Failure for mmeUeS1apId " MME_UE_S1AP_ID_FMT " and enbId %d ..\n", msg->ue_id, msg->linked_ebi);
    esm_sap_t                               esm_sap = {0};
    esm_sap.primitive = ESM_PDN_CONNECTIVITY_REJ;
    esm_sap.ue_id = msg->ue_id;
    esm_sap.ctx = emm_context;
    esm_sap.recv = NULL;
    // todo: how to verify that esm proc is unique?!
    esm_sap.data.pdn_connect.linked_ebi = msg->linked_ebi; // pdn_context->default_ebi; /**< Default Bearer Id of default APN. */
// todo:   esm_sap.data.pdn_connect.cid = emm_context->esm_ctx.esm_proc_data->pdn_cid;
    esm_sap.data.pdn_connect.esm_cause = ESM_CAUSE_NETWORK_FAILURE;
    rc = esm_sap_send(&esm_sap);
    OAILOG_FUNC_RETURN(LOG_NAS_ESM, rc);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
static int _esm_cn_pdn_disconnect_res(const esm_cn_pdn_disconnect_res_t * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNok;
  struct emm_data_context_s              *emm_context = NULL;
  ESM_msg                                 esm_msg = {.header = {0}};
  int                                     esm_cause;
  esm_sap_t                               esm_sap = {0};
  bool                                    pdn_local_delete = false;

  ue_context_t *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, msg->ue_id);
  DevAssert(ue_context); // todo:

  emm_context = emm_data_context_get (&_emm_data, msg->ue_id);
  if (emm_context == NULL) {
    OAILOG_ERROR (LOG_NAS_ESM, "EMMCN-SAP  - " "Failed to find UE associated to id " MME_UE_S1AP_ID_FMT "...\n", msg->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
  }
  memset (&esm_msg, 0, sizeof (ESM_msg));

  /** todo: Inform the ESM layer about the PDN session removal. */

  // todo: check if detach_proc is active or UE is in DEREGISTRATION_INITIATED state, dn's

  /*
   * Check if there is a specific detach procedure running.
   * In that case, check if other PDN contexts exist. Delete those sessions, too.
   * Finally send Detach Accept back to the UE.
   */
  nas_emm_detach_proc_t  *detach_proc = get_nas_specific_procedure_detach(emm_context);
  // todo: implicit detach ? specific procedure?
  if(detach_proc || EMM_DEREGISTERED_INITIATED == emm_fsm_get_state(emm_context)){
    OAILOG_INFO (LOG_NAS_ESM, "Detach procedure ongoing for UE " MME_UE_S1AP_ID_FMT". Performing local PDN context deletion. \n", emm_context->ue_id);
    pdn_local_delete = true;
  }
  /** Check for the number of pdn connections left. */
  if(false /*!emm_context->esm_ctx.n_pdns */){
    OAILOG_INFO (LOG_NAS_ESM, "No more PDN contexts exist for UE " MME_UE_S1AP_ID_FMT". Continuing with detach procedure. \n", emm_context->ue_id);
    /** If a specific detach procedure is running, check if it is switch, off, if so, send detach accept back. */
    if(detach_proc){
      if (!detach_proc->ies->switch_off) {
        /*
         * Normal detach without UE switch-off
         */
        emm_sap_t                               emm_sap = {0};
        emm_as_data_t                          *emm_as = &emm_sap.u.emm_as.u.data;

        MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMM_AS_NAS_INFO_DETACH ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
        /*
         * Setup NAS information message to transfer
         */
        emm_as->nas_info = EMM_AS_NAS_INFO_DETACH;
        emm_as->nas_msg = NULL;
        /*
         * Set the UE identifier
         */
        emm_as->ue_id = emm_context->ue_id;
        /*
         * Setup EPS NAS security data
         */
        emm_as_set_security_data (&emm_as->sctx, &emm_context->_security, false, true);
        /*
         * Notify EMM-AS SAP that Detach Accept message has to
         * be sent to the network
         */
        emm_sap.primitive = EMMAS_DATA_REQ;
        rc = emm_sap_send (&emm_sap);
      }
    }
    /** Detach the UE. */
    if (rc != RETURNerror) {

      mme_ue_s1ap_id_t old_ue_id = emm_context->ue_id;

      emm_sap_t                               emm_sap = {0};

      /*
       * Notify EMM FSM that the UE has been implicitly detached
       */
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_DETACH_CNF ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
      emm_sap.primitive = EMMREG_DETACH_CNF;
      emm_sap.u.emm_reg.ue_id = emm_context->ue_id;
      emm_sap.u.emm_reg.ctx = emm_context;
      emm_sap.u.emm_reg.free_proc = true;
      rc = emm_sap_send (&emm_sap);
      // Notify MME APP to remove the remaining MME_APP and S1AP contexts..
      nas_itti_detach_req(old_ue_id);
      // todo: review unlock
//      unlock_ue_contexts(ue_context);
      OAILOG_FUNC_RETURN(LOG_NAS_ESM, rc);

    } else{
      // todo:
      DevAssert(0);
    }
  }else{
// todo:   OAILOG_INFO (LOG_NAS_ESM, "%d more PDN contexts exist for UE " MME_UE_S1AP_ID_FMT". Continuing with PDN Disconnect procedure. \n", emm_context->esm_ctx.n_pdns, emm_context->ue_id);
    /*
     * Send Disconnect Request to the ESM layer.
     * It will disconnect the first PDN which is pending deactivation.
     * This can be called at detach procedure with multiple PDNs, too.
     *
     * If detach procedure, or MME initiated PDN context deactivation procedure, we will first locally remove the PDN context.
     * The remaining PDN contexts will not be those who need NAS messaging to be purged, but the ones, who were not purged locally at all.
     */
    if(!pdn_local_delete){
      esm_sap.primitive = ESM_PDN_DISCONNECT_CNF;
      esm_sap.ue_id = emm_context->ue_id;
      esm_sap.ctx = emm_context;
      esm_sap.recv = NULL;
      esm_sap.data.pdn_disconnect.local_delete = pdn_local_delete; // pdn_context->default_ebi; /**< Default Bearer Id of default APN. */
      esm_sap_send(&esm_sap);
      OAILOG_FUNC_RETURN(LOG_NAS_ESM, rc);
    }else{
      OAILOG_INFO (LOG_NAS_ESM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Non Standalone & local detach triggered PDN deactivation. \n", emm_context->ue_id);
      esm_sap_t                               esm_sap = {0};
      /** Send Disconnect Request to all PDNs. */
      pdn_context_t * pdn_context = RB_MIN(PdnContexts, &ue_context->pdn_contexts);

      esm_sap.primitive = ESM_PDN_DISCONNECT_REQ;
      esm_sap.ue_id = emm_context->ue_id;
      esm_sap.ctx = emm_context;
//   todo:   esm_sap.recv = emm_context->esm_msg;
      esm_sap.data.pdn_disconnect.default_ebi = pdn_context->default_ebi; /**< Default Bearer Id of default APN. */
      esm_sap.data.pdn_disconnect.cid         = pdn_context->context_identifier; /**< Context Identifier of default APN. */
      esm_sap.data.pdn_disconnect.local_delete = true;                            /**< Local deletion of PDN contexts. */
      esm_sap_send(&esm_sap);
      /*
       * Remove the contexts and send S11 Delete Session Request to the SAE-GW.
       * Will continue with the detach procedure when S11 Delete Session Response arrives.
       */

      /**
       * Not waiting for a response. Will assume that the session is correctly purged.. Continuing with the detach and assuming that the SAE-GW session is purged.
       * Assuming, that in 5G AMF/SMF structure, it is like in PCRF, sending DELETE_SESSION_REQUEST and not caring about the response. Assuming the session is deactivated.
       */
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
    }
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
static int _esm_cn_activate_dedicated_bearer_req (esm_eps_activate_eps_bearer_ctx_req_t * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNok;
  /** Like PDN Config Response, directly forwarded to ESM. */
  // forward to ESM
  esm_sap_t                               esm_sap = {0};

  emm_data_context_t *emm_context = emm_data_context_get( &_emm_data, msg->ue_id);

  esm_sap.primitive = ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_REQ;
  esm_sap.ctx           = emm_context;
  esm_sap.ue_id         = msg->ue_id;
  memcpy((void*)&esm_sap.data.eps_dedicated_bearer_context_activate, msg, sizeof(esm_eps_activate_eps_bearer_ctx_req_t));

  MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_REQ ue id " MME_UE_S1AP_ID_FMT, esm_sap.ue_id);

  rc = esm_sap_send (&esm_sap);

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
static int _esm_cn_modify_eps_bearer_ctx_req (esm_eps_modify_eps_bearer_ctx_req_t * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNok;
  /** Like PDN Config Response, directly forwarded to ESM. */
  // forward to ESM
  esm_sap_t                               esm_sap = {0};

  emm_data_context_t *emm_context = emm_data_context_get( &_emm_data, msg->ue_id);

  esm_sap.primitive = ESM_EPS_BEARER_CONTEXT_MODIFY_REQ;
  esm_sap.ctx           = emm_context;
  esm_sap.ue_id         = msg->ue_id;
  memcpy((void*)&esm_sap.data.eps_bearer_context_modify, msg, sizeof(esm_cn_modify_eps_bearer_ctx_req_t));

  MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_EPS_BEARER_CONTEXT_MODIFY_REQ ue id " MME_UE_S1AP_ID_FMT, esm_sap.ue_id);

  rc = esm_sap_send (&esm_sap);

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
static int _esm_cn_deactivate_dedicated_bearer_req (esm_cn_deactivate_dedicated_bearer_req_t * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNok;
  /** Like PDN Config Response, directly forwarded to ESM. */
  // forward to ESM
  esm_sap_t                               esm_sap = {0};

  emm_data_context_t *emm_context = emm_data_context_get( &_emm_data, msg->ue_id);

  esm_sap.primitive = ESM_DEDICATED_EPS_BEARER_CONTEXT_DEACTIVATE_REQ;
  esm_sap.ctx           = emm_context;
  esm_sap.ue_id         = msg->ue_id;
  memcpy((void*)&esm_sap.data.eps_dedicated_bearer_context_deactivate, msg, sizeof(esm_cn_deactivate_dedicated_bearer_req_t));

  MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_DEDICATED_EPS_BEARER_CONTEXT_DEACTIVATE_REQ ue id " MME_UE_S1AP_ID_FMT, esm_sap.ue_id);

  rc = esm_sap_send (&esm_sap);

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
static int _esm_cn_modify_esm_bearer_ctxs_req (esm_eps_modify_eps_bearer_ctx_req_t * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNok;
  /** Like PDN Config Response, directly forwarded to ESM. */
  // forward to ESM
  esm_sap_t                               esm_sap = {0};

  emm_data_context_t *emm_context = emm_data_context_get( &_emm_data, msg->ue_id);

  esm_sap.primitive = ESM_EPS_UPDATE_ESM_BEARER_CTXS_REQ;
  esm_sap.ctx           = emm_context;
  esm_sap.ue_id         = msg->ue_id;
  memcpy((void*)&esm_sap.data.eps_update_esm_bearer_ctxs, msg, sizeof(esm_cn_update_esm_bearer_ctxs_req_t)); // todo: pointer directly?

  MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_UPDATE_ESM_BEARER_CTXS_REQ ue id " MME_UE_S1AP_ID_FMT, esm_sap.ue_id);

  rc = esm_sap_send (&esm_sap);

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

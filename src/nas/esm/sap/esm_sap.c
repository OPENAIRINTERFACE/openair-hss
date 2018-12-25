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
  Source      esm_sap.c

  Version     0.1

  Date        2012/11/22

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Frederic Maurel

  Description Defines the ESM Service Access Points at which the EPS
        Session Management sublayer provides procedures for the
        EPS bearer context handling and resources allocation.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "bstrlib.h"
#include "assertions.h"

#include "log.h"
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "mme_app_defs.h"
#include "mme_app_itti_messaging.h"
#include "nas_message.h"
#include "common_defs.h"
#include "TLVDecoder.h"
#include "esm_ebr.h"
#include "esm_proc.h"
#include "esm_cause.h"
#include "esm_msg.h"
#include "esm_recv.h"
#include "esm_msgDef.h"
#include "esm_sap.h"
#include "esm_message.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
   String representation of ESM-SAP primitives
*/
static const char                      *_esm_sap_primitive_str[] = {
    "ESM_PDN_CONFIG_RES",
    "ESM_PDN_CONFIG_FAIL",
    "ESM_PDN_CONNECTIVITY_CNF",
    "ESM_PDN_CONNECTIVITY_REJ",
    "ESM_PDN_DISCONNECT_CNF",
    "ESM_PDN_DISCONNECT_REJ",

    "ESM_EPS_BEARER_CONTEXT_ACTIVATE_REQ",
    "ESM_EPS_BEARER_CONTEXT_MODIFY_REQ",
    "ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ",
    "ESM_DETACH_IND",

    "ESM_TIMEOUT_IND",
};


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    esm_sap_initialize()                                      **
 **                                                                        **
 ** Description: Initializes the ESM Service Access Point state machine    **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    NONE                                       **
 **                                                                        **
 ***************************************************************************/
void
esm_sap_initialize (
  void)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  /*
   * Initialize ESM state machine
   */
  //esm_fsm_initialize();
  OAILOG_FUNC_OUT (LOG_NAS_ESM);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_sap_send()                                            **
 **                                                                        **
 ** Description: Processes the ESM Service Access Point primitive          **
 **                                                                        **
 ** Inputs:  msg:       The ESM-SAP primitive to process           **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void
esm_sap_signal(esm_sap_t * msg, bstring *rsp)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  pdn_cid_t                               pid = MAX_APN_PER_UE;
  ESM_msg                                 esm_resp_msg;
  ebi_t                                   ebi = 0;

  /*
   * Check the ESM-SAP primitive
   */

  memset((void*)&esm_resp_msg, 0, sizeof(ESM_msg));
  msg->esm_cause = ESM_CAUSE_SUCCESS;

  assert ((msg->primitive > ESM_START) && (msg->primitive < ESM_END));
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Received primitive %s (%d)\n", _esm_sap_primitive_str[msg->primitive - ESM_START - 1], msg->primitive);

  switch (msg->primitive) {

  case ESM_DETACH_IND:{
    pti_t pti = PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED;
    esm_proc_detach_request(msg->ue_id);
  }
  break;

  case ESM_PDN_CONFIG_RES:{
    pti_t pti = PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED;
    msg->esm_cause = esm_proc_pdn_config_res(msg->ue_id, &msg->is_attach, &pti, msg->data.pdn_config_res->imsi64);
    if(msg->esm_cause != ESM_CAUSE_SUCCESS) {
      /*
       * Send a PDN connectivity reject.
       */
      esm_send_pdn_connectivity_reject(pti, &esm_resp_msg, msg->esm_cause);
    }
  }
  break;

  case ESM_PDN_CONFIG_FAIL:{
    /*
     * Check if a procedure exists for PDN Connectivity. If so continue with it.
     */
    nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity = _esm_proc_get_pdn_connectivity_procedure(msg->ue_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED);
    if(!esm_proc_pdn_connectivity){
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - No ESM transaction for UE ueId " MME_UE_S1AP_ID_FMT " exists. Ignoring the received ULA. \n",
          msg->ue_id);
    } else{
      msg->is_attach = esm_proc_pdn_connectivity->is_attach;
      /** Send a PDN Connectivity Reject back. */
      esm_send_pdn_connectivity_reject(esm_proc_pdn_connectivity->esm_base_proc.pti, &esm_resp_msg, msg->esm_cause);
      /** Directly process the ULA. A response message might be returned. */
      _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
    }
  }
  break;

  case ESM_PDN_CONNECTIVITY_CNF:{
    /*
     * Check if a procedure exists for PDN Connectivity. If so continue with it.
     */
    pdn_context_t                       * pdn_context                 = NULL;
    bearer_context_t                    * bearer_context              = NULL;
    nas_esm_proc_pdn_connectivity_t     * esm_proc_pdn_connectivity   = _esm_proc_get_pdn_connectivity_procedure(msg->ue_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED);

    if(!esm_proc_pdn_connectivity){
      OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - No ESM transaction for UE ueId " MME_UE_S1AP_ID_FMT " exists. Ignoring the received PDN Connectivity confirmation. \n",
          msg->ue_id);
    } else {
      msg->is_attach = esm_proc_pdn_connectivity->is_attach;
      msg->esm_cause = esm_proc_pdn_connectivity_res(msg->ue_id, esm_proc_pdn_connectivity,
          &msg->data.pdn_connectivity_res->apn_ambr, &msg->data.pdn_connectivity_res->bearer_qos,
          msg->data.pdn_connectivity_res->pdn_type, msg->data.pdn_connectivity_res->paa);
      if(msg->esm_cause != ESM_CAUSE_SUCCESS) {
        /*
         * Send a PDN connectivity reject.
         * The ESM procedure should have been removed by now.
         */
        esm_send_pdn_connectivity_reject(esm_proc_pdn_connectivity->esm_base_proc.pti, &esm_resp_msg, msg->esm_cause);
      } else {
        /*
         * Directly process the PDN connectivity confirmation.
         * Activate Default Bearer Req or PDN connectivity reject is expected.
         * The procedure exists until Activate Default Bearer Accept is received.
         */
        esm_proc_default_eps_bearer_context(msg->ue_id, esm_proc_pdn_connectivity);
        /*
         * Send an Activate Default Bearer Request message.
         * Check for attach to use the correct callback method.
         */
        esm_send_activate_default_eps_bearer_context_request(esm_proc_pdn_connectivity,
            &msg->data.pdn_connectivity_res->apn_ambr, &msg->data.pdn_connectivity_res->bearer_qos,
            msg->data.pdn_connectivity_res->paa, &esm_resp_msg);
        msg->data.pdn_connectivity_res->paa = NULL;  /**< Unlink the PAA (only here). */
      }
    }
  }
  break;

  case ESM_PDN_CONNECTIVITY_REJ:{
    /*
     * Check if a procedure exists for PDN Connectivity. If so continue with it.
     */
    nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity = _esm_proc_get_pdn_connectivity_procedure(msg->ue_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED);
    if(!esm_proc_pdn_connectivity){
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - No ESM transaction for UE ueId " MME_UE_S1AP_ID_FMT " exists. Ignoring the received PDN Connectivity reject. \n",
          msg->ue_id);
    } else {
      msg->is_attach = esm_proc_pdn_connectivity->is_attach;
      /*
       * The ESM procedure will be removed and any timer will be stopped.
       * Also the pending PDN context will be removed.
       */
      esm_send_pdn_connectivity_reject(esm_proc_pdn_connectivity->esm_base_proc.pti, &esm_resp_msg, ESM_CAUSE_NETWORK_FAILURE);
      esm_proc_pdn_connectivity_failure(msg->ue_id, esm_proc_pdn_connectivity);
      msg->esm_cause = ESM_CAUSE_REQUEST_REJECTED_BY_GW;
    }
  }
  break;

  /*
   * Ignore the case of disconnectivity failure.
   * This message should not be sent from the core, in case of detach.
   */
  case ESM_PDN_DISCONNECT_CNF:
  case ESM_PDN_DISCONNECT_REJ:{
    /*
     * Check if a procedure exists for PDN Connectivity. If so continue with it.
     */
    pti_t                                pti                         = PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED;
    msg->esm_cause = esm_proc_eps_bearer_context_deactivate_request(msg->ue_id, &pti,
        &msg->data.pdn_disconnect_res->ebi, &esm_resp_msg);
    if(msg->esm_cause != ESM_CAUSE_SUCCESS){
      esm_send_pdn_disconnect_reject(pti, &esm_resp_msg, msg->esm_cause);
    }
  }
  break;

  /*
   * Dedicated EPS Bearer Context Handling
   */
  case ESM_EPS_BEARER_CONTEXT_ACTIVATE_REQ:{
    /*
     * Check that no network initiated procedure exists.
     * If so, reject it (no response is sent).
     * Else process it.
     */
    msg->esm_cause = esm_proc_dedicated_eps_bearer_context (msg->ue_id,     /**< Create an ESM procedure and store the bearers in the procedure as pending. */
        PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED,                     /**< No UE triggered bearer context activation is supported. */
        msg->data.eps_bearer_context_activate.linked_ebi,
        msg->data.eps_bearer_context_activate.pdn_cid,
        msg->data.eps_bearer_context_activate.bc_tbc,
        &esm_resp_msg);
    /** For each bearer separately process with the bearer establishment. */
    if (msg->esm_cause != ESM_CAUSE_SUCCESS) {   /**< We assume that no ESM procedure exists. */
      /**
       * Send a NAS ITTI directly for the specific bearer. This will reduce the number of bearers to be processed.
       * No bearer should be allocated and no ESM procedure should be created.
       * We will check the remaining bearer requests and reject bearers individually (we might have a mix of rejected and accepted bearers).
       * The remaining must also be rejected, such that the procedure has no pending elements anymore.
       */
      nas_itti_activate_eps_bearer_ctx_rej(msg->ue_id, msg->data.eps_bearer_context_activate.bc_tbc->s1u_sgw_fteid.teid, msg->esm_cause); /**< Assuming, no other CN bearer procedure will intervene. */
    }
  }
  break;

  case ESM_EPS_BEARER_CONTEXT_MODIFY_REQ:{
    msg->esm_cause = esm_proc_modify_eps_bearer_context(msg->ue_id,     /**< Create an ESM procedure and store the bearers in the procedure as pending. */
           PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED,
           msg->data.eps_bearer_context_modify.linked_ebi,
           msg->data.eps_bearer_context_modify.pdn_cid,
           msg->data.eps_bearer_context_modify.bc_tbu,
           &msg->data.eps_bearer_context_modify.apn_ambr,
           &esm_resp_msg);
    /** For each bearer separately process with the bearer establishment. */
    if (msg->esm_cause != ESM_CAUSE_SUCCESS) {   /**< We assume that no ESM procedure exists. */
      /** No Procedure is expected for the error case, should be handled internally. */
      nas_itti_modify_eps_bearer_ctx_rej(msg->ue_id, msg->data.eps_bearer_context_modify.bc_tbu->eps_bearer_id, msg->esm_cause); /**< Assuming, no other CN bearer procedure will intervene. */
    }

  }
  break;

  case ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ:{
    msg->esm_cause = esm_proc_eps_bearer_context_deactivate_request(msg->ue_id, &msg->data.eps_bearer_context_deactivate.pti,
       &msg->data.eps_bearer_context_deactivate.ded_ebi, &esm_resp_msg);
   if (msg->esm_cause != ESM_CAUSE_SUCCESS) {   /**< We assume that no ESM procedure exists. */
     /* Only if no bearer context. */
     nas_itti_dedicated_eps_bearer_deactivation_complete(msg->ue_id, msg->data.eps_bearer_context_deactivate.ded_ebi);
   }
  }
  break;

  case ESM_TIMEOUT_IND: {
    /*
     * Get the procedure of the timer.
     */
    nas_esm_proc_t * esm_base_proc = (nas_esm_proc_t*)msg->data.esm_proc_timeout;
    if(!esm_base_proc){
      OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - No procedure for timeout found.\n");
    }else {
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Timer expired for transaction (type=%d, ue_id=" MME_UE_S1AP_ID_FMT "), " "retransmission counter = %d\n",
          esm_base_proc->type, esm_base_proc->ue_id, esm_base_proc->retx_count);
      /**
       * Process the timeout.
       * Encode the returned message. Currently, building and encoding a new message every time.
       */
      esm_base_proc->timeout_notif(esm_base_proc);
    }
  }
  break;

  default:
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Received unhandled primitive %s (%d). No response message is expected. \n", _esm_sap_primitive_str[msg->primitive - ESM_START - 1], msg->primitive);
    break;
  }

  DevAssert(!*rsp);
  /*
   * ESM message processing failed and no response/reject message is received.
   * No status message for signaling.
   */
  if (esm_resp_msg.header.message_type) {
#define ESM_SAP_BUFFER_SIZE 4096
    uint8_t           esm_sap_buffer[ESM_SAP_BUFFER_SIZE];
    /*
     * Encode the returned ESM response message
     */
    int size = esm_msg_encode (&esm_resp_msg, (uint8_t *) esm_sap_buffer, ESM_SAP_BUFFER_SIZE);
    if (size > 0) {
      *rsp = blk2bstr(esm_sap_buffer, size);
      /* Send Attach Reject. */
      if(msg->is_attach) {
        if(msg->esm_cause != ESM_CAUSE_SUCCESS){
          _emm_wrapper_attach_reject(msg->ue_id, *rsp);
          bdestroy_wrapper(rsp); /**< Will be copied inside into the esm message. Remove it here. */
        }
      }
    }
  }
  esm_msg_free(&esm_resp_msg);
  OAILOG_FUNC_OUT(LOG_NAS_ESM);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    _esm_sap_recv()                                           **
 **                                                                        **
 ** Description: Processes ESM messages received from the network: Decodes **
 **      the message and checks whether it is of the expected ty-  **
 **      pe, checks the validity of the procedure transaction iden-**
 **      tity, checks the validity of the EPS bearer identity, and **
 **      parses the message content.                               **
 **      If no protocol error is found the ESM response message is **
 **      returned in order to be sent back onto the network upon   **
 **      the relevant ESM procedure completion.                    **
 **      If a protocol error is found the ESM status message is    **
 **      returned including the value of the ESM cause code.       **
 **                                                                        **
 ** Inputs:
 **      ue_id:      UE identifier within the MME               **
 **      req:       The encoded ESM request message to process **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     rsp:       The encoded ESM response message to be re- **
 **             turned upon ESM procedure completion to lower layers. Will be
 **             processed in the state of the EMM context.     **
 **      err:       Error code of the ESM procedure            **
 **      Return:    RETURNok, RETURNerror :
 **       error if the message could not be processed,         **
 **       will be signaled to EMM directly. EMM will decide on **
 **       to discard the error or abort the EM procedure.      **
 **                                                                        **
 ***************************************************************************/
void
_esm_sap_recv (
  mme_ue_s1ap_id_t    mme_ue_s1ap_id,
  imsi_t             *imsi,
  tai_t              *visited_tai,
  const_bstring       req,
  bstring            *rsp)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  ESM_msg                                 esm_msg, esm_resp_msg;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  bool                                    is_attach = false;
  int                                     decoder_rc;

  /*
   * Decode the received ESM message
   */
  memset(&esm_msg, 0, sizeof(ESM_msg));
  memset(&esm_resp_msg, 0, sizeof(ESM_msg));

  decoder_rc = esm_msg_decode (&esm_msg, (uint8_t *)bdata(req), blength(req));

  /*
   * Process decoding errors.
   * Respond with an ESM status message back.
   * The ESM Cause will be evaluated in the lower layers.
   */
  if (decoder_rc < 0) {
    /*
     * 3GPP TS 24.301, section 7.2
     * * * * Ignore received message that is too short to contain a complete
     * * * * message type information element
     */
    if (decoder_rc == TLV_BUFFER_TOO_SHORT) {
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Discard message too short to contain a complete message type IE\n");
      /*
       * Return indication that received message has been discarded
       */
      esm_cause = ESM_CAUSE_MESSAGE_NOT_COMPATIBLE;
    }
    /*
     * 3GPP TS 24.301, section 7.2
     * * * * Unknown or unforeseen message type
     */
    else if (decoder_rc == TLV_WRONG_MESSAGE_TYPE) {
      esm_cause = ESM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED;
    }
    /*
     * 3GPP TS 24.301, section 7.7.2
     * * * * Conditional IE errors
     */
    else if (decoder_rc == TLV_UNEXPECTED_IEI) {
      esm_cause = ESM_CAUSE_CONDITIONAL_IE_ERROR;
    } else {
      esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
    }
  }
  /*
   * Get the procedure transaction identity
   */
  pti_t                            pti = esm_msg.header.procedure_transaction_identity;

  /*
   * Get the EPS bearer identity
   */
  ebi_t                            ebi = esm_msg.header.eps_bearer_identity;

  /*
   * Indicate whether the ESM bearer context related procedure was triggered
   * * * * by the receipt of a transaction related request message from the UE or
   * * * * was triggered network-internally
   */
  int                                     triggered_by_ue = (pti != PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED);

  if (esm_cause != ESM_CAUSE_SUCCESS) {
    OAILOG_ERROR (LOG_NAS_ESM , "ESM-SAP   - Failed to decode ESM message for UE " MME_UE_S1AP_ID_FMT " with cause %d. "
        "Will create a status message for unhandled message..\n", mme_ue_s1ap_id, esm_cause);
  } else{
    /*
     * Process the received ESM message.
     * In contrast to ESM signals, the ESM messages may be duplicates and the sends should be processed inside.
     */
    switch (esm_msg.header.message_type) {

    /** Initial Attach Messages. */

    case PDN_CONNECTIVITY_REQUEST: {
      OAILOG_DEBUG (LOG_NAS_ESM, "ESM-SAP   - PDN_CONNECTIVITY_REQUEST pti %u ebi %u\n", pti, ebi);
      /*
       * Process PDN connectivity request message received from the UE.
       */
      esm_cause = esm_recv_pdn_connectivity_request (&is_attach, mme_ue_s1ap_id, imsi, pti, ebi, visited_tai, &esm_msg.pdn_connectivity_request, &esm_resp_msg);
      if (esm_cause != ESM_CAUSE_SUCCESS) {
        /*
         * No transaction expected (not touching network triggered transactions.
         */
        esm_send_pdn_connectivity_reject(pti, &esm_resp_msg, esm_cause);
      }
    }
    break;

    case ESM_INFORMATION_RESPONSE: {
      esm_cause = esm_recv_information_response (&is_attach, mme_ue_s1ap_id, pti, ebi, &esm_msg.esm_information_response);
      if (esm_cause != ESM_CAUSE_SUCCESS) {
        /*
         * No transaction expected (not touching network triggered transactions).
         * Directly encode the message.
         */
        esm_send_pdn_connectivity_reject(pti, &esm_resp_msg, esm_cause);
      }
    }
    break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
      /*
       * Process activate default EPS bearer context accept message
       * received from the UE
       */
      esm_cause = esm_recv_activate_default_eps_bearer_context_accept (mme_ue_s1ap_id, pti, ebi, &esm_msg.activate_default_eps_bearer_context_accept);
      if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
        /*
         * 3GPP TS 24.301, section 7.3.1, case f
         * * * * Ignore ESM message received with reserved PTI value
         * * * * 3GPP TS 24.301, section 7.3.2, case f
         * * * * Ignore ESM message received with reserved or assigned
         * * * * value that does not match an existing EPS bearer context
         */
      }
      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT:
      /*
       * Process activate default EPS bearer context reject message
       * received from the UE
       */
      esm_cause = esm_recv_activate_default_eps_bearer_context_reject (mme_ue_s1ap_id, pti, ebi, &esm_msg.activate_default_eps_bearer_context_reject);

      if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
        /*
         * 3GPP TS 24.301, section 7.3.1, case f
         * * * * Ignore ESM message received with reserved PTI value
         * * * * 3GPP TS 24.301, section 7.3.2, case f
         * * * * Ignore ESM message received with reserved or assigned
         * * * * value that does not match an existing EPS bearer context
         */
      }
      /** Not triggering an attach reject, just removing the ESM context. */
      break;

    case PDN_DISCONNECT_REQUEST:
      /*
       * Process PDN disconnect request message received from the UE.
       * Get the Linked Default EBI.
       * In case the PTI is 0 or no PDN Context exist send a reject back (no PDN context exists).
       * Else send both an S11 Delete Session Request and an Deactivate Default EPS Context request.
       * Sending the ESM Request directly, makes retransmission handling easier. We are not interested in the outcome of the Delete Session Response.
       */
      esm_cause = esm_recv_pdn_disconnect_request (mme_ue_s1ap_id, pti, ebi, &esm_msg.pdn_disconnect_request, &esm_resp_msg);
      if (esm_cause != ESM_CAUSE_SUCCESS) {
        /*
         * Return reject message (if no PDN context is found or received PTI was 0 (implicitly detached PDN context)).
         *
         */
        esm_send_pdn_disconnect_reject (pti, &esm_resp_msg, esm_cause);
      }
      break;

    case DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT:
      /*
       * Process deactivate EPS bearer context accept message
       * received from the UE
       */
      esm_cause = esm_recv_deactivate_eps_bearer_context_accept (mme_ue_s1ap_id, pti, ebi, &esm_msg.deactivate_eps_bearer_context_accept);

      if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
        /*
         * 3GPP TS 24.301, section 7.3.1, case f
         * * * * Ignore ESM message received with reserved PTI value
         * * * * 3GPP TS 24.301, section 7.3.2, case f
         * * * * Ignore ESM message received with reserved or assigned
         * * * * value that does not match an existing EPS bearer context
         */
      }
      break;

      /** Dedicated Bearer only functions. */
    case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT:
      /*
       * Process activate dedicated EPS bearer context accept message
       * received from the UE
       */
      esm_cause = esm_recv_activate_dedicated_eps_bearer_context_accept (mme_ue_s1ap_id, pti, ebi, &esm_msg.activate_dedicated_eps_bearer_context_accept);

      if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
        /*
         * 3GPP TS 24.301, section 7.3.1, case f
         * * * * Ignore ESM message received with reserved PTI value
         *  * * * 3GPP TS 24.301, section 7.3.2, case f
         * * * * Ignore ESM message received with reserved or assigned
         * * * * value that does not match an existing EPS bearer context
         */
      }
      break;

    case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT:
      /*
       * Process activate dedicated EPS bearer context reject message
       * received from the UE
       */
      esm_cause = esm_recv_activate_dedicated_eps_bearer_context_reject (mme_ue_s1ap_id, pti, ebi, &esm_msg.activate_dedicated_eps_bearer_context_reject);

      if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
        /*
         * 3GPP TS 24.301, section 7.3.1, case f
         * * * * Ignore ESM message received with reserved PTI value
         * * * * 3GPP TS 24.301, section 7.3.2, case f
         * * * * Ignore ESM message received with reserved or assigned
         * * * * value that does not match an existing EPS bearer context
         */
      }
      break;

    case MODIFY_EPS_BEARER_CONTEXT_ACCEPT:
      /*
       * Process activate dedicated EPS bearer context accept message
       * received from the UE
       */
      esm_cause = esm_recv_modify_eps_bearer_context_accept( mme_ue_s1ap_id, pti, ebi, &esm_msg.modify_eps_bearer_context_accept);

      if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
        /*
         * 3GPP TS 24.301, section 7.3.1, case f
         * * * * Ignore ESM message received with reserved PTI value
         *  * * * 3GPP TS 24.301, section 7.3.2, case f
         * * * * Ignore ESM message received with reserved or assigned
         * * * * value that does not match an existing EPS bearer context
         */
      }
      break;

    case MODIFY_EPS_BEARER_CONTEXT_REJECT:
      /*
       * Process activate dedicated EPS bearer context reject message
       * received from the UE
       */
      esm_cause = esm_recv_modify_eps_bearer_context_reject(mme_ue_s1ap_id, pti, ebi, &esm_msg.modify_eps_bearer_context_reject);

      if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
        /*
         * 3GPP TS 24.301, section 7.3.1, case f
         * * * * Ignore ESM message received with reserved PTI value
         * * * * 3GPP TS 24.301, section 7.3.2, case f
         * * * * Ignore ESM message received with reserved or assigned
         * * * * value that does not match an existing EPS bearer context
         */
      }
      break;

    case BEARER_RESOURCE_ALLOCATION_REQUEST:
      break;

    case BEARER_RESOURCE_MODIFICATION_REQUEST:
      break;

    case ESM_STATUS:
      /*
       * Process received ESM status message
       */
      esm_cause = esm_recv_status (mme_ue_s1ap_id, pti, ebi, &esm_msg.esm_status);
      break;

    /** Non-EMM Related Messages. */

    default:
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Received unexpected ESM message " "0x%x\n", esm_msg.header.message_type);
      esm_cause = ESM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED;
      break;
    }
  }
  /*
   * ESM message processing failed and no response/reject message is received.
   */
  if (esm_cause != ESM_CAUSE_SUCCESS && !esm_resp_msg.header.message_type) {
    /*
     * 3GPP TS 24.301, section 7.1
     * * * * Handling of unknown, unforeseen, and erroneous protocol data
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Received ESM message is not valid " "(cause=%d). No reject message triggered, triggering an ESM status message. \n", esm_cause);
    /*
     * Return an ESM status message
     */
    esm_send_status (pti, ebi, &esm_resp_msg, esm_cause);
    /*
     * Consider this as an error case, may still return an encoded message but will call the error callback.
     */
  } else {
    /*
     * ESM message processing succeed
     */
  }
  if (esm_resp_msg.header.message_type) {
    uint8_t           esm_sap_buffer[ESM_SAP_BUFFER_SIZE];
    /*
     * Encode the returned ESM response message
     */
    int size = esm_msg_encode (&esm_resp_msg, (uint8_t *) esm_sap_buffer, ESM_SAP_BUFFER_SIZE);
    if (size > 0) {
      *rsp = blk2bstr(esm_sap_buffer, size);
      /* Send Attach Reject. */
      if(is_attach && esm_cause != ESM_CAUSE_SUCCESS){
        _emm_wrapper_attach_reject(mme_ue_s1ap_id, *rsp);
        /** Will be copied inside into the esm message. Remove it here. */
        bdestroy_wrapper(rsp);
      }
    }
  }
  esm_msg_free(&esm_msg);
  esm_msg_free(&esm_resp_msg);
  OAILOG_FUNC_OUT(LOG_NAS_ESM);
}

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
#include "mme_app_ue_context.h"
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

static int _esm_sap (
  mme_ue_s1ap_id_t mme_ue_s1ap_id,
  const_bstring req,
  esm_cause_t *esm_cause,
  bstring *rsp);

static int _esm_sap_send (
  int msg_type,
  esm_context_t * esm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const esm_sap_data_t * data,
  bstring * rsp);

/*
   String representation of ESM-SAP primitives
*/
static const char                      *_esm_sap_primitive_str[] = {
    "ESM_ATTACH_IND_RES",
    "ESM_PDN_CONFIG_RES",
    "ESM_PDN_CONFIG_REJ",
    "ESM_PDN_CONNECTIVITY_RES",
    "ESM_PDN_CONNECTIVITY_REJ",

//  "ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REQ",
//  "ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_CNF",
//  "ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REJ",
//  "ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_REQ",
//  "ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_CNF",
//  "ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_REJ",
//  "ESM_EPS_BEARER_CONTEXT_MODIFY_REQ",
//  "ESM_EPS_BEARER_CONTEXT_MODIFY_CNF",
//  "ESM_EPS_BEARER_CONTEXT_MODIFY_REJ",
//  "ESM_DEDICATED_EPS_BEARER_CONTEXT_DEACTIVATE_REQ",
//  "ESM_DEDICATED_EPS_BEARER_CONTEXT_DEACTIVATE_CNF",
//
//  "ESM_EPS_UPDATE_ESM_BEARER_CTXS_REQ",
//
//  "ESM_PDN_CONFIG_RES",
//
//  "ESM_PDN_CONNECTIVITY_REQ",
//  "ESM_PDN_CONNECTIVITY_CNF",
//  "ESM_PDN_CONNECTIVITY_REJ",
//  "ESM_PDN_DISCONNECT_REQ",
//  "ESM_PDN_DISCONNECT_CNF",
//  "ESM_PDN_DISCONNECT_REJ",
//  "ESM_BEARER_RESOURCE_ALLOCATE_REQ",
//  "ESM_BEARER_RESOURCE_ALLOCATE_REJ",
//  "ESM_BEARER_RESOURCE_MODIFY_REQ",
//  "ESM_BEARER_RESOURCE_MODIFY_REJ",
//  "ESM_UNITDATA_IND",
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
int
esm_sap_signal(esm_sap_t * msg, bstring *rsp, bool *is_attach)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  pdn_cid_t                               pid = MAX_APN_PER_UE;
  ebi_t                                   ebi = 0;

  /*
   * Check the ESM-SAP primitive
   */
  esm_primitive_t                         primitive   = msg->primitive;
  esm_cause_t                             esm_cause   = ESM_CAUSE_SUCCESS;

  pdn_context_t                          *pdn_context = NULL;
  pdn_context_t                          *established_pdn = NULL;
  ue_context_t                           *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, msg->ctx->ue_id);

  ESM_msg                                esm_resp_msg;

  memset((void*)&esm_resp_msg, 0, sizeof(ESM_msg));

  assert ((primitive > ESM_START) && (primitive < ESM_END));
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Received primitive %s (%d)\n", _esm_sap_primitive_str[primitive - ESM_START - 1], primitive);

  switch (primitive) {

  case ESM_PDN_CONFIG_RES:{
    /*
     * Check if a procedure exists for PDN Connectivity. If so continue with it.
     */
    nas_esm_pdn_connectivity_proc_t * esm_pdn_connectivity_proc = esm_data_get_procedure(msg->ue_id);
    if(!esm_pdn_connectivity_proc){
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - No ESM transaction for UE ueId " MME_UE_S1AP_ID_FMT " exists. Ignoring the received ULA. \n",
          msg->ue_id);
      rc = RETURNok;
    }else {
      is_attach = esm_pdn_connectivity_proc->is_attach;
      /** If no APN was set, we know have an APN. */
      DevAssert(nas_pdn_connectivity_proc->apn_subscribed);
      /**
       * For the case of PDN Connectivity via ESM, we will always trigger an S11 Create Session Request.
       * We won't enter this case in case of a Tracking Area Update procedure.
       */
      pdn_context_t *pdn_context_duplicate = NULL;
      mme_app_get_pdn_context(ue_context, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, ESM_EBI_UNASSIGNED, esm_pdn_connectivity_proc->apn_subscribed, &pdn_context_duplicate);
      if(pdn_context_duplicate){
        /*
         * This can only be initial. Discard the ULA (assume that the current procedure is still being worked on).
         * No message will be triggered below.
         */
        OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - Found an established PDN context for the APN \"%s\". Ignoring the received PDN configuration." "(ue_id=%d, pti=%d)\n",
            bdata(msg->accesspointname), mme_ue_s1ap_id, pti);
      }else {
        /** Directly process the ULA. A response message might be returned. */
        esm_cause = esm_proc_pdn_config_res(msg->ue_id, esm_pdn_connectivity_proc, msg->data.pdn_config_res.imsi64);
        if (esm_cause != ESM_CAUSE_SUCCESS) {
          /*
           * No transaction expected (not touching network triggered transactions).
           * Directly encode the message.
           */
          esm_send_pdn_connectivity_reject(esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.pti, &esm_resp_msg, esm_cause);
        }
        rc = RETURNok;
      }
    }
  }
  break;

  case ESM_PDN_CONFIG_FAIL:{
    /*
     * Check if a procedure exists for PDN Connectivity. If so continue with it.
     */
    nas_esm_pdn_connectivity_proc_t * esm_pdn_connectivity_proc = esm_data_get_procedure(msg->ue_id);
    if(!esm_pdn_connectivity_proc){
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - No ESM transaction for UE ueId " MME_UE_S1AP_ID_FMT " exists. Ignoring the received ULA. \n",
          msg->ue_id);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
    }
    /** Directly process the ULA. A response message might be returned. */
    esm_cause = esm_proc_pdn_config_fail(msg->ue_id, esm_pdn_connectivity_proc, &esm_resp_msg);
    if(esm_cause != ESM_CAUSE_SUCCESS) {
      /*
       * Send a PDN connectivity reject.
       */
      esm_send_pdn_connectivity_reject(esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.pti, &esm_resp_msg, esm_cause);
    }
    rc = RETURNok;
  }
  break;

  case ESM_PDN_CONNECTIVITY_CNF:{
    /*
     * Check if a procedure exists for PDN Connectivity. If so continue with it.
     */
    pdn_context_t                       * pdn_context                 = NULL;
    bearer_context_t                    * bearer_context              = NULL;
    nas_esm_pdn_connectivity_proc_t     * esm_pdn_connectivity_proc   = esm_data_get_procedure(msg->ue_id);

    if(!esm_pdn_connectivity_proc){
      OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - No ESM transaction for UE ueId " MME_UE_S1AP_ID_FMT " exists. Ignoring the received PDN Connectivity confirmation. \n",
          msg->ue_id);
    } else {
      is_attach = esm_pdn_connectivity_proc->is_attach;

      esm_proc_pdn_connectivity_res(msg->ue_id, esm_pdn_connectivity_proc, &msg->data.pdn_connectivity_res->bearer_qos, msg->data.pdn_connectivity_res->apn_ambr,
          &bearer_context, &pdn_context);

      /**
       * Directly process the PDN connectivity confirmation. Activate Default Bearer Req or PDN connectivity reject is expected.
       * The procedure exists until Activate Default Bearer Accept is received.
       */
      esm_cause = esm_proc_default_eps_bearer_context(msg->ue_id, esm_pdn_connectivity_proc);
      if(esm_cause != ESM_CAUSE_SUCCESS) {
        /*
         * Send a PDN connectivity reject.
         */
        esm_send_pdn_connectivity_reject(esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.pti, &esm_resp_msg, esm_cause);
      } else {
        DevAssert(pdn_context);
        DevAssert(bearer_context);
        /*
         * Send an Activate Default Bearer Request message.
         * Check for attach to use the correct callback method.
         */
        esm_send_activate_default_eps_bearer_context_request(esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.pti,
            esm_pdn_connectivity_proc, &esm_resp_msg, pdn_context, bearer_context);
      }
      rc = RETURNok;
    }
  }
  break;

  case ESM_PDN_CONNECTIVITY_REJ:{
    /*
     * Check if a procedure exists for PDN Connectivity. If so continue with it.
     */
    nas_esm_pdn_connectivity_proc_t * esm_pdn_connectivity_proc = esm_data_get_procedure(msg->ue_id);
    if(!esm_pdn_connectivity_proc){
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - No ESM transaction for UE ueId " MME_UE_S1AP_ID_FMT " exists. Ignoring the received PDN Connectivity reject. \n",
          msg->ue_id);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
    }
    is_attach = esm_pdn_connectivity_proc->is_attach;
    /*
     * The ESM procedure will be removed and any timer will be stopped.
     * Also the pending PDN context will be removed.
     */
    esm_cause = esm_proc_pdn_connectivity_fail(msg->ue_id, esm_pdn_connectivity_proc);
    esm_send_pdn_connectivity_reject(esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.pti, &esm_resp_msg, esm_cause);
    rc = RETURNok;
  }
  break;

  case ESM_TIMEOUT_IND: {
    /*
     * Get the procedure of the timer.
     */
    nas_esm_pdn_connectivity_proc_t * esm_pdn_connectivity_proc = esm_data_get_procedure(msg->ue_id);
    if(!esm_pdn_connectivity_proc){
      OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - No procedure for UE " MME_UE_S1AP_ID_FMT " found.\n", ue_id);
      rc = RETURNok;
    }else {
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Timer expired for transaction (type=%d, ue_id=" MME_UE_S1AP_ID_FMT "), " "retransmission counter = %d\n",
          esm_pdn_connectivity_proc->trx_base_proc.trx_type, ue_id, esm_pdn_connectivity_proc->trx_base_proc.esm_proc_timer.count);
      /**
       * Process the timeout.
       * Encode the returned message. Currently, building and encoding a new message every time.
       */
      rc = esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.timeout_notif(esm_pdn_connectivity_proc, &esm_resp_msg);
    }
  }
  break;

  default:
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Received unhandled primitive %s (%d). No response message is expected. \n", _esm_sap_primitive_str[primitive - ESM_START - 1], primitive);
    break;
  }

  if (rc != RETURNok) {
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Failed to process primitive %s (%d). No response message is expected. \n", _esm_sap_primitive_str[primitive - ESM_START - 1], primitive);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
  }
  if(rsp){
    OAILOG_DEBUG(LOG_NAS_ESM, "ESM-SAP   - An encoded response message for primitive %s (%d) already exists. Returning it. \n", _esm_sap_primitive_str[primitive - ESM_START - 1], primitive);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
  }
  /*
   * ESM message processing failed and no response/reject message is received.
   * No status message for signaling.
   */
  if (esm_resp_msg->header.message_type) {
    #define ESM_SAP_BUFFER_SIZE 4096
    uint8_t           esm_sap_buffer[ESM_SAP_BUFFER_SIZE];
    /*
     * Encode the returned ESM response message
     */
    int size = esm_msg_encode (esm_resp_msg, (uint8_t *) esm_sap_buffer, ESM_SAP_BUFFER_SIZE);

    if (size > 0) {
      *rsp = blk2bstr(esm_sap_buffer, size);
      /* Send Attach Reject. */
      if(is_attach) {
        if(esm_cause != ESM_CAUSE_SUCCESS){
          _emm_wrapper_attach_reject(msg->ue_id, rsp);
        } else if (primitive == ESM_PDN_CONNECTIVITY_CNF) {
          _emm_wrapper_attach_accept(msg->ue_id, rsp);
        }
        *rsp = NULL
        OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
      }
    }
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
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
int
_esm_sap_recv (
  mme_ue_s1ap_id_t    mme_ue_s1ap_id,
  imsi_t             *imsi,
  tai_t              *visited_tai,
  const_bstring       req,
  esm_cause_t        *esm_cause,
  bstring            *rsp)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  ESM_msg                                 esm_msg, esm_resp_msg;
  bool                                    is_attach = false;
  int                                     decoder_rc;
  int                                     rc = RETURNerror;

  *esm_cause = ESM_CAUSE_SUCCESS;
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
      *esm_cause = ESM_CAUSE_MESSAGE_NOT_COMPATIBLE
    }
    /*
     * 3GPP TS 24.301, section 7.2
     * * * * Unknown or unforeseen message type
     */
    else if (decoder_rc == TLV_WRONG_MESSAGE_TYPE) {
      *esm_cause = ESM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED;
    }
    /*
     * 3GPP TS 24.301, section 7.7.2
     * * * * Conditional IE errors
     */
    else if (decoder_rc == TLV_UNEXPECTED_IEI) {
      *esm_cause = ESM_CAUSE_CONDITIONAL_IE_ERROR;
    } else {
      *esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
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

  /*
   * Indicate whether the received message shall be ignored
   */
  bool                                     is_discarded = false;

  if (*esm_cause != ESM_CAUSE_SUCCESS) {
    OAILOG_ERROR (LOG_NAS_ESM , "ESM-SAP   - Failed to decode ESM message for UE " MME_UE_S1AP_ID_FMT " with cause %d. "
        "Will create a status message for unhandled message..\n", mme_ue_s1ap_id, *esm_cause);
  } else{
    /*
     * Process the received ESM message
     */
    switch (esm_msg.header.message_type) {

    /** Initial Attach Messages. */

    case PDN_CONNECTIVITY_REQUEST: {
      OAILOG_DEBUG (LOG_NAS_ESM, "ESM-SAP   - PDN_CONNECTIVITY_REQUEST pti %u ebi %u\n", pti, ebi);
      /*
       * Process PDN connectivity request message received from the UE
       */
      *esm_cause = esm_recv_pdn_connectivity_request (&is_attach, mme_ue_s1ap_id, imsi, pti, ebi, visited_tai, &esm_msg.pdn_connectivity_request, &esm_resp_msg);
      if (*esm_cause != ESM_CAUSE_SUCCESS) {
        /*
         * No transaction expected (not touching network triggered transactions.
         */
        esm_send_pdn_connectivity_reject(&esm_resp_msg, esm_cause);
      }
    }
    break;

    case ESM_INFORMATION_RESPONSE: {
      esm_cause = esm_recv_information_response (&is_attach, mme_ue_s1ap_id, pti, ebi, &esm_msg.esm_information_response, &esm_resp_msg);
      if (*esm_cause != ESM_CAUSE_SUCCESS) {
        /*
         * No transaction expected (not touching network triggered transactions).
         * Directly encode the message.
         */
        esm_send_pdn_connectivity_reject(&esm_resp_msg, esm_cause);
      }
    }
    break;

    /** Non-EMM Related Messages. */

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
      OAILOG_DEBUG (LOG_NAS_ESM, "ESM-SAP   - ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT pti %u ebi %u\n", pti, ebi);
      /** Set the bearer state as active and remove the transaction. */
      rc = esm_recv_activate_default_eps_bearer_context_accept(mme_ue_s1ap_id, pti, &esm_msg.activate_default_eps_bearer_context_accept);
    break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT:
      OAILOG_DEBUG (LOG_NAS_ESM, "ESM-SAP   - ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT pti %u ebi %u\n", pti, ebi);
      /** Remove the PDN context and the transaction. */
      esm_cause = esm_recv_activate_default_eps_bearer_context_reject(mme_ue_s1ap_id, pti, ebi, &esm_msg.activate_default_eps_bearer_context_reject, &esm_resp_msg);
    break;

    default:
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Received unexpected ESM message " "0x%x\n", esm_msg.header.message_type);
      *esm_cause = ESM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED;
      break;
    }
  }
  /*
   * ESM message processing failed and no response/reject message is received.
   */
  if (*esm_cause != ESM_CAUSE_SUCCESS && !esm_resp_msg.header.message_type) {
    /*
     * 3GPP TS 24.301, section 7.1
     * * * * Handling of unknown, unforeseen, and erroneous protocol data
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Received ESM message is not valid " "(cause=%d). No reject message triggered, triggering an ESM status message. \n", *esm_cause);
    /*
     * Return an ESM status message
     */
    esm_send_status (pti, ebi, &esm_resp_msg.esm_status, esm_cause);
    /*
     * Consider this as an error case, may still return an encoded message but will call the error callback.
     */
    rc = RETURNerror;
  } else {
    /*
     * ESM message processing succeed
     */
    rc = RETURNok;
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
        _emm_wrapper_attach_reject(mme_ue_s1ap_id, rsp);
        *rsp = NULL;
        OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
      }
    }
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _esm_sap_send()                                           **
 **                                                                        **
 ** Description: Processes ESM messages to send onto the network: Encoded  **
 **      the message and execute the relevant ESM procedure.       **
 **                                                                        **
 ** Inputs:  msg_type:  Type of the ESM message to be sent         **
 **      ue_id:      UE identifier within the MME               **
 **      pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      data:      Data required to build the message         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     rsp:       The encoded ESM response message to be re- **
 **             turned upon ESM procedure completion       **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
static int
_esm_sap_send (
  int msg_type,
  esm_context_t * esm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const esm_sap_data_t * data,
  bstring * rsp)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_proc_procedure_t                    esm_procedure = NULL;
  int                                     rc = RETURNok;

  /*
   * Indicate whether the message is sent by the UE or the MME
   */
  bool                                    sent_by_ue = false;
  ESM_msg                                 esm_msg;

  memset (&esm_msg, 0, sizeof (ESM_msg));
  esm_proc_pdn_type_t esm_pdn_type = 0;
  /*
   * Process the ESM message to send
   */
  switch (msg_type) {
  break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST: {
      const   esm_eps_activate_eps_bearer_ctx_req_t *msg = &data->eps_dedicated_bearer_context_activate;
      EpsQualityOfService eps_qos = {0};
      /** Sending a EBR-Request per bearer context. */
      bearer_contexts_to_be_created_t *bcs_tbc = (bearer_contexts_to_be_created_t*)msg->bcs_to_be_created_ptr;
      for(int num_bc = 0; num_bc < bcs_tbc->num_bearer_context; num_bc++) {
        if(bcs_tbc->bearer_contexts[num_bc].eps_bearer_id == ebi){
          memset((void*)&eps_qos, 0, sizeof(eps_qos));
          /** Set the EPS QoS. */
          rc = qos_params_to_eps_qos(bcs_tbc->bearer_contexts[num_bc].bearer_level_qos.qci,
              bcs_tbc->bearer_contexts[num_bc].bearer_level_qos.mbr.br_dl,bcs_tbc->bearer_contexts[num_bc].bearer_level_qos.mbr.br_ul,
              bcs_tbc->bearer_contexts[num_bc].bearer_level_qos.gbr.br_dl, bcs_tbc->bearer_contexts[num_bc].bearer_level_qos.gbr.br_ul,
              &eps_qos, false);
          if (RETURNok == rc) {
            rc = esm_send_activate_dedicated_eps_bearer_context_request (
                pti, bcs_tbc->bearer_contexts[num_bc].eps_bearer_id,
                &esm_msg.activate_dedicated_eps_bearer_context_request,
                msg->linked_ebi, &eps_qos,
                &bcs_tbc->bearer_contexts[num_bc].tft,
                &bcs_tbc->bearer_contexts[num_bc].pco);
            esm_procedure = esm_proc_dedicated_eps_bearer_context_request; /**< Not the procedure. */
          }
          /** Exit the loop directly. */
          OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Successfully sent request to establish dedicated bearer for ebi %d for UE " MME_UE_S1AP_ID_FMT " . \n",
              ebi, esm_context->ue_id);
          break;
        }
      }
    }
    /** Exit the case always here. */
    break;

  case MODIFY_EPS_BEARER_CONTEXT_REQUEST:{
    const   esm_eps_modify_eps_bearer_ctx_req_t *msg = &data->eps_bearer_context_modify;

    EpsQualityOfService eps_qos = {0};
    memset((void*)&eps_qos, 0, sizeof(eps_qos));
    /** Sending a EBR-Request per bearer context. */
    bearer_contexts_to_be_updated_t *bcs_tbu = (bearer_contexts_to_be_updated_t*)msg->bcs_to_be_updated_ptr;
    for(int num_bc = 0; num_bc < bcs_tbu->num_bearer_context; num_bc++){
      if(bcs_tbu->bearer_contexts[num_bc].eps_bearer_id == ebi){
        if(bcs_tbu->bearer_contexts[num_bc].bearer_level_qos){
          memset((void*)&eps_qos, 0, sizeof(eps_qos));
          rc = qos_params_to_eps_qos(bcs_tbu->bearer_contexts[num_bc].bearer_level_qos->qci,
              bcs_tbu->bearer_contexts[num_bc].bearer_level_qos->mbr.br_dl,
              bcs_tbu->bearer_contexts[num_bc].bearer_level_qos->mbr.br_ul,
              bcs_tbu->bearer_contexts[num_bc].bearer_level_qos->gbr.br_dl,
              bcs_tbu->bearer_contexts[num_bc].bearer_level_qos->gbr.br_ul,
              &eps_qos, false);
        }
        if (RETURNok == rc) {
          ambr_t * ambr = !num_bc ? &msg->apn_ambr: NULL; /**< Send the new received AMBR (if exists) with the first bearer update message. */
          rc = esm_send_modify_eps_bearer_context_request (
              pti, bcs_tbu->bearer_contexts[num_bc].eps_bearer_id,
              &esm_msg.activate_dedicated_eps_bearer_context_request, &eps_qos,
              &bcs_tbu->bearer_contexts[num_bc].tft,
              ambr, &bcs_tbu->bearer_contexts[num_bc].pco);

          esm_procedure = esm_proc_modify_eps_bearer_context_request; /**< Not the procedure. */
        }
        /** Exit the loop directly. */
        OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Successfully sent request to modify bearer for ebi %d for UE " MME_UE_S1AP_ID_FMT " . \n",
            ebi, esm_context->ue_id);
        break;
      }
    }
  }
  break;

  case DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST:
    /**
     * Create and encode the message,
     * set the procedure to set the pending status and to start the timers.
     * The EBI should already be set when the ESM message has been received.
     * This is always non local, since else we don't send message sent.
     */
    rc = esm_send_deactivate_eps_bearer_context_request (pti, ebi,
        &esm_msg.deactivate_eps_bearer_context_request, ESM_CAUSE_REGULAR_DEACTIVATION);
    if (rc != RETURNerror) {
      /** If no local pdn context deletion, directly continue with the NAS/S1AP message.
       * Setup the callback function used to send deactivate EPS
       * * * * bearer context request message onto the network
       */
      esm_procedure = esm_proc_eps_bearer_context_deactivate_request;
    }
    break;

  case PDN_DISCONNECT_REJECT:
    break;

  case BEARER_RESOURCE_ALLOCATION_REJECT:
    break;

  case BEARER_RESOURCE_MODIFICATION_REJECT:
    break;

  default:
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Send unexpected ESM message 0x%x\n", msg_type);
    break;
  }

  if (rc != RETURNerror) {
    #define ESM_SAP_BUFFER_SIZE 4096
    uint8_t           esm_sap_buffer[ESM_SAP_BUFFER_SIZE];
    /*
     * Encode the returned ESM response message
     */
    int size = esm_msg_encode (&esm_msg, esm_sap_buffer, ESM_SAP_BUFFER_SIZE);

    if (size > 0) {
      *rsp = blk2bstr(esm_sap_buffer, size);
    } else{
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Error encoding ESM message 0x%x\n", msg_type);
      OAILOG_FUNC_RETURN(LOG_NAS_ESM, RETURNerror);
    }

    /*
     * Execute the relevant ESM procedure
     */
    if (esm_procedure) {
      rc = (*esm_procedure) (true, esm_context, ebi, rsp, sent_by_ue);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

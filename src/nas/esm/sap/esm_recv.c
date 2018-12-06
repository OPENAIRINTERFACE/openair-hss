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
  Source      esm_recv.c

  Version     0.1

  Date        2013/02/06

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Frederic Maurel

  Description Defines functions executed at the ESM Service Access
        Point upon receiving EPS Session Management messages
        from the EPS Mobility Management sublayer.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "bstrlib.h"
#include "log.h"
#include "dynamic_memory_check.h"
#include "common_types.h"
#include "assertions.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "3gpp_requirements_24.301.h"
#include "common_defs.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "nas_itti_messaging.h"
#include "mme_config.h"
#include "mme_app_apn_selection.h"
#include "esm_ebr.h"
#include "esm_proc.h"
#include "esm_pt.h"
#include "esm_cause.h"
#include "esm_sap.h"
#include "esm_recv.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
   Functions executed by both the UE and the MME upon receiving ESM messages
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_status()                                         **
 **                                                                        **
 ** Description: Processes ESM status message                              **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **      pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/

esm_cause_t
esm_recv_status (
  const mme_ue_s1ap_id_t mme_ue_s1ap_id,
  proc_tid_t pti,
  ebi_t ebi,
  const pdn_connectivity_request_msg * msg,
  ESM_msg **esm_response)
{
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_ESM);
  OAILOG_INFO(LOG_NAS_ESM,  "ESM-SAP   - Received ESM status message (pti=%d, ebi=%d) for UE " MME_UE_S1AP_ID_FMT ". \n", pti, ebi, mme_ue_s1ap_id);
//  /*
//   * Message processing
//   */
//  /*
//   * Get the ESM cause
//   */
//  esm_cause = msg->esmcause;
//  /*
//   * Execute the ESM status procedure
//   */
//  rc = esm_proc_status_ind (esm_context, pti, ebi, &esm_cause);
//
//  if (rc != RETURNerror) {
//    esm_cause = ESM_CAUSE_SUCCESS;
//  }

  /*
   * Return the ESM cause value
   */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
}


/*
   --------------------------------------------------------------------------
   Functions executed by the MME upon receiving ESM message from the UE
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_pdn_connectivity_request()                       **
 **                                                                        **
 ** Description: Processes PDN connectivity request message                **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **      imsi:      IMSI                                       **
 **      pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     new_ebi:   New assigned EPS bearer identity           **
 **      esm_response:      PDN connection and EPS bearer context data **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_recv_pdn_connectivity_request (
  const mme_ue_s1ap_id_t mme_ue_s1ap_id,
  const imsi_t *imsi,
  proc_tid_t pti,
  ebi_t ebi,
  tai_t *visited_tai,
  const pdn_connectivity_request_msg * msg,
  ESM_msg **esm_response)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  pdn_context_t                          *new_pdn_context  = NULL;
  int                                     rc               = RETURNerror;
  esm_proc_pdn_request_t                  pdn_request_type = 0;
  esm_proc_pdn_type_t                     pdn_type         = 0;

  OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Received PDN Connectivity Request message " "(ue_id=%u, pti=%d, ebi=%d)\n", esm_context->ue_id, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if ((pti == ESM_PT_UNASSIGNED) || esm_pt_is_reserved (pti)) {
    /*
     * 3GPP TS 24.301, section 7.3.1, case a
     * * * * Reserved or unassigned PTI value
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid PTI value (pti=%d)\n", pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * EPS bearer identity checking
   */
  else if (ebi != ESM_EBI_UNASSIGNED) {
    /*
     * 3GPP TS 24.301, section 7.3.2, case a
     * * * * Reserved or assigned EPS bearer identity value
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)\n", ebi);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }
  /** Check if a procedure exists. */
  // todo: duplicate..
  nas_esm_proc_t * esm_proc = esm_data_get_procedure(mme_ue_s1ap_id);
  if(esm_proc){
    OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Found a procedure (pti=%d, type=%d) for UE " MME_UE_S1AP_ID_FMT ".\n", esm_proc->base_proc.pti, esm_proc->type, mme_ue_s1ap_id);
    /** If it is a transactional procedure, check if the PTI is the same. */
    if(esm_proc->type == ESM_PROC_TRANSACTION){
      /** We have another transactional procedure. If the PTIs don't match, reject the message. */
      if(esm_proc->base_proc.pti != pti){
        OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - Already an existing UE triggered ESM procedure (pti=%d, type=%d) for UE " MME_UE_S1AP_ID_FMT "."
            " Rejecting request for new procedure (pti=%d).\n", esm_proc->base_proc.pti, esm_proc->type, mme_ue_s1ap_id, pti);
        OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
      }
//      if(esm_proc->esm_procedure_type != ESM_PROC_TRANSACTION_PDN_CONNECTIVITY){ /**< We cannot receive another PDN-Connectivity while ESM is running (no-attach). No duplicates expected here. */
//        OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - ESM procedure for UE " MME_UE_S1AP_ID_FMT " (pti=%d) has invalid procedure type %d for ESM-Information response handling.\n",
//            ue_id, pti, esm_proc->esm_procedure_type);
//        OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
//      }
      /** Procedure already exists. Check if a response message is appended to the procedure.. Return the procedure. */
      *esm_response = &((nas_esm_transaction_proc_t*)esm_proc)->esm_result;
      OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Returning Already an existing procedure (pti=%d, type=%d) for UE " MME_UE_S1AP_ID_FMT "."
          " Rejecting request for new procedure (pti=%d).\n", esm_proc->base_proc.pti, esm_proc->type, mme_ue_s1ap_id, pti);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
    } else {
      /** A network initiated procedure exists. Not allowed. */
      OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - UE initiated ESM procedure (pti=%d, type=%d) for UE " MME_UE_S1AP_ID_FMT " collided with network initiated bearer procedure (pti=0).\n",
          esm_proc->base_proc.pti, esm_proc->type, mme_ue_s1ap_id);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_COLLISION_WITH_NETWORK_INITIATED_REQUEST);
    }
  }
  OAILOG_DEBUG(LOG_NAS_ESM, "ESM-SAP   - No ESM procedure for UE " MME_UE_S1AP_ID_FMT " exists. Proceeding with handling the new ESM request (pti=%d) for PDN connectivity.\n", pti, mme_ue_s1ap_id);

  /** Create a new procedure and fill it. */
  /*
   * Get the PDN connectivity request type.
   */
  if (msg->requesttype == REQUEST_TYPE_INITIAL_REQUEST) {
    pdn_request_type = ESM_PDN_REQUEST_INITIAL;
  } else if (msg->requesttype == REQUEST_TYPE_HANDOVER) {
    pdn_request_type = ESM_PDN_REQUEST_HANDOVER;
  } else if (msg->requesttype == REQUEST_TYPE_EMERGENCY) {
    pdn_request_type = ESM_PDN_REQUEST_EMERGENCY;
  } else {
    /*
     * Unknown PDN request type
     */
    pdn_request_type = -1;
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Invalid PDN request type (INITIAL/HANDOVER/EMERGENCY) (%d) for UE " MME_UE_S1AP_ID_FMT".\n", mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_MANDATORY_INFO);
  }

  /*
   * Get the value of the PDN type indicator
   */
  if (msg->pdntype == PDN_TYPE_IPV4) {
    pdn_type = ESM_PDN_TYPE_IPV4;
  } else if (msg->pdntype == PDN_TYPE_IPV6) {
    pdn_type = ESM_PDN_TYPE_IPV6;
  } else if (msg->pdntype == PDN_TYPE_IPV4V6) {
    pdn_type = ESM_PDN_TYPE_IPV4V6;
  } else {
    /*
     * Unknown PDN type
     */
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Invalid PDN type %d.\n", msg->pdntype);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_UNKNOWN_PDN_TYPE);
  }

  /**
   * For the case of PDN Connectivity via ESM, we will always trigger an S11 Create Session Request.
   * We won't enter this case in case of a Tracking Area Update procedure.
   */
  pdn_context_t *pdn_context_duplicate = NULL;
  mme_app_get_pdn_context(ue_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, ESM_EBI_UNASSIGNED, msg->accesspointname, &pdn_context_duplicate);
  if(pdn_context_duplicate){
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - Found an established PDN context for the APN \"%s\". Rejecting the redundant PDN connectivity procedure." "(ue_id=%d, pti=%d)\n",
          bdata(msg->accesspointname), mme_ue_s1ap_id, pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_MULTIPLE_PDN_CONNECTIONS_NOT_ALLOWED);
  }

  /*
   * Establish PDN connectivity.
   * Currently only checking that request type is INITIAL_REQUEST.
   * todo: perform other validations than request type.
   */
  if(pdn_request_type!= ESM_PDN_REQUEST_INITIAL){
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - No other request type than INITIAL_REQUEST is supported for PDN connectivity procedure. "
        "Rejecting the PDN connectivity procedure  (req_type=%d)." "(ue_id=%d, pti=%d)\n", pdn_request_type, mme_ue_s1ap_id, pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SERVICE_OPTION_NOT_SUPPORTED);
  }

  /**
   * Create a new ESM procedure for PDN Connectivity. Not important if we continue with ESM information or PDN connectivity, since this function will handle
   * the received messages (no specific callback needed).
   */
  nas_esm_pdn_connectivity_proc_t * esm_pdn_connectivity_proc = _esm_proc_create_pdn_connectivity_procedure(mme_ue_s1ap_id, imsi, pti);
  esm_pdn_connectivity_proc->request_type = pdn_request_type;
  esm_pdn_connectivity_proc->pdn_type = pdn_type;
  memcpy(&esm_pdn_connectivity_proc->visited_tai, visited_tai, sizeof(tai_t));
  // todo: LOCK_ESM_TRANSACTION!

  /*
   * Get the ESM information transfer flag
   */
  if (msg->presencemask & PDN_CONNECTIVITY_REQUEST_ESM_INFORMATION_TRANSFER_FLAG_PRESENT) {
    /*
     * 3GPP TS 24.301, sections 6.5.1.2, 6.5.1.3
     * * * * ESM information, i.e. protocol configuration options, APN, or both,
     * * * * has to be sent after the NAS signaling security has been activated
     * * * * between the UE and the MME.
     * * * *
     * * * * The MME then at a later stage in the PDN connectivity procedure
     * * * * initiates the ESM information request procedure in which the UE
     * * * * can provide the MME with protocol configuration options or APN
     * * * * or both.
     * * * * The MME waits for completion of the ESM information request
     * * * * procedure before proceeding with the PDN connectivity procedure.
     */
    if (!mme_config.nas_config.disable_esm_information && msg->esminformationtransferflag) {
      /**
       * Create an ESM message for ESM information request and configures the timer/timeout.
       */
      esm_proc_esm_information_request(mme_ue_s1ap_id, esm_pdn_connectivity_proc, esm_response);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
    }
  }

  /**
   * Continuing with the PDN Configuration.
   * Get the Access Point Name, if provided
   */
  if (msg->presencemask & PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT) {
    esm_pdn_connectivity_proc->apn_subscribed = msg->accesspointname;
  } else{
    /** No APN Name received from UE. We will used the default one from the subscription data. */
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - No APN name received from UE to establish PDN connection and subscription exists. "
        "Will attach to the default APN in the subscription information. " "(ue_id=%d, pti=%d)\n", mme_ue_s1ap_id, pti);
    esm_pdn_connectivity_proc->apn_subscribed = NULL;
  }

  if (msg->presencemask & PDN_CONNECTIVITY_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT) {
    // todo: implement this after checking for fixes..
    //    copy_protocol_configuration_options(&esm_data->pco, &msg->protocolconfigurationoptions);
  }
  imsi64_t imsi64 = imsi_to_imsi64(imsi);
  /** Checking if APN configuration profile for the desired APN profile exists. */
  struct apn_configuration_s* apn_config = mme_app_select_apn(&esm_pdn_connectivity_proc->imsi, esm_pdn_connectivity_proc->apn_subscribed);
  if(!apn_config){
    if(mme_app_select_subscription_data(&esm_pdn_connectivity_proc->imsi)){
      OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Subscription Data exists for IMSI " IMSI_64_FMT" but no APN configuration for the APN \"%s\". " " (ue_id=%d, pti=%d)\n", imsi64, bdata(msg->accesspointname), ue_id, pti);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_APN_RESTRICTION_VALUE_NOT_COMPATIBLE);
    }
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - No APN configuration for the APN \"%s\" has been downloaded from the HSS yet.. "
        "Getting subscription profile from the HSS after receiving ESM information request. " "(ue_id=%d, pti=%d)\n", bdata(msg->accesspointname), ue_id, pti);
    /** Trigger a itti message to download subscription profile. */
    nas_itti_pdn_config_req(mme_ue_s1ap_id, TASK_NAS_ESM, &esm_pdn_connectivity_proc->imsi, esm_pdn_connectivity_proc->request_type, &esm_pdn_connectivity_proc->visited_plmn);
    /** No ESM response message. Downloading subscription data via ULR. */
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  }
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Found a valid APN configuration for the APN \"%s\" from HSS. Continuing with the PDN Connectivity procedure." "(ue_id=%d, pti=%d)\n",
      bdata(msg->accesspointname), ue_id, pti);

  /**
   * Establish the PDN Connectivity, using the default APN-QoS values received in the subscription information, which will be processed in the MME_APP layer
   * inside the UE_CONTEXT. Just trigger PDN connectivity here.
   */
  // todo: check the PCOs and establish IPv6
  /*
   * Set the ESM Proc Data values.
   * Update the UE context and PDN context information with it.
   * todo: how to check that this is still our last ESM proc data?
   * Execute the PDN connectivity procedure requested by the UE
   */
  rc = esm_proc_pdn_connectivity_request (mme_ue_s1ap_id, imsi, pti, apn_config->context_identifier, esm_pdn_connectivity_proc->request_type, esm_pdn_connectivity_proc->apn_subscribed, esm_pdn_connectivity_proc->pdn_type);
//      (esm_context->esm_proc_data->pco.num_protocol_or_container_id ) ? &esm_context->esm_proc_data->pco:NULL);
  if(rc == RETURNerror){
    /** Remove the procedure and send a pdn rejection back, which will trigger the attach reject.
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_information_response()                        **
 **                                                                        **
 ** Description: Processes ESM information response                 **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **      pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_response:      PDN connection and EPS bearer context data **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
//------------------------------------------------------------------------------
esm_cause_t esm_recv_information_response (
  mme_ue_s1ap_id_t ue_id,
  proc_tid_t pti,
  ebi_t ebi,
  const esm_information_response_msg * msg,
  ESM_msg *esm_response)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                               esm_cause = ESM_CAUSE_SUCCESS;
  int                                       rc = RETURNok;

  OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Received ESM Information response message " "(ue_id=%d, pti=%d, ebi=%d)\n", ue_id, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if ((pti == ESM_PT_UNASSIGNED) || esm_pt_is_reserved (pti)) {
    /*
     * 3GPP TS 24.301, section 7.3.1, case b
     * * * * Reserved or unassigned PTI value
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid PTI value (pti=%d)\n", pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * EPS bearer identity checking
   */
  else if (ebi != ESM_EBI_UNASSIGNED) {
    /*
     * 3GPP TS 24.301, section 7.3.2, case b
     * * * * Reserved or assigned EPS bearer identity value
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)\n", ebi);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  nas_esm_pdn_connectivity_proc_t * esm_pdn_connectivity_proc = esm_data_get_procedure(ue_id);
  if(!esm_pdn_connectivity_proc){
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - No procedure for UE " MME_UE_S1AP_ID_FMT " found.\n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }
  if(esm_pdn_connectivity_proc->trx_base_proc.esm_proc.type == ESM_PROC_TRANSACTION){
    /** We have another transactional procedure. If the PTIs don't match, reject the message. */
    if(esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.pti != pti){
      OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - Already an existing UE triggered ESM procedure (pti=%d, type=%d) for UE " MME_UE_S1AP_ID_FMT "."
          " Rejecting request for new procedure (pti=%d).\n", esm_pdn_connectivity_proc->base_proc.pti, esm_pdn_connectivity_proc->type, ue_id, pti);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
    }
    /** Procedure already exists. Check if a response message is appended to the procedure.. Return the procedure. */
//    if(esm_pdn_connectivity_proc->trx_base_proc.esm_proc.esm_procedure_type != ESM_PROC_INFORMATION_REQUEST_PROCEDURE){
//      OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - ESM procedure for UE " MME_UE_S1AP_ID_FMT " (pti=%d) has invalid procedure type %d for ESM-Information response handling.\n",
//          ue_id, pti, esm_pdn_connectivity_proc->trx_base_proc.esm_proc.esm_procedure_type);
//      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
//    }
    OAILOG_DEBUG(LOG_NAS_ESM, "ESM-SAP   - Found a valid ESM procedure for UE " MME_UE_S1AP_ID_FMT ". Proceeding with handling the new ESM request (pti=%d) for PDN connectivity.\n", pti, ue_id);
  } else {
    /** A network initiated procedure exists. Not allowed. */
    OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - UE initiated ESM information procedure (pti=%d, type=%d) for UE " MME_UE_S1AP_ID_FMT " collided with network initiated bearer procedure (pti=0).\n",
        esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.pti, esm_pdn_connectivity_proc->trx_base_proc.esm_proc.type, ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_COLLISION_WITH_NETWORK_INITIATED_REQUEST);
  }

  imsi64_t imsi64 = imsi_to_imsi64(&esm_pdn_connectivity_proc->imsi);
  /** Checking if APN configuration profile for the desired APN profile exists. */
  struct apn_configuration_s* apn_config = mme_app_select_apn(&esm_pdn_connectivity_proc->imsi, esm_pdn_connectivity_proc->apn_subscribed);
  if(!apn_config){
    if(mme_app_select_subscription_data(&esm_pdn_connectivity_proc->imsi)){
      OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Subscription Data exists for IMSI " IMSI_64_FMT" but no APN configuration for the APN \"%s\". " " (ue_id=%d, pti=%d)\n", imsi64, bdata(msg->accesspointname), ue_id, pti);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_APN_RESTRICTION_VALUE_NOT_COMPATIBLE);
    }
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - No APN configuration for the APN \"%s\" has been downloaded from the HSS yet.. "
        "Getting subscription profile from the HSS after receiving ESM information request. " "(ue_id=%d, pti=%d)\n", bdata(msg->accesspointname), ue_id, pti);
    /** Trigger a itti message to download subscription profile. */
    nas_itti_pdn_config_req(ue_id, TASK_NAS_ESM, &esm_pdn_connectivity_proc->imsi, &esm_pdn_connectivity_proc->visited_plmn, esm_pdn_connectivity_proc->request_type);
    /** No ESM response message. Downloading subscription data via ULR. */
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  }
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Found a valid APN configuration for the APN \"%s\" from HSS. Continuing with the PDN Connectivity procedure." "(ue_id=%d, pti=%d)\n",
      bdata(msg->accesspointname), ue_id, pti);

  /**
   * For the case of PDN Connectivity via ESM, we will always trigger an S11 Create Session Request.
   * We won't enter this case in case of a Tracking Area Update procedure.
   */
  pdn_context_t *pdn_context_duplicate = NULL;
  mme_app_get_pdn_context(ue_context, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, ESM_EBI_UNASSIGNED, msg->accesspointname, &pdn_context_duplicate);
  if(pdn_context_duplicate){
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - Found an established PDN context for the APN \"%s\". Rejecting the redundant PDN connectivity procedure." "(ue_id=%d, pti=%d)\n",
        bdata(msg->accesspointname), ue_id, pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_MULTIPLE_PDN_CONNECTIONS_NOT_ALLOWED);
  }

  /**
   * Establish the PDN Connectivity, using the default APN-QoS values received in the subscription information, which will be processed in the MME_APP layer
   * inside the UE_CONTEXT. Just trigger PDN connectivity here.
   */
  // todo: check the PCOs and establish IPv6
  /*
   * Set the ESM Proc Data values.
   * Update the UE context and PDN context information with it.
   * todo: how to check that this is still our last ESM proc data?
   * Execute the PDN connectivity procedure requested by the UE
   */
  rc = esm_proc_pdn_connectivity_request (ue_id, &esm_pdn_connectivity_proc->imsi, pti, apn_config->context_identifier, esm_pdn_connectivity_proc->request_type, esm_pdn_connectivity_proc->apn_subscribed, esm_pdn_connectivity_proc->pdn_type);
  //      (esm_context->esm_proc_data->pco.num_protocol_or_container_id ) ? &esm_context->esm_proc_data->pco:NULL);
  if(rc != RETURNerror)
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_pdn_disconnect_request()                         **
 **                                                                        **
 ** Description: Processes PDN disconnect request message                  **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **      pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     linked_ebi:    Linked EPS bearer identity of the default  **
 **             bearer associated with the PDN to discon-  **
 **             nect from                                  **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_recv_pdn_disconnect_request (
  esm_context_t * esm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const pdn_disconnect_request_msg * msg,
  ebi_t *linked_ebi)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                               esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                          ue_id = esm_context->ue_id;

  OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Received PDN Disconnect Request message " "(ue_id=%d, pti=%d, ebi=%d)\n", ue_id, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if ((pti == ESM_PT_UNASSIGNED) || esm_pt_is_reserved (pti)) {
    /*
     * 3GPP TS 24.301, section 7.3.1, case b
     * * * * Reserved or unassigned PTI value
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid PTI value (pti=%d)\n", pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * EPS bearer identity checking
   */
  else if (ebi != ESM_EBI_UNASSIGNED) {
    /*
     * 3GPP TS 24.301, section 7.3.2, case b
     * * * * Reserved or assigned EPS bearer identity value (Transaction Related messages).
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)\n", ebi);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */

  /** Get the linked EBI. */
  *linked_ebi = msg->linkedepsbeareridentity;

  /*
   * Execute the PDN disconnect procedure requested by the UE.
   * Validating the message in the context of the UE and finding the PDN context to remove (validating not last PDN).
   * Triggering a Delete Session Request for the PDN.
   *
   */
  int pid = esm_proc_pdn_disconnect_request (esm_context, pti, 0, *linked_ebi, &esm_cause);

  if (pid != RETURNerror) {
    /*
     * Get the identity of the default EPS bearer context assigned to
     * * * * the PDN connection to disconnect from
     */
    /*
     * Check if it is a local release, if so directly release all bearers of the PDN connection and the PDN connection itself.
     * If not, do a validation, only.
     * Default bearer request will be sent in the esm_sap layer after this.
     */
//    int rc = esm_proc_eps_bearer_context_deactivate (esm_context, false, ESM_SAP_ALL_EBI, pid, &esm_cause);

//    if (rc != RETURNerror) {
      esm_cause = ESM_CAUSE_SUCCESS;
//    }
  }
  /*
   * Return the ESM cause value.
   * We sent the S11 Delete Session Request message and request default bearer removal from the UE.
   */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_activate_default_eps_bearer_context_accept()     **
 **                                                                        **
 ** Description: Processes Activate Default EPS Bearer Context Accept      **
 **      message                                                   **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **          pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_recv_activate_default_eps_bearer_context_accept (
  mme_ue_s1ap_id_t ue_id,
  proc_tid_t pti,
  ebi_t ebi,
  const activate_default_eps_bearer_context_accept_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                              esm_cause = ESM_CAUSE_SUCCESS;

  OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Received Activate Default EPS Bearer Context " "Accept message (ue_id=%d, pti=%d, ebi=%d)\n", ue_id, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if (esm_pt_is_reserved (pti)) {
    /*
     * 3GPP TS 24.301, section 7.3.1, case f
     * * * * Reserved PTI value
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid PTI value (pti=%d)\n", pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * EPS bearer identity checking
   */
  else if (esm_ebr_is_reserved (ebi)){ // || esm_ebr_is_not_in_use (esm_context, ebi)) {
    /*
     * 3GPP TS 24.301, section 7.3.2, case f
     * * * * Reserved or assigned value that does not match an existing EPS
     * * * * bearer context
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)\n", ebi);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }
  // todo: LOCK_ESM_TRX --> SHOULD BE REMOVED VIA TIMER?!?
  /*
   * Execute the default EPS bearer context activation procedure accepted
   * * * * by the UE
   */
  esm_proc_default_eps_bearer_context_accept (ue_id, esm_pdn_connectivity_proc);
  /*
   * Remove the PDN Connectivity procedure.
   * It stops T3485 timer if running
   */
  _esm_proc_free_pdn_connectivity_procedure(&esm_pdn_connectivity_proc);

  // todo: LOCK_ESM_TRX --> SHOULD BE REMOVED VIA TIMER?!?

  /*
   * Return the ESM cause value
   */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_activate_default_eps_bearer_context_reject()     **
 **                                                                        **
 ** Description: Processes Activate Default EPS Bearer Context Reject      **
 **      message                                                   **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **          pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fail                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_recv_activate_default_eps_bearer_context_reject (
  mme_ue_s1ap_id_t ue_id,
  proc_tid_t pti,
  ebi_t ebi,
  const activate_default_eps_bearer_context_accept_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;

  OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Received Activate Default EPS Bearer Context " "Reject message (ue_id=%d, pti=%d, ebi=%d)\n",
          ue_id, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if (esm_pt_is_reserved (pti)) {
    /*
     * 3GPP TS 24.301, section 7.3.1, case f
     * * * * Reserved PTI value
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid PTI value (pti=%d)\n", pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * Remove the PDN Context for the given APN.
   * No EMM message will be triggered, just a local remove.
   * Inform the SAE-GW
   */
  int rc = esm_proc_default_eps_bearer_context_reject (esm_context, ebi, &esm_cause);

  /*
   * Remove the PDN Connectivity procedure.
   * It stops T3485 timer if running
   */
  _esm_proc_free_pdn_connectivity_procedure(&esm_pdn_connectivity_proc);



  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /*
   * Return the ESM cause value
   */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_activate_dedicated_eps_bearer_context_accept()   **
 **                                                                        **
 ** Description: Processes Activate Dedicated EPS Bearer Context Accept    **
 **      message                                                   **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **          pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_recv_activate_dedicated_eps_bearer_context_accept (
  esm_context_t * esm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const activate_dedicated_eps_bearer_context_accept_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                        ue_id = esm_context->ue_id;

  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, esm_context->ue_id);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Received Activate Dedicated EPS Bearer " "Context Accept message (ue_id=%d, pti=%d, ebi=%d)\n",
          ue_id, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if (esm_pt_is_reserved (pti)) {
    /*
     * 3GPP TS 24.301, section 7.3.1, case f
     * * * * Reserved PTI value
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid PTI value (pti=%d)\n", pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
  }
//  /*
//   * EPS bearer identity checking
//   * todo: check with original functions (no ebi allocated)
//   */
//  else if (esm_ebr_is_reserved (ebi) || !mme_app_get_session_bearer_context_from_all(ue_context, ebi)) {    // todo: check old function esm_ebr_is_not_in_use (esm_context, ebi
//    /*
//     * 3GPP TS 24.301, section 7.3.2, case f
//     * * * * Reserved or assigned value that does not match an existing EPS
//     * * * * bearer context
//     */
//    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)\n", ebi);
//    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
//  }

  /*
   * Message processing
   */
  /*
   * Execute the dedicated EPS bearer context activation procedure accepted
   * * * * by the UE
   */
  int rc = esm_proc_dedicated_eps_bearer_context_accept (esm_context, ebi, &esm_cause);

  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /*
   * Return the ESM cause value
   */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_activate_dedicated_eps_bearer_context_reject()   **
 **                                                                        **
 ** Description: Processes Activate Dedicated EPS Bearer Context Reject    **
 **      message                                                   **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **          pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fail                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_recv_activate_dedicated_eps_bearer_context_reject (
  esm_context_t * esm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const activate_dedicated_eps_bearer_context_reject_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                        ue_id = esm_context->ue_id;

  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Received Activate Dedicated EPS Bearer " "Context Reject message (ue_id=%d, pti=%d, ebi=%d)\n",
          ue_id, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if (esm_pt_is_reserved (pti)) {
    /*
     * 3GPP TS 24.301, section 7.3.1, case f
     * * * * Reserved PTI value
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid PTI value (pti=%d)\n", pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * EPS bearer identity checking
   */
  else if (esm_ebr_is_reserved (ebi)){ // || esm_ebr_is_not_in_use (esm_context, ebi)) {
    /*
     * 3GPP TS 24.301, section 7.3.2, case f
     * * * * Reserved or assigned value that does not match an existing EPS
     * * * * bearer context
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)\n", ebi);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  /*
   * Execute the dedicated EPS bearer context activation procedure not
   * * * *  accepted by the UE
   */
  int rc = esm_proc_dedicated_eps_bearer_context_reject (esm_context, ebi, &esm_cause);

  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /*
   * Return the ESM cause value
   */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_modify_eps_bearer_context_accept()   **
 **                                                                        **
 ** Description: Processes Modidfy EPS Bearer Context Accept    **
 **      message                                                   **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **          pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_recv_modify_eps_bearer_context_accept (
  esm_context_t * esm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const modify_eps_bearer_context_accept_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                        ue_id = esm_context->ue_id;

  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, esm_context->ue_id);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Received Modify EPS Bearer " "Context Accept message (ue_id=%d, pti=%d, ebi=%d)\n",
          ue_id, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if (esm_pt_is_reserved (pti)) {
    /*
     * 3GPP TS 24.301, section 7.3.1, case f
     * * * * Reserved PTI value
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid PTI value (pti=%d)\n", pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
  }

  /*
   * Execute the dedicated EPS bearer context activation procedure accepted
   * * * * by the UE
   */
  int rc = esm_proc_modify_eps_bearer_context_accept(esm_context, ebi, &esm_cause);

  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /*
   * Return the ESM cause value
   */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_modify_eps_bearer_context_reject()   **
 **                                                                        **
 ** Description: Processes Modify EPS Bearer Context Reject    **
 **      message                                                   **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **          pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fail                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_recv_modify_eps_bearer_context_reject (
  esm_context_t * esm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const modify_eps_bearer_context_reject_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                        ue_id = esm_context->ue_id;

  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Received Modify EPS Bearer " "Context Reject message (ue_id=%d, pti=%d, ebi=%d)\n",
          ue_id, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if (esm_pt_is_reserved (pti)) {
    /*
     * 3GPP TS 24.301, section 7.3.1, case f
     * * * * Reserved PTI value
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid PTI value (pti=%d)\n", pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * EPS bearer identity checking
   */
  else if (esm_ebr_is_reserved (ebi)){ // || esm_ebr_is_not_in_use (esm_context, ebi)) {
    /*
     * 3GPP TS 24.301, section 7.3.2, case f
     * * * * Reserved or assigned value that does not match an existing EPS
     * * * * bearer context
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)\n", ebi);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  /*
   * Execute the dedicated EPS bearer context activation procedure not
   * * * *  accepted by the UE
   */
  int rc = esm_proc_modify_eps_bearer_context_reject(esm_context, ebi, &esm_cause, true);

  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /*
   * Return the ESM cause value
   */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_deactivate_eps_bearer_context_accept()           **
 **                                                                        **
 ** Description: Processes Deactivate EPS Bearer Context Accept message    **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **          pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_recv_deactivate_eps_bearer_context_accept (
  esm_context_t * esm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const deactivate_eps_bearer_context_accept_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                        ue_id = esm_context->ue_id;
  ue_context_t                           *ue_context  = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, esm_context->ue_id);
  pdn_context_t                          *pdn_context = NULL;

  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Received Deactivate EPS Bearer Context " "Accept message (ue_id=%d, pti=%d, ebi=%d)\n",
          ue_id, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if (esm_pt_is_reserved (pti)) {
    /*
     * 3GPP TS 24.301, section 7.3.1, case f
     * * * * Reserved PTI value
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid PTI value (pti=%d)\n", pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * EPS bearer identity checking
   */
  else if (esm_ebr_is_reserved (ebi)){ // || esm_ebr_is_not_in_use (esm_context, ebi)) {
    /*
     * 3GPP TS 24.301, section 7.3.2, case f
     * * * * Reserved or assigned value that does not match an existing EPS
     * * * * bearer context
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)\n", ebi);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  bearer_context_t * bc_p = NULL;
  mme_app_get_session_bearer_context_from_all(ue_context, ebi, &bc_p);
  if(!bc_p){
    OAILOG_ERROR(LOG_NAS_ESM , "ESM-PROC  - Could not find PDN context from ebi %d. \n", ebi);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
  }

  ebi_t linked_ebi = bc_p->linked_ebi;

  /*
   * Execute the default EPS bearer context activation procedure accepted
   * * * * by the UE
   */
  int pid = esm_proc_eps_bearer_context_deactivate_accept (esm_context, ebi, &esm_cause);
  if (pid != RETURNerror) {
    /*
     * Check if it was the default ebi. If so, release the pdn context.
     * If not, respond with a delete bearer response back. Keep the UE context and PDN context as valid.
     */
    mme_app_get_pdn_context(ue_context, pid, linked_ebi, NULL, &pdn_context);
    if(!pdn_context){
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - No PDN context could be found. (pid=%d)\n", pid);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
    }
    int rc = RETURNerror;
    if(pdn_context->default_ebi == ebi){
      // todo: check that only 1 deactivate message for the default bearer exists
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - We released  the default EBI. Deregistering the PDN context. (ebi=%d,pid=%d)\n", ebi,pid);
      rc = esm_proc_pdn_disconnect_accept (esm_context, pid, ebi, &esm_cause); /**< Delete Session Request is already sent at the beginning. We don't care for the response. */
    }else{
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - We released  the dedicated EBI. Responding with delete bearer response back. (ebi=%d,pid=%d)\n", ebi,pid);
      /** Respond per bearer. */
      nas_itti_dedicated_eps_bearer_deactivation_complete(esm_context->ue_id, ebi);
      /** Successfully informed the MME_APP layer about the bearer deactivation. We are complete. */
    }
    if (rc != RETURNerror) {
      esm_cause = ESM_CAUSE_SUCCESS;
    }
  }

  /*
   * Return the ESM cause value
   */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

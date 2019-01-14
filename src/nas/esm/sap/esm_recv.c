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
#include "conversions.h"
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
  const mme_ue_s1ap_id_t ue_id,
  proc_tid_t pti,
  ebi_t ebi,
  const esm_status_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;

  OAILOG_INFO(LOG_NAS_ESM,  "ESM-SAP   - Received ESM status message (pti=%d, ebi=%d) for UE " MME_UE_S1AP_ID_FMT ". \n",
      pti, ebi, ue_id);
  /*
   * Message processing
   */
  /*
   * Get the ESM cause
   */
  esm_cause = msg->esmcause;
  /*
   * Execute the ESM status procedure
   */
  esm_cause = esm_proc_status_ind (ue_id, pti, ebi, &esm_cause);

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
  bool *is_attach,
  const mme_ue_s1ap_id_t ue_id,
  const imsi_t *imsi,
  proc_tid_t pti,
  ebi_t ebi,
  tai_t *visited_tai,
  const pdn_connectivity_request_msg * msg,
  const ESM_msg * esm_rsp_msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_proc_pdn_request_t                  pdn_request_type = 0;
  esm_proc_pdn_type_t                     pdn_type         = 0;

  OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Received PDN Connectivity Request message " "(ue_id=%u, pti=%d, ebi=%d)\n", ue_id, pti, ebi);

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
  /*
   * Check if a procedure exists.
   * No procedure should exist at all (also no network triggered procedure without PTI.
   * Not checking any bearer context procedure at this step.
   */
  nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity = _esm_proc_get_pdn_connectivity_procedure(ue_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED);
  if(esm_proc_pdn_connectivity){
    OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Found a PDN connectivity procedure (pti=%d) for UE " MME_UE_S1AP_ID_FMT ".\n",
        esm_proc_pdn_connectivity->esm_base_proc.pti, ue_id);
    /** We have another transactional procedure. If the PTIs don't match, reject the message. */
    if(esm_proc_pdn_connectivity->esm_base_proc.pti == pti){
      /* We have a duplicate PDN connectivity request. */
      DevAssert(!*is_attach);
      OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - A PDN connectivity request (pti=%d) for UE " MME_UE_S1AP_ID_FMT " is already received. \n"
          " Continuing to check the new procedure (pti=%d).\n", esm_proc_pdn_connectivity->esm_base_proc.pti, ue_id, pti);
      esm_cause_t esm_cause = esm_proc_pdn_connectivity_retx(ue_id, esm_proc_pdn_connectivity, esm_rsp_msg);
      if(esm_cause == ESM_CAUSE_SUCCESS && esm_rsp_msg->header.message_type){
        /** Restart the T3485 timer and resend the ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT message. */
        esm_proc_default_eps_bearer_context(ue_id, esm_proc_pdn_connectivity);
      }
      OAILOG_FUNC_RETURN(LOG_NAS_ESM, esm_cause);
    } else {
      OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - Already an existing UE triggered ESM PDN connectivity procedure (pti=%d) for UE " MME_UE_S1AP_ID_FMT " (no pdn context)."
          "Removing old procedure and rejecting request for new procedure (pti=%d).\n", esm_proc_pdn_connectivity->esm_base_proc.pti, ue_id, pti);
      /* todo: We don't check the PDN context of the old procedure yet. */
//      _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
      OAILOG_FUNC_RETURN(LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
    }
  }
  /* Check no bearer context procedure exists. Not handling them in parallel. */
  nas_esm_proc_bearer_context_t * esm_proc_bearer_context = _esm_proc_get_bearer_context_procedure(ue_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, ESM_EBI_UNASSIGNED);
  if(esm_proc_bearer_context){
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - An Bearer Context procedure is ongoing. Ignoring received PDN connectivity request for UE " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_COLLISION_WITH_NETWORK_INITIATED_REQUEST);
  }

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
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Invalid PDN request type (INITIAL/HANDOVER/EMERGENCY) (%d) for UE " MME_UE_S1AP_ID_FMT".\n", ue_id);
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

  /*
   * Establish PDN connectivity.
   * Currently only checking that request type is INITIAL_REQUEST.
   */
  if(pdn_request_type!= ESM_PDN_REQUEST_INITIAL){
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - No other request type than INITIAL_REQUEST is supported for PDN connectivity procedure. "
        "Rejecting the PDN connectivity procedure  (req_type=%d)." "(ue_id=%d, pti=%d)\n", pdn_request_type, ue_id, pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SERVICE_OPTION_NOT_SUPPORTED);
  }

  apn_configuration_t * apn_configuration = NULL;
  imsi64_t imsi64 = imsi_to_imsi64(imsi);

  if(mme_app_select_apn(imsi64, msg->accesspointname, &apn_configuration) == RETURNerror){
    DevAssert(msg->accesspointname);
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - No APN configuration could be found for APN \"%s\". "
        "Rejecting the PDN connectivity procedure." "(ue_id=%d, pti=%d)\n", bdata(msg->accesspointname), ue_id, pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_UNKNOWN_ACCESS_POINT_NAME);
  }

  ue_context_t        * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  DevAssert(ue_context);
  *is_attach = ue_context->mm_state == UE_UNREGISTERED;

  /** An APN Name must be present, if it is not attach. */
  if(!*is_attach && !(msg->presencemask & PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT)){
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - No APN name present for extra PDN connectivity request. Rejecting the PDN connectivity procedure." "(ue_id=%d, pti=%d)\n", ue_id, pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_UNKNOWN_ACCESS_POINT_NAME);
  }

  /**
   * Create a new ESM procedure for PDN Connectivity. Not important if we continue with ESM information or PDN connectivity, since this function will handle
   * the received messages (no specific callback needed).
   */
  DevAssert(!esm_proc_pdn_connectivity);
  OAILOG_DEBUG(LOG_NAS_ESM, "ESM-SAP   - No ESM procedure for UE " MME_UE_S1AP_ID_FMT " exists. Proceeding with handling the new ESM request (pti=%d) for PDN connectivity.\n", pti, ue_id);
  esm_proc_pdn_connectivity = _esm_proc_create_pdn_connectivity_procedure(ue_id, imsi, pti);
  esm_proc_pdn_connectivity->request_type = pdn_request_type;
  esm_proc_pdn_connectivity->pdn_type = pdn_type;
  esm_proc_pdn_connectivity->is_attach = *is_attach;
  memcpy(&esm_proc_pdn_connectivity->imsi, imsi, sizeof(imsi_t));
  memcpy(&esm_proc_pdn_connectivity->visited_tai, visited_tai, sizeof(tai_t));
  if(msg->presencemask & PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT)
    esm_proc_pdn_connectivity->subscribed_apn = bstrcpy(msg->accesspointname);

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
       * Create an ESM message for ESM information request and start T3489.
       */
      esm_proc_esm_information_request(esm_proc_pdn_connectivity, esm_rsp_msg);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
    } else {
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Ignoring received ESM Information Transfer flag. " "(ue_id=%d, pti=%d)\n", ue_id, pti);
      /** Will use default APN configuration. */
    }
  }

  /**
   * Continuing with the PDN Configuration.
   * Get the Access Point Name, if provided.
   */
  if (!esm_proc_pdn_connectivity->subscribed_apn) {
    /** No APN Name received from UE. We will used the default one from the subscription data. */
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - No APN name received from UE to establish PDN connection and subscription exists. "
        "Will attach to the default APN in the subscription information. " "(ue_id=%d, pti=%d)\n", ue_id, pti);
  }

  if (msg->presencemask & PDN_CONNECTIVITY_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT) {
    /** Copy the protocol configuration options. */
    clear_protocol_configuration_options(&esm_proc_pdn_connectivity->pco);
    copy_protocol_configuration_options(&esm_proc_pdn_connectivity->pco, &msg->protocolconfigurationoptions);
  }
  if(!apn_configuration) { /**< We should have an APN identifier by now. If no APN-Config is available, request subscription data. */
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Getting subscription profile for IMSI "IMSI_64_FMT ". " "(ue_id=%d, pti=%d)\n", imsi64, ue_id, pti);
    nas_itti_pdn_config_req(ue_id, TASK_NAS_ESM, imsi, esm_proc_pdn_connectivity->request_type, &esm_proc_pdn_connectivity->visited_tai.plmn);
    /*
     * No ESM response message. Downloading subscription data via ULR.
     * An ESM procedure is created but no timer is started.
     */
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  }

  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Found a valid (default) APN configuration (cid=%d). Continuing with the PDN Connectivity procedure." "(ue_id=%d, pti=%d)\n", apn_configuration->context_identifier, ue_id, pti);

  if(!esm_proc_pdn_connectivity->subscribed_apn){
    esm_proc_pdn_connectivity->subscribed_apn = blk2bstr(apn_configuration->service_selection, apn_configuration->service_selection_length);  /**< Set the APN-NI from the service selection. */
  }
  /* The APN-Configuration must be the correct one. */
  esm_proc_pdn_connectivity->pdn_cid = apn_configuration->context_identifier;

  /*
   * Establish the PDN Connectivity, using the default APN-QoS values received in the subscription information, which will be processed in the MME_APP layer
   * inside the UE_CONTEXT. Just trigger PDN connectivity here.
   *
   * Set the ESM Proc Data values.
   * Update the UE context and PDN context information with it.
   * todo: how to check that this is still our last ESM proc data?
   * Execute the PDN connectivity procedure requested by the UE
   */
  esm_cause_t esm_cause = esm_proc_pdn_connectivity_request (ue_id, imsi,
      visited_tai, esm_proc_pdn_connectivity,
      apn_configuration);
//      (pco.num_protocol_or_container_id ) ? &pco:NULL);
  if(esm_cause != ESM_CAUSE_SUCCESS){
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Error while trying to establish an APN session for APN \"%s\" (cid=%d). " "(ue_id=%d, pti=%d)\n",
        bdata(esm_proc_pdn_connectivity->subscribed_apn), esm_proc_pdn_connectivity->pdn_cid, ue_id, pti);
    /** Remove the procedure and send a pdn rejection back, which will trigger the attach reject. */
    _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
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
  bool *is_attach,
  mme_ue_s1ap_id_t ue_id,
  proc_tid_t pti,
  ebi_t ebi,
  const esm_information_response_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

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

  nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity = _esm_proc_get_pdn_connectivity_procedure(ue_id, pti);
  if(!esm_proc_pdn_connectivity){
    OAILOG_WARNING(LOG_NAS_ESM, "ESM-SAP   - No procedure for UE " MME_UE_S1AP_ID_FMT " found. Ignoring the received ESM information request.\n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  }

  /** We have another transactional procedure. If the PTIs don't match, reject the message and remove the procedure. */
  if(esm_proc_pdn_connectivity->esm_base_proc.pti != pti){
    _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
    /** Not removing the associated PDN context yet. */
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - Already an existing UE triggered ESM procedure (pti=%d, type=%d) for UE " MME_UE_S1AP_ID_FMT "."
        " Rejecting request for new procedure (pti=%d).\n", esm_proc_pdn_connectivity->esm_base_proc.pti, esm_proc_pdn_connectivity->esm_base_proc.type, ue_id, pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
  }

  nas_esm_proc_bearer_context_t * esm_proc_bearer_context = _esm_proc_get_bearer_context_procedure(ue_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, ESM_EBI_UNASSIGNED);
  if(esm_proc_bearer_context){
    /** A network initiated procedure exists. Not allowed (not removing the procedure). */
    OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - UE initiated ESM information procedure (pti=%d, type=%d) for UE " MME_UE_S1AP_ID_FMT " collided with network initiated bearer procedure (pti=%d).\n",
        esm_proc_pdn_connectivity->esm_base_proc.pti, ue_id, esm_proc_bearer_context->esm_base_proc.pti);
    /** Free the PDN connectivity procedure. */
    _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_COLLISION_WITH_NETWORK_INITIATED_REQUEST);
  }

  /** Procedure already exists. Check if a response message is appended to the procedure.. Return the procedure. */
  OAILOG_DEBUG(LOG_NAS_ESM, "ESM-SAP   - Found a valid ESM procedure for UE " MME_UE_S1AP_ID_FMT ". Proceeding with handling the new ESM request (pti=%d) for PDN connectivity.\n", ue_id, pti);
  *is_attach = esm_proc_pdn_connectivity->is_attach;

  esm_proc_esm_information_response(ue_id, pti, esm_proc_pdn_connectivity, msg);

  imsi64_t imsi64 = imsi_to_imsi64(&esm_proc_pdn_connectivity->imsi);
  /** Checking if APN configuration profile for the desired APN profile exists. */
  apn_configuration_t * apn_configuration = NULL;
  if(mme_app_select_apn(imsi64, esm_proc_pdn_connectivity->subscribed_apn, &apn_configuration) == RETURNerror){
    DevAssert(esm_proc_pdn_connectivity->subscribed_apn);
    _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - No APN configuration could be found for APN \"%s\". "
        "Rejecting the PDN connectivity procedure." "(ue_id=%d, pti=%d)\n", bdata(esm_proc_pdn_connectivity->subscribed_apn), ue_id, pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_UNKNOWN_ACCESS_POINT_NAME);
  }

  if(!apn_configuration) { /**< Will always be the default configuration, even if a name is given. */
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Getting subscription profile for IMSI "IMSI_64_FMT ". " "(ue_id=%d, pti=%d)\n", imsi64, ue_id, pti);
    nas_itti_pdn_config_req(ue_id, TASK_NAS_ESM, &esm_proc_pdn_connectivity->imsi, esm_proc_pdn_connectivity->request_type, &esm_proc_pdn_connectivity->visited_tai.plmn);
    /*
     * No ESM response message. Downloading subscription data via ULR.
     * An ESM procedure is created but no timer is started.
     */
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  }

  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Found a valid (default) APN configuration (cid=%d). Continuing with the PDN Connectivity procedure." "(ue_id=%d, pti=%d)\n",
      apn_configuration->context_identifier, ue_id, pti);

  if(!esm_proc_pdn_connectivity->subscribed_apn){
    esm_proc_pdn_connectivity->subscribed_apn = blk2bstr(apn_configuration->service_selection, apn_configuration->service_selection_length);  /**< Set the APN-NI from the service selection. */
  }
  esm_proc_pdn_connectivity->pdn_cid = apn_configuration->context_identifier;

  /*
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
  if(esm_proc_pdn_connectivity_request (ue_id, &esm_proc_pdn_connectivity->imsi,
      &esm_proc_pdn_connectivity->visited_tai, esm_proc_pdn_connectivity,
      apn_configuration) != ESM_CAUSE_SUCCESS){
    //      (esm_context->esm_proc_data->pco.num_protocol_or_container_id ) ? &esm_context->esm_proc_data->pco:NULL);
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Error while trying to establish an APN session for APN \"%s\" (cid=%d). " "(ue_id=%d, pti=%d)\n",
        bdata(esm_proc_pdn_connectivity->subscribed_apn), esm_proc_pdn_connectivity->pdn_cid, ue_id, pti);
    /** Remove the procedure and send a pdn rejection back, which will trigger the attach reject. */
    _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
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
  mme_ue_s1ap_id_t ue_id,
  proc_tid_t pti,
  ebi_t ebi,
  const pdn_disconnect_request_msg * msg,
  ESM_msg * const esm_rsp_msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

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
   * Check that no transaction for this UE for PDN Disconnection exists.
   * Create a new transaction.
   */
  nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_disconnect = _esm_proc_get_pdn_connectivity_procedure(ue_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED);
  if(esm_proc_pdn_disconnect){
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - An PDN Connectivity procedure for UE ueId " MME_UE_S1AP_ID_FMT " exists. . \n", ue_id);
    if(esm_proc_pdn_disconnect->esm_base_proc.pti != pti){
      OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - Already an existing UE triggered ESM procedure (pti=%d, type=%d) for UE " MME_UE_S1AP_ID_FMT "."
          " Rejecting request for new procedure (pti=%d).\n", esm_proc_pdn_disconnect->esm_base_proc.pti, esm_proc_pdn_disconnect->esm_base_proc.type, ue_id, pti);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
    }
    /** Procedure already exists. Check if a response message is appended to the procedure.. Return the procedure. */
    OAILOG_WARNING(LOG_NAS_ESM, "ESM-SAP   - Found a valid ESM pdn connectivity procedure (pti=%d) for UE " MME_UE_S1AP_ID_FMT ". Rehandling the ESM disconnection procedure. \n", pti, ue_id);
    /*
     * Return deactivate EPS bearer context request message for the default bearer of the pdn connection.
     * Sending the ESM Request directly, makes retransmission handling easier. We are not interested in the outcome of the Delete Session Response.
     */
    esm_send_deactivate_eps_bearer_context_request (pti, ebi, esm_rsp_msg, ESM_CAUSE_REGULAR_DEACTIVATION);
    /*
     * Restart the timer.
     */
    nas_stop_esm_timer(ue_id, &esm_proc_pdn_disconnect->esm_base_proc.esm_proc_timer);
    /** Start the T3485 timer for additional PDN connectivity. */
    esm_proc_pdn_disconnect->esm_base_proc.esm_proc_timer.id = nas_esm_timer_start (mme_config.nas_config.t3495_sec, 0, (nas_esm_proc_t*)esm_proc_pdn_disconnect); /**< Address field should be big enough to save an ID. */
    /* Don't change the timeout handler (should still be (_eps_bearer_deactivate_t3495_handler). */
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  }
  DevAssert(!esm_proc_pdn_disconnect);
  /* Check if a bearer context procedure exists (any). */
  nas_esm_proc_bearer_context_t * esm_proc_bearer_context = _esm_proc_get_bearer_context_procedure(ue_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, ESM_EBI_UNASSIGNED);
  if(esm_proc_bearer_context){
    /** A network initiated procedure exists. Not allowed (not removing the procedure). */
    OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - UE initiated ESM information procedure (pti=%d) for UE " MME_UE_S1AP_ID_FMT " collided with network initiated bearer procedure (pti=0).\n", pti, ue_id);
    /** Currently not handling both procedures. */
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_COLLISION_WITH_NETWORK_INITIATED_REQUEST);
  }
  /*
   * Execute the PDN disconnect procedure requested by the UE.
   * Starting the T3495 timer and setting it as the timeout handler.
   * Validating the message in the context of the UE and finding the PDN context to remove (validating not last PDN).
   * Setting the ESM contexts of the bearer into INACTIVE_PENDING state (not caring about the CN states).
   * Triggering a Delete Session Request for the PDN.
   */
  // todo: PCOs @ esm disconnect
  esm_cause_t esm_cause = esm_proc_pdn_disconnect_request (ue_id, pti, PDN_CONTEXT_IDENTIFIER_UNASSIGNED, msg->linkedepsbeareridentity);
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
  else if (ebi == ESM_EBI_UNASSIGNED) {
    /*
     * 3GPP TS 24.301, section 7.3.2, case b
     * * * * Reserved or assigned EPS bearer identity value (Transaction Related messages).
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)\n", ebi);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  nas_esm_proc_pdn_connectivity_t * esm_pdn_connectivity_proc = _esm_proc_get_pdn_connectivity_procedure(ue_id, pti);
  if(!esm_pdn_connectivity_proc){
    OAILOG_WARNING(LOG_NAS_ESM, "ESM-SAP   - No procedure for UE " MME_UE_S1AP_ID_FMT " found. Ignoring the received ESM message ADBA.\n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  }

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
  /*
   * Return the ESM cause value
   */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
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
  const activate_default_eps_bearer_context_reject_msg * msg)
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

  nas_esm_proc_pdn_connectivity_t * esm_pdn_connectivity_proc = _esm_proc_get_pdn_connectivity_procedure(ue_id, pti);
  if(!esm_pdn_connectivity_proc){
    OAILOG_WARNING(LOG_NAS_ESM, "ESM-SAP   - No procedure for UE " MME_UE_S1AP_ID_FMT " found. Ignoring the received ESM information request.\n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  }

  /*
   * Remove the PDN Context for the given APN.
   * No EMM message will be triggered, just a local remove.
   * Inform the SAE-GW about the failed bearers.
   */
  esm_proc_pdn_connectivity_failure(ue_id, esm_pdn_connectivity_proc);

  /*
   * Return the ESM cause value
   */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
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
  mme_ue_s1ap_id_t ue_id,
  proc_tid_t pti,
  ebi_t ebi,
  const activate_dedicated_eps_bearer_context_accept_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNok;

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

  /*
   * Check that an EPS procedure exists.
   */
  nas_esm_proc_bearer_context_t * esm_bearer_procedure = _esm_proc_get_bearer_context_procedure(ue_id, pti, ebi);
  if(!esm_bearer_procedure){
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-PROC  - No ESM bearer procedure exists for successfully modified dedicated bearer (ebi=%d, pti=%d) for UE " MME_UE_S1AP_ID_FMT ". \n", ebi, pti, ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED);
  }

  /*
   * Execute the dedicated EPS bearer context activation procedure accepted
   * * * * by the UE
   */
  esm_cause = esm_proc_dedicated_eps_bearer_context_accept (ue_id, ebi, esm_bearer_procedure);
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
  mme_ue_s1ap_id_t ue_id,
  proc_tid_t pti,
  ebi_t ebi,
  const activate_dedicated_eps_bearer_context_reject_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

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

  nas_esm_proc_bearer_context_t * esm_proc_bearer_context = _esm_proc_get_bearer_context_procedure(ue_id, pti, ebi);
  if(!esm_proc_bearer_context){
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - No EPS bearer context modification procedure exists for pti=%d" "not accepted by the UE (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d, cause=%d)\n",
        pti, ue_id, ebi, msg->esmcause);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
  }

  /*
   * Execute the dedicated EPS bearer context activation procedure not
   * * * *  accepted by the UE
   */
  esm_proc_dedicated_eps_bearer_context_reject (ue_id, ebi, esm_proc_bearer_context, msg->esmcause);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
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
  mme_ue_s1ap_id_t ue_id,
  proc_tid_t pti,
  ebi_t ebi,
  const modify_eps_bearer_context_accept_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;

  /*
   * Procedure transaction identity checking.
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
   * Check that an EPS procedure exists.
   */
  nas_esm_proc_bearer_context_t * esm_bearer_procedure = _esm_proc_get_bearer_context_procedure(ue_id, pti, ebi);
  if(!esm_bearer_procedure){
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-PROC  - No ESM bearer procedure exists for accepted dedicated bearer (ebi=%d, pti=%d) for UE " MME_UE_S1AP_ID_FMT ". \n", ebi, pti, ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED);
  }
  /*
   * Execute the dedicated EPS bearer context activation procedure accepted
   * * * * by the UE
   */
  esm_cause = esm_proc_modify_eps_bearer_context_accept(ue_id, ebi, esm_bearer_procedure);
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
  mme_ue_s1ap_id_t ue_id,
  proc_tid_t pti,
  ebi_t ebi,
  const modify_eps_bearer_context_reject_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

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

  nas_esm_proc_bearer_context_t * esm_proc_bearer_context = _esm_proc_get_bearer_context_procedure(ue_id, pti, ebi);
  if(!esm_proc_bearer_context){
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - No EPS bearer context modification procedure exists for pti=%d" "not accepted by the UE (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d, cause=%d)\n",
        pti, ue_id, ebi, msg->esmcause);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
  }

  /*
   * Execute the dedicated EPS bearer context activation procedure not
   * * * *  accepted by the UE
   */
  esm_proc_modify_eps_bearer_context_reject(ue_id, ebi, esm_proc_bearer_context, msg->esmcause);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
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
  mme_ue_s1ap_id_t ue_id,
  proc_tid_t pti,
  ebi_t ebi,
  const deactivate_eps_bearer_context_accept_msg * msg)
{
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

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
   * Check the procedure for the UE with the PTI (could be 0).
   * It could be a PDN disconnection or a EPS bearer deactivation (pti >=0).
   * Only one transaction is allowed.
   */
  nas_esm_proc_t * esm_base_proc = _esm_proc_get_pdn_connectivity_procedure(ue_id, pti);
  if(!esm_base_proc){
    // todo: assume both cannot exist together.
    esm_base_proc = _esm_proc_get_bearer_context_procedure(ue_id, pti, ebi);
    if(!esm_base_proc){
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - No ESM procedure (neither bearer nor pdn) could be found for UE " MME_UE_S1AP_ID_FMT " (pti=%d)\n", ue_id, pti);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_PTI_VALUE);
    }
  }
  if(esm_base_proc->type == ESM_PROC_PDN_CONTEXT){
    /** Deactivate the PDN context. */
    nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity = (nas_esm_proc_pdn_connectivity_t*)esm_base_proc;
    /*
     * Delete the PDN connectivity in the MME_APP UE context.
     */
    mme_app_esm_delete_pdn_context(ue_id, esm_proc_pdn_connectivity->subscribed_apn, esm_proc_pdn_connectivity->pdn_cid, esm_proc_pdn_connectivity->default_ebi); /**< Frees it by putting it back to the pool. */
    _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
    /** Nothing to signal. */
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  }else {
    nas_esm_proc_bearer_context_t * esm_proc_bearer_context = (nas_esm_proc_bearer_context_t*)esm_base_proc;
    /** Bearer Context Procedures may be transactional or not. */
    mme_app_release_bearer_context(ue_id, NULL, esm_proc_bearer_context->linked_ebi, ebi);
    _esm_proc_free_bearer_context_procedure(&esm_proc_bearer_context);
    /*
     * Deactivate the EPS bearer context (must be a dedicated bearer).
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - We released  the dedicated EBI. Responding with delete bearer response back. (ebi=%d) for UE " MME_UE_S1AP_ID_FMT ". \n", ebi, ue_id);
    /** Respond per bearer. */
    nas_itti_dedicated_eps_bearer_deactivation_complete(ue_id, ebi);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  }
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

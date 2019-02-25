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
  Source      PdnConnectivity.c

  Version     0.1

  Date        2013/01/02

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Frederic Maurel

  Description Defines the PDN connectivity ESM procedure executed by the
        Non-Access Stratum.

        The PDN connectivity procedure is used by the UE to request
        the setup of a default EPS bearer to a PDN.

        The procedure is used either to establish the 1st default
        bearer by including the PDN CONNECTIVITY REQUEST message
        into the initial attach message, or to establish subsequent
        default bearers to additional PDNs in order to allow the UE
        simultaneous access to multiple PDNs by sending the message
        stand-alone.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "assertions.h"
#include "log.h"
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "3gpp_36.401.h"
#include "mme_app_ue_context.h"
#include "common_defs.h"
#include "mme_api.h"
#include "mme_app_apn_selection.h"
#include "mme_app_pdn_context.h"
#include "mme_app_bearer_context.h"
#include "mme_app_defs.h"
#include "esm_data.h"
#include "esm_ebr.h"
#include "esm_proc.h"
#include "esm_pt.h"
#include "esm_cause.h"
#include "esm_sapDef.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
    Internal data handled by the PDN connectivity procedure in the MME
   --------------------------------------------------------------------------
*/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_pdn_connectivity_reject()                        **
 **                                                                        **
 ** Description: Builds PDN Connectivity Reject message                    **
 **                                                                        **
 **      The PDN connectivity reject message is sent by the net-   **
 **      work to the UE to reject establishment of a PDN connec-   **
 **      tion.                                                     **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity             **
 **      esm_cause: ESM cause code                             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void
esm_send_pdn_connectivity_reject (
  pti_t pti,
  ESM_msg * esm_msg,
  esm_cause_t esm_cause)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  memset(esm_msg, 0, sizeof(ESM_msg));
  /*
   * Mandatory - ESM message header
   */
  esm_msg->pdn_connectivity_reject.protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  esm_msg->pdn_connectivity_reject.epsbeareridentity = EPS_BEARER_IDENTITY_UNASSIGNED;
  esm_msg->pdn_connectivity_reject.messagetype = PDN_CONNECTIVITY_REJECT;
  esm_msg->pdn_connectivity_reject.proceduretransactionidentity = pti;
  /*
   * Mandatory - ESM cause code
   */
  esm_msg->pdn_connectivity_reject.esmcause = esm_cause;
  /*
   * Optional IEs
   */
  esm_msg->pdn_connectivity_reject.presencemask = 0;
  OAILOG_DEBUG (LOG_NAS_ESM, "ESM-SAP   - Send PDN Connectivity Reject message " "(pti=%d, ebi=%d)\n",
      esm_msg->pdn_connectivity_reject.proceduretransactionidentity, esm_msg->pdn_connectivity_reject.epsbeareridentity);
  OAILOG_FUNC_OUT(LOG_NAS_ESM);
}

//-----------------------------------------------------------------------------
nas_esm_proc_pdn_connectivity_t *_esm_proc_create_pdn_connectivity_procedure(mme_ue_s1ap_id_t ue_id, imsi_t *imsi, pti_t pti)
{
  /** APN may be missing at the beginning. */
  nas_esm_proc_pdn_connectivity_t  *esm_proc_pdn_connectivity = mme_app_nas_esm_create_pdn_connectivity_procedure(ue_id, pti);
  AssertFatal(esm_proc_pdn_connectivity, "TODO Handle this");

  if(imsi)
    memcpy((void*)&esm_proc_pdn_connectivity->imsi, imsi, sizeof(imsi_t));
  return esm_proc_pdn_connectivity;
}

//-----------------------------------------------------------------------------
void _esm_proc_free_pdn_connectivity_procedure(nas_esm_proc_pdn_connectivity_t ** esm_proc_pdn_connectivity){
  // free content
  void *unused = NULL;
  /** Forget the name of the timer.. only one can exist (also used for activate default EPS bearer context.. */
  nas_stop_esm_timer((*esm_proc_pdn_connectivity)->esm_base_proc.ue_id,
      &((*esm_proc_pdn_connectivity)->esm_base_proc.esm_proc_timer));
  mme_app_nas_esm_delete_pdn_connectivity_proc(esm_proc_pdn_connectivity);
}

//-----------------------------------------------------------------------------
nas_esm_proc_pdn_connectivity_t *_esm_proc_get_pdn_connectivity_procedure(mme_ue_s1ap_id_t ue_id, pti_t pti){
  return mme_app_nas_esm_get_pdn_connectivity_procedure(ue_id, pti);
}

/*
   --------------------------------------------------------------------------
        PDN connectivity procedure executed by the MME
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_connectivity_request()                       **
 **                                                                        **
 ** Description: Performs PDN connectivity procedure requested by the UE.  **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.1.3                           **
 **      Upon receipt of the PDN CONNECTIVITY REQUEST message, the **
 **      MME checks if connectivity with the requested PDN can be  **
 **      established. If no requested  APN  is provided  the  MME  **
 **      shall use the default APN as the  requested  APN if the   **
 **      request type is different from "emergency", or the APN    **
 **      configured for emergency bearer services if the request   **
 **      type is "emergency".                                      **
 **      If connectivity with the requested PDN is accepted by the **
 **      network, the MME shall initiate the default EPS bearer    **
 **      context activation procedure.                             **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **      esm_pdn_connectivity_proc: ESM PDN connectivity procedure.
 **      Others:    apn_configuration       **
 **                                                                        **
 **      Return:    The identifier of the PDN connection if    **
 **             successfully created;                      **
 **             RETURNerror otherwise.                     **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ***************************************************************************/

esm_cause_t
esm_proc_pdn_connectivity_request (
  mme_ue_s1ap_id_t             ue_id,
  imsi_t                      *imsi,
  tai_t                       *visited_tai,
  nas_esm_proc_pdn_connectivity_t * const esm_proc_pdn_connectivity,
  const apn_configuration_t   *apn_configuration)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - PDN connectivity requested by the UE "
             "(ue_id=" MME_UE_S1AP_ID_FMT ", pti=%d) PDN type = %s, APN = %s \n", ue_id, esm_proc_pdn_connectivity->esm_base_proc.pti,
             (esm_proc_pdn_connectivity->pdn_type == ESM_PDN_TYPE_IPV4) ? "IPv4" : (esm_proc_pdn_connectivity->pdn_type == ESM_PDN_TYPE_IPV6) ? "IPv6" : "IPv4v6",
             (char *)bdata(esm_proc_pdn_connectivity->subscribed_apn));
  /*
   * Create new PDN context in the MME_APP UE context.
   * This will also allocate a bearer context (NULL bearer set).
   */
  pdn_context_t * pdn_context = NULL;
  int rc = mme_app_esm_create_pdn_context(ue_id, ESM_EBI_UNASSIGNED, apn_configuration, esm_proc_pdn_connectivity->subscribed_apn, apn_configuration->context_identifier, &apn_configuration->ambr, &pdn_context);
  if (rc != RETURNok) {
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - Failed to create PDN connection for UE " MME_UE_S1AP_ID_FMT ", apn = \"%s\" (pti=%d).\n", ue_id, bdata(esm_proc_pdn_connectivity->subscribed_apn),
        esm_proc_pdn_connectivity->esm_base_proc.pti);
    /** No empty/garbage contexts. */
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }
  esm_proc_pdn_connectivity->default_ebi = pdn_context->default_ebi;
  /*
   * Inform the MME_APP procedure to establish a session in the SAE-GW.
   */
  mme_app_send_s11_create_session_req(ue_id, imsi, pdn_context, visited_tai, &esm_proc_pdn_connectivity->pco, false);
  /** No message returned and no timeout set for the ESM procedure. */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_connectivity_retx()                       **
 **                                                                        **
 ** Description: Performs PDN connectivity retx handling.           **
 **
 **
 **
 **         TODO: FILL DESCRIPTION                                                               **
 **              3GPP TS 24.301, section 6.5.1.3                           **
 **      Upon receipt of the PDN CONNECTIVITY REQUEST message, the **
 **      MME checks if connectivity with the requested PDN can be  **
 **      established. If no requested  APN  is provided  the  MME  **
 **      shall use the default APN as the  requested  APN if the   **
 **      request type is different from "emergency", or the APN    **
 **      configured for emergency bearer services if the request   **
 **      type is "emergency".                                      **
 **      If connectivity with the requested PDN is accepted by the **
 **      network, the MME shall initiate the default EPS bearer    **
 **      context activation procedure.                             **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **      pti:       Identifies the PDN connectivity procedure  **
 **             requested by the UE                        **
 **      request_type:  Type of the PDN request                    **
 **      pdn_type:  PDN type value (IPv4, IPv6, IPv4v6)        **
 **      apn:       Requested Access Point Name                **
 **      Others:    _esm_data                                  **
 **                                                                        **
 **      Return:    The identifier of the PDN connection if    **
 **             successfully created;                      **
 **             RETURNerror otherwise.                     **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ***************************************************************************/

esm_cause_t
esm_proc_pdn_connectivity_retx(const mme_ue_s1ap_id_t ue_id, const nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity, ESM_msg * esm_rsp_msg){
  OAILOG_FUNC_IN(LOG_NAS_ESM);
  /*
   * Check that the PDN context exists and is in the correct ACTIVATION-PENDING state.
   * If not, ignore the received PDN connectivity request.
   */
  pdn_context_t *dup_pdn_context = NULL;
  mme_app_get_pdn_context(ue_id, esm_proc_pdn_connectivity->pdn_cid, esm_proc_pdn_connectivity->default_ebi, esm_proc_pdn_connectivity->subscribed_apn, &dup_pdn_context);
  if(!dup_pdn_context){
    OAILOG_WARNING(LOG_NAS_ESM, "ESM-SAP   - No PDN context for the APN \"%s\" exists yet for received retransmission. "
        "Ignoring received PDN connectivity request." "(ue_id=%d, pti=%d)\n",
        (esm_proc_pdn_connectivity->subscribed_apn) ? bdata(esm_proc_pdn_connectivity->subscribed_apn) : "NULL", ue_id, esm_proc_pdn_connectivity->esm_base_proc.pti);
    OAILOG_FUNC_RETURN(LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED);
  }
  /** Check that it is in the correct state. */
  bearer_context_t * bearer_context = mme_app_get_session_bearer_context(dup_pdn_context, dup_pdn_context->default_ebi);
  if(!bearer_context || bearer_context->esm_ebr_context.status != ESM_EBR_ACTIVE_PENDING){
    OAILOG_WARNING(LOG_NAS_ESM, "ESM-SAP   - No default bearer for PDN context for the APN \"%s\" exists yet or is not in ACTIVE_PENDING state (%d). "
        "Ignoring received PDN connectivity request." "(ue_id=%d, pti=%d)\n",
        (esm_proc_pdn_connectivity->subscribed_apn) ? bdata(esm_proc_pdn_connectivity->subscribed_apn) : "NULL",
            (bearer_context) ? esm_ebr_state2string(bearer_context->esm_ebr_context.status) : "NULL",
                ue_id, esm_proc_pdn_connectivity->esm_base_proc.pti);
    OAILOG_FUNC_RETURN(LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED);
  }
  /** Process duplicate transaction. */
  OAILOG_WARNING(LOG_NAS_ESM, "ESM-SAP   - Found an established PDN context for the APN \"%s\" from an earlier message. "
      "Resending PDN connectivity request and restarting T3485." "(ue_id=%d, pti=%d)\n",
      bdata(esm_proc_pdn_connectivity->subscribed_apn), ue_id, esm_proc_pdn_connectivity->esm_base_proc.pti);
  esm_send_activate_default_eps_bearer_context_request(esm_proc_pdn_connectivity, &dup_pdn_context->subscribed_apn_ambr,
      &bearer_context->bearer_level_qos, dup_pdn_context->paa, dup_pdn_context->pco, esm_rsp_msg);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
}

/****************************************************************************
 **                                                                        **
 ** Name:        esm_proc_pdn_connectivity_response()                       **
 **                                                                        **
 ** Description: Performs PDN connectivity responseupon receiving noti-  **
 **              fication from the EPS Mobility Management sublayer that   **
 **              EMM procedure that initiated PDN connectivity activation  **
 **              succeeded.                                                **
 **                                                                        **
 **         Inputs:  ue_id:      UE local identifier                        **
 **                  pdn_cid:       Identifier of the PDN connection to be     **
 **                             released                                   **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    RETURNok, RETURNerror                      **
 **                  Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t esm_proc_pdn_connectivity_res (mme_ue_s1ap_id_t ue_id, nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity) {
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                           rc = RETURNok;
  /*
   * Update default EPS bearer context and pdn context.
   * The PCO of the ESM procedure does not need to be updated. The PDN context already has an updated PCO.
   */

  /** All fields must be set, just update the ESM_EBR_STATE. */
  rc = mme_app_esm_update_ebr_state(ue_id, esm_proc_pdn_connectivity->subscribed_apn, esm_proc_pdn_connectivity->pdn_cid, esm_proc_pdn_connectivity->default_ebi, esm_proc_pdn_connectivity->default_ebi, ESM_EBR_ACTIVE_PENDING);
  if (rc == RETURNerror) {
    OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Could not update the default EPS bearer context activation (ue_id=" MME_UE_S1AP_ID_FMT ", context_identifier=%d). \n. ",
        ue_id, esm_proc_pdn_connectivity->pdn_cid);
    /** Remove the ESM procedure. */
    _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }
  OAILOG_FUNC_RETURN(LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
}

/****************************************************************************
 **                                                                        **
 ** Name:        esm_proc_pdn_connectivity_failure()                       **
 **                                                                        **
 ** Description: Performs PDN connectivity procedure upon receiving noti-  **
 **              fication from the EPS Mobility Management sublayer that   **
 **              EMM procedure that initiated PDN connectivity activation  **
 **              locally failed.                                           **
 **                                                                        **
 **              The MME releases the PDN connection entry allocated when  **
 **              the PDN connectivity procedure was requested by the UE.   **
 **                                                                        **
 **         Inputs:  ue_id:      UE local identifier                        **
 **                  pdn_cid:       Identifier of the PDN connection to be     **
 **                             released                                   **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    RETURNok, RETURNerror                      **
 **                  Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void esm_proc_pdn_connectivity_failure (mme_ue_s1ap_id_t ue_id, nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - PDN connectivity failure (ue_id=" MME_UE_S1AP_ID_FMT ", pdn_cid=%d, ebi%d)\n", ue_id, esm_proc_pdn_connectivity->pdn_cid, esm_proc_pdn_connectivity->default_ebi);

  pdn_context_t *pdn_context = NULL;
  mme_app_get_pdn_context(ue_id, esm_proc_pdn_connectivity->pdn_cid, esm_proc_pdn_connectivity->default_ebi, esm_proc_pdn_connectivity->subscribed_apn, &pdn_context);
  if(pdn_context){
    /*
     * Delete the PDN connectivity in the MME_APP UE context.
     */
    struct in_addr saegw_peer_ipv4 = pdn_context->s_gw_address_s11_s4.address.ipv4_address;
    if(saegw_peer_ipv4.s_addr == 0){
      mme_app_select_service(&esm_proc_pdn_connectivity->visited_tai, &saegw_peer_ipv4);
    }

    nas_itti_pdn_disconnect_req(ue_id, esm_proc_pdn_connectivity->default_ebi, esm_proc_pdn_connectivity->esm_base_proc.pti, false,
        saegw_peer_ipv4, pdn_context->s_gw_teid_s11_s4,
        esm_proc_pdn_connectivity->pdn_cid);
    mme_app_esm_delete_pdn_context(ue_id, esm_proc_pdn_connectivity->subscribed_apn, esm_proc_pdn_connectivity->pdn_cid, esm_proc_pdn_connectivity->default_ebi); /**< Frees it by putting it back to the pool. */
  }
  /*
   * Remove the ESM procedure and stop the timer.
   */
  _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
  OAILOG_FUNC_OUT(LOG_NAS_ESM);
}

/****************************************************************************
 **                                                                        **
 ** Name:        esm_proc_pdn_config_res()                                 **
 **                                                                        **
 ** Description: Performs PDN config response processing. Decides on if a
 **              PDN Connectivity has to be established or not.            **
 **                                                                        **
 **         Inputs:  ue_id:      UE local identifier                       **
 **                  imsi:       IMSI of the UE                            **
 **                                                                        **
 ** Outputs:         esm_msg:   response message of the ESM                **
 **                  Return:    RETURNok, RETURNerror                      **
 **                  Others:    None                                       **
 **                                                                        **
 ***************************************************************************/

esm_cause_t
esm_proc_pdn_config_res(mme_ue_s1ap_id_t ue_id, bool * is_attach, pti_t * pti, imsi64_t imsi){
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  ebi_t                                   new_ebi = 0;
  pdn_context_t                          *pdn_context = NULL;
  struct apn_configuration_s             *apn_config = NULL;
  bearer_context_t                       *bearer_context = NULL;
  int                                     rc = RETURNok;

  /*
   * Check if a procedure exists for PDN Connectivity. If so continue with it.
   */
  nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity = _esm_proc_get_pdn_connectivity_procedure(ue_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED);
  if(!esm_proc_pdn_connectivity){
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - No ESM PDN connectivity procedure for UE ueId " MME_UE_S1AP_ID_FMT " exists. Handling the current pdn contexts. \n", ue_id);
    *is_attach = true;
    /** Update the current pdn contexts. */
    subscription_data_t *subscription_data = mme_ue_subscription_data_exists_imsi(&mme_app_desc.mme_ue_contexts, imsi);
    if(!subscription_data){
      OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "PDN configuration response not processed in EMM layer only due TAU. Ignoring (should be sent to ESM).. for id " MME_UE_S1AP_ID_FMT ". \n", ue_id);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, ESM_CAUSE_NETWORK_FAILURE);
    }
    esm_cause_t esm_cause = mme_app_update_pdn_context(ue_id, subscription_data);
    OAILOG_FUNC_RETURN(LOG_NAS_ESM, esm_cause);
  }
  *is_attach = esm_proc_pdn_connectivity->is_attach;
  *pti = esm_proc_pdn_connectivity->esm_base_proc.pti;
  /** There may still be no APN set. Get a (default) APN configuration. */
  if(mme_app_select_apn(imsi, esm_proc_pdn_connectivity->subscribed_apn, &apn_config) == RETURNerror){
    DevAssert(esm_proc_pdn_connectivity->subscribed_apn);
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - No APN configuration could be found for APN \"%s\". "
        "Rejecting the PDN connectivity procedure." "(ue_id=%d, pti=%d)\n", bdata(esm_proc_pdn_connectivity->subscribed_apn), ue_id, *pti);
    _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_UNKNOWN_ACCESS_POINT_NAME);
  } else if (!apn_config){
    _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - No APN configuration could be found (after ULA). Rejecting the PDN connectivity procedure." "(ue_id="MME_UE_S1AP_ID_FMT", pti=%d)\n", ue_id, *pti);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_UNKNOWN_ACCESS_POINT_NAME);
  }

  if(!esm_proc_pdn_connectivity->subscribed_apn){
    if(!esm_proc_pdn_connectivity->subscribed_apn){
      esm_proc_pdn_connectivity->subscribed_apn = blk2bstr(apn_config->service_selection, apn_config->service_selection_length);  /**< Set the APN-NI from the service selection. */
    }
  }
  esm_proc_pdn_connectivity->pdn_cid = apn_config->context_identifier;

  /**
   * For the case of PDN Connectivity via ESM, we will always trigger an S11 Create Session Request.
   * We won't enter this case in case of a Tracking Area Update procedure.
   */
  pdn_context_t *pdn_context_duplicate = NULL;
  mme_app_get_pdn_context(ue_id, PDN_CONTEXT_IDENTIFIER_UNASSIGNED, ESM_EBI_UNASSIGNED, esm_proc_pdn_connectivity->subscribed_apn, &pdn_context_duplicate);
  if(pdn_context_duplicate){
    /*
     * This can only be initial. Discard the ULA (assume that the current procedure is still being worked on).
     * No message will be triggered below.
     */
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - Found an established PDN context for the APN \"%s\". Ignoring the received PDN configuration." "(ue_id=%d, pti=%d)\n",
        bdata(esm_proc_pdn_connectivity->subscribed_apn), ue_id, esm_proc_pdn_connectivity->esm_base_proc.pti);
    /** Not removing the ESM procedure. */
    OAILOG_FUNC_RETURN(LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  }
  /** Directly process the ULA. A response message might be returned. */
  /*
   * Execute the PDN connectivity procedure requested by the UE.
   * Allocate a new PDN context and trigger an S11 Create Session Request.
   */
  //      (esm_context->esm_proc_data->pco.num_protocol_or_container_id ) ? &esm_context->esm_proc_data->pco:NULL,
  if(esm_proc_pdn_connectivity_request (ue_id, &esm_proc_pdn_connectivity->imsi, &esm_proc_pdn_connectivity->visited_tai,
      esm_proc_pdn_connectivity, apn_config) != ESM_CAUSE_SUCCESS){
    /** Remove the procedure. */
    _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
    /** No PDN Context is expected. */
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }
  /** The procedure will stay. */
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, ESM_CAUSE_SUCCESS);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

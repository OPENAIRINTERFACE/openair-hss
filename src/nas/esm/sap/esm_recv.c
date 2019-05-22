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
#include "commonDef.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "nas_itti_messaging.h"
#include "esm_recv.h"
#include "esm_pt.h"
#include "esm_ebr.h"
#include "esm_proc.h"
#include "esm_cause.h"
#include "mme_config.h"
#include "esm_sap.h"
#include "mme_app_apn_selection.h"

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
  emm_data_context_t * emm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const esm_status_msg * msg)
{
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_ESM);
  OAILOG_INFO(LOG_NAS_ESM,  "ESM-SAP   - Received ESM status message (pti=%d, ebi=%d)\n", pti, ebi);
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
  rc = esm_proc_status_ind (emm_context, pti, ebi, &esm_cause);

  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

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
 **      pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     new_ebi:   New assigned EPS bearer identity           **
 **      data:      PDN connection and EPS bearer context data **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_recv_pdn_connectivity_request (
  emm_data_context_t * emm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const pdn_connectivity_request_msg * msg,
  ebi_t *new_ebi,
  bool *is_pdn_connectivity,
  pdn_context_t **pdn_context_pp)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     esm_cause       = ESM_CAUSE_SUCCESS;
  ue_context_t                           *ue_context      = NULL;
  pdn_context_t                          *new_pdn_context = NULL;
  int                                     rc              = RETURNerror;

  OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Received PDN Connectivity Request message " "(ue_id=%u, pti=%d, ebi=%d)\n", emm_context->ue_id, pti, ebi);

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
   * Message processing
   */
  /*
   * Get PDN connection and EPS bearer context data structure to setup
   */
  if (!emm_context->esm_ctx.esm_proc_data) {
    // todo: why not checking if another ESM procedure is running?
    // todo: timers will be reset like in EMM procedures?
    emm_context->esm_ctx.esm_proc_data  = (esm_proc_data_t *) calloc(1, sizeof(*emm_context->esm_ctx.esm_proc_data));
  }else{
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - ESM Proc Data was existing!" "(ue_id=%d)\n", emm_context->ue_id);
  }

  struct esm_proc_data_s * esm_data = emm_context->esm_ctx.esm_proc_data;

  esm_data->pti = pti;
  /*
   * Get the PDN connectivity request type
   */

  if (msg->requesttype == REQUEST_TYPE_INITIAL_REQUEST) {
    esm_data->request_type = ESM_PDN_REQUEST_INITIAL;
  } else if (msg->requesttype == REQUEST_TYPE_HANDOVER) {
    esm_data->request_type = ESM_PDN_REQUEST_HANDOVER;
  } else if (msg->requesttype == REQUEST_TYPE_EMERGENCY) {
    esm_data->request_type = ESM_PDN_REQUEST_EMERGENCY;
  } else {
    /*
     * Unknown PDN request type
     */
    esm_data->request_type = -1;
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Invalid PDN request type (INITIAL/HANDOVER/EMERGENCY)\n");
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_MANDATORY_INFO);
  }

  /*
   * Get the value of the PDN type indicator
   */
  if (msg->pdntype == PDN_TYPE_IPV4) {
    esm_data->pdn_type = ESM_PDN_TYPE_IPV4;
  } else if (msg->pdntype == PDN_TYPE_IPV6) {
    esm_data->pdn_type = ESM_PDN_TYPE_IPV6;
  } else if (msg->pdntype == PDN_TYPE_IPV4V6) {
    esm_data->pdn_type = ESM_PDN_TYPE_IPV4V6;
  } else {
    /*
     * Unkown PDN type
     */
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Invalid PDN type\n");
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_UNKNOWN_PDN_TYPE);
  }

  /*
   * Get the Access Point Name, if provided
   */
  if (msg->presencemask & PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT) {
    if (esm_data->apn) bdestroy_wrapper(&esm_data->apn);
    esm_data->apn = msg->accesspointname;
  }

  if (msg->presencemask & PDN_CONNECTIVITY_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT) {
    if (esm_data->pco.num_protocol_or_container_id) clear_protocol_configuration_options(&esm_data->pco);
    copy_protocol_configuration_options(&esm_data->pco, &msg->protocolconfigurationoptions);
  }
  /*
   * Get the ESM information transfer flag
   */
  if (msg->presencemask & PDN_CONNECTIVITY_REQUEST_ESM_INFORMATION_TRANSFER_FLAG_PRESENT) {
    /*
     * 3GPP TS 24.301, sections 6.5.1.2, 6.5.1.3
     * * * * ESM information, i.e. protocol configuration options, APN, or both,
     * * * * has to be sent after the NAS signalling security has been activated
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
      esm_proc_esm_information_request(emm_context, pti);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
    }
  }

#if ORIGINAL_CODE
  /*
   * Execute the PDN connectivity procedure requested by the UE
   */
  ambr_t ambr = {0};
  int pid = esm_proc_pdn_connectivity_request (emm_context, pti, request_type,
      &esm_data->apn,
      esm_data->pdn_type,
      &esm_data->pdn_addr,
      &ambr
      &esm_data->qos,
      &esm_cause);

  if (pid != RETURNerror) {
    /*
     * Create local default EPS bearer context
     */
    int rc = esm_proc_default_eps_bearer_context (ctx, pid, new_ebi, &esm_data->qos, &esm_cause);

    if (rc != RETURNerror) {
      esm_cause = ESM_CAUSE_SUCCESS;
    }
  }
#else

  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);

  if(ue_context->subscription_known != SUBSCRIPTION_KNOWN){
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - UE Context has no subscription information. Triggering ULR. " "(ue_id=%d, pti=%d)\n",
        emm_context->ue_id, pti);
    nas_itti_pdn_config_req(emm_context->ue_id, &emm_context->_imsi, esm_data, esm_data->request_type);
    esm_cause = ESM_CAUSE_SUCCESS;
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
  }
  if(!msg->accesspointname){
    /** No APN Name received from UE. */
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - No APN name received from UE to establish PDN connection and subscription exists. Rejecting. " "(ue_id=%d, pti=%d)\n",
        emm_context->ue_id, pti);
    /** Will send automatic PDN connectivity reject. */
    esm_cause = ESM_CAUSE_UNKNOWN_ACCESS_POINT_NAME;
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
  }

  /** Checking if APN configuration profile for the desired apn profile exists. */
  struct apn_configuration_s* apn_config = mme_app_select_apn(ue_context, msg->accesspointname);
  if(!apn_config){
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - UE Context has valid subscription information. No APN configuration for the APN \"%s\" has been downloaded from the HSS. " "(ue_id=%d, pti=%d)\n",
        bdata(msg->accesspointname), emm_context->ue_id, pti);
    /** Will send automatic PDN connectivity reject. */
    esm_cause = ESM_CAUSE_UNKNOWN_ACCESS_POINT_NAME;
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
  }
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - UE Context has valid subscription information. APN configuration for \"%s\" has also been downloaded from the HSS. Establishing PDN connectivity. " "(ue_id=%d, pti=%d)\n",
      bdata(msg->accesspointname), emm_context->ue_id, pti);
  /*
   * Check if the PDN Context exists, if so send a PDN Connectivity Response with success back.
   * It should then trigger a Default Bearer Setup Request.
   */
  pdn_context_t *pdn_context_duplicate = NULL;
  mme_app_get_pdn_context(ue_context, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, ESM_EBI_UNASSIGNED, msg->accesspointname, &pdn_context_duplicate);
  if(pdn_context_duplicate){
    // todo: better handling if PDN context exists
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - * * * * * ABNORMAL: An established PDN UE Context has valid subscription information. PDN connectivity for APN \"%s\" has already been established. "
        "Continuing with RAB establishment. " "(ue_id=%d, pti=%d)\n", bdata(msg->accesspointname), emm_context->ue_id, pti);
    /** Check that the default bearer exists. */
    bearer_context_t * bearer_context_duplicate = mme_app_get_session_bearer_context(pdn_context_duplicate, pdn_context_duplicate->default_ebi);
    DevAssert(bearer_context_duplicate);
    // todo: better checks
    DevAssert(ESM_EBR_ACTIVE == esm_ebr_get_status(emm_context, pdn_context_duplicate->default_ebi));


    esm_cause = ESM_CAUSE_SUCCESS;
    *is_pdn_connectivity = true;
    *pdn_context_pp = pdn_context_duplicate;
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
  }
  /*
   * Establish PDN connectivity.
   * Currently only checking that request type is INITIAL_REQUEST.
   * todo: perform other validations than request type.
   */
  if(msg->requesttype != REQUEST_TYPE_INITIAL_REQUEST){
    esm_cause = ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED;
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
  }
  /** Establish the PDN Connectivity. */
  // todo: check the PCOs and establish IPv6
  // todo: add the 3GPP Requirements

  /*
   * Set the ESM Proc Data values.
   * Update the UE context and PDN context information with it.
   * todo: how to check that this is still our last ESM proc data?
   */
  if(emm_context->esm_ctx.esm_proc_data){
    /*
     * Execute the PDN connectivity procedure requested by the UE
     */
    emm_context->esm_ctx.esm_proc_data->pdn_cid              = apn_config->context_identifier; /**< Set it to the one matched. */
    emm_context->esm_ctx.esm_proc_data->bearer_qos.qci       = apn_config->subscribed_qos.qci;
    emm_context->esm_ctx.esm_proc_data->bearer_qos.pci       = apn_config->subscribed_qos.allocation_retention_priority.pre_emp_capability;
    emm_context->esm_ctx.esm_proc_data->bearer_qos.pl        = apn_config->subscribed_qos.allocation_retention_priority.priority_level;
    emm_context->esm_ctx.esm_proc_data->bearer_qos.pvi       = apn_config->subscribed_qos.allocation_retention_priority.pre_emp_vulnerability;
    emm_context->esm_ctx.esm_proc_data->bearer_qos.gbr.br_ul = 0;
    emm_context->esm_ctx.esm_proc_data->bearer_qos.gbr.br_dl = 0;
    emm_context->esm_ctx.esm_proc_data->bearer_qos.mbr.br_ul = 0;
    emm_context->esm_ctx.esm_proc_data->bearer_qos.mbr.br_dl = 0;
    // TODO  "Better to throw emm_ctx->esm_ctx.esm_proc_data as a parameter or as a hidden parameter ?"
    // todo: if PDN_CONTEXT exist --> we might need to send an ESM update message like MODIFY EPS BEARER CONTEXT REQUEST to the UE
    rc = esm_proc_pdn_connectivity_request (emm_context,
        emm_context->esm_ctx.esm_proc_data->pti,
        apn_config->context_identifier,
        emm_context->esm_ctx.esm_proc_data->request_type,
        emm_context->esm_ctx.esm_proc_data->apn,
        emm_context->esm_ctx.esm_proc_data->pdn_type,
        emm_context->esm_ctx.esm_proc_data->pdn_addr,
        &apn_config->ambr,
        &emm_context->esm_ctx.esm_proc_data->bearer_qos,
        (emm_context->esm_ctx.esm_proc_data->pco.num_protocol_or_container_id ) ? &emm_context->esm_ctx.esm_proc_data->pco:NULL,
            &esm_cause,
            &new_pdn_context);

    pdn_context_t *pdn_ctx_p1 = NULL;
    mme_app_get_pdn_context(ue_context, apn_config->context_identifier, ue_context->next_def_ebi_offset + 5 - 1, emm_context->esm_ctx.esm_proc_data->apn, &pdn_ctx_p1);
    DevAssert(pdn_ctx_p1);

    // todo: optimize this
    DevAssert(new_pdn_context);
    if (rc != RETURNerror) {
        /*
         * Create local default EPS bearer context
         */
        if ((!is_pdn_connectivity) || ((is_pdn_connectivity) /*&& (EPS_BEARER_IDENTITY_UNASSIGNED == new_pdn_context->default_ebi)*/)) {
          rc = esm_proc_default_eps_bearer_context (emm_context, emm_context->esm_ctx.esm_proc_data->pti, new_pdn_context, emm_context->esm_ctx.esm_proc_data->apn, &new_ebi, &emm_context->esm_ctx.esm_proc_data->bearer_qos, &esm_cause);
        }
        // todo: if the bearer already exist, we may modify the qos parameters with Modify_Bearer_Request!

        if (rc != RETURNerror) {
          esm_cause = ESM_CAUSE_SUCCESS;
        }
      } else {
      }
  }
  *pdn_context_pp = new_pdn_context;
  esm_cause = ESM_CAUSE_SUCCESS;
#endif
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
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
  emm_data_context_t * emm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const pdn_disconnect_request_msg * msg,
  ebi_t *linked_ebi)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                               esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                          ue_id = emm_context->ue_id;

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
  int pid = esm_proc_pdn_disconnect_request (emm_context, pti, 0, *linked_ebi, &esm_cause);

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
//    int rc = esm_proc_eps_bearer_context_deactivate (emm_context, false, ESM_SAP_ALL_EBI, pid, &esm_cause);

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



//------------------------------------------------------------------------------
esm_cause_t esm_recv_information_response (
  emm_data_context_t * emm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const esm_information_response_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                               esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                          ue_id = emm_context->ue_id;

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

  /*
   * Message processing
   */
  /*
   * Execute the PDN disconnect procedure requested by the UE
   */

  int pid = esm_proc_esm_information_response (emm_context, pti, msg->accesspointname, &msg->protocolconfigurationoptions, &esm_cause);

  if (pid != RETURNerror) {


    emm_data_context_t * emm_context_test = emm_data_context_get_by_imsi (&_emm_data, emm_context->_imsi64);
    DevAssert(emm_context_test);


    // Continue with pdn connectivity request
    nas_itti_pdn_config_req(ue_id, &emm_context->_imsi, emm_context->esm_ctx.esm_proc_data, emm_context->esm_ctx.esm_proc_data->request_type);

    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /*
   * Return the ESM cause value
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
  emm_data_context_t * emm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const activate_default_eps_bearer_context_accept_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                              esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                         ue_id = emm_context->ue_id;

  OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Received Activate Default EPS Bearer Context " "Accept message (ue_id=%d, pti=%d, ebi=%d)\n",
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
  else if (esm_ebr_is_reserved (ebi)){ // || esm_ebr_is_not_in_use (emm_context, ebi)) {
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
   * Execute the default EPS bearer context activation procedure accepted
   * * * * by the UE
   */
  int rc = esm_proc_default_eps_bearer_context_accept (emm_context, ebi, &esm_cause);

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
  emm_data_context_t * emm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const activate_default_eps_bearer_context_reject_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                        ue_id = emm_context->ue_id;

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
   * EPS bearer identity checking
   */
  else if (esm_ebr_is_reserved (ebi)){ // || esm_ebr_is_not_in_use (emm_context, ebi)) {
    /*
     * 3GPP TS 24.301, section 7.3.2, case f
     * * * * Reserved or assigned value that does not match an existing EPS
     * * * * bearer context
     */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)", ebi);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  /*
   * Execute the default EPS bearer context activation procedure not accepted
   * * * * by the UE
   */
  int rc = esm_proc_default_eps_bearer_context_reject (emm_context, ebi, &esm_cause);

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
  emm_data_context_t * emm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const activate_dedicated_eps_bearer_context_accept_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                        ue_id = emm_context->ue_id;

  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);

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
//  else if (esm_ebr_is_reserved (ebi) || !mme_app_get_session_bearer_context_from_all(ue_context, ebi)) {    // todo: check old function esm_ebr_is_not_in_use (emm_context, ebi
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
  int rc = esm_proc_dedicated_eps_bearer_context_accept (emm_context, ebi, &esm_cause);

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
  emm_data_context_t * emm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const activate_dedicated_eps_bearer_context_reject_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                        ue_id = emm_context->ue_id;

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
  else if (esm_ebr_is_reserved (ebi)){ // || esm_ebr_is_not_in_use (emm_context, ebi)) {
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
  int rc = esm_proc_dedicated_eps_bearer_context_reject (emm_context, ebi, &esm_cause);

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
  emm_data_context_t * emm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const modify_eps_bearer_context_accept_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                        ue_id = emm_context->ue_id;

  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);

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
  int rc = esm_proc_modify_eps_bearer_context_accept(emm_context, ebi, &esm_cause);

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
  emm_data_context_t * emm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const modify_eps_bearer_context_reject_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                        ue_id = emm_context->ue_id;

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
  else if (esm_ebr_is_reserved (ebi)){ // || esm_ebr_is_not_in_use (emm_context, ebi)) {
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
  int rc = esm_proc_modify_eps_bearer_context_reject(emm_context, ebi, &esm_cause, true);

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
  emm_data_context_t * emm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const deactivate_eps_bearer_context_accept_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  mme_ue_s1ap_id_t                        ue_id = emm_context->ue_id;
  ue_context_t                           *ue_context  = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);
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
  else if (esm_ebr_is_reserved (ebi)){ // || esm_ebr_is_not_in_use (emm_context, ebi)) {
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
  int pid = esm_proc_eps_bearer_context_deactivate_accept (emm_context, ebi, &esm_cause);
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
    if(!pdn_context->is_active){
      // todo: check that only 1 deactivate message for the default bearer exists
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - We released  the default EBI. Deregistering the PDN context. (ebi=%d,pid=%d)\n", ebi,pid);
      rc = esm_proc_pdn_disconnect_accept (emm_context, pid, ebi, &esm_cause); /**< Delete Session Request is already sent at the beginning. We don't care for the response. */
    }else{
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - We released  the dedicated EBI. Responding with delete bearer response back. (ebi=%d,pid=%d)\n", ebi,pid);
      /** Respond per bearer. */
      nas_itti_dedicated_eps_bearer_deactivation_complete(emm_context->ue_id, ebi);
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

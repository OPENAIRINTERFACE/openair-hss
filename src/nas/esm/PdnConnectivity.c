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
#include "commonDef.h"
#include "esm_proc.h"
#include "esm_data.h"
#include "esm_cause.h"
#include "esm_pt.h"
#include "mme_api.h"
#include "emm_sap.h"
#include "mme_app_apn_selection.h"
#include "mme_app_pdn_context.h"
#include "mme_app_bearer_context.h"

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
/*
   PDN connection handlers
*/
static pdn_cid_t _pdn_connectivity_create (
  emm_data_context_t * emm_context,
  const proc_tid_t pti,
  const pdn_cid_t  pdn_cid,
  const context_identifier_t   context_identifier,
  const_bstring const apn,
  pdn_type_t pdn_type,
  const_bstring const pdn_addr,
  protocol_configuration_options_t * const pco,
  const bool is_emergency,
  pdn_context_t              **pdn_context_pP);

proc_tid_t _pdn_connectivity_delete (emm_data_context_t * emm_context, pdn_cid_t pdn_cid);


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/


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
 **      pti:       Identifies the PDN connectivity procedure  **
 **             requested by the UE                        **
 **      request_type:  Type of the PDN request                    **
 **      pdn_type:  PDN type value (IPv4, IPv6, IPv4v6)        **
 **      apn:       Requested Access Point Name                **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     apn:       Default Access Point Name                  **
 **      pdn_addr:  Assigned IPv4 address and/or IPv6 suffix   **
 **      esm_qos:   EPS bearer level QoS parameters            **
 **      esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    The identifier of the PDN connection if    **
 **             successfully created;                      **
 **             RETURNerror otherwise.                     **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ***************************************************************************/
int
esm_proc_pdn_connectivity_request (
  emm_data_context_t              * emm_context,
  const proc_tid_t             pti,
  const pdn_cid_t              pdn_cid,
  const context_identifier_t   context_identifier,
  const esm_proc_pdn_request_t request_type,
  const_bstring          const apn,
  esm_proc_pdn_type_t          pdn_type,
  const_bstring          const pdn_addr,
  bearer_qos_t             * default_qos,
  protocol_configuration_options_t * const pco,
  esm_cause_t                 *esm_cause,
  pdn_context_t              **pdn_context_pP)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNok;
  mme_ue_s1ap_id_t                        ue_id = emm_context->ue_id;

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - PDN connectivity requested by the UE "
             "(ue_id=" MME_UE_S1AP_ID_FMT ", pti=%d) PDN type = %s, APN = %s pdn addr = %s pdn id %d\n", ue_id, pti,
             (pdn_type == ESM_PDN_TYPE_IPV4) ? "IPv4" : (pdn_type == ESM_PDN_TYPE_IPV6) ? "IPv6" : "IPv4v6",
             (apn) ? (char *)bdata(apn) : "null",
             (pdn_addr) ? (char *)bdata(pdn_addr) : "null", pdn_cid);

  /*
   * Check network IP capabilities
   */
  *esm_cause = ESM_CAUSE_SUCCESS;
  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - _esm_data.conf.features %08x\n", _esm_data.conf.features);

  int is_emergency = (request_type == ESM_PDN_REQUEST_EMERGENCY);

  /*
   * Create new PDN connection
   */
  rc = _pdn_connectivity_create (emm_context, pti, pdn_cid, context_identifier, apn, pdn_type, pdn_addr, pco, is_emergency, pdn_context_pP);

  if (rc < 0) {
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Failed to create PDN connection\n");
    *esm_cause = ESM_CAUSE_INSUFFICIENT_RESOURCES;
  }

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_connectivity_reject()                        **
 **                                                                        **
 ** Description: Performs PDN connectivity procedure not accepted by the   **
 **      network.                                                  **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.1.4                           **
 **      If connectivity with the requested PDN cannot be accepted **
 **      by the network, the MME shall send a PDN CONNECTIVITY RE- **
 **      JECT message to the UE.                                   **
 **                                                                        **
 ** Inputs:  is_standalone: Indicates whether the PDN connectivity     **
 **             procedure was initiated as part of the at- **
 **             tach procedure                             **
 **      ue_id:      UE lower layer identifier                  **
 **      ebi:       Not used                                   **
 **      msg:       Encoded PDN connectivity reject message to **
 **             be sent                                    **
 **      ue_triggered:  Not used                                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
esm_proc_pdn_connectivity_reject (
  bool is_standalone,
  emm_data_context_t * emm_context,
  ebi_t ebi,
  STOLEN_REF bstring *msg,
  bool ue_triggered)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  mme_ue_s1ap_id_t                        ue_id = emm_context->ue_id;

  OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - PDN connectivity not accepted by the " "network (ue_id=" MME_UE_S1AP_ID_FMT ")\n", ue_id);

  if (is_standalone) {
    emm_sap_t                               emm_sap = {0};

    /*
     * Notity EMM that ESM PDU has to be forwarded to lower layers
     */
    emm_sap.primitive = EMMESM_UNITDATA_REQ;
    emm_sap.u.emm_esm.ue_id = ue_id;
    emm_sap.u.emm_esm.ctx = emm_context;
    emm_sap.u.emm_esm.u.data.msg = *msg;
    msg = NULL;
    MSC_LOG_TX_MESSAGE (MSC_NAS_ESM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMESM_UNITDATA_REQ  (PDN CONNECTIVITY REJECT) ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    rc = emm_sap_send (&emm_sap);
  }

  /*
   * If the PDN connectivity procedure initiated as part of the initial
   * * * * attach procedure has failed, an error is returned to notify EMM that
   * * * * the ESM sublayer did not accept UE requested PDN connectivity
   */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
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
int esm_proc_pdn_connectivity_failure (emm_data_context_t * emm_context, pdn_cid_t pdn_cid)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  proc_tid_t                                     pti = ESM_PT_UNASSIGNED;
  mme_ue_s1ap_id_t                               ue_id = emm_context->ue_id;

  OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - PDN connectivity failure (ue_id=" MME_UE_S1AP_ID_FMT ", pdn_cid=%d)\n", ue_id, pdn_cid);
  /*
   * Delete the PDN connection entry
   */
  pti = _pdn_connectivity_delete (emm_context, pdn_cid);

  if (pti != ESM_PT_UNASSIGNED) {
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
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
 **                  is_pdn_connectivity:  Set to true if a PDN            **
 **                  connectivity has to be established.                   **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    RETURNok, RETURNerror                      **
 **                  Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_pdn_config_res(emm_data_context_t * emm_context, pdn_cid_t **pdn_cid, bool ** is_pdn_connectivity, imsi64_t imsi){
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  ue_context_t                           *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);
  int                                     rc = RETURNok;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  ebi_t                                   new_ebi = 0;
  pdn_context_t                          *pdn_context = NULL;
  DevAssert(ue_context);

  // todo: lock UE context

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Processing new subscription data handling from HSS (ue_id=" MME_UE_S1AP_ID_FMT ")\n", ue_context->mme_ue_s1ap_id);

  //----------------------------------------------------------------------------
  // PDN selection here
  // Because NAS knows APN selected by UE if any
  // default APN selection
  // todo: what if no pdn config for the UE requested APN
  struct apn_configuration_s* apn_config = mme_app_select_apn(ue_context, emm_context->esm_ctx.esm_proc_data->apn);

  if (!apn_config) {
    /*
     * Unfortunately we didn't find our default APN...
     * todo: check if any specific procedures exist and abort them!
     */
    OAILOG_INFO (LOG_NAS_ESM, "No suitable APN found ue_id=" MME_UE_S1AP_ID_FMT ")\n",ue_context->mme_ue_s1ap_id);
    return RETURNerror;
  }

  // search for an already set PDN context
  for ((**pdn_cid) = 0; (**pdn_cid) < MAX_APN_PER_UE; (**pdn_cid)++) {

    /** Check if a tunnel already exists depending on the flag. */
    pdn_context_t * pdn_context = mme_app_get_pdn_context(ue_context, apn_config->context_identifier);
    if(pdn_context){
      OAILOG_INFO(LOG_NAS_EMM, "EMMCN-SAP  - " "PDN context was found for UE " MME_UE_S1AP_ID_FMT" already. "
          "(Assuming PDN connectivity is already established before ULA). Will update PDN/UE context information and continue with the accept procedure for id " MME_UE_S1AP_ID_FMT "...\n", msg_pP->ue_id);
      /** Not creating updated bearers. */
      **is_pdn_connectivity = true;
      /** Set the state of the ESM bearer context as ACTIVE (not setting as active if no TAU has followed). */
      rc = esm_ebr_set_status (emm_context, pdn_context->default_ebi, ESM_EBR_ACTIVE, false);
    }
  }
  /*
   * Set the ESM Proc Data values.
   * Update the UE context and PDN context information with it.
   * todo: how to check that this is still our last ESM proc data?
   */
  if(emm_context->esm_ctx.esm_proc_data){
    /*
     * Execute the PDN connectivity procedure requested by the UE
     */
    emm_context->esm_ctx.esm_proc_data->pdn_cid = (**pdn_cid);
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
        emm_context->esm_ctx.esm_proc_data->pdn_cid,
        apn_config->context_identifier,
        emm_context->esm_ctx.esm_proc_data->request_type,
        emm_context->esm_ctx.esm_proc_data->apn,
        emm_context->esm_ctx.esm_proc_data->pdn_type,
        emm_context->esm_ctx.esm_proc_data->pdn_addr,
        &emm_context->esm_ctx.esm_proc_data->bearer_qos,
        (emm_context->esm_ctx.esm_proc_data->pco.num_protocol_or_container_id ) ? &emm_context->esm_ctx.esm_proc_data->pco:NULL,
            &esm_cause,
            &pdn_context);

    // todo: optimize this
    DevAssert(pdn_context);
    if (rc != RETURNerror) {
        /*
         * Create local default EPS bearer context
         */
        if ((!is_pdn_connectivity) || ((is_pdn_connectivity) && (EPS_BEARER_IDENTITY_UNASSIGNED == pdn_context->default_ebi))) {
          rc = esm_proc_default_eps_bearer_context (emm_context, emm_context->esm_ctx.esm_proc_data->pti, (**pdn_cid), &new_ebi, emm_context->esm_ctx.esm_proc_data->bearer_qos.qci, &esm_cause);
        }
        // todo: if the bearer already exist, we may modify the qos parameters with Modify_Bearer_Request!

        if (rc != RETURNerror) {
          esm_cause = ESM_CAUSE_SUCCESS;
        }
      } else {
      }
    }
  //      unlock_ue_contexts(ue_context);
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/


/*
  ---------------------------------------------------------------------------
                PDN connection handlers
  ---------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:        _pdn_connectivity_create()                                **
 **                                                                        **
 ** Description: Creates a new PDN connection entry for the specified UE   **
 **                                                                        **
 ** Inputs:          ue_id:      UE local identifier                        **
 **                  ctx:       UE context                                 **
 **                  pti:       Procedure transaction identity             **
 **                  Pan:       Access Point Name of the PDN connection    **
 **                  pdn_type:  PDN type (IPv4, IPv6, IPv4v6)              **
 **                  pdn_addr:  Network allocated PDN IPv4 or IPv6 address **
 **              is_emergency:  true if the PDN connection has to be esta- **
 **                             blished for emergency bearer services      **
 **                  Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    The identifier of the PDN connection if    **
 **                             successfully created; -1 otherwise.        **
 **                  Others:    _esm_data                                  **
 **                                                                        **
 ***************************************************************************/
static int
_pdn_connectivity_create (
  emm_data_context_t * emm_context,
  const proc_tid_t pti,
  const pdn_cid_t  pdn_cid,
  const context_identifier_t   context_identifier,
  const_bstring const apn,
  pdn_type_t pdn_type,
  const_bstring const pdn_addr,
  protocol_configuration_options_t * const pco,
  const bool is_emergency,
  pdn_context_t              **pdn_context_pP)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  ue_context_t                        *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Create new PDN connection (pti=%d) APN = %s, IP address = %s PDN id %d (ue_id=" MME_UE_S1AP_ID_FMT ")\n",
      pti, bdata(apn),
      (pdn_type == ESM_PDN_TYPE_IPV4) ? esm_data_get_ipv4_addr (pdn_addr) : (pdn_type == ESM_PDN_TYPE_IPV6) ? esm_data_get_ipv6_addr (pdn_addr) : esm_data_get_ipv4v6_addr (pdn_addr),
      pdn_cid, ue_context->mme_ue_s1ap_id);

  pdn_context_t pdn_context_key = {.apn_in_use = apn, .context_identifier = context_identifier}; // todo: check setting apn
  *pdn_context_pP = RB_FIND (PdnContexts, &ue_context->pdn_contexts, &pdn_context_key);

  if (!*pdn_context_pP) {

    /*
     * Create new PDN connection
     */
    *pdn_context_pP = mme_app_create_pdn_context(ue_context, pdn_cid, context_identifier);

    if (*pdn_context_pP) {
      /*
       * Increment the number of PDN connections
       */
      emm_context->esm_ctx.n_pdns += 1;
      /*
       * Set the procedure transaction identity
       */
      (*pdn_context_pP)->esm_data.pti = pti;
      /*
       * Set the emergency bearer services indicator
       */
      (*pdn_context_pP)->esm_data.is_emergency = is_emergency;

      if (pco) {
        if (!(*pdn_context_pP)->pco) {
          (*pdn_context_pP)->pco = calloc(1, sizeof(protocol_configuration_options_t));
        } else {
          clear_protocol_configuration_options((*pdn_context_pP)->pco);
        }
        copy_protocol_configuration_options((*pdn_context_pP)->pco, pco);
      }

      /*
       * Setup the IP address allocated by the network
       */
      (*pdn_context_pP)->pdn_type = pdn_type;
      if (pdn_addr) {
        (*pdn_context_pP)->paa.pdn_type = pdn_type;
        switch (pdn_type) {
        case IPv4:
          IPV4_STR_ADDR_TO_INADDR ((const char *)pdn_addr->data, (*pdn_context_pP)->paa.ipv4_address, "BAD IPv4 ADDRESS FORMAT FOR PAA!\n");
          break;
        case IPv6:
          AssertFatal (1 == inet_pton(AF_INET6, (const char *)pdn_addr->data, &(*pdn_context_pP)->paa.ipv6_address), "BAD IPv6 ADDRESS FORMAT FOR PAA!\n");
          break;
        case IPv4_AND_v6:
          AssertFatal (0, "TODO\n");
          break;
        case IPv4_OR_v6:
          AssertFatal (0, "TODO\n");
          break;
        default:;
        }
      }
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
    }

    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Failed to create new PDN connection (pdn_cid=%d)\n", pdn_cid);
  } else {
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - PDN connection already exist (pdn_cid=%d)\n", pdn_cid);
    // already created
    /*
     * Set the procedure transaction identity and update the pdn context information of the existing pdn context.
     * Will not update the bearers again.
     *
     */
    (*pdn_context_pP)->esm_data.pti = pti;
    (*pdn_context_pP)->esm_data.is_emergency = is_emergency;
    if (pco) {
      if (!(*pdn_context_pP)->pco) {
        (*pdn_context_pP)->pco = calloc(1, sizeof(protocol_configuration_options_t));
      } else {
        clear_protocol_configuration_options((*pdn_context_pP)->pco);
      }
      copy_protocol_configuration_options((*pdn_context_pP)->pco, pco);
    }
    (*pdn_context_pP)->pdn_type = pdn_type;
    if (pdn_addr) {
      (*pdn_context_pP)->paa.pdn_type = pdn_type;
      switch (pdn_type) {
      case IPv4:
        IPV4_STR_ADDR_TO_INADDR ((const char *)pdn_addr->data, (*pdn_context_pP)->paa.ipv4_address, "BAD IPv4 ADDRESS FORMAT FOR PAA!\n");
        break;
      case IPv6:
        AssertFatal (1 == inet_pton(AF_INET6, (const char *)pdn_addr->data, &(*pdn_context_pP)->paa.ipv6_address), "BAD IPv6 ADDRESS FORMAT FOR PAA!\n");
        break;
      case IPv4_AND_v6:
        AssertFatal (0, "TODO\n");
        break;
      case IPv4_OR_v6:
        AssertFatal (0, "TODO\n");
        break;
      default:;
      }
    }
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
}

/****************************************************************************
 **                                                                        **
 ** Name:        _pdn_connectivity_delete()                                **
 **                                                                        **
 ** Description: Deletes PDN connection to the specified UE associated to  **
 **              PDN connection entry with given identifier                **
 **                                                                        **
 ** Inputs:          ue_id:      UE local identifier                        **
 **                  pdn_cid:       Identifier of the PDN connection to be     **
 **                             released                                   **
 **                  Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    The identity of the procedure transaction  **
 **                             assigned to the PDN connection when suc-   **
 **                             cessfully released;                        **
 **                             UNASSIGNED value otherwise.                **
 **                  Others:    _esm_data                                  **
 **                                                                        **
 ***************************************************************************/
proc_tid_t _pdn_connectivity_delete (emm_data_context_t * emm_context, pdn_cid_t pdn_cid)
{
  proc_tid_t                                     pti = ESM_PT_UNASSIGNED;

  if (!emm_context) {
    return pti;
  }
  ue_context_t                        *ue_context   = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);
  pdn_context_t                       *pdn_context  = NULL;
  if (pdn_cid < MAX_APN_PER_UE) {
    /** Get PDN Context. */
    pdn_context = mme_app_get_pdn_context(ue_context, pdn_cid, ESM_EBI_UNDEFINED, NULL);
    if (!pdn_context) {
      OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - PDN connection has not been allocated\n");
    } else if (pdn_context->is_active) {
      OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - PDN connection is active\n");
    } else if (pdn_context->esm_data.n_bearers) {
      OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - PDN connection has beaerer\n");
    } else if (!RB_EMPTY(pdn_context->session_bearers)) {
      OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - PDN connection bearer list is not empty \n");
    } else {
      /*
       * We don't check the PTI. It may be an implicit or a normal detach.
       */
      pti = pdn_context->esm_data.pti;
    }
  } else {
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - PDN connection identifier is not valid\n");
  }
  /*
   * Decrement the number of PDN connections
   */
  emm_context->esm_ctx.n_pdns -= 1;

  /*
   * Remove from the PDN sessions of the UE context.
   */
  // todo: add a lot of locks..
  /** Removed a bearer context from the UE contexts bearer pool and adds it into the PDN sessions bearer pool. */
  pdn_context_t *pdn_context_removed = RB_REMOVE(BearerPool, &ue_context->pdn_contexts, pdn_context);
  if(!pdn_context_removed){
    OAILOG_ERROR(LOG_MME_APP,  "Could not find pdn context with pid %d for ue_id " MME_UE_S1AP_FMT "! \n",
        pdn_context->context_identifier, ue_context->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /*
   * Release allocated PDN connection data
   */
  mme_app_free_pdn_context(&pdn_context); /**< Frees it by putting it back to the pool. */

  OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - PDN connection %d released\n", pdn_cid);

  /*
   * Return the procedure transaction identity
   */
  return pti;
}

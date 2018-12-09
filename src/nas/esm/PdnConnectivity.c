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
  int esm_cause)
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
nas_esm_pdn_connectivity_proc_t *_esm_proc_create_pdn_connectivity_procedure(mme_ue_s1ap_id_t ue_id, imsi_t *imsi, pti_t pti)
{
  // todo: eventually setting apn..
  nas_esm_pdn_connectivity_proc_t  *pdn_connectivity_proc = nas_new_pdn_connectivity_procedure(ue_id, pti);
  AssertFatal(pdn_connectivity_proc, "TODO Handle this");
  memcpy((void*)&pdn_connectivity_proc->imsi, imsi, sizeof(imsi_t));
  pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.failure_notif = _esm_proc_pdn_connectivity_fail;
  /** Timeout notifier set separately. */
  return pdn_connectivity_proc;
}

//-----------------------------------------------------------------------------
void _esm_proc_free_pdn_connectivity_procedure(nas_esm_pdn_connectivity_proc_t ** nas_pdn_connectivity_proc){
  // todo: remove from the hashmap
  // free content
  void *unused = NULL;
  /** Forget the name of the timer.. only one can exist (also used for activate default EPS bearer context.. */
  // todo: remove activate default eps bearer..
  nas_stop_esm_timer((*nas_pdn_connectivity_proc)->trx_base_proc.esm_proc.base_proc.ue_id,
      &(*nas_pdn_connectivity_proc)->trx_base_proc.esm_proc_timer);

  if((*nas_pdn_connectivity_proc)->apn_subscribed)
    bdestroy_wrapper(&(*nas_pdn_connectivity_proc)->apn_subscribed);

  /** Delete the ESM context. */
  free_wrapper((void **)nas_pdn_connectivity_proc);
}

//-----------------------------------------------------------------------------
nas_esm_bearer_context_proc_t *_esm_proc_create_bearer_context_procedure(mme_ue_s1ap_id_t ue_id, imsi_t *imsi, pti_t pti, ebi_t ebi)
{
  // todo: eventually setting apn..
  nas_esm_bearer_context_proc_t  *esm_bearer_context_proc = nas_new_bearer_context_proc(ue_id, pti, ebi);
  AssertFatal(esm_bearer_context_proc, "TODO Handle this");
  memcpy((void*)&esm_bearer_context_proc->imsi, imsi, sizeof(imsi_t));
  esm_bearer_context_proc->trx_base_proc.esm_proc.base_proc.failure_notif = _esm_proc_bearer_context_fail;
  /*
   * Timeout notifier set separately.
   * This bearer context procedure may be transactional or not.
   */
  return esm_bearer_context_proc;
}

// todo: LOCKS needed for setting/removing a procedure?!

//-----------------------------------------------------------------------------
void _esm_proc_free_bearer_context_procedure(nas_esm_bearer_context_proc_t * esm_bearer_context_proc){
  // todo: remove from the hashmap
  // free content
  void *unused = NULL;
  /** Forget the name of the timer.. only one can exist (also used for activate default EPS bearer context.. */
  // todo: remove activate default eps bearer..
  nas_stop_esm_timer(esm_bearer_context_proc->trx_base_proc.esm_proc.base_proc.ue_id,
      &esm_bearer_context_proc->trx_base_proc.esm_proc_timer);

  /** Delete the ESM context. */
  free_wrapper((void **) &esm_bearer_context_proc);
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

int
esm_proc_pdn_connectivity_request (
  mme_ue_s1ap_id_t             ue_id,
  imsi_t                      *imsi,
  tai_t                       *visited_tai,
  const proc_tid_t             pti,
  const apn_configuration_t   *apn_configuration,
  const_bstring                const apn_subscribed,
  const esm_proc_pdn_request_t request_type,
  esm_proc_pdn_type_t          pdn_type)
{
  int                          rc = RETURNok;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - PDN connectivity requested by the UE "
             "(ue_id=" MME_UE_S1AP_ID_FMT ", pti=%d) PDN type = %s, APN = %s context_identifier %d\n", ue_id, pti,
             (pdn_type == ESM_PDN_TYPE_IPV4) ? "IPv4" : (pdn_type == ESM_PDN_TYPE_IPV6) ? "IPv6" : "IPv4v6",
             (char *)bdata(apn), context_identifier);
  /*
   * Check network IP capabilities.
   */
  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - _esm_data.conf.features %08x\n", _esm_data.conf.features);
  /*
   * Create new PDN context in the MME_APP UE context.
   * This will also allocate a bearer context (NULL bearer set).
   * todo: Use locks on the MME_APP UE Context and consider the case of error/implicit detach..
   */
  pdn_context_t * pdn_context = NULL;
  rc = mme_app_esm_create_pdn_context(ue_id, pti, apn_configuration, apn_subscribed, apn_configuration.context_identifier, apn_subscribed, &pdn_context);
  if (rc < 0) {
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - Failed to create PDN connection for UE " MME_UE_S1AP_ID_FMT ", apn = \"%s\" (pti=%d).\n", ue_id, (char *)bdata(apn_subscribed), pti);
    /** No empty/garbage contexts. */
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
  }
  /*
   * Inform the MME_APP procedure to establish a session in the SAE-GW.
   */
  rc = mme_app_send_s11_create_session_req(ue_id, imsi, pdn_context, visited_tai, false);
  DevAssert(rc == RETURNok);
  /** No message returned and no timeout set for the ESM procedure. */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
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
esm_cause_t esm_proc_pdn_connectivity_res (mme_ue_s1ap_id_t ue_id, ambr_t * apn_ambr, bearer_qos_t * bearer_level_qos, nas_esm_pdn_connectivity_proc_t * esm_pdn_connectivity_proc) {
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  /*
   * Update default EPS bearer context and pdn context.
   */
  // todo: PAA!!!
  rc = mme_app_esm_update_pdn_context(ue_id, esm_pdn_connectivity_proc->apn_subscribed, esm_pdn_connectivity_proc->pdn_cid, esm_pdn_connectivity_proc->default_ebi,
      ESM_EBR_ACTIVE_PENDING, apn_ambr, bearer_level_qos, (protocol_configuration_options_t*)NULL);
  if (rc == RETURNerror) {
    OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Could not update the default EPS bearer context activation (ue_id=" MME_UE_S1AP_ID_FMT ", context_identifier=%d,  QCI %u). \n. ",
        ue_id, esm_pdn_connectivity_proc->pdn_cid, bearer_qos->qci);
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
esm_cause_t esm_proc_pdn_connectivity_failure (mme_ue_s1ap_id_t ue_id, nas_esm_pdn_connectivity_proc_t * esm_pdn_connectivity_proc)
{
  proc_tid_t                                     pti = ESM_PT_UNASSIGNED;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - PDN connectivity failure (ue_id=" MME_UE_S1AP_ID_FMT ", pdn_cid=%d, ebi%d)\n", ue_id, pdn_cid, default_ebi);
  /*
   * Delete the PDN connectivity in the MME_APP UE context.
   */
  mme_app_esm_delete_pdn_context(ue_id, esm_pdn_connectivity_proc->apn_subscribed, esm_pdn_connectivity_proc->pdn_cid, esm_pdn_connectivity_proc->default_ebi); /**< Frees it by putting it back to the pool. */

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - PDN connection for APN \"%s\" (cid=%d,ebi=%d) released for UE: " MME_UE_S1AP_ID_FMT ".\n", bdata(apn), default_ebi, pdn_cid, ue_id);

  /*
   * Remove the ESM procedure and stop the timer.
   */
  _esm_proc_free_pdn_connectivity_procedure(&esm_pdn_connectivity_proc);

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
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

esm_cause_t esm_proc_pdn_config_res(mme_ue_s1ap_id_t ue_id, nas_esm_pdn_connectivity_proc_t *esm_pdn_connectivity_proc, imsi64_t imsi){
  ebi_t                                   new_ebi = 0;
  pdn_context_t                          *pdn_context = NULL;
  bearer_context_t                       *bearer_context = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  /** We reject at the beginning if no APN has been transmitted at initial attach procedure. */
  struct apn_configuration_s* apn_config = NULL;
  rc = mme_app_select_apn(imsi, esm_pdn_connectivity_proc->apn_subscribed, &apn_config);
  if(rc == RETURNerror){
    DevAssert(esm_pdn_connectivity_proc->apn_subscribed);
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - No APN configuration could be found for APN \"%s\". "
        "Rejecting the PDN connectivity procedure." "(ue_id=%d, pti=%d)\n", bdata(esm_pdn_connectivity_proc->apn_subscribed), mme_ue_s1ap_id, pti);
    _esm_proc_free_pdn_connectivity_procedure(&esm_pdn_connectivity_proc);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_UNKNOWN_ACCESS_POINT_NAME);
  }
  /*
   * Execute the PDN connectivity procedure requested by the UE.
   * Allocate a new PDN context and trigger an S11 Create Session Request.
   */
  rc = esm_proc_pdn_connectivity_request (ue_id, &esm_pdn_connectivity_proc->imsi, &esm_pdn_connectivity_proc->visited_tai,
      esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.pti, apn_config,
      esm_pdn_connectivity_proc->request_type, esm_pdn_connectivity_proc->apn_subscribed, esm_pdn_connectivity_proc->pdn_type);
  //      (esm_context->esm_proc_data->pco.num_protocol_or_container_id ) ? &esm_context->esm_proc_data->pco:NULL,
  if(rc == RETURNerror){
    _esm_proc_free_pdn_connectivity_procedure(&esm_pdn_connectivity_proc);
    /** No PDN Context is expected. */
   OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }
  /** The procedure will stay. */
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, ESM_CAUSE_SUCCESS);
}

/****************************************************************************
 **                                                                        **
 ** Name:        esm_proc_pdn_config_fail()                                **
 **                                                                        **
 ** Description: Performs PDN config failure processing. Rejects the       **
 **              requested PDN Connectivity from the UE.                   **
 **                                                                        **
 **         Inputs:  ue_id:      UE local identifier                       **
 **                  connectivity has to be established.                   **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    RETURNok, RETURNerror                      **
 **                  Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t esm_proc_pdn_config_fail(mme_ue_s1ap_id_t ue_id, nas_esm_pdn_connectivity_proc_t *esm_pdn_connectivity_proc, ESM_msg * esm_resp_msg){
  ebi_t                                   new_ebi = 0;
  pdn_context_t                          *pdn_context = NULL;
  bearer_context_t                       *bearer_context = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  /*
   * Check if the procedure is transactional.
   */
  if(esm_pdn_connectivity_proc->trx_base_proc.esm_proc.type == ESM_PROC_EPS_BEARER_CONTEXT){
    /* Ignore it. */
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - The ESM procedure is non-transactional for UE: " MME_UE_S1AP_ID_FMT ". Ignoring the received PDN configuration failure. \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  }
  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Freed ESM transactional procedure (pti=%d) for PDN connection for APN \"%s\" (cid=%d,ebi=%d) (failed ULA) for UE: " MME_UE_S1AP_ID_FMT ".\n",
      esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.pti, bdata(apn), pdn_cid, default_ebi, ue_id);
  _esm_proc_free_pdn_connectivity_procedure(&esm_pdn_connectivity_proc);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_UNKNOWN_ACCESS_POINT_NAME);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

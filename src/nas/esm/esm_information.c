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

/*! \file esm_information.c
   \brief
   \author  Lionel GAUTHIER
   \date 2017
   \email: lionel.gauthier@eurecom.fr
*/

#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bstrlib.h"
#include "dynamic_memory_check.h"
#include "log.h"

#include "3gpp_24.007.h"
#include "3gpp_24.301.h"
#include "3gpp_36.401.h"
#include "common_types.h"
#include "emm_sap.h"
#include "nas_timer.h"
#include "esm_msg.h"
#include "EsmInformationRequest.h"
#include "EsmInformationResponse.h"
#include "PdnConnectivityRequest.h"
#include "mme_app_ue_context.h"
#include "esm_proc.h"
#include "mme_app_esm_procedures.h"
#include "nas_esm_proc.h"
#include "esm_message.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

static void
_esm_information_t3489_handler(
  nas_esm_proc_t * esm_base_proc, ESM_msg *esm_rsp_msg);

/* Maximum value of the deactivate EPS bearer context request
   retransmission counter */
#define ESM_INFORMATION_COUNTER_MAX   3

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
   Functions executed by the MME to send ESM messages
   --------------------------------------------------------------------------
*/
static void esm_send_esm_information_request (pti_t pti, ebi_t ebi, ESM_msg * esm_req_msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  memset((void*)esm_req_msg, 0, sizeof(ESM_msg));
  /*
   * Mandatory - ESM message header
   */
  esm_req_msg->esm_information_request.protocoldiscriminator        = EPS_SESSION_MANAGEMENT_MESSAGE;
  esm_req_msg->esm_information_request.epsbeareridentity            = ebi;
  esm_req_msg->esm_information_request.messagetype                  = ESM_INFORMATION_REQUEST;
  esm_req_msg->esm_information_request.proceduretransactionidentity = pti;
  OAILOG_NOTICE (LOG_NAS_ESM, "ESM-SAP   - Send ESM_INFORMATION_REQUEST message (pti=%d, ebi=%d)\n", esm_req_msg->esm_information_request.proceduretransactionidentity, esm_req_msg->esm_information_request.epsbeareridentity);
  OAILOG_FUNC_OUT(LOG_NAS_ESM);
}

//------------------------------------------------------------------------------
void esm_proc_esm_information_request (nas_esm_proc_pdn_connectivity_t *esm_proc_pdn_connectivity, ESM_msg * esm_result_msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  /*
   * Send ESM information request message and
   * start timer T3489
   */
  esm_send_esm_information_request (esm_proc_pdn_connectivity->esm_base_proc.pti, EPS_BEARER_IDENTITY_UNASSIGNED, &esm_result_msg);

  nas_stop_esm_timer(esm_proc_pdn_connectivity->esm_base_proc.ue_id, &esm_proc_pdn_connectivity->esm_base_proc.esm_proc_timer);
  /*
   * Start T3489 timer
   */
  esm_proc_pdn_connectivity->esm_base_proc.esm_proc_timer.id = nas_esm_timer_start (mme_config.nas_config.t3489_sec, 0 /*usec*/, esm_proc_pdn_connectivity->esm_base_proc.ue_id);
  /**< Address field should be big enough to save an ID. */

  esm_proc_pdn_connectivity->esm_base_proc.timeout_notif = _esm_information_t3489_handler;
  OAILOG_FUNC_OUT(LOG_NAS_ESM);
}

//------------------------------------------------------------------------------
void esm_proc_esm_information_response (mme_ue_s1ap_id_t ue_id, pti_t pti, nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity, const esm_information_response_msg * const esm_information_resp)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  /*
   * Stop T3489 timer if running
   */
  nas_stop_esm_timer(ue_id, &esm_proc_pdn_connectivity->esm_base_proc.esm_proc_timer);

  /** Process the result. */
  if (esm_information_resp->presencemask & PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT) {
    if(esm_proc_pdn_connectivity->subscribed_apn)
      bdestroy_wrapper(&esm_proc_pdn_connectivity->subscribed_apn);
    esm_proc_pdn_connectivity->subscribed_apn = bstrcpy(esm_information_resp->accesspointname);
  } else{
    /** No APN Name received from UE. We will used the default one from the subscription data. */
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - No APN name received in the ESM information response " "(ue_id=%d, pti=%d).\n", ue_id, pti);
  }

  if(!esm_proc_pdn_connectivity->subscribed_apn){
    /** No APN Name received from UE. We will used the default one from the subscription data. */
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - No APN name received from UE to establish PDN connection and subscription exists. "
        "Will attach to the default APN in the subscription information. " "(ue_id=%d, pti=%d)\n", ue_id, pti);
  }

//  if ((pco) && (pco->num_protocol_or_container_id)) {
//    if (esm_context->esm_proc_data->pco.num_protocol_or_container_id) {
//      clear_protocol_configuration_options(&esm_context->esm_proc_data->pco);
//    }
//    copy_protocol_configuration_options(&esm_context->esm_proc_data->pco, pco);
//  }

  OAILOG_FUNC_OUT(LOG_NAS_ESM);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/

static void
_esm_information_t3489_handler(nas_esm_proc_t * esm_base_proc, ESM_msg *esm_rsp_msg) {
  OAILOG_FUNC_IN(LOG_NAS_ESM);

  nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity = (nas_esm_proc_pdn_connectivity_t*)esm_base_proc;
  esm_base_proc->retx_count += 1;
  if (esm_base_proc->retx_count < ESM_INFORMATION_COUNTER_MAX) {
    /*
     * Create a new ESM-Information request and restart the timer.
     * Keep the ESM transaction.
     */
    esm_proc_esm_information_request(esm_proc_pdn_connectivity, esm_rsp_msg);
    OAILOG_FUNC_OUT(LOG_NAS_ESM);
  } else {
    /*
     * No PDN context is expected.
     * Prepare the reject message.
     */
    esm_send_pdn_connectivity_reject(esm_base_proc->pti, esm_rsp_msg, ESM_CAUSE_ESM_INFORMATION_NOT_RECEIVED);
    /*
     * Release the transactional PDN connectivity procedure.
     */
    _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
    OAILOG_FUNC_OUT(LOG_NAS_ESM);
  }
}

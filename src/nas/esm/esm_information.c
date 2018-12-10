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
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"
#include "log.h"
#include "dynamic_memory_check.h"
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "mme_app_ue_context.h"
#include "common_defs.h"
#include "mme_config.h"
#include "mme_app_defs.h"
#include "nas_itti_messaging.h"
#include "emm_data.h"
#include "emm_sap.h"
#include "esm_ebr.h"
#include "esm_proc.h"
#include "esm_cause.h"
#include "esm_sap.h"
#include "esm_data.h"
#include "sap/esm_message.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

static int
_esm_information (
  nas_esm_proc_pdn_connectivity_t * const esm_proc_pdn_connectivity);

static int
_esm_information_t3489_handler(
  nas_esm_proc_pdn_connectivity_t * esm_pdn_connectivity_proc, ESM_msg *esm_resp_msg);

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
int esm_send_esm_information_request (pti_t pti, ebi_t ebi, esm_information_request_msg * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  /*
   * Mandatory - ESM message header
   */
  msg->protocoldiscriminator        = EPS_SESSION_MANAGEMENT_MESSAGE;
  msg->epsbeareridentity            = ebi;
  msg->messagetype                  = ESM_INFORMATION_REQUEST;
  msg->proceduretransactionidentity = pti;
  OAILOG_NOTICE (LOG_NAS_ESM, "ESM-SAP   - Send ESM_INFORMATION_REQUEST message (pti=%d, ebi=%d)\n", msg->proceduretransactionidentity, msg->epsbeareridentity);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
}

//------------------------------------------------------------------------------
int esm_proc_esm_information_request (nas_esm_proc_pdn_connectivity_t *esm_proc_pdn_connectivity, ESM_msg * esm_result_msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  /*
   * Send ESM information request message and
   * start timer T3489
   */
  int rc = esm_send_esm_information_request (esm_proc_pdn_connectivity->esm_base_proc.pti, EPS_BEARER_IDENTITY_UNASSIGNED, &esm_result_msg);

  if (rc != RETURNerror) {
    rc = _esm_information (esm_proc_pdn_connectivity);
  }
  esm_proc_pdn_connectivity->esm_base_proc.timeout_notif = _esm_information_t3489_handler;
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}


//------------------------------------------------------------------------------
int esm_proc_esm_information_response (mme_ue_s1ap_id_t ue_id, pti_t pti, nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity, esm_information_response_msg * esm_information_resp)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNok;

  /*
   * Stop T3489 timer if running
   */
  nas_stop_T3489(ue_id, &esm_proc_pdn_connectivity->esm_base_proc.esm_proc_timer);

  /** Process the result. */
  if (esm_information_resp->presencemask & PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT) {
    if(esm_proc_pdn_connectivity->subscribed_apn)
      bdestroy_wrapper(&esm_proc_pdn_connectivity->subscribed_apn);
    esm_proc_pdn_connectivity->apn_subscribed = bstrcpy(esm_proc_pdn_connectivity->apn_subscribed);
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

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/

/****************************************************************************
 **                                                                        **
 ** Name:    _esm_information()                                  **
 **                                                                        **
 ** Description: Sends DEACTIVATE EPS BEREAR CONTEXT REQUEST message and   **
 **      starts timer T3489                                        **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **      timer:       NAS timer                                     **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3489                                      **
 **                                                                        **
 ***************************************************************************/
static int
_esm_information (
  nas_esm_proc_pdn_connectivity_t * const esm_proc_pdn_connectivity)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  emm_sap_t                               emm_sap = {0};
  int                                     rc;
  mme_ue_s1ap_id_t                        ue_id = esm_proc_pdn_connectivity->esm_base_proc.ue_id;

  nas_stop_esm_timer(ue_id, &esm_proc_pdn_connectivity->esm_base_proc.esm_proc_timer);
  /*
   * Start T3489 timer
   */
  esm_proc_pdn_connectivity->esm_base_proc.esm_proc_timer.id = nas_timer_start (esm_proc_pdn_connectivity->esm_base_proc.esm_proc_timer.sec, 0 /*usec*/, TASK_NAS_ESM, _nas_proc_pdn_connectivity_timeout_handler, ue_id); /**< Address field should be big enough to save an ID. */
  MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3489 started UE " MME_UE_S1AP_ID_FMT " ", ue_id);

  OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "Timer T3489 (%lx) expires in %ld seconds\n",
      ue_id, nas_timer->id, nas_timer->sec);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

static int
_esm_information_t3489_handler(nas_esm_proc_pdn_connectivity_t * esm_pdn_connectivity_proc, ESM_msg *esm_resp_msg) {
  int                         rc = RETURNok;

  OAILOG_FUNC_IN(LOG_NAS_ESM);

  esm_pdn_connectivity_proc->esm_base_proc.retx_count += 1;
  if (esm_pdn_connectivity_proc->esm_base_proc.retx_count < ESM_INFORMATION_COUNTER_MAX) {
    /*
     * Create a new ESM-Information request and restart the timer.
     * Keep the ESM transaction.
     */
    rc = esm_send_esm_information_request (esm_pdn_connectivity_proc->esm_base_proc.pti, esm_pdn_connectivity_proc->default_ebi, &esm_resp_msg);

    if (rc != RETURNerror) {
      rc = _esm_information (esm_pdn_connectivity_proc);
    }

    OAILOG_FUNC_RETURN(LOG_NAS_ESM, rc);
  } else {
    /*
     * No PDN context is expected.
     * Prepare the reject message.
     */
    esm_send_pdn_connectivity_reject(esm_pdn_connectivity_proc->esm_base_proc.pti, esm_resp_msg, ESM_CAUSE_ESM_INFORMATION_NOT_RECEIVED);
    /*
     * Release the transactional PDN connectivity procedure.
     */
    _esm_proc_free_pdn_connectivity_procedure(&esm_pdn_connectivity_proc);
    OAILOG_FUNC_RETURN(LOG_NAS_ESM, RETURNok);
  }
}

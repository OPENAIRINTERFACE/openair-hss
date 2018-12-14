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
  Source      DefaultEpsBearerContextActivation.c

  Version     0.1

  Date        2013/01/28

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Frederic Maurel

  Description Defines the default EPS bearer context activation ESM
        procedure executed by the Non-Access Stratum.

        The purpose of the default bearer context activation procedure
        is to establish a default EPS bearer context between the UE
        and the EPC.

        The procedure is initiated by the network as a response to
        the PDN CONNECTIVITY REQUEST message received from the UE.
        It can be part of the attach procedure.

*****************************************************************************/
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
#include "emm_data.h"
#include "emm_sap.h"
#include "esm_proc.h"
#include "esm_ebr.h"
#include "esm_cause.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/


/*
   --------------------------------------------------------------------------
   Internal data handled by the default EPS bearer context activation
   procedure in the MME
   --------------------------------------------------------------------------
*/

/* Maximum value of the deactivate EPS bearer context request
   retransmission counter */
#define DEFAULT_EPS_BEARER_ACTIVATE_COUNTER_MAX   3

static int
_default_eps_bearer_activate_t3485_handler(nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity, ESM_msg *esm_resp_msg);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/


/*
   --------------------------------------------------------------------------
                Timer handlers
   --------------------------------------------------------------------------
*/

/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_activate_default_eps_bearer_context_request()    **
 **                                                                        **
 ** Description: Builds Activate Default EPS Bearer Context Request        **
 **      message                                                   **
 **                                                                        **
 **      The activate default EPS bearer context request message   **
 **      is sent by the network to the UE to request activation of **
 **      a default EPS bearer context.                             **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      qos:       Subscribed EPS quality of service          **
 **      apn:       Access Point Name in used                  **
 **      pdn_addr:  PDN IPv4 address and/or IPv6 suffix        **
 **      esm_cause: ESM cause code                             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void
esm_send_activate_default_eps_bearer_context_request (
  nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity,
  ESM_msg * esm_resp_msg,
  pdn_context_t * pdn_context,
  bearer_context_t * bearer_context)
{
  /** Get the PDN context. */

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  memset(esm_resp_msg, 0, sizeof(ESM_msg));
  /*
   * Mandatory - ESM message header
   */
  esm_resp_msg->activate_default_eps_bearer_context_request.protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  esm_resp_msg->activate_default_eps_bearer_context_request.epsbeareridentity = esm_proc_pdn_connectivity->default_ebi;
  esm_resp_msg->activate_default_eps_bearer_context_request.messagetype = ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST;
  esm_resp_msg->activate_default_eps_bearer_context_request.proceduretransactionidentity = esm_proc_pdn_connectivity->esm_base_proc.pti;
  /*
   * Mandatory - EPS QoS
   */
  esm_resp_msg->activate_default_eps_bearer_context_request.epsqos.qci = *bearer_context->qci;
  /*
   * Mandatory - Access Point Name
   * No need to copy, because directly encoded.
   */
  esm_resp_msg->activate_default_eps_bearer_context_request.accesspointname = esm_proc_pdn_connectivity->subscribed_apn;
  /*
   * Mandatory - PDN address
   */
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - pdn_type is %u\n", pdn_type);
  esm_resp_msg->activate_default_eps_bearer_context_request.pdnaddress.pdntypevalue = esm_proc_pdn_connectivity->pdn_type;
  esm_resp_msg->activate_default_eps_bearer_context_request.pdnaddress.pdnaddressinformation = paa_to_bstring(pdn_context->paa);
  /*
   * Optional - ESM cause code
   */
  esm_resp_msg->activate_default_eps_bearer_context_request.presencemask = 0;

//  if (esm_cause != ESM_CAUSE_SUCCESS) {
    // todo: UE requested IP address
//    msg->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_PRESENT;
//    msg->esmcause = esm_cause;
//  }

//  if (pco) {
//    msg->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
//    /**
//     * The PCOs actually received by the SAE-GW.
//     * todo: After handover, the received PCOs might have been changed (like DNS address), we need to check them and inform the UE.
//     * todo: We also need to be able to ask the SAE-GW for specific PCOs?
//     * In handover, any change in the ESM context information has to be forwarded to the UE --> MODIFY EPS BEARER CONTEXT REQUEST!! (probably not possible with TAU accept).
//     */
//    copy_protocol_configuration_options(&msg->protocolconfigurationoptions, pco);
//  }

  /** Implementing subscribed values. */
  esm_resp_msg->activate_default_eps_bearer_context_request.presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT;
  ambr_kbps_calc(&esm_resp_msg->activate_default_eps_bearer_context_request.apnambr, (pdn_context->subscribed_apn_ambr.br_dl/1000), (pdn_context->subscribed_apn_ambr.br_ul/1000));
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Send Activate Default EPS Bearer Context " "Request message (pti=%d, ebi=%d)\n",
      esm_resp_msg->activate_default_eps_bearer_context_request.proceduretransactionidentity, esm_resp_msg->activate_default_eps_bearer_context_request.epsbeareridentity);
  OAILOG_FUNC_OUT (LOG_NAS_ESM);
}

/*
   --------------------------------------------------------------------------
      Default EPS bearer context activation procedure executed by the MME
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_default_eps_bearer_context()                     **
 **                                                                        **
 ** Description: Allocates resources required for activation of a default  **
 **      EPS bearer context.                                       **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                           **
 **          proc:       PDN connectivity procedure                    **
 **          bearer_qos: Received authorized bearer qos from the PCRF  **
 **          apn_ambr:   Received authorized apn-ambr from the PCRF    **
 **                                                                        **
 ** Outputs: rsp:        ESM response message                       **
 **      esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_proc_default_eps_bearer_context (
  mme_ue_s1ap_id_t   ue_id,
  const nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity)
{

  int                rc = RETURNok;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Default EPS bearer context activation (ue_id=" MME_UE_S1AP_ID_FMT ", context_identifier=%d,  QCI %u)\n",ue_id, esm_proc_pdn_connectivity->pdn_cid, bearer_qos->qci);
  REQUIREMENT_3GPP_24_301(R10_6_4_1_2);
  if(esm_proc_pdn_connectivity->is_attach){
    /** Not starting the T3485 timer. */
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  }else{
    OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Starting T3485 for Default EPS bearer context activation (ue_id=" MME_UE_S1AP_ID_FMT ", context_identifier=%d,  QCI %u)\n",
        ue_id, esm_proc_pdn_connectivity->pdn_cid, bearer_qos->qci);
    /** Stop any timer if running. */
    nas_stop_esm_timer(ue_id, &esm_proc_pdn_connectivity->esm_base_proc.esm_proc_timer);
    /** Start the T3485 timer for additional PDN connectivity. */
    esm_proc_pdn_connectivity->esm_base_proc.esm_proc_timer.id = nas_timer_start (esm_proc_pdn_connectivity->esm_base_proc.esm_proc_timer.sec, 0 /*usec*/, false,
        _nas_proc_pdn_connectivity_timeout_handler, ue_id); /**< Address field should be big enough to save an ID. */
    esm_proc_pdn_connectivity->esm_base_proc.timeout_notif = _default_eps_bearer_activate_t3485_handler;
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
  }
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_default_eps_bearer_context_accept()              **
 **                                                                        **
 ** Description: Performs default EPS bearer context activation procedure  **
 **      accepted by the UE.                                       **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.1.3                           **
 **      Upon receipt of the ACTIVATE DEFAULT EPS BEARER CONTEXT   **
 **      ACCEPT message, the MME shall enter the state BEARER CON- **
 **      TEXT ACTIVE and stop the timer T3485, if it is running.   **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **      esm_prod :      ESM PDN connectivity procedure             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void
esm_proc_default_eps_bearer_context_accept (mme_ue_s1ap_id_t ue_id,
    nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity){

  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc;
  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Default EPS bearer context activation "
      "accepted by the UE (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d)\n", ue_id, ebi);
  /*
   * Update the ESM bearer context state of the default bearer as ACTIVE.
   * The CN state & FTEID will be set by the MME_APP layer in independently of this.
   */
  rc = mme_app_esm_update_pdn_context(ue_id, esm_proc_pdn_connectivity->subscribed_apn, esm_proc_pdn_connectivity->pdn_cid, esm_proc_pdn_connectivity->default_ebi, ESM_EBR_ACTIVE,
      NULL, NULL, NULL);
  /** No need to inform the anything else. */
  OAILOG_FUNC_OUT(LOG_NAS_ESM, rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/
/****************************************************************************
 **                                                                        **
 ** Name:    _default_eps_bearer_activate_t3485_handler()              **
 **                                                                        **
 ** Description: T3485 timeout handler                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 6.4.1.6, case a                   **
 **      On the first expiry of the timer T3485, the MME shall re- **
 **      send the ACTIVATE DEFAULT EPS BEARER CONTEXT REQUEST and  **
 **      shall reset and restart timer T3485. This retransmission  **
 **      is repeated four times, i.e. on the fifth expiry of timer **
 **      T3485, the MME shall release possibly allocated resources **
 **      for this activation and shall abort the procedure.        **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/

static int
_default_eps_bearer_activate_t3485_handler(nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity, ESM_msg *esm_resp_msg) {
  int                         rc = RETURNok;

  OAILOG_FUNC_IN(LOG_NAS_ESM);

  esm_proc_pdn_connectivity->esm_base_proc.retx_count += 1;
  if (esm_proc_pdn_connectivity->esm_base_proc.retx_count < DEFAULT_EPS_BEARER_ACTIVATE_COUNTER_MAX) {
    /*
     * Create a new ESM-Information request and restart the timer.
     * Keep the ESM transaction.
     */

    pdn_context_t * pdn_context = NULL;
    mme_app_get_pdn_context(esm_proc_pdn_connectivity->esm_base_proc.ue_id, esm_proc_pdn_connectivity->pdn_cid,
        esm_proc_pdn_connectivity->default_ebi, esm_proc_pdn_connectivity->subscribed_apn, &pdn_context);
    if(pdn_context){
      bearer_context_t * bearer_context = NULL;
      mme_app_get_session_bearer_context(pdn_context, esm_proc_pdn_connectivity->default_ebi);
      if(bearer_context){
        rc = esm_send_activate_default_eps_bearer_context_request(esm_proc_pdn_connectivity->esm_base_proc.ue_id, pdn_context, bearer_context);
        if (rc != RETURNerror) {
          rc = esm_proc_default_eps_bearer_context (esm_proc_pdn_connectivity->esm_base_proc.ue_id, esm_proc_pdn_connectivity);
          OAILOG_FUNC_RETURN(LOG_NAS_ESM, rc);
        }
      }
    }
  }
  /*
   * No PDN context is expected.
   * Prepare the reject message.
   */
  esm_send_pdn_connectivity_reject(esm_proc_pdn_connectivity->esm_base_proc.pti, esm_resp_msg, ESM_CAUSE_ESM_INFORMATION_NOT_RECEIVED);
  /*
   * Release the transactional PDN connectivity procedure.
   */
  _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
  OAILOG_FUNC_RETURN(LOG_NAS_ESM, RETURNok);
}

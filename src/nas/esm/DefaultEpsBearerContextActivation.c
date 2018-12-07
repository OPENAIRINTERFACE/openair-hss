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
#include "esm_ebr_context.h"
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
void _default_eps_bearer_activate_t3485_handler (nas_esm_pdn_connectivity_proc_t * esm_pdn_conn_procedure, ESM_msg * esm_resp_msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc;

  ////      rc = esm_proc_default_eps_bearer_context(mme_ue_s1ap_id, esm_pdn_connectivity_proc,
  ////          pdn_context_duplicate->
  ////          &msg->data.pdn_connectivity_res->bearer_qos, msg->data.pdn_connectivity_res->apn_ambr, &esm_resp_msg, &esm_cause);
  //  if(rc == RETURNerror){
  //
  //    if(*esm_cause == ESM_CAUSE_SUCCESS){
  //      /** Ignore the received message. */
  //    }else{
  //        // todo: rc = esm_pdn_connectivity_reject()
  //        // todo: esm_pdn_connectivity_reject_send;
  //      }
  //      // todo: remove the transaction..
  //    }
  //    // todo: start the timer, only if the nas procedure is not initial..
  //    esm_send_activate_default_eps_bearer_context_request();
  //
  //
  //  esm_activate_defesm_atproc_activate_pdn_connectivity_accept();
  //  _esm_send_activate_default_bearer_context();
  //  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);



  /*
   * Get retransmission timer parameters data
   */
  esm_ebr_timer_data_t                   *esm_ebr_timer_data = (esm_ebr_timer_data_t *) (args);

  if (esm_ebr_timer_data) {
    /*
     * Increment the retransmission counter
     */
    esm_ebr_timer_data->count += 1;
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - T3485 timer expired (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d), " "retransmission counter = %d\n",
        esm_ebr_timer_data->ue_id, esm_ebr_timer_data->ebi, esm_ebr_timer_data->count);

    if (esm_ebr_timer_data->count < DEFAULT_EPS_BEARER_ACTIVATE_COUNTER_MAX) {
      /*
       * Re-send activate default EPS bearer context request message
       * * * * to the UE
       */
      bstring b = bstrcpy(esm_ebr_timer_data->msg);
      rc = _default_eps_bearer_activate (esm_ebr_timer_data->ctx, esm_ebr_timer_data->ebi, &b);
    } else {
      /*
       * The maximum number of activate default EPS bearer context request
       * message retransmission has exceed
       */
      pdn_cid_t                               pid = MAX_APN_PER_UE;
      int                                     bidx = BEARERS_PER_UE;

      /*
       * Release the default EPS bearer context and enter state INACTIVE
       */
      rc = esm_proc_eps_bearer_context_deactivate (esm_ebr_timer_data->ctx, true, esm_ebr_timer_data->ebi, pid, NULL);

      if (rc != RETURNerror) {
        /*
         * Stop timer T3485
         */
        rc = esm_ebr_stop_timer (esm_ebr_timer_data->ctx, esm_ebr_timer_data->ebi);
      }
    }
    if (esm_ebr_timer_data->msg) {
      bdestroy_wrapper (&esm_ebr_timer_data->msg);
    }
    free_wrapper ((void**)&esm_ebr_timer_data);
  }

  OAILOG_FUNC_OUT (LOG_NAS_ESM);
}

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
  pti_t pti,
  ebi_t ebi,
  activate_default_eps_bearer_context_request_msg * msg,
  bstring apn,
  const protocol_configuration_options_t * pco,
  const ambr_t * ambr,
  int pdn_type,
  bstring pdn_addr,
  const EpsQualityOfService * qos,
  int esm_cause)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  /*
   * Mandatory - ESM message header
   */
  msg->protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  msg->epsbeareridentity = ebi;
  msg->messagetype = ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST;
  msg->proceduretransactionidentity = pti;
  /*
   * Mandatory - EPS QoS
   */
  msg->epsqos = *qos;
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - epsqos  qci:  %u\n", qos->qci);

  if (qos->bitRatesPresent) {
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - epsqos  maxBitRateForUL:  %u\n", qos->bitRates.maxBitRateForUL);
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - epsqos  maxBitRateForDL:  %u\n", qos->bitRates.maxBitRateForDL);
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - epsqos  guarBitRateForUL: %u\n", qos->bitRates.guarBitRateForUL);
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - epsqos  guarBitRateForDL: %u\n", qos->bitRates.guarBitRateForDL);
  } else {
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - epsqos  no bit rates defined\n");
  }

  if (qos->bitRatesExtPresent) {
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - epsqos  maxBitRateForUL  Ext: %u\n", qos->bitRatesExt.maxBitRateForUL);
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - epsqos  maxBitRateForDL  Ext: %u\n", qos->bitRatesExt.maxBitRateForDL);
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - epsqos  guarBitRateForUL Ext: %u\n", qos->bitRatesExt.guarBitRateForUL);
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - epsqos  guarBitRateForDL Ext: %u\n", qos->bitRatesExt.guarBitRateForDL);
  } else {
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - epsqos  no bit rates ext defined\n");
  }

  if (apn == NULL) {
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - apn is NULL!\n");
  } else {
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - apn is %s\n", bdata(apn));
  }
  /*
   * Mandatory - Access Point Name
   */
  msg->accesspointname = apn;
  /*
   * Mandatory - PDN address
   */
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - pdn_type is %u\n", pdn_type);
  msg->pdnaddress.pdntypevalue = pdn_type;
  OAILOG_STREAM_HEX (OAILOG_LEVEL_DEBUG, LOG_NAS_ESM, "ESM-SAP   - pdn_addr is ", bdata(pdn_addr), blength(pdn_addr));
  msg->pdnaddress.pdnaddressinformation = pdn_addr;
  /*
   * Optional - ESM cause code
   */
  msg->presencemask = 0;

  if (esm_cause != ESM_CAUSE_SUCCESS) {
    msg->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_PRESENT;
    msg->esmcause = esm_cause;
  }

  if (pco) {
    msg->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
    /**
     * The PCOs actually received by the SAE-GW.
     * todo: After handover, the received PCOs might have been changed (like DNS address), we need to check them and inform the UE.
     * todo: We also need to be able to ask the SAE-GW for specific PCOs?
     * In handover, any change in the ESM context information has to be forwarded to the UE --> MODIFY EPS BEARER CONTEXT REQUEST!! (probably not possible with TAU accept).
     */
    copy_protocol_configuration_options(&msg->protocolconfigurationoptions, pco);
  }

  /** Implementing subscribed values. */
  if(ambr){
    msg->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT;
    ambr_kbps_calc(&msg->apnambr, (ambr->br_dl/1000), (ambr->br_ul/1000));
  }else {
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - no APN AMBR is present for activating default eps bearer. \n");
  }
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Send Activate Default EPS Bearer Context " "Request message (pti=%d, ebi=%d)\n",
      msg->proceduretransactionidentity, msg->epsbeareridentity);
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
int
esm_proc_default_eps_bearer_context (
  mme_ue_s1ap_id_t   ue_id,
  const nas_esm_pdn_connectivity_proc_t * esm_pdn_connectivity_proc,
  bearer_qos_t      *bearer_qos,
  ambr_t             apn_ambr,
  esm_cause_t       *esm_cause)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Default EPS bearer context activation (ue_id=" MME_UE_S1AP_ID_FMT ", context_identifier=%d,  QCI %u)\n",ue_id, esm_pdn_connectivity_proc->pdn_cid, bearer_qos->qci);
  /*
   * Update default EPS bearer context and pdn context.
   */
  int rc = mme_app_esm_update_pdn_context(ue_id, esm_pdn_connectivity_proc->apn_subscribed, esm_pdn_connectivity_proc->pdn_cid, esm_pdn_connectivity_proc->default_ebi,
      ESM_EBR_ACTIVE_PENDING, apn_ambr, bearer_qos, (protocol_configuration_options_t*)NULL);
  if(rc == RETURNerror){
    OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Could not update the default EPS bearer context activation (ue_id=" MME_UE_S1AP_ID_FMT ", context_identifier=%d,  QCI %u)\. "
        "Removing the PDN context.",
        ue_id, esm_pdn_connectivity_proc->pdn_cid, bearer_qos->qci);
    // todo:    rc = esm_ebr_context_release( )proc_eps_bearer_context_deactivate (esm_context, true, *ebi, *pid, NULL);

  }
  REQUIREMENT_3GPP_24_301(R10_6_4_1_2);
  if(esm_pdn_connectivity_proc->attach_success){
    /** Not starting the T3485 timer. */
  }else{
    OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Starting T3485 for Default EPS bearer context activation (ue_id=" MME_UE_S1AP_ID_FMT ", context_identifier=%d,  QCI %u)\n",
        ue_id, esm_pdn_connectivity_proc->pdn_cid, bearer_qos->qci);
    /** Stop any timer if running. */
    nas_stop_esm_timer(ue_id, &esm_pdn_connectivity_proc->trx_base_proc.esm_proc_timer);
    /** Start the T3485 timer for additional PDN connectivity. */
    esm_pdn_connectivity_proc->trx_base_proc.esm_proc_timer.id = nas_timer_start (esm_pdn_connectivity_proc->trx_base_proc.esm_proc_timer.sec, 0 /*usec*/, TASK_NAS_ESM,
        _nas_proc_pdn_connectivity_timeout_handler, ue_id); /**< Address field should be big enough to save an ID. */


    nas_timer_start(esm_pdn_connectivity_proc->trx_base_proc.esm_proc_timer);
  }
  //     /**
//      *     //  emm_sap.primitive       = EMMESM_ACTIVATE_BEARER_REQ;
//   //  emm_sap.u.emm_esm.ue_id = ue_id;
//     /*
//      * Start T3485 retransmission timer
//      */
//     rc = esm_ebr_start_timer (esm_context, ebi, msg_dup, mme_config.nas_config.t3485_sec, _default_eps_bearer_activate_t3485_handler);
// //     *
// //     * Check if it is a standalone transactional pdn connectivity procedure.
// //     * If so, start the timer T3485, else don
// //     */
// //    //      todo: if it is a standalone procedure --> When informing the lower layer..
// //    //    /*
// ////     * Send activate default EPS bearer context request message and
// ////     * * * * start timer T3485
// ////     */
// ////    OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Initiate standalone default EPS bearer context activation " "(ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d)\n",
// ////        ue_id, ebi);




     //    //  emm_sap.primitive       = EMMESM_ACTIVATE_BEARER_REQ;
     //  //  emm_sap.u.emm_esm.ue_id = ue_id;
     //  //  emm_sap.u.emm_esm.ctx   = esm_context;
     //  //  emm_esm_activate->msg            = *msg;
     //  //  emm_esm_activate->ebi            = ebi;
     //    /*
     //     * Start T3485 retransmission timer
     //     */
     //    rc = esm_ebr_start_timer (esm_context, ebi, msg_dup, mme_config.nas_config.t3485_sec, _default_eps_bearer_activate_t3485_handler);


  *esm_cause = (rc == RETURNerror) ? ESM_CAUSE_REQUEST_REJECTED_BY_GW : ESM_CAUSE_SUCCESS;
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
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
    nas_esm_pdn_connectivity_proc_t * esm_pdn_connectivity_proc){

  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc;
  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Default EPS bearer context activation "
      "accepted by the UE (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d)\n", ue_id, ebi);
  /*
   * Update the ESM bearer context state of the default bearer as ACTIVE.
   * The CN state & FTEID will be set by the MME_APP layer in independently of this.
   */
  mme_app_update_esm_state(ue_id, esm_pdn_connectivity_proc->apn_subscribed, esm_pdn_connectivity_proc->pdn_cid,
      esm_pdn_connectivity_proc->default_ebi, ESM_EBR_ACTIVE);
  /** No need to inform the anything else. */
  OAILOG_FUNC_OUT(LOG_NAS_ESM);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

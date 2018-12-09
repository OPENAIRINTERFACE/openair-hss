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
  Source      EpsBearerContextModification.c

  Version     0.1

  Date        2018/11/19

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Dincer Beken

  The purpose of the EPS bearer context modification procedure is to modify an EPS bearer context with a specific QoS
  and TFT. The EPS bearer context modification procedure is initiated by the network, but it may also be initiated as part
  of the UE requested bearer resource allocation procedure or the UE requested bearer resource modification procedure.

  The network may also initiate the EPS bearer context modification procedure to update the APN-AMBR of the UE, for
  instance after an inter-system handover. See 3GPP TS 23.401 [10] annex E.

  The network may initiate the EPS bearer context modification procedure together with the completion of the service
  request procedure.

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
#include "assertions.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "mme_app_ue_context.h"
#include "common_defs.h"
#include "mme_config.h"
#include "nas_itti_messaging.h"
#include "mme_app_defs.h"
#include "emm_data.h"
#include "emm_sap.h"
#include "esm_ebr.h"
#include "esm_ebr_context.h"
#include "esm_proc.h"
#include "esm_cause.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
   Internal data handled by the EPS bearer context modification
   procedure in the MME
   --------------------------------------------------------------------------
*/
/*
   Timer handlers
*/
static void _modify_eps_bearer_context_t3486_handler (void *);

/* Maximum value of the modify EPS bearer context request
   retransmission counter */
#define EPS_BEARER_CONTEXT_MODIFY_COUNTER_MAX   5

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_modify_eps_bearer_context_request()  **
 **                                                                        **
 ** Description: Builds Modify EPS Bearer Context Request message  **
 **                                                                        **
 **      The modify EPS bearer context request message **
 **      is sent by the network to the UE to request modification of **
 **      an EPS bearer context which is already activated..          **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      qos:       EPS quality of service                     **
 **      tft:       Traffic flow template                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
esm_send_modify_eps_bearer_context_request (
  pti_t pti,
  ebi_t ebi,
  modify_eps_bearer_context_request_msg * msg,
  const EpsQualityOfService * qos,
  traffic_flow_template_t *tft,
  ambr_t *ambr,
  protocol_configuration_options_t *pco)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  /*
   * Mandatory - ESM message header
   */
  msg->protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  msg->epsbeareridentity = ebi;
  msg->messagetype = MODIFY_EPS_BEARER_CONTEXT_REQUEST;
  msg->proceduretransactionidentity = pti;
  msg->presencemask = 0;

  /*
   * Optional - EPS QoS
   */
  if(qos) {
    msg->newepsqos = *qos;
    msg->presencemask |= MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_QOS_PRESENT;
  }
  /*
   * Optional - traffic flow template.
   * Send the TFT as it is. It will contain the operation.
   * We will modify the bearer context when the bearers are accepted.
   */
  if (tft) {
    memcpy(&msg->tft, tft, sizeof(traffic_flow_template_t));
    msg->presencemask |= MODIFY_EPS_BEARER_CONTEXT_REQUEST_TFT_PRESENT;
  }
  /*
   * Optional - APN AMBR
   * Implementing subscribed values.
   */
  if(ambr){
    if(ambr->br_dl && ambr->br_ul){
      msg->presencemask |= MODIFY_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT;
      ambr_kbps_calc(&msg->apnambr, (ambr->br_dl/1000), (ambr->br_ul/1000));
    }
  }else {
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - no APN AMBR is present for activating default eps bearer. \n");
  }

  if (pco) {
    memcpy(&msg->protocolconfigurationoptions, pco, sizeof(protocol_configuration_options_t));
    msg->presencemask |= MODIFY_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
  }
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Send Modify EPS Bearer Context " "Request message (pti=%d, ebi=%d). \n", msg->proceduretransactionidentity, msg->epsbeareridentity);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
}

/*
   --------------------------------------------------------------------------
      EPS bearer context modification procedure executed by the MME
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_modify_eps_bearer_context_accept()                   **
 **                                                                        **
 ** Description: Allocates resources required for modification of an EPS   **
 **      bearer context.                                                   **
 **                                                                        **
 ** Inputs:  esm_context: ESM Context                                      **
 **          pid:       PDN connection identifier                  **
 **      esm_qos:   EPS bearer level QoS parameters            **
 **      tft:       Traffic flow template parameters           **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     ebi:       EPS bearer identity assigned to the new    **
 **             dedicated bearer context                   **
 **      default_ebi:   EPS bearer identity of the associated de-  **
 **             fault EPS bearer context                   **
 **      esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_proc_modify_eps_bearer_context (
  mme_ue_s1ap_id_t   ue_id,
  const proc_tid_t   pti,
  nas_esm_bearer_context_proc_t  * esm_bearer_context_proc,
  bearer_context_to_be_updated_t * bc_tbu)
{
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - EPS bearer context modification " "(ue_id=" MME_UE_S1AP_ID_FMT ")\n", ue_id);

  esm_cause = mme_app_esm_modify_bearer_context(ue_id, bc_tbu->eps_bearer_id, bc_tbu->bearer_level_qos, &bc_tbu->tft);
  if(esm_cause != ESM_CAUSE_SUCCESS){
    /*
     * Error verifying the bearer QoS/TFT.
     * Return a faulty ESM cause, reject the message and destroy the transaction.
     */
    /** Inform the MME APP. */
    nas_itti_modify_eps_bearer_ctx_rej(ue_id, bc_tbu->eps_bearer_id, esm_cause); /**< Assuming, no other CN bearer procedure will intervere. */

    _esm_proc_free_bearer_context_procedure(&esm_bearer_context_proc);
    OAILOG_FUNC_RETURN(LOG_NAS_ESM, esm_cause);
  }

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Verified ePS bearer context modification " "(ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d)\n", ue_id, bc_tbu->eps_bearer_id);

  /*
   * Setup the transaction.
   */
  esm_proc_modify_eps_bearer_context_request; /**< Not the procedure. */

  //             rc = esm_proc_modify_eps_bearer_context_reject(msg->ctx, bcs_tbu->bearer_contexts[num_bc].eps_bearer_id,
  //                 &esm_cause, false);
  //             /** Inform the MME APP. */
  //             nas_itti_modify_eps_bearer_ctx_rej(msg->ue_id, bcs_tbu->bearer_contexts[num_bc].eps_bearer_id, esm_cause); /**< Assuming, no other CN bearer procedure will intervere. */
      // todo: set the timer and update the esm ebr context
  //    esm_procedure = esm_proc_modify_eps_bearer_context_request; /**< Not the procedure. */
}
  OAILOG_FUNC_RETURN(LOG_NAS_ESM, esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_update_eps_bearer_context()                   **
 **                                                                        **
 ** Description: Makes the final configuration on the existing session     **
 **      bearer context for successfully updated bearer contexts           **
 **      after the UBR has been sent.                                      **
 **                                                                        **
 ** Inputs:  bc_tbu:      Contains, cause, qos and TFT parameters of       **
 **                           the bearers. E local identifier              **
 **      Others:    None                                       **
 **                                                                        **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
esm_proc_update_eps_bearer_context (
  esm_context_t * esm_context,
  const bearer_context_to_be_updated_t *bc_tbu)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNok;
  mme_ue_s1ap_id_t                        ue_id = esm_context->ue_id;
  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Updating EPS bearer context configuration to new values " "(ue_id=" MME_UE_S1AP_ID_FMT " \n", ue_id);

  ue_context_t                        *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, esm_context->ue_id);

  /** Check the cause. */
  if(bc_tbu->cause.cause_value != REQUEST_ACCEPTED){
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-PROC  - ESM Bearer Context for ebi %d has not been successfully updated for ueId " MME_UE_S1AP_ID_FMT ". \n", bc_tbu->eps_bearer_id, ue_context->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
  }

  if(esm_ebr_get_status(esm_context, bc_tbu->eps_bearer_id) != ESM_EBR_ACTIVE){
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-PROC  - ESM Bearer Context for ebi %d is not ACTIVE for ue " MME_UE_S1AP_ID_FMT ". \n", bc_tbu->eps_bearer_id, ue_context->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
  }

  /** Update the QoS parameter if exists. */
  bearer_context_t *bc = NULL;
  mme_app_get_session_bearer_context_from_all(ue_context, bc_tbu->eps_bearer_id, &bc);
  DevAssert(bc);

  rc = esm_ebr_context_update(esm_context, bc, bc_tbu->bearer_level_qos, &bc_tbu->tft, &bc_tbu->pco);

  /** Not updating the parameters yet. Updating later when a success is received. State will be updated later. */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_modify_eps_bearer_context_request()           **
 **                                                                        **
 ** Description: Initiates the EPS bearer context modification pro-**
 **      cedure                                                    **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.3.2                           **
 **      The MME initiates the EPS bearer context modification     **
 **      procedure by sending an MODIFY EPS BEARER CONTEXT         **
 **      REQUEST message, starting timer T3486 and entering        **
 **      state BEARER CONTEXT MODIFY PENDING.                      **
 **                                                                        **
 ** Inputs:  is_standalone: Not used (always true)                     **
 **      ue_id:      UE lower layer identifier                  **
 **      ebi:       EPS bearer identity                        **
 **      msg:       Encoded ESM message to be sent             **
 **      ue_triggered:  true if the EPS bearer context procedure   **
 **             was triggered by the UE                    **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
esm_proc_modify_eps_bearer_context_request(
  mme_ue_s1ap_id_t ue_id,
  ebi_t ebi,
  STOLEN_REF bstring *msg)
{
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Initiate EPS bearer context " "modification (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d)\n",
      ue_id, ebi);
  /*
   * Send modify EPS bearer context request message and
   * * * * start timer T3486
   */
  rc = _modify_eps_bearer_context(esm_context, ebi, msg);

  if (rc != RETURNerror) {
    /*
     * Set the EPS bearer context state to MODIFY PENDING
     */
    rc = esm_ebr_set_status (esm_context, ebi, ESM_EBR_MODIFY_PENDING, ue_triggered);

    if (rc != RETURNok) {
      /*
       * The EPS bearer context was already in MODIFY PENDING state
       */
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - EBI %d was already MODIFY PENDING\n", ebi);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_modify_eps_bearer_context_accept()            **
 **                                                                        **
 ** Description: Performs EPS bearer context modification procedu- **
 **      re accepted by the UE.                                    **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.3.3                           **
 **      Upon receipt of the MODIFY EPS BEARER CONTEXT ACCEPT      **
 **      message, the MME shall stop the timer T3486 and enter     **
 **      enter the state BEARER CONTEXT ACTIVE.                    **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **      ebi:       EPS bearer identity                        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_proc_modify_eps_bearer_context_accept (
  mme_ue_s1ap_id_t ue_id,
  ebi_t ebi,
  esm_cause_t *esm_cause)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - EPS bearer context modification " "accepted by the UE (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d)\n",
      ue_id, ebi);

  /*
   * Set the MME_APP Bearer context as final.
   * todo: update the TFT and qos now or before?
   */
  mme_app_esm_finalize_bearer_context(ue_id, ebi)

  /*
   * Check that an EPS procedure exists.
   */
  nas_esm_bearer_context_proc_t * esm_bearer_procedure = _esm_get_bearer_procedure(ue_id, ebi, pti);
  if(!esm_bearer_procedure){
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-PROC  - No ESM bearer procedure exists for accepted dedicated bearer (ebi=%d, pti=%d) for UE " MME_UE_S1AP_ID_FMT ". \n", ebi, pti, ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED);
  }

  /*
   * Stop the ESM procedure.
   */
  _esm_proc_free_bearer_context_procedure(&esm_bearer_procedure);

  /*
   * Update the bearer context.
   */
  esm_cause = mme_app_esm_finalize_bearer_context(ue_id, ebi);

  if(esm_cause == ESM_CAUSE_SUCCESS){
    /**
     * The ESM procedure was terminated implicitly but the bearer context remained (old status was same (ESM_ACTIVE).
     * We changed the status of the ESM bearer context, so the ESM reply came before any E-RAB failures, if any.
     *
     * The current bearers are not updated yet.
     */
    nas_itti_modify_eps_bearer_ctx_cnf(ue_id, ebi);
  }else {
    /** Bearer not found, assuming due E-RAB error handling. */
  }

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_modify_eps_bearer_context_reject()            **
 **                                                                        **
 ** Description: Performs dedicated EPS bearer context modification
 **      procedure not accepted by the UE.                                 **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.3.4                           **
 **      Upon receipt of the MODIFY EPS BEARER CONTEXT REJECT      **
 **      message, the MME shall stop the timer T3486, enter the    **
 **      state BEARER CONTEXT ACTIVE and abort the EPS EPS bearer context
 **      modification activation procedure.
 **
 **      If the network receives the MODIFY EPS BEARER CONTEXT REJECT      **
 **      message with ESM cause #43 "invalid EPS bearer identity", the MME **
 **      locally deactivates the EPS bearer context(s)                     **
 **      without peer-to-peer ESM signaling.                               **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **      ebi:       EPS bearer identity                        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_proc_modify_eps_bearer_context_reject (
  mme_ue_s1ap_id_t ue_id,
  ebi_t ebi,
  esm_cause_t *esm_cause)
{
  int                                     rc;
  pdn_cid_t                               pdn_ci = PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED;

  OAILOG_FUNC_IN (LOG_NAS_ESM);
  OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - EPS bearer context modification " "not accepted by the UE (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d, esm_cause %d)\n",
      ue_id, ebi, *esm_cause);
  /*
   * Stop T3486 timer if running.
   * Will also check if the bearer exists. If not (E-RAB Setup Failure), the message will be dropped.
   */
  rc = esm_ebr_stop_timer (esm_context, ebi);

  if (rc != RETURNerror) {
    /*
     * Session bearer still exists.
     * Check the cause code and depending continue or disable the bearer context locally.
     */

    esm_ebr_state oldState = esm_ebr_get_status(esm_context, ebi);

    if(*esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY){
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Deactivating the ESM bearer locally. " "(ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d)\n", ue_id, ebi);
      ebi = esm_ebr_context_release (esm_context, ebi, pdn_ci, false);
      if (ebi == ESM_EBI_UNASSIGNED) {  /**< Bearer was already released. */
        OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Failed to release EPS bearer context\n");
        OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
      }
    }

    if (*esm_cause != ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY) {
      /*
       * Session bearer existed and was not removed.
       * Failed to release the modify the EPS bearer context
       */
      *esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
      /** Set the state back to active. */
      esm_ebr_set_status(esm_context, ebi, ESM_EBR_ACTIVE, ue_requested);
    }
    /**
     * Inform the MME_APP only if it came as an ESM response, the bearer existed and no E-RAB failure was received yet. The S11 procedure should be handled with already.
     * We don't check the return value of status setting, because the bearer might have also been removed implicitly.
     */
    if(ue_requested && oldState != ESM_EBR_ACTIVE) /**< Check if the procedure was not released before due E-RAB failure. */
      nas_itti_modify_eps_bearer_ctx_rej(ue_id, ebi, esm_cause); /**< The SAE-GW should release the cause, too. */
  } else {
    /** No E-RAB  could be found. Can live with it. */
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
                Timer handlers
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    _modify_eps_bearer_context_t3486_handler()                    **
 **                                                                        **
 ** Description: T3486 timeout handler                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 6.4.3.6, case a                   **
 **      On the first expiry of the timer T3486, the MME shall re- **
 **      send the MODIFY DEDICATED EPS BEARER CONTEXT REQUEST    **
 **      and shall reset and restart timer T3486. This retransmis- **
 **      sion is repeated four times, i.e. on the fifth expiry of  **
 **      timer T3486, the MME shall abort the procedure, release   **
 **      any resources allocated for this activation and enter the **
 **      state BEARER CONTEXT ACTIVE .
 **
 **      The MME may continue to use the previous configuration of **
 **      the EPS bearer context or initiate an EPS bearer context  **
 **      deactivation procedure (currently continue using          **
 **      previous configuration).                                  **
 **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void _modify_eps_bearer_context_t3486_handler (void *args)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc;

  /*
   * Get retransmission timer parameters data
   */
  esm_ebr_timer_data_t                   *esm_ebr_timer_data = (esm_ebr_timer_data_t *) (args);

  if (esm_ebr_timer_data) {
    /*
     * Increment the retransmission counter
     */
    esm_ebr_timer_data->count += 1;
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - T3486 timer expired (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d), " "retransmission counter = %d\n",
        esm_ebr_timer_data->ue_id, esm_ebr_timer_data->ebi, esm_ebr_timer_data->count);

    if (esm_ebr_timer_data->count < EPS_BEARER_CONTEXT_MODIFY_COUNTER_MAX) {
      /*
       * Re-send modify EPS bearer context request message
       * * * * to the UE
       */
      bstring b = bstrcpy(esm_ebr_timer_data->msg);
      rc = _modify_eps_bearer_context (esm_ebr_timer_data->ctx, esm_ebr_timer_data->ebi, &b);
    } else {
      /*
       * The maximum number of modify EPS bearer context request
       * message retransmission has exceed
       */
      pdn_cid_t                               pid = MAX_APN_PER_UE;

      /*
       * Keep the current content of the EPS bearer context and stay in state ACTIVE.
       */
      rc = esm_ebr_set_status(esm_ebr_timer_data->ctx, esm_ebr_timer_data->ebi, ESM_EBR_ACTIVE, false);

      if (rc != RETURNerror) {
        /*
         * Stop timer T3486
         */
        rc = esm_ebr_stop_timer (esm_ebr_timer_data->ctx, esm_ebr_timer_data->ebi);
      }

      /** Send a reject back to the MME_APP layer to continue with the Update Bearer Response. */
      nas_itti_modify_eps_bearer_ctx_rej(esm_ebr_timer_data->ctx->ue_id, esm_ebr_timer_data->ebi, 0);

    }
    if (esm_ebr_timer_data->msg) {
      bdestroy_wrapper (&esm_ebr_timer_data->msg);
    }
    free_wrapper ((void**)&esm_ebr_timer_data);
  }

  OAILOG_FUNC_OUT (LOG_NAS_ESM);
}

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/

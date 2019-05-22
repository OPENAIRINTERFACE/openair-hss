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
  Source      DedicatedEpsBearerContextActivation.c

  Version     0.1

  Date        2013/07/16

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Frederic Maurel

  Description Defines the dedicated EPS bearer context activation ESM
        procedure executed by the Non-Access Stratum.

        The purpose of the dedicated EPS bearer context activation
        procedure is to establish an EPS bearer context with specific
        QoS and TFT between the UE and the EPC.

        The procedure is initiated by the network, but may be requested
        by the UE by means of the UE requested bearer resource alloca-
        tion procedure or the UE requested bearer resource modification
        procedure.
        It can be part of the attach procedure or be initiated together
        with the default EPS bearer context activation procedure when
        the UE initiated stand-alone PDN connectivity procedure.

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
   Internal data handled by the dedicated EPS bearer context activation
   procedure in the MME
   --------------------------------------------------------------------------
*/
/*
   Timer handlers
*/
static esm_cause_t _dedicated_eps_bearer_activate_t3485_handler (nas_esm_proc_t * esm_base_proc, ESM_msg *esm_resp_msg);

/* Maximum value of the activate dedicated EPS bearer context request
   retransmission counter */
#define DEDICATED_EPS_BEARER_ACTIVATE_COUNTER_MAX   1

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_activate_dedicated_eps_bearer_context_request()  **
 **                                                                        **
 ** Description: Builds Activate Dedicated EPS Bearer Context Request      **
 **      message                                                   **
 **                                                                        **
 **      The activate dedicated EPS bearer context request message **
 **      is sent by the network to the UE to request activation of **
 **      a dedicated EPS bearer context associated with the same   **
 **      PDN address(es) and APN as an already active default EPS  **
 **      bearer context.                                           **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      linked_ebi:    EPS bearer identity of the default bearer  **
 **             associated with the EPS dedicated bearer   **
 **             to be activated                            **
 **      qos:       EPS quality of service                     **
 **      tft:       Traffic flow template                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void
esm_send_activate_dedicated_eps_bearer_context_request (
  pti_t pti,
  ebi_t ebi,
  ESM_msg * esm_msg,
  ebi_t linked_ebi,
  const EpsQualityOfService * qos,
  traffic_flow_template_t *tft,
  protocol_configuration_options_t *pco)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  memset(esm_msg, 0, sizeof(ESM_msg));
  /*
   * Mandatory - ESM message header
   */
  esm_msg->activate_dedicated_eps_bearer_context_request.protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  esm_msg->activate_dedicated_eps_bearer_context_request.epsbeareridentity = ebi;
  esm_msg->activate_dedicated_eps_bearer_context_request.messagetype = ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST;
  esm_msg->activate_dedicated_eps_bearer_context_request.proceduretransactionidentity = pti;
  esm_msg->activate_dedicated_eps_bearer_context_request.linkedepsbeareridentity = linked_ebi;
  /*
   * Mandatory - EPS QoS
   */
  memcpy((void*)&esm_msg->activate_dedicated_eps_bearer_context_request.epsqos, qos, sizeof(EpsQualityOfService));
  /*
   * Mandatory - traffic flow template
   */
  if (tft) {
    memcpy(&esm_msg->activate_dedicated_eps_bearer_context_request.tft, tft, sizeof(traffic_flow_template_t));
  }

  /*
   * Optional
   */
  esm_msg->activate_dedicated_eps_bearer_context_request.presencemask = 0;
//  if (pco) {
//    memcpy(&msg->protocolconfigurationoptions, pco, sizeof(protocol_configuration_options_t));
//    msg->presencemask |= ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
//  }
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Send Activate Dedicated EPS Bearer Context " "Request message (pti=%d, ebi=%d). \n",
      esm_msg->activate_dedicated_eps_bearer_context_request.proceduretransactionidentity, esm_msg->activate_dedicated_eps_bearer_context_request.epsbeareridentity);
  OAILOG_FUNC_OUT(LOG_NAS_ESM);
}

/*
   --------------------------------------------------------------------------
      Dedicated EPS bearer context activation procedure executed by the MME
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_dedicated_eps_bearer_context()                   **
 **                                                                        **
 ** Description: Allocates resources required for activation of a dedica-  **
 **      ted EPS bearer context.                                   **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
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
esm_proc_dedicated_eps_bearer_context (
  mme_ue_s1ap_id_t   ue_id,
  const proc_tid_t   pti,
  ebi_t              linked_ebi,
  const pdn_cid_t    pdn_cid,
  bearer_context_to_be_created_t *bc_tbc,
  bool               * const pending_pdn_proc,
  ESM_msg           *esm_rsp_msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  ebi_t                                   ded_ebi = 0;
  pdn_context_t                          *pdn_context = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Dedicated EPS bearer context activation " "(ue_id=" MME_UE_S1AP_ID_FMT ", pid=%d)\n", ue_id, pdn_cid);

  mme_app_get_pdn_context(ue_id, pdn_cid, linked_ebi, NULL, &pdn_context);
  if(!pdn_context){
    OAILOG_ERROR(LOG_NAS_EMM, "EMMCN-SAP  - " "No PDN context was found for UE " MME_UE_S1AP_ID_FMT" for cid %d and default ebi %d to assign dedicated bearers.\n",
        ue_id, pdn_cid, linked_ebi);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_PDN_CONNECTION_DOES_NOT_EXIST);
  }

  nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity = _esm_proc_get_pdn_connectivity_procedure(ue_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED);
  if(esm_proc_pdn_connectivity){
    if(esm_proc_pdn_connectivity->default_ebi == linked_ebi){
      OAILOG_ERROR(LOG_NAS_EMM, "EMMCN-SAP  - " "A PDN procedure for default ebi %d exists for UE " MME_UE_S1AP_ID_FMT" (cid=%d). Rejecting the establishment of the dedicated bearer.\n",
          linked_ebi, ue_id, esm_proc_pdn_connectivity->pdn_cid);
      *pending_pdn_proc = true;
      esm_proc_pdn_connectivity->pending_qos = true;
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
    } else {
      OAILOG_WARNING(LOG_NAS_EMM, "EMMCN-SAP  - " "A PDN procedure for default ebi %d exists for UE " MME_UE_S1AP_ID_FMT" (cid=%d). Continuing with establishment of dedicated bearers.\n",
          esm_proc_pdn_connectivity->default_ebi, ue_id, esm_proc_pdn_connectivity->pdn_cid);
    }
  }

  /*
   * Register a new EPS bearer context into the MME.
   * This should only be for dedicated bearers (todo: handover with dedicated bearers).
   */
  // todo: PCO handling
  traffic_flow_template_t * tft = bc_tbc->tft;
  esm_cause = mme_app_register_dedicated_bearer_context(ue_id, ESM_EBR_ACTIVE_PENDING, pdn_context->context_identifier, pdn_context->default_ebi, bc_tbc, ESM_EBI_UNASSIGNED);  /**< We set the ESM state directly to active in handover. */
  if(esm_cause != ESM_CAUSE_SUCCESS){
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-PROC  - Error assigning bearer context for ue " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
  }

  /*
   * No need to check for PDN connectivity procedures.
   * They should be handled together.
   * Create a new EPS bearer context transaction and starts the timer, since no further CN operation necessary for dedicated bearers.
   */
  nas_esm_proc_bearer_context_t * esm_proc_bearer_context = _esm_proc_create_bearer_context_procedure(ue_id, pti, linked_ebi, pdn_cid, bc_tbc->eps_bearer_id, bc_tbc->s1u_sgw_fteid.teid,
      mme_config.nas_config.t3485_sec, 0 /*usec*/, _dedicated_eps_bearer_activate_t3485_handler);
  if(!esm_proc_bearer_context){
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-PROC  - Error creating a new procedure for the bearer context (ebi=%d) for UE " MME_UE_S1AP_ID_FMT ". \n", bc_tbc->eps_bearer_id, ue_id);
    mme_app_release_bearer_context(ue_id, &pdn_context->context_identifier, pdn_context->default_ebi, bc_tbc->eps_bearer_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }
  /** Send the response message back. */
  EpsQualityOfService eps_qos = {0};
  /** Sending a EBR-Request per bearer context. */
  memset((void*)&eps_qos, 0, sizeof(eps_qos));
  /** Set the EPS QoS. */
  qos_params_to_eps_qos(bc_tbc->bearer_level_qos.qci,
      bc_tbc->bearer_level_qos.mbr.br_dl, bc_tbc->bearer_level_qos.mbr.br_ul,
      bc_tbc->bearer_level_qos.gbr.br_dl, bc_tbc->bearer_level_qos.gbr.br_ul,
      &eps_qos, false);
  esm_send_activate_dedicated_eps_bearer_context_request (
      PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, bc_tbc->eps_bearer_id,
      esm_rsp_msg,
      linked_ebi, &eps_qos,
      tft,
      &bc_tbc->pco);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_dedicated_eps_bearer_context_accept()            **
 **                                                                        **
 ** Description: Performs dedicated EPS bearer context activation procedu- **
 **      re accepted by the UE.                                    **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.2.3                           **
 **      Upon receipt of the ACTIVATE DEDICATED EPS BEARER CONTEXT **
 **      ACCEPT message, the MME shall stop the timer T3485 and    **
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
esm_proc_dedicated_eps_bearer_context_accept (
  mme_ue_s1ap_id_t ue_id,
  ebi_t            ebi,
  nas_esm_proc_bearer_context_t *esm_bearer_procedure)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Dedicated EPS bearer context activation " "accepted by the UE (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d)\n",
      ue_id, ebi);

  /*
   * Update the bearer context.
   */
  esm_cause = mme_app_finalize_bearer_context(ue_id, esm_bearer_procedure->pdn_cid, esm_bearer_procedure->linked_ebi, ebi, NULL, NULL, NULL, NULL);

  if(esm_cause == ESM_CAUSE_SUCCESS){
    /*
     * No need for an ESM procedure.
     * Just set the status to active and inform the MME_APP layer.
     */
    nas_itti_activate_eps_bearer_ctx_cnf(ue_id, ebi, esm_bearer_procedure->s1u_saegw_teid);
  } else{
    nas_itti_activate_eps_bearer_ctx_rej(ue_id, esm_bearer_procedure->s1u_saegw_teid, esm_cause);
  }

  /*
   * Stop the ESM procedure.
   */
  _esm_proc_free_bearer_context_procedure(&esm_bearer_procedure);

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_dedicated_eps_bearer_context_reject()            **
 **                                                                        **
 ** Description: Performs dedicated EPS bearer context activation procedu- **
 **      re not accepted by the UE.                                **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.2.4                           **
 **      Upon receipt of the ACTIVATE DEDICATED EPS BEARER CONTEXT **
 **      REJECT message, the MME shall stop the timer T3485, enter **
 **      the state BEARER CONTEXT INACTIVE and abort the dedicated **
 **      EPS bearer context activation procedure.                  **
 **      The MME also requests the lower layer to release the ra-  **
 **      dio resources that were established during the dedicated  **
 **      EPS bearer context activation.                            **
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
void
esm_proc_dedicated_eps_bearer_context_reject (
  mme_ue_s1ap_id_t  ue_id,
  ebi_t ebi,
  nas_esm_proc_bearer_context_t * esm_bearer_procedure,
  esm_cause_t esm_cause)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  /*
   * Stop T3485 timer if running.
   * Will also check if the bearer exists. If not (E-RAB Setup Failure), the message will be dropped.
   */
  mme_app_release_bearer_context(ue_id, &esm_bearer_procedure->pdn_cid, esm_bearer_procedure->linked_ebi, ebi);

  /*
   * Failed to release the dedicated EPS bearer context.
   * Inform the MME_APP layer.
   */
  nas_itti_activate_eps_bearer_ctx_rej(ue_id, esm_bearer_procedure->s1u_saegw_teid, esm_cause);

  /*
   * Stop the ESM procedure.
   */
  _esm_proc_free_bearer_context_procedure(&esm_bearer_procedure);
  OAILOG_FUNC_OUT(LOG_NAS_ESM);
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
 ** Name:    _dedicated_eps_bearer_activate_t3485_handler()            **
 **                                                                        **
 ** Description: T3485 timeout handler                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 6.4.2.6, case a                   **
 **      On the first expiry of the timer T3485, the MME shall re- **
 **      send the ACTIVATE DEDICATED EPS BEARER CONTEXT REQUEST    **
 **      and shall reset and restart timer T3485. This retransmis- **
 **      sion is repeated four times, i.e. on the fifth expiry of  **
 **      timer T3485, the MME shall abort the procedure, release   **
 **      any resources allocated for this activation and enter the **
 **      state BEARER CONTEXT INACTIVE.                            **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static esm_cause_t _dedicated_eps_bearer_activate_t3485_handler (nas_esm_proc_t * esm_base_proc, ESM_msg *esm_resp_msg)
{
  OAILOG_FUNC_IN(LOG_NAS_ESM);
  nas_esm_proc_bearer_context_t * esm_proc_bearer_context = (nas_esm_proc_bearer_context_t*)esm_base_proc;
  pdn_context_t * pdn_context = NULL;
  bearer_context_t * bearer_context = NULL;

  esm_base_proc->retx_count += 1;
  if (esm_base_proc->retx_count < DEDICATED_EPS_BEARER_ACTIVATE_COUNTER_MAX) {
    /*
     * Create a new ESM-Information request and restart the timer.
     * Keep the ESM transaction.
     */
    mme_app_get_pdn_context(esm_base_proc->ue_id, esm_proc_bearer_context->pdn_cid,
        esm_proc_bearer_context->linked_ebi, NULL , &pdn_context);
    if(pdn_context){
      bearer_context = mme_app_get_session_bearer_context(pdn_context, esm_proc_bearer_context->bearer_ebi);
      if(bearer_context){
        /* Resend the message and restart the timer. */
        EpsQualityOfService eps_qos = {0};
        /** Sending a EBR-Request per bearer context. */
        memset((void*)&eps_qos, 0, sizeof(eps_qos));
        /** Set the EPS QoS. */
        qos_params_to_eps_qos(bearer_context->bearer_level_qos.qci,
            bearer_context->bearer_level_qos.mbr.br_dl, bearer_context->bearer_level_qos.mbr.br_ul,
            bearer_context->bearer_level_qos.gbr.br_dl, bearer_context->bearer_level_qos.gbr.br_ul,
            &eps_qos, false);
        esm_send_activate_dedicated_eps_bearer_context_request(PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED,
            bearer_context->ebi,
            esm_resp_msg,
            bearer_context->linked_ebi,
            &bearer_context->bearer_level_qos,
            bearer_context->esm_ebr_context.tft,
            /* bearer_context->esm_ebr_context.pco */ NULL);
        // todo: ideally, the following should also be in a lock..
        /* Restart the T3485 timer. */
        nas_stop_esm_timer(esm_base_proc->ue_id, &esm_base_proc->esm_proc_timer);
        esm_base_proc->esm_proc_timer.id = nas_esm_timer_start(mme_config.nas_config.t3485_sec, 0 /*usec*/, (void*)esm_base_proc);
        /** Retransmission only applies for NAS messages. */
        OAILOG_FUNC_RETURN(LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
      }
    }
  }
  /*
   * The maximum number of activate dedicated EPS bearer context request
   * message retransmission has exceed
   */
  pdn_cid_t                               pid = MAX_APN_PER_UE;
  /*
   * Release the dedicated EPS bearer context and enter state INACTIVE
   */
  teid_t s1u_saegw_teid = esm_proc_bearer_context->s1u_saegw_teid;
  /** Send a reject back to the MME_APP layer. */
  nas_itti_activate_eps_bearer_ctx_rej(esm_base_proc->ue_id, s1u_saegw_teid, ESM_CAUSE_INSUFFICIENT_RESOURCES);

  /*
   * Release the dedicated EPS bearer context and enter state INACTIVE
   */
  mme_app_release_bearer_context(esm_base_proc->ue_id,
      &esm_proc_bearer_context->pdn_cid, esm_proc_bearer_context->linked_ebi, esm_proc_bearer_context->bearer_ebi);

  /*
   * Release the transactional PDN connectivity procedure.
   */
  _esm_proc_free_bearer_context_procedure(&esm_proc_bearer_context);

  OAILOG_FUNC_RETURN(LOG_NAS_ESM, ESM_CAUSE_NETWORK_FAILURE);
}

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/


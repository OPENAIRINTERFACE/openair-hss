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
  Source      EpsBearerContextDeactivation.c

  Version     0.1

  Date        2013/05/22

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Frederic Maurel

  Description Defines the EPS bearer context deactivation ESM procedure
        executed by the Non-Access Stratum.

        The purpose of the EPS bearer context deactivation procedure
        is to deactivate an EPS bearer context or disconnect from a
        PDN by deactivating all EPS bearer contexts to the PDN.
        The EPS bearer context deactivation procedure is initiated
        by the network, and it may be triggered by the UE by means
        of the UE requested bearer resource modification procedure
        or UE requested PDN disconnect procedure.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"
#include "assertions.h"

#include "log.h"
#include "dynamic_memory_check.h"
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "mme_app_ue_context.h"
#include "mme_app_bearer_context.h"
#include "common_defs.h"
#include "mme_config.h"
#include "mme_app_defs.h"
#include "emm_data.h"
#include "emm_sap.h"
#include "esm_ebr.h"
#include "esm_proc.h"
#include "esm_cause.h"
#include "esm_sap.h"
#include "esm_data.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
   Timer handlers
*/
static void
_eps_bearer_deactivate_t3495_handler(nas_esm_proc_t * esm_proc, ESM_msg *esm_resp_msg);

/*
   --------------------------------------------------------------------------
   Internal data handled by the EPS bearer context deactivation procedure
   in the MME
   --------------------------------------------------------------------------
*/

/* Maximum value of the deactivate EPS bearer context request
   retransmission counter */
#define EPS_BEARER_DEACTIVATE_COUNTER_MAX   5

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_deactivate_eps_bearer_context_request()          **
 **                                                                        **
 ** Description: Builds Deactivate EPS Bearer Context Request message      **
 **                                                                        **
 **      The deactivate EPS bearer context request message is sent **
 **      by the network to request deactivation of an active EPS   **
 **      bearer context.                                           **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      esm_cause: ESM cause code                             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void
esm_send_deactivate_eps_bearer_context_request (
  pti_t pti,
  ebi_t ebi,
  ESM_msg * esm_rsp_msg,
  esm_cause_t esm_cause)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  memset(esm_rsp_msg, 0, sizeof(ESM_msg));

  /*
   * Mandatory - ESM message header
   */
  esm_rsp_msg->deactivate_eps_bearer_context_request.protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  esm_rsp_msg->deactivate_eps_bearer_context_request.epsbeareridentity = ebi;
  esm_rsp_msg->deactivate_eps_bearer_context_request.messagetype = DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST;
  esm_rsp_msg->deactivate_eps_bearer_context_request.proceduretransactionidentity = pti;
  /*
   * Mandatory - ESM cause code
   */
  esm_rsp_msg->deactivate_eps_bearer_context_request.esmcause = esm_cause;
  /*
   * Optional IEs
   */
  esm_rsp_msg->deactivate_eps_bearer_context_request.presencemask = 0;
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Send Deactivate EPS Bearer Context Request " "message (pti=%d, ebi=%d)\n",
      esm_rsp_msg->deactivate_eps_bearer_context_request.proceduretransactionidentity, esm_rsp_msg->deactivate_eps_bearer_context_request.epsbeareridentity);
  OAILOG_FUNC_OUT(LOG_NAS_ESM);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_eps_bearer_context_deactivate_request()          **
 **                                                                        **
 ** Description: Initiates the EPS bearer context deactivation procedure   **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.4.2                           **
 **      If a NAS signalling connection exists, the MME initiates  **
 **      the EPS bearer context deactivation procedure by sending  **
 **      a DEACTIVATE EPS BEARER CONTEXT REQUEST message to the    **
 **      UE, starting timer T3495 and entering state BEARER CON-   **
 **      TEXT INACTIVE PENDING.                                    **
 **                                                                        **
 ** Inputs:  is_standalone: Not used - Always true                     **
 **      ue_id:      UE lower layer identifier                  **
 **      ebi:       EPS bearer identity                        **
 **      msg:       Encoded ESM message to be sent             **
 **      ue_triggered:  true if the EPS bearer context procedure   **
 **             was triggered by the UE (not used)         **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_proc_eps_bearer_context_deactivate_request (
  mme_ue_s1ap_id_t ue_id,
  pti_t *pti,
  ebi_t *ebi,
  ESM_msg * esm_rsp_msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  nas_esm_proc_t * esm_base_proc = (nas_esm_proc_t*)_esm_proc_get_pdn_connectivity_procedure(ue_id, *pti);
  if(esm_base_proc){
    OAILOG_INFO(LOG_NAS_ESM, "ESM-PROC  - A PDN disconnection procedure found (pti=%d) for UE " MME_UE_S1AP_ID_FMT ".\n",
        *pti, ue_id);
    /* Start the timer (this should not be in detach case). */
    nas_stop_esm_timer(ue_id, &esm_base_proc->esm_proc_timer);
    /** Start the T3485 timer for additional PDN connectivity. */
    esm_base_proc->esm_proc_timer.id = nas_esm_timer_start (mme_config.nas_config.t3495_sec, 0, (void*)esm_base_proc); /**< Address field should be big enough to save an ID. */
    /* Set the timeout handler as the PDN Disconnection handler. */
    esm_base_proc->timeout_notif = _eps_bearer_deactivate_t3495_handler;
    /* Overwrite the EBI. */
    *ebi = ((nas_esm_proc_pdn_connectivity_t*)esm_base_proc)->default_ebi;
    *pti = esm_base_proc->pti;
  } else {
    /*
     * Check for PDN connectivity procedures.
     * Create a new EPS bearer context transaction and starts the timer, since no further CN operation necessary for dedicated bearers.
     */
    esm_base_proc = (nas_esm_proc_t*)_esm_proc_create_bearer_context_procedure(ue_id, *pti, ESM_EBI_UNASSIGNED, PDN_CONTEXT_IDENTIFIER_UNASSIGNED, *ebi,
        mme_config.nas_config.t3495_sec, 0 /*usec*/, _eps_bearer_deactivate_t3495_handler);
    DevAssert(esm_base_proc);
  }
  /*
   * Set the (default) bearer context of the PDN context into INACTIVE PENDING state.
   */
  if(mme_app_esm_modify_bearer_context(ue_id, *ebi, ESM_EBR_INACTIVE_PENDING, NULL, NULL, NULL) != ESM_CAUSE_SUCCESS){
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - No PDN connection found (ebi=%d, pti=%d) for UE " MME_UE_S1AP_ID_FMT ".\n",
        *ebi, *pti, ue_id);
    if(esm_base_proc->type == ESM_PROC_EPS_BEARER_CONTEXT)
      _esm_proc_free_bearer_context_procedure((nas_esm_proc_bearer_context_t**)&esm_base_proc);
    else
      _esm_proc_free_pdn_connectivity_procedure((nas_esm_proc_pdn_connectivity_t**)&esm_base_proc);

    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_PDN_CONNECTION_DOES_NOT_EXIST);
  }

  esm_send_deactivate_eps_bearer_context_request (
      *pti, *ebi,
      esm_rsp_msg,
      ESM_CAUSE_REGULAR_DEACTIVATION);

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
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
 ** Name:    _eps_bearer_deactivate_t3495_handler()                    **
 **                                                                        **
 ** Description: T3495 timeout handler                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 6.4.4.5, case a                   **
 **      On the first expiry of the timer T3495, the MME shall re- **
 **      send the DEACTIVATE EPS BEARER CONTEXT REQUEST and shall  **
 **      reset and restart timer T3495. This retransmission is     **
 **      repeated four times, i.e. on the fifth expiry of timer    **
 **      T3495, the MME shall abort the procedure and deactivate   **
 **      the EPS bearer context locally.                           **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void _eps_bearer_deactivate_t3495_handler (nas_esm_proc_t * esm_base_proc, ESM_msg * esm_rsp_msg)
{
  OAILOG_FUNC_IN(LOG_NAS_ESM);

  ebi_t bearer_ebi = ESM_EBI_UNASSIGNED;
  if(esm_base_proc->type == ESM_PROC_EPS_BEARER_CONTEXT){
    bearer_ebi = ((nas_esm_proc_bearer_context_t*)esm_base_proc)->bearer_ebi;
  }else {
    bearer_ebi = ((nas_esm_proc_pdn_connectivity_t*)esm_base_proc)->default_ebi;
  }

  OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - T3495 timer expired (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d), " "retransmission counter = %d\n",
      esm_base_proc->ue_id, bearer_ebi, esm_base_proc->retx_count);

  esm_base_proc->retx_count += 1;
  if (esm_base_proc->retx_count < EPS_BEARER_DEACTIVATE_COUNTER_MAX) {
    /*
     * Create a new ESM-Information request and restart the timer.
     * Keep the ESM transaction.
     */
    esm_send_deactivate_eps_bearer_context_request(esm_base_proc->pti, bearer_ebi, esm_rsp_msg, ESM_CAUSE_REGULAR_DEACTIVATION);
    // todo: ideally, the following should also be in a lock..
    /* Restart the T3486 timer. */
    nas_stop_esm_timer(esm_base_proc->ue_id, &esm_base_proc->esm_proc_timer);
    esm_base_proc->esm_proc_timer.id = nas_esm_timer_start(mme_config.nas_config.t3495_sec, 0 /*usec*/, (void*)esm_base_proc);
    esm_base_proc->timeout_notif = _eps_bearer_deactivate_t3495_handler;
    OAILOG_FUNC_OUT(LOG_NAS_ESM);
  }
  if(esm_base_proc->type == ESM_PROC_EPS_BEARER_CONTEXT){
    /*
     * Release the dedicated EPS bearer context and enter state INACTIVE
     */
    mme_app_release_bearer_context(esm_base_proc->ue_id, NULL, ESM_EBI_UNASSIGNED, bearer_ebi);
    /** Send a reject back to the MME_APP layer. */
    nas_itti_dedicated_eps_bearer_deactivation_complete(esm_base_proc->ue_id, bearer_ebi);
    /*
     * Release the transactional PDN connectivity procedure.
     */
    _esm_proc_free_bearer_context_procedure((nas_esm_proc_bearer_context_t**)&esm_base_proc);
    OAILOG_FUNC_OUT(LOG_NAS_ESM);
  }else {
    nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity = (nas_esm_proc_pdn_connectivity_t*)esm_base_proc;
    /*
     * Delete the PDN connectivity in the MME_APP UE context.
     */
    mme_app_esm_delete_pdn_context(esm_proc_pdn_connectivity->esm_base_proc.ue_id, esm_proc_pdn_connectivity->subscribed_apn, esm_proc_pdn_connectivity->pdn_cid, esm_proc_pdn_connectivity->default_ebi); /**< Frees it by putting it back to the pool. */
    _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_connectivity);
    OAILOG_FUNC_OUT(LOG_NAS_ESM);
  }
}

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/

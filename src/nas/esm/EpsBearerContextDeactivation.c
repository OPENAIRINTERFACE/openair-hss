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
#include "esm_ebr_context.h"
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
   --------------------------------------------------------------------------
   Internal data handled by the EPS bearer context deactivation procedure
   in the MME
   --------------------------------------------------------------------------
*/
/*
   Timer handlers
*/
static int
_eps_bearer_deactivate_t3492_handler(nas_esm_proc_t * esm_proc, ESM_msg *esm_resp_msg);

/* Maximum value of the deactivate EPS bearer context request
   retransmission counter */
#define EPS_BEARER_DEACTIVATE_COUNTER_MAX   5

static int _eps_bearer_release (    esm_context_t * esm_context, ebi_t ebi, pdn_cid_t *pid);


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
  int esm_cause)
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

/*
   --------------------------------------------------------------------------
    EPS bearer context deactivation procedure executed by the MME
   --------------------------------------------------------------------------
*/
    /*
     * Locally releasing the bearers and MME bearer contexts without asking the UE.
     * Will check for default bearer inside the method.
     */
    rc = _eps_bearer_release (esm_context, ebi, &pid); /**< Implicitly removing the PDN context, if the default bearer is removed. */
    /** Return after releasing. */
//    if(!pdn_context->is_active){
//      // todo: check that only 1 deactivate message for the default bearer exists
//      *esm_cause = ESM_CAUSE_INSUFFICIENT_RESOURCES;
//      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - We released  the default EBI. Deregistering the PDN context (E-RAB failure). (ebi=%d,pid=%d)\n", ebi,pid);
//      rc = esm_proc_pdn_disconnect_accept (esm_context, pdn_context->context_identifier, ebi, esm_cause); /**< Delete Session Request is already sent at the beginning. We don't care for the response. */
//    }
  /** Will continue with sending the bearer deactivation request. */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
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
  const ebi_t ebi,
  nas_esm_pdn_connectivity_proc_t * esm_pdn_disconnect_proc)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Initiate EPS bearer context deactivation " "(ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d)\n", ue_id, ebi);

  /*
   * Set the default bearer contexts of the PDN context into INACTIVE PENDING state.
   */
  int rc = mme_app_esm_update_pdn_context(ue_id, NULL, PDN_CONTEXT_IDENTIFIER_UNASSIGNED, esm_pdn_disconnect_proc->default_ebi, ESM_EBR_INACTIVE_PENDING, NULL, NULL, NULL);
  if(rc == RETURNerror){
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - No PDN connection found (ebi=%d, pti=%d) for UE " MME_UE_S1AP_ID_FMT ".\n", default_ebi, pti, ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_PDN_CONNECTION_DOES_NOT_EXIST);
  }

  /*
   * Send deactivate EPS bearer context request message and
   * * * * start timer T3495/T3492
   */
  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Starting T3492 for Deactivate Default EPS bearer context (ue_id=" MME_UE_S1AP_ID_FMT ", context_identifier=%d)\n",
      ue_id, esm_pdn_disconnect_proc->pdn_cid);
  /** Stop any timer if running. */
  nas_stop_esm_timer(ue_id, &esm_pdn_disconnect_proc->trx_base_proc.esm_proc.esm_proc_timer);
  /** Start the T3485 timer for additional PDN connectivity. */
  esm_pdn_disconnect_proc->trx_base_proc.esm_proc.esm_proc_timer.id = nas_timer_start (esm_pdn_disconnect_proc->trx_base_proc.esm_proc.esm_proc_timer.sec, 0 /*usec*/, TASK_NAS_ESM,
      _nas_proc_pdn_connectivity_timeout_handler, ue_id); /**< Address field should be big enough to save an ID. */
  nas_timer_start(esm_pdn_disconnect_proc->trx_base_proc.esm_proc_timer);
  esm_pdn_disconnect_proc->trx_base_proc.esm_proc.timeout_notif = _eps_bearer_deactivate_t3492_handler;
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_eps_bearer_context_deactivate_accept()           **
 **                                                                        **
 ** Description: Performs EPS bearer context deactivation procedure accep- **
 **      ted by the UE.                                            **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.4.3                           **
 **      Upon receipt of the DEACTIVATE EPS BEARER CONTEXT ACCEPT  **
 **      message, the MME shall enter the state BEARER CONTEXT     **
 **      INACTIVE and stop the timer T3495.                        **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **      ebi:       EPS bearer identity                        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    The identifier of the PDN connection to be **
 **             released, if it exists;                    **
 **             RETURNerror otherwise.                     **
 **      Others:    T3495                                      **
 **                                                                        **
 ***************************************************************************/
pdn_cid_t
esm_proc_eps_bearer_context_deactivate_accept (
  esm_context_t * esm_context,
  ebi_t ebi,
  esm_cause_t *esm_cause)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  pdn_cid_t                               pid = MAX_APN_PER_UE;
  mme_ue_s1ap_id_t      ue_id = esm_context->ue_id;

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - EPS bearer context deactivation " "accepted by the UE (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d)\n",
      ue_id, ebi);

  /*
   * Release the EPS bearer context.
   */
  rc = _eps_bearer_release (esm_context, ebi, &pid);

  if (rc != RETURNok) {
    /*
     * Failed to release the EPS bearer context
     */
    *esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
    pid = RETURNerror;
  }

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, pid);
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
static void _eps_bearer_deactivate_t3492_handler (nas_esm_proc_t * esm_proc, ESM_msg * esm_rsp_msg)
{

  OAILOG_FUNC_IN(LOG_NAS_ESM);

  OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - T3492 timer expired (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d), " "retransmission counter = %d\n",
       esm_pdn_disconnect_proc->trx_base_proc.esm_proc.ue_id, esm_pdn_disconnect_proc->default_ebi, esm_pdn_disconnect_proc->trx_base_proc.retx_count);

  pdn_context_t * pdn_context = NULL;
  mme_app_get_pdn_context(esm_pdn_disconnect_proc->trx_base_proc.esm_proc.ue_id, esm_pdn_disconnect_proc->pdn_cid,
      esm_pdn_disconnect_proc->default_ebi, esm_pdn_disconnect_proc->apn_subscribed, &pdn_context);

  esm_pdn_disconnect_proc->trx_base_proc.retx_count += 1;
  if (esm_pdn_disconnect_proc->trx_base_proc.retx_count < EPS_BEARER_DEACTIVATE_COUNTER_MAX) {
    /*
     * Create a new ESM-Information request and restart the timer.
     * Keep the ESM transaction.
     */
    if(pdn_context){
      bearer_context_t * bearer_context = NULL;
      mme_app_get_session_bearer_context(pdn_context, esm_pdn_disconnect_proc->default_ebi);
      if(bearer_context){
        rc = esm_send_deactivate_eps_bearer_context_request(esm_pdn_disconnect_proc->trx_base_proc.esm_proc.pti,
            esm_pdn_disconnect_proc->default_ebi, esm_rsp_msg, ESM_CAUSE_REGULAR_DEACTIVATION);
        if (rc != RETURNerror) {
          rc = esm_proc_eps_bearer_context_deactivate_request (esm_pdn_disconnect_proc->trx_base_proc.esm_proc.ue_id, esm_pdn_disconnect_proc->default_ebi, esm_pdn_disconnect_proc);
          OAILOG_FUNC_RETURN(LOG_NAS_ESM, rc);
        }
      }
    }
  }

  OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - T3492 timer expired (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d), " "retransmission counter = %d\n",
       esm_pdn_disconnect_proc->trx_base_proc.esm_proc.ue_id, esm_pdn_disconnect_proc->default_ebi, esm_pdn_disconnect_proc->trx_base_proc.retx_count);

  /*
   * Check if it is the default EBI, if so remove the PDN context.
   */
  if(esm_proc->type == ESM_PROC_PDN_CONTEXT){
    nas_esm_pdn_connectivity_proc_t * esm_pdn_disconnect_proc = (nas_esm_pdn_connectivity_proc_t*)esm_proc;
    /* Deactivate the bearer/pdn context implicitly. */
    esm_proc_pdn_disconnect_accept(esm_pdn_disconnect_proc->trx_base_proc.esm_proc.ue_id, esm_pdn_disconnect_proc->pdn_cid, esm_pdn_disconnect_proc->default_ebi,
        esm_pdn_disconnect_proc->apn_subscribed);

    _esm_proc_free_pdn_connectivity_procedure(&esm_pdn_disconnect_proc);
    OAILOG_FUNC_OUT (LOG_NAS_ESM);
  } else {
    nas_esm_bearer_context_proc_t * esm_bearer_context_proc = (nas_esm_bearer_context_proc_t*)esm_proc;
    /** Bearer Context Procedures may be transactional or not. */
    /* Deactivate the bearer/pdn context implicitly. */
    esm_proc_eps_bearer_context_deactivate_accept(esm_bearer_context_proc->trx_base_proc.esm_proc.ue_id, esm_bearer_context_proc->pdn_cid,
        esm_bearer_context_proc->bearer_ebi, esm_bearer_context_proc->apn);
    _esm_proc_free_bearer_context_procedure(&esm_bearer_context_proc);
  }
  OAILOG_FUNC_OUT (LOG_NAS_ESM);
}

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/

/****************************************************************************
 **                                                                        **
 ** Name:    _eps_bearer_release()                                     **
 **                                                                        **
 ** Description: Releases the EPS bearer context identified by the given   **
 **      EPS bearer identity and enters state INACTIVE.            **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **      ebi:       EPS bearer identity                        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     pid:       Identifier of the PDN connection the EPS   **
 **             bearer belongs to                          **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int
_eps_bearer_release (
  esm_context_t * esm_context,
  ebi_t ebi,
  pdn_cid_t *pid)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;


  if (ebi == ESM_EBI_UNASSIGNED) {
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Failed to release EPS bearer context\n");
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
  }
  if ((ebi < ESM_EBI_MIN) || (ebi > ESM_EBI_MAX)) {
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
  }

  /*
   * Release and the contexts and update the counters.
   */
  ebi = esm_ebr_context_release (esm_context, ebi, pid, false);
  if (ebi == ESM_EBI_UNASSIGNED) {
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Failed to release EPS bearer context\n");
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
  }

  /* Successfully released the context for EIB. */
  OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Successfully released EPS bearer data for ebi %d and pid %d. \n", ebi, pid);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
}

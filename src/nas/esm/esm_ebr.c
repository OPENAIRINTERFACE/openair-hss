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
  Source      esm_ebr.c

  Version     0.1

  Date        2013/01/29

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Frederic Maurel

  Description Defines functions used to handle state of EPS bearer contexts
        and manage ESM messages re-transmission.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "log.h"
#include "msc.h"
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "assertions.h"
#include "common_defs.h"
#include "mme_app_ue_context.h"
#include "mme_api.h"
#include "mme_app_defs.h"
#include "emm_data.h"
#include "esm_ebr_context.h"
#include "esm_ebr.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/


#define ESM_EBR_NB_UE_MAX   (MME_API_NB_UE_MAX + 1)

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* String representation of EPS bearer context status */
static const char                      *_esm_ebr_state_str[ESM_EBR_STATE_MAX] = {
  "BEARER CONTEXT INACTIVE",
  "BEARER CONTEXT ACTIVE",
  "BEARER CONTEXT INACTIVE PENDING",
  "BEARER CONTEXT MODIFY PENDING",
  "BEARER CONTEXT ACTIVE PENDING"
};

/*
   ----------------------
   User notification data
   ----------------------
*/


///* Returns the index of the next available entry in the list of EPS bearer
//   context data */
//static int                              _esm_ebr_get_available_entry (esm_context_t * esm_context);


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

//------------------------------------------------------------------------------
const char * esm_ebr_state2string(esm_ebr_state esm_ebr_state)
{
  switch (esm_ebr_state) {
    case ESM_EBR_INACTIVE:
      return "ESM_EBR_INACTIVE";
    case ESM_EBR_ACTIVE:
      return "ESM_EBR_ACTIVE";
    case ESM_EBR_INACTIVE_PENDING:
      return "ESM_EBR_INACTIVE_PENDING";
    case ESM_EBR_MODIFY_PENDING:
      return "ESM_EBR_MODIFY_PENDING";
    case ESM_EBR_ACTIVE_PENDING:
      return "ESM_EBR_ACTIVE_PENDING";
    default:
      return "UNKNOWN";
  }
}
/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_release()                                         **
 **                                                                        **
 ** Description: Release the given EPS bearer context and its ESM          **
 **               procedure, if existing.                                  **
 **                                                                        **
 ** Inputs:  ue_id:      Lower layers UE identifier                 **
 **      ebi:       The identity of the EPS bearer context to  **
 **             be released                                **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok if the EPS bearer context has     **
 **             been successfully released;                **
 **             RETURNerror otherwise.                     **
 **      Others:    _esm_ebr_data                              **
 **                                                                        **
 ***************************************************************************/
int esm_ebr_release (mme_ue_s1ap_id_t ue_id, bstring apn, pdn_cid_t pdn_cid, ebi_t ebi)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_ebr_context_t                      *ebr_ctx = NULL;

  /*
   * Stop the running bearer context procedure (incl. timer) for the specific EBI.
   * Other processes should not be affected.
   */
  nas_esm_bearer_context_proc_t * esm_bearer_context_proc = esm_data_get_bearer_procedure(ue_id, ebi);
  if(esm_bearer_context_proc){
    OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Removing ESM bearer procedure for released bearer context (ebi=%d) for UE " MME_UE_S1AP_ID_FMT". \n", ue_id, ebi);
    /** This will also stop the timer of the procedure, no extra bearer timer is needed. */
    // todo: locks needed in ESM?
    _esm_proc_free_bearer_context_procedure(esm_bearer_context_proc);
  }else {
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - No ESM bearer procedure exists for bearer context (ebi=%d) for UE " MME_UE_S1AP_ID_FMT". \n", ue_id, ebi);
  }
  /*
   * Deregister the single bearer context from the PDN context.
   */
  rc = mme_app_deregister_bearer_context(ue_id, ebi, pdn_context);
  if(rc != RETURNok){
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Failed to deregister the EPS bearer context from the pdn context for ebi %d for ueId " MME_UE_S1AP_ID_FMT ". \n", bearer_context->ebi, esm_context->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
  }

  OAILOG_INFO (LOG_NAS_ESM, "ESM-FSM   - EPS bearer context %d released for ueId " MME_UE_S1AP_ID_FMT ". \n", bearer_context->ebi, esm_context->ue_id);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_start_timer()                                     **
 **                                                                        **
 ** Description: Start the timer of the specified EPS bearer context to    **
 **      expire after a given time interval. Timer expiration will **
 **      schedule execution of the callback function where stored  **
 **      ESM message should be re-transmit.                        **
 **                                                                        **
 ** Inputs:  ue_id:      Lower layers UE identifier                 **
 **      ebi:       The identity of the EPS bearer             **
 **      msg:       The encoded ESM message to be stored       **
 **      sec:       The value of the time interval in seconds  **
 **      cb:        Function executed upon timer expiration    **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _esm_ebr_data                              **
 **                                                                        **
 ***************************************************************************/
int esm_ebr_start_timer (esm_context_t * esm_context, ebi_t ebi,
  CLONE_REF const_bstring msg,
  long sec,
  nas_timer_callback_t cb)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_ebr_context_t                      *ebr_ctx = NULL;
  bearer_context_t                       *bearer_context = NULL;


  if ((ebi < ESM_EBI_MIN) || (ebi > ESM_EBI_MAX)) {
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-FSM   - Retransmission timer bad ebi %d\n", ebi);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
  }

  ue_context_t                        *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, esm_context->ue_id);

  /*
   * Get EPS bearer context data
   */
  mme_app_get_session_bearer_context_from_all(ue_context, ebi, &bearer_context);

  if ((bearer_context == NULL) || (bearer_context->ebi != ebi)) {
    /*
     * EPS bearer context not assigned
     */
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-FSM   - EPS bearer context not assigned\n");
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
  }
  ebr_ctx = &bearer_context->esm_ebr_context;

  esm_ebr_timer_data_t * esm_ebr_timer_data = NULL;
  if (ebr_ctx->timer.id != NAS_TIMER_INACTIVE_ID) {
    /*
     * Re-start the retransmission timer
     */
    ebr_ctx->timer.id = nas_timer_stop (ebr_ctx->timer.id, (void**)&esm_ebr_timer_data);
    ebr_ctx->timer.id = nas_timer_start (sec, 0 /* usec */, cb, esm_ebr_timer_data);
    MSC_LOG_EVENT (MSC_NAS_ESM_MME, "0 Timer %x ebi %u restarted", ebr_ctx->timer.id, ebi);
  } else {
    /*
     * Setup the retransmission timer parameters
     */
    esm_ebr_timer_data = (esm_ebr_timer_data_t *) calloc (1, sizeof (esm_ebr_timer_data_t));

    if (esm_ebr_timer_data) {
      /*
       * Set the UE identifier
       */
      esm_ebr_timer_data->ue_id = ue_context->mme_ue_s1ap_id;
      esm_ebr_timer_data->ctx = esm_context;
      /*
       * Set the EPS bearer identity
       */
      esm_ebr_timer_data->ebi = ebi;
      /*
       * Reset the retransmission counter
       */
      esm_ebr_timer_data->count = 0;
      /*
       * Set the ESM message to be re-transmited
       */
      esm_ebr_timer_data->msg = bstrcpy (msg);

      /*
       * Setup the retransmission timer to expire at the given
       * * * * time interval
       */
      ebr_ctx->timer.id = nas_timer_start (sec, 0 /* usec */, cb, esm_ebr_timer_data);
      MSC_LOG_EVENT (MSC_NAS_ESM_MME, "0 Timer %x ebi %u started", ebr_ctx->timer.id, ebi);
      ebr_ctx->timer.sec = sec;
    }
  }

  if ((esm_ebr_timer_data) && (ebr_ctx->timer.id != NAS_TIMER_INACTIVE_ID)) {
    OAILOG_INFO (LOG_NAS_ESM, "ESM-FSM   - Retransmission timer %ld expires in " "%ld seconds\n", ebr_ctx->timer.id, ebr_ctx->timer.sec);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
  } else {
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-FSM   - esm_ebr_timer_data == NULL(%p) or ebr_ctx->timer.id == NAS_TIMER_INACTIVE_ID == -1 (%ld)\n",
        esm_ebr_timer_data, ebr_ctx->timer.id);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
}

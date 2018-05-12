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

#include "log.h"
#include "dynamic_memory_check.h"
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "mme_app_ue_context.h"
#include "esm_proc.h"
#include "commonDef.h"
#include "emm_data.h"
#include "esm_data.h"
#include "esm_cause.h"
#include "esm_ebr.h"
#include "esm_ebr_context.h"

#include "emm_sap.h"
#include "esm_sap.h"

#include "mme_config.h"
#include "mme_app_defs.h"
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
static void _eps_bearer_deactivate_t3495_handler (void *);

/* Maximum value of the deactivate EPS bearer context request
   retransmission counter */
#define EPS_BEARER_DEACTIVATE_COUNTER_MAX   5

static int _eps_bearer_deactivate (emm_data_context_t * ue_context, ebi_t ebi, STOLEN_REF bstring *msg);
static int _eps_bearer_release (emm_data_context_t * ue_context, ebi_t ebi, pdn_cid_t *pid);


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
    EPS bearer context deactivation procedure executed by the MME
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_eps_bearer_context_deactivate()                  **
 **                                                                        **
 ** Description: Locally releases the EPS bearer context identified by the **
 **      given EPS bearer identity, without peer-to-peer signal-   **
 **      ling between the UE and the MME, or checks whether an EPS **
 **      bearer context with specified EPS bearer identity has     **
 **      been activated for the given UE.                          **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      is local:  true if the EPS bearer context has to be   **
 **             locally released without peer-to-peer si-  **
 **             gnalling between the UE and the MME        **
 **      ebi:       EPS bearer identity of the EPS bearer con- **
 **             text to be deactivated                     **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     pid:       Identifier of the PDN connection the EPS   **
 **             bearer belongs to                          **
 **      bid:       Identifier of the released EPS bearer con- **
 **             text entry                                 **
 **      esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
esm_proc_eps_bearer_context_deactivate (
  emm_data_context_t * const emm_context,
  const bool is_local,
  const ebi_t ebi,
  pdn_cid_t pid,
  esm_cause_t * const esm_cause)
{

  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  ue_context_t                           *ue_context  = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);
  DevAssert(ue_context);

  /** Get the PDN Context. */
  pdn_context_t                          *pdn_context = mme_app_get_pdn_context(ue_context, pdn_context, ESM_EBI_UNASSIGNED, NULL);
  DevAssert(pdn_context);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - EPS default bearer context deactivation " "(ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d)\n", ue_context->mme_ue_s1ap_id, ebi);

  if (is_local) {
    if (ebi != ESM_SAP_ALL_EBI && ebi != pdn_context->default_ebi) {
      /*
       * Locally release the specified EPS bearer context
       */
      rc = _eps_bearer_release (ue_context, ebi, pid);
    } else {
      /*
       * Locally release all the PDN context.
       * No NAS ITTI S11 Delete Session Request message will be sent (we must have removed them already, with PDN Disconnect Request processing).
       */
      rc = _pdn_context_release (ue_context, pdn_context, pid);
      if (rc != RETURNok) {
        break;
      }
    }
    /** Return after releasing. */
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
  }

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - EPS bearer context deactivation " "(ue_id=" MME_UE_S1AP_ID_FMT ", pid=%d, ebi=%d)\n", ue_mm_context->mme_ue_s1ap_id, pid, ebi);

  if ((ue_context ) && (pid < MAX_APN_PER_UE)) {
    if (!pdn_context) {
      OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - PDN connection %d has not been " "allocated\n", pid);
      *esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
    } else {
      int                                     i;

      *esm_cause = ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY;

      /** todo: validate the session bearers of the PDN context. */
      bearer_context_t *session_bearer = NULL;
      RB_FOREACH(session_bearer, BearerPool, &pdn_context->session_bearers){
        DevAssert(session_bearer->pdn_cx_id == pid && session_bearer->esm_ebr_context.status == ESM_EBR_ACTIVE);
        /*
         * todo: validate a single bearer!
         * todo: better, more meaningful validation
         * The EPS bearer context to be released is valid.
         */
        *esm_cause = ESM_CAUSE_SUCCESS;
        rc = RETURNok;
      }
    }
  }
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
int
esm_proc_eps_bearer_context_deactivate_request (
  const bool is_standalone,
  emm_data_context_t * const ue_context,
  const ebi_t ebi,
  STOLEN_REF bstring *msg,
  const bool ue_triggered)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc;
  mme_ue_s1ap_id_t      ue_id = ue_context->ue_id;

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Initiate EPS bearer context deactivation " "(ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d)\n",
      ue_id, ebi);
  /*
   * Send deactivate EPS bearer context request message and
   * * * * start timer T3495
   */
  rc = _eps_bearer_deactivate (ue_context, ebi, msg);
  msg = NULL;

  if (rc != RETURNerror) {
    /*
     * Set the EPS bearer context state to INACTIVE PENDING
     */
    rc = esm_ebr_set_status (ue_context, ebi, ESM_EBR_INACTIVE_PENDING, ue_triggered);

    if (rc != RETURNok) {
      /*
       * The EPS bearer context was already in INACTIVE state
       */
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - EBI %d was already INACTIVE PENDING\n", ebi);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
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
  emm_data_context_t * ue_context,
  ebi_t ebi,
  esm_cause_t *esm_cause)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  pdn_cid_t                               pid = MAX_APN_PER_UE;
  mme_ue_s1ap_id_t      ue_id = ue_context->ue_id;

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - EPS bearer context deactivation " "accepted by the UE (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d)\n",
      ue_id, ebi);

  if (rc != RETURNerror) {
    int                                   bid = BEARERS_PER_UE;

    /*
     * Release the EPS bearer context.
     */
    rc = _eps_bearer_release (ue_context, ebi, &pid, &bid);

    if (rc != RETURNok) {
      /*
       * Failed to release the EPS bearer context
       */
      *esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
      pid = RETURNerror;
    }
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
static void _eps_bearer_deactivate_t3495_handler (void *args)
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
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - T3495 timer expired (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d), " "retransmission counter = %d\n",
        esm_ebr_timer_data->ue_id, esm_ebr_timer_data->ebi, esm_ebr_timer_data->count);

    if (esm_ebr_timer_data->count < EPS_BEARER_DEACTIVATE_COUNTER_MAX) {
      /*
       * Re-send deactivate EPS bearer context request message to the UE
       */
      bstring b = bstrcpy(esm_ebr_timer_data->msg);
      rc = _eps_bearer_deactivate (esm_ebr_timer_data->ctx, esm_ebr_timer_data->ebi, &b);
    } else {
      /*
       * The maximum number of deactivate EPS bearer context request
       * message retransmission has exceed
       */
      pdn_cid_t                               pid = MAX_APN_PER_UE;
      int                                     bid = BEARERS_PER_UE;

      /*
       * Deactivate the EPS bearer context locally without peer-to-peer
       * * * * signalling between the UE and the MME
       */
      rc = _eps_bearer_release (esm_ebr_timer_data->ctx, esm_ebr_timer_data->ebi, &pid, &bid);

      if (rc != RETURNerror) {
        /*
         * Stop timer T3495
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

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/

/****************************************************************************
 **                                                                        **
 ** Name:    _eps_bearer_deactivate()                                  **
 **                                                                        **
 ** Description: Sends DEACTIVATE EPS BEREAR CONTEXT REQUEST message and   **
 **      starts timer T3495                                        **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **      ebi:       EPS bearer identity                        **
 **      msg:       Encoded ESM message to be sent             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3495                                      **
 **                                                                        **
 ***************************************************************************/
static int
_eps_bearer_deactivate (
  emm_data_context_t * ue_context,
  ebi_t ebi,
  STOLEN_REF bstring *msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  emm_sap_t                               emm_sap = {0};
  int                                     rc;
  mme_ue_s1ap_id_t                        ue_id = ue_context->ue_id;

  /*
   * Notify EMM that a deactivate EPS bearer context request message
   * has to be sent to the UE
   */
  emm_esm_data_t                         *emm_esm = &emm_sap.u.emm_esm.u.data;

  emm_sap.primitive = EMMESM_UNITDATA_REQ;
  emm_sap.u.emm_esm.ue_id = ue_id;
  emm_sap.u.emm_esm.ctx = ue_context;
  emm_esm->msg = *msg;
  bstring msg_dup = bstrcpy(*msg);
  *msg = NULL;
  MSC_LOG_TX_MESSAGE (MSC_NAS_ESM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMESM_UNITDATA_REQ (bearer deactivate) ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
  rc = emm_sap_send (&emm_sap);

  if (rc != RETURNerror) {
    /*
     * Start T3495 retransmission timer
     */
    rc = esm_ebr_start_timer (ue_context, ebi, msg_dup, mme_config.nas_config.t3495_sec, _eps_bearer_deactivate_t3495_handler);
  }else {
    bdestroy_wrapper(&msg_dup);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

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
  emm_data_context_t * emm_context,
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
  ebi = esm_ebr_context_release (emm_context, ebi, pid);
  if (ebi == ESM_EBI_UNASSIGNED) {
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Failed to release EPS bearer context\n");
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
  }

  /* Successfully released the context for EIB. */
  OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Successfully released EPS bearer data for ebi %d and pid %d. \n", ebi, pid);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

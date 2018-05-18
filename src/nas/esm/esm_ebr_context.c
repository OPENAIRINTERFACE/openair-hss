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
  Source      esm_ebr_context.h

  Version     0.1

  Date        2013/05/28

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Frederic Maurel

  Description Defines functions used to handle EPS bearer contexts.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "obj_hashtable.h"
#include "log.h"
#include "msc.h"
#include "gcc_diag.h"
#include "dynamic_memory_check.h"
#include "assertions.h"
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "mme_app_ue_context.h"
#include "mme_app_bearer_context.h"
#include "common_defs.h"
#include "commonDef.h"
#include "emm_data.h"
#include "esm_ebr.h"
#include "esm_ebr_context.h"
#include "emm_sap.h"
#include "mme_app_defs.h"


/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_context_create()                                  **
 **                                                                        **
 ** Description: Creates a new EPS bearer context to the PDN with the spe- **
 **      cified PDN connection identifier                          **
 **                                                                        **
 ** Inputs:  ue_id:      UE identifier                              **
 **      pid:       PDN connection identifier                  **
 **      ebi:       EPS bearer identity                        **
 **      is_default:    true if the new bearer is a default EPS    **
 **             bearer context                             **
 **      esm_qos:   EPS bearer level QoS parameters            **
 **      tft:       Traffic flow template parameters           **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The EPS bearer identity of the default EPS **
 **             bearer associated to the new EPS bearer    **
 **             context if successfully created;           **
 **             UNASSIGN EPS bearer value otherwise.       **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ***************************************************************************/
ebi_t
esm_ebr_context_create (
  emm_data_context_t * emm_context,
  const proc_tid_t pti,
  pdn_cid_t pid,
  ebi_t ebi,
  struct fteid_set_s * fteid_set,
  bool is_default,
  const bearer_qos_t *bearer_level_qos,
  traffic_flow_template_t * tft,
  protocol_configuration_options_t * pco)
{
  int                                     bidx = 0;
  esm_context_t                          *esm_ctx = NULL;
  esm_pdn_t                              *pdn = NULL;
  pdn_context_t                          *pdn_context = NULL;

  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_ctx = &emm_context->esm_ctx;
  ue_context_t                        *ue_context  = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);

  pdn_context = mme_app_get_pdn_context(ue_context, pid); // todo: invalid/default values for other IDs..
  DevAssert(pdn_context);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Create new %s EPS bearer context (ebi=%d) " "for PDN connection (pid=%d)\n",
      (is_default) ? "default" : "dedicated", ebi, pid);

  /** Get the PDN Session for the UE. */
  if (pid < MAX_APN_PER_UE) {
    pdn_context = mme_app_get_pdn_context(ue_context, pid, ESM_EBI_UNASSIGNED, NULL);
    if (!pdn_context) {
      OAILOG_ERROR(LOG_NAS_ESM , "ESM-PROC  - PDN connection %d has not been " "allocated\n", pid);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
    }
    /*
     * Check the total number of active EPS bearers
     * No maximum number of bearers per UE.
     */
    else if (esm_ctx->n_active_ebrs > BEARERS_PER_UE) {
      OAILOG_WARNING (LOG_NAS_ESM , "ESM-PROC  - The total number of active EPS" "bearers is exceeded\n");
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
    }
    /*
     * Check that the EBI is available (no already allocated).
     */
  }else{
    OAILOG_WARNING (LOG_NAS_ESM , "ESM-PROC  - Failed to create new EPS bearer " "context (invalid_ebi=%d)\n", ebi);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
  }

  /*
   * Get the PDN connection entry
   */
  pdn = &pdn_context->esm_data;

  // todo: moved to esm_ebr_assign
//  /*
//   * Until here only validation.
//   * Create new EPS bearer context.
//   */
//  if(mme_app_register_bearer_context(ue_context, ebi, pdn_context) == RETURNerror){
//    /** Error registering a new bearer context into the pdn session. */
//    OAILOG_ERROR(LOG_NAS_ESM , "ESM-PROC  - A EPS bearer context could not be allocated from the bearer pool into the session pool of the pdn context. \n");
//    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
//  }
  bearer_context_t * bearer_context = mme_app_get_session_bearer_context(pdn_context, ebi);
  if(bearer_context){
    MSC_LOG_EVENT (MSC_NAS_ESM_MME, "0 Registered Bearer ebi %u cid %u pti %u", ebi, pid, pti);
    bearer_context->transaction_identifier = pti;
    /*
     * Increment the total number of active EPS bearers
     */
    esm_ctx->n_active_ebrs += 1;
    /*
     * Increment the number of EPS bearer for this PDN connection
     */
    pdn->n_bearers += 1;
    /*
     * Setup the EPS bearer data
     */

    bearer_context->qci = bearer_level_qos->qci;
    bearer_context->esm_ebr_context.gbr_dl = bearer_level_qos->gbr.br_dl;
    bearer_context->esm_ebr_context.gbr_ul = bearer_level_qos->gbr.br_ul;
    bearer_context->esm_ebr_context.mbr_dl = bearer_level_qos->mbr.br_dl;
    bearer_context->esm_ebr_context.mbr_ul = bearer_level_qos->mbr.br_ul;

    if (bearer_context->esm_ebr_context.tft) {
      free_traffic_flow_template(&bearer_context->esm_ebr_context.tft);
    }
    bearer_context->esm_ebr_context.tft = tft;

    if (bearer_context->esm_ebr_context.pco) {
      free_protocol_configuration_options(&bearer_context->esm_ebr_context.pco);
    }
    bearer_context->esm_ebr_context.pco = pco;

    /*
     * Set the FTEIDs.
     * It looks ugly here, but we don't have the EBI in the MME_APP when S11 CBR is received, and need also don't have the ebi's there yet.
     */

    //    Assert(!RB_INSERT (BearerFteids, &s11_proc_create_bearer->fteid_set, fteid_set)); /**< Find the correct FTEID later by using the S1U FTEID as key.. */
    memcpy((void*)&bearer_context->s_gw_fteid_s1u , &fteid_set->s1u_fteid, sizeof(fteid_t));
    memcpy((void*)&bearer_context->p_gw_fteid_s5_s8_up , &fteid_set->s5_fteid, sizeof(fteid_t));
    // Todo: we cannot store them in a map, because when we evaluate the response, EBI is our key, which is not set here. That's why, we need to forward it to ESM.

    /** Set the MME_APP states (todo: may be with Activate Dedicated Bearer Response). */
    bearer_context->bearer_state   |= BEARER_STATE_SGW_CREATED;
    bearer_context->bearer_state   |= BEARER_STATE_MME_CREATED;

    if (is_default) {
      /*
       * Set the PDN connection activation indicator
       */
      pdn_context->is_active = true;

      pdn_context->default_ebi = ebi;
      /*
       * Update the emergency bearer services indicator
       */
      if (pdn->is_emergency) {
        esm_ctx->is_emergency = true;
      }
    }

    /*
     * Return the EPS bearer identity of the default EPS bearer
     * * * * associated to the new EPS bearer context
     */
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, bearer_context->ebi);
  }
}

//------------------------------------------------------------------------------
void esm_ebr_context_init (esm_ebr_context_t *esm_ebr_context)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  if (esm_ebr_context)  {
    memset(esm_ebr_context, 0, sizeof(*esm_ebr_context));
    /*
     * Set the EPS bearer context status to INACTIVE
     */
    esm_ebr_context->status = ESM_EBR_INACTIVE;
    /*
     * Disable the retransmission timer
     */
    esm_ebr_context->timer.id = NAS_TIMER_INACTIVE_ID;
  }
  OAILOG_FUNC_OUT (LOG_NAS_ESM);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_context_release()                                 **
 **                                                                        **
 ** Description: Releases EPS bearer context entry previously allocated    **
 **      to the EPS bearer with the specified EPS bearer identity  **
 **                                                                        **
 ** Inputs:  ue_id:      UE identifier                              **
 **      ebi:       EPS bearer identity                        **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     pid:       Identifier of the PDN connection entry the **
 **             EPS bearer context belongs to              **
 **      bid:       Identifier of the released EPS bearer con- **
 **             text entry                                 **
 **      Return:    The EPS bearer identity associated to the  **
 **             EPS bearer context if successfully relea-  **
 **             sed; UNASSIGN EPS bearer value otherwise.  **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ***************************************************************************/
ebi_t
esm_ebr_context_release (
  emm_data_context_t * emm_context,
  ebi_t ebi,
  pdn_cid_t *pid,
  bool ue_requested)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     found = false;
  esm_pdn_t                              *pdn = NULL;

  ue_context_t                        *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);
  pdn_context_t                       *pdn_context = NULL;
  bearer_context_t                    *bearer_context = NULL;

  if (ebi != ESM_EBI_UNASSIGNED) {
    if ((ebi < ESM_EBI_MIN) || (ebi > ESM_EBI_MAX)) {
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
    }
  }else{
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
  }

  if(*pid != NULL){
    pdn_context = mme_app_get_pdn_context(ue_context, *pid, ebi);
  }else{
    /** Get the bearer context from all session bearers. */
    bearer_context = mme_app_get_session_bearer_context_from_all(ue_context, ebi);
    if(!bearer_context){
      OAILOG_ERROR(LOG_NAS_ESM , "ESM-PROC  - Could not find PDN context from ebi %d. \n", ebi);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
    }
    pdn_context = mme_app_get_pdn_context(ue_context, bearer_context->pdn_cx_id, ebi);
    *pid = bearer_context->pdn_cx_id;
  }

  /** At this point, we must have found the pdn_context. */
  if(!pdn_context){
    OAILOG_ERROR(LOG_NAS_ESM , "ESM-PROC  - PDN connection identifier %d " "is not valid\n", *pid);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
  }

  pdn = &pdn_context->esm_data;
  /*
   * Check if it is a default ebi, if so release all the bearer contexts.
   * If not, just release a single bearer context.
   */
  if (ebi == pdn_context->default_ebi) {
    /*
     * 3GPP TS 24.301, section 6.4.4.3, 6.4.4.6
     * * * * If the EPS bearer identity is that of the default bearer to a
     * * * * PDN, the UE shall delete all EPS bearer contexts associated to
     * * * * that PDN connection.
     */
    RB_FOREACH(bearer_context, SessionBearers, &pdn_context->session_bearers){
      OAILOG_WARNING (LOG_NAS_ESM , "ESM-PROC  - Release EPS bearer context " "(ebi=%d, pid=%d)\n", bearer_context->ebi, *pid);
      /*
       * Delete the TFT
       */
      // TODO Look at "free_traffic_flow_template"
      //free_traffic_flow_template(&pdn->bearer[i]->tft);

      /*
       * Set the EPS bearer context state to INACTIVE
       */
      int rc  = esm_ebr_release (ue_context, bearer_context, pdn_context, ue_requested);
      if (rc != RETURNok) {
         /*
          * Failed to update ESM bearer status.
          */
         OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Failed to update ESM bearer status. \n");
         OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
      }

      pdn->n_bearers -= 1;

      // todo: update esm context
      emm_context->esm_ctx.n_active_ebrs--;
    }

    /*
     * Reset the PDN connection activation indicator
     */
    // TODO Look at "Reset the PDN connection activation indicator"
    pdn_context->is_active = false;

  }else{
    /*
      * The identity of the EPS bearer to released is given;
      * Release the EPS bearer context entry that match the specified EPS
      * bearer identity
      */
     bearer_context = mme_app_get_session_bearer_context(pdn_context, ebi);
     if(!bearer_context) {
       OAILOG_ERROR(LOG_NAS_ESM , "ESM-PROC  - PDN connection identifier %d " "is not valid\n", *pid);
       OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
     }
     /*
      * Delete the TFT
      */
     // TODO Look at "free_traffic_flow_template"
     //free_traffic_flow_template(&pdn->bearer[i]->tft);

     /*
      * Set the EPS bearer context state to INACTIVE
      */
     int rc  = esm_ebr_release (ue_context, bearer_context, pdn_context, ue_requested);
     if (rc != RETURNok) {
        /*
         * Failed to update ESM bearer status.
         */
        OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Failed to update ESM bearer status. \n");
        OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
     }

     pdn->n_bearers -= 1;

     // todo: update esm context
     emm_context->esm_ctx.n_active_ebrs--;
  }

  /*
   * Update the emergency bearer services indicator
   */
  if (pdn->is_emergency) {
    pdn->is_emergency = false;
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ebi);
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

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


/*! \file mme_app_bearer_context.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "common_types.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "sgw_ie_defs.h"
#include "common_defs.h"
#include "mme_app_bearer_context.h"

/**
 * Create a bearer context pool, not to reallocate for each new UE.
 * todo: remove the pool with shutdown?
 */
static bearer_context_t                 *bearerContextPool = NULL;

//------------------------------------------------------------------------------
bstring bearer_state2string(const mme_app_bearer_state_t bearer_state)
{
  bstring bsstr = NULL;
  if  (BEARER_STATE_NULL == bearer_state) {
    bsstr = bfromcstr("BEARER_STATE_NULL");
    return bsstr;
  }
  bsstr = bfromcstr(" ");
  if  (BEARER_STATE_SGW_CREATED & bearer_state) bcatcstr(bsstr, "SGW_CREATED ");
  if  (BEARER_STATE_MME_CREATED & bearer_state) bcatcstr(bsstr, "MME_CREATED ");
  if  (BEARER_STATE_ENB_CREATED & bearer_state) bcatcstr(bsstr, "ENB_CREATED ");
  if  (BEARER_STATE_ACTIVE & bearer_state) bcatcstr(bsstr, "ACTIVE");
  return bsstr;
}

//------------------------------------------------------------------------------
static void mme_app_bearer_context_init(bearer_context_t *const  bearer_context)
{
  if (bearer_context) {
    ebi_t ebi = bearer_context->ebi;
    memset(bearer_context, 0, sizeof(*bearer_context));
    bearer_context->ebi = ebi;
//    bearer_context->bearer_state = BEARER_STATE_NULL;

    esm_bearer_context_init(&bearer_context->esm_ebr_context);
  }
}

//------------------------------------------------------------------------------
bearer_context_t *mme_app_new_bearer(){
  bearer_context_t * thiz = NULL;
  if (bearerContextPool) {
    thiz = bearerContextPool;
    bearerContextPool = bearerContextPool->next_bc;
  } else {
    thiz = calloc (1, sizeof (bearer_context_t));
  }
  return thiz;
}

//------------------------------------------------------------------------------
int mme_app_bearer_context_delete (bearer_context_t *bearer_context)
{
  mme_app_bearer_context_init(bearer_context);
  bearer_context->next_bc = bearerContextPool;
  bearerContextPool = bearer_context;
  return RETURNok;
}

//------------------------------------------------------------------------------
void mme_app_free_bearer_context (bearer_context_t ** const bearer_context)
{
  free_esm_bearer_context(&(*bearer_context)->esm_ebr_context);
  free_wrapper((void**)bearer_context);
}

//------------------------------------------------------------------------------
bearer_context_t* mme_app_get_session_bearer_context(pdn_context_t * const pdn_context, const ebi_t ebi)
{
  bearer_context_t    bc_key = {.ebi = ebi};
  return RB_FIND(SessionBearers, &pdn_context->session_bearers, &bc_key);
}

//------------------------------------------------------------------------------
void mme_app_get_session_bearer_context_from_all(ue_context_t * const ue_context, const ebi_t ebi, bearer_context_t ** bc_pp)
{
  bearer_context_t  * bearer_context = NULL;
  pdn_context_t     * pdn_context = NULL;
  RB_FOREACH (pdn_context, PdnContexts, &ue_context->pdn_contexts) {
    RB_FOREACH (bearer_context, SessionBearers, &pdn_context->session_bearers) {
      // todo: better error handling
      if(bearer_context->ebi == ebi){
        *bc_pp = bearer_context;
      }
    }
  }
}

//------------------------------------------------------------------------------
void mme_app_register_bearer_context(ue_context_t * const ue_context, ebi_t ebi, const pdn_context_t *pdn_context, bearer_context_t ** bc_pp)
{
  OAILOG_FUNC_IN (LOG_MME_APP);

  AssertFatal((EPS_BEARER_IDENTITY_LAST >= ebi) && (EPS_BEARER_IDENTITY_FIRST <= ebi), "Bad ebi %u", ebi);
  bearer_context_t            *pBearerCtx = NULL; /**< Define a bearer context key. */

  /** Check that the PDN session exists. */
  // todo: add a lot of locks..
  bearer_context_t bc_key = { .ebi = ebi}; /**< Define a bearer context key. */ // todo: just setting one element, and maybe without the key?
  /** Removed a bearer context from the UE contexts bearer pool and adds it into the PDN sessions bearer pool. */
  pBearerCtx = RB_FIND(BearerPool, &ue_context->bearer_pool, &bc_key);
  if(!pBearerCtx){
    OAILOG_ERROR(LOG_MME_APP,  "Could not find a free bearer context with ebi %d for ue_id " MME_UE_S1AP_ID_FMT"! \n", ebi, ue_context->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, NULL);
  }
  bearer_context_t * pBearerCtx_1 = RB_REMOVE(BearerPool, &ue_context->bearer_pool, pBearerCtx);
  DevAssert(pBearerCtx_1);
  bearer_context_t * pBearerCtx_2 = RB_FIND(BearerPool, &ue_context->bearer_pool, &bc_key);
  DevAssert(!pBearerCtx_2);
  /** Received a free bearer context from the free bearer pool of the UE context. It should already be initialized. */
  /* Check that there is no collision when adding the bearer context into the PDN sessions bearer pool. */
  pBearerCtx->pdn_cx_id       = pdn_context->context_identifier;
  /** Insert the bearer context. */
  /** This should not happen with locks. */
  bearer_context_t *pbc_test_1 = NULL;
  pbc_test_1 = RB_INSERT (SessionBearers, &pdn_context->session_bearers, pBearerCtx);
  DevAssert(!pbc_test_1); /**< Collision Check. */

  /** Register the values of the newly registered bearer context. Values might be empty. */
  pBearerCtx->preemption_capability    = pdn_context->default_bearer_eps_subscribed_qos_profile.allocation_retention_priority.pre_emp_capability;
  pBearerCtx->preemption_vulnerability = pdn_context->default_bearer_eps_subscribed_qos_profile.allocation_retention_priority.pre_emp_vulnerability;
  pBearerCtx->priority_level           = pdn_context->default_bearer_eps_subscribed_qos_profile.allocation_retention_priority.priority_level;

  OAILOG_INFO (LOG_MME_APP, "Successfully set bearer context with ebi %d for PDN id %u and for ue id " MME_UE_S1AP_ID_FMT "\n",
      pBearerCtx->ebi, pdn_context->context_identifier, ue_context->mme_ue_s1ap_id);
  *bc_pp = pBearerCtx;
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
int mme_app_deregister_bearer_context(ue_context_t * const ue_context, ebi_t ebi, const pdn_context_t *pdn_context)
{
  AssertFatal((EPS_BEARER_IDENTITY_LAST >= ebi) && (EPS_BEARER_IDENTITY_FIRST <= ebi), "Bad ebi %u", ebi);
  bearer_context_t            *pBearerCtx = NULL; /**< Define a bearer context key. */

  /** Check that the PDN session exists. */
  // todo: add a lot of locks..
  bearer_context_t bc_key = { .ebi = ebi}; /**< Define a bearer context key. */ // todo: just setting one element, and maybe without the key?
  /** Removed a bearer context from the UE contexts bearer pool and adds it into the PDN sessions bearer pool. */
  pBearerCtx = RB_REMOVE(SessionBearers, &pdn_context->session_bearers, &bc_key);
  if(!pBearerCtx){
    OAILOG_ERROR(LOG_MME_APP,  "Could not find an session bearer context with ebi %d for ue_id " MME_UE_S1AP_ID_FMT " inside pdn context with context id %d! \n",
        ebi, ue_context->mme_ue_s1ap_id, pdn_context->context_identifier);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /*
   * We don't have one pool where tunnels are allocated. We allocate a fixed number of bearer contexts at the beginning inside the UE context.
   * So the delete function is unlike to GTPv2c tunnels.
   */

  /** Initialize the new bearer context. */
  mme_app_bearer_context_init(pBearerCtx);
  /** Insert the bearer context into the free bearer of the ue context. */
//  Assert(!RB_INSERT (BearerPool, &ue_context->bearer_pool, pBearerCtx));
  OAILOG_INFO(LOG_MME_APP, "Successfully deregistered the bearer context with ebi %d from PDN id %u and for ue_id " MME_UE_S1AP_ID_FMT "\n",
      pBearerCtx->ebi, pdn_context->context_identifier, ue_context->mme_ue_s1ap_id);
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
void mme_app_bearer_context_s1_release_enb_informations(bearer_context_t * const bc)
{
  if (bc) {
    bc->bearer_state = BEARER_STATE_S1_RELEASED;
    memset(&bc->enb_fteid_s1u, 0, sizeof(bc->enb_fteid_s1u));
    bc->enb_fteid_s1u.teid = INVALID_TEID;
  }
}

//------------------------------------------------------------------------------
void mme_app_bearer_context_update_handover(bearer_context_t * bc_registered, bearer_context_to_be_created_t * const bc_tbc_s10)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  /* Received an initialized bearer context, set the qos values from the pdn_connections IE. */
  // todo: optimize this!
//  DevAssert(bc_registered);
//  DevAssert(bc_tbc_s10);

  /*
   * Initialize the ESM EBR context and set the received QoS values.
   * The ESM EBR state will be set to active with TAU request.
   */
  esm_ebr_context_init(&bc_registered->esm_ebr_context);
  /*
   * Set the bearer level QoS values in the bearer context and the ESM EBR context (updating ESM layer information from MME_APP, unfortunately).
   * No memcpy because of non-GBR MBR/GBR values.
   */
  bc_registered->qci                      = bc_tbc_s10->bearer_level_qos.qci;
  bc_registered->priority_level           = bc_tbc_s10->bearer_level_qos.pl;
  bc_registered->preemption_capability    = bc_tbc_s10->bearer_level_qos.pci;
  bc_registered->preemption_vulnerability = bc_tbc_s10->bearer_level_qos.pvi;

  /*
   * We may have received a set of GBR bearers, for which we need to set the QCI values.
   */
  if(bc_tbc_s10->bearer_level_qos.qci <= 4){
    /** Set the MBR/GBR values for the GBR bearers. */
    bc_registered->esm_ebr_context.gbr_dl   = bc_tbc_s10->bearer_level_qos.gbr.br_dl;
    bc_registered->esm_ebr_context.gbr_ul   = bc_tbc_s10->bearer_level_qos.gbr.br_ul;
    bc_registered->esm_ebr_context.mbr_dl   = bc_tbc_s10->bearer_level_qos.gbr.br_dl;
    bc_registered->esm_ebr_context.mbr_ul   = bc_tbc_s10->bearer_level_qos.gbr.br_ul;
  }else{
    /** Not setting GBR/MBR values for non-GBR bearers to send to the SAE-GW. */
  }
  OAILOG_DEBUG (LOG_MME_APP, "Set qci and bearer level qos values from handover information %u in bearer %u\n", bc_registered->qci, bc_registered->ebi);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}


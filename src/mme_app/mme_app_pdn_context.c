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


/*! \file mme_app_pdn_context.c
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
#include "common_defs.h"
#include "mme_app_pdn_context.h"
#include "mme_app_apn_selection.h"

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

static void mme_app_pdn_context_init(ue_context_t * const ue_context, pdn_context_t *const  pdn_context);
static void mme_app_delete_pdn_context(ue_context_t * const ue_context, pdn_context_t ** pdn_context_pp);
static void mme_app_free_pdn_context (pdn_context_t ** const pdn_context);
static void mme_app_esm_update_bearer_context(bearer_context_t * bearer_context, bearer_qos_t * const bearer_level_qos, esm_ebr_state esm_ebr_state, traffic_flow_template_t * bearer_tft);

//------------------------------------------------------------------------------
void mme_app_get_pdn_context (mme_ue_s1ap_id_t ue_id, pdn_cid_t const context_id, ebi_t const default_ebi, bstring const subscribed_apn, pdn_context_t **pdn_ctx)
{
  ue_context_t *ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }

  /** Checking for valid fields inside the search comparison function. No locks are taken. */
  pdn_context_t pdn_context_key = {.apn_subscribed = subscribed_apn, .default_ebi = default_ebi, .context_identifier = context_id};
  pdn_context_t * pdn_ctx_p = RB_FIND(PdnContexts, &ue_context->pdn_contexts, &pdn_context_key);
  *pdn_ctx = pdn_ctx_p;
  if(!pdn_ctx_p && subscribed_apn){
    /** Could not find the PDN context, search again using the APN. */
    RB_FOREACH (pdn_ctx_p, PdnContexts, &ue_context->pdn_contexts) {
      DevAssert(pdn_ctx_p);
      if(subscribed_apn){
        if(!bstricmp (pdn_ctx_p->apn_subscribed, subscribed_apn)){
            *pdn_ctx = pdn_ctx_p;
            OAILOG_FUNC_OUT(LOG_MME_APP);
        }
      }
    }
  }
  OAILOG_ERROR (LOG_MME_APP, "No PDN context for (ebi=%d,cid=%d,apn=\"%s\") was found for UE: " MME_UE_S1AP_ID_FMT ". \n", default_ebi, context_id,
      (subscribed_apn) ? bdata(subscribed_apn) : "NULL",
      ue_id);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

/*
 * Method to generate bearer contexts to be created.
 */
//------------------------------------------------------------------------------
void mme_app_get_bearer_contexts_to_be_created(pdn_context_t * pdn_context, bearer_contexts_to_be_created_t *bc_tbc, mme_app_bearer_state_t bc_state){

  OAILOG_FUNC_IN (LOG_MME_APP);

  bearer_context_t * bearer_context_to_setup  = NULL;

  RB_FOREACH (bearer_context_to_setup, SessionBearers, &pdn_context->session_bearers) {
    DevAssert(bearer_context_to_setup);
    // todo: make it selective for multi PDN!
    /*
     * Set the bearer of the pdn context to establish.
     * Set regardless of GBR/NON-GBR QCI the MBR/GBR values.
     * They are already set to zero if non-gbr in the registration of the bearer contexts.
     */
    /** EBI. */
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].eps_bearer_id              = bearer_context_to_setup->ebi;
    /** MBR/GBR values (setting indep. of QCI level). */
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.gbr.br_ul = bearer_context_to_setup->bearer_level_qos.gbr.br_ul;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.gbr.br_dl = bearer_context_to_setup->bearer_level_qos.gbr.br_dl;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.mbr.br_ul = bearer_context_to_setup->bearer_level_qos.mbr.br_ul;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.mbr.br_dl = bearer_context_to_setup->bearer_level_qos.mbr.br_dl;
    /** QCI. */
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.qci       = bearer_context_to_setup->bearer_level_qos.qci;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.pvi       = bearer_context_to_setup->bearer_level_qos.pvi;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.pci       = bearer_context_to_setup->bearer_level_qos.pci;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.pl        = bearer_context_to_setup->bearer_level_qos.pl;
    /** Set the S1U SAE-GW FTEID. */
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].s1u_sgw_fteid.ipv4                = bearer_context_to_setup->s_gw_fteid_s1u.ipv4;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].s1u_sgw_fteid.interface_type      = bearer_context_to_setup->s_gw_fteid_s1u.interface_type;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].s1u_sgw_fteid.ipv4_address.s_addr = bearer_context_to_setup->s_gw_fteid_s1u.ipv4_address.s_addr;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].s1u_sgw_fteid.teid                = bearer_context_to_setup->s_gw_fteid_s1u.teid;
    // todo: ipv6, other interfaces!


    bc_tbc->num_bearer_context++;
    /*
     * Update the bearer state.
     * Setting the bearer state as MME_CREATED when successful CSResp is received and as ACTIVE after successful MBResp is received!!
     */
    if(bc_state != BEARER_STATE_NULL){
      bearer_context_to_setup->bearer_state   |= bc_state;
    }
    // todo: set TFTs for dedicated bearers (currently PCRF triggered).
  }
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
int
mme_app_esm_create_pdn_context(mme_ue_s1ap_id_t ue_id, const apn_configuration_t *apn_configuration, const bstring apn,  pdn_cid_t pdn_cid, pdn_context_t **pdn_context_pp)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert(apn);
  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  mme_app_get_pdn_context(ue_id, pdn_cid, EPS_BEARER_IDENTITY_UNASSIGNED, apn, pdn_context_pp);
  if((*pdn_context_pp)){
    /** No PDN context was found. */
    OAILOG_ERROR (LOG_MME_APP, "A PDN context for APN \"%s\" and cid=%d already exists UE: " MME_UE_S1AP_ID_FMT ". \n", bdata(apn), pdn_cid, ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  pdn_context_t * pdn_ctx_TEST = NULL;
  RB_FOREACH (pdn_ctx_TEST, PdnContexts, &ue_context->pdn_contexts) {
    DevAssert(pdn_ctx_TEST);
    OAILOG_DEBUG (LOG_MME_APP, "PDN context %p for APN \"%s\" and cid=%d, ebi=%d (first bearer = %p) already exists UE: " MME_UE_S1AP_ID_FMT ". \n", pdn_ctx_TEST, bdata(pdn_ctx_TEST->apn_subscribed),
        pdn_ctx_TEST->context_identifier, pdn_ctx_TEST->default_ebi, RB_MIN(SessionBearers, &pdn_ctx_TEST->session_bearers), ue_id);
  }

  bearer_context_t * free_bearer = RB_MIN(BearerPool, &ue_context->bearer_pool);
  if(!free_bearer){
    OAILOG_ERROR(LOG_MME_APP, "No available bearer context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  (*pdn_context_pp) = calloc(1, sizeof(pdn_context_t));
  if (!(*pdn_context_pp)) {
    OAILOG_CRITICAL(LOG_MME_APP, "Error creating PDN context for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  //  LOCK_UE_CONTEXT(ue_context);
  if(!RB_MIN(PdnContexts, &ue_context->pdn_contexts)){
    OAILOG_INFO(LOG_MME_APP, "For the first PDN context, creating S11 keys for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
        ue_context->enb_s1ap_id_key,
        ue_context->mme_ue_s1ap_id,
        ue_context->imsi,
        (teid_t) ue_context,       // mme_s11_teid is new
        ue_context->local_mme_teid_s10,
        &ue_context->guti);
  }

  bearer_context_t *bc_test = NULL;
  /** List all bearer contexts. */
  RB_FOREACH(bc_test, BearerPool, &ue_context->bearer_pool){
    OAILOG_DEBUG (LOG_MME_APP, "Current bearer context %p with ebi %d for pdn_context %p for APN \"%s\" and cid=%d for UE: " MME_UE_S1AP_ID_FMT ". \n",
        bc_test, bc_test->ebi, (*pdn_context_pp), bdata((*pdn_context_pp)->apn_subscribed),
        (*pdn_context_pp)->context_identifier, ue_id);

  }
  /*
   * Check if an APN configuration exists. If so, use it to update the fields.
   */
  mme_app_pdn_context_init(ue_context, (*pdn_context_pp));
  /** Get the default bearer context directly. */
  free_bearer = RB_REMOVE(BearerPool, &ue_context->bearer_pool, free_bearer);
  DevAssert(free_bearer);
  AssertFatal((EPS_BEARER_IDENTITY_LAST >= free_bearer->ebi) && (EPS_BEARER_IDENTITY_FIRST <= free_bearer->ebi), "Bad ebi %u", free_bearer->ebi);
  /* Check that there is no collision when adding the bearer context into the PDN sessions bearer pool. */
  /* Insert the bearer context. */
  RB_INSERT (SessionBearers, &(*pdn_context_pp)->session_bearers, free_bearer);

  OAILOG_CRITICAL(LOG_MME_APP, "Received first default bearer context %p with ebi %d for apn of UE: " MME_UE_S1AP_ID_FMT ". \n", free_bearer, free_bearer->ebi, ue_id);


  (*pdn_context_pp)->default_ebi = free_bearer->ebi;
  ue_context->next_def_ebi_offset++;
  free_bearer->linked_ebi = free_bearer->ebi;
  /** Set the APN independently. */
  (*pdn_context_pp)->apn_subscribed = bstrcpy(apn);
  if (apn_configuration) {
    (*pdn_context_pp)->subscribed_apn_ambr          = apn_configuration->ambr; /**< APN received from the HSS APN configuration, to be updated with the PCRF. */
    (*pdn_context_pp)->pdn_type                     = apn_configuration->pdn_type;
    (*pdn_context_pp)->context_identifier           = apn_configuration->context_identifier;
    /** Set the subscribed APN-AMBR from the default profile. */
    (*pdn_context_pp)->subscribed_apn_ambr.br_dl    = apn_configuration->ambr.br_dl;
    (*pdn_context_pp)->subscribed_apn_ambr.br_ul    = apn_configuration->ambr.br_ul;
#ifdef APN_CONFIG_IP_ADDR
    if (apn_configuration->nb_ip_address) { // todo: later
      *pdn_context_pp->paa = calloc(1, sizeof(paa_t));
      *pdn_context_pp->paa->pdn_type           = apn_configuration->ip_address[0].pdn_type;// TODO check this later...
      *pdn_context_pp->paa->ipv4_address       = apn_configuration->ip_address[0].address.ipv4_address;
      if (2 == apn_configuration->nb_ip_address) {
        *pdn_context_pp->paa->ipv6_address       = apn_configuration->ip_address[1].address.ipv6_address;
        *pdn_context_pp->paa->ipv6_prefix_length = 64;
      }
    }
#endif
    /** Set the default QoS values. */
    free_bearer->bearer_level_qos.qci = apn_configuration->subscribed_qos.qci;
    free_bearer->bearer_level_qos.pl  = apn_configuration->subscribed_qos.allocation_retention_priority.priority_level;
    free_bearer->bearer_level_qos.pvi = (apn_configuration->subscribed_qos.allocation_retention_priority.pre_emp_vulnerability) ? 0 : 1;
    free_bearer->bearer_level_qos.pci = (apn_configuration->subscribed_qos.allocation_retention_priority.pre_emp_capability) ? 0 : 1;
//    (*pdn_context_pp)->apn_subscribed = blk2bstr(apn_configuration->service_selection, apn_configuration->service_selection_length);  /**< Set the APN-NI from the service selection. */
  } else {
    (*pdn_context_pp)->context_identifier = pdn_cid + (free_bearer->ebi - 5); /**< Add the temporary context ID from the ebi offset. */
    free_bearer->pdn_cx_id = (*pdn_context_pp)->context_identifier;
    OAILOG_INFO(LOG_MME_APP, "No APN configuration exists for UE " MME_UE_S1AP_ID_FMT ". Subscribed APN \"%s.\" \n", ue_id, bdata((*pdn_context_pp)->apn_subscribed));
    /** The subscribed APN should already exist from handover (forward relocation request). */
    free_bearer->esm_ebr_context.status = ESM_EBR_ACTIVE;
  }
  /* Set the emergency bearer services indicator */
  //    (*pdn_context_pP)->esm_data.is_emergency = is_emergency;
  DevAssert(!(*pdn_context_pp)->pco);
  /** Insert the PDN context into the map of PDN contexts. */
  pdn_context_t * pdn_context_test = RB_INSERT (PdnContexts, &ue_context->pdn_contexts, (*pdn_context_pp));
  if(pdn_context_test){
    OAILOG_INFO(LOG_MME_APP, "ERROR ADDING PDN CONTEXT for UE " MME_UE_S1AP_ID_FMT ". Subscribed APN \"%s.\", pdn_cid=%d, ebi=%d (free_bearer = %p) \n", ue_id, bdata((*pdn_context_pp)->apn_subscribed),
        (*pdn_context_pp)->context_identifier, (*pdn_context_pp)->default_ebi, free_bearer);

  }
  // UNLOCK_UE_CONTEXT
  MSC_LOG_EVENT (MSC_NAS_ESM_MME, "0 Create PDN cid %u APN %s", (*pdn_context_pp)->context_identifier, bdata((*pdn_context_pp)->apn_subscribed));
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
int
mme_app_esm_update_pdn_context(mme_ue_s1ap_id_t ue_id, const bstring apn, pdn_cid_t pdn_cid, ebi_t linked_ebi,
    const pdn_type_t pdn_type, const paa_t * const paa,
    esm_ebr_state esm_ebr_state, ambr_t * const apn_ambr, bearer_qos_t * const default_bearer_qos,
    protocol_configuration_options_t *pco){
  ue_context_t              *ue_context         = NULL;
  pdn_context_t             *pdn_context        = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  mme_app_get_pdn_context(ue_id, pdn_cid, linked_ebi, apn, &pdn_context);
  if(!pdn_context){
    OAILOG_ERROR (LOG_MME_APP, "No PDN context for could be found for APN \"%s\" for UE: " MME_UE_S1AP_ID_FMT ". \n", bdata(apn), ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  //  LOCK_UE_CONTEXT(ue_context);
  if(apn_ambr){
    if(apn_ambr->br_dl && apn_ambr->br_ul){
      /** Update the authorized APN ambr of the pdn context. */
      // todo: check if UE apn-ambr reached, if so, just add the remaining. (todo: inform sae/pcrf)
      pdn_context->subscribed_apn_ambr.br_dl = apn_ambr->br_dl;
      pdn_context->subscribed_apn_ambr.br_ul = apn_ambr->br_ul;
    } else {
      /** No APN AMBR values are set, setting the current values. */
      apn_ambr->br_dl = pdn_context->subscribed_apn_ambr.br_dl;
      apn_ambr->br_ul = pdn_context->subscribed_apn_ambr.br_ul;
    }
  }
  /** Set the new PAA. */
  if (paa) {
    /** Set the PAA. */
    DevAssert(!pdn_context->paa);
    pdn_context->paa = paa;
  }
  /** Get the default bearer context directly. */
  bearer_context_t * default_bearer = RB_MIN(SessionBearers, &pdn_context->session_bearers);
  DevAssert(default_bearer);
  DevAssert(default_bearer->ebi == default_bearer->linked_ebi);
  DevAssert(default_bearer->ebi == pdn_context->default_ebi);
  mme_app_esm_update_bearer_context(default_bearer, default_bearer_qos, esm_ebr_state, (traffic_flow_template_t*)NULL);
  /** Save the PCO for handover purposes. */
  if(pco){
    if (!pdn_context->pco) {
      pdn_context->pco = calloc(1, sizeof(protocol_configuration_options_t));
    } else {
      clear_protocol_configuration_options(pdn_context->pco);
    }
    copy_protocol_configuration_options(pdn_context->pco, pco);
  }

  //  UNLOCK_UE_CONTEXT(ue_context);
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
void
mme_app_esm_delete_pdn_context(mme_ue_s1ap_id_t ue_id, bstring apn, pdn_cid_t pdn_cid, ebi_t linked_ebi){
  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context_t        * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  pdn_context_t       * pdn_context = NULL;

  if(!ue_context){
    OAILOG_WARNING(LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT " to release \"%s\". \n", ue_id, bdata(apn));
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  mme_app_get_pdn_context(ue_id, pdn_cid, linked_ebi, apn, &pdn_context);
  if (!pdn_context) {
    OAILOG_WARNING(LOG_NAS_ESM, "ESM-PROC  - PDN connection for cid=%d and ebi=%d and APN \"%s\" has not been allocated for UE "MME_UE_S1AP_ID_FMT". \n", pdn_cid, linked_ebi, bdata(apn), ue_id);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  //  LOCK_UE_CONTEXT(ue_context);
  mme_app_delete_pdn_context(ue_context, &pdn_context);
  // UNLOCK_UE_CONTEXT
  MSC_LOG_EVENT (MSC_NAS_ESM_MME, "0 Create PDN cid %u APN %s", (*pdn_context_pp)->context_identifier, (*pdn_context_pp)->apn_in_use);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_esm_detach(mme_ue_s1ap_id_t ue_id){
  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context_t        * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  pdn_context_t       * pdn_context = NULL;

  if(!ue_context){
    OAILOG_WARNING(LOG_MME_APP, "No UE context could be found for UE: " MME_UE_S1AP_ID_FMT " to release ESM contexts. \n", ue_id);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  // todo: LOCK UE CONTEXTS
  mme_app_nas_esm_free_bearer_context_procedures(ue_context);
  mme_app_nas_esm_free_pdn_connectivity_procedures(ue_context);

  OAILOG_INFO(LOG_MME_APP, "Removed all ESM procedures of UE: " MME_UE_S1AP_ID_FMT " for detach. \n", ue_id);
  pdn_context = RB_MIN(PdnContexts, &ue_context->pdn_contexts);
  while(pdn_context){
    mme_app_delete_pdn_context(ue_context, &pdn_context);
    pdn_context = RB_MIN(PdnContexts, &ue_context->pdn_contexts);
  }
  OAILOG_INFO(LOG_MME_APP, "Removed all ESM contexts of UE: " MME_UE_S1AP_ID_FMT " for detach. \n", ue_id);
  bearer_context_t * bc_free = RB_MIN(BearerPool, &ue_context->bearer_pool);
  // todo: LOCK UE CONTEXTS
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

//------------------------------------------------------------------------------
static void mme_app_pdn_context_init(ue_context_t * const ue_context, pdn_context_t *const  pdn_context)
{
  if ((pdn_context) && (ue_context)) {
    memset(pdn_context, 0, sizeof(*pdn_context));
//    pdn_context->is_active = false;
    /** Initialize the session bearers map. */
    RB_INIT(&pdn_context->session_bearers);
  }
}

//------------------------------------------------------------------------------
static void mme_app_delete_pdn_context(ue_context_t * const ue_context, pdn_context_t ** pdn_context_pp){
  OAILOG_FUNC_IN (LOG_MME_APP);

  if(!(*pdn_context_pp)){
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }

  /** We found the PDN context, we will remove it directly from the UE context, remove & clean up the session bearers and free the PDN context. */
  pdn_context_t *pdn_context_removed = RB_REMOVE(PdnContexts, &ue_context->pdn_contexts, (*pdn_context_pp));
  DevAssert(pdn_context_removed);
  /*
   * Release all session bearers of the PDN context back into the UE pool.
   */
  bearer_context_t * pBearerCtx = RB_MIN(SessionBearers, &(*pdn_context_pp)->session_bearers);
  while(pBearerCtx){
    DevAssert(RB_REMOVE(SessionBearers, &(*pdn_context_pp)->session_bearers, pBearerCtx));
    /*
     * Delete the TFT,
     * todo: check any other allocated fields..
     *
     */
    // TODO Look at "free_traffic_flow_template"
    //free_traffic_flow_template(&pdn->bearer[i]->tft);
    /*
     * We don't have one pool where tunnels are allocated. We allocate a fixed number of bearer contexts at the beginning inside the UE context.
     * So the delete function is unlike to GTPv2c tunnels.
     */
    // no timers to stop, no DSR to be sent..
    // todo: PCOs
    //     * Set the emergency bearer services indicator
    //     */
    //    (*pdn_context_pP)->esm_data.is_emergency = is_emergency;
    //      if (pco) {
    //        if (!(*pdn_context_pP)->pco) {
    //          (*pdn_context_pP)->pco = calloc(1, sizeof(protocol_configuration_options_t));
    //        } else {
    //          clear_protocol_configuration_options((*pdn_context_pP)->pco);
    //        }
    //        copy_protocol_configuration_options((*pdn_context_pP)->pco, pco);
    //      }
    /** Initialize the new bearer context. */
    mme_app_bearer_context_initialize(pBearerCtx);
    /** Insert the bearer context into the free bearer of the ue context. */
    RB_INSERT (BearerPool, &ue_context->bearer_pool, pBearerCtx);
    OAILOG_INFO(LOG_MME_APP, "Successfully deregistered the bearer context with ebi %d from PDN id %u and for ue_id " MME_UE_S1AP_ID_FMT "\n",
        pBearerCtx->ebi, (*pdn_context_pp)->context_identifier, ue_context->mme_ue_s1ap_id);
    pBearerCtx = RB_MIN(SessionBearers, &(*pdn_context_pp)->session_bearers);
  }
  /** Successfully removed all bearer contexts, clean up the PDN context procedure. */
  mme_app_free_pdn_context(pdn_context_pp); /**< Frees it by putting it back to the pool. */
  /** Insert the PDN context into the map of PDN contexts. */
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
static void mme_app_free_pdn_context (pdn_context_t ** const pdn_context)
{
  if ((*pdn_context)->apn_in_use) {
    bdestroy_wrapper(&(*pdn_context)->apn_in_use);
  }
  if ((*pdn_context)->apn_subscribed) {
    bdestroy_wrapper(&(*pdn_context)->apn_subscribed);
  }
  if ((*pdn_context)->apn_oi_replacement) {
    bdestroy_wrapper(&(*pdn_context)->apn_oi_replacement);
  }
  if ((*pdn_context)->pco) {
    free_protocol_configuration_options(&((*pdn_context)->pco));
  }
  // todo: free PAA
  if((*pdn_context)->paa){
    free_wrapper((void**)&((*pdn_context)->paa));
  }
  /** Clean the PDN context and deallocate it. */
  memset(*pdn_context, 0, sizeof(pdn_context_t));
  free_wrapper((void**)pdn_context);
}

// todo: move this into mme_app_bearer_context.c !
//------------------------------------------------------------------------------
static void
mme_app_esm_update_bearer_context(bearer_context_t * bearer_context, bearer_qos_t * const bearer_level_qos, esm_ebr_state esm_ebr_state, traffic_flow_template_t * bearer_tft){
  OAILOG_FUNC_IN (LOG_MME_APP);
  /*
   * Setup the EPS bearer data with verified TFT and QoS.
   */
  if(bearer_level_qos){
    if(bearer_level_qos->qci && bearer_level_qos->pl){
      bearer_context->bearer_level_qos.qci = bearer_level_qos->qci;
      bearer_context->bearer_level_qos.pci = bearer_level_qos->pci; // todo: pcipreemption_capability    = // (*bearer_level_qos)->pci == 0 ? 1 : 0;
      bearer_context->bearer_level_qos.pvi = bearer_level_qos->pvi; // todo: preemption_vulnerability = (*bearer_level_qos)->pvi == 0 ? 1 : 0;
      bearer_context->bearer_level_qos.pl  = bearer_level_qos->pl;
      DevAssert(!bearer_context->esm_ebr_context.tft);
    } else {
      memcpy((void*)bearer_level_qos, &bearer_context->bearer_level_qos, sizeof(bearer_qos_t));
    }
  }
  OAILOG_DEBUG(LOG_MME_APP, "Setting the ESM state of ebi %d to \"%s\". \n", bearer_context->ebi, esm_ebr_state2string(esm_ebr_state));
  bearer_context->esm_ebr_context.status = esm_ebr_state;
  if(bearer_tft){
    if(bearer_context->esm_ebr_context.tft)
      free_traffic_flow_template(&bearer_context->esm_ebr_context.tft);
    bearer_context->esm_ebr_context.tft = bearer_tft;
    bearer_context->esm_ebr_context.status = esm_ebr_state;
  }
  /*
   * Return the EPS bearer identity of the default EPS bearer
   * * * * associated to the new EPS bearer context
   */
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

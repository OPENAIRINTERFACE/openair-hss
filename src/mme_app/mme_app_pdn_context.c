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
#include "mme_app_session_context.h"
#include "mme_app_defs.h"
#include "common_defs.h"
#include "esm_cause.h"
#include "mme_app_pdn_context.h"
#include "mme_app_esm_procedures.h"
#include "mme_app_apn_selection.h"
#include "mme_app_bearer_context.h"

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

static void mme_app_free_pdn_context (pdn_context_t ** const pdn_context);

//------------------------------------------------------------------------------
void mme_app_get_pdn_context (mme_ue_s1ap_id_t ue_id, pdn_cid_t const context_id, ebi_t const default_ebi, bstring const apn_subscribed, pdn_context_t **pdn_ctx)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_session_pool_t * ue_session_pool =  mme_ue_session_pool_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_session_pools, ue_id);
  if(!ue_session_pool){
    OAILOG_ERROR (LOG_MME_APP, "No UE session pool could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  /** Checking for valid fields inside the search comparison function. No locks are taken. */
  pdn_context_t pdn_context_key = {.apn_subscribed = apn_subscribed, .default_ebi = default_ebi, .context_identifier = context_id};
  pdn_context_t * pdn_ctx_p = RB_FIND(PdnContexts, &ue_session_pool->pdn_contexts, &pdn_context_key);
  *pdn_ctx = pdn_ctx_p;
  if(*pdn_ctx) {
	  OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  if(apn_subscribed){
    /** Could not find the PDN context, search again using the APN. */
    RB_FOREACH (pdn_ctx_p, PdnContexts, &ue_session_pool->pdn_contexts) {
      DevAssert(pdn_ctx_p);
      if(apn_subscribed){
        if(!bstricmp (pdn_ctx_p->apn_subscribed, apn_subscribed)){
            *pdn_ctx = pdn_ctx_p;
            OAILOG_FUNC_OUT(LOG_MME_APP);
        }
      }
    }
  }
  OAILOG_WARNING(LOG_MME_APP, "No PDN context for (ebi=%d,cid=%d,apn=\"%s\") was found for UE: " MME_UE_S1AP_ID_FMT ". \n", default_ebi, context_id,
      (apn_subscribed) ? bdata(apn_subscribed) : "NULL",
      ue_id);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
int
mme_app_esm_create_pdn_context(mme_ue_s1ap_id_t ue_id, const ebi_t linked_ebi, const apn_configuration_t *apn_configuration, const bstring apn_subscribed,  pdn_cid_t pdn_cid, const ambr_t * const apn_ambr, pdn_type_t pdn_type, pdn_context_t **pdn_context_pp)
{
  OAILOG_FUNC_IN (LOG_MME_APP);

  ue_session_pool_t   * ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_session_pools, ue_id);
  if(!ue_session_pool){
	  /** No UE session pool exists: should be created when the UE is created. */
	  OAILOG_ERROR (LOG_MME_APP, "No UE session pool for UE: " MME_UE_S1AP_ID_FMT ". "
			  "Cannot create a new pdn context for APN \"%s\" and cid=%d. \n",
			  apn_subscribed ? bdata(apn_subscribed) : "NULL", pdn_cid, ue_id);
	  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }
  // todo: unlock the mme_desc

  mme_app_get_pdn_context(ue_session_pool, pdn_cid, EPS_BEARER_IDENTITY_UNASSIGNED, apn_subscribed, pdn_context_pp);
  if((*pdn_context_pp)){
    OAILOG_ERROR (LOG_MME_APP, "A PDN context for APN \"%s\" and cid=%d already exists UE: " MME_UE_S1AP_ID_FMT ". \n",
    		apn_subscribed ? bdata(apn_subscribed) : "NULL", pdn_cid, ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  // todo: lock the session pool
  bearer_context_new_t * free_bearer = NULL;
  if(linked_ebi != EPS_BEARER_IDENTITY_UNASSIGNED){
	  mme_app_get_free_bearer_context(ue_session_pool, linked_ebi, &free_bearer); /**< Find the EBI which is matching (should be available). */
  } else{
	  free_bearer = STAILQ_FIRST(&ue_session_pool->free_bearers); /**< Find the EBI which is matching (should be available). */
  }
  if(!free_bearer){
    OAILOG_ERROR(LOG_MME_APP, "No available bearer context could be found for UE: " MME_UE_S1AP_ID_FMT " with linked_ebi=%d. \n", ue_id, linked_ebi);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  (*pdn_context_pp) = calloc(1, sizeof(pdn_context_t));
  if (!(*pdn_context_pp)) {
    OAILOG_CRITICAL(LOG_MME_APP, "Error creating PDN context for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  //  LOCK_UE_SESSION_POOL;

  /*
   * Check if an APN configuration exists. If so, use it to update the fields.
   */
  memset((*pdn_context_pp), 0, sizeof((*pdn_context_pp)));
  /** Initialize the session bearers map. */
  // LIST_INIT(&(*pdn_context_pp)->session_bearers);
  STAILQ_INIT(&(*pdn_context_pp)->session_bearers);
  /** Get the default bearer context directly. */
  STAILQ_REMOVE(&ue_session_pool->free_bearers, free_bearer, bearer_context_new_s, entries);
  DevAssert(free_bearer);
  AssertFatal((EPS_BEARER_IDENTITY_LAST >= free_bearer->ebi) && (EPS_BEARER_IDENTITY_FIRST <= free_bearer->ebi), "Bad ebi %u", free_bearer->ebi);
  /* Check that there is no collision when adding the bearer context into the PDN sessions bearer pool. */
  /* Insert the bearer context. */
  STAILQ_INSERT_TAIL(&(*pdn_context_pp)->session_bearers, free_bearer, entries);
  OAILOG_INFO(LOG_MME_APP, "Received first default bearer context %p with ebi %d for apn \"%s\" of UE: " MME_UE_S1AP_ID_FMT ". \n",
		  free_bearer, free_bearer->ebi, bdata(apn_subscribed), ue_id);
  (*pdn_context_pp)->default_ebi = free_bearer->ebi;
  ue_session_pool->next_def_ebi_offset++;
  free_bearer->linked_ebi = free_bearer->ebi;
  /** Set the APN independently. */
  (*pdn_context_pp)->apn_subscribed = bstrcpy(apn_subscribed);
  /** Set the default QoS values. */
  (*pdn_context_pp)->subscribed_apn_ambr.br_dl = apn_ambr->br_dl;
  (*pdn_context_pp)->subscribed_apn_ambr.br_ul = apn_ambr->br_ul;
  (*pdn_context_pp)->pdn_type                     = pdn_type;
  if (apn_configuration) {
    (*pdn_context_pp)->context_identifier           = apn_configuration->context_identifier;
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
    free_bearer->bearer_level_qos.qci = apn_configuration->subscribed_qos.qci;
    free_bearer->bearer_level_qos.pl  = apn_configuration->subscribed_qos.allocation_retention_priority.priority_level;
    free_bearer->bearer_level_qos.pvi = (apn_configuration->subscribed_qos.allocation_retention_priority.pre_emp_vulnerability) ? 0 : 1;
    free_bearer->bearer_level_qos.pci = (apn_configuration->subscribed_qos.allocation_retention_priority.pre_emp_capability) ? 0 : 1;
    free_bearer->pdn_cx_id            = (*pdn_context_pp)->context_identifier;

    //  if(pdn_connection->ipv4_address.s_addr) {
    ////    IPV4_STR_ADDR_TO_INADDR ((const char *)pdn_connection->ipv4_address, pdn_context->paa->ipv4_address, "BAD IPv4 ADDRESS FORMAT FOR PAA!\n");
    //    pdn_context->paa->ipv4_address.s_addr = pdn_connection->ipv4_address.s_addr;
    //  }
    //  if(pdn_connection->ipv6_address) {
    //    //    memset (pdn_connections->pdn_connection[num_pdn].ipv6_address, 0, 16);
    //    memcpy (pdn_context->paa->ipv6_address.s6_addr, pdn_connection->ipv6_address.s6_addr, 16);
    //    pdn_context->paa->ipv6_prefix_length = pdn_connection->ipv6_prefix_length;
    //    pdn_context->pdn_type++;
    //    if(pdn_connection->ipv4_address.s_addr)
    //      pdn_context->pdn_type++;
    //  }


//    (*pdn_context_pp)->apn_subscribed = blk2bstr(apn_configuration->service_selection, apn_configuration->service_selection_length);  /**< Set the APN-NI from the service selection. */
  } else {
    (*pdn_context_pp)->context_identifier = pdn_cid + (free_bearer->ebi - 5); /**< Add the temporary context ID from the ebi offset. */
    free_bearer->pdn_cx_id = (*pdn_context_pp)->context_identifier;
    OAILOG_INFO(LOG_MME_APP, "No APN configuration exists for UE " MME_UE_S1AP_ID_FMT ". Subscribed APN \"%s.\" \n", ue_id, bdata((*pdn_context_pp)->apn_subscribed));
    /** The subscribed APN should already exist from handover (forward relocation request). */
  }
  /* Set the emergency bearer services indicator */
  //    (*pdn_context_pP)->esm_data.is_emergency = is_emergency;
  DevAssert(!(*pdn_context_pp)->pco);
  /** Insert the PDN context into the map of PDN contexts. */
  pdn_context_t * pdn_context_test = RB_INSERT (PdnContexts, &ue_session_pool->pdn_contexts, (*pdn_context_pp));
  if(pdn_context_test){
    OAILOG_INFO(LOG_MME_APP, "ERROR ADDING PDN CONTEXT for UE " MME_UE_S1AP_ID_FMT ". Subscribed APN \"%s.\", pdn_cid=%d, ebi=%d (free_bearer = %p) \n", ue_id, bdata((*pdn_context_pp)->apn_subscribed),
        (*pdn_context_pp)->context_identifier, (*pdn_context_pp)->default_ebi, free_bearer);
  }
  // UNLOCK_UE_SESSION_POOL
  MSC_LOG_EVENT (MSC_NAS_ESM_MME, "0 Create PDN cid %u APN %s", (*pdn_context_pp)->context_identifier, bdata((*pdn_context_pp)->apn_subscribed));
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
esm_cause_t
mme_app_update_pdn_context(mme_ue_s1ap_id_t ue_id, imsi64_t imsi, const subscription_data_t * const subscription_data, eps_bearer_context_status_t * const bc_status){
  OAILOG_FUNC_IN (LOG_MME_APP);

  ue_session_pool_t   * ue_session_pool   = mme_ue_session_pool_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_session_pools, ue_id);
  pdn_context_t 	  * pdn_context 	  = NULL, * pdn_context_safe = NULL;
  apn_configuration_t * apn_configuration = NULL;

  if(!ue_session_pool){
    OAILOG_WARNING(LOG_MME_APP, "No MME_APP UE session pool could be found for UE: " MME_UE_S1AP_ID_FMT " to update the pdn context information from subscription data. \n", ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }

  /** Check if any PDN context is there (idle TAU) with failed S10. */
  // todo: LOCK_UE_SESSION_POOL
  /**
   * Iterate through all the pdn contexts of the UE.
   * Check if for each pdn context, an APN configuration exists.
   * If not remove those PDN contexts.
   */
  bool changed = false;
  do {
    RB_FOREACH_SAFE(pdn_context, PdnContexts, &ue_session_pool->pdn_contexts, pdn_context_safe) { /**< Use the safe iterator. */
      /** Remove the pdn context. */
      changed = false;
      mme_app_select_apn(imsi, pdn_context->apn_subscribed, &apn_configuration);
      if(apn_configuration){
        /**
         * We found an APN configuration. Updating it. Might traverse the list new.
         * We check if anything (ctxId) will be changed, that might alter the position of the element in the list.
         */
        OAILOG_INFO(LOG_NAS_EMM, "EMMCN-SAP  - " "Updating the created PDN context from the subscription profile for UE " MME_UE_S1AP_ID_FMT". \n", ue_id);
        changed = pdn_context->context_identifier >= PDN_CONTEXT_IDENTIFIER_UNASSIGNED;
        pdn_cid_t old_cid = pdn_context->context_identifier;
        /** If it is an invalid context identifier, update it. */
        if(changed){
          pdn_context = RB_REMOVE(PdnContexts, &ue_session_pool->pdn_contexts, pdn_context);
          DevAssert(pdn_context);
          pdn_context->context_identifier = apn_configuration->context_identifier;
          /** Set the context also to all the bearers. */
          bearer_context_new_t * bearer_context = NULL;
          STAILQ_FOREACH (bearer_context, &pdn_context->session_bearers, entries) {
            bearer_context->pdn_cx_id = apn_configuration->context_identifier; /**< Update for all bearer contexts. */
          }
          /** Update the PDN context ambr only if it was 0. */
          if(!pdn_context->subscribed_apn_ambr.br_dl)
            pdn_context->subscribed_apn_ambr.br_dl = apn_configuration->ambr.br_dl;
          if(!pdn_context->subscribed_apn_ambr.br_ul)
            pdn_context->subscribed_apn_ambr.br_ul = apn_configuration->ambr.br_ul;

          DevAssert(!RB_INSERT (PdnContexts, &ue_session_pool->pdn_contexts, pdn_context));
        }
        if(changed)
          break; /**< Start from beginning. */
        /** Nothing changed, continue processing the elements. */
        continue;
      }else {
        /**
         * Remove the PDN context and trigger a DSR.
         * Set in the flags, that it should not be signaled back to the EMM layer. Might not need to traverse the list new.
         */
        OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "PDN context for APN \"%s\" could not be found in subscription profile for UE "
            "with ue_id " MME_UE_S1AP_ID_FMT ". Triggering deactivation of PDN context. \n", bdata(pdn_context->apn_subscribed), ue_id);
        /** Set the flag for the delete tunnel. */
        bool deleteTunnel = (RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts)== pdn_context);
        nas_itti_pdn_disconnect_req(ue_id, pdn_context->default_ebi, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, deleteTunnel, false,
            &pdn_context->s_gw_addr_s11_s4, pdn_context->s_gw_teid_s11_s4, pdn_context->context_identifier);
        /**
         * No response is expected.
         * Implicitly detach the PDN context bearer contexts from the UE.
         */
        mme_app_delete_pdn_context(ue_session_pool, &pdn_context);
        OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Invalid PDN context removed successfully ue_id " MME_UE_S1AP_ID_FMT ". \n", ue_id);
        continue;
      }
    }
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - " "Completed the checking of PDN contexts for ue_id " MME_UE_S1AP_ID_FMT ". \n", ue_id);
  } while(changed);

  // todo: UNLOCK_UE_CONTEXT
  /** Set the context identifier when updating the pdn_context. */
  OAILOG_INFO(LOG_NAS_EMM, "EMMCN-SAP  - " "Successfully updated all PDN contexts for UE " MME_UE_S1AP_ID_FMT". \n", ue_id);

  RB_FOREACH(pdn_context, PdnContexts, &ue_session_pool->pdn_contexts) { /**< Use the safe iterator. */
	  bearer_context_new_t * bearer_context = NULL;
	  STAILQ_FOREACH (bearer_context, &pdn_context->session_bearers, entries) {
		  (*bc_status) |= (0x01 << bearer_context->ebi);
	  }
  }
  (*bc_status) = ntohs(*bc_status);

  if(RB_EMPTY(&ue_session_pool->pdn_contexts)){
	  OAILOG_ERROR(LOG_NAS_EMM, "EMMCN-SAP  - " "No PDN contexts left for UE " MME_UE_S1AP_ID_FMT" "
			  "after updating for the received subscription information. \n", ue_id);
	  OAILOG_FUNC_RETURN (LOG_NAS_EMM, ESM_CAUSE_PDN_CONNECTION_DOES_NOT_EXIST);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, ESM_CAUSE_SUCCESS);
}

//------------------------------------------------------------------------------
void mme_app_delete_pdn_context(ue_session_pool_t * const ue_session_pool, pdn_context_t ** pdn_context_pp){
  OAILOG_FUNC_IN (LOG_MME_APP);

  if(!(*pdn_context_pp)){
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }

  /** We found the PDN context, we will remove it directly from the UE context, remove & clean up the session bearers and free the PDN context. */
  pdn_context_t *pdn_context = RB_REMOVE(PdnContexts, &ue_session_pool->pdn_contexts, *pdn_context_pp);
  DevAssert(pdn_context);

  /*
   * Release all session bearers of the PDN context back into the UE pool.
   */
  bearer_context_new_t * pBearerCtx = STAILQ_FIRST(&(*pdn_context_pp)->session_bearers);
  while(pBearerCtx){
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
    clear_bearer_context(ue_session_pool, pBearerCtx);
    OAILOG_INFO(LOG_MME_APP, "Successfully deregistered the bearer context with ebi %d from PDN id %u. \n",
        pBearerCtx->ebi, (*pdn_context_pp)->context_identifier);
  }
  /** Successfully removed all bearer contexts, clean up the PDN context procedure. */
  mme_app_free_pdn_context(pdn_context_pp); /**< Frees it by putting it back to the pool. */
  /** Insert the PDN context into the map of PDN contexts. */
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_esm_delete_pdn_context(mme_ue_s1ap_id_t ue_id, bstring apn, pdn_cid_t pdn_cid, ebi_t linked_ebi){
  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_session_pool_t   * ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_session_pools, ue_id);
  pdn_context_t       * pdn_context = NULL;

  if(!ue_session_pool){
    OAILOG_WARNING(LOG_MME_APP, "No MME_APP UE session pool could be found for UE: " MME_UE_S1AP_ID_FMT " to release \"%s\". \n",
    		ue_id, bdata(apn));
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  mme_app_get_pdn_context(ue_id, pdn_cid, linked_ebi, apn, &pdn_context);
  if (!pdn_context) {
    OAILOG_WARNING(LOG_NAS_ESM, "ESM-PROC  - PDN connection for cid=%d and ebi=%d and APN \"%s\" has not been allocated for UE "MME_UE_S1AP_ID_FMT". \n",
    		pdn_cid, linked_ebi, bdata(apn), ue_id);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  //  LOCK_UE_CONTEXT(ue_context);
  mme_app_delete_pdn_context(ue_session_pool, &pdn_context);
  // UNLOCK_UE_CONTEXT
  MSC_LOG_EVENT (MSC_NAS_ESM_MME, "0 Create PDN cid %u APN %s", (*pdn_context_pp)->context_identifier, (*pdn_context_pp)->apn_in_use);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

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
}

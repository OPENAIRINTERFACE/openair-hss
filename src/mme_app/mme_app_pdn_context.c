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
#include "esm_cause.h"
#include "mme_app_pdn_context.h"
#include "mme_app_esm_procedures.h"
#include "mme_app_apn_selection.h"
#include "mme_app_bearer_context.h"

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

static void mme_app_pdn_context_init(ue_context_t * const ue_context, pdn_context_t *const  pdn_context);
static void mme_app_delete_pdn_context(ue_context_t * const ue_context, pdn_context_t ** pdn_context_pp);
static void mme_app_free_pdn_context (pdn_context_t ** const pdn_context);

static teid_t                           mme_app_teid_generator = 0x00000001;

//------------------------------------------------------------------------------
void mme_app_get_pdn_context (mme_ue_s1ap_id_t ue_id, pdn_cid_t const context_id, ebi_t const default_ebi, bstring const apn_subscribed, pdn_context_t **pdn_ctx)
{
	OAILOG_FUNC_IN (LOG_MME_APP);

	ue_context_t *ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);

  if(!ue_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }

  /** Checking for valid fields inside the search comparison function. No locks are taken. */
  pdn_context_t pdn_context_key = {.apn_subscribed = apn_subscribed, .default_ebi = default_ebi, .context_identifier = context_id};
  pdn_context_t * pdn_ctx_p = RB_FIND(PdnContexts, &ue_context->pdn_contexts, &pdn_context_key);
  *pdn_ctx = pdn_ctx_p;
  if(*pdn_ctx) {
	  OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  if(apn_subscribed){
    /** Could not find the PDN context, search again using the APN. */
    RB_FOREACH (pdn_ctx_p, PdnContexts, &ue_context->pdn_contexts) {
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

/*
 * Method to generate bearer contexts to be created.
 */
//------------------------------------------------------------------------------
void mme_app_get_bearer_contexts_to_be_created(pdn_context_t * pdn_context, bearer_contexts_to_be_created_t *bc_tbc, mme_app_bearer_state_t bc_state){

  OAILOG_FUNC_IN (LOG_MME_APP);

  bearer_context_new_t * bearer_context_to_setup  = NULL;

  LIST_FOREACH (bearer_context_to_setup, pdn_context->session_bearers, entries) {
    DevAssert(bearer_context_to_setup);
    /*
     * Set the bearer of the pdn context to establish.
     * Set regardless of GBR/NON-GBR QCI the MBR/GBR values.
     * They are already set to zero if non-gbr in the registration of the bearer contexts.
     */
    /** EBI. */
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].eps_bearer_id              = bearer_context_to_setup->ebi;
    /**
     * MBR/GBR values (setting indep. of QCI level).
     * Dividing by 1000 happens later in S11/S10 only.
     */
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

    /**
     * TFTs (method used for S11 CSReq).
     */
    /** Set the TFT. */
    if(bearer_context_to_setup->esm_ebr_context.tft){
      DevAssert(!bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].tft);
      bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].tft = (traffic_flow_template_t*)calloc(1, sizeof(traffic_flow_template_t));
      copy_traffic_flow_template(bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].tft, bearer_context_to_setup->esm_ebr_context.tft);
    }

    bc_tbc->num_bearer_context++;
    /*
     * Update the bearer state.
     * Setting the bearer state as MME_CREATED when successful CSResp is received and as ACTIVE after successful MBResp is received!!
     */
    if(bc_state != BEARER_STATE_NULL){
      bearer_context_to_setup->bearer_state   |= bc_state;
    }
  }
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
int
mme_app_esm_create_pdn_context(mme_ue_s1ap_id_t ue_id, const ebi_t linked_ebi, const apn_configuration_t *apn_configuration, const bstring apn_subscribed,  pdn_cid_t pdn_cid, const ambr_t * const apn_ambr, pdn_type_t pdn_type, pdn_context_t **pdn_context_pp)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  mme_app_get_pdn_context(ue_id, pdn_cid, EPS_BEARER_IDENTITY_UNASSIGNED, apn_subscribed, pdn_context_pp);
  if((*pdn_context_pp)){
    /** No PDN context was found. */
    OAILOG_ERROR (LOG_MME_APP, "A PDN context for APN \"%s\" and cid=%d already exists UE: " MME_UE_S1AP_ID_FMT ". \n", apn_subscribed ? bdata(apn_subscribed) : "NULL", pdn_cid, ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  pdn_context_t * pdn_ctx_TEST = NULL;
  RB_FOREACH (pdn_ctx_TEST, PdnContexts, &ue_context->pdn_contexts) {
    DevAssert(pdn_ctx_TEST);
    OAILOG_DEBUG (LOG_MME_APP, "PDN context %p for APN \"%s\" and cid=%d, ebi=%d (first bearer = %p) already exists UE: " MME_UE_S1AP_ID_FMT ". \n", pdn_ctx_TEST, bdata(pdn_ctx_TEST->apn_subscribed),
        pdn_ctx_TEST->context_identifier, pdn_ctx_TEST->default_ebi, LIST_FIRST(pdn_ctx_TEST->session_bearers), ue_id);
  }

  bearer_context_new_t * free_bearer = NULL;
  if(linked_ebi != EPS_BEARER_IDENTITY_UNASSIGNED){
	  mme_app_get_free_bearer_context(ue_context, linked_ebi, &free_bearer); /**< Find the EBI which is matching (should be available). */
  } else{
	  free_bearer = LIST_FIRST(ue_context->ue_bearer_pool->free_bearers); /**< Find the EBI which is matching (should be available). */
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
  //  LOCK_UE_CONTEXT(ue_context);
  if(!RB_MIN(PdnContexts, &ue_context->pdn_contexts)){
    OAILOG_INFO(LOG_MME_APP, "For the first PDN context, creating S11 keys for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    teid_t teid = __sync_fetch_and_add (&mme_app_teid_generator, 0x00000001);
    mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
        ue_context->enb_s1ap_id_key,
        ue_context->mme_ue_s1ap_id,
        ue_context->imsi,
        teid,       // mme_s11_teid is new
        ue_context->local_mme_teid_s10,
        &ue_context->guti);
  }

  bearer_context_new_t *bc_test = NULL;
  /** List all bearer contexts. */
  LIST_FOREACH(bc_test, ue_context->ue_bearer_pool->free_bearers, entries){
    OAILOG_TRACE (LOG_MME_APP, "Current bearer context %p with ebi %d for pdn_context %p for APN \"%s\" and cid=%d for UE: " MME_UE_S1AP_ID_FMT ". \n",
        bc_test, bc_test->ebi, (*pdn_context_pp), bdata((*pdn_context_pp)->apn_subscribed),
        (*pdn_context_pp)->context_identifier, ue_id);
  }
  /*
   * Check if an APN configuration exists. If so, use it to update the fields.
   */
  mme_app_pdn_context_init(ue_context, (*pdn_context_pp));
  /** Get the default bearer context directly. */
  LIST_REMOVE(free_bearer, entries);
  DevAssert(free_bearer);
  AssertFatal((EPS_BEARER_IDENTITY_LAST >= free_bearer->ebi) && (EPS_BEARER_IDENTITY_FIRST <= free_bearer->ebi), "Bad ebi %u", free_bearer->ebi);
  /* Check that there is no collision when adding the bearer context into the PDN sessions bearer pool. */
  /* Insert the bearer context. */
  LIST_INSERT_HEAD((*pdn_context_pp)->session_bearers, free_bearer, entries);

  OAILOG_INFO(LOG_MME_APP, "Received first default bearer context %p with ebi %d for apn \"%s\" of UE: " MME_UE_S1AP_ID_FMT ". \n", free_bearer, free_bearer->ebi, bdata(apn_subscribed), ue_id);

  (*pdn_context_pp)->default_ebi = free_bearer->ebi;
  ue_context->next_def_ebi_offset++;
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
esm_cause_t
mme_app_update_pdn_context(mme_ue_s1ap_id_t ue_id, const subscription_data_t * const subscription_data, eps_bearer_context_status_t * const bc_status){
  OAILOG_FUNC_IN (LOG_MME_APP);

  ue_context_t        * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  pdn_context_t 	  * pdn_context = NULL, * pdn_context_safe = NULL;
  apn_configuration_t * apn_configuration = NULL;

  if(!ue_context){
    OAILOG_WARNING(LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT " to update the pdn context information from subscription data. \n", ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }

  /** Check if any PDN context is there (idle TAU) with failed S10. */

  // todo: LOCK_UE_CONTEXT_HERE
  /**
   * Iterate through all the pdn contexts of the UE.
   * Check if for each pdn context, an APN configuration exists.
   * If not remove those PDN contexts.
   */
  bool changed = false;
  do {
    RB_FOREACH_SAFE(pdn_context, PdnContexts, &ue_context->pdn_contexts, pdn_context_safe) { /**< Use the safe iterator. */
      /** Remove the pdn context. */
      changed = false;
      mme_app_select_apn(ue_context->imsi, pdn_context->apn_subscribed, &apn_configuration);
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
          pdn_context = RB_REMOVE(PdnContexts, &ue_context->pdn_contexts, pdn_context);
          DevAssert(pdn_context);
          pdn_context->context_identifier = apn_configuration->context_identifier;
          /** Set the context also to all the bearers. */
          bearer_context_new_t * bearer_context = NULL;
          LIST_FOREACH (bearer_context, pdn_context->session_bearers, entries) {
            bearer_context->pdn_cx_id = apn_configuration->context_identifier; /**< Update for all bearer contexts. */
          }
          /** Update the PDN context ambr only if it was 0. */
          if(!pdn_context->subscribed_apn_ambr.br_dl)
            pdn_context->subscribed_apn_ambr.br_dl = apn_configuration->ambr.br_dl;
          if(!pdn_context->subscribed_apn_ambr.br_ul)
            pdn_context->subscribed_apn_ambr.br_ul = apn_configuration->ambr.br_ul;

          DevAssert(!RB_INSERT (PdnContexts, &ue_context->pdn_contexts, pdn_context));
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
            "with ue_id " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT ". Triggering deactivation of PDN context. \n",
            bdata(pdn_context->apn_subscribed), ue_id, ue_context->imsi);

        /** Set the flag for the delete tunnel. */
        bool deleteTunnel = (RB_MIN(PdnContexts, &ue_context->pdn_contexts)== pdn_context);
        nas_itti_pdn_disconnect_req(ue_id, pdn_context->default_ebi, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, deleteTunnel, false,
            &pdn_context->s_gw_addr_s11_s4, pdn_context->s_gw_teid_s11_s4, pdn_context->context_identifier);
        /**
         * No response is expected.
         * Implicitly detach the PDN conte bearer contexts from the UE.
         */
        mme_app_delete_pdn_context(ue_context, &pdn_context);
        OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Invalid PDN context removed successfully ue_id " MME_UE_S1AP_ID_FMT ". \n", ue_id);
        continue;
      }
    }
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - " "Completed the checking of PDN contexts for ue_id " MME_UE_S1AP_ID_FMT ". \n", ue_id);
  } while(changed);

  // todo: UNLOCK_UE_CONTEXT
  /** Set the context identifier when updating the pdn_context. */
  OAILOG_INFO(LOG_NAS_EMM, "EMMCN-SAP  - " "Successfully updated all PDN contexts for UE " MME_UE_S1AP_ID_FMT". \n", ue_id);

  RB_FOREACH(pdn_context, PdnContexts, &ue_context->pdn_contexts) { /**< Use the safe iterator. */
	  bearer_context_new_t * bearer_context = NULL;
	  LIST_FOREACH (bearer_context, pdn_context->session_bearers, entries) {
		  (*bc_status) |= (0x01 << bearer_context->ebi);
	  }
  }
  (*bc_status) = ntohs(*bc_status);

  if(RB_EMPTY(&ue_context->pdn_contexts)){
	  OAILOG_ERROR(LOG_NAS_EMM, "EMMCN-SAP  - " "No PDN contexts left for UE " MME_UE_S1AP_ID_FMT" after updating for the received subscription information. \n", ue_id);
	  OAILOG_FUNC_RETURN (LOG_NAS_EMM, ESM_CAUSE_PDN_CONNECTION_DOES_NOT_EXIST);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, ESM_CAUSE_SUCCESS);
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
  bearer_context_new_t * bc_free = LIST_FIRST(ue_context->ue_bearer_pool->free_bearers);
  // todo: UNLOCK UE CONTEXTS
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
int
mme_app_esm_update_ebr_state(const mme_ue_s1ap_id_t ue_id, const bstring apn_subscribed, const pdn_cid_t pdn_cid, const ebi_t linked_ebi, const ebi_t bearer_ebi, esm_ebr_state ebr_state){
  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context_t        * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  pdn_context_t       * pdn_context = NULL;
  if(!ue_context){
    OAILOG_ERROR(LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT " to release \"%s\". \n", ue_id, bdata(apn_subscribed));
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }
  mme_app_get_pdn_context(ue_id, pdn_cid, linked_ebi, apn_subscribed, &pdn_context);
  if (!pdn_context) {
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-PROC  - PDN connection (cid=%d,linkeed_ebi=%d) and APN \"%s\" has not been allocated for UE "MME_UE_S1AP_ID_FMT". \n", pdn_cid, linked_ebi, bdata(apn_subscribed), ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }
  /** Get the default EBI. */
  bearer_context_new_t * bc = mme_app_get_session_bearer_context(pdn_context, bearer_ebi);
  bc->esm_ebr_context.status = ebr_state;
  // todo: UNLOCK UE CONTEXTS
  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
int
mme_app_pdn_process_session_creation(mme_ue_s1ap_id_t ue_id, fteid_t * saegw_s11_fteid, gtpv2c_cause_t *cause,
    bearer_contexts_created_t * bcs_created, ambr_t *ambr, paa_t ** paa, protocol_configuration_options_t * pco){

  OAILOG_FUNC_IN(LOG_MME_APP);

  pdn_context_t     * pdn_context = NULL;
  ue_context_t      * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context) {
    OAILOG_WARNING(LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT " to process CSResp. \n", ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }
  /** Get the first unestablished PDN context from the UE context. */
  RB_FOREACH (pdn_context, PdnContexts, &ue_context->pdn_contexts) {
    if(!pdn_context->s_gw_teid_s11_s4){
      /** Found. */
      break;
    }
  }
  if(!pdn_context || pdn_context->s_gw_teid_s11_s4){
    OAILOG_WARNING(LOG_MME_APP, "No unestablished PDN context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }
  // LOCK_UE_CONTEXT
  /** Set the S11 FTEID for each PDN connection. */
  pdn_context->s_gw_teid_s11_s4 = saegw_s11_fteid->teid;
  if(!ue_context->s_gw_teid_s11_s4)
    ue_context->s_gw_teid_s11_s4 = pdn_context->s_gw_teid_s11_s4;

  if(pco){
    if (!pdn_context->pco) {
      pdn_context->pco = calloc(1, sizeof(protocol_configuration_options_t));
    } else {
      clear_protocol_configuration_options(pdn_context->pco);
    }
    copy_protocol_configuration_options(pdn_context->pco, pco);
  }

  if(saegw_s11_fteid->ipv4){
	  ((struct sockaddr_in*)&pdn_context->s_gw_addr_s11_s4)->sin_addr.s_addr = saegw_s11_fteid->ipv4_address.s_addr;
	  ((struct sockaddr_in*)&pdn_context->s_gw_addr_s11_s4)->sin_family = AF_INET;
  } else {
	  ((struct sockaddr_in6*)&pdn_context->s_gw_addr_s11_s4)->sin6_family = AF_INET6;
	  memcpy(&((struct sockaddr_in6*)&pdn_context->s_gw_addr_s11_s4)->sin6_addr, &saegw_s11_fteid->ipv6_address, sizeof(saegw_s11_fteid->ipv6_address));
  }
  /** Check the received cause. */
  if(cause->cause_value != REQUEST_ACCEPTED && cause->cause_value != REQUEST_ACCEPTED_PARTIALLY){
    OAILOG_ERROR (LOG_MME_APP, "Received S11_CREATE_SESSION_RESPONSE REJECTION with cause value %d for ue " MME_UE_S1AP_ID_FMT "from S+P-GW. \n", cause->cause_value, ue_id);
    /** Destroy the PDN context (not in the ESM layer). */
    mme_app_esm_delete_pdn_context(ue_id, pdn_context->apn_subscribed, pdn_context->context_identifier, pdn_context->default_ebi); /**< Frees it & puts session bearers back to the pool. */
    // todo: UNLOCK
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
  }
  /** Process the success case, no bearer at this point. */
  if (*paa) {
    /** Set the PAA. */
    if(pdn_context->paa){
      free_wrapper((void**)&pdn_context->paa);
    }
    pdn_context->paa = *paa;
    /** Decouple it from the message. */
    *paa = NULL;
  }

  //#define TEMPORARY_DEBUG 1
  //#if TEMPORARY_DEBUG
  // bstring b = protocol_configuration_options_to_xml(&ue_context->pending_pdn_connectivity_req_pco);
  // OAILOG_DEBUG (LOG_MME_APP, "PCO %s\n", bdata(b));
  // bdestroy_wrapper(&b);
  //#endif

  /** Check the received APN-AMBR is in bounds (a subscription profile may exist or not, but these values must exist). */
  DevAssert(ue_context->subscribed_ue_ambr.br_dl && ue_context->subscribed_ue_ambr.br_ul);
  if(ambr->br_dl && ambr->br_ul) { /**< New APN-AMBR received. */
    ambr_t current_apn_ambr = mme_app_total_p_gw_apn_ambr_rest(ue_context, pdn_context->context_identifier);
    ambr_t total_apn_ambr;
    total_apn_ambr.br_dl = current_apn_ambr.br_dl + ambr->br_dl;
    total_apn_ambr.br_ul = current_apn_ambr.br_ul + ambr->br_ul;
    /** Actualized, used total APN-AMBR. */
    if(total_apn_ambr.br_dl > ue_context->subscribed_ue_ambr.br_dl || total_apn_ambr.br_ul > ue_context->subscribed_ue_ambr.br_ul){
      OAILOG_ERROR( LOG_MME_APP, "Received new APN_AMBR for PDN \"%s\" (ctx_id=%d) for UE " MME_UE_S1AP_ID_FMT " exceeds the subscribed ue ambr (br_dl=%d,br_ul=%d). \n",
    		  bdata(pdn_context->apn_subscribed), pdn_context->context_identifier, ue_id, ue_context->subscribed_ue_ambr.br_dl, ue_context->subscribed_ue_ambr.br_ul);
      /** Will use the current APN bitrates, either from the APN configuration received from the HSS or from S10. */
      // todo: inform PCRF, that default APN bitrates could not be updated.
      DevAssert(pdn_context->subscribed_apn_ambr.br_dl && pdn_context->subscribed_apn_ambr.br_ul);
      if(ue_context->mm_state == UE_REGISTERED) {
    	  /**
    	   * This case is only if it is a multi-APN, including the handover case.
    	   * Check if there is any AMBR left in the UE-AMBR, so the remaining and inform the PGW about it.
    	   * If not reject the request.
    	   */
    	  /** Check if any PDN connectivity procedure is running, if not reject the request. */
    	  nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity = mme_app_nas_esm_get_pdn_connectivity_procedure(ue_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED);
    	  if(esm_proc_pdn_connectivity == NULL){
    		  OAILOG_ERROR( LOG_MME_APP, "For APN \"%s\" (ctx_id=%d) for UE " MME_UE_S1AP_ID_FMT ", no PDN connectivity procedure is running.\n",
    				  bdata(pdn_context->apn_subscribed), pdn_context->context_identifier, ue_id);
    		  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
    	  }
    	  if((current_apn_ambr.br_dl >= ue_context->subscribed_ue_ambr.br_dl)
    			  || (current_apn_ambr.br_ul >= ue_context->subscribed_ue_ambr.br_ul)){
    		  OAILOG_ERROR( LOG_MME_APP, "No more AMBR left for UE " MME_UE_S1AP_ID_FMT ". Rejecting the request for an additional PDN (apn_subscribed=\"%s\", ctx_id=%d). \n",
    				  ue_id, bdata(pdn_context->apn_subscribed), pdn_context->context_identifier);
    		  cause->cause_value = NO_RESOURCES_AVAILABLE; /**< Reject the pdn connection in particular, remove the PDN context. */
//    		  mme_app_esm_delete_pdn_context(ue_id, pdn_context->apn_subscribed, pdn_context->context_identifier, pdn_context->default_ebi); /**< Frees it & puts session bearers back to the pool. */
    		  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
    	  }
    	  pdn_context->subscribed_apn_ambr.br_dl = ue_context->subscribed_ue_ambr.br_dl - current_apn_ambr.br_dl;
    	  pdn_context->subscribed_apn_ambr.br_ul = ue_context->subscribed_ue_ambr.br_ul - current_apn_ambr.br_ul;
    	  OAILOG_WARNING( LOG_MME_APP, "For the UE " MME_UE_S1AP_ID_FMT ", enforcing the remaining AMBR (br_dl=%d,br_ul=%d) on the additionally requested PDN (apn_subscribed=\"%s\", ctx_id=%d). \n",
    			  ue_id, pdn_context->subscribed_apn_ambr.br_dl, pdn_context->subscribed_apn_ambr.br_ul, bdata(pdn_context->apn_subscribed), pdn_context->context_identifier);
    	  /** Continue to process it. */
    	  esm_proc_pdn_connectivity->saegw_qos_modification = true;
      } else {
    	  /**
    	   * If it is a handover, or S10 triggered idle TAU, there should be no subscription data at this point. So reject it.
    	   *
    	   * Else, it is either initial attach or an initial tau without the S10 procedure. In that case the PDN should also be the default PDN.
    	   * Use the UE-AMBR for the PDN AMBR. Mark it as changed to update the AMBR in the PGW/PCRF.
    	   */
    	   subscription_data_t * subscription_data = mme_ue_subscription_data_exists_imsi(&mme_app_desc.mme_ue_contexts, ue_context->imsi);
    	   if(subscription_data && !(current_apn_ambr.br_dl | current_apn_ambr.br_ul)) {
    		   nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity = mme_app_nas_esm_get_pdn_connectivity_procedure(ue_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED);
    		   if(esm_proc_pdn_connectivity == NULL){
    			   OAILOG_ERROR( LOG_MME_APP, "For APN \"%s\" (ctx_id=%d) for UE " MME_UE_S1AP_ID_FMT ", no PDN connectivity procedure is running (initial attach/tau).\n",
    					   bdata(pdn_context->apn_subscribed), pdn_context->context_identifier, ue_id);
    			   OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
    		   }
    		   /* Put the remaining PDN AMBR as the UE AMBR. There should be no other PDN context allocated (no remaining AMBR). */
    		   pdn_context->subscribed_apn_ambr.br_dl = subscription_data->subscribed_ambr.br_dl;
    		   pdn_context->subscribed_apn_ambr.br_ul = subscription_data->subscribed_ambr.br_ul;
    		   /** Mark the procedure as modified. */
    		   esm_proc_pdn_connectivity->saegw_qos_modification = true;
    	   } else {
    		   /**
    		    * No subscription data exists. Assuming a handover/S10 TAU procedure (even if a (decoupled) subscription data exists.
    		    * Rejecting the handover or the S10 part of the idle TAU (continue with initial-TAU).
    		    */
    		   if(pdn_context->subscribed_apn_ambr.br_dl && pdn_context->subscribed_apn_ambr.br_ul){
        		   OAILOG_WARNING(LOG_MME_APP, "Using the PDN-AMBR (br_dl=%d, br_ul=%d) from handover for APN (apn_subscribed=\"%s\", ctx_id=%d) exceeds UE AMBR for UE " MME_UE_S1AP_ID_FMT " with IMSI "IMSI_64_FMT ". "
        				   "Rejecting the PDN connectivity for the inter-MME mobility procedure. \n",
						   pdn_context->subscribed_apn_ambr.br_dl, pdn_context->subscribed_apn_ambr.br_ul,
						   bdata(pdn_context->apn_subscribed), pdn_context->context_identifier, ue_id, ue_context->imsi);
        		   // todo: no ESM proc exists.
    		   } else {
        		   OAILOG_ERROR(LOG_MME_APP, "Requested PDN (apn_subscribed=\"%s\", ctx_id=%d) exceeds UE AMBR for UE " MME_UE_S1AP_ID_FMT " with IMSI "IMSI_64_FMT ". "
        				   "Rejecting the PDN connectivity for the inter-MME mobility procedure. \n", bdata(pdn_context->apn_subscribed), pdn_context->context_identifier, ue_id, ue_context->imsi);
        		   cause->cause_value = NO_RESOURCES_AVAILABLE; /**< Reject the request for this PDN connectivity procedure. */
        		   mme_app_esm_delete_pdn_context(ue_id, pdn_context->apn_subscribed, pdn_context->context_identifier, pdn_context->default_ebi); /**< Frees it & puts session bearers back to the pool. */
        		   OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
    		   }
    	   }
      }
    }else{
      OAILOG_DEBUG( LOG_MME_APP, "Received new valid APN_AMBR for APN \"%s\" (ctx_id=%d) for UE " MME_UE_S1AP_ID_FMT ". Updating APN ambr. \n",
          bdata(pdn_context->apn_subscribed), pdn_context->context_identifier, ue_id);
      pdn_context->subscribed_apn_ambr.br_dl = ambr->br_dl;
      pdn_context->subscribed_apn_ambr.br_ul = ambr->br_ul;
    }
  }

  /** Updated all PDN (bearer generic) information. Traverse all bearers, including the default bearer. */
  for (int i=0; i < bcs_created->num_bearer_context; i++) {
    ebi_t bearer_id = bcs_created->bearer_contexts[i].eps_bearer_id;
    bearer_context_created_t * bc_created = &bcs_created->bearer_contexts[i];
    bearer_context_new_t * bearer_context = mme_app_get_session_bearer_context(pdn_context, bearer_id);
    /*
     * Depending on s11 result we have to send reject or accept for bearers
     */
    DevCheck ((bearer_id < BEARERS_PER_UE + 5) && (bearer_id >= 5), bearer_id, BEARERS_PER_UE, 0);
    DevAssert (bcs_created->bearer_contexts[i].s1u_sgw_fteid.interface_type == S1_U_SGW_GTP_U);
    /** Check if the bearer could be created in the PGW. */
    if(bc_created->cause.cause_value != REQUEST_ACCEPTED){
      /** Check that it is a dedicated bearer. */
      DevAssert(pdn_context->default_ebi != bearer_id);
      /*
       * Release all session bearers of the PDN context back into the UE pool.
       */
      if(bearer_context){
        /** Initialize the new bearer context. */
        clear_bearer_context(ue_context->ue_bearer_pool, bearer_context);
        OAILOG_WARNING(LOG_MME_APP, "Successfully deregistered the bearer context (ebi=%d) from PDN \"%s\" and for ue_id " MME_UE_S1AP_ID_FMT "\n",
        		bearer_id, bdata(pdn_context->apn_subscribed), ue_id);
      }
      continue;
    }
    /** Bearer context could be established successfully. */
    if(bearer_context){
      bearer_context->bearer_state |= BEARER_STATE_SGW_CREATED;
      /** No context identifier might be set yet (multiple might exist and all might be 0. */
      //      AssertFatal((pdn_cx_id >= 0) && (pdn_cx_id < MAX_APN_PER_UE), "Bad pdn id for bearer");
      /*
       * Updating statistics
       */
      mme_app_desc.mme_ue_contexts.nb_bearers_managed++;
      mme_app_desc.mme_ue_contexts.nb_bearers_since_last_stat++;
      /** Update the FTEIDs of the SAE-GW. */
      memcpy(&bearer_context->s_gw_fteid_s1u, &bcs_created->bearer_contexts[i].s1u_sgw_fteid, sizeof(fteid_t)); /**< Also copying the IPv4/V6 address. */
      memcpy(&bearer_context->p_gw_fteid_s5_s8_up, &bcs_created->bearer_contexts[i].s5_s8_u_pgw_fteid, sizeof(fteid_t));
      /** Check if the bearer level QoS parameters have been modified by the PGW. */
      if (bcs_created->bearer_contexts[i].bearer_level_qos.qci &&
          bcs_created->bearer_contexts[i].bearer_level_qos.pl) {
        /**
         * We set them here, since we may not have a NAS context in (S10) mobility.
         * We don't check the subscribed HSS values. The PGW may ask for more.
         */
        bearer_context->bearer_level_qos.qci = bcs_created->bearer_contexts[i].bearer_level_qos.qci;
        bearer_context->bearer_level_qos.pl  = bcs_created->bearer_contexts[i].bearer_level_qos.pl;
        bearer_context->bearer_level_qos.pvi = bcs_created->bearer_contexts[i].bearer_level_qos.pvi;
        bearer_context->bearer_level_qos.pci = bcs_created->bearer_contexts[i].bearer_level_qos.pci;
        OAILOG_DEBUG (LOG_MME_APP, "Set qci %u in bearer %u\n", bearer_context->bearer_level_qos.qci, bearer_id);
      }
      /** Not touching TFT, ESM-EBR-State here. */
    } else {
      DevMessage("Bearer context that could be established successfully in the SAE-GW could not be found in the MME session bearers."); /**< This should not happen, since we lock the pdn_context.. */
//        continue;
    }
  }
  /** Assert that the default bearer at least exists. */
  DevAssert(mme_app_get_session_bearer_context(pdn_context, pdn_context->default_ebi));
  // todo: UNLOCK_UE_CONTEXT(ue_context)
  OAILOG_INFO(LOG_MME_APP, "Processed all %d bearer contexts for APN \"%s\" for ue_id " MME_UE_S1AP_ID_FMT ". \n", bcs_created->num_bearer_context, bdata(pdn_context->apn_subscribed), ue_id);
  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

//------------------------------------------------------------------------------
static void mme_app_pdn_context_init(ue_context_t * const ue_context, pdn_context_t *const  pdn_context)
{
  if ((pdn_context) && (ue_context)) {
    memset(pdn_context, 0, sizeof(*pdn_context));
    /** Initialize the session bearers map. */
    LIST_INIT(pdn_context->session_bearers);
  }
}

//------------------------------------------------------------------------------
static void mme_app_delete_pdn_context(ue_context_t * const ue_context, pdn_context_t ** pdn_context_pp){
  OAILOG_FUNC_IN (LOG_MME_APP);

  if(!(*pdn_context_pp)){
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }

  /** We found the PDN context, we will remove it directly from the UE context, remove & clean up the session bearers and free the PDN context. */
  pdn_context_t *pdn_context = RB_REMOVE(PdnContexts, &ue_context->pdn_contexts, *pdn_context_pp);
  DevAssert(pdn_context);

  /*
   * Release all session bearers of the PDN context back into the UE pool.
   */
  bearer_context_new_t * pBearerCtx = LIST_FIRST((*pdn_context_pp)->session_bearers);
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
    clear_bearer_context(ue_context->ue_bearer_pool, pBearerCtx);
    OAILOG_INFO(LOG_MME_APP, "Successfully deregistered the bearer context with ebi %d from PDN id %u and for ue_id " MME_UE_S1AP_ID_FMT "\n",
        pBearerCtx->ebi, (*pdn_context_pp)->context_identifier, ue_context->mme_ue_s1ap_id);
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

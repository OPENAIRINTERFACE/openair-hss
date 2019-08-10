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

/*! \file mme_app_session_context.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
  \author Dincer Beken
  \company Blackned GmbH
  \email: dbeken@blackned.de
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

//------------------------------------------------------------------------------
static void clear_session_pool(ue_session_pool_t * ue_session_pool) {
	/** Release the procedures. */
	if (ue_session_pool->s11_procedures) {
		mme_app_delete_s11_procedures(ue_session_pool);
	}

	/** Free the ESM procedures. */
	if(ue_session_pool->esm_procedures.pdn_connectivity_procedures){
		OAILOG_WARNING(LOG_MME_APP, "ESM PDN Connectivity procedures still exist for UE "MME_UE_S1AP_ID_FMT ". \n", ue_session_pool->mme_ue_s1ap_id);
	    mme_app_nas_esm_free_pdn_connectivity_procedures(ue_session_pool);
	}

	if(ue_session_pool->esm_procedures.bearer_context_procedures){
		OAILOG_WARNING(LOG_MME_APP, "ESM Bearer Context procedures still exist for UE "MME_UE_S1AP_ID_FMT ". \n", ue_session_pool->mme_ue_s1ap_id);
	    mme_app_nas_esm_free_bearer_context_procedures(ue_session_pool);
	}

	DevAssert(RB_EMPTY(&ue_session_pool->pdn_contexts));

	/** No need to check if list is full, since it is stacked. Just clear the allocations in each session context. */
	LIST_INIT(ue_session_pool->free_bearers);
	for(int num_bearer = 0; num_bearer < MAX_NUM_BEARERS_UE; num_bearer++) {
		clear_bearer_context(ue_session_pool, &ue_session_pool->bcs_ue[num_bearer]);
	}
	/** Re-initialize the empty list: it should not point to a next free variable: determined when it is put back into the list (like LIST_INIT). */
	ue_session_pool->next_free_sp = NULL;
}

//------------------------------------------------------------------------------
ue_session_pool_t * get_new_session_pool() {
	OAILOG_FUNC_IN (LOG_MME_APP);
	// todo: lock the mme_desc
	ue_session_pool_t * free_sp = NULL;
	free_sp = mme_app_desc.free_sp;
	if(!free_sp) {
		OAILOG_ERROR(LOG_NAS_EMM, "EMMCN-SAP  - " "No session pool left to allocate for UE.\n");
		OAILOG_FUNC_RETURN (LOG_MME_APP, NULL);
	}
	if(!free_sp->next_free_sp){
		/** Check if this is the last session pool. */
		if(mme_app_desc.free_sp != &mme_app_desc.ue_session_pool[CHANGEABLE_VALUE])
			mme_app_desc.free_sp = free_sp++;
		else {
			/** This was the last one. Don't specify a new value. */
			mme_app_desc.free_sp = NULL;
		}
	} else {
		mme_app_desc.free_sp = free_sp->next_free_sp;
	}
	// todo: unlock the mme_desc

	/** Initialize the bearers in the pool. */
	/** Remove the EMS-EBR context of the bearer-context. */
	if(free_sp){
		clear_session_pool(free_sp);
	}
	OAILOG_FUNC_RETURN (LOG_MME_APP, free_sp);
}

//------------------------------------------------------------------------------
void release_session_pool(ue_session_pool_t ** ue_session_pool) {
	OAILOG_FUNC_IN (LOG_MME_APP);

	/** Clear the UE session pool. */
	clear_session_pool(*ue_session_pool);
	// todo: lock the mme_desc (ue_context should already be locked)
	/** Set the current free one as the last received one. */
	ue_session_pool_t * last_free_sp = mme_app_desc.free_sp;
	/** Set the last one as the free one. */
	mme_app_desc.free_sp = *ue_session_pool;
	/** Check if there is a free one left. */
	(*ue_session_pool)->next_free_sp = last_free_sp; /**< May be null. */
	*ue_session_pool = NULL;
	// todo: unlock the mme_desc
}

//------------------------------------------------------------------------------
void
mme_app_esm_detach(mme_ue_s1ap_id_t ue_id){
  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_session_pool_t   * ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, ue_id);
  pdn_context_t       * pdn_context = NULL;

  if(!ue_session_pool){
    OAILOG_WARNING(LOG_MME_APP, "No UE session_pool could be found for UE: " MME_UE_S1AP_ID_FMT " to release ESM contexts. \n", ue_id);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  // todo: LOCK UE SESSION POOL
  mme_app_nas_esm_free_bearer_context_procedures(ue_session_pool);
  mme_app_nas_esm_free_pdn_connectivity_procedures(ue_session_pool);

  OAILOG_INFO(LOG_MME_APP, "Removed all ESM procedures of UE: " MME_UE_S1AP_ID_FMT " for detach. \n", ue_id);
  pdn_context = RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
  while(pdn_context){
    mme_app_delete_pdn_context(ue_session_pool, &pdn_context);
    pdn_context = RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
  }
  OAILOG_INFO(LOG_MME_APP, "Removed all ESM contexts of UE: " MME_UE_S1AP_ID_FMT " for detach. \n", ue_id);
  bearer_context_new_t * bc_free = LIST_FIRST(ue_session_pool->free_bearers);
  // todo: UNLOCK UE SESSION POOL
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
int
mme_app_pdn_process_session_creation(mme_ue_s1ap_id_t ue_id, imsi64_t imsi, mm_state_t mm_state, ambr_t subscribed_ue_ambr, fteid_t * saegw_s11_fteid, gtpv2c_cause_t *cause,
    bearer_contexts_created_t * bcs_created, ambr_t *ambr, paa_t ** paa, protocol_configuration_options_t * pco){

  OAILOG_FUNC_IN(LOG_MME_APP);

  pdn_context_t    		 * pdn_context = NULL;
  ue_session_pool_t      * ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, ue_id);
  if(!ue_session_pool) {
    OAILOG_WARNING(LOG_MME_APP, "No MME_APP UE session pool could be found for UE: " MME_UE_S1AP_ID_FMT " to process CSResp. \n", ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }
  /** Get the first unestablished PDN context from the UE context. */
  RB_FOREACH (pdn_context, PdnContexts, &ue_session_pool->pdn_contexts) {
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
  DevAssert(subscribed_ue_ambr.br_dl && subscribed_ue_ambr.br_ul);
  if(ambr->br_dl && ambr->br_ul) { /**< New APN-AMBR received. */
    ambr_t current_apn_ambr = mme_app_total_p_gw_apn_ambr_rest(ue_session_pool, pdn_context->context_identifier);
    ambr_t total_apn_ambr;
    total_apn_ambr.br_dl = current_apn_ambr.br_dl + ambr->br_dl;
    total_apn_ambr.br_ul = current_apn_ambr.br_ul + ambr->br_ul;
    /** Actualized, used total APN-AMBR. */
    if(total_apn_ambr.br_dl > subscribed_ue_ambr.br_dl || total_apn_ambr.br_ul > subscribed_ue_ambr.br_ul){
      OAILOG_ERROR( LOG_MME_APP, "Received new APN_AMBR for PDN \"%s\" (ctx_id=%d) for UE " MME_UE_S1AP_ID_FMT " exceeds the subscribed ue ambr (br_dl=%d,br_ul=%d). \n",
    		  bdata(pdn_context->apn_subscribed), pdn_context->context_identifier, ue_id, subscribed_ue_ambr.br_dl, subscribed_ue_ambr.br_ul);
      /** Will use the current APN bitrates, either from the APN configuration received from the HSS or from S10. */
      // todo: inform PCRF, that default APN bitrates could not be updated.
      DevAssert(pdn_context->subscribed_apn_ambr.br_dl && pdn_context->subscribed_apn_ambr.br_ul);
      if(mm_state == UE_REGISTERED) {
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
    	  if((current_apn_ambr.br_dl >= subscribed_ue_ambr.br_dl)
    			  || (current_apn_ambr.br_ul >= subscribed_ue_ambr.br_ul)){
    		  OAILOG_ERROR( LOG_MME_APP, "No more AMBR left for UE " MME_UE_S1AP_ID_FMT ". Rejecting the request for an additional PDN (apn_subscribed=\"%s\", ctx_id=%d). \n",
    				  ue_id, bdata(pdn_context->apn_subscribed), pdn_context->context_identifier);
    		  cause->cause_value = NO_RESOURCES_AVAILABLE; /**< Reject the pdn connection in particular, remove the PDN context. */
//    		  mme_app_esm_delete_pdn_context(ue_id, pdn_context->apn_subscribed, pdn_context->context_identifier, pdn_context->default_ebi); /**< Frees it & puts session bearers back to the pool. */
    		  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
    	  }
    	  pdn_context->subscribed_apn_ambr.br_dl = subscribed_ue_ambr.br_dl - current_apn_ambr.br_dl;
    	  pdn_context->subscribed_apn_ambr.br_ul = subscribed_ue_ambr.br_ul - current_apn_ambr.br_ul;
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
    	   subscription_data_t * subscription_data = mme_ue_subscription_data_exists_imsi(&mme_app_desc.mme_ue_contexts, imsi);
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
        		   OAILOG_WARNING(LOG_MME_APP, "Using the PDN-AMBR (br_dl=%d, br_ul=%d) from handover for APN (apn_subscribed=\"%s\", ctx_id=%d) exceeds UE AMBR for UE " MME_UE_S1AP_ID_FMT "."
        				   "Rejecting the PDN connectivity for the inter-MME mobility procedure. \n",
						   pdn_context->subscribed_apn_ambr.br_dl, pdn_context->subscribed_apn_ambr.br_ul,
						   bdata(pdn_context->apn_subscribed), pdn_context->context_identifier, ue_id);
        		   // todo: no ESM proc exists.
    		   } else {
        		   OAILOG_ERROR(LOG_MME_APP, "Requested PDN (apn_subscribed=\"%s\", ctx_id=%d) exceeds UE AMBR for UE " MME_UE_S1AP_ID_FMT " with IMSI "IMSI_64_FMT ". "
        				   "Rejecting the PDN connectivity for the inter-MME mobility procedure. \n", bdata(pdn_context->apn_subscribed), pdn_context->context_identifier, ue_id, imsi);
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
        clear_bearer_context(ue_session_pool, bearer_context);
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
      mme_app_desc.mme_ue_session_pools.nb_bearers_managed++;
      mme_app_desc.mme_ue_session_pools.nb_bearers_since_last_stat++;
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
  // todo: UNLOCK_UE_SESSION_POOL
  OAILOG_INFO(LOG_MME_APP, "Processed all %d bearer contexts for APN \"%s\" for ue_id " MME_UE_S1AP_ID_FMT ". \n",
		  bcs_created->num_bearer_context, bdata(pdn_context->apn_subscribed), ue_id);
  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
ambr_t mme_app_total_p_gw_apn_ambr(ue_session_pool_t *ue_session_pool){
  pdn_context_t * registered_pdn_ctx = NULL;
  ambr_t apn_ambr_sum= {0, 0};
  RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_session_pool->pdn_contexts) {
    DevAssert(registered_pdn_ctx);
    apn_ambr_sum.br_dl += registered_pdn_ctx->subscribed_apn_ambr.br_dl;
    apn_ambr_sum.br_ul += registered_pdn_ctx->subscribed_apn_ambr.br_ul;
  }
  return apn_ambr_sum;
}

//------------------------------------------------------------------------------
ambr_t mme_app_total_p_gw_apn_ambr_rest(ue_session_pool_t *ue_session_pool, pdn_cid_t pci){
  /** Get the total APN AMBR excluding the given PCI. */
  pdn_context_t * registered_pdn_ctx = NULL;
  ambr_t apn_ambr_sum= {0, 0};
  RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_session_pool->pdn_contexts) {
    DevAssert(registered_pdn_ctx);
    if(registered_pdn_ctx->context_identifier == pci)
      continue;
    apn_ambr_sum.br_dl += registered_pdn_ctx->subscribed_apn_ambr.br_dl;
    apn_ambr_sum.br_ul += registered_pdn_ctx->subscribed_apn_ambr.br_ul;
  }
  return apn_ambr_sum;
}

//------------------------------------------------------------------------------
void mme_app_ue_session_pool_s1_release_enb_informations(mme_ue_s1ap_id_t ue_id)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_session_pool_t      * ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, ue_id);

  // LOCK UE_SESSION
  pdn_context_t * registered_pdn_ctx = NULL;
  /** Update all bearers and get the pdn context id. */
  RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_session_pool->pdn_contexts) {
    DevAssert(registered_pdn_ctx);
    /*
     * Get the first PDN whose bearers are not established yet.
     * Do the MBR just one PDN at a time.
     */
    bearer_context_new_t * bearer_context_to_set_idle = NULL, * bearer_context_to_set_idle_safe = NULL;
    LIST_FOREACH_SAFE (bearer_context_to_set_idle, registered_pdn_ctx->session_bearers, entries, bearer_context_to_set_idle_safe) {
      DevAssert(bearer_context_to_set_idle);
      /** Add them to the bearears list of the MBR. */
      mme_app_bearer_context_s1_release_enb_informations(bearer_context_to_set_idle);
    }
  }

  // UNLOCK UE_SESSION
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
ue_session_pool_t                           *
mme_ue_session_pool_exists_mme_ue_s1ap_id (
  mme_ue_session_pool_t * const mme_ue_session_pool_p,
  const mme_ue_s1ap_id_t mme_ue_s1ap_id)
{
  struct ue_session_pool_s                    *ue_session_pool = NULL;

  hashtable_ts_get (mme_ue_session_pool_p->mme_ue_s1ap_id_ue_session_pool_htbl, (const hash_key_t)mme_ue_s1ap_id, (void **)&ue_session_pool);
  if (ue_session_pool) {
//    lock_ue_contexts(ue_context);
//    OAILOG_TRACE (LOG_MME_APP, "UE  " MME_UE_S1AP_ID_FMT " fetched MM state %s, ECM state %s\n ",mme_ue_s1ap_id,
//        (ue_context->mm_state == UE_UNREGISTERED) ? "UE_UNREGISTERED":(ue_context->mm_state == UE_REGISTERED) ? "UE_REGISTERED":"UNKNOWN",
//        (ue_context->ecm_state == ECM_IDLE) ? "ECM_IDLE":(ue_context->ecm_state == ECM_CONNECTED) ? "ECM_CONNECTED":"UNKNOWN");
  }
  return ue_session_pool;
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/


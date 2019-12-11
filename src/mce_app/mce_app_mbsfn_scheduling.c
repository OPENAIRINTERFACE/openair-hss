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

/*! \file mce_app_mbsfn_scheduling.c
  \brief
  \author Dincer Beken
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <inttypes.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "gcc_diag.h"
#include "dynamic_memory_check.h"
#include "assertions.h"
#include "log.h"
#include "msc.h"
#include "3gpp_requirements_36.413.h"
#include "common_types.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "dlsch_tbs_full.h"
#include "mme_config.h"
#include "enum_string.h"
#include "timer.h"
#include "mce_app_mbms_service_context.h"
#include "mce_app_defs.h"
#include "mce_app_itti_messaging.h"
#include "mme_app_procedures.h"
#include "common_defs.h"

// todo: think about locking the MCE_APP context or EMM context, which one to lock, why to lock at all? lock seperately?
////------------------------------------------------------------------------------
//int lock_ue_contexts(mbms_service_t * const ue_context) {
//  int rc = RETURNerror;
//  if (ue_context) {
//    struct timeval start_time;
//    gettimeofday(&start_time, NULL);
//    struct timespec wait = {0}; // timed is useful for debug
//    wait.tv_sec=start_time.tv_sec + 5;
//    wait.tv_nsec=start_time.tv_usec*1000;
//    rc = pthread_mutex_timedlock(&ue_context->recmutex, &wait);
//    if (rc) {
//      OAILOG_ERROR (LOG_MCE_APP, "Cannot lock UE context mutex, err=%s\n", strerror(rc));
//#if ASSERT_MUTEX
//      struct timeval end_time;
//      gettimeofday(&end_time, NULL);
//      AssertFatal(!rc, "Cannot lock UE context mutex, err=%s took %ld seconds \n", strerror(rc), end_time.tv_sec - start_time.tv_sec);
//#endif
//    }
//#if DEBUG_MUTEX
//    OAILOG_TRACE (LOG_MCE_APP, "UE context mutex locked, count %d lock %d\n",
//        ue_context->recmutex.__data.__count, ue_context->recmutex.__data.__lock);
//#endif
//  }
//  return rc;
//}
////------------------------------------------------------------------------------
//int unlock_ue_contexts(mbms_service_t * const ue_context) {
//  int rc = RETURNerror;
//  if (ue_context) {
//    rc = pthread_mutex_unlock(&ue_context->recmutex);
//    if (rc) {
//      OAILOG_ERROR (LOG_MCE_APP, "Cannot unlock UE context mutex, err=%s\n", strerror(rc));
//    }
//#if DEBUG_MUTEX
//    OAILOG_TRACE (LOG_MCE_APP, "UE context mutex unlocked, count %d lock %d\n",
//        ue_context->recmutex.__data.__count, ue_context->recmutex.__data.__lock);
//#endif
//  }
//  return rc;
//}
//
/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

//------------------------------------------------------------------------------
static
int mce_app_get_mch_mcs(mbsfn_area_context_t * const mbsfn_area_context, const qci_e qci);

//------------------------------------------------------------------------------
static
void mce_app_reuse_csa_pattern(struct csa_patterns_s * csa_patterns_mbsfn_p, mchs_t * mchs, const struct csa_patterns_s * const csa_patterns_alloced, const struct mbsfn_area_context_s * const mbsfn_area_ctx);

//------------------------------------------------------------------------------
static
int mce_app_alloc_csa_pattern(struct csa_patterns_s * new_csa_patterns,
		struct mchs_s * mchs, const uint8_t total_csa_pattern_offset,
		const struct mbsfn_area_context_s * mbsfn_area_ctx);

//------------------------------------------------------------------------------
static
int mce_app_calculate_mbsfn_csa_patterns(struct csa_patterns_s * const csa_patterns_mbsfn_p,
	const struct csa_patterns_s * const csa_patterns_included, const uint8_t excluded_csa_pattern_offset,
	const struct csa_pattern_s * const csa_pattern_common, mchs_t * const mchs, const struct mbsfn_area_context_s * const mbsfn_area_ctx);

//------------------------------------------------------------------------------
static
int mce_app_log_method_single_rf_csa_pattern(struct csa_patterns_s * new_csa_patterns,
		int *num_radio_frames_p,
		struct mchs_s * mchs, const struct mbsfn_area_context_s * const mbsfn_area_context,
		uint8_t total_offset_allocated);

//------------------------------------------------------------------------------
static
void mce_app_calculate_mbsfn_mchs(const struct mbsfn_area_context_s * const mbsfn_area_context,
		const mbms_service_indexes_t * const mbms_service_indexes_active,
		mchs_t *const mchs);

//------------------------------------------------------------------------------
static void mce_app_allocate_4frame(struct csa_patterns_s * new_csa_patterns, int * num_radio_frames_p, struct mchs_s * mchs, const struct mbsfn_area_context_s * const mbsfn_area_context, struct csa_patterns_s * csa_patterns_allocated);

//------------------------------------------------------------------------------
static
void mce_app_set_fresh_radio_frames(struct csa_pattern_s * csa_pattern, struct mchs_s * mchs, const uint8_t csa_sf_available);

//------------------------------------------------------------------------------
static
void mce_app_reuse_csa_pattern_set_subframes(struct csa_pattern_s * csa_pattern_mbsfn, struct csa_pattern_s * csa_pattern, struct mchs_s * const mchs, int *mch_subframes_to_be_scheduled_p,
		const struct mbsfn_area_context_s * const mbsfn_area_ctx);

//------------------------------------------------------------------------------
static int
mce_app_assign_mch_subframes(const struct csa_patterns_s * const csa_patterns_mbsfn,
		const struct mchs_s 				 * mchs,
		const mbsfn_area_context_t 	 * const mbsfn_area_context,
		const mbsfn_area_cfg_t 			 * const mbsfn_area_cfg,
		const mbms_service_indexes_t * const mbms_service_indexes_active_p);

/**
 * Method that calculate the CSA pattern considering MBSFN clusters with NLG MBSFN areas & local MBSFN areas.
 * Will check the local-global flag and assert that both nl-global and local MBSFN areas does not exist.
 * Then it will just then the assigned sfAlloc subframes of the MCCHs of the MBSFN areas.
 *
 * We check the assigned sf-Alloc subframes when new MBSFN areas are created.
 * If the local-global flag is NOT sent, we will reserve subframes for all configured global MBMS service areas.
 * If the eNB capacity only allows 2 subframes (TDD) in a CSA pattern, and 2 global MBSFN areas area configured: only 2 global MBSFN areas
 * can exist, no local MBMS areas.
 */
//------------------------------------------------------------------------------
int mce_app_calculate_csa_common_pattern(	const mbsfn_area_ids_t							* nlglobal_mbsfn_area_ids,
	const mbsfn_area_ids_t							* local_mbsfn_area_ids,
	struct csa_pattern_s 								* const common_csa_pattern)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	mbsfn_area_context_t					*mbsfn_area_context = NULL;

	/** Assert that the local-global flag is NOT set in the MME config. */
	mme_config_read_lock (&mme_config);
	if(mme_config.mbms.mbms_global_mbsfn_area_per_local_group){
		if(nlglobal_mbsfn_area_ids && local_mbsfn_area_ids){
			OAILOG_ERROR(LOG_MCE_APP, "If local-global flag is set, we cannot have an MBSFN cluster with local and nl-global MBSFN areas.\n");
			mme_config_unlock(&mme_config);
			OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
		}
	}
	mme_config_unlock(&mme_config);

	/**
	 * Calculate the CSA pattern based on the given MBSFN areas.
	 * It is transparent to this function, whethever a subframe is reserved or assigned. We just consider the assigned ones.
	 * They need to be checked/handled at MBSFN area creation//M2-eNodeB setup procedure.
	 * (Assume already done at this point, and all MBSFN areas can have a place in the common CSA pattern!
	 *
	 * Just check the assigned sf-AllocInfo of the global MBSFN areas, even if they don't have an MBMS service.
	 * All active MBSFN areas are given in the argument!
	 */
	if(nlglobal_mbsfn_area_ids && nlglobal_mbsfn_area_ids->num_mbsfn_area_ids){
		for(int num_global_mbsfnarea = 0; num_global_mbsfnarea < nlglobal_mbsfn_area_ids->num_mbsfn_area_ids; num_global_mbsfnarea++){
			/** Get the MBSFN area context. */
			mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, nlglobal_mbsfn_area_ids->mbsfn_area_id[num_global_mbsfnarea]);
			DevAssert(mbsfn_area_context);
			/** Get the sfAllocPattern of the NL-Global MBSFN area. */
			uint8_t mbms_mcch_subframe = mbsfn_area_context->privates.fields.mbsfn_area.mbms_mcch_subframes;
			DevAssert(mbms_mcch_subframe);
			/** Assert that the MBMS MCCH subframes don't overlap. */
			DevAssert(!(common_csa_pattern->csa_pattern_sf.mbms_mch_csa_pattern_1rf & mbms_mcch_subframe));
			/** Add the global MBSFN area into the first pattern. Assign the resources outside. */
			common_csa_pattern->csa_pattern_sf.mbms_mch_csa_pattern_1rf |= mbms_mcch_subframe;
			/** Remaining subframes in the CSA pattern repetitions will be set later. */
		}
	}
	OAILOG_INFO(LOG_MCE_APP, "Common CSA pattern after handling non-local global MBSFN areas: (%x).\n", common_csa_pattern->csa_pattern_sf.mbms_mch_csa_pattern_1rf);
	/**
	 * Check the remaining local MBSFN areas given in the list.
	 * Assign a subframe, only if the MBSFN area is active. No matter if MBMS services exist or not.
	 */
	if(local_mbsfn_area_ids && local_mbsfn_area_ids->num_mbsfn_area_ids){
		for(int num_local_mbsfnarea = 0; num_local_mbsfnarea < local_mbsfn_area_ids->num_mbsfn_area_ids; num_local_mbsfnarea++){
			/** Get the MBSFN area context. */
			mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, local_mbsfn_area_ids->mbsfn_area_id[num_local_mbsfnarea]);
			DevAssert(mbsfn_area_context);
			/** Get the sfAllocPattern of the local MBSFN area. */
			uint8_t mbms_mcch_subframe = mbsfn_area_context->privates.fields.mbsfn_area.mbms_mcch_subframes;
			DevAssert(mbms_mcch_subframe);
			/** Assert that the MBMS MCCH subframes don't overlap, also not with the non-local global MBSFN areas. */
			DevAssert(!(common_csa_pattern->csa_pattern_sf.mbms_mch_csa_pattern_1rf & mbms_mcch_subframe));
			/** Add the global MBSFN area into the first pattern. Assign the resources outside. */
			common_csa_pattern->csa_pattern_sf.mbms_mch_csa_pattern_1rf |= mbms_mcch_subframe;
			/** Remaining subframes in the CSA pattern repetitions will be set later. */
		}
	}
	OAILOG_INFO(LOG_MCE_APP, "Common CSA pattern after handling local MBSFN areas: (%x). No MCH resources are allocated yet. \n",
			common_csa_pattern->csa_pattern_sf.mbms_mch_csa_pattern_1rf);
	/** Assign the generic values of the common CSA pattern. */
	common_csa_pattern->csa_pattern_offset_rf = COMMON_CSA_PATTERN;
	common_csa_pattern->mbms_csa_pattern_rfs 	= CSA_ONE_FRAME;
	/**
	 * One Frame CSA pattern only occurs in every 8RFs. Shared by all MBSFN areas. Fixed! No matter if FDD or TDD or TDD DL/UL configuration.
	 * Use the maximum repetition, since we cannot allocate the RF offset with any other CSA pattern. Resources would remain unused.
	 */
	common_csa_pattern->csa_pattern_repetition_period_rf = get_csa_rf_alloc_period_rf(CSA_RF_ALLOC_PERIOD_RF8);
	/** @configuration time, the FDD/TDD format and the TDD DL/UL configuration is set. */
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

//------------------------------------------------------------------------------
int
mce_app_schedule_mbsfn_resources(const mbsfn_area_ids_t			* mbsfn_area_ids,
		const struct csa_pattern_s														  * csa_pattern_common,
		const struct csa_patterns_s 														* csa_patterns_union_p,
		const uint8_t 																					 	excluded_csa_pattern_offset,
		long		 												 													mcch_rep_abs_rf,
		/**< MBSFN areas to be set with MCH/CSA scheduling information (@MCCH modify timeout). */
		struct mbsfn_areas_s							  										* const mbsfn_areas_to_schedule)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	mbsfn_area_context_t 						*mbsfn_area_context = NULL;
	mcch_modification_periods_t		   mcch_modification_periods		= {0};

	/**
	 * Calculate the MCHs of the given non-local global MBMS services.
	 * Take the first MBSFN area context, to get the phy layer properties, for all global MBMS services..
	 */
	if(!mbsfn_area_ids && !mbsfn_area_ids->num_mbsfn_area_ids){
		OAILOG_WARNING(LOG_MCE_APP, "Cannot schedule MBSFN resources. No MBSFN Area Id list has been received.\n");
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
	}

	/**
	 * Iterate over all MBSFN areas of the given group.
	 * Calculate the resources for the MBSFN areas.
	 */
	for(int num_mbsfn_area = 0; num_mbsfn_area < mbsfn_area_ids->num_mbsfn_area_ids; num_mbsfn_area++)
	{
		struct csa_patterns_s 			csa_patterns_mbsfn 							= {0};
		/** Continue with the last assigned MBSFN area configuration. */
		mchs_t 										 *mchs_p 													= &mbsfn_areas_to_schedule->mbsfn_area_cfg[mbsfn_areas_to_schedule->num_mbsfn_areas].mchs;
		mbms_service_indexes_t			mbms_service_indexes_active 		= {0},
																mbms_service_indexes_active_p 	= &mbms_service_indexes_active;
		/** Calculate the MCHs independently of the MBMS services. */
		mbsfn_area_context = mce_mbms_service_exists_mbms_service_index(&mce_app_desc.mce_mbms_service_contexts, mbsfn_area_ids->mbsfn_area_id[num_mbsfn_area]);
		DevAssert(mbsfn_area_context);
		/**
		 * Get the CSA pattern for each group.
		 * If local-global flag is not set, consider the global MBSFN areas also in the local groups for the CSA pattern calculation.
		 * Calculate the CSA pattern for the non-local global MBSFN areas (lg-flag independent).
		 * Check if we have non-local global MBSFN areas to check, then make a list of active MBMS services, including the newest one, if its a nl-global.
		 *
		 * todo: do a cluster structure and put this element into the cluster!
		 * todo: in the cluster, the global/local resources should ALL have the same MCCH modification period!
		 */
		mcch_modification_periods->mcch_modif_start_abs_period 				= mcch_rep_abs_rf / mbsfn_area_context->privates.fields.mbsfn_area.mcch_modif_period_rf;
		mcch_modification_periods->mcch_modif_stop_abs_period 				= mcch_rep_abs_rf / mbsfn_area_context->privates.fields.mbsfn_area.mcch_modif_period_rf;
		/** Get active MBMS services for the given MCCH modification period of the MBSFN area. */
		hashtable_apply_callback_on_elements(mbsfn_area_context->privates.mbms_service_idx_mcch_modification_times_hashmap,
				mce_app_get_active_mbms_services_per_mbsfn_area, (void*)mcch_modification_periods, (void**)&mbms_service_indexes_active_p);
		if(!mbms_service_indexes_active_p->num_mbms_service_indexes){
			OAILOG_WARNING(LOG_MCE_APP, "No MBMS services could be active in MCCH repetition/modification period (abs) (%d)/(%d) for MBSFN Area Id " MBSFN_AREA_ID_FMT" for local MBMS area (%d). "  +
					"Assigning only MCCH subframes from COMMON_CSA into resulting CSA patterns. \n",
					mcch_rep_abs_rf, mcch_modification_periods->mcch_modif_start_abs_period, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id,
					mbsfn_area_context->privates.fields.local_mbms_area);
			continue;
		}
		/**
		 * Calculate the MCHs for this MBSFN area from the given list of active MBMS services.
		 * Filter from the list the MBMS services, which belong to the MBSFN area.
		 */
		mce_app_calculate_mbsfn_mchs(mbsfn_area_context, mbms_service_indexes_active_p, mchs_p);
		if(!mchs_p->total_subframes_per_csa_period_necessary) {
			OAILOG_ERROR(LOG_MCE_APP, "No MCH subframes calculated for MBSFN Area " MBSFN_AREA_ID_FMT " in local-mbms-area (%d). \n",
					mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id, mbsfn_area_context->privates.fields.local_mbms_area);
			DevAssert(0);
		}

		/**
		 * Method below also sets the unused subframes in the COMMON_CSA pattern, depending on the MBSFNs of the first pattern.
		 * The resulting values are taken as absolute, and it is not important if local-global flag is set or not.
		 */
		if(mce_app_calculate_mbsfn_csa_patterns(&csa_patterns_mbsfn, csa_patterns_union_p, excluded_csa_pattern_offset, csa_pattern_common, mchs_p, mbsfn_area_context) == RETURNerror)
		{
			OAILOG_ERROR(LOG_MCE_APP, "CSA patterns for MBSFN Area " MBSFN_AREA_ID_FMT " for local MBMS area (%d) could not be fitted into resources.\n",
					mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id, mbsfn_area_context->privates.fields.local_mbms_area);
			/** Could not fit the MBSFN area, we return false and let the upper method perform ARP preemption, if possible. */
			OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
		}

		/**
		 * Update the overall CSA pattern.
		 * We don't have to explicitly insert the common-CSA pattern, it is included in the csa_patterns_mbsfn.
		 */
		mce_app_update_csa_pattern_union(csa_patterns_union_p, &csa_patterns_mbsfn);
		/** Assign the MCH subframes, for MBSFN services, whose MCCH modification period has been reached. */
		if(mcch_rep_abs_rf % mbsfn_area_context->privates.fields.mbsfn_area.mcch_modif_period_rf){
			OAILOG_DEBUG(LOG_MCE_APP, "MBSFN Area " MBSFN_AREA_ID_FMT " in local MBMS area (%d) has not reached MCCH modification for MCCH repetition RF (%d).\n",
					mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id, mbsfn_area_context->privates.fields.local_mbms_area, mcch_rep_abs_rf);
				/** Consider the MBSFN area for resource check but no need to schedule any MCHs. */
			continue;
		}
		/** Assign the MCH subframes. */
		if(!mce_app_assign_mch_subframes(&csa_patterns_mbsfn, mchs_p, mbsfn_area_context, &mbsfn_areas_to_schedule->mbsfn_area_cfg[num_mbsfn_area],
				mbms_service_indexes_active_p) == RETURNerror)
		{
			OAILOG_ERROR(LOG_MCE_APP, "Error while assigning resources in CSA patterns of MBSFN Area " MBSFN_AREA_ID_FMT " in local MBMS area (%d) to the MCHs.\n",
					mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id, mbsfn_area_context->privates.fields.local_mbms_area);
			/** Could not fit the MBSFN area, we return false and let the upper method perform ARP preemption, if possible. */
			DevAssert(0);
		}
		mbsfn_areas_to_schedule->num_mbsfn_areas++;
		OAILOG_INFO(LOG_MCE_APP, "Successfully scheduled the resources of the MBSFN area " MBSFN_AREA_ID_FMT" and local MBMS area (%d).\n",
				mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id, mbsfn_area_context->privates.fields.local_mbms_area);
	}
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

/**
 * We check the resulting MBSFN areas, where the capacity should be split among and calculate the resources needed.
 * Finally, we fill an CSA pattern offset bitmap.
 * MBSFN Areas might share CSA Patterns (Radio Frames), but once a subframe is set for a given periodicity,
 * the subframe will be allocated for a SINGLE MBSFN area only. The different CSA patterns will have a different offset.
 * In total 8 CSA patterns can exist for all MBSFN areas.
 *
 * No matter if the local-global flag is set or with which local active MBMS services we calculate the resources: the global
 * MBSFN areas resource calculation should always be the same!
 *
 * Amongs all cluster, which share resources, the COMMON_CSA pattern [7] will contain the same non-local global MBSFN areas.
 * In total, we can assign up to 6 MBSFN areas to the COMMON_CSA pattern (to the last repetition).
 *
 * Subframe indicating the MCCH will be set at M2 Setup (initialization).
 * ToDo: It also must be unique against different MBSFN area combinations!
 * In the strict order, calculate first the global CSA pattern, if exists.
 * Then calculate the local CSA pattern. Regard the global CSA pattern and don't mix them!
 * If we have multiple clusters, that share resources, global MBSFN areas have always the same CSA pattern, independent of the locals!
 */
//------------------------------------------------------------------------------
int mce_app_check_mbsfn_cluster_resources (const mbsfn_area_ids_t							* nlglobal_mbsfn_area_ids,
		const mbsfn_area_ids_t							* local_mbsfn_area_ids, /**< Contains also local global. */
		const mbms_service_indexes_t				* const mbms_service_indexes_active_nlg_p,
		const mbms_service_indexes_t				* const mbms_service_indexes_active_local_p)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);

	struct csa_pattern_s												  csa_pattern_common 			= {0};
	struct csa_patterns_s 		  	 								csa_patterns_global			= {0};
	struct csa_patterns_s 		  	 								csa_patterns_local			= {0};
	const mbsfn_area_context_t 									 *mbsfn_area_context      = NULL;
	/**
	 * We first create the pattern for all received local and nl-global mbms areas the first COMMON_CSA[7] subpattern.
	 * Later, below, we first try to fill the nl-global, afterwards the local areas.
	 */
	if(mce_app_calculate_csa_common_pattern(nlglobal_mbsfn_area_ids, local_mbsfn_area_ids, &csa_pattern_common) == RETURNerror){
		DevMessage("Error during common CSA calculation should not exist!");
	}

	/**
	 * Calculate the resources in the non-local global MBMS areas.
	 */
	if(nlglobal_mbsfn_area_ids && nlglobal_mbsfn_area_ids->num_mbsfn_area_ids){
		if(mce_app_schedule_mbsfn_resources(nlglobal_mbsfn_area_ids, mbms_service_indexes_active_nlg_p
				&csa_patterns_global, 0, NULL) == RETURNerror){
			OAILOG_ERROR(LOG_MCE_APP,"Could not schedule the given (%d) non-local global MBSFN areas.\n", nlglobal_mbsfn_area_ids->num_mbsfn_area_ids);
			OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
		}
		OAILOG_INFO(LOG_MCE_APP,"Successfully scheduled given (%d) non-local global MBSFN areas. Checking the local MBSFN areas.\n", nlglobal_mbsfn_area_ids->num_mbsfn_area_ids);
	}

	/**
	 * Handle the local MBMS areas.
	 * Take the above handled non-local global MBSFN areas into account.
	 * Calculate the MCHs of the given local MBMS services.
	 * Take the first MBSFN area context, to get the phy layer properties, for all global MBMS services..
	 */
	if(local_mbsfn_area_ids && local_mbsfn_area_ids->num_mbsfn_area_ids){
		if(mce_app_schedule_mbsfn_resources(local_mbsfn_area_ids, mbms_service_indexes_active_local_p
				&csa_patterns_local, csa_patterns_global.total_csa_pattern_offset, NULL) == RETURNerror){
			OAILOG_ERROR(LOG_MCE_APP,"Could not schedule the given (%d) local MBSFN areas.\n", local_mbsfn_area_ids->num_mbsfn_area_ids);
			OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
		}
		OAILOG_INFO(LOG_MCE_APP,"Successfully scheduled given (%d) local MBSFN areas. Done with the MBSFN cluster.\n", local_mbsfn_area_ids->num_mbsfn_area_ids);
	}
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

//------------------------------------------------------------------------------
int mce_app_mbms_arp_preempt(mbms_service_indexes_t				* const mbms_service_indexes_to_preemtp, const mbsfn_area_id_t mbsfn_area_id){
	OAILOG_FUNC_IN(LOG_MCE_APP);

	mbms_service_t 	 			*mbms_service_tp							  = NULL;
	mbsfn_area_context_t 	*mbsfn_area_context_tp			   	= NULL;
	int 								   mbms_service_in_list						= 0;
	uint8_t 							 low_arp_prio_level   		 			= 0;
	mbms_service_index_t   final_mbms_service_idx_tp		 	= INVALID_MBMS_SERVICE_INDEX;
	int 									 mbms_service_active_list_index = -1;

	/** Get all MBMS services, which area active in the given MCCH modification period. */
	if(!mbms_service_indexes_to_preemtp->num_mbms_service_indexes){
		OAILOG_ERROR("No active MBMS services received to preempt.\n");
  	OAILOG_FUNC_RETURN(LOG_MCE_APP, INVALID_MBMS_SERVICE_INDEX);
  }
	if(mbsfn_area_id != INVALID_MBSFN_AREA_ID){
		mbsfn_area_context_tp = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbms_service_contexts, mbsfn_area_id);
		DevAssert(mbsfn_area_context_tp);
	}

	/** Remove the MBMS session with the lowest ARP prio level. */
	for(int num_ms = 0; num_ms < mbms_service_indexes_to_preemtp->num_mbms_service_indexes; num_ms++){
		/** Go through all MBMS services and check if they can be considered for preemption. */
		mbms_service_index_t mbms_service_idx_tp = mbms_service_indexes_to_preemtp->mbms_service_index_array[num_ms];
		if(mbms_service_idx_tp && mbms_service_idx_tp != INVALID_MBMS_SERVICE_INDEX) {
			/** Get the service, compare with the MBSFN area ID and check the PVI flag. */
			mbms_service_tp = mce_mbms_service_exists_mbms_service_index(&mce_app_desc.mce_mbms_service_contexts, mbms_service_idx_tp);
			DevAssert(mbms_service_tp);
			if(mbsfn_area_context_tp){
				/** Check if the service is registered in the MBSFN area context. */
				if(HASH_TABLE_OK != hashtable_ts_is_key_exists(mbsfn_area_context_tp->privates.mbms_service_idx_mcch_modification_times_hashmap, (hash_key_t)mbms_service_idx_tp)){
					/** Not considering MBMS service, since not part of the given MBSFN area. */
					OAILOG_WARNING(LOG_MCE_APP, "Not considering MBMS service Index " MBMS_SERVICE_INDEX_FMT " for preemption, since not part of the MBSFN area id " MBSFN_AREA_ID_FMT ".\n",
							mbms_service_idx_tp, mbsfn_area_id);
					continue;
				}
			}
			/** Check if it can be preempted. */
			if(mbms_service_tp->privates.fields.mbms_bc.eps_bearer_context.bearer_level_qos.pvi) {
				if(low_arp_prio_level < mbms_service_tp->privates.fields.mbms_bc.eps_bearer_context.bearer_level_qos.pl) {
					/**
					 * Found a new MBMS Service with preemption vulnerability & lowest ARP priority level.
					 */
					low_arp_prio_level = mbms_service_tp->privates.fields.mbms_bc.eps_bearer_context.bearer_level_qos.pl;
					final_mbms_service_idx_tp = mbms_service_idx_tp;
					mbms_service_in_list = num_ms;
				}
			}
		}
	}

	/** Check if we found an MBMS service to preemp, if so remove it from the list of active MBMS services and signal it back. */
	if(final_mbms_service_idx_tp != INVALID_MBMS_SERVICE_INDEX){
		OAILOG_WARNING(LOG_MCE_APP, "Found final MBMS Service Index "MBMS_SERVICE_INDEX_FMT " with ARP prio (%d) to preempt. Removing it from active list, too. \n",
				final_mbms_service_idx_tp, low_arp_prio_level);
		/** Don't reduce the size of active list. */
		mbms_service_indexes_to_preemtp[mbms_service_in_list] = INVALID_MBMS_SERVICE_INDEX;
	}
	OAILOG_FUNC_RETURN(LOG_MCE_APP, final_mbms_service_idx_tp);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/**
 * Calculate the TBS index based on TS 36.213 Tabke 7.1.7.1-1.
 */
//------------------------------------------------------------------------------
static
int get_itbs(uint8_t mcs){
	if(mcs <= 9)
		return mcs;
	else if (mcs <=16)
		return (mcs-1);
	else if(mcs <=27)
		return (mcs-2);
	else return -1;
}

//------------------------------------------------------------------------------
static
int mce_app_get_mch_mcs(mbsfn_area_context_t * const mbsfn_area_context, const qci_e qci) {
	uint32_t m2_enb_count = mbsfn_area_context->privates.m2_enb_id_hashmap->num_elements;
	DevAssert(m2_enb_count); /**< Must be larger than 1, else there is an error. */
	int mcs = get_qci_mcs(qci, ceil(mbsfn_area_context->privates.fields.mbsfn_area.mch_mcs_enb_factor * m2_enb_count));
	OAILOG_INFO(LOG_MCE_APP, "Calculated new MCS (%d) for MBSFN Area " MBSFN_AREA_ID_FMT" with %d eNBs. \n",
			mcs, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id, m2_enb_count);
	return mcs;
}

/**
 * We update the union of CSA patterns allocated.
 * We iterate over the newly received CSA patterns and update the existing patterns with the same RF Pattern, periodicity and offset.
 * CSA offset is the real identifier.
 * CSA patterns, with existing offsets, should have the same periodicity and the same RF pattern (1/4).
 * So just check the offsets.
 */
//------------------------------------------------------------------------------
static
void mce_app_update_csa_pattern_union(struct csa_patterns_s * resulting_csa_patterns, const struct csa_patterns_s * const new_csa_patterns)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	resulting_csa_patterns->total_csa_pattern_offset |= new_csa_patterns->total_csa_pattern_offset;
	/**
	 * Add the CSA patterns in total.
	 * This will also include the COMMON CSA pattern, which is included in the num_csa_patterns.
	 * It has the flag for the MCCH and any MBSFN data flags in the free subframes of the common CSA pattern.
	 * For the remaining MBSFN areas, the updated common CSA patterns will be checked. So it might be filled directly with the non-local global MBSFN.
	 */
	/** Update the reused CSA patterns with the same offset, with the new subframes. */
	for(int num_csa_pattern = 0; num_csa_pattern < new_csa_patterns->num_csa_pattern; num_csa_pattern++){
		uint8_t csa_pattern_offset_rf = new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_offset_rf;
		/** Check the already existing ones. */
		for(int num_csa_pattern_old = 0; num_csa_pattern_old < resulting_csa_patterns->num_csa_pattern; num_csa_pattern++){
			/** If the offset is the same, assert that the pattern and period are also the same. */
			if(resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_offset_rf == csa_pattern_offset_rf){
				/** Same Pattern. */
				DevAssert(resulting_csa_patterns->csa_pattern[num_csa_pattern_old].mbms_csa_pattern_rfs ==
						new_csa_patterns->csa_pattern[num_csa_pattern].mbms_csa_pattern_rfs);
				/** Same Periodicity. */
				DevAssert(resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_repetition_period_rf ==
						new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_repetition_period_rf);
				/** Update the set subframes --> Subframes allocated by multiple MBSFN areas and are not available anymore for upcoming MBSNF areas. */
				*((uint32_t*)&resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_sf) |=
						*((uint32_t*)&new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_sf);
				OAILOG_INFO(LOG_MCE_APP, "Updated the existing CSA pattern with offset (%d) and repetition period(%d). Resulting new CSA subframes (%x). Not changint the RF offset..\n",
						resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_offset_rf,
						resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_repetition_period_rf,
						*((uint32_t*)&resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_sf));
				break; /**< Continue with the next used one. */
			}
		}
		/**
		 * No matching CSA subframe was found in the already allocated CSA patterns.
		 * Allocate a new one in the resulting CSA patterns.
		 * This would also include the COMMON_CSA pattern.
		 */
		resulting_csa_patterns->csa_pattern[resulting_csa_patterns->num_csa_pattern].csa_pattern_offset_rf = csa_pattern_offset_rf;
		resulting_csa_patterns->csa_pattern[resulting_csa_patterns->num_csa_pattern].csa_pattern_repetition_period_rf =
				new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_repetition_period_rf;
		resulting_csa_patterns->csa_pattern[resulting_csa_patterns->num_csa_pattern].mbms_csa_pattern_rfs =
				new_csa_patterns->csa_pattern[num_csa_pattern].mbms_csa_pattern_rfs;
		memcpy((void*)&resulting_csa_patterns->csa_pattern[resulting_csa_patterns->num_csa_pattern].csa_pattern_sf,
				(void*)&new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_sf, sizeof(uint32_t));
		/** Increase the number of CSA patterns and set the RF offset. */
		resulting_csa_patterns->num_csa_pattern++;
		resulting_csa_patterns->total_csa_pattern_offset |= resulting_csa_patterns->total_csa_pattern_offset;
		OAILOG_INFO(LOG_MCE_APP, "Added new CSA pattern with offset (%d) and repetition period(%d) to existing one. Resulting new CSA subframes (%x). Number of resulting CSA subframes (%d). Total RF offset (%p). \n",
				new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_offset_rf, new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_repetition_period_rf,
				*((uint32_t*)&new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_sf), resulting_csa_patterns->num_csa_pattern, resulting_csa_patterns->total_csa_pattern_offset);
	}
	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

/**
 * Set the subframes in an empty single frame RF pattern, with the given CSA repetition period.
 */
#define NUM_SF_CSA_PATTERN_TOTAL (6 * csa_pattern->mbms_csa_pattern_rfs)
//------------------------------------------------------------------------------
static
void mce_app_set_fresh_radio_frames(struct csa_pattern_s * csa_pattern, struct mchs_s * mchs, const uint8_t csa_sf_available)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	uint8_t sf_full 							= 0;
	uint32_t sfAlloc							= 0;

//	const uint32_t sfAlloc = csa_pattern->mbms_csa_pattern_rfs == CSA_FOUR_FRAME ? csa_pattern->csa_pattern_sf.mbms_mch_csa_pattern_4rf : csa_pattern->csa_pattern_sf.mbms_mch_csa_pattern_1rf;
	/** Check if it is a 4 or 1 Frame pattern. */
	for(int num_rf = 0; num_rf < csa_pattern->mbms_csa_pattern_rfs; num_rf){
		uint8_t sfAlloc_RF_free 			= csa_sf_available;
		uint8_t csa_sf = 0;
		while (sfAlloc_RF_free){
			if(!(sfAlloc_RF_free & 0x20)) {
				csa_sf++;
				sfAlloc_RF_free <<=csa_sf;
				continue;
			}
			sfAlloc |= ((0x20 >> csa_sf) << (num_rf * 6));
			sfAlloc_RF_free &= ~(0x20 >> csa_sf);
			/** Shift it, also when a match occurs. */
			csa_sf++;
			sfAlloc_RF_free <<=csa_sf;
			mchs->total_subframes_per_csa_period_necessary -= (MBMS_CSA_PERIOD_GCS_AS_RF / csa_pattern->csa_pattern_repetition_period_rf);
			/** Reduce it from the total SFs necessary. */
			if(mchs->total_subframes_per_csa_period_necessary <= 0) {
				break;
			}
		}
	}
	*((uint32_t*)&csa_pattern->csa_pattern_sf) = sfAlloc;
	if(mchs->total_subframes_per_csa_period_necessary <= 0) {
		mchs->total_subframes_per_csa_period_necessary = 0;
		OAILOG_INFO(LOG_MCE_APP, "All MCH subframes for MBSFN area " MBSFN_AREA_ID_FMT " fitted into new (%d)RF-CSA pattern with offset (%p) and (%d)RF repetition factor. \n",
				mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id, csa_pattern->mbms_csa_pattern_rfs, csa_pattern->csa_pattern_offset_rf, csa_pattern->csa_pattern_repetition_period_rf);
	} else {
		OAILOG_WARNING(LOG_MCE_APP, "(%d) MCH subframes for MBSFN area " MBSFN_AREA_ID_FMT " remain after filling  (%d)RF-CSA pattern with offset (%p) and (%d)RF repetition factor. \n",
				mchs->total_subframes_per_csa_period_necessary, mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id, csa_pattern->mbms_csa_pattern_rfs, csa_pattern->csa_pattern_offset_rf, csa_pattern->csa_pattern_repetition_period_rf);
	}
	/** No total RF offset needs to be take. */
	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

/**
 * Schedule the subframes given in the CSA pattern into the MCHs, within one CSA period.
 */
//------------------------------------------------------------------------------
static int
mce_app_assign_mch_subframes(const struct csa_patterns_s * const csa_patterns_mbsfn,
		const struct mchs_s 				 * mchs,
		const mbsfn_area_context_t 	 * const mbsfn_area_context,
		const mbsfn_area_cfg_t 			 * const mbsfn_area_cfg,
		const mbms_service_indexes_t * const mbms_service_indexes_active_p)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	/**
	 * Calculate the total number of subframes in the total CSA period.
	 * Subframes only are for the given MBSFN area. The CSA patterns are sequential.
	 */
	uint32_t sfAlloc							= 0;
	uint8_t	 mch_checked 					= 0;
	while(!mchs->mch_array[mch_checked].mch_qci || mch_checked == MAX_MCH_PER_MBSFN)
		mch_checked++;
	if(mch_checked == MAX_MCH_PER_MBSFN){
		DevAssert(0);
	}
	OAILOG_INFO(LOG_MCE_APP, "Checking MCH with QCI (%d) and order (%d) for MBSFN Area " MBSFN_AREA_ID_FMT".\n",
			mchs->mch_array[mch_checked].mch_qci, mch_checked, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id)
	/** Check if it is a 4 or 1 Frame pattern. */
	for(int num_csa_pattern = 0; num_csa_pattern < csa_patterns_mbsfn->num_csa_pattern; num_csa_pattern++){
		struct csa_pattern_s * csa_pattern = csa_patterns_mbsfn->csa_pattern[num_csa_pattern];
		/** Repeat this for all offsets. */
		uint8_t csa_pattern_repetition = MBMS_CSA_PERIOD_GCS_AS_RF/get_csa_period_rf(csa_pattern->csa_pattern_repetition_period_rf);
		/** Iterate through each repeated CSA pattern. */
		for(int num_csa_offset = 0; num_csa_offset < csa_pattern_repetition; num_csa_offset++) {
			/** Iterate through each CSA pattern. */
			for(int num_rf = 0; num_rf < csa_pattern->mbms_csa_pattern_rfs; num_rf++){
				uint8_t sfAlloc_RF_available = ((uint32_t*)(&csa_pattern->csa_pattern_sf)) >> (8 * num_rf);
				uint8_t csa_sf = 0;

				// TODO: CHECK THE MCCH subframe from the configuration!


				while (sfAlloc_RF_available){
					if(!(sfAlloc_RF_available & 0x20)) {
						csa_sf++;
						sfAlloc_RF_available <<=csa_sf;
						continue;
					}
					sfAlloc |= ((0x20 >> csa_sf) << (num_rf * 6));
					sfAlloc_RF_available &= ~(0x20 >> csa_sf);
					/** Shift it, also when a match occurs. */
					csa_sf++;
					sfAlloc_RF_available <<=csa_sf;

					/**
					 * Check if the MCH has a subframe start, if not assign the #SF within the CSA period to it.
					 * The MCH will be scheduled fixed on the same subframe, and the subframe always belongs to the MBSFN.
					 * But the MCCH may be in different MCHs.
					 */
					mchs->mch_array[mch_checked].mch_subframes_per_csa_period--;
					if(!mchs->mch_array[mch_checked].mch_subframe_start){
						mchs->mch_array[mch_checked].mch_subframe_start = (csa_pattern->csa_pattern_repetition_period_rf * csa_pattern_repetition)
								+ (csa_pattern->csa_pattern_offset_rf * 10) + get_absolute_subframe(mbsfn_area_context, csa_sf);
					}
					if(!mchs->mch_array[mch_checked].mch_subframes_per_csa_period) {
						DevAssert(!mchs->mch_array[mch_checked].mch_subframe_stop);
						mchs->mch_array[mch_checked].mch_subframe_stop = (csa_pattern->csa_pattern_repetition_period_rf * csa_pattern_repetition)
								+ (csa_pattern->csa_pattern_offset_rf * 10) + get_absolute_subframe(mbsfn_area_context, csa_sf);
						/** Go to the next MCH. */
						OAILOG_INFO(LOG_MCE_APP, "Scheduled all subframes of the MCH (%d) for MBSFN area " MBSFN_AREA_ID_FMT". Checking remaining MCHs.",
								mch_checked, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
						mch_checked++;
						while(!mchs->mch_array[mch_checked].mch_qci && mch_checked < MAX_MCH_PER_MBSFN)
							mch_checked++;
						if(mch_checked == MAX_MCH_PER_MBSFN){
							OAILOG_INFO(LOG_MCE_APP, "No more MCHs to schedule for MBSFN Area " MBSFN_AREA_ID_FMT ". \n",
									mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
							/** Directly return, we handled all MCHs and done with the given CSA pattern. */
							OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
						}
						OAILOG_INFO(LOG_MCE_APP, "Checking MCH with QCI (%d) and order (%d) for MBSFN Area " MBSFN_AREA_ID_FMT" with the remaining CSA patterns.\n",
								mchs->mch_array[mch_checked].mch_qci, mch_checked, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id)
					}
				}
			}
		}
	}
	/** Set them to the MCHs. */
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

//------------------------------------------------------------------------------
static
int mce_app_log_method_single_rf_csa_pattern(struct csa_patterns_s * new_csa_patterns,
		int *num_radio_frames_p,
		struct mchs_s * mchs, const struct mbsfn_area_context_s * const mbsfn_area_context,
		uint8_t total_offset_allocated)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	int power2 											 							= 0;
	int radio_frames_alloced_per_csa_pattern 			= 0;
	int num_csa_patterns_set											= 0; /**< Calculate the CSA pattern offset from other MBSFN areas and the current MBSFN area. */

	/**
	 * Check if a 4RF pattern has been allocated (max 1 possible).
	 */
	if(total_offset_allocated == 0xFF){
		OAILOG_ERROR(LOG_MCE_APP,"No more single RF CSA pattern can be allocated. All offsets are full. \n");
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
	}

	/** Check the other MBSFN areas. Increase the radio frame offset. */
	for (; total_offset_allocated; num_csa_patterns_set++)
	{
		total_offset_allocated &= (total_offset_allocated-1);
	}

	/**
	 * Check each power of 2. Calculate a CSA pattern for each with a different offset and a period (start with the most frequest period).
	 * We may not use the last CSA pattern.
	 */
	while(*num_radio_frames_p){
		/**
		 * Determines the periodicity of CSA patterns..
		 * We start with the highest possible periodicity.
		 * For each single frame, we increase the used CSA pattern offset by once, no matter what the periodicity is.
		 */
		DevAssert((power2 = floor(log2(*num_radio_frames_p))));
		radio_frames_alloced_per_csa_pattern = pow(2, power2);
//		/**
//		 * Next we will calculate a single pattern for each modulus. We then will increase the new_csa_patterns total_csa_offset bitmap,
//		 * make union, with the already allocated one and check if free offsets are left.
//		 */
		if(new_csa_patterns->total_csa_pattern_offset | total_offset_allocated == 0xFF){
			OAILOG_ERROR(LOG_MCE_APP, "No more CSA patterns left to allocate a single RF CSA pattern for MBSFN Area "MBSFN_AREA_ID_FMT".\n",
					mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
			OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
		}
		/**
		 * Calculate the number of radio frames, that can scheduled in a single RF CSA pattern in this periodicity.
		 * Consider the CSA pattern with the given CSA offset.
		 */
	  new_csa_patterns->csa_pattern[new_csa_patterns->num_csa_pattern].csa_pattern_offset_rf							= (0x01 << (num_csa_patterns_set- 1));
	  new_csa_patterns->csa_pattern[new_csa_patterns->num_csa_pattern].mbms_csa_pattern_rfs 							= CSA_ONE_FRAME;
	  new_csa_patterns->csa_pattern[new_csa_patterns->num_csa_pattern].csa_pattern_repetition_period_rf		= get_csa_rf_alloc_period_rf(CSA_RF_ALLOC_PERIOD_RF32) / (radio_frames_alloced_per_csa_pattern / 4);
	  new_csa_patterns->total_csa_pattern_offset |= (0x01 << (num_csa_patterns_set- 1));
	  mce_app_set_fresh_radio_frames(&new_csa_patterns->csa_pattern[new_csa_patterns->num_csa_pattern], mchs,
	  		get_enb_mbsfn_subframes(get_enb_type(mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_band), mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_tdd_dl_ul_perc));
	  /** Increase the CSA pattern offset. Check if the last radio frame (CSA_COMMON) is reached. */
	  num_csa_patterns_set++;
	  new_csa_patterns->num_csa_pattern++;
	  /**
	   * We set the allocated subframes 1RF CSA pattern and reduced the number of remaining subframes left for scheduling.
	   * We allocated some MBSFN radio frames starting with the highest priority. Reduce the number of remaining MBSFN radio frames.
	   */
	  *num_radio_frames_p -= radio_frames_alloced_per_csa_pattern;
	  if(mchs->total_subframes_per_csa_period_necessary == 0){
	  	DevAssert(*num_radio_frames_p);
	  	break;
	  }
	}
	DevAssert(mchs->total_subframes_per_csa_period_necessary == 0);
	/** Successfully scheduled all radio frames! */
	OAILOG_INFO(LOG_MCE_APP, "Successfully scheduled all radio subframes for the MCHs of the MBSFN Area Id "MBSFN_AREA_ID_FMT " into the CSA patterns. "
			"Total CSA pattern offset of new CSA is (%d). \n", new_csa_patterns->total_csa_pattern_offset);
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

//------------------------------------------------------------------------------
static
void mce_app_calculate_mbsfn_mchs(const struct mbsfn_area_context_s * const mbsfn_area_context,
		const mbms_service_indexes_t * const mbms_service_indexes_active,
		mchs_t *const mchs) {
	OAILOG_FUNC_IN(LOG_MCE_APP);

	int 									 total_duration_in_ms 						= mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_csa_period_rf * 10;
	bitrate_t 						 pmch_total_available_br_per_sf 	= 0;
	mbms_service_t			  *mbms_service										  = NULL;

	/**
	 * No hash callback, just iterate over the active MBMS services.
	 */
	for(int num_mbms_service_index = 0; num_mbms_service_index < mbms_service_indexes_active->num_mbms_service_indexes; num_mbms_service_index++) {
		/** Get the MBMS service, check if it belongs to the given MBSFN area. */
		if(HASH_TABLE_OK != hashtable_ts_is_key_exists(mbsfn_area_context->privates.mbms_service_idx_mcch_modification_times_hashmap,
				(hash_key_t)mbms_service_indexes_active->mbms_service_index_array[num_mbms_service_index]))
			continue;

		/** Active MBMS service is contained in the MBSFN MBMS service hashmap. Calculate the MCHs. */
		mbms_service = mce_mbms_service_exists_mbms_service_index(&mce_app_desc.mce_mbms_service_contexts, mbms_service_indexes_active->mbms_service_index_array[num_mbms_service_index]);
		DevAssert(mbms_service);
		/** Calculate the resources based on the active eNBs in the MBSFN area. */
		qci_e qci = mbms_service->privates.fields.mbms_bc.eps_bearer_context.bearer_level_qos.qci;
		// todo: Current all 15 QCIs fit!! todo --> later it might not!
		mch_t mch = mchs->mch_array[get_qci_ord(qci)];
		if(!mch.mch_qci){
			DevAssert(!mch.total_gbr_dl_bps);
			mch.mch_qci = qci;
		}
		/** Calculate per MCH the total bandwidth (bits per seconds // multiplied by 1000 @sm decoding). */
		mch.total_gbr_dl_bps += mbms_service->privates.fields.mbms_bc.eps_bearer_context.bearer_level_qos.gbr.br_dl;
		/** Add the TMGI. */
		memcpy((void*)&mch.mbms_session_list.tmgis[mch.mbms_session_list.num_mbms_sessions++], (void*)&mbms_service->privates.fields.tmgi, sizeof(tmgi_t));
		OAILOG_INFO(LOG_MCE_APP, "Added MBMS service index " MBMS_SERVICE_INDEX_FMT " with TMGI " TMGI_FMT " into MCHs for MBSFN area " MBSFN_AREA_ID_FMT ".\n",
				mbms_service_indexes_active->mbms_service_index_array[num_mbms_service_index],
				TMGI_ARG(&mbms_service->privates.fields.tmgi), mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
	}
	/** Resulting MCHs of the MBSFN area context. */
	OAILOG_INFO(LOG_MCE_APP, "(%d) MCHs for MBSFN area " MBSFN_AREA_ID_FMT " resulted. Calculating total subframes required. \n", mchs->num_mch, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);

	/**
	 * The CSA period is set as 1 second (RF128). The minimum time of a service duration is set to the MCCH modification period!
	 * MSP will be set to the length of the CSA period for now. Should be enough!
	 * Calculate the actual MCS of the MCH and how many bit you can transmit with an SF.
	 */
	for(int num_mch = 0; num_mch < MAX_MCH_PER_MBSFN; num_mch++){
		mch_t mch = mchs->mch_array[num_mch];
		if(mch.mch_qci) {
			/**
			 * Set MCH.
			 * Calculate per MCH, the necessary subframes needed in the CSA period.
			 * Calculate the MCS of the MCH.
			 */
			mch.mcs = mce_app_get_mch_mcs(mbsfn_area_context, mch.mch_qci);
			if(mch.mcs == -1){
				DevMessage("Error while calculating MCS for MBSFN Area " + mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id + " and QCI " + qci);
			}
			/** Calculate the necessary transport blocks. */
			int itbs = get_itbs(mch.mcs);
			if(itbs == -1){
				DevMessage("Error while calculating TBS index for MBSFN Area " + mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id + " for MCS " + mch.mcs);
			}
			mch.msp_rf = mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_csa_period_rf;
			/**
			 * We assume a single antenna port and just one Transport Block per subframe.
			 * No MIMO is expected.
			 * Number of bits transmitted per 1ms (1028)ms.
			 * Subframes, allocated for this MCH gives us the following capacity.
			 * No MCH subframe-interleaving is foreseen. So each MCH will have own subframes. Calculate the capacity of the subframes.
			 * The duration is the CSA period, we calculate the MCHs on.
			 * ITBS starts from zero, so use actual values.
			 */
			bitrate_t available_br_per_subframe = TBStable[itbs][mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_bw -1];
			bitrate_t mch_total_br_per_ms = mch.total_gbr_dl_bps /1000; /**< 1000 */
			bitrate_t total_bitrate_in_csa_period = mch_total_br_per_ms * total_duration_in_ms; /**< 1028*/
			/** Check how many subframes we need. */
			mch.mch_subframes_per_csa_period = ceil(total_bitrate_in_csa_period / available_br_per_subframe);
			/** Check if half or full slot. */
			if(mbsfn_area_context->privates.fields.mbsfn_area.mbms_sf_slots_half){
				/** Multiply by two, since only half a slot is used. */
				mch.mch_subframes_per_csa_period *=2;
			}
			/** Don't count the MCCH. */
			mchs->total_subframes_per_csa_period_necessary += mch.mch_subframes_per_csa_period;
		}
	}
	/** Resulting MCHs of the MBSFN area context. */
	OAILOG_INFO(LOG_MCE_APP, "(%d) MCHs for MBSFN area " MBSFN_AREA_ID_FMT " resulted in (%d) total subframes required. \n",
			mchs->num_mch, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id, mchs->total_subframes_per_csa_period_necessary);
	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

//------------------------------------------------------------------------------
static
void mce_app_reuse_csa_pattern_set_subframes(struct csa_pattern_s * csa_pattern_mbsfn, struct csa_pattern_s * csa_pattern, struct mchs_s * const mchs, int *mch_subframes_to_be_scheduled_p,
		const struct mbsfn_area_context_s * const mbsfn_area_ctx){
	OAILOG_FUNC_IN(LOG_MCE_APP);

	/** Check any subframes are left: Count the bits in each octet. */
	uint8_t sf_full = 0;
	uint8_t sfAlloc_RF_free = 0;
	uint8_t num_available_subframes_first_csa_pattern = 0;

	/** The CSA pattern of subframes which can be allocated and the total number of subframes available per single RF CSA pattern. */
	uint8_t csa_pattern_sf_size    = get_enb_subframe_size(get_enb_type(mbsfn_area_ctx->privates.fields.mbsfn_area.m2_enb_band), mbsfn_area_ctx->privates.fields.mbsfn_area.m2_enb_tdd_dl_ul_perc);
	uint8_t m2_enb_mbsfn_subframes = get_enb_mbsfn_subframes(get_enb_type(mbsfn_area_ctx->privates.fields.mbsfn_area.m2_enb_band), mbsfn_area_ctx->privates.fields.mbsfn_area.m2_enb_tdd_dl_ul_perc);

	const uint32_t sfAlloc = csa_pattern->mbms_csa_pattern_rfs == CSA_FOUR_FRAME ? csa_pattern->csa_pattern_sf.mbms_mch_csa_pattern_4rf : csa_pattern->csa_pattern_sf.mbms_mch_csa_pattern_1rf;
	while(sf_full < NUM_SF_CSA_PATTERN_TOTAL){
		sfAlloc_RF_free = ((sfAlloc >> sf_full) & 0x3F) ^ m2_enb_mbsfn_subframes; /**< Last one should be 0. */
		if(sfAlloc_RF_free){
			/**
			 * Pattern not filled fully.
			 * Check how many subframes are set.
			 */
			/** Count the number of set MBSFN subframes, no matter if 1 or 4 RFs. */
			for (; sfAlloc_RF_free; num_available_subframes_first_csa_pattern++)
			{
				sfAlloc_RF_free &= (sfAlloc_RF_free-1);
			}
			/** Assert that the remaining subframes are zero! We wan't to set them in order without gaps!. */
			DevAssert(!(sfAlloc >> num_sf_checked)); /**< Does not need to be in bounds. Remaining CSA patterns should be unallocated. */
			break;
		}
		sf_full +=6; /**< Move 6 subframes. */
	}
	if(sf_full == NUM_SF_CSA_PATTERN_TOTAL){
		OAILOG_DEBUG(LOG_MCE_APP, "(%d)RF-CSA pattern has no free subframes left. Checking the other CSA patterns.\n",
				csa_pattern->mbms_csa_pattern_rfs);
		OAILOG_FUNC_OUT(LOG_MCE_APP);
	}
	DevAssert(csa_pattern_sf_size-num_available_subframes_first_csa_pattern);

	/**
	 * Copy the offset, repetition period and type.
	 * */
	csa_pattern_mbsfn->csa_pattern_offset_rf = csa_pattern->csa_pattern_offset_rf;
	csa_pattern_mbsfn->csa_pattern_repetition_period_rf = csa_pattern->csa_pattern_repetition_period_rf;
	csa_pattern_mbsfn->mbms_csa_pattern_rfs = csa_pattern->mbms_csa_pattern_rfs;

	/** Allocate the subframes from the first free CSA pattern. */
	uint8_t csa_sf = 0;
	uint32_t alloced_csa_pattern = 0;
	while (sfAlloc_RF_free){
		if(!(sfAlloc_RF_free & 0x20)) {
			csa_sf++;
			sfAlloc_RF_free <<=csa_sf;
			continue;
		}
//		csa_pattern_mbsfn->csa_pattern_sf.mbms_mch_csa_pattern_1rf |= (0x20 >> csa_sf);
		alloced_csa_pattern |= (0x20 >> csa_sf);
		mchs->total_subframes_per_csa_period_necessary -= (MBMS_CSA_PERIOD_GCS_AS_RF / csa_pattern->csa_pattern_repetition_period_rf);
		if(mchs->total_subframes_per_csa_period_necessary <= 0) {
			break;
		}
	}
	*((uint32_t*)&csa_pattern_mbsfn->csa_pattern_sf) = (alloced_csa_pattern << sf_full);
	if(mchs->total_subframes_per_csa_period_necessary <= 0) {
		mchs->total_subframes_per_csa_period_necessary = 0;
		OAILOG_WARNING(LOG_MCE_APP, "All MCH subframes for MBSFN area " MBSFN_AREA_ID_FMT " fitted into the reused CSA pattern. \n", mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
		/** No total RF offset needs to be take. */
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
	}
	if(!sfAlloc_RF_free)
		sf_full+=6;
	/** Check, if it is a 4RF pattern, for remaining subframes, which can be occupied. */
	if(sf_full == NUM_SF_CSA_PATTERN_TOTAL){
		OAILOG_WARNING(LOG_MCE_APP, "No more free subframes left MCH subframes for MBSFN area " MBSFN_AREA_ID_FMT " in (%d)-RF CSA pattern. \n",
				mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id, csa_pattern->mbms_csa_pattern_rfs);
		/** No total RF offset needs to be take. */
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
	}

	/** Check the remaining CSA patterns in the 4RF pattern. */
	while(sf_full < NUM_SF_CSA_PATTERN_TOTAL){
		csa_sf = 0;
		sfAlloc_RF_free = ((sfAlloc >> sf_full) & 0x3F) ^ m2_enb_mbsfn_subframes; /**< Last one should be 0. */
		uint8_t csa_pattern_free = 0;
		while (sfAlloc_RF_free){
			if(!(sfAlloc_RF_free & 0x20)) {
				csa_sf++;
				sfAlloc_RF_free <<=csa_sf;
				continue;
			}
			csa_pattern_free |= (0x20 >> csa_sf);
			csa_sf++;
			sfAlloc_RF_free <<=csa_sf;
			mchs->total_subframes_per_csa_period_necessary -= (MBMS_CSA_PERIOD_GCS_AS_RF / csa_pattern->csa_pattern_repetition_period_rf);
			/** Reduce it from the total SFs necessary. */
			if(mchs->total_subframes_per_csa_period_necessary <= 0) {
				break;
			}
		}
		if(mchs->total_subframes_per_csa_period_necessary <= 0) {
			mchs->total_subframes_per_csa_period_necessary = 0;
			OAILOG_WARNING(LOG_MCE_APP, "All MCH subframes for MBSFN area " MBSFN_AREA_ID_FMT " fitted into reused CSA pattern. \n", mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
			csa_pattern_mbsfn->csa_pattern_sf.mbms_mch_csa_pattern_4rf |= (csa_pattern_free << sf_full);
			/** No total RF offset needs to be take. */
			OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
		}
	}
	DevAssert(mchs->total_subframes_per_csa_period_necessary);
	OAILOG_WARNING(LOG_MCE_APP, "No more free subframes left MCH remaining (%d) subframes for MBSFN area ID " MBSFN_AREA_ID_FMT " in 4RF CSA pattern. \n",
			mchs->total_subframes_per_csa_period_necessary, mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
	/** No total RF offset needs to be take. */
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

/**
 * We cannot enter this method one by one for each MBSFN area.
 * @param: csa_patterns_alloced: should be a union of the CSA patterns of all previously scheduled MBSFN areas.
 */
//------------------------------------------------------------------------------
static
void mce_app_reuse_csa_pattern(struct csa_patterns_s * csa_patterns_mbsfn_p, mchs_t * mchs, const struct csa_patterns_s * const csa_patterns_alloced, const struct mbsfn_area_context_s * const mbsfn_area_ctx){
	OAILOG_FUNC_IN(LOG_MCE_APP);
	/**
	 * Iterate the CSA patterns, till the COMMON_CSA pattern.
	 * Check for any available 4RF and 1RF CSA patterns.
	 * Start with the lowest repetition factor (4). Move up to 16.
	 */
	for(csa_frame_num_e csa_frame_num = CSA_FOUR_FRAME; csa_frame_num < 1; csa_frame_num/=4) {
		for(csa_rf_alloc_period_e num_csa_repetition = CSA_RF_ALLOC_PERIOD_RF32; num_csa_repetition >= CSA_PERIOD_RF8; num_csa_repetition--){ /**< We use 32, 16, 8. */
			/** The index is always absolute and not necessarily equal to the CSA offset. */
			for(int num_csa_pattern = 0; num_csa_pattern < COMMON_CSA_PATTERN; num_csa_pattern++){
				/** Check if 4RF. */
				int csa_pattern_repetition_rf = get_csa_rf_alloc_period_rf(num_csa_repetition);
				struct csa_pattern_s * csa_pattern = &csa_patterns_alloced->csa_pattern[num_csa_pattern];
				if(csa_pattern->mbms_csa_pattern_rfs == csa_frame_num && csa_pattern->csa_pattern_repetition_period_rf == csa_pattern_repetition_rf){
					struct csa_pattern_s * csa_pattern_mbsfn = &csa_patterns_mbsfn_p->csa_pattern[num_csa_pattern];
					mce_app_reuse_csa_pattern_set_subframes(csa_pattern_mbsfn, csa_pattern, mchs, mbsfn_area_ctx);
					/** No return expected, just check if all subframes where scheduled. */
					if(mchs->total_subframes_per_csa_period_necessary <= 0){
						mchs->total_subframes_per_csa_period_necessary = 0;
						/**
						 * No more MCH subframes to be scheduled.
						 * No further CSA offsets need to be defined, we can re-use the existing. */
						OAILOG_INFO(LOG_MCE_APP, "Fitted (%d) newly received MCHs into existing CSA (%d)RF-pattern with RF offset (%d), and repetition period RF(%d). \n",
								mchs->num_mch, csa_pattern->mbms_csa_pattern_rfs, csa_pattern->csa_pattern_offset_rf,  csa_pattern->csa_pattern_repetition_period_rf);
						OAILOG_FUNC_OUT(LOG_MCE_APP);
					}
					/** Continue checking remaining allocated CSA patterns. */
				}
			}
		}
	}
	OAILOG_INFO(LOG_MCE_APP, "After all (%d) reusable CSA patterns checked, still (%d) subframes exist for (%d) MCHs. \n",
			(csa_patterns_alloced->num_csa_pattern -1), mchs->total_subframes_per_csa_period_necessary, mchs->num_mch);
	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

//------------------------------------------------------------------------------
static void mce_app_allocate_4frame(struct csa_patterns_s * new_csa_patterns, int * num_radio_frames_p, struct mchs_s * mchs, const struct mbsfn_area_context_s * const mbsfn_area_context, struct csa_patterns_s * csa_patterns_allocated){
	OAILOG_FUNC_IN(LOG_MCE_APP);
	// todo: 0.80 is variable, just a threshold where we consider allocating a 4RF pattern. What could be any other reason?
	/** 16 Radio Frames are the minimum radio frames allocated with a /32 periodicity. */
	mme_config_read_lock(&mme_config);
	int csa_4_frame_rfs_repetition = floor((double)(*num_radio_frames_p * mme_config.mbms.mbsfn_csa_4_rf_threshold) / ((MBMS_CSA_PERIOD_GCS_AS_RF/get_csa_period_rf(CSA_PERIOD_RF32)) * CSA_FOUR_FRAME));
	mme_config_unlock(&mme_config);
	if(!csa_4_frame_rfs_repetition) {
		OAILOG_INFO(LOG_MCE_APP, "Skipping 4RF pattern for MBSFN Area Id " MBSFN_AREA_ID_FMT". Not enough RFs (%d). \n.", mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id, num_radio_frames);
		OAILOG_FUNC_OUT(LOG_MCE_APP);
	}
	/** 4RF pattern will be allocated. */
	uint8_t total_csa_pattern_offset = 0xF0; /**< 8 Radio Frames. */
	while (total_csa_pattern_offset & csa_patterns_allocated->total_csa_pattern_offset){
		/** Overlap between the already allocated and the newly allocated CSA pattern. */
		total_csa_pattern_offset >>=  0x01;
		if(total_csa_pattern_offset == 0x0F) {
			OAILOG_ERROR(LOG_MCE_APP, "No more free radio frame offsets available to schedule the MCHs of MBSFN Area Id " MBSFN_AREA_ID_FMT " in a 4RF pattern. Collision with CSA_COMMON. \n.",
					mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
			OAILOG_FUNC_OUT(LOG_MCE_APP);
		}
	}
	/**
	 * No matter what the 4RF repetition is, it will be allocated in the first common 8 radio frames. So checking it is enough.
	 * Allocate a 4RF radio frame, and then remove it from the necessary subframes to be scheduled.
	 */
	OAILOG_INFO(LOG_MCE_APP, "Allocating 4RF CSA Pattern with allocated RFs (offset) (%p) and repetition (%d).\n", total_csa_pattern_offset, csa_4_frame_rfs_repetition);
	/**
	 * No looping is necessary. We know the #RFs to be allocated by 4 * csa_pattern_repetition_period.
	 * The CSA pattern will always be the first CSA pattern of the MBSFN area.
	 */
	new_csa_patterns->total_csa_pattern_offset												= total_csa_pattern_offset;
	new_csa_patterns->csa_pattern[new_csa_patterns->num_csa_pattern-1].csa_pattern_offset_rf						= total_csa_pattern_offset;
	new_csa_patterns->csa_pattern[new_csa_patterns->num_csa_pattern-1].mbms_csa_pattern_rfs 						= CSA_FOUR_FRAME;
	new_csa_patterns->csa_pattern[new_csa_patterns->num_csa_pattern-1].csa_pattern_repetition_period_rf	= get_csa_period_rf(CSA_PERIOD_RF32)/csa_4_frame_rfs_repetition;
	mce_app_set_fresh_radio_frames(&new_csa_patterns->csa_pattern[new_csa_patterns->num_csa_pattern-1], mchs,
			get_enb_mbsfn_subframes(get_enb_type(mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_band), mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_tdd_dl_ul_perc));
	new_csa_patterns->num_csa_pattern++;

	*num_radio_frames_p  = *num_radio_frames_p - ((MBMS_CSA_PERIOD_GCS_AS_RF/get_csa_period_rf(CSA_PERIOD_RF32)) * csa_4_frame_rfs_repetition * 4);

	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

//------------------------------------------------------------------------------
static
int mce_app_alloc_csa_pattern(struct csa_patterns_s * new_csa_patterns,
		struct mchs_s * mchs, const uint8_t total_csa_pattern_offset,
		const struct mbsfn_area_context_s * mbsfn_area_ctx)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	/** Check if any the union has allocated all CSA pattern offsets. If so, reject. */
	if(total_csa_pattern_offset == 0xFF){
		OAILOG_ERROR(LOG_MCE_APP, "All CSA pattern offsets area allocated already. Cannot allocate a new CSA pattern for MBSFN Area " MBSFN_AREA_ID_FMT". \n",
				mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
	}
	/** Check with full allocation how many subframes are needed. */
	int csa_pattern_subframe_size = get_enb_subframe_size(get_enb_type(mbsfn_area_ctx->privates.fields.mbsfn_area.m2_enb_band), mbsfn_area_ctx->privates.fields.mbsfn_area.enb_tdd_dl_ul_perc);
	DevAssert(csa_pattern_subframe_size);

	/**< Received number of fresh radio frames, for which a new pattern fill be fully filled. Make it the next multiple of 4. */
	int num_radio_frames = ceil(mchs->total_subframes_per_csa_period_necessary/csa_pattern_subframe_size);
	num_radio_frames += (num_radio_frames %4);
	/**
	 * Allocate a 4RF CSA pattern with the given period.
	 * For the remaining RFs calculate a single frame CSA pattern.
	 */
	mce_app_allocate_4frame(new_csa_patterns, &num_radio_frames, mchs, mbsfn_area_ctx, total_csa_pattern_offset);
	if(mchs->total_subframes_per_csa_period_necessary <=0){
		mchs->total_subframes_per_csa_period_necessary = 0;
		OAILOG_INFO(LOG_MCE_APP, "MCHs of MBSFN Area Id "MBSFN_AREA_ID_FMT " are allocated in a 4RF pattern completely.\n", mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
	}
	/** Check if there are any offsets left for the 1RF CSA pattern. */
	OAILOG_INFO(LOG_MCE_APP, "Checking for a new 1RF pattern for MCHs of MBSFN Area Id "MBSFN_AREA_ID_FMT ". (%d) MBSFN subframes left to allocate. \n",
			mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id, mchs->total_subframes_per_csa_period_necessary);

	/**
	 * Check the number of 1RF CSA patterns you need (periodic).
	 * New CSA pattern should be already allocated, inside, compare it with the csa_patterns_allocated.
	 */
	if(mce_app_log_method_single_rf_csa_pattern(new_csa_patterns, &num_radio_frames, mchs, mbsfn_area_ctx, total_csa_pattern_offset) == RETURNerror){
		OAILOG_ERROR(LOG_MCE_APP, "Error while allocating new CSA CSA patterns for MBSFN Area Id " MBSFN_AREA_ID_FMT". (%d) subframes remain in MCH. \n",
				mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id, mchs->total_subframes_per_csa_period_necessary);
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
	}
	DevAssert(mchs->total_subframes_per_csa_period_necessary == 0);
	OAILOG_INFO(LOG_MCE_APP, "Completed the allocation of 1RF pattern for MBSFN Area Id " MBSFN_AREA_ID_FMT ".\n", mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

/**
 * Check the CSA pattern for this MBSFN!
 * May reuse the CSA patterns of the already allocated MBSFNs (const - unchangeable).
 * We use the total number of MBSFN area to update first CSA[7] where we also leave the last repetition for MCCH subframes.
 * If the MCCH Modification period is 2*CSA_PERIOD (2s), the subframes in the last repetition will not be filled with data,
 * because it would overwrite in the CSA period where the MCCHs occur.
 *
 * After scheduling all subframes for the MCHs and marking the subframes, offsets and repetition patterns, we calculate the allocated subframes end per MCH later.
 */
//------------------------------------------------------------------------------
static
int mce_app_calculate_mbsfn_csa_patterns(struct csa_patterns_s * const csa_patterns_mbsfn_p,
	const struct csa_patterns_s * const csa_patterns_included, const uint8_t excluded_csa_pattern_offset,
	const struct csa_pattern_s * const csa_pattern_common, mchs_t * const mchs, const struct mbsfn_area_context_s * const mbsfn_area_ctx) {
	OAILOG_FUNC_IN(LOG_MCE_APP);

	uint8_t m2_enb_mbsfn_subframes = get_enb_mbsfn_subframes(get_enb_type(mbsfn_area_ctx->privates.fields.mbsfn_area.m2_enb_band), mbsfn_area_ctx->privates.fields.mbsfn_area.enb_tdd_dl_ul_perc);
	/**
	 * We construct CSA patterns for the given MBSFN area context.
	 * No matter if local/global, first check empty subframes in the common CSA pattern. Fill them to the MBSFN area.
	 * XOR should be enough, since we just may have some unset 1s.
	 *
	 * The repetitions of the assigned assigned MCCH bits for the MBSFN area area already set. Remove them from the MCHs total needed subframes.
	 * And then check for remaining empty bits in the CSA pattern.
	 */
	mchs->total_subframes_per_csa_period_necessary -= ((MBMS_CSA_PERIOD_GCS_AS_RF / csa_pattern_common->csa_pattern_repetition_period_rf) -1);
	/** Set the COMMON CSA properties. */
	csa_patterns_mbsfn_p->csa_pattern[COMMON_CSA_PATTERN].mbms_csa_pattern_rfs 	= CSA_ONE_FRAME;
	csa_patterns_mbsfn_p->csa_pattern[COMMON_CSA_PATTERN].csa_pattern_offset_rf = COMMON_CSA_PATTERN;
	/** Allocate the COMMON CSA in each subframe. */
	csa_patterns_mbsfn_p->csa_pattern[COMMON_CSA_PATTERN].csa_pattern_repetition_period_rf = get_csa_rf_alloc_period_rf(CSA_RF_ALLOC_PERIOD_RF8);
	csa_patterns_mbsfn_p->num_csa_pattern++;
	csa_patterns_mbsfn_p->total_csa_pattern_offset |= 0x80; /**< Set the last CSA pattern as allocated. */

	if(mchs->total_subframes_per_csa_period_necessary <= 0){
		mchs->total_subframes_per_csa_period_necessary = 0;
		OAILOG_INFO(LOG_MCE_APP, "All MCH subframes fitted into the assigned MCCH subframe repetitions in the COMMON_CSA pattern for MBSFN area " MBSFN_AREA_ID_FMT ".\n",
				mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
	}
	uint8_t common_csa_free = csa_patterns_included->csa_pattern[COMMON_CSA_PATTERN].csa_pattern_sf.mbms_mch_csa_pattern_1rf ^ m2_enb_mbsfn_subframes;
	if(common_csa_free){
		OAILOG_INFO(LOG_MCE_APP, "CSA_COMMON subframes have not yet been fully allocated. Allocate them for MBSFN area " MBSFN_AREA_ID_FMT ".\n",
				mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
		/** Allocate the subframes from the left and reduce them from the MCH. */
		uint8_t csa_sf = 0;
		while (common_csa_free){
			if(!(common_csa_free & 0x20)) {
				csa_sf++;
				common_csa_free <<=csa_sf;
				continue;
			}
			csa_patterns_mbsfn_p->csa_pattern[COMMON_CSA_PATTERN].csa_pattern_sf.mbms_mch_csa_pattern_1rf |= (0x20 >> csa_sf);
			mchs->total_subframes_per_csa_period_necessary -= (MBMS_CSA_PERIOD_GCS_AS_RF / csa_patterns_mbsfn_p->csa_pattern[COMMON_CSA_PATTERN].csa_pattern_repetition_period_rf);
			if(mchs->total_subframes_per_csa_period_necessary <= 0){
				mchs->total_subframes_per_csa_period_necessary = 0;
				OAILOG_WARNING(LOG_MCE_APP, "All MCH subframes for MBSFN area " MBSFN_AREA_ID_FMT " fitted into common-csa pattern. \n", mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
				/** No total RF offset needs to be take. */
				OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
			}
		}
		OAILOG_INFO(LOG_MCE_APP, "All COMMON-CSA pattern subframes filled for MBSFN area " MBSFN_AREA_ID_FMT ". \n", mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
	} else {
		OAILOG_INFO(LOG_MCE_APP, "No free COMMON-CSA pattern subframes available for MBSFN area " MBSFN_AREA_ID_FMT ". \n", mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
	}

	/**
	 * Start checking by the already allocated CSA subframes in the included CSA patterns.
	 * We don't need consecutive allocates of subframes/radio frames between MBSFN areas.
	 * No return value is needed, since we will try to allocate new resources, if MBSFN SFs remain.
	 * The new csa_patterns will be derived from the already allocated csa_patterns in the mbsfn_areas object.
	 * At reuse, we first check 4RFs and shortest period.
	 */
	if(csa_patterns_included->num_csa_pattern > 1){
		OAILOG_INFO(LOG_MCE_APP, "Checking previous allocated CSA patterns(except CSA_Common) in the included area for MBSFN area id " MBSFN_AREA_ID_FMT".\n",
				mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
		mce_app_reuse_csa_pattern(csa_patterns_mbsfn_p, mchs, csa_patterns_included, mbsfn_area_ctx);
		if(mchs->total_subframes_per_csa_period_necessary <= 0){
			mchs->total_subframes_per_csa_period_necessary = 0;
			/**
			 * Total CSA pattern offset is not incremented. We check it always against COMMON_CSA_PATTERN (reserved).
			 */
			OAILOG_INFO(LOG_MCE_APP, "Fitted all data into previously allocated CSA patterns for MBSFN Area Id " MBSFN_AREA_ID_FMT ". No need to calculate new CSA patterns.\n",
					mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
			OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
		}
	}

	OAILOG_INFO(LOG_MCE_APP, "After checking reused CSA patterns, trying to allocate new CSA patterns for the remaining (%d) MCH subframes of MBSFN Area Id " MBSFN_AREA_ID_FMT " .\n",
			mchs->total_subframes_per_csa_period_necessary, mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);

	/**
	 * Check if a new pattern needs to be allocated.
	 * Check for the repetition period and CSA pattern form that is necessary.
	 * Check if the offset can be fitted?
	 * Allocate the subframes according to FDD/TDD.
	 * Start from 1 RF and longest period (32/16/8) -> then move to 4RF.
	 */
	if(mce_app_alloc_csa_pattern(csa_patterns_mbsfn_p, mchs, (csa_patterns_included->total_csa_pattern_offset | excluded_csa_pattern_offset), mbsfn_area_ctx) == RETURNerror) {
		OAILOG_ERROR(LOG_MCE_APP, "Error allocating new CSA patterns for MBSFN Area Id " MBSFN_AREA_ID_FMT".\n", mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
	}
	OAILOG_INFO(LOG_MCE_APP, "Successfully allocated (%d) CSA resources for MBSFN Area Id " MBSFN_AREA_ID_FMT ".\n",
			csa_patterns_mbsfn_p->num_csa_pattern, mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

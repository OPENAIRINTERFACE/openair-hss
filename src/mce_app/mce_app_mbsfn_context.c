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

/*! \file mce_app_mbsfn_context.c
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
void mce_app_calculate_csa_common_pattern(struct csa_patterns_s * csa_patterns_p, mchs_t * mchs, uint8_t num_mbsfn_idx, uint8_t total_mbsfn_areas_tbs);

//------------------------------------------------------------------------------
static
void mce_app_reuse_csa_pattern(struct csa_patterns_s * csa_patterns_mbsfn_p, mchs_t * mchs, const struct csa_patterns_s * const csa_patterns_alloced);

//------------------------------------------------------------------------------
static
int mce_app_alloc_csa_pattern(struct csa_patterns_s * new_csa_patterns, struct mchs_s * mchs, const struct csa_patterns_s * csa_patterns_allocated) ;

//------------------------------------------------------------------------------
static
int mce_app_calculate_overall_csa_pattern(struct csa_patterns_s * const csa_patterns_p, mchs_t * const mchs, const uint8_t num_mbsfn_idx,
		const uint8_t total_mbsfn_areas_to_be_scheduled, struct csa_patterns_s * const union_csa_patterns_allocated_p, const struct mbsfn_area_context_s * mbsfn_area_context);

//------------------------------------------------------------------------------
static
bool mce_mbsfn_area_compare_by_mbms_sai (__attribute__((unused)) const hash_key_t keyP,
		void * const elementP,
		void * parameterP, void **resultP);

//------------------------------------------------------------------------------
static
bool mce_mbsfn_area_remove_mbms_service (__attribute__((unused)) const hash_key_t keyP,
                                    void * const elementP,
                                    void * parameterP, void __attribute__((unused)) **resultP);
//------------------------------------------------------------------------------
static
void mce_app_log_method_single_rf_csa_pattern(struct csa_patterns_s * new_csa_patterns, int num_radio_frames,
		struct mchs_s * mchs,
		struct csa_patterns_s * csa_patterns_allocated);

//------------------------------------------------------------------------------
static
void mce_app_calculate_mbsfn_mchs(struct mbsfn_area_context_s * const mbsfn_area_context, mbms_services_t * const mbms_services_active, mchs_t *const mchs);

//------------------------------------------------------------------------------
static
bool mce_app_update_mbsfn_area(const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, const uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id);

//------------------------------------------------------------------------------
static int
mce_insert_mbsfn_area_context (
  mce_mbsfn_area_contexts_t * const mce_mbsfn_area_contexts_p, const struct mbsfn_area_context_s *const mbsfn_area_context);

//------------------------------------------------------------------------------
static
void clear_mbsfn_area(mbsfn_area_context_t * const mbsfn_area_context);

//------------------------------------------------------------------------------
static
bool mce_app_create_mbsfn_area(const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, const uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id);

//------------------------------------------------------------------------------
static
void mce_update_mbsfn_area(struct mbsfn_areas_s * const mbsfn_areas, const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id);

//------------------------------------------------------------------------------
static void mce_app_allocate_4frame(struct csa_patterns_s * new_csa_patterns, int num_radio_frames, struct mchs_s * mchs, struct csa_patterns_s * csa_patterns_allocated);

//------------------------------------------------------------------------------
static bool mce_mbsfn_area_reset_m2_enb_id (__attribute__((unused))const hash_key_t keyP,
               void * const mbsfn_area_ctx_ref,
               void * m2_enb_id_P,
               void __attribute__((unused)) **unused_resultP);

//------------------------------------------------------------------------------
static bool mce_mbsfn_area_reset_mbms_service (__attribute__((unused))const hash_key_t keyP,
               void * const mbsfn_area_ctx_ref,
               void * mbms_service_idx_P,
               void __attribute__((unused)) **unused_resultP);

//------------------------------------------------------------------------------
static
void mce_app_set_fresh_radio_frames(struct csa_pattern_s * csa_pattern_mbsfn, struct mchs_s * mchs);

//------------------------------------------------------------------------------
struct mbsfn_area_context_s                           *
mce_mbsfn_area_exists_mbsfn_area_id(
  mce_mbsfn_area_contexts_t * const mce_mbsfn_areas_p, const mbsfn_area_id_t mbsfn_area_id)
{
  struct mbsfn_area_context_s                    *mbsfn_area_context = NULL;
  hashtable_ts_get (mce_mbsfn_areas_p->mbsfn_area_id_mbsfn_area_htbl, (const hash_key_t)mbsfn_area_id, (void **)&mbsfn_area_context);
  return mbsfn_area_context;
}
//
////------------------------------------------------------------------------------
//void mce_app_get_active_mbms_services (const mbsfn_area_ids_t * const mbsfn_area_ids,
//		const mcch_modification_periods_t * const mcch_modif_periods,
//		mbms_service_indexes_t * const mbms_service_indexes_active,
//		const mbms_service_indexes_t * const mbms_service_indexes_tbr)
//{
//	OAILOG_FUNC_IN(LOG_MCE_APP);
//	mbsfn_area_context_t			  *mbsfn_area_context 					= NULL;
//	/** Make a callback for all MBSFN areas. */
//	for(int num_mbsfn_area = 0; num_mbsfn_area < mbsfn_area_ids->num_mbsfn_area_ids; num_mbsfn_area){
//		mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_ids->mbsfn_area_id[num_mbsfn_area]);
//		DevAssert(mbsfn_area_context);
//		/** Collect all active MBMS services of the MBSFN area. */
//		mbms_service_indexes_t mbms_service_indexes_per_mbsfn = {0}, *mbms_service_indexes_per_mbsfn_p = &mbms_service_indexes_per_mbsfn;
//		hashtable_apply_callback_on_elements(mbsfn_area_context->privates.mbms_service_idx_mcch_modification_times_hashmap,
//				mce_app_get_active_mbms_services_per_mbsfn_area, (void*)mcch_modif_periods, (void**)&mbms_service_indexes_per_mbsfn_p);
//		for(int num_mbms_service_mbsfn = 0; num_mbms_service_mbsfn < mbms_service_indexes_per_mbsfn.num_mbms_service; num_mbms_service_mbsfn++ ){
//			for(int num_mbms_service_tbr = 0; num_mbms_service_tbr < mbms_service_indexes_tbr->num_mbms_service; num_mbms_service_tbr++ ){
//				if(mbms_service_indexes_tbr->mbms_service_index_array[num_mbms_service_tbr] == mbms_service_indexes_per_mbsfn->mbms_service_index_array[num_mbms_service_mbsfn]){
//					OAILOG_ERROR(LOG_MCE_APP, "MBMS service idx "MBMS_SERVICE_INDEX_FMT " is already set to be preempted. Not adding to active list.\n",
//							mbms_service_indexes_per_mbsfn->mbms_service_index_array[num_mbms_service_mbsfn]);
//					break;
//				}
//				OAILOG_DEBUG(LOG_MCE_APP, "MBMS service idx "MBMS_SERVICE_INDEX_FMT " is not as preempted. Adding to active list.\n",
//						mbms_service_indexes_per_mbsfn->mbms_service_index_array[num_mbms_service_mbsfn]);
//				mbms_service_indexes_active->mbms_service_index_array[mbms_service_indexes_active->num_mbms_service++] =
//						mbms_service_indexes_per_mbsfn->mbms_service_index_array[num_mbms_service_mbsfn];
//			}
//		}
//		OAILOG_INFO(LOG_MCE_APP, "(%d) active MBMS services to check after checking MBSFN area " MBSFN_AREA_ID_FMT".\n",
//				mbms_service_indexes_active->num_mbms_service, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
//	}
//	OAILOG_INFO(LOG_MCE_APP, "Total (%d) active MBMS services after checking all (%d) MBSFN areas.\n",
//			mbms_service_indexes_active->num_mbms_service, mbsfn_area_ids->num_mbsfn_area_ids);
//	OAILOG_FUNC_OUT(LOG_MCE_APP);
//}


////------------------------------------------------------------------------------
//void mce_app_get_active_mbms_services (const mbsfn_area_id_t const mbsfn_area_id,
//		const mcch_modification_periods_t * const mcch_modif_periods,
//		mbms_service_indexes_t * const mbms_service_indexes_active)
//{
//	OAILOG_FUNC_IN(LOG_MCE_APP);
//	mbsfn_area_context_t			  *mbsfn_area_context 					= NULL;
//	/** Make a callback for all MBSFN areas. */
//	for(int num_mbsfn_area = 0; num_mbsfn_area < mbsfn_area_ids->num_mbsfn_area_ids; num_mbsfn_area){
//		mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_ids->mbsfn_area_id[num_mbsfn_area]);
//		DevAssert(mbsfn_area_context);
//		/** Collect all active MBMS services of the MBSFN area. */
//		hashtable_apply_callback_on_elements(mbsfn_area_context->privates.mbms_service_idx_mcch_modification_times_hashmap,
//				mce_app_get_active_mbms_services_per_mbsfn_area, (void*)mcch_modif_periods, (void**)&mbms_service_indexes_active);
////		for(int num_mbms_service_mbsfn = 0; num_mbms_service_mbsfn < mbms_service_indexes_per_mbsfn.num_mbms_service; num_mbms_service_mbsfn++ ){
////			for(int num_mbms_service_tbr = 0; num_mbms_service_tbr < mbms_service_indexes_tbr->num_mbms_service; num_mbms_service_tbr++ ){
////				if(mbms_service_indexes_tbr->mbms_service_index_array[num_mbms_service_tbr] == mbms_service_indexes_per_mbsfn->mbms_service_index_array[num_mbms_service_mbsfn]){
////					OAILOG_ERROR(LOG_MCE_APP, "MBMS service idx "MBMS_SERVICE_INDEX_FMT " is already set to be preempted. Not adding to active list.\n",
////							mbms_service_indexes_per_mbsfn->mbms_service_index_array[num_mbms_service_mbsfn]);
////					break;
////				}
////				OAILOG_DEBUG(LOG_MCE_APP, "MBMS service idx "MBMS_SERVICE_INDEX_FMT " is not as preempted. Adding to active list.\n",
////						mbms_service_indexes_per_mbsfn->mbms_service_index_array[num_mbms_service_mbsfn]);
////				mbms_service_indexes_active->mbms_service_index_array[mbms_service_indexes_active->num_mbms_service++] =
////						mbms_service_indexes_per_mbsfn->mbms_service_index_array[num_mbms_service_mbsfn];
////			}
////		}
//		OAILOG_INFO(LOG_MCE_APP, "(%d) active MBMS services to check after checking MBSFN area " MBSFN_AREA_ID_FMT".\n",
//				mbms_service_indexes_active->num_mbms_service, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
//	}
//	OAILOG_INFO(LOG_MCE_APP, "Total (%d) active MBMS services after checking all (%d) MBSFN areas.\n",
//			mbms_service_indexes_active->num_mbms_service, mbsfn_area_ids->num_mbsfn_area_ids);
//	OAILOG_FUNC_OUT(LOG_MCE_APP);
//}

//------------------------------------------------------------------------------
void
mce_mbsfn_areas_exists_mbms_service_area_id(
		const mce_mbsfn_area_contexts_t * const mce_mbsfn_areas_p, const mbms_service_area_id_t mbms_service_area_id, struct mbsfn_area_ids_s * mbsfn_area_ids)
{
  hashtable_rc_t              h_rc 					= HASH_TABLE_OK;
	mbsfn_area_id_t							mbsfn_area_id = INVALID_MBSFN_AREA_ID;
	mbms_service_area_id_t     *mbms_sai_p 		= (mbms_service_area_id_t*)&mbms_service_area_id;
	hashtable_ts_apply_callback_on_elements(mce_mbsfn_areas_p->mbsfn_area_id_mbsfn_area_htbl, mce_mbsfn_area_compare_by_mbms_sai, (void *)mbms_sai_p, (void**)&mbsfn_area_ids);
}

//------------------------------------------------------------------------------
int mce_app_mbsfn_area_register_mbms_service(mbsfn_area_context_t * mbsfn_area_context, mbms_service_index_t mbms_service_index,
		const uint32_t sec_since_epoch, const long double usec_since_epoch, const mbms_session_duration_t * mbms_session_duration)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);

	/** For the MBSFN area, calculate the MCCH modification start and stop timers first. */
	mcch_modification_periods_t * mcch_modif_periods = calloc(1, sizeof(mcch_modification_periods_t));
  mce_app_calculate_mbms_service_mcch_periods(sec_since_epoch, usec_since_epoch, mbms_session_duration,
  		mbsfn_area_context->privates.fields.mbsfn_area.mcch_modif_period_rf, mcch_modif_periods);
  if(!(mcch_modif_periods->mcch_modif_start_abs_period && mcch_modif_periods->mcch_modif_stop_abs_period)){
  	free_wrapper(&mcch_modif_periods);
  	OAILOG_ERROR(LOG_MCE_APP, "MBMS Service Idx " MBMS_SERVICE_INDEX_FMT " has invalid or expired start/end times for the MBSFN area. \n", mbms_service_index);
  	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
  }

  /** Check if the MBMS service is already registered in the MBSFN area, if so, just modify the ending. */
	mcch_modification_periods_t * old_mcch_modification_periods NULL;
	hashtable_ts_get(mbsfn_area_context->privates.mbms_service_idx_mcch_modification_times_hashmap, (hash_key_t)mbms_service_index, (void**)&old_mcch_modification_periods);
	if(old_mcch_modification_periods)
	{
		OAILOG_WARNING(LOG_MCE_APP, "MBMS Service Idx " MBMS_SERVICE_INDEX_FMT " was already registered in MBSFN area " MBSFN_AREA_ID_FMT". "
				"Changing MCCH modification periods (END). \n", mbms_service_index, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
		/** Set the original start time. */
		if(old_mcch_modification_periods->mcch_modif_start_abs_period)
			mcch_modif_periods->mcch_modif_start_abs_period = old_mcch_modification_periods->mcch_modif_start_abs_period;
		/** Remove the values. */
		hashtable_ts_remove(mbsfn_area_context->privates.mbms_service_idx_mcch_modification_times_hashmap, mbms_service_index);
	}
	/** Insert the new values. */
	hashtable_rc_t hash_rc = hashtable_ts_insert(mbsfn_area_context->privates.mbms_service_idx_mcch_modification_times_hashmap, mbms_service_index, mcch_modif_periods);
  OAILOG_FUNC_RETURN(LOG_MCE_APP, (HASH_TABLE_OK == hash_rc) ? RETURNok: RETURNerror);
}

//------------------------------------------------------------------------------
bool mce_app_check_mbsfn_mcch_modif (const hash_key_t keyP,
               void * const mbsfn_area_context_ref,
               void * parameterP,
               void **resultP)
{
	long 											  mcch_repeat_rf_abs  		= *((long*)parameterP);
	mbsfn_area_id_t			 	      mbsfn_area_id 					= (mbsfn_area_id_t)keyP;
	mbsfn_areas_t	 	      		 *mbsfn_areas							= (mbsfn_areas_t*)*resultP;

	/*** Get the MBSFN areas to be modified. */
	mbsfn_area_context_t * mbsfn_area_context = (mbsfn_area_context_t*)mbsfn_area_context_ref;
	/** Assert that the bitmap is not full. Capacity should have been checked before. */
//	DevAssert(mbsfn_areas->mbsfn_csa_offsets != 0xFF);

	/** MBMS service may have started before. And should prevail in the given MCCH modification period. */
	if(mcch_repeat_rf_abs % mbsfn_area_context->privates.fields.mbsfn_area.mcch_modif_period_rf){
		OAILOG_DEBUG(LOG_MCE_APP, "MBSFN Area " MBSFN_AREA_ID_FMT " MCCH modification period not reached yet for "
				"MCCH repetition RF (%d).\n", mbsfn_area_id, mcch_repeat_rf_abs);
		return false;
	}
	OAILOG_INFO(LOG_MCE_APP, "MBSFN Area " MBSFN_AREA_ID_FMT " MCCH modification period REACHED for "
					"MCCH repetition RF (%d).\n", mbsfn_area_id, mcch_repeat_rf_abs);
	// 8 CSA patterns per MBSFN area are allowed, currently just once considered!!
	/**
	 * To calculate the CSA[7], need too MBSFN areas and # of the current MBSFN area.
	 */
	long mcch_modif_period_abs[2] = {
			mcch_repeat_rf_abs / mbsfn_area_context->privates.fields.mbsfn_area.mcch_modif_period_rf,
			mcch_repeat_rf_abs / mbsfn_area_context->privates.fields.mbsfn_area.mcch_modif_period_rf
	};

	/**
	 * MBSFN Areas overall object will be returned in the method below.
	 */
	if(!mce_app_check_mbsfn_resources(keyP, mbsfn_area_context_ref, mcch_modif_period_abs,
			&mbsfn_areas)){
		OAILOG_DEBUG(LOG_MCE_APP, "MBSFN Area " MBSFN_AREA_ID_FMT " MCCH modification period REACHED for "
				"MCCH repetition RF (%d). No CSA modification detected. \n", mbsfn_area_id, mcch_repeat_rf_abs);
		return false;
	}

// todo: check for changes in the cSA pattern of the MBSFN context here.
//	/** Check for changes in the CSA pattern. */
//	bool change = memcmp((void*)&mbsfn_area_context->privates.fields.mbsfn_area.csa_patterns, (void*)&new_csa_patterns, sizeof(struct csa_patterns_s)) != 0;
//
//	pthread_rwlock_trywrlock(&mce_app_desc.rw_lock);	// todo: lock mce_desc
//	memcpy((void*)&mbsfn_area_context->privates.fields.mbsfn_area.csa_patterns, (void*)&new_csa_patterns, sizeof(struct csa_patterns_s));
//	// todo: update other fields..
//	pthread_rwlock_unlock(&mce_app_desc.rw_lock);

	// todo: assume that MCCH modification timer increments even when no update occurs.
	OAILOG_DEBUG(LOG_MCE_APP, "MBSFN Area " MBSFN_AREA_ID_FMT " MCCH modification period REACHED for "
			"MCCH repetition RF (%d). CSA modification detected. Updating the scheduling. \n", mbsfn_area_id, mcch_repeat_rf_abs);
	memcpy((void*)&mbsfn_areas->mbsfn_area_cfg[mbsfn_areas->num_mbsfn_areas++].mbsfnArea, (void*)&mbsfn_area_context->privates.fields.mbsfn_area, sizeof(mbsfn_area_t));
	/**
	 * MBSFN area is to be modified (MBMS service was added, removed or modified).
	 * Iterate through the whole list.
	 */
	// todo: if the bitmap is full, we might return true..
	return false;
}

/**
 * Check the resources for the given period over all MBSFN areas.
 * We assume that the M2 eNBs share the band for multiple MBSFN areas, thus also the resources.
 */
//------------------------------------------------------------------------------
bool mce_app_check_mbsfn_neighbors (const hash_key_t keyP,
               void * const mbsfn_area_context_ref,
               void * parameterP,
               void **resultP) {
	mbsfn_area_context_t       *mbsfn_area_context      				= (mbsfn_area_context_t*)mbsfn_area_context_ref;
	mbsfn_area_id_t			 	      mbsfn_area_id 									= (mbsfn_area_id_t)keyP;
	mbsfn_area_ids_t					 *mbsfn_area_ids_p								= *(mbsfn_area_ids_t**)resultP;
	uint8_t										  local_mbms_area									= *((uint8_t*)parameterP);
	uint8_t										  non_local_global_areas_allowed 	= 0;

	mme_config_read_lock(&mme_config);
	non_local_global_areas_allowed = mme_config.mbms.mbms_global_mbsfn_area_per_local_group;
	mme_config_unlock(&mme_config);

	/** Is the local MBMS area 0? (non-local global MBMS service area). */
	if(!local_mbms_area){
		/**
		 * We changed a non-local global MBSFN area.
		 * Check the configuration if local-specific global areas are allowed.
		 * If not, we need to separately check over all global and local MBMS service areas.
		 */
		if(mbsfn_area_context->privates.fields.local_mbms_area){
			OAILOG_INFO(LOG_MCE_APP, "We changed a non-local global MBSFN area. Adding all non-local global MBSFN areas, and (dep. on configuration) ALL local MBSFN areas. \n");
			if(!non_local_global_areas_allowed){
				OAILOG_INFO(LOG_MCE_APP, "Add adding MBSFN area "MBSFN_AREA_ID_FMT " with MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT " in local_mbms_area (%d) into the results.\n",
						mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id,
						mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id,
						mbsfn_area_context->privates.fields.local_mbms_area);
				mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].mbsfn_area_id 				= mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id;
				mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].mbms_service_area_id	= mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id;
				mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].num_mbsfn_area_ids++;
			} else {
				OAILOG_INFO(LOG_MCE_APP, "Not adding local global MBSFN area "MBSFN_AREA_ID_FMT " with MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT " in local_mbms_area (%d) into the results.\n",
						mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id,
						mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id,
						mbsfn_area_context->privates.fields.local_mbms_area);
			}
			return false;
		}
		/**
		 * Always add non-local global MBSFN areas into the calculation.
		 * Configuration unimportant. We will check all non-local global and each local MBMS area.
		 */
		OAILOG_INFO(LOG_MCE_APP, "Adding non-local global MBSFN area "MBSFN_AREA_ID_FMT " with MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT " into the results.\n",
				mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id,
				mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id);
		mbsfn_area_ids_p[0].mbsfn_area_id 				= mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id;
		mbsfn_area_ids_p[0].mbms_service_area_id	= mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id;
		mbsfn_area_ids_p[0].num_mbsfn_area_ids++;
		return false;
	}
	/** We changed a local MBMS service area. */
	if(mbsfn_area_context->privates.fields.local_mbms_area == local_mbms_area) {
		/** Add it no matter the configuration. */
		OAILOG_INFO(LOG_MCE_APP, "Adding local MBSFN area "MBSFN_AREA_ID_FMT " with MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT " in local_mbms_area (%d) into the results.\n",
				mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id,
				mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id,
				mbsfn_area_context->privates.fields.local_mbms_area);
		mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].mbsfn_area_id 				= mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id;
		mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].mbms_service_area_id	= mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id;
		mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].num_mbsfn_area_ids++;
		return false;
	} else if(!mbsfn_area_context->privates.fields.local_mbms_area){
		/** Check the configuration, only add it, if local-global MBMS service areas are not allowed. */
		if(!non_local_global_areas_allowed){
			mbsfn_area_ids_p[0].mbsfn_area_id 				= mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id;
			mbsfn_area_ids_p[0].mbms_service_area_id	= mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id;
			mbsfn_area_ids_p[0].num_mbsfn_area_ids++;
			OAILOG_INFO(LOG_MCE_APP, "Added non-LOCAL-global MBSFN area "MBSFN_AREA_ID_FMT " with MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT "  into the results.\n",
					mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id,
					mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id);
		}
		return false;
	}
	OAILOG_INFO(LOG_MCE_APP, "Not adding MBSFN area "MBSFN_AREA_ID_FMT " with MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT " in local_mbms_area (%d) into the results.\n",
			mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id,
			mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id,
			mbsfn_area_context->privates.fields.local_mbms_area);
	return false;
}

//------------------------------------------------------------------------------
bool mce_app_get_active_mbms_services_per_mbsfn_area (const hash_key_t keyP,
               void * const mcch_modif_periods_Ref,
               void * parameterP,
               void **resultP)
{
	mbms_service_index_t 	       mbms_service_idx 		 						= (mbms_service_index_t)keyP;
	mcch_modification_periods_t *mcch_modification_periods_in 		= (mcch_modification_periods_t*)parameterP;
	mcch_modification_periods_t *mcch_modification_periods_mbsfn	= (mcch_modification_periods_t*)mcch_modif_periods_Ref;
	mbms_service_indexes_t   		*mbms_service_indexes							= (mbms_service_indexes_t*)*resultP;
	/**
	 * Check active services, for start and end MCCH modification periods.
	 * MBMS service may have started before. And should prevail in the given MCCH modification period.
	 */
	if(mcch_modification_periods_mbsfn->mcch_modif_start_abs_period <= mcch_modification_periods_in->mcch_modif_stop_abs_period
			&& mcch_modification_periods_in->mcch_modif_start_abs_period <= mcch_modification_periods_mbsfn->mcch_modif_stop_abs_period){
		/**
		 * Received an active MBMS service, whos start/stop intervals overlap with the given intervals.
		 * Add it to the service of active MBMS.
		 */
		mbms_service_indexes->mbms_service_index_array[mbms_service_indexes->num_mbms_service++] = mbms_service_idx;
	}
	/** Iterate through the whole list. */
	return false;
}

///**
// * Check the resources for the given period over all MBSFN areas.
// * We assume that the M2 eNBs share the band for multiple MBSFN areas, thus also the resources.
// */
////------------------------------------------------------------------------------
//bool mce_app_check_mbsfn_resources (const hash_key_t keyP,
//               void * const mbsfn_area_context_ref,
//               void * parameterP,
//               void **resultP)
//{
//	OAILOG_FUNC_IN(LOG_MCE_APP);
//
//	/**
//	 * Array of two MCCH modification periods start and end.
//	 */
////	long 											 *mcch_modif_periods_abs	= ((long*)parameterP);
//	mbsfn_area_context_t       *mbsfn_area_context      = (mbsfn_area_context_t*)mbsfn_area_context_ref;
//	mchs_t										  mchs										= {0};
//	mbsfn_area_id_t			 	      mbsfn_area_id 					= (mbsfn_area_id_t)keyP;
//	mbms_services_t 			 			mbms_services_active 		= {0},
//														 *mbms_services_active_p 	= &mbms_services_active;
//
//	/** CSA pattern allocation for the actual MBSFN area. May contain new patterns and reuse old patterns. */
//	struct csa_patterns_s 		  new_csa_patterns 			  = {0};
//	/** Contain the allocated MBSFN Areas. CSA Pattern Allocation for the current MBSFN area will be done based on the already received MBSFN areas. */
//	mbsfn_areas_t							 *mbsfn_areas							= (mbsfn_areas_t*)*resultP;
//
//	/** Check the total MBMS Resources across MBSFN areas for the MCCH modification period in question. */
//	mme_config_read_lock(&mme_config);
//	mbms_service_t * mbms_services_active_array[mme_config.mbms.max_mbms_services];
//	memset(&mbms_services_active_array, 0, (sizeof(mbms_service_t*) * mme_config.mbms.max_mbms_services));
//	mbms_services_active.mbms_service_array = mbms_services_active_array;
//	mme_config_unlock(&mme_config);
//
//	/**
//	 * Get all MBMS services which are active in the given absolute MCCH period of the MBMS Service START!!
//	 * Calculate the total CSA period duration, where we will check the capacity afterwards
//	 * Contains the precise numbers of MBSFN subframes needed for data, including the ones, that will be scheduled for the last common CSA Pattern[7].
//	 * The mbsfn_areas object, which contains the allocated resources for the other MBSFN areas, should be updated automatically.
//	 */
//
//	/** Get the active MBMS services for the MBSFN area. */
////	long parametersP[3] = {mcch_modif_periods_abs[0], mcch_modif_periods_abs[1], mbsfn_area_id};
//	hashtable_ts_apply_callback_on_elements((hash_table_ts_t * const)mce_app_desc.mce_mbms_service_contexts.mbms_service_index_mbms_service_htbl,
//			mce_app_get_active_mbms_services, (void*)parametersP, (void**)&mbms_services_active_p);
//	mce_app_calculate_mbsfn_mchs(mbsfn_area_context, &mbms_services_active, &mchs);
//
//	struct csa_patterns_s *resulting_csa_patterns =NULL; // CALCULATE FROM THE MBSFN AREAS!
//	/**
//	 * In consideration of the total number of MBSFN with which we calculate CSA[7], where we reserve subframes for the MCCH,
//	 * calculate the subframe for the MBSFN area.
//	 */
//	if(mce_app_calculate_overall_csa_pattern(&new_csa_patterns, &mchs, mbsfn_areas->num_mbsfn_areas,
//			mbsfn_areas->num_mbsfn_areas_to_be_scheduled, resulting_csa_patterns, mbsfn_area_context) == RETURNerror) {
//		OAILOG_ERROR(LOG_MCE_APP, "CSA pattern for MBSFN Area " MBSFN_AREA_ID_FMT" does exceed resources.\n", mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
//		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
//	}
//	OAILOG_INFO(LOG_MCE_APP, "Successfully calculated CSA pattern for MBSFN Area " MBSFN_AREA_ID_FMT ". Checking resources. \n",
//			mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
//	/**
//	 * Update the MBSFN Areas with a new configuration.
//	 * We wan't to make sure, not to modify the MBSFN areas object in the method mce_app_calculate_overall_csa_pattern,
//	 * that's why we copy it.
//	 */
//	memcpy((void*)&mbsfn_areas->mbsfn_area_cfg[mbsfn_areas->num_mbsfn_areas].mbsfnArea.csa_patterns,
//			(void*)&new_csa_patterns, sizeof(struct csa_patterns_s));
//	mbsfn_areas->num_mbsfn_areas++;
//
//	/**
//	 * We update the union of CSA patterns allocated.
//	 * We iterate over the newly received CSA patterns and update the existing patterns with the same RF Pattern, periodicity and offset.
//	 *
//	 * CSA patterns, with existing offsets, should have the same periodicity and the same RF pattern (1/4).
//	 * So just check the offsets.
//	 * todo: array indexes in the union!
//	 * todo: the offsets might be the same, for reused ones, check that!
//	 */
//	resulting_csa_patterns->total_csa_pattern_offset |= new_csa_patterns.total_csa_pattern_offset;
//	/** Add the CSA patterns in total. */
//	// todo: update by offset --> CSA offset is the real identifier!
//	// todo: COMMON should not be updated, once set at the beginning
//	/** Update the reused CSA patterns with the same offset, with the new subframes. */
//	for(int num_csa_pattern = 0; num_csa_pattern < new_csa_patterns.num_csa_pattern; num_csa_pattern++){
//		uint8_t csa_pattern_offset_rf = new_csa_patterns.csa_pattern[num_csa_pattern].csa_pattern_offset_rf;
//		/** Check the already existing ones. */
//		for(int num_csa_pattern_old = 0; num_csa_pattern_old < resulting_csa_patterns->num_csa_pattern; num_csa_pattern++){
//			/** If the offset is the same, assert that the pattern and periodicity are also the same. */
//			if(resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_offset_rf == csa_pattern_offset_rf){
//				/** Same Pattern. */
//				DevAssert(resulting_csa_patterns->csa_pattern[num_csa_pattern_old].mbms_csa_pattern_rfs ==
//						new_csa_patterns.csa_pattern[num_csa_pattern].mbms_csa_pattern_rfs);
//				/** Same Periodicity. */
//				DevAssert(resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_repetition_period_rf ==
//						new_csa_patterns.csa_pattern[num_csa_pattern].csa_pattern_repetition_period_rf);
//				/** Update the set subframes. */
//				*((uint32_t*)&resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_sf) =
//						*((uint32_t*)&resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_sf)
//						| *((uint32_t*)&new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_sf);
//				OAILOG_INFO(LOG_MCE_APP, "Updated the existing CSA pattern with offset (%d) and repetition period(%d). Resulting new CSA subframes (%d).\n",
//						resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_offset_rf, resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_repetition_period_rf,
//						*((uint32_t*)&resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_sf));
//				break; /**< Continue with the next used one. */
//			}
//		}
//		/**
//		 * No matching CSA subframe was found in the already allocated CSA patterns.
//		 * Allocate a new one in the resulting CSA patterns.
//		 */
//		resulting_csa_patterns->csa_pattern[resulting_csa_patterns->num_csa_pattern].csa_pattern_offset_rf = csa_pattern_offset_rf;
//		resulting_csa_patterns->csa_pattern[resulting_csa_patterns->num_csa_pattern].csa_pattern_repetition_period_rf =
//				new_csa_patterns.csa_pattern[num_csa_pattern].csa_pattern_repetition_period_rf;
//		resulting_csa_patterns->csa_pattern[resulting_csa_patterns->num_csa_pattern].mbms_csa_pattern_rfs =
//				new_csa_patterns.csa_pattern[num_csa_pattern].mbms_csa_pattern_rfs;
//		memcpy((void*)&resulting_csa_patterns->csa_pattern[resulting_csa_patterns->num_csa_pattern].csa_pattern_sf,
//				(void*)&new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_sf, sizeof(uint32_t));
//		resulting_csa_patterns->num_csa_pattern++;
//		OAILOG_INFO(LOG_MCE_APP, "Added new CSA pattern with offset (%d) and repetition period(%d) to existing one. Resulting new CSA subframes (%d). Number of resulting CSA Subframes(%d). \n",
//				new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_offset_rf, new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_repetition_period_rf,
//				*((uint32_t*)&new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_sf), resulting_csa_patterns->num_csa_pattern
//		);
//	}
//
//	/**
//	 * Set the radio frame offset of all the CSA patterns used so far using the MBSFN Areas item.
//	 * Then also create a constant pointer to used CSA patterns.
//	 * Last CSA pattern is universal, shared among different MBSFN areas and includes the MCCHs in each MCCH repetition period
//	 * (other subframes of that CSA Patter Repetition period in the CSA Period are not used).
//	 */
////	todo: csa_patterns_p->total_csa_pattern_offset = mbsfn_areas->total_csa_pattern_offset;
//	/**
//	 * Set the current offset of CSA patterns.
//	 * todo: later optimize this..
//	 */
//
//	//	mbsfn_areas->total_csa_pattern_offset = new_csa_patterns.total_csa_pattern_offset;
//
//	OAILOG_FUNC_RETURN(LOG_MCE_APP, false);
//}

/**
 * In the strict order, calculate the global CSA pattern, if exists.
 * Then calculate the local CSA pattern. Regard the global CSA pattern and don't mix them!
 */
//------------------------------------------------------------------------------
int mce_app_check_mbsfn_resources (const mbsfn_area_context_t * const mbsfn_area_context,
		const mbms_service_indexes_t				* const mbms_service_indexes_active_nlg_p,
		const mbms_service_indexes_t				* const mbms_service_indexes_active_local_p)
{
	/**
	 * We don't recalculate resources, when they are removed. That's why use the CSA pattern calculation in each MCCH modification period.
	 * We check the resulting MBSFN areas, where the capacity should be split among and calculate the resources needed.
	 * Finally, we fill an CSA pattern offset bitmap.
	 * MBSFN Areas might share CSA Patterns (Radio Frames), but once a subframe is set for a given periodicity,
	 * the subframe will be allocated for a SINGLE MBSFN area only. The different CSA patterns will have a different offset.
	 * In total 8 CSA patterns can exist for all MBSFN areas.
	 *
	 * Subframe indicating the MCCH will be set at M2 Setup (initialization).
	 * ToDo: It also must be unique against different MBSFN area combinations!
	 */

	OAILOG_FUNC_IN(LOG_MCE_APP);
	mchs_t										  mchs										= {0};
	/** CSA pattern allocation for the actual MBSFN area. May contain new patterns and reuse old patterns. */
	struct csa_patterns_s 		  csa_patterns_local		  = {0};
	struct csa_patterns_s 		  csa_patterns_global			= {0};

	/**
	 * Get all MBMS services which are active in the given absolute MCCH period of the MBMS Service START!!
	 * Calculate the total CSA period duration, where we will check the capacity afterwards
	 * Contains the precise numbers of MBSFN subframes needed for data, including the ones, that will be scheduled for the last common CSA Pattern[7].
	 * The mbsfn_areas object, which contains the allocated resources for the other MBSFN areas, should be updated automatically.
	 */

	/** Take the first MBSFN area context, to get the phy layer properties. */
	mce_app_calculate_mbsfn_mchs(mbsfn_area_context, &mbms_services_active, &mchs);

	/**
	 * In consideration of the total number of MBSFN with which we calculate CSA[7], where we reserve subframes for the MCCH,
	 * calculate the subframe for the MBSFN area.
	 */
	for(int num_mbsfn_area = 0; num_mbsfn_area < mbsfn_area_ids->num_mbsfn_area_ids; num_mbsfn_area++){
		if(mce_app_calculate_overall_csa_pattern(&new_csa_patterns, &mchs, num_mbsfn_area,
				mbsfn_area_ids->num_mbsfn_area_ids, resulting_csa_patterns, mbsfn_area_context) == RETURNerror) {
			OAILOG_ERROR(LOG_MCE_APP, "CSA pattern (%d)th MBSFN Area does exceed resources.\n", num_mbsfn_area);
			OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
		}
		OAILOG_INFO(LOG_MCE_APP, "Successfully calculated CSA pattern for MBSFN Area " MBSFN_AREA_ID_FMT ". Checking resources. \n",
				mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
		/** Update the overall CSA pattern. */
		mce_app_update_csa_pattern_union(resulting_csa_patterns, &new_csa_patterns);
		/** Updated the union of CSA patterns, continue with the calculation. */
	}

	/**
	 * Set the radio frame offset of all the CSA patterns used so far using the MBSFN Areas item.
	 * Then also create a constant pointer to used CSA patterns.
	 * Last CSA pattern is universal, shared among different MBSFN areas and includes the MCCHs in each MCCH repetition period
	 * (other subframes of that CSA Patter Repetition period in the CSA Period are not used).
	 */
//	todo: csa_patterns_p->total_csa_pattern_offset = mbsfn_areas->total_csa_pattern_offset;
	/**
	 * Set the current offset of CSA patterns.
	 * todo: later optimize this..
	 */

	if(mbsfn_areas_p->num_mbsfn_areas == mbsfn_areas.num_mbsfn_areas_to_be_scheduled){
		/** We could schedule all MBSFN areas, including the latest received MBMS service. */
		OAILOG_INFO(LOG_MCE_APP, "MBSFN resources are enough to activate new MBMS Service Id " MBMS_SERVICE_INDEX_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " together with (%d) MBSFN areas. \n",
				mbms_service_index, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id, mbsfn_areas_p->num_mbsfn_areas);
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
	}

	//	mbsfn_areas->total_csa_pattern_offset = new_csa_patterns.total_csa_pattern_offset;
	OAILOG_INFO(LOG_MCE_APP, "Completed checking the MBSFN area resources.\n");
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

////------------------------------------------------------------------------------
//static
//bool mce_mbsfn_area_reset_m2_enb_id (__attribute__((unused))const hash_key_t keyP,
//               void * const mbsfn_area_ref,
//               void * m2_end_idP,
//               void __attribute__((unused)) **unused_resultP)
//{
//  const mbsfn_area_context_t * mbsfn_area_ctx =  mbsfn_area_context_t * mbsfn_area_ref;
//  if (mbsfn_area_ctx == NULL) {
//    return false;
//  }
//  m2ap_dump_enb(enb_ref);
//  return false;
//}

//------------------------------------------------------------------------------
void mce_app_reset_m2_enb_registration(const uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id) {
	OAILOG_FUNC_IN(LOG_MCE_APP);
	/**
	 * Apply a callback function on all registered MBSFN areas.
	 * Remove for each the M2 eNB and decrement the eNB count.
	 */
  hashtable_ts_apply_callback_on_elements(mce_app_desc.mce_mbsfn_area_contexts.mbsfn_area_id_mbsfn_area_htbl,
  		mce_mbsfn_area_reset_m2_enb_id, (void *)&m2_enb_id, NULL);

	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

//------------------------------------------------------------------------------
void mce_app_reset_mbsfn_service_registration(const mbms_service_index_t mbms_service_idx) {
	OAILOG_FUNC_IN(LOG_MCE_APP);
	/**
	 * Apply a callback function on all registered MBSFN areas.
	 * Remove for each the M2 eNB and decrement the eNB count.
	 */
  hashtable_ts_apply_callback_on_elements(mce_app_desc.mce_mbsfn_area_contexts.mbsfn_area_id_mbsfn_area_htbl,
  		mce_mbsfn_area_reset_mbms_service, (void *)&mbms_service_idx, NULL);

	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

//------------------------------------------------------------------------------
int mce_app_get_local_mbsfn_areas(const mbms_service_area_t *mbms_service_areas, const uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id, mbsfn_areas_t * const mbsfn_areas)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);

	mbsfn_area_id_t				  mbsfn_area_id      				= INVALID_MBSFN_AREA_ID;
	mbsfn_area_context_t	 *mbsfn_area_context 				= NULL; 									/**< Context stored in the MCE_APP hashmap. */
  /** A single local MBMS service area defined for all MBSFN areas. */
  uint8_t			  					local_mbms_service_area 	= 0;

	/**
   * For each MBMS Service area, check if it is a valid local MBMS Service area.
   * Create a dedicated MBSFN area for it.
   */
	for(int num_mbms_area = 0; num_mbms_area < mbms_service_areas->num_service_area; num_mbms_area++) {
		mbsfn_area_id = INVALID_MBSFN_AREA_ID;
		/**
		 * Get from the MME config, if the MBMS Service Area is global or not.
		 */
	  mme_config_read_lock (&mme_config);
		if(!mme_config.mbms.mbms_local_service_area_types || !mme_config.mbms.mbms_local_service_areas){
		  mme_config_unlock(&mme_config);
		  OAILOG_WARNING(LOG_MCE_APP, "No local MBMS Service Areas are configured.\n");
			OAILOG_FUNC_RETURN(LOG_MCE_APP, 0);
		}
		/** Get the MBSFN Area ID, should be a deterministic function, depending only on the MBMS Service Area Id. */
		if(mbms_service_areas->serviceArea[num_mbms_area] <= 0){
	  	/** Skip the MBMS Area. No modifications on the MBSFN area*/
			mme_config_unlock(&mme_config);
			continue;
	  }
		if(mbms_service_areas->serviceArea[num_mbms_area] <= mme_config.mbms.mbms_global_service_area_types){
			/** Global MBMS Service Areas will be checked later after Local MBMS Service Areas. */
		  mme_config_unlock(&mme_config);
		  continue;
		}
		/**
		 * Check if it is a valid local MBMS service area.
		 */
	  int val = (mbms_service_areas->serviceArea[num_mbms_area] - (mme_config.mbms.mbms_global_service_area_types + 1));
	  int local_area_index = val / mme_config.mbms.mbms_local_service_area_types; /**< 0..  mme_config.mbms.mbms_local_service_areas - 1. */
	  int local_area_type = val % mme_config.mbms.mbms_local_service_area_types;  /**< 0..  mme_config.mbms.mbms_local_service_area_types - 1. */
	  if(local_area_index < mme_config.mbms.mbms_local_service_areas) {
	  	/**
	  	 * Check that no other local service area is set for the eNB.
	  	 */
	  	if(local_mbms_service_area) {
	  		if(local_mbms_service_area != local_area_type +1){
	  			mme_config_unlock(&mme_config);
	  			OAILOG_ERROR(LOG_MCE_APP, "A local MBMS area (%d) is already set for the eNB. Skipping new local MBMS area (%d). \n",
	  					local_mbms_service_area, local_area_type +1);
	  			continue;
	  		}
	  		/** Continuing with the same local MBMS service area. */
	  	} else {
	  		local_mbms_service_area = local_area_type + 1;
	  		OAILOG_INFO(LOG_MCE_APP, "Setting local MBMS service area (%d) for the eNB. \n", local_mbms_service_area);
	  	}
	  	OAILOG_INFO(LOG_MCE_APP, "Found a valid local MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT ". \n", mbms_service_areas->serviceArea[num_mbms_area]);
	  	/** Return the MBSFN Area. */
	  	if(mme_config.mbms.mbms_global_mbsfn_area_per_local_group){
	  		mbsfn_area_id = mme_config.mbms.mbms_global_service_area_types
	  				+ local_area_type * (mme_config.mbms.mbms_local_service_area_types + mme_config.mbms.mbms_global_service_area_types) + (local_area_type +1);
	  	} else {
	  		/** Return the MBMS service area as the MBSFN area. We use the same identifiers. */
	  		mbsfn_area_id = mbms_service_areas->serviceArea[num_mbms_area];
	  	}
	  	/**
	  	 * Check if the MBSFN service area is already set for this particular request.
	  	 * In both cases, we should have unique MBSFN areas for the MBMS service areas.
	  	 */
			mme_config_unlock(&mme_config);
			mce_update_mbsfn_area(mbsfn_areas, mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area], m2_enb_id, assoc_id);
	  } else {
	  	OAILOG_ERROR(LOG_MCE_APP, "MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT " is not a valid local MBMS service area. Skipping. \n", mbms_service_areas->serviceArea[num_mbms_area]);
	  }
	}
	/** Get the local service area (geographical). */
	OAILOG_FUNC_RETURN(LOG_MCE_APP, local_mbms_service_area);
}

/**
 * Function to handle global MBSFN areas.
 * This assumes, that the MBSFN areas object already did undergone local MBMS service area operations.
 */
//------------------------------------------------------------------------------
void mce_app_get_global_mbsfn_areas(const mbms_service_area_t *mbms_service_areas, const uint32_t m2_enb_id,
		const sctp_assoc_id_t assoc_id, mbsfn_areas_t * const mbsfn_areas, int local_mbms_service_area)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	mbsfn_area_id_t					mbsfn_area_id			 = INVALID_MBSFN_AREA_ID;
	/** Iterate through the whole list again. */
	for(int num_mbms_area = 0; num_mbms_area < mbms_service_areas->num_service_area; num_mbms_area++) {
		mbsfn_area_id = INVALID_MBSFN_AREA_ID;
		/**
		 * Get from the MME config, if the MBMS Service Area is global or not.
		 */
	  mme_config_read_lock (&mme_config);
		if(!mme_config.mbms.mbms_global_service_area_types){
		  mme_config_unlock(&mme_config);
		  OAILOG_ERROR(LOG_MCE_APP, "No global MBMS Service Areas are configured.\n");
			OAILOG_FUNC_OUT(LOG_MCE_APP);
		}
		/** Get the MBSFN Area ID, should be a deterministic function, depending only on the MBMS Service Area Id. */
		if(mbms_service_areas->serviceArea[num_mbms_area] <= 0){
	  	/** Skip the MBMS Area. No modifications on the MBSFN area*/
		  mme_config_unlock(&mme_config);
			continue;
	  }
		/** Check if it is a local MBMS Area. */
		if(mbms_service_areas->serviceArea[num_mbms_area] > mme_config.mbms.mbms_global_service_area_types){
			/** Skip the local area. */
		  mme_config_unlock(&mme_config);
		  continue;
		}
		/**
		 * Found a global area. If the eNB has a local area and if we use local MBMS service areas,
		 * we use a different MBSFN a
		 * MBMS Service areas would be set. */
		if(mme_config.mbms.mbms_global_mbsfn_area_per_local_group) {
			/**
			 * We check if the eNB is in a local MBMS area, so assign a group specific MBSFN Id.
			 * todo: later --> Resources will be partitioned accordingly.
			 */
			if(local_mbms_service_area) {
				OAILOG_INFO(LOG_MCE_APP, "Configuring local MBMS group specific global MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " for local group %d.\n",
						mbms_service_areas->serviceArea[num_mbms_area], local_mbms_service_area);
				mbsfn_area_id = mme_config.mbms.mbms_global_service_area_types +
						(local_mbms_service_area -1) * (mme_config.mbms.mbms_local_service_area_types + mme_config.mbms.mbms_global_service_area_types)
								+ mbms_service_areas->serviceArea[num_mbms_area];
				/** If the MBSFN area is already allocated, skip to the next one.
				 * Found a new local specific valid global MBMS group.
				 * Check if there exists an MBSFN group for that.
				 */
  		  mme_config_unlock(&mme_config);
  			mce_update_mbsfn_area(mbsfn_areas, mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area], m2_enb_id, assoc_id);
  		  continue;
			}
		}
		/**
		 * Local specific global MBMS areas are not allowed or eNB does not belong to any local MBMS area.
		 * Return the MBSFN Id as the global MBMS Service Area.
		 */
		mbsfn_area_id = mbms_service_areas->serviceArea[num_mbms_area];
		/**
		 * If the MBSFN area is already allocated, skip to the next one.
		 * Found a new local specific valid global MBMS group.
		 * Check if there exists an MBSFN group for that.
		 */
		mce_update_mbsfn_area(mbsfn_areas, mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area], m2_enb_id, assoc_id);
		mme_config_unlock(&mme_config);
	}
	/** Done checking all global MBMS service areas. */
	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

//mce_app_get_active_mbms_services(mbsfn_area_ids, mcch_modification_periods, &mbms_services_active);
////------------------------------------------------------------------------------
//bool mce_app_get_active_mbms_services_2 (const mbsfn_area_ids_t * mbsfn_area_ids,
//		mcch_modification_periods_t * mcch_modification_periods,
//		mbms_services_t * mbms_services)
//{
//	/** Check active services, for start and end MCCH modification periods. */
//	long parameters[3] = {0, 0, 0};
//	parameters[0] = mcch_modification_periods->mcch_modif_start_abs_period;
//	parameters[1] = mcch_modification_periods->mcch_modif_stop_abs_period;
//	parameters[2] = mcch_modification_periods->mcch_modif_stop_abs_period;
//
//
//	hashtable_ts_apply_callback_on_elements(&mce_app_desc.mce_mbms_service_contexts, mce_app_get_active_mbms_services,)
//		/** MBMS service may have started before. And should prevail in the given MCCH modification period. */
//		if(mbms_service->privates.fields.mbms_service_mcch_start_period <= mcch_modif_period_abs_stop
//				&& mcch_modif_period_abs_start <= mbms_service->privates.fields.mbms_service_mcch_stop_period){
//			/**
//			 * Received an active MBMS service, whos start/stop intervals overlap with the given intervals.
//			 * Add it to the service of active MBMS.
//			 */
//			mbms_services->mbms_service_array[mbms_services->num_mbms_service++] = mbms_service;
//		}
//	}
//	/** Iterate through the whole list. */
//	return false;
//}

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
	if(!mbms_service_indexes_to_preemtp->num_mbms_service){
		OAILOG_ERROR("No active MBMS services received to preempt.\n");
  	OAILOG_FUNC_RETURN(LOG_MCE_APP, INVALID_MBMS_SERVICE_INDEX);
  }
	if(mbsfn_area_id != INVALID_MBSFN_AREA_ID){
		mbsfn_area_context_tp = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbms_service_contexts, mbsfn_area_id);
		DevAssert(mbsfn_area_context_tp);
	}

	/** Remove the MBMS session with the lowest ARP prio level. */
	for(int num_ms = 0; num_ms < mbms_service_indexes_to_preemtp->num_mbms_service; num_ms++){
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

//------------------------------------------------------------------------------
void
mce_app_mbsfn_remove_mbms_service(const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_sa_id) {
	void 									 *unusedP					 = NULL;
	mbms_service_index_t		mbms_service_idx = mce_get_mbms_service_index(tmgi, mbms_sa_id);
	hashtable_ts_apply_callback_on_elements(&mce_app_desc.mce_mbsfn_area_contexts, mce_mbsfn_area_reset_mbms_service, (void *)&mbms_service_idx, (void**)&unusedP);
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

//------------------------------------------------------------------------------
static
bool mce_mbsfn_area_compare_by_mbms_sai (__attribute__((unused)) const hash_key_t keyP,
                                    void * const elementP,
                                    void * parameterP, void **resultP)
{
  const mbms_service_area_id_t    * const mbms_sai_p 		= (const mbms_service_area_id_t*const)parameterP;
  mbsfn_area_context_t            * mbsfn_area_context  = (mbsfn_area_context_t*)elementP;
  mbsfn_area_ids_t								* mbsfn_area_ids_p		= (mbsfn_area_ids_t*)*resultP;
  /** Set the matched MBSFN area into the list. */
  if ( *mbms_sai_p == mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id) {
  	/** Set the MBSFN area id. */
    mbsfn_area_ids_p->mbsfn_area_id[mbsfn_area_ids_p->num_mbsfn_area_ids] = mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id;
    mbsfn_area_ids_p->mbms_service_area_id[mbsfn_area_ids_p->num_mbsfn_area_ids] = mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id;
    mbsfn_area_ids_p->num_mbsfn_area_ids++;
  }
  return false;
}

//------------------------------------------------------------------------------
static bool mce_mbsfn_area_reset_m2_enb_id (__attribute__((unused))const hash_key_t keyP,
               void * const mbsfn_area_ctx_ref,
               void * m2_enb_id_P,
               void __attribute__((unused)) **unused_resultP)
{
  const mbsfn_area_context_t * const mbsfn_area_ctx = (const mbsfn_area_context_t *)mbsfn_area_ctx_ref;
  if (mbsfn_area_ctx == NULL) {
    return false;
  }
  /**
   * Remove the key from the MBSFN Area context.
   * No separate counter is necessarry.*/
  hashtable_uint64_ts_remove(mbsfn_area_ctx->privates.m2_enb_id_hashmap, *((const hash_key_t*)m2_enb_id_P));
  return false;
}

//------------------------------------------------------------------------------
static bool mce_mbsfn_area_reset_mbms_service (__attribute__((unused))const hash_key_t keyP,
               void * const mbsfn_area_ctx_ref,
               void * mbms_service_idx_P,
               void __attribute__((unused)) **unused_resultP)
{
  const mbsfn_area_context_t * const mbsfn_area_ctx = (const mbsfn_area_context_t *)mbsfn_area_ctx_ref;
  /**
   * Remove the key from the MBSFN Area context.
   * No separate counter is necessary.
   */
  if (mbsfn_area_ctx) {
    hashtable_ts_remove(mbsfn_area_ctx->privates.mbms_service_idx_mcch_modification_times_hashmap, *((const hash_key_t*)mbms_service_idx_P));
  }
  return false;
}

//------------------------------------------------------------------------------
static
bool mce_mbsfn_area_remove_mbms_service (__attribute__((unused)) const hash_key_t keyP,
                                    void * const elementP,
                                    void * parameterP, void __attribute__((unused)) **resultP)
{
  const mbms_service_index_t      * const mbms_service_idx_p	= (const mbms_service_index_t *const)parameterP;
  mbsfn_area_context_t            * mbsfn_area_context  			= (mbsfn_area_context_t*)elementP;
  /** Set the matched MBSFN area into the list. */
  hashtable_ts_remove(mbsfn_area_context->privates.mbms_service_idx_mcch_modification_times_hashmap, (hash_key_t)*mbms_service_idx_p);
  return false;
}

//------------------------------------------------------------------------------
static
void mce_app_update_csa_pattern_union(struct csa_patterns_s * resulting_csa_patterns, const struct csa_patterns_s * const new_csa_patterns)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	/**
	 * We update the union of CSA patterns allocated.
	 * We iterate over the newly received CSA patterns and update the existing patterns with the same RF Pattern, periodicity and offset.
	 *
	 * CSA patterns, with existing offsets, should have the same periodicity and the same RF pattern (1/4).
	 * So just check the offsets.
	 * todo: array indexes in the union!
	 * todo: the offsets might be the same, for reused ones, check that!
	 */


	//	/**
	//	 * Update the MBSFN Areas with a new configuration.
	//	 * We wan't to make sure, not to modify the MBSFN areas object in the method mce_app_calculate_overall_csa_pattern,
	//	 * that's why we copy it.
	//	 */
	//	memcpy((void*)&mbsfn_areas->mbsfn_area_cfg[mbsfn_areas->num_mbsfn_areas].mbsfnArea.csa_patterns,
	//			(void*)&new_csa_patterns, sizeof(struct csa_patterns_s));
	//	mbsfn_areas->num_mbsfn_areas++;


	resulting_csa_patterns->total_csa_pattern_offset |= new_csa_patterns->total_csa_pattern_offset;
	/** Add the CSA patterns in total. */
	// todo: update by offset --> CSA offset is the real identifier!
	// todo: COMMON should not be updated, once set at the beginning
	/** Update the reused CSA patterns with the same offset, with the new subframes. */
	for(int num_csa_pattern = 0; num_csa_pattern < new_csa_patterns->num_csa_pattern; num_csa_pattern++){
		uint8_t csa_pattern_offset_rf = new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_offset_rf;
		/** Check the already existing ones. */
		for(int num_csa_pattern_old = 0; num_csa_pattern_old < resulting_csa_patterns->num_csa_pattern; num_csa_pattern++){
			/** If the offset is the same, assert that the pattern and periodicity are also the same. */
			if(resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_offset_rf == csa_pattern_offset_rf){
				/** Same Pattern. */
				DevAssert(resulting_csa_patterns->csa_pattern[num_csa_pattern_old].mbms_csa_pattern_rfs ==
						new_csa_patterns->csa_pattern[num_csa_pattern].mbms_csa_pattern_rfs);
				/** Same Periodicity. */
				DevAssert(resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_repetition_period_rf ==
						new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_repetition_period_rf);
				/** Update the set subframes. */
				*((uint32_t*)&resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_sf) =
						*((uint32_t*)&resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_sf)
						| *((uint32_t*)&new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_sf);
				OAILOG_INFO(LOG_MCE_APP, "Updated the existing CSA pattern with offset (%d) and repetition period(%d). Resulting new CSA subframes (%d).\n",
						resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_offset_rf,
						resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_repetition_period_rf,
						*((uint32_t*)&resulting_csa_patterns->csa_pattern[num_csa_pattern_old].csa_pattern_sf));
				break; /**< Continue with the next used one. */
			}
		}
		/**
		 * No matching CSA subframe was found in the already allocated CSA patterns.
		 * Allocate a new one in the resulting CSA patterns.
		 */
		resulting_csa_patterns->csa_pattern[resulting_csa_patterns->num_csa_pattern].csa_pattern_offset_rf = csa_pattern_offset_rf;
		resulting_csa_patterns->csa_pattern[resulting_csa_patterns->num_csa_pattern].csa_pattern_repetition_period_rf =
				new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_repetition_period_rf;
		resulting_csa_patterns->csa_pattern[resulting_csa_patterns->num_csa_pattern].mbms_csa_pattern_rfs =
				new_csa_patterns->csa_pattern[num_csa_pattern].mbms_csa_pattern_rfs;
		memcpy((void*)&resulting_csa_patterns->csa_pattern[resulting_csa_patterns->num_csa_pattern].csa_pattern_sf,
				(void*)&new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_sf, sizeof(uint32_t));
		resulting_csa_patterns->num_csa_pattern++;
		OAILOG_INFO(LOG_MCE_APP, "Added new CSA pattern with offset (%d) and repetition period(%d) to existing one. Resulting new CSA subframes (%d). Number of resulting CSA Subframes(%d). \n",
				new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_offset_rf, new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_repetition_period_rf,
				*((uint32_t*)&new_csa_patterns->csa_pattern[num_csa_pattern].csa_pattern_sf), resulting_csa_patterns->num_csa_pattern
		);
	}
}

//------------------------------------------------------------------------------
static
bool mce_app_update_mbsfn_area(const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, const uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id) {
	OAILOG_FUNC_IN(LOG_MCE_APP);
	mbsfn_area_context_t 									* mbsfn_area_context = NULL;
	if(!pthread_rwlock_trywrlock(&mce_app_desc.rw_lock)) {
		mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_id);
		if(mbsfn_area_context) {
			/** Found an MBSFN area, check if the eNB is registered. */
			if(hashtable_uint64_ts_is_key_exists (&mbsfn_area_context->privates.m2_enb_id_hashmap, (const hash_key_t)m2_enb_id) == HASH_TABLE_OK) {
	 			/** MBSFN Area contains eNB Id. Continuing. */
	 			DevMessage("MBSFN Area " + mbsfn_area_id + " has M2 eNB id " + m2_enb_id". Error during resetting M2 eNB.");
	 		}
	 		/**
	 		 * Updating the eNB count.
	 		 * MCS will be MCH specific of the MBSFN areas, and depend on the QCI/BLER.
	 		 */
			hashtable_uint64_ts_insert(&mbsfn_area_context->privates.m2_enb_id_hashmap, (const hash_key_t)m2_enb_id, NULL);
			/** Check if the MCCH timer is running, if not start it. */
			pthread_rwlock_unlock(&mce_app_desc.rw_lock);
			OAILOG_FUNC_RETURN (LOG_MME_APP, true);
		}
		OAILOG_INFO(LOG_MCE_APP, "No MBSFN Area could be found for the MBMS SAI " MBMS_SERVICE_AREA_ID_FMT ". Cannot update. \n", mbms_service_area_id);
		pthread_rwlock_unlock(&mce_app_desc.rw_lock);
	}
	OAILOG_FUNC_RETURN (LOG_MME_APP, false);
}

//------------------------------------------------------------------------------
static int
mce_insert_mbsfn_area_context (
  mce_mbsfn_area_contexts_t * const mce_mbsfn_area_contexts_p,
  const struct mbsfn_area_context_s *const mbsfn_area_context)
{
  OAILOG_FUNC_IN (LOG_MCE_APP);
  hashtable_rc_t                          h_rc 					= HASH_TABLE_OK;
  mbsfn_area_id_t													mbsfn_area_id = INVALID_MBSFN_AREA_ID;

  DevAssert (mce_mbsfn_area_contexts_p);
  DevAssert (mbsfn_area_context);
  mbsfn_area_id = mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id;
  DevAssert (mbsfn_area_id != INVALID_MBSFN_AREA_ID);
  DevAssert (mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id != INVALID_MBMS_SERVICE_AREA_ID);
  h_rc = hashtable_ts_is_key_exists (mce_mbsfn_area_contexts_p->mbsfn_area_id_mbsfn_area_htbl, (const hash_key_t)mbsfn_area_id);
  if (HASH_TABLE_OK == h_rc) {
	  OAILOG_ERROR(LOG_MCE_APP, "The MBSFN area " MBSFN_AREA_ID_FMT" is already existing. \n", mbsfn_area_id);
	  OAILOG_FUNC_RETURN (LOG_MCE_APP, RETURNerror);
  }
  h_rc = hashtable_ts_insert (mce_mbsfn_area_contexts_p->mbsfn_area_id_mbsfn_area_htbl, (const hash_key_t)mbsfn_area_id, (void *)mbsfn_area_context);
  if (HASH_TABLE_OK != h_rc) {
	  OAILOG_ERROR(LOG_MCE_APP, "Error could not register the MBSFN Area context %p with MBSFN Area Id " MBSFN_AREA_ID_FMT" and MBMS Service Index " MBMS_SERVICE_INDEX_FMT ". \n",
			  mbsfn_area_context, mbsfn_area_id, mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id);
	  OAILOG_FUNC_RETURN (LOG_MCE_APP, RETURNerror);
  }

  /** Also a separate map for MBMS Service Area is necessary. We then match MBMS Sm Service Request directly to MBSFN area. */
  if (mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id) {
  	h_rc = hashtable_uint64_ts_insert (mce_mbsfn_area_contexts_p->mbms_sai_mbsfn_area_ctx_htbl,
			  (const hash_key_t)mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id, (void *)((uintptr_t)mbsfn_area_id));
	  if (HASH_TABLE_OK != h_rc) {
		  OAILOG_ERROR(LOG_MCE_APP, "Error could not register the MBSFN Service context %p with MBSFN Area Id "MBSFN_AREA_ID_FMT" to MBMS Service Index " MBMS_SERVICE_INDEX_FMT ". \n",
				  mbsfn_area_context, mbsfn_area_id, mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id);
		  OAILOG_FUNC_RETURN (LOG_MCE_APP, RETURNerror);
	  }
  }
  /*
   * Updating statistics
   */
  __sync_fetch_and_add (&mce_mbsfn_area_contexts_p->nb_mbsfn_area_managed, 1);
  __sync_fetch_and_add (&mce_mbsfn_area_contexts_p->nb_mbsfn_are_since_last_stat, 1);
  OAILOG_FUNC_RETURN (LOG_MCE_APP, RETURNok);
}

//------------------------------------------------------------------------------
static
void clear_mbsfn_area(mbsfn_area_context_t * const mbsfn_area_context) {
	OAILOG_FUNC_IN (LOG_MCE_APP);
	mbsfn_area_id_t mbsfn_area_id = mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id;
	mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id = INVALID_MBSFN_AREA_ID;
	memset(&mbsfn_area_context->privates.fields, 0, sizeof(mbsfn_area_context->privates.fields));
	/** Clear the M2 eNB hashmap. */
	hashtable_uint64_ts_destroy(mbsfn_area_context->privates.m2_enb_id_hashmap);
	/** Clear the MBMS Service Area Id hashmap. */
	hashtable_ts_destroy(mbsfn_area_context->privates.mbms_service_idx_mcch_modification_times_hashmap);
	OAILOG_INFO(LOG_MME_APP, "Successfully cleared MBSFN area "MME_UE_S1AP_ID_FMT ". \n", mbsfn_area_id);
	OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
static
bool mce_app_create_mbsfn_area(const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, const uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id) {
	OAILOG_FUNC_IN(LOG_MCE_APP);

	mbsfn_area_context_t 									* mbsfn_area_context = NULL;
	// todo: check the lock mechanism
	if(!pthread_rwlock_trywrlock(&mce_app_desc.rw_lock)) {
		/** Try to get a free MBMS Service context. */
		mbsfn_area_context = STAILQ_FIRST(&mce_app_desc.mce_mbsfn_area_contexts_list);
		DevAssert(mbsfn_area_context); /**< todo: with locks, it should be guaranteed, that this should exist. */
		if(mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id != INVALID_MBSFN_AREA_ID){
			OAILOG_ERROR(LOG_MCE_APP, "No free MBSFN area left. Cannot allocate a new one.\n");
			pthread_rwlock_unlock(&mce_app_desc.rw_lock);
			OAILOG_FUNC_RETURN (LOG_MCE_APP, false);
		}
		/** Found a free MBSFN Area: Remove it from the head, add the MBSFN area id and set it to the end. */
		STAILQ_REMOVE_HEAD(&mce_app_desc.mce_mbsfn_area_contexts_list, entries); /**< free_mbsfn is removed. */
		/** Remove the EMS-EBR context of the bearer-context. */
		OAILOG_INFO(LOG_MCE_APP, "Clearing received current MBSFN area %p.\n", mbsfn_area_context);
		clear_mbsfn_area(mbsfn_area_context); /**< Stop all timers and clear all fields. */

		mme_config_read_lock(&mme_config);
		/**
		 * Initialize the M2 eNB Id bitmap.
		 * We wan't to make sure, to keep the MBSFN structure size unchanged, no memory overwrites.
		 * todo: optimization possible with hashFunc..
		 */
		mbsfn_area_context->privates.m2_enb_id_hashmap = hashtable_uint64_ts_create((hash_size_t)mme_config.mbms.max_m2_enbs, NULL, NULL);
		/**
		 * Initialize the MBMS services hashmap.
		 * We need a list of MBMS Service Id, each one may have a different MCCH modification period.
		 */
		mbsfn_area_context->privates.mbms_service_idx_mcch_modification_times_hashmap = hashtable_ts_create((hash_size_t)mme_config.mbms.max_mbms_services, NULL, hash_free_func, NULL);
		mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id			  = mbsfn_area_id;
		/** A single MBMS area per MBSFN area. */
		mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id = mbms_service_area_id;

		/** Set the CSA period to a fixed RF128, to calculate the resources based on a 1sec base. */
		mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_csa_period_rf  = get_csa_period_rf(CSA_PERIOD_RF128);

		/**
		 * Set the MCCH configurations from the MmeCfg file.
		 * MME config is already locked.
		 */
		mbsfn_area_context->privates.fields.mbsfn_area.mcch_modif_period_rf 			= mme_config.mbms.mbms_mcch_modification_period_rf;
		mbsfn_area_context->privates.fields.mbsfn_area.mcch_repetition_period_rf  = mme_config.mbms.mbms_mcch_repetition_period_rf;
		mbsfn_area_context->privates.fields.mbsfn_area.mbms_mcch_msi_mcs 					= mme_config.mbms.mbms_mcch_msi_mcs;
		mbsfn_area_context->privates.fields.mbsfn_area.mch_mcs_enb_factor			 	  = mme_config.mbms.mch_mcs_enb_factor;
		mbsfn_area_context->privates.fields.mbsfn_area.mbms_sf_slots_half					= mme_config.mbms.mbms_subframe_slot_half;

		// todo: a function which checks multiple MBSFN areas for MCCH offset
		// todo: MCCH offset is calculated in the MCH resources?
		mbsfn_area_context->privates.fields.mbsfn_area.mcch_offset_rf			 				= 0;
		/** Indicate the MCCH subframes. */
		// todo: a function, depending on the number of total MBSFN areas, repetition/modification periods..
		// todo: calculate at least for 1 MCH of 1 MCCH the MCCH allocation, eNB can allocate dynamically for the correct subframe?
		mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_band = mme_config.mbms.mbms_m2_enb_band;
		mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_bw	 = mme_config.mbms.mbms_m2_enb_bw;
		/** Set the M2 eNB Id. Nothing else needs to be done for the MCS part. */
		hashtable_uint64_ts_insert(mbsfn_area_context->privates.m2_enb_id_hashmap, (hash_key_t)m2_enb_id, NULL);
		/** Add the MBSFN area to the back of the list. */
		STAILQ_INSERT_TAIL(&mce_app_desc.mce_mbsfn_area_contexts_list, mbsfn_area_context, entries);
		/** Add the MBSFN area into the MBMS service Hash Map. */
		DevAssert (mce_insert_mbsfn_area_context(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_context) == 0);
		pthread_rwlock_unlock(&mce_app_desc.rw_lock);
		mme_config_unlock(&mme_config);
		OAILOG_FUNC_RETURN (LOG_MME_APP, true);
	}
	OAILOG_FUNC_RETURN (LOG_MME_APP, false);
}

//------------------------------------------------------------------------------
static
void mce_update_mbsfn_area(struct mbsfn_areas_s * const mbsfn_areas, const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id) {
	OAILOG_FUNC_IN(LOG_MCE_APP);
	mbsfn_area_context_t * mbsfn_area_context = NULL;
	/**
	 * Updated the response (MBSFN areas). Check if the MBSFN areas are existing or not.
	 */
	if(mce_app_update_mbsfn_area(mbsfn_area_id, mbms_service_area_id, m2_enb_id, assoc_id)){
		/**
		 * Successfully updated existing MBMS Service Area with eNB information.
		 * Set it in the MBSFN response!
		 */
		mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_id);
		memcpy((void*)&mbsfn_areas->mbsfn_area_cfg[mbsfn_areas->num_mbsfn_areas++].mbsfnArea, (void*)&mbsfn_area_context->privates.fields.mbsfn_area, sizeof(mbsfn_area_t));
		OAILOG_INFO(LOG_MCE_APP, "MBSFN Area " MBSFN_AREA_ID_FMT " is already active. Successfully updated for MBMS_SAI " MBMS_SERVICE_AREA_ID_FMT " with eNB information (m2_enb_id=%d, sctp_assoc=%d).\n",
				mbsfn_area_id, mbms_service_area_id, m2_enb_id, assoc_id);
		OAILOG_FUNC_OUT(LOG_MCE_APP);
	} else {
		/** Could not update. Check if it needs to be created. */
		if(mce_app_create_mbsfn_area(mbsfn_area_id, mbms_service_area_id, m2_enb_id, assoc_id)){
			mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_id);
			memcpy((void*)&mbsfn_areas->mbsfn_area_cfg[mbsfn_areas->num_mbsfn_areas++].mbsfnArea, (void*)&mbsfn_area_context->privates.fields.mbsfn_area, sizeof(mbsfn_area_t));
			/**
			 * Intelligently, amongst your MBSFN areas set the MCCH subframes.
			 */
			//		if(get_enb_type(mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_band) == ENB_TYPE_NULL){
						mbsfn_area_context->privates.fields.mbsfn_area.mbms_mcch_subframes				= 0b10001; // todo: just temporary..
			//		} else if(get_enb_type(mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_band) == ENB_TYPE_TDD) {
			//			mbsfn_area_context->privates.fields.mbsfn_area.mbms_mcch_subframes				= 0b000001;
			//		}
			OAILOG_INFO(LOG_MCE_APP, "Created new MBSFN Area " MBSFN_AREA_ID_FMT " for MBMS_SAI " MBMS_SERVICE_AREA_ID_FMT " with eNB information (m2_enb_id=%d, sctp_assoc=%d).\n",
					mbsfn_area_id, mbms_service_area_id, m2_enb_id, assoc_id);
			OAILOG_FUNC_OUT(LOG_MCE_APP);
		}
	}
	OAILOG_ERROR(LOG_MCE_APP, "MBSFN Area " MBSFN_AREA_ID_FMT " for MBMS_SAI " MBMS_SERVICE_AREA_ID_FMT " with eNB information (m2_enb_id=%d, sctp_assoc=%d) could neither be created nor updated. Skipping..\n",
			mbsfn_area_id, mbms_service_area_id, m2_enb_id, assoc_id);
	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

/**
 * Set the subframes in an empty single frame RF pattern, with the given CSA repetition period.
 */
//------------------------------------------------------------------------------
static
void mce_app_set_fresh_radio_frames(struct csa_pattern_s * csa_pattern_mbsfn, struct mchs_s * mchs)
{
	/** No matter if FDD or TDD, we will try to fit 6 subframes into 1RF. */
	for(int num_sf = 1; num_sf <= (CSA_SF_SINGLE_FRAME * csa_pattern_mbsfn->mbms_csa_pattern_rfs); num_sf++ ){ /**< 6 or 24. */
		/** Set the subframe in the CSA allocation pattern. */
		csa_pattern_mbsfn->csa_pattern_sf.mbms_mch_csa_pattern_1rf |= (0x01 << ((CSA_SF_SINGLE_FRAME* csa_pattern_mbsfn->mbms_csa_pattern_rfs)-num_sf)); /**< 5 to 0. */
		/** Reduced the number of SFs, multiplied by the CSA pattern repetition period. */
		if((MBMS_CSA_PERIOD_GCS_AS_RF / csa_pattern_mbsfn->csa_pattern_repetition_period_rf) > mchs->total_subframes_per_csa_period_necessary)
			mchs->total_subframes_per_csa_period_necessary -= (MBMS_CSA_PERIOD_GCS_AS_RF / csa_pattern_mbsfn->csa_pattern_repetition_period_rf);
		else
			mchs->total_subframes_per_csa_period_necessary = 0;
		if(!mchs->total_subframes_per_csa_period_necessary){
			OAILOG_DEBUG(LOG_MCE_APP,"No more subframes to schedule in (%d)RF CSA pattern.", csa_pattern_mbsfn->mbms_csa_pattern_rfs);
			break;
		}
	}
}

//------------------------------------------------------------------------------
static
void mce_app_log_method_single_rf_csa_pattern(struct csa_patterns_s * new_csa_patterns,
		int num_radio_frames,
		struct mchs_s * mchs,
		struct csa_patterns_s * csa_patterns_allocated)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	int power2 											 = 0;
	int radio_frames_alloced 				 = 0;
	int num_csa_patterns_allocated 	 = 0;

	/**
	 * Calculate the CSA pattern offset from other MBSFN areas and the current MBSFN area.
	 */
	int csa_pattern_offset         	 = 0;

	/**
	 * Check if a 4RF pattern has been allocated (max 1 possible).
	 */
	if(new_csa_patterns->total_csa_pattern_offset){
		num_csa_patterns_allocated++;
		csa_pattern_offset = 4;
	}

	/** Check the other MBSFN areas. Increase the radio frame offset. */
	for (; csa_patterns_allocated->total_csa_pattern_offset; csa_pattern_offset++)
	{
		csa_patterns_allocated->total_csa_pattern_offset &= (csa_patterns_allocated->total_csa_pattern_offset-1);
	}
	OAILOG_DEBUG(LOG_MCE_APP, "Calculating 1RF CSA pattern with already set offset (%d). \n", csa_pattern_offset);

	/**
	 * Check each power of 2. Calculate a CSA pattern for each with a different offset and a period (start with the most frequest period).
	 * We may not use the last CSA pattern.
	 */
	while(num_radio_frames){
		/**
		 * Determines the periodicity of CSA patterns..
		 * We start with the highest possible periodicity.
		 * For each single frame, we increase the used CSA pattern offset by once, no matter what the periodicity is.
		 */
		DevAssert((power2 = floor(log2(num_radio_frames))));
		radio_frames_alloced = pow(2, power2);
		/**
		 * Next we will calculate a single pattern for each modulus. We then will increase the new_csa_patterns total_csa_offset bitmap,
		 * make union, with the already allocated one and check if free offsets are left.
		 */
		if(new_csa_patterns->total_csa_pattern_offset | csa_patterns_allocated->total_csa_pattern_offset == 0xFF){
			OAILOG_ERROR(LOG_MCE_APP, "No more CSA patterns left to allocate resources for MBSFN Area.\n");
			OAILOG_FUNC_OUT(LOG_MCE_APP);
		}
		/**
		 * Calculate the number of radio frames, that can scheduled in a single RF CSA pattern in this periodicity.
		 * Consider the CSA pattern with the given CSA offset.
		 */
	  new_csa_patterns->csa_pattern[num_csa_patterns_allocated].mbms_csa_pattern_rfs 							= CSA_ONE_FRAME;
	  new_csa_patterns->csa_pattern[num_csa_patterns_allocated].csa_pattern_repetition_period_rf	= get_csa_rf_alloc_period_rf(CSA_RF_ALLOC_PERIOD_RF32) / (radio_frames_alloced / 4);
	  mce_app_set_fresh_radio_frames(&new_csa_patterns->csa_pattern[num_csa_patterns_allocated], mchs);
	  /** Increase the CSA pattern offset. Check if the last radio frame (CSA_COMMON) is reached. */
	  csa_pattern_offset++;
	  /**
	   * Set the total_csa_pattern offset with the newly set CSA pattern.
	   * We will allocate radio frames continuously, except the last radio frame.
	   * So the sum of the bitmap of totally set radio frames will also indicate, which radio frame we are at (CONSIDER CSA_COMMON!).
	   */
	  new_csa_patterns->total_csa_pattern_offset |= (0x01 << (8 - (csa_pattern_offset)));
	  /**
	   * We set the allocated subframes 1RF CSA pattern and reduced the number of remaining subframes left for scheduling.
	   * We allocated some MBSFN radio frames starting with the highest priority. Reduce the number of remaining MBSFN radio frames.
	   */
	  num_radio_frames -= radio_frames_alloced;
	}
	/** Successfully scheduled all radio frames! */
	OAILOG_INFO(LOG_MCE_APP, "Successfully scheduled all radio subframes for the MCHs into the CSA patterns. Current number of CSA offset is (%d). \n", num_total_csa_pattern_offset);
	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

//------------------------------------------------------------------------------
static
void mce_app_calculate_mbsfn_mchs(struct mbsfn_area_context_s * mbsfn_area_context, mbms_services_t * const mbms_services_active, mchs_t *const mchs) {
	int 									 total_duration_in_ms 						= 0;
	bitrate_t 						 pmch_total_available_br_per_sf 	= 0;

	total_duration_in_ms = mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_csa_period_rf * 10;
	/**
	 * No hash callback, just iterate over the active MBMS services.
	 */
	for(int num_mbms_service = 0; num_mbms_service < mbms_services_active->num_mbms_service; num_mbms_service++) {
		/**
		 * Get the MBSFN area.
		 * Calculate the resources based on the active eNBs in the MBSFN area.
		 */
		qci_e qci = mbms_services_active->mbms_service_array[num_mbms_service]->privates.fields.mbms_bc.eps_bearer_context.bearer_level_qos.qci;
		// todo: Current all 15 QCIs fit!! todo --> later it might not!
		qci_ordinal_e qci_ord = get_qci_ord(qci);
		mch_t mch = mchs->mch_array[qci_ord];
		if(!mch.mch_qci){
			DevAssert(!mch.total_gbr_dl_bps);
			mch.mch_qci = qci;
		}
		/** Calculate per MCH the total bandwidth (bits per seconds // multiplied by 1000 @sm decoding). */
		mch.total_gbr_dl_bps += mbms_services_active->mbms_service_array[num_mbms_service]->privates.fields.mbms_bc.eps_bearer_context.bearer_level_qos.gbr.br_dl;
		/** Add the TMGI. */
		memcpy((void*)&mch.mbms_session_list.tmgis[mch.mbms_session_list.num_mbms_sessions++], (void*)&mbms_services_active->mbms_service_array[num_mbms_service]->privates.fields.tmgi, sizeof(tmgi_t));
	}

	/**
	 * The CSA period is set as 1 second (RF128). The minimum time of a service duration is set to the MCCH modification period!
	 * MSP will be set to the half length of the CSA period for now. Should be enough!
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
			int mcs = mce_app_get_mch_mcs(mbsfn_area_context, mch.mch_qci);
			if(mcs == -1){
				DevMessage("Error while calculating MCS for MBSFN Area " + mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id + " and QCI " + qci);
			}
			/** Calculate the necessary transport blocks. */
			int itbs = get_itbs(mcs);
			if(itbs == -1){
				DevMessage("Error while calculating TBS index for MBSFN Area " + mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id + " for MCS " + mcs);
			}
			/**
			 * We assume a single antenna port and just one Transport Block per subframe.
			 * No MIMO is expected.
			 * Number of bits transmitted per 1ms (1028)ms.
			 * Subframes, allocated for this MCH gives us the following capacity.
			 * No MCH subframe-interleaving is forseen. So each MCH will have own subframes. Calculate the capacity of the subframes.
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
}

//------------------------------------------------------------------------------
static
void mce_app_calculate_csa_common_pattern(struct csa_patterns_s * csa_patterns_p, mchs_t * mchs, uint8_t num_mbsfn_idx, uint8_t total_mbsfn_areas_tbs){
  /**
   * Depending on how many MBSFN areas are to be scheduled (i), set the ith subframe to the MBSFN area.
   * Check that there should be at least one MBSFN area to be scheduled.
   */
	csa_patterns_p->csa_pattern[COMMON_CSA_PATTERN].csa_pattern_offset_rf = COMMON_CSA_PATTERN;
	csa_patterns_p->csa_pattern[COMMON_CSA_PATTERN].mbms_csa_pattern_rfs = CSA_ONE_FRAME;
	csa_patterns_p->csa_pattern[COMMON_CSA_PATTERN].csa_pattern_repetition_period_rf = get_csa_rf_alloc_period_rf(CSA_RF_ALLOC_PERIOD_RF32); /**< One Frame CSA pattern only occurs in every 32RFs. Shared by all MBSFN areas. */
	/**
	 * Set the bits depending on the number of the total MBSFN areas.
	 * Set all bits to the MBSFN area if the total num is 1.
	 * Else, just set the ith bit.
	 */

	// TODO FDD/TDD specific setting with UL/DL percentages..

	int csa_bits_set 	= (6/total_mbsfn_areas_tbs); 		   /**< 0x06, 0x03, 0x02. */
	int csa_offset 		= num_mbsfn_idx * csa_bits_set;    /**< Also indicates the # of current MBSFN area. */
	for(int i_bit = 0; i_bit < csa_bits_set; i_bit++){
		/** Set the bits of the last CSA. */
		csa_patterns_p->csa_pattern[COMMON_CSA_PATTERN].csa_pattern_sf.mbms_mch_csa_pattern_1rf = ((0x01 << (csa_offset + i_bit)));
	}
	// todo: set the last subframe of the MCH as the end-1 repetition period of the CSA pattern!
	/** Amount of subframes that can be allocated in the CSA[COMMON_CSA_PATTERN]. */
	int num_final_csa_data_sf = csa_bits_set * ((MBMS_CSA_PERIOD_GCS_AS_RF / csa_patterns_p->csa_pattern[COMMON_CSA_PATTERN].csa_pattern_repetition_period_rf) -1);
	OAILOG_DEBUG(LOG_MCE_APP, "Set %d SFs in COMMON_CSA for MBSFN area. Removing from total #sf %d.", num_final_csa_data_sf, mchs->total_subframes_per_csa_period_necessary);
	if(mchs->total_subframes_per_csa_period_necessary >= num_final_csa_data_sf)
		mchs->total_subframes_per_csa_period_necessary -= num_final_csa_data_sf;
	else
		mchs->total_subframes_per_csa_period_necessary = 0;
}

//------------------------------------------------------------------------------
static
void mce_app_reuse_csa_pattern_set_subframes_fdd(struct csa_pattern_s * csa_pattern_mbsfn, struct csa_pattern_s * csa_pattern, int *mch_subframes_to_be_scheduled_p){
	OAILOG_FUNC_IN(LOG_MCE_APP);

	// todo : TDD combinations
	/** Check any subframes are left: Count the bits in each octet. */
	uint8_t csa_pattern_4rf_ = 0;
	uint8_t num_bits_checked = 0;
	uint8_t num_subframes_per_csa_pattern = 0;
	uint8_t num_bits_in_csa_pattern = (8 * csa_pattern->mbms_csa_pattern_rfs);
	while(num_bits_checked < num_bits_in_csa_pattern){
		csa_pattern_4rf_ = (csa_pattern->csa_pattern_sf.mbms_mch_csa_pattern_4rf >> num_bits_checked) & 0x3F; /**< Last one should be 0. */
		num_bits_checked+=8;
		if(csa_pattern_4rf_ != 0x3F){
			/**
			 * Pattern not filled fully.
			 * Check how many subframes are set.
			 */
			/** Count the number of set MBSFN subframes, no matter if 1 or 4 RFs. */
			for (; csa_pattern_4rf_; num_subframes_per_csa_pattern++)
			{
				csa_pattern_4rf_ &= (csa_pattern_4rf_-1);
			}
			/** Assert that the remaining subframes are zero! We wan't to set them in order without gaps!. */
			DevAssert(!(csa_pattern->csa_pattern_sf.mbms_mch_csa_pattern_4rf >> num_bits_checked)); /**< Does not need to be in bounds. */
			break;
		}
	}
	if(num_bits_checked >= num_bits_in_csa_pattern){
		OAILOG_DEBUG(LOG_MCE_APP, "4RF-CSA pattern has no free subframes left. Checking the other CSA patterns. \n");
		OAILOG_FUNC_OUT(LOG_MCE_APP);
	}
	DevAssert(CSA_SF_SINGLE_FRAME-num_subframes_per_csa_pattern);

	/**
	 * Copy the offset, repetition period and type.
	 * */
	csa_pattern_mbsfn->csa_pattern_offset_rf = csa_pattern->csa_pattern_offset_rf;
	csa_pattern_mbsfn->csa_pattern_repetition_period_rf = csa_pattern->csa_pattern_repetition_period_rf;
	csa_pattern_mbsfn->mbms_csa_pattern_rfs = csa_pattern->mbms_csa_pattern_rfs;
	/** Derive the SF allocation from the existing CSA pattern. */
	int num_sf_free = (MBMS_CSA_PERIOD_GCS_AS_RF / csa_pattern->csa_pattern_repetition_period_rf) * (CSA_SF_SINGLE_FRAME-num_subframes_per_csa_pattern)
			+ ((num_bits_in_csa_pattern - num_bits_checked) * CSA_SF_SINGLE_FRAME);
//	OAILOG_DEBUG(LOG_MCE_APP, "Found (%d) empty subframes in (%d)-RF CSA with offset (%d) and repetition period (%d)RF. \n",
//			num_sf_free, csa_pattern->mbms_csa_pattern_rfs, csa_pattern->csa_pattern_offset_rf, csa_pattern->csa_pattern_repetition_rf);

	/** Remove from the total MCH subframes, the part-pattern free subframes. */
	uint8_t csa_pattern_to_be_set = csa_pattern_mbsfn->csa_pattern_sf.mbms_mch_csa_pattern_4rf >> (num_bits_checked - 8);
	for(int num_sf = 0; num_sf < (CSA_SF_SINGLE_FRAME-num_subframes_per_csa_pattern); num_sf++ ){
		/** Reduced the number of SFs. */
		*mch_subframes_to_be_scheduled_p -= (MBMS_CSA_PERIOD_GCS_AS_RF / csa_pattern->csa_pattern_repetition_period_rf);
		/** Set the subframe in the CSA allocation pattern. */
		csa_pattern_to_be_set |= (0x01 << ((CSA_SF_SINGLE_FRAME-num_subframes_per_csa_pattern) -1));
		if(!*mch_subframes_to_be_scheduled_p){
//			OAILOG_INFO(LOG_MCE_APP, "Completely set all SFs of the MCH in previously allocated CSA pattern. \n",
//					num_sf_free, csa_pattern->csa_pattern_offset_rf, csa_pattern->csa_pattern_repetition_rf);
			break;
		}
	}
	if(!*mch_subframes_to_be_scheduled_p){
		OAILOG_INFO(LOG_MCE_APP, "Stuffed all bits of the MBSFN CSA pattern into the first found CSA octet. \n");
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
	}

	while((num_bits_checked + 8) > num_bits_in_csa_pattern && *mch_subframes_to_be_scheduled_p){
		/** We fitted into the last pattern element. Continue. */
		int sf = (int)(ceil(*mch_subframes_to_be_scheduled_p / (MBMS_CSA_PERIOD_GCS_AS_RF / csa_pattern->csa_pattern_repetition_period_rf))) % (CSA_SF_SINGLE_FRAME + 1);
		// todo: devAssert(0x3F3F3F3F);
		// todo: continue or so..
		int i_sf = 1;
		while(sf) { /**< Starting from the left, set the bits.*/
			csa_pattern_mbsfn->csa_pattern_sf.mbms_mch_csa_pattern_4rf |= (0x01 << (CSA_SF_SINGLE_FRAME - i_sf)) << num_bits_checked;
			sf--;
			*mch_subframes_to_be_scheduled_p-=sf;
		}
		num_bits_checked+=8;
	}
	if(!*mch_subframes_to_be_scheduled_p){
		OAILOG_INFO(LOG_MCE_APP, "Stuffed all bits of the MBSFN CSA pattern into the remaining CSA octets. \n");
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
	}
//	OAILOG_INFO(LOG_MCE_APP, "Iterated through the CSA Pattern with offset (%d) and repetition period. (%d) subframes are remaining. "
//			"Checking the remaning subframe patterns. \n", csa_pattern->csa_pattern_offset_rf, csa_pattern_repetition_rf, *mch_subframes_to_be_scheduled_p);
	/**
	 * Set the remaining subframes of the partial frame as set!.
	 * Remaining subframes should also be empty, if any, exist, allocate as much as possible.
	 */
}

//------------------------------------------------------------------------------
static
void mce_app_reuse_csa_pattern_set_subframes_tdd(struct csa_pattern_s * csa_pattern_mbsfn, struct csa_pattern_s * csa_pattern, int num_subframes_in_tdd_rf, int *mch_subframes_to_be_scheduled_p){
	OAILOG_FUNC_IN(LOG_MCE_APP);

	// todo : TDD combinations
	/** Check any subframes are left: Count the bits in each octet. */
	uint8_t csa_pattern_4rf_ = 0;
	uint8_t num_bits_checked = 0;
	uint8_t num_subframes_per_csa_pattern = 0;
	uint8_t num_bits_in_csa_pattern = (8 * csa_pattern->mbms_csa_pattern_rfs);
	while(num_bits_checked < num_bits_in_csa_pattern){
		csa_pattern_4rf_ = (csa_pattern->csa_pattern_sf.mbms_mch_csa_pattern_4rf >> num_bits_checked) & 0x3F; /**< Last one should be 0. */
		num_bits_checked+=8;
		if(csa_pattern_4rf_ != 0x3F){
			/**
			 * Pattern not filled fully.
			 * Check how many subframes are set.
			 */
			/** Count the number of set MBSFN subframes, no matter if 1 or 4 RFs. */
			for (; csa_pattern_4rf_; num_subframes_per_csa_pattern++)
			{
				csa_pattern_4rf_ &= (csa_pattern_4rf_-1);
			}
			/** Assert that the remaining subframes are zero! We wan't to set them in order without gaps!. */
			DevAssert(!(csa_pattern->csa_pattern_sf.mbms_mch_csa_pattern_4rf >> num_bits_checked)); /**< Does not need to be in bounds. */
			break;
		}
	}
	if(num_bits_checked >= num_bits_in_csa_pattern){
		OAILOG_DEBUG(LOG_MCE_APP, "4RF-CSA pattern has no free subframes left. Checking the other CSA patterns. \n");
		OAILOG_FUNC_OUT(LOG_MCE_APP);
	}
	DevAssert(CSA_SF_SINGLE_FRAME-num_subframes_per_csa_pattern);

	/**
	 * Copy the offset, repetition period and type.
	 * */
	csa_pattern_mbsfn->csa_pattern_offset_rf = csa_pattern->csa_pattern_offset_rf;
	csa_pattern_mbsfn->csa_pattern_repetition_period_rf = csa_pattern->csa_pattern_repetition_period_rf;
	csa_pattern_mbsfn->mbms_csa_pattern_rfs = csa_pattern->mbms_csa_pattern_rfs;
	/** Derive the SF allocation from the existing CSA pattern. */
	int num_sf_free = (MBMS_CSA_PERIOD_GCS_AS_RF / csa_pattern->csa_pattern_repetition_period_rf) * (num_subframes_in_tdd_rf - num_subframes_per_csa_pattern)
			+ ((num_bits_in_csa_pattern - num_bits_checked) * num_subframes_in_tdd_rf);
//	OAILOG_DEBUG(LOG_MCE_APP, "Found (%d) empty subframes in (%d)-RF CSA with offset (%d) and repetition period (%d)RF. \n",
//			num_sf_free, csa_pattern->mbms_csa_pattern_rfs, csa_pattern->csa_pattern_offset_rf, csa_pattern->csa_pattern_repetition_rf);

	/** Remove from the total MCH subframes, the part-pattern free subframes. */
	uint8_t csa_pattern_to_be_set = csa_pattern_mbsfn->csa_pattern_sf.mbms_mch_csa_pattern_4rf >> (num_bits_checked - 8);
	for(int num_sf = 0; num_sf < (CSA_SF_SINGLE_FRAME-num_subframes_per_csa_pattern); num_sf++ ){
		/** Reduced the number of SFs. */
		*mch_subframes_to_be_scheduled_p -= (MBMS_CSA_PERIOD_GCS_AS_RF / csa_pattern->csa_pattern_repetition_period_rf);
		/** Set the subframe in the CSA allocation pattern. */
		csa_pattern_to_be_set |= (0x01 << ((num_subframes_in_tdd_rf-num_subframes_per_csa_pattern) -1));
		if(!*mch_subframes_to_be_scheduled_p){
//			OAILOG_INFO(LOG_MCE_APP, "Completely set all SFs of the MCH in previously allocated CSA pattern. \n",
//					num_sf_free, csa_pattern->csa_pattern_offset_rf, csa_pattern->csa_pattern_repetition_rf);
			break;
		}
	}
	if(!*mch_subframes_to_be_scheduled_p){
		OAILOG_INFO(LOG_MCE_APP, "Stuffed all bits of the MBSFN CSA pattern into the first found CSA octet. \n");
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
	}

	while((num_bits_checked + 8) > num_bits_in_csa_pattern && *mch_subframes_to_be_scheduled_p){
		/** We fitted into the last pattern element. Continue. */
		int sf = (int)(ceil(*mch_subframes_to_be_scheduled_p / (MBMS_CSA_PERIOD_GCS_AS_RF / csa_pattern->csa_pattern_repetition_period_rf))) % (num_subframes_in_tdd_rf + 1);
		// todo: devAssert(0x3F3F3F3F);
		// todo: continue or so..
		int i_sf = 1;
		while(sf) { /**< Starting from the left, set the bits.*/
			csa_pattern_mbsfn->csa_pattern_sf.mbms_mch_csa_pattern_4rf |= (0x01 << (num_subframes_in_tdd_rf - i_sf)) << num_bits_checked;
			sf--;
			*mch_subframes_to_be_scheduled_p-=sf;
		}
		num_bits_checked+=8;
	}
	if(!*mch_subframes_to_be_scheduled_p){
		OAILOG_INFO(LOG_MCE_APP, "Stuffed all bits of the MBSFN CSA pattern into the remaining CSA octets. \n");
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
	}
//	OAILOG_INFO(LOG_MCE_APP, "Iterated through the CSA Pattern with offset (%d) and repetition period. (%d) subframes are remaining. "
//			"Checking the remaning subframe patterns. \n", csa_pattern->csa_pattern_offset_rf, csa_pattern_repetition_rf, *mch_subframes_to_be_scheduled_p);
	/**
	 * Set the remaining subframes of the partial frame as set!.
	 * Remaining subframes should also be empty, if any, exist, allocate as much as possible.
	 */
}

/**
 * We cannot enter this method one by one for each MBSFN area.
 * @param: csa_patterns_alloced: should be a union of the CSA patterns of all previously scheduled MBSFN areas.
 */
//------------------------------------------------------------------------------
static
void mce_app_reuse_csa_pattern(struct csa_patterns_s * csa_patterns_mbsfn_p, mchs_t * mchs, const struct csa_patterns_s * const csa_patterns_alloced){
	OAILOG_FUNC_IN(LOG_MCE_APP);
	/**
	 * Iterate the CSA patterns, till the COMMON_CSA pattern.
	 * Check for any available 4RF and 1RF CSA patterns.
	 * Start with the lowest repetition factor (4). Move up to 16.
	 */
	for(csa_frame_num_e csa_frame_num = CSA_FOUR_FRAME; csa_frame_num*=4; csa_frame_num <= 4) {
		for(csa_rf_alloc_period_e num_csa_repetition = CSA_RF_ALLOC_PERIOD_RF32; num_csa_repetition >= CSA_PERIOD_RF8; num_csa_repetition--){ /**< We use 32, 16, 8. */
			/** The index is always absolute and not necessarily equal to the CSA offset. */
			for(int num_csa_pattern = 0; num_csa_pattern < COMMON_CSA_PATTERN; num_csa_pattern++){
				/** Check if 4RF. */
				int csa_pattern_repetition_rf = get_csa_rf_alloc_period_rf(num_csa_repetition);
				struct csa_pattern_s * csa_pattern = &csa_patterns_alloced->csa_pattern[num_csa_pattern];
				if(csa_pattern->mbms_csa_pattern_rfs == csa_frame_num && csa_pattern->csa_pattern_repetition_period_rf == csa_pattern_repetition_rf){
					struct csa_pattern_s * csa_pattern_mbsfn = &csa_patterns_mbsfn_p->csa_pattern[num_csa_pattern];
					//
					//	// TODO FDD/TDD specific setting with UL/DL percentages..
					//
					mce_app_reuse_csa_pattern_set_subframes_fdd(csa_pattern_mbsfn, csa_pattern, &mchs->total_subframes_per_csa_period_necessary);
					/** No return expected, just check if all subframes where scheduled. */
					if(!mchs->total_subframes_per_csa_period_necessary){
						/**
						 * No more MCH subframes to be scheduled.
						 * No further CSA offsets need to be defined, we can re-use the existing. */
						OAILOG_INFO(LOG_MCE_APP, "Fitted %d newly received MCHs into existing CSA patterns. \n", mchs->num_mch);
						OAILOG_FUNC_OUT(LOG_MCE_APP);
					} else {
						OAILOG_INFO(LOG_MCE_APP, "After checking CSA (%d)RF-pattern with offset %d and period (%d), "
								"still %d subframes exist for (%d) MCHs. \n", csa_frame_num, csa_pattern->csa_pattern_offset_rf,
								csa_pattern->csa_pattern_repetition_period_rf, mchs->total_subframes_per_csa_period_necessary, mchs->num_mch);
						OAILOG_FUNC_OUT(LOG_MCE_APP);
					}
				}
			}
		}
	}
	OAILOG_INFO(LOG_MCE_APP, "After all existing CSA patterns, still %d subframes exist for (%d) MCHs. \n", mchs->total_subframes_per_csa_period_necessary, mchs->num_mch);
	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

//------------------------------------------------------------------------------
static void mce_app_allocate_4frame(struct csa_patterns_s * new_csa_patterns, int num_radio_frames, struct mchs_s * mchs, struct csa_patterns_s * csa_patterns_allocated){
	OAILOG_FUNC_IN(LOG_MCE_APP);
	// todo: 0.75 is variable, just a threshold where we consider allocating a 4RF pattern. What could be any other reason?
	/** 16 Radio Frames are the minimum radio frames allocated with a /32 periodicity. */
	int csa_4_frame_rfs_repetition = num_radio_frames/(((MBMS_CSA_PERIOD_GCS_AS_RF/get_csa_period_rf(CSA_PERIOD_RF32)) * 16) * 0.75);
	if(!csa_4_frame_rfs_repetition) {
		OAILOG_INFO(LOG_MCE_APP, "Skipping 4RF pattern for MBSFN Area Id " MBSFN_AREA_ID_FMT ". \n.", mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
		OAILOG_FUNC_OUT(LOG_MCE_APP);
	}
	/** 4RF pattern will be allocated. */
	uint8_t total_csa_pattern_offset = 0b11110000; /**< 8 Radio Frames. */
	while (total_csa_pattern_offset & csa_patterns_allocated->total_csa_pattern_offset){
		/** Overlap between the already allocated and the newly allocated CSA pattern. */
		total_csa_pattern_offset = (total_csa_pattern_offset >> 0x01);
		if(total_csa_pattern_offset == 0x0F) {
			OAILOG_ERROR(LOG_MCE_APP, "No more free radio frame offsets available to schedule the MCHs of MBSFN Area Id " MBSFN_AREA_ID_FMT " in a 4RF pattern. We cannot use CSA_COMMON. \n.",
					mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
			OAILOG_FUNC_OUT(LOG_MCE_APP);
		}
	}
	/**
	 * No matter what the 4RF repetition is, it will be allocated in the first common 8 radio frames. So checking it is enough.
	 * Allocate a 4RF radio frame, and then remove it from the necessary subframes to be scheduled.
	 */
	OAILOG_DEBUG(LOG_MCE_APP, "Allocating 4RF CSA Pattern with alloced RFs (%d) and repetition (%d).\n", new_csa_patterns->total_csa_pattern_offset, csa_4_frame_rfs_repetition);
	/**
	 * No looping is necessary. We know the #RFs to be allocated by 4 * csa_pattern_repetition_period.
	 * The CSA pattern will always be the first CSA pattern of the MBSFN area.
	 */
	new_csa_patterns->total_csa_pattern_offset												= total_csa_pattern_offset;
	new_csa_patterns->csa_pattern[0].mbms_csa_pattern_rfs 						= CSA_FOUR_FRAME;
	new_csa_patterns->csa_pattern[0].csa_pattern_repetition_period_rf	= csa_4_frame_rfs_repetition;
	mce_app_set_fresh_radio_frames(&new_csa_patterns->csa_pattern[0], mchs);
	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

//------------------------------------------------------------------------------
static
int mce_app_alloc_csa_pattern(struct csa_patterns_s * new_csa_patterns,
		struct mchs_s * mchs, const struct csa_patterns_s * csa_patterns_allocated,
		const struct mbsfn_area_context_s * mbsfn_area_ctx)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	/** Check if any the union has allocated all CSA pattern offsets. If so, reject. */
	if(csa_patterns_allocated->total_csa_pattern_offset == 0xFF){
		OAILOG_ERROR(LOG_MCE_APP, "All CSA pattern offsets area allocated already. Cannot allocate a new CSA pattern. \n");
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
	}
	/** Check with full allocation how many subframes are needed. */
	int subframe_count = 6;
	if(TDD == get_enb_type(mbsfn_area_ctx->privates.fields.mbsfn_area.m2_enb_band)){
	  /** check the subframe count. */
		subframe_count = get_enb_tdd_subframe_size(mbsfn_area_ctx->privates.fields.mbsfn_area.m2_enb_band);
	}
	/** Check that subframes exist. */
	DevAssert(subframe_count);

	/**< Received number of fresh radio frames, for which a new pattern fill be fully filled. Make it the next multiple of 4. */
	int num_radio_frames = ceil(mchs->total_subframes_per_csa_period_necessary/subframe_count);
	num_radio_frames += (num_radio_frames %4);
	/**
	 * Allocate a 4RF CSA pattern with the given period.
	 * For the remaining RFs calculate a single frame CSA pattern.
	 */
	mce_app_allocate_4frame(new_csa_patterns, num_radio_frames, mchs, mbsfn_area_ctx, csa_patterns_allocated);
	if(!mchs->total_subframes_per_csa_period_necessary){
		OAILOG_INFO(LOG_MCE_APP, "MCHs of MBSFN Area Id "MBSFN_AREA_ID_FMT " are allocated in a 4RF pattern completely.\n", mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
	}
	/** Check if there are any offsets left for the 1RF CSA pattern. */
	OAILOG_INFO(LOG_MCE_APP, "Checking for a new 1RF pattern for MCHs of MBSFN Area Id "MBSFN_AREA_ID_FMT ".\n",
			mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);

	/**
	 * Check the number of 1RF CSA patterns you need (periodicity).
	 * New CSA pattern should be already allocated, inside, compare it with the csa_patterns_allocated.
	 */
	if(mce_app_log_method_single_rf_csa_pattern(new_csa_patterns, num_radio_frames, mchs, csa_patterns_allocated) == RETURNerror){
		OAILOG_ERROR(LOG_MCE_APP, "Error while scheduling the CSA pattern.\n");
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
	}
	OAILOG_INFO(LOG_MCE_APP, "Completed the allocation of 1RF pattern for MBSFN Area Id " MBSFN_AREA_ID_FMT ".\n", mbsfn_area_ctx->privates.fields.mbsfn_area.mbsfn_area_id);
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

/**
 * Check the CSA pattern for this MBSFN!
 * May reuse the CSA patterns of the already allocated MBSFNs (const - unchangeable).
 * We use the total number of MBSFN area to update first CSA[7] where we also leave the last repetition for MCCH subframes.
 * If the MCCH Modification period is 2*CSA_PERIOD (2s), the subframes in the last repetition will not be filled with data,
 * because it would overwrite in the CSA period where the MCCHs occur.
 */
//------------------------------------------------------------------------------
static
int mce_app_calculate_overall_csa_pattern(struct csa_patterns_s * const csa_patterns_p, mchs_t * const mchs, const uint8_t num_mbsfn_idx,
		const uint8_t total_mbsfn_areas_to_be_scheduled, struct csa_patterns_s * const union_csa_patterns_allocated_p, const struct mbsfn_area_context_s * mbsfn_area_ctx) {
	OAILOG_FUNC_IN(LOG_MCE_APP);
	/** Calculate the Common CSA pattern for the MBSFN area and reduce the #SFs left. */
	// todo: Check the update of the overall CSA pattern..
	mce_app_calculate_csa_common_pattern(csa_patterns_p, mchs, num_mbsfn_idx, total_mbsfn_areas_to_be_scheduled);
	if(!mchs->total_subframes_per_csa_period_necessary){
		/**
		 * Very few MBSFN subframes were needed, all fittet into CSA_COMMON.
		 * Total CSA pattern offset is not incremented. We check it always against COMMON_CSA_PATTERN (reserved).
		 */
		OAILOG_INFO(LOG_MCE_APP, "Fitted all data into CSA_COMMON for MBSFN-Area-idx (%d).\n", num_mbsfn_idx);
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
	}

	/**
	 * Update the CSA pattern of the MBSFN area with the remaining subframes. Start checking by the already allocated CSA subframes.
	 * We don't need consecutive allocates of subframes/radio frames between MBSFN areas.
	 * No return value is needed, since we will try to allocate new resources, if MBSFN SFs remain.
	 * The new csa_patterns will be derived from the already allocated csa_patterns in the mbsfn_areas object.
	 * At reuse, we first check 4RFs and shortest period.
	 */
	if(num_mbsfn_idx){
		OAILOG_INFO(LOG_MCE_APP, "Checking previous MBSFN area CSA patterns.\n");
		mce_app_reuse_csa_pattern(csa_patterns_p, mchs, union_csa_patterns_allocated_p);
		if(!mchs->total_subframes_per_csa_period_necessary){
			/**
			 * Total CSA pattern offset is not incremented. We check it always against COMMON_CSA_PATTERN (reserved).
			 */
			OAILOG_INFO(LOG_MCE_APP, "Fitted all data into previously allocated CSA patterns for MBSFN-Area-idx (%d). No need to calculate new CSA patterns.\n", num_mbsfn_idx);
			OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
		}
	}

	/**
	 * Check if a new pattern needs to be allocated.
	 * Check for the repetition period and CSA pattern form that is necessary.
	 * Check if the offset can be fitted?
	 * Allocate the subframes according to FDD/TDD.
	 * Start from 1 RF and longest period (32/16/8) -> then move to 4RF.
	 */
	if(!mce_app_alloc_csa_pattern(csa_patterns_p, union_csa_patterns_allocated_p, mchs, mbsfn_area_ctx)) {
		OAILOG_ERROR(LOG_MCE_APP, "Error generating new necessary CSA patterns for MBSFN-Area-idx (%d). Cannot allocate re sources.\n",
				num_mbsfn_idx);
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
	}
	OAILOG_INFO(LOG_MCE_APP, "Successfully allocated CSA resources for MBSFN-Area-idx (%d).\n", num_mbsfn_idx);
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

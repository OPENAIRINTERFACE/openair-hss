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
bool mce_mbsfn_area_compare_by_mbms_sai (__attribute__((unused)) const hash_key_t keyP,
		void * const elementP,
		void * parameterP, void **resultP);

//------------------------------------------------------------------------------
static
bool mce_mbsfn_area_compare_by_local_mbms_area (__attribute__((unused)) const hash_key_t keyP,
		void * const elementP,
		void * parameterP, void **resultP);

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
int get_mbsfn_mcch_sf(const uint8_t local_mbms_area, const const mbms_service_area_id_t mbms_sai_global);

//------------------------------------------------------------------------------
static
int mce_app_create_mbsfn_area(const mbsfn_area_id_t mbsfn_area_id, const uint8_t local_mbms_area, const mbms_service_area_id_t mbms_service_area_id, const uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id);

//------------------------------------------------------------------------------
static
void mce_update_mbsfn_area(struct mbsfn_areas_s * const mbsfn_areas, uint8_t local_mbms_area, const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id);

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
struct mbsfn_area_context_s                           *
mce_mbsfn_area_exists_mbsfn_area_id(
  mce_mbsfn_area_contexts_t * const mce_mbsfn_areas_p, const mbsfn_area_id_t mbsfn_area_id)
{
  struct mbsfn_area_context_s                    *mbsfn_area_context = NULL;
  hashtable_ts_get (mce_mbsfn_areas_p->mbsfn_area_id_mbsfn_area_htbl, (const hash_key_t)mbsfn_area_id, (void **)&mbsfn_area_context);
  return mbsfn_area_context;
}

//------------------------------------------------------------------------------
void
mce_mbsfn_areas_exists_mbms_service_area_id(
		const mce_mbsfn_area_contexts_t * const mce_mbsfn_areas_p, const mbms_service_area_id_t mbms_service_area_id, struct mbsfn_area_ids_s * mbsfn_area_ids)
{
  hashtable_rc_t              h_rc 					= HASH_TABLE_OK;
	mbms_service_area_id_t     *mbms_sai_p 		= (mbms_service_area_id_t*)&mbms_service_area_id;
	hashtable_ts_apply_callback_on_elements(mce_mbsfn_areas_p->mbsfn_area_id_mbsfn_area_htbl,
			mce_mbsfn_area_compare_by_mbms_sai, (void *)mbms_sai_p, (void**)&mbsfn_area_ids);
}

//------------------------------------------------------------------------------
void
mce_mbsfn_areas_exists_mbms_area(
		const mce_mbsfn_area_contexts_t * const mce_mbsfn_areas_p, const uint8_t mbms_area,
		struct mbsfn_area_ids_s * mbsfn_area_ids)
{
  hashtable_rc_t              h_rc 					= HASH_TABLE_OK;
	hashtable_ts_apply_callback_on_elements(mce_mbsfn_areas_p->mbsfn_area_id_mbsfn_area_htbl,
			mce_mbsfn_area_compare_by_local_mbms_area, (void *)&mbms_area, (void**)&mbsfn_area_ids);
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
  	OAILOG_ERROR(LOG_MCE_APP, "MBMS Service Idx " MBMS_SERVICE_INDEX_FMT " has invalid or expired start/end times. Cannot be registered in MBSFN Area ID "MBSFN_AREA_ID_FMT". \n",
  			mbms_service_index, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
  	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
  }

  /** Check if the MBMS service is already registered in the MBSFN area, if so, just modify the ending. */
	mcch_modification_periods_t * old_mcch_modification_periods = NULL;
	hashtable_ts_get(mbsfn_area_context->privates.mbms_service_idx_mcch_modification_times_hashmap, (hash_key_t)mbms_service_index, (void**)&old_mcch_modification_periods);
	if(old_mcch_modification_periods)
	{
		OAILOG_WARNING(LOG_MCE_APP, "MBMS Service Idx " MBMS_SERVICE_INDEX_FMT " was already registered in MBSFN area " MBSFN_AREA_ID_FMT". "
				"Changing MCCH modification periods (END). \n", mbms_service_index, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
		/** Set the original start time. */
		if(old_mcch_modification_periods->mcch_modif_start_abs_period)
			mcch_modif_periods->mcch_modif_start_abs_period = old_mcch_modification_periods->mcch_modif_start_abs_period;
		/** Remove the values. */
		hashtable_ts_remove(mbsfn_area_context->privates.mbms_service_idx_mcch_modification_times_hashmap, mbms_service_index, (void**)&old_mcch_modification_periods);
	}
	/** Insert the new values. */
	hashtable_rc_t hash_rc = hashtable_ts_insert(mbsfn_area_context->privates.mbms_service_idx_mcch_modification_times_hashmap, mbms_service_index, mcch_modif_periods);
	if(hash_rc != HASH_TABLE_OK)
		free_wrapper(&mcch_modif_periods);
  OAILOG_FUNC_RETURN(LOG_MCE_APP, (HASH_TABLE_OK == hash_rc) ? RETURNok: RETURNerror);
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
	uint8_t										  local_global_areas_allowed 			= 0;

	mme_config_read_lock(&mme_config);
	local_global_areas_allowed = mme_config.mbms.mbms_global_mbsfn_area_per_local_group;
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
			if(!local_global_areas_allowed){
				OAILOG_INFO(LOG_MCE_APP, "Add adding MBSFN area "MBSFN_AREA_ID_FMT " with MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT " in local_mbms_area (%d) into the results.\n",
						mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id,
						mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id,
						mbsfn_area_context->privates.fields.local_mbms_area);
				/** Put the index as the local area --> all will be checked. */
				mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].mbsfn_area_id[mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].num_mbsfn_area_ids] =
						mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id;
				mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].mbms_service_area_id[mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].num_mbsfn_area_ids] =
						mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id;
				mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].num_mbsfn_area_ids++;
			} else {
				OAILOG_INFO(LOG_MCE_APP, "Not adding local MBSFN area "MBSFN_AREA_ID_FMT " with MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT " in local_mbms_area (%d) into the results.\n",
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
		mbsfn_area_ids_p[0].mbsfn_area_id[mbsfn_area_ids_p[0].num_mbsfn_area_ids] =
				mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id;
		mbsfn_area_ids_p[0].mbms_service_area_id[mbsfn_area_ids_p[0].num_mbsfn_area_ids] =
				mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id;
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
		mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].mbsfn_area_id[mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].num_mbsfn_area_ids] =
				mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id;
		mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].mbms_service_area_id[mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].num_mbsfn_area_ids] =
				mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id;
		mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].num_mbsfn_area_ids++;
		return false;
	} else if(!mbsfn_area_context->privates.fields.local_mbms_area){
		/** Check the configuration, only add it, if local-global MBMS service areas are not allowed. */
		if(!local_global_areas_allowed){
			mbsfn_area_ids_p[0].mbsfn_area_id[mbsfn_area_ids_p[0].num_mbsfn_area_ids] =
					mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id;
			mbsfn_area_ids_p[0].mbms_service_area_id[mbsfn_area_ids_p[0].num_mbsfn_area_ids] =
					mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id;
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
		mbms_service_indexes->mbms_service_index_array[mbms_service_indexes->num_mbms_service_indexes++] = mbms_service_idx;
	}
	/** Iterate through the whole list. */
	return false;
}

//------------------------------------------------------------------------------
bool mce_app_get_mbsfn_groups (const hash_key_t keyP,
               void * const mbsfn_context_Ref,
               void __attribute__((unused)) * parameterP,
               void **resultP)
{
	mbsfn_area_id_t			 	       mbsfn_area_id										= (mbsfn_area_id_t)keyP;
	mbsfn_area_context_t				*mbsfn_area_context 							= (mbsfn_area_context_t*)mbsfn_context_Ref;
	mbsfn_area_ids_t					  *mbsfn_area_ids_p									= (mbsfn_area_ids_t*)*resultP;

	/** Without checking the local-global flag, set the MBSFN area into the correct group. */
	mbsfn_area_ids_p[mbsfn_area_context->privates.fields.local_mbms_area].mbsfn_area_id[mbsfn_area_ids_p->num_mbsfn_area_ids++] = mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id;
}

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
			mce_update_mbsfn_area(mbsfn_areas, local_mbms_service_area, mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area], m2_enb_id, assoc_id);
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
  			mce_update_mbsfn_area(mbsfn_areas, local_mbms_service_area, mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area], m2_enb_id, assoc_id);
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
		mce_update_mbsfn_area(mbsfn_areas, 0, mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area], m2_enb_id, assoc_id);
		mme_config_unlock(&mme_config);
	}
	/** Done checking all global MBMS service areas. */
	OAILOG_FUNC_OUT(LOG_MCE_APP);
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
static
bool mce_mbsfn_area_compare_by_local_mbms_area (__attribute__((unused)) const hash_key_t keyP,
                                    void * const elementP,
                                    void * parameterP, void **resultP)
{
  uint8_t 												* local_mbms_area  		= ((uint8_t*)parameterP);
  mbsfn_area_context_t            * mbsfn_area_context  = (mbsfn_area_context_t*)elementP;
  mbsfn_area_ids_t								* mbsfn_area_ids_p		= (mbsfn_area_ids_t*)*resultP;
  /** Set the matched MBSFN area into the list. */
  if (*local_mbms_area == mbsfn_area_context->privates.fields.local_mbms_area) {
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
  	void * result_unused = NULL;
    hashtable_ts_remove(mbsfn_area_ctx->privates.mbms_service_idx_mcch_modification_times_hashmap, *((const hash_key_t*)mbms_service_idx_P), (void**)&result_unused);
  }
  return false;
}

//------------------------------------------------------------------------------
static
bool mce_app_update_mbsfn_area(const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, const uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id) {
	OAILOG_FUNC_IN(LOG_MCE_APP);
	mbsfn_area_context_t 									* mbsfn_area_context = NULL;
	if(pthread_rwlock_trywrlock(&mce_app_desc.rw_lock)) {
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
// TODO: REMOVE THIS --> MBMS SAI can have multiple MBSFN areas!
//  /** Also a separate map for MBMS Service Area is necessary. We then match MBMS Sm Service Request directly to MBSFN area. */
//  if (mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id) {
//  	h_rc = hashtable_uint64_ts_insert (mce_mbsfn_area_contexts_p->mbms_sai_mbsfn_area_ctx_htbl,
//			  (const hash_key_t)mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id, (void *)((uintptr_t)mbsfn_area_id));
//	  if (HASH_TABLE_OK != h_rc) {
//		  OAILOG_ERROR(LOG_MCE_APP, "Error could not register the MBSFN Service context %p with MBSFN Area Id "MBSFN_AREA_ID_FMT" to MBMS Service Index " MBMS_SERVICE_INDEX_FMT ". \n",
//				  mbsfn_area_context, mbsfn_area_id, mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id);
//		  OAILOG_FUNC_RETURN (LOG_MCE_APP, RETURNerror);
//	  }
//  }
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
int get_mbsfn_mcch_sf(const uint8_t local_mbms_area, const const mbms_service_area_id_t mbms_sai_global)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);

	uint8_t 					mbsfn_mcch_sf 						= 0;
	mbsfn_area_ids_t 	mbsfn_area_ids_local 			= {0};
	mbsfn_area_ids_t 	mbsfn_area_ids_nl_global 	= {0};

	mme_config_read_lock(&mme_config);
	uint8_t 					local_global_mbms_areas = mme_config.mbms.mbms_global_mbsfn_area_per_local_group;
	uint8_t					  mcch_sf_total = get_enb_mbsfn_subframes(get_enb_type(mme_config.mbms.mbms_m2_enb_band), mme_config.mbms.mbms_m2_enb_tdd_ul_dl_sf_conf);
	uint8_t					  mcch_sf_total_size = get_enb_subframe_size(get_enb_type(mme_config.mbms.mbms_m2_enb_band), mme_config.mbms.mbms_m2_enb_tdd_ul_dl_sf_conf);
	uint8_t 					global_mbms_areas_cfg = mme_config.mbms.mbms_global_service_area_types;
	mme_config_unlock(&mme_config);

	/** Get all currently established local/global MBSFN areas. */
	mce_mbsfn_areas_exists_mbms_area(&mce_app_desc.mce_mbsfn_area_contexts, 0, 							 &mbsfn_area_ids_nl_global);

	/**
	 * This is all local & local-global MBSFN areas for the given MBMS area if flag is set.
	 * The above one is not of concern.
	 */
	mce_mbsfn_areas_exists_mbms_area(&mce_app_desc.mce_mbsfn_area_contexts, local_mbms_area, &mbsfn_area_ids_local);

	if(local_global_mbms_areas){
		/**
		 * All MBMS areas are are independent from each other.
		 * Assign the MCCH subframe incrementally, depending on the local MBMS area.
		 * Get all MBSFN areas already configured for the local MBMS area.
		 * Take the next available subframe.
		 *
		 * Iterate over the MBSFN areas and check if any free subframes for the MCCH remain. Take it.
		 */
		if(mcch_sf_total_size == mbsfn_area_ids_local.num_mbsfn_area_ids){
			OAILOG_ERROR(LOG_MCE_APP, "All (%d) available CSA Common subframes for MCCHs allocated. Cannot add new MBSFN in (local) MBMS area (%d).\n",
					mcch_sf_total, local_mbms_area);
			OAILOG_FUNC_RETURN(LOG_MCE_APP, 0x00);
		}
		uint8_t mcch_sf_available = mcch_sf_total;
		for(int num_mbsfn_area = 0; num_mbsfn_area < mbsfn_area_ids_local.num_mbsfn_area_ids; num_mbsfn_area++) {
			mbsfn_area_context_t * mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_ids_local.mbsfn_area_id[num_mbsfn_area]);
			DevAssert(mbsfn_area_context);
			mcch_sf_available ^= mbsfn_area_context->privates.fields.mbsfn_area.mbms_mcch_csa_pattern_1rf;
		}
		/** Iterate through the available subframes. */
		uint8_t csa_sf = 0;
		while (mcch_sf_available){
			if(mcch_sf_available & 0x20) {
				/** Found a free one! Can directly assign to the MBSFN area. */
				mbsfn_mcch_sf = (0x20 >> csa_sf);
				break;
			}
			csa_sf++;
			mcch_sf_available <<=1;
		}
		OAILOG_FUNC_RETURN(LOG_MCE_APP, mcch_sf_available);
	}

	/**
	 * Global MBSFN areas should also be valid in local MBMS areas, this limits the number of MBSFN areas possible.
	 * Global MBSFN areas should also have the same MCCH subframe configuration in the MBSFN synchronization area.
	 * Thus, they need to be reserved.
	 */
	DevAssert(mcch_sf_total_size >= global_mbms_areas_cfg);
	if(mcch_sf_total_size == global_mbms_areas_cfg){
		if(local_mbms_area){
			OAILOG_ERROR(LOG_MCE_APP, "Total available subframes for M2-TDD-ENB with given DL/UL cfg is (%x). Allocating all to global MBMS areas. No local MBMS area is allowed. "
					"Cannot add new MBSFN Area in local MBMS area (%d).\n",
					mcch_sf_total, local_mbms_area);
			OAILOG_FUNC_RETURN(LOG_MCE_APP, 0x00);
		}
	}
	/**
	 * Iterate through the available subframes.
	 * Check the already allocated local MBSFN ares.
	 */
	uint8_t mcch_sf_available = mcch_sf_total;
	for(int num_mbsfn_area = 0; num_mbsfn_area < mbsfn_area_ids_local.num_mbsfn_area_ids; num_mbsfn_area++) {
		mbsfn_area_context_t * mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_ids_local.mbsfn_area_id[num_mbsfn_area]);
		DevAssert(mbsfn_area_context);
		mcch_sf_available ^= mbsfn_area_context->privates.fields.mbsfn_area.mbms_mcch_csa_pattern_1rf;
	}
	/** Check with the reserved MCCH MBSFN subframes. */
	uint8_t csa_sf = 0, csa_sf_matched = 0;
	while (mcch_sf_available) {
		if(!(mcch_sf_available & 0x20)) {
			csa_sf++;
			mcch_sf_available <<=1;
			continue;
		}
		/** Found a free one! Can directly assign to the MBSFN area. */
		csa_sf_matched++;
		if(!local_mbms_area){
			if(csa_sf_matched == mbms_sai_global){
				mbsfn_mcch_sf = (0x20 >> csa_sf);
				break;
			}else
				continue; /**< Continue, till the reserved one is found. */
		} else {
			/** Check the the match count is higher than the reserved sf. */
			if(csa_sf_matched > global_mbms_areas_cfg){
				mbsfn_mcch_sf = (0x20 >> csa_sf);
				break;
			}else
				continue; /**< Continue, till the free one is found. */
		}
	}
	OAILOG_FUNC_RETURN(LOG_MCE_APP, mbsfn_mcch_sf);
}

//------------------------------------------------------------------------------
// Allocation/reservation of mbsfn_area_context->privates.fields.mbsfn_area.mbms_mcch_subframes
static
int mce_app_create_mbsfn_area(const mbsfn_area_id_t mbsfn_area_id, const uint8_t local_mbms_area, const mbms_service_area_id_t mbms_service_area_id, const uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id) {
	OAILOG_FUNC_IN(LOG_MCE_APP);

	mbsfn_area_context_t 									* mbsfn_area_context = NULL;
	// todo: check the lock mechanism
	if(pthread_rwlock_trywrlock(&mce_app_desc.rw_lock)) {
		/** Try to get a free MBMS Service context. */
		mbsfn_area_context = STAILQ_FIRST(&mce_app_desc.mce_mbsfn_area_contexts_list);
		DevAssert(mbsfn_area_context); /**< todo: with locks, it should be guaranteed, that this should exist. */
		if(mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id != INVALID_MBSFN_AREA_ID){
			OAILOG_ERROR(LOG_MCE_APP, "No free MBSFN area left. Cannot allocate a new one.\n");
			pthread_rwlock_unlock(&mce_app_desc.rw_lock);
			OAILOG_FUNC_RETURN (LOG_MCE_APP, RETURNerror);
		}
		/** Check if a free MCCH subframe can be allocated, given the MCE and eNB configuration. */
		uint8_t mcch_mbsfn_sf = get_mbsfn_mcch_sf(local_mbms_area, mbms_service_area_id);
		if(!mcch_mbsfn_sf){
			OAILOG_ERROR(LOG_MCE_APP, "For local MBMS area (%d) and newly received MBSFN area id " MBSFN_AREA_ID_FMT " no MCCH subframes could be scheduled.\n",
					local_mbms_area, mbsfn_area_id);
			pthread_rwlock_unlock(&mce_app_desc.rw_lock);
			OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
		}
		OAILOG_INFO(LOG_MCE_APP, "For local MBMS area (%d) and newly received MBSFN area id " MBSFN_AREA_ID_FMT " could allocate MCCH subframe (%x).",
				local_mbms_area, mbsfn_area_id, mcch_mbsfn_sf);
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
		mbsfn_area_context->privates.fields.local_mbms_area  								= local_mbms_area;

		/**
		 * Set the MCCH configurations from the MmeCfg file.
		 * MME config is already locked.
		 */
		mbsfn_area_context->privates.fields.mbsfn_area.mcch_modif_period_rf 			= mme_config.mbms.mbms_mcch_modification_period_rf;
		mbsfn_area_context->privates.fields.mbsfn_area.mcch_repetition_period_rf  = mme_config.mbms.mbms_mcch_repetition_period_rf;
		mbsfn_area_context->privates.fields.mbsfn_area.mbms_mcch_msi_mcs 					= mme_config.mbms.mbms_mcch_msi_mcs;
		mbsfn_area_context->privates.fields.mbsfn_area.mch_mcs_enb_factor			 	  = mme_config.mbms.mch_mcs_enb_factor;
		mbsfn_area_context->privates.fields.mbsfn_area.mbms_sf_slots_half					= mme_config.mbms.mbms_subframe_slot_half;
		mbsfn_area_context->privates.fields.mbsfn_area.mcch_offset_rf			 				= COMMON_CSA_PATTERN; /**< The 8. RF is the one with the CSA. */
		mbsfn_area_context->privates.fields.mbsfn_area.mbms_mcch_csa_pattern_1rf |= mcch_mbsfn_sf;
		/**
		 * Intelligently, amongst your MBSFN areas set the MCCH subframes.
		 * If the MBSFN area is a non-local global and your local-global flag is set to false: The nl-global
		 * MBSFN areas can exist outside loca MBMS areas and inside multiple different local MBMS areas.
		 * In each area and outside, the exact same number of nl-global MBSFN areas must exist.
		 * They need to have the same CSA pattern for data and the same MCCH subframes!
		 * Thus, they cannot be set incrementedly. Assigned reserved subframes.
		 *
		 * For FDD/TDD, this limits the number of MBSFN areas you can have!
		 * Worst case: 2 MBSFN areas (local+global) in case of TDD: Even if no local MBSFN areas are established yet, if you have 2 nl-global MBSFN areas,
		 * you cannot allow any more local MBSFN areas.
		 */
		//		if(get_enb_type(mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_band) == ENB_TYPE_NULL){
		//		} else if(get_enb_type(mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_band) == ENB_TYPE_TDD) {
		//			mbsfn_area_context->privates.fields.mbsfn_area.mbms_mcch_subframes				= 0b000001;
		//		}

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
		OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
	}
	OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
}

//------------------------------------------------------------------------------
static
void mce_update_mbsfn_area(struct mbsfn_areas_s * const mbsfn_areas, uint8_t local_mbms_area, const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id) {
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
		if(mce_app_create_mbsfn_area(mbsfn_area_id, local_mbms_area, mbms_service_area_id, m2_enb_id, assoc_id) == RETURNok){
			mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_id);
			memcpy((void*)&mbsfn_areas->mbsfn_area_cfg[mbsfn_areas->num_mbsfn_areas++].mbsfnArea, (void*)&mbsfn_area_context->privates.fields.mbsfn_area, sizeof(mbsfn_area_t));
			OAILOG_INFO(LOG_MCE_APP, "Created new MBSFN Area " MBSFN_AREA_ID_FMT " for MBMS_SAI " MBMS_SERVICE_AREA_ID_FMT " with eNB information (m2_enb_id=%d, sctp_assoc=%d).\n",
					mbsfn_area_id, mbms_service_area_id, m2_enb_id, assoc_id);
			OAILOG_FUNC_OUT(LOG_MCE_APP);
		}
	}
	OAILOG_ERROR(LOG_MCE_APP, "MBSFN Area " MBSFN_AREA_ID_FMT " for MBMS_SAI " MBMS_SERVICE_AREA_ID_FMT " with eNB information (m2_enb_id=%d, sctp_assoc=%d) could neither be created nor updated. Skipping..\n",
			mbsfn_area_id, mbms_service_area_id, m2_enb_id, assoc_id);
	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

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
static struct mbsfn_area_context_s                           *
mce_mbsfn_area_exists_mbsfn_area_index(
  mce_mbsfn_area_contexts_t * const mce_mbsfn_areas_p, const mbsfn_area_id_t mbsfn_area_id);

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
bool mce_app_set_mcch_timer(struct mbsfn_area_context_s * const mbsfn_area_context);

//------------------------------------------------------------------------------
static
bool mce_app_create_mbsfn_area(const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, const sctp_assoc_id_t assoc_id, const uint32_t m2_enb_id);

//------------------------------------------------------------------------------
static
void mce_update_mbsfn_area(struct mbsfn_areas_s * const mbsfn_areas, const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id);

//------------------------------------------------------------------------------
void mce_app_get_local_mbsfn_areas(const mbms_service_area_t *mbms_service_areas, const uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id, mbsfn_areas_t * const mbsfn_areas)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);

	uint8_t									mbsfn_bitmap_old	 = 0;
	mbsfn_area_id_t				  mbsfn_area_id      = INVALID_MBSFN_AREA_ID;
	mbsfn_area_context_t	 *mbsfn_area_context = NULL; 									/**< Context stored in the MCE_APP hashmap. */

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
			OAILOG_FUNC_OUT(LOG_MCE_APP);
		}
		/** Get the MBSFN Area ID, should be a deterministic function, depending only on the MBMS Service Area Id. */
		mbsfn_bitmap_old = mbsfn_areas->mbsfn_bitmap;
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
	  int val = (mbms_service_areas->serviceArea[num_mbms_area] - mme_config.mbms.mbms_global_service_area_types + 1);
	  int local_area_index = val / mme_config.mbms.mbms_local_service_area_types; /**< 0..  mme_config.mbms.mbms_local_service_areas - 1. */
	  int local_area_type = val % mme_config.mbms.mbms_local_service_area_types;  /**< 0 .. mme_config.mbms.mbms_local_service_area_types - 1. */
	  if(local_area_type < mme_config.mbms.mbms_local_service_areas) {
	  	/**
	  	 * Check that no other local service area is set for the eNB.
	  	 */
	  	if(mbsfn_areas->local_mbms_service_area) {
	  		if(mbsfn_areas->local_mbms_service_area != local_area_type +1){
	  			mme_config_unlock(&mme_config);
	  			OAILOG_ERROR(LOG_MCE_APP, "A local mbms area (%d) is already set for the eNB. Skipping new No local MBMS Service Area (%d). \n",
	  					mbsfn_areas->local_mbms_service_area, local_area_type +1);
	  			continue;
	  		}
	  		/** Continuing with the same local MBMS service area. */
	  	} else {
	  		mbsfn_areas->local_mbms_service_area = local_area_type + 1;
	  		OAILOG_INFO(LOG_MCE_APP, "Setting local MBMS service area (%d) for the eNB. \n", mbsfn_areas->local_mbms_service_area);
	  	}
	  	OAILOG_INFO(LOG_MCE_APP, "Found a valid local MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT ". \n", mbms_service_areas->serviceArea[num_mbms_area]);
	  	/** Return the MBSFN Area. */
	  	if(mme_config.mbms.mbms_global_mbsfn_area_per_local_group){
	  		mbsfn_area_id = mme_config.mbms.mbms_global_service_area_types
	  				+ local_area_type * (mme_config.mbms.mbms_local_service_area_types + mme_config.mbms.mbms_global_service_area_types)
						+ (local_area_type +1);
	  	} else {
	  		/** Return the MBMS service area as the MBSFN area. We use the same identifiers. */
	  		mbsfn_area_id = mbms_service_areas->serviceArea[num_mbms_area];
	  	}
	  	/**
	  	 * Check if the MBSFN service area is already set for this particular request.
	  	 * In both cases, we should have unique MBSFN areas for the MBMS service areas.
	  	 */
	  	mbsfn_areas->mbsfn_bitmap |= ((0x01) << mbsfn_area_id);
	  	if(mbsfn_bitmap_old == mbsfn_areas->mbsfn_bitmap){
	  		/** We assume, that it is the same MBMS service area. Don't further process the MBMS Service area. */
	  		OAILOG_ERROR(LOG_MCE_APP, "Local MBSFN Area " MBSFN_AREA_ID_FMT " is already set in the MBSFN Area object. Skipping MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". \n",
	  				mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area]);
	  	} else {
	  		mce_update_mbsfn_area(mbsfn_areas, mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area], m2_enb_id, assoc_id);
	  	}
	  } else {
	  	OAILOG_ERROR(LOG_MCE_APP, "MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT " is not a valid local MBMS service area. Skipping. \n", mbms_service_areas->serviceArea[num_mbms_area]);
	  }
		mme_config_unlock(&mme_config);
	}
	/** Get the local service area (geographical). */
	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

/**
 * Function to handle global MBSFN areas.
 * This assumes, that the MBSFN areas object already did undergone local MBMS service area operations.
 */
//------------------------------------------------------------------------------
void mce_app_get_global_mbsfn_areas(const mbms_service_area_t *mbms_service_areas, const uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id, mbsfn_areas_t * const mbsfn_areas)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	mbsfn_area_id_t					mbsfn_area_id			 = INVALID_MBSFN_AREA_ID;
	uint8_t									mbsfn_bitmap_old	 = 0;
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
		mbsfn_bitmap_old = mbsfn_areas->mbsfn_bitmap;
		/**
		 * Found a global area. If the eNB has a local area and if we use local MBMS service areas,
		 * we use a different MBSFN a
		 * MBMS Service areas would be set. */
		if(mme_config.mbms.mbms_global_mbsfn_area_per_local_group) {
			/**
			 * We check if the eNB is in a local MBMS area, so assign a group specific MBSFN Id.
			 * todo: later --> Resources will be partitioned accordingly.
			 */
			if(mbsfn_areas->local_mbms_service_area) {
				OAILOG_INFO(LOG_MCE_APP, "Configuring local MBMS group specific global MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " for local group %d.\n",
						mbms_service_areas->serviceArea[num_mbms_area], mbsfn_areas->local_mbms_service_area);
				mbsfn_area_id = mme_config.mbms.mbms_global_service_area_types +
						(mbsfn_areas->local_mbms_service_area -1) * (mme_config.mbms.mbms_local_service_area_types + mme_config.mbms.mbms_global_service_area_types)
								+ mme_config.mbms.mbms_global_service_area_types;
				/** If the MBSFN area is already allocated, skip to the next one. */
				mbsfn_areas->mbsfn_bitmap |= ((0x01) << mbsfn_area_id);
				if(mbsfn_bitmap_old == mbsfn_areas->mbsfn_bitmap){
					/** We assume, that it is the same MBMS service area. Don't further process the MBMS Service area. */
					OAILOG_ERROR(LOG_MCE_APP, "Global MBSFN Area " MBSFN_AREA_ID_FMT " is already set in the MBSFN Areas object. Skipping MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". \n",
							mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area]);
					mme_config_unlock(&mme_config);
					continue;
				}
				/**
				 * Found a new local specific valid global MBMS group.
				 * Check if there exists an MBSFN group for that.
				 */
  			mce_update_mbsfn_area(mbsfn_areas, mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area], m2_enb_id, assoc_id);
  		  mme_config_unlock(&mme_config);
  		  continue;
			}
		}
		/**
		 * Local specific global MBMS areas are not allowed or eNB does not belong to any local MBMS area.
		 * Return the MBSFN Id as the global MBMS Service Area.
		 */
		mbsfn_area_id = mbms_service_areas->serviceArea[num_mbms_area];
		/** If the MBSFN area is already allocated, skip to the next one. */
		mbsfn_areas->mbsfn_bitmap |= ((0x01) << mbsfn_area_id);
		if(mbsfn_bitmap_old == mbsfn_areas->mbsfn_bitmap){
			/** We assume, that it is the same MBMS service area. Don't further process the MBMS Service area. */
			OAILOG_ERROR(LOG_MCE_APP, "Global MBSFN Area " MBSFN_AREA_ID_FMT " is already set in the MBSFN Areas object. Skipping MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". \n",
					mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area]);
			mme_config_unlock(&mme_config);
			continue;
		}
		/**
		 * Found a new local specific valid global MBMS group.
		 * Check if there exists an MBSFN group for that.
		 */
		mce_update_mbsfn_area(mbsfn_areas, mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area], m2_enb_id, assoc_id);
		mme_config_unlock(&mme_config);
	}
	/** Done checking all global MBMS service areas. */
	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

////------------------------------------------------------------------------------
//static
//void mce_app_update_mbsfn_mcs(mbsfn_area_t * const mbsfn_area) {
//	uint32_t 						m2_enb_count 			= 0;
//	uint32_t					  mbsfn_enb_bitmap 	= mbsfn_area->m2_enb_id_bitmap;
//	/** Count the number of eNBs in the MBSFN. */
//	for (; mbsfn_enb_bitmap; m2_enb_count++)
//	{
//		mbsfn_enb_bitmap &= mbsfn_enb_bitmap - 1;
//	}
//	OAILOG_INFO(LOG_MCE_APP, "Calculated new MCS (%d) for MBSFN Area " MBSFN_AREA_ID_FMT" with %d eNBs. \n.",
//			mbsfn_area->mcs, mbsfn_area->mbsfn_area_id, m2_enb_count);
//}
//
////------------------------------------------------------------------------------
//static
//uint8_t get_mch_mcs_for_enb_id(uint32_t m2_enb_count) {
//
//	/**
//	 * We don't calculate the MCS here.
//	 * MCS depends on the resource calculation time on the Calculate the dynamic MCS for the user plane of the MBSFN area.
//	 * We make the MCS dependent on the QCI of the MBMS Services (should b
//	 */
//
//	return 2; // todo: enum them.. MCS_SINGLE;
//}

//------------------------------------------------------------------------------
static struct mbsfn_area_context_s                           *
mce_mbsfn_area_exists_mbsfn_area_index(
  mce_mbsfn_area_contexts_t * const mce_mbsfn_areas_p, const mbsfn_area_id_t mbsfn_area_id)
{
  struct mbsfn_area_context_s                    *mbsfn_area_context = NULL;
  hashtable_ts_get (mce_mbsfn_areas_p->mbsfn_area_id_mbsfn_area_htbl, (const hash_key_t)mbsfn_area_id, (void **)&mbsfn_area_context);
  return mbsfn_area_context;
}

//------------------------------------------------------------------------------
static
bool mce_app_update_mbsfn_area(const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, const uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id) {
	OAILOG_FUNC_IN(LOG_MCE_APP);
	mbsfn_area_context_t 									* mbsfn_area_context = NULL;
	if(!pthread_rwlock_trywrlock(&mce_app_desc.rw_lock)) {
		mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_index(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_id);
		if(mbsfn_area_context) {
			/** Found an MBSFN area, check if the eNB is registered. */
	 		if(mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_id_bitmap & (1 << m2_enb_id)) {
	 			/** MBSFN Area contains eNB Id. Continuing. */
	 			OAILOG_ERROR(LOG_MCE_APP, "M2 eNB Id (%d) is already in the MBSFN area " MBSFN_AREA_ID_FMT ". Not updating eNB count (we should have distinct M2 eNB id). \n",
	 					m2_enb_id, mbsfn_area_id);
	 			pthread_rwlock_unlock(&mce_app_desc.rw_lock);
	 			OAILOG_FUNC_RETURN (LOG_MME_APP, false);
	 		}
	 		if(mbsfn_area_context->privates.mcch_timer.id == MCE_APP_TIMER_INACTIVE_ID) {
				if(!mce_app_set_mcch_timer(mbsfn_area_context)) {
					/** Put it back as free. */
					OAILOG_INFO(LOG_MCE_APP, "ERROR starting MCCH timer for to be updated MBSFN area %p with MBSFN AI " MBSFN_AREA_ID_FMT".\n", mbsfn_area_context, mbsfn_area_id);
					pthread_rwlock_unlock(&mce_app_desc.rw_lock);
					OAILOG_FUNC_RETURN (LOG_MME_APP, false);
				}
	 		}
	 		/**
	 		 * Updating the eNB count.
	 		 * MCS will be MCH specific of the MBSFN areas, and depend on the QCI/BLER.
	 		 */
	 		mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_id_bitmap  |= (0x01 << m2_enb_id);
	 		/** Check if the MCCH timer is running, if not start it. */
		} else {
			OAILOG_INFO(LOG_MCE_APP, "No MBSFN Area could be found for the MBMS SAI " MBMS_SERVICE_AREA_ID_FMT ". Cannot update. \n.", mbms_service_area_id);
		}
		pthread_rwlock_unlock(&mce_app_desc.rw_lock);
		OAILOG_FUNC_RETURN (LOG_MME_APP, true);
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
	  OAILOG_ERROR(LOG_MCE_APP, "Error could not register the MBSFN Area context %p with MBSFN Area Id " MBSFN_AREA_ID_FMT" and MBMS Service Index " MCE_MBMS_SERVICE_INDEX_FMT ". \n",
			  mbsfn_area_context, mbsfn_area_id, mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id);
	  OAILOG_FUNC_RETURN (LOG_MCE_APP, RETURNerror);
  }

  /** Also a separate map for MBMS Service Area is necessary. We then match MBMS Sm Service Request directly to MBSFN area. */
  if (mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id) {
  	h_rc = hashtable_uint64_ts_insert (mce_mbsfn_area_contexts_p->mbms_sai_mbsfn_area_ctx_htbl,
			  (const hash_key_t)mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id, (void *)((uintptr_t)mbsfn_area_id));
	  if (HASH_TABLE_OK != h_rc) {
		  OAILOG_ERROR(LOG_MCE_APP, "Error could not register the MBSFN Service context %p with MBSFN Area Id "MBSFN_AREA_ID_FMT" to MBMS Service Index " MCE_MBMS_SERVICE_INDEX_FMT ". \n",
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
	/** Stop the MCCH timer of the MBSFN area. */
  if (timer_remove(mbsfn_area_context->privates.mcch_timer.id, NULL)) {
  	OAILOG_ERROR (LOG_MME_APP, "Failed to stop the MCCH timer for the MBSFN area "MBSFN_AREA_ID_FMT ". \n", mbsfn_area_id);
  }
  mbsfn_area_context->privates.mcch_timer.id = MCE_APP_TIMER_INACTIVE_ID;
	memset(&mbsfn_area_context->privates.fields, 0, sizeof(mbsfn_area_context->privates.fields));
	OAILOG_INFO(LOG_MME_APP, "Successfully cleared MBSFN area "MME_UE_S1AP_ID_FMT ". \n", mbsfn_area_id);
	OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
static
bool mce_app_set_mcch_timer(struct mbsfn_area_context_s * const mbsfn_area_context) {
	/** Start the MCCH timer for the MBSFN MCCH. */
	int mcch_timer_us = mme_config.mbms.mbms_mcch_repetition_period_rf * 1000;
	int mcch_timer_s  = mcch_timer_us = mcch_timer_us / 1000000;
	mcch_timer_us = mcch_timer_us % 1000000;
	// todo: set the OFFset && clear the synchronized with the eNBs.. (set another timer to set the timer?!)
	if (timer_setup (mcch_timer_s, mcch_timer_us,
			TASK_MCE_APP, INSTANCE_DEFAULT, TIMER_PERIODIC,  (void *)(mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id), &(mbsfn_area_context->privates.mcch_timer.id)) < 0) {
		OAILOG_ERROR (LOG_MME_APP, "Failed to start the MCCH repetition timer for MBSFN Area " MBSFN_AREA_ID_FMT" for duration of %d RFs. Will retry.. \n", mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id,
				mme_config.mbms.mbms_mcch_repetition_period_rf);
		mbsfn_area_context->privates.mcch_timer.id = MCE_APP_TIMER_INACTIVE_ID;
		return false;
	}
	OAILOG_ERROR (LOG_MME_APP, "Started the MCCH repetition timer for MBSFN Area " MBSFN_AREA_ID_FMT" for duration of %d RFs. \n", mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id,
			mme_config.mbms.mbms_mcch_repetition_period_rf);
	/** Upon expiration, invalidate the timer.. no flag needed. */
	return true;
}

extern int enb_bands[];

//------------------------------------------------------------------------------
static
bool mce_app_create_mbsfn_area(const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, const sctp_assoc_id_t assoc_id, const uint32_t m2_enb_id) {
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
		if(!mce_app_set_mcch_timer(mbsfn_area_context)) {
			/** Put it back as free. */
			STAILQ_INSERT_HEAD(&mce_app_desc.mce_mbsfn_area_contexts_list, mbsfn_area_context, entries);
			OAILOG_INFO(LOG_MCE_APP, "ERROR starting MCCH timer for MBSFN area %p with MBSFN AI " MBSFN_AREA_ID_FMT".\n", mbsfn_area_context, mbsfn_area_id);
			pthread_rwlock_unlock(&mce_app_desc.rw_lock);
			OAILOG_FUNC_RETURN (LOG_MME_APP, false);
		}
		mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id			  = mbsfn_area_id;
		mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id = mbms_service_area_id;

		/**
		 * Set the MCCH configurations from the MmeCfg file.
		 * MME Config is already locked.
		 */
		mbsfn_area_context->privates.fields.mbsfn_area.mcch_modif_period_rf 			= mme_config.mbms.mbms_mcch_modification_period_rf;
		mbsfn_area_context->privates.fields.mbsfn_area.mcch_repetition_period_rf  = mme_config.mbms.mbms_mcch_repetition_period_rf;
		mbsfn_area_context->privates.fields.mbsfn_area.mbms_mcch_msi_mcs 					= mme_config.mbms.mbms_mcch_msi_mcs;
		// todo: a function which checks multiple MBSFN areas for MCCH offset
		// todo: MCCH offset is calculated in the MCH resources?
		mbsfn_area_context->privates.fields.mbsfn_area.mcch_offset_rf			 				= 0;
		/** Indicate the MCCH subframes. */
		// todo: a function, depending on the number of total MBSFN areas, repetition/modification periods..
		// todo: calculate at least for 1 MCH of 1 MCCH the MCCH allocation, eNB can allocate dynamically for the correct subframe?
		mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_type = enb_bands[mme_config.mbms.mbms_enb_band];
		if(mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_type == ENB_TYPE_FDD){
			mbsfn_area_context->privates.fields.mbsfn_area.mbms_mcch_subframes				= 0b00100000;
		} else if(mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_type == ENB_TYPE_TDD) {
			mbsfn_area_context->privates.fields.mbsfn_area.mbms_mcch_subframes				= 0b00001000;
		} else {
			DevMessage("MBSFN band is invalid!");
		}


		/** Set the M2 eNB Id. Nothing else needs to be done for the MCS part. */
		mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_id_bitmap |= (0x01 << m2_enb_id);
		/** Add the MBSFN area to the back of the list. */
		STAILQ_INSERT_TAIL(&mce_app_desc.mce_mbsfn_area_contexts_list, mbsfn_area_context, entries);
		/** Add the MBSFN area into the MBMS service Hash Map. */
		DevAssert (mce_insert_mbsfn_area_context(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_context) == 0);
		pthread_rwlock_unlock(&mce_app_desc.rw_lock);
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
		mbsfn_areas->num_mbsfn_areas++;
		mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_index(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_id);
		memcpy((void*)&mbsfn_areas->mbsfnArea[mbsfn_areas->num_mbsfn_areas], (void*)&mbsfn_area_context->privates.fields.mbsfn_area, sizeof(mbsfn_area_t));
		OAILOG_INFO(LOG_MCE_APP, "MBSFN Area " MBSFN_AREA_ID_FMT " is already active. Successfully updated for MBMS_SAI " MBMS_SERVICE_AREA_ID_FMT " with eNB information (m2_enb_id=%d, sctp_assoc=%d).\n",
				mbsfn_area_id, mbms_service_area_id, m2_enb_id, assoc_id);
	} else {
		/** Could not update. Check if it needs to be created. */
		if(mce_app_create_mbsfn_area(mbsfn_area_id, mbms_service_area_id, assoc_id, m2_enb_id)){
			mbsfn_areas->num_mbsfn_areas++;
			mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_index(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_id);
			memcpy((void*)&mbsfn_areas->mbsfnArea[mbsfn_areas->num_mbsfn_areas], (void*)&mbsfn_area_context->privates.fields.mbsfn_area, sizeof(mbsfn_area_t));
			OAILOG_INFO(LOG_MCE_APP, "Created new MBSFN Area " MBSFN_AREA_ID_FMT " for MBMS_SAI " MBMS_SERVICE_AREA_ID_FMT " with eNB information (m2_enb_id=%d, sctp_assoc=%d).\n",
					mbsfn_area_id, mbms_service_area_id, m2_enb_id, assoc_id);
		}
	}
	OAILOG_ERROR(LOG_MCE_APP, "MBSFN Area " MBSFN_AREA_ID_FMT " for MBMS_SAI " MBMS_SERVICE_AREA_ID_FMT " with eNB information (m2_enb_id=%d, sctp_assoc=%d) could neither be created nor updated. Skipping..\n",
			mbsfn_area_id, mbms_service_area_id, m2_enb_id, assoc_id);
	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

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

////------------------------------------------------------------------------------
//static bstring bearer_state2string(const mce_app_bearer_state_t bearer_state)
//{
//  bstring bsstr = NULL;
//  if  (BEARER_STATE_NULL == bearer_state) {
//    bsstr = bfromcstr("BEARER_STATE_NULL");
//    return bsstr;
//  }
//  bsstr = bfromcstr(" ");
//  if  (BEARER_STATE_SGW_CREATED & bearer_state) bcatcstr(bsstr, "SGW_CREATED ");
//  if  (BEARER_STATE_MME_CREATED & bearer_state) bcatcstr(bsstr, "MME_CREATED ");
//  if  (BEARER_STATE_ENB_CREATED & bearer_state) bcatcstr(bsstr, "ENB_CREATED ");
//  if  (BEARER_STATE_ACTIVE & bearer_state) bcatcstr(bsstr, "ACTIVE");
//  return bsstr;
//}

//------------------------------------------------------------------------------
static
uint8_t get_mcs_for_enb_id(uint32_t m2_enb_count) {

	return 2; // todo: enum them.. MCS_SINGLE;
}


//------------------------------------------------------------------------------
static
mbsfn_area_id_t generate_mbsfn_area_id(const mbms_service_area_id_t mbms_sai) {

	return INVALID_MBSFN_AREA_ID;
}

//------------------------------------------------------------------------------
struct mbsfn_area_context_s                           *
mce_mbsfn_area_exists_mbsfn_area_index(
  mce_mbsfn_areas_t * const mce_mbsfn_areas_p, const mbsfn_area_id_t mbsfn_area_id)
{
  struct mbsfn_area_context_s                    *mbsfn_area_context = NULL;
  hashtable_ts_get (mce_mbsfn_areas_p->mbsfn_area_id_mbsfn_area_htbl, (const hash_key_t)mbsfn_area_id, (void **)&mbsfn_area_context);
  return mbsfn_area_context;
}

//------------------------------------------------------------------------------
static
void mce_app_update_mbsfn_mcs(mbsfn_area_t * const mbsfn_area) {
	OAILOG_FUNC_IN(LOG_MCE_APP);

	uint32_t 						m2_enb_count = 0;
	uint32_t					  mbsfn_enb_bitmap = mbsfn_area->m2_enb_id_bitmap;
	/** Count the number of eNBs in the MBSFN. */
	for (; mbsfn_enb_bitmap; m2_enb_count++)
	{
		mbsfn_enb_bitmap &= mbsfn_enb_bitmap - 1;
	}

	mbsfn_area->mcs = get_mcs_for_enb_id(m2_enb_count);
	OAILOG_INFO(LOG_MCE_APP, "Calculated new MCS (%d) for MBSFN Area " MBSFN_AREA_ID_FMT" \n.", mbsfn_area->mcs, mbsfn_area->mbsfn_area_id);

	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

//------------------------------------------------------------------------------
static
void mce_app_reset_mbsfn_area_ids(const uint32_t m2_enb_id, const sctp_assoc_id_t assoc_id) {
	/**
	 * Iterate through the MBSFN services, check which one contains the given eNB Id.
	 * Decrease the #eNBs and recalculate the MCS.
	 */
	// iterate through all MBSFN services, since an eNB may be in multiple MBSFN services.
	mbsfn_area_context_t * mbsfn_area_context = NULL;
	STAILQ_FOREACH(mbsfn_area_context, &mce_app_desc.mce_mbsfn_area_contexts_list, entries) {
		if(mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_id_bitmap & (0x01 << m2_enb_id)){
			mbsfn_area_context->privates.fields.mbsfn_area.m2_enb_id_bitmap &= ~(0x01 << m2_enb_id);
			mce_app_update_mbsfn_mcs(&mbsfn_area_context->privates.fields.mbsfn_area);
		}
	}
}

//------------------------------------------------------------------------------
static
bool mce_app_update_mbsfn_area(const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, const sctp_assoc_id_t assoc_id, const uint32_t m2_enb_id) {

	mbsfn_area_t 									* mbsfn_area = NULL;
	OAILOG_FUNC_IN(LOG_MCE_APP);

	// todo: lock contexts and get the MBSFN service areas..
	if(mbsfn_area) {
		/** Found an MBSFN area, check if the eNB is registered. */
		if(mbsfn_area->m2_enb_id_bitmap & (1 << m2_enb_id)){
			/** MBSFN Area contains eNB Id. Continuing. */
			OAILOG_INFO(LOG_MCE_APP, "eNB Id (%d) is already in the MBSFN area " MBSFN_AREA_ID_FMT ". Not updating eNB count. \n", m2_enb_id, mbsfn_area->mbsfn_area_id);
			// OAILOG_FUNC_RETURN(LOG_MCE_APP, mbsfn_service_area->mbms_service_area_id);
		} else {
			/** Reset all MBSFN service areas for the given MBSFN Id. */
			mbsfn_area->m2_enb_id_bitmap  |= (0x01 << m2_enb_id);
			/** Recalculate the MCS of the MBSFN area, based on the new eNB Id. */
			mce_app_update_mbsfn_mcs(mbsfn_area);
		}
	} else {
		OAILOG_INFO(LOG_MCE_APP, "No MBSFN Area could be found for the MBMS SAI " MBMS_SERVICE_AREA_ID_FMT ". \n.", mbms_service_area_id);
	}
	// todo: unlock
	OAILOG_FUNC_RETURN(LOG_MCE_APP, (mbsfn_area));
}

//------------------------------------------------------------------------------
static
bool mce_app_create_mbsfn_area(const mbsfn_area_id_t mbsfn_area_id, const mbms_service_area_id_t mbms_service_area_id, const sctp_assoc_id_t assoc_id, const uint32_t m2_enb_id) {

	mbsfn_area_t 									* mbsfn_area = NULL;
	OAILOG_FUNC_IN(LOG_MCE_APP);

//	// todo: lock contexts and get the MBSFN service areas..
//	if(mbsfn_area) {
//		/** Found an MBSFN area, check if the eNB is registered. */
//		if(mbsfn_area->m2_enb_id_bitmap & (1 << m2_enb_id)){
//			/** MBSFN Area contains eNB Id. Continuing. */
//			OAILOG_INFO(LOG_MCE_APP, "eNB Id (%d) is already in the MBSFN area " MBSFN_AREA_ID_FMT ". Not updating eNB count. \n", m2_enb_id, mbsfn_area->mbsfn_area_id);
//			// OAILOG_FUNC_RETURN(LOG_MCE_APP, mbsfn_service_area->mbms_service_area_id);
//		} else {
//			/** Reset all MBSFN service areas for the given MBSFN Id. */
//			mbsfn_area->m2_enb_id_bitmap  |= (0x01 << m2_enb_id);
//			/** Recalculate the MCS of the MBSFN area, based on the new eNB Id. */
//			mce_app_update_mbsfn_mcs(mbsfn_area);
//		}
//	} else {
//		OAILOG_INFO(LOG_MCE_APP, "No MBSFN Area could be found for the MBMS SAI " MBMS_SERVICE_AREA_ID_FMT ". \n.", mbms_service_area_id);
//	}
	// todo: unlock
	OAILOG_FUNC_RETURN(LOG_MCE_APP, (mbsfn_area));
}

//------------------------------------------------------------------------------
void
mce_app_update_mbsfn_areas(const mbms_service_area_t * mbms_service_areas, const sctp_assoc_id_t assoc_id, const uint32_t m2_enb_id,
		mbsfn_areas_t * const mbsfn_areas)
{
	mbsfn_area_context_t * mbsfn_area_context = NULL, * mbsfn_area_context_enb 		= NULL;
	mbsfn_area_id_t 			mbsfn_area_id = INVALID_MBSFN_AREA_ID;
	uint8_t								mbsfn_bitmap  = 0;

	OAILOG_FUNC_IN(LOG_MCE_APP);
	DevAssert(mbms_service_areas);

	/** Reset the eNB configuration. */
	mce_app_reset_mbsfn_area_ids(m2_enb_id, assoc_id);
	/** Go for each MBMS Service area, check for MBSFN area. */
	for(int num_mbms_area = 0; num_mbms_area < mbms_service_areas->num_service_area; num_mbms_area ++){
		/** Get the MBSFN Area ID, should be a deterministic function, depending only on the MBMS Service Area Id. */
		mbsfn_bitmap = mbsfn_areas->mbsfn_bitmap;
		mbsfn_area_id = generate_mbsfn_area_id(mbms_service_areas->serviceArea[num_mbms_area]);
		/** Check if an MBSFN Area exists. */
		if(mbsfn_area_id == INVALID_MBSFN_AREA_ID) {
			OAILOG_ERROR(LOG_MCE_APP, "No MBSFN Area Id " MBSFN_AREA_ID_FMT " could be generated for the MBMS Service Area Id "MBMS_SERVICE_AREA_ID_FMT ". \n",
					mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area]);
			continue;
		}
		/** Check the eNB registration. */
		mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_index(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_id);
		if(mbsfn_area_context) {
			/** Update the MBSFN area. */
			if(mce_app_update_mbsfn_area(mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area], assoc_id, m2_enb_id)) {
				OAILOG_INFO(LOG_MCE_APP, "Found an MBSFN Area " MBSFN_AREA_ID_FMT " for MBMS SAI " MBMS_SERVICE_AREA_ID_FMT " and updated it with the eNB information. \n",
						mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id, mbms_service_areas->serviceArea[num_mbms_area]);
				/** Add the MBSFN area matched to the MBMS Service Area. */
				mbsfn_areas->mbsfnArea[mbsfn_areas->num_mbsfn_areas++].mbms_service_area_id = mbms_service_areas->serviceArea[num_mbms_area];
				/** For checking, that the MBSFN areas are unique. */
				mbsfn_areas->mbsfn_bitmap |= ((0x01) << mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
				if(mbsfn_bitmap == mbsfn_areas->mbsfn_bitmap){
					OAILOG_WARNING(LOG_MCE_APP, "MBSFN Area " MBSFN_AREA_ID_FMT " was already existing. Not incrementing count. \n", mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
				}
			} else {
				OAILOG_ERROR(LOG_MCE_APP, "Error updating MBSFN Area " MBSFN_AREA_ID_FMT " for MBMS SAI " MBMS_SERVICE_AREA_ID_FMT " with the eNB information. Not returning. \n",
						mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id, mbms_service_areas->serviceArea[num_mbms_area]);
				continue;
			}
		} else {
			OAILOG_INFO(LOG_MCE_APP, "No MBSFN Area was found for MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". Trying to establish a new one. \n",
					mbms_service_areas->serviceArea[num_mbms_area]);
			if(!mce_app_create_mbsfn_area(mbsfn_area_id, mbms_service_areas->serviceArea[num_mbms_area], assoc_id, m2_enb_id)) {
				OAILOG_ERROR(LOG_MCE_APP, "No MBSFN Area could be created for MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". Skipping. \n",
						mbms_service_areas->serviceArea[num_mbms_area]);
				continue;
			} else {
				OAILOG_INFO(LOG_MCE_APP, "Successfully created a new MBSFN Area for MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ", for the eNB (sctp_id=%d, m2_enb_id=%d). \n",
						mbms_service_areas->serviceArea[num_mbms_area], assoc_id, m2_enb_id);
				mbsfn_areas->mbsfnArea[mbsfn_areas->num_mbsfn_areas++].mbms_service_area_id = mbms_service_areas->serviceArea[num_mbms_area];
				mbsfn_areas->mbsfn_bitmap |= ((0x01) << mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
				if(mbsfn_bitmap == mbsfn_areas->mbsfn_bitmap){
					DevMessage("MBSFN Area " + mbsfn_area_context->privates.fields.mbsfn_area.mbms_service_area_id + " was already existing for newly created MBSFN Area.");
				}
				/** MBSFN Areas area updated. */
			}
		}
	}

	/** Check if the SCTP association for the given eNB-Id exists. */
	OAILOG_INFO(LOG_MCE_APP, "Returning %d MBSFN areas for the received %d MBMS Service Areas. \n",
			mbsfn_areas->num_mbsfn_areas, mbms_service_areas->num_service_area);
	/** Update the MBSFN service area for the given eNB. */
	OAILOG_FUNC_OUT(LOG_MCE_APP);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

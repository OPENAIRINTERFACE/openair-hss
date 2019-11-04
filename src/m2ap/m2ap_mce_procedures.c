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

/*! \file m2ap_mce_procedures.c
  \brief
  \author Dincer Beken
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "assertions.h"
#include "hashtable.h"
#include "log.h"
#include "msc.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "asn1_conversions.h"
#include "timer.h"

#include "mme_config.h"
#include "m2ap_common.h"
#include "m2ap_mce_encoder.h"
#include "m2ap_mce.h"
#include "m2ap_mce_itti_messaging.h"
#include "m2ap_mce_procedures.h"


/* Every time a new MBMS service is associated, increment this variable.
   But care if it wraps to increment also the mce_mbms_m2ap_id_has_wrapped
   variable. Limit: UINT32_MAX (in stdint.h).
*/
extern const char                      *m2ap_direction2String[];
extern hash_table_ts_t 					g_m2ap_mbms_coll; 	// MCE MBMS M2AP ID association to MBMS Reference;
extern hash_table_ts_t 					g_m2ap_enb_coll; 	// SCTP Association ID association to M2AP eNB Reference;


/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/
static int m2ap_generate_mbms_session_start_request(mce_mbms_m2ap_id_t mbms_m2ap_id, const uint8_t num_m2ap_enbs, m2ap_enb_description_t ** m2ap_enb_descriptions);
static int m2ap_generate_mbms_session_update_request(mce_mbms_m2ap_id_t mce_mbms_m2ap_id, sctp_assoc_id_t sctp_assoc_id);
static int m2ap_generate_mbms_session_stop_request(mce_mbms_m2ap_id_t mce_mbms_m2ap_id, sctp_assoc_id_t sctp_assoc_id);
static int m2ap_update_mbms_service_context(const mce_mbms_m2ap_id_t mce_mbms_m2ap_id);

//------------------------------------------------------------------------------
void
m3ap_handle_mbms_session_start_request (
  const itti_m3ap_mbms_session_start_req_t * const mbms_session_start_req_pP)
{
  /*
   * MBMS-GW triggers a new MBMS Service. No eNBs are specified but only the MBMS service area.
   * MCE_APP will not be eNB specific. eNB specific messages will be handled in the M2AP_APP.
   * That a single eNB fails to broadcast, is not of importance to the MCE_APP.
   * This message initiates for all eNBs in the given MBMS Service are a new MBMS Bearer Service.
   */
  uint8_t                                *buffer_p = NULL;
  uint32_t                                length = 0;
  uint8_t								  num_m2ap_enbs = 0;
  mbms_description_t               		 *mbms_ref = NULL;
  mce_mbms_m2ap_id_t 					  mce_mbms_m2ap_id 	= INVALID_MCE_MBMS_M2AP_ID;
  OAILOG_FUNC_IN (LOG_M2AP);
  DevAssert (mbms_session_start_req_pP != NULL);

  /*
   * We need to check the MBMS Service via TMGI and MBMS Service Index.
   * Currently, only their combination must be unique and only 1 SA can be activated at a time.
   *
   * MCE MBMS M2AP ID is 24 bits long, so we cannot use MBMS Service Index.
   */
  mbms_ref = m2ap_is_mbms_tmgi_in_list(&mbms_session_start_req_pP->tmgi, mbms_session_start_req_pP->mbms_service_area_id); /**< Nothing eNB specific. */
  if (mbms_ref) {
    /**
     * Just remove this implicitly without notifying the eNB.
     * There should be complications with the response, eNB should be able to handle this.
     */
    OAILOG_ERROR (LOG_M2AP, "An MBMS Service Description with for TMGI " TMGI_FMT " and MBMS_Service_Area ID " MBMS_SERVICE_AREA_ID_FMT "already exists. Removing implicitly. \n",
        TMGI_ARG(&mbms_session_start_req_pP->tmgi), mbms_session_start_req_pP->mbms_service_area_id);
    /**
     * Remove the MBMS Reference from the HashTable.
     * MBMS Remove Function expected to be called. Should decrement also the MBMS count in the eNBs.
     */
    hashtable_ts_free (&g_m2ap_mbms_coll, mbms_ref->mce_mbms_m2ap_id);
  }

  /** Check that there exists at least a single eNB with the MBMS Service Area (we don't start MBMS sessions for eNBs which later on connected). */
  mme_config_read_lock (&mme_config);
  m2ap_enb_description_t *			         m2ap_enb_p_elements[mme_config.max_m2_enbs];
  memset(&m2ap_enb_p_elements, 0, (sizeof(m2ap_enb_description_t*) * mme_config.max_m2_enbs));
  mme_config_unlock (&mme_config);

  m2ap_is_mbms_sai_in_list(mbms_session_start_req_pP->mbms_service_area_id, &num_m2ap_enbs, (m2ap_enb_description_t **)&m2ap_enb_p_elements);
  if(!num_m2ap_enbs){
    OAILOG_ERROR (LOG_M2AP, "No M2AP eNBs could be found for the MBMS SA " MBMS_SERVICE_AREA_ID_FMT" for the MBMS Service with TMGI " TMGI_FMT". \n",
    	mbms_session_start_req_pP->mbms_service_area_id, TMGI_ARG(&mbms_session_start_req_pP->tmgi));
    OAILOG_FUNC_OUT (LOG_M2AP);
  }

  /**
   * We don't care to inform the MCE_APP layer.
   * Create a new MBMS Service Description.
   * Allocate an MCE M2AP MBMS ID (24) inside it. Will be used for all eNB associations.
   */
  if((mbms_ref = m2ap_new_mbms (&mbms_session_start_req_pP->tmgi, mbms_session_start_req_pP->mbms_service_area_id)) == NULL) {
    // If we failed to allocate a new MBMS Service Description return -1
    OAILOG_ERROR (LOG_M2AP, "M2AP:MBMS Session Start Request- Failed to allocate M2AP Service Description for TMGI " TMGI_FMT " and MBMS Service Area Id "MBMS_SERVICE_AREA_ID_FMT". \n",
    	TMGI_ARG(&mbms_session_start_req_pP->tmgi), mbms_session_start_req_pP->mbms_service_area_id);
    OAILOG_FUNC_OUT (LOG_M2AP);
  }

  mce_mbms_m2ap_id = mbms_ref->mce_mbms_m2ap_id;
  hashtable_rc_t  hash_rc = hashtable_ts_insert (&g_m2ap_enb_coll, (const hash_key_t)mce_mbms_m2ap_id, (void *)mbms_ref);
  if (HASH_TABLE_OK != hash_rc) {
	OAILOG_ERROR (LOG_M2AP, "M2AP:MBMS Service Reference for MCE MBMS M2AP Id "MCE_MBMS_M2AP_ID_FMT". \n", mce_mbms_m2ap_id);
    OAILOG_FUNC_OUT(LOG_M2AP);
  }

  /**
   * Update the created MBMS Service Description.
   * Directly set the values and don't wait for a response, we will set these values into the eNB, once the timer runs out.
   * We don't need the MBMS Session Duration. It will be handled in the MCE_APP layer.
   */
  memcpy((void*)&mbms_ref->mbms_bc.mbms_ip_mc_distribution,  (void*)&mbms_session_start_req_pP->mbms_bearer_tbc.mbms_ip_mc_dist, sizeof(mbms_ip_multicast_distribution_t));
  memcpy((void*)&mbms_ref->mbms_bc.eps_bearer_context.bearer_level_qos, (void*)&mbms_session_start_req_pP->mbms_bearer_tbc.bc_tbc.bearer_level_qos, sizeof(bearer_qos_t));
  /**
   * Check if a timer has been given, if so start the timer.
   * If not send immediately.
   */
  if(mbms_session_start_req_pP->abs_start_time_sec){
	uint32_t delta_to_start_sec = mbms_session_start_req_pP->abs_start_time_sec - time(NULL);
	OAILOG_INFO (LOG_M2AP, "M2AP:MBMS Session Start Request- Received a timer of (%d)s for start of TMGI " TMGI_FMT " and MBMS Service Area ID "MBMS_SERVICE_AREA_ID_FMT". \n",
		delta_to_start_sec, TMGI_ARG(&mbms_session_start_req_pP->tmgi), mbms_session_start_req_pP->mbms_service_area_id);
	// todo: calculate delta for usec
    if (timer_setup (delta_to_start_sec, mbms_session_start_req_pP->abs_start_time_usec,
           TASK_M2AP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void*)mce_mbms_m2ap_id, &(mbms_ref->m2ap_action_timer.id)) < 0) {
      OAILOG_ERROR (LOG_M2AP, "Failed to start MBMS Sesssion Start timer for MBMS with MBMS M2AP MCE ID " MCE_MBMS_M2AP_ID_FMT". \n", mce_mbms_m2ap_id);
      mbms_ref->m2ap_action_timer.id = M2AP_TIMER_INACTIVE_ID;
      /** Send immediately. */
    } else {
      OAILOG_DEBUG (LOG_M2AP, "Started M2AP MBMS Session start timer (timer id 0x%lx) for MBMS Session MBMS M2AP MCE ID " MCE_MBMS_M2AP_ID_FMT". "
    		  "Waiting for timeout to trigger M2AP Session Start to M2AP eNBs.\n", mbms_ref->m2ap_action_timer.id, mce_mbms_m2ap_id);
      /** Leave the method. */
      OAILOG_FUNC_OUT(LOG_M2AP);
    }
  }
  m2ap_generate_mbms_session_start_request(mce_mbms_m2ap_id, num_m2ap_enbs, m2ap_enb_p_elements);
  OAILOG_FUNC_OUT(LOG_M2AP);
}

//------------------------------------------------------------------------------
void
m3ap_handle_mbms_session_update_request (
  const itti_m3ap_mbms_session_update_req_t * const mbms_session_update_req_pP)
{
  /*
   * MBMS-GW triggers the stop of an MBMS Service on all the eNBs which are receiving it.
   */
  uint8_t                                *buffer_p = NULL;
  uint32_t                                length = 0;
  mbms_description_t               		 *mbms_ref 					 = NULL;
  M2AP_M2AP_PDU_t                         pdu = {0};

  OAILOG_FUNC_IN (LOG_M2AP);
  DevAssert (mbms_session_update_req_pP != NULL);
  /*
   * We need to check the MBMS Service via TMGI and MBMS Service Index.
   */
  mbms_ref = m2ap_is_mbms_tmgi_in_list(&mbms_session_update_req_pP->tmgi, mbms_session_update_req_pP->old_mbms_service_area_id); /**< Nothing eNB specific. */
  if (!mbms_ref) {
    /** No MBMS Reference found, just ignore the message and return. */
	OAILOG_ERROR (LOG_M2AP, "No MBMS Service Description with for TMGI " TMGI_FMT " and MBMS_Service_Area ID " MBMS_SERVICE_AREA_ID_FMT "already exists. \n",
		TMGI_ARG(&mbms_session_update_req_pP->tmgi), mbms_session_update_req_pP->old_mbms_service_area_id);
	OAILOG_FUNC_OUT(LOG_M2AP);
  }

  /**
   * Update the MBMB Service Description rightaway. No need to check eNBs.
   * We at most destroy the sctp-> enb_mbms_m2ap id associations.
   */
  mbms_ref->mbms_service_area_id = mbms_session_update_req_pP->new_mbms_service_area_id;
  memcpy((void*)&mbms_ref->mbms_bc.mbms_ip_mc_distribution,  (void*)&mbms_session_update_req_pP->mbms_bearer_tbc.mbms_ip_mc_dist, sizeof(mbms_ip_multicast_distribution_t));
  memcpy((void*)&mbms_ref->mbms_bc.eps_bearer_context.bearer_level_qos, (void*)&mbms_session_update_req_pP->mbms_bearer_tbc.bc_tbc.bearer_level_qos, sizeof(bearer_qos_t));

  /**
   * Check if there is a timer set.
   * If so postpone the update.
   */
  if(mbms_session_update_req_pP->abs_update_time_sec){
    uint32_t delta_to_update_sec = mbms_session_update_req_pP->abs_update_time_sec- time(NULL);
    OAILOG_INFO (LOG_M2AP, "M2AP:MBMS Session Update Request- Received a timer of (%d)s for start of TMGI " TMGI_FMT " and MBMS Service Area ID "MBMS_SERVICE_AREA_ID_FMT". \n",
    	delta_to_update_sec, TMGI_ARG(&mbms_session_update_req_pP->tmgi), mbms_session_update_req_pP->new_mbms_service_area_id);
    if (timer_setup (delta_to_update_sec, mbms_session_update_req_pP->abs_update_time_usec,
           TASK_M2AP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void*)mbms_ref->mce_mbms_m2ap_id, &(mbms_ref->m2ap_action_timer.id)) < 0) {
         OAILOG_ERROR (LOG_M2AP, "Failed to start MBMS Session Update timer for MBMS with MBMS M2AP MCE ID " MCE_MBMS_M2AP_ID_FMT". \n", mbms_ref->mce_mbms_m2ap_id);
         mbms_ref->m2ap_action_timer.id = M2AP_TIMER_INACTIVE_ID;
         /** Send immediately. */
       } else {
         OAILOG_DEBUG (LOG_M2AP, "Started M2AP MBMS Session Update timer (timer id 0x%lx) for MBMS Session MBMS M2AP MCE ID " MCE_MBMS_M2AP_ID_FMT". "
        		 "Waiting for timeout to trigger M2AP Session Update to M2AP eNBs.\n", mbms_ref->m2ap_action_timer.id, mbms_ref->mce_mbms_m2ap_id);
         /** Leave the method. */
         OAILOG_FUNC_OUT(LOG_M2AP);
       }
  }
  /** Update the MBMS Service Context. */
  m2ap_update_mbms_service_context(mbms_ref->mce_mbms_m2ap_id);
  OAILOG_FUNC_OUT(LOG_M2AP);
}

//------------------------------------------------------------------------------
void
m3ap_handle_mbms_session_stop_request (
  const itti_m3ap_mbms_session_stop_req_t * const mbms_session_stop_req_pP)
{
  /*
   * MBMS-GW triggers the stop of an MBMS Service on all the eNBs which are receiving it.
   */
  uint8_t                                *buffer_p = NULL;
  uint32_t                                length = 0;
  mbms_description_t               		 *mbms_ref = NULL;
  m2ap_enb_description_t                 *target_enb_ref = NULL;
  M2AP_M2AP_PDU_t                         pdu = {0};

  OAILOG_FUNC_IN (LOG_M2AP);
  DevAssert (mbms_session_stop_req_pP != NULL);

  /*
   * We need to check the MBMS Service via TMGI and MBMS Service Index.
   */
  mbms_ref = m2ap_is_mbms_tmgi_in_list(&mbms_session_stop_req_pP->tmgi, mbms_session_stop_req_pP->mbms_service_area_id); /**< Nothing eNB specific. */
  if (!mbms_ref) {
    /**
     * No MBMS Reference found, just ignore the message and return.
     */
	OAILOG_ERROR (LOG_M2AP, "No MBMS Service Description with for TMGI " TMGI_FMT " and MBMS_Service_Area ID " MBMS_SERVICE_AREA_ID_FMT "already exists. \n",
		TMGI_ARG(&mbms_session_stop_req_pP->tmgi), mbms_session_stop_req_pP->mbms_service_area_id);
	OAILOG_FUNC_OUT(LOG_M2AP);
  }

  /** Check that there exists at least a single eNB with the MBMS Service Area (we don't start MBMS sessions for eNBs which later on connected). */
  mme_config_read_lock (&mme_config);
  m2ap_enb_description_t *			         m2ap_enb_p_elements[mme_config.max_m2_enbs];
  memset(&m2ap_enb_p_elements, 0, (sizeof(m2ap_enb_description_t*) * mme_config.max_m2_enbs));
  mme_config_unlock (&mme_config);

  int num_m2ap_enbs = 0;
  m2ap_is_mbms_sai_in_list(mbms_session_stop_req_pP->mbms_service_area_id, &num_m2ap_enbs, (m2ap_enb_description_t **)&m2ap_enb_p_elements);
  if(num_m2ap_enbs){
    // todo: the next_sctp_stream is the one without incrementation?
	for(int i = 0; i < num_m2ap_enbs; i++){
	  m2ap_enb_description_t * target_enb_ref = m2ap_enb_p_elements[i];
	  m2ap_generate_mbms_session_stop_request(mbms_ref->mce_mbms_m2ap_id, target_enb_ref->sctp_assoc_id);
	}
  } else {
    OAILOG_ERROR(LOG_M2AP, "No M2AP eNBs could be found for the MBMS SA " MBMS_SERVICE_AREA_ID_FMT" for the MBMS Service with TMGI " TMGI_FMT" to be removed. Removing implicitly. \n",
    	mbms_session_stop_req_pP->mbms_service_area_id, TMGI_ARG(&mbms_session_stop_req_pP->tmgi));
  }
  /**
   * Remove the MBMS Service.
   * Should also remove all the sctp associations.
   */
  hashtable_ts_free (&g_m2ap_mbms_coll, mbms_ref->mce_mbms_m2ap_id);
  OAILOG_FUNC_OUT(LOG_M2AP);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

//------------------------------------------------------------------------------
static
int m2ap_update_mbms_service_context(const mce_mbms_m2ap_id_t mce_mbms_m2ap_id) {
  mbms_description_t               		 *mbms_ref 					 = NULL;
  int 									  num_m2ap_enbs_new_mbms_sai = 0;
  m2ap_enb_description_t           		 *m2ap_enb_description 		 = NULL;

  OAILOG_FUNC_IN (LOG_M2AP);

  /** Check that there exists at least a single eNB with the MBMS Service Area (we don't start MBMS sessions for eNBs which later on connected). */
  mme_config_read_lock (&mme_config);
  m2ap_enb_description_t *			         new_mbms_sai_m2ap_enb_p_elements[mme_config.max_m2_enbs];
  memset(&new_mbms_sai_m2ap_enb_p_elements, 0, (sizeof(m2ap_enb_description_t*) * mme_config.max_m2_enbs));
  mme_config_unlock (&mme_config);

  mbms_ref = m2ap_is_mbms_mce_m2ap_id_in_list(mce_mbms_m2ap_id); /**< Nothing eNB specific. */
  if (!mbms_ref) {
    /** No MBMS Reference found, just ignore the message and return. */
	OAILOG_ERROR (LOG_M2AP, "No MBMS Service Description with for MCE MBMS M2AP ID " MCE_MBMS_M2AP_ID_FMT " existing. Cannot update. \n", mce_mbms_m2ap_id);
	OAILOG_FUNC_OUT(LOG_M2AP);
  }

  /**
   * Check if the MBMS Service Area has been changed.
   * If eNB in list, which don't support new MBMS Service Area ID --> STOP!
   * If so, check if the current eNBs in this list also support the new service area --> UPDATE!
   * If eNB in list, which support new MBMS Service Area ID --> UPDATE!
   * If new eNBs outside list, which Support new MBMS Service Area ID --> START --> Will be added into the list later --> LOCK THE LIST!!!
   */
  int num_m2ap_enbs_to_be_updated = 0;
  /** Get a List of eNBs, which support the new MBMS Service Area ID. */
  m2ap_is_mbms_sai_in_list(mbms_ref->mbms_service_area_id, &num_m2ap_enbs_new_mbms_sai, (m2ap_enb_description_t **)&new_mbms_sai_m2ap_enb_p_elements);
  if(!num_m2ap_enbs_new_mbms_sai){
	  OAILOG_ERROR (LOG_M2AP, "No M2AP eNBs could be found for the new MBMS SA " MBMS_SERVICE_AREA_ID_FMT" for the MBMS Service with TMGI " TMGI_FMT". Removing the MBMS Description Reference. \n",
			  mbms_ref->mbms_service_area_id, TMGI_ARG(&mbms_ref->tmgi));
	  /**
	   * Destroy the MBMS Service SCTP hashmap, which should also remove all associations (hashtable_free).
	   */
	  hashtable_ts_destroy(&mbms_ref->g_m2ap_assoc_id2mce_enb_id_coll);
	  // todo: when inserting a new eNB, check if it exists, if not re-create!
	  OAILOG_FUNC_OUT (LOG_M2AP);
  }

  hashtable_key_array_t * current_mbms_sai_m2ap_enb_sctp_ids = hashtable_ts_get_keys (&mbms_ref->g_m2ap_assoc_id2mce_enb_id_coll);
  // todo: dealloc
  sctp_assoc_id_t 		* m2ap_enb_assoc_to_be_updated[current_mbms_sai_m2ap_enb_sctp_ids->num_keys];
  memset(&m2ap_enb_assoc_to_be_updated, 0, (sizeof(sctp_assoc_id_t*) * current_mbms_sai_m2ap_enb_sctp_ids->num_keys));

  /** We have some eNBs. Iterate through the current eNBs and remove all M2AP eNB associations, not in this list. */
  if(current_mbms_sai_m2ap_enb_sctp_ids){
    for(int i = 0; i < current_mbms_sai_m2ap_enb_sctp_ids->num_keys; i++) {
	  /** Get the eNB from the SCTP association. */
	  m2ap_enb_description_t * m2ap_enb_ref = m2ap_is_enb_assoc_id_in_list(current_mbms_sai_m2ap_enb_sctp_ids->keys[i]);
	  if(m2ap_enb_ref) {
	    /** Updating number of MBMS Services of the MBMS eNB. */
		for(int mbms_num_sai = 0; mbms_num_sai < m2ap_enb_ref->mbms_sa_list.num_service_area; mbms_num_sai++){
		  if(m2ap_enb_ref->mbms_sa_list.serviceArea[mbms_num_sai] == mbms_ref->mbms_service_area_id){
			/** Leave the eNB association, continue to check other eNBS. */
			m2ap_enb_assoc_to_be_updated[num_m2ap_enbs_to_be_updated] = m2ap_enb_ref->sctp_assoc_id;
			num_m2ap_enbs_to_be_updated++;
			m2ap_enb_ref = NULL;
			break;
		  }
		}
		if(m2ap_enb_ref){
		//            /** New MBMS Service Area Index is found in the M2AP eNB. */
		  /** eNB is removed, send an MBMS Session Stop request to the eNB rightaway. */
		  m2ap_generate_mbms_session_stop_request(mbms_ref->mce_mbms_m2ap_id, m2ap_enb_description->sctp_assoc_id);
		  /** Remove the SCTP association here, since we need the eNB MBMS M2AP ID above */
		  hashtable_ts_free(&mbms_ref->g_m2ap_assoc_id2mce_enb_id_coll, current_mbms_sai_m2ap_enb_sctp_ids->keys[i]);
		}
	  } else {
	    /** Remove the SCTP association here, since we need the eNB MBMS M2AP ID above */
		hashtable_ts_free(&mbms_ref->g_m2ap_assoc_id2mce_enb_id_coll, current_mbms_sai_m2ap_enb_sctp_ids->keys[i]);
	  }
    }
  }

  /**
   * Update and Start the Remaining eNBs.
   * Check if there is a timer for update, if so, re-enter this method again, when the timer runs out.
   * We already removed the unnecessary associations. The MBMS Service ID will be updated. Timer will have MBMS M2AP ID argument.
   * It should check from the eNB MBMS M2AP Id, that it is an update.
   */

  /**
   * All remaining eNBs in the list need to be updated with the updated MBMS Service values, including the new MBMS Service Area Id.
   * We don't touch the SCTP associations.
   */
  for(int i = 0; i < num_m2ap_enbs_to_be_updated; i++) {
    m2ap_enb_description_t * m2ap_enb_ref = m2ap_is_enb_assoc_id_in_list(*(m2ap_enb_assoc_to_be_updated[i]));
    DevAssert(m2ap_enb_ref); /**< Should be checked above!. */
    /**
     * Send an MBMS Session Update Request to the eNB.
     * MBMS Service Description is already updated.
     */
    int rc = m2ap_generate_mbms_session_update_request(mbms_ref->mce_mbms_m2ap_id, *(m2ap_enb_assoc_to_be_updated[i]));
    if(rc != RETURNok) {
	  OAILOG_ERROR(LOG_M2AP, "Error updating M2AP eNB with SCTP Assoc ID (%d) for the updated MBMS Service with TMGI " TMGI_FMT". "
		"Removing the association.\n", *(m2ap_enb_assoc_to_be_updated[i]), TMGI_ARG(&mbms_ref->tmgi));
	  /** Remove the hash key, which should also update the eNB. */
	  hashtable_ts_free(&mbms_ref->g_m2ap_assoc_id2mce_enb_id_coll, (const hash_key_t)*(m2ap_enb_assoc_to_be_updated[i]));
	  OAILOG_ERROR(LOG_M2AP, "Removed association after erroneous update.\n");
    }
    /** Successfully updated MBMS Service. SCTP association stays in the MBMS service. */
  }
  /** Finally, check for new eNB, not in the list of MBMS. */
  for(int i = 0; i < num_m2ap_enbs_new_mbms_sai; i++){
	m2ap_enb_description_t * target_enb_ref = new_mbms_sai_m2ap_enb_p_elements[i];
	/** Check if the new eNB has an eNB association in the MBMS Service, if not, send an MBMS Session Start Request. */
	enb_mbms_m2ap_id_t *enb_mbms_m2ap_id = NULL;
	hashtable_ts_get (&mbms_ref->g_m2ap_assoc_id2mce_enb_id_coll, (const hash_key_t)target_enb_ref->sctp_assoc_id, (void **)&enb_mbms_m2ap_id);
	if(!enb_mbms_m2ap_id) {
	  /**
	   * Trigger a new MBMS Session Start Request towards the new eNB.
	   * It will be added to the list and the eNB MBMS associations will be updated with the MBMS Session Start Response.
	   */
	  m2ap_generate_mbms_session_start_request(mbms_ref->mce_mbms_m2ap_id, num_m2ap_enbs_new_mbms_sai, new_mbms_sai_m2ap_enb_p_elements);
	}
  }
  /** Free the keys. */
  if(current_mbms_sai_m2ap_enb_sctp_ids) {
    free_wrapper(&current_mbms_sai_m2ap_enb_sctp_ids->keys);
    free_wrapper(&current_mbms_sai_m2ap_enb_sctp_ids);
  }
  OAILOG_FUNC_OUT(LOG_M2AP);
}

//------------------------------------------------------------------------------
static
int m2ap_generate_mbms_session_start_request(mce_mbms_m2ap_id_t mce_mbms_m2ap_id, const uint8_t num_m2ap_enbs, m2ap_enb_description_t ** m2ap_enb_descriptions)
{
  OAILOG_FUNC_IN (LOG_M2AP);
  mbms_description_t                     *mbms_ref = NULL;
  uint8_t                                *buffer_p = NULL;
  uint32_t                                length = 0;
  MessagesIds                             message_id = MESSAGES_ID_MAX;
  void                                   *id = NULL;
  M2AP_M2AP_PDU_t                         pdu = {0};
  M2AP_SessionStartRequest_t			 *out;
  M2AP_SessionStartRequest_Ies_t		 *ie = NULL;

  mbms_ref = m2ap_is_mbms_mce_m2ap_id_in_list(mce_mbms_m2ap_id);
  if (!mbms_ref) {
	OAILOG_ERROR (LOG_M2AP, "No MBMS MCE M2AP ID " MCE_MBMS_M2AP_ID_FMT". Cannot generate MBMS Session Start Request. \n", mce_mbms_m2ap_id);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }

  /**
   * Get all M2AP eNBs which match the MBMS Service Area ID and transmit to all of them.
   * We don't encode ENB MBMS M2AP ID, so we can reuse the encoded message.
   */
  if(!num_m2ap_enbs || !*m2ap_enb_descriptions){
	OAILOG_ERROR (LOG_M2AP, "No M2AP eNBs given for the received MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT " for MBMS Service " MCE_MBMS_M2AP_ID_FMT". \n",
	  mbms_ref->mbms_service_area_id, mce_mbms_m2ap_id);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }

  /*
   * We have found the UE in the list.
   * Create new IE list message and encode it.
   */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = M2AP_ProcedureCode_id_sessionStart;
  pdu.choice.initiatingMessage.criticality = M2AP_Criticality_ignore;
  pdu.choice.initiatingMessage.value.present = M2AP_InitiatingMessage__value_PR_SessionStartRequest;
  out = &pdu.choice.initiatingMessage.value.choice.SessionStartRequest;

  /*
   * mandatory
   * Setting MBMS informations with the ones found in ue_ref
   */
  ie = (M2AP_SessionStartRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionStartRequest_Ies_t));
  ie->id = M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID;
  ie->criticality = M2AP_Criticality_reject;
  ie->value.present = M2AP_SessionStartRequest_Ies__value_PR_MCE_MBMS_M2AP_ID;
  ie->value.choice.MCE_MBMS_M2AP_ID = mce_mbms_m2ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (M2AP_SessionStartRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionStartRequest_Ies_t));
  ie->id = M2AP_ProtocolIE_ID_id_TMGI;
  ie->criticality = M2AP_Criticality_reject;
  ie->value.present = M2AP_SessionStartRequest_Ies__value_PR_TMGI;
  INT24_TO_OCTET_STRING(mbms_ref->tmgi.mbms_service_id, &ie->value.choice.TMGI.serviceID);
  TBCD_TO_PLMN_T(&ie->value.choice.TMGI.pLMNidentity, &mbms_ref->tmgi.plmn);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /** No MBMS Session Id since we only support GCS-AS (public safety). */

  /* mandatory
   * Only a single MBMS Service Area Id per MBMS Service is supported right now.
   */
  ie = (M2AP_SessionStartRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionStartRequest_Ies_t));
  ie->id = M2AP_ProtocolIE_ID_id_TMGI;
  ie->criticality = M2AP_Criticality_reject;
  ie->value.present = M2AP_SessionStartRequest_Ies__value_PR_MBMS_Service_Area;
  uint32_t mbms_sai = mbms_ref->mbms_service_area_id | (1 << 16); /**< Add the length into the encoded value. */
  INT24_TO_OCTET_STRING(mbms_sai, &ie->value.choice.MBMS_Service_Area);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory
   * Add the Downlink Tunnel Information
   */
  ie = (M2AP_SessionStartRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionStartRequest_Ies_t));
  ie->id = M2AP_ProtocolIE_ID_id_TNL_Information;
  ie->criticality = M2AP_Criticality_reject;
  ie->value.present = M2AP_SessionStartRequest_Ies__value_PR_TNL_Information;
  INT32_TO_OCTET_STRING (mbms_ref->mbms_bc.mbms_ip_mc_distribution.cteid, &ie->value.choice.TNL_Information.gTP_TEID);
  /** Distribution Address. */
  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.pdn_type == IPv4 ) {
    ie->value.choice.TNL_Information.iPMCAddress.buf = calloc(4, sizeof(uint8_t));
	IN_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.address.ipv4_address, ie->value.choice.TNL_Information.iPMCAddress.buf);
	ie->value.choice.TNL_Information.iPMCAddress.size = 4;
  } else  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.pdn_type == IPv6 )  {
    ie->value.choice.TNL_Information.iPMCAddress.buf = calloc(16, sizeof(uint8_t));
    IN6_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.address.ipv6_address, ie->value.choice.TNL_Information.iPMCAddress.buf);
    ie->value.choice.TNL_Information.iPMCAddress.size = 16;
  } else {
	DevMessage("INVALID PDN TYPE FOR IP MC DISTRIBUTION " + mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.pdn_type);
  }
  /** Source Address. */
  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.pdn_type == IPv4 ) {
    ie->value.choice.TNL_Information.iPSourceAddress.buf = calloc(4, sizeof(uint8_t));
	IN_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.address.ipv4_address, ie->value.choice.TNL_Information.iPSourceAddress.buf);
	ie->value.choice.TNL_Information.iPSourceAddress.size = 4;
  } else  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.pdn_type == IPv6 )  {
    ie->value.choice.TNL_Information.iPSourceAddress.buf = calloc(16, sizeof(uint8_t));
    IN6_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.address.ipv6_address, ie->value.choice.TNL_Information.iPSourceAddress.buf);
    ie->value.choice.TNL_Information.iPSourceAddress.size = 16;
  } else {
	DevMessage("INVALID PDN TYPE FOR IP MC SOURCE " + mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.pdn_type);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // todo: qos, sctpm, scheduling..
  if (m2ap_mme_encode_pdu (&pdu, &buffer_p, &length) < 0) {
	OAILOG_ERROR (LOG_M2AP, "Failed to encode MBMS Session Start Request. \n");
	OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }

  // todo: the next_sctp_stream is the one without incrementation?
  for(int i = 0; i < num_m2ap_enbs; i++){
	  m2ap_enb_description_t * target_enb_ref = m2ap_enb_descriptions[i];
	  if(target_enb_ref){
		/** Check if it is in the list of the MBMS Service. */
		bstring b = blk2bstr(buffer_p, length);
		free(buffer_p);
		OAILOG_NOTICE (LOG_M2AP, "Send M2AP_MBMS_SESSION_START_REQUEST message MCE_MBMS_M2AP_ID = " MCE_MBMS_M2AP_ID_FMT "\n", mce_mbms_m2ap_id);
		/** For the sake of complexity (we wan't to keep it simple, and the same ), we are using the same SCTP Stream Id for all MBMS Service Index. */
		m2ap_mce_itti_send_sctp_request(&b, target_enb_ref->sctp_assoc_id, MBMS_SERVICE_SCTP_STREAM_ID, mce_mbms_m2ap_id);
	  }
  }

  OAILOG_FUNC_RETURN (LOG_M2AP, RETURNok);
}

//------------------------------------------------------------------------------
static
int m2ap_generate_mbms_session_update_request(mce_mbms_m2ap_id_t mce_mbms_m2ap_id, sctp_assoc_id_t sctp_assoc_id)
{
  OAILOG_FUNC_IN (LOG_M2AP);
  mbms_description_t                     *mbms_ref = NULL;
  m2ap_enb_description_t				 *m2ap_enb_description = NULL;
  uint8_t                                *buffer_p = NULL;
  uint32_t                                length = 0;
  enb_mbms_m2ap_id_t					  enb_mbms_m2ap_id = INVALID_ENB_MBMS_M2AP_ID;
  MessagesIds                             message_id = MESSAGES_ID_MAX;
  void                                   *id = NULL;
  M2AP_M2AP_PDU_t                         pdu = {0};
  M2AP_SessionUpdateRequest_t		 	 *out;
  M2AP_SessionUpdateRequest_Ies_t		 *ie = NULL;

  mbms_ref = m2ap_is_mbms_mce_m2ap_id_in_list(mce_mbms_m2ap_id);
  if (!mbms_ref) {
	OAILOG_ERROR (LOG_M2AP, "No MCE MBMS M2AP ID " MCE_MBMS_M2AP_ID_FMT". Cannot generate MBMS Session Update Request. \n", mce_mbms_m2ap_id);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }

  m2ap_enb_description = m2ap_is_enb_assoc_id_in_list(sctp_assoc_id);
  if (!m2ap_enb_description) {
	OAILOG_ERROR (LOG_M2AP, "No M2AP eNB description for SCTP Assoc Id (%d). Cannot trigger MBMS Session Update Request for MBMS Service with MCE MBMS M2AP ID " MCE_MBMS_M2AP_ID_FMT". \n", sctp_assoc_id, mce_mbms_m2ap_id);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }

  /** Check that an eNB-MBMS-ID exists. */
  hashtable_ts_get (&mbms_ref->g_m2ap_assoc_id2mce_enb_id_coll, (const hash_key_t)sctp_assoc_id, (void **)&enb_mbms_m2ap_id);
  if(enb_mbms_m2ap_id == INVALID_ENB_MBMS_M2AP_ID){
	OAILOG_ERROR (LOG_M2AP, "No ENB MBMS M2AP ID could be retrieved. Cannot generate MBMS Session Update Request. \n");
	OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }
  /*
   * We have found the UE in the list.
   * Create new IE list message and encode it.
   */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = M2AP_ProcedureCode_id_sessionUpdate;
  pdu.choice.initiatingMessage.criticality = M2AP_Criticality_ignore;
  pdu.choice.initiatingMessage.value.present = M2AP_InitiatingMessage__value_PR_SessionUpdateRequest;
  out = &pdu.choice.initiatingMessage.value.choice.SessionUpdateRequest;

  /*
   * Setting MBMS informations with the ones found in ue_ref
   */
  /* mandatory */
  ie = (M2AP_SessionUpdateRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionUpdateRequest_Ies_t));
  ie->id = M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID;
  ie->criticality = M2AP_Criticality_reject;
  ie->value.present = M2AP_SessionUpdateRequest_Ies__value_PR_MCE_MBMS_M2AP_ID;
  ie->value.choice.MCE_MBMS_M2AP_ID = mce_mbms_m2ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (M2AP_SessionUpdateRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionUpdateRequest_Ies_t));
  ie->id = M2AP_ProtocolIE_ID_id_ENB_MBMS_M2AP_ID;
  ie->criticality = M2AP_Criticality_reject;
  ie->value.present = M2AP_SessionUpdateRequest_Ies__value_PR_ENB_MBMS_M2AP_ID;
  ie->value.choice.ENB_MBMS_M2AP_ID = enb_mbms_m2ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (M2AP_SessionUpdateRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionUpdateRequest_Ies_t));
  ie->id = M2AP_ProtocolIE_ID_id_TMGI;
  ie->criticality = M2AP_Criticality_reject;
  ie->value.present = M2AP_SessionUpdateRequest_Ies__value_PR_TMGI;
  INT24_TO_OCTET_STRING(mbms_ref->tmgi.mbms_service_id, &ie->value.choice.TMGI.serviceID);
  TBCD_TO_PLMN_T(&ie->value.choice.TMGI.pLMNidentity, &mbms_ref->tmgi.plmn);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /** No MBMS Session Id since we only support GCS-AS (public safety). */

  /* mandatory
   * Only a single MBMS Service Area Id per MBMS Service is supported right now.
   */
  ie = (M2AP_SessionUpdateRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionUpdateRequest_Ies_t));
  ie->id = M2AP_ProtocolIE_ID_id_TMGI;
  ie->criticality = M2AP_Criticality_reject;
  ie->value.present = M2AP_SessionUpdateRequest_Ies__value_PR_MBMS_Service_Area;
  uint32_t mbms_sai = mbms_ref->mbms_service_area_id | (1 << 16); /**< Add the length into the encoded value. */
  INT24_TO_OCTET_STRING(mbms_sai, &ie->value.choice.MBMS_Service_Area);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory
   * Add the Downlink Tunnel Information
   */
  ie = (M2AP_SessionUpdateRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionUpdateRequest_Ies_t));
  ie->id = M2AP_ProtocolIE_ID_id_TNL_Information;
  ie->criticality = M2AP_Criticality_reject;
  ie->value.present = M2AP_SessionUpdateRequest_Ies__value_PR_TNL_Information;
  INT32_TO_OCTET_STRING (mbms_ref->mbms_bc.mbms_ip_mc_distribution.cteid, &ie->value.choice.TNL_Information.gTP_TEID);
  /** Distribution Address. */
  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.pdn_type == IPv4 ) {
    ie->value.choice.TNL_Information.iPMCAddress.buf = calloc(4, sizeof(uint8_t));
	IN_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.address.ipv4_address, ie->value.choice.TNL_Information.iPMCAddress.buf);
	ie->value.choice.TNL_Information.iPMCAddress.size = 4;
  } else  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.pdn_type == IPv6 )  {
    ie->value.choice.TNL_Information.iPMCAddress.buf = calloc(16, sizeof(uint8_t));
    IN6_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.address.ipv6_address, ie->value.choice.TNL_Information.iPMCAddress.buf);
    ie->value.choice.TNL_Information.iPMCAddress.size = 16;
  } else {
	DevMessage("INVALID PDN TYPE FOR IP MC DISTRIBUTION " + mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.pdn_type);
  }
  /** Source Address. */
  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.pdn_type == IPv4 ) {
    ie->value.choice.TNL_Information.iPSourceAddress.buf = calloc(4, sizeof(uint8_t));
	IN_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.address.ipv4_address, ie->value.choice.TNL_Information.iPSourceAddress.buf);
	ie->value.choice.TNL_Information.iPSourceAddress.size = 4;
  } else  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.pdn_type == IPv6 )  {
    ie->value.choice.TNL_Information.iPSourceAddress.buf = calloc(16, sizeof(uint8_t));
    IN6_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.address.ipv6_address, ie->value.choice.TNL_Information.iPSourceAddress.buf);
    ie->value.choice.TNL_Information.iPSourceAddress.size = 16;
  } else {
	DevMessage("INVALID PDN TYPE FOR IP MC SOURCE " + mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.pdn_type);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // todo: qos, sctpm, scheduling..
  if (m2ap_mme_encode_pdu (&pdu, &buffer_p, &length) < 0) {
	// TODO: handle something
	OAILOG_ERROR (LOG_M2AP, "Failed to encode MBMS Session Update Request. \n");
	OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }

  bstring b = blk2bstr(buffer_p, length);
  free(buffer_p);
  OAILOG_NOTICE (LOG_M2AP, "Send M2AP_MBMS_SESSION_UPDATE_REQUEST message MCE_MBMS_M2AP_ID = " MCE_MBMS_M2AP_ID_FMT "\n", mce_mbms_m2ap_id);
  m2ap_mce_itti_send_sctp_request(&b, m2ap_enb_description->sctp_assoc_id, MBMS_SERVICE_SCTP_STREAM_ID, mce_mbms_m2ap_id);
  OAILOG_FUNC_RETURN (LOG_M2AP, RETURNok);
}

//------------------------------------------------------------------------------
static
int m2ap_generate_mbms_session_stop_request(mce_mbms_m2ap_id_t mce_mbms_m2ap_id, sctp_assoc_id_t sctp_assoc_id)
{
  OAILOG_FUNC_IN (LOG_M2AP);
  mbms_description_t                     *mbms_ref = NULL;
  m2ap_enb_description_t				 *m2ap_enb_description = NULL;
  uint8_t                                *buffer_p = NULL;
  uint32_t                                length = 0;
  enb_mbms_m2ap_id_t					  enb_mbms_m2ap_id = INVALID_ENB_MBMS_M2AP_ID;
  MessagesIds                             message_id = MESSAGES_ID_MAX;
  void                                   *id = NULL;
  M2AP_M2AP_PDU_t                         pdu = {0};
  M2AP_SessionStopRequest_t			 	 *out;
  M2AP_SessionStopRequest_Ies_t		 	 *ie = NULL;

  mbms_ref = m2ap_is_mbms_mce_m2ap_id_in_list(mce_mbms_m2ap_id);
  if (!mbms_ref) {
	OAILOG_ERROR (LOG_M2AP, "No MCE MBMS M2AP ID " MCE_MBMS_M2AP_ID_FMT". Cannot generate MBMS Session Stop Request. \n", mce_mbms_m2ap_id);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }

  m2ap_enb_description = m2ap_is_enb_assoc_id_in_list(sctp_assoc_id);
  if (!m2ap_enb_description) {
	OAILOG_ERROR (LOG_M2AP, "No M2AP eNB description for SCTP Assoc Id (%d). Cannot trigger MBMS Session Stop Request for MBMS Service with MCE MBMS M2AP ID " MCE_MBMS_M2AP_ID_FMT". \n",
			sctp_assoc_id, mce_mbms_m2ap_id);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }

  /** Check that an eNB-MBMS-ID exists. */
  hashtable_ts_get (&mbms_ref->g_m2ap_assoc_id2mce_enb_id_coll, (const hash_key_t)sctp_assoc_id, (void **)&enb_mbms_m2ap_id);
  if(enb_mbms_m2ap_id == INVALID_ENB_MBMS_M2AP_ID){
	OAILOG_ERROR (LOG_M2AP, "No ENB MBMS M2AP ID could be retrieved. Cannot generate MBMS Session Stop Request. \n", enb_mbms_m2ap_id);
	OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }

  /*
   * We have found the UE in the list.
   * Create new IE list message and encode it.
   */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = M2AP_ProcedureCode_id_sessionStop;
  pdu.choice.initiatingMessage.criticality = M2AP_Criticality_ignore;
  pdu.choice.initiatingMessage.value.present = M2AP_InitiatingMessage__value_PR_SessionStopRequest;
  out = &pdu.choice.initiatingMessage.value.choice.SessionStopRequest;

  /*
   * Setting MBMS informations with the ones found in ue_ref
   */
  /* mandatory */
  ie = (M2AP_SessionStopRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionStopRequest_Ies_t));
  ie->id = M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID;
  ie->criticality = M2AP_Criticality_reject;
  ie->value.present = M2AP_SessionStopRequest_Ies__value_PR_MCE_MBMS_M2AP_ID;
  ie->value.choice.MCE_MBMS_M2AP_ID = mce_mbms_m2ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (M2AP_SessionStopRequest_Ies_t *)calloc(1, sizeof(M2AP_SessionStopRequest_Ies_t));
  ie->id = M2AP_ProtocolIE_ID_id_ENB_MBMS_M2AP_ID;
  ie->criticality = M2AP_Criticality_reject;
  ie->value.present = M2AP_SessionStopRequest_Ies__value_PR_ENB_MBMS_M2AP_ID;
  ie->value.choice.ENB_MBMS_M2AP_ID = enb_mbms_m2ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // todo: qos, sctpm, scheduling..
  if (m2ap_mme_encode_pdu (&pdu, &buffer_p, &length) < 0) {
	// TODO: handle something
	OAILOG_ERROR (LOG_M2AP, "Failed to encode MBMS Session Stop Request. \n");
	OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }

  bstring b = blk2bstr(buffer_p, length);
  free(buffer_p);
  OAILOG_NOTICE (LOG_M2AP, "Send M2AP_MBMS_SESSION_STOP_REQUEST message MCE_MBMS_M2AP_ID = " MCE_MBMS_M2AP_ID_FMT "\n", mce_mbms_m2ap_id);
  m2ap_mce_itti_send_sctp_request(&b, m2ap_enb_description->sctp_assoc_id, MBMS_SERVICE_SCTP_STREAM_ID, mce_mbms_m2ap_id);
  OAILOG_FUNC_RETURN (LOG_M2AP, RETURNok);
}


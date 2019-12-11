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
#include "common_types_mbms.h"
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

static void m2ap_update_mbms_service_context(const mce_mbms_m2ap_id_t mce_mbms_m2ap_id);
static int m2ap_generate_mbms_session_start_request(mce_mbms_m2ap_id_t mbms_m2ap_id, const uint8_t num_m2ap_enbs, m2ap_enb_description_t ** m2ap_enb_descriptions);
static int m2ap_generate_mbms_session_update_request(mce_mbms_m2ap_id_t mce_mbms_m2ap_id, sctp_assoc_id_t sctp_assoc_id);
static int m2ap_mbms_scheduling_cluster(const uint8_t num_m2_enb_mbms_area, const m2ap_enb_description_t** const m2ap_enb_p_elements, const uint8_t num_mbms_area, const mbsfn_areas_t * const mbsfn_cluster_global, const mbsfn_areas_t * const mbsfn_cluster_local, const long mcch_rep_abs_rf);
static int m2ap_generate_mbms_scheduling_information(const m2ap_enb_description_t * m2_enb_description, mbsfn_area_cfg_t **mbsfn_area_cfg, const uint8_t num_mbsfn_area_per_enb, long mcch_rep_abs_rf);
//------------------------------------------------------------------------------
void
m2ap_handle_mbms_session_start_request (
  const itti_m3ap_mbms_session_start_req_t * const mbms_session_start_req_pP)
{
  /*
   * MBMS-GW triggers a new MBMS Service. No eNBs are specified but only the MBMS service area.
   * MCE_APP will not be eNB specific. eNB specific messages will be handled in the M2AP_APP.
   * That a single eNB fails to broadcast, is not of importance to the MCE_APP.
   * This message initiates for all eNBs in the given MBMS Service are a new MBMS Bearer Service.
   */
  uint8_t                              *buffer_p = NULL;
  uint32_t                              length = 0;
  uint8_t								  							num_m2ap_enbs = 0;
  mbms_description_t               		 *mbms_ref = NULL;
  mce_mbms_m2ap_id_t 					  				mce_mbms_m2ap_id 	= INVALID_MCE_MBMS_M2AP_ID;
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
    OAILOG_ERROR (LOG_M2AP, "An MBMS Service Description with for TMGI " TMGI_FMT " and MBMS_Service_Area ID " MBMS_SERVICE_AREA_ID_FMT "already exists. Removing implicitly. \n",
        TMGI_ARG(&mbms_session_start_req_pP->tmgi), mbms_session_start_req_pP->mbms_service_area_id);
    /**
     * Trigger a session stop and inform the eNBs.
     * MBMS Services should only be removed over this function.
     * We will update the eNB statistics also.
     */
    m2ap_handle_mbms_session_stop_request(&mbms_ref->tmgi, mbms_ref->mbms_service_area_id, true);
  }
  /** Check that there exists at least a single eNB with the MBMS Service Area (we don't start MBMS sessions for eNBs which later on connected). */
  mme_config_read_lock (&mme_config);
  m2ap_enb_description_t *			         m2ap_enb_p_elements[mme_config.mbms.max_m2_enbs];
  memset(m2ap_enb_p_elements, 0, (sizeof(m2ap_enb_description_t*) * mme_config.mbms.max_m2_enbs));
  mme_config_unlock (&mme_config);
  m2ap_is_mbms_sai_in_list(mbms_session_start_req_pP->mbms_service_area_id, &num_m2ap_enbs, (m2ap_enb_description_t **)m2ap_enb_p_elements);
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
  /**
   * Update the created MBMS Service Description.
   * Directly set the values and don't wait for a response, we will set these values into the eNB, once the timer runs out.
   * We don't need the MBMS Session Duration. It will be handled in the MCE_APP layer.
   */
  memcpy((void*)&mbms_ref->mbms_bc.mbms_ip_mc_distribution,  (void*)&mbms_session_start_req_pP->mbms_bearer_tbc.mbms_ip_mc_dist, sizeof(mbms_ip_multicast_distribution_t));
  memcpy((void*)&mbms_ref->mbms_bc.eps_bearer_context.bearer_level_qos, (void*)&mbms_session_start_req_pP->mbms_bearer_tbc.bc_tbc.bearer_level_qos, sizeof(bearer_qos_t));
  /**
   * Check if a timer has been given, if so start the timer.
   * If not send immediately. MBMS Session start will be based on the absolute time and not on the MCCH modification/repetition periods.
   * They will be handled separately/independently.
   */
  if(mbms_session_start_req_pP->abs_start_time_sec){
  	uint32_t delta_to_start_sec = mbms_session_start_req_pP->abs_start_time_sec - time(NULL);
  	OAILOG_INFO (LOG_M2AP, "M2AP:MBMS Session Start Request- Received a timer of (%d)s for start of TMGI " TMGI_FMT " and MBMS Service Area ID "MBMS_SERVICE_AREA_ID_FMT". \n",
  			delta_to_start_sec, TMGI_ARG(&mbms_session_start_req_pP->tmgi), mbms_session_start_req_pP->mbms_service_area_id);
  	// todo: calculate delta for usec
  	if (timer_setup (delta_to_start_sec, mbms_session_start_req_pP->abs_start_time_usec,
           TASK_M2AP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void*)mce_mbms_m2ap_id, &(mbms_ref->m2ap_action_timer.id)) < 0) {
      OAILOG_ERROR (LOG_M2AP, "Failed to start MBMS Session Start timer for MBMS with MBMS M2AP MCE ID " MCE_MBMS_M2AP_ID_FMT". \n", mce_mbms_m2ap_id);
      mbms_ref->m2ap_action_timer.id = M2AP_TIMER_INACTIVE_ID;
      /** Send immediately. */
    } else {
      OAILOG_DEBUG (LOG_M2AP, "Started M2AP MBMS Session start timer (timer id 0x%lx) for MBMS Session MBMS M2AP MCE ID " MCE_MBMS_M2AP_ID_FMT". "
    		  "Waiting for timeout to trigger M2AP Session Start to M2AP eNBs.\n", mbms_ref->m2ap_action_timer.id, mce_mbms_m2ap_id);
      /** Leave the method. */
      OAILOG_FUNC_OUT(LOG_M2AP);
    }
  }
  /**
   * Try to send the MBMS Session Start requests.
   * If it cannot be sent, we still keep the MBMS Service Description in the map.
   * The SCTP associations will only be removed with a successful response.
   */
  m2ap_generate_mbms_session_start_request(mce_mbms_m2ap_id, num_m2ap_enbs, &m2ap_enb_p_elements);
  OAILOG_FUNC_OUT(LOG_M2AP);
}

//------------------------------------------------------------------------------
void
m2ap_handle_mbms_session_update_request (
  const itti_m3ap_mbms_session_update_req_t * const mbms_session_update_req_pP)
{
  /*
   * MBMS-GW triggers the stop of an MBMS Service on all the eNBs which are receiving it.
   */
  uint8_t                *buffer_p 					 = NULL;
  uint32_t                length 					 = 0;
  mbms_description_t     *mbms_ref 					 = NULL;
  M2AP_M2AP_PDU_t         pdu 						 = {0};
  int 									  num_m2ap_enbs_new_mbms_sai = 0;

  OAILOG_FUNC_IN (LOG_M2AP);
  DevAssert (mbms_session_update_req_pP != NULL);
  /*
   * We need to check the MBMS Service via TMGI and MBMS Service Index.
   */
  mbms_ref = m2ap_is_mbms_tmgi_in_list(&mbms_session_update_req_pP->tmgi, mbms_session_update_req_pP->old_mbms_service_area_id); /**< Nothing eNB specific. */
  if (!mbms_ref) {
    /** No MBMS Reference found, just ignore the message and return. */
  	OAILOG_ERROR (LOG_M2AP, "No MBMS Service Description with for TMGI " TMGI_FMT " and MBMS_Service_Area ID " MBMS_SERVICE_AREA_ID_FMT " already exists. \n",
  			TMGI_ARG(&mbms_session_update_req_pP->tmgi), mbms_session_update_req_pP->old_mbms_service_area_id);
  	OAILOG_FUNC_OUT(LOG_M2AP);
  }

  /**
   * Cleared the MBMS service from eNBs with unmatching MBMS Service Area Id.
   * We can continue with the update of the MBMS service.
   */
  memcpy((void*)&mbms_ref->mbms_bc.mbms_ip_mc_distribution,  (void*)&mbms_session_update_req_pP->mbms_bearer_tbc.mbms_ip_mc_dist, sizeof(mbms_ip_multicast_distribution_t));
  memcpy((void*)&mbms_ref->mbms_bc.eps_bearer_context.bearer_level_qos, (void*)&mbms_session_update_req_pP->mbms_bearer_tbc.bc_tbc.bearer_level_qos, sizeof(bearer_qos_t));

  /**
   * Before the timer, check if the new MBMS Service Area Id is served by any eNB.
   * If not, directly remove the MBMS description.
   */
  mme_config_read_lock (&mme_config);
  m2ap_enb_description_t *			         m2ap_enb_p_elements[mme_config.mbms.max_m2_enbs];
  memset(m2ap_enb_p_elements, 0, (sizeof(m2ap_enb_description_t*) * mme_config.mbms.max_m2_enbs));
  mme_config_unlock (&mme_config);
  m2ap_is_mbms_sai_in_list(mbms_session_update_req_pP->new_mbms_service_area_id, &num_m2ap_enbs_new_mbms_sai,
		  (m2ap_enb_description_t **)m2ap_enb_p_elements);
  if(!num_m2ap_enbs_new_mbms_sai){
    OAILOG_ERROR (LOG_M2AP, "No M2AP eNBs could be found for the MBMS SAI " MBMS_SERVICE_AREA_ID_FMT" for the MBMS Service with TMGI " TMGI_FMT". "
    		"Stopping the MBMS service. \n", mbms_session_update_req_pP->new_mbms_service_area_id, TMGI_ARG(&mbms_session_update_req_pP->tmgi));
    m2ap_handle_mbms_session_stop_request(&mbms_ref->tmgi, mbms_ref->mbms_service_area_id, true); /**< Should also remove the MBMS service. */
    OAILOG_FUNC_OUT (LOG_M2AP);
  }
  /**
   * Iterate through the current MBMS services. Check an current M2 eNBs, where the new MBMS service area id is NOT supported.
   * Send a session stop to all of them without removing the MBMS service.
   */

  mme_config_read_lock (&mme_config);
  memset(m2ap_enb_p_elements, 0, (sizeof(m2ap_enb_description_t*) * mme_config.mbms.max_m2_enbs));
  mme_config_unlock (&mme_config);
  int num_m2ap_enbs_missing_new_mbms_sai = 0;
  m2ap_is_mbms_sai_not_in_list(mbms_session_update_req_pP->new_mbms_service_area_id, &num_m2ap_enbs_missing_new_mbms_sai, (m2ap_enb_description_t **)&m2ap_enb_p_elements);
  if(num_m2ap_enbs_missing_new_mbms_sai){
	  OAILOG_ERROR (LOG_M2AP, "(%d) M2AP eNBs not supporting new MBMS SAI " MBMS_SERVICE_AREA_ID_FMT" for the MBMS Service with TMGI " TMGI_FMT". "
		"Stopping the MBMS session in the M2AP eNBs. \n", mbms_session_update_req_pP->new_mbms_service_area_id, TMGI_ARG(&mbms_session_update_req_pP->tmgi));
     /** Send an MBMS session stop and remove the association. */
     for(int i = 0; i < num_m2ap_enbs_missing_new_mbms_sai; i++){
       m2ap_generate_mbms_session_stop_request(mbms_ref->mce_mbms_m2ap_id, m2ap_enb_p_elements[i]->sctp_assoc_id);
       /** Remove the association and decrement the count. */
       m2ap_enb_p_elements[i]->nb_mbms_associated--; /**< We don't check for restart, since it is trigger due update. */
       hashtable_uint64_ts_free(&mbms_ref->g_m2ap_assoc_id2mce_enb_id_coll, m2ap_enb_p_elements[i]->sctp_assoc_id);
     }
  } else {
	  OAILOG_INFO(LOG_M2AP, "All existing eNBs of the MBMS Service " MCE_MBMS_M2AP_ID_FMT " support the new MBMS SA " MBMS_SERVICE_AREA_ID_FMT". \n",
    	mbms_ref->mce_mbms_m2ap_id, mbms_session_update_req_pP->new_mbms_service_area_id);
  }
  /** Done cleaning up old MBMS Service Area Id artifacts. Continue with the new MBMS Service Area Id. */
  mbms_ref->mbms_service_area_id = mbms_session_update_req_pP->new_mbms_service_area_id;

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
m2ap_handle_mbms_session_stop_request (
  const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_service_area_id, const bool inform_enbs)
{
  OAILOG_FUNC_IN (LOG_M2AP);

  /*
   * MBMS-GW triggers the stop of an MBMS Service on all the eNBs which are receiving it.
   */
  uint8_t                                *buffer_p = NULL;
  uint32_t                                length = 0;
  mbms_description_t               		 	 *mbms_ref = NULL;
  m2ap_enb_description_t                 *target_enb_ref = NULL;
  M2AP_M2AP_PDU_t                         pdu = {0};

  /*
   * We need to check the MBMS Service via TMGI and MBMS Service Index.
   */
  mbms_ref = m2ap_is_mbms_tmgi_in_list(tmgi, mbms_service_area_id); /**< Nothing eNB specific. */
  if (!mbms_ref) {
    /**
     * No MBMS Reference found, just ignore the message and return.
     */
  	OAILOG_ERROR (LOG_M2AP, "No MBMS Service Description with for TMGI " TMGI_FMT " and MBMS_Service_Area ID " MBMS_SERVICE_AREA_ID_FMT "already exists. \n",
  			TMGI_ARG(tmgi), mbms_service_area_id);
  	OAILOG_FUNC_OUT(LOG_M2AP);
  }

  if(inform_enbs) {
    /** Check that there exists at least a single eNB with the MBMS Service Area (we don't start MBMS sessions for eNBs which later on connected). */
  	mme_config_read_lock (&mme_config);
  	m2ap_enb_description_t *			         m2ap_enb_p_elements[mme_config.mbms.max_m2_enbs];
  	memset(m2ap_enb_p_elements, 0, (sizeof(m2ap_enb_description_t*) * mme_config.mbms.max_m2_enbs));
  	mme_config_unlock (&mme_config);
  	int num_m2ap_enbs = 0;
  	m2ap_is_mbms_sai_in_list(mbms_service_area_id, &num_m2ap_enbs, (m2ap_enb_description_t **)m2ap_enb_p_elements);
  	if(num_m2ap_enbs){
  		for(int i = 0; i < num_m2ap_enbs; i++){
  			m2ap_generate_mbms_session_stop_request(mbms_ref->mce_mbms_m2ap_id, m2ap_enb_p_elements[i]->sctp_assoc_id);
  		}
  	} else {
  		OAILOG_ERROR(LOG_M2AP, "No M2AP eNBs could be found for the MBMS SA " MBMS_SERVICE_AREA_ID_FMT" for the MBMS Service with TMGI " TMGI_FMT" to be removed. Removing implicitly. \n",
  				mbms_service_area_id, TMGI_ARG(tmgi));
  	}
  }

  /**
   * Remove the MBMS Service.
   * Should also remove all the sctp associations from the eNBs and decrement the eNB MBMS numbers.
   */
  hashtable_ts_free (&g_m2ap_mbms_coll, mbms_ref->mce_mbms_m2ap_id);
  OAILOG_FUNC_OUT(LOG_M2AP);
}

//------------------------------------------------------------------------------
int m2ap_generate_mbms_session_stop_request(mce_mbms_m2ap_id_t mce_mbms_m2ap_id, sctp_assoc_id_t sctp_assoc_id)
{
  OAILOG_FUNC_IN (LOG_M2AP);
  mbms_description_t             *mbms_ref = NULL;
  m2ap_enb_description_t				 *m2ap_enb_description = NULL;
  uint8_t                        *buffer_p = NULL;
  uint32_t                        length = 0;
  enb_mbms_m2ap_id_t					  	enb_mbms_m2ap_id = INVALID_ENB_MBMS_M2AP_ID;
  MessagesIds                     message_id = MESSAGES_ID_MAX;
  void                           *id = NULL;
  M2AP_M2AP_PDU_t                 pdu = {0};
  M2AP_SessionStopRequest_t			 *out;
  M2AP_SessionStopRequest_Ies_t	 *ie = NULL;

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
  hashtable_uint64_ts_get (&mbms_ref->g_m2ap_assoc_id2mce_enb_id_coll, (const hash_key_t)sctp_assoc_id, (void *)&enb_mbms_m2ap_id);
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
  if (m2ap_mce_encode_pdu (&pdu, &buffer_p, &length) < 0) {
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

//------------------------------------------------------------------------------
void
m2ap_mce_handle_mbms_action_timer_expiry (void *arg)
{
  MessageDef                             *message_p = NULL;
  OAILOG_FUNC_IN (LOG_M2AP);
  DevAssert (arg != NULL);

  mbms_description_t *mbms_ref_p  =        (mbms_description_t *)arg;

  mbms_ref_p->m2ap_action_timer.id = M2AP_TIMER_INACTIVE_ID;
  OAILOG_DEBUG (LOG_M2AP, "Expired- MBMS Action timer MCE MBMS M2AP " MCE_MBMS_M2AP_ID_FMT" . \n", mbms_ref_p->mce_mbms_m2ap_id);

  /**
   * If there are no associated eNBs, we need to start the MBMS service.
   * If there are some associated eNBs, we need to update the MBMS service.
   * No timer for MBMS Service stop.
   */
  if(!mbms_ref_p->g_m2ap_assoc_id2mce_enb_id_coll.num_elements) {
    OAILOG_DEBUG (LOG_M2AP, "Starting MBMS service with MCE MBMS M2AP " MCE_MBMS_M2AP_ID_FMT" . \n", mbms_ref_p->mce_mbms_m2ap_id);
    uint8_t								  num_m2ap_enbs = 0;
    /** Get the list of eNBs of the matching service area. */
    mme_config_read_lock (&mme_config);
    m2ap_enb_description_t *			         m2ap_enb_p_elements[mme_config.mbms.max_m2_enbs];
    memset(m2ap_enb_p_elements, 0, (sizeof(m2ap_enb_description_t*) * mme_config.mbms.max_m2_enbs));
    mme_config_unlock (&mme_config);

    m2ap_is_mbms_sai_in_list(mbms_ref_p->mbms_service_area_id, &num_m2ap_enbs, (m2ap_enb_description_t **)m2ap_enb_p_elements);
    if(!num_m2ap_enbs){
      OAILOG_ERROR (LOG_M2AP, "No M2AP eNBs could be found for the MBMS SA " MBMS_SERVICE_AREA_ID_FMT" for the MBMS Service with TMGI " TMGI_FMT" after timeout. Removing implicitly. \n",
    		  mbms_ref_p->mbms_service_area_id, TMGI_ARG(&mbms_ref_p->tmgi));
      hashtable_ts_free (&g_m2ap_mbms_coll, mbms_ref_p->mce_mbms_m2ap_id);
      OAILOG_FUNC_OUT (LOG_M2AP);
    }
    m2ap_generate_mbms_session_start_request(mbms_ref_p->mce_mbms_m2ap_id, num_m2ap_enbs, m2ap_enb_p_elements);
  } else {
    OAILOG_DEBUG (LOG_M2AP, "MBMS service with MCE MBMS M2AP " MCE_MBMS_M2AP_ID_FMT" has already some eNBs, updating it. \n", mbms_ref_p->mce_mbms_m2ap_id);
    m2ap_update_mbms_service_context(mbms_ref_p->mce_mbms_m2ap_id);
  }
  OAILOG_FUNC_OUT (LOG_M2AP);
}

//------------------------------------------------------------------------------
int m2ap_handle_m3ap_mbms_scheduling_info(itti_m3ap_mbms_scheduling_info_t * m3ap_mbms_scheduling_info){
	OAILOG_FUNC_IN(LOG_M2AP);
	int																		 rc = RETURNerror;
  /** Iterate through the MBSFN clusters. The M2AP eNB associations here should have an MBSFN Id. */
  for(int num_mbms_area = 0; num_mbms_area < (MME_CONFIG_MAX_LOCAL_MBMS_SERVICE_AREAS + 1); num_mbms_area++){
  	/** Take a cluster: Check if eNBs exist for the MBMS area. */
    /** Check if anything is scheduled for the MBMS area. */
    if(!m3ap_mbms_scheduling_info->mbsfn_cluster[num_mbms_area].num_mbsfn_areas){
    	OAILOG_DEBUG(LOG_MCE_APP, "No MBSFN areas scheduled for for MBMS area (%d). Skipping.. \n", num_mbms_areas);
    	continue;
    }
  	uint8_t num_m2_enb_mbms_area = 0;
  	mme_config_read_lock (&mme_config);
    m2ap_enb_description_t *			         m2ap_enb_p_elements[mme_config.mbms.max_m2_enbs];
    memset(m2ap_enb_p_elements, 0, (sizeof(m2ap_enb_description_t*) * mme_config.mbms.max_m2_enbs));
    uint8_t local_global_active = mme_config.mbms.mbms_global_mbsfn_area_per_local_group;
    mme_config_unlock (&mme_config);
    m2ap_is_mbms_area_list(num_mbms_area, &num_m2_enb_mbms_area, (m2ap_enb_description_t**)m2ap_enb_p_elements);
    if(!num_m2_enb_mbms_area){
    	OAILOG_WARNING(LOG_MCE_APP, "No M2 eNBs could be found for MBMS area (%d).\n", num_mbms_areas);
    	continue;
    }
    /**
     * We will only get eNBs with the matching local MBMS area.
     * Even if local-global flag is not set, we will not get eNBs which belong to a local area for (0).
     * Create the MBMS Scheduling message for the eNBs.
     */
    if(m2ap_mbms_scheduling_cluster(num_m2_enb_mbms_area, (m2ap_enb_description_t**)m2ap_enb_p_elements, num_mbms_area,
    		(num_mbms_area && local_global_active) ? NULL : &m3ap_mbms_scheduling_info->mbsfn_cluster[0],
    				num_mbms_area ? &m3ap_mbms_scheduling_info->mbsfn_cluster[num_mbms_area] : NULL,
				m3ap_mbms_scheduling_info->mcch_rep_abs_rf) == RETURNerror)
    {
    	DevMessage("Could not generate MBMS scheduling information for MBMS area " + num_mbms_area);
    }
    OAILOG_INFO(LOG_MCE_APP, "Successfully handled MBMS Scheduling for MBMS area (%d) for (%d) M2 eNBs.\n",
    		num_mbms_area, num_m2ap_enbs);
  }
  OAILOG_INFO(LOG_MCE_APP, "Successfully handled MBMS scheduling for all MBMS areas. \n");
  OAILOG_FUNC_RETURN(LOG_M2AP, RETURNok);
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

//------------------------------------------------------------------------------
static
void m2ap_update_mbms_service_context(const mce_mbms_m2ap_id_t mce_mbms_m2ap_id) {
  mbms_description_t               		 *mbms_ref 					 = NULL;
  int 									  num_m2ap_enbs_new_mbms_sai = 0;

  OAILOG_FUNC_IN (LOG_M2AP);

  /** Check that there exists at least a single eNB with the MBMS Service Area (we don't start MBMS sessions for eNBs which later on connected). */
  mme_config_read_lock (&mme_config);
  m2ap_enb_description_t *			         new_mbms_sai_m2ap_enb_p_elements[mme_config.mbms.max_m2_enbs];
  memset(new_mbms_sai_m2ap_enb_p_elements, 0, (sizeof(m2ap_enb_description_t*) * mme_config.mbms.max_m2_enbs));
  mme_config_unlock (&mme_config);

  mbms_ref = m2ap_is_mbms_mce_m2ap_id_in_list(mce_mbms_m2ap_id); /**< Nothing eNB specific. */
  if (!mbms_ref) {
    /** No MBMS Reference found, just ignore the message and return. */
	OAILOG_ERROR (LOG_M2AP, "No MBMS Service Description with for MCE MBMS M2AP ID " MCE_MBMS_M2AP_ID_FMT " existing. Cannot update. \n", mce_mbms_m2ap_id);
	OAILOG_FUNC_OUT(LOG_M2AP);
  }

  /**
   * Get the list of all eNBs.
   * If the eNB is not in the current list, send an update, else send a session start.
   * We should have removed (stopped the MBMS Session) in the eNBs, which don't support the MBMS Service Area ID before.
   */
  mme_config_read_lock (&mme_config);
  m2ap_enb_description_t *			         m2ap_enb_p_elements[mme_config.mbms.max_m2_enbs];
  memset(m2ap_enb_p_elements, 0, (sizeof(m2ap_enb_description_t*) * mme_config.mbms.max_m2_enbs));
  mme_config_unlock (&mme_config);
  m2ap_is_mbms_sai_in_list(mbms_ref->mbms_service_area_id, &num_m2ap_enbs_new_mbms_sai, (m2ap_enb_description_t **)m2ap_enb_p_elements);
  if(!num_m2ap_enbs_new_mbms_sai){
	OAILOG_ERROR (LOG_M2AP, "No M2AP eNBs could be found for the MBMS SAI " MBMS_SERVICE_AREA_ID_FMT" for the MBMS Service with TMGI " TMGI_FMT". "
		"Stopping the MBMS service. \n", mbms_ref->mbms_service_area_id, TMGI_ARG(&mbms_ref->tmgi));
	/** Change of the MBMS SAI before, does not affect this. */
	m2ap_handle_mbms_session_stop_request(&mbms_ref->tmgi, mbms_ref->mbms_service_area_id, true); /**< Should also remove the MBMS service. */
	OAILOG_FUNC_OUT (LOG_M2AP);
  }

  /** Update all eNBs, which are already in the MBMS Service. */
  for(int i = 0; i < num_m2ap_enbs_new_mbms_sai; i++) {
    /** Get the eNB from the SCTP association. */
	m2ap_enb_description_t * m2ap_enb_ref = m2ap_enb_p_elements[i];
	hashtable_rc_t h_rc = hashtable_uint64_ts_is_key_exists(&mbms_ref->g_m2ap_assoc_id2mce_enb_id_coll, m2ap_enb_ref->sctp_assoc_id);
	if(HASH_TABLE_OK == h_rc) {
	  /** eNB is already in the list, update it. */
	  int rc = m2ap_generate_mbms_session_update_request(mbms_ref->mce_mbms_m2ap_id, m2ap_enb_ref->sctp_assoc_id);
	  if(rc != RETURNok) {
	    OAILOG_ERROR(LOG_M2AP, "Error updating M2AP eNB with SCTP Assoc ID (%d) for the updated MBMS Service with TMGI " TMGI_FMT". "
	    "Removing the association.\n", m2ap_enb_ref->sctp_assoc_id, TMGI_ARG(&mbms_ref->tmgi));
	    m2ap_generate_mbms_session_stop_request(mbms_ref->mce_mbms_m2ap_id, m2ap_enb_ref->sctp_assoc_id);
	    /** Remove the hash key, which should also update the eNB. */
	    hashtable_uint64_ts_free(&mbms_ref->g_m2ap_assoc_id2mce_enb_id_coll, (const hash_key_t)m2ap_enb_ref->sctp_assoc_id);
	    if(m2ap_enb_ref->nb_mbms_associated)
	      m2ap_enb_ref->nb_mbms_associated--;
	    OAILOG_ERROR(LOG_M2AP, "Removed association after erroneous update.\n");
	  }
	  OAILOG_INFO(LOG_M2AP, "Successfully updated eNB with SCTP-Assoc (%d) for MBMS Service " MCE_MBMS_M2AP_ID_FMT " with updated MBMS SAI (%d).\n",
			  m2ap_enb_ref->sctp_assoc_id, mbms_ref->mce_mbms_m2ap_id, mbms_ref->mbms_service_area_id);
	  /** continue. */
	} else {
		int rc = m2ap_generate_mbms_session_start_request(mbms_ref->mce_mbms_m2ap_id, 1, &m2ap_enb_ref); // todo: test if this works..
		/** continue. */
		if(rc == RETURNok) {
		  OAILOG_INFO(LOG_M2AP, "Successfully adding new eNB with SCTP-Assoc (%d) for MBMS Service " MCE_MBMS_M2AP_ID_FMT " with updated MBMS SAI (%d).\n",
				  m2ap_enb_ref->sctp_assoc_id, mbms_ref->mce_mbms_m2ap_id, mbms_ref->mbms_service_area_id);
		} else {
		  OAILOG_ERROR(LOG_M2AP, "Error adding new NB with SCTP-Assoc (%d) for MBMS Service " MCE_MBMS_M2AP_ID_FMT " with updated MBMS SAI (%d).\n",
				  m2ap_enb_ref->sctp_assoc_id, mbms_ref->mce_mbms_m2ap_id, mbms_ref->mbms_service_area_id);
		}
	}
  }
  OAILOG_FUNC_OUT(LOG_M2AP);
}

//------------------------------------------------------------------------------
static
int m2ap_mbms_scheduling_cluster(const uint8_t num_m2_enb_mbms_area, const m2ap_enb_description_t** const m2ap_enb_p_elements,
		const uint8_t num_mbms_area, const mbsfn_areas_t * const mbsfn_cluster_global, const mbsfn_areas_t * const mbsfn_cluster_local, const long mcch_rep_abs_rf){

	OAILOG_FUNC_IN(LOG_MCE_APP);
 	/**
	 * Each eNB in the cluster may have different MBSFN areas.
	 * Thus we need to encode for each M2 eNB separately, based on its MBSFN areas.
	 */
	for(int num_m2_enbs = 0; num_m2_enbs < num_m2_enb_mbms_area; num_m2_enbs++){
		uint8_t 			 					num_scheduled_mbsfn_areas_cfgs_m2_enb = 0;
		mbsfn_area_cfg_t 			 *mbsfn_area_cfgs_pP[MAX_MBMSFN_AREAS];
		m2ap_enb_description_t *m2_enb_description = m2ap_enb_p_elements[num_m2_enbs];
		memset((void*)mbsfn_area_cfgs_pP, 0, MAX_MBMSFN_AREAS * sizeof(mbsfn_areas_t*));
		for(int num_mbsfn_area_m2_enb = 0; num_mbsfn_area_m2_enb < m2_enb_description->mbsfn_area_ids.num_mbsfn_area_ids; num_mbsfn_area_m2_enb++){
			/** Collect the scheduling data for the MBSFN area from the global list. No need to check local/global flag. */
			if(m2_enb_description->local_mbms_area == 0 || !mme_config.mbms.mbms_global_mbsfn_area_per_local_group){
				if(mbsfn_cluster_global){
					/** Search the global areas. */
					for(int num_scheduled_mbsfn_area = 0; num_scheduled_mbsfn_area < mbsfn_cluster_global->num_mbsfn_areas; num_scheduled_mbsfn_area++){
						if(mbsfn_cluster_global->mbsfn_area_cfg[num_scheduled_mbsfn_area].mbsfnArea.mbsfn_area_id == m2_enb_description->mbsfn_area_ids.mbsfn_area_id[num_mbsfn_area_m2_enb]){
							/** Add the configuration to the list of MBSFN areas. */
							mbsfn_area_cfgs_pP[num_scheduled_mbsfn_areas_cfgs_m2_enb++] = &mbsfn_cluster_global->mbsfn_area_cfg[num_scheduled_mbsfn_area];
						}
					}
				}
			}
			/** If the local MBMS area is not 0, check the local MBSFN areas, in addition. */
			if(mbsfn_cluster_local){
				for(int num_scheduled_mbsfn_area = 0; num_scheduled_mbsfn_area < mbsfn_cluster_local->num_mbsfn_areas; num_scheduled_mbsfn_area++){
					if(mbsfn_cluster_local->mbsfn_area_cfg[num_scheduled_mbsfn_area].mbsfnArea.mbsfn_area_id == m2_enb_description->mbsfn_area_ids.mbsfn_area_id[num_mbsfn_area_m2_enb]){
						/** Add the configuration to the list of MBSFN areas. */
						mbsfn_area_cfgs_pP[num_scheduled_mbsfn_areas_cfgs_m2_enb++] = &mbsfn_cluster_local->mbsfn_area_cfg[num_scheduled_mbsfn_area];
					}
				}
			}
		}
		OAILOG_INFO(LOG_MCE_APP, "Collected (%d) MBSFN areas for M2AP eNB with eNB Id (%d) in local_mbms_area (%d).\n",
				num_scheduled_mbsfn_areas_cfgs_m2_enb, m2_enb_description->m2ap_enb_id, m2_enb_description->local_mbms_area);
		if(num_scheduled_mbsfn_areas_cfgs_m2_enb){
			if(m2ap_generate_mbms_scheduling_information(m2_enb_description, (mbsfn_area_cfg_t **)mbsfn_area_cfgs_pP, num_scheduled_mbsfn_areas_cfgs_m2_enb, mcch_rep_abs_rf) == RETURNerror){
				OAILOG_ERROR(LOG_MCE_APP, "Error encoding MBMS scheduling information for M2AP eNB with eNB Id (%d) in local_mbms_area (%d).\n",
						m2_enb_description->m2ap_enb_id, m2_enb_description->local_mbms_area);
				OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
			}
		}
		/** Done scheduling the eNB, continue clear with the next eNB. */
	}
	OAILOG_INFO(LOG_MCE_APP, "Successfully scheduled all (%d) M2 eNBs in MBMS area (%d). \n", num_m2_enb_mbms_area, num_mbms_area);
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
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
  M2AP_SessionStartRequest_t			 			 *out;
  M2AP_SessionStartRequest_Ies_t		  	 *ie = NULL;

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
  uint32_t mbms_sai = mbms_ref->mbms_service_area_id | (0x01 << 16); /**< Add the length into the encoded value. */
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

  if (m2ap_mce_encode_pdu (&pdu, &buffer_p, &length) < 0) {
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
  hashtable_uint64_ts_get (&mbms_ref->g_m2ap_assoc_id2mce_enb_id_coll, (const hash_key_t)sctp_assoc_id, (void *)&enb_mbms_m2ap_id);
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
  if (m2ap_mce_encode_pdu (&pdu, &buffer_p, &length) < 0) {
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

//-----------------------------------------------------------------------------
static
int m2ap_generate_mbms_scheduling_information(const m2ap_enb_description_t * m2_enb_description, mbsfn_area_cfg_t **mbsfn_area_cfg, const uint8_t num_mbsfn_area_per_enb, long mcch_rep_abs_rf)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);

	uint8_t                                *buffer_p = NULL;
	uint32_t                                length = 0;
	uint8_t															 		num_m2ap_enbs = 0;
	mbsfn_area_cfg_t					  					 *mbsfn_area_cfg				= NULL;
	void                                   *id = NULL;

	M2AP_M2AP_PDU_t                         pdu = {0};
	M2AP_MbmsSchedulingInformation_t			 *out;
	M2AP_MbmsSchedulingInformation_Ies_t	 *ie = NULL;
	M2AP_MBSFN_Area_Configuration_Item_t	 *mbsfnAreaCfgItem = NULL;

	/**
	 * At least one eNB exists,
	 * Create and encode the message for all eNBs.
	 * We have found the UE in the list.
	 * Create new IE list message and encode it.
	 */
	memset(&pdu, 0, sizeof(pdu));
	pdu.present = M2AP_M2AP_PDU_PR_initiatingMessage;
	pdu.choice.initiatingMessage.procedureCode = M2AP_ProcedureCode_id_mbmsSchedulingInformation;
	pdu.choice.initiatingMessage.criticality = M2AP_Criticality_ignore;
	pdu.choice.initiatingMessage.value.present = M2AP_InitiatingMessage__value_PR_MbmsSchedulingInformation;
	out = &pdu.choice.initiatingMessage.value.choice.MbmsSchedulingInformation;

	/*
	 * mandatory
	 */
	ie = (M2AP_MbmsSchedulingInformation_Ies_t *)calloc(1, sizeof(M2AP_MbmsSchedulingInformation_Ies_t));
	ie->id = M2AP_ProtocolIE_ID_id_MCCH_Update_Time;
	ie->criticality = M2AP_Criticality_reject;
	ie->value.present = M2AP_MbmsSchedulingInformation_Ies__value_PR_MCCH_Update_Time;
	DevAssert(mcch_rep_abs_rf % mbsfn_area_cfg->mbsfnArea.mcch_modif_period_rf == 0); /**< Assert that it is a multiple. */
	uint8_t mcch_update_time = (mcch_rep_abs_rf / mbsfn_area_cfg->mbsfnArea.mcch_modif_period_rf) % 256;
	ie->value.choice.MCCH_Update_Time = mcch_update_time; /**< Absolute counter of the MCCH update period for the MBSFN area. */
	ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

	/*
	 * mandatory
	 */
	ie = (M2AP_MbmsSchedulingInformation_Ies_t *)calloc(1, sizeof(M2AP_MbmsSchedulingInformation_Ies_t));
	ie->id = M2AP_ProtocolIE_ID_id_MBSFN_Area_Configuration_List;
	ie->criticality = M2AP_Criticality_reject;
	ie->value.present = M2AP_MbmsSchedulingInformation_Ies__value_PR_MBSFN_Area_Configuration_List;
	ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

	/** Create list items and add them to the scheduling information. */
	for(int num_mbsfn = 0; num_mbsfn < num_mbsfn_area_per_enb; num_mbsfn++){
		/** Create a list item for each MBSFN area .*/
		M2AP_MBSFN_Area_Configuration_List_t * m2ap_mbsfn_config_item = (M2AP_MBSFN_Area_Configuration_List_t *)calloc(1, sizeof(M2AP_MBSFN_Area_Configuration_List_t));

		/**
	   * Mandatory
	   * MBSFN Area Id
	   */
		mbsfnAreaCfgItem = (M2AP_MBSFN_Area_Configuration_Item_t*)calloc(1, sizeof (M2AP_MBSFN_Area_Configuration_Item_t));
		mbsfnAreaCfgItem->id = M2AP_ProtocolIE_ID_id_MBSFN_Area_ID;
		mbsfnAreaCfgItem->criticality 	= M2AP_Criticality_reject;
		mbsfnAreaCfgItem->value.present = M2AP_MBSFN_Area_Configuration_Item__value_PR_MBSFN_Area_ID;
		ASN_SEQUENCE_ADD(m2ap_mbsfn_config_item, mbsfnAreaCfgItem);
		mbsfnAreaCfgItem->value.choice.MBSFN_Area_ID = mbsfn_area_cfg->mbsfnArea.mbsfn_area_id;

		/**
		 * Mandatory: PMCH Configuration List for MBSFN Area.
		 */
		mbsfnAreaCfgItem = (M2AP_MBSFN_Area_Configuration_Item_t*)calloc(1, sizeof (M2AP_MBSFN_Area_Configuration_Item_t));
		mbsfnAreaCfgItem->id = M2AP_ProtocolIE_ID_id_PMCH_Configuration_List;
		mbsfnAreaCfgItem->criticality = M2AP_Criticality_reject;
		mbsfnAreaCfgItem->value.present = M2AP_MBSFN_Area_Configuration_Item__value_PR_PMCH_Configuration_List;
		ASN_SEQUENCE_ADD(m2ap_mbsfn_config_item, mbsfnAreaCfgItem);

		/** Set the PMCH (MCH) configuration list, for every MCH. */
		for(int n_mch = 0; n_mch < MAX_MCH_PER_MBSFN; n_mch++){
			if(mbsfn_area_cfg->mchs.mch_array[n_mch].mch_qci){
				M2AP_PMCH_Configuration_ItemIEs_t * pmch_configuration_item_ie = (M2AP_PMCH_Configuration_ItemIEs_t*)calloc(1, sizeof (M2AP_PMCH_Configuration_ItemIEs_t));
				pmch_configuration_item_ie->id = M2AP_ProtocolIE_ID_id_PMCH_Configuration_Item;
				pmch_configuration_item_ie->criticality 	= M2AP_Criticality_reject;
				pmch_configuration_item_ie->value.present = M2AP_PMCH_Configuration_ItemIEs__value_PR_PMCH_Configuration_Item;
				M2AP_PMCH_Configuration_Item_t * pmch_configuration_item = &pmch_configuration_item_ie->value.choice.PMCH_Configuration_Item.pmch_Configuration;
				/** Absolute subframe number in a CSA period. */
				pmch_configuration_item->pmch_Configuration.allocatedSubframesEnd = mbsfn_area_cfg->mchs.mch_array[n_mch].mch_subframe_stop;
				pmch_configuration_item->pmch_Configuration.dataMCS								= mbsfn_area_cfg->mchs.mch_array[n_mch].mcs;
				pmch_configuration_item->pmch_Configuration.mchSchedulingPeriod   = mbsfn_area_cfg->mchs.mch_array[n_mch].msp_rf;
				/** Set the MBMS sessions to schedule. */
				for(int num_mbms_session = 0; num_mbms_session < mbsfn_area_cfg->mchs.mch_array[n_mch].mbms_session_list.num_mbms_sessions; num_mbms_session++){
					/**
					 * The MCH Set the MBMS session into the PMCH list.
					 * MCCH subframe handling will be done in the MCH layer.
					 * Here we only have the MBMS services.
					 */
					MBMSsessionListPerPMCH_Item__Member * mbms_session_list_item = (MBMSsessionListPerPMCH_Item__Member*)calloc(1, sizeof(MBMSsessionListPerPMCH_Item__Member));
					mbms_session_list_item->lcid = (num_mbms_session +1);
					tmgi_t * tmgi_p = &mbsfn_area_cfg->mchs.mch_array[n_mch].mbms_session_list.tmgis[num_mbms_session];
				  INT24_TO_OCTET_STRING(tmgi_p->mbms_service_id, &mbms_session_list_item->tmgi.serviceID);
				  TBCD_TO_PLMN_T(&mbms_session_list_item->tmgi.pLMNidentity, &tmgi_p->plmn);
				  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
				}
				ASN_SEQUENCE_ADD(&mbsfnAreaCfgItem->value.choice.PMCH_Configuration_List.list, pmch_configuration_item_ie);
			}
		}

		/** Configure an MBSFN Subframe configuration list. */
		mbsfnAreaCfgItem = calloc(1, sizeof (M2AP_MBSFN_Area_Configuration_Item_t));
		mbsfnAreaCfgItem->id = M2AP_ProtocolIE_ID_id_MBSFN_Subframe_Configuration_List;
		mbsfnAreaCfgItem->criticality = M2AP_Criticality_reject;
		mbsfnAreaCfgItem->value.present = M2AP_MBSFN_Area_Configuration_Item__value_PR_MBSFN_Subframe_ConfigurationList;
		ASN_SEQUENCE_ADD(m2ap_mbsfn_config_item, mbsfnAreaCfgItem);

		/**
		 * Mandatory
		 * Subframe Configuration List for MBSFN Area.
		 * Set all (up to 8) CSA patterns, that the MBSFN area uses (with other MBSFN areas).
		 */
		for(int mbsfn_csa_pattern = 0; mbsfn_area_cfg->csa_patterns.num_csa_pattern; mbsfn_csa_pattern++) {
			M2AP_MBSFN_Subframe_Configuration_t * mbsfn_sf_confg = calloc(1, sizeof(M2AP_MBSFN_Subframe_Configuration_t));
			/** Check the RF-Allocation Period as log2. */
			mbsfn_sf_confg->radioframeAllocationPeriod = log2(mbsfn_area_cfg->csa_patterns.csa_pattern[mbsfn_csa_pattern].csa_pattern_repetition_period_rf);
			DevAssert(mbsfn_sf_confg->radioframeAllocationPeriod >=0);
			DevAssert(mbsfn_sf_confg->radioframeAllocationPeriod <= 5);
			mbsfn_sf_confg->radioframeAllocationOffset = log2(mbsfn_area_cfg->csa_patterns.csa_pattern[mbsfn_csa_pattern].csa_pattern_offset_rf);
			DevAssert(mbsfn_sf_confg->radioframeAllocationOffset >=0);
			DevAssert(mbsfn_sf_confg->radioframeAllocationOffset <= 7);
			if(mbsfn_area_cfg->csa_patterns.csa_pattern[mbsfn_csa_pattern].mbms_csa_pattern_rfs == CSA_FOUR_FRAME) {
				/** 24 bit bitmap. */
				mbsfn_sf_confg->subframeAllocation.present = M2AP_MBSFN_Subframe_Configuration__subframeAllocation_PR_fourFrames;
				FOUR_FRAME_ITEM_SF_TO_BIT_STRING(mbsfn_area_cfg->csa_patterns.csa_pattern[mbsfn_csa_pattern].csa_pattern_sf.mbms_mch_csa_pattern_4rf,
						&mbsfn_sf_confg->subframeAllocation.choice.fourFrames);
			} else {
				/** 6 bit bitmap. */
				mbsfn_sf_confg->subframeAllocation.present = M2AP_MBSFN_Subframe_Configuration__subframeAllocation_PR_oneFrame;
				ONE_FRAME_ITEM_SF_TO_BIT_STRING(mbsfn_area_cfg->csa_patterns.csa_pattern[mbsfn_csa_pattern].csa_pattern_sf.mbms_mch_csa_pattern_1rf,
						&mbsfn_sf_confg->subframeAllocation.choice.oneFrame);
			}
			/** Add it to the MBSFN configuration items. */
			ASN_SEQUENCE_ADD(&mbsfnAreaCfgItem->value.choice.MBSFN_Subframe_ConfigurationList.list, mbsfn_sf_confg);
		}

		/**
		 * Mandatory
		 * Set the CSA Period
		 */
		mbsfnAreaCfgItem = calloc(1, sizeof (M2AP_MBSFN_Area_Configuration_Item_t));
		mbsfnAreaCfgItem->id = M2AP_ProtocolIE_ID_id_Common_Subframe_Allocation_Period;
		mbsfnAreaCfgItem->criticality = M2AP_Criticality_reject;
		mbsfnAreaCfgItem->value.present = M2AP_MBSFN_Area_Configuration_Item__value_PR_Common_Subframe_Allocation_Period;
		ASN_SEQUENCE_ADD(m2ap_mbsfn_config_item, mbsfnAreaCfgItem);
		mbsfnAreaCfgItem->value.choice.Common_Subframe_Allocation_Period = log2(mbsfn_area_cfg->mbsfnArea.mbsfn_csa_period_rf/4);
		DevAssert(mbsfnAreaCfgItem->value.choice.Common_Subframe_Allocation_Period >=0);
		DevAssert(mbsfnAreaCfgItem->value.choice.Common_Subframe_Allocation_Period <= 6);

		/** Set the MBSFN configuration item. */
		ASN_SET_ADD(&ie->value.choice.MBSFN_Area_Configuration_List, m2ap_mbsfn_config_item);
	}

	/**
	 * Encode and send it to all eNBs.
	 */
	if (m2ap_mce_encode_pdu (&pdu, &buffer_p, &length) < 0) {
		OAILOG_ERROR (LOG_M2AP, "Error encoding MBMS Scheduling Information.\n");
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
	}
	bstring b = blk2bstr(buffer_p, length);
	free(buffer_p);

	/**
	 * Todo: Encode and send it to all M2AP eNBs.. hash_callback.
	 * Non-MBMS signalling -> stream 0
	 */
	int rc = m2ap_mce_itti_send_sctp_request (&b, m2_enb_description->sctp_assoc_id, M2AP_ENB_SERVICE_SCTP_STREAM_ID, INVALID_MCE_MBMS_M2AP_ID);
	if(rc == RETURNerror){
		OAILOG_ERROR (LOG_M2AP, "Error sending MBMS Scheduling Information to eNB with sctp_assoc=%d.\n", m2_enb_desc->sctp_assoc_id);
		/** Continue. */
		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
	}
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

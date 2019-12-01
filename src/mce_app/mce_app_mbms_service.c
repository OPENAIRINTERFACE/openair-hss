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

/*! \file mce_app_mbms_service.c
  \brief
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
#include <time.h>

#include "3gpp_requirements_23.246.h"
#include "3gpp_requirements_29.468.h"
#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "mce_app_defs.h"
#include "timer.h"
#include "common_defs.h"
#include "gcc_diag.h"
#include "mce_app_itti_messaging.h"
#include "mce_app_statistics.h"
// #include "m2ap_mme.h"
#include "mme_app_session_context.h"
#include "mce_app_mbms_service_context.h"

//#include "mme_app_procedures.h"

static teid_t                           mce_app_mme_sm_teid_generator = 0x00000001;

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

static void mce_app_stop_mbms_service(tmgi_t * tmgi, mbms_service_area_id_t mbms_sa_id, teid_t mme_sm_teid, struct sockaddr * mbms_peer_ip);

//------------------------------------------------------------------------------
static
int mce_app_check_resources(const mbms_service_index_t mbms_service_index, const mbms_service_area_id_t mbms_service_area_id, long *mcch_modif_periods);

//------------------------------------------------------------------------------
void
mce_app_handle_mbms_session_start_request(
     itti_sm_mbms_session_start_request_t * const mbms_session_start_request_pP
    )
{
  OAILOG_FUNC_IN (LOG_MCE_APP);

  MessageDef                             *message_p  			= NULL;
  teid_t 								 									mme_sm_teid			= INVALID_TEID;
  mbms_service_t 						 						 *mbms_service 		= NULL;
  mbms_service_index_t 					  				mbms_service_idx      			= INVALID_MBMS_SERVICE_INDEX;
  mme_app_mbms_proc_t				 	 					 *mbms_sm_proc 								= NULL;
  mbms_service_area_id_t 			      			mbms_service_area_id 				= INVALID_MBMS_SERVICE_AREA_ID;
  mbsfn_area_context_t									 *mbsfn_area_context					= NULL;
  int                                     rc 													= RETURNok;

  /** Check the destination TEID is 0 in the MME-APP to respond and to clear the transaction. */
  if(mbms_session_start_request_pP->teid != (teid_t)0){
    OAILOG_WARNING (LOG_SM, "Destination TEID of MBMS Session Start Request is not 0, instead " TEID_FMT ". Rejecting MBMS Session Start Request for TMGI " TMGI_FMT". \n",
    		mbms_session_start_request_pP->teid, TMGI_ARG(&mbms_session_start_request_pP->tmgi));
    /** Send a negative response before crashing. */
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid,
	 	   &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * Check if the MBMS Service Area and PLMN is served. No TA/CELL List will be checked.
   * The MME/MCE supports per Sm message only one MBMS Service Area.
   * If multiple MBMS Service Areas are received, we will just consider the first one, we don't check which eNBs from which MBMS Service Areas are attached, due to split of layers.
   *
   * MME may have received consecutive multiple MBMS Service Areas, but we will require MBMS Flow Identifier.
   * Since the MBMS Flow Identifier is set as location (MBMS Service Area) of an MBMS Service, we will only consider one MBMS Service Area for the MBMS Service.
   * We will register the given MBMS Service (TMGI) with the MBMS Service Area uniquely.
   */
  if ((mbms_service_area_id = mce_app_check_mbms_sa_exists(&mbms_session_start_request_pP->tmgi.plmn, &mbms_session_start_request_pP->mbms_service_area) == INVALID_MBMS_SERVICE_AREA_ID)) {
    /**
     * The MBMS Service Area and PLMN are served by this MME.
     */
    OAILOG_ERROR(LOG_MCE_APP, "PLMN " PLMN_FMT " or none of the Target SAs are served by current MME. Rejecting MBMS Session Start Request for TMGI "TMGI_FMT".\n",
    	PLMN_ARG(&mbms_session_start_request_pP->tmgi.plmn), TMGI_ARG(&mbms_session_start_request_pP->tmgi));
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
    /** No MBMS service or tunnel endpoint is allocated yet. */
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * Check that MBSFN area for the MBMS service area is existing.
   * Later check the resources.
   * todo: iterate through the remaining..
   */
  if(!(mbsfn_area_context = mbsfn_area_mbms_service_id_in_list(&mce_app_desc.mce_mbsfn_area_contexts, mbms_service_area_id))){
  	OAILOG_ERROR(LOG_MCE_APP, "No MBSFN Area context for MBMS SAI "MBMS_SERVICE_AREA_ID_FMT" could be found. Rejecting MBMS Session Start Request for TMGI "TMGI_FMT".\n",
  			mbms_service_area_id, TMGI_ARG(&mbms_session_start_request_pP->tmgi));
  	mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
  	/** No MBMS service or tunnel endpoint is allocated yet. */
  	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /** Days not allowed because fu. */
  if (mbms_session_start_request_pP->mbms_session_duration.days) {
    /** The MBMS Service Area and PLMN are served by this MME. */
  	OAILOG_ERROR(LOG_MCE_APP, "No session duration for days are allowed. Rejecting MBMS Service Start Request for TMGI " TMGI_FMT ". \n", TMGI_ARG(&mbms_session_start_request_pP->tmgi));
  	mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
  	/** No MBMS service or tunnel endpoint is allocated yet. */
  	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /** Check that service duration is > 0. */
  if (!mbms_session_start_request_pP->mbms_session_duration.seconds) {
    /** The MBMS Service Area and PLMN are served by this MME. */
  	OAILOG_ERROR(LOG_MCE_APP, "No session duration given for requested MBMS Service Request for TMGI " TMGI_FMT ". \n", TMGI_ARG(&mbms_session_start_request_pP->tmgi));
  	mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
  	/** No MBMS service or tunnel endpoint is allocated yet. */
  	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  mme_config_read_lock (&mme_config);
  /** Check for a minimum time. */
  if(mbms_session_start_request_pP->mbms_session_duration.seconds < mme_config.mbms.mbms_min_session_duration_in_sec) {
    OAILOG_WARNING(LOG_MCE_APP, "MBM Session duration (%ds) is shorter than the minimum (%ds) for MBMS Service Request for TMGI " TMGI_FMT ". Setting to minval. \n",
    	mbms_session_start_request_pP->mbms_session_duration.seconds, mme_config.mbms.mbms_min_session_duration_in_sec, TMGI_ARG(&mbms_session_start_request_pP->tmgi));
    mbms_session_start_request_pP->mbms_session_duration.seconds = mme_config.mbms.mbms_min_session_duration_in_sec;
  }
  mme_config_unlock (&mme_config);

  /** Check that the MBMS Bearer QoS is a valid GBR. */
  if(!is_qci_gbr(mbms_session_start_request_pP->mbms_bearer_level_qos.qci)){
    /** Return error, no modification on non-GBR is allowed. */
  	OAILOG_ERROR(LOG_MCE_APP, "Non-GBR MBMS Bearer Level QoS (qci=%d) not supported. Rejecting MBMS Service Request for TMGI " TMGI_FMT ". \n",
			mbms_session_start_request_pP->mbms_bearer_level_qos.qci, TMGI_ARG(&mbms_session_start_request_pP->tmgi));
  	mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
  	/** No MBMS service or tunnel endpoint is allocated yet. */
  	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /** Verify the received gbr qos, if any is received. */
  if(validateEpsQosParameter(mbms_session_start_request_pP->mbms_bearer_level_qos.qci,
		  mbms_session_start_request_pP->mbms_bearer_level_qos.pvi, mbms_session_start_request_pP->mbms_bearer_level_qos.pci, mbms_session_start_request_pP->mbms_bearer_level_qos.pl,
		  mbms_session_start_request_pP->mbms_bearer_level_qos.gbr.br_dl, mbms_session_start_request_pP->mbms_bearer_level_qos.gbr.br_ul,
		  mbms_session_start_request_pP->mbms_bearer_level_qos.mbr.br_dl, mbms_session_start_request_pP->mbms_bearer_level_qos.mbr.br_ul) == RETURNerror){
    OAILOG_ERROR(LOG_MCE_APP, "MBMS Bearer Level QoS (qci=%d) could not be validated.Rejecting MBMS Service Request for TMGI " TMGI_FMT ". \n",
    		mbms_session_start_request_pP->mbms_bearer_level_qos.pci, TMGI_ARG(&mbms_session_start_request_pP->tmgi));
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
    /** No MBMS service or tunnel endpoint is allocated yet. */
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * Check that no MBMS Service with the given service ID and service area exists, if so reject
   * the request and implicitly remove the MBMS service & tunnel.
   */
  if((mbms_service = mce_mbms_service_exists_tmgi(&mce_app_desc.mce_mbms_service_contexts, &mbms_session_start_request_pP->tmgi, mbms_service_area_id))) {
    OAILOG_ERROR(LOG_MCE_APP, "An old MBMS Service context already existed for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". Implicitly removing old one (& removing procedures). \n",
    	TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);
    /** Removing old MBMS Service Context and informing the MCE APP. No response is expected from MCE. */
    mce_app_itti_m3ap_mbms_session_stop_request(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id, true);
    /** Remove the Sm Tunnel for the old one. */
    mce_app_stop_mbms_service(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id, mbms_service->privates.fields.mme_teid_sm,
    	(struct sockaddr*)&mbms_service->privates.fields.mbms_peer_ip);
    /** Check the flags,if the MBMS Re-Establishment indication is set, continue. */
    if(!mbms_session_start_request_pP->mbms_flags.msri) {
    	OAILOG_ERROR(LOG_MCE_APP, "No Re-Establishment request is received for duplicate MBMS Service Request for TMGI " TMGI_FMT". Rejecting MBMS Session Start Request.\n", TMGI_ARG(&mbms_session_start_request_pP->tmgi));
    	mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, SYSTEM_FAILURE);
    	OAILOG_FUNC_OUT (LOG_MCE_APP);
    }
    OAILOG_WARNING(LOG_MCE_APP, "MBMS Re-Establishment indication is set for service with TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". Continuing with the establishment after removal of duplicate MBMS Service Context. \n",
    	TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);
  }

  /**
   * Check the requested MBMS Bearer.
   * Session Start should always trigger a new MBMS session. It may reuse an MBMS Bearer.
   * C-TEID is always unique of the MBMS Bearer service for the given MBMS SA and flow-ID.
   */
  if((mbms_service = mbms_cteid_in_list(&mce_app_desc.mce_mbms_service_contexts, mbms_session_start_request_pP->mbms_ip_mc_address.cteid)) != NULL){
    OAILOG_ERROR(LOG_MCE_APP, "An old MBMS Service context already existed for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " already exist with given CTEID "TEID_FMT". "
    	"Rejecting new one for TMGI "TMGI_FMT". Leaving old one. \n", TMGI_ARG(&mbms_service->privates.fields.tmgi), mbms_service->privates.fields.mbms_service_area_id, TMGI_ARG(&mbms_session_start_request_pP->tmgi));
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * Verify the start and end MCCH modification periods.
   * Time is since 1970.
   */
  long mcch_modif_periods[2] = {0, 0};
  mce_app_calculate_mbms_service_mcch_periods(mbms_session_start_request_pP->abs_start_time.sec_since_epoch,
  		mbms_session_start_request_pP->abs_start_time.usec, &mbms_session_start_request_pP->mbms_session_duration,
  		mbsfn_area_context->privates.fields.mbsfn_area.mcch_modif_period_rf, mcch_modif_periods);

  if(!(mcch_modif_periods[0] && mcch_modif_periods[1])){
  	OAILOG_ERROR(LOG_MCE_APP, "MBMS Service for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " has invalid or expired start/end times for the MBSFN area. Rejecting the MBMS Session Start Request. \n",
  			TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);
  	mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
  	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  OAILOG_INFO(LOG_MCE_APP, "Successfully processed the request for a new MBMS Service for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT". Continuing with the resource calculation. \n",
  		TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);

  mme_sm_teid = __sync_fetch_and_add (&mce_app_mme_sm_teid_generator, 0x00000001);
  OAILOG_INFO(LOG_MCE_APP, "Successfully processed the request for a new MBMS Service for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT". Continuing with the establishment. \n",
    		TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);

  if (!mce_register_mbms_service(&mbms_session_start_request_pP->tmgi, &mbms_session_start_request_pP->mbms_service_area, mme_sm_teid)) {
    OAILOG_ERROR(LOG_MCE_APP, "No MBMS Service context could be generated from TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". \n",
    		TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, SYSTEM_FAILURE);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }
  /** Update the MBMS Service with the given parameters. */
  mce_app_update_mbms_service(&mbms_session_start_request_pP->tmgi, mbms_service_area_id, mbms_service_area_id, &mbms_session_start_request_pP->mbms_bearer_level_qos,
  		mbms_session_start_request_pP->mbms_flow_id, &mbms_session_start_request_pP->mbms_ip_mc_address, &mbms_session_start_request_pP->mbms_peer_ip,
			mcch_modif_periods);

  mbms_service_index_t mbms_service_index = mce_get_mbms_service_index(&mbms_session_start_request_pP->tmgi, mbms_service_area_id);
  if(mce_app_check_resources(mbms_service_index, mbms_service_area_id, mcch_modif_periods) == RETURNerror){
  	OAILOG_ERROR(LOG_MCE_APP, "Resource check for new MBMS Service context with TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " failed. "
  			"Rejecting Sm MBMS Session Start Request. \n", TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);
  	mce_app_stop_mbms_service(&mbms_service->privates.fields.tmgi,
  			mbms_service->privates.fields.mbms_service_area_id, mme_sm_teid, NULL);
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, SYSTEM_FAILURE);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * Update the MBMS session start time, if the MCCH modification period has been missed.
   * Check the difference between the absolute start time and the current time.
   *
   * Create an MME APP procedure (not MCE).
   * Since the procedure is new, we don't need to check if another procedure exists.
   */
  DevAssert(mme_app_create_mbms_procedure(&mbms_service, mbms_session_start_request_pP->abs_start_time.sec_since_epoch,
		  mbms_session_start_request_pP->abs_start_time.usec, &mbms_session_start_request_pP->mbms_session_duration));

  /**
   * The MME may return an MBMS Session Start Response to the MBMS-GW as soon as the session request is accepted by one E-UTRAN node.
   * That's why, we start an Sm procedure for the received Sm message. At timeout (no eNB response, we automatically purge the MBMS Service Context and respond to the MBMS-GW).
   * We send an eNB general MBMS Session Start trigger over M3.
   */
  OAILOG_INFO(LOG_MCE_APP, "Created a MBMS procedure for new MBMS Session with TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". Informing the MCE over M3. \n",
    TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);
  /** Trigger M3AP MBMS Session Start Request. */
  mce_app_itti_m3ap_mbms_session_start_request(&mbms_session_start_request_pP->tmgi, mbms_service_area_id, &mbms_session_start_request_pP->mbms_bearer_level_qos,
  		&mbms_service->privates.fields.mbms_bc.mbms_ip_mc_distribution, mbms_session_start_request_pP->abs_start_time.sec_since_epoch,
			mbms_session_start_request_pP->abs_start_time.usec);
  /**
   * Directly respond to the MBMS-GW.
   * Don't wait to check, if the E-UTRAN has been established, not worth it.
   * We don't wait for the first successfully response from the E-UTRAN, because we don't know how far in the future the absolute start time is.
   * The eNB cannot store the MBMS Absolute Start Time.
   */
  NOT_REQUIREMENT_3GPP_23_246(R8_3_2__6);
  mce_app_itti_sm_mbms_session_start_response(mbms_service->privates.fields.mme_teid_sm, mbms_service->privates.fields.mbms_teid_sm,
  		(struct sockaddr*)&mbms_service->privates.fields.mbms_peer_ip, (void*)mbms_session_start_request_pP->trxn, REQUEST_ACCEPTED);
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
void
mce_app_handle_mbms_session_update_request(
     itti_sm_mbms_session_update_request_t * const mbms_session_update_request_pP
    )
{
  OAILOG_FUNC_IN (LOG_MCE_APP);

  MessageDef                             *message_p  								= NULL;
  mbsfn_area_context_t									 *mbsfn_area_context			  = NULL;
  mbms_service_t 												 *mbms_service 							= NULL;
  mme_app_mbms_proc_t					 					 *mbms_sm_proc 							= NULL;
  mbms_service_index_t 									  mbms_service_idx  				= INVALID_MBMS_SERVICE_INDEX;
  teid_t 								  								mme_sm_teid								= INVALID_TEID;
  mbms_service_area_id_t 			      			new_mbms_service_area_id 	= INVALID_MBMS_SERVICE_AREA_ID,
  																				old_mbms_service_area_id 	= INVALID_MBMS_SERVICE_AREA_ID;
  int                                     rc 												= RETURNok;

  /**
   * Check if an MBMS Service exists.
   */
  mbms_service = mce_mbms_service_exists_sm_teid(&mce_app_desc.mce_mbms_service_contexts, &mbms_session_update_request_pP->teid);
  if(!mbms_service) {
    /** The MBMS Service Area and PLMN are served by this MME. */
    OAILOG_ERROR(LOG_MCE_APP, "No MBMS Service context exists for TEID " TEID_FMT ". Rejecting MBMS Session Update. \n", mbms_session_update_request_pP->teid);
    mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid,
    		mbms_session_update_request_pP->sm_mbms_fteid.teid, &mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, CONTEXT_NOT_FOUND);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  if(memcmp((void*)&mbms_service->privates.fields.tmgi, (void*)&mbms_session_update_request_pP->tmgi, sizeof(tmgi_t)) != 0){
    OAILOG_ERROR(LOG_MCE_APP, "Received TMGI " TMGI_FMT " not matching existing TMGI " TMGI_FMT " for TEID " TEID_FMT ". Rejecting MBMS Session Update. \n",
    		TMGI_ARG(&mbms_session_update_request_pP->tmgi), TMGI_ARG(&mbms_service->privates.fields.tmgi), mbms_session_update_request_pP->teid);
    mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid,
    		mbms_session_update_request_pP->sm_mbms_fteid.teid, &mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, REQUEST_REJECTED);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * The MBMS-StartStop-Indication AVP set to "UPDATE", the TMGI AVP and the
   * MBMS-Flow-Identifier AVP to designate the bearer to be modified.
   */
  REQUIREMENT_3GPP_29_468(R5_3_4);
  if(mbms_service->privates.fields.mbms_flow_id != mbms_session_update_request_pP->mbms_flow_id) {
    OAILOG_ERROR(LOG_MCE_APP, "MBMS Service with TMGI " TMGI_FMT " and MBMS flow id (%d) not received MBMS flow id (%d).. Rejecting MBMS Session Update. \n",
    	TMGI_ARG(&mbms_session_update_request_pP->tmgi), mbms_service->privates.fields.mbms_flow_id, mbms_session_update_request_pP->mbms_flow_id);
    mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid,
    	mbms_session_update_request_pP->sm_mbms_fteid.teid, &mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, REQUEST_REJECTED);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * We allow only one MBMS Service Area to change.
   */
  if ((new_mbms_service_area_id = mce_app_check_mbms_sa_exists(&mbms_session_update_request_pP->tmgi.plmn, &mbms_session_update_request_pP->mbms_service_area)) == INVALID_MBMS_SERVICE_AREA_ID) {
    /**
     * The MBMS Service Area and PLMN are served by this MME.
     */
    OAILOG_ERROR(LOG_MCE_APP, "PLMN " PLMN_FMT " or none of the Target SAs are served by current MME. Rejecting MBMS Session Update Request for TMGI "TMGI_FMT".\n",
    	PLMN_ARG(&mbms_session_update_request_pP->tmgi.plmn), TMGI_ARG(&mbms_session_update_request_pP->tmgi));
    mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid, mbms_session_update_request_pP->sm_mbms_fteid.teid,
    		&mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, REQUEST_REJECTED);
    /** No MBMS service or tunnel endpoint is allocated yet. */
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  if (mbms_session_update_request_pP->mbms_session_duration.days) {
    /** The MBMS Service Area and PLMN are served by this MME. */
  	OAILOG_ERROR(LOG_MCE_APP, "No session duration for days are allowed. Rejecting MBMS Service Update Request for TMGI " TMGI_FMT ". \n", TMGI_ARG(&mbms_session_update_request_pP->tmgi));
  	mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid, mbms_session_update_request_pP->sm_mbms_fteid.teid,
  		&mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, REQUEST_REJECTED);
  	/** No MBMS service or tunnel endpoint is allocated yet. */
  	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /** Check that service duration is > 0. */
  if (!mbms_session_update_request_pP->mbms_session_duration.seconds) {
    /** The MBMS Service Area and PLMN are served by this MME. */
  	OAILOG_ERROR(LOG_MCE_APP, "No session duration given for requested MBMS Service Request for TMGI " TMGI_FMT ". \n", TMGI_ARG(&mbms_session_update_request_pP->tmgi));
  	mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid, mbms_session_update_request_pP->sm_mbms_fteid.teid,
  		&mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, REQUEST_REJECTED);
  	/** No MBMS service or tunnel endpoint is allocated yet. */
  	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  mme_config_read_lock (&mme_config);
  /** Check for a minimum time. */
  if(mbms_session_update_request_pP->mbms_session_duration.seconds < mme_config.mbms.mbms_min_session_duration_in_sec) {
    OAILOG_WARNING(LOG_MCE_APP, "MBM Session duration (%ds) is shorter than the minimum (%ds) for MBMS Service Request for TMGI " TMGI_FMT ". Setting to minval. \n",
    	mbms_session_update_request_pP->mbms_session_duration.seconds, mme_config.mbms.mbms_min_session_duration_in_sec, TMGI_ARG(&mbms_session_update_request_pP->tmgi));
    mbms_session_update_request_pP->mbms_session_duration.seconds = mme_config.mbms.mbms_min_session_duration_in_sec;
  }
  mme_config_unlock (&mme_config);

  /** Check that the MBMS Bearer QoS is a valid GBR. */
  if(!is_qci_gbr(mbms_session_update_request_pP->mbms_bearer_level_qos.qci)){
  	/** Return error, no modification on non-GBR is allowed. */
  	OAILOG_ERROR(LOG_MCE_APP, "Non-GBR MBMS Bearer Level QoS (qci=%d) not supported. Rejecting MBMS Service Request for TMGI " TMGI_FMT ". \n",
  			mbms_session_update_request_pP->mbms_bearer_level_qos.qci, TMGI_ARG(&mbms_session_update_request_pP->tmgi));
  	mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid, mbms_session_update_request_pP->sm_mbms_fteid.teid, &mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, REQUEST_REJECTED);
  	/** No MBMS service or tunnel endpoint is allocated yet. */
  	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /** Verify the received gbr qos, if any is received. */
  if(validateEpsQosParameter(mbms_session_update_request_pP->mbms_bearer_level_qos.qci,
		  mbms_session_update_request_pP->mbms_bearer_level_qos.pvi, mbms_session_update_request_pP->mbms_bearer_level_qos.pci, mbms_session_update_request_pP->mbms_bearer_level_qos.pl,
		  mbms_session_update_request_pP->mbms_bearer_level_qos.gbr.br_dl, mbms_session_update_request_pP->mbms_bearer_level_qos.gbr.br_ul,
		  mbms_session_update_request_pP->mbms_bearer_level_qos.mbr.br_dl, mbms_session_update_request_pP->mbms_bearer_level_qos.mbr.br_ul) == RETURNerror){
    OAILOG_ERROR(LOG_MCE_APP, "MBMS Bearer Level QoS (qci=%d) could not be validated.Rejecting MBMS Service Request for TMGI " TMGI_FMT ". \n",
    		mbms_session_update_request_pP->mbms_bearer_level_qos.pci, TMGI_ARG(&mbms_session_update_request_pP->tmgi));
    mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid, mbms_session_update_request_pP->sm_mbms_fteid.teid, &mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, REQUEST_REJECTED);
    /** No MBMS service or tunnel endpoint is allocated yet. */
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * Check that MBSFN area for the MBMS service area is existing.
   * Later check the resources.
   * todo: iterate through the remaining..
   */
  if(!(mbsfn_area_context = mbsfn_area_mbms_service_id_in_list(&mce_app_desc.mce_mbsfn_area_contexts, new_mbms_service_area_id))){
  	OAILOG_ERROR(LOG_MCE_APP, "No MBSFN Area context for MBMS SAI "MBMS_SERVICE_AREA_ID_FMT" could be found. Rejecting MBMS Session Update Request for TMGI "TMGI_FMT".\n",
  			new_mbms_service_area_id, TMGI_ARG(&mbms_session_update_request_pP->tmgi));
    mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid, mbms_session_update_request_pP->sm_mbms_fteid.teid, &mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, REQUEST_REJECTED);
  	/** No MBMS service or tunnel endpoint is allocated yet. */
  	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * As in an MBMS Session Start Request message, calculate the MCCH start end end periods.
   * If the diff is shorter than x MCCH modification period, apply the update directly (no extra MBMS procedure is needed).
   * Verify the start and end MCCH modification periods.
   * Time is since 1970.
   */
  long mcch_modif_periods[2] = {0, 0};
  mce_app_calculate_mbms_service_mcch_periods(mbms_session_update_request_pP->abs_update_time.sec_since_epoch,
  		mbms_session_update_request_pP->abs_update_time.usec, &mbms_session_update_request_pP->mbms_session_duration,
			mbsfn_area_context->privates.fields.mbsfn_area.mcch_modif_period_rf, mcch_modif_periods);

  if(!(mcch_modif_periods[0] && mcch_modif_periods[1])){
  	OAILOG_ERROR(LOG_MCE_APP, "MBMS Service for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " has invalid or expired start/end times for the MBSFN area. Rejecting the MBMS Session Update Request. \n",
  			TMGI_ARG(&mbms_service->privates.fields.tmgi), new_mbms_service_area_id);
    mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid, mbms_session_update_request_pP->sm_mbms_fteid.teid, &mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, REQUEST_REJECTED);
  	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * CTEID or MBMS Multicast IPs don't change.
   */
  OAILOG_INFO(LOG_MCE_APP, "Successfully processed the update of MBMS Service for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT". Continuing with the update. \n",
  		TMGI_ARG(&mbms_session_update_request_pP->tmgi), new_mbms_service_area_id);

  /**
   * Check if any MBMS procedure for the MBMS Service exists, if so reject it.
   * Use the current MBMS Service Area.
   */
  if(mbms_service->mbms_procedure){
  	/**
  	 * We don't check between START/UPDATE procedures. We only care for the MBMS Session duration (since we apply the actions immediately on the
  	 * MBMS Service context, since we don't wait for the E-UTRAN response due to the unknown MBMS Absolute Start/Update timer.
  	 * Remove the procedure and stop its timer, not 2 can exist together.
  	 * We also respond immediately to the MBMS-Gw, so we don't expect duplicate requests (should be handled in the NW-GTPv2c stack.
  	 */
  	mme_app_delete_mbms_procedure(mbms_service);
  	OAILOG_INFO(LOG_MCE_APP, "Stopped & removed existing Sm procedure for MBMS Service with TMGI " TMGI_FMT ". \n", TMGI_ARG(&mbms_session_update_request_pP->tmgi));
  }

  /**
   * Set the new MBMS Bearer Context to be updated & new MBMS Service Area into the procedure!
   * Set it into the MBMS Service with the first response.
   * This will also update the MBMS Service Area and cause a new MBMS Service Index.
   * We set the new MCCH modification periods for start and end. We don't change the session start MCCH period,
   * since we wan't the MBMS service still to be calculated till it is activated.
   * We may have updated the MBMS SAI. So we will directly start to calculate the resources for the given MBSFN Area, without delay.
   */
  old_mbms_service_area_id = mbms_service->privates.fields.mbms_service_area_id;
  mce_app_update_mbms_service(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id, new_mbms_service_area_id,
  		&mbms_session_update_request_pP->mbms_bearer_level_qos, mbms_service->privates.fields.mbms_flow_id, NULL, NULL, NULL);

  /**
   * Check the resources also in the MBMS Session Update case.
   */
  mbms_service_index_t mbms_service_index = mce_get_mbms_service_index(&mbms_session_update_request_pP->tmgi, new_mbms_service_area_id);
  if(mce_app_check_resources(mbms_service_index, new_mbms_service_area_id, mcch_modif_periods) == RETURNerror){
  	OAILOG_ERROR(LOG_MCE_APP, "Resource check for updated MBMS Service context with TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " failed. "
  			"Rejecting Sm MBMS Session Update Request. \n", TMGI_ARG(&mbms_session_update_request_pP->tmgi), new_mbms_service_area_id);
  	mce_app_stop_mbms_service(&mbms_service->privates.fields.tmgi,
  			mbms_service->privates.fields.mbms_service_area_id, mme_sm_teid, NULL);
    mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid, mbms_session_update_request_pP->sm_mbms_fteid.teid, &mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, REQUEST_REJECTED);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * Don't update the MBMS service,
   * Create an MBMS Service Update Procedure and store the values there.
   * Use the current MBMS TMGI and the current MBMS Service Id.
   * If at least one eNB replies positively, update the values of the MBMS service with the procedures values. */
  mbms_sm_proc = mme_app_create_mbms_procedure(mbms_service, mbms_session_update_request_pP->abs_update_time.sec_since_epoch,
		  mbms_session_update_request_pP->abs_update_time.usec, &mbms_session_update_request_pP->mbms_session_duration);

  /**
   * Also transmit the MBMS IP, since new eNB may be added (changed of MBMS Service Area).
   */
  mce_app_itti_m3ap_mbms_session_update_request(&mbms_session_update_request_pP->tmgi, new_mbms_service_area_id, old_mbms_service_area_id,
  		&mbms_session_update_request_pP->mbms_bearer_level_qos, &mbms_service->privates.fields.mbms_bc.mbms_ip_mc_distribution,
			mbms_session_update_request_pP->abs_update_time.sec_since_epoch, mbms_session_update_request_pP->abs_update_time.usec);

  /**
   * Directly respond to the MBMS-GW.
   * Don't wait to check, if the E-UTRAN has been established, not worth it.
   * We don't wait for the first successfully response from the E-UTRAN, because we don't know how far in the future the absolute start time is.
   * The eNB cannot store the MBMS Absolute Start Time.
   */
  NOT_REQUIREMENT_3GPP_23_246(R8_3_2__6);
  mce_app_itti_sm_mbms_session_update_response(mbms_service->privates.fields.mme_teid_sm, mbms_service->privates.fields.mbms_teid_sm,
    (struct sockaddr*)&mbms_service->privates.fields.mbms_peer_ip, (void*)mbms_session_update_request_pP->trxn, REQUEST_ACCEPTED);
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
void
mce_app_handle_mbms_session_stop_request(
     itti_sm_mbms_session_stop_request_t * const mbms_session_stop_request_pP
    )
{
  OAILOG_FUNC_IN (LOG_MCE_APP);

  MessageDef                             *message_p  		= NULL;
  struct mbms_service_s                  *mbms_service 		= NULL;
  mbms_service_index_t					  mbms_service_idx 	= INVALID_MBMS_SERVICE_INDEX;
  int                                     rc 				= RETURNok;

  /**
   * Check if an MBMS Service exists.
   */
  mbms_service = mce_mbms_service_exists_sm_teid(&mce_app_desc.mce_mbms_service_contexts, &mbms_session_stop_request_pP->teid);
  if(!mbms_service) {
    /** The MBMS Service Area and PLMN are served by this MME. */
    OAILOG_ERROR(LOG_MCE_APP, "No MBMS Service context exists for TEID " TEID_FMT ". Ignoring MBMS Session Stop (MBMS Sm TEID unknowns). \n", mbms_session_stop_request_pP->teid);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  if(mbms_service->privates.fields.mbms_flow_id != mbms_session_stop_request_pP->mbms_flow_id){
    OAILOG_ERROR(LOG_MCE_APP, "MBMS Service with TMGI " TMGI_FMT " has MBMS Flow ID %d, but received MBMS Stop Request for MBMS Flow ID (%d). "
    		"Ignoring this error and continuing with the MBMS Session stop procedure. \n", TMGI_ARG(&mbms_service->privates.fields.tmgi),
			mbms_service->privates.fields.mbms_flow_id, mbms_session_stop_request_pP->mbms_flow_id);
  }

  /**
   * Check if there is any existing procedure running. Remove it.
   * Then start an MBMS Session Stop procedure for the Absolute Stop Timeout.
   */
  if(mbms_service->mbms_procedure){
    mme_app_delete_mbms_procedure(mbms_service);
  }
  /** Create a new MBMS procedure and set the  into the procedure. */
  //  uint32_t abs_stop_time_sec =  - time(NULL);
  if(mbms_session_stop_request_pP->abs_stop_time.sec_since_epoch && !mbms_session_stop_request_pP->mbms_flags.lmri){
  	mme_app_create_mbms_procedure(mbms_service, mbms_session_stop_request_pP->abs_stop_time.sec_since_epoch,
  			mbms_session_stop_request_pP->abs_stop_time.usec, NULL);
  }

  /**
   * The MBMS Flag will be sent to the MBMS Gw.
   *  if the MBMS GW has already moved the control of the MBMS session to an alternative MME(s), the MBMS
   *  GW shall send an MBMS Session Stop Request message to the MME previously controlling the MBMS session
   *  with a "Local MBMS bearer context release" indication to instruct that MME to release its MBMS bearer
   *  context locally, without sending any message to the MCE(s).
   */
  mce_app_itti_sm_mbms_session_stop_response(mbms_session_stop_request_pP->teid, mbms_service->privates.fields.mbms_teid_sm,
		  (struct sockaddr*)&mbms_service->privates.fields.mbms_peer_ip, mbms_session_stop_request_pP->trxn, REQUEST_ACCEPTED);
  /** M3AP Session Stop Request --> No Response is expected. Immediately terminate the MBMS Service afterwards. */
  if(!mbms_session_stop_request_pP->abs_stop_time.sec_since_epoch || mbms_session_stop_request_pP->mbms_flags.lmri) {
  	OAILOG_INFO(LOG_MCE_APP, "No MBMS session stop time is given for TMGI " TMGI_FMT ". Stopping immediately and informing the MCE over M3. \n", TMGI_ARG(&mbms_service->privates.fields.tmgi));
  	mbms_service_idx = mce_get_mbms_service_index(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id);
  	mce_app_itti_m3ap_mbms_session_stop_request(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id, (mbms_session_stop_request_pP->mbms_flags.lmri));
    mce_app_stop_mbms_service(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id, mbms_session_stop_request_pP->teid, NULL);
  }
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
void
 mce_app_handle_m3ap_enb_setup_request(
     itti_m3ap_enb_setup_req_t * const m3ap_enb_setup_req_p)
{
  OAILOG_FUNC_IN (LOG_MCE_APP);
  mbsfn_areas_t									mbsfn_areas = {0};

  /**
   * Check if an MBSFN area for the eNB exists, if so give it back.
   */
  mce_app_reset_m2_enb_registration(m3ap_enb_setup_req_p->m2ap_enb_id, m3ap_enb_setup_req_p->sctp_assoc);
  int local_mbms_service_area = mce_app_get_local_mbsfn_areas(&m3ap_enb_setup_req_p->mbms_service_areas,  m3ap_enb_setup_req_p->m2ap_enb_id, m3ap_enb_setup_req_p->sctp_assoc, &mbsfn_areas);
  mce_app_get_global_mbsfn_areas(&m3ap_enb_setup_req_p->mbms_service_areas, m3ap_enb_setup_req_p->m2ap_enb_id, m3ap_enb_setup_req_p->sctp_assoc, &mbsfn_areas, local_mbms_service_area);
  /** Send the resulting MBSFN areas for the eNB back. */
  mce_app_itti_m3ap_enb_setup_response(&mbsfn_areas, m3ap_enb_setup_req_p->m2ap_enb_id, m3ap_enb_setup_req_p->sctp_assoc);
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
void
mce_app_handle_mbsfn_mcch_repetition_timeout_timer_expiry ()
{
  OAILOG_FUNC_OUT (LOG_MCE_APP);

  mbsfn_area_context_t						*mbsfn_area_context 	= NULL;
  mbsfn_areas_t 									 mbsfn_areas 					= {0},
  																*mbsfn_areas_p 				= &mbsfn_areas;
  uint32_t 												 mcch_rep_rf 					= 0,
  																 mbsfn_mcch_mod_rf 		= 0;

	if(!pthread_rwlock_trywrlock(&mce_app_desc.rw_lock)) {
		OAILOG_ERROR(LOG_MCE_APP, "MCE APP: Could not retrieve the MCE desc lock. \n");
		OAILOG_FUNC_OUT(LOG_MCE_APP);
	}

  /**
   * Check which radio frame we are at.
   * Trigger for all pending MBSFN areas, which have %mod_rf = 0 --> MCCH scheduling.
   * Below is an absolute value of the MBSFN Sycn Area common MCCH repetition timer.
   */
	mce_app_desc.mcch_repetition_period++; // todo: wrap up to 0?
  mme_config_read_lock (&mme_config);
  mcch_rep_rf = mme_config.mbms.mbms_mcch_repetition_period_rf * mce_app_desc.mcch_repetition_period; /**< Your actual RF (also absolute). */
  memset((void*)&mce_app_desc.mcch_repetition_period_tv, 0, sizeof(mce_app_desc.mcch_repetition_period_tv));
  gettimeofday(&mce_app_desc.mcch_repetition_period_tv, NULL);
  mme_config_unlock (&mme_config);

  // todo: here determine how many MBSFN Areas you wan't to be scheduled!
  // todo: get a better approach, but currently we try to schedule all of them.
  // todo: check that it is same size as the hash table…
  	// todo: scheduling of the MBSFN areas depends on their local/global status..
// TODO: CHECK MBSFN SUBFRAMES
//	 * Subframe indicating the MCCH will be set at M2 Setup (initialization).
//	 * ToDo: It also must be unique against different MBSFN area combinations!
//	 *
  mbsfn_areas.num_mbsfn_areas_to_be_scheduled = mce_app_desc.mce_mbsfn_area_contexts.nb_mbsfn_area_managed;
	pthread_rwlock_unlock(&mce_app_desc.rw_lock);
  OAILOG_DEBUG(LOG_MCE_APP, "MCCH repetition timer is at absolute RF# (%d). Checking pending MBSFN areas with MCCH modification timeout. \n", mcch_rep_rf);

  /**
   * We don't recalculate resources, when they are removed. That's why use the CSA pattern calculation
   * in each MCCH modification period.
   * We iterate all MBSFNs and calculate the resources needed, and fill an CSA pattern offset bitmap.
   * Multiple CSA patterns may exist in one MBSFN area or among multiple MBSFN areas.
   * Each needs to have a different offset and we don't allow overlaps.
   */
  hashtable_ts_apply_callback_on_elements((hash_table_ts_t * const)mce_app_desc.mce_mbsfn_area_contexts.mbsfn_area_id_mbsfn_area_htbl,
   		mce_app_check_mbsfn_mcch_modif, (void*)&mcch_rep_rf, (void**)&mbsfn_areas_p);
  OAILOG_DEBUG(LOG_MCE_APP, "Found an MBSFN Area ID " MBSFN_AREA_ID_FMT " with an updated CSA pattern & MCCH modification period timeout. \n",
  		mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
  if(mbsfn_areas.num_mbsfn_areas) {

  	/**
  	 * We always calculate the overall CSA pattern from scratch, and don't check if the last CSA pattern was enough since, meanwhile, we might have removed some MBMS services.
  	 * Later on, we compare the CSA pattern with the stored one, to decide if we wan't to transmit or not?
  	 * 	ToDo: If we have multiple MBSFN areas, what if we don't tranmit one MBSFN area?? Last one counts?
  	 * ToDo: Before entering this method, check if the current MBSFN area CSA pattern is enough for the MCCH modification period.
  	 * If so don't change it, even if services are removed.
  	 */




  	OAILOG_INFO(LOG_MCE_APP, "Found (%d) MBSFN areas with modification for MCCH modification period RF (%d) timeout. \n",
  			mbsfn_areas.num_mbsfn_areas, mcch_rep_rf);
  	mce_app_itti_m3ap_send_mbms_scheduling_info(&mbsfn_areas_p, mcch_rep_rf);
  	// todo: guarantee, that we area bit before the MBSFN
  	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }
  OAILOG_DEBUG(LOG_MCE_APP, "No MBSFN changes for MCCH modification period RF (%).\n", mcch_rep_rf);
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
void
mce_app_handle_mbms_session_duration_timer_expiry (const struct tmgi_s *tmgi, const mbms_service_area_id_t mbms_service_area_id)
{
  OAILOG_FUNC_IN (LOG_MCE_APP);

  mbms_service_t								*mbms_service    		= NULL;
  mme_app_mbms_proc_t						*mme_app_mbms_proc 	= NULL;
  mbms_service_index_t					 mbms_service_idx   = INVALID_MBMS_SERVICE_INDEX;

  /**
   * Check that no MBMS Service with the given service ID and service area exists, if so reject
   * the request and implicitly remove the MBMS service & tunnel.
   */
  mbms_service = mce_mbms_service_exists_tmgi(&mce_app_desc.mce_mbms_service_contexts, tmgi, mbms_service_area_id);
  if(!mbms_service)
    OAILOG_FUNC_OUT (LOG_MCE_APP);

  if(!mbms_service->mbms_procedure) {
    /** The MBMS Service Area and PLMN are served by this MME. */
    OAILOG_ERROR(LOG_MCE_APP, "No MBMS Sm Procedure could be found for expired for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". \n", TMGI_ARG(tmgi), mbms_service_area_id);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }
  OAILOG_INFO (LOG_MCE_APP, "Expired- Sm Procedure Timer for MBMS Service with TMGI " TMGI_FMT ". Check the procedure. \n", TMGI_ARG(tmgi));

  /**
   * We don't check if the MBMS Session has been successfully established. We don't know the MBMS Absolute Start time
   * and don't wan't to be dependent on it. We immediately respond to the MBMS-GW, instead.
   */
  NOT_REQUIREMENT_3GPP_23_246(R8_3_2__6);

  /**
   * We check if the timeout was short enough to trigger an implicit MBMS session stop.
   */
  REQUIREMENT_3GPP_23_246(R4_4_2__6);
  if(mme_app_mbms_proc->trigger_mbms_session_stop) {
    OAILOG_INFO (LOG_MCE_APP, "Triggering MBMS Session stop for expired MBMS Session duration for MBMS service with TMGI " TMGI_FMT ". \n", TMGI_ARG(tmgi));
    /*
     * todo: Need to get a better architecture here, kind of don't know what I am doing considering the locks (where to use the object, where to use identifiers).
     * Reason for that mainly is, what the final lock/structure architecture will look like..
     * Remove the Sm tunnel endpoint implicitly and all MBMS Service References in the M2 layer.
     * No need to inform the MBMS Gateway. Will inform the M2 layer and all the eNBs.
     */
    OAILOG_INFO(LOG_MCE_APP, "MBMS Service stopped after timeout. This only applies if the Stopping the MBMS Service for TMGI " TMGI_FMT " immediately. Informing the MCE over M3. \n", TMGI_ARG(&mbms_service->privates.fields.tmgi));
    /** M3AP Session Stop Request --> No Response is expected. Immediately terminate the MBMS Service afterwards. */
    mce_app_itti_m3ap_mbms_session_stop_request(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id, true);
    mce_app_stop_mbms_service(tmgi, mbms_service_area_id, mbms_service->privates.fields.mme_teid_sm, (struct sockaddr*)&mbms_service->privates.fields.mbms_peer_ip);
  } else {
    /**
     * We let the MBMS Session continue to exist.
     * Expect a removal from the MBMS-GW.
     * Just remove the MBMS-Procedure.
     */
    mme_app_delete_mbms_procedure(mbms_service);
  }
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/
//------------------------------------------------------------------------------
static void mce_app_stop_mbms_service(tmgi_t * tmgi, mbms_service_area_id_t mbms_sa_id, teid_t mme_sm_teid, struct sockaddr * mbms_peer_ip) {
  OAILOG_FUNC_IN(LOG_MCE_APP);
  DevAssert(tmgi);
  DevAssert(mbms_sa_id != INVALID_MBMS_SERVICE_AREA_ID);

  OAILOG_INFO(LOG_MCE_APP, "Clearing MBMS Service with TMGI " TMGI_FMT " and MBMS-Service-Area ID " MBMS_SERVICE_AREA_ID_FMT". \n", TMGI_ARG(tmgi), mbms_sa_id);
  /**
   * Inform the M2AP Layer about the removed MBMS Service.
   * No negative response will arrive, so continue.
   */
  if(mbms_peer_ip){
    /** Remove the Sm Tunnel Endpoint implicitly. */
  	mce_app_remove_sm_tunnel_endpoint(mme_sm_teid, mbms_peer_ip);
  }
  mce_app_remove_mbms_service(tmgi, mbms_sa_id, mme_sm_teid);
  OAILOG_FUNC_OUT(LOG_MCE_APP);
}

/**
 * Check the resources for the MCCH modification period, where the MBMS service will be activated.
 * Method will not remove MBMS services, only return a set of MBMS services which need to be removed,
 * if the resources of the MBSFN area are not enough.
 * Since there is no MBMS stop time but a session duration, we will just add and offset to the MBMS Session Start time, if the next MCCH modification period has been missed.
 *
 *
 * Can call this method after it has been registered & updated.
 * @param: mbms_service_idx & mbms_sai are the parameters of the new MBMS service to be checked
 */
//------------------------------------------------------------------------------
static
int mce_app_check_resources(const mbms_service_index_t mbms_service_index, const mbms_service_area_id_t mbms_service_area_id, long *mcch_modif_periods) {
	OAILOG_FUNC_IN(LOG_MCE_APP);

	mbsfn_areas_t					 mbsfn_areas 						= {0},
									 	 	  *mbsfn_areas_p 					= &mbsfn_areas;
	mbsfn_area_context_t  *mbsfn_area_context 		= NULL;
	mbms_services_t 			 mbms_services_active 	= {0},
												*mbms_services_active_p = &mbms_services_active;
	mbms_services_t 			 mbms_services_tbr		 	= {0},
												*mbms_services_tbr_p		= &mbms_services_tbr;

	/**
	 * Check the resources dedicated to the MBSFN area.
	 * The MBMS service in question might be returned due ARP, if so reject it to the MBMS-GW.
	 * Remove any other MBMS Services implicitly in the M2AP layer.
	 */
	mme_config_read_lock (&mme_config);
	mbms_service_t * mbms_services_active_array[mme_config.mbms.max_mbms_services];
	memset(&mbms_services_active_array, 0, (sizeof(mbms_service_t*) * mme_config.mbms.max_mbms_services));
	mbms_services_active.mbms_service_array = mbms_services_active_array;
	mme_config_unlock(&mme_config);

  /** Check the actual MBSFN area for the lowest ARP and redo the total capacity check again. */
	mbsfn_area_context = mce_mbsfn_area_exists_mbms_service_area_id(&mce_app_desc.mce_mbsfn_area_contexts.mbms_sai_mbsfn_area_ctx_htbl, mbms_service_area_id);
  DevAssert(mbsfn_area_context);

  /**
   * We don't recalculate resources, when they are removed. That's why use the CSA pattern calculation in each MCCH modification period.
   * We iterate all MBSFNs and calculate the resources needed, and fill an CSA pattern offset bitmap.
   * Multiple CSA patterns may exist in one MBSFN area or among multiple MBSFN areas.
   * Each needs to have a different offset and we don't allow overlaps.
   *
   * Return the given MBSFN Area context, which could not be fit in into the eNB band.
   *
   * TODO: CHECK only MBSFN Areas which will be shared!
   *
   *
	 * Subframe indicating the MCCH will be set at M2 Setup (initialization).
	 * ToDo: It also must be unique against different MBSFN area combinations!
	 *
   */
  hashtable_ts_apply_callback_on_elements((hash_table_ts_t * const)mce_app_desc.mce_mbsfn_area_contexts.mbsfn_area_id_mbsfn_area_htbl,
  		mce_app_check_mbsfn_resources, (void*)&mcch_modif_periods, (void**)&mbsfn_areas_p);
  /**
   * Check if resources could be scheduled to the MBSFN areas.
   */
  if(mbsfn_areas_p->num_mbsfn_areas == mbsfn_areas.num_mbsfn_areas_to_be_scheduled){
  	/** We could schedule all MBSFN areas, including the latest received MBMS service. */
  	OAILOG_INFO(LOG_MCE_APP, "MBSFN resources are enough to activate new MBMS Service Id " MBMS_SERVICE_INDEX_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " together with (%d) MBSFN areas. \n",
  			mbms_service_index, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id, mbsfn_areas_p->num_mbsfn_areas);
  	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
  }

  /** Check all MBSFN areas until all MBMS Services fit in. */
  while(mbsfn_areas_p->num_mbsfn_areas != mbsfn_areas.num_mbsfn_areas_to_be_scheduled){
    OAILOG_ERROR(LOG_MCE_APP, "Out of (%d) MBSFN areas, only (%d) MBSFN areas could be scheduled. "
    		"Removing MBMS service in the currently checked MBSFN Area Id " MBSFN_AREA_ID_FMT". \n",
  			mbsfn_areas.num_mbsfn_areas_to_be_scheduled, mbsfn_areas.num_mbsfn_areas, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);

    /** Need to calculate for the given MBSFN area the MCHs again and check the resources. */
    for(int num_mbsfn_area = 0; num_mbsfn_area < mbsfn_areas.num_mbsfn_areas; num_mbsfn_area++){
    	/** Get the active MBMS services for the MBSFN area. */
    	long parametersP[3] = {mcch_modif_periods[0], mcch_modif_periods[1], mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id};
    	hashtable_ts_apply_callback_on_elements((hash_table_ts_t * const)mce_app_desc.mce_mbms_service_contexts.mbms_service_index_mbms_service_htbl,
    			mce_app_get_active_mbms_services, (void*)parametersP, (void**)&mbms_services_active_p);
    }

    /**<
     * Remove the MBMS service with the lowest ARP for the given MCCH modification period.
     * Returns the service to be removed from the index.
     */
    int mbms_service_active_list_idx = mce_app_mbms_arp_preempt(mbms_services_active_p);
    if(mbms_service_active_list_idx == -1) {
    	OAILOG_ERROR(LOG_MCE_APP, "Error while ARP preemption while checking resources for MCCH modification period (%d) for MBSFN Area "MBSFN_AREA_ID_FMT". \n",
    			mcch_modif_periods[0], mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
    	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
    }
    mbms_service_t * mbms_service_tbr = mbms_services_active_p->mbms_service_array[mbms_service_active_list_idx];
    DevAssert(mbms_service_tbr);
    /**
     * Check if the MBMS Service, which was sorted out, is the newly received MBMS service.
     * If so, return false.
     */
    mbms_service_index_t mbms_service_index_tbr = mce_get_mbms_service_index(&mbms_service_tbr->privates.fields.tmgi, mbms_service_tbr->privates.fields.mbms_service_area_id);
    if(mbms_service_index== mbms_service_index_tbr){
    	/**
    	 * Stop and remove the MBMS service.
    	 * Explicitly send the Session Start Response. No Sm Tunnels should be built!
    	 */
    	DevAssert(!mbms_services_tbr.num_mbms_service); /**< Should be the first and only MBMS service, that is to be removed. */
    	OAILOG_ERROR(LOG_MCE_APP, "No resources for newly received MBMS Service with TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". \n",
    			TMGI_ARG(&mbms_service_tbr->privates.fields.tmgi), mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id);
    	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
    }
    /**
     * Service, which was removed not the newly received service. Add the MBMS Service to the TBR list and continue to check the overlal capacity.
     * Service will then be removed implicitly and we will continue with the newly received MBMS service.
     */
    mbms_services_active_p->mbms_service_array[mbms_service_active_list_idx] = NULL;
    OAILOG_WARNING(LOG_MCE_APP, "Adding MBMS Service with idx (%d) and TMGI " TMGI_FMT " into list of MBMS services to be removed.\n",
    		mbms_service_active_list_idx, TMGI_ARG(&mbms_service_tbr->privates.fields.tmgi));
    mbms_services_tbr_p->mbms_service_array[mbms_services_tbr_p->num_mbms_service++] = mbms_service_tbr;

    hashtable_ts_apply_callback_on_elements((hash_table_ts_t * const)mce_app_desc.mce_mbsfn_area_contexts.mbsfn_area_id_mbsfn_area_htbl,
    		mce_app_check_mbsfn_resources, (void*)&mcch_modif_periods, (void**)&mbsfn_areas_p);
  }

	/** We could schedule all MBSFN areas, including the latest received MBMS service. */
 	OAILOG_INFO(LOG_MCE_APP, "After preempting {} service, finally MBSFN resources are enough to activate new MBMS Service Id " MBMS_SERVICE_INDEX_FMT " "
 			"and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " together with (%d) MBSFN areas. \n", mbms_service_index, mbsfn_area_context->privates.fields.mbsfn_area.mbsfn_area_id, mbsfn_areas_p->num_mbsfn_areas);

 	/** At least once service should exist, which needs to be removed.. */
 	DevAssert(mbms_services_tbr.num_mbms_service);
 	/**
 	 * We don't know in each MCCH modification period, if services where removed.
 	 * That's why we need currently to calculate the CSA subframes in each MCCH modification period.
 	 */
 	OAILOG_WARNING(LOG_MCE_APP, "Found (%d) MBMS services to be removed.\n", mbms_services_tbr.num_mbms_service);
 	for(int i = 0; i < mbms_services_tbr.num_mbms_service; i++){
 		mbms_service_t *mbms_service_tbr = mbms_services_tbr.mbms_service_array[i];
 		if(mbms_service_tbr){
 			OAILOG_WARNING(LOG_MCE_APP, "MBMS Service for TMGI " TMGI_FMT " with ARP prio (%d) will be removed for new MBMS Service request with MBMS Service Index "MBMS_SERVICE_INDEX_FMT". \n",
 					TMGI_ARG(&mbms_service_tbr->privates.fields.tmgi), mbms_service_tbr->privates.fields.mbms_bc.eps_bearer_context.bearer_level_qos.pl, mbms_service_index);
 			/** Remove the old MBMS session. */
 			mce_app_itti_m3ap_mbms_session_stop_request(&mbms_service_tbr->privates.fields.tmgi, mbms_service_tbr->privates.fields.mbms_service_area_id, 0);
 			mce_app_stop_mbms_service(&mbms_service_tbr->privates.fields.tmgi,
 					mbms_service_tbr->privates.fields.mbms_service_area_id, mbms_service_tbr->privates.fields.mme_teid_sm, NULL);
 			/**
 			 * No need to calculate the new allocated resources.
 			 * Will be done with the upcoming MCCH modification period timeout.
 			 */
 		}
 	}
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

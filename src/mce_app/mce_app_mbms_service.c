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
int mce_app_check_mbms_service_resources(const mbms_service_index_t mbms_service_index, const mbms_service_area_id_t mbms_service_area_id, const mbsfn_area_ids_t * const mbsfn_area_ids);

//------------------------------------------------------------------------------
static
int mce_app_check_shared_resources(const mbms_service_index_t mbms_service_index, const mbsfn_area_id_t mbsfn_area_id, mbms_service_indexes_t * const mbms_service_indexes_tbr);

//------------------------------------------------------------------------------
static
int mce_app_check_mbsfn_cluster_capacity(const mbms_service_index_t mbms_service_index, const mbsfn_area_id_t mbsfn_area_id,
		const mbsfn_area_ids_t	  		*const mbsfn_area_ids_nlg_p,
		const mbsfn_area_ids_t				*const mbsfn_area_ids_local_p,
		mbms_service_indexes_t				*const mbms_service_indexes_active_nlg_p,
		mbms_service_indexes_t				*const mbms_service_indexes_active_local_p,
		mbms_service_indexes_t 				*const mbms_services_tbr);

//------------------------------------------------------------------------------
static
int mce_app_update_mbsfn_area_registration(const mbms_service_index_t new_mbms_service_index, const mbms_service_index_t old_mbms_service_index,
		const uint32_t sec_since_epoch, const long double usec_since_epoch, const mbms_session_duration_t * mbms_session_duration,
		const mbsfn_area_ids_t * const mbsfn_area_ids);

//------------------------------------------------------------------------------
void
mce_app_handle_mbms_session_start_request(
     itti_sm_mbms_session_start_request_t * const mbms_session_start_request_pP
    )
{
  OAILOG_FUNC_IN (LOG_MCE_APP);

  MessageDef                             *message_p  									= NULL;
  teid_t 								 									mme_sm_teid									= INVALID_TEID;
  mbms_service_t 						 						 *mbms_service 								= NULL;
  mbms_service_index_t 					  				new_mbms_service_idx      	= INVALID_MBMS_SERVICE_INDEX;
  mme_app_mbms_proc_t				 	 					 *mbms_sm_proc 								= NULL;
  mbms_service_area_id_t 			      			mbms_service_area_id 				= INVALID_MBMS_SERVICE_AREA_ID;
  mbsfn_area_ids_t												mbsfn_area_ids							= {0};
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
   * If multiple MBMS Service Areas are received, we will just consider the first one. We check if any eNBs for the respective MBSFN area are attached, due to split of layers.
   *
   * MME may have received consecutive multiple MBMS Service Areas, but we will require MBMS Flow Identifier.
   * Since the MBMS Flow Identifier is set as location (MBMS Service Area) of an MBMS Service, we will only consider one MBMS Service Area for the MBMS Service.
   * We will register the given MBMS Service (TMGI) with the MBMS Service Area uniquely.
   */
  if ((mbms_service_area_id = mce_app_check_mbms_sa_exists(&mbms_session_start_request_pP->tmgi.plmn, &mbms_session_start_request_pP->mbms_service_area) == INVALID_MBMS_SERVICE_AREA_ID)) {
    /**
     * The MBMS Service Area and PLMN are served by this MME.
     */
    OAILOG_ERROR(LOG_MCE_APP, "PLMN " PLMN_FMT " or none of the Target MBMS SAs are served by current MME. Rejecting MBMS Session Start Request for TMGI "TMGI_FMT".\n",
    	PLMN_ARG(&mbms_session_start_request_pP->tmgi.plmn), TMGI_ARG(&mbms_session_start_request_pP->tmgi));
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
    /** No MBMS service or tunnel endpoint is allocated yet. */
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * Check that MBSFN area for the MBMS service area is existing.
   * Multiple MBSFN Areas might be active for one MBMS Service Area Id, if the local-global flag is active
   * (Depending on eNBs which have a location information, and others which don't.
   */
  mce_mbsfn_areas_exists_mbms_service_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbms_service_area_id, &mbsfn_area_ids);
  if(!mbsfn_area_ids.num_mbsfn_area_ids){
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
  if(mbms_session_start_request_pP->mbms_session_duration.seconds < mme_config.mbms.mbms_mcch_modification_period_rf * 10000) {
    OAILOG_WARNING(LOG_MCE_APP, "MBM Session duration (%ds) is shorter than the minimum (%ds) for MBMS Service Request for TMGI " TMGI_FMT " (MCCH modify). Setting to minval. \n",
    	mbms_session_start_request_pP->mbms_session_duration.seconds, mme_config.mbms.mbms_mcch_modification_period_rf * 10000, TMGI_ARG(&mbms_session_start_request_pP->tmgi));
    mbms_session_start_request_pP->mbms_session_duration.seconds = mme_config.mbms.mbms_mcch_modification_period_rf * 10000;
  }
  mme_config_unlock (&mme_config);

  /** Check that the MBMS Bearer QoS is a valid GBR. */
  if(!is_qci_gbr(mbms_session_start_request_pP->mbms_bearer_level_qos.qci)){
    /** Return error, no modification on non-GBR is allowed. */
  	OAILOG_ERROR(LOG_MCE_APP, "Non-GBR MBMS Bearer Level QoS (QCI=%d) not supported. Rejecting MBMS Service Request for TMGI " TMGI_FMT ". \n",
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

  OAILOG_INFO(LOG_MCE_APP, "Successfully processed the request for a new MBMS Service for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT". Continuing with the resource calculation. \n",
  		TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);

  mme_sm_teid = __sync_fetch_and_add (&mce_app_mme_sm_teid_generator, 0x00000001);
  OAILOG_INFO(LOG_MCE_APP, "Successfully processed the request for a new MBMS Service for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT". Continuing with the establishment. \n",
    		TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);

  /** Register the MBMS service in the hashmap, not in mbsfn areas yet. */
  if (!mce_register_mbms_service(&mbms_session_start_request_pP->tmgi, mbms_service_area_id, mme_sm_teid)) {
  	OAILOG_ERROR(LOG_MCE_APP, "No MBMS Service context could be created and stored from TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". \n",
  			TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);
  	mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, SYSTEM_FAILURE);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /** Update the MBMS Service with the given parameters.
   * Will calculate the MCCH modification periods and insert them in the MBSFN areas. */
  mce_app_update_mbms_service(&mbms_session_start_request_pP->tmgi, mbms_service_area_id, mbms_service_area_id, &mbms_session_start_request_pP->mbms_bearer_level_qos,
  		mbms_session_start_request_pP->mbms_flow_id, &mbms_session_start_request_pP->mbms_ip_mc_address, &mbms_session_start_request_pP->mbms_peer_ip);

  /**
   * Update the MBMS service in the MBSFN areas.
   * MBMS services, where the MBMS area id has changed, will be removed from the MBSFN areas and deactivated in the eNBs immediately.
   */
  new_mbms_service_idx = mce_get_mbms_service_index(&mbms_session_start_request_pP->tmgi, mbms_service_area_id);
  if(mce_app_update_mbsfn_area_registration(new_mbms_service_idx, INVALID_MBMS_SERVICE_INDEX,
  		mbms_session_start_request_pP->abs_start_time.sec_since_epoch,
			mbms_session_start_request_pP->abs_start_time.usec,
			&mbms_session_start_request_pP->mbms_session_duration,
			&mbsfn_area_ids) == RETURNerror)
  {
  	/** Error updating the MBMS service in the MBSFN areas. */
  	OAILOG_ERROR(LOG_MCE_APP, "Resource check for updated MBMS Service context with TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " failed. "
  			"Rejecting Sm MBMS Session Start Request. \n", TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);
  	mce_app_stop_mbms_service(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id, mme_sm_teid, NULL);
    mce_app_itti_sm_mbms_session_stop_response(mbms_session_start_request_pP->teid, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * If we have multiple MBSFN areas, check for their resources separately.
   */
  if(!mce_app_check_mbms_service_resources(new_mbms_service_idx, mbms_service_area_id, &mbsfn_area_ids)){
  	OAILOG_ERROR(LOG_MCE_APP, "Resource check for new MBMS Service context with TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " failed. "
  			"Rejecting Sm MBMS Session Start Request. \n", TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);
  	mce_app_stop_mbms_service(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id, mme_sm_teid, NULL);
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, SYSTEM_FAILURE);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * At least in one group, we could allocated the resources. We might have removed some other MBMS areas implicitly.
   * Update the MBMS session start time, if the MCCH modification period has been missed.
   * Check the difference between the absolute start time and the current time.
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
  mbms_service_t 												 *mbms_service 							= NULL;
  mme_app_mbms_proc_t					 					 *mbms_sm_proc 							= NULL;
  mbms_service_index_t 									  old_mbms_service_idx	   	= INVALID_MBMS_SERVICE_INDEX;
  teid_t 								  								mme_sm_teid								= INVALID_TEID;
  mbms_service_area_id_t 			      			new_mbms_service_area_id 	= INVALID_MBMS_SERVICE_AREA_ID,
  																				old_mbms_service_area_id 	= INVALID_MBMS_SERVICE_AREA_ID;
  mbsfn_area_ids_t												mbsfn_area_ids						= {0};
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
  if(mbms_session_update_request_pP->mbms_session_duration.seconds < mme_config.mbms.mbms_mcch_modification_period_rf * 10000) {
    OAILOG_WARNING(LOG_MCE_APP, "MBM Session duration (%ds) is shorter than the minimum (%ds) for MBMS Service Request for TMGI " TMGI_FMT " (MCCH modify). Setting to minval. \n",
    	mbms_session_update_request_pP->mbms_session_duration.seconds, mme_config.mbms.mbms_mcch_modification_period_rf * 10000, TMGI_ARG(&mbms_session_update_request_pP->tmgi));
    mbms_session_update_request_pP->mbms_session_duration.seconds = mme_config.mbms.mbms_mcch_modification_period_rf * 10000;
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
   * Multiple MBSFN Areas might be active for one MBMS Service Area Id
   */
  mce_mbsfn_areas_exists_mbms_service_area_id(&mce_app_desc.mce_mbsfn_area_contexts, new_mbms_service_area_id, &mbsfn_area_ids);
  if(!mbsfn_area_ids.num_mbsfn_area_ids){
  	OAILOG_ERROR(LOG_MCE_APP, "No MBSFN Area context for MBMS SAI "MBMS_SERVICE_AREA_ID_FMT" could be found. Rejecting MBMS Session Update Request for TMGI "TMGI_FMT".\n",
  			new_mbms_service_area_id, TMGI_ARG(&mbms_session_update_request_pP->tmgi));
  	mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid, mbms_session_update_request_pP->sm_mbms_fteid.teid, &mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, REQUEST_REJECTED);
  	/** No MBMS service or tunnel endpoint is allocated yet. */
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
   *
   * TODO: REMOVE ANY MBMS service, immediately, if the MBMS service area id has changed.
   */
  old_mbms_service_area_id = mbms_service->privates.fields.mbms_service_area_id;
  old_mbms_service_idx = mce_get_mbms_service_index(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id);
  mbms_service_index_t new_mbms_service_index = mce_get_mbms_service_index(&mbms_session_update_request_pP->tmgi, new_mbms_service_area_id);
  mce_app_update_mbms_service(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id, new_mbms_service_area_id,
  		&mbms_session_update_request_pP->mbms_bearer_level_qos, mbms_service->privates.fields.mbms_flow_id, NULL, NULL);

  /**
   * Update the MBMS service in the MBSFN areas.
   * MBMS services, where the MBMS area id has changed, will be removed from the MBSFN areas and deactivated in the eNBs immediately.
   */

  if(mce_app_update_mbsfn_area_registration(old_mbms_service_idx, new_mbms_service_index,
  		mbms_session_update_request_pP->abs_update_time.sec_since_epoch,
			mbms_session_update_request_pP->abs_update_time.usec,
			&mbms_session_update_request_pP->mbms_session_duration,
			&mbsfn_area_ids) == RETURNerror)
  {
  	/** Error updating the MBMS service in the MBSFN areas. */
  	OAILOG_ERROR(LOG_MCE_APP, "Resource check for updated MBMS Service context with TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " failed. "
  			"Rejecting Sm MBMS Session Update Request. \n", TMGI_ARG(&mbms_session_update_request_pP->tmgi), new_mbms_service_area_id);
  	mce_app_stop_mbms_service(&mbms_service->privates.fields.tmgi,
  			mbms_service->privates.fields.mbms_service_area_id, mme_sm_teid, NULL);
    mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid, mbms_session_update_request_pP->sm_mbms_fteid.teid, &mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, REQUEST_REJECTED);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * Check the resources also in the MBMS Session Update case.
   */
  if(mce_app_check_mbms_service_resources(new_mbms_service_index, new_mbms_service_area_id, &mbsfn_area_ids) == RETURNerror){
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
  mce_app_itti_m3ap_enb_setup_response(&mbsfn_areas,local_mbms_service_area, m3ap_enb_setup_req_p->m2ap_enb_id, m3ap_enb_setup_req_p->sctp_assoc);
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
void
mce_app_handle_mbsfn_mcch_repetition_timeout_timer_expiry (hash_table_ts_t * const mcch_mbsfn_cfg_htbl)
{
  OAILOG_FUNC_OUT (LOG_MCE_APP);

	struct csa_patterns_s 					 csa_patterns_global 											= {0};
	long		 												 mcch_rep_abs_rf													= 0;
	mbms_service_indexes_t				 	 mbms_service_indexes_active_nlg 					= {0},
																	*mbms_service_indexes_active_nlg_p 				= &mbms_service_indexes_active_nlg;

	if(!pthread_rwlock_trywrlock(&mce_app_desc.rw_lock)) {
		OAILOG_ERROR(LOG_MCE_APP, "MCE APP: Could not retrieve the MCE desc lock. \n");
		OAILOG_FUNC_OUT(LOG_MCE_APP);
	}

	/**
	 * Check which radio frame we are at.
	 * Trigger for all pending MBSFN areas, which have %mod_rf = 0 --> MCCH scheduling.
	 * Below is an absolute value of the MBSFN Sycn Area common MCCH repetition timer.
	 */
	mme_config_read_lock (&mme_config);
	uint8_t mbms_global_mbsfn_area_per_local_group = mme_config.mbms.mbms_global_mbsfn_area_per_local_group;
	uint8_t max_mbms_local_areas = mme_config.mbms.mbms_local_service_areas;
	mbsfn_area_ids_t 	mbsfn_area_id_clusters[mme_config.mbms.mbms_local_service_areas +1]; /**< O: non-local global MBMS areas. */
	memset((void*)mbsfn_area_id_clusters, 0, sizeof(mbsfn_area_id_clusters));
	mbsfn_areas_t mbsfn_clusters_to_schedule[mme_config.mbms.mbms_local_service_areas +1];
	memset((void*)mbsfn_clusters_to_schedule, 0, sizeof(mbsfn_clusters_to_schedule));
	/** Set the MBSFN areas to 0. */
	mce_app_desc.mcch_repetition_period++; // todo: wrap up to 0?
	mcch_rep_abs_rf = (long)(mme_config.mbms.mbms_mcch_repetition_period_rf * mce_app_desc.mcch_repetition_period); /**< Your actual RF (also absolute). */
	memset((void*)&mce_app_desc.mcch_repetition_period_tv, 0, sizeof(mce_app_desc.mcch_repetition_period_tv));
	gettimeofday(&mce_app_desc.mcch_repetition_period_tv, NULL);
	pthread_rwlock_unlock(&mce_app_desc.rw_lock);
	mme_config_unlock(&mme_config);

	OAILOG_DEBUG(LOG_MCE_APP, "MCCH repetition timer is at absolute RF# (%u). Checking pending MBSFN areas with MCCH modification timeout. \n", mcch_rep_abs_rf);
	/**
	 * Collect all MBSFNs into clusters depending on the local MBMS area.
	 */
	hashtable_ts_apply_callback_on_elements((hash_table_ts_t * const)mce_app_desc.mce_mbsfn_area_contexts.mbsfn_area_id_mbsfn_area_htbl,
			mce_app_get_mbsfn_groups, (void*)NULL, (void**)&mbsfn_area_id_clusters);

	/**
	 * For each group, we assume same PHY properties and MCCH modification timeouts.
	 * Furthermore, if we have no local global flag active, the properties are also shared with the non-local global MBSFN areas.
	 * Thus: all physical properties are same (or no global MBSFN areas exist).
	 * First do the non-local global MBSFN areas.
	 */

	mcch_modification_periods_t mcch_modification_periods_nlg 	= {0};
	mcch_modification_periods_t mcch_modification_periods_local = {0};
	/** Collect the non-local global active MBMS services. */
	for( int num_mbsfn_nlg = 0; num_mbsfn_nlg < mbsfn_area_id_clusters[0].num_mbsfn_area_ids; num_mbsfn_nlg++){
		mbsfn_area_context_t * mbsfn_area_context_nlg_p = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_id_clusters[0].mbsfn_area_id[num_mbsfn_nlg]);
		DevAssert(mbsfn_area_context_nlg_p);
		/** Get the MCCH modification start ABS period. */
		mcch_modification_periods_nlg.mcch_modif_start_abs_period = mcch_rep_abs_rf / mbsfn_area_context_nlg_p->privates.fields.mbsfn_area.mcch_modif_period_rf;
		mcch_modification_periods_nlg.mcch_modif_stop_abs_period 	= mcch_rep_abs_rf / mbsfn_area_context_nlg_p->privates.fields.mbsfn_area.mcch_modif_period_rf;
		/**
		 * No matter if the MCCH modification boundary has been reached or not. Calculate the resources assigned for the services of the MBSFN areas.
		 * At the end, send the message for MCCH modification boundary MBSFN areas.
		 */
		hashtable_apply_callback_on_elements(mbsfn_area_context_nlg_p->privates.mbms_service_idx_mcch_modification_times_hashmap,
				mce_app_get_active_mbms_services_per_mbsfn_area, (void*)&mcch_modification_periods_nlg, (void**)&mbms_service_indexes_active_nlg_p);
	}
	if(mbsfn_area_id_clusters[0].num_mbsfn_area_ids){
		/**
		 * No matter if services scheduled or not, schedule resources for the MBSNF area.
		 * We should have checked beforehand, if the capacity overall is enough. So no capacity errors are expected.
		 */
		if(mce_app_check_mbsfn_cluster_resources(&mbsfn_area_id_clusters[0], NULL, &mbms_service_indexes_active_nlg, NULL, &mbsfn_clusters_to_schedule[0]) == RETURNerror){
			OAILOG_ERROR(LOG_MCE_APP,"Error assigning the resources for the (%d) non-local global MBSFN areas. Resources should be validated for the MBMS service at service request time!\n", mbsfn_area_id_clusters[0].num_mbsfn_area_ids);
			DevAssert(0);
		}
	}
	OAILOG_INFO(LOG_MCE_APP,"Successfully scheduled given (%d) non-local global MBSFN areas. Checking the local MBSFN areas.\n",  mbsfn_area_id_clusters[0].num_mbsfn_area_ids);
	/**
	 * If the non-local global flag is active, the above calculated resources will be taken into account for each local MBMS area, too.
	 */
	for(int num_local_mbms_area = 1; num_local_mbms_area < max_mbms_local_areas + 1; num_local_mbms_area++)
	{
		mbms_service_indexes_t				 	 mbms_service_indexes_active_local				= {0},
																		*mbms_service_indexes_active_local_p			= &mbms_service_indexes_active_local;
		/** Check if MBSFN areas with resources exist for given local MBMS area. */
		if(!mbsfn_area_id_clusters[num_local_mbms_area].num_mbsfn_area_ids){
			OAILOG_DEBUG(LOG_MCE_APP, "No MBSFN areas for scheduling in local MBMS area (%d).\n", num_local_mbms_area);
			continue;
		}
		for(int num_mbsfn_local = 0; num_mbsfn_local < mbsfn_area_id_clusters[0].num_mbsfn_area_ids; num_mbsfn_local++){
			mbsfn_area_context_t * mbsfn_area_context_local_p = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_id_clusters[num_local_mbms_area].mbsfn_area_id[num_mbsfn_local]);
			DevAssert(mbsfn_area_context_local_p);
			/** Get the MCCH modification start ABS period. */
			mcch_modification_periods_local.mcch_modif_start_abs_period = mcch_rep_abs_rf / mbsfn_area_context_local_p->privates.fields.mbsfn_area.mcch_modif_period_rf;
			mcch_modification_periods_local.mcch_modif_stop_abs_period 	= mcch_rep_abs_rf / mbsfn_area_context_local_p->privates.fields.mbsfn_area.mcch_modif_period_rf;
			/**
			 * No matter if the MCCH modification boundary has been reached or not. Calculate the resources assigned for the services of the MBSFN areas.
			 * At the end, send the message for MCCH modification boundary MBSFN areas.
			 */
			hashtable_apply_callback_on_elements(mbsfn_area_context_local_p->privates.mbms_service_idx_mcch_modification_times_hashmap,
					mce_app_get_active_mbms_services_per_mbsfn_area, (void*)&mcch_modification_periods_local, (void**)&mbms_service_indexes_active_local_p);
		}
		/**
		 * Calculate for the summed area the capacity and set the MCH subframes.
		 * Take into account the local/global flag.
		 */
		if(mce_app_check_mbsfn_cluster_resources(
				(mbms_global_mbsfn_area_per_local_group ? NULL : &mbsfn_area_id_clusters[0]),
				&mbsfn_area_id_clusters[num_local_mbms_area],
				(mbms_global_mbsfn_area_per_local_group ? NULL : &mbms_service_indexes_active_nlg),
				mbms_service_indexes_active_local_p,
				/** Assign the resources to the MBMS local area index. */
				&mbsfn_clusters_to_schedule[num_local_mbms_area]) == RETURNerror){
			OAILOG_ERROR(LOG_MCE_APP,"Error assigning the resources for the (%d) local MBSFN areas in local MBMS area (%d). Resources should be validated for the MBMS service at service request time!\n", mbsfn_area_id_clusters[num_local_mbms_area].num_mbsfn_area_ids, num_local_mbms_area);
			DevAssert(0);
		}
		OAILOG_INFO(LOG_MCE_APP,"Successfully scheduled (%d) local MBSFN areas. Done with the MBSFN cluster for local MBMS area (%d).\n", mbsfn_area_id_clusters[num_local_mbms_area].num_mbsfn_area_ids, num_local_mbms_area);
	}

	/**
	 * We have multiple MBSFN clusters, whose MCCH modification period may have reached.
	 * Send them all to the M2AP layer. Each eNB will be informed about the MBMS Scheduling based on the local MBMS area.
	 * Check from the received MBSFN area clusters, which are changed in comparison to its last MCCH modification period.
	 * Get the timer argument.
	 */
	uint8_t mbsfn_areas_to_be_scheduled = 0;
	for(int mbms_local_area = 0; mbms_local_area < max_mbms_local_areas + 1; mbms_local_area++) {
		mbsfn_areas_t * mbsfn_areas_ts = &mbsfn_clusters_to_schedule[mbms_local_area];
		for(int num_mbsfn_area = 0; num_mbsfn_area < mbsfn_areas_ts->num_mbsfn_areas; num_mbsfn_area++) {
			mbsfn_area_id_t 	mbsfn_area_id = mbsfn_clusters_to_schedule[mbms_local_area].mbsfn_area_cfg[num_mbsfn_area].mbsfnArea.mbsfn_area_id;
			/** Check if the MCCH modification boundary has been reached. */
			mbsfn_area_context_t * mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_id);
			if(mcch_rep_abs_rf % mbsfn_area_context->privates.fields.mbsfn_area.mcch_modif_period_rf){
				OAILOG_DEBUG(LOG_MCE_APP, "MBSFN Area Id " MBSFN_AREA_ID_FMT " in local MBMS area (%d) has not reached MCCH modification boundary for MCCH repetition RF (%d). Not considering for MBMS scheduling. \n",
						mbsfn_area_id, mbms_local_area, mcch_rep_abs_rf);
				/** Set the MBSFN area id to invalid. */
				mbsfn_clusters_to_schedule[mbms_local_area].mbsfn_area_cfg[num_mbsfn_area].mbsfnArea.mbsfn_area_id = INVALID_MBSFN_AREA_ID;
				continue;
			}
			/**
			 * Although resources are reserved, don't transmit MBMS Scheduling information for MBSFN Areas which don't have any active MBMS services,
			 * since M2AP required MBMS session list as mandatory field.
			 */
			if(!mbsfn_clusters_to_schedule[mbms_local_area].mbsfn_area_cfg[num_mbsfn_area].mchs.total_subframes_per_csa_period_necessary){
				OAILOG_DEBUG(LOG_MCE_APP, "MBSFN Area Id " MBSFN_AREA_ID_FMT " in local MBMS area (%d) has reached MCCH modification boundary for MCCH repetition RF (%d), but no active MBSFN services. Not considering for MBMS scheduling. \n",
						mbsfn_area_id, mbms_local_area, mcch_rep_abs_rf);
				/** Set the MBSFN area id to invalid. */
				mbsfn_clusters_to_schedule[mbms_local_area].mbsfn_area_cfg[num_mbsfn_area].mbsfnArea.mbsfn_area_id = INVALID_MBSFN_AREA_ID;
				continue;
			}
			/** Check if it has been changed since last MCCH modification period. */
			mbsfn_area_cfg_t *mbsfn_area_cfg_last = NULL;
			hashtable_ts_get(mcch_mbsfn_cfg_htbl, mbsfn_area_id, (void**)&mbsfn_area_cfg_last);
			if(mbsfn_area_cfg_last){
				// todo: a more efficient way?
				if(memcmp((void*)mbsfn_area_cfg_last, (void*)&mbsfn_clusters_to_schedule[mbms_local_area].mbsfn_area_cfg[num_mbsfn_area], sizeof(mbsfn_area_cfg_t)) == 0){
					// todo: new eNBs should be informed with the MCCH scheduling..
					OAILOG_INFO(LOG_MCE_APP, "For MBSFN area " MBSFN_AREA_ID_FMT " in local MBMS area (%d) the MBSFN scheduling HAS NOT changed. Not retriggering an MBMS Scheduling update due MBSFN area. \n", mbsfn_area_id, mbms_local_area);
					/** Leave the size but set it to 0. */
					mbsfn_clusters_to_schedule[mbms_local_area].mbsfn_area_cfg[num_mbsfn_area].mbsfnArea.mbsfn_area_id = INVALID_MBSFN_AREA_ID;
					continue;
				}
			}
			OAILOG_INFO(LOG_MCE_APP, "For MBSFN area " MBSFN_AREA_ID_FMT " in local MBMS area (%d) the MBSFN scheduling HAS changed (or no cached MBSFN config) & MCCH modification boundary reached."
					"Retriggering an MBMS Scheduling update due MBSFN area. \n", mbsfn_area_id, mbms_local_area);
			mbsfn_areas_to_be_scheduled++;
			continue;
		}
	}

	if(mbsfn_areas_to_be_scheduled) {
		OAILOG_INFO(LOG_MCE_APP, "Found (%d) MBSFN areas for MCCH absolute modification period (%d) for scheduling!\n",
  			mbsfn_areas_to_be_scheduled, mcch_rep_abs_rf);
  	/** Transmit all of them to the M2AP layer. */
  	mce_app_itti_m3ap_send_mbms_scheduling_info(mbsfn_clusters_to_schedule, max_mbms_local_areas + 1, mcch_rep_abs_rf);
  	// todo: guarantee, that we area bit before the MBSFN
  } else {
  	OAILOG_INFO(LOG_MCE_APP, "No MBSFN areas to be scheduled for MCCH modification absolute period (%d).\n", mcch_rep_abs_rf);
  }
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
  /**
   * Remove the MBMS service from the MBSFN areas.
   */
  mce_app_mbsfn_remove_mbms_service(tmgi, mbms_sa_id);
  OAILOG_FUNC_OUT(LOG_MCE_APP);
}

/**
 * Update the MBSFN area registrations of the MBMS services.
 * Before using this method, the MBMS service index & MBMS service area Id must need to be updated.
 * The M2AP layer will afterwards separately deal with removing resources from eNB of old MBMS service area id.
 */
//------------------------------------------------------------------------------
static
int mce_app_update_mbsfn_area_registration(const mbms_service_index_t new_mbms_service_index, const mbms_service_index_t old_mbms_service_index,
		const uint32_t sec_since_epoch, const long double usec_since_epoch, const mbms_session_duration_t * mbms_session_duration,
		const mbsfn_area_ids_t * const mbsfn_area_ids)
{
  OAILOG_FUNC_IN(LOG_MCE_APP);

  bool			mbsfn_success = false;
	int 			rc 						= RETURNerror;

	/**
	 * If the old MBMS Service index is not the new MBMS service index,
	 * remove the MBMS service from all MBSFN areas.
	 */
	if(old_mbms_service_index != INVALID_MBMS_SERVICE_INDEX && old_mbms_service_index != new_mbms_service_index){
		OAILOG_WARNING(LOG_MCE_APP, "MBMS Service index changed from " MBMS_SERVICE_INDEX_FMT " to " MBMS_SERVICE_INDEX_FMT ". Removing the old associations in the MBSFN areas. \n",
				old_mbms_service_index, new_mbms_service_index);
		/**
		 * We don't care about the old start times, the MBMS services will be removed immediately.
		 * The new MCCH modification time will be inserted and until then, not considered in the scheduling.
		 */
		mce_app_reset_mbsfn_service_registration(old_mbms_service_index);
		OAILOG_WARNING(LOG_MCE_APP, "Removed MBMS Service index " MBMS_SERVICE_INDEX_FMT " from the MBSFN areas. \n", old_mbms_service_index);
	}

	/**
	 * Check if it is a new MBMS service (no old MBMS Service Area Id).
	 * If so, directly register.
	 */
  for(int num_mbsfn_area = 0; num_mbsfn_area < mbsfn_area_ids->num_mbsfn_area_ids; num_mbsfn_area++)
  {
  	/** Retrieve the MBSFN area. */
  	mbsfn_area_context_t * mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts,
  			mbsfn_area_ids->mbsfn_area_id[num_mbsfn_area]);
  	if(mbsfn_area_context)
  	{
  		/**
  		 * Register the MBMS service in the MBSFN Area with the correct MCCH modification periods.
  		 * MBSFN areas of an old & changed MBMS service area ids should already be cleaned of the MBMS service.
  		 */
  		mbsfn_success |= (mce_app_mbsfn_area_register_mbms_service(mbsfn_area_context, new_mbms_service_index,
  				sec_since_epoch, usec_since_epoch, mbms_session_duration) == RETURNok);
  	}
  }
  OAILOG_FUNC_RETURN(LOG_MCE_APP, mbsfn_success ? RETURNok : RETURNerror);
}

/**
 * Merge the list of active local & global MBMS services.
 * Check if the resources fit, if not perform ARP preemption on the MBSFN area of the given MBMS service index to check (ONLY!), not in the other ones.
 * Do this as long as the resources fit, or the service which has to be preempted is the newly received MBMS service, if so return RETURNerror.
 * Remove the preempted MBMS services from the respective lists of active MBMS services, and add them into the TBR list.
 * No common CSA pattern will be returned, so the MCCH modification timeout cannot use this method (uses mce_app_check_mbsfn_cluster_resources, though).
 */
//------------------------------------------------------------------------------
static
int mce_app_check_mbsfn_cluster_capacity(const mbms_service_index_t mbms_service_index, const mbsfn_area_id_t mbsfn_area_id,
		const mbsfn_area_ids_t	  		*const mbsfn_area_ids_nlg_p,
		const mbsfn_area_ids_t				*const mbsfn_area_ids_local_p,
		mbms_service_indexes_t				*const mbms_service_indexes_active_nlg_p,
		mbms_service_indexes_t				*const mbms_service_indexes_active_local_p,
		mbms_service_indexes_t 				*const mbms_services_tbr)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	int										 		 						rc												= RETURNok;
	mbsfn_areas_t __attribute__((unused)) mbsfn_areas 							= {0};
	mbsfn_area_context_t  							 *mbsfn_area_context				= mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_id);
	DevAssert(mbsfn_area_context);

	/** Initialize the list of local MBMS services. */
	mme_config_read_lock (&mme_config);
	mbms_service_index_t * mbms_service_indexes_total_array[mme_config.mbms.max_mbms_services];
	memset(mbms_service_indexes_total_array, 0, sizeof(mbms_service_index_t) * mme_config.mbms.max_mbms_services);
	mme_config_unlock(&mme_config);

	/** Check the capacity in the merged MBMS services. */
	while (mce_app_check_mbsfn_cluster_resources(mbsfn_area_ids_nlg_p, mbsfn_area_ids_local_p,
			mbms_service_indexes_active_nlg_p, mbms_service_indexes_active_local_p, &mbsfn_areas) == RETURNerror)
	{
		OAILOG_ERROR(LOG_MCE_APP,"Could not fit all active MBMS services. Performing ARP preemption for MBSFN Area Id " MBSFN_AREA_ID_FMT".\n", mbsfn_area_id);
		mbms_service_indexes_t * mbms_service_indexes_to_preemtp = mbsfn_area_context->privates.fields.local_mbms_area ? mbms_service_indexes_active_local_p : mbms_service_indexes_active_nlg_p;
		/**
		 * If an MBSFN area is given, will preempt only from that MBSFN area, if not, may preempt from any MBSFN area of the list of MBMS services.
		 * Active MBMS list size will not change, but elements (MBMS service indexes) may be invalidated.
		 */
		mbms_service_index_t mbms_service_idx_to_preempt = mce_app_mbms_arp_preempt(mbms_service_indexes_to_preemtp, mbsfn_area_id);
    if(mbms_service_idx_to_preempt == INVALID_MBMS_SERVICE_INDEX) {
    	OAILOG_ERROR(LOG_MCE_APP, "Error while ARP preemption. Cannot allow MBMS service index " MBMS_SERVICE_INDEX_FMT " in MBSFN Area "MBSFN_AREA_ID_FMT". \n",
    			mbms_service_index, mbsfn_area_id);
    	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
    }
    if(mbms_service_index == mbms_service_idx_to_preempt){
    	OAILOG_ERROR(LOG_MCE_APP, "No resources for newly received MBMS Service Index "MBMS_SERVICE_INDEX_FMT " in MBSFN Area " MBSFN_AREA_ID_FMT". \n",
    			mbms_service_index, mbsfn_area_id);
    	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
    }
    /** Add the MBMS service into the list of MBMS services TBR. */
    OAILOG_WARNING(LOG_MCE_APP, "Adding MBMS Service Index "MBMS_SERVICE_INDEX_FMT" into list of MBMS services to preempt and re-checking capacity.\n",
    		mbms_service_idx_to_preempt);
    mbms_services_tbr->mbms_service_index_array[mbms_services_tbr->num_mbms_service_indexes++] = mbms_service_idx_to_preempt;
    memset((void*)&mbsfn_areas, 0, sizeof(mbsfn_areas_t));
	}
	/** We could schedule all MBSFN areas, including the latest received MBMS service. */
 	OAILOG_INFO(LOG_MCE_APP, "After preempting (%d) services, finally MBSFN resources are enough to activate new MBMS Service Id " MBMS_SERVICE_INDEX_FMT " in MBSFN Area Id " MBSFN_AREA_ID_FMT". \n",
 			mbms_services_tbr->num_mbms_service_indexes, mbms_service_index, mbsfn_area_id);
	OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

/**
 * Check each eMBMS cluster. Since the resources are shared and the MBMS services can exist only once among resource sharing MBSFN clusters,
 * each cluster must allow the MBMS service.
 * This method does not result in a CSA pattern, it only checks all the active MBMS services.
 * Local MBSFN areas will be scheduled independently from each other. And we schedule the global MBSFN areas always in the beginning.
 * So for each cluster, the global MBSFN areas will have the same resources.
 */
//------------------------------------------------------------------------------
static
int mce_app_check_shared_resources(const mbms_service_index_t mbms_service_index, const mbsfn_area_id_t mbsfn_area_id, mbms_service_indexes_t * const mbms_service_indexes_tbr)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	int										 				 rc																= RETURNok;
	mbsfn_area_context_t					*mbsfn_area_context								= NULL;
	mcch_modification_periods_t		*mcch_modification_periods				= NULL;
	mbsfn_areas_t					 				 mbsfn_areas 											= {0},
									 	 	  				*mbsfn_areas_p 										= &mbsfn_areas;
	uint8_t 							 				 local_global_areas_allowed 			= 0;
	uint8_t								 				 max_mbms_local_areas			    		= 0;
	uint8_t								 				 mbms_service_in_areas						= 0;

	/** MBMS service indexes for non-local global MBSFN areas. */
	mbms_service_indexes_t				mbms_service_indexes_active_nlg 	= {0},
															 *mbms_service_indexes_active_nlg_p = &mbms_service_indexes_active_nlg;

	/**
	 * The MBMS service in question might be returned due ARP, if so reject it to the MBMS-GW.
	 * Remove any other MBMS services implicitly in the M2AP layer.
	 */
	mme_config_read_lock (&mme_config);
	max_mbms_local_areas = mme_config.mbms.mbms_local_service_areas;
	mbsfn_area_ids_t mbsfn_areas_to_be_checked[max_mbms_local_areas +1]; /**< O: non-local global MBMS areas. */
	/** Set the MBSFN areas to 0. */
	memset((void*)mbsfn_areas_to_be_checked, 0, (max_mbms_local_areas +1) * sizeof(mbsfn_area_ids_t));
	local_global_areas_allowed = mme_config.mbms.mbms_global_mbsfn_area_per_local_group;
	/** Initialize the list of global MBMS service indexes, too. */
	mbms_service_index_t mbms_service_indexes_active_array[mme_config.mbms.max_mbms_services];
	memset(mbms_service_indexes_active_array, 0, sizeof(mbms_service_index_t) * mme_config.mbms.max_mbms_services);
	/** Set the MBSFN areas to 0. */
	mbms_service_indexes_active_nlg.mbms_service_index_array = mbms_service_indexes_active_array;
	mme_config_unlock(&mme_config);

	mbsfn_area_context = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_area_id);
	DevAssert(mbsfn_area_context);

	/** Get the MCCH modification period, from the MBSFN area. */
	hashtable_ts_get(mbsfn_area_context->privates.mbms_service_idx_mcch_modification_times_hashmap, (hash_key_t)mbms_service_index, (void**)&mcch_modification_periods);
	DevAssert(mcch_modification_periods);

	/**
	 * Collect the MBSFN Areas which will be considered in the capacity calculation.
	 * We use the local_mbms_area as index.
	 * If the MBMS service area is global ->  check the configuration!
	 */
	hashtable_ts_apply_callback_on_elements((hash_table_ts_t * const)mce_app_desc.mce_mbsfn_area_contexts.mbsfn_area_id_mbsfn_area_htbl,
			mce_app_check_mbsfn_neighbors, (void*)&mbsfn_area_context->privates.fields.local_mbms_area, (void**)&mbsfn_areas_to_be_checked);

	/**
	 * Check if we have non-local global MBSFN areas to check, then make a list of active MBMS services, including the newest one, if its a nl-global.
	 * Do this, no matter the changed local-mbms-area is local or global.
	 */
	if(mbsfn_areas_to_be_checked[0].num_mbsfn_area_ids){
		for( int num_mbsfn_nlg = 0; num_mbsfn_nlg < mbsfn_areas_to_be_checked[0].num_mbsfn_area_ids; num_mbsfn_nlg++){
			mbsfn_area_context_t * mbsfn_area_context_nlg = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts, mbsfn_areas_to_be_checked[0].mbsfn_area_id[num_mbsfn_nlg]);
			DevAssert(mbsfn_area_context_nlg);
			hashtable_apply_callback_on_elements(mbsfn_area_context_nlg->privates.mbms_service_idx_mcch_modification_times_hashmap,
					mce_app_get_active_mbms_services_per_mbsfn_area, (void*)mcch_modification_periods, (void**)&mbms_service_indexes_active_nlg_p);
		}
	}

	/** Verify the resources of the global MBMS first, if the MBSFN area is a nlg.. */
	if(!mbsfn_area_context->privates.fields.local_mbms_area) {
		/**
		 * Method will eliminate active global MBMS services from list.
		 * So we may continue with the local MBMS service area checks below, since the active list will be updated.
		 * The size of the active list will not be changed, it will just contain invalid MBMS service indexes.
		 */
		if(mce_app_check_mbsfn_cluster_capacity(mbms_service_index, mbsfn_area_id,
				&mbsfn_areas_to_be_checked[0], NULL,
				mbms_service_indexes_active_nlg_p, NULL,
				mbms_service_indexes_tbr) == RETURNerror){
			OAILOG_ERROR(LOG_MCE_APP, "Error verifying resources for new global MBMS service "MBMS_SERVICE_INDEX_FMT " in MBSFN area " MBSFN_AREA_ID_FMT " (amongst global only).\n", mbms_service_index, mbsfn_area_id);
			OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
		}
		/** Fits in first cluster. */
		OAILOG_INFO(LOG_MCE_APP, "Successfully checked resources for global MBMS service " MBMS_SERVICE_INDEX_FMT" in MBSFN area " MBSFN_AREA_ID_FMT " (amongst global). (%d) global services to be removed already. Continuing with locals..\n",
				mbms_service_index, mbsfn_area_id, mbms_service_indexes_tbr->num_mbms_service_indexes);
	} else {
		OAILOG_INFO(LOG_MCE_APP, "Newly received MBMS service " MBMS_SERVICE_INDEX_FMT" in MBSFN area " MBSFN_AREA_ID_FMT " is local. Not checking global capacity (all global services remain active)..\n",
				mbms_service_index, mbsfn_area_id);
	}
  /** Check the local MBSFN areas. */
  for(int local_mbms_area_to_check = 1; local_mbms_area_to_check < max_mbms_local_areas + 1; local_mbms_area_to_check++){
  	mbms_service_indexes_t				mbms_service_indexes_active_local		= {0},
  															 *mbms_service_indexes_active_local_p = &mbms_service_indexes_active_local;
  	/** Initialize the list of local MBMS services. */
		mme_config_read_lock (&mme_config);
		mbms_service_index_t mbms_service_indexes_active_array[mme_config.mbms.max_mbms_services];
		memset(mbms_service_indexes_active_array, 0, sizeof(mbms_service_index_t) * mme_config.mbms.max_mbms_services);
		/** Set the MBSFN areas to 0. */
		mbms_service_indexes_active_local.mbms_service_index_array = mbms_service_indexes_active_array;
		mme_config_unlock(&mme_config);

  	if(!mbsfn_areas_to_be_checked[local_mbms_area_to_check].num_mbsfn_area_ids){
  		/** Skip.. Nothing else to do. Pass all active MBMS services so far. */
  		OAILOG_INFO(LOG_MCE_APP, "No local MBSFN areas to check for local MBMS area (%d). Skipping..\n", local_mbms_area_to_check);
  		continue;
  	}
  	/** Get an initial list of the active MBMS services for the local MBSFN areas. */
  	for(int num_local_mbsfn_area = 0; num_local_mbsfn_area < mbsfn_areas_to_be_checked[local_mbms_area_to_check].num_mbsfn_area_ids; num_local_mbsfn_area++){
  		/** Set the list of active MBMS services checking all local MBSFN areas. */
  		mbsfn_area_context_t * mbsfn_area_context_local = mce_mbsfn_area_exists_mbsfn_area_id(&mce_app_desc.mce_mbsfn_area_contexts,
  				mbsfn_areas_to_be_checked[local_mbms_area_to_check].mbsfn_area_id[num_local_mbsfn_area]);
  		DevAssert(mbsfn_area_context_local);
  		hashtable_apply_callback_on_elements(mbsfn_area_context_local->privates.mbms_service_idx_mcch_modification_times_hashmap,
  				mce_app_get_active_mbms_services_per_mbsfn_area, (void*)mcch_modification_periods, (void**)&mbms_service_indexes_active_local_p);
  	}
  	/**
  	 * We have one cluster local and global services.
  	 * Check the resulting MBSFN areas. The MCCH modification periods will be inside the MBSFN areas.
  	 * They need to share the same physical resources.
  	 * Method returns false, only if the given MBMS service index could not be allocated.
  	 * Further MBMS service areas might be removed.
  	 * Currently, if just once the given MBMS service area can be allocated we return ok.
  	 * If an MBMS service, is removed in the second MBSFN area group, the scheduler of the first MBSFN area will also be affected, but will continue.
  	 */
		/** Method will eliminate active local or global MBMS services from list, depending on the type of the MBMS service to check. */
  	rc = mce_app_check_mbsfn_cluster_capacity(mbms_service_index, mbsfn_area_id,
  			(local_global_areas_allowed) ? NULL : &mbsfn_areas_to_be_checked[0],
				&mbsfn_areas_to_be_checked[local_mbms_area_to_check],
  			mbms_service_indexes_active_nlg_p, mbms_service_indexes_active_local_p, mbms_service_indexes_tbr);
  	if(rc == RETURNok){
  		OAILOG_ERROR(LOG_MCE_APP, "Error verifying resources for new MBMS service "MBMS_SERVICE_INDEX_FMT " in MBSFN area " MBSFN_AREA_ID_FMT " after checking local MBMS service of local mbms area (%d).\n",
  				mbms_service_index, mbsfn_area_id, local_mbms_area_to_check);
  		/** Directly return false. */
  		OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNerror);
  	}
  	/** Fits in first cluster. */
  	OAILOG_INFO(LOG_MCE_APP, "Successfully checked resources for local MBMS area (%d) with globals. Continuing.. \n", local_mbms_area_to_check);
  }
	OAILOG_INFO(LOG_MCE_APP, "Successfully checked resources for all local & global MBSFN areas for the shared resource area. (%d) MBMS services removed implicitly."
			"MBMS service " MBMS_SERVICE_INDEX_FMT " for MBSFN area " MBSFN_AREA_ID_FMT " will be allowed.. \n",
			mbms_service_indexes_tbr->num_mbms_service_indexes, mbms_service_index, mbsfn_area_id);
  OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

/**
 * Check resources for the given MBMS service in the list of MBSFN areas, which are INDEPENDENT!
 * MBSFN areas of the same MBMS area may be independent!
 * For each area: We check independently the resources for each MBSFN clusters, where the MBSFN is part of.
 * If the MBMS service in one of the MBSFN areas prevails, we return success back!
 */
//------------------------------------------------------------------------------
static
int mce_app_check_mbms_service_resources(const mbms_service_index_t mbms_service_index, const mbms_service_area_id_t mbms_service_area_id, const mbsfn_area_ids_t * const mbsfn_area_ids)
{
	OAILOG_FUNC_IN(LOG_MCE_APP);
	int										 rc																= RETURNerror;

	/** We need to make check for the capacity in all local_mbms_areas affected. */
	for(int num_mbsfn_area = 0; num_mbsfn_area < mbsfn_area_ids->num_mbsfn_area_ids; num_mbsfn_area++){
		mbms_service_indexes_t mbms_service_indexes_tbr				= {0};
		/** Clear the MBMS Service Indexes. */
		mme_config_read_lock (&mme_config);
		mbms_service_index_t * mbms_service_indexes_tbr_array[mme_config.mbms.max_mbms_services];
		memset(mbms_service_indexes_tbr_array, 0, sizeof(mbms_service_index_t) * mme_config.mbms.max_mbms_services);
		/** Set the MBSFN areas to 0. */
		mbms_service_indexes_tbr.mbms_service_index_array = mbms_service_indexes_tbr_array;
		mme_config_unlock(&mme_config);

		/** Checking capacity for MBSFN Area Id, in all MBSFN clusters, where it is involved. */
		if(mce_app_check_shared_resources(mbms_service_index, mbsfn_area_ids->mbsfn_area_id[num_mbsfn_area], &mbms_service_indexes_tbr) == RETURNok){
			rc = RETURNok; /**< At least one is set, we will send back Sm-Success to MBMS-GW. */
			OAILOG_INFO(LOG_MCE_APP, "We checked all MBSFN clusters which are of relevance to the MBSFN area " MBSFN_AREA_ID_FMT" for MBMS service index " MBMS_SERVICE_INDEX_FMT" for MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " successfully. "
					"MBMS service will be active. (%d) MBMS services will be preempted. \n",
					mbsfn_area_ids->mbsfn_area_id[num_mbsfn_area], mbms_service_index, mbms_service_area_id, mbms_service_indexes_tbr.num_mbms_service_indexes);
			/** Remove the given MBMS services in the TBR array. */
			for(int num_mbms_tbr = 0; num_mbms_tbr < mbms_service_indexes_tbr.num_mbms_service_indexes; num_mbms_tbr) {
				DevAssert(mbms_service_indexes_tbr.mbms_service_index_array[num_mbms_tbr] != mbms_service_index);
		 		mbms_service_t *mbms_service_tbr = mce_mbms_service_exists_mbms_service_index(&mce_app_desc.mce_mbms_service_contexts, mbms_service_indexes_tbr.mbms_service_index_array[num_mbms_tbr]);
		 		if(mbms_service_tbr){
		 			OAILOG_WARNING(LOG_MCE_APP, "MBMS Service for TMGI " TMGI_FMT " with ARP prio (%d) will be implicitly removed for new MBMS Service request with MBMS Service Index "MBMS_SERVICE_INDEX_FMT". \n",
		 					TMGI_ARG(&mbms_service_tbr->privates.fields.tmgi), mbms_service_tbr->privates.fields.mbms_bc.eps_bearer_context.bearer_level_qos.pl, mbms_service_index);
		 			/** Remove the MBMS session. */
		 			mce_app_itti_m3ap_mbms_session_stop_request(&mbms_service_tbr->privates.fields.tmgi, mbms_service_tbr->privates.fields.mbms_service_area_id, 0);
		 			mce_app_stop_mbms_service(&mbms_service_tbr->privates.fields.tmgi,
		 					mbms_service_tbr->privates.fields.mbms_service_area_id, mbms_service_tbr->privates.fields.mme_teid_sm,
							(struct sockaddr*)&mbms_service_tbr->privates.fields.mbms_peer_ip); /**< Remove the Sm tunnel, too, since no Sm message will be send. */
		 		} else {
		 			DevMessage("MBMS service for index " + mbms_service_indexes_tbr.mbms_service_index_array[num_mbms_tbr] + " could not be found to be preempted!");
		 		}
			}
		} else {
			OAILOG_ERROR(LOG_MCE_APP, "Capacity check of all MBSFN clusters which are of relevance to the MBSFN area " MBSFN_AREA_ID_FMT" for MBMS service index " MBMS_SERVICE_INDEX_FMT" in MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " failed. \n",
					mbsfn_area_ids->mbsfn_area_id[num_mbsfn_area], mbms_service_index, mbms_service_area_id);
			/** Continue. */
		}
	}
	OAILOG_FUNC_RETURN(LOG_MCE_APP, rc);
}

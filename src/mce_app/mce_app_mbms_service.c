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

#include "3gpp_requirements_23.246.h"
#include "3gpp_requirements_29.468.h"
#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "common_types.h"
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
void
mce_app_handle_mbms_session_start_request(
     itti_sm_mbms_session_start_request_t * const mbms_session_start_request_pP
    )
{
  MessageDef                             *message_p  			= NULL;
  teid_t 								  mme_sm_teid			= INVALID_TEID;
  mbms_service_t 						 *mbms_service 			= NULL;
  mme_app_sm_proc_t						 *mbms_sm_proc 			= NULL;
  mbms_service_area_id_t 			      mbms_service_area_id 	= INVALID_MBMS_SERVICE_AREA_ID;
  int                                     rc 					= RETURNok;

  OAILOG_FUNC_IN (LOG_MCE_APP);

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
  if ((mbms_service_area_id = mce_app_check_sa_local(&mbms_session_start_request_pP->tmgi.plmn, &mbms_session_start_request_pP->mbms_service_area) == INVALID_MBMS_SERVICE_AREA_ID)) {
    /**
     * The MBMS Service Area and PLMN are served by this MME.
     */
    OAILOG_ERROR(LOG_MCE_APP, "PLMN " PLMN_FMT " or none of the Target SAs are served by current MME. Rejecting MBMS Session Start Request for TMGI "TMGI_FMT".\n",
    	PLMN_ARG(&mbms_session_start_request_pP->tmgi.plmn), TMGI_ARG(&mbms_session_start_request_pP->tmgi));
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
  if(mbms_session_start_request_pP->mbms_session_duration.seconds < mme_config.mbms_min_session_duration_in_sec) {
    OAILOG_WARNING(LOG_MCE_APP, "MBM Session duration (%ds) is shorter than the minimum (%ds) for MBMS Service Request for TMGI " TMGI_FMT ". Setting to minval. \n",
    	mbms_session_start_request_pP->mbms_session_duration.seconds, mme_config.mbms_min_session_duration_in_sec, TMGI_ARG(&mbms_session_start_request_pP->tmgi));
    mbms_session_start_request_pP->mbms_session_duration.seconds = mme_config.mbms_min_session_duration_in_sec;
  }
  mme_config_unlock (&mme_config);

  /** Check that the MBMS Bearer QoS is a valid GBR. */
  if((5 <= mbms_session_start_request_pP->mbms_bearer_level_qos.qci && mbms_session_start_request_pP->mbms_bearer_level_qos.qci <= 9) || (69 <= mbms_session_start_request_pP->mbms_bearer_level_qos.qci && mbms_session_start_request_pP->mbms_bearer_level_qos.qci <= 70) || (79 == mbms_session_start_request_pP->mbms_bearer_level_qos.qci)){
    /** Return error, no modification on non-GBR is allowed. */
	OAILOG_ERROR(LOG_MCE_APP, "Non-GBR MBMS Bearer Level QoS (qci=%d) not supported. Rejecting MBMS Service Request for TMGI " TMGI_FMT ". \n",
			mbms_session_start_request_pP->mbms_bearer_level_qos.pci, TMGI_ARG(&mbms_session_start_request_pP->tmgi));
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
    /** Check if there is a MBMS Start procedure, ignore the request. No need to trigger anything. */
    if((mbms_sm_proc = mme_app_get_sm_procedure(&mbms_session_start_request_pP->tmgi, mbms_service_area_id))){
      if(mbms_sm_proc->proc.type == MME_APP_SM_PROC_TYPE_MBMS_START_SESSION){
        OAILOG_WARNING(LOG_MCE_APP, "MBMS Service context for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " has a running start procedure. "
          "Ignoring new start request. \n", TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);
        OAILOG_FUNC_OUT (LOG_MCE_APP);
      }
    }
    OAILOG_ERROR(LOG_MCE_APP, "An old MBMS Service context already existed for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". Rejecting new one and implicitly removing old one (& removing procedures). \n",
    	TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, SYSTEM_FAILURE);
    /** Removing old MBMS Service Context and informing the MCE APP. No response is expected from MCE. */
    mce_app_stop_mbms_service(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id, mbms_service->privates.fields.mme_teid_sm,
    	(struct sockaddr*)&mbms_service->privates.fields.mbms_peer_ip);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * Check the requested MBMS Bearer.
   * Session Start should always trigger a new MBMS session. It may reuse an MBMS Bearer.
   * C-TEID is always unique of the MBMS Bearer service for the given MBMS SA and flow-ID.
   */
  if((mbms_service = mbms_cteid_in_list(&mce_app_desc.mce_mbms_service_contexts, mbms_session_start_request_pP->mbms_ip_mc_address->cteid)) != NULL){
    OAILOG_ERROR(LOG_MCE_APP, "An old MBMS Service context already existed for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT " already exist with given CTEID "TEID_FMT". "
    	"Rejecting new one for TMGI "TMGI_FMT". Leaving old one. \n", TMGI_ARG(&mbms_service->privates.fields.tmgi), mbms_service->privates.fields.mbms_service_area_id, TMGI_ARG(&mbms_session_start_request_pP->tmgi));
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  OAILOG_INFO(LOG_MCE_APP, "Successfully processed the request for a new MBMS Service for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT". Continuing with the establishment. \n",
	TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);

  mme_sm_teid = __sync_fetch_and_add (&mce_app_mme_sm_teid_generator, 0x00000001);
  // todo: locks!?
  if (!mce_register_mbms_service(&mbms_session_start_request_pP->tmgi, &mbms_session_start_request_pP->mbms_service_area, mme_sm_teid)) {
    OAILOG_ERROR(LOG_MCE_APP, "No MBMS Service context could be generated from TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". \n",
    		TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->mbms_peer_ip, mbms_session_start_request_pP->trxn, SYSTEM_FAILURE);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }
  /** Update the MBMS Service with the given parameters. */
  mce_app_update_mbms_service(&mbms_session_start_request_pP->tmgi, mbms_service_area_id, &mbms_session_start_request_pP->mbms_bearer_level_qos,
	mbms_session_start_request_pP->mbms_flow_id, mbms_session_start_request_pP->mbms_ip_mc_address, &mbms_session_start_request_pP->mbms_peer_ip);

  /**
   * Create an MME APP procedure (not MCE).
   * Since the procedure is new, we don't need to check if another procedure exists.
   */
  DevAssert(mme_app_create_sm_procedure_mbms_session_start(&mbms_session_start_request_pP->tmgi, mbms_service_area_id, &mbms_session_start_request_pP->mbms_session_duration));

  /**
   * The MME may return an MBMS Session Start Response to the MBMS-GW as soon as the session request is accepted by one E-UTRAN node.
   * That's why, we start an Sm procedure for the received Sm message. At timeout (no eNB response, we automatically purge the MBMS Service Context and respond to the MBMS-GW).
   * We send an eNB general MBMS Session Start trigger over M3.
   */
  REQUIREMENT_3GPP_23_246(R8_3_2__6);
  OAILOG_INFO(LOG_MCE_APP, "Created a MBMS procedure for new MBMS Session with TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". Informing the MCE over M3. \n",
    TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_service_area_id);
//  tmgi_t * tmgi, mbms_service_area_id_t * mbms_service_area_id, bearer_context_to_be_created_t * mbms_bc_tbc, mbms_session_duration_t * mbms_session_duration,
//	void* min_time_to_mbms_data_transfer, mbms_ip_multicast_distribution_t * mbms_ip_mc_dist, mbms_abs_time_data_transfer_t * abs_start_time
//  mce_app_itti_m3ap_mbms_session_start_request(&mbms_session_start_request_pP->tmgi, mbms_service_area_id, NULL);

  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
void
mce_app_handle_mbms_session_update_request(
     itti_sm_mbms_session_update_request_t * const mbms_session_update_request_pP
    )
{
  MessageDef                             *message_p  			= NULL;
  mbms_service_t 						 *mbms_service 			= NULL;
  mme_app_sm_proc_t						 *mbms_sm_proc 			= NULL;
  teid_t 								  mme_sm_teid			= INVALID_TEID;
  mbms_service_area_id_t 			      mbms_service_area_id 	= INVALID_MBMS_SERVICE_AREA_ID;
  int                                     rc 					= RETURNok;

  OAILOG_FUNC_IN (LOG_MCE_APP);

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
  if ((mbms_service_area_id = mce_app_check_sa_local(&mbms_session_update_request_pP->tmgi.plmn, &mbms_session_update_request_pP->mbms_service_area) == INVALID_MBMS_SERVICE_AREA_ID)) {
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
  if(mbms_session_update_request_pP->mbms_session_duration.seconds < mme_config.mbms_min_session_duration_in_sec) {
    OAILOG_WARNING(LOG_MCE_APP, "MBM Session duration (%ds) is shorter than the minimum (%ds) for MBMS Service Request for TMGI " TMGI_FMT ". Setting to minval. \n",
    	mbms_session_update_request_pP->mbms_session_duration.seconds, mme_config.mbms_min_session_duration_in_sec, TMGI_ARG(&mbms_session_update_request_pP->tmgi));
    mbms_session_update_request_pP->mbms_session_duration.seconds = mme_config.mbms_min_session_duration_in_sec;
  }
  mme_config_unlock (&mme_config);

  /** Check that the MBMS Bearer QoS is a valid GBR. */
  if((5 <= mbms_session_update_request_pP->mbms_bearer_level_qos->qci && mbms_session_update_request_pP->mbms_bearer_level_qos->qci <= 9) || (69 <= mbms_session_update_request_pP->mbms_bearer_level_qos->qci && mbms_session_update_request_pP->mbms_bearer_level_qos->qci <= 70) || (79 == mbms_session_update_request_pP->mbms_bearer_level_qos->qci)){
    /** Return error, no modification on non-GBR is allowed. */
 	OAILOG_ERROR(LOG_MCE_APP, "Non-GBR MBMS Bearer Level QoS (qci=%d) not supported. Rejecting MBMS Service Request for TMGI " TMGI_FMT ". \n",
 			mbms_session_update_request_pP->mbms_bearer_level_qos->pci, TMGI_ARG(&mbms_session_update_request_pP->tmgi));
 	mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid, mbms_session_update_request_pP->sm_mbms_fteid.teid, &mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, REQUEST_REJECTED);
 	/** No MBMS service or tunnel endpoint is allocated yet. */
 	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /** Verify the received gbr qos, if any is received. */
  if(validateEpsQosParameter(mbms_session_update_request_pP->mbms_bearer_level_qos->qci,
		  mbms_session_update_request_pP->mbms_bearer_level_qos->pvi, mbms_session_update_request_pP->mbms_bearer_level_qos->pci, mbms_session_update_request_pP->mbms_bearer_level_qos->pl,
		  mbms_session_update_request_pP->mbms_bearer_level_qos->gbr.br_dl, mbms_session_update_request_pP->mbms_bearer_level_qos->gbr.br_ul,
		  mbms_session_update_request_pP->mbms_bearer_level_qos->mbr.br_dl, mbms_session_update_request_pP->mbms_bearer_level_qos->mbr.br_ul) == RETURNerror){
    OAILOG_ERROR(LOG_MCE_APP, "MBMS Bearer Level QoS (qci=%d) could not be validated.Rejecting MBMS Service Request for TMGI " TMGI_FMT ". \n",
    		mbms_session_update_request_pP->mbms_bearer_level_qos->pci, TMGI_ARG(&mbms_session_update_request_pP->tmgi));
    mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid, mbms_session_update_request_pP->sm_mbms_fteid.teid, &mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, REQUEST_REJECTED);
    /** No MBMS service or tunnel endpoint is allocated yet. */
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * Check if any MBMS procedure for the MBMS Service exists, if so reject it.
   * Use the current MBMS Service Area.
   */
  mbms_sm_proc = mme_app_get_sm_procedure(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id);
  if(mbms_sm_proc){
    /** We just send a temporary reject. */
	OAILOG_WARNING(LOG_MCE_APP, "MBMS Service with TMGI " TMGI_FMT " has already an MBMS update procedure running. \n", TMGI_ARG(&mbms_session_update_request_pP->tmgi));
	mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid, mbms_session_update_request_pP->sm_mbms_fteid.teid, &mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, COLLISION_WITH_NETWORK_INITIATED_REQUEST);
	/** No MBMS service or tunnel endpoint is allocated yet. */
	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /** Check the MBMS Bearer Context status. */
  if(mbms_service->privates.fields.mbms_bc.eps_bearer_context.bearer_state != BEARER_STATE_ACTIVE){
	/** We just send a temporary reject. */
  	OAILOG_WARNING(LOG_MCE_APP, "MBMS Service with TMGI " TMGI_FMT " bearer with CTEID "TEID_FMT " is not in ACTIVE state, instead (%d). \n",
  			TMGI_ARG(&mbms_session_update_request_pP->tmgi), mbms_service->privates.fields.mbms_bc.mbms_ip_mc_distribution.cteid,
			mbms_service->privates.fields.mbms_bc.eps_bearer_context.bearer_state);
  	mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->teid, mbms_session_update_request_pP->sm_mbms_fteid.teid, &mbms_session_update_request_pP->mbms_peer_ip, mbms_session_update_request_pP->trxn, COLLISION_WITH_NETWORK_INITIATED_REQUEST);
  	/** No MBMS service or tunnel endpoint is allocated yet. */
  	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /**
   * CTEID or MBMS Multicast IPs don't change.
   */
  OAILOG_INFO(LOG_MCE_APP, "Successfully processed the update of MBMS Service for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT". Continuing with the update. \n",
 	TMGI_ARG(&mbms_session_update_request_pP->tmgi), mbms_service_area_id);
  // todo: locks!?

  /**
   * Don't update the MBMS service,
   * Create an MBMS Service Update Procedure and store the values there.
   * Use the current MBMS TMGI and the current MBMS Service Id.
   * If at least one eNB replies positively, update the values of the MBMS service with the procedures values. */
  mbms_sm_proc = mme_app_create_sm_procedure_mbms_session_update(&mbms_service->privates.fields.tmgi,
		  mbms_service->privates.fields.mbms_service_area_id, &mbms_session_update_request_pP->mbms_session_duration);
  /**
   * Set the new MBMS Bearer Context to be updated & new MBMS Service Area into the procedure!
   * Set it into the MBMS Service with the first response.
   */
  ((mme_app_sm_proc_mbms_session_update_t*)mbms_sm_proc)->mbms_service_area_id = mbms_service_area_id;
  ((mme_app_sm_proc_mbms_session_update_t*)mbms_sm_proc)->bc_tbu.bearer_level_qos = mbms_session_update_request_pP->mbms_bearer_level_qos;
  mbms_session_update_request_pP->mbms_bearer_level_qos = NULL;

  /** Toggle the MBMS Bearer context status. */
  mbms_service->privates.fields.mbms_bc.eps_bearer_context.bearer_state &= (~BEARER_STATE_ACTIVE);
  mbms_service->privates.fields.mbms_bc.eps_bearer_context.bearer_state &= (~BEARER_STATE_ENB_CREATED);

  /**
   * The MME may return an MBMS Session Update Response to the MBMS-GW as soon as the session update request is accepted by one E-UTRAN node.
   */
  OAILOG_INFO(LOG_MCE_APP, "Created a MBMS procedure for updated MBMS Session with TMGI " TMGI_FMT ". Informing the MCE over M3. \n", TMGI_ARG(&mbms_session_update_request_pP->tmgi));
//   tmgi_t * tmgi, mbms_service_area_id_t * mbms_service_area_id, bearer_context_to_be_created_t * mbms_bc_tbc, mbms_session_duration_t * mbms_session_duration,
////	void* min_time_to_mbms_data_transfer, mbms_ip_multicast_distribution_t * mbms_ip_mc_dist, mbms_abs_time_data_transfer_t * abs_start_time
////  mce_app_itti_m3ap_mbms_session_update_request(&mbms_session_start_request_pP->tmgi, mbms_service_area_id, NULL);

  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
void
mce_app_handle_mbms_session_stop_request(
     itti_sm_mbms_session_stop_request_t * const mbms_session_stop_request_pP
    )
{
 MessageDef                             *message_p  = NULL;
 struct mbms_service_s                  *mbms_service = NULL;
 int                                     rc = RETURNok;

 OAILOG_FUNC_IN (LOG_MCE_APP);

  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
void
mce_app_handle_mbms_session_duration_timer_expiry (const struct tmgi_s *tmgi, const mbms_service_area_id_t mbms_service_area_id)
{
  mbms_service_t							* mbms_service    = NULL;
  mme_app_sm_proc_t							* mme_app_sm_proc = NULL;

  OAILOG_FUNC_IN (LOG_MCE_APP);
  /**
   * Check that no MBMS Service with the given service ID and service area exists, if so reject
   * the request and implicitly remove the MBMS service & tunnel.
   */
  mme_app_sm_proc = mme_app_get_sm_procedure (tmgi, mbms_service_area_id);
  if(!mme_app_sm_proc) {
    /** The MBMS Service Area and PLMN are served by this MME. */
    OAILOG_ERROR(LOG_MCE_APP, "No MBMS Sm Procedure could be found for expired for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_ID_FMT ". \n", TMGI_ARG(tmgi), mbms_service_area_id);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }
  OAILOG_INFO (LOG_MCE_APP, "Expired- Sm Procedure Timer for MBMS Service with TMGI " TMGI_FMT ". Check the procedure. \n", TMGI_ARG(tmgi));

  mbms_service = mce_mbms_service_exists_tmgi(&mce_app_desc.mce_mbms_service_contexts, tmgi, mbms_service_area_id);

  /**
   * Check if the MBMS Session has been successfully established (at least one eNB).
   * After that, we check for MBMS session duration.
   */
  REQUIREMENT_3GPP_23_246(R8_3_2__6);
  if(mbms_service->privates.fields.mbms_bc.eps_bearer_context.bearer_state != BEARER_STATE_ACTIVE){
	/**
	 * The MBMS Bearer context is not been activated yet.
	 * Trigger the removal of the MBMS Bearer context.
	 * Send a reject back. For the update case, we leave the Sm Tunnel, for the Start Case, the Sm tunnel will automatically be removed.
	 * ToDo: should the MBMS Bearer be implicitly removed if update fails?
	 */
	if(mme_app_sm_proc->type == MME_APP_SM_PROC_TYPE_MBMS_START_SESSION) {
	  mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_service->privates.fields.mbms_teid_sm, (struct sockaddr*)&mbms_service->privates.fields.mbms_peer_ip, mme_app_sm_proc->sm_trxn, SYSTEM_FAILURE);
	  OAILOG_ERROR (LOG_MCE_APP, "MBMS Session for TMGI " TMGI_FMT " could not be established successfully in time. Triggering a MBMS session reject and removing MBMS Service. \n", TMGI_ARG(tmgi));
	  mce_app_stop_mbms_service(tmgi, mbms_service_area_id, mbms_service->privates.fields.mme_teid_sm, NULL);
	  OAILOG_FUNC_OUT(LOG_MCE_APP);
	} else {
	  mce_app_itti_sm_mbms_session_update_response(INVALID_TEID, mbms_service->privates.fields.mbms_teid_sm, (struct sockaddr*)&mbms_service->privates.fields.mbms_peer_ip, mme_app_sm_proc->sm_trxn, SYSTEM_FAILURE);
	  /** Remove the SM procedure, don't change the MBMS Service, don'T care about the lower layer. */
	  mme_app_delete_sm_procedure_mbms_session_update(tmgi, mbms_service_area_id); /**< Old MBMS SA. */
	  mbms_service->privates.fields.mbms_bc.eps_bearer_context.bearer_state |= BEARER_STATE_ACTIVE;
	  mbms_service->privates.fields.mbms_bc.eps_bearer_context.bearer_state |= BEARER_STATE_ENB_CREATED;
	  // todo: what happens when update fails?!
	  OAILOG_ERROR (LOG_MCE_APP, "MBMS Session for TMGI " TMGI_FMT " could not be updated successfully in time. Triggering a MBMS session reject. Keeping the MBMS Service. \n", TMGI_ARG(tmgi));
	  OAILOG_FUNC_OUT(LOG_MCE_APP);
	}
  }
  /**
   * If the MBMS Service was successfully established, at at least 1 eNB.
   * Check the procedure and remove it if it was a "short idle period" (reducing signaling overhead).
   */
  REQUIREMENT_3GPP_23_246(R4_4_2__6);
  if(mme_app_sm_proc->trigger_mbms_session_stop) {
    OAILOG_INFO (LOG_MCE_APP, "Triggering MBMS Session stop for expired MBMS Session duration for MBMS service with TMGI " TMGI_FMT ". \n", TMGI_ARG(tmgi));
    /*
     * todo: Need to get a better architecture here, kind of don't know what I am doing considering the locks (where to use the object, where to use identifiers).
     * Reason for that mainly is, what the final lock/structure architecture will look like..
     *
     * Remove the Sm tunnel endpoint implicitly and all MBMS Service References in the M2 layer.
     * No need to inform the MBMS Gateway. Will inform the M2 layer and all the eNBs.
     */
    mce_app_stop_mbms_service(tmgi, mbms_service_area_id, mbms_service->privates.fields.mme_teid_sm, (struct sockaddr*)&mbms_service->privates.fields.mbms_peer_ip);
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
   * Inform the MXAP Layer about the removed MBMS Service.
   * No negative response will arrive, so continue.
   */
  mce_app_itti_m3ap_mbms_session_stop_request(tmgi, mbms_sa_id);
  if(mbms_peer_ip){
    /** Remove the Sm Tunnel Endpoint implicitly. */
	mce_app_remove_sm_tunnel_endpoint(mme_sm_teid, mbms_peer_ip);
  }
  mce_app_remove_mbms_service(tmgi, mbms_sa_id, mme_sm_teid);
  OAILOG_FUNC_OUT(LOG_MCE_APP);
}

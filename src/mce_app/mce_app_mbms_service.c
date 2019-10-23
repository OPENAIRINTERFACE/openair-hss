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

static teid_t                           mce_app_mme_sm_teid_generator = 0x00000001;

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

//------------------------------------------------------------------------------
void
mce_app_handle_mbms_session_start_request(
     itti_sm_mbms_session_start_request_t * const mbms_session_start_request_pP
    )
{
  MessageDef                             *message_p  	= NULL;
  teid_t 								  mme_sm_teid	= INVALID_TEID;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MCE_APP);

  /** Check the destination TEID is 0 in the MME-APP to respond and to clear the transaction. */
  if(mbms_session_start_request_pP->teid != (teid_t)0){
    OAILOG_WARNING (LOG_SM, "Destination TEID of MBMS Session Start Request is not 0, instead " TEID_FMT ". Rejecting MBMS Session Start Request for TMGI " TMGI_FMT". \n",
    		mbms_session_start_request_pP->teid, TMGI_ARG(&mbms_session_start_request_pP->tmgi));
    /** Send a negative response before crashing. */
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid,
	 	   &mbms_session_start_request_pP->peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /** Check if the service area and PLMN is served. No TA/CELL List will be checked. */
  if (!mce_app_check_sa_local(&mbms_session_start_request_pP->tmgi.plmn, &mbms_session_start_request_pP->mbms_service_area)) {
    /**
     * The MBMS Service Area and PLMN are served by this MME.
     */
    OAILOG_ERROR(LOG_MCE_APP, "PLMN " PLMN_FMT " and Target SA " MBMS_SERVICE_AREA_FMT " are not served by current MME. Rejecting MBMS Session Start Request for TMGI "TMGI_FMT".\n",
    		&mbms_session_start_request_pP->tmgi.plmn, &mbms_session_start_request_pP->mbms_service_area, TMGI_ARG(&mbms_session_start_request_pP->tmgi));
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid,
    	 	   &mbms_session_start_request_pP->peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
    /** No MBMS service or tunnel endpoint is allocated yet. */
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /** Check that service duration is > 0. */
  if (!(mbms_session_start_request_pP->mbms_session_duration.seconds || mbms_session_start_request_pP->mbms_session_duration.days)) {
    /**
     * The MBMS Service Area and PLMN are served by this MME.
     */
	OAILOG_ERROR(LOG_MCE_APP, "No session duration given for requested MBMS Service Request for TMGI " TMGI_FMT ". \n", TMGI_ARG(&mbms_session_start_request_pP->tmgi));
	mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid,
			&mbms_session_start_request_pP->peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
	/** No MBMS service or tunnel endpoint is allocated yet. */
	OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  /** Check that the MBMS Bearer QoS is a valid GBR. */
  if((5 <= mbms_session_start_request_pP->mbms_bearer_level_qos.qci && mbms_session_start_request_pP->mbms_bearer_level_qos.qci <= 9) || (69 <= mbms_session_start_request_pP->mbms_bearer_level_qos.qci && mbms_session_start_request_pP->mbms_bearer_level_qos.qci <= 70) || (79 == mbms_session_start_request_pP->mbms_bearer_level_qos.qci)){
    /** Return error, no modification on non-GBR is allowed. */
	OAILOG_ERROR(LOG_MCE_APP, "Non-GBR MBMS Bearer Level QoS (qci=%d) not supported. Rejecting MBMS Service Request for TMGI " TMGI_FMT ". \n",
			mbms_session_start_request_pP->mbms_bearer_level_qos.pci, TMGI_ARG(&mbms_session_start_request_pP->tmgi));
	mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid,
			&mbms_session_start_request_pP->peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
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
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid,
    		&mbms_session_start_request_pP->peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
    /** No MBMS service or tunnel endpoint is allocated yet. */
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }

  // todo: AT LEAST WARNING AND RESPONSE INFO THAT ONLY 1 SERVICE AREA IS ALLOWED
  /**
   * Check that no MBMS Service with the given service ID and service area exists, if so reject
   * the request and implicitly remove the MBMS service & tunnel.
   */
  mbms_service_t * mbms_service = mce_mbms_service_exists_mbms_service_id(&mce_app_desc.mce_mbms_service_contexts, &mbms_session_start_request_pP->tmgi, &mbms_session_start_request_pP->mbms_service_area);
  if(mbms_service) {
    /**
     * The MBMS Service Area and PLMN are served by this MME.
     */
    OAILOG_ERROR(LOG_MCE_APP, "An old MBMS Service context already existed for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_FMT ". Rejecting new one and implicitly removing old one. \n",
    		TMGI_ARG(&mbms_session_start_request_pP->tmgi), &mbms_session_start_request_pP->mbms_service_area);
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->peer_ip, mbms_session_start_request_pP->trxn, SYSTEM_FAILURE);
    /** Removing old MBMS Service Context. */
    mce_remove_mbms_service(&mce_app_desc.mbms_services, mbms_service);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }
  /**
   * Check the requested MBMS Bearer.
   * Session Start should always trigger a new MBMS session. It may reuse an MBMS Bearer.
   * C-TEID is always unique of the MBMS Bearer service for the given MBMS SA and flow-ID.
   */
  if((mbms_service = mbms_cteid_in_list(&mce_app_desc.mce_mbms_service_contexts, mbms_session_start_request_pP->mbms_ip_mc_address->cteid)) != NULL){
    OAILOG_ERROR(LOG_MCE_APP, "An old MBMS Service context already existed for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_FMT " already exist with given CTEID "TEID_FMT". "
    	"Rejecting new one for TMGI "TMGI_FMT". Leaving old one. \n",
		TMGI_ARG(&mbms_service->privates.fields.tmgi), mbms_session_start_request_pP->mbms_service_area.serviceArea[0], TMGI_ARG(&mbms_session_start_request_pP->tmgi));
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->peer_ip, mbms_session_start_request_pP->trxn, REQUEST_REJECTED);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }
  OAILOG_INFO(LOG_MCE_APP, "Successfully processed the request for a new MBMS Service for TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_FMT ". Continuing with the establishment. \n",
	TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_session_start_request_pP->mbms_service_area);

  mme_sm_teid = __sync_fetch_and_add (&mce_app_mme_sm_teid_generator, 0x00000001);
  /** We should only send the handover request and not deal with anything else. */
  // todo: locks!?
  if ((mbms_service = register_mbms_service(&mbms_session_start_request_pP->tmgi, &mbms_session_start_request_pP->mbms_service_area, mme_sm_teid)) == NULL) {
    OAILOG_ERROR(LOG_MCE_APP, "No MBMS Service context could be generated from TMGI " TMGI_FMT " and MBMS Service Area " MBMS_SERVICE_AREA_FMT ". \n",
    		TMGI_ARG(&mbms_session_start_request_pP->tmgi), mbms_session_start_request_pP->mbms_service_area.serviceArea[0]);
    mce_app_itti_sm_mbms_session_start_response(INVALID_TEID, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->peer_ip, mbms_session_start_request_pP->trxn, SYSTEM_FAILURE);
    OAILOG_FUNC_OUT (LOG_MCE_APP);
  }
  /** Update the MBMS Service with the given parameters. */
  mce_app_update_mbms_service(mbms_service, &mbms_session_start_request_pP->abs_start_time, &mbms_session_start_request_pP->mbms_bearer_level_qos,
	mbms_session_start_request_pP->mbms_flags, mbms_session_start_request_pP->mbms_flow_id, mbms_session_start_request_pP->mbms_ip_mc_address,
	&mbms_session_start_request_pP->mbms_session_duration, &mbms_session_start_request_pP->sm_mbms_fteid);
// todo: inform the M2AP Layer (all eNBS) --> NOT WAITING FOR RESPONSE!?! --> NO PROCEDURE?!
  mce_app_itti_sm_mbms_session_start_response(mme_sm_teid, mbms_session_start_request_pP->sm_mbms_fteid.teid, &mbms_session_start_request_pP->peer_ip, mbms_session_start_request_pP->trxn, REQUEST_ACCEPTED);
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
void
mce_app_handle_mbms_session_update_request(
     itti_sm_mbms_session_update_request_t * const mbms_session_update_request_pP
    )
{
 MessageDef                             *message_p  = NULL;
 struct mbms_service_s                  *mbms_service = NULL;
 int                                     rc = RETURNok;

 OAILOG_FUNC_IN (LOG_MCE_APP);

//// /** Everything stack to this point. */
//// /** Check that the TAI & PLMN are actually served. */
//// tai_t target_tai;
//// memset(&target_tai, 0, sizeof (tai_t));
//// target_tai.plmn.mcc_digit1 = forward_relocation_request_pP->target_identification.mcc[0];
//// target_tai.plmn.mcc_digit2 = forward_relocation_request_pP->target_identification.mcc[1];
//// target_tai.plmn.mcc_digit3 = forward_relocation_request_pP->target_identification.mcc[2];
//// target_tai.plmn.mnc_digit1 = forward_relocation_request_pP->target_identification.mnc[0];
//// target_tai.plmn.mnc_digit2 = forward_relocation_request_pP->target_identification.mnc[1];
//// target_tai.plmn.mnc_digit3 = forward_relocation_request_pP->target_identification.mnc[2];
//// target_tai.tac = forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac;
//
//// /** Get the eNB Id. */
//// target_type_t target_type = forward_relocation_request_pP->target_identification.target_type;
//// uint32_t enb_id = 0;
//// switch(forward_relocation_request_pP->target_identification.target_type){
////   case TARGET_ID_MACRO_ENB_ID:
////     enb_id = forward_relocation_request_pP->target_identification.target_id.macro_enb_id.enb_id;
////     break;
////
////   case TARGET_ID_HOME_ENB_ID:
////     enb_id = forward_relocation_request_pP->target_identification.target_id.macro_enb_id.enb_id;
////     break;
////
////   default:
////     OAILOG_DEBUG (LOG_MCE_APP, "Target ENB type %d cannot be handovered to. Rejecting S10 handover request.. \n",
////         forward_relocation_request_pP->target_identification.target_type);
////     mce_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid,
////         &forward_relocation_request_pP->peer_ip, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
////     OAILOG_FUNC_OUT (LOG_MCE_APP);
//// }
//// /** Check the target-ENB is reachable. */
//// if (mce_app_check_ta_local(&target_tai.plmn, forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac)) {
////   /** The target PLMN and TAC are served by this MME. */
////   OAILOG_DEBUG (LOG_MCE_APP, "Target TAC " TAC_FMT " is served by current MME. \n", forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac);
////   /*
////    * Currently only a single TA will be served by each MME and we are expecting TAU from the UE side.
////    * Check that the eNB is also served, that an SCTP association exists for the eNB.
////    */
////   if(s1ap_is_enb_id_in_list(enb_id) != NULL){
////     OAILOG_DEBUG (LOG_MCE_APP, "Target ENB_ID %u is served by current MME. \n", enb_id);
////     /** Continue with the handover establishment. */
////   }else{
////     /** The target PLMN and TAC are not served by this MME. */
////     OAILOG_ERROR(LOG_MCE_APP, "Target ENB_ID %u is NOT served by the current MME. \n", enb_id);
////     mce_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, &forward_relocation_request_pP->peer_ip, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
////     OAILOG_FUNC_OUT (LOG_MCE_APP);
////   }
//// }else{
////   /** The target PLMN and TAC are not served by this MME. */
////   OAILOG_ERROR(LOG_MCE_APP, "TARGET TAC " TAC_FMT " is NOT served by current MME. \n", forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac);
////   mce_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid,
////		   &forward_relocation_request_pP->peer_ip, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
////   /** No UE context or tunnel endpoint is allocated yet. */
////   OAILOG_FUNC_OUT (LOG_MCE_APP);
//// }
// /** We should only send the handover request and not deal with anything else. */
// if ((mbms_service = mce_mbms_service_exists_mbms_service_id_teid(mbms_session_stop_request_pP->mbms_)) == NULL) {
//   /** Send a negative response before crashing. */
//   mce_app_send_sm_mbms_session_stop_response(mbms_session_update_request_pP->mbms_teid.teid, &mbms_session_update_request_pP->peer_ip, mbms_session_update_request_pP->trxn, SYSTEM_FAILURE);
//   OAILOG_FUNC_OUT (LOG_MCE_APP);
// }
// /** Register the new MME_UE context into the map. Don't register the IMSI yet. Leave it for the NAS layer. */
// mbmsServiceId64_t imsi = mbms_session_update_request_pP->mbms_service_id;
// teid_t teid = __sync_fetch_and_add (&mce_app_mms_sm_teid_generator, 0x00000001);
// /** Get the old MBMS service and invalidate its MBMS Service ID relation. */
// mbms_service_t * old_mbms_service = mce_mbms_service_exists_mbms_service_id(&mce_app_desc.mce_mbms_services, mbms_session_update_request_pP->mbms_service_id);
//// if(old_mbms_service){
////   // todo: any locks here?
////   OAILOG_WARNING(LOG_MCE_APP, "An old MBMS Service with mbmsServiceId " MCE_MBMS_SERVICE_ID_FMT " already exists. \n", old_mbms_service->privates.mme_ue_s1ap_id, imsi);
////   mce_mbms_service_update_coll_keys (&mce_app_desc.mce_mbms_services, old_mbms_service,
////		// todo: old_mbms_service->privates.enb_s1ap_id_key, // todo: could this be an identifier?!?
////		INVALID_MBMS_SERVICE_ID,
////		old_mbms_service->privates.fields.local_mme_teid_sm);
//// }
//
// mce_mbms_service_update_coll_keys (&mce_app_desc.mce_mbms_services, mbms_service,
//      // todo: mbms_service->privates.enb_s1ap_id_key,
//      mbms_service->privates.mce_mbms_service_id,
//      // todo: imsi,            /**< Invalidate the IMSI relationship to the old UE context, nothing else.. */
//	  mbms_service->privates.fields.local_mme_teid_sm);
//
// mce_app_itti_sm_mbms_session_update_response(mbms_session_update_request_pP->mbms_teid.teid,
//		 &mbms_session_update_request_pP->peer_ip, mbms_session_update_request_pP->trxn, cause_value);
//
// // todo: inform the M2AP Layer (all eNBS) --> NOT WAITING FOR RESPONSE!?! --> NO PROCEDURE?!
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

//// /** Everything stack to this point. */
//// /** Check that the TAI & PLMN are actually served. */
//// tai_t target_tai;
//// memset(&target_tai, 0, sizeof (tai_t));
//// target_tai.plmn.mcc_digit1 = forward_relocation_request_pP->target_identification.mcc[0];
//// target_tai.plmn.mcc_digit2 = forward_relocation_request_pP->target_identification.mcc[1];
//// target_tai.plmn.mcc_digit3 = forward_relocation_request_pP->target_identification.mcc[2];
//// target_tai.plmn.mnc_digit1 = forward_relocation_request_pP->target_identification.mnc[0];
//// target_tai.plmn.mnc_digit2 = forward_relocation_request_pP->target_identification.mnc[1];
//// target_tai.plmn.mnc_digit3 = forward_relocation_request_pP->target_identification.mnc[2];
//// target_tai.tac = forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac;
//
//// /** Get the eNB Id. */
//// target_type_t target_type = forward_relocation_request_pP->target_identification.target_type;
//// uint32_t enb_id = 0;
//// switch(forward_relocation_request_pP->target_identification.target_type){
////   case TARGET_ID_MACRO_ENB_ID:
////     enb_id = forward_relocation_request_pP->target_identification.target_id.macro_enb_id.enb_id;
////     break;
////
////   case TARGET_ID_HOME_ENB_ID:
////     enb_id = forward_relocation_request_pP->target_identification.target_id.macro_enb_id.enb_id;
////     break;
////
////   default:
////     OAILOG_DEBUG (LOG_MCE_APP, "Target ENB type %d cannot be handovered to. Rejecting S10 handover request.. \n",
////         forward_relocation_request_pP->target_identification.target_type);
////     mce_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid,
////         &forward_relocation_request_pP->peer_ip, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
////     OAILOG_FUNC_OUT (LOG_MCE_APP);
//// }
//// /** Check the target-ENB is reachable. */
//// if (mce_app_check_ta_local(&target_tai.plmn, forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac)) {
////   /** The target PLMN and TAC are served by this MME. */
////   OAILOG_DEBUG (LOG_MCE_APP, "Target TAC " TAC_FMT " is served by current MME. \n", forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac);
////   /*
////    * Currently only a single TA will be served by each MME and we are expecting TAU from the UE side.
////    * Check that the eNB is also served, that an SCTP association exists for the eNB.
////    */
////   if(s1ap_is_enb_id_in_list(enb_id) != NULL){
////     OAILOG_DEBUG (LOG_MCE_APP, "Target ENB_ID %u is served by current MME. \n", enb_id);
////     /** Continue with the handover establishment. */
////   }else{
////     /** The target PLMN and TAC are not served by this MME. */
////     OAILOG_ERROR(LOG_MCE_APP, "Target ENB_ID %u is NOT served by the current MME. \n", enb_id);
////     mce_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, &forward_relocation_request_pP->peer_ip, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
////     OAILOG_FUNC_OUT (LOG_MCE_APP);
////   }
//// }else{
////   /** The target PLMN and TAC are not served by this MME. */
////   OAILOG_ERROR(LOG_MCE_APP, "TARGET TAC " TAC_FMT " is NOT served by current MME. \n", forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac);
////   mce_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid,
////		   &forward_relocation_request_pP->peer_ip, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
////   /** No UE context or tunnel endpoint is allocated yet. */
////   OAILOG_FUNC_OUT (LOG_MCE_APP);
//// }
// /** We should only send the handover request and not deal with anything else. */
// if ((mbms_service = mce_mbms_service_exists_mbms_service_id_teid(mbms_session_stop_request_pP->mbms_)) == NULL) {
//   /** Send a negative response before crashing. */
//   mce_app_send_sm_mbms_session_stop_response(mbms_session_stop_request_pP->mbms_teid.teid, &mbms_session_stop_request_pP->peer_ip, mbms_session_stop_request_pP->trxn, SYSTEM_FAILURE);
//   OAILOG_FUNC_OUT (LOG_MCE_APP);
// }
// /** Register the new MME_UE context into the map. Don't register the IMSI yet. Leave it for the NAS layer. */
// mbmsServiceId64_t imsi = mbms_session_stop_request_pP->mbms_service_id;
// teid_t teid = __sync_fetch_and_add (&mce_app_mms_sm_teid_generator, 0x00000001);
// /** Get the old MBMS service and invalidate its MBMS Service ID relation. */
// mbms_service_t * old_mbms_service = mce_mbms_service_exists_mbms_service_id(&mce_app_desc.mce_mbms_services, mbms_session_stop_request_pP->mbms_service_id);
//// if(old_mbms_service){
////   // todo: any locks here?
////   OAILOG_WARNING(LOG_MCE_APP, "An old MBMS Service with mbmsServiceId " MCE_MBMS_SERVICE_ID_FMT " already exists. \n", old_mbms_service->privates.mme_ue_s1ap_id, imsi);
////   mce_mbms_service_update_coll_keys (&mce_app_desc.mce_mbms_services, old_mbms_service,
////		// todo: old_mbms_service->privates.enb_s1ap_id_key, // todo: could this be an identifier?!?
////		INVALID_MBMS_SERVICE_ID,
////		old_mbms_service->privates.fields.local_mme_teid_sm);
//// }
//
// // todo: remove the MBMS Service !
//// mce_mbms_service_update_coll_keys (&mce_app_desc.mce_mbms_services, mbms_service,
////    // todo: mbms_service->privates.enb_s1ap_id_key,
////    mbms_service->privates.mce_mbms_service_id,
////    // todo: imsi,            /**< Invalidate the IMSI relationship to the old UE context, nothing else.. */
//// mbms_service->privates.fields.local_mme_teid_sm);
//
// mce_app_itti_sm_mbms_session_stop_response(mbms_session_stop_request_pP->mbms_teid.teid,
//		 &mbms_session_stop_request_pP->peer_ip, mbms_session_stop_request_pP->trxn, cause_value);
//
// // todo: inform the M2AP Layer (all eNBS) --> NOT WAITING FOR RESPONSE!?! --> NO PROCEDURE?!
 OAILOG_FUNC_OUT (LOG_MCE_APP);
}

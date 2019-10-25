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

/*! \file mce_app_itti_messaging.h
  \brief
  \author Dincer Beken
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#include "mce_app_mbms_service_context.h"

#ifndef FILE_MCE_APP_ITTI_MESSAGING_SEEN
#define FILE_MCE_APP_ITTI_MESSAGING_SEEN

int mce_app_remove_sm_tunnel_endpoint(teid_t local_teid, struct sockaddr *peer_ip);

/** MBMS Session Start Request. */
void mce_app_itti_sm_mbms_session_start_response(teid_t mme_sm_teid, teid_t mbms_sm_teid, struct sockaddr *mbms_ip_address, void *trxn,  gtpv2c_cause_value_t gtpv2cCause);

/** MBMS Session Update Request. */
void mce_app_itti_sm_mbms_session_update_response(teid_t mme_sm_teid, teid_t mbms_sm_teid, struct sockaddr *mbms_ip_address, void *trxn,  gtpv2c_cause_value_t gtpv2cCause);

/** MBMS Session Stop Request. */
void mce_app_itti_sm_mbms_session_stop_response(teid_t mme_sm_teid, teid_t mbms_sm_teid, struct sockaddr *mbms_ip_address, void *trxn,  gtpv2c_cause_value_t gtpv2cCause);

/** M3AP Session Start Request. */
void mce_app_itti_m3ap_mbms_session_start_request(tmgi_t * tmgi, mbms_service_area_id_t * mbms_service_area_id, bearer_context_to_be_created_t * mbms_bc_tbc, mbms_session_duration_t * mbms_session_duration,
	void* min_time_to_mbms_data_transfer, mbms_ip_multicast_distribution_t * mbms_ip_mc_dist, mbms_abs_time_data_transfer_t * abs_start_time);

/** M3AP Session Update Request. */
void mce_app_itti_m3ap_mbms_session_update_request(tmgi_t * tmgi, mbms_service_area_id_t * mbms_service_area_id, bearer_context_to_be_updated_t * mbms_bc_tbu, mbms_session_duration_t * mbms_session_duration,
	void* min_time_to_mbms_data_transfer, mbms_ip_multicast_distribution_t * mbms_ip_mc_dist, mbms_abs_time_data_transfer_t * abs_start_time);

/** M3AP Session Stop Request. */
void mce_app_itti_m3ap_mbms_session_stop_request(tmgi_t * tmgi, mbms_service_area_id_t mbms_sa_id);

#endif /* FILE_MCE_APP_ITTI_MESSAGING_SEEN */

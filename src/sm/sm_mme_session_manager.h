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

/*! \file sm_mme_session_manager.h
* \brief
* \author Dincer Beken
* \company Blackned GmbH
* \email: dbeken@blackned.de
*
*/

#ifndef FILE_SM_MME_SESSION_MANAGER_SEEN
#define FILE_SM_MME_SESSION_MANAGER_SEEN

/**
 * MBMS SESSION START REQUEST
 */

/* @brief Handle an MBMS Session Start Request received from MBMS-GW. */
int sm_mme_handle_mbms_session_start_request(nw_gtpv2c_stack_handle_t * stack_p, nw_gtpv2c_ulp_api_t * pUlpApi);

/* @brief Create a new MBMS Session Start Response and send it to provided MBMS-GW. */
int sm_mme_mbms_session_start_response(nw_gtpv2c_stack_handle_t *stack_p,     itti_sm_mbms_session_start_response_t *mbms_session_start_rsp_p);

/**
 * MBMS SESSION UPDATE REQUEST
 */

/* @brief Handle an MBMS Session Update Request received from MBMS-GW. */
int sm_mme_handle_mbms_session_update_request(nw_gtpv2c_stack_handle_t * stack_p, nw_gtpv2c_ulp_api_t * pUlpApi);

/* @brief Create a new MBMS Session Update Response and send it to provided MBMS-GW. */
int sm_mme_mbms_session_update_response(nw_gtpv2c_stack_handle_t *stack_p,     itti_sm_mbms_session_update_response_t *mbms_session_update_rsp_p);

/**
 * MBMS SESSION STOP REQUEST
 */

/* @brief Handle an MBMS Session Stop Request received from MBMS-GW. */
int sm_mme_handle_mbms_session_stop_request(nw_gtpv2c_stack_handle_t * stack_p, nw_gtpv2c_ulp_api_t * pUlpApi);

/* @brief Create a new MBMS Session Stop Response and send it to provided MBMS-GW. */
int sm_mme_mbms_session_stop_response(nw_gtpv2c_stack_handle_t *stack_p,     itti_sm_mbms_session_stop_response_t *mbms_session_stop_rsp_p);

/** Handle (timeout) error indication. */
int
sm_mme_handle_ulp_error_indicatior(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi);

/** Remove UE Tunnel in unexpected situations. */
int sm_mme_remove_ue_tunnel ( nw_gtpv2c_stack_handle_t *stack_p, itti_sm_remove_ue_tunnel_t * remove_ue_tunnel_p);

#endif /* FILE_SM_MME_SESSION_MANAGER_SEEN */

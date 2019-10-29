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

//WARNING: Do not include this header directly. Use intertask_interface.h instead.
MESSAGE_DEF(M3AP_MBMS_SESSION_START_REQUEST,   MESSAGE_PRIORITY_MED, itti_m3ap_mbms_session_start_req_t,  m3ap_mbms_session_start_req)
MESSAGE_DEF(M3AP_MBMS_SESSION_UPDATE_REQUEST,  MESSAGE_PRIORITY_MED, itti_m3ap_mbms_session_update_req_t, m3ap_mbms_session_update_req)
MESSAGE_DEF(M3AP_MBMS_SESSION_STOP_REQUEST,    MESSAGE_PRIORITY_MED, itti_m3ap_mbms_session_stop_req_t,   m3ap_mbms_session_stop_req)

MESSAGE_DEF(M3AP_ERROR_INDICATION          , MESSAGE_PRIORITY_MED, itti_m3ap_error_ind_t           								  ,  m3ap_error_ind)
MESSAGE_DEF(MCE_APP_M3_MBMS_SERVICE_COUNTING_REQ          , MESSAGE_PRIORITY_MED, itti_m3ap_mbms_service_counting_req_t           ,  m3ap_mbms_service_counting_req)

MESSAGE_DEF(M3AP_ENB_INITIATED_RESET_REQ   ,  MESSAGE_PRIORITY_MED, itti_m3ap_enb_initiated_reset_req_t   ,  m3ap_initiated_reset_req)
MESSAGE_DEF(M3AP_ENB_INITIATED_RESET_ACK   ,  MESSAGE_PRIORITY_MED, itti_m3ap_enb_initiated_reset_ack_t   ,  m3ap_enb_initiated_reset_ack)

/** No MCE Configuration Update in the MXAP wihout MCE_APP.*/

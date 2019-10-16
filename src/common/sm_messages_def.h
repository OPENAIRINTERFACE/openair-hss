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
MESSAGE_DEF(SM_MBMS_SESSION_START_REQUEST,  MESSAGE_PRIORITY_MED, itti_sm_mbms_session_start_request_t,  sm_mbms_session_start_request)
MESSAGE_DEF(SM_MBMS_SESSION_START_RESPONSE,  MESSAGE_PRIORITY_MED, itti_sm_mbms_session_start_response_t,  sm_mbms_session_start_response)

MESSAGE_DEF(SM_MBMS_SESSION_UPDATE_REQUEST,  MESSAGE_PRIORITY_MED, itti_sm_mbms_session_update_request_t,  sm_mbms_session_update_request)
MESSAGE_DEF(SM_MBMS_SESSION_UPDATE_RESPONSE,  MESSAGE_PRIORITY_MED, itti_sm_mbms_session_update_response_t,  sm_mbms_session_update_response)

MESSAGE_DEF(SM_MBMS_SESSION_STOP_REQUEST,  MESSAGE_PRIORITY_MED, itti_sm_mbms_session_stop_request_t,  sm_mbms_session_stop_request)
MESSAGE_DEF(SM_MBMS_SESSION_STOP_RESPONSE,  MESSAGE_PRIORITY_MED, itti_sm_mbms_session_stop_response_t,  sm_mbms_session_stop_response)

/** Internal Messages. */
MESSAGE_DEF(SM_REMOVE_UE_TUNNEL,   MESSAGE_PRIORITY_MED, itti_sm_remove_ue_tunnel_t,  sm_remove_ue_tunnel)


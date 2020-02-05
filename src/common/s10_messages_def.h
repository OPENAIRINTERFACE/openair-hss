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

// WARNING: Do not include this header directly. Use intertask_interface.h
// instead.
MESSAGE_DEF(S10_FORWARD_RELOCATION_REQUEST, MESSAGE_PRIORITY_MED,
            itti_s10_forward_relocation_request_t,
            s10_forward_relocation_request)
MESSAGE_DEF(S10_FORWARD_RELOCATION_RESPONSE, MESSAGE_PRIORITY_MED,
            itti_s10_forward_relocation_response_t,
            s10_forward_relocation_response)

MESSAGE_DEF(S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION, MESSAGE_PRIORITY_MED,
            itti_s10_forward_access_context_notification_t,
            s10_forward_access_context_notification)
MESSAGE_DEF(S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE, MESSAGE_PRIORITY_MED,
            itti_s10_forward_access_context_acknowledge_t,
            s10_forward_access_context_acknowledge)

MESSAGE_DEF(S10_FORWARD_RELOCATION_COMPLETE_NOTIFICATION, MESSAGE_PRIORITY_MED,
            itti_s10_forward_relocation_complete_notification_t,
            s10_forward_relocation_complete_notification)
MESSAGE_DEF(S10_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGE, MESSAGE_PRIORITY_MED,
            itti_s10_forward_relocation_complete_acknowledge_t,
            s10_forward_relocation_complete_acknowledge)

MESSAGE_DEF(S10_CONTEXT_REQUEST, MESSAGE_PRIORITY_MED,
            itti_s10_context_request_t, s10_context_request)
MESSAGE_DEF(S10_CONTEXT_RESPONSE, MESSAGE_PRIORITY_MED,
            itti_s10_context_response_t, s10_context_response)
MESSAGE_DEF(S10_CONTEXT_ACKNOWLEDGE, MESSAGE_PRIORITY_MED,
            itti_s10_context_acknowledge_t, s10_context_acknowledge)

MESSAGE_DEF(S10_RELOCATION_CANCEL_REQUEST, MESSAGE_PRIORITY_MED,
            itti_s10_relocation_cancel_request_t, s10_relocation_cancel_request)
MESSAGE_DEF(S10_RELOCATION_CANCEL_RESPONSE, MESSAGE_PRIORITY_MED,
            itti_s10_relocation_cancel_response_t,
            s10_relocation_cancel_response)

/** Internal Messages. */
MESSAGE_DEF(S10_REMOVE_UE_TUNNEL, MESSAGE_PRIORITY_MED,
            itti_s10_remove_ue_tunnel_t, s10_remove_ue_tunnel)

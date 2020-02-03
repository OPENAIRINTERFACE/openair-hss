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

/*! \file S11_messages_def.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
MESSAGE_DEF(S11_CREATE_SESSION_REQUEST,  MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S11_CREATE_SESSION_RESPONSE, MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S11_CREATE_BEARER_REQUEST,   MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S11_CREATE_BEARER_RESPONSE,  MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S11_MODIFY_BEARER_REQUEST,   MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S11_MODIFY_BEARER_RESPONSE,  MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S11_DELETE_SESSION_REQUEST,  MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S11_DELETE_SESSION_RESPONSE, MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S11_RELEASE_ACCESS_BEARERS_REQUEST, MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S11_RELEASE_ACCESS_BEARERS_RESPONSE, MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S11_DOWNLINK_DATA_NOTIFICATION, MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S11_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE, MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S11_DOWNLINK_DATA_NOTIFICATION_FAILURE_INDICATION, MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S11_REMOTE_UE_REPORT_NOTIFICATION,  MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S11_REMOTE_UE_REPORT_ACKNOWLEDGE, MESSAGE_PRIORITY_MED)

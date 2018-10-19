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

/*! \file s1ap_messages_def.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

/* Messages for S1AP logging */
MESSAGE_DEF(S1AP_UPLINK_NAS_LOG            , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_UE_CAPABILITY_IND_LOG     , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_INITIAL_CONTEXT_SETUP_LOG , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_NAS_NON_DELIVERY_IND_LOG  , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_DOWNLINK_NAS_LOG          , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_S1_SETUP_LOG              , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_INITIAL_UE_MESSAGE_LOG    , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_REQ_LOG, MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_COMMAND_LOG, MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_LOG    , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_ENB_RESET_LOG             , MESSAGE_PRIORITY_MED)

MESSAGE_DEF(S1AP_UE_CAPABILITIES_IND       ,  MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_ENB_DEREGISTERED_IND      ,  MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_DEREGISTER_UE_REQ         ,  MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_REQ    ,  MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_COMMAND,  MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_COMPLETE, MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_NAS_DL_DATA_REQ           ,  MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_INITIAL_UE_MESSAGE         , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_E_RAB_SETUP_REQ            , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_E_RAB_SETUP_RSP            , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_ENB_INITIATED_RESET_REQ   ,  MESSAGE_PRIORITY_MED)
MESSAGE_DEF(S1AP_ENB_INITIATED_RESET_ACK   ,  MESSAGE_PRIORITY_MED)

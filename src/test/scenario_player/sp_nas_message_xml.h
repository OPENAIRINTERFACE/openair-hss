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

/*****************************************************************************
Source      nas_message_xml.h
Author      Lionel GAUTHIER
*****************************************************************************/
#ifndef FILE_SP_NAS_MESSAGE_XML_SEEN
#define FILE_SP_NAS_MESSAGE_XML_SEEN

#define SECURITY_PROTECTED_NAS_MESSAGE_XML_STR "security_protected_nas_message"
#define PLAIN_NAS_MESSAGE_XML_STR              "plain_nas_message"


bool sp_nas_message_plain_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, nas_message_plain_t * const nas_message);
bool sp_nas_message_protected_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, nas_message_t * const nas_message);
bool sp_nas_pdu_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, bstring *bnas_pdu);


#endif /* FILE_SP_NAS_MESSAGE_XML_SEEN*/

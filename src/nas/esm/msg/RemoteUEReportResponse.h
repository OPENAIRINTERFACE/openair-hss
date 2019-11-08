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

#ifndef REMOTE_UE_REPORT_RESPONSE_H_
#define REMOTE_UE_REPORT_RESPONSE_H_
#include "MessageType.h"
#include "3gpp_23.003.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define REMOTE_UE_REPORT_RESPONSE_MINIMUM_LENGTH (0)

/* Maximum length macro. Formed by maximum length of each field */
#define REMOTE_UE_REPORT_RESPONSE_MAXIMUM_LENGTH 
/*
 * Message name: Remote UE Report Response
 * Description: This message is sent by the network to UE to acknowledge the reception of Remote UE Report message
 * Significance: dual
 * Direction: Network to UE
 */

typedef struct remote_ue_report_response_msg_tag {
  /* Mandatory fields */
  eps_protocol_discriminator_t                           protocoldiscriminator:4;
  ebi_t                                                  epsbeareridentity:4;
  pti_t                                                  proceduretransactionidentity;
  message_type_t                                         messagetype;
  }remote_ue_report_response_msg;

int decode_remote_ue_report_response(remote_ue_report_response_msg *remoteuereportresponse, uint8_t *buffer, uint32_t len);

int encode_remote_ue_report_response(remote_ue_report_response_msg *remoteuereportresponse, uint8_t *buffer, uint32_t);



#endif /* ! defined(REMOTE_UE_REPORT_RESPONSE_H_) */
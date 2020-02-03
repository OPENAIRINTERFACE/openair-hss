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
#include <stdio.h>
#include <stdlib.h>

#ifndef REMOTE_UE_REPORT_SEEN
#define REMOTE_UE_REPORT_SEEN

#include "3gpp_23.003.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "security_types.h"
#include "EsmInformationTransferFlag.h"
#include "MessageType.h"
#include "NasRequestType.h"
#include "PdnType.h"
#include "PKMFAddress.h"
#include "RemoteUEContext.h"

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define REMOTE_UE_REPORT_MINIMUM_LENGTH (0)
/* Maximum length macro. Formed by maximum length of each mandatory field */
#define REMOTE_UE_REPORT_MAXIMUM_LENGTH (20)

#define REMOTE_UE_CONTEXT_PRESENT (1<<0)
#define REMOTE_UE_REPORT_PKMF_ADDRESS_PRESENT (1<<1)

typedef enum remote_ue_report_iei_tag{
REMOTE_UE_REPORT_UE_CONTEXT_IEI = 0x79,
REMOTE_UE_REPORT_PKMF_ADDRESS_IEI = 0x6f,
}remote_ue_report_iei;

/*
 * Message name: Remote UE Report 
 * Description: This message is sent by the UE to the network to inform about an off-network connected or disconnected UE.
 * Significance: dual
 * Direction: UE to network
 */
 typedef struct remote_ue_report_msg_tag{
  eps_protocol_discriminator_t                           protocoldiscriminator:4;
  ebi_t                                                  epsbeareridentity:4;
  pti_t                                                  proceduretransactionidentity;
  message_type_t                                         messagetype;
  /* Optional fields */
  uint32_t                                               presencemask;
  //remote_ue_context_t                                    remoteuecontext;       
  pkmf_address_t                                         pkmfaddress;
  }remote_ue_report_msg;

int decode_remote_ue_report(remote_ue_report_msg *remoteuereport, uint8_t *buffer, uint32_t len);
int encode_remote_ue_report(remote_ue_report_msg *remoteuereport, uint8_t *buffer, uint32_t len);

#endif /* REMOTE_UE_REPORT_SEEN */
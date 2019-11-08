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
#include <stdint.h>
#include "RemoteUserID.h"

#ifndef REMOTE_UE_CONTEXT_SEEN
#define REMOTE_UE_CONTEXT_SEEN

#define REMOTE_UE_CONTEXT_MINIMUM_LENGTH 16
#define REMOTE_UE_CONTEXT_MAXIMUM_LENGTH 28

typedef struct remote_ue_context_s{
    uint8_t spare:4;

#define NUMBER_OF_REMOTE_UE_CONTEXT_IDENTITIES 1
    uint8_t numberofuseridentity:8;

#define EVEN_IDENTITY 0
#define ODD_IDENTITY 1
    uint8_t oddevenindic:1;
    uint8_t flags_present;
    imsi_identity_t *imsi_identity;
}remote_ue_context_t;


int encode_remote_ue_context(remote_ue_context_t *remoteuecontext, uint8_t iei, uint8_t *buffer, uint32_t len);
int decode_remote_ue_context(remote_ue_context_t *remoteuecontext, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* REMOTE_UE_CONTEXT_SEEN */
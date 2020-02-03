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
//#include "RemoteUserID.h"

#ifndef REMOTE_UE_CONTEXT_SEEN
#define REMOTE_UE_CONTEXT_SEEN

#define REMOTE_UE_CONTEXT_MINIMUM_LENGTH 16
#define REMOTE_UE_CONTEXT_MAXIMUM_LENGTH 28


typedef struct remote_ue_ip_remote_ue_context_s{
  uint8_t spare:5;
  #define ADDRESS_TYPE_IPV4 0b001
  #define ADDRESS_TYPE_IPV6 0b010
  uint8_t addresstype:3;
  #define REMOTE_UE_IP_ADDRESS
  union{
  struct in_addr *ipv4;
  struct in6_addr *ipv6;  
  }remoteueipaddress;
}remote_ue_ip_remote_ue_context_t;


typedef struct imsi_remote_ue_context_s {
  uint8_t  identity_digit1:4;
  #define REMOTE_UE_CONTEXT_IDENTITY_EVEN  0
  #define REMOTE_UE_CONTEXT_IDENTITY_ODD   1
  uint8_t  oddeven:1;
  uint8_t  typeofidentity:3;
  uint8_t  identity_digit2:4;
  uint8_t  identity_digit3:4;
  uint8_t  identity_digit4:4;
  uint8_t  identity_digit5:4;
  uint8_t  identity_digit6:4;
  uint8_t  identity_digit7:4;
  uint8_t  identity_digit8:4;
  uint8_t  identity_digit9:4;
  uint8_t  identity_digit10:4;
  uint8_t  identity_digit11:4;
  uint8_t  identity_digit12:4;
  uint8_t  identity_digit13:4;
  uint8_t  identity_digit14:4;
  uint8_t  identity_digit15:4;
  // because of union put this extra attribute at the end
  uint8_t  num_digits;
} imsi_remote_ue_context_t;

typedef struct encrypted_imsi_remote_ue_context_s {
  uint8_t  identity_digit1:4;
  #define REMOTE_UE_CONTEXT_IDENTITY_EVEN  0
  #define REMOTE_UE_CONTEXT_IDENTITY_ODD   1
  uint8_t  oddeven:1;
  uint8_t  typeofidentity:3;
  uint8_t  identity_digit2:4;
  uint8_t  identity_digit3:4;
  uint8_t  identity_digit4:4;
  uint8_t  identity_digit5:4;
  uint8_t  identity_digit6:4;
  uint8_t  identity_digit7:4;
  uint8_t  identity_digit8:4;
  uint8_t  identity_digit9:4;
  uint8_t  identity_digit10:4;
  uint8_t  identity_digit11:4;
  uint8_t  identity_digit12:4;
  uint8_t  identity_digit13:4;
  uint8_t  identity_digit14:4;
  uint8_t  identity_digit15:4;
  // because of union put this extra attribute at the end
  uint8_t  num_digits;
} encrypted_imsi_remote_ue_context_t;

typedef struct remote_ue_context_s{
  #define NUMBER_OF_REMOTE_UE_CONTEXT_IDENTITIES 2
  #define REMOTE_UE_CONTEXT_IDENTITY_ENCRYPTED_IMSI  0b001
  #define REMOTE_UE_CONTEXT_IDENTITY_IMSI            0b010
  encrypted_imsi_remote_ue_context_t encrypted_imsi;
  imsi_remote_ue_context_t imsi;
  remote_ue_ip_remote_ue_context_t remoteueipaddress;
  //uint8_t numberofuseridentity:8;

#define EVEN_IDENTITY 0
#define ODD_IDENTITY 1
uint8_t oddevenindic:1;
}remote_ue_context_t;


int encode_remote_ue_context(remote_ue_context_t *remoteuecontext, uint8_t iei, uint8_t *buffer, uint32_t len);
int decode_remote_ue_context(remote_ue_context_t *remoteuecontext, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* REMOTE_UE_CONTEXT_SEEN */
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
#include "EpsMobileIdentity.h"

#ifndef REMOTE_USER_ID_SEEN
#define REMOTE_USER_ID_SEEN

#define REMOTE_USER_ID_MINIMUM_LENGTH 10

typedef struct imsi_identity_s{
uint8_t spareimsi:4;
#define IMSI_INSTANCE 0
uint8_t instanceimsi:4;
uint8_t identity_digit1:4;

#define IMSI_EVEN 0
#define IMSI_ODD 1
uint8_t oddeven:1;
#define REMOTE_UE_MOBILE_IDENTITY_IMSI 010
uint8_t typeofidentity:3;
uint8_t identity_digit2:4;
uint8_t identity_digit3:4;
uint8_t identity_digit4:4;
uint8_t identity_digit5:4;
uint8_t identity_digit6:4;
uint8_t identity_digit7:4;
uint8_t identity_digit8:4;
uint8_t identity_digit9:4;
uint8_t identity_digit10:4;
uint8_t identity_digit11:4;
uint8_t identity_digit12:4;
uint8_t identity_digit13:4;
uint8_t identity_digit14:4;
uint8_t identity_digit15:4;
uint8_t num_digits;

}imsi_identity_t;

typedef struct remote_user_id_s
{
uint8_t spare:4;
#define REMOTE_USER_ID_INSTANCE 0
uint8_t instance:4;
uint8_t spare1:6;

#define REMOTE_USER_ID_IMEIF 0
uint8_t imeif:1;

#define REMOTE_USER_ID_MSISDN 0
uint8_t msisdnf:1;
imsi_identity_t *imsi_identity;

uint8_t flags_present;
uint8_t spare_instance; 
  //imsi_identity_t *imsi_identity;
}remote_user_id_t;

int encode_remote_user_id(remote_user_id_t *remoteuserid, uint8_t iei, uint8_t *buffer, uint32_t len);
int decode_remote_user_id(remote_user_id_t *remoteuserid, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* REMOTE_USER_ID_SEEN */
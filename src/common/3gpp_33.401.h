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

/*! \file 3gpp_33.401.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#include "security_types.h"

#ifndef FILE_3GPP_33_401_SEEN
#define FILE_3GPP_33_401_SEEN

//------------------------------------------------------------------------------
// 5.1.3.2 Algorithm Identifier Values
//------------------------------------------------------------------------------
#define EEA0_ALG_ID        0b000
#define EEA1_128_ALG_ID    0b001
#define EEA2_128_ALG_ID    0b010

//------------------------------------------------------------------------------
// 5.1.4.2 Algorithm Identifier Values
//------------------------------------------------------------------------------
#define EIA0_ALG_ID        0b000
#define EIA1_128_ALG_ID    0b001
#define EIA2_128_ALG_ID    0b010

//------------------------------------------------------------------------------
// 6.1.2 Distribution of authentication data from HSS to serving network
//------------------------------------------------------------------------------
/* NOTE 2: It is recommended that the MME fetch only one EPS authentication vector at a time as the need to perform
 * AKA runs has been reduced in EPS through the use of a more elaborate key hierarchy. In particular,
 * service requests can be authenticated using a stored K ASME without the need to perform AKA.
 * Furthermore, the sequence number management schemes in TS 33.102, Annex C [4], designed to avoid
 * re-synchronisation problems caused by interleaving use of batches of authentication vectors, are only
 * optional. Re-synchronisation problems in EPS can be avoided, independently of the sequence number
 * management scheme, by immediately using an authentication vector retrieved from the HSS in an
 * authentication procedure between UE and MME.
 */
#define MAX_EPS_AUTH_VECTORS          1


//----------------------------
typedef struct mm_ue_eps_authentication_quadruplet_s{
  uint8_t                   rand[16];
  uint8_t                   xres_len;
  uint8_t                   xres[XRES_LENGTH_MAX];
  uint8_t                   autn_len;
  uint8_t                   autn[AUTN_LENGTH_OCTETS];
  uint8_t                   k_asme[32];
} mm_ue_eps_authentication_quadruplet_t;

typedef struct mm_ue_eps_authentication_quintuplet_s{
  uint8_t                   rand[16];
  uint8_t                   xres_len;
  uint8_t                   xres[XRES_LENGTH_MAX];
  uint8_t                   ck[16];
  uint8_t                   ik[16];
  uint8_t                   autn_len;
  uint8_t                   autn[AUTN_LENGTH_OCTETS];
} mm_ue_eps_authentication_quintuplet_t;

#endif /* FILE_3GPP_33_401_SEEN */

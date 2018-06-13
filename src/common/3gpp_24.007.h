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

/*! \file 3gpp_24.007.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_3GPP_24_007_SEEN
#define FILE_3GPP_24_007_SEEN

//..............................................................................
// 11.2.3.1.1  Protocol discriminator
//..............................................................................
typedef uint8_t                       eps_protocol_discriminator_t; //4 bits only


typedef enum eps_protocol_discriminator_value_e {
  GROUP_CALL_CONTROL                  =    0x0,
  BROADCAST_CALL_CONTROL              =    0x1,
  EPS_SESSION_MANAGEMENT_MESSAGE      =    0x2, /* Protocol discriminator identifier for EPS Session Management */
  CALL_CONTROL_CC_RELATED_SS_MESSAGE  =    0x3,
  GPRS_TRANSPARENT_TRANSPORT_PROTOCOL =    0x4,
  MOBILITY_MANAGEMENT_MESSAGE         =    0x5,
  RADIO_RESOURCES_MANAGEMENT_MESSAGE  =    0x6,
  EPS_MOBILITY_MANAGEMENT_MESSAGE     =    0x7, /* Protocol discriminator identifier for EPS Mobility Management */
  GPRS_MOBILITY_MANAGEMENT_MESSAGE    =    0x8,
  SMS_MESSAGE                         =    0x9,
  GPRS_SESSION_MANAGEMENT_MESSAGE     =    0xA,
  NON_CALL_RELATED_SS_MESSAGE         =    0xB,
} eps_protocol_discriminator_value_t;

//..............................................................................
// 11.2.3.1.5  EPS bearer identity
//..............................................................................
typedef uint8_t                       ebi_t; //4 bits only

#define EPS_BEARER_IDENTITY_UNASSIGNED   (ebi_t)0
#define EPS_BEARER_IDENTITY_RESERVED1    (ebi_t)1
#define EPS_BEARER_IDENTITY_RESERVED2    (ebi_t)2
#define EPS_BEARER_IDENTITY_RESERVED3    (ebi_t)3
#define EPS_BEARER_IDENTITY_RESERVED4    (ebi_t)4
#define EPS_BEARER_IDENTITY_FIRST        (ebi_t)5
#define EPS_BEARER_IDENTITY_LAST         (ebi_t)15
//..............................................................................
// 11.2.3.1a   Procedure transaction identity
//..............................................................................
typedef uint8_t                       pti_t;

#define PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED   (pti_t)0
#define PROCEDURE_TRANSACTION_IDENTITY_FIRST        (pti_t)1
#define PROCEDURE_TRANSACTION_IDENTITY_LAST         (pti_t)254
#define PROCEDURE_TRANSACTION_IDENTITY_RESERVED     (pti_t)255


#endif /* FILE_3GPP_24_007_SEEN */

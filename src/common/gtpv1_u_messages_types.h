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

/*! \file gtpv1_u_messages_types.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_GTPV1_U_MESSAGES_TYPES_SEEN
#define FILE_GTPV1_U_MESSAGES_TYPES_SEEN

#include "3gpp_24.007.h"
#include "common_types.h"

typedef struct {
  teid_t context_teid;  ///< Tunnel Endpoint Identifier
  ebi_t eps_bearer_id;
} Gtpv1uCreateTunnelReq;

typedef struct {
  uint8_t status;       ///< Status of S1U endpoint creation (Failed = 0xFF or
                        ///< Success = 0x0)
  teid_t context_teid;  ///< local SGW S11 Tunnel Endpoint Identifier
  teid_t S1u_teid;      ///< Tunnel Endpoint Identifier
  ebi_t eps_bearer_id;
} Gtpv1uCreateTunnelResp;

typedef struct {
  teid_t context_teid;  ///< S11 Tunnel Endpoint Identifier
  teid_t sgw_S1u_teid;  ///< SGW S1U local Tunnel Endpoint Identifier
  teid_t enb_S1u_teid;  ///< eNB S1U Tunnel Endpoint Identifier
  ip_address_t enb_ip_address_for_S1u;
  ebi_t eps_bearer_id;
} Gtpv1uUpdateTunnelReq;

typedef struct {
  uint8_t status;       ///< Status (Failed = 0xFF or Success = 0x0)
  teid_t context_teid;  ///< S11 Tunnel Endpoint Identifier
  teid_t sgw_S1u_teid;  ///< SGW S1U local Tunnel Endpoint Identifier
  teid_t enb_S1u_teid;  ///< eNB S1U Tunnel Endpoint Identifier
  ebi_t eps_bearer_id;
} Gtpv1uUpdateTunnelResp;

typedef struct {
  teid_t context_teid;  ///< local SGW S11 Tunnel Endpoint Identifier
  teid_t S1u_teid;      ///< local S1U Tunnel Endpoint Identifier to be deleted
} Gtpv1uDeleteTunnelReq;

typedef struct {
  uint8_t status;       ///< Status of S1U endpoint deleteion (Failed = 0xFF or
                        ///< Success = 0x0)
  teid_t context_teid;  ///< local SGW S11 Tunnel Endpoint Identifier
  teid_t S1u_teid;      ///< local S1U Tunnel Endpoint Identifier to be deleted
} Gtpv1uDeleteTunnelResp;

typedef struct {
  uint8_t* buffer;
  uint32_t length;
  uint32_t offset;        ///< start of message offset in buffer
  teid_t local_S1u_teid;  ///< Tunnel Endpoint Identifier
} Gtpv1uTunnelDataInd;

typedef struct {
  uint8_t* buffer;
  uint32_t length;
  uint32_t offset;        ///< start of message offset in buffer
  teid_t local_S1u_teid;  ///< Tunnel Endpoint Identifier
  teid_t S1u_enb_teid;    ///< Tunnel Endpoint Identifier
} Gtpv1uTunnelDataReq;

#endif /* FILE_GTPV1_U_MESSAGES_TYPES_SEEN */

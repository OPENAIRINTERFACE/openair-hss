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
/*! \file udp_messages_types.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#ifndef FILE_UDP_MESSAGES_TYPES_SEEN
#define FILE_UDP_MESSAGES_TYPES_SEEN

#ifdef __cplusplus
extern "C" {
#endif

#define UDP_INIT(mSGpTR)        ((udp_init_t*)(mSGpTR)->itti_msg)
#define UDP_DATA_IND(mSGpTR)    ((udp_data_ind_t*)(mSGpTR)->itti_msg)
#define UDP_DATA_REQ(mSGpTR)    ((udp_data_req_t*)(mSGpTR)->itti_msg)
#define UDP_DATA_MAX_MSG_LEN    (4096)  /**< Maximum supported gtpv2c packet length including header */

typedef struct {
  struct in_addr  in_addr;
  struct in6_addr  in6_addr;
  uint16_t        port;
} udp_init_t;

typedef struct {
  uint8_t  *buffer;
  uint32_t  buffer_length;
  uint32_t  buffer_offset;
  struct sockaddr  *peer_address;
  uint16_t  peer_port;
} udp_data_req_t;

typedef struct {
  uint8_t  *buffer;
  uint32_t  buffer_length;
  struct sockaddr   peer_address;
  uint16_t  peer_port;
} udp_data_ind_t;

#ifdef __cplusplus
}
#endif
#endif /* FILE_UDP_MESSAGES_TYPES_SEEN */

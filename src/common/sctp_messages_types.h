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

/*! \file sctp_messages_types.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_SCTP_MESSAGES_TYPES_SEEN
#define FILE_SCTP_MESSAGES_TYPES_SEEN

#define SCTP_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.sctp_data_ind
#define SCTP_DATA_REQ(mSGpTR)           (mSGpTR)->ittiMsg.sctp_data_req
#define SCTP_DATA_CNF(mSGpTR)           (mSGpTR)->ittiMsg.sctp_data_cnf
#define SCTP_INIT_MSG(mSGpTR)           (mSGpTR)->ittiMsg.sctpInit
#define SCTP_NEW_ASSOCIATION(mSGpTR)    (mSGpTR)->ittiMsg.sctp_new_peer
#define SCTP_CLOSE_ASSOCIATION(mSGpTR)  (mSGpTR)->ittiMsg.sctp_close_association


//typedef struct sctp_data_rej_s {
//  sctp_assoc_id_t  assoc_id;
//  sctp_stream_id_t stream;
//  uint32_t         mme_ue_s1ap_id;
//} sctp_data_rej_t;

typedef struct sctp_data_cnf_s {
  sctp_assoc_id_t  assoc_id;
  sctp_stream_id_t stream;
  uint32_t         mme_ue_s1ap_id;
  bool             is_success;
} sctp_data_cnf_t;

typedef struct sctp_data_req_s {
  bstring          payload;
  sctp_assoc_id_t  assoc_id;
  sctp_stream_id_t stream;
  uint32_t         mme_ue_s1ap_id; // for helping data_rej
} sctp_data_req_t;

typedef struct sctp_data_ind_s {
  bstring            payload;          ///< SCTP buffer
  sctp_assoc_id_t    assoc_id;         ///< SCTP physical association ID
  sctp_stream_id_t   stream;           ///< Stream number on which data had been received
  uint16_t           instreams;        ///< Number of input streams for the SCTP connection between peers
  uint16_t           outstreams;       ///< Number of output streams for the SCTP connection between peers
} sctp_data_ind_t;

typedef struct sctp_init_s {
  /* Request usage of ipv4 */
  unsigned  ipv4:1;
  /* Request usage of ipv6 */
  unsigned  ipv6:1;
  uint8_t   nb_ipv4_addr;
  struct in_addr  ipv4_address[10];
  uint8_t   nb_ipv6_addr;
  struct in6_addr ipv6_address[10];
  uint16_t  port;
  uint32_t  ppid;
} SctpInit;

typedef struct sctp_close_association_s {
  sctp_assoc_id_t  assoc_id;
  bool             reset;
} sctp_close_association_t;

typedef struct sctp_new_peer_s {
  uint32_t        instreams;
  uint32_t        outstreams;
  sctp_assoc_id_t assoc_id;
} sctp_new_peer_t;

#endif /* FILE_SCTP_MESSAGES_TYPES_SEEN */

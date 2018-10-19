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

/*! \file sctp_messages_def.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

//WARNING: Do not include this header directly. Use intertask_interface.h instead.

MESSAGE_DEF(SCTP_INIT_MSG,          MESSAGE_PRIORITY_MED, SctpInit,                 sctpInit)
MESSAGE_DEF(SCTP_DATA_REQ,          MESSAGE_PRIORITY_MED, sctp_data_req_t,          sctp_data_req)
MESSAGE_DEF(SCTP_DATA_IND,          MESSAGE_PRIORITY_MED, sctp_data_ind_t,          sctp_data_ind)
MESSAGE_DEF(SCTP_DATA_CNF,          MESSAGE_PRIORITY_MED, sctp_data_cnf_t,          sctp_data_cnf)
MESSAGE_DEF(SCTP_NEW_ASSOCIATION,   MESSAGE_PRIORITY_MAX, sctp_new_peer_t,          sctp_new_peer)
MESSAGE_DEF(SCTP_CLOSE_ASSOCIATION, MESSAGE_PRIORITY_MAX, sctp_close_association_t, sctp_close_association)

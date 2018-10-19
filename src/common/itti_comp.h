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

/*! \file itti_comp.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_ITTI_COMP_SEEN
#define FILE_ITTI_COMP_SEEN

int itti_msg_comp_sctp_new_association(const sctp_new_peer_t * const itti_msg1, const sctp_new_peer_t * const itti_msg2);
int itti_msg_comp_sctp_close_association(const sctp_close_association_t * const itti_msg1, const sctp_close_association_t * const itti_msg2);
int itti_msg_comp_s1ap_ue_context_release_req(const itti_s1ap_ue_context_release_req_t * const itti_msg1, const itti_s1ap_ue_context_release_req_t * const itti_msg2);
int itti_msg_comp_mme_app_connection_establishment_cnf(const itti_mme_app_connection_establishment_cnf_t * const itti_msg1, const itti_mme_app_connection_establishment_cnf_t * const itti_msg2);
int itti_msg_comp_nas_downlink_data_req(const itti_nas_dl_data_req_t * const itti_msg1, const itti_nas_dl_data_req_t * const itti_msg2);

#endif


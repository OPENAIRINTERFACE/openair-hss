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

/*! \file mme_app_itti_messaging.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/


#ifndef FILE_MME_APP_ITTI_MESSAGING_SEEN
#define FILE_MME_APP_ITTI_MESSAGING_SEEN

int mme_app_notify_s1ap_ue_context_released(const mme_ue_s1ap_id_t   ue_idP);
int mme_app_send_nas_signalling_connection_rel_ind(const mme_ue_s1ap_id_t ue_id);
int mme_app_send_s11_release_access_bearers_req (struct ue_context_s *const ue_context, const pdn_cid_t pdn_index);
int mme_app_send_s11_create_session_req (  struct ue_context_s *const ue_context, pdn_context_t * pdn_context, tai_t * serving_tai);
int mme_app_send_s11_modify_bearer_req(struct ue_context_s *const ue_context, pdn_context_t * pdn_context);
int mme_app_remove_s10_tunnel_endpoint(teid_t local_teid, teid_t remote_teid, struct in_addr peer_ip);
int mme_app_send_delete_session_request (struct ue_context_s * const ue_context_p, const ebi_t ebi, const pdn_context_t* pdn_context); /**< Moved Delete Session Request from mme_app_detach. */

int
mme_app_send_s11_create_bearer_rsp (
  struct ue_context_s *const ue_context,
//  struct in_addr  peer_ip,
  teid_t          saegw_s11_teid,
  LIST_HEAD(bearer_contexts_sucess_s, bearer_context_s) *bearer_contexts_success,
  LIST_HEAD(bearer_contexts_failed_s, bearer_context_s) *bearer_contexts_failed);

#endif /* FILE_MME_APP_ITTI_MESSAGING_SEEN */

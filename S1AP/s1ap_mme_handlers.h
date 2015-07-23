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


#include "s1ap_ies_defs.h"
#include "intertask_interface.h"

#ifndef S1AP_MME_HANDLERS_H_
#define S1AP_MME_HANDLERS_H_

/** \brief Handle decoded incoming messages from SCTP
 * \param assoc_id SCTP association ID
 * \param stream Stream number
 * \param message_p The message decoded by the ASN1C decoder
 * @returns int
 **/
int s1ap_mme_handle_message(uint32_t assoc_id, uint32_t stream,
                            struct s1ap_message_s *message_p);

int s1ap_mme_handle_ue_cap_indication(uint32_t assoc_id, uint32_t stream,
                                      struct s1ap_message_s *message);

/** \brief Handle an S1 Setup request message.
 * Typically add the eNB in the list of served eNB if not present, simply reset
 * UEs association otherwise. S1SetupResponse message is sent in case of success or
 * S1SetupFailure if the MME cannot accept the configuration received.
 * \param assoc_id SCTP association ID
 * \param stream Stream number
 * \param message_p The message decoded by the ASN1C decoder
 * @returns int
 **/
int s1ap_mme_handle_s1_setup_request(uint32_t assoc_id, uint32_t stream,
                                     struct s1ap_message_s *message_p);

int s1ap_mme_handle_path_switch_request(uint32_t assoc_id, uint32_t stream,
                                        struct s1ap_message_s *message_p);

int s1ap_mme_handle_ue_context_release_request(uint32_t assoc_id,
    uint32_t stream, struct s1ap_message_s *message_p);

int s1ap_handle_ue_context_release_command(
		const s1ap_ue_context_release_command_t * const ue_context_release_command_pP);

int s1ap_mme_handle_ue_context_release_complete(uint32_t assoc_id,
    uint32_t stream, struct s1ap_message_s *message_p);

int s1ap_mme_handle_initial_context_setup_failure(uint32_t assoc_id,
    uint32_t stream, struct s1ap_message_s *message_p);

int s1ap_mme_handle_initial_context_setup_response(
  uint32_t assoc_id,
  uint32_t stream,
  struct s1ap_message_s *message_p);

int s1ap_handle_sctp_deconnection(uint32_t assoc_id);

int s1ap_handle_new_association(sctp_new_peer_t *sctp_new_peer_p);

int s1ap_handle_create_session_response(SgwCreateSessionResponse
                                        *session_response_p);

int s1ap_mme_set_cause(S1ap_Cause_t *cause_p, S1ap_Cause_PR cause_type, long cause_value);

int s1ap_mme_generate_s1_setup_failure(
  uint32_t assoc_id, S1ap_Cause_PR cause_type, long cause_value,
  long time_to_wait);

#endif /* S1AP_MME_HANDLERS_H_ */

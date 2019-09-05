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

/*! \file mme_app_bearer_context.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_MME_APP_BEARER_CONTEXT_SEEN
#define FILE_MME_APP_BEARER_CONTEXT_SEEN


/** Create & deallocate a bearer context. Will also initialize the bearer contexts. */
void clear_bearer_context(ue_session_pool_t * ue_session_pool, bearer_context_new_t * bc);

/** Find an allocated PDN session bearer context. */
bearer_context_new_t* mme_app_get_session_bearer_context(pdn_context_t * const pdn_context, const ebi_t ebi);

void mme_app_get_free_bearer_context(ue_session_pool_t * const ue_sp, const ebi_t ebi, bearer_context_new_t** bc_pp);

/*
 * Receive Bearer Context VOs to send in CSR/Handover Request, etc..
 * Will set bearer state, unless it is null.
 */
void mme_app_get_bearer_contexts_to_be_created(pdn_context_t * pdn_context, bearer_contexts_to_be_created_t *bc_tbc, mme_app_bearer_state_t bc_state);

// todo_: combine these two methods
void mme_app_get_session_bearer_context_from_all(ue_session_pool_t * const ue_session_pool, const ebi_t ebi, bearer_context_new_t ** bc_pp);

/*
 * New method to get a bearer context from the bearer pool of the UE context and add it into the pdn session.
 * If the file using this method does not include the header file, the returned pointer is garbage. We overcome this with giving the PP.
 */
esm_cause_t mme_app_register_dedicated_bearer_context(const mme_ue_s1ap_id_t ue_id, const esm_ebr_state esm_ebr_state, pdn_cid_t pdn_cid, ebi_t linked_ebi, bearer_context_to_be_created_t * const bc_tbu, const ebi_t ded_ebi);

void mme_app_bearer_context_s1_release_enb_informations(bearer_context_new_t * const bc);

/*
 * Update the bearer context for initial context setup response and handover.
 */
int mme_app_modify_bearers(const mme_ue_s1ap_id_t mme_ue_s1ap_id, bearer_contexts_to_be_modified_t *bcs_to_be_modified);

/*
 * Set bearers as released (idle).
 * todo: review idle mode..
 */
void mme_app_release_bearers(const mme_ue_s1ap_id_t mme_ue_s1ap_id, e_rab_list_t * e_rab_list, ebi_list_t * const ebi_list);

#endif

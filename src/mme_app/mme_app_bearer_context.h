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

bstring bearer_state2string(const mme_app_bearer_state_t bearer_state);
/** Create & deallocate a bearer context. */
bearer_context_t *mme_app_new_bearer();
void mme_app_bearer_context_initialize(bearer_context_t *bearer_context);
/** Find an allocated PDN session bearer context. */
bearer_context_t* mme_app_get_session_bearer_context(pdn_context_t * const pdn_context, const ebi_t ebi);

// todo_: combine these two methods
void mme_app_get_session_bearer_context_from_all(ue_context_t * const ue_context, const ebi_t ebi, bearer_context_t ** bc_pp);

/*
 * New method to get a bearer context from the bearer pool of the UE context and add it into the pdn session.
 * If the file using this method does not include the header file, the returned pointer is garbage. We overcome this with giving the PP.
 */
esm_cause_t mme_app_register_dedicated_bearer_context(const mme_ue_s1ap_id_t ue_id, const esm_ebr_state esm_ebr_state, pdn_cid_t pdn_cid, ebi_t linked_ebi, bearer_context_to_be_created_t * const bc_tbu);

/*
 * Method to deregister a bearer context.
 * Could be called from the MME_APP layer as well as ESM layer.
 * It will not remove any ESM procedures/stop timers, that should be done in ESM layer before calling this method..
 */
int mme_app_deregister_bearer_context(ue_context_t * const ue_context, ebi_t ebi, const pdn_context_t *pdn_context);

void mme_app_free_bearer_context (bearer_context_t ** const bearer_context);
void mme_app_bearer_context_s1_release_enb_informations(bearer_context_t * const bc);

void mme_app_bearer_context_update_handover(bearer_context_t * bc_registered, bearer_context_to_be_created_t * const bc_tbc_s10);

#endif

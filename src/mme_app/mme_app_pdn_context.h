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
/*! \file mme_app_pdn_context.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#ifndef FILE_MME_APP_PDN_CONTEXT_SEEN
#define FILE_MME_APP_PDN_CONTEXT_SEEN

/** PDN Context and Bearer Context operations from ESM (UE triggered) or handover. */
int mme_app_esm_create_pdn_context(mme_ue_s1ap_id_t ue_id, const ebi_t linked_ebi, const apn_configuration_t *apn_configuration, const bstring apn, pdn_cid_t pdn_cid, const ambr_t * const apn_ambr, pdn_context_t **pdn_context_pp);

/** Update the created PDN context from the APN configuration of the subscription. */
esm_cause_t mme_app_update_pdn_context(mme_ue_s1ap_id_t ue_id, const subscription_data_t * const subscription_data);

/**
 * Release all bearers of a PDN context and release the PDN context.
 * If this method is triggered by the ESM layer, the ESM procedures/timers need to be released before..
 */
void mme_app_esm_delete_pdn_context(mme_ue_s1ap_id_t ue_id, bstring apn, pdn_cid_t pdn_cid, ebi_t linked_ebi);

void mme_app_esm_detach (mme_ue_s1ap_id_t ue_id);

int
mme_app_esm_update_ebr_state(const mme_ue_s1ap_id_t ue_id, const bstring apn_subscribed, const pdn_cid_t pdn_cid, const ebi_t linked_ebi, const ebi_t bearer_ebi, esm_ebr_state ebr_state);

/*
 * Process S11 session creation.
 */
int
mme_app_pdn_process_session_creation(mme_ue_s1ap_id_t ue_id, fteid_t * saegw_s11_fteid, gtpv2c_cause_t *cause,
    bearer_contexts_created_t * bcs_created, ambr_t *ambr, paa_t ** paa, protocol_configuration_options_t * pco);

int
mme_app_cn_update_bearer_context(mme_ue_s1ap_id_t ue_id, const ebi_t ebi,
    struct e_rab_setup_item_s * s1u_erab_setup_item, struct fteid_s * s1u_saegw_fteid);

esm_cause_t
mme_app_finalize_bearer_context(mme_ue_s1ap_id_t ue_id, const pdn_cid_t pdn_cid, const ebi_t def_ebi, const ebi_t ebi, ambr_t *ambr, bearer_qos_t * bearer_level_qos, traffic_flow_template_t * tft,
    protocol_configuration_options_t * pco);

void
mme_app_release_bearer_context(mme_ue_s1ap_id_t ue_id, const pdn_cid_t *pdn_cid, const ebi_t linked_ebi, const ebi_t ebi);

esm_cause_t
mme_app_esm_modify_bearer_context(mme_ue_s1ap_id_t ue_id, const ebi_t ebi, ebi_list_t * const ded_ebis, const esm_ebr_state esm_ebr_state, struct bearer_qos_s * bearer_level_qos, traffic_flow_template_t * tft, ambr_t *apn_ambr);

void mme_app_get_pdn_context (mme_ue_s1ap_id_t ue_id, pdn_cid_t const context_id, ebi_t const default_ebi, bstring const subscribed_apn, pdn_context_t **pdn_ctx);

esm_cause_t
mme_app_validate_bearer_resource_modification(mme_ue_s1ap_id_t ue_id, ebi_t ebi, traffic_flow_aggregate_description_t *tad, flow_qos_t * flow_qos);

/*
 * Receive Bearer Context VOs to send in CSR/Handover Request, etc..
 * Will set bearer state, unless it is null.
 */
void mme_app_get_bearer_contexts_to_be_created(pdn_context_t * pdn_context, bearer_contexts_to_be_created_t *bc_tbc, mme_app_bearer_state_t bc_state);

#endif

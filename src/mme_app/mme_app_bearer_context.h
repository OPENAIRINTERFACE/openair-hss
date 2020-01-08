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

#include "esm_data.h"

typedef uint8_t mme_app_bearer_state_t;

/*
 * @struct bearer_context_new_t
 * @brief Parameters that should be kept for an eps bearer. Used for stacked memory
 *
 * Structure of an EPS bearer
 * --------------------------
 * An EPS bearer is a logical concept which applies to the connection
 * between two endpoints (UE and PDN Gateway) with specific QoS attri-
 * butes. An EPS bearer corresponds to one Quality of Service policy
 * applied within the EPC and E-UTRAN.
 */
typedef struct bearer_context_new_s {
  // EPS Bearer ID: An EPS bearer identity uniquely identifies an EP S bearer for one UE accessing via E-UTRAN
  ebi_t                       				ebi;
  ebi_t                       				linked_ebi;

  // S-GW IP address for S1-u: IP address of the S-GW for the S1-u interfaces.
  // S-GW TEID for S1u: Tunnel Endpoint Identifier of the S-GW for the S1-u interface.
  fteid_t                      				s_gw_fteid_s1u;            // set by S11 CREATE_SESSION_RESPONSE

  // PDN GW TEID for S5/S8 (user plane): P-GW Tunnel Endpoint Identifier for the S5/S8 interface for the user plane. (Used for S-GW change only).
  // NOTE:
  // The PDN GW TEID is needed in MME context as S-GW relocation is triggered without interaction with the source S-GW, e.g. when a TAU
  // occurs. The Target S-GW requires this Information Element, so it must be stored by the MME.
  // PDN GW IP address for S5/S8 (user plane): P GW IP address for user plane for the S5/S8 interface for the user plane. (Used for S-GW change only).
  // NOTE:
  // The PDN GW IP address for user plane is needed in MME context as S-GW relocation is triggered without interaction with the source S-GW,
  // e.g. when a TAU occurs. The Target S GW requires this Information Element, so it must be stored by the MME.
  fteid_t                      				p_gw_fteid_s5_s8_up;

  // EPS bearer QoS: QCI and ARP, optionally: GBR and MBR for GBR bearer

  // extra 23.401 spec members
  pdn_cid_t                         	pdn_cx_id;

  /*
   * Two bearer states, one mme_app_bearer_state (towards SAE-GW) and one towards eNodeB (if activated in RAN).
   * todo: setting one, based on the other is possible?
   */
  mme_app_bearer_state_t            	bearer_state;     /**< Need bearer state to establish them. */
  esm_ebr_context_t                 	esm_ebr_context;  /**< Contains the bearer level QoS parameters. */
  fteid_t                           	enb_fteid_s1u;

  /* QoS for this bearer */
  bearer_qos_t                				bearer_level_qos;

  /** Add an entry field to make it part of a list (session or UE, no need to save more lists). */
  // LIST_ENTRY(bearer_context_new_s) 	entries;
  STAILQ_ENTRY (bearer_context_new_s)	entries;
}__attribute__((__packed__)) bearer_context_new_t;

/*
 * New method to get a bearer context from the bearer pool of the UE context and add it into the pdn session.
 * If the file using this method does not include the header file, the returned pointer is garbage. We overcome this with giving the PP.
 */
esm_cause_t mme_app_register_dedicated_bearer_context(const mme_ue_s1ap_id_t ue_id, const esm_ebr_state esm_ebr_state, pdn_cid_t pdn_cid, ebi_t linked_ebi, bearer_context_to_be_created_t * const bc_tbu, const ebi_t ded_ebi);

void mme_app_bearer_context_s1_release_enb_informations(struct bearer_context_new_s * const bc);

/*
 * Update the bearer context for initial context setup response and handover.
 */
int mme_app_modify_bearers(const mme_ue_s1ap_id_t mme_ue_s1ap_id, bearer_contexts_to_be_modified_t *bcs_to_be_modified);

/*
 * Set bearers as released (idle).
 * todo: review idle mode..
 */
void mme_app_release_bearers(const mme_ue_s1ap_id_t mme_ue_s1ap_id, const e_rab_list_t * const e_rab_list, ebi_list_t * const ebi_list);

#endif

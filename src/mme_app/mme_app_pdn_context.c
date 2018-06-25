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


/*! \file mme_app_pdn_context.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "common_types.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "sgw_ie_defs.h"
#include "common_defs.h"
#include "mme_app_pdn_context.h"
#include "mme_app_apn_selection.h"

static void mme_app_pdn_context_init(ue_context_t * const ue_context, pdn_context_t *const  pdn_context);

//------------------------------------------------------------------------------
void mme_app_free_pdn_context (pdn_context_t ** const pdn_context)
{
  if ((*pdn_context)->apn_in_use) {
    bdestroy_wrapper(&(*pdn_context)->apn_in_use);
  }
  if ((*pdn_context)->apn_subscribed) {
    bdestroy_wrapper(&(*pdn_context)->apn_subscribed);
  }
  if ((*pdn_context)->apn_oi_replacement) {
    bdestroy_wrapper(&(*pdn_context)->apn_oi_replacement);
  }
  if ((*pdn_context)->pco) {
    free_protocol_configuration_options(&(*pdn_context)->pco);
  }
  // todo: free PAA
  if((*pdn_context)->paa){
    free_wrapper((void**)&((*pdn_context)->paa));
  }

  memset(&(*pdn_context)->esm_data, 0, sizeof((*pdn_context)->esm_data));

  free_wrapper((void**)pdn_context);
}

//------------------------------------------------------------------------------
void mme_app_get_pdn_context (ue_context_t * const ue_context, pdn_cid_t const context_id, ebi_t const default_ebi, bstring const apn_subscribed, pdn_context_t **pdn_ctx)
{
  /** Checking for valid fields inside the search comparision function. */
  pdn_context_t pdn_context_key = {.apn_subscribed = apn_subscribed, .default_ebi = default_ebi, .context_identifier = context_id};
  pdn_context_t * pdn_ctx_p = RB_FIND(PdnContexts, &ue_context->pdn_contexts, &pdn_context_key);
  *pdn_ctx = pdn_ctx_p;
}

//------------------------------------------------------------------------------
static void mme_app_pdn_context_init(ue_context_t * const ue_context, pdn_context_t *const  pdn_context)
{
  if ((pdn_context) && (ue_context)) {
    memset(pdn_context, 0, sizeof(*pdn_context));
    pdn_context->is_active = false;
    /** Initialize the session bearers map. */
    RB_INIT(&pdn_context->session_bearers);
  }
}

/*
 * Method to generate bearer contexts to be created.
 */
//------------------------------------------------------------------------------
void mme_app_get_bearer_contexts_to_be_created(pdn_context_t * pdn_context, bearer_contexts_to_be_created_t *bc_tbc, mme_app_bearer_state_t bc_state){

  OAILOG_FUNC_IN (LOG_MME_APP);

  bearer_context_t * bearer_context_to_setup  = NULL;

  int num_bc = bc_tbc->num_bearer_context;
  RB_FOREACH (bearer_context_to_setup, SessionBearers, &pdn_context->session_bearers) {
    DevAssert(bearer_context_to_setup);
    // todo: make it selective for multi PDN!
    /*
     * Set the bearer of the pdn context to establish.
     * Set regardless of GBR/NON-GBR QCI the MBR/GBR values.
     * They are already set to zero if non-gbr in the registration of the bearer contexts.
     */
    /** EBI. */
    bc_tbc->bearer_contexts[num_bc].eps_bearer_id              = bearer_context_to_setup->ebi;
    /** MBR/GBR values (setting indep. of QCI level). */
    bc_tbc->bearer_contexts[num_bc].bearer_level_qos.gbr.br_ul = bearer_context_to_setup->esm_ebr_context.gbr_ul;
    bc_tbc->bearer_contexts[num_bc].bearer_level_qos.gbr.br_dl = bearer_context_to_setup->esm_ebr_context.gbr_dl;
    bc_tbc->bearer_contexts[num_bc].bearer_level_qos.mbr.br_ul = bearer_context_to_setup->esm_ebr_context.mbr_ul;
    bc_tbc->bearer_contexts[num_bc].bearer_level_qos.mbr.br_dl = bearer_context_to_setup->esm_ebr_context.mbr_dl;
    /** QCI. */
    bc_tbc->bearer_contexts[num_bc].bearer_level_qos.qci       = bearer_context_to_setup->qci;
    bc_tbc->bearer_contexts[num_bc].bearer_level_qos.pvi       = bearer_context_to_setup->preemption_vulnerability;
    bc_tbc->bearer_contexts[num_bc].bearer_level_qos.pci       = bearer_context_to_setup->preemption_capability;
    bc_tbc->bearer_contexts[num_bc].bearer_level_qos.pl        = bearer_context_to_setup->priority_level;
    /** Set the S1U SAE-GW FTEID. */
    bc_tbc->bearer_contexts[num_bc].s1u_sgw_fteid.ipv4                = bearer_context_to_setup->s_gw_fteid_s1u.ipv4;
    bc_tbc->bearer_contexts[num_bc].s1u_sgw_fteid.interface_type      = bearer_context_to_setup->s_gw_fteid_s1u.interface_type;
    bc_tbc->bearer_contexts[num_bc].s1u_sgw_fteid.ipv4_address.s_addr = bearer_context_to_setup->s_gw_fteid_s1u.ipv4_address.s_addr;
    bc_tbc->bearer_contexts[num_bc].s1u_sgw_fteid.teid                = bearer_context_to_setup->s_gw_fteid_s1u.teid;
    // todo: ipv6, other interfaces!


    bc_tbc->num_bearer_context++;
    /*
     * Update the bearer state.
     * Setting the bearer state as MME_CREATED when successful CSResp is received and as ACTIVE after successful MBResp is received!!
     */
    if(bc_state != BEARER_STATE_NULL){
      bearer_context_to_setup->bearer_state   |= bc_state;
    }
    // todo: set TFTs for dedicated bearers (currently PCRF triggered).
  }
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
pdn_context_t *  mme_app_create_pdn_context(ue_context_t * const ue_context, const bstring apn, const context_identifier_t context_identifier)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  pdn_context_t * pdn_context = calloc(1, sizeof(*pdn_context));

  if (!pdn_context) {
    OAILOG_CRITICAL(LOG_MME_APP, "Error creating PDN context for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, NULL);
  }
  /*
   * Check if an APN configuration exists. If so, use it to update the fields.
   * todo: how to update the fields if ULA received after CSR/pdn_context creation (Handover/idle_tau).
   */
  struct apn_configuration_s *apn_configuration = mme_app_get_apn_config(ue_context, context_identifier);
  if (apn_configuration) {
    mme_app_pdn_context_init(ue_context, pdn_context);

    pdn_context->default_ebi            = 5 + ue_context->next_def_ebi_offset;
    ue_context->next_def_ebi_offset++;

    pdn_context->context_identifier     = apn_configuration->context_identifier;
    pdn_context->pdn_type               = apn_configuration->pdn_type;
    if (apn_configuration->nb_ip_address) {
      pdn_context->paa->pdn_type           = apn_configuration->ip_address[0].pdn_type;// TODO check this later...
      pdn_context->paa->ipv4_address       = apn_configuration->ip_address[0].address.ipv4_address;
      if (2 == apn_configuration->nb_ip_address) {
        pdn_context->paa->ipv6_address       = apn_configuration->ip_address[1].address.ipv6_address;
        pdn_context->paa->ipv6_prefix_length = 64;
      }
    }
    // todo: add the apn as bstring into the pdn_context and store it in the map
    //pdn_context->apn_oi_replacement
    memcpy (&pdn_context->default_bearer_eps_subscribed_qos_profile, &apn_configuration->subscribed_qos, sizeof(eps_subscribed_qos_profile_t));
    pdn_context->apn_subscribed = blk2bstr(apn_configuration->service_selection, apn_configuration->service_selection_length);  /**< Set the APN-NI from the service selection. */
  } else {
    pdn_context->apn_subscribed = apn;
    pdn_context->context_identifier = context_identifier;
    OAILOG_WARNING(LOG_MME_APP, "No APN configuration exists for UE " MME_UE_S1AP_ID_FMT ". Assuming Handover/TAU UE: " MME_UE_S1AP_ID_FMT ". Subscribed apn %s. \n",
        ue_context->mme_ue_s1ap_id, bdata(pdn_context->apn_subscribed));
    // todo: check that a procedure exists?
    /** The subscribed APN should already exist from handover (forward relocation request). */

  }
  pdn_context->is_active = true;
//  pdn_context->default_ebi = EPS_BEARER_IDENTITY_UNASSIGNED;
  // TODO pdn_context->apn_in_use     =
  /** Insert the PDN context into the map of PDN contexts. */
  DevAssert(!RB_INSERT (PdnContexts, &ue_context->pdn_contexts, pdn_context));
  MSC_LOG_EVENT (MSC_NAS_ESM_MME, "0 Create PDN cid %u APN %s", pdn_context->context_identifier, pdn_context->apn_in_use);
  OAILOG_FUNC_RETURN (LOG_MME_APP, pdn_context);
}


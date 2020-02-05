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

/*! \file 3gpp_24.008_sm_ies.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>

#include "bstrlib.h"

#include "3gpp_24.008.h"
#include "3gpp_24.301.h"
#include "3gpp_29.274.h"
#include "assertions.h"
#include "common_defs.h"
#include "dynamic_memory_check.h"
#include "log.h"

//------------------------------------------------------------------------------
void free_bearer_contexts_to_be_created(
    bearer_contexts_to_be_created_t **bcs_tbc) {
  bearer_contexts_to_be_created_t *bcstbc = *bcs_tbc;
  // nothing to do for packet filters
  for (int i = 0; i < bcstbc->num_bearer_context; i++) {
    if (bcstbc->bearer_context[i].tft)
      free_traffic_flow_template(&bcstbc->bearer_context[i].tft);
  }
  free_wrapper((void **)bcs_tbc);
}

//------------------------------------------------------------------------------
void free_bearer_contexts_to_be_updated(
    bearer_contexts_to_be_updated_t **bcs_tbu) {
  bearer_contexts_to_be_updated_t *bcstbu = *bcs_tbu;
  // nothing to do for packet filters
  for (int i = 0; i < bcstbu->num_bearer_context; i++) {
    /** Destroy the bearer level qos. */
    if (bcstbu->bearer_context[i].bearer_level_qos)
      free_wrapper((void **)&bcstbu->bearer_context[i].bearer_level_qos);
    if (bcstbu->bearer_context[i].tft)
      free_traffic_flow_template(&bcstbu->bearer_context[i].tft);
  }
  free_wrapper((void **)bcs_tbu);
}

//------------------------------------------------------------------------------
void free_bearer_contexts_to_be_deleted(
    bearer_contexts_to_be_removed_t **bcs_tbr) {
  bearer_contexts_to_be_removed_t *bctbr = *bcs_tbr;
  // nothing to do for packet filters
  free_wrapper((void **)bcs_tbr);
}

//------------------------------------------------------------------------------
// 7.3.1 PDN Connections IE
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
static void free_pdn_connection(pdn_connection_t *pdn_connection) {
  /** APN String. */
  bdestroy_wrapper(&pdn_connection->apn_str);

  /** Bearer Contexts to be Created. */
  for (int i = 0; i < pdn_connection->bearer_context_list.num_bearer_context;
       i++) {
    if (pdn_connection->bearer_context_list.bearer_context[i].tft)
      free_traffic_flow_template(
          &pdn_connection->bearer_context_list.bearer_context[i].tft);
  }
}

//------------------------------------------------------------------------------
void free_mme_ue_eps_pdn_connections(
    mme_ue_eps_pdn_connections_t **pdn_connections) {
  mme_ue_eps_pdn_connections_t *pdnconnections = *pdn_connections;
  // nothing to do for packet filters
  for (int i = 0; i < pdnconnections->num_pdn_connections; i++) {
    free_pdn_connection(&pdnconnections->pdn_connection[i]);
  }
  free_wrapper((void **)pdn_connections);
}

//------------------------------------------------------------------------------
// 8.38 MM Context IE
//------------------------------------------------------------------------------

void free_mm_context_eps(mm_context_eps_t **ue_eps_mm_context) {
  mm_context_eps_t *ueepsmmcontext = *ue_eps_mm_context;

  for (int i = 0; i < ueepsmmcontext->num_quad; i++) {
    if (ueepsmmcontext->auth_quadruplet[i])
      free_wrapper((void **)&ueepsmmcontext->auth_quadruplet[i]);
  }

  for (int i = 0; i < ueepsmmcontext->num_quit; i++) {
    if (ueepsmmcontext->auth_quintuplet[i])
      free_wrapper((void **)&ueepsmmcontext->auth_quintuplet[i]);
  }

  free_wrapper((void **)ue_eps_mm_context);
}

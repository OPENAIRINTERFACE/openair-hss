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



/* This file contains definitions related to mme applicative layer and should
 * not be included within other layers.
 * Use mme_app_extern.h to expose mme applicative layer procedures/data.
 */

#include "intertask_interface.h"
#include "mme_app_ue_context.h"

#ifndef MME_APP_DEFS_H_
#define MME_APP_DEFS_H_

#ifndef MME_APP_DEBUG
# define MME_APP_DEBUG(x, args...) do { fprintf(stdout, "[MMEA][D]"x, ##args); } while(0)
#endif
#ifndef MME_APP_ERROR
# define MME_APP_ERROR(x, args...) do { fprintf(stdout, "[MMEA][E]"x, ##args); } while(0)
#endif

typedef struct {
  /* UE contexts + some statistics variables */
  mme_ue_context_t mme_ue_contexts;

  long statistic_timer_id;
  uint32_t statistic_timer_period;
} mme_app_desc_t;

extern mme_app_desc_t mme_app_desc;


#if defined(DISABLE_USE_NAS)
int mme_app_handle_attach_req(nas_attach_req_t *attach_req_p);
#endif

int mme_app_handle_s1ap_ue_capabilities_ind  (const s1ap_ue_cap_ind_t const * s1ap_ue_cap_ind_pP);

int mme_app_send_s11_create_session_req      (struct ue_context_s * const ue_context_pP);

int mme_app_send_s6a_update_location_req     (struct ue_context_s * const ue_context_pP);

int mme_app_handle_s6a_update_location_ans   (const s6a_update_location_ans_t * const ula_pP);

int mme_app_handle_nas_pdn_connectivity_req  ( nas_pdn_connectivity_req_t * const nas_pdn_connectivity_req_p);

void mme_app_handle_conn_est_cnf             (const nas_conn_est_cnf_t * const nas_conn_est_cnf_pP);

void mme_app_handle_conn_est_ind             (const mme_app_connection_establishment_ind_t * const conn_est_ind_pP);

int mme_app_handle_create_sess_resp          (const SgwCreateSessionResponse * const create_sess_resp_pP);

int mme_app_handle_establish_ind             (const nas_establish_ind_t * const nas_establish_ind_pP);

int mme_app_handle_authentication_info_answer(const s6a_auth_info_ans_t * const s6a_auth_info_ans_pP);

int mme_app_handle_nas_auth_resp             (const nas_auth_resp_t * const nas_auth_resp_pP);

nas_cause_t s6a_error_2_nas_cause            (const uint32_t s6a_errorP, const int experimentalP);

void mme_app_handle_nas_auth_param_req       (const nas_auth_param_req_t * const nas_auth_param_req_pP);

void mme_app_handle_initial_context_setup_rsp(const mme_app_initial_context_setup_rsp_t * const initial_ctxt_setup_rsp_pP);

#endif /* MME_APP_DEFS_H_ */

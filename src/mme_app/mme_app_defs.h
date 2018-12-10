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

/*! \file mme_app_defs.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

/* This file contains definitions related to mme applicative layer and should
 * not be included within other layers.
 * Use mme_app_extern.h to expose mme applicative layer procedures/data.
 */


#ifndef FILE_MME_APP_DEFS_SEEN
#define FILE_MME_APP_DEFS_SEEN
#include "intertask_interface.h"
#include "mme_app_ue_context.h"

typedef struct mme_app_desc_s {
  /* UE contexts + some statistics variables */
  mme_ue_context_t mme_ue_contexts;

  long statistic_timer_id;
  uint32_t statistic_timer_period;


  uint32_t mme_mobility_management_timer_period;

  /* Reader/writer lock */
  pthread_rwlock_t rw_lock;


  /* ***************Statistics*************
   * number of attached UE,number of connected UE,
   * number of idle UE,number of default bearers,
   * number of S1_U bearers,number of PDN sessions
   */

  uint32_t               nb_enb_connected;
  uint32_t               nb_ue_attached;
  uint32_t               nb_ue_connected;
  uint32_t               nb_default_eps_bearers;
  uint32_t               nb_s1u_bearers;

  /* ***************Changes in Statistics**************/

  uint32_t               nb_ue_attached_since_last_stat;
  uint32_t               nb_ue_detached_since_last_stat;
  uint32_t               nb_ue_connected_since_last_stat;
  uint32_t               nb_ue_disconnected_since_last_stat;
  uint32_t               nb_eps_bearers_established_since_last_stat;
  uint32_t               nb_eps_bearers_released_since_last_stat;
  uint32_t               nb_enb_connected_since_last_stat;
  uint32_t               nb_enb_released_since_last_stat;
  uint32_t               nb_s1u_bearers_released_since_last_stat;
  uint32_t               nb_s1u_bearers_established_since_last_stat;
} mme_app_desc_t;

extern mme_app_desc_t mme_app_desc;

void mme_app_handle_s1ap_enb_deregistered_ind (const itti_s1ap_eNB_deregistered_ind_t * const enb_dereg_ind);

int mme_app_handle_s1ap_ue_capabilities_ind  (const itti_s1ap_ue_cap_ind_t * const s1ap_ue_cap_ind_pP);

void mme_app_handle_s1ap_ue_context_release_complete (const itti_s1ap_ue_context_release_complete_t * const s1ap_ue_context_release_complete);

void mme_app_itti_ue_context_release (    mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id, enum s1cause cause, uint32_t target_enb_id);

void mme_app_itti_notify_request(const imsi64_t imsi, const plmn_t * handovered_plmn, const bool mobility_completion);

// todo: put back in consts
int mme_app_handle_s6a_update_location_ans   (s6a_update_location_ans_t * ula_pP);

int mme_app_handle_nas_pdn_disconnect_req    ( itti_nas_pdn_disconnect_req_t * const nas_pdn_disconnect_req_pP);

void mme_app_handle_detach_req (const itti_nas_detach_req_t * const detach_req_p);

void mme_app_handle_conn_est_cnf             (itti_nas_conn_est_cnf_t * const nas_conn_est_cnf_pP);

void mme_app_handle_initial_ue_message       (itti_s1ap_initial_ue_message_t * const conn_est_ind_pP);

int mme_app_handle_create_sess_resp          (itti_s11_create_session_response_t * const create_sess_resp_pP); //not const because we need to free internal stucts

void mme_app_handle_nas_erab_setup_req (itti_nas_erab_setup_req_t * const itti_nas_erab_setup_req);

void mme_app_handle_nas_erab_modify_req (itti_nas_erab_modify_req_t * const itti_nas_erab_modify_setup_req);

void mme_app_handle_nas_erab_release_req (itti_nas_erab_release_req_t * const itti_nas_erab_release_req);

void mme_app_handle_delete_session_rsp	     (const itti_s11_delete_session_response_t * const delete_sess_respP);

int mme_app_handle_establish_ind             (const nas_establish_ind_t * const nas_establish_ind_pP);

int mme_app_handle_authentication_info_answer(const s6a_auth_info_ans_t * const s6a_auth_info_ans_pP);

void  mme_app_handle_release_access_bearers_resp (const itti_s11_release_access_bearers_response_t * const rel_access_bearers_rsp_pP);

void mme_app_handle_s11_create_bearer_req        (itti_s11_create_bearer_request_t * create_bearer_request_pP);

void mme_app_handle_s11_update_bearer_req        (itti_s11_update_bearer_request_t * update_bearer_request_pP);

void mme_app_handle_s11_delete_bearer_req (    itti_s11_delete_bearer_request_t *  delete_bearer_request_pP);

void mme_app_bearer_create_workflow (ue_context_t * const ue_context, const pdn_cid_t cid);

//nas_cause_t s6a_error_2_nas_cause            (const uint32_t s6a_errorP, const int experimentalP);

void mme_app_handle_nas_auth_param_req       (const itti_nas_auth_param_req_t * const nas_auth_param_req_pP);

void mme_app_handle_initial_context_setup_rsp(itti_mme_app_initial_context_setup_rsp_t * const initial_ctxt_setup_rsp_pP);

bool mme_app_dump_ue_context (const hash_key_t keyP, void *const ue_context, void *unused_param_pP, void **unused_result_pP);

int mme_app_handle_nas_dl_req ( itti_nas_dl_data_req_t *const nas_dl_req_pP);

void mme_app_handle_e_rab_setup_rsp (itti_s1ap_e_rab_setup_rsp_t  * const e_rab_setup_rsp);

void mme_app_handle_e_rab_modify_rsp (itti_s1ap_e_rab_modify_rsp_t  * const e_rab_modify_rsp);

void mme_app_handle_activate_eps_bearer_ctx_cnf (itti_mme_app_activate_eps_bearer_ctx_cnf_t   * const activate_eps_bearer_ctx_cnf);

void mme_app_handle_activate_eps_bearer_ctx_rej (itti_mme_app_activate_eps_bearer_ctx_rej_t   * const activate_eps_bearer_ctx_rej);

void mme_app_handle_modify_eps_bearer_ctx_cnf (itti_mme_app_modify_eps_bearer_ctx_cnf_t   * const modify_eps_bearer_ctx_cnf);

void mme_app_handle_modify_eps_bearer_ctx_rej (itti_mme_app_modify_eps_bearer_ctx_rej_t   * const modify_eps_bearer_ctx_rej);

void mme_app_handle_deactivate_eps_bearer_ctx_cnf (itti_mme_app_deactivate_eps_bearer_ctx_cnf_t   * const deactivate_bearer_cnf);

void mme_app_trigger_mme_initiated_dedicated_bearer_deactivation_procedure (ue_context_t * const ue_context, const pdn_cid_t cid);


/** S1AP Handover messaging. */
void mme_app_handle_s1ap_handover_required( itti_s1ap_handover_required_t * handover_required_pP );

void mme_app_handle_handover_cancel( const itti_s1ap_handover_cancel_t * const handover_cancel_pP );

/** Handling S10 Messages. */
void mme_app_handle_forward_relocation_request( itti_s10_forward_relocation_request_t * const forward_relocation_request_pP );

void mme_app_handle_forward_relocation_response(  itti_s10_forward_relocation_response_t* const forward_relocation_response_pP );

void mme_app_handle_forward_access_context_notification( itti_s10_forward_access_context_notification_t * const forward_access_context_notification_pP );

void mme_app_handle_forward_access_context_acknowledge( const itti_s10_forward_access_context_acknowledge_t* const forward_access_context_acknowledge_pP );

void mme_app_handle_handover_request_acknowledge(itti_s1ap_handover_request_acknowledge_t * const handover_request_acknowledge_pP    );

void mme_app_handle_handover_failure(const itti_s1ap_handover_failure_t * const handover_failure_pP    );

void mme_app_s1ap_error_indication(const itti_s1ap_error_indication_t * const s1ap_error_indication_pP    );

void mme_app_handle_enb_status_transfer(itti_s1ap_status_transfer_t* const s1ap_status_transfer_pP    );

void mme_app_handle_forward_relocation_complete_notification(const itti_s10_forward_relocation_complete_notification_t* const forward_relocation_complete_notification_pP    );

void mme_app_handle_forward_relocation_complete_acknowledge(const itti_s10_forward_relocation_complete_acknowledge_t* const forward_relocation_complete_acknowledge_pP    );

/** Relocation Cancel Request & Response. */
void
mme_app_handle_relocation_cancel_request(
     const itti_s10_relocation_cancel_request_t * const relocation_cancel_request_pP
    );

void
mme_app_handle_relocation_cancel_response(
     const itti_s10_relocation_cancel_response_t * const relocation_cancel_response_pP
    );

/** TAU Related Messaging. */
void mme_app_handle_nas_context_req(itti_nas_context_req_t * const nas_context_req_pP);

void mme_app_handle_s10_context_request( const itti_s10_context_request_t * const context_request_pP );

void mme_app_handle_s10_context_response( itti_s10_context_response_t * const context_response_pP );

void mme_app_handle_s10_context_acknowledge( const itti_s10_context_acknowledge_t * const context_acknowledge_pP );


/** Paging Functions. */

void mme_app_handle_downlink_data_notification (const itti_s11_downlink_data_notification_t * const saegw_dl_data_ntf_pP);

#define mme_stats_read_lock(mMEsTATS)  pthread_rwlock_rdlock(&(mMEsTATS)->rw_lock)
#define mme_stats_write_lock(mMEsTATS) pthread_rwlock_wrlock(&(mMEsTATS)->rw_lock)
#define mme_stats_unlock(mMEsTATS)     pthread_rwlock_unlock(&(mMEsTATS)->rw_lock)

#endif /* MME_APP_DEFS_H_ */

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

//WARNING: Do not include this header directly. Use intertask_interface.h instead.

/*! \file nas_messages_def.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

/** EMM to ESM messages. */
MESSAGE_DEF(NAS_ESM_DATA_IND,                   MESSAGE_PRIORITY_MED,   itti_nas_esm_data_ind_t,         nas_esm_data_ind)
MESSAGE_DEF(NAS_ESM_DETACH_IND,                 MESSAGE_PRIORITY_MED,   itti_nas_esm_detach_ind_t,       nas_esm_detach_ind)

MESSAGE_DEF(NAS_INITIAL_UE_MESSAGE,             MESSAGE_PRIORITY_MED,   itti_nas_initial_ue_message_t,   nas_initial_ue_message)
MESSAGE_DEF(NAS_CONNECTION_ESTABLISHMENT_CNF,   MESSAGE_PRIORITY_MED,   itti_nas_conn_est_cnf_t,         nas_conn_est_cnf)
MESSAGE_DEF(NAS_UPLINK_DATA_IND,                MESSAGE_PRIORITY_MED,   itti_nas_ul_data_ind_t,          nas_ul_data_ind)
MESSAGE_DEF(NAS_DOWNLINK_DATA_REQ,              MESSAGE_PRIORITY_MED,   itti_nas_dl_data_req_t,          nas_dl_data_req)
MESSAGE_DEF(NAS_DL_DATA_CNF,                    MESSAGE_PRIORITY_MED,   itti_nas_dl_data_cnf_t,          nas_dl_data_cnf)
MESSAGE_DEF(NAS_DL_DATA_REJ,                    MESSAGE_PRIORITY_MED,   itti_nas_dl_data_rej_t,          nas_dl_data_rej)
MESSAGE_DEF(NAS_ERAB_SETUP_REQ,                 MESSAGE_PRIORITY_MED,   itti_nas_erab_setup_req_t,       nas_erab_setup_req)
MESSAGE_DEF(NAS_ERAB_MODIFY_REQ,                MESSAGE_PRIORITY_MED,   itti_nas_erab_modify_req_t,      nas_erab_modify_req)
MESSAGE_DEF(NAS_ERAB_RELEASE_REQ,               MESSAGE_PRIORITY_MED,   itti_nas_erab_release_req_t,     nas_erab_release_req)

/* NAS layer -> MME app messages */
MESSAGE_DEF(NAS_DETACH_REQ,       	          	MESSAGE_PRIORITY_MED,   itti_nas_detach_req_t,           	nas_detach_req)

/* MME app -> NAS layer messages */
MESSAGE_DEF(NAS_PDN_CONNECTIVITY_RSP,           MESSAGE_PRIORITY_MED,   itti_nas_pdn_connectivity_rsp_t,  nas_pdn_connectivity_rsp)
MESSAGE_DEF(NAS_PDN_CONFIG_RSP,                 MESSAGE_PRIORITY_MED,   itti_nas_pdn_config_rsp_t,       nas_pdn_config_rsp)
MESSAGE_DEF(NAS_PDN_CONFIG_FAIL,                MESSAGE_PRIORITY_MED,   itti_nas_pdn_config_fail_t,      nas_pdn_config_fail)
MESSAGE_DEF(NAS_SIGNALLING_CONNECTION_REL_IND,  MESSAGE_PRIORITY_MED,   itti_nas_signalling_connection_rel_ind_t, nas_signalling_connection_rel_ind)
MESSAGE_DEF(NAS_PAGING_DUE_SIGNALING_IND     ,  MESSAGE_PRIORITY_MED,   itti_nas_paging_due_signaling_ind_t 	,  nas_paging_due_signaling_ind)

/** Remote UE Report Response. */
MESSAGE_DEF(NAS_REMOTE_UE_REPORT_RSP,           MESSAGE_PRIORITY_MED,   itti_nas_remote_ue_report_response_rsp_t,   nas_remote_ue_report_response_rsp)

/** Dedicated Bearer Messaging. */
MESSAGE_DEF(NAS_RETRY_BEARER_CTX_PROC_IND     , MESSAGE_PRIORITY_MED, itti_nas_retry_bearer_ctx_proc_ind_t     ,  nas_retry_bearer_ctx_proc_ind)
MESSAGE_DEF(NAS_ACTIVATE_EPS_BEARER_CTX_REQ   , MESSAGE_PRIORITY_MED, itti_nas_activate_eps_bearer_ctx_req_t   ,  nas_activate_eps_bearer_ctx_req)
MESSAGE_DEF(NAS_ACTIVATE_EPS_BEARER_CTX_CNF   , MESSAGE_PRIORITY_MED, itti_nas_activate_eps_bearer_ctx_cnf_t   ,  nas_activate_eps_bearer_ctx_cnf)
MESSAGE_DEF(NAS_ACTIVATE_EPS_BEARER_CTX_REJ   , MESSAGE_PRIORITY_MED, itti_nas_activate_eps_bearer_ctx_rej_t   ,  nas_activate_eps_bearer_ctx_rej)
MESSAGE_DEF(NAS_MODIFY_EPS_BEARER_CTX_REQ     , MESSAGE_PRIORITY_MED, itti_nas_modify_eps_bearer_ctx_req_t     ,  nas_modify_eps_bearer_ctx_req)
MESSAGE_DEF(NAS_MODIFY_EPS_BEARER_CTX_CNF     , MESSAGE_PRIORITY_MED, itti_nas_modify_eps_bearer_ctx_cnf_t     ,  nas_modify_eps_bearer_ctx_cnf)
MESSAGE_DEF(NAS_MODIFY_EPS_BEARER_CTX_REJ     , MESSAGE_PRIORITY_MED, itti_nas_modify_eps_bearer_ctx_rej_t     ,  nas_modify_eps_bearer_ctx_rej)
MESSAGE_DEF(NAS_DEACTIVATE_EPS_BEARER_CTX_REQ , MESSAGE_PRIORITY_MED, itti_nas_deactivate_eps_bearer_ctx_req_t ,  nas_deactivate_eps_bearer_ctx_req)
MESSAGE_DEF(NAS_DEACTIVATE_EPS_BEARER_CTX_CNF , MESSAGE_PRIORITY_MED, itti_nas_deactivate_eps_bearer_ctx_cnf_t ,  nas_deactivate_eps_bearer_ctx_cnf)

/** todo: for multi-pdn. */
MESSAGE_DEF(NAS_PDN_DISCONNECT_REQ,             MESSAGE_PRIORITY_MED,   itti_nas_pdn_disconnect_req_t,   nas_pdn_disconnect_req)
MESSAGE_DEF(NAS_PDN_DISCONNECT_RSP,             MESSAGE_PRIORITY_MED,   itti_nas_pdn_disconnect_rsp_t,   nas_pdn_disconnect_rsp)


/** S10 Context Transfer (TAU). */
MESSAGE_DEF(NAS_CONTEXT_REQ,                    MESSAGE_PRIORITY_MED,   itti_nas_context_req_t,         nas_context_req)
MESSAGE_DEF(NAS_CONTEXT_RES,                    MESSAGE_PRIORITY_MED,   itti_nas_context_res_t,         nas_context_res)
MESSAGE_DEF(NAS_CONTEXT_FAIL,                   MESSAGE_PRIORITY_MED,   itti_nas_context_fail_t,        nas_context_fail)

MESSAGE_DEF(NAS_IMPLICIT_DETACH_UE_IND,         MESSAGE_PRIORITY_MED,   itti_nas_implicit_detach_ue_ind_t, nas_implicit_detach_ue_ind)



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

/*! \file nas_itti_messaging.h
   \brief
   \author  Sebastien ROUX, Lionel GAUTHIER
   \date
   \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_NAS_ITTI_MESSAGING_SEEN
#define FILE_NAS_ITTI_MESSAGING_SEEN

#include "nas_message.h"
#include "as_message.h"
#include "esm_proc.h"
// todo: find a better place for this
#include "mme_app_esm_procedures.h"
#include "intertask_interface_types.h"

int
nas_itti_esm_data_ind(
  const mme_ue_s1ap_id_t  ue_id,
  bstring                 esm_msg_p,
  imsi_t                 *imsi,
  tai_t                  *visited_tai);

int
nas_itti_esm_detach_ind(
  const mme_ue_s1ap_id_t  ue_id,
  const bool clr);

int
nas_itti_s11_retry_ind(
  const mme_ue_s1ap_id_t  ue_id);

int nas_itti_dl_data_req(
  const mme_ue_s1ap_id_t ue_idP,
  bstring                nas_msgP,
  nas_error_code_t transaction_status);

int
nas_itti_erab_setup_req (
    const mme_ue_s1ap_id_t ue_id,
    const ebi_t            ebi,
    const bitrate_t        mbr_dl,
    const bitrate_t        mbr_ul,
    const bitrate_t        gbr_dl,
    const bitrate_t        gbr_ul,
    bstring                nas_msg);

int
nas_itti_erab_modify_req (
    const mme_ue_s1ap_id_t ue_id,
    const ebi_t            ebi,
    const bitrate_t        mbr_dl,
    const bitrate_t        mbr_ul,
    const bitrate_t        gbr_dl,
    const bitrate_t        gbr_ul,
    bstring                nas_msg);

int
nas_itti_erab_release_req (const mme_ue_s1ap_id_t ue_id,
    const ebi_t ebi,
    bstring                nas_msg);

void nas_itti_pdn_config_req(
  unsigned int            ue_idP,
  const imsi_t           *const imsi_pP,
  esm_proc_pdn_request_t  request_type,
  plmn_t                 *visited_plmn);

void nas_itti_pdn_disconnect_req(
  mme_ue_s1ap_id_t        ue_idP,
  ebi_t                   default_ebi,
  pti_t                   pti,
  bool                    deleteTunnel,
  bool                    handover,
  struct in_addr          saegw_s11_addr, /**< Put them into the UE context ? */
  teid_t                  saegw_s11_teid,
  pdn_cid_t               pdn_cid);

void nas_itti_ctx_req(
  const uint32_t        ue_idP,
  const guti_t        * const guti_p,
  tai_t         * const new_taiP,
  tai_t         * const last_visited_taiP,
  bstring               request_msg);

void nas_itti_auth_info_req(
  const mme_ue_s1ap_id_t ue_idP,
  const imsi_t   * const imsiP,
  const bool             is_initial_reqP,
  plmn_t         * const visited_plmnP,
  const uint8_t          num_vectorsP,
  const_bstring    const auts_pP);

void nas_itti_s11_bearer_resource_cmd (
  const pti_t            pti,
  const ebi_t            linked_ebi,
  const teid_t           local_teid,
  const teid_t           peer_teid,
  const struct in_addr  *saegw_s11_ipv4,
  const ebi_t                    ebi,
  const traffic_flow_template_t * const tad,
  const flow_qos_t              * const flow_qos);

void nas_itti_establish_cnf(
  const mme_ue_s1ap_id_t ue_idP,
  const nas_error_code_t error_codeP,
  bstring                msgP,
  const uint32_t         nas_count,
  const uint16_t         selected_encryption_algorithmP,
  const uint16_t         selected_integrity_algorithmP);

void nas_itti_detach_req(
  const mme_ue_s1ap_id_t      ue_idP);

void nas_itti_activate_eps_bearer_ctx_cnf(
    const mme_ue_s1ap_id_t ue_idP,
    const ebi_t            ebi,
    const teid_t           saegw_s1u_teid);

void nas_itti_activate_eps_bearer_ctx_rej(
    const mme_ue_s1ap_id_t ue_idP,
    const teid_t           saegw_s1u_teid,
    const esm_cause_t      esm_cause);

void nas_itti_modify_eps_bearer_ctx_cnf(
    const mme_ue_s1ap_id_t ue_idP,
    const ebi_t            ebi);

void nas_itti_modify_eps_bearer_ctx_rej(
    const mme_ue_s1ap_id_t ue_idP,
    const ebi_t            ebi,
    const esm_cause_t      esm_cause);

void nas_itti_dedicated_eps_bearer_deactivation_complete(
    const mme_ue_s1ap_id_t ue_idP,
    const ebi_t ded_ebi);

void  s6a_auth_info_rsp_timer_expiry_handler (void *args);

void  s10_context_req_timer_expiry_handler (void *args);

#endif /* FILE_NAS_ITTI_MESSAGING_SEEN */

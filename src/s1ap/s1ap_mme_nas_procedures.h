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

/*! \file s1ap_mme_nas_procedures.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_S1AP_MME_NAS_PROCEDURES_SEEN
#define FILE_S1AP_MME_NAS_PROCEDURES_SEEN

#include "common_defs.h"

/** \brief Handle an Initial UE message.
 * \param assocId lower layer assoc id (SCTP)
 * \param stream SCTP stream on which data had been received
 * \param message The message as decoded by the ASN.1 codec
 * @returns -1 on failure, 0 otherwise
 **/
int s1ap_mme_handle_initial_ue_message(const sctp_assoc_id_t assocId,
                                       const sctp_stream_id_t stream,
                                       S1AP_S1AP_PDU_t* message);

/** \brief Handle an Uplink NAS transport message.
 * Process the RRC transparent container and forward it to NAS entity.
 * \param assocId lower layer assoc id (SCTP)
 * \param stream SCTP stream on which data had been received
 * \param message The message as decoded by the ASN.1 codec
 * @returns -1 on failure, 0 otherwise
 **/
int s1ap_mme_handle_uplink_nas_transport(const sctp_assoc_id_t assocId,
                                         const sctp_stream_id_t stream,
                                         S1AP_S1AP_PDU_t* message);

/** \brief Handle a NAS non delivery indication message from eNB
 * \param assocId lower layer assoc id (SCTP)
 * \param stream SCTP stream on which data had been received
 * \param message The message as decoded by the ASN.1 codec
 * @returns -1 on failure, 0 otherwise
 **/
int s1ap_mme_handle_nas_non_delivery(const sctp_assoc_id_t assocId,
                                     const sctp_stream_id_t stream,
                                     S1AP_S1AP_PDU_t* message);

void s1ap_handle_conn_est_cnf(
    const itti_mme_app_connection_establishment_cnf_t* const conn_est_cnf_p);

int s1ap_generate_downlink_nas_transport(const enb_ue_s1ap_id_t enb_ue_s1ap_id,
                                         const mme_ue_s1ap_id_t ue_id,
                                         const uint32_t enb_id,
                                         STOLEN_REF bstring* payload);

int s1ap_generate_s1ap_e_rab_setup_req(
    itti_s1ap_e_rab_setup_req_t* const e_rab_setup_req);

int s1ap_generate_s1ap_e_rab_modify_req(
    itti_s1ap_e_rab_modify_req_t* const e_rab_modify_req);

int s1ap_generate_s1ap_e_rab_release_req(
    itti_s1ap_e_rab_release_req_t* const e_rab_release_req);

/** S1AP Path Switch Request Acknowledge. */
void s1ap_handle_path_switch_req_ack(
    const itti_s1ap_path_switch_request_ack_t* const path_switch_req_ack_pP);

int s1ap_handle_handover_preparation_failure(
    const itti_s1ap_handover_preparation_failure_t* handover_prep_failure_pP);

int s1ap_handle_path_switch_request_failure(
    const itti_s1ap_path_switch_request_failure_t*
        path_switch_request_failure_pP);

int s1ap_handover_preparation_failure(const sctp_assoc_id_t assoc_id,
                                      const mme_ue_s1ap_id_t mme_ue_s1ap_id,
                                      const enb_ue_s1ap_id_t enb_ue_s1ap_id,
                                      const enum s1cause s1cause_val);

void s1ap_handle_handover_cancel_acknowledge(
    const itti_s1ap_handover_cancel_acknowledge_t* const
        handover_cancel_acknowledge_pP);

int s1ap_send_path_switch_request_failure(const sctp_assoc_id_t assoc_id,
                                          const mme_ue_s1ap_id_t mme_ue_s1ap_id,
                                          const enb_ue_s1ap_id_t enb_ue_s1ap_id,
                                          const S1AP_Cause_PR cause_type);

/** S1AP Handover Command. */
void s1ap_handle_handover_command(
    const itti_s1ap_handover_command_t* const handover_command_pP);

/** S1AP MME Status Transfer. */
void s1ap_handle_mme_status_transfer(
    const itti_s1ap_status_transfer_t* const s1ap_status_transfer_pP);

void s1ap_handle_mme_ue_id_notification(
    const itti_mme_app_s1ap_mme_ue_id_notification_t* const notification_p);

/** S1AP Paging. */
void s1ap_handle_paging(const itti_s1ap_paging_t* const s1ap_paging_pP);

void s1ap_mme_configuration_transfer(
    const itti_s1ap_configuration_transfer_t* const
        s1ap_mme_configuration_transfer_pP);

#endif /* FILE_S1AP_MME_NAS_PROCEDURES_SEEN */

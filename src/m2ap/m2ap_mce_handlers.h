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

/*! \file m2ap_mce_handlers.h
  \brief
  \author Dincer Beken
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#ifndef FILE_M2AP_MCE_HANDLERS_SEEN
#define FILE_M2AP_MCE_HANDLERS_SEEN
#include "m2ap_mce.h"
#include "intertask_interface.h"
#define MAX_NUM_PARTIAL_M2_CONN_RESET 256

/** \brief Handle decoded incoming messages from SCTP
 * \param assoc_id SCTP association ID
 * \param stream Stream number
 * \param pdu The message decoded by the ASN1C decoder
 * @returns int
 **/
int m2ap_mce_handle_message(const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
                            M2AP_M2AP_PDU_t *pdu);

//int m2ap_mce_handle_ue_cap_indication(const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
//                                      M2AP_M2AP_PDU_t *message);

/** \brief Handle an M2 Setup request message.
 * Typically add the eNB in the list of served eNB if not present, simply reset
 * eNB association otherwise. M2SetupResponse message is sent in case of success or
 * M2SetupFailure if the MCE cannot accept the configuration received.
 * \param assoc_id SCTP association ID
 * \param stream Stream number
 * \param pdu The message decoded by the ASN1C decoder
 * @returns int
 **/
int m2ap_mce_handle_m2_setup_request(const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
                                     M2AP_M2AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_session_start_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_session_start_failure (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_session_stop_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_session_stop_failure (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_session_update_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_session_update_failure (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_scheduling_information_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m2ap_mce_handle_reset_request (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_service_counting_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_service_counting_failure (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m2ap_mce_handle_error_indication (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu) ;

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_service_counting_result_report (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_overload_notification (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int m2ap_handle_sctp_disconnection(const sctp_assoc_id_t assoc_id, bool reset);

int m2ap_handle_new_association(sctp_new_peer_t *sctp_new_peer_p);

int m2ap_mce_set_cause(M2AP_Cause_t *cause_p, const M2AP_Cause_PR cause_type, const long cause_value);

int m2ap_mce_generate_m2_setup_failure(
    const sctp_assoc_id_t assoc_id, const M2AP_Cause_PR cause_type, const long cause_value,
    const long time_to_wait);

///*** HANDLING EXPIRED TIMERS. */
//void s1ap_mme_handle_ue_context_rel_comp_timer_expiry (void *ue_ref_p);
//

int m3ap_handle_enb_initiated_reset_ack (const itti_m3ap_enb_initiated_reset_ack_t * const enb_reset_ack_p);

int m2ap_mce_handle_error_ind_message (const sctp_assoc_id_t assoc_id,
                                       const sctp_stream_id_t stream, M2AP_M2AP_PDU_t *message);

int m2ap_mce_handle_enb_reset (const sctp_assoc_id_t assoc_id,
                               const sctp_stream_id_t stream, M2AP_M2AP_PDU_t *message);

#endif /* FILE_M2AP_MCE_HANDLERS_SEEN */

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

/*! \file s10_mme_session_manager.h
 * \brief
 * \author Dincer Beken
 * \company Blackned GmbH
 * \email: dbeken@blackned.de
 *
 */

#ifndef FILE_S10_MME_SESSION_MANAGER_SEEN
#define FILE_S10_MME_SESSION_MANAGER_SEEN

/**
 * FORWARD RELOCATION REQUEST
 */

/* @brief Create a new Forward Relocation Request and send it to provided target
 * MME. */
int s10_mme_forward_relocation_request(
    nw_gtpv2c_stack_handle_t* stack_p,
    itti_s10_forward_relocation_request_t* forward_relocation_p);

/* @brief Handle a Forward Relocation Response received from target MME. */
int s10_mme_handle_forward_relocation_response(
    nw_gtpv2c_stack_handle_t* stack_p, nw_gtpv2c_ulp_api_t* pUlpApi);

int s10_mme_forward_access_context_notification(
    nw_gtpv2c_stack_handle_t* stack_p,
    itti_s10_forward_access_context_notification_t*
        forward_access_context_notif_p);

int s10_mme_handle_forward_access_context_notification(
    nw_gtpv2c_stack_handle_t* stack_p, nw_gtpv2c_ulp_api_t* pUlpApi);

int s10_mme_forward_access_context_acknowledge(
    nw_gtpv2c_stack_handle_t* stack_p,
    itti_s10_forward_access_context_acknowledge_t*
        forward_access_context_ack_p);

int s10_mme_handle_forward_access_context_acknowledge(
    nw_gtpv2c_stack_handle_t* stack_p, nw_gtpv2c_ulp_api_t* pUlpApi);

/* @brief Create a new Forward Relocation Response and send it to provided
 * source MME. */
int s10_mme_forward_relocation_response(
    nw_gtpv2c_stack_handle_t* stack_p,
    itti_s10_forward_relocation_response_t* forward_relocation_rsp_p);

/* @brief Handle a Forward Relocation Request received from source MME. */
int s10_mme_handle_forward_relocation_request(nw_gtpv2c_stack_handle_t* stack_p,
                                              nw_gtpv2c_ulp_api_t* pUlpApi);

/**
 * FORWARD ACCESS CONTROL NOTIFICATION.
 */
//
///* @brief Create a new Forward Access Context Notification and send it to
/// provided target MME. */
// int s10_mme_forward_access_context_notification(nw_gtpv2c_stack_handle_t
// *stack_p, itti_s10_forward_access_context_notification_t
// *forward_access_ctx_notif_p);
//
///* @brief Handle a Forward Access Context Acknowledge received from target
/// MME. */
// int s10_mme_handle_forward_access_context_acknowlege
// (nw_gtpv2c_stack_handle_t * stack_p, nw_gtpv2c_ulp_api_t * pUlpApi);
//
///* @brief Create a new Forward Access Context Acknowledge and send it to
/// provided source MME. */
// int s10_mme_forward_access_context_acknowledge(nw_gtpv2c_stack_handle_t
// *stack_p, itti_s10_forward_access_context_acknowledge_t
// *forward_access_ctx_ack_p);
//
///* @brief Handle a Forward Access Context Notification received from source
/// MME. */
// int
// s10_mme_handle_forward_access_context_notification(nw_gtpv2c_stack_handle_t *
// stack_p, nw_gtpv2c_ulp_api_t * pUlpApi);

// todo: indirect data tunneling not implented yet.

/**
 * FORWARD RELOCATION COMPLETE
 */

/* @brief Create a new Forward Relocation Complete Notification and send it to
 * provided target MME. */
int s10_mme_forward_relocation_complete_notification(
    nw_gtpv2c_stack_handle_t* stack_p,
    itti_s10_forward_relocation_complete_notification_t*
        forward_reloc_cmpl_notif_p);

/* @brief Handle a Forward Relocation Complete Notification received from source
 * MME. */
int s10_mme_handle_forward_relocation_complete_notification(
    nw_gtpv2c_stack_handle_t* stack_p, nw_gtpv2c_ulp_api_t* pUlpApi);

/* @brief Handle a Forward Relocation Complete Acknowledge received from target
 * MME. */
int s10_mme_handle_forward_relocation_complete_acknowledge(
    nw_gtpv2c_stack_handle_t* stack_p, nw_gtpv2c_ulp_api_t* pUlpApi);

/* @brief Create a new Forward Relocation Complete Acknowledge and send it to
 * provided source MME. */
int s10_mme_forward_relocation_complete_acknowledge(
    nw_gtpv2c_stack_handle_t* stack_p,
    itti_s10_forward_relocation_complete_acknowledge_t*
        forward_reloc_cmpl_ack_p);

int s10_mme_context_request(nw_gtpv2c_stack_handle_t* stack_p,
                            itti_s10_context_request_t* req_p);

int s10_mme_context_response(nw_gtpv2c_stack_handle_t* stack_p,
                             itti_s10_context_response_t* rsp_p);

int s10_mme_context_acknowledge(nw_gtpv2c_stack_handle_t* stack_p,
                                itti_s10_context_acknowledge_t* ack_p);

/* Handling TAU related S10 messaging. */
int s10_mme_handle_context_request(nw_gtpv2c_stack_handle_t* stack_p,
                                   nw_gtpv2c_ulp_api_t* pUlpApi);

int s10_mme_handle_context_response(nw_gtpv2c_stack_handle_t* stack_p,
                                    nw_gtpv2c_ulp_api_t* pUlpApi);

int s10_mme_handle_context_acknowledgement(nw_gtpv2c_stack_handle_t* stack_p,
                                           nw_gtpv2c_ulp_api_t* pUlpApi);

int s10_mme_relocation_cancel_request(
    nw_gtpv2c_stack_handle_t* stack_p,
    itti_s10_relocation_cancel_request_t* relocation_cancel_request_p);

int s10_mme_handle_relocation_cancel_request(nw_gtpv2c_stack_handle_t* stack_p,
                                             nw_gtpv2c_ulp_api_t* pUlpApi);

int s10_mme_relocation_cancel_response(
    nw_gtpv2c_stack_handle_t* stack_p,
    itti_s10_relocation_cancel_response_t* relocation_cancel_response_p);

int s10_mme_handle_relocation_cancel_response(nw_gtpv2c_stack_handle_t* stack_p,
                                              nw_gtpv2c_ulp_api_t* pUlpApi);

/** Handle (timeout) error indication. */
int s10_mme_handle_ulp_error_indicatior(nw_gtpv2c_stack_handle_t* stack_p,
                                        nw_gtpv2c_ulp_api_t* pUlpApi);

/** Remove UE Tunnel in unexpected situations. */
int s10_mme_remove_ue_tunnel(nw_gtpv2c_stack_handle_t* stack_p,
                             itti_s10_remove_ue_tunnel_t* remove_ue_tunnel_p);

#endif /* FILE_S10_MME_SESSION_MANAGER_SEEN */

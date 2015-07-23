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


#include <stdint.h>

#include "s1ap_ies_defs.h"
#include "intertask_interface.h"

#ifndef S1AP_MME_NAS_PROCEDURES_H_
#define S1AP_MME_NAS_PROCEDURES_H_

/** \brief Handle an Initial UE message.
 * \param assocId lower layer assoc id (SCTP)
 * \param stream SCTP stream on which data had been received
 * \param message The message as decoded by the ASN.1 codec
 * @returns -1 on failure, 0 otherwise
 **/
int s1ap_mme_handle_initial_ue_message(uint32_t assocId, uint32_t stream,
                                       struct s1ap_message_s *message);

/** \brief Handle an Uplink NAS transport message.
 * Process the RRC transparent container and forward it to NAS entity.
 * \param assocId lower layer assoc id (SCTP)
 * \param stream SCTP stream on which data had been received
 * \param message The message as decoded by the ASN.1 codec
 * @returns -1 on failure, 0 otherwise
 **/
int s1ap_mme_handle_uplink_nas_transport(uint32_t assocId, uint32_t stream,
    struct s1ap_message_s *message);

/** \brief Handle a NAS non delivery indication message from eNB
 * \param assocId lower layer assoc id (SCTP)
 * \param stream SCTP stream on which data had been received
 * \param message The message as decoded by the ASN.1 codec
 * @returns -1 on failure, 0 otherwise
 **/
int s1ap_mme_handle_nas_non_delivery(uint32_t assocId, uint32_t stream,
                                     struct s1ap_message_s *message);

#if defined(DISABLE_USE_NAS)
int s1ap_handle_attach_accepted(nas_attach_accept_t *attach_accept_p);
#else
void s1ap_handle_conn_est_cnf(const mme_app_connection_establishment_cnf_t * const conn_est_cnf_p);
#endif

int s1ap_generate_downlink_nas_transport(const uint32_t ue_id, void * const data,
    const uint32_t size);
#endif /* S1AP_MME_NAS_PROCEDURES_H_ */

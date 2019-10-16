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

/*! \file sm_message_types.h
* \brief
* \author Dincer Beken
* \company Blackned GmbH
* \email: dbeken@blackned.de
*
*/

#ifndef FILE_SM_MESSAGES_TYPES_SEEN
#define FILE_SM_MESSAGES_TYPES_SEEN

#include "mme_ie_defs.h"
#include "3gpp_29.274.h"
#include "3gpp_36.413.h"

// todo: the sender and receiver side both use the same messages?
#define SM_MBMS_SESSION_START_REQUEST(mSGpTR)                   (mSGpTR)->ittiMsg.sm_mbms_session_start_request
#define SM_MBMS_SESSION_START_RESPONSE(mSGpTR)                  (mSGpTR)->ittiMsg.sm_mbms_session_start_response
#define SM_MBMS_SESSION_UPDATE_REQUEST(mSGpTR)                  (mSGpTR)->ittiMsg.sm_mbms_session_update_request
#define SM_MBMS_SESSION_UPDATE_RESPONSE(mSGpTR)                 (mSGpTR)->ittiMsg.sm_mbms_session_update_response
#define SM_MBMS_SESSION_STOP_REQUEST(mSGpTR)                    (mSGpTR)->ittiMsg.sm_mbms_session_stop_request
#define SM_MBMS_SESSION_STOP_RESPONSE(mSGpTR)                   (mSGpTR)->ittiMsg.sm_mbms_session_stop_response

/** Internal Messages. */
#define SM_REMOVE_UE_TUNNEL(mSGpTR)                            (mSGpTR)->ittiMsg.sm_remove_ue_tunnel

//-----------------------------------------------------------------------------
/** @struct itti_sm_mbms_session_start_request_t
 *  @brief MBMS Session Start Request
 *
 * Spec 3GPP TS 29.274, Universal Mobile Telecommunications System (UMTS);
 *                      LTE; 3GPP Evolved Packet System (EPS);
 *                      Evolved General Packet Radio Service (GPRS);
 *                      Tunnelling Protocol for Control plane (GTPv2-C); Stage 3
 * The Forward Relocation Request will be sent on Sm interface as
 * part of these procedures:
 * - Tracking Area Update procedure with MME change
 * - S1 based handover with MME change
 * - todo: also sent at attach with MME change with GUTI!! (without getting security context from HSS by new MME)
 */
typedef struct itti_sm_mbms_session_start_request_s {
  teid_t                          teid;                ///< SM-Target MME Tunnel Endpoint Identifier

  fteid_t                         sm_mbms_teid;        ///< SM-Source MBMS-GW Tunnel Endpoint Identifier

  /* S11 stack specific parameter. Not used in standalone epc mode */
  uintptr_t                       trxn;                 ///< Transaction identifier
  struct sockaddr                 peer_ip;              ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  uint16_t                        peer_port;            ///< MME port for S-GW or S-GW port for MME
} itti_sm_mbms_session_start_request_t;

//-----------------------------------------------------------------------------
/** @struct itti_sm_mbms_session_start_response_t
 *  @brief MBMS Session Start Response
 *
 * The Forward Relocation Response will be sent on Sm interface as
 * part of these procedures:
 * - Tracking Area Update procedure with MME change
 * - S1-based handover with MME change
 * - todo: also sent at attach with MME change with GUTI!! (without getting security context from HSS by new MME)
 */
typedef struct itti_sm_mbms_session_start_response_s {
  teid_t                   teid;                ///< Tunnel Endpoint Identifier

  // here fields listed in 3GPP TS 29.274
  gtpv2c_cause_t               cause;               ///< If the MME could successfully establish the UE context and the beaers.

  fteid_t                  sm_mme_teid;        ///< Target MME SM control plane (sender fteid)
  ///< This IE shall be sent on the Sm interfaces.

  // todo: Indication : This IE shall be included if any of the flags are set to 1. SGW Change Indication:   - This flag shall be set to 1 if the target MME/SGSN   has selected a new SGW.

  /* S11 stack specific parameter. Not used in standalone epc mode */
  struct sockaddr                 peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  void                    	      *trxn;               ///< Transaction identifier
} itti_sm_mbms_session_start_response_t;

//-----------------------------------------------------------------------------
/** @struct itti_sm_mbms_session_update_request_t
 *  @brief MBMS Session Update Request
 *
 * Spec 3GPP TS 29.274, Universal Mobile Telecommunications System (UMTS);
 *                      LTE; 3GPP Evolved Packet System (EPS);
 *                      Evolved General Packet Radio Service (GPRS);
 *                      Tunnelling Protocol for Control plane (GTPv2-C); Stage 3
 * The Forward Relocation Request will be sent on Sm interface as
 * part of these procedures:
 * - Tracking Area Update procedure with MME change
 * - S1 based handover with MME change
 * - todo: also sent at attach with MME change with GUTI!! (without getting security context from HSS by new MME)
 */
typedef struct itti_sm_mbms_session_update_request_s {
  teid_t                          teid;                ///< SM-Target MME Tunnel Endpoint Identifier

  fteid_t                         sm_mbms_teid;        ///< SM-Source MBMS-GW Tunnel Endpoint Identifier

  /* S11 stack specific parameter. Not used in standalone epc mode */
  uintptr_t                       trxn;                 ///< Transaction identifier
  struct sockaddr                 peer_ip;              ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  uint16_t                        peer_port;            ///< MME port for S-GW or S-GW port for MME
} itti_sm_mbms_session_update_request_t;

//-----------------------------------------------------------------------------
/** @struct itti_sm_mbms_session_update_response_t
 *  @brief MBMS Session Update Response
 *
 * The Forward Relocation Response will be sent on Sm interface as
 * part of these procedures:
 * - Tracking Area Update procedure with MME change
 * - S1-based handover with MME change
 * - todo: also sent at attach with MME change with GUTI!! (without getting security context from HSS by new MME)
 */
typedef struct itti_sm_mbms_session_update_response_s {
  teid_t                   teid;                ///< Tunnel Endpoint Identifier

  // here fields listed in 3GPP TS 29.274
  gtpv2c_cause_t               cause;               ///< If the MME could successfully establish the UE context and the beaers.

  fteid_t                  sm_mme_teid;        ///< Target MME SM control plane (sender fteid)
  ///< This IE shall be sent on the SM interfaces.

  // todo: Indication : This IE shall be included if any of the flags are set to 1. SGW Change Indication:   - This flag shall be set to 1 if the target MME/SGSN   has selected a new SGW.

  /* S11 stack specific parameter. Not used in standalone epc mode */
  struct sockaddr                 peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  void                    	      *trxn;               ///< Transaction identifier
} itti_sm_mbms_session_update_response_t;

//-----------------------------------------------------------------------------
/** @struct itti_sm_mbms_session_stop_request_t
 *  @brief MBMS Session Stop Request
 *
 * Spec 3GPP TS 29.274, Universal Mobile Telecommunications System (UMTS);
 *                      LTE; 3GPP Evolved Packet System (EPS);
 *                      Evolved General Packet Radio Service (GPRS);
 *                      Tunnelling Protocol for Control plane (GTPv2-C); Stage 3
 * The Forward Relocation Request will be sent on SM interface as
 * part of these procedures:
 * - Tracking Area Update procedure with MME change
 * - S1 based handover with MME change
 * - todo: also sent at attach with MME change with GUTI!! (without getting security context from HSS by new MME)
 */
typedef struct itti_sm_mbms_session_stop_request_s {
  teid_t                          teid;                ///< SM-Target MME Tunnel Endpoint Identifier
  /* S11 stack specific parameter. Not used in standalone epc mode */
  uintptr_t                       trxn;                 ///< Transaction identifier
  struct sockaddr                 peer_ip;              ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  uint16_t                        peer_port;            ///< MME port for S-GW or S-GW port for MME
} itti_sm_mbms_session_stop_request_t;

//-----------------------------------------------------------------------------
/** @struct itti_sm_mbms_session_stop_response_t
 *  @brief MBMS Session Stop Response
 *
 * The Forward Relocation Response will be sent on Sm interface as
 * part of these procedures:
 * - Tracking Area Update procedure with MME change
 * - S1-based handover with MME change
 * - todo: also sent at attach with MME change with GUTI!! (without getting security context from HSS by new MME)
 */
typedef struct itti_sm_mbms_session_stop_response_s {
  teid_t                   teid;                ///< Tunnel Endpoint Identifier

  // here fields listed in 3GPP TS 29.274
  gtpv2c_cause_t               cause;               ///< If the MME could successfully establish the UE context and the beaers.

  // todo: Indication : This IE shall be included if any of the flags are set to 1. SGW Change Indication:   - This flag shall be set to 1 if the target MME/SGSN   has selected a new SGW.

  /* S11 stack specific parameter. Not used in standalone epc mode */
  struct sockaddr                 peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  void                    	      *trxn;               ///< Transaction identifier
} itti_sm_mbms_session_stop_response_t;

//-----------------------------------------------------------------------------
/**
 * Internal Messages for error handling etc.
 */
typedef struct itti_sm_remove_ue_tunnel_s {
  /** Local Tunnel TEID. */
  teid_t                  local_teid;                ///< Sm MME Tunnel Endpoint Identifier

  struct sockaddr         peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  // here fields listed in 3GPP TS 29.274

  /** Cause to set (like error cause in GTPV2c State Machine). */
//  gtpv2c_cause_t               cause;

} itti_sm_remove_ue_tunnel_t;

#endif /* FILE_SM_MESSAGES_TYPES_SEEN */

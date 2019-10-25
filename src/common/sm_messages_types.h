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
#define SM_REMOVE_TUNNEL(mSGpTR)                             	(mSGpTR)->ittiMsg.sm_remove_tunnel

//-----------------------------------------------------------------------------
/** @struct itti_sm_mbms_session_start_request_t
 *  @brief MBMS Session Start Request
 *
 * Spec 3GPP TS 29.274, Universal Mobile Telecommunications System (UMTS);
 *                      LTE; 3GPP Evolved Packet System (EPS);
 *                      Evolved General Packet Radio Service (GPRS);
 *                      Tunnelling Protocol for Control plane (GTPv2-C); Stage 3
 * The MBMS Session Start Request message shall be sent on the Sm/Sn interface by the MBMS GW to the MME/SGSN
 * as specified in 3GPP TS 23.246 [37] and 3GPP TS 23.007 [13].
 */
typedef struct itti_sm_mbms_session_start_request_s {
  teid_t                             teid;                  ///< SM-MME Tunnel Endpoint Identifier
  fteid_t                            sm_mbms_fteid;          ///< SM-MBMS-GW Tunnel Endpoint Identifier

  /** MBMS specific parameters. */
  tmgi_t						     tmgi;                  ///< TMGI Identifier
  mbms_abs_time_data_transfer_t      abs_start_time;        ///< MBMS Absolute Time of Data Transfer. Anything else is not supported currently.
  mbms_flags_t				         mbms_flags;            ///< MBMS Flags
  mbms_ip_multicast_distribution_t  *mbms_ip_mc_address;    ///< MBMS IP Multicast Address.
  mbms_service_area_t                mbms_service_area;     ///< Service Area of the MBMS Service (todo: multiple are allowed to be sent, but only 1 will be supported).
  mbms_session_duration_t            mbms_session_duration; ///< The duration, where the TMGI of the MBMS session is still valid.
  bearer_qos_t                       mbms_bearer_level_qos; ///< QoS profile of the MBMS bearers to be established.
  uint16_t                           mbms_flow_id; 		    ///< MBMS Flow Id.

  /** Session Identifier is not supported, since it will only support GCS-AS. */

  /* Sm stack specific parameter. Not used in standalone epc mode */
  uintptr_t                       	 trxn;                 ///< Transaction identifier
  /* Sm stack specific parameter. Not used in standalone epc mode */
  union {
    struct sockaddr_in                peer_ipv4;             ///< MBMS-GW Sm IPv4 address
    struct sockaddr_in6               peer_ipv6;             ///< MBMS-GW Sm IPv6 address
  }mbms_peer_ip;
  uint16_t                        	 peer_port;            ///< MBMS-GW port for Sm interface.
} itti_sm_mbms_session_start_request_t;

//-----------------------------------------------------------------------------
/** @struct itti_sm_mbms_session_start_response_t
 *  @brief MBMS Session Start Response
 *
 * The MBMS Session Start Response message shall be sent as a response to the MBMS Session Start Request message
 * on the Sm/Sn interface by the MME/SGSN to the MBMS GW.
 */
typedef struct itti_sm_mbms_session_start_response_s {
  teid_t                             teid;                  ///< SM-MBMS-GW Tunnel Endpoint Identifier
  fteid_t                            sm_mme_teid;          ///< SM-MME Tunnel Endpoint Identifier

  // here fields listed in 3GPP TS 29.274
  gtpv2c_cause_t               		 cause;               	///< If the MME could successfully establish the MBMS service and the bearers.

  /* S11 stack specific parameter. */
  void                    	       	*trxn;               	///< Transaction identifier
  /* Sm stack specific parameter. Not used in standalone epc mode */
  union {
    struct sockaddr_in                peer_ipv4;             ///< MBMS-GW Sm IPv4 address
    struct sockaddr_in6               peer_ipv6;             ///< MBMS-GW Sm IPv6 address
  }mbms_peer_ip;
} itti_sm_mbms_session_start_response_t;

//-----------------------------------------------------------------------------
/** @struct itti_sm_mbms_session_update_request_t
 *  @brief MBMS Session Update Request
 *
 * Spec 3GPP TS 29.274, Universal Mobile Telecommunications System (UMTS);
 *                      LTE; 3GPP Evolved Packet System (EPS);
 *                      Evolved General Packet Radio Service (GPRS);
 *                      Tunnelling Protocol for Control plane (GTPv2-C); Stage 3
 * The MBMS Session Update Request message shall be sent on the Sm/Sn interface by the MBMS GW to the
 * MME/SGSN as specified in 3GPP TS 23.246 [37] and 3GPP TS 23.007 [13].
 */
typedef struct itti_sm_mbms_session_update_request_s {

  teid_t                             teid;                  ///< SM-MME Tunnel Endpoint Identifier
  fteid_t                            sm_mbms_fteid;          ///< SM-MBMS-GW Tunnel Endpoint Identifier

  /** MBMS specific parameters. */
  tmgi_t						     tmgi;                  ///< TMGI Identifier
  mbms_abs_time_data_transfer_t      abs_update_time;       ///< MBMS Absolute Time of Data Transfer. Anything else is not supported currently.
  mbms_flags_t				         mbms_flags;            ///< MBMS Flags
  mbms_service_area_t                mbms_service_area;     ///< Service Area of the MBMS Service (todo: multiple are allowed to be sent, but only 1 will be supported).
  mbms_session_duration_t            mbms_session_duration; ///< The duration, where the TMGI of the MBMS session is still valid.
  bearer_qos_t                      *mbms_bearer_level_qos; ///< QoS profile of the MBMS bearers to be established.
  uint16_t                           mbms_flow_id; 		    ///< MBMS Flow Id.

  /* Sm stack specific parameter. */
  uintptr_t                       	 trxn;                 ///< Transaction identifier
  /* Sm stack specific parameter. Not used in standalone epc mode */
  union {
    struct sockaddr_in                peer_ipv4;             ///< MBMS-GW Sm IPv4 address
    struct sockaddr_in6               peer_ipv6;             ///< MBMS-GW Sm IPv6 address
  }mbms_peer_ip;
  uint16_t                        	 peer_port;            ///< MBMS-GW Sm port
} itti_sm_mbms_session_update_request_t;

//-----------------------------------------------------------------------------
/** @struct itti_sm_mbms_session_update_response_t
 *  @brief MBMS Session Update Response
 *
 * The MBMS Session Update Response message shall be sent as a response to the MBMS Session Update Request
 * message on the Sm/Sn interface by the MME/SGSN to the MBMS GW.
 */
typedef struct itti_sm_mbms_session_update_response_s {
  teid_t                             teid;                ///< SM-MBMS-GW Tunnel Endpoint Identifier
  teid_t                             mms_sm_teid;         ///< SM-MME Tunnel Endpoint Identifier

  // here fields listed in 3GPP TS 29.274
  gtpv2c_cause_t               		 cause;               ///< If the MME could successfully establish the UE context and the beaers.

  /* Sm stack specific parameter. */
  /* Sm stack specific parameter. Not used in standalone epc mode */
  union {
    struct sockaddr_in                peer_ipv4;             ///< MBMS-GW Sm IPv4 address
    struct sockaddr_in6               peer_ipv6;             ///< MBMS-GW Sm IPv6 address
  }mbms_peer_ip;
  void                    	      	*trxn;                ///< Transaction identifier
} itti_sm_mbms_session_update_response_t;

//-----------------------------------------------------------------------------
/** @struct itti_sm_mbms_session_stop_request_t
 *  @brief MBMS Session Stop Request
 *
 * Spec 3GPP TS 29.274, Universal Mobile Telecommunications System (UMTS);
 *                      LTE; 3GPP Evolved Packet System (EPS);
 *                      Evolved General Packet Radio Service (GPRS);
 *                      Tunnelling Protocol for Control plane (GTPv2-C); Stage 3
 * The MBMS Session Stop Request message shall be sent on the Sm/Sn interface by the MBMS GW to the MME/SGSN
 * as specified in 3GPP TS 23.246 [37] and 3GPP TS 23.007 [13].
 */
typedef struct itti_sm_mbms_session_stop_request_s {
  teid_t                             teid;                  ///< SM-MME Tunnel Endpoint Identifier

  /** MBMS specific parameters. */
  mbms_abs_time_data_transfer_t      abs_stop_time;        ///< MBMS Absolute Time of Data Transfer. Anything else is not supported currently.
  mbms_flags_t				         mbms_flags;           ///< MBMS Flags
  uint16_t                           mbms_flow_id; 		   ///< MBMS Flow Id.

  /* Sm stack specific parameter. Not used in standalone epc mode */
  uintptr_t                       	 trxn;                 ///< Transaction identifier
  /* Sm stack specific parameter. Not used in standalone epc mode */
  union {
    struct sockaddr_in                peer_ipv4;             ///< MBMS-GW Sm IPv4 address
    struct sockaddr_in6               peer_ipv6;             ///< MBMS-GW Sm IPv6 address
  }mbms_peer_ip;
  uint16_t                        	 peer_port;            ///< MBMS-GW Sm port

} itti_sm_mbms_session_stop_request_t;

//-----------------------------------------------------------------------------
/** @struct itti_sm_mbms_session_stop_response_t
 *  @brief MBMS Session Stop Response
 *
 * The MBMS Session Start Response message shall be sent as a response to the MBMS Session Start Request message on the Sm/Sn
 * interface by the MME/SGSN to the MBMS GW.
 */
typedef struct itti_sm_mbms_session_stop_response_s {
  teid_t                   		 teid;                ///< MBMS-Gw Sm TEID
  teid_t                         mms_sm_teid;         ///< SM-MME Tunnel Endpoint Identifier

  // here fields listed in 3GPP TS 29.274
  gtpv2c_cause_t               	 cause;               ///< If the MME could successfully establish the UE context and the beaers.

  // todo: Indication : This IE shall be included if any of the flags are set to 1. SGW Change Indication:   - This flag shall be set to 1 if the target MME/SGSN   has selected a new SGW.

  /* Sm stack specific parameter. Not used in standalone epc mode */
  union {
    struct sockaddr_in                peer_ipv4;             ///< MBMS-GW Sm IPv4 address
    struct sockaddr_in6               peer_ipv6;             ///< MBMS-GW Sm IPv6 address
  }mbms_peer_ip;

  void                    	    *trxn;                ///< Transaction identifier
} itti_sm_mbms_session_stop_response_t;

//-----------------------------------------------------------------------------
/**
 * Internal Messages for error handling etc.
 */
typedef struct itti_sm_remove_tunnel_s {
  /** Local Tunnel TEID. */
  teid_t                  local_teid;                ///< Sm MME local Tunnel Endpoint Identifier

  /* Sm stack specific parameter. Not used in standalone epc mode */
  union {
    struct sockaddr_in                peer_ipv4;             ///< MBMS-GW Sm IPv4 address
    struct sockaddr_in6               peer_ipv6;             ///< MBMS-GW Sm IPv6 address
  }mbms_peer_ip;
  // here fields listed in 3GPP TS 29.274

  /** Cause to set (like error cause in GTPV2c State Machine). */
} itti_sm_remove_tunnel_t;

#endif /* FILE_SM_MESSAGES_TYPES_SEEN */

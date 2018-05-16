/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */

/*! \file s10_message_types.h
* \brief
* \author Dincer Beken
* \company Blackned GmbH
* \email: dbeken@blackned.de
*
*/

#ifndef FILE_S10_MESSAGES_TYPES_SEEN
#define FILE_S10_MESSAGES_TYPES_SEEN

#include "mme_ie_defs.h"

// todo: the sender and receiver side both use the same messages?
#define S10_FORWARD_RELOCATION_REQUEST(mSGpTR)                  (mSGpTR)->ittiMsg.s10_forward_relocation_request
#define S10_FORWARD_RELOCATION_RESPONSE(mSGpTR)                 (mSGpTR)->ittiMsg.s10_forward_relocation_response
#define S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION(mSGpTR)         (mSGpTR)->ittiMsg.s10_forward_access_context_notification
#define S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE(mSGpTR)          (mSGpTR)->ittiMsg.s10_forward_access_context_acknowledge
#define S10_FORWARD_RELOCATION_COMPLETE_NOTIFICATION(mSGpTR)    (mSGpTR)->ittiMsg.s10_forward_relocation_complete_notification
#define S10_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGE(mSGpTR)     (mSGpTR)->ittiMsg.s10_forward_relocation_complete_acknowledge
#define S10_CONTEXT_REQUEST(mSGpTR)                             (mSGpTR)->ittiMsg.s10_context_request
#define S10_CONTEXT_RESPONSE(mSGpTR)                            (mSGpTR)->ittiMsg.s10_context_response
#define S10_CONTEXT_ACKNOWLEDGE(mSGpTR)                         (mSGpTR)->ittiMsg.s10_context_acknowledge
#define S10_RELOCATION_CANCEL_REQUEST(mSGpTR)                   (mSGpTR)->ittiMsg.s10_relocation_cancel_request
#define S10_RELOCATION_CANCEL_RESPONSE(mSGpTR)                  (mSGpTR)->ittiMsg.s10_relocation_cancel_response

/** Internal Messages. */
#define S10_REMOVE_UE_TUNNEL(mSGpTR)                            (mSGpTR)->ittiMsg.s10_remove_ue_tunnel

//-----------------------------------------------------------------------------
/** @struct itti_s10_forward_relocation_request_t
 *  @brief Forward Relocation Request
 *
 * Spec 3GPP TS 29.274, Universal Mobile Telecommunications System (UMTS);
 *                      LTE; 3GPP Evolved Packet System (EPS);
 *                      Evolved General Packet Radio Service (GPRS);
 *                      Tunnelling Protocol for Control plane (GTPv2-C); Stage 3
 * The Forward Relocation Request will be sent on S10 interface as
 * part of these procedures:
 * - Tracking Area Update procedure with MME change
 * - S1 based handover with MME change
 * - todo: also sent at attach with MME change with GUTI!! (without getting security context from HSS by new MME)
 */
typedef struct itti_s10_forward_relocation_request_s {
  teid_t                          teid;                ///< S10-Target MME Tunnel Endpoint Identifier

  imsi_t                          imsi;                ///< The IMSI shall be included in the message on the S4/S11
  ///< interface, and on S5/S8 interface if provided by the
  ///< MME/SGSN, except for the case:
  ///<     - If the UE is emergency attached and the UE is UICCless.
  ///< The IMSI shall be included in the message on the S4/S11
  ///< interface, and on S5/S8 interface if provided by the
  ///< MME/SGSN, but not used as an identifier
  ///<     - if UE is emergency attached but IMSI is not authenticated.
  ///< The IMSI shall be included in the message on the S2b interface.

  fteid_t                         s10_source_mme_teid;                ///< S10-Source MME Tunnel Endpoint Identifier

  F_Cause_t                      *f_cause;

  target_identification_t         target_identification; /**< Making them pointer to decouple them. */

  mme_ue_eps_pdn_connections_t   *pdn_connections;
  ///< MME/SGSN UE EPS PDN Connections
  ///< Several IEs with this type and instance values shall be PDN Connection included as necessary to represent a list of PDN Connections

  fteid_t                         s11_sgw_teid;        ///< Sender F-TEID for control plane
  ///< This IE shall be sent on the S11/S4 interfaces. For the
  ///< S5/S8/S2b interfaces it is not needed because its content
  ///< would be identical to the IE PGW S5/S8/S2b F-TEID for
  ///< PMIP based interface or for GTP based Control Plane
  ///< interface.

  char*                           source_sgw_fqdn;
  ///< Source SGW FQDN

  mm_context_eps_t               *ue_eps_mm_context;   ///< EPS MM Context

  //todo: indication flags --> start with 0 to indicate no indirect data tunneling
  F_Container_t                  *eutran_container;   //E-UTRAN transparent container

  /** Source-To-Target Transparent Container. */
  // todo: if this is an buffer, how is it freed? does everything needs to be stacked

  /* S11 stack specific parameter. Not used in standalone epc mode */
  uintptr_t                       trxn;                ///< Transaction identifier
  struct in_addr                 *peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  uint16_t                        peer_port;           ///< MME port for S-GW or S-GW port for MME
} itti_s10_forward_relocation_request_t;


//-----------------------------------------------------------------------------
/** @struct itti_s10_forward_relocation_response_t
 *  @brief Forward Relocation Response
 *
 * The Forward Relocation Response will be sent on S10 interface as
 * part of these procedures:
 * - Tracking Area Update procedure with MME change
 * - S1-based handover with MME change
 * - todo: also sent at attach with MME change with GUTI!! (without getting security context from HSS by new MME)
 */
typedef struct itti_s10_forward_relocation_response_s {
  teid_t                   teid;                ///< Tunnel Endpoint Identifier

  // here fields listed in 3GPP TS 29.274
  gtpv2c_cause_t               cause;               ///< If the MME could successfully establish the UE context and the beaers.

  fteid_t                  s10_target_mme_teid;        ///< Target MME S10 control plane (sender fteid)
  ///< This IE shall be sent on the S10 interfaces.

  // todo: Indication : This IE shall be included if any of the flags are set to 1. SGW Change Indication:   - This flag shall be set to 1 if the target MME/SGSN   has selected a new SGW.

  // todo: list of bearer contexts (todo: after RAB has been established?)
  bearer_contexts_to_be_created_t  handovered_bearers;

  // todo: This IE is included if cause value is contained in S1-AP message. Refer to the 3GPP TS 29.010 [42] for the mapping of cause values between S1AP, RANAP and BSSGP.

  F_Container_t            eutran_container;

  F_Cause_t                       f_cause;

  // EPC_Timer pgw_back_off_time                ///< This IE may be included on the S5/S8 and S4/S11
  ///< interfaces when the PDN GW rejects the Create Session
  ///< Request with the cause "APN congestion". It indicates the
  ///< time during which the MME or S4-SGSN should refrain
  ///< from sending subsequent PDN connection establishment
  ///< requests to the PGW for the congested APN for services
  ///< other than Service Users/emergency services.
  ///< See NOTE 3:
  ///< The last received value of the PGW Back-Off Time IE shall supersede any previous values received
  ///< from that PGW and for this APN in the MME/SGSN.

  // Private Extension                          ///< This IE may be sent on the S5/S8, S4/S11 and S2b
  ///< interfaces.

  /* S11 stack specific parameter. Not used in standalone epc mode */
  void                    *trxn;               ///< Transaction identifier
  struct in_addr           peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
} itti_s10_forward_relocation_response_t;


//-----------------------------------------------------------------------------
/** @struct itti_s10_forward_access_context_notification_t
 *  @brief Forward Access Context Notification
 *
 * The Forward Access Context Notification will be sent on S10 interface as
 * part of these procedures:
 * - E-UTRAN Tracking Area Update with MME Change
 * - S1-based Handover with MME change
 * - todo: also sent at attach with MME change with GUTI!! (without getting security context from HSS by new MME)
 */
typedef struct itti_s10_forward_access_context_notification_s {
  teid_t                     local_teid;  /**< Local TEID. */
  teid_t                     teid;       ///< not in specs for inner MME use

  F_Container_t              eutran_container;
  // todo: Indication : This IE shall be included if any of the flags are set to 1. SGW Change Indication:   - This flag shall be set to 1 if the target MME/SGSN   has selected a new SGW.

  // Private Extension   Private Extension

  /* GTPv2-C specific parameters */
  void                      *trxn;                        ///< Transaction identifier
  struct in_addr             peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
} itti_s10_forward_access_context_notification_t;

/** @struct itti_s10_forward_access_context_notification_acknowledge_t
 *  @brief Forward Access Context Notification Acknowledge
 *
 * The Forward Access Context Notification Acknowledge will be sent on S10 interface as
 * part of these procedures:
 * - E-UTRAN Tracking Area Update with MME Change
 * - S1-based Handover with MME change
 * - todo: also sent at attach with MME change with GUTI!! (without getting security context from HSS by new MME)
 */
typedef struct itti_s10_forward_access_context_acknowledge_s {
  teid_t                     teid;       ///< not in specs for inner MME use
  teid_t                     local_teid;  /**< Local TEID. */

  gtpv2c_cause_t                 cause;               ///< If the MME could successfully establish the UE context and the beaers.

  /* GTPv2-C specific parameters */
  void                      *trxn;                        ///< Transaction identifier
  struct in_addr             peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
} itti_s10_forward_access_context_acknowledge_t;


//-----------------------------------------------------------------------------
/** @struct itti_s10_forward_relocation_complete_notification_t
 *  @brief Forward Relocation Complete Notification
 *
 * The Forward Relocation Complete Notification will be sent on S10 interface as
 * part of these procedures:
 * - E-UTRAN Tracking Area Update with MME Change
 * - S1-based Handover with MME change
 * - todo: also sent at attach with MME change with GUTI!! (without getting security context from HSS by new MME)
 */
typedef struct itti_s10_forward_relocation_complete_notification_s {
  /** Destination TEID. */
  teid_t                     teid;

  /** Sender TEID. */
  teid_t                     local_teid;       ///< not in specs for inner MME use


  // todo: Indication : This IE shall be included if any of the flags are set to 1. SGW Change Indication:   - This flag shall be set to 1 if the target MME/SGSN   has selected a new SGW.

  // Private Extension   Private Extension

  /* GTPv2-C specific parameters */
  void                      *trxn;                        ///< Transaction identifier
  struct in_addr             peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
} itti_s10_forward_relocation_complete_notification_t;

//-----------------------------------------------------------------------------
/** @struct itti_s10_forward_relocation_complete_acknowledge_t
 *  @brief Forward Relocation Complete Acknowledge
 *
 * The Forward Relocation Complete Acknowledge will be sent on S10 interface as
 * part of these procedures:
 * - E-UTRAN Tracking Area Update with MME Change
 * - S1-based Handover with MME change
 */
typedef struct itti_s10_forward_relocation_complete_acknowledge_s {
  /** Local TEID. */
  teid_t                     teid;                ///< S11 MME Tunnel Endpoint Identifier
  /** Sender TEID. */
  teid_t                     local_teid;       ///< not in specs for inner MME use
  // here fields listed in 3GPP TS 29.274
  gtpv2c_cause_t                 cause;               ///<
  // Private Extension Private Extension        ///< optional
  /* S11 stack specific parameter. Not used in standalone epc mode */
  struct in_addr             peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  void                      *trxn;                      ///< Transaction identifier
} itti_s10_forward_relocation_complete_acknowledge_t;

//-----------------------------------------------------------------------------
/** @struct itti_s10_context_request_t
 *  @brief Context Request
 *
 * The Context Request message be sent on S10 interface as
 * part of these procedures from the new to the old MME:
 * - E-UTRAN Tracking Area Update with MME Change
 */
typedef struct itti_s10_context_request_s{
  teid_t                   teid;                ///< S11 MME Tunnel Endpoint Identifier

  // here fields listed in 3GPP TS 29.274
  guti_t                   old_guti;               ///<

  imsi_t                   imsi;                ///< The IMSI shall be included in the message on the S4/S11

  fteid_t                  s10_target_mme_teid;             ///< New MME S10 Tunnel Endpoint Identifier

  rat_type_t               rat_type;

  ServingNetwork_t          serving_network;     ///< This IE shall be included on the S4/S11, S5/S8 and S2b

  // recovery_t(restart counter) recovery;      ///< This IE shall be included if contacting the peer for the first
  ///< time.

  // Private Extension Private Extension        ///< optional

  /** Original Tracking area update message. */
  bstring      complete_request_message; /**< Could be a TAU or an attach request message. */
  /* S11 stack specific parameter. Not used in standalone epc mode */

  void                           *trxn;                ///< Transaction identifier
  struct in_addr                  peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  uint16_t                        peer_port;           ///< MME port for S-GW or S-GW port for MME

} itti_s10_context_request_t;

//-----------------------------------------------------------------------------
/** @struct itti_s10_context_response_t
 *  @brief Context Request
 *
 * The Context Request message be sent on S10 interface as
 * part of these procedures from the old to the new MME:
 * - E-UTRAN Tracking Area Update with MME Change
 */
typedef struct itti_s10_context_response_s{
  teid_t                   teid;                ///< S11 MME Tunnel Endpoint Identifier (new MME)

  gtpv2c_cause_t               cause;               ///< If the MME could successfully establish the UE context and the beaers.

  // here fields listed in 3GPP TS 29.274

  fteid_t                  s10_source_mme_teid;             ///< Old MME S10 Tunnel Endpoint Identifier

  imsi_t                   imsi;                ///< The IMSI shall be included in the message on the S4/S11
  ///< interface, and on S5/S8 interface if provided by the
  ///< MME/SGSN, except for the case:
  ///<     - If the UE is emergency attached and the UE is UICCless.
  ///< The IMSI shall be included in the message on the S4/S11
  ///< interface, and on S5/S8 interface if provided by the
  ///< MME/SGSN, but not used as an identifier
  ///<     - if UE is emergency attached but IMSI is not authenticated.
  ///< The IMSI shall be included in the message on the S2b interface.

  mme_ue_eps_pdn_connections_t   *pdn_connections;
  ///< MME/SGSN UE EPS PDN Connections
  ///< Several IEs with this type and instance values shall be PDN Connection included as necessary to represent a list of PDN Connections

  fteid_t                  s11_sgw_teid;        ///< Old SGW F-TEID for control plane
  ///< This IE shall be sent on the S11/S4 interfaces. For the
  ///< S5/S8/S2b interfaces it is not needed because its content
  ///< would be identical to the IE PGW S5/S8/S2b F-TEID for
  ///< PMIP based interface or for GTP based Control Plane
  ///< interface.

  char*                    source_sgw_fqdn;
  ///< Old Source SGW FQDN

  mm_context_eps_t         ue_eps_mm_context;
  // todo: indication

  // recovery_t(restart counter) recovery;      ///< This IE shall be included if contacting the peer for the first
  ///< time.

  // Private Extension Private Extension        ///< optional

  /* S11 stack specific parameter. Not used in standalone epc mode */
  void*                           trxn;                      ///< Transaction identifier (received) or the transaction itself (when sent)

  struct in_addr                  peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  uint16_t                        peer_port;           ///< MME port for S-GW or S-GW port for MME
} itti_s10_context_response_t;

//-----------------------------------------------------------------------------
/** @struct itti_s10_context_acknowledge_t
 *  @brief Context Acknowledge
 *
 * The Context Acknowledge will be sent on S10 interface as
 * part of these procedures:
 * - E-UTRAN Tracking Area Update with MME Change
 */
typedef struct itti_s10_context_acknowledge_s {
  teid_t                   teid;                ///< S11 MME Tunnel Endpoint Identifier
  uint32_t                 ue_id;
  // here fields listed in 3GPP TS 29.274
  gtpv2c_cause_t               cause;               ///<

  // todo: indication flags
  // recovery_t(restart counter) recovery;      ///< This IE shall be included if contacting the peer for the first
  ///< time.

  // Private Extension Private Extension        ///< optional

  /* S11 stack specific parameter. Not used in standalone epc mode */
  struct in_addr                peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  uint16_t                      peer_port;      ///< MME port for S-GW or S-GW port for MME
  uint32_t                      trxnId;           ///< Transaction identifier
} itti_s10_context_acknowledge_t;

//-----------------------------------------------------------------------------
typedef struct itti_s10_relocation_cancel_request_s {
  teid_t                   teid;                ///< S11 MME Tunnel Endpoint Identifier
  uint32_t                 ue_id;
  // here fields listed in 3GPP TS 29.274
  imsi_t                   imsi;
  teid_t                   local_teid;       ///< not in specs for inner MME use

  // recovery_t(restart counter) recovery;      ///< This IE shall be included if contacting the peer for the first
  ///< time.

  // Private Extension Private Extension        ///< optional

  /* S11 stack specific parameter. Not used in standalone epc mode */
  struct in_addr                peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  uint16_t                      peer_port;      ///< MME port for S-GW or S-GW port for MME
  void                         *trxn;           ///< Transaction identifier
} itti_s10_relocation_cancel_request_t;

typedef struct itti_s10_relocation_cancel_response_s {
  teid_t                   teid;                ///< S11 MME Tunnel Endpoint Identifier
  uint32_t                 ue_id;
  // here fields listed in 3GPP TS 29.274
  gtpv2c_cause_t               cause;               ///<
  teid_t                   local_teid;       ///< not in specs for inner MME use

  // todo: indication flags
  // recovery_t(restart counter) recovery;      ///< This IE shall be included if contacting the peer for the first
  ///< time.

  // Private Extension Private Extension        ///< optional

  /* S11 stack specific parameter. Not used in standalone epc mode */
  struct in_addr                peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  uint16_t                      peer_port;      ///< MME port for S-GW or S-GW port for MME
  void                         *trxn;           ///< Transaction identifier
} itti_s10_relocation_cancel_response_t;

//-----------------------------------------------------------------------------
/**
 * Internal Messages for error handling etc.
 */
typedef struct itti_s10_remove_ue_tunnel_s {
  /** Local Tunnel TEID. */
  teid_t                  local_teid;                ///< S10 MME Tunnel Endpoint Identifier
  teid_t                  remote_teid;                ///< S10 MME Tunnel Endpoint Identifier

  struct in_addr                peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  // here fields listed in 3GPP TS 29.274

  /** Cause to set (like error cause in GTPV2c State Machine). */
//  gtpv2c_cause_t               cause;

} itti_s10_remove_ue_tunnel_t;

#endif /* FILE_S10_MESSAGES_TYPES_SEEN */

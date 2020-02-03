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

/*! \file s11_messages_types.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_S11_MESSAGES_TYPES_SEEN
#define FILE_S11_MESSAGES_TYPES_SEEN

#include "3gpp_29.274.h"

#ifdef __cplusplus
extern "C" {
#endif

#define S11_CREATE_SESSION_REQUEST(mSGpTR)         ((itti_s11_create_session_request_t*)(mSGpTR)->itti_msg)
#define S11_CREATE_SESSION_RESPONSE(mSGpTR)        ((itti_s11_create_session_response_t*)(mSGpTR)->itti_msg)
#define S11_CREATE_BEARER_REQUEST(mSGpTR)          ((itti_s11_create_bearer_request_t*)(mSGpTR)->itti_msg)
#define S11_CREATE_BEARER_RESPONSE(mSGpTR)         ((itti_s11_create_bearer_response_t*)(mSGpTR)->itti_msg)
#define S11_MODIFY_BEARER_REQUEST(mSGpTR)          ((itti_s11_modify_bearer_request_t*)(mSGpTR)->itti_msg)
#define S11_MODIFY_BEARER_RESPONSE(mSGpTR)         ((itti_s11_modify_bearer_response_t*)(mSGpTR)->itti_msg)
#define S11_REMOTE_UE_REPORT_NOTIFICATION(mSGpTR)  ((itti_s11_remote_ue_remote_notification_t*)(mSGpTR)->itti_msg)
#define S11_REMOTE_UE_REPORT_ACKNOWLEDGE(mSGpTR)   ((itti_s11_remote_ue_remote_acknowledge_t*)(mSGpTR)->itti_msg)
#define S11_DELETE_SESSION_REQUEST(mSGpTR)         ((itti_s11_delete_session_request_t*)(mSGpTR)->itti_msg)
#define S11_DELETE_BEARER_COMMAND(mSGpTR)          ((itti_s11_delete_bearer_command_t*)(mSGpTR)->itti_msg)
#define S11_DELETE_SESSION_RESPONSE(mSGpTR)        ((itti_s11_delete_session_response_t*)(mSGpTR)->itti_msg)
#define S11_RELEASE_ACCESS_BEARERS_REQUEST(mSGpTR) ((itti_s11_release_access_bearers_request_t*)(mSGpTR)->itti_msg)
#define S11_RELEASE_ACCESS_BEARERS_RESPONSE(mSGpTR) ((itti_s11_release_access_bearers_response_t*)(mSGpTR)->itti_msg)
#define S11_DOWNLINK_DATA_NOTIFICATION(mSGpTR)	   ((itti_s11_downlink_data_notification_t*)(mSGpTR)->itti_msg)
#define S11_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE(mSGpTR)     ((itti_s11_downlink_data_notification_acknowledge_t*)(mSGpTR)->itti_msg)
#define S11_DOWNLINK_DATA_NOTIFICATION_FAILURE_INDICATION(mSGpTR)  ((itti_s11_downlink_data_notification_failure_indication_t*)(mSGpTR)->itti_msg)


//-----------------------------------------------------------------------------
/** @struct itti_s11_create_session_request_t
 *  @brief Create Session Request
 *
 * Spec 3GPP TS 29.274, Universal Mobile Telecommunications System (UMTS);
 *                      LTE; 3GPP Evolved Packet System (EPS);
 *                      Evolved General Packet Radio Service (GPRS);
 *                      Tunnelling Protocol for Control plane (GTPv2-C); Stage 3
 * The Create Session Request will be sent on S11 interface as
 * part of these procedures:
 * - E-UTRAN Initial Attach
 * - UE requested PDN connectivity
 * - Tracking Area Update procedure with Serving GW change
 * - S1/X2-based handover with SGW change
 */
typedef struct itti_s11_create_session_request_s {
  uint64_t           ie_presence_mask;
#define S11_CREATE_SESSION_REQUEST_PR_IE_IMSI                               ((uint64_t)1)
#define S11_CREATE_SESSION_REQUEST_PR_IE_MSISDN                              ((uint64_t)1 << 1)
#define S11_CREATE_SESSION_REQUEST_PR_IE_MEI                                 ((uint64_t)1 << 2)
#define S11_CREATE_SESSION_REQUEST_PR_IE_ULI                                 ((uint64_t)1 << 3)
#define S11_CREATE_SESSION_REQUEST_PR_IE_SERVING_NETWORK                     ((uint64_t)1 << 4)
#define S11_CREATE_SESSION_REQUEST_PR_IE_RAT_TYPE                            ((uint64_t)1 << 5)
#define S11_CREATE_SESSION_REQUEST_PR_IE_INDICATION_FLAGS                    ((uint64_t)1 << 6)
#define S11_CREATE_SESSION_REQUEST_PR_IE_SENDER_FTEID_FOR_CONTROL_PLANE      ((uint64_t)1 << 7)
#define S11_CREATE_SESSION_REQUEST_PR_IE_PGW_S5S8_ADDRESS_FOR_CONTROL_PLANE  ((uint64_t)1 << 8)
#define S11_CREATE_SESSION_REQUEST_PR_IE_APN                                 ((uint64_t)1 << 9)
#define S11_CREATE_SESSION_REQUEST_PR_IE_SELECTION_MODE                      ((uint64_t)1 << 10)
#define S11_CREATE_SESSION_REQUEST_PR_IE_PDN_TYPE                            ((uint64_t)1 << 11)
#define S11_CREATE_SESSION_REQUEST_PR_IE_PAA                                 ((uint64_t)1 << 12)
#define S11_CREATE_SESSION_REQUEST_PR_IE_MAXIMUM_APN_RESTRICTION             ((uint64_t)1 << 13)
#define S11_CREATE_SESSION_REQUEST_PR_IE_APN_AMBR                            ((uint64_t)1 << 14)
#define S11_CREATE_SESSION_REQUEST_PR_IE_LINKED_EPS_BEARER_ID                ((uint64_t)1 << 15)
#define S11_CREATE_SESSION_REQUEST_PR_IE_TRUSTED_WLAN_MODE_INDICATION        ((uint64_t)1 << 16)
#define S11_CREATE_SESSION_REQUEST_PR_IE_PCO                                 ((uint64_t)1 << 17)
#define S11_CREATE_SESSION_REQUEST_PR_IE_BEARER_CONTEXTS_TO_BE_CREATED       ((uint64_t)1 << 18)
#define S11_CREATE_SESSION_REQUEST_PR_IE_BEARER_CONTEXTS_TO_BE_REMOVED       ((uint64_t)1 << 19)
#define S11_CREATE_SESSION_REQUEST_PR_IE_TRACE_INFORMATION                   ((uint64_t)1 << 20)
#define S11_CREATE_SESSION_REQUEST_PR_IE_RECOVERY                            ((uint64_t)1 << 21)
#define S11_CREATE_SESSION_REQUEST_PR_IE_MME_FQ_CSID                         ((uint64_t)1 << 22)
#define S11_CREATE_SESSION_REQUEST_PR_IE_SGW_FQ_CSID                         ((uint64_t)1 << 23)
#define S11_CREATE_SESSION_REQUEST_PR_IE_EPDG_FQ_CSID                        ((uint64_t)1 << 24)
#define S11_CREATE_SESSION_REQUEST_PR_IE_TWAN_FQ_CSID                        ((uint64_t)1 << 25)
#define S11_CREATE_SESSION_REQUEST_PR_IE_UE_TIME_ZONE                        ((uint64_t)1 << 26)
#define S11_CREATE_SESSION_REQUEST_PR_IE_UCI                                 ((uint64_t)1 << 27)
#define S11_CREATE_SESSION_REQUEST_PR_IE_CHARGING_CHARACTERISTICS            ((uint64_t)1 << 28)
#define S11_CREATE_SESSION_REQUEST_PR_IE_MME_S4_SGSN_LDN                     ((uint64_t)1 << 29)
#define S11_CREATE_SESSION_REQUEST_PR_IE_SGW_LDN                             ((uint64_t)1 << 30)
#define S11_CREATE_SESSION_REQUEST_PR_IE_EPDG_LDN                            ((uint64_t)1 << 31)
#define S11_CREATE_SESSION_REQUEST_PR_IE_TWAN_LDN                            ((uint64_t)1 << 32)
#define S11_CREATE_SESSION_REQUEST_PR_IE_SIGNALLING_PRIORITY_INDICATION      ((uint64_t)1 << 33)
#define S11_CREATE_SESSION_REQUEST_PR_IE_UE_LOCAL_IP_ADDRESS                 ((uint64_t)1 << 34)
#define S11_CREATE_SESSION_REQUEST_PR_IE_UE_UDP_PORT                         ((uint64_t)1 << 35)
#define S11_CREATE_SESSION_REQUEST_PR_IE_APCO                                ((uint64_t)1 << 36)
#define S11_CREATE_SESSION_REQUEST_PR_IE_HENB_LOCAL_IP_ADDRESS               ((uint64_t)1 << 37)
#define S11_CREATE_SESSION_REQUEST_PR_IE_HENB_UDP_PORT                       ((uint64_t)1 << 38)
#define S11_CREATE_SESSION_REQUEST_PR_IE_MME_S4_SGSN_IDENTIFIER              ((uint64_t)1 << 39)
#define S11_CREATE_SESSION_REQUEST_PR_IE_TWAN_IDENTIFIER                     ((uint64_t)1 << 40)
#define S11_CREATE_SESSION_REQUEST_PR_IE_EPDG_IP_ADDRESS                     ((uint64_t)1 << 41)
#define S11_CREATE_SESSION_REQUEST_PR_IE_CN_OPERATOR_SELECTION_ENTITY        ((uint64_t)1 << 42)
#define S11_CREATE_SESSION_REQUEST_PR_IE_PRESENCE_REPORTING_AREA_INFORMATION ((uint64_t)1 << 43)
#define S11_CREATE_SESSION_REQUEST_PR_IE_MMES4_SGSN_OVERLOAD_CONTROL_INFORMATION ((uint64_t)1 << 44)
#define S11_CREATE_SESSION_REQUEST_PR_IE_SGW_OVERLOAD_CONTROL_INFORMATION    ((uint64_t)1 << 45)
#define S11_CREATE_SESSION_REQUEST_PR_IE_TWAN_EPDG_OVERLOAD_CONTROL_INFORMATION ((uint64_t)1 << 46)
#define S11_CREATE_SESSION_REQUEST_PR_IE_ORIGINATION_TIME_STAMP              ((uint64_t)1 << 47)
#define S11_CREATE_SESSION_REQUEST_PR_IE_MAXIMUM_WAIT_TIME                   ((uint64_t)1 << 48)
#define S11_CREATE_SESSION_REQUEST_PR_IE_WLAN_LOCATION_INFORMATION           ((uint64_t)1 << 49)
#define S11_CREATE_SESSION_REQUEST_PR_IE_WLAN_LOCATION_TIMESTAMP             ((uint64_t)1 << 50)
#define S11_CREATE_SESSION_REQUEST_PR_IE_NBIFOM_CONTAINER                    ((uint64_t)1 << 51)
#define S11_CREATE_SESSION_REQUEST_PR_IE_REMOTE_UE_CONTEXT_CONNECTED         ((uint64_t)1 << 52)
#define S11_CREATE_SESSION_REQUEST_PR_IE_3GPP_AAA_SERVER_IDENTIFIER          ((uint64_t)1 << 53)
#define S11_CREATE_SESSION_REQUEST_PR_IE_EPCO                                ((uint64_t)1 << 54)
#define S11_CREATE_SESSION_REQUEST_PR_IE_SERVING_PLMN_RATE_CONTROL           ((uint64_t)1 << 55)
#define S11_CREATE_SESSION_REQUEST_PR_IE_MO_EXCEPTION_DATA_COUNTER           ((uint64_t)1 << 56)
#define S11_CREATE_SESSION_REQUEST_PR_IE_UE_TCP_PORT                         ((uint64_t)1 << 57)
#define S11_CREATE_SESSION_REQUEST_PR_IE_MAPPED_UE_USAGE_TYPE                ((uint64_t)1 << 58)
#define S11_CREATE_SESSION_REQUEST_PR_IE_ULI_FOR_SGW                         ((uint64_t)1 << 59)
#define S11_CREATE_SESSION_REQUEST_PR_IE_SGW_U_NODE_NAME                     ((uint64_t)1 << 60)
#define S11_CREATE_SESSION_REQUEST_PR_IE_PRIVATE_EXTENSION                   ((uint64_t)1 << 61)

  teid_t             teid;                ///< S11- S-GW Tunnel Endpoint Identifier

  imsi_t             imsi;                ///< The IMSI shall be included in the message on the S4/S11
  ///< interface, and on S5/S8 interface if provided by the
  ///< MME/SGSN, except for the case:
  ///<     - If the UE is emergency attached and the UE is UICCless.
  ///< The IMSI shall be included in the message on the S4/S11
  ///< interface, and on S5/S8 interface if provided by the
  ///< MME/SGSN, but not used as an identifier
  ///<     - if UE is emergency attached but IMSI is not authenticated.
  ///< The IMSI shall be included in the message on the S2b interface.

  Msisdn_t           msisdn;              ///< For an E-UTRAN Initial Attach the IE shall be included
  ///< when used on the S11 interface, if provided in the
  ///< subscription data from the HSS.
  ///< For a PDP Context Activation procedure the IE shall be
  ///< included when used on the S4 interface, if provided in the
  ///< subscription data from the HSS.
  ///< The IE shall be included for the case of a UE Requested
  ///< PDN Connectivity, if the MME has it stored for that UE.
  ///< It shall be included when used on the S5/S8 interfaces if
  ///< provided by the MME/SGSN.
  ///< The ePDG shall include this IE on the S2b interface during
  ///< an Attach with GTP on S2b and a UE initiated Connectivity
  ///< to Additional PDN with GTP on S2b, if provided by the
  ///< HSS/AAA.

  Mei_t              mei;                 ///< The MME/SGSN shall include the ME Identity (MEI) IE on
  ///< the S11/S4 interface:
  ///<     - If the UE is emergency attached and the UE is UICCless
  ///<     - If the UE is emergency attached and the IMSI is not authenticated
  ///< For all other cases the MME/SGSN shall include the ME
  ///< Identity (MEI) IE on the S11/S4 interface if it is available.
  ///< If the SGW receives this IE, it shall forward it to the PGW
  ///< on the S5/S8 interface.

  Uli_t              uli;                 ///< This IE shall be included on the S11 interface for E-
  ///< UTRAN Initial Attach and UE-requested PDN Connectivity
  ///< procedures. It shall include ECGI&TAI. The MME/SGSN
  ///< shall also include it on the S11/S4 interface for
  ///< TAU/RAU/X2-Handover/Enhanced SRNS Relocation
  ///< procedure if the PGW has requested location information
  ///< change reporting and MME/SGSN support location
  ///< information change reporting. The SGW shall include this
  ///< IE on S5/S8 if it receives the ULI from MME/SGSN.

  ServingNetwork_t   serving_network;     ///< This IE shall be included on the S4/S11, S5/S8 and S2b
  ///< interfaces for an E-UTRAN initial attach, a PDP Context
  ///< Activation, a UE requested PDN connectivity, an Attach
  ///< with GTP on S2b, a UE initiated Connectivity to Additional
  ///< PDN with GTP on S2b and a Handover to Untrusted Non-
  ///< 3GPP IP Access with GTP on S2b.

  rat_type_t         rat_type;            ///< This IE shall be set to the 3GPP access type or to the
  ///< value matching the characteristics of the non-3GPP access
  ///< the UE is using to attach to the EPS.
  ///< The ePDG may use the access technology type of the
  ///< untrusted non-3GPP access network if it is able to acquire
  ///< it; otherwise it shall indicate Virtual as the RAT Type.
  ///< See NOTE 3, NOTE 4.

  indication_flags_t indication_flags;    ///< This IE shall be included if any one of the applicable flags
  ///< is set to 1.
  ///< Applicable flags are:
  ///<     - S5/S8 Protocol Type: This flag shall be used on
  ///<       the S11/S4 interfaces and set according to the
  ///<       protocol chosen to be used on the S5/S8
  ///<       interfaces.
  ///<
  ///<     - Dual Address Bearer Flag: This flag shall be used
  ///<       on the S2b, S11/S4 and S5/S8 interfaces and shall
  ///<       be set to 1 when the PDN Type, determined based
  ///<       on UE request and subscription record, is set to
  ///<       IPv4v6 and all SGSNs which the UE may be
  ///<       handed over to support dual addressing. This shall
  ///<       be determined based on node pre-configuration by
  ///<       the operator.
  ///<
  ///<     - Handover Indication: This flag shall be set to 1 on
  ///<       the S11/S4 and S5/S8 interface during an E-
  ///<       UTRAN Initial Attach or a UE Requested PDN
  ///<       Connectivity or aPDP Context Activation procedure
  ///<       if the PDN connection/PDP Context is handed-over
  ///<       from non-3GPP access.
  ///<       This flag shall be set to 1 on the S2b interface
  ///<       during a Handover to Untrusted Non-3GPP IP
  ///<       Access with GTP on S2b and IP address
  ///<       preservation is requested by the UE.
  ///<
  ///<       ....
  ///<     - Unauthenticated IMSI: This flag shall be set to 1
  ///<       on the S4/S11 and S5/S8 interfaces if the IMSI
  ///<       present in the message is not authenticated and is
  ///<       for an emergency attached UE.

  fteid_t            sender_fteid_for_cp; ///< Sender F-TEID for control plane (MME)

  fteid_t            pgw_address_for_cp;  ///< PGW S5/S8 address for control plane or PMIP
  ///< This IE shall be sent on the S11 / S4 interfaces. The TEID
  ///< or GRE Key is set to "0" in the E-UTRAN initial attach, the
  ///< PDP Context Activation and the UE requested PDN
  ///< connectivity procedures.

  char               apn[ACCESS_POINT_NAME_MAX_LENGTH + 1]; ///< Access Point Name

  SelectionMode_t    selection_mode;      ///< Selection Mode
  ///< This IE shall be included on the S4/S11 and S5/S8
  ///< interfaces for an E-UTRAN initial attach, a PDP Context
  ///< Activation and a UE requested PDN connectivity.
  ///< This IE shall be included on the S2b interface for an Initial
  ///< Attach with GTP on S2b and a UE initiated Connectivity to
  ///< Additional PDN with GTP on S2b.
  ///< It shall indicate whether a subscribed APN or a non
  ///< subscribed APN chosen by the MME/SGSN/ePDG was
  ///< selected.
  ///< CO: When available, this IE shall be sent by the MME/SGSN on
  ///< the S11/S4 interface during TAU/RAU/HO with SGW
  ///< relocation.

  pdn_type_t         pdn_type;            ///< PDN Type
  ///< This IE shall be included on the S4/S11 and S5/S8
  ///< interfaces for an E-UTRAN initial attach, a PDP Context
  ///< Activation and a UE requested PDN connectivity.
  ///< This IE shall be set to IPv4, IPv6 or IPv4v6. This is based
  ///< on the UE request and the subscription record retrieved
  ///< from the HSS (for MME see 3GPP TS 23.401 [3], clause
  ///< 5.3.1.1, and for SGSN see 3GPP TS 23.060 [35], clause
  ///< 9.2.1). See NOTE 1.

  paa_t              paa;                 ///< PDN Address Allocation
  ///< This IE shall be included the S4/S11, S5/S8 and S2b
  ///< interfaces for an E-UTRAN initial attach, a PDP Context
  ///< Activation, a UE requested PDN connectivity, an Attach
  ///< with GTP on S2b, a UE initiated Connectivity to Additional
  ///< PDN with GTP on S2b and a Handover to Untrusted Non-
  ///< 3GPP IP Access with GTP on S2b. For PMIP-based
  ///< S5/S8, this IE shall also be included on the S4/S11
  ///< interfaces for TAU/RAU/Handover cases involving SGW
  ///< relocation.
  ///< The PDN type field in the PAA shall be set to IPv4, or IPv6
  ///< or IPv4v6 by MME, based on the UE request and the
  ///< subscription record retrieved from the HSS.
  ///< For static IP address assignment (for MME see 3GPP TS
  ///< 23.401 [3], clause 5.3.1.1, for SGSN see 3GPP TS 23.060
  ///< [35], clause 9.2.1, and for ePDG see 3GPP TS 23.402 [45]
  ///< subclause 4.7.3), the MME/SGSN/ePDG shall set the IPv4
  ///< address and/or IPv6 prefix length and IPv6 prefix and
  ///< Interface Identifier based on the subscribed values
  ///< received from HSS, if available. The value of PDN Type
  ///< field shall be consistent with the value of the PDN Type IE,
  ///< if present in this message.
  ///< For a Handover to Untrusted Non-3GPP IP Access with
  ///< GTP on S2b, the ePDG shall set the IPv4 address and/or
  ///< IPv6 prefix length and IPv6 prefix and Interface Identifier
  ///< based on the IP address(es) received from the UE.
  ///< If static IP address assignment is not used, and for
  ///< scenarios other than a Handover to Untrusted Non-3GPP
  ///< IP Access with GTP on S2b, the IPv4 address shall be set
  ///< to 0.0.0.0, and/or the IPv6 Prefix Length and IPv6 prefix
  ///< and Interface Identifier shall all be set to zero.
  ///<
  ///< CO: This IE shall be sent by the MME/SGSN on S11/S4
  ///< interface during TAU/RAU/HO with SGW relocation.

  // APN Restriction Maximum_APN_Restriction ///< This IE shall be included on the S4/S11 and S5/S8
  ///< interfaces in the E-UTRAN initial attach, PDP Context
  ///< Activation and UE Requested PDN connectivity
  ///< procedures.
  ///< This IE denotes the most stringent restriction as required
  ///< by any already active bearer context. If there are no
  ///< already active bearer contexts, this value is set to the least
  ///< restrictive type.

  ebi_t              default_ebi;

  ambr_t             ambr;                ///< Aggregate Maximum Bit Rate (APN-AMBR)
  ///< This IE represents the APN-AMBR. It shall be included on
  ///< the S4/S11, S5/S8 and S2b interfaces for an E-UTRAN
  ///< initial attach, UE requested PDN connectivity, the PDP
  ///< Context Activation procedure using S4, the PS mobility
  ///< from the Gn/Gp SGSN to the S4 SGSN/MME procedures,
  ///< Attach with GTP on S2b and a UE initiated Connectivity to
  ///< Additional PDN with GTP on S2b.

  // EBI Linked EPS Bearer ID             ///< This IE shall be included on S4/S11 in RAU/TAU/HO
  ///< except in the Gn/Gp SGSN to MME/S4-SGSN
  ///< RAU/TAU/HO procedures with SGW change to identify the
  ///< default bearer of the PDN Connection

  protocol_configuration_options_t         pco;                 /// PCO protocol_configuration_options
  ///< This IE is not applicable to TAU/RAU/Handover. If
  ///< MME/SGSN receives PCO from UE (during the attach
  ///< procedures), the MME/SGSN shall forward the PCO IE to
  ///< SGW. The SGW shall also forward it to PGW.

  bearer_contexts_to_be_created_t bearer_contexts_to_be_created;    ///< Bearer Contexts to be created
  ///< Several IEs with the same type and instance value shall be
  ///< included on the S4/S11 and S5/S8 interfaces as necessary
  ///< to represent a list of Bearers. One single IE shall be
  ///< included on the S2b interface.
  ///< One bearer shall be included for an E-UTRAN Initial
  ///< Attach, a PDP Context Activation, a UE requested PDN
  ///< Connectivity, an Attach with GTP on S2b, a UE initiated
  ///< Connectivity to Additional PDN with GTP on S2b and a
  ///< Handover to Untrusted Non-3GPP IP Access with GTP on
  ///< S2b.
  ///< One or more bearers shall be included for a
  ///< Handover/TAU/RAU with an SGW change.

  bearer_contexts_to_be_removed_t bearer_contexts_to_be_removed;    ///< This IE shall be included on the S4/S11 interfaces for the
  ///< TAU/RAU/Handover cases where any of the bearers
  ///< existing before the TAU/RAU/Handover procedure will be
  ///< deactivated as consequence of the TAU/RAU/Handover
  ///< procedure.
  ///< For each of those bearers, an IE with the same type and
  ///< instance value shall be included.

  // Trace Information trace_information  ///< This IE shall be included on the S4/S11 interface if an
  ///< SGW trace is activated, and/or on the S5/S8 and S2b
  ///< interfaces if a PGW trace is activated. See 3GPP TS
  ///< 32.422 [18].

  // Recovery Recovery                    ///< This IE shall be included on the S4/S11, S5/S8 and S2b
  ///< interfaces if contacting the peer for the first time

  FQ_CSID_t          mme_fq_csid;         ///< This IE shall be included by the MME on the S11 interface
  ///< and shall be forwarded by an SGW on the S5/S8 interfaces
  ///< according to the requirements in 3GPP TS 23.007 [17].

  FQ_CSID_t          sgw_fq_csid;         ///< This IE shall included by the SGW on the S5/S8 interfaces
  ///< according to the requirements in 3GPP TS 23.007 [17].

  //FQ_CSID_t          epdg_fq_csid;      ///< This IE shall be included by the ePDG on the S2b interface
  ///< according to the requirements in 3GPP TS 23.007 [17].

  UETimeZone_t       ue_time_zone;        ///< This IE shall be included by the MME over S11 during
  ///< Initial Attach, UE Requested PDN Connectivity procedure.
  ///< This IE shall be included by the SGSN over S4 during PDP
  ///< Context Activation procedure.
  ///< This IE shall be included by the MME/SGSN over S11/S4
  ///< TAU/RAU/Handover with SGW relocation.
  ///< C: If SGW receives this IE, SGW shall forward it to PGW
  ///< across S5/S8 interface.

  UCI_t              uci;                 ///< User CSG Information
  ///< CO This IE shall be included on the S4/S11 interface for E-
  ///< UTRAN Initial Attach, UE-requested PDN Connectivity and
  ///< PDP Context Activation using S4 procedures if the UE is
  ///< accessed via CSG cell or hybrid cell. The MME/SGSN
  ///< shall also include it for TAU/RAU/Handover procedures if
  ///< the PGW has requested CSG info reporting and
  ///< MME/SGSN support CSG info reporting. The SGW shall
  ///< include this IE on S5/S8 if it receives the User CSG
  ///< information from MME/SGSN.

  // Charging Characteristics
  // MME/S4-SGSN LDN
  // SGW LDN
  // ePDG LDN
  // Signalling Priority Indication
  // MMBR Max MBR/APN-AMBR
  // Private Extension

  /* S11 stack specific parameter. Not used in standalone epc mode */
  void              *trxn;                ///< Transaction identifier
  struct in_addr     peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME
  uint16_t           peer_port;           ///< MME port for S-GW or S-GW port for MME
} itti_s11_create_session_request_t;


//-----------------------------------------------------------------------------
/** @struct itti_s11_create_session_response_t
 *  @brief Create Session Response
 *
 * The Create Session Response will be sent on S11 interface as
 * part of these procedures:
 * - E-UTRAN Initial Attach
 * - UE requested PDN connectivity
 * - Tracking Area Update procedure with SGW change
 * - S1/X2-based handover with SGW change
 */
typedef struct itti_s11_create_session_response_s {
  uint64_t           ie_presence_mask;
#define S11_CREATE_SESSION_RESPONSE_PR_IE_CAUSE                              ((uint64_t)1)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_CHANGE_REPORTING_ACTION            ((uint64_t)1 << 1)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_CSG_INFORMATION_REPORTING_ACTION   ((uint64_t)1 << 2)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_HENB_INFORMATION_REPORTING         ((uint64_t)1 << 3)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_SENDER_FTEID_FOR_CONTROL_PLANE     ((uint64_t)1 << 4)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_PGW_S5S8_FTEID_FOR_CONTROL_PLANE   ((uint64_t)1 << 5)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_PAA                                ((uint64_t)1 << 6)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_APN_RESTRICTION                    ((uint64_t)1 << 7)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_APN_AMBR                           ((uint64_t)1 << 8)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_LINKED_EPS_BEARER_ID               ((uint64_t)1 << 9)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_PCO                                ((uint64_t)1 << 10)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_BEARER_CONTEXTS_CREATED            ((uint64_t)1 << 11)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_BEARER_CONTEXTS_MARKED_FOR_REMOVAL ((uint64_t)1 << 12)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_RECOVERY                           ((uint64_t)1 << 13)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_CHARGING_GATEWAY_NAME              ((uint64_t)1 << 14)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_CHARGING_GATEWAY_ADDRESS           ((uint64_t)1 << 15)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_PGW_FQ_CSID                        ((uint64_t)1 << 16)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_SGW_FQ_CSID                        ((uint64_t)1 << 17)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_SGW_LDN                            ((uint64_t)1 << 18)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_PGW_LDN                            ((uint64_t)1 << 19)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_PGW_BACK_OFF_TIME                  ((uint64_t)1 << 20)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_APCO                               ((uint64_t)1 << 21)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_TRUSTED_WLAN_IPV4_PARAMETERS       ((uint64_t)1 << 22)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_INDICATION_FLAGS                   ((uint64_t)1 << 23)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_PRESENCE_REPORTING_AREA_ACTION     ((uint64_t)1 << 24)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_PGW_NODE_LEVEL_LOAD_CONTROL_INFORMATION ((uint64_t)1 << 25)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_PGW_APN_LEVEL_LOAD_CONTROL_INFORMATION  ((uint64_t)1 << 26)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_SGW_NODE_LEVEL_LOAD_CONTROL_INFORMATION ((uint64_t)1 << 27)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_PGW_OVERLOAD_CONTROL_INFORMATION   ((uint64_t)1 << 28)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_SGW_OVERLOAD_CONTROL_INFORMATION   ((uint64_t)1 << 29)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_NBIFOM_CONTAINER                   ((uint64_t)1 << 30)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_PDN_CONNECTION_CHARGING_ID         ((uint64_t)1 << 31)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_EPCO                               ((uint64_t)1 << 32)
#define S11_CREATE_SESSION_RESPONSE_PR_IE_PRIVATE_EXTENSION                  ((uint64_t)1 << 33)

  teid_t                   teid;                ///< Tunnel Endpoint Identifier

  // here fields listed in 3GPP TS 29.274
  gtpv2c_cause_t            cause;               ///< If the SGW cannot accept any of the "Bearer Context Created" IEs within Create Session Request
  ///< message, the SGW shall send the Create Session Response with appropriate reject Cause value.

  // change_reporting_action                    ///< This IE shall be included on the S5/S8 and S4/S11
  ///< interfaces with the appropriate Action field if the location
  ///< Change Reporting mechanism is to be started or stopped
  ///< for this subscriber in the SGSN/MME.

  // csg_Information_reporting_action           ///< This IE shall be included on the S5/S8 and S4/S11
  ///< interfaces with the appropriate Action field if the CSG Info
  ///< reporting mechanism is to be started or stopped for this
  ///< subscriber in the SGSN/MME.

  fteid_t                  s11_sgw_fteid;        ///< Sender F-TEID for control plane
  ///< This IE shall be sent on the S11/S4 interfaces. For the
  ///< S5/S8/S2b interfaces it is not needed because its content
  ///< would be identical to the IE PGW S5/S8/S2b F-TEID for
  ///< PMIP based interface or for GTP based Control Plane
  ///< interface.

  fteid_t                  s5_s8_pgw_fteid;      ///< PGW S5/S8/S2b F-TEID for PMIP based interface or for GTP based Control Plane interface
  ///< PGW shall include this IE on the S5/S8 interfaces during
  ///< the Initial Attach, UE requested PDN connectivity and PDP
  ///< Context Activation procedures.
  ///< If SGW receives this IE it shall forward the IE to MME/S4-
  ///< SGSN on S11/S4 interface.
  ///< This IE shall include the TEID in the GTP based S5/S8
  ///< case and the GRE key in the PMIP based S5/S8 case.
  ///< In PMIP based S5/S8 case, same IP address is used for
  ///< both control plane and the user plane communication.
  ///<
  ///< PGW shall include this IE on the S2b interface during the
  ///< Attach with GTP on S2b, UE initiated Connectivity to
  ///< Additional PDN with GTP on S2b and Handover to
  ///< Untrusted Non-3GPP IP Access with GTP on S2b
  ///< procedures.


  paa_t                    paa;                 ///< PDN Address Allocation
  ///< This IE shall be included on the S5/S8, S4/S11 and S2b
  ///< interfaces for the E-UTRAN initial attach, PDP Context
  ///< Activation, UE requested PDN connectivity, Attach with
  ///< GTP on S2b, UE initiated Connectivity to Additional PDN
  ///< with GTP on S2b and Handover to Untrusted Non-3GPP IP
  ///< Access with GTP on S2b procedures.
  ///< The PDN type field in the PAA shall be set to IPv4, or IPv6
  ///< or IPv4v6 by the PGW.
  ///< For the interfaces other than S2b, if the DHCPv4 is used
  ///< for IPv4 address allocation, the IPv4 address field shall be
  ///< set to 0.0.0.0.

  APNRestriction_t         apn_restriction;     ///< This IE shall be included on the S5/S8 and S4/S11
  ///< interfaces in the E-UTRAN initial attach, PDP Context
  ///< Activation and UE Requested PDN connectivity
  ///< procedures.
  ///< This IE shall also be included on S4/S11 during the Gn/Gp
  ///< SGSN to S4 SGSN/MME RAU/TAU procedures.
  ///< This IE denotes the restriction on the combination of types
  ///< of APN for the APN associated with this EPS bearer
  ///< Context.

  ambr_t             apn_ambr;                  ///< Aggregate Maximum Bit Rate (APN-AMBR)
  ///< This IE represents the APN-AMBR. It shall be included on
  ///< the S5/S8, S4/S11 and S2b interfaces if the received APN-
  ///< AMBR has been modified by the PCRF.

  // EBI Linked EPS Bearer ID                   ///< This IE shall be sent on the S4/S11 interfaces during
  ///< Gn/Gp SGSN to S4-SGSN/MME RAU/TAU procedure to
  ///< identify the default bearer the PGW selects for the PDN
  ///< Connection.

  protocol_configuration_options_t         pco;// PCO protocol_configuration_options
  ///< This IE is not applicable for TAU/RAU/Handover. If PGW
  ///< decides to return PCO to the UE, PGW shall send PCO to
  ///< SGW. If SGW receives the PCO IE, SGW shall forward it
  ///< MME/SGSN.

  bearer_contexts_created_t bearer_contexts_created;///< EPS bearers corresponding to Bearer Contexts sent in
  ///< request message. Several IEs with the same type and
  ///< instance value may be included on the S5/S8 and S4/S11
  ///< as necessary to represent a list of Bearers. One single IE
  ///< shall be included on the S2b interface.
  ///< One bearer shall be included for E-UTRAN Initial Attach,
  ///< PDP Context Activation or UE Requested PDN
  ///< Connectivity , Attach with GTP on S2b, UE initiated
  ///< Connectivity to Additional PDN with GTP on S2b, and
  ///< Handover to Untrusted Non-3GPP IP Access with GTP on
  ///< S2b.
  ///< One or more created bearers shall be included for a
  ///< Handover/TAU/RAU with an SGW change. See NOTE 2.

  bearer_contexts_marked_for_removal_t bearer_contexts_marked_for_removal; ///< EPS bearers corresponding to Bearer Contexts to be
  ///< removed that were sent in the Create Session Request
  ///< message.
  ///< For each of those bearers an IE with the same type and
  ///< instance value shall be included on the S4/S11 interfaces.

  // Recovery Recovery                          ///< This IE shall be included on the S4/S11, S5/S8 and S2b
  ///< interfaces if contacting the peer for the first time

  // FQDN charging_Gateway_name                 ///< When Charging Gateway Function (CGF) Address is
  ///< configured, the PGW shall include this IE on the S5
  ///< interface.
  ///< NOTE 1: Both Charging Gateway Name and Charging Gateway Address shall not be included at the same
  ///< time. When both are available, the operator configures a preferred value.

  // IP Address charging_Gateway_address        ///< When Charging Gateway Function (CGF) Address is
  ///< configured, the PGW shall include this IE on the S5
  ///< interface. See NOTE 1.


  FQ_CSID_t                pgw_fq_csid;         ///< This IE shall be included by the PGW on the S5/S8 and
  ///< S2b interfaces and, when received from S5/S8 be
  ///< forwarded by the SGW on the S11 interface according to
  ///< the requirements in 3GPP TS 23.007 [17].

  FQ_CSID_t                sgw_fq_csid;         ///< This IE shall be included by the SGW on the S11 interface
  ///< according to the requirements in 3GPP TS 23.007 [17].

  // Local Distinguished Name (LDN) SGW LDN     ///< This IE is optionally sent by the SGW to the MME/SGSN
  ///< on the S11/S4 interfaces (see 3GPP TS 32.423 [44]),
  ///< when contacting the peer node for the first time.
  ///< Also:
  ///< This IE is optionally sent by the SGW to the MME/SGSN
  ///< on the S11/S4 interfaces (see 3GPP TS 32.423 [44]),
  ///< when communicating the LDN to the peer node for the first
  ///< time.

  // Local Distinguished Name (LDN) PGW LDN     ///< This IE is optionally included by the PGW on the S5/S8
  ///< and S2b interfaces (see 3GPP TS 32.423 [44]), when
  ///< contacting the peer node for the first time.
  ///< Also:
  ///< This IE is optionally included by the PGW on the S5/S8
  ///< interfaces (see 3GPP TS 32.423 [44]), when
  ///< communicating the LDN to the peer node for the first time.

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
  struct in_addr           peer_ip;            ///< MME ipv4 address
} itti_s11_create_session_response_t;

//-----------------------------------------------------------------------------
/** @struct itti_s11_create_bearer_request_t
 *  @brief Create Bearer Request
 *
 * The direction of this message shall be from PGW to SGW and from SGW to MME/S4-SGSN, and from PGW to ePDG
 * The Create Bearer Request message shall be sent on the S5/S8 interface by the PGW to the SGW and on the S11
 * interface by the SGW to the MME as part of the Dedicated Bearer Activation procedure.
 * The message shall also be sent on the S5/S8 interface by the PGW to the SGW and on the S4 interface by the SGW to
 * the SGSN as part of the Secondary PDP Context Activation procedure or the Network Requested Secondary PDP
 * Context Activation procedure.
 * The message shall also be sent on the S2b interface by the PGW to the ePDG as part of the Dedicated S2b bearer
 * activation with GTP on S2b.
 */
typedef struct itti_s11_create_bearer_request_s {
  teid_t                     local_teid;       ///< not in specs for inner use

  teid_t                     teid;             ///< S11 SGW Tunnel Endpoint Identifier

  pti_t                      pti; ///< C: This IE shall be sent on the S5/S8 and S4/S11 interfaces
  ///< when the procedure was initiated by a UE Requested
  ///< Bearer Resource Modification Procedure or UE Requested
  ///< Bearer Resource Allocation Procedure (see NOTE 1) or
  ///< Secondary PDP Context Activation Procedure.
  ///< The PTI shall be the same as the one used in the
  ///< corresponding Bearer Resource Command.

  ebi_t                      linked_eps_bearer_id; ///< M: This IE shall be included to indicate the default bearer
  ///< associated with the PDN connection.

  protocol_configuration_options_t pco;///< O: This IE may be sent on the S5/S8 and S4/S11 interfaces

  bearer_contexts_to_be_created_t bearer_contexts;    ///< M: Several IEs with this type and instance values shall be
  ///< included as necessary to represent a list of Bearers.

  FQ_CSID_t                  pgw_fq_csid;       ///< C: This IE shall be included by MME on S11 and shall be
  ///< forwarded by SGW on S5/S8 according to the
  ///< requirements in 3GPP TS 23.007 [17].

  FQ_CSID_t                  sgw_fq_csid;       ///< C:This IE shall be included by the SGW on the S11 interface
  ///< according to the requirements in 3GPP TS 23.007 [17].

  //Change Reporting Action ///< This IE shall be included on the S5/S8 and S4/S11
  ///< interfaces with the appropriate Action field If the location
  ///< Change Reporting mechanism is to be started or stopped
  ///< for this subscriber in the SGSN/MME.

  //CSG Information ///< This IE shall be included on the S5/S8 and S4/S11
  ///< interfaces with the appropriate Action field if the CSG Info Reporting Action
  ///< reporting mechanism is to be started or stopped for this
  ///< subscriber in the SGSN/MME.

  // Private Extension   Private Extension

  /* GTPv2-C specific parameters */
  void                      *trxn;                        ///< Transaction identifier
  struct in_addr             peer_ip;
} itti_s11_create_bearer_request_t;

//-----------------------------------------------------------------------------
/** @struct itti_s11_create_bearer_response_t
 *  @brief Create Bearer Response
 *
 * The Create Bearer Response message shall be sent on the S5/S8 interface by the SGW to the PGW, and on the S11
 * interface by the MME to the SGW as part of the Dedicated Bearer Activation procedure.
 * The message shall also be sent on the S5/S8 interface by the SGW to the PGW and on the S4 interface by the SGSN to
 * the SGW as part of Secondary PDP Context Activation procedure or the Network Requested Secondary PDP Context
 * Activation procedure.
 * The message shall also be sent on the S2b interface by the ePDG to the PGW as part of the Dedicated S2b bearer
 * activation with GTP on S2b.
 * Possible Cause values are specified in Table 8.4-1. Message specific cause values are:
 * - "Request accepted".
 * - "Request accepted partially".
 * - "Context not found".
 * - "Semantic error in the TFT operation".
 * - "Syntactic error in the TFT operation".
 * - "Semantic errors in packet filter(s)".
 * - "Syntactic errors in packet filter(s)".
 * - "Service not supported".
 * - "Unable to page UE".
 * - "UE not responding".
 * - "Unable to page UE due to Suspension".
 * - "UE refuses".
 * - "Denied in RAT".
 * - "UE context without TFT already activated".
 */
typedef struct itti_s11_create_bearer_response_s {
  teid_t                   local_teid;       ///< not in specs for inner MME use
  teid_t                   teid;                ///< S11 MME Tunnel Endpoint Identifier

  // here fields listed in 3GPP TS 29.274
  gtpv2c_cause_t            cause;               ///< M

  bearer_contexts_within_create_bearer_response_t bearer_contexts;///< Several IEs with this type and instance value shall be
  ///< included on the S4/S11, S5/S8 and S2b interfaces as
  ///< necessary to represent a list of Bearers.

  // Recovery   C This IE shall be included on the S4/S11, S5/S8 and S2b interfaces if contacting the peer for the first time

  FQ_CSID_t                mme_fq_csid;         ///< C This IE shall be included by the MME on the S11
  ///< interface and shall be forwarded by the SGW on the S5/S8
  ///< interfaces according to the requirements in 3GPP TS
  ///< 23.007 [17].

  FQ_CSID_t                sgw_fq_csid;         ///< C This IE shall be included by the MME on the S11
  ///< interface and shall be forwarded by the SGW on the S5/S8
  ///< interfaces according to the requirements in 3GPP TS
  ///< 23.007 [17].

  FQ_CSID_t                epdg_fq_csid;         ///< C This IE shall be included by the ePDG on the S2b interface
  ///< according to the requirements in 3GPP TS 23.007 [17].

  protocol_configuration_options_t pco;///< C: If the UE includes the PCO IE, then the MME/SGSN shall
  ///< copy the content of this IE transparently from the PCO IE
  ///< included by the UE. If the SGW receives PCO from
  ///< MME/SGSN, SGW shall forward it to the PGW.

  UETimeZone_t             ue_time_zone;      ///< O: This IE is optionally included by the MME on the S11
  ///< interface or by the SGSN on the S4 interface.
  ///< CO: The SGW shall forward this IE on the S5/S8 interface if the
  ///< SGW supports this IE and it receives it from the
  ///< MME/SGSN.

  Uli_t                    uli;              ///< O: This IE is optionally included by the MME on the S11
  ///< interface or by the SGSN on the S4 interface.
  ///< CO: The SGW shall forward this IE on the S5/S8 interface if the
  ///< SGW supports this IE and it receives it from the
  ///< MME/SGSN.

  // Private Extension Private Extension        ///< optional

  /* S11 stack specific parameter. Not used in standalone epc mode */
  void                    *trxn;                      ///< Transaction identifier
} itti_s11_create_bearer_response_t;


//-----------------------------------------------------------------------------
/** @struct itti_s11_modify_bearer_request_t
 *  @brief Modify Bearer Request
 *
 * The Modify Bearer Request will be sent on S11 interface as
 * part of these procedures:
 * - E-UTRAN Tracking Area Update without SGW Change
 * - UE triggered Service Request
 * - S1-based Handover
 * - E-UTRAN Initial Attach
 * - UE requested PDN connectivity
 * - X2-based handover without SGWrelocation
 */
typedef struct itti_s11_modify_bearer_request_s {
  uint64_t           ie_presence_mask;
#define S11_MODIFY_BEARER_REQUEST_PR_IE_MEI                                 ((uint64_t)1)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_ULI                                 ((uint64_t)1 << 1)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_SERVING_NETWORK                     ((uint64_t)1 << 2)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_RAT_TYPE                            ((uint64_t)1 << 3)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_INDICATION_FLAGS                    ((uint64_t)1 << 4)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_SENDER_FTEID_FOR_CONTROL_PLANE      ((uint64_t)1 << 5)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_APN_AMBR                            ((uint64_t)1 << 6)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_DELAY_DOWNLINK_PACKET_NOTIFICATION_REQUEST ((uint64_t)1 << 7)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_BEARER_CONTEXTS_TO_BE_MODIFIED      ((uint64_t)1 << 8)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_BEARER_CONTEXTS_TO_BE_REMOVED       ((uint64_t)1 << 9)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_RECOVERY                            ((uint64_t)1 << 10)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_UE_TIME_ZONE                        ((uint64_t)1 << 11)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_MME_FQ_CSID                         ((uint64_t)1 << 12)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_SGW_FQ_CSID                         ((uint64_t)1 << 13)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_UCI                                 ((uint64_t)1 << 14)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_UE_LOCAL_IP_ADDRESS                 ((uint64_t)1 << 15)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_UE_UDP_PORT                         ((uint64_t)1 << 16)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_MME_S4_SGSN_LDN                     ((uint64_t)1 << 17)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_SGW_LDN                             ((uint64_t)1 << 18)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_HENB_LOCAL_IP_ADDRESS               ((uint64_t)1 << 19)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_HENB_UDP_PORT                       ((uint64_t)1 << 20)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_MME_S4_SGSN_IDENTIFIER              ((uint64_t)1 << 21)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_CN_OPERATOR_SELECTION_ENTITY        ((uint64_t)1 << 22)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_PRESENCE_REPORTING_AREA_INFORMATION ((uint64_t)1 << 23)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_MMES4_SGSN_OVERLOAD_CONTROL_INFORMATION ((uint64_t)1 << 24)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_SGW_OVERLOAD_CONTROL_INFORMATION    ((uint64_t)1 << 25)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_EPDG_OVERLOAD_CONTROL_INFORMATION   ((uint64_t)1 << 26)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_SERVING_PLMN_RATE_CONTROL           ((uint64_t)1 << 27)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_MO_EXCEPTION_DATA_COUNTER           ((uint64_t)1 << 28)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_IMSI                                ((uint64_t)1 << 29)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_ULI_FOR_SGW                         ((uint64_t)1 << 30)
#define S11_MODIFY_BEARER_REQUEST_PR_IE_PRIVATE_EXTENSION                   ((uint64_t)1 << 61)
  teid_t                     local_teid;       ///< not in specs for inner MME use

  teid_t                     teid;             ///< S11 SGW Tunnel Endpoint Identifier

  // MEI                    ME Identity (MEI)  ///< C:This IE shall be sent on the S5/S8 interfaces for the Gn/Gp
  ///< SGSN to MME TAU.

  Uli_t                      uli;              ///< C: The MME/SGSN shall include this IE for
  ///< TAU/RAU/Handover procedures if the PGW has requested
  ///< location information change reporting and MME/SGSN
  ///< support location information change reporting.
  ///< An MME/SGSN which supports location information
  ///< change shall include this IE for UE-initiated Service
  ///< Request procedure if the PGW has requested location
  ///< information change reporting and the UE’s location info
  ///< has changed.
  ///< The SGW shall include this IE on S5/S8 if it receives the
  ///< ULI from MME/SGSN.
  ///< CO:This IE shall also be included on the S4/S11 interface for a
  ///< TAU/RAU/Handover with MME/SGSN change without
  ///< SGW change procedure, if the level of support (User
  ///< Location Change Reporting and/or CSG Information
  ///< Change Reporting) changes the MME shall include the
  ///< ECGI/TAI in the ULI, the SGSN shall include the CGI/SAI
  ///< in the ULI.
  ///< The SGW shall include this IE on S5/S8 if it receives the
  ///< ULI from MME/SGSN.

  ServingNetwork_t           serving_network;  ///< CO:This IE shall be included on S11/S4 interface during the
  ///< following procedures:
  ///< - TAU/RAU/handover if Serving Network is changed.
  ///< - TAU/RAU when the UE was ISR activated which is
  ///<   indicated by ISRAU flag.
  ///< - UE triggered Service Request when UE is ISR
  ///<   activated.
  ///< - UE initiated Service Request if ISR is not active, but
  ///<   the Serving Network has changed during previous
  ///<   mobility procedures, i.e. intra MME/S4-SGSN
  ///<   TAU/RAU and the change has not been reported to
  ///<   the PGW yet.
  ///< - TAU/RAU procedure as part of the optional network
  ///<   triggered service restoration procedure with ISR, as
  ///<   specified by 3GPP TS 23.007 [17].
  ///<
  ///< CO:This IE shall be included on S5/S8 if the SGW receives this
  ///< IE from MME/SGSN and if ISR is not active.
  ///< This IE shall be included on S5/S8 if the SGW receives this
  ///< IE from MME/SGSN and ISR is active and the Modify
  ///< Bearer Request message needs to be sent to the PGW as
  ///< specified in the 3GPP TS 23.401 [3].

  rat_type_t                 rat_type;         ///< C: This IE shall be sent on the S11 interface for a TAU with
  ///< an SGSN interaction, UE triggered Service Request or an I-
  ///< RAT Handover.
  ///< This IE shall be sent on the S4 interface for a RAU with
  ///< MME interaction, a RAU with an SGSN change, a UE
  ///< Initiated Service Request or an I-RAT Handover.
  ///< This IE shall be sent on the S5/S8 interface if the RAT type
  ///< changes.
  ///< CO: CO If SGW receives this IE from MME/SGSN during a
  ///< TAU/RAU/Handover with SGW change procedure, the
  ///< SGW shall forward it across S5/S8 interface to PGW.
  ///< CO: The IE shall be sent on the S11/S4 interface during the
  ///< following procedures:
  ///< - an inter MM TAU or inter SGSN RAU when UE was
  ///<   ISR activated which is indicated by ISRAU flag.
  ///< - TAU/RAU procedure as part of optional network
  ///<   triggered service restoration procedure with ISR, as
  ///<   specified by 3GPP TS 23.007 [17].
  ///< If ISR is active, this IE shall also be included on the S11
  ///< interface in the S1-U GTP-U tunnel setup procedure during
  ///< an intra-MME intra-SGW TAU procedure.

  indication_flags_t         indication_flags; ///< C:This IE shall be included if any one of the applicable flags
  ///< is set to 1.
  ///< Applicable flags are:
  ///< -ISRAI: This flag shall be used on S4/S11 interface
  ///<   and set to 1 if the ISR is established between the
  ///<   MME and the S4 SGSN.
  ///< - Handover Indication: This flag shall be set to 1 on
  ///<   the S4/S11 and S5/S8 interfaces during an E-
  ///<   UTRAN Initial Attach or for a UE Requested PDN
  ///<   Connectivity or a PDP Context Activation
  ///<   procedure, if the PDN connection/PDP context is
  ///<   handed-over from non-3GPP access.
  ///< - Direct Tunnel Flag: This flag shall be used on the
  ///<   S4 interface and set to 1 if Direct Tunnel is used.
  ///< - Change Reporting support Indication: shall be
  ///<   used on S4/S11, S5/S8 and set if the SGSN/MME
  ///<   supports location Info Change Reporting. This flag
  ///<   should be ignored by SGW if no message is sent
  ///<   on S5/S8. See NOTE 4.
  ///< - CSG Change Reporting Support Indication: shall
  ///<   be used on S4/S11, S5/S8 and set if the
  ///<   SGSN/MME supports CSG Information Change
  ///<   Reporting. This flag shall be ignored by SGW if no
  ///<   message is sent on S5/S8. See NOTE 4.
  ///< - Change F-TEID support Indication: This flag shall
  ///<   be used on S4/S11 for an IDLE state UE initiated
  ///<   TAU/RAU procedure and set to 1 to allow the
  ///<   SGW changing the GTP-U F-TEID.

  fteid_t                  sender_fteid_for_cp; ///< C: Sender F-TEID for control plane
  ///< This IE shall be sent on the S11 and S4 interfaces for a
  ///< TAU/RAU/ Handover with MME/SGSN change and without
  ///< any SGW change.
  ///< This IE shall be sent on the S5 and S8 interfaces for a
  ///< TAU/RAU/Handover with a SGW change.

  ambr_t                   apn_ambr;            ///< C: Aggregate Maximum Bit Rate (APN-AMBR)
  ///< The APN-AMBR shall be sent for the PS mobility from the
  ///< Gn/Gp SGSN to the S4 SGSN/MME procedures..

  /* Delay Value in integer multiples of 50 millisecs, or zero */
  DelayValue_t               delay_dl_packet_notif_req; ///<C:This IE shall be sent on the S11 interface for a UE
  ///< triggered Service Request.
  ///< CO: This IE shall be sent on the S4 interface for a UE triggered
  ///< Service Request.

  bearer_contexts_to_be_modified_t bearer_contexts_to_be_modified;///< C: This IE shall be sent on the S4/S11 interface and S5/S8
  ///< interface except on the S5/S8 interface for a UE triggered
  ///< Service Request.
  ///< When Handover Indication flag is set to 1 (i.e., for
  ///< EUTRAN Initial Attach or UE Requested PDN Connectivity
  ///< when the UE comes from non-3GPP access), the PGW
  ///< shall ignore this IE. See NOTE 1.
  ///< Several IEs with the same type and instance value may be
  ///< included as necessary to represent a list of Bearers to be
  ///< modified.
  ///< During a TAU/RAU/Handover procedure with an SGW
  ///< change, the SGW includes all bearers it received from the
  ///< MME/SGSN (Bearer Contexts to be created, or Bearer
  ///< Contexts to be modified and also Bearer Contexts to be
  ///< removed) into the list of 'Bearer Contexts to be modified'
  ///< IEs, which are then sent on the S5/S8 interface to the
  ///< PGW (see NOTE 2).

  bearer_contexts_to_be_removed_t bearer_contexts_to_be_removed;    ///< C: This IE shall be included on the S4 and S11 interfaces for
  ///< the TAU/RAU/Handover and Service Request procedures
  ///< where any of the bearers existing before the
  ///< TAU/RAU/Handover procedure and Service Request
  ///< procedures will be deactivated as consequence of the
  ///< TAU/RAU/Handover procedure and Service Request
  ///< procedures. (NOTE 3)
  ///< For each of those bearers, an IE with the same type and
  ///< instance value, shall be included.

  // recovery_t(restart counter) recovery;      ///< C: This IE shall be included if contacting the peer for the first
  ///< time.

  UETimeZone_t               ue_time_zone;      ///< CO: This IE shall be included by the MME/SGSN on the S11/S4
  ///< interfaces if the UE Time Zone has changed in the case of
  ///< TAU/RAU/Handover.
  ///< C: If SGW receives this IE, SGW shall forward it to PGW
  ///< across S5/S8 interface.

  FQ_CSID_t                  mme_fq_csid;       ///< C: This IE shall be included by MME on S11 and shall be
  ///< forwarded by SGW on S5/S8 according to the
  ///< requirements in 3GPP TS 23.007 [17].

  FQ_CSID_t                  sgw_fq_csid;       ///< C: This IE shall be included by SGW on S5/S8 according to
  ///< the requirements in 3GPP TS 23.007 [17].

  UCI_t                      uci;               ///< CO: The MME/SGSN shall include this IE for
  ///< TAU/RAU/Handover procedures and UE-initiated Service
  ///< Request procedure if the PGW has requested CSG Info
  ///< reporting and the MME/SGSN support the CSG
  ///< information reporting. The SGW shall include this IE on
  ///< S5/S8 if it receives the User CSG Information from
  ///< MME/SGSN.

  // Local Distinguished Name (LDN) MME/S4-SGSN LDN ///< O: This IE is optionally sent by the MME to the SGW on the
  ///< S11 interface and by the SGSN to the SGW on the S4
  ///< interface (see 3GPP TS 32.423 [44]), when communicating
  ///< the LDN to the peer node for the first time.

  // Local Distinguished Name (LDN) SGW LDN     ///< O: This IE is optionally sent by the SGW to the PGW on the
  ///< S5/S8 interfaces (see 3GPP TS 32.423 [44]), for inter-
  ///< SGW mobity, when communicating the LDN to the peer
  ///< node for the first time.

  // MMBR           Max MBR/APN-AMBR            ///< CO: If the S4-SGSN supports Max MBR/APN-AMBR, this IE
  ///< shall be included by the S4-SGSN over S4 interface in the
  ///< following cases:
  ///< - during inter SGSN RAU/SRNS relocation without
  ///<   SGW relocation and inter SGSN SRNS relocation
  ///<   with SGW relocation if Higher bitrates than
  ///<   16 Mbps flag is not included in the MM Context IE
  ///<   in the Context Response message or in the MM
  ///<   Context IE in the Forward Relocation Request
  ///<   message from the old S4-SGSN, while it is
  ///<   received from target RNC or a local Max
  ///<   MBR/APN-AMBR is configured based on
  ///<   operator's policy.
  ///<   - during Service Request procedure if Higher
  ///<   bitrates than 16 Mbps flag is received but the S4-
  ///<   SGSN has not received it before from an old RNC
  ///<   or the S4-SGSN has not updated the Max
  ///<   MBR/APN-AMBR to the PGW yet.
  ///< If SGW receives this IE, SGW shall forward it to PGW
  ///< across S5/S8 interface.

  // Private Extension   Private Extension

  /* GTPv2-C specific parameters */
  void                      *trxn;                        ///< Transaction identifier
  struct in_addr             peer_ip;
} itti_s11_modify_bearer_request_t;

//-----------------------------------------------------------------------------
/** @struct itti_s11_modify_bearer_response_t
 *  @brief Modify Bearer Response
 *
 * The Modify Bearer Response will be sent on S11 interface as
 * part of these procedures:
 * - E-UTRAN Tracking Area Update without SGW Change
 * - UE triggered Service Request
 * - S1-based Handover
 * - E-UTRAN Initial Attach
 * - UE requested PDN connectivity
 * - X2-based handover without SGWrelocation
 */
typedef struct itti_s11_modify_bearer_response_s {
  teid_t                   teid;                ///< S11 MME Tunnel Endpoint Identifier

  // here fields listed in 3GPP TS 29.274
  gtpv2c_cause_t            cause;               ///<

  ebi_t                    linked_eps_bearer_id;///< This IE shall be sent on S5/S8 when the UE moves from a
  ///< Gn/Gp SGSN to the S4 SGSN or MME to identify the
  ///< default bearer the PGW selects for the PDN Connection.
  ///< This IE shall also be sent by SGW on S11, S4 during
  ///< Gn/Gp SGSN to S4-SGSN/MME HO procedures to identify
  ///< the default bearer the PGW selects for the PDN
  ///< Connection.

  ambr_t                   apn_ambr;            ///< Aggregate Maximum Bit Rate (APN-AMBR)
  ///< This IE shall be included in the PS mobility from Gn/Gp
  ///< SGSN to the S4 SGSN/MME procedures if the received
  ///< APN-AMBR has been modified by the PCRF.

  APNRestriction_t         apn_restriction;     ///< This IE denotes the restriction on the combination of types
  ///< of APN for the APN associated with this EPS bearer
  ///< Context. This IE shall be included over S5/S8 interfaces,
  ///< and shall be forwarded over S11/S4 interfaces during
  ///< Gn/Gp SGSN to MME/S4-SGSN handover procedures.
  ///< This IE shall also be included on S5/S8 interfaces during
  ///< the Gn/Gp SGSN to S4 SGSN/MME RAU/TAU
  ///< procedures.
  ///< The target MME or SGSN determines the Maximum APN
  ///< Restriction using the APN Restriction.
  // PCO protocol_configuration_options         ///< If SGW receives this IE from PGW on GTP or PMIP based
  ///< S5/S8, the SGW shall forward PCO to MME/S4-SGSN
  ///< during Inter RAT handover from the UTRAN or from the
  ///< GERAN to the E-UTRAN. See NOTE 2:
  ///< If MME receives the IE, but no NAS message is sent, MME discards the IE.

  bearer_contexts_modified_t bearer_contexts_modified;///< EPS bearers corresponding to Bearer Contexts to be
  ///< modified that were sent in Modify Bearer Request
  ///< message. Several IEs with the same type and instance
  ///< value may be included as necessary to represent a list of
  ///< the Bearers which are modified.

  bearer_contexts_marked_for_removal_t bearer_contexts_marked_for_removal; ///< EPS bearers corresponding to Bearer Contexts to be
  ///< removed sent in the Modify Bearer Request message.
  ///< Shall be included if request message contained Bearer
  ///< Contexts to be removed.
  ///< For each of those bearers an IE with the same type and
  ///< instance value shall be included.

  // change_reporting_action                    ///< This IE shall be included with the appropriate Action field If
  ///< the location Change Reporting mechanism is to be started
  ///< or stopped for this subscriber in the SGSN/MME.

  // csg_Information_reporting_action           ///< This IE shall be included with the appropriate Action field if
  ///< the location CSG Info change reporting mechanism is to be
  ///< started or stopped for this subscriber in the SGSN/MME.

  // FQDN Charging Gateway Name                 ///< When Charging Gateway Function (CGF) Address is
  ///< configured, the PGW shall include this IE on the S5
  ///< interface during SGW relocation and when the UE moves
  ///< from Gn/Gp SGSN to S4-SGSN/MME. See NOTE 1:
  ///< Both Charging Gateway Name and Charging Gateway Address shall not be included at the same
  ///< time. When both are available, the operator configures a preferred value.

  // IP Address Charging Gateway Address        ///< When Charging Gateway Function (CGF) Address is
  ///< configured, the PGW shall include this IE on the S5
  ///< interface during SGW relocation and when the UE moves
  ///< from Gn/Gp SGSN to S4-SGSN/MME. See NOTE 1:
  ///< Both Charging Gateway Name and Charging Gateway Address shall not be included at the same
  ///< time. When both are available, the operator configures a preferred value.

  FQ_CSID_t                pgw_fq_csid;         ///< This IE shall be included by PGW on S5/S8and shall be
  ///< forwarded by SGW on S11 according to the requirements
  ///< in 3GPP TS 23.007 [17].

  FQ_CSID_t                sgw_fq_csid;         ///< This IE shall be included by SGW on S11 according to the
  ///< requirements in 3GPP TS 23.007 [17].

  // recovery_t(restart counter) recovery;      ///< This IE shall be included if contacting the peer for the first
  ///< time.

  // Local Distinguished Name (LDN) SGW LDN     ///< This IE is optionally sent by the SGW to the MME/SGSN
  ///< on the S11/S4 interfaces (see 3GPP TS 32.423 [44]),
  ///< when contacting the peer node for the first time.

  // Local Distinguished Name (LDN) PGW LDN     ///< This IE is optionally included by the PGW on the S5/S8
  ///< and S2b interfaces (see 3GPP TS 32.423 [44]), when
  ///< contacting the peer node for the first time.

  // Private Extension Private Extension        ///< optional

  /* S11 stack specific parameter. Not used in standalone epc mode */
  void                         *trxn;                      ///< Transaction identifier
} itti_s11_modify_bearer_response_t;

//-----------------------------------------------------------------------------
/** @struct itti_s11_remote_ue_remote_notification_t
 *  @brief Remote UE Report notification
 *
 * The Remote UE Report will be sent on S11 interface as
 * part of these Remote UE connection/disconnection procedure
 */
typedef struct itti_s11_remote_ue_remote_notification_s {
uint64_t           ie_presence_mask;
#define S11_REMOTE_UE_REPORT_NOTIFICATION_PR_IE_REMOTE_UE_CONTEXT_CONNECTED                              ((uint64_t)1)
#define S11_REMOTE_UE_REPORT_NOTIFICATION_PR_IE_REMOTE_UE_CONTEXT_DISCONNECTED                           ((uint64_t)1 << 1)

remote_ue_context_connected_t                remoteuecontext_connected;
remote_ue_context_disconnected_t             remoteuecontext_disconnected; 
 
 } itti_s11_remote_ue_remote_notification_t;


//-----------------------------------------------------------------------------

typedef struct itti_s11_remote_ue_remote_acknowledge_s {
  teid_t      local_teid;             ///< not in specs for inner MME use
  teid_t      teid;                   ///< Tunnel Endpoint Identifier
  //ebi_t       lbi;                    ///< Linked EPS Bearer ID
  //fteid_t     sender_fteid_for_cp;    ///< Sender F-TEID for control plane
  //bool        noDelete;

  gtpv2c_cause_t  cause;

  /* GTPv2-C specific parameters */
  void          *trxn;
  struct in_addr peer_ip;
} itti_s11_remote_ue_remote_acknowledge_t;

//-----------------------------------------------------------------------------
typedef struct itti_s11_delete_session_request_s {
  teid_t      local_teid;             ///< not in specs for inner MME use
  teid_t      teid;                   ///< Tunnel Endpoint Identifier
  ebi_t       lbi;                    ///< Linked EPS Bearer ID
  fteid_t     sender_fteid_for_cp;    ///< Sender F-TEID for control plane
  bool        noDelete;

  /* Operation Indication: This flag shall be set over S4/S11 interface
   * if the SGW needs to forward the Delete Session Request message to
   * the PGW. This flag shall not be set if the ISR associated GTP
   * entity sends this message to the SGW in the Detach procedure.
   * This flag shall also not be set to 1 in the SRNS Relocation Cancel
   * Using S4 (6.9.2.2.4a in 3GPP TS 23.060 [4]), Inter RAT handover
   * Cancel procedure with SGW change TAU with Serving GW change,
   * Gn/Gb based RAU (see 5.5.2.5, 5.3.3.1, D.3.5 in 3GPP TS 23.401 [3],
   * respectively), S1 Based handover Cancel procedure with SGW change.
   */
  indication_flags_t indication_flags;

  /* GTPv2-C specific parameters */
  void          *trxn;
  struct in_addr peer_ip;
} itti_s11_delete_session_request_t;


//-----------------------------------------------------------------------------
/** @struct itti_s11_delete_session_response_t
 *  @brief Delete Session Response
 *
 * The Delete Session Response will be sent on S11 interface as
 * part of these procedures:
 * - EUTRAN Initial Attach
 * - UE, HSS or MME Initiated Detach
 * - UE or MME Requested PDN Disconnection
 * - Tracking Area Update with SGW Change
 * - S1 Based Handover with SGW Change
 * - X2 Based Handover with SGW Relocation
 * - S1 Based handover cancel with SGW change
 */
typedef struct itti_s11_delete_session_response_s {
  teid_t         teid;                ///< Remote Tunnel Endpoint Identifier
  gtpv2c_cause_t  cause;
  //recovery_t recovery;              ///< This IE shall be included on the S5/S8, S4/S11 and S2b
                                      ///< interfaces if contacting the peer for the first time
  protocol_configuration_options_t pco;///< PGW shall include Protocol Configuration Options (PCO)
                                      ///< IE on the S5/S8 interface, if available.
                                      ///< If SGW receives this IE, SGW shall forward it to
                                      ///< SGSN/MME on the S4/S11 interface.

  /* GTPv2-C specific parameters */
  void       *trxn;
  struct in_addr  peer_ip;
} itti_s11_delete_session_response_t;


//-----------------------------------------------------------------------------
/** @struct itti_s11_release_access_bearers_request_t
 *  @brief Release AccessBearers Request
 *
 * The Release Access Bearers Request message shall sent on the S11 interface by
 * the MME to the SGW as part of the S1 release procedure.
 * The message shall also be sent on the S4 interface by the SGSN to the SGW as
 * part of the procedures:
 * -    RAB release using S4
 * -    Iu Release using S4
 * -    READY to STANDBY transition within the network
 */
typedef struct itti_s11_release_access_bearers_request_s {
  teid_t     local_teid;               ///< not in specs for inner MME use
  teid_t     teid;                     ///< Tunnel Endpoint Identifier
  node_type_t originating_node;        ///< This IE shall be sent on S11 interface, if ISR is active in the MME.
                                         ///< This IE shall be sent on S4 interface, if ISR is active in the SGSN
  // Private Extension Private Extension ///< optional
  /* GTPv2-C specific parameters */
  void           *trxn;
  struct in_addr  peer_ip;
} itti_s11_release_access_bearers_request_t;


//-----------------------------------------------------------------------------
/** @struct itti_s11_release_access_bearers_response_t
 *  @brief Release AccessBearers Response
 *
 * The Release Access Bearers Response message is sent on the S11 interface by the SGW to the MME as part of the S1
 * release procedure.
 * The message shall also be sent on the S4 interface by the SGW to the SGSN as part of the procedures:
 * -  RAB release using S4
 * -  Iu Release using S4
 * -  READY to STANDBY transition within the network
 * Possible Cause values are specified in Table 8.4-1. Message specific cause values are:
 * - "Request accepted".
 * - "Request accepted partially".
 * - "Context not found
 */
typedef struct itti_s11_release_access_bearers_response_s {
  teid_t          teid;                   ///< Tunnel Endpoint Identifier
  gtpv2c_cause_t   cause;
  // Recovery           ///< optional This IE shall be included if contacting the peer for the first time
  // Private Extension  ///< optional
  /* GTPv2-C specific parameters */
  void           *trxn;
  struct in_addr  peer_ip;
} itti_s11_release_access_bearers_response_t;

//-----------------------------------------------------------------------------
/** @struct itti_s11_delete_bearer_command_t
 *  @brief Initiate Delete Bearer procedure
 *
 * A Delete Bearer Command message shall be sent on the S11 interface by the MME to the SGW and on the S5/S8
 * interface by the SGW to the PGW as a part of the eNodeB requested bearer release or MME-Initiated Dedicated Bearer
 * Deactivation procedure.
 * The message shall also be sent on the S4 interface by the SGSN to the SGW and on the S5/S8 interface by the SGW to
 * the PGW as part of the MS and SGSN Initiated Bearer Deactivation procedure using S4.
 */
typedef struct itti_s11_delete_bearer_command_s {
  teid_t          teid;                   ///< Tunnel Endpoint Identifier

  // TODO
  void           *trxn;
  struct in_addr  peer_ip;
} itti_s11_delete_bearer_command_t;

//-----------------------------------------------------------------------------
/** @struct itti_s11_downlink_data_notification_t
 *  @brief Downlink Data Notification
 *
 * The Downlink Data Notification message is sent on the S11 interface by the SGW to the MME as part of the S1 paging procedure.
 */
typedef struct itti_s11_downlink_data_notification_s {
  teid_t          local_teid;  ///< not in specs for inner SPGW use
  teid_t          teid;                   ///< Tunnel Endpoint Identifier
  gtpv2c_cause_t  cause;                  ///< If SGW receives an Error Indication from eNodeB/RNC/S4-
                                          ///< SGSN/MME, the SGW shall send the Cause IE with value
                                          ///< "Error Indication received from RNC/eNodeB/S4-
                                          ///< SGSN/MME" to MME/S4-SGSN as specified in 3GPP TS23.007 [17].
  ebi_t           ebi;                    ///< Linked EPS Bearer ID
  imsi_t          imsi;                   ///< This IE shall be included on the S11/S4 interface as part of
                                          ///< the network triggered service restoration procedure if both
                                          ///< the SGW and the MME/S4-SGSN support this optional
                                          ///< feature (see 3GPP TS 23.007 [17]).
  fteid_t         sender_fteid_for_cp;    ///< This IE may be included on the S11/S4 interface towards
                                          ///< the restarted CN node or an alternative CN node (same
                                          ///< type of mobility node as the failed one) as part of the
                                          ///< network triggered service restoration procedure with or
                                          ///< without ISR if both the SGW and the MME/S4-SGSN
                                          ///< support this optional feature (see 3GPP TS 23.007 [17]).
                                          ///< This IE shall not be included otherwise.
  arp_t           arp;
  indication_flags_t    indication_flags; ///< This IE shall be included if any one of the applicable flags
                                          ///< is set to 1.
                                          ///<      - Applicable flags are:
                                          ///<        Associate OCI with SGW node's identity: The
                                          ///<        SGW shall set this flag to 1 on the S11/S4
                                          ///<        interface if it has included the "SGW's Overload
                                          ///<        Control Information" and if this information is to be
                                          ///<        associated with the node identity (i.e. FQDN or the
                                          ///<        IP address received from the DNS during the SGW
                                          ///<        selection) of the serving SGW.
  //SGW's node level Load Control Information
#define DOWNLINK_DATA_NOTIFICATION_PR_IE_CAUSE                  0x0001
#define DOWNLINK_DATA_NOTIFICATION_PR_IE_EPS_BEARER_ID          0x0002
#define DOWNLINK_DATA_NOTIFICATION_PR_IE_ARP                    0x0004
#define DOWNLINK_DATA_NOTIFICATION_PR_IE_IMSI                   0x0008
#define DOWNLINK_DATA_NOTIFICATION_PR_IE_SENDER_FTEID_FOR_CP    0x0010
#define DOWNLINK_DATA_NOTIFICATION_PR_IE_INDICATION_FLAGS       0x0020
#define DOWNLINK_DATA_NOTIFICATION_PR_IE_SGW_NODE_LEVEL_LCI     0x0040
#define DOWNLINK_DATA_NOTIFICATION_PR_IE_SGW_OVERLOAD_CI        0x0080
  uint32_t            ie_presence_mask;
  /* GTPv2-C specific parameters */
  void       *trxn;
  struct in_addr  peer_ip;
} itti_s11_downlink_data_notification_t;

//-----------------------------------------------------------------------------
/** @struct itti_s11_downlink_data_notification_acknowledge_t
 *  @brief Downlink Data Notification Acknowledge
 *
 * The Downlink Data Notification Acknowledge message is sent on the S11 interface by the MME to the SGW as part of the S1 paging procedure.
 */
typedef struct itti_s11_downlink_data_notification_acknowledge_s {
  teid_t          teid;                   ///< Tunnel Endpoint Identifier
  teid_t          local_teid;                   ///< Tunnel Endpoint Identifier
  gtpv2c_cause_t  cause;
  imsi_t          imsi;
  // Recovery           ///< optional This IE shall be included if contacting the peer for the first time
  // Private Extension  ///< optional
#define DOWNLINK_DATA_NOTIFICATION_ACK_PR_IE_CAUSE                                 0x0001
#define DOWNLINK_DATA_NOTIFICATION_ACK_PR_IE_DATA_NOTIFICATION_DELAY               0x0002
#define DOWNLINK_DATA_NOTIFICATION_ACK_PR_IE_RECOVERY                              0x0004
#define DOWNLINK_DATA_NOTIFICATION_ACK_PR_IE_DL_LOW_PRIORITY_TRAFFIC_THROTTLING    0x0008
#define DOWNLINK_DATA_NOTIFICATION_ACK_PR_IE_IMSI                                  0x0010
#define DOWNLINK_DATA_NOTIFICATION_ACK_PR_IE_DL_BUFFERING_DURATION                 0x0020
#define DOWNLINK_DATA_NOTIFICATION_ACK_PR_IE_DL_BUFFERING_SUGGESTED_PACKET_COUNT   0x0040
  uint32_t            ie_presence_mask;
  /* GTPv2-C specific parameters */
  void       *trxn;
  uint32_t    peer_ip;
} itti_s11_downlink_data_notification_acknowledge_t;


//-----------------------------------------------------------------------------
/** @struct itti_s11_downlink_data_notification_acknowledge_t
 *  @brief Downlink Data Notification Acknowledge
 *
 * The Downlink Data Notification Acknowledge message is sent on the S11 interface by the MME to the SGW as part of the S1 paging procedure.
 */
typedef struct itti_s11_downlink_data_notification_failure_indication_s {
  teid_t          teid;                   ///< Tunnel Endpoint Identifier
  teid_t          local_teid;                   ///< Tunnel Endpoint Identifier
  gtpv2c_cause_t  cause;
  imsi_t          imsi;
  // Recovery           ///< optional This IE shall be included if contacting the peer for the first time
  // Private Extension  ///< optional
#define DOWNLINK_DATA_NOTIFICATION_FAILURE_IND_PR_IE_CAUSE                0x0001
#define DOWNLINK_DATA_NOTIFICATION_FAILURE_IND_PR_IE_ORIGINATING_NODE     0x0002
#define DOWNLINK_DATA_NOTIFICATION_FAILURE_IND_PR_IE_IMSI                 0x0004
#define DOWNLINK_DATA_NOTIFICATION_FAILURE_IND_PR_IE_PRIVATE_EXTENSION    0x0008
  uint32_t            ie_presence_mask;
  /* GTPv2-C specific parameters */
  void       *trxn;
  struct in_addr  peer_ip;
} itti_s11_downlink_data_notification_failure_indication_t;


#ifdef __cplusplus
}
#endif
#endif
/* FILE_S11_MESSAGES_TYPES_SEEN */

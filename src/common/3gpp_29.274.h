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

/*! \file 3gpp_29.274.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_3GPP_29_274_SEEN
#define FILE_3GPP_29_274_SEEN
#include <arpa/inet.h>

#include "common_types.h"
#include "3gpp_24.301.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"

//-------------------------------------
// 8.4 Cause

typedef enum gtpv2c_cause_value_e {
  /* Request / Initial message */
  LOCAL_DETACH                    = 2,
  COMPLETE_DETACH                 = 3,
  RAT_CHANGE_3GPP_TO_NON_3GPP     = 4,  ///< RAT changed from 3GPP to Non-3GPP
  ISR_DEACTIVATION                = 5,
  ERROR_IND_FROM_RNC_ENB_SGSN     = 6,
  IMSI_DETACH_ONLY                = 7,
  REACTIVATION_REQUESTED          = 8,
  PDN_RECONNECTION_TO_THIS_APN_DISALLOWED = 9,
  ACCESS_CHANGED_FROM_NON_3GPP_TO_3GPP = 10,
  PDN_CONNECTION_INACTIVITY_TIMER_EXPIRES = 11,

  /* Acceptance in a Response/Triggered message */
  REQUEST_ACCEPTED                = 16,
  REQUEST_ACCEPTED_PARTIALLY      = 17,
  NEW_PDN_TYPE_NW_PREF            = 18, ///< New PDN type due to network preference
  NEW_PDN_TYPE_SAB_ONLY           = 19, ///< New PDN type due to single address bearer only
  /* Rejection in a Response triggered message. */
  CONTEXT_NOT_FOUND               = 64,
  INVALID_MESSAGE_FORMAT          = 65,
  VERSION_NOT_SUPPORTED_BY_NEXT_PEER = 66,
  INVALID_LENGTH                  = 67,
  SERVICE_NOT_SUPPORTED           = 68,
  MANDATORY_IE_INCORRECT          = 69,
  MANDATORY_IE_MISSING            = 70,
  SYSTEM_FAILURE                  = 72,
  NO_RESOURCES_AVAILABLE          = 73,
  SEMANTIC_ERROR_IN_TFT           = 74,
  SYNTACTIC_ERROR_IN_TFT          = 75,
  SEMANTIC_ERRORS_IN_PF           = 76,
  SYNTACTIC_ERRORS_IN_PF          = 77,
  MISSING_OR_UNKNOWN_APN          = 78,
  GRE_KEY_NOT_FOUND               = 80,
  RELOCATION_FAILURE              = 81,
  DENIED_IN_RAT                   = 82,
  PREFERRED_PDN_TYPE_NOT_SUPPORTED = 83,
  ALL_DYNAMIC_ADDRESSES_ARE_OCCUPIED = 84,
  UE_CONTEXT_WITHOUT_TFT_ALREADY_ACTIVATED = 85,
  PROTOCOL_TYPE_NOT_SUPPORTED     = 86,
  UE_NOT_RESPONDING               = 87,
  UE_REFUSES                      = 88,
  SERVICE_DENIED                  = 89,
  UNABLE_TO_PAGE_UE               = 90,
  NO_MEMORY_AVAILABLE             = 91,
  USER_AUTHENTICATION_FAILED      = 92,
  APN_ACCESS_DENIED_NO_SUBSCRIPTION = 93,
  REQUEST_REJECTED                = 94,
  P_TMSI_SIGNATURE_MISMATCH       = 95,
  IMSI_IMEI_NOT_KNOWN             = 96,
  SEMANTIC_ERROR_IN_THE_TAD_OPERATION = 97,
  SYNTACTIC_ERROR_IN_THE_TAD_OPERATION = 98,
  REMOTE_PEER_NOT_RESPONDING      = 100,
  COLLISION_WITH_NETWORK_INITIATED_REQUEST = 101,
  UNABLE_TO_PAGE_UE_DUE_TO_SUSPENSION = 102,
  CONDITIONAL_IE_MISSING          = 103,
  APN_RESTRICTION_TYPE_INCOMPATIBLE_WITH_CURRENTLY_ACTIVE_PDN_CONNECTION = 104,
  INVALID_OVERALL_LENGTH_OF_THE_TRIGGERED_RESPONSE_MESSAGE_AND_A_PIGGYBACKED_INITIAL_MESSAGE = 105,
  DATA_FORWARDING_NOT_SUPPORTED   = 106,
  INVALID_REPLY_FROM_REMOTE_PEER  = 107,
  FALLBACK_TO_GTPV1               = 108,
  INVALID_PEER                    = 109,
  TEMP_REJECT_HO_IN_PROGRESS      = 110, ///< Temporarily rejected due to handover procedure in progress
  MODIFICATIONS_NOT_LIMITED_TO_S1_U_BEARERS = 111,
  REJECTED_FOR_PMIPv6_REASON      = 112, ///< Request rejected for a PMIPv6 reason (see 3GPP TS 29.275 [26]).
  APN_CONGESTION                  = 113,
  BEARER_HANDLING_NOT_SUPPORTED   = 114,
  UE_ALREADY_RE_ATTACHED          = 115,
  M_PDN_APN_NOT_ALLOWED           = 116, ///< Multiple PDN connections for a given APN not allowed.
  SGW_CAUSE_MAX
} gtpv2c_cause_value_t;


typedef struct {
  gtpv2c_cause_value_t  cause_value;
  uint8_t  pce:1;
  uint8_t  bce:1;
  uint8_t  cs:1;

  uint8_t  offending_ie_type;
  uint16_t offending_ie_length;
  uint8_t  offending_ie_instance;
} gtpv2c_cause_t;

//-------------------------------------
// 8.15 Bearer Quality of Service (Bearer QoS)
#define PRE_EMPTION_CAPABILITY_ENABLED  (0x0)
#define PRE_EMPTION_CAPABILITY_DISABLED (0x1)
#define PRE_EMPTION_VULNERABILITY_ENABLED  (0x0)
#define PRE_EMPTION_VULNERABILITY_DISABLED (0x1)

typedef struct bearer_qos_s {
  /* PCI (Pre-emption Capability)
   * The following values are defined:
   * - PRE-EMPTION_CAPABILITY_ENABLED (0)
   *    This value indicates that the service data flow or bearer is allowed
   *    to get resources that were already assigned to another service data
   *    flow or bearer with a lower priority level.
   * - PRE-EMPTION_CAPABILITY_DISABLED (1)
   *    This value indicates that the service data flow or bearer is not
   *    allowed to get resources that were already assigned to another service
   *    data flow or bearer with a lower priority level.
   * Default value: PRE-EMPTION_CAPABILITY_DISABLED
   */
  unsigned pci:1;
  /* PL (Priority Level): defined in 3GPP TS.29.212 #5.3.45
   * Values 1 to 15 are defined, with value 1 as the highest level of priority.
   * Values 1 to 8 should only be assigned for services that are authorized to
   * receive prioritized treatment within an operator domain. Values 9 to 15
   * may be assigned to resources that are authorized by the home network and
   * thus applicable when a UE is roaming.
   */
  unsigned pl:4;
  /* PVI (Pre-emption Vulnerability): defined in 3GPP TS.29.212 #5.3.47
   * Defines whether a service data flow can lose the resources assigned to it
   * in order to admit a service data flow with higher priority level.
   * The following values are defined:
   * - PRE-EMPTION_VULNERABILITY_ENABLED (0)
   *   This value indicates that the resources assigned to the service data
   *   flow or bearer can be pre-empted and allocated to a service data flow
   *   or bearer with a higher priority level.
   * - PRE-EMPTION_VULNERABILITY_DISABLED (1)
   *   This value indicates that the resources assigned to the service data
   *   flow or bearer shall not be pre-empted and allocated to a service data
   *   flow or bearer with a higher priority level.
   * Default value: EMPTION_VULNERABILITY_ENABLED
   */
  unsigned pvi:1;
  uint8_t  qci;
  ambr_t   gbr;           ///< Guaranteed bit rate
  ambr_t   mbr;           ///< Maximum bit rate
} bearer_qos_t;

//-------------------------------------
// 8.22 Fully Qualified TEID (F-TEID)

/* WARNING: not complete... */
typedef enum interface_type_e {
  INTERFACE_TYPE_MIN = 0,
  S1_U_ENODEB_GTP_U = INTERFACE_TYPE_MIN,
  S1_U_SGW_GTP_U,
  S12_RNC_GTP_U,
  S12_SGW_GTP_U,
  S5_S8_SGW_GTP_U,
  S5_S8_PGW_GTP_U,
  S5_S8_SGW_GTP_C,
  S5_S8_PGW_GTP_C,
  S5_S8_SGW_PMIPv6,
  S5_S8_PGW_PMIPv6,
  S11_MME_GTP_C,
  S11_SGW_GTP_C,
  S10_MME_GTP_C,
  S3_MME_GTP_C,
  S3_SGSN_GTP_C,
  S4_SGSN_GTP_U,
  S4_SGW_GTP_U,
  S4_SGSN_GTP_C,
  S16_SGSN_GTP_C,
  ENODEB_GTP_U_DL_DATA_FORWARDING,
  ENODEB_GTP_U_UL_DATA_FORWARDING,
  RNC_GTP_U_DATA_FORWARDING,
  SGSN_GTP_U_DATA_FORWARDING,
  SGW_GTP_U_DL_DATA_FORWARDING,
  SM_MBMS_GW_GTP_C,
  SN_MBMS_GW_GTP_C,
  SM_MME_GTP_C,
  SN_SGSN_GTP_C,
  SGW_GTP_U_UL_DATA_FORWARDING,
  SN_SGSN_GTP_U,
  S2B_EPDG_GTP_C ,
  INTERFACE_TYPE_MAX = S2B_EPDG_GTP_C
} interface_type_t;

typedef struct fteid_s {
  unsigned        ipv4:1;
  unsigned        ipv6:1;
  interface_type_t interface_type;
  teid_t          teid; ///< TEID or GRE Key
  struct in_addr  ipv4_address;
  struct in6_addr ipv6_address;
} fteid_t;


typedef struct {
  uint8_t                  eps_bearer_id;    ///< EBI,  Mandatory CSR
  bearer_qos_t             bearer_level_qos;
  traffic_flow_template_t  tft;              ///< Bearer TFT, Optional CSR, This IE may be included on the S4/S11 and S5/S8 interfaces.
} bearer_to_create_t;

//-----------------
typedef struct bearer_context_to_be_created_s {
  uint8_t                  eps_bearer_id;       ///< EBI,  Mandatory CSR
  traffic_flow_template_t  tft;                 ///< Bearer TFT, Optional CSR, This IE may be included on the S4/S11 and S5/S8 interfaces.
  fteid_t                  s1u_enb_fteid;       ///< S1-U eNodeB F-TEID, Conditional CSR, This IE shall be included on the S11 interface for X2-based handover with SGW relocation.
  fteid_t                  s1u_sgw_fteid;       ///< S1-U SGW F-TEID, Conditional CSR, This IE shall be included on the S11 interface for X2-based handover with SGW relocation.
  fteid_t                  s4u_sgsn_fteid;      ///< S4-U SGSN F-TEID, Conditional CSR, This IE shall be included on the S4 interface if the S4-U interface is used.
  fteid_t                  s5_s8_u_sgw_fteid;   ///< S5/S8-U SGW F-TEID, Conditional CSR, This IE shall be included on the S5/S8 interface for an "eUTRAN Initial Attach",
                                                ///  a "PDP Context Activation" or a "UE Requested PDN Connectivity".
  fteid_t                  s5_s8_u_pgw_fteid;   ///< S5/S8-U PGW F-TEID, Conditional CSR, This IE shall be included on the S4 and S11 interfaces for the TAU/RAU/Handover
                                                /// cases when the GTP-based S5/S8 is used.
  fteid_t                  s12_rnc_fteid;       ///< S12 RNC F-TEID, Conditional Optional CSR, This IE shall be included on the S4 interface if the S12
                                                /// interface is used in the Enhanced serving RNS relocation with SGW relocation procedure.
  fteid_t                  s2b_u_epdg_fteid;    ///< S2b-U ePDG F-TEID, Conditional CSR, This IE shall be included on the S2b interface for an Attach
                                                /// with GTP on S2b, a UE initiated Connectivity to Additional PDN with GTP on S2b and a Handover to Untrusted Non-
                                                /// 3GPP IP Access with GTP on S2b.
  /* This parameter is received only if the QoS parameters have been modified */
  bearer_qos_t                      bearer_level_qos;    ///< Bearer QoS, Mandatory CSR
  protocol_configuration_options_t  pco;///< This IE may be sent on the S5/S8 and S4/S11 interfaces
                                    ///< if ePCO is not supported by the UE or the network. This bearer level IE takes precedence
                                    ///< over the PCO IE in the message body if they both exist.
} bearer_context_to_be_created_t;

typedef struct bearer_contexts_to_be_created_s {
#define MSG_CREATE_SESSION_REQUEST_MAX_BEARER_CONTEXTS   11
uint8_t num_bearer_context;
bearer_context_to_be_created_t bearer_contexts[MSG_CREATE_SESSION_REQUEST_MAX_BEARER_CONTEXTS];    ///< Bearer Contexts to be created
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
} bearer_contexts_to_be_created_t;

//-------------------------------------
// 8.38 MM EPS Context

typedef struct mm_context_eps_s {
  // todo: better structure for flags
//  uint32_t                  mm_context_flags:24;
  uint8_t                   sec_mode:3;
  // todo: uint8_t                   drxi:1;
  uint8_t                   ksi:3;
  uint8_t                   num_quit:3;
  uint8_t                   num_quad:3;
  // todo: osci 0 --> old stuff (everything from s to s+64 in 29.274 --> 8-38.5 not present
  uint8_t                   nas_int_alg:3;
  uint8_t                   nas_cipher_alg:4;
//  uint32_t                   nas_dl_count[3]; // todo: or directly uint32_t?
//  uint8_t                   nas_ul_count[3]; // todo: or directly uint32_t?
  count_t                   nas_dl_count;
  count_t                   nas_ul_count;
  uint8_t                   k_asme[32];
  mm_ue_eps_authentication_quadruplet_t* auth_quadruplet[5];
  mm_ue_eps_authentication_quintuplet_t* auth_quintuplet[5];
  // todo : drx_t*                    drx;
  uint8_t                   nh[32];
  uint8_t                   ncc:3;
  uint32_t                  ul_subscribed_ue_ambr;
  uint32_t                  dl_subscribed_ue_ambr;
  uint32_t                  ul_used_ue_ambr;
  uint32_t                  dl_used_ue_ambr;
  uint8_t                   ue_nc_length;
  ue_network_capability_t   ue_nc;
  uint8_t                   ms_nc_length;
  ms_network_capability_t   ms_nc;
  uint8_t                   mei_length;
  Mei_t                     mei;
  uint8_t                   vdp_lenth;
  uint8_t                   vdp; // todo: ??
  uint8_t                   access_restriction_flags;
} mm_context_eps_t;
//----------------------------


//-------------------------------------------------
// 8.39: PDN Connection

typedef struct pdn_connection_s {
//  char                      apn[APN_MAX_LENGTH + 1]; ///< Access Point Name
  //  protocol_configuration_options_t pco;
  bstring                   apn_str;
//  int                       pdn_type;

  APNRestriction_t          apn_restriction;     ///< This IE shall be included on the S5/S8 and S4/S11
  ///< interfaces in the E-UTRAN initial attach, PDP Context
  ///< Activation and UE Requested PDN connectivity
  ///< procedures.
  ///< This IE shall also be included on S4/S11 during the Gn/Gp
  ///< SGSN to S4 SGSN/MME RAU/TAU procedures.
  ///< This IE denotes the restriction on the combination of types
  ///< of APN for the APN associated with this EPS bearer
  ///< Context.

  SelectionMode_t           selection_mode;      ///< Selection Mode
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

//  uint8_t                   ipv4_address[4];
//  uint8_t                   ipv6_address[16];
//  pdn_type_value_t pdn_type;
  struct in_addr            ipv4_address;
  struct in6_addr           ipv6_address;
  uint8_t                   ipv6_prefix_length;

  ebi_t                     linked_eps_bearer_id;

  fteid_t                   pgw_address_for_cp;  ///< PGW S5/S8 address for control plane or PMIP


  bearer_contexts_to_be_created_t  bearer_context_list;

  ambr_t                    apn_ambr;

} pdn_connection_t;

//------------------------
#define MSG_FORWARD_RELOCATION_REQUEST_MAX_PDN_CONNECTIONS   3
#define MSG_FORWARD_RELOCATION_REQUEST_MAX_BEARER_CONTEXTS   11

typedef struct mme_ue_eps_pdn_connections_s {
  uint8_t num_pdn_connections;
  pdn_connection_t pdn_connection[MSG_FORWARD_RELOCATION_REQUEST_MAX_PDN_CONNECTIONS];
} mme_ue_eps_pdn_connections_t;
//----------------------------

//-------------------------------------
// 7.2.4-2: Bearer Context within Create Bearer Response

typedef struct bearer_context_within_create_bearer_response_s {
  uint8_t      eps_bearer_id;       ///< EBI
  gtpv2c_cause_t  cause;               ///< This IE shall indicate if the bearer handling was successful,
                                    ///< and if not, it gives information on the reason.
  fteid_t      s1u_enb_fteid;       ///< This IE shall be sent on the S11 interface if the S1-U interface is used.
  fteid_t      s1u_sgw_fteid;       ///< This IE shall be sent on the S11 interface. It shall be used
                                    ///< to correlate the bearers with those in the Create Bearer
                                    ///< Request.
  fteid_t      s5_s8_u_sgw_fteid;   ///< This IE shall be sent on the S5/S8 interfaces.
  fteid_t      s5_s8_u_pgw_fteid;   ///< This IE shall be sent on the S5/S8 interfaces. It shall be
                                    ///< used to correlate the bearers with those in the Create
                                    ///< Bearer Request.
  fteid_t      s12_rnc_fteid;       ///< C This IE shall be sent on the S4 interface if the S12
                                    ///< interface is used. See NOTE 1.
  fteid_t      s12_sgw_fteid;       ///< C This IE shall be sent on the S4 interface. It shall be used to
                                    ///< correlate the bearers with those in the Create Bearer
                                    ///< Request. See NOTE1.
  fteid_t      s4_u_sgsn_fteid ;    ///< C This IE shall be sent on the S4 interface if the S4-U
                                    ///< interface is used. See NOTE1.
  fteid_t      s4_u_sgw_fteid;      ///< C This IE shall be sent on the S4 interface. It shall be used to
                                    ///< correlate the bearers with those in the Create Bearer
                                    ///< Request. See NOTE1.
  fteid_t      s2b_u_epdg_fteid;    ///<  C This IE shall be sent on the S2b interface.
  fteid_t      s2b_u_pgw_fteid;     ///<  C This IE shall be sent on the S2b interface. It shall be used
                                    ///< to correlate the bearers with those in the Create Bearer
                                    ///<   Request.
  protocol_configuration_options_t  pco;///< If the UE includes the PCO IE in the corresponding
                                    ///< message, then the MME/SGSN shall copy the content of
                                    ///< this IE transparently from the PCO IE included by the UE.
                                    ///< If the SGW receives PCO from MME/SGSN, SGW shall
                                    ///< forward it to the PGW. This bearer level IE takes
                                    ///< precedence over the PCO IE in the message body if they
                                    ///< both exist.
} bearer_context_within_create_bearer_response_t;



void free_bearer_contexts_to_be_created(bearer_contexts_to_be_created_t **bcs_tbc);
void free_mme_ue_eps_pdn_connections(mme_ue_eps_pdn_connections_t ** pdn_connections);
void free_mm_context_eps(mm_context_eps_t ** ue_eps_mm_context);

#endif /* FILE_3GPP_29_274_SEEN */

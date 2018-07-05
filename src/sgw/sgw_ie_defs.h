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

/*! \file sgw_ie_defs.h
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_SGW_IE_DEFS_SEEN
#define FILE_SGW_IE_DEFS_SEEN
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t  DelayValue_t;
typedef uint32_t SequenceNumber_t;


/* 3GPP TS 29.274 Figure 8.12 */

typedef struct indication_flags_s {
#define INDICATION_FLAGS_MAX_OCTETS 7
  uint8_t octets[INDICATION_FLAGS_MAX_OCTETS];
  uint8_t num_octets;

} indication_flags_t;

/* Bit mask for octet 5 in indication IE */
#define O5_DAF_FLAG_BIT_POS      7
#define O5_DTF_FLAG_BIT_POS      6
#define O5_HI_FLAG_BIT_POS       5
#define O5_DFI_FLAG_BIT_POS      4
#define O5_OI_FLAG_BIT_POS       3
#define O5_ISRSI_FLAG_BIT_POS    2
#define O5_ISRAI_FLAG_BIT_POS    1
#define O5_SGWCI_FLAG_BIT_POS    0

/* Bit mask for octet 6 in indication IE */
#define O6_SQSI_FLAG_BIT_POS   7
#define O6_UIMSI_FLAG_BIT_POS  6
#define O6_CFSI_FLAG_BIT_POS   5
#define O6_CRSI_FLAG_BIT_POS   4
#define O6_P_FLAG_BIT_POS      3
#define O6_PT_FLAG_BIT_POS     2
#define O6_SI_FLAG_BIT_POS     1
#define O6_MSV_FLAG_BIT_POS    0

/* Bit mask for octet 7 in indication IE */
// UPDATE RELEASE 10
#define O7_S6AF_FLAG_BIT_POS  4
#define O7_S4AF_FLAG_BIT_POS  3
#define O7_MBMDT_FLAG_BIT_POS 2
#define O7_ISRAU_FLAG_BIT_POS 1
#define O7_CCRSI_FLAG_BIT_POS 0

/* Bit mask for octet 8 in indication IE */
#define O7_RETLOC_FLAG_BIT_POS  7
#define O7_PBIC_FLAG_BIT_POS  6
#define O7_SRNI_FLAG_BIT_POS  5
#define O7_S6AF_FLAG_BIT_POS  4
#define O7_S4AF_FLAG_BIT_POS  3
#define O7_MBMDT_FLAG_BIT_POS 2
#define O7_ISRAU_FLAG_BIT_POS 1
#define O7_CCRSI_FLAG_BIT_POS 0

// Octet 8
#define O8_CPRAI_FLAG_BIT_POS 7
#define O8_ARRL_FLAG_BIT_POS  6
#define O8_PPOF_FLAG_BIT_POS  5
#define O8_PPON_FLAG_BIT_POS  4
#define O8_PPSI_FLAG_BIT_POS  3
#define O8_CSFBI_FLAG_BIT_POS 2
#define O8_CLII_FLAG_BIT_POS  1
#define O8_CPSR_FLAG_BIT_POS  0

// Octet 9
#define O9_NSI_FLAG_BIT_POS  7
#define O9_UASI_FLAG_BIT_POS 6
#define O9_DTCI_FLAG_BIT_POS 5
#define O9_BDWI_FLAG_BIT_POS 4
#define O9_PSCI_FLAG_BIT_POS 3
#define O9_PCRI_FLAG_BIT_POS 2
#define O9_AOSI_FLAG_BIT_POS 1
#define O9_AOPI_FLAG_BIT_POS 0

// Octet 10
#define O10_ROAAI_FLAG_BIT_POS   7
#define O10_EPCOSI_FLAG_BIT_POS  6
#define O10_CPOPCI_FLAG_BIT_POS  5
#define O10_PMTSMI_FLAG_BIT_POS  4
#define O10_S11TF_FLAG_BIT_POS   3
#define O10_PNSI_FLAG_BIT_POS    2
#define O10_UNACCSI_FLAG_BIT_POS 1
#define O10_WPMSI_FLAG_BIT_POS   0

// Octet 11
#define O11_SPARE7_FLAG_BIT_POS  7
#define O11_SPARE6_FLAG_BIT_POS  6
#define O11_SPARE5_FLAG_BIT_POS  5
#define O11_SPARE4_FLAG_BIT_POS  4
#define O11_SPARE3_FLAG_BIT_POS  3
#define O11_SPARE2_FLAG_BIT_POS  2
#define O11_ENBCRSI_FLAG_BIT_POS 1
#define O11_TSPCMI_FLAG_BIT_POS  0


typedef struct {
  uint8_t digit[MSISDN_LENGTH];
  uint8_t length;
} Msisdn_t;

typedef struct {
  uint8_t  mcc[3];
  uint8_t  mnc[3];
  uint16_t lac;
  uint16_t ci;
} Cgi_t;

typedef struct {
  uint8_t  mcc[3];
  uint8_t  mnc[3];
  uint16_t lac;
  uint16_t sac;
} Sai_t;

typedef struct {
  uint8_t  mcc[3];
  uint8_t  mnc[3];
  uint16_t lac;
  uint16_t rac;
} Rai_t;

typedef struct {
  uint8_t  mcc[3];
  uint8_t  mnc[3];
  uint16_t tac;
} Tai_t;

typedef struct {
  uint8_t  mcc[3];
  uint8_t  mnc[3];
  uint32_t eci;
} Ecgi_t;

typedef struct {
  uint8_t  mcc[3];
  uint8_t  mnc[3];
  uint16_t lac;
} Lai_t;

#define ULI_CGI  0x01
#define ULI_SAI  0x02
#define ULI_RAI  0x04
#define ULI_TAI  0x08
#define ULI_ECGI 0x10
#define ULI_LAI  0x20

typedef struct {
  uint8_t present;
  struct {
    Cgi_t  cgi;
    Sai_t  sai;
    Rai_t  rai;
    Tai_t  tai;
    Ecgi_t ecgi;
    Lai_t  lai;
  } s;
} Uli_t;

typedef struct {
  uint8_t mcc[3];
  uint8_t mnc[3];
} ServingNetwork_t;


#define FTEID_T_2_IP_ADDRESS_T(fte_p,ip_p) \
do { \
    if ((fte_p)->ipv4) { \
      (ip_p)->pdn_type = IPv4; \
      (ip_p)->address.ipv4_address.s_addr = (fte_p)->ipv4_address.s_addr;         \
    } \
    if ((fte_p)->ipv6) { \
        if ((fte_p)->ipv4) { \
          (ip_p)->pdn_type = IPv4_AND_v6; \
        } else { \
          (ip_p)->pdn_type = IPv6; \
        } \
        memcpy(&(ip_p)->address.ipv6_address, &(fte_p)->ipv6_address, sizeof((fte_p)->ipv6_address)); \
    } \
} while (0)

typedef enum {
  TARGET_ID_RNC_ID       = 0,
  TARGET_ID_MACRO_ENB_ID = 1,
  TARGET_ID_CELL_ID      = 2,
  TARGET_ID_HOME_ENB_ID  = 3
                           /* Other values are spare */
} target_type_t;

typedef struct {
  uint16_t lac;
  uint8_t  rac;

  /* Length of RNC Id can be 2 bytes if length of element is 8
   * or 4 bytes long if length is 10.
   */
  uint16_t id;
  uint16_t xid;
} rnc_id_t;

typedef struct {
  unsigned enb_id:20;
  uint16_t tac;
} macro_enb_id_t;

typedef struct {
  unsigned enb_id:28;
  uint16_t tac;
} home_enb_id_t;

typedef struct {
  /* Common part */
  uint8_t target_type;

  uint8_t  mcc[3];
  uint8_t  mnc[3];
  union {
    rnc_id_t       rnc_id;
    macro_enb_id_t macro_enb_id;
    home_enb_id_t  home_enb_id;
  } target_id;
} target_identification_t;

typedef struct {
  uint32_t uplink_ambr;
  uint32_t downlink_ambr;
} AMBR_t;

typedef enum node_id_type_e {
  GLOBAL_UNICAST_IPv4 = 0,
  GLOBAL_UNICAST_IPv6 = 1,
  TYPE_EXOTIC         = 2, ///< (MCC * 1000 + MNC) << 12 + Integer value assigned to MME by operator
} node_id_type_t;

typedef struct {
  node_id_type_t node_id_type;
  uint16_t       csid;          ///< Connection Set Identifier
  union {
    struct in_addr   unicast_ipv4;
    struct in6_addr  unicast_ipv6;
    struct {
      uint16_t mcc;
      uint16_t mnc;
      uint16_t operator_specific_id;
    } exotic;
  } node_id;
} FQ_CSID_t;

typedef struct {
  uint8_t  time_zone;
  unsigned daylight_saving_time:2;
} UETimeZone_t;

typedef enum AccessMode_e {
  CLOSED_MODE = 0,
  HYBRID_MODE = 1,
} AccessMode_t;

typedef struct {
  uint8_t  mcc[3];
  uint8_t  mnc[3];
  uint32_t csg_id;
  AccessMode_t access_mode;
  unsigned lcsg:1;
  unsigned cmi:1;
} UCI_t;

typedef struct {
  /* PPC (Prohibit Payload Compression):
   * This flag is used to determine whether an SGSN should attempt to
   * compress the payload of user data when the users asks for it
   * to be compressed (PPC = 0), or not (PPC = 1).
   */
  unsigned ppc:1;

  /* VB (Voice Bearer):
   * This flag is used to indicate a voice bearer when doing PS-to-CS
   * SRVCC handover.
   */
  unsigned vb:1;
} bearer_flags_t;


typedef enum node_type_e {
  NODE_TYPE_MME  = 0,
  NODE_TYPE_SGSN = 1
} node_type_t;

//-----------------
typedef struct bearer_context_created_s {
  uint8_t       eps_bearer_id;       ///< EPS Bearer ID
  gtpv2c_cause_t cause;

  /* This parameter is used on S11 interface only */
  fteid_t       s1u_sgw_fteid;       ///< S1-U SGW F-TEID

  /* This parameter is used on S4 interface only */
  fteid_t       s4u_sgw_fteid;       ///< S4-U SGW F-TEID

  /* This parameter is used on S11 and S5/S8 interface only for a
   * GTP-based S5/S8 interface and during:
   * - E-UTRAN Inintial attch
   * - PDP Context Activation
   * - UE requested PDN connectivity
   */
  fteid_t       s5_s8_u_pgw_fteid;   ///< S4-U SGW F-TEID

  /* This parameter is used on S4 interface only and when S12 interface is used */
  fteid_t       s12_sgw_fteid;       ///< S12 SGW F-TEID

  /* This parameter is received only if the QoS parameters have been modified */
  bearer_qos_t *bearer_level_qos;

  traffic_flow_template_t  tft;                 ///< Bearer TFT
} bearer_context_created_t;

typedef struct bearer_contexts_created_s {
  uint8_t num_bearer_context;
  bearer_context_created_t bearer_contexts[MSG_CREATE_SESSION_REQUEST_MAX_BEARER_CONTEXTS];
} bearer_contexts_created_t;

//-----------------
typedef struct bearer_context_modified_s {
  uint8_t       eps_bearer_id;   ///< EPS Bearer ID
  gtpv2c_cause_t cause;
  fteid_t       s1u_sgw_fteid;   ///< Sender F-TEID for user plane
} bearer_context_modified_t;

typedef struct bearer_contexts_modified_s {
#define MSG_MODIFY_BEARER_RESPONSE_MAX_BEARER_CONTEXTS   11
  uint8_t num_bearer_context;
  bearer_context_modified_t bearer_contexts[MSG_MODIFY_BEARER_RESPONSE_MAX_BEARER_CONTEXTS];
} bearer_contexts_modified_t;

//-----------------
typedef struct bearer_context_marked_for_removal_s {
  uint8_t       eps_bearer_id;   ///< EPS bearer ID
  gtpv2c_cause_t cause;
} bearer_context_marked_for_removal_t;

typedef struct bearer_contexts_marked_for_removal_s {
  uint8_t num_bearer_context;
  bearer_context_marked_for_removal_t bearer_contexts[MSG_MODIFY_BEARER_RESPONSE_MAX_BEARER_CONTEXTS];
} bearer_contexts_marked_for_removal_t;

//-----------------
typedef struct bearer_context_to_be_modified_s {
  uint8_t eps_bearer_id;      ///< EPS Bearer ID
  fteid_t s1_eNB_fteid;       ///< S1 eNodeB F-TEID
  fteid_t s1u_sgw_fteid;      ///< S1-U SGW F-TEID
} bearer_context_to_be_modified_t;

typedef struct bearer_contexts_to_be_modified_s {
#define MSG_MODIFY_BEARER_REQUEST_MAX_BEARER_CONTEXTS   11
  uint8_t num_bearer_context;
  bearer_context_to_be_modified_t bearer_contexts[MSG_MODIFY_BEARER_REQUEST_MAX_BEARER_CONTEXTS];
} bearer_contexts_to_be_modified_t;
//-----------------

typedef struct bearer_context_to_be_removed_s {
  uint8_t eps_bearer_id;      ///< EPS Bearer ID, Mandatory
  fteid_t s4u_sgsn_fteid;     ///< S4-U SGSN F-TEID, Conditional , redundant
} bearer_context_to_be_removed_t; // Within Create Session Request, Modify Bearer Request, Modify Access Bearers Request


typedef struct bearer_contexts_to_be_removed_s {
  uint8_t num_bearer_context;
  bearer_context_to_be_removed_t bearer_contexts[MSG_CREATE_SESSION_REQUEST_MAX_BEARER_CONTEXTS];
} bearer_contexts_to_be_removed_t;

typedef struct ebi_list_s {
  uint32_t   num_ebi;
  #define RELEASE_ACCESS_BEARER_MAX_BEARERS   8
  ebi_t      ebis[RELEASE_ACCESS_BEARER_MAX_BEARERS]  ;
} ebi_list_t;


//-----------------
#define MSG_CREATE_BEARER_REQUEST_MAX_BEARER_CONTEXTS   11

typedef struct bearer_contexts_within_create_bearer_response_s {
#define MSG_CREATE_BEARER_RESPONSE_MAX_BEARER_CONTEXTS   11
  uint8_t num_bearer_context;
  bearer_context_within_create_bearer_response_t bearer_contexts[MSG_CREATE_BEARER_RESPONSE_MAX_BEARER_CONTEXTS];
} bearer_contexts_within_create_bearer_response_t;

#ifdef __cplusplus
}
#endif

#endif  /* FILE_SGW_IE_DEFS_SEEN */


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



typedef struct {
  uint32_t uplink_ambr;
  uint32_t downlink_ambr;
} AMBR_t;

<<<<<<< HEAD
=======
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
  fteid_t       s1u_enb_fteid;       ///< S1-U ENB F-TEID

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
typedef struct ebis_to_be_deleted_s {
#define MSG_DELETE_BEARER_REQUEST_MAX_EBIS_TO_BE_DELETED 11
  uint8_t        num_ebis;
  uint8_t        eps_bearer_id[MSG_DELETE_BEARER_REQUEST_MAX_EBIS_TO_BE_DELETED];   ///< EPS bearer ID
} ebis_to_be_deleted_t;

//-----------------
typedef struct bearer_context_marked_for_removal_s {
  uint8_t        eps_bearer_id;   ///< EPS bearer ID
  gtpv2c_cause_t cause;
  fteid_t        s1u_sgw_fteid;   ///< Sender F-TEID for user plane
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
  gtpv2c_cause_t cause;
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
>>>>>>> dc94741...  bearer deactivation and review / NO TEST

#ifdef __cplusplus
}
#endif



#endif  /* FILE_SGW_IE_DEFS_SEEN */


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

/*! \file mme_ie_defs.h
* \brief
* \author Dincer Beken
* \company Blackned GmbH
* \email: dbeken@blackned.de
*
*/

#ifndef FILE_MME_IE_DEFS_SEEN
#define FILE_MME_IE_DEFS_SEEN
#include "common_types.h"
#include "3gpp_24.008.h"
#include "3gpp_36.401.h"
#include "3gpp_24.301.h"
#include "sgw_ie_defs.h"
#include "nas_timer.h"

/* Cause as defined in 3GPP TS 29.274 #8.4 */

typedef enum {
  FCAUSE_RANAP,
  FCAUSE_BSSGP,
  FCAUSE_S1AP,
}F_Cause_Type_t;

typedef enum {
  FCAUSE_S1AP_RNL      = 0,  // Radio Network Layer
  FCAUSE_S1AP_TL       = 1,  // Transport Layer
  FCAUSE_S1AP_NAS      = 2,  // NAS Layer
  FCAUSE_S1AP_Protocol = 3,
  FCAUSE_S1AP_Misc     = 4,
}F_Cause_S1AP_Type_t;

/** Only S1AP will be supported for RAN cause. */
typedef struct {
  F_Cause_Type_t      fcause_type;
  F_Cause_S1AP_Type_t fcause_s1ap_type;
  uint8_t             fcause_value;
}F_Cause_t;
//
//typedef enum {
//  COMPLETE_ATTACH_REQUEST_TYPE      = 0,
//  COMPLETE_TAU_REQUEST_TYPE         = 1,
//}Complete_Request_Message_Type_t;

typedef struct F_Container{
  bstring     container_value;
  uint8_t     container_type;
}F_Container_t;

typedef struct Complete_Request_Message{
  bstring                             request_value;
  Complete_Request_Message_Type_t     request_type;
}Complete_Request_Message_t;

//-----------------
typedef struct bearer_context_setup {
  uint8_t      eps_bearer_id;       ///< EPS Bearer ID
  // todo: rest is for indirect data forwarding!
} bearer_context_setup_t;

#define MAX_SETUP_BEARERS 11

typedef struct list_of_setup_bearers_s {
  uint8_t num_bearer_context;
  bearer_context_setup_t bearer_contexts[MAX_SETUP_BEARERS];
} list_of_setup_bearers_t;

//------------------------
#define MSG_FORWARD_RELOCATION_REQUEST_MAX_PDN_CONNECTIONS   3
#define MSG_FORWARD_RELOCATION_REQUEST_MAX_BEARER_CONTEXTS   11

typedef struct mme_ue_eps_pdn_connections_s {
  uint8_t num_pdn_connections;
  pdn_connection_t pdn_connection[MSG_FORWARD_RELOCATION_REQUEST_MAX_PDN_CONNECTIONS];
} mme_ue_eps_pdn_connections_t;
//----------------------------

///*
// * Minimal and maximal value of an EPS bearer identity:
// * The EPS Bearer Identity (EBI) identifies a message flow
// */
//#define ESM_EBI_MIN     (EPS_BEARER_IDENTITY_FIRST)
//#define ESM_EBI_MAX     (EPS_BEARER_IDENTITY_LAST)
//
///* EPS bearer context states */
//typedef enum {
//  ESM_EBR_INACTIVE = 0,     /* No EPS bearer context exists     */
//  ESM_EBR_ACTIVE,           /* The EPS bearer context is active, in the UE, in the network        */
//  ESM_EBR_INACTIVE_PENDING, /* The network has initiated an EPS bearer context deactivation towards the UE  */
//  ESM_EBR_MODIFY_PENDING,   /* The network has initiated an EPS bearer context modification towards the UE  */
//  ESM_EBR_ACTIVE_PENDING,   /* The network has initiated an EPS bearer context activation towards the UE    */
//  ESM_EBR_STATE_MAX
//} esm_ebr_state;
//
///* ESM message timer retransmission data */
//typedef struct esm_ebr_timer_data_s {
//  struct emm_context_s  *ctx;
//  mme_ue_s1ap_id_t       ue_id;      /* Lower layers UE identifier       */
//  ebi_t                  ebi;        /* EPS bearer identity          */
//  unsigned int           count;      /* Retransmission counter       */
//  bstring                msg;        /* Encoded ESM message to re-transmit   */
//} esm_ebr_timer_data_t;

///*
// * -----------------------
// * EPS bearer context data
// * -----------------------
// */
//typedef struct esm_ebr_context_s {
//  //ebi_t                           ebi;      /* EPS bearer identity          */
//  esm_ebr_state                     status;   /* EPS bearer context status        */
//  bitrate_t                         gbr_dl;
//  bitrate_t                         gbr_ul;
//  bitrate_t                         mbr_dl;
//  bitrate_t                         mbr_ul;
//  traffic_flow_template_t          *tft;
//  protocol_configuration_options_t *pco;
//  nas_timer_t                       timer;   /* Retransmission timer         */
//  esm_ebr_timer_data_t             *args; /* Retransmission timer parameters data */
//} esm_ebr_context_t;
//
//typedef struct esm_ebr_data_s {
//  unsigned char index;    /* Index of the next EPS bearer context
//                 * identity to be used */
//#define ESM_EBR_DATA_SIZE (ESM_EBI_MAX - ESM_EBI_MIN + 1)
//  esm_ebr_context_t *context[ESM_EBR_DATA_SIZE + 1];
//} esm_ebr_data_t;
//
//#define BEARER_STATE_NULL        0
//#define BEARER_STATE_SGW_CREATED (1 << 0)
//#define BEARER_STATE_MME_CREATED (1 << 1)
//#define BEARER_STATE_ENB_CREATED (1 << 2)
//#define BEARER_STATE_ACTIVE      (1 << 3)
//#define BEARER_STATE_S1_RELEASED (1 << 4)
//
//typedef uint8_t mme_app_bearer_state_t;
//
//
///** @struct bearer_context_t
// *  @brief Parameters that should be kept for an eps bearer.
// */
//typedef struct bearer_context_s {
//  // EPS Bearer ID: An EPS bearer identity uniquely identifies an EP S bearer for one UE accessing via E-UTRAN
//  ebi_t                       ebi;
//
//  // TI Transaction Identifier
//  proc_tid_t                  transaction_identifier;
//
//  // S-GW IP address for S1-u: IP address of the S-GW for the S1-u interfaces.
//  // S-GW TEID for S1u: Tunnel Endpoint Identifier of the S-GW for the S1-u interface.
//  fteid_t                      s_gw_fteid_s1u;            // set by S11 CREATE_SESSION_RESPONSE
//
//  // PDN GW TEID for S5/S8 (user plane): P-GW Tunnel Endpoint Identifier for the S5/S8 interface for the user plane. (Used for S-GW change only).
//  // NOTE:
//  // The PDN GW TEID is needed in MME context as S-GW relocation is triggered without interaction with the source S-GW, e.g. when a TAU
//  // occurs. The Target S-GW requires this Information Element, so it must be stored by the MME.
//  // PDN GW IP address for S5/S8 (user plane): P GW IP address for user plane for the S5/S8 interface for the user plane. (Used for S-GW change only).
//  // NOTE:
//  // The PDN GW IP address for user plane is needed in MME context as S-GW relocation is triggered without interaction with the source S-GW,
//  // e.g. when a TAU occurs. The Target S GW requires this Information Element, so it must be stored by the MME.
//  fteid_t                      p_gw_fteid_s5_s8_up;
//
//  // EPS bearer QoS: QCI and ARP, optionally: GBR and MBR for GBR bearer
//  qci_t                       qci;
//
//  // TFT: Traffic Flow Template. (For PMIP-based S5/S8 only)
//  //traffic_flow_template_t          *tft_pmip;
//
//  // extra 23.401 spec members
//  pdn_cid_t                         pdn_cx_id;
//  mme_app_bearer_state_t            bearer_state;
//  esm_ebr_context_t                 esm_ebr_context;
//  fteid_t                           enb_fteid_s1u;
//
//  /* QoS for this bearer */
//  priority_level_t            priority_level;
//  pre_emption_vulnerability_t preemption_vulnerability;
//  pre_emption_capability_t    preemption_capability;
//
//} bearer_context_t;


#endif  /* FILE_MME_IE_DEFS_SEEN */


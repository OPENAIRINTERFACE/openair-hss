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
#ifndef FILE_MME_APP_ESM_PROCEDURES_SEEN
#define FILE_MME_APP_ESM_PROCEDURES_SEEN

/*! \file mme_app_esm_procedures.h
  \brief
  \author Dincer Beken
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/


////////////////////////////////////////////////////////////////////////////////
// ESM procedures
////////////////////////////////////////////////////////////////////////////////


/*
 * Define callbacks instead of checking the message type in the lower layer and accessing EMM functionalities.
 * Also the EMM context will enter COMMON state and create a new GUTI after this callback.
 */

typedef int (*esm_failure_cb_t)(struct nas_esm_base_proc_s*);
/** Method called inside the timeout. */
typedef int (*esm_timeout_cb_t)(struct nas_esm_base_proc_s *);


typedef enum {
  ESM_PROC_NONE = 0,
  ESM_PROC_EPS_BEARER_CONTEXT,
  ESM_PROC_PDN_CONTEXT
} esm_proc_type_t;

//
//typedef enum {
//  MME_APP_BASE_PROC_TYPE_NONE = 0,
//  MME_APP_BASE_PROC_TYPE_S1AP,
//  MME_APP_BASE_PROC_TYPE_S10,
//  MME_APP_BASE_PROC_TYPE_S11
//} mme_app_base_proc_type_t;
//
//
//typedef struct mme_app_base_proc_s {
//  // PDU interface
//  //pdu_in_resp_t              resp_in;
//  //pdu_in_rej_t               fail_in;
//  time_out_t                 time_out;
//  mme_app_base_proc_type_t   type;
//} mme_app_base_proc_t;
//
///* S10 */
//typedef enum {
//  MME_APP_S10_PROC_TYPE_NONE = 0,
//  MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER,
//  MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER
//} mme_app_s10_proc_type_t;
//
//typedef struct mme_app_s10_proc_s {
//  mme_app_base_proc_t         proc;
//  mme_app_s10_proc_type_t     type;
//  struct mme_app_timer_t      timer;
//
//  /** S10 Tunnel Endpoint information. */
//  teid_t                      local_teid;
//  teid_t                      remote_teid;
//  struct in_addr              peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME.
//  bool                        target_mme;
//  uintptr_t                   s10_trxn;
//  LIST_ENTRY(mme_app_s10_proc_s) entries;      /* List. */
//} mme_app_s10_proc_t;
//
///*
// * S10 Procedure for Handover only on the target-MME side.
// * On the source MME we don't need a procedure, a timer is enough.
// */
//typedef struct mme_app_s10_proc_mme_handover_s {
//  mme_app_s10_proc_t            proc;
//
//  mme_ue_s1ap_id_t              mme_ue_s1ap_id;
//
//  enb_ue_s1ap_id_t              source_enb_ue_s1ap_id;
//  enb_ue_s1ap_id_t              target_enb_ue_s1ap_id;
//  time_out_t                   *s10_mme_handover_timeout;
//
//  uintptr_t                     forward_relocation_trxn;
//  /** Peer Information. */
//  fteid_t                       remote_mme_teid;
//  uint16_t                      peer_port;
//  target_identification_t       target_id;
//  F_Container_t                 source_to_target_eutran_f_container;
//  F_Cause_t                     f_cause;
//  /** NAS context information. */
//  nas_s10_context_t             nas_s10_context;
//  /** PDN Connections. */
//  mme_ue_eps_pdn_connections_t *pdn_connections;
//  uint8_t                       next_processed_pdn_connection;
//
//  /** Target Information to store on the source side. */
////  S1ap_ENB_ID_PR                target_enb_type;
//
//  tai_t                         target_tai;
//  bool                          ho_command_sent;
//  ecgi_t                        source_ecgi;  /**< Source home/macro enb id. */
//  ecgi_t                        target_ecgi;  /**< Target home/macro enb id. */
//  bool                          pending_clear_location_request;
//
//  LIST_ENTRY(mme_app_handover_proc_s) entries;      /* List. */
//} mme_app_s10_proc_mme_handover_t;
//
///* S11 */
//typedef enum {
//  MME_APP_S11_PROC_TYPE_NONE = 0,
//  MME_APP_S11_PROC_TYPE_CREATE_BEARER,
//  MME_APP_S11_PROC_TYPE_UPDATE_BEARER,
//  MME_APP_S11_PROC_TYPE_DELETE_BEARER
//} mme_app_s11_proc_type_t;


// TODO: MIGHT REVIEW
//
//typedef struct esm_procedures_s {
//  /** UE/PTI triggered ESM procedure. */
//  LIST_HEAD(nas_esm_transaction_procedures_head_s, nas_esm_transaction_procedures_s)  esm_transaction_procs;
////  /** CN Triggered ESM procedure. */
////  LIST_HEAD(nas_esm_bearer_context_procedures_head_s, nas_esm_bearer_context_procedure_s)  esm_bearer_context_procs;
//} esm_procedures_t;

typedef struct nas_esm_proc_s {
  mme_ue_s1ap_id_t             ue_id;
  esm_timeout_cb_t             timeout_notif;
  esm_proc_type_t              type;
//  esm_transaction_proc_type_t esm_procedure_type;
  nas_timer_t                  esm_proc_timer;
  uint8_t                      retx_count;
  pti_t                        pti;
} nas_esm_proc_t;

/*
 * Structure for ESM PDN connectivity and disconnectivity procedure.
 */
typedef struct nas_esm_proc_pdn_connectivity_s {
  /** Initial mandatory elements. */
  nas_esm_proc_t               esm_base_proc;
  bool                         is_attach;
  esm_proc_pdn_type_t          pdn_type;
  esm_proc_pdn_request_t       request_type;
  imsi_t                       imsi;
  /** Additional elements requested from the UE and set with time.. */
  bstring                      apn_subscribed;
  tai_t                        visited_tai;
  pdn_cid_t                    pdn_cid;
  ebi_t                        default_ebi;
  //  protocol_configuration_options_t  pco;
  LIST_ENTRY(nas_esm_proc_pdn_connectivity_s) entries;      /* List. */
} nas_esm_proc_pdn_connectivity_t;


/*
 * Structure for ESM bearer context procedure.
 */
typedef struct nas_esm_proc_bearer_context_s {
  /** Initial mandatory elements. */
  nas_esm_proc_t               esm_base_proc;  /**< It may be a transactional procedure (PTI set // triggered through resource modification, or not (pti=0), triggered by the core network. */
  imsi_t                       imsi;
  /** Additional elements requested from the UE and set with time.. */
  pdn_cid_t                    pdn_cid;
  ebi_t                        bearer_ebi;
  ebi_t                        linked_ebi;
  teid_t                       s1u_saegw_teid;
  bstring                      subscribed_apn;
//  protocol_configuration_options_t  pco;
  LIST_ENTRY(nas_esm_proc_bearer_context_s) entries;      /* List. */
} nas_esm_proc_bearer_context_t;

typedef enum {
  S11_PROC_BEARER_UNKNOWN  = 0,
  S11_PROC_BEARER_PENDING  = 1,
  S11_PROC_BEARER_FAILED   = 2,
  S11_PROC_BEARER_SUCCESS  = 3
} s11_proc_bearer_status_t;

//typedef struct mme_app_s11_proc_create_bearer_s {
//  mme_app_s11_proc_t           proc;
//  int                          num_bearers_unhandled;
//  int                          num_status_received;
//
//  ebi_t                        linked_ebi;
//  pdn_cid_t                    pci;
//
//  // TODO here give a NAS/S1AP/.. reason -> GTPv2-C reason
//  bearer_contexts_to_be_created_t *bcs_tbc; /**< Store the bearer contexts to be created here, and don't register them yet in the MME_APP context. */
//} mme_app_s11_proc_create_bearer_t;


// todo: put them temporary in MME_APP UE context
///* Declaration (prototype) of the function to store bearer contexts. */
//RB_PROTOTYPE(BearerFteids, fteid_set_s, fteid_set_rbt_Node, fteid_set_compare_s1u_saegw)
//------------------------------------------------------------------------------
/*
 * PDN Connectivity Procedures.
 */
void mme_app_nas_esm_free_pdn_connectivity_procedures(ue_context_t * const ue_context_p);
nas_esm_proc_pdn_connectivity_t* mme_app_nas_esm_create_pdn_connectivity_procedure(mme_ue_s1ap_id_t mme_ue_s1ap_id, pti_t pti);
nas_esm_proc_pdn_connectivity_t* mme_app_nas_esm_get_pdn_connectivity_procedure(mme_ue_s1ap_id_t mme_ue_s1ap_id);
void mme_app_nas_esm_free_pdn_connectivity_proc(nas_esm_proc_pdn_connectivity_t **esm_pdn_connectivity_proc);

//------------------------------------------------------------------------------
/*
 * ESM Bearer Context Procedures.
 */
void mme_app_nas_esm_free_bearer_context_procedures(ue_context_t * const ue_context_p); // todo: static!
nas_esm_proc_bearer_context_t* mme_app_nas_esm_create_bearer_context_procedure(mme_ue_s1ap_id_t mme_ue_s1ap_id);
nas_esm_proc_bearer_context_t* mme_app_nas_esm_get_bearer_context_procedure(mme_ue_s1ap_id_t mme_ue_s1ap_id);
void mme_app_nas_esm_free_bearer_context_proc(nas_esm_proc_bearer_context_t     **esm_bearer_context_proc);

#endif

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
#ifndef FILE_MME_APP_PROCEDURES_SEEN
#define FILE_MME_APP_PROCEDURES_SEEN

// todo: #define MME_APP_INITIAL_CONTEXT_SETUP_RSP_TIMER_VALUE 2 // In seconds
/* Timer structure */
struct mme_app_timer_t {
  long id;         /* The timer identifier                 */
  long sec;       /* The timer interval value in seconds  */
};

/*! \file mme_app_procedures.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

//typedef int (*mme_app_pdu_in_resp_t)(void *arg);
//typedef int (*mme_app_pdu_in_rej_t)(void *arg);
//typedef int (*mme_app_time_out_t)(void *arg);
//typedef int (*mme_app_sdu_out_not_delivered_t)(void *arg);


typedef enum {
  MME_APP_BASE_PROC_TYPE_NONE = 0,
  MME_APP_BASE_PROC_TYPE_S1AP,
  MME_APP_BASE_PROC_TYPE_S10,
  MME_APP_BASE_PROC_TYPE_S11
} mme_app_base_proc_type_t;


typedef struct mme_app_base_proc_s {
  // PDU interface
  //pdu_in_resp_t              resp_in;
  //pdu_in_rej_t               fail_in;
  time_out_t                 time_out;
  mme_app_base_proc_type_t   type;
  bool						 in_progress;
  int						 in_progress_count;
} mme_app_base_proc_t;

/* S10 */
typedef enum {
  MME_APP_S10_PROC_TYPE_NONE = 0,
  MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER,
  MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER
} mme_app_s10_proc_type_t;

typedef struct mme_app_s10_proc_s {
  mme_app_base_proc_t         proc;
  mme_app_s10_proc_type_t     type;
  struct mme_app_timer_t      timer;

  /** S10 Tunnel Endpoint information. */
  teid_t                      local_teid;
  teid_t                      remote_teid;

  struct sockaddr             *peer_ip;             ///< MME ipv4 address for S-GW or S-GW ipv4 address for MME.

  bool                        target_mme;
  uintptr_t                   s10_trxn;
  LIST_ENTRY(mme_app_s10_proc_s) entries;      /* List. */
} mme_app_s10_proc_t;

/*
 * S10 Procedure for Handover only on the target-MME side.
 * On the source MME we don't need a procedure, a timer is enough.
 */
typedef struct mme_app_s10_proc_mme_handover_s {
  mme_app_s10_proc_t            proc;

  mme_ue_s1ap_id_t              mme_ue_s1ap_id;
  enb_ue_s1ap_id_t              source_enb_ue_s1ap_id;
  enb_ue_s1ap_id_t              target_enb_ue_s1ap_id;
  time_out_t                   *s10_mme_handover_timeout;

  uintptr_t                     forward_relocation_trxn;
  /** Peer Information. */
  fteid_t                       remote_mme_teid;
//  struct sockaddr				*peer_addr; /**< Actually used address. */
  uint16_t                      peer_port;
  target_identification_t       target_id;
  F_Container_t                 source_to_target_eutran_f_container;
  F_Cause_t                     f_cause;
  /** NAS context information. */
  nas_s10_context_t             nas_s10_context;
  /** PDN Connections. */
  mme_ue_eps_pdn_connections_t *pdn_connections;

  ebi_list_t                    failed_ebi_list;
  imsi_t                        imsi;

  /** Target Information to store on the source side. */
  //  S1ap_ENB_ID_PR                target_enb_type;

  tai_t                         target_tai;
  bool                          ho_command_sent;
  ecgi_t                        source_ecgi;  /**< Source home/macro enb id. */
  ecgi_t                        target_ecgi;  /**< Target home/macro enb id. */
  bool                          pending_clear_location_request;
  bool                          due_tau;

  /** Flags just for Tester imperfections. */
  bool 							mme_status_context_handled;
  bool 							received_early_ho_notify;
  bool 							handover_completed; /*< Because S9 completing TAU before handover completes. */

  LIST_ENTRY(mme_app_handover_proc_s) entries;      /* List. */
} mme_app_s10_proc_mme_handover_t;

/* S11 */
typedef enum {
  MME_APP_S11_PROC_TYPE_NONE = 0,
  MME_APP_S11_PROC_TYPE_CREATE_BEARER,
  MME_APP_S11_PROC_TYPE_UPDATE_BEARER,
  MME_APP_S11_PROC_TYPE_DELETE_BEARER
} mme_app_s11_proc_type_t;

typedef struct mme_app_s11_proc_s {
  mme_app_base_proc_t         proc;
  int                         num_bearers_unhandled;
  mme_app_s11_proc_type_t     type;
  pti_t					      pti;
  uintptr_t                   s11_trxn;
  LIST_ENTRY(mme_app_s11_proc_s) entries;      /* List. */
} mme_app_s11_proc_t;

typedef struct mme_app_s11_proc_create_bearer_s {
  mme_app_s11_proc_t           proc;
  int                          num_status_received;

  ebi_t                        linked_ebi;
  pdn_cid_t                    pci;

  // TODO here give a NAS/S1AP/.. reason -> GTPv2-C reason
  bearer_contexts_to_be_created_t *bcs_tbc; /**< Store the bearer contexts to be created here, and don't register them yet in the MME_APP context. */
} mme_app_s11_proc_create_bearer_t;

typedef struct mme_app_s11_proc_update_bearer_s {
  mme_app_s11_proc_t           		 proc;
  int                          		 num_status_received;

  pdn_cid_t                    		 pci;
  pti_t                        		 pti;
  ambr_t                       		 new_used_ue_ambr;
  ambr_t					   					 		 apn_ambr;
  ebi_t                        		 linked_ebi;
  // TODO here give a NAS/S1AP/.. reason -> GTPv2-C reason
  bearer_contexts_to_be_updated_t *bcs_tbu; /**< Store the bearer contexts to be created here, and don't register them yet in the MME_APP context. */
} mme_app_s11_proc_update_bearer_t;

typedef struct mme_app_s11_proc_delete_bearer_s {
  mme_app_s11_proc_t           proc;
  ebi_t                        linked_ebi;
  int                          num_status_received;
  ebi_list_t                   ebis;

  // TODO here give a NAS/S1AP/.. reason -> GTPv2-C reason
  bearer_contexts_to_be_removed_t bcs_failed; /**< Store the bearer contexts to be created here, and don't register them yet in the MME_APP context. */
} mme_app_s11_proc_delete_bearer_t;

#endif /** FILE_MME_APP_PROCEDURES_SEEN **/

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

/** Method called inside the timeout. */
typedef void (*esm_timeout_cb_t)(struct nas_esm_proc_s *);


typedef enum {
  ESM_PROC_NONE = 0,
  ESM_PROC_EPS_BEARER_CONTEXT,
  ESM_PROC_PDN_CONTEXT
} esm_proc_type_t;


/* Type of PDN address */
typedef enum {
  ESM_PDN_TYPE_IPV4 = NET_PDN_TYPE_IPV4,
  ESM_PDN_TYPE_IPV6 = NET_PDN_TYPE_IPV6,
  ESM_PDN_TYPE_IPV4V6 = NET_PDN_TYPE_IPV4V6
} esm_proc_pdn_type_t;

/* Type of PDN request */
typedef enum {
  ESM_PDN_REQUEST_INITIAL = 1,
  ESM_PDN_REQUEST_HANDOVER,
  ESM_PDN_REQUEST_EMERGENCY
} esm_proc_pdn_request_t;

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
  bstring                      subscribed_apn; /**< Should be copied with bstrcpy(). */
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
  /** Additional elements requested from the UE and set with time.. */
  pdn_cid_t                    pdn_cid;
  ebi_t                        bearer_ebi;
  ebi_t                        linked_ebi;
  teid_t                       s1u_saegw_teid;
  ambr_t                       apn_ambr;
  traffic_flow_template_t     *tft;
  bearer_qos_t                 bearer_level_qos;
//  protocol_configuration_options_t  pco;
  LIST_ENTRY(nas_esm_proc_bearer_context_s) entries;      /* List. */
} nas_esm_proc_bearer_context_t;


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
nas_esm_proc_pdn_connectivity_t* mme_app_nas_esm_get_pdn_connectivity_procedure(mme_ue_s1ap_id_t mme_ue_s1ap_id, pti_t pti);
void mme_app_nas_esm_free_pdn_connectivity_proc(nas_esm_proc_pdn_connectivity_t **esm_pdn_connectivity_proc);

//------------------------------------------------------------------------------
/*
 * ESM Bearer Context Procedures.
 */
void mme_app_nas_esm_free_bearer_context_procedures(ue_context_t * const ue_context_p); // todo: static!
nas_esm_proc_bearer_context_t* mme_app_nas_esm_create_bearer_context_procedure(mme_ue_s1ap_id_t ue_id, pti_t pti, ebi_t ebi, int timeout_sec, int timeout_usec, esm_timeout_cb_t timeout_notif, void * arg);
nas_esm_proc_bearer_context_t* mme_app_nas_esm_get_bearer_context_procedure(mme_ue_s1ap_id_t mme_ue_s1ap_id, pti_t pti, ebi_t ebi);
void mme_app_nas_esm_free_bearer_context_proc(nas_esm_proc_bearer_context_t     **esm_bearer_context_proc);

#endif

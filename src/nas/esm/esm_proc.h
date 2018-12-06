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

/*****************************************************************************
Source      esm_proc.h

Version     0.1

Date        2013/01/02

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines the EPS Session Management procedures executed at
        the ESM Service Access Points.

*****************************************************************************/
#ifndef __ESM_PROC_H__
#define __ESM_PROC_H__

#include "networkDef.h"
#include "common_defs.h"
#include "common_types.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/


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

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *              ESM status procedure
 * --------------------------------------------------------------------------
 */

int esm_proc_status_ind(struct esm_context_s * esm_context, const proc_tid_t pti, ebi_t ebi, esm_cause_t *esm_cause);
int esm_proc_status(const bool is_standalone, struct esm_context_s * const esm_context, const ebi_t ebi,
    bstring *msg, const bool sent_by_ue);


/*
 * --------------------------------------------------------------------------
 *          PDN connectivity procedure
 * --------------------------------------------------------------------------
 */
/**
 * PDN Connectivity procedure (UE triggered - incl. initial attach).
 */
nas_esm_pdn_connectivity_proc_t *_esm_proc_create_pdn_connectivity_procedure(mme_ue_s1ap_id_t ue_id, imsi_t *imsi, pti_t pti);
void _esm_proc_free_pdn_connectivity_procedure(nas_esm_pdn_connectivity_proc_t * nas_pdn_connectivity_proc);

int
esm_proc_pdn_connectivity_request (
  mme_ue_s1ap_id_t             ue_id,
  imsi_t                      *imsi,
  tai_t                       *visited_tai,
  const proc_tid_t             pti,
  const apn_configuration_t   *apn_configuration,
  const esm_proc_pdn_request_t request_type,
  const_bstring                const apn_subscribed,
  esm_proc_pdn_type_t          pdn_type);

int esm_proc_pdn_connectivity_failure(struct esm_context_s * esm_context, pdn_cid_t pid, ebi_t default_ebi);

int esm_proc_pdn_config_res(mme_ue_s1ap_id_t ue_id, nas_esm_pdn_connectivity_proc_t *nas_pdn_connectivity_proc, imsi64_t imsi, ESM_msg *esm_resp_msg);

int esm_proc_pdn_config_fail(mme_ue_s1ap_id_t ue_id, nas_esm_pdn_connectivity_proc_t *nas_pdn_connectivity_proc, ESM_msg *esm_resp_msg);

int esm_proc_pdn_connectivity_res(mme_ue_s1ap_id_t ue_id, nas_esm_pdn_connectivity_proc_t *nas_pdn_connectivity_proc, imsi64_t imsi, ESM_msg *esm_resp_msg);

int esm_proc_pdn_connectivity_fail(mme_ue_s1ap_id_t ue_id, nas_esm_pdn_connectivity_proc_t *nas_pdn_connectivity_proc, imsi64_t imsi, ESM_msg *esm_resp_msg);

/*
 * --------------------------------------------------------------------------
 *              PDN disconnect procedure
 * --------------------------------------------------------------------------
 */

int esm_proc_pdn_disconnect_request(struct esm_context_s * esm_context, const proc_tid_t pti, pdn_cid_t  pdn_cid, ebi_t default_ebi, esm_cause_t *esm_cause);

int esm_proc_pdn_disconnect_accept(struct esm_context_s * esm_context, pdn_cid_t pid, ebi_t default_ebi, esm_cause_t *esm_cause);
int esm_proc_pdn_disconnect_reject(const bool is_standalone, struct esm_context_s * esm_context,
    ebi_t ebi, bstring *msg, const bool ue_triggered);


/*
 * --------------------------------------------------------------------------
 *              ESM information procedure
 * --------------------------------------------------------------------------
 */

int esm_proc_esm_information_request (nas_esm_transaction_proc_t *esm_trx_proc, ESM_msg * esm_result_msg);

int esm_proc_esm_information_response (struct esm_context_s * ue_context, pti_t pti, const_bstring const apn, const protocol_configuration_options_t * const pco, esm_cause_t * const esm_cause);

/*
 * --------------------------------------------------------------------------
 *      Default EPS bearer context activation procedure
 * --------------------------------------------------------------------------
 */
int esm_proc_default_eps_bearer_context (
  mme_ue_s1ap_id_t   ue_id,
  const nas_esm_pdn_connectivity_proc_t * nas_pdn_connectivity_proc,
  bearer_qos_t *bearer_qos,
  ambr_t       *apn_ambr,
  ESM_msg *esm_rsp_msg,
  esm_cause_t *esm_cause);

int esm_proc_default_eps_bearer_context_request(bool is_standalone, struct esm_context_s * const esm_context, const ebi_t ebi, STOLEN_REF bstring *msg, const bool ue_triggered);
int esm_proc_default_eps_bearer_context_failure (struct esm_context_s * esm_context, pdn_cid_t * const pid, ebi_t *ebi);

int esm_proc_default_eps_bearer_context_accept(struct esm_context_s * esm_context, ebi_t ebi, esm_cause_t *esm_cause);
int esm_proc_default_eps_bearer_context_reject(struct esm_context_s * esm_context, ebi_t ebi, esm_cause_t *esm_cause);


/*
 * --------------------------------------------------------------------------
 *      Dedicated EPS bearer context activation procedure
 * --------------------------------------------------------------------------
 */

int esm_proc_dedicated_eps_bearer_context( struct esm_context_s * esm_context,
    ebi_t  default_ebi,
    const proc_tid_t   pti,                  // todo: will always be 0 for network initiated bearer establishment.
    const pdn_cid_t    pdn_cid,              /**< todo: Per APN for now. */
    bearer_context_to_be_created_t *bc_tbc,
    esm_cause_t *esm_cause);

int esm_proc_dedicated_eps_bearer_context_request(const bool is_standalone, struct esm_context_s * const esm_context, const ebi_t ebi, STOLEN_REF bstring *msg, const bool ue_triggered);

int esm_proc_dedicated_eps_bearer_context_accept(struct esm_context_s * esm_context, ebi_t ebi, esm_cause_t *esm_cause);
int esm_proc_dedicated_eps_bearer_context_reject(struct esm_context_s * esm_context, ebi_t ebi, esm_cause_t *esm_cause);


/*
 * --------------------------------------------------------------------------
 *      EPS bearer context modification procedure
 * --------------------------------------------------------------------------
 */
int esm_proc_modify_eps_bearer_context( struct esm_context_s * esm_context,
    const proc_tid_t   pti,                  // todo: will always be 0 for network initiated bearer establishment.
    bearer_context_to_be_updated_t *bc_tbu,
    esm_cause_t *esm_cause);

int
esm_proc_update_eps_bearer_context (
  struct esm_context_s * esm_context,
  const bearer_context_to_be_updated_t *bc_tbu);

int esm_proc_modify_eps_bearer_context_request(const bool is_standalone, struct esm_context_s * const esm_context, const ebi_t ebi, STOLEN_REF bstring *msg, const bool ue_triggered);

int esm_proc_modify_eps_bearer_context_accept(struct esm_context_s * esm_context, ebi_t ebi, esm_cause_t *esm_cause);
int esm_proc_modify_eps_bearer_context_reject(struct esm_context_s * esm_context, ebi_t ebi, esm_cause_t *esm_cause, bool ue_requested);


/*
 * --------------------------------------------------------------------------
 *      EPS bearer context deactivation procedure
 * --------------------------------------------------------------------------
 */
int esm_proc_eps_bearer_context_deactivate(struct esm_context_s * const esm_context,const bool is_local, const ebi_t ebi,pdn_cid_t pid, esm_cause_t * const esm_cause);
int esm_proc_eps_bearer_context_deactivate_request(const bool is_standalone, struct esm_context_s * const esm_context, const ebi_t ebi, STOLEN_REF bstring *msg, const bool ue_triggered);
pdn_cid_t esm_proc_eps_bearer_context_deactivate_accept(struct esm_context_s * esm_context, ebi_t ebi, esm_cause_t *esm_cause);

#endif /* __ESM_PROC_H__*/

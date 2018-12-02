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

/*! \file nas_esm_procedures.h
   \brief
   \author  Lionel GAUTHIER
   \date 2017
   \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_NAS_ESM_PROCEDURES_SEEN
#define FILE_NAS_ESM_PROCEDURES_SEEN

struct nas_esm_base_proc_s;

typedef int (*esm_success_cb_t)(struct esm_context_s*);
typedef int (*esm_failure_cb_t)(struct esm_context_s*);

typedef int (*esm_proc_abort_t)(struct esm_context_s*, struct nas_esm_base_proc_s*);

typedef int (*esm_pdu_in_resp_t)(struct esm_context_s*, void *arg); // can be RESPONSE, COMPLETE, ACCEPT
typedef int (*esm_pdu_in_rej_t)(struct esm_context_s*, void *arg);  // REJECT.
typedef int (*esm_pdu_out_rej_t)(struct esm_context_s*, struct nas_esm_base_proc_s *);  // REJECT.
typedef void (*esm_time_out_t)(void *arg);

typedef struct nas_esm_base_proc_s {
  esm_success_cb_t            success_notif;
  esm_failure_cb_t            failure_notif;
  esm_proc_abort_t            abort;

  // PDU interface
  //pdu_in_resp_t           resp_in;
  esm_pdu_in_rej_t            fail_in;
  esm_pdu_out_rej_t           fail_out;
  esm_time_out_t              time_out;
  uint64_t                nas_puid; // procedure unique identifier for internal use

  struct nas_esm_base_proc_s *parent;
  struct nas_esm_base_proc_s *child;
} nas_esm_base_proc_t;

////////////////////////////////////////////////////////////////////////////////
// ESM procedures
////////////////////////////////////////////////////////////////////////////////
typedef enum {
  ESM_PROC_NONE = 0,
  ESM_PROC_EPS_BEARER_CONTEXT,
  ESM_PROC_TRANSACTION
} esm_proc_type_t;

typedef struct nas_esm_proc_s {
  nas_esm_base_proc_t         base_proc;
  esm_proc_type_t             type;
} nas_esm_proc_t;

//typedef enum {
//  ESM_BEARER_CTX_PROC_NONE = 0,
//  ESM_PROC_DEFAULT_EPS_BEARER_CTXT_ACTIVATION,
//  ESM_PROC_DEDICATED_EPS_BEARER_CTXT_ACTIVATION,
//  ESM_PROC_EPS_BEARER_CTXT_MODIFICATION,
//  ESM_PROC_EPS_BEARER_CTXT_DEACTIVATION
//} esm_bearer_ctx_proc_type_t;

typedef enum {
  ESM_TRANSACTION_PROC_NONE = 0,
  ESM_PROC_TRANSACTION_PDN_CONNECTIVITY,
  ESM_PROC_TRANSACTION_PDN_DISCONNECT,
  ESM_PROC_TRANSACTION_BEARER_RESOURCE_ALLOCATION,
  ESM_PROC_TRANSACTION_BEARER_RESOURCE_MODIFICATION,
} esm_transaction_proc_type_t;

typedef struct nas_esm_transaction_proc_s {
  nas_esm_proc_t               esm_proc;
  esm_transaction_proc_type_t  type;
} nas_esm_transaction_proc_t;

typedef struct nas_esm_transaction_procedures_s {
  nas_esm_transaction_proc_t                          *proc;
  LIST_ENTRY(nas_esm_transaction_procedures_s)  entries;
} nas_esm_transaction_procedures_t;

typedef struct esm_procedures_s {
  /** UE/PTI triggered ESM procedure. */
  LIST_HEAD(nas_esm_transaction_procedures_head_s, nas_esm_transaction_procedures_s)  esm_transaction_procs;
//  /** CN Triggered ESM procedure. */
//  LIST_HEAD(nas_esm_bearer_context_procedures_head_s, nas_esm_bearer_context_procedure_s)  esm_bearer_context_procs;
} esm_procedures_t;

////////////////////////////////////////////////////////////////////////////////
// Mix them (kind of class hierarchy--)
////////////////////////////////////////////////////////////////////////////////
typedef union {
  nas_esm_base_proc_t         base_proc;
  nas_esm_proc_t              esm_proc;
} nas_proc_esm_t;

int nas_unlink_esm_procedures(nas_esm_base_proc_t * const parent_proc, nas_esm_base_proc_t * const child_proc);

///** New ESM Bearer Context procedure. */
//nas_esm_bearer_ctx_proc_t *nas_new_esm_bearer_context_procedure(struct esm_context_s * const esm_context);
//nas_esm_bearer_context_procedure_t  *get_esm_bearer_context_procedure(const struct esm_context_s * const ctxt, esm_bearer_ctx_proc_type_t esm_bc_proc_type); /**< Only one procedure can exist at a time. */
//nas_esm_transaction_proc_t *get_esm_transaction_procedure(const struct esm_context_s * const ctxt, esm_transaction_proc_type_t esm_trx_proc_type); /**< Only one procedure can exist at a time. */
//void nas_delete_all_esm_procedures(const struct esm_context_s * const esm_context);
//void _nas_delete_esm_transaction_procedures(const struct esm_context_s *esm_context, nas_esm_transaction_proc_t ** trx_proc);
////void _nas_delete_esm_bearer_context_procedures(struct esm_context_s *esm_context, nas_esm_bearer_ctx_proc_t * bc_proc);

#endif

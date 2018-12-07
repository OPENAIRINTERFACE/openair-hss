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

/*! \file nas_procedures.c
   \brief
   \author  Lionel GAUTHIER
   \date 2017
   \email: lionel.gauthier@eurecom.fr
*/



#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"
#include "gcc_diag.h"
#include "dynamic_memory_check.h"
#include "assertions.h"
#include "log.h"
#include "msc.h"
#include "common_types.h"
#include "3gpp_24.008.h"
#include "3gpp_36.401.h"
#include "3gpp_29.274.h"
#include "conversions.h"
#include "3gpp_requirements_24.301.h"
#include "nas_message.h"
#include "as_message.h"
#include "mme_app_ue_context.h"
#include "networkDef.h"
#include "mme_api.h"
#include "mme_config.h"
#include "mme_app_defs.h"
#include "nas_esm_procedures.h"
#include "esm_proc.h"
#include "esm_sap.h"
#include "NasSecurityAlgorithms.h"
#include "nas_timer.h"
//#include "digest.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ESM PROCEDURES                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
static esm_procedures_t *_nas_new_esm_procedures(struct esm_context_s * const esm_context)
{
//  esm_procedures_t *esm_procedures = calloc(1, sizeof(*esm_context->esm_procedures));
////  LIST_INIT(&esm_procedures->esm_bearer_context_procs);
//  LIST_INIT(&esm_procedures->esm_transaction_procs);
//  return esm_procedures;
}

////-----------------------------------------------------------------------------
//nas_esm_bearer_ctx_proc_t *nas_new_esm_bearer_context_procedure(struct emm_data_context_s * const emm_context)
//{
//  if (!(emm_context->esm_ctx.esm_procedures)) {
//    emm_context->esm_ctx.esm_procedures = _nas_new_esm_procedures(&emm_context->esm_ctx);
//  }
//
//  nas_esm_bearer_ctx_proc_t * bc_proc =  calloc(1, sizeof(nas_esm_bearer_ctx_proc_t));
//
//  bc_proc->esm_proc.base_proc.nas_puid = __sync_fetch_and_add (&nas_puid, 1);
//  bc_proc->esm_proc.base_proc.type = NAS_PROC_TYPE_EMM;
//  bc_proc->esm_proc.type           = ESM_PROC_EPS_BEARER_CONTEXT;
//  bc_proc->type                    = ESM_PROC_DEDICATED_EPS_BEARER_CTXT_ACTIVATION;
//
//  LIST_INIT(&bc_proc->pending_bearers);
//
//  /** No special ESM timers needed for the procedure. The timers are set per ESM Bearer Activation Request message. The last message, or the last timer will trigger a timeout. */
//
//  nas_esm_bearer_context_procedure_t * wrapper = calloc(1, sizeof(*wrapper));
//  if (wrapper) {
//    wrapper->proc = bc_proc;
//    LIST_INSERT_HEAD(&emm_context->esm_ctx.esm_procedures->esm_bearer_context_procs, wrapper, entries);
//    OAILOG_TRACE (LOG_NAS_EMM, "New ESM_BEARER_CONTEXT procedure. \n");
//    return bc_proc;
//  } else {
//    free_wrapper((void**)&bc_proc);
//  }
//  return NULL;
//}

//-----------------------------------------------------------------------------
static void nas_esm_procedure_gc(struct esm_context_s * const esm_context)
{
//  if (
////      LIST_EMPTY(&esm_context->esm_procedures->esm_bearer_context_procs) &&
//       LIST_EMPTY(&esm_context->esm_procedures->esm_transaction_procs) ) {
//    free_wrapper((void**)&esm_context->esm_procedures);
//  }
}

////-----------------------------------------------------------------------------
//static void nas_delete_esm_bearer_context_procedure(struct emm_data_context_s *emm_context, nas_esm_bearer_ctx_proc_t ** bearer_context_proc)
//{
//  if (*bearer_context_proc) {
//   OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " ESM Bearer context procedure. \n", emm_context->ue_id);
//   if ((*bearer_context_proc)->esm_proc.base_proc.parent) {
//      (*bearer_context_proc)->esm_proc.base_proc.parent->child = NULL;
//    }
//    void *unused = NULL;
////    nas_stop_Ts6a_auth_info(emm_context->ue_id, &(*bearer_context_proc)->timer_s6a, unused);
//    free_wrapper((void**)bearer_context_proc);
//  }
//}
//
////-----------------------------------------------------------------------------
//void _nas_delete_esm_bearer_context_procedures(struct emm_data_context_s *emm_context, nas_esm_bearer_ctx_proc_t * bc_proc)
//{
//  if (emm_context->esm_ctx.esm_procedures) {
//    nas_esm_bearer_context_procedure_t *p1 = LIST_FIRST(&emm_context->esm_ctx.esm_procedures->esm_bearer_context_procs);
//    nas_esm_bearer_context_procedure_t *p2 = NULL;
//    nas_esm_bearer_ctx_proc_t          *esm_bc_proc = NULL;
//    // 2 methods: this one, the other: use parent struct macro and LIST_REMOVE without searching matching element in the list
//    while (p1) {
//      p2 = LIST_NEXT(p1, entries);
//      if (p1->proc == bc_proc) {
//        switch (bc_proc->type) {
//          case ESM_PROC_DEFAULT_EPS_BEARER_CTXT_ACTIVATION:
//          case ESM_PROC_DEDICATED_EPS_BEARER_CTXT_ACTIVATION:
//          case ESM_PROC_EPS_BEARER_CTXT_MODIFICATION:
//          case ESM_PROC_EPS_BEARER_CTXT_DEACTIVATION:
//            /** Same method for all the ESM procedures. */
//            esm_bc_proc = (nas_esm_bearer_ctx_proc_t *)p1->proc;
//            OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Delete ESM bearer context procedure\n", emm_context->ue_id);
//            nas_delete_esm_bearer_context_procedure(emm_context, (nas_esm_bearer_ctx_proc_t**)&esm_bc_proc);
//            break;
//          case ESM_BEARER_CTX_PROC_NONE:
//            free_wrapper((void**)&bc_proc);
//            break;
//          default:;
//        }
//        OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Delete ESM bearer context procedure %p\n", emm_context->ue_id, p1->proc);
//        LIST_REMOVE(p1, entries);
//        free_wrapper((void**)&p1);
//        return;
//      }
//      p1 = p2;
//    }
//    nas_esm_procedure_gc(&emm_context->esm_ctx);
//  }
//}
//
////-----------------------------------------------------------------------------
//static void nas_delete_all_esm_bearer_context_procedures(struct emm_data_context_s *emm_context)
//{
//  if (emm_context->esm_ctx.esm_procedures) {
//    nas_esm_bearer_context_procedure_t *p1 = LIST_FIRST(&emm_context->esm_ctx.esm_procedures->esm_bearer_context_procs);
//    nas_esm_bearer_context_procedure_t *p2 = NULL;
//    while (p1) {
//      p2 = LIST_NEXT(p1, entries);
//      _nas_delete_esm_bearer_context_procedures(emm_context, (nas_esm_bearer_ctx_proc_t**)&p1->proc);
//    }
//    LIST_REMOVE(p1, entries);
//    free_wrapper((void**)&p1);
//    p1 = p2;
//  }
//  nas_esm_procedure_gc(&emm_context->esm_ctx);
//}

//-----------------------------------------------------------------------------
static void nas_delete_esm_transaction_procedure(const struct esm_context_s *esm_context, nas_esm_transaction_proc_t ** trx_proc)
{
//  if (*trx_proc) {
//   OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Transaction procedure. \n", esm_context->ue_id);
//   if ((*trx_proc)->esm_proc.base_proc.parent) {
//      (*trx_proc)->esm_proc.base_proc.parent->child = NULL;
//    }
//    void *unused = NULL;
////    nas_stop_Ts6a_auth_info(emm_context->ue_id, &(*trx_proc)->timer_s6a, unused);
//    free_wrapper((void**)trx_proc);
//  }
}

//-----------------------------------------------------------------------------
void _nas_delete_esm_transaction_procedures(const struct esm_context_s *esm_context, nas_esm_transaction_proc_t ** trx_proc)
{
//  if (esm_context->esm_procedures) {
//    nas_esm_transaction_procedures_t *p1 = LIST_FIRST(&esm_context->esm_procedures->esm_transaction_procs);
//    nas_esm_transaction_procedures_t *p2 = NULL;
//    // 2 methods: this one, the other: use parent struct macro and LIST_REMOVE without searching matching element in the list
//    nas_esm_transaction_proc_t         *esm_trx_proc = NULL;
//    while (p1) {
//      p2 = LIST_NEXT(p1, entries);
//      if (p1->proc == trx_proc) {
//        switch (trx_proc->type) {
//          case ESM_PROC_TRANSACTION_PDN_CONNECTIVITY:
//          case ESM_PROC_TRANSACTION_PDN_DISCONNECT:
//          case ESM_PROC_TRANSACTION_BEARER_RESOURCE_ALLOCATION:
//          case ESM_PROC_TRANSACTION_BEARER_RESOURCE_MODIFICATION:
//            /** Same method for all the ESM procedures. */
//            esm_trx_proc = (nas_esm_transaction_proc_t *)p1->proc;
//            OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Delete Trx procedure\n", esm_context->ue_id);
//            nas_delete_esm_transaction_procedure(esm_context, (nas_esm_transaction_proc_t**)&esm_trx_proc);
//            break;
//          case ESM_TRANSACTION_PROC_NONE:
//            free_wrapper((void**)&trx_proc);
//            break;
//          default:;
//        }
//        OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Delete Trx procedure %p\n", esm_context->ue_id, p1->proc);
//        LIST_REMOVE(p1, entries);
//        free_wrapper((void**)&p1);
//        return;
//      }
//      p1 = p2;
//    }
//    nas_esm_procedure_gc(&esm_context);
//  }
}

//-----------------------------------------------------------------------------
static void nas_delete_all_esm_transaction_procs(struct esm_context_s *esm_context)
{
//  if (esm_context->esm_procedures) {
//    nas_esm_transaction_procedures_t *p1 = LIST_FIRST(&esm_context->esm_procedures->esm_transaction_procs);
//    nas_esm_transaction_procedures_t *p2 = NULL;
//    while (p1) {
//      p2 = LIST_NEXT(p1, entries);
//      _nas_delete_esm_transaction_procedures(esm_context, (nas_esm_transaction_proc_t**)&p1->proc);
//    }
//    LIST_REMOVE(p1, entries);
//    free_wrapper((void**)&p1);
//    p1 = p2;
//  }
//  nas_esm_procedure_gc(esm_context);
}

//-----------------------------------------------------------------------------
void nas_delete_all_esm_procedures(struct esm_context_s * const esm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
//  if (esm_context->esm_procedures) {
////    nas_delete_all_esm_bearer_context_procedures(emm_context);
//    nas_delete_all_esm_transaction_procs(esm_context);
//
//    // gc
//    if (esm_context->esm_procedures) {
//      free_wrapper((void**)&esm_context.esm_procedures);
//    }
//  }
  OAILOG_FUNC_OUT (LOG_NAS_EMM);
}

////------------------------------------------------------------------------------
//nas_esm_bearer_context_procedure_t  *get_esm_bearer_context_procedure(const struct emm_data_context_s * const ctxt, esm_bearer_ctx_proc_type_t esm_bc_proc_type) /**< Only one procedure can exist at a time. */
//{
//  if (ctxt) {
//    if (ctxt->esm_ctx.esm_procedures) {
//      nas_esm_bearer_context_procedure_t *p1 = LIST_FIRST(&ctxt->esm_ctx.esm_procedures->esm_bearer_context_procs);
//      nas_esm_bearer_context_procedure_t *p2 = NULL;
//      while (p1) {
//        p2 = LIST_NEXT(p1, entries);
//        if (p1->proc->type == esm_bc_proc_type) {
//          return p1->proc;
//        }
//        p1 = p2;
//      }
//    }
//  }
//  return NULL;
//}

//------------------------------------------------------------------------------
nas_esm_transaction_proc_t *get_esm_transaction_procedure(const struct esm_context_s * const esm_context, esm_transaction_proc_type_t esm_trx_proc_type) /**< Only one procedure can exist at a time. */
{
//  if (esm_context) {
//    if (esm_context.esm_procedures) {
//      nas_esm_transaction_procedures_t *p1 = LIST_FIRST(&esm_context->esm_procedures->esm_transaction_procs);
//      nas_esm_transaction_procedures_t *p2 = NULL;
//      while (p1) {
//        p2 = LIST_NEXT(p1, entries);
//        if (p1->proc->type == esm_trx_proc_type) {
//          return p1->proc;
//        }
//        p1 = p2;
//      }
//    }
//  }
  return NULL;
}

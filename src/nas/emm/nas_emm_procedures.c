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

#include "emm_data.h"
#include "emm_proc.h"
#include "emm_cause.h"
#include "emm_sap.h"
#include "NasSecurityAlgorithms.h"
#include "nas_timer.h"

#include "nas_emm_procedures.h"
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
#include "mme_app_session_context.h"
#include "networkDef.h"
#include "mme_api.h"
#include "mme_config.h"
#include "nas_itti_messaging.h"
#include "mme_app_defs.h"
#include "emm_fsm.h"
//#include "digest.h"

static  nas_emm_common_proc_t *get_nas_common_procedure(const struct emm_data_context_s * const ctxt, emm_common_proc_type_t proc_type);
static  nas_emm_cn_proc_t *get_nas_cn_procedure(const struct emm_data_context_s * const ctxt, cn_proc_type_t proc_type);

static void nas_emm_procedure_gc(struct emm_data_context_s * const emm_context);
static void nas_delete_con_mngt_procedure(nas_emm_con_mngt_proc_t **  proc);
static void nas_delete_auth_info_procedure(struct emm_data_context_s *emm_context, nas_auth_info_proc_t ** auth_info_proc);
static void nas_delete_context_req_procedure(struct emm_data_context_s *emm_context, nas_ctx_req_proc_t ** ctx_req_proc);
static void nas_delete_child_procedures(struct emm_data_context_s * const emm_context, nas_emm_base_proc_t * const parent_proc);
static void nas_delete_cn_procedures(struct emm_data_context_s *emm_context);
static void nas_delete_common_procedures(struct emm_data_context_s *emm_context);
static nas_emm_proc_t * nas_emm_find_procedure_by_puid(struct emm_data_context_s * const emm_context, uint64_t puid);

static uint64_t  nas_puid = 1;

//------------------------------------------------------------------------------
static  nas_emm_common_proc_t *get_nas_common_procedure(const struct emm_data_context_s * const ctxt, emm_common_proc_type_t proc_type)
{
  if (ctxt) {
    if (ctxt->emm_procedures) {
      nas_emm_common_procedure_t *p1 = LIST_FIRST(&ctxt->emm_procedures->emm_common_procs);
      nas_emm_common_procedure_t *p2 = NULL;
      while (p1) {
        p2 = LIST_NEXT(p1, entries);
        if (p1->proc->type == proc_type) {
          return p1->proc;
        }
        p1 = p2;
      }
    }
  }
  return NULL;
}

//------------------------------------------------------------------------------
static  nas_emm_cn_proc_t *get_nas_cn_procedure(const struct emm_data_context_s * const ctxt, cn_proc_type_t proc_type)
{
  if (ctxt) {
    if (ctxt->emm_procedures) {
      nas_cn_procedure_t *p1 = LIST_FIRST(&ctxt->emm_procedures->cn_procs);
      nas_cn_procedure_t *p2 = NULL;
      while (p1) {
        p2 = LIST_NEXT(p1, entries);
        if (p1->proc->type == proc_type) {
          return p1->proc;
        }
        p1 = p2;
      }
    }
  }
  return NULL;
}
//------------------------------------------------------------------------------
inline bool is_nas_common_procedure_guti_realloc_running(const struct emm_data_context_s * const ctxt)
{
  if (get_nas_common_procedure_guti_realloc(ctxt)) return true;
  return false;
}

//------------------------------------------------------------------------------
inline bool is_nas_common_procedure_authentication_running(const struct emm_data_context_s * const ctxt)
{
  if (get_nas_common_procedure_authentication(ctxt)) return true;
  return false;
}

//------------------------------------------------------------------------------
inline bool is_nas_common_procedure_smc_running(const struct emm_data_context_s * const ctxt)
{
  if (get_nas_common_procedure_smc(ctxt)) return true;
  return false;
}

//------------------------------------------------------------------------------
inline bool is_nas_common_procedure_identification_running(const struct emm_data_context_s * const ctxt)
{
  if (get_nas_common_procedure_identification(ctxt)) return true;
  return false;
}

//------------------------------------------------------------------------------
nas_emm_guti_proc_t *get_nas_common_procedure_guti_realloc(const struct emm_data_context_s * const ctxt)
{
  return (nas_emm_guti_proc_t*)get_nas_common_procedure(ctxt, EMM_COMM_PROC_GUTI);
}

//------------------------------------------------------------------------------
nas_emm_auth_proc_t *get_nas_common_procedure_authentication(const struct emm_data_context_s * const ctxt)
{
  return (nas_emm_auth_proc_t*)get_nas_common_procedure(ctxt, EMM_COMM_PROC_AUTH);
}

//------------------------------------------------------------------------------
nas_auth_info_proc_t *get_nas_cn_procedure_auth_info(const struct emm_data_context_s * const ctxt)
{
  return (nas_auth_info_proc_t*)get_nas_cn_procedure(ctxt, CN_PROC_AUTH_INFO);
}

//------------------------------------------------------------------------------
nas_ctx_req_proc_t *get_nas_cn_procedure_ctx_req(const struct emm_data_context_s * const ctxt)
{
  return (nas_ctx_req_proc_t*)get_nas_cn_procedure(ctxt, CN_PROC_CTX_REQ);
}

//------------------------------------------------------------------------------
nas_emm_smc_proc_t *get_nas_common_procedure_smc(const struct emm_data_context_s * const ctxt)
{
  return (nas_emm_smc_proc_t*)get_nas_common_procedure(ctxt, EMM_COMM_PROC_SMC);
}

//------------------------------------------------------------------------------
nas_emm_ident_proc_t *get_nas_common_procedure_identification(const struct emm_data_context_s * const ctxt)
{
  return (nas_emm_ident_proc_t*)get_nas_common_procedure(ctxt, EMM_COMM_PROC_IDENT);
}

//------------------------------------------------------------------------------
inline bool is_nas_specific_procedure_attach_running(const struct emm_data_context_s * const ctxt)
{
  if ((ctxt) && (ctxt->emm_procedures)  && (ctxt->emm_procedures->emm_specific_proc) &&
      ((EMM_SPEC_PROC_TYPE_ATTACH == ctxt->emm_procedures->emm_specific_proc->type)))
    return true;
  return false;
}

//-----------------------------------------------------------------------------
inline bool is_nas_specific_procedure_detach_running(const struct emm_data_context_s * const ctxt)
{
  if ((ctxt) && (ctxt->emm_procedures)  && (ctxt->emm_procedures->emm_specific_proc) &&
      ((EMM_SPEC_PROC_TYPE_DETACH == ctxt->emm_procedures->emm_specific_proc->type)))
    return true;
  return false;
}

//-----------------------------------------------------------------------------
inline bool is_nas_specific_procedure_tau_running(const struct emm_data_context_s * const ctxt)
{
  if ((ctxt) && (ctxt->emm_procedures)  && (ctxt->emm_procedures->emm_specific_proc) &&
      ((EMM_SPEC_PROC_TYPE_TAU == ctxt->emm_procedures->emm_specific_proc->type)))
    return true;
  return false;
}

//------------------------------------------------------------------------------
nas_emm_specific_proc_t *get_nas_specific_procedure(const struct emm_data_context_s * const ctxt)
{
  if ((ctxt) && (ctxt->emm_procedures)  && (ctxt->emm_procedures->emm_specific_proc))
    return ctxt->emm_procedures->emm_specific_proc;
  return NULL;
}

//------------------------------------------------------------------------------
nas_emm_attach_proc_t *get_nas_specific_procedure_attach(const struct emm_data_context_s * const ctxt)
{
  if ((ctxt) && (ctxt->emm_procedures)  && (ctxt->emm_procedures->emm_specific_proc) &&
      ((EMM_SPEC_PROC_TYPE_ATTACH == ctxt->emm_procedures->emm_specific_proc->type)))
    return (nas_emm_attach_proc_t *)ctxt->emm_procedures->emm_specific_proc;
  return NULL;
}

//-----------------------------------------------------------------------------
nas_emm_detach_proc_t *get_nas_specific_procedure_detach(const struct emm_data_context_s * const ctxt)
{
  if ((ctxt) && (ctxt->emm_procedures)  && (ctxt->emm_procedures->emm_specific_proc) &&
      ((EMM_SPEC_PROC_TYPE_DETACH == ctxt->emm_procedures->emm_specific_proc->type)))
    return (nas_emm_detach_proc_t *)ctxt->emm_procedures->emm_specific_proc;
  return NULL;
}

//-----------------------------------------------------------------------------
nas_emm_tau_proc_t *get_nas_specific_procedure_tau(const struct emm_data_context_s * const ctxt)
{
  if ((ctxt) && (ctxt->emm_procedures)  && (ctxt->emm_procedures->emm_specific_proc) &&
      ((EMM_SPEC_PROC_TYPE_TAU == ctxt->emm_procedures->emm_specific_proc->type)))
    return (nas_emm_tau_proc_t *)ctxt->emm_procedures->emm_specific_proc;
  return NULL;
}

//------------------------------------------------------------------------------
nas_sr_proc_t *get_nas_con_mngt_procedure_service_request(const struct emm_data_context_s * const ctxt)
{
  if ((ctxt) && (ctxt->emm_procedures)  && (ctxt->emm_procedures->emm_con_mngt_proc) &&
      ((EMM_CON_MNGT_PROC_SERVICE_REQUEST == ctxt->emm_procedures->emm_con_mngt_proc->type)))
    return (nas_sr_proc_t *)ctxt->emm_procedures->emm_con_mngt_proc;
  return NULL;
}


//-----------------------------------------------------------------------------
inline bool is_nas_attach_accept_sent(const nas_emm_attach_proc_t * const attach_proc)
{
  if (attach_proc->attach_accept_sent) {
    return true;
  } else {
    return false;
  }
}
//-----------------------------------------------------------------------------
inline bool is_nas_attach_reject_sent(const nas_emm_attach_proc_t * const attach_proc)
{
  return attach_proc->attach_reject_sent;
}
//-----------------------------------------------------------------------------
inline bool is_nas_attach_complete_received(const nas_emm_attach_proc_t * const attach_proc)
{
  return attach_proc->attach_complete_received;
}


//-----------------------------------------------------------------------------
inline bool is_nas_tau_accept_sent(const nas_emm_tau_proc_t * const tau_proc)
{
  if (tau_proc->tau_accept_sent) {
    return true;
  } else {
    return false;
  }
}
//-----------------------------------------------------------------------------
inline bool is_nas_tau_reject_sent(const nas_emm_tau_proc_t * const tau_proc)
{
  return tau_proc->tau_reject_sent;
}
//-----------------------------------------------------------------------------
inline bool is_nas_tau_complete_received(const nas_emm_tau_proc_t * const tau_proc)
{
  return tau_proc->tau_complete_received;
}


//------------------------------------------------------------------------------
int nas_unlink_emm_procedures(nas_emm_base_proc_t * const parent_proc, nas_emm_base_proc_t * const child_proc)
{
  if ((parent_proc) && (child_proc)) {
    if ((parent_proc->child == child_proc) && (child_proc->parent == parent_proc)) {
      child_proc->parent = NULL;
      parent_proc->child = NULL;
      return RETURNok;
    }
  }
  return RETURNerror;
}

//-----------------------------------------------------------------------------
static void nas_emm_procedure_gc(struct emm_data_context_s * const emm_context)
{
  if ( LIST_EMPTY(&emm_context->emm_procedures->emm_common_procs) &&
       LIST_EMPTY(&emm_context->emm_procedures->cn_procs) &&
       (!emm_context->emm_procedures->emm_con_mngt_proc) &&
       (!emm_context->emm_procedures->emm_specific_proc) ) {
    free_wrapper((void**)&emm_context->emm_procedures);
  }
}
//-----------------------------------------------------------------------------
static void nas_delete_child_procedures(struct emm_data_context_s * const emm_context, nas_emm_base_proc_t * const parent_proc)
{
  // abort child procedures
  if (emm_context->emm_procedures) {
    nas_emm_common_procedure_t *p1 = LIST_FIRST(&emm_context->emm_procedures->emm_common_procs);
    nas_emm_common_procedure_t *p2 = NULL;
    while (p1) {
      p2 = LIST_NEXT(p1, entries);
      if (((nas_emm_base_proc_t *)p1->proc)->parent == parent_proc) {
        nas_delete_common_procedure(emm_context, &p1->proc);
        // Done by nas_delete_common_procedure: LIST_REMOVE(p1, entries);
        //Done by nas_delete_common_procedure: free_wrapper((void**)&p1);
      }
      p1 = p2;
    }

    if (emm_context->emm_procedures->emm_con_mngt_proc) {
      if (((nas_emm_base_proc_t *)(emm_context->emm_procedures->emm_con_mngt_proc))->parent == parent_proc) {
        nas_delete_con_mngt_procedure(&emm_context->emm_procedures->emm_con_mngt_proc);
      }
    }
  }
}

//-----------------------------------------------------------------------------
static void nas_delete_con_mngt_procedure(nas_emm_con_mngt_proc_t **  proc)
{
  if (*proc) {
    AssertFatal(0, "TODO");
    free_wrapper((void**)proc);
  }
}
//-----------------------------------------------------------------------------
void nas_delete_common_procedure(struct emm_data_context_s *emm_context, nas_emm_common_proc_t **  proc)
{
  if (*proc) {
    // free proc content
    switch ((*proc)->type) {
      case EMM_COMM_PROC_GUTI:
        OAILOG_TRACE (LOG_NAS_EMM, "Delete GUTI procedure %"PRIx64"\n", (*proc)->emm_proc.base_proc.nas_puid);
        break;
      case EMM_COMM_PROC_AUTH: {
          nas_emm_auth_proc_t *auth_info_proc = (nas_emm_auth_proc_t *)(*proc);
          OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Delete AUTH procedure\n", auth_info_proc->ue_id);
          if (auth_info_proc->unchecked_imsi) {
            free_wrapper((void**)&auth_info_proc->unchecked_imsi);
          }
        }
        break;
      case EMM_COMM_PROC_SMC: {
        OAILOG_TRACE (LOG_NAS_EMM, "Delete SMC procedure %"PRIx64"\n", (*proc)->emm_proc.base_proc.nas_puid);
          //nas_emm_smc_proc_t *smc_proc = (nas_emm_smc_proc_t *)(*proc);
        }
        break;
      case EMM_COMM_PROC_IDENT: {
        OAILOG_TRACE (LOG_NAS_EMM, "Delete IDENT procedure %"PRIx64"\n", (*proc)->emm_proc.base_proc.nas_puid);
          //nas_emm_ident_proc_t *ident_proc = (nas_emm_ident_proc_t *)(*proc);
        }
        break;
      case EMM_COMM_PROC_INFO:
        OAILOG_TRACE (LOG_NAS_EMM, "Delete INFO procedure %"PRIx64"\n", (*proc)->emm_proc.base_proc.nas_puid);
        break;
      default: ;
    }

    // remove proc from list
    if (emm_context->emm_procedures) {
      nas_emm_common_procedure_t *p1 = LIST_FIRST(&emm_context->emm_procedures->emm_common_procs);
      nas_emm_common_procedure_t *p2 = NULL;
      // 2 methods: this one, the other: use parent struct macro and LIST_REMOVE without searching matching element in the list
      while (p1) {
        p2 = LIST_NEXT(p1, entries);
        if (p1->proc == (nas_emm_common_proc_t*)(*proc)) {
          LIST_REMOVE(p1, entries);
          free_wrapper((void**)&p1->proc);
          free_wrapper((void**)&p1);
          *proc = NULL;
          return;
        }
        p1 = p2;
      }
      nas_emm_procedure_gc(emm_context);
    }
    // if not found in list, free it anyway
    if (*proc) {
      free_wrapper((void**)proc);
    }
  }
}

//-----------------------------------------------------------------------------
static void nas_delete_common_procedures(struct emm_data_context_s *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  // remove proc from list
  if (emm_context->emm_procedures) {
    nas_emm_common_procedure_t *p1 = LIST_FIRST(&emm_context->emm_procedures->emm_common_procs);
    nas_emm_common_procedure_t *p2 = NULL;
    while (p1) {
      p2 = LIST_NEXT(p1, entries);
      LIST_REMOVE(p1, entries);

      switch (p1->proc->type) {
        case EMM_COMM_PROC_GUTI:
          OAILOG_TRACE (LOG_NAS_EMM, "Delete GUTI procedure %"PRIx64"\n", p1->proc->emm_proc.base_proc.nas_puid);
          break;
        case EMM_COMM_PROC_AUTH: {
            nas_emm_auth_proc_t *auth_info_proc = (nas_emm_auth_proc_t *)p1->proc;
            OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Delete AUTH procedure & timer\n", auth_info_proc->ue_id);
            if (auth_info_proc->unchecked_imsi) {
              free_wrapper((void**)&auth_info_proc->unchecked_imsi);
            }
            void * unused = NULL;
            nas_stop_T3460(auth_info_proc->ue_id, &auth_info_proc->T3460, unused);

          }
          break;
        case EMM_COMM_PROC_SMC: {
          OAILOG_DEBUG (LOG_NAS_EMM, "Delete SMC procedure %"PRIx64"\n", p1->proc->emm_proc.base_proc.nas_puid);
            //nas_emm_smc_proc_t *smc_proc = (nas_emm_smc_proc_t *)(*proc);
          nas_emm_smc_proc_t *smc_proc = (nas_emm_smc_proc_t *)p1->proc;
          void * unused = NULL;
          nas_stop_T3460(smc_proc->ue_id, &smc_proc->T3460, unused);

          }
          break;
        case EMM_COMM_PROC_IDENT: {
          OAILOG_TRACE (LOG_NAS_EMM, "Delete IDENT procedure %"PRIx64"\n", p1->proc->emm_proc.base_proc.nas_puid);
          //nas_emm_ident_proc_t *ident_proc = (nas_emm_ident_proc_t *)(*proc);
          nas_emm_ident_proc_t *ident_proc = (nas_emm_ident_proc_t *)p1->proc;
          void * unused = NULL;
          nas_stop_T3470(ident_proc->ue_id, &ident_proc->T3470, unused);

          }
          break;
        case EMM_COMM_PROC_INFO:
          OAILOG_TRACE (LOG_NAS_EMM, "Delete INFO procedure %"PRIx64"\n", p1->proc->emm_proc.base_proc.nas_puid);
          break;
        default: ;
      }

      free_wrapper((void**)&p1->proc);
      free_wrapper((void**)&p1);

      p1 = p2;
    }
    nas_emm_procedure_gc(emm_context);
  }
  OAILOG_FUNC_OUT (LOG_NAS_EMM);
}
#include "s1ap_mme.h"

//-----------------------------------------------------------------------------
void nas_delete_attach_procedure(struct emm_data_context_s *emm_context)
{
  nas_emm_attach_proc_t     *proc = get_nas_specific_procedure_attach(emm_context);
  if (proc) {
    // free content
    mme_ue_s1ap_id_t      ue_id = emm_context->ue_id;
    OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Delete ATTACH procedure\n", ue_id);
    void *unused = NULL;
    nas_stop_T3450(ue_id, &proc->T3450, unused);
    ue_description_t * ue_ref = s1ap_is_ue_mme_id_in_list(emm_context->ue_id);

    if (proc->ies) {
      free_emm_attach_request_ies(&proc->ies);
    }
    if (proc->esm_msg_out) {
      bdestroy_wrapper(&proc->esm_msg_out);
    }

    if(ue_ref){
      OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC (NASx)  -  * * * * * (2) ueREF %p has mmeId " MME_UE_S1AP_ID_FMT ", enbId " ENB_UE_S1AP_ID_FMT " state %d and eNB_ref %p. \n",
          ue_ref, ue_ref->mme_ue_s1ap_id, ue_ref->enb_ue_s1ap_id, ue_ref->s1_ue_state, ue_ref->enb);
    }
    unused = NULL;
    nas_stop_T_retry_specific_procedure(ue_id, &proc->emm_spec_proc.retry_timer, unused);

    OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " stopped the retry timer for attach procedure\n", ue_id);
    if(ue_ref){
      OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC (NASx)  -  * * * * * (2.5) ueREF %p has mmeId " MME_UE_S1AP_ID_FMT ", enbId " ENB_UE_S1AP_ID_FMT " state %d and eNB_ref %p \n",
                ue_ref, ue_ref->mme_ue_s1ap_id, ue_ref->enb_ue_s1ap_id, ue_ref->s1_ue_state, ue_ref->enb);
    }
    nas_delete_child_procedures(emm_context, (nas_emm_base_proc_t *)proc);
    free_wrapper((void**)&emm_context->emm_procedures->emm_specific_proc);
    nas_emm_procedure_gc(emm_context);
  }
}

//-----------------------------------------------------------------------------
void nas_delete_tau_procedure(struct emm_data_context_s *emm_context)
{
  nas_emm_tau_proc_t     *proc = get_nas_specific_procedure_tau(emm_context);
  if (proc) {
    // free content
    mme_ue_s1ap_id_t      ue_id = emm_context->ue_id;
    OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Delete TAU procedure\n", ue_id);
    void *unused = NULL;
    nas_stop_T3450(ue_id, &proc->T3450, unused);
    if (proc->ies) {
      free_emm_tau_request_ies(&proc->ies);
    }
    if (proc->esm_msg_out) {
      bdestroy_wrapper(&proc->esm_msg_out);
    }
    nas_stop_T_retry_specific_procedure(emm_context->ue_id, &proc->emm_spec_proc.retry_timer, unused);
    OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " stopped the retry timer for TAU procedure\n", ue_id);

    nas_delete_child_procedures(emm_context, (nas_emm_base_proc_t *)proc);

    free_wrapper((void**)&emm_context->emm_procedures->emm_specific_proc);
    nas_emm_procedure_gc(emm_context);
  }
}

//-----------------------------------------------------------------------------
void nas_delete_detach_procedure(struct emm_data_context_s *emm_context)
{
  nas_emm_detach_proc_t     *proc = get_nas_specific_procedure_detach(emm_context);
  if (proc) {
    OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Delete DETACH procedure\n", emm_context->ue_id);
    // free content
    if (proc->ies) {
      free_emm_detach_request_ies(&proc->ies);
    }

    nas_delete_child_procedures(emm_context, (nas_emm_base_proc_t *)proc);

    free_wrapper((void**)&emm_context->emm_procedures->emm_specific_proc);
    nas_emm_procedure_gc(emm_context);
  }
}

//-----------------------------------------------------------------------------
static void nas_delete_auth_info_procedure(struct emm_data_context_s *emm_context, nas_auth_info_proc_t ** auth_info_proc)
{
  if (*auth_info_proc) {
   OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Delete AUTH INFO procedure\n", emm_context->ue_id);
   if ((*auth_info_proc)->cn_proc.base_proc.parent) {
      (*auth_info_proc)->cn_proc.base_proc.parent->child = NULL;
    }
    void *unused = NULL;
    nas_stop_Ts6a_auth_info(emm_context->ue_id, &(*auth_info_proc)->timer_s6a, unused);
    free_wrapper((void**)auth_info_proc);
  }
}

//-----------------------------------------------------------------------------
static void nas_delete_context_req_procedure(struct emm_data_context_s *emm_context, nas_ctx_req_proc_t ** ctx_req_proc)
{
  if (*ctx_req_proc) {
   OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Delete Context Request procedure\n", emm_context->ue_id);
   if ((*ctx_req_proc)->cn_proc.base_proc.parent) {
     (*ctx_req_proc)->cn_proc.base_proc.parent->child = NULL;
   }
   if((*ctx_req_proc)->pdn_connections){
	   free_mme_ue_eps_pdn_connections(&(*ctx_req_proc)->pdn_connections);
   }
   if((*ctx_req_proc)->nas_s10_context.mm_eps_ctx){
	   free_mm_context_eps(&(*ctx_req_proc)->nas_s10_context.mm_eps_ctx);
   }
   ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, emm_context->ue_id);

   void *unused = NULL;
   OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Delete Context Request procedure timer with timer id %u \n", emm_context->ue_id, (*ctx_req_proc)->timer_s10.id);

   nas_stop_Ts10_ctx_res(emm_context->ue_id, &(*ctx_req_proc)->timer_s10, unused);

   free_wrapper((void**)ctx_req_proc);
  }
}

//-----------------------------------------------------------------------------
void nas_delete_cn_procedure(struct emm_data_context_s *emm_context, nas_emm_cn_proc_t * cn_proc)
{
  if (emm_context->emm_procedures) {
    nas_cn_procedure_t *p1 = LIST_FIRST(&emm_context->emm_procedures->cn_procs);
    nas_cn_procedure_t *p2 = NULL;
    // 2 methods: this one, the other: use parent struct macro and LIST_REMOVE without searching matching element in the list
    while (p1) {
      p2 = LIST_NEXT(p1, entries);
      if (p1->proc == cn_proc) {
        switch (cn_proc->type) {
          case CN_PROC_AUTH_INFO:
            nas_delete_auth_info_procedure(emm_context, (nas_auth_info_proc_t**)&cn_proc);
            break;
          case CN_PROC_CTX_REQ:
            nas_delete_context_req_procedure(emm_context, (nas_ctx_req_proc_t**)&cn_proc);
            break;
          case CN_PROC_NONE:
            free_wrapper((void**)&cn_proc);
            break;
          default:;
        }
        OAILOG_TRACE (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Delete CN procedure %p\n", emm_context->ue_id, p1->proc);
        LIST_REMOVE(p1, entries);
        free_wrapper((void**)&p1);
        return;
      }
      p1 = p2;
    }
    nas_emm_procedure_gc(emm_context);
  }
}

//-----------------------------------------------------------------------------
static void nas_delete_cn_procedures(struct emm_data_context_s *emm_context)
{
  if (emm_context->emm_procedures) {
    nas_cn_procedure_t *p1 = LIST_FIRST(&emm_context->emm_procedures->cn_procs);
    nas_cn_procedure_t *p2 = NULL;
    while (p1) {
      p2 = LIST_NEXT(p1, entries);
      switch (p1->proc->type) {
        case CN_PROC_AUTH_INFO:
          nas_delete_auth_info_procedure(emm_context, (nas_auth_info_proc_t**)&p1->proc);
          break;
        case CN_PROC_CTX_REQ:
          nas_delete_context_req_procedure(emm_context, (nas_ctx_req_proc_t**)&p1->proc);
          break;
        default:
          free_wrapper((void**)&p1->proc);
      }
      LIST_REMOVE(p1, entries);
      free_wrapper((void**)&p1);
      p1 = p2;
    }
    nas_emm_procedure_gc(emm_context);
  }
}

//-----------------------------------------------------------------------------
void nas_delete_all_emm_procedures(struct emm_data_context_s * const emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  if (emm_context->emm_procedures) {
    nas_delete_cn_procedures(emm_context);
    nas_delete_common_procedures(emm_context);
    //TODO nas_delete_con_mngt_procedure(emm_context);
    nas_delete_attach_procedure(emm_context);
    nas_delete_detach_procedure(emm_context);
    nas_delete_tau_procedure(emm_context);
    // gc
    if (emm_context->emm_procedures) {
      free_wrapper((void**)&emm_context->emm_procedures);
    }
  }
  OAILOG_FUNC_OUT (LOG_NAS_EMM);
}

//-----------------------------------------------------------------------------
static emm_procedures_t *_nas_new_emm_procedures(struct emm_data_context_s * const emm_context)
{
  emm_procedures_t *emm_procedures = calloc(1, sizeof(*emm_context->emm_procedures));
  LIST_INIT(&emm_procedures->emm_common_procs);
  return emm_procedures;
}

//-----------------------------------------------------------------------------
nas_emm_attach_proc_t* nas_new_attach_procedure(struct emm_data_context_s * const emm_context)
{
  if (!(emm_context->emm_procedures)) {
    emm_context->emm_procedures = _nas_new_emm_procedures(emm_context);
  } else if (emm_context->emm_procedures->emm_specific_proc) {
    OAILOG_ERROR (LOG_NAS_EMM,
        "UE " MME_UE_S1AP_ID_FMT " Attach procedure creation requested but another specific procedure found\n", emm_context->ue_id);
    return NULL;
  }
  emm_context->emm_procedures->emm_specific_proc = calloc(1, sizeof(nas_emm_attach_proc_t));
  emm_context->emm_procedures->emm_specific_proc->emm_proc.base_proc.nas_puid = __sync_fetch_and_add (&nas_puid, 1);
  emm_context->emm_procedures->emm_specific_proc->emm_proc.type  = NAS_EMM_PROC_TYPE_SPECIFIC;
  /** Timer. */
  emm_context->emm_procedures->emm_specific_proc->retry_timer.sec = TIMER_SPECIFIC_RETRY_DEFAULT_VALUE;
  emm_context->emm_procedures->emm_specific_proc->retry_timer.usec= TIMER_SPECIFIC_RETRY_DEFAULT_VALUE_USEC;
  emm_context->emm_procedures->emm_specific_proc->retry_timer.id  = NAS_TIMER_INACTIVE_ID;
  emm_context->emm_procedures->emm_specific_proc->type  = EMM_SPEC_PROC_TYPE_ATTACH;
  /** Set the success notifications, entered when the UE goes from EMM_DEREGISTERED to EMM_REGISTERED (former MME_APP callbacks). */
//  emm_context->emm_procedures->emm_specific_proc->emm_proc.base_proc.success_notif = _emm_registration_complete;

  nas_emm_attach_proc_t * proc = (nas_emm_attach_proc_t*)emm_context->emm_procedures->emm_specific_proc;

  proc->ue_id           = emm_context->ue_id;
  proc->T3450.sec       = mme_config.nas_config.t3450_sec;
  proc->T3450.id        = NAS_TIMER_INACTIVE_ID;

  OAILOG_TRACE (LOG_NAS_EMM, "New EMM_SPEC_PROC_TYPE_ATTACH\n");
  return proc;
}

//-----------------------------------------------------------------------------
nas_emm_tau_proc_t *nas_new_tau_procedure(struct emm_data_context_s * const emm_context)
{
  if (!(emm_context->emm_procedures)) {
    emm_context->emm_procedures = _nas_new_emm_procedures(emm_context);
  } else if (emm_context->emm_procedures->emm_specific_proc) {
    OAILOG_ERROR (LOG_NAS_EMM,
        "UE " MME_UE_S1AP_ID_FMT " TAU procedure creation requested but another specific procedure found\n", emm_context->ue_id);
    return NULL;
  }
  emm_context->emm_procedures->emm_specific_proc = calloc(1, sizeof(nas_emm_tau_proc_t));
  emm_context->emm_procedures->emm_specific_proc->emm_proc.base_proc.nas_puid = __sync_fetch_and_add (&nas_puid, 1);
  emm_context->emm_procedures->emm_specific_proc->emm_proc.type  = NAS_EMM_PROC_TYPE_SPECIFIC;
  /** Timer. */
  emm_context->emm_procedures->emm_specific_proc->retry_timer.sec  = TIMER_SPECIFIC_RETRY_DEFAULT_VALUE;
  emm_context->emm_procedures->emm_specific_proc->retry_timer.usec = TIMER_SPECIFIC_RETRY_DEFAULT_VALUE_USEC;
  emm_context->emm_procedures->emm_specific_proc->retry_timer.id   = NAS_TIMER_INACTIVE_ID;
  emm_context->emm_procedures->emm_specific_proc->type  = EMM_SPEC_PROC_TYPE_TAU;

  nas_emm_tau_proc_t * proc = (nas_emm_tau_proc_t*)emm_context->emm_procedures->emm_specific_proc;

  proc->ue_id           = emm_context->ue_id;
  proc->T3450.sec       = mme_config.nas_config.t3450_sec;
  proc->T3450.id        = NAS_TIMER_INACTIVE_ID;

  return proc;
}

//-----------------------------------------------------------------------------
nas_emm_detach_proc_t *nas_new_detach_procedure(struct emm_data_context_s * const emm_context)
{
  if (!(emm_context->emm_procedures)) {
    emm_context->emm_procedures = _nas_new_emm_procedures(emm_context);
  } else if (emm_context->emm_procedures->emm_specific_proc) {
    OAILOG_ERROR (LOG_NAS_EMM,
        "UE " MME_UE_S1AP_ID_FMT " Detach procedure creation requested but another specific procedure found\n", emm_context->ue_id);
    return NULL;
  }
  emm_context->emm_procedures->emm_specific_proc = calloc(1, sizeof(nas_emm_detach_proc_t));
  emm_context->emm_procedures->emm_specific_proc->emm_proc.base_proc.nas_puid = __sync_fetch_and_add (&nas_puid, 1);
  emm_context->emm_procedures->emm_specific_proc->emm_proc.type  = NAS_EMM_PROC_TYPE_SPECIFIC;
  emm_context->emm_procedures->emm_specific_proc->type  = EMM_SPEC_PROC_TYPE_DETACH;

  nas_emm_detach_proc_t * proc = (nas_emm_detach_proc_t*)emm_context->emm_procedures->emm_specific_proc;

  proc->ue_id           = emm_context->ue_id;

  return proc;
}

//-----------------------------------------------------------------------------
nas_sr_proc_t* nas_new_service_request_procedure(struct emm_data_context_s * const emm_context)
{
  if (!(emm_context->emm_procedures)) {
    emm_context->emm_procedures = _nas_new_emm_procedures(emm_context);
  } else if (emm_context->emm_procedures->emm_con_mngt_proc) {
    OAILOG_ERROR (LOG_NAS_EMM,
        "UE " MME_UE_S1AP_ID_FMT " SR procedure creation requested but another connection management procedure found\n", emm_context->ue_id);
    return NULL;
  }
  emm_context->emm_procedures->emm_con_mngt_proc = calloc(1, sizeof(nas_sr_proc_t));
  emm_context->emm_procedures->emm_con_mngt_proc->emm_proc.base_proc.nas_puid = __sync_fetch_and_add (&nas_puid, 1);
  emm_context->emm_procedures->emm_con_mngt_proc->emm_proc.type  = NAS_EMM_PROC_TYPE_CONN_MNGT;
  emm_context->emm_procedures->emm_con_mngt_proc->type  = EMM_CON_MNGT_PROC_SERVICE_REQUEST;

  nas_sr_proc_t * proc = (nas_sr_proc_t*)emm_context->emm_procedures->emm_con_mngt_proc;

  return proc;
}

//-----------------------------------------------------------------------------
nas_emm_ident_proc_t *nas_new_identification_procedure(struct emm_data_context_s * const emm_context)
{
  if (!(emm_context->emm_procedures)) {
    emm_context->emm_procedures = _nas_new_emm_procedures(emm_context);
  }

  nas_emm_ident_proc_t * ident_proc =  calloc(1, sizeof(nas_emm_ident_proc_t));

  ident_proc->emm_com_proc.emm_proc.base_proc.nas_puid = __sync_fetch_and_add (&nas_puid, 1);
  ident_proc->emm_com_proc.emm_proc.type      = NAS_EMM_PROC_TYPE_COMMON;
  ident_proc->emm_com_proc.type           = EMM_COMM_PROC_IDENT;

  ident_proc->T3470.sec       = mme_config.nas_config.t3470_sec;
  ident_proc->T3470.id        = NAS_TIMER_INACTIVE_ID;

  nas_emm_common_procedure_t * wrapper = calloc(1, sizeof(*wrapper));
  if (wrapper) {
    wrapper->proc = &ident_proc->emm_com_proc;
    LIST_INSERT_HEAD(&emm_context->emm_procedures->emm_common_procs, wrapper, entries);
    OAILOG_TRACE (LOG_NAS_EMM, "New EMM_COMM_PROC_IDENT\n");
    return ident_proc;
  } else {
    free_wrapper((void**)&ident_proc);
  }
  return ident_proc;
}

//-----------------------------------------------------------------------------
nas_emm_auth_proc_t *nas_new_authentication_procedure(struct emm_data_context_s * const emm_context)
{
  if (!(emm_context->emm_procedures)) {
    emm_context->emm_procedures = _nas_new_emm_procedures(emm_context);
  }

  nas_emm_auth_proc_t * auth_proc =  calloc(1, sizeof(nas_emm_auth_proc_t));

  auth_proc->emm_com_proc.emm_proc.base_proc.nas_puid = __sync_fetch_and_add (&nas_puid, 1);
  auth_proc->emm_com_proc.emm_proc.type      = NAS_EMM_PROC_TYPE_COMMON;
  auth_proc->emm_com_proc.type           = EMM_COMM_PROC_AUTH;

  auth_proc->T3460.sec       = mme_config.nas_config.t3460_sec;
  auth_proc->T3460.id        = NAS_TIMER_INACTIVE_ID;

  nas_emm_common_procedure_t * wrapper = calloc(1, sizeof(*wrapper));
  if (wrapper) {
    wrapper->proc = &auth_proc->emm_com_proc;
    LIST_INSERT_HEAD(&emm_context->emm_procedures->emm_common_procs, wrapper, entries);
    OAILOG_TRACE (LOG_NAS_EMM, "New EMM_COMM_PROC_AUTH\n");
    return auth_proc;
  } else {
    free_wrapper((void**)&auth_proc);
  }
  return NULL;
}

//-----------------------------------------------------------------------------
nas_emm_smc_proc_t *nas_new_smc_procedure(struct emm_data_context_s * const emm_context)
{
  if (!(emm_context->emm_procedures)) {
    emm_context->emm_procedures = _nas_new_emm_procedures(emm_context);
  }

  nas_emm_smc_proc_t * smc_proc =  calloc(1, sizeof(nas_emm_smc_proc_t));

  smc_proc->emm_com_proc.emm_proc.base_proc.nas_puid = __sync_fetch_and_add (&nas_puid, 1);
  smc_proc->emm_com_proc.emm_proc.type      = NAS_EMM_PROC_TYPE_COMMON;
  smc_proc->emm_com_proc.type               = EMM_COMM_PROC_SMC;

  smc_proc->T3460.sec       = mme_config.nas_config.t3460_sec;
  smc_proc->T3460.id        = NAS_TIMER_INACTIVE_ID;

  nas_emm_common_procedure_t * wrapper = calloc(1, sizeof(*wrapper));
  if (wrapper) {
    wrapper->proc = &smc_proc->emm_com_proc;
    LIST_INSERT_HEAD(&emm_context->emm_procedures->emm_common_procs, wrapper, entries);
    OAILOG_TRACE (LOG_NAS_EMM, "New EMM_COMM_PROC_SMC\n");
    return smc_proc;
  } else {
    free_wrapper((void**)&smc_proc);
  }
  return NULL;
}

//-----------------------------------------------------------------------------
nas_auth_info_proc_t *nas_new_cn_auth_info_procedure(struct emm_data_context_s * const emm_context)
{
  if (!(emm_context->emm_procedures)) {
    emm_context->emm_procedures = _nas_new_emm_procedures(emm_context);
  }

  nas_auth_info_proc_t * auth_info_proc =  calloc(1, sizeof(nas_auth_info_proc_t));
  auth_info_proc->cn_proc.base_proc.nas_puid = __sync_fetch_and_add (&nas_puid, 1);
  auth_info_proc->cn_proc.type           = CN_PROC_AUTH_INFO;
  auth_info_proc->timer_s6a.sec          = TIMER_S6A_AUTH_INFO_RSP_DEFAULT_VALUE;
  auth_info_proc->timer_s6a.id           = NAS_TIMER_INACTIVE_ID;

  nas_cn_procedure_t * wrapper = calloc(1, sizeof(*wrapper));
  if (wrapper) {
    wrapper->proc = &auth_info_proc->cn_proc;
    LIST_INSERT_HEAD(&emm_context->emm_procedures->cn_procs, wrapper, entries);
    OAILOG_TRACE (LOG_NAS_EMM, "New CN_PROC_AUTH_INFO\n");
    return auth_info_proc;
  } else {
    free_wrapper((void**)&auth_info_proc);
  }
  return NULL;
}

//-----------------------------------------------------------------------------
nas_ctx_req_proc_t *nas_new_cn_ctx_req_procedure(struct emm_data_context_s * const emm_context)
{
  if (!(emm_context->emm_procedures)) {
    emm_context->emm_procedures = _nas_new_emm_procedures(emm_context);
  }

  nas_ctx_req_proc_t * ctx_req_proc =  calloc(1, sizeof(nas_ctx_req_proc_t));
  ctx_req_proc->cn_proc.base_proc.nas_puid = __sync_fetch_and_add (&nas_puid, 1);
  ctx_req_proc->cn_proc.type           = CN_PROC_CTX_REQ;
  // todo: timer necessary for context request?
  ctx_req_proc->timer_s10.sec          = TIMER_SPECIFIC_RETRY_DEFAULT_VALUE;
  ctx_req_proc->timer_s10.id           = NAS_TIMER_INACTIVE_ID;

  nas_cn_procedure_t * wrapper = calloc(1, sizeof(*wrapper));
  if (wrapper) {
    wrapper->proc = &ctx_req_proc->cn_proc;
    LIST_INSERT_HEAD(&emm_context->emm_procedures->cn_procs, wrapper, entries);
    OAILOG_TRACE (LOG_NAS_EMM, "New CN_PROC_CTX_REQ\n");
    return ctx_req_proc;
  } else {
    free_wrapper((void**)&ctx_req_proc);
  }
  return NULL;
}

//-----------------------------------------------------------------------------
void nas_digest_msg(const unsigned char * const msg, const size_t msg_len, char * const digest, /*INOUT*/ size_t * const digest_length)
{
  unsigned int   result_len = 0;
  unsigned char *result = NULL;
//  if (RETURNok == digest_buffer(EVP_md4, msg, msg_len, &result, &result_len)) {
//    //OAILOG_STREAM_HEX (OAILOG_LEVEL_TRACE, LOG_NAS_EMM, "NAS Msg :", msg, msg_len);
//    int min_length = min(result_len, (*digest_length));
//    memcpy(digest, result, min_length);
//    *digest_length = min_length;
//    //OAILOG_STREAM_HEX (OAILOG_LEVEL_TRACE, LOG_NAS_EMM, "Digest:", digest, (*digest_length));
//    OPENSSL_free(result);
//  }
}


//-----------------------------------------------------------------------------
static nas_emm_proc_t * nas_emm_find_procedure_by_puid(struct emm_data_context_s * const emm_context, uint64_t puid)
{
  if ((emm_context) && (emm_context->emm_procedures)) {
    // start with common procedures
    nas_emm_common_procedure_t *p1 = LIST_FIRST(&emm_context->emm_procedures->emm_common_procs);
    while (p1) {
      if (p1->proc->emm_proc.base_proc.nas_puid == puid) {
        OAILOG_TRACE (LOG_NAS_EMM, "Found emm_common_proc UID 0x%"PRIx64"\n", puid);
        return &p1->proc->emm_proc;
      }
      p1 = LIST_NEXT(p1, entries);
    }

    if (emm_context->emm_procedures->emm_specific_proc) {
      if (emm_context->emm_procedures->emm_specific_proc->emm_proc.base_proc.nas_puid == puid) {
        OAILOG_TRACE (LOG_NAS_EMM, "Found emm_specific_proc UID 0x%"PRIx64"\n", puid);
        return &emm_context->emm_procedures->emm_specific_proc->emm_proc;
      }
    }

    if (emm_context->emm_procedures->emm_con_mngt_proc) {
      if (emm_context->emm_procedures->emm_con_mngt_proc->emm_proc.base_proc.nas_puid == puid) {
        OAILOG_TRACE (LOG_NAS_EMM, "Found emm_con_mngt_proc UID 0x%"PRIx64"\n", puid);
        return &emm_context->emm_procedures->emm_con_mngt_proc->emm_proc;
      }
    }
  }
  OAILOG_TRACE (LOG_NAS_EMM, "Did not find proc UID 0x%"PRIx64"\n", puid);
  return NULL;
}

//-----------------------------------------------------------------------------
void nas_emm_procedure_register_emm_message(mme_ue_s1ap_id_t ue_id, const uint64_t puid, bstring nas_msg)
{
  emm_data_context_t *emm_data_context = emm_data_context_get(&_emm_data, ue_id);
  if (nas_msg) {
    nas_emm_proc_t * emm_proc = nas_emm_find_procedure_by_puid(emm_data_context, puid);

    if (emm_proc) {
      int index = emm_data_context->emm_procedures->nas_proc_mess_sign_next_location;
      emm_data_context->emm_procedures->nas_proc_mess_sign[index].nas_msg_length = blength(nas_msg);
      emm_data_context->emm_procedures->nas_proc_mess_sign[index].puid = puid;
      emm_data_context->emm_procedures->nas_proc_mess_sign[index].digest_length = NAS_MSG_DIGEST_SIZE;

      nas_digest_msg((const unsigned char * const)bdata(nas_msg), blength(nas_msg),
          (char * const)emm_data_context->emm_procedures->nas_proc_mess_sign[index].digest,
          &emm_data_context->emm_procedures->nas_proc_mess_sign[index].digest_length);

      emm_data_context->emm_procedures->nas_proc_mess_sign_next_location = (index + 1) % MAX_NAS_PROC_MESS_SIGN;
    } else {
      // forward to ESM, TODO later...
    }
  }
//  unlock_ue_contexts(ue_context);
}

//-----------------------------------------------------------------------------
nas_emm_proc_t * nas_emm_find_procedure_by_msg_digest(struct emm_data_context_s * const emm_context,
    const char * const digest,
    const size_t digest_bytes,
    const size_t msg_size)
{
  nas_emm_proc_t * emm_proc = NULL;
  if ((emm_context) && (emm_context->emm_procedures)) {
    for (int i = 0; i < MAX_NAS_PROC_MESS_SIGN; i++) {
      if (emm_context->emm_procedures->nas_proc_mess_sign[i].nas_msg_length == msg_size) {
        if (1 <= digest_bytes) {
          size_t min = min(digest_bytes, NAS_MSG_DIGEST_SIZE);
          if (!memcmp(digest, emm_context->emm_procedures->nas_proc_mess_sign[i].digest, min)) {
            emm_proc = nas_emm_find_procedure_by_puid(emm_context, emm_context->emm_procedures->nas_proc_mess_sign[i].puid);
            break;
          }
        } else {
          emm_proc = nas_emm_find_procedure_by_puid(emm_context, emm_context->emm_procedures->nas_proc_mess_sign[i].puid);
          break;
        }
      }
    }
  }
  return emm_proc;
}

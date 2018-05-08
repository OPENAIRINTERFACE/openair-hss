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

/*! \file mme_app_procedures.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "common_types.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "sgw_ie_defs.h"
#include "common_defs.h"
#include "mme_app_procedures.h"

static void mme_app_free_s11_procedure_create_bearer(mme_app_s11_proc_t **s11_proc);
static void mme_app_free_s10_procedure_inter_mme_handover(mme_app_s11_proc_t **s11_proc);

//------------------------------------------------------------------------------
void mme_app_delete_s11_procedures(ue_context_t * const ue_context_p)
{
  if (ue_context_p->s11_procedures) {
    mme_app_s11_proc_t *s11_proc1 = NULL;
    mme_app_s11_proc_t *s11_proc2 = NULL;

    s11_proc1 = LIST_FIRST(ue_context_p->s11_procedures);                 /* Faster List Deletion. */
    while (s11_proc1) {
      s11_proc2 = LIST_NEXT(s11_proc1, entries);
      if (MME_APP_S11_PROC_TYPE_CREATE_BEARER == s11_proc1->type) {
        mme_app_free_s11_procedure_create_bearer(&s11_proc1);
      } // else ...
      s11_proc1 = s11_proc2;
    }
    LIST_INIT(ue_context_p->s11_procedures);
    free_wrapper((void**)&ue_context_p->s11_procedures);
  }
}

//------------------------------------------------------------------------------
mme_app_s11_proc_create_bearer_t* mme_app_create_s11_procedure_create_bearer(ue_context_t * const ue_context_p)
{
  mme_app_s11_proc_create_bearer_t *s11_proc_create_bearer = calloc(1, sizeof(mme_app_s11_proc_create_bearer_t));
  s11_proc_create_bearer->proc.proc.type = MME_APP_BASE_PROC_TYPE_S11;
  s11_proc_create_bearer->proc.type      = MME_APP_S11_PROC_TYPE_CREATE_BEARER;
  mme_app_s11_proc_t *s11_proc = (mme_app_s11_proc_t *)s11_proc_create_bearer;

  if (!ue_context_p->s11_procedures) {
    ue_context_p->s11_procedures = calloc(1, sizeof(struct s11_procedures_s));
    LIST_INIT(ue_context_p->s11_procedures);
  }
  LIST_INSERT_HEAD((ue_context_p->s11_procedures), s11_proc, entries);
  return s11_proc_create_bearer;
}

//------------------------------------------------------------------------------
mme_app_s11_proc_create_bearer_t* mme_app_get_s11_procedure_create_bearer(ue_context_t * const ue_context_p)
{
  if (ue_context_p->s11_procedures) {
    mme_app_s11_proc_t *s11_proc = NULL;

    LIST_FOREACH(s11_proc, ue_context_p->s11_procedures, entries) {
      if (MME_APP_S11_PROC_TYPE_CREATE_BEARER == s11_proc->type) {
        return (mme_app_s11_proc_create_bearer_t*)s11_proc;
      }
    }
  }
  return NULL;
}
//------------------------------------------------------------------------------
void mme_app_delete_s11_procedure_create_bearer(ue_context_t * const ue_context_p)
{
  if (ue_context_p->s11_procedures) {
    mme_app_s11_proc_t *s11_proc = NULL;

    LIST_FOREACH(s11_proc, ue_context_p->s11_procedures, entries) {
      if (MME_APP_S11_PROC_TYPE_CREATE_BEARER == s11_proc->type) {
        LIST_REMOVE(s11_proc, entries);
        mme_app_free_s11_procedure_create_bearer(&s11_proc);
        return;
      }
    }
  }
}
//------------------------------------------------------------------------------
static void mme_app_free_s11_procedure_create_bearer(mme_app_s11_proc_t **s11_proc)
{
  // DO here specific releases (memory,etc)
  // nothing to do actually
  free_wrapper((void**)s11_proc);
}

/**
 * S10 Procedures.
 */
static int remove_s10_tunnel_endpoint(ue_context_t * ue_context, mme_app_s10_proc_t *s10_proc){
  OAILOG_FUNC_IN(LOG_MME_APP);
  int             rc = RETURNerror;
  /** Removed S10 tunnel endpoint. */
  mme_app_free_s10_procedure_inter_mme_handover(&s10_proc);
  /** Deregister the key. */
  mme_ue_context_update_coll_keys( &mme_app_desc.mme_ue_contexts,
      ue_context,
      ue_context->enb_s1ap_id_key,
      ue_context->mme_ue_s1ap_id,
      ue_context->imsi,
      ue_context->mme_teid_s11,
      INVALID_TEID,
      &ue_context->guti);
  OAILOG_FUNC_RETURN(LOG_MME_APP, rc);
}
//------------------------------------------------------------------------------
void mme_app_delete_s10_procedures(ue_context_t * const ue_context_p)
{
  if (ue_context_p->s10_procedures) {
    mme_app_s10_proc_t *s10_proc1 = NULL;
    mme_app_s10_proc_t *s10_proc2 = NULL;

    s10_proc1 = LIST_FIRST(ue_context_p->s10_procedures);                 /* Faster List Deletion. */
    while (s10_proc1) {
      s10_proc2 = LIST_NEXT(s10_proc1, entries);
      if (MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER == s10_proc1->type) {
        /** Stop the timer. */
        if(s10_proc1->timer.id != MME_APP_TIMER_INACTIVE_ID){
          if (timer_remove(s10_proc1->timer.id, NULL)) {
            OAILOG_ERROR (LOG_MME_APP, "Failed to stop the procedure timer for inter-MMME handover for UE id  %d \n", ue_context_p->mme_ue_s1ap_id);
            s10_proc1->timer.id = MME_APP_TIMER_INACTIVE_ID;
          }
        }
        s10_proc1->timer.id = MME_APP_TIMER_INACTIVE_ID;
        remove_s10_tunnel_endpoint(ue_context_p, s10_proc1);
      } // else ...
      s10_proc1 = s10_proc2;
    }
    LIST_INIT(ue_context_p->s10_procedures);
    free_wrapper((void**)&ue_context_p->s10_procedures);
  }
}

//------------------------------------------------------------------------------
static void
mme_app_handle_mme_s10_handover_completion_timer_expiry (mme_app_s10_proc_inter_mme_handover_t *s10_proc_inter_mme_handover)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  MessageDef                             *message_p = NULL;
  /** Get the IMSI. */
  imsi64_t imsi64 = imsi_to_imsi64(&s10_proc_inter_mme_handover->imsi);
  ue_context_t * ue_context = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, imsi64);
  DevAssert(ue_context);
  OAILOG_INFO (LOG_MME_APP, "Expired- MME S10 Handover Completion timer for UE " MME_UE_S1AP_ID_FMT " run out. "
      "Performing S1AP UE Context Release Command and successive NAS implicit detach. \n", ue_context_p->mme_ue_s1ap_id);
  s10_proc_inter_mme_handover->ho_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
  /** Delete the procedure. */
  mme_app_delete_s10_procedure_inter_mme_handover(ue_context)

  ue_context->s1_ue_context_release_cause = S1AP_HANDOVER_CANCELLED;
  /*
   * Send a UE Context Release Command which would trigger a context release.
   * The e_cgi IE will be set with Handover Notify.
   */

  mme_app_itti_ue_context_release(ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id, ue_context->s1_ue_context_release_cause, &s10_proc_inter_mme_handover->target_id.target_id);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
mme_app_s10_proc_inter_mme_handover_t* mme_app_create_s10_procedure_inter_mme_handover(ue_context_t * ue_context){
  mme_app_s10_proc_inter_mme_handover_t *s10_proc_inter_mme_handover = calloc(1, sizeof(mme_app_s10_proc_inter_mme_handover_t));
  // todo: checking hear for correct allocation
  if(!s10_proc_inter_mme_handover){
    return NULL;
  }
  s10_proc_inter_mme_handover->proc.type      = MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER;

  /*
   * Add the timeout method and start the timer.
   * We need a separate function and a timeout, since we saw in the tests with real equipment,
   * that sometimes after no tracking area update request follows the inter-MME handover.
   */
  s10_proc_inter_mme_handover->proc.proc.time_out = mme_app_handle_mme_s10_handover_completion_timer_expiry;
  /*
   * Start a fresh S10 MME Handover Completion timer for the forward relocation request procedure.
   * Give the procedure as the argument.
   */
  if (timer_setup (mme_config.mme_s10_handover_completion_timer, 0,
      TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *) s10_proc_inter_mme_handover, &(s10_proc_inter_mme_handover->ho_completion_timer.id)) < 0) {
    OAILOG_ERROR (LOG_MME_APP, "Failed to start the MME S10 Handover Completion timer for UE id " MME_UE_S1AP_ID_FMT " for duration %d \n", ue_context->mme_ue_s1ap_id,
        mme_config.mme_mobility_completion_timer);
    s10_proc_inter_mme_handover->ho_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
    /**
     * UE will be implicitly detached, if this timer runs out. It should be manually removed.
     * S10 FW Relocation Complete removes this timer.
     */
  } else {
    OAILOG_DEBUG (LOG_MME_APP, "MME APP : Activated the MME S10 Handover Completion timer UE id  " MME_UE_S1AP_ID_FMT ". Waiting for UE to go back from IDLE mode to ACTIVE mode.. Timer Id %u. "
        "Timer duration %d \n", ue_context->mme_ue_s1ap_id, s10_proc_inter_mme_handover->ho_completion_timer.id, mme_config.mme_s10_handover_completion_timer);
    /** Upon expiration, invalidate the timer.. no flag needed. */
  }

  /** Add the S10 procedure. */
  mme_app_s10_proc_t *s10_proc = (mme_app_s10_proc_t *)s10_proc_inter_mme_handover;

  if (!ue_context->s10_procedures) {
    ue_context->s10_procedures = calloc(1, sizeof(struct s10_procedures_s));
    LIST_INIT(ue_context->s10_procedures);
  }
  LIST_INSERT_HEAD((ue_context->s10_procedures), s10_proc, entries);
  return s10_proc_inter_mme_handover;
}

//------------------------------------------------------------------------------
mme_app_s10_proc_inter_mme_handover_t* mme_app_get_s10_procedure_inter_mme_handover(ue_context_t * const ue_context_p)
{
  if (ue_context_p->s10_procedures) {
    mme_app_s10_proc_t *s10_proc = NULL;

    LIST_FOREACH(s10_proc, ue_context_p->s10_procedures, entries) {
      if (MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER == s10_proc->type) {
        return (mme_app_s10_proc_create_bearer_t*)s10_proc;
      }
    }
  }
  return NULL;
}

//------------------------------------------------------------------------------
void mme_app_delete_s10_procedure_inter_mme_handover(ue_context_t * const ue_context)
{
  if (ue_context->s10_procedures) {
    mme_app_s10_proc_t *s10_proc = NULL;

    LIST_FOREACH(s10_proc, ue_context->s10_procedures, entries) {
      if (MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER == s10_proc->type) {
        LIST_REMOVE(s10_proc, entries);
        /*
         * Cannot remove the S10 tunnel endpoint with transaction.
         * The S10 tunnel endpoint will remain throughout the UE contexts lifetime.
         * It will be removed when the UE context is removed.
         * The UE context will also be registered by the S10 teid also.
         * We cannot get the process without getting the UE context, first.
         */
        /** Check if a timer is running, if so remove the timer. */
        if(s10_proc->timer.id != MME_APP_TIMER_INACTIVE_ID){
          if (timer_remove(s10_proc->timer.id, NULL)) {
            OAILOG_ERROR (LOG_MME_APP, "Failed to stop the procedure timer for inter-MMME handover for UE id  %d \n", ue_context_p->mme_ue_s1ap_id);
            s10_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
          }
        }
        s10_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
        /** Remove the S10 Tunnel endpoint and set the UE context S10 as invalid. */
        remove_s10_tunnel_endpoint(ue_context, s10_proc);
        mme_app_free_s10_procedure_inter_mme_handover(&s10_proc);
        return;
      }
    }
  }
}

//------------------------------------------------------------------------------
static void mme_app_free_s10_procedure_inter_mme_handover(mme_app_s10_proc_t **s10_proc)
{
  /** Remove the pending IEs. */
  mme_app_s10_proc_inter_mme_handover_t * s10_proc_inter_mme_handover = (mme_app_s10_proc_inter_mme_handover_t*) s10_proc;
  if(s10_proc_inter_mme_handover->nas_s10_context.mm_eps_ctx){
    free_wrapper(&s10_proc_inter_mme_handover->nas_s10_context.mm_eps_ctx); /**< Setting the reference inside the procedure also to null. */
  }
  if(s10_proc_inter_mme_handover->source_to_target_eutran_container){
    bdestroy(s10_proc_inter_mme_handover->source_to_target_eutran_container->container_value);
    free_wrapper(&s10_proc_inter_mme_handover->source_to_target_eutran_container); /**< Setting the reference inside the procedure also to null. */
  }
  /*
   * Todo: Make a generic function for this (proc_element with free_wrapper_ie() method).
   */
  if(s10_proc_inter_mme_handover->f_cause){
    free_wrapper(&s10_proc_inter_mme_handover->f_cause); /**< Setting the reference inside the procedure also to null. */
  }
  if(s10_proc_inter_mme_handover->peer_ip){
    free_wrapper(&s10_proc_inter_mme_handover->peer_ip); /**< Setting the reference inside the procedure also to null. */
  }
  // DO here specific releases (memory,etc)
  // nothing to do actually
  free_wrapper((void**)s10_proc);
}

/*
 * Our procedure is not for a single message but currently for a whole inter_mme handover procedure.
 */

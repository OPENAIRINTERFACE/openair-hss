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
#include "timer.h"
#include "mme_config.h"
#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "sgw_ie_defs.h"
#include "common_defs.h"
#include "mme_app_procedures.h"

static void mme_app_free_s11_procedure_create_bearer(mme_app_s11_proc_t **s11_proc);
static void mme_app_free_s10_procedure_mme_handover(mme_app_s10_proc_t **s10_proc);

/*---------------------------------------------------------------------------
   FTEID Set RBTree Search Data Structure
  --------------------------------------------------------------------------*/
//
///**
//  Comparator function for comparing two fteids.
//
//  @param[in] a: Pointer to bearer context a.
//  @param[in] b: Pointer to bearer context b.
//  @return  An integer greater than, equal to or less than zero according to whether the
//  object pointed to by a is greater than, equal to or less than the object pointed to by b.
//*/
//
//static
//inline int32_t                    mme_app_compare_bearer_context(
//    struct fteid_set_s *a,
//    struct fteid_set_s *b) {
//    if (a->s1u_fteid.teid > b->s1u_fteid.teid)
//      return 1;
//
//    if (a->s1u_fteid.teid < b->s1u_fteid.teid )
//      return -1;
//
//    /* Not more field to compare. */
//    return 0;
//}
//
//RB_GENERATE (BearerFteids, fteid_set_s, fteid_set_rbt_Node, fteid_set_compare_s1u_saegw)


//------------------------------------------------------------------------------
void mme_app_delete_s11_procedures(ue_context_t * const ue_context)
{
  if (ue_context->s11_procedures) {
    mme_app_s11_proc_t *s11_proc1 = NULL;
    mme_app_s11_proc_t *s11_proc2 = NULL;

    s11_proc1 = LIST_FIRST(ue_context->s11_procedures);                 /* Faster List Deletion. */
    while (s11_proc1) {
      s11_proc2 = LIST_NEXT(s11_proc1, entries);
      if (MME_APP_S11_PROC_TYPE_CREATE_BEARER == s11_proc1->type) {
        mme_app_free_s11_procedure_create_bearer(&s11_proc1);
      } // else ...
      s11_proc1 = s11_proc2;
    }
    LIST_INIT(ue_context->s11_procedures);
    free_wrapper((void**)&ue_context->s11_procedures);
  }
}

//------------------------------------------------------------------------------
mme_app_s11_proc_create_bearer_t* mme_app_create_s11_procedure_create_bearer(ue_context_t * const ue_context)
{
  mme_app_s11_proc_create_bearer_t *s11_proc_create_bearer = calloc(1, sizeof(mme_app_s11_proc_create_bearer_t));
  s11_proc_create_bearer->proc.proc.type = MME_APP_BASE_PROC_TYPE_S11;
  s11_proc_create_bearer->proc.type      = MME_APP_S11_PROC_TYPE_CREATE_BEARER;
  mme_app_s11_proc_t *s11_proc = (mme_app_s11_proc_t *)s11_proc_create_bearer;

  /** Initialize the of the procedure. */
  LIST_INIT(s11_proc_create_bearer->bearer_contexts_success);
  LIST_INIT(s11_proc_create_bearer->bearer_contexts_failed);


  if (!ue_context->s11_procedures) {
    ue_context->s11_procedures = calloc(1, sizeof(struct s11_procedures_s));
    LIST_INIT(ue_context->s11_procedures);
  }
  LIST_INSERT_HEAD((ue_context->s11_procedures), s11_proc, entries);

  return s11_proc_create_bearer;
}

//------------------------------------------------------------------------------
mme_app_s11_proc_create_bearer_t* mme_app_get_s11_procedure_create_bearer(ue_context_t * const ue_context)
{
  if (ue_context->s11_procedures) {
    mme_app_s11_proc_t *s11_proc = NULL;

    LIST_FOREACH(s11_proc, ue_context->s11_procedures, entries) {
      if (MME_APP_S11_PROC_TYPE_CREATE_BEARER == s11_proc->type) {
        return (mme_app_s11_proc_create_bearer_t*)s11_proc;
      }
    }
  }
  return NULL;
}
//------------------------------------------------------------------------------
void mme_app_delete_s11_procedure_create_bearer(ue_context_t * const ue_context)
{
  if (ue_context->s11_procedures) {
    mme_app_s11_proc_t *s11_proc = NULL;

    LIST_FOREACH(s11_proc, ue_context->s11_procedures, entries) {
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
static int remove_s10_tunnel_endpoint(ue_context_t * ue_context, struct in_addr peer_ip){
  OAILOG_FUNC_IN(LOG_MME_APP);
  int             rc = RETURNerror;
//  /** Removed S10 tunnel endpoint. */
  mme_app_remove_s10_tunnel_endpoint(ue_context->local_mme_teid_s10, peer_ip);
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
void mme_app_delete_s10_procedures(ue_context_t * const ue_context)
{
  if (ue_context->s10_procedures) {
    mme_app_s10_proc_t *s10_proc1 = NULL;
    mme_app_s10_proc_t *s10_proc2 = NULL;

    // todo: intra
    s10_proc1 = LIST_FIRST(ue_context->s10_procedures);                 /* Faster List Deletion. */
    while (s10_proc1) {
      s10_proc2 = LIST_NEXT(s10_proc1, entries);
      if (MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER == s10_proc1->type) {
        /** Stop the timer. */
        if(s10_proc1->timer.id != MME_APP_TIMER_INACTIVE_ID){
          if (timer_remove(s10_proc1->timer.id, NULL)) {
            OAILOG_ERROR (LOG_MME_APP, "Failed to stop the procedure timer for inter-MMME handover for UE id  %d \n", ue_context->mme_ue_s1ap_id);
            s10_proc1->timer.id = MME_APP_TIMER_INACTIVE_ID;
          }
        }
        s10_proc1->timer.id = MME_APP_TIMER_INACTIVE_ID;
//        if(s10_proc1->target_mme)
//          remove_s10_tunnel_endpoint(ue_context, s10_proc1);
      } // else ...
      else if (MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER == s10_proc1->type) {
        /** Stop the timer. */
        if(s10_proc1->timer.id != MME_APP_TIMER_INACTIVE_ID){
          if (timer_remove(s10_proc1->timer.id, NULL)) {
            OAILOG_ERROR (LOG_MME_APP, "Failed to stop the procedure timer for inter-MMME handover for UE id  %d \n", ue_context->mme_ue_s1ap_id);
            s10_proc1->timer.id = MME_APP_TIMER_INACTIVE_ID;
          }
        }
        s10_proc1->timer.id = MME_APP_TIMER_INACTIVE_ID;
      } // else ...
      s10_proc1 = s10_proc2;
    }
    LIST_INIT(ue_context->s10_procedures);
    free_wrapper((void**)&ue_context->s10_procedures);
  }
}

//------------------------------------------------------------------------------
static void
mme_app_handle_mme_s10_handover_completion_timer_expiry (mme_app_s10_proc_mme_handover_t *s10_proc_mme_handover)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  MessageDef                             *message_p = NULL;
  /** Get the IMSI. */
//  imsi64_t imsi64 = imsi_to_imsi64(&s10_proc_mme_handover->imsi);
  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, s10_proc_mme_handover->mme_ue_s1ap_id);
  DevAssert(ue_context);
  OAILOG_INFO (LOG_MME_APP, "Expired- MME S10 Handover Completion timer for UE " MME_UE_S1AP_ID_FMT " run out. "
      "Performing S1AP UE Context Release Command and successive NAS implicit detach. \n", ue_context->mme_ue_s1ap_id);
  s10_proc_mme_handover->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;
  /** Delete the procedure. */
  mme_app_delete_s10_procedure_mme_handover(ue_context);

  ue_context->s1_ue_context_release_cause = S1AP_HANDOVER_CANCELLED;
  /*
   * Send a UE Context Release Command which would trigger a context release.
   * The e_cgi IE will be set with Handover Notify.
   */

  mme_app_itti_ue_context_release(ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id, ue_context->s1_ue_context_release_cause, &s10_proc_mme_handover->target_id.target_id);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
static void
mme_app_handle_mobility_completion_timer_expiry (mme_app_s10_proc_mme_handover_t *s10_proc_mme_handover)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, s10_proc_mme_handover->mme_ue_s1ap_id);
  DevAssert (ue_context != NULL);
  MessageDef                             *message_p = NULL;
  OAILOG_INFO (LOG_MME_APP, "Expired- MME Handover Completion timer for UE " MME_UE_S1AP_ID_FMT " run out. \n", ue_context->mme_ue_s1ap_id);
  if(s10_proc_mme_handover->pending_clear_location_request){
    OAILOG_INFO (LOG_MME_APP, "CLR flag is set for UE " MME_UE_S1AP_ID_FMT ". Performing implicit detach. \n", ue_context->mme_ue_s1ap_id);
    s10_proc_mme_handover->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;
    ue_context->s1_ue_context_release_cause = S1AP_NAS_DETACH;
    /** Check if the CLR flag has been set. */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->mme_ue_s1ap_id; /**< We don't send a Detach Type such that no Detach Request is sent to the UE if handover is performed. */

    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  }else{
    OAILOG_WARNING(LOG_MME_APP, "CLR flag is NOT set for UE " MME_UE_S1AP_ID_FMT ". Not performing implicit detach. \n", ue_context->mme_ue_s1ap_id);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
mme_app_s10_proc_mme_handover_t* mme_app_create_s10_procedure_mme_handover(ue_context_t * const ue_context, bool target_mme, mme_app_s10_proc_type_t  s1ap_ho_type){
  mme_app_s10_proc_mme_handover_t *s10_proc_mme_handover = calloc(1, sizeof(mme_app_s10_proc_mme_handover_t));
  // todo: checking hear for correct allocation
  if(!s10_proc_mme_handover){
    return NULL;
  }
  s10_proc_mme_handover->proc.type      = s1ap_ho_type;
  s10_proc_mme_handover->proc.type      = s1ap_ho_type;
  s10_proc_mme_handover->mme_ue_s1ap_id = ue_context->mme_ue_s1ap_id;

  /*
   * Add the timeout method and start the timer.
   * We need a separate function and a timeout, since we saw in the tests with real equipment,
   * that sometimes after no tracking area update request follows the inter-MME handover.
   */
  if(target_mme){
    s10_proc_mme_handover->proc.proc.time_out = mme_app_handle_mme_s10_handover_completion_timer_expiry;
  }else{
    s10_proc_mme_handover->proc.proc.time_out = mme_app_handle_mobility_completion_timer_expiry;
  }
  /*
   * Start a fresh S10 MME Handover Completion timer for the forward relocation request procedure.
   * Give the procedure as the argument.
   */
  if(target_mme && s1ap_ho_type == MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER){
    if (timer_setup (mme_config.mme_s10_handover_completion_timer, 0,
        TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *) s10_proc_mme_handover, &(s10_proc_mme_handover->proc.timer.id)) < 0) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to start the MME Handover Completion timer for UE id " MME_UE_S1AP_ID_FMT " for duration %d \n", ue_context->mme_ue_s1ap_id,
          mme_config.mme_mobility_completion_timer);
      s10_proc_mme_handover->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;
      /**
       * UE will be implicitly detached, if this timer runs out. It should be manually removed.
       * S10 FW Relocation Complete removes this timer.
       */
    } else {
      OAILOG_DEBUG (LOG_MME_APP, "MME APP : Activated the MME Handover Completion timer UE id  " MME_UE_S1AP_ID_FMT ". "
          "Waiting for UE to go back from IDLE mode to ACTIVE mode.. Timer Id %u. Timer duration %d \n",
          ue_context->mme_ue_s1ap_id, s10_proc_mme_handover->proc.timer.id, mme_config.mme_s10_handover_completion_timer);
      /** Upon expiration, invalidate the timer.. no flag needed. */
    }
  }else{
    /*
     * The case that it is INTRA-MME-HANDOVER or source side, we don't start a mme_app_s10_proc timer.
     * We only leave the UE reference timer to remove the UE_Reference towards the source enb.
     * That's not part of the procedure and also should run if the process is removed.
     */
  }
    /** Add the S10 procedure. */
  mme_app_s10_proc_t *s10_proc = (mme_app_s10_proc_t *)s10_proc_mme_handover;

  s10_proc->target_mme = target_mme;
  if (!ue_context->s10_procedures) {
    ue_context->s10_procedures = calloc(1, sizeof(struct s10_procedures_s));
    LIST_INIT(ue_context->s10_procedures);
  }
  LIST_INSERT_HEAD((ue_context->s10_procedures), s10_proc, entries);
  return s10_proc_mme_handover;
}

//------------------------------------------------------------------------------
mme_app_s10_proc_mme_handover_t* mme_app_get_s10_procedure_mme_handover(ue_context_t * const ue_context)
{
  if (ue_context->s10_procedures) {
    mme_app_s10_proc_t *s10_proc = NULL;

    LIST_FOREACH(s10_proc, ue_context->s10_procedures, entries) {
      if (MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER == s10_proc->type) {
        return (mme_app_s10_proc_mme_handover_t*)s10_proc;
      }else if (MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER == s10_proc->type) {
        return (mme_app_s10_proc_mme_handover_t*)s10_proc;
      }
    }
  }
  return NULL;
}

//------------------------------------------------------------------------------
static void mme_app_free_s10_procedure_mme_handover(mme_app_s10_proc_t **s10_proc)
{
  /** Remove the pending IEs. */
  mme_app_s10_proc_mme_handover_t ** s10_proc_mme_handover_pp = (mme_app_s10_proc_mme_handover_t**) s10_proc;
  if((*s10_proc_mme_handover_pp)->nas_s10_context.mm_eps_ctx){
    free_wrapper(&((*s10_proc_mme_handover_pp)->nas_s10_context.mm_eps_ctx)); /**< Setting the reference inside the procedure also to null. */
  }
  if((*s10_proc_mme_handover_pp)->source_to_target_eutran_f_container.container_value){
    bdestroy_wrapper(&(*s10_proc_mme_handover_pp)->source_to_target_eutran_f_container.container_value);
  }
  /*
   * Todo: Make a generic function for this (proc_element with free_wrapper_ie() method).
   */
  /** PDN Connections. */
  if((*s10_proc_mme_handover_pp)->pdn_connections){
    free_wrapper(&((*s10_proc_mme_handover_pp)->pdn_connections)); /**< Setting the reference inside the procedure also to null. */
  }
  (*s10_proc_mme_handover_pp)->s10_mme_handover_timeout = NULL; // todo: deallocate too

//  (*s10_proc_mme_handover_pp)->entries.le_next = NULL;
//  (*s10_proc_mme_handover_pp)->entries.le_prev = NULL;

  // DO here specific releases (memory,etc)
  // nothing to do actually
  free_wrapper((void**)s10_proc_mme_handover_pp);
}

/*
 * Handover procedure used for inter and intra MME S1 handover.
 */
//------------------------------------------------------------------------------
void mme_app_delete_s10_procedure_mme_handover(ue_context_t * const ue_context)
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
            OAILOG_ERROR (LOG_MME_APP, "Failed to stop the procedure timer for -MMME handover for UE id  %d \n", ue_context->mme_ue_s1ap_id);
            s10_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
          }
        }
        s10_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
        /** Remove the S10 Tunnel endpoint and set the UE context S10 as invalid. */
//        if(s10_proc->target_mme)
          remove_s10_tunnel_endpoint(ue_context, s10_proc->peer_ip);
        mme_app_free_s10_procedure_mme_handover(&s10_proc);
        return;
      }else if (MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER == s10_proc->type){
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
            OAILOG_ERROR (LOG_MME_APP, "Failed to stop the procedure timer for -MMME handover for UE id  %d \n", ue_context->mme_ue_s1ap_id);
            s10_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
          }
        }
        s10_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
        /** Remove the S10 Tunnel endpoint and set the UE context S10 as invalid. */
//        remove_s10_tunnel_endpoint(ue_context, s10_proc);
        mme_app_free_s10_procedure_mme_handover(&s10_proc);
        return;
      }
    }
  }
}

/*
 * Our procedure is not for a single message but currently for a whole inter_mme handover procedure.
 */

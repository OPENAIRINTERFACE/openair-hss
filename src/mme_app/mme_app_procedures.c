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
#include "mce_app_mbms_service_context.h"
#include "mme_app_defs.h"
#include "mce_app_defs.h"
#include "common_defs.h"


static void mme_app_free_s11_procedure_create_bearer(mme_app_s11_proc_t **s11_proc);
static void mme_app_free_s11_procedure_update_bearer(mme_app_s11_proc_t **s11_proc);
static void mme_app_free_s11_procedure_delete_bearer(mme_app_s11_proc_t **s11_proc);
static void mme_app_free_s10_procedure_mme_handover(mme_app_s10_proc_t **s10_proc);
static void mme_app_free_sm_procedure_mbms_session_start(mme_app_sm_proc_t **sm_proc);
static void mme_app_free_sm_procedure_mbms_session_update(mme_app_sm_proc_t **sm_proc);
static void mme_app_free_sm_procedure_mbms_session_stop(mme_app_sm_proc_t **sm_proc);

//------------------------------------------------------------------------------
void mme_app_delete_s11_procedures(struct ue_session_pool_s * const ue_session_pool)
{
  mme_app_s11_proc_t *s11_proc1 = NULL;
  mme_app_s11_proc_t *s11_proc2 = NULL;

  s11_proc1 = LIST_FIRST(&ue_session_pool->s11_procedures);                 /* Faster List Deletion. */
  while (s11_proc1) {
    s11_proc2 = LIST_NEXT(s11_proc1, entries);
    if (MME_APP_S11_PROC_TYPE_CREATE_BEARER == s11_proc1->type) {
    	mme_app_free_s11_procedure_create_bearer(&s11_proc1);
    } else if (MME_APP_S11_PROC_TYPE_UPDATE_BEARER == s11_proc1->type) {
    	mme_app_free_s11_procedure_update_bearer(&s11_proc1);
    } else if (MME_APP_S11_PROC_TYPE_DELETE_BEARER == s11_proc1->type) {
    	mme_app_free_s11_procedure_delete_bearer(&s11_proc1);
    } // else ...
    s11_proc1 = s11_proc2;
  }
  LIST_INIT(&ue_session_pool->s11_procedures);
}

//------------------------------------------------------------------------------
mme_app_s11_proc_create_bearer_t* mme_app_create_s11_procedure_create_bearer(struct ue_session_pool_s * const ue_session_pool)
{
  /** Check if the list of S11 procedures is empty. */
  if(!LIST_EMPTY(&ue_session_pool->s11_procedures)){
	  OAILOG_ERROR (LOG_MME_APP, "UE with ueId " MME_UE_S1AP_ID_FMT " has already a S11 procedure ongoing. Cannot create CBR procedure. \n",
			  ue_session_pool->privates.mme_ue_s1ap_id);
	  return NULL;
  }

  mme_app_s11_proc_create_bearer_t *s11_proc_create_bearer = calloc(1, sizeof(mme_app_s11_proc_create_bearer_t));
  s11_proc_create_bearer->proc.proc.type = MME_APP_BASE_PROC_TYPE_S11;
  s11_proc_create_bearer->proc.type      = MME_APP_S11_PROC_TYPE_CREATE_BEARER;
  mme_app_s11_proc_t *s11_proc = (mme_app_s11_proc_t *)s11_proc_create_bearer;

  /** Initialize the of the procedure. */

  LIST_INIT(&ue_session_pool->s11_procedures);
  LIST_INSERT_HEAD((&ue_session_pool->s11_procedures), s11_proc, entries);

  return s11_proc_create_bearer;
}

//------------------------------------------------------------------------------
mme_app_s11_proc_t* mme_app_get_s11_procedure (struct ue_session_pool_s * const ue_session_pool)
{
  return LIST_FIRST(&ue_session_pool->s11_procedures);
}

//------------------------------------------------------------------------------
mme_app_s11_proc_create_bearer_t* mme_app_get_s11_procedure_create_bearer(struct ue_session_pool_s * const ue_session_pool)
{
  mme_app_s11_proc_t *s11_proc = NULL;

  LIST_FOREACH(s11_proc, &ue_session_pool->s11_procedures, entries) {
	if (MME_APP_S11_PROC_TYPE_CREATE_BEARER == s11_proc->type) {
		return (mme_app_s11_proc_create_bearer_t*)s11_proc;
	}
  }
  return NULL;
}

//------------------------------------------------------------------------------
void mme_app_delete_s11_procedure_create_bearer(struct ue_session_pool_s * const ue_session_pool)
{
  mme_app_s11_proc_t *s11_proc = NULL, *s11_proc_safe = NULL;

  LIST_FOREACH_SAFE(s11_proc, &ue_session_pool->s11_procedures, entries, s11_proc_safe) {
	  if (MME_APP_S11_PROC_TYPE_CREATE_BEARER == s11_proc->type) {
        LIST_REMOVE(s11_proc, entries);
        mme_app_free_s11_procedure_create_bearer(&s11_proc);
        return;
      }
    }

  if(LIST_EMPTY(&ue_session_pool->s11_procedures)){
	  LIST_INIT(&ue_session_pool->s11_procedures);
	  OAILOG_INFO (LOG_MME_APP, "UE with ueId " MME_UE_S1AP_ID_FMT " has no more S11 procedures left. Cleared the list. \n", ue_session_pool->privates.mme_ue_s1ap_id);
  }
}

//------------------------------------------------------------------------------
mme_app_s11_proc_update_bearer_t* mme_app_create_s11_procedure_update_bearer(struct ue_session_pool_s * const ue_session_pool)
{
  /** Check if the list of S11 procedures is empty. */
  if(!LIST_EMPTY(&ue_session_pool->s11_procedures)){
	OAILOG_ERROR (LOG_MME_APP, "UE with ueId " MME_UE_S1AP_ID_FMT " has already a S11 procedure ongoing. Cannot create UBR procedure. \n",
			ue_session_pool->privates.mme_ue_s1ap_id);
	return NULL;
  }
  mme_app_s11_proc_update_bearer_t *s11_proc_update_bearer = calloc(1, sizeof(mme_app_s11_proc_update_bearer_t));
  s11_proc_update_bearer->proc.proc.type = MME_APP_BASE_PROC_TYPE_S11;
  s11_proc_update_bearer->proc.type      = MME_APP_S11_PROC_TYPE_UPDATE_BEARER;
  mme_app_s11_proc_t *s11_proc = (mme_app_s11_proc_t *)s11_proc_update_bearer;

  /** Initialize the of the procedure. */

  LIST_INIT(&ue_session_pool->s11_procedures);
  LIST_INSERT_HEAD((&ue_session_pool->s11_procedures), s11_proc, entries);

  return s11_proc_update_bearer;
}

//------------------------------------------------------------------------------
mme_app_s11_proc_update_bearer_t* mme_app_get_s11_procedure_update_bearer(struct ue_session_pool_s * const ue_session_pool)
{
  mme_app_s11_proc_t *s11_proc = NULL;

  LIST_FOREACH(s11_proc, &ue_session_pool->s11_procedures, entries) {
	if (MME_APP_S11_PROC_TYPE_UPDATE_BEARER == s11_proc->type) {
		return (mme_app_s11_proc_create_bearer_t*)s11_proc;
	}
  }
  return NULL;
}

//------------------------------------------------------------------------------
void mme_app_delete_s11_procedure_update_bearer(struct ue_session_pool_s * const ue_session_pool)
{
  /** Check if the list of S11 procedures is empty. */
  mme_app_s11_proc_t *s11_proc = NULL, *s11_proc_safe = NULL;

  LIST_FOREACH_SAFE(s11_proc, &ue_session_pool->s11_procedures, entries, s11_proc_safe) {
	  if (MME_APP_S11_PROC_TYPE_UPDATE_BEARER == s11_proc->type) {
		  LIST_REMOVE(s11_proc, entries);
		  mme_app_free_s11_procedure_update_bearer(&s11_proc);
		  return;
	  }
  }

  if(LIST_EMPTY(&ue_session_pool->s11_procedures)){
 	  LIST_INIT(&ue_session_pool->s11_procedures);
 	  OAILOG_INFO (LOG_MME_APP, "UE with ueId " MME_UE_S1AP_ID_FMT " has no more S11 procedures left. Cleared the list. \n", ue_session_pool->privates.mme_ue_s1ap_id);
   }
}

//------------------------------------------------------------------------------
mme_app_s11_proc_delete_bearer_t* mme_app_create_s11_procedure_delete_bearer(struct ue_session_pool_s * const ue_session_pool)
{
	if(!LIST_EMPTY(&ue_session_pool->s11_procedures)){
		OAILOG_ERROR (LOG_MME_APP, "UE with ueId " MME_UE_S1AP_ID_FMT " has already a S11 procedure ongoing. Cannot create DBR procedure. \n",
				ue_session_pool->privates.mme_ue_s1ap_id);
		return NULL;
	}

  mme_app_s11_proc_delete_bearer_t *s11_proc_delete_bearer = calloc(1, sizeof(mme_app_s11_proc_delete_bearer_t));
  s11_proc_delete_bearer->proc.proc.type = MME_APP_BASE_PROC_TYPE_S11;
  s11_proc_delete_bearer->proc.type      = MME_APP_S11_PROC_TYPE_DELETE_BEARER;
  mme_app_s11_proc_t *s11_proc = (mme_app_s11_proc_t *)s11_proc_delete_bearer;

  /** Initialize the of the procedure. */

  LIST_INIT(&ue_session_pool->s11_procedures);
  LIST_INSERT_HEAD((&ue_session_pool->s11_procedures), s11_proc, entries);

  return s11_proc_delete_bearer;
}

//------------------------------------------------------------------------------
mme_app_s11_proc_delete_bearer_t* mme_app_get_s11_procedure_delete_bearer(struct ue_session_pool_s * const ue_session_pool)
{
	mme_app_s11_proc_t *s11_proc = NULL;

    LIST_FOREACH(s11_proc, &ue_session_pool->s11_procedures, entries) {
      if (MME_APP_S11_PROC_TYPE_DELETE_BEARER == s11_proc->type) {
        return (mme_app_s11_proc_delete_bearer_t*)s11_proc;
      }
    }
  return NULL;
}

//------------------------------------------------------------------------------
void mme_app_delete_s11_procedure_delete_bearer(struct ue_session_pool_s * const ue_session_pool)
{

    mme_app_s11_proc_t *s11_proc = NULL, *s11_proc_safe = NULL;


    LIST_FOREACH_SAFE(s11_proc, &ue_session_pool->s11_procedures, entries, s11_proc_safe) {
      if (MME_APP_S11_PROC_TYPE_DELETE_BEARER == s11_proc->type) {
        LIST_REMOVE(s11_proc, entries);
        mme_app_free_s11_procedure_delete_bearer(&s11_proc);
        return;
      }
    }

  if(LIST_EMPTY(&ue_session_pool->s11_procedures)){
 	  LIST_INIT(&ue_session_pool->s11_procedures);

 	  OAILOG_INFO (LOG_MME_APP, "UE with ueId " MME_UE_S1AP_ID_FMT " has no more S11 procedures left. Cleared the list. \n", ue_session_pool->privates.mme_ue_s1ap_id);
   }
}

//------------------------------------------------------------------------------
static void mme_app_free_s11_procedure_create_bearer(mme_app_s11_proc_t **s11_proc_cbr)
{
  // DO here specific releases (memory,etc)
  /** Remove the bearer contexts to be setup. */
  mme_app_s11_proc_create_bearer_t ** s11_proc_create_bearer_pp = (mme_app_s11_proc_create_bearer_t**)s11_proc_cbr;
  free_bearer_contexts_to_be_created(&(*s11_proc_create_bearer_pp)->bcs_tbc);
  free_wrapper((void**)s11_proc_create_bearer_pp);
}

//------------------------------------------------------------------------------
static void mme_app_free_s11_procedure_update_bearer(mme_app_s11_proc_t **s11_proc_ubr)
{
  // DO here specific releases (memory,etc)
  /** Remove the bearer contexts to be setup. */
  mme_app_s11_proc_update_bearer_t ** s11_proc_update_bearer_pp = (mme_app_s11_proc_update_bearer_t**)s11_proc_ubr;
  if((*s11_proc_update_bearer_pp)->bcs_tbu)
    free_bearer_contexts_to_be_updated(&(*s11_proc_update_bearer_pp)->bcs_tbu);
  free_wrapper((void**)s11_proc_update_bearer_pp);
}

//------------------------------------------------------------------------------
static void mme_app_free_s11_procedure_delete_bearer(mme_app_s11_proc_t **s11_proc_dbr)
{
  // DO here specific releases (memory,etc)
  /** Remove the bearer contexts to be setup. */
//  mme_app_s11_proc_delete_bearer_t ** s11_proc_delete_bearer_pp = (mme_app_s11_proc_delete_bearer_t**)s11_proc_dbr;
  free_wrapper((void**)s11_proc_dbr);
}

/**
 * S10 Procedures.
 */
static int remove_s10_tunnel_endpoint(struct ue_context_s * ue_context, struct sockaddr *peer_ip){
  OAILOG_FUNC_IN(LOG_MME_APP);
  int             rc = RETURNerror;
//  /** Removed S10 tunnel endpoint. */
  if(ue_context->privates.fields.local_mme_teid_s10 == (teid_t)0){
    OAILOG_ERROR (LOG_MME_APP, "UE with ueId " MME_UE_S1AP_ID_FMT " has no local S10 teid. Not triggering tunnel removal. \n", ue_context->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, rc);
  }
  rc = mme_app_remove_s10_tunnel_endpoint(ue_context->privates.fields.local_mme_teid_s10, peer_ip);
  /** Deregister the key. */
  mme_ue_context_update_coll_keys( &mme_app_desc.mme_ue_contexts,
      ue_context,
      ue_context->privates.enb_s1ap_id_key,
      ue_context->privates.mme_ue_s1ap_id,
      ue_context->privates.fields.imsi,
      ue_context->privates.fields.mme_teid_s11,
      INVALID_TEID,
      &ue_context->privates.fields.guti);
  OAILOG_FUNC_RETURN(LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
void mme_app_delete_s10_procedures(struct ue_context_s * const ue_context)
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
            OAILOG_ERROR (LOG_MME_APP, "Failed to stop the procedure timer for inter-MMME handover for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
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
            OAILOG_ERROR (LOG_MME_APP, "Failed to stop the procedure timer for inter-MMME handover for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
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
  struct ue_context_s * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, s10_proc_mme_handover->mme_ue_s1ap_id);
  DevAssert(ue_context);
  OAILOG_WARNING(LOG_MME_APP, "Expired- MME S10 Handover Completion timer for UE " MME_UE_S1AP_ID_FMT " run out. "
      "Performing S1AP UE Context Release Command and successive NAS implicit detach. \n", ue_context->privates.mme_ue_s1ap_id);
  s10_proc_mme_handover->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;

  /** Check if an NAS context exists (this might happen if a TAU_COMPLETE has not arrived or arrived and was discarded due security reasons. */
  emm_data_context_t * emm_context = emm_data_context_get(&_emm_data, ue_context->privates.mme_ue_s1ap_id);
  if(emm_context){
    OAILOG_WARNING (LOG_MME_APP, " **** ABNORMAL **** An EMM context for UE " MME_UE_S1AP_ID_FMT " exists. Performing implicit detach due time out of handover completion timer! \n", ue_context->privates.mme_ue_s1ap_id);
    /** Check if the CLR flag has been set. */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->privates.mme_ue_s1ap_id; /**< We don't send a Detach Type such that no Detach Request is sent to the UE if handover is performed. */

    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  } else {
	  ue_context->privates.s1_ue_context_release_cause = S1AP_HANDOVER_CANCELLED;
	  /*
	   * Send a UE Context Release Command which would trigger a context release.
	   * The e_cgi IE will be set with Handover Notify.
	   */
	  /** Send a FW-Relocation Response error if no local teid is set (no FW-Relocation Response is send yet). */
	  if(!ue_context->privates.fields.local_mme_teid_s10){
	    mme_app_send_s10_forward_relocation_response_err(s10_proc_mme_handover->remote_mme_teid.teid,
	        s10_proc_mme_handover->proc.peer_ip,
	        s10_proc_mme_handover->forward_relocation_trxn, REQUEST_REJECTED);
	  }
	  /** Delete the procedure. */
	  mme_app_delete_s10_procedure_mme_handover(ue_context);

	  /** Trigger an ESM detach, also removing all PDN contexts in the MME and the SAE-GW. */
	  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_ESM_DETACH_IND);
	  DevAssert (message_p != NULL);
	  message_p->ittiMsg.nas_esm_detach_ind.ue_id = ue_context->privates.mme_ue_s1ap_id; /**< We don't send a Detach Type such that no Detach Request is sent to the UE if handover is performed. */

	  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_ESM_DETACH_IND");
	  itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
	  OAILOG_FUNC_OUT (LOG_MME_APP);
  }
}

//------------------------------------------------------------------------------
static void
mme_app_handle_mobility_completion_timer_expiry (mme_app_s10_proc_mme_handover_t *s10_proc_mme_handover)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, s10_proc_mme_handover->mme_ue_s1ap_id);
  DevAssert (ue_context != NULL);
  MessageDef                             *message_p = NULL;
  OAILOG_INFO (LOG_MME_APP, "Expired- MME Handover Completion timer for UE " MME_UE_S1AP_ID_FMT " run out. \n", ue_context->privates.mme_ue_s1ap_id);
  /*
   * This timer is only expired in the inter-MME handover case for the source MME.
   * The timer will be stopped when successfully the S10 Forward Relocation Completion message arrives.
   */
  if(s10_proc_mme_handover->pending_clear_location_request){
    OAILOG_INFO (LOG_MME_APP, "CLR flag is set for UE " MME_UE_S1AP_ID_FMT ". Performing implicit detach. \n", ue_context->privates.mme_ue_s1ap_id);
    s10_proc_mme_handover->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;
    ue_context->privates.s1_ue_context_release_cause = S1AP_NAS_DETACH;
    /** Check if the CLR flag has been set. */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->privates.mme_ue_s1ap_id; /**< We don't send a Detach Type such that no Detach Request is sent to the UE if handover is performed. */
    message_p->ittiMsg.nas_implicit_detach_ue_ind.clr = s10_proc_mme_handover->pending_clear_location_request; /**< Inform about the CLR. */

    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
  }else{
    OAILOG_WARNING(LOG_MME_APP, "CLR flag is NOT set for UE " MME_UE_S1AP_ID_FMT ". Not performing implicit detach. \n", ue_context->privates.mme_ue_s1ap_id);
    /*
     * Handover failed on the target MME side.
     * Aborting the handover procedure and leaving the UE intact.
     * Going into Idle mode.
     */
    /** Remove the context in the target eNB, if it is INTRA-Handover. */
    if(s10_proc_mme_handover->proc.type == MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER){
      OAILOG_WARNING(LOG_MME_APP, "Releasing the request first for the target-ENB for INTRA handover for UE " MME_UE_S1AP_ID_FMT ". Not performing implicit detach. \n", ue_context->privates.mme_ue_s1ap_id);
      mme_app_itti_ue_context_release(ue_context->privates.mme_ue_s1ap_id, s10_proc_mme_handover->target_enb_ue_s1ap_id, S1AP_HANDOVER_FAILED, s10_proc_mme_handover->target_ecgi.cell_identity.enb_id);
      ue_context->privates.s1_ue_context_release_cause = S1AP_HANDOVER_FAILED;
    }else{
      OAILOG_WARNING(LOG_MME_APP, "Releasing the request for the source-ENB for INTER handover for UE " MME_UE_S1AP_ID_FMT ". Not performing implicit detach. \n", ue_context->privates.mme_ue_s1ap_id);
      /** Check if the UE is in connected mode. */
      if(ue_context->privates.fields.ecm_state == ECM_CONNECTED){
        OAILOG_DEBUG(LOG_MME_APP, "UE " MME_UE_S1AP_ID_FMT " is in ECM connected state (assuming handover). \n", ue_context->privates.mme_ue_s1ap_id);
        /*
         * Intra and we are source, so remove the handover procedure locally and send Release Command to source eNB.
         * We might have send HO-Command, but FW-Relocation-Complete message has not arrived.
         * So we must set the source ENB into idle mode manually.
         */
        if(s10_proc_mme_handover->ho_command_sent){ /**< This is the error timeout. The real mobility completion timeout comes from S1AP. */
          OAILOG_WARNING(LOG_MME_APP, "HO command already set for UE. Setting S1AP reference to idle mode for UE " MME_UE_S1AP_ID_FMT ". Not performing implicit detach. \n", ue_context->privates.mme_ue_s1ap_id);
          mme_app_itti_ue_context_release(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, S1AP_HANDOVER_FAILED, ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id);
          /** Remove the handover procedure. */
          mme_app_delete_s10_procedure_mme_handover(ue_context);
          OAILOG_FUNC_OUT (LOG_MME_APP);

        }else{
        	if(s10_proc_mme_handover->due_tau) {
        		/** Send a Notify Request to trigger idle TAU. */
        		OAILOG_WARNING(LOG_MME_APP, "Source MME CLR not received (Idle Tau - source side) for UE " MME_UE_S1AP_ID_FMT ". Triggering a notify to HSS. \n", ue_context->privates.mme_ue_s1ap_id);
        		mme_app_itti_notify_request(ue_context->privates.fields.imsi, &s10_proc_mme_handover->target_tai.plmn, true);
        		mme_app_delete_s10_procedure_mme_handover(ue_context);
        		OAILOG_FUNC_OUT (LOG_MME_APP);
        	} else {
        		/** This should not happen. The Ho-Cancel should come first. */
        		OAILOG_WARNING(LOG_MME_APP, "HO command not set yet for UE. Setting S1AP reference to idle mode for UE " MME_UE_S1AP_ID_FMT ". Not performing implicit detach. \n", ue_context->privates.mme_ue_s1ap_id);
        		mme_app_send_s1ap_handover_preparation_failure(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.fields.sctp_assoc_id_key, S1AP_HANDOVER_FAILED);

        		/** Inform the target side of the failure. */
        		message_p = itti_alloc_new_message (TASK_MME_APP, S10_RELOCATION_CANCEL_REQUEST);
        		DevAssert (message_p != NULL);
        		itti_s10_relocation_cancel_request_t *relocation_cancel_request_p = &message_p->ittiMsg.s10_relocation_cancel_request;
        		memset ((void*)relocation_cancel_request_p, 0, sizeof (itti_s10_relocation_cancel_request_t));
        		relocation_cancel_request_p->teid = s10_proc_mme_handover->proc.remote_teid; /**< May or may not be 0. */
        		relocation_cancel_request_p->local_teid = ue_context->privates.fields.local_mme_teid_s10; /**< May or may not be 0. */
        		// todo: check the table!
        		memcpy((void*)&relocation_cancel_request_p->mme_peer_ip, s10_proc_mme_handover->proc.peer_ip,
        				(s10_proc_mme_handover->proc.peer_ip->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));
        		/** IMSI. */
        		memcpy((void*)&relocation_cancel_request_p->imsi, &s10_proc_mme_handover->imsi, sizeof(imsi_t));
        		MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 RELOCATION_CANCEL_REQUEST_MESSAGE");
        		itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);

        		/** Not setting UE into idle mode at source (not changing the UE state). Still removing the S10 tunnel (see what happens..). */
        		remove_s10_tunnel_endpoint(ue_context, s10_proc_mme_handover->proc.peer_ip);
        		mme_app_delete_s10_procedure_mme_handover(ue_context);
        		OAILOG_FUNC_OUT (LOG_MME_APP);
        	}
        }
      }else{
        OAILOG_DEBUG(LOG_MME_APP, "UE " MME_UE_S1AP_ID_FMT " is in idle state (assuming idle tau). Removing handover procedure. \n", ue_context->privates.mme_ue_s1ap_id);
        /** Send a Notify Request to trigger idle TAU. */
        mme_app_itti_notify_request(ue_context->privates.fields.imsi, &s10_proc_mme_handover->target_tai.plmn, true);
        mme_app_delete_s10_procedure_mme_handover(ue_context);
        OAILOG_FUNC_OUT (LOG_MME_APP);
      }
    }

    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
}

//------------------------------------------------------------------------------
mme_app_s10_proc_mme_handover_t* mme_app_create_s10_procedure_mme_handover(struct ue_context_s * const ue_context,
		bool target_mme, mme_app_s10_proc_type_t  s1ap_ho_type, struct sockaddr* sockaddr){
  mme_app_s10_proc_mme_handover_t *s10_proc_mme_handover = calloc(1, sizeof(mme_app_s10_proc_mme_handover_t));
  // todo: checking hear for correct allocation
  if(!s10_proc_mme_handover){
    return NULL;
  }
  s10_proc_mme_handover->proc.type      = s1ap_ho_type;
  if(sockaddr) {
	  s10_proc_mme_handover->proc.peer_ip   = calloc(1, (sockaddr->sa_family == AF_INET6) ? sizeof(struct sockaddr_in6) :  sizeof(struct sockaddr_in));
	  memcpy(s10_proc_mme_handover->proc.peer_ip, sockaddr, (sockaddr->sa_family == AF_INET6) ? sizeof(struct sockaddr_in6) :  sizeof(struct sockaddr_in));
  }

  s10_proc_mme_handover->mme_ue_s1ap_id = ue_context->privates.mme_ue_s1ap_id;

  /*
   * Start a fresh S10 MME Handover Completion timer for the forward relocation request procedure.
   * Give the procedure as the argument.
   */
  if(s1ap_ho_type == MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER && target_mme){
    s10_proc_mme_handover->proc.proc.time_out = mme_app_handle_mme_s10_handover_completion_timer_expiry;
    mme_config_read_lock (&mme_config);
    if (timer_setup (mme_config.mme_s10_handover_completion_timer * 1, 0,
        TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,  (void *)(ue_context->privates.mme_ue_s1ap_id), &(s10_proc_mme_handover->proc.timer.id)) < 0) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to start the MME Handover Completion timer for UE id " MME_UE_S1AP_ID_FMT " for duration %d \n", ue_context->privates.mme_ue_s1ap_id,
          mme_config.mme_s10_handover_completion_timer * 1);
      s10_proc_mme_handover->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;
      /**
       * UE will be implicitly detached, if this timer runs out. It should be manually removed.
       * S10 FW Relocation Complete removes this timer.
       */
    } else {
      OAILOG_DEBUG (LOG_MME_APP, "MME APP : Activated the MME Handover Completion timer UE id  " MME_UE_S1AP_ID_FMT ". "
          "Waiting for UE to go back from IDLE mode to ACTIVE mode.. Timer Id %u. Timer duration %d \n",
          ue_context->privates.mme_ue_s1ap_id, s10_proc_mme_handover->proc.timer.id, mme_config.mme_s10_handover_completion_timer * 1);
      /** Upon expiration, invalidate the timer.. no flag needed. */
    }
    mme_config_unlock (&mme_config);
  }else{
    /*
     * The case that it is INTER-MME-HANDOVER or source side, we start a mme_app_s10_proc timer.
     * It is run until the Forward_Relocation_Complete message arrives.
     * It is used, if no Handover Notify message arrives at the target MME, that the source MME can eventually exit the handover procedure.
     */
    s10_proc_mme_handover->proc.proc.time_out = mme_app_handle_mobility_completion_timer_expiry;
    mme_config_read_lock (&mme_config);
    if (timer_setup (mme_config.mme_mobility_completion_timer * 1, 0,
        TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *)(ue_context->privates.mme_ue_s1ap_id), &(s10_proc_mme_handover->proc.timer.id)) < 0) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to start the MME Mobility Completion timer for UE id " MME_UE_S1AP_ID_FMT " for duration %d \n", ue_context->privates.mme_ue_s1ap_id,
          (mme_config.mme_mobility_completion_timer * 1));
      s10_proc_mme_handover->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;
      /**
       * UE will be implicitly detached, if this timer runs out. It should be manually removed.
       * S10 FW Relocation Complete removes this timer.
       */
    } else {
      OAILOG_DEBUG (LOG_MME_APP, "MME APP : Activated the MME Mobility Completion timer for the source MME for UE id  " MME_UE_S1AP_ID_FMT ". "
          "Waiting for UE to go back from IDLE mode to ACTIVE mode.. Timer Id %u. Timer duration %d \n",
          ue_context->privates.mme_ue_s1ap_id, s10_proc_mme_handover->proc.timer.id, (mme_config.mme_mobility_completion_timer * 1));
      /** Upon expiration, invalidate the timer.. no flag needed. */
    }
    mme_config_unlock (&mme_config);

  }
  /*
   * The case that it is INTRA-MME-HANDOVER or source side, we don't start a mme_app_s10_proc timer.
   * We only leave the UE reference timer to remove the UE_Reference towards the source enb.
   * That's not part of the procedure and also should run if the process is removed.
   */
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
mme_app_s10_proc_mme_handover_t* mme_app_get_s10_procedure_mme_handover(struct ue_context_s * const ue_context)
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
    free_mme_ue_eps_pdn_connections(&(*s10_proc_mme_handover_pp)->pdn_connections);
  }

  if((*s10_proc_mme_handover_pp)->proc.peer_ip){
    free_wrapper(&(*s10_proc_mme_handover_pp)->proc.peer_ip);
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
void mme_app_delete_s10_procedure_mme_handover(struct ue_context_s * const ue_context)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  if (ue_context->s10_procedures) {
    mme_app_s10_proc_t *s10_proc = NULL, *s10_proc_safe = NULL;

    LIST_FOREACH_SAFE(s10_proc, ue_context->s10_procedures, entries, s10_proc_safe) {
      if (MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER == s10_proc->type) {

    	if(s10_proc->target_mme && !((mme_app_s10_proc_mme_handover_t*)s10_proc)->handover_completed){
    		if(ue_context->privates.s1_ue_context_release_cause == S1AP_HANDOVER_CANCELLED
    				|| ue_context->privates.s1_ue_context_release_cause == S1AP_HANDOVER_FAILED){
        		OAILOG_WARNING(LOG_MME_APP, "Handover for UE " MME_UE_S1AP_ID_FMT " failed on the target side. Continuing with the deletion. \n", ue_context->privates.mme_ue_s1ap_id);
    		}else {
        		OAILOG_ERROR(LOG_MME_APP, "Handover for UE " MME_UE_S1AP_ID_FMT " on target MME has not been completed yet.. Cannot delete. \n", ue_context->privates.mme_ue_s1ap_id);
        		OAILOG_FUNC_OUT (LOG_MME_APP);
    		}
    	}

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
            OAILOG_ERROR (LOG_MME_APP, "Failed to stop the procedure timer for -MMME handover for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
            s10_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
          }else{
            OAILOG_DEBUG(LOG_MME_APP, "Successfully removed timer for -MMME handover for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
          }
        }
        s10_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
        /** Remove the S10 Tunnel endpoint and set the UE context S10 as invalid. */
//        if(s10_proc->target_mme)
        remove_s10_tunnel_endpoint(ue_context, s10_proc->peer_ip);
        mme_app_free_s10_procedure_mme_handover(&s10_proc);
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
            OAILOG_ERROR (LOG_MME_APP, "Failed to stop the procedure timer for -MMME handover for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
            s10_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
          }
        }
        s10_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
        /** Remove the S10 Tunnel endpoint and set the UE context S10 as invalid. */
        mme_app_free_s10_procedure_mme_handover(&s10_proc);
      }
    }
    /** Remove the list. */
    LIST_INIT(ue_context->s10_procedures);
    free_wrapper((void**)&ue_context->s10_procedures);

    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  OAILOG_INFO(LOG_MME_APP, "No S10 Procedures existing for UE " MME_UE_S1AP_ID_FMT ". \n", ue_context->privates.mme_ue_s1ap_id);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

/**
 * SM Procedures.
 */
//------------------------------------------------------------------------------
void mme_app_delete_sm_procedures(mbms_service_t * const mbms_service)
{
  mme_app_sm_proc_t *sm_proc1 = NULL;
  mme_app_sm_proc_t *sm_proc2 = NULL;

  /** The MBMS Service should already be locked. */
  sm_proc1 = LIST_FIRST(&mbms_service->sm_procedures);                 /* Faster List Deletion. */
  while (sm_proc1) {
    sm_proc2 = LIST_NEXT(sm_proc1, entries);
    if (MME_APP_SM_PROC_TYPE_MBMS_START_SESSION == sm_proc1->type) {
    	mme_app_free_sm_procedure_mbms_session_start(&sm_proc1);
    } else if (MME_APP_SM_PROC_TYPE_MBMS_UPDATE_SESSION == sm_proc1->type) {
    	mme_app_free_sm_procedure_mbms_session_update(&sm_proc1);
    } // else ...
    sm_proc1 = sm_proc2;
  }
  LIST_INIT(&mbms_service->sm_procedures);
}

//------------------------------------------------------------------------------
mme_app_sm_proc_t* mme_app_get_sm_procedure (const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_service_area_id)
{
  mbms_service_t * mbms_service = mce_mbms_service_exists_tmgi(&mce_app_desc.mce_mbms_service_contexts, tmgi, mbms_service_area_id);
  if(!mbms_service)
	return NULL;

  return LIST_FIRST(&mbms_service->sm_procedures);
}

//------------------------------------------------------------------------------
mme_app_sm_proc_mbms_session_start_t * mme_app_create_sm_procedure_mbms_session_start(const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_service_area_id, const mbms_session_duration_t * const mbms_session_duration)
{
  mbms_service_t * mbms_service = mce_mbms_service_exists_tmgi(&mce_app_desc.mce_mbms_service_contexts, tmgi, mbms_service_area_id);
  if(!mbms_service)
    return NULL;

  /** Check if the list of Sm procedures is empty. */
  if(!LIST_EMPTY(&mbms_service->sm_procedures)){
	OAILOG_ERROR (LOG_MME_APP, "MBMS Service with TMGI " TMGI_FMT " has already a Sm procedure ongoing. Cannot create new MBMS Start procedure. \n",
	  TMGI_ARG(&mbms_service->privates.fields.tmgi));
	return NULL;
  }

  mme_app_sm_proc_mbms_session_start_t *sm_proc_mbms_session_start = calloc(1, sizeof(mme_app_sm_proc_mbms_session_start_t));
  sm_proc_mbms_session_start->proc.proc.type = MME_APP_BASE_PROC_TYPE_SM;
  sm_proc_mbms_session_start->proc.type      = MME_APP_SM_PROC_TYPE_MBMS_START_SESSION;

  /** Get the MBMS Service Index. */
  mbms_service_index_t mbms_service_idx = mce_get_mbms_service_index(tmgi, mbms_service_area_id);
  DevAssert(mbms_service_idx != INVALID_MBMS_SERVICE_INDEX);

  mme_config_read_lock (&mme_config);
  /**
   * Set the timer, depending on the duration of the MBMS Session Duration and the configuration, the MBMS Session may be terminated, or just checked for existence (clear-up).
   * Mark the MME procedure, depending on the timeout (we won't know after wakeup).
   * We also use the same timer, to check if the session has been successfully established and if not to remove it.
   */
  if(mme_config.mbms_short_idle_session_duration_in_sec > mbms_session_duration->seconds){
	  OAILOG_INFO(LOG_MME_APP, "MBMS Session Start procedure for MBMS Service-Index " MCE_MBMS_SERVICE_INDEX_FMT " has session duration (%ds) is shorter/equal than the minimum (%ds). \n",
			 mbms_service_idx, mbms_session_duration, mme_config.mbms_short_idle_session_duration_in_sec);
//	  sm_proc_mbms_session_start->proc.trigger_mbms_session_stop = true;
  }
  if (timer_setup (mbms_session_duration->seconds, 0,
    TASK_MCE_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,  (void *) (mbms_service_idx), &(sm_proc_mbms_session_start->proc.timer.id)) < 0) {
	  OAILOG_ERROR (LOG_MME_APP, "Failed to start the MME MBMS Session Start timer for MBMS Service Idx " MCE_MBMS_SERVICE_INDEX_FMT " for duration %ds \n",
		mbms_service_idx, mbms_session_duration);
	  sm_proc_mbms_session_start->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;
//	  sm_proc_mbms_session_start->proc.trigger_mbms_session_stop = false;
  }
  mme_config_unlock (&mme_config);
  mme_app_sm_proc_t *sm_proc = (mme_app_sm_proc_t *)sm_proc_mbms_session_start;

  /** Initialize the of the procedure. */
  LIST_INIT(&mbms_service->sm_procedures);
  LIST_INSERT_HEAD((&mbms_service->sm_procedures), sm_proc, entries);
  return sm_proc_mbms_session_start;
}


//------------------------------------------------------------------------------
mme_app_sm_proc_mbms_session_start_t* mme_app_get_sm_procedure_mbms_session_start(const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_service_area_id)
{
  mbms_service_t * mbms_service = mce_mbms_service_exists_tmgi(&mce_app_desc.mce_mbms_service_contexts, tmgi, mbms_service_area_id);
  if(!mbms_service)
	return NULL;

  mme_app_sm_proc_t *sm_proc = NULL;
  LIST_FOREACH(sm_proc, &mbms_service->sm_procedures, entries) {
	if (MME_APP_SM_PROC_TYPE_MBMS_START_SESSION == sm_proc->type) {
		return (mme_app_sm_proc_mbms_session_start_t*)sm_proc;
	}
  }
  return NULL;
}

//------------------------------------------------------------------------------
void mme_app_delete_sm_procedure_mbms_session_start(const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_service_area_id)
{
  mbms_service_t * mbms_service = mce_mbms_service_exists_tmgi(&mce_app_desc.mce_mbms_service_contexts, tmgi, mbms_service_area_id);
  if(!mbms_service)
    return;

  mme_app_sm_proc_t *sm_proc = NULL, *sm_proc_safe = NULL;
  LIST_FOREACH_SAFE(sm_proc, &mbms_service->sm_procedures, entries, sm_proc_safe) {
    if (MME_APP_SM_PROC_TYPE_MBMS_START_SESSION == sm_proc->type) {
      LIST_REMOVE(sm_proc, entries);
      if(sm_proc->timer.id != MME_APP_TIMER_INACTIVE_ID){
        if (timer_remove(sm_proc->timer.id, NULL)) {
          OAILOG_ERROR (LOG_MME_APP, "Failed to stop the procedure timer for -MMME MBMS Service Start with TMGI " TMGI_FMT ". \n", TMGI_ARG(tmgi));
          sm_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
        }else{
          OAILOG_DEBUG(LOG_MME_APP, "Successfully removed timer for -MMME MBMS Service Start for TMGI " TMGI_FMT ". \n", TMGI_ARG(tmgi));
        }
      }
//      sm_proc->trigger_mbms_session_stop = false;
      mme_app_free_sm_procedure_mbms_session_start(&sm_proc);
      return;
    }
  }
  if(LIST_EMPTY(&mbms_service->sm_procedures)){
	  LIST_INIT(&mbms_service->sm_procedures);
	  OAILOG_INFO (LOG_MME_APP, "MBMS Service with TMGI " TMGI_FMT " has no more Sm procedures left. Cleared the list. \n",
		TMGI_ARG(&mbms_service->privates.fields.tmgi));
  }
}

//------------------------------------------------------------------------------
mme_app_sm_proc_mbms_session_update_t * mme_app_create_sm_procedure_mbms_session_update(const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_service_area_id, const mbms_session_duration_t * const mbms_session_duration)
{
  mbms_service_t * mbms_service = mce_mbms_service_exists_tmgi(&mce_app_desc.mce_mbms_service_contexts, tmgi, mbms_service_area_id);
  if(!mbms_service)
    return NULL;

  /** Check if the list of Sm procedures is empty. */
  if(!LIST_EMPTY(&mbms_service->sm_procedures)){
	OAILOG_ERROR (LOG_MME_APP, "MBMS Service with TMGI " TMGI_FMT " has already a Sm procedure ongoing. Cannot create new MBMS Update procedure. \n",
		TMGI_ARG(&mbms_service->privates.fields.tmgi));
	return NULL;
  }

  mme_app_sm_proc_mbms_session_update_t *sm_proc_mbms_session_update = calloc(1, sizeof(mme_app_sm_proc_mbms_session_update_t));
  sm_proc_mbms_session_update->proc.proc.type = MME_APP_BASE_PROC_TYPE_SM;
  sm_proc_mbms_session_update->proc.type      = MME_APP_SM_PROC_TYPE_MBMS_UPDATE_SESSION;

  /** Get the MBMS Service Index. */
  mbms_service_index_t mbms_service_idx = mce_get_mbms_service_index(tmgi, mbms_service_area_id);
  DevAssert(mbms_service_idx != INVALID_MBMS_SERVICE_INDEX);

  mme_config_read_lock (&mme_config);
  /**
   * Set the timer, depending on the duration of the MBMS Session Duration and the configuration, the MBMS Session may be terminated, or just checked for existence (clear-up).
   * Mark the MME procedure, depending on the timeout (we won't know after wakeup).
   * We also use the same timer, to check if the session has been successfully established and if not to remove it.
   */
  if(mme_config.mbms_short_idle_session_duration_in_sec > mbms_session_duration->seconds){
	  OAILOG_INFO(LOG_MME_APP, "MBMS Session Update procedure for MBMS Service-Index " MCE_MBMS_SERVICE_INDEX_FMT " has session duration (%ds) is shorter/equal than the minimum (%ds). \n",
			 mbms_service_idx, mbms_session_duration, mme_config.mbms_short_idle_session_duration_in_sec);
//	  sm_proc_mbms_session_update->proc.trigger_mbms_session_stop = true;
  }
  if (timer_setup (mbms_session_duration->seconds, 0,
    TASK_MCE_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,  (void *) (mbms_service_idx), &(sm_proc_mbms_session_update->proc.timer.id)) < 0) {
	  OAILOG_ERROR (LOG_MME_APP, "Failed to start the MME MBMS Session update timer for MBMS Service Idx " MCE_MBMS_SERVICE_INDEX_FMT " for duration %ds \n",
		mbms_service_idx, mbms_session_duration);
	  sm_proc_mbms_session_update->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;
//	  sm_proc_mbms_session_update->proc.trigger_mbms_session_stop = false;
  }
  mme_config_unlock (&mme_config);
  mme_app_sm_proc_t *sm_proc = (mme_app_sm_proc_t *)sm_proc_mbms_session_update;

  /** Initialize the of the procedure. */
  LIST_INIT(&mbms_service->sm_procedures);
  LIST_INSERT_HEAD((&mbms_service->sm_procedures), sm_proc, entries);
  return sm_proc_mbms_session_update;
}

//------------------------------------------------------------------------------
mme_app_sm_proc_mbms_session_update_t* mme_app_get_sm_procedure_mbms_session_update(const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_service_area_id)
{
  mme_app_sm_proc_t *sm_proc = NULL;
  mbms_service_t * mbms_service = mce_mbms_service_exists_tmgi(&mce_app_desc.mce_mbms_service_contexts, tmgi, mbms_service_area_id);
  if(!mbms_service)
    return NULL;

  LIST_FOREACH(sm_proc, &mbms_service->sm_procedures, entries) {
	if (MME_APP_SM_PROC_TYPE_MBMS_UPDATE_SESSION == sm_proc->type) {
		return (mme_app_sm_proc_mbms_session_start_t*)sm_proc;
	}
  }
  return NULL;
}

//------------------------------------------------------------------------------
void mme_app_delete_sm_procedure_mbms_session_update(const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_service_area_id)
{
  /** Check if the list of Sm procedures is empty. */
  mme_app_sm_proc_t *sm_proc = NULL, *sm_proc_safe = NULL;
  mbms_service_t * mbms_service = mce_mbms_service_exists_tmgi(&mce_app_desc.mce_mbms_service_contexts, tmgi, mbms_service_area_id);
  if(!mbms_service)
    return;

  LIST_FOREACH_SAFE(sm_proc, &mbms_service->sm_procedures, entries, sm_proc_safe) {
	  if (MME_APP_SM_PROC_TYPE_MBMS_UPDATE_SESSION == sm_proc->type) {
		  LIST_REMOVE(sm_proc, entries);
	      if(sm_proc->timer.id != MME_APP_TIMER_INACTIVE_ID){
	        if (timer_remove(sm_proc->timer.id, NULL)) {
	          OAILOG_ERROR (LOG_MME_APP, "Failed to stop the procedure timer for -MMME MBMS Service Update with TMGI " TMGI_FMT ". \n", TMGI_ARG(tmgi));
	          sm_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
	        }else{
	          OAILOG_DEBUG(LOG_MME_APP, "Successfully removed timer for -MMME MBMS Service Update for TMGI " TMGI_FMT ". \n", TMGI_ARG(tmgi));
	        }
	      }
//	      sm_proc->trigger_mbms_session_stop = false;
	      mme_app_free_sm_procedure_mbms_session_update(&sm_proc);
		  return;
	  }
  }

  if(LIST_EMPTY(&mbms_service->sm_procedures)){
 	  LIST_INIT(&mbms_service->sm_procedures);
 	  OAILOG_INFO (LOG_MME_APP, "MBMS with TMGI " TMGI_FMT " has no more Sm procedures left. Cleared the list. \n", TMGI_ARG(&mbms_service->privates.fields.tmgi));
   }
}

//------------------------------------------------------------------------------
static void mme_app_free_sm_procedure_mbms_session_start(mme_app_sm_proc_t **sm_proc_mssr)
{
  // DO here specific releases (memory,etc)
//  /** Remove the MBMS Service stuff, to be set . */
  mme_app_sm_proc_mbms_session_start_t ** sm_proc_mbms_session_start_pp = (mme_app_sm_proc_mbms_session_start_t**)sm_proc_mssr;
//  free_bearer_contexts_to_be_created(&(*sm_proc_mbms_session_start_pp)->bcs_tbc);
  free_wrapper((void**)sm_proc_mbms_session_start_pp);
}

//------------------------------------------------------------------------------
static void mme_app_free_sm_procedure_mbms_session_update(mme_app_sm_proc_t **sm_proc_msur)
{
  // DO here specific releases (memory,etc)
	//  /** Remove the MBMS Service stuff, to be set . */
  mme_app_sm_proc_mbms_session_update_t ** sm_proc_mbms_session_update_pp = (mme_app_sm_proc_mbms_session_update_t**)sm_proc_msur;
  if((*sm_proc_mbms_session_update_pp)->bc_tbu.bearer_level_qos)
    free_bearer_contexts_to_be_updated(&((*sm_proc_mbms_session_update_pp)->bc_tbu.bearer_level_qos));
  free_wrapper((void**)sm_proc_mbms_session_update_pp);
}

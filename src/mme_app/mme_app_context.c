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

/*! \file mme_app_context.c
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <inttypes.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "gcc_diag.h"
#include "dynamic_memory_check.h"
#include "assertions.h"
#include "log.h"
#include "msc.h"
#include "3gpp_requirements_36.413.h"
#include "common_types.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "enum_string.h"
#include "timer.h"
#include "mme_app_ue_context.h"
#include "mme_app_bearer_context.h"
#include "mme_app_defs.h"
#include "mme_app_itti_messaging.h"
#include "mme_app_procedures.h"
#include "mme_app_pdn_context.h"
#include "s1ap_mme.h"
#include "common_defs.h"
#include "esm_ebr.h"

// todo: think about locking the MME_APP context or EMM context, which one to lock, why to lock at all? lock seperately?
////------------------------------------------------------------------------------
//int lock_ue_contexts(ue_context_t * const ue_context) {
//  int rc = RETURNerror;
//  if (ue_context) {
//    struct timeval start_time;
//    gettimeofday(&start_time, NULL);
//    struct timespec wait = {0}; // timed is useful for debug
//    wait.tv_sec=start_time.tv_sec + 5;
//    wait.tv_nsec=start_time.tv_usec*1000;
//    rc = pthread_mutex_timedlock(&ue_context->recmutex, &wait);
//    if (rc) {
//      OAILOG_ERROR (LOG_MME_APP, "Cannot lock UE context mutex, err=%s\n", strerror(rc));
//#if ASSERT_MUTEX
//      struct timeval end_time;
//      gettimeofday(&end_time, NULL);
//      AssertFatal(!rc, "Cannot lock UE context mutex, err=%s took %ld seconds \n", strerror(rc), end_time.tv_sec - start_time.tv_sec);
//#endif
//    }
//#if DEBUG_MUTEX
//    OAILOG_TRACE (LOG_MME_APP, "UE context mutex locked, count %d lock %d\n",
//        ue_context->recmutex.__data.__count, ue_context->recmutex.__data.__lock);
//#endif
//  }
//  return rc;
//}
////------------------------------------------------------------------------------
//int unlock_ue_contexts(ue_context_t * const ue_context) {
//  int rc = RETURNerror;
//  if (ue_context) {
//    rc = pthread_mutex_unlock(&ue_context->recmutex);
//    if (rc) {
//      OAILOG_ERROR (LOG_MME_APP, "Cannot unlock UE context mutex, err=%s\n", strerror(rc));
//    }
//#if DEBUG_MUTEX
//    OAILOG_TRACE (LOG_MME_APP, "UE context mutex unlocked, count %d lock %d\n",
//        ue_context->recmutex.__data.__count, ue_context->recmutex.__data.__lock);
//#endif
//  }
//  return rc;
//}
//

//------------------------------------------------------------------------------
ue_context_t *mme_create_new_ue_context (void)
{
  ue_context_t                           *new_p = calloc (1, sizeof (ue_context_t));
  // todo: if MME_APP is to be locked,
  pthread_mutexattr_t mutexattr = {0};
  int rc = pthread_mutexattr_init(&mutexattr);
  if (rc) {
    OAILOG_ERROR (LOG_MME_APP, "Cannot create UE context, failed to init mutex attribute: %s\n", strerror(rc));
    return NULL;
  }
  rc = pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
  if (rc) {
    OAILOG_ERROR (LOG_MME_APP, "Cannot create UE context, failed to set mutex attribute type: %s\n", strerror(rc));
    return NULL;
  }
  rc = pthread_mutex_init(&new_p->recmutex, &mutexattr);
  if (rc) {
    OAILOG_ERROR (LOG_MME_APP, "Cannot create UE context, failed to init mutex: %s\n", strerror(rc));
    return NULL;
  }
//  rc = lock_ue_contexts(new_p);
  if (rc) {
    OAILOG_ERROR (LOG_MME_APP, "Cannot create UE context, failed to lock mutex: %s\n", strerror(rc));
    return NULL;
  }

  new_p->mme_ue_s1ap_id = INVALID_MME_UE_S1AP_ID;
  new_p->enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;
  /** EMM context is separated. So we don't have to initialize the EMM context, but have to set the MME_APP context identifiers. */
  //  emm_init_context(&new_p->emm_context, true);

  // todo: in integ_broadband, where are the MME_APP timers, not used at all? moved to EMM? we definitely need them.
  // Initialize timers to INVALID IDs
  new_p->mobile_reachability_timer.id = MME_APP_TIMER_INACTIVE_ID;
  new_p->implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;
  new_p->initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
//  new_p->mme_s10_handover_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
//  new_p->mme_mobility_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;

//  new_p->ue_radio_cap_length = 0;
  new_p->s1_ue_context_release_cause = S1AP_INVALID_CAUSE;
  RB_INIT(&new_p->pdn_contexts);

  /*
   * Get 11 new bearers from the bearer pool.
   * They might or might not be pre-allocated.
   */
  for(uint8_t ebi = 5; ebi < 5 + MAX_NUM_BEARERS_UE - 1; ebi++) {
    /** Insert 11 new bearers. */
    bearer_context_t * bearer_ctx_p = mme_app_new_bearer();
    DevAssert (bearer_ctx_p != NULL);
    bearer_ctx_p->ebi = ebi;
    RB_INSERT (BearerPool, &(new_p->bearer_pool), bearer_ctx_p);
  }
  return new_p;
}


//------------------------------------------------------------------------------
void mme_app_free_pdn_connection (pdn_context_t ** const pdn_connection)
{
  bdestroy_wrapper(&(*pdn_connection)->apn_in_use);
  bdestroy_wrapper(&(*pdn_connection)->apn_oi_replacement);
  free_wrapper((void**)pdn_connection);
}

//------------------------------------------------------------------------------
void mme_app_ue_context_free_content (ue_context_t * const ue_context)
{
  bdestroy_wrapper (&ue_context->msisdn);
  bdestroy_wrapper (&ue_context->ue_radio_capability);
  bdestroy_wrapper (&ue_context->apn_oi_replacement);

  DevAssert(ue_context != NULL);

  // Stop Mobile reachability timer,if running
  if (ue_context->mobile_reachability_timer.id != MME_APP_TIMER_INACTIVE_ID) {
    if (timer_remove(ue_context->mobile_reachability_timer.id, NULL)) {

      OAILOG_ERROR (LOG_MME_APP, "Failed to stop Mobile Reachability timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
    }
    ue_context->mobile_reachability_timer.id = MME_APP_TIMER_INACTIVE_ID;
  }
  // Stop Implicit detach timer,if running
  if (ue_context->implicit_detach_timer.id != MME_APP_TIMER_INACTIVE_ID) {
    if (timer_remove(ue_context->implicit_detach_timer.id, NULL)) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to stop Implicit Detach timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
    }
    ue_context->implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;
  }

  // Stop Initial context setup process guard timer,if running
  if (ue_context->initial_context_setup_rsp_timer.id != MME_APP_TIMER_INACTIVE_ID) {
    if (timer_remove(ue_context->initial_context_setup_rsp_timer.id, NULL)) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to stop Initial Context Setup Rsp timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
    }
    ue_context->initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  }

//  /** Reset the source MME handover timer. */
//  if (ue_context->mme_mobility_completion_timer.id != MME_APP_TIMER_INACTIVE_ID) {
//    if (timer_remove(ue_context->mme_mobility_completion_timer.id, NULL)) {
//      OAILOG_ERROR (LOG_MME_APP, "Failed to stop MME Mobility Completion timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
//    }
//    ue_context->mme_mobility_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
//  }
//
//  /** Reset the target MME handover timer. */
//  if (ue_context->mme_s10_handover_completion_timer.id != MME_APP_TIMER_INACTIVE_ID) {
//    if (timer_remove(ue_context->mme_s10_handover_completion_timer.id, NULL)) {
//      OAILOG_ERROR (LOG_MME_APP, "Failed to stop MME S10 Handover Completion timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
//    }
//    ue_context->mme_s10_handover_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
//  }

  if (ue_context->ue_radio_capability) {
    free_wrapper((void**) &(ue_context->ue_radio_capability));
  }
//  ue_context->ue_radio_cap_length = 0;

  ue_context->s1_ue_context_release_cause = S1AP_INVALID_CAUSE;

  // todo: remove all the MME_APP UE context bearer contexts (available bearers) --> When this point is reached, NAS is purged--> All pdn/bearer  contexts should have been removed!

//  pdn_context_t * registered_pdn_ctx = NULL;
//  RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_context->pdn_contexts) {
//    DevAssert(registered_pdn_ctx);
//    mme_app_deregister_bearer_context(registered_pdn_ctx, registered_pdn_ctx, BEARER_STATE_NULL);
//    /** The number of bearers will be incremented in the method. S10 should just pick the ebi. */
//  }

//  for (int i = 0; i < BEARERS_PER_UE; i++) {
//    if (ue_context->bearer_contexts[i]) {
//      mme_app_deregister_bearer_context(&ue_context->bearer_contexts[i]);
//    }
//  }
//
//
//  // todo: when is this method called? it should put the bearer contexts of the pdn context back to the UE contexts bearer list!
//  for (int i = 0; i < MAX_APN_PER_UE; i++) {
//    if (ue_context->pdn_contexts[i]) {
//      mme_app_free_pdn_connection(&ue_context->pdn_contexts[i]);
//    }
//  }


  /** Will remove the S10 tunnel endpoints. */
  if (ue_context->s10_procedures) {
    mme_app_delete_s10_procedures(ue_context);
  }

  if (ue_context->s11_procedures) {
    mme_app_delete_s11_procedures(ue_context);
  }

  // todo: unlock?
  memset(ue_context, 0, sizeof(*ue_context));
}

////------------------------------------------------------------------------------
//ue_context_t                           *
//mme_ue_context_exists_enb_ue_s1ap_id (
//  mme_ue_context_t * const mme_ue_context_p,
//  const enb_s1ap_id_key_t enb_key)
//{
//  struct ue_context_s                    *ue_context = NULL;
//
//  hashtable_ts_get (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)enb_key, (void **)&ue_context);
//  if (ue_context) {
//    OAILOG_TRACE (LOG_MME_APP, "Fetched ue_context (key " MME_APP_ENB_S1AP_ID_KEY_FORMAT ") MME_UE_S1AP_ID=" MME_UE_S1AP_ID_FMT " ENB_UE_S1AP_ID=" ENB_UE_S1AP_ID_FMT " @%p\n",
//        enb_key, ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id, ue_context);
//  }
//  return ue_context;
//}

////------------------------------------------------------------------------------
//ue_context_t                           *
//mme_ue_context_exists_mme_ue_s1ap_id (
//  mme_ue_context_t * const mme_ue_context_p,
//  const mme_ue_s1ap_id_t mme_ue_s1ap_id)
//{
//  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
//  uint64_t                                enb_s1ap_id_key64 = 0;
//
//  h_rc = hashtable_uint64_ts_get (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)mme_ue_s1ap_id, &enb_s1ap_id_key64);
//
//  if (HASH_TABLE_OK == h_rc) {
//    return mme_ue_context_exists_enb_ue_s1ap_id (mme_ue_context_p, (enb_s1ap_id_key_t) enb_s1ap_id_key64);
//  }
//
//  return NULL;
//}


//------------------------------------------------------------------------------
ue_context_t                           *
mme_ue_context_exists_enb_ue_s1ap_id (
  mme_ue_context_t * const mme_ue_context_p,
  const enb_s1ap_id_key_t enb_key)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
  uint64_t                                mme_ue_s1ap_id64 = 0;

  hashtable_uint64_ts_get (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)enb_key, &mme_ue_s1ap_id64);

  if (HASH_TABLE_OK == h_rc) {
    return mme_ue_context_exists_mme_ue_s1ap_id (mme_ue_context_p, (mme_ue_s1ap_id_t) mme_ue_s1ap_id64);
  }
  return NULL;
}

//------------------------------------------------------------------------------
ue_context_t                           *
mme_ue_context_exists_mme_ue_s1ap_id (
  mme_ue_context_t * const mme_ue_context_p,
  const mme_ue_s1ap_id_t mme_ue_s1ap_id)
{
  struct ue_context_s                    *ue_context = NULL;

  hashtable_ts_get (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)mme_ue_s1ap_id, (void **)&ue_context);
  if (ue_context) {
//    lock_ue_contexts(ue_context);
//    OAILOG_TRACE (LOG_MME_APP, "UE  " MME_UE_S1AP_ID_FMT " fetched MM state %s, ECM state %s\n ",mme_ue_s1ap_id,
//        (ue_context->mm_state == UE_UNREGISTERED) ? "UE_UNREGISTERED":(ue_context->mm_state == UE_REGISTERED) ? "UE_REGISTERED":"UNKNOWN",
//        (ue_context->ecm_state == ECM_IDLE) ? "ECM_IDLE":(ue_context->ecm_state == ECM_CONNECTED) ? "ECM_CONNECTED":"UNKNOWN");
  }
  return ue_context;

}

//------------------------------------------------------------------------------
struct ue_context_s                    *
mme_ue_context_exists_imsi (
  mme_ue_context_t * const mme_ue_context_p,
  const imsi64_t imsi)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
  uint64_t                                mme_ue_s1ap_id64 = 0;

  h_rc = hashtable_uint64_ts_get (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)imsi, &mme_ue_s1ap_id64);

  if (HASH_TABLE_OK == h_rc) {
    return mme_ue_context_exists_mme_ue_s1ap_id (mme_ue_context_p, (mme_ue_s1ap_id_t)mme_ue_s1ap_id64);
  }

  return NULL;
}

//------------------------------------------------------------------------------
struct ue_context_s                    *
mme_ue_context_exists_s11_teid (
  mme_ue_context_t * const mme_ue_context_p,
  const s11_teid_t teid)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
  uint64_t                                mme_ue_s1ap_id64 = 0;

  h_rc = hashtable_uint64_ts_get (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)teid, &mme_ue_s1ap_id64);

  if (HASH_TABLE_OK == h_rc) {
    return mme_ue_context_exists_mme_ue_s1ap_id (mme_ue_context_p, (mme_ue_s1ap_id_t) mme_ue_s1ap_id64);
  }
  return NULL;
}

//------------------------------------------------------------------------------
struct ue_context_s                    *
mme_ue_context_exists_s10_teid (
  mme_ue_context_t * const mme_ue_context_p,
  const s10_teid_t teid)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
  uint64_t                                mme_ue_s1ap_id64 = 0;

  h_rc = hashtable_uint64_ts_get (mme_ue_context_p->tun10_ue_context_htbl, (const hash_key_t)teid, &mme_ue_s1ap_id64);

  if (HASH_TABLE_OK == h_rc) {
    return mme_ue_context_exists_mme_ue_s1ap_id (mme_ue_context_p, (mme_ue_s1ap_id_t) mme_ue_s1ap_id64);
  }
  return NULL;
}

//------------------------------------------------------------------------------
ue_context_t                           *
mme_ue_context_exists_guti (
  mme_ue_context_t * const mme_ue_context_p,
  const guti_t * const guti_p)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
  uint64_t                                mme_ue_s1ap_id64 = 0;

  h_rc = obj_hashtable_uint64_ts_get (mme_ue_context_p->guti_ue_context_htbl, (const void *)guti_p, sizeof (*guti_p), &mme_ue_s1ap_id64);

  if (HASH_TABLE_OK == h_rc) {
    return mme_ue_context_exists_mme_ue_s1ap_id (mme_ue_context_p, (mme_ue_s1ap_id_t)mme_ue_s1ap_id64);
  }

  return NULL;
}

//------------------------------------------------------------------------------
void mme_app_move_context (ue_context_t *dst, ue_context_t *src)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  if ((dst) && (src)) {
    OAILOG_DEBUG (LOG_MME_APP,
           "mme_app_move_context("ENB_UE_S1AP_ID_FMT " <- " ENB_UE_S1AP_ID_FMT ")\n",
           dst->enb_ue_s1ap_id, src->enb_ue_s1ap_id);
    mme_app_ue_context_free_content(dst);
    memcpy(dst, src, sizeof(*dst)); // dangerous (memory leaks), TODO check
    memset(src, 0, sizeof(*src));

    // todo: setting the IDs of the destination again?
//    dst->enb_s1ap_id_key =  enb_s1ap_id_key;
//    dst->enb_ue_s1ap_id =  enb_ue_s1ap_id;
//    dst->mme_ue_s1ap_id =  mme_ue_s1ap_id;
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

// todo: take a look at this later..
////------------------------------------------------------------------------------
//void mme_app_move_context (ue_context_t *dst, ue_context_t *src)
//{
//  OAILOG_FUNC_IN (LOG_MME_APP);
//  if ((dst) && (src)) {
//    dst->imsi                = src->imsi;
//    dst->imsi_auth           = src->imsi_auth;
//    //enb_s1ap_id_key
//    //enb_ue_s1ap_id
//    //mme_ue_s1ap_id
//    dst->sctp_assoc_id_key       = src->sctp_assoc_id_key;
//    dst->subscription_known      = src->subscription_known;
//    memcpy((void *)dst->msisdn, (const void *)src->msisdn, sizeof(src->msisdn));
//    dst->msisdn_length           = src->msisdn_length;src->msisdn_length = 0;
//    dst->mm_state                = src->mm_state;
//    dst->ecm_state               = src->ecm_state;
//    dst->is_guti_set             = src->is_guti_set;
//    dst->guti                    = src->guti;
//    dst->me_identity             = src->me_identity;
//    dst->e_utran_cgi             = src->e_utran_cgi;
//    dst->cell_age                = src->cell_age;
//    dst->access_mode             = src->access_mode;
//    dst->apn_profile             = src->apn_profile;
//    dst->access_restriction_data = src->access_restriction_data;
//    dst->sub_status              = src->sub_status;
//    dst->subscribed_ambr         = src->subscribed_ambr;
//    dst->used_ambr               = src->used_ambr;
//    dst->rau_tau_timer           = src->rau_tau_timer;
//    dst->mme_s11_teid            = src->mme_s11_teid;
//    dst->sgw_s11_teid            = src->sgw_s11_teid;
//    memcpy((void *)dst->pending_pdn_connectivity_req_imsi, (const void *)src->pending_pdn_connectivity_req_imsi, sizeof(src->pending_pdn_connectivity_req_imsi));
//    dst->pending_pdn_connectivity_req_imsi_length = src->pending_pdn_connectivity_req_imsi_length;
//    dst->pending_pdn_connectivity_req_apn         = src->pending_pdn_connectivity_req_apn;
//    src->pending_pdn_connectivity_req_apn         = NULL;
//
//    dst->pending_pdn_connectivity_req_pdn_addr    = src->pending_pdn_connectivity_req_pdn_addr;
//    src->pending_pdn_connectivity_req_pdn_addr    = NULL;
//    dst->pending_pdn_connectivity_req_pti         = src->pending_pdn_connectivity_req_pti;
//    dst->pending_pdn_connectivity_req_ue_id       = src->pending_pdn_connectivity_req_ue_id;
//    dst->pending_pdn_connectivity_req_qos         = src->pending_pdn_connectivity_req_qos;
//    dst->pending_pdn_connectivity_req_pco         = src->pending_pdn_connectivity_req_pco;
//    dst->pending_pdn_connectivity_req_proc_data   = src->pending_pdn_connectivity_req_proc_data;
//    src->pending_pdn_connectivity_req_proc_data   = NULL;
//    dst->pending_pdn_connectivity_req_request_type= src->pending_pdn_connectivity_req_request_type;
//    dst->default_bearer_id       = src->default_bearer_id;
//    // todo: move the bearer hash table
////    memcpy((void *)dst->eps_bearers, (const void *)src->eps_bearers, sizeof(bearer_context_t)*BEARERS_PER_UE);
////    OAILOG_DEBUG (LOG_MME_APP,
////           "mme_app_move_context("ENB_UE_S1AP_ID_FMT " <- " ENB_UE_S1AP_ID_FMT ") done\n",
////           dst->enb_ue_s1ap_id, src->enb_ue_s1ap_id);
//  }
//  OAILOG_FUNC_OUT (LOG_MME_APP);
//}

// todo: use this method in the NAS layer to remove an old MME_APP UE context permanently!
//------------------------------------------------------------------------------
// a duplicate ue_mm context can be detected only while receiving an INITIAL UE message
// moves old ue context content to new ue context
ue_context_t *
mme_ue_context_duplicate_enb_ue_s1ap_id_detected (
  const enb_s1ap_id_key_t new_enb_key,
  const mme_ue_s1ap_id_t  old_mme_ue_s1ap_id,
  const bool              is_remove_old)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
  uint64_t                                id64 = 0;
  enb_ue_s1ap_id_t                        new_enb_ue_s1ap_id = 0;
  enb_s1ap_id_key_t                       old_enb_key = 0;

  new_enb_ue_s1ap_id = MME_APP_ENB_S1AP_ID_KEY2ENB_S1AP_ID(new_enb_key);

  if (INVALID_MME_UE_S1AP_ID == old_mme_ue_s1ap_id) {
    OAILOG_ERROR (LOG_MME_APP,
        "Error could not associate this enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
        new_enb_ue_s1ap_id, old_mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, NULL);
  }
  h_rc = hashtable_uint64_ts_get (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)old_mme_ue_s1ap_id, &id64);
  if (HASH_TABLE_OK == h_rc) {
    old_enb_key = (enb_s1ap_id_key_t) id64;
    if (old_enb_key != new_enb_key) {
      if (is_remove_old) {
        ue_context_t                           *old = NULL;
        h_rc = hashtable_uint64_ts_remove (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)old_mme_ue_s1ap_id);
        h_rc = hashtable_uint64_ts_insert (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)old_mme_ue_s1ap_id, (uint64_t)new_enb_key);
        h_rc = hashtable_ts_remove (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)old_enb_key, (void **)&old);
        if (HASH_TABLE_OK == h_rc) {
          ue_context_t                           *new = NULL;
          h_rc = hashtable_ts_get (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)new_enb_key, (void **)&new);
          mme_app_move_context(new, old);
          new->enb_s1ap_id_key = new_enb_key;
          new->enb_ue_s1ap_id  = new_enb_ue_s1ap_id;
          new->mme_ue_s1ap_id  = old_mme_ue_s1ap_id;

          //mme_app_ue_context_free_content(old);
          OAILOG_DEBUG (LOG_MME_APP,
                  "Removed old UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
                  MME_APP_ENB_S1AP_ID_KEY2ENB_S1AP_ID(old_enb_key), old_mme_ue_s1ap_id);
          OAILOG_FUNC_RETURN(LOG_MME_APP, new);
        }
      } else { // remove new context
        ue_context_t                           *new = NULL;
        h_rc = hashtable_uint64_ts_remove (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)old_mme_ue_s1ap_id);
        h_rc = hashtable_uint64_ts_insert (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)old_mme_ue_s1ap_id, (uint64_t)new_enb_key);
        h_rc = hashtable_ts_remove (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)new_enb_key, (void **)&new);
        if (HASH_TABLE_OK != h_rc) {
          OAILOG_ERROR (LOG_MME_APP,"Could not remove new UE context new enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " \n",new_enb_ue_s1ap_id);
        }
        ue_context_t                           *old = NULL;
        h_rc = hashtable_ts_remove (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)old_enb_key, (void **)&old);
        if (HASH_TABLE_OK == h_rc) {
          h_rc = hashtable_ts_insert (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)new_enb_key, (void *)old);
          old->enb_s1ap_id_key = new_enb_key;
          old->enb_ue_s1ap_id  = new_enb_ue_s1ap_id;
        } else {
          OAILOG_ERROR (LOG_MME_APP,"Could not insert old UE context with new enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " \n",new_enb_ue_s1ap_id);
        }
        mme_app_ue_context_free_content(new);
        free_wrapper((void**)&new);

        s1ap_notified_new_ue_mme_s1ap_id_association (old->sctp_assoc_id_key, old->enb_ue_s1ap_id, old->mme_ue_s1ap_id);

        OAILOG_DEBUG (LOG_MME_APP,
            "Removed new UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
            new_enb_ue_s1ap_id, old_mme_ue_s1ap_id);
        OAILOG_FUNC_RETURN(LOG_MME_APP, old);
      }
    } else {
      OAILOG_DEBUG (LOG_MME_APP,
          "No duplicated context found enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
          new_enb_ue_s1ap_id, old_mme_ue_s1ap_id);
    }
  } else {
    OAILOG_ERROR (LOG_MME_APP,
            "Error could find this mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
            old_mme_ue_s1ap_id);
  }
  OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
}

////------------------------------------------------------------------------------
//// this is detected only while receiving an INITIAL UE message
//
///***********************************************IMPORTANT*****************************************************
// * We are not using this function. If plan to use this in future then the key insertion and removal within this
// * function need to modified.
// **********************************************IMPORTANT*****************************************************/
//void
//mme_ue_context_duplicate_enb_ue_s1ap_id_detected (
//  const enb_s1ap_id_key_t enb_key,
//  const mme_ue_s1ap_id_t  mme_ue_s1ap_id,
//  const bool              is_remove_old)
//{
//  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
//  void                                   *id = NULL;
//  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;
//  enb_s1ap_id_key_t                       old_enb_key = 0;
//
//  OAILOG_FUNC_IN (LOG_MME_APP);
//  enb_ue_s1ap_id = MME_APP_ENB_S1AP_ID_KEY2ENB_S1AP_ID(enb_key);
//
//  if (INVALID_MME_UE_S1AP_ID == mme_ue_s1ap_id) {
//    OAILOG_ERROR (LOG_MME_APP,
//        "Error could not associate this enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
//        enb_ue_s1ap_id, mme_ue_s1ap_id);
//    OAILOG_FUNC_OUT (LOG_MME_APP);
//  }
//  h_rc = hashtable_ts_get (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)mme_ue_s1ap_id,  (void **)&id);
//  if (HASH_TABLE_OK == h_rc) {
//    old_enb_key = (enb_s1ap_id_key_t)(uintptr_t) id;
//    if (old_enb_key != enb_key) {
//      if (is_remove_old) {
//        ue_context_t                           *old = NULL;
//        /* TODO
//         * Insert and remove need to be corrected. mme_ue_s1ap_id is used to point to context ptr and
//         * enb_ue_s1ap_id_key is used to point to mme_ue_s1ap_id
//         */
//        h_rc = hashtable_ts_remove (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)mme_ue_s1ap_id, (void **)&id);
//        h_rc = hashtable_ts_insert (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)mme_ue_s1ap_id, (void *)(uintptr_t)enb_key);
//        h_rc = hashtable_ts_remove (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)old_enb_key, (void **)&old);
//        if (HASH_TABLE_OK == h_rc) {
//          ue_context_t                           *new = NULL;
//          h_rc = hashtable_ts_get (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)enb_key, (void **)&new);
//          mme_app_move_context(new, old);
//          mme_app_ue_context_free_content(old);
//          OAILOG_DEBUG (LOG_MME_APP,
//                  "Removed old UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
//                  MME_APP_ENB_S1AP_ID_KEY2ENB_S1AP_ID(old_enb_key), mme_ue_s1ap_id);
//        }
//      } else {
//        ue_context_t                           *new = NULL;
//        h_rc = hashtable_ts_remove (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)enb_key, (void **)&new);
//        if (HASH_TABLE_OK == h_rc) {
//          mme_app_ue_context_free_content(new);
//          OAILOG_DEBUG (LOG_MME_APP,
//                  "Removed new UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
//                  enb_ue_s1ap_id, mme_ue_s1ap_id);
//        }
//      }
//    } else {
//      OAILOG_DEBUG (LOG_MME_APP,
//          "No duplicated context found enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
//          enb_ue_s1ap_id, mme_ue_s1ap_id);
//    }
//  } else {
//    OAILOG_ERROR (LOG_MME_APP,
//            "Error could find this mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
//            mme_ue_s1ap_id);
//  }
//  OAILOG_FUNC_OUT (LOG_MME_APP);
//}



// todo: definitely set the mme_app_s1ap_ue_id as the main key.
// todo: where is it used? NAS uses this ?
//------------------------------------------------------------------------------
int
mme_ue_context_notified_new_ue_s1ap_id_association (
  const enb_s1ap_id_key_t  enb_key,
  const mme_ue_s1ap_id_t   mme_ue_s1ap_id)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
  ue_context_t                           *ue_context = NULL;
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;

  OAILOG_FUNC_IN (LOG_MME_APP);
  enb_ue_s1ap_id = MME_APP_ENB_S1AP_ID_KEY2ENB_S1AP_ID(enb_key);

  if (INVALID_MME_UE_S1AP_ID == mme_ue_s1ap_id) {
    OAILOG_ERROR (LOG_MME_APP,
        "Error could not associate this enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
        enb_ue_s1ap_id, mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  ue_context = mme_ue_context_exists_enb_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, enb_key);
  if (ue_context) {
    if (ue_context->enb_s1ap_id_key == enb_key) { // useless
      if (INVALID_MME_UE_S1AP_ID == ue_context->mme_ue_s1ap_id) {
        // new insertion of mme_ue_s1ap_id, not a change in the id
        h_rc = hashtable_uint64_ts_insert (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)mme_ue_s1ap_id, (uint64_t)enb_key);
        // todo: insert via mme_app_ue_s1ap_id
//                h_rc = hashtable_ts_insert (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)mme_ue_s1ap_id, (void *)ue_context);
        if (HASH_TABLE_OK == h_rc) {
          ue_context->mme_ue_s1ap_id = mme_ue_s1ap_id;
          OAILOG_DEBUG (LOG_MME_APP,
              "Associated this enb_ue_s1ap_ue_id " ENB_UE_S1AP_ID_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
              ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id);

          s1ap_notified_new_ue_mme_s1ap_id_association (ue_context->sctp_assoc_id_key, enb_ue_s1ap_id, mme_ue_s1ap_id);
          //          unlock_ue_contexts(ue_context);
          OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
        }
      }
    }
    //    unlock_ue_contexts(ue_context);
  }
  OAILOG_ERROR (LOG_MME_APP,
      "Error could not associate this enb_ue_s1ap_ue_id " ENB_UE_S1AP_ID_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
      enb_ue_s1ap_id, mme_ue_s1ap_id);
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
}

// todo: eventually remove GUTI and also no need for variablility in mme_app_ue_s1ap_id.
// first of all we assert here and second we register the newly created object once with the mme_insert_ue_context method!

// todo: check the locks here
//------------------------------------------------------------------------------
void
mme_ue_context_update_coll_keys (
  mme_ue_context_t * const mme_ue_context_p,
  ue_context_t     * const ue_context,
  const enb_s1ap_id_key_t  enb_s1ap_id_key,
  const mme_ue_s1ap_id_t   mme_ue_s1ap_id,
  const imsi64_t     imsi,
  const s11_teid_t         mme_teid_s11,
  const s10_teid_t         local_mme_teid_s10,
  const guti_t     * const guti_p)  //  never NULL, if none put &ue_context->guti
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
  void                                   *id = NULL;

  OAILOG_FUNC_IN(LOG_MME_APP);

  OAILOG_TRACE (LOG_MME_APP, "Update ue context.enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ue context.mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ue context.IMSI " IMSI_64_FMT " ue context.GUTI "GUTI_FMT"\n",
      ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id, ue_context->imsi, GUTI_ARG(&ue_context->guti));

  if (guti_p) {
    OAILOG_TRACE (LOG_MME_APP, "Update ue context %p enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " GUTI " GUTI_FMT "\n",
        ue_context, ue_context->enb_ue_s1ap_id, mme_ue_s1ap_id, imsi, GUTI_ARG(guti_p));
  } else {
    OAILOG_TRACE (LOG_MME_APP, "Update ue context %p enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT "\n",
        ue_context, ue_context->enb_ue_s1ap_id, mme_ue_s1ap_id, imsi);
  }
  //  AssertFatal(ue_context->enb_s1ap_id_key == enb_s1ap_id_key,
  //      "Mismatch in UE context enb_s1ap_id_key "MME_APP_ENB_S1AP_ID_KEY_FORMAT"/"MME_APP_ENB_S1AP_ID_KEY_FORMAT"\n",
  //      ue_context->enb_s1ap_id_key, enb_s1ap_id_key);

  AssertFatal((ue_context->mme_ue_s1ap_id == mme_ue_s1ap_id)
      && (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id),
      "Mismatch in UE context mme_ue_s1ap_id "MME_UE_S1AP_ID_FMT"/"MME_UE_S1AP_ID_FMT"\n",
      ue_context->mme_ue_s1ap_id, mme_ue_s1ap_id);

  if ((INVALID_ENB_UE_S1AP_ID_KEY != enb_s1ap_id_key) && (ue_context->enb_s1ap_id_key != enb_s1ap_id_key)) {
    // new insertion of enb_ue_s1ap_id_key,
    h_rc = hashtable_ts_remove (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context->enb_s1ap_id_key, (void **)&id);
    h_rc = hashtable_ts_insert (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)enb_s1ap_id_key, (void *)(uintptr_t)mme_ue_s1ap_id);

    if (HASH_TABLE_OK != h_rc) {
      OAILOG_ERROR (LOG_MME_APP,
          "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " %s\n",
          ue_context, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id, hashtable_rc_code2string(h_rc));
    }
    ue_context->enb_s1ap_id_key = enb_s1ap_id_key;
  }


  if ((INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id) && (ue_context->mme_ue_s1ap_id != mme_ue_s1ap_id)) {
    // new insertion of mme_ue_s1ap_id, not a change in the id
    h_rc = hashtable_ts_remove (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context->mme_ue_s1ap_id,  (void **)&ue_context);
    h_rc = hashtable_ts_insert (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)mme_ue_s1ap_id, (void *)ue_context);

    if (HASH_TABLE_OK != h_rc) {
      OAILOG_ERROR (LOG_MME_APP,
          "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " %s\n",
          ue_context, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id, hashtable_rc_code2string(h_rc));
    }
    ue_context->mme_ue_s1ap_id = mme_ue_s1ap_id;

    if (INVALID_IMSI64 != imsi) {
      h_rc = hashtable_ts_remove (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)ue_context->imsi, (void **)&id);
      h_rc = hashtable_ts_insert (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)imsi, (void *)(uintptr_t)mme_ue_s1ap_id);
      if (HASH_TABLE_OK != h_rc) {
        OAILOG_ERROR (LOG_MME_APP,
            "Error could not update this ue context %p enb_ue_s1ap_ue_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " imsi " IMSI_64_FMT ": %s\n",
            ue_context, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id, imsi, hashtable_rc_code2string(h_rc));
      }
        ue_context->imsi = imsi;
      }
      /** S11 Key. */
      h_rc = hashtable_ts_remove (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context->mme_teid_s11, (void **)&id);
      h_rc = hashtable_ts_insert (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)mme_teid_s11, (void *)(uintptr_t)mme_ue_s1ap_id);
      if (HASH_TABLE_OK != h_rc) {
        OAILOG_TRACE (LOG_MME_APP,
            "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " mme_teid_s11 " TEID_FMT " : %s\n",
            ue_context, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id, mme_teid_s11, hashtable_rc_code2string(h_rc));
      }
      ue_context->mme_teid_s11= mme_teid_s11;

      /** S10 Key. */
      h_rc = hashtable_ts_remove (mme_ue_context_p->tun10_ue_context_htbl, (const hash_key_t)ue_context->local_mme_teid_s10, (void **)&id);
      h_rc = hashtable_ts_insert (mme_ue_context_p->tun10_ue_context_htbl, (const hash_key_t)local_mme_teid_s10, (void *)(uintptr_t)mme_ue_s1ap_id);
      if (HASH_TABLE_OK != h_rc) {
        OAILOG_TRACE (LOG_MME_APP,
            "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " local_mme_teid_s10 " TEID_FMT " : %s\n",
            ue_context, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id, local_mme_teid_s10, hashtable_rc_code2string(h_rc));
      }
      ue_context->local_mme_teid_s10 = local_mme_teid_s10;


      if (guti_p)
      {
        h_rc = obj_hashtable_ts_remove (mme_ue_context_p->guti_ue_context_htbl, (const void *const)&ue_context->guti, sizeof (ue_context->guti), (void **)&id);
        h_rc = obj_hashtable_ts_insert (mme_ue_context_p->guti_ue_context_htbl, (const void *const)guti_p, sizeof (*guti_p), (void *)(uintptr_t)mme_ue_s1ap_id);
        if (HASH_TABLE_OK != h_rc) {
          OAILOG_TRACE (LOG_MME_APP, "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " guti " GUTI_FMT " %s\n",
              ue_context, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id, GUTI_ARG(guti_p), hashtable_rc_code2string(h_rc));
        }
        ue_context->guti = *guti_p;
      }
  }

  if ((ue_context->imsi != imsi)
      || (ue_context->mme_ue_s1ap_id != mme_ue_s1ap_id)) {
    h_rc = hashtable_ts_remove (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)ue_context->imsi, (void **)&id);
    if (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id) {
      h_rc = hashtable_ts_insert (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)imsi, (void *)(uintptr_t)mme_ue_s1ap_id);
    } else {
      h_rc = HASH_TABLE_KEY_NOT_EXISTS;
    }
    if (HASH_TABLE_OK != h_rc) {
      OAILOG_TRACE (LOG_MME_APP,
          "Error could not update this ue context %p enb_ue_s1ap_ue_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " imsi " IMSI_64_FMT ": %s\n",
          ue_context, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id, imsi, hashtable_rc_code2string(h_rc));
    }
    ue_context->imsi = imsi;
  }

  /** S11. */
  if ((ue_context->mme_teid_s11 != mme_teid_s11)
      || (ue_context->mme_ue_s1ap_id != mme_ue_s1ap_id)) {
    h_rc = hashtable_ts_remove (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context->mme_teid_s11, (void **)&id);
    if (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id && INVALID_TEID != mme_teid_s11) {
      h_rc = hashtable_ts_insert (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)mme_teid_s11, (void *)(uintptr_t)mme_ue_s1ap_id);
    } else {
      h_rc = HASH_TABLE_KEY_NOT_EXISTS;
    }

    if (HASH_TABLE_OK != h_rc && INVALID_TEID != mme_teid_s11) {
      OAILOG_TRACE (LOG_MME_APP,
          "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " mme_s11_teid " TEID_FMT " : %s\n",
          ue_context, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id, mme_teid_s11, hashtable_rc_code2string(h_rc));
        }
    ue_context->mme_teid_s11 = mme_teid_s11;
  }

  /** S10. */
  if ((ue_context->local_mme_teid_s10 != local_mme_teid_s10)
      || (ue_context->mme_ue_s1ap_id != mme_ue_s1ap_id)) {
    h_rc = hashtable_ts_remove (mme_ue_context_p->tun10_ue_context_htbl, (const hash_key_t)ue_context->local_mme_teid_s10, (void **)&id);
    if (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id && INVALID_TEID != local_mme_teid_s10) {
      h_rc = hashtable_ts_insert (mme_ue_context_p->tun10_ue_context_htbl, (const hash_key_t)local_mme_teid_s10, (void *)(uintptr_t)mme_ue_s1ap_id);
    } else {
      h_rc = HASH_TABLE_KEY_NOT_EXISTS;
    }

    if (HASH_TABLE_OK != h_rc && INVALID_TEID != local_mme_teid_s10) {
      OAILOG_TRACE (LOG_MME_APP,
          "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " mme_s11_teid " TEID_FMT " : %s\n",
          ue_context, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id, local_mme_teid_s10, hashtable_rc_code2string(h_rc));
        }
    ue_context->local_mme_teid_s10 = local_mme_teid_s10;
  }

  // todo: check if GUTI is necesary in MME_APP context
  if (guti_p) {
    if ((guti_p->gummei.mme_code != ue_context->guti.gummei.mme_code)
        || (guti_p->gummei.mme_gid != ue_context->guti.gummei.mme_gid)
        || (guti_p->m_tmsi != ue_context->guti.m_tmsi)
        || (guti_p->gummei.plmn.mcc_digit1 != ue_context->guti.gummei.plmn.mcc_digit1)
        || (guti_p->gummei.plmn.mcc_digit2 != ue_context->guti.gummei.plmn.mcc_digit2)
        || (guti_p->gummei.plmn.mcc_digit3 != ue_context->guti.gummei.plmn.mcc_digit3)
        || (ue_context->mme_ue_s1ap_id != mme_ue_s1ap_id)) {

        // may check guti_p with a kind of instanceof()?
        h_rc = obj_hashtable_ts_remove (mme_ue_context_p->guti_ue_context_htbl, &ue_context->guti, sizeof (*guti_p), (void **)&id);
        if (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id) {
          h_rc = obj_hashtable_ts_insert (mme_ue_context_p->guti_ue_context_htbl, (const void *const)guti_p, sizeof (*guti_p), (void *)(uintptr_t)mme_ue_s1ap_id);
        } else {
          h_rc = HASH_TABLE_KEY_NOT_EXISTS;
        }

        if (HASH_TABLE_OK != h_rc) {
          OAILOG_TRACE (LOG_MME_APP, "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " guti " GUTI_FMT " %s\n",
              ue_context, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id, GUTI_ARG(guti_p), hashtable_rc_code2string(h_rc));
        }
        ue_context->guti = *guti_p;
    }
  }
  OAILOG_FUNC_OUT(LOG_MME_APP);
}


//------------------------------------------------------------------------------
void mme_ue_context_dump_coll_keys(void)
{
  bstring tmp = bfromcstr(" ");
  btrunc(tmp, 0);

  hashtable_uint64_ts_dump_content (mme_app_desc.mme_ue_contexts.imsi_ue_context_htbl, tmp);
  OAILOG_TRACE (LOG_MME_APP,"imsi_ue_context_htbl %s\n", bdata(tmp));

  btrunc(tmp, 0);
  hashtable_uint64_ts_dump_content (mme_app_desc.mme_ue_contexts.tun11_ue_context_htbl, tmp);
  OAILOG_TRACE (LOG_MME_APP,"tun11_ue_context_htbl %s\n", bdata(tmp));

  btrunc(tmp, 0);
  hashtable_uint64_ts_dump_content (mme_app_desc.mme_ue_contexts.tun10_ue_context_htbl, tmp);
  OAILOG_TRACE (LOG_MME_APP,"tun10_ue_context_htbl %s\n", bdata(tmp));

  btrunc(tmp, 0);
  hashtable_uint64_ts_dump_content (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, tmp);
  OAILOG_TRACE (LOG_MME_APP,"mme_ue_s1ap_id_ue_context_htbl %s\n", bdata(tmp));

  btrunc(tmp, 0);
  hashtable_ts_dump_content (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl, tmp);
  OAILOG_TRACE (LOG_MME_APP,"enb_ue_s1ap_id_ue_context_htbl %s\n", bdata(tmp));

  btrunc(tmp, 0);
  obj_hashtable_uint64_ts_dump_content (mme_app_desc.mme_ue_contexts.guti_ue_context_htbl, tmp);
  OAILOG_TRACE (LOG_MME_APP,"guti_ue_context_htbl %s", bdata(tmp));
}

// todo: check the locks here
//------------------------------------------------------------------------------
int
mme_insert_ue_context (
  mme_ue_context_t * const mme_ue_context_p,
  const struct ue_context_s *const ue_context)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;

    OAILOG_FUNC_IN (LOG_MME_APP);
    DevAssert (mme_ue_context_p );
    DevAssert (ue_context );


    // filled ENB UE S1AP ID
    h_rc = hashtable_ts_is_key_exists (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context->enb_s1ap_id_key);
    if (HASH_TABLE_OK == h_rc) {
      OAILOG_DEBUG (LOG_MME_APP, "This ue context %p already exists enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT "\n",
          ue_context, ue_context->enb_ue_s1ap_id);
      OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
    }
    h_rc = hashtable_ts_insert (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl,
                               (const hash_key_t)ue_context->enb_s1ap_id_key,
                                (void *)((uintptr_t)ue_context->mme_ue_s1ap_id));

    if (HASH_TABLE_OK != h_rc) {
      OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ue_id 0x%x\n",
          ue_context, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id);
      OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
    }

    if (INVALID_MME_UE_S1AP_ID != ue_context->mme_ue_s1ap_id) {
      h_rc = hashtable_ts_is_key_exists (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context->mme_ue_s1ap_id);

      if (HASH_TABLE_OK == h_rc) {
        OAILOG_DEBUG (LOG_MME_APP, "This ue context %p already exists mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
            ue_context, ue_context->mme_ue_s1ap_id);
        OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
      }

      h_rc = hashtable_ts_insert (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl,
                                  (const hash_key_t)ue_context->mme_ue_s1ap_id,
                                  (void *)ue_context);

      if (HASH_TABLE_OK != h_rc) {
        OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
            ue_context, ue_context->mme_ue_s1ap_id);
        OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
      }

      // filled IMSI
      if (ue_context->imsi) {
        h_rc = hashtable_ts_insert (mme_ue_context_p->imsi_ue_context_htbl,
                                    (const hash_key_t)ue_context->imsi,
                                    (void *)((uintptr_t)ue_context->mme_ue_s1ap_id));

        if (HASH_TABLE_OK != h_rc) {
          OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " imsi %" SCNu64 "\n",
              ue_context, ue_context->mme_ue_s1ap_id, ue_context->imsi);
          OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
        }
      }

      // filled S11 tun id
      if (ue_context->mme_teid_s11) {
        h_rc = hashtable_ts_insert (mme_ue_context_p->tun11_ue_context_htbl,
                                   (const hash_key_t)ue_context->mme_teid_s11,
                                   (void *)((uintptr_t)ue_context->mme_ue_s1ap_id));

        if (HASH_TABLE_OK != h_rc) {
          OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " mme_teid_s11 " TEID_FMT "\n",
              ue_context, ue_context->mme_ue_s1ap_id, ue_context->mme_teid_s11);
          OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
        }
      }

      // filled S10 tun id
      if (ue_context->local_mme_teid_s10) {
        h_rc = hashtable_ts_insert (mme_ue_context_p->tun10_ue_context_htbl,
            (const hash_key_t)ue_context->local_mme_teid_s10,
            (void *)((uintptr_t)ue_context->mme_ue_s1ap_id));

        if (HASH_TABLE_OK != h_rc) {
          OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " local_mme_teid_s10 " TEID_FMT "\n",
              ue_context, ue_context->mme_ue_s1ap_id, ue_context->local_mme_teid_s10);
          OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
        }
      }

      // filled guti
      if ((0 != ue_context->guti.gummei.mme_code) || (0 != ue_context->guti.gummei.mme_gid) || (0 != ue_context->guti.m_tmsi) || (0 != ue_context->guti.gummei.plmn.mcc_digit1) ||     // MCC 000 does not exist in ITU table
          (0 != ue_context->guti.gummei.plmn.mcc_digit2)
          || (0 != ue_context->guti.gummei.plmn.mcc_digit3)) {

        h_rc = obj_hashtable_ts_insert (mme_ue_context_p->guti_ue_context_htbl,
                                       (const void *const)&ue_context->guti,
                                       sizeof (ue_context->guti),
                                       (void *)((uintptr_t)ue_context->mme_ue_s1ap_id));

        if (HASH_TABLE_OK != h_rc) {
          OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " guti "GUTI_FMT"\n",
                  ue_context, ue_context->mme_ue_s1ap_id, GUTI_ARG(&ue_context->guti));
          OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
        }
      }
    }
  /*
   * Updating statistics
   */
  __sync_fetch_and_add (&mme_ue_context_p->nb_ue_managed, 1);
  __sync_fetch_and_add (&mme_ue_context_p->nb_ue_since_last_stat, 1);

  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}

/**
 * We don't need to notify anything.
 * 1- EMM and MME_APP are decoupled. That MME_APP is removed or not is/should be no problem for the EMM.
 * 2- The PDN session deletion process from the SAE-GW is now done always before the detach. That eases stuff.
 */
////------------------------------------------------------------------------------
//void mme_notify_ue_context_released (
//    mme_ue_context_t * const mme_ue_context_p,
//    struct ue_context_s *ue_context)
//{
//  OAILOG_FUNC_IN (LOG_MME_APP);
//  DevAssert (mme_ue_context_p );
//  DevAssert (ue_context );
//  /*
//   * Updating statistics
//   */
//  __sync_fetch_and_sub (&mme_ue_context_p->nb_ue_managed, 1);
//  __sync_fetch_and_sub (&mme_ue_context_p->nb_ue_since_last_stat, 1);
//
////  mme_app_send_nas_signalling_connection_rel_ind(ue_context->mme_ue_s1ap_id);
//
//  // TODO HERE free resources
//
//  OAILOG_FUNC_OUT (LOG_MME_APP);
//}

//------------------------------------------------------------------------------
void mme_remove_ue_context (
  mme_ue_context_t * const mme_ue_context_p,
  struct ue_context_s *ue_context)
{
  unsigned int                           *id = NULL;
  hashtable_rc_t                          hash_rc = HASH_TABLE_OK;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (mme_ue_context_p);
  DevAssert (ue_context);

  // IMSI
  if (ue_context->imsi) {
    hash_rc = hashtable_ts_remove (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)ue_context->imsi, (void **)&id);
    if (HASH_TABLE_OK != hash_rc)
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", IMSI %" SCNu64 "  not in IMSI collection",
          ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id, ue_context->imsi);
  }

  // eNB UE S1P UE ID
  hash_rc = hashtable_ts_remove (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context->enb_s1ap_id_key, (void **)&id);
  if (HASH_TABLE_OK != hash_rc)
    OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", ENB_UE_S1AP_ID not ENB_UE_S1AP_ID collection",
      ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id);

  // filled S11 tun id
  if (ue_context->mme_teid_s11) {
    hash_rc = hashtable_ts_remove (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context->mme_teid_s11, (void **)&id);
    if (HASH_TABLE_OK != hash_rc)
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", MME TEID_S11 " TEID_FMT "  not in S11 collection",
          ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id, ue_context->mme_teid_s11);
  }

  // filled S10 tun id
  if (ue_context->local_mme_teid_s10) {
    hash_rc = hashtable_ts_remove (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context->local_mme_teid_s10, (void **)&id);
    if (HASH_TABLE_OK != hash_rc)
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", LOCAL MME TEID S10 " TEID_FMT "  not in S10 collection",
          ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id, ue_context->local_mme_teid_s10);
  }

  // filled guti
  if ((ue_context->guti.gummei.mme_code) || (ue_context->guti.gummei.mme_gid) || (ue_context->guti.m_tmsi) ||
      (ue_context->guti.gummei.plmn.mcc_digit1) || (ue_context->guti.gummei.plmn.mcc_digit2) || (ue_context->guti.gummei.plmn.mcc_digit3)) { // MCC 000 does not exist in ITU table
    hash_rc = obj_hashtable_ts_remove (mme_ue_context_p->guti_ue_context_htbl, (const void *const)&ue_context->guti, sizeof (ue_context->guti), (void **)&id);
    if (HASH_TABLE_OK != hash_rc)
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", GUTI  not in GUTI collection",
          ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id);
  }

  // filled NAS UE ID/ MME UE S1AP ID
  if (INVALID_MME_UE_S1AP_ID != ue_context->mme_ue_s1ap_id) {
    hash_rc = hashtable_ts_remove (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context->mme_ue_s1ap_id, (void **)&ue_context);
    if (HASH_TABLE_OK != hash_rc)
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT ", mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " not in MME UE S1AP ID collection",
          ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id);
  }

  mme_app_ue_context_free_content(ue_context);
  // todo: unlock?
  //  unlock_ue_contexts(ue_context);

  free_wrapper ((void**) &ue_context);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_dump_protocol_configuration_options (const protocol_configuration_options_t * const pco,
    const                 bool ms2network_direction,
    const uint8_t         indent_spaces,
    bstring               bstr_dump)
{
  int                                     i = 0;

  if (pco) {
    bformata (bstr_dump, "        Protocol configuration options:\n");
    bformata (bstr_dump, "        Configuration protocol .......: %"PRIx8"\n", pco->configuration_protocol);
    while (i < pco->num_protocol_or_container_id) {
      switch (pco->protocol_or_container_ids[i].id) {
      case PCO_PI_LCP:
        bformata (bstr_dump, "        Protocol ID .......: LCP\n");
        break;
      case PCO_PI_PAP:
        bformata (bstr_dump, "        Protocol ID .......: PAP\n");
        break;
      case PCO_PI_CHAP:
        bformata (bstr_dump, "        Protocol ID .......: CHAP\n");
        break;
      case PCO_PI_IPCP:
        bformata (bstr_dump, "        Protocol ID .......: IPCP\n");
        break;

      default:
        if (ms2network_direction) {
          switch (pco->protocol_or_container_ids[i].id) {
          case PCO_CI_P_CSCF_IPV6_ADDRESS_REQUEST:
            bformata (bstr_dump, "        Container ID .......: P_CSCF_IPV6_ADDRESS_REQUEST\n");
            break;
          case PCO_CI_DNS_SERVER_IPV6_ADDRESS_REQUEST:
            bformata (bstr_dump, "        Container ID .......: DNS_SERVER_IPV6_ADDRESS_REQUEST\n");
            break;
          case PCO_CI_MS_SUPPORT_OF_NETWORK_REQUESTED_BEARER_CONTROL_INDICATOR:
            bformata (bstr_dump, "        Container ID .......: MS_SUPPORT_OF_NETWORK_REQUESTED_BEARER_CONTROL_INDICATOR\n");
            break;
          case PCO_CI_DSMIPV6_HOME_AGENT_ADDRESS_REQUEST:
            bformata (bstr_dump, "        Container ID .......: DSMIPV6_HOME_AGENT_ADDRESS_REQUEST\n");
            break;
          case PCO_CI_DSMIPV6_HOME_NETWORK_PREFIX_REQUEST:
            bformata (bstr_dump, "        Container ID .......: DSMIPV6_HOME_NETWORK_PREFIX_REQUEST\n");
            break;
          case PCO_CI_DSMIPV6_IPV4_HOME_AGENT_ADDRESS_REQUEST:
            bformata (bstr_dump, "        Container ID .......: DSMIPV6_IPV4_HOME_AGENT_ADDRESS_REQUEST\n");
            break;
          case PCO_CI_IP_ADDRESS_ALLOCATION_VIA_NAS_SIGNALLING:
            bformata (bstr_dump, "        Container ID .......: IP_ADDRESS_ALLOCATION_VIA_NAS_SIGNALLING\n");
            break;
          case PCO_CI_IPV4_ADDRESS_ALLOCATION_VIA_DHCPV4:
            bformata (bstr_dump, "        Container ID .......: IPV4_ADDRESS_ALLOCATION_VIA_DHCPV4\n");
            break;
          case PCO_CI_P_CSCF_IPV4_ADDRESS_REQUEST:
            bformata (bstr_dump, "        Container ID .......: P_CSCF_IPV4_ADDRESS_REQUEST\n");
            break;
          case PCO_CI_DNS_SERVER_IPV4_ADDRESS_REQUEST:
            bformata (bstr_dump, "        Container ID .......: DNS_SERVER_IPV4_ADDRESS_REQUEST\n");
            break;
          case PCO_CI_MSISDN_REQUEST:
            bformata (bstr_dump, "        Container ID .......: MSISDN_REQUEST\n");
            break;
          case PCO_CI_IFOM_SUPPORT_REQUEST:
            bformata (bstr_dump, "        Container ID .......: IFOM_SUPPORT_REQUEST\n");
            break;
          case PCO_CI_IPV4_LINK_MTU_REQUEST:
            bformata (bstr_dump, "        Container ID .......: IPV4_LINK_MTU_REQUEST\n");
            break;
          case PCO_CI_IM_CN_SUBSYSTEM_SIGNALING_FLAG:
            bformata (bstr_dump, "        Container ID .......: IM_CN_SUBSYSTEM_SIGNALING_FLAG\n");
            break;
          default:
            bformata (bstr_dump, "       Unhandled container id %u length %d\n", pco->protocol_or_container_ids[i].id, blength(pco->protocol_or_container_ids[i].contents));
          }
        }  else {
          switch (pco->protocol_or_container_ids[i].id) {
          case PCO_CI_P_CSCF_IPV6_ADDRESS:
            bformata (bstr_dump, "        Container ID .......: P_CSCF_IPV6_ADDRESS\n");
            break;
          case PCO_CI_DNS_SERVER_IPV6_ADDRESS:
            bformata (bstr_dump, "        Container ID .......: DNS_SERVER_IPV6_ADDRESS\n");
            break;
          case PCO_CI_POLICY_CONTROL_REJECTION_CODE:
            bformata (bstr_dump, "        Container ID .......: POLICY_CONTROL_REJECTION_CODE\n");
            break;
          case PCO_CI_SELECTED_BEARER_CONTROL_MODE:
            bformata (bstr_dump, "        Container ID .......: SELECTED_BEARER_CONTROL_MODE\n");
            break;
          case PCO_CI_DSMIPV6_HOME_AGENT_ADDRESS:
            bformata (bstr_dump, "        Container ID .......: DSMIPV6_HOME_AGENT_ADDRESS\n");
            break;
          case PCO_CI_DSMIPV6_HOME_NETWORK_PREFIX:
            bformata (bstr_dump, "        Container ID .......: DSMIPV6_HOME_NETWORK_PREFIX\n");
            break;
          case PCO_CI_DSMIPV6_IPV4_HOME_AGENT_ADDRESS:
            bformata (bstr_dump, "        Container ID .......: DSMIPV6_IPV4_HOME_AGENT_ADDRESS\n");
            break;
          case PCO_CI_P_CSCF_IPV4_ADDRESS:
            bformata (bstr_dump, "        Container ID .......: P_CSCF_IPV4_ADDRESS\n");
            break;
          case PCO_CI_DNS_SERVER_IPV4_ADDRESS:
            bformata (bstr_dump, "        Container ID .......: DNS_SERVER_IPV4_ADDRESS\n");
            break;
          case PCO_CI_MSISDN:
            bformata (bstr_dump, "        Container ID .......: MSISDN\n");
            break;
          case PCO_CI_IFOM_SUPPORT:
            bformata (bstr_dump, "        Container ID .......: IFOM_SUPPORT\n");
            break;
          case PCO_CI_IPV4_LINK_MTU:
            bformata (bstr_dump, "        Container ID .......: IPV4_LINK_MTU\n");
            break;
          case PCO_CI_IM_CN_SUBSYSTEM_SIGNALING_FLAG:
            bformata (bstr_dump, "        Container ID .......: IM_CN_SUBSYSTEM_SIGNALING_FLAG\n");
            break;
          default:
            bformata (bstr_dump, "       Unhandled container id %u length %d\n", pco->protocol_or_container_ids[i].id, blength(pco->protocol_or_container_ids[i].contents));
          }
        }
      }
      OAILOG_STREAM_HEX(OAILOG_LEVEL_DEBUG, LOG_MME_APP, "        Hex data: ", bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents))
      i++;
    }
  }
}

//------------------------------------------------------------------------------
void mme_app_dump_bearer_context (const bearer_context_t * const bc, uint8_t indent_spaces, bstring bstr_dump)
{
  bformata (bstr_dump, "%*s - Bearer id .......: %02u\n", indent_spaces, " ", bc->ebi);
  bformata (bstr_dump, "%*s - Transaction ID ..: %x\n", indent_spaces, " ", bc->transaction_identifier);
  if (bc->s_gw_fteid_s1u.ipv4) {
    char ipv4[INET_ADDRSTRLEN];
    inet_ntop (AF_INET, (void*)&bc->s_gw_fteid_s1u.ipv4_address.s_addr, ipv4, INET_ADDRSTRLEN);
    bformata (bstr_dump, "%*s - S-GW S1-U IPv4 Address...: [%s]\n", indent_spaces, " ", ipv4);
  } else if (bc->s_gw_fteid_s1u.ipv6) {
    char                                    ipv6[INET6_ADDRSTRLEN];
    inet_ntop (AF_INET6, &bc->s_gw_fteid_s1u.ipv6_address, ipv6, INET6_ADDRSTRLEN);
    bformata (bstr_dump, "%*s - S-GW S1-U IPv6 Address...: [%s]\n", indent_spaces, " ", ipv6);
  }
  bformata (bstr_dump, "%*s - S-GW TEID (UP)...: " TEID_FMT "\n", indent_spaces, " ", bc->s_gw_fteid_s1u.teid);
  if (bc->p_gw_fteid_s5_s8_up.ipv4) {
    char ipv4[INET_ADDRSTRLEN];
    inet_ntop (AF_INET, (void*)&bc->p_gw_fteid_s5_s8_up.ipv4_address.s_addr, ipv4, INET_ADDRSTRLEN);
    bformata (bstr_dump, "%*s - P-GW S5-S8 IPv4..: [%s]\n", ipv4);
  } else if (bc->p_gw_fteid_s5_s8_up.ipv6) {
    char                                    ipv6[INET6_ADDRSTRLEN];
    inet_ntop (AF_INET6, &bc->p_gw_fteid_s5_s8_up.ipv6_address, ipv6, INET6_ADDRSTRLEN);
    bformata (bstr_dump, "%*s - P-GW S5-S8 IPv6..: [%s]\n", indent_spaces, " ", ipv6);
  }
  bformata (bstr_dump, "%*s - P-GW TEID S5-S8..: " TEID_FMT "\n", indent_spaces, " ", bc->p_gw_fteid_s5_s8_up.teid);
  bformata (bstr_dump, "%*s - QCI .............: %u\n", indent_spaces, " ", bc->qci);
  bformata (bstr_dump, "%*s - Priority level ..: %u\n", indent_spaces, " ", bc->priority_level);
  bformata (bstr_dump, "%*s - Pre-emp vul .....: %s\n", indent_spaces, " ", (bc->preemption_vulnerability == PRE_EMPTION_VULNERABILITY_ENABLED) ? "ENABLED" : "DISABLED");
  bformata (bstr_dump, "%*s - Pre-emp cap .....: %s\n", indent_spaces, " ", (bc->preemption_capability == PRE_EMPTION_CAPABILITY_ENABLED) ? "ENABLED" : "DISABLED");
  bformata (bstr_dump, "%*s - GBR UL ..........: %010" PRIu64 "\n", indent_spaces, " ", bc->esm_ebr_context.gbr_ul);
  bformata (bstr_dump, "%*s - GBR DL ..........: %010" PRIu64 "\n", indent_spaces, " ", bc->esm_ebr_context.gbr_dl);
  bformata (bstr_dump, "%*s - MBR UL ..........: %010" PRIu64 "\n", indent_spaces, " ", bc->esm_ebr_context.mbr_ul);
  bformata (bstr_dump, "%*s - MBR DL ..........: %010" PRIu64 "\n", indent_spaces, " ", bc->esm_ebr_context.mbr_dl);
  bstring bstate = bearer_state2string(bc->bearer_state);
  bformata (bstr_dump, "%*s - State ...........: %s\n", indent_spaces, " ", bdata(bstate));
  bdestroy_wrapper(&bstate);
  bformata (bstr_dump, "%*s - "ANSI_COLOR_BOLD_ON"NAS ESM bearer private data .:\n", indent_spaces, " ");
  bformata (bstr_dump, "%*s -     ESM State .......: %s\n", indent_spaces, " ", esm_ebr_state2string(bc->esm_ebr_context.status));
  bformata (bstr_dump, "%*s -     Timer id ........: %lx\n", indent_spaces, " ", bc->esm_ebr_context.timer.id);
  bformata (bstr_dump, "%*s -     Timer TO(seconds): %ld\n", indent_spaces, " ", bc->esm_ebr_context.timer.sec);
//  bformata (bstr_dump, "%*s -     Timer arg .......: %p\n", indent_spaces, " ", bc->esm_ebr_context.timer.args);
//  if (bc->esm_ebr_context.args) {
//    bformata (bstr_dump, "%*s -     Ctx ...........: %p\n", indent_spaces, " ", bc->esm_ebr_context.args->ctx);
//    bformata (bstr_dump, "%*s -     mme_ue_s1ap_id : " MME_UE_S1AP_ID_FMT "\n", indent_spaces, " ", bc->esm_ebr_context.args->ue_id);
//    bformata (bstr_dump, "%*s -     ebi          ..: %u\n", indent_spaces, " ", bc->esm_ebr_context.args->ebi);
//    bformata (bstr_dump, "%*s -     RTx counter ...: %u\n", indent_spaces, " ", bc->esm_ebr_context.args->count);
//    bformata (bstr_dump, "%*s -     RTx ESM msg ...: \n", indent_spaces, " ");
//    OAILOG_STREAM_HEX(OAILOG_LEVEL_DEBUG, LOG_MME_APP, NULL, bdata(bc->esm_ebr_context.args->msg), blength(bc->esm_ebr_context.args->msg));
//  }
  bformata (bstr_dump, "%*s - PDN id ..........: %u\n", indent_spaces, " ", bc->pdn_cx_id);
}

//------------------------------------------------------------------------------
void mme_app_dump_pdn_context (const struct ue_context_s *const ue_context,
    const pdn_context_t * const pdn_context,
    const pdn_cid_t       pdn_cid,
    const uint8_t         indent_spaces,
    bstring bstr_dump)
{
  if (pdn_context) {
    bformata (bstr_dump, "%*s - PDN ID %u:\n", indent_spaces, " ", pdn_cid);
    bformata (bstr_dump, "%*s - Context Identifier .: %x\n", indent_spaces, " ", pdn_context->context_identifier);
    bformata (bstr_dump, "%*s - Is active          .: %s\n", indent_spaces, " ", (pdn_context->is_active) ? "yes":"no");
    bformata (bstr_dump, "%*s - APN in use .........: %s\n", indent_spaces, " ", bdata(pdn_context->apn_in_use));
    bformata (bstr_dump, "%*s - APN subscribed......: %s\n", indent_spaces, " ", bdata(pdn_context->apn_subscribed));
    bformata (bstr_dump, "%*s - APN OI replacement .: %s\n", indent_spaces, " ", bdata(pdn_context->apn_oi_replacement));

    bformata (bstr_dump, "%*s - PDN type ...........: %s\n", indent_spaces, " ", PDN_TYPE_TO_STRING (pdn_context->paa->pdn_type));
    if (pdn_context->paa->pdn_type == IPv4) {
      char ipv4[INET_ADDRSTRLEN];
      inet_ntop (AF_INET, (void*)&pdn_context->paa->ipv4_address, ipv4, INET_ADDRSTRLEN);
      bformata (bstr_dump, "%*s - PAA (IPv4)..........: %s\n", indent_spaces, " ", ipv4);
    } else {
      char                                    ipv6[INET6_ADDRSTRLEN];
      inet_ntop (AF_INET6, &pdn_context->paa->ipv6_address, ipv6, INET6_ADDRSTRLEN);
      bformata (bstr_dump, "%*s - PAA (IPv6)..........: %s\n", indent_spaces, " ", ipv6);
    }
    if (pdn_context->p_gw_address_s5_s8_cp.pdn_type == IPv4) {
      char ipv4[INET_ADDRSTRLEN];
      inet_ntop (AF_INET, (void*)&pdn_context->p_gw_address_s5_s8_cp.address.ipv4_address, ipv4, INET_ADDRSTRLEN);
      bformata (bstr_dump, "%*s - P-GW s5 s8 cp (IPv4): %s\n", indent_spaces, " ", ipv4);
    } else {
      char                                    ipv6[INET6_ADDRSTRLEN];
      inet_ntop (AF_INET6, &pdn_context->p_gw_address_s5_s8_cp.address.ipv6_address, ipv6, INET6_ADDRSTRLEN);
      bformata (bstr_dump, "%*s - P-GW s5 s8 cp (IPv6): %s\n", indent_spaces, " ", ipv6);
    }
    bformata (bstr_dump, "%*s - P-GW TEID s5 s8 cp .: " TEID_FMT "\n", indent_spaces, " ", pdn_context->p_gw_teid_s5_s8_cp);
    if (pdn_context->s_gw_address_s11_s4.pdn_type == IPv4) {
      char ipv4[INET_ADDRSTRLEN];
      inet_ntop (AF_INET, (void*)&pdn_context->s_gw_address_s11_s4.address.ipv4_address, ipv4, INET_ADDRSTRLEN);
      bformata (bstr_dump, "%*s - S-GW s11_s4 (IPv4) .: %s\n", indent_spaces, " ", ipv4);
    } else {
      char                                    ipv6[INET6_ADDRSTRLEN];
      inet_ntop (AF_INET6, &pdn_context->s_gw_address_s11_s4.address.ipv6_address, ipv6, INET6_ADDRSTRLEN);
      bformata (bstr_dump, "%*s - S-GW s11_s4 (IPv6) .: %s\n", indent_spaces, " ", indent_spaces, " ", ipv6);
    }
    bformata (bstr_dump, "%*s - S-GW TEID s5 s8 cp .: " TEID_FMT "\n", indent_spaces, " ", pdn_context->s_gw_teid_s11_s4);

    bformata (bstr_dump, "%*s - Default bearer eps subscribed qos profile:\n", indent_spaces, " ");
    bformata (bstr_dump, "%*s     - QCI ......................: %u\n", indent_spaces, " ", pdn_context->default_bearer_eps_subscribed_qos_profile.qci);
    bformata (bstr_dump, "%*s     - Priority level ...........: %u\n", indent_spaces, " ", pdn_context->default_bearer_eps_subscribed_qos_profile.allocation_retention_priority.priority_level);
    bformata (bstr_dump, "%*s     - Pre-emp vulnerabil .......: %s\n", indent_spaces, " ", (pdn_context->default_bearer_eps_subscribed_qos_profile.allocation_retention_priority.pre_emp_vulnerability == PRE_EMPTION_VULNERABILITY_ENABLED) ? "ENABLED" : "DISABLED");
    bformata (bstr_dump, "%*s     - Pre-emp capability .......: %s\n", indent_spaces, " ", (pdn_context->default_bearer_eps_subscribed_qos_profile.allocation_retention_priority.pre_emp_capability == PRE_EMPTION_CAPABILITY_ENABLED) ? "ENABLED" : "DISABLED");
    bformata (bstr_dump, "%*s     - APN-AMBR (bits/s) DL .....: %010" PRIu64 "\n", indent_spaces, " ", pdn_context->subscribed_apn_ambr.br_dl);
    bformata (bstr_dump, "%*s     - APN-AMBR (bits/s) UL .....: %010" PRIu64 "\n", indent_spaces, " ", pdn_context->subscribed_apn_ambr.br_ul);
    bformata (bstr_dump, "%*s     - P-GW-APN-AMBR (bits/s) DL : %010" PRIu64 "\n", indent_spaces, " ", pdn_context->p_gw_apn_ambr.br_dl);
    bformata (bstr_dump, "%*s     - P-GW-APN-AMBR (bits/s) UL : %010" PRIu64 "\n", indent_spaces, " ", pdn_context->p_gw_apn_ambr.br_ul);
    bformata (bstr_dump, "%*s     - Default EBI ..............: %u\n", indent_spaces, " ", pdn_context->default_ebi);
    bformata (bstr_dump, "%*s - NAS ESM private data:\n");
    bformata (bstr_dump, "%*s     - Procedure transaction ID .: %x\n", indent_spaces, " ", pdn_context->esm_data.pti);
    bformata (bstr_dump, "%*s     -  Is emergency .............: %s\n", indent_spaces, " ", (pdn_context->esm_data.is_emergency) ? "yes":"no");
    bformata (bstr_dump, "%*s     -  APN AMBR .................: %u\n", indent_spaces, " ", pdn_context->esm_data.ambr);
    bformata (bstr_dump, "%*s     -  Addr realloc allowed......: %s\n", indent_spaces, " ", (pdn_context->esm_data.addr_realloc) ? "yes":"no");
    bformata (bstr_dump, "%*s     -  Num allocated EPS bearers.: %d\n", indent_spaces, " ", pdn_context->esm_data.n_bearers);
    bformata (bstr_dump, "%*s - Bearer List:\n");

    /** Set all bearers of the EBI to valid. */
    bearer_context_t * bc_to_dump = NULL;
    RB_FOREACH (bc_to_dump, SessionBearers, &pdn_context->session_bearers) {
      AssertFatal(bc_to_dump, "Mismatch in configuration bearer context NULL\n");
      bformata (bstr_dump, "%*s - Bearer item ----------------------------\n");
      mme_app_dump_bearer_context(bc_to_dump, indent_spaces + 4, bstr_dump);
    }
  }
}

//------------------------------------------------------------------------------
bool
mme_app_dump_ue_context (
  const hash_key_t keyP,
  void *const ue_context_pP,
  void *unused_param_pP,
  void** unused_result_pP)
//------------------------------------------------------------------------------
{
  struct ue_context_s           *const ue_context = (struct ue_context_s *)ue_context_pP;
  uint8_t                                 j = 0;
  if (ue_context) {
    bstring                                 bstr_dump = bfromcstralloc(4096, "\n-----------------------UE MM context ");
    bformata (bstr_dump, "%p --------------------\n", ue_context);
    bformata (bstr_dump, "    - Authenticated ..: %s\n", (ue_context->imsi_auth == IMSI_UNAUTHENTICATED) ? "FALSE" : "TRUE");
    bformata (bstr_dump, "    - eNB UE s1ap ID .: %08x\n", ue_context->enb_ue_s1ap_id);
    bformata (bstr_dump, "    - MME UE s1ap ID .: %08x\n", ue_context->mme_ue_s1ap_id);
    bformata (bstr_dump, "    - MME S11 TEID ...: " TEID_FMT "\n", ue_context->mme_teid_s11);
    bformata (bstr_dump, "                        | mcc | mnc | cell identity |\n");
    bformata (bstr_dump, "    - E-UTRAN CGI ....: | %u%u%u | %u%u%c | %05x.%02x    |\n",
                 ue_context->e_utran_cgi.plmn.mcc_digit1,
                 ue_context->e_utran_cgi.plmn.mcc_digit2 ,
                 ue_context->e_utran_cgi.plmn.mcc_digit3,
                 ue_context->e_utran_cgi.plmn.mnc_digit1,
                 ue_context->e_utran_cgi.plmn.mnc_digit2,
                 (ue_context->e_utran_cgi.plmn.mnc_digit3 > 9) ? ' ':0x30+ue_context->e_utran_cgi.plmn.mnc_digit3,
                 ue_context->e_utran_cgi.cell_identity.enb_id, ue_context->e_utran_cgi.cell_identity.cell_id);
    /*
     * Ctime return a \n in the string
     */
    bformata (bstr_dump, "    - Last acquired ..: %s", ctime (&ue_context->cell_age));

//    emm_context_dump(&ue_context->emm_context, 4, bstr_dump);
    /*
     * Display UE info only if we know them
     */
    if (SUBSCRIPTION_KNOWN == ue_context->subscription_known) {
      // TODO bformata (bstr_dump, "    - Status .........: %s\n", (ue_context->sub_status == SS_SERVICE_GRANTED) ? "Granted" : "Barred");
#define DISPLAY_BIT_MASK_PRESENT(mASK)   \
    ((ue_context->access_restriction_data & mASK) ? 'X' : 'O')
      bformata (bstr_dump, "    (O = allowed, X = !O) |UTRAN|GERAN|GAN|HSDPA EVO|E_UTRAN|HO TO NO 3GPP|\n");
      bformata (bstr_dump, "    - Access restriction  |  %c  |  %c  | %c |    %c    |   %c   |      %c      |\n",
          DISPLAY_BIT_MASK_PRESENT (ARD_UTRAN_NOT_ALLOWED),
          DISPLAY_BIT_MASK_PRESENT (ARD_GERAN_NOT_ALLOWED),
          DISPLAY_BIT_MASK_PRESENT (ARD_GAN_NOT_ALLOWED), DISPLAY_BIT_MASK_PRESENT (ARD_I_HSDPA_EVO_NOT_ALLOWED), DISPLAY_BIT_MASK_PRESENT (ARD_E_UTRAN_NOT_ALLOWED), DISPLAY_BIT_MASK_PRESENT (ARD_HO_TO_NON_3GPP_NOT_ALLOWED));
      // TODO bformata (bstr_dump, "    - Access Mode ....: %s\n", ACCESS_MODE_TO_STRING (ue_context->access_mode));
      // TODO MSISDN
      //bformata (bstr_dump, "    - MSISDN .........: %s\n", (ue_context->msisdn) ? ue_context->msisdn->data:"None");
      bformata (bstr_dump, "    - RAU/TAU timer ..: %u\n", ue_context->rau_tau_timer);
      // TODO IMEISV
      //if (IS_EMM_CTXT_PRESENT_IMEISV(&ue_context->nas_emm_context)) {
      //  bformata (bstr_dump, "    - IMEISV .........: %*s\n", IMEISV_DIGITS_MAX, ue_context->nas_emm_context._imeisv);
      //}
      bformata (bstr_dump, "    - AMBR (bits/s)     ( Downlink |  Uplink  )\n");
      // TODO bformata (bstr_dump, "        Subscribed ...: (%010" PRIu64 "|%010" PRIu64 ")\n", ue_context->subscribed_ambr.br_dl, ue_context->subscribed_ambr.br_ul);
      bformata (bstr_dump, "        Allocated ....: (%010" PRIu64 "|%010" PRIu64 ")\n", ue_context->used_ambr.br_dl, ue_context->used_ambr.br_ul);

      bformata (bstr_dump, "    - APN config list:\n");

      for (j = 0; j < ue_context->apn_config_profile.nb_apns; j++) {
        struct apn_configuration_s             *apn_config_p;

        apn_config_p = &ue_context->apn_config_profile.apn_configuration[j];
        /*
         * Default APN ?
         */
        bformata (bstr_dump, "        - Default APN ...: %s\n", (apn_config_p->context_identifier == ue_context->apn_config_profile.context_identifier)
                     ? "TRUE" : "FALSE");
        bformata (bstr_dump, "        - APN ...........: %s\n", apn_config_p->service_selection);
        bformata (bstr_dump, "        - AMBR (bits/s) ( Downlink |  Uplink  )\n");
        bformata (bstr_dump, "                        (%010" PRIu64 "|%010" PRIu64 ")\n", apn_config_p->ambr.br_dl, apn_config_p->ambr.br_ul);
        bformata (bstr_dump, "        - PDN type ......: %s\n", PDN_TYPE_TO_STRING (apn_config_p->pdn_type));
        bformata (bstr_dump, "        - QOS\n");
        bformata (bstr_dump, "            QCI .........: %u\n", apn_config_p->subscribed_qos.qci);
        bformata (bstr_dump, "            Prio level ..: %u\n", apn_config_p->subscribed_qos.allocation_retention_priority.priority_level);
        bformata (bstr_dump, "            Pre-emp vul .: %s\n", (apn_config_p->subscribed_qos.allocation_retention_priority.pre_emp_vulnerability == PRE_EMPTION_VULNERABILITY_ENABLED) ? "ENABLED" : "DISABLED");
        bformata (bstr_dump, "            Pre-emp cap .: %s\n", (apn_config_p->subscribed_qos.allocation_retention_priority.pre_emp_capability == PRE_EMPTION_CAPABILITY_ENABLED) ? "ENABLED" : "DISABLED");

        if (apn_config_p->nb_ip_address == 0) {
          bformata (bstr_dump, "            IP addr .....: Dynamic allocation\n");
        } else {
          int                                     i;

          bformata (bstr_dump, "            IP addresses :\n");

          for (i = 0; i < apn_config_p->nb_ip_address; i++) {
            if (apn_config_p->ip_address[i].pdn_type == IPv4) {
              char ipv4[INET_ADDRSTRLEN];
              inet_ntop (AF_INET, (void*)&apn_config_p->ip_address[i].address.ipv4_address, ipv4, INET_ADDRSTRLEN);
              bformata (bstr_dump, "                           [%s]\n", ipv4);
            } else {
              char                                    ipv6[INET6_ADDRSTRLEN];
              inet_ntop (AF_INET6, &apn_config_p->ip_address[i].address.ipv6_address, ipv6, INET6_ADDRSTRLEN);
              bformata (bstr_dump, "                           [%s]\n", ipv6);
            }
          }
        }
        bformata (bstr_dump, "\n");
      }
      bformata (bstr_dump, "    - PDNs:\n");

      /** No matter if it is a INTRA or INTER MME handover, continue with the MBR for other PDNs. */
      pdn_context_t * pdn_context_to_dump = NULL;
      RB_FOREACH (pdn_context_to_dump, PdnContexts, &ue_context->pdn_contexts) {
        AssertFatal(pdn_context_to_dump, "Mismatch in configuration bearer context NULL\n");
        mme_app_dump_pdn_context(ue_context, pdn_context_to_dump, pdn_context_to_dump->context_identifier, 8, bstr_dump);
      }


    }
    bcatcstr (bstr_dump, "---------------------------------------------------------\n");
    OAILOG_DEBUG(LOG_MME_APP, "%s\n", bdata(bstr_dump));
    bdestroy_wrapper(&bstr_dump);
    return false;
  }
  return true;
}


//------------------------------------------------------------------------------
void
mme_app_dump_ue_contexts (
  const mme_ue_context_t * const mme_ue_context_p)
//------------------------------------------------------------------------------
{
  hashtable_ts_apply_callback_on_elements (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, mme_app_dump_ue_context, NULL, NULL);
}

//------------------------------------------------------------------------------
static void
_mme_app_handle_s1ap_ue_context_release (const mme_ue_s1ap_id_t mme_ue_s1ap_id,
                                         const enb_ue_s1ap_id_t enb_ue_s1ap_id,
                                         uint32_t  enb_id,
                                         enum s1cause cause)
{
  struct ue_context_s                    *ue_context = NULL;
  enb_s1ap_id_key_t                       enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;
  MessageDef *message_p;

  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id);
  if (!ue_context) {
    /*
     * Use enb_ue_s1ap_id_key to get the UE context - In case MME APP could not update S1AP with valid mme_ue_s1ap_id
     * before context release is triggered from s1ap.
     */
    MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, enb_id, enb_ue_s1ap_id);
    ue_context = mme_ue_context_exists_enb_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, enb_s1ap_id_key);

    OAILOG_WARNING (LOG_MME_APP, "Invalid mme_ue_s1ap_ue_id "
      MME_UE_S1AP_ID_FMT " received from S1AP. Using enb_s1ap_id_key %ld to get the context \n", mme_ue_s1ap_id, enb_s1ap_id_key);
  }
  if ((!ue_context) || (INVALID_MME_UE_S1AP_ID == ue_context->mme_ue_s1ap_id)) {
    NOT_REQUIREMENT_3GPP_36_413(R10_8_3_3_2__2);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "0 S1AP_UE_CONTEXT_RELEASE_REQ Unknown mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "  enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ",
        s1ap_ue_context_release_req->mme_ue_s1ap_id, s1ap_ue_context_release_req->enb_ue_s1ap_id);
    OAILOG_ERROR (LOG_MME_APP, " UE Context Release Req: UE context doesn't exist for enb_ue_s1ap_ue_id "
        ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id, mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  // Set the UE context release cause in UE context. This is used while constructing UE Context Release Command
  ue_context->s1_ue_context_release_cause = cause;

  if (ue_context->ecm_state == ECM_IDLE) {
    // This case could happen during sctp reset, before the UE could move to ECM_CONNECTED
    // calling below function to set the enb_s1ap_id_key to invalid
    if (ue_context->s1_ue_context_release_cause == S1AP_SCTP_SHUTDOWN_OR_RESET) {
      mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);
      mme_app_itti_ue_context_release(ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id, ue_context->s1_ue_context_release_cause, enb_id);
      OAILOG_WARNING (LOG_MME_APP, "UE Context Release Reqeust:Cause SCTP RESET/SHUTDOWN. UE state: IDLE. mme_ue_s1ap_id = %d, enb_ue_s1ap_id = %d Action -- Handle the message\n ",
          ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id);
    }
    OAILOG_ERROR (LOG_MME_APP, "ERROR: UE Context Release Request: UE state : IDLE. enb_ue_s1ap_ue_id "
    ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " Action--- Ignore the message\n", ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id);
//    unlock_ue_contexts(ue_context);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  // Stop Initial context setup process guard timer,if running
  if (ue_context->initial_context_setup_rsp_timer.id != MME_APP_TIMER_INACTIVE_ID) {
    if (timer_remove(ue_context->initial_context_setup_rsp_timer.id, NULL)) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to stop Initial Context Setup Rsp timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
    }
    ue_context->initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
    // Setting UE context release cause as Initial context setup failure
    ue_context->s1_ue_context_release_cause = S1AP_INITIAL_CONTEXT_SETUP_FAILED;
  }
  if (ue_context->mm_state == UE_UNREGISTERED) {
    OAILOG_INFO(LOG_MME_APP, "UE is in UNREGISTERED state. Releasing the UE context in the eNB and triggering an MME context removal for "MME_UE_S1AP_ID_FMT ". \n.", ue_context->mme_ue_s1ap_id);

    /*
     * As before, in the UE_UNREGISTERED state, we assume either that we have no EMM context at all, a DEREGISTERED EMM context context, or one, where a common or specific procedure is running.
     * The EMM context will either be left in the inactive DEREGISTERED state or even if a procedure is running. It won't be touched.
     * The fate of the NAS context will be decided by the procedures.
     *
     * Bearers either are not created (CSR) or established (MBR) at all.
     * We don't need to send an explicit bearer removal request here.
     *
     * In UNREGISTERED state there should be no reason to send RELEASE_ACCESS_BEARER message to the SAE-GW, because notin established.
     *
     * If there area bearers established without an EMM context, the UE_CTX_RELEASE_COMPLETE will trigger the removal of the MME_APP UE context.
     * During removal of MME_APP UE context, if no EMM context is there, then probably HO without TAU has happened.
     * Only in that case, this should trigger Delete Session Request (only case where MME_APP does send it). Else it should only be send by EMM/NAS layer.
     * todo: or handover_completion_timer should take care of that (that TAU_Complete has arrived).
     *
     * So in any case, just release the context. With release complete we should clean the MME_APP UE context.
     *
     * No need to send NAS implicit detach.
     */
    mme_app_itti_ue_context_release(mme_ue_s1ap_id, enb_ue_s1ap_id, ue_context->s1_ue_context_release_cause, enb_id);

  } else {
    /*
     * In the UE_REGISTERED case, we should always have established bearers.
     * No need to check pending S1U downlink bearers.
     * Also in the case of intra-MME handover with a EMM-REGISTERED UE, we will not receive a UE-Context-Release Request, we will send the release command.
     * No more pending S1U DL FTEID fields necessary, we will automatically remove them once with UE-Ctx-Release-Complete.
     * The bearer contexts of the PDN session will directly be updated when eNodeB sends S1U Dl FTEIDs (can be before MBR is sent to the SAE-GW).
     */
    // release S1-U tunnel mapping in S_GW for all the active bearers for the UE
    // /** Check if this is the main S1AP UE reference, if o
    // todo:   Assert(at_least_1_BEARER_IS_ESTABLISHED_TO_SAEGW)!?!?
     OAILOG_INFO(LOG_MME_APP, "UE is REGISTERED. Sending Release Access Bearer Request for ueId "MME_UE_S1AP_ID_FMT". \n.", ue_context->mme_ue_s1ap_id);
    mme_app_send_s11_release_access_bearers_req (ue_context); /**< Release Access bearers and then send context release request.  */
    // todo: not sending all together, send one by one when release access bearer response is received.
  }
//  unlock_ue_contexts(ue_context);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}


/*
   From GPP TS 23.401 version 10.13.0 Release 10, section 5.3.5 S1 release procedure, point 6:
   The eNodeB confirms the S1 Release by returning an S1 UE Context Release Complete message to the MME.
   With this, the signalling connection between the MME and the eNodeB for that UE is released. This step shall be
   performed promptly after step 4, e.g. it shall not be delayed in situations where the UE does not acknowledge the
   RRC Connection Release.
   The MME deletes any eNodeB related information ("eNodeB Address in Use for S1-MME" and "eNB UE S1AP
   ID") from the UE's MME context, but, retains the rest of the UE's MME context including the S-GW's S1-U
   configuration information (address and TEIDs). All non-GBR EPS bearers established for the UE are preserved
   in the MME and in the Serving GW.
   If the cause of S1 release is because of User I inactivity, Inter-RAT Redirection, the MME shall preserve the
   GBR bearers. If the cause of S1 release is because of CS Fallback triggered, further details about bearer handling
   are described in TS 23.272 [58]. Otherwise, e.g. Radio Connection With UE Lost, the MME shall trigger the
   MME Initiated Dedicated Bearer Deactivation procedure (clause 5.4.4.2) for the GBR bearer(s) of the UE after
   the S1 Release procedure is completed.
*/
//------------------------------------------------------------------------------
void
mme_app_handle_s1ap_ue_context_release_complete (
  const itti_s1ap_ue_context_release_complete_t const
  *s1ap_ue_context_release_complete)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p = NULL;

  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, s1ap_ue_context_release_complete->mme_ue_s1ap_id);

  if (!ue_context) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "0 S1AP_UE_CONTEXT_RELEASE_COMPLETE Unknown mme_ue_s1ap_id 0x%06" PRIX32 " ", s1ap_ue_context_release_complete->mme_ue_s1ap_id);
    OAILOG_WARNING (LOG_MME_APP, "UE context doesn't exist for enb_ue_s1ap_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
        s1ap_ue_context_release_complete->enb_ue_s1ap_id, s1ap_ue_context_release_complete->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /*
   * Check if there is a handover procedure ongoing.
   */
  mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
  if(s10_handover_proc){
    if(s10_handover_proc->proc.type == MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER){
      /** Check if the CLR flag is received. If so delete the UE Context. */
      if (s10_handover_proc->pending_clear_location_request){
        /** Set the ECM state to IDLE. */
        mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);

        OAILOG_INFO (LOG_MME_APP, "Implicitly detaching the UE due CLR flag @ completion of MME_MOBILITY timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
        message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
        DevAssert (message_p != NULL);
        message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->mme_ue_s1ap_id; /**< Rest won't be sent, so no NAS Detach Request will be sent. */
        MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
        itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);

        OAILOG_FUNC_OUT (LOG_MME_APP);
      }else{
        /** No CLR flag received yet. Release the UE reference. */
        OAILOG_DEBUG(LOG_MME_APP, "Received UE context release complete for the main ue_reference of the UE with mme_ue_s1ap_id "MME_UE_S1AP_ID_FMT" and enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT". \n",
              s1ap_ue_context_release_complete->mme_ue_s1ap_id, s1ap_ue_context_release_complete->enb_ue_s1ap_id);
        /** Not releasing any bearer information. */
        /* Update keys and ECM state. */
        mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);
        OAILOG_FUNC_OUT (LOG_MME_APP);
      }
    }else{
      if(ue_context->s1_ue_context_release_cause == S1AP_HANDOVER_CANCELLED){
        /** Don't change the signaling connection state. */
        // todo: this case might also be a too fast handover back handover failure! don't handle it here, handle it before and immediately Deregister the UE
        mme_app_send_s1ap_handover_cancel_acknowledge(ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id, ue_context->sctp_assoc_id_key);
        OAILOG_DEBUG(LOG_MME_APP, "Successfully terminated the resources in the target eNB %d for UE with mme_ue_s1ap_ue_id "MME_UE_S1AP_ID_FMT " (REGISTERED). "
            "Sending HO-CANCELLATION-ACK back to the source eNB. \n", s1ap_ue_context_release_complete->enb_id, ue_context->mme_ue_s1ap_id, ue_context->mm_state);
        ue_context->s1_ue_context_release_cause = S1AP_INVALID_CAUSE;
        /** Not releasing the main connection. Removing the handover procedure. */
        mme_app_delete_s10_procedure_mme_handover(ue_context);

        OAILOG_FUNC_OUT (LOG_MME_APP);
      }else{
        /*
         * Assuming normal completion of S10 Intra-MME handover.
         * Delete the S10 Handover Procedure.
         */
        // todo: review this
        /* Update keys and ECM state. */
//        mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);

        mme_app_delete_s10_procedure_mme_handover(ue_context);
        /** Re-Establish the UE-Reference as the main reference. */
        if(ue_context->enb_ue_s1ap_id != s1ap_ue_context_release_complete->enb_ue_s1ap_id){
          OAILOG_DEBUG(LOG_MME_APP, "Establishing the SCTP current UE_REFERENCE enb_ue_s1ap_ue_id " ENB_UE_S1AP_ID_FMT ". ", s1ap_ue_context_release_complete->enb_ue_s1ap_id);
          /**
           * This will overwrite the association towards the old eNB if single MME S1AP handover.
           * The old eNB will be referenced by the enb_ue_s1ap_id.
           */
          notify_s1ap_new_ue_mme_s1ap_id_association (ue_context->sctp_assoc_id_key, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id);
        }
        OAILOG_FUNC_OUT (LOG_MME_APP);
      }
    }
  }
  /* No handover procedure ongoing or no CLR is received yet from the HSs. Assume that this is the main connection and release it. */
  OAILOG_DEBUG(LOG_MME_APP, "No Handover procedure ongoing. Received UE context release complete for the main ue_reference of the UE with mme_ue_s1ap_id "MME_UE_S1AP_ID_FMT" and enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT". \n",
        s1ap_ue_context_release_complete->mme_ue_s1ap_id, s1ap_ue_context_release_complete->enb_ue_s1ap_id);
  /** Set the bearers into IDLE mode. */
  mme_app_ue_context_s1_release_enb_informations(ue_context);
  /* Update keys and ECM state. */
  mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);


  /* ****************************************************************************************************************************************** *
   *      BELOW EVERYTHING EXCEPT update_ue_sig_connection will be THROWN OUT! NAS won't and should not be affected by this message!
   *      All the session deletion procedures for all PDN sessions should be triggered by NAS only. S1AP signaling radio errors should be treated locally.
   *      No need to inform NAS about every single S1AP signaling error. If Periodic TAU or so does not work in time or the mobile reachability timer runs off,
   *      UE will be implicitly detached anyway.
   *
   *      Dedicated bearer deactivation will not be triggered by this message. All bearers should be handled samewise for S1AP signaling conditions.
   *      The maximum would be that it is removed @ UE_RELEASE_REQ from S1AP --> changing the UE_sig connection state!
   *      CBReq timer --> put it also into NAS and don't make it dependent on the idle mode request by the eNB.
   *      It should handle the message somehow but managed by NAS and appropriate timers.
   *      // todo:
   * ****************************************************************************************************************************************** */

  // todo: deleting the create bearer procedures (this also seems wrong, we should delete the procedures in a more centralized way).
  //  mme_app_delete_s11_procedure_create_bearer(ue_context);
  // todo: should treat all dedicated bearers equally and not make any exceptions.
//  if (((S1ap_Cause_PR_radioNetwork == ue_context->s1_ue_context_release_cause.present) &&
//       (S1ap_CauseRadioNetwork_radio_connection_with_ue_lost == ue_context->s1_ue_context_release_cause.choice.radioNetwork))) {
////    /*
////     * MME initiated dedicated bearer deactivation only be sent in case of congestion (Bearer Resource Command or Delete Bearer Command).
////     * Sending delete session request for each session should also remove all dedicated bearers of that PDN session.
////     */
////    // initiate deactivation procedure
////    for (pdn_cid_t i = 0; i < MAX_APN_PER_UE; i++) {
////      if (ue_context->pdn_contexts[i]) {
////        mme_app_trigger_mme_initiated_dedicated_bearer_deactivation_procedure (ue_context, i);
////      }
////    }
//  }

  if (ue_context->mm_state == UE_UNREGISTERED) {
    OAILOG_DEBUG (LOG_MME_APP, "Deleting UE context associated in MME for mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n ",
        s1ap_ue_context_release_complete->mme_ue_s1ap_id);
    /**
     * We will not inform NAS. Assuming that it is handled.
     * Since no hanThis will not remove the EMM context. MME_APP does not remove the EMM context. The detach procedure is already activated.
     * This removed the MME_APP context only.
     * We assume that all sessions are already released. We don't trigger session removal via S1AP/MME_APP, but only via ESM.
     */

    /** The S10 tunnel will be removed together with the S10 related process. */
    mme_remove_ue_context(&mme_app_desc.mme_ue_contexts, ue_context);
    // todo
    ////      update_mme_app_stats_connected_ue_sub();
    ////      // return here avoid unlock_ue_contexts() // todo: don't we need to unlock it after retrieving an MME_APP ?
    ////      OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  // TODO remove in context GBR bearers
  //  unlock_ue_contexts(ue_context);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

// todo: locking the UE context?
//-------------------------------------------------------------------------------------------------------
void mme_ue_context_update_ue_sig_connection_state (
  mme_ue_context_t * const mme_ue_context_p,
  struct ue_context_s *ue_context,
  ecm_state_t new_ecm_state)
{
  // Function is used to update UE's Signaling Connection State
  hashtable_rc_t                          hash_rc = HASH_TABLE_OK;
  unsigned int                           *id = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (mme_ue_context_p);
  DevAssert (ue_context);
  if (new_ecm_state == ECM_IDLE)
  {
    hash_rc = hashtable_ts_remove (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context->enb_s1ap_id_key, (void **)&id);
    if (HASH_TABLE_OK != hash_rc)
    {
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id_key %ld mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", ENB_UE_S1AP_ID_KEY could not be found",
                                  ue_context->enb_s1ap_id_key, ue_context->mme_ue_s1ap_id);
    }
    ue_context->enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;

    OAILOG_DEBUG (LOG_MME_APP, "MME_APP: UE Connection State changed to IDLE. mme_ue_s1ap_id = %d\n", ue_context->mme_ue_s1ap_id);

    if (mme_config.nas_config.t3412_min > 0) {
      // Start Mobile reachability timer only if periodic TAU timer is not disabled
      if (timer_setup (ue_context->mobile_reachability_timer.sec, 0, TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *)&(ue_context->mme_ue_s1ap_id), &(ue_context->mobile_reachability_timer.id)) < 0) {
        OAILOG_ERROR (LOG_MME_APP, "Failed to start Mobile Reachability timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
        ue_context->mobile_reachability_timer.id = MME_APP_TIMER_INACTIVE_ID;
      } else {
        OAILOG_DEBUG (LOG_MME_APP, "Started Mobile Reachability timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
      }
    }
    if (ue_context->ecm_state == ECM_CONNECTED) {
      ue_context->ecm_state       = ECM_IDLE;
      // Update Stats
      update_mme_app_stats_connected_ue_sub();
    }

  }else if ((ue_context->ecm_state == ECM_IDLE) && (new_ecm_state == ECM_CONNECTED))
  {
    ue_context->ecm_state = ECM_CONNECTED;

    OAILOG_DEBUG (LOG_MME_APP, "MME_APP: UE Connection State changed to CONNECTED.enb_ue_s1ap_id = %d, mme_ue_s1ap_id = %d\n", ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id);

    // Stop Mobile reachability timer,if running
    if (ue_context->mobile_reachability_timer.id != MME_APP_TIMER_INACTIVE_ID)
    {
      if (timer_remove(ue_context->mobile_reachability_timer.id, NULL)) {

        OAILOG_ERROR (LOG_MME_APP, "Failed to stop Mobile Reachability timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
      }
      ue_context->mobile_reachability_timer.id = MME_APP_TIMER_INACTIVE_ID;
    }
    // Stop Implicit detach timer,if running
    if (ue_context->implicit_detach_timer.id != MME_APP_TIMER_INACTIVE_ID)
    {
      if (timer_remove(ue_context->implicit_detach_timer.id, NULL)) {
        OAILOG_ERROR (LOG_MME_APP, "Failed to stop Implicit Detach timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
      }
      ue_context->implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;
    }
    // Update Stats
    update_mme_app_stats_connected_ue_add();
  }
  return;
}

//-------------------------------------------------------------------------------------------------------
void mme_ue_context_update_ue_emm_state (
  mme_ue_s1ap_id_t       mme_ue_s1ap_id, mm_state_t  new_mm_state)
{
  // Function is used to update UE's mobility management State- Registered/Un-Registered

  /** Only checks for state changes. */
  struct ue_context_s                    *ue_context = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id);
  if (ue_context == NULL) {
    OAILOG_CRITICAL (LOG_MME_APP, "**** Abnormal - UE context is null.****\n");
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  if (ue_context->mm_state == UE_UNREGISTERED && (new_mm_state == UE_REGISTERED))
  {
    ue_context->mm_state = new_mm_state;

    // Update Stats
    update_mme_app_stats_attached_ue_add();

  } else if ((ue_context->mm_state == UE_REGISTERED) && (new_mm_state == UE_UNREGISTERED))
  {
    ue_context->mm_state = new_mm_state;

    // Update Stats
    update_mme_app_stats_attached_ue_sub();
  }else{
    OAILOG_CRITICAL(LOG_MME_APP, "**** Abnormal - No handler for state transition of UE with mme_ue_s1ap_ue_id "MME_UE_S1AP_ID_FMT " "
        "entering %d state from %d state. ****\n", ue_context->mm_state, new_mm_state);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  // todo: transition to/from UE_HANDOVER state!
  OAILOG_FUNC_OUT (LOG_MME_APP);
}


// todo: replace this method by a better one! search a bearer context inside a pdn context (used) or unused inside an UE context
////------------------------------------------------------------------------------
//bearer_context_t*
//mme_app_is_bearer_context_in_list (
//  const mme_ue_s1ap_id_t mme_ue_s1ap_id, const ebi_t ebi)
//{
//  ue_context_t                           *ue_context = NULL;
//  mme_ue_s1ap_id_t                       *mme_ue_s1ap_id_p = (mme_ue_s1ap_id_t*)&mme_ue_s1ap_id;
//  bearer_context_t                       *bearer_context_p = NULL;
//
//  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id);
//  DevAssert(ue_context);
//
//  hashtable_ts_get ((hash_table_ts_t * const)&ue_context->bearer_ctxs, (const hash_key_t)ebi, (void **)&bearer_context_p);
//
//  OAILOG_TRACE(LOG_MME_APP, "Return bearer_context %p \n", bearer_context_p);
//  return bearer_context_p;
//}


//------------------------------------------------------------------------------
void
mme_app_handle_s1ap_ue_context_release_req (
  const itti_s1ap_ue_context_release_req_t const *s1ap_ue_context_release_req)

{
  _mme_app_handle_s1ap_ue_context_release(s1ap_ue_context_release_req->mme_ue_s1ap_id,
                                          s1ap_ue_context_release_req->enb_ue_s1ap_id,
                                          s1ap_ue_context_release_req->enb_id,
                                          S1AP_RADIO_EUTRAN_GENERATED_REASON);
}

//------------------------------------------------------------------------------
void
mme_app_handle_enb_deregister_ind(const itti_s1ap_eNB_deregistered_ind_t const * eNB_deregistered_ind) {
  for (int i = 0; i < eNB_deregistered_ind->nb_ue_to_deregister; i++) {
    _mme_app_handle_s1ap_ue_context_release(eNB_deregistered_ind->mme_ue_s1ap_id[i],
                                            eNB_deregistered_ind->enb_ue_s1ap_id[i],
                                            eNB_deregistered_ind->enb_id,
                                            S1AP_SCTP_SHUTDOWN_OR_RESET);
  }
}

//------------------------------------------------------------------------------
void
mme_app_handle_enb_reset_req (const itti_s1ap_enb_initiated_reset_req_t const * enb_reset_req)
{

  MessageDef *message_p;
  OAILOG_DEBUG (LOG_MME_APP, " eNB Reset request received. eNB id = %d, reset_type  %d \n ", enb_reset_req->enb_id, enb_reset_req->s1ap_reset_type);
  DevAssert (enb_reset_req->ue_to_reset_list != NULL);
  if (enb_reset_req->s1ap_reset_type == RESET_ALL) {
  // Full Reset. Trigger UE Context release release for all the connected UEs.
    for (int i = 0; i < enb_reset_req->num_ue; i++) {
      _mme_app_handle_s1ap_ue_context_release(*(enb_reset_req->ue_to_reset_list[i].mme_ue_s1ap_id),
                                            *(enb_reset_req->ue_to_reset_list[i].enb_ue_s1ap_id),
                                            enb_reset_req->enb_id,
                                            S1AP_SCTP_SHUTDOWN_OR_RESET);
    }

  } else { // Partial Reset
    for (int i = 0; i < enb_reset_req->num_ue; i++) {
      if (enb_reset_req->ue_to_reset_list[i].mme_ue_s1ap_id == NULL &&
                          enb_reset_req->ue_to_reset_list[i].enb_ue_s1ap_id == NULL)
        continue;
      else
        _mme_app_handle_s1ap_ue_context_release(*(enb_reset_req->ue_to_reset_list[i].mme_ue_s1ap_id),
                                            *(enb_reset_req->ue_to_reset_list[i].enb_ue_s1ap_id),
                                            enb_reset_req->enb_id,
                                            S1AP_SCTP_SHUTDOWN_OR_RESET);
    }

  }
  // Send Reset Ack to S1AP module

  message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_ENB_INITIATED_RESET_ACK);
  DevAssert (message_p != NULL);
  memset ((void *)&message_p->ittiMsg.s1ap_enb_initiated_reset_ack, 0, sizeof (itti_s1ap_enb_initiated_reset_ack_t));
  S1AP_ENB_INITIATED_RESET_ACK (message_p).s1ap_reset_type = enb_reset_req->s1ap_reset_type;
  S1AP_ENB_INITIATED_RESET_ACK (message_p).sctp_assoc_id = enb_reset_req->sctp_assoc_id;
  S1AP_ENB_INITIATED_RESET_ACK (message_p).sctp_stream_id = enb_reset_req->sctp_stream_id;
  S1AP_ENB_INITIATED_RESET_ACK (message_p).num_ue = enb_reset_req->num_ue;
  /*
   * Send the same ue_reset_list to S1AP module to be used to construct S1AP Reset Ack message. This would be freed by
   * S1AP module.
   */

  S1AP_ENB_INITIATED_RESET_ACK (message_p).ue_to_reset_list = enb_reset_req->ue_to_reset_list;
  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_DEBUG (LOG_MME_APP, " Reset Ack sent to S1AP. eNB id = %d, reset_type  %d \n ", enb_reset_req->enb_id, enb_reset_req->s1ap_reset_type);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

/** Handle S10/Handover Related Information. */
//------------------------------------------------------------------------------
void
mme_app_handle_nas_context_req(itti_nas_context_req_t * const nas_context_req_pP){
  /**
   * Send a context request for the given UE to the specified MME with the given last visited TAI.
   */
  MessageDef            *message_p;
  struct ue_context_s   *ue_context = NULL;
  /*
   * Check that the UE does exist.
   * This should come through an initial request for attach/TAU.
   * MME_APP UE context is created and is in UE_UNREGISTERED mode.
   */
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, nas_context_req_pP->ue_id);
  if (ue_context == NULL) { /**< Always think separate of EMM_DATA context and the rest. Could mean or not mean, that no EMM_DATA exists. */
    OAILOG_ERROR(LOG_MME_APP, "An UE MME context does not exist for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " and guti: " GUTI_FMT ". \n",
        nas_context_req_pP->ue_id, GUTI_ARG(&nas_context_req_pP->old_guti));
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "An UE MME context does not exist for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " and guti: " GUTI_FMT ". \n",
        nas_context_req_pP->ue_id, GUTI_ARG(&nas_context_req_pP->old_guti));
    /** This should always clear the allocated UE context resources, if there are any. We do not clear them directly, but only inform NAS/EMM layer of what happened. */
    _mme_app_send_nas_context_response_err(nas_context_req_pP->ue_id, SYSTEM_FAILURE);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
//  /*
//   * Check that the UE is in EMM_UNREGISTERED state.
//   */
//  if(ue_context->mm_state != UE_UNREGISTERED){
//    OAILOG_ERROR(LOG_MME_APP, "UE MME context is not in UE_UNREGISTERED state but instead in %d for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " and guti: " GUTI_FMT ". \n",
//        ue_context->mm_state, nas_context_req_pP->ue_id, GUTI_ARG(&nas_context_req_pP->old_guti));
//    MSC_LOG_EVENT (MSC_MMEAPP_MME, "UE MME context is not in UE_UNREGISTERED state but instead in %d for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " and guti: " GUTI_FMT ". \n",
//        ue_context->mm_state, nas_context_req_pP->ue_id, GUTI_ARG(&nas_context_req_pP->old_guti));
//    /** This should always clear the allocated UE context resources. */
//    _mme_app_send_nas_context_response_err(nas_context_req_pP->ue_id, SYSTEM_FAILURE);
//    OAILOG_FUNC_OUT (LOG_MME_APP);
//  }
//  /** Assert that there is no s10 handover procedure running. */
  mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
  DevAssert(!s10_handover_proc);

//    /** Report back a context failure, which should remove the UE context together with any processes. */
//    OAILOG_ERROR(LOG_MME_APP, "UE MME context has already a handover procedure for ueId " MME_UE_S1AP_ID_FMT". NAS layer should not have asked this. \n", ue_context->mme_ue_s1ap_id);
//    MSC_LOG_EVENT (MSC_MMEAPP_MME, "UE MME context has already a handover procedure for ueId " MME_UE_S1AP_ID_FMT". NAS layer should not have asked this. \n", ue_context->mme_ue_s1ap_id);
//    // todo: currently, not sending the pending EPS MM context information back!
//    /** This should always clear the allocated UE context resources. */
//    _mme_app_send_nas_context_response_err(ue_context->mme_ue_s1ap_id, SYSTEM_FAILURE);
//    OAILOG_FUNC_OUT (LOG_MME_APP);
//  }

  /**
   * Store the received PDN connectivity information as pending information in the MME_APP UE context.
   * todo: if normal attach --> Create Session Request will be sent from the subscription information ULA.
   * We could send the PDN Connection IE together to the NAS, but since we have subscription information yet, we still need the
   * method mme_app_send_s11_create_session_req_from_handover_tau which sends the CREATE_SESSION_REQUEST from the pending information.
   */

  struct in_addr neigh_mme_ipv4_addr;
  neigh_mme_ipv4_addr.s_addr = 0;

  if (1) {
    // TODO prototype may change
    mme_app_select_service(&nas_context_req_pP->originating_tai, &neigh_mme_ipv4_addr);
    //    session_request_p->peer_ip.in_addr = mme_config.ipv4.
    if(neigh_mme_ipv4_addr.s_addr == 0){
      OAILOG_ERROR(LOG_MME_APP, "Could not find a neighboring MME for handling missing NAS context. \n");
      OAILOG_DEBUG (LOG_MME_APP, "The selected TAI " TAI_FMT " is not configured as an S10 MME neighbor. "
          "Not proceeding with the NAS UE context request for mme_ue_s1ap_id of UE: "MME_UE_S1AP_ID_FMT ". \n",
          TAI_ARG(&nas_context_req_pP->originating_tai), nas_context_req_pP->ue_id);
      /** Send a nas_context_reject back. */
      _mme_app_send_nas_context_response_err(nas_context_req_pP->ue_id, RELOCATION_FAILURE);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
  }

  /**
   * No temporary handover target information needed to be allocated.
   * Also the MME_APP UE context will not be changed (incl. MME_APP UE state).
   */
  message_p = itti_alloc_new_message (TASK_MME_APP, S10_CONTEXT_REQUEST);
  DevAssert (message_p != NULL);
  itti_s10_context_request_t *s10_context_request_p = &message_p->ittiMsg.s10_context_request;

  /** Set the Source MME_S10_FTEID the same as in S11. */
  teid_t local_teid = 0x0;
  do{
    local_teid = (teid_t) (rand() % 0xFFFFFFFF);
  }while(mme_ue_context_exists_s10_teid(&mme_app_desc.mme_ue_contexts, local_teid) != NULL);


  /** Always set the counterpart to 0. */
  s10_context_request_p->teid = 0;
  /** Prepare the S10 message and initialize the S10 GTPv2c tunnel endpoints. */
  // todo: search the list of neighboring MMEs for the correct origin TAI
  s10_context_request_p->peer_ip.s_addr = neigh_mme_ipv4_addr.s_addr;
  /** Set the Target MME_S10_FTEID (this MME's S10 Tunnel endpoint). */
  OAI_GCC_DIAG_OFF(pointer-to-int-cast);
  s10_context_request_p->s10_target_mme_teid.teid = local_teid;
  OAI_GCC_DIAG_ON(pointer-to-int-cast);
  s10_context_request_p->s10_target_mme_teid.interface_type = S10_MME_GTP_C;
  mme_config_read_lock (&mme_config);
  s10_context_request_p->s10_target_mme_teid.ipv4_address = mme_config.ipv4.s10;
  mme_config_unlock (&mme_config);
  s10_context_request_p->s10_target_mme_teid.ipv4 = 1;

  /** Update the MME_APP UE context with the new S10 local TEID to find it from the S10 answer. */
  mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
      ue_context->enb_s1ap_id_key,
      ue_context->mme_ue_s1ap_id,
      ue_context->imsi,
      ue_context->mme_teid_s11,       /**< Won't be changed. */
      local_teid,
      &ue_context->guti); /**< Don't register with the old GUTI. */

  /** Set the Complete Request Message. */
  s10_context_request_p->complete_request_message.request_value = nas_context_req_pP->nas_msg;
  if (!nas_context_req_pP->nas_msg) {
    OAILOG_ERROR(LOG_MME_APP, "No complete request exists for TAU of UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " and guti: " GUTI_FMT ". \n",
        nas_context_req_pP->ue_id, GUTI_ARG(&nas_context_req_pP->old_guti));
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "No complete request exists for TAU of UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " and guti: " GUTI_FMT ". \n",
        nas_context_req_pP->ue_id, GUTI_ARG(&nas_context_req_pP->old_guti));
    _mme_app_send_nas_context_response_err(nas_context_req_pP->ue_id, REQUEST_REJECTED);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
//  s10_context_request_p->complete_request_message.request_value = nas_context_req_pP->nas_msg;
  nas_context_req_pP->nas_msg = NULL;

  /** Set the RAT_TYPE. */
  s10_context_request_p->rat_type = nas_context_req_pP->rat_type;
  /** Set the GUTI. */
  memcpy((void*)&s10_context_request_p->old_guti, (void*)&nas_context_req_pP->old_guti, sizeof(guti_t));
  /** Serving Network. */
  s10_context_request_p->serving_network.mcc[0] = ue_context->e_utran_cgi.plmn.mcc_digit1;
  s10_context_request_p->serving_network.mcc[1] = ue_context->e_utran_cgi.plmn.mcc_digit2;
  s10_context_request_p->serving_network.mcc[2] = ue_context->e_utran_cgi.plmn.mcc_digit3;
  s10_context_request_p->serving_network.mnc[0] = ue_context->e_utran_cgi.plmn.mnc_digit1;
  s10_context_request_p->serving_network.mnc[1] = ue_context->e_utran_cgi.plmn.mnc_digit2;
  s10_context_request_p->serving_network.mnc[2] = ue_context->e_utran_cgi.plmn.mnc_digit3;

  /** Send the Forward Relocation Message to S11. */
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME,  MSC_S10_MME ,
      NULL, 0, "0 S10_CONTEXT_REQ for mmeUeS1apId " MME_UE_S1AP_ID_FMT ". \n", nas_ue_context_request_pP->ue_id);
  itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);

  /**
   * S10 Initial request timer will be started on the target MME side in the GTPv2c stack.
   * UE context removal & Attach/TAU reject will be be performed.
   * Also GTPv2c has a transaction started. If no response from source MME arrives, an ITTI message with SYSTEM_FAILURE cause will be returned.
   */
  OAILOG_INFO(LOG_MME_APP, "Successfully sent S10 Context Request for received NAS request for UE id  %d \n", ue_context->mme_ue_s1ap_id);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//----------------------------------------------------------------------------------------------------------
void mme_app_set_ue_eps_mm_context(mm_context_eps_t * ue_eps_mme_context_p, struct ue_context_s *ue_context, emm_data_context_t *ue_nas_ctx) {

  int                           rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);

  DevAssert(ue_context);
  DevAssert(ue_eps_mme_context_p);

  /** Add the MM_Context from the security context. */
  ue_eps_mme_context_p->ksi  = ue_nas_ctx->_security.eksi;

  /** Used NAS Integrity Algorithm. */
  ue_eps_mme_context_p->nas_int_alg  = ue_nas_ctx->_security.selected_algorithms.integrity & 0x07;
  ue_eps_mme_context_p->nas_cipher_alg  = ue_nas_ctx->_security.selected_algorithms.encryption & 0x0F;
  // nas_dl_count --> copy int to byte!
  ue_eps_mme_context_p->nas_ul_count  = ue_nas_ctx->_security.ul_count;
  ue_eps_mme_context_p->nas_dl_count  = ue_nas_ctx->_security.dl_count;
  /** Add the NCC. */
  ue_eps_mme_context_p->ncc       = ue_nas_ctx->_security.ncc;
  /** Add the K_ASME. */
  // todo: copy k_asme
  memset(ue_eps_mme_context_p->k_asme, 0, 32);
  memcpy(ue_eps_mme_context_p->k_asme, ue_nas_ctx->_vector[ue_nas_ctx->_security.vector_index].kasme, 32);
  /** Add the NH key. */
  memset(ue_eps_mme_context_p->nh, 0, 32);
  memcpy(ue_eps_mme_context_p->nh, ue_nas_ctx->_vector[ue_nas_ctx->_security.vector_index].nh_conj, 32);

  // Add the UE Network Capability.
  ue_eps_mme_context_p->ue_nc.eea = ue_nas_ctx->_ue_network_capability.eea;
  ue_eps_mme_context_p->ue_nc.eia = ue_nas_ctx->_ue_network_capability.eia; /*<< Check that they exist.*/
  ue_eps_mme_context_p->ue_nc_length = 2;
  if(ue_nas_ctx->_security.capability.umts_integrity && ue_nas_ctx->_security.capability.umts_encryption){
    OAILOG_DEBUG(LOG_MME_APP, "Adding UMTS encryption and UMTS integrity alghorithms into the forward relocation request.");
    ue_eps_mme_context_p->ue_nc_length +=2;
    ue_eps_mme_context_p->ue_nc.umts_present = true;
    ue_eps_mme_context_p->ue_nc.uia = ue_nas_ctx->_security.capability.umts_integrity;
    ue_eps_mme_context_p->ue_nc.uea = ue_nas_ctx->_security.capability.umts_encryption;
  }
  // todo: gprs/misc!?
  /** Set the length of the MS network capability to 0. */
  ue_eps_mme_context_p->ms_nc_length = 0;
  ue_eps_mme_context_p->mei_length   = 0;
  ue_eps_mme_context_p->vdp_lenth    = 0;
  // todo: access restriction
  ue_eps_mme_context_p->access_restriction_flags        = ue_context->access_restriction_data & 0xFF;

  OAILOG_INFO (LOG_MME_APP, "Setting MM UE context for UE " MME_UE_S1AP_ID_FMT " with KSI %d. \n", ue_context->mme_ue_s1ap_id, ue_eps_mme_context_p->ksi);

  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Send an S10 Context Response with error code.
 * It shall not trigger creating a local S10 tunnel.
 * Parameter is the TEID of the Source-MME.
 */
static
void _mme_app_send_s10_context_response_err(teid_t mme_source_s10_teid, struct in_addr mme_source_ipv4_address, void *trxn,  gtpv2c_cause_value_t cause_val){
  OAILOG_FUNC_IN (LOG_MME_APP);

  /** Send a Context RESPONSE with error cause. */
  MessageDef * message_p = itti_alloc_new_message (TASK_MME_APP, S10_CONTEXT_RESPONSE);
  DevAssert (message_p != NULL);

  itti_s10_context_response_t *s10_context_response_p = &message_p->ittiMsg.s10_context_response;
  memset ((void*)s10_context_response_p, 0, sizeof (itti_s10_context_response_t));
  /** Set the TEID of the source MME. */
  s10_context_response_p->teid = mme_source_s10_teid; /**< Not set into the UE context yet. */
  /** Set the IPv4 address of the source MME. */
  s10_context_response_p->peer_ip = mme_source_ipv4_address;
  s10_context_response_p->cause.cause_value = cause_val;
  s10_context_response_p->trxn  = trxn;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "MME_APP Sending S10 CONTEXT_RESPONSE_ERR");

  /** Sending a message to S10. */
  itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_s10_context_request(const itti_s10_context_request_t * const s10_context_request_pP )
{
 struct ue_context_s                    *ue_context = NULL;
 MessageDef                             *message_p = NULL;
 mme_app_s10_proc_mme_handover_t        *s10_proc_mme_handover = NULL;
 int                                     rc = RETURNok;

 DevAssert(s10_context_request_pP);
 DevAssert(s10_context_request_pP->trxn);

 OAILOG_FUNC_IN (LOG_MME_APP);
 OAILOG_DEBUG (LOG_MME_APP, "Received S10_CONTEXT_REQUEST from S10. \n");

 /** Get the GUTI and try to get the context via GUTI. */
 // todo: checking that IMSI exists.. means that UE is validated on the target side.. check 23.401 for this case..
 /** Check that the UE does not exist. */
 ue_context = mme_ue_context_exists_guti(&mme_app_desc.mme_ue_contexts, &s10_context_request_pP->old_guti);
 if (ue_context == NULL) {
   /** No UE was found. */
   OAILOG_ERROR (LOG_MME_APP, "No UE for GUTI " GUTI_FMT " was found. Cannot proceed with context request. \n", GUTI_ARG(&s10_context_request_pP->old_guti));
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S10_CONTEXT_REQUEST. No UE existing for guti: " GUTI_FMT, GUTI_ARG(&s10_context_request_pP->old_guti));
   // todo: error check
   _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, s10_context_request_pP->s10_target_mme_teid.ipv4_address, s10_context_request_pP->trxn, CONTEXT_NOT_FOUND);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 OAILOG_INFO(LOG_MME_APP, "Received a CONTEXT_REQUEST for new UE with GUTI" GUTI_FMT ". \n", GUTI_ARG(&s10_context_request_pP->old_guti));

 /** Check that NAS/EMM context is existing. */
 emm_data_context_t *ue_nas_ctx = emm_data_context_get_by_guti(&_emm_data, &s10_context_request_pP->old_guti);
 if (!ue_nas_ctx) {
   OAILOG_ERROR(LOG_MME_APP, "A NAS EMM context is not existing for this GUTI "GUTI_FMT " already exists. \n", GUTI_ARG(&s10_context_request_pP->old_guti));
   _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, s10_context_request_pP->s10_target_mme_teid.ipv4_address, s10_context_request_pP->trxn, CONTEXT_NOT_FOUND);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** Check that a valid security context exists for the MME_UE_CONTEXT. */
 if (!IS_EMM_CTXT_PRESENT_SECURITY(ue_nas_ctx)) {
   OAILOG_ERROR(LOG_MME_APP, "A NAS EMM context is present but no security context is existing for this GUTI "GUTI_FMT ". \n", GUTI_ARG(&s10_context_request_pP->old_guti));
   _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, s10_context_request_pP->s10_target_mme_teid.ipv4_address, s10_context_request_pP->trxn, CONTEXT_NOT_FOUND);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** Check that the UE is registered. Due to some errors in the RRC, it may be idle or connected. Don't know. */
 if (UE_REGISTERED != ue_context->mm_state) { /**< Should also mean EMM_REGISTERED. */
   /** UE may be in idle mode or it may be detached. */
   OAILOG_ERROR(LOG_MME_APP, "UE NAS EMM context is not in UE_REGISTERED state for GUTI "GUTI_FMT ". \n", GUTI_ARG(&s10_context_request_pP->old_guti));
   _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, s10_context_request_pP->s10_target_mme_teid.ipv4_address, s10_context_request_pP->trxn, REQUEST_REJECTED);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }

 if(ue_context->ecm_state != ECM_IDLE){
   OAILOG_WARNING(LOG_MME_APP, "UE NAS EMM context is NOT in ECM_IDLE state for GUTI "GUTI_FMT ". Continuing with processing of S10 Context Request. \n", GUTI_ARG(&s10_context_request_pP->old_guti));
 }
 /** Check if there is any S10 procedure existing. */
 s10_proc_mme_handover = mme_app_get_s10_procedure_mme_handover(ue_context);
 if(s10_proc_mme_handover){
   OAILOG_WARNING (LOG_MME_APP, "EMM context for UE with ue_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " in EMM_REGISTERED state has a running S10 procedure. "
         "Rejecting further procedures. \n", ue_context->mme_ue_s1ap_id, ue_context->imsi);
   _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, s10_context_request_pP->s10_target_mme_teid.ipv4_address, s10_context_request_pP->trxn, REQUEST_REJECTED);
   // todo: here abort the procedure! and continue with the handover
   // todo: mme_app_delete_s10_procedure_mme_handover(ue_context); /**< Should remove all pending data. */
   // todo: aborting should just clear all pending information
   // todo: additionally invalidate NAS below (if one exists)
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /*
  * Create a new handover procedure, no matter a UE context exists or not.
  * Store the transaction in it.
  * Store all pending PDN connections in it.
  * Each time a CSResp for a specific APN arrives, send another CSReq if needed.
  *
  * Remove the UE context when the timer runs out for any case (also if context request is rejected).
  */
 s10_proc_mme_handover = mme_app_create_s10_procedure_mme_handover(ue_context, false, MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER);
 DevAssert(s10_proc_mme_handover);

 /** No S10 Procedure running. */
 // todo: validate NAS message!
// rc = emm_data_context_validate_complete_nas_request(ue_nas_ctx, &s10_context_request_pP->complete_request_message);
// if(rc != RETURNok){
//   OAILOG_ERROR(LOG_MME_APP, "UE NAS message for IMSI " IMSI_64_FMT " could not be validated. \n", ue_context->imsi);
//   _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, s10_context_request_pP->s10_target_mme_teid.ipv4, s10_context_request_pP->trxn, REQUEST_REJECTED);
//   bdestroy(s10_context_request_pP->complete_request_message.request_value);
//   OAILOG_FUNC_OUT (LOG_MME_APP);
// }

 /**
  * Destroy the message finally
  * todo: check what if already destroyed.
  */
 bdestroy(s10_context_request_pP->complete_request_message.request_value);
 /** Prepare the S10 CONTEXT_RESPONSE. */
 message_p = itti_alloc_new_message (TASK_MME_APP, S10_CONTEXT_RESPONSE);
 DevAssert (message_p != NULL);
 itti_s10_context_response_t *context_response_p = &message_p->ittiMsg.s10_context_response;

 /** Set the target S10 TEID. */
 context_response_p->teid    = s10_context_request_pP->s10_target_mme_teid.teid; /**< Only a single target-MME TEID can exist at a time. */
 context_response_p->peer_ip = s10_context_request_pP->s10_target_mme_teid.ipv4_address; /**< todo: Check this is correct. */
 context_response_p->trxn    = s10_context_request_pP->trxn;
 /** Set the cause. Since the UE context state has not been changed yet, nothing to do in the context if success or failure.*/
 context_response_p->cause.cause_value = REQUEST_ACCEPTED; // todo: check the NAS message here!

 if(context_response_p->cause.cause_value == REQUEST_ACCEPTED){
   /** Set the Source MME_S10_FTEID the same as in S11. */
   OAI_GCC_DIAG_OFF(pointer-to-int-cast);
   context_response_p->s10_source_mme_teid.teid = (teid_t) ue_context; /**< This one also sets the context pointer. */
   OAI_GCC_DIAG_ON(pointer-to-int-cast);
   context_response_p->s10_source_mme_teid.interface_type = S10_MME_GTP_C;
   mme_config_read_lock (&mme_config);
   context_response_p->s10_source_mme_teid.ipv4_address = mme_config.ipv4.s10;
   mme_config_unlock (&mme_config);
   context_response_p->s10_source_mme_teid.ipv4 = 1;

   /**
    * Update the local_s10_key.
    * Not setting the key directly in the  ue_context structure. Only over this function!
    */
   mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
       ue_context->enb_s1ap_id_key,
       ue_context->mme_ue_s1ap_id,
       ue_context->imsi,
       ue_context->mme_teid_s11,       // mme_s11_teid is new
       context_response_p->s10_source_mme_teid.teid,       // set with forward_relocation_request // s10_context_response!
       &ue_context->guti);

   pdn_context_t * first_pdn = RB_MIN(PdnContexts, &ue_context->pdn_contexts);
   DevAssert(first_pdn);

   /** Set the S11 Source SAEGW FTEID. */
   OAI_GCC_DIAG_OFF(pointer-to-int-cast);
   context_response_p->s11_sgw_teid.teid = first_pdn->s_gw_teid_s11_s4;
   OAI_GCC_DIAG_ON(pointer-to-int-cast);
   context_response_p->s11_sgw_teid.interface_type = S11_MME_GTP_C;
   mme_config_read_lock (&mme_config);
   context_response_p->s11_sgw_teid.ipv4_address = first_pdn->s_gw_address_s11_s4.address.ipv4_address;
   mme_config_unlock (&mme_config);
   context_response_p->s11_sgw_teid.ipv4 = 1;

   /** IMSI. */
   memcpy((void*)&context_response_p->imsi, &ue_nas_ctx->_imsi, sizeof(imsi_t));

   /** Set the PDN_CONNECTION IE. */
   context_response_p->pdn_connections = calloc(1, sizeof(mme_ue_eps_pdn_connections_t));
   mme_app_set_pdn_connections(context_response_p->pdn_connections, ue_context);

   /** Set the MM_UE_EPS_CONTEXT. */
   context_response_p->ue_eps_mm_context = calloc(1, sizeof(mm_context_eps_t));
   mme_app_set_ue_eps_mm_context(context_response_p->ue_eps_mm_context, ue_context, ue_nas_ctx);

   /*
    * Start timer to wait the handover/TAU procedure to complete.
    * A Clear_Location_Request message received from the HSS will cause the resources to be removed.
    * Resources will not be removed if that is not received. --> TS.23.401 defines for SGSN "remove after CLReq" explicitly).
    */
   mme_config_read_lock (&mme_config);
   if (timer_setup (mme_config.mme_mobility_completion_timer, 0,
       TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *) &(ue_context->mme_ue_s1ap_id), &(s10_proc_mme_handover->proc.timer.id)) < 0) {
     OAILOG_ERROR (LOG_MME_APP, "Failed to start MME mobility completion timer for UE id  %d for duration %d \n", ue_context->mme_ue_s1ap_id, mme_config.mme_mobility_completion_timer);
     s10_proc_mme_handover->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;
   } else {
     OAILOG_DEBUG (LOG_MME_APP, "MME APP : Handled S10_CONTEXT_REQUEST at source MME side and started timer for UE context removal. "
         "Activated the MME mobilty timer UE id  %d. Waiting for CANCEL_LOCATION_REQUEST from HSS.. Timer Id %u. Timer duration %d. \n",
         ue_context->mme_ue_s1ap_id, s10_proc_mme_handover->proc.timer.id, mme_config.mme_mobility_completion_timer);
     /** Upon expiration, invalidate the timer.. no flag needed. */
   }
   mme_config_unlock (&mme_config);
 }else{
   /**
    * No source-MME (local) FTEID needs to be set. No tunnel needs to be established.
    * Just respond with the error cause to the given target-MME FTEID.
    */
 }
 /** Without interacting with NAS, directly send the S10_CONTEXT_RESPONSE message. */
 OAILOG_INFO(LOG_MME_APP, "Allocated S10_CONTEXT_RESPONSE MESSAGE for UE with IMSI " IMSI_64_FMT " and mmeUeS1apId " MME_UE_S1AP_ID_FMT " with error cause %d. \n",
     ue_context->imsi, ue_context->mme_ue_s1ap_id, context_response_p->cause);

 MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME,  MSC_S11_MME ,
     NULL, 0, "0 S10_CONTEXT_RESPONSE for UE %d is sent. \n", ue_context->mme_ue_s1ap_id);
 itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
 /** Send just S10_CONTEXT_RESPONSE. Currently not waiting for the S10_CONTEXT_ACKNOWLEDGE and nothing done if it does not arrive (no timer etc.). */
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//-------------------------------------------------------------------------------------------
pdn_context_t * mme_app_handle_pdn_connectivity_from_s10(ue_context_t *ue_context, pdn_connection_t * pdn_connection){

  OAILOG_FUNC_IN (LOG_MME_APP);

  context_identifier_t    context_identifier = 0; // todo: how is this set via S10?
  pdn_cid_t               pdn_cid = 0;
  pdn_context_t          *pdn_context = NULL;
  int                     rc = RETURNok;

  /** Get and handle the PDN Connection element as pending PDN connection element (using the default_ebi and the apn). */
  mme_app_get_pdn_context(ue_context, 0, pdn_connection->linked_eps_bearer_id, pdn_connection->apn_str, &pdn_context);
  if(pdn_context){
    /* Found the PDN context. */
    OAILOG_ERROR(LOG_MME_APP, "PDN context for apn %s and default ebi %d already exists for UE_ID: " MME_UE_S1AP_ID_FMT". Skipping the establishment (or update). \n",
        pdn_connection->apn_str, pdn_connection->linked_eps_bearer_id, ue_context->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
  }
  /*
   * Create new PDN connection.
   * No context identifier will be set.
   * Later set context identifier by ULA?
   */
  /** Craete a PDN connection and later set the ESM values when NAS layer is established. */
  pdn_context = mme_app_create_pdn_context(ue_context, pdn_connection->apn_str, 0); /**< Create the pdn context using the APN network identifier. */
  if(!pdn_context) {
    OAILOG_ERROR(LOG_MME_APP, "Could not create a new pdn context for apn \" %s \" for UE_ID " MME_UE_S1AP_ID_FMT " from S10 PDN_CONNECTIONS IE. "
        "Skipping the establishment of pdn context. \n", pdn_connection->apn_str, ue_context->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
  }
  /*
   * Will update the PDN context with the PCOs received from the SAE-GW.
   * todo: No PCO elements received @ handover from source MME? MS to network PCO or the result of the first PCO?
   * We receive PCO elements at Create Session Response, register them.
   *
   * Setup the IP address allocated by the network and the PDN type directly based
   * on the received PDN information.
   */
  /* Create a PAA object. Usually not created, such that no empty IP addresses are sent with CSR of initial request. */
  pdn_context->paa = calloc(1, sizeof(paa_t));

  /* Set the default EBI. */
  pdn_context->default_ebi = pdn_connection->linked_eps_bearer_id;

  if(pdn_connection->ipv4_address.s_addr) {
//    IPV4_STR_ADDR_TO_INADDR ((const char *)pdn_connection->ipv4_address, pdn_context->paa->ipv4_address, "BAD IPv4 ADDRESS FORMAT FOR PAA!\n");
    pdn_context->paa->ipv4_address.s_addr = pdn_connection->ipv4_address.s_addr;
  }
//  if(pdn_connection->ipv6_address) {
//    //    memset (pdn_connections->pdn_connection[num_pdn].ipv6_address, 0, 16);
//    memcpy (pdn_context->paa->ipv6_address.s6_addr, pdn_connection->ipv6_address.s6_addr, 16);
//    pdn_context->paa->ipv6_prefix_length = pdn_connection->ipv6_prefix_length;
//    pdn_context->pdn_type++;
//    if(pdn_connection->ipv4_address.s_addr)
//      pdn_context->pdn_type++;
//  }
  /*
   * The IP addresses of the ITTI message will later be erased automatically in the itti_free_defined_msg.
   *
   * No UE subscription data will be sent via FRR. APN subscription will be stored.
   * They will later be updated/set with ULA at the target HSS.
   *
   * Allocate the bearer contexts and the bearer level QoS to immediately send .
   */
  pdn_context->subscribed_apn_ambr = pdn_connection->apn_ambr;

  for (int num_bearer = 0; num_bearer < pdn_connection->bearer_context_list.num_bearer_context; num_bearer++){
    bearer_context_to_be_created_t * bearer_context_to_be_created_s10 = &pdn_connection->bearer_context_list.bearer_contexts[num_bearer];
    /* Create bearer contexts in the PDN context. */
    bearer_context_t * bearer_context_registered = NULL;
    mme_app_register_bearer_context(ue_context, bearer_context_to_be_created_s10->eps_bearer_id, pdn_context, &bearer_context_registered);
    // todo: optimize this!
    DevAssert(bearer_context_registered);
    /*
     * Set the bearer level QoS parameters and update the statistics.
     */
    mme_app_desc.mme_ue_contexts.nb_bearers_managed++;
    mme_app_desc.mme_ue_contexts.nb_bearers_since_last_stat++;
    /* Received an initialized bearer context, set the QoS values from the pdn_connections IE. */
    mme_app_bearer_context_update_handover(bearer_context_registered, bearer_context_to_be_created_s10);
  }
  // todo: apn restriction data!
  OAILOG_INFO (LOG_MME_APP, "Successfully updated the MME_APP UE context with the pending pdn information for UE id  %d. \n", ue_context->mme_ue_s1ap_id);
  OAILOG_FUNC_RETURN(LOG_MME_APP, pdn_context);
}

//------------------------------------------------------------------------------
void
mme_app_handle_s10_context_response(
    itti_s10_context_response_t* const s10_context_response_pP
    )
{
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p = NULL;
  uint64_t                                imsi = 0;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (s10_context_response_pP );

  ue_context = mme_ue_context_exists_s10_teid (&mme_app_desc.mme_ue_contexts, s10_context_response_pP->teid);
  /** Check that the UE_CONTEXT exists for the S10_FTEID. */
  if (ue_context == NULL) { /**< If no UE_CONTEXT found, all tunnels are assumed to be cleared and not tunnels established when S10_CONTEXT_RESPONSE is received. */
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 S10_CONTEXT_RESPONSE local S10 TEID " TEID_FMT " ", s10_context_response_pP->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", s10_context_response_pP->teid);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /** Get the NAS layer MME relocation procedure. */
  emm_data_context_t * emm_context = emm_data_context_get(&_emm_data, ue_context->mme_ue_s1ap_id);
  /* Check if there is an MME relocation procedure running. */
  nas_ctx_req_proc_t * emm_cn_proc_ctx_req = get_nas_cn_procedure_ctx_req(emm_context);

  if(!emm_cn_proc_ctx_req){
    OAILOG_WARNING(LOG_MME_APP, "A NAS CN context request procedure is not active for UE " MME_UE_S1AP_ID_FMT " in state %d. "
        "Dropping newly received S10 Context Response. \n.",
        ue_context->mme_ue_s1ap_id, ue_context->mm_state);
    /*
     * The received message should be removed automatically.
     * todo: the message will have a new transaction, removing it not that it causes an assert?
     */
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  OAILOG_INFO(LOG_MME_APP, "NAS CN context request procedure is active. "
      "Processing the newly received context response for ueId " MME_UE_S1AP_ID_FMT". \n", ue_context->mme_ue_s1ap_id);

  /** Check the cause first, before parsing the IMSI. */
  if(s10_context_response_pP->cause.cause_value != REQUEST_ACCEPTED){
    OAILOG_ERROR(LOG_MME_APP, "Received an erroneous cause  value %d for S10 Context Request for UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT ". "
        "Rejecting attach/tau & implicit detach. \n", s10_context_response_pP->cause.cause_value, ue_context->mme_ue_s1ap_id, ue_context->mm_state);
    _mme_app_send_nas_context_response_err(ue_context->mme_ue_s1ap_id, s10_context_response_pP->cause.cause_value);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  AssertFatal ((s10_context_response_pP->imsi.length > 0)
      && (s10_context_response_pP->imsi.length < 16), "BAD IMSI LENGTH %d", s10_context_response_pP->imsi.length);
  AssertFatal ((s10_context_response_pP->imsi.length > 0)
      && (s10_context_response_pP->imsi.length < 16), "STOP ON IMSI LENGTH %d", s10_context_response_pP->imsi.length);

  /** Parse IMSI first, then get the MME_APP UE context. */
  imsi = imsi_to_imsi64(&s10_context_response_pP->imsi);
  OAILOG_DEBUG (LOG_MME_APP, "Handling S10 Context Response for imsi " IMSI_64_FMT ". \n", imsi);


  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 S10_CONTEXT_RESPONSe local S10 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      s10_context_response_pP->teid, ue_context->imsi);
//  /**
//   * Check that the UE_Context is in correct state.
//   */
//  if(ue_context->mm_state != UE_UNREGISTERED){ /**< Should be in UNREGISTERED state, else nothing to be done in the source MME side, just send a reject back and detch the UE. */
//    /** Deal with the error case. */
//    OAILOG_ERROR(LOG_MME_APP, "UE MME context with IMSI " IMSI_64_FMT " and mmeS1apUeId %d is not in UNREGISTERED state but instead %d. "
//        "Doing an implicit detach (todo: currently ignoring). \n",
//        ue_context->imsi, ue_context->mme_ue_s1ap_id, ue_context->mm_state);
//    _mme_app_send_nas_context_response_err(ue_context->mme_ue_s1ap_id, REQUEST_REJECTED);
//    OAILOG_FUNC_OUT (LOG_MME_APP);
//  }

  /**
   * The UE was successfully authenticated at the source MME. We can sent a S10 Context acknowledge back.
   */
  message_p = itti_alloc_new_message (TASK_MME_APP, S10_CONTEXT_ACKNOWLEDGE);
  DevAssert (message_p != NULL);
  itti_s10_context_acknowledge_t * s10_context_ack_p = &message_p->ittiMsg.s10_context_acknowledge;

  /** Fill up . */
  s10_context_ack_p->cause.cause_value = REQUEST_ACCEPTED; /**< Since we entered UE_REGISTERED state. */
  /** Set the transaction: Peer IP, Peer Port, Peer TEID should be deduced from this. */
  s10_context_ack_p->trxnId     = s10_context_response_pP->trxn;
  s10_context_ack_p->peer_ip    = s10_context_response_pP->s10_source_mme_teid.ipv4_address;
  s10_context_ack_p->peer_port  = 2123; // todo: s10_context_response_pP->peer_port;
  s10_context_ack_p->teid       = s10_context_response_pP->s10_source_mme_teid.teid;

  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_S10_MME, NULL, 0, "0 S10 CONTEXT_ACK for UE " MME_UE_S1AP_ID_FMT "! \n", ue_context->mme_ue_s1ap_id);
  itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
  OAILOG_INFO(LOG_MME_APP, "Sent S10 Context Acknowledge to the source MME FTEID " TEID_FMT " for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT ". \n",
      ue_context->mme_ue_s1ap_id, s10_context_ack_p->teid);

  /**
   * Update the coll_keys with the IMSI.
   */
  mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
      ue_context->enb_s1ap_id_key,
      ue_context->mme_ue_s1ap_id,
      imsi,      /**< New IMSI. */
      ue_context->mme_teid_s11,
      ue_context->local_mme_teid_s10,
      &ue_context->guti);

  ue_context->imsi_auth = IMSI_AUTHENTICATED;
  memcpy((void*)&emm_cn_proc_ctx_req->nas_s10_context._imsi, &s10_context_response_pP->imsi, sizeof(imsi_t));

  emm_cn_proc_ctx_req->nas_s10_context.mm_eps_ctx = s10_context_response_pP->ue_eps_mm_context;
  s10_context_response_pP->ue_eps_mm_context = NULL;

  /** Copy the pdn connections also into the emm_cn procedure. */
  emm_cn_proc_ctx_req->pdn_connections = s10_context_response_pP->pdn_connections;
  /** Unlink. */
  s10_context_response_pP->pdn_connections = NULL;

  /** Handle PDN Connections. */
  OAILOG_INFO(LOG_MME_APP, "For ueId " MME_UE_S1AP_ID_FMT " processing the pdn_connections (continuing with CSR). \n", ue_context->mme_ue_s1ap_id);
  /** Process PDN Connections IE. Will initiate a Create Session Request message for the pending pdn_connections. */
  pdn_connection_t * pdn_connection = &emm_cn_proc_ctx_req->pdn_connections->pdn_connection[emm_cn_proc_ctx_req->next_processed_pdn_connection];
  pdn_context_t * pdn_context = mme_app_handle_pdn_connectivity_from_s10(ue_context, pdn_connection);
  DevAssert(pdn_context);
  emm_cn_proc_ctx_req->next_processed_pdn_connection++;
  /*
   * When Create Session Response is received, continue to process the next PDN connection, until all are processed.
   * When all pdn_connections are completed, continue with handover request.
   */
  if(mme_app_send_s11_create_session_req (ue_context, &s10_context_response_pP->imsi, pdn_context, &emm_context->originating_tai) != RETURNok){
    /**
     * Error sending CSReq. Send S10 Context Response error to NAS.
     */
    OAILOG_ERROR(LOG_MME_APP, "Error sending Create Session Request for UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT " for context request procedure. "
               "Rejecting attach/tau & implicit detach. \n", s10_context_response_pP->cause, ue_context->mme_ue_s1ap_id, ue_context->mm_state);
    _mme_app_send_nas_context_response_err(ue_context->mme_ue_s1ap_id, s10_context_response_pP->cause.cause_value);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  OAILOG_INFO(LOG_MME_APP, "Successfully sent CSR for UE " MME_UE_S1AP_ID_FMT ". Waiting for CSResp to continue to process s10 context response on target MME side. \n", ue_context->mme_ue_s1ap_id);
  /*
   * Use the received PDN connectivity information to update the session/bearer information with the PDN connections IE, before informing the NAS layer about the context.
   * Not performing state change. The MME_APP UE context will stay in the same state.
   * State change will be handled by EMM layer.
   */
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_s10_context_acknowledge(
    const itti_s10_context_acknowledge_t* const s10_context_acknowledge_pP
    )
{
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p = NULL;
  uint64_t                                imsi = 0;
  int16_t                                 bearer_id =0;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (s10_context_acknowledge_pP);

  OAILOG_DEBUG (LOG_MME_APP, "Handling S10 CONTEXT ACKNOWLEDGE for TEID " TEID_FMT ". \n", s10_context_acknowledge_pP->teid);

  ue_context = mme_ue_context_exists_s10_teid (&mme_app_desc.mme_ue_contexts, s10_context_acknowledge_pP->teid);
  /** Check that the UE_CONTEXT exists for the S10_FTEID. */
  if (ue_context == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 S10_CONTEXT_ACKNOWLEDGE local S11 teid " TEID_FMT " ", s10_context_acknowledge_pP->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", s10_context_acknowledge_pP->teid);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 S10_CONTEXT_ACKNOWLEDGE local S10 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      s10_context_acknowledge_pP->teid, ue_context->imsi);
  /** Check the cause. */
  if(s10_context_acknowledge_pP->cause.cause_value != REQUEST_ACCEPTED){
    OAILOG_ERROR(LOG_MME_APP, "The S10 Context Acknowledge for local teid " TEID_FMT " was not valid/could not be received. "
        "Ignoring the handover state. \n", s10_context_acknowledge_pP->teid);
    // todo: what to do in this case? Ignoring the S6a cancel location request?
  }
  // todo : Mark the UE context as invalid
  OAILOG_FUNC_OUT (LOG_MME_APP);
}


//------------------------------------------------------------------------------
void
mme_app_handle_relocation_cancel_request(
     const itti_s10_relocation_cancel_request_t* const relocation_cancel_request_pP
    )
{
 struct ue_context_s                    *ue_context = NULL;
 MessageDef                             *message_p  = NULL;
 imsi64_t                                imsi64 = INVALID_IMSI64;
 itti_s10_relocation_cancel_response_t  *relocation_cancel_response_p = NULL;

 OAILOG_FUNC_IN (LOG_MME_APP);

 IMSI_STRING_TO_IMSI64 (&relocation_cancel_request_pP->imsi, &imsi64);
 OAILOG_DEBUG (LOG_MME_APP, "Handling S10_RELOCATION_CANCEL_REQUEST REQUEST for imsi " IMSI_64_FMT " with TEID " TEID_FMT". \n", imsi64, relocation_cancel_request_pP->teid);

 /** Check that the UE does exist. */
 ue_context = mme_ue_context_exists_s10_teid (&mme_app_desc.mme_ue_contexts, relocation_cancel_request_pP->teid); /**< Get the UE context from the local TEID. */
 if (ue_context == NULL) {
   /** Check IMSI. */
   ue_context = mme_ue_context_exists_imsi(&mme_app_desc.mme_ue_contexts, imsi64); /**< Get the UE context from the IMSI. */
 }
 /** Prepare the RELOCATION_CANCEL_RESPONSE. */
 message_p = itti_alloc_new_message (TASK_MME_APP, S10_RELOCATION_CANCEL_RESPONSE);
 DevAssert (message_p != NULL);
 relocation_cancel_response_p = &message_p->ittiMsg.s10_relocation_cancel_response;
 memset ((void*)relocation_cancel_response_p, 0, sizeof (itti_s10_relocation_cancel_response_t));
 relocation_cancel_response_p->peer_ip.s_addr = relocation_cancel_request_pP->peer_ip.s_addr;
 relocation_cancel_response_p->trxn           = relocation_cancel_request_pP->trxn;
 if(!ue_context || ue_context->imsi != imsi64){
   OAILOG_ERROR (LOG_MME_APP, "We didn't find this UE in list of UE: " IMSI_64_FMT". \n", imsi64);
   /** Send a relocation cancel response with 0 TEID if remote TEID is not set yet (although spec says otherwise). It may may not be that a transaction might be removed. */
   relocation_cancel_response_p->cause.cause_value = IMSI_IMEI_NOT_KNOWN;
   itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** Check if there is a handover process. */
 mme_app_s10_proc_mme_handover_t * s10_handover_process = mme_app_get_s10_procedure_mme_handover(ue_context);
 if(!s10_handover_process){
   OAILOG_WARNING(LOG_MME_APP, "No S10 Handover Process is ongoing for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT ". Ignoring Relocation Cancle Request (ack). \n", ue_context->mme_ue_s1ap_id);
   relocation_cancel_response_p->cause.cause_value = REQUEST_ACCEPTED;
   itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 relocation_cancel_response_p->teid = s10_handover_process->remote_mme_teid.teid; /**< Only a single target-MME TEID can exist at a time. */
 /** An EMM context may not exist yet. */
 emm_data_context_t * ue_nas_ctx = emm_data_context_get_by_imsi(&_emm_data, imsi64);
 if(ue_nas_ctx){
   OAILOG_ERROR (LOG_MME_APP, "An EMM Data Context already exists for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT ". \n", ue_context->mme_ue_s1ap_id, imsi64);
   /*
    * Since RELOCATION_CANCEL_REQUEST is already sent when S10 Handover Cancellation is performed. If we have a NAS context, it should be an invalidated one,
    * already purging itself.
    */
   if(ue_context->s1_ue_context_release_cause != S1AP_INVALIDATE_NAS){
     OAILOG_ERROR (LOG_MME_APP, "The EMM Data Context is not invalidated. We will disregard the RELOCATION_CANCEL_REQUEST for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT ". \n", ue_context->mme_ue_s1ap_id, imsi64);
     relocation_cancel_response_p->cause.cause_value = SYSTEM_FAILURE;
     itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
     OAILOG_FUNC_OUT (LOG_MME_APP);
   }
   /*
    * Continue with the cancellation procedure! Will overwrite the release cause that the UE Context is also removed.
    */
   OAILOG_WARNING (LOG_MME_APP, "The EMM Data Context is INVALIDATED. We will continue the RELOCATION_CANCEL_REQUEST for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT ". \n", ue_context->mme_ue_s1ap_id, imsi64);
 }
 /**
  * Lastly, check if the UE is already REGISTERED in the TARGET cell (here).
  * In that case, its too late to remove for HO-CANCELLATION.
  * Assuming that it is an error on the source side.
  */
 if(ue_context->mm_state == UE_REGISTERED){
   OAILOG_ERROR (LOG_MME_APP, "UE Context/EMM Data Context for IMSI " IMSI_64_FMT " and mmeUeS1apId " MME_UE_S1AP_ID_FMT " is already REGISTERED. "
       "Not purging due to RELOCATION_CANCEL_REQUEST. \n", imsi64, ue_context->mme_ue_s1ap_id);
   /*
    * This might happen only after too fast handovering the UE NAS context is not invalidated yet.
    */
   relocation_cancel_response_p->cause.cause_value = SYSTEM_FAILURE;
   itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /**
  * Perform an UE Context Release with cause Handover Cancellation.
  * Will also cancel all MME_APP timers and send a S1AP Release Command with HO-Cancellation cause.
  * First the default bearers should be removed. Then the UE context in the eNodeB.
  */
 ue_context->s1_ue_context_release_cause = S1AP_HANDOVER_CANCELLED;

 /**
  * Send a S1AP Context Release Request.
  * If no S1AP UE reference is existing, we will send a UE context release command with the MME_UE_S1AP_ID.
  * todo: macro/home
  */
 mme_app_itti_ue_context_release(ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id, ue_context->s1_ue_context_release_cause, s10_handover_process->target_id.target_id.macro_enb_id.enb_id);
 /**
  * Respond with a Relocation Cancel Response (without waiting for the S10-Triggered detach to complete).
  */
 relocation_cancel_response_p->cause.cause_value = REQUEST_ACCEPTED;
 itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
 /** Triggered an Implicit Detach Message back. */
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_relocation_cancel_response( /**< Will only be sent to cancel a handover, not a TAU. */
     const itti_s10_relocation_cancel_response_t * const relocation_cancel_response_pP
    )
{
 struct ue_context_s                    *ue_context = NULL;
 MessageDef                             *message_p  = NULL;

 OAILOG_FUNC_IN (LOG_MME_APP);
 OAILOG_DEBUG (LOG_MME_APP, "Received S10_RELOCATION_CANCEL_RESPONSE from S10. \n");

 /** Check that the UE does exist. */
 if(relocation_cancel_response_pP->teid == 0){
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 ue_context = mme_ue_context_exists_s10_teid(&mme_app_desc.mme_ue_contexts, relocation_cancel_response_pP->teid);
 if (ue_context == NULL) {
   OAILOG_ERROR(LOG_MME_APP, "An UE MME context does not exist for UE with s10 teid %d. \n", relocation_cancel_response_pP->teid);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S10_RELOCATION_CANCEL_RESPONSE. No UE existing teid %d. \n", relocation_cancel_response_pP->teid);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }

 /**
  * Check the cause of the UE.
  * If it is SYSTEM_FAILURE, perform an implicit detach.
  * todo: Lionel --> this case could also be ignored.
  */
 if(relocation_cancel_response_pP->cause.cause_value != REQUEST_ACCEPTED){
   OAILOG_WARNING(LOG_MME_APP, "RELOCATION_CANCEL_REPONSE for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " is not accepted, instead %d. Ignoring the error and continuing with Handover Cancellation. \n",
       ue_context->mme_ue_s1ap_id, relocation_cancel_response_pP->cause);
 }else{
   OAILOG_INFO(LOG_MME_APP, "RELOCATION_CANCEL_REPONSE was accepted at TARGET-MME side for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
 }
 /** We will not wait for RELOCATION_CANCEL_REQUEST to send Handover Cancel Acknowledge back. */
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//-------------------------------------------------------------------------
void mme_app_set_pdn_connections(struct mme_ue_eps_pdn_connections_s * pdn_connections, struct ue_context_s * ue_context){

  OAILOG_FUNC_IN (LOG_MME_APP);

  DevAssert(ue_context);
  DevAssert(pdn_connections);

  /** Set the PDN connections for all available PDN contexts. */
  pdn_context_t *pdn_context_to_forward = NULL;
  RB_FOREACH (pdn_context_to_forward, PdnContexts, &ue_context->pdn_contexts) {
    DevAssert(pdn_context_to_forward);
    int num_pdn = pdn_connections->num_pdn_connections;
    /*
     * Fill the PDN context for each PDN session into the forward relocation request message (multi APN handover).
     * Use bassign to copy the value of the bstring.
     */
    pdn_connections->pdn_connection[num_pdn].apn_str = bstrcpy(pdn_context_to_forward->apn_subscribed);
    DevAssert(pdn_context_to_forward->paa);
    pdn_connections->pdn_connection[num_pdn].ipv4_address.s_addr = pdn_context_to_forward->paa->ipv4_address.s_addr;
    //    memset (pdn_connections->pdn_connection[num_pdn].ipv6_address, 0, 16);
    memcpy (pdn_connections->pdn_connection[num_pdn].ipv6_address.s6_addr, pdn_context_to_forward->paa->ipv6_address.s6_addr, 16);
    pdn_connections->pdn_connection[num_pdn].ipv6_prefix_length= pdn_context_to_forward->paa->ipv6_prefix_length;
    /** Set linked EBI. */
    pdn_connections->pdn_connection[num_pdn].linked_eps_bearer_id = pdn_context_to_forward->default_ebi;
    /** Set a blank PGW-S5/S8-FTEID. */
    OAI_GCC_DIAG_OFF(pointer-to-int-cast);
    pdn_connections->pdn_connection[num_pdn].pgw_address_for_cp.teid = (teid_t) 0x000000; /**< Which does not matter. */
    OAI_GCC_DIAG_ON(pointer-to-int-cast);
    pdn_connections->pdn_connection[num_pdn].pgw_address_for_cp.interface_type = S5_S8_PGW_GTP_C;
    mme_config_read_lock (&mme_config);
    pdn_connections->pdn_connection[num_pdn].pgw_address_for_cp.ipv4_address = mme_config.ipv4.s11;
    mme_config_unlock (&mme_config);
    pdn_connections->pdn_connection[num_pdn].pgw_address_for_cp.ipv4 = 1;
    /** APN Restriction. */
    pdn_connections->pdn_connection[num_pdn].apn_restriction = 0; // pdn_context_to_forward->apn_restriction
    /** APN-AMBR */
    pdn_connections->pdn_connection[num_pdn].apn_ambr.br_ul = pdn_context_to_forward->subscribed_apn_ambr.br_ul;
    pdn_connections->pdn_connection[num_pdn].apn_ambr.br_dl = pdn_context_to_forward->subscribed_apn_ambr.br_ul;
    /** Set the bearer contexts for all existing bearers of the PDN. */
    bearer_context_t * bearer_context_to_forward = NULL;
    RB_FOREACH (bearer_context_to_forward, SessionBearers, &pdn_context_to_forward->session_bearers) {
      int num_bearer = pdn_connections->pdn_connection[num_pdn].bearer_context_list.num_bearer_context;
      bearer_contexts_to_be_created_t * bearer_list  = &pdn_connections->pdn_connection[num_pdn].bearer_context_list;
      bearer_list->bearer_contexts[num_bearer].eps_bearer_id = bearer_context_to_forward->ebi;
      bearer_list->bearer_contexts[num_bearer].s1u_sgw_fteid.teid = bearer_context_to_forward->s_gw_fteid_s1u.teid; /**< Which does not matter. */
      bearer_list->bearer_contexts[num_bearer].s1u_sgw_fteid.interface_type = S1_U_SGW_GTP_U;
      bearer_list->bearer_contexts[num_bearer].s1u_sgw_fteid.ipv4_address.s_addr = bearer_context_to_forward->s_gw_fteid_s1u.ipv4_address.s_addr;
      bearer_list->bearer_contexts[num_bearer].s1u_sgw_fteid.ipv4 = bearer_context_to_forward->s_gw_fteid_s1u.ipv4;
      /*
       * Set the bearer level level qos values.
       * Also set the MBR/GBR values for each bearer, the target side, then should send MBR/GBR as 0 for non-GBR bearers.
       */
      // todo: divide by 1000?
      bearer_list->bearer_contexts[num_bearer].bearer_level_qos.gbr.br_ul = bearer_context_to_forward->esm_ebr_context.gbr_ul;
      bearer_list->bearer_contexts[num_bearer].bearer_level_qos.gbr.br_dl = bearer_context_to_forward->esm_ebr_context.gbr_dl;
      bearer_list->bearer_contexts[num_bearer].bearer_level_qos.mbr.br_ul = bearer_context_to_forward->esm_ebr_context.mbr_ul;
      bearer_list->bearer_contexts[num_bearer].bearer_level_qos.mbr.br_dl = bearer_context_to_forward->esm_ebr_context.mbr_dl;
      bearer_list->bearer_contexts[num_bearer].bearer_level_qos.qci       = bearer_context_to_forward->qci;
      bearer_list->bearer_contexts[num_bearer].bearer_level_qos.pvi       = bearer_context_to_forward->preemption_vulnerability;
      bearer_list->bearer_contexts[num_bearer].bearer_level_qos.pci       = bearer_context_to_forward->preemption_capability;
      bearer_list->bearer_contexts[num_bearer].bearer_level_qos.pl        = bearer_context_to_forward->priority_level;
      bearer_list->num_bearer_context++;
    }
    pdn_connections->num_pdn_connections++;
  }
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

////-------------------------------------------------------------------------------------------
//void mme_app_process_pdn_connection_ie(ue_context_t * ue_context, pdn_connection_t * pdn_connection){
//  OAILOG_FUNC_IN (LOG_MME_APP);
//
//  /** Update the PDN session information directly in the new UE_Context. */
//  pdn_context_t * pdn_context = mme_app_handle_pdn_connectivity_from_s10(ue_context, &forward_relocation_request_pP->pdn_connections.pdn_connection[num_pdn]);
//    if(pdn_context){
//      OAILOG_INFO (LOG_MME_APP, "Successfully updated the PDN connections for ueId " MME_UE_S1AP_ID_FMT " for pdn %s. \n",
//          ue_context->mme_ue_s1ap_id, forward_relocation_request_pP->pdn_connections.pdn_connection[num_pdn].apn);
//      // todo: @ multi apn handover sending them all together without waiting?
//      /*
//       * Leave the UE context in UNREGISTERED state.
//       * No subscription information at this point.
//       * Not informing the NAS layer at this point. It will be done at the TAU step later on.
//       * We also did not receive any NAS message until yet.
//       *
//       * Just store the received pending MM_CONTEXT and PDN information as pending.
//       * Will check on  them @TAU, before sending S10_CONTEXT_REQUEST to source MME.
//       * The pending TAU information is already stored.
//         */
//        OAILOG_INFO(LOG_MME_APP, "UE_CONTEXT for UE " MME_UE_S1AP_ID_FMT " does not has a default bearer %d. Continuing with CSR. \n", ue_context->mme_ue_s1ap_id, ue_context->default_bearer_id);
//        if(mme_app_send_s11_create_session_req(ue_context, pdn_context, &target_tai) != RETURNok) {
//          OAILOG_CRITICAL (LOG_MME_APP, "MME_APP_FORWARD_RELOCATION_REQUEST. Sending CSR to SAE-GW failed for UE " MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
//          /*
//           * Deallocate the ue context and remove from MME_APP map.
//           * NAS context should not exis or be invalidated*/
//          mme_remove_ue_context (&mme_app_desc.mme_ue_contexts, ue_context);
//          /** Send back failure. */
//          mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, forward_relocation_request_pP->s10_source_mme_teid.ipv4_address, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
//          OAILOG_FUNC_OUT (LOG_MME_APP);
//        }
//      }else{
//        OAILOG_ERROR(LOG_MME_APP, "Error updating PDN connection for ueId " MME_UE_S1AP_ID_FMT " for pdn %s. Skipping the PDN.\n",
//            ue_context->mme_ue_s1ap_id, forward_relocation_request_pP->pdn_connections.pdn_connection[num_pdn].apn);
//      }
//    }
//}

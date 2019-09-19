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
#include "esm_cause.h"
#include "mme_app_ue_context.h"
#include "mme_app_session_context.h"
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
/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/
static void clear_ue_context(ue_context_t * ue_context);
static void release_ue_context(ue_context_t ** ue_context);
static int mme_insert_ue_context(mme_ue_context_t * const mme_ue_context_p, const struct ue_context_s *const ue_context);

//------------------------------------------------------------------------------
static bstring bearer_state2string(const mme_app_bearer_state_t bearer_state)
{
  bstring bsstr = NULL;
  if  (BEARER_STATE_NULL == bearer_state) {
    bsstr = bfromcstr("BEARER_STATE_NULL");
    return bsstr;
  }
  bsstr = bfromcstr(" ");
  if  (BEARER_STATE_SGW_CREATED & bearer_state) bcatcstr(bsstr, "SGW_CREATED ");
  if  (BEARER_STATE_MME_CREATED & bearer_state) bcatcstr(bsstr, "MME_CREATED ");
  if  (BEARER_STATE_ENB_CREATED & bearer_state) bcatcstr(bsstr, "ENB_CREATED ");
  if  (BEARER_STATE_ACTIVE & bearer_state) bcatcstr(bsstr, "ACTIVE");
  return bsstr;
}

//------------------------------------------------------------------------------
ue_context_t * get_new_ue_context() {
  OAILOG_FUNC_IN (LOG_MME_APP);

  // todo: lock the mme_desc
  mme_ue_s1ap_id_t ue_id = mme_app_ctx_get_new_ue_id ();
  if (ue_id == INVALID_MME_UE_S1AP_ID) {
    OAILOG_CRITICAL (LOG_MME_APP, "MME_APP_INITIAL_UE_MESSAGE. MME_UE_S1AP_ID allocation Failed.\n");
    OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
  }
  /** Check the first element in the list. If it is not empty, reject. */
  ue_context_t * ue_context = STAILQ_FIRST(&mme_app_desc.mme_ue_contexts_list);
  DevAssert(ue_context); /**< todo: with locks, it should be guaranteed, that this should exist. */
  if(ue_context->privates.mme_ue_s1ap_id != INVALID_MME_UE_S1AP_ID){
	  OAILOG_ERROR(LOG_MME_APP, "No free UE context left. Cannot allocate a new one.\n");
	  OAILOG_FUNC_RETURN (LOG_MME_APP, NULL);
  }
  // todo: lock the UE_context!!
  /** Found a free pool: Remove it from the head, add the ue_id and set it to the end. */
  STAILQ_REMOVE_HEAD(&mme_app_desc.mme_ue_contexts_list, entries); /**< free_sp is removed. */

  OAILOG_INFO(LOG_MME_APP, "EMMCN-SAP  - " "Clearing received current ue_context %p.\n", ue_context);
  clear_ue_context(ue_context);
  ue_context->privates.mme_ue_s1ap_id = ue_id;
  // todo: unlock the UE_context
  /** Add it to the back of the list. */
  STAILQ_INSERT_TAIL(&mme_app_desc.mme_ue_contexts_list, ue_context, entries);

  /** Add the UE context. */
  /** Since the NAS and MME_APP contexts are split again, we assign a new mme_ue_s1ap_id here. */
  OAILOG_DEBUG (LOG_MME_APP, "MME_APP_INITIAL_UE_MESSAGE. Allocated new MME UE context and new mme_ue_s1ap_id. %d\n", ue_context->privates.mme_ue_s1ap_id);
  DevAssert(mme_insert_ue_context (&mme_app_desc.mme_ue_contexts, ue_context) == 0);
  // todo: unlock mme_desc!
  OAILOG_FUNC_RETURN (LOG_MME_APP, ue_context);
}

void test_ue_context_extablishment(){
	ue_context_t *ue_context_1 = get_new_ue_context();
	ue_context_t *ue_context_2 = get_new_ue_context();
	ue_context_t *ue_context_3 = get_new_ue_context();

	mme_remove_ue_context(&mme_app_desc.mme_ue_contexts, ue_context_1);
	mme_remove_ue_context(&mme_app_desc.mme_ue_contexts, ue_context_2);
	mme_remove_ue_context(&mme_app_desc.mme_ue_contexts, ue_context_3);
}
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
subscription_data_t                           *
mme_ue_subscription_data_exists_imsi (
  mme_ue_context_t * const mme_ue_context_p,
  const imsi64_t imsi)
{
  struct subscription_data_t                    *subscription_data = NULL;

  hashtable_ts_get (mme_ue_context_p->imsi_subscription_profile_htbl, (const hash_key_t)imsi, (void **)&subscription_data);
  if (subscription_data) {
//    lock_ue_contexts(ue_context);
//    OAILOG_TRACE (LOG_MME_APP, "UE  " MME_UE_S1AP_ID_FMT " fetched MM state %s, ECM state %s\n ",mme_ue_s1ap_id,
//        (ue_context->privates.fields.mm_state == UE_UNREGISTERED) ? "UE_UNREGISTERED":(ue_context->privates.fields.mm_state == UE_REGISTERED) ? "UE_REGISTERED":"UNKNOWN",
//        (ue_context->privates.fields.ecm_state == ECM_IDLE) ? "ECM_IDLE":(ue_context->privates.fields.ecm_state == ECM_CONNECTED) ? "ECM_CONNECTED":"UNKNOWN");
  }
  return subscription_data;
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
//        (ue_context->privates.fields.mm_state == UE_UNREGISTERED) ? "UE_UNREGISTERED":(ue_context->privates.fields.mm_state == UE_REGISTERED) ? "UE_REGISTERED":"UNKNOWN",
//        (ue_context->privates.fields.ecm_state == ECM_IDLE) ? "ECM_IDLE":(ue_context->privates.fields.ecm_state == ECM_CONNECTED) ? "ECM_CONNECTED":"UNKNOWN");
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
  const guti_t     * const guti_p)  //  never NULL, if none put &ue_context->privates.fields.guti
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
  void                                   *id = NULL;

  OAILOG_FUNC_IN(LOG_MME_APP);

  OAILOG_TRACE (LOG_MME_APP, "Update ue context.enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ue context.mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ue context.IMSI " IMSI_64_FMT " ue context.GUTI "GUTI_FMT"\n",
      ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.imsi, GUTI_ARG(&ue_context->privates.fields.guti));

  if (guti_p) {
    OAILOG_TRACE (LOG_MME_APP, "Update ue context %p enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " GUTI " GUTI_FMT "\n",
        ue_context, ue_context->privates.fields.enb_ue_s1ap_id, mme_ue_s1ap_id, imsi, GUTI_ARG(guti_p));
  } else {
    OAILOG_TRACE (LOG_MME_APP, "Update ue context %p enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT "\n",
        ue_context, ue_context->privates.fields.enb_ue_s1ap_id, mme_ue_s1ap_id, imsi);
  }
  //  AssertFatal(ue_context->privates.enb_s1ap_id_key == enb_s1ap_id_key,
  //      "Mismatch in UE context enb_s1ap_id_key "MME_APP_ENB_S1AP_ID_KEY_FORMAT"/"MME_APP_ENB_S1AP_ID_KEY_FORMAT"\n",
  //      ue_context->privates.enb_s1ap_id_key, enb_s1ap_id_key);

  AssertFatal((ue_context->privates.mme_ue_s1ap_id == mme_ue_s1ap_id)
      && (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id),
      "Mismatch in UE context mme_ue_s1ap_id "MME_UE_S1AP_ID_FMT"/"MME_UE_S1AP_ID_FMT"\n",
      ue_context->privates.mme_ue_s1ap_id, mme_ue_s1ap_id);

  if ((INVALID_ENB_UE_S1AP_ID_KEY != enb_s1ap_id_key) && (ue_context->privates.enb_s1ap_id_key != enb_s1ap_id_key)) {
    // new insertion of enb_ue_s1ap_id_key,
    h_rc = hashtable_uint64_ts_remove (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context->privates.enb_s1ap_id_key);
    h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)enb_s1ap_id_key, (void *)(uintptr_t)mme_ue_s1ap_id);

    if (HASH_TABLE_OK != h_rc) {
      OAILOG_ERROR (LOG_MME_APP,
          "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " %s\n",
          ue_context, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id, hashtable_rc_code2string(h_rc));
    }
    ue_context->privates.enb_s1ap_id_key = enb_s1ap_id_key;
  }


  if ((INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id) && (ue_context->privates.mme_ue_s1ap_id != mme_ue_s1ap_id)) {
    // new insertion of mme_ue_s1ap_id, not a change in the id
    h_rc = hashtable_ts_remove (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context->privates.mme_ue_s1ap_id,  (void **)&ue_context);
    h_rc = hashtable_ts_insert (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)mme_ue_s1ap_id, (void *)ue_context);

    if (HASH_TABLE_OK != h_rc) {
      OAILOG_ERROR (LOG_MME_APP,
          "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " %s\n",
          ue_context, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id, hashtable_rc_code2string(h_rc));
    }
    ue_context->privates.mme_ue_s1ap_id = mme_ue_s1ap_id;

    if (INVALID_IMSI64 != imsi) {
      h_rc = hashtable_uint64_ts_remove (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)ue_context->privates.fields.imsi);
      h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)imsi, (void *)(uintptr_t)mme_ue_s1ap_id);
      if (HASH_TABLE_OK != h_rc) {
        OAILOG_ERROR (LOG_MME_APP,
            "Error could not update this ue context %p enb_ue_s1ap_ue_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " imsi " IMSI_64_FMT ": %s\n",
            ue_context, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id, imsi, hashtable_rc_code2string(h_rc));
      }
        ue_context->privates.fields.imsi = imsi;
      }
      /** S11 Key. */
      h_rc = hashtable_uint64_ts_remove (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context->privates.fields.mme_teid_s11);
      h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)mme_teid_s11, (void *)(uintptr_t)mme_ue_s1ap_id);
      if (HASH_TABLE_OK != h_rc) {
        OAILOG_TRACE (LOG_MME_APP,
            "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " mme_teid_s11 " TEID_FMT " : %s\n",
            ue_context, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id, mme_teid_s11, hashtable_rc_code2string(h_rc));
      }
      ue_context->privates.fields.mme_teid_s11= mme_teid_s11;

      /** S10 Key. */
      h_rc = hashtable_uint64_ts_remove (mme_ue_context_p->tun10_ue_context_htbl, (const hash_key_t)ue_context->privates.fields.local_mme_teid_s10);
      h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->tun10_ue_context_htbl, (const hash_key_t)local_mme_teid_s10, (void *)(uintptr_t)mme_ue_s1ap_id);
      if (HASH_TABLE_OK != h_rc) {
        OAILOG_TRACE (LOG_MME_APP,
            "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " local_mme_teid_s10 " TEID_FMT " : %s\n",
            ue_context, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id, local_mme_teid_s10, hashtable_rc_code2string(h_rc));
      }
      ue_context->privates.fields.local_mme_teid_s10 = local_mme_teid_s10;


      if (guti_p)
      {
        h_rc = obj_hashtable_uint64_ts_remove(mme_ue_context_p->guti_ue_context_htbl, (const void *const)&ue_context->privates.fields.guti, sizeof (ue_context->privates.fields.guti));
        h_rc = obj_hashtable_uint64_ts_insert (mme_ue_context_p->guti_ue_context_htbl, (const void *const)guti_p, sizeof (*guti_p), (void *)(uintptr_t)mme_ue_s1ap_id);
        if (HASH_TABLE_OK != h_rc) {
          OAILOG_TRACE (LOG_MME_APP, "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " guti " GUTI_FMT " %s\n",
              ue_context, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id, GUTI_ARG(guti_p), hashtable_rc_code2string(h_rc));
        }
        ue_context->privates.fields.guti = *guti_p;
      }
  }

  if ((ue_context->privates.fields.imsi != imsi)
      || (ue_context->privates.mme_ue_s1ap_id != mme_ue_s1ap_id)) {
    h_rc = hashtable_uint64_ts_remove (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)ue_context->privates.fields.imsi);
    if (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id) {
      h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)imsi, (void *)(uintptr_t)mme_ue_s1ap_id);
    } else {
      h_rc = HASH_TABLE_KEY_NOT_EXISTS;
    }
    if (HASH_TABLE_OK != h_rc) {
      OAILOG_TRACE (LOG_MME_APP,
          "Error could not update this ue context %p enb_ue_s1ap_ue_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " imsi " IMSI_64_FMT ": %s\n",
          ue_context, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id, imsi, hashtable_rc_code2string(h_rc));
    }
    ue_context->privates.fields.imsi = imsi;
  }

  /** S11. */
  if ((ue_context->privates.fields.mme_teid_s11 != mme_teid_s11)
      || (ue_context->privates.mme_ue_s1ap_id != mme_ue_s1ap_id)) {
    h_rc = hashtable_uint64_ts_remove (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context->privates.fields.mme_teid_s11);
    if (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id && INVALID_TEID != mme_teid_s11) {
      h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)mme_teid_s11, (void *)(uintptr_t)mme_ue_s1ap_id);
    } else {
      h_rc = HASH_TABLE_KEY_NOT_EXISTS;
    }

    if (HASH_TABLE_OK != h_rc && INVALID_TEID != mme_teid_s11) {
      OAILOG_TRACE (LOG_MME_APP,
          "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " mme_s11_teid " TEID_FMT " : %s\n",
          ue_context, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id, mme_teid_s11, hashtable_rc_code2string(h_rc));
        }
    ue_context->privates.fields.mme_teid_s11 = mme_teid_s11;
  }

  /** S10. */
  if ((ue_context->privates.fields.local_mme_teid_s10 != local_mme_teid_s10)
      || (ue_context->privates.mme_ue_s1ap_id != mme_ue_s1ap_id)) {
    h_rc = hashtable_uint64_ts_remove (mme_ue_context_p->tun10_ue_context_htbl, (const hash_key_t)ue_context->privates.fields.local_mme_teid_s10);
    if (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id && INVALID_TEID != local_mme_teid_s10) {
      h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->tun10_ue_context_htbl, (const hash_key_t)local_mme_teid_s10, (void *)(uintptr_t)mme_ue_s1ap_id);
    } else {
      h_rc = HASH_TABLE_KEY_NOT_EXISTS;
    }

    if (HASH_TABLE_OK != h_rc && INVALID_TEID != local_mme_teid_s10) {
      OAILOG_TRACE (LOG_MME_APP,
          "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " mme_s11_teid " TEID_FMT " : %s\n",
          ue_context, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id, local_mme_teid_s10, hashtable_rc_code2string(h_rc));
        }
    ue_context->privates.fields.local_mme_teid_s10 = local_mme_teid_s10;
  }

  // todo: check if GUTI is necesary in MME_APP context
  if (guti_p) {
    if ((guti_p->gummei.mme_code != ue_context->privates.fields.guti.gummei.mme_code)
        || (guti_p->gummei.mme_gid != ue_context->privates.fields.guti.gummei.mme_gid)
        || (guti_p->m_tmsi != ue_context->privates.fields.guti.m_tmsi)
        || (guti_p->gummei.plmn.mcc_digit1 != ue_context->privates.fields.guti.gummei.plmn.mcc_digit1)
        || (guti_p->gummei.plmn.mcc_digit2 != ue_context->privates.fields.guti.gummei.plmn.mcc_digit2)
        || (guti_p->gummei.plmn.mcc_digit3 != ue_context->privates.fields.guti.gummei.plmn.mcc_digit3)
        || (ue_context->privates.mme_ue_s1ap_id != mme_ue_s1ap_id)) {

        // may check guti_p with a kind of instanceof()?
        h_rc = obj_hashtable_uint64_ts_remove (mme_ue_context_p->guti_ue_context_htbl, &ue_context->privates.fields.guti, sizeof (*guti_p));
        if (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id) {
          h_rc = obj_hashtable_uint64_ts_insert (mme_ue_context_p->guti_ue_context_htbl, (const void *const)guti_p, sizeof (*guti_p), (void *)(uintptr_t)mme_ue_s1ap_id);
        } else {
          h_rc = HASH_TABLE_KEY_NOT_EXISTS;
        }

        if (HASH_TABLE_OK != h_rc) {
          OAILOG_TRACE (LOG_MME_APP, "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " guti " GUTI_FMT " %s\n",
              ue_context, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id, GUTI_ARG(guti_p), hashtable_rc_code2string(h_rc));
        }
        ue_context->privates.fields.guti = *guti_p;
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
  hashtable_ts_dump_content(mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, tmp);
  OAILOG_TRACE (LOG_MME_APP,"mme_ue_s1ap_id_ue_context_htbl %s\n", bdata(tmp));

  btrunc(tmp, 0);
  hashtable_uint64_ts_dump_content (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl, tmp);
  OAILOG_TRACE (LOG_MME_APP,"enb_ue_s1ap_id_ue_context_htbl %s\n", bdata(tmp));

  btrunc(tmp, 0);
  obj_hashtable_uint64_ts_dump_content (mme_app_desc.mme_ue_contexts.guti_ue_context_htbl, tmp);
  OAILOG_TRACE (LOG_MME_APP,"guti_ue_context_htbl %s", bdata(tmp));
  bdestroy_wrapper(&tmp);
}

// todo: check the locks here
//------------------------------------------------------------------------------
static int
mme_insert_ue_context (
  mme_ue_context_t * const mme_ue_context_p,
  const struct ue_context_s *const ue_context)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;

    OAILOG_FUNC_IN (LOG_MME_APP);
    DevAssert (mme_ue_context_p );
    DevAssert (ue_context );


    // filled ENB UE S1AP ID
    /** Check that the eNB_S1AP_ID_KEY exists. */
    if(ue_context->privates.enb_s1ap_id_key != INVALID_ENB_UE_S1AP_ID_KEY){
      h_rc = hashtable_uint64_ts_is_key_exists (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context->privates.enb_s1ap_id_key);
      if (HASH_TABLE_OK == h_rc) {
        OAILOG_DEBUG (LOG_MME_APP, "This ue context %p already exists enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT "\n",
            ue_context, ue_context->privates.fields.enb_ue_s1ap_id);
        OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
      }
      h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl,
                                 (const hash_key_t)ue_context->privates.enb_s1ap_id_key,
                                  (void *)((uintptr_t)ue_context->privates.mme_ue_s1ap_id));
    }else{
      OAILOG_DEBUG (LOG_MME_APP, "The received enb_ue_s1ap_id_key is invalid " ENB_UE_S1AP_ID_FMT ". Skipping. \n",
          ue_context, ue_context->privates.fields.enb_ue_s1ap_id);
    }

    if (HASH_TABLE_OK != h_rc) {
      OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ue_id 0x%x\n",
          ue_context, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);
      OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
    }

    if (INVALID_MME_UE_S1AP_ID != ue_context->privates.mme_ue_s1ap_id) {
      h_rc = hashtable_ts_is_key_exists (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context->privates.mme_ue_s1ap_id);

      if (HASH_TABLE_OK == h_rc) {
        OAILOG_DEBUG (LOG_MME_APP, "This ue context %p already exists mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
            ue_context, ue_context->privates.mme_ue_s1ap_id);
        OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
      }

      h_rc = hashtable_ts_insert (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl,
                                  (const hash_key_t)ue_context->privates.mme_ue_s1ap_id,
                                  (void *)ue_context);

      if (HASH_TABLE_OK != h_rc) {
        OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
            ue_context, ue_context->privates.mme_ue_s1ap_id);
        OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
      }

      // filled IMSI
      if (ue_context->privates.fields.imsi) {
        h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->imsi_ue_context_htbl,
                                    (const hash_key_t)ue_context->privates.fields.imsi,
                                    (void *)((uintptr_t)ue_context->privates.mme_ue_s1ap_id));

        if (HASH_TABLE_OK != h_rc) {
          OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " imsi %" SCNu64 "\n",
              ue_context, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.imsi);
          OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
        }
      }

      // filled S11 tun id
      if (ue_context->privates.fields.mme_teid_s11) {
        h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->tun11_ue_context_htbl,
                                   (const hash_key_t)ue_context->privates.fields.mme_teid_s11,
                                   (void *)((uintptr_t)ue_context->privates.mme_ue_s1ap_id));

        if (HASH_TABLE_OK != h_rc) {
          OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " mme_teid_s11 " TEID_FMT "\n",
              ue_context, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.mme_teid_s11);
          OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
        }
      }

      // filled S10 tun id
      if (ue_context->privates.fields.local_mme_teid_s10) {
        h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->tun10_ue_context_htbl,
            (const hash_key_t)ue_context->privates.fields.local_mme_teid_s10,
            (void *)((uintptr_t)ue_context->privates.mme_ue_s1ap_id));

        if (HASH_TABLE_OK != h_rc) {
          OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " local_mme_teid_s10 " TEID_FMT "\n",
              ue_context, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.local_mme_teid_s10);
          OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
        }
      }

      // filled guti
      if ((0 != ue_context->privates.fields.guti.gummei.mme_code) || (0 != ue_context->privates.fields.guti.gummei.mme_gid) || (0 != ue_context->privates.fields.guti.m_tmsi) || (0 != ue_context->privates.fields.guti.gummei.plmn.mcc_digit1) ||     // MCC 000 does not exist in ITU table
          (0 != ue_context->privates.fields.guti.gummei.plmn.mcc_digit2)
          || (0 != ue_context->privates.fields.guti.gummei.plmn.mcc_digit3)) {

        h_rc = obj_hashtable_uint64_ts_insert (mme_ue_context_p->guti_ue_context_htbl,
                                       (const void *const)&ue_context->privates.fields.guti,
                                       sizeof (ue_context->privates.fields.guti),
                                       (void *)((uintptr_t)ue_context->privates.mme_ue_s1ap_id));

        if (HASH_TABLE_OK != h_rc) {
          OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " guti "GUTI_FMT"\n",
                  ue_context, ue_context->privates.mme_ue_s1ap_id, GUTI_ARG(&ue_context->privates.fields.guti));
          OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
        }
      }
    }
  /*
   * Updating statistics
   */
  __sync_fetch_and_add (&mme_ue_context_p->nb_ue_context_managed, 1);
  __sync_fetch_and_add (&mme_ue_context_p->nb_ue_context_since_last_stat, 1);

  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}

/**
 * We don't need to notify anything.
 * 1- EMM and MME_APP are decoupled. That MME_APP is removed or not is/should be no problem for the EMM.
 * 2- The PDN session deletion process from the SAE-GW is now done always before the detach. That eases stuff.
 */

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
  if (ue_context->privates.fields.imsi) {
    hash_rc = hashtable_uint64_ts_remove (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)ue_context->privates.fields.imsi);
    if (HASH_TABLE_OK != hash_rc)
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", IMSI %" SCNu64 "  not in IMSI collection",
          ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.imsi);
  }

  // eNB UE S1P UE ID
  hash_rc = hashtable_uint64_ts_remove (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context->privates.enb_s1ap_id_key);
  if (HASH_TABLE_OK != hash_rc)
    OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", ENB_UE_S1AP_ID not ENB_UE_S1AP_ID collection. \n",
      ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);

  // filled S11 tun id
  if (ue_context->privates.fields.mme_teid_s11) {
    hash_rc = hashtable_uint64_ts_remove (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context->privates.fields.mme_teid_s11);
    if (HASH_TABLE_OK != hash_rc)
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", MME TEID_S11 " TEID_FMT "  not in S11 collection. \n",
          ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.mme_teid_s11);
  }

  // filled S10 tun id
  if (ue_context->privates.fields.local_mme_teid_s10) {
    hash_rc = hashtable_uint64_ts_remove (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context->privates.fields.local_mme_teid_s10);
    if (HASH_TABLE_OK != hash_rc)
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", LOCAL MME TEID S10 " TEID_FMT "  not in S10 collection. \n",
          ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.local_mme_teid_s10);
  }

  // filled guti
  if ((ue_context->privates.fields.guti.gummei.mme_code) || (ue_context->privates.fields.guti.gummei.mme_gid) || (ue_context->privates.fields.guti.m_tmsi) ||
      (ue_context->privates.fields.guti.gummei.plmn.mcc_digit1) || (ue_context->privates.fields.guti.gummei.plmn.mcc_digit2) || (ue_context->privates.fields.guti.gummei.plmn.mcc_digit3)) { // MCC 000 does not exist in ITU table
    hash_rc = obj_hashtable_uint64_ts_remove (mme_ue_context_p->guti_ue_context_htbl, (const void *const)&ue_context->privates.fields.guti, sizeof (ue_context->privates.fields.guti));
    if (HASH_TABLE_OK != hash_rc)
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", GUTI  not in GUTI collection",
          ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);
  }

  // filled NAS UE ID/ MME UE S1AP ID
  if (INVALID_MME_UE_S1AP_ID != ue_context->privates.mme_ue_s1ap_id) {
    hash_rc = hashtable_ts_remove (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context->privates.mme_ue_s1ap_id, (void **)&ue_context);
    if (HASH_TABLE_OK != hash_rc)
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT ", mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " not in MME UE S1AP ID collection",
          ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);
  }

  release_ue_context(&ue_context);
  // todo: unlock?
  //  unlock_ue_contexts(ue_context);
  /*
   * Updating statistics
   */
  __sync_fetch_and_sub (&mme_ue_context_p->nb_ue_context_managed, 1);
  __sync_fetch_and_sub (&mme_ue_context_p->nb_ue_context_since_last_stat, 1);

  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
int
mme_insert_subscription_profile(
  mme_ue_context_t * const mme_ue_context_p,
  const imsi64_t imsi, const subscription_data_t * subscription_profile)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (mme_ue_context_p );
  DevAssert (subscription_profile);

  h_rc = hashtable_ts_is_key_exists (mme_ue_context_p->imsi_subscription_profile_htbl, (const hash_key_t)imsi);

  if (HASH_TABLE_OK == h_rc) {
    OAILOG_DEBUG (LOG_MME_APP, "Subscription profile for IMSI " IMSI_64_FMT " already exists.\n", imsi);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  h_rc = hashtable_ts_insert (mme_ue_context_p->imsi_subscription_profile_htbl,
      (const hash_key_t)imsi, (void *)subscription_profile);

  if (HASH_TABLE_OK != h_rc) {
    OAILOG_DEBUG (LOG_MME_APP, "Error could not register the subscription profile for IMSI " IMSI_64_FMT ". \n", imsi);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);

  /*
   * Updating statistics
   */
  __sync_fetch_and_add (&mme_ue_context_p->nb_apn_configuration, 1);

  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
subscription_data_t * mme_remove_subscription_profile(mme_ue_context_t * const mme_ue_context_p, imsi64_t imsi){
  // filled NAS UE ID/ MME UE S1AP ID
//  LOCK_SUBSCRIPTION_DATA_TREE();
  hashtable_rc_t                          hash_rc = HASH_TABLE_OK;
  subscription_data_t                    *subscription_data = NULL;
  OAILOG_FUNC_IN (LOG_MME_APP);
  hash_rc = hashtable_ts_remove (mme_ue_context_p->imsi_subscription_profile_htbl, (const hash_key_t)imsi, (void **)&subscription_data);
  if (HASH_TABLE_OK != hash_rc){
    OAILOG_WARNING(LOG_MME_APP, "No subscription data was found for IMSI " IMSI_64_FMT " in the subscription profile cache.", imsi);
    OAILOG_FUNC_RETURN( LOG_MME_APP, NULL);
  }
  /*
   * Updating statistics
   */
  __sync_fetch_and_sub(&mme_ue_context_p->nb_apn_configuration, 1);

  OAILOG_FUNC_RETURN(LOG_MME_APP, subscription_data);
}

//------------------------------------------------------------------------------
int mme_app_update_ue_subscription(mme_ue_s1ap_id_t ue_id, subscription_data_t * subscription_data){
  OAILOG_FUNC_IN (LOG_MME_APP);

  struct ue_session_pool_s                    *ue_session_pool = NULL;
  struct ue_context_s	                      *ue_context 	   = NULL;

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, ue_id);
  if (!ue_session_pool) {
    OAILOG_WARNING (LOG_MME_APP, "No UE session pool was found for ue_id " MME_UE_S1AP_ID_FMT ". Cannot update subscription profile. \n", ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }

  ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if (!ue_context) {
    OAILOG_WARNING (LOG_MME_APP, "No UE context was found for ue_id " MME_UE_S1AP_ID_FMT ". Cannot update subscription profile. \n", ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }
  /*
   * This is the UE-AMBR and will always be enforced upon all established PDN contexts as total used bitrate (to the eNB).
   */
  // todo: LOCK UE_session_pool
  memcpy (&ue_session_pool->privates.fields.subscribed_ue_ambr, &subscription_data->subscribed_ambr, sizeof (ambr_t));
  // todo: UNLOCK UE_session_pool

  // todo: LOCK UE_context
  ue_context->privates.fields.access_restriction_data = subscription_data->access_restriction;

  if(ue_context->privates.fields.msisdn)
	  bdestroy_wrapper(&ue_context->privates.fields.msisdn);
  ue_context->privates.fields.msisdn = blk2bstr(subscription_data->msisdn, subscription_data->msisdn_length);
  //  AssertFatal (ula_pP->subscription_data.msisdn_length != 0, "MSISDN LENGTH IS 0"); todo: msisdn
  AssertFatal (subscription_data->msisdn_length <= MSISDN_LENGTH, "MSISDN LENGTH is too high %u", MSISDN_LENGTH);
  ue_context->privates.fields.network_access_mode = subscription_data->access_mode;
  /*
   * Set the value of  Mobile Reachability timer based on value of T3412 (Periodic TAU timer) sent in Attach accept /TAU accept.
   * Set it to MME_APP_DELTA_T3412_REACHABILITY_TIMER minutes greater than T3412.
   * Set the value of Implicit timer. Set it to MME_APP_DELTA_REACHABILITY_IMPLICIT_DETACH_TIMER minutes greater than  Mobile Reachability timer
    */
  ue_context->privates.mobile_reachability_timer.id = MME_APP_TIMER_INACTIVE_ID;
  ue_context->privates.mobile_reachability_timer.sec = ((mme_config.nas_config.t3412_min) + MME_APP_DELTA_T3412_REACHABILITY_TIMER) * 60;
  ue_context->privates.implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;
  ue_context->privates.implicit_detach_timer.sec = (ue_context->privates.mobile_reachability_timer.sec) + MME_APP_DELTA_REACHABILITY_IMPLICIT_DETACH_TIMER * 60;
  // todo: UNLOCK UE_CONTEXT
  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
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
void mme_app_dump_bearer_context (const bearer_context_new_t * const bc, uint8_t indent_spaces, bstring bstr_dump)
{
  bformata (bstr_dump, "%*s - Bearer id .......: %02u\n", indent_spaces, " ", bc->ebi);
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
  bformata (bstr_dump, "%*s - QCI .............: %u\n", indent_spaces, " ", bc->bearer_level_qos.qci);
  bformata (bstr_dump, "%*s - Priority level ..: %u\n", indent_spaces, " ", bc->bearer_level_qos.pl);
  // todo: use real values else than s11
//  bformata (bstr_dump, "%*s - Pre-emp vul .....: %s\n", indent_spaces, " ", ((bc->bearer_level_qos.pci) == PRE_EMPTION_VULNERABILITY_ENABLED) ? "ENABLED" : "DISABLED");
//  bformata (bstr_dump, "%*s - Pre-emp cap .....: %s\n", indent_spaces, " ", (bc->bearer_level_qos.pvi   == PRE_EMPTION_CAPABILITY_ENABLED) ? "ENABLED" : "DISABLED");
  bformata (bstr_dump, "%*s - GBR UL ..........: %010" PRIu64 "\n", indent_spaces, " ", bc->bearer_level_qos.gbr.br_ul);
  bformata (bstr_dump, "%*s - GBR DL ..........: %010" PRIu64 "\n", indent_spaces, " ", bc->bearer_level_qos.gbr.br_dl);
  bformata (bstr_dump, "%*s - MBR UL ..........: %010" PRIu64 "\n", indent_spaces, " ", bc->bearer_level_qos.mbr.br_ul);
  bformata (bstr_dump, "%*s - MBR DL ..........: %010" PRIu64 "\n", indent_spaces, " ", bc->bearer_level_qos.mbr.br_ul);
  bstring bstate = bearer_state2string(bc->bearer_state);
  bformata (bstr_dump, "%*s - State ...........: %s\n", indent_spaces, " ", bdata(bstate));
  bdestroy_wrapper(&bstate);
  bformata (bstr_dump, "%*s - "ANSI_COLOR_BOLD_ON"NAS ESM bearer private data .:\n", indent_spaces, " ");
  bformata (bstr_dump, "%*s -     ESM State .......: %s\n", indent_spaces, " ", esm_ebr_state2string(bc->esm_ebr_context.status));
//  bformata (bstr_dump, "%*s -     Timer id ........: %lx\n", indent_spaces, " ", bc->esm_ebr_context.timer.id);
//  bformata (bstr_dump, "%*s -     Timer TO(seconds): %ld\n", indent_spaces, " ", bc->esm_ebr_context.timer.sec);
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
    if (((struct sockaddr*)&pdn_context->s_gw_addr_s11_s4)->sa_family == AF_INET) {
      char ipv4[INET_ADDRSTRLEN];
//      struct sockaddr_in * test = (struct sockaddr_in *)&pdn_context->s_gw_address_s11_s4;
      inet_ntop (AF_INET,
    		  (void*)&((struct sockaddr_in *)&pdn_context->s_gw_addr_s11_s4)->sin_addr,

			  ipv4, INET_ADDRSTRLEN);
      bformata (bstr_dump, "%*s - S-GW s11_s4 (IPv4) .: %s\n", indent_spaces, " ", ipv4);
    } else {
      char                                    ipv6[INET6_ADDRSTRLEN];
      inet_ntop (AF_INET6, & ((struct sockaddr_in6*)&pdn_context->s_gw_addr_s11_s4)->sin6_addr, ipv6, INET6_ADDRSTRLEN);
      bformata (bstr_dump, "%*s - S-GW s11_s4 (IPv6) .: %s\n", indent_spaces, " ", indent_spaces, " ", ipv6);
    }
    bformata (bstr_dump, "%*s - S-GW TEID s5 s8 cp .: " TEID_FMT "\n", indent_spaces, " ", pdn_context->s_gw_teid_s11_s4);

    bformata (bstr_dump, "%*s     - APN-AMBR (bits/s) DL .....: %010" PRIu64 "\n", indent_spaces, " ", pdn_context->subscribed_apn_ambr.br_dl);
    bformata (bstr_dump, "%*s     - APN-AMBR (bits/s) UL .....: %010" PRIu64 "\n", indent_spaces, " ", pdn_context->subscribed_apn_ambr.br_ul);
    bformata (bstr_dump, "%*s     - Default EBI ..............: %u\n", indent_spaces, " ", pdn_context->default_ebi);
    bformata (bstr_dump, "%*s - NAS ESM private data:\n");
//    bformata (bstr_dump, "%*s     - Procedure transaction ID .: %x\n", indent_spaces, " ", pdn_context->pti);
//    bformata (bstr_dump, "%*s     -  Is emergency .............: %s\n", indent_spaces, " ", (pdn_context->esm_data.is_emergency) ? "yes":"no");
//    bformata (bstr_dump, "%*s     -  APN AMBR .................: %u\n", indent_spaces, " ", pdn_context->esm_data.ambr);
//    bformata (bstr_dump, "%*s     -  Addr realloc allowed......: %s\n", indent_spaces, " ", (pdn_context->esm_data.addr_realloc) ? "yes":"no");
//    bformata (bstr_dump, "%*s     -  Num allocated EPS bearers.: %d\n", indent_spaces, " ", pdn_context->esm_data.n_bearers);
    bformata (bstr_dump, "%*s - Bearer List:\n");

    /** Set all bearers of the EBI to valid. */
    bearer_context_new_t * bc_to_dump = NULL;
    STAILQ_FOREACH(bc_to_dump, &pdn_context->session_bearers, entries) {
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
    bformata (bstr_dump, "    - eNB UE s1ap ID .: %08x\n", ue_context->privates.fields.enb_ue_s1ap_id);
    bformata (bstr_dump, "    - MME UE s1ap ID .: %08x\n", ue_context->privates.mme_ue_s1ap_id);
    bformata (bstr_dump, "    - MME S11 TEID ...: " TEID_FMT "\n", ue_context->privates.fields.mme_teid_s11);
    bformata (bstr_dump, "                        | mcc | mnc | cell identity |\n");
    bformata (bstr_dump, "    - E-UTRAN CGI ....: | %u%u%u | %u%u%c | %05x.%02x    |\n",
                 ue_context->privates.fields.e_utran_cgi.plmn.mcc_digit1,
                 ue_context->privates.fields.e_utran_cgi.plmn.mcc_digit2 ,
                 ue_context->privates.fields.e_utran_cgi.plmn.mcc_digit3,
                 ue_context->privates.fields.e_utran_cgi.plmn.mnc_digit1,
                 ue_context->privates.fields.e_utran_cgi.plmn.mnc_digit2,
                 (ue_context->privates.fields.e_utran_cgi.plmn.mnc_digit3 > 9) ? ' ':0x30+ue_context->privates.fields.e_utran_cgi.plmn.mnc_digit3,
                 ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id, ue_context->privates.fields.e_utran_cgi.cell_identity.cell_id);
    /*
     * Ctime return a \n in the string
     */
    bformata (bstr_dump, "    - Last acquired ..: %s", ctime (&ue_context->privates.fields.cell_age));

//    emm_context_dump(&ue_context->emm_context, 4, bstr_dump);
    /*
     * Display UE info only if we know them
     */
    // todo: subscription-known
//    if (SUBSCRIPTION_KNOWN == ue_context->subscription_known) {
//      // TODO bformata (bstr_dump, "    - Status .........: %s\n", (ue_context->sub_status == SS_SERVICE_GRANTED) ? "Granted" : "Barred");
//#define DISPLAY_BIT_MASK_PRESENT(mASK)   \
//    ((ue_context->privates.fields.access_restriction_data & mASK) ? 'X' : 'O')
//      bformata (bstr_dump, "    (O = allowed, X = !O) |UTRAN|GERAN|GAN|HSDPA EVO|E_UTRAN|HO TO NO 3GPP|\n");
//      bformata (bstr_dump, "    - Access restriction  |  %c  |  %c  | %c |    %c    |   %c   |      %c      |\n",
//          DISPLAY_BIT_MASK_PRESENT (ARD_UTRAN_NOT_ALLOWED),
//          DISPLAY_BIT_MASK_PRESENT (ARD_GERAN_NOT_ALLOWED),
//          DISPLAY_BIT_MASK_PRESENT (ARD_GAN_NOT_ALLOWED), DISPLAY_BIT_MASK_PRESENT (ARD_I_HSDPA_EVO_NOT_ALLOWED), DISPLAY_BIT_MASK_PRESENT (ARD_E_UTRAN_NOT_ALLOWED), DISPLAY_BIT_MASK_PRESENT (ARD_HO_TO_NON_3GPP_NOT_ALLOWED));
//      // TODO bformata (bstr_dump, "    - Access Mode ....: %s\n", ACCESS_MODE_TO_STRING (ue_context->access_mode));
//      // TODO MSISDN
//      //bformata (bstr_dump, "    - MSISDN .........: %s\n", (ue_context->privates.fields.msisdn) ? ue_context->privates.fields.msisdn->data:"None");
//      bformata (bstr_dump, "    - RAU/TAU timer ..: %u\n", ue_context->rau_tau_timer);
//      // TODO IMEISV
//      //if (IS_EMM_CTXT_PRESENT_IMEISV(&ue_context->nas_emm_context)) {
//      //  bformata (bstr_dump, "    - IMEISV .........: %*s\n", IMEISV_DIGITS_MAX, ue_context->nas_emm_context._imeisv);
//      //}
//      bformata (bstr_dump, "    - AMBR (bits/s)     ( Downlink |  Uplink  )\n");
//      // TODO bformata (bstr_dump, "        Subscribed ...: (%010" PRIu64 "|%010" PRIu64 ")\n", ue_context->subscribed_ambr.br_dl, ue_context->subscribed_ambr.br_ul);
//      bformata (bstr_dump, "        Allocated ....: (%010" PRIu64 "|%010" PRIu64 ")\n", ue_context->privates.fields.subscribed_ue_ambr.br_dl, ue_context->privates.fields.subscribed_ue_ambr.br_ul);
//
//      bformata (bstr_dump, "    - APN config list:\n");
//
//      for (j = 0; j < ue_context->apn_config_profile.nb_apns; j++) {
//        struct apn_configuration_s             *apn_config_p;
//
//        apn_config_p = &ue_context->apn_config_profile.apn_configuration[j];
//        /*
//         * Default APN ?
//         */
//        bformata (bstr_dump, "        - Default APN ...: %s\n", (apn_config_p->context_identifier == ue_context->apn_config_profile.context_identifier)
//                     ? "TRUE" : "FALSE");
//        bformata (bstr_dump, "        - APN ...........: %s\n", apn_config_p->service_selection);
//        bformata (bstr_dump, "        - AMBR (bits/s) ( Downlink |  Uplink  )\n");
//        bformata (bstr_dump, "                        (%010" PRIu64 "|%010" PRIu64 ")\n", apn_config_p->ambr.br_dl, apn_config_p->ambr.br_ul);
//        bformata (bstr_dump, "        - PDN type ......: %s\n", PDN_TYPE_TO_STRING (apn_config_p->pdn_type));
//        bformata (bstr_dump, "        - QOS\n");
//        bformata (bstr_dump, "            QCI .........: %u\n", apn_config_p->subscribed_qos.qci);
//        bformata (bstr_dump, "            Prio level ..: %u\n", apn_config_p->subscribed_qos.allocation_retention_priority.priority_level);
//        bformata (bstr_dump, "            Pre-emp vul .: %s\n", (apn_config_p->subscribed_qos.allocation_retention_priority.pre_emp_vulnerability == PRE_EMPTION_VULNERABILITY_ENABLED) ? "ENABLED" : "DISABLED");
//        bformata (bstr_dump, "            Pre-emp cap .: %s\n", (apn_config_p->subscribed_qos.allocation_retention_priority.pre_emp_capability == PRE_EMPTION_CAPABILITY_ENABLED) ? "ENABLED" : "DISABLED");
//
//        if (apn_config_p->nb_ip_address == 0) {
//          bformata (bstr_dump, "            IP addr .....: Dynamic allocation\n");
//        } else {
//          int                                     i;
//
//          bformata (bstr_dump, "            IP addresses :\n");
//
//          for (i = 0; i < apn_config_p->nb_ip_address; i++) {
//            if (apn_config_p->ip_address[i].pdn_type == IPv4) {
//              char ipv4[INET_ADDRSTRLEN];
//              inet_ntop (AF_INET, (void*)&apn_config_p->ip_address[i].address.ipv4_address, ipv4, INET_ADDRSTRLEN);
//              bformata (bstr_dump, "                           [%s]\n", ipv4);
//            } else {
//              char                                    ipv6[INET6_ADDRSTRLEN];
//              inet_ntop (AF_INET6, &apn_config_p->ip_address[i].address.ipv6_address, ipv6, INET6_ADDRSTRLEN);
//              bformata (bstr_dump, "                           [%s]\n", ipv6);
//            }
//          }
//        }
//        bformata (bstr_dump, "\n");
//      }
//      bformata (bstr_dump, "    - PDNs:\n");
//
//      /** No matter if it is a INTRA or INTER MME handover, continue with the MBR for other PDNs. */
//      pdn_context_t * pdn_context_to_dump = NULL;
//      RB_FOREACH (pdn_context_to_dump, PdnContexts, &ue_context->pdn_contexts) {
//        AssertFatal(pdn_context_to_dump, "Mismatch in configuration bearer context NULL\n");
//        mme_app_dump_pdn_context(ue_context, pdn_context_to_dump, pdn_context_to_dump->context_identifier, 8, bstr_dump);
//      }
//
//
//    }
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
  hashtable_uint64_ts_apply_callback_on_elements (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, mme_app_dump_ue_context, NULL, NULL);
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
  if ((!ue_context) || (INVALID_MME_UE_S1AP_ID == ue_context->privates.mme_ue_s1ap_id)) {
    NOT_REQUIREMENT_3GPP_36_413(R10_8_3_3_2__2);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "0 S1AP_UE_CONTEXT_RELEASE_REQ Unknown mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "  enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ",
        s1ap_ue_context_release_req->mme_ue_s1ap_id, s1ap_ue_context_release_req->enb_ue_s1ap_id);
    OAILOG_ERROR (LOG_MME_APP, " UE Context Release Req: UE context doesn't exist for enb_ue_s1ap_ue_id "
        ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id, mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  // Set the UE context release cause in UE context. This is used while constructing UE Context Release Command
  ue_context->privates.s1_ue_context_release_cause = cause;
  if (ue_context->privates.fields.ecm_state == ECM_IDLE) {
    // This case could happen during sctp reset, before the UE could move to ECM_CONNECTED
    // calling below function to set the enb_s1ap_id_key to invalid
    if (ue_context->privates.s1_ue_context_release_cause == S1AP_SCTP_SHUTDOWN_OR_RESET) {
      mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);
      mme_app_itti_ue_context_release(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.s1_ue_context_release_cause, enb_id);
      OAILOG_WARNING (LOG_MME_APP, "UE Context Release Reqeust:Cause SCTP RESET/SHUTDOWN. UE state: IDLE. mme_ue_s1ap_id = %d, enb_ue_s1ap_id = %d Action -- Handle the message\n ",
          ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id);
    }
    OAILOG_ERROR (LOG_MME_APP, "ERROR: UE Context Release Request: UE state : IDLE. enb_ue_s1ap_ue_id "
    ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " Action--- Ignore the message\n", ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);
//    unlock_ue_contexts(ue_context);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  // Stop Initial context setup process guard timer,if running
  if (ue_context->privates.initial_context_setup_rsp_timer.id != MME_APP_TIMER_INACTIVE_ID) {
    if (timer_remove(ue_context->privates.initial_context_setup_rsp_timer.id, NULL)) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to stop Initial Context Setup Rsp timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
    }
    ue_context->privates.initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
    // Setting UE context release cause as Initial context setup failure
    ue_context->privates.s1_ue_context_release_cause = S1AP_INITIAL_CONTEXT_SETUP_FAILED;
  }
  if (ue_context->privates.fields.mm_state == UE_UNREGISTERED) {
    OAILOG_INFO(LOG_MME_APP, "UE is in UNREGISTERED state. Releasing the UE context in the eNB and triggering an MME context removal for "MME_UE_S1AP_ID_FMT ". \n.", ue_context->privates.mme_ue_s1ap_id);
    emm_data_context_t * emm_context = emm_data_context_get(&_emm_data, ue_context->privates.mme_ue_s1ap_id);
    if(emm_context && is_nas_specific_procedure_attach_running(emm_context)) {
      OAILOG_INFO(LOG_MME_APP, "Attach procedure is running for UE "MME_UE_S1AP_ID_FMT ". Triggering implicit detach (aborting attach procedure). \n.", ue_context->privates.mme_ue_s1ap_id);
      message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
      DevAssert (message_p != NULL);
      message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->privates.mme_ue_s1ap_id; /**< Rest won't be sent, so no NAS Detach Request will be sent. */
      MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
      itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }else{
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
      if(ue_context->privates.s1_ue_context_release_cause == S1AP_INVALID_CAUSE)
        ue_context->privates.s1_ue_context_release_cause = S1AP_INITIAL_CONTEXT_SETUP_FAILED;
      mme_app_itti_ue_context_release(mme_ue_s1ap_id, enb_ue_s1ap_id, ue_context->privates.s1_ue_context_release_cause, enb_id);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
  } else {
    /*
     * In the UE_REGISTERED case, we should always have established bearers.
     * No need to check pending S1U downlink bearers.
     * Also in the case of intra-MME handover with a EMM-REGISTERED UE, we will not receive a UE-Context-Release Request, we will send the release command.
     * No more pending S1U DL FTEID fields necessary, we will automatically remove them once with UE-Ctx-Release-Complete.
     * The bearer contexts of the PDN session will directly be updated when eNodeB sends S1U Dl FTEIDs (can be before MBR is sent to the SAE-GW).
     *
     *
     */

    // release S1-U tunnel mapping in S_GW for all the active bearers for the UE
    // /** Check if this is the main S1AP UE reference, if o
    // todo:   Assert(at_least_1_BEARER_IS_ESTABLISHED_TO_SAEGW)!?!?
	OAILOG_INFO(LOG_MME_APP, "UE is REGISTERED. Sending Release Access Bearer Request for ueId "MME_UE_S1AP_ID_FMT". \n.", mme_ue_s1ap_id);
    mme_app_send_s11_release_access_bearers_req(ue_context->privates.mme_ue_s1ap_id); /**< Release Access bearers and then send context release request.  */
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
  const itti_s1ap_ue_context_release_complete_t * const
  s1ap_ue_context_release_complete)
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

        OAILOG_INFO (LOG_MME_APP, "Implicitly detaching the UE due CLR flag @ completion of MME_MOBILITY timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
        message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
        DevAssert (message_p != NULL);
        message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->privates.mme_ue_s1ap_id; /**< Rest won't be sent, so no NAS Detach Request will be sent. */
        message_p->ittiMsg.nas_implicit_detach_ue_ind.clr = true; /**< Inform about the CLR. */

        MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
        itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);

        OAILOG_FUNC_OUT (LOG_MME_APP);
      }else{
        /** No CLR flag received yet. Release the UE reference. */
        OAILOG_DEBUG(LOG_MME_APP, "Received UE context release complete for the main ue_reference of the UE with mme_ue_s1ap_id "MME_UE_S1AP_ID_FMT" and enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT". \n",
              s1ap_ue_context_release_complete->mme_ue_s1ap_id, s1ap_ue_context_release_complete->enb_ue_s1ap_id);
        /** Not releasing any bearer information. */
        /* Update keys and ECM state. */
        /** Set the ECM state to IDLE. */
        mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);
        if(ue_context->privates.fields.mm_state == UE_UNREGISTERED){
          OAILOG_DEBUG(LOG_MME_APP, "Received UE context release complete for the main ue_reference of the UE with mme_ue_s1ap_id "MME_UE_S1AP_ID_FMT" "
        		  "for an UNREGISTERED UE. \n", s1ap_ue_context_release_complete->mme_ue_s1ap_id);
          /* Don't remove it directly, do it over the ESM. */
          OAILOG_INFO (LOG_MME_APP, "Implicitly detaching the UE due CLR flag @ completion of MME_MOBILITY timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
          message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
          DevAssert (message_p != NULL);
          message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->privates.mme_ue_s1ap_id; /**< Rest won't be sent, so no NAS Detach Request will be sent. */
          MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
          itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
//          mme_remove_ue_context(&mme_app_desc.mme_ue_contexts, ue_context);
          OAILOG_FUNC_OUT (LOG_MME_APP);
        }
        OAILOG_DEBUG(LOG_MME_APP, "Received UE context release complete for the main ue_reference of the UE with mme_ue_s1ap_id "MME_UE_S1AP_ID_FMT" and enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT" in UE_REGISTERED state. "
            "Not performing implicit detach, only idle mode (missing CLR). Notifying the HSS. \n", s1ap_ue_context_release_complete->mme_ue_s1ap_id, s1ap_ue_context_release_complete->enb_ue_s1ap_id);
        mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);

        /** This part is not %100 specification but send a notification request. */
        mme_app_itti_notify_request(ue_context->privates.fields.imsi, &s10_handover_proc->target_tai.plmn, true);

        /** Remove the handover procedure. */
        if (ue_context->s10_procedures) {
          mme_app_delete_s10_procedure_mme_handover(ue_context); // todo: generic s10 function
        }

        OAILOG_FUNC_OUT (LOG_MME_APP);
      }
    }else{
      if(ue_context->privates.s1_ue_context_release_cause == S1AP_HANDOVER_CANCELLED){
        /** Don't change the signaling connection state. */
        // todo: this case might also be a too fast handover back handover failure! don't handle it here, handle it before and immediately Deregister the UE
        mme_app_send_s1ap_handover_cancel_acknowledge(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.fields.sctp_assoc_id_key);
        OAILOG_DEBUG(LOG_MME_APP, "Successfully terminated the resources in the target eNB %d for UE with mme_ue_s1ap_ue_id "MME_UE_S1AP_ID_FMT " (REGISTERED). "
            "Sending HO-CANCELLATION-ACK back to the source eNB. \n", s1ap_ue_context_release_complete->enb_id, ue_context->privates.mme_ue_s1ap_id);
        ue_context->privates.s1_ue_context_release_cause = S1AP_INVALID_CAUSE;
        /** Not releasing the main connection. Removing the handover procedure. */
        if (ue_context->s10_procedures) {
          mme_app_delete_s10_procedure_mme_handover(ue_context); // todo: generic s10 function
        }

        OAILOG_FUNC_OUT (LOG_MME_APP);
      }else{
        /*
         * Assuming normal completion of S10 Intra-MME handover.
         * Delete the S10 Handover Procedure.
         */
        /* Update keys and ECM state. */
//        mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);

        if (ue_context->s10_procedures) {
        	/** If the procedure currently running does not have the enb_ue_s1ap_id in source or target, dismiss. */
        	mme_app_s10_proc_mme_handover_t *s10_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
        	if(s10_proc) {
        		if(ue_context->privates.fields.enb_ue_s1ap_id != s1ap_ue_context_release_complete->enb_ue_s1ap_id
        				&& s10_proc->source_enb_ue_s1ap_id != s1ap_ue_context_release_complete->enb_ue_s1ap_id
        				&& s10_proc->target_enb_ue_s1ap_id != s1ap_ue_context_release_complete->enb_ue_s1ap_id){
        			 OAILOG_WARNING(LOG_MME_APP, "Received a late UE context release complete for ENB_UE_S1AP_ID " ENB_UE_S1AP_ID_FMT " for ueId " MME_UE_S1AP_ID_FMT ". "
        					 "Not further processing the request.", s1ap_ue_context_release_complete->enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);
        			 OAILOG_FUNC_OUT (LOG_MME_APP);
        		}
        	}
            mme_app_delete_s10_procedure_mme_handover(ue_context);
        }

        /** Re-Establish the UE-Reference as the main reference. */
        if(ue_context->privates.fields.enb_ue_s1ap_id != s1ap_ue_context_release_complete->enb_ue_s1ap_id){

          /**
           * Check the cause, if a handover failure occurred with the target enb, without waiting for handover cancel from source, remove the UE reference to source enb, too.
           */
          if(ue_context->privates.s1_ue_context_release_cause == S1AP_HANDOVER_FAILED){
            OAILOG_DEBUG(LOG_MME_APP, "Handover failure. Removing the UE reference to the source enb with enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT ", too (no cancel received) for ueId " MME_UE_S1AP_ID_FMT ". ",
                ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);
            mme_app_itti_ue_context_release(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.s1_ue_context_release_cause, ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id);

          }else{
            OAILOG_DEBUG(LOG_MME_APP, "No handover failure. Establishing the SCTP current UE_REFERENCE enb_ue_s1ap_ue_id " ENB_UE_S1AP_ID_FMT ". ", ue_context->privates.fields.enb_ue_s1ap_id);

            /**
             * This will overwrite the association towards the old eNB if single MME S1AP handover.
             * The old eNB will be referenced by the enb_ue_s1ap_id.
             */
            notify_s1ap_new_ue_mme_s1ap_id_association (ue_context->privates.fields.sctp_assoc_id_key, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);
          }
        }
        /** Continue below, check if the UE is in DEREGISTERED state. */
      }
    }
  }
  if(ue_context->privates.fields.enb_ue_s1ap_id == s1ap_ue_context_release_complete->enb_ue_s1ap_id
		  && (ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id == s1ap_ue_context_release_complete->enb_id)){
	  /* No handover procedure ongoing or no CLR is received yet from the HSs. Assume that this is the main connection and release it. */
	  OAILOG_DEBUG(LOG_MME_APP, "No Handover procedure ongoing. Received UE context release complete for the main ue_reference of the UE with mme_ue_s1ap_id "MME_UE_S1AP_ID_FMT" and enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT""
			  " with enb_id %d. \n",
	        s1ap_ue_context_release_complete->mme_ue_s1ap_id, s1ap_ue_context_release_complete->enb_ue_s1ap_id, s1ap_ue_context_release_complete->enb_id);
	  /** Set the bearers into IDLE mode. */
	  mme_app_ue_session_pool_s1_release_enb_informations(ue_context->privates.mme_ue_s1ap_id);
	  /* Update keys and ECM state. */
	  mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);
  } else {
	  OAILOG_DEBUG(LOG_MME_APP, "Not setting UE context with with mme_ue_s1ap_id "MME_UE_S1AP_ID_FMT" and enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT" to idle. "
			  "Complete received from enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT" and enb_id %d.. \n", ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id,
			  s1ap_ue_context_release_complete->enb_ue_s1ap_id, s1ap_ue_context_release_complete->enb_id);
  }
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
//  if (((S1AP_Cause_PR_radioNetwork == ue_context->privates.s1_ue_context_release_cause.present) &&
//       (S1ap_CauseRadioNetwork_radio_connection_with_ue_lost == ue_context->privates.s1_ue_context_release_cause.choice.radioNetwork))) {
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

  if (ue_context->privates.fields.mm_state == UE_UNREGISTERED) {
    OAILOG_DEBUG (LOG_MME_APP, "Deleting UE context associated in MME for mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n ",
        s1ap_ue_context_release_complete->mme_ue_s1ap_id);
    /**
     * We will not inform NAS. Assuming that it is handled.
     * We assume that all sessions are already released. We don't trigger session removal via S1AP/MME_APP, but only via ESM (UE_SESSION_POOL).
     * Eventually, they will be independently removed.
     */

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
    hash_rc = hashtable_uint64_ts_remove (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context->privates.enb_s1ap_id_key);
    if (HASH_TABLE_OK != hash_rc)
    {
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id_key %ld mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", ENB_UE_S1AP_ID_KEY could not be found",
                                  ue_context->privates.enb_s1ap_id_key, ue_context->privates.mme_ue_s1ap_id);
    }
    ue_context->privates.enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;

    OAILOG_DEBUG (LOG_MME_APP, "MME_APP: UE Connection State changed to IDLE. mme_ue_s1ap_id = %d\n", ue_context->privates.mme_ue_s1ap_id);

    if (mme_config.nas_config.t3412_min > 0) {
      // Start Mobile reachability timer only if periodic TAU timer is not disabled
      if (timer_setup (ue_context->privates.mobile_reachability_timer.sec, 0, TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *)&(ue_context->privates.mme_ue_s1ap_id), &(ue_context->privates.mobile_reachability_timer.id)) < 0) {
        OAILOG_ERROR (LOG_MME_APP, "Failed to start Mobile Reachability timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
        ue_context->privates.mobile_reachability_timer.id = MME_APP_TIMER_INACTIVE_ID;
      } else {
        OAILOG_DEBUG (LOG_MME_APP, "Started Mobile Reachability timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
      }
    }
    if (ue_context->privates.fields.ecm_state == ECM_CONNECTED) {
      ue_context->privates.fields.ecm_state       = ECM_IDLE;
      // Update Stats
      update_mme_app_stats_connected_ue_sub();
    }

  }else if ((ue_context->privates.fields.ecm_state == ECM_IDLE) && (new_ecm_state == ECM_CONNECTED))
  {
    ue_context->privates.fields.ecm_state = ECM_CONNECTED;

    OAILOG_DEBUG (LOG_MME_APP, "MME_APP: UE Connection State changed to CONNECTED.enb_ue_s1ap_id = %d, mme_ue_s1ap_id = %d\n", ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);

    // Stop Mobile reachability timer,if running
    if (ue_context->privates.mobile_reachability_timer.id != MME_APP_TIMER_INACTIVE_ID)
    {
      if (timer_remove(ue_context->privates.mobile_reachability_timer.id, NULL)) {

        OAILOG_ERROR (LOG_MME_APP, "Failed to stop Mobile Reachability timer for UE id "MME_UE_S1AP_ID_FMT". \n", ue_context->privates.mme_ue_s1ap_id);
      }
      ue_context->privates.mobile_reachability_timer.id = MME_APP_TIMER_INACTIVE_ID;
    }
    // Stop Implicit detach timer,if running
    if (ue_context->privates.implicit_detach_timer.id != MME_APP_TIMER_INACTIVE_ID)
    {
      if (timer_remove(ue_context->privates.implicit_detach_timer.id, NULL)) {
        OAILOG_ERROR (LOG_MME_APP, "Failed to stop Implicit Detach timer for UE id "MME_UE_S1AP_ID_FMT". \n", ue_context->privates.mme_ue_s1ap_id);
      }
      ue_context->privates.implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;
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
  if (ue_context->privates.fields.mm_state == UE_UNREGISTERED && (new_mm_state == UE_REGISTERED))
  {
    ue_context->privates.fields.mm_state = new_mm_state;

    // Update Stats
    update_mme_app_stats_attached_ue_add();

  } else if ((ue_context->privates.fields.mm_state == UE_REGISTERED) && (new_mm_state == UE_UNREGISTERED))
  {
    ue_context->privates.fields.mm_state = new_mm_state;

    // Update Stats
    update_mme_app_stats_attached_ue_sub();
  }else{
    //OAILOG_TRACE(LOG_MME_APP, "**** Abnormal - No handler for state transition of UE with mme_ue_s1ap_ue_id "MME_UE_S1AP_ID_FMT " "
    //    "entering %d state from %d state. ****\n", mme_ue_s1ap_id, ue_context->privates.fields.mm_state, new_mm_state);
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
  const itti_s1ap_ue_context_release_req_t * const s1ap_ue_context_release_req)

{
  _mme_app_handle_s1ap_ue_context_release(s1ap_ue_context_release_req->mme_ue_s1ap_id,
                                          s1ap_ue_context_release_req->enb_ue_s1ap_id,
                                          s1ap_ue_context_release_req->enb_id,
                                          S1AP_RADIO_EUTRAN_GENERATED_REASON);
}
//
////------------------------------------------------------------------------------
//void
//mme_app_handle_enb_deregister_ind(const itti_s1ap_eNB_deregistered_ind_t const * eNB_deregistered_ind) {
//  for (int i = 0; i < eNB_deregistered_ind->nb_ue_to_deregister; i++) {
//    _mme_app_handle_s1ap_ue_context_release(eNB_deregistered_ind->mme_ue_s1ap_id[i],
//                                            eNB_deregistered_ind->enb_ue_s1ap_id[i],
//                                            eNB_deregistered_ind->enb_id,
//                                            S1AP_SCTP_SHUTDOWN_OR_RESET);
//  }
//}


//------------------------------------------------------------------------------
void mme_app_handle_s1ap_enb_deregistered_ind (const itti_s1ap_eNB_deregistered_ind_t * const enb_dereg_ind)
{
  for (int ue_idx = 0; ue_idx < enb_dereg_ind->nb_ue_to_deregister; ue_idx++) {
      mme_app_send_nas_signalling_connection_rel_ind(enb_dereg_ind->mme_ue_s1ap_id[ue_idx]); /**< If any procedures were ongoing, kill them. */
      _mme_app_handle_s1ap_ue_context_release(enb_dereg_ind->mme_ue_s1ap_id[ue_idx], enb_dereg_ind->enb_ue_s1ap_id[ue_idx], enb_dereg_ind->enb_id, S1AP_SCTP_SHUTDOWN_OR_RESET);
  }
}

//------------------------------------------------------------------------------
void
mme_app_handle_enb_reset_req (itti_s1ap_enb_initiated_reset_req_t * const enb_reset_req)
{

  MessageDef *message_p;
  OAILOG_DEBUG (LOG_MME_APP, " eNB Reset request received. eNB id = %d, reset_type  %d \n ", enb_reset_req->enb_id, enb_reset_req->s1ap_reset_type);
  DevAssert (enb_reset_req->ue_to_reset_list != NULL);
  if (enb_reset_req->s1ap_reset_type == RESET_ALL) {
  // Full Reset. Trigger UE Context release release for all the connected UEs.
    for (int i = 0; i < enb_reset_req->num_ue; i++) {
      _mme_app_handle_s1ap_ue_context_release((enb_reset_req->ue_to_reset_list[i].mme_ue_s1ap_id) ? *(enb_reset_req->ue_to_reset_list[i].mme_ue_s1ap_id) : INVALID_MME_UE_S1AP_ID,
    		  	  	  	  	  	  	  		(enb_reset_req->ue_to_reset_list[i].enb_ue_s1ap_id) ? *(enb_reset_req->ue_to_reset_list[i].enb_ue_s1ap_id) : 0,
                                            enb_reset_req->enb_id,
                                            S1AP_SCTP_SHUTDOWN_OR_RESET);
    }

  } else { // Partial Reset
    for (int i = 0; i < enb_reset_req->num_ue; i++) {
      if (enb_reset_req->ue_to_reset_list[i].mme_ue_s1ap_id == NULL &&
                          enb_reset_req->ue_to_reset_list[i].enb_ue_s1ap_id == NULL)
        continue;
      else
        _mme_app_handle_s1ap_ue_context_release((enb_reset_req->ue_to_reset_list[i].mme_ue_s1ap_id) ? *(enb_reset_req->ue_to_reset_list[i].mme_ue_s1ap_id) : INVALID_MME_UE_S1AP_ID,
        								    (enb_reset_req->ue_to_reset_list[i].enb_ue_s1ap_id) ? *(enb_reset_req->ue_to_reset_list[i].enb_ue_s1ap_id) : 0,
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
  enb_reset_req->ue_to_reset_list = NULL;
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
  OAILOG_FUNC_IN (LOG_MME_APP);
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
//  if(ue_context->privates.fields.mm_state != UE_UNREGISTERED){
//    OAILOG_ERROR(LOG_MME_APP, "UE MME context is not in UE_UNREGISTERED state but instead in %d for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " and guti: " GUTI_FMT ". \n",
//        ue_context->privates.fields.mm_state, nas_context_req_pP->ue_id, GUTI_ARG(&nas_context_req_pP->old_guti));
//    MSC_LOG_EVENT (MSC_MMEAPP_MME, "UE MME context is not in UE_UNREGISTERED state but instead in %d for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " and guti: " GUTI_FMT ". \n",
//        ue_context->privates.fields.mm_state, nas_context_req_pP->ue_id, GUTI_ARG(&nas_context_req_pP->old_guti));
//    /** This should always clear the allocated UE context resources. */
//    _mme_app_send_nas_context_response_err(nas_context_req_pP->ue_id, SYSTEM_FAILURE);
//    OAILOG_FUNC_OUT (LOG_MME_APP);
//  }
//  /** Assert that there is no s10 handover procedure running. */
  mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
  DevAssert(!s10_handover_proc);

//    /** Report back a context failure, which should remove the UE context together with any processes. */
//    OAILOG_ERROR(LOG_MME_APP, "UE MME context has already a handover procedure for ueId " MME_UE_S1AP_ID_FMT". NAS layer should not have asked this. \n", ue_context->privates.mme_ue_s1ap_id);
//    MSC_LOG_EVENT (MSC_MMEAPP_MME, "UE MME context has already a handover procedure for ueId " MME_UE_S1AP_ID_FMT". NAS layer should not have asked this. \n", ue_context->privates.mme_ue_s1ap_id);
//    // todo: currently, not sending the pending EPS MM context information back!
//    /** This should always clear the allocated UE context resources. */
//    _mme_app_send_nas_context_response_err(ue_context->privates.mme_ue_s1ap_id, SYSTEM_FAILURE);
//    OAILOG_FUNC_OUT (LOG_MME_APP);
//  }

  struct sockaddr *neigh_mme_ip_addr;
  if (1) {
    // TODO prototype may change
    mme_app_select_service(&nas_context_req_pP->originating_tai, &neigh_mme_ip_addr, S10_MME_GTP_C);
    if(!neigh_mme_ip_addr){
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
  memcpy((void*)&s10_context_request_p->peer_ip, neigh_mme_ip_addr,
    		  (neigh_mme_ip_addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

  /** Set the Target MME_S10_FTEID (this MME's S10 Tunnel endpoint). */
  OAI_GCC_DIAG_OFF(pointer-to-int-cast);
  s10_context_request_p->s10_target_mme_teid.teid = local_teid;
  OAI_GCC_DIAG_ON(pointer-to-int-cast);
  s10_context_request_p->s10_target_mme_teid.interface_type = S10_MME_GTP_C;
  /** Set the MME IPv4 and IPv6 addresses in the FTEID. */
  mme_config_read_lock (&mme_config);
  if(mme_config.ip.s11_mme_v4.s_addr){
	  s10_context_request_p->s10_target_mme_teid.ipv4_address = mme_config.ip.s11_mme_v4;
	  s10_context_request_p->s10_target_mme_teid.ipv4 = 1;
  }
  if(memcmp(&mme_config.ip.s11_mme_v6.s6_addr, (void*)&in6addr_any, sizeof(mme_config.ip.s11_mme_v6.s6_addr)) != 0) {
	  memcpy(s10_context_request_p->s10_target_mme_teid.ipv6_address.s6_addr, mme_config.ip.s11_mme_v6.s6_addr,  sizeof(mme_config.ip.s11_mme_v6.s6_addr));
	  s10_context_request_p->s10_target_mme_teid.ipv6 = 1;
  }
  mme_config_unlock (&mme_config);

  s10_context_request_p->s10_target_mme_teid.ipv4 = 1;

  /** Update the MME_APP UE context with the new S10 local TEID to find it from the S10 answer. */
  mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
      ue_context->privates.enb_s1ap_id_key,
      ue_context->privates.mme_ue_s1ap_id,
      ue_context->privates.fields.imsi,
      ue_context->privates.fields.mme_teid_s11,       /**< Won't be changed. */
      local_teid,
      &ue_context->privates.fields.guti); /**< Don't register with the old GUTI. */

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
  s10_context_request_p->serving_network.mcc[0] = ue_context->privates.fields.e_utran_cgi.plmn.mcc_digit1;
  s10_context_request_p->serving_network.mcc[1] = ue_context->privates.fields.e_utran_cgi.plmn.mcc_digit2;
  s10_context_request_p->serving_network.mcc[2] = ue_context->privates.fields.e_utran_cgi.plmn.mcc_digit3;
  s10_context_request_p->serving_network.mnc[0] = ue_context->privates.fields.e_utran_cgi.plmn.mnc_digit1;
  s10_context_request_p->serving_network.mnc[1] = ue_context->privates.fields.e_utran_cgi.plmn.mnc_digit2;
  s10_context_request_p->serving_network.mnc[2] = ue_context->privates.fields.e_utran_cgi.plmn.mnc_digit3;

  /** Send the Forward Relocation Message to S11. */
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME,  MSC_S10_MME ,
      NULL, 0, "0 S10_CONTEXT_REQ for mmeUeS1apId " MME_UE_S1AP_ID_FMT ". \n", nas_ue_context_request_pP->ue_id);
  itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);

  /**
   * S10 Initial request timer will be started on the target MME side in the GTPv2c stack.
   * UE context removal & Attach/TAU reject will be be performed.
   * Also GTPv2c has a transaction started. If no response from source MME arrives, an ITTI message with SYSTEM_FAILURE cause will be returned.
   */
  OAILOG_INFO(LOG_MME_APP, "Successfully sent S10 Context Request for received NAS request for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//----------------------------------------------------------------------------------------------------------
void mme_app_set_ue_eps_mm_context(mm_context_eps_t * ue_eps_mme_context_p, const struct ue_context_s * const ue_context, const struct ue_session_pool_s * const ue_session_pool, emm_data_context_t *ue_nas_ctx) {

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

  /**
   * Set the UE ambr (subscribed).
   * Not divide by 100 here.
   */
  ue_eps_mme_context_p->subscribed_ue_ambr = ue_session_pool->privates.fields.subscribed_ue_ambr;
  /** Calculate the total. */
  ue_eps_mme_context_p->used_ue_ambr       = mme_app_total_p_gw_apn_ambr(ue_session_pool);

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
  ue_eps_mme_context_p->access_restriction_flags        = ue_context->privates.fields.access_restriction_data & 0xFF;
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Send an S10 Context Response with error code.
 * It shall not trigger creating a local S10 tunnel.
 * Parameter is the TEID of the Source-MME.
 */
static
void _mme_app_send_s10_context_response_err(teid_t mme_source_s10_teid, struct sockaddr *mme_source_ip_address, void *trxn,  gtpv2c_cause_value_t cause_val){
  OAILOG_FUNC_IN (LOG_MME_APP);

  /** Send a Context RESPONSE with error cause. */
  MessageDef * message_p = itti_alloc_new_message (TASK_MME_APP, S10_CONTEXT_RESPONSE);
  DevAssert (message_p != NULL);

  itti_s10_context_response_t *s10_context_response_p = &message_p->ittiMsg.s10_context_response;
  memset ((void*)s10_context_response_p, 0, sizeof (itti_s10_context_response_t));
  /** Set the TEID of the source MME. */
  s10_context_response_p->teid = mme_source_s10_teid; /**< Not set into the UE context yet. */
  /** Set the IPv4 address of the source MME. */
  memcpy((void*)&s10_context_response_p->peer_ip, mme_source_ip_address,
		  (mme_source_ip_address->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

  s10_context_response_p->cause.cause_value = cause_val;
  s10_context_response_p->trxnId  = trxn;
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
 mme_app_s10_proc_mme_handover_t        *s10_proc_mme_tau = NULL;
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
   _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, &s10_context_request_pP->peer_ip, s10_context_request_pP->trxn, CONTEXT_NOT_FOUND);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 OAILOG_INFO(LOG_MME_APP, "Received a CONTEXT_REQUEST for new UE with GUTI" GUTI_FMT ". \n", GUTI_ARG(&s10_context_request_pP->old_guti));

 /** Check that NAS/EMM context is existing. */
 emm_data_context_t *ue_nas_ctx = emm_data_context_get_by_guti(&_emm_data, &s10_context_request_pP->old_guti);
 if (!ue_nas_ctx) {
   OAILOG_ERROR(LOG_MME_APP, "A NAS EMM context is not existing for this GUTI "GUTI_FMT " already exists. \n", GUTI_ARG(&s10_context_request_pP->old_guti));
   _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, &s10_context_request_pP->peer_ip, s10_context_request_pP->trxn, CONTEXT_NOT_FOUND);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }

 /** Check if a UE session pool exists. */
 ue_session_pool_t * ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, ue_context->privates.mme_ue_s1ap_id);
 if (ue_context == NULL) {
    /** No UE session was found. */
    OAILOG_ERROR (LOG_MME_APP, "No UE session was found for UE " MME_UE_S1AP_ID_FMT". Rejecting context request.\n", ue_context->privates.mme_ue_s1ap_id);
    _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, &s10_context_request_pP->peer_ip, s10_context_request_pP->trxn, CONTEXT_NOT_FOUND);
    OAILOG_FUNC_OUT (LOG_MME_APP);
 }

// if(1){
//   _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, &s10_context_request_pP->peer_ip, s10_context_request_pP->trxn, CONTEXT_NOT_FOUND);	//	 _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, &s10_context_request_pP->peer_ip, s10_context_request_pP->trxn, CONTEXT_NOT_FOUND);
//   OAILOG_FUNC_OUT (LOG_MME_APP);
// }

 /** Check that a valid security context exists for the MME_UE_CONTEXT. */
 if (!IS_EMM_CTXT_PRESENT_SECURITY(ue_nas_ctx)) {
   OAILOG_ERROR(LOG_MME_APP, "A NAS EMM context is present but no security context is existing for this GUTI "GUTI_FMT ". \n", GUTI_ARG(&s10_context_request_pP->old_guti));
   _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, &s10_context_request_pP->peer_ip, s10_context_request_pP->trxn, CONTEXT_NOT_FOUND);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** Check that the UE is registered. Due to some errors in the RRC, it may be idle or connected. Don't know. */
 if (UE_REGISTERED != ue_context->privates.fields.mm_state) { /**< Should also mean EMM_REGISTERED. */
   /** UE may be in idle mode or it may be detached. */
   OAILOG_ERROR(LOG_MME_APP, "UE NAS EMM context is not in UE_REGISTERED state for GUTI "GUTI_FMT ". \n", GUTI_ARG(&s10_context_request_pP->old_guti));
   _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, &s10_context_request_pP->peer_ip, s10_context_request_pP->trxn, REQUEST_REJECTED);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }

 if(ue_context->privates.fields.ecm_state != ECM_IDLE){
   OAILOG_WARNING(LOG_MME_APP, "UE NAS EMM context is NOT in ECM_IDLE state for GUTI "GUTI_FMT ". Continuing with processing of S10 Context Request. \n", GUTI_ARG(&s10_context_request_pP->old_guti));
 }
 /** Check if there is any S10 procedure existing. */
 s10_proc_mme_tau = mme_app_get_s10_procedure_mme_handover(ue_context);
 if(s10_proc_mme_tau){
   OAILOG_WARNING (LOG_MME_APP, "EMM context for UE with ue_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " in EMM_REGISTERED state has a running S10 procedure. "
         "Rejecting further procedures. \n", ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.imsi);
   _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, &s10_context_request_pP->peer_ip, s10_context_request_pP->trxn, REQUEST_REJECTED);
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
  * We won't stop the timer and restart it. The timer for idle TAU will run out.
  */
 s10_proc_mme_tau = mme_app_create_s10_procedure_mme_handover(ue_context, false, MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER, &s10_context_request_pP->peer_ip);
 DevAssert(s10_proc_mme_tau);
 s10_proc_mme_tau->due_tau = true;

 /** S10 */
 tai_t target_tai;
 memset(&target_tai, 0, sizeof (tai_t));
 s10_proc_mme_tau->target_tai.plmn.mcc_digit1 = s10_context_request_pP->serving_network.mcc[0];
 s10_proc_mme_tau->target_tai.plmn.mcc_digit2 = s10_context_request_pP->serving_network.mcc[1];
 s10_proc_mme_tau->target_tai.plmn.mcc_digit3 = s10_context_request_pP->serving_network.mcc[2];
 s10_proc_mme_tau->target_tai.plmn.mnc_digit1 = s10_context_request_pP->serving_network.mnc[0];
 s10_proc_mme_tau->target_tai.plmn.mnc_digit2 = s10_context_request_pP->serving_network.mnc[1];
 s10_proc_mme_tau->target_tai.plmn.mnc_digit3 = s10_context_request_pP->serving_network.mnc[2];
 /** Not setting the TAI. */

 /** No S10 Procedure running. */
 // todo: validate NAS message!
// rc = emm_data_context_validate_complete_nas_request(ue_nas_ctx, &s10_context_request_pP->complete_request_message);
// if(rc != RETURNok){
//   OAILOG_ERROR(LOG_MME_APP, "UE NAS message for IMSI " IMSI_64_FMT " could not be validated. \n", ue_context->privates.fields.imsi);
//   _mme_app_send_s10_context_response_err(s10_context_request_pP->s10_target_mme_teid.teid, s10_context_request_pP->s10_target_mme_teid.ipv4, s10_context_request_pP->trxn, REQUEST_REJECTED);
//   OAILOG_FUNC_OUT (LOG_MME_APP);
// }

 /**
  * Destroy the message finally
  * todo: check what if already destroyed.
  */
 /** Prepare the S10 CONTEXT_RESPONSE. */
 message_p = itti_alloc_new_message (TASK_MME_APP, S10_CONTEXT_RESPONSE);
 DevAssert (message_p != NULL);
 itti_s10_context_response_t *context_response_p = &message_p->ittiMsg.s10_context_response;

 /** Set the target S10 TEID. */
 context_response_p->teid    = s10_context_request_pP->s10_target_mme_teid.teid; /**< Only a single target-MME TEID can exist at a time. */
 memcpy((void*)&context_response_p->peer_ip, &s10_context_request_pP->peer_ip,
		  (s10_context_request_pP->peer_ip.sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

 context_response_p->trxnId    = s10_context_request_pP->trxn;
 /** Set the cause. Since the UE context state has not been changed yet, nothing to do in the context if success or failure.*/
 context_response_p->cause.cause_value = REQUEST_ACCEPTED; // todo: check the NAS message here!

 if(context_response_p->cause.cause_value == REQUEST_ACCEPTED){
   /** Set the Source MME_S10_FTEID the same as in S11. */
   OAI_GCC_DIAG_OFF(pointer-to-int-cast);
   context_response_p->s10_source_mme_teid.teid = (teid_t) ue_context; /**< This one also sets the context pointer. */
   OAI_GCC_DIAG_ON(pointer-to-int-cast);
   context_response_p->s10_source_mme_teid.interface_type = S10_MME_GTP_C;

   /** Set the MME IPv4 and IPv6 addresses in the FTEID. */
   mme_config_read_lock (&mme_config);
   if(mme_config.ip.s11_mme_v4.s_addr){
	   context_response_p->s10_source_mme_teid.ipv4_address = mme_config.ip.s11_mme_v4;
	   context_response_p->s10_source_mme_teid.ipv4 = 1;
   }
   if(memcmp(&mme_config.ip.s11_mme_v6.s6_addr, (void*)&in6addr_any, sizeof(mme_config.ip.s11_mme_v6.s6_addr)) != 0) {
	   memcpy(context_response_p->s10_source_mme_teid.ipv6_address.s6_addr, mme_config.ip.s11_mme_v6.s6_addr,  sizeof(mme_config.ip.s11_mme_v6.s6_addr));
	   context_response_p->s10_source_mme_teid.ipv6 = 1;
   }
   mme_config_unlock (&mme_config);

   /**
    * Update the local_s10_key.
    * Not setting the key directly in the  ue_context structure. Only over this function!
    */
   mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
       ue_context->privates.enb_s1ap_id_key,
       ue_context->privates.mme_ue_s1ap_id,
       ue_context->privates.fields.imsi,
       ue_context->privates.fields.mme_teid_s11,       // mme_s11_teid is new
       context_response_p->s10_source_mme_teid.teid,       // set with forward_relocation_request // s10_context_response!
       &ue_context->privates.fields.guti);

   pdn_context_t * first_pdn = RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
   DevAssert(first_pdn);

   /** Set the S11 Source SAEGW FTEID. */
   OAI_GCC_DIAG_OFF(pointer-to-int-cast);
   context_response_p->s11_sgw_teid.teid = first_pdn->s_gw_teid_s11_s4;
   OAI_GCC_DIAG_ON(pointer-to-int-cast);
   context_response_p->s11_sgw_teid.interface_type = S11_MME_GTP_C;

   /** Set the MME IPv4 and IPv6 addresses in the FTEID. */
   if(first_pdn->s_gw_addr_s11_s4.ipv4_addr.sin_addr.s_addr){
	   context_response_p->s11_sgw_teid.ipv4_address = first_pdn->s_gw_addr_s11_s4.ipv4_addr.sin_addr;
	   context_response_p->s11_sgw_teid.ipv4 = 1;
   }
   if(memcmp(&first_pdn->s_gw_addr_s11_s4.ipv6_addr.sin6_addr.s6_addr, (void*)&in6addr_any, sizeof(first_pdn->s_gw_addr_s11_s4.ipv6_addr.sin6_addr.s6_addr)) != 0) {
	   memcpy(context_response_p->s11_sgw_teid.ipv6_address.s6_addr, first_pdn->s_gw_addr_s11_s4.ipv6_addr.sin6_addr.s6_addr,  sizeof(first_pdn->s_gw_addr_s11_s4.ipv6_addr.sin6_addr.s6_addr));
	   context_response_p->s11_sgw_teid.ipv6 = 1;
   }

   /** IMSI. */
   memcpy((void*)&context_response_p->imsi, &ue_nas_ctx->_imsi, sizeof(imsi_t));

   /** Set the PDN_CONNECTION IE. */
   context_response_p->pdn_connections = calloc(1, sizeof(mme_ue_eps_pdn_connections_t));
   mme_app_set_pdn_connections(context_response_p->pdn_connections, ue_session_pool);

   /** Set the MM_UE_EPS_CONTEXT. */
   context_response_p->ue_eps_mm_context = calloc(1, sizeof(mm_context_eps_t));
   mme_app_set_ue_eps_mm_context(context_response_p->ue_eps_mm_context, ue_context, ue_session_pool, ue_nas_ctx);

   /*
    * Start timer to wait the handover/TAU procedure to complete.
    * A Clear_Location_Request message received from the HSS will cause the resources to be removed.
    * Resources will not be removed if that is not received. --> TS.23.401 defines for SGSN "remove after CLReq" explicitly).
    */
 }else{
   /**
    * No source-MME (local) FTEID needs to be set. No tunnel needs to be established.
    * Just respond with the error cause to the given target-MME FTEID.
    */
 }
 /** Without interacting with NAS, directly send the S10_CONTEXT_RESPONSE message. */
 OAILOG_INFO(LOG_MME_APP, "Allocated S10_CONTEXT_RESPONSE MESSAGE for UE with IMSI " IMSI_64_FMT " and mmeUeS1apId " MME_UE_S1AP_ID_FMT " with error cause %d. \n",
     ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id, context_response_p->cause);

 MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME,  MSC_S11_MME ,
     NULL, 0, "0 S10_CONTEXT_RESPONSE for UE %d is sent. \n", ue_context->privates.mme_ue_s1ap_id);
 itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
 /** Send just S10_CONTEXT_RESPONSE. Currently not waiting for the S10_CONTEXT_ACKNOWLEDGE and nothing done if it does not arrive (no timer etc.). */
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//-------------------------------------------------------------------------------------------
pdn_context_t * mme_app_handle_pdn_connectivity_from_s10(ue_session_pool_t * ue_session_pool, pdn_connection_t * pdn_connection){

  OAILOG_FUNC_IN (LOG_MME_APP);

  context_identifier_t    context_identifier = 0; // todo: how is this set via S10?
  pdn_cid_t               pdn_cid = 0;
  pdn_context_t          *pdn_context = NULL;
  int                     rc = RETURNok;

  /** Get and handle the PDN Connection element as pending PDN connection element (using the default_ebi and the apn). */
  mme_app_get_pdn_context(ue_session_pool->privates.mme_ue_s1ap_id, PDN_CONTEXT_IDENTIFIER_UNASSIGNED, pdn_connection->linked_eps_bearer_id, pdn_connection->apn_str, &pdn_context);
  if(pdn_context){
    /* Found the PDN context. */
    OAILOG_ERROR(LOG_MME_APP, "PDN context for apn %s and default ebi %d already exists for UE_ID: " MME_UE_S1AP_ID_FMT". Skipping the establishment (or update). \n",
        bdata(pdn_connection->apn_str), pdn_connection->linked_eps_bearer_id, ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
  }
  /*
   * Create new PDN connection.
   * No context identifier will be set.
   * Later set context identifier by ULA?
   */
  /** Create a PDN connection with a default bearer and later set the ESM values when NAS layer is established. */
  pdn_type_value_t pdn_type = (((pdn_connection->ipv4_address.s_addr) ? 1 : 0) +
		  ((memcmp(&pdn_connection->ipv6_address, &in6addr_any, sizeof(in6addr_any)) != 0) ? (1 << 1) : 0) -1);

  // todo: store the received IPs
  if(mme_app_esm_create_pdn_context(ue_session_pool->privates.mme_ue_s1ap_id, pdn_connection->linked_eps_bearer_id,
		  NULL, pdn_connection->apn_str, PDN_CONTEXT_IDENTIFIER_UNASSIGNED, &pdn_connection->apn_ambr, pdn_type, &pdn_context) == RETURNerror){ /**< Create the pdn context using the APN network identifier. */
    OAILOG_ERROR(LOG_MME_APP, "Could not create a new pdn context for apn \" %s \" for UE_ID " MME_UE_S1AP_ID_FMT " from S10 PDN_CONNECTIONS IE. "
        "Skipping the establishment of pdn context. \n", bdata(pdn_connection->apn_str), ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
  }
  pdn_context_t * pdn_test = NULL;
  mme_app_get_pdn_context(ue_session_pool->privates.mme_ue_s1ap_id, ue_session_pool->privates.fields.next_def_ebi_offset + PDN_CONTEXT_IDENTIFIER_UNASSIGNED - 1, ue_session_pool->privates.fields.next_def_ebi_offset + 5 -1 , pdn_connection->apn_str, &pdn_test);
  DevAssert(pdn_test);
  /** Create and finalize the remaining bearer contexts. */
  for (int num_bearer = 0; num_bearer < pdn_connection->bearer_context_list.num_bearer_context; num_bearer++){
    bearer_context_to_be_created_t * bearer_context_to_be_created_s10 = &pdn_connection->bearer_context_list.bearer_context[num_bearer];
    /*
     * Create bearer contexts in the PDN context, only for dedicated bearers.
     * Since no activation in the ESM is necessary, set them as active.
     */
    bearer_context_new_t * bearer_context_registered = NULL;
    esm_cause_t esm_cause = ESM_CAUSE_SUCCESS;
    if(bearer_context_to_be_created_s10->eps_bearer_id != pdn_context->default_ebi){
      esm_cause = mme_app_register_dedicated_bearer_context(ue_session_pool->privates.mme_ue_s1ap_id, ESM_EBR_ACTIVE, pdn_context->context_identifier,
    		  pdn_context->default_ebi, bearer_context_to_be_created_s10, bearer_context_to_be_created_s10->eps_bearer_id);
    } else {
      /** Finalize the default bearer, updating qos. */
      esm_cause = mme_app_finalize_bearer_context(ue_session_pool->privates.mme_ue_s1ap_id, pdn_context->context_identifier,
          pdn_context->default_ebi, bearer_context_to_be_created_s10->eps_bearer_id,
          NULL, &bearer_context_to_be_created_s10->bearer_level_qos, NULL, NULL);
    }
    /** Finalize it, thereby set the qos values. */
    if(esm_cause != ESM_CAUSE_SUCCESS){
      OAILOG_ERROR(LOG_MME_APP, "Error while preparing bearer (ebi=%d) for APN \"%s\" for UE " MME_UE_S1AP_ID_FMT" received via handover/TAU. "
          "Currently ignoring (assuming implicit detach). \n", bearer_context_to_be_created_s10->eps_bearer_id, bdata(pdn_context->apn_subscribed), ue_session_pool->privates.mme_ue_s1ap_id);
    }
  }
  // todo: apn restriction data!
  OAILOG_INFO (LOG_MME_APP, "Successfully updated the MME_APP UE context with the pending pdn information for UE id  %d. \n", ue_session_pool->privates.mme_ue_s1ap_id);
  OAILOG_FUNC_RETURN(LOG_MME_APP, pdn_context);
}

//------------------------------------------------------------------------------
void
mme_app_handle_s10_context_response(
    itti_s10_context_response_t* const s10_context_response_pP
    )
{
  struct ue_context_s                    *ue_context = NULL;
  struct ue_session_pool_s               *ue_session_pool = NULL;
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

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, ue_context->privates.mme_ue_s1ap_id);
  /** Check that the UE_CONTEXT exists for the S10_FTEID. */
  if (ue_session_pool == NULL) { /**< If no ue_session_pool found, all tunnels are assumed to be cleared and not tunnels established when S10_CONTEXT_RESPONSE is received. */
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this UE session pool: " MME_UE_S1AP_ID_FMT". \n", ue_context->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  /** Get the NAS layer MME relocation procedure. */
  emm_data_context_t * emm_context = emm_data_context_get(&_emm_data, ue_context->privates.mme_ue_s1ap_id);
  /* Check if there is an MME relocation procedure running. */
  nas_ctx_req_proc_t * emm_cn_proc_ctx_req = get_nas_cn_procedure_ctx_req(emm_context);

  if(!emm_cn_proc_ctx_req){
    OAILOG_WARNING(LOG_MME_APP, "A NAS CN context request procedure is not active for UE " MME_UE_S1AP_ID_FMT " in state %d. "
        "Dropping newly received S10 Context Response. \n.",
        ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.mm_state);
    /*
     * The received message should be removed automatically.
     * todo: the message will have a new transaction, removing it not that it causes an assert?
     */
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  OAILOG_INFO(LOG_MME_APP, "NAS CN context request procedure is active. "
      "Processing the newly received context response for ueId " MME_UE_S1AP_ID_FMT". \n", ue_context->privates.mme_ue_s1ap_id);

  /** Check the cause first, before parsing the IMSI. */
  if(s10_context_response_pP->cause.cause_value != REQUEST_ACCEPTED){
    OAILOG_ERROR(LOG_MME_APP, "Received an erroneous cause  value %d for S10 Context Request for UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT ". "
        "Rejecting attach/tau & implicit detach. \n", s10_context_response_pP->cause.cause_value, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.mm_state);
    _mme_app_send_nas_context_response_err(ue_context->privates.mme_ue_s1ap_id, s10_context_response_pP->cause.cause_value);
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
      s10_context_response_pP->teid, ue_context->privates.fields.imsi);
//  /**
//   * Check that the UE_Context is in correct state.
//   */
//  if(ue_context->privates.fields.mm_state != UE_UNREGISTERED){ /**< Should be in UNREGISTERED state, else nothing to be done in the source MME side, just send a reject back and detch the UE. */
//    /** Deal with the error case. */
//    OAILOG_ERROR(LOG_MME_APP, "UE MME context with IMSI " IMSI_64_FMT " and mmeS1apUeId %d is not in UNREGISTERED state but instead %d. "
//        "Doing an implicit detach (todo: currently ignoring). \n",
//        ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.mm_state);
//    _mme_app_send_nas_context_response_err(ue_context->privates.mme_ue_s1ap_id, REQUEST_REJECTED);
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
  s10_context_ack_p->trxnId       = s10_context_response_pP->trxnId;
//  s10_context_ack_p->peer_ip    = s10_context_response_pP->peer_ip;
  memcpy((void*)&s10_context_ack_p->peer_ip, &s10_context_response_pP->peer_ip,
		  (s10_context_response_pP->peer_ip.sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

  s10_context_ack_p->peer_port  = s10_context_response_pP->peer_port;
  s10_context_ack_p->teid       = s10_context_response_pP->s10_source_mme_teid.teid;
  s10_context_ack_p->local_teid = ue_context->privates.fields.local_mme_teid_s10;
  s10_context_ack_p->local_port = s10_context_response_pP->local_port;

  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_S10_MME, NULL, 0, "0 S10 CONTEXT_ACK for UE " MME_UE_S1AP_ID_FMT "! \n", ue_context->privates.mme_ue_s1ap_id);
  itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
  OAILOG_INFO(LOG_MME_APP, "Sent S10 Context Acknowledge to the source MME FTEID " TEID_FMT " for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT ". \n",
      ue_context->privates.mme_ue_s1ap_id, s10_context_ack_p->teid);

  // todo: LOCK!!
  /** Set the subscribed AMBR values. */
  ue_session_pool->privates.fields.subscribed_ue_ambr.br_dl = s10_context_response_pP->ue_eps_mm_context->subscribed_ue_ambr.br_dl;
  ue_session_pool->privates.fields.subscribed_ue_ambr.br_ul = s10_context_response_pP->ue_eps_mm_context->subscribed_ue_ambr.br_ul;

  /*
   * Update the coll_keys with the IMSI and remove the S10 Tunnel Endpoint.
   */
  mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
      ue_context->privates.enb_s1ap_id_key,
      ue_context->privates.mme_ue_s1ap_id,
	  imsi,      /**< New IMSI. */
      ue_context->privates.fields.mme_teid_s11,
      INVALID_TEID,
      &ue_context->privates.fields.guti);


  /** Check if another UE context with the given IMSI exists. */
  ue_context_t * ue_context_old = mme_ue_context_exists_imsi(&mme_app_desc.mme_ue_contexts, imsi);
  if(ue_context_old && (ue_context->privates.mme_ue_s1ap_id != ue_context_old->privates.mme_ue_s1ap_id)){
    OAILOG_ERROR(LOG_MME_APP, "An old UE context already exists with ueId " MME_UE_S1AP_ID_FMT " for the UE with IMSI " IMSI_64_FMT ". Rejecting NAS context request procedure. \n",
        ue_context_old->privates.mme_ue_s1ap_id, ue_context_old->privates.fields.imsi);

    /** Reject the TAU of the new UE context and implictly detach the old context. */
    _mme_app_send_nas_context_response_err(ue_context->privates.mme_ue_s1ap_id, REQUEST_REJECTED);

    /*
     * Let the timeout happen for the new UE context. Will discard this information and continue with normal identification procedure.
     * Meanwhile remove the old UE.
     */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context_old->privates.mme_ue_s1ap_id; /**< Rest won't be sent, so no NAS Detach Request will be sent. */
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_OUT (LOG_MME_APP);
    /** Waiting timeout for detaching the new UE context. */
  }
//  sleep(100);

  memcpy((void*)&emm_cn_proc_ctx_req->nas_s10_context._imsi, &s10_context_response_pP->imsi, sizeof(imsi_t));

  emm_cn_proc_ctx_req->nas_s10_context.mm_eps_ctx = s10_context_response_pP->ue_eps_mm_context;
  s10_context_response_pP->ue_eps_mm_context = NULL;

  /** Set the target side S10 information. */
  memcpy((void*)&emm_cn_proc_ctx_req->remote_mme_teid, (void*)&s10_context_response_pP->s10_source_mme_teid, sizeof(fteid_t));

  /** Copy the pdn connections also into the emm_cn procedure. */
  emm_cn_proc_ctx_req->pdn_connections = s10_context_response_pP->pdn_connections;
  /** Unlink. */
  s10_context_response_pP->pdn_connections = NULL;

  /** Handle PDN Connections. */
  OAILOG_INFO(LOG_MME_APP, "For ueId " MME_UE_S1AP_ID_FMT " processing the pdn_connections (continuing with CSR). \n", ue_context->privates.mme_ue_s1ap_id);
  /** Process PDN Connections IE. Will initiate a Create Session Request message for the pending pdn_connections. */
  pdn_connection_t * pdn_connection = &emm_cn_proc_ctx_req->pdn_connections->pdn_connection[emm_cn_proc_ctx_req->pdn_connections->num_processed_pdn_connections];
  pdn_context_t * pdn_context = mme_app_handle_pdn_connectivity_from_s10(ue_session_pool, pdn_connection);
  if(pdn_context){
	  /*
	   * When Create Session Response is received, continue to process the next PDN connection, until all are processed.
	   * When all pdn_connections are completed, continue with handover request.
	   */
	  mme_app_send_s11_create_session_req (ue_context->privates.mme_ue_s1ap_id, &s10_context_response_pP->imsi, pdn_context, &emm_context->originating_tai, pdn_context->pco, true);
	  OAILOG_INFO(LOG_MME_APP, "Successfully sent CSR for UE " MME_UE_S1AP_ID_FMT ". Waiting for CSResp to continue to process s10 context response on target MME side. \n", ue_context->privates.mme_ue_s1ap_id);
	  /*
	   * Use the received PDN connectivity information to update the session/bearer information with the PDN connections IE, before informing the NAS layer about the context.
	   * Not performing state change. The MME_APP UE context will stay in the same state.
	   * State change will be handled by EMM layer.
	   */
  }
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
      s10_context_acknowledge_pP->teid, ue_context->privates.fields.imsi);
  /** Check the cause. */
  if(s10_context_acknowledge_pP->cause.cause_value != REQUEST_ACCEPTED){
    OAILOG_ERROR(LOG_MME_APP, "The S10 Context Acknowledge for local teid " TEID_FMT " was not valid/could not be received. "
        "Ignoring the handover state. \n", s10_context_acknowledge_pP->teid);
    // todo: what to do in this case? Ignoring the S6a cancel location request?
  }

  /** The S10 Tunnel endpoint will be removed with the completion of the MME-Mobility Completion timer of the procedure. */
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

 imsi64 = imsi_to_imsi64(&relocation_cancel_request_pP->imsi);

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
// relocation_cancel_response_p->peer_ip		  = relocation_cancel_request_pP->peer_ip; /**< Value should be safely allocated in the transaction.. */
 relocation_cancel_response_p->trxn           = relocation_cancel_request_pP->trxn;
 if(!ue_context){
   OAILOG_ERROR (LOG_MME_APP, "We didn't find this UE in list of UE: " IMSI_64_FMT". \n", imsi64);
   /** Send a relocation cancel response with 0 TEID if remote TEID is not set yet (although spec says otherwise). It may may not be that a transaction might be removed. */
   relocation_cancel_response_p->cause.cause_value = IMSI_IMEI_NOT_KNOWN;
   itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** Check if there is a handover process. */
 mme_app_s10_proc_mme_handover_t * s10_handover_process = mme_app_get_s10_procedure_mme_handover(ue_context);
 if(!s10_handover_process){
   OAILOG_WARNING(LOG_MME_APP, "No S10 Handover Process is ongoing for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT ". Ignoring Relocation Cancle Request (ack). \n", ue_context->privates.mme_ue_s1ap_id);
   relocation_cancel_response_p->cause.cause_value = REQUEST_ACCEPTED;
   itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 relocation_cancel_response_p->teid = s10_handover_process->remote_mme_teid.teid; /**< Only a single target-MME TEID can exist at a time. */
 /** An EMM context may not exist yet. */
 emm_data_context_t * ue_nas_ctx = emm_data_context_get_by_imsi(&_emm_data, imsi64);
 if(ue_nas_ctx){
   OAILOG_ERROR (LOG_MME_APP, "An EMM Data Context already exists for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT ". \n", ue_context->privates.mme_ue_s1ap_id, imsi64);
   /*
    * Since RELOCATION_CANCEL_REQUEST is already sent when S10 Handover Cancellation is performed. If we have a NAS context, it should be an invalidated one,
    * already purging itself.
    */
   if(ue_context->privates.s1_ue_context_release_cause != S1AP_INVALIDATE_NAS){
     OAILOG_ERROR (LOG_MME_APP, "The EMM Data Context is not invalidated. We will disregard the RELOCATION_CANCEL_REQUEST for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT ". \n", ue_context->privates.mme_ue_s1ap_id, imsi64);
     relocation_cancel_response_p->cause.cause_value = SYSTEM_FAILURE;
     itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
     OAILOG_FUNC_OUT (LOG_MME_APP);
   }
   /*
    * Continue with the cancellation procedure! Will overwrite the release cause that the UE Context is also removed.
    */
   OAILOG_WARNING (LOG_MME_APP, "The EMM Data Context is INVALIDATED. We will continue the RELOCATION_CANCEL_REQUEST for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT ". \n", ue_context->privates.mme_ue_s1ap_id, imsi64);
 }
 /**
  * Lastly, check if the UE is already REGISTERED in the TARGET cell (here).
  * In that case, its too late to remove for HO-CANCELLATION.
  * Assuming that it is an error on the source side.
  */
 if(ue_context->privates.fields.mm_state == UE_REGISTERED){
   OAILOG_ERROR (LOG_MME_APP, "UE Context/EMM Data Context for IMSI " IMSI_64_FMT " and mmeUeS1apId " MME_UE_S1AP_ID_FMT " is already REGISTERED. "
       "Not purging due to RELOCATION_CANCEL_REQUEST. \n", imsi64, ue_context->privates.mme_ue_s1ap_id);
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
 ue_context->privates.s1_ue_context_release_cause = S1AP_HANDOVER_CANCELLED;
 /**
  * Respond with a Relocation Cancel Response (without waiting for the S10-Triggered detach to complete).
  */
 relocation_cancel_response_p->cause.cause_value = REQUEST_ACCEPTED;
 itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);

 /** Delete the handover procedure. */
 mme_app_delete_s10_procedure_mme_handover(ue_context);

 /** Send an EMM/ESM implicit detach message. */
 message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
 DevAssert (message_p != NULL);
 message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->privates.mme_ue_s1ap_id; /**< Rest won't be sent, so no NAS Detach Request will be sent. */
 MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
 itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);

 /** Trigger an ESM detach, also removing all PDN contexts in the MME and the SAE-GW. */
 message_p = itti_alloc_new_message (TASK_MME_APP, NAS_ESM_DETACH_IND);
 DevAssert (message_p != NULL);
 message_p->ittiMsg.nas_esm_detach_ind.ue_id = ue_context->privates.mme_ue_s1ap_id; /**< We don't send a Detach Type such that no Detach Request is sent to the UE if handover is performed. */

 MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_ESM_DETACH_IND");
 itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
 OAILOG_FUNC_OUT (LOG_MME_APP);

// /**
//  * Send a S1AP Context Release Request.
//  * If no S1AP UE reference is existing, we will send a UE context release command with the MME_UE_S1AP_ID.
//  * todo: macro/home
//  */
// mme_app_itti_ue_context_release(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.s1_ue_context_release_cause, s10_handover_process->target_id.target_id.macro_enb_id.enb_id);

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
       ue_context->privates.mme_ue_s1ap_id, relocation_cancel_response_pP->cause);
 }else{
   OAILOG_INFO(LOG_MME_APP, "RELOCATION_CANCEL_REPONSE was accepted at TARGET-MME side for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT ". \n", ue_context->privates.mme_ue_s1ap_id);
 }
 /** Check if there is a S10 Handover Procedure. Remove it if exists. */
 mme_app_delete_s10_procedure_mme_handover(ue_context);
 /** Send Cancel Ack back. */
 mme_app_send_s1ap_handover_cancel_acknowledge(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.fields.sctp_assoc_id_key);

 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//-------------------------------------------------------------------------
void mme_app_set_pdn_connections(struct mme_ue_eps_pdn_connections_s * pdn_connections, struct ue_session_pool_s * ue_session_pool){

  OAILOG_FUNC_IN (LOG_MME_APP);
  pdn_context_t *pdn_context_to_forward = NULL;

  DevAssert(ue_session_pool);
  DevAssert(pdn_connections);

  /** Set the PDN connections for all available PDN contexts. */
  RB_FOREACH (pdn_context_to_forward, PdnContexts, &ue_session_pool->pdn_contexts) {
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

    /** Set the PGW soure side CP IP addresses. */
    if(pdn_context_to_forward->s_gw_addr_s11_s4.ipv4_addr.sin_addr.s_addr){
    	pdn_connections->pdn_connection[num_pdn].pgw_address_for_cp.ipv4_address = pdn_context_to_forward->s_gw_addr_s11_s4.ipv4_addr.sin_addr;
    	pdn_connections->pdn_connection[num_pdn].pgw_address_for_cp.ipv4 = 1;
    }
    if(memcmp(&pdn_context_to_forward->s_gw_addr_s11_s4.ipv6_addr.sin6_addr.s6_addr, (void*)&in6addr_any, sizeof(pdn_context_to_forward->s_gw_addr_s11_s4.ipv6_addr.sin6_addr.s6_addr)) != 0) {
    	memcpy(pdn_connections->pdn_connection[num_pdn].pgw_address_for_cp.ipv6_address.s6_addr, pdn_context_to_forward->s_gw_addr_s11_s4.ipv6_addr.sin6_addr.s6_addr,  sizeof(pdn_context_to_forward->s_gw_addr_s11_s4.ipv6_addr.sin6_addr.s6_addr));
    	pdn_connections->pdn_connection[num_pdn].pgw_address_for_cp.ipv6 = 1;
    }

    /** APN Restriction. */
    pdn_connections->pdn_connection[num_pdn].apn_restriction = 0; // pdn_context_to_forward->apn_restriction
    /** APN-AMBR : forward the currently used apn ambr. The MM Context will have the UE AMBR. */
    pdn_connections->pdn_connection[num_pdn].apn_ambr.br_ul = pdn_context_to_forward->subscribed_apn_ambr.br_ul;
    pdn_connections->pdn_connection[num_pdn].apn_ambr.br_dl = pdn_context_to_forward->subscribed_apn_ambr.br_dl;
    /** Set the bearer contexts for all existing bearers of the PDN. */
    mme_app_get_bearer_contexts_to_be_created(pdn_context_to_forward, &pdn_connections->pdn_connection[num_pdn].bearer_context_list, BEARER_STATE_NULL);
    pdn_connections->num_pdn_connections++;
  }
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
/*
 *
 *  Name:    mme_app_mobility_complete()
 *
 *  Description: Notify the MME that a UE has successfully completed a mobility (handover/tau) procedure.
 *
 *  Inputs:
 *         ueid:      emm_context id
 *         bool:
 *  Return:    void
 *
 */
int mme_app_mobility_complete(const mme_ue_s1ap_id_t mme_ue_s1ap_id, bool activate_bearers){
  OAILOG_FUNC_IN (LOG_NAS);

  ue_context_t                           * ue_context = NULL;
  ue_session_pool_t                      * ue_session_pool = NULL;
  mme_app_s10_proc_mme_handover_t        * s10_handover_proc = NULL;

  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id);
  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, mme_ue_s1ap_id);
  DevAssert(ue_context);
  DevAssert(ue_session_pool);
  /** Get the handover procedure if exists. */
  if(((s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context)) != NULL)){
    activate_bearers = true;
    if(s10_handover_proc->proc.type == MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER){
      /** Complete the Intra Handover. */
      OAILOG_DEBUG(LOG_MME_APP, "UE MME context with imsi " IMSI_64_FMT " and mmeS1apUeId " MME_UE_S1AP_ID_FMT " has successfully completed intra-MME handover process after HANDOVER_NOTIFY. \n",
          ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id);
      /** For INTRA-MME handover trigger the timer mentioned in TS 23.401 to remove the UE Context and the old S1AP UE reference to the source eNB. */
      ue_description_t * old_ue_reference = s1ap_is_enb_ue_s1ap_id_in_list_per_enb(s10_handover_proc->source_enb_ue_s1ap_id, s10_handover_proc->source_ecgi.cell_identity.enb_id);
      if(old_ue_reference){
        /** For INTRA-MME handover, start the timer to remove the old UE reference here. No timer should be started for the S10 Handover Process. */
    	enb_s1ap_id_key_t enb_ue_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;
    	MME_APP_ENB_S1AP_ID_KEY(enb_ue_s1ap_id_key, s10_handover_proc->source_ecgi.cell_identity.enb_id, old_ue_reference->enb_ue_s1ap_id);
		if (timer_setup (mme_config.mme_mobility_completion_timer, 0,
            TASK_S1AP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, enb_ue_s1ap_id_key, &(old_ue_reference->s1ap_handover_completion_timer.id)) < 0) {
          OAILOG_ERROR (LOG_MME_APP, "Failed to start s1ap_mobility_completion timer for source eNB for enbUeS1apId " ENB_UE_S1AP_ID_FMT " for duration %d. "
              "Still continuing with MBR. \n",
              old_ue_reference->enb_ue_s1ap_id, mme_config.mme_mobility_completion_timer);
          old_ue_reference->s1ap_handover_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
        } else {
          OAILOG_DEBUG (LOG_MME_APP, "MME APP : Completed Handover Procedure at (source) MME side after handling S1AP_HANDOVER_NOTIFY. "
              "Activated the S1AP Handover completion timer enbUeS1apId " ENB_UE_S1AP_ID_FMT ". Removing source eNB resources after timer.. Timer Id %u. Timer duration %d \n",
              old_ue_reference->enb_ue_s1ap_id, old_ue_reference->s1ap_handover_completion_timer.id, mme_config.mme_mobility_completion_timer);
          /** MBR will be independent of this. */
        }
      }else{
        OAILOG_DEBUG(LOG_MME_APP, "No old UE_REFERENCE was found for mmeS1apUeId " MME_UE_S1AP_ID_FMT " and enbUeS1apId "ENB_UE_S1AP_ID_FMT ". Not starting a new timer. \n",
            ue_context->privates.fields.enb_ue_s1ap_id, s10_handover_proc->source_ecgi.cell_identity.enb_id);
        OAILOG_FUNC_OUT(LOG_MME_APP);
      }
      /*
       * Delete the handover procedure.
       * For multi-APN Intra MME handover, the MBResp should trigger further MBReqs.
       */
      mme_app_delete_s10_procedure_mme_handover(ue_context);
    }else{
      s10_handover_proc->handover_completed = true;
      /*
       * INTRA-MME Handover (INTER-MME idle TAU won't have this procedure).
       * If the UE is registered, remove the handover procedure.
       * If not, just continue with the MBR.
       */
      if(ue_context->privates.fields.mm_state == UE_REGISTERED){
        /** UE is registered, we completed the handover procedure completely. Deleting the procedure. */
        mme_app_delete_s10_procedure_mme_handover(ue_context);
        /*
         * Still need to check pending bearer deactivation.
         * If so, no MBR will be sent.
         */
      }else{
       /*
        * UE is not registered yet. Assuming handover notify arrived and TAU is expected. Continuing with the MBR.
        * Not removing the handover procedure.
        */
      }
    }
  }
  if(activate_bearers){
    /**
     * Use the MBR procedure to activate the bearers.
     * MBReq will be sent for only those bearers which are not in ACTIVE state yet, but are established in the target eNB (ENB_CREATED).
     */
    pdn_context_t * first_pdn = RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
    if(first_pdn){
    	OAILOG_INFO(LOG_MME_APP, " Triggering MBReq for completed handover for UE " MME_UE_S1AP_ID_FMT ". \n", mme_ue_s1ap_id);
        mme_app_send_s11_modify_bearer_req(ue_session_pool, first_pdn, 0);
    }
  }
  OAILOG_INFO(LOG_MME_APP, "Completed registration of UE " MME_UE_S1AP_ID_FMT ". \n", mme_ue_s1ap_id);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

////-------------------------------------------------------------------------------------------
//void mme_app_process_pdn_connection_ie(ue_context_t * ue_context, pdn_connection_t * pdn_connection){
//  OAILOG_FUNC_IN (LOG_MME_APP);
//
//  /** Update the PDN session information directly in the new UE_Context. */
//  pdn_context_t * pdn_context = mme_app_handle_pdn_connectivity_from_s10(ue_session_pool, &forward_relocation_request_pP->pdn_connections.pdn_connection[num_pdn]);
//    if(pdn_context){
//      OAILOG_INFO (LOG_MME_APP, "Successfully updated the PDN connections for ueId " MME_UE_S1AP_ID_FMT " for pdn %s. \n",
//          ue_context->privates.mme_ue_s1ap_id, forward_relocation_request_pP->pdn_connections.pdn_connection[num_pdn].apn);
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
//        OAILOG_INFO(LOG_MME_APP, "UE_CONTEXT for UE " MME_UE_S1AP_ID_FMT " does not has a default bearer %d. Continuing with CSR. \n", ue_context->privates.mme_ue_s1ap_id, ue_context->default_bearer_id);
//        if(mme_app_send_s11_create_session_req(ue_context, pdn_context, &target_tai) != RETURNok) {
//          OAILOG_CRITICAL (LOG_MME_APP, "MME_APP_FORWARD_RELOCATION_REQUEST. Sending CSR to SAE-GW failed for UE " MME_UE_S1AP_ID_FMT ". \n", ue_context->privates.mme_ue_s1ap_id);
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
//            ue_context->privates.mme_ue_s1ap_id, forward_relocation_request_pP->pdn_connections.pdn_connection[num_pdn].apn);
//      }
//    }
//}



/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

// todo: UE context should already be locked : any way to check it?!?
//------------------------------------------------------------------------------
static void clear_ue_context(ue_context_t * ue_context) {
	OAILOG_FUNC_IN(LOG_MME_APP);

	DevAssert(ue_context != NULL);

	// todo: lock UE context
	bdestroy_wrapper (&ue_context->privates.fields.msisdn);
	bdestroy_wrapper (&ue_context->privates.fields.ue_radio_capability);
	bdestroy_wrapper (&ue_context->privates.fields.apn_oi_replacement);

	// Stop Mobile reachability timer,if running
	if (ue_context->privates.mobile_reachability_timer.id != MME_APP_TIMER_INACTIVE_ID) {
		if (timer_remove(ue_context->privates.mobile_reachability_timer.id, NULL)) {
			OAILOG_ERROR (LOG_MME_APP, "Failed to stop Mobile Reachability timer for UE id "MME_UE_S1AP_ID_FMT". \n", ue_context->privates.mme_ue_s1ap_id);
		}
		ue_context->privates.mobile_reachability_timer.id = MME_APP_TIMER_INACTIVE_ID;
	}
	// Stop Implicit detach timer,if running
	if (ue_context->privates.implicit_detach_timer.id != MME_APP_TIMER_INACTIVE_ID) {
		if (timer_remove(ue_context->privates.implicit_detach_timer.id, NULL)) {
			OAILOG_ERROR (LOG_MME_APP, "Failed to stop Implicit Detach timer for UE id "MME_UE_S1AP_ID_FMT". \n", ue_context->privates.mme_ue_s1ap_id);
		}
		ue_context->privates.implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;
	}
	// Stop Initial context setup process guard timer,if running
	if (ue_context->privates.initial_context_setup_rsp_timer.id != MME_APP_TIMER_INACTIVE_ID) {
		if (timer_remove(ue_context->privates.initial_context_setup_rsp_timer.id, NULL)) {
			OAILOG_ERROR (LOG_MME_APP, "Failed to stop Initial Context Setup Rsp timer for UE id "MME_UE_S1AP_ID_FMT". \n", ue_context->privates.mme_ue_s1ap_id);
		}
		ue_context->privates.initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
	}
	//  /** Reset the source MME handover timer. */
	//  if (ue_context->mme_mobility_completion_timer.id != MME_APP_TIMER_INACTIVE_ID) {
	//    if (timer_remove(ue_context->mme_mobility_completion_timer.id, NULL)) {
	//      OAILOG_ERROR (LOG_MME_APP, "Failed to stop MME Mobility Completion timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
	//    }
	//    ue_context->mme_mobility_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
	//  }
	//
	//  /** Reset the target MME handover timer. */
	//  if (ue_context->mme_s10_handover_completion_timer.id != MME_APP_TIMER_INACTIVE_ID) {
	//    if (timer_remove(ue_context->mme_s10_handover_completion_timer.id, NULL)) {
	//      OAILOG_ERROR (LOG_MME_APP, "Failed to stop MME S10 Handover Completion timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
	//    }
	//    ue_context->mme_s10_handover_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
	//  }
	ue_context->privates.s1_ue_context_release_cause = S1AP_INVALID_CAUSE;

	/** Will remove the S10 procedures and tunnel endpoints. */
	if (ue_context->s10_procedures) {
		mme_app_delete_s10_procedure_mme_handover(ue_context); // todo: generic s10 function
	}

	mme_ue_s1ap_id_t ue_id = ue_context->privates.mme_ue_s1ap_id;
	OAILOG_INFO(LOG_MME_APP, "Clearing UE context of UE "MME_UE_S1AP_ID_FMT ". \n", ue_id);
	ue_context->privates.mme_ue_s1ap_id = INVALID_MME_UE_S1AP_ID;
	ue_context->privates.enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;

	//  new_p->mme_s10_handover_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
	//  new_p->mme_mobility_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
	//  new_p->ue_radio_cap_length = 0;
	/** Initialize the fields. */
	memset(&ue_context->privates.fields, 0, sizeof(ue_context->privates.fields));
	OAILOG_INFO(LOG_MME_APP, "Successfully cleared UE context for UE "MME_UE_S1AP_ID_FMT ". \n", ue_id);
}

//------------------------------------------------------------------------------
static void release_ue_context(ue_context_t ** ue_context) {
	OAILOG_FUNC_IN (LOG_MME_APP);

	/** Clear the UE context. */
	OAILOG_INFO(LOG_MME_APP, "Releasing the UE context %p of UE " MME_UE_S1AP_ID_FMT".\n",
			*ue_context, (*ue_context)->privates.mme_ue_s1ap_id);

	mme_ue_s1ap_id_t ue_id = (*ue_context)->privates.mme_ue_s1ap_id;
	// todo: lock the mme_desc (ue_context should already be locked)
	/** Remove the ue_session pool from the list (probably at the back - must not be at the very end. */
	STAILQ_REMOVE(&mme_app_desc.mme_ue_contexts_list, (*ue_context), ue_context_s, entries);
	clear_ue_context(*ue_context);
	/** Put it into the head. */
	STAILQ_INSERT_HEAD(&mme_app_desc.mme_ue_contexts_list, (*ue_context), entries);
	*ue_context= NULL;
	// todo: unlock the mme_desc
}


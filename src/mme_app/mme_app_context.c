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
#include "mme_app_ue_context.h"
#include "mme_app_bearer_context.h"
#include "mme_app_defs.h"
#include "mme_app_itti_messaging.h"
#include "mme_app_procedures.h"
#include "s1ap_mme.h"
#include "common_defs.h"
#include "esm_ebr.h"

//------------------------------------------------------------------------------
ue_mm_context_t *mme_create_new_ue_context (void)
{
  ue_mm_context_t                           *new_p = calloc (1, sizeof (ue_mm_context_t));
  emm_init_context(&new_p->emm_context, true);
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
void mme_app_ue_context_free_content (ue_mm_context_t * const mme_ue_context_p)
{
  bdestroy_wrapper (&mme_ue_context_p->msisdn);
  bdestroy_wrapper (&mme_ue_context_p->ue_radio_capabilities);
  bdestroy_wrapper (&mme_ue_context_p->apn_oi_replacement);

  for (int i = 0; i < MAX_APN_PER_UE; i++) {
    if (mme_ue_context_p->pdn_contexts[i]) {
      mme_app_free_pdn_connection(&mme_ue_context_p->pdn_contexts[i]);
    }
  }

  for (int i = 0; i < BEARERS_PER_UE; i++) {
    if (mme_ue_context_p->bearer_contexts[i]) {
      mme_app_free_bearer_context(&mme_ue_context_p->bearer_contexts[i]);
    }
  }
  if (mme_ue_context_p->s11_procedures) {
    mme_app_delete_s11_procedures(mme_ue_context_p);
  }
  memset(mme_ue_context_p, 0, sizeof(*mme_ue_context_p));
}

//------------------------------------------------------------------------------
ue_mm_context_t                           *
mme_ue_context_exists_enb_ue_s1ap_id (
  mme_ue_context_t * const mme_ue_context_p,
  const enb_s1ap_id_key_t enb_key)
{
  struct ue_mm_context_s                    *ue_context_p = NULL;

  hashtable_ts_get (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)enb_key, (void **)&ue_context_p);
  if (ue_context_p) {
    OAILOG_TRACE (LOG_MME_APP, "Fetched ue_mm_context (key " MME_APP_ENB_S1AP_ID_KEY_FORMAT ") MME_UE_S1AP_ID=" MME_UE_S1AP_ID_FMT " ENB_UE_S1AP_ID=" ENB_UE_S1AP_ID_FMT " @%p\n",
        enb_key, ue_context_p->mme_ue_s1ap_id, ue_context_p->enb_ue_s1ap_id, ue_context_p);
  }
  return ue_context_p;
}

//------------------------------------------------------------------------------
ue_mm_context_t                           *
mme_ue_context_exists_mme_ue_s1ap_id (
  mme_ue_context_t * const mme_ue_context_p,
  const mme_ue_s1ap_id_t mme_ue_s1ap_id)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
  uint64_t                                enb_s1ap_id_key64 = 0;

  h_rc = hashtable_uint64_ts_get (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)mme_ue_s1ap_id, &enb_s1ap_id_key64);

  if (HASH_TABLE_OK == h_rc) {
    return mme_ue_context_exists_enb_ue_s1ap_id (mme_ue_context_p, (enb_s1ap_id_key_t) enb_s1ap_id_key64);
  }

  return NULL;
}
//------------------------------------------------------------------------------
struct ue_mm_context_s                    *
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
struct ue_mm_context_s                    *
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
ue_mm_context_t                           *
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
void mme_app_move_context (ue_mm_context_t *dst, ue_mm_context_t *src)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  if ((dst) && (src)) {
    OAILOG_DEBUG (LOG_MME_APP,
           "mme_app_move_context("ENB_UE_S1AP_ID_FMT " <- " ENB_UE_S1AP_ID_FMT ")\n",
           dst->enb_ue_s1ap_id, src->enb_ue_s1ap_id);
    mme_app_ue_context_free_content(dst);
    memcpy(dst, src, sizeof(*dst)); // dangerous (memory leaks), TODO check
    memset(src, 0, sizeof(*src));
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}
//------------------------------------------------------------------------------
// a duplicate ue_mm context can be detected only while receiving an INITIAL UE message
// moves old ue context content to new ue context
ue_mm_context_t *
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
        ue_mm_context_t                           *old = NULL;
        h_rc = hashtable_uint64_ts_remove (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)old_mme_ue_s1ap_id);
        h_rc = hashtable_uint64_ts_insert (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)old_mme_ue_s1ap_id, (uint64_t)new_enb_key);
        h_rc = hashtable_ts_remove (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)old_enb_key, (void **)&old);
        if (HASH_TABLE_OK == h_rc) {
          ue_mm_context_t                           *new = NULL;
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
        ue_mm_context_t                           *new = NULL;
        h_rc = hashtable_uint64_ts_remove (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)old_mme_ue_s1ap_id);
        h_rc = hashtable_uint64_ts_insert (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)old_mme_ue_s1ap_id, (uint64_t)new_enb_key);
        h_rc = hashtable_ts_remove (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)new_enb_key, (void **)&new);
        if (HASH_TABLE_OK != h_rc) {
          OAILOG_ERROR (LOG_MME_APP,"Could not remove new UE context new enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " \n",new_enb_ue_s1ap_id);
        }
        ue_mm_context_t                           *old = NULL;
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

//------------------------------------------------------------------------------
int
mme_ue_context_notified_new_ue_s1ap_id_association (
  const enb_s1ap_id_key_t  enb_key,
  const mme_ue_s1ap_id_t   mme_ue_s1ap_id)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
  ue_mm_context_t                           *ue_context_p = NULL;
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;

  OAILOG_FUNC_IN (LOG_MME_APP);
  enb_ue_s1ap_id = MME_APP_ENB_S1AP_ID_KEY2ENB_S1AP_ID(enb_key);

  if (INVALID_MME_UE_S1AP_ID == mme_ue_s1ap_id) {
    OAILOG_ERROR (LOG_MME_APP,
        "Error could not associate this enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
        enb_ue_s1ap_id, mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  ue_context_p = mme_ue_context_exists_enb_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, enb_key);
  if (ue_context_p) {
    if (ue_context_p->enb_s1ap_id_key == enb_key) { // useless
      if (INVALID_MME_UE_S1AP_ID == ue_context_p->mme_ue_s1ap_id) {
        // new insertion of mme_ue_s1ap_id, not a change in the id
        h_rc = hashtable_uint64_ts_insert (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)mme_ue_s1ap_id, (uint64_t)enb_key);
        if (HASH_TABLE_OK == h_rc) {
          ue_context_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
          OAILOG_DEBUG (LOG_MME_APP,
              "Associated this enb_ue_s1ap_ue_id " ENB_UE_S1AP_ID_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
              ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id);

          s1ap_notified_new_ue_mme_s1ap_id_association (ue_context_p->sctp_assoc_id_key, enb_ue_s1ap_id, mme_ue_s1ap_id);
          OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
        }
      }
    }
  }
  OAILOG_ERROR (LOG_MME_APP,
      "Error could not associate this enb_ue_s1ap_ue_id " ENB_UE_S1AP_ID_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
      enb_ue_s1ap_id, mme_ue_s1ap_id);
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
}
//------------------------------------------------------------------------------
void
mme_ue_context_update_coll_keys (
  mme_ue_context_t * const mme_ue_context_p,
  ue_mm_context_t     * const ue_context_p,
  const enb_s1ap_id_key_t  enb_s1ap_id_key,
  const mme_ue_s1ap_id_t   mme_ue_s1ap_id,
  const imsi64_t     imsi,
  const s11_teid_t         mme_teid_s11,
  const guti_t     * const guti_p)  //  never NULL, if none put &ue_context_p->nas_emm_context._guti
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;

  OAILOG_FUNC_IN(LOG_MME_APP);

  OAILOG_TRACE (LOG_MME_APP, "Update ue context.enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ue context.mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ue context.IMSI " IMSI_64_FMT " ue context.GUTI "GUTI_FMT"\n",
             ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id, ue_context_p->emm_context._imsi64, GUTI_ARG(&ue_context_p->emm_context._guti));

  if (guti_p) {
    OAILOG_TRACE (LOG_MME_APP, "Update ue context %p enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " GUTI " GUTI_FMT "\n",
            ue_context_p, ue_context_p->enb_ue_s1ap_id, mme_ue_s1ap_id, imsi, GUTI_ARG(guti_p));
  } else {
    OAILOG_TRACE (LOG_MME_APP, "Update ue context %p enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT "\n",
            ue_context_p, ue_context_p->enb_ue_s1ap_id, mme_ue_s1ap_id, imsi);
  }
  AssertFatal(ue_context_p->enb_s1ap_id_key == enb_s1ap_id_key,
      "Mismatch in UE context enb_s1ap_id_key "MME_APP_ENB_S1AP_ID_KEY_FORMAT"/"MME_APP_ENB_S1AP_ID_KEY_FORMAT"\n",
      ue_context_p->enb_s1ap_id_key, enb_s1ap_id_key);


  if (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id) {
    if (ue_context_p->mme_ue_s1ap_id != mme_ue_s1ap_id) {
      // new insertion of mme_ue_s1ap_id, not a change in the id
      h_rc = hashtable_uint64_ts_remove (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context_p->mme_ue_s1ap_id);
      h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)mme_ue_s1ap_id, (uint64_t)enb_s1ap_id_key);

      if (HASH_TABLE_OK != h_rc) {
        OAILOG_ERROR (LOG_MME_APP,
            "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " %s\n",
            ue_context_p, ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id, hashtable_rc_code2string(h_rc));
      }
      ue_context_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
    }

    if (INVALID_IMSI64 != ue_context_p->emm_context._imsi64) {
      h_rc = hashtable_uint64_ts_remove (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)ue_context_p->emm_context._imsi64);
      h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)ue_context_p->emm_context._imsi64, (uint64_t)mme_ue_s1ap_id);
    }
    h_rc = hashtable_uint64_ts_remove (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context_p->mme_teid_s11);
    h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context_p->mme_teid_s11, (uint64_t)mme_ue_s1ap_id);

    h_rc = obj_hashtable_uint64_ts_remove (mme_ue_context_p->guti_ue_context_htbl, (const void *const)&ue_context_p->emm_context._guti,
        sizeof (ue_context_p->emm_context._guti));
    if (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id) {
      h_rc = obj_hashtable_uint64_ts_insert (mme_ue_context_p->guti_ue_context_htbl, (const void *const)&ue_context_p->emm_context._guti,
          sizeof (ue_context_p->emm_context._guti), (uint64_t)mme_ue_s1ap_id);
    }
  }

  if ((ue_context_p->emm_context._imsi64 != imsi)
      || (ue_context_p->mme_ue_s1ap_id != mme_ue_s1ap_id)) {
    h_rc = hashtable_uint64_ts_remove (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)ue_context_p->emm_context._imsi64);
    if (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id) {
      h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)imsi, (uint64_t)mme_ue_s1ap_id);
    } else {
      h_rc = HASH_TABLE_KEY_NOT_EXISTS;
    }
    if (HASH_TABLE_OK != h_rc) {
      OAILOG_TRACE (LOG_MME_APP,
          "Error could not update this ue context %p enb_ue_s1ap_ue_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " imsi " IMSI_64_FMT ": %s\n",
          ue_context_p, ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id, imsi, hashtable_rc_code2string(h_rc));
    }
    ue_context_p->emm_context._imsi64 = imsi;
  }

  if ((ue_context_p->mme_teid_s11 != mme_teid_s11)
      || (ue_context_p->mme_ue_s1ap_id != mme_ue_s1ap_id)) {
    h_rc = hashtable_uint64_ts_remove (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context_p->mme_teid_s11);
    if (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id) {
      h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)mme_teid_s11, (uint64_t)mme_ue_s1ap_id);
    } else {
      h_rc = HASH_TABLE_KEY_NOT_EXISTS;
    }

    if (HASH_TABLE_OK != h_rc) {
      OAILOG_TRACE (LOG_MME_APP,
          "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " mme_teid_s11 " TEID_FMT " : %s\n",
          ue_context_p, ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id, mme_teid_s11, hashtable_rc_code2string(h_rc));
    }
    OAILOG_TRACE (LOG_MME_APP,
        "Update ue context.mme_teid_s11 " TEID_FMT " -> " TEID_FMT "\n", ue_context_p->mme_teid_s11, mme_teid_s11);
    ue_context_p->mme_teid_s11 = mme_teid_s11;
  }

  if (guti_p) {
    if ((guti_p->gummei.mme_code != ue_context_p->emm_context._guti.gummei.mme_code)
      || (guti_p->gummei.mme_gid != ue_context_p->emm_context._guti.gummei.mme_gid)
      || (guti_p->m_tmsi != ue_context_p->emm_context._guti.m_tmsi)
      || (guti_p->gummei.plmn.mcc_digit1 != ue_context_p->emm_context._guti.gummei.plmn.mcc_digit1)
      || (guti_p->gummei.plmn.mcc_digit2 != ue_context_p->emm_context._guti.gummei.plmn.mcc_digit2)
      || (guti_p->gummei.plmn.mcc_digit3 != ue_context_p->emm_context._guti.gummei.plmn.mcc_digit3)
      || (ue_context_p->mme_ue_s1ap_id != mme_ue_s1ap_id)) {

      // may check guti_p with a kind of instanceof()?
      h_rc = obj_hashtable_uint64_ts_remove (mme_ue_context_p->guti_ue_context_htbl, &ue_context_p->emm_context._guti, sizeof (*guti_p));
      if (INVALID_MME_UE_S1AP_ID != mme_ue_s1ap_id) {
        h_rc = obj_hashtable_uint64_ts_insert (mme_ue_context_p->guti_ue_context_htbl, (const void *const)guti_p, sizeof (*guti_p), (uint64_t)mme_ue_s1ap_id);
      } else {
        h_rc = HASH_TABLE_KEY_NOT_EXISTS;
      }

      if (HASH_TABLE_OK != h_rc) {
        OAILOG_TRACE (LOG_MME_APP, "Error could not update this ue context %p enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " guti " GUTI_FMT " %s\n",
            ue_context_p, ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id, GUTI_ARG(guti_p), hashtable_rc_code2string(h_rc));
      }
      memcpy(&ue_context_p->emm_context._guti , guti_p, sizeof(ue_context_p->emm_context._guti));
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
  hashtable_uint64_ts_dump_content (mme_app_desc.mme_ue_contexts.mme_ue_s1ap_id_ue_context_htbl, tmp);
  OAILOG_TRACE (LOG_MME_APP,"mme_ue_s1ap_id_ue_context_htbl %s\n", bdata(tmp));

  btrunc(tmp, 0);
  hashtable_ts_dump_content (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl, tmp);
  OAILOG_TRACE (LOG_MME_APP,"enb_ue_s1ap_id_ue_context_htbl %s\n", bdata(tmp));

  btrunc(tmp, 0);
  obj_hashtable_uint64_ts_dump_content (mme_app_desc.mme_ue_contexts.guti_ue_context_htbl, tmp);
  OAILOG_TRACE (LOG_MME_APP,"guti_ue_context_htbl %s", bdata(tmp));
}

//------------------------------------------------------------------------------
int
mme_insert_ue_context (
  mme_ue_context_t * const mme_ue_context_p,
  const struct ue_mm_context_s *const ue_context_p)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (mme_ue_context_p );
  DevAssert (ue_context_p );


  // filled ENB UE S1AP ID
  h_rc = hashtable_ts_is_key_exists (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context_p->enb_s1ap_id_key);
  if (HASH_TABLE_OK == h_rc) {
    OAILOG_DEBUG (LOG_MME_APP, "This ue context %p already exists enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT "\n",
        ue_context_p, ue_context_p->enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  h_rc = hashtable_ts_insert (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl,
                             (const hash_key_t)ue_context_p->enb_s1ap_id_key,
                             (void *)ue_context_p);

  if (HASH_TABLE_OK != h_rc) {
    OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ue_id 0x%x\n",
        ue_context_p, ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }


  OAILOG_DEBUG (LOG_MME_APP, "Registered this ue context %p enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
      ue_context_p, ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id);

  if ( INVALID_MME_UE_S1AP_ID != ue_context_p->mme_ue_s1ap_id) {
    h_rc = hashtable_uint64_ts_is_key_exists (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context_p->mme_ue_s1ap_id);

    if (HASH_TABLE_OK == h_rc) {
      OAILOG_DEBUG (LOG_MME_APP, "This ue context %p already exists mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
          ue_context_p, ue_context_p->mme_ue_s1ap_id);
      OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
    }
    //OAI_GCC_DIAG_OFF(discarded-qualifiers);
    h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl,
                                (const hash_key_t)ue_context_p->mme_ue_s1ap_id,
                                (uint64_t)(ue_context_p->enb_s1ap_id_key));
    //OAI_GCC_DIAG_ON(discarded-qualifiers);

    if (HASH_TABLE_OK != h_rc) {
      OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
          ue_context_p, ue_context_p->mme_ue_s1ap_id);
      OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
    }

    // filled IMSI
    if (ue_context_p->emm_context._imsi64) {
      h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->imsi_ue_context_htbl,
                                  (const hash_key_t)ue_context_p->emm_context._imsi64,
                                  (uint64_t)(ue_context_p->mme_ue_s1ap_id));

      if (HASH_TABLE_OK != h_rc) {
        OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " imsi " IMSI_64_FMT "\n",
            ue_context_p, ue_context_p->mme_ue_s1ap_id, ue_context_p->emm_context._imsi64);
        OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
      }
    }

    // filled S11 tun id
    if (ue_context_p->mme_teid_s11) {
      h_rc = hashtable_uint64_ts_insert (mme_ue_context_p->tun11_ue_context_htbl,
                                 (const hash_key_t)ue_context_p->mme_teid_s11,
                                 (uint64_t)(ue_context_p->mme_ue_s1ap_id));

      if (HASH_TABLE_OK != h_rc) {
        OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " mme_teid_s11 " TEID_FMT "\n",
            ue_context_p, ue_context_p->mme_ue_s1ap_id, ue_context_p->mme_teid_s11);
        OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
      }
    }

    // filled guti
    if ((0 != ue_context_p->emm_context._guti.gummei.mme_code) ||
        (0 != ue_context_p->emm_context._guti.gummei.mme_gid) ||
        (0 != ue_context_p->emm_context._guti.m_tmsi) ||
        (0 != ue_context_p->emm_context._guti.gummei.plmn.mcc_digit1) ||     // MCC 000 does not exist in ITU table
        (0 != ue_context_p->emm_context._guti.gummei.plmn.mcc_digit2) ||
        (0 != ue_context_p->emm_context._guti.gummei.plmn.mcc_digit3)) {

      h_rc = obj_hashtable_uint64_ts_insert (mme_ue_context_p->guti_ue_context_htbl,
                                     (const void *const)&ue_context_p->emm_context._guti,
                                     sizeof (ue_context_p->emm_context._guti),
                                     (uint64_t)(ue_context_p->mme_ue_s1ap_id));

      if (HASH_TABLE_OK != h_rc) {
        OAILOG_DEBUG (LOG_MME_APP, "Error could not register this ue context %p mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " guti "GUTI_FMT"\n",
                ue_context_p, ue_context_p->mme_ue_s1ap_id, GUTI_ARG(&ue_context_p->emm_context._guti));
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


//------------------------------------------------------------------------------
void mme_notify_ue_context_released (
    mme_ue_context_t * const mme_ue_context_p,
    struct ue_mm_context_s *ue_context_p)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (mme_ue_context_p );
  DevAssert (ue_context_p );
  /*
   * Updating statistics
   */
  __sync_fetch_and_sub (&mme_ue_context_p->nb_ue_managed, 1);
  __sync_fetch_and_sub (&mme_ue_context_p->nb_ue_since_last_stat, 1);

  mme_app_send_nas_signalling_connection_rel_ind(ue_context_p->mme_ue_s1ap_id);

  // TODO HERE free resources

  OAILOG_FUNC_OUT (LOG_MME_APP);
}
//------------------------------------------------------------------------------
void mme_remove_ue_context (
  mme_ue_context_t * const mme_ue_context_p,
  struct ue_mm_context_s *ue_context_p)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  hashtable_rc_t                          hash_rc = HASH_TABLE_OK;

  DevAssert (mme_ue_context_p );
  DevAssert (ue_context_p );
  /*
   * Updating statistics
   */
  __sync_fetch_and_sub (&mme_ue_context_p->nb_ue_managed, 1);
  __sync_fetch_and_sub (&mme_ue_context_p->nb_ue_since_last_stat, 1);

  if (ue_context_p->emm_context._imsi64) {
    hash_rc = hashtable_uint64_ts_remove (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)ue_context_p->emm_context._imsi64);
    if (HASH_TABLE_OK != hash_rc)
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", IMSI " IMSI_64_FMT "  not in IMSI collection\n",
          ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id, ue_context_p->emm_context._imsi64);
  }
  // filled NAS UE ID
  if (INVALID_MME_UE_S1AP_ID != ue_context_p->mme_ue_s1ap_id) {
    hash_rc = hashtable_uint64_ts_remove (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context_p->mme_ue_s1ap_id);
    if (HASH_TABLE_OK != hash_rc)
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT ", mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " not in MME UE S1AP ID collection\n",
          ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id);
  }
  // filled S11 tun id
  if (ue_context_p->mme_teid_s11) {
    hash_rc = hashtable_uint64_ts_remove (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context_p->mme_teid_s11);
    if (HASH_TABLE_OK != hash_rc)
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", MME S11 TEID  " TEID_FMT "  not in S11 collection\n",
          ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id, ue_context_p->mme_teid_s11);
  }
  // filled guti
  if ((ue_context_p->emm_context._guti.gummei.mme_code) || (ue_context_p->emm_context._guti.gummei.mme_gid) || (ue_context_p->emm_context._guti.m_tmsi) ||
      (ue_context_p->emm_context._guti.gummei.plmn.mcc_digit1) || (ue_context_p->emm_context._guti.gummei.plmn.mcc_digit2) || (ue_context_p->emm_context._guti.gummei.plmn.mcc_digit3)) { // MCC 000 does not exist in ITU table
    hash_rc = obj_hashtable_uint64_ts_remove (mme_ue_context_p->guti_ue_context_htbl, (const void *const)&ue_context_p->emm_context._guti, sizeof (ue_context_p->emm_context._guti));
    if (HASH_TABLE_OK != hash_rc)
      OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", GUTI  not in GUTI collection\n",
          ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id);
  }

  hash_rc = hashtable_ts_remove (mme_ue_context_p->enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context_p->enb_s1ap_id_key, (void **)&ue_context_p);
  if (HASH_TABLE_OK != hash_rc)
    OAILOG_DEBUG(LOG_MME_APP, "UE context enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ", ENB_UE_S1AP_ID not ENB_UE_S1AP_ID collection\n",
      ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id);

  mme_app_ue_context_free_content(ue_context_p);
  free_wrapper ((void**)&ue_context_p);
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
  bformata (bstr_dump, "%*s -     Timer arg .......: %p\n", indent_spaces, " ", bc->esm_ebr_context.args);
  if (bc->esm_ebr_context.args) {
    bformata (bstr_dump, "%*s -     Ctx ...........: %p\n", indent_spaces, " ", bc->esm_ebr_context.args->ctx);
    bformata (bstr_dump, "%*s -     mme_ue_s1ap_id : " MME_UE_S1AP_ID_FMT "\n", indent_spaces, " ", bc->esm_ebr_context.args->ue_id);
    bformata (bstr_dump, "%*s -     ebi          ..: %u\n", indent_spaces, " ", bc->esm_ebr_context.args->ebi);
    bformata (bstr_dump, "%*s -     RTx counter ...: %u\n", indent_spaces, " ", bc->esm_ebr_context.args->count);
    bformata (bstr_dump, "%*s -     RTx ESM msg ...: \n", indent_spaces, " ");
    OAILOG_STREAM_HEX(OAILOG_LEVEL_DEBUG, LOG_MME_APP, NULL, bdata(bc->esm_ebr_context.args->msg), blength(bc->esm_ebr_context.args->msg));
  }
  bformata (bstr_dump, "%*s - PDN id ..........: %u\n", indent_spaces, " ", bc->pdn_cx_id);
}

//------------------------------------------------------------------------------
void mme_app_dump_pdn_context (const struct ue_mm_context_s *const ue_mm_context,
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

    bformata (bstr_dump, "%*s - PDN type ...........: %s\n", indent_spaces, " ", PDN_TYPE_TO_STRING (pdn_context->paa.pdn_type));
    if (pdn_context->paa.pdn_type == IPv4) {
      char ipv4[INET_ADDRSTRLEN];
      inet_ntop (AF_INET, (void*)&pdn_context->paa.ipv4_address, ipv4, INET_ADDRSTRLEN);
      bformata (bstr_dump, "%*s - PAA (IPv4)..........: %s\n", indent_spaces, " ", ipv4);
    } else {
      char                                    ipv6[INET6_ADDRSTRLEN];
      inet_ntop (AF_INET6, &pdn_context->paa.ipv6_address, ipv6, INET6_ADDRSTRLEN);
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
    for (int bindex = 0; bindex < 0; bindex++) {
      // should be equal to bindex if valid
      int bcindex = pdn_context->bearer_contexts[bindex];
      if ((0 <= bcindex) && ( BEARERS_PER_UE > bcindex)) {
        AssertFatal(bindex == bcindex, "Mismatch in configuration bearer index pdn %i != %i\n", bindex, bcindex);

        bearer_context_t *bc = ue_mm_context->bearer_contexts[bcindex];
        AssertFatal(bc, "Mismatch in configuration bearer context NULL\n");
        bformata (bstr_dump, "%*s - Bearer item ----------------------------\n");
        mme_app_dump_bearer_context(bc, indent_spaces + 4, bstr_dump);
      }
    }
  }
}

//------------------------------------------------------------------------------
bool
mme_app_dump_ue_context (
  const hash_key_t keyP,
  void *const ue_mm_context_pP,
  void *unused_param_pP,
  void** unused_result_pP)
//------------------------------------------------------------------------------
{
  struct ue_mm_context_s           *const ue_mm_context = (struct ue_mm_context_s *)ue_mm_context_pP;
  uint8_t                                 j = 0;
  if (ue_mm_context) {
    bstring                                 bstr_dump = bfromcstralloc(4096, "\n-----------------------UE MM context ");
    bformata (bstr_dump, "%p --------------------\n", ue_mm_context);
    bformata (bstr_dump, "    - Authenticated ..: %s\n", (ue_mm_context->imsi_auth == IMSI_UNAUTHENTICATED) ? "FALSE" : "TRUE");
    bformata (bstr_dump, "    - eNB UE s1ap ID .: %08x\n", ue_mm_context->enb_ue_s1ap_id);
    bformata (bstr_dump, "    - MME UE s1ap ID .: %08x\n", ue_mm_context->mme_ue_s1ap_id);
    bformata (bstr_dump, "    - MME S11 TEID ...: " TEID_FMT "\n", ue_mm_context->mme_teid_s11);
    bformata (bstr_dump, "                        | mcc | mnc | cell identity |\n");
    bformata (bstr_dump, "    - E-UTRAN CGI ....: | %u%u%u | %u%u%c | %05x.%02x    |\n",
                 ue_mm_context->e_utran_cgi.plmn.mcc_digit1,
                 ue_mm_context->e_utran_cgi.plmn.mcc_digit2 ,
                 ue_mm_context->e_utran_cgi.plmn.mcc_digit3,
                 ue_mm_context->e_utran_cgi.plmn.mnc_digit1,
                 ue_mm_context->e_utran_cgi.plmn.mnc_digit2,
                 (ue_mm_context->e_utran_cgi.plmn.mnc_digit3 > 9) ? ' ':0x30+ue_mm_context->e_utran_cgi.plmn.mnc_digit3,
                 ue_mm_context->e_utran_cgi.cell_identity.enb_id, ue_mm_context->e_utran_cgi.cell_identity.cell_id);
    /*
     * Ctime return a \n in the string
     */
    bformata (bstr_dump, "    - Last acquired ..: %s", ctime (&ue_mm_context->cell_age));

    emm_context_dump(&ue_mm_context->emm_context, 4, bstr_dump);
    /*
     * Display UE info only if we know them
     */
    if (SUBSCRIPTION_KNOWN == ue_mm_context->subscription_known) {
      // TODO bformata (bstr_dump, "    - Status .........: %s\n", (ue_mm_context->sub_status == SS_SERVICE_GRANTED) ? "Granted" : "Barred");
#define DISPLAY_BIT_MASK_PRESENT(mASK)   \
    ((ue_mm_context->access_restriction_data & mASK) ? 'X' : 'O')
      bformata (bstr_dump, "    (O = allowed, X = !O) |UTRAN|GERAN|GAN|HSDPA EVO|E_UTRAN|HO TO NO 3GPP|\n");
      bformata (bstr_dump, "    - Access restriction  |  %c  |  %c  | %c |    %c    |   %c   |      %c      |\n",
          DISPLAY_BIT_MASK_PRESENT (ARD_UTRAN_NOT_ALLOWED),
          DISPLAY_BIT_MASK_PRESENT (ARD_GERAN_NOT_ALLOWED),
          DISPLAY_BIT_MASK_PRESENT (ARD_GAN_NOT_ALLOWED), DISPLAY_BIT_MASK_PRESENT (ARD_I_HSDPA_EVO_NOT_ALLOWED), DISPLAY_BIT_MASK_PRESENT (ARD_E_UTRAN_NOT_ALLOWED), DISPLAY_BIT_MASK_PRESENT (ARD_HO_TO_NON_3GPP_NOT_ALLOWED));
      // TODO bformata (bstr_dump, "    - Access Mode ....: %s\n", ACCESS_MODE_TO_STRING (ue_mm_context->access_mode));
      // TODO MSISDN
      //bformata (bstr_dump, "    - MSISDN .........: %s\n", (ue_mm_context->msisdn) ? ue_mm_context->msisdn->data:"None");
      bformata (bstr_dump, "    - RAU/TAU timer ..: %u\n", ue_mm_context->rau_tau_timer);
      // TODO IMEISV
      //if (IS_EMM_CTXT_PRESENT_IMEISV(&ue_mm_context->nas_emm_context)) {
      //  bformata (bstr_dump, "    - IMEISV .........: %*s\n", IMEISV_DIGITS_MAX, ue_mm_context->nas_emm_context._imeisv);
      //}
      bformata (bstr_dump, "    - AMBR (bits/s)     ( Downlink |  Uplink  )\n");
      // TODO bformata (bstr_dump, "        Subscribed ...: (%010" PRIu64 "|%010" PRIu64 ")\n", ue_mm_context->subscribed_ambr.br_dl, ue_mm_context->subscribed_ambr.br_ul);
      bformata (bstr_dump, "        Allocated ....: (%010" PRIu64 "|%010" PRIu64 ")\n", ue_mm_context->used_ambr.br_dl, ue_mm_context->used_ambr.br_ul);

      bformata (bstr_dump, "    - APN config list:\n");

      for (j = 0; j < ue_mm_context->apn_config_profile.nb_apns; j++) {
        struct apn_configuration_s             *apn_config_p;

        apn_config_p = &ue_mm_context->apn_config_profile.apn_configuration[j];
        /*
         * Default APN ?
         */
        bformata (bstr_dump, "        - Default APN ...: %s\n", (apn_config_p->context_identifier == ue_mm_context->apn_config_profile.context_identifier)
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
      for (pdn_cid_t pdn_cid = 0; pdn_cid < MAX_APN_PER_UE; pdn_cid++) {
        pdn_context_t         *pdn_context = ue_mm_context->pdn_contexts[pdn_cid];
        if (pdn_context) {
          mme_app_dump_pdn_context(ue_mm_context, pdn_context, pdn_cid, 8, bstr_dump);
        }
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
void
mme_app_handle_s1ap_ue_context_release_req (
  const itti_s1ap_ue_context_release_req_t const *s1ap_ue_context_release_req)
//------------------------------------------------------------------------------
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_mm_context_s                    *ue_context_p = NULL;

  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, s1ap_ue_context_release_req->mme_ue_s1ap_id);
  if (!ue_context_p) {
    ue_context_p = mme_ue_context_exists_enb_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, s1ap_ue_context_release_req->enb_ue_s1ap_id);
    if ((!ue_context_p) || (INVALID_MME_UE_S1AP_ID == ue_context_p->mme_ue_s1ap_id)) {
      NOT_REQUIREMENT_3GPP_36_413(R10_8_3_3_2__2);
      MSC_LOG_EVENT (MSC_MMEAPP_MME, "0 S1AP_UE_CONTEXT_RELEASE_REQ Unknown mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "  enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ",
          s1ap_ue_context_release_req->mme_ue_s1ap_id, s1ap_ue_context_release_req->enb_ue_s1ap_id);
      OAILOG_ERROR (LOG_MME_APP, "S1AP_UE_CONTEXT_RELEASE_REQ group %d cause %ld, UE context doesn't exist for enb_ue_s1ap_ue_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
          s1ap_ue_context_release_req->cause.present, s1ap_ue_context_release_req->cause.choice.misc, s1ap_ue_context_release_req->enb_ue_s1ap_id, s1ap_ue_context_release_req->mme_ue_s1ap_id);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
  }
  ue_context_p->is_s1_ue_context_release = true;


  if (!ue_context_p->nb_active_pdn_contexts) {
    // no session was created, no need for releasing bearers in SGW
    S1ap_Cause_t            s1_ue_context_release_cause = {0};
    s1_ue_context_release_cause.present = S1ap_Cause_PR_radioNetwork;
    s1_ue_context_release_cause.choice.radioNetwork = S1ap_CauseRadioNetwork_release_due_to_eutran_generated_reason;
    mme_app_send_s1ap_ue_context_release_command(ue_context_p, s1_ue_context_release_cause);
    mme_app_send_nas_signalling_connection_rel_ind(ue_context_p->mme_ue_s1ap_id);
  } else {
    for (pdn_cid_t i = 0; i < MAX_APN_PER_UE; i++) {
      if (ue_context_p->pdn_contexts[i]) {
        mme_app_send_s11_release_access_bearers_req (ue_context_p, i);
      }
    }
  }
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
//------------------------------------------------------------------------------
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_mm_context_s                    *ue_context_p = NULL;

  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, s1ap_ue_context_release_complete->mme_ue_s1ap_id);

  if (!ue_context_p) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "0 S1AP_UE_CONTEXT_RELEASE_COMPLETE Unknown mme_ue_s1ap_id 0x%06" PRIX32 " ", s1ap_ue_context_release_complete->mme_ue_s1ap_id);
    OAILOG_WARNING (LOG_MME_APP, "UE context doesn't exist for enb_ue_s1ap_id "ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
        s1ap_ue_context_release_complete->enb_ue_s1ap_id, s1ap_ue_context_release_complete->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

//  ue_context_p->ecm_state = ECM_IDLE;
//  ue_context_p->is_s1_ue_context_release = false;
//
  mme_app_ue_context_s1_release_enb_informations(ue_context_p);
  mme_app_delete_s11_procedure_create_bearer(ue_context_p);

  if (((S1ap_Cause_PR_radioNetwork == ue_context_p->s1_ue_context_release_cause.present) &&
       (S1ap_CauseRadioNetwork_radio_connection_with_ue_lost == ue_context_p->s1_ue_context_release_cause.choice.radioNetwork))) {
    // initiate deactivation procedure
    for (pdn_cid_t i = 0; i < MAX_APN_PER_UE; i++) {
      if (ue_context_p->pdn_contexts[i]) {
        mme_app_trigger_mme_initiated_dedicated_bearer_deactivation_procedure (ue_context_p, i);
      }
    }
  }

  mme_notify_ue_context_released(&mme_app_desc.mme_ue_contexts, ue_context_p);

  if (ue_context_p->mm_state == UE_UNREGISTERED) {
    OAILOG_DEBUG (LOG_MME_APP, "Deleting UE context associated in MME for mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n ",
                  s1ap_ue_context_release_complete->mme_ue_s1ap_id);
    mme_remove_ue_context(&mme_app_desc.mme_ue_contexts, ue_context_p);
  } else {
    ue_context_p->ecm_state = ECM_IDLE;
  }

  // TODO remove in context GBR bearers
  OAILOG_FUNC_OUT (LOG_MME_APP);
}


//------------------------------------------------------------------------------
void mme_app_send_s1ap_ue_context_release_command (ue_mm_context_t *ue_context, S1ap_Cause_t cause)
{
  MessageDef *message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_UE_CONTEXT_RELEASE_COMMAND);
  AssertFatal (message_p , "itti_alloc_new_message Failed");
  S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).mme_ue_s1ap_id = ue_context->mme_ue_s1ap_id;
  S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).enb_ue_s1ap_id = ue_context->enb_ue_s1ap_id;
  S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).cause = cause;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0, "0 S1AP_UE_CONTEXT_RELEASE_COMMAND mme_ue_s1ap_id %06" PRIX32 " ",
      S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).mme_ue_s1ap_id);
  int to_task = (RUN_MODE_SCENARIO_PLAYER == mme_config.run_mode) ? TASK_MME_SCENARIO_PLAYER:TASK_S1AP;
  itti_send_msg_to_task (to_task, INSTANCE_DEFAULT, message_p);
}

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



#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <arpa/inet.h>

#include "intertask_interface.h"
#include "mme_config.h"
#include "assertions.h"
#include "conversions.h"
#include "tree.h"
#include "enum_string.h"
#include "common_types.h"
#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "msc.h"
#include "dynamic_memory_check.h"
#include "log.h"

//------------------------------------------------------------------------------
ue_context_t                           *
mme_create_new_ue_context (
  void)
//------------------------------------------------------------------------------
{
  ue_context_t                           *new_p = CALLOC_CHECK (1, sizeof (ue_context_t));

  return new_p;
}


//------------------------------------------------------------------------------
struct ue_context_s                    *
mme_ue_context_exists_imsi (
  mme_ue_context_t * const mme_ue_context_p,
  const mme_app_imsi_t imsi)
//------------------------------------------------------------------------------
{
  hashtable_rc_t                          h_rc;
  void                                   *id = NULL;

  h_rc = hashtable_ts_get (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)imsi, (void **)&id);

  if (h_rc == HASH_TABLE_OK) {
    return mme_ue_context_exists_mme_ue_s1ap_id (mme_ue_context_p, (uint32_t) id);
  }

  return NULL;
}

//------------------------------------------------------------------------------
struct ue_context_s                    *
mme_ue_context_exists_s11_teid (
  mme_ue_context_t * const mme_ue_context_p,
  const uint32_t teid)
//------------------------------------------------------------------------------
{
  hashtable_rc_t                          h_rc;
  void                                   *id = NULL;

  h_rc = hashtable_ts_get (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)teid, (void **)&id);

  if (h_rc == HASH_TABLE_OK) {
    return mme_ue_context_exists_mme_ue_s1ap_id (mme_ue_context_p, (uint32_t) id);
  }

  return NULL;
}


//------------------------------------------------------------------------------
ue_context_t                           *
mme_ue_context_exists_mme_ue_s1ap_id (
  mme_ue_context_t * const mme_ue_context_p,
  const uint32_t mme_ue_s1ap_id)
//------------------------------------------------------------------------------
{
  struct ue_context_s                    *ue_context_p = NULL;

  hashtable_ts_get (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)mme_ue_s1ap_id, (void **)&ue_context_p);
  return ue_context_p;
}


//------------------------------------------------------------------------------
ue_context_t                           *
mme_ue_context_exists_nas_ue_id (
  mme_ue_context_t * const mme_ue_context_p,
  const uint32_t nas_ue_id)
//------------------------------------------------------------------------------
{
  hashtable_rc_t                          h_rc;
  void                                   *id = NULL;

  h_rc = hashtable_ts_get (mme_ue_context_p->nas_ue_id_ue_context_htbl, (const hash_key_t)nas_ue_id, (void **)&id);

  if (h_rc == HASH_TABLE_OK) {
    return mme_ue_context_exists_mme_ue_s1ap_id (mme_ue_context_p, (uint32_t) id);
  }

  return NULL;
}


//------------------------------------------------------------------------------
ue_context_t                           *
mme_ue_context_exists_guti (
  mme_ue_context_t * const mme_ue_context_p,
  const GUTI_t * const guti_p)
//------------------------------------------------------------------------------
{
  hashtable_rc_t                          h_rc;
  void                                   *id = NULL;

  h_rc = obj_hashtable_ts_get (mme_ue_context_p->guti_ue_context_htbl, (const void *)guti_p, sizeof (*guti_p), (void **)&id);

  if (h_rc == HASH_TABLE_OK) {
    return mme_ue_context_exists_mme_ue_s1ap_id (mme_ue_context_p, id);
  }

  return NULL;
}

//------------------------------------------------------------------------------
void
mme_ue_context_update_coll_keys (
  mme_ue_context_t * const mme_ue_context_p,
  ue_context_t * const ue_context_p,
  const uint32_t mme_ue_s1ap_id,
  const mme_app_imsi_t imsi,
  const uint32_t mme_s11_teid,
  const uint32_t nas_ue_id,
  const GUTI_t * const guti_p)  //  never NULL, if none put &ue_context_p->guti
//------------------------------------------------------------------------------
{
  ue_context_t                           *same_ue_context_p = NULL;
  hashtable_rc_t                          h_rc;
  void                                   *id = NULL;

  if (mme_ue_s1ap_id > 0) {

    if ((ue_context_p->imsi != imsi)
        || (ue_context_p->mme_ue_s1ap_id != mme_ue_s1ap_id)) {
      h_rc = hashtable_ts_remove (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)ue_context_p->imsi, (void **)&id);
      h_rc = hashtable_ts_insert (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)imsi, (void *)mme_ue_s1ap_id);

      if (h_rc != HASH_TABLE_OK) {
        LOG_DEBUG (LOG_MME_APP, "mme_ue_context_update_coll_keys: Error could not update this ue context %p mme_ue_s1ap_id 0x%x imsi %" SCNu64 "\n",
        		ue_context_p, ue_context_p->mme_ue_s1ap_id, imsi);
      }

      ue_context_p->imsi = imsi;
    }

    if ((ue_context_p->mme_s11_teid != mme_s11_teid)
        || (ue_context_p->mme_ue_s1ap_id != mme_ue_s1ap_id)) {
      h_rc = hashtable_ts_remove (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context_p->mme_s11_teid, (void **)&id);
      h_rc = hashtable_ts_insert (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)mme_s11_teid, (void *)mme_ue_s1ap_id);

      if (h_rc != HASH_TABLE_OK) {
        LOG_DEBUG (LOG_MME_APP, "mme_ue_context_update_coll_keys: Error could not update this ue context %p mme_ue_s1ap_id 0x%x mme_s11_teid 0x%x\n", ue_context_p, ue_context_p->mme_ue_s1ap_id, mme_s11_teid);
      }

      ue_context_p->mme_s11_teid = mme_s11_teid;
    }

    if ((ue_context_p->ue_id != nas_ue_id)
        || (ue_context_p->mme_ue_s1ap_id != mme_ue_s1ap_id)) {
      h_rc = hashtable_ts_remove (mme_ue_context_p->nas_ue_id_ue_context_htbl, (const hash_key_t)ue_context_p->ue_id, (void **)&id);
      h_rc = hashtable_ts_insert (mme_ue_context_p->nas_ue_id_ue_context_htbl, (const hash_key_t)nas_ue_id, (void *)mme_ue_s1ap_id);

      if (h_rc != HASH_TABLE_OK) {
        LOG_DEBUG (LOG_MME_APP, "mme_ue_context_update_coll_keys: Error could not update this ue context %p mme_ue_s1ap_id 0x%x nas_ue_id 0x%x\n", ue_context_p, ue_context_p->mme_ue_s1ap_id, nas_ue_id);
      }

      ue_context_p->ue_id = nas_ue_id;
    }

    if ((guti_p->gummei.MMEcode != ue_context_p->guti.gummei.MMEcode)
        || (guti_p->gummei.MMEgid != ue_context_p->guti.gummei.MMEgid)
        || (guti_p->m_tmsi != ue_context_p->guti.m_tmsi)
        || (guti_p->gummei.plmn.MCCdigit1 != ue_context_p->guti.gummei.plmn.MCCdigit1)
        || (guti_p->gummei.plmn.MCCdigit2 != ue_context_p->guti.gummei.plmn.MCCdigit2)
        || (guti_p->gummei.plmn.MCCdigit3 != ue_context_p->guti.gummei.plmn.MCCdigit3)
        || (ue_context_p->mme_ue_s1ap_id != mme_ue_s1ap_id)) {
      h_rc = obj_hashtable_ts_remove (mme_ue_context_p->guti_ue_context_htbl, (const void *)guti_p, sizeof (*guti_p), (void **)&id);
      h_rc = obj_hashtable_ts_insert (mme_ue_context_p->guti_ue_context_htbl, (const void *)guti_p, sizeof (*guti_p), (void *)mme_ue_s1ap_id);

      if (h_rc != HASH_TABLE_OK) {
        char                                    guti_str[GUTI2STR_MAX_LENGTH];

        GUTI2STR (guti_p, guti_str, GUTI2STR_MAX_LENGTH);
        LOG_DEBUG (LOG_MME_APP, "mme_ue_context_update_coll_keys: Error could not update this ue context %p mme_ue_s1ap_id 0x%x guti %s\n", ue_context_p, ue_context_p->mme_ue_s1ap_id, guti_str);
      }

      ue_context_p->guti = *guti_p;
    }

    if (ue_context_p->mme_ue_s1ap_id != mme_ue_s1ap_id) {
      h_rc = hashtable_ts_remove (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context_p->mme_ue_s1ap_id, (void **)&same_ue_context_p);
      h_rc = hashtable_ts_insert (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)mme_ue_s1ap_id, (void *)ue_context_p);

      if (h_rc != HASH_TABLE_OK) {
        LOG_DEBUG (LOG_MME_APP, "mme_ue_context_update_coll_keys: Error could not update this ue context %p mme_ue_s1ap_id 0x%x\n",
            ue_context_p, ue_context_p->mme_ue_s1ap_id);
      }

      ue_context_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
    }
  }
}


//------------------------------------------------------------------------------
int
mme_insert_ue_context (
  mme_ue_context_t * const mme_ue_context_p,
  const struct ue_context_s *const ue_context_p)
//------------------------------------------------------------------------------
{
  hashtable_rc_t                          h_rc;

  DevAssert (mme_ue_context_p != NULL);
  DevAssert (ue_context_p != NULL);
  h_rc = hashtable_ts_is_key_exists (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context_p->mme_ue_s1ap_id);

  if (h_rc == HASH_TABLE_OK) {
    LOG_DEBUG (LOG_MME_APP, "mme_insert_ue_context: This ue context %p already exists mme_ue_s1ap_id 0x%x\n", ue_context_p, ue_context_p->mme_ue_s1ap_id);
    return -1;
  }

  h_rc = hashtable_ts_insert (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context_p->mme_ue_s1ap_id, ue_context_p);

  if (h_rc != HASH_TABLE_OK) {
    LOG_DEBUG (LOG_MME_APP, "mme_insert_ue_context: Error could not register this ue context %p mme_ue_s1ap_id 0x%x\n", ue_context_p, ue_context_p->mme_ue_s1ap_id);
    return -1;
  }

  /*
   * Updating statistics
   */
  __sync_fetch_and_add (&mme_ue_context_p->nb_ue_managed, 1);
  __sync_fetch_and_add (&mme_ue_context_p->nb_ue_since_last_stat, 1);

  // filled IMSI
  if (ue_context_p->imsi > 0) {
    h_rc = hashtable_ts_insert (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)ue_context_p->imsi, (void *)ue_context_p->mme_ue_s1ap_id);

    if (h_rc != HASH_TABLE_OK) {
      LOG_DEBUG (LOG_MME_APP, "mme_insert_ue_context: Error could not register this ue context %p mme_ue_s1ap_id 0x%x imsi %" SCNu64 "\n", ue_context_p, ue_context_p->mme_ue_s1ap_id, ue_context_p->imsi);
      return -1;
    }
  }
  // filled NAS UE ID
  if (ue_context_p->ue_id > 0) {
    h_rc = hashtable_ts_insert (mme_ue_context_p->nas_ue_id_ue_context_htbl, (const hash_key_t)ue_context_p->ue_id, (void *)ue_context_p->mme_ue_s1ap_id);

    if (h_rc != HASH_TABLE_OK) {
      LOG_DEBUG (LOG_MME_APP, "mme_insert_ue_context: Error could not register this ue context %p mme_ue_s1ap_id 0x%x ue_id 0x%x\n", ue_context_p, ue_context_p->mme_ue_s1ap_id, ue_context_p->ue_id);
      return -1;
    }
  }
  // filled S11 tun id
  if (ue_context_p->mme_s11_teid > 0) {
    h_rc = hashtable_ts_insert (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context_p->mme_s11_teid, (void *)ue_context_p->mme_ue_s1ap_id);

    if (h_rc != HASH_TABLE_OK) {
      LOG_DEBUG (LOG_MME_APP, "mme_insert_ue_context: Error could not register this ue context %p mme_ue_s1ap_id 0x%x mme_s11_teid 0x%x\n", ue_context_p, ue_context_p->mme_ue_s1ap_id, ue_context_p->mme_s11_teid);
      return -1;
    }
  }
  // filled guti
  if ((0 != ue_context_p->guti.gummei.MMEcode) || (0 != ue_context_p->guti.gummei.MMEgid) || (0 != ue_context_p->guti.m_tmsi) || (0 != ue_context_p->guti.gummei.plmn.MCCdigit1) ||     // MCC 000 does not exist in ITU table
      (0 != ue_context_p->guti.gummei.plmn.MCCdigit2)
      || (0 != ue_context_p->guti.gummei.plmn.MCCdigit3)) {
    GUTI_t                                 *guti_p = MALLOC_CHECK (sizeof (GUTI_t));

    if (NULL != guti_p) {
      memcpy (guti_p, &ue_context_p->guti, sizeof (*guti_p));
      h_rc = obj_hashtable_ts_insert (mme_ue_context_p->guti_ue_context_htbl, (const void *const)&ue_context_p->guti, sizeof (ue_context_p->guti), (void *)ue_context_p->mme_ue_s1ap_id);

      if (h_rc != HASH_TABLE_OK) {
        char                                    guti_str[GUTI2STR_MAX_LENGTH];

        GUTI2STR (&ue_context_p->guti, guti_str, GUTI2STR_MAX_LENGTH);
        LOG_DEBUG (LOG_MME_APP, "mme_insert_ue_context: Error could not register this ue context %p mme_ue_s1ap_id 0x%x guti %s\n", ue_context_p, ue_context_p->mme_ue_s1ap_id, guti_str);
        return -1;
      }
    } else
      return -1;
  }

  return 0;
}



//------------------------------------------------------------------------------
void
mme_remove_ue_context (
  mme_ue_context_t * const mme_ue_context_p,
  struct ue_context_s *ue_context_p)
//------------------------------------------------------------------------------
{
  unsigned int                           *id = NULL;

  DevAssert (mme_ue_context_p != NULL);
  DevAssert (ue_context_p != NULL);
  /*
   * Updating statistics
   */
  __sync_fetch_and_sub (&mme_ue_context_p->nb_ue_managed, 1);
  __sync_fetch_and_sub (&mme_ue_context_p->nb_ue_since_last_stat, 1);

  if (ue_context_p->imsi > 0) {
    hashtable_ts_remove (mme_ue_context_p->imsi_ue_context_htbl, (const hash_key_t)ue_context_p->imsi, (void **)&id);
  }
  // filled NAS UE ID
  if (ue_context_p->ue_id > 0) {
    hashtable_ts_remove (mme_ue_context_p->nas_ue_id_ue_context_htbl, (const hash_key_t)ue_context_p->ue_id, (void **)&id);
  }
  // filled S11 tun id
  if (ue_context_p->mme_s11_teid > 0) {
    hashtable_ts_remove (mme_ue_context_p->tun11_ue_context_htbl, (const hash_key_t)ue_context_p->mme_s11_teid, (void **)&id);
  }
  // filled guti
  if ((0 != ue_context_p->guti.gummei.MMEcode) || (0 != ue_context_p->guti.gummei.MMEgid) || (0 != ue_context_p->guti.m_tmsi) || (0 != ue_context_p->guti.gummei.plmn.MCCdigit1) ||     // MCC 000 does not exist in ITU table
      (0 != ue_context_p->guti.gummei.plmn.MCCdigit2)
      || (0 != ue_context_p->guti.gummei.plmn.MCCdigit3)) {
    obj_hashtable_ts_remove (mme_ue_context_p->guti_ue_context_htbl, (const void *const)&ue_context_p->guti, sizeof (ue_context_p->guti), (void **)&id);
  }

  hashtable_ts_remove (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context_p->mme_ue_s1ap_id, (void **)&ue_context_p);
#warning "TODO mme_ue_context_free_content"
  //TODO mme_ue_context_free_content(ue_context_p);
  FREE_CHECK (ue_context_p);
}

//------------------------------------------------------------------------------
void
mme_app_dump_ue_context (
  const hash_key_t keyP,
  const void *const ue_context_pP,
  void *unused_param_pP)
//------------------------------------------------------------------------------
{
  struct ue_context_s                    *const context_p = (struct ue_context_s *)ue_context_pP;
  uint8_t                                 j;

  LOG_DEBUG (LOG_MME_APP, "-----------------------UE context -----------------------\n");
  LOG_DEBUG (LOG_MME_APP, "    - IMSI ...........: %" SCNu64 "\n", context_p->imsi);
  LOG_DEBUG (LOG_MME_APP, "                        |  m_tmsi  | mmec | mmegid | mcc | mnc |\n");
  LOG_DEBUG (LOG_MME_APP, "    - GUTI............: | %08x |  %02x  |  %04x  | %03u | %03u |\n", context_p->guti.m_tmsi, context_p->guti.gummei.MMEcode, context_p->guti.gummei.MMEgid,
                 /*
                  * TODO check if two or three digits MNC...
                  */
                 context_p->guti.gummei.plmn.MCCdigit3 * 100 +
                 context_p->guti.gummei.plmn.MCCdigit2 * 10 + context_p->guti.gummei.plmn.MCCdigit1, context_p->guti.gummei.plmn.MNCdigit3 * 100 + context_p->guti.gummei.plmn.MNCdigit2 * 10 + context_p->guti.gummei.plmn.MNCdigit1);
  LOG_DEBUG (LOG_MME_APP, "    - Authenticated ..: %s\n", (context_p->imsi_auth == IMSI_UNAUTHENTICATED) ? "FALSE" : "TRUE");
  LOG_DEBUG (LOG_MME_APP, "    - eNB UE s1ap ID .: %08x\n", context_p->eNB_ue_s1ap_id);
  LOG_DEBUG (LOG_MME_APP, "    - MME UE s1ap ID .: %08x\n", context_p->mme_ue_s1ap_id);
  LOG_DEBUG (LOG_MME_APP, "    - MME S11 TEID ...: %08x\n", context_p->mme_s11_teid);
  LOG_DEBUG (LOG_MME_APP, "    - SGW S11 TEID ...: %08x\n", context_p->sgw_s11_teid);
  LOG_DEBUG (LOG_MME_APP, "                        | mcc | mnc | cell id  |\n");
  LOG_DEBUG (LOG_MME_APP, "    - E-UTRAN CGI ....: | %03u | %03u | %08x |\n",
                 context_p->e_utran_cgi.plmn.MCCdigit3 * 100 +
                 context_p->e_utran_cgi.plmn.MCCdigit2 * 10 +
                 context_p->e_utran_cgi.plmn.MCCdigit1, context_p->e_utran_cgi.plmn.MNCdigit3 * 100 + context_p->e_utran_cgi.plmn.MNCdigit2 * 10 + context_p->e_utran_cgi.plmn.MNCdigit1, context_p->e_utran_cgi.cell_identity);
  /*
   * Ctime return a \n in the string
   */
  LOG_DEBUG (LOG_MME_APP, "    - Last acquired ..: %s", ctime (&context_p->cell_age));

  /*
   * Display UE info only if we know them
   */
  if (context_p->subscription_known == SUBSCRIPTION_KNOWN) {
    LOG_DEBUG (LOG_MME_APP, "    - Status .........: %s\n", (context_p->sub_status == SS_SERVICE_GRANTED) ? "Granted" : "Barred");
#define DISPLAY_BIT_MASK_PRESENT(mASK)   \
  ((context_p->access_restriction_data & mASK) ? 'X' : 'O')
    LOG_DEBUG (LOG_MME_APP, "    (O = allowed, X = !O) |UTRAN|GERAN|GAN|HSDPA EVO|E_UTRAN|HO TO NO 3GPP|\n");
    LOG_DEBUG (LOG_MME_APP,
      "    - Access restriction  |  %c  |  %c  | %c |    %c    |   %c   |      %c      |\n",
       DISPLAY_BIT_MASK_PRESENT (ARD_UTRAN_NOT_ALLOWED),
       DISPLAY_BIT_MASK_PRESENT (ARD_GERAN_NOT_ALLOWED),
       DISPLAY_BIT_MASK_PRESENT (ARD_GAN_NOT_ALLOWED), DISPLAY_BIT_MASK_PRESENT (ARD_I_HSDPA_EVO_NOT_ALLOWED), DISPLAY_BIT_MASK_PRESENT (ARD_E_UTRAN_NOT_ALLOWED), DISPLAY_BIT_MASK_PRESENT (ARD_HO_TO_NON_3GPP_NOT_ALLOWED));
    LOG_DEBUG (LOG_MME_APP, "    - Access Mode ....: %s\n", ACCESS_MODE_TO_STRING (context_p->access_mode));
    LOG_DEBUG (LOG_MME_APP, "    - MSISDN .........: %-*s\n", MSISDN_LENGTH, context_p->msisdn);
    LOG_DEBUG (LOG_MME_APP, "    - RAU/TAU timer ..: %u\n", context_p->rau_tau_timer);
    LOG_DEBUG (LOG_MME_APP, "    - IMEISV .........: %*s\n", IMEISV_DIGITS_MAX, context_p->me_identity.imeisv);
    LOG_DEBUG (LOG_MME_APP, "    - AMBR (bits/s)     ( Downlink |  Uplink  )\n");
    LOG_DEBUG (LOG_MME_APP, "        Subscribed ...: (%010" PRIu64 "|%010" PRIu64 ")\n", context_p->subscribed_ambr.br_dl, context_p->subscribed_ambr.br_ul);
    LOG_DEBUG (LOG_MME_APP, "        Allocated ....: (%010" PRIu64 "|%010" PRIu64 ")\n", context_p->used_ambr.br_dl, context_p->used_ambr.br_ul);
    LOG_DEBUG (LOG_MME_APP, "    - Known vectors ..: %u\n", context_p->nb_of_vectors);

    for (j = 0; j < context_p->nb_of_vectors; j++) {
      int                                     k;
      char                                    xres_string[3 * XRES_LENGTH_MAX + 1];
      eutran_vector_t                        *vector_p;

      vector_p = &context_p->vector_list[j];
      LOG_DEBUG (LOG_MME_APP, "        - RAND ..: " RAND_FORMAT "\n", RAND_DISPLAY (vector_p->rand));
      LOG_DEBUG (LOG_MME_APP, "        - AUTN ..: " AUTN_FORMAT "\n", AUTN_DISPLAY (vector_p->autn));
      LOG_DEBUG (LOG_MME_APP, "        - KASME .: " KASME_FORMAT "\n", KASME_DISPLAY_1 (vector_p->kasme));
      LOG_DEBUG (LOG_MME_APP, "                   " KASME_FORMAT "\n", KASME_DISPLAY_2 (vector_p->kasme));

      for (k = 0; k < vector_p->xres.size; k++) {
        sprintf (&xres_string[k * 3], "%02x,", vector_p->xres.data[k]);
      }

      xres_string[k * 3 - 1] = '\0';
      LOG_DEBUG (LOG_MME_APP, "        - XRES ..: %s\n", xres_string);
    }

    LOG_DEBUG (LOG_MME_APP, "    - PDN List:\n");

    for (j = 0; j < context_p->apn_profile.nb_apns; j++) {
      struct apn_configuration_s             *apn_config_p;

      apn_config_p = &context_p->apn_profile.apn_configuration[j];
      /*
       * Default APN ?
       */
      LOG_DEBUG (LOG_MME_APP, "        - Default APN ...: %s\n", (apn_config_p->context_identifier == context_p->apn_profile.context_identifier)
                     ? "TRUE" : "FALSE");
      LOG_DEBUG (LOG_MME_APP, "        - APN ...........: %s\n", apn_config_p->service_selection);
      LOG_DEBUG (LOG_MME_APP, "        - AMBR (bits/s) ( Downlink |  Uplink  )\n");
      LOG_DEBUG (LOG_MME_APP, "                        (%010" PRIu64 "|%010" PRIu64 ")\n", apn_config_p->ambr.br_dl, apn_config_p->ambr.br_ul);
      LOG_DEBUG (LOG_MME_APP, "        - PDN type ......: %s\n", PDN_TYPE_TO_STRING (apn_config_p->pdn_type));
      LOG_DEBUG (LOG_MME_APP, "        - QOS\n");
      LOG_DEBUG (LOG_MME_APP, "            QCI .........: %u\n", apn_config_p->subscribed_qos.qci);
      LOG_DEBUG (LOG_MME_APP, "            Prio level ..: %u\n", apn_config_p->subscribed_qos.allocation_retention_priority.priority_level);
      LOG_DEBUG (LOG_MME_APP, "            Pre-emp vul .: %s\n", (apn_config_p->subscribed_qos.allocation_retention_priority.pre_emp_vulnerability == PRE_EMPTION_VULNERABILITY_ENABLED) ? "ENABLED" : "DISABLED");
      LOG_DEBUG (LOG_MME_APP, "            Pre-emp cap .: %s\n", (apn_config_p->subscribed_qos.allocation_retention_priority.pre_emp_capability == PRE_EMPTION_CAPABILITY_ENABLED) ? "ENABLED" : "DISABLED");

      if (apn_config_p->nb_ip_address == 0) {
        LOG_DEBUG (LOG_MME_APP, "            IP addr .....: Dynamic allocation\n");
      } else {
        int                                     i;

        LOG_DEBUG (LOG_MME_APP, "            IP addresses :\n");

        for (i = 0; i < apn_config_p->nb_ip_address; i++) {
          if (apn_config_p->ip_address[i].pdn_type == IPv4) {
            LOG_DEBUG (LOG_MME_APP, "                           [" IPV4_ADDR "]\n", IPV4_ADDR_DISPLAY_8 (apn_config_p->ip_address[i].address.ipv4_address));
          } else {
            char                                    ipv6[40];

            inet_ntop (AF_INET6, apn_config_p->ip_address[i].address.ipv6_address, ipv6, 40);
            LOG_DEBUG (LOG_MME_APP, "                           [%s]\n", ipv6);
          }
        }
      }

      LOG_DEBUG (LOG_MME_APP, "\n");
    }

    LOG_DEBUG (LOG_MME_APP, "    - Bearer List:\n");

    for (j = 0; j < BEARERS_PER_UE; j++) {
      bearer_context_t                       *bearer_context_p;

      bearer_context_p = &context_p->eps_bearers[j];

      if (bearer_context_p->s_gw_teid != 0) {
        LOG_DEBUG (LOG_MME_APP, "        Bearer id .......: %02u\n", j);
        LOG_DEBUG (LOG_MME_APP, "        S-GW TEID (UP)...: %08x\n", bearer_context_p->s_gw_teid);
        LOG_DEBUG (LOG_MME_APP, "        P-GW TEID (UP)...: %08x\n", bearer_context_p->p_gw_teid);
        LOG_DEBUG (LOG_MME_APP, "        QCI .............: %u\n", bearer_context_p->qci);
        LOG_DEBUG (LOG_MME_APP, "        Priority level ..: %u\n", bearer_context_p->prio_level);
        LOG_DEBUG (LOG_MME_APP, "        Pre-emp vul .....: %s\n", (bearer_context_p->pre_emp_vulnerability == PRE_EMPTION_VULNERABILITY_ENABLED) ? "ENABLED" : "DISABLED");
        LOG_DEBUG (LOG_MME_APP, "        Pre-emp cap .....: %s\n", (bearer_context_p->pre_emp_capability == PRE_EMPTION_CAPABILITY_ENABLED) ? "ENABLED" : "DISABLED");
      }
    }
  }

  LOG_DEBUG (LOG_MME_APP, "---------------------------------------------------------\n");
}


//------------------------------------------------------------------------------
void
mme_app_dump_ue_contexts (
  const mme_ue_context_t * const mme_ue_context_p)
//------------------------------------------------------------------------------
{
  hashtable_ts_apply_funct_on_elements (mme_ue_context_p->mme_ue_s1ap_id_ue_context_htbl, mme_app_dump_ue_context, NULL);
}


//------------------------------------------------------------------------------
void
mme_app_handle_s1ap_ue_context_release_req (
  const s1ap_ue_context_release_req_t const *s1ap_ue_context_release_req)
//------------------------------------------------------------------------------
{
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;

  LOG_DEBUG (LOG_MME_APP, "Received S1AP_UE_CONTEXT_RELEASE_REQ from S1AP\n");
  ue_context_p = mme_ue_context_exists_nas_ue_id (&mme_app_desc.mme_ue_contexts, s1ap_ue_context_release_req->mme_ue_s1ap_id);

  if (ue_context_p == NULL) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "0 S1AP_UE_CONTEXT_RELEASE_REQ Unknown mme_ue_s1ap_id 0x%06" PRIX32 " ", s1ap_ue_context_release_req->mme_ue_s1ap_id);
    LOG_ERROR (LOG_MME_APP, "UE context doesn't exist for UE 0x%06" PRIX32 "/dec%u\n", s1ap_ue_context_release_req->mme_ue_s1ap_id, s1ap_ue_context_release_req->mme_ue_s1ap_id);
    return;
  }

  if ((ue_context_p->mme_s11_teid == 0) && (ue_context_p->sgw_s11_teid == 0)) {
    // no session was created, no need for releasing bearers in SGW
    message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_UE_CONTEXT_RELEASE_COMMAND);
    AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
    memset ((void *)&message_p->ittiMsg.s1ap_ue_context_release_command, 0, sizeof (s1ap_ue_context_release_command_t));
    S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).mme_ue_s1ap_id = ue_context_p->mme_ue_s1ap_id;
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0, "0 S1AP_UE_CONTEXT_RELEASE_COMMAND mme_ue_s1ap_id %06" PRIX32 " ", S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).mme_ue_s1ap_id);
    itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);
  } else {
    mme_app_send_s11_release_access_bearers_req (ue_context_p);
  }
}


/*
   From GPP TS 23.401 version 11.11.0 Release 11, section 5.3.5 S1 release procedure, point 6:
   The MME deletes any eNodeB related information ("eNodeB Address in Use for S1-MME" and "eNB UE S1AP
   ID") from the UE's MME context, but, retains the rest of the UE's MME context including the S-GW's S1-U
   configuration information (address and TEIDs). All non-GBR EPS bearers established for the UE are preserved
   in the MME and in the Serving GW.
   If the cause of S1 release is because of User is inactivity, Inter-RAT Redirection, the MME shall preserve the
   GBR bearers. If the cause of S1 release is because of CS Fallback triggered, further details about bearer handling
   are described in TS 23.272 [58]. Otherwise, e.g. Radio Connection With UE Lost, S1 signalling connection lost,
   eNodeB failure the MME shall trigger the MME Initiated Dedicated Bearer Deactivation procedure
   (clause 5.4.4.2) for the GBR bearer(s) of the UE after the S1 Release procedure is completed.
*/
//------------------------------------------------------------------------------
void
mme_app_handle_s1ap_ue_context_release_complete (
  const s1ap_ue_context_release_complete_t const
  *s1ap_ue_context_release_complete)
//------------------------------------------------------------------------------
{
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;

  LOG_DEBUG (LOG_MME_APP, "Received S1AP_UE_CONTEXT_RELEASE_COMPLETE from S1AP\n");
  ue_context_p = mme_ue_context_exists_nas_ue_id (&mme_app_desc.mme_ue_contexts, s1ap_ue_context_release_complete->mme_ue_s1ap_id);

  if (ue_context_p == NULL) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "0 S1AP_UE_CONTEXT_RELEASE_COMPLETE Unknown mme_ue_s1ap_id 0x%06" PRIX32 " ", s1ap_ue_context_release_complete->mme_ue_s1ap_id);
    LOG_ERROR (LOG_MME_APP, "UE context doesn't exist for mme_ue_s1ap_id 0x%06" PRIX32 "/dec%u\n", s1ap_ue_context_release_complete->mme_ue_s1ap_id, s1ap_ue_context_release_complete->mme_ue_s1ap_id);
    return;
  }

  message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_DEREGISTER_UE_REQ);
  memset ((void *)&message_p->ittiMsg.s1ap_deregister_ue_req, 0, sizeof (s1ap_deregister_ue_req_t));
  S1AP_DEREGISTER_UE_REQ (message_p).mme_ue_s1ap_id = s1ap_ue_context_release_complete->mme_ue_s1ap_id;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 S1AP_DEREGISTER_UE_REQ");
  itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  //mme_remove_ue_context(&mme_app_desc.mme_ue_contexts, ue_context_p);
  ue_context_p->eNB_ue_s1ap_id = 0;
  ue_context_p->mme_ue_s1ap_id = 0;
  // TODO remove in context GBR bearers
}

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


#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#  include "3gpp_24.007.h"
#  include "3gpp_24.301.h"
#  include "assertions.h"
#  include "tree.h"
#  include "emmData.h"
#  include "log.h"
#  include "security_types.h"
#  include "obj_hashtable.h"
#  include "msc.h"

static bool emm_data_context_dump_hash_table_wrapper (
  hash_key_t keyP,
  void *dataP,
  void *parameterP,
  void**resultP);

struct emm_data_context_s              *
emm_data_context_get (
  emm_data_t * emm_data,
  const mme_ue_s1ap_id_t ue_id)
{
  struct emm_data_context_s              *emm_data_context_p = NULL;

  DevCheck (ue_id > 0, ue_id, 0, 0);
  DevAssert (emm_data );
  hashtable_ts_get (emm_data->ctx_coll_ue_id, (const hash_key_t)(ue_id), (void **)&emm_data_context_p);
  OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX - get UE id " MME_UE_S1AP_ID_FMT " context %p\n", ue_id, emm_data_context_p);
  return emm_data_context_p;
}


struct emm_data_context_s              *
emm_data_context_get_by_guti (
  emm_data_t * emm_data,
  guti_t * guti)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
  mme_ue_s1ap_id_t                        emm_ue_id = INVALID_MME_UE_S1AP_ID;

  DevAssert (emm_data );

  if ( guti) {
    char                                    guti_str[GUTI2STR_MAX_LENGTH];

    GUTI2STR (guti, guti_str, GUTI2STR_MAX_LENGTH);
    h_rc = obj_hashtable_ts_get (emm_data->ctx_coll_guti, (const void *)guti, sizeof (*guti), (void **)(uintptr_t)&emm_ue_id);

    if (HASH_TABLE_OK == h_rc) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX - get_by_guti UE id " MME_UE_S1AP_ID_FMT " %s", emm_ue_id, guti_str);
      return emm_data_context_get (emm_data, (const hash_key_t)emm_ue_id);
    } else {
      emm_ue_id = INVALID_MME_UE_S1AP_ID;
      OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX - get_by_guti %s failed\n", guti_str);
      OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX - get_by_guti PLMN     %01x%01x%01x%01x%01x%01x failed\n",
                 guti->gummei.plmn.mcc_digit1, guti->gummei.plmn.mcc_digit2, guti->gummei.plmn.mnc_digit3,
                 guti->gummei.plmn.mnc_digit1, guti->gummei.plmn.mnc_digit2, guti->gummei.plmn.mcc_digit3);
      OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX - get_by_guti mme_gid  %04x failed\n", guti->gummei.mme_gid);
      OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX - get_by_guti mme_code %01x failed\n", guti->gummei.mme_code);
      OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX - get_by_guti m_tmsi   %08x failed\n", guti->m_tmsi);
    }
  }
  return NULL;
}



struct emm_data_context_s              *
emm_data_context_remove (
  emm_data_t * emm_data,
  struct emm_data_context_s *elm)
{
  struct emm_data_context_s              *emm_data_context_p = NULL;
  mme_ue_s1ap_id_t                       *emm_ue_id          = NULL;

  OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Remove in context %p UE id " MME_UE_S1AP_ID_FMT " ", elm, elm->ue_id);

  if ( elm->guti) {
    guti_t guti = {0};
    memcpy(&guti, (const void*)elm->guti, sizeof(guti));
    obj_hashtable_ts_remove (emm_data->ctx_coll_guti, (const void *)&guti, sizeof (guti), (void **)&emm_ue_id);

    char                                    guti_str[GUTI2STR_MAX_LENGTH];
    GUTI2STR (elm->guti, guti_str, GUTI2STR_MAX_LENGTH);
    OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Remove in ctx_coll_guti context %p UE id " MME_UE_S1AP_ID_FMT " guti %s\n", elm, (mme_ue_s1ap_id_t)((uintptr_t)emm_ue_id), guti_str);
  }

  hashtable_ts_remove (emm_data->ctx_coll_ue_id, (const hash_key_t)(elm->ue_id), (void **)&emm_data_context_p);
  return emm_data_context_p;
}


int
emm_data_context_add (
  emm_data_t * emm_data,
  struct emm_data_context_s *elm)
{
  hashtable_rc_t                          h_rc;

  h_rc = hashtable_ts_insert (emm_data->ctx_coll_ue_id, (const hash_key_t)(elm->ue_id), elm);

  if (HASH_TABLE_OK == h_rc) {
    OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX - Add in context %p UE id " MME_UE_S1AP_ID_FMT "\n", elm, elm->ue_id);

    if ( elm->guti) {
      h_rc = obj_hashtable_ts_insert (emm_data->ctx_coll_guti, (const void *const)(elm->guti), sizeof (*elm->guti), (void*)((uintptr_t )elm->ue_id));

      char                                    guti_str[GUTI2STR_MAX_LENGTH];
      GUTI2STR (elm->guti, guti_str, GUTI2STR_MAX_LENGTH);
      if (HASH_TABLE_OK == h_rc) {
        OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with GUTI %s\n", elm->ue_id, guti_str);
        return RETURNok;
      } else {
        OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with GUTI %s Failed %s\n", elm->ue_id, guti_str, hashtable_rc_code2string (h_rc));
        return RETURNerror;
      }
    } else
      return RETURNok;
  } else {
    OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX - Add in context %p UE id " MME_UE_S1AP_ID_FMT " Failed %s\n", elm, elm->ue_id, hashtable_rc_code2string (h_rc));
    return RETURNerror;
  }
}

void free_emm_data_context(
    struct emm_data_context_s * const emm_ctx)
{
  if (emm_ctx ) {
    if (emm_ctx->imsi) {
      FREE_CHECK (emm_ctx->imsi);
    }
    if (emm_ctx->guti) {
      FREE_CHECK (emm_ctx->guti);
    }
    if (emm_ctx->old_guti) {
      FREE_CHECK (emm_ctx->old_guti);
    }
    if (emm_ctx->imei) {
      FREE_CHECK (emm_ctx->imei);
    }
    if (emm_ctx->imeisv) {
      FREE_CHECK (emm_ctx->imeisv);
    }
    if (emm_ctx->ue_network_capability_ie) {
      FREE_CHECK (emm_ctx->ue_network_capability_ie);
    }
    if (emm_ctx->ms_network_capability_ie) {
      FREE_CHECK (emm_ctx->ms_network_capability_ie);
    }
    if (emm_ctx->drx_parameter) {
      FREE_CHECK (emm_ctx->drx_parameter);
    }
    if (emm_ctx->eps_bearer_context_status) {
      FREE_CHECK (emm_ctx->eps_bearer_context_status);
    }
    /*
     * Release NAS security context
     */
    if (emm_ctx->security) {
      emm_security_context_t                 *security = emm_ctx->security;

      if (security->kasme.value) {
        FREE_CHECK (security->kasme.value);
      }

      if (security->knas_enc.value) {
        FREE_CHECK (security->knas_enc.value);
      }

      if (security->knas_int.value) {
        FREE_CHECK (security->knas_int.value);
      }

      FREE_CHECK (emm_ctx->security);
    }
    if (emm_ctx->non_current_security) {
      emm_security_context_t                 *security = emm_ctx->non_current_security;

      if (security->kasme.value) {
        FREE_CHECK (security->kasme.value);
      }

      if (security->knas_enc.value) {
        FREE_CHECK (security->knas_enc.value);
      }

      if (security->knas_int.value) {
        FREE_CHECK (security->knas_int.value);
      }

      FREE_CHECK (emm_ctx->non_current_security);
    }

    if (emm_ctx->esm_msg.length > 0) {
      FREE_CHECK (emm_ctx->esm_msg.value);
    }


    /*
     * Stop timer T3450
     */
    if (emm_ctx->T3450.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3450 (%d)\n", emm_ctx->T3450.id);
      emm_ctx->T3450.id = nas_timer_stop (emm_ctx->T3450.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3450 stopped UE " MME_UE_S1AP_ID_FMT " ", emm_ctx->ue_id);
    }

    /*
     * Stop timer T3460
     */
    if (emm_ctx->T3460.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3460 (%d)\n", emm_ctx->T3460.id);
      emm_ctx->T3460.id = nas_timer_stop (emm_ctx->T3460.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3460 stopped UE " MME_UE_S1AP_ID_FMT " ", emm_ctx->ue_id);
    }

    /*
     * Stop timer T3470
     */
    if (emm_ctx->T3470.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3470 (%d)\n", emm_ctx->T3460.id);
      emm_ctx->T3470.id = nas_timer_stop (emm_ctx->T3470.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3470 stopped UE " MME_UE_S1AP_ID_FMT " ", emm_ctx->ue_id);
    }
    free_esm_data_context(&emm_ctx->esm_data_ctx);
  }
}

void
emm_data_context_dump (
  const struct emm_data_context_s * const elm_pP)
{
  if (elm_pP ) {
    char                                    imsi_str[16];
    char                                    guti_str[GUTI2STR_MAX_LENGTH];
    int                                     k = 0,
                                            size = 0,
                                            remaining_size = 0;
    char                                    key_string[KASME_LENGTH_OCTETS * 2];

    OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX: ue id:           " MME_UE_S1AP_ID_FMT " (UE identifier)\n", elm_pP->ue_id);
    OAILOG_INFO (LOG_NAS_EMM, "         is_dynamic:       %u      (Dynamically allocated context indicator)\n", elm_pP->is_dynamic);
    OAILOG_INFO (LOG_NAS_EMM, "         is_attached:      %u      (Attachment indicator)\n", elm_pP->is_attached);
    OAILOG_INFO (LOG_NAS_EMM, "         is_emergency:     %u      (Emergency bearer services indicator)\n", elm_pP->is_emergency);
    NAS_IMSI2STR (elm_pP->imsi, imsi_str, 16);
    OAILOG_INFO (LOG_NAS_EMM, "         imsi:             %s      (The IMSI provided by the UE or the MME)\n", imsi_str);
    OAILOG_INFO (LOG_NAS_EMM, "         imei:             TODO    (The IMEI provided by the UE)\n");
    OAILOG_INFO (LOG_NAS_EMM, "         imeisv:           TODO    (The IMEISV provided by the UE)\n");
    OAILOG_INFO (LOG_NAS_EMM, "         guti_is_new:      %u      (New GUTI indicator)", elm_pP->guti_is_new);
    GUTI2STR (elm_pP->guti, guti_str, GUTI2STR_MAX_LENGTH);
    OAILOG_INFO (LOG_NAS_EMM, "         guti:             %s      (The GUTI assigned to the UE)\n", guti_str);
    GUTI2STR (elm_pP->old_guti, guti_str, GUTI2STR_MAX_LENGTH);
    OAILOG_INFO (LOG_NAS_EMM, "         old_guti:         %s      (The old GUTI)\n", guti_str);
    for (k=0; k < elm_pP->tai_list.n_tais; k++) {
      OAILOG_INFO (LOG_NAS_EMM, "         tai:              %u%u%u%u%u%u:0x%04x   (Tracking area identity the UE is registered to)\n",
        elm_pP->tai_list.tai[k].plmn.mcc_digit1,
        elm_pP->tai_list.tai[k].plmn.mcc_digit2,
        elm_pP->tai_list.tai[k].plmn.mcc_digit3,
        elm_pP->tai_list.tai[k].plmn.mnc_digit1,
        elm_pP->tai_list.tai[k].plmn.mnc_digit2,
        elm_pP->tai_list.tai[k].plmn.mnc_digit3,
        elm_pP->tai_list.tai[k].tac);
    }
    OAILOG_INFO (LOG_NAS_EMM, "         ksi:              %u      (Security key set identifier provided by the UE)\n", elm_pP->ksi);
    OAILOG_INFO (LOG_NAS_EMM, "         auth_vector:              (EPS authentication vector)\n");
    OAILOG_INFO (LOG_NAS_EMM, "             kasme: " KASME_FORMAT "" KASME_FORMAT "\n", KASME_DISPLAY_1 (elm_pP->vector.kasme), KASME_DISPLAY_2 (elm_pP->vector.kasme));
    OAILOG_INFO (LOG_NAS_EMM, "             rand:  " RAND_FORMAT "\n", RAND_DISPLAY (elm_pP->vector.rand));
    OAILOG_INFO (LOG_NAS_EMM, "             autn:  " AUTN_FORMAT "\n", AUTN_DISPLAY (elm_pP->vector.autn));

    for (k = 0; k < XRES_LENGTH_MAX; k++) {
      sprintf (&key_string[k * 3], "%02x,", elm_pP->vector.xres[k]);
    }

    key_string[k * 3 - 1] = '\0';
    OAILOG_INFO (LOG_NAS_EMM, "             xres:  %s\n", key_string);

    if (elm_pP->security ) {
      OAILOG_INFO (LOG_NAS_EMM, "         security context:          (Current EPS NAS security context)\n");
      OAILOG_INFO (LOG_NAS_EMM, "             type:  %s              (Type of security context)\n", (elm_pP->security->type == EMM_KSI_NOT_AVAILABLE) ? "KSI_NOT_AVAILABLE" : (elm_pP->security->type == EMM_KSI_NATIVE) ? "KSI_NATIVE" : "KSI_MAPPED");
      OAILOG_INFO (LOG_NAS_EMM, "             eksi:  %u              (NAS key set identifier for E-UTRAN)\n", elm_pP->security->eksi);

      remaining_size = KASME_LENGTH_OCTETS * 2;
      if (elm_pP->security->kasme.length > 0) {
        size = 0;
        size = 0;

        for (k = 0; k < elm_pP->security->kasme.length; k++) {
          size += snprintf (&key_string[size], remaining_size, "0x%x ", elm_pP->security->kasme.value[k]);
          remaining_size -= size;
        }
      } else {
        size += snprintf (&key_string[0], remaining_size, "None");
      }

      OAILOG_INFO (LOG_NAS_EMM, "             kasme: %s     (ASME security key (native context))\n", key_string);

      if (elm_pP->security->knas_enc.length > 0) {
        size = 0;
        size = 0;
        remaining_size = KASME_LENGTH_OCTETS * 2;

        for (k = 0; k < elm_pP->security->knas_enc.length; k++) {
          size += snprintf (&key_string[size], remaining_size, "0x%x ", elm_pP->security->knas_enc.value[k]);
          remaining_size -= size;
        }
      } else {
        size += snprintf (&key_string[0], KASME_LENGTH_OCTETS * 2, "None");
      }

      OAILOG_INFO (LOG_NAS_EMM, "             knas_enc: %s     (NAS cyphering key)\n", key_string);

      if (elm_pP->security->knas_int.length > 0) {
        size = 0;
        remaining_size = KASME_LENGTH_OCTETS * 2;

        for (k = 0; k < elm_pP->security->knas_int.length; k++) {
          size += snprintf (&key_string[size], remaining_size, "0x%x ", elm_pP->security->knas_int.value[k]);
          remaining_size -= size;
        }
      } else {
        size += snprintf (&key_string[0], KASME_LENGTH_OCTETS * 2, "None");
      }

      OAILOG_INFO (LOG_NAS_EMM, "             knas_int: %s     (NAS integrity key)\n", key_string);
      OAILOG_INFO (LOG_NAS_EMM, "             dl_count.overflow: %u\n", elm_pP->security->dl_count.overflow);
      OAILOG_INFO (LOG_NAS_EMM, "             dl_count.seq_num:  %u\n", elm_pP->security->dl_count.seq_num);
      OAILOG_INFO (LOG_NAS_EMM, "             ul_count.overflow: %u\n", elm_pP->security->ul_count.overflow);
      OAILOG_INFO (LOG_NAS_EMM, "             ul_count.seq_num:  %u\n", elm_pP->security->ul_count.seq_num);
      OAILOG_INFO (LOG_NAS_EMM, "             TODO  capability");
      OAILOG_INFO (LOG_NAS_EMM, "             selected_algorithms.encryption:  %x\n", elm_pP->security->selected_algorithms.encryption);
      OAILOG_INFO (LOG_NAS_EMM, "             selected_algorithms.integrity:   %x\n", elm_pP->security->selected_algorithms.integrity);
    } else {
      OAILOG_INFO (LOG_NAS_EMM, "         No security context\n");
    }

    OAILOG_INFO (LOG_NAS_EMM, "         _emm_fsm_status     %u\n", elm_pP->_emm_fsm_status);
    OAILOG_INFO (LOG_NAS_EMM, "         TODO  esm_data_ctx\n");
  }
}

static bool
emm_data_context_dump_hash_table_wrapper (
  hash_key_t keyP,
  void *dataP,
  void *parameterP,
  void**resultP)
{
  emm_data_context_dump (dataP);
  return false; // otherwise dump stop
}

void
emm_data_context_dump_all (
  void)
{
  OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX - Dump all contexts:\n");
  hashtable_ts_apply_callback_on_elements (_emm_data.ctx_coll_ue_id, emm_data_context_dump_hash_table_wrapper, NULL, NULL);
}

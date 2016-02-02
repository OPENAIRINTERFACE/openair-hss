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

#if NAS_BUILT_IN_EPC
#  include "assertions.h"
#  include "tree.h"
#  include "emmData.h"
#  include "log.h"
#  include "security_types.h"
#  include "obj_hashtable.h"
#  include "msc.h"

struct emm_data_context_s              *
emm_data_context_get (
  emm_data_t * emm_data,
  const unsigned int ueid)
{
  struct emm_data_context_s              *emm_data_context_p = NULL;

  DevCheck (ueid > 0, ueid, 0, 0);
  DevAssert (emm_data != NULL);
  hashtable_ts_get (emm_data->ctx_coll_ue_id, (const hash_key_t)(ueid), (void **)&emm_data_context_p);
  LOG_TRACE (INFO, "EMM-CTX - get UE id " NAS_UE_ID_FMT " context %p", ueid, emm_data_context_p);
  return emm_data_context_p;
}


struct emm_data_context_s              *
emm_data_context_get_by_guti (
  emm_data_t * emm_data,
  GUTI_t * guti)
{
  hashtable_rc_t                          h_rc;
  unsigned int                            emm_ue_id;

  DevAssert (emm_data != NULL);

  if (NULL != guti) {
    char                                    guti_str[GUTI2STR_MAX_LENGTH];

    GUTI2STR (guti, guti_str, GUTI2STR_MAX_LENGTH);
    h_rc = obj_hashtable_ts_get (emm_data->ctx_coll_guti, (const void *)guti, sizeof (*guti), (void **)&emm_ue_id);

    if (h_rc == HASH_TABLE_OK) {
      LOG_TRACE (INFO, "EMM-CTX - get_by_guti UE id " NAS_UE_ID_FMT " %s", emm_ue_id, guti_str);
      return emm_data_context_get (emm_data, (const hash_key_t)emm_ue_id);
    }

    LOG_TRACE (INFO, "EMM-CTX - get_by_guti UE id " NAS_UE_ID_FMT " %s failed", emm_ue_id, guti_str);
    LOG_TRACE (INFO, "EMM-CTX - get_by_guti UE id " NAS_UE_ID_FMT " PLMN    %01x%01x%01x%01x%01x%01x failed",
               emm_ue_id, guti->gummei.plmn.MCCdigit1, guti->gummei.plmn.MCCdigit2, guti->gummei.plmn.MNCdigit3, guti->gummei.plmn.MNCdigit1, guti->gummei.plmn.MNCdigit2, guti->gummei.plmn.MCCdigit3);
    LOG_TRACE (INFO, "EMM-CTX - get_by_guti UE id " NAS_UE_ID_FMT " MMEgid  %04x failed", emm_ue_id, guti->gummei.MMEgid);
    LOG_TRACE (INFO, "EMM-CTX - get_by_guti UE id " NAS_UE_ID_FMT " MMEcode %01x failed", emm_ue_id, guti->gummei.MMEcode);
    LOG_TRACE (INFO, "EMM-CTX - get_by_guti UE id " NAS_UE_ID_FMT " m_tmsi  %08x failed", emm_ue_id, guti->m_tmsi);
  }

  return NULL;
}



struct emm_data_context_s              *
emm_data_context_remove (
  emm_data_t * emm_data,
  struct emm_data_context_s *elm)
{
  struct emm_data_context_s              *emm_data_context_p = NULL;
  unsigned int                           *emm_ue_id = NULL;

  LOG_TRACE (INFO, "EMM-CTX - Remove in context %p UE id " NAS_UE_ID_FMT " ", elm, elm->ueid);

  if (NULL != elm->guti) {
    char                                    guti_str[GUTI2STR_MAX_LENGTH];

    GUTI2STR (elm->guti, guti_str, GUTI2STR_MAX_LENGTH);
    obj_hashtable_ts_remove (emm_data->ctx_coll_guti, (const void *)(elm->guti), sizeof (*elm->guti), (void **)&emm_ue_id);
    LOG_TRACE (INFO, "EMM-CTX - Remove in ctx_coll_guti context %p UE id " NAS_UE_ID_FMT " guti %s", elm, emm_ue_id, guti_str);
  }

  hashtable_ts_remove (emm_data->ctx_coll_ue_id, (const hash_key_t)(elm->ueid), (void **)&emm_data_context_p);
  return emm_data_context_p;
}


int
emm_data_context_add (
  emm_data_t * emm_data,
  struct emm_data_context_s *elm)
{
  hashtable_rc_t                          h_rc;

  h_rc = hashtable_ts_insert (emm_data->ctx_coll_ue_id, (const hash_key_t)(elm->ueid), elm);

  if (h_rc == HASH_TABLE_OK) {
    LOG_TRACE (INFO, "EMM-CTX - Add in context %p UE id " NAS_UE_ID_FMT " ", elm, elm->ueid);

    if (NULL != elm->guti) {
      char                                    guti_str[GUTI2STR_MAX_LENGTH];

      GUTI2STR (elm->guti, guti_str, GUTI2STR_MAX_LENGTH);
      h_rc = obj_hashtable_ts_insert (emm_data->ctx_coll_guti, (void *)(elm->guti), sizeof (*elm->guti), (void *)elm->ueid);

      if (h_rc == HASH_TABLE_OK) {
        LOG_TRACE (INFO, "EMM-CTX - Add in context UE id " NAS_UE_ID_FMT " with GUTI %s", elm->ueid, guti_str);
        return RETURNok;
      } else {
        LOG_TRACE (INFO, "EMM-CTX - Add in context UE id " NAS_UE_ID_FMT " with GUTI %s Failed %s", elm->ueid, guti_str, hashtable_rc_code2string (h_rc));
        return RETURNerror;
      }
    } else
      return RETURNok;
  } else {
    LOG_TRACE (INFO, "EMM-CTX - Add in context %p UE id " NAS_UE_ID_FMT " Failed %s", elm, elm->ueid, hashtable_rc_code2string (h_rc));
    return RETURNerror;
  }
}

void free_emm_data_context(
    struct emm_data_context_s * const emm_ctx)
{
  if (emm_ctx != NULL) {
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
      LOG_TRACE (INFO, "EMM-PROC  - Stop timer T3450 (%d)", emm_ctx->T3450.id);
      emm_ctx->T3450.id = nas_timer_stop (emm_ctx->T3450.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3450 stopped UE " NAS_UE_ID_FMT " ", emm_ctx->ueid);
    }

    /*
     * Stop timer T3460
     */
    if (emm_ctx->T3460.id != NAS_TIMER_INACTIVE_ID) {
      LOG_TRACE (INFO, "EMM-PROC  - Stop timer T3460 (%d)", emm_ctx->T3460.id);
      emm_ctx->T3460.id = nas_timer_stop (emm_ctx->T3460.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3460 stopped UE " NAS_UE_ID_FMT " ", emm_ctx->ueid);
    }

    /*
     * Stop timer T3470
     */
    if (emm_ctx->T3470.id != NAS_TIMER_INACTIVE_ID) {
      LOG_TRACE (INFO, "EMM-PROC  - Stop timer T3470 (%d)", emm_ctx->T3460.id);
      emm_ctx->T3470.id = nas_timer_stop (emm_ctx->T3470.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3470 stopped UE " NAS_UE_ID_FMT " ", emm_ctx->ueid);
    }
    free_esm_data_context(&emm_ctx->esm_data_ctx);
  }
}

void
emm_data_context_dump (
  const struct emm_data_context_s * const elm_pP)
{
  if (elm_pP != NULL) {
    char                                    imsi_str[16];
    char                                    guti_str[GUTI2STR_MAX_LENGTH];
    int                                     k,
                                            size,
                                            remaining_size;
    char                                    key_string[KASME_LENGTH_OCTETS * 2];

    LOG_TRACE (INFO, "EMM-CTX: ue id:           " NAS_UE_ID_FMT " (UE identifier)", elm_pP->ueid);
    LOG_TRACE (INFO, "         is_dynamic:       %u      (Dynamically allocated context indicator)", elm_pP->is_dynamic);
    LOG_TRACE (INFO, "         is_attached:      %u      (Attachment indicator)", elm_pP->is_attached);
    LOG_TRACE (INFO, "         is_emergency:     %u      (Emergency bearer services indicator)", elm_pP->is_emergency);
    NAS_IMSI2STR (elm_pP->imsi, imsi_str, 16);
    LOG_TRACE (INFO, "         imsi:             %s      (The IMSI provided by the UE or the MME)", imsi_str);
    LOG_TRACE (INFO, "         imei:             TODO    (The IMEI provided by the UE)");
    LOG_TRACE (INFO, "         guti_is_new:      %u      (New GUTI indicator)", elm_pP->guti_is_new);
    GUTI2STR (elm_pP->guti, guti_str, GUTI2STR_MAX_LENGTH);
    LOG_TRACE (INFO, "         guti:             %s      (The GUTI assigned to the UE)", guti_str);
    GUTI2STR (elm_pP->old_guti, guti_str, GUTI2STR_MAX_LENGTH);
    LOG_TRACE (INFO, "         old_guti:         %s      (The old GUTI)", guti_str);
    for (k=0; k < elm_pP->tai_list.n_tais; k++) {
    LOG_TRACE (INFO, "         tai:              %u%u%u%u%u%u:0x%04x   (Tracking area identity the UE is registered to)",
        elm_pP->tai_list.tai[k].plmn.MCCdigit1,
        elm_pP->tai_list.tai[k].plmn.MCCdigit2,
        elm_pP->tai_list.tai[k].plmn.MCCdigit3,
        elm_pP->tai_list.tai[k].plmn.MNCdigit1,
        elm_pP->tai_list.tai[k].plmn.MNCdigit2,
        elm_pP->tai_list.tai[k].plmn.MNCdigit3,
        elm_pP->tai_list.tai[k].tac);
    }
    LOG_TRACE (INFO, "         ksi:              %u      (Security key set identifier provided by the UE)", elm_pP->ksi);
    LOG_TRACE (INFO, "         auth_vector:              (EPS authentication vector)");
    LOG_TRACE (INFO, "             kasme: " KASME_FORMAT "" KASME_FORMAT, KASME_DISPLAY_1 (elm_pP->vector.kasme), KASME_DISPLAY_2 (elm_pP->vector.kasme));
    LOG_TRACE (INFO, "             rand:  " RAND_FORMAT, RAND_DISPLAY (elm_pP->vector.rand));
    LOG_TRACE (INFO, "             autn:  " AUTN_FORMAT, AUTN_DISPLAY (elm_pP->vector.autn));

    for (k = 0; k < XRES_LENGTH_MAX; k++) {
      sprintf (&key_string[k * 3], "%02x,", elm_pP->vector.xres[k]);
    }

    key_string[k * 3 - 1] = '\0';
    LOG_TRACE (INFO, "             xres:  %s\n", key_string);

    if (elm_pP->security != NULL) {
      LOG_TRACE (INFO, "         security context:          (Current EPS NAS security context)");
      LOG_TRACE (INFO, "             type:  %s              (Type of security context)", (elm_pP->security->type == EMM_KSI_NOT_AVAILABLE) ? "KSI_NOT_AVAILABLE" : (elm_pP->security->type == EMM_KSI_NATIVE) ? "KSI_NATIVE" : "KSI_MAPPED");
      LOG_TRACE (INFO, "             eksi:  %u              (NAS key set identifier for E-UTRAN)", elm_pP->security->eksi);

      if (elm_pP->security->kasme.length > 0) {
        size = 0;
        size = 0;
        remaining_size = KASME_LENGTH_OCTETS * 2;

        for (k = 0; k < elm_pP->security->kasme.length; k++) {
          size += snprintf (&key_string[size], remaining_size, "0x%x ", elm_pP->security->kasme.value[k]);
          remaining_size -= size;
        }
      } else {
        size += snprintf (&key_string[0], remaining_size, "None");
      }

      LOG_TRACE (INFO, "             kasme: %s     (ASME security key (native context))", key_string);

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

      LOG_TRACE (INFO, "             knas_enc: %s     (NAS cyphering key)", key_string);

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

      LOG_TRACE (INFO, "             knas_int: %s     (NAS integrity key)", key_string);
      LOG_TRACE (INFO, "             dl_count.overflow: %u     ", elm_pP->security->dl_count.overflow);
      LOG_TRACE (INFO, "             dl_count.seq_num:  %u     ", elm_pP->security->dl_count.seq_num);
      LOG_TRACE (INFO, "             ul_count.overflow: %u     ", elm_pP->security->ul_count.overflow);
      LOG_TRACE (INFO, "             ul_count.seq_num:  %u     ", elm_pP->security->ul_count.seq_num);
      LOG_TRACE (INFO, "             TODO  capability");
      LOG_TRACE (INFO, "             selected_algorithms.encryption:  %x     ", elm_pP->security->selected_algorithms.encryption);
      LOG_TRACE (INFO, "             selected_algorithms.integrity:   %x     ", elm_pP->security->selected_algorithms.integrity);
    } else {
      LOG_TRACE (INFO, "         No security context");
    }

    LOG_TRACE (INFO, "         _emm_fsm_status     %u   ", elm_pP->_emm_fsm_status);
    LOG_TRACE (INFO, "         TODO  esm_data_ctx");
  }
}

static void
emm_data_context_dump_hash_table_wrapper (
  hash_key_t keyP,
  void *dataP,
  void *parameterP)
{
  emm_data_context_dump (dataP);
}

void
emm_data_context_dump_all (
  void)
{
  LOG_TRACE (INFO, "EMM-CTX - Dump all contexts:");
  hashtable_ts_apply_funct_on_elements (_emm_data.ctx_coll_ue_id, emm_data_context_dump_hash_table_wrapper, NULL);
}
#endif

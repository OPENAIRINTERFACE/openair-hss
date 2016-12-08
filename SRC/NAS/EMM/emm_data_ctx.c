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
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "assertions.h"
#include "log.h"
#include "msc.h"
#include "tree.h"
#include "3gpp_24.007.h"
#include "3gpp_24.301.h"
#include "common_types.h"
#include "commonDef.h"
#include "common_defs.h"
#include "mme_app_ue_context.h"
#include "NasSecurityAlgorithms.h"
#include "conversions.h"
#include "emm_data.h"
#include "esm_data.h"
#include "emm_proc.h"
#include "security_types.h"
#include "mme_config.h"
#include "secu_defs.h"
#include "emm_cause.h"

static mme_ue_s1ap_id_t mme_ue_s1ap_id_generator = 1;

//------------------------------------------------------------------------------
static bool emm_context_dump_hash_table_wrapper (
  hash_key_t keyP,
  void *dataP,
  void *parameterP,
  void**resultP);

mme_ue_s1ap_id_t emm_ctx_get_new_ue_id(emm_context_t *ctxt)
{
  mme_ue_s1ap_id_t tmp = 0;
  if (RUN_MODE_TEST == mme_config.run_mode) {
    tmp = __sync_fetch_and_add (&mme_ue_s1ap_id_generator, 1);
  } else {
    tmp = (mme_ue_s1ap_id_t)((uint)((uintptr_t)ctxt) >> 4);// ^ 0xBABA53AB;
  }
  return tmp;
}

//------------------------------------------------------------------------------
inline void emm_ctx_mark_common_procedure_running(emm_context_t * const ctxt, const int proc_id)
{
  __sync_fetch_and_or(&ctxt->common_proc_mask, proc_id);
}

inline void emm_ctx_unmark_common_procedure_running(emm_context_t * const ctxt, const int proc_id)
{
  __sync_fetch_and_and(&ctxt->common_proc_mask, ~proc_id);
}

inline bool emm_ctx_is_common_procedure_running(emm_context_t * const ctxt, const int proc_id)
{
  if ((ctxt) && (ctxt->common_proc_mask & proc_id)) return true;
  return false;
}

inline void emm_ctx_mark_specific_procedure_running(emm_context_t * const ctxt, const int proc_id)
{
  __sync_fetch_and_or(&ctxt->specific_proc_mask, proc_id);
}

inline void emm_ctx_unmark_specific_procedure_running(emm_context_t * const ctxt, const int proc_id)
{
  __sync_fetch_and_and(&ctxt->specific_proc_mask, ~proc_id);
}

inline bool emm_ctx_is_specific_procedure_running(emm_context_t * const ctxt, const int proc_id)
{
  if ((ctxt) && (ctxt->specific_proc_mask & proc_id)) return true;
  return false;
}


//------------------------------------------------------------------------------
inline void emm_ctx_set_attribute_present(emm_context_t * const ctxt, const int attribute_bit_pos)
{
  ctxt->member_present_mask |= attribute_bit_pos;
}

inline void emm_ctx_clear_attribute_present(emm_context_t * const ctxt, const int attribute_bit_pos)
{
  ctxt->member_present_mask &= ~attribute_bit_pos;
  ctxt->member_valid_mask &= ~attribute_bit_pos;
}

inline void emm_ctx_set_attribute_valid(emm_context_t * const ctxt, const int attribute_bit_pos)
{
  ctxt->member_present_mask |= attribute_bit_pos;
  ctxt->member_valid_mask   |= attribute_bit_pos;
}

inline void emm_ctx_clear_attribute_valid(emm_context_t * const ctxt, const int attribute_bit_pos)
{
  ctxt->member_valid_mask &= ~attribute_bit_pos;
}

//------------------------------------------------------------------------------
/* Clear GUTI  */
inline void emm_ctx_clear_guti(emm_context_t * const ctxt)
{
  clear_guti(&ctxt->_guti);
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_GUTI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " GUTI cleared\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set GUTI */
inline void emm_ctx_set_guti(emm_context_t * const ctxt, guti_t *guti)
{
  ctxt->_guti = *guti;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_GUTI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set GUTI " GUTI_FMT " (present)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, GUTI_ARG(&ctxt->_guti));
}

/* Set GUTI, mark it as valid */
inline void emm_ctx_set_valid_guti(emm_context_t * const ctxt, guti_t *guti)
{
  ctxt->_guti = *guti;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_GUTI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set GUTI " GUTI_FMT " (valid)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, GUTI_ARG(&ctxt->_guti));
}

//------------------------------------------------------------------------------
/* Clear old GUTI  */
inline void emm_ctx_clear_old_guti(emm_context_t * const ctxt)
{
  clear_guti(&ctxt->_old_guti);
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_OLD_GUTI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " old GUTI cleared\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set GUTI */
inline void emm_ctx_set_old_guti(emm_context_t * const ctxt, guti_t *guti)
{
  ctxt->_old_guti = *guti;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_OLD_GUTI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set old GUTI " GUTI_FMT " (present)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, GUTI_ARG(&ctxt->_old_guti));
}

/* Set GUTI, mark it as valid */
inline void emm_ctx_set_valid_old_guti(emm_context_t * const ctxt, guti_t *guti)
{
  ctxt->_old_guti = *guti;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_OLD_GUTI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set old GUTI " GUTI_FMT " (valid)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, GUTI_ARG(&ctxt->_old_guti));
}

//------------------------------------------------------------------------------
/* Clear IMSI */
inline void emm_ctx_clear_imsi(emm_context_t * const ctxt)
{
  clear_imsi(&ctxt->_imsi);
  ctxt->_imsi64 = INVALID_IMSI64;
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_IMSI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared IMSI\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set IMSI */
inline void emm_ctx_set_imsi(emm_context_t * const ctxt, imsi_t *imsi, const imsi64_t imsi64)
{
  ctxt->_imsi   = *imsi;
  ctxt->_imsi64 = imsi64;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_IMSI);
#if  DEBUG_IS_ON
  char     imsi_str[IMSI_BCD_DIGITS_MAX+1] = {0};
  IMSI64_TO_STRING (ctxt->_imsi64, imsi_str);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set IMSI %s (valid)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, imsi_str);
#endif
}

/* Set IMSI, mark it as valid */
inline void emm_ctx_set_valid_imsi(emm_context_t * const ctxt, imsi_t *imsi, const imsi64_t imsi64)
{
  ctxt->_imsi   = *imsi;
  ctxt->_imsi64 = imsi64;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_IMSI);
#if  DEBUG_IS_ON
  char     imsi_str[IMSI_BCD_DIGITS_MAX+1] = {0};
  IMSI64_TO_STRING (ctxt->_imsi64, imsi_str);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set IMSI %s (valid)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, imsi_str);
#endif
  mme_api_notify_imsi((PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, imsi64);
  hashtable_ts_insert (_emm_data.ctx_coll_imsi, imsi64, (void*)((uintptr_t )(PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id));
}

//------------------------------------------------------------------------------
/* Clear IMEI */
inline void emm_ctx_clear_imei(emm_context_t * const ctxt)
{
  clear_imei(&ctxt->_imei);
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_IMEI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " IMEI cleared\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set IMEI */
inline void emm_ctx_set_imei(emm_context_t * const ctxt, imei_t *imei)
{
  ctxt->_imei = *imei;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_IMEI);
#if  DEBUG_IS_ON
  char     imei_str[16];
  IMEI_TO_STRING (imei, imei_str, 16);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set IMEI %s (present)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, imei_str);
#endif
}

/* Set IMEI, mark it as valid */
inline void emm_ctx_set_valid_imei(emm_context_t * const ctxt, imei_t *imei)
{
  ctxt->_imei = *imei;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_IMEI);
#if  DEBUG_IS_ON
  char     imei_str[16];
  IMEI_TO_STRING (imei, imei_str, 16);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set IMEI %s (valid)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, imei_str);
#endif
}

//------------------------------------------------------------------------------
/* Clear IMEI_SV */
inline void emm_ctx_clear_imeisv(emm_context_t * const ctxt)
{
  clear_imeisv(&ctxt->_imeisv);
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_IMEI_SV);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared IMEI_SV \n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set IMEI_SV */
inline void emm_ctx_set_imeisv(emm_context_t * const ctxt, imeisv_t *imeisv)
{
  ctxt->_imeisv = *imeisv;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_IMEI_SV);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set IMEI_SV (present)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set IMEI_SV, mark it as valid */
inline void emm_ctx_set_valid_imeisv(emm_context_t * const ctxt, imeisv_t *imeisv)
{
  ctxt->_imeisv = *imeisv;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_IMEI_SV);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set IMEI_SV (valid)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

//------------------------------------------------------------------------------
/* Clear last_visited_registered_tai */
inline void emm_ctx_clear_lvr_tai(emm_context_t * const ctxt)
{
  clear_tai(&ctxt->_lvr_tai);
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_LVR_TAI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared last visited registered TAI\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set last_visited_registered_tai */
inline void emm_ctx_set_lvr_tai(emm_context_t * const ctxt, tai_t *lvr_tai)
{
  ctxt->_lvr_tai = *lvr_tai;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_LVR_TAI);
  //log_message(NULL, OAILOG_LEVEL_DEBUG,    LOG_NAS_EMM, __FILE__, __LINE__,
  //    "ue_id="MME_UE_S1AP_ID_FMT" set last visited registered TAI "TAI_FMT" (present)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, TAI_ARG(&ctxt->_lvr_tai));

  //OAILOG_DEBUG (LOG_NAS_EMM, "ue_id="MME_UE_S1AP_ID_FMT" set last visited registered TAI "TAI_FMT" (present)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, TAI_ARG(&ctxt->_lvr_tai));
}

/* Set last_visited_registered_tai, mark it as valid */
inline void emm_ctx_set_valid_lvr_tai(emm_context_t * const ctxt, tai_t *lvr_tai)
{
  ctxt->_lvr_tai = *lvr_tai;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_LVR_TAI);
  //OAILOG_DEBUG (LOG_NAS_EMM, "ue_id="MME_UE_S1AP_ID_FMT" set last visited registered TAI "TAI_FMT" (valid)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, TAI_ARG(&ctxt->_lvr_tai));
}

//------------------------------------------------------------------------------
/* Clear AUTH vectors  */
inline void emm_ctx_clear_auth_vectors(emm_context_t * const ctxt)
{
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_AUTH_VECTORS);
  for (int i = 0; i < MAX_EPS_AUTH_VECTORS; i++) {
    memset((void *)&ctxt->_vector[i], 0, sizeof(ctxt->_vector[i]));
    emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_AUTH_VECTOR0+i);
  }
  emm_ctx_clear_security_vector_index(ctxt);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared auth vectors \n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}
//------------------------------------------------------------------------------
/* Clear AUTH vector  */
inline void emm_ctx_clear_auth_vector(emm_context_t * const ctxt, ksi_t eksi)
{
  AssertFatal(eksi < MAX_EPS_AUTH_VECTORS, "Out of bounds eksi %d", eksi);
  memset((void *)&ctxt->_vector[eksi], 0, sizeof(ctxt->_vector[eksi]));
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_AUTH_VECTOR0+eksi);
  int remaining_vectors = 0;
  for (int i = 0; i < MAX_EPS_AUTH_VECTORS; i++) {
    if (IS_EMM_CTXT_VALID_AUTH_VECTOR(ctxt, i)) {
      remaining_vectors+=1;
    }
  }
  ctxt->remaining_vectors = remaining_vectors;
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared auth vector %u \n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, eksi);
  if (!(remaining_vectors)) {
    emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_AUTH_VECTORS);
    emm_ctx_clear_security_vector_index(ctxt);
    OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared auth vectors\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
  }
}
//------------------------------------------------------------------------------
/* Clear security  */
inline void emm_ctx_clear_security(emm_context_t * const ctxt)
{
  memset (&ctxt->_security, 0, sizeof (ctxt->_security));
  emm_ctx_set_security_type(ctxt, SECURITY_CTX_TYPE_NOT_AVAILABLE);
  emm_ctx_set_security_eksi(ctxt, KSI_NO_KEY_AVAILABLE);
  emm_ctx_clear_security_vector_index(ctxt);
  ctxt->_security.selected_algorithms.encryption = NAS_SECURITY_ALGORITHMS_EEA0;
  ctxt->_security.selected_algorithms.integrity  = NAS_SECURITY_ALGORITHMS_EIA0;
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_SECURITY);
  ctxt->_security.direction_decode = SECU_DIRECTION_UPLINK;
  ctxt->_security.direction_encode = SECU_DIRECTION_DOWNLINK;
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared security context \n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

//------------------------------------------------------------------------------
inline void emm_ctx_set_security_type(emm_context_t * const ctxt, emm_sc_type_t sc_type)
{
  ctxt->_security.sc_type = sc_type;
  OAILOG_TRACE (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set security context security type %d\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, sc_type);
}

//------------------------------------------------------------------------------
inline void emm_ctx_set_security_eksi(emm_context_t * const ctxt, ksi_t eksi)
{
  ctxt->_security.eksi = eksi;
  OAILOG_TRACE (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set security context eksi %d\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, eksi);
}

//------------------------------------------------------------------------------
inline void emm_ctx_clear_security_vector_index(emm_context_t * const ctxt)
{
  ctxt->_security.vector_index = EMM_SECURITY_VECTOR_INDEX_INVALID;
  OAILOG_TRACE (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " clear security context vector index\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}
//------------------------------------------------------------------------------
inline void emm_ctx_set_security_vector_index(emm_context_t * const ctxt, int vector_index)
{
  ctxt->_security.vector_index = vector_index;
  OAILOG_TRACE (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set security context vector index %d\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, vector_index);
}


//------------------------------------------------------------------------------
/* Clear non current security  */
inline void emm_ctx_clear_non_current_security(emm_context_t * const ctxt)
{
  memset (&ctxt->_non_current_security, 0, sizeof (ctxt->_non_current_security));
  ctxt->_non_current_security.sc_type      = SECURITY_CTX_TYPE_NOT_AVAILABLE;
  ctxt->_non_current_security.eksi         = KSI_NO_KEY_AVAILABLE;
  emm_ctx_clear_non_current_security_vector_index(ctxt);
  ctxt->_non_current_security.selected_algorithms.encryption = NAS_SECURITY_ALGORITHMS_EEA0;
  ctxt->_non_current_security.selected_algorithms.integrity  = NAS_SECURITY_ALGORITHMS_EIA0;
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_NON_CURRENT_SECURITY);
  ctxt->_security.direction_decode = SECU_DIRECTION_UPLINK;
  ctxt->_security.direction_encode = SECU_DIRECTION_DOWNLINK;
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared non current security context \n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}
//------------------------------------------------------------------------------
inline void emm_ctx_clear_non_current_security_vector_index(emm_context_t * const ctxt)
{
  ctxt->_non_current_security.vector_index = EMM_SECURITY_VECTOR_INDEX_INVALID;
  OAILOG_TRACE (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " clear non current security context vector index\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}
//------------------------------------------------------------------------------
inline void emm_ctx_set_non_current_security_vector_index(emm_context_t * const ctxt, int vector_index)
{
  ctxt->_non_current_security.vector_index = vector_index;
  OAILOG_TRACE (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set non current security context vector index %d\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, vector_index);
}

//------------------------------------------------------------------------------
/* Clear UE network capability IE   */
inline void emm_ctx_clear_ue_nw_cap(emm_context_t * const ctxt)
{
  memset (&ctxt->_ue_network_capability, 0, sizeof (ctxt->_ue_network_capability));
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_UE_NETWORK_CAPABILITY_IE);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared UE network capability IE\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set UE network capability IE */
inline void emm_ctx_set_ue_nw_cap(emm_context_t * const ctxt, const ue_network_capability_t * const ue_nw_cap_ie)
{
  ctxt->_ue_network_capability = *ue_nw_cap_ie;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_UE_NETWORK_CAPABILITY_IE);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set UE network capability IE (present)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set UE network capability IE, mark it as valid */
inline void emm_ctx_set_valid_ue_nw_cap(emm_context_t * const ctxt, const ue_network_capability_t * const ue_nw_cap_ie)
{
  ctxt->_ue_network_capability = *ue_nw_cap_ie;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_UE_NETWORK_CAPABILITY_IE);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set UE network capability IE (valid)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}


//------------------------------------------------------------------------------
/* Clear MS network capability IE   */
inline void emm_ctx_clear_ms_nw_cap(emm_context_t * const ctxt)
{
  memset (&ctxt->_ms_network_capability, 0, sizeof (ctxt->_ue_network_capability));
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_MS_NETWORK_CAPABILITY_IE);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared MS network capability IE\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set UE network capability IE */
inline void emm_ctx_set_ms_nw_cap(emm_context_t * const ctxt, const ms_network_capability_t * const ms_nw_cap_ie)
{
  ctxt->_ms_network_capability = *ms_nw_cap_ie;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_MS_NETWORK_CAPABILITY_IE);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set MS network capability IE (present)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set UE network capability IE, mark it as valid */
inline void emm_ctx_set_valid_ms_nw_cap(emm_context_t * const ctxt, const ms_network_capability_t * const ms_nw_cap_ie)
{
  ctxt->_ms_network_capability = *ms_nw_cap_ie;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_MS_NETWORK_CAPABILITY_IE);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set MS network capability IE (valid)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

//------------------------------------------------------------------------------
/* Clear current DRX parameter   */
inline void emm_ctx_clear_current_drx_parameter(emm_context_t * const ctxt)
{
  memset (&ctxt->_current_drx_parameter, 0, sizeof (ctxt->_current_drx_parameter));
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_CURRENT_DRX_PARAMETER);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared current DRX parameter\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set current DRX parameter */
inline void emm_ctx_set_current_drx_parameter(emm_context_t * const ctxt, drx_parameter_t *drx)
{
  ctxt->_current_drx_parameter = *drx;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_CURRENT_DRX_PARAMETER);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set current DRX parameter (present)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set current DRX parameter, mark it as valid */
inline void emm_ctx_set_valid_current_drx_parameter(emm_context_t * const ctxt, drx_parameter_t *drx)
{
  ctxt->_current_drx_parameter = *drx;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_CURRENT_DRX_PARAMETER);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set current DRX parameter (valid)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

//------------------------------------------------------------------------------
/* Clear non current DRX parameter   */
inline void emm_ctx_clear_pending_current_drx_parameter(emm_context_t * const ctxt)
{
  memset (&ctxt->_pending_drx_parameter, 0, sizeof (ctxt->_pending_drx_parameter));
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_PENDING_DRX_PARAMETER);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared pending DRX parameter\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set current DRX parameter */
inline void emm_ctx_set_pending_current_drx_parameter(emm_context_t * const ctxt, drx_parameter_t *drx)
{
  ctxt->_pending_drx_parameter = *drx;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_PENDING_DRX_PARAMETER);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set pending DRX parameter (present)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set current DRX parameter, mark it as valid */
inline void emm_ctx_set_valid_pending_current_drx_parameter(emm_context_t * const ctxt, drx_parameter_t *drx)
{
  ctxt->_pending_drx_parameter = *drx;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_PENDING_DRX_PARAMETER);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set pending DRX parameter (valid)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

//------------------------------------------------------------------------------
/* Clear EPS bearer context status   */
inline void emm_ctx_clear_eps_bearer_context_status(emm_context_t * const ctxt)
{
  memset (&ctxt->_eps_bearer_context_status, 0, sizeof (ctxt->_eps_bearer_context_status));
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_EPS_BEARER_CONTEXT_STATUS);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared EPS bearer context status\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set current DRX parameter */
inline void emm_ctx_set_eps_bearer_context_status(emm_context_t * const ctxt, eps_bearer_context_status_t *status)
{
  ctxt->_eps_bearer_context_status = *status;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_EPS_BEARER_CONTEXT_STATUS);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set EPS bearer context status (present)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}

/* Set current DRX parameter, mark it as valid */
inline void emm_ctx_set_valid_eps_bearer_context_status(emm_context_t * const ctxt, eps_bearer_context_status_t *status)
{
  ctxt->_eps_bearer_context_status = *status;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_EPS_BEARER_CONTEXT_STATUS);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set EPS bearer context status (valid)\n", (PARENT_STRUCT(ctxt, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
}


//------------------------------------------------------------------------------
struct emm_context_s              *
emm_context_remove (
  emm_data_t * emm_data,
  struct emm_context_s *elm)
{
  struct emm_context_s              *emm_context_p = NULL;
  mme_ue_s1ap_id_t                       *emm_ue_id          = NULL;

  OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Remove in context %p UE id " MME_UE_S1AP_ID_FMT "\n", elm, (PARENT_STRUCT(elm, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);

  if ( IS_EMM_CTXT_PRESENT_GUTI(elm)) {
    obj_hashtable_ts_remove (emm_data->ctx_coll_guti, (const void *)&elm->_guti, sizeof (elm->_guti), (void **)&emm_ue_id);

    OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Remove in ctx_coll_guti context %p UE id " MME_UE_S1AP_ID_FMT " guti "GUTI_FMT"\n",
        elm, (mme_ue_s1ap_id_t)((uintptr_t)emm_ue_id), GUTI_ARG(&elm->_guti));
  }

  if ( IS_EMM_CTXT_PRESENT_IMSI(elm)) {
    imsi64_t imsi64 = INVALID_IMSI64;
    imsi64 = imsi_to_imsi64(&elm->_imsi);
    hashtable_ts_remove (emm_data->ctx_coll_imsi, (const hash_key_t)imsi64, (void **)&emm_ue_id);

    OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Remove in ctx_coll_imsi context %p UE id " MME_UE_S1AP_ID_FMT " imsi " IMSI_64_FMT "\n",
        elm, (mme_ue_s1ap_id_t)((uintptr_t)emm_ue_id), imsi64);
  }

  hashtable_ts_remove (emm_data->ctx_coll_ue_id, (const hash_key_t)((PARENT_STRUCT(elm, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id), (void **)&emm_context_p);
  return emm_context_p;
}


//------------------------------------------------------------------------------
int
emm_context_add_guti (
  emm_data_t * emm_data,
  struct emm_context_s *elm)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;

  if ( IS_EMM_CTXT_PRESENT_GUTI(elm)) {
    h_rc = obj_hashtable_ts_insert (emm_data->ctx_coll_guti, (const void *const)(&elm->_guti), sizeof (elm->_guti), (void*)((uintptr_t )(PARENT_STRUCT(elm, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id));

    if (HASH_TABLE_OK == h_rc) {
      OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with GUTI "GUTI_FMT"\n", (PARENT_STRUCT(elm, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, GUTI_ARG(&elm->_guti));
    } else {
      OAILOG_ERROR (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with GUTI "GUTI_FMT" Failed %s\n", (PARENT_STRUCT(elm, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, GUTI_ARG(&elm->_guti), hashtable_rc_code2string (h_rc));
      return RETURNerror;
    }
  }
  return RETURNok;
}
//------------------------------------------------------------------------------
int
emm_context_add_old_guti (
  emm_data_t * emm_data,
  struct emm_context_s *elm)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;

  if ( IS_EMM_CTXT_PRESENT_OLD_GUTI(elm)) {
    h_rc = obj_hashtable_ts_insert (emm_data->ctx_coll_guti, (const void *const)(&elm->_old_guti), sizeof (elm->_old_guti), (void*)((uintptr_t )(PARENT_STRUCT(elm, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id));

    if (HASH_TABLE_OK == h_rc) {
      OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with old GUTI "GUTI_FMT"\n", (PARENT_STRUCT(elm, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, GUTI_ARG(&elm->_old_guti));
    } else {
      OAILOG_ERROR (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with old GUTI "GUTI_FMT" Failed %s\n", (PARENT_STRUCT(elm, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, GUTI_ARG(&elm->_old_guti), hashtable_rc_code2string (h_rc));
      return RETURNerror;
    }
  }
  return RETURNok;
}

//------------------------------------------------------------------------------
int
emm_context_add_imsi (
  emm_data_t * emm_data,
  struct emm_context_s *elm)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;

  if ( IS_EMM_CTXT_PRESENT_IMSI(elm)) {
    h_rc = hashtable_ts_insert (emm_data->ctx_coll_imsi, elm->_imsi64, (void*)((uintptr_t )(PARENT_STRUCT(elm, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id));

    if (HASH_TABLE_OK == h_rc) {
      OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with IMSI "IMSI_64_FMT"\n", (PARENT_STRUCT(elm, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, elm->_imsi64);
    } else {
      OAILOG_ERROR (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with IMSI "IMSI_64_FMT" Failed %s\n",
          (PARENT_STRUCT(elm, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id, elm->_imsi64, hashtable_rc_code2string (h_rc));
      return RETURNerror;
    }
  }
  return RETURNok;
}

//------------------------------------------------------------------------------
void emm_init_context(struct emm_context_s *emm_ctx)
{
    emm_ctx->emm_cause       = EMM_CAUSE_SUCCESS;
    emm_ctx->_emm_fsm_state = EMM_INVALID;
    emm_ctx->T3450.id        = NAS_TIMER_INACTIVE_ID;
    emm_ctx->T3450.sec       = mme_config.nas_config.t3450_sec;
    emm_ctx->T3460.id        = NAS_TIMER_INACTIVE_ID;
    emm_ctx->T3460.sec       = mme_config.nas_config.t3460_sec;
    emm_ctx->T3470.id        = NAS_TIMER_INACTIVE_ID;
    emm_ctx->T3470.sec       = mme_config.nas_config.t3470_sec;
    struct ue_mm_context_s * ue_mm_context = PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context);
    emm_fsm_set_state (ue_mm_context->mme_ue_s1ap_id, emm_ctx, EMM_DEREGISTERED);

    emm_ctx_clear_guti(emm_ctx);
    emm_ctx_clear_old_guti(emm_ctx);
    emm_ctx_clear_imsi(emm_ctx);
    emm_ctx_clear_imei(emm_ctx);
    emm_ctx_clear_imeisv(emm_ctx);
    emm_ctx_clear_lvr_tai(emm_ctx);
    emm_ctx_clear_security(emm_ctx);
    emm_ctx_clear_non_current_security(emm_ctx);
    emm_ctx_clear_auth_vectors(emm_ctx);
    emm_ctx_clear_ms_nw_cap(emm_ctx);
    emm_ctx_clear_ue_nw_cap(emm_ctx);
    emm_ctx_clear_current_drx_parameter(emm_ctx);
    emm_ctx_clear_pending_current_drx_parameter(emm_ctx);
    emm_ctx_clear_eps_bearer_context_status(emm_ctx);

    esm_init_context(&emm_ctx->esm_ctx);
}

//------------------------------------------------------------------------------
void emm_context_stop_all_timers (struct emm_context_s *emm_ctx)
{
  if (emm_ctx ) {
    /*
     * Stop timer T3450
     */
    if (emm_ctx->T3450.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3450 (%ld)\n", emm_ctx->T3450.id);
      emm_ctx->T3450.id = nas_timer_stop (emm_ctx->T3450.id);
      if (NAS_TIMER_INACTIVE_ID == emm_ctx->T3450.id) {
        MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3450 stopped UE " MME_UE_S1AP_ID_FMT " ", (PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
        OAILOG_DEBUG (LOG_NAS_EMM, "T3450 stopped UE " MME_UE_S1AP_ID_FMT " ", (PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
      } else {
        OAILOG_ERROR (LOG_NAS_EMM, "Could not stop T3450 stopped UE " MME_UE_S1AP_ID_FMT " ", (PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
      }
    }

    /*
     * Stop timer T3460
     */
    if (emm_ctx->T3460.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3460 (%ld)\n", emm_ctx->T3460.id);
      emm_ctx->T3460.id = nas_timer_stop (emm_ctx->T3460.id);
      if (NAS_TIMER_INACTIVE_ID == emm_ctx->T3460.id) {
        MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3460 stopped UE " MME_UE_S1AP_ID_FMT " ", (PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
        OAILOG_DEBUG (LOG_NAS_EMM, "T3460 stopped UE " MME_UE_S1AP_ID_FMT " ", (PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
      } else {
        OAILOG_ERROR (LOG_NAS_EMM, "Could not stop T3460 stopped UE " MME_UE_S1AP_ID_FMT " ", (PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
      }
    }

    /*
     * Stop timer T3470
     */
    if (emm_ctx->T3470.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3470 (%ld)\n", emm_ctx->T3460.id);
      emm_ctx->T3470.id = nas_timer_stop (emm_ctx->T3470.id);
      if (NAS_TIMER_INACTIVE_ID == emm_ctx->T3470.id) {
        MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3470 stopped UE " MME_UE_S1AP_ID_FMT " ", (PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
        OAILOG_DEBUG (LOG_NAS_EMM, "T3470 stopped UE " MME_UE_S1AP_ID_FMT " ", (PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
      } else {
        OAILOG_ERROR (LOG_NAS_EMM, "Could not stop T3470 stopped UE " MME_UE_S1AP_ID_FMT " ", (PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
      }
    }
  }
}

//------------------------------------------------------------------------------
void emm_context_silently_reset_procedures(
    struct emm_context_s * const emm_ctx)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  if (emm_ctx) {
    emm_context_stop_all_timers(emm_ctx);
    if (emm_ctx->common_proc) {
      emm_common_cleanup(&emm_ctx->common_proc);
    }
  }
  OAILOG_FUNC_OUT (LOG_NAS_EMM);
}
//------------------------------------------------------------------------------
void emm_context_free(struct emm_context_s * const emm_ctx)
{
  if (emm_ctx ) {
    if (emm_ctx->esm_msg) {
      bdestroy_wrapper (&emm_ctx->esm_msg);
      emm_ctx->esm_msg = NULL;
    }

    emm_context_stop_all_timers(emm_ctx);

    // NO need:free_esm_data_context(&emm_ctx->esm_ctx);
    free_wrapper((void**)&emm_ctx);
  }
}

//------------------------------------------------------------------------------
void emm_context_free_content(struct emm_context_s * const emm_ctx)
{
  if (emm_ctx ) {
    if (emm_ctx->esm_msg) {
      bdestroy_wrapper (&emm_ctx->esm_msg);
      emm_ctx->esm_msg = NULL;
    }

    emm_context_stop_all_timers(emm_ctx);

    // no need: esm_data_context_free_content(&emm_ctx->esm_ctx);

    memset(emm_ctx, 0, sizeof(*emm_ctx));
  }
}

//------------------------------------------------------------------------------
void emm_context_dump (
  const struct emm_context_s * const emm_context,
  const uint8_t         indent_spaces,
  bstring bstr_dump)
{
  if (emm_context ) {
    char                                    key_string[KASME_LENGTH_OCTETS * 2+1];
    char                                    imsi_str[16+1];
    int                                     k = 0,
                                            size = 0,
                                            remaining_size = 0;

    bformata (bstr_dump, "%*s - EMM-CTX: ue id:           " MME_UE_S1AP_ID_FMT " (UE identifier)\n", indent_spaces, " ", (PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context))->mme_ue_s1ap_id);
    bformata (bstr_dump, "%*s     - is_dynamic:       %u      (Dynamically allocated context indicator)\n", indent_spaces, " ", emm_context->is_dynamic);
    bformata (bstr_dump, "%*s     - is_attached:      %u      (Attachment indicator)\n", indent_spaces, " ", emm_context->is_attached);
    bformata (bstr_dump, "%*s     - is_emergency:     %u      (Emergency bearer services indicator)\n", indent_spaces, " ", emm_context->is_emergency);
    IMSI_TO_STRING (&emm_context->_imsi, imsi_str, 16);
    bformata (bstr_dump, "%*s     - imsi:             %s      (The IMSI provided by the UE or the MME)\n", indent_spaces, " ", imsi_str);
    bformata (bstr_dump, "%*s     - imei:             TODO    (The IMEI provided by the UE)\n", indent_spaces, " ");
    bformata (bstr_dump, "%*s     - imeisv:           TODO    (The IMEISV provided by the UE)\n", indent_spaces, " ");
    bformata (bstr_dump, "%*s     - guti:             "GUTI_FMT"      (The GUTI assigned to the UE)\n", indent_spaces, " ", GUTI_ARG(&emm_context->_guti));
    bformata (bstr_dump, "%*s     - old_guti:         "GUTI_FMT"      (The old GUTI)\n", indent_spaces, " ", GUTI_ARG(&emm_context->_old_guti));
    for (k=0; k < emm_context->_tai_list.numberoflists; k++) {
      switch (emm_context->_tai_list.partial_tai_list[k].typeoflist) {
      case TRACKING_AREA_IDENTITY_LIST_ONE_PLMN_NON_CONSECUTIVE_TACS: {
          tai_t tai = {0};
          tai.mcc_digit1 = emm_context->_tai_list.partial_tai_list[k].u.tai_one_plmn_non_consecutive_tacs.mcc_digit1;
          tai.mcc_digit2 = emm_context->_tai_list.partial_tai_list[k].u.tai_one_plmn_non_consecutive_tacs.mcc_digit2;
          tai.mcc_digit3 = emm_context->_tai_list.partial_tai_list[k].u.tai_one_plmn_non_consecutive_tacs.mcc_digit3;
          tai.mnc_digit1 = emm_context->_tai_list.partial_tai_list[k].u.tai_one_plmn_non_consecutive_tacs.mnc_digit1;
          tai.mnc_digit2 = emm_context->_tai_list.partial_tai_list[k].u.tai_one_plmn_non_consecutive_tacs.mnc_digit2;
          tai.mnc_digit3 = emm_context->_tai_list.partial_tai_list[k].u.tai_one_plmn_non_consecutive_tacs.mnc_digit3;
          for (int p = 0; p < (emm_context->_tai_list.partial_tai_list[k].numberofelements+1); p++) {
            tai.tac        = emm_context->_tai_list.partial_tai_list[k].u.tai_one_plmn_non_consecutive_tacs.tac[p];

            bformata (bstr_dump, "%*s     - tai:              "TAI_FMT" (Tracking area identity the UE is registered to)\n", indent_spaces, " ",
              TAI_ARG(&tai));
          }
        }
        break;
      case TRACKING_AREA_IDENTITY_LIST_ONE_PLMN_CONSECUTIVE_TACS:
        bformata (bstr_dump, "%*s     - tai:              "TAI_FMT"+%u consecutive tacs   (Tracking area identity the UE is registered to)\n", indent_spaces, " ",
          TAI_ARG(&emm_context->_tai_list.partial_tai_list[k].u.tai_one_plmn_consecutive_tacs), emm_context->_tai_list.partial_tai_list[k].numberofelements);
        break;
      case TRACKING_AREA_IDENTITY_LIST_MANY_PLMNS:
        for (int p = 0; p < (emm_context->_tai_list.partial_tai_list[k].numberofelements+1); p++) {
          bformata (bstr_dump, "%*s     - tai:              "TAI_FMT" (Tracking area identity the UE is registered to)\n", indent_spaces, " ",
            TAI_ARG(&emm_context->_tai_list.partial_tai_list[k].u.tai_many_plmn[p]));
        }
        break;
      default: ;
      }
    }
    bformata (bstr_dump, "%*s     - eksi:             %u      (Security key set identifier)\n", indent_spaces, " ", emm_context->_security.eksi);
    for (int vector_index = 0; vector_index < MAX_EPS_AUTH_VECTORS; vector_index++) {
      bformata (bstr_dump, "%*s     - auth_vector[%d]:              (EPS authentication vector)\n", indent_spaces, " ", vector_index);
      bformata (bstr_dump, "%*s         - kasme: " KASME_FORMAT "" KASME_FORMAT "\n", indent_spaces, " ",
                               KASME_DISPLAY_1 (emm_context->_vector[vector_index].kasme),
                               KASME_DISPLAY_2 (emm_context->_vector[vector_index].kasme));
      bformata (bstr_dump, "%*s         - rand:  " RAND_FORMAT "\n", indent_spaces, " ", RAND_DISPLAY (emm_context->_vector[vector_index].rand));
      bformata (bstr_dump, "%*s         - autn:  " AUTN_FORMAT "\n", indent_spaces, " ", AUTN_DISPLAY (emm_context->_vector[vector_index].autn));

      for (k = 0; k < XRES_LENGTH_MAX; k++) {
        sprintf (&key_string[k * 3], "%02x,", emm_context->_vector[vector_index].xres[k]);
      }

      key_string[k * 3 - 1] = '\0';
      bformata (bstr_dump, "%*s         - xres:  %s\n", indent_spaces, " ", key_string);
    }

    if (IS_EMM_CTXT_PRESENT_SECURITY(emm_context)) {
      bformata (bstr_dump, "%*s     - security context:          (Current EPS NAS security context)\n", indent_spaces, " ");
      bformata (bstr_dump, "%*s         - type:  %s              (Type of security context)\n", indent_spaces, " ",
          (emm_context->_security.sc_type == SECURITY_CTX_TYPE_NOT_AVAILABLE)  ? "NOT_AVAILABLE" :
          (emm_context->_security.sc_type == SECURITY_CTX_TYPE_PARTIAL_NATIVE) ? "PARTIAL_NATIVE" :
          (emm_context->_security.sc_type == SECURITY_CTX_TYPE_FULL_NATIVE)    ? "FULL_NATIVE" :  "MAPPED");
      bformata (bstr_dump, "%*s         - eksi:  %u              (NAS key set identifier for E-UTRAN)\n", indent_spaces, " ", emm_context->_security.eksi);

      if (SECURITY_CTX_TYPE_PARTIAL_NATIVE <= emm_context->_security.sc_type) {
        bformata (bstr_dump, "%*s         - dl_count.overflow: %05u", indent_spaces, " ", emm_context->_security.dl_count.overflow);
        bformata (bstr_dump, " dl_count.seq_num:  %03u\n", emm_context->_security.dl_count.seq_num);
        bformata (bstr_dump, "%*s         - ul_count.overflow: %05u", indent_spaces, " ", emm_context->_security.ul_count.overflow);
        bformata (bstr_dump, " ul_count.seq_num:  %03u\n", emm_context->_security.ul_count.seq_num);

//        if (SECURITY_CTX_TYPE_FULL_NATIVE <= emm_context->_security.sc_type) {
        if (true) {
          size = 0;
          remaining_size = KASME_LENGTH_OCTETS * 2;

          for (k = 0; k < AUTH_KNAS_ENC_SIZE; k++) {
            int ret = snprintf (&key_string[size], remaining_size, "%02x ", emm_context->_security.knas_enc[k]);
            if (0 < ret) {
              size += ret;
              remaining_size -= ret;
            } else break;
          }

          bformata (bstr_dump, "%*s     - knas_enc: %s     (NAS cyphering key)\n", indent_spaces, " ", key_string);

          size = 0;
          remaining_size = KASME_LENGTH_OCTETS * 2;

          for (k = 0; k < AUTH_KNAS_INT_SIZE; k++) {
            int ret = snprintf (&key_string[size], remaining_size, "%02x ", emm_context->_security.knas_int[k]);
            if (0 < ret) {
              size += ret;
              remaining_size -= ret;
            } else break;
          }


          bformata (bstr_dump, "%*s     - knas_int: %s     (NAS integrity key)\n", indent_spaces, " ", key_string);
          bformata (bstr_dump, "%*s     - UE network capabilities\n", indent_spaces, " ");
          bformata (bstr_dump, "%*s         EEA: %c%c%c%c%c%c%c%c   EIA: %c%c%c%c%c%c%c%c\n", indent_spaces, " ",
              (emm_context->_ue_network_capability.eea & UE_NETWORK_CAPABILITY_EEA0) ? '0':'_',
              (emm_context->_ue_network_capability.eea & UE_NETWORK_CAPABILITY_EEA1) ? '1':'_',
              (emm_context->_ue_network_capability.eea & UE_NETWORK_CAPABILITY_EEA2) ? '2':'_',
              (emm_context->_ue_network_capability.eea & UE_NETWORK_CAPABILITY_EEA3) ? '3':'_',
              (emm_context->_ue_network_capability.eea & UE_NETWORK_CAPABILITY_EEA4) ? '4':'_',
              (emm_context->_ue_network_capability.eea & UE_NETWORK_CAPABILITY_EEA5) ? '5':'_',
              (emm_context->_ue_network_capability.eea & UE_NETWORK_CAPABILITY_EEA6) ? '6':'_',
              (emm_context->_ue_network_capability.eea & UE_NETWORK_CAPABILITY_EEA7) ? '7':'_',
              (emm_context->_ue_network_capability.eia & UE_NETWORK_CAPABILITY_EIA0) ? '0':'_',
              (emm_context->_ue_network_capability.eia & UE_NETWORK_CAPABILITY_EIA1) ? '1':'_',
              (emm_context->_ue_network_capability.eia & UE_NETWORK_CAPABILITY_EIA2) ? '2':'_',
              (emm_context->_ue_network_capability.eia & UE_NETWORK_CAPABILITY_EIA3) ? '3':'_',
              (emm_context->_ue_network_capability.eia & UE_NETWORK_CAPABILITY_EIA4) ? '4':'_',
              (emm_context->_ue_network_capability.eia & UE_NETWORK_CAPABILITY_EIA5) ? '5':'_',
              (emm_context->_ue_network_capability.eia & UE_NETWORK_CAPABILITY_EIA6) ? '6':'_',
              (emm_context->_ue_network_capability.eia & UE_NETWORK_CAPABILITY_EIA7) ? '7':'_');
          if (emm_context->_ue_network_capability.umts_present) {
            bformata (bstr_dump, "%*s         UEA: %c%c%c%c%c%c%c%c   UIA:  %c%c%c%c%c%c%c \n", indent_spaces, " ",
                (emm_context->_ue_network_capability.uea & UE_NETWORK_CAPABILITY_UEA0) ? '0':'_',
                (emm_context->_ue_network_capability.uea & UE_NETWORK_CAPABILITY_UEA1) ? '1':'_',
                (emm_context->_ue_network_capability.uea & UE_NETWORK_CAPABILITY_UEA2) ? '2':'_',
                (emm_context->_ue_network_capability.uea & UE_NETWORK_CAPABILITY_UEA3) ? '3':'_',
                (emm_context->_ue_network_capability.uea & UE_NETWORK_CAPABILITY_UEA4) ? '4':'_',
                (emm_context->_ue_network_capability.uea & UE_NETWORK_CAPABILITY_UEA5) ? '5':'_',
                (emm_context->_ue_network_capability.uea & UE_NETWORK_CAPABILITY_UEA6) ? '6':'_',
                (emm_context->_ue_network_capability.uea & UE_NETWORK_CAPABILITY_UEA7) ? '7':'_',
                (emm_context->_ue_network_capability.uia & UE_NETWORK_CAPABILITY_UIA1) ? '1':'_',
                (emm_context->_ue_network_capability.uia & UE_NETWORK_CAPABILITY_UIA2) ? '2':'_',
                (emm_context->_ue_network_capability.uia & UE_NETWORK_CAPABILITY_UIA3) ? '3':'_',
                (emm_context->_ue_network_capability.uia & UE_NETWORK_CAPABILITY_UIA4) ? '4':'_',
                (emm_context->_ue_network_capability.uia & UE_NETWORK_CAPABILITY_UIA5) ? '5':'_',
                (emm_context->_ue_network_capability.uia & UE_NETWORK_CAPABILITY_UIA6) ? '6':'_',
                (emm_context->_ue_network_capability.uia & UE_NETWORK_CAPABILITY_UIA7) ? '7':'_');
            bformata (bstr_dump, "%*s         Alphabet | CSFB | LPP | LCS | SRVCC | NF \n", indent_spaces, " ");
            bformata (bstr_dump, "%*s           %s       %c     %c     %c     %c      %c\n", indent_spaces, " ",
                (emm_context->_ue_network_capability.ucs2) ? "UCS2":"DEFT",
                (emm_context->_ue_network_capability.csfb) ?   '1':'0',
                (emm_context->_ue_network_capability.lpp) ?   '1':'0',
                (emm_context->_ue_network_capability.lcs) ?   '1':'0',
                (emm_context->_ue_network_capability.srvcc) ? '1':'0',
                (emm_context->_ue_network_capability.nf) ? '1':'0');
          }
          bformata (bstr_dump, "%*s     - MS network capabilities TODO\n", indent_spaces, " ");

          bformata (bstr_dump, "%*s     - selected_algorithms EEA%u EIA%u\n", indent_spaces, " ",
              emm_context->_security.selected_algorithms.encryption, emm_context->_security.selected_algorithms.integrity);
        }
      }
    } else {
      bformata (bstr_dump, "%*s     - No security context\n", indent_spaces, " ");
    }

    bformata (bstr_dump, "%*s     - EMM state:     %s\n", indent_spaces, " ", emm_fsm_get_state_str(emm_context));
    bformata (bstr_dump, "%*s     - Timers : T3450      T3460     T3470\n", indent_spaces, " ");
    bformata (bstr_dump, "%*s                %s   %s  %s\n", indent_spaces, " ",
        (NAS_TIMER_INACTIVE_ID != emm_context->T3450.id) ? "Running ":"Inactive",
        (NAS_TIMER_INACTIVE_ID != emm_context->T3460.id) ? "Running ":"Inactive",
        (NAS_TIMER_INACTIVE_ID != emm_context->T3470.id) ? "Running ":"Inactive");

    if (emm_context->esm_msg) {
      bformata (bstr_dump, "%*s     - Pending ESM msg :\n", indent_spaces, " ");
      bformata (bstr_dump, "%*s     +-----+-------------------------------------------------+\n", indent_spaces, " ");
      bformata (bstr_dump, "%*s     |     |  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f |\n", indent_spaces, " ");
      bformata (bstr_dump, "%*s     |-----|-------------------------------------------------|\n", indent_spaces, " ");

      int octet_index;
      for (octet_index = 0; octet_index < blength(emm_context->esm_msg); octet_index++) {
        if ((octet_index % 16) == 0) {
          if (octet_index != 0) {
            bformata (bstr_dump, " |\n");
          }
          bformata (bstr_dump, "%*s     |%04ld |", indent_spaces, " ", octet_index);
        }

        /*
         * Print every single octet in hexadecimal form
         */
        bformata (bstr_dump, " %02x", emm_context->esm_msg->data[octet_index]);
      }
      /*
       * Append enough spaces and put final pipe
       */
      for (int index = octet_index%16; index < 16; ++index) {
        bformata (bstr_dump, "   ");
      }
      bformata (bstr_dump, " |\n");
      bformata (bstr_dump, "%*s     +-----+-------------------------------------------------+\n", indent_spaces, " ");
    }
    bformata (bstr_dump, "%*s     - TODO  esm_data_ctx\n", indent_spaces, " ");
    //esm_context_dump(&emm_context->esm_ctx, indent_spaces, bstr_dump);
  }
}

//------------------------------------------------------------------------------
static bool
emm_context_dump_hash_table_wrapper (
  hash_key_t keyP,
  void *dataP,
  void *parameterP,
  void**resultP)
{
  bstring  bstr_dump = bfromcstralloc(2048, "----------------------- EMM context -----------------------\n");
  emm_context_dump (dataP, 4, bstr_dump);
  return false; // otherwise dump stop
}

//------------------------------------------------------------------------------
void
emm_context_dump_all (
  void)
{
  OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX - Dump all contexts:\n");
  hashtable_ts_apply_callback_on_elements (_emm_data.ctx_coll_ue_id, emm_context_dump_hash_table_wrapper, NULL, NULL);
}

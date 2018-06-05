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
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.301.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
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
#include "mme_app_defs.h"
#include "nas_itti_messaging.h"

//#include "EmmCommon.h"
#include "../../mme/mme_ie_defs.h"


static int _ctx_req_proc_success_cb (struct emm_data_context_s *emm_ctx);
static int _ctx_req_proc_failure_cb (struct emm_data_context_s *emm_ctx);


//------------------------------------------------------------------------------

static hashtable_rc_t
_emm_data_context_hashtable_insert(
    emm_data_t *emm_data,
    struct emm_data_context_s *elm)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;

  if ( IS_EMM_CTXT_PRESENT_IMSI(elm)) {
    h_rc = hashtable_ts_insert (emm_data->ctx_coll_imsi, elm->_imsi64, (void*)(&elm->ue_id));
  } else {
    // This should not happen. Possible UE bug?
    OAILOG_WARNING(LOG_NAS_EMM, "EMM-CTX doesn't contain valid imsi UE id " MME_UE_S1AP_ID_FMT "\n", elm->ue_id);
  }
  return h_rc;
}


//------------------------------------------------------------------------------
mme_ue_s1ap_id_t emm_ctx_get_new_ue_id(const emm_data_context_t * const ctxt)
{
  return (mme_ue_s1ap_id_t)((uint)((uintptr_t)ctxt) >> 4);
}


//------------------------------------------------------------------------------
inline void emm_ctx_set_attribute_present(emm_data_context_t * const ctxt, const int attribute_bit_pos)
{
  ctxt->member_present_mask |= attribute_bit_pos;
}

inline void emm_ctx_clear_attribute_present(emm_data_context_t * const ctxt, const int attribute_bit_pos)
{
  ctxt->member_present_mask &= ~attribute_bit_pos;
  ctxt->member_valid_mask &= ~attribute_bit_pos;
}

inline void emm_ctx_set_attribute_valid(emm_data_context_t * const ctxt, const int attribute_bit_pos)
{
  ctxt->member_present_mask |= attribute_bit_pos;
  ctxt->member_valid_mask   |= attribute_bit_pos;
}

inline void emm_ctx_clear_attribute_valid(emm_data_context_t * const ctxt, const int attribute_bit_pos)
{
  ctxt->member_valid_mask &= ~attribute_bit_pos;
}

//------------------------------------------------------------------------------
/* Clear GUTI  */
inline void emm_ctx_clear_guti(emm_data_context_t * const ctxt)
{
  clear_guti(&ctxt->_guti);
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_GUTI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " GUTI cleared\n", ctxt->ue_id);
}

/* Set GUTI */
inline void emm_ctx_set_guti(emm_data_context_t * const ctxt, guti_t *guti)
{
  ctxt->_guti = *guti;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_GUTI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set GUTI " GUTI_FMT " (present)\n", ctxt->ue_id, GUTI_ARG(&ctxt->_guti));
}

/* Set GUTI, mark it as valid */
inline void emm_ctx_set_valid_guti(emm_data_context_t * const ctxt, guti_t *guti)
{
  ctxt->_guti = *guti;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_GUTI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set GUTI " GUTI_FMT " (valid)\n", ctxt->ue_id, GUTI_ARG(&ctxt->_guti));
}

//------------------------------------------------------------------------------
/* Clear old GUTI  */
inline void emm_ctx_clear_old_guti(emm_data_context_t * const ctxt)
{
  clear_guti(&ctxt->_old_guti);
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_OLD_GUTI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " old GUTI cleared\n", ctxt->ue_id);
}

/* Set GUTI */
inline void emm_ctx_set_old_guti(emm_data_context_t * const ctxt, guti_t *guti)
{
  ctxt->_old_guti = *guti;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_OLD_GUTI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set old GUTI " GUTI_FMT " (present)\n", ctxt->ue_id, GUTI_ARG(&ctxt->_old_guti));
}

/* Set GUTI, mark it as valid */
inline void emm_ctx_set_valid_old_guti(emm_data_context_t * const ctxt, guti_t *guti)
{
  ctxt->_old_guti = *guti;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_OLD_GUTI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set old GUTI " GUTI_FMT " (valid)\n", ctxt->ue_id, GUTI_ARG(&ctxt->_old_guti));
}

//------------------------------------------------------------------------------
/* Clear IMSI */
inline void emm_ctx_clear_imsi(emm_data_context_t * const ctxt)
{
  clear_imsi(&ctxt->_imsi);
  ctxt->_imsi64 = INVALID_IMSI64;
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_IMSI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared IMSI\n", ctxt->ue_id);
}

/* Set IMSI */
inline void emm_ctx_set_imsi(emm_data_context_t * const ctxt, imsi_t *imsi, const imsi64_t imsi64)
{
  ctxt->_imsi   = *imsi;
  ctxt->_imsi64 = imsi64;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_IMSI);
#if  DEBUG_IS_ON
  char     imsi_str[IMSI_BCD_DIGITS_MAX+1] = {0};
  IMSI64_TO_STRING (ctxt->_imsi64, imsi_str);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set IMSI %s (valid)\n", ctxt->ue_id, imsi_str);
#endif
}

/* Set IMSI, mark it as valid */
inline void emm_ctx_set_valid_imsi(emm_data_context_t * const ctxt, imsi_t *imsi, const imsi64_t imsi64)
{
  ctxt->_imsi   = *imsi;
  ctxt->_imsi64 = imsi64;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_IMSI);
#if  DEBUG_IS_ON
  char     imsi_str[IMSI_BCD_DIGITS_MAX+1] = {0};
  IMSI64_TO_STRING (ctxt->_imsi64, imsi_str);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set IMSI %s (valid)\n", ctxt->ue_id, imsi_str);
#endif
  /** Notifying the MME_APP without an ITTI request of the validated IMSI, to update the registration tables. */
  mme_api_notify_imsi(ctxt->ue_id, imsi64);
  ue_context_t * test_ue_ctx = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, imsi64);
  DevAssert(test_ue_ctx);
}

//------------------------------------------------------------------------------
/* Clear IMEI */
inline void emm_ctx_clear_imei(emm_data_context_t * const ctxt)
{
  clear_imei(&ctxt->_imei);
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_IMEI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " IMEI cleared\n", ctxt->ue_id);
}

/* Set IMEI */
inline void emm_ctx_set_imei(emm_data_context_t * const ctxt, imei_t *imei)
{
  ctxt->_imei = *imei;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_IMEI);
#if  DEBUG_IS_ON
  char     imei_str[16];
  IMEI_TO_STRING (imei, imei_str, 16);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set IMEI %s (present)\n", ctxt->ue_id, imei_str);
#endif
}

/* Set IMEI, mark it as valid */
inline void emm_ctx_set_valid_imei(emm_data_context_t * const ctxt, imei_t *imei)
{
  ctxt->_imei = *imei;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_IMEI);
#if  DEBUG_IS_ON
  char     imei_str[16];
  IMEI_TO_STRING (imei, imei_str, 16);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set IMEI %s (valid)\n", ctxt->ue_id, imei_str);
#endif
}

//------------------------------------------------------------------------------
/* Clear IMEI_SV */
inline void emm_ctx_clear_imeisv(emm_data_context_t * const ctxt)
{
  clear_imeisv(&ctxt->_imeisv);
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_IMEI_SV);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared IMEI_SV \n", ctxt->ue_id);
}

/* Set IMEI_SV */
inline void emm_ctx_set_imeisv(emm_data_context_t * const ctxt, imeisv_t *imeisv)
{
  ctxt->_imeisv = *imeisv;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_IMEI_SV);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set IMEI_SV (present)\n", ctxt->ue_id);
}

/* Set IMEI_SV, mark it as valid */
inline void emm_ctx_set_valid_imeisv(emm_data_context_t * const ctxt, imeisv_t *imeisv)
{
  ctxt->_imeisv = *imeisv;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_IMEI_SV);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set IMEI_SV (valid)\n", ctxt->ue_id);
}

//------------------------------------------------------------------------------
/* Clear last_visited_registered_tai */
inline void emm_ctx_clear_lvr_tai(emm_data_context_t * const ctxt)
{
  clear_tai(&ctxt->_lvr_tai);
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_LVR_TAI);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared last visited registered TAI\n", ctxt->ue_id);
}

/* Set last_visited_registered_tai */
inline void emm_ctx_set_lvr_tai(emm_data_context_t * const ctxt, tai_t *lvr_tai)
{
  ctxt->_lvr_tai = *lvr_tai;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_LVR_TAI);
  //log_message(NULL, OAILOG_LEVEL_DEBUG,    LOG_NAS_EMM, __FILE__, __LINE__,
  //    "ue_id="MME_UE_S1AP_ID_FMT" set last visited registered TAI "TAI_FMT" (present)\n", ctxt->ue_id, TAI_ARG(&ctxt->_lvr_tai));

  //OAILOG_DEBUG (LOG_NAS_EMM, "ue_id="MME_UE_S1AP_ID_FMT" set last visited registered TAI "TAI_FMT" (present)\n", ctxt->ue_id, TAI_ARG(&ctxt->_lvr_tai));
}

/* Set last_visited_registered_tai, mark it as valid */
inline void emm_ctx_set_valid_lvr_tai(emm_data_context_t * const ctxt, tai_t *lvr_tai)
{
  if (lvr_tai) { // optionnal
    ctxt->_lvr_tai = *lvr_tai;
    emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_LVR_TAI);
    //OAILOG_DEBUG (LOG_NAS_EMM, "ue_id="MME_UE_S1AP_ID_FMT" set last visited registered TAI "TAI_FMT" (valid)\n", ctxt->ue_id, TAI_ARG(&ctxt->_lvr_tai));
  }
}

//------------------------------------------------------------------------------
/* Clear AUTH vectors  */
inline void emm_ctx_clear_auth_vectors(emm_data_context_t * const ctxt)
{
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_AUTH_VECTORS);
  for (int i = 0; i < MAX_EPS_AUTH_VECTORS; i++) {
    memset((void *)&ctxt->_vector[i], 0, sizeof(ctxt->_vector[i]));
    emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_AUTH_VECTOR0+i);
  }
  emm_ctx_clear_security_vector_index(ctxt);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared auth vectors \n", ctxt->ue_id);
}
//------------------------------------------------------------------------------
/* Clear AUTH vector  */
inline void emm_ctx_clear_auth_vector(emm_data_context_t * const ctxt, ksi_t eksi)
{
  AssertFatal(eksi < MAX_EPS_AUTH_VECTORS, "Out of bounds eksi %d", eksi);
  memset((void *)&ctxt->_vector[eksi%MAX_EPS_AUTH_VECTORS], 0, sizeof(ctxt->_vector[eksi%MAX_EPS_AUTH_VECTORS]));
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_AUTH_VECTOR0+eksi);
  int remaining_vectors = 0;
  for (int i = 0; i < MAX_EPS_AUTH_VECTORS; i++) {
    if (IS_EMM_CTXT_VALID_AUTH_VECTOR(ctxt, i)) {
      remaining_vectors+=1;
    }
  }
  ctxt->remaining_vectors = remaining_vectors;
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared auth vector %u \n", ctxt->ue_id, eksi);
  if (!(remaining_vectors)) {
    emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_AUTH_VECTORS);
    emm_ctx_clear_security_vector_index(ctxt);
    OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared auth vectors\n", ctxt->ue_id);
  }
}
//------------------------------------------------------------------------------
/* Clear security  */
inline void emm_ctx_clear_security(emm_data_context_t * const ctxt)
{
  memset (&ctxt->_security, 0, sizeof (ctxt->_security));
  emm_ctx_set_security_type(ctxt, SECURITY_CTX_TYPE_NOT_AVAILABLE);
  emm_ctx_set_security_eksi(ctxt, KSI_NO_KEY_AVAILABLE);
  ctxt->_security.selected_algorithms.encryption = NAS_SECURITY_ALGORITHMS_EEA0;
  ctxt->_security.selected_algorithms.integrity  = NAS_SECURITY_ALGORITHMS_EIA0;
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_SECURITY);
  ctxt->_security.direction_decode = SECU_DIRECTION_UPLINK;
  ctxt->_security.direction_encode = SECU_DIRECTION_DOWNLINK;
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared security context \n", ctxt->ue_id);
}

//------------------------------------------------------------------------------
inline void emm_ctx_set_security_type(emm_data_context_t * const ctxt, emm_sc_type_t sc_type)
{
  ctxt->_security.sc_type = sc_type;
  OAILOG_TRACE (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set security context security type %d\n", ctxt->ue_id, sc_type);
}

//------------------------------------------------------------------------------
inline void emm_ctx_set_security_eksi(emm_data_context_t * const ctxt, ksi_t eksi)
{
  ctxt->_security.eksi = eksi;
  OAILOG_TRACE (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set security context eksi %d\n", ctxt->ue_id, eksi);
}

//------------------------------------------------------------------------------
inline void emm_ctx_clear_security_vector_index(emm_data_context_t * const ctxt)
{
  ctxt->_security.vector_index = EMM_SECURITY_VECTOR_INDEX_INVALID;
  OAILOG_TRACE (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " clear security context vector index\n", ctxt->ue_id);
}
//------------------------------------------------------------------------------
inline void emm_ctx_set_security_vector_index(emm_data_context_t * const ctxt, int vector_index)
{
  ctxt->_security.vector_index = vector_index;
  OAILOG_TRACE (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set security context vector index %d\n", ctxt->ue_id, vector_index);
}


//------------------------------------------------------------------------------
/* Clear non current security  */
inline void emm_ctx_clear_non_current_security(emm_data_context_t * const ctxt)
{
  memset (&ctxt->_non_current_security, 0, sizeof (ctxt->_non_current_security));
  ctxt->_non_current_security.sc_type      = SECURITY_CTX_TYPE_NOT_AVAILABLE;
  ctxt->_non_current_security.eksi         = KSI_NO_KEY_AVAILABLE;
  ctxt->_non_current_security.selected_algorithms.encryption = NAS_SECURITY_ALGORITHMS_EEA0;
  ctxt->_non_current_security.selected_algorithms.integrity  = NAS_SECURITY_ALGORITHMS_EIA0;
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_NON_CURRENT_SECURITY);
  ctxt->_security.direction_decode = SECU_DIRECTION_UPLINK;
  ctxt->_security.direction_encode = SECU_DIRECTION_DOWNLINK;
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared non current security context \n", ctxt->ue_id);
}

//------------------------------------------------------------------------------
/* Clear UE network capability IE   */
inline void emm_ctx_clear_ue_nw_cap(emm_data_context_t * const ctxt)
{
  memset (&ctxt->_ue_network_capability, 0, sizeof (ctxt->_ue_network_capability));
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_UE_NETWORK_CAPABILITY_IE);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared UE network capability IE\n", ctxt->ue_id);
}

/* Set UE network capability IE */
inline void emm_ctx_set_ue_nw_cap(emm_data_context_t * const ctxt, const ue_network_capability_t * const ue_nw_cap_ie)
{
  ctxt->_ue_network_capability = *ue_nw_cap_ie;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_UE_NETWORK_CAPABILITY_IE);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set UE network capability IE (present)\n", ctxt->ue_id);
}

/* Set UE network capability IE, mark it as valid */
inline void emm_ctx_set_valid_ue_nw_cap(emm_data_context_t * const ctxt, const ue_network_capability_t * const ue_nw_cap_ie)
{
  ctxt->_ue_network_capability = *ue_nw_cap_ie;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_UE_NETWORK_CAPABILITY_IE);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set UE network capability IE (valid)\n", ctxt->ue_id);
}


//------------------------------------------------------------------------------
/* Clear MS network capability IE   */
inline void emm_ctx_clear_ms_nw_cap(emm_data_context_t * const ctxt)
{
  memset (&ctxt->_ms_network_capability, 0, sizeof (ctxt->_ue_network_capability));
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_MS_NETWORK_CAPABILITY_IE);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared MS network capability IE\n", ctxt->ue_id);
}

/* Set UE network capability IE */
inline void emm_ctx_set_ms_nw_cap(emm_data_context_t * const ctxt, const ms_network_capability_t * const ms_nw_cap_ie)
{
  ctxt->_ms_network_capability = *ms_nw_cap_ie;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_MS_NETWORK_CAPABILITY_IE);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set MS network capability IE (present)\n", ctxt->ue_id);
}

/* Set UE network capability IE, mark it as valid */
inline void emm_ctx_set_valid_ms_nw_cap(emm_data_context_t * const ctxt, const ms_network_capability_t * const ms_nw_cap_ie)
{
  ctxt->_ms_network_capability = *ms_nw_cap_ie;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_MS_NETWORK_CAPABILITY_IE);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set MS network capability IE (valid)\n", ctxt->ue_id);
}

//------------------------------------------------------------------------------
/* Clear current DRX parameter   */
inline void emm_ctx_clear_drx_parameter(emm_data_context_t * const ctxt)
{
  memset(&ctxt->_drx_parameter, 0, sizeof(drx_parameter_t));
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_CURRENT_DRX_PARAMETER);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared current DRX parameter\n", ctxt->ue_id);
}

/* Set current DRX parameter */
inline void emm_ctx_set_drx_parameter(emm_data_context_t * const ctxt, drx_parameter_t *drx)
{
  memcpy(&ctxt->_drx_parameter, drx, sizeof(drx_parameter_t));
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_CURRENT_DRX_PARAMETER);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set current DRX parameter (present)\n", ctxt->ue_id);
}

/* Set current DRX parameter, mark it as valid */
inline void emm_ctx_set_valid_drx_parameter(emm_data_context_t * const ctxt, drx_parameter_t *drx)
{
  emm_ctx_set_drx_parameter(ctxt, drx);
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_CURRENT_DRX_PARAMETER);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set current DRX parameter (valid)\n", ctxt->ue_id);
}

//------------------------------------------------------------------------------
/* Clear EPS bearer context status   */
inline void emm_ctx_clear_eps_bearer_context_status(emm_data_context_t * const ctxt)
{
  memset (&ctxt->_eps_bearer_context_status, 0, sizeof (ctxt->_eps_bearer_context_status));
  emm_ctx_clear_attribute_present(ctxt, EMM_CTXT_MEMBER_EPS_BEARER_CONTEXT_STATUS);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " cleared EPS bearer context status\n", ctxt->ue_id);
}

/* Set current DRX parameter */
inline void emm_ctx_set_eps_bearer_context_status(emm_data_context_t * const ctxt, eps_bearer_context_status_t *status)
{
  ctxt->_eps_bearer_context_status = *status;
  emm_ctx_set_attribute_present(ctxt, EMM_CTXT_MEMBER_EPS_BEARER_CONTEXT_STATUS);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set EPS bearer context status (present)\n", ctxt->ue_id);
}

/* Set current DRX parameter, mark it as valid */
inline void emm_ctx_set_valid_eps_bearer_context_status(emm_data_context_t * const ctxt, eps_bearer_context_status_t *status)
{
  ctxt->_eps_bearer_context_status = *status;
  emm_ctx_set_attribute_valid(ctxt, EMM_CTXT_MEMBER_EPS_BEARER_CONTEXT_STATUS);
  OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " set EPS bearer context status (valid)\n", ctxt->ue_id);
}

/** Update the EMM context from the received MM Context during Handover/TAU procedure. */
//   todo: _clear_emm_ctxt(emm_ctx_p);
inline void emm_ctx_update_from_mm_eps_context(emm_data_context_t * const emm_ctx_p, void* const _mm_eps_ctxt){
  int                                     rc = RETURNerror;
  int                                     mme_eea = NAS_SECURITY_ALGORITHMS_EEA0;
  int                                     mme_eia = NAS_SECURITY_ALGORITHMS_EIA0;

  OAILOG_FUNC_IN (LOG_NAS_EMM);

  DevAssert(emm_ctx_p);
  DevAssert(_mm_eps_ctxt);

  /** Update the EMM context and the Security context from a received MM context. */
  mm_context_eps_t *mm_eps_ctxt = (mm_context_eps_t*)_mm_eps_ctxt;

  /** Attachment indicator. */
//  DevAssert(!emm_ctx_p->is_attached);

  /*
   * todo: fill the authentication vector based on auth_quadruplet and auth_quintuplet
   */
//  for (int i = 0; i < auth_info_proc->nb_vectors; i++) {
//    AssertFatal (MAX_EPS_AUTH_VECTORS > i, " TOO many vectors");
//    int destination_index = (i + eksi)%MAX_EPS_AUTH_VECTORS;
//    memcpy (emm_ctx->_vector[destination_index].kasme, auth_info_proc->vector[i]->kasme, AUTH_KASME_SIZE);
//    memcpy (emm_ctx->_vector[destination_index].autn,  auth_info_proc->vector[i]->autn, AUTH_AUTN_SIZE);
//    memcpy (emm_ctx->_vector[destination_index].rand, auth_info_proc->vector[i]->rand, AUTH_RAND_SIZE);
//    memcpy (emm_ctx->_vector[destination_index].xres, auth_info_proc->vector[i]->xres.data, auth_info_proc->vector[i]->xres.size);
//    emm_ctx->_vector[destination_index].xres_size = auth_info_proc->vector[i]->xres.size;
//    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Received Vector %u:\n", i);
//    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Received XRES ..: " XRES_FORMAT "\n", XRES_DISPLAY (emm_ctx->_vector[destination_index].xres));
//    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Received RAND ..: " RAND_FORMAT "\n", RAND_DISPLAY (emm_ctx->_vector[destination_index].rand));
//    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Received AUTN ..: " AUTN_FORMAT "\n", AUTN_DISPLAY (emm_ctx->_vector[destination_index].autn));
//    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Received KASME .: " KASME_FORMAT " " KASME_FORMAT "\n",
//        KASME_DISPLAY_1 (emm_ctx->_vector[destination_index].kasme), KASME_DISPLAY_2 (emm_ctx->_vector[destination_index].kasme));
//    emm_ctx_set_attribute_valid(emm_ctx, EMM_CTXT_MEMBER_AUTH_VECTOR0+destination_index);
//  }
//  if (auth_info_proc->nb_vectors > 0) {
//      emm_ctx_set_attribute_present(emm_ctx_p, EMM_CTXT_MEMBER_AUTH_VECTORS);
//
//      for (; eksi < MAX_EPS_AUTH_VECTORS; eksi++) {
//        if (IS_EMM_CTXT_VALID_AUTH_VECTOR(emm_ctx, (eksi % MAX_EPS_AUTH_VECTORS))) {
//          break;
//        }
//      }
//
//
//      // eksi should always be 0
//      ksi_t eksi_mod = eksi % MAX_EPS_AUTH_VECTORS;
//      AssertFatal(IS_EMM_CTXT_VALID_AUTH_VECTOR(emm_ctx, eksi_mod), "TODO No valid vector, should not happen");
// todo: validate that the AUTH_VECTOR is VALID (from MM EPS context )


  /** NAS counts. */
  memcpy (emm_ctx_p->_vector[0].kasme, mm_eps_ctxt->k_asme, AUTH_KASME_SIZE);
  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Received Vector %u from S10 Context Response from Source MME:\n", 0);
  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Received KASME .: " KASME_FORMAT " " KASME_FORMAT "\n",
      KASME_DISPLAY_1 (emm_ctx_p->_vector[0].kasme), KASME_DISPLAY_2 (emm_ctx_p->_vector[0].kasme));
  emm_ctx_set_attribute_present(emm_ctx_p, EMM_CTXT_MEMBER_AUTH_VECTOR0); /**< Still setting AUTH_VECTOR present todo: only set them via quanXXX from MM context. */
  emm_ctx_set_attribute_present(emm_ctx_p, EMM_CTXT_MEMBER_AUTH_VECTORS);

  ksi_t eksi = 0;
  if (emm_ctx_p->_security.eksi !=  KSI_NO_KEY_AVAILABLE) {
    AssertFatal(0 !=  0, "emm_ctx->_security.eksi %d", emm_ctx_p->_security.eksi);
    eksi = (emm_ctx_p->_security.eksi + 1) % (EKSI_MAX_VALUE + 1);
  }
  emm_ctx_set_security_vector_index(emm_ctx_p, 0);

  /**
   * Don't start the authentication procedure.
   * Validate the rest of the parameters which were received with Authentication Response from UE in normal attach.
   */
  emm_ctx_p->auth_sync_fail_count = 0;

  emm_ctx_set_security_eksi(emm_ctx_p, mm_eps_ctxt->ksi);
  OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC  - Setting the UE authentication parameters from S10 Context Response as successfull for UE "
      "with IMSI " IMSI_64_FMT ". \n", emm_ctx_p->_imsi64);

  /**
   * Continue with the parameters set and validated in the SECURITY_MODE_CONTROL messages.
   */
  /** NAS counts. */
  emm_ctx_p->_security.ul_count = mm_eps_ctxt->nas_ul_count;
  emm_ctx_p->_security.dl_count = mm_eps_ctxt->nas_dl_count;

  /** Increment the NAS UL_COUNT. */
  emm_ctx_p->_security.ul_count.seq_num += 1;
  if (!emm_ctx_p->_security.ul_count.seq_num) {
    emm_ctx_p->_security.ul_count.overflow += 1;
  }

  /**
   * Set the NAS ciphering and integrity keys.
   * Check if the UE & MS network capabilities are already set.
   */

  /** UE & MS Network Capabilities. */
  if (IS_EMM_CTXT_PRESENT_UE_NETWORK_CAPABILITY(emm_ctx_p)) {
    OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC  - UE network capabilities already present for IMSI " IMSI_64_FMT ". Not setting from S10 Context Response. \n", emm_ctx_p->_imsi64);
  }else{
    emm_ctx_set_ue_nw_cap(emm_ctx_p, &mm_eps_ctxt->ue_nc);
  }
  if (IS_EMM_CTXT_PRESENT_MS_NETWORK_CAPABILITY(emm_ctx_p)) {
    OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC  - MS network capabilities already present for IMSI " IMSI_64_FMT ". Not setting from S10 Context Response. \n", emm_ctx_p->_imsi64);
  }else{
    emm_ctx_set_attribute_present(emm_ctx_p, EMM_CTXT_MEMBER_MS_NETWORK_CAPABILITY_IE);
    memcpy((void*)&emm_ctx_p->_ms_network_capability, (ms_network_capability_t*)&mm_eps_ctxt->ms_nc, sizeof(ms_network_capability_t)); // todo: review this
    // todo: review all of this
//    emm_ctx_p->gea = (mm_eps_ctxt->ms_nc.gea1 << 6)| mm_eps_ctxt->ms_nc.egea;
//    emm_ctx_p->gprs_present = true; /**< Todo: how to find this out? */
  }
  // todo: these parameters are not present?
//  emm_ctx->_security.capability.eps_encryption = mm_eps_ctxt->ue_nc.eea;
//  emm_ctx->_security.capability.eps_integrity  = mm_eps_ctxt->ue_nc.eia;
//  if(mm_eps_ctxt->ue_nc.umts_present){
//    emm_ctx->_security.capability.umts_present    = mm_eps_ctxt->ue_nc.umts_present;
//    emm_ctx->_security.capability.umts_encryption = mm_eps_ctxt->ue_nc.uea;
//    emm_ctx->_security.capability.umts_integrity  = mm_eps_ctxt->ue_nc.uia;
//  }

  /** Set the selected encryption and integrity algorithms from the S10 Context Request. */
  emm_ctx_p->_security.selected_algorithms.encryption = mm_eps_ctxt->nas_cipher_alg;
  emm_ctx_p->_security.selected_algorithms.integrity = mm_eps_ctxt->nas_int_alg;

  emm_ctx_set_security_type(emm_ctx_p, SECURITY_CTX_TYPE_FULL_NATIVE);
  /** Derive the KNAS integrity and ciphering keys. */
  AssertFatal(EMM_SECURITY_VECTOR_INDEX_INVALID != emm_ctx_p->_security.vector_index, "Vector index not initialized");
  AssertFatal(MAX_EPS_AUTH_VECTORS >  emm_ctx_p->_security.vector_index, "Vector index outbound value %d/%d", emm_ctx_p->_security.vector_index, MAX_EPS_AUTH_VECTORS);
  derive_key_nas (NAS_INT_ALG, emm_ctx_p->_security.selected_algorithms.integrity,  emm_ctx_p->_vector[emm_ctx_p->_security.vector_index].kasme, emm_ctx_p->_security.knas_int);
  derive_key_nas (NAS_ENC_ALG, emm_ctx_p->_security.selected_algorithms.encryption, emm_ctx_p->_vector[emm_ctx_p->_security.vector_index].kasme, emm_ctx_p->_security.knas_enc);

  memcpy(emm_ctx_p->_vector[emm_ctx_p->_security.vector_index].kasme, mm_eps_ctxt->k_asme, 32);

  /**
   * Set new security context indicator.
   */
  emm_ctx_set_attribute_valid(emm_ctx_p, EMM_CTXT_MEMBER_SECURITY);

  /**
   * All fields set and validated like SECURITY_MODE_COMPLETE was received and processed from the UE.
   * Set the rest parameters received in the MM_UE_EPS_CONTEXT.
   */

  /* No need to abort any common procedures. Directly continue with the PDN Configuration Request procedure in both cases (ULR). */


  /**
   * Set the current NCC and NH key.
   * Eventually derive a new one with the TAU_ACCEPT.
   */
  emm_ctx_p->_security.ncc      = mm_eps_ctxt->ncc;
  memcpy(emm_ctx_p->_security.nh_conj, mm_eps_ctxt->nh, 32);
  memcpy(emm_ctx_p->_vector[emm_ctx_p->_security.vector_index].nh_conj, mm_eps_ctxt->nh, 32);
  /** All remaining capabilities should be set with the TAC/Attach Request. */
  OAILOG_FUNC_OUT (LOG_NAS_EMM);
}

//------------------------------------------------------------------------------
struct emm_data_context_s              *
emm_data_context_get (
  emm_data_t * emm_data,
  const mme_ue_s1ap_id_t ue_id)
{
  struct emm_data_context_s              *emm_data_context_p = NULL;

  DevAssert (emm_data );
  if (INVALID_MME_UE_S1AP_ID != ue_id) {
    hashtable_ts_get (emm_data->ctx_coll_ue_id, (const hash_key_t)(ue_id), (void **)&emm_data_context_p);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-CTX - get UE id " MME_UE_S1AP_ID_FMT " context %p\n", ue_id, emm_data_context_p);
  }
  return emm_data_context_p;
}

//------------------------------------------------------------------------------
struct emm_data_context_s              *
emm_data_context_get_by_imsi (
  emm_data_t * emm_data,
  imsi64_t     imsi64)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
  mme_ue_s1ap_id_t                       *emm_ue_id_p = NULL;

  DevAssert (emm_data );

  h_rc = hashtable_ts_get (emm_data->ctx_coll_imsi, (const hash_key_t)imsi64,  /* sizeof(imsi64_t), */(void **)&emm_ue_id_p);

  if (HASH_TABLE_OK == h_rc) {
    struct emm_data_context_s * tmp = emm_data_context_get (emm_data, (const hash_key_t)*emm_ue_id_p);
#if DEBUG_IS_ON
    if ((tmp)) {
      OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - get UE id " MME_UE_S1AP_ID_FMT " context %p by imsi " IMSI_64_FMT "\n", tmp->ue_id, tmp, imsi64);
    }
#endif
    return tmp;
  }
  return NULL;
}

//------------------------------------------------------------------------------
struct emm_data_context_s              *
emm_data_context_get_by_guti (
  emm_data_t * emm_data,
  guti_t * guti)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
  mme_ue_s1ap_id_t                        *emm_ue_id_p = NULL;

  DevAssert (emm_data );

  if ( guti) {

    h_rc = obj_hashtable_uint64_ts_get (emm_data->ctx_coll_guti, (const void *)guti, sizeof (*guti), (void **) &emm_ue_id_p);

    if (HASH_TABLE_OK == h_rc) {
      struct emm_data_context_s * tmp = emm_data_context_get (emm_data, *emm_ue_id_p);
#if DEBUG_IS_ON
      if ((tmp)) {
        OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - get UE id " MME_UE_S1AP_ID_FMT " context %p by guti " GUTI_FMT "\n", tmp->ue_id, tmp, GUTI_ARG(guti));
      }
#endif
      return tmp;
    }
  }
  return NULL;
}

//------------------------------------------------------------------------------
int emm_context_unlock (struct emm_data_context_s *emm_context_p)
{
  if (emm_context_p) {
//    return unlock_ue_contexts(PARENT_STRUCT(emm_context_p, struct ue_context_s, emm_context));
  }
  return RETURNerror;
}

//------------------------------------------------------------------------------
struct emm_data_context_s              *
emm_data_context_remove (
  emm_data_t * emm_data,
  struct emm_data_context_s *elm)
{
  struct emm_data_context_s              *emm_data_context_p = NULL;
  mme_ue_s1ap_id_t                       *emm_ue_id          = NULL;

  OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Remove in context %p UE id " MME_UE_S1AP_ID_FMT "\n", elm, elm->ue_id);

  if ( IS_EMM_CTXT_PRESENT_GUTI(elm)) {
    obj_hashtable_ts_remove(emm_data->ctx_coll_guti, (const void *) &elm->_guti, sizeof(elm->_guti),
                            (void **) &emm_ue_id);
    if (emm_ue_id) {
      // The GUTI is only inserted as part of attach complete, so it might be NULL.
      OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Remove in ctx_coll_guti context %p UE id "
          MME_UE_S1AP_ID_FMT " guti " " " GUTI_FMT "\n", elm, (mme_ue_s1ap_id_t) (*emm_ue_id), GUTI_ARG(&elm->_guti));
    }
    emm_ctx_clear_guti(elm);
  }

  if ( IS_EMM_CTXT_PRESENT_IMSI(elm)) {
    imsi64_t imsi64 = imsi_to_imsi64(&elm->_imsi);
    hashtable_ts_remove (emm_data->ctx_coll_imsi, (const hash_key_t)imsi64, (void **)&emm_ue_id);

    OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Remove in ctx_coll_imsi context %p UE id " MME_UE_S1AP_ID_FMT " imsi " IMSI_64_FMT "\n",
                  elm, (mme_ue_s1ap_id_t)((uintptr_t)emm_ue_id), imsi64);
    emm_ctx_clear_imsi(elm);
  }

  hashtable_ts_remove (emm_data->ctx_coll_ue_id, (const hash_key_t)(elm->ue_id), (void **)&emm_data_context_p);
  return emm_data_context_p;
}

//------------------------------------------------------------------------------
void 
emm_data_context_remove_mobile_ids (
  emm_data_t * emm_data, struct emm_data_context_s *elm)
{

  OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Remove in context %p UE id " MME_UE_S1AP_ID_FMT "\n", elm, elm->ue_id);

//  if ( IS_EMM_CTXT_PRESENT_GUTI(elm)) {
//    obj_hashtable_uint64_ts_remove(emm_data->ctx_coll_guti, (const void *) &elm->_guti, sizeof(elm->_guti));
//  }
//
//  emm_ctx_clear_guti(elm);
//
//  if ( IS_EMM_CTXT_PRESENT_IMSI(elm)) {
//    imsi64_t imsi64 = imsi_to_imsi64(&elm->_imsi);
//    hashtable_uint64_ts_remove (emm_data->ctx_coll_imsi, (const hash_key_t)imsi64);
//  }
//  emm_ctx_clear_imsi(elm);
//  return;
}
//
////------------------------------------------------------------------------------
//int
//emm_context_upsert_imsi (
//    emm_data_t * emm_data,
//    struct emm_data_context_s *elm)
//{
//  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
//  mme_ue_s1ap_id_t                        ue_id = elm->ue_id;
//
//  h_rc = hashtable_uint64_ts_remove (emm_data->ctx_coll_imsi, (const hash_key_t)elm->_imsi64);
//  if (INVALID_MME_UE_S1AP_ID != ue_id) {
//    h_rc = hashtable_uint64_ts_insert (emm_data->ctx_coll_imsi, (const hash_key_t)(const hash_key_t)elm->_imsi64, ue_id);
//  } else {
//    h_rc = HASH_TABLE_KEY_NOT_EXISTS;
//  }
//  if (HASH_TABLE_OK != h_rc) {
//    OAILOG_TRACE (LOG_MME_APP,
//        "Error could not update this ue context mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " imsi " IMSI_64_FMT ": %s\n",
//        ue_id, elm->_imsi64, hashtable_rc_code2string(h_rc));
//    return RETURNerror;
//  }
//  return RETURNok;
//}



////------------------------------------------------------------------------------
//
//int
//emm_data_context_upsert_imsi (
//    emm_data_t * emm_data,
//    struct emm_data_context_s *elm)
//{
//  hashtable_rc_t                          h_rc = HASH_TABLE_OK;
//
//  h_rc = _emm_data_context_hashtable_insert(emm_data, elm);
//
//  if (HASH_TABLE_OK == h_rc || HASH_TABLE_INSERT_OVERWRITTEN_DATA == h_rc) {
//    OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Upsert in context UE id " MME_UE_S1AP_ID_FMT " with IMSI "IMSI_64_FMT"\n",
//                  elm->ue_id, elm->_imsi64);
//    return RETURNok;
//  } else {
//    OAILOG_ERROR (LOG_NAS_EMM, "EMM-CTX - Upsert in context UE id " MME_UE_S1AP_ID_FMT " with IMSI "IMSI_64_FMT" "
//        "Failed %s\n", elm->ue_id, elm->_imsi64, hashtable_rc_code2string (h_rc));
//    return RETURNerror;
//  }
//}
//

//------------------------------------------------------------------------------
void emm_init_context(struct emm_data_context_s * const emm_ctx, const bool init_esm_ctxt)
{
  emm_ctx->_emm_fsm_state  = EMM_DEREGISTERED;

  OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " Init EMM-CTX\n", emm_ctx->ue_id);

  bdestroy_wrapper(&emm_ctx->esm_msg);
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
  emm_ctx_clear_drx_parameter(emm_ctx);
//  emm_ctx_clear_pending_current_drx_parameter(&emm_ctx);
//  emm_ctx_clear_eps_bearer_context_status(emm_ctx); todo

  if (init_esm_ctxt) {
    esm_init_context(&emm_ctx->esm_ctx);
  }
}

//------------------------------------------------------------------------------
int emm_data_context_update_security_parameters(const mme_ue_s1ap_id_t ue_id,
    uint16_t *encryption_algorithm_capabilities,
    uint16_t *integrity_algorithm_capabilities){
  uint8_t                 kenb[32];
  uint32_t                ul_nas_count;

  OAILOG_FUNC_IN (LOG_NAS_EMM);

  emm_data_context_t                     *emm_ctx = emm_data_context_get (&_emm_data, ue_id);

  if (!emm_ctx) {
    OAILOG_ERROR(LOG_NAS_EMM, "EMM-CTX - no EMM context existing for UE id " MME_UE_S1AP_ID_FMT ". Cannot update the AS security. \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }
  if (!IS_EMM_CTXT_VALID_SECURITY(emm_ctx)) {
    OAILOG_ERROR(LOG_NAS_EMM, "EMM-CTX - no valid security context exist EMM context for UE id " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT ". Cannot update the AS security. \n",
        ue_id, emm_ctx->_imsi64);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }

  AssertFatal((0 <= emm_ctx->_security.vector_index) && (MAX_EPS_AUTH_VECTORS > emm_ctx->_security.vector_index),
      "Invalid vector index %d", emm_ctx->_security.vector_index);

  *encryption_algorithm_capabilities = ((uint16_t)emm_ctx->_ue_network_capability.eea & ~(1 << 7)) << 1;
  *integrity_algorithm_capabilities = ((uint16_t)emm_ctx->_ue_network_capability.eia & ~(1 << 7)) << 1;

  /** Derive the next hop. */
  derive_nh(emm_ctx->_vector[emm_ctx->_security.vector_index].kasme, emm_ctx->_vector[emm_ctx->_security.vector_index].nh_conj);

  /** Increase the next hop counter. */
  emm_ctx->_security.ncc++;
  /* The NCC is a 3-bit key index (values from 0 to 7) for the NH and is sent to the UE in the handover command signaling. */
  emm_ctx->_security.ncc = emm_ctx->_security.ncc % 8;

  OAILOG_INFO(LOG_NAS_EMM, "EMM-CTX - Updated AS security parameters for EMM context with UE id " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT ". \n",
          ue_id, emm_ctx->_imsi64);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

//------------------------------------------------------------------------------
void emm_data_context_get_security_parameters(const mme_ue_s1ap_id_t ue_id,
    uint16_t *encryption_algorithm_capabilities,
    uint16_t *integrity_algorithm_capabilities){
  uint8_t                 kenb[32];
  uint32_t                ul_nas_count;

  OAILOG_FUNC_IN (LOG_NAS_EMM);

  emm_data_context_t                     *emm_ctx = emm_data_context_get (&_emm_data, ue_id);

  if (!emm_ctx) {
    OAILOG_ERROR(LOG_NAS_EMM, "EMM-CTX - no EMM context existing for UE id " MME_UE_S1AP_ID_FMT ". Cannot update the AS security. \n", ue_id);
    OAILOG_FUNC_OUT(LOG_NAS_EMM);
  }
  if (!IS_EMM_CTXT_VALID_SECURITY(emm_ctx)) {
    OAILOG_ERROR(LOG_NAS_EMM, "EMM-CTX - no valid security context exist EMM context for UE id " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT ". Cannot update the AS security. \n",
        ue_id, emm_ctx->_imsi64);
    OAILOG_FUNC_OUT(LOG_NAS_EMM);
  }

  AssertFatal((0 <= emm_ctx->_security.vector_index) && (MAX_EPS_AUTH_VECTORS > emm_ctx->_security.vector_index),
      "Invalid vector index %d", emm_ctx->_security.vector_index);

  *encryption_algorithm_capabilities = ((uint16_t)emm_ctx->_ue_network_capability.eea & ~(1 << 7)) << 1;
  *integrity_algorithm_capabilities = ((uint16_t)emm_ctx->_ue_network_capability.eia & ~(1 << 7)) << 1;

  OAILOG_FUNC_OUT(LOG_NAS_EMM);
}

//------------------------------------------------------------------------------
int _start_context_request_procedure(struct emm_data_context_s *emm_context, nas_emm_specific_proc_t * const spec_proc,
    success_cb_t * _context_res_proc_success, failure_cb_t* _context_res_proc_fail)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  mme_ue_s1ap_id_t                        ue_id = emm_context->ue_id;
  // Ask upper layer to fetch new security context
  nas_ctx_req_proc_t * ctx_req_proc = get_nas_cn_procedure_ctx_req(emm_context);
  if (!ctx_req_proc) {
    ctx_req_proc = nas_new_cn_ctx_req_procedure(emm_context);
  }
  /*
   * Context request.
   */
  ctx_req_proc->cn_proc.base_proc.parent = (nas_base_proc_t*)&spec_proc;
  /*
   * Not configuring an AS common procedure as parent procedure of the S10 procedure.
   */
//  ctx_req_proc->emm_com_proc.emm_proc.base_proc.child = &auth_info_proc->cn_proc.base_proc;
  ctx_req_proc->success_notif = _context_res_proc_success; /**< Continue with a PDN Configuration (ULR, CSR). */
  ctx_req_proc->failure_notif = _context_res_proc_fail; /**< Continue with the identification procedure (IdReq, AuthReq, SMC). */
  ctx_req_proc->cn_proc.base_proc.time_out = s10_context_req_timer_expiry_handler;
  ctx_req_proc->ue_id = ue_id;

  nas_start_Ts10_ctx_req( ctx_req_proc->ue_id, &ctx_req_proc->timer_s10, ctx_req_proc->cn_proc.base_proc.time_out, emm_context);

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

// todo: start_identification_procedure for attach!

//------------------------------------------------------------------------------
static int _ctx_req_proc_success_cb (struct emm_data_context_s *emm_ctx_p)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  nas_ctx_req_proc_t * ctx_req_proc = get_nas_cn_procedure_ctx_req(emm_ctx_p);
  int                                     rc = RETURNerror;

  if (ctx_req_proc) {
    if (!emm_ctx_p) {
      OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Failed to find UE id " MME_UE_S1AP_ID_FMT "\n", ctx_req_proc->ue_id);
      // todo: not deleting the CN procedure?
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    }

    /** Check that there is a specific procedure running. */
    if(ctx_req_proc->cn_proc.base_proc.parent){
      /**
       * Set the identity values (IMSI) valid and present.
       * Assuming IMSI is always returned with S10 Context Response and the IMSI hastable registration method validates the received IMSI.
       */
      clear_imsi(&emm_ctx_p->_imsi);
      emm_ctx_set_valid_imsi(emm_ctx_p, &ctx_req_proc->nas_s10_context._imsi, ctx_req_proc->nas_s10_context.imsi);

      //  todo:    emm_data_context_upsert_imsi(&_emm_data, emm_ctx_p); /**< Register the IMSI in the hash table. */

      // todo: identification
  //    if (rc != RETURNok) {
  //      OAILOG_ERROR(LOG_NAS_EMM, "EMM-PROC  - " "Error inserting EMM_DATA_CONTEXT for mmeUeS1apId " MME_UE_S1AP_ID_FMT " "
  //          "for the RECEIVED IMSI " IMSI_64_FMT ". \n", emm_ctx_p->ue_id, msg->imsi);
  //      /** Sending TAU or Attach Reject back, which will purge the EMM/MME_APP UE contexts. */
  //      if(is_nas_specific_procedure_tau_running(emm_ctx_p)){
  //        rc = emm_proc_tracking_area_update_reject(emm_ctx_p->ue_id, SYSTEM_FAILURE);
  //        // todo: consider this as an internal error, not continue with identification procedure.
  //        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  //      }else if(is_nas_specific_procedure_attach_running(emm_ctx_p)){
  //        rc = emm_proc_attach_reject(emm_ctx_p->ue_id, msg->cause);
  //        // todo: consider this as an internal error, not continue with identification procedure.
  //        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  //      }
  //      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  //    }

      /*
       * Update the security context & security vectors of the UE independent of TAU/Attach here (set fields valid/present).
       * Then inform the MME_APP that the context was successfully authenticated. Trigger a CSR.
       */
      emm_ctx_update_from_mm_eps_context(emm_ctx_p, ctx_req_proc->nas_s10_context.mm_eps_ctx);
      OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - " "Successfully updated the EMM context with ueId " MME_UE_S1AP_ID_FMT " from the received MM_EPS_Context from the MME for UE with imsi: " IMSI_64_FMT ". \n",
          emm_ctx_p->ue_id, emm_ctx_p->_imsi64);

      /** Delete the CN procedure. */
      nas_delete_cn_procedure(emm_ctx_p, &ctx_req_proc->cn_proc);

//      if (rc != RETURNok) {
//        emm_sap_t                               emm_sap = {0};
//        emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
//        emm_sap.u.emm_reg.ue_id     = ue_id;
//        emm_sap.u.emm_reg.ctx       = emm_ctx;
//        emm_sap.u.emm_reg.notify    = true;
//        emm_sap.u.emm_reg.free_proc = true;
//        emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
//        emm_sap.u.emm_reg.u.common.previous_emm_fsm_state = auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
//        rc = emm_sap_send (&emm_sap);
//      }
    } else {
      OAILOG_ERROR(LOG_NAS_EMM, "EMM-PROC  - " "Could not find a parent specific procedure for the EMM context with ueId " MME_UE_S1AP_ID_FMT " with imsi: " IMSI_64_FMT ". \n",
          emm_ctx_p->ue_id, emm_ctx_p->_imsi64);
      nas_delete_cn_procedure(emm_ctx_p, &ctx_req_proc->cn_proc);
    }
  }
  // todo: pdn config req!//ULR!//tau_accept!!
//    /*
//     * Currently, the PDN_CONNECTIVITY_REQUEST ESM IE is not processed by the ESM layer.
//     * Later, with multi APN, process it and also the PDN_CONNECTION IE.
//     * Currently, just send ITTI signal to build up session via the stored pending PDN information.
//     */
//
//    /**
//     * Prepare a CREATE_SESSION_REQUEST message from the pending data.
//     * NO (default) apn_configuration is present (or expected) since ULA is not sent yet to HSS.
//     */
//    rc =  mme_app_send_s11_create_session_req_from_handover_tau(emm_ctx->ue_id);
//    /** Leave the UE in EMM_DEREGISTERED state until TAU_ACCEPT. */
//    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
//  }else{

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
}

//------------------------------------------------------------------------------
static int _ctx_req_proc_failure_cb (struct emm_data_context_s *emm_ctx)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  nas_ctx_req_proc_t * ctx_req_proc = get_nas_cn_procedure_ctx_req(emm_ctx);
  int                                     rc = RETURNerror;

  /**
   * An UE could or could not exist. We need to check. If it exists, it needs to be purged.
   * Since no UE context is established yet, we don't have security/no bearers.0
   * If the message is received after the timeout, the MME_APP context also should not exist.
   * If the MME_APP context existed at that point in time, it will later be removed.
   * Just discard the message then.
   */
  emm_ctx = emm_data_context_get (&_emm_data, emm_ctx->ue_id);

  if (ctx_req_proc) {
    nas_emm_specific_proc_t  * emm_specific_proc = (nas_emm_specific_proc_t*)((nas_base_proc_t *)ctx_req_proc)->parent;

    int emm_cause = ctx_req_proc->nas_cause;
    nas_delete_cn_procedure(emm_ctx, &ctx_req_proc->cn_proc);
    /** EMM Context. */
    if (emm_ctx == NULL) {
      OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Failed to find UE associated to id " MME_UE_S1AP_ID_FMT "...\n", emm_ctx->ue_id);
      /*
       * In this case, don't wait for the timer to remove the rest! Assume no timers exist.
       * Purge the rest of the UE context (MME_APP etc.).
       */
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    }

    /** Get the specific procedure. The CN procedure is not part of a common procedure. */
    if(emm_specific_proc){
      /** Continue with an identification procedure. */
      // todo: currently execute the failure method!
      OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - " "Rejecting the running specific procedure for UE associated to id " MME_UE_S1AP_ID_FMT " of type %d (todo: identification procedure)...\n", emm_ctx->ue_id, emm_specific_proc->type);
      // todo: enter the correct method..
      // todo: add this into the correct method
//      if(is_nas_specific_procedure_tau_running(emm_ctx)){
//        nas_emm_tau_proc_t * tau_proc = (nas_emm_tau_proc_t*)emm_specific_proc;
//        tau_proc->emm_cause = emm_cause;
//        emm_sap_t                               emm_sap = {0};
//        emm_sap.primitive = EMMREG_TAU_REJ;
//        emm_sap.u.emm_reg.ue_id = emm_ctx->ue_id;
//        emm_sap.u.emm_reg.ctx = emm_ctx;
//        emm_sap.u.emm_reg.notify = false;
//        emm_sap.u.emm_reg.free_proc = true;
//        emm_sap.u.emm_reg.u.tau.proc = tau_proc;
//        rc = emm_sap_send (&emm_sap);
//      }else if (is_nas_specific_procedure_attach_running(emm_ctx)) {
//        nas_emm_attach_proc_t * attach_proc = (nas_emm_attach_proc_t*)emm_specific_proc;
//        attach_proc->emm_cause = emm_cause;
//        emm_sap_t                               emm_sap = {0};
//        emm_sap.primitive = EMMREG_ATTACH_REJ;
//        emm_sap.u.emm_reg.ue_id = emm_ctx->ue_id;
//        emm_sap.u.emm_reg.ctx = emm_ctx;
//        emm_sap.u.emm_reg.notify = false;
//        emm_sap.u.emm_reg.free_proc = true;
//        emm_sap.u.emm_reg.u.attach.proc = attach_proc;
//        rc = emm_sap_send (&emm_sap);
//      }else{
//        OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - " "Performing an implicit detach due %d for id " MME_UE_S1AP_ID_FMT " (todo: identification procedure)...\n", emm_specific_proc->type, emm_ctx->ue_id);
//        // todo: implicit detach
//
//      }
    }else{
      OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - " "Failed to find specific procedure for UE associated to id " MME_UE_S1AP_ID_FMT "...\n", emm_ctx->ue_id);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }
    //    nas_emm_ctauth_proc_t * auth_proc = get_nas_common_procedure_authentication(emm_ctx);
    /*
     * Check the TAU status/flag.
     * Depending on the received TAU procedure (must exist, return an TAU accept back).
     */

    OAILOG_ERROR(LOG_NAS_EMM, "EMM-PROC  - " "UE context failure received for ue_id " MME_UE_S1AP_ID_FMT " which is neither is in TAU nor Attach procedure. "
        "Continuing to reject the specific procedure. \n", emm_ctx->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }
  OAILOG_ERROR(LOG_NAS_EMM, "EMM-PROC  - " "Received NULL CN-procedure for ue_id " MME_UE_S1AP_ID_FMT ". \n", emm_ctx->ue_id);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
}

//------------------------------------------------------------------------------
void nas_start_T3450(const mme_ue_s1ap_id_t ue_id, struct nas_timer_s * const T3450,  time_out_t time_out_cb, void *timer_callback_args)
{
  if ((T3450) && (T3450->id == NAS_TIMER_INACTIVE_ID)) {
    T3450->id = nas_timer_start (T3450->sec, 0, time_out_cb, timer_callback_args);
    if (NAS_TIMER_INACTIVE_ID != T3450->id) {
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3450 started UE " MME_UE_S1AP_ID_FMT " ", ue_id);
      OAILOG_DEBUG (LOG_NAS_EMM, "T3450 started UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
    } else {
      OAILOG_ERROR (LOG_NAS_EMM, "Could not start T3450 UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    }
  }
}
//------------------------------------------------------------------------------
void nas_start_T3460(const mme_ue_s1ap_id_t ue_id, struct nas_timer_s * const T3460,  time_out_t time_out_cb, void *timer_callback_args)
{
  if ((T3460) && (T3460->id == NAS_TIMER_INACTIVE_ID)) {
    T3460->id = nas_timer_start (T3460->sec, 0, time_out_cb, timer_callback_args);
    if (NAS_TIMER_INACTIVE_ID != T3460->id) {
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3460 started UE " MME_UE_S1AP_ID_FMT " ", ue_id);
      OAILOG_DEBUG (LOG_NAS_EMM, "T3460 started UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
    } else {
      OAILOG_ERROR (LOG_NAS_EMM, "Could not start T3460 UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    }
  }
}
//------------------------------------------------------------------------------
void nas_start_T3470(const mme_ue_s1ap_id_t ue_id, struct nas_timer_s * const T3470,  time_out_t time_out_cb, void *timer_callback_args)
{
  if ((T3470) && (T3470->id == NAS_TIMER_INACTIVE_ID)) {
    T3470->id = nas_timer_start (T3470->sec, 0, time_out_cb, timer_callback_args);
    if (NAS_TIMER_INACTIVE_ID != T3470->id) {
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3470 started UE " MME_UE_S1AP_ID_FMT " ", ue_id);
      OAILOG_DEBUG (LOG_NAS_EMM, "T3470 started UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
    } else {
      OAILOG_ERROR (LOG_NAS_EMM, "Could not start T3470 UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    }
  }
}

//------------------------------------------------------------------------------
void nas_start_Ts6a_auth_info(const mme_ue_s1ap_id_t ue_id, struct nas_timer_s * const Ts6a_auth_info,  time_out_t time_out_cb, void *timer_callback_args)
{
  if ((Ts6a_auth_info) && (Ts6a_auth_info->id == NAS_TIMER_INACTIVE_ID)) {
    Ts6a_auth_info->id = nas_timer_start (Ts6a_auth_info->sec, 0, time_out_cb, timer_callback_args);
    if (NAS_TIMER_INACTIVE_ID != Ts6a_auth_info->id) {
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 Ts6a_auth_info started UE " MME_UE_S1AP_ID_FMT " ", ue_id);
      OAILOG_DEBUG (LOG_NAS_EMM, "Ts6a_auth_info started UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
    } else {
      OAILOG_ERROR (LOG_NAS_EMM, "Could not start Ts6a_auth_info UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    }
  }
}

//------------------------------------------------------------------------------
void nas_start_Ts10_ctx_req(const mme_ue_s1ap_id_t ue_id, struct nas_timer_s * const Ts10_ctx_res,  time_out_t time_out_cb, void *timer_callback_args)
{
  if ((Ts10_ctx_res) && (Ts10_ctx_res->id == NAS_TIMER_INACTIVE_ID)) {
    Ts10_ctx_res->id = nas_timer_start (Ts10_ctx_res->sec, 0, time_out_cb, timer_callback_args);
    if (NAS_TIMER_INACTIVE_ID != Ts10_ctx_res->id) {
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 Ts10_ctx_res started UE " MME_UE_S1AP_ID_FMT " ", ue_id);
      OAILOG_DEBUG (LOG_NAS_EMM, "Ts10_ctx_res started UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
    } else {
      OAILOG_ERROR (LOG_NAS_EMM, "Could not start Ts10_ctx_res UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    }
  }
}

//------------------------------------------------------------------------------
void nas_start_T_retry_specific_procedure(const mme_ue_s1ap_id_t ue_id, struct nas_timer_s * const T_retry,  time_out_t time_out_cb, void *timer_callback_args)
{
  if ((T_retry) && (T_retry->id == NAS_TIMER_INACTIVE_ID)) {
    T_retry->id = nas_timer_start (T_retry->sec, 0, time_out_cb, timer_callback_args);
    if (NAS_TIMER_INACTIVE_ID != T_retry->id) {
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T_retry started UE " MME_UE_S1AP_ID_FMT " ", ue_id);
      OAILOG_DEBUG (LOG_NAS_EMM, "T_retry started UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
    } else {
      OAILOG_ERROR (LOG_NAS_EMM, "Could not start T_retry UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    }
  }
}

//------------------------------------------------------------------------------
void nas_stop_T3450(const mme_ue_s1ap_id_t ue_id, struct nas_timer_s * const T3450, void *timer_callback_args)
{
  if ((T3450) && (T3450->id != NAS_TIMER_INACTIVE_ID)) {
    T3450->id = nas_timer_stop(T3450->id, &timer_callback_args);
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3450 stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    OAILOG_DEBUG (LOG_NAS_EMM, "T3450 stopped UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
  }
}

//------------------------------------------------------------------------------
void nas_stop_T3460(const mme_ue_s1ap_id_t ue_id, struct nas_timer_s * const T3460, void *timer_callback_args)
{
  if ((T3460) && (T3460->id != NAS_TIMER_INACTIVE_ID)) {
    T3460->id = nas_timer_stop(T3460->id, &timer_callback_args);
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3460 stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    OAILOG_DEBUG (LOG_NAS_EMM, "T3460 stopped UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
  }
}

//------------------------------------------------------------------------------
void nas_stop_T3470(const mme_ue_s1ap_id_t ue_id, struct nas_timer_s * const T3470, void *timer_callback_args)
{
  if ((T3470) && (T3470->id != NAS_TIMER_INACTIVE_ID)) {
    T3470->id = nas_timer_stop(T3470->id, &timer_callback_args);
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3470 stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    OAILOG_DEBUG (LOG_NAS_EMM, "T3470 stopped UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
  }
}

//------------------------------------------------------------------------------
void nas_stop_Ts6a_auth_info(const mme_ue_s1ap_id_t ue_id, struct nas_timer_s * const Ts6a_auth_info, void *timer_callback_args)
{
  if ((Ts6a_auth_info) && (Ts6a_auth_info->id != NAS_TIMER_INACTIVE_ID)) {
    Ts6a_auth_info->id = nas_timer_stop(Ts6a_auth_info->id, &timer_callback_args);
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 Ts6a_auth_info stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    OAILOG_DEBUG (LOG_NAS_EMM, "Ts6a_auth_info stopped UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
  }
}

//------------------------------------------------------------------------------
void nas_stop_Ts10_ctx_res(const mme_ue_s1ap_id_t ue_id, struct nas_timer_s * const Ts10_ctx_res, void *timer_callback_args)
{
  if ((Ts10_ctx_res) && (Ts10_ctx_res->id != NAS_TIMER_INACTIVE_ID)) {
    Ts10_ctx_res->id = nas_timer_stop(Ts10_ctx_res->id, &timer_callback_args);
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 Ts10_ctx_res stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    OAILOG_DEBUG (LOG_NAS_EMM, "Ts10_ctx_res stopped UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
  }
}

//------------------------------------------------------------------------------
void nas_stop_T_retry_specific_procedure(const mme_ue_s1ap_id_t ue_id, struct nas_timer_s * const T_retry, void *timer_callback_args)
{
  if ((T_retry) && (T_retry->id != NAS_TIMER_INACTIVE_ID)) {
    T_retry->id = nas_timer_stop(T_retry->id, &timer_callback_args);
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T_retry stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    OAILOG_DEBUG (LOG_NAS_EMM, "T_retry stopped UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
  }
}

//------------------------------------------------------------------------------
void emm_context_dump (
  const struct emm_data_context_s * const emm_context,
  const uint8_t         indent_spaces,
  bstring bstr_dump)
{
  if (emm_context ) {
    char                                    key_string[KASME_LENGTH_OCTETS * 2+1];
    char                                    imsi_str[16+1];
    int                                     k = 0,
                                            size = 0,
                                            remaining_size = 0;


    bformata (bstr_dump, "%*s - EMM-CTX: ue id:           " MME_UE_S1AP_ID_FMT " (UE identifier)\n", indent_spaces, " ", emm_context->ue_id);
    bformata (bstr_dump, "%*s     - is_dynamic:       %u      (Dynamically allocated context indicator)\n", indent_spaces, " ", emm_context->is_dynamic);
//    bformata (bstr_dump, "%*s     - is_attached:      %u      (Attachment indicator)\n", indent_spaces, " ", emm_context->is_attached);
    bformata (bstr_dump, "%*s     - is_has_been_attached:      %u      (Attachment indicator)\n", indent_spaces, " ", emm_context->is_has_been_attached);
    bformata (bstr_dump, "%*s     - is_emergency:     %u      (Emergency bearer services indicator)\n", indent_spaces, " ", emm_context->is_emergency);
    if (IS_EMM_CTXT_PRESENT_IMSI(emm_context)) {
      IMSI_TO_STRING (&emm_context->_imsi, imsi_str, 16);
      bformata (bstr_dump, "%*s     - imsi:             %s      (The IMSI provided by the UE or the MME)\n", indent_spaces, " ", imsi_str);
    } else {
      bformata (bstr_dump, "%*s     - imsi:             UNKNOWN\n", indent_spaces, " ");
    }
    bformata (bstr_dump, "%*s     - imei:             TODO    (The IMEI provided by the UE)\n", indent_spaces, " ");
    if (IS_EMM_CTXT_PRESENT_IMEISV(emm_context)) {
      bformata (bstr_dump, "%*s     - imeisv:           %x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x \n", indent_spaces, " ",
        emm_context->_imeisv.u.num.tac1, emm_context->_imeisv.u.num.tac2, emm_context->_imeisv.u.num.tac3, emm_context->_imeisv.u.num.tac4,
        emm_context->_imeisv.u.num.tac5, emm_context->_imeisv.u.num.tac6, emm_context->_imeisv.u.num.tac7, emm_context->_imeisv.u.num.tac8,
        emm_context->_imeisv.u.num.snr1, emm_context->_imeisv.u.num.snr2, emm_context->_imeisv.u.num.snr3, emm_context->_imeisv.u.num.snr4,
        emm_context->_imeisv.u.num.snr5, emm_context->_imeisv.u.num.snr6, emm_context->_imeisv.u.num.svn1, emm_context->_imeisv.u.num.svn2);
    } else {
      bformata (bstr_dump, "%*s     - imeisv:           UNKNOWN\n", indent_spaces, " ");
    }
    if (IS_EMM_CTXT_PRESENT_GUTI(emm_context)) {
      bformata (bstr_dump, "%*s                         |  m_tmsi  | mmec | mmegid | mcc | mnc |\n", indent_spaces, " ");
      bformata (bstr_dump, "%*s     - GUTI............: | %08x |  %02x  |  %04x  | %u%u%u | %u%u%c |\n", indent_spaces, " ",
        emm_context->_guti.m_tmsi, emm_context->_guti.gummei.mme_code,
        emm_context->_guti.gummei.mme_gid,
        emm_context->_guti.gummei.plmn.mcc_digit1,
        emm_context->_guti.gummei.plmn.mcc_digit2,
        emm_context->_guti.gummei.plmn.mcc_digit3,
        emm_context->_guti.gummei.plmn.mnc_digit1,
        emm_context->_guti.gummei.plmn.mnc_digit2,
        (emm_context->_guti.gummei.plmn.mnc_digit3 > 9) ? ' ':0x30+emm_context->_guti.gummei.plmn.mnc_digit3);
      //bformata (bstr_dump, "%*s     - guti:             "GUTI_FMT"      (The GUTI assigned to the UE)\n", indent_spaces, " ", GUTI_ARG(&emm_context->_guti));
    } else {
      bformata (bstr_dump, "%*s     - GUTI............: UNKNOWN\n", indent_spaces, " ");
    }
    if (IS_EMM_CTXT_PRESENT_OLD_GUTI(emm_context)) {
      bformata (bstr_dump, "%*s                         |  m_tmsi  | mmec | mmegid | mcc | mnc |\n", indent_spaces, " ");
      bformata (bstr_dump, "%*s     - OLD GUTI........: | %08x |  %02x  |  %04x  | %u%u%u | %u%u%c |\n", indent_spaces, " ",
        emm_context->_old_guti.m_tmsi, emm_context->_old_guti.gummei.mme_code,
        emm_context->_old_guti.gummei.mme_gid,
        emm_context->_old_guti.gummei.plmn.mcc_digit1,
        emm_context->_old_guti.gummei.plmn.mcc_digit2,
        emm_context->_old_guti.gummei.plmn.mcc_digit3,
        emm_context->_old_guti.gummei.plmn.mnc_digit1,
        emm_context->_old_guti.gummei.plmn.mnc_digit2,
        (emm_context->_old_guti.gummei.plmn.mnc_digit3 > 9) ? ' ':0x30+emm_context->_old_guti.gummei.plmn.mnc_digit3);
      //bformata (bstr_dump, "%*s     - old_guti:         "GUTI_FMT"      (The old GUTI)\n", indent_spaces, " ", GUTI_ARG(&emm_context->_old_guti));
    } else {
      bformata (bstr_dump, "%*s     - OLD GUTI........: UNKNOWN\n", indent_spaces, " ");
    }
    for (k=0; k < emm_context->_tai_list.numberoflists; k++) {
      switch (emm_context->_tai_list.partial_tai_list[k].typeoflist) {
      case TRACKING_AREA_IDENTITY_LIST_ONE_PLMN_NON_CONSECUTIVE_TACS: {
          tai_t tai = {0};
          tai.plmn.mcc_digit1 = emm_context->_tai_list.partial_tai_list[k].u.tai_one_plmn_non_consecutive_tacs.mcc_digit1;
          tai.plmn.mcc_digit2 = emm_context->_tai_list.partial_tai_list[k].u.tai_one_plmn_non_consecutive_tacs.mcc_digit2;
          tai.plmn.mcc_digit3 = emm_context->_tai_list.partial_tai_list[k].u.tai_one_plmn_non_consecutive_tacs.mcc_digit3;
          tai.plmn.mnc_digit1 = emm_context->_tai_list.partial_tai_list[k].u.tai_one_plmn_non_consecutive_tacs.mnc_digit1;
          tai.plmn.mnc_digit2 = emm_context->_tai_list.partial_tai_list[k].u.tai_one_plmn_non_consecutive_tacs.mnc_digit2;
          tai.plmn.mnc_digit3 = emm_context->_tai_list.partial_tai_list[k].u.tai_one_plmn_non_consecutive_tacs.mnc_digit3;
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
int
emm_data_context_add (
  emm_data_t * emm_data,
  struct emm_data_context_s *elm)
{
  hashtable_rc_t                          h_rc;

  h_rc = hashtable_ts_insert (emm_data->ctx_coll_ue_id, (const hash_key_t)(elm->ue_id), elm);

  if (HASH_TABLE_OK == h_rc) {
    OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Add in context %p UE id " MME_UE_S1AP_ID_FMT "\n", elm, elm->ue_id);

    if ( IS_EMM_CTXT_PRESENT_GUTI(elm)) {
      h_rc = obj_hashtable_ts_insert (emm_data->ctx_coll_guti, (const void *const)(&elm->_guti), sizeof (elm->_guti),
                                      &elm->ue_id);

      if (HASH_TABLE_OK == h_rc) {
        OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with GUTI "GUTI_FMT"\n", elm->ue_id, GUTI_ARG(&elm->_guti));
      } else {
        OAILOG_ERROR (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with GUTI "GUTI_FMT" Failed %s\n", elm->ue_id, GUTI_ARG(&elm->_guti), hashtable_rc_code2string (h_rc));
        return RETURNerror;
      }
    }
    if ( IS_EMM_CTXT_PRESENT_IMSI(elm)) {
      imsi64_t imsi64 = imsi_to_imsi64(&elm->_imsi);
      h_rc = hashtable_ts_insert (emm_data->ctx_coll_imsi, imsi64, (void*)&elm->ue_id);

      if (HASH_TABLE_OK == h_rc) {
        OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with IMSI "IMSI_64_FMT"\n", elm->ue_id, imsi64);
      } else {
        OAILOG_ERROR (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with IMSI "IMSI_64_FMT" Failed %s\n", elm->ue_id, imsi64, hashtable_rc_code2string (h_rc));
        return RETURNerror;
      }
    }
    return RETURNok;
  } else {
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-CTX - Add in context %p UE id " MME_UE_S1AP_ID_FMT " Failed %s\n", elm, elm->ue_id, hashtable_rc_code2string (h_rc));
    return RETURNerror;
  }
}

//------------------------------------------------------------------------------
int
emm_data_context_add_guti (
  emm_data_t * emm_data,
  struct emm_data_context_s *elm)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;

  if ( IS_EMM_CTXT_PRESENT_GUTI(elm)) {
    h_rc = obj_hashtable_ts_insert (emm_data->ctx_coll_guti, (const void *const)(&elm->_guti), sizeof (elm->_guti),
                                    &elm->ue_id);

    if (HASH_TABLE_OK == h_rc) {
      OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with GUTI "GUTI_FMT"\n", elm->ue_id, GUTI_ARG(&elm->_guti));
    } else {
      OAILOG_ERROR (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with GUTI "GUTI_FMT" Failed %s\n", elm->ue_id, GUTI_ARG(&elm->_guti), hashtable_rc_code2string (h_rc));
      return RETURNerror;
    }
  }
  return RETURNok;
}
//------------------------------------------------------------------------------
int
emm_data_context_add_old_guti (
  emm_data_t * emm_data,
  struct emm_data_context_s *elm)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;

  if ( IS_EMM_CTXT_PRESENT_OLD_GUTI(elm)) {
    h_rc = obj_hashtable_ts_insert (emm_data->ctx_coll_guti, (const void *const)(&elm->_old_guti),
                                    sizeof(elm->_old_guti), &elm->ue_id);

    if (HASH_TABLE_OK == h_rc) {
      OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with old GUTI "GUTI_FMT"\n", elm->ue_id, GUTI_ARG(&elm->_old_guti));
    } else {
      OAILOG_ERROR (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with old GUTI "GUTI_FMT" Failed %s\n", elm->ue_id, GUTI_ARG(&elm->_old_guti), hashtable_rc_code2string (h_rc));
      return RETURNerror;
    }
  }
  return RETURNok;
}

//------------------------------------------------------------------------------

int
emm_data_context_add_imsi (
  emm_data_t * emm_data,
  struct emm_data_context_s *elm) {
  hashtable_rc_t h_rc = HASH_TABLE_OK;

  h_rc = _emm_data_context_hashtable_insert(emm_data, elm);

  if (HASH_TABLE_OK == h_rc) {
    OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with IMSI " IMSI_64_FMT "\n",
                  elm->ue_id, elm->_imsi64);
    return RETURNok;
  } else {
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-CTX - Add in context UE id " MME_UE_S1AP_ID_FMT " with IMSI " IMSI_64_FMT
        " Failed %s\n", elm->ue_id, elm->_imsi64, hashtable_rc_code2string(h_rc));
    return RETURNerror;
  }
}

//------------------------------------------------------------------------------

int
emm_data_context_upsert_imsi (
    emm_data_t * emm_data,
    struct emm_data_context_s *elm)
{
  hashtable_rc_t                          h_rc = HASH_TABLE_OK;

  h_rc = _emm_data_context_hashtable_insert(emm_data, elm);

  if (HASH_TABLE_OK == h_rc || HASH_TABLE_INSERT_OVERWRITTEN_DATA == h_rc) {
    OAILOG_DEBUG (LOG_NAS_EMM, "EMM-CTX - Upsert in context UE id " MME_UE_S1AP_ID_FMT " with IMSI "IMSI_64_FMT"\n",
                  elm->ue_id, elm->_imsi64);
    return RETURNok;
  } else {
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-CTX - Upsert in context UE id " MME_UE_S1AP_ID_FMT " with IMSI "IMSI_64_FMT" "
        "Failed %s\n", elm->ue_id, elm->_imsi64, hashtable_rc_code2string (h_rc));
    return RETURNerror;
  }
}

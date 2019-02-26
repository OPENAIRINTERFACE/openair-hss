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

/*! \file nas_itti_messaging.c
   \brief
   \author  Sebastien ROUX, Lionel GAUTHIER
   \date
   \email: lionel.gauthier@eurecom.fr
*/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"
#include "tree.h"
#include "gcc_diag.h"

#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "dynamic_memory_check.h"
#include "intertask_interface.h"
#include "common_defs.h"
#include "secu_defs.h"
#include "mme_app_ue_context.h"
#include "nas_itti_messaging.h"

#include "nas_emm_proc.h"
#include "esm_proc.h"
#include "mme_app_defs.h"

//------------------------------------------------------------------------------
int
nas_itti_esm_data_ind(
  const mme_ue_s1ap_id_t  ue_id,
  bstring                 esm_msg_p,
  imsi_t                 *imsi,
  tai_t                  *visited_tai)
{
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_EMM, NAS_ESM_DATA_IND);
  NAS_ESM_DATA_IND (message_p).ue_id   = ue_id;
  NAS_ESM_DATA_IND (message_p).req     = esm_msg_p;
  if(imsi)
    memcpy(&NAS_ESM_DATA_IND (message_p).imsi, imsi, sizeof(imsi_t));
  if(visited_tai)
    memcpy(&NAS_ESM_DATA_IND (message_p).visited_tai, visited_tai, sizeof(tai_t));

  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_S1AP_MME, NULL, 0, "0 NAS_ESM_DATA_IND ue id " MME_UE_S1AP_ID_FMT " len %u", ue_id, blength(esm_msg_p));
  // make a long way by MME_APP instead of S1AP to retrieve the sctp_association_id key.
  return itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
nas_itti_esm_detach_ind(
  const mme_ue_s1ap_id_t  ue_id,
  const bool clr)
{
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_EMM, NAS_ESM_DETACH_IND);
  NAS_ESM_DETACH_IND (message_p).ue_id   = ue_id;
  NAS_ESM_DETACH_IND (message_p).clr     = clr;

  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_S1AP_MME, NULL, 0, "0 NAS_ESM_DETACH_IND ue id " MME_UE_S1AP_ID_FMT, ue_id);
  // make a long way by MME_APP instead of S1AP to retrieve the sctp_association_id key.
  return itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
nas_itti_s11_retry_ind(
  const mme_ue_s1ap_id_t  ue_id)
{
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_EMM, NAS_RETRY_BEARER_CTX_PROC_IND);
  NAS_RETRY_BEARER_CTX_PROC_IND (message_p).ue_id   = ue_id;

  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_S1AP_MME, NULL, 0, "0 NAS_RETRY_BEARER_CTX_PROC_IND ue id " MME_UE_S1AP_ID_FMT, ue_id);
  // make a long way by MME_APP instead of S1AP to retrieve the sctp_association_id key.
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
nas_itti_dl_data_req (
  const mme_ue_s1ap_id_t ue_id,
  bstring                nas_msg,
  nas_error_code_t transaction_status
  )
{
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_EMM, NAS_DOWNLINK_DATA_REQ);
  NAS_DOWNLINK_DATA_REQ (message_p).ue_id   = ue_id;
  NAS_DOWNLINK_DATA_REQ (message_p).nas_msg = nas_msg;
  nas_msg = NULL;
  NAS_DOWNLINK_DATA_REQ (message_p).transaction_status = transaction_status;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_S1AP_MME, NULL, 0, "0 NAS_DOWNLINK_DATA_REQ ue id " MME_UE_S1AP_ID_FMT " len %u", ue_id, blength(nas_msg));
  // make a long way by MME_APP instead of S1AP to retrieve the sctp_association_id key.
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
nas_itti_erab_setup_req (const mme_ue_s1ap_id_t ue_id,
    const ebi_t ebi,
    const bitrate_t        mbr_dl,
    const bitrate_t        mbr_ul,
    const bitrate_t        gbr_dl,
    const bitrate_t        gbr_ul,
    bstring                nas_msg)
{
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_ESM, NAS_ERAB_SETUP_REQ);
  NAS_ERAB_SETUP_REQ (message_p).ue_id   = ue_id;
  NAS_ERAB_SETUP_REQ (message_p).ebi     = ebi;
  NAS_ERAB_SETUP_REQ (message_p).mbr_dl  = mbr_dl;
  NAS_ERAB_SETUP_REQ (message_p).mbr_ul  = mbr_ul;
  NAS_ERAB_SETUP_REQ (message_p).gbr_dl  = gbr_dl;
  NAS_ERAB_SETUP_REQ (message_p).gbr_ul  = gbr_ul;
  NAS_ERAB_SETUP_REQ (message_p).nas_msg = nas_msg;
  nas_msg = NULL;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_MMEAPP_MME, NULL, 0, "0 NAS_ERAB_SETUP_REQ ue id " MME_UE_S1AP_ID_FMT " ebi %u len %u", ue_id, ebi, blength(NAS_ERAB_SETUP_REQ (message_p).nas_msg));
  // make a long way by MME_APP instead of S1AP to retrieve the sctp_association_id key.
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
nas_itti_erab_modify_req (const mme_ue_s1ap_id_t ue_id,
    const ebi_t ebi,
    const bitrate_t        mbr_dl,
    const bitrate_t        mbr_ul,
    const bitrate_t        gbr_dl,
    const bitrate_t        gbr_ul,
    bstring                nas_msg)
{
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_ESM, NAS_ERAB_MODIFY_REQ);
  NAS_ERAB_MODIFY_REQ (message_p).ue_id   = ue_id;
  NAS_ERAB_MODIFY_REQ (message_p).ebi     = ebi;
  NAS_ERAB_MODIFY_REQ (message_p).mbr_dl  = mbr_dl;
  NAS_ERAB_MODIFY_REQ (message_p).mbr_ul  = mbr_ul;
  NAS_ERAB_MODIFY_REQ (message_p).gbr_dl  = gbr_dl;
  NAS_ERAB_MODIFY_REQ (message_p).gbr_ul  = gbr_ul;
  NAS_ERAB_MODIFY_REQ (message_p).nas_msg = nas_msg;
  nas_msg = NULL;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_MMEAPP_MME, NULL, 0, "0 NAS_ERAB_MODIFY_REQ ue id " MME_UE_S1AP_ID_FMT " ebi %u len %u", ue_id, ebi, blength(NAS_ERAB_MODIFY_REQ (message_p).nas_msg));
  // make a long way by MME_APP instead of S1AP to retrieve the sctp_association_id key.
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
nas_itti_erab_release_req (const mme_ue_s1ap_id_t ue_id,
    const ebi_t ebi,
    bstring                nas_msg)
{
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_ESM, NAS_ERAB_RELEASE_REQ);
  NAS_ERAB_RELEASE_REQ (message_p).ue_id   = ue_id;
  NAS_ERAB_RELEASE_REQ (message_p).ebi     = ebi;
  NAS_ERAB_RELEASE_REQ (message_p).nas_msg = nas_msg;
  nas_msg = NULL;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_MMEAPP_MME, NULL, 0, "0 NAS_ERAB_RELEASE_REQ ue id " MME_UE_S1AP_ID_FMT " ebi %u len %u", ue_id, ebi, blength(NAS_ERAB_SETUP_REQ (message_p).nas_msg));
  // make a long way by MME_APP instead of S1AP to retrieve the sctp_association_id key.
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
void nas_itti_activate_eps_bearer_ctx_cnf(
    const mme_ue_s1ap_id_t ue_idP,
    const ebi_t            ebi,
    const teid_t           saegw_s1u_teid)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_ESM, NAS_ACTIVATE_EPS_BEARER_CTX_CNF);
  NAS_ACTIVATE_EPS_BEARER_CTX_CNF (message_p).ue_id   = ue_idP;
  NAS_ACTIVATE_EPS_BEARER_CTX_CNF (message_p).ebi     = ebi;
  NAS_ACTIVATE_EPS_BEARER_CTX_CNF (message_p).saegw_s1u_teid  = saegw_s1u_teid;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_MMEAPP_MME, NULL, 0, "0 NAS_ACTIVATE_EPS_BEARER_CTX_CNF ue id " MME_UE_S1AP_ID_FMT, ue_idP);
  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_NAS);
}

//------------------------------------------------------------------------------
void nas_itti_activate_eps_bearer_ctx_rej(
    const mme_ue_s1ap_id_t ue_idP,
    const teid_t           saegw_s1u_teid,
    const esm_cause_t      esm_cause)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_ESM, NAS_ACTIVATE_EPS_BEARER_CTX_REJ);
  NAS_ACTIVATE_EPS_BEARER_CTX_REJ (message_p).ue_id           = ue_idP;
  NAS_ACTIVATE_EPS_BEARER_CTX_REJ (message_p).saegw_s1u_teid  = saegw_s1u_teid;
  switch(esm_cause){
  case ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION:
    NAS_ACTIVATE_EPS_BEARER_CTX_REJ (message_p).cause_value = SEMANTIC_ERROR_IN_TFT;
    break;
  case ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION:
    NAS_ACTIVATE_EPS_BEARER_CTX_REJ (message_p).cause_value = SYNTACTIC_ERROR_IN_TFT;
    break;
  case ESM_CAUSE_SEMANTIC_ERRORS_IN_PACKET_FILTER:
    NAS_ACTIVATE_EPS_BEARER_CTX_REJ (message_p).cause_value = SEMANTIC_ERRORS_IN_PF;
    break;
  case ESM_CAUSE_SYNTACTICAL_ERROR_IN_PACKET_FILTER:
    NAS_ACTIVATE_EPS_BEARER_CTX_REJ (message_p).cause_value = SYNTACTIC_ERRORS_IN_PF;
    break;
  case ESM_CAUSE_EPS_QOS_NOT_ACCEPTED:
    NAS_ACTIVATE_EPS_BEARER_CTX_REJ (message_p).cause_value = MANDATORY_IE_INCORRECT;
    break;
  default:
    NAS_ACTIVATE_EPS_BEARER_CTX_REJ (message_p).cause_value = REQUEST_REJECTED;
    break;
  }
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_MMEAPP_MME, NULL, 0, "0 NAS_ACTIVATE_EPS_BEARER_CTX_REJ ue id " MME_UE_S1AP_ID_FMT, ue_idP);
  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_NAS);
}

//------------------------------------------------------------------------------
void nas_itti_modify_eps_bearer_ctx_cnf(
    const mme_ue_s1ap_id_t ue_idP,
    const ebi_t            ebi)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_ESM, NAS_MODIFY_EPS_BEARER_CTX_CNF);
  NAS_MODIFY_EPS_BEARER_CTX_CNF (message_p).ue_id   = ue_idP;
  NAS_MODIFY_EPS_BEARER_CTX_CNF (message_p).ebi     = ebi;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_MMEAPP_MME, NULL, 0, "0 NAS_MODIFY_EPS_BEARER_CTX_CNF ue id " MME_UE_S1AP_ID_FMT, ue_idP);
  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_NAS);
}

//------------------------------------------------------------------------------
void nas_itti_modify_eps_bearer_ctx_rej(
    const mme_ue_s1ap_id_t ue_idP,
    const ebi_t            ebi,
    const esm_cause_t      esm_cause)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_ESM, NAS_MODIFY_EPS_BEARER_CTX_REJ);
  NAS_MODIFY_EPS_BEARER_CTX_REJ (message_p).ue_id   = ue_idP;
  NAS_MODIFY_EPS_BEARER_CTX_REJ (message_p).ebi     = ebi;
  switch(esm_cause){
  case ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION:
    NAS_MODIFY_EPS_BEARER_CTX_REJ (message_p).cause_value = SEMANTIC_ERROR_IN_TFT;
    break;
  case ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION:
    NAS_MODIFY_EPS_BEARER_CTX_REJ (message_p).cause_value = SYNTACTIC_ERROR_IN_TFT;
    break;
  case ESM_CAUSE_SEMANTIC_ERRORS_IN_PACKET_FILTER:
    NAS_MODIFY_EPS_BEARER_CTX_REJ (message_p).cause_value = SEMANTIC_ERRORS_IN_PF;
    break;
  case ESM_CAUSE_SYNTACTICAL_ERROR_IN_PACKET_FILTER:
    NAS_MODIFY_EPS_BEARER_CTX_REJ (message_p).cause_value = SYNTACTIC_ERRORS_IN_PF;
    break;
  case ESM_CAUSE_EPS_QOS_NOT_ACCEPTED:
    NAS_MODIFY_EPS_BEARER_CTX_REJ (message_p).cause_value = MANDATORY_IE_INCORRECT;
    break;
  case ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY:
    NAS_MODIFY_EPS_BEARER_CTX_REJ (message_p).cause_value = NO_RESOURCES_AVAILABLE;
    break;
  default:
    NAS_MODIFY_EPS_BEARER_CTX_REJ (message_p).cause_value = REQUEST_REJECTED;
    break;
  }
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_MMEAPP_MME, NULL, 0, "0 NAS_MODIFY_EPS_BEARER_CTX_REJ ue id " MME_UE_S1AP_ID_FMT, ue_idP);
  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_NAS);
}

//------------------------------------------------------------------------------
void nas_itti_dedicated_eps_bearer_deactivation_complete(
    const mme_ue_s1ap_id_t ue_idP,
    const ebi_t ded_ebi)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_ESM, NAS_DEACTIVATE_EPS_BEARER_CTX_CNF);
  NAS_DEACTIVATE_EPS_BEARER_CTX_CNF (message_p).ue_id     = ue_idP;
  NAS_DEACTIVATE_EPS_BEARER_CTX_CNF (message_p).ded_ebi   = ded_ebi;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_MMEAPP_MME, NULL, 0, "0 NAS_DEACTIVATE_EPS_BEARER_CTX_CNF ue id " MME_UE_S1AP_ID_FMT " ebi %u", ue_idP, ded_ebi);
  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_NAS);
}

//------------------------------------------------------------------------------
void nas_itti_pdn_config_req(
  unsigned int            ue_idP,
  const imsi_t           *const imsi_pP,
  esm_proc_pdn_request_t  request_type,
  plmn_t                 *visited_plmn)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef *message_p = NULL;
  AssertFatal(imsi_pP       != NULL, "imsi_pP param is NULL");

  message_p = itti_alloc_new_message (TASK_NAS_ESM, S6A_UPDATE_LOCATION_REQ);

  if (message_p == NULL) {
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  s6a_update_location_req_t *s6a_ulr_p = &message_p->ittiMsg.s6a_update_location_req;

  IMSI_TO_STRING(imsi_pP, s6a_ulr_p->imsi, IMSI_BCD_DIGITS_MAX+1);
  s6a_ulr_p->imsi_length = strlen (s6a_ulr_p->imsi);
  s6a_ulr_p->initial_attach = request_type;

  memcpy (&s6a_ulr_p->visited_plmn, visited_plmn, sizeof (plmn_t));
  s6a_ulr_p->rat_type = RAT_EUTRAN;
  /*
   * Check if we already have UE data
   * todo: remove this?
   */
  s6a_ulr_p->skip_subscriber_data = 0;
  MSC_LOG_TX_MESSAGE (MSC_NAS_ESM, MSC_S6A_MME, NULL, 0, "0 S6A_UPDATE_LOCATION_REQ imsi %s", s6a_ulr_p->imsi);
  int rc =  itti_send_msg_to_task (TASK_S6A, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
void nas_itti_pdn_disconnect_req(
  mme_ue_s1ap_id_t        ue_idP,
  ebi_t                   default_ebi,
  pti_t                   pti,
  bool                    deleteTunnel,
  bool                    handover,
  struct in_addr          saegw_s11_addr, /**< Put them into the UE context ? */
  teid_t                  saegw_s11_teid,
  pdn_cid_t               pdn_cid)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef *message_p = NULL;

  message_p = itti_alloc_new_message(TASK_NAS_ESM, NAS_PDN_DISCONNECT_REQ);

  NAS_PDN_DISCONNECT_REQ(message_p).pdn_cid         = pdn_cid;
  NAS_PDN_DISCONNECT_REQ(message_p).pti             = pti;
  NAS_PDN_DISCONNECT_REQ(message_p).default_ebi     = default_ebi;
  NAS_PDN_DISCONNECT_REQ(message_p).handover		= handover;
  NAS_PDN_DISCONNECT_REQ(message_p).ue_id           = ue_idP;
  NAS_PDN_DISCONNECT_REQ(message_p).noDelete        = ((pti != PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED) || !deleteTunnel);
  NAS_PDN_DISCONNECT_REQ(message_p).saegw_s11_ip_addr    = saegw_s11_addr;
  NAS_PDN_DISCONNECT_REQ(message_p).saegw_s11_teid       = saegw_s11_teid;

  if(saegw_s11_addr.s_addr == 0){
    OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC  - No SAE-GW IP for UE " MME_UE_S1AP_ID_FMT " to send PDN-Disc Req. \n", ue_idP);
  }
  MSC_LOG_TX_MESSAGE(
        MSC_NAS_MME,
        MSC_MMEAPP_MME,
        NULL,0,
        "NAS_PDN_DISCONNECT_REQ ue id " MME_UE_S1AP_ID_FMT,
        ue_idP);

  itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_OUT(LOG_NAS);
}

//------------------------------------------------------------------------------
void nas_itti_ctx_req(
  const uint32_t        ue_idP,
  const guti_t        * const guti_p,
  tai_t         * const new_taiP,
  tai_t         * const last_visited_taiP,
  bstring               request_msg)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef                             *message_p = NULL;
  itti_nas_context_req_t                 *nas_context_req_p = NULL;

  message_p = itti_alloc_new_message (TASK_NAS_EMM, NAS_CONTEXT_REQ);
  nas_context_req_p = &message_p->ittiMsg.s10_context_request;
  memset(nas_context_req_p, 0, sizeof(itti_nas_context_req_t));

  /** We may not have an IMSI at all. */
  /* GUTI. */
  memcpy((void*)&nas_context_req_p->old_guti, (void*)guti_p, sizeof(guti_t));
  /* RAT-Type. */
  nas_context_req_p->rat_type = RAT_EUTRAN;
  /** UE_ID. */
  nas_context_req_p->ue_id = ue_idP;
  /** Complete Request Message. */
  nas_context_req_p->nas_msg = bstrcpy(request_msg);
  /** Originating TAI. */
  nas_context_req_p->originating_tai = *last_visited_taiP;

  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_S10_MME, NULL, 0, "0 S10_CTX_REQ GUTI "GUTI_FMT" originating TAI " TAI_FMT,
      GUTI_ARG(&nas_context_req_p->old_guti), TAI_FMT(last_visited_taiP));
  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p); /**< Send it to MME_APP since we already may have one. */

  /** No NAS S10 context response timer needs to be started. It will be started in the GTPv2c stack. */
  OAILOG_FUNC_OUT(LOG_NAS);
}

//------------------------------------------------------------------------------
void nas_itti_auth_info_req(
  const mme_ue_s1ap_id_t ue_idP,
  const imsi_t   * const imsiP,
  const bool             is_initial_reqP,
  plmn_t         * const visited_plmnP,
  const uint8_t          num_vectorsP,
  const_bstring const auts_pP)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef                             *message_p = NULL;
  s6a_auth_info_req_t                    *auth_info_req = NULL;

  message_p = itti_alloc_new_message (TASK_NAS_EMM, S6A_AUTH_INFO_REQ);
  auth_info_req = &message_p->ittiMsg.s6a_auth_info_req;

  IMSI_TO_STRING(imsiP,auth_info_req->imsi, IMSI_BCD_DIGITS_MAX+1);
  auth_info_req->imsi_length = strlen(auth_info_req->imsi);

  AssertFatal((15 >= auth_info_req->imsi_length) && (0 < auth_info_req->imsi_length),
      "Bad IMSI length %d", auth_info_req->imsi_length);

  auth_info_req->visited_plmn  = *visited_plmnP;
  auth_info_req->nb_of_vectors = num_vectorsP;

  if (is_initial_reqP ) {
    auth_info_req->re_synchronization = 0;
    memset (auth_info_req->auts, 0, sizeof auth_info_req->auts);
  } else {
    AssertFatal(auts_pP != NULL, "Autn Null during resynchronization");
    auth_info_req->re_synchronization = 1;
    memcpy (auth_info_req->auts, auts_pP->data, blength(auts_pP));
  }

  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_S6A_MME, NULL, 0, "0 S6A_AUTH_INFO_REQ IMSI "IMSI_64_FMT" visited_plmn "PLMN_FMT" re_sync %u",
      auth_info_req->imsi, PLMN_ARG(visited_plmnP), auth_info_req->re_synchronization);
  itti_send_msg_to_task (TASK_S6A, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_OUT(LOG_NAS);
}

//------------------------------------------------------------------------------
void nas_itti_s11_bearer_resource_cmd (
  const pti_t            pti,
  const ebi_t            linked_ebi,
  const teid_t           local_teid,
  const teid_t           peer_teid,
  const struct in_addr  *saegw_s11_ipv4,
  const ebi_t            ebi,
  const traffic_flow_template_t * const tad,
  const flow_qos_t              * const flow_qos)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef                             *message_p = NULL;
  itti_s11_bearer_resource_command_t     *bearer_resource_cmd = NULL;

  message_p = itti_alloc_new_message (TASK_NAS_EMM, S11_BEARER_RESOURCE_COMMAND);
  bearer_resource_cmd = &message_p->ittiMsg.s11_bearer_resource_command;
  bearer_resource_cmd->pti = pti;
  bearer_resource_cmd->linked_ebi = linked_ebi;

  bearer_resource_cmd->ebi = ebi;
  copy_traffic_flow_template(&bearer_resource_cmd->tad, tad);

  if(flow_qos && flow_qos->qci){
    memcpy(&bearer_resource_cmd->flow_qos, flow_qos, sizeof(flow_qos_t));
  }

  bearer_resource_cmd->local_teid = local_teid;
  bearer_resource_cmd->teid       = peer_teid;
  bearer_resource_cmd->peer_ip.s_addr = saegw_s11_ipv4->s_addr;

  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_S6A_MME, NULL, 0, "0 S11_BEARER_RESOURCE_CMD LOCAL_TEID "TEID_FMT" PEER_TEID "TEID_FMT" ", local_teid, peer_teid);
  itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_OUT(LOG_NAS);
}

//------------------------------------------------------------------------------
void nas_itti_establish_cnf(
  const mme_ue_s1ap_id_t ue_idP,
  const nas_error_code_t error_codeP,
  bstring                msgP,
  const uint32_t         nas_count,
  const uint16_t         selected_encryption_algorithmP,
  const uint16_t         selected_integrity_algorithmP)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef                             *message_p        = NULL;
  emm_data_context_t                     *emm_ctx = NULL;

  emm_ctx = emm_data_context_get(&_emm_data, ue_idP);

  message_p = itti_alloc_new_message(TASK_NAS_EMM, NAS_CONNECTION_ESTABLISHMENT_CNF);

  NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).ue_id                           = ue_idP;
  NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).err_code                        = error_codeP;
  NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).nas_msg                         = msgP; msgP = NULL;

  // According to 3GPP 9.2.1.40, the UE security capabilities are 16-bit
  // strings, EEA0 is inherently supported, so its support is not tracked in
  // the bit string. However, emm_ctx->eea is an 8-bit string with the highest
  // order bit representing EEA0 support, so we need to trim it. The same goes
  // for integrity.
  //
  // TODO: change the way the EEA and EIA are translated into the packets.
  //       Currently, the 16-bit string is 8-bit rotated to produce the string
  //       sent in the packets, which is why we're using bits 8-10 to
  //       represent EEA1/2/3 (and EIA1/2/3) support here.
  NAS_CONNECTION_ESTABLISHMENT_CNF(message_p)
  .encryption_algorithm_capabilities =
      ((uint16_t)emm_ctx->_ue_network_capability.eea & ~(1 << 7)) << 1;
  NAS_CONNECTION_ESTABLISHMENT_CNF(message_p)
  .integrity_algorithm_capabilities =
      ((uint16_t)emm_ctx->_ue_network_capability.eia & ~(1 << 7)) << 1;

  AssertFatal((0 <= emm_ctx->_security.vector_index) && (MAX_EPS_AUTH_VECTORS > emm_ctx->_security.vector_index),
      "Invalid vector index %d", emm_ctx->_security.vector_index);

  OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC  - KeNB with UL Count %d for UE " MME_UE_S1AP_ID_FMT ". \n", nas_count, emm_ctx->ue_id);

  derive_keNB (emm_ctx->_vector[emm_ctx->_security.vector_index].kasme,
      nas_count, NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).kenb);

  uint8_t                                 zero[32];
  memset(zero, 0, 32);
  memset(emm_ctx->_vector[emm_ctx->_security.vector_index].nh_conj, 0, 32);

  OAILOG_STREAM_HEX(OAILOG_LEVEL_DEBUG, LOG_NAS, "KENB: ", NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).kenb, 32)

  /** Derive the next hop. */
  /** Calculate and set the next hop value into the context. Use the above derived kenb as first nh value. */
  if(memcmp(emm_ctx->_vector[emm_ctx->_security.vector_index].nh_conj, zero, 32) == 0){
    OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC  - NH value is 0 as expected. Setting kEnb as NH0  \n");
    memcpy(emm_ctx->_vector[emm_ctx->_security.vector_index].nh_conj, NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).kenb, 32);
  }else{
    OAILOG_WARNING( LOG_NAS_EMM, "EMM-PROC  - NH value is NOT 0 @ initial S1AP context establishment  \n");
  }

  OAILOG_STREAM_HEX(OAILOG_LEVEL_DEBUG, LOG_NAS, "New NH_CONJ of emmCtx: ", emm_ctx->_vector[emm_ctx->_security.vector_index].nh_conj, 32);


  /** Reset the next chaining hop counter.
   * TS. 33.401:
   * Upon receipt of the NAS Service Request message, the MME shall derive key KeNB as specified in Annex A.3 using the
    NAS COUNT [9] corresponding to the NAS Service Request and initialize the value of the Next hop Chaining Counter
    (NCC) to one. The MME shall further derive a next hop parameter NH as specified in Annex A.4 using the newly
    derived KeNB as basis for the derivation. This fresh {NH, NCC=1} pair shall be stored in the MME and shall be used for
    the next forward security key derivation. The MME shall communicate the {KeNB, NCC=0} pair and KSIASME to the
    serving eNB in the S1-AP procedure INITIAL CONTEXT SETUP.
   */
  emm_ctx->_security.ncc = 0;

  MSC_LOG_TX_MESSAGE(
      MSC_NAS_MME,
      MSC_MMEAPP_MME,
      NULL,0,
      "NAS_CONNECTION_ESTABLISHMENT_CNF ue id " MME_UE_S1AP_ID_FMT " len %u sea %x sia %x ",
      ue_idP, blength(msgP), selected_encryption_algorithmP, selected_integrity_algorithmP);

  itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_OUT(LOG_NAS);
}

//------------------------------------------------------------------------------
void nas_itti_detach_req(const mme_ue_s1ap_id_t ue_id)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef *message_p;

  OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - Sending NAS_ITTI_DETACH_REQ for UE " MME_UE_S1AP_ID_FMT ". \n", ue_id);

  message_p = itti_alloc_new_message(TASK_NAS_EMM, NAS_DETACH_REQ);

  NAS_DETACH_REQ(message_p).ue_id = ue_id;

  MSC_LOG_TX_MESSAGE(
                MSC_NAS_MME,
                MSC_MMEAPP_MME,
                NULL,0,
                "0 NAS_DETACH_REQ ue id " MME_UE_S1AP_ID_FMT, ue_id);

  itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_NAS);
}

//***************************************************************************
void  s6a_auth_info_rsp_timer_expiry_handler (void *args)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  mme_ue_s1ap_id_t ue_id = (mme_ue_s1ap_id_t) (args);

  emm_data_context_t * emm_ctx = emm_data_context_get(&_emm_data, ue_id);

  if (emm_ctx) {

    nas_auth_info_proc_t * auth_info_proc = get_nas_cn_procedure_auth_info(emm_ctx);
    if (!auth_info_proc) {
      OAILOG_FUNC_OUT (LOG_NAS_EMM);
    }

    void * timer_callback_args = NULL;
    nas_stop_Ts6a_auth_info(auth_info_proc->ue_id, &auth_info_proc->timer_s6a, timer_callback_args);

    auth_info_proc->timer_s6a.id = NAS_TIMER_INACTIVE_ID;
    if (auth_info_proc->resync) {
      OAILOG_ERROR (LOG_NAS_EMM,
          "EMM-PROC  - Timer timer_s6_auth_info_rsp expired. Resync auth procedure was in progress. Aborting attach procedure. UE id " MME_UE_S1AP_ID_FMT "\n",
          auth_info_proc->ue_id);
    } else {

      OAILOG_ERROR (LOG_NAS_EMM,
          "EMM-PROC  - Timer timer_s6_auth_info_rsp expired. Initial auth procedure was in progress. Aborting attach procedure. UE id " MME_UE_S1AP_ID_FMT "\n",
          auth_info_proc->ue_id);
    }

    // Send Attach Reject with cause NETWORK FAILURE and delete UE context
    nas_proc_auth_param_fail (auth_info_proc->ue_id, NAS_CAUSE_NETWORK_FAILURE);
  } else {
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - Timer timer_s6_auth_info_rsp expired. Null EMM Context for UE " MME_UE_S1AP_ID_FMT". \n", ue_id);
  }

  OAILOG_FUNC_OUT (LOG_NAS_EMM);
}

//***************************************************************************
void  s10_context_req_timer_expiry_handler (void *args)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_data_context_t  *emm_ctx = (emm_data_context_t *) (args);
  if (emm_ctx) {

    nas_ctx_req_proc_t * ctx_req_proc = get_nas_cn_procedure_ctx_req(emm_ctx);
    if (!ctx_req_proc) {
      OAILOG_FUNC_OUT (LOG_NAS_EMM);
    }

//    void * timer_callback_args = NULL;
//    nas_stop_Ts10_ctx_res(ctx_req_proc->ue_id, &ctx_req_proc->timer_s10, timer_callback_args);

//    ctx_req_proc->timer_s10.id = NAS_TIMER_INACTIVE_ID;
      
    /*
     * Notify the failed context request procedure.
     * The respective specific procedure then will take action.
     */
    nas_proc_context_fail(emm_ctx->ue_id, NAS_CAUSE_NETWORK_FAILURE);
  } else { 
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - Timer timer_s10_context_req expired. Null EMM Context for UE \n");
  }

  OAILOG_FUNC_OUT (LOG_NAS_EMM);
}
//***************************************************************************

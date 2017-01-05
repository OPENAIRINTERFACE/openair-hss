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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"
#include "tree.h"

#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "common_defs.h"
#include "secu_defs.h"
#include "mme_app_ue_context.h"
#include "esm_proc.h"
#include "nas_itti_messaging.h"
#include "mme_app_defs.h"


#define TASK_ORIGIN  TASK_NAS_MME

//------------------------------------------------------------------------------
int
nas_itti_dl_data_req (
  const mme_ue_s1ap_id_t ue_id,
  bstring                nas_msg)
{
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_MME, NAS_DOWNLINK_DATA_REQ);
  NAS_DL_DATA_REQ (message_p).enb_ue_s1ap_id = INVALID_ENB_UE_S1AP_ID;
  NAS_DL_DATA_REQ (message_p).ue_id   = ue_id;
  NAS_DL_DATA_REQ (message_p).nas_msg = nas_msg;
  nas_msg = NULL;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_MMEAPP_MME, NULL, 0, "0 NAS_DOWNLINK_DATA_REQ ue id " MME_UE_S1AP_ID_FMT " len %u", ue_id, blength(nas_msg));
  // make a long way by MME_APP instead of S1AP to retrieve the sctp_association_id key.
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
nas_itti_erab_setup_req (const mme_ue_s1ap_id_t ue_id,
    const ebi_t ebi,
    bstring                nas_msg)
{
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_MME, NAS_ERAB_SETUP_REQ);
  NAS_ERAB_SETUP_REQ (message_p).ue_id   = ue_id;
  NAS_ERAB_SETUP_REQ (message_p).ebi     = ebi;
  NAS_ERAB_SETUP_REQ (message_p).nas_msg = nas_msg;
  nas_msg = NULL;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_MMEAPP_MME, NULL, 0, "0 NAS_ERAB_SETUP_REQ ue id " MME_UE_S1AP_ID_FMT " ebi %u len %u", ue_id, ebi, blength(NAS_ERAB_SETUP_REQ (message_p).nas_msg));
  // make a long way by MME_APP instead of S1AP to retrieve the sctp_association_id key.
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
void nas_itti_pdn_config_req(
  int                     ptiP,
  unsigned int            ue_idP,
  const imsi_t           *const imsi_pP,
  esm_proc_data_t        *proc_data_pP,
  esm_proc_pdn_request_t  request_typeP)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef *message_p = NULL;

  AssertFatal(imsi_pP       != NULL, "imsi_pP param is NULL");
  AssertFatal(proc_data_pP  != NULL, "proc_data_pP param is NULL");


  message_p = itti_alloc_new_message(TASK_NAS_MME, NAS_PDN_CONFIG_REQ);

  hexa_to_ascii((uint8_t *)imsi_pP->u.value,
      NAS_PDN_CONFIG_REQ(message_p).imsi,
      imsi_pP->length);
  NAS_PDN_CONFIG_REQ(message_p).imsi_length = imsi_pP->length;

  NAS_PDN_CONFIG_REQ(message_p).ue_id           = ue_idP;


  bassign(NAS_PDN_CONFIG_REQ(message_p).apn, proc_data_pP->apn);
  bassign(NAS_PDN_CONFIG_REQ(message_p).pdn_addr, proc_data_pP->pdn_addr);

  switch (proc_data_pP->pdn_type) {
  case ESM_PDN_TYPE_IPV4:
    NAS_PDN_CONFIG_REQ(message_p).pdn_type = IPv4;
    break;

  case ESM_PDN_TYPE_IPV6:
    NAS_PDN_CONFIG_REQ(message_p).pdn_type = IPv6;
    break;

  case ESM_PDN_TYPE_IPV4V6:
    NAS_PDN_CONFIG_REQ(message_p).pdn_type = IPv4_AND_v6;
    break;

  default:
    NAS_PDN_CONFIG_REQ(message_p).pdn_type = IPv4;
    break;
  }

  NAS_PDN_CONFIG_REQ(message_p).request_type  = request_typeP;


  MSC_LOG_TX_MESSAGE(
        MSC_NAS_MME,
        MSC_MMEAPP_MME,
        NULL,0,
        "NAS_PDN_CONFIG_REQ ue id %06"PRIX32" IMSI %X",
        ue_idP, NAS_PDN_CONFIG_REQ(message_p).imsi);

  itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_NAS);
}


//------------------------------------------------------------------------------
void nas_itti_pdn_connectivity_req(
  int                     ptiP,
  mme_ue_s1ap_id_t        ue_idP,
  pdn_cid_t               pdn_cidP,
  const imsi_t           *const imsi_pP,
  esm_proc_data_t        *proc_data_pP,
  esm_proc_pdn_request_t  request_typeP)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef *message_p = NULL;

  AssertFatal(imsi_pP       != NULL, "imsi_pP param is NULL");
  AssertFatal(proc_data_pP  != NULL, "proc_data_pP param is NULL");


  message_p = itti_alloc_new_message(TASK_NAS_MME, NAS_PDN_CONNECTIVITY_REQ);

  hexa_to_ascii((uint8_t *)imsi_pP->u.value,
                NAS_PDN_CONNECTIVITY_REQ(message_p).imsi,
                8);

  NAS_PDN_CONNECTIVITY_REQ(message_p).pdn_cid         = pdn_cidP;
  NAS_PDN_CONNECTIVITY_REQ(message_p).pti             = ptiP;
  NAS_PDN_CONNECTIVITY_REQ(message_p).ue_id           = ue_idP;
  NAS_PDN_CONNECTIVITY_REQ(message_p).imsi[15]        = '\0';

  if (isdigit(NAS_PDN_CONNECTIVITY_REQ(message_p).imsi[14])) {
    NAS_PDN_CONNECTIVITY_REQ(message_p).imsi_length = 15;
  } else {
    NAS_PDN_CONNECTIVITY_REQ(message_p).imsi_length = 14;
    NAS_PDN_CONNECTIVITY_REQ(message_p).imsi[14] = '\0';
  }

  bassign(NAS_PDN_CONNECTIVITY_REQ(message_p).apn, proc_data_pP->apn);
  bassign(NAS_PDN_CONNECTIVITY_REQ(message_p).pdn_addr, proc_data_pP->pdn_addr);

  switch (proc_data_pP->pdn_type) {
  case ESM_PDN_TYPE_IPV4:
    NAS_PDN_CONNECTIVITY_REQ(message_p).pdn_type = IPv4;
    break;

  case ESM_PDN_TYPE_IPV6:
    NAS_PDN_CONNECTIVITY_REQ(message_p).pdn_type = IPv6;
    break;

  case ESM_PDN_TYPE_IPV4V6:
    NAS_PDN_CONNECTIVITY_REQ(message_p).pdn_type = IPv4_AND_v6;
    break;

  default:
    NAS_PDN_CONNECTIVITY_REQ(message_p).pdn_type = IPv4;
    break;
  }

  // not efficient but be careful about "typedef network_qos_t esm_proc_qos_t;"
  memcpy(&NAS_PDN_CONNECTIVITY_REQ(message_p).bearer_qos, &proc_data_pP->bearer_qos, sizeof (proc_data_pP->bearer_qos));

  NAS_PDN_CONNECTIVITY_REQ(message_p).request_type  = request_typeP;

  copy_protocol_configuration_options (&NAS_PDN_CONNECTIVITY_REQ(message_p).pco, &proc_data_pP->pco);

  MSC_LOG_TX_MESSAGE(
        MSC_NAS_MME,
        MSC_MMEAPP_MME,
        NULL,0,
        "NAS_PDN_CONNECTIVITY_REQ ue id %06"PRIX32" IMSI %X",
        ue_idP, NAS_PDN_CONNECTIVITY_REQ(message_p).imsi);

  itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_NAS);
}

//------------------------------------------------------------------------------
void nas_itti_auth_info_req(
  const mme_ue_s1ap_id_t ue_idP,
  const imsi64_t         imsi64_P,
  const bool             is_initial_reqP,
  plmn_t         * const visited_plmnP,
  const uint8_t          num_vectorsP,
  const_bstring const auts_pP)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef                             *message_p = NULL;
  s6a_auth_info_req_t                    *auth_info_req = NULL;


  message_p = itti_alloc_new_message (TASK_NAS_MME, S6A_AUTH_INFO_REQ);
  auth_info_req = &message_p->ittiMsg.s6a_auth_info_req;

  auth_info_req->imsi_length =
      snprintf (auth_info_req->imsi, IMSI_BCD_DIGITS_MAX+1, IMSI_64_FMT, imsi64_P);

  AssertFatal((15 >= auth_info_req->imsi_length) && (0 < auth_info_req->imsi_length),
      "Bad IMSI length %d", auth_info_req->imsi_length);

  auth_info_req->visited_plmn  = *visited_plmnP;
  auth_info_req->nb_of_vectors = num_vectorsP;

  if (auts_pP ) {
    auth_info_req->re_synchronization = 1;
    memcpy (auth_info_req->auts, auts_pP->data, sizeof (auth_info_req->auts));
  } else {
    auth_info_req->re_synchronization = 0;
    memset (auth_info_req->auts, 0, sizeof (auth_info_req->auts));
  }

  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_S6A_MME, NULL, 0, "0 S6A_AUTH_INFO_REQ IMSI "IMSI_64_FMT" visited_plmn "PLMN_FMT" re_sync %u",
      imsi64_P, PLMN_ARG(visited_plmnP), auth_info_req->re_synchronization);
  itti_send_msg_to_task (TASK_S6A, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_OUT(LOG_NAS);
}

//------------------------------------------------------------------------------
void nas_itti_establish_rej(
  const mme_ue_s1ap_id_t  ue_idP,
  const imsi_t     *const imsi_pP
  , uint8_t               initial_reqP)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef *message_p;

  message_p = itti_alloc_new_message(TASK_NAS_MME, NAS_AUTHENTICATION_PARAM_REQ);

  hexa_to_ascii((uint8_t *)imsi_pP->u.value,
                NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi, 8);

  NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi[15] = '\0';

  if (isdigit(NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi[14])) {
    NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi_length = 15;
  } else {
    NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi_length = 14;
    NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi[14]    = '\0';
  }

  NAS_AUTHENTICATION_PARAM_REQ(message_p).initial_req = initial_reqP;
  NAS_AUTHENTICATION_PARAM_REQ(message_p).ue_id       = ue_idP;

  MSC_LOG_TX_MESSAGE(
        MSC_NAS_MME,
        MSC_MMEAPP_MME,
        NULL,0,
        "NAS_AUTHENTICATION_PARAM_REQ ue id %06"PRIX32" IMSI %s (establish reject)",
        ue_idP, NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi);

  itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_NAS);
}

//------------------------------------------------------------------------------
void nas_itti_establish_cnf(
  const mme_ue_s1ap_id_t ue_idP,
  const nas_error_code_t error_codeP,
  bstring                msgP,
  const uint16_t         selected_encryption_algorithmP,
  const uint16_t         selected_integrity_algorithmP)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef                             *message_p        = NULL;
  ue_mm_context_t                        *ue_mm_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ue_idP);
  emm_context_t                          *emm_ctx = NULL;

  if (ue_mm_context) {
    emm_ctx = &ue_mm_context->emm_context;

    message_p = itti_alloc_new_message(TASK_NAS_MME, NAS_CONNECTION_ESTABLISHMENT_CNF);
    NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).ue_id                           = ue_idP;
    NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).err_code                        = error_codeP;
    NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).nas_msg                         = msgP; msgP = NULL;
    NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).selected_encryption_algorithm   = selected_encryption_algorithmP;
    NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).selected_integrity_algorithm    = selected_integrity_algorithmP;


    AssertFatal((0 <= emm_ctx->_security.vector_index) && (MAX_EPS_AUTH_VECTORS > emm_ctx->_security.vector_index),
        "Invalid vector index %d", emm_ctx->_security.vector_index);

    derive_keNB (emm_ctx->_vector[emm_ctx->_security.vector_index].kasme,
        emm_ctx->_security.ul_count.seq_num | (emm_ctx->_security.ul_count.overflow << 8),
        NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).kenb);

    MSC_LOG_TX_MESSAGE(
        MSC_NAS_MME,
        MSC_MMEAPP_MME,
        NULL,0,
        "NAS_CONNECTION_ESTABLISHMENT_CNF ue id %06"PRIX32" len %u sea %x sia %x ",
        ue_idP, blength(msgP), selected_encryption_algorithmP, selected_integrity_algorithmP);

    itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  }

  OAILOG_FUNC_OUT(LOG_NAS);
}

//------------------------------------------------------------------------------
void nas_itti_detach_req(const mme_ue_s1ap_id_t      ue_idP)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef *message_p;

  message_p = itti_alloc_new_message(TASK_NAS_MME, NAS_DETACH_REQ);

  NAS_DETACH_REQ(message_p).ue_id = ue_idP;

  MSC_LOG_TX_MESSAGE(
                MSC_NAS_MME,
                MSC_MMEAPP_MME,
                NULL,0,
                "0 NAS_DETACH_REQ ue id %06"PRIX32" ",
          ue_idP);

  itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_NAS);
}

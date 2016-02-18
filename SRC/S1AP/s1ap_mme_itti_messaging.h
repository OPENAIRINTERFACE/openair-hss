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


#include <stdint.h>
#include <string.h>

#include "intertask_interface.h"
#include "common_types.h"
#include "s1ap_common.h"
#include "msc.h"
#include "assertions.h"
#include "log.h"

#ifndef FILE_S1AP_MME_ITTI_MESSAGING_SEEN
#define FILE_S1AP_MME_ITTI_MESSAGING_SEEN

int s1ap_mme_itti_send_sctp_request(uint8_t *buffer, const size_t length,
                                    const  uint32_t sctp_assoc_id_t, const sctp_stream_id_t stream);

int s1ap_mme_itti_nas_uplink_ind(const mme_ue_s1ap_id_t ue_id,
                                 uint8_t *const buffer,
                                 const size_t length);

int s1ap_mme_itti_nas_downlink_cnf(const mme_ue_s1ap_id_t ue_id,
                                   const nas_error_code_t error_code);


static inline void s1ap_mme_itti_mme_app_establish_ind(
  const enb_ue_s1ap_id_t  enb_ue_s1ap_id,
  const mme_ue_s1ap_id_t  mme_ue_s1ap_id,
  const uint8_t * const   nas_msg,
  const size_t            nas_msg_length,
  const long              cause,
  const tai_t             tai,
  const as_stmsi_t        s_tmsi)
{
  MessageDef  *message_p = NULL;

  LOG_FUNC_IN (LOG_S1AP);
  AssertFatal((nas_msg_length < 1000), "Bad length for NAS message %lu", nas_msg_length);
  message_p = itti_alloc_new_message(TASK_S1AP, MME_APP_CONNECTION_ESTABLISHMENT_IND);

  MME_APP_CONNECTION_ESTABLISHMENT_IND(message_p).mme_ue_s1ap_id           = mme_ue_s1ap_id;
  MME_APP_CONNECTION_ESTABLISHMENT_IND(message_p).enb_ue_s1ap_id           = enb_ue_s1ap_id;

  MME_APP_CONNECTION_ESTABLISHMENT_IND(message_p).nas.ue_id                 = mme_ue_s1ap_id;
  /* Mapping between asn1 definition and NAS definition */
  MME_APP_CONNECTION_ESTABLISHMENT_IND(message_p).nas.as_cause              = cause + 1;
  MME_APP_CONNECTION_ESTABLISHMENT_IND(message_p).nas.tai                  = tai;
  MME_APP_CONNECTION_ESTABLISHMENT_IND(message_p).nas.initial_nas_msg.length = nas_msg_length;

  MME_APP_CONNECTION_ESTABLISHMENT_IND(message_p).nas.initial_nas_msg.data   = CALLOC_CHECK(nas_msg_length, sizeof(uint8_t));
  memcpy(MME_APP_CONNECTION_ESTABLISHMENT_IND(message_p).nas.initial_nas_msg.data, nas_msg, nas_msg_length);

  MME_APP_CONNECTION_ESTABLISHMENT_IND(message_p).nas.s_tmsi            = s_tmsi;



  MSC_LOG_TX_MESSAGE(
        MSC_S1AP_MME,
        MSC_MMEAPP_MME,
        NULL,0,
        "0 MME_APP_CONNECTION_ESTABLISHMENT_IND ue_id "MME_UE_S1AP_ID_FMT" as cause %u  tac %u stmsi %" PRIX8 ".%" PRIX32" len %u ",
        mme_ue_s1ap_id,
        MME_APP_CONNECTION_ESTABLISHMENT_IND(message_p).nas.as_cause,
        MME_APP_CONNECTION_ESTABLISHMENT_IND(message_p).nas.tac,
        MME_APP_CONNECTION_ESTABLISHMENT_IND(message_p).nas.s_tmsi.mme_code,
        MME_APP_CONNECTION_ESTABLISHMENT_IND(message_p).nas.s_tmsi.m_tmsi,
        MME_APP_CONNECTION_ESTABLISHMENT_IND(message_p).nas.initial_nas_msg.length);
  // should be sent to MME_APP, but this one would forward it to NAS_MME, so send it directly to NAS_MME
  // but let's see
  itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  LOG_FUNC_OUT (LOG_S1AP);
}


#if ORIGINAL_CODE
static inline void s1ap_mme_itti_nas_establish_ind(
  const uint32_t ue_id,
  uint8_t * const nas_msg,
  const size_t nas_msg_length,
  const long cause,
  const tai_t tai,
  const as_stmsi_t  s_tmsi)
{
  MessageDef     *message_p;

  LOG_FUNC_IN (LOG_S1AP);
  message_p = itti_alloc_new_message(TASK_S1AP, NAS_CONNECTION_ESTABLISHMENT_IND);

  NAS_CONN_EST_IND(message_p).nas.ue_id                 = ue_id;
  /* Mapping between asn1 definition and NAS definition */
  NAS_CONN_EST_IND(message_p).nas.as_cause              = cause + 1;
  NAS_CONN_EST_IND(message_p).nas.tai                  = tai;
  NAS_CONN_EST_IND(message_p).nas.s_tmsi               = s_tmsi;
  NAS_CONN_EST_IND(message_p).nas.initial_nas_msg.length = nas_msg_length;

  NAS_CONN_EST_IND(message_p).nas.initial_nas_msg.data = MALLOC_CHECK(sizeof(uint8_t) * nas_msg_length);
  memcpy(NAS_CONN_EST_IND(message_p).nas.initial_nas_msg.data, nas_msg, nas_msg_length);


  MSC_LOG_TX_MESSAGE(
  		MSC_S1AP_MME,
  		MSC_NAS_MME,
  		NULL,0,
  		"0 NAS_CONNECTION_ESTABLISHMENT_IND ue_id "ENB_UE_S1AP_ID_FMT" as cause %u  tac %u len %u",
  		ue_id,
  		NAS_CONN_EST_IND(message_p).nas.as_cause,
  		NAS_CONN_EST_IND(message_p).nas.tac,
  		NAS_CONN_EST_IND(message_p).nas.initial_nas_msg.length);

  // should be sent to MME_APP, but this one would forward it to NAS_MME, so send it directly to NAS_MME
  // but let's see
  itti_send_msg_to_task(TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  LOG_FUNC_OUT (LOG_S1AP);
}
#endif

static inline void s1ap_mme_itti_nas_non_delivery_ind(
    const mme_ue_s1ap_id_t ue_id, uint8_t * const nas_msg, const size_t nas_msg_length)
{
  MessageDef     *message_p;

  LOG_FUNC_IN (LOG_S1AP);
  message_p = itti_alloc_new_message(TASK_S1AP, NAS_DOWNLINK_DATA_REJ);

  NAS_DL_DATA_REJ(message_p).ue_id               = ue_id;
  /* Mapping between asn1 definition and NAS definition */
  //NAS_NON_DELIVERY_IND(message_p).as_cause              = cause + 1;
  NAS_DL_DATA_REJ(message_p).nas_msg.length = nas_msg_length;

  NAS_DL_DATA_REJ(message_p).nas_msg.data = malloc(sizeof(uint8_t) * nas_msg_length);
  memcpy(NAS_DL_DATA_REJ(message_p).nas_msg.data, nas_msg, nas_msg_length);


  MSC_LOG_TX_MESSAGE(
  		MSC_S1AP_MME,
  		MSC_NAS_MME,
  		NULL,0,
  		"0 NAS_DOWNLINK_DATA_REJ ue_id "MME_UE_S1AP_ID_FMT" len %u",
  		ue_id,
  		NAS_DL_DATA_REJ(message_p).nas_msg.length);

  // should be sent to MME_APP, but this one would forward it to NAS_MME, so send it directly to NAS_MME
  // but let's see
  itti_send_msg_to_task(TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  LOG_FUNC_OUT (LOG_S1AP);
}

#endif /* FILE_S1AP_MME_ITTI_MESSAGING_SEEN */

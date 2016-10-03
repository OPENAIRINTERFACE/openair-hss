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
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "log.h"
#include "assertions.h"
#include "intertask_interface.h"
#include "s1ap_common.h"
#include "s1ap_mme_itti_messaging.h"
#include "xml_msg_dump_itti.h"


//------------------------------------------------------------------------------
int
s1ap_mme_itti_send_sctp_request (
  STOLEN_REF bstring *payload,
  const sctp_assoc_id_t assoc_id,
  const sctp_stream_id_t stream,
  const mme_ue_s1ap_id_t ue_id)
{
  MessageDef                             *message_p = NULL;

  message_p = itti_alloc_new_message (TASK_S1AP, SCTP_DATA_REQ);

  SCTP_DATA_REQ (message_p).payload = *payload;
  *payload = NULL;
  SCTP_DATA_REQ (message_p).assoc_id = assoc_id;
  SCTP_DATA_REQ (message_p).stream = stream;
  SCTP_DATA_REQ (message_p).mme_ue_s1ap_id = ue_id;
  return itti_send_msg_to_task (TASK_SCTP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
s1ap_mme_itti_nas_uplink_ind (
  const mme_ue_s1ap_id_t  ue_id,
  STOLEN_REF bstring *payload,
  const tai_t      const* tai,
  const ecgi_t     const* cgi)
{
  MessageDef                             *message_p = NULL;

  message_p = itti_alloc_new_message (TASK_S1AP, NAS_UPLINK_DATA_IND);

  NAS_UL_DATA_IND (message_p).ue_id          = ue_id;
  NAS_UL_DATA_IND (message_p).nas_msg        = *payload;
  *payload = NULL;
  NAS_UL_DATA_IND (message_p).tai            = *tai;
  NAS_UL_DATA_IND (message_p).cgi            = *cgi;

  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_UPLINK_DATA_IND ue_id " MME_UE_S1AP_ID_FMT " len %u",
      NAS_UL_DATA_IND (message_p).ue_id, blength(NAS_UL_DATA_IND (message_p).nas_msg));
  XML_MSG_DUMP_ITTI_NAS_UPLINK_DATA_IND(&NAS_UL_DATA_IND (message_p), TASK_S1AP, TASK_NAS_MME, NULL);
  return itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
s1ap_mme_itti_nas_downlink_cnf (
  const mme_ue_s1ap_id_t ue_id,
  const bool             is_success)
{
  MessageDef                             *message_p = NULL;

  message_p = itti_alloc_new_message (TASK_S1AP, NAS_DOWNLINK_DATA_CNF);

  NAS_DL_DATA_CNF (message_p).ue_id = ue_id;
  if (is_success) {
    NAS_DL_DATA_CNF (message_p).err_code = AS_SUCCESS;
  } else {
    NAS_DL_DATA_CNF (message_p).err_code = AS_FAILURE;
  }
  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_DOWNLINK_DATA_CNF ue_id " MME_UE_S1AP_ID_FMT " err_code %u",
      NAS_DL_DATA_CNF (message_p).ue_id, NAS_DL_DATA_CNF (message_p).err_code);
  XML_MSG_DUMP_ITTI_NAS_DOWNLINK_DATA_CNF(&NAS_DL_DATA_CNF (message_p), TASK_S1AP, TASK_NAS_MME, NULL);
  return itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
}
//------------------------------------------------------------------------------
void s1ap_mme_itti_mme_app_initial_ue_message(
  const sctp_assoc_id_t   assoc_id,
  const enb_ue_s1ap_id_t  enb_ue_s1ap_id,
  const mme_ue_s1ap_id_t  mme_ue_s1ap_id,
  const uint8_t * const   nas_msg,
  const size_t            nas_msg_length,
  const tai_t      const* tai,
  const ecgi_t     const* cgi,
  const long              rrc_cause,
  const s_tmsi_t   const* opt_s_tmsi,
  const csg_id_t   const* opt_csg_id,
  const gummei_t   const* opt_gummei,
  const void       const* opt_cell_access_mode,
  const void       const* opt_cell_gw_transport_address,
  const void       const* opt_relay_node_indicator)
{
  MessageDef  *message_p = NULL;

  OAILOG_FUNC_IN (LOG_S1AP);
  AssertFatal((nas_msg_length < 1000), "Bad length for NAS message %lu", nas_msg_length);
  message_p = itti_alloc_new_message(TASK_S1AP, MME_APP_INITIAL_UE_MESSAGE);

  MME_APP_INITIAL_UE_MESSAGE(message_p).sctp_assoc_id          = assoc_id;
  MME_APP_INITIAL_UE_MESSAGE(message_p).enb_ue_s1ap_id         = enb_ue_s1ap_id;
  MME_APP_INITIAL_UE_MESSAGE(message_p).mme_ue_s1ap_id         = mme_ue_s1ap_id;

  MME_APP_INITIAL_UE_MESSAGE(message_p).nas                    = blk2bstr(nas_msg, nas_msg_length);

  MME_APP_INITIAL_UE_MESSAGE(message_p).tai                    = *tai;
  MME_APP_INITIAL_UE_MESSAGE(message_p).cgi                    = *cgi;
  MME_APP_INITIAL_UE_MESSAGE(message_p).rrc_establishment_cause = rrc_cause + 1;

  if (opt_s_tmsi) {
    MME_APP_INITIAL_UE_MESSAGE(message_p).is_s_tmsi_valid      = true;
    MME_APP_INITIAL_UE_MESSAGE(message_p).opt_s_tmsi           = *opt_s_tmsi;
  } else {
    MME_APP_INITIAL_UE_MESSAGE(message_p).is_s_tmsi_valid      = false;
  }
  if (opt_csg_id) {
    MME_APP_INITIAL_UE_MESSAGE(message_p).is_csg_id_valid      = true;
    MME_APP_INITIAL_UE_MESSAGE(message_p).opt_csg_id           = *opt_csg_id;
  } else {
    MME_APP_INITIAL_UE_MESSAGE(message_p).is_csg_id_valid      = false;
  }
  if (opt_gummei) {
    MME_APP_INITIAL_UE_MESSAGE(message_p).is_gummei_valid      = true;
    MME_APP_INITIAL_UE_MESSAGE(message_p).opt_gummei           = *opt_gummei;
  } else {
    MME_APP_INITIAL_UE_MESSAGE(message_p).is_gummei_valid      = false;
  }

  MME_APP_INITIAL_UE_MESSAGE(message_p).transparent.mme_ue_s1ap_id = mme_ue_s1ap_id;
  MME_APP_INITIAL_UE_MESSAGE(message_p).transparent.enb_ue_s1ap_id = enb_ue_s1ap_id;
  MME_APP_INITIAL_UE_MESSAGE(message_p).transparent.e_utran_cgi    = *cgi;


  MSC_LOG_TX_MESSAGE(
        MSC_S1AP_MME,
        MSC_MMEAPP_MME,
        NULL,0,
        "0 MME_APP_INITIAL_UE_MESSAGE ue_id "MME_UE_S1AP_ID_FMT" as cause %u tai:%c%c%c.%c%c%c:%u len %u ",
        mme_ue_s1ap_id,
        MME_APP_INITIAL_UE_MESSAGE(message_p).rrc_establishment_cause,
        (char)(MME_APP_INITIAL_UE_MESSAGE(message_p).tai.mcc_digit1 + 0x30),
        (char)(MME_APP_INITIAL_UE_MESSAGE(message_p).tai.mcc_digit2 + 0x30),
        (char)(MME_APP_INITIAL_UE_MESSAGE(message_p).tai.mcc_digit3 + 0x30),
        (char)(MME_APP_INITIAL_UE_MESSAGE(message_p).tai.mnc_digit1 + 0x30),
        (char)(MME_APP_INITIAL_UE_MESSAGE(message_p).tai.mnc_digit2 + 0x30),
        (9 < MME_APP_INITIAL_UE_MESSAGE(message_p).tai.mnc_digit3) ? ' ': (char)(MME_APP_INITIAL_UE_MESSAGE(message_p).tai.mnc_digit3 + 0x30),
        MME_APP_INITIAL_UE_MESSAGE(message_p).tai.tac,
        MME_APP_INITIAL_UE_MESSAGE(message_p).nas->slen);
  itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_S1AP);
}
//------------------------------------------------------------------------------
void s1ap_mme_itti_nas_non_delivery_ind(
    const mme_ue_s1ap_id_t ue_id, uint8_t * const nas_msg, const size_t nas_msg_length, const S1ap_Cause_t * const cause)
{
  MessageDef     *message_p = NULL;
  // TODO translate, insert, cause in message
  OAILOG_FUNC_IN (LOG_S1AP);
  message_p = itti_alloc_new_message(TASK_S1AP, NAS_DOWNLINK_DATA_REJ);

  NAS_DL_DATA_REJ(message_p).ue_id               = ue_id;
  /* Mapping between asn1 definition and NAS definition */
  //NAS_NON_DELIVERY_IND(message_p).as_cause              = cause + 1;
  NAS_DL_DATA_REJ(message_p).nas_msg  = blk2bstr(nas_msg, nas_msg_length);

  MSC_LOG_TX_MESSAGE(
      MSC_S1AP_MME,
      MSC_NAS_MME,
      NULL,0,
      "0 NAS_DOWNLINK_DATA_REJ ue_id "MME_UE_S1AP_ID_FMT" len %u",
      ue_id,
      NAS_DL_DATA_REJ(message_p).nas_msg->slen);

  XML_MSG_DUMP_ITTI_NAS_DOWNLINK_DATA_REJ(&NAS_DL_DATA_REJ (message_p), TASK_S1AP, TASK_NAS_MME, NULL);

  // should be sent to MME_APP, but this one would forward it to NAS_MME, so send it directly to NAS_MME
  // but let's see
  itti_send_msg_to_task(TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_S1AP);
}

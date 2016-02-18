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

#include <string.h>

#include "intertask_interface.h"
#include "nas_itti_messaging.h"
#include "msc.h"


#define TASK_ORIGIN  TASK_NAS_MME


static const uint8_t                    emm_message_ids[] = {
  ATTACH_REQUEST,
  ATTACH_ACCEPT,
  ATTACH_COMPLETE,
  ATTACH_REJECT,
  DETACH_REQUEST,
  DETACH_ACCEPT,
  TRACKING_AREA_UPDATE_REQUEST,
  TRACKING_AREA_UPDATE_ACCEPT,
  TRACKING_AREA_UPDATE_COMPLETE,
  TRACKING_AREA_UPDATE_REJECT,
  EXTENDED_SERVICE_REQUEST,
  SERVICE_REQUEST,
  SERVICE_REJECT,
  GUTI_REALLOCATION_COMMAND,
  GUTI_REALLOCATION_COMPLETE,
  AUTHENTICATION_REQUEST,
  AUTHENTICATION_RESPONSE,
  AUTHENTICATION_REJECT,
  AUTHENTICATION_FAILURE,
  IDENTITY_REQUEST,
  IDENTITY_RESPONSE,
  SECURITY_MODE_COMMAND,
  SECURITY_MODE_COMPLETE,
  SECURITY_MODE_REJECT,
  EMM_STATUS,
  EMM_INFORMATION,
  DOWNLINK_NAS_TRANSPORT,
  UPLINK_NAS_TRANSPORT,
  CS_SERVICE_NOTIFICATION,
};

static const uint8_t                    esm_message_ids[] = {
  ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST,
  ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT,
  ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT,
  ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST,
  ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT,
  ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT,
  MODIFY_EPS_BEARER_CONTEXT_REQUEST,
  MODIFY_EPS_BEARER_CONTEXT_ACCEPT,
  MODIFY_EPS_BEARER_CONTEXT_REJECT,
  DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST,
  DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT,
  PDN_CONNECTIVITY_REQUEST,
  PDN_CONNECTIVITY_REJECT,
  PDN_DISCONNECT_REQUEST,
  PDN_DISCONNECT_REJECT,
  BEARER_RESOURCE_ALLOCATION_REQUEST,
  BEARER_RESOURCE_ALLOCATION_REJECT,
  BEARER_RESOURCE_MODIFICATION_REQUEST,
  BEARER_RESOURCE_MODIFICATION_REJECT,
  ESM_INFORMATION_REQUEST,
  ESM_INFORMATION_RESPONSE,
  ESM_STATUS,
};

static int
_nas_find_message_index (
  const uint8_t message_id,
  const uint8_t * message_ids,
  const int ids_number)
{
  int                                     i;

  for (i = 0; i < ids_number; i++) {
    if (message_id == message_ids[i]) {
      return (2 + i);
    }
  }

  return (1);
}

int
nas_itti_plain_msg (
  const char *buffer,
  const nas_message_t * msg,
  const size_t          length,
  const bool            is_down_link)
{
  MessageDef                             *message_p = NULL;
  int                                     data_length = length < NAS_DATA_LENGHT_MAX ? length : NAS_DATA_LENGHT_MAX;
  int                                     message_type = -1;
  MessagesIds                             messageId_raw = -1;
  MessagesIds                             messageId_plain = -1;

  /*
   * Define message ids
   */
  if (msg->header.protocol_discriminator == EPS_MOBILITY_MANAGEMENT_MESSAGE) {
    message_type = 0;
    messageId_raw = is_down_link ? NAS_DL_EMM_RAW_MSG : NAS_UL_EMM_RAW_MSG;
    messageId_plain = is_down_link ? NAS_DL_EMM_PLAIN_MSG : NAS_UL_EMM_PLAIN_MSG;
  } else {
    if (msg->header.protocol_discriminator == EPS_SESSION_MANAGEMENT_MESSAGE) {
      message_type = 1;
      messageId_raw = is_down_link ? NAS_DL_ESM_RAW_MSG : NAS_UL_ESM_RAW_MSG;
      messageId_plain = is_down_link ? NAS_DL_ESM_PLAIN_MSG : NAS_UL_ESM_PLAIN_MSG;
    }
  }

  if (message_type >= 0) {
    /*
     * Create and send the RAW message
     */
    message_p = itti_alloc_new_message (TASK_ORIGIN, messageId_raw);
    NAS_DL_EMM_RAW_MSG (message_p).length = length;
    memset ((void *)&(NAS_DL_EMM_RAW_MSG (message_p).data), 0, NAS_DATA_LENGHT_MAX);
    memcpy ((void *)&(NAS_DL_EMM_RAW_MSG (message_p).data), buffer, data_length);
    itti_send_msg_to_task (TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);

    /*
     * Create and send the plain message
     */
    if (message_type == 0) {
      message_p = itti_alloc_new_message (TASK_ORIGIN, messageId_plain);
      NAS_DL_EMM_PLAIN_MSG (message_p).present = _nas_find_message_index (msg->plain.emm.header.message_type, emm_message_ids, sizeof (emm_message_ids) / sizeof (emm_message_ids[0]));
      memcpy ((void *)&(NAS_DL_EMM_PLAIN_MSG (message_p).choice), &msg->plain.emm, sizeof (EMM_msg));
    } else {
      message_p = itti_alloc_new_message (TASK_ORIGIN, messageId_plain);
      NAS_DL_ESM_PLAIN_MSG (message_p).present = _nas_find_message_index (msg->plain.esm.header.message_type, esm_message_ids, sizeof (esm_message_ids) / sizeof (esm_message_ids[0]));
      memcpy ((void *)&(NAS_DL_ESM_PLAIN_MSG (message_p).choice), &msg->plain.esm, sizeof (ESM_msg));
    }

    return itti_send_msg_to_task (TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
  }

  return EXIT_FAILURE;
}

int
nas_itti_protected_msg (
  const char          *buffer,
  const nas_message_t *msg,
  const size_t         length,
  const bool           is_down_link)
{
  MessageDef                             *message_p = NULL;

  if (msg->header.protocol_discriminator == EPS_MOBILITY_MANAGEMENT_MESSAGE) {
    message_p = itti_alloc_new_message (TASK_ORIGIN, is_down_link ? NAS_DL_EMM_PROTECTED_MSG : NAS_UL_EMM_PROTECTED_MSG);
    memcpy ((void *)&(NAS_DL_EMM_PROTECTED_MSG (message_p).header), &msg->header, sizeof (nas_message_security_header_t));
    NAS_DL_EMM_PROTECTED_MSG (message_p).present = _nas_find_message_index (msg->security_protected.plain.emm.header.message_type, emm_message_ids, sizeof (emm_message_ids) / sizeof (emm_message_ids[0]));
    memcpy ((void *)&(NAS_DL_EMM_PROTECTED_MSG (message_p).choice), &msg->security_protected.plain.emm, sizeof (EMM_msg));
  } else {
    if (msg->header.protocol_discriminator == EPS_SESSION_MANAGEMENT_MESSAGE) {
      message_p = itti_alloc_new_message (TASK_ORIGIN, is_down_link ? NAS_DL_ESM_PROTECTED_MSG : NAS_UL_ESM_PROTECTED_MSG);
      memcpy ((void *)&(NAS_DL_ESM_PROTECTED_MSG (message_p).header), &msg->header, sizeof (nas_message_security_header_t));
      NAS_DL_ESM_PROTECTED_MSG (message_p).present = _nas_find_message_index (msg->security_protected.plain.esm.header.message_type, esm_message_ids, sizeof (esm_message_ids) / sizeof (esm_message_ids[0]));
      memcpy ((void *)&(NAS_DL_ESM_PROTECTED_MSG (message_p).choice), &msg->security_protected.plain.esm, sizeof (ESM_msg));
    }
  }

  if (message_p ) {
    return itti_send_msg_to_task (TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
  }

  return EXIT_FAILURE;
}

int
nas_itti_dl_data_req (
  const mme_ue_s1ap_id_t ue_id,
  void         *const    data,
  const size_t           length)
{
  MessageDef  *message_p = itti_alloc_new_message (TASK_NAS_MME, NAS_DOWNLINK_DATA_REQ);
  NAS_DL_DATA_REQ (message_p).enb_ue_s1ap_id = INVALID_ENB_UE_S1AP_ID;
  NAS_DL_DATA_REQ (message_p).ue_id = ue_id;
  NAS_DL_DATA_REQ (message_p).nas_msg.data = data;
  NAS_DL_DATA_REQ (message_p).nas_msg.length = length;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_S1AP_MME, NULL, 0, "0 NAS_DOWNLINK_DATA_REQ ue id " MME_UE_S1AP_ID_FMT " len %u", ue_id, length);
  // make a long way by MME_APP instead of S1AP to retrieve the sctp_association_id key.
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}


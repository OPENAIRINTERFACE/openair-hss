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



#ifndef FILE_MME_APP_ITTI_MESSAGING_SEEN
#define FILE_MME_APP_ITTI_MESSAGING_SEEN

static inline void
mme_app_itti_auth_fail(
  const mme_ue_s1ap_id_t ue_id,
  const nas_cause_t cause)
{
  MessageDef *message_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  message_p = itti_alloc_new_message(TASK_MME_APP, NAS_AUTHENTICATION_PARAM_FAIL);

  NAS_AUTHENTICATION_PARAM_FAIL(message_p).ue_id = ue_id;
  NAS_AUTHENTICATION_PARAM_FAIL(message_p).cause = cause;

  MSC_LOG_TX_MESSAGE(
  		MSC_MMEAPP_MME,
  		MSC_NAS_MME,
  		NULL,0,
  		"0 NAS_AUTHENTICATION_PARAM_FAIL ue_id %06"PRIX32" cause %u",
  		ue_id, cause);

  itti_send_msg_to_task(TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}



static inline void mme_app_itti_auth_rsp(
  const mme_ue_s1ap_id_t         ue_id,
  const uint8_t                  nb_vectors,
  const  eutran_vector_t * const vector)
{
  MessageDef *message_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  message_p = itti_alloc_new_message(TASK_MME_APP, NAS_AUTHENTICATION_PARAM_RSP);

  NAS_AUTHENTICATION_PARAM_RSP(message_p).ue_id       = ue_id;
  NAS_AUTHENTICATION_PARAM_RSP(message_p).nb_vectors  = nb_vectors;
  memcpy(&NAS_AUTHENTICATION_PARAM_RSP(message_p).vector, vector, sizeof(*vector));

  MSC_LOG_TX_MESSAGE(
  		MSC_MMEAPP_MME,
  		MSC_NAS_MME,
  		NULL,0,
  		"0 NAS_AUTHENTICATION_PARAM_RSP ue_id %06"PRIX32" nb_vectors %u",
  		ue_id, nb_vectors);
  itti_send_msg_to_task(TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

#endif /* FILE_MME_APP_ITTI_MESSAGING_SEEN */

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

#include "msc.h"


static inline void mme_app_itti_delete_session_rsp(
  const mme_ue_s1ap_id_t   ue_idP)
{
  MessageDef *message_p;

  message_p = itti_alloc_new_message(TASK_MME_APP, MME_APP_DELETE_SESSION_RSP);

  MME_APP_DELETE_SESSION_RSP(message_p).ue_id = ue_idP;

  MSC_LOG_TX_MESSAGE(
                MSC_MMEAPP_MME,
                MSC_S1AP_MME,
                NULL,0,
                "0 MME_APP_DELETE_SESSION_RSP ue id %06"PRIX32" ",
          ue_idP);

  itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_NAS);
}
#endif /* FILE_MME_APP_ITTI_MESSAGING_SEEN */

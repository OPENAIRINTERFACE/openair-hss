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


#include "common_types.h"
#include "assertions.h"
#include "msc.h"
#include "log.h"
#include "mme_app_if.h"

#if ITTI_LITE
int itf_mme_app_nas_initial_ue_message(
    const mme_ue_s1ap_id_t  mme_ue_s1ap_id,
    const uint8_t * const nas_msg,
    const uint32_t  nas_msg_length,
    const long      cause,
    const uint8_t   tai_plmn[3],
    const uint16_t  tai_tac,
    const uint64_t  s_tmsi)
{
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;

  LOG_FUNC_IN (LOG_MME_APP);
  LOG_DEBUG (LOG_MME_APP, "Received MME_APP_CONNECTION_ESTABLISHMENT_IND from S1AP\n");
  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id);

  if (ue_context_p == NULL) {
  LOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: %06" PRIX32 "/dec%u\n", mme_ue_s1ap_id, mme_ue_s1ap_id);
  LOG_DEBUG (LOG_MME_APP, "UE context doesn't exist -> create one\n");

    if ((ue_context_p = mme_create_new_ue_context ()) == NULL) {
      /*
       * Error during ue context MALLOC_CHECK
       */
      /*
       * TODO
       */
      DevMessage ("mme_create_new_ue_context");
      LOG_FUNC_OUT (LOG_MME_APP);
    }
    // S1AP UE ID AND NAS UE ID ARE THE SAME
    ue_context_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
    ue_context_p->ue_id = mme_ue_s1ap_id;
    DevAssert (mme_insert_ue_context (&mme_app_desc.mme_ue_contexts, ue_context_p) == 0);
    // tests
    ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id);
    AssertFatal (ue_context_p , "mme_ue_context_exists_mme_ue_s1ap_id Failed");
    ue_context_p = mme_ue_context_exists_nas_ue_id (&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id);
    AssertFatal (ue_context_p , "mme_ue_context_exists_nas_ue_id Failed");
  }

  // call NAS API

/*  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_CONNECTION_ESTABLISHMENT_IND);
  // do this because of same message types name but not same struct in different .h
  message_p->ittiMsg.nas_conn_est_ind.nas.ue_id    = mme_ue_s1ap_id;
  message_p->ittiMsg.nas_conn_est_ind.nas.plmn[0] = tai_plmn[0];
  message_p->ittiMsg.nas_conn_est_ind.nas.plmn[1] = tai_plmn[1];
  message_p->ittiMsg.nas_conn_est_ind.nas.plmn[2] = tai_plmn[2];
  message_p->ittiMsg.nas_conn_est_ind.nas.tac     = tai_tac;
  message_p->ittiMsg.nas_conn_est_ind.nas.as_cause = cause;
  message_p->ittiMsg.nas_conn_est_ind.nas.s_tmsi  = s_tmsi;
  memcpy (&message_p->ittiMsg.nas_conn_est_ind.nas.initial_nas_msg, &nas_msg, nas_msg_length);
  //memcpy(&NAS_CONN_EST_IND(message_p).nas,
  // &conn_est_ind_pP->nas,
  // sizeof (nas_establish_ind_t));

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_CONNECTION_ESTABLISHMENT_IND");
  itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
*/
  LOG_FUNC_OUT (LOG_MME_APP);
}
#endif


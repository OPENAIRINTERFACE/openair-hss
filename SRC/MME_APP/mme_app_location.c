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


/*! \file mme_app_location.c
   \brief
   \author Sebastien ROUX, Lionel GAUTHIER
   \version 1.0
   \company Eurecom
   \email: lionel.gauthier@eurecom.fr
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"

#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "common_types.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "gcc_diag.h"
#include "common_defs.h"
#include "mme_config.h"
#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "secu_defs.h"

//------------------------------------------------------------------------------
int mme_app_send_s6a_update_location_req (
  struct ue_mm_context_s *const ue_context_pP)
{
  struct ue_mm_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;
  s6a_update_location_req_t              *s6a_ulr_p = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);

  message_p = itti_alloc_new_message (TASK_MME_APP, S6A_UPDATE_LOCATION_REQ);

  if (message_p == NULL) {
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  s6a_ulr_p = &message_p->ittiMsg.s6a_update_location_req;
  IMSI64_TO_STRING (ue_context_pP->emm_context._imsi64, s6a_ulr_p->imsi);
  s6a_ulr_p->imsi_length = strlen (s6a_ulr_p->imsi);
  s6a_ulr_p->initial_attach = INITIAL_ATTACH;
  memcpy (&s6a_ulr_p->visited_plmn, &ue_context_p->emm_context._guti.gummei.plmn, sizeof (plmn_t));
  s6a_ulr_p->rat_type = RAT_EUTRAN;
  /*
   * Check if we already have UE data
   */
  s6a_ulr_p->skip_subscriber_data = 0;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S6A_MME, NULL, 0, "0 S6A_UPDATE_LOCATION_REQ imsi " IMSI_64_FMT, ue_context_pP->emm_context._imsi64);
  rc =  itti_send_msg_to_task (TASK_S6A, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}


//------------------------------------------------------------------------------
int mme_app_handle_s6a_update_location_ans (
  const s6a_update_location_ans_t * const ula_pP)
{
  uint64_t                                imsi64 = 0;
  struct ue_mm_context_s                 *ue_mm_context = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ula_pP );

  if (ula_pP->result.present == S6A_RESULT_BASE) {
    if (ula_pP->result.choice.base != DIAMETER_SUCCESS) {
      /*
       * The update location procedure has failed. Notify the NAS layer
       * and don't initiate the bearer creation on S-GW side.
       */
      OAILOG_DEBUG (LOG_MME_APP, "ULR/ULA procedure returned non success (ULA.result.choice.base=%d)\n", ula_pP->result.choice.base);
      DevMessage ("ULR/ULA procedure returned non success\n");
    }
  } else {
    /*
     * The update location procedure has failed. Notify the NAS layer
     * and don't initiate the bearer creation on S-GW side.
     */
    OAILOG_DEBUG (LOG_MME_APP, "ULR/ULA procedure returned non success (ULA.result.present=%d)\n", ula_pP->result.present);
    DevMessage ("ULR/ULA procedure returned non success\n");
  }

  IMSI_STRING_TO_IMSI64 ((char *)ula_pP->imsi, &imsi64);
  OAILOG_DEBUG (LOG_MME_APP, "%s Handling imsi " IMSI_64_FMT "\n", __FUNCTION__, imsi64);

  if ((ue_mm_context = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, imsi64)) == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "That's embarrassing as we don't know this IMSI\n");
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "0 S6A_UPDATE_LOCATION unknown imsi " IMSI_64_FMT" ", imsi64);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  ue_mm_context->subscription_known = SUBSCRIPTION_KNOWN;
  ue_mm_context->subscriber_status = ula_pP->subscription_data.subscriber_status;
  ue_mm_context->access_restriction_data = ula_pP->subscription_data.access_restriction;
  /*
   * Copy the subscribed ambr to the sgw create session request message
   */
  memcpy (&ue_mm_context->suscribed_ue_ambr, &ula_pP->subscription_data.subscribed_ambr, sizeof (ambr_t));

  ue_mm_context->msisdn = blk2bstr(ula_pP->subscription_data.msisdn, ula_pP->subscription_data.msisdn_length);
  AssertFatal (ula_pP->subscription_data.msisdn_length != 0, "MSISDN LENGTH IS 0");
  AssertFatal (ula_pP->subscription_data.msisdn_length <= MSISDN_LENGTH, "MSISDN LENGTH is too high %u", MSISDN_LENGTH);

  ue_mm_context->rau_tau_timer = ula_pP->subscription_data.rau_tau_timer;
  ue_mm_context->network_access_mode = ula_pP->subscription_data.access_mode;
  memcpy (&ue_mm_context->apn_config_profile, &ula_pP->subscription_data.apn_config_profile, sizeof (apn_config_profile_t));


  MessageDef                             *message_p = NULL;
  itti_nas_pdn_config_rsp_t              *nas_pdn_config_rsp = NULL;

  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_PDN_CONFIG_RSP);

  if (message_p == NULL) {
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  nas_pdn_config_rsp = &message_p->ittiMsg.nas_pdn_config_rsp;
  nas_pdn_config_rsp->ue_id = ue_mm_context->mme_ue_s1ap_id;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_PDN_CONFIG_RESP imsi " IMSI_64_FMT, ue_mm_context->mme_ue_s1ap_id);
  rc =  itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

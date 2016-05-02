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
#include <stdlib.h>
#include <string.h>

#include "intertask_interface.h"
#include "mme_config.h"
#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "secu_defs.h"
#include "assertions.h"
#include "common_types.h"
#include "msc.h"
#include "log.h"


int
mme_app_send_s6a_update_location_req (
  struct ue_context_s *const ue_context_pP)
{
  struct ue_context_s                    *ue_context_p = NULL;
  mme_app_imsi_t                          imsi = {.length = 0};
  MessageDef                             *message_p = NULL;
  s6a_update_location_req_t              *s6a_ulr_p = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  mme_app_string_to_imsi (&imsi, (char *) ue_context_pP->pending_pdn_connectivity_req_imsi);
  OAILOG_DEBUG (LOG_MME_APP, "Handling imsi %" IMSI_FORMAT "\n", IMSI_DATA(imsi));

  if ((ue_context_p = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, imsi)) == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "That's embarrassing as we don't know this IMSI\n");
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  message_p = itti_alloc_new_message (TASK_MME_APP, S6A_UPDATE_LOCATION_REQ);

  if (message_p == NULL) {
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  s6a_ulr_p = &message_p->ittiMsg.s6a_update_location_req;
  memset ((void *)s6a_ulr_p, 0, sizeof (s6a_update_location_req_t));

  mme_app_imsi_to_string (s6a_ulr_p->imsi, &imsi);
  s6a_ulr_p->imsi_length = strlen (s6a_ulr_p->imsi);

  s6a_ulr_p->initial_attach = INITIAL_ATTACH;
  memcpy (&s6a_ulr_p->visited_plmn, &ue_context_p->guti.gummei.plmn, sizeof (plmn_t));
  s6a_ulr_p->rat_type = RAT_EUTRAN;
  /*
   * Check if we already have UE data
   */
  s6a_ulr_p->skip_subscriber_data = 0;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S6A_MME, NULL, 0, "0 S6A_UPDATE_LOCATION_REQ imsi %" IMSI_FORMAT, IMSI_DATA(imsi));
  rc =  itti_send_msg_to_task (TASK_S6A, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}



int
mme_app_handle_s6a_update_location_ans (
  const s6a_update_location_ans_t * const ula_pP)
{
  mme_app_imsi_t                          imsi = {.length = 0};
  struct ue_context_s                     *ue_context_p = NULL;
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

  mme_app_string_to_imsi( &imsi, (char *)ula_pP->imsi);
  OAILOG_DEBUG (LOG_MME_APP, "%s Handling imsi %" IMSI_FORMAT "\n", __FUNCTION__, IMSI_DATA(imsi));

  if ((ue_context_p = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, imsi)) == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "That's embarrassing as we don't know this IMSI\n");
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "0 S6A_UPDATE_LOCATION unknown imsi %" IMSI_FORMAT" ", IMSI_DATA(imsi));
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  ue_context_p->subscription_known = SUBSCRIPTION_KNOWN;
  ue_context_p->sub_status = ula_pP->subscription_data.subscriber_status;
  ue_context_p->access_restriction_data = ula_pP->subscription_data.access_restriction;
  /*
   * Copy the subscribed ambr to the sgw create session request message
   */
  memcpy (&ue_context_p->subscribed_ambr, &ula_pP->subscription_data.subscribed_ambr, sizeof (ambr_t));
  memcpy (ue_context_p->msisdn, ula_pP->subscription_data.msisdn, ula_pP->subscription_data.msisdn_length);
  ue_context_p->msisdn_length = ula_pP->subscription_data.msisdn_length;
  AssertFatal (ula_pP->subscription_data.msisdn_length != 0, "MSISDN LENGTH IS 0");
  AssertFatal (ula_pP->subscription_data.msisdn_length <= MSISDN_LENGTH, "MSISDN LENGTH is too high %u", MSISDN_LENGTH);
  ue_context_p->msisdn[ue_context_p->msisdn_length] = '\0';
  ue_context_p->rau_tau_timer = ula_pP->subscription_data.rau_tau_timer;
  ue_context_p->access_mode = ula_pP->subscription_data.access_mode;
  memcpy (&ue_context_p->apn_profile, &ula_pP->subscription_data.apn_config_profile, sizeof (apn_config_profile_t));
  rc =  mme_app_send_s11_create_session_req (ue_context_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

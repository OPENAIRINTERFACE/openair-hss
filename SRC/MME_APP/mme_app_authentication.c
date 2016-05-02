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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "msc.h"
#include "intertask_interface.h"
#include "mme_app_itti_messaging.h"
#include "mme_config.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "mcc_mnc_itu.h"
#include "assertions.h"
#include "log.h"

static
int                                     mme_app_request_authentication_info (
  const char *imsi,
  const uint8_t nb_of_vectors,
  const plmn_t * plmn,
  const uint8_t * auts);

static
  int
mme_app_request_authentication_info (
  const char *imsi,
  const uint8_t nb_of_vectors,
  const plmn_t * plmn,
  const uint8_t * auts)
{
  s6a_auth_info_req_t                    *auth_info_req = NULL;
  MessageDef                             *message_p = NULL;
  int                                     imsi_length = strlen (imsi);
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (plmn );
  message_p = itti_alloc_new_message (TASK_MME_APP, S6A_AUTH_INFO_REQ);
  auth_info_req = &message_p->ittiMsg.s6a_auth_info_req;
  memset (auth_info_req, 0, sizeof (*auth_info_req));
  strncpy (auth_info_req->imsi, imsi, imsi_length);
  auth_info_req->imsi_length = imsi_length;
  //MME_APP_IMSI_TO_STRING(imsi, auth_info_req->imsi);
  memcpy (&auth_info_req->visited_plmn, plmn, sizeof (plmn_t));
  OAILOG_DEBUG (LOG_MME_APP, "visited_plmn MCC %X%X%X MNC %X%X%X\n",
                 auth_info_req->visited_plmn.mcc_digit1, auth_info_req->visited_plmn.mcc_digit2, auth_info_req->visited_plmn.mcc_digit3,
                 auth_info_req->visited_plmn.mnc_digit1, auth_info_req->visited_plmn.mnc_digit2, auth_info_req->visited_plmn.mnc_digit3);
  uint8_t                                *ptr = (uint8_t *) & auth_info_req->visited_plmn;

  OAILOG_DEBUG (LOG_MME_APP, "visited_plmn %02X%02X%02X\n", ptr[0], ptr[1], ptr[2]);
  auth_info_req->nb_of_vectors = nb_of_vectors;

  if (auts ) {
    auth_info_req->re_synchronization = 1;
    memcpy (auth_info_req->auts, auts, sizeof (auth_info_req->auts));
  } else {
    auth_info_req->re_synchronization = 0;
    memset (auth_info_req->auts, 0, sizeof (auth_info_req->auts));
  }

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S6A_MME, NULL, 0, "0 S6A_AUTH_INFO_REQ IMSI %s visited_plmn %02X%02X%02X re_sync %u", imsi, ptr[0], ptr[1], ptr[2], auth_info_req->re_synchronization);
  rc = itti_send_msg_to_task (TASK_S6A, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}


int
mme_app_handle_authentication_info_answer (
  const s6a_auth_info_ans_t * const s6a_auth_info_ans_pP)
{
  struct ue_context_s                    *ue_context = NULL;
  mme_app_imsi_t                         imsi = {.length = 0};

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (s6a_auth_info_ans_pP );
  mme_app_string_to_imsi(&imsi, (char *)s6a_auth_info_ans_pP->imsi);
  OAILOG_DEBUG (LOG_MME_APP, "Handling imsi %" IMSI_FORMAT "\n", IMSI_DATA(imsi));

  if ((ue_context = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, imsi)) == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "That's embarrassing as we don't know this IMSI\n");
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "0 S6A_AUTH_INFO_ANS Unknown imsi %" IMSI_FORMAT, IMSI_DATA(imsi));
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  if ((s6a_auth_info_ans_pP->result.present == S6A_RESULT_BASE)
      && (s6a_auth_info_ans_pP->result.choice.base == DIAMETER_SUCCESS)) {
    /*
     * S6A procedure has succeeded.
     * We have to request UE authentication.
     */
    /*
     * Check that list is not empty and contain only one element
     */
    DevCheck (s6a_auth_info_ans_pP->auth_info.nb_of_vectors == 1, s6a_auth_info_ans_pP->auth_info.nb_of_vectors, 1, 0);

    if (ue_context->vector_list == NULL) {
      ue_context->vector_list = MALLOC_CHECK (sizeof (eutran_vector_t));
      DevAssert (ue_context->vector_list );
    } else {
      /*
       * Some vector already exist
       */
      ue_context->vector_list = realloc (ue_context->vector_list, (ue_context->nb_of_vectors + s6a_auth_info_ans_pP->auth_info.nb_of_vectors) * sizeof (eutran_vector_t));
      DevAssert (ue_context->vector_list );
    }

    memcpy (&ue_context->vector_list[ue_context->nb_of_vectors], &s6a_auth_info_ans_pP->auth_info.eutran_vector, sizeof (eutran_vector_t));
    ue_context->vector_in_use = &ue_context->vector_list[ue_context->nb_of_vectors];
    ue_context->nb_of_vectors += s6a_auth_info_ans_pP->auth_info.nb_of_vectors;
    OAILOG_DEBUG (LOG_MME_APP, "INFORMING NAS ABOUT AUTH RESP SUCCESS got %u vector(s)\n", s6a_auth_info_ans_pP->auth_info.nb_of_vectors);
    mme_app_itti_auth_rsp (ue_context->mme_ue_s1ap_id, 1, &s6a_auth_info_ans_pP->auth_info.eutran_vector);
  } else {
    OAILOG_ERROR (LOG_MME_APP, "INFORMING NAS ABOUT AUTH RESP ERROR CODE\n");
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "0 S6A_AUTH_INFO_ANS S6A Failure imsi %" IMSI_FORMAT, IMSIimsi);

    /*
     * Inform NAS layer with the right failure
     */
    if (s6a_auth_info_ans_pP->result.present == S6A_RESULT_BASE) {
      mme_app_itti_auth_fail (ue_context->mme_ue_s1ap_id, s6a_error_2_nas_cause (s6a_auth_info_ans_pP->result.choice.base, 0));
    } else {
      mme_app_itti_auth_fail (ue_context->mme_ue_s1ap_id, s6a_error_2_nas_cause (s6a_auth_info_ans_pP->result.choice.experimental, 1));
    }
  }
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}


void
mme_app_handle_nas_auth_param_req (
  const itti_nas_auth_param_req_t * const nas_auth_param_req_pP)
{
  plmn_t                                 *visited_plmn = NULL;
  struct ue_context_s                    *ue_context = NULL;
  mme_app_imsi_t                          imsi = {.length = 0};
  int                                     mnc_length = 0;

  plmn_t                                  visited_plmn_from_req = {
    .mcc_digit3 = 0,
    .mcc_digit2 = 0,
    .mcc_digit1 = 0,
    .mnc_digit1 = 0,
    .mnc_digit2 = 0,
    .mnc_digit3 = 0,
  };
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (nas_auth_param_req_pP );
  visited_plmn = &visited_plmn_from_req;
  visited_plmn_from_req.mcc_digit1 = nas_auth_param_req_pP->imsi[0];
  visited_plmn_from_req.mcc_digit2 = nas_auth_param_req_pP->imsi[1];
  visited_plmn_from_req.mcc_digit3 = nas_auth_param_req_pP->imsi[2];
  mnc_length = find_mnc_length (nas_auth_param_req_pP->imsi[0], nas_auth_param_req_pP->imsi[1], nas_auth_param_req_pP->imsi[2], nas_auth_param_req_pP->imsi[3], nas_auth_param_req_pP->imsi[4], nas_auth_param_req_pP->imsi[5]
    );

  if (mnc_length == 2) {
    visited_plmn_from_req.mnc_digit1 = nas_auth_param_req_pP->imsi[3];
    visited_plmn_from_req.mnc_digit2 = nas_auth_param_req_pP->imsi[4];
    visited_plmn_from_req.mnc_digit3 = 15;
  } else if (mnc_length == 3) {
    visited_plmn_from_req.mnc_digit1 = nas_auth_param_req_pP->imsi[3];
    visited_plmn_from_req.mnc_digit2 = nas_auth_param_req_pP->imsi[4];
    visited_plmn_from_req.mnc_digit3 = nas_auth_param_req_pP->imsi[5];
  } else {
    AssertFatal (0, "MNC Not found (mcc_mnc_list)");
  }

  if (mnc_length == 3) {
    OAILOG_DEBUG (LOG_MME_APP, "visited_plmn_from_req  %1d%1d%1d.%1d%1d%1d\n",
                   visited_plmn_from_req.mcc_digit1, visited_plmn_from_req.mcc_digit2, visited_plmn_from_req.mcc_digit3,
                   visited_plmn_from_req.mnc_digit1, visited_plmn_from_req.mnc_digit2, visited_plmn_from_req.mnc_digit3);
  } else {
    OAILOG_DEBUG (LOG_MME_APP, "visited_plmn_from_req  %1d%1d%1d.%1d%1d\n",
                   visited_plmn_from_req.mcc_digit1, visited_plmn_from_req.mcc_digit2, visited_plmn_from_req.mcc_digit3,
                   visited_plmn_from_req.mnc_digit1, visited_plmn_from_req.mnc_digit2);
  }

  mme_app_string_to_imsi(&imsi, &(nas_auth_param_req_pP->imsi));
  OAILOG_DEBUG (LOG_MME_APP, "Handling imsi %" IMSI_FORMAT "\n", IMSI_DATA(imsi));
  OAILOG_DEBUG (LOG_MME_APP, "Handling imsi from req  %s (mnc length %d)\n", nas_auth_param_req_pP->imsi, mnc_length);
  /*
   * Fetch the context associated with this IMSI
   */
  ue_context = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, imsi);

  if (ue_context == NULL) {
    /*
     * Currently no context available -> trigger an authentication request
     * to the HSS.
     */
    OAILOG_DEBUG (LOG_MME_APP, "UE context search by IMSI failed, try by mme_ue_s1ap_id\n");
    ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, nas_auth_param_req_pP->ue_id);

    if (ue_context == NULL) {
      OAILOG_ERROR (LOG_MME_APP, "UE context doesn't exist no action (should never see this message)\n");
      // should have been created by initial ue message
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }

    /*
     * We have no vector for this UE, send an authentication request
     * to the HSS.
     */
    /*
     * Acquire the current time
     */
    time (&ue_context->cell_age);

    // merge GUTI
    //TODO:  Very strange, check that!
    memcpy (&ue_context->guti.gummei.plmn, visited_plmn, sizeof (plmn_t));
    // update IMSI
    mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts,
                                     ue_context,
                                     ue_context->enb_ue_s1ap_id,
                                     ue_context->mme_ue_s1ap_id,
                                     imsi,       // imsi is new
                                     ue_context->mme_s11_teid,
                                     &ue_context->guti);       // guti is new
    OAILOG_DEBUG (LOG_MME_APP, "and we have no auth. vector for it, request authentication information\n");
    mme_app_request_authentication_info (nas_auth_param_req_pP->imsi, 1, visited_plmn, NULL);
  } else {
    // merge GUTI
    //TODO:  Very strange, check that!
    memcpy (&ue_context->guti.gummei.plmn, visited_plmn, sizeof (plmn_t));
    // update IMSI
    mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts,
                                     ue_context,
                                     ue_context->enb_ue_s1ap_id,
                                     ue_context->mme_ue_s1ap_id,
                                     ue_context->imsi,
                                     ue_context->mme_s11_teid,
                                     &ue_context->guti);      // guti is new
    mme_app_request_authentication_info (nas_auth_param_req_pP->imsi, 1, visited_plmn, nas_auth_param_req_pP->auts);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

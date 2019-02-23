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
#include "common_defs.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "mme_config.h"
#include "mme_app_procedures.h"

//------------------------------------------------------------------------------
int mme_app_handle_s6a_update_location_ans (
  s6a_update_location_ans_t * ula_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  MessageDef                             *message_p = NULL;
  uint64_t                                imsi64 = 0;
  struct ue_context_s                    *ue_context  = NULL;
  struct emm_data_context_s              *emm_context = NULL;
  int                                     rc = RETURNok;

  DevAssert (ula_pP );

  IMSI_STRING_TO_IMSI64 ((char *)ula_pP->imsi, &imsi64);
  OAILOG_DEBUG (LOG_MME_APP, "%s Handling imsi " IMSI_64_FMT "\n", __FUNCTION__, imsi64);

  if ((ue_context = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, imsi64)) == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "That's embarrassing as we don't know this IMSI\n");
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "0 S6A_UPDATE_LOCATION unknown imsi " IMSI_64_FMT" ", imsi64);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /** Recheck that the EMM Data Context is found by the IMSI. */
  if ((emm_context = emm_data_context_get_by_imsi(&_emm_data, imsi64)) == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "That's embarrassing as we don't know this IMSI " IMSI_64_FMT " for the EMM Data Context. \n", imsi64);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /** Remove the cached subscription profile and set the new one. */
  subscription_data_t * subscription_data = ula_pP->subscription_data;
  mme_remove_subscription_profile(&mme_app_desc.mme_ue_contexts, imsi64);
  if(ula_pP->subscription_data){
    mme_insert_subscription_profile(&mme_app_desc.mme_ue_contexts, imsi64, subscription_data);
  } else {
    goto err;
  }
  ula_pP->subscription_data = NULL;

  OAILOG_INFO(LOG_MME_APP, "Updated the subscription profile for IMSI " IMSI_64_FMT " in the cache. \n", imsi64);

  /** Send the S6a message. */
  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_PDN_CONFIG_RSP);
  if (message_p == NULL) {
    goto err;
  }
  if (ula_pP->result.present == S6A_RESULT_BASE) {
    if (ula_pP->result.choice.base != DIAMETER_SUCCESS) {
      /*
       * The update location procedure has failed. Notify the NAS layer
       * and don't initiate the bearer creation on S-GW side.
       */
      OAILOG_DEBUG (LOG_MME_APP, "ULR/ULA procedure returned non success (ULA.result.choice.base=%d)\n", ula_pP->result.choice.base);
      goto err;
    }
  } else {
    /*
     * The update location procedure has failed. Notify the NAS layer
     * and don't initiate the bearer creation on S-GW side.
     */
    OAILOG_DEBUG (LOG_MME_APP, "ULR/ULA procedure returned non success (ULA.result.present=%d)\n", ula_pP->result.present);
    goto err;
  }

  mme_app_update_ue_subscription(ue_context->mme_ue_s1ap_id, subscription_data);

  message_p->ittiMsg.s6a_update_location_req.ue_id  = ue_context->mme_ue_s1ap_id;
  message_p->ittiMsg.s6a_update_location_req.imsi64 = imsi64;

  /** Check if an attach procedure is running, if so send it to the ESM layer, if not, we assume it is triggered by a TAU request, send it to the EMM. */
  /** For error codes, use nas_pdn_cfg_fail. */
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_ESM, NULL, 0, "0 NAS_PDN_CONFIG_RESP IMSI " IMSI_64_FMT, imsi64);
  rc =  itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);

err:
  OAILOG_ERROR(LOG_MME_APP, "Error processing ULA for ueId " MME_UE_S1AP_ID_FMT ". Sending NAS_PDN_CONFIG_FAIL to NAS. (ULA.result.choice.base=%d)\n",
      ue_context->mme_ue_s1ap_id, ula_pP->result.choice.base);
  /** Send the S6a message. */
  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_PDN_CONFIG_FAIL);

  if (message_p == NULL) {
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  itti_nas_pdn_config_fail_t *nas_pdn_config_fail = &message_p->ittiMsg.nas_pdn_config_fail;
  nas_pdn_config_fail->ue_id  = ue_context->mme_ue_s1ap_id;

  /**
   * For error codes, use nas_pdn_cfg_fail.
   * Also in case of TAU, send back to ESM layer.
   */
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_PDN_CONFIG_FAIL IMSI " IMSI_64_FMT, imsi64);
  rc =  itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
//  UNLOCK_UE_CONTEXTS(ue_context);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
int
mme_app_handle_s6a_cancel_location_req(
  const s6a_cancel_location_req_t * const clr_pP)
{
  uint64_t                                imsi = 0;
  struct ue_context_s                    *ue_context = NULL;
  int                                     rc = RETURNok;
  MessageDef                             *message_p = NULL;
  mme_app_s10_proc_mme_handover_t        *s10_handover_proc = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (clr_pP );

  IMSI_STRING_TO_IMSI64 ((char *)clr_pP->imsi, &imsi);
  OAILOG_DEBUG (LOG_MME_APP, "%s Handling CANCEL_LOCATION_REQUEST for imsi " IMSI_64_FMT "\n", __FUNCTION__, imsi);

  if ((ue_context = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, imsi)) == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "That's embarrassing as we don't know this IMSI\n");
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "0 S6A_CANCEL_LOCATION unknown imsi " IMSI_64_FMT" ", imsi);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  /** Check the cancellation type.. */
  if(clr_pP->cancellation_type == MME_UPDATE_PROCEDURE){
    /**
     * Handle handover based cancellation procedure.
     * Check the MME mobility timer.. if the timer has not expired yet, just set a flag (not waiting in this thread and resending the signal
     * not to block resources --> any other way to send a signal after a given time --> timer with signal sending!!!).
     * Else remove the UE implicitly.. (not sending Purge_UE for this one).
     */
    OAILOG_INFO(LOG_MME_APP, "Handling CLR for MME_UPDATE_PROCEDURE for UE with imsi " IMSI_64_FMT " "
        "Checking the MME_MOBILITY_COMPLETION timer. \n", imsi);
    /** Checking CLR in the handover procedure. */
    s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
    if(s10_handover_proc){
      if(s10_handover_proc->proc.timer.id != MME_APP_TIMER_INACTIVE_ID){
        OAILOG_INFO(LOG_MME_APP, "S10 Handover Proc timer %u, is still running. Marking CLR but not removing the UE yet with imsi " IMSI_64_FMT ". \n",
            s10_handover_proc->proc.timer.id, imsi);
        s10_handover_proc->pending_clear_location_request = true; // todo: not checking the pending flag..
        OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
      }else{
        OAILOG_INFO(LOG_MME_APP, "S10 Handover Proc timer is not running for UE. Implicit removal for UE with imsi " IMSI_64_FMT " due handover/tau. \n", imsi);
        /** We also will remove the UE context, if the UE handovered back and CLR received afterwards. */
      }
    }else{
      OAILOG_INFO(LOG_MME_APP, "No S10 Handover Proc found for UE. Implicit removal for UE with imsi " IMSI_64_FMT " due handover/tau. \n", imsi);
    }
  }else{
    /** todo: handle rest of cancellation procedures.. */
    OAILOG_INFO(LOG_MME_APP, "Received cancellation type %d (not due handover). Not checking the MME_MOBILITY timer. Performing directly implicit detach on "
        "UE with imsi " IMSI_64_FMT " received unhandled cancellation type %d. \n", clr_pP->cancellation_type, imsi);
    // todo: not implicit removal but proper detach in this case..
  }

  /** Perform an implicit detach via NAS layer.. We purge context ourself or purge the MME_APP context. NAS has to purge the EMM context and the MME_APP context. */
  OAILOG_INFO (LOG_MME_APP, "Expired- Implicit Detach timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
  ue_context->implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;
  // Initiate Implicit Detach for the UE
  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
  DevAssert (message_p != NULL);
  message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->mme_ue_s1ap_id;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
  itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

int
mme_app_handle_s6a_reset_req(
  const s6a_reset_req_t * const rr_pP)
{
  uint64_t                                imsi = 0;
  struct ue_context_s                    *ue_context = NULL;
  int                                     rc = RETURNok;
  MessageDef                             *message_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (rr_pP );

  OAILOG_DEBUG (LOG_MME_APP, "%s Handling RESET_REQUEST \n", __FUNCTION__);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}



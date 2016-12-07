/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"

#include "log.h"
#include "assertions.h"
#include "msc.h"
#include "intertask_interface.h"
#include "gcc_diag.h"
#include "mme_app_itti_messaging.h"
#include "mme_config.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "mcc_mnc_itu.h"


//------------------------------------------------------------------------------
void mme_app_send_delete_session_request (struct ue_mm_context_s * const ue_context_p, const pdn_cid_t cid)
{
  MessageDef                             *message_p = NULL;

  message_p = itti_alloc_new_message (TASK_MME_APP, S11_DELETE_SESSION_REQUEST);
  AssertFatal (message_p , "itti_alloc_new_message Failed");
  S11_DELETE_SESSION_REQUEST (message_p).local_teid = ue_context_p->mme_teid_s11;
  S11_DELETE_SESSION_REQUEST (message_p).teid       = ue_context_p->pdn_contexts[cid]->s_gw_teid_s11_s4;
  S11_DELETE_SESSION_REQUEST (message_p).lbi        = 0; //default bearer

  OAI_GCC_DIAG_OFF(pointer-to-int-cast);
  S11_DELETE_SESSION_REQUEST (message_p).sender_fteid_for_cp.teid = (teid_t) ue_context_p;
  OAI_GCC_DIAG_ON(pointer-to-int-cast);
  S11_DELETE_SESSION_REQUEST (message_p).sender_fteid_for_cp.interface_type = S11_MME_GTP_C;
  mme_config_read_lock (&mme_config);
  S11_DELETE_SESSION_REQUEST (message_p).sender_fteid_for_cp.ipv4_address = mme_config.ipv4.s11;
  mme_config_unlock (&mme_config);
  S11_DELETE_SESSION_REQUEST (message_p).sender_fteid_for_cp.ipv4 = 1;

  /*
   * S11 stack specific parameter. Not used in standalone epc mode
   */
  S11_DELETE_SESSION_REQUEST  (message_p).trxn = NULL;
  mme_config_read_lock (&mme_config);
  S11_DELETE_SESSION_REQUEST (message_p).peer_ip = ue_context_p->pdn_contexts[cid]->s_gw_address_s11_s4.address.ipv4_address;
  mme_config_unlock (&mme_config);

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME,
                      NULL, 0, "0  S11_DELETE_SESSION_REQUEST teid %u lbi %u",
                      S11_DELETE_SESSION_REQUEST  (message_p).teid,
                      S11_DELETE_SESSION_REQUEST  (message_p).lbi);

  itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}


//------------------------------------------------------------------------------
void
mme_app_handle_detach_req (
  const itti_nas_detach_req_t * const detach_req_p)
{
  struct ue_mm_context_s *ue_context    = NULL;
  bool   sent_sgw = false;

  DevAssert(detach_req_p != NULL);
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, detach_req_p->ue_id);
  if (ue_context == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "UE context doesn't exist -> Nothing to do :-) \n");
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  else {
    for (pdn_cid_t cid = 0; cid < MAX_APN_PER_UE; cid++) {
      // No session with S-GW
      if (INVALID_TEID != ue_context->mme_teid_s11) {
        if (ue_context->pdn_contexts[cid]) {
          if (INVALID_TEID != ue_context->pdn_contexts[cid]->s_gw_teid_s11_s4) {
            // Send a DELETE_SESSION_REQUEST message to the SGW
            mme_app_send_delete_session_request  (ue_context, cid);
            sent_sgw = true;
            // CAROLE il vaut miex attendre de recevoir le delete session response pour effacer le contexte
            // mme_remove_ue_context(&mme_app_desc.mme_ue_contexts, ue_context);
          }
        }
      }
    }
    if (!sent_sgw) {
      // no session was created, no need for deleting session in SGW
      MessageDef *message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_UE_CONTEXT_RELEASE_COMMAND);
      AssertFatal (message_p , "itti_alloc_new_message Failed");
      S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).mme_ue_s1ap_id = ue_context->mme_ue_s1ap_id;
      S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).enb_ue_s1ap_id = ue_context->enb_ue_s1ap_id;
      MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0, "0 S1AP_UE_CONTEXT_RELEASE_COMMAND mme_ue_s1ap_id %06" PRIX32 " ",
          S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).mme_ue_s1ap_id);
      int to_task = (RUN_MODE_SCENARIO_PLAYER == mme_config.run_mode) ? TASK_MME_SCENARIO_PLAYER:TASK_S1AP;
      itti_send_msg_to_task (to_task, INSTANCE_DEFAULT, message_p);
    }
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}


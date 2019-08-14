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

/*! \file nas_mme_task.c
   \brief
   \author  Sebastien ROUX, Lionel GAUTHIER
   \date
   \email: lionel.gauthier@eurecom.fr
*/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emm_main.h"
#include "nas_emm.h"
#include "nas_emm_proc.h"
#include "nas_timer.h"
#include "bstrlib.h"

#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "common_defs.h"
#include "intertask_interface.h"
#include "itti_free_defined_msg.h"
#include "mme_config.h"
#include "nas_network.h"

static void nas_emm_exit(void);

//------------------------------------------------------------------------------
static void *nas_emm_intertask_interface (void *args_p)
{
  itti_mark_task_ready (TASK_NAS_EMM);

  while (1) {
    MessageDef                             *received_message_p = NULL;

    itti_receive_msg (TASK_NAS_EMM, &received_message_p);

    switch (ITTI_MSG_ID (received_message_p)) {
    case MESSAGE_TEST:{
        OAI_FPRINTF_INFO("TASK_NAS_EMM received MESSAGE_TEST\n");
      }
      break;

      /*
       * We don't need the S-TMSI: if with the given UE_ID we can find an EMM context, that means,
       * that a valid UE context could be matched for the UE context, and we can continue with it.
       */
    case NAS_INITIAL_UE_MESSAGE:{
          nas_establish_ind_t                    *nas_est_ind_p = NULL;
          nas_est_ind_p = &received_message_p->ittiMsg.nas_initial_ue_message.nas;
          nas_proc_establish_ind (nas_est_ind_p->ue_id,
              nas_est_ind_p->tai,
              nas_est_ind_p->ecgi,
              nas_est_ind_p->as_cause,
              &nas_est_ind_p->initial_nas_msg);
        }
        break;

    case NAS_DL_DATA_CNF:{
        nas_proc_dl_transfer_cnf (NAS_DL_DATA_CNF (received_message_p).ue_id, NAS_DL_DATA_CNF (received_message_p).err_code, &NAS_DL_DATA_REJ (received_message_p).nas_msg);
      }
      break;

    case NAS_UPLINK_DATA_IND:{
      nas_proc_ul_transfer_ind (NAS_UPLINK_DATA_IND (received_message_p).ue_id,
          NAS_UPLINK_DATA_IND (received_message_p).tai,
          NAS_UPLINK_DATA_IND (received_message_p).cgi,
          &NAS_UPLINK_DATA_IND (received_message_p).nas_msg);
      }
      break;

    case NAS_DL_DATA_REJ:{
        nas_proc_dl_transfer_rej (NAS_DL_DATA_REJ (received_message_p).ue_id, NAS_DL_DATA_REJ (received_message_p).err_code, &NAS_DL_DATA_REJ (received_message_p).nas_msg);
      }
      break;

    case NAS_IMPLICIT_DETACH_UE_IND:{
      nas_proc_implicit_detach_ue_ind (NAS_IMPLICIT_DETACH_UE_IND (received_message_p).ue_id, NAS_IMPLICIT_DETACH_UE_IND (received_message_p).emm_cause, NAS_IMPLICIT_DETACH_UE_IND (received_message_p).detach_type,
    		  NAS_IMPLICIT_DETACH_UE_IND (received_message_p).clr);
    }
    break;

    case S1AP_DEREGISTER_UE_REQ:{
        nas_proc_deregister_ue (S1AP_DEREGISTER_UE_REQ (received_message_p).mme_ue_s1ap_id);
      }
      break;

    case S6A_AUTH_INFO_ANS:{
        /*
         * We received the authentication vectors from HSS, trigger a ULR
         * for now. Normaly should trigger an authentication procedure with UE.
         */
        nas_proc_authentication_info_answer (&S6A_AUTH_INFO_ANS(received_message_p));
      }
      break;

    case NAS_CONTEXT_RES: {
      nas_proc_context_res(&NAS_CONTEXT_RES(received_message_p));
    }
    break;

    case NAS_CONTEXT_FAIL: {
      nas_proc_context_fail(NAS_CONTEXT_FAIL(received_message_p).ue_id, NAS_CONTEXT_FAIL(received_message_p).cause);
    }
    break;

    case NAS_SIGNALLING_CONNECTION_REL_IND:{
       nas_proc_signalling_connection_rel_ind (NAS_SIGNALLING_CONNECTION_REL_IND (received_message_p).ue_id);
    }
    break;

    case TERMINATE_MESSAGE:{
      nas_emm_exit();
      OAI_FPRINTF_INFO("TASK_NAS_EMM terminated\n");
      itti_free_msg_content(received_message_p);
      itti_free (ITTI_MSG_ORIGIN_ID (received_message_p), received_message_p);
      itti_exit_task ();
      }
      break;

    case TIMER_HAS_EXPIRED:{
        /*
         * Call the NAS timer api
         */
        nas_timer_handle_signal_expiry (TIMER_HAS_EXPIRED (received_message_p).timer_id, TIMER_HAS_EXPIRED (received_message_p).arg);
        free_wrapper((void**)&TIMER_HAS_EXPIRED (received_message_p).arg);
      }
      break;

    default:{
        OAILOG_DEBUG (LOG_NAS, "Unknown message ID %d:%s from %s\n", ITTI_MSG_ID (received_message_p), ITTI_MSG_NAME (received_message_p), ITTI_MSG_ORIGIN_NAME (received_message_p));
      }
      break;
    }

    itti_free_msg_content(received_message_p);
    itti_free (ITTI_MSG_ORIGIN_ID (received_message_p), received_message_p);
    received_message_p = NULL;

  }

  return NULL;
}

//------------------------------------------------------------------------------
int nas_emm_init (mme_config_t * mme_config_p)
{
  OAILOG_DEBUG (LOG_NAS, "Initializing NAS EMM task interface\n");
  nas_timer_init ();
  emm_main_initialize(mme_config_p);

  if (itti_create_task (TASK_NAS_EMM, &nas_emm_intertask_interface, NULL) < 0) {
    OAILOG_ERROR (LOG_NAS, "Create NAS EMM task failed");
    return -1;
  }

  OAILOG_DEBUG (LOG_NAS, "Initializing NAS EMM task interface: DONE\n");
  return 0;
}

//------------------------------------------------------------------------------
static void nas_emm_exit(void)
{
  OAILOG_DEBUG (LOG_NAS, "Cleaning NAS EMM task interface\n");
  emm_main_cleanup();
  nas_timer_cleanup();
  OAILOG_DEBUG (LOG_NAS, "Cleaning NAS EMM task interface: DONE\n");
}

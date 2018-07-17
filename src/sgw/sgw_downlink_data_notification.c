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
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>



#include "dynamic_memory_check.h"
#include "assertions.h"
#include "conversions.h"
#include "common_defs.h"
#include "intertask_interface.h"
#include "log.h"
#include "common_types.h"
#include "sgw_downlink_data_notification.h"
#include "sgw_context_manager.h"
#include "gtpv1_u_messages_types.h"
#include "sgw.h"
#include "ControllerMain.h"


#ifdef __cplusplus
extern "C" {
#endif

extern sgw_app_t                        sgw_app;


//------------------------------------------------------------------------------
int sgw_notify_downlink_data(const struct in_addr ue_ip, const ebi_t ebi)
{

  Gtpv1uDownlinkDataNotification     *gtpv1u_dl_data = NULL;
  MessageDef                             *message_p = NULL;

  // thread of OF controller
  if ((message_p = itti_alloc_new_message_sized (TASK_UNKNOWN, GTPV1U_DOWNLINK_DATA_NOTIFICATION, sizeof(Gtpv1uDownlinkDataNotification)))) {
    gtpv1u_dl_data = GTPV1U_DOWNLINK_DATA_NOTIFICATION(message_p);
    gtpv1u_dl_data->ue_ip = ue_ip;
    gtpv1u_dl_data->eps_bearer_id = ebi;

    int rv = itti_send_msg_to_task (TASK_SPGW_APP, INSTANCE_DEFAULT, message_p);
    return rv;
  }
  OAILOG_ERROR (LOG_SPGW_APP, "Failed to send GTPV1U_DOWNLINK_DATA_NOTIFICATION to task TASK_SPGW_APP\n");
  return RETURNerror;
}

//------------------------------------------------------------------------------
int
sgw_handle_s11_downlink_data_notification_ack (
  const itti_s11_downlink_data_notification_acknowledge_t * const ack)
{
  OAILOG_FUNC_IN(LOG_SPGW_APP);
  s_plus_p_gw_eps_bearer_context_information_t *bearer_ctxt_info_p = NULL;
  int rc = RETURNerror;

  // TODO procedure for DL DATA NOTIFICATION
  if (RETURNok == (rc = hashtable_ts_get (sgw_app.s11_bearer_context_information_hashtable, ack->teid, (void **)&bearer_ctxt_info_p))) {
    int bidx = 0;
    while ((NULL == bearer_ctxt_info_p->sgw_eps_bearer_context_information.pdn_connection.sgw_eps_bearers_array[bidx]) && (bidx < BEARERS_PER_UE)) {
      bidx++;
    }
    if (bidx< BEARERS_PER_UE) {
      paa_t paa = bearer_ctxt_info_p->sgw_eps_bearer_context_information.pdn_connection.sgw_eps_bearers_array[bidx]->paa;
      if ((IPv4 == paa.pdn_type) || (IPv4_AND_v6 == paa.pdn_type)) {
        switch (ack->cause.cause_value) {
        case REQUEST_ACCEPTED:
          openflow_controller_stop_dl_data_notification_ue(paa.ipv4_address, PAGING_CONFIRMED_CLAMPING_TIMEOUT_SEC);
          OAILOG_FUNC_RETURN(LOG_SPGW_APP, RETURNok);
          break;
        case TEMP_REJECT_HO_IN_PROGRESS:
        case UE_ALREADY_RE_ATTACHED:
          OAILOG_FUNC_RETURN(LOG_SPGW_APP, RETURNok);
          break;
        case UNABLE_TO_PAGE_UE:
        case CONTEXT_NOT_FOUND:
        case UNABLE_TO_PAGE_UE_DUE_TO_SUSPENSION:
          openflow_controller_stop_dl_data_notification_ue(paa.ipv4_address, PAGING_REJECTED_CLAMPING_TIMEOUT_SEC);
          OAILOG_FUNC_RETURN(LOG_SPGW_APP, RETURNok);
          break;
        default:
          OAILOG_NOTICE (LOG_SPGW_APP, "DL Data Notification Ack: Cause value not handled: %d\n", ack->cause.cause_value);
        }
      }
    }
  }
  OAILOG_DEBUG (LOG_SPGW_APP, "DL Data Notification Ack: Failed to get EPC Bearer Context Information\n");
  OAILOG_FUNC_RETURN(LOG_SPGW_APP, RETURNerror);
}

//------------------------------------------------------------------------------
int
sgw_handle_s11_downlink_data_notification_failure_ind (
  const itti_s11_downlink_data_notification_failure_indication_t * const ind)
{
  OAILOG_FUNC_IN(LOG_SPGW_APP);
  s_plus_p_gw_eps_bearer_context_information_t *bearer_ctxt_info_p = NULL;
  int rc = RETURNerror;

  // TODO procedure for DL DATA NOTIFICATION
  if (RETURNok == (rc = hashtable_ts_get (sgw_app.s11_bearer_context_information_hashtable, ind->teid, (void **)&bearer_ctxt_info_p))) {
    int bidx = 0;
    while ((NULL == bearer_ctxt_info_p->sgw_eps_bearer_context_information.pdn_connection.sgw_eps_bearers_array[bidx]) && (bidx < BEARERS_PER_UE)) {
      bidx++;
    }
    if (bidx< BEARERS_PER_UE) {
      paa_t paa = bearer_ctxt_info_p->sgw_eps_bearer_context_information.pdn_connection.sgw_eps_bearers_array[bidx]->paa;
      if ((IPv4 == paa.pdn_type) || (IPv4_AND_v6 == paa.pdn_type)) {
        switch (ind->cause.cause_value) {
        case SERVICE_DENIED:
          openflow_controller_stop_dl_data_notification_ue(paa.ipv4_address, PAGING_SERVICE_DENIED_CLAMPING_TIMEOUT_SEC);
          OAILOG_FUNC_RETURN(LOG_SPGW_APP, RETURNok);
          break;
        case UE_ALREADY_RE_ATTACHED:
          OAILOG_FUNC_RETURN(LOG_SPGW_APP, RETURNok);
          break;
        case UE_NOT_RESPONDING:
          openflow_controller_stop_dl_data_notification_ue(paa.ipv4_address, PAGING_UE_NOT_RESPONDING_CLAMPING_TIMEOUT_SEC);
          OAILOG_FUNC_RETURN(LOG_SPGW_APP, RETURNok);
          break;
        default:
          OAILOG_NOTICE (LOG_SPGW_APP, "DL Data Notification Failure Indication: Cause value not handled: %d\n", ind->cause.cause_value);
        }
      }
    }
  }
  OAILOG_DEBUG (LOG_SPGW_APP, "DL Data Notification Ack: Failed to get EPC Bearer Context Information\n");
  OAILOG_FUNC_RETURN(LOG_SPGW_APP, RETURNerror);
}


#ifdef __cplusplus
}
#endif

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
#include "gtpv1_u_messages_types.h"


#ifdef __cplusplus
extern "C" {
#endif



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


#ifdef __cplusplus
}
#endif

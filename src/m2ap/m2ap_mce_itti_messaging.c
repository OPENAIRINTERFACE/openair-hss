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

/*! \file m2ap_mce_itti_messaging.c
  \brief
  \author Dincer BEKEN
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "m2ap_common.h"
#include "bstrlib.h"
#include "m2ap_mce_itti_messaging.h"

#include "log.h"
#include "assertions.h"
#include "intertask_interface.h"


//------------------------------------------------------------------------------
int
m2ap_mce_itti_send_sctp_request (
  STOLEN_REF bstring *payload,
  const sctp_assoc_id_t assoc_id,
  const sctp_stream_id_t stream,
  const mce_mbms_m2ap_id_t mbms_id)
{
  MessageDef                             *message_p = NULL;

  message_p = itti_alloc_new_message (TASK_M2AP, SCTP_DATA_REQ);

  SCTP_DATA_REQ (message_p).payload = *payload;
  *payload = NULL;
  SCTP_DATA_REQ (message_p).assoc_id = assoc_id;
  SCTP_DATA_REQ (message_p).stream = stream;
// todo  SCTP_DATA_REQ (message_p).mce_mbms_m2ap_id = mbms_id;
  return itti_send_msg_to_task (TASK_SCTP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
void m2ap_mce_itti_m3ap_enb_setup_request(
  const sctp_assoc_id_t   assoc_id,
	const uint32_t          m2ap_enb_id,
  const mbms_service_area_t * const mbms_service_areas)
{
  MessageDef  *message_p = NULL;
  OAILOG_FUNC_IN (LOG_M2AP);
  message_p = itti_alloc_new_message(TASK_M2AP, M3AP_ENB_SETUP_REQUEST);
  M3AP_ENB_SETUP_REQUEST(message_p).sctp_assoc          = assoc_id;
  M3AP_ENB_SETUP_REQUEST(message_p).m2ap_enb_id         = m2ap_enb_id;
  memcpy((void*)&M3AP_ENB_SETUP_REQUEST(message_p).mbms_service_areas, (void*)mbms_service_areas, sizeof(mbms_service_area_t));
//  M2AP_SETUP_REQ(message_p).ecgi                   = *ecgi;
//  M2AP_SETUP_REQ(message_p).transparent.enb_ue_s1ap_id = enb_ue_s1ap_id;
//  M2AP_SETUP_REQ(message_p).transparent.e_utran_cgi    = *ecgi;

  itti_send_msg_to_task(TASK_MCE_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_S1AP);
}


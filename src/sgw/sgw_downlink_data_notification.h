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

/*! \file sgw_downlink_data_notification.h
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/
#ifndef FILE_SGW_DOWNLINK_DATA_NOTIFICATION_SEEN
#define FILE_SGW_DOWNLINK_DATA_NOTIFICATION_SEEN

#ifdef __cplusplus
extern "C" {
#endif
#include <netinet/in.h>
#include "3gpp_24.007.h"
#include "s11_messages_types.h"

#define PAGING_UNCONFIRMED_CLAMPING_TIMEOUT_SEC        6
#define PAGING_CONFIRMED_CLAMPING_TIMEOUT_SEC          60
#define PAGING_UE_NOT_RESPONDING_CLAMPING_TIMEOUT_SEC  30
#define PAGING_SERVICE_DENIED_CLAMPING_TIMEOUT_SEC     180
#define PAGING_REJECTED_CLAMPING_TIMEOUT_SEC           8192

int sgw_notify_downlink_data(const struct in_addr ue_ip, const ebi_t ebi);

int sgw_handle_s11_downlink_data_notification_ack (const itti_s11_downlink_data_notification_acknowledge_t * const ack);

int sgw_handle_s11_downlink_data_notification_failure_ind (const itti_s11_downlink_data_notification_failure_indication_t * const ind);


#ifdef __cplusplus
}
#endif

#endif /* FILE_SGW_DOWNLINK_DATA_NOTIFICATION_SEEN */

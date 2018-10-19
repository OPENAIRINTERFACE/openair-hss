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

/*! \file sgw_handler_gtpu.h
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_SGW_HANDLER_GTPU_SEEN
#define FILE_SGW_HANDLER_GTPU_SEEN

#include "gtpv1_u_messages_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int sgw_handle_gtpu_downlink_data_notification (const Gtpv1uDownlinkDataNotification * const gtpu_dl_data_notif);

#ifdef __cplusplus
}
#endif

#endif /* FILE_SGW_HANDLER_GTPU_SEEN */

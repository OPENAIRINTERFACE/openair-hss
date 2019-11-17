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

/*! \file m2ap_mme_itti_messaging.h
  \brief
  \author Dincer BEKEN
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#ifndef FILE_M2AP_MCE_ITTI_MESSAGING_SEEN
#define FILE_M2AP_MCE_ITTI_MESSAGING_SEEN

#include "common_defs.h"

//------------------------------------------------------------------------------
int m2ap_mce_itti_send_sctp_request(STOLEN_REF bstring *payload,
                                    const  uint32_t sctp_assoc_id_t,
                                    const sctp_stream_id_t stream,
                                    const mce_mbms_m2ap_id_t mbms_id);

//------------------------------------------------------------------------------
void m2ap_mce_itti_m3ap_enb_setup_request(
  const sctp_assoc_id_t   assoc_id,
  const uint32_t          m2ap_enb_id,
  const mbms_service_area_t * const mbms_service_areas);

#endif /* FILE_M2AP_MCE_ITTI_MESSAGING_SEEN */

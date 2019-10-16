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


#ifndef FILE_SM_IE_FORMATTER_SEEN
#define FILE_SM_IE_FORMATTER_SEEN

#include "../gtpv2-c/gtpv2c_ie_formatter/shared/gtpv2c_ie_formatter.h"
#include "3gpp_29.274.h"

nw_rc_t sm_tmgi_ie_get ( uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t * ieValue, void *arg);

nw_rc_t sm_mbms_session_duration_ie_get ( uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

nw_rc_t sm_mbms_service_area_ie_get ( uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

nw_rc_t sm_mbms_flow_identifier_ie_get(uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

nw_rc_t sm_mbms_ip_multicast_distribution_ie_get(uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

nw_rc_t sm_mbms_data_transfer_start_ie_get(uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

nw_rc_t sm_mbms_flags_ie_get(uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

#endif /* FILE_SM_IE_FORMATTER_SEEN */

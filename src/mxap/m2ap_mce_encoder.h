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


#include <stdint.h>

#ifndef FILE_M2AP_MCE_ENCODER_SEEN
#define FILE_M2AP_MCE_ENCODER_SEEN

//#include "M2AP_M2AP-PDU.h"
#include "m2ap_common.h"

int m2ap_mce_encode_pdu(M2AP_M2AP_PDU_t *pdu, uint8_t **buffer, uint32_t *len) __attribute__ ((warn_unused_result));

#endif /* FILE_M2AP_MCE_ENCODER_SEEN */

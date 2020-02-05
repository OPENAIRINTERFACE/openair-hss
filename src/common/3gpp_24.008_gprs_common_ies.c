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

/*! \file 3gpp_24.008_gprs_common_ies.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdbool.h>

#include "bstrlib.h"

#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "TLVDecoder.h"
#include "TLVEncoder.h"
#include "assertions.h"
#include "common_defs.h"
#include "log.h"

//******************************************************************************
// 10.5.7 GPRS Common information elements
//******************************************************************************

//------------------------------------------------------------------------------
// 10.5.7.3 GPRS Timer
//------------------------------------------------------------------------------

static const long _gprs_timer_unit[] = {2, 60, 360, 60, 60, 60, 60, 0};

//------------------------------------------------------------------------------
int decode_gprs_timer_ie(gprs_timer_t* gprstimer, uint8_t iei, uint8_t* buffer,
                         const uint32_t len) {
  int decoded = 0;

  if (iei > 0) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, GPRS_TIMER_IE_MIN_LENGTH, len);
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, GPRS_TIMER_IE_MIN_LENGTH - 1,
                                         len);
  }

  gprstimer->unit = (*(buffer + decoded) >> 5) & 0x7;
  gprstimer->timervalue = *(buffer + decoded) & 0x1f;
  decoded++;
  return decoded;
}

//------------------------------------------------------------------------------
int encode_gprs_timer_ie(gprs_timer_t* gprstimer, uint8_t iei, uint8_t* buffer,
                         const uint32_t len) {
  uint32_t encoded = 0;

  /*
   * Checking IEI and pointer
   */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, GPRS_TIMER_IE_MIN_LENGTH, len);

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) =
      0x00 | ((gprstimer->unit & 0x7) << 5) | (gprstimer->timervalue & 0x1f);
  encoded++;
  return encoded;
}

//------------------------------------------------------------------------------

long gprs_timer_value(gprs_timer_t* gprstimer) {
  return (gprstimer->timervalue * _gprs_timer_unit[gprstimer->unit]);
}

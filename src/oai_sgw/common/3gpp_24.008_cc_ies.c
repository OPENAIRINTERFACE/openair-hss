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

/*! \file 3gpp_24.008_cc_ies.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/


#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"

#include "log.h"
#include "common_defs.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "TLVDecoder.h"
#include "TLVEncoder.h"

#ifdef __cplusplus
extern "C" {
#endif
//******************************************************************************
// 10.5.4 Call control information elements.
//******************************************************************************

//------------------------------------------------------------------------------
// 10.5.4.32 Supported codec list
//------------------------------------------------------------------------------
int decode_supported_codec_list (
    supported_codec_list_t * supportedcodeclist,
    const bool iei_present,
    uint8_t * buffer,
    const uint32_t len)
{
  int                                     decode_result = 0;
  int                                     decoded = 0;
  uint8_t                                 ielen = 0;

  if (iei_present) {
    CHECK_IEI_DECODER (CC_SUPPORTED_CODEC_LIST_IE, *buffer);
    decoded++;
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);
  if ((decode_result = decode_bstring (supportedcodeclist,
      ielen,
      buffer + decoded,
      len - decoded)) < 0) {
    return decode_result;
  } else {
    decoded += decode_result;
  }
  return decoded;
}

//------------------------------------------------------------------------------
int encode_supported_codec_list (
    supported_codec_list_t * supportedcodeclist,
    const bool iei_present,
    uint8_t * buffer,
    const uint32_t len)
{
  uint8_t                                *lenPtr;
  uint32_t                                encoded = 0;
  uint32_t                                encode_result = 0;

  /*
   * Checking IEI and pointer
   */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, SUPPORTED_CODEC_LIST_IE_MIN_LENGTH, len);

  if (iei_present) {
    *buffer = CC_SUPPORTED_CODEC_LIST_IE;
    encoded++;
  }

  lenPtr = (buffer + encoded);
  encoded++;

  if ((encode_result = encode_bstring (*supportedcodeclist,
      buffer + encoded, len - encoded)) < 0)
    return encode_result;
  else
    encoded += encode_result;

  *lenPtr = encoded - 1 - ((iei_present) ? 1 : 0);
  return encoded;
}
#ifdef __cplusplus
}
#endif

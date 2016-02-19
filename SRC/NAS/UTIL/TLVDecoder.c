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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "TLVDecoder.h"
#include "log.h"

int                                     errorCodeDecoder = 0;

const char                             *errorCodeStringDecoder[] = {
  "No error",
  "Buffer NULL",
  "Buffer too short",
  "Unexpected IEI",
  "Mandatory field not present",
  "Wrong message type",
  "EXT value doesn't match",
  "Protocol not supported",
};

void
tlv_decode_perror (
  void)
{
  if (errorCodeDecoder >= 0)
    // No error or TLV_DECODE_ERR_OK
    return;

  LOG_ERROR (LOG_NAS, "TLV decoder : (%d, %s)", errorCodeDecoder, errorCodeStringDecoder[errorCodeDecoder * -1]);
}

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
#include "IMSIEncoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

namespace openflow {

uint64_t IMSIEncoder::compact_imsi(const std::string& imsi) {
  const char* imsi_cstr = imsi.c_str();
  uint32_t pad = 0;
  for (; pad < strlen(imsi_cstr); pad++) {
    if (imsi_cstr[pad] != '0') break;
  }
  uint64_t num = strtoull(imsi_cstr + pad, NULL, 10);
  return num << 2 | (pad & 0x3);
}

std::string IMSIEncoder::expand_imsi(uint64_t compact) {
  // log(MAX_UINT_64) + 2 leading zeros + null
  char buf[19 + 2 + 1];
  uint32_t pad = compact & 0x3;
  unsigned long long num = compact >> 2;
  for (uint32_t i = 0; i < pad; i++) {
    buf[i] = '0';
  }
  sprintf(buf + pad, "%llu", num);
  return std::string(buf);
}

}  // namespace openflow

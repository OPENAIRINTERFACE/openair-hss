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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "ApnAggregateMaximumBitRate.h"
#include "TLVDecoder.h"
#include "TLVEncoder.h"

//------------------------------------------------------------------------------
int decode_apn_aggregate_maximum_bit_rate(
    ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, uint8_t iei,
    uint8_t *buffer, uint32_t len) {
  int decoded = 0;
  uint8_t ielen = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER(len - decoded, ielen);
  apnaggregatemaximumbitrate->apnambrfordownlink = *(buffer + decoded);
  decoded++;
  apnaggregatemaximumbitrate->apnambrforuplink = *(buffer + decoded);
  decoded++;

  if (ielen >= 4) {
    apnaggregatemaximumbitrate->apnambrfordownlink_extended =
        *(buffer + decoded);
    decoded++;
    apnaggregatemaximumbitrate->apnambrforuplink_extended = *(buffer + decoded);
    decoded++;

    if (ielen >= 6) {
      apnaggregatemaximumbitrate->apnambrfordownlink_extended2 =
          *(buffer + decoded);
      decoded++;
      apnaggregatemaximumbitrate->apnambrforuplink_extended2 =
          *(buffer + decoded);
      decoded++;
    }
  }
  return decoded;
}

//------------------------------------------------------------------------------
int encode_apn_aggregate_maximum_bit_rate(
    ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, uint8_t iei,
    uint8_t *buffer, uint32_t len) {
  uint8_t *lenPtr;
  uint32_t encoded = 0;

  /*
   * Checking IEI and pointer
   */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(
      buffer, APN_AGGREGATE_MAXIMUM_BIT_RATE_MINIMUM_LENGTH, len);

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr = (buffer + encoded);
  encoded++;
  *(buffer + encoded) = apnaggregatemaximumbitrate->apnambrfordownlink;
  encoded++;
  *(buffer + encoded) = apnaggregatemaximumbitrate->apnambrforuplink;
  encoded++;

  if (apnaggregatemaximumbitrate->extensions &
      APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION_PRESENT) {
    *(buffer + encoded) =
        apnaggregatemaximumbitrate->apnambrfordownlink_extended;
    encoded++;
    *(buffer + encoded) = apnaggregatemaximumbitrate->apnambrforuplink_extended;
    encoded++;

    if (apnaggregatemaximumbitrate->extensions &
        APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION2_PRESENT) {
      *(buffer + encoded) =
          apnaggregatemaximumbitrate->apnambrfordownlink_extended2;
      encoded++;
      *(buffer + encoded) =
          apnaggregatemaximumbitrate->apnambrforuplink_extended2;
      encoded++;
    }
  }

  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
  return encoded;
}

//------------------------------------------------------------------------------
int ambr_kbps_calc(ApnAggregateMaximumBitRate *apnambr, uint64_t kbr_dl,
                   uint64_t kbr_ul) {
  // if someone volunteer for subroutines..., no time yet.
  memset(apnambr, 0, sizeof(ApnAggregateMaximumBitRate));
  /** Set downlink bitrate. */

  if (kbr_dl == 0) {
    apnambr->apnambrfordownlink = 0xff;
  } else if ((kbr_dl > 0) && (kbr_dl <= 63)) {
    apnambr->apnambrfordownlink = kbr_dl;
  } else if ((kbr_dl > 63) && (kbr_dl <= 568)) {
    apnambr->apnambrfordownlink = ((kbr_dl - 64) / 8) + 64;
  } else if ((kbr_dl >= 576) && (kbr_dl <= 8640)) {
    apnambr->apnambrfordownlink = ((kbr_dl - 128) / 64) + 128;
  } else if (kbr_dl > 8640 && kbr_dl <= 256000) {
    apnambr->apnambrfordownlink = 0xfe;
    apnambr->extensions =
        0 | APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION_PRESENT;
    if ((kbr_dl >= 8600) && (kbr_dl <= 16000)) {
      apnambr->apnambrfordownlink_extended = (kbr_dl - 8600) / 100;
    } else if ((kbr_dl > 16000) && (kbr_dl <= 128000)) {
      apnambr->apnambrfordownlink_extended = ((kbr_dl - 16000) / 1000) + 74;
    } else if ((kbr_dl > 128000) && (kbr_dl <= 256000)) {
      apnambr->apnambrfordownlink_extended = ((kbr_dl - 128000) / 2000) + 186;
    }
  } else if (kbr_dl > 256000 && kbr_dl <= 65280000) {
    apnambr->apnambrfordownlink = 0xfe;
    apnambr->apnambrfordownlink_extended = 0xfa;
    apnambr->apnambrfordownlink_extended2 = (kbr_dl - 256000) / 256000;
    apnambr->extensions =
        0 | APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION_PRESENT;
    apnambr->extensions =
        0 | APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION2_PRESENT;
  } else {
    /** Leaving empty (error). */
    return RETURNerror;
  }

  /** Set uplink bitrate. */
  if (kbr_ul == 0) {
    apnambr->apnambrforuplink = 0xff;
  } else if ((kbr_ul > 0) && (kbr_ul <= 63)) {
    apnambr->apnambrforuplink = kbr_ul;
  } else if ((kbr_ul > 63) && (kbr_ul <= 568)) {
    apnambr->apnambrforuplink = ((kbr_ul - 64) / 8) + 64;
  } else if ((kbr_ul >= 576) && (kbr_ul <= 8640)) {
    apnambr->apnambrforuplink = ((kbr_ul - 128) / 64) + 128;
  } else if (kbr_ul > 8640 && kbr_ul <= 256000) {
    apnambr->apnambrforuplink = 0xfe;
    apnambr->extensions =
        0 | APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION_PRESENT;
    if ((kbr_ul >= 8600) && (kbr_ul <= 16000)) {
      apnambr->apnambrforuplink_extended = (kbr_ul - 8600) / 100;
    } else if ((kbr_ul > 16000) && (kbr_ul <= 128000)) {
      apnambr->apnambrforuplink_extended = ((kbr_ul - 16000) / 1000) + 74;
    } else if ((kbr_ul > 128000) && (kbr_ul <= 256000)) {
      apnambr->apnambrforuplink_extended = ((kbr_ul - 128000) / 2000) + 186;
    }
  } else if (kbr_ul > 256000 && kbr_ul <= 65280000) {
    apnambr->apnambrforuplink = 0xfe;
    apnambr->apnambrforuplink_extended = 0xfa;
    apnambr->apnambrforuplink_extended2 = (kbr_ul - 256000) / 256000;
    apnambr->extensions =
        0 | APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION_PRESENT;
    apnambr->extensions =
        0 | APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION2_PRESENT;
  } else {
    /** Leaving empty (error). */
    return RETURNerror;
  }

  return RETURNok;
}

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

/*! \file conversions.c
  \brief
  \author Sebastien ROUX
  \company Eurecom
*/
#include "assertions.h"
#include "common_defs.h"
#include "conversions.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

static const char                       hex_to_ascii_table[16] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

static const signed char                ascii_to_hex_table[0x100] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

void
hexa_to_ascii (
  uint8_t * from,
  char *to,
  size_t length)
{
  size_t                                 i;

  for (i = 0; i < length; i++) {
    uint8_t                                 upper = (from[i] & 0xf0) >> 4;
    uint8_t                                 lower = from[i] & 0x0f;

    to[2 * i] = hex_to_ascii_table[upper];
    to[2 * i + 1] = hex_to_ascii_table[lower];
  }
}

int
ascii_to_hex (
  uint8_t * dst,
  const char *h)
{
  const unsigned char                    *hex = (const unsigned char *)h;
  unsigned                                i = 0;

  for (;;) {
    int                                     high,
                                            low;

    while (*hex && isspace (*hex))
      hex++;

    if (!*hex)
      return 1;

    high = ascii_to_hex_table[*hex++];

    if (high < 0)
      return 0;

    while (*hex && isspace (*hex))
      hex++;

    if (!*hex)
      return 0;

    low = ascii_to_hex_table[*hex++];

    if (low < 0)
      return 0;

    dst[i++] = (high << 4) | low;
  }
}
//------------------------------------------------------------------------------
imsi64_t imsi_to_imsi64(const imsi_t * const imsi)
{
  imsi64_t imsi64 = INVALID_IMSI64;
  if (imsi) {
    imsi64 = 0;
    for (int i=0; i < IMSI_BCD8_SIZE; i++) {
      uint8_t d2 = imsi->u.value[i];
      uint8_t d1 = (d2 & 0xf0) >> 4;
      d2 = d2 & 0x0f;
      if (10 > d1) {
        imsi64 = imsi64*10 + d1;
        if (10 > d2) {
          imsi64 = imsi64*10 + d2;
        } else {
          break;
        }
      } else {
        break;
      }
    }
  }
  return imsi64;
}

//------------------------------------------------------------------------------
bstring fteid_ip_address_to_bstring(const struct fteid_s * const fteid)
{
  bstring bstr = NULL;
  if (fteid->ipv4) {
    bstr = blk2bstr(&fteid->ipv4_address.s_addr, 4);
  } else if (fteid->ipv6) {
    bstr = blk2bstr(&fteid->ipv6_address, 16);
  } else {
    char avoid_seg_fault[4] = {0,0,0,0};
    bstr = blk2bstr(avoid_seg_fault, 4);
  }
  return bstr;
}

//------------------------------------------------------------------------------
bstring ip_address_to_bstring(ip_address_t *ip_address)
{
  bstring bstr = NULL;
  if (ip_address->ipv4) {
    bstr = blk2bstr(&ip_address->address.ipv4_address.s_addr, 4);
  } else if (ip_address->ipv6) {
    bstr = blk2bstr(&ip_address->address.ipv6_address, 16);
  }
  return bstr;
}

//------------------------------------------------------------------------------
void bstring_to_ip_address(bstring const bstr, ip_address_t * const ip_address)
{
  if (bstr) {
    ip_address->ipv4 = false;
    ip_address->ipv6 = false;
    switch (blength(bstr)) {
    case 4:
      ip_address->ipv4 = true;
      memcpy(&ip_address->address.ipv4_address, bstr->data, blength(bstr));
      break;
    case 16:
      ip_address->ipv6 = true;
      memcpy(&ip_address->address.ipv6_address, bstr->data, blength(bstr));
      break;
    default:
      ;
    }
  }
}

//------------------------------------------------------------------------------
bstring paa_to_bstring(paa_t *paa)
{
  bstring bstr = NULL;
  switch (paa->pdn_type) {
  case IPv4:
    bstr = blk2bstr(&paa->ipv4_address.s_addr, 4);
    break;
  case IPv6:
    DevAssert (paa->ipv6_prefix_length == 64);    // NAS seems to only support 64 bits
    bstr = blk2bstr(&paa->ipv6_address, paa->ipv6_prefix_length / 8);
    break;
  case IPv4_AND_v6:
    DevAssert (paa->ipv6_prefix_length == 64);    // NAS seems to only support 64 bits
    bstr = blk2bstr(&paa->ipv4_address.s_addr, 4);
    bcatblk(bstr, &paa->ipv6_address, paa->ipv6_prefix_length / 8);
    break;
  default:
    ;
  }
  return bstr;
}

//------------------------------------------------------------------------------
void copy_paa(paa_t *paa_dst, paa_t *paa_src)
{
  memcpy(paa_dst, paa_src, sizeof(paa_t));
}

#ifdef __cplusplus
}
#endif

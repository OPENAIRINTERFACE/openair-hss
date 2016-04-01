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
#include <stdint.h>

#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "ProtocolConfigurationOptions.h"

int
decode_ProtocolConfigurationOptions (
  ProtocolConfigurationOptions * protocolconfigurationoptions,
  const uint8_t iei,
  const uint8_t * const buffer,
  const uint32_t len)
{
  uint32_t                                decoded = 0;
  uint8_t                                 ielen = 0;
  int                                     decode_result;

  if (iei > 0) {
    CHECK_IEI_DECODER (iei, *buffer);
    decoded++;
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);

  if (((*(buffer + decoded) >> 7) & 0x1) != 1) {
    errorCodeDecoder = TLV_VALUE_DOESNT_MATCH;
    return TLV_VALUE_DOESNT_MATCH;
  }

  /*
   * Bits 7 to 4 of octet 3 are spare, read as 0
   */
  if (((*(buffer + decoded) & 0x78) >> 3) != 0) {
    errorCodeDecoder = TLV_VALUE_DOESNT_MATCH;
    return TLV_VALUE_DOESNT_MATCH;
  }

  protocolconfigurationoptions->configuration_protocol = (*(buffer + decoded) >> 1) & 0x7;
  decoded++;
  //IES_DECODE_U16(protocolconfigurationoptions->protocolid, *(buffer + decoded));
  protocolconfigurationoptions->num_protocol_or_container_id = 0;

  while ((len - decoded) > 0) {
    IES_DECODE_U16 (buffer, decoded, protocolconfigurationoptions->protocol_or_container_ids[protocolconfigurationoptions->num_protocol_or_container_id].id);
    DECODE_U8 (buffer + decoded, protocolconfigurationoptions->protocol_or_container_ids[protocolconfigurationoptions->num_protocol_or_container_id].length, decoded);

    if (0 < protocolconfigurationoptions->protocol_or_container_ids[protocolconfigurationoptions->num_protocol_or_container_id].length) {
      if ((decode_result = decode_bstring (&protocolconfigurationoptions->protocol_or_container_ids[protocolconfigurationoptions->num_protocol_or_container_id].contents,
          protocolconfigurationoptions->protocol_or_container_ids[protocolconfigurationoptions->num_protocol_or_container_id].length,
          buffer + decoded,
          len - decoded)) < 0) {
        return decode_result;
      } else {
        decoded += decode_result;
      }
    } else {
      protocolconfigurationoptions->protocol_or_container_ids[protocolconfigurationoptions->num_protocol_or_container_id].contents = NULL;
    }
    protocolconfigurationoptions->num_protocol_or_container_id += 1;
  }

#if NAS_DEBUG
  dump_ProtocolConfigurationOptions_xml (protocolconfigurationoptions, iei);
#endif
  return decoded;
}

int
encode_ProtocolConfigurationOptions (
  ProtocolConfigurationOptions * protocolconfigurationoptions,
  uint8_t iei,
  uint8_t * buffer,
  uint32_t len)
{
  uint8_t                                *lenPtr;
  uint8_t                                 num_protocol_or_container_id = 0;
  uint32_t                                encoded = 0;
  int                                     encode_result;

  /*
   * Checking IEI and pointer
   */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, PROTOCOL_CONFIGURATION_OPTIONS_MINIMUM_LENGTH, len);
#if NAS_DEBUG
  dump_ProtocolConfigurationOptions_xml (protocolconfigurationoptions, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr = (buffer + encoded);
  encoded++;
  *(buffer + encoded) = 0x00 | (1 << 7) | (protocolconfigurationoptions->configuration_protocol & 0x7);
  encoded++;

  while (num_protocol_or_container_id < protocolconfigurationoptions->num_protocol_or_container_id) {
    IES_ENCODE_U16 (buffer, encoded, protocolconfigurationoptions->protocol_or_container_ids[num_protocol_or_container_id].id);
    *(buffer + encoded) = protocolconfigurationoptions->protocol_or_container_ids[num_protocol_or_container_id].length;
    encoded++;

    if ((encode_result = encode_bstring (protocolconfigurationoptions->protocol_or_container_ids[num_protocol_or_container_id].contents,
        buffer + encoded, len - encoded)) < 0)
      return encode_result;
    else
      encoded += encode_result;

    num_protocol_or_container_id += 1;
  }

  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
  return encoded;
}

void
dump_ProtocolConfigurationOptions_xml (
  ProtocolConfigurationOptions * protocolconfigurationoptions,
  uint8_t iei)
{
  int                                     i;

  OAILOG_DEBUG (LOG_NAS, "<Protocol Configuration Options>\n");

  if (iei > 0)
    /*
     * Don't display IEI if = 0
     */
    OAILOG_DEBUG (LOG_NAS, "    <IEI>0x%X</IEI>\n", iei);

  OAILOG_DEBUG (LOG_NAS, "    <Configuration protol>%u</Configuration protol>\n", protocolconfigurationoptions->configuration_protocol);
  i = 0;

  while (i < protocolconfigurationoptions->num_protocol_or_container_id) {
    OAILOG_DEBUG (LOG_NAS, "        <Protocol ID>%u</Protocol ID>\n", protocolconfigurationoptions->protocol_or_container_ids[i].id);
    OAILOG_DEBUG (LOG_NAS, "        <Length of protocol ID>%u</Length of protocol ID>\n", protocolconfigurationoptions->protocol_or_container_ids[i].length);
    bstring b = dump_bstring_xml (protocolconfigurationoptions->protocol_or_container_ids[i].contents);
    OAILOG_DEBUG (LOG_NAS, "        %s", bdata(b));
    bdestroy(b);
    i++;
  }

  OAILOG_DEBUG (LOG_NAS, "</Protocol Configuration Options>\n");
}

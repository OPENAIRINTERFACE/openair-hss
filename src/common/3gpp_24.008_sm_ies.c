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

/*! \file 3gpp_24.008_sm_ies.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>

#include "bstrlib.h"

#include "log.h"
#include "dynamic_memory_check.h"
#include "common_defs.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_24.301.h"
#include "TLVDecoder.h"
#include "TLVEncoder.h"


//******************************************************************************
// 10.5.6 Session management information elements
//******************************************************************************
//------------------------------------------------------------------------------
// 10.5.6.1 Access Point Name
//------------------------------------------------------------------------------
int decode_access_point_name_ie (
  access_point_name_t * access_point_name,
  bool is_ie_present,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;
  uint8_t                                 ielen = 0;

  *access_point_name = NULL;

  if (is_ie_present > 0) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, ACCESS_POINT_NAME_IE_MIN_LENGTH, len);
    CHECK_IEI_DECODER (SM_ACCESS_POINT_NAME_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (ACCESS_POINT_NAME_IE_MIN_LENGTH - 1), len);
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);

  if (1 < ielen) {
    int length_apn = *(buffer + decoded);
    decoded++;
    *access_point_name = blk2bstr((void *)(buffer + decoded), length_apn);
    decoded += length_apn;
    ielen = ielen - 1 - length_apn;
    while (1 <= ielen) {
      bconchar(*access_point_name, '.');
      length_apn = *(buffer + decoded);
      decoded++;
      ielen = ielen - 1;

      // apn terminated by '.' ?
      if (length_apn > 0) {
        AssertFatal(ielen >= length_apn, "Mismatch in lengths remaining ielen %d apn length %d", ielen, length_apn);
        bcatblk(*access_point_name, (void *)(buffer + decoded), length_apn);
        decoded += length_apn;
        ielen = ielen - length_apn;
      }
    }
  }
  return decoded;
}

//------------------------------------------------------------------------------
int encode_access_point_name_ie (
  access_point_name_t access_point_name,
  bool is_ie_present,
  uint8_t * buffer,
  const uint32_t len)
{
  uint8_t                                *lenPtr = NULL;
  uint32_t                                encoded = 0;
  int                                     encode_result = 0;
  uint32_t                                length_index = 0;
  uint32_t                                index = 0;
  uint32_t                                index_copy = 0;
  uint8_t                                 apn_encoded[ACCESS_POINT_NAME_IE_MAX_LENGTH] = {0};


  if (is_ie_present > 0) {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, ACCESS_POINT_NAME_IE_MAX_LENGTH, len);
    *buffer = SM_ACCESS_POINT_NAME_IEI;
    encoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (ACCESS_POINT_NAME_IE_MIN_LENGTH - 1), len);
  }

  lenPtr = (buffer + encoded);
  encoded++;
  index = 0;                    // index on original APN string
  length_index = 0;             // marker where to write partial length
  index_copy = 1;

  while ((access_point_name->data[index] != 0) && (index < access_point_name->slen)) {
    if (access_point_name->data[index] == '.') {
      apn_encoded[length_index] = index_copy - length_index - 1;
      length_index = index_copy;
      index_copy = length_index + 1;
    } else {
      apn_encoded[index_copy] = access_point_name->data[index];
      index_copy++;
    }

    index++;
  }

  apn_encoded[length_index] = index_copy - length_index - 1;
  bstring bapn = blk2bstr(apn_encoded, index_copy);

  if ((encode_result = encode_bstring (bapn, buffer + encoded, len - encoded)) < 0) {
    bdestroy_wrapper (&bapn);
    return encode_result;
  } else {
    encoded += encode_result;
  }
  bdestroy_wrapper (&bapn);
  *lenPtr = encoded - 1 - ((is_ie_present) ? 1 : 0);
  return encoded;
}

//------------------------------------------------------------------------------
// 10.5.6.3 Protocol configuration options
//------------------------------------------------------------------------------
void copy_protocol_configuration_options (protocol_configuration_options_t * const pco_dst, const protocol_configuration_options_t * const pco_src)
{
  if ((pco_dst) && (pco_src)) {
    pco_dst->ext = pco_src->ext;
    pco_dst->spare = pco_src->spare;
    pco_dst->configuration_protocol = pco_src->configuration_protocol;
    pco_dst->num_protocol_or_container_id = pco_src->num_protocol_or_container_id;
    AssertFatal(PCO_UNSPEC_MAXIMUM_PROTOCOL_ID_OR_CONTAINER_ID >= pco_dst->num_protocol_or_container_id,
        "Invalid number of protocol_or_container_id %d", pco_dst->num_protocol_or_container_id);
    for (int i = 0; i < pco_src->num_protocol_or_container_id; i++) {
      pco_dst->protocol_or_container_ids[i].id     = pco_src->protocol_or_container_ids[i].id;
      pco_dst->protocol_or_container_ids[i].length = pco_src->protocol_or_container_ids[i].length;
      pco_dst->protocol_or_container_ids[i].contents = bstrcpy(pco_src->protocol_or_container_ids[i].contents);
    }
  }
}

//------------------------------------------------------------------------------
void clear_protocol_configuration_options (protocol_configuration_options_t * const pco)
{
  if (pco) {
    for (int i = 0; i < PCO_UNSPEC_MAXIMUM_PROTOCOL_ID_OR_CONTAINER_ID; i++) {
      if (pco->protocol_or_container_ids[i].contents) {
        bdestroy_wrapper (&pco->protocol_or_container_ids[i].contents);
      }
    }
    memset(pco, 0, sizeof(protocol_configuration_options_t));
  }
}

//------------------------------------------------------------------------------
void free_protocol_configuration_options (protocol_configuration_options_t ** const protocol_configuration_options)
{
  protocol_configuration_options_t *pco = *protocol_configuration_options;
  if (pco) {
    for (int i = 0; i < PCO_UNSPEC_MAXIMUM_PROTOCOL_ID_OR_CONTAINER_ID; i++) {
      if (pco->protocol_or_container_ids[i].contents) {
        bdestroy_wrapper (&pco->protocol_or_container_ids[i].contents);
      }
    }
    free_wrapper((void**)protocol_configuration_options);
  }
}

//------------------------------------------------------------------------------
int
decode_protocol_configuration_options (
    protocol_configuration_options_t * protocolconfigurationoptions,
    const uint8_t * const buffer,
    const uint32_t len)
{
  int                                     decoded = 0;
  int                                     decode_result = 0;


  if (((*(buffer + decoded) >> 7) & 0x1) != 1) {
    return TLV_VALUE_DOESNT_MATCH;
  }

  /*
   * Bits 7 to 4 of octet 3 are spare, read as 0
   */
  if (((*(buffer + decoded) & 0x78) >> 3) != 0) {
    return TLV_VALUE_DOESNT_MATCH;
  }

  protocolconfigurationoptions->configuration_protocol = (*(buffer + decoded) >> 1) & 0x7;
  decoded++;
  protocolconfigurationoptions->num_protocol_or_container_id = 0;

  while (3 <= ((int32_t)len - (int32_t)decoded)) {
    DECODE_U16 (buffer + decoded, protocolconfigurationoptions->protocol_or_container_ids[protocolconfigurationoptions->num_protocol_or_container_id].id, decoded);
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

  return decoded;
}
//------------------------------------------------------------------------------
int
decode_protocol_configuration_options_ie (
    protocol_configuration_options_t * protocolconfigurationoptions,
    const bool iei_present,
    const uint8_t * const buffer,
    const uint32_t len)
{
  int                                     decoded = 0;
  int                                     decoded2 = 0;
  uint8_t                                 ielen = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, PROTOCOL_CONFIGURATION_OPTIONS_IE_MIN_LENGTH, len);
    CHECK_IEI_DECODER (SM_PROTOCOL_CONFIGURATION_OPTIONS_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (PROTOCOL_CONFIGURATION_OPTIONS_IE_MIN_LENGTH - 1), len);
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);

  decoded2 = decode_protocol_configuration_options(protocolconfigurationoptions, buffer + decoded, len - decoded);
  if (decoded2 < 0) return decoded2;
  return decoded+decoded2;
}
//------------------------------------------------------------------------------
int
encode_protocol_configuration_options (
    const protocol_configuration_options_t * const protocolconfigurationoptions,
    uint8_t * buffer,
    const uint32_t len)
{
  uint8_t                                 num_protocol_or_container_id = 0;
  uint32_t                                encoded = 0;
  int                                     encode_result = 0;

  *(buffer + encoded) = 0x00 | (1 << 7) | (protocolconfigurationoptions->configuration_protocol & 0x7);
  encoded++;

  while (num_protocol_or_container_id < protocolconfigurationoptions->num_protocol_or_container_id) {
    ENCODE_U16 (buffer + encoded, protocolconfigurationoptions->protocol_or_container_ids[num_protocol_or_container_id].id, encoded);
    *(buffer + encoded) = protocolconfigurationoptions->protocol_or_container_ids[num_protocol_or_container_id].length;
    encoded++;

    if ((encode_result = encode_bstring (protocolconfigurationoptions->protocol_or_container_ids[num_protocol_or_container_id].contents,
        buffer + encoded, len - encoded)) < 0)
      return encode_result;
    else
      encoded += encode_result;

    num_protocol_or_container_id += 1;
  }
  return encoded;
}

//------------------------------------------------------------------------------
int
encode_protocol_configuration_options_ie (
    const protocol_configuration_options_t * const protocolconfigurationoptions,
    const bool iei_present,
    uint8_t * buffer,
    const uint32_t len)
{
  uint8_t                                *lenPtr = NULL;
  uint32_t                                encoded = 0;

 if (iei_present) {
   CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, PROTOCOL_CONFIGURATION_OPTIONS_IE_MIN_LENGTH, len);
   *buffer = SM_PROTOCOL_CONFIGURATION_OPTIONS_IEI;
   encoded++;
 } else {
   CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (PROTOCOL_CONFIGURATION_OPTIONS_IE_MIN_LENGTH - 1), len);
 }

 lenPtr = (buffer + encoded);
 encoded++;

 encoded += encode_protocol_configuration_options(protocolconfigurationoptions, buffer + encoded, len - encoded);

  *lenPtr = encoded - 1 - ((iei_present) ? 1 : 0);
  return encoded;
}

//------------------------------------------------------------------------------
// 10.5.6.5 Quality of service
//------------------------------------------------------------------------------
int decode_quality_of_service_ie (
  quality_of_service_t * qualityofservice,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;
  uint8_t                                 ielen = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, QUALITY_OF_SERVICE_IE_MIN_LENGTH, len);
    CHECK_IEI_DECODER (SM_QUALITY_OF_SERVICE_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (QUALITY_OF_SERVICE_IE_MIN_LENGTH - 1), len);
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);
  qualityofservice->delayclass = (*(buffer + decoded) >> 3) & 0x7;
  qualityofservice->reliabilityclass = *(buffer + decoded) & 0x7;
  decoded++;
  qualityofservice->peakthroughput = (*(buffer + decoded) >> 4) & 0xf;
  qualityofservice->precedenceclass = *(buffer + decoded) & 0x7;
  decoded++;
  qualityofservice->meanthroughput = *(buffer + decoded) & 0x1f;
  decoded++;
  qualityofservice->trafficclass = (*(buffer + decoded) >> 5) & 0x7;
  qualityofservice->deliveryorder = (*(buffer + decoded) >> 3) & 0x3;
  qualityofservice->deliveryoferroneoussdu = *(buffer + decoded) & 0x7;
  decoded++;
  qualityofservice->maximumsdusize = *(buffer + decoded);
  decoded++;
  qualityofservice->maximumbitrateuplink = *(buffer + decoded);
  decoded++;
  qualityofservice->maximumbitratedownlink = *(buffer + decoded);
  decoded++;
  qualityofservice->residualber = (*(buffer + decoded) >> 4) & 0xf;
  qualityofservice->sduratioerror = *(buffer + decoded) & 0xf;
  decoded++;
  qualityofservice->transferdelay = (*(buffer + decoded) >> 2) & 0x3f;
  qualityofservice->traffichandlingpriority = *(buffer + decoded) & 0x3;
  decoded++;
  qualityofservice->guaranteedbitrateuplink = *(buffer + decoded);
  decoded++;
  qualityofservice->guaranteedbitratedownlink = *(buffer + decoded);
  decoded++;
  qualityofservice->signalingindication = (*(buffer + decoded) >> 4) & 0x1;
  qualityofservice->sourcestatisticsdescriptor = *(buffer + decoded) & 0xf;
  decoded++;
  return decoded;
}

//------------------------------------------------------------------------------
int encode_quality_of_service_ie (
  quality_of_service_t * qualityofservice,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  uint8_t                                *lenPtr;
  uint32_t                                encoded = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, QUALITY_OF_SERVICE_IE_MIN_LENGTH, len);
    *buffer = SM_QUALITY_OF_SERVICE_IEI;
    encoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (QUALITY_OF_SERVICE_IE_MIN_LENGTH - 1), len);
  }

  lenPtr = (buffer + encoded);
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->delayclass & 0x7) << 3) | (qualityofservice->reliabilityclass & 0x7);
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->peakthroughput & 0xf) << 4) | (qualityofservice->precedenceclass & 0x7);
  encoded++;
  *(buffer + encoded) = 0x00 | (qualityofservice->meanthroughput & 0x1f);
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->trafficclass & 0x7) << 5) | ((qualityofservice->deliveryorder & 0x3) << 3) | (qualityofservice->deliveryoferroneoussdu & 0x7);
  encoded++;
  *(buffer + encoded) = qualityofservice->maximumsdusize;
  encoded++;
  *(buffer + encoded) = qualityofservice->maximumbitrateuplink;
  encoded++;
  *(buffer + encoded) = qualityofservice->maximumbitratedownlink;
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->residualber & 0xf) << 4) | (qualityofservice->sduratioerror & 0xf);
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->transferdelay & 0x3f) << 2) | (qualityofservice->traffichandlingpriority & 0x3);
  encoded++;
  *(buffer + encoded) = qualityofservice->guaranteedbitrateuplink;
  encoded++;
  *(buffer + encoded) = qualityofservice->guaranteedbitratedownlink;
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->signalingindication & 0x1) << 4) | (qualityofservice->sourcestatisticsdescriptor & 0xf);
  encoded++;
  *lenPtr = encoded - 1 - ((iei_present) ? 1 : 0);
  return encoded;
}

//------------------------------------------------------------------------------
// 10.5.6.7 Linked TI
//------------------------------------------------------------------------------
int encode_linked_ti_ie(linked_ti_t *linkedti, const bool iei_present, uint8_t *buffer, const uint32_t len)
{
  AssertFatal(0, "TODO");
  return 0;
}

int decode_linked_ti_ie(linked_ti_t *linkedti, const bool iei_present, uint8_t *buffer, const uint32_t len)
{
  AssertFatal(0, "TODO");
  return 0;
}

//------------------------------------------------------------------------------
// 10.5.6.9 LLC service access point identifier
//------------------------------------------------------------------------------
int decode_llc_service_access_point_identifier_ie (
  llc_service_access_point_identifier_t * llc_sap_id,
  bool is_ie_present,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;

  if (is_ie_present) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IE_MAX_LENGTH, len);
    CHECK_IEI_DECODER (SM_LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IE_MAX_LENGTH - 1), len);
  }

  *llc_sap_id = *buffer & 0xf;
  decoded++;
  return decoded;
}

//------------------------------------------------------------------------------
int encode_llc_service_access_point_identifier_ie (
  llc_service_access_point_identifier_t * llc_sap_id,
  bool is_ie_present,
  uint8_t * buffer,
  const uint32_t len)
{
  uint32_t                                encoded = 0;

  if (is_ie_present) {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IE_MIN_LENGTH, len);
    *buffer = SM_LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IEI;
    encoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IE_MIN_LENGTH - 1), len);
  }

  *(buffer + encoded) = 0x00 | (*llc_sap_id & 0xf);
  encoded++;
  return encoded;
}

//------------------------------------------------------------------------------
// 10.5.6.11 Packet Flow Identifier
//------------------------------------------------------------------------------
int decode_packet_flow_identifier_ie (
  packet_flow_identifier_t * packetflowidentifier,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;
  uint8_t                                 ielen = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, PACKET_FLOW_IDENTIFIER_IE_MAX_LENGTH, len);
    CHECK_IEI_DECODER (SM_PACKET_FLOW_IDENTIFIER_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (PACKET_FLOW_IDENTIFIER_IE_MAX_LENGTH - 1), len);
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);
  *packetflowidentifier = *buffer & 0x7f;
  decoded++;
  return decoded;
}

//------------------------------------------------------------------------------
int encode_packet_flow_identifier_ie (
  packet_flow_identifier_t * packetflowidentifier,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  uint8_t                                *lenPtr;
  uint32_t                                encoded = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, PACKET_FLOW_IDENTIFIER_IE_MIN_LENGTH, len);
    *buffer = SM_PACKET_FLOW_IDENTIFIER_IEI;
    encoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (PACKET_FLOW_IDENTIFIER_IE_MIN_LENGTH - 1), len);
  }

  lenPtr = (buffer + encoded);
  encoded++;
  *(buffer + encoded) = 0x00 | (*packetflowidentifier & 0x7f);
  encoded++;
  *lenPtr = encoded - 1 - ((iei_present) ? 1 : 0);
  return encoded;
}


//------------------------------------------------------------------------------
// 10.5.6.12 Traffic Flow Template
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int decode_traffic_flow_template_packet_filter_identifier (
    packet_filter_identifier_t * packetfilteridentifier,
    const uint8_t * const buffer,
    const uint32_t len)
{
  int                                     decoded = 0;
  /*
   * Packet filter identifier
   */
  IES_DECODE_U8 (buffer, decoded, (packetfilteridentifier->identifier));
  return decoded;
}

//------------------------------------------------------------------------------
static int decode_traffic_flow_template_packet_filter (
    packet_filter_t * packetfilter,
    const uint8_t * const buffer,
    const uint32_t len)
{
  int                                     decoded = 0,j;

  if (len - decoded <= 0) {
    /*
     * Mismatch between the number of packet filters subfield,
     * * * * and the number of packet filters in the packet filter list
     */
    return (TLV_VALUE_DOESNT_MATCH);
  }

  /*
   * Initialize the packet filter presence flag indicator
   */
  packetfilter->packetfiltercontents.flags = 0;
  /*
   * Packet filter direction
   */
  packetfilter->direction = *(buffer + decoded) >> 4;
  /*
   * Packet filter identifier
   */
  packetfilter->identifier = *(buffer + decoded) & 0x0f;
  decoded++;
  /*
   * Packet filter evaluation precedence
   */
  IES_DECODE_U8 (buffer, decoded, packetfilter->eval_precedence);
  /*
   * Length of the Packet filter contents field
   */
  IES_DECODE_U8 (buffer, decoded, packetfilter->length);
  /*
   * Packet filter contents
   */
  int                                     pkfstart = decoded;

  while (decoded - pkfstart < packetfilter->length) {
    /*
     * Packet filter component type identifier
     */
    uint8_t                                 component_type;

    IES_DECODE_U8 (buffer, decoded, component_type);

    switch (component_type) {
    case TRAFFIC_FLOW_TEMPLATE_IPV4_REMOTE_ADDR:
      /*
       * IPv4 remote address type
       */
      packetfilter->packetfiltercontents.flags |= TRAFFIC_FLOW_TEMPLATE_IPV4_REMOTE_ADDR_FLAG;

      for (j = 0; j < TRAFFIC_FLOW_TEMPLATE_IPV4_ADDR_SIZE; j++) {
        packetfilter->packetfiltercontents.ipv4remoteaddr[j].addr = *(buffer + decoded);
        packetfilter->packetfiltercontents.ipv4remoteaddr[j].mask = *(buffer + decoded + TRAFFIC_FLOW_TEMPLATE_IPV4_ADDR_SIZE);
        decoded++;
      }

      decoded += TRAFFIC_FLOW_TEMPLATE_IPV4_ADDR_SIZE;
      break;

    case TRAFFIC_FLOW_TEMPLATE_IPV6_REMOTE_ADDR:
      /*
       * IPv6 remote address type
       */
      packetfilter->packetfiltercontents.flags |= TRAFFIC_FLOW_TEMPLATE_IPV6_REMOTE_ADDR_FLAG;

      for (j = 0; j < TRAFFIC_FLOW_TEMPLATE_IPV6_ADDR_SIZE; j++) {
        packetfilter->packetfiltercontents.ipv6remoteaddr[j].addr = *(buffer + decoded);
        packetfilter->packetfiltercontents.ipv6remoteaddr[j].mask = *(buffer + decoded + TRAFFIC_FLOW_TEMPLATE_IPV6_ADDR_SIZE);
        decoded++;
      }

      decoded += TRAFFIC_FLOW_TEMPLATE_IPV6_ADDR_SIZE;
      break;

    case TRAFFIC_FLOW_TEMPLATE_PROTOCOL_NEXT_HEADER:
      /*
       * Protocol identifier/Next header type
       */
      packetfilter->packetfiltercontents.flags |= TRAFFIC_FLOW_TEMPLATE_PROTOCOL_NEXT_HEADER_FLAG;
      IES_DECODE_U8 (buffer, decoded, packetfilter->packetfiltercontents.protocolidentifier_nextheader);
      break;

    case TRAFFIC_FLOW_TEMPLATE_SINGLE_LOCAL_PORT:
      /*
       * Single local port type
       */
      packetfilter->packetfiltercontents.flags |= TRAFFIC_FLOW_TEMPLATE_SINGLE_LOCAL_PORT_FLAG;
      IES_DECODE_U16 (buffer, decoded, packetfilter->packetfiltercontents.singlelocalport);
      break;

    case TRAFFIC_FLOW_TEMPLATE_LOCAL_PORT_RANGE:
      /*
       * Local port range type
       */
      packetfilter->packetfiltercontents.flags |= TRAFFIC_FLOW_TEMPLATE_LOCAL_PORT_RANGE_FLAG;
      IES_DECODE_U16 (buffer, decoded, packetfilter->packetfiltercontents.localportrange.lowlimit);
      IES_DECODE_U16 (buffer, decoded, packetfilter->packetfiltercontents.localportrange.highlimit);
      break;

    case TRAFFIC_FLOW_TEMPLATE_SINGLE_REMOTE_PORT:
      /*
       * Single remote port type
       */
      packetfilter->packetfiltercontents.flags |= TRAFFIC_FLOW_TEMPLATE_SINGLE_REMOTE_PORT_FLAG;
      IES_DECODE_U16 (buffer, decoded, packetfilter->packetfiltercontents.singleremoteport);
      break;

    case TRAFFIC_FLOW_TEMPLATE_REMOTE_PORT_RANGE:
      /*
       * Remote port range type
       */
      packetfilter->packetfiltercontents.flags |= TRAFFIC_FLOW_TEMPLATE_REMOTE_PORT_RANGE_FLAG;
      IES_DECODE_U16 (buffer, decoded, packetfilter->packetfiltercontents.remoteportrange.lowlimit);
      IES_DECODE_U16 (buffer, decoded, packetfilter->packetfiltercontents.remoteportrange.highlimit);
      break;

    case TRAFFIC_FLOW_TEMPLATE_SECURITY_PARAMETER_INDEX:
      /*
       * Security parameter index type
       */
      packetfilter->packetfiltercontents.flags |= TRAFFIC_FLOW_TEMPLATE_SECURITY_PARAMETER_INDEX_FLAG;
      IES_DECODE_U32 (buffer, decoded, packetfilter->packetfiltercontents.securityparameterindex);
      break;

    case TRAFFIC_FLOW_TEMPLATE_TYPE_OF_SERVICE_TRAFFIC_CLASS:
      /*
       * Type of service/Traffic class type
       */
      packetfilter->packetfiltercontents.flags |= TRAFFIC_FLOW_TEMPLATE_TYPE_OF_SERVICE_TRAFFIC_CLASS_FLAG;
      IES_DECODE_U8 (buffer, decoded, packetfilter->packetfiltercontents.typdeofservice_trafficclass.value);
      IES_DECODE_U8 (buffer, decoded, packetfilter->packetfiltercontents.typdeofservice_trafficclass.mask);
      break;

    case TRAFFIC_FLOW_TEMPLATE_FLOW_LABEL:
      /*
       * Flow label type
       */
      packetfilter->packetfiltercontents.flags |= TRAFFIC_FLOW_TEMPLATE_FLOW_LABEL_FLAG;
      IES_DECODE_U24 (buffer, decoded, packetfilter->packetfiltercontents.flowlabel);
      break;

    default:
      /*
       * Packet filter component type identifier is not valid
       */
      return (TLV_UNEXPECTED_IEI);
      break;
    }
  }

//  if (len - decoded != 0) {
//    /*
//     * Mismatch between the number of packet filters subfield,
//     * * * * and the number of packet filters in the packet filter list
//     */
//    return (TLV_VALUE_DOESNT_MATCH);
//  }

  return decoded;
}
//------------------------------------------------------------------------------
static int decode_traffic_flow_template_delete_packet (
  delete_packet_filter_t * packetfilter,
  const uint8_t * const buffer,
  const uint32_t len)
{
  return decode_traffic_flow_template_packet_filter_identifier ((packet_filter_identifier_t *)packetfilter, buffer, len);
}

//------------------------------------------------------------------------------
static int decode_traffic_flow_template_create_tft (
  create_new_tft_t * packetfilter,
  const uint8_t * const buffer,
  const uint32_t len)
{
  return decode_traffic_flow_template_packet_filter ((packet_filter_t *)packetfilter, buffer, len);
}

//------------------------------------------------------------------------------
static int decode_traffic_flow_template_add_packet (
  add_packet_filter_t * packetfilter,
  const uint8_t * const buffer,
  const uint32_t len)
{
  return decode_traffic_flow_template_packet_filter ((packet_filter_t *)packetfilter, buffer, len);
}

//------------------------------------------------------------------------------
static int decode_traffic_flow_template_replace_packet (
  replace_packet_filter_t * packetfilter,
  const uint8_t * const buffer,
  const uint32_t len)
{
  return decode_traffic_flow_template_packet_filter((packet_filter_t *)packetfilter, buffer, len);
}

//------------------------------------------------------------------------------
int decode_traffic_flow_template (
  traffic_flow_template_t * trafficflowtemplate,
  const uint8_t * const buffer,
  const uint32_t len)
{
  int                                     decoded = 0;
  int                                     decoded_result = 0;

  trafficflowtemplate->tftoperationcode = (*(buffer + decoded) >> 5) & 0x7;
  trafficflowtemplate->ebit = (*(buffer + decoded) >> 4) & 0x1;
  trafficflowtemplate->numberofpacketfilters = *(buffer + decoded) & 0xf;
  decoded++;

  /*
   * Decoding packet filter list
   */
  if (trafficflowtemplate->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_DELETE_PACKET_FILTERS_FROM_EXISTING_TFT) {
    for (int i = 0; i < trafficflowtemplate->numberofpacketfilters; i++) {
      decoded_result = decode_traffic_flow_template_delete_packet (&trafficflowtemplate->packetfilterlist.deletepacketfilter[i], (buffer + decoded), len - decoded);
      if (decoded_result < 0) {
        return decoded_result;
      }
      decoded += decoded_result;
    }
  } else if (trafficflowtemplate->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_CREATE_NEW_TFT) {
    for (int i = 0; i < trafficflowtemplate->numberofpacketfilters; i++) {
      decoded_result = decode_traffic_flow_template_create_tft (&trafficflowtemplate->packetfilterlist.createnewtft[i], (buffer + decoded), len - decoded);
      if (decoded_result < 0) {
        return decoded_result;
      }
      decoded += decoded_result;
    }
  } else if (trafficflowtemplate->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_ADD_PACKET_FILTER_TO_EXISTING_TFT) {
    for (int i = 0; i < trafficflowtemplate->numberofpacketfilters; i++) {
      decoded_result = decode_traffic_flow_template_add_packet (&trafficflowtemplate->packetfilterlist.addpacketfilter[i], (buffer + decoded), len - decoded);
      if (decoded_result < 0) {
        return decoded_result;
      }
      decoded += decoded_result;
    }
  } else if (trafficflowtemplate->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_REPLACE_PACKET_FILTERS_IN_EXISTING_TFT) {
    for (int i = 0; i < trafficflowtemplate->numberofpacketfilters; i++) {
      decoded_result = decode_traffic_flow_template_replace_packet (&trafficflowtemplate->packetfilterlist.replacepacketfilter[i], (buffer + decoded), len - decoded);
      if (decoded_result < 0) {
        return decoded_result;
      }
      decoded += decoded_result;
    }
  }


  return decoded;
}
//------------------------------------------------------------------------------
int
decode_traffic_flow_template_ie (
    traffic_flow_template_t * trafficflowtemplate,
    const bool iei_present,
    const uint8_t * const buffer,
    const uint32_t len)
{
  int                                     decoded = 0;
  int                                     decoded2 = 0;
  uint8_t                                 ielen = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, TRAFFIC_FLOW_TEMPLATE_MINIMUM_LENGTH, len);
    CHECK_IEI_DECODER (SM_PROTOCOL_CONFIGURATION_OPTIONS_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (TRAFFIC_FLOW_TEMPLATE_MINIMUM_LENGTH - 1), len);
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);

  decoded2 = decode_traffic_flow_template(trafficflowtemplate, buffer + decoded, len - decoded);
  if (decoded2 < 0) return decoded2;
  return decoded+decoded2;
}

//------------------------------------------------------------------------------
static int encode_traffic_flow_template_packet_filter_identifier (
  const   packet_filter_identifier_t * packetfilteridentifier,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     encoded = 0;

  /*
   * Packet filter identifier
   */
  IES_ENCODE_U8 (buffer, encoded, packetfilteridentifier->identifier);

  return encoded;
}

//------------------------------------------------------------------------------
static int encode_traffic_flow_template_packet_filter (
  const   packet_filter_t * packetfilter,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     encoded = 0, j;

  if (len - encoded <= 0) {
    /*
     * Mismatch between the number of packet filters subfield,
     * * * * and the number of packet filters in the packet filter list
     */
    return (TLV_VALUE_DOESNT_MATCH);
  }

  /*
   * Packet filter identifier and direction
   */
  IES_ENCODE_U8 (buffer, encoded, ((packetfilter->direction << 4) | (packetfilter->identifier)));
  /*
   * Packet filter evaluation precedence
   */
  IES_ENCODE_U8 (buffer, encoded, packetfilter->eval_precedence);

  /*
   * Save address of the Packet filter contents field length
   */
  uint8_t                                *pkflenPtr = buffer + encoded;

  encoded++;
  /*
   * Packet filter contents
   */
  int                                     pkfstart = encoded;
  uint16_t                                flag = TRAFFIC_FLOW_TEMPLATE_IPV4_REMOTE_ADDR_FLAG;

  while (flag <= TRAFFIC_FLOW_TEMPLATE_FLOW_LABEL_FLAG) {
    switch (packetfilter->packetfiltercontents.flags & flag) {
    case TRAFFIC_FLOW_TEMPLATE_IPV4_REMOTE_ADDR_FLAG:
      /*
       * IPv4 remote address type
       */
      IES_ENCODE_U8 (buffer, encoded, TRAFFIC_FLOW_TEMPLATE_IPV4_REMOTE_ADDR);

      for (j = encoded; j < TRAFFIC_FLOW_TEMPLATE_IPV4_ADDR_SIZE + encoded; j++) {
        *((uint8_t*)(buffer + j)) = packetfilter->packetfiltercontents.ipv4remoteaddr[j - encoded].addr;
        *((uint8_t*)(buffer + j + TRAFFIC_FLOW_TEMPLATE_IPV4_ADDR_SIZE)) = packetfilter->packetfiltercontents.ipv4remoteaddr[j - encoded].mask;
      }

      encoded += TRAFFIC_FLOW_TEMPLATE_IPV4_ADDR_SIZE;
      encoded += TRAFFIC_FLOW_TEMPLATE_IPV4_ADDR_SIZE;
      break;

    case TRAFFIC_FLOW_TEMPLATE_IPV6_REMOTE_ADDR_FLAG:
      /*
       * IPv6 remote address type
       */
      IES_ENCODE_U8 (buffer, encoded, TRAFFIC_FLOW_TEMPLATE_IPV6_REMOTE_ADDR);

      for (j = encoded; j < TRAFFIC_FLOW_TEMPLATE_IPV6_ADDR_SIZE + encoded; j++) {
        *((uint8_t*)(buffer + j)) = packetfilter->packetfiltercontents.ipv6remoteaddr[j - encoded].addr;
        *((uint8_t*)(buffer + j + TRAFFIC_FLOW_TEMPLATE_IPV6_ADDR_SIZE)) = packetfilter->packetfiltercontents.ipv6remoteaddr[j - encoded].mask;
      }

      encoded += TRAFFIC_FLOW_TEMPLATE_IPV6_ADDR_SIZE;
      encoded += TRAFFIC_FLOW_TEMPLATE_IPV6_ADDR_SIZE;
      break;

    case TRAFFIC_FLOW_TEMPLATE_PROTOCOL_NEXT_HEADER_FLAG:
      /*
       * Protocol identifier/Next header type
       */
      IES_ENCODE_U8 (buffer, encoded, TRAFFIC_FLOW_TEMPLATE_PROTOCOL_NEXT_HEADER);
      IES_ENCODE_U8 (buffer, encoded, packetfilter->packetfiltercontents.protocolidentifier_nextheader);
      break;

    case TRAFFIC_FLOW_TEMPLATE_SINGLE_LOCAL_PORT_FLAG:
      /*
       * Single local port type
       */
      IES_ENCODE_U8 (buffer, encoded, TRAFFIC_FLOW_TEMPLATE_SINGLE_LOCAL_PORT);
      IES_ENCODE_U16 (buffer, encoded, packetfilter->packetfiltercontents.singlelocalport);
      break;

    case TRAFFIC_FLOW_TEMPLATE_LOCAL_PORT_RANGE_FLAG:
      /*
       * Local port range type
       */
      IES_ENCODE_U8 (buffer, encoded, TRAFFIC_FLOW_TEMPLATE_LOCAL_PORT_RANGE);
      IES_ENCODE_U16 (buffer, encoded, packetfilter->packetfiltercontents.localportrange.lowlimit);
      IES_ENCODE_U16 (buffer, encoded, packetfilter->packetfiltercontents.localportrange.highlimit);
      break;

    case TRAFFIC_FLOW_TEMPLATE_SINGLE_REMOTE_PORT_FLAG:
      /*
       * Single remote port type
       */
      IES_ENCODE_U8 (buffer, encoded, TRAFFIC_FLOW_TEMPLATE_SINGLE_REMOTE_PORT);
      IES_ENCODE_U16 (buffer, encoded, packetfilter->packetfiltercontents.singleremoteport);
      break;

    case TRAFFIC_FLOW_TEMPLATE_REMOTE_PORT_RANGE_FLAG:
      /*
       * Remote port range type
       */
      IES_ENCODE_U8 (buffer, encoded, TRAFFIC_FLOW_TEMPLATE_REMOTE_PORT_RANGE);
      IES_ENCODE_U16 (buffer, encoded, packetfilter->packetfiltercontents.remoteportrange.lowlimit);
      IES_ENCODE_U16 (buffer, encoded, packetfilter->packetfiltercontents.remoteportrange.highlimit);
      break;

    case TRAFFIC_FLOW_TEMPLATE_SECURITY_PARAMETER_INDEX_FLAG:
      /*
       * Security parameter index type
       */
      IES_ENCODE_U8 (buffer, encoded, TRAFFIC_FLOW_TEMPLATE_SECURITY_PARAMETER_INDEX);
      IES_ENCODE_U32 (buffer, encoded, packetfilter->packetfiltercontents.securityparameterindex);
      break;

    case TRAFFIC_FLOW_TEMPLATE_TYPE_OF_SERVICE_TRAFFIC_CLASS_FLAG:
      /*
       * Type of service/Traffic class type
       */
      IES_ENCODE_U8 (buffer, encoded, TRAFFIC_FLOW_TEMPLATE_TYPE_OF_SERVICE_TRAFFIC_CLASS);
      IES_ENCODE_U8 (buffer, encoded, packetfilter->packetfiltercontents.typdeofservice_trafficclass.value);
      IES_ENCODE_U8 (buffer, encoded, packetfilter->packetfiltercontents.typdeofservice_trafficclass.mask);
      break;

    case TRAFFIC_FLOW_TEMPLATE_FLOW_LABEL_FLAG:
      /*
       * Flow label type
       */
      IES_ENCODE_U8 (buffer, encoded, TRAFFIC_FLOW_TEMPLATE_FLOW_LABEL);
      IES_ENCODE_U24 (buffer, encoded, packetfilter->packetfiltercontents.flowlabel & 0x000fffff);
      break;

    default:
      break;
    }

    flag = flag << 1;
  }

  /*
   * Length of the Packet filter contents field
   */
  *pkflenPtr = encoded - pkfstart;

  return encoded;
}

//------------------------------------------------------------------------------
static int encode_traffic_flow_template_delete_packet (
    const delete_packet_filter_t * packetfilter,
    uint8_t * buffer,
    const uint32_t len)
{
  return encode_traffic_flow_template_packet_filter_identifier (packetfilter, buffer, len);
}

//------------------------------------------------------------------------------
static int encode_traffic_flow_template_create_tft (
    const create_new_tft_t * packetfilter,
    uint8_t * buffer,
    const uint32_t len)
{
  return encode_traffic_flow_template_packet_filter (packetfilter, buffer, len);
}

//------------------------------------------------------------------------------
static int encode_traffic_flow_template_add_packet (
    const add_packet_filter_t * packetfilter,
    uint8_t * buffer,
    const uint32_t len)
{
  return encode_traffic_flow_template_packet_filter (packetfilter, buffer, len);
}

//------------------------------------------------------------------------------
static int encode_traffic_flow_template_replace_packet (
    const replace_packet_filter_t * packetfilter,
    uint8_t * buffer,
    const uint32_t len)
{
  return encode_traffic_flow_template_packet_filter (packetfilter, buffer, len);
}

//------------------------------------------------------------------------------
int encode_traffic_flow_template (
  const traffic_flow_template_t * trafficflowtemplate,
  uint8_t * buffer,
  const uint32_t len)
{
  uint32_t                                encoded = 0;


  *(buffer + encoded) = ((trafficflowtemplate->tftoperationcode & 0x7) << 5) | ((trafficflowtemplate->ebit & 0x1) << 4) | (trafficflowtemplate->numberofpacketfilters & 0xf);
  encoded++;

  /*
   * Encoding packet filter list
   */
  if (trafficflowtemplate->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_DELETE_PACKET_FILTERS_FROM_EXISTING_TFT) {
    for (int i = 0; i < trafficflowtemplate->numberofpacketfilters; i++) {
      encoded += encode_traffic_flow_template_delete_packet (&trafficflowtemplate->packetfilterlist.deletepacketfilter[i], (buffer + encoded), len - encoded);
    }
  } else if (trafficflowtemplate->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_CREATE_NEW_TFT) {
    for (int i = 0; i < trafficflowtemplate->numberofpacketfilters; i++) {
      encoded += encode_traffic_flow_template_create_tft (&trafficflowtemplate->packetfilterlist.createnewtft[i], (buffer + encoded), len - encoded);
    }
  } else if (trafficflowtemplate->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_ADD_PACKET_FILTER_TO_EXISTING_TFT) {
    for (int i = 0; i < trafficflowtemplate->numberofpacketfilters; i++) {
      encoded += encode_traffic_flow_template_add_packet (&trafficflowtemplate->packetfilterlist.addpacketfilter[i], (buffer + encoded), len - encoded);
    }
  } else if (trafficflowtemplate->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_REPLACE_PACKET_FILTERS_IN_EXISTING_TFT) {
    for (int i = 0; i < trafficflowtemplate->numberofpacketfilters; i++) {
      encoded += encode_traffic_flow_template_replace_packet (&trafficflowtemplate->packetfilterlist.replacepacketfilter[i], (buffer + encoded), len - encoded);
    }
  }

  return encoded;
}

//------------------------------------------------------------------------------
int
encode_traffic_flow_template_ie (
    const traffic_flow_template_t * const trafficflowtemplate,
    const bool iei_present,
    uint8_t * buffer,
    const uint32_t len)
{
  uint8_t                                *lenPtr = NULL;
  uint32_t                                encoded = 0;

 if (iei_present) {
   CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, TRAFFIC_FLOW_TEMPLATE_MINIMUM_LENGTH, len);
   *buffer = SM_TRAFFIC_FLOW_TEMPLATE_IEI;
   encoded++;
 } else {
   CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (TRAFFIC_FLOW_TEMPLATE_MINIMUM_LENGTH - 1), len);
 }

 lenPtr = (buffer + encoded);
 encoded++;

 encoded += encode_traffic_flow_template(trafficflowtemplate, buffer + encoded, len - encoded);

  *lenPtr = encoded - 1 - ((iei_present) ? 1 : 0);
  return encoded;
}

//------------------------------------------------------------------------------
void copy_traffic_flow_template (traffic_flow_template_t * const tft_dst, const traffic_flow_template_t * const tft_src)
{
  if ((tft_dst) && (tft_src)) {
    tft_dst->tftoperationcode = tft_src->tftoperationcode;
    tft_dst->ebit = tft_src->ebit;
    tft_dst->numberofpacketfilters = tft_src->numberofpacketfilters;
    memcpy(&tft_dst->packetfilterlist, &tft_src->packetfilterlist, sizeof(tft_src->packetfilterlist));
    tft_dst->parameterslist.num_parameters = tft_src->parameterslist.num_parameters;
    // not necessary now to create a subroutine for subtype
    for (int i = 0; i < tft_src->parameterslist.num_parameters; i++) {
      tft_dst->parameterslist.parameter[i].parameteridentifier = tft_src->parameterslist.parameter[i].parameteridentifier;
      tft_dst->parameterslist.parameter[i].length = tft_src->parameterslist.parameter[i].length;
      tft_dst->parameterslist.parameter[i].contents = bstrcpy(tft_src->parameterslist.parameter[i].contents);
    }
  }
}
//------------------------------------------------------------------------------
static void free_traffic_flow_template_parameter (parameter_t * param)
{
  bdestroy_wrapper (&param->contents);
}

//------------------------------------------------------------------------------
void free_traffic_flow_template(traffic_flow_template_t ** tft)
{
  traffic_flow_template_t * trafficflowtemplate = *tft;
  // nothing to do for packet filters
  for (int i = 0; i < trafficflowtemplate->parameterslist.num_parameters; i++) {
    free_traffic_flow_template_parameter (&trafficflowtemplate->parameterslist.parameter[i]);
  }
  free_wrapper((void**)tft);
}

//  2) When the TFT operation is "Create a new TFT" or "Add packet filters to existing TFT" or "Replace packet
//  filters in existing TFT" and two or more packet filters in all TFTs associated with this PDP address and APN
//  would have identical packet filter precedence values.

//------------------------------------------------------------------------------
/**
 * We just add 1 to the identifier in the bitmaps.
 */
#include "esm_cause.h"
esm_cause_t
verify_traffic_flow_template_syntactical(traffic_flow_template_t * tft, traffic_flow_template_t * tft_original)
{
  if(!tft || !tft->numberofpacketfilters){
    return ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION;
  }
  switch(tft->tftoperationcode){
  case TRAFFIC_FLOW_TEMPLATE_OPCODE_CREATE_NEW_TFT: {
    if(tft_original){
      return ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION;
    }
    for(int num_pf = 0; num_pf < tft->numberofpacketfilters; num_pf++){
      /** Check that all packet filters exist. */
      if(!tft->packetfilterlist.createnewtft[num_pf].length){
        /** No packet filter exists, although mentioned in length. */
        return ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION;
      }
      /** Verify each packet filter has distinct identifier and precedence. */
      if(tft->packet_filter_identifier_bitmap & (0x01 << tft->packetfilterlist.createnewtft[num_pf].identifier + 1)){
        /** Packet Filter Identifier already set. */
        return ESM_CAUSE_SYNTACTICAL_ERROR_IN_PACKET_FILTER;
      }
      if(tft->precedence_set[tft->packetfilterlist.createnewtft[num_pf].eval_precedence]){
        /** Packet Filter Identifier already set. */
        return ESM_CAUSE_SYNTACTICAL_ERROR_IN_PACKET_FILTER;
      }
      /** Mark the new packet filter. */
      tft->packet_filter_identifier_bitmap |= (0x01 << tft->packetfilterlist.createnewtft[num_pf].identifier + 1);
      tft->precedence_set[tft->packetfilterlist.createnewtft[num_pf].eval_precedence] = tft->packetfilterlist.createnewtft[num_pf].identifier + 1;
      /** Successfully validated packet filter syntax, */
    }
    /** Successfully checked addition TFTs. */
    return ESM_CAUSE_SUCCESS;
  }
  case TRAFFIC_FLOW_TEMPLATE_OPCODE_ADD_PACKET_FILTER_TO_EXISTING_TFT: {
    if(!tft_original){
      return ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION;
    }
    for(int num_pf = 0; num_pf < tft->numberofpacketfilters; num_pf++){
      /** Check that all packet filters exist. */
      if(!tft->packetfilterlist.addpacketfilter[num_pf].length){
        /** No packet filter exists, although mentioned in length. */
        return ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION;
      }
      /** Check that the packet filter does not exist in the original TFT. */
      if(tft_original->numberofpacketfilters == TRAFFIC_FLOW_TEMPLATE_NB_PACKET_FILTERS_MAX){
        /** Original TFT already has max number of packet filters. */
        return ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION;
      }
      if((tft_original->packet_filter_identifier_bitmap | tft->packet_filter_identifier_bitmap)
          & (0x01 << tft->packetfilterlist.addpacketfilter[num_pf].identifier + 1)) {
        /** Id already existing. */
        return ESM_CAUSE_SYNTACTICAL_ERROR_IN_PACKET_FILTER;
      }
      if(tft_original->precedence_set[tft->packetfilterlist.addpacketfilter[num_pf].eval_precedence] || tft->precedence_set[tft->packetfilterlist.addpacketfilter[num_pf].eval_precedence]){
        /** Precedence already exists in the original or already in the new TFT. */
        return ESM_CAUSE_SYNTACTICAL_ERROR_IN_PACKET_FILTER;
      }
      /** Set the precedence value in the new TFT. */
      tft->packet_filter_identifier_bitmap |= (0x01 << tft->packetfilterlist.addpacketfilter[num_pf].identifier + 1);
      tft->precedence_set[tft->packetfilterlist.addpacketfilter[num_pf].eval_precedence] = tft->packetfilterlist.addpacketfilter[num_pf].identifier + 1;
    }
    /** Successfully checked addition TFTs. */
    return ESM_CAUSE_SUCCESS;
  }
  case TRAFFIC_FLOW_TEMPLATE_OPCODE_REPLACE_PACKET_FILTERS_IN_EXISTING_TFT: {
    if(!tft_original){
      return ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION;
    }
    for(int num_pf = 0; num_pf < tft->numberofpacketfilters; num_pf++){
      /** Check that all packet filters exist. */
      if(!tft->packetfilterlist.replacepacketfilter[num_pf].length){
        /** No packet filter exists, although mentioned in length. */
        return ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION;
      }
      /**
       * Just check that it does not occur twice in the current new TFT.
       * We will find it in the original TFT (must exist).
       */
      if(tft->packet_filter_identifier_bitmap & (0x01 << tft->packetfilterlist.replacepacketfilter[num_pf].identifier + 1)){
        return ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION;
      }
      /** Must exist in the original one. */
      if(!(tft_original->packet_filter_identifier_bitmap & (0x01 << tft->packetfilterlist.replacepacketfilter[num_pf].identifier + 1))){
        return ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION;
      }
      /** Check that the precedence is not already given in the same new TFT. */
      if(tft->precedence_set[tft->packetfilterlist.replacepacketfilter[num_pf].eval_precedence]){
        /** Precedence already exists in the original or already in the new TFT. */
        return ESM_CAUSE_SYNTACTICAL_ERROR_IN_PACKET_FILTER;
      }
      /** Set the precedence value in the new TFT. */
      tft->precedence_set[tft->packetfilterlist.replacepacketfilter[num_pf].eval_precedence] = tft->packetfilterlist.replacepacketfilter[num_pf].identifier + 1;
      tft->packet_filter_identifier_bitmap |= (0x01 << tft->packetfilterlist.replacepacketfilter[num_pf].identifier + 1);
    }
    /** Check that the precedences all move. */
    for(int num_pf = 0; num_pf < tft->numberofpacketfilters; num_pf++){
      /** Successfully validated packet filter syntax. */
      if(tft_original->precedence_set[tft->packetfilterlist.replacepacketfilter[num_pf].eval_precedence] &&
          tft_original->precedence_set[tft->packetfilterlist.replacepacketfilter[num_pf].eval_precedence] != (tft->packetfilterlist.replacepacketfilter[num_pf].identifier + 1)) {
        /** Found in the original TFT a packet filter with same precedence but other identifier. For this identifier, a new precedence also should exist (check only one layer). */
        uint8_t ow_ident = tft_original->precedence_set[tft->packetfilterlist.replacepacketfilter[num_pf].eval_precedence]; /**< The one added. */
        if(!(tft->packet_filter_identifier_bitmap & (0x01 << ow_ident))){
          /** No replacement for packet filter whose precedence is overwritten.. */
          return ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION;
        }
      }
    }
    /** Successfully checked addition TFTs. */
    return ESM_CAUSE_SUCCESS;
  }
  case TRAFFIC_FLOW_TEMPLATE_OPCODE_DELETE_PACKET_FILTERS_FROM_EXISTING_TFT:
    if(!tft_original){
      return ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION;
    }
    /** Check that not all packet filters are removed from the TFT. */
    if(tft_original->numberofpacketfilters <= tft->numberofpacketfilters){
      return ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION;
    }
    for(int num_pf = 0; num_pf < tft->numberofpacketfilters; num_pf++){
        /**
         * Find the packet filter in the original TFT.
         * Not checking if multiple with same identifiert.
         */
        if(!(tft_original->packet_filter_identifier_bitmap & (0x01 << tft->packetfilterlist.deletepacketfilter[num_pf].identifier + 1))){
          return ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION;
        }
        if(tft->packet_filter_identifier_bitmap & (0x01 << tft->packetfilterlist.deletepacketfilter[num_pf].identifier + 1)){
          return ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION;
        }
        tft->packet_filter_identifier_bitmap |= (0x01 << tft->packetfilterlist.deletepacketfilter[num_pf].identifier + 1);
    }
    return ESM_CAUSE_SUCCESS;
  default:
    /** No check implemented for TFT operation code. */
    return ESM_CAUSE_REQUEST_REJECTED_BY_GW;
  }
}



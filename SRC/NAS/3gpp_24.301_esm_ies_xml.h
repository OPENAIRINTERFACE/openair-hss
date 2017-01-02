/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */

#ifndef FILE_3GPP_24_301_ESM_IES_XML_SEEN
#define FILE_3GPP_24_301_ESM_IES_XML_SEEN
#include "xml_load.h"
//==============================================================================
// 9 General message format and information elements coding
//==============================================================================

//------------------------------------------------------------------------------
// 9.9.4 EPS Session Management (ESM) information elements
//------------------------------------------------------------------------------
// 9.9.4.1 Access point name
// See subclause 10.5.6.1 in 3GPP TS 24.008 [13].

// 9.9.4.2 APN aggregate maximum bit rate
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_XML_STR                              "apn_agregate_maximum_bit_rate"
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_DOWNLINK_ATTR_XML_STR            "apn_ambr_dl"
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_UPLINK_ATTR_XML_STR              "apn_ambr_ul"
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_DOWNLINK_EXTENDED_ATTR_XML_STR   "apn_ambr_dl_ext"
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_UPLINK_EXTENDED_ATTR_XML_STR     "apn_ambr_ul_ext"
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_DOWNLINK_EXTENDED2_ATTR_XML_STR  "apn_ambr_dl_ext2"
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_UPLINK_EXTENDED2_ATTR_XML_STR    "apn_ambr_ul_ext2"
bool apn_aggregate_maximum_bit_rate_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, ApnAggregateMaximumBitRate * const apnaggregatemaximumbitrate);
void apn_aggregate_maximum_bit_rate_to_xml(ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, xmlTextWriterPtr writer);

// 9.9.4.2A Connectivity type
// See subclause 10.5.6.19 in 3GPP TS 24.008 [13].

// 9.9.4.3 EPS quality of service
#define EPS_QUALITY_OF_SERVICE_XML_STR                        "eps_quality_of_service"
#define QCI_ATTR_XML_STR                                      "qci"
#define MAXIMUM_BIT_RATE_FOR_UPLINK_ATTR_XML_STR              "maximum_bit_rate_for_uplink"
#define MAXIMUM_BIT_RATE_FOR_DOWNLINK_ATTR_XML_STR            "maximum_bit_rate_for_downlink"
#define GUARANTED_BIT_RATE_FOR_UPLINK_ATTR_XML_STR            "guaranted_bit_rate_for_uplink"
#define GUARANTED_BIT_RATE_FOR_DOWNLINK_ATTR_XML_STR          "guaranted_bit_rate_for_downlink"
#define MAXIMUM_BIT_RATE_FOR_UPLINK_EXTENDED_ATTR_XML_STR     "maximum_bit_rate_for_uplink_extended"
#define MAXIMUM_BIT_RATE_FOR_DOWNLINK_EXTENDED_ATTR_XML_STR   "maximum_bit_rate_for_downlink_extended"
#define GUARANTED_BIT_RATE_FOR_UPLINK_EXTENDED_ATTR_XML_STR   "guaranted_bit_rate_for_uplink_extended"
#define GUARANTED_BIT_RATE_FOR_DOWNLINK_EXTENDED_ATTR_XML_STR "guaranted_bit_rate_for_downlink_extended"
bool eps_quality_of_service_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    EpsQualityOfService   * const epsqualityofservice);
void eps_quality_of_service_to_xml (EpsQualityOfService * epsqualityofservice, xmlTextWriterPtr writer);

// 9.9.4.4 ESM cause
#define ESM_CAUSE_XML_STR         "esm_cause"
#define ESM_CAUSE_XML_SCAN_FMT    "%"SCNx8
#define ESM_CAUSE_XML_FMT         "0x%"PRIx8
NUM_FROM_XML_PROTOTYPE(esm_cause);
void esm_cause_to_xml (esm_cause_t * esmcause, xmlTextWriterPtr writer);

// 9.9.4.5 ESM information transfer flag
#define ESM_INFORMATION_TRANSFER_FLAG_XML_STR         "esm_information_transfer_flag"
#define ESM_INFORMATION_TRANSFER_FLAG_XML_SCAN_FMT    "%"SCNx8
#define ESM_INFORMATION_TRANSFER_FLAG_XML_FMT         "0x%"PRIx8
NUM_FROM_XML_PROTOTYPE(esm_information_transfer_flag);
void esm_information_transfer_flag_to_xml (esm_information_transfer_flag_t * esminformationtransferflag, xmlTextWriterPtr writer);

// 9.9.4.6 Linked EPS bearer identity
#define LINKED_EPS_BEARER_IDENTITY_XML_STR         "linked_eps_bearer_identity"
#define LINKED_EPS_BEARER_IDENTITY_XML_SCAN_FMT    "%"SCNx8
#define LINKED_EPS_BEARER_IDENTITY_XML_FMT         "0x%"PRIx8
NUM_FROM_XML_PROTOTYPE(linked_eps_bearer_identity);
void linked_eps_bearer_identity_to_xml (linked_eps_bearer_identity_t * linkedepsbeareridentity, xmlTextWriterPtr writer);

// 9.9.4.7 LLC service access point identifier
// See subclause 10.5.6.9 in 3GPP TS 24.008 [13].

// 9.9.4.7A Notification indicator
// TODO

// 9.9.4.8 Packet flow identifier
// See subclause 10.5.6.11 in 3GPP TS 24.008 [13].

// 9.9.4.9 PDN address
#define PDN_ADDRESS_XML_STR                       "pdn_address"
#define PDN_TYPE_VALUE_ATTR_XML_STR               "pdn_type_value"
#define PDN_ADDRESS_INFORMATION_ATTR_XML_STR      "pdn_address_information"
#define PDN_TYPE_VALUE_IPV4_VAL_XML_STR           "IPV4"
#define PDN_TYPE_VALUE_IPV6_VAL_XML_STR           "IPV6"
#define PDN_TYPE_VALUE_IPV4V6_VAL_XML_STR         "IPV4V6"
bool pdn_address_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, PdnAddress * const pdnaddress);
void pdn_address_to_xml (PdnAddress * pdnaddress, xmlTextWriterPtr writer);

// 9.9.4.10 PDN type
#define PDN_TYPE_XML_STR            "pdn_type"
#define PDN_TYPE_IPV4_VAL_XML_STR   "IPv4"
#define PDN_TYPE_IPV6_VAL_XML_STR   "IPv6"
#define PDN_TYPE_IPV4V6_VAL_XML_STR "IPV4V6"
#define PDN_TYPE_UNUSED_VAL_XML_STR "UNUSED"
bool pdn_type_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, pdn_type_t * const pdntype);
void pdn_type_to_xml (pdn_type_t * pdntype, xmlTextWriterPtr writer);

// 9.9.4.11 Protocol configuration options
// See subclause 10.5.6.3 in 3GPP TS 24.008 [13].
// 9.9.4.12 Quality of service
// See subclause 10.5.6.5 in 3GPP TS 24.008 [13].

// 9.9.4.13 Radio priority
// See subclause 10.5.7.2 in 3GPP TS 24.008 [13].
#define RADIO_PRIORITY_XML_STR         "radio_priority"
#define RADIO_PRIORITY_XML_SCAN_FMT    "%"SCNx8
#define RADIO_PRIORITY_XML_FMT         "0x%"PRIx8
NUM_FROM_XML_PROTOTYPE(radio_priority);
void radio_priority_to_xml (radio_priority_t * radio_priority, xmlTextWriterPtr writer);

// 9.9.4.14 Request type
// See subclause 10.5.6.17 in 3GPP TS 24.008 [13].
#define REQUEST_TYPE_XML_STR         "request_type"
#define REQUEST_TYPE_XML_SCAN_FMT    "%"SCNx8
#define REQUEST_TYPE_XML_FMT         "0x%"PRIx8
NUM_FROM_XML_PROTOTYPE(request_type);
void request_type_to_xml (request_type_t * requesttype, xmlTextWriterPtr writer);


// 9.9.4.15 Traffic flow aggregate description
#define TRAFFIC_FLOW_AGGREGATE_DESCRIPTION_XML_STR "traffic_flow_aggregate_description"
// TODO void traffic_flow_aggregate_description_to_xml(TrafficFlowAggregateDescription *trafficflowaggregatedescription, uint8_t iei);

// 9.9.4.16 Traffic flow template
// See subclause 10.5.6.12 in 3GPP TS 24.008 [13].

// 9.9.4.17 Transaction identifier
// The Transaction identifier information element is coded as the Linked TI information element in 3GPP TS 24.008 [13],
// subclause 10.5.6.7.

#endif /* FILE_3GPP_24_301_ESM_IES_XML_SEEN */

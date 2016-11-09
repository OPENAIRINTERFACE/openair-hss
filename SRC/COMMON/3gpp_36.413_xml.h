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

#ifndef FILE_SP_3GPP_36_413_XML_SEEN
#define FILE_SP_3GPP_36_413_XML_SEEN

#define E_RAB_TO_BE_SETUP_LIST_XML_STR         "e_rab_to_be_setup_list"
#define E_RAB_TO_BE_SETUP_ITEM_XML_STR         "e_rab_to_be_setup_item"
#define E_RAB_ID_IE_XML_STR                    "e_rab_id"
#define E_RAB_LEVEL_QOS_PARAMETERS_XML_STR     "e_rab_level_qos_parameters"
#define ALLOCATION_AND_RETENTION_PRIORITY_IE_XML_STR "allocation_and_retention_priority"
#define PRIORITY_LEVEL_IE_XML_STR              "priority_level"
#define PRE_EMPTION_CAPABILITY_IE_XML_STR      "pre_emption_capability"
#define PRE_EMPTION_VULNERABILITY_IE_XML_STR   "pre_emption_vulnerability"
#define E_RAB_GUARANTEED_BIT_RATE_DOWNLINK_IE_XML_STR "e_rab_guaranteed_bit_rate_downlink"
#define GBR_QOS_INFORMATION_XML_STR            "gbr_qos_information"
#define  E_RAB_MAXIMUM_BIT_RATE_DOWNLINK_IE_XML_STR    "e_rab_maximum_bit_rate_downlink"
#define  E_RAB_MAXIMUM_BIT_RATE_UPLINK_IE_XML_STR      "e_rab_maximum_bit_rate_uplink"
#define  E_RAB_GUARANTEED_BIT_RATE_DOWNLINK_IE_XML_STR "e_rab_guaranteed_bit_rate_downlink"
#define  E_RAB_GUARANTEED_BIT_RATE_UPLINK_IE_XML_STR   "e_rab_guaranteed_bit_rate_uplink"


bool e_rab_to_be_setup_list_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    e_rab_to_be_setup_list_t * const list);
void e_rab_to_be_setup_list_to_xml (const e_rab_to_be_setup_list_t * const list, xmlTextWriterPtr writer);


bool e_rab_to_be_setup_item_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    e_rab_to_be_setup_item_t * const item);
void e_rab_to_be_setup_item_to_xml (const e_rab_to_be_setup_item_t * const item, xmlTextWriterPtr writer);


bool e_rab_id_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, e_rab_id_t * const e_rab_id);
void e_rab_id_to_xml (const e_rab_id_t * const e_rab_id, xmlTextWriterPtr writer);

bool e_rab_level_qos_parameters_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    e_rab_level_qos_parameters_t * const params);
void e_rab_level_qos_parameters_to_xml (const e_rab_level_qos_parameters_t * const params, xmlTextWriterPtr writer);


bool qci_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, qci_t * const qci);
void qci_to_xml (const qci_t * const qci, xmlTextWriterPtr writer);

bool allocation_and_retention_priority_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    allocation_and_retention_priority_t * const arp);
void allocation_and_retention_priority_to_xml (const allocation_and_retention_priority_t * const arp, xmlTextWriterPtr writer);

bool gbr_qos_information_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    gbr_qos_information_t * const gqi);
void gbr_qos_information_to_xml (const gbr_qos_information_t * const gqi, xmlTextWriterPtr writer);

#endif /* FILE_SP_3GPP_36_413_XML_SEEN */

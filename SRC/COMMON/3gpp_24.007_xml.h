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

#ifndef FILE_3GPP_24_007_XML_SEEN
#define FILE_3GPP_24_007_XML_SEEN

typedef uint8_t sequence_number_t;
#define SEQUENCE_NUMBER_XML_STR                         "sequence_number"
#define SEQUENCE_NUMBER_XML_SCAN_FMT                    "%"SCNx8
#define SEQUENCE_NUMBER_XML_FMT                         "0x%"PRIx8

bool sequence_number_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, uint8_t * const sn);
void sequence_number_to_xml(const uint8_t * const sn, xmlTextWriterPtr writer);

//..............................................................................
// 11.2.3.1.1  Protocol discriminator
//..............................................................................
#define PROTOCOL_DISCRIMINATOR_XML_STR                   "protocol_discriminator"
#define GROUP_CALL_CONTROL_VAL_XML_STR                   "GROUP_CALL_CONTROL"
#define BROADCAST_CALL_CONTROL_VAL_XML_STR               "BROADCAST_CALL_CONTROL"
#define EPS_SESSION_MANAGEMENT_MESSAGE_VAL_XML_STR       "EPS_SESSION_MANAGEMENT_MESSAGE"
#define CALL_CONTROL_CC_RELATED_SS_MESSAGE_VAL_XML_STR   "CALL_CONTROL_CC_RELATED_SS_MESSAGE"
#define GPRS_TRANSPARENT_TRANSPORT_PROTOCOL_VAL_XML_STR  "GPRS_TRANSPARENT_TRANSPORT_PROTOCOL"
#define MOBILITY_MANAGEMENT_MESSAGE_VAL_XML_STR          "MOBILITY_MANAGEMENT_MESSAGE"
#define RADIO_RESOURCES_MANAGEMENT_MESSAGE_VAL_XML_STR   "RADIO_RESOURCES_MANAGEMENT_MESSAGE"
#define EPS_MOBILITY_MANAGEMENT_MESSAGE_VAL_XML_STR      "EPS_MOBILITY_MANAGEMENT_MESSAGE"
#define GPRS_MOBILITY_MANAGEMENT_MESSAGE_VAL_XML_STR     "GPRS_MOBILITY_MANAGEMENT_MESSAGE"
#define SMS_MESSAGE_VAL_XML_STR                          "SMS_MESSAGE"
#define GPRS_SESSION_MANAGEMENT_MESSAGE_VAL_XML_STR      "GPRS_SESSION_MANAGEMENT_MESSAGE"
#define NON_CALL_RELATED_SS_MESSAGE_VAL_XML_STR          "NON_CALL_RELATED_SS_MESSAGE"

bool protocol_discriminator_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, eps_protocol_discriminator_t * const pd);
void protocol_discriminator_to_xml(const eps_protocol_discriminator_t * const pd, xmlTextWriterPtr writer);

//..............................................................................
// 11.2.3.1.5  EPS bearer identity
//..............................................................................
#define EBI_XML_STR                                 "ebi"
#define EBI_UNASSIGNED_VAL_XML_STR                  "EBI_UNASSIGNED"
#define EBI_XML_SCAN_FMT                            "%"SCNx8
#define EBI_XML_FMT                                 "0x%"PRIx8
bool eps_bearer_identity_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, ebi_t * const ebi);
void eps_bearer_identity_to_xml(const ebi_t * const ebi, xmlTextWriterPtr writer);

//..............................................................................
// 11.2.3.1a   Procedure transaction identity
//..............................................................................
#define PTI_XML_STR                                            "pti"
#define PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED_VAL_XML_STR  "PTI_UNASSIGNED"
#define PROCEDURE_TRANSACTION_IDENTITY_RESERVED_VAL_XML_STR    "PTI_RESERVED"
#define PTI_XML_SCAN_FMT                                       "%"SCNx8
#define PTI_XML_FMT                                            "0x%"PRIx8
bool procedure_transaction_identity_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, pti_t * const pti);
void procedure_transaction_identity_to_xml(const pti_t * const pti, xmlTextWriterPtr writer);

#endif /* FILE_3GPP_24_007_XML_SEEN */

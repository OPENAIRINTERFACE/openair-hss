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

#ifndef FILE_3GPP_29_274_XML_SEEN
#define FILE_3GPP_29_274_XML_SEEN

#include "xml_load.h"

#define INTERFACE_TYPE_XML_STR    "interface_type"
#define S1U_SGW_FTEID_XML_STR     "s1u_sgw_fteid"
#define S5_S8_U_PGW_FTEID_XML_STR "s5_s8_u_pgw_fteid"
#define S12_SGW_FTEID_XML_STR     "s12_sgw_fteid"
#define S4_U_SGW_FTEID_XML_STR    "s4_u_sgw_fteid"
#define S2B_U_PGW_FTEID_XML_STR   "s2b_u_pgw_fteid"
#define S2A_U_PGW_FTEID_XML_STR   "s2a_u_pgw_fteid"
#define IPV4_ADDRESS_XML_STR      "ipv4_address"
#define IPV6_ADDRESS_XML_STR      "ipv6_address"
#define BITRATE_UL_XML_STR        "bitrate_ul"
#define BITRATE_DL_XML_STR        "bitrate_dl"
#define GUARANTED_AMBR_XML_STR    "guaranted_ambr"
#define MAXIMUM_AMBR_XML_STR      "maximum_ambr"

#define BEARER_CONTEXT_XML_STR    "bearer_context"

void ambr_to_xml(const ambr_t * const ambr, const char * const xml_tag,  xmlTextWriterPtr writer);
bool ambr_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, const char * const xml_tag, ambr_t * const ambr);

void bearer_qos_to_xml(const bearer_qos_t * const bearer_qos, xmlTextWriterPtr writer);
bool bearer_qos_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, bearer_qos_t * const bearer_qos);

void interface_type_to_xml (const interface_type_t * const interface_type, xmlTextWriterPtr writer);
bool interface_type_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, interface_type_t * const interface_type);

NUM_FROM_XML_PROTOTYPE(teid);
void teid_to_xml (const teid_t * const teid, xmlTextWriterPtr writer);

void ipv4_address_to_xml (const struct in_addr * const in_addr, xmlTextWriterPtr writer);

void ipv6_address_to_xml (const struct in6_addr * const in6_addr, xmlTextWriterPtr writer);
void fteid_to_xml (const fteid_t * const fteid, const char * const xml_tag, xmlTextWriterPtr writer);

void bearer_context_within_create_bearer_request_to_xml (const bearer_context_within_create_bearer_request_t * const bearer_context, xmlTextWriterPtr writer);
void bearer_contexts_within_create_bearer_request_to_xml (const bearer_contexts_within_create_bearer_request_t * const bearer_contexts, xmlTextWriterPtr writer);


#endif /* FILE_3GPP_29_274_XML_SEEN */

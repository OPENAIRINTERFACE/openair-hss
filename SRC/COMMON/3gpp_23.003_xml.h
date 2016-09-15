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

#ifndef FILE_3GPP_23_003_XML_SEEN
#define FILE_3GPP_23_003_XML_SEEN

#include "xml_load.h"
//==============================================================================
// 2 Identification of mobile subscribers
//==============================================================================

//------------------------------------------------------------------------------
// 2.2 Composition of IMSI
//------------------------------------------------------------------------------
#define IMSI_IE_XML_STR         "imsi"

void imsi_to_xml (const imsi_t * const imsi, xmlTextWriterPtr writer);

//------------------------------------------------------------------------------
// 2.4 Structure of TMSI
//------------------------------------------------------------------------------
#define TMSI_IE_XML_STR                "tmsi"
#define TMSI_XML_SCAN_FMT              "%"SCNx32
#define TMSI_XML_FMT                   "%"PRIx32

NUM_FROM_XML_PROTOTYPE(tmsi);
void tmsi_to_xml (const tmsi_t * const tmsi, xmlTextWriterPtr writer);

//------------------------------------------------------------------------------
// 2.8 Globally Unique Temporary UE Identity (GUTI)
//------------------------------------------------------------------------------
#define GUTI_IE_XML_STR                  "guti"
#define GUMMEI_IE_XML_STR                "gummei"
#define MME_GID_IE_XML_STR               "mme_group_identifier"
#define MME_GID_XML_SCAN_FMT             "%"SCNx16
#define MME_GID_XML_FMT                  "%"PRIx16
#define MME_CODE_IE_XML_STR              "mme_code"
#define MME_CODE_XML_SCAN_FMT            "%"SCNx8
#define MME_CODE_XML_FMT                 "%"PRIx8
#define M_TMSI_IE_XML_STR                "m_tmsi"
#define M_TMSI_XML_SCAN_FMT              "%"SCNx32
#define M_TMSI_XML_FMT                   "%"PRIx32

bool m_tmsi_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, tmsi_t * const m_tmsi);
void m_tmsi_to_xml (const tmsi_t * const m_tmsi, xmlTextWriterPtr writer);
NUM_FROM_XML_PROTOTYPE(mme_code);
void mme_code_to_xml (const mme_code_t * const mme_code, xmlTextWriterPtr writer);
NUM_FROM_XML_PROTOTYPE(mme_gid);
void mme_gid_to_xml (const mme_gid_t * const mme_gid, xmlTextWriterPtr writer);
bool gummei_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, gummei_t * const gummei);
void gummei_to_xml (const gummei_t * const gummei, xmlTextWriterPtr writer);
bool guti_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, guti_t * const guti);
void guti_to_xml (const guti_t * const guti, xmlTextWriterPtr writer);

//------------------------------------------------------------------------------
// 2.9 Structure of the S-Temporary Mobile Subscriber Identity (S-TMSI)
//------------------------------------------------------------------------------
#define S_TMSI_IE_XML_STR                  "s_tmsi"
bool s_tmsi_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, s_tmsi_t * const s_tmsi);
void s_tmsi_to_xml (const s_tmsi_t * const s_tmsi, xmlTextWriterPtr writer);

//==============================================================================
// 4 Identification of location areas and base stations
//==============================================================================

//------------------------------------------------------------------------------
// 4.7  Closed Subscriber Group
//------------------------------------------------------------------------------
#define CSG_ID_IE_XML_STR                  "csg_id"

bool csg_id_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, csg_id_t * const csg_id);
void csg_id_to_xml (const csg_id_t * const csg_id, xmlTextWriterPtr writer);


//==============================================================================
// 6 International Mobile Station Equipment Identity and Software Version Number
//==============================================================================

//------------------------------------------------------------------------------
// 6.2.1 Composition of IMEI
//------------------------------------------------------------------------------
#define IMEI_IE_XML_STR                               "imeisv"
void imei_to_xml (const imei_t * const imei, xmlTextWriterPtr writer);

//------------------------------------------------------------------------------
// 6.2.2 Composition of IMEISV
//------------------------------------------------------------------------------
#define IMEISV_IE_XML_STR                               "imeisv"
void imeisv_to_xml (const imeisv_t * const imeisv, xmlTextWriterPtr writer);


//==============================================================================
// 19  Numbering, addressing and identification for the Evolved Packet Core (EPC)
//==============================================================================

//------------------------------------------------------------------------------
// 19.6  E-UTRAN Cell Identity (ECI) and E-UTRAN Cell Global Identification (ECGI)
//------------------------------------------------------------------------------
#define E_UTRAN_CELL_IDENTITY_IE_XML_STR                "eci"
#define E_UTRAN_CELL_GLOBAL_IDENTIFICATION_IE_XML_STR   "ecgi"

bool eci_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    eci_t                 * const eci);
void eci_to_xml (const eci_t * const ecgi, xmlTextWriterPtr writer);
bool ecgi_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    ecgi_t                * const ecgi);
void ecgi_to_xml (const ecgi_t * const ecgi, xmlTextWriterPtr writer);

#endif /* FILE_3GPP_23_003_XML_SEEN */

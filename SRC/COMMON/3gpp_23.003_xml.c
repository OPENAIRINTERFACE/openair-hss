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

/*! \file 3gpp_23.003_xml.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "common_defs.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "3gpp_23.003_xml.h"

#include "xml_load.h"
#include "3gpp_24.008_xml.h"
#include "mme_scenario_player.h"
#include "xml2_wrapper.h"
#include "gcc_diag.h"
#include "conversions.h"
#include "log.h"

//==============================================================================
// 2 Identification of mobile subscribers
//==============================================================================

void imsi_to_xml (const imsi_t * const imsi, xmlTextWriterPtr writer)
{
  uint8_t digits[IMSI_BCD_DIGITS_MAX+1];
  int j = 0;
  for (int i = 0; i < imsi->length; i++) {
    if ((9 >= (imsi->u.value[i] & 0x0F)) && (j < IMSI_BCD_DIGITS_MAX)){
      digits[j++]=imsi->u.value[i] & 0x0F;
    }
    if ((0x90 >= (imsi->u.value[i] & 0xF0)) && (j < IMSI_BCD_DIGITS_MAX)){
      digits[j++]=(imsi->u.value[i] & 0xF0) >> 4;
    }
  }
  OAI_GCC_DIAG_OFF(address);
  XML_WRITE_HEX_ELEMENT(writer, IMSI_XML_STR, digits, j);
  OAI_GCC_DIAG_ON(address);
}

//------------------------------------------------------------------------------
// 2.4 Structure of TMSI
//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( tmsi , TMSI );

//------------------------------------------------------------------------------
void tmsi_to_xml (const tmsi_t * const tmsi, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, TMSI_XML_STR, "0x%08"PRIx32, *tmsi);
}

//------------------------------------------------------------------------------
// 2.8 Globally Unique Temporary UE Identity (GUTI)
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
bool m_tmsi_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    tmsi_t                    * const m_tmsi)
{
  OAILOG_FUNC_IN (LOG_XML);
  bstring xpath_expr = bformat("./%s",M_TMSI_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx32, (void*)m_tmsi, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
void m_tmsi_to_xml (const tmsi_t * const m_tmsi, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, M_TMSI_XML_STR, "0x%"PRIx32, *m_tmsi);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( mme_code , MME_CODE );

//------------------------------------------------------------------------------
void mme_code_to_xml (const mme_code_t * const mme_code, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, MME_CODE_XML_STR, "0x%"PRIx8, *mme_code);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( mme_gid , MME_GID );
//------------------------------------------------------------------------------
void mme_gid_to_xml (const mme_gid_t * const mme_gid, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, MME_GID_XML_STR, "0x%"PRIx16, *mme_gid);
}

//------------------------------------------------------------------------------
bool gummei_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    gummei_t                  * const gummei)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",GUMMEI_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));
      if (res) {res = plmn_from_xml (xml_doc, xpath_ctx, &gummei->plmn);}
      if (res) {res = mme_gid_from_xml (xml_doc, xpath_ctx, &gummei->mme_gid, NULL);}
      if (res) {res = mme_code_from_xml (xml_doc, xpath_ctx, &gummei->mme_code, NULL);}
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
void gummei_to_xml (const gummei_t * const gummei, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, GUMMEI_XML_STR);
  plmn_to_xml(&gummei->plmn, writer);
  mme_gid_to_xml(&gummei->mme_gid, writer);
  mme_code_to_xml(&gummei->mme_code, writer);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool guti_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    guti_t                    * const guti)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",GUTI_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));
      if (res) {res = gummei_from_xml (xml_doc, xpath_ctx, &guti->gummei);}
      if (res) {res = m_tmsi_from_xml (xml_doc, xpath_ctx, &guti->m_tmsi);}
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
void guti_to_xml (const guti_t * const guti, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, GUTI_XML_STR);
  gummei_to_xml(&guti->gummei, writer);
  m_tmsi_to_xml(&guti->m_tmsi, writer);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
// 2.9 Structure of the S-Temporary Mobile Subscriber Identity (S-TMSI)
//------------------------------------------------------------------------------

bool s_tmsi_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    s_tmsi_t                  * const s_tmsi)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",S_TMSI_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));
      if (res) {res = mme_code_from_xml(xml_doc, xpath_ctx, &s_tmsi->mme_code, NULL);}
      if (res) {res = m_tmsi_from_xml(xml_doc, xpath_ctx, &s_tmsi->m_tmsi);}
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void s_tmsi_to_xml (const s_tmsi_t * const s_tmsi, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, S_TMSI_XML_STR);
  mme_code_to_xml(&s_tmsi->mme_code, writer);
  m_tmsi_to_xml(&s_tmsi->m_tmsi, writer);
  XML_WRITE_END_ELEMENT(writer);
}

//==============================================================================
// 4 Identification of location areas and base stations
//==============================================================================

//------------------------------------------------------------------------------
// 4.7  Closed Subscriber Group
//------------------------------------------------------------------------------

bool csg_id_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    csg_id_t                  * const csg_id)
{
  OAILOG_FUNC_IN (LOG_XML);
  bstring xpath_expr = bformat("./%s",CSG_ID_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx32, (void*)csg_id, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
void csg_id_to_xml (const csg_id_t * const csg_id, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, CSG_ID_XML_STR, "%07"PRIx32, *csg_id);
}

//==============================================================================
// 6 International Mobile Station Equipment Identity and Software Version Number
//==============================================================================

//------------------------------------------------------------------------------
// 6.2.1 Composition of IMEI
//------------------------------------------------------------------------------
void imei_to_xml (const imei_t * const imei, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, IMEI_XML_STR, "%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x",
      imei->u.num.tac1,imei->u.num.tac2,imei->u.num.tac3,imei->u.num.tac4,imei->u.num.tac5,imei->u.num.tac6,imei->u.num.tac7,imei->u.num.tac8,
      imei->u.num.snr1,imei->u.num.snr2,imei->u.num.snr3,imei->u.num.snr4,imei->u.num.snr5,imei->u.num.snr6, imei->u.num.cdsd);
}

//------------------------------------------------------------------------------
// 6.2.2 Composition of IMEISV
//------------------------------------------------------------------------------
void imeisv_to_xml (const imeisv_t * const imeisv, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, IMEISV_XML_STR, "%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x",
      imeisv->u.num.tac1,imeisv->u.num.tac2,imeisv->u.num.tac3,imeisv->u.num.tac4,imeisv->u.num.tac5,imeisv->u.num.tac6,imeisv->u.num.tac7,imeisv->u.num.tac8,
      imeisv->u.num.snr1,imeisv->u.num.snr2,imeisv->u.num.snr3,imeisv->u.num.snr4,imeisv->u.num.snr5,imeisv->u.num.snr6, imeisv->u.num.svn1, imeisv->u.num.svn2);
}

//==============================================================================
// 19  Numbering, addressing and identification for the Evolved Packet Core (EPC)
//==============================================================================

//------------------------------------------------------------------------------
// 19.6  E-UTRAN Cell Identity (ECI) and E-UTRAN Cell Global Identification (ECGI)
//------------------------------------------------------------------------------
#define E_UTRAN_CELL_IDENTITY_XML_STR                "eci"
#define E_UTRAN_CELL_GLOBAL_IDENTIFICATION_XML_STR   "ecgi"

//------------------------------------------------------------------------------
bool eci_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    eci_t                 * const eci)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",E_UTRAN_CELL_IDENTITY_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (xml_doc)) {
      xmlChar *key = xmlNodeListGetString(xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
      uint32_t enb_id = 0;
      uint32_t cell_id = 0;
      int ret = sscanf((const char*)key, "%05x%02x", &enb_id, &cell_id);
      xmlFree(key);
      if (2 == ret) {
        eci->enb_id = enb_id;
        eci->cell_id = cell_id;
        res = true;
        OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s = %05x%02x\n",
            E_UTRAN_CELL_IDENTITY_XML_STR,
            eci->enb_id, eci->cell_id);
      }
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void eci_to_xml (const eci_t * const eci, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, E_UTRAN_CELL_IDENTITY_XML_STR, "%05x%02x", eci->enb_id, eci->cell_id);
}

//------------------------------------------------------------------------------
bool ecgi_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    ecgi_t                * const ecgi)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",E_UTRAN_CELL_GLOBAL_IDENTIFICATION_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));
      if (res) {res = plmn_from_xml (xml_doc, xpath_ctx, &ecgi->plmn);}
      if (res) {res = eci_from_xml (xml_doc, xpath_ctx, &ecgi->cell_identity);}
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void ecgi_to_xml (const ecgi_t * const ecgi, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, E_UTRAN_CELL_GLOBAL_IDENTIFICATION_XML_STR);
  plmn_to_xml (&ecgi->plmn, writer);
  eci_to_xml (&ecgi->cell_identity, writer);
  XML_WRITE_END_ELEMENT(writer);
}


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
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "log.h"
#include "assertions.h"
#include "xml2_wrapper.h"
#include "common_defs.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "mme_scenario_player.h"
#include "3gpp_36.401.h"
#include "3gpp_36.401_xml.h"
#include "sp_3gpp_36.401_xml.h"
#include "3gpp_36.331.h"
#include "3gpp_23.003_xml.h"
#include "sp_3gpp_23.003_xml.h"

#include "gcc_diag.h"
#include "conversions.h"
#include "sp_3gpp_24.008_xml.h"

//==============================================================================
// 2 Identification of mobile subscribers
//==============================================================================


//------------------------------------------------------------------------------
// 2.4 Structure of TMSI
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( tmsi , TMSI );

//------------------------------------------------------------------------------
// 2.8 Globally Unique Temporary UE Identity (GUTI)
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// TODO ? type m_tmsi_t
bool sp_m_tmsi_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    tmsi_t                    * const m_tmsi)
{
  bstring xpath_expr = bformat("./%s",M_TMSI_IE_XML_STR);
  bstring xml_var = NULL;
  bool res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, M_TMSI_XML_SCAN_FMT, (void*)m_tmsi, &xml_var);
  if (!res && xml_var) {
    if ('$' == xml_var->data[0]) {
      if (BSTR_OK == bdelete(xml_var, 0, 1)) {
        // Check if var already exists
        scenario_player_item_t * spi_var = sp_get_var(scenario, (unsigned char *)bdata(xml_var));
        AssertFatal(spi_var, "var %s not found", bdata(xml_var));
        if (SCENARIO_PLAYER_ITEM_VAR == spi_var->item_type) {
          *m_tmsi = (tmsi_t)spi_var->u.var.value.value_u64;
          res = true;
          OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Set %s=" M_TMSI_XML_FMT " from var uid=0x%lx\n",
              M_TMSI_IE_XML_STR, *m_tmsi, (uintptr_t)spi_var->uid);
        } else {
          AssertFatal (0, "Could not find %s var uid, should have been declared in scenario\n", M_TMSI_IE_XML_STR);
        }
      }
    } else if ('#' == xml_var->data[0]) {
      if (BSTR_OK == bdelete(xml_var, 0, 1)) {
        // Check if var already exists
        scenario_player_item_t * spi_var = sp_get_var(scenario, (unsigned char *)bdata(xml_var));
        AssertFatal(spi_var, "var %s not found", bdata(xml_var));
        if (SCENARIO_PLAYER_ITEM_VAR == spi_var->item_type) {
          res = true;
          OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Set %s to be loaded\n", M_TMSI_IE_XML_STR);
        }
      }
    }
    bdestroy_wrapper (&xml_var);
  }
  bdestroy_wrapper (&xpath_expr);
  return res;
}

//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE(mme_code, MME_CODE);
SP_NUM_FROM_XML_GENERATE(mme_gid, MME_GID);

//------------------------------------------------------------------------------
bool sp_gummei_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    gummei_t                  * const gummei)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;
  bstring xpath_expr = bformat("./%s",GUMMEI_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (msg->xml_doc)) {
      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], msg->xpath_ctx));
      if (res) {res = sp_plmn_from_xml (scenario, msg, &gummei->plmn);}
      if (res) {res = sp_mme_gid_from_xml (scenario, msg, &gummei->mme_gid);}
      if (res) {res = sp_mme_code_from_xml (scenario, msg, &gummei->mme_code);}
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_guti_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    guti_t                    * const guti)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;
  bstring xpath_expr = bformat("./%s",GUTI_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (msg->xml_doc)) {
      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], msg->xpath_ctx));
      if (res) {res = sp_gummei_from_xml (scenario, msg, &guti->gummei);}
      if (res) {res = sp_m_tmsi_from_xml (scenario, msg, &guti->m_tmsi);}
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
// 2.9 Structure of the S-Temporary Mobile Subscriber Identity (S-TMSI)
//------------------------------------------------------------------------------

bool sp_s_tmsi_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    s_tmsi_t                  * const s_tmsi)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;
  bstring xpath_expr = bformat("./%s",S_TMSI_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (msg->xml_doc)) {

      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], msg->xpath_ctx));
      if (res) {res = sp_mme_code_from_xml(scenario, msg, &s_tmsi->mme_code);}
      if (res) {res = sp_m_tmsi_from_xml(scenario, msg, &s_tmsi->m_tmsi);}
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}


//==============================================================================
// 4 Identification of location areas and base stations
//==============================================================================

//------------------------------------------------------------------------------
// 4.7  Closed Subscriber Group
//------------------------------------------------------------------------------

bool sp_csg_id_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    csg_id_t                  * const csg_id)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bstring xpath_expr = bformat("./%s",CSG_ID_IE_XML_STR);
  bool res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, "%"SCNx32, (void*)csg_id, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}
//==============================================================================
// 6 International Mobile Station Equipment Identity and Software Version Number
//==============================================================================

//------------------------------------------------------------------------------
// 6.2.1 Composition of IMEI
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// 6.2.2 Composition of IMEISV
//------------------------------------------------------------------------------

//==============================================================================
// 19  Numbering, addressing and identification for the Evolved Packet Core (EPC)
//==============================================================================

//------------------------------------------------------------------------------
// 19.6  E-UTRAN Cell Identity (ECI) and E-UTRAN Cell Global Identification (ECGI)
//------------------------------------------------------------------------------
#define E_UTRAN_CELL_IDENTITY_IE_XML_STR                "eci"
#define E_UTRAN_CELL_GLOBAL_IDENTIFICATION_IE_XML_STR   "ecgi"

//------------------------------------------------------------------------------
bool sp_eci_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    eci_t                 * const eci)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;
  bstring xpath_expr = bformat("./%s",E_UTRAN_CELL_IDENTITY_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (msg->xml_doc)) {
      xmlChar *key = xmlNodeListGetString(msg->xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
      uint32_t enb_id = 0;
      uint32_t cell_id = 0;
      int ret = sscanf((const char*)key, "%05x%02x", &enb_id, &cell_id);
      xmlFree(key);
      if (2 == ret) {
        eci->enb_id = enb_id;
        eci->cell_id = cell_id;
        res = true;
        OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s = %05x%02x\n",
            E_UTRAN_CELL_IDENTITY_IE_XML_STR,
            eci->enb_id, eci->cell_id);
      }
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}



//------------------------------------------------------------------------------
bool sp_ecgi_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    ecgi_t                * const ecgi)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;
  bstring xpath_expr = bformat("./%s",E_UTRAN_CELL_GLOBAL_IDENTIFICATION_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (msg->xml_doc)) {
      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], msg->xpath_ctx));
      if (res) {res = sp_plmn_from_xml (scenario, msg, &ecgi->plmn);}
      if (res) {res = sp_eci_from_xml (scenario, msg, &ecgi->cell_identity);}
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}



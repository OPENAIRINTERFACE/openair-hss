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
#include "common_defs.h"
#include "common_types.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "3gpp_36.413.h"

#include "sgw_ie_defs.h"
#include "3gpp_29.274.h"
#include "3gpp_29.274_xml.h"

#include "xml_load.h"
#include "xml_msg_tags.h"
#include "3gpp_24.008_xml.h"
#include "3gpp_24.007_xml.h"
#include "3gpp_36.413_xml.h"
#include "mme_scenario_player.h"
#include "xml2_wrapper.h"
#include "gcc_diag.h"
#include "conversions.h"
#include "assertions.h"
#include "log.h"

static char* g_interface_type_2_string[] = {
    "S1_U_ENODEB_GTP_U",
    "S1_U_SGW_GTP_U",
    "S12_RNC_GTP_U",
    "S12_SGW_GTP_U",
    "S5_S8_SGW_GTP_U",
    "S5_S8_PGW_GTP_U",
    "S5_S8_SGW_GTP_C",
    "S5_S8_PGW_GTP_C",
    "S5_S8_SGW_PMIPv6",
    "S5_S8_PGW_PMIPv6",
    "S11_MME_GTP_C",
    "S11_SGW_GTP_C",
    "S10_MME_GTP_C",
    "S3_MME_GTP_C",
    "S3_SGSN_GTP_C",
    "S4_SGSN_GTP_U",
    "S4_SGW_GTP_U",
    "S4_SGSN_GTP_C",
    "S16_SGSN_GTP_C",
    "ENODEB_GTP_U_DL_DATA_FORWARDING",
    "ENODEB_GTP_U_UL_DATA_FORWARDING",
    "RNC_GTP_U_DATA_FORWARDING",
    "SGSN_GTP_U_DATA_FORWARDING",
    "SGW_GTP_U_DL_DATA_FORWARDING",
    "SM_MBMS_GW_GTP_C",
    "SN_MBMS_GW_GTP_C",
    "SM_MME_GTP_C",
    "SN_SGSN_GTP_C",
    "SGW_GTP_U_UL_DATA_FORWARDING",
    "SN_SGSN_GTP_U",
    "S2B_EPDG_GTP_C"
};
//------------------------------------------------------------------------------
void ambr_to_xml(const ambr_t * const ambr, const char * const xml_tag,  xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, xml_tag);
  XML_WRITE_FORMAT_ELEMENT(writer, BITRATE_UL_XML_STR, "%"PRIu64, ambr->br_ul);
  XML_WRITE_FORMAT_ELEMENT(writer, BITRATE_DL_XML_STR, "%"PRIu64, ambr->br_dl);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool ambr_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, const char * const xml_tag, ambr_t * const ambr)
{
  bool res = false;
  bstring xpath_expr_ambr = bformat("./%s", xml_tag);
  xmlXPathObjectPtr xpath_obj_ambr = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_ambr);
  if (xpath_obj_ambr) {
    xmlNodeSetPtr nodes_ambr = xpath_obj_ambr->nodesetval;
    int size = (nodes_ambr) ? nodes_ambr->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_ambr->nodeTab[0], xpath_ctx));
      if (res) {
        res = false;
        uint64_t  br = 0;
        bstring xpath_expr = bformat("./%s",BITRATE_UL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu64, (void*)&br, NULL);
        bdestroy_wrapper (&xpath_expr);
        ambr->br_ul = br;

        xpath_expr = bformat("./%s",BITRATE_DL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu64, (void*)&br, NULL);
        bdestroy_wrapper (&xpath_expr);
        ambr->br_dl = br;
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_ambr);
  }
  bdestroy_wrapper(&xpath_expr_ambr);
  return res;
}

//------------------------------------------------------------------------------
void bearer_qos_to_xml(const bearer_qos_t * const bearer_qos, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, BEARER_CONTEXT_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, PRIORITY_LEVEL_IE_XML_STR, "%"PRIu8, bearer_qos->pl);
  XML_WRITE_FORMAT_ELEMENT(writer, PRE_EMPTION_CAPABILITY_IE_XML_STR, "%"PRIu8, bearer_qos->pci);
  XML_WRITE_FORMAT_ELEMENT(writer, PRE_EMPTION_VULNERABILITY_IE_XML_STR, "%"PRIu8, bearer_qos->pvi);
  qci_to_xml(&bearer_qos->qci, writer);
  ambr_to_xml(&bearer_qos->gbr, GUARANTED_AMBR_XML_STR, writer);
  ambr_to_xml(&bearer_qos->mbr, MAXIMUM_AMBR_XML_STR, writer);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool bearer_qos_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, bearer_qos_t * const bearer_qos)
{
  bool res = false;
  bstring xpath_expr_bc = bformat("./%s", BEARER_CONTEXT_XML_STR);
  xmlXPathObjectPtr xpath_obj_bc = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_bc);
  if (xpath_obj_bc) {
    xmlNodeSetPtr nodes_bc = xpath_obj_bc->nodesetval;
    int size = (nodes_bc) ? nodes_bc->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_bc->nodeTab[0], xpath_ctx));
      if (res) {
        res = qci_from_xml (xml_doc, xpath_ctx, &bearer_qos->qci);

        if (res) {
          res = ambr_from_xml (xml_doc, xpath_ctx, GUARANTED_AMBR_XML_STR, &bearer_qos->gbr);
        }
        if (res) {
          res = ambr_from_xml (xml_doc, xpath_ctx, MAXIMUM_AMBR_XML_STR, &bearer_qos->mbr);
        }

        if (res) {
          unsigned  pl = 0;
          bstring xpath_expr = bformat("./%s", PRIORITY_LEVEL_IE_XML_STR);
          res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&pl, NULL);
          bdestroy_wrapper (&xpath_expr);
          bearer_qos->pl = pl;
        }
        if (res) {
          unsigned  pci = 0;
          bstring xpath_expr = bformat("./%s", PRE_EMPTION_CAPABILITY_IE_XML_STR);
          res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&pci, NULL);
          bdestroy_wrapper (&xpath_expr);
          bearer_qos->pci = pci;
        }
        if (res) {
          unsigned  pvi = 0;
          bstring xpath_expr = bformat("./%s", PRE_EMPTION_VULNERABILITY_IE_XML_STR);
          res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&pvi, NULL);
          bdestroy_wrapper (&xpath_expr);
          bearer_qos->pvi = pvi;
        }
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_bc);
  }
  bdestroy_wrapper(&xpath_expr_bc);
  return res;
}


//------------------------------------------------------------------------------
void interface_type_to_xml (const interface_type_t * const interface_type, xmlTextWriterPtr writer)
{
  if ((INTERFACE_TYPE_MIN <= *interface_type) && (INTERFACE_TYPE_MAX >= *interface_type)) {
    XML_WRITE_FORMAT_ELEMENT(writer, INTERFACE_TYPE_XML_STR, "%s", g_interface_type_2_string[*interface_type]);
  } else {
    XML_WRITE_FORMAT_ELEMENT(writer, INTERFACE_TYPE_XML_STR, "%d", *interface_type);
  }
}

//------------------------------------------------------------------------------
bool interface_type_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, interface_type_t * const interface_type)
{
  bstring xpath_expr = bformat("./%s",INTERFACE_TYPE_XML_STR);
  bstring failed_value = NULL;
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%d", (void*)&interface_type, &failed_value);
  if ((!res) && (failed_value)) {
    for (int i = 0; i <= INTERFACE_TYPE_MAX /*be carefull INTERFACE_TYPE_MIN = 0*/; i++) {
      if (biseqcstr(failed_value, g_interface_type_2_string[i])) {
        *interface_type = i;
        break;
      }
    }

    bdestroy_wrapper(&failed_value);
  }
  bdestroy_wrapper(&xpath_expr);
  return res;
}


NUM_FROM_XML_GENERATE( teid , TEID );

//------------------------------------------------------------------------------
void teid_to_xml (const teid_t * const teid, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, TEID_IE_XML_STR, TEID_XML_FMT, *teid);
}

//------------------------------------------------------------------------------
void ipv4_address_to_xml (const struct in_addr * const in_addr, xmlTextWriterPtr writer)
{
  char          str[INET_ADDRSTRLEN];

  inet_ntop(AF_INET, in_addr, str, INET_ADDRSTRLEN);
  XML_WRITE_FORMAT_ELEMENT(writer, IPV4_ADDRESS_XML_STR, "%s", str);
}

//------------------------------------------------------------------------------
void ipv6_address_to_xml (const struct in6_addr * const in6_addr, xmlTextWriterPtr writer)
{
  char          str[INET6_ADDRSTRLEN];

  inet_ntop(AF_INET6, in6_addr, str, INET6_ADDRSTRLEN);
  XML_WRITE_FORMAT_ELEMENT(writer, IPV6_ADDRESS_XML_STR, "%s", str);
}

//------------------------------------------------------------------------------
void fteid_to_xml (const fteid_t * const fteid, const char * const xml_tag, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, xml_tag);
  interface_type_to_xml(&fteid->interface_type, writer);
  teid_to_xml(&fteid->teid, writer);
  if (fteid->ipv4) {
    ipv4_address_to_xml(&fteid->ipv4_address, writer);
  }
  if (fteid->ipv6) {
    ipv6_address_to_xml(&fteid->ipv6_address, writer);
  }
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
void bearer_context_within_create_bearer_request_to_xml (const bearer_context_within_create_bearer_request_t * const bearer_context, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, BEARER_CONTEXT_XML_STR);
  eps_bearer_identity_to_xml(&bearer_context->eps_bearer_id, writer);
  traffic_flow_template_to_xml(&bearer_context->tft, writer);
  if (bearer_context->s1u_sgw_fteid.teid) {
    fteid_to_xml(&bearer_context->s1u_sgw_fteid, S1U_SGW_FTEID_XML_STR, writer);
  }
  if (bearer_context->s5_s8_u_pgw_fteid.teid) {
    fteid_to_xml(&bearer_context->s5_s8_u_pgw_fteid, S5_S8_U_PGW_FTEID_XML_STR, writer);
  }
  if (bearer_context->s12_sgw_fteid.teid) {
    fteid_to_xml(&bearer_context->s12_sgw_fteid, S12_SGW_FTEID_XML_STR, writer);
  }
  if (bearer_context->s4_u_sgw_fteid.teid) {
    fteid_to_xml(&bearer_context->s4_u_sgw_fteid, S4_U_SGW_FTEID_XML_STR, writer);
  }
  if (bearer_context->s2b_u_pgw_fteid.teid) {
    fteid_to_xml(&bearer_context->s2b_u_pgw_fteid, S2B_U_PGW_FTEID_XML_STR, writer);
  }
  if (bearer_context->s2a_u_pgw_fteid.teid) {
    fteid_to_xml(&bearer_context->s2a_u_pgw_fteid, S2A_U_PGW_FTEID_XML_STR, writer);
  }
  bearer_qos_to_xml(&bearer_context->bearer_level_qos, writer);
  protocol_configuration_options_to_xml(&bearer_context->pco, writer, false);
  XML_WRITE_END_ELEMENT(writer);

}

//------------------------------------------------------------------------------
void bearer_contexts_within_create_bearer_request_to_xml (const bearer_contexts_within_create_bearer_request_t * const bearer_contexts, xmlTextWriterPtr writer)
{
  for (int i = 0; i < bearer_contexts->num_bearer_context; i++) {
    bearer_context_within_create_bearer_request_to_xml(&bearer_contexts->bearer_contexts[i], writer);
  }
}

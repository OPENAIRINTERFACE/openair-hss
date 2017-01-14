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
#include <pthread.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "3gpp_33.401.h"
#include "security_types.h"
#include "common_types.h"
#include "3gpp_36.413.h"
#include "common_defs.h"
#include "mme_scenario_player.h"
#include "3gpp_36.413_xml.h"
#include "conversions.h"
#include "nas_message.h"
#include "nas_message_xml.h"
#include "S1ap-Cause.h"

#include "xml_load.h"
#include "xml2_wrapper.h"
#include "xml_msg_tags.h"
#include "log.h"

//------------------------------------------------------------------------------
bool e_rab_item_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    e_rab_item_t * const item)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = true;

  res = e_rab_id_from_xml (xml_doc, xpath_ctx, &item->e_rab_id);
  // TODO s1ap_cause_from_xml(xml_doc, xpath_ctx, &item->cause);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void e_rab_item_to_xml (const e_rab_item_t * const item, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, E_RAB_ITEM_XML_STR);
  e_rab_id_to_xml(&item->e_rab_id, writer);
  // TODO s1ap_cause_to_xml(&item->cause, writer);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool e_rab_setup_item_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    e_rab_setup_item_t * const item)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = true;

  res = e_rab_id_from_xml (xml_doc, xpath_ctx, &item->e_rab_id);
  if (res) {
    bstring xpath_expr_tla = bformat("./%s",TRANSPORT_LAYER_ADDRESS_XML_STR);
    res = xml_load_hex_stream_leaf_tag(xml_doc, xpath_ctx, xpath_expr_tla, &item->transport_layer_address);
    bdestroy_wrapper (&xpath_expr_tla);
  }
  if (res) {
    bstring xpath_expr = bformat("./%s",TEID_XML_STR);
    res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx32, (void*)&item->gtp_teid, NULL);
    bdestroy_wrapper (&xpath_expr);
  }

  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void e_rab_setup_item_to_xml (const e_rab_setup_item_t * const item, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, E_RAB_SETUP_ITEM_XML_STR);
  e_rab_id_to_xml(&item->e_rab_id, writer);
  XML_WRITE_HEX_ELEMENT(writer, TRANSPORT_LAYER_ADDRESS_XML_STR,
      bdata(item->transport_layer_address),
      blength(item->transport_layer_address));
  XML_WRITE_FORMAT_ELEMENT(writer, TEID_XML_STR, "%"PRIx32, item->gtp_teid);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool e_rab_setup_list_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    e_rab_setup_list_t        * const list)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  list->no_of_items = 0;
  bstring xpath_expr = bformat("./%s",E_RAB_SETUP_LIST_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));

      if (res) {
        bstring xpath_expr_items = bformat("./%s",E_RAB_SETUP_ITEM_XML_STR);
        xmlXPathObjectPtr xpath_obj_items = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_items);
        if (xpath_obj_items) {
          xmlNodeSetPtr items = xpath_obj_items->nodesetval;
          int no_of_items = (items) ? items->nodeNr : 0;
          for (int item = 0; item < no_of_items; item++) {
            xmlNodePtr saved_node_ptr2 = xpath_ctx->node;
            res = (RETURNok == xmlXPathSetContextNode(items->nodeTab[item], xpath_ctx));
            if (res) res = e_rab_setup_item_from_xml (xml_doc, xpath_ctx, &list->item[list->no_of_items]);
            if (res) list->no_of_items++;
            res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr2, xpath_ctx)) & res;
          }
          xmlXPathFreeObject(xpath_obj_items);
        }
        bdestroy_wrapper (&xpath_expr_items);
      }

      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void e_rab_setup_list_to_xml (const e_rab_setup_list_t * const list, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, E_RAB_SETUP_LIST_XML_STR);
  for (int item = 0; item < list->no_of_items; item++) {
    e_rab_setup_item_to_xml (&list->item[item], writer);
  }
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool e_rab_list_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    e_rab_list_t              * const list,
    char * const xml_tag)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  list->no_of_items = 0;
  bstring xpath_expr = NULL;
  if (xml_tag) {
    xpath_expr = bformat("./%s",xml_tag);
  } else {
    xpath_expr = bformat("./%s",E_RAB_LIST_XML_STR);
  }
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));

      if (res) {
        bstring xpath_expr_items = bformat("./%s",E_RAB_ITEM_XML_STR);
        xmlXPathObjectPtr xpath_obj_items = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_items);
        if (xpath_obj_items) {
          xmlNodeSetPtr items = xpath_obj_items->nodesetval;
          int no_of_items = (items) ? items->nodeNr : 0;
          for (int item = 0; item < no_of_items; item++) {
            xmlNodePtr saved_node_ptr2 = xpath_ctx->node;
            res = (RETURNok == xmlXPathSetContextNode(items->nodeTab[item], xpath_ctx));
            if (res) res = e_rab_item_from_xml (xml_doc, xpath_ctx, &list->item[list->no_of_items]);
            if (res) list->no_of_items++;
            res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr2, xpath_ctx)) & res;
          }
          xmlXPathFreeObject(xpath_obj_items);
        }
        bdestroy_wrapper (&xpath_expr_items);
      }

      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void e_rab_list_to_xml (const e_rab_list_t * const list, xmlTextWriterPtr writer, char * const xml_tag)
{
  if (xml_tag) {
    XML_WRITE_START_ELEMENT(writer, xml_tag);
  } else {
    XML_WRITE_START_ELEMENT(writer, E_RAB_LIST_XML_STR);
  }
  for (int item = 0; item < list->no_of_items; item++) {
    e_rab_item_to_xml(&list->item[item], writer);
  }
  XML_WRITE_END_ELEMENT(writer);
}
//------------------------------------------------------------------------------
bool e_rab_to_be_setup_list_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    e_rab_to_be_setup_list_t * const list)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  list->no_of_items = 0;
  bstring xpath_expr = bformat("./%s",E_RAB_TO_BE_SETUP_LIST_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));

      if (res) {
        bstring xpath_expr_items = bformat("./%s",E_RAB_TO_BE_SETUP_ITEM_XML_STR);
        xmlXPathObjectPtr xpath_obj_items = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_items);
        if (xpath_obj_items) {
          xmlNodeSetPtr items = xpath_obj_items->nodesetval;
          int no_of_items = (items) ? items->nodeNr : 0;
          for (int item = 0; item < no_of_items; item++) {
            xmlNodePtr saved_node_ptr2 = xpath_ctx->node;
            res = (RETURNok == xmlXPathSetContextNode(items->nodeTab[item], xpath_ctx));
            if (res) res = e_rab_to_be_setup_item_from_xml (xml_doc, xpath_ctx, &list->item[list->no_of_items]);
            if (res) list->no_of_items++;
            res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr2, xpath_ctx)) & res;
          }
          xmlXPathFreeObject(xpath_obj_items);
        }
        bdestroy_wrapper (&xpath_expr_items);
      }

      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void e_rab_to_be_setup_list_to_xml (const e_rab_to_be_setup_list_t * const list, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, E_RAB_TO_BE_SETUP_LIST_XML_STR);
  for (int item = 0; item < list->no_of_items; item++) {
    e_rab_to_be_setup_item_to_xml (&list->item[item], writer);
  }
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool e_rab_to_be_setup_item_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    e_rab_to_be_setup_item_t * const item)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = true;

  res = e_rab_id_from_xml (xml_doc, xpath_ctx, &item->e_rab_id);
  if (res) {
    res = e_rab_level_qos_parameters_from_xml (xml_doc, xpath_ctx, &item->e_rab_level_qos_parameters);
  }
  if (res) {

  }

  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void e_rab_to_be_setup_item_to_xml (const e_rab_to_be_setup_item_t * const item, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, E_RAB_TO_BE_SETUP_ITEM_XML_STR);
  e_rab_id_to_xml(&item->e_rab_id, writer);
  e_rab_level_qos_parameters_to_xml(&item->e_rab_level_qos_parameters, writer);
  XML_WRITE_FORMAT_ELEMENT(writer, TEID_XML_STR, "%"PRIx32, item->gtp_teid);
  XML_WRITE_HEX_ELEMENT(writer, TRANSPORT_LAYER_ADDRESS_XML_STR,
           bdata(item->transport_layer_address),
           blength(item->transport_layer_address));

  nas_pdu_to_xml(item->nas_pdu, writer);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool e_rab_id_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, e_rab_id_t * const e_rab_id)
{
  OAILOG_FUNC_IN (LOG_XML);
  bstring xpath_expr = bformat("./%s",E_RAB_ID_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)e_rab_id, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void e_rab_id_to_xml (const e_rab_id_t * const e_rab_id, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, E_RAB_ID_XML_STR, "%"PRIu8, *e_rab_id);
}

//------------------------------------------------------------------------------
bool e_rab_level_qos_parameters_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    e_rab_level_qos_parameters_t * const params)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",E_RAB_LEVEL_QOS_PARAMETERS_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));

      if (res) {
        res = qci_from_xml (xml_doc, xpath_ctx, &params->qci);
      }
      if (res) {
        res = allocation_and_retention_priority_from_xml (xml_doc, xpath_ctx, &params->allocation_and_retention_priority);
      }

      if (res) {
        res = gbr_qos_information_from_xml (xml_doc, xpath_ctx, &params->gbr_qos_information);
      }

      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void e_rab_level_qos_parameters_to_xml (const e_rab_level_qos_parameters_t * const params, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, E_RAB_LEVEL_QOS_PARAMETERS_XML_STR);
  qci_to_xml (&params->qci, writer);
  allocation_and_retention_priority_to_xml (&params->allocation_and_retention_priority, writer);
  gbr_qos_information_to_xml(&params->gbr_qos_information, writer);
  XML_WRITE_END_ELEMENT(writer);
}


//------------------------------------------------------------------------------
bool qci_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, qci_t * const qci)
{
  OAILOG_FUNC_IN (LOG_XML);
  bstring xpath_expr = bformat("./%s",QCI_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)qci, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
void qci_to_xml (const qci_t * const qci, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, QCI_XML_STR, "%"PRIu8, *qci);
}

//------------------------------------------------------------------------------
bool allocation_and_retention_priority_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    allocation_and_retention_priority_t * const arp)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",ALLOCATION_AND_RETENTION_PRIORITY_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));

      bstring xpath_expr_prio = bformat("./%s",PRIORITY_LEVEL_XML_STR);
      res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_prio, "%"SCNu8, (void*)&arp->priority_level, NULL);
      bdestroy_wrapper (&xpath_expr_prio);
      if (res) {
        bstring xpath_expr_cap = bformat("./%s",PRE_EMPTION_CAPABILITY_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_cap, "%"SCNu8, (void*)&arp->pre_emption_capability, NULL);
        bdestroy_wrapper (&xpath_expr_cap);
      }
      if (res) {
        bstring xpath_expr_vul = bformat("./%s",PRE_EMPTION_VULNERABILITY_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_vul, "%"SCNu8, (void*)&arp->pre_emption_vulnerability, NULL);
        bdestroy_wrapper (&xpath_expr_vul);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void allocation_and_retention_priority_to_xml (const allocation_and_retention_priority_t * const arp, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, ALLOCATION_AND_RETENTION_PRIORITY_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, PRIORITY_LEVEL_XML_STR, "%"PRIu8, arp->priority_level);
  XML_WRITE_FORMAT_ELEMENT(writer, PRE_EMPTION_CAPABILITY_XML_STR, "%"PRIu8, arp->pre_emption_capability);
  XML_WRITE_FORMAT_ELEMENT(writer, PRE_EMPTION_VULNERABILITY_XML_STR, "%"PRIu8, arp->pre_emption_vulnerability);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool gbr_qos_information_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    gbr_qos_information_t * const gqi)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",GBR_QOS_INFORMATION_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));

      bstring xpath_expr_prio = bformat("./%s",E_RAB_MAXIMUM_BIT_RATE_DOWNLINK_XML_STR);
      res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_prio, "%"SCNu64, (void*)&gqi->e_rab_maximum_bit_rate_downlink, NULL);
      bdestroy_wrapper (&xpath_expr_prio);
      if (res) {
        bstring xpath_expr_cap = bformat("./%s",E_RAB_MAXIMUM_BIT_RATE_UPLINK_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_cap, "%"SCNu64, (void*)&gqi->e_rab_maximum_bit_rate_uplink, NULL);
        bdestroy_wrapper (&xpath_expr_cap);
      }
      if (res) {
        bstring xpath_expr_vul = bformat("./%s",E_RAB_GUARANTEED_BIT_RATE_DOWNLINK_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_vul, "%"SCNu64, (void*)&gqi->e_rab_guaranteed_bit_rate_downlink, NULL);
        bdestroy_wrapper (&xpath_expr_vul);
      }
      if (res) {
        bstring xpath_expr_vul = bformat("./%s",E_RAB_GUARANTEED_BIT_RATE_UPLINK_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_vul, "%"SCNu64, (void*)&gqi->e_rab_guaranteed_bit_rate_uplink, NULL);
        bdestroy_wrapper (&xpath_expr_vul);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void gbr_qos_information_to_xml (const gbr_qos_information_t * const gqi, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, GBR_QOS_INFORMATION_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, E_RAB_MAXIMUM_BIT_RATE_DOWNLINK_XML_STR, "%"PRIu64, gqi->e_rab_maximum_bit_rate_downlink);
  XML_WRITE_FORMAT_ELEMENT(writer, E_RAB_MAXIMUM_BIT_RATE_UPLINK_XML_STR, "%"PRIu64, gqi->e_rab_maximum_bit_rate_uplink);
  XML_WRITE_FORMAT_ELEMENT(writer, E_RAB_GUARANTEED_BIT_RATE_DOWNLINK_XML_STR, "%"PRIu64, gqi->e_rab_guaranteed_bit_rate_downlink);
  XML_WRITE_FORMAT_ELEMENT(writer, E_RAB_GUARANTEED_BIT_RATE_UPLINK_XML_STR, "%"PRIu64, gqi->e_rab_guaranteed_bit_rate_uplink);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool ue_aggregate_maximum_bit_rate_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    ue_aggregate_maximum_bit_rate_t * const ue_ambr)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",UE_AGGREGATE_MAXIMUM_BIT_RATE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));

      bstring xpath_expr_prio = bformat("./%s",DOWNLINK_XML_STR);
      res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_prio, "%"SCNu64, (void*)&ue_ambr->dl, NULL);
      bdestroy_wrapper (&xpath_expr_prio);
      if (res) {
        bstring xpath_expr_cap = bformat("./%s",UPLINK_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_cap, "%"SCNu64, (void*)&ue_ambr->ul, NULL);
        bdestroy_wrapper (&xpath_expr_cap);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void ue_aggregate_maximum_bit_rate_to_xml (const ue_aggregate_maximum_bit_rate_t * const ue_ambr, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, UE_AGGREGATE_MAXIMUM_BIT_RATE_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, DOWNLINK_XML_STR, "%"PRIu64, ue_ambr->dl);
  XML_WRITE_FORMAT_ELEMENT(writer, UPLINK_XML_STR, "%"PRIu64, ue_ambr->ul);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool s1ap_cause_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, S1ap_Cause_t * const cause)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",S1AP_CAUSE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));

      char group_name_str[256]  = {0};
      bstring xpath_expr_group = bformat("./%s",S1AP_CAUSE_GROUP_XML_STR);
      bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_group, "%s", (void*)group_name_str, NULL);
      bdestroy_wrapper (&xpath_expr_group);
      if (res) {
        bstring xpath_expr_cause_value = bformat("./%s",S1AP_CAUSE_GROUP_CAUSE_XML_STR);
        if (!strcasecmp(group_name_str, S1AP_CAUSE_GROUP_RADIO_NETWORK_LAYER_XML_STR)) {
          cause->present = S1ap_Cause_PR_radioNetwork;
          res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_cause_value, S1AP_CAUSE_GROUP_CAUSE_SCAN_FMT, (void*)&cause->choice.radioNetwork, NULL);
        } else  if (!strcasecmp(group_name_str, S1AP_CAUSE_GROUP_TRANSPORT_LAYER_XML_STR)) {
          cause->present = S1ap_Cause_PR_transport;
          res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_cause_value, S1AP_CAUSE_GROUP_CAUSE_SCAN_FMT, (void*)&cause->choice.transport, NULL);
        } else  if (!strcasecmp(group_name_str, S1AP_CAUSE_GROUP_NAS_XML_STR)) {
          cause->present = S1ap_Cause_PR_nas;
          res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_cause_value, S1AP_CAUSE_GROUP_CAUSE_SCAN_FMT, (void*)&cause->choice.nas, NULL);
        } else  if (!strcasecmp(group_name_str, S1AP_CAUSE_GROUP_PROTOCOL_XML_STR)) {
          cause->present = S1ap_Cause_PR_protocol;
          res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_cause_value, S1AP_CAUSE_GROUP_CAUSE_SCAN_FMT, (void*)&cause->choice.protocol, NULL);
        } else  if (!strcasecmp(group_name_str, S1AP_CAUSE_GROUP_MISC_XML_STR)) {
          cause->present = S1ap_Cause_PR_misc;
          res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_cause_value, S1AP_CAUSE_GROUP_CAUSE_SCAN_FMT, (void*)&cause->choice.misc, NULL);
        } else {
          cause->present = S1ap_Cause_PR_NOTHING;
        }
        bdestroy_wrapper (&xpath_expr_cause_value);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);

}


//------------------------------------------------------------------------------
void s1ap_cause_to_xml (const S1ap_Cause_t * const cause, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, S1AP_CAUSE_XML_STR);
  switch (cause->present) {
    case S1ap_Cause_PR_NOTHING:
      XML_WRITE_FORMAT_ELEMENT(writer, S1AP_CAUSE_GROUP_XML_STR, "%s", S1AP_CAUSE_GROUP_NOTHING_XML_STR);
      break;
    case S1ap_Cause_PR_radioNetwork:
      XML_WRITE_FORMAT_ELEMENT(writer, S1AP_CAUSE_GROUP_XML_STR, "%s", S1AP_CAUSE_GROUP_RADIO_NETWORK_LAYER_XML_STR);
      XML_WRITE_FORMAT_ELEMENT(writer, S1AP_CAUSE_GROUP_CAUSE_XML_STR, S1AP_CAUSE_GROUP_CAUSE_FMT, cause->choice.radioNetwork);
      break;
    case S1ap_Cause_PR_transport:
      XML_WRITE_FORMAT_ELEMENT(writer, S1AP_CAUSE_GROUP_XML_STR, "%s", S1AP_CAUSE_GROUP_TRANSPORT_LAYER_XML_STR);
      XML_WRITE_FORMAT_ELEMENT(writer, S1AP_CAUSE_GROUP_CAUSE_XML_STR, S1AP_CAUSE_GROUP_CAUSE_FMT, cause->choice.transport);
      break;
    case S1ap_Cause_PR_nas:
      XML_WRITE_FORMAT_ELEMENT(writer, S1AP_CAUSE_GROUP_XML_STR, "%s", S1AP_CAUSE_GROUP_NAS_XML_STR);
      XML_WRITE_FORMAT_ELEMENT(writer, S1AP_CAUSE_GROUP_CAUSE_XML_STR, S1AP_CAUSE_GROUP_CAUSE_FMT, cause->choice.nas);
      break;
   case S1ap_Cause_PR_protocol:
     XML_WRITE_FORMAT_ELEMENT(writer, S1AP_CAUSE_GROUP_XML_STR, "%s", S1AP_CAUSE_GROUP_PROTOCOL_XML_STR);
     XML_WRITE_FORMAT_ELEMENT(writer, S1AP_CAUSE_GROUP_CAUSE_XML_STR, S1AP_CAUSE_GROUP_CAUSE_FMT, cause->choice.protocol);
     break;
    case S1ap_Cause_PR_misc:
      XML_WRITE_FORMAT_ELEMENT(writer, S1AP_CAUSE_GROUP_XML_STR, "%s", S1AP_CAUSE_GROUP_MISC_XML_STR);
      XML_WRITE_FORMAT_ELEMENT(writer, S1AP_CAUSE_GROUP_CAUSE_XML_STR, S1AP_CAUSE_GROUP_CAUSE_FMT, cause->choice.misc);
      break;
    default:;
  }
  XML_WRITE_END_ELEMENT(writer);
}


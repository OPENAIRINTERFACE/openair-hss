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

#include "assertions.h"
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
#include "sp_3gpp_36.413_xml.h"
#include "sp_common_xml.h"
#include "nas_message.h"
#include "sp_nas_message_xml.h"
#include "conversions.h"

#include "xml_load.h"
#include "xml2_wrapper.h"
#include "xml_msg_tags.h"
#include "log.h"


//------------------------------------------------------------------------------
bool sp_e_rab_setup_item_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    e_rab_setup_item_t * const item)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = true;

  res = sp_e_rab_id_from_xml (scenario, msg, &item->e_rab_id);
  if (res) {
    bstring xpath_expr_tla = bformat("./%s",TRANSPORT_LAYER_ADDRESS_XML_STR);
    res = sp_xml_load_hex_stream_leaf_tag(scenario, msg, xpath_expr_tla, &item->transport_layer_address);
    bdestroy_wrapper (&xpath_expr_tla);
  }
  if (res) {
    res = sp_teid_from_xml(scenario, msg, &item->gtp_teid);
  }

  OAILOG_FUNC_RETURN (LOG_XML, res);
}


//------------------------------------------------------------------------------
bool sp_e_rab_setup_list_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    e_rab_setup_list_t        * const list)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  list->no_of_items = 0;
  bstring xpath_expr = bformat("./%s",E_RAB_SETUP_LIST_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (msg->xml_doc)) {
      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], msg->xpath_ctx));

      if (res) {
        bstring xpath_expr_items = bformat("./%s",E_RAB_SETUP_ITEM_XML_STR);
        xmlXPathObjectPtr xpath_obj_items = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr_items);
        if (xpath_obj_items) {
          xmlNodeSetPtr items = xpath_obj_items->nodesetval;
          int no_of_items = (items) ? items->nodeNr : 0;
          for (int item = 0; item < no_of_items; item++) {
            xmlNodePtr saved_node_ptr2 = msg->xpath_ctx->node;
            res = (RETURNok == xmlXPathSetContextNode(items->nodeTab[item], msg->xpath_ctx));
            if (res) res = sp_e_rab_setup_item_from_xml (scenario, msg, &list->item[list->no_of_items]);
            if (res) list->no_of_items++;
            res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr2, msg->xpath_ctx)) & res;
          }
          xmlXPathFreeObject(xpath_obj_items);
        }
        bdestroy_wrapper (&xpath_expr_items);
      }

      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}



//------------------------------------------------------------------------------
bool sp_e_rab_to_be_setup_list_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    e_rab_to_be_setup_list_t * const list)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  list->no_of_items = 0;
  bstring xpath_expr = bformat("./%s",E_RAB_TO_BE_SETUP_LIST_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (msg->xml_doc)) {
      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], msg->xpath_ctx));

      if (res) {
        bstring xpath_expr_items = bformat("./%s",E_RAB_TO_BE_SETUP_ITEM_XML_STR);
        xmlXPathObjectPtr xpath_obj_items = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr_items);
        if (xpath_obj_items) {
          xmlNodeSetPtr items = xpath_obj_items->nodesetval;
          int no_of_items = (items) ? items->nodeNr : 0;
          for (int item = 0; item < no_of_items; item++) {
            xmlNodePtr saved_node_ptr2 = msg->xpath_ctx->node;
            res = (RETURNok == xmlXPathSetContextNode(items->nodeTab[item], msg->xpath_ctx));
            if (res) res = sp_e_rab_to_be_setup_item_from_xml (scenario, msg, &list->item[list->no_of_items]);
            if (res) list->no_of_items++;
            res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr2, msg->xpath_ctx)) & res;
          }
          xmlXPathFreeObject(xpath_obj_items);
        }
        bdestroy_wrapper (&xpath_expr_items);
      }

      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
bool sp_e_rab_to_be_setup_item_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    e_rab_to_be_setup_item_t * const item)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = true;

  res = sp_e_rab_id_from_xml (scenario, msg, &item->e_rab_id);

  if (res) {
    res = sp_e_rab_level_qos_parameters_from_xml (scenario, msg, &item->e_rab_level_qos_parameters);
  }

  if (res) {
    bstring xpath_expr_tla = bformat("./%s",TRANSPORT_LAYER_ADDRESS_XML_STR);
    res = sp_xml_load_hex_stream_leaf_tag(scenario, msg, xpath_expr_tla, &item->transport_layer_address);
    bdestroy_wrapper (&xpath_expr_tla);
  }

  if (res) {
    res = sp_teid_from_xml(scenario, msg, &item->gtp_teid);
  }

  if (res) {
    res = sp_nas_pdu_from_xml(scenario, msg, &item->nas_pdu);
  }

  OAILOG_FUNC_RETURN (LOG_XML, res);
}


//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( e_rab_id, E_RAB_ID);



//------------------------------------------------------------------------------
bool sp_e_rab_level_qos_parameters_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    e_rab_level_qos_parameters_t * const params)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",E_RAB_LEVEL_QOS_PARAMETERS_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (msg->xml_doc)) {
      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], msg->xpath_ctx));

      if (res) {
        res = sp_qci_from_xml (scenario, msg, &params->qci);
      }
      if (res) {
        res = sp_allocation_and_retention_priority_from_xml (scenario, msg, &params->allocation_and_retention_priority);
      }

      if (res) {
        res = sp_gbr_qos_information_from_xml (scenario, msg, &params->gbr_qos_information);
      }

      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
bool sp_allocation_and_retention_priority_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    allocation_and_retention_priority_t * const arp)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",ALLOCATION_AND_RETENTION_PRIORITY_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (msg->xml_doc)) {
      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], msg->xpath_ctx));

      if (res) {
        res = sp_priority_level_from_xml(scenario, msg, &arp->priority_level);
      }
      if (res) {
        res = sp_pre_emption_capability_from_xml(scenario, msg, &arp->pre_emption_capability);
      }
      if (res) {
        res = sp_pre_emption_vulnerability_from_xml(scenario, msg, &arp->pre_emption_vulnerability);
      }

      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}


//------------------------------------------------------------------------------
bool sp_gbr_qos_information_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    gbr_qos_information_t * const gqi)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",GBR_QOS_INFORMATION_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (msg->xml_doc)) {
      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], msg->xpath_ctx));

      if (res) {
        res = sp_u64_from_xml (scenario, msg, (uint64_t*)&gqi->e_rab_maximum_bit_rate_downlink, E_RAB_MAXIMUM_BIT_RATE_DOWNLINK_XML_STR);
      }
      if (res) {
        res = sp_u64_from_xml (scenario, msg, (uint64_t*)&gqi->e_rab_maximum_bit_rate_uplink, E_RAB_MAXIMUM_BIT_RATE_UPLINK_XML_STR);
      }
      if (res) {
        res = sp_u64_from_xml (scenario, msg, (uint64_t*)&gqi->e_rab_guaranteed_bit_rate_downlink, E_RAB_GUARANTEED_BIT_RATE_DOWNLINK_XML_STR);
      }
      if (res) {
        res = sp_u64_from_xml (scenario, msg, (uint64_t*)&gqi->e_rab_guaranteed_bit_rate_uplink, E_RAB_GUARANTEED_BIT_RATE_UPLINK_XML_STR);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}


//------------------------------------------------------------------------------
bool sp_ue_aggregate_maximum_bit_rate_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    ue_aggregate_maximum_bit_rate_t * const ue_ambr)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",UE_AGGREGATE_MAXIMUM_BIT_RATE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (msg->xml_doc)) {
      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], msg->xpath_ctx));

      if (res) {
        res = sp_u64_from_xml (scenario, msg, (uint64_t*)&ue_ambr->dl, DOWNLINK_XML_STR);
      }
      if (res) {
        res = sp_u64_from_xml (scenario, msg, (uint64_t*)&ue_ambr->dl, UPLINK_XML_STR);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}


//------------------------------------------------------------------------------
bool sp_s1ap_cause_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    S1ap_Cause_t * const cause)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",S1AP_CAUSE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (msg->xml_doc)) {
      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], msg->xpath_ctx));

      bstring group_name  = NULL;
      bstring xpath_expr_group = bformat("./%s",S1AP_CAUSE_GROUP_XML_STR);
      bool res = sp_xml_load_ascii_stream_leaf_tag(scenario, msg, xpath_expr_group, &group_name);
      bdestroy_wrapper (&xpath_expr_group);
      if (res) {
        uint64_t cause64 = 0;
        res = sp_u64_from_xml (scenario, msg, (uint64_t*)&cause64, S1AP_CAUSE_GROUP_CAUSE_XML_STR);
        if (!strcasecmp((const char *)group_name->data, S1AP_CAUSE_GROUP_RADIO_NETWORK_LAYER_XML_STR)) {
          cause->present = S1ap_Cause_PR_radioNetwork;
          if (res) {
            cause->choice.radioNetwork = cause64;
          }
        } else  if (!strcasecmp((const char *)group_name->data, S1AP_CAUSE_GROUP_TRANSPORT_LAYER_XML_STR)) {
          cause->present = S1ap_Cause_PR_transport;
          if (res) {
            cause->choice.transport = cause64;
          }
        } else  if (!strcasecmp((const char *)group_name->data, S1AP_CAUSE_GROUP_NAS_XML_STR)) {
          cause->present = S1ap_Cause_PR_nas;
          if (res) {
            cause->choice.nas = cause64;
          }
        } else  if (!strcasecmp((const char *)group_name->data, S1AP_CAUSE_GROUP_PROTOCOL_XML_STR)) {
          cause->present = S1ap_Cause_PR_protocol;
          if (res) {
            cause->choice.protocol = cause64;
          }
        } else  if (!strcasecmp((const char *)group_name->data, S1AP_CAUSE_GROUP_MISC_XML_STR)) {
          cause->present = S1ap_Cause_PR_misc;
          if (res) {
            cause->choice.misc = cause64;
          }
        } else {
          cause->present = S1ap_Cause_PR_NOTHING;
        }
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);

}

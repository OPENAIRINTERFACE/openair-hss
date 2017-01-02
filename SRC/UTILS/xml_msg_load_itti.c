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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "hashtable.h"
#include "obj_hashtable.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "mme_scenario_player.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "3gpp_24.301.h"
#include "security_types.h"
#include "common_types.h"
#include "emm_msg.h"
#include "esm_msg.h"
#include "intertask_interface.h"
#include "timer.h"
#include "dynamic_memory_check.h"
#include "common_defs.h"
#include "xml_msg_tags.h"
#include "xml_msg_load_itti.h"

#include "../TEST/SCENARIO_PLAYER/sp_xml_load.h"
#include "itti_free_defined_msg.h"
#include "3gpp_23.003_xml.h"
#include "sp_3gpp_23.003_xml.h"
#include "3gpp_24.007_xml.h"
#include "3gpp_36.401_xml.h"
#include "3gpp_36.331_xml.h"
#include "3gpp_36.413_xml.h"
#include "3gpp_24.301_emm_ies_xml.h"
#include "nas_message_xml.h"
#include "xml_load.h"
#include "sp_3gpp_24.007_xml.h"
#include "sp_3gpp_36.401_xml.h"
#include "sp_3gpp_36.413_xml.h"
#include "sp_3gpp_24.301_emm_ies_xml.h"
#include "sp_common_xml.h"

#include "sp_nas_message_xml.h"


bool xml_msg_load_action_tag(xmlDocPtr const xml_doc, void * const container, xmlNodeSetPtr const nodes);
int itti_task_str2itti_task_id(const char * const task_id_str);
bool xml_load_itti_task_id_tag(
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    bstring                       xpath_expr,
    int                   * const container);

//------------------------------------------------------------------------------
bool xml_msg_load_action_tag(
    xmlDocPtr const     xml_doc,
    void * const        container,
    xmlNodeSetPtr const nodes)
{
  bool * is_tx = (bool *)container;
  int size = (nodes) ? nodes->nodeNr : 0;
  if ((1 == size) && (container) && (xml_doc)) {
    xmlChar *key = xmlNodeListGetString(xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
    OAILOG_TRACE (LOG_XML, "Found %s = %s\n", ACTION_XML_STR, (const char*)key);
    if ((!xmlStrcmp(key, (const xmlChar *)ACTION_SEND_XML_STR))){
      *is_tx = true;
    } else {
      *is_tx = false;
    }
    xmlFree(key);
    return true;
  }
  OAILOG_TRACE (LOG_XML, "Warning: No result for searching tag %s\n", ACTION_XML_STR);
  return false;
}

//------------------------------------------------------------------------------
int itti_task_str2itti_task_id(const char * const task_id_str)
{
  if (!strcasecmp(task_id_str, "TASK_TIMER")) {
    return TASK_TIMER;
  } else if (!strcasecmp(task_id_str, "TASK_GTPV1_U")) {
    return TASK_GTPV1_U;
  } else if (!strcasecmp(task_id_str, "TASK_MME_APP")) {
    return TASK_MME_APP;
  } else if (!strcasecmp(task_id_str, "TASK_NAS_MME")) {
    return TASK_NAS_MME;
  } else if (!strcasecmp(task_id_str, "TASK_S11")) {
    return TASK_S11;
  } else if (!strcasecmp(task_id_str, "TASK_S1AP")) {
    return TASK_S1AP;
  } else if (!strcasecmp(task_id_str, "TASK_S6A")) {
    return TASK_S6A;
  } else if (!strcasecmp(task_id_str, "TASK_SCTP")) {
    return TASK_SCTP;
  } else if (!strcasecmp(task_id_str, "TASK_SPGW_APP")) {
    return TASK_SPGW_APP;
  } else if (!strcasecmp(task_id_str, "TASK_UDP")) {
    return TASK_UDP;
  } else if (!strcasecmp(task_id_str, "TASK_LOG")) {
    return TASK_LOG;
  } else if (!strcasecmp(task_id_str, "TASK_SHARED_TS_LOG")) {
    return TASK_SHARED_TS_LOG;
  } else if (!strcasecmp(task_id_str, "TASK_MME_SCENARIO_PLAYER")) {
    return TASK_MME_SCENARIO_PLAYER;
  } else {
    OAILOG_ERROR (LOG_XML, "Could not find task id for %s\n", task_id_str);
    return TASK_UNKNOWN;
  }
}
//------------------------------------------------------------------------------
bool xml_load_itti_task_id_tag(
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    bstring                       xpath_expr,
    int                   * const container)
{
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  bool res = false;
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {
      xmlChar *key = xmlNodeListGetString(xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
      if (key) {
        res = true;
        *container = itti_task_str2itti_task_id((const char *)key);
        OAILOG_TRACE (LOG_XML, "Found %s=%s (%d)\n", bdata(xpath_expr), key, *container);
        xmlFree(key);
      } else {
        *container = TASK_UNKNOWN;
      }
    }
    xmlXPathFreeObject(xpath_obj);
  }
  return res;
}
//------------------------------------------------------------------------------
int xml_msg_load_itti_sctp_new_association(scenario_t * const scenario, scenario_player_msg_t * const msg)
{
  bool res = false;
  if ((msg) && (msg->xml_doc)) {
    xmlNodePtr  cur = NULL;
    cur = xmlDocGetRootElement(msg->xml_doc);
    AssertFatal (cur, "Empty document");

    if (xmlStrcmp(cur->name, (const xmlChar *) ITTI_SCTP_NEW_ASSOCIATION_XML_STR)) {
      OAILOG_ERROR (LOG_XML, "Could not find tag %s\n", ITTI_SCTP_NEW_ASSOCIATION_XML_STR);
      return RETURNerror;
    }

    if (!msg->xpath_ctx) {
      // Create xpath evaluation context
      msg->xpath_ctx = xmlXPathNewContext(msg->xml_doc);
    }

    xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
    if (RETURNok != xmlXPathSetContextNode(cur, msg->xpath_ctx)) {
      return RETURNerror;
    }

    // free it (may be called from msp_reload_message)
    if (msg->itti_msg) {
      itti_free_msg_content (msg->itti_msg);
      itti_free (ITTI_MSG_ORIGIN_ID (msg->itti_msg), msg->itti_msg);
    }

    msg->itti_msg = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, SCTP_NEW_ASSOCIATION);

    if (msg->itti_msg) {

      bstring xpath_expr = bformat("./%s",ACTION_XML_STR);
      res = xml_load_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &xml_msg_load_action_tag, (void*)&msg->is_tx);
      bdestroy_wrapper (&xpath_expr);

      if (res) {
        xpath_expr = bformat("./%s",ITTI_SENDER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_sender_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",ITTI_RECEIVER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_receiver_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",NUMBER_INBOUND_STREAMS_XML_STR);
        res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, "%"SCNu16, (void*)&msg->itti_msg->ittiMsg.sctp_new_peer.instreams, NULL);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",NUMBER_OUTBOUND_STREAMS_XML_STR);
        res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, "%"SCNu16, (void*)&msg->itti_msg->ittiMsg.sctp_new_peer.outstreams, NULL);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",SCTP_ASSOC_ID_XML_STR);
        res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, "%"SCNu32, (void*)&msg->itti_msg->ittiMsg.sctp_new_peer.assoc_id, NULL);
        bdestroy_wrapper (&xpath_expr);
      }
    } else {
      res = false;
    }
    if (saved_node_ptr) {
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  return (res)? RETURNok:RETURNerror;
}

//------------------------------------------------------------------------------
int xml_msg_load_itti_sctp_close_association(scenario_t * const scenario, scenario_player_msg_t * const msg)
{
  bool res = false;
  if ((msg) && (msg->xml_doc)) {
    xmlNodePtr  cur = NULL;
    cur = xmlDocGetRootElement(msg->xml_doc);
    AssertFatal (cur, "Empty document");

    if (xmlStrcmp(cur->name, (const xmlChar *) ITTI_SCTP_CLOSE_ASSOCIATION_XML_STR)) {
      OAILOG_ERROR (LOG_XML, "Could not find tag %s\n", ITTI_SCTP_CLOSE_ASSOCIATION_XML_STR);
      return RETURNerror;
    }

    if (!msg->xpath_ctx) {
      // Create xpath evaluation context
      msg->xpath_ctx = xmlXPathNewContext(msg->xml_doc);
    }

    xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
    if (RETURNok != xmlXPathSetContextNode(cur, msg->xpath_ctx)) {
      return RETURNerror;
    }

    // free it (may be called from msp_reload_message)
    if (msg->itti_msg) {
      itti_free_msg_content (msg->itti_msg);
      itti_free (ITTI_MSG_ORIGIN_ID (msg->itti_msg), msg->itti_msg);
    }
    msg->itti_msg = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, SCTP_CLOSE_ASSOCIATION);

    if (msg->itti_msg) {
      bstring xpath_expr = bformat("./%s",ACTION_XML_STR);
      res = xml_load_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &xml_msg_load_action_tag, (void*)&msg->is_tx);
      bdestroy_wrapper (&xpath_expr);

      if (res) {
        xpath_expr = bformat("./%s",ITTI_SENDER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_sender_task);
         bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",ITTI_RECEIVER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_receiver_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",SCTP_ASSOC_ID_XML_STR);
        res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, "%"SCNu32, (void*)&msg->itti_msg->ittiMsg.sctp_close_association.assoc_id, NULL);
        bdestroy_wrapper (&xpath_expr);
      }
    } else {
      res = false;
    }
    if (saved_node_ptr) {
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  return (res)? RETURNok:RETURNerror;
}


//------------------------------------------------------------------------------
int xml_msg_load_itti_s1ap_ue_context_release_req(scenario_t * const scenario, scenario_player_msg_t * const msg)
{
  bool res = false;
  if ((msg) && (msg->xml_doc)) {
    xmlNodePtr  cur = NULL;
    cur = xmlDocGetRootElement(msg->xml_doc);
    AssertFatal (cur, "Empty document");

    if (xmlStrcmp(cur->name, (const xmlChar *) ITTI_S1AP_UE_CONTEXT_RELEASE_REQ_XML_STR)) {
      OAILOG_ERROR (LOG_XML, "Could not find tag %s\n", ITTI_S1AP_UE_CONTEXT_RELEASE_REQ_XML_STR);
      return RETURNerror;
    }

    if (!msg->xpath_ctx) {
      // Create xpath evaluation context
      msg->xpath_ctx = xmlXPathNewContext(msg->xml_doc);
    }

    xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
    if (RETURNok != xmlXPathSetContextNode(cur, msg->xpath_ctx)) {
      return RETURNerror;
    }

    // free it (may be called from msp_reload_message)
    if (msg->itti_msg) {
      itti_free_msg_content (msg->itti_msg);
      itti_free (ITTI_MSG_ORIGIN_ID (msg->itti_msg), msg->itti_msg);
    }

    msg->itti_msg = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, S1AP_UE_CONTEXT_RELEASE_REQ);
    if (msg->itti_msg) {
      bstring xpath_expr = bformat("./%s",ACTION_XML_STR); // anywhere in XML tree
      res = xml_load_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &xml_msg_load_action_tag, (void*)&msg->is_tx);
      bdestroy_wrapper (&xpath_expr);

      if (res) {
        xpath_expr = bformat("./%s",ITTI_SENDER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_sender_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",ITTI_RECEIVER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_receiver_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        enb_ue_s1ap_id_t enb_ue_s1ap_id = 0;
        res = sp_enb_ue_s1ap_id_from_xml(scenario, msg, &enb_ue_s1ap_id);
        msg->itti_msg->ittiMsg.s1ap_ue_context_release_req.enb_ue_s1ap_id = enb_ue_s1ap_id;
      }

      if (res) {
        res = sp_mme_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.s1ap_ue_context_release_req.mme_ue_s1ap_id);
      }
    } else {
      res = false;
    }
    if (saved_node_ptr) {
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  return (res)? RETURNok:RETURNerror;
}

//------------------------------------------------------------------------------
int xml_msg_load_itti_s1ap_ue_context_release_command(scenario_t * const scenario, scenario_player_msg_t * const msg)
{
  bool res = false;
  if ((msg) && (msg->xml_doc)) {
    xmlNodePtr  cur = NULL;
    cur = xmlDocGetRootElement(msg->xml_doc);
    AssertFatal (cur, "Empty document");

    if (xmlStrcmp(cur->name, (const xmlChar *) ITTI_S1AP_UE_CONTEXT_RELEASE_COMMAND_XML_STR)) {
      OAILOG_ERROR (LOG_XML, "Could not find tag %s\n", ITTI_S1AP_UE_CONTEXT_RELEASE_COMMAND_XML_STR);
      return RETURNerror;
    }

    if (!msg->xpath_ctx) {
      // Create xpath evaluation context
      msg->xpath_ctx = xmlXPathNewContext(msg->xml_doc);
    }

    xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
    if (RETURNok != xmlXPathSetContextNode(cur, msg->xpath_ctx)) {
      return RETURNerror;
    }

    // free it (may be called from msp_reload_message)
    if (msg->itti_msg) {
      itti_free_msg_content (msg->itti_msg);
      itti_free (ITTI_MSG_ORIGIN_ID (msg->itti_msg), msg->itti_msg);
    }

    msg->itti_msg = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, S1AP_UE_CONTEXT_RELEASE_COMMAND);
    if (msg->itti_msg) {
      bstring xpath_expr = bformat("./%s",ACTION_XML_STR); // anywhere in XML tree
      res = xml_load_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &xml_msg_load_action_tag, (void*)&msg->is_tx);
      bdestroy_wrapper (&xpath_expr);

      if (res) {
        xpath_expr = bformat("./%s",ITTI_SENDER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_sender_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",ITTI_RECEIVER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_receiver_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        enb_ue_s1ap_id_t enb_ue_s1ap_id = 0;
        res = sp_enb_ue_s1ap_id_from_xml(scenario, msg, &enb_ue_s1ap_id);
        msg->itti_msg->ittiMsg.s1ap_ue_context_release_command.enb_ue_s1ap_id = enb_ue_s1ap_id;
      }

      if (res) {
        res = sp_mme_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.s1ap_ue_context_release_command.mme_ue_s1ap_id);
      }
    } else {
      res = false;
    }
    if (saved_node_ptr) {
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  return (res)? RETURNok:RETURNerror;
}

//------------------------------------------------------------------------------
int xml_msg_load_itti_s1ap_ue_context_release_complete(scenario_t * const scenario, scenario_player_msg_t * const msg)
{
  bool res = false;
  if ((msg) && (msg->xml_doc)) {
    xmlNodePtr  cur = NULL;
    cur = xmlDocGetRootElement(msg->xml_doc);
    AssertFatal (cur, "Empty document");

    if (xmlStrcmp(cur->name, (const xmlChar *) ITTI_S1AP_UE_CONTEXT_RELEASE_COMPLETE_XML_STR)) {
      OAILOG_ERROR (LOG_XML, "Could not find tag %s\n", ITTI_S1AP_UE_CONTEXT_RELEASE_COMPLETE_XML_STR);
      return RETURNerror;
    }

    if (!msg->xpath_ctx) {
      // Create xpath evaluation context
      msg->xpath_ctx = xmlXPathNewContext(msg->xml_doc);
    }

    xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
    if (RETURNok != xmlXPathSetContextNode(cur, msg->xpath_ctx)) {
      return RETURNerror;
    }

    // free it (may be called from msp_reload_message)
    if (msg->itti_msg) {
      itti_free_msg_content (msg->itti_msg);
      itti_free (ITTI_MSG_ORIGIN_ID (msg->itti_msg), msg->itti_msg);
    }

    msg->itti_msg = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, S1AP_UE_CONTEXT_RELEASE_COMPLETE);
    if (msg->itti_msg) {
      bstring xpath_expr = bformat("./%s",ACTION_XML_STR); // anywhere in XML tree
      res = xml_load_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &xml_msg_load_action_tag, (void*)&msg->is_tx);
      bdestroy_wrapper (&xpath_expr);

      if (res) {
        xpath_expr = bformat("./%s",ITTI_SENDER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_sender_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",ITTI_RECEIVER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_receiver_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        enb_ue_s1ap_id_t enb_ue_s1ap_id = 0;
        res = sp_enb_ue_s1ap_id_from_xml(scenario, msg, &enb_ue_s1ap_id);
        msg->itti_msg->ittiMsg.s1ap_ue_context_release_complete.enb_ue_s1ap_id = enb_ue_s1ap_id;
      }

      if (res) {
        res = sp_mme_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.s1ap_ue_context_release_complete.mme_ue_s1ap_id);
      }
    } else {
      res = false;
    }
    if (saved_node_ptr) {
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  return (res)? RETURNok:RETURNerror;
}

//------------------------------------------------------------------------------
int xml_msg_load_itti_s1ap_initial_ue_message (scenario_t * const scenario, scenario_player_msg_t * const msg)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;
  if ((msg) && (msg->xml_doc)) {
    xmlNodePtr  cur = NULL;
    cur = xmlDocGetRootElement(msg->xml_doc);
    AssertFatal (cur, "Empty document");

    if (xmlStrcmp(cur->name, (const xmlChar *) ITTI_S1AP_INITIAL_UE_MESSAGE_XML_STR)) {
      OAILOG_ERROR (LOG_XML, "Could not find tag %s\n", ITTI_S1AP_INITIAL_UE_MESSAGE_XML_STR);
      OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, RETURNerror);
    }

    if (!msg->xpath_ctx) {
      // Create xpath evaluation context
      msg->xpath_ctx = xmlXPathNewContext(msg->xml_doc);
    }

    xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
    if (RETURNok != xmlXPathSetContextNode(cur, msg->xpath_ctx)) {
      OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, RETURNerror);
    }

    // free it (may be called from msp_reload_message)
    if (msg->itti_msg) {
      itti_free_msg_content (msg->itti_msg);
      itti_free (ITTI_MSG_ORIGIN_ID (msg->itti_msg), msg->itti_msg);
    }

    msg->itti_msg = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, S1AP_INITIAL_UE_MESSAGE);

    if (msg->itti_msg) {
      bstring xpath_expr = bformat("./%s",ACTION_XML_STR); // anywhere in XML tree
      res = xml_load_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &xml_msg_load_action_tag, (void*)&msg->is_tx);
      bdestroy_wrapper (&xpath_expr);

      if (res) {
        xpath_expr = bformat("./%s",ITTI_SENDER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_sender_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",ITTI_RECEIVER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_receiver_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        res = sp_enb_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.s1ap_initial_ue_message.enb_ue_s1ap_id);
      }

      // Don't forget
      msg->itti_msg->ittiMsg.s1ap_initial_ue_message.mme_ue_s1ap_id = INVALID_MME_UE_S1AP_ID;

      if (res) {
        res = sp_nas_pdu_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.s1ap_initial_ue_message.nas);
      }

      if (res) {
        res = tracking_area_identity_from_xml(msg->xml_doc, msg->xpath_ctx, &msg->itti_msg->ittiMsg.s1ap_initial_ue_message.tai);
      }

      if (res) {
        res = ecgi_from_xml(msg->xml_doc, msg->xpath_ctx, &msg->itti_msg->ittiMsg.s1ap_initial_ue_message.ecgi);
      }

      if (res) {
        res = rrc_establishment_cause_from_xml(msg->xml_doc, msg->xpath_ctx, &msg->itti_msg->ittiMsg.s1ap_initial_ue_message.rrc_establishment_cause);
      }

      if (sp_s_tmsi_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.s1ap_initial_ue_message.opt_s_tmsi)) {
        msg->itti_msg->ittiMsg.s1ap_initial_ue_message.is_s_tmsi_valid = true;
      }

      if (sp_csg_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.s1ap_initial_ue_message.opt_csg_id)) {
        msg->itti_msg->ittiMsg.s1ap_initial_ue_message.is_csg_id_valid = true;
      }

      if (sp_gummei_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.s1ap_initial_ue_message.opt_gummei)) {
        msg->itti_msg->ittiMsg.s1ap_initial_ue_message.is_gummei_valid = true;
      }

      // Don't forget
      msg->itti_msg->ittiMsg.s1ap_initial_ue_message.transparent.mme_ue_s1ap_id = msg->itti_msg->ittiMsg.s1ap_initial_ue_message.mme_ue_s1ap_id;
      msg->itti_msg->ittiMsg.s1ap_initial_ue_message.transparent.enb_ue_s1ap_id = msg->itti_msg->ittiMsg.s1ap_initial_ue_message.enb_ue_s1ap_id;
      msg->itti_msg->ittiMsg.s1ap_initial_ue_message.transparent.e_utran_cgi    = msg->itti_msg->ittiMsg.s1ap_initial_ue_message.ecgi;
    } else {
      res = false;
    }
    if (saved_node_ptr) {
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  if (res)
    OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, RETURNok);
  else
    OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, RETURNerror);
}

//------------------------------------------------------------------------------
int xml_msg_load_itti_s1ap_e_rab_setup_req (scenario_t * const scenario, scenario_player_msg_t * const msg)
{
  bool res = false;
  if ((msg) && (msg->xml_doc)) {
    xmlNodePtr  cur = NULL;
    cur = xmlDocGetRootElement(msg->xml_doc);
    AssertFatal (cur, "Empty document");

    if (xmlStrcmp(cur->name, (const xmlChar *) ITTI_S1AP_E_RAB_SETUP_REQ_XML_STR)) {
      OAILOG_ERROR (LOG_XML, "Could not find tag %s\n", ITTI_S1AP_E_RAB_SETUP_REQ_XML_STR);
      return RETURNerror;
    }

    if (!msg->xpath_ctx) {
      // Create xpath evaluation context
      msg->xpath_ctx = xmlXPathNewContext(msg->xml_doc);
    }

    xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
    if (RETURNok != xmlXPathSetContextNode(cur, msg->xpath_ctx)) {
      return RETURNerror;
    }

    // free it (may be called from msp_reload_message)
    if (msg->itti_msg) {
      itti_free_msg_content (msg->itti_msg);
      itti_free (ITTI_MSG_ORIGIN_ID (msg->itti_msg), msg->itti_msg);
    }

    msg->itti_msg = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, S1AP_E_RAB_SETUP_REQ);

    if (msg->itti_msg) {
      bstring xpath_expr = bformat("./%s",ACTION_XML_STR); // anywhere in XML tree
      res = xml_load_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &xml_msg_load_action_tag, (void*)&msg->is_tx);
      bdestroy_wrapper (&xpath_expr);

      if (res) {
        xpath_expr = bformat("./%s",ITTI_SENDER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_sender_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",ITTI_RECEIVER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_receiver_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        res = sp_enb_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.s1ap_e_rab_setup_req.enb_ue_s1ap_id);
      }

      if (res) {
        res = sp_mme_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.s1ap_e_rab_setup_req.mme_ue_s1ap_id);
      }

      if (res) {
        res = sp_ue_aggregate_maximum_bit_rate_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.s1ap_e_rab_setup_req.ue_aggregate_maximum_bit_rate);
      }

      if (res) {
        res = sp_e_rab_to_be_setup_list_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.s1ap_e_rab_setup_req.e_rab_to_be_setup_list);
      }
    } else {
      res = false;
    }
    if (saved_node_ptr) {
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  return (res)? RETURNok:RETURNerror;
}

//------------------------------------------------------------------------------
int xml_msg_load_itti_s1ap_e_rab_setup_rsp (scenario_t * const scenario, scenario_player_msg_t * const msg)
{
  bool res = false;
  if ((msg) && (msg->xml_doc)) {
    xmlNodePtr  cur = NULL;
    cur = xmlDocGetRootElement(msg->xml_doc);
    AssertFatal (cur, "Empty document");

    if (xmlStrcmp(cur->name, (const xmlChar *) ITTI_S1AP_E_RAB_SETUP_RSP_XML_STR)) {
      OAILOG_ERROR (LOG_XML, "Could not find tag %s\n", ITTI_S1AP_E_RAB_SETUP_RSP_XML_STR);
      return RETURNerror;
    }

    if (!msg->xpath_ctx) {
      // Create xpath evaluation context
      msg->xpath_ctx = xmlXPathNewContext(msg->xml_doc);
    }

    xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
    if (RETURNok != xmlXPathSetContextNode(cur, msg->xpath_ctx)) {
      return RETURNerror;
    }

    // free it (may be called from msp_reload_message)
    if (msg->itti_msg) {
      itti_free_msg_content (msg->itti_msg);
      itti_free (ITTI_MSG_ORIGIN_ID (msg->itti_msg), msg->itti_msg);
    }

    msg->itti_msg = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, S1AP_E_RAB_SETUP_RSP);

    if (msg->itti_msg) {
      bstring xpath_expr = bformat("./%s",ACTION_XML_STR); // anywhere in XML tree
      res = xml_load_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &xml_msg_load_action_tag, (void*)&msg->is_tx);
      bdestroy_wrapper (&xpath_expr);

      if (res) {
        xpath_expr = bformat("./%s",ITTI_SENDER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_sender_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",ITTI_RECEIVER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_receiver_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        res = sp_enb_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.s1ap_e_rab_setup_rsp.enb_ue_s1ap_id);
      }

      if (res) {
        res = sp_mme_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.s1ap_e_rab_setup_rsp.mme_ue_s1ap_id);
      }

      if (res) {
        res = e_rab_setup_list_from_xml(msg->xml_doc, msg->xpath_ctx, &msg->itti_msg->ittiMsg.s1ap_e_rab_setup_rsp.e_rab_setup_list);
      }
    } else {
      res = false;
    }
    if (saved_node_ptr) {
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  return (res)? RETURNok:RETURNerror;
}

//------------------------------------------------------------------------------
int xml_msg_load_itti_mme_app_initial_context_setup_rsp(scenario_t * const scenario, scenario_player_msg_t * const msg)
{
  bool res = false;
  if ((msg) && (msg->xml_doc)) {
    xmlNodePtr  cur = NULL;
    cur = xmlDocGetRootElement(msg->xml_doc);
    AssertFatal (cur, "Empty document");

    if (xmlStrcmp(cur->name, (const xmlChar *) ITTI_MME_APP_INITIAL_CONTEXT_SETUP_RSP_XML_STR)) {
      OAILOG_ERROR (LOG_XML, "Could not find tag %s\n", ITTI_MME_APP_INITIAL_CONTEXT_SETUP_RSP_XML_STR);
      return RETURNerror;
    }

    if (!msg->xpath_ctx) {
      // Create xpath evaluation context
      msg->xpath_ctx = xmlXPathNewContext(msg->xml_doc);
    }

    xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
    if (RETURNok != xmlXPathSetContextNode(cur, msg->xpath_ctx)) {
      return RETURNerror;
    }

    // free it (may be called from msp_reload_message)
    if (msg->itti_msg) {
      itti_free_msg_content (msg->itti_msg);
      itti_free (ITTI_MSG_ORIGIN_ID (msg->itti_msg), msg->itti_msg);
    }

    msg->itti_msg = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, MME_APP_INITIAL_CONTEXT_SETUP_RSP);

    if (msg->itti_msg) {
      bstring xpath_expr = bformat("./%s",ACTION_XML_STR); // anywhere in XML tree
      res = xml_load_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &xml_msg_load_action_tag, (void*)&msg->is_tx);
      bdestroy_wrapper (&xpath_expr);

      if (res) {
        xpath_expr = bformat("./%s",ITTI_SENDER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_sender_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",ITTI_RECEIVER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_receiver_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        res = sp_mme_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.mme_app_initial_context_setup_rsp.ue_id);
      }

      if (res) {
        bstring xpath_expr_erab = bformat("./%s",E_RAB_SETUP_ITEM_XML_STR);
        xmlXPathObjectPtr xpath_obj_erab = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr_erab);
        if (xpath_obj_erab) {
          xmlNodeSetPtr nodes_erab = xpath_obj_erab->nodesetval;
          int size = (nodes_erab) ? nodes_erab->nodeNr : 0;
          if ((1 <= size) && (msg->xml_doc)) {
            msg->itti_msg->ittiMsg.mme_app_initial_context_setup_rsp.no_of_e_rabs = size;
            for (int e=0; e< size; e++) {
              xmlNodePtr saved_node_ptr2 = msg->xpath_ctx->node;
              res = (RETURNok == xmlXPathSetContextNode(nodes_erab->nodeTab[e], msg->xpath_ctx));

              if (res) {
                res = sp_ebi_from_xml (scenario, msg, &msg->itti_msg->ittiMsg.mme_app_initial_context_setup_rsp.e_rab_id[e]);
              }
              if (res) {
                res = sp_teid_from_xml(scenario, msg, (void*)&msg->itti_msg->ittiMsg.mme_app_initial_context_setup_rsp.gtp_teid[e]);
              }
              if (res) {
                bstring xpath_expr_tla = bformat("./%s",TRANSPORT_LAYER_ADDRESS_XML_STR);
                res = sp_xml_load_hex_stream_leaf_tag(scenario, msg, xpath_expr_tla, &msg->itti_msg->ittiMsg.mme_app_initial_context_setup_rsp.transport_layer_address[e]);
                bdestroy_wrapper (&xpath_expr_tla);
              }
              res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr2, msg->xpath_ctx)) & res;
            }
          }
        }
      }
    } else {
      res = false;
    }
    if (saved_node_ptr) {
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  return (res)? RETURNok:RETURNerror;
}

//------------------------------------------------------------------------------
int xml_msg_load_itti_mme_app_connection_establishment_cnf(scenario_t * const scenario, scenario_player_msg_t * const msg)
{
  bool res = false;
  if ((msg) && (msg->xml_doc)) {
    xmlNodePtr  cur = NULL;
    cur = xmlDocGetRootElement(msg->xml_doc);
    AssertFatal (cur, "Empty document");

    if (xmlStrcmp(cur->name, (const xmlChar *) ITTI_MME_APP_CONNECTION_ESTABLISHMENT_CNF_XML_STR)) {
      OAILOG_ERROR (LOG_XML, "Could not find tag %s\n", ITTI_MME_APP_CONNECTION_ESTABLISHMENT_CNF_XML_STR);
      return RETURNerror;
    }

    if (!msg->xpath_ctx) {
      // Create xpath evaluation context
      msg->xpath_ctx = xmlXPathNewContext(msg->xml_doc);
    }

    xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
    if (RETURNok != xmlXPathSetContextNode(cur, msg->xpath_ctx)) {
      return RETURNerror;
    }

    // free it (may be called from msp_reload_message)
    if (msg->itti_msg) {
      itti_free_msg_content (msg->itti_msg);
      itti_free (ITTI_MSG_ORIGIN_ID (msg->itti_msg), msg->itti_msg);
    }

    msg->itti_msg = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, MME_APP_CONNECTION_ESTABLISHMENT_CNF);

    if (msg->itti_msg) {
      bstring xpath_expr = bformat("./%s",ACTION_XML_STR); // anywhere in XML tree
      res = xml_load_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &xml_msg_load_action_tag, (void*)&msg->is_tx);
      bdestroy_wrapper (&xpath_expr);

      if (res) {
        xpath_expr = bformat("./%s",ITTI_SENDER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_sender_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",ITTI_RECEIVER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_receiver_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        res = sp_mme_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.mme_app_connection_establishment_cnf.ue_id);
      }

      if (res) {
        bstring xpath_expr_erab = bformat("./%s",E_RAB_SETUP_ITEM_XML_STR);
        xmlXPathObjectPtr xpath_obj_erab = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr_erab);
        if (xpath_obj_erab) {
          xmlNodeSetPtr nodes_erab = xpath_obj_erab->nodesetval;
          int size = (nodes_erab) ? nodes_erab->nodeNr : 0;
          if ((1 <= size) && (msg->xml_doc)) {
            msg->itti_msg->ittiMsg.mme_app_connection_establishment_cnf.no_of_e_rabs = size;
            for (int e=0; e< size; e++) {
              xmlNodePtr saved_node_ptr2 = msg->xpath_ctx->node;
              res = (RETURNok == xmlXPathSetContextNode(nodes_erab->nodeTab[e], msg->xpath_ctx));

              if (res) {
                res = sp_ebi_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.mme_app_connection_establishment_cnf.e_rab_id[e]);
              }
              if (res) {
                res = sp_qci_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.mme_app_connection_establishment_cnf.e_rab_level_qos_qci[e]);
              }
              if (res) {
                res = sp_priority_level_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.mme_app_connection_establishment_cnf.e_rab_level_qos_priority_level[e]);
              }
              if (res) {
                res = sp_pre_emption_capability_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.mme_app_connection_establishment_cnf.e_rab_level_qos_preemption_capability[e]);
              }
              if (res) {
                res = sp_pre_emption_vulnerability_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.mme_app_connection_establishment_cnf.e_rab_level_qos_preemption_vulnerability[e]);
              }
              if (res) {
                bstring xpath_expr_tla = bformat("./%s",TRANSPORT_LAYER_ADDRESS_XML_STR);
                res = sp_xml_load_hex_stream_leaf_tag(scenario, msg, xpath_expr_tla, &msg->itti_msg->ittiMsg.mme_app_connection_establishment_cnf.transport_layer_address[e]);
                bdestroy_wrapper (&xpath_expr_tla);
              }
              if (res) {
                res = sp_teid_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.mme_app_connection_establishment_cnf.gtp_teid[e]);
              }
              if (res) {
                res = sp_nas_pdu_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.mme_app_connection_establishment_cnf.nas_pdu[e]);
              }
              res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr2, msg->xpath_ctx)) & res;
            }
          }
        }
      }

      if (res) {
        xpath_expr = bformat("./%s",UE_SECURITY_CAPABILITIES_ENCRYPTION_ALGORITHMS_XML_STR);
        res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, "%"SCNx16,
            (void*)&msg->itti_msg->ittiMsg.mme_app_connection_establishment_cnf.ue_security_capabilities_encryption_algorithms, NULL);
        bdestroy_wrapper (&xpath_expr);
      }
      if (res) {
        xpath_expr = bformat("./%s",UE_SECURITY_CAPABILITIES_INTEGRITY_PROTECTION_ALGORITHMS_XML_STR);
        res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, "%"SCNx16,
            (void*)&msg->itti_msg->ittiMsg.mme_app_connection_establishment_cnf.ue_security_capabilities_integrity_algorithms, NULL);
        bdestroy_wrapper (&xpath_expr);
      }
      if (res) {
        bstring xpath_expr_kenb = bformat("./%s",KENB_XML_STR);
        bstring b = NULL;
        res = sp_xml_load_hex_stream_leaf_tag(scenario, msg, xpath_expr_kenb, &b);

        if ((res) && (b)) {
          memcpy(msg->itti_msg->ittiMsg.mme_app_connection_establishment_cnf.kenb,
                 b->data,
                 min(b->slen, AUTH_KENB_SIZE));
          bdestroy_wrapper (&b);
        }
        bdestroy_wrapper (&xpath_expr_kenb);
      }
    } else {
      res = false;
    }
    if (saved_node_ptr) {
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  return (res)? RETURNok:RETURNerror;
}

//------------------------------------------------------------------------------
int xml_msg_load_itti_nas_uplink_data_ind(scenario_t * const scenario, scenario_player_msg_t * const msg)
{
  bool res = false;
  if ((msg) && (msg->xml_doc)) {
    xmlNodePtr  cur = NULL;
    cur = xmlDocGetRootElement(msg->xml_doc);
    AssertFatal (cur, "Empty document");

    if (xmlStrcmp(cur->name, (const xmlChar *) ITTI_NAS_UPLINK_DATA_IND_XML_STR)) {
      OAILOG_ERROR (LOG_XML, "Could not find tag %s\n", ITTI_NAS_UPLINK_DATA_IND_XML_STR);
      return RETURNerror;
    }

    if (!msg->xpath_ctx) {
      // Create xpath evaluation context
      msg->xpath_ctx = xmlXPathNewContext(msg->xml_doc);
    }

    xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
    if (RETURNok != xmlXPathSetContextNode(cur, msg->xpath_ctx)) {
      return RETURNerror;
    }

    // free it (may be called from msp_reload_message)
    if (msg->itti_msg) {
      itti_free_msg_content (msg->itti_msg);
      itti_free (ITTI_MSG_ORIGIN_ID (msg->itti_msg), msg->itti_msg);
    }

    msg->itti_msg = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, NAS_UPLINK_DATA_IND);

    if (msg->itti_msg) {
      bstring xpath_expr = bformat("./%s",ACTION_XML_STR); // anywhere in XML tree
      res = xml_load_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &xml_msg_load_action_tag, (void*)&msg->is_tx);
      bdestroy_wrapper (&xpath_expr);

      if (res) {
        xpath_expr = bformat("./%s",ITTI_SENDER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_sender_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",ITTI_RECEIVER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_receiver_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        res = sp_mme_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.nas_ul_data_ind.ue_id);
      }

      if (res) {
        res = sp_nas_pdu_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.nas_ul_data_ind.nas_msg);
      }

      if (res) {
        res = sp_tracking_area_identity_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.nas_ul_data_ind.tai);
      }

      if (res) {
        res = sp_ecgi_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.nas_ul_data_ind.cgi);
      }
    } else {
      res = false;
    }
    if (saved_node_ptr) {
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  return (res)? RETURNok:RETURNerror;
}
//------------------------------------------------------------------------------
int xml_msg_load_itti_nas_downlink_data_req(scenario_t * const scenario, scenario_player_msg_t * const msg)
{
  bool res = false;
  if ((msg) && (msg->xml_doc)) {
    OAILOG_TRACE(LOG_XML, "Reloading NAS_DOWNLINK_DATA_REQ message\n");
    xmlNodePtr  cur = NULL;
    cur = xmlDocGetRootElement(msg->xml_doc);
    AssertFatal (cur, "Empty document");

    if (xmlStrcmp(cur->name, (const xmlChar *) ITTI_NAS_DOWNLINK_DATA_REQ_XML_STR)) {
      OAILOG_ERROR (LOG_XML, "Could not find tag %s\n", ITTI_NAS_DOWNLINK_DATA_REQ_XML_STR);
      return RETURNerror;
    }

    if (!msg->xpath_ctx) {
      // Create xpath evaluation context
      msg->xpath_ctx = xmlXPathNewContext(msg->xml_doc);
    }

    xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
    if (RETURNok != xmlXPathSetContextNode(cur, msg->xpath_ctx)) {
      return RETURNerror;
    }

    // free it (may be called from msp_reload_message)
    if (msg->itti_msg) {
      itti_free_msg_content (msg->itti_msg);
      itti_free (ITTI_MSG_ORIGIN_ID (msg->itti_msg), msg->itti_msg);
    }

    msg->itti_msg = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, NAS_DOWNLINK_DATA_REQ);

    if (msg->itti_msg) {
      bstring xpath_expr = bformat("./%s",ACTION_XML_STR); // anywhere in XML tree
      res = xml_load_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &xml_msg_load_action_tag, (void*)&msg->is_tx);
      bdestroy_wrapper (&xpath_expr);

      if (res) {
        xpath_expr = bformat("./%s",ITTI_SENDER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_sender_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",ITTI_RECEIVER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_receiver_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        res = sp_enb_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.nas_dl_data_req.enb_ue_s1ap_id);
      }

      if (res) {
        res = sp_mme_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.nas_dl_data_req.ue_id);
      }

      if (res) {
        res = sp_nas_pdu_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.nas_dl_data_req.nas_msg);
      }

    } else {
      res = false;
    }
    if (saved_node_ptr) {
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  return (res)? RETURNok:RETURNerror;
}
//------------------------------------------------------------------------------
int xml_msg_load_itti_nas_downlink_data_rej(scenario_t * const scenario, scenario_player_msg_t * const msg)
{
  bool res = false;
  if ((msg) && (msg->xml_doc)) {
    xmlNodePtr  cur = NULL;
    cur = xmlDocGetRootElement(msg->xml_doc);
    AssertFatal (cur, "Empty document");

    if (xmlStrcmp(cur->name, (const xmlChar *) ITTI_NAS_DOWNLINK_DATA_REJ_XML_STR)) {
      OAILOG_ERROR (LOG_XML, "Could not find tag %s\n", ITTI_NAS_DOWNLINK_DATA_REJ_XML_STR);
      return RETURNerror;
    }

    if (!msg->xpath_ctx) {
      // Create xpath evaluation context
      msg->xpath_ctx = xmlXPathNewContext(msg->xml_doc);
    }

    xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
    if (RETURNok != xmlXPathSetContextNode(cur, msg->xpath_ctx)) {
      return RETURNerror;
    }

    // free it (may be called from msp_reload_message)
    if (msg->itti_msg) {
      itti_free_msg_content (msg->itti_msg);
      itti_free (ITTI_MSG_ORIGIN_ID (msg->itti_msg), msg->itti_msg);
    }

    msg->itti_msg = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, NAS_DOWNLINK_DATA_REJ);

    if (msg->itti_msg) {
      bstring xpath_expr = bformat("./%s",ACTION_XML_STR); // anywhere in XML tree
      res = xml_load_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &xml_msg_load_action_tag, (void*)&msg->is_tx);
      bdestroy_wrapper (&xpath_expr);

      if (res) {
        xpath_expr = bformat("./%s",ITTI_SENDER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_sender_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",ITTI_RECEIVER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_receiver_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        res = sp_mme_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.nas_dl_data_rej.ue_id);
      }

      if (res) {
        res = sp_nas_pdu_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.nas_dl_data_rej.nas_msg);
      }
    } else {
      res = false;
    }
    if (saved_node_ptr) {
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  return (res)? RETURNok:RETURNerror;
}
//------------------------------------------------------------------------------
int xml_msg_load_itti_nas_downlink_data_cnf(scenario_t * const scenario, scenario_player_msg_t * const msg)
{
  bool res = false;
  if ((msg) && (msg->xml_doc)) {
    xmlNodePtr  cur = NULL;
    cur = xmlDocGetRootElement(msg->xml_doc);
    AssertFatal (cur, "Empty document");

    if (xmlStrcmp(cur->name, (const xmlChar *) ITTI_NAS_DOWNLINK_DATA_CNF_XML_STR)) {
      OAILOG_ERROR (LOG_XML, "Could not find tag %s\n", ITTI_NAS_DOWNLINK_DATA_CNF_XML_STR);
      return RETURNerror;
    }

    if (!msg->xpath_ctx) {
      // Create xpath evaluation context
      msg->xpath_ctx = xmlXPathNewContext(msg->xml_doc);
    }

    xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
    if (RETURNok != xmlXPathSetContextNode(cur, msg->xpath_ctx)) {
      return RETURNerror;
    }

    // free it (may be called from msp_reload_message)
    if (msg->itti_msg) {
      itti_free_msg_content (msg->itti_msg);
      itti_free (ITTI_MSG_ORIGIN_ID (msg->itti_msg), msg->itti_msg);
    }

    msg->itti_msg = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, NAS_DOWNLINK_DATA_CNF);

    if (msg->itti_msg) {
      bstring xpath_expr = bformat("./%s",ACTION_XML_STR); // anywhere in XML tree
      res = xml_load_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &xml_msg_load_action_tag, (void*)&msg->is_tx);
      bdestroy_wrapper (&xpath_expr);

      if (res) {
        xpath_expr = bformat("./%s",ITTI_SENDER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_sender_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        xpath_expr = bformat("./%s",ITTI_RECEIVER_TASK_XML_STR);
        res = xml_load_itti_task_id_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, &msg->itti_receiver_task);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        res = sp_mme_ue_s1ap_id_from_xml(scenario, msg, &msg->itti_msg->ittiMsg.nas_dl_data_cnf.ue_id);
      }

      if (res) {
        xpath_expr = bformat("./%s",NAS_ERROR_CODE_XML_STR);
        res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, "%"SCNx8, (void*)&msg->itti_msg->ittiMsg.nas_dl_data_cnf.err_code, NULL);
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
      }
    } else {
      res = false;
    }
    if (saved_node_ptr) {
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  return (res)? RETURNok:RETURNerror;
}



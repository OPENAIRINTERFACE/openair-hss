/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "log.h"
#include "dynamic_memory_check.h"
#include "secu_defs.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "3gpp_24.301.h"
#include "security_types.h"
#include "common_defs.h"
#include "common_types.h"
#include "mme_scenario_player.h"
#include "3gpp_36.401_xml.h"
#include "3gpp_24.007_xml.h"
#include "3gpp_24.301_ies_xml.h"
#include "xml2_wrapper.h"
#include "sp_xml_load.h"
#include "nas_message.h"
#include "sp_nas_message_xml.h"
#include "sp_3gpp_24.301_esm_msg_xml.h"
#include "sp_3gpp_24.301_emm_msg_xml.h"
#include "sp_3gpp_24.301_ies_xml.h"
#include "sp_3gpp_24.007_xml.h"




//------------------------------------------------------------------------------
bool sp_nas_message_plain_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    nas_message_plain_t       * const nas_message)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;
  bstring xpath_expr = bformat("./%s",PLAIN_NAS_MESSAGE_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (msg->xml_doc)) {
      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], msg->xpath_ctx));


      eps_protocol_discriminator_t protocol_discriminator = 0;
      if (res) {
        res = protocol_discriminator_from_xml (msg->xml_doc, msg->xpath_ctx, &protocol_discriminator);
      }
      if (res) {
        if (EPS_MOBILITY_MANAGEMENT_MESSAGE == protocol_discriminator) {
          // dump EPS Mobility Management L3 message
          res = sp_emm_msg_from_xml (scenario, msg, &nas_message->emm);
        } else if (EPS_SESSION_MANAGEMENT_MESSAGE == protocol_discriminator) {
          // Dump EPS Session Management L3 message
          res = sp_esm_msg_from_xml (scenario, msg, &nas_message->esm);
        } else {
          /*
           * Discard L3 messages with not supported protocol discriminator
           */
          OAILOG_WARNING(LOG_MME_SCENARIO_PLAYER, "NET-API   - Protocol discriminator 0x%x is " "not supported\n", nas_message->emm.header.protocol_discriminator);
        }
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_nas_message_protected_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    nas_message_t             * const nas_message)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;
  bstring xpath_expr = bformat("./%s",SECURITY_PROTECTED_NAS_MESSAGE_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (msg->xml_doc)) {
      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], msg->xpath_ctx));

      uint8_t security_header_type = 0;
      res = security_header_type_from_xml (msg->xml_doc, msg->xpath_ctx, &security_header_type, NULL);

      eps_protocol_discriminator_t protocol_discriminator = 0;
      if (res) {
        nas_message->security_protected.header.security_header_type = security_header_type;
        res = protocol_discriminator_from_xml (msg->xml_doc, msg->xpath_ctx, &protocol_discriminator);
      }

      uint32_t message_authentication_code = 0;
      if (res) {
        nas_message->security_protected.header.protocol_discriminator = protocol_discriminator;
        res = sp_mac_from_xml (scenario, msg, &message_authentication_code);
      }

      uint8_t sequence_number = 0;
      if (res) {
        nas_message->security_protected.header.message_authentication_code = message_authentication_code;
        res = sp_sequence_number_from_xml (scenario, msg, &sequence_number);
      }

      if (res) {
        nas_message->security_protected.header.sequence_number = sequence_number;
        res = sp_nas_message_plain_from_xml (scenario, msg, &nas_message->security_protected.plain);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;

    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_nas_pdu_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    bstring               *bnas_pdu)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  nas_message_t                           nas_msg = {.security_protected.header = {0},
                                                     .security_protected.plain.emm.header = {0},
                                                     .security_protected.plain.esm.header = {0}};
  unsigned char raw_buffer[4096];
  if (sp_nas_message_protected_from_xml(scenario, msg, &nas_msg)) {
    int rc = nas_message_encode(raw_buffer, &nas_msg, 4096, scenario->ue_emulated_emm_security_context);
    if (0 < rc) {
      *bnas_pdu = blk2bstr(raw_buffer, rc);
      OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, true);
    }
  } else if (sp_nas_message_plain_from_xml(scenario, msg, &nas_msg.plain)) {
    int rc = nas_message_encode(raw_buffer, &nas_msg, 4096, NULL);
    if (0 < rc) {
      *bnas_pdu = blk2bstr(raw_buffer, rc);
      OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, true);
    }
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, false);
}

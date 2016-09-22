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
#include <pthread.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "log.h"
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
#include "3gpp_23.003_xml.h"
#include "3gpp_24.008_xml.h"
#include "3gpp_24.301_emm_ies_xml.h"
#include "sp_3gpp_24.301_emm_ies_xml.h"
#include "conversions.h"
#include "xml2_wrapper.h"
#include "esm_msg.h"
#include "3gpp_24.301_esm_msg_xml.h"
#include "sp_3gpp_24.301_esm_msg_xml.h"

//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( additional_update_result, ADDITIONAL_UPDATE_RESULT);

//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( additional_update_type,   ADDITIONAL_UPDATE_TYPE);
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( csfb_response,            CSFB_RESPONSE);
//------------------------------------------------------------------------------
bool sp_detach_type_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    detach_type_t            * const detachtype)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( emm_cause,            EMM_CAUSE);
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( eps_attach_result,    EPS_ATTACH_RESULT);
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( eps_attach_type,      EPS_ATTACH_TYPE);
//------------------------------------------------------------------------------
bool sp_eps_mobile_identity_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    eps_mobile_identity_t          * const epsmobileidentity)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( eps_network_feature_support,      EPS_NETWORK_FEATURE_SUPPORT);
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( eps_update_result,                EPS_UPDATE_RESULT);
//------------------------------------------------------------------------------
bool sp_esm_message_container_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    EsmMessageContainer            esmmessagecontainer)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}
//------------------------------------------------------------------------------
bool sp_nas_key_set_identifier_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    NasKeySetIdentifier * const naskeysetidentifier)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}
//------------------------------------------------------------------------------
bool sp_nas_message_container_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    NasMessageContainer   * const nasmessagecontainer)
{
  OAILOG_FUNC_IN (LOG_NAS);
  bool res = false;

  bstring xpath_expr = bformat("./%s",NAS_MESSAGE_CONTAINER_IE_XML_STR);
  res = xml_load_hex_stream_leaf_tag(msg->xml_doc,msg->xpath_ctx, xpath_expr, nasmessagecontainer);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_NAS, res);
}

//------------------------------------------------------------------------------
bool sp_nas_security_algorithms_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    NasSecurityAlgorithms          * const nassecurityalgorithms)
{
  bool res = false;
  bstring xpath_expr_nsa = bformat("./%s",NAS_SECURITY_ALGORITHMS_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj_nsa = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr_nsa);
  if (xpath_obj_nsa) {
    xmlNodeSetPtr nodes_nsa = xpath_obj_nsa->nodesetval;
    int size = (nodes_nsa) ? nodes_nsa->nodeNr : 0;
    if ((1 == size) && (msg->xml_doc)) {

      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_nsa->nodeTab[0], msg->xpath_ctx));
      if (res) {
        uint64_t  typeofcipheringalgorithm = 0;
        res = sp_u64_from_xml (scenario, msg, &typeofcipheringalgorithm, TYPE_OF_CYPHERING_ALGORITHM_ATTR_XML_STR);
        nassecurityalgorithms->typeofcipheringalgorithm = typeofcipheringalgorithm;
      }
      if (res) {
        uint64_t  typeofintegrityalgorithm = 0;
        res = sp_u64_from_xml (scenario, msg, &typeofintegrityalgorithm,TYPE_OF_INTEGRITY_PROTECTION_ALGORITHM_ATTR_XML_STR);
        nassecurityalgorithms->typeofintegrityalgorithm = typeofintegrityalgorithm;
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
  }
  bdestroy_wrapper (&xpath_expr_nsa);
  return res;
}
//------------------------------------------------------------------------------
bool sp_nonce_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    const char * const ie, nonce_t * const nonce)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}

//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( paging_identity, PAGING_IDENTITY);
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( service_type, SERVICE_TYPE);
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( short_mac, SHORT_MAC);
//------------------------------------------------------------------------------
bool sp_tracking_area_identity_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    tai_t * const tai)
{
#warning "TODO"
  return tracking_area_identity_from_xml(msg->xml_doc, msg->xpath_ctx, tai);
}

//------------------------------------------------------------------------------
bool sp_tracking_area_code_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    tac_t                 * const tac)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}

//------------------------------------------------------------------------------
bool sp_tracking_area_identity_list_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    tai_list_t            * const tai_list)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}

//------------------------------------------------------------------------------
bool sp_ue_network_capability_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    ue_network_capability_t * const uenetworkcapability)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}

//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( ue_radio_capability_information_update_needed, UE_RADIO_CAPABILITY_INFORMATION_UPDATE_NEEDED);
//------------------------------------------------------------------------------
bool sp_ue_security_capability_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    ue_security_capability_t           * const uesecuritycapability)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( ss_code, SS_CODE);
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( lcs_indicator, LCS_INDICATOR);
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool sp_guti_type_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    guti_type_t * const gutitype)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}


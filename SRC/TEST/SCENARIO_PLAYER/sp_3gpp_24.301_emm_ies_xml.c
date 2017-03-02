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
#include "sp_3gpp_23.003_xml.h"
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
    eps_mobile_identity_t * const epsmobileidentity)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;
  bstring xpath_expr_mi = bformat("./%s",EPS_MOBILE_IDENTITY_XML_STR);
  xmlXPathObjectPtr xpath_obj_mi = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr_mi);
  if (xpath_obj_mi) {
    xmlNodeSetPtr nodes_mi = xpath_obj_mi->nodesetval;
    int size = (nodes_mi) ? nodes_mi->nodeNr : 0;
    if ((1 == size) && (msg->xml_doc)) {

      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_mi->nodeTab[0], msg->xpath_ctx));
      if (res) {
        uint8_t  typeofidentity = 0;
        bstring xpath_expr = bformat("./%s",TYPE_OF_IDENTITY_ATTR_XML_STR);
        res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, "%"SCNx8, (void*)&typeofidentity, NULL);
        bdestroy_wrapper (&xpath_expr);
        if (res) {
          switch (typeofidentity) {
          case EPS_MOBILE_IDENTITY_IMSI: {
            epsmobileidentity->imsi.typeofidentity = EPS_MOBILE_IDENTITY_IMSI;

              uint8_t  oddeven = 0;
              bstring xpath_expr = bformat("./%s",ODDEVEN_ATTR_XML_STR);
              res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, "%"SCNx8, (void*)&oddeven, NULL);
              bdestroy_wrapper (&xpath_expr);
              epsmobileidentity->imsi.oddeven = oddeven;

              if (res) {
                bstring imsi_bstr = NULL;
                uint8_t digit[15] = {0};
                xpath_expr = bformat("./%s",IMSI_ATTR_XML_STR);
                res =  sp_xml_load_ascii_stream_leaf_tag(scenario, msg, xpath_expr, &imsi_bstr);
                bdestroy_wrapper (&xpath_expr);
                if (res) {
                  int ret = sscanf((const char*)bdata(imsi_bstr),
                      "%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8,
                      &digit[0], &digit[1], &digit[2], &digit[3], &digit[4], &digit[5], &digit[6], &digit[7],
                      &digit[8], &digit[9], &digit[10], &digit[11], &digit[12], &digit[13], &digit[14]);
                  if (1 <= ret) {
                    for (int ff=ret; ff < 15; ff++) {
                      digit[ff] = 0xf;
                    }
                    epsmobileidentity->imsi.identity_digit1 = digit[0];
                    epsmobileidentity->imsi.identity_digit2 = digit[1];
                    epsmobileidentity->imsi.identity_digit3 = digit[2];
                    epsmobileidentity->imsi.identity_digit4 = digit[3];
                    epsmobileidentity->imsi.identity_digit5 = digit[4];
                    epsmobileidentity->imsi.identity_digit6 = digit[5];
                    epsmobileidentity->imsi.identity_digit7 = digit[6];
                    epsmobileidentity->imsi.identity_digit8 = digit[7];
                    epsmobileidentity->imsi.identity_digit9 = digit[8];
                    epsmobileidentity->imsi.identity_digit10 = digit[9];
                    epsmobileidentity->imsi.identity_digit11 = digit[10];
                    epsmobileidentity->imsi.identity_digit12 = digit[11];
                    epsmobileidentity->imsi.identity_digit13 = digit[12];
                    epsmobileidentity->imsi.identity_digit14 = digit[13];
                    epsmobileidentity->imsi.identity_digit15 = digit[14];
                    epsmobileidentity->imsi.num_digits = ret;
                  }
                  res = (14 <= ret) && (15 >= ret);
                  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found IMSI/%s = %x%x%x%x%x%x%x%x%x%x%x%x%x%x%x num digits %d\n",
                      IMSI_ATTR_XML_STR,
                      epsmobileidentity->imsi.identity_digit1,
                      epsmobileidentity->imsi.identity_digit2,
                      epsmobileidentity->imsi.identity_digit3,
                      epsmobileidentity->imsi.identity_digit4,
                      epsmobileidentity->imsi.identity_digit5,
                      epsmobileidentity->imsi.identity_digit6,
                      epsmobileidentity->imsi.identity_digit7,
                      epsmobileidentity->imsi.identity_digit8,
                      epsmobileidentity->imsi.identity_digit9,
                      epsmobileidentity->imsi.identity_digit10,
                      epsmobileidentity->imsi.identity_digit11,
                      epsmobileidentity->imsi.identity_digit12,
                      epsmobileidentity->imsi.identity_digit13,
                      epsmobileidentity->imsi.identity_digit14,
                      epsmobileidentity->imsi.identity_digit15,
                      epsmobileidentity->imsi.num_digits);
                }
                bdestroy_wrapper (&imsi_bstr);
              }
            }
            break;

          case EPS_MOBILE_IDENTITY_IMEI: {
              epsmobileidentity->imei.typeofidentity = EPS_MOBILE_IDENTITY_IMEI;

              uint8_t  oddeven = 0;
              bstring xpath_expr = bformat("./%s",ODDEVEN_ATTR_XML_STR);
              res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, "%"SCNx8, (void*)&oddeven, NULL);
              bdestroy_wrapper (&xpath_expr);
              epsmobileidentity->imei.oddeven = oddeven;

              if (res) {
                bstring imei_bstr = NULL;
                xpath_expr = bformat("./%s",IMEI_XML_STR); // TODO CHECK IMSI/IMEI_ATTR_XML_STR
                res =  sp_xml_load_ascii_stream_leaf_tag(scenario, msg, xpath_expr, &imei_bstr);
                bdestroy_wrapper (&xpath_expr);
                if (res) {
                  uint8_t digit[15] = {0};
                  int ret = sscanf((const char*)imei_bstr,
                      "%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8,
                      &digit[0], &digit[1], &digit[2], &digit[3], &digit[4], &digit[5], &digit[6], &digit[7],
                      &digit[8], &digit[9], &digit[10], &digit[11], &digit[12], &digit[13], &digit[14]);
                  if (15 == ret) {
                    epsmobileidentity->imei.identity_digit1 = digit[0];
                    epsmobileidentity->imei.identity_digit2 = digit[1];
                    epsmobileidentity->imei.identity_digit3 = digit[2];
                    epsmobileidentity->imei.identity_digit4 = digit[3];
                    epsmobileidentity->imei.identity_digit5 = digit[4];
                    epsmobileidentity->imei.identity_digit6 = digit[5];
                    epsmobileidentity->imei.identity_digit7 = digit[6];
                    epsmobileidentity->imei.identity_digit8 = digit[7];
                    epsmobileidentity->imei.identity_digit9 = digit[8];
                    epsmobileidentity->imei.identity_digit10 = digit[9];
                    epsmobileidentity->imei.identity_digit11 = digit[10];
                    epsmobileidentity->imei.identity_digit12 = digit[11];
                    epsmobileidentity->imei.identity_digit13 = digit[12];
                    epsmobileidentity->imei.identity_digit14 = digit[13];
                    epsmobileidentity->imei.identity_digit15 = digit[14];
                  }
                  res = (15 == ret);
                  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found IMEI/%s = %x%x%x%x%x%x%x%x%x%x%x%x%x%x%x\n",
                      IMEI_XML_STR,
                      epsmobileidentity->imei.identity_digit1,
                      epsmobileidentity->imei.identity_digit2,
                      epsmobileidentity->imei.identity_digit3,
                      epsmobileidentity->imei.identity_digit4,
                      epsmobileidentity->imei.identity_digit5,
                      epsmobileidentity->imei.identity_digit6,
                      epsmobileidentity->imei.identity_digit7,
                      epsmobileidentity->imei.identity_digit8,
                      epsmobileidentity->imei.identity_digit9,
                      epsmobileidentity->imei.identity_digit10,
                      epsmobileidentity->imei.identity_digit11,
                      epsmobileidentity->imei.identity_digit12,
                      epsmobileidentity->imei.identity_digit13,
                      epsmobileidentity->imei.identity_digit14,
                      epsmobileidentity->imei.identity_digit15);
                }
                bdestroy_wrapper (&imei_bstr);
              }
            }
            break;

          case EPS_MOBILE_IDENTITY_GUTI: {
              epsmobileidentity->guti.typeofidentity = EPS_MOBILE_IDENTITY_GUTI;

              uint8_t  oddeven = 0;
              bstring xpath_expr = bformat("./%s",ODDEVEN_ATTR_XML_STR);
              res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, "%"SCNx8, (void*)&oddeven, NULL);
              bdestroy_wrapper (&xpath_expr);
              epsmobileidentity->guti.oddeven = oddeven;
              if (res) {res = sp_mme_gid_from_xml(scenario, msg, &epsmobileidentity->guti.mme_group_id);}
              if (res) {res = sp_mme_code_from_xml (scenario, msg, &epsmobileidentity->guti.mme_code);}
              if (res) {res = sp_m_tmsi_from_xml (scenario, msg, &epsmobileidentity->guti.m_tmsi);}
              if (res) {
                uint64_t mcc64 = 0;
                res =  sp_u64_from_xml(scenario, msg, &mcc64, MOBILE_COUNTRY_CODE_ATTR_XML_STR);
                if (res) {
                  epsmobileidentity->guti.mcc_digit1 = (mcc64 & 0x000000F00) >> 8;
                  epsmobileidentity->guti.mcc_digit2 = (mcc64 & 0x0000000F0) >> 4;
                  epsmobileidentity->guti.mcc_digit3 = (mcc64 & 0x00000000F);
                  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found GUTI/%s = %x%x%x\n",
                      MOBILE_COUNTRY_CODE_ATTR_XML_STR,
                      epsmobileidentity->guti.mcc_digit1,
                      epsmobileidentity->guti.mcc_digit2,
                      epsmobileidentity->guti.mcc_digit3);
                }
              }
              if (res) {
                uint64_t mnc64 = 0;
                res =  sp_u64_from_xml(scenario, msg, &mnc64, MOBILE_NETWORK_CODE_ATTR_XML_STR);
                if (res) {
                  epsmobileidentity->guti.mnc_digit1 = (mnc64 & 0x000000F00) >> 8;
                  epsmobileidentity->guti.mnc_digit2 = (mnc64 & 0x0000000F0) >> 4;
                  epsmobileidentity->guti.mnc_digit3 = (mnc64 & 0x00000000F);
                  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found GUTI/%s = %x%x%x\n",
                      MOBILE_NETWORK_CODE_ATTR_XML_STR,
                      epsmobileidentity->guti.mnc_digit1,
                      epsmobileidentity->guti.mnc_digit2,
                      epsmobileidentity->guti.mnc_digit3);
                }
              }
            }
            break;

          default:
            res = false;
          }
        }
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_mi);
  }
  bdestroy_wrapper (&xpath_expr_mi);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( eps_network_feature_support,      EPS_NETWORK_FEATURE_SUPPORT);
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( eps_update_result,                EPS_UPDATE_RESULT);
//------------------------------------------------------------------------------
bool sp_esm_message_container_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    EsmMessageContainer            *esmmessagecontainer)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr_mi = bformat("./%s",ESM_MESSAGE_CONTAINER_XML_STR);
  xmlXPathObjectPtr xpath_obj_mi = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr_mi);
  if (xpath_obj_mi) {
    xmlNodeSetPtr nodes_mi = xpath_obj_mi->nodesetval;
    int size = (nodes_mi) ? nodes_mi->nodeNr : 0;
    if ((1 == size) && (msg->xml_doc)) {

      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_mi->nodeTab[0], msg->xpath_ctx));
      if (res) {
        ESM_msg esm;
        memset(&esm, 0, sizeof(esm));
        // Do not load attribute ESM_MESSAGE_CONTAINER_HEX_DUMP_ATTR_XML_STR now. May be used for check ? ...no
        res = sp_esm_msg_from_xml (scenario, msg, &esm);
        if (res) {
#define ESM_BUFF_1024 1024
          uint8_t buffer[ESM_BUFF_1024];
          int rc = esm_msg_encode (&esm, buffer, ESM_BUFF_1024);
          bdestroy_wrapper(esmmessagecontainer);
          if ((0 < rc) && (ESM_BUFF_1024 >= rc)) {
            *esmmessagecontainer = blk2bstr(buffer, rc);
          }
        }
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_mi);
  }
  bdestroy_wrapper (&xpath_expr_mi);
  OAILOG_FUNC_RETURN (LOG_XML, res);
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
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;

  bstring xpath_expr = bformat("./%s",NAS_MESSAGE_CONTAINER_XML_STR);
  res = xml_load_hex_stream_leaf_tag(msg->xml_doc,msg->xpath_ctx, xpath_expr, nasmessagecontainer);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_nas_security_algorithms_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    NasSecurityAlgorithms          * const nassecurityalgorithms)
{
  bool res = false;
  bstring xpath_expr_nsa = bformat("./%s",NAS_SECURITY_ALGORITHMS_XML_STR);
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
    xmlXPathFreeObject(xpath_obj_nsa);
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


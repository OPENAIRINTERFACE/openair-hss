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
#include "assertions.h"
#include "common_defs.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "log.h"
#include "mme_scenario_player.h"
#include "xml_load.h"
#include "3gpp_24.008_xml.h"
#include "sp_3gpp_24.008_xml.h"
#include "xml2_wrapper.h"


//******************************************************************************
// 10.5.1 Common information elements
//******************************************************************************
//------------------------------------------------------------------------------
// 10.5.1.2 Ciphering Key Sequence Number
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE(ciphering_key_sequence_number, CIPHERING_KEY_SEQUENCE_NUMBER);
//------------------------------------------------------------------------------
// 10.5.1.3 Location Area Identification
//------------------------------------------------------------------------------
bool sp_location_area_identification_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    location_area_identification_t * const locationareaidentification)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}

//------------------------------------------------------------------------------
// 10.5.1.4 Mobile Identity
//------------------------------------------------------------------------------
bool sp_mobile_identity_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    mobile_identity_t              * const mobileidentity)
{
  bool res = false;
  bstring xpath_expr_mi = bformat("./%s",MOBILE_IDENTITY_XML_STR);
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
          case MOBILE_IDENTITY_IMSI: {
              mobileidentity->imsi.typeofidentity = MOBILE_IDENTITY_IMSI;

              uint8_t  oddeven = 0;
              bstring xpath_expr = bformat("./%s",ODDEVEN_ATTR_XML_STR);
              res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, "%"SCNx8, (void*)&oddeven, NULL);
              bdestroy_wrapper (&xpath_expr);
              mobileidentity->imsi.oddeven = oddeven;

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
                    mobileidentity->imsi.digit1 = digit[0];
                    mobileidentity->imsi.digit2 = digit[1];
                    mobileidentity->imsi.digit3 = digit[2];
                    mobileidentity->imsi.digit4 = digit[3];
                    mobileidentity->imsi.digit5 = digit[4];
                    mobileidentity->imsi.digit6 = digit[5];
                    mobileidentity->imsi.digit7 = digit[6];
                    mobileidentity->imsi.digit8 = digit[7];
                    mobileidentity->imsi.digit9 = digit[8];
                    mobileidentity->imsi.digit10 = digit[9];
                    mobileidentity->imsi.digit11 = digit[10];
                    mobileidentity->imsi.digit12 = digit[11];
                    mobileidentity->imsi.digit13 = digit[12];
                    mobileidentity->imsi.digit14 = digit[13];
                    mobileidentity->imsi.digit15 = digit[14];
                  }
                  res = (1 <= ret);
                }
                bdestroy_wrapper (&imsi_bstr);
              }
            }
            OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s/%s = %u%u%u%u%u%u%u%u%u%u%u%u%u%u%u\n",
                MOBILE_IDENTITY_XML_STR, IMSI_ATTR_XML_STR,
                mobileidentity->imsi.digit1, mobileidentity->imsi.digit2,
                mobileidentity->imsi.digit3, mobileidentity->imsi.digit4,
                mobileidentity->imsi.digit5, mobileidentity->imsi.digit6,
                mobileidentity->imsi.digit7, mobileidentity->imsi.digit8,
                mobileidentity->imsi.digit9, mobileidentity->imsi.digit10,
                mobileidentity->imsi.digit11, mobileidentity->imsi.digit12,
                mobileidentity->imsi.digit13, mobileidentity->imsi.digit14,
                mobileidentity->imsi.digit15);
            break;

          case MOBILE_IDENTITY_IMEI:
            AssertFatal(0, "TODO");
            break;

          case MOBILE_IDENTITY_IMEISV:
            AssertFatal(0, "TODO");

            break;

          case MOBILE_IDENTITY_TMSI:
            AssertFatal(0, "TODO");
            break;

          case MOBILE_IDENTITY_TMGI:
              mobileidentity->tmgi.typeofidentity = MOBILE_IDENTITY_TMGI;
              AssertFatal(0, "TODO");
            break;

          case MOBILE_IDENTITY_NOT_AVAILABLE: {
              mobileidentity->no_id.typeofidentity = MOBILE_IDENTITY_NOT_AVAILABLE;

              uint8_t  oddeven = 0;
              bstring xpath_expr = bformat("./%s",ODDEVEN_ATTR_XML_STR);
              res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, "%"SCNx8, (void*)&oddeven, NULL);
              bdestroy_wrapper (&xpath_expr);
              mobileidentity->no_id.oddeven = oddeven;
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
  return res;
}

//------------------------------------------------------------------------------
// 10.5.1.6 Mobile Station Classmark 2
//------------------------------------------------------------------------------
bool sp_mobile_station_classmark_2_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    mobile_station_classmark2_t * const mobilestationclassmark2)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}

//------------------------------------------------------------------------------
// 10.5.1.7 Mobile Station Classmark 3
//------------------------------------------------------------------------------
bool sp_mobile_station_classmark_3_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    mobile_station_classmark3_t * const mobilestationclassmark3)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}

//------------------------------------------------------------------------------
// 10.5.1.13 PLMN list
//------------------------------------------------------------------------------
bool sp_plmn_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    plmn_t                * const plmn)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;
  bstring xpath_expr_plmn = bformat("./%s",PLMN_XML_STR);
  xmlXPathObjectPtr xpath_obj_plmn = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr_plmn);
  if (xpath_obj_plmn) {
    xmlNodeSetPtr nodes_plmn = xpath_obj_plmn->nodesetval;
    int size = (nodes_plmn) ? nodes_plmn->nodeNr : 0;
    if ((1 == size) && (msg->xml_doc)) {

      xmlNodePtr saved_node_ptr = msg->xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_plmn->nodeTab[0], msg->xpath_ctx));
      if (res) {
        res = false;
        bstring xpath_expr = bformat("./%s",MOBILE_COUNTRY_CODE_ATTR_XML_STR);
        xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
        if (xpath_obj) {
          xmlNodeSetPtr nodes = xpath_obj->nodesetval;
          size = (nodes) ? nodes->nodeNr : 0;
          if ((1 == size) && (msg->xml_doc)) {
            xmlChar *key = xmlNodeListGetString(msg->xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
            uint8_t mcc_digit[3] = {0};
            int ret = sscanf((const char*)key, "%1"SCNx8"%1"SCNx8"%1"SCNx8, &mcc_digit[0], &mcc_digit[1], &mcc_digit[2]);
            xmlFree(key);
            if (3 == ret) {
              plmn->mcc_digit1 = mcc_digit[0];
              plmn->mcc_digit2 = mcc_digit[1];
              plmn->mcc_digit3 = mcc_digit[2];
              OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s/%s = %x%x%x\n",
                  PLMN_XML_STR, MOBILE_COUNTRY_CODE_ATTR_XML_STR,
                  plmn->mcc_digit1, plmn->mcc_digit2, plmn->mcc_digit3);
            }
            res = (3 == ret);
          }
          xmlXPathFreeObject(xpath_obj);
        }
        bdestroy_wrapper (&xpath_expr);
      }

      if (res) {
        res = false;
        bstring xpath_expr = bformat("./%s",MOBILE_NETWORK_CODE_ATTR_XML_STR);
        xmlXPathObjectPtr xpath_obj = xml_find_nodes(msg->xml_doc, &msg->xpath_ctx, xpath_expr);
        if (xpath_obj) {
          xmlNodeSetPtr nodes = xpath_obj->nodesetval;
          int size = (nodes) ? nodes->nodeNr : 0;
          if ((1 == size) && (msg->xml_doc)) {
            xmlChar *key = xmlNodeListGetString(msg->xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
            uint8_t mnc_digit[3] = {0};
            int ret = sscanf((const char*)key, "%1"SCNx8"%1"SCNx8"%1"SCNx8, &mnc_digit[0], &mnc_digit[1], &mnc_digit[2]);
            xmlFree(key);
            if (3 == ret) {
              plmn->mnc_digit1 = mnc_digit[0];
              plmn->mnc_digit2 = mnc_digit[1];
              plmn->mnc_digit3 = mnc_digit[2];
              OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s/%s = %x%x%x\n",
                  PLMN_XML_STR, MOBILE_NETWORK_CODE_ATTR_XML_STR,
                  plmn->mnc_digit1, plmn->mnc_digit2, plmn->mnc_digit3);
            }
            res = (3 == ret);
          }
          xmlXPathFreeObject(xpath_obj);
        }
        bdestroy_wrapper (&xpath_expr);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, msg->xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_plmn);
  }
  bdestroy_wrapper (&xpath_expr_plmn);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_plmn_list_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    plmn_list_t           * const plmnlist)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}
//------------------------------------------------------------------------------
// 10.5.1.15 MS network feature support
//------------------------------------------------------------------------------
bool sp_ms_network_feature_support_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    ms_network_feature_support_t * const msnetworkfeaturesupport)
{
  AssertFatal(0, "TODO if necessary");
  return false;
}


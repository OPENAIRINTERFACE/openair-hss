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
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_24.301.h"
#include "3gpp_33.401.h"
#include "common_defs.h"
#include "security_types.h"
#include "common_types.h"
#include "3gpp_23.003_xml.h"
#include "3gpp_24.008_xml.h"
#include "3gpp_24.301_emm_ies_xml.h"
#include "3gpp_24.301_esm_msg_xml.h"
#include "conversions.h"
#include "xml2_wrapper.h"
#include "esm_msg.h"

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( additional_update_result, ADDITIONAL_UPDATE_RESULT);
//------------------------------------------------------------------------------
void additional_update_result_to_xml(additional_update_result_t *additionalupdateresult, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, ADDITIONAL_UPDATE_RESULT_XML_STR, "0x%"PRIx8, *additionalupdateresult);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( additional_update_type,   ADDITIONAL_UPDATE_TYPE);
//------------------------------------------------------------------------------
void additional_update_type_to_xml(additional_update_type_t *additionalupdatetype, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, ADDITIONAL_UPDATE_TYPE_XML_STR, "0x%"PRIx8, *additionalupdatetype);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( csfb_response,            CSFB_RESPONSE);
//------------------------------------------------------------------------------
void csfb_response_to_xml(csfb_response_t *csfbresponse, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, CSFB_RESPONSE_XML_STR, "0x%x", *csfbresponse);
}

//------------------------------------------------------------------------------
bool detach_type_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    detach_type_t            * const detachtype)
{
  OAILOG_FUNC_IN (LOG_XML);
  memset(detachtype, 0, sizeof(*detachtype));
  bool res = false;
  bstring xpath_expr = bformat("./%s",DETACH_TYPE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));

      bstring xpath_expr_so = bformat("./%s",SWITCH_OFF_ATTR_XML_STR);
      res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_so, "%"SCNx8, (void*)&detachtype->switchoff, NULL);
      bdestroy_wrapper (&xpath_expr_so);

      bstring xpath_expr_tod = bformat("./%s",TYPE_OF_DETACH_ATTR_XML_STR);
      res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_tod, "%"SCNx8, (void*)&detachtype->typeofdetach, NULL);
      bdestroy_wrapper (&xpath_expr_tod);

      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
void detach_type_to_xml(detach_type_t *detachtype, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, DETACH_TYPE_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, SWITCH_OFF_ATTR_XML_STR, "0x%"PRIx8, detachtype->switchoff);
  XML_WRITE_FORMAT_ELEMENT(writer, TYPE_OF_DETACH_ATTR_XML_STR, "0x%"PRIx8, detachtype->typeofdetach);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( emm_cause,            EMM_CAUSE);
//------------------------------------------------------------------------------
void emm_cause_to_xml (emm_cause_t * emmcause, xmlTextWriterPtr writer)
{
  XML_WRITE_COMMENT(writer, "TODO human readable form, even in comment ?")
  XML_WRITE_FORMAT_ELEMENT(writer, EMM_CAUSE_XML_STR, "0x%"PRIx8, *emmcause);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( eps_attach_result,    EPS_ATTACH_RESULT);
//------------------------------------------------------------------------------
void eps_attach_result_to_xml (eps_attach_result_t * epsattachresult, xmlTextWriterPtr writer)
{
  XML_WRITE_COMMENT(writer, "EPS_ATTACH_RESULT EPS=0x1, IMSI=0x2");
  XML_WRITE_FORMAT_ELEMENT(writer, EPS_ATTACH_RESULT_XML_STR, "0x%x", *epsattachresult);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( eps_attach_type,      EPS_ATTACH_TYPE);
//------------------------------------------------------------------------------
void eps_attach_type_to_xml (eps_attach_type_t * epsattachtype, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, EPS_ATTACH_TYPE_XML_STR, "0x%"PRIx8, *epsattachtype);
}

//------------------------------------------------------------------------------
bool eps_mobile_identity_from_xml (
    xmlDocPtr                              xml_doc,
    xmlXPathContextPtr                     xpath_ctx,
    eps_mobile_identity_t          * const epsmobileidentity)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr_mi = bformat("./%s",EPS_MOBILE_IDENTITY_XML_STR);
  xmlXPathObjectPtr xpath_obj_mi = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_mi);
  if (xpath_obj_mi) {
    xmlNodeSetPtr nodes_mi = xpath_obj_mi->nodesetval;
    int size = (nodes_mi) ? nodes_mi->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_mi->nodeTab[0], xpath_ctx));
      if (res) {
        uint8_t  typeofidentity = 0;
        bstring xpath_expr = bformat("./%s",TYPE_OF_IDENTITY_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&typeofidentity, NULL);
        bdestroy_wrapper (&xpath_expr);
        if (res) {
          switch (typeofidentity) {
          case EPS_MOBILE_IDENTITY_IMSI: {
            epsmobileidentity->imsi.typeofidentity = EPS_MOBILE_IDENTITY_IMSI;

              uint8_t  oddeven = 0;
              bstring xpath_expr = bformat("./%s",ODDEVEN_ATTR_XML_STR);
              res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&oddeven, NULL);
              bdestroy_wrapper (&xpath_expr);
              epsmobileidentity->imsi.oddeven = oddeven;

              if (res) {
                char imsi_str[32] = {0}; // MOBILE_IDENTITY_IE_IMSI_LENGTH*2+1
                bstring xpath_expr = bformat("./%s",IMSI_ATTR_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)&imsi_str, NULL);
                bdestroy_wrapper (&xpath_expr);
                if (res) {
                  uint8_t digit[15] = {0};
                  int ret = sscanf((const char*)imsi_str,
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
              }
            }
            break;

          case EPS_MOBILE_IDENTITY_IMEI: {
              epsmobileidentity->imei.typeofidentity = EPS_MOBILE_IDENTITY_IMEI;

              uint8_t  oddeven = 0;
              bstring xpath_expr = bformat("./%s",ODDEVEN_ATTR_XML_STR);
              res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&oddeven, NULL);
              bdestroy_wrapper (&xpath_expr);
              epsmobileidentity->imei.oddeven = oddeven;

              if (res) {
                char imei_str[32] = {0}; // MOBILE_IDENTITY_IE_IMEI_LENGTH*2+1
                bstring xpath_expr = bformat("./%s",IMEI_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)&imei_str, NULL);
                bdestroy_wrapper (&xpath_expr);
                if (res) {
                  uint8_t digit[15] = {0};
                  int ret = sscanf((const char*)imei_str,
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
              }
            }
            break;

          case EPS_MOBILE_IDENTITY_GUTI: {
              epsmobileidentity->guti.typeofidentity = EPS_MOBILE_IDENTITY_GUTI;

              uint8_t  oddeven = 0;
              bstring xpath_expr = bformat("./%s",ODDEVEN_ATTR_XML_STR);
              res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&oddeven, NULL);
              bdestroy_wrapper (&xpath_expr);
              epsmobileidentity->guti.oddeven = oddeven;
              if (res) {res = mme_gid_from_xml(xml_doc, xpath_ctx, &epsmobileidentity->guti.mme_group_id, NULL);}
              if (res) {res = mme_code_from_xml (xml_doc, xpath_ctx, &epsmobileidentity->guti.mme_code, NULL);}
              if (res) {res = m_tmsi_from_xml (xml_doc, xpath_ctx, &epsmobileidentity->guti.m_tmsi);}
              if (res) {
                res = false;
                bstring xpath_expr = bformat("./%s",MOBILE_COUNTRY_CODE_ATTR_XML_STR);
                xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
                if (xpath_obj) {
                  xmlNodeSetPtr nodes = xpath_obj->nodesetval;
                  int size = (nodes) ? nodes->nodeNr : 0;
                  if ((1 == size) && (xml_doc)) {
                    xmlChar *key = xmlNodeListGetString(xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
                    uint8_t mcc_digit[3] = {0};
                    int ret = sscanf((const char*)key, "%1"SCNx8"%1"SCNx8"%1"SCNx8, &mcc_digit[0], &mcc_digit[1], &mcc_digit[2]);
                    xmlFree(key);
                    if (3 == ret) {
                      epsmobileidentity->guti.mcc_digit1 = mcc_digit[0];
                      epsmobileidentity->guti.mcc_digit2 = mcc_digit[1];
                      epsmobileidentity->guti.mcc_digit3 = mcc_digit[2];
                      OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found GUTI/%s = %x%x%x\n",
                          MOBILE_COUNTRY_CODE_ATTR_XML_STR,
                          epsmobileidentity->guti.mcc_digit1,
                          epsmobileidentity->guti.mcc_digit2,
                          epsmobileidentity->guti.mcc_digit3);
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
                xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
                if (xpath_obj) {
                  xmlNodeSetPtr nodes = xpath_obj->nodesetval;
                  int size = (nodes) ? nodes->nodeNr : 0;
                  if ((1 == size) && (xml_doc)) {
                    xmlChar *key = xmlNodeListGetString(xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
                    uint8_t mnc_digit[3] = {0};
                    int ret = sscanf((const char*)key, "%1"SCNx8"%1"SCNx8"%1"SCNx8, &mnc_digit[0], &mnc_digit[1], &mnc_digit[2]);
                    xmlFree(key);
                    if (3 == ret) {
                      epsmobileidentity->guti.mnc_digit1 = mnc_digit[0];
                      epsmobileidentity->guti.mnc_digit2 = mnc_digit[1];
                      epsmobileidentity->guti.mnc_digit3 = mnc_digit[2];
                      OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found GUTI/%s = %x%x%x\n",
                          MOBILE_NETWORK_CODE_ATTR_XML_STR,
                          epsmobileidentity->guti.mnc_digit1, epsmobileidentity->guti.mnc_digit2, epsmobileidentity->guti.mnc_digit3);
                    }
                    res = (3 == ret);
                  }
                  xmlXPathFreeObject(xpath_obj);
                }
                bdestroy_wrapper (&xpath_expr);
              }
            }
            break;

          default:
            res = false;
          }
        }
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_mi);
  }
  bdestroy_wrapper (&xpath_expr_mi);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
void eps_mobile_identity_to_xml (eps_mobile_identity_t * epsmobileidentity, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, EPS_MOBILE_IDENTITY_XML_STR);

  if (epsmobileidentity->imsi.typeofidentity == EPS_MOBILE_IDENTITY_IMSI) {
    imsi_eps_mobile_identity_t                *imsi = &epsmobileidentity->imsi;
    XML_WRITE_FORMAT_ELEMENT(writer, ODDEVEN_ATTR_XML_STR, "0x%x", imsi->oddeven);
    XML_WRITE_COMMENT(writer, "type_of_identity = IMSI");
    XML_WRITE_FORMAT_ELEMENT(writer, TYPE_OF_IDENTITY_ATTR_XML_STR, "0x%x", EPS_MOBILE_IDENTITY_IMSI);
    bstring imsi_str = bfromcstralloc(17, "");
    bformata(imsi_str, "%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x",
        imsi->identity_digit1,imsi->identity_digit2,imsi->identity_digit3,
        imsi->identity_digit4, imsi->identity_digit5,imsi->identity_digit6,
        imsi->identity_digit7,imsi->identity_digit8, imsi->identity_digit9,
        imsi->identity_digit10,imsi->identity_digit11,imsi->identity_digit12,
        imsi->identity_digit13,imsi->identity_digit14,imsi->identity_digit15);
    // !....!
    if (imsi->num_digits < 15) {
      btrunc(imsi_str, imsi->num_digits);
    }
    XML_WRITE_FORMAT_ELEMENT(writer, IMSI_ATTR_XML_STR, "%s", bdata(imsi_str));
    bdestroy_wrapper(&imsi_str);
  } else if (epsmobileidentity->guti.typeofidentity == EPS_MOBILE_IDENTITY_GUTI) {
    guti_eps_mobile_identity_t                *guti = &epsmobileidentity->guti;

    XML_WRITE_FORMAT_ELEMENT(writer, ODDEVEN_ATTR_XML_STR, "0x%x", guti->oddeven);
    XML_WRITE_COMMENT(writer, "type_of_identity = GUTI");
    XML_WRITE_FORMAT_ELEMENT(writer, TYPE_OF_IDENTITY_ATTR_XML_STR, "0x%x", EPS_MOBILE_IDENTITY_GUTI);
    XML_WRITE_FORMAT_ELEMENT(writer, MOBILE_COUNTRY_CODE_ATTR_XML_STR, "%x%x%x",
        guti->mcc_digit1, guti->mcc_digit2, guti->mcc_digit3);
    XML_WRITE_FORMAT_ELEMENT(writer, MOBILE_NETWORK_CODE_ATTR_XML_STR, "%x%x%x",
        guti->mnc_digit1, guti->mnc_digit2, guti->mnc_digit3);
    mme_gid_to_xml(&guti->mme_group_id, writer);
    mme_code_to_xml(&guti->mme_code, writer);
    m_tmsi_to_xml(&guti->m_tmsi, writer);

  } else if (epsmobileidentity->imei.typeofidentity == EPS_MOBILE_IDENTITY_IMEI) {
    imei_eps_mobile_identity_t                *imei = &epsmobileidentity->imei;

    XML_WRITE_FORMAT_ELEMENT(writer, ODDEVEN_ATTR_XML_STR, "0x%x", imei->oddeven);
    XML_WRITE_COMMENT(writer, "type_of_identity = IMEI");
    XML_WRITE_FORMAT_ELEMENT(writer, TYPE_OF_IDENTITY_ATTR_XML_STR, "0x%x", MOBILE_IDENTITY_IMEI);
    XML_WRITE_FORMAT_ELEMENT(writer, TAC_ATTR_XML_STR, "%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x",
        imei->identity_digit1, imei->identity_digit2, imei->identity_digit3, imei->identity_digit4,
        imei->identity_digit5, imei->identity_digit6, imei->identity_digit7, imei->identity_digit8,
        imei->identity_digit9, imei->identity_digit10, imei->identity_digit11, imei->identity_digit12,
        imei->identity_digit13, imei->identity_digit14, imei->identity_digit15);

  } else {
    XML_WRITE_FORMAT_COMMENT(writer, "ERROR: Wrong type of EPS mobile identity (%u)\n", epsmobileidentity->guti.typeofidentity);
  }
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( eps_network_feature_support,      EPS_NETWORK_FEATURE_SUPPORT);
//------------------------------------------------------------------------------
void eps_network_feature_support_to_xml (eps_network_feature_support_t * epsnetworkfeaturesupport, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, EPS_NETWORK_FEATURE_SUPPORT_XML_STR, "0x%"PRIx8, *epsnetworkfeaturesupport);
}

NUM_FROM_XML_GENERATE( eps_update_result,                EPS_UPDATE_RESULT);

//------------------------------------------------------------------------------
void eps_update_result_to_xml (eps_update_result_t * epsupdateresult, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, EPS_UPDATE_RESULT_XML_STR, "0x%"PRIx8, *epsupdateresult);
}
//------------------------------------------------------------------------------
void eps_update_type_to_xml (EpsUpdateType * epsupdatetype, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, EPS_UPDATE_TYPE_XML_STR);
  XML_WRITE_FORMAT_ATTRIBUTE(writer, ACTIVE_FLAG_ATTR_XML_STR, "0x%x", epsupdatetype->active_flag);
  XML_WRITE_FORMAT_ATTRIBUTE(writer, EPS_UPDATE_TYPE_VALUE_ATTR_XML_STR, "0x%x", epsupdatetype->eps_update_type_value);
  XML_WRITE_END_ELEMENT(writer);
}
//------------------------------------------------------------------------------
bool esm_message_container_from_xml (
    xmlDocPtr                              xml_doc,
    xmlXPathContextPtr                     xpath_ctx,
    EsmMessageContainer            *esmmessagecontainer)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr_mi = bformat("./%s",ESM_MESSAGE_CONTAINER_XML_STR);
  xmlXPathObjectPtr xpath_obj_mi = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_mi);
  if (xpath_obj_mi) {
    xmlNodeSetPtr nodes_mi = xpath_obj_mi->nodesetval;
    int size = (nodes_mi) ? nodes_mi->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_mi->nodeTab[0], xpath_ctx));
      if (res) {
        ESM_msg esm;
        memset(&esm, 0, sizeof(esm));
        // Do not load attribute ESM_MESSAGE_CONTAINER_HEX_DUMP_ATTR_XML_STR now. May be used for check ? ...no
        res = esm_msg_from_xml (xml_doc, xpath_ctx, &esm);
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
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_mi);
  }
  bdestroy_wrapper (&xpath_expr_mi);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void esm_message_container_to_xml (EsmMessageContainer esmmessagecontainer, xmlTextWriterPtr writer)
{
  if (esmmessagecontainer) {
    XML_WRITE_START_ELEMENT(writer, ESM_MESSAGE_CONTAINER_XML_STR);
    XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", blength(esmmessagecontainer));
    XML_WRITE_HEX_ATTRIBUTE(writer, ESM_MESSAGE_CONTAINER_HEX_DUMP_ATTR_XML_STR, bdata(esmmessagecontainer), blength(esmmessagecontainer));
    ESM_msg esm;
    memset(&esm, 0, sizeof(esm));
    esm_msg_to_xml (&esm, (uint8_t *) bdata(esmmessagecontainer), blength(esmmessagecontainer), writer);

    XML_WRITE_END_ELEMENT(writer);
  }
}
//------------------------------------------------------------------------------
void ksi_and_sequence_number_to_xml (KsiAndSequenceNumber * ksiandsequencenumber, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, KSI_AND_SEQUENCE_NUMBER_XML_STR);
  XML_WRITE_FORMAT_ATTRIBUTE(writer, KSI_ATTR_XML_STR, "0x%x", ksiandsequencenumber->ksi);
  XML_WRITE_FORMAT_ATTRIBUTE(writer, SEQUENCE_NUMBER_ATTR_XML_STR, "0x%x", ksiandsequencenumber->sequencenumber);
  XML_WRITE_END_ELEMENT(writer);
}
//------------------------------------------------------------------------------
bool nas_key_set_identifier_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, NasKeySetIdentifier * const naskeysetidentifier)
{
  bool res = true;
  bstring xpath_expr_ksi = bformat("./%s",NAS_KEY_SET_IDENTIFIER_XML_STR);
  xmlXPathObjectPtr xpath_obj_ksi = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_ksi);
  if (xpath_obj_ksi) {
    xmlNodeSetPtr nodes = xpath_obj_ksi->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {
      //xmlNodePtr saved_node_ptr = xpath_ctx->node;
      //res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));
      if (res) {
        xmlChar *attr = xmlGetProp(nodes->nodeTab[0], (const xmlChar *)TSC_ATTR_XML_STR);
        if (attr) {
          OAILOG_TRACE (LOG_XML, "Found %s=%s\n", TSC_ATTR_XML_STR, attr);
          uint8_t  tsc = 0;
          sscanf((const char*)attr, "%"SCNx8, &tsc);
          naskeysetidentifier->tsc = tsc;
          xmlFree(attr);
        } else {
          res = false;
        }
      }
      if (res) {
        xmlChar *attr = xmlGetProp(nodes->nodeTab[0], (const xmlChar *)NAS_KEY_SET_IDENTIFIER_ATTR_XML_STR);
        if (attr) {
          OAILOG_TRACE (LOG_XML, "Found %s=%s\n", NAS_KEY_SET_IDENTIFIER_ATTR_XML_STR, attr);
          uint8_t  naskeysetidentifier_a = 0;
          sscanf((const char*)attr, "%"SCNx8, &naskeysetidentifier_a);
          naskeysetidentifier->naskeysetidentifier = naskeysetidentifier_a;
          xmlFree(attr);
        } else {
          res = false;
        }
      }
      //res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_ksi);
  }
  bdestroy_wrapper (&xpath_expr_ksi);
  return res;
}
//------------------------------------------------------------------------------
void nas_key_set_identifier_to_xml (NasKeySetIdentifier * naskeysetidentifier, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, NAS_KEY_SET_IDENTIFIER_XML_STR);
  XML_WRITE_FORMAT_ATTRIBUTE(writer, TSC_ATTR_XML_STR, "0x%"PRIx8, naskeysetidentifier->tsc);
  XML_WRITE_FORMAT_ATTRIBUTE(writer, NAS_KEY_SET_IDENTIFIER_ATTR_XML_STR, "0x%"PRIx8, naskeysetidentifier->naskeysetidentifier);
  XML_WRITE_END_ELEMENT(writer);
}
//------------------------------------------------------------------------------
bool nas_message_container_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    NasMessageContainer   * const nasmessagecontainer)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;

  bstring xpath_expr = bformat("./%s",NAS_MESSAGE_CONTAINER_XML_STR);
  res = xml_load_hex_stream_leaf_tag(xml_doc,xpath_ctx, xpath_expr, nasmessagecontainer);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void nas_message_container_to_xml (NasMessageContainer nasmessagecontainer, xmlTextWriterPtr writer)
{
  XML_WRITE_HEX_ELEMENT(writer, NAS_MESSAGE_CONTAINER_XML_STR, bdata(nasmessagecontainer), blength(nasmessagecontainer));
}

//------------------------------------------------------------------------------
bool nas_security_algorithms_from_xml (
    xmlDocPtr                              xml_doc,
    xmlXPathContextPtr                     xpath_ctx,
    NasSecurityAlgorithms          * const nassecurityalgorithms)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr_nsa = bformat("./%s",NAS_SECURITY_ALGORITHMS_XML_STR);
  xmlXPathObjectPtr xpath_obj_nsa = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_nsa);
  if (xpath_obj_nsa) {
    xmlNodeSetPtr nodes_nsa = xpath_obj_nsa->nodesetval;
    int size = (nodes_nsa) ? nodes_nsa->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_nsa->nodeTab[0], xpath_ctx));
      if (res) {
        uint8_t  typeofcipheringalgorithm = 0;
        bstring xpath_expr = bformat("./%s",TYPE_OF_CYPHERING_ALGORITHM_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&typeofcipheringalgorithm, NULL);
        nassecurityalgorithms->typeofcipheringalgorithm = typeofcipheringalgorithm;
        bdestroy_wrapper (&xpath_expr);
      }
      if (res) {
        uint8_t  typeofintegrityalgorithm = 0;
        bstring xpath_expr = bformat("./%s",TYPE_OF_INTEGRITY_PROTECTION_ALGORITHM_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&typeofintegrityalgorithm, NULL);
        nassecurityalgorithms->typeofintegrityalgorithm = typeofintegrityalgorithm;
        bdestroy_wrapper (&xpath_expr);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_nsa);
  }
  bdestroy_wrapper (&xpath_expr_nsa);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
void nas_security_algorithms_to_xml (NasSecurityAlgorithms * nassecurityalgorithms, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, NAS_SECURITY_ALGORITHMS_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, TYPE_OF_CYPHERING_ALGORITHM_ATTR_XML_STR, "0x%"PRIx8, nassecurityalgorithms->typeofcipheringalgorithm);
  XML_WRITE_FORMAT_ELEMENT(writer, TYPE_OF_INTEGRITY_PROTECTION_ALGORITHM_ATTR_XML_STR, "0x%"PRIx8, nassecurityalgorithms->typeofintegrityalgorithm);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool nonce_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, const char * const ie, nonce_t * const nonce)
{
  OAILOG_FUNC_IN (LOG_XML);

  bstring xpath_expr = bformat("./%s",ie);
  bool res =  xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx32, (void*)nonce, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
void nonce_to_xml (const char * const ie, nonce_t * nonce, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, ie, "0x%"PRIx32, *nonce);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( paging_identity, PAGING_IDENTITY);
//------------------------------------------------------------------------------
void paging_identity_to_xml (paging_identity_t * pagingidentity, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, PAGING_IDENTITY_XML_STR, "0x%x", *pagingidentity);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( service_type, SERVICE_TYPE);
//------------------------------------------------------------------------------
void service_type_to_xml (service_type_t * servicetype, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, SERVICE_TYPE_XML_STR, "0x%"PRIx8, *servicetype);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( short_mac, SHORT_MAC);
//------------------------------------------------------------------------------
void short_mac_to_xml (short_mac_t * shortmac, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, SHORT_MAC_XML_STR, SHORT_MAC_FMT, *shortmac);
}

//------------------------------------------------------------------------------
bool tracking_area_identity_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    tai_t * const tai)
{
  OAILOG_FUNC_IN (LOG_XML);
  clear_tai(tai);
  bool res = false;
  bstring xpath_expr_tai = bformat("./%s",TRACKING_AREA_IDENTITY_XML_STR);
  xmlXPathObjectPtr xpath_obj_tai = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_tai);
  if (xpath_obj_tai) {
    xmlNodeSetPtr nodes_tai = xpath_obj_tai->nodesetval;
    int size = (nodes_tai) ? nodes_tai->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_tai->nodeTab[0], xpath_ctx));

      if (res) {
        res = false;
        bstring xpath_expr = bformat("./%s",MOBILE_COUNTRY_CODE_ATTR_XML_STR);
        xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
        if (xpath_obj) {
          xmlNodeSetPtr nodes = xpath_obj->nodesetval;
          size = (nodes) ? nodes->nodeNr : 0;
          if ((1 == size) && (xml_doc)) {
            xmlChar *key = xmlNodeListGetString(xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
            uint8_t mcc_digit[3] = {0};
            int ret = sscanf((const char*)key, "%1"SCNx8"%1"SCNx8"%1"SCNx8, &mcc_digit[0], &mcc_digit[1], &mcc_digit[2]);
            xmlFree(key);
            if (3 == ret) {
              tai->mcc_digit1 = mcc_digit[0];
              tai->mcc_digit2 = mcc_digit[1];
              tai->mcc_digit3 = mcc_digit[2];
              OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s/%s = %x%x%x\n",
                  TRACKING_AREA_IDENTITY_XML_STR, MOBILE_COUNTRY_CODE_ATTR_XML_STR,
                  tai->mcc_digit1, tai->mcc_digit2, tai->mcc_digit3);
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
        xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
        if (xpath_obj) {
          xmlNodeSetPtr nodes = xpath_obj->nodesetval;
          int size = (nodes) ? nodes->nodeNr : 0;
          if ((1 == size) && (xml_doc)) {
            xmlChar *key = xmlNodeListGetString(xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
            uint8_t mnc_digit[3] = {0};
            int ret = sscanf((const char*)key, "%1"SCNx8"%1"SCNx8"%1"SCNx8, &mnc_digit[0], &mnc_digit[1], &mnc_digit[2]);
                xmlFree(key);
            if (3 == ret) {
              tai->mnc_digit1 = mnc_digit[0];
              tai->mnc_digit2 = mnc_digit[1];
              tai->mnc_digit3 = mnc_digit[2];
              OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s/%s = %x%x%x\n",
                  TRACKING_AREA_IDENTITY_XML_STR, MOBILE_NETWORK_CODE_ATTR_XML_STR,
                  tai->mnc_digit1, tai->mnc_digit2, tai->mnc_digit3);
            }
            res = (3 == ret);
          }
          xmlXPathFreeObject(xpath_obj);
        }
        bdestroy_wrapper (&xpath_expr);
      }
      if (res) {
        // may be not needed
        res = tracking_area_code_from_xml(xml_doc, xpath_ctx, &tai->tac);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_tai);
  }
  bdestroy_wrapper (&xpath_expr_tai);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
bool tracking_area_code_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    tac_t                 * const tac)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",TRACKING_AREA_CODE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {
      xmlChar *key = xmlNodeListGetString(xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
      int ret = sscanf((const char*)key, "%"SCNx16, tac);
      xmlFree(key);
      if (1 == ret) {
        res = true;
        OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s = 0x%"PRIx16"\n",
            TRACKING_AREA_CODE_XML_STR, *tac);
      }
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);

  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void tracking_area_code_to_xml (const tac_t * const tac, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, TRACKING_AREA_CODE_XML_STR, "0x%"PRIx16, *tac);
}

//------------------------------------------------------------------------------
void tracking_area_identity_to_xml (const tai_t * const tai, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, TRACKING_AREA_IDENTITY_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, MOBILE_COUNTRY_CODE_ATTR_XML_STR, "%"PRIx8"%"PRIx8"%"PRIx8,
      tai->mcc_digit1, tai->mcc_digit2, tai->mcc_digit3);
  XML_WRITE_FORMAT_ELEMENT(writer, MOBILE_NETWORK_CODE_ATTR_XML_STR, "%"PRIx8"%"PRIx8"%"PRIx8,
      tai->mnc_digit1, tai->mnc_digit2, tai->mnc_digit3);
  tracking_area_code_to_xml (&tai->tac, writer);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool tracking_area_identity_list_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    tai_list_t            * const tai_list)
{
  OAILOG_FUNC_IN (LOG_XML);
  memset(tai_list, 0, sizeof(*tai_list));
  bool res = false;
  bstring xpath_expr_tai_list = bformat("./%s",TRACKING_AREA_IDENTITY_LIST_XML_STR);
  xmlXPathObjectPtr xpath_obj_tai_list = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_tai_list);
  if (xpath_obj_tai_list) {
    xmlNodeSetPtr nodes_tai_list = xpath_obj_tai_list->nodesetval;
    int size = (nodes_tai_list) ? nodes_tai_list->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_tai_list->nodeTab[0], xpath_ctx));

      if (res) {
        res = false;
        bstring xpath_expr_partial = bformat("./%s",PARTIAL_TRACKING_AREA_IDENTITY_LIST_XML_STR);
        xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_partial);
        if (xpath_obj) {
          res = true;
          xmlNodeSetPtr nodes = xpath_obj->nodesetval;
          size = (nodes) ? nodes->nodeNr : 0;
          if ((0 < size) && (xml_doc)) {
            xmlNodePtr saved_node_ptr2 = xpath_ctx->node;
            for (int p = 0; p < size; p++) {
              res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[p], xpath_ctx)) & res;

              if (res) {
                bstring xpath_expr = bformat("./%s",PARTIAL_TRACKING_AREA_IDENTITY_LIST_TYPE_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&tai_list->partial_tai_list[p].typeoflist, NULL);
                bdestroy_wrapper (&xpath_expr);
              }

              if (res) {
                bstring xpath_expr = bformat("./%s",PARTIAL_TRACKING_AREA_IDENTITY_LIST_NUM_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&tai_list->partial_tai_list[p].numberofelements, NULL);
                // LW: number of elements is coded as N-1 (0 -> 1 element, 1 -> 2 elements...), see 3GPP TS 24.301, section 9.9.3.33.1
                tai_list->partial_tai_list[p].numberofelements--;
                bdestroy_wrapper (&xpath_expr);
              }
              if (res) {
                switch (tai_list->partial_tai_list[p].typeoflist) {
                  case TRACKING_AREA_IDENTITY_LIST_ONE_PLMN_CONSECUTIVE_TACS:
                    res = tracking_area_identity_from_xml (xml_doc, xpath_ctx, &tai_list->partial_tai_list[p].u.tai_one_plmn_consecutive_tacs) & res;
                    break;
                  case TRACKING_AREA_IDENTITY_LIST_ONE_PLMN_NON_CONSECUTIVE_TACS:
                    {
                      bstring xpath_expr_tai = bformat("./%s",TRACKING_AREA_IDENTITY_XML_STR);
                      xmlXPathObjectPtr xpath_obj_tai = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_tai);
                      if (xpath_obj_tai) {
                        xmlNodeSetPtr nodes_tai = xpath_obj_tai->nodesetval;
                        int size = (nodes_tai) ? nodes_tai->nodeNr : 0;
                        if ((1 == size) && (xml_doc)) {

                          xmlNodePtr saved_node_ptr = xpath_ctx->node;
                          res = (RETURNok == xmlXPathSetContextNode(nodes_tai->nodeTab[0], xpath_ctx));

                          if (res) {
                            res = false;
                            bstring xpath_expr = bformat("./%s",MOBILE_COUNTRY_CODE_ATTR_XML_STR);
                            xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
                            if (xpath_obj) {
                              xmlNodeSetPtr nodes = xpath_obj->nodesetval;
                              size = (nodes) ? nodes->nodeNr : 0;
                              if ((1 == size) && (xml_doc)) {
                                xmlChar *key = xmlNodeListGetString(xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
                                uint8_t mcc_digit[3] = {0};
                                int ret = sscanf((const char*)key, "%1"SCNx8"%1"SCNx8"%1"SCNx8, &mcc_digit[0], &mcc_digit[1], &mcc_digit[2]);
                                xmlFree(key);
                                if (3 == ret) {
                                  tai_list->partial_tai_list[p].u.tai_one_plmn_non_consecutive_tacs.mcc_digit1 = mcc_digit[0];
                                  tai_list->partial_tai_list[p].u.tai_one_plmn_non_consecutive_tacs.mcc_digit2 = mcc_digit[1];
                                  tai_list->partial_tai_list[p].u.tai_one_plmn_non_consecutive_tacs.mcc_digit3 = mcc_digit[2];
                                  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s/%s = %x%x%x\n",
                                      TRACKING_AREA_IDENTITY_XML_STR, MOBILE_COUNTRY_CODE_ATTR_XML_STR,
                                      tai_list->partial_tai_list[p].u.tai_one_plmn_non_consecutive_tacs.mcc_digit1,
                                      tai_list->partial_tai_list[p].u.tai_one_plmn_non_consecutive_tacs.mcc_digit2,
                                      tai_list->partial_tai_list[p].u.tai_one_plmn_non_consecutive_tacs.mcc_digit3);
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
                            xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
                            if (xpath_obj) {
                              xmlNodeSetPtr nodes = xpath_obj->nodesetval;
                              int size = (nodes) ? nodes->nodeNr : 0;
                              if ((1 == size) && (xml_doc)) {
                                xmlChar *key = xmlNodeListGetString(xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
                                uint8_t mnc_digit[3] = {0};
                                int ret = sscanf((const char*)key, "%1"SCNx8"%1"SCNx8"%1"SCNx8, &mnc_digit[0], &mnc_digit[1], &mnc_digit[2]);
                                    xmlFree(key);
                                if (3 == ret) {
                                  tai_list->partial_tai_list[p].u.tai_one_plmn_non_consecutive_tacs.mnc_digit1 = mnc_digit[0];
                                  tai_list->partial_tai_list[p].u.tai_one_plmn_non_consecutive_tacs.mnc_digit2 = mnc_digit[1];
                                  tai_list->partial_tai_list[p].u.tai_one_plmn_non_consecutive_tacs.mnc_digit3 = mnc_digit[2];
                                  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s/%s = %x%x%x\n",
                                      TRACKING_AREA_IDENTITY_XML_STR, MOBILE_NETWORK_CODE_ATTR_XML_STR,
                                      tai_list->partial_tai_list[p].u.tai_one_plmn_non_consecutive_tacs.mnc_digit1,
                                      tai_list->partial_tai_list[p].u.tai_one_plmn_non_consecutive_tacs.mnc_digit2,
                                      tai_list->partial_tai_list[p].u.tai_one_plmn_non_consecutive_tacs.mnc_digit3);
                                }
                                res = (3 == ret);
                              }
                              xmlXPathFreeObject(xpath_obj);
                            }
                            bdestroy_wrapper (&xpath_expr);
                          }
                          if (res) {
                            // may be not needed
                            for (int t = 0; t < tai_list->partial_tai_list[p].numberofelements +1; t++) {
                              res = tracking_area_code_from_xml(xml_doc, xpath_ctx, &tai_list->partial_tai_list[p].u.tai_one_plmn_non_consecutive_tacs.tac[t]) & res;
                            }
                          }
                          res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
                        }
                      }
                    }
                    break;
                  case TRACKING_AREA_IDENTITY_LIST_MANY_PLMNS:
                    for (int t = 0; t < tai_list->partial_tai_list[p].numberofelements +1; t++) {
                      res = tracking_area_identity_from_xml (xml_doc, xpath_ctx, &tai_list->partial_tai_list[p].u.tai_many_plmn[t]) & res;
                    }
                    break;
                  default:
                    OAILOG_ERROR (LOG_XML, "Type of TAI list not handled %d", tai_list->partial_tai_list[p].typeoflist);
                    res = false;
                }
              }
              res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr2, xpath_ctx)) & res;
            }
          }
          xmlXPathFreeObject(xpath_obj);
        }
        bdestroy_wrapper (&xpath_expr_partial);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_tai_list);
  }
  bdestroy_wrapper (&xpath_expr_tai_list);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
void tracking_area_identity_list_to_xml (tai_list_t * trackingareaidentitylist, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, TRACKING_AREA_IDENTITY_LIST_XML_STR);
  for (int i = 0; i < trackingareaidentitylist->numberoflists; i++) {
    if (TRACKING_AREA_IDENTITY_LIST_ONE_PLMN_CONSECUTIVE_TACS == trackingareaidentitylist->partial_tai_list[i].typeoflist) {
      XML_WRITE_START_ELEMENT(writer, PARTIAL_TRACKING_AREA_IDENTITY_LIST_XML_STR);
      XML_WRITE_FORMAT_ELEMENT(writer, PARTIAL_TRACKING_AREA_IDENTITY_LIST_TYPE_XML_STR, "0x%"PRIx8, trackingareaidentitylist->partial_tai_list[i].typeoflist);
      // LW: number of elements is coded as N-1 (0 -> 1 element, 1 -> 2 elements...), see 3GPP TS 24.301, section 9.9.3.33.1
      XML_WRITE_FORMAT_ELEMENT(writer, PARTIAL_TRACKING_AREA_IDENTITY_LIST_NUM_XML_STR, "%"PRIu8, trackingareaidentitylist->partial_tai_list[i].numberofelements + 1);
      tracking_area_identity_to_xml(&trackingareaidentitylist->partial_tai_list[i].u.tai_one_plmn_consecutive_tacs, writer);
      XML_WRITE_END_ELEMENT(writer);
    } else  if (TRACKING_AREA_IDENTITY_LIST_ONE_PLMN_NON_CONSECUTIVE_TACS == trackingareaidentitylist->partial_tai_list[i].typeoflist) {
      XML_WRITE_START_ELEMENT(writer, PARTIAL_TRACKING_AREA_IDENTITY_LIST_XML_STR);
      XML_WRITE_FORMAT_ELEMENT(writer, PARTIAL_TRACKING_AREA_IDENTITY_LIST_TYPE_XML_STR, "0x%"PRIx8, trackingareaidentitylist->partial_tai_list[i].typeoflist);
      // LW: number of elements is coded as N-1 (0 -> 1 element, 1 -> 2 elements...), see 3GPP TS 24.301, section 9.9.3.33.1
      XML_WRITE_FORMAT_ELEMENT(writer, PARTIAL_TRACKING_AREA_IDENTITY_LIST_NUM_XML_STR, "%"PRIu8, trackingareaidentitylist->partial_tai_list[i].numberofelements + 1);

      XML_WRITE_START_ELEMENT(writer, TRACKING_AREA_IDENTITY_XML_STR);
      XML_WRITE_FORMAT_ELEMENT(writer, MOBILE_COUNTRY_CODE_ATTR_XML_STR, "%"PRIx8"%"PRIx8"%"PRIx8,
          trackingareaidentitylist->partial_tai_list[i].u.tai_one_plmn_non_consecutive_tacs.mcc_digit1,
          trackingareaidentitylist->partial_tai_list[i].u.tai_one_plmn_non_consecutive_tacs.mcc_digit2,
          trackingareaidentitylist->partial_tai_list[i].u.tai_one_plmn_non_consecutive_tacs.mcc_digit3);
      XML_WRITE_FORMAT_ELEMENT(writer, MOBILE_NETWORK_CODE_ATTR_XML_STR, "%"PRIx8"%"PRIx8"%"PRIx8,
          trackingareaidentitylist->partial_tai_list[i].u.tai_one_plmn_non_consecutive_tacs.mnc_digit1,
          trackingareaidentitylist->partial_tai_list[i].u.tai_one_plmn_non_consecutive_tacs.mnc_digit2,
          trackingareaidentitylist->partial_tai_list[i].u.tai_one_plmn_non_consecutive_tacs.mnc_digit3);
      for (int j = 0; j < trackingareaidentitylist->partial_tai_list[i].numberofelements + 1; j++) {
        tracking_area_code_to_xml (&trackingareaidentitylist->partial_tai_list[i].u.tai_one_plmn_non_consecutive_tacs.tac[j] , writer);
      }
      XML_WRITE_END_ELEMENT(writer);

      XML_WRITE_END_ELEMENT(writer);
    } else if (TRACKING_AREA_IDENTITY_LIST_MANY_PLMNS == trackingareaidentitylist->partial_tai_list[i].typeoflist) {
      XML_WRITE_START_ELEMENT(writer, PARTIAL_TRACKING_AREA_IDENTITY_LIST_XML_STR);
      XML_WRITE_FORMAT_ELEMENT(writer, PARTIAL_TRACKING_AREA_IDENTITY_LIST_TYPE_XML_STR, "0x%"PRIx8, trackingareaidentitylist->partial_tai_list[i].typeoflist);
      // LW: number of elements is coded as N-1 (0 -> 1 element, 1 -> 2 elements...), see 3GPP TS 24.301, section 9.9.3.33.1
      XML_WRITE_FORMAT_ELEMENT(writer, PARTIAL_TRACKING_AREA_IDENTITY_LIST_NUM_XML_STR, "%"PRIu8, trackingareaidentitylist->partial_tai_list[i].numberofelements + 1);

      for (int j = 0; j < trackingareaidentitylist->partial_tai_list[i].numberofelements +1; j++) {
        tracking_area_identity_to_xml (&trackingareaidentitylist->partial_tai_list[i].u.tai_many_plmn[j] , writer);
      }

      XML_WRITE_END_ELEMENT(writer);
    } else {
      XML_WRITE_FORMAT_COMMENT(writer,"Type of TAI list not handled %d", trackingareaidentitylist->partial_tai_list[i].typeoflist);
      OAILOG_ERROR (LOG_XML, "Type of TAI list not handled %d", trackingareaidentitylist->partial_tai_list[i].typeoflist);
    }
  }
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool ue_network_capability_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, ue_network_capability_t * const uenetworkcapability)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr_nc = bformat("./%s",UE_NETWORK_CAPABILITY_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_nc);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));
      if (res) {
        uint8_t  eea = 0;
        bstring xpath_expr = bformat("./%s",UE_NETWORK_CAPABILITY_EPS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&eea, NULL);
        bdestroy_wrapper (&xpath_expr);
        uenetworkcapability->eea = eea;
      }
      if (res) {
        uint8_t  eia = 0;
        bstring xpath_expr = bformat("./%s",UE_NETWORK_CAPABILITY_EPS_INTEGRITY_ALGORITHMS_SUPPORTED_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&eia, NULL);
        bdestroy_wrapper (&xpath_expr);
        uenetworkcapability->eia = eia;
      }
      if (res) {
        uint8_t  uea = 0;
        bstring xpath_expr = bformat("./%s",UE_NETWORK_CAPABILITY_UMTS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&uea, NULL);
        bdestroy_wrapper (&xpath_expr);
        uenetworkcapability->uea = uea;
        if (res) {
          uenetworkcapability->umts_present = true;
          uint8_t  uia = 0;
          bstring xpath_expr = bformat("./%s",UE_NETWORK_CAPABILITY_UMTS_INTEGRITY_ALGORITHMS_SUPPORTED_ATTR_XML_STR);
          res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&uia, NULL);
          bdestroy_wrapper (&xpath_expr);
          uenetworkcapability->uia = uia;

          if (res) {
            uint8_t  ucs2 = 0;
            bstring xpath_expr = bformat("./%s",UE_NETWORK_CAPABILITY_UCS2_SUPPORT_ATTR_XML_STR);
            res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&ucs2, NULL);
            bdestroy_wrapper (&xpath_expr);
            uenetworkcapability->ucs2 = ucs2;
          }
        }
        res = true; // do not report that ue_network_capability_from_xml failed at all
      }

      if (res) {
        uint8_t  nf = 0;
        bstring xpath_expr = bformat("./%s",UE_NETWORK_CAPABILITY_NF_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&nf, NULL);
        bdestroy_wrapper (&xpath_expr);
        uenetworkcapability->nf = nf;
        if (res) {
          uenetworkcapability->misc_present = true;
          uint8_t  spare = 0;
          bstring xpath_expr = bformat("./%s",UE_NETWORK_CAPABILITY_SPARE_ATTR_XML_STR);
          res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&spare, NULL);
          bdestroy_wrapper (&xpath_expr);
          uenetworkcapability->uia = spare;

          if (res) {
            uint8_t  srvcc = 0;
            bstring xpath_expr = bformat("./%s",UE_NETWORK_CAPABILITY_1xSRVCC_ATTR_XML_STR);
            res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&srvcc, NULL);
            bdestroy_wrapper (&xpath_expr);
            uenetworkcapability->srvcc = srvcc;
          }
          if (res) {
            uint8_t  lcs = 0;
            bstring xpath_expr = bformat("./%s",UE_NETWORK_CAPABILITY_LOCATION_SERVICES_ATTR_XML_STR);
            res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&lcs, NULL);
            bdestroy_wrapper (&xpath_expr);
            uenetworkcapability->lcs = lcs;
          }
          if (res) {
            uint8_t  lpp = 0;
            bstring xpath_expr = bformat("./%s",UE_NETWORK_CAPABILITY_LTE_POSITIONING_PROTOCOL_ATTR_XML_STR);
            res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&lpp, NULL);
            bdestroy_wrapper (&xpath_expr);
            uenetworkcapability->lpp = lpp;
          }
          if (res) {
            uint8_t  csfb = 0;
            bstring xpath_expr = bformat("./%s",UE_NETWORK_CAPABILITY_ACCESS_CLASS_CONTROL_FOR_CSFB_ATTR_XML_STR);
            res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&csfb, NULL);
            bdestroy_wrapper (&xpath_expr);
            uenetworkcapability->csfb = csfb;
          }
        }
        res = true; // do not report that ue_network_capability_from_xml failed at all
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr_nc);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
void ue_network_capability_to_xml (ue_network_capability_t * uenetworkcapability, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, UE_NETWORK_CAPABILITY_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, UE_NETWORK_CAPABILITY_EPS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR, "0x%"PRIx8, uenetworkcapability->eea);
  XML_WRITE_FORMAT_ELEMENT(writer, UE_NETWORK_CAPABILITY_EPS_INTEGRITY_ALGORITHMS_SUPPORTED_ATTR_XML_STR, "0x%"PRIx8, uenetworkcapability->eia);

  if (uenetworkcapability->umts_present) {
    XML_WRITE_FORMAT_ELEMENT(writer, UE_NETWORK_CAPABILITY_UMTS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR, "0x%"PRIx8,uenetworkcapability->uea);
    XML_WRITE_FORMAT_ELEMENT(writer, UE_NETWORK_CAPABILITY_UMTS_INTEGRITY_ALGORITHMS_SUPPORTED_ATTR_XML_STR, "0x%"PRIx8, uenetworkcapability->uia);
    XML_WRITE_FORMAT_ELEMENT(writer, UE_NETWORK_CAPABILITY_UCS2_SUPPORT_ATTR_XML_STR, "0x%"PRIx8, uenetworkcapability->ucs2);
  }

  if (uenetworkcapability->misc_present) {
    XML_WRITE_FORMAT_ELEMENT(writer, UE_NETWORK_CAPABILITY_SPARE_ATTR_XML_STR, "0x%"PRIx8, uenetworkcapability->spare);
    XML_WRITE_FORMAT_ELEMENT(writer, UE_NETWORK_CAPABILITY_NF_ATTR_XML_STR, "0x%"PRIx8, uenetworkcapability->nf);
    XML_WRITE_FORMAT_ELEMENT(writer, UE_NETWORK_CAPABILITY_1xSRVCC_ATTR_XML_STR, "0x%"PRIx8, uenetworkcapability->srvcc);
    XML_WRITE_FORMAT_ELEMENT(writer, UE_NETWORK_CAPABILITY_LOCATION_SERVICES_ATTR_XML_STR, "0x%"PRIx8, uenetworkcapability->lcs);
    XML_WRITE_FORMAT_ELEMENT(writer, UE_NETWORK_CAPABILITY_LTE_POSITIONING_PROTOCOL_ATTR_XML_STR, "0x%"PRIx8, uenetworkcapability->lpp);
    XML_WRITE_FORMAT_ELEMENT(writer, UE_NETWORK_CAPABILITY_ACCESS_CLASS_CONTROL_FOR_CSFB_ATTR_XML_STR, "0x%"PRIx8, uenetworkcapability->csfb);
  }
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( ue_radio_capability_information_update_needed, UE_RADIO_CAPABILITY_INFORMATION_UPDATE_NEEDED);
//------------------------------------------------------------------------------
void ue_radio_capability_information_update_needed_to_xml (
  ue_radio_capability_information_update_needed_t * ueradiocapabilityinformationupdateneeded, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, UE_RADIO_CAPABILITY_INFORMATION_UPDATE_NEEDED_XML_STR, "0x%x", *ueradiocapabilityinformationupdateneeded);
}

//------------------------------------------------------------------------------
bool ue_security_capability_from_xml (
    xmlDocPtr                              xml_doc,
    xmlXPathContextPtr                     xpath_ctx,
    ue_security_capability_t           * const uesecuritycapability)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr_nsa = bformat("./%s",UE_SECURITY_CAPABILITY_XML_STR);
  xmlXPathObjectPtr xpath_obj_nsa = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_nsa);
  if (xpath_obj_nsa) {
    xmlNodeSetPtr nodes_nsa = xpath_obj_nsa->nodesetval;
    int size = (nodes_nsa) ? nodes_nsa->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_nsa->nodeTab[0], xpath_ctx));
      if (res) {
        uint8_t  eea = 0;
        bstring xpath_expr = bformat("./%s",UE_SECURITY_CAPABILITY_EPS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&eea, NULL);
        uesecuritycapability->eea = eea;
        bdestroy_wrapper (&xpath_expr);
      }
      if (res) {
        uint8_t  eia = 0;
        bstring xpath_expr = bformat("./%s",UE_SECURITY_CAPABILITY_EPS_INTEGRITY_ALGORITHMS_SUPPORTED_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&eia, NULL);
        uesecuritycapability->eia = eia;
        bdestroy_wrapper (&xpath_expr);
      }
      if (res) {
        uint8_t  uea = 0;
        bstring xpath_expr_uea = bformat("./%s",UE_SECURITY_CAPABILITY_UMTS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_uea, "%"SCNx8, (void*)&uea, NULL);
        bdestroy_wrapper (&xpath_expr_uea);
        if (res) {
          uesecuritycapability->eia = uea;
          uesecuritycapability->umts_present = true;

          uint8_t  uia = 0;
          bstring xpath_expr_uia = bformat("./%s",UE_SECURITY_CAPABILITY_UMTS_INTEGRITY_ALGORITHMS_SUPPORTED_ATTR_XML_STR);
          res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_uia, "%"SCNx8, (void*)&uia, NULL);
          uesecuritycapability->uia = uia;
          bdestroy_wrapper (&xpath_expr_uia);
        }
        uint8_t  gea = 0;
        bstring xpath_expr_gea = bformat("./%s",UE_SECURITY_CAPABILITY_GPRS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr_gea, "%"SCNx8, (void*)&gea, NULL);
        bdestroy_wrapper (&xpath_expr_gea);
        if (res) {
          uesecuritycapability->gea = gea;
          uesecuritycapability->gprs_present = true;
        }
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_nsa);
  }
  bdestroy_wrapper (&xpath_expr_nsa);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void ue_security_capability_to_xml (ue_security_capability_t * uesecuritycapability, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, UE_SECURITY_CAPABILITY_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, UE_SECURITY_CAPABILITY_EPS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR, "0x%"PRIx8, uesecuritycapability->eea);
  XML_WRITE_FORMAT_ELEMENT(writer, UE_SECURITY_CAPABILITY_EPS_INTEGRITY_ALGORITHMS_SUPPORTED_ATTR_XML_STR, "0x%"PRIx8, uesecuritycapability->eia);

  if (uesecuritycapability->umts_present) {
    XML_WRITE_FORMAT_ELEMENT(writer, UE_SECURITY_CAPABILITY_UMTS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR, "0x%"PRIx8,uesecuritycapability->uea);
    XML_WRITE_FORMAT_ELEMENT(writer, UE_SECURITY_CAPABILITY_UMTS_INTEGRITY_ALGORITHMS_SUPPORTED_ATTR_XML_STR, "0x%"PRIx8, uesecuritycapability->uia);
  }

  if (uesecuritycapability->gprs_present) {
    XML_WRITE_FORMAT_ELEMENT(writer, UE_SECURITY_CAPABILITY_GPRS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR, "0x%"PRIx8, uesecuritycapability->gea);
  }
  XML_WRITE_END_ELEMENT(writer);
}


//------------------------------------------------------------------------------
void cli_to_xml (Cli cli, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, CLI_XML_STR, "%s", bdata(cli));
}


//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( ss_code, SS_CODE);
//------------------------------------------------------------------------------
void ss_code_to_xml (ss_code_t * sscode, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, SS_CODE_XML_STR, "0x%x", *sscode);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( lcs_indicator, LCS_INDICATOR);
//------------------------------------------------------------------------------
void lcs_indicator_to_xml (lcs_indicator_t * lcsindicator, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, LCS_INDICATOR_XML_STR, "0x%x", *lcsindicator);
}

//------------------------------------------------------------------------------
void lcs_client_identity_to_xml (LcsClientIdentity  lcsclientidentity, xmlTextWriterPtr writer)
{
  XML_WRITE_HEX_ELEMENT(writer, LCS_CLIENT_IDENTITY_XML_STR, bdata(lcsclientidentity), blength(lcsclientidentity));
}

//------------------------------------------------------------------------------
bool guti_type_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, guti_type_t * const gutitype)
{
  bstring xpath_expr = bformat("./%s",GUTI_TYPE_XML_STR);
  char gutitype_str[64] = {0};
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)gutitype_str, NULL);
  if (!strcasecmp(gutitype_str, NATIVE_VAL_XML_STR)) {
    *gutitype = GUTI_NATIVE;
  } else if (!strcasecmp(gutitype_str, MAPPED_VAL_XML_STR)) {
    *gutitype = GUTI_MAPPED;
  }
  bdestroy_wrapper (&xpath_expr);
  return res;
}
//------------------------------------------------------------------------------
void guti_type_to_xml (guti_type_t * gutitype, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, GUTI_TYPE_XML_STR, "%s", (*gutitype == 0) ? NATIVE_VAL_XML_STR:MAPPED_VAL_XML_STR);
}

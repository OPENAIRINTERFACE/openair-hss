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
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "log.h"
#include "mme_scenario_player.h"
#include "3gpp_24.008_xml.h"
#include "xml_load.h"
#include "xml2_wrapper.h"


//******************************************************************************
// 10.5.1 Common information elements
//******************************************************************************
//------------------------------------------------------------------------------
// 10.5.1.2 Ciphering Key Sequence Number
//------------------------------------------------------------------------------
bool ciphering_key_sequence_number_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, ciphering_key_sequence_number_t * const cipheringkeysequencenumber)
{
  bstring xpath_expr = bformat("./%s",CIPHERING_KEY_SEQUENCE_NUMBER_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)cipheringkeysequencenumber, NULL);
  bdestroy_wrapper(&xpath_expr);
  return res;
}

//------------------------------------------------------------------------------
void ciphering_key_sequence_number_to_xml ( const ciphering_key_sequence_number_t * const cipheringkeysequencenumber, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, CIPHERING_KEY_SEQUENCE_NUMBER_XML_STR, "%"PRIu8, *cipheringkeysequencenumber);
}

//------------------------------------------------------------------------------
// 10.5.1.3 Location Area Identification
//------------------------------------------------------------------------------
bool location_area_identification_from_xml (
    xmlDocPtr                              xml_doc,
    xmlXPathContextPtr                     xpath_ctx,
    location_area_identification_t * const locationareaidentification)
{
  bool res = false;
  bstring xpath_expr_lai = bformat("./%s",LOCATION_AREA_IDENTIFICATION_XML_STR);
  xmlXPathObjectPtr xpath_obj_lai = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_lai);
  if (xpath_obj_lai) {
    xmlNodeSetPtr nodes_lai = xpath_obj_lai->nodesetval;
    int size = (nodes_lai) ? nodes_lai->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_lai->nodeTab[0], xpath_ctx));
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
              locationareaidentification->mccdigit1 = mcc_digit[0];
              locationareaidentification->mccdigit2 = mcc_digit[1];
              locationareaidentification->mccdigit3 = mcc_digit[2];
              OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s/%s = %x%x%x\n",
                  LOCATION_AREA_IDENTIFICATION_XML_STR, MOBILE_COUNTRY_CODE_ATTR_XML_STR,
                  locationareaidentification->mccdigit1,
                  locationareaidentification->mccdigit2,
                  locationareaidentification->mccdigit3);
            }
            res = (3 == ret);
          }
          xmlXPathFreeObject(xpath_obj);
        }
        bdestroy_wrapper(&xpath_expr);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
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
              locationareaidentification->mncdigit1 = mnc_digit[0];
              locationareaidentification->mncdigit2 = mnc_digit[1];
              locationareaidentification->mncdigit3 = mnc_digit[2];
              OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s/%s = %x%x%x\n",
                  LOCATION_AREA_IDENTIFICATION_XML_STR, MOBILE_NETWORK_CODE_ATTR_XML_STR,
                  locationareaidentification->mncdigit1,
                  locationareaidentification->mncdigit2,
                  locationareaidentification->mncdigit3);
            }
            res = (3 == ret);
          }
          xmlXPathFreeObject(xpath_obj);
        }
        bdestroy_wrapper (&xpath_expr);
      }
      if (res) {
        bstring xpath_expr = bformat("./%s",LOCATION_AREA_CODE_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&locationareaidentification->lac, NULL);
        bdestroy_wrapper(&xpath_expr);
      }
      //useless
      //if (res) {res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));}
    }
    xmlXPathFreeObject(xpath_obj_lai);
  }
  bdestroy_wrapper(&xpath_expr_lai);
  return res;
}

//------------------------------------------------------------------------------
void location_area_identification_to_xml (const location_area_identification_t * const locationareaidentification, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, LOCATION_AREA_IDENTIFICATION_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, MOBILE_COUNTRY_CODE_ATTR_XML_STR, "%x%x%x",
      locationareaidentification->mccdigit1, locationareaidentification->mccdigit2, locationareaidentification->mccdigit3);
  XML_WRITE_FORMAT_ELEMENT(writer, MOBILE_NETWORK_CODE_ATTR_XML_STR, "%x%x%x",
      locationareaidentification->mncdigit1, locationareaidentification->mncdigit2, locationareaidentification->mncdigit3);
  XML_WRITE_FORMAT_ELEMENT(writer, LOCATION_AREA_CODE_ATTR_XML_STR, "0x%"PRIx16, locationareaidentification->lac);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
// 10.5.1.4 Mobile Identity
//------------------------------------------------------------------------------
bool mobile_identity_from_xml (
    xmlDocPtr                              xml_doc,
    xmlXPathContextPtr                     xpath_ctx,
    mobile_identity_t              * const mobileidentity)
{
  bool res = false;
  bstring xpath_expr_mi = bformat("./%s",MOBILE_IDENTITY_XML_STR);
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
          case MOBILE_IDENTITY_IMSI: {
              mobileidentity->imsi.typeofidentity = MOBILE_IDENTITY_IMSI;

              uint8_t  oddeven = 0;
              bstring xpath_expr = bformat("./%s",ODDEVEN_ATTR_XML_STR);
              res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&oddeven, NULL);
              bdestroy_wrapper (&xpath_expr);
              mobileidentity->imsi.oddeven = oddeven;

              if (res) {
                char imsi_str[32] = {0}; // MOBILE_IDENTITY_IMSI_LENGTH*2+1
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
              }
            }
            break;

          case MOBILE_IDENTITY_IMEI: {
              mobileidentity->imei.typeofidentity = MOBILE_IDENTITY_IMEI;

              uint8_t  oddeven = 0;
              bstring xpath_expr = bformat("./%s",ODDEVEN_ATTR_XML_STR);
              res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&oddeven, NULL);
              bdestroy_wrapper (&xpath_expr);
              mobileidentity->imei.oddeven = oddeven;

              if (res) {
                char tac_str[32] = {0};//8+1
                bstring xpath_expr = bformat("./%s",TAC_ATTR_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)&tac_str, NULL);
                bdestroy_wrapper (&xpath_expr);
                if (res) {
                  uint8_t tac[8] = {0};
                  int ret = sscanf((const char*)tac_str,
                      "%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8,
                      &tac[0], &tac[1], &tac[2], &tac[3], &tac[4], &tac[5], &tac[6], &tac[7]);
                  if (8 == ret) {
                    mobileidentity->imei.tac1 = tac[0];
                    mobileidentity->imei.tac2 = tac[1];
                    mobileidentity->imei.tac3 = tac[2];
                    mobileidentity->imei.tac4 = tac[3];
                    mobileidentity->imei.tac5 = tac[4];
                    mobileidentity->imei.tac6 = tac[5];
                    mobileidentity->imei.tac7 = tac[6];
                    mobileidentity->imei.tac8 = tac[7];
                  }
                  res = (8 == ret);
                }
              }
              if (res) {
                char snr_str[32] = {0}; //6+1
                bstring xpath_expr = bformat("./%s",SNR_ATTR_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)&snr_str, NULL);
                bdestroy_wrapper (&xpath_expr);
                if (res) {
                  uint8_t snr[6] = {0};
                  int ret = sscanf((const char*)snr_str,
                      "%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8,
                      &snr[0], &snr[1], &snr[2], &snr[3], &snr[4], &snr[5]);
                  if (6 == ret) {
                    mobileidentity->imei.snr1 = snr[0];
                    mobileidentity->imei.snr2 = snr[1];
                    mobileidentity->imei.snr3 = snr[2];
                    mobileidentity->imei.snr4 = snr[3];
                    mobileidentity->imei.snr5 = snr[4];
                    mobileidentity->imei.snr6 = snr[5];
                  }
                  res = (6 == ret);
                }
              }

              uint8_t  cdsd = 0;
              xpath_expr = bformat("./%s",CDSD_ATTR_XML_STR);
              res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&cdsd, NULL);
              bdestroy_wrapper (&xpath_expr);
              mobileidentity->imei.cdsd = cdsd;
            }
            break;

          case MOBILE_IDENTITY_IMEISV: {
              mobileidentity->imeisv.typeofidentity = MOBILE_IDENTITY_IMEISV;

              uint8_t  oddeven = 0;
              bstring xpath_expr = bformat("./%s",ODDEVEN_ATTR_XML_STR);
              res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&oddeven, NULL);
              bdestroy_wrapper (&xpath_expr);
              mobileidentity->imeisv.oddeven = oddeven;

              if (res) {
                char tac_str[32] = {0};//8+1
                bstring xpath_expr = bformat("./%s",TAC_ATTR_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)&tac_str, NULL);
                bdestroy_wrapper (&xpath_expr);
                if (res) {
                  uint8_t tac[8] = {0};
                  int ret = sscanf((const char*)tac_str,
                      "%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8,
                      &tac[0], &tac[1], &tac[2], &tac[3], &tac[4], &tac[5], &tac[6], &tac[7]);
                  if (8 == ret) {
                    mobileidentity->imeisv.tac1 = tac[0];
                    mobileidentity->imeisv.tac2 = tac[1];
                    mobileidentity->imeisv.tac3 = tac[2];
                    mobileidentity->imeisv.tac4 = tac[3];
                    mobileidentity->imeisv.tac5 = tac[4];
                    mobileidentity->imeisv.tac6 = tac[5];
                    mobileidentity->imeisv.tac7 = tac[6];
                    mobileidentity->imeisv.tac8 = tac[7];
                  }
                  res = (8 == ret);
                }
              }
              if (res) {
                char snr_str[32] = {0}; //6+1
                bstring xpath_expr = bformat("./%s",SNR_ATTR_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)&snr_str, NULL);
                bdestroy_wrapper (&xpath_expr);
                if (res) {
                  uint8_t snr[6] = {0};
                  int ret = sscanf((const char*)snr_str,
                      "%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8"%1"SCNx8,
                      &snr[0], &snr[1], &snr[2], &snr[3], &snr[4], &snr[5]);
                  if (6 == ret) {
                    mobileidentity->imeisv.snr1 = snr[0];
                    mobileidentity->imeisv.snr2 = snr[1];
                    mobileidentity->imeisv.snr3 = snr[2];
                    mobileidentity->imeisv.snr4 = snr[3];
                    mobileidentity->imeisv.snr5 = snr[4];
                    mobileidentity->imeisv.snr6 = snr[5];
                  }
                  res = (6 == ret);
                }
              }
              if (res) {
                char svn_str[32] = {0}; //2+1
                bstring xpath_expr = bformat("./%s",SVN_ATTR_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)&svn_str, NULL);
                bdestroy_wrapper (&xpath_expr);
                if (res) {
                  uint8_t svn[2] = {0};
                  int ret = sscanf((const char*)svn_str,
                      "%1"SCNx8"%1"SCNx8, &svn[0], &svn[1]);
                  if (2 == ret) {
                    mobileidentity->imeisv.svn1 = svn[0];
                    mobileidentity->imeisv.svn2 = svn[1];
                  }
                  res = (2 == ret);
                }
              }
            }
            break;

          case MOBILE_IDENTITY_TMSI: {
              mobileidentity->tmsi.typeofidentity = MOBILE_IDENTITY_TMSI;

              uint8_t  oddeven = 0;
              bstring xpath_expr = bformat("./%s",ODDEVEN_ATTR_XML_STR);
              res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&oddeven, NULL);
              bdestroy_wrapper (&xpath_expr);
              mobileidentity->tmsi.oddeven = oddeven;

              if (res) {
                char tmsi_str[32] = {0}; //6+1
                bstring xpath_expr = bformat("./%s",TMSI_ATTR_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)&tmsi_str, NULL);
                bdestroy_wrapper (&xpath_expr);
                if (res) {
                  uint8_t tmsi[4] = {0};
                  int ret = sscanf((const char*)tmsi_str,
                      "%02"SCNx8"%02"SCNx8"%02"SCNx8"%02"SCNx8,
                      &tmsi[0], &tmsi[1], &tmsi[2], &tmsi[3]);
                  if (4 == ret) {
                    mobileidentity->tmsi.tmsi[0] = tmsi[0];
                    mobileidentity->tmsi.tmsi[1] = tmsi[1];
                    mobileidentity->tmsi.tmsi[2] = tmsi[2];
                    mobileidentity->tmsi.tmsi[3] = tmsi[3];
                  }
                  res = (4 == ret);
                }
              }

            }
            break;

          case MOBILE_IDENTITY_TMGI: {
              mobileidentity->tmgi.typeofidentity = MOBILE_IDENTITY_TMGI;

              if (res) {
                uint8_t  oddeven = 0;
                bstring xpath_expr = bformat("./%s",ODDEVEN_ATTR_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&oddeven, NULL);
                bdestroy_wrapper (&xpath_expr);
                mobileidentity->tmgi.oddeven = oddeven;
              }

              if (res) {
                uint8_t  mbmssessionidindication = 0;
                bstring xpath_expr = bformat("./%s",MBMS_SESSION_ID_INDIC_ATTR_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&mbmssessionidindication, NULL);
                bdestroy_wrapper (&xpath_expr);
                mobileidentity->tmgi.mbmssessionidindication = mbmssessionidindication;
              }

              if (res) {
                uint8_t  mccmncindication = 0;
                bstring xpath_expr = bformat("./%s",MCC_MNC_INDIC_ATTR_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&mccmncindication, NULL);
                bdestroy_wrapper (&xpath_expr);
                mobileidentity->tmgi.mccmncindication = mccmncindication;
              }

              if (res) {
                uint32_t  mbmsserviceid = 0;
                bstring xpath_expr = bformat("./%s",MCC_MNC_INDIC_ATTR_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx32, (void*)&mbmsserviceid, NULL);
                bdestroy_wrapper (&xpath_expr);
                mobileidentity->tmgi.mbmsserviceid = mbmsserviceid;
              }

              if (mobileidentity->tmgi.mccmncindication) {
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
                        mobileidentity->tmgi.mccdigit1 = mcc_digit[0];
                        mobileidentity->tmgi.mccdigit2 = mcc_digit[1];
                        mobileidentity->tmgi.mccdigit3 = mcc_digit[2];
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
                        mobileidentity->tmgi.mncdigit1 = mnc_digit[0];
                        mobileidentity->tmgi.mncdigit2 = mnc_digit[1];
                        mobileidentity->tmgi.mncdigit3 = mnc_digit[2];
                      }
                      res = (3 == ret);
                    }
                    xmlXPathFreeObject(xpath_obj);
                  }
                  bdestroy_wrapper (&xpath_expr);
                }
              }
              if (mobileidentity->tmgi.mbmssessionidindication) {
                if (res) {
                  uint8_t  mbmssessionid = 0;
                  bstring xpath_expr = bformat("./%s",MBMS_SESSION_ID_ATTR_XML_STR);
                  res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&mbmssessionid, NULL);
                  bdestroy_wrapper (&xpath_expr);
                  mobileidentity->tmgi.mbmssessionid = mbmssessionid;
                }
              }
            }
            break;

          case MOBILE_IDENTITY_NOT_AVAILABLE: {
              mobileidentity->no_id.typeofidentity = MOBILE_IDENTITY_NOT_AVAILABLE;

              uint8_t  oddeven = 0;
              bstring xpath_expr = bformat("./%s",ODDEVEN_ATTR_XML_STR);
              res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&oddeven, NULL);
              bdestroy_wrapper (&xpath_expr);
              mobileidentity->no_id.oddeven = oddeven;
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
  return res;
}

//------------------------------------------------------------------------------
void mobile_identity_to_xml (const mobile_identity_t * const mobileidentity, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, MOBILE_IDENTITY_XML_STR);

  if (mobileidentity->imsi.typeofidentity == MOBILE_IDENTITY_IMSI) {
    const imsi_mobile_identity_t                   * const imsi = &mobileidentity->imsi;
    XML_WRITE_FORMAT_ELEMENT(writer, ODDEVEN_ATTR_XML_STR, "0x%"PRIx8, imsi->oddeven);
    XML_WRITE_COMMENT(writer, "type_of_identity = IMSI");
    XML_WRITE_FORMAT_ELEMENT(writer, TYPE_OF_IDENTITY_ATTR_XML_STR, "0x%"PRIx8, MOBILE_IDENTITY_IMSI);
    XML_WRITE_FORMAT_ELEMENT(writer, IMSI_ATTR_XML_STR, "%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x",
        imsi->digit1,imsi->digit2,imsi->digit3,imsi->digit4,imsi->digit5,imsi->digit6,imsi->digit7,imsi->digit8,
        imsi->digit9,imsi->digit10,imsi->digit11,imsi->digit12,imsi->digit13,imsi->digit14,imsi->digit15);

  } else if (mobileidentity->imei.typeofidentity == MOBILE_IDENTITY_IMEI) {
    const imei_mobile_identity_t                   * const imei = &mobileidentity->imei;

    XML_WRITE_FORMAT_ELEMENT(writer, ODDEVEN_ATTR_XML_STR, "0x%"PRIx8, imei->oddeven);
    XML_WRITE_COMMENT(writer, "type_of_identity = IMEI");
    XML_WRITE_FORMAT_ELEMENT(writer, TYPE_OF_IDENTITY_ATTR_XML_STR, "0x%"PRIx8, MOBILE_IDENTITY_IMEI);
    XML_WRITE_FORMAT_ELEMENT(writer, TAC_ATTR_XML_STR, "%x%x%x%x%x%x%x%x",
        imei->tac1,imei->tac2,imei->tac3,imei->tac4,imei->tac5,imei->tac6,imei->tac7,imei->tac8);
    XML_WRITE_FORMAT_ELEMENT(writer, SNR_ATTR_XML_STR, "%x%x%x%x%x%x",
        imei->snr1,imei->snr2,imei->snr3,imei->snr4,imei->snr5,imei->snr6);
    XML_WRITE_FORMAT_ELEMENT(writer, CDSD_ATTR_XML_STR, "0x%"PRIx8, imei->cdsd);

  } else if (mobileidentity->imeisv.typeofidentity == MOBILE_IDENTITY_IMEISV) {
    const imeisv_mobile_identity_t                 * const imeisv = &mobileidentity->imeisv;

    XML_WRITE_FORMAT_ELEMENT(writer, ODDEVEN_ATTR_XML_STR, "0x%"PRIx8, imeisv->oddeven);
    XML_WRITE_COMMENT(writer, "type_of_identity = IMEISV");
    XML_WRITE_FORMAT_ELEMENT(writer, TYPE_OF_IDENTITY_ATTR_XML_STR, "0x%"PRIx8, MOBILE_IDENTITY_IMEISV);
    XML_WRITE_FORMAT_ELEMENT(writer, TAC_ATTR_XML_STR, "%x%x%x%x%x%x%x%x",
        imeisv->tac1,imeisv->tac2,imeisv->tac3,imeisv->tac4,imeisv->tac5,imeisv->tac6,imeisv->tac7,imeisv->tac8);
    XML_WRITE_FORMAT_ELEMENT(writer, SNR_ATTR_XML_STR, "%x%x%x%x%x%x",
        imeisv->snr1,imeisv->snr2,imeisv->snr3,imeisv->snr4,imeisv->snr5,imeisv->snr6);
    XML_WRITE_FORMAT_ELEMENT(writer, SVN_ATTR_XML_STR, "%x%x",
        imeisv->svn1,imeisv->svn2);

  } else if (mobileidentity->tmsi.typeofidentity == MOBILE_IDENTITY_TMSI) {
    const tmsi_mobile_identity_t                   * const tmsi = &mobileidentity->tmsi;

    XML_WRITE_FORMAT_ELEMENT(writer, ODDEVEN_ATTR_XML_STR, "0x%"PRIx8, tmsi->oddeven);
    XML_WRITE_COMMENT(writer, "type_of_identity = TMSI");
    XML_WRITE_FORMAT_ELEMENT(writer, TYPE_OF_IDENTITY_ATTR_XML_STR, "0x%"PRIx8, MOBILE_IDENTITY_TMSI);
    XML_WRITE_FORMAT_ELEMENT(writer, TMSI_ATTR_XML_STR, "%02X%02X%02X%02X",
        tmsi->tmsi[0],tmsi->tmsi[1],tmsi->tmsi[2],tmsi->tmsi[3]);

  } else if (mobileidentity->tmgi.typeofidentity == MOBILE_IDENTITY_TMGI) {
    const tmgi_mobile_identity_t                   * const tmgi = &mobileidentity->tmgi;

    XML_WRITE_FORMAT_ELEMENT(writer, MBMS_SESSION_ID_INDIC_ATTR_XML_STR, "0x%"PRIx8, tmgi->mbmssessionidindication);
    XML_WRITE_FORMAT_ELEMENT(writer, MCC_MNC_INDIC_ATTR_XML_STR, "0x%"PRIx8, tmgi->mccmncindication);
    XML_WRITE_FORMAT_ELEMENT(writer, ODDEVEN_ATTR_XML_STR, "0x%"PRIx8, tmgi->oddeven);
    XML_WRITE_COMMENT(writer, "type_of_identity = TMGI");
    XML_WRITE_FORMAT_ELEMENT(writer, TYPE_OF_IDENTITY_ATTR_XML_STR, "0x%"PRIx8, MOBILE_IDENTITY_TMGI);
    XML_WRITE_FORMAT_ELEMENT(writer, MBMS_SERVICE_ID_ATTR_XML_STR, "0x%"PRIx32, tmgi->mbmsserviceid);

    if (tmgi->mccmncindication) {
      XML_WRITE_FORMAT_ELEMENT(writer, MOBILE_COUNTRY_CODE_ATTR_XML_STR, "%x%x%x",
          tmgi->mccdigit1, tmgi->mccdigit2, tmgi->mccdigit3);
      XML_WRITE_FORMAT_ELEMENT(writer, MOBILE_NETWORK_CODE_ATTR_XML_STR, "%x%x%x",
          tmgi->mncdigit1, tmgi->mncdigit2, tmgi->mncdigit3);
    }
    if (tmgi->mbmssessionidindication) {
      XML_WRITE_FORMAT_ELEMENT(writer, MBMS_SESSION_ID_ATTR_XML_STR, "0x%"PRIx8, tmgi->mbmssessionid);
    }

  } else if (mobileidentity->tmgi.typeofidentity == MOBILE_IDENTITY_NOT_AVAILABLE) {
    const no_mobile_identity_t                   * const no = &mobileidentity->no_id;
    XML_WRITE_FORMAT_ELEMENT(writer, ODDEVEN_ATTR_XML_STR, "0x%"PRIx8, no->oddeven);
    XML_WRITE_COMMENT(writer, "type_of_identity = NO_IDENTITY");
    XML_WRITE_FORMAT_ELEMENT(writer, TYPE_OF_IDENTITY_ATTR_XML_STR, "0x%"PRIx8, MOBILE_IDENTITY_NOT_AVAILABLE);
  } else {
    AssertFatal (0,"Wrong type of mobile identity (%u)\n", mobileidentity->imsi.typeofidentity);
  }
  XML_WRITE_END_ELEMENT(writer);
}


//------------------------------------------------------------------------------
// 10.5.1.6 Mobile Station Classmark 2
//------------------------------------------------------------------------------
bool mobile_station_classmark_2_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, mobile_station_classmark2_t * const mobilestationclassmark2)
{
  bool res = false;
  bstring xpath_expr = bformat("./%s",MOBILE_STATION_CLASSMARK_2_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));
      if (res) {
        uint8_t  revisionlevel = 0;
        bstring xpath_expr = bformat("./%s",REVISION_LEVEL_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&revisionlevel, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->revisionlevel = revisionlevel;
      }
      if (res) {
        uint8_t  esind = 0;
        bstring xpath_expr = bformat("./%s",ES_IND_LEVEL_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&esind, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->esind = esind;
      }
      if (res) {
        uint8_t  a51 = 0;
        bstring xpath_expr = bformat("./%s",A51_LEVEL_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&a51, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->a51 = a51;
      }
      if (res) {
        uint8_t  rfpowercapability = 0;
        bstring xpath_expr = bformat("./%s",RF_POWER_CAPABILITY_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&rfpowercapability, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->rfpowercapability = rfpowercapability;
      }
      if (res) {
        uint8_t  pscapability = 0;
        bstring xpath_expr = bformat("./%s",PS_CAPABILITY_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&pscapability, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->pscapability = pscapability;
      }
      if (res) {
        uint8_t  ssscreenindicator = 0;
        bstring xpath_expr = bformat("./%s",SS_SCREEN_INDICATOR_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&ssscreenindicator, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->ssscreenindicator = ssscreenindicator;
      }
      if (res) {
        uint8_t  smcapability = 0;
        bstring xpath_expr = bformat("./%s",SM_CAPABILITY_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&smcapability, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->smcapability = smcapability;
      }
      if (res) {
        uint8_t  vbs = 0;
        bstring xpath_expr = bformat("./%s",VBS_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&vbs, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->vbs = vbs;
      }
      if (res) {
        uint8_t  vgcs = 0;
        bstring xpath_expr = bformat("./%s",VGCS_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&vgcs, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->vgcs = vgcs;
      }
      if (res) {
        uint8_t  fc = 0;
        bstring xpath_expr = bformat("./%s",FC_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&fc, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->fc = fc;
      }
      if (res) {
        uint8_t  cm3 = 0;
        bstring xpath_expr = bformat("./%s",CM3_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&cm3, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->cm3 = cm3;
      }
      if (res) {
        uint8_t  lcsvacap = 0;
        bstring xpath_expr = bformat("./%s",LCS_VA_CAP_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&lcsvacap, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->lcsvacap = lcsvacap;
      }
      if (res) {
        uint8_t  ucs2 = 0;
        bstring xpath_expr = bformat("./%s",UCS2_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&ucs2, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->ucs2 = ucs2;
      }
      if (res) {
        uint8_t  solsa = 0;
        bstring xpath_expr = bformat("./%s",SOLSA_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&solsa, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->solsa = solsa;
      }
      if (res) {
        uint8_t  cmsp = 0;
        bstring xpath_expr = bformat("./%s",CMSP_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&cmsp, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->cmsp = cmsp;
      }
      if (res) {
        uint8_t  a53 = 0;
        bstring xpath_expr = bformat("./%s",A53_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&a53, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->a53 = a53;
      }
      if (res) {
        uint8_t  a52 = 0;
        bstring xpath_expr = bformat("./%s",A52_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx16, (void*)&a52, NULL);
        bdestroy_wrapper (&xpath_expr);
        mobilestationclassmark2->a52 = a52;
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  return res;
}

//------------------------------------------------------------------------------
void mobile_station_classmark_2_to_xml (const mobile_station_classmark2_t * const mobilestationclassmark2, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, MOBILE_STATION_CLASSMARK_2_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, REVISION_LEVEL_ATTR_XML_STR, "0x%x", mobilestationclassmark2->revisionlevel);
  XML_WRITE_FORMAT_ELEMENT(writer, ES_IND_LEVEL_ATTR_XML_STR, "0x%x", mobilestationclassmark2->esind);
  XML_WRITE_FORMAT_ELEMENT(writer, A51_LEVEL_ATTR_XML_STR, "0x%x", mobilestationclassmark2->a51);
  XML_WRITE_FORMAT_ELEMENT(writer, RF_POWER_CAPABILITY_ATTR_XML_STR, "0x%x", mobilestationclassmark2->rfpowercapability);
  XML_WRITE_FORMAT_ELEMENT(writer, PS_CAPABILITY_ATTR_XML_STR, "0x%x", mobilestationclassmark2->pscapability);
  XML_WRITE_FORMAT_ELEMENT(writer, SS_SCREEN_INDICATOR_ATTR_XML_STR, "0x%x", mobilestationclassmark2->ssscreenindicator);
  XML_WRITE_FORMAT_ELEMENT(writer, SM_CAPABILITY_ATTR_XML_STR, "0x%x", mobilestationclassmark2->smcapability);
  XML_WRITE_FORMAT_ELEMENT(writer, VBS_ATTR_XML_STR, "0x%x", mobilestationclassmark2->vbs);
  XML_WRITE_FORMAT_ELEMENT(writer, VGCS_ATTR_XML_STR, "0x%x", mobilestationclassmark2->vgcs);
  XML_WRITE_FORMAT_ELEMENT(writer, FC_ATTR_XML_STR, "0x%x", mobilestationclassmark2->fc);
  XML_WRITE_FORMAT_ELEMENT(writer, CM3_ATTR_XML_STR, "0x%x", mobilestationclassmark2->cm3);
  XML_WRITE_FORMAT_ELEMENT(writer, LCS_VA_CAP_ATTR_XML_STR, "0x%x", mobilestationclassmark2->lcsvacap);
  XML_WRITE_FORMAT_ELEMENT(writer, UCS2_ATTR_XML_STR, "0x%x", mobilestationclassmark2->ucs2);
  XML_WRITE_FORMAT_ELEMENT(writer, SOLSA_ATTR_XML_STR, "0x%x", mobilestationclassmark2->solsa);
  XML_WRITE_FORMAT_ELEMENT(writer, CMSP_ATTR_XML_STR, "0x%x", mobilestationclassmark2->cmsp);
  XML_WRITE_FORMAT_ELEMENT(writer, A53_ATTR_XML_STR, "0x%x", mobilestationclassmark2->a53);
  XML_WRITE_FORMAT_ELEMENT(writer, A52_ATTR_XML_STR, "0x%x", mobilestationclassmark2->a52);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
// 10.5.1.7 Mobile Station Classmark 3
//------------------------------------------------------------------------------
bool mobile_station_classmark_3_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, mobile_station_classmark3_t * const mobilestationclassmark3)
{
  bool res = false;
  bstring xpath_expr = bformat("./%s",MOBILE_STATION_CLASSMARK_3_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {
      AssertFatal(0, "TODO");
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  return res;
}

//------------------------------------------------------------------------------
void mobile_station_classmark_3_to_xml (const mobile_station_classmark3_t * const mobilestationclassmark3, xmlTextWriterPtr writer)
{
  XML_WRITE_COMMENT(writer, "TODO "MOBILE_STATION_CLASSMARK_3_XML_STR);
}

//------------------------------------------------------------------------------
// 10.5.1.13 PLMN list
//------------------------------------------------------------------------------
bool plmn_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    plmn_t                * const plmn)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr_plmn = bformat("./%s",PLMN_XML_STR);
  xmlXPathObjectPtr xpath_obj_plmn = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_plmn);
  if (xpath_obj_plmn) {
    xmlNodeSetPtr nodes_plmn = xpath_obj_plmn->nodesetval;
    int size = (nodes_plmn) ? nodes_plmn->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_plmn->nodeTab[0], xpath_ctx));
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
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_plmn);
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void plmn_to_xml (const plmn_t * const plmn, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, PLMN_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, MOBILE_COUNTRY_CODE_ATTR_XML_STR, "%x%x%x",
      plmn->mcc_digit1, plmn->mcc_digit2, plmn->mcc_digit3);
  XML_WRITE_FORMAT_ELEMENT(writer, MOBILE_NETWORK_CODE_ATTR_XML_STR, "%x%x%x",
      plmn->mnc_digit1, plmn->mnc_digit2, plmn->mnc_digit3);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool plmn_list_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    plmn_list_t           * const plmnlist)
{
  OAILOG_FUNC_IN (LOG_XML);
  memset((void*)plmnlist, 0, sizeof(*plmnlist));
  bool res = false;
  bstring xpath_expr_plmnlist = bformat("./%s",PLMN_LIST_XML_STR);
  xmlXPathObjectPtr xpath_obj_plmnlist = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_plmnlist);
  if (xpath_obj_plmnlist) {
    xmlNodeSetPtr nodes_plmnlist = xpath_obj_plmnlist->nodesetval;
    int sizelist = (nodes_plmnlist) ? nodes_plmnlist->nodeNr : 0;
    if ((1 == sizelist) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_plmnlist->nodeTab[0], xpath_ctx));
      if (res) {
        bstring xpath_expr_plmn = bformat("./%s",PLMN_XML_STR);
        xmlXPathObjectPtr xpath_obj_plmn = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_plmn);
        if (xpath_obj_plmn) {
          xmlNodeSetPtr nodes_plmn = xpath_obj_plmn->nodesetval;
          int size = (nodes_plmn) ? nodes_plmn->nodeNr : 0;
          if ((0 < size) && (xml_doc)) {
            for (int i=0; i < size; i++) {
              xmlNodePtr saved2_node_ptr = xpath_ctx->node;
              res = (RETURNok == xmlXPathSetContextNode(nodes_plmn->nodeTab[i], xpath_ctx));
              if (res) {
                res = false;
                bstring xpath_expr_mcc = bformat("./%s",MOBILE_COUNTRY_CODE_ATTR_XML_STR);
                xmlXPathObjectPtr xpath_obj_mcc = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_mcc);
                if (xpath_obj_mcc) {
                  xmlNodeSetPtr nodes_mcc = xpath_obj_mcc->nodesetval;
                  int size_mcc = (nodes_mcc) ? nodes_mcc->nodeNr : 0;
                  if ((1 == size_mcc) && (xml_doc)) {
                    xmlChar *key = xmlNodeListGetString(xml_doc, nodes_mcc->nodeTab[0]->xmlChildrenNode, 1);
                    uint8_t mcc_digit[3] = {0};
                    int ret = sscanf((const char*)key, "%1"SCNx8"%1"SCNx8"%1"SCNx8, &mcc_digit[0], &mcc_digit[1], &mcc_digit[2]);
                    xmlFree(key);
                    if (3 == ret) {
                      plmnlist->plmn[i].mcc_digit1 = mcc_digit[0];
                      plmnlist->plmn[i].mcc_digit2 = mcc_digit[1];
                      plmnlist->plmn[i].mcc_digit3 = mcc_digit[2];
                      OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s/%s = %x%x%x\n",
                          PLMN_XML_STR, MOBILE_COUNTRY_CODE_ATTR_XML_STR,
                          plmnlist->plmn[i].mcc_digit1,
                          plmnlist->plmn[i].mcc_digit2,
                          plmnlist->plmn[i].mcc_digit3);
                    }
                    res = (3 == ret);
                  }
                  xmlXPathFreeObject(xpath_obj_mcc);
                }
                bdestroy_wrapper (&xpath_expr_mcc);
              }
              res = (RETURNok == xmlXPathSetContextNode(saved2_node_ptr, xpath_ctx)) & res;
              if (res) {
                res = false;
                bstring xpath_expr_mnc = bformat("./%s",MOBILE_NETWORK_CODE_ATTR_XML_STR);
                xmlXPathObjectPtr xpath_obj_mnc = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_mnc);
                if (xpath_obj_mnc) {
                  xmlNodeSetPtr nodes_mnc = xpath_obj_mnc->nodesetval;
                  int size_mnc = (nodes_mnc) ? nodes_mnc->nodeNr : 0;
                  if ((1 == size_mnc) && (xml_doc)) {
                    xmlChar *key = xmlNodeListGetString(xml_doc, nodes_mnc->nodeTab[0]->xmlChildrenNode, 1);
                    uint8_t mnc_digit[3] = {0};
                    int ret = sscanf((const char*)key, "%1"SCNx8"%1"SCNx8"%1"SCNx8, &mnc_digit[0], &mnc_digit[1], &mnc_digit[2]);
                    xmlFree(key);
                    if (3 == ret) {
                      plmnlist->plmn[i].mnc_digit1 = mnc_digit[0];
                      plmnlist->plmn[i].mnc_digit2 = mnc_digit[1];
                      plmnlist->plmn[i].mnc_digit3 = mnc_digit[2];
                      OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s/%s = %x%x%x\n",
                          PLMN_XML_STR, MOBILE_NETWORK_CODE_ATTR_XML_STR,
                          plmnlist->plmn[i].mnc_digit1,
                          plmnlist->plmn[i].mnc_digit2,
                          plmnlist->plmn[i].mnc_digit3);
                    }
                    res = (3 == ret);
                  }
                  xmlXPathFreeObject(xpath_obj_mnc);
                }
                bdestroy_wrapper (&xpath_expr_mnc);
              }
            }
            if (res) {
              plmnlist->num_plmn = size;
            }
          }
        }
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_plmnlist);
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
void plmn_list_to_xml (const plmn_list_t * const plmnlist, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, PLMN_LIST_XML_STR);
  for (int i = 0; i < plmnlist->num_plmn; i++) {
    plmn_to_xml(&plmnlist->plmn[i], writer);
  }
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
// 10.5.1.15 MS network feature support
//------------------------------------------------------------------------------
bool ms_network_feature_support_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, ms_network_feature_support_t * const msnetworkfeaturesupport)
{
  OAILOG_FUNC_IN (LOG_XML);
  memset(msnetworkfeaturesupport, 0, sizeof(*msnetworkfeaturesupport));
  bool res = false;
  bstring xpath_expr = bformat("./%s",MS_NEWORK_FEATURE_SUPPORT_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));
      if (res) {
        uint8_t extended_periodic_timers= 0;
        bstring xpath_expr = bformat("./%s",EXTENDED_PERIODIC_TIMERS_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&extended_periodic_timers, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkfeaturesupport->extended_periodic_timers = extended_periodic_timers;
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
void ms_network_feature_support_to_xml(const ms_network_feature_support_t * const msnetworkfeaturesupport, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, MS_NEWORK_FEATURE_SUPPORT_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, EXTENDED_PERIODIC_TIMERS_XML_STR, "%"PRIu8, msnetworkfeaturesupport->extended_periodic_timers);
  XML_WRITE_END_ELEMENT(writer);
}



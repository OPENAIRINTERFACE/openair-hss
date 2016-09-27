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
#include "conversions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_24.301.h"
#include "3gpp_33.401.h"
#include "common_defs.h"
#include "security_types.h"
#include "common_types.h"
#include "ApnAggregateMaximumBitRate.h"
#include "EpsQualityOfService.h"
#include "EsmCause.h"
#include "EsmInformationTransferFlag.h"
#include "LinkedEpsBearerIdentity.h"
#include "PdnAddress.h"
#include "RadioPriority.h"
#include "PdnType.h"
#include "NasRequestType.h"
#include "TrafficFlowAggregateDescription.h"

#include "xml_load.h"
#include "mme_scenario_player.h"
#include "3gpp_24.301_esm_ies_xml.h"
#include "xml2_wrapper.h"

//------------------------------------------------------------------------------
bool apn_aggregate_maximum_bit_rate_from_xml (
    xmlDocPtr                          xml_doc,
    xmlXPathContextPtr                 xpath_ctx,
    ApnAggregateMaximumBitRate * const apnaggregatemaximumbitrate)
{
  OAILOG_FUNC_IN (LOG_NAS);
  memset(apnaggregatemaximumbitrate, 0, sizeof(*apnaggregatemaximumbitrate));
  bool res = false;
  bstring xpath_expr_apn = bformat("./%s",APN_AGGREGATE_MAXIMUM_BIT_RATE_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj_apn = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_apn);
  if (xpath_obj_apn) {
    xmlNodeSetPtr nodes_apn = xpath_obj_apn->nodesetval;
    int size = (nodes_apn) ? nodes_apn->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_apn->nodeTab[0], xpath_ctx));
      if (res) {
        uint8_t  apnambrfordownlink = 0;
        bstring xpath_expr = bformat("./%s",APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_DOWNLINK_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&apnambrfordownlink, NULL);
        bdestroy_wrapper (&xpath_expr);
        apnaggregatemaximumbitrate->apnambrfordownlink = apnambrfordownlink;
      }
      if (res) {
        uint8_t  apnambrforuplink = 0;
        bstring xpath_expr = bformat("./%s",APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_UPLINK_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&apnambrforuplink, NULL);
        bdestroy_wrapper (&xpath_expr);
        apnaggregatemaximumbitrate->apnambrforuplink = apnambrforuplink;
      }
      if (res) {
        uint8_t  apnambrfordownlink_extended = 0;
        bstring xpath_expr = bformat("./%s",APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_DOWNLINK_EXTENDED_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&apnambrfordownlink_extended, NULL);
        bdestroy_wrapper (&xpath_expr);
        apnaggregatemaximumbitrate->apnambrfordownlink_extended = apnambrfordownlink_extended;

        if (res) {
          apnaggregatemaximumbitrate->extensions |= APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION_PRESENT;

          uint8_t  apnambrforuplink_extended = 0;
          bstring xpath_expr = bformat("./%s",APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_UPLINK_EXTENDED_ATTR_XML_STR);
          res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&apnambrforuplink_extended, NULL);
          bdestroy_wrapper (&xpath_expr);
          apnaggregatemaximumbitrate->apnambrforuplink_extended = apnambrforuplink_extended;

          if (res) {
            uint8_t  apnambrfordownlink_extended2 = 0;
            xpath_expr = bformat("./%s",APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_DOWNLINK_EXTENDED2_ATTR_XML_STR);
            res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&apnambrfordownlink_extended2, NULL);
            bdestroy_wrapper (&xpath_expr);
            apnaggregatemaximumbitrate->apnambrfordownlink_extended2 = apnambrfordownlink_extended2;

            if (res) {
              apnaggregatemaximumbitrate->extensions |= APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION2_PRESENT;
              uint8_t  apnambrforuplink_extended2 = 0;
              xpath_expr = bformat("./%s",APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_UPLINK_EXTENDED2_ATTR_XML_STR);
              res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&apnambrforuplink_extended2, NULL);
              bdestroy_wrapper (&xpath_expr);
              apnaggregatemaximumbitrate->apnambrforuplink_extended2 = apnambrforuplink_extended2;
            } else {
              res = true; // do not report that apn_aggregate_maximum_bit_rate_from_xml failed at all
            }
          }
        } else {
          res = true; // do not report that apn_aggregate_maximum_bit_rate_from_xml failed at all
        }
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
  }
  bdestroy_wrapper (&xpath_expr_apn);
  OAILOG_FUNC_RETURN (LOG_NAS, res);
}
//------------------------------------------------------------------------------
void apn_aggregate_maximum_bit_rate_to_xml(ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, APN_AGGREGATE_MAXIMUM_BIT_RATE_IE_XML_STR);
  XML_WRITE_COMMENT(writer, "TODO human readable form, even in comment ?")
  XML_WRITE_FORMAT_ELEMENT(writer, APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_DOWNLINK_ATTR_XML_STR, "%"PRIx8, apnaggregatemaximumbitrate->apnambrfordownlink);
  XML_WRITE_FORMAT_ELEMENT(writer, APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_UPLINK_ATTR_XML_STR, "%"PRIx8, apnaggregatemaximumbitrate->apnambrforuplink);
  if (apnaggregatemaximumbitrate->extensions & APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION_PRESENT) {
    XML_WRITE_FORMAT_ELEMENT(writer, APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_DOWNLINK_EXTENDED_ATTR_XML_STR, "%"PRIx8, apnaggregatemaximumbitrate->apnambrfordownlink_extended);
    XML_WRITE_FORMAT_ELEMENT(writer, APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_UPLINK_EXTENDED_ATTR_XML_STR, "%"PRIx8, apnaggregatemaximumbitrate->apnambrforuplink_extended);
    if (apnaggregatemaximumbitrate->extensions & APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION2_PRESENT) {
      XML_WRITE_FORMAT_ELEMENT(writer, APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_DOWNLINK_EXTENDED2_ATTR_XML_STR, "%"PRIx8, apnaggregatemaximumbitrate->apnambrfordownlink_extended2);
      XML_WRITE_FORMAT_ELEMENT(writer, APN_AGGREGATE_MAXIMUM_BIT_RATE_FOR_UPLINK_EXTENDED2_ATTR_XML_STR, "%"PRIx8, apnaggregatemaximumbitrate->apnambrforuplink_extended2);
    }
  }
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool eps_quality_of_service_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    EpsQualityOfService   * const epsqualityofservice)
{
  OAILOG_FUNC_IN (LOG_NAS);
  memset(epsqualityofservice, 0, sizeof(*epsqualityofservice));
  bool res = false;
  bstring xpath_expr_qos = bformat("./%s",EPS_QUALITY_OF_SERVICE_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj_qos = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_qos);
  if (xpath_obj_qos) {
    xmlNodeSetPtr nodes_qos = xpath_obj_qos->nodesetval;
    int size = (nodes_qos) ? nodes_qos->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

/*
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_qos->nodeTab[0], xpath_ctx));
      if (res) {
        uint8_t  qci = 0;
        bstring xpath_expr = bformat("./%s",QCI_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&qci, NULL);
        bdestroy_wrapper (&xpath_expr);
        epsqualityofservice->qci = qci;
      }
*/
      res = true;
      if (res) {
        xmlChar *attr = xmlGetProp(nodes_qos->nodeTab[0], (const xmlChar *)QCI_ATTR_XML_STR);
        if (attr) {
          OAILOG_TRACE (LOG_NAS, "Found %s=%s\n", QCI_ATTR_XML_STR, attr);
          uint8_t  qci = 0;
          sscanf((const char*)attr, "%"SCNx8, &qci);
          epsqualityofservice->qci = qci;
          xmlFree(attr);
        } else {
          res = false;
        }
      }

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_qos->nodeTab[0], xpath_ctx));
      if (res) {
        uint8_t  maxbitrateforul = 0;
        bstring xpath_expr = bformat("./%s",MAXIMUM_BIT_RATE_FOR_UPLINK_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&maxbitrateforul, NULL);
        bdestroy_wrapper (&xpath_expr);
        epsqualityofservice->bitRates.maxBitRateForUL = maxbitrateforul;

        if (res) {
          epsqualityofservice->bitRatesPresent = true;

          uint8_t  maxbitratefordl = 0;
          xpath_expr = bformat("./%s",MAXIMUM_BIT_RATE_FOR_DOWNLINK_ATTR_XML_STR);
          res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&maxbitratefordl, NULL);
          bdestroy_wrapper (&xpath_expr);
          epsqualityofservice->bitRates.maxBitRateForDL = maxbitratefordl;

          if (res) {
            uint8_t  guarbitrateforul = 0;
            xpath_expr = bformat("./%s",GUARANTED_BIT_RATE_FOR_UPLINK_ATTR_XML_STR);
            res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&guarbitrateforul, NULL);
            bdestroy_wrapper (&xpath_expr);
            epsqualityofservice->bitRates.guarBitRateForUL = guarbitrateforul;
          }
          if (res) {
            uint8_t  guarbitratefordl = 0;
            xpath_expr = bformat("./%s",GUARANTED_BIT_RATE_FOR_DOWNLINK_ATTR_XML_STR);
            res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&guarbitratefordl, NULL);
            bdestroy_wrapper (&xpath_expr);
            epsqualityofservice->bitRates.guarBitRateForDL = guarbitratefordl;
          }

          if (res) {
            uint8_t  extmaxbitrateforul = 0;
            xpath_expr = bformat("./%s",MAXIMUM_BIT_RATE_FOR_UPLINK_EXTENDED_ATTR_XML_STR);
            res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&extmaxbitrateforul, NULL);
            bdestroy_wrapper (&xpath_expr);
            epsqualityofservice->bitRatesExt.maxBitRateForUL = extmaxbitrateforul;
            if (res) {
              epsqualityofservice->bitRatesExtPresent = true;

              uint8_t  extmaxbitratefordl = 0;
              xpath_expr = bformat("./%s",MAXIMUM_BIT_RATE_FOR_DOWNLINK_EXTENDED_ATTR_XML_STR);
              res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&extmaxbitratefordl, NULL);
              bdestroy_wrapper (&xpath_expr);
              epsqualityofservice->bitRatesExt.maxBitRateForDL = extmaxbitratefordl;

              if (res) {
                uint8_t  extguarbitrateforul = 0;
                xpath_expr = bformat("./%s",GUARANTED_BIT_RATE_FOR_UPLINK_EXTENDED_ATTR_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&extguarbitrateforul, NULL);
                bdestroy_wrapper (&xpath_expr);
                epsqualityofservice->bitRatesExt.guarBitRateForUL = extguarbitrateforul;
              }
              if (res) {
                uint8_t  extguarbitratefordl = 0;
                xpath_expr = bformat("./%s",GUARANTED_BIT_RATE_FOR_DOWNLINK_EXTENDED_ATTR_XML_STR);
                res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&extguarbitratefordl, NULL);
                bdestroy_wrapper (&xpath_expr);
                epsqualityofservice->bitRatesExt.guarBitRateForDL = extguarbitratefordl;
              }
            }else {
              res = true; // do not report that eps_quality_of_service_from_xml failed at all
            }
          }
        } else {
          res = true; // do not report that eps_quality_of_service_from_xml failed at all
        }
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
  }
  bdestroy_wrapper (&xpath_expr_qos);
  OAILOG_FUNC_RETURN (LOG_NAS, res);
}
//------------------------------------------------------------------------------
void eps_quality_of_service_to_xml (EpsQualityOfService * epsqualityofservice, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, EPS_QUALITY_OF_SERVICE_IE_XML_STR);

  XML_WRITE_FORMAT_ATTRIBUTE(writer, QCI_ATTR_XML_STR, "%"PRIx8, epsqualityofservice->qci);

  if (epsqualityofservice->bitRatesPresent) {
    XML_WRITE_FORMAT_ELEMENT(writer, MAXIMUM_BIT_RATE_FOR_UPLINK_ATTR_XML_STR, "%"PRIx8, epsqualityofservice->bitRates.maxBitRateForUL);
    XML_WRITE_FORMAT_ELEMENT(writer, MAXIMUM_BIT_RATE_FOR_DOWNLINK_ATTR_XML_STR, "%"PRIx8, epsqualityofservice->bitRates.maxBitRateForDL);
    XML_WRITE_FORMAT_ELEMENT(writer, GUARANTED_BIT_RATE_FOR_UPLINK_ATTR_XML_STR, "%"PRIx8, epsqualityofservice->bitRates.guarBitRateForUL);
    XML_WRITE_FORMAT_ELEMENT(writer, GUARANTED_BIT_RATE_FOR_DOWNLINK_ATTR_XML_STR, "%"PRIx8, epsqualityofservice->bitRates.guarBitRateForDL);
  }

  if (epsqualityofservice->bitRatesExtPresent) {
    XML_WRITE_FORMAT_ELEMENT(writer, MAXIMUM_BIT_RATE_FOR_UPLINK_EXTENDED_ATTR_XML_STR, "%"PRIx8, epsqualityofservice->bitRatesExt.maxBitRateForUL);
    XML_WRITE_FORMAT_ELEMENT(writer, MAXIMUM_BIT_RATE_FOR_DOWNLINK_EXTENDED_ATTR_XML_STR, "%"PRIx8, epsqualityofservice->bitRatesExt.maxBitRateForDL);
    XML_WRITE_FORMAT_ELEMENT(writer, GUARANTED_BIT_RATE_FOR_UPLINK_EXTENDED_ATTR_XML_STR, "%"PRIx8, epsqualityofservice->bitRatesExt.guarBitRateForUL);
    XML_WRITE_FORMAT_ELEMENT(writer, GUARANTED_BIT_RATE_FOR_DOWNLINK_EXTENDED_ATTR_XML_STR, "%"PRIx8, epsqualityofservice->bitRatesExt.guarBitRateForDL);
  }

  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( esm_cause, ESM_CAUSE);
//------------------------------------------------------------------------------
void esm_cause_to_xml (esm_cause_t * esmcause, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, ESM_CAUSE_IE_XML_STR, "%"PRIx8, *esmcause);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( esm_information_transfer_flag, ESM_INFORMATION_TRANSFER_FLAG);
//------------------------------------------------------------------------------
void esm_information_transfer_flag_to_xml (esm_information_transfer_flag_t * esminformationtransferflag, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, ESM_INFORMATION_TRANSFER_FLAG_IE_XML_STR, "%"PRIx8, *esminformationtransferflag);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( linked_eps_bearer_identity, LINKED_EPS_BEARER_IDENTITY);
//------------------------------------------------------------------------------
void linked_eps_bearer_identity_to_xml (linked_eps_bearer_identity_t * linkedepsbeareridentity, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, LINKED_EPS_BEARER_IDENTITY_IE_XML_STR, "%"PRIx8, *linkedepsbeareridentity);
}

//------------------------------------------------------------------------------
bool pdn_address_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    PdnAddress            * const pdnaddress)
{
  OAILOG_FUNC_IN (LOG_NAS);
  memset(pdnaddress, 0, sizeof(*pdnaddress));
  bool res = false;
  bstring xpath_expr = bformat("./%s",PDN_ADDRESS_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {
      res = true;
      xmlChar *attr = xmlGetProp(nodes->nodeTab[0], (const xmlChar *)PDN_TYPE_VALUE_ATTR_XML_STR);
      if (attr) {
        OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Found %s=%s\n", PDN_TYPE_VALUE_ATTR_XML_STR, attr);
        if (!strcasecmp((const char*)attr, PDN_TYPE_VALUE_IPV4_VAL_XML_STR)) {
          pdnaddress->pdntypevalue = PDN_VALUE_TYPE_IPV4;
        } else if (!strcasecmp((const char*)attr, PDN_TYPE_VALUE_IPV6_VAL_XML_STR)) {
          pdnaddress->pdntypevalue = PDN_VALUE_TYPE_IPV6;
        } else  if (!strcasecmp((const char*)attr, PDN_TYPE_VALUE_IPV4V6_VAL_XML_STR)) {
          pdnaddress->pdntypevalue = PDN_VALUE_TYPE_IPV4V6;
        } else {
          res = false;
        }
        xmlFree(attr);
      } else {
        res = false;
      }

      if (res) {
        xmlNodePtr saved_node_ptr = xpath_ctx->node;
        res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));
        bstring xpath_expr_pdnai = bformat("./%s",PDN_ADDRESS_INFORMATION_ATTR_XML_STR);
        xml_load_hex_stream_leaf_tag(xml_doc,xpath_ctx, xpath_expr_pdnai,&pdnaddress->pdnaddressinformation);
        bdestroy_wrapper (&xpath_expr_pdnai);
        res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
      }
    }
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_NAS, res);
}
//------------------------------------------------------------------------------
void pdn_address_to_xml (PdnAddress * pdnaddress, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, PDN_ADDRESS_IE_XML_STR);
  switch(pdnaddress->pdntypevalue) {
  case PDN_VALUE_TYPE_IPV4:
    XML_WRITE_FORMAT_ATTRIBUTE(writer, PDN_TYPE_VALUE_ATTR_XML_STR, "%s", PDN_TYPE_VALUE_IPV4_VAL_XML_STR);
    break;
  case PDN_VALUE_TYPE_IPV6:
    XML_WRITE_FORMAT_ATTRIBUTE(writer, PDN_TYPE_VALUE_ATTR_XML_STR, "%s", PDN_TYPE_VALUE_IPV6_VAL_XML_STR);
    break;
  case PDN_VALUE_TYPE_IPV4V6:
    XML_WRITE_FORMAT_ATTRIBUTE(writer, PDN_TYPE_VALUE_ATTR_XML_STR, "%s", PDN_TYPE_VALUE_IPV4V6_VAL_XML_STR);
    break;
  default:
    XML_WRITE_FORMAT_ATTRIBUTE(writer, PDN_TYPE_VALUE_ATTR_XML_STR, "%x", pdnaddress->pdntypevalue);
  }
  // hexa to ascii
  XML_WRITE_HEX_ELEMENT(writer, PDN_ADDRESS_INFORMATION_ATTR_XML_STR, bdata(pdnaddress->pdnaddressinformation), blength(pdnaddress->pdnaddressinformation));
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool pdn_type_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    pdn_type_t               * const pdntype)
{
  OAILOG_FUNC_IN (LOG_NAS);
  memset(pdntype, 0, sizeof(*pdntype));
  bstring xpath_expr = bformat("./%s",PDN_TYPE_IE_XML_STR);
  char pdn_type_str[64] = {0};
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)pdn_type_str, NULL);
  if (res) {
    if (!strcasecmp(pdn_type_str, PDN_TYPE_IPV4_VAL_XML_STR)) {
      *pdntype = PDN_TYPE_IPV4;
    } else if (!strcasecmp(pdn_type_str, PDN_TYPE_IPV6_VAL_XML_STR)) {
      *pdntype = PDN_TYPE_IPV6;
    } else if (!strcasecmp(pdn_type_str, PDN_TYPE_IPV4V6_VAL_XML_STR)) {
      *pdntype = PDN_TYPE_IPV4V6;
    } else if (!strcasecmp(pdn_type_str, PDN_TYPE_UNUSED_VAL_XML_STR)) {
      *pdntype = PDN_TYPE_UNUSED;
      res = false;
    } else {
      res = false;
    }
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_NAS, res);
}

//------------------------------------------------------------------------------
void pdn_type_to_xml (pdn_type_t * pdntype, xmlTextWriterPtr writer)
{
  if (pdntype) {
    switch (*pdntype) {
    case PDN_TYPE_IPV4:
      XML_WRITE_FORMAT_ELEMENT(writer, PDN_TYPE_IE_XML_STR, "%s", PDN_TYPE_IPV4_VAL_XML_STR);
      break;
    case PDN_TYPE_IPV6:
      XML_WRITE_FORMAT_ELEMENT(writer, PDN_TYPE_IE_XML_STR, "%s", PDN_TYPE_IPV6_VAL_XML_STR);
      break;
    case PDN_TYPE_IPV4V6:
      XML_WRITE_FORMAT_ELEMENT(writer, PDN_TYPE_IE_XML_STR, "%s", PDN_TYPE_IPV4V6_VAL_XML_STR);
      break;
    case PDN_TYPE_UNUSED:
      XML_WRITE_FORMAT_ELEMENT(writer, PDN_TYPE_IE_XML_STR, "%s", PDN_TYPE_UNUSED_VAL_XML_STR);
      break;
    default:
      XML_WRITE_FORMAT_ELEMENT(writer, PDN_TYPE_IE_XML_STR, "%x", *pdntype);
    }
  }
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE(radio_priority, RADIO_PRIORITY);
//------------------------------------------------------------------------------
void radio_priority_to_xml (radio_priority_t * radio_priority, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, RADIO_PRIORITY_IE_XML_STR, RADIO_PRIORITY_XML_FMT, *radio_priority);
}

//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE( request_type, REQUEST_TYPE);
//------------------------------------------------------------------------------
void request_type_to_xml (request_type_t * requesttype, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, REQUEST_TYPE_IE_XML_STR, REQUEST_TYPE_XML_FMT, *requesttype);
}



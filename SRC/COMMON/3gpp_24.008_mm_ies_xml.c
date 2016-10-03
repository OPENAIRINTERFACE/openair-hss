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
#include "log.h"
#include "conversions.h"
#include "common_defs.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "xml_load.h"
#include "3gpp_24.008_xml.h"
#include "xml2_wrapper.h"


//******************************************************************************
// 10.5.3 Mobility management information elements.
//******************************************************************************
//------------------------------------------------------------------------------
// 10.5.3.1 Authentication parameter RAND
//------------------------------------------------------------------------------
bool authentication_parameter_rand_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, authentication_parameter_rand_t * authenticationparameterrand)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  char hexascii[64]  = {0};
  uint8_t hex[32]    = {0};
  bstring xpath_expr = bformat("./%s",AUTHENTICATION_PARAMETER_RAND_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)hexascii, NULL);
  if (res) {
    int len = strlen(hexascii);
    if (!ascii_to_hex(hex, hexascii)) res = false;
    if (res) {
      *authenticationparameterrand = blk2bstr((const void *)hex, len/2);
    }
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------
void authentication_parameter_rand_to_xml (authentication_parameter_rand_t authenticationparameterrand, xmlTextWriterPtr writer)
{
  XML_WRITE_HEX_ELEMENT(writer, AUTHENTICATION_PARAMETER_RAND_IE_XML_STR, bdata(authenticationparameterrand), blength(authenticationparameterrand));
}

//------------------------------------------------------------------------------
// 10.5.3.1.1 Authentication Parameter AUTN (UMTS and EPS authentication challenge)
//------------------------------------------------------------------------------
bool authentication_parameter_autn_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, authentication_parameter_autn_t * authenticationparameterautn)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  char hexascii[64]  = {0};
  uint8_t hex[32]    = {0};
  bstring xpath_expr = bformat("./%s",AUTHENTICATION_PARAMETER_AUTN_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)hexascii, NULL);
  if (res) {
    int len = strlen(hexascii);
    if (!ascii_to_hex(hex, hexascii)) res = false;
    if (res) {
      *authenticationparameterautn = blk2bstr((const void *)hex, len/2);
    }
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------

void authentication_parameter_autn_to_xml (authentication_parameter_autn_t authenticationparameterautn, xmlTextWriterPtr writer)
{
  XML_WRITE_HEX_ELEMENT(writer, AUTHENTICATION_PARAMETER_AUTN_IE_XML_STR, bdata(authenticationparameterautn), blength(authenticationparameterautn));
}

//------------------------------------------------------------------------------
// 10.5.3.2 Authentication Response parameter
//------------------------------------------------------------------------------
bool authentication_response_parameter_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, authentication_response_parameter_t * authenticationresponseparameter)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  char hexascii[64]  = {0};
  uint8_t hex[32]    = {0};
  bstring xpath_expr = bformat("./%s",AUTHENTICATION_RESPONSE_PARAMETER_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)hexascii, NULL);
  if (res) {
    int len = strlen(hexascii);
    if (!ascii_to_hex(hex, hexascii)) res = false;
    if (res) {
      *authenticationresponseparameter = blk2bstr((const void *)hex, len/2);
    }
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------
void authentication_response_parameter_to_xml (authentication_response_parameter_t authenticationresponseparameter, xmlTextWriterPtr writer)
{
  XML_WRITE_HEX_ELEMENT(writer, AUTHENTICATION_RESPONSE_PARAMETER_IE_XML_STR, bdata(authenticationresponseparameter), blength(authenticationresponseparameter));
}

//------------------------------------------------------------------------------
// 10.5.3.2.2 Authentication Failure parameter (UMTS and EPS authentication challenge)
//------------------------------------------------------------------------------
bool authentication_failure_parameter_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, authentication_failure_parameter_t * authenticationfailureparameter)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  char hexascii[64]  = {0};
  uint8_t hex[32]    = {0};
  bstring xpath_expr = bformat("./%s",AUTHENTICATION_FAILURE_PARAMETER_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)hexascii, NULL);
  if (res) {
    int len = strlen(hexascii);
    if (!ascii_to_hex(hex, hexascii)) res = false;
    if (res) {
      *authenticationfailureparameter = blk2bstr((const void *)hex, len/2);
    }
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------
void authentication_failure_parameter_to_xml (authentication_failure_parameter_t authenticationfailureparameter, xmlTextWriterPtr writer)
{
  XML_WRITE_HEX_ELEMENT(writer, AUTHENTICATION_FAILURE_PARAMETER_IE_XML_STR, bdata(authenticationfailureparameter), blength(authenticationfailureparameter));
}

//------------------------------------------------------------------------------
// 10.5.3.5a Network Name
//------------------------------------------------------------------------------
bool network_name_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, const char * const ie, network_name_t * const networkname)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bool res = false;
  bstring xpath_expr_nn = bformat("./%s",ie);
  xmlXPathObjectPtr xpath_obj_plmn = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_nn);
  if (xpath_obj_plmn) {
    xmlNodeSetPtr nodes = xpath_obj_plmn->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));

      if (res) {
        uint8_t  codingscheme = 0;
        bstring xpath_expr = bformat("./%s",CODING_SCHEME_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&codingscheme, NULL);
        bdestroy_wrapper (&xpath_expr);
        networkname->codingscheme = codingscheme;
      }
      if (res) {
        uint8_t  addci       = 0;
        bstring xpath_expr = bformat("./%s",ADD_CI_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&addci, NULL);
        bdestroy_wrapper (&xpath_expr);
        networkname->addci = addci;
      }
      if (res) {
        uint8_t  numberofsparebitsinlastoctet = 0;
        bstring xpath_expr = bformat("./%s",NUMBER_OF_SPARE_BITS_IN_LAST_OCTET_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&numberofsparebitsinlastoctet, NULL);
        bdestroy_wrapper (&xpath_expr);
        networkname->numberofsparebitsinlastoctet = numberofsparebitsinlastoctet;
      }
      if (res) {
        char     network_name[NETWORK_NAME_IE_MAX_LENGTH+1];
        bstring xpath_expr = bformat("./%s",TEXT_STRING_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)&network_name, NULL);
        bdestroy_wrapper (&xpath_expr);
        networkname->textstring = bfromcstr(network_name);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
  }
  bdestroy_wrapper (&xpath_expr_nn);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------
void network_name_to_xml (const char * const ie, const network_name_t * const networkname, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, ie);
  XML_WRITE_FORMAT_ELEMENT(writer, CODING_SCHEME_ATTR_XML_STR, "%"PRIu8, networkname->codingscheme);
  XML_WRITE_FORMAT_ELEMENT(writer, ADD_CI_ATTR_XML_STR, "%"PRIu8, networkname->addci);
  XML_WRITE_FORMAT_ELEMENT(writer, NUMBER_OF_SPARE_BITS_IN_LAST_OCTET_ATTR_XML_STR, "%"PRIu8, networkname->numberofsparebitsinlastoctet);
  XML_WRITE_FORMAT_ELEMENT(writer, TEXT_STRING_ATTR_XML_STR, "%s", bdata(networkname->textstring));
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
// 10.5.3.8 Time Zone
//------------------------------------------------------------------------------
bool time_zone_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, time_zone_t * const timezone)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bstring xpath_expr = bformat("./%s",TIME_ZONE_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)timezone, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------
void time_zone_to_xml (const time_zone_t * const timezone, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, TIME_ZONE_IE_XML_STR, "%"PRIx8, *timezone);
}

//------------------------------------------------------------------------------
// 10.5.3.9 Time Zone and Time
//------------------------------------------------------------------------------
bool time_zone_and_time_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, time_zone_and_time_t * const timezoneandtime)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bool res = false;
  bstring xpath_expr_tz = bformat("./%s",TIME_ZONE_AND_TIME_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_tz);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));

      if (res) {
        bstring xpath_expr = bformat("./%s",YEAR_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&timezoneandtime->year, NULL);
        bdestroy_wrapper (&xpath_expr);
      }
      if (res) {
        bstring xpath_expr = bformat("./%s",MONTH_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&timezoneandtime->month, NULL);
        bdestroy_wrapper (&xpath_expr);
      }
      if (res) {
        bstring xpath_expr = bformat("./%s",DAY_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&timezoneandtime->day, NULL);
        bdestroy_wrapper (&xpath_expr);
      }
      if (res) {
        bstring xpath_expr = bformat("./%s",HOUR_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&timezoneandtime->hour, NULL);
        bdestroy_wrapper (&xpath_expr);
      }
      if (res) {
        bstring xpath_expr = bformat("./%s",MINUTE_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&timezoneandtime->minute, NULL);
        bdestroy_wrapper (&xpath_expr);
      }
      if (res) {
        bstring xpath_expr = bformat("./%s",SECOND_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&timezoneandtime->second, NULL);
        bdestroy_wrapper (&xpath_expr);
      }
      if (res) {
        res = time_zone_from_xml(xml_doc, xpath_ctx, &timezoneandtime->timezone);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
  }
  bdestroy_wrapper (&xpath_expr_tz);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------
void time_zone_and_time_to_xml (const time_zone_and_time_t * const timezoneandtime, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, TIME_ZONE_AND_TIME_IE_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, YEAR_ATTR_XML_STR, "%"PRIu8,   timezoneandtime->year);
  XML_WRITE_FORMAT_ELEMENT(writer, MONTH_ATTR_XML_STR, "%"PRIu8,  timezoneandtime->month);
  XML_WRITE_FORMAT_ELEMENT(writer, DAY_ATTR_XML_STR, "%"PRIu8,    timezoneandtime->day);
  XML_WRITE_FORMAT_ELEMENT(writer, HOUR_ATTR_XML_STR, "%"PRIu8,   timezoneandtime->hour);
  XML_WRITE_FORMAT_ELEMENT(writer, MINUTE_ATTR_XML_STR, "%"PRIu8, timezoneandtime->minute);
  XML_WRITE_FORMAT_ELEMENT(writer, SECOND_ATTR_XML_STR, "%"PRIu8, timezoneandtime->second);
  // ??? IE or ATTR
  time_zone_to_xml(&timezoneandtime->timezone, writer);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
// 10.5.3.12 Daylight Saving Time
//------------------------------------------------------------------------------
bool daylight_saving_time_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, daylight_saving_time_t * const daylightsavingtime)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bstring xpath_expr = bformat("./%s",DAYLIGHT_SAVING_TIME_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)daylightsavingtime, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------
void daylight_saving_time_to_xml (const daylight_saving_time_t * const daylightsavingtime, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, DAYLIGHT_SAVING_TIME_IE_XML_STR, "%"PRIu8, *daylightsavingtime);
}

//------------------------------------------------------------------------------
// 10.5.3.13 Emergency Number List
//------------------------------------------------------------------------------
bool emergency_number_list_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, emergency_number_list_t * const emergencynumberlist)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  // TODO...later
  OAILOG_FUNC_RETURN (LOG_UTIL, false);
}

//------------------------------------------------------------------------------
void emergency_number_list_to_xml (const emergency_number_list_t * const emergencynumberlist, xmlTextWriterPtr writer)
{
  const emergency_number_list_t *  e = emergencynumberlist;
  XML_WRITE_START_ELEMENT(writer, EMERGENCY_NUMBER_LIST_IE_XML_STR);
  while (e) {
    XML_WRITE_START_ELEMENT(writer, EMERGENCY_NUMBER_LIST_ITEM_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(writer, ADD_CI_ATTR_XML_STR, "0x%x", e->emergencyservicecategoryvalue);
    uint8_t digits[EMERGENCY_NUMBER_MAX_DIGITS+1];
    for (int i=0; i < EMERGENCY_NUMBER_MAX_DIGITS; i++) {
      if (e->number_digit[i] < 10) {
        digits[i] = e->number_digit[i] + 0x30;
      } else {
        digits[i] = 0;
        break;
      }
    }
    XML_WRITE_FORMAT_ELEMENT(writer, NUMBER_DIGITS_ATTR_XML_STR, "%s", digits);
    XML_WRITE_END_ELEMENT(writer);
    e = e->next;
  }
  XML_WRITE_END_ELEMENT(writer);
}

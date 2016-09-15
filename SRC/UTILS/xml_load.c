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
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "log.h"
#include "common_defs.h"
#include "conversions.h"
#include "xml_load.h"


//------------------------------------------------------------------------------
xmlXPathObjectPtr xml_find_nodes(xmlDocPtr const xml_doc, xmlXPathContextPtr  *xpath_ctx, bstring xpath_expr)
{
  xmlXPathObjectPtr   xpath_obj = NULL;

  if (!(*xpath_ctx)) {
    // Create xpath evaluation context
    *xpath_ctx = xmlXPathNewContext(xml_doc);
  }
  if (!(*xpath_ctx)) {
    OAILOG_ERROR (LOG_UTIL, "Error: unable to create new XPath context\n");
    return NULL;
  }

  /* Evaluate xpath expression */
  xpath_obj = xmlXPathEvalExpression((const xmlChar *)bdata(xpath_expr), *xpath_ctx);
  if(!xpath_obj) {
    OAILOG_ERROR (LOG_UTIL, "Error: unable to evaluate xpath expression \"%s\"\n", (const xmlChar *)bdata(xpath_expr));
    return NULL;
  }
  return xpath_obj;
}

//------------------------------------------------------------------------------
bool xml_load_tag(
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    bstring                       xpath_expr,
    msp_msg_load_callback_fn_t    loader_callback,
    void *                        container)
{
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    bool res = (*loader_callback)(xml_doc, container, nodes);
    xmlXPathFreeObject(xpath_obj);
    return res;
  }
  OAILOG_ERROR (LOG_UTIL, "Failed loading %s\n", bdata(xpath_expr));
  return false;
}

//------------------------------------------------------------------------------
bool xml_load_leaf_tag(
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    bstring                       xpath_expr,
    const char            * const scanf_format,
    void                  * const container,
    bstring               * failed_value)
{
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  bool res = false;
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {
      xmlChar *key = xmlNodeListGetString(xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
      int ret = sscanf((const char*)key, scanf_format, container);
      if (1 == ret) {
        res = true;
        OAILOG_TRACE (LOG_UTIL, "Found %s=%s\n", bdata(xpath_expr), key);
      } else if ((failed_value)) {
        *failed_value = bfromcstr((const char *)key);
        OAILOG_TRACE (LOG_UTIL, "Found %s=%s!\n", bdata(xpath_expr), key);
        // return false
      }
      xmlFree(key);
    }
    xmlXPathFreeObject(xpath_obj);
  }
  return res;
}
//------------------------------------------------------------------------------
bool xml_load_hex_stream_leaf_tag(
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    bstring                       xpath_expr,
    bstring               * const container)
{
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    bool res = false;
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {
      xmlChar *key = xmlNodeListGetString(xml_doc, nodes->nodeTab[0]->xmlChildrenNode, 1);
      if (key) {
        int len = strlen((const char*)key);
        uint8_t hex[len/2];
        int ret = ascii_to_hex ((uint8_t *) hex, (const char *)key);
        if (ret) {
          OAILOG_TRACE (LOG_UTIL, "Found %s=%s\n", bdata(xpath_expr), key);
          res = true;
          *container = blk2bstr(hex,len/2);
        }
        xmlFree(key);
      }
    }
    xmlXPathFreeObject(xpath_obj);
    return res;
  }
  OAILOG_TRACE (LOG_UTIL, "Warning: No result for searching %s\n", bdata(xpath_expr));
  return false;
}





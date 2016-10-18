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
#include "common_defs.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "3gpp_24.008_xml.h"
#include "xml_load.h"
#include "xml2_wrapper.h"

//******************************************************************************
// 10.5.7 GPRS Common information elements
//******************************************************************************

//------------------------------------------------------------------------------
// 10.5.7.3 GPRS Timer
//------------------------------------------------------------------------------
bool gprs_timer_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, const char * const ie, gprs_timer_t * const gprstimer)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr_gprs = bformat("./%s",ie);
  xmlXPathObjectPtr xpath_obj_gprs = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_gprs);
  if (xpath_obj_gprs) {
    xmlNodeSetPtr nodes = xpath_obj_gprs->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));
      if (res) {
        uint8_t  unit = 0;
        bstring xpath_expr = bformat("./%s",UNIT_IE_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&unit, NULL);
        bdestroy_wrapper (&xpath_expr);
        gprstimer->unit = unit;
      }
      if (res) {
        uint8_t  timervalue = 0;
        bstring xpath_expr = bformat("./%s",TIMER_VALUE_IE_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&timervalue, NULL);
        bdestroy_wrapper (&xpath_expr);
        gprstimer->timervalue = timervalue;
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj_gprs);
  }
  bdestroy_wrapper (&xpath_expr_gprs);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void gprs_timer_to_xml (const char * const ie_xml_str, const gprs_timer_t * const gprstimer, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, ie_xml_str);
  XML_WRITE_FORMAT_ELEMENT(writer, UNIT_IE_XML_STR, "%"PRIu8, gprstimer->unit);
  XML_WRITE_FORMAT_ELEMENT(writer, TIMER_VALUE_IE_XML_STR, "%"PRIu8, gprstimer->timervalue);
  XML_WRITE_END_ELEMENT(writer);
}

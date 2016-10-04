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

#include "dynamic_memory_check.h"
#include "log.h"
#include "assertions.h"
#include "common_defs.h"
#include "conversions.h"
#include "mme_scenario_player.h"
#include "sp_xml_compare.h"
#include "sp_common_xml.h"

#define ANY_ATTR_VALUE_XML_STR "ANY"

//------------------------------------------------------------------------------
static int sp_xml_compare_elements(scenario_t *scenario, xmlNode * received_node, xmlNode * expected_node)
{
    xmlNode *received_cur_node = NULL;
    xmlNode *expected_cur_node = NULL;
    int      res       = RETURNok;

    expected_cur_node = expected_node;
    for (received_cur_node = received_node; (received_cur_node) && (expected_cur_node); received_cur_node = received_cur_node->next) {
      if ((received_cur_node->type == XML_ELEMENT_NODE) && (received_cur_node->type == XML_ELEMENT_NODE)) {
        if (!xmlStrEqual(received_cur_node->name, expected_cur_node->name)) {
          OAILOG_WARNING (LOG_MME_SCENARIO_PLAYER, "Node name %s != %s\n",received_cur_node->name, expected_cur_node->name);
          res = RETURNerror;
        }
      } else if ((received_cur_node->type == XML_TEXT_NODE) && (expected_cur_node->type == XML_TEXT_NODE)) {
        if ((0 < xmlStrlen(received_cur_node->content)) &&  (0 < xmlStrlen(expected_cur_node->content))) {
          if (!xmlStrEqual(received_cur_node->content, expected_cur_node->content)) {
            if (!strcasecmp((const char*)expected_node->content, ANY_ATTR_VALUE_XML_STR)) {
              res = RETURNok;
              // may be a variable beginning with '$' or '#'
            } else if ('$' == expected_cur_node->content[0]) {
              res = sp_compare_string_value_with_var(scenario, (char *)&expected_cur_node->content[1], (char *)received_cur_node->content);
            } else if ('#' == expected_cur_node->content[0]) {
              res = sp_assign_value_to_var(scenario, (char *)&expected_cur_node->content[1], (char *)received_cur_node->content);
            } else {
              OAILOG_WARNING (LOG_MME_SCENARIO_PLAYER, "Compare received %s/%d vs expected %s/%d failed\n",
                  received_cur_node->content, xmlStrlen(received_cur_node->content),
                  expected_cur_node->content, xmlStrlen(expected_cur_node->content));
              res = RETURNerror;
            }
          }
        } else if (xmlStrlen(received_cur_node->content) != xmlStrlen(expected_cur_node->content)) {
          OAILOG_WARNING (LOG_MME_SCENARIO_PLAYER, "Compare len received %s/%d vs expected %s/%d failed\n",
              received_cur_node->content, xmlStrlen(received_cur_node->content),
              expected_cur_node->content, xmlStrlen(expected_cur_node->content));
          res = RETURNerror;
        }
      } else if (received_cur_node->type != expected_cur_node->type) {
        OAILOG_WARNING (LOG_MME_SCENARIO_PLAYER, "Compare XML nodes types received %d vs expected %d failed\n",
            received_cur_node->type, expected_cur_node->type);
        res = RETURNerror;
      }

      res = sp_xml_compare_elements(scenario, received_cur_node->children, expected_cur_node->children);
      if (RETURNerror == res) return RETURNerror;
      expected_cur_node = expected_cur_node->next;
    }
    if ((received_cur_node) || (expected_cur_node)) {
      OAILOG_WARNING (LOG_MME_SCENARIO_PLAYER, "Compare XML nodes failed: not same number of elements\n");
      res = RETURNerror;
    }
    return res;
}

//------------------------------------------------------------------------------
int sp_compare_xml_docs(scenario_t *scenario, xmlDoc  *received_doc, xmlDoc  *expected_doc)
{
  /*Get the root element node */
  xmlNode *root_received_element = xmlDocGetRootElement(received_doc);
  xmlNode *root_expected_element = xmlDocGetRootElement(expected_doc);

  int res = sp_xml_compare_elements(scenario, root_received_element, root_expected_element);
  if (RETURNok == res) {
    OAILOG_NOTICE(LOG_MME_SCENARIO_PLAYER, "Comparison of XML docs returned SUCCESS\n");
  } else {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Comparison of XML docs returned FAILURE\n");
  }
  return res;
}


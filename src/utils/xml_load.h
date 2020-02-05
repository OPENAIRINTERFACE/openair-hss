/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file xml_load.h
   \brief
   \author  Lionel GAUTHIER
   \date 2016
   \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_XML_LOAD_SEEN
#define FILE_XML_LOAD_SEEN

xmlXPathObjectPtr xml_find_nodes(xmlDocPtr const xml_doc,
                                 xmlXPathContextPtr* xpath_ctx,
                                 bstring xpath_expr);

typedef bool (*msp_msg_load_callback_fn_t)(xmlDocPtr const xml_doc,
                                           void* const container,
                                           xmlNodeSetPtr const nodes);

bool xml_load_tag(xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx,
                  bstring xpath_expr,
                  msp_msg_load_callback_fn_t loader_callback, void* container);

bool xml_load_leaf_tag(
    xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, bstring xpath_expr,
    const char* const scanf_format, void* const container,
    bstring* failed_value);  // if container could not be scanned, the put the
                             // scanned string into failed_value, else NULL
                             // (useful for XML vars custom syntax)

bool xml_load_hex_stream_leaf_tag(xmlDocPtr xml_doc,
                                  xmlXPathContextPtr xpath_ctx,
                                  bstring xpath_expr, bstring* const container);

#define NUM_FROM_XML_PROTOTYPE(name_lower)                                    \
  bool name_lower##_from_xml(xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, \
                             name_lower##_t* const name_lower,                \
                             bstring failed_result)

#define NUM_FROM_XML_GENERATE(name_lower, name_upper)                          \
  bool name_lower##_from_xml(xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx,  \
                             name_lower##_t* const name_lower,                 \
                             bstring failed_result) {                          \
    bstring xpath_expr = bformat("./%s", name_upper##_XML_STR);                \
    bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr,               \
                                 name_upper##_XML_SCAN_FMT, (void*)name_lower, \
                                 &failed_result);                              \
    bdestroy_wrapper(&xpath_expr);                                             \
    return res;                                                                \
  }

#endif

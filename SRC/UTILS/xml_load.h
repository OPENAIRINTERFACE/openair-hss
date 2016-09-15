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

#ifndef FILE_XML_LOAD_SEEN
#define FILE_XML_LOAD_SEEN

xmlXPathObjectPtr xml_find_nodes(xmlDocPtr const xml_doc, xmlXPathContextPtr  *xpath_ctx, bstring xpath_expr);

typedef bool (*msp_msg_load_callback_fn_t)(xmlDocPtr const xml_doc, void * const container, xmlNodeSetPtr const nodes);

bool xml_load_tag(
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    bstring                       xpath_expr,
    msp_msg_load_callback_fn_t    loader_callback,
    void *                        container);

bool xml_load_leaf_tag(
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    bstring                       xpath_expr,
    const char            * const scanf_format,
    void                  * const container,
    bstring               * failed_value); // if container could not be scanned, the put the scanned string into failed_value, else NULL (useful for XML vars custom syntax)

bool xml_load_hex_stream_leaf_tag(
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    bstring                       xpath_expr,
    bstring               * const container);

#define NUM_FROM_XML_PROTOTYPE(name_lower) \
bool name_lower##_from_xml (\
    xmlDocPtr                     xml_doc,\
    xmlXPathContextPtr            xpath_ctx,\
    name_lower##_t        * const name_lower,\
    bstring                       failed_result)

#define NUM_FROM_XML_GENERATE(name_lower, name_upper) \
bool name_lower##_from_xml (\
    xmlDocPtr                     xml_doc,\
    xmlXPathContextPtr            xpath_ctx,\
    name_lower##_t        * const name_lower,\
    bstring                       failed_result)\
{\
  bstring xpath_expr = bformat("./%s",name_upper##_IE_XML_STR);\
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, name_upper##_XML_SCAN_FMT, (void*)name_lower, &failed_result);\
  bdestroy_wrapper (&xpath_expr);\
  return res;\
}\

#endif


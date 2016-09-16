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
#include "3gpp_24.008_xml.h"
#include "xml_load.h"
#include "xml2_wrapper.h"
#include "log.h"
#include "conversions.h"

//******************************************************************************
// 10.5.4 Call control information elements.
//******************************************************************************

//------------------------------------------------------------------------------
// 10.5.4.32 Supported codec list
//------------------------------------------------------------------------------
bool supported_codec_list_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, supported_codec_list_t * supportedcodeclist)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  char hexascii[SUPPORTED_CODEC_LIST_IE_MAX_LENGTH*2]  = {0};
  uint8_t hex[SUPPORTED_CODEC_LIST_IE_MAX_LENGTH]      = {0};
  bstring xpath_expr = bformat("./%s",SUPPORTED_CODEC_LIST_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)hexascii, NULL);
  if (res) {
    int len = strlen(hexascii);
    if (!ascii_to_hex(hex, hexascii)) res = false;
    if (res) {
      *supportedcodeclist = blk2bstr((const void *)hex, len/2);
    }
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------
void supported_codec_list_to_xml (const supported_codec_list_t * const supportedcodeclist, xmlTextWriterPtr writer)
{
  XML_WRITE_HEX_ELEMENT(writer, SUPPORTED_CODEC_LIST_IE_XML_STR, bdata(*supportedcodeclist), blength(*supportedcodeclist));
}

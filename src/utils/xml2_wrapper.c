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

/*! \file xml2_wrapper.c
   \brief
   \author  Lionel GAUTHIER
   \date 2016
   \email: lionel.gauthier@eurecom.fr
*/

#include <stdbool.h>
#include <stdint.h>

#include <libxml/encoding.h>
#include "bstrlib.h"

#include "log.h"
#include "xml2_wrapper.h"

//------------------------------------------------------------------------------
// From http://xmlsoft.org/examples/testWriter.c, no License
xmlChar *xml_encoding_convert_input(const char *in, const char *encoding) {
  xmlChar *out;
  int ret;
  int size;
  int out_size;
  int temp;
  xmlCharEncodingHandlerPtr handler;

  if (in == 0) return 0;

  handler = xmlFindCharEncodingHandler(encoding);

  if (!handler) {
    OAILOG_ERROR(
        LOG_XML,
        "xml_encoding_convert_input: no encoding handler found for '%s'\n",
        encoding ? encoding : "");
    return 0;
  }

  size = (int)strlen(in) + 1;
  out_size = size * 2 - 1;
  out = (unsigned char *)xmlMalloc((size_t)out_size);

  if (out != 0) {
    temp = size - 1;
    ret = handler->input(out, &out_size, (const xmlChar *)in, &temp);
    if ((ret < 0) || (temp - size + 1)) {
      if (ret < 0) {
        OAILOG_ERROR(
            LOG_XML,
            "xml_encoding_convert_input: conversion wasn't successful.\n");
      } else {
        OAILOG_ERROR(LOG_XML,
                     "xml_encoding_convert_input: conversion wasn't "
                     "successful. converted: %i octets.\n",
                     temp);
      }

      xmlFree(out);
      out = 0;
    } else {
      out = (unsigned char *)xmlRealloc(out, out_size + 1);
      out[out_size] = 0; /*null terminating out */
    }
  } else {
    OAILOG_ERROR(LOG_XML, "xml_encoding_convert_input: no mem\n");
  }

  return out;
}

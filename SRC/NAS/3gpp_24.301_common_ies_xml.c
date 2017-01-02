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
#include "xml_load.h"
#include "3gpp_24.301_common_ies_xml.h"
#include "xml2_wrapper.h"



//------------------------------------------------------------------------------
NUM_FROM_XML_GENERATE(eps_bearer_context_status, EPS_BEARER_CONTEXT_STATUS);
//------------------------------------------------------------------------------
void eps_bearer_context_status_to_xml (eps_bearer_context_status_t * epsbearercontextstatus, xmlTextWriterPtr writer)
{
  //XML_WRITE_FORMAT_COMMENT(writer, "%s:"UINT8_TO_BINARY_FMT, EPS_BEARER_CONTEXT_STATUS_XML_STR, UINT8_TO_BINARY_ARG(((uint8_t*)epsbearercontextstatus)[0]));
  //XML_WRITE_FORMAT_COMMENT(writer, "%s:"UINT8_TO_BINARY_FMT, EPS_BEARER_CONTEXT_STATUS_XML_STR, UINT8_TO_BINARY_ARG(((uint8_t*)epsbearercontextstatus)[1]));
  XML_WRITE_FORMAT_ELEMENT(writer, EPS_BEARER_CONTEXT_STATUS_XML_STR, "0x%"PRIx16, *epsbearercontextstatus);
}

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
#include <pthread.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "3gpp_33.401.h"
#include "security_types.h"
#include "common_types.h"
#include "common_defs.h"
#include "mme_scenario_player.h"
#include "3gpp_36.331_xml.h"

#include "xml_load.h"
#include "xml2_wrapper.h"
#include "log.h"


//------------------------------------------------------------------------------
bool rrc_establishment_cause_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    rrc_establishment_cause_t * const cause)
{
  OAILOG_FUNC_IN (LOG_XML);
  bstring xpath_expr = bformat("./%s",RRC_ESTABLISHMENT_CAUSE_XML_STR);
  uint8_t val = 0;
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, &val, NULL);
  *cause = val;
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
void rrc_establishment_cause_to_xml (const rrc_establishment_cause_t * const cause, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, RRC_ESTABLISHMENT_CAUSE_XML_STR, "%"PRIu8, *cause);
}



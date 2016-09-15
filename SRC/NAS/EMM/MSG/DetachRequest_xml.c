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
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "hashtable.h"
#include "obj_hashtable.h"
#include "log.h"
#include "common_defs.h"
#include "mme_scenario_player.h"
#include "DetachRequest.h"
#include "DetachRequest_xml.h"

#include "../../../UTILS/xml_load.h"
#include "3gpp_24.008_xml.h"

//------------------------------------------------------------------------------
bool detach_request_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    detach_request_msg  * const detach_request)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  bool res = false;

  res = nas_key_set_identifier_from_xml (xml_doc, xpath_ctx, &detach_request->naskeysetidentifier);
  if (res) {res = detach_type_from_xml (xml_doc, xpath_ctx, &detach_request->detachtype);}
  if (res) {res = eps_mobile_identity_from_xml (xml_doc, xpath_ctx, &detach_request->gutiorimsi);}
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, res);
}

//------------------------------------------------------------------------------
int detach_request_to_xml (
  detach_request_msg * detach_request,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  nas_key_set_identifier_to_xml (&detach_request->naskeysetidentifier, writer);
  detach_type_to_xml (&detach_request->detachtype, writer);
  eps_mobile_identity_to_xml (&detach_request->gutiorimsi, writer);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

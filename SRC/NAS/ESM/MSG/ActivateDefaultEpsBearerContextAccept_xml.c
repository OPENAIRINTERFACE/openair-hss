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
#include "ActivateDefaultEpsBearerContextAccept.h"
#include "ActivateDefaultEpsBearerContextAccept_xml.h"
#include "3gpp_24.008_xml.h"

//------------------------------------------------------------------------------
bool activate_default_eps_bearer_context_accept_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    activate_default_eps_bearer_context_accept_msg * activate_default_eps_bearer_context_accept)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  memset(activate_default_eps_bearer_context_accept, 0, sizeof(*activate_default_eps_bearer_context_accept));
  bool res = false;
  res = protocol_configuration_options_from_xml (xml_doc, xpath_ctx, &activate_default_eps_bearer_context_accept->protocolconfigurationoptions, true);
  if (res) {
    activate_default_eps_bearer_context_accept->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, true);
}

//------------------------------------------------------------------------------
int activate_default_eps_bearer_context_accept_to_xml (
  activate_default_eps_bearer_context_accept_msg * activate_default_eps_bearer_context_accept,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  if ((activate_default_eps_bearer_context_accept->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT) {
    protocol_configuration_options_to_xml (&activate_default_eps_bearer_context_accept->protocolconfigurationoptions, writer, true);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
}

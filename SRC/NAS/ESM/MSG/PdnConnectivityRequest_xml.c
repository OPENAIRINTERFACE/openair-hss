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
#include "PdnConnectivityRequest.h"
#include "PdnConnectivityRequest_xml.h"
#include "3gpp_24.008_xml.h"

//------------------------------------------------------------------------------
bool pdn_connectivity_request_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    pdn_connectivity_request_msg * pdn_connectivity_request)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  memset(pdn_connectivity_request, 0, sizeof(*pdn_connectivity_request));
  bool res = false;

  res = pdn_type_from_xml (xml_doc, xpath_ctx, &pdn_connectivity_request->pdntype);
  if (res) {res = request_type_from_xml (xml_doc, xpath_ctx, &pdn_connectivity_request->requesttype);}

  if (res) {
    res = esm_information_transfer_flag_from_xml (xml_doc, xpath_ctx, &pdn_connectivity_request->esminformationtransferflag);
    if (res) {
      pdn_connectivity_request->presencemask |= PDN_CONNECTIVITY_REQUEST_ESM_INFORMATION_TRANSFER_FLAG_PRESENT;
    }

    res = access_point_name_from_xml (xml_doc, xpath_ctx, &pdn_connectivity_request->accesspointname);
    if (res) {
      pdn_connectivity_request->presencemask |= PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT;
    }

    res = protocol_configuration_options_from_xml (xml_doc, xpath_ctx, &pdn_connectivity_request->protocolconfigurationoptions, true);
    if (res) {
      pdn_connectivity_request->presencemask |= PDN_CONNECTIVITY_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, res);
}

//------------------------------------------------------------------------------
int pdn_connectivity_request_to_xml (
  pdn_connectivity_request_msg * pdn_connectivity_request,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  pdn_type_to_xml (&pdn_connectivity_request->pdntype, writer);
  request_type_to_xml (&pdn_connectivity_request->requesttype, writer);

  if ((pdn_connectivity_request->presencemask & PDN_CONNECTIVITY_REQUEST_ESM_INFORMATION_TRANSFER_FLAG_PRESENT)
      == PDN_CONNECTIVITY_REQUEST_ESM_INFORMATION_TRANSFER_FLAG_PRESENT) {
    esm_information_transfer_flag_to_xml (&pdn_connectivity_request->esminformationtransferflag, writer);
  }

  if ((pdn_connectivity_request->presencemask & PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT)
      == PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT) {
    access_point_name_to_xml (pdn_connectivity_request->accesspointname, writer);
  }

  if ((pdn_connectivity_request->presencemask & PDN_CONNECTIVITY_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT)
      == PDN_CONNECTIVITY_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT) {
    protocol_configuration_options_to_xml (&pdn_connectivity_request->protocolconfigurationoptions, writer, true);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
}

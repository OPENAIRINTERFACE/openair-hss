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

/*****************************************************************************
Source    3gpp_24.301_esm_msg_xml.c"
Author    Lionel GAUTHIER
EMM msg C struct to/from XML functions
*****************************************************************************/
#include <stdbool.h>
#include <stdint.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_24.301.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "security_types.h"
#include "common_defs.h"
#include "common_types.h"
#include "xml_load.h"
#include "3gpp_24.301_esm_msg_xml.h"
#include "3gpp_24.301_ies_xml.h"
#include "3gpp_24.301_esm_ies_xml.h"
#include "3gpp_36.401_xml.h"
#include "3gpp_24.007_xml.h"
#include "3gpp_24.008_xml.h"
#include "log.h"


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/
//------------------------------------------------------------------------------
bool activate_dedicated_eps_bearer_context_request_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    activate_dedicated_eps_bearer_context_request_msg * const activate_dedicated_eps_bearer_context_request)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;

  res = linked_eps_bearer_identity_from_xml (xml_doc, xpath_ctx, &activate_dedicated_eps_bearer_context_request->linkedepsbeareridentity, NULL);
  if (res) {res = eps_quality_of_service_from_xml (xml_doc, xpath_ctx, &activate_dedicated_eps_bearer_context_request->epsqos);}
  if (res) {res = traffic_flow_template_from_xml (xml_doc, xpath_ctx, &activate_dedicated_eps_bearer_context_request->tft);}

  activate_dedicated_eps_bearer_context_request->presencemask = 0;
  if (res) {
    res = linked_ti_from_xml (xml_doc, xpath_ctx, &activate_dedicated_eps_bearer_context_request->transactionidentifier);
    if (res) {
      activate_dedicated_eps_bearer_context_request->presencemask |= ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_PRESENT;
    }

    res = quality_of_service_from_xml (xml_doc, xpath_ctx, &activate_dedicated_eps_bearer_context_request->negotiatedqos);
    if (res) {
      activate_dedicated_eps_bearer_context_request->presencemask |= ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_PRESENT;
    }

    res = llc_service_access_point_identifier_from_xml (xml_doc, xpath_ctx, &activate_dedicated_eps_bearer_context_request->negotiatedllcsapi);
    if (res) {
      activate_dedicated_eps_bearer_context_request->presencemask |= ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT;
    }

    res = radio_priority_from_xml (xml_doc, xpath_ctx, &activate_dedicated_eps_bearer_context_request->radiopriority, NULL);
    if (res) {
      activate_dedicated_eps_bearer_context_request->presencemask |= ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT;
    }

    res = packet_flow_identifier_from_xml (xml_doc, xpath_ctx, &activate_dedicated_eps_bearer_context_request->packetflowidentifier);
    if (res) {
      activate_dedicated_eps_bearer_context_request->presencemask |= ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT;
    }

    res = protocol_configuration_options_from_xml (xml_doc, xpath_ctx, &activate_dedicated_eps_bearer_context_request->protocolconfigurationoptions, false);
    if (res) {
      activate_dedicated_eps_bearer_context_request->presencemask |= ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int activate_dedicated_eps_bearer_context_request_to_xml (
  activate_dedicated_eps_bearer_context_request_msg * activate_dedicated_eps_bearer_context_request,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  linked_eps_bearer_identity_to_xml (&activate_dedicated_eps_bearer_context_request->linkedepsbeareridentity, writer);
  eps_quality_of_service_to_xml (&activate_dedicated_eps_bearer_context_request->epsqos, writer);
  traffic_flow_template_to_xml (&activate_dedicated_eps_bearer_context_request->tft, writer);

  if ((activate_dedicated_eps_bearer_context_request->presencemask & ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_PRESENT)
      == ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_PRESENT) {
    linked_ti_to_xml (&activate_dedicated_eps_bearer_context_request->transactionidentifier, writer);
  }

  if ((activate_dedicated_eps_bearer_context_request->presencemask & ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_PRESENT)
      == ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_PRESENT) {
    quality_of_service_to_xml (&activate_dedicated_eps_bearer_context_request->negotiatedqos, writer);
  }

  if ((activate_dedicated_eps_bearer_context_request->presencemask & ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT)
      == ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT) {
    llc_service_access_point_identifier_to_xml (&activate_dedicated_eps_bearer_context_request->negotiatedllcsapi, writer);
  }

  if ((activate_dedicated_eps_bearer_context_request->presencemask & ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT)
      == ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT) {
    radio_priority_to_xml (&activate_dedicated_eps_bearer_context_request->radiopriority, writer);
  }

  if ((activate_dedicated_eps_bearer_context_request->presencemask & ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT)
      == ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT) {
    packet_flow_identifier_to_xml (&activate_dedicated_eps_bearer_context_request->packetflowidentifier, writer);
  }

  if ((activate_dedicated_eps_bearer_context_request->presencemask & ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT)
      == ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT) {
    protocol_configuration_options_to_xml (&activate_dedicated_eps_bearer_context_request->protocolconfigurationoptions, writer, false);
  }
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool activate_default_eps_bearer_context_accept_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    activate_default_eps_bearer_context_accept_msg * activate_default_eps_bearer_context_accept)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  res = protocol_configuration_options_from_xml (xml_doc, xpath_ctx, &activate_default_eps_bearer_context_accept->protocolconfigurationoptions, true);
  activate_default_eps_bearer_context_accept->presencemask = 0;
  if (res) {
    activate_default_eps_bearer_context_accept->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
  }
  OAILOG_FUNC_RETURN (LOG_XML, true);
}

//------------------------------------------------------------------------------
int activate_default_eps_bearer_context_accept_to_xml (
  activate_default_eps_bearer_context_accept_msg * activate_default_eps_bearer_context_accept,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);

  if ((activate_default_eps_bearer_context_accept->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT) {
    protocol_configuration_options_to_xml (&activate_default_eps_bearer_context_accept->protocolconfigurationoptions, writer, true);
  }
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool activate_default_eps_bearer_context_request_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    activate_default_eps_bearer_context_request_msg * activate_default_eps_bearer_context_request)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;

  res = eps_quality_of_service_from_xml (xml_doc, xpath_ctx, &activate_default_eps_bearer_context_request->epsqos);
  if (res) {res = access_point_name_from_xml (xml_doc, xpath_ctx, &activate_default_eps_bearer_context_request->accesspointname);}
  if (res) {res = pdn_address_from_xml (xml_doc, xpath_ctx, &activate_default_eps_bearer_context_request->pdnaddress);}

  activate_default_eps_bearer_context_request->presencemask = 0;
  if (res) {
    res = linked_ti_from_xml (xml_doc, xpath_ctx, &activate_default_eps_bearer_context_request->transactionidentifier);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_PRESENT;
    }

    res = quality_of_service_from_xml (xml_doc, xpath_ctx, &activate_default_eps_bearer_context_request->negotiatedqos);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_PRESENT;
    }

    res = llc_service_access_point_identifier_from_xml (xml_doc, xpath_ctx, &activate_default_eps_bearer_context_request->negotiatedllcsapi);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT;
    }

    res = radio_priority_from_xml (xml_doc, xpath_ctx, &activate_default_eps_bearer_context_request->radiopriority, NULL);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT;
    }

    res = packet_flow_identifier_from_xml (xml_doc, xpath_ctx, &activate_default_eps_bearer_context_request->packetflowidentifier);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT;
    }

    res = apn_aggregate_maximum_bit_rate_from_xml (xml_doc, xpath_ctx, &activate_default_eps_bearer_context_request->apnambr);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT;
    }

    res = esm_cause_from_xml (xml_doc, xpath_ctx, &activate_default_eps_bearer_context_request->esmcause, NULL);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_PRESENT;
    }

    res = protocol_configuration_options_from_xml (xml_doc, xpath_ctx, &activate_default_eps_bearer_context_request->protocolconfigurationoptions, false);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int activate_default_eps_bearer_context_request_to_xml (
  activate_default_eps_bearer_context_request_msg * activate_default_eps_bearer_context_request,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);

  eps_quality_of_service_to_xml (&activate_default_eps_bearer_context_request->epsqos, writer);

  access_point_name_to_xml (activate_default_eps_bearer_context_request->accesspointname, writer);

  pdn_address_to_xml (&activate_default_eps_bearer_context_request->pdnaddress, writer);

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_PRESENT) {
    linked_ti_to_xml (&activate_default_eps_bearer_context_request->transactionidentifier, writer);
  }

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_PRESENT) {
    quality_of_service_to_xml (&activate_default_eps_bearer_context_request->negotiatedqos, writer);
  }

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT) {
    llc_service_access_point_identifier_to_xml (&activate_default_eps_bearer_context_request->negotiatedllcsapi, writer);
  }

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT) {
    radio_priority_to_xml (&activate_default_eps_bearer_context_request->radiopriority, writer);
  }

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT) {
    packet_flow_identifier_to_xml (&activate_default_eps_bearer_context_request->packetflowidentifier, writer);
  }

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT) {
    apn_aggregate_maximum_bit_rate_to_xml (&activate_default_eps_bearer_context_request->apnambr, writer);
  }

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_PRESENT) {
    esm_cause_to_xml (&activate_default_eps_bearer_context_request->esmcause, writer);
  }

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT) {
    protocol_configuration_options_to_xml (&activate_default_eps_bearer_context_request->protocolconfigurationoptions, writer, false);
  }
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}

//------------------------------------------------------------------------------
bool bearer_resource_allocation_request_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    bearer_resource_allocation_request_msg * bearer_resource_allocation_request)
{
  OAILOG_FUNC_IN (LOG_XML);
  memset(bearer_resource_allocation_request, 0, sizeof(*bearer_resource_allocation_request));
  bool res = false;


  /*
   * Decoding mandatory fields
   */
  res = linked_eps_bearer_identity_from_xml (xml_doc, xpath_ctx, &bearer_resource_allocation_request->linkedepsbeareridentity, NULL);

  if (res) res = traffic_flow_template_from_xml (xml_doc, xpath_ctx, &bearer_resource_allocation_request->trafficflowaggregate);
  if (res) res = eps_quality_of_service_from_xml (xml_doc, xpath_ctx, &bearer_resource_allocation_request->requiredtrafficflowqos);

  if (res) {
    res = protocol_configuration_options_from_xml (xml_doc, xpath_ctx, &bearer_resource_allocation_request->protocolconfigurationoptions, true);
    if (res) {
      bearer_resource_allocation_request->presencemask |= BEARER_RESOURCE_ALLOCATION_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int bearer_resource_allocation_request_to_xml (
    bearer_resource_allocation_request_msg * bearer_resource_allocation_request,
    xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  linked_eps_bearer_identity_to_xml (&bearer_resource_allocation_request->linkedepsbeareridentity, writer);
  traffic_flow_template_to_xml (&bearer_resource_allocation_request->trafficflowaggregate, writer);
  eps_quality_of_service_to_xml (&bearer_resource_allocation_request->requiredtrafficflowqos, writer);

  if ((bearer_resource_allocation_request->presencemask & BEARER_RESOURCE_ALLOCATION_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT)
      == BEARER_RESOURCE_ALLOCATION_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT) {
    protocol_configuration_options_to_xml (&bearer_resource_allocation_request->protocolconfigurationoptions, writer, true);
  }
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}

//------------------------------------------------------------------------------
bool pdn_connectivity_request_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    pdn_connectivity_request_msg * pdn_connectivity_request)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;

  res = pdn_type_from_xml (xml_doc, xpath_ctx, &pdn_connectivity_request->pdntype);
  if (res) {res = request_type_from_xml (xml_doc, xpath_ctx, &pdn_connectivity_request->requesttype, NULL);}

  pdn_connectivity_request->presencemask = 0;
  if (res) {
    res = esm_information_transfer_flag_from_xml (xml_doc, xpath_ctx, &pdn_connectivity_request->esminformationtransferflag, NULL);
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
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int pdn_connectivity_request_to_xml (
  pdn_connectivity_request_msg * pdn_connectivity_request,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
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
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}

//------------------------------------------------------------------------------
bool esm_information_request_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    esm_information_request_msg * esm_information_request)
{
  OAILOG_FUNC_IN (LOG_XML);
  OAILOG_FUNC_RETURN (LOG_XML, true);
}

//------------------------------------------------------------------------------
int esm_information_request_to_xml (
  esm_information_request_msg * esm_information_request,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}

//------------------------------------------------------------------------------
bool esm_information_response_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    esm_information_response_msg * esm_information_response)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = true;
  esm_information_response->presencemask = 0;

  res = access_point_name_from_xml (xml_doc, xpath_ctx, &esm_information_response->accesspointname);
  if (res) {
    esm_information_response->presencemask |= ESM_INFORMATION_RESPONSE_ACCESS_POINT_NAME_PRESENT;
  }

  res = protocol_configuration_options_from_xml (xml_doc, xpath_ctx, &esm_information_response->protocolconfigurationoptions, true);
  if (res) {
    esm_information_response->presencemask |= ESM_INFORMATION_RESPONSE_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
  }
  res = true;
  OAILOG_FUNC_RETURN (LOG_XML, true);
}

//------------------------------------------------------------------------------
int esm_information_response_to_xml (
    esm_information_response_msg * esm_information_response,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  if ((esm_information_response->presencemask & ESM_INFORMATION_RESPONSE_ACCESS_POINT_NAME_PRESENT)
      == ESM_INFORMATION_RESPONSE_ACCESS_POINT_NAME_PRESENT) {
    access_point_name_to_xml (esm_information_response->accesspointname, writer);
  }

  if ((esm_information_response->presencemask & ESM_INFORMATION_RESPONSE_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT)
      == ESM_INFORMATION_RESPONSE_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT) {
    protocol_configuration_options_to_xml (&esm_information_response->protocolconfigurationoptions, writer, true);
  }
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}

//------------------------------------------------------------------------------
bool esm_msg_from_xml (
    xmlDocPtr                              xml_doc,
    xmlXPathContextPtr                     xpath_ctx,
    ESM_msg               * const esm_msg)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool                                    res = false;

  // First decode the EMM message header
  res = esm_msg_header_from_xml (xml_doc, xpath_ctx, &esm_msg->header);
  if (res) {
    switch (esm_msg->header.message_type) {
    case DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT:
      //res = deactivate_eps_bearer_context_accept_from_xml (xml_doc, xpath_ctx, &esm_msg->deactivate_eps_bearer_context_accept);
      break;

    case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT:
      //res = activate_dedicated_eps_bearer_context_accept_from_xml (xml_doc, xpath_ctx, &esm_msg->activate_dedicated_eps_bearer_context_accept);
      break;

    case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT:
      //res = activate_dedicated_eps_bearer_context_reject_from_xml (xml_doc, xpath_ctx, &esm_msg->activate_dedicated_eps_bearer_context_reject);
      break;

    case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST:
      res = activate_dedicated_eps_bearer_context_request_from_xml (xml_doc, xpath_ctx, &esm_msg->activate_dedicated_eps_bearer_context_request);
      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
      res = activate_default_eps_bearer_context_accept_from_xml (xml_doc, xpath_ctx, &esm_msg->activate_default_eps_bearer_context_accept);
      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT:
      //encode_result = activate_default_eps_bearer_context_reject_from_xml (&esm_msg->activate_default_eps_bearer_context_reject);
      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST:
      res = activate_default_eps_bearer_context_request_from_xml (xml_doc, xpath_ctx, &esm_msg->activate_default_eps_bearer_context_request);
      break;

    case BEARER_RESOURCE_ALLOCATION_REJECT:
      //res = bearer_resource_allocation_reject_from_xml (xml_doc, xpath_ctx, &esm_msg->bearer_resource_allocation_reject);
      break;

    case BEARER_RESOURCE_ALLOCATION_REQUEST:
      //res = bearer_resource_allocation_request_from_xml (xml_doc, xpath_ctx, &esm_msg->bearer_resource_allocation_request);
      break;

    case BEARER_RESOURCE_MODIFICATION_REJECT:
      //res = bearer_resource_modification_reject_from_xml (xml_doc, xpath_ctx, &esm_msg->bearer_resource_modification_reject);
      break;

    case BEARER_RESOURCE_MODIFICATION_REQUEST:
      //res = bearer_resource_modification_request_from_xml (xml_doc, xpath_ctx, &esm_msg->bearer_resource_modification_request);
      break;

    case DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST:
      //res = deactivate_eps_bearer_context_request_from_xml (xml_doc, xpath_ctx, &esm_msg->deactivate_eps_bearer_context_request);
      break;

    case ESM_INFORMATION_REQUEST:
      res = esm_information_request_from_xml (xml_doc, xpath_ctx, &esm_msg->esm_information_request);
      break;

    case ESM_INFORMATION_RESPONSE:
      res = esm_information_response_from_xml (xml_doc, xpath_ctx, &esm_msg->esm_information_response);
      break;

    case ESM_STATUS:
      //res = esm_status_from_xml (xml_doc, xpath_ctx, &esm_msg->esm_status);
      break;

    case MODIFY_EPS_BEARER_CONTEXT_ACCEPT:
      //res = modify_eps_bearer_context_accept_from_xml (xml_doc, xpath_ctx, &esm_msg->modify_eps_bearer_context_accept);
      break;

    case MODIFY_EPS_BEARER_CONTEXT_REJECT:
      //encode_result = modify_eps_bearer_context_reject_from_xml (xml_doc, xpath_ctx, &esm_msg->modify_eps_bearer_context_reject);
      break;

    case MODIFY_EPS_BEARER_CONTEXT_REQUEST:
      //res = modify_eps_bearer_context_request_from_xml (xml_doc, xpath_ctx, &esm_msg->modify_eps_bearer_context_request);
      break;

    case PDN_CONNECTIVITY_REJECT:
      //res = pdn_connectivity_reject_from_xml (xml_doc, xpath_ctx, &esm_msg->pdn_connectivity_reject);
      break;

    case PDN_CONNECTIVITY_REQUEST:
      res = pdn_connectivity_request_from_xml (xml_doc, xpath_ctx, &esm_msg->pdn_connectivity_request);
      break;

    case PDN_DISCONNECT_REQUEST:
      //res = pdn_disconnect_request_from_xml (xml_doc, xpath_ctx, &esm_msg->pdn_disconnect_request);
      break;

    case PDN_DISCONNECT_REJECT:
      //res = pdn_disconnect_reject_from_xml (xml_doc, xpath_ctx, &esm_msg->pdn_disconnect_reject);
      break;

    default:
      OAILOG_ERROR (LOG_XML, "ESM-MSG   - Unexpected message type: 0x%x\n", esm_msg->header.message_type);
      res = false;
      break;
    }
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

/****************************************************************************
 **                                                                        **
 ** Name:  esm_msg_to_xml()                                          **
 **                                                                        **
 ** Description: Encode EPS Session Management messages                    **
 **                                                                        **
 ** Inputs:  msg:   The ESM message structure to encode        **
 **      length:  Maximal capacity of the output buffer      **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of bytes in the buffer if data  **
 **       have been successfully encoded;            **
 **       A negative error code otherwise.           **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_msg_to_xml (
  ESM_msg * msg,
  uint8_t * buffer,
  uint32_t len,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  int                                     header_result = 0;
  int                                     decode_result = 0;
  int                                     encode_result = 0;
  //uint8_t                                *buffer_log = buffer;
  //int                                     down_link = 1;
  // First encode the ESM message header

  header_result = esm_msg_header_to_xml (&msg->header, buffer, len, writer);

  if (header_result < 0) {
    OAILOG_ERROR (LOG_XML, "ESM-MSG   - Failed to encode ESM message header " "(%d)\n", header_result);
    OAILOG_FUNC_RETURN (LOG_XML, header_result);
  }

  OAILOG_TRACE (LOG_XML, "ESM-MSG   - Encoded ESM message header " "(%d)\n", header_result);
  buffer += header_result;
  len -= header_result;

  switch (msg->header.message_type) {
  case DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT:
    //encode_result = deactivate_eps_bearer_context_accept_to_xml (&msg->deactivate_eps_bearer_context_accept, writer);
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT:
    //encode_result = activate_dedicated_eps_bearer_context_accept_to_xml (&msg->activate_dedicated_eps_bearer_context_accept, writer);
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT:
    //encode_result = activate_dedicated_eps_bearer_context_reject_to_xml (&msg->activate_dedicated_eps_bearer_context_reject, writer);
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST:
    decode_result = decode_activate_dedicated_eps_bearer_context_request (&msg->activate_dedicated_eps_bearer_context_request, buffer, len);
    encode_result = activate_dedicated_eps_bearer_context_request_to_xml (&msg->activate_dedicated_eps_bearer_context_request, writer);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
    decode_result = decode_activate_default_eps_bearer_context_accept (&msg->activate_default_eps_bearer_context_accept, buffer, len);
    encode_result = activate_default_eps_bearer_context_accept_to_xml (&msg->activate_default_eps_bearer_context_accept, writer);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT:
    //encode_result = activate_default_eps_bearer_context_reject_to_xml (&msg->activate_default_eps_bearer_context_reject, writer);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST:
    decode_result = decode_activate_default_eps_bearer_context_request (&msg->activate_default_eps_bearer_context_request, buffer, len);
    encode_result = activate_default_eps_bearer_context_request_to_xml (&msg->activate_default_eps_bearer_context_request, writer);
    break;

  case BEARER_RESOURCE_ALLOCATION_REJECT:
    //encode_result = bearer_resource_allocation_reject_to_xml (&msg->bearer_resource_allocation_reject, writer);
    break;

  case BEARER_RESOURCE_ALLOCATION_REQUEST:
    //encode_result = bearer_resource_allocation_request_to_xml (&msg->bearer_resource_allocation_request, writer);
    break;

  case BEARER_RESOURCE_MODIFICATION_REJECT:
    //encode_result = bearer_resource_modification_reject_to_xml (&msg->bearer_resource_modification_reject, writer);
    break;

  case BEARER_RESOURCE_MODIFICATION_REQUEST:
    //encode_result = bearer_resource_modification_request_to_xml (&msg->bearer_resource_modification_request, writer);
    break;

  case DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST:
    //encode_result = deactivate_eps_bearer_context_request_to_xml (&msg->deactivate_eps_bearer_context_request, writer);
    break;

  case ESM_INFORMATION_REQUEST:
    decode_result = decode_esm_information_request (&msg->esm_information_request, buffer, len);
    encode_result = esm_information_request_to_xml (&msg->esm_information_request, writer);
    break;

  case ESM_INFORMATION_RESPONSE:
    decode_result = decode_esm_information_response (&msg->esm_information_response, buffer, len);
    encode_result = esm_information_response_to_xml (&msg->esm_information_response, writer);
    break;

  case ESM_STATUS:
    //encode_result = esm_status_to_xml (&msg->esm_status, writer);
    break;

  case MODIFY_EPS_BEARER_CONTEXT_ACCEPT:
    //encode_result = modify_eps_bearer_context_accept_to_xml (&msg->modify_eps_bearer_context_accept, writer);
    break;

  case MODIFY_EPS_BEARER_CONTEXT_REJECT:
    //encode_result = modify_eps_bearer_context_reject_to_xml (&msg->modify_eps_bearer_context_reject, writer);
    break;

  case MODIFY_EPS_BEARER_CONTEXT_REQUEST:
    //encode_result = modify_eps_bearer_context_request_to_xml (&msg->modify_eps_bearer_context_request, writer);
    break;

  case PDN_CONNECTIVITY_REJECT:
    //encode_result = pdn_connectivity_reject_to_xml (&msg->pdn_connectivity_reject, writer);
    break;

  case PDN_DISCONNECT_REQUEST:
    //encode_result = pdn_disconnect_request_to_xml (&msg->pdn_disconnect_request, writer);
    break;

  case PDN_DISCONNECT_REJECT:
    //encode_result = pdn_disconnect_reject_to_xml (&msg->pdn_disconnect_reject, writer);
    break;

  case PDN_CONNECTIVITY_REQUEST:
    decode_result = decode_pdn_connectivity_request (&msg->pdn_connectivity_request, buffer, len);
    encode_result = pdn_connectivity_request_to_xml (&msg->pdn_connectivity_request, writer);
    break;

  default:
    OAILOG_ERROR (LOG_XML, "ESM-MSG   - Unexpected message type: 0x%x\n", msg->header.message_type);
    encode_result = TLV_WRONG_MESSAGE_TYPE;
    break;
  }

  if (encode_result < 0) {
    OAILOG_ERROR (LOG_XML, "ESM-MSG   - Failed to encode L3 ESM message 0x%x " "(%d)\n", msg->header.message_type, encode_result);
  }

  OAILOG_FUNC_RETURN (LOG_XML, header_result + decode_result);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

bool esm_msg_header_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    esm_msg_header_t          * const header)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool     res                           = false;
  ebi_t   eps_bearer_identity            = 0;
  eps_protocol_discriminator_t protocol_discriminator = 0;
  pti_t   procedure_transaction_identity = 0;
  uint8_t message_type                   = 0;

  res = eps_bearer_identity_from_xml(xml_doc, xpath_ctx, &eps_bearer_identity);
  header->eps_bearer_identity = eps_bearer_identity;
  if (res) {
    res = protocol_discriminator_from_xml(xml_doc, xpath_ctx, &protocol_discriminator);
    header->protocol_discriminator = protocol_discriminator;
  }
  if (res) {
    res = procedure_transaction_identity_from_xml(xml_doc, xpath_ctx, &procedure_transaction_identity);
    header->procedure_transaction_identity = procedure_transaction_identity;
  }
  if (res) {
    res = message_type_from_xml(xml_doc, xpath_ctx, &message_type);
    header->message_type = message_type;
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

/****************************************************************************
 **                                                                        **
 ** Name:  esm_msg_header_to_xml()                                  **
 **                                                                        **
 **    The protocol discriminator and the security header type   **
 **    have already been encoded.                                **
 **                                                                        **
 ** Inputs:  header:  The ESM message header to encode           **
 **      len:   Maximal capacity of the output buffer      **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of bytes in the buffer if data  **
 **       have been successfully encoded;            **
 **       A negative error code otherwise.           **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_msg_header_to_xml (
  esm_msg_header_t * header,
  uint8_t * buffer,
  const uint32_t len,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  int                                     header_result = 0;

  header_result= esm_msg_decode_header (header, buffer, len);

  if (header_result < 0) {
    OAILOG_ERROR (LOG_XML, "EMM-MSG   - Failed to decode ESM message header " "(%d)\n", header_result);
    OAILOG_FUNC_RETURN (LOG_XML, header_result);
  }

  //buffer += header_result;
  //len -= header_result;

  ebi_t ebi = header->eps_bearer_identity;
  eps_bearer_identity_to_xml(&ebi, writer);
  eps_protocol_discriminator_t pd = header->protocol_discriminator;
  protocol_discriminator_to_xml(&pd, writer);
  pti_t pti = header->procedure_transaction_identity;
  procedure_transaction_identity_to_xml(&pti, writer);
  message_type_to_xml(&header->message_type , writer);
  return sizeof(esm_msg_header_t);
}

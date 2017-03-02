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
#include "mme_scenario_player.h"
#include "xml_load.h"
#include "3gpp_24.301_ies_xml.h"
#include "3gpp_24.301_esm_msg_xml.h"
#include "sp_3gpp_24.301_esm_msg_xml.h"
#include "3gpp_24.301_esm_ies_xml.h"
#include "sp_3gpp_24.301_ies_xml.h"
#include "3gpp_24.301_esm_ies_xml.h"
#include "sp_3gpp_24.301_esm_ies_xml.h"
#include "3gpp_36.401_xml.h"
#include "3gpp_24.007_xml.h"
#include "3gpp_24.008_xml.h"
#include "sp_3gpp_24.007_xml.h"
#include "sp_3gpp_24.008_xml.h"
#include "log.h"


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/
//------------------------------------------------------------------------------
bool sp_activate_dedicated_eps_bearer_context_accept_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    activate_dedicated_eps_bearer_context_accept_msg * activate_dedicated_eps_bearer_context_accept)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;
  res = protocol_configuration_options_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_dedicated_eps_bearer_context_accept->protocolconfigurationoptions, true);
  if (res) {
    activate_dedicated_eps_bearer_context_accept->presencemask |= ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, true);
}

//------------------------------------------------------------------------------
bool sp_activate_dedicated_eps_bearer_context_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    activate_dedicated_eps_bearer_context_request_msg * const activate_dedicated_eps_bearer_context_request)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;

  res = sp_linked_eps_bearer_identity_from_xml (scenario, msg, &activate_dedicated_eps_bearer_context_request->linkedepsbeareridentity);
  if (res) {res = eps_quality_of_service_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_dedicated_eps_bearer_context_request->epsqos);}
  if (res) {res = traffic_flow_template_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_dedicated_eps_bearer_context_request->tft);}

  if (res) {
    res = linked_ti_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_dedicated_eps_bearer_context_request->transactionidentifier);
    if (res) {
      activate_dedicated_eps_bearer_context_request->presencemask |= ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_PRESENT;
    }

    res = quality_of_service_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_dedicated_eps_bearer_context_request->negotiatedqos);
    if (res) {
      activate_dedicated_eps_bearer_context_request->presencemask |= ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_PRESENT;
    }

    res = llc_service_access_point_identifier_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_dedicated_eps_bearer_context_request->negotiatedllcsapi);
    if (res) {
      activate_dedicated_eps_bearer_context_request->presencemask |= ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT;
    }

    res = radio_priority_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_dedicated_eps_bearer_context_request->radiopriority, NULL);
    if (res) {
      activate_dedicated_eps_bearer_context_request->presencemask |= ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT;
    }

    res = packet_flow_identifier_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_dedicated_eps_bearer_context_request->packetflowidentifier);
    if (res) {
      activate_dedicated_eps_bearer_context_request->presencemask |= ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT;
    }

    res = protocol_configuration_options_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_dedicated_eps_bearer_context_request->protocolconfigurationoptions, false);
    if (res) {
      activate_dedicated_eps_bearer_context_request->presencemask |= ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_activate_default_eps_bearer_context_accept_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    activate_default_eps_bearer_context_accept_msg * activate_default_eps_bearer_context_accept)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;
  res = protocol_configuration_options_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_default_eps_bearer_context_accept->protocolconfigurationoptions, true);
  if (res) {
    activate_default_eps_bearer_context_accept->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, true);
}

//------------------------------------------------------------------------------
bool sp_activate_default_eps_bearer_context_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    activate_default_eps_bearer_context_request_msg * activate_default_eps_bearer_context_request)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;

  res = eps_quality_of_service_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_default_eps_bearer_context_request->epsqos);
  if (res) {res = sp_access_point_name_from_xml (scenario, msg, &activate_default_eps_bearer_context_request->accesspointname);}
  if (res) {res = pdn_address_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_default_eps_bearer_context_request->pdnaddress);}

  if (res) {
    res = linked_ti_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_default_eps_bearer_context_request->transactionidentifier);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_PRESENT;
    }

    res = quality_of_service_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_default_eps_bearer_context_request->negotiatedqos);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_PRESENT;
    }

    res = llc_service_access_point_identifier_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_default_eps_bearer_context_request->negotiatedllcsapi);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT;
    }

    res = radio_priority_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_default_eps_bearer_context_request->radiopriority, NULL);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT;
    }

    res = packet_flow_identifier_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_default_eps_bearer_context_request->packetflowidentifier);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT;
    }

    res = apn_aggregate_maximum_bit_rate_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_default_eps_bearer_context_request->apnambr);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT;
    }

    res = esm_cause_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_default_eps_bearer_context_request->esmcause, NULL);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_PRESENT;
    }

    res = protocol_configuration_options_from_xml (msg->xml_doc, msg->xpath_ctx, &activate_default_eps_bearer_context_request->protocolconfigurationoptions, false);
    if (res) {
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_bearer_resource_allocation_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    bearer_resource_allocation_request_msg * bearer_resource_allocation_request)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;


  /*
   * Decoding mandatory fields
   */
  res = sp_linked_eps_bearer_identity_from_xml (scenario, msg, &bearer_resource_allocation_request->linkedepsbeareridentity);

  if (res) res = traffic_flow_template_from_xml (msg->xml_doc, msg->xpath_ctx, &bearer_resource_allocation_request->trafficflowaggregate);
  if (res) res = eps_quality_of_service_from_xml (msg->xml_doc, msg->xpath_ctx, &bearer_resource_allocation_request->requiredtrafficflowqos);

  if (res) {
    res = protocol_configuration_options_from_xml (msg->xml_doc, msg->xpath_ctx, &bearer_resource_allocation_request->protocolconfigurationoptions, true);
    if (res) {
      bearer_resource_allocation_request->presencemask |= BEARER_RESOURCE_ALLOCATION_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_pdn_connectivity_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    pdn_connectivity_request_msg * pdn_connectivity_request)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;

  res = pdn_type_from_xml (msg->xml_doc, msg->xpath_ctx, &pdn_connectivity_request->pdntype);
  if (res) {res = request_type_from_xml (msg->xml_doc, msg->xpath_ctx, &pdn_connectivity_request->requesttype, NULL);}

  if (res) {
    res = sp_esm_information_transfer_flag_from_xml (scenario, msg, &pdn_connectivity_request->esminformationtransferflag);
    if (res) {
      pdn_connectivity_request->presencemask |= PDN_CONNECTIVITY_REQUEST_ESM_INFORMATION_TRANSFER_FLAG_PRESENT;
    }

    res = sp_access_point_name_from_xml (scenario, msg, &pdn_connectivity_request->accesspointname);
    if (res) {
      pdn_connectivity_request->presencemask |= PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT;
    }

    res = protocol_configuration_options_from_xml (msg->xml_doc, msg->xpath_ctx, &pdn_connectivity_request->protocolconfigurationoptions, true);
    if (res) {
      pdn_connectivity_request->presencemask |= PDN_CONNECTIVITY_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_esm_information_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    esm_information_request_msg * esm_information_request)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, true);
}

//------------------------------------------------------------------------------
bool sp_esm_information_response_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    esm_information_response_msg * esm_information_response)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = true;

  res = sp_access_point_name_from_xml (scenario, msg, &esm_information_response->accesspointname);
  if (res) {
    esm_information_response->presencemask |= ESM_INFORMATION_RESPONSE_ACCESS_POINT_NAME_PRESENT;
  }

  res = protocol_configuration_options_from_xml (msg->xml_doc, msg->xpath_ctx, &esm_information_response->protocolconfigurationoptions, true);
  if (res) {
    esm_information_response->presencemask |= ESM_INFORMATION_RESPONSE_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
  }
  res = true;
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_esm_msg_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    ESM_msg               * const esm_msg)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool                                    res = false;

  // First decode the EMM message header
  res = sp_esm_msg_header_from_xml (scenario, msg, &esm_msg->header);
  if (res) {
    switch (esm_msg->header.message_type) {
    case DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT:
      //res = sp_deactivate_eps_bearer_context_accept_from_xml (scenario, msg, &esm_msg->deactivate_eps_bearer_context_accept);
      break;

    case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT:
      res = sp_activate_dedicated_eps_bearer_context_accept_from_xml (scenario, msg, &esm_msg->activate_dedicated_eps_bearer_context_accept);
      break;

    case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT:
      //res = sp_activate_dedicated_eps_bearer_context_reject_from_xml (scenario, msg, &esm_msg->activate_dedicated_eps_bearer_context_reject);
      break;

    case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST:
      res = sp_activate_dedicated_eps_bearer_context_request_from_xml (scenario, msg, &esm_msg->activate_dedicated_eps_bearer_context_request);
      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
      res = sp_activate_default_eps_bearer_context_accept_from_xml (scenario, msg, &esm_msg->activate_default_eps_bearer_context_accept);
      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT:
      //encode_result = sp_activate_default_eps_bearer_context_reject_from_xml (scenario, msg, &esm_msg->activate_default_eps_bearer_context_reject);
      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST:
      res = sp_activate_default_eps_bearer_context_request_from_xml (scenario, msg, &esm_msg->activate_default_eps_bearer_context_request);
      break;

    case BEARER_RESOURCE_ALLOCATION_REJECT:
      //res = sp_bearer_resource_allocation_reject_from_xml (scenario, msg, &esm_msg->bearer_resource_allocation_reject);
      break;

    case BEARER_RESOURCE_ALLOCATION_REQUEST:
      res = sp_bearer_resource_allocation_request_from_xml (scenario, msg, &esm_msg->bearer_resource_allocation_request);
      break;

    case BEARER_RESOURCE_MODIFICATION_REJECT:
      //res = sp_bearer_resource_modification_reject_from_xml (scenario, msg, &esm_msg->bearer_resource_modification_reject);
      break;

    case BEARER_RESOURCE_MODIFICATION_REQUEST:
      //res = sp_bearer_resource_modification_request_from_xml (scenario, msg, &esm_msg->bearer_resource_modification_request);
      break;

    case DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST:
      //res = sp_deactivate_eps_bearer_context_request_from_xml (scenario, msg, &esm_msg->deactivate_eps_bearer_context_request);
      break;

    case ESM_INFORMATION_REQUEST:
      res = sp_esm_information_request_from_xml (scenario, msg, &esm_msg->esm_information_request);
      break;

    case ESM_INFORMATION_RESPONSE:
      res = sp_esm_information_response_from_xml (scenario, msg, &esm_msg->esm_information_response);
      break;

    case ESM_STATUS:
      //res = sp_esm_status_from_xml (scenario, msg, &esm_msg->esm_status);
      break;

    case MODIFY_EPS_BEARER_CONTEXT_ACCEPT:
      //res = sp_modify_eps_bearer_context_accept_from_xml (scenario, msg, &esm_msg->modify_eps_bearer_context_accept);
      break;

    case MODIFY_EPS_BEARER_CONTEXT_REJECT:
      //encode_result = sp_modify_eps_bearer_context_reject_from_xml (scenario, msg, &esm_msg->modify_eps_bearer_context_reject);
      break;

    case MODIFY_EPS_BEARER_CONTEXT_REQUEST:
      //res = sp_modify_eps_bearer_context_request_from_xml (scenario, msg, &esm_msg->modify_eps_bearer_context_request);
      break;

    case PDN_CONNECTIVITY_REJECT:
      //res = sp_pdn_connectivity_reject_from_xml (scenario, msg, &esm_msg->pdn_connectivity_reject);
      break;

    case PDN_CONNECTIVITY_REQUEST:
      res = sp_pdn_connectivity_request_from_xml (scenario, msg, &esm_msg->pdn_connectivity_request);
      break;

    case PDN_DISCONNECT_REQUEST:
      //res = sp_pdn_disconnect_request_from_xml (scenario, msg, &esm_msg->pdn_disconnect_request);
      break;

    case PDN_DISCONNECT_REJECT:
      //res = pdn_disconnect_reject_from_xml (scenario, msg, &esm_msg->pdn_disconnect_reject);
      break;

    default:
      OAILOG_ERROR (LOG_MME_SCENARIO_PLAYER, "ESM-MSG   - Unexpected message type: 0x%x\n", esm_msg->header.message_type);
      res = false;
      break;
    }
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}


bool sp_esm_msg_header_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    esm_msg_header_t          * const header)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool     res                           = false;

  ebi_t   eps_bearer_identity            = 0;
  res = sp_ebi_from_xml(scenario, msg, &eps_bearer_identity);
  header->eps_bearer_identity = eps_bearer_identity;
  if (res) {
    eps_protocol_discriminator_t protocol_discriminator = 0;
    res = protocol_discriminator_from_xml(msg->xml_doc, msg->xpath_ctx, &protocol_discriminator);
    header->protocol_discriminator = (uint8_t)protocol_discriminator;
  }
  if (res) {
    pti_t   procedure_transaction_identity = 0;
    res = sp_pti_from_xml(scenario, msg, &procedure_transaction_identity);
    header->procedure_transaction_identity = procedure_transaction_identity;
  }
  if (res) {
    uint8_t message_type                   = 0;
    res = message_type_from_xml(msg->xml_doc, msg->xpath_ctx, &message_type);
    header->message_type = message_type;
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}


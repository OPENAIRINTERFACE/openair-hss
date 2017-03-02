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
Source    sp_3gpp_24.301_emm_msg_xml.c"
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
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "security_types.h"
#include "common_defs.h"
#include "common_types.h"
#include "mme_scenario_player.h"
#include "xml_load.h"
#include "sp_3gpp_24.301_emm_msg_xml.h"
#include "sp_3gpp_24.008_xml.h"

#include "3gpp_24.301_ies_xml.h"
#include "3gpp_24.301_emm_ies_xml.h"
#include "3gpp_24.301_common_ies_xml.h"
#include "sp_3gpp_24.301_emm_ies_xml.h"
#include "3gpp_36.401_xml.h"
#include "3gpp_24.007_xml.h"
#include "3gpp_24.008_xml.h"
#include "log.h"


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/
//------------------------------------------------------------------------------
bool sp_attach_accept_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    attach_accept_msg  * const attach_accept)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;

  res = eps_attach_result_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->epsattachresult, NULL);
  if (res) {res = gprs_timer_from_xml (msg->xml_doc, msg->xpath_ctx, GPRS_TIMER_T3412_XML_STR, &attach_accept->t3412value);}
  if (res) {res = tracking_area_identity_list_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->tailist);}
  if (res) {res = sp_esm_message_container_from_xml (scenario, msg, &attach_accept->esmmessagecontainer);}
  if (res) {
    res = sp_eps_mobile_identity_from_xml (scenario, msg, &attach_accept->guti);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_GUTI_PRESENT;
    }

    res = location_area_identification_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->locationareaidentification);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_LOCATION_AREA_IDENTIFICATION_PRESENT;
    }

    res = mobile_identity_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->msidentity);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_MS_IDENTITY_PRESENT;
    }

    res = emm_cause_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->emmcause, NULL);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_EMM_CAUSE_PRESENT;
    }

    res = gprs_timer_from_xml (msg->xml_doc, msg->xpath_ctx, GPRS_TIMER_T3402_XML_STR, &attach_accept->t3402value);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_T3402_VALUE_PRESENT;
    }

    res = gprs_timer_from_xml (msg->xml_doc, msg->xpath_ctx, GPRS_TIMER_T3423_XML_STR, &attach_accept->t3423value);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_T3423_VALUE_PRESENT;
    }

    res = plmn_list_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->equivalentplmns);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_EQUIVALENT_PLMNS_PRESENT;
    }

    res = emergency_number_list_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->emergencynumberlist);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_EMERGENCY_NUMBER_LIST_PRESENT;
    }

    res = eps_network_feature_support_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->epsnetworkfeaturesupport, NULL);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_EPS_NETWORK_FEATURE_SUPPORT_PRESENT;
    }

    res = additional_update_result_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->additionalupdateresult, NULL);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_ADDITIONAL_UPDATE_RESULT_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}


//------------------------------------------------------------------------------
bool sp_attach_complete_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    attach_complete_msg * const attach_complete)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = sp_esm_message_container_from_xml (scenario, msg, &attach_complete->esmmessagecontainer);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_attach_reject_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    attach_reject_msg  * const attach_reject)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;

  res = sp_esm_message_container_from_xml (scenario, msg, &attach_reject->esmmessagecontainer);
  if (res) {
    attach_reject->presencemask |= ATTACH_REJECT_ESM_MESSAGE_CONTAINER_PRESENT;
  }
  res = true; // esm container optional
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_attach_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    attach_request_msg * const attach_request)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;

  res = nas_key_set_identifier_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->naskeysetidentifier);
  if (res) {res = eps_attach_type_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->epsattachtype, NULL);}
  if (res) {res = sp_eps_mobile_identity_from_xml (scenario, msg, &attach_request->oldgutiorimsi);}
  if (res) {res = ue_network_capability_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->uenetworkcapability);}
  if (res) {res = sp_esm_message_container_from_xml (scenario, msg, &attach_request->esmmessagecontainer);}
  if (res) {
    res = p_tmsi_signature_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->oldptmsisignature);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_OLD_PTMSI_SIGNATURE_PRESENT;
    }

    res = sp_eps_mobile_identity_from_xml (scenario, msg, &attach_request->additionalguti);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_ADDITIONAL_GUTI_PRESENT;
    }

    res = tracking_area_identity_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->lastvisitedregisteredtai);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_LAST_VISITED_REGISTERED_TAI_PRESENT;
    }

    res = drx_parameter_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->drxparameter);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_DRX_PARAMETER_PRESENT;
    }

    res = ms_network_capability_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->msnetworkcapability);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_MS_NETWORK_CAPABILITY_PRESENT;
    }

    res = location_area_identification_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->oldlocationareaidentification);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_OLD_LOCATION_AREA_IDENTIFICATION_PRESENT;
    }

    res = tmsi_status_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->tmsistatus);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_TMSI_STATUS_PRESENT;
    }

    res = mobile_station_classmark_2_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->mobilestationclassmark2);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_MOBILE_STATION_CLASSMARK_2_PRESENT;
    }

    res = mobile_station_classmark_3_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->mobilestationclassmark3);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_MOBILE_STATION_CLASSMARK_3_PRESENT;
    }

    res = supported_codec_list_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->supportedcodecs);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_SUPPORTED_CODECS_PRESENT;
    }

    res = additional_update_type_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->additionalupdatetype, NULL);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_ADDITIONAL_UPDATE_TYPE_PRESENT;
    }

    res = guti_type_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->oldgutitype);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_OLD_GUTI_TYPE_PRESENT;
    }

    res = voice_domain_preference_and_ue_usage_setting_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->voicedomainpreferenceandueusagesetting);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_VOICE_DOMAIN_PREFERENCE_AND_UE_USAGE_SETTING_PRESENT;
    }

    res = ms_network_feature_support_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->msnetworkfeaturesupport);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_MS_NETWORK_FEATURE_SUPPORT_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_authentication_failure_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    authentication_failure_msg  * const authentication_failure)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = sp_emm_cause_from_xml (scenario, msg, &authentication_failure->emmcause);
  if (res) {
    bstring xpath_expr = bformat("./%s",AUTHENTICATION_FAILURE_PARAMETER_XML_STR);
    bool res = sp_xml_load_hex_stream_leaf_tag(scenario, msg, xpath_expr, &authentication_failure->authenticationfailureparameter);
    bdestroy_wrapper (&xpath_expr);
    if (res) {
      authentication_failure->presencemask |= AUTHENTICATION_FAILURE_AUTHENTICATION_FAILURE_PARAMETER_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_authentication_reject_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    authentication_reject_msg  * const authentication_reject)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  // NOTHING TODO
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, true);
}


//------------------------------------------------------------------------------
bool sp_authentication_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    authentication_request_msg  * const authentication_request)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = nas_key_set_identifier_from_xml (msg->xml_doc, msg->xpath_ctx, &authentication_request->naskeysetidentifierasme);
  if (res) {
    bstring xpath_expr = bformat("./%s",AUTHENTICATION_PARAMETER_RAND_XML_STR);
    res = sp_xml_load_hex_stream_leaf_tag(scenario, msg, xpath_expr, &authentication_request->authenticationparameterrand);
    bdestroy_wrapper (&xpath_expr);
  }
  if (res) {
    bstring xpath_expr = bformat("./%s",AUTHENTICATION_PARAMETER_AUTN_XML_STR);
    res = sp_xml_load_hex_stream_leaf_tag(scenario, msg, xpath_expr, &authentication_request->authenticationparameterautn);
    bdestroy_wrapper (&xpath_expr);
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_authentication_response_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    authentication_response_msg  * const authentication_response)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bstring xpath_expr = bformat("./%s",AUTHENTICATION_RESPONSE_PARAMETER_XML_STR);
  bool res = sp_xml_load_hex_stream_leaf_tag(scenario, msg, xpath_expr, &authentication_response->authenticationresponseparameter);
  bdestroy_wrapper (&xpath_expr);

  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_detach_accept_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    detach_accept_msg  * const detach_accept)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  // NOTHING TODO
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, true);
}

//------------------------------------------------------------------------------
bool sp_detach_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    detach_request_msg  * const detach_request)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;

  res = nas_key_set_identifier_from_xml (msg->xml_doc, msg->xpath_ctx, &detach_request->naskeysetidentifier);
  if (res) {res = detach_type_from_xml (msg->xml_doc, msg->xpath_ctx, &detach_request->detachtype);}
  if (res) {res = sp_eps_mobile_identity_from_xml (scenario, msg, &detach_request->gutiorimsi);}
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_downlink_nas_transport_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    downlink_nas_transport_msg   * const downlink_nas_transport)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = sp_nas_message_container_from_xml (scenario, msg, &downlink_nas_transport->nasmessagecontainer);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_emm_information_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    emm_information_msg * const emm_information)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  if (network_name_from_xml (msg->xml_doc, msg->xpath_ctx, FULL_NETWORK_NAME_XML_STR, &emm_information->fullnamefornetwork)) {
    emm_information->presencemask |= EMM_INFORMATION_FULL_NAME_FOR_NETWORK_PRESENT;
  }

  if (network_name_from_xml (msg->xml_doc, msg->xpath_ctx, SHORT_NETWORK_NAME_XML_STR, &emm_information->shortnamefornetwork)) {
    emm_information->presencemask |= EMM_INFORMATION_SHORT_NAME_FOR_NETWORK_PRESENT;
  }

  if (time_zone_from_xml (msg->xml_doc, msg->xpath_ctx, &emm_information->localtimezone)) {
    emm_information->presencemask |= EMM_INFORMATION_LOCAL_TIME_ZONE_PRESENT;
  }

  if (time_zone_and_time_from_xml (msg->xml_doc, msg->xpath_ctx, &emm_information->universaltimeandlocaltimezone)) {
    emm_information->presencemask |= EMM_INFORMATION_UNIVERSAL_TIME_AND_LOCAL_TIME_ZONE_PRESENT;
  }

  if (daylight_saving_time_from_xml (msg->xml_doc, msg->xpath_ctx, &emm_information->networkdaylightsavingtime)) {
    emm_information->presencemask |= EMM_INFORMATION_NETWORK_DAYLIGHT_SAVING_TIME_PRESENT;
  }
  // no mandatory field
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, true);
}

//------------------------------------------------------------------------------
bool sp_emm_status_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    emm_status_msg * const emm_status)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = emm_cause_from_xml (msg->xml_doc, msg->xpath_ctx, &emm_status->emmcause, NULL);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_identity_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    identity_request_msg * const identity_request)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = identity_type_2_from_xml (msg->xml_doc, msg->xpath_ctx, &identity_request->identitytype);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_identity_response_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    identity_response_msg * const identity_response)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = sp_mobile_identity_from_xml (scenario, msg, &identity_response->mobileidentity);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_security_mode_command_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    security_mode_command_msg * const security_mode_command)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = false;

  res = sp_nas_security_algorithms_from_xml (scenario, msg, &security_mode_command->selectednassecurityalgorithms);
  if (res) {res = nas_key_set_identifier_from_xml (msg->xml_doc, msg->xpath_ctx, &security_mode_command->naskeysetidentifier);}
  if (res) {res = ue_security_capability_from_xml (msg->xml_doc, msg->xpath_ctx, &security_mode_command->replayeduesecuritycapabilities);}
  if (res) {
    res = imeisv_request_from_xml (msg->xml_doc, msg->xpath_ctx, &security_mode_command->imeisvrequest);
    if (res) {
      security_mode_command->presencemask |= SECURITY_MODE_COMMAND_IMEISV_REQUEST_PRESENT;
    }

    res = nonce_from_xml (msg->xml_doc, msg->xpath_ctx, REPLAYED_NONCE_UE_XML_STR, &security_mode_command->replayednonceue);
    if (res) {
      security_mode_command->presencemask |= SECURITY_MODE_COMMAND_REPLAYED_NONCEUE_PRESENT;
    }

    res = nonce_from_xml (msg->xml_doc, msg->xpath_ctx, NONCE_MME_XML_STR, &security_mode_command->noncemme);
    if (res) {
      security_mode_command->presencemask |= SECURITY_MODE_COMMAND_NONCEMME_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_security_mode_complete_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    security_mode_complete_msg * const security_mode_complete)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = mobile_identity_from_xml (msg->xml_doc, msg->xpath_ctx, &security_mode_complete->imeisv);
  if (res) {
    security_mode_complete->presencemask |= SECURITY_MODE_COMPLETE_IMEISV_PRESENT;
  }
  res = true;
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}


//------------------------------------------------------------------------------
bool sp_security_mode_reject_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    security_mode_reject_msg * const security_mode_reject)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = emm_cause_from_xml (msg->xml_doc, msg->xpath_ctx, &security_mode_reject->emmcause, NULL);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
int sp_service_reject_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
  service_reject_msg * service_reject)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, RETURNok);
}

//------------------------------------------------------------------------------
int sp_service_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
  service_request_msg * service_request)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, RETURNok);
}

//------------------------------------------------------------------------------
int sp_tracking_area_update_accept_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
  tracking_area_update_accept_msg * tracking_area_update_accept)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, RETURNok);
}

//------------------------------------------------------------------------------
int sp_tracking_area_update_complete_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
  tracking_area_update_complete_msg * tracking_area_update_complete)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, RETURNok);
}

//------------------------------------------------------------------------------
int sp_tracking_area_update_reject_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    tracking_area_update_reject_msg * tracking_area_update_reject)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, RETURNok);
}

//------------------------------------------------------------------------------
int sp_tracking_area_update_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    tracking_area_update_request_msg * tracking_area_update_request)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, RETURNok);
}

//------------------------------------------------------------------------------
bool sp_uplink_nas_transport_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    uplink_nas_transport_msg     * const uplink_nas_transport)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool res = sp_nas_message_container_from_xml (scenario, msg, &uplink_nas_transport->nasmessagecontainer);
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}


//------------------------------------------------------------------------------
bool sp_emm_msg_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    EMM_msg                     * emm_msg)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool                                    res = false;

  // First decode the EMM message header
  res = sp_emm_msg_header_from_xml (scenario, msg, &emm_msg->header);
  if (res) {
    switch (emm_msg->header.message_type) {
    case ATTACH_ACCEPT:
      res = sp_attach_accept_from_xml (scenario, msg, &emm_msg->attach_accept);
      break;

    case ATTACH_COMPLETE:
      res = sp_attach_complete_from_xml (scenario, msg, &emm_msg->attach_complete);
      break;

    case ATTACH_REJECT:
      res = sp_attach_reject_from_xml (scenario, msg, &emm_msg->attach_reject);
      break;

    case ATTACH_REQUEST:
      res = sp_attach_request_from_xml (scenario, msg, &emm_msg->attach_request);
      break;

    case AUTHENTICATION_FAILURE:
      res = sp_authentication_failure_from_xml (scenario, msg, &emm_msg->authentication_failure);
      break;

    case AUTHENTICATION_REJECT:
      res = sp_authentication_reject_from_xml (scenario, msg, &emm_msg->authentication_reject);
      break;

    case AUTHENTICATION_REQUEST:
      res = sp_authentication_request_from_xml (scenario, msg, &emm_msg->authentication_request);
      break;

    case AUTHENTICATION_RESPONSE:
      res = sp_authentication_response_from_xml (scenario, msg, &emm_msg->authentication_response);
      break;

    case CS_SERVICE_NOTIFICATION:
      //res = sp_cs_service_notification_from_xml (scenario, msg, &emm_msg->cs_service_notification);
      break;

    case DETACH_ACCEPT:
      res = sp_detach_accept_from_xml (scenario, msg, &emm_msg->detach_accept);
      break;

    case DETACH_REQUEST:
      res = sp_detach_request_from_xml (scenario, msg, &emm_msg->detach_request);
      break;

    case DOWNLINK_NAS_TRANSPORT:
      res = sp_downlink_nas_transport_from_xml (scenario, msg, &emm_msg->downlink_nas_transport);
      break;

    case EMM_INFORMATION:
      res = sp_emm_information_from_xml (scenario, msg, &emm_msg->emm_information);
      break;

    case EMM_STATUS:
      res = sp_emm_status_from_xml (scenario, msg, &emm_msg->emm_status);
      break;

    case EXTENDED_SERVICE_REQUEST:
      //res = sp_extended_service_request_from_xml (scenario, msg, &emm_msg->extended_service_request);
      break;

    case GUTI_REALLOCATION_COMMAND:
      //res = sp_guti_reallocation_command_from_xml (scenario, msg, &emm_msg->guti_reallocation_command);
      break;

    case GUTI_REALLOCATION_COMPLETE:
      //res = sp_guti_reallocation_complete_from_xml (scenario, msg, &emm_msg->guti_reallocation_complete);
      break;

    case IDENTITY_REQUEST:
      res = sp_identity_request_from_xml (scenario, msg, &emm_msg->identity_request);
      break;

    case IDENTITY_RESPONSE:
      res = sp_identity_response_from_xml (scenario, msg, &emm_msg->identity_response);
      break;

    case SECURITY_MODE_COMMAND:
      res = sp_security_mode_command_from_xml (scenario, msg, &emm_msg->security_mode_command);
      break;

    case SECURITY_MODE_COMPLETE:
      res = sp_security_mode_complete_from_xml (scenario, msg, &emm_msg->security_mode_complete);
      break;

    case SECURITY_MODE_REJECT:
      res = sp_security_mode_reject_from_xml (scenario, msg, &emm_msg->security_mode_reject);
      break;

    case SERVICE_REJECT:
      res = sp_service_reject_from_xml (scenario, msg, &emm_msg->service_reject);
      break;

    case TRACKING_AREA_UPDATE_ACCEPT:
      res = sp_tracking_area_update_accept_from_xml (scenario, msg, &emm_msg->tracking_area_update_accept);
      break;

    case TRACKING_AREA_UPDATE_COMPLETE:
      res = sp_tracking_area_update_complete_from_xml (scenario, msg, &emm_msg->tracking_area_update_complete);
      break;

    case TRACKING_AREA_UPDATE_REJECT:
      res = sp_tracking_area_update_reject_from_xml (scenario, msg, &emm_msg->tracking_area_update_reject);
      break;

    case TRACKING_AREA_UPDATE_REQUEST:
      res = sp_tracking_area_update_request_from_xml (scenario, msg, &emm_msg->tracking_area_update_request);
      break;

    case UPLINK_NAS_TRANSPORT:
      res = sp_uplink_nas_transport_from_xml (scenario, msg, &emm_msg->uplink_nas_transport);
      break;

    default:
      OAILOG_ERROR (LOG_MME_SCENARIO_PLAYER, "EMM-MSG   - Unexpected message type: 0x%x", emm_msg->header.message_type);
      res = false;
      // TODO: Handle not standard layer 3 messages: SERVICE_REQUEST
    }
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}

//------------------------------------------------------------------------------
bool sp_emm_msg_header_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    emm_msg_header_t * header)
{
  OAILOG_FUNC_IN (LOG_MME_SCENARIO_PLAYER);
  bool                         res = false;
  security_header_type_t       sht = 0;
  eps_protocol_discriminator_t pd = 0;
  message_type_t               message_type = 0;

  res = security_header_type_from_xml(msg->xml_doc, msg->xpath_ctx, &sht, NULL);
  if (res) {
    header->security_header_type = sht;
    res = protocol_discriminator_from_xml(msg->xml_doc, msg->xpath_ctx, &pd);
  }
  if (res) {
    header->protocol_discriminator = pd;
    res = message_type_from_xml(msg->xml_doc, msg->xpath_ctx, &message_type);
  }
  if (res) {
    header->message_type = message_type;
  }
  OAILOG_FUNC_RETURN (LOG_MME_SCENARIO_PLAYER, res);
}


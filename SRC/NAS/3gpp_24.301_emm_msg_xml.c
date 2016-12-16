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
Source    3gpp_24.301_emm_msg_xml.c"
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
#include "3gpp_24.301_emm_msg_xml.h"

#include "xml_load.h"
#include "3gpp_24.301_ies_xml.h"
#include "3gpp_24.301_emm_ies_xml.h"
#include "3gpp_24.301_common_ies_xml.h"
#include "3gpp_24.301_ies_xml.h"
#include "3gpp_36.401_xml.h"
#include "3gpp_24.007_xml.h"
#include "3gpp_24.008_xml.h"
#include "log.h"


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/
//------------------------------------------------------------------------------
bool attach_accept_from_xml (
    xmlDocPtr           xml_doc,
    xmlXPathContextPtr  xpath_ctx,
    attach_accept_msg  * const attach_accept)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;

  res = eps_attach_result_from_xml (xml_doc, xpath_ctx, &attach_accept->epsattachresult, NULL);
  if (res) {res = gprs_timer_from_xml (xml_doc, xpath_ctx, GPRS_TIMER_T3412_IE_XML_STR, &attach_accept->t3412value);}
  if (res) {res = tracking_area_identity_list_from_xml (xml_doc, xpath_ctx, &attach_accept->tailist);}
  if (res) {res = esm_message_container_from_xml (xml_doc, xpath_ctx, &attach_accept->esmmessagecontainer);}

  attach_accept->presencemask = 0;
  if (res) {
    res = eps_mobile_identity_from_xml (xml_doc, xpath_ctx, &attach_accept->guti);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_GUTI_PRESENT;
    }

    res = location_area_identification_from_xml (xml_doc, xpath_ctx, &attach_accept->locationareaidentification);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_LOCATION_AREA_IDENTIFICATION_PRESENT;
    }

    res = mobile_identity_from_xml (xml_doc, xpath_ctx, &attach_accept->msidentity);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_MS_IDENTITY_PRESENT;
    }

    res = emm_cause_from_xml (xml_doc, xpath_ctx, &attach_accept->emmcause, NULL);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_EMM_CAUSE_PRESENT;
    }

    res = gprs_timer_from_xml (xml_doc, xpath_ctx, GPRS_TIMER_T3402_IE_XML_STR, &attach_accept->t3402value);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_T3402_VALUE_PRESENT;
    }

    res = gprs_timer_from_xml (xml_doc, xpath_ctx, GPRS_TIMER_T3423_IE_XML_STR, &attach_accept->t3423value);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_T3423_VALUE_PRESENT;
    }

    res = plmn_list_from_xml (xml_doc, xpath_ctx, &attach_accept->equivalentplmns);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_EQUIVALENT_PLMNS_PRESENT;
    }

    res = emergency_number_list_from_xml (xml_doc, xpath_ctx, &attach_accept->emergencynumberlist);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_EMERGENCY_NUMBER_LIST_PRESENT;
    }

    res = eps_network_feature_support_from_xml (xml_doc, xpath_ctx, &attach_accept->epsnetworkfeaturesupport, NULL);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_EPS_NETWORK_FEATURE_SUPPORT_PRESENT;
    }

    res = additional_update_result_from_xml (xml_doc, xpath_ctx, &attach_accept->additionalupdateresult, NULL);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_ADDITIONAL_UPDATE_RESULT_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int attach_accept_to_xml (
  attach_accept_msg * attach_accept,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  eps_attach_result_to_xml (&attach_accept->epsattachresult, writer);

  gprs_timer_to_xml (GPRS_TIMER_T3412_IE_XML_STR, &attach_accept->t3412value, writer);

  tracking_area_identity_list_to_xml (&attach_accept->tailist, writer);

  esm_message_container_to_xml (attach_accept->esmmessagecontainer, writer);

  if ((attach_accept->presencemask & ATTACH_ACCEPT_GUTI_PRESENT)
      == ATTACH_ACCEPT_GUTI_PRESENT) {
    eps_mobile_identity_to_xml (&attach_accept->guti, writer);
  }

  if ((attach_accept->presencemask & ATTACH_ACCEPT_LOCATION_AREA_IDENTIFICATION_PRESENT)
      == ATTACH_ACCEPT_LOCATION_AREA_IDENTIFICATION_PRESENT) {
    location_area_identification_to_xml (&attach_accept->locationareaidentification, writer);
  }

  if ((attach_accept->presencemask & ATTACH_ACCEPT_MS_IDENTITY_PRESENT)
      == ATTACH_ACCEPT_MS_IDENTITY_PRESENT) {
    mobile_identity_to_xml (&attach_accept->msidentity, writer);
  }

  if ((attach_accept->presencemask & ATTACH_ACCEPT_EMM_CAUSE_PRESENT)
      == ATTACH_ACCEPT_EMM_CAUSE_PRESENT) {
    emm_cause_to_xml (&attach_accept->emmcause, writer);
  }

  if ((attach_accept->presencemask & ATTACH_ACCEPT_T3402_VALUE_PRESENT)
      == ATTACH_ACCEPT_T3402_VALUE_PRESENT) {
    gprs_timer_to_xml (GPRS_TIMER_T3402_IE_XML_STR, &attach_accept->t3402value, writer);
  }

  if ((attach_accept->presencemask & ATTACH_ACCEPT_T3423_VALUE_PRESENT)
      == ATTACH_ACCEPT_T3423_VALUE_PRESENT) {
    gprs_timer_to_xml (GPRS_TIMER_T3423_IE_XML_STR, &attach_accept->t3423value, writer);
  }

  if ((attach_accept->presencemask & ATTACH_ACCEPT_EQUIVALENT_PLMNS_PRESENT)
      == ATTACH_ACCEPT_EQUIVALENT_PLMNS_PRESENT) {
    plmn_list_to_xml (&attach_accept->equivalentplmns, writer);
  }

  if ((attach_accept->presencemask & ATTACH_ACCEPT_EMERGENCY_NUMBER_LIST_PRESENT)
      == ATTACH_ACCEPT_EMERGENCY_NUMBER_LIST_PRESENT) {
    emergency_number_list_to_xml (&attach_accept->emergencynumberlist, writer);
  }

  if ((attach_accept->presencemask & ATTACH_ACCEPT_EPS_NETWORK_FEATURE_SUPPORT_PRESENT)
      == ATTACH_ACCEPT_EPS_NETWORK_FEATURE_SUPPORT_PRESENT) {
    eps_network_feature_support_to_xml (&attach_accept->epsnetworkfeaturesupport, writer);
  }

  if ((attach_accept->presencemask & ATTACH_ACCEPT_ADDITIONAL_UPDATE_RESULT_PRESENT)
      == ATTACH_ACCEPT_ADDITIONAL_UPDATE_RESULT_PRESENT) {
    additional_update_result_to_xml (&attach_accept->additionalupdateresult, writer);
  }

  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool attach_complete_from_xml (
    xmlDocPtr           xml_doc,
    xmlXPathContextPtr  xpath_ctx,
    attach_complete_msg * const attach_complete)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res =   res = esm_message_container_from_xml (xml_doc, xpath_ctx, &attach_complete->esmmessagecontainer);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int attach_complete_to_xml (
  attach_complete_msg * attach_complete,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  esm_message_container_to_xml (attach_complete->esmmessagecontainer, writer);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool attach_reject_from_xml (
    xmlDocPtr           xml_doc,
    xmlXPathContextPtr  xpath_ctx,
    attach_reject_msg  * const attach_reject)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;

  res = esm_message_container_from_xml (xml_doc, xpath_ctx, &attach_reject->esmmessagecontainer);
  attach_reject->presencemask = 0;
  if (res) {
    attach_reject->presencemask |= ATTACH_REJECT_ESM_MESSAGE_CONTAINER_PRESENT;
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int attach_reject_to_xml (
  attach_reject_msg * attach_reject,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  emm_cause_to_xml (&attach_reject->emmcause, writer);

  if ((attach_reject->presencemask & ATTACH_REJECT_ESM_MESSAGE_CONTAINER_PRESENT)
      == ATTACH_REJECT_ESM_MESSAGE_CONTAINER_PRESENT) {
    esm_message_container_to_xml (attach_reject->esmmessagecontainer, writer);
  }

  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool attach_request_from_xml (
    xmlDocPtr           xml_doc,
     xmlXPathContextPtr  xpath_ctx,
    attach_request_msg * const attach_request)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;

  res = nas_key_set_identifier_from_xml (xml_doc, xpath_ctx, &attach_request->naskeysetidentifier);
  if (res) {res = eps_attach_type_from_xml (xml_doc, xpath_ctx, &attach_request->epsattachtype, NULL);}
  if (res) {res = eps_mobile_identity_from_xml (xml_doc, xpath_ctx, &attach_request->oldgutiorimsi);}
  if (res) {res = ue_network_capability_from_xml (xml_doc, xpath_ctx, &attach_request->uenetworkcapability);}
  if (res) {res = esm_message_container_from_xml (xml_doc, xpath_ctx, &attach_request->esmmessagecontainer);}
  attach_request->presencemask = 0;
  if (res) {
    res = p_tmsi_signature_from_xml (xml_doc, xpath_ctx, &attach_request->oldptmsisignature);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_OLD_PTMSI_SIGNATURE_PRESENT;
    }

    // TODO ERROR HERE: do not search with eps_mobile_identity_from_xml
    //res = eps_mobile_identity_from_xml (xml_doc, xpath_ctx, &attach_request->additionalguti);
    //if (res) {
    //  attach_request->presencemask |= ATTACH_REQUEST_ADDITIONAL_GUTI_PRESENT;
    //}

    res = tracking_area_identity_from_xml (xml_doc, xpath_ctx, &attach_request->lastvisitedregisteredtai);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_LAST_VISITED_REGISTERED_TAI_PRESENT;
    }

    res = drx_parameter_from_xml (xml_doc, xpath_ctx, &attach_request->drxparameter);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_DRX_PARAMETER_PRESENT;
    }

    res = ms_network_capability_from_xml (xml_doc, xpath_ctx, &attach_request->msnetworkcapability);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_MS_NETWORK_CAPABILITY_PRESENT;
    }

    res = location_area_identification_from_xml (xml_doc, xpath_ctx, &attach_request->oldlocationareaidentification);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_OLD_LOCATION_AREA_IDENTIFICATION_PRESENT;
    }

    res = tmsi_status_from_xml (xml_doc, xpath_ctx, &attach_request->tmsistatus);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_TMSI_STATUS_PRESENT;
    }

    res = mobile_station_classmark_2_from_xml (xml_doc, xpath_ctx, &attach_request->mobilestationclassmark2);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_MOBILE_STATION_CLASSMARK_2_PRESENT;
    }

    res = mobile_station_classmark_3_from_xml (xml_doc, xpath_ctx, &attach_request->mobilestationclassmark3);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_MOBILE_STATION_CLASSMARK_3_PRESENT;
    }

    res = supported_codec_list_from_xml (xml_doc, xpath_ctx, &attach_request->supportedcodecs);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_SUPPORTED_CODECS_PRESENT;
    }

    res = additional_update_type_from_xml (xml_doc, xpath_ctx, &attach_request->additionalupdatetype, NULL);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_ADDITIONAL_UPDATE_TYPE_PRESENT;
    }

    res = guti_type_from_xml (xml_doc, xpath_ctx, &attach_request->oldgutitype);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_OLD_GUTI_TYPE_PRESENT;
    }

    res = voice_domain_preference_and_ue_usage_setting_from_xml (xml_doc, xpath_ctx, &attach_request->voicedomainpreferenceandueusagesetting);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_VOICE_DOMAIN_PREFERENCE_AND_UE_USAGE_SETTING_PRESENT;
    }

    res = ms_network_feature_support_from_xml (xml_doc, xpath_ctx, &attach_request->msnetworkfeaturesupport);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_MS_NETWORK_FEATURE_SUPPORT_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int attach_request_to_xml (
  attach_request_msg * attach_request,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  nas_key_set_identifier_to_xml (&attach_request->naskeysetidentifier, writer);
  eps_attach_type_to_xml (&attach_request->epsattachtype, writer);
  eps_mobile_identity_to_xml (&attach_request->oldgutiorimsi, writer);

  ue_network_capability_to_xml (&attach_request->uenetworkcapability, writer);

  esm_message_container_to_xml (attach_request->esmmessagecontainer, writer);

  if ((attach_request->presencemask & ATTACH_REQUEST_OLD_PTMSI_SIGNATURE_PRESENT)
      == ATTACH_REQUEST_OLD_PTMSI_SIGNATURE_PRESENT) {
    p_tmsi_signature_to_xml (&attach_request->oldptmsisignature, writer);
  }

  if ((attach_request->presencemask & ATTACH_REQUEST_ADDITIONAL_GUTI_PRESENT)
      == ATTACH_REQUEST_ADDITIONAL_GUTI_PRESENT) {
    eps_mobile_identity_to_xml (&attach_request->additionalguti, writer);
  }

  if ((attach_request->presencemask & ATTACH_REQUEST_LAST_VISITED_REGISTERED_TAI_PRESENT)
      == ATTACH_REQUEST_LAST_VISITED_REGISTERED_TAI_PRESENT) {
    tracking_area_identity_to_xml (&attach_request->lastvisitedregisteredtai, writer);
  }

  if ((attach_request->presencemask & ATTACH_REQUEST_DRX_PARAMETER_PRESENT)
      == ATTACH_REQUEST_DRX_PARAMETER_PRESENT) {
    drx_parameter_to_xml (&attach_request->drxparameter, writer);
  }

  if ((attach_request->presencemask & ATTACH_REQUEST_MS_NETWORK_CAPABILITY_PRESENT)
      == ATTACH_REQUEST_MS_NETWORK_CAPABILITY_PRESENT) {
    ms_network_capability_to_xml (&attach_request->msnetworkcapability, writer);
  }

  if ((attach_request->presencemask & ATTACH_REQUEST_OLD_LOCATION_AREA_IDENTIFICATION_PRESENT)
      == ATTACH_REQUEST_OLD_LOCATION_AREA_IDENTIFICATION_PRESENT) {
    location_area_identification_to_xml (&attach_request->oldlocationareaidentification, writer);
  }

  if ((attach_request->presencemask & ATTACH_REQUEST_TMSI_STATUS_PRESENT)
      == ATTACH_REQUEST_TMSI_STATUS_PRESENT) {
    tmsi_status_to_xml (&attach_request->tmsistatus , writer);
  }

  if ((attach_request->presencemask & ATTACH_REQUEST_MOBILE_STATION_CLASSMARK_2_PRESENT)
      == ATTACH_REQUEST_MOBILE_STATION_CLASSMARK_2_PRESENT) {
    mobile_station_classmark_2_to_xml (&attach_request->mobilestationclassmark2, writer);
  }

  if ((attach_request->presencemask & ATTACH_REQUEST_MOBILE_STATION_CLASSMARK_3_PRESENT)
      == ATTACH_REQUEST_MOBILE_STATION_CLASSMARK_3_PRESENT) {
    mobile_station_classmark_3_to_xml (&attach_request->mobilestationclassmark3, writer);
  }

  if ((attach_request->presencemask & ATTACH_REQUEST_SUPPORTED_CODECS_PRESENT)
      == ATTACH_REQUEST_SUPPORTED_CODECS_PRESENT) {
    supported_codec_list_to_xml (&attach_request->supportedcodecs, writer);
  }

  if ((attach_request->presencemask & ATTACH_REQUEST_ADDITIONAL_UPDATE_TYPE_PRESENT)
      == ATTACH_REQUEST_ADDITIONAL_UPDATE_TYPE_PRESENT) {
    additional_update_type_to_xml (&attach_request->additionalupdatetype, writer);
  }

  if ((attach_request->presencemask & ATTACH_REQUEST_OLD_GUTI_TYPE_PRESENT)
      == ATTACH_REQUEST_OLD_GUTI_TYPE_PRESENT) {
    guti_type_to_xml (&attach_request->oldgutitype, writer);
  }

  if ((attach_request->presencemask & ATTACH_REQUEST_VOICE_DOMAIN_PREFERENCE_AND_UE_USAGE_SETTING_PRESENT)
      == ATTACH_REQUEST_VOICE_DOMAIN_PREFERENCE_AND_UE_USAGE_SETTING_PRESENT) {
    voice_domain_preference_and_ue_usage_setting_to_xml (&attach_request->voicedomainpreferenceandueusagesetting, writer);
  }

  if ((attach_request->presencemask & ATTACH_REQUEST_MS_NETWORK_FEATURE_SUPPORT_PRESENT)
      == ATTACH_REQUEST_MS_NETWORK_FEATURE_SUPPORT_PRESENT) {
    ms_network_feature_support_to_xml (&attach_request->msnetworkfeaturesupport, writer);
  }

  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool authentication_failure_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    authentication_failure_msg  * const authentication_failure)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = emm_cause_from_xml (xml_doc, xpath_ctx, &authentication_failure->emmcause, NULL);
  authentication_failure->presencemask = 0;
  if (res) {
    res = authentication_failure_parameter_from_xml (xml_doc, xpath_ctx, &authentication_failure->authenticationfailureparameter);
    if (res) {
      authentication_failure->presencemask |= AUTHENTICATION_FAILURE_AUTHENTICATION_FAILURE_PARAMETER_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int authentication_failure_to_xml (
  authentication_failure_msg * authentication_failure,
  xmlTextWriterPtr writer)
{

  OAILOG_FUNC_IN (LOG_XML);
  emm_cause_to_xml (&authentication_failure->emmcause, writer);

  if ((authentication_failure->presencemask & AUTHENTICATION_FAILURE_AUTHENTICATION_FAILURE_PARAMETER_PRESENT)
      == AUTHENTICATION_FAILURE_AUTHENTICATION_FAILURE_PARAMETER_PRESENT) {
    authentication_failure_parameter_to_xml (authentication_failure->authenticationfailureparameter, writer);
  }

  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool authentication_reject_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    authentication_reject_msg  * const authentication_reject)
{
  OAILOG_FUNC_IN (LOG_XML);
  // NOTHING TODO
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}

//------------------------------------------------------------------------------
int authentication_reject_to_xml (
  authentication_reject_msg * authentication_reject,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  // NOTHING TODO
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool authentication_request_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    authentication_request_msg  * const authentication_request)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = nas_key_set_identifier_from_xml (xml_doc, xpath_ctx, &authentication_request->naskeysetidentifierasme);
  if (res) {res = authentication_parameter_rand_from_xml (xml_doc, xpath_ctx, &authentication_request->authenticationparameterrand);}
  if (res) {res = authentication_parameter_autn_from_xml (xml_doc, xpath_ctx, &authentication_request->authenticationparameterautn);}
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int authentication_request_to_xml (
  authentication_request_msg * authentication_request,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  nas_key_set_identifier_to_xml (&authentication_request->naskeysetidentifierasme, writer);
  authentication_parameter_rand_to_xml (authentication_request->authenticationparameterrand, writer);
  authentication_parameter_autn_to_xml (authentication_request->authenticationparameterautn, writer);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool authentication_response_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    authentication_response_msg  * const authentication_response)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = authentication_response_parameter_from_xml (xml_doc, xpath_ctx, &authentication_response->authenticationresponseparameter);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int authentication_response_to_xml (
  authentication_response_msg * authentication_response,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  authentication_response_parameter_to_xml(authentication_response->authenticationresponseparameter, writer);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool detach_accept_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    detach_accept_msg  * const detach_accept)
{
  OAILOG_FUNC_IN (LOG_XML);
  // NOTHING TODO
  OAILOG_FUNC_RETURN (LOG_XML, true);
}

//------------------------------------------------------------------------------
int detach_accept_to_xml (
  detach_accept_msg * detach_accept,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  // NOTHING TODO
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool detach_request_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    detach_request_msg  * const detach_request)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;

  res = nas_key_set_identifier_from_xml (xml_doc, xpath_ctx, &detach_request->naskeysetidentifier);
  if (res) {res = detach_type_from_xml (xml_doc, xpath_ctx, &detach_request->detachtype);}
  if (res) {res = eps_mobile_identity_from_xml (xml_doc, xpath_ctx, &detach_request->gutiorimsi);}
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int detach_request_to_xml (
  detach_request_msg * detach_request,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  nas_key_set_identifier_to_xml (&detach_request->naskeysetidentifier, writer);
  detach_type_to_xml (&detach_request->detachtype, writer);
  eps_mobile_identity_to_xml (&detach_request->gutiorimsi, writer);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool downlink_nas_transport_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    downlink_nas_transport_msg   * const downlink_nas_transport)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = nas_message_container_from_xml (xml_doc, xpath_ctx, &downlink_nas_transport->nasmessagecontainer);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int downlink_nas_transport_to_xml (
  downlink_nas_transport_msg * downlink_nas_transport,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  nas_message_container_to_xml (downlink_nas_transport->nasmessagecontainer, writer);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool emm_information_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    emm_information_msg * const emm_information)
{
  OAILOG_FUNC_IN (LOG_XML);
  emm_information->presencemask = 0;
  if (network_name_from_xml (xml_doc, xpath_ctx, FULL_NETWORK_NAME_IE_XML_STR, &emm_information->fullnamefornetwork)) {
    emm_information->presencemask |= EMM_INFORMATION_FULL_NAME_FOR_NETWORK_PRESENT;
  }

  if (network_name_from_xml (xml_doc, xpath_ctx, SHORT_NETWORK_NAME_IE_XML_STR, &emm_information->shortnamefornetwork)) {
    emm_information->presencemask |= EMM_INFORMATION_SHORT_NAME_FOR_NETWORK_PRESENT;
  }

  if (time_zone_from_xml (xml_doc, xpath_ctx, &emm_information->localtimezone)) {
    emm_information->presencemask |= EMM_INFORMATION_LOCAL_TIME_ZONE_PRESENT;
  }

  if (time_zone_and_time_from_xml (xml_doc, xpath_ctx, &emm_information->universaltimeandlocaltimezone)) {
    emm_information->presencemask |= EMM_INFORMATION_UNIVERSAL_TIME_AND_LOCAL_TIME_ZONE_PRESENT;
  }

  if (daylight_saving_time_from_xml (xml_doc, xpath_ctx, &emm_information->networkdaylightsavingtime)) {
    emm_information->presencemask |= EMM_INFORMATION_NETWORK_DAYLIGHT_SAVING_TIME_PRESENT;
  }
  // no mandatory field
  OAILOG_FUNC_RETURN (LOG_XML, true);
}

//------------------------------------------------------------------------------
int emm_information_to_xml (
  emm_information_msg * emm_information,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  if ((emm_information->presencemask & EMM_INFORMATION_FULL_NAME_FOR_NETWORK_PRESENT)
      == EMM_INFORMATION_FULL_NAME_FOR_NETWORK_PRESENT) {
    network_name_to_xml (FULL_NETWORK_NAME_IE_XML_STR, &emm_information->fullnamefornetwork, writer);
  }

  if ((emm_information->presencemask & EMM_INFORMATION_SHORT_NAME_FOR_NETWORK_PRESENT)
      == EMM_INFORMATION_SHORT_NAME_FOR_NETWORK_PRESENT) {
    network_name_to_xml (SHORT_NETWORK_NAME_IE_XML_STR, &emm_information->shortnamefornetwork, writer);
  }

  if ((emm_information->presencemask & EMM_INFORMATION_LOCAL_TIME_ZONE_PRESENT)
      == EMM_INFORMATION_LOCAL_TIME_ZONE_PRESENT) {
    time_zone_to_xml (&emm_information->localtimezone, writer);
  }

  if ((emm_information->presencemask & EMM_INFORMATION_UNIVERSAL_TIME_AND_LOCAL_TIME_ZONE_PRESENT)
      == EMM_INFORMATION_UNIVERSAL_TIME_AND_LOCAL_TIME_ZONE_PRESENT) {
    time_zone_and_time_to_xml (&emm_information->universaltimeandlocaltimezone, writer);
  }

  if ((emm_information->presencemask & EMM_INFORMATION_NETWORK_DAYLIGHT_SAVING_TIME_PRESENT)
      == EMM_INFORMATION_NETWORK_DAYLIGHT_SAVING_TIME_PRESENT) {
    daylight_saving_time_to_xml (&emm_information->networkdaylightsavingtime, writer);
  }

  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool emm_status_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    emm_status_msg * const emm_status)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = emm_cause_from_xml (xml_doc, xpath_ctx, &emm_status->emmcause, NULL);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int emm_status_to_xml (
  emm_status_msg * emm_status,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  emm_cause_to_xml (&emm_status->emmcause, writer);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool identity_request_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    identity_request_msg * const identity_request)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = identity_type_2_from_xml (xml_doc, xpath_ctx, &identity_request->identitytype);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int identity_request_to_xml (
  identity_request_msg * identity_request,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  identity_type_2_to_xml (&identity_request->identitytype, writer);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool identity_response_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    identity_response_msg * const identity_response)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = mobile_identity_from_xml (xml_doc, xpath_ctx, &identity_response->mobileidentity);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int identity_response_to_xml (
  identity_response_msg * identity_response,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  mobile_identity_to_xml (&identity_response->mobileidentity, writer);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool security_mode_command_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    security_mode_command_msg * const security_mode_command)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;

  res = nas_security_algorithms_from_xml (xml_doc, xpath_ctx, &security_mode_command->selectednassecurityalgorithms);
  if (res) {res = nas_key_set_identifier_from_xml (xml_doc, xpath_ctx, &security_mode_command->naskeysetidentifier);}
  if (res) {res = ue_security_capability_from_xml (xml_doc, xpath_ctx, &security_mode_command->replayeduesecuritycapabilities);}
  security_mode_command->presencemask = 0;
  if (res) {
    res = imeisv_request_from_xml (xml_doc, xpath_ctx, &security_mode_command->imeisvrequest);
    if (res) {
      security_mode_command->presencemask |= SECURITY_MODE_COMMAND_IMEISV_REQUEST_PRESENT;
    }

    res = nonce_from_xml (xml_doc, xpath_ctx, REPLAYED_NONCE_UE_IE_XML_STR, &security_mode_command->replayednonceue);
    if (res) {
      security_mode_command->presencemask |= SECURITY_MODE_COMMAND_REPLAYED_NONCEUE_PRESENT;
    }

    res = nonce_from_xml (xml_doc, xpath_ctx, NONCE_MME_IE_XML_STR, &security_mode_command->noncemme);
    if (res) {
      security_mode_command->presencemask |= SECURITY_MODE_COMMAND_NONCEMME_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
int security_mode_command_to_xml (
  security_mode_command_msg * security_mode_command,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  nas_security_algorithms_to_xml (&security_mode_command->selectednassecurityalgorithms, writer);

  nas_key_set_identifier_to_xml (&security_mode_command->naskeysetidentifier, writer);

  ue_security_capability_to_xml (&security_mode_command->replayeduesecuritycapabilities, writer);

  if ((security_mode_command->presencemask & SECURITY_MODE_COMMAND_IMEISV_REQUEST_PRESENT)
      == SECURITY_MODE_COMMAND_IMEISV_REQUEST_PRESENT) {
    imeisv_request_to_xml (&security_mode_command->imeisvrequest, writer);
  }

  if ((security_mode_command->presencemask & SECURITY_MODE_COMMAND_REPLAYED_NONCEUE_PRESENT)
      == SECURITY_MODE_COMMAND_REPLAYED_NONCEUE_PRESENT) {
    nonce_to_xml (REPLAYED_NONCE_UE_IE_XML_STR, &security_mode_command->replayednonceue, writer);
  }

  if ((security_mode_command->presencemask & SECURITY_MODE_COMMAND_NONCEMME_PRESENT)
      == SECURITY_MODE_COMMAND_NONCEMME_PRESENT) {
    nonce_to_xml (NONCE_MME_IE_XML_STR, &security_mode_command->noncemme, writer);
  }
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool security_mode_complete_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    security_mode_complete_msg * const security_mode_complete)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = mobile_identity_from_xml (xml_doc, xpath_ctx, &security_mode_complete->imeisv);
  security_mode_complete->presencemask = 0;
  if (res) {
    security_mode_complete->presencemask |= SECURITY_MODE_COMPLETE_IMEISV_PRESENT;
  }
  res = true;
  OAILOG_FUNC_RETURN (LOG_XML, res);
}


//------------------------------------------------------------------------------
int security_mode_complete_to_xml (
  security_mode_complete_msg * security_mode_complete,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  if ((security_mode_complete->presencemask & SECURITY_MODE_COMPLETE_IMEISV_PRESENT)
      == SECURITY_MODE_COMPLETE_IMEISV_PRESENT) {
    mobile_identity_to_xml (&security_mode_complete->imeisv, writer);
  }
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool security_mode_reject_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    security_mode_reject_msg * const security_mode_reject)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = emm_cause_from_xml (xml_doc, xpath_ctx, &security_mode_reject->emmcause, NULL);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int security_mode_reject_to_xml (
  security_mode_reject_msg * security_mode_reject,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  emm_cause_to_xml (&security_mode_reject->emmcause, writer);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool service_reject_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
  service_reject_msg * service_reject)
{
  OAILOG_FUNC_IN (LOG_XML);
  OAILOG_FUNC_RETURN (LOG_XML, true);
}

//------------------------------------------------------------------------------
int service_reject_to_xml (
  service_reject_msg * service_reject,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);

  emm_cause_to_xml (&service_reject->emmcause, writer);
  // Just wait a litle bit for CS...
  //gprs_timer_to_xml (GPRS_TIMER_T3442_IE_XML_STR, &service_reject->t3442value, writer);
  //gprs_timer_to_xml (GPRS_TIMER_T3346_IE_XML_STR, &service_reject->t3346value, writer);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool service_request_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
  service_request_msg * service_request)
{
  OAILOG_FUNC_IN (LOG_XML);
  OAILOG_FUNC_RETURN (LOG_XML, true);
}

//------------------------------------------------------------------------------
int service_request_to_xml (
  service_request_msg * service_request,
  xmlTextWriterPtr writer)
{

  OAILOG_FUNC_IN (LOG_XML);
  ksi_and_sequence_number_to_xml (&service_request->ksiandsequencenumber,writer);
  short_mac_to_xml (&service_request->messageauthenticationcode, writer);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool tracking_area_update_accept_from_xml (
  xmlDocPtr                     xml_doc,
  xmlXPathContextPtr            xpath_ctx,
  tracking_area_update_accept_msg * tracking_area_update_accept)
{
  OAILOG_FUNC_IN (LOG_XML);
  OAILOG_FUNC_RETURN (LOG_XML, true);
}

//------------------------------------------------------------------------------
int tracking_area_update_accept_to_xml (
  tracking_area_update_accept_msg * tracking_area_update_accept,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);

  eps_update_result_to_xml (&tracking_area_update_accept->epsupdateresult, writer);

  if ((tracking_area_update_accept->presencemask & TRACKING_AREA_UPDATE_ACCEPT_T3412_VALUE_PRESENT)
      == TRACKING_AREA_UPDATE_ACCEPT_T3412_VALUE_PRESENT) {
    gprs_timer_to_xml (GPRS_TIMER_T3412_IE_XML_STR, &tracking_area_update_accept->t3412value, writer);
  }

  if ((tracking_area_update_accept->presencemask & TRACKING_AREA_UPDATE_ACCEPT_GUTI_PRESENT)
      == TRACKING_AREA_UPDATE_ACCEPT_GUTI_PRESENT) {
    eps_mobile_identity_to_xml (&tracking_area_update_accept->guti, writer);
  }

  if ((tracking_area_update_accept->presencemask & TRACKING_AREA_UPDATE_ACCEPT_TAI_LIST_PRESENT)
      == TRACKING_AREA_UPDATE_ACCEPT_TAI_LIST_PRESENT) {
    tracking_area_identity_list_to_xml (&tracking_area_update_accept->tailist, writer);
  }

  if ((tracking_area_update_accept->presencemask & TRACKING_AREA_UPDATE_ACCEPT_EPS_BEARER_CONTEXT_STATUS_PRESENT)
      == TRACKING_AREA_UPDATE_ACCEPT_EPS_BEARER_CONTEXT_STATUS_PRESENT) {
    eps_bearer_context_status_to_xml (&tracking_area_update_accept->epsbearercontextstatus, writer);
  }

  if ((tracking_area_update_accept->presencemask & TRACKING_AREA_UPDATE_ACCEPT_LOCATION_AREA_IDENTIFICATION_PRESENT)
      == TRACKING_AREA_UPDATE_ACCEPT_LOCATION_AREA_IDENTIFICATION_PRESENT) {
    location_area_identification_to_xml (&tracking_area_update_accept->locationareaidentification, writer);
  }

  if ((tracking_area_update_accept->presencemask & TRACKING_AREA_UPDATE_ACCEPT_MS_IDENTITY_PRESENT)
      == TRACKING_AREA_UPDATE_ACCEPT_MS_IDENTITY_PRESENT) {
    mobile_identity_to_xml (&tracking_area_update_accept->msidentity, writer);
  }

  if ((tracking_area_update_accept->presencemask & TRACKING_AREA_UPDATE_ACCEPT_EMM_CAUSE_PRESENT)
      == TRACKING_AREA_UPDATE_ACCEPT_EMM_CAUSE_PRESENT) {
    emm_cause_to_xml (&tracking_area_update_accept->emmcause, writer);
  }

  if ((tracking_area_update_accept->presencemask & TRACKING_AREA_UPDATE_ACCEPT_T3402_VALUE_PRESENT)
      == TRACKING_AREA_UPDATE_ACCEPT_T3402_VALUE_PRESENT) {
    gprs_timer_to_xml (GPRS_TIMER_T3402_IE_XML_STR, &tracking_area_update_accept->t3402value, writer);
  }

  if ((tracking_area_update_accept->presencemask & TRACKING_AREA_UPDATE_ACCEPT_T3423_VALUE_PRESENT)
      == TRACKING_AREA_UPDATE_ACCEPT_T3423_VALUE_PRESENT) {
    gprs_timer_to_xml (GPRS_TIMER_T3423_IE_XML_STR, &tracking_area_update_accept->t3423value, writer);
  }

  if ((tracking_area_update_accept->presencemask & TRACKING_AREA_UPDATE_ACCEPT_EQUIVALENT_PLMNS_PRESENT)
      == TRACKING_AREA_UPDATE_ACCEPT_EQUIVALENT_PLMNS_PRESENT) {
    plmn_list_to_xml (&tracking_area_update_accept->equivalentplmns, writer);
  }

  if ((tracking_area_update_accept->presencemask & TRACKING_AREA_UPDATE_ACCEPT_EMERGENCY_NUMBER_LIST_PRESENT)
      == TRACKING_AREA_UPDATE_ACCEPT_EMERGENCY_NUMBER_LIST_PRESENT) {
    emergency_number_list_to_xml (&tracking_area_update_accept->emergencynumberlist, writer);
  }

  if ((tracking_area_update_accept->presencemask & TRACKING_AREA_UPDATE_ACCEPT_EPS_NETWORK_FEATURE_SUPPORT_PRESENT)
      == TRACKING_AREA_UPDATE_ACCEPT_EPS_NETWORK_FEATURE_SUPPORT_PRESENT) {
    eps_network_feature_support_to_xml (&tracking_area_update_accept->epsnetworkfeaturesupport, writer);
  }

  if ((tracking_area_update_accept->presencemask & TRACKING_AREA_UPDATE_ACCEPT_ADDITIONAL_UPDATE_RESULT_PRESENT)
      == TRACKING_AREA_UPDATE_ACCEPT_ADDITIONAL_UPDATE_RESULT_PRESENT) {
    additional_update_result_to_xml (&tracking_area_update_accept->additionalupdateresult, writer);
  }
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool tracking_area_update_complete_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
  tracking_area_update_complete_msg * tracking_area_update_complete)
{
  OAILOG_FUNC_IN (LOG_XML);
  OAILOG_FUNC_RETURN (LOG_XML, true);
}

//------------------------------------------------------------------------------
int tracking_area_update_complete_to_xml (
  tracking_area_update_complete_msg * tracking_area_update_complete,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool tracking_area_update_reject_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    tracking_area_update_reject_msg * tracking_area_update_reject)
{
  OAILOG_FUNC_IN (LOG_XML);
  OAILOG_FUNC_RETURN (LOG_XML, true);
}

//------------------------------------------------------------------------------
int tracking_area_update_reject_to_xml (
  tracking_area_update_reject_msg * tracking_area_update_reject,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  emm_cause_to_xml (&tracking_area_update_reject->emmcause, writer);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);

}
//------------------------------------------------------------------------------
bool tracking_area_update_request_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    tracking_area_update_request_msg * tracking_area_update_request)
{
  OAILOG_FUNC_IN (LOG_XML);
  OAILOG_FUNC_RETURN (LOG_XML, true);
}

//------------------------------------------------------------------------------
int tracking_area_update_request_to_xml (
  tracking_area_update_request_msg * tracking_area_update_request,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  eps_update_type_to_xml (&tracking_area_update_request->epsupdatetype, writer);
  nas_key_set_identifier_to_xml (&tracking_area_update_request->naskeysetidentifier, writer);
  eps_mobile_identity_to_xml (&tracking_area_update_request->oldguti, writer);

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_NONCURRENT_NATIVE_NAS_KEY_SET_IDENTIFIER_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_NONCURRENT_NATIVE_NAS_KEY_SET_IDENTIFIER_PRESENT) {
    nas_key_set_identifier_to_xml (&tracking_area_update_request->noncurrentnativenaskeysetidentifier, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_GPRS_CIPHERING_KEY_SEQUENCE_NUMBER_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_GPRS_CIPHERING_KEY_SEQUENCE_NUMBER_PRESENT) {
    ciphering_key_sequence_number_to_xml (&tracking_area_update_request->gprscipheringkeysequencenumber, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_OLD_PTMSI_SIGNATURE_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_OLD_PTMSI_SIGNATURE_PRESENT) {
    p_tmsi_signature_to_xml (&tracking_area_update_request->oldptmsisignature, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_ADDITIONAL_GUTI_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_ADDITIONAL_GUTI_PRESENT) {
    eps_mobile_identity_to_xml (&tracking_area_update_request->additionalguti, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_NONCEUE_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_NONCEUE_PRESENT) {
    nonce_to_xml (REPLAYED_NONCE_UE_IE_XML_STR, &tracking_area_update_request->nonceue, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_UE_NETWORK_CAPABILITY_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_UE_NETWORK_CAPABILITY_PRESENT) {
    ue_network_capability_to_xml (&tracking_area_update_request->uenetworkcapability, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_LAST_VISITED_REGISTERED_TAI_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_LAST_VISITED_REGISTERED_TAI_PRESENT) {
    tracking_area_identity_to_xml (&tracking_area_update_request->lastvisitedregisteredtai, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_DRX_PARAMETER_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_DRX_PARAMETER_PRESENT) {
    drx_parameter_to_xml (&tracking_area_update_request->drxparameter, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_UE_RADIO_CAPABILITY_INFORMATION_UPDATE_NEEDED_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_UE_RADIO_CAPABILITY_INFORMATION_UPDATE_NEEDED_PRESENT) {
    ue_radio_capability_information_update_needed_to_xml (&tracking_area_update_request->ueradiocapabilityinformationupdateneeded, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_EPS_BEARER_CONTEXT_STATUS_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_EPS_BEARER_CONTEXT_STATUS_PRESENT) {
    eps_bearer_context_status_to_xml (&tracking_area_update_request->epsbearercontextstatus, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_MS_NETWORK_CAPABILITY_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_MS_NETWORK_CAPABILITY_PRESENT) {
    ms_network_capability_to_xml (&tracking_area_update_request->msnetworkcapability, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_OLD_LOCATION_AREA_IDENTIFICATION_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_OLD_LOCATION_AREA_IDENTIFICATION_PRESENT) {
    location_area_identification_to_xml (&tracking_area_update_request->oldlocationareaidentification, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_TMSI_STATUS_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_TMSI_STATUS_PRESENT) {
    tmsi_status_to_xml (&tracking_area_update_request->tmsistatus, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_MOBILE_STATION_CLASSMARK_2_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_MOBILE_STATION_CLASSMARK_2_PRESENT) {
    mobile_station_classmark_2_to_xml (&tracking_area_update_request->mobilestationclassmark2, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_MOBILE_STATION_CLASSMARK_3_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_MOBILE_STATION_CLASSMARK_3_PRESENT) {
    mobile_station_classmark_3_to_xml (&tracking_area_update_request->mobilestationclassmark3, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_SUPPORTED_CODECS_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_SUPPORTED_CODECS_PRESENT) {
    supported_codec_list_to_xml (&tracking_area_update_request->supportedcodecs, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_ADDITIONAL_UPDATE_TYPE_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_ADDITIONAL_UPDATE_TYPE_PRESENT) {
    additional_update_type_to_xml (&tracking_area_update_request->additionalupdatetype, writer);
  }

  if ((tracking_area_update_request->presencemask & TRACKING_AREA_UPDATE_REQUEST_OLD_GUTI_TYPE_PRESENT)
      == TRACKING_AREA_UPDATE_REQUEST_OLD_GUTI_TYPE_PRESENT) {
    guti_type_to_xml (&tracking_area_update_request->oldgutitype, writer);
  }

  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}
//------------------------------------------------------------------------------
bool uplink_nas_transport_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    uplink_nas_transport_msg     * const uplink_nas_transport)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = nas_message_container_from_xml (xml_doc, xpath_ctx, &uplink_nas_transport->nasmessagecontainer);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int uplink_nas_transport_to_xml (
  uplink_nas_transport_msg * uplink_nas_transport,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  nas_message_container_to_xml (uplink_nas_transport->nasmessagecontainer, writer);
  OAILOG_FUNC_RETURN (LOG_XML, RETURNok);
}


//------------------------------------------------------------------------------
bool emm_msg_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    EMM_msg                     * emm_msg)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool                                    res = false;

  // First decode the EMM message header
  res = emm_msg_header_from_xml (xml_doc, xpath_ctx, &emm_msg->header);
  if (res) {
    switch (emm_msg->header.message_type) {
    case ATTACH_ACCEPT:
      res = attach_accept_from_xml (xml_doc, xpath_ctx, &emm_msg->attach_accept);
      break;

    case ATTACH_COMPLETE:
      res = attach_complete_from_xml (xml_doc, xpath_ctx, &emm_msg->attach_complete);
      break;

    case ATTACH_REJECT:
      res = attach_reject_from_xml (xml_doc, xpath_ctx, &emm_msg->attach_reject);
      break;

    case ATTACH_REQUEST:
      res = attach_request_from_xml (xml_doc, xpath_ctx, &emm_msg->attach_request);
      break;

    case AUTHENTICATION_FAILURE:
      res = authentication_failure_from_xml (xml_doc, xpath_ctx, &emm_msg->authentication_failure);
      break;

    case AUTHENTICATION_REJECT:
      res = authentication_reject_from_xml (xml_doc, xpath_ctx, &emm_msg->authentication_reject);
      break;

    case AUTHENTICATION_REQUEST:
      res = authentication_request_from_xml (xml_doc, xpath_ctx, &emm_msg->authentication_request);
      break;

    case AUTHENTICATION_RESPONSE:
      res = authentication_response_from_xml (xml_doc, xpath_ctx, &emm_msg->authentication_response);
      break;

    case CS_SERVICE_NOTIFICATION:
      //res = cs_service_notification_from_xml (xml_doc, xpath_ctx, &emm_msg->cs_service_notification);
      break;

    case DETACH_ACCEPT:
      res = detach_accept_from_xml (xml_doc, xpath_ctx, &emm_msg->detach_accept);
      break;

    case DETACH_REQUEST:
      res = detach_request_from_xml (xml_doc, xpath_ctx, &emm_msg->detach_request);
      break;

    case DOWNLINK_NAS_TRANSPORT:
      res = downlink_nas_transport_from_xml (xml_doc, xpath_ctx, &emm_msg->downlink_nas_transport);
      break;

    case EMM_INFORMATION:
      res = emm_information_from_xml (xml_doc, xpath_ctx, &emm_msg->emm_information);
      break;

    case EMM_STATUS:
      res = emm_status_from_xml (xml_doc, xpath_ctx, &emm_msg->emm_status);
      break;

    case EXTENDED_SERVICE_REQUEST:
      //res = extended_service_request_from_xml (xml_doc, xpath_ctx, &emm_msg->extended_service_request);
      break;

    case GUTI_REALLOCATION_COMMAND:
      //res = guti_reallocation_command_from_xml (xml_doc, xpath_ctx, &emm_msg->guti_reallocation_command);
      break;

    case GUTI_REALLOCATION_COMPLETE:
      //res = guti_reallocation_complete_from_xml (xml_doc, xpath_ctx, &emm_msg->guti_reallocation_complete);
      break;

    case IDENTITY_REQUEST:
      res = identity_request_from_xml (xml_doc, xpath_ctx, &emm_msg->identity_request);
      break;

    case IDENTITY_RESPONSE:
      res = identity_response_from_xml (xml_doc, xpath_ctx, &emm_msg->identity_response);
      break;

    case SECURITY_MODE_COMMAND:
      res = security_mode_command_from_xml (xml_doc, xpath_ctx, &emm_msg->security_mode_command);
      break;

    case SECURITY_MODE_COMPLETE:
      res = security_mode_complete_from_xml (xml_doc, xpath_ctx, &emm_msg->security_mode_complete);
      break;

    case SECURITY_MODE_REJECT:
      res = security_mode_reject_from_xml (xml_doc, xpath_ctx, &emm_msg->security_mode_reject);
      break;

    case SERVICE_REJECT:
      res = service_reject_from_xml (xml_doc, xpath_ctx, &emm_msg->service_reject);
      break;

    case TRACKING_AREA_UPDATE_ACCEPT:
      res = tracking_area_update_accept_from_xml (xml_doc, xpath_ctx, &emm_msg->tracking_area_update_accept);
      break;

    case TRACKING_AREA_UPDATE_COMPLETE:
      res = tracking_area_update_complete_from_xml (xml_doc, xpath_ctx, &emm_msg->tracking_area_update_complete);
      break;

    case TRACKING_AREA_UPDATE_REJECT:
      res = tracking_area_update_reject_from_xml (xml_doc, xpath_ctx, &emm_msg->tracking_area_update_reject);
      break;

    case TRACKING_AREA_UPDATE_REQUEST:
      res = tracking_area_update_request_from_xml (xml_doc, xpath_ctx, &emm_msg->tracking_area_update_request);
      break;

    case UPLINK_NAS_TRANSPORT:
      res = uplink_nas_transport_from_xml (xml_doc, xpath_ctx, &emm_msg->uplink_nas_transport);
      break;

    default:
      OAILOG_ERROR (LOG_XML, "EMM-MSG   - Unexpected message type: 0x%x", emm_msg->header.message_type);
      res = false;
      // TODO: Handle not standard layer 3 messages: SERVICE_REQUEST
    }
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int emm_msg_to_xml (
  EMM_msg * msg,
  uint8_t * buffer,
  uint32_t len,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  int                                     header_result;
  int                                     encode_result = RETURNerror;
  int                                     decode_result = RETURNerror;
  //bool                                    is_down_link = true;

  header_result = emm_msg_header_to_xml (&msg->header, buffer, len, writer);

  if (header_result < 0) {
    OAILOG_ERROR (LOG_XML, "EMM-MSG   - Failed to dump EMM message header to XML" "(%d)\n", header_result);
    OAILOG_FUNC_RETURN (LOG_XML, header_result);
  }

  buffer += header_result;
  len -= header_result;

  switch (msg->header.message_type) {
  case ATTACH_ACCEPT:
    decode_result = decode_attach_accept (&msg->attach_accept, buffer, len);
    if (0 < decode_result)
      encode_result = attach_accept_to_xml (&msg->attach_accept, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode ATTACH_ACCEPT\n");
    break;

  case ATTACH_COMPLETE:
    decode_result = decode_attach_complete (&msg->attach_complete, buffer, len);
    if (0 < decode_result)
      encode_result = attach_complete_to_xml (&msg->attach_complete, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode ATTACH_COMPLETE\n");
    break;

  case ATTACH_REJECT:
    decode_result = decode_attach_reject (&msg->attach_reject, buffer, len);
    encode_result = attach_reject_to_xml (&msg->attach_reject, writer);
    break;

  case ATTACH_REQUEST:
    decode_result = decode_attach_request (&msg->attach_request, buffer, len);
    if (0 < decode_result)
      encode_result = attach_request_to_xml (&msg->attach_request, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode ATTACH_REQUEST\n");
    break;

  case AUTHENTICATION_REJECT:
    decode_result = decode_authentication_reject (&msg->authentication_reject, buffer, len);
    if (0 <= decode_result)
      encode_result = authentication_reject_to_xml (&msg->authentication_reject, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode AUTHENTICATION_REJECT\n");
    break;

  case AUTHENTICATION_FAILURE:
    decode_result = decode_authentication_failure (&msg->authentication_failure, buffer, len);
    encode_result = authentication_failure_to_xml (&msg->authentication_failure, writer);
    break;

  case AUTHENTICATION_REQUEST:
    decode_result = decode_authentication_request (&msg->authentication_request, buffer, len);
    if (0 < decode_result)
      encode_result = authentication_request_to_xml (&msg->authentication_request, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode AUTHENTICATION_REQUEST\n");
    break;

  case AUTHENTICATION_RESPONSE:
    decode_result = decode_authentication_response (&msg->authentication_response, buffer, len);
    if (0 < decode_result)
      encode_result = authentication_response_to_xml (&msg->authentication_response, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode AUTHENTICATION_RESPONSE\n");
    break;

  case CS_SERVICE_NOTIFICATION:
    //encode_result = encode_cs_service_notification_to_xml (&msg->cs_service_notification writer);
    break;

  case DETACH_ACCEPT:
    decode_result = decode_detach_accept (&msg->detach_accept, buffer, len);
    if (0 < decode_result)
      encode_result = detach_accept_to_xml (&msg->detach_accept, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode DETACH_ACCEPT\n");
    break;

  case DETACH_REQUEST:
    decode_result = decode_detach_request (&msg->detach_request, buffer, len);
    if (0 < decode_result)
      encode_result = detach_request_to_xml (&msg->detach_request, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode DETACH_REQUEST\n");
    break;

  case DOWNLINK_NAS_TRANSPORT:
    decode_result = decode_downlink_nas_transport (&msg->downlink_nas_transport, buffer, len);
    if (0 < decode_result)
      encode_result = downlink_nas_transport_to_xml (&msg->downlink_nas_transport, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode DOWNLINK_NAS_TRANSPORT\n");
    break;

  case EMM_INFORMATION:
    decode_result = decode_emm_information (&msg->emm_information, buffer, len);
    if (0 < decode_result)
      encode_result = emm_information_to_xml (&msg->emm_information, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode EMM_INFORMATION\n");
    break;

  case EMM_STATUS:
    decode_result = decode_emm_status (&msg->emm_status, buffer, len);
    if (0 < decode_result)
      encode_result = emm_status_to_xml (&msg->emm_status, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode EMM_STATUS\n");
    break;

  case EXTENDED_SERVICE_REQUEST:
    //encode_result = extended_service_request_to_xml (&msg->extended_service_request writer);
    break;

  case GUTI_REALLOCATION_COMMAND:
    //encode_result = guti_reallocation_command_to_xml (&msg->guti_reallocation_command writer);
    break;

  case GUTI_REALLOCATION_COMPLETE:
    //encode_result = guti_reallocation_complete_to_xml (&msg->guti_reallocation_complete writer);
    break;

  case IDENTITY_RESPONSE:
    decode_result = decode_identity_response (&msg->identity_response, buffer, len);
    if (0 < decode_result)
      encode_result = identity_response_to_xml (&msg->identity_response, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode IDENTITY_RESPONSE\n");
    break;

  case IDENTITY_REQUEST:
    decode_result = decode_identity_request (&msg->identity_request, buffer, len);
    if (0 < decode_result)
      encode_result = identity_request_to_xml (&msg->identity_request, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode IDENTITY_REQUEST\n");
    break;

  case SECURITY_MODE_COMMAND:
    decode_result = decode_security_mode_command (&msg->security_mode_command, buffer, len);
    if (0 < decode_result)
      encode_result = security_mode_command_to_xml (&msg->security_mode_command, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode SECURITY_MODE_COMMAND\n");
    break;

  case SECURITY_MODE_COMPLETE:
    decode_result = decode_security_mode_complete (&msg->security_mode_complete, buffer, len);
    if (0 < decode_result)
      encode_result = security_mode_complete_to_xml (&msg->security_mode_complete, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode SECURITY_MODE_COMPLETE\n");
    break;

  case SECURITY_MODE_REJECT:
    decode_result = decode_security_mode_reject (&msg->security_mode_reject, buffer, len);
    if (0 < decode_result)
      encode_result = security_mode_reject_to_xml (&msg->security_mode_reject, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode SECURITY_MODE_REJECT\n");
    break;

  case SERVICE_REJECT:
    decode_result = decode_service_reject (&msg->service_reject, buffer, len);
    if (0 < decode_result)
      encode_result = service_reject_to_xml (&msg->service_reject, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode SERVICE_REJECT\n");
    break;

  case SERVICE_REQUEST:
    decode_result = decode_service_request (&msg->service_request, buffer, len);
    if (0 < decode_result)
      encode_result = service_request_to_xml (&msg->service_request, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode SERVICE_REQUEST\n");
    break;

  case TRACKING_AREA_UPDATE_ACCEPT:
    decode_result = decode_tracking_area_update_accept (&msg->tracking_area_update_accept, buffer, len);
    if (0 < decode_result)
      encode_result = tracking_area_update_accept_to_xml (&msg->tracking_area_update_accept, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode TRACKING_AREA_UPDATE_ACCEPT\n");
    break;

  case TRACKING_AREA_UPDATE_COMPLETE:
    decode_result = decode_tracking_area_update_complete (&msg->tracking_area_update_complete, buffer, len);
    if (0 < decode_result)
      encode_result = tracking_area_update_complete_to_xml (&msg->tracking_area_update_complete, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode TRACKING_AREA_UPDATE_COMPLETE\n");
    break;

  case TRACKING_AREA_UPDATE_REJECT:
    decode_result = decode_tracking_area_update_reject (&msg->tracking_area_update_reject, buffer, len);
    if (0 < decode_result)
      encode_result = tracking_area_update_reject_to_xml (&msg->tracking_area_update_reject, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode TRACKING_AREA_UPDATE_REJECT\n");
    break;

  case TRACKING_AREA_UPDATE_REQUEST:
    decode_result = decode_tracking_area_update_request (&msg->tracking_area_update_request, buffer, len);
    if (0 < decode_result)
      encode_result = tracking_area_update_request_to_xml (&msg->tracking_area_update_request, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode TRACKING_AREA_UPDATE_REQUEST\n");
    break;

  case UPLINK_NAS_TRANSPORT:
    decode_result = decode_uplink_nas_transport (&msg->uplink_nas_transport, buffer, len);
    if (0 < decode_result)
      encode_result = uplink_nas_transport_to_xml (&msg->uplink_nas_transport, writer);
    else
      OAILOG_ERROR (LOG_XML, "Failed to decode UPLINK_NAS_TRANSPORT\n");
    break;

  default:
    OAILOG_ERROR (LOG_XML, "EMM-MSG   - Unexpected message type: 0x%x\n", msg->header.message_type);
    encode_result = TLV_WRONG_MESSAGE_TYPE;
    /*
     * TODO: Handle not standard layer 3 messages: SERVICE_REQUEST
     */
  }

  if (encode_result < 0) {
    OAILOG_ERROR (LOG_XML, "EMM-MSG   - Failed to dump L3 EMM message 0x%x " "(%d) to XML\n", msg->header.message_type, encode_result);
  }

  OAILOG_FUNC_RETURN (LOG_XML, header_result + encode_result);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

//------------------------------------------------------------------------------
bool emm_msg_header_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    emm_msg_header_t * header)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool                         res = false;
  security_header_type_t       sht = 0;
  eps_protocol_discriminator_t pd = 0;
  message_type_t               message_type = 0;

  res = security_header_type_from_xml(xml_doc, xpath_ctx, &sht, NULL);
  if (res) {
    header->security_header_type = sht;
    res = protocol_discriminator_from_xml(xml_doc, xpath_ctx, &pd);
  }
  if (res) {
    header->protocol_discriminator = pd;
    res = message_type_from_xml(xml_doc, xpath_ctx, &message_type);
  }
  if (res) {
    header->message_type = message_type;
  }
  OAILOG_FUNC_RETURN (LOG_XML, res);
}

//------------------------------------------------------------------------------
int emm_msg_header_to_xml (
  emm_msg_header_t * header,
  const uint8_t * buffer,
  uint32_t len,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  int                                     header_result = 0;

  header_result= emm_msg_decode_header (header, buffer, len);

  if (header_result < 0) {
    OAILOG_ERROR (LOG_XML, "EMM-MSG   - Failed to decode EMM message header " "(%d)\n", header_result);
    OAILOG_FUNC_RETURN (LOG_XML, header_result);
  }

  buffer += header_result;
  len -= header_result;
  security_header_type_t sht = header->security_header_type;
  security_header_type_to_xml(&sht, writer);
  eps_protocol_discriminator_t pd = header->protocol_discriminator;
  protocol_discriminator_to_xml(&pd, writer);
  message_type_t message_type = header->message_type;
  message_type_to_xml(&message_type , writer);
  OAILOG_FUNC_RETURN (LOG_XML, sizeof(emm_msg_header_t));
}

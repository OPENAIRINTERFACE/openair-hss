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
#include "AttachRequest.h"
#include "AttachRequest_xml.h"

#include "../../../UTILS/xml_load.h"
#include "3gpp_24.008_xml.h"

//------------------------------------------------------------------------------
bool attach_request_from_xml (
    struct scenario_s     * const scenario,
    struct scenario_player_msg_s * const msg,
    attach_request_msg * const attach_request)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  bool res = false;

  res = nas_key_set_identifier_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->naskeysetidentifier);
  if (res) {res = eps_attach_type_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->epsattachtype);}
  if (res) {res = eps_mobile_identity_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->oldgutiorimsi);}
  if (res) {res = ue_network_capability_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->uenetworkcapability);}
  if (res) {res = esm_message_container_from_xml (scenario, msg, attach_request->esmmessagecontainer);}
  if (res) {
    res = p_tmsi_signature_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->oldptmsisignature);
    if (res) {
      attach_request->presencemask |= ATTACH_REQUEST_OLD_PTMSI_SIGNATURE_PRESENT;
    }

    res = eps_mobile_identity_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->additionalguti);
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

    res = additional_update_type_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_request->additionalupdatetype);
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
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, res);
}

//------------------------------------------------------------------------------
int attach_request_to_xml (
  attach_request_msg * attach_request,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
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
  
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

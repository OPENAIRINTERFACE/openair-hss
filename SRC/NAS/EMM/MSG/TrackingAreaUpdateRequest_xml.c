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
#include "TrackingAreaUpdateRequest.h"
#include "TrackingAreaUpdateRequest_xml.h"
#include "3gpp_24.008_xml.h"

//------------------------------------------------------------------------------
int tracking_area_update_request_from_xml (
  tracking_area_update_request_msg * tracking_area_update_request,
  uint8_t * buffer,
  uint32_t len)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

//------------------------------------------------------------------------------
int tracking_area_update_request_to_xml (
  tracking_area_update_request_msg * tracking_area_update_request,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
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

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

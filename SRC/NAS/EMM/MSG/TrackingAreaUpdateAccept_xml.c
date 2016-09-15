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
#include "TrackingAreaUpdateAccept.h"
#include "TrackingAreaUpdateAccept_xml.h"
#include "3gpp_24.008_xml.h"

//------------------------------------------------------------------------------
int tracking_area_update_accept_from_xml (
  tracking_area_update_accept_msg * tracking_area_update_accept,
  uint8_t * buffer,
  uint32_t len)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

//------------------------------------------------------------------------------
int tracking_area_update_accept_to_xml (
  tracking_area_update_accept_msg * tracking_area_update_accept,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);

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

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

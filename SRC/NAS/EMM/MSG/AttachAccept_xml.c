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
#include "AttachAccept.h"
#include "AttachAccept_xml.h"

#include "../../../UTILS/xml_load.h"
#include "3gpp_24.008_xml.h"

//------------------------------------------------------------------------------
bool attach_accept_from_xml (
    struct scenario_s            * const scenario,
    struct scenario_player_msg_s * const msg,
    attach_accept_msg  * const attach_accept)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  bool res = false;

  res = eps_attach_result_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->epsattachresult);
  if (res) {res = gprs_timer_from_xml (msg->xml_doc, msg->xpath_ctx, GPRS_TIMER_T3412_IE_XML_STR, &attach_accept->t3412value);}
  if (res) {res = tracking_area_identity_list_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->tailist);}
  if (res) {res = esm_message_container_from_xml (scenario, msg, attach_accept->esmmessagecontainer);}
  if (res) {
    res = eps_mobile_identity_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->guti);
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

    res = emm_cause_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->emmcause);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_EMM_CAUSE_PRESENT;
    }

    res = gprs_timer_from_xml (msg->xml_doc, msg->xpath_ctx, GPRS_TIMER_T3402_IE_XML_STR, &attach_accept->t3402value);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_T3402_VALUE_PRESENT;
    }

    res = gprs_timer_from_xml (msg->xml_doc, msg->xpath_ctx, GPRS_TIMER_T3423_IE_XML_STR, &attach_accept->t3423value);
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

    res = eps_network_feature_support_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->epsnetworkfeaturesupport);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_EPS_NETWORK_FEATURE_SUPPORT_PRESENT;
    }

    res = additional_update_result_from_xml (msg->xml_doc, msg->xpath_ctx, &attach_accept->additionalupdateresult);
    if (res) {
      attach_accept->presencemask |= ATTACH_ACCEPT_ADDITIONAL_UPDATE_RESULT_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, res);
}

//------------------------------------------------------------------------------
int attach_accept_to_xml (
  attach_accept_msg * attach_accept,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
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

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

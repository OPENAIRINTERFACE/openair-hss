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
#include "EmmInformation.h"
#include "EmmInformation_xml.h"

#include "../../../UTILS/xml_load.h"
#include "3gpp_24.008_xml.h"

//------------------------------------------------------------------------------
bool emm_information_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    emm_information_msg * const emm_information)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
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
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
}

//------------------------------------------------------------------------------
int emm_information_to_xml (
  emm_information_msg * emm_information,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
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

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

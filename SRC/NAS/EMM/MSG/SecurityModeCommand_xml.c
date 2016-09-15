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
#include "Nonce.h"
#include "mme_scenario_player.h"
#include "NASSecurityModeCommand.h"
#include "SecurityModeCommand_xml.h"

#include "../../../UTILS/xml_load.h"
#include "3gpp_24.008_xml.h"

//------------------------------------------------------------------------------
bool security_mode_command_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    security_mode_command_msg * const security_mode_command)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  bool res = false;

  res = nas_security_algorithms_from_xml (xml_doc, xpath_ctx, &security_mode_command->selectednassecurityalgorithms);
  if (res) {res = nas_key_set_identifier_from_xml (xml_doc, xpath_ctx, &security_mode_command->naskeysetidentifier);}
  if (res) {res = ue_security_capability_from_xml (xml_doc, xpath_ctx, &security_mode_command->replayeduesecuritycapabilities);}
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
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, res);
}
//------------------------------------------------------------------------------
int security_mode_command_to_xml (
  security_mode_command_msg * security_mode_command,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
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
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

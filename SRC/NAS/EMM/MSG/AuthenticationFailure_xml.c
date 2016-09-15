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
#include "AuthenticationFailure.h"
#include "AuthenticationFailure_xml.h"

#include "../../../UTILS/xml_load.h"
#include "3gpp_24.008_xml.h"

//------------------------------------------------------------------------------
bool authentication_failure_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    authentication_failure_msg  * const authentication_failure)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  bool res = emm_cause_from_xml (xml_doc, xpath_ctx, &authentication_failure->emmcause);
  if (res) {
    res = authentication_failure_parameter_from_xml (xml_doc, xpath_ctx, &authentication_failure->authenticationfailureparameter);
    if (res) {
      authentication_failure->presencemask |= AUTHENTICATION_FAILURE_AUTHENTICATION_FAILURE_PARAMETER_PRESENT;
    }
    res = true;
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, res);
}

//------------------------------------------------------------------------------
int authentication_failure_to_xml (
  authentication_failure_msg * authentication_failure,
  xmlTextWriterPtr writer)
{

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_cause_to_xml (&authentication_failure->emmcause, writer);

  if ((authentication_failure->presencemask & AUTHENTICATION_FAILURE_AUTHENTICATION_FAILURE_PARAMETER_PRESENT)
      == AUTHENTICATION_FAILURE_AUTHENTICATION_FAILURE_PARAMETER_PRESENT) {
    authentication_failure_parameter_to_xml (authentication_failure->authenticationfailureparameter, writer);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

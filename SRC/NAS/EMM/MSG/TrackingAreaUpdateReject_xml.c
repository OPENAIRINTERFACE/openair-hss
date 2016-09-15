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
#include "TrackingAreaUpdateReject.h"
#include "TrackingAreaUpdateReject_xml.h"
#include "3gpp_24.008_xml.h"

//------------------------------------------------------------------------------
int tracking_area_update_reject_from_xml (
  tracking_area_update_reject_msg * tracking_area_update_reject,
  uint8_t * buffer,
  uint32_t len)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

//------------------------------------------------------------------------------
int tracking_area_update_reject_to_xml (
  tracking_area_update_reject_msg * tracking_area_update_reject,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_cause_to_xml (&tracking_area_update_reject->emmcause, writer);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);

}

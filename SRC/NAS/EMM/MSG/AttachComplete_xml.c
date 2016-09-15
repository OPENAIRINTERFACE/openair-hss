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
#include "AttachComplete.h"
#include "AttachComplete_xml.h"

#include "../../../UTILS/xml_load.h"
#include "3gpp_24.008_xml.h"


//------------------------------------------------------------------------------
bool attach_complete_from_xml (
    struct scenario_s            * const scenario,
    struct scenario_player_msg_s * const msg,
    attach_complete_msg * const attach_complete)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  bool res =   res = esm_message_container_from_xml (scenario, msg, attach_complete->esmmessagecontainer);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, res);
}

//------------------------------------------------------------------------------
int attach_complete_to_xml (
  attach_complete_msg * attach_complete,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  esm_message_container_to_xml (attach_complete->esmmessagecontainer, writer);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

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
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "common_defs.h"
#include "mme_scenario_player.h"
#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "AttachReject.h"
#include "xml_load.h"
#include "3gpp_24.008_xml.h"
#include "3gpp_24.301_emm_ies_xml.h"
#include "AttachReject_xml.h"
#include "log.h"


//------------------------------------------------------------------------------
bool attach_reject_from_xml (
    struct scenario_s            * const scenario,
    struct scenario_player_msg_s * const msg,
    attach_reject_msg  * const attach_reject)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  bool res = false;

  res = esm_message_container_from_xml (scenario, msg, attach_reject->esmmessagecontainer);
  if (res) {
    attach_reject->presencemask |= ATTACH_REJECT_ESM_MESSAGE_CONTAINER_PRESENT;
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, res);
}

//------------------------------------------------------------------------------
int attach_reject_to_xml (
  attach_reject_msg * attach_reject,
  xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_cause_to_xml (&attach_reject->emmcause, writer);

  if ((attach_reject->presencemask & ATTACH_REJECT_ESM_MESSAGE_CONTAINER_PRESENT)
      == ATTACH_REJECT_ESM_MESSAGE_CONTAINER_PRESENT) {
    esm_message_container_to_xml (attach_reject->esmmessagecontainer, writer);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "xml_load.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "3gpp_24.301.h"
#include "security_types.h"
#include "securityDef.h"
#include "common_types.h"
#include "mme_api.h"
#include "emm_data.h"
#include "emm_msg.h"
#include "esm_msg.h"
#include "intertask_interface.h"
#include "timer.h"
#include "dynamic_memory_check.h"
#include "common_defs.h"
#include "mme_scenario_player.h"
#include "xml_msg_tags.h"
#include "xml_msg_load_itti.h"
#include "xml_msg_dump.h"
#include "xml_msg_dump_itti.h"
#include "itti_free_defined_msg.h"
#include "itti_comp.h"
#include "sp_xml_compare.h"

extern scenario_player_t g_msp_scenarios;

bstring mme_scenario_player_dump_nas_downlink_data_req (const MessageDef * const received_message);
bstring mme_scenario_player_dump_s1ap_ue_context_release_command (const MessageDef * const received_message);
bstring mme_scenario_player_dump_s11_create_bearer_request (const MessageDef * const received_message);

//------------------------------------------------------------------------------
// Return xml filename
bstring mme_scenario_player_dump_s11_create_bearer_request (const MessageDef * const received_message)
{
  const itti_s11_create_bearer_request_t * const itti_s11_create_bearer_request = &S11_CREATE_BEARER_REQUEST (received_message);

  //-------------------------------
  // Dump received message in XML
  //-------------------------------
  xmlTextWriterPtr xml_text_writer = NULL;
  char             filename[NAME_MAX+9];
  if (snprintf(filename, sizeof(filename), "/tmp/mme_msg_%06lu_%s.xml", xml_msg_dump_get_seq_uid(), ITTI_S11_CREATE_BEARER_REQ_XML_STR) <= 0) {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "xmlTextWriterStartDocument\n");
    return NULL;
  }
  /* Create a new XmlWriter for uri, with no compression. */
  xml_text_writer = xmlNewTextWriterFilename(filename, 0);
  if (xml_text_writer == NULL) {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error creating the xml writer\n");
    return NULL;
  } else {
    int rc = xmlTextWriterSetIndent(xml_text_writer, 1);
    if (!(!rc)) {
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error indenting Document\n");
    }
    rc = xmlTextWriterSetIndentString(xml_text_writer, (const xmlChar *)"  ");
    if (!(!rc)) {
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error indenting Document\n");
    }
    xml_msg_dump_itti_s11_create_bearer_request(itti_s11_create_bearer_request, received_message->ittiMsgHeader.originTaskId, TASK_MME_SCENARIO_PLAYER, xml_text_writer);
    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (!(!rc)) {
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error ending Document\n");
    }
    xmlFreeTextWriter(xml_text_writer);
    return bfromcstr(filename);
  }
  return NULL;
}


//------------------------------------------------------------------------------
// Return xml filename
bstring mme_scenario_player_dump_nas_downlink_data_req (const MessageDef * const received_message)
{
  const itti_nas_dl_data_req_t * const nas_dl_data_req = &NAS_DL_DATA_REQ (received_message);

  //-------------------------------
  // Dump received message in XML
  //-------------------------------
  xmlTextWriterPtr xml_text_writer = NULL;
  char             filename[NAME_MAX+9];
  if (snprintf(filename, sizeof(filename), "/tmp/mme_msg_%06lu_%s.xml", xml_msg_dump_get_seq_uid(), ITTI_NAS_DOWNLINK_DATA_REQ_XML_STR) <= 0) {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "xmlTextWriterStartDocument\n");
    return NULL;
  }
  /* Create a new XmlWriter for uri, with no compression. */
  xml_text_writer = xmlNewTextWriterFilename(filename, 0);
  if (xml_text_writer == NULL) {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error creating the xml writer\n");
    return NULL;
  } else {
    int rc = xmlTextWriterSetIndent(xml_text_writer, 1);
    if (!(!rc)) {
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error indenting Document\n");
    }
    rc = xmlTextWriterSetIndentString(xml_text_writer, (const xmlChar *)"  ");
    if (!(!rc)) {
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error indenting Document\n");
    }
    xml_msg_dump_itti_nas_downlink_data_req(nas_dl_data_req, received_message->ittiMsgHeader.originTaskId, TASK_MME_SCENARIO_PLAYER, xml_text_writer);
    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (!(!rc)) {
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error ending Document\n");
    }
    xmlFreeTextWriter(xml_text_writer);
    return bfromcstr(filename);
  }
  return NULL;
}

//------------------------------------------------------------------------------
void mme_scenario_player_handle_nas_downlink_data_req (instance_t instance, const MessageDef * const received_message)
{
  // get message specified in scenario
  scenario_t *scenario = g_msp_scenarios.current_scenario;
  if (!scenario) {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No scenario started, NAS_DOWNLINK_DATA_REQ discarded\n");
    return;
  }

  OAILOG_NOTICE(LOG_MME_SCENARIO_PLAYER, "Received NAS_DOWNLINK_DATA_REQ message\n");

  pthread_mutex_lock(&scenario->lock);
  scenario_player_item_t *item = scenario->last_played_item;
  if (item) item = item->next_item;

  if (!item) {
    pthread_mutex_unlock(&scenario->lock);
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No Pending RX message in scenario, NAS_DOWNLINK_DATA_REQ discarded\n");
    bstring filename = mme_scenario_player_dump_nas_downlink_data_req(received_message);
    bdestroy_wrapper(&filename);
    return;
  }

  if ((SCENARIO_PLAYER_ITEM_ITTI_MSG == item->item_type) && !(item->u.msg.is_tx) && !(item->is_played)) {
    if (NAS_DOWNLINK_DATA_REQ == ITTI_MSG_ID (item->u.msg.itti_msg)) {
      // OK messages seems to match
      if (item->u.msg.timer_id) {
        timer_remove(item->u.msg.timer_id);
      }
      msp_get_elapsed_time_since_scenario_start(scenario, &item->u.msg.time_stamp);

      OAILOG_TRACE(LOG_MME_SCENARIO_PLAYER, "Found matching NAS_DOWNLINK_DATA_REQ message UID %d\n", item->uid);

      bstring filename = mme_scenario_player_dump_nas_downlink_data_req(received_message);
      if (!filename) {
        scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
        pthread_mutex_unlock(&scenario->lock);
        return;
      }
      //-------------------------------
      // Compare (in XML) received message with scenario message
      //-------------------------------
      xmlDoc *doc = xmlReadFile(bdata(filename), NULL, 0);
      if (sp_compare_xml_docs(scenario, doc, item->u.msg.xml_doc)) {
        scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
      } else {
        if (SCENARIO_STATUS_PAUSED == scenario->status) {
          scenario_set_status(scenario, SCENARIO_STATUS_PLAYING, __FILE__, __LINE__);
        }
      }
      bdestroy_wrapper(&filename);
      xmlFreeDoc(doc);

      item->is_played = true;
    } else {
      scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Pending RX message in scenario is not NAS_DOWNLINK_DATA_REQ, scenario failed\n");
    }
    scenario->last_played_item = item;
    pthread_mutex_unlock(&scenario->lock);
    msp_scenario_tick(scenario);
    return;
  } else {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No Pending RX message in scenario, NAS_DOWNLINK_DATA_REQ discarded\n");
  }
  pthread_mutex_unlock(&scenario->lock);
}
//------------------------------------------------------------------------------
void mme_scenario_player_handle_mme_app_connection_establishment_cnf (instance_t instance, const MessageDef * const received_message)
{
  // get message specified in scenario
  scenario_t *scenario = g_msp_scenarios.current_scenario;
  if (!scenario) {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No scenario started, MME_APP_CONNECTION_ESTABLISHMENT_CNF discarded\n");
    return;
  }

  pthread_mutex_lock(&scenario->lock);
  scenario_player_item_t *item = scenario->last_played_item;
  if (item) item = item->next_item;
  else {
    pthread_mutex_unlock(&scenario->lock);
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No Pending RX message in scenario, MME_APP_CONNECTION_ESTABLISHMENT_CNF discarded\n");
    return;
  }

  if ((SCENARIO_PLAYER_ITEM_ITTI_MSG == item->item_type) && !(item->u.msg.is_tx) && !(item->is_played)) {
    if (MME_APP_CONNECTION_ESTABLISHMENT_CNF == ITTI_MSG_ID (item->u.msg.itti_msg)) {
      // OK messages seems to match
      msp_get_elapsed_time_since_scenario_start(scenario, &item->u.msg.time_stamp);
      const itti_mme_app_connection_establishment_cnf_t * const mme_app_connection_establishment_cnf = &MME_APP_CONNECTION_ESTABLISHMENT_CNF (received_message);

      //-------------------------------
      // Dump received message in XML
      //-------------------------------
      xmlTextWriterPtr xml_text_writer = NULL;
      char             filename[NAME_MAX+9];
      if (snprintf(filename, sizeof(filename), "/tmp/mme_msg_%06lu_%s.xml", xml_msg_dump_get_seq_uid(), ITTI_MME_APP_CONNECTION_ESTABLISHMENT_CNF_XML_STR) <= 0) {
        scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
        pthread_mutex_unlock(&scenario->lock);
        OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "xmlTextWriterStartDocument\n");
        return;
      }
      /* Create a new XmlWriter for uri, with no compression. */
      xml_text_writer = xmlNewTextWriterFilename(filename, 0);
      if (xml_text_writer == NULL) {
        scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
        OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error creating the xml writer\n");
      } else {
        int rc = xmlTextWriterSetIndent(xml_text_writer, 1);
        if (!(!rc)) {
          OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error indenting Document\n");
        }
        rc = xmlTextWriterSetIndentString(xml_text_writer, (const xmlChar *)"  ");
        if (!(!rc)) {
          OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error indenting Document\n");
        }
        xml_msg_dump_itti_mme_app_connection_establishment_cnf(mme_app_connection_establishment_cnf, received_message->ittiMsgHeader.originTaskId, TASK_MME_SCENARIO_PLAYER, xml_text_writer);
        rc = xmlTextWriterEndDocument(xml_text_writer);
        if (!(!rc)) {
          OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error ending Document\n");
        }
        xmlFreeTextWriter(xml_text_writer);

        // OLE.... pretty xml
        char cmd[400];
        snprintf(cmd, 400, "tmpfile=`mktemp`;xmllint --format %s > $tmpfile; cp $tmpfile %s;rm  $tmpfile; sync;", filename, filename);
        system(cmd);

        //-------------------------------
        // Compare (in XML) received message with scenario message
        //-------------------------------
        xmlDoc *doc = xmlReadFile(filename, NULL, 0);
        if (sp_compare_xml_docs(scenario, doc, item->u.msg.xml_doc)) {
          scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
        } else {
          if (SCENARIO_STATUS_PAUSED == scenario->status) {
            scenario_set_status(scenario, SCENARIO_STATUS_PLAYING, __FILE__, __LINE__);
          }
        }
        xmlFreeDoc(doc);
      }

      item->is_played = true;
    } else {
      scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Pending RX message in scenario is not MME_APP_CONNECTION_ESTABLISHMENT_CNF, scenario failed\n");
    }
    scenario->last_played_item = item;
    pthread_mutex_unlock(&scenario->lock);
    msp_scenario_tick(scenario);
    return;
  } else {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No Pending RX message in scenario, MME_APP_CONNECTION_ESTABLISHMENT_CNF discarded\n");
  }
  pthread_mutex_unlock(&scenario->lock);

}


//------------------------------------------------------------------------------
// Return xml filename
bstring mme_scenario_player_dump_s1ap_ue_context_release_command (const MessageDef * const received_message)
{
  const itti_s1ap_ue_context_release_command_t * const s1ap_ue_context_release_command = &S1AP_UE_CONTEXT_RELEASE_COMMAND (received_message);

  //-------------------------------
  // Dump received message in XML
  //-------------------------------
  xmlTextWriterPtr xml_text_writer = NULL;
  char             filename[NAME_MAX+9];
  if (snprintf(filename, sizeof(filename), "/tmp/mme_msg_%06lu_%s.xml", xml_msg_dump_get_seq_uid(), ITTI_S1AP_UE_CONTEXT_RELEASE_COMMAND_XML_STR) <= 0) {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "xmlTextWriterStartDocument\n");
    return NULL;
  }
  /* Create a new XmlWriter for uri, with no compression. */
  xml_text_writer = xmlNewTextWriterFilename(filename, 0);
  if (xml_text_writer == NULL) {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error creating the xml writer\n");
    return NULL;
  } else {
    int rc = xmlTextWriterSetIndent(xml_text_writer, 1);
    if (!(!rc)) {
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error indenting Document\n");
    }
    rc = xmlTextWriterSetIndentString(xml_text_writer, (const xmlChar *)"  ");
    if (!(!rc)) {
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error indenting Document\n");
    }
    xml_msg_dump_itti_s1ap_ue_context_release_command(s1ap_ue_context_release_command, received_message->ittiMsgHeader.originTaskId, TASK_MME_SCENARIO_PLAYER, xml_text_writer);
    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (!(!rc)) {
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error ending Document\n");
    }
    xmlFreeTextWriter(xml_text_writer);
    return bfromcstr(filename);
  }
  return NULL;
}

//------------------------------------------------------------------------------
void mme_scenario_player_handle_s1ap_ue_context_release_command (instance_t instance, const MessageDef * const received_message)
{
  // get message specified in scenario
  scenario_t *scenario = g_msp_scenarios.current_scenario;
  if (!scenario) {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No scenario started, S1AP_UE_CONTEXT_RELEASE_COMMAND discarded\n");
    return;
  }

  OAILOG_NOTICE(LOG_MME_SCENARIO_PLAYER, "Received S1AP_UE_CONTEXT_RELEASE_COMMAND message\n");

  pthread_mutex_lock(&scenario->lock);
  scenario_player_item_t *item = scenario->last_played_item;
  if (item) item = item->next_item;

  if (!item) {
    pthread_mutex_unlock(&scenario->lock);
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No Pending RX message in scenario, S1AP_UE_CONTEXT_RELEASE_COMMAND discarded\n");
    bstring filename = mme_scenario_player_dump_s1ap_ue_context_release_command(received_message);
    bdestroy_wrapper(&filename);
    return;
  }

  if ((SCENARIO_PLAYER_ITEM_ITTI_MSG == item->item_type) && !(item->u.msg.is_tx) && !(item->is_played)) {
    if (S1AP_UE_CONTEXT_RELEASE_COMMAND == ITTI_MSG_ID (item->u.msg.itti_msg)) {
      // OK messages seems to match
      if (item->u.msg.timer_id) {
        timer_remove(item->u.msg.timer_id);
      }
      msp_get_elapsed_time_since_scenario_start(scenario, &item->u.msg.time_stamp);

      OAILOG_TRACE(LOG_MME_SCENARIO_PLAYER, "Found matching S1AP_UE_CONTEXT_RELEASE_COMMAND message UID %d\n", item->uid);

      bstring filename = mme_scenario_player_dump_s1ap_ue_context_release_command(received_message);
      if (!filename) {
        scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
        pthread_mutex_unlock(&scenario->lock);
        return;
      }
      //-------------------------------
      // Compare (in XML) received message with scenario message
      //-------------------------------
      xmlDoc *doc = xmlReadFile(bdata(filename), NULL, 0);
      if (sp_compare_xml_docs(scenario, doc, item->u.msg.xml_doc)) {
        scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
      } else {
        if (SCENARIO_STATUS_PAUSED == scenario->status) {
          scenario_set_status(scenario, SCENARIO_STATUS_PLAYING, __FILE__, __LINE__);
        }
      }
      xmlFreeDoc(doc);
      bdestroy_wrapper(&filename);
      item->is_played = true;
    } else {
      scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Pending RX message in scenario is not S1AP_UE_CONTEXT_RELEASE_COMMAND, scenario failed\n");
    }
    scenario->last_played_item = item;
    pthread_mutex_unlock(&scenario->lock);
    msp_scenario_tick(scenario);
    return;
  } else {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No Pending RX message in scenario, S1AP_UE_CONTEXT_RELEASE_COMMAND discarded\n");
  }
  pthread_mutex_unlock(&scenario->lock);
}

//------------------------------------------------------------------------------
// Return xml filename
bstring mme_scenario_player_dump_s1ap_e_rab_setup_req (const MessageDef * const received_message)
{
  const itti_s1ap_e_rab_setup_req_t * const itti_s1ap_e_rab_setup_req = &S1AP_E_RAB_SETUP_REQ (received_message);

  //-------------------------------
  // Dump received message in XML
  //-------------------------------
  xmlTextWriterPtr xml_text_writer = NULL;
  char             filename[NAME_MAX+9];
  if (snprintf(filename, sizeof(filename), "/tmp/mme_msg_%06lu_%s.xml", xml_msg_dump_get_seq_uid(), ITTI_S1AP_E_RAB_SETUP_REQ_XML_STR) <= 0) {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "xmlTextWriterStartDocument\n");
    return NULL;
  }
  /* Create a new XmlWriter for uri, with no compression. */
  xml_text_writer = xmlNewTextWriterFilename(filename, 0);
  if (xml_text_writer == NULL) {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error creating the xml writer\n");
    return NULL;
  } else {
    int rc = xmlTextWriterSetIndent(xml_text_writer, 1);
    if (!(!rc)) {
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error indenting Document\n");
    }
    rc = xmlTextWriterSetIndentString(xml_text_writer, (const xmlChar *)"  ");
    if (!(!rc)) {
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error indenting Document\n");
    }
    xml_msg_dump_itti_s1ap_e_rab_setup_req(itti_s1ap_e_rab_setup_req, received_message->ittiMsgHeader.originTaskId, TASK_MME_SCENARIO_PLAYER, xml_text_writer);
    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (!(!rc)) {
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error ending Document\n");
    }
    xmlFreeTextWriter(xml_text_writer);
    return bfromcstr(filename);
  }
  return NULL;
}

//------------------------------------------------------------------------------
void mme_scenario_player_handle_s1ap_e_rab_setup_req (instance_t instance, const MessageDef * const received_message)
{
  // get message specified in scenario
  scenario_t *scenario = g_msp_scenarios.current_scenario;
  if (!scenario) {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No scenario started, S1AP_E_RAB_SETUP_REQ discarded\n");
    return;
  }

  OAILOG_NOTICE(LOG_MME_SCENARIO_PLAYER, "Received S1AP_E_RAB_SETUP_REQ message\n");

  pthread_mutex_lock(&scenario->lock);
  scenario_player_item_t *item = scenario->last_played_item;
  if (item) item = item->next_item;

  if (!item) {
    pthread_mutex_unlock(&scenario->lock);
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No Pending RX message in scenario, S1AP_E_RAB_SETUP_REQ discarded\n");
    bstring filename = mme_scenario_player_dump_s1ap_e_rab_setup_req(received_message);
    bdestroy_wrapper(&filename);
    return;
  }

  if ((SCENARIO_PLAYER_ITEM_ITTI_MSG == item->item_type) && !(item->u.msg.is_tx) && !(item->is_played)) {
    if (S1AP_E_RAB_SETUP_REQ == ITTI_MSG_ID (item->u.msg.itti_msg)) {
      // OK messages seems to match
      if (item->u.msg.timer_id) {
        timer_remove(item->u.msg.timer_id);
      }
      msp_get_elapsed_time_since_scenario_start(scenario, &item->u.msg.time_stamp);

      OAILOG_TRACE(LOG_MME_SCENARIO_PLAYER, "Found matching S1AP_E_RAB_SETUP_REQ message UID %d\n", item->uid);

      bstring filename = mme_scenario_player_dump_s1ap_e_rab_setup_req(received_message);
      if (!filename) {
        scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
        pthread_mutex_unlock(&scenario->lock);
        return;
      }
      //-------------------------------
      // Compare (in XML) received message with scenario message
      //-------------------------------
      xmlDoc *doc = xmlReadFile(bdata(filename), NULL, 0);
      if (sp_compare_xml_docs(scenario, doc, item->u.msg.xml_doc)) {
        scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
      } else {
        if (SCENARIO_STATUS_PAUSED == scenario->status) {
          scenario_set_status(scenario, SCENARIO_STATUS_PLAYING, __FILE__, __LINE__);
        }
      }
      xmlFreeDoc(doc);
      bdestroy_wrapper(&filename);
      item->is_played = true;
    } else {
      bstring filename = mme_scenario_player_dump_s1ap_e_rab_setup_req(received_message);
      bdestroy_wrapper(&filename);
      scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Pending RX message in scenario is not S1AP_E_RAB_SETUP_REQ, scenario failed\n");
    }
    scenario->last_played_item = item;
    pthread_mutex_unlock(&scenario->lock);
    msp_scenario_tick(scenario);
    return;
  } else {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No Pending RX message in scenario, S1AP_E_RAB_SETUP_REQ discarded\n");
  }
  pthread_mutex_unlock(&scenario->lock);
}


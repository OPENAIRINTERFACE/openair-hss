/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "hashtable.h"
#include "obj_hashtable.h"
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
#include "common_types.h"
#include "emm_msg.h"
#include "esm_msg.h"
#include "intertask_interface.h"
#include "3gpp_23.003_xml.h"
#include "3gpp_24.007_xml.h"
#include "3gpp_36.331_xml.h"
#include "3gpp_36.401_xml.h"
#include "nas_message_xml.h"
#include "xml_msg_dump.h"
#include "xml_msg_dump_itti.h"
#include "xml2_wrapper.h"
#include "common_defs.h"
#include "3gpp_24.301_emm_ies_xml.h"
#include "mme_scenario_player.h"

char* itti_task_id2itti_task_str(int task_id);

//------------------------------------------------------------------------------
xmlTextWriterPtr xml_msg_dump_itti_get_new_xml_text_writter(
    const unsigned long msg_num,
    const char * const msg_display_name)
{
  xmlTextWriterPtr xml_text_writer = NULL;
  char             filename[NAME_MAX+9];
  if (snprintf(filename, sizeof(filename), "/tmp/mme_msg_%06lu_%s.xml",
      msg_num, msg_display_name) <= 0) {
    OAI_FPRINTF_ERR("%s: Error at xmlTextWriterStartDocument\n", __FUNCTION__);
    return NULL;
  }
  /* Create a new XmlWriter for uri, with no compression. */
  xml_text_writer = xmlNewTextWriterFilename(filename, 0);
  if (xml_text_writer == NULL) {
    OAI_FPRINTF_ERR("%s: Error creating the xml writer\n", __FUNCTION__);
    return NULL;
  }
  return xml_text_writer;
}

//------------------------------------------------------------------------------
char* itti_task_id2itti_task_str(int task_id)
{
  switch(task_id) {
  case TASK_TIMER: return "TASK_TIMER"; break;
  case TASK_GTPV1_U: return "TASK_GTPV1_U"; break;
  case TASK_MME_APP: return "TASK_MME_APP"; break;
  case TASK_NAS_MME: return "TASK_NAS_MME"; break;
  case TASK_S11: return "TASK_S11"; break;
  case TASK_S1AP: return "TASK_S1AP"; break;
  case TASK_S6A: return "TASK_S6A"; break;
  case TASK_SCTP: return "TASK_SCTP"; break;
  case TASK_SPGW_APP: return "TASK_SPGW_APP"; break;
  case TASK_UDP: return "TASK_UDP"; break;
  case TASK_LOG: return "TASK_LOG"; break;
  case TASK_SHARED_TS_LOG: return "TASK_SHARED_TS_LOG"; break;
  case TASK_MME_SCENARIO_PLAYER: return "TASK_MME_SCENARIO_PLAYER"; break;
  default: return "TASK_UNKNOWN";
  }
}
//------------------------------------------------------------------------------
void xml_msg_dump_itti_sctp_new_association(const sctp_new_peer_t * const itti_msg,
    const int sender_task,
    const int receiver_task,
    xmlTextWriterPtr xml_text_writer_param)
{
  xmlTextWriterPtr xml_text_writer = NULL;
  int              rc = RETURNok;
  struct timeval   elapsed_time = {0};
  unsigned long    msg_num = xml_msg_dump_get_seq_uid();

  shared_log_get_elapsed_time_since_start(&elapsed_time);

  if (!xml_text_writer_param) {
    xml_text_writer = xml_msg_dump_itti_get_new_xml_text_writter(msg_num, ITTI_SCTP_NEW_ASSOCIATION_XML_STR);
  } else {
    xml_text_writer = xml_text_writer_param;
  }

  if (xml_text_writer) {

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(xml_text_writer, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterStartDocument\n", __FUNCTION__);
      xmlFreeTextWriter(xml_text_writer);
      return;
    }

    XML_WRITE_START_ELEMENT(xml_text_writer, ITTI_SCTP_NEW_ASSOCIATION_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ACTION_XML_STR, "%s", ACTION_SEND_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_SENDER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(sender_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_RECEIVER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(receiver_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, TIMESTAMP_XML_STR, "%lu.%lu", elapsed_time.tv_sec, elapsed_time.tv_usec);

    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, NUMBER_INBOUND_STREAMS_XML_STR, NUMBER_STREAMS_FMT,  itti_msg->instreams);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, NUMBER_OUTBOUND_STREAMS_XML_STR, NUMBER_STREAMS_FMT, itti_msg->outstreams);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, SCTP_ASSOC_ID_XML_STR, SCTP_ASSOC_ID_FMT, itti_msg->assoc_id);

    XML_WRITE_END_ELEMENT(xml_text_writer);

    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterEndDocument\n", __FUNCTION__);
    }

    if (!xml_text_writer_param) {
      xmlFreeTextWriter(xml_text_writer);
    }
  }
}

//------------------------------------------------------------------------------
void xml_msg_dump_itti_sctp_close_association(const sctp_close_association_t * const itti_msg,
    const int sender_task,
    const int receiver_task,
    xmlTextWriterPtr xml_text_writer_param)
{
  xmlTextWriterPtr xml_text_writer = NULL;
  int              rc = RETURNok;
  struct timeval   elapsed_time = {0};
  unsigned long    msg_num = xml_msg_dump_get_seq_uid();

  shared_log_get_elapsed_time_since_start(&elapsed_time);

  if (!xml_text_writer_param) {
    xml_text_writer = xml_msg_dump_itti_get_new_xml_text_writter(msg_num, ITTI_SCTP_CLOSE_ASSOCIATION_XML_STR);
  } else {
    xml_text_writer = xml_text_writer_param;
  }

  if (xml_text_writer) {

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(xml_text_writer, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterStartDocument\n", __FUNCTION__);
      xmlFreeTextWriter(xml_text_writer);
      return;
    }

    XML_WRITE_START_ELEMENT(xml_text_writer,  ITTI_SCTP_CLOSE_ASSOCIATION_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ACTION_XML_STR, "%s", ACTION_SEND_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_SENDER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(sender_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_RECEIVER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(receiver_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, TIMESTAMP_XML_STR, "%lu.%lu", elapsed_time.tv_sec, elapsed_time.tv_usec);

    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, SCTP_ASSOC_ID_XML_STR, "%x", itti_msg->assoc_id);

    XML_WRITE_END_ELEMENT(xml_text_writer);

    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterEndDocument\n", __FUNCTION__);
    }

    if (!xml_text_writer_param) {
      xmlFreeTextWriter(xml_text_writer);
    }
  }
}

//------------------------------------------------------------------------------
void xml_msg_dump_itti_s1ap_ue_context_release_req(const itti_s1ap_ue_context_release_req_t * const itti_msg,
    int sender_task,
    int receiver_task,
    xmlTextWriterPtr xml_text_writer_param)
{
  xmlTextWriterPtr xml_text_writer = NULL;
  int              rc = RETURNok;
  struct timeval   elapsed_time = {0};
  unsigned long    msg_num = xml_msg_dump_get_seq_uid();

  shared_log_get_elapsed_time_since_start(&elapsed_time);

  if (!xml_text_writer_param) {
    xml_text_writer = xml_msg_dump_itti_get_new_xml_text_writter(msg_num, ITTI_S1AP_UE_CONTEXT_RELEASE_REQ_XML_STR);
  } else {
    xml_text_writer = xml_text_writer_param;
  }

  if (xml_text_writer) {

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(xml_text_writer, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterStartDocument\n", __FUNCTION__);
      xmlFreeTextWriter(xml_text_writer);
      return;
    }

    XML_WRITE_START_ELEMENT(xml_text_writer, ITTI_S1AP_UE_CONTEXT_RELEASE_REQ_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ACTION_XML_STR, "%s", ACTION_SEND_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_SENDER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(sender_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_RECEIVER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(receiver_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, TIMESTAMP_XML_STR, "%lu.%lu", elapsed_time.tv_sec, elapsed_time.tv_usec);

    enb_ue_s1ap_id_t  enb_ue_s1ap_id = itti_msg->enb_ue_s1ap_id;
    enb_ue_s1ap_id_to_xml(&enb_ue_s1ap_id, xml_text_writer);
    mme_ue_s1ap_id_to_xml(&itti_msg->mme_ue_s1ap_id, xml_text_writer);

    XML_WRITE_END_ELEMENT(xml_text_writer);

    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterEndDocument\n", __FUNCTION__);
    }

    if (!xml_text_writer_param) {
      xmlFreeTextWriter(xml_text_writer);
    }
  }
}

//------------------------------------------------------------------------------
void xml_msg_dump_itti_s1ap_ue_context_release_command(const itti_s1ap_ue_context_release_command_t * const itti_msg,
    int sender_task,
    int receiver_task,
    xmlTextWriterPtr xml_text_writer_param)
{
  xmlTextWriterPtr xml_text_writer = NULL;
  int              rc = RETURNok;
  struct timeval   elapsed_time = {0};
  unsigned long    msg_num = xml_msg_dump_get_seq_uid();

  shared_log_get_elapsed_time_since_start(&elapsed_time);

  if (!xml_text_writer_param) {
    xml_text_writer = xml_msg_dump_itti_get_new_xml_text_writter(msg_num, ITTI_S1AP_UE_CONTEXT_RELEASE_COMMAND_XML_STR);
  } else {
    xml_text_writer = xml_text_writer_param;
  }

  if (xml_text_writer) {

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(xml_text_writer, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterStartDocument\n", __FUNCTION__);
      xmlFreeTextWriter(xml_text_writer);
      return;
    }

    XML_WRITE_START_ELEMENT(xml_text_writer, ITTI_S1AP_UE_CONTEXT_RELEASE_COMMAND_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ACTION_XML_STR, "%s", ACTION_WAIT_RECEIVE_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_SENDER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(sender_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_RECEIVER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(receiver_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, TIMESTAMP_XML_STR, "%lu.%lu", elapsed_time.tv_sec, elapsed_time.tv_usec);

    enb_ue_s1ap_id_t  enb_ue_s1ap_id = itti_msg->enb_ue_s1ap_id;
    enb_ue_s1ap_id_to_xml(&enb_ue_s1ap_id, xml_text_writer);
    mme_ue_s1ap_id_to_xml(&itti_msg->mme_ue_s1ap_id, xml_text_writer);

    XML_WRITE_END_ELEMENT(xml_text_writer);

    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterEndDocument\n", __FUNCTION__);
    }

    if (!xml_text_writer_param) {
      xmlFreeTextWriter(xml_text_writer);
    }
  }
}

//------------------------------------------------------------------------------
void xml_msg_dump_itti_s1ap_ue_context_release_complete(const itti_s1ap_ue_context_release_complete_t * const itti_msg,
    int sender_task,
    int receiver_task,
    xmlTextWriterPtr xml_text_writer_param)
{
  xmlTextWriterPtr xml_text_writer = NULL;
  int              rc = RETURNok;
  struct timeval   elapsed_time = {0};
  unsigned long    msg_num = xml_msg_dump_get_seq_uid();

  shared_log_get_elapsed_time_since_start(&elapsed_time);

  if (!xml_text_writer_param) {
    xml_text_writer = xml_msg_dump_itti_get_new_xml_text_writter(msg_num, ITTI_S1AP_UE_CONTEXT_RELEASE_COMPLETE_XML_STR);
  } else {
    xml_text_writer = xml_text_writer_param;
  }

  if (xml_text_writer) {

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(xml_text_writer, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterStartDocument\n", __FUNCTION__);
      xmlFreeTextWriter(xml_text_writer);
      return;
    }

    XML_WRITE_START_ELEMENT(xml_text_writer, ITTI_S1AP_UE_CONTEXT_RELEASE_COMPLETE_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ACTION_XML_STR, "%s", ACTION_SEND_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_SENDER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(sender_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_RECEIVER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(receiver_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, TIMESTAMP_XML_STR, "%lu.%lu", elapsed_time.tv_sec, elapsed_time.tv_usec);

    enb_ue_s1ap_id_t  enb_ue_s1ap_id = itti_msg->enb_ue_s1ap_id;
    enb_ue_s1ap_id_to_xml(&enb_ue_s1ap_id, xml_text_writer);
    mme_ue_s1ap_id_to_xml(&itti_msg->mme_ue_s1ap_id, xml_text_writer);

    XML_WRITE_END_ELEMENT(xml_text_writer);

    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterEndDocument\n", __FUNCTION__);
    }

    if (!xml_text_writer_param) {
      xmlFreeTextWriter(xml_text_writer);
    }
  }
}

//------------------------------------------------------------------------------
void xml_msg_dump_itti_mme_app_initial_ue_message(
    const itti_mme_app_initial_ue_message_t * const initial_pP,
    const int sender_task,
    const int receiver_task,
    xmlTextWriterPtr xml_text_writer_param)
{
  xmlTextWriterPtr xml_text_writer = NULL;
  int              rc = RETURNok;
  struct timeval   elapsed_time = {0};
  unsigned long    msg_num = xml_msg_dump_get_seq_uid();

  shared_log_get_elapsed_time_since_start(&elapsed_time);

  if (!xml_text_writer_param) {
    xml_text_writer = xml_msg_dump_itti_get_new_xml_text_writter(msg_num, ITTI_MME_APP_INITIAL_UE_MESSAGE_XML_STR);
  } else {
    xml_text_writer = xml_text_writer_param;
  }

  if (xml_text_writer) {

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(xml_text_writer, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterStartDocument\n", __FUNCTION__);
      xmlFreeTextWriter(xml_text_writer);
      return;
    }

    XML_WRITE_START_ELEMENT(xml_text_writer, ITTI_MME_APP_INITIAL_UE_MESSAGE_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ACTION_XML_STR, "%s", ACTION_SEND_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_SENDER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(sender_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_RECEIVER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(receiver_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, TIMESTAMP_XML_STR, "%lu.%lu", elapsed_time.tv_sec, elapsed_time.tv_usec);

    // mandatory attributes of initial_ue_message
    enb_ue_s1ap_id_to_xml(&initial_pP->enb_ue_s1ap_id, xml_text_writer);
    nas_pdu_to_xml(initial_pP->nas, xml_text_writer);
    tracking_area_identity_to_xml(&initial_pP->tai, xml_text_writer);
    ecgi_to_xml(&initial_pP->cgi, xml_text_writer);
    rrc_establishment_cause_to_xml(&initial_pP->rrc_establishment_cause, xml_text_writer);

    // optional attributes
    if (initial_pP->is_s_tmsi_valid) {
      s_tmsi_to_xml(&initial_pP->opt_s_tmsi, xml_text_writer);
    }
    if (initial_pP->is_csg_id_valid) {
      csg_id_to_xml(&initial_pP->opt_csg_id, xml_text_writer);
    }
    if (initial_pP->is_gummei_valid) {
      gummei_to_xml(&initial_pP->opt_gummei, xml_text_writer);
    }
    // optional Cell Access Mode
    // optional GW Transport Layer Address
    // optional Relay Node Indicator
    XML_WRITE_END_ELEMENT(xml_text_writer);

    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterEndDocument\n", __FUNCTION__);
    }

    if (!xml_text_writer_param) {
      xmlFreeTextWriter(xml_text_writer);
    }
  }
}

//------------------------------------------------------------------------------
void xml_msg_dump_itti_mme_app_initial_context_setup_rsp(const itti_mme_app_initial_context_setup_rsp_t * const itti_msg,
    int sender_task,
    int receiver_task,
    xmlTextWriterPtr xml_text_writer_param)
{
  xmlTextWriterPtr xml_text_writer = NULL;
  int              rc = RETURNok;
  struct timeval   elapsed_time = {0};
  unsigned long    msg_num = xml_msg_dump_get_seq_uid();

  shared_log_get_elapsed_time_since_start(&elapsed_time);

  if (!xml_text_writer_param) {
    xml_text_writer = xml_msg_dump_itti_get_new_xml_text_writter(msg_num, ITTI_MME_APP_INITIAL_CONTEXT_SETUP_RSP_XML_STR);
  } else {
    xml_text_writer = xml_text_writer_param;
  }

  if (xml_text_writer) {

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(xml_text_writer, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterStartDocument\n", __FUNCTION__);
      xmlFreeTextWriter(xml_text_writer);
      return;
    }

    XML_WRITE_START_ELEMENT(xml_text_writer, ITTI_MME_APP_INITIAL_CONTEXT_SETUP_RSP_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ACTION_XML_STR, "%s", ACTION_SEND_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_SENDER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(sender_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_RECEIVER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(receiver_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, TIMESTAMP_XML_STR, "%lu.%lu", elapsed_time.tv_sec, elapsed_time.tv_usec);

    // mandatory attributes of initial_ue_message
    mme_ue_s1ap_id_to_xml(&itti_msg->mme_ue_s1ap_id, xml_text_writer);
    for (int i=0; i < itti_msg->no_of_e_rabs; i++) {
      XML_WRITE_START_ELEMENT(xml_text_writer, E_RAB_SETUP_ITEM_IE_XML_STR);
      eps_bearer_identity_to_xml(&itti_msg->e_rab_id[i], xml_text_writer);
      XML_WRITE_FORMAT_ELEMENT(xml_text_writer, TEID_IE_XML_STR, "%"PRIx32, itti_msg->gtp_teid[i]);
      XML_WRITE_HEX_ELEMENT(xml_text_writer, TRANSPORT_LAYER_ADDRESS_IE_XML_STR,
          bdata(itti_msg->transport_layer_address[i]),
          blength(itti_msg->transport_layer_address[i]));
      XML_WRITE_END_ELEMENT(xml_text_writer);
    }
    XML_WRITE_END_ELEMENT(xml_text_writer);

    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterEndDocument\n", __FUNCTION__);
    }
    if (!xml_text_writer_param) {
      xmlFreeTextWriter(xml_text_writer);
    }
  }
}

//------------------------------------------------------------------------------
void xml_msg_dump_itti_mme_app_connection_establishment_cnf(const itti_mme_app_connection_establishment_cnf_t * const itti_msg,
    int sender_task,
    int receiver_task,
    xmlTextWriterPtr xml_text_writer_param)
{
  xmlTextWriterPtr xml_text_writer = NULL;
  int              rc = RETURNok;
  struct timeval   elapsed_time = {0};
  unsigned long    msg_num = xml_msg_dump_get_seq_uid();

  shared_log_get_elapsed_time_since_start(&elapsed_time);

  if (!xml_text_writer_param) {
    xml_text_writer = xml_msg_dump_itti_get_new_xml_text_writter(msg_num, ITTI_MME_APP_CONNECTION_ESTABLISHMENT_CNF_XML_STR);
  } else {
    xml_text_writer = xml_text_writer_param;
  }

  if (xml_text_writer) {

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(xml_text_writer, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterStartDocument\n", __FUNCTION__);
      xmlFreeTextWriter(xml_text_writer);
      return;
    }

    XML_WRITE_START_ELEMENT(xml_text_writer, ITTI_MME_APP_CONNECTION_ESTABLISHMENT_CNF_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ACTION_XML_STR, "%s", ACTION_WAIT_RECEIVE_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_SENDER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(sender_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_RECEIVER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(receiver_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, TIMESTAMP_XML_STR, "%lu.%lu", elapsed_time.tv_sec, elapsed_time.tv_usec);

    // mandatory attributes of initial_ue_message
    mme_ue_s1ap_id_to_xml(&itti_msg->ue_id, xml_text_writer);
    for (int i=0; i < itti_msg->no_of_e_rabs; i++) {
      XML_WRITE_START_ELEMENT(xml_text_writer, E_RAB_SETUP_ITEM_IE_XML_STR);
      eps_bearer_identity_to_xml(&itti_msg->e_rab_id[i], xml_text_writer);
      XML_WRITE_FORMAT_ELEMENT(xml_text_writer, QCI_XML_STR, "%"PRIu8, itti_msg->e_rab_level_qos_qci[i]);
      XML_WRITE_FORMAT_ELEMENT(xml_text_writer, PRIORITY_LEVEL_XML_STR, "%"PRIu8, itti_msg->e_rab_level_qos_priority_level[i]);
      XML_WRITE_FORMAT_ELEMENT(xml_text_writer, PREEMPTION_CAPABILITY_XML_STR, "%"PRIu8, itti_msg->e_rab_level_qos_preemption_capability[i]);
      XML_WRITE_FORMAT_ELEMENT(xml_text_writer, PREEMPTION_VULNERABILITY_XML_STR, "%"PRIu8, itti_msg->e_rab_level_qos_preemption_vulnerability[i]);
      XML_WRITE_HEX_ELEMENT(xml_text_writer, TRANSPORT_LAYER_ADDRESS_IE_XML_STR,
          bdata(itti_msg->transport_layer_address[i]),
          blength(itti_msg->transport_layer_address[i]));
      XML_WRITE_FORMAT_ELEMENT(xml_text_writer, TEID_IE_XML_STR, "%"PRIx32, itti_msg->gtp_teid[i]);
      nas_pdu_to_xml(itti_msg->nas_pdu[i], xml_text_writer);
      XML_WRITE_END_ELEMENT(xml_text_writer);
    }
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, UE_SECURITY_CAPABILITIES_ENCRYPTION_ALGORITHMS_XML_STR, "%"PRIx16, itti_msg->ue_security_capabilities_encryption_algorithms);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, UE_SECURITY_CAPABILITIES_INTEGRITY_PROTECTION_ALGORITHMS_XML_STR, "%"PRIx16, itti_msg->ue_security_capabilities_integrity_algorithms);
    XML_WRITE_HEX_ELEMENT(xml_text_writer, KENB_XML_STR, itti_msg->kenb, AUTH_KASME_SIZE);

    XML_WRITE_END_ELEMENT(xml_text_writer);

    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterEndDocument\n", __FUNCTION__);
    }
    if (!xml_text_writer_param) {
      xmlFreeTextWriter(xml_text_writer);
    }
  }
}

//------------------------------------------------------------------------------
void xml_msg_dump_itti_nas_uplink_data_ind(const itti_nas_ul_data_ind_t * const itti_msg,
    int sender_task,
    int receiver_task,
    xmlTextWriterPtr xml_text_writer_param)
{
  xmlTextWriterPtr xml_text_writer = NULL;
  int              rc = RETURNok;
  struct timeval   elapsed_time = {0};
  unsigned long    msg_num = xml_msg_dump_get_seq_uid();

  shared_log_get_elapsed_time_since_start(&elapsed_time);

  if (!xml_text_writer_param) {
    xml_text_writer = xml_msg_dump_itti_get_new_xml_text_writter(msg_num, ITTI_NAS_UPLINK_DATA_IND_XML_STR);
  } else {
    xml_text_writer = xml_text_writer_param;
  }

  if (xml_text_writer) {

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(xml_text_writer, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterStartDocument\n", __FUNCTION__);
      xmlFreeTextWriter(xml_text_writer);
      return;
    }

    XML_WRITE_START_ELEMENT(xml_text_writer, ITTI_NAS_UPLINK_DATA_IND_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ACTION_XML_STR, "%s", ACTION_SEND_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_SENDER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(sender_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_RECEIVER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(receiver_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, TIMESTAMP_XML_STR, "%lu.%lu", elapsed_time.tv_sec, elapsed_time.tv_usec);

    // mandatory attributes of nas_uplink_data_ind
    mme_ue_s1ap_id_to_xml(&itti_msg->ue_id, xml_text_writer);
    nas_pdu_to_xml(itti_msg->nas_msg, xml_text_writer);
    tracking_area_identity_to_xml(&itti_msg->tai, xml_text_writer);
    ecgi_to_xml(&itti_msg->cgi, xml_text_writer);
    XML_WRITE_END_ELEMENT(xml_text_writer);
    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterEndDocument\n", __FUNCTION__);
    }
    if (!xml_text_writer_param) {
      xmlFreeTextWriter(xml_text_writer);
    }
  }
}
//------------------------------------------------------------------------------
void xml_msg_dump_itti_nas_downlink_data_req(const itti_nas_dl_data_req_t * const itti_msg,
    int sender_task,
    int receiver_task,
    xmlTextWriterPtr xml_text_writer_param)
{
  xmlTextWriterPtr xml_text_writer = NULL;
  int              rc = RETURNok;
  struct timeval   elapsed_time = {0};
  unsigned long    msg_num = xml_msg_dump_get_seq_uid();

  shared_log_get_elapsed_time_since_start(&elapsed_time);

  if (!xml_text_writer_param) {
    xml_text_writer = xml_msg_dump_itti_get_new_xml_text_writter(msg_num, ITTI_NAS_DOWNLINK_DATA_REQ_XML_STR);
  } else {
    xml_text_writer = xml_text_writer_param;
  }

  if (xml_text_writer) {

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(xml_text_writer, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterStartDocument\n", __FUNCTION__);
      xmlFreeTextWriter(xml_text_writer);
      return;
    }

    XML_WRITE_START_ELEMENT(xml_text_writer, ITTI_NAS_DOWNLINK_DATA_REQ_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ACTION_XML_STR, "%s", ACTION_WAIT_RECEIVE_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_SENDER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(sender_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_RECEIVER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(receiver_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, TIMESTAMP_XML_STR, "%lu.%lu", elapsed_time.tv_sec, elapsed_time.tv_usec);

    // mandatory attributes of itti_nas_dl_data_rej_t
    mme_ue_s1ap_id_to_xml(&itti_msg->ue_id, xml_text_writer);
    enb_ue_s1ap_id_to_xml(&itti_msg->enb_ue_s1ap_id, xml_text_writer);
    nas_pdu_to_xml(itti_msg->nas_msg, xml_text_writer);
    // TODO optional handover Restriction list
    // TODO optional Subscriber Profile ID for RAT/Frequency priority
    XML_WRITE_END_ELEMENT(xml_text_writer);

    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterEndDocument\n", __FUNCTION__);
    }

    if (!xml_text_writer_param) {
      xmlFreeTextWriter(xml_text_writer);
    }
  }
}
//------------------------------------------------------------------------------
void xml_msg_dump_itti_nas_downlink_data_rej(const itti_nas_dl_data_rej_t * const itti_msg,
    int sender_task,
    int receiver_task,
    xmlTextWriterPtr xml_text_writer_param)
{
  xmlTextWriterPtr xml_text_writer = NULL;
  int              rc = RETURNok;
  struct timeval   elapsed_time = {0};
  unsigned long    msg_num = xml_msg_dump_get_seq_uid();

  shared_log_get_elapsed_time_since_start(&elapsed_time);

  if (!xml_text_writer_param) {
    xml_text_writer = xml_msg_dump_itti_get_new_xml_text_writter(msg_num, ITTI_NAS_DOWNLINK_DATA_REJ_XML_STR);
  } else {
    xml_text_writer = xml_text_writer_param;
  }

  if (xml_text_writer) {

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(xml_text_writer, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterStartDocument\n", __FUNCTION__);
      xmlFreeTextWriter(xml_text_writer);
      return;
    }

    XML_WRITE_START_ELEMENT(xml_text_writer, ITTI_NAS_DOWNLINK_DATA_REJ_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ACTION_XML_STR, "%s", ACTION_SEND_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_SENDER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(sender_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_RECEIVER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(receiver_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, TIMESTAMP_XML_STR, "%lu.%lu", elapsed_time.tv_sec, elapsed_time.tv_usec);

    // mandatory attributes of itti_nas_dl_data_rej_t
    mme_ue_s1ap_id_to_xml(&itti_msg->ue_id, xml_text_writer);
    nas_pdu_to_xml(itti_msg->nas_msg, xml_text_writer);
    XML_WRITE_END_ELEMENT(xml_text_writer);

    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterEndDocument\n", __FUNCTION__);
    }

    if (!xml_text_writer_param) {
      xmlFreeTextWriter(xml_text_writer);
    }
  }
}
//------------------------------------------------------------------------------
void xml_msg_dump_itti_nas_downlink_data_cnf(const itti_nas_dl_data_cnf_t * const itti_msg,
    int sender_task,
    int receiver_task,
    xmlTextWriterPtr xml_text_writer_param)
{
  xmlTextWriterPtr xml_text_writer = NULL;
  int              rc = RETURNok;
  struct timeval   elapsed_time = {0};
  unsigned long    msg_num = xml_msg_dump_get_seq_uid();

  shared_log_get_elapsed_time_since_start(&elapsed_time);

  if (!xml_text_writer_param) {
    xml_text_writer = xml_msg_dump_itti_get_new_xml_text_writter(msg_num, ITTI_NAS_DOWNLINK_DATA_CNF_XML_STR);
  } else {
    xml_text_writer = xml_text_writer_param;
  }

  if (xml_text_writer) {

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(xml_text_writer, NULL, XML_ENCODING, NULL);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterStartDocument\n", __FUNCTION__);
      xmlFreeTextWriter(xml_text_writer);
      return;
    }

    XML_WRITE_START_ELEMENT(xml_text_writer, ITTI_NAS_DOWNLINK_DATA_CNF_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ACTION_XML_STR, "%s", ACTION_SEND_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_SENDER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(sender_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, ITTI_RECEIVER_TASK_XML_STR, "%s", itti_task_id2itti_task_str(receiver_task));
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, TIMESTAMP_XML_STR, "%lu.%lu", elapsed_time.tv_sec, elapsed_time.tv_usec);

    // mandatory attributes of itti_nas_dl_data_cnf_t
    mme_ue_s1ap_id_to_xml(&itti_msg->ue_id, xml_text_writer);
    XML_WRITE_FORMAT_ELEMENT(xml_text_writer, NAS_ERROR_CODE_XML_STR, "%"PRIx8, itti_msg->err_code);
    XML_WRITE_END_ELEMENT(xml_text_writer);

    rc = xmlTextWriterEndDocument(xml_text_writer);
    if (rc < 0) {
      OAI_FPRINTF_ERR("%s: Error at xmlTextWriterEndDocument\n", __FUNCTION__);
    }

    if (!xml_text_writer_param) {
      xmlFreeTextWriter(xml_text_writer);
    }
  }
}


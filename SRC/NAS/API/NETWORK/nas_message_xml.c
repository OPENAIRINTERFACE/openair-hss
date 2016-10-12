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
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "assertions.h"
#include "dynamic_memory_check.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_24.301.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "security_types.h"
#include "common_defs.h"
#include "common_types.h"
#include "xml_load.h"
#include "3gpp_24.301_emm_ies_xml.h"
#include "3gpp_24.301_emm_msg_xml.h"
#include "3gpp_24.301_esm_msg_xml.h"
#include "3gpp_24.301_ies_xml.h"
#include "3gpp_24.301_esm_ies_xml.h"
#include "3gpp_36.401_xml.h"
#include "3gpp_24.007_xml.h"
#include "3gpp_24.008_xml.h"
#include "log.h"
#include "as_message.h"
#include "nas_message.h"
#include "nas_message_xml.h"

#include "3gpp_24.301_ies_xml.h"
#include "mme_api.h"
#include "mme_config.h"
#include "emm_main.h"
#include "esm_main.h"
#include "emm_sap.h"
#include "emm_data.h"
#include "xml2_wrapper.h"
#include "xml_load.h"



//------------------------------------------------------------------------------
bool nas_message_plain_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    nas_message_plain_t       * const nas_message)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",PLAIN_NAS_MESSAGE_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));


      eps_protocol_discriminator_t protocol_discriminator = 0;
      if (res) {
        res = protocol_discriminator_from_xml (xml_doc, xpath_ctx, &protocol_discriminator);
      }
      if (res) {
        if (EPS_MOBILITY_MANAGEMENT_MESSAGE == protocol_discriminator) {
          // dump EPS Mobility Management L3 message
          res = emm_msg_from_xml (xml_doc, xpath_ctx, &nas_message->emm);
        } else if (EPS_SESSION_MANAGEMENT_MESSAGE == protocol_discriminator) {
          // Dump EPS Session Management L3 message
          res = esm_msg_from_xml (xml_doc, xpath_ctx, &nas_message->esm);
        } else {
          /*
           * Discard L3 messages with not supported protocol discriminator
           */
          OAILOG_WARNING(LOG_XML, "NET-API   - Protocol discriminator 0x%x is " "not supported\n", nas_message->emm.header.protocol_discriminator);
        }
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
static int nas_message_plain_to_xml (
    const unsigned char *buffer,
    nas_message_plain_t * msg,
    const size_t length,
    xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  int                                     bytes = TLV_PROTOCOL_NOT_SUPPORTED;

  // really need to change this
  msg->emm.header.protocol_discriminator =  *buffer & 0x0f;

  XML_WRITE_START_ELEMENT(writer,  PLAIN_NAS_MESSAGE_IE_XML_STR);

  if (msg->emm.header.protocol_discriminator == EPS_MOBILITY_MANAGEMENT_MESSAGE) {
    // dump EPS Mobility Management L3 message
    bytes = emm_msg_to_xml (&msg->emm, (uint8_t *) buffer, length, writer);
  } else if (msg->esm.header.protocol_discriminator == EPS_SESSION_MANAGEMENT_MESSAGE) {
    // Dump EPS Session Management L3 message
    bytes = esm_msg_to_xml (&msg->esm, (uint8_t *) buffer, length, writer);
  } else {
    /*
     * Discard L3 messages with not supported protocol discriminator
     */
    OAILOG_WARNING(LOG_XML, "NET-API   - Protocol discriminator 0x%x is " "not supported\n", msg->emm.header.protocol_discriminator);
  }
  XML_WRITE_END_ELEMENT(writer);
  OAILOG_FUNC_RETURN (LOG_XML, bytes);
}

//------------------------------------------------------------------------------
bool nas_message_protected_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    nas_message_t             * const nas_message)
{
  OAILOG_FUNC_IN (LOG_XML);
  bool res = false;
  bstring xpath_expr = bformat("./%s",SECURITY_PROTECTED_NAS_MESSAGE_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size)  && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));

      uint8_t security_header_type = 0;
      res = security_header_type_from_xml (xml_doc, xpath_ctx, &security_header_type, NULL);

      eps_protocol_discriminator_t protocol_discriminator = 0;
      if (res) {
        nas_message->security_protected.header.security_header_type = security_header_type;
        res = protocol_discriminator_from_xml (xml_doc, xpath_ctx, &protocol_discriminator);
      }

      uint32_t message_authentication_code = 0;
      if (res) {
        nas_message->security_protected.header.protocol_discriminator = protocol_discriminator;
        res = mac_from_xml (xml_doc, xpath_ctx, &message_authentication_code);
      }

      uint8_t sequence_number = 0;
      if (res) {
        nas_message->security_protected.header.message_authentication_code = message_authentication_code;
        res = sequence_number_from_xml (xml_doc, xpath_ctx, &sequence_number);
      }

      if (res) {
        nas_message->security_protected.header.sequence_number = sequence_number;
        res = nas_message_plain_from_xml (xml_doc, xpath_ctx, &nas_message->security_protected.plain);
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;

    }
    xmlXPathFreeObject(xpath_obj);
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_XML, res);
}
//------------------------------------------------------------------------------
static int nas_message_protected_to_xml(
    unsigned char * const buffer,
    nas_message_t * nas_message,
    size_t length,
    emm_security_context_t * const emm_security_context,
    nas_message_decode_status_t * const status,
    xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  int                                     bytes = TLV_BUFFER_TOO_SHORT;

  XML_WRITE_START_ELEMENT(writer,  SECURITY_PROTECTED_NAS_MESSAGE_IE_XML_STR);
  security_header_type_t sht = nas_message->security_protected.header.security_header_type;
  security_header_type_to_xml(&sht, writer);
  eps_protocol_discriminator_t pd = nas_message->security_protected.header.protocol_discriminator;
  protocol_discriminator_to_xml(&pd, writer);
  mac_to_xml(&nas_message->security_protected.header.message_authentication_code, writer);
  sequence_number_to_xml(&nas_message->security_protected.header.sequence_number , writer);

  // we assume msg is not encrypted or encrypted with EEA0
  bytes = nas_message_plain_to_xml (buffer, &nas_message->security_protected.plain, length , writer);

  XML_WRITE_END_ELEMENT(writer);

  OAILOG_FUNC_RETURN (LOG_XML, bytes);
}

//------------------------------------------------------------------------------
bool nas_pdu_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    bstring                    const *bnas_pdu)
{
  OAILOG_FUNC_IN (LOG_XML);
  nas_message_t                           nas_msg;
  memset((void*)&nas_msg, 0, sizeof(nas_msg));

  if (nas_message_protected_from_xml(xml_doc, xpath_ctx, &nas_msg)) {
    OAILOG_FUNC_RETURN (LOG_XML, true);
  } else if (nas_message_plain_from_xml(xml_doc, xpath_ctx, &nas_msg.plain)) {
    OAILOG_FUNC_RETURN (LOG_XML, true);
  }
  OAILOG_FUNC_RETURN (LOG_XML, false);
}
//------------------------------------------------------------------------------
void nas_pdu_to_xml (bstring bnas_pdu, xmlTextWriterPtr writer)
{
  OAILOG_FUNC_IN (LOG_XML);
  unsigned char                          *buffer = (unsigned char*)bdata(bnas_pdu);
  size_t                                  length = blength(bnas_pdu);
  emm_security_context_t                 *emm_security_context = NULL; // TODO maybe later
  uint32_t                                mac   = 0;
  int                                     size  = 0;
  bool                                    is_sr = false;
  nas_message_decode_status_t             decode_status = {0};
  nas_message_t                           nas_msg = {.security_protected.header = {0},
                                                     .security_protected.plain.emm.header = {0},
                                                     .security_protected.plain.esm.header = {0}};

  AssertFatal((bdata(bnas_pdu))&& (0<blength(bnas_pdu)), "No PDU to dump");
  // Decode the header
  size  = nas_message_header_decode (buffer, &nas_msg.header, length, &decode_status, &is_sr);

  if (size < 0) {
    OAILOG_WARNING(LOG_XML, "Message header %lu bytes is too short\n", length);
    OAILOG_FUNC_OUT (LOG_XML);
  } else
  // SERVICE REQUEST
  if (is_sr) {
    if (length < NAS_MESSAGE_SERVICE_REQUEST_SECURITY_HEADER_SIZE) {
      /*
       * The buffer is not big enough to contain security header
       */
      OAILOG_WARNING(LOG_XML, "Message header %lu bytes is too short %u bytes\n", length, NAS_MESSAGE_SECURITY_HEADER_SIZE);
      OAILOG_FUNC_OUT (LOG_XML);
    }
    /*
     * Decode the sequence number + ksi: be careful
     */
    DECODE_U8 (buffer + size, nas_msg.header.sequence_number, size);
    /*
     * Decode the message authentication code
     */
    DECODE_U16 (buffer + size, mac, size);

    // sequence number
    uint8_t sequence_number = nas_msg.header.sequence_number;
    // sequence number

    //shortcut
    nas_msg.plain.emm.service_request.ksiandsequencenumber.ksi  = sequence_number >> 5;
    nas_msg.plain.emm.service_request.ksiandsequencenumber.sequencenumber  = sequence_number & 0x1F;
    nas_msg.plain.emm.service_request.messageauthenticationcode = mac;
    nas_msg.plain.emm.service_request.protocoldiscriminator     = EPS_MOBILITY_MANAGEMENT_MESSAGE;
    nas_msg.plain.emm.service_request.securityheadertype        = SECURITY_HEADER_TYPE_SERVICE_REQUEST;
    nas_msg.plain.emm.service_request.messagetype               =  SERVICE_REQUEST;

    XML_WRITE_START_ELEMENT(writer,  SECURITY_PROTECTED_NAS_MESSAGE_IE_XML_STR);
    security_header_type_t sht = nas_msg.header.security_header_type;
    security_header_type_to_xml(&sht, writer);
    eps_protocol_discriminator_t pd = nas_msg.plain.emm.service_request.protocoldiscriminator;
    protocol_discriminator_to_xml(&pd, writer);
    ksi_and_sequence_number_to_xml (&nas_msg.plain.emm.service_request.ksiandsequencenumber,  writer);
    short_mac_t sm = mac;
    short_mac_to_xml(&sm, writer);
    XML_WRITE_END_ELEMENT(writer);
  } else if (size > 1) {
    // found security header

    /*
     * Decode security protected NAS message
     */
    nas_message_protected_to_xml((unsigned char *const)(buffer + size),
        &nas_msg, length - size, emm_security_context, &decode_status, writer);
  } else {
    /*
     * Decode plain NAS message
     */
    nas_message_plain_to_xml(buffer, &nas_msg.plain, length, writer);
  }
  OAILOG_FUNC_OUT (LOG_XML);
}

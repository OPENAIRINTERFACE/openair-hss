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

/*! \file 3gpp_24.301_ies_xml.c
   \brief EMM msg C struct to/from XML functions
   \author  Lionel GAUTHIER
   \date 2016
   \email: lionel.gauthier@eurecom.fr
*/

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "log.h"
#include "common_defs.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "xml2_wrapper.h"
#include "3gpp_24.301.h"
#include "3gpp_24.301_ies_xml.h"


NUM_FROM_XML_GENERATE(security_header_type, SECURITY_HEADER_TYPE);
//------------------------------------------------------------------------------
void security_header_type_to_xml ( const security_header_type_t * const security_header_type, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, SECURITY_HEADER_TYPE_XML_STR, SECURITY_HEADER_TYPE_XML_FMT, *security_header_type);
}

//------------------------------------------------------------------------------
bool mac_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    uint32_t                  * const mac)
{
  bstring xpath_expr = bformat("./%s",MAC_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx32, (void*)mac, NULL);
  bdestroy_wrapper (&xpath_expr);
  return res;
}
//------------------------------------------------------------------------------
void mac_to_xml ( const uint32_t * const mac, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, MAC_XML_STR, MAC_FMT, *mac);
}
//------------------------------------------------------------------------------
bool message_type_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, message_type_t * const messagetype)
{
  char message_type_str[128]  = {0};
  bstring xpath_expr = bformat("./%s",MESSAGE_TYPE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)message_type_str, NULL);
  bdestroy_wrapper (&xpath_expr);
  *messagetype = 0;
  if (res) {
    if (!strcasecmp(ATTACH_REQUEST_VAL_XML_STR, message_type_str))  {*messagetype = ATTACH_REQUEST;return res;}
    if (!strcasecmp(ATTACH_ACCEPT_VAL_XML_STR,  message_type_str))  {*messagetype = ATTACH_ACCEPT;return res;}
    if (!strcasecmp(ATTACH_COMPLETE_VAL_XML_STR, message_type_str)) {*messagetype = ATTACH_COMPLETE;return res;}
    if (!strcasecmp(ATTACH_REJECT_VAL_XML_STR, message_type_str))   {*messagetype = ATTACH_REJECT;return res;}
    if (!strcasecmp(DETACH_REQUEST_VAL_XML_STR, message_type_str))  {*messagetype = DETACH_REQUEST;return res;}
    if (!strcasecmp(DETACH_ACCEPT_VAL_XML_STR, message_type_str))   {*messagetype = DETACH_ACCEPT;return res;}
    if (!strcasecmp(TRACKING_AREA_UPDATE_REQUEST_VAL_XML_STR, message_type_str)) {*messagetype = TRACKING_AREA_UPDATE_REQUEST;return res;}
    if (!strcasecmp(TRACKING_AREA_UPDATE_ACCEPT_VAL_XML_STR, message_type_str)) {*messagetype = TRACKING_AREA_UPDATE_ACCEPT;return res;}
    if (!strcasecmp(TRACKING_AREA_UPDATE_COMPLETE_VAL_XML_STR, message_type_str)) {*messagetype = TRACKING_AREA_UPDATE_COMPLETE;return res;}
    if (!strcasecmp(TRACKING_AREA_UPDATE_REJECT_VAL_XML_STR, message_type_str)) {*messagetype = TRACKING_AREA_UPDATE_REJECT;return res;}
    if (!strcasecmp(EXTENDED_SERVICE_REQUEST_VAL_XML_STR, message_type_str)) {*messagetype = EXTENDED_SERVICE_REQUEST;return res;}
    if (!strcasecmp(SERVICE_REJECT_VAL_XML_STR, message_type_str)) {*messagetype = SERVICE_REJECT;return res;}
    if (!strcasecmp(GUTI_REALLOCATION_COMMAND_VAL_XML_STR, message_type_str)) {*messagetype = GUTI_REALLOCATION_COMMAND;return res;}
    if (!strcasecmp(GUTI_REALLOCATION_COMPLETE_VAL_XML_STR, message_type_str)) {*messagetype = GUTI_REALLOCATION_COMPLETE;return res;}
    if (!strcasecmp(AUTHENTICATION_REQUEST_VAL_XML_STR, message_type_str)) {*messagetype = AUTHENTICATION_REQUEST;return res;}
    if (!strcasecmp(AUTHENTICATION_RESPONSE_VAL_XML_STR, message_type_str)) {*messagetype = AUTHENTICATION_RESPONSE;return res;}
    if (!strcasecmp(AUTHENTICATION_REJECT_VAL_XML_STR, message_type_str)) {*messagetype = AUTHENTICATION_REJECT;return res;}
    if (!strcasecmp(AUTHENTICATION_FAILURE_VAL_XML_STR, message_type_str)) {*messagetype = AUTHENTICATION_FAILURE;return res;}
    if (!strcasecmp(IDENTITY_REQUEST_VAL_XML_STR, message_type_str)) {*messagetype = IDENTITY_REQUEST;return res;}
    if (!strcasecmp(IDENTITY_RESPONSE_VAL_XML_STR, message_type_str)) {*messagetype = IDENTITY_RESPONSE;return res;}
    if (!strcasecmp(SECURITY_MODE_COMMAND_VAL_XML_STR, message_type_str)) {*messagetype = SECURITY_MODE_COMMAND;return res;}
    if (!strcasecmp(SECURITY_MODE_COMPLETE_VAL_XML_STR, message_type_str)) {*messagetype = SECURITY_MODE_COMPLETE;return res;}
    if (!strcasecmp(SECURITY_MODE_REJECT_VAL_XML_STR, message_type_str)) {*messagetype = SECURITY_MODE_REJECT;return res;}
    if (!strcasecmp(EMM_STATUS_VAL_XML_STR, message_type_str)) {*messagetype = EMM_STATUS;return res;}
    if (!strcasecmp(EMM_INFORMATION_VAL_XML_STR, message_type_str)) {*messagetype = EMM_INFORMATION;return res;}
    if (!strcasecmp(DOWNLINK_NAS_TRANSPORT_VAL_XML_STR, message_type_str)) {*messagetype = DOWNLINK_NAS_TRANSPORT;return res;}
    if (!strcasecmp(UPLINK_NAS_TRANSPORT_VAL_XML_STR, message_type_str)) {*messagetype = UPLINK_NAS_TRANSPORT;return res;}
    if (!strcasecmp(CS_SERVICE_NOTIFICATION_VAL_XML_STR, message_type_str)) {*messagetype = CS_SERVICE_NOTIFICATION;return res;}
    if (!strcasecmp(DOWNLINK_GENERIC_NAS_TRANSPORT_VAL_XML_STR, message_type_str)) {*messagetype = DOWNLINK_GENERIC_NAS_TRANSPORT;return res;}
    if (!strcasecmp(UPLINK_GENERIC_NAS_TRANSPORT_VAL_XML_STR, message_type_str)) {*messagetype = UPLINK_GENERIC_NAS_TRANSPORT;return res;}

    if (!strcasecmp(ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_VAL_XML_STR, message_type_str)) {*messagetype = ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST;return res;}
    if (!strcasecmp(ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT_VAL_XML_STR, message_type_str)) {*messagetype = ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT;return res;}
    if (!strcasecmp(ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT_VAL_XML_STR, message_type_str)) {*messagetype = ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT;return res;}
    if (!strcasecmp(ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_VAL_XML_STR, message_type_str)) {*messagetype = ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST;return res;}
    if (!strcasecmp(ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT_VAL_XML_STR, message_type_str)) {*messagetype = ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT;return res;}
    if (!strcasecmp(ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT_VAL_XML_STR, message_type_str)) {*messagetype = ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT;return res;}
    if (!strcasecmp(MODIFY_EPS_BEARER_CONTEXT_REQUEST_VAL_XML_STR, message_type_str)) {*messagetype = MODIFY_EPS_BEARER_CONTEXT_REQUEST;return res;}
    if (!strcasecmp(MODIFY_EPS_BEARER_CONTEXT_ACCEPT_VAL_XML_STR, message_type_str)) {*messagetype = MODIFY_EPS_BEARER_CONTEXT_ACCEPT;return res;}
    if (!strcasecmp(MODIFY_EPS_BEARER_CONTEXT_REJECT_VAL_XML_STR, message_type_str)) {*messagetype = MODIFY_EPS_BEARER_CONTEXT_REJECT;return res;}
    if (!strcasecmp(DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST_VAL_XML_STR, message_type_str)) {*messagetype = DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST;return res;}
    if (!strcasecmp(DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT_VAL_XML_STR, message_type_str)) {*messagetype = DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT;return res;}
    if (!strcasecmp(PDN_CONNECTIVITY_REQUEST_VAL_XML_STR, message_type_str)) {*messagetype = PDN_CONNECTIVITY_REQUEST;return res;}
    if (!strcasecmp(PDN_CONNECTIVITY_REJECT_VAL_XML_STR, message_type_str)) {*messagetype = PDN_CONNECTIVITY_REJECT;return res;}
    if (!strcasecmp(PDN_DISCONNECT_REQUEST_VAL_XML_STR, message_type_str)) {*messagetype = PDN_DISCONNECT_REQUEST;return res;}
    if (!strcasecmp(PDN_DISCONNECT_REJECT_VAL_XML_STR, message_type_str)) {*messagetype = PDN_DISCONNECT_REJECT;return res;}
    if (!strcasecmp(BEARER_RESOURCE_ALLOCATION_REQUEST_VAL_XML_STR, message_type_str)) {*messagetype = BEARER_RESOURCE_ALLOCATION_REQUEST;return res;}
    if (!strcasecmp(BEARER_RESOURCE_ALLOCATION_REJECT_VAL_XML_STR, message_type_str)) {*messagetype = BEARER_RESOURCE_ALLOCATION_REJECT;return res;}
    if (!strcasecmp(BEARER_RESOURCE_MODIFICATION_REQUEST_VAL_XML_STR, message_type_str)) {*messagetype = BEARER_RESOURCE_MODIFICATION_REQUEST;return res;}
    if (!strcasecmp(BEARER_RESOURCE_MODIFICATION_REJECT_VAL_XML_STR, message_type_str)) {*messagetype = BEARER_RESOURCE_MODIFICATION_REJECT;return res;}
    if (!strcasecmp(ESM_INFORMATION_REQUEST_VAL_XML_STR, message_type_str)) {*messagetype = ESM_INFORMATION_REQUEST;return res;}
    if (!strcasecmp(ESM_INFORMATION_RESPONSE_VAL_XML_STR, message_type_str)) {*messagetype = ESM_INFORMATION_RESPONSE;return res;}
    if (!strcasecmp(ESM_STATUS_VAL_XML_STR, message_type_str)) {*messagetype = ESM_STATUS;return res;}
  }
  return false;
}

//------------------------------------------------------------------------------
void message_type_to_xml(const message_type_t * const messagetype, xmlTextWriterPtr writer)
{
  // better do an array ?
  switch (*messagetype) {
  case ATTACH_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", ATTACH_REQUEST_VAL_XML_STR);
    break;
  case ATTACH_ACCEPT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", ATTACH_ACCEPT_VAL_XML_STR);
    break;
  case ATTACH_COMPLETE:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", ATTACH_COMPLETE_VAL_XML_STR);
    break;
  case ATTACH_REJECT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", ATTACH_REJECT_VAL_XML_STR);
    break;
  case DETACH_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", DETACH_REQUEST_VAL_XML_STR);
    break;
  case DETACH_ACCEPT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", DETACH_ACCEPT_VAL_XML_STR);
    break;
  case TRACKING_AREA_UPDATE_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", TRACKING_AREA_UPDATE_REQUEST_VAL_XML_STR);
    break;
  case TRACKING_AREA_UPDATE_ACCEPT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", TRACKING_AREA_UPDATE_ACCEPT_VAL_XML_STR);
    break;
  case TRACKING_AREA_UPDATE_COMPLETE:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", TRACKING_AREA_UPDATE_COMPLETE_VAL_XML_STR);
    break;
  case TRACKING_AREA_UPDATE_REJECT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", TRACKING_AREA_UPDATE_REJECT_VAL_XML_STR);
    break;
  case EXTENDED_SERVICE_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", EXTENDED_SERVICE_REQUEST_VAL_XML_STR);
    break;
  case SERVICE_REJECT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", SERVICE_REJECT_VAL_XML_STR);
    break;
  case GUTI_REALLOCATION_COMMAND:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", GUTI_REALLOCATION_COMMAND_VAL_XML_STR);
    break;
  case GUTI_REALLOCATION_COMPLETE:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", GUTI_REALLOCATION_COMPLETE_VAL_XML_STR);
    break;
  case AUTHENTICATION_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", AUTHENTICATION_REQUEST_VAL_XML_STR);
    break;
  case AUTHENTICATION_RESPONSE:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", AUTHENTICATION_RESPONSE_VAL_XML_STR);
    break;
  case AUTHENTICATION_REJECT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", AUTHENTICATION_REJECT_VAL_XML_STR);
    break;
  case AUTHENTICATION_FAILURE:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", AUTHENTICATION_FAILURE_VAL_XML_STR);
    break;
  case IDENTITY_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", IDENTITY_REQUEST_VAL_XML_STR);
    break;
  case IDENTITY_RESPONSE:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", IDENTITY_RESPONSE_VAL_XML_STR);
    break;
  case SECURITY_MODE_COMMAND:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", SECURITY_MODE_COMMAND_VAL_XML_STR);
    break;
  case SECURITY_MODE_COMPLETE:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", SECURITY_MODE_COMPLETE_VAL_XML_STR);
    break;
  case SECURITY_MODE_REJECT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", SECURITY_MODE_REJECT_VAL_XML_STR);
    break;
  case EMM_STATUS:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", EMM_STATUS_VAL_XML_STR);
    break;
  case EMM_INFORMATION:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", EMM_INFORMATION_VAL_XML_STR);
    break;
  case DOWNLINK_NAS_TRANSPORT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", DOWNLINK_NAS_TRANSPORT_VAL_XML_STR);
    break;
  case UPLINK_NAS_TRANSPORT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", UPLINK_NAS_TRANSPORT_VAL_XML_STR);
    break;
  case CS_SERVICE_NOTIFICATION:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", CS_SERVICE_NOTIFICATION_VAL_XML_STR);
    break;
  case DOWNLINK_GENERIC_NAS_TRANSPORT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", DOWNLINK_GENERIC_NAS_TRANSPORT_VAL_XML_STR);
    break;
  case UPLINK_GENERIC_NAS_TRANSPORT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", UPLINK_GENERIC_NAS_TRANSPORT_VAL_XML_STR);
    break;
  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_VAL_XML_STR);
    break;
  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT_VAL_XML_STR);
    break;
  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT_VAL_XML_STR);
    break;
  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_VAL_XML_STR);
    break;
  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT_VAL_XML_STR);
    break;
  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT_VAL_XML_STR);
    break;
  case MODIFY_EPS_BEARER_CONTEXT_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", MODIFY_EPS_BEARER_CONTEXT_REQUEST_VAL_XML_STR);
    break;
  case MODIFY_EPS_BEARER_CONTEXT_ACCEPT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", MODIFY_EPS_BEARER_CONTEXT_ACCEPT_VAL_XML_STR);
    break;
  case MODIFY_EPS_BEARER_CONTEXT_REJECT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", MODIFY_EPS_BEARER_CONTEXT_REJECT_VAL_XML_STR);
    break;
  case DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST_VAL_XML_STR);
    break;
  case DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT_VAL_XML_STR);
    break;
  case PDN_CONNECTIVITY_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", PDN_CONNECTIVITY_REQUEST_VAL_XML_STR);
    break;
  case PDN_CONNECTIVITY_REJECT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", PDN_CONNECTIVITY_REJECT_VAL_XML_STR);
    break;
  case PDN_DISCONNECT_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", PDN_DISCONNECT_REQUEST_VAL_XML_STR);
    break;
  case PDN_DISCONNECT_REJECT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", PDN_DISCONNECT_REJECT_VAL_XML_STR);
    break;
  case BEARER_RESOURCE_ALLOCATION_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", BEARER_RESOURCE_ALLOCATION_REQUEST_VAL_XML_STR);
    break;
  case BEARER_RESOURCE_ALLOCATION_REJECT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s",BEARER_RESOURCE_ALLOCATION_REJECT_VAL_XML_STR);
    break;
  case BEARER_RESOURCE_MODIFICATION_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", BEARER_RESOURCE_MODIFICATION_REQUEST_VAL_XML_STR);
    break;
  case BEARER_RESOURCE_MODIFICATION_REJECT:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", BEARER_RESOURCE_MODIFICATION_REJECT_VAL_XML_STR);
    break;
  case ESM_INFORMATION_REQUEST:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", ESM_INFORMATION_REQUEST_VAL_XML_STR);
    break;
  case ESM_INFORMATION_RESPONSE:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", ESM_INFORMATION_RESPONSE_VAL_XML_STR);
    break;
  case ESM_STATUS:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "%s", ESM_STATUS_VAL_XML_STR);
    break;
  default:
    XML_WRITE_FORMAT_ELEMENT(writer, MESSAGE_TYPE_XML_STR, "0x%x", *messagetype);
  }
}

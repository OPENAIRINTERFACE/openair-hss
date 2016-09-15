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

#ifndef FILE_3GPP_24_301_XML_SEEN
#define FILE_3GPP_24_301_XML_SEEN

#include "MessageType.h"
#include "SecurityHeaderType.h"
#include "xml_load.h"


//==============================================================================
// 9 General message format and information elements coding
//==============================================================================
//------------------------------------------------------------------------------
// 9.3 Security header type and EPS bearer identity
//------------------------------------------------------------------------------
#define SECURITY_HEADER_TYPE_NOT_PROTECTED_ATTR_XML_STR                    "security_not_protected"
#define SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_ATTR_XML_STR              "security_integrity_protected"
#define SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_CYPHERED_ATTR_XML_STR     "security_integrity_protected_cyphered"
#define SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_NEW_ATTR_XML_STR          "security_integrity_protected_new_eps_security_context"
#define SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_CYPHERED_NEW_ATTR_XML_STR "security_integrity_protected_cyphered_new_eps_security_context"
#define SECURITY_HEADER_TYPE_SERVICE_REQUEST_ATTR_XML_STR                  "security_service_request"
#define SECURITY_HEADER_TYPE_RESERVED1_ATTR_XML_STR                        "security_reserved1"
#define SECURITY_HEADER_TYPE_RESERVED2_ATTR_XML_STR                        "security_reserved2"
#define SECURITY_HEADER_TYPE_RESERVED3_ATTR_XML_STR                        "security_reserved3"

#define SECURITY_HEADER_TYPE_IE_XML_STR         "security_header_type"
#define SECURITY_HEADER_TYPE_XML_FMT            "%06"PRIX8
#define SECURITY_HEADER_TYPE_XML_SCAN_FMT       "%"SCNx8
NUM_FROM_XML_PROTOTYPE(security_header_type);
void security_header_type_to_xml ( const security_header_type_t * const security_header_type, xmlTextWriterPtr writer);


//------------------------------------------------------------------------------
// 9.5 Message authentication code
//------------------------------------------------------------------------------
#define MAC_IE_XML_STR                                                     "mac"
#define MAC_FMT                                                            "0x%08"PRIX32

bool mac_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, uint32_t * const mac);
void mac_to_xml ( const uint32_t * const mac, xmlTextWriterPtr writer);


//------------------------------------------------------------------------------
// 9.8 Message type
//------------------------------------------------------------------------------
#define MESSAGE_TYPE_IE_XML_STR  "message_type"
bool message_type_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, message_type_t * const messagetype);
void message_type_to_xml(const message_type_t * const messagetype, xmlTextWriterPtr writer);
// Table 9.8.1: Message types for EPS mobility management
/* Message identifiers for EPS Mobility Management     */
# define ATTACH_REQUEST_VAL_XML_STR                                       "ATTACH_REQUEST"
# define ATTACH_ACCEPT_VAL_XML_STR                                        "ATTACH_ACCEPT"
# define ATTACH_COMPLETE_VAL_XML_STR                                      "ATTACH_COMPLETE"
# define ATTACH_REJECT_VAL_XML_STR                                        "ATTACH_REJECT"
# define DETACH_REQUEST_VAL_XML_STR                                       "DETACH_REQUEST"
# define DETACH_ACCEPT_VAL_XML_STR                                        "DETACH_ACCEPT"
# define TRACKING_AREA_UPDATE_REQUEST_VAL_XML_STR                         "TRACKING_AREA_UPDATE_REQUEST"
# define TRACKING_AREA_UPDATE_ACCEPT_VAL_XML_STR                          "TRACKING_AREA_UPDATE_ACCEPT"
# define TRACKING_AREA_UPDATE_COMPLETE_VAL_XML_STR                        "TRACKING_AREA_UPDATE_COMPLETE"
# define TRACKING_AREA_UPDATE_REJECT_VAL_XML_STR                          "TRACKING_AREA_UPDATE_REJECT"
# define EXTENDED_SERVICE_REQUEST_VAL_XML_STR                             "EXTENDED_SERVICE_REQUEST"
# define SERVICE_REJECT_VAL_XML_STR                                       "SERVICE_REJECT"
# define GUTI_REALLOCATION_COMMAND_VAL_XML_STR                            "GUTI_REALLOCATION_COMMAND"
# define GUTI_REALLOCATION_COMPLETE_VAL_XML_STR                           "GUTI_REALLOCATION_COMPLETE"
# define AUTHENTICATION_REQUEST_VAL_XML_STR                               "AUTHENTICATION_REQUEST"
# define AUTHENTICATION_RESPONSE_VAL_XML_STR                              "AUTHENTICATION_RESPONSE"
# define AUTHENTICATION_REJECT_VAL_XML_STR                                "AUTHENTICATION_REJECT"
# define AUTHENTICATION_FAILURE_VAL_XML_STR                               "AUTHENTICATION_FAILURE"
# define IDENTITY_REQUEST_VAL_XML_STR                                     "IDENTITY_REQUEST"
# define IDENTITY_RESPONSE_VAL_XML_STR                                    "IDENTITY_RESPONSE"
# define SECURITY_MODE_COMMAND_VAL_XML_STR                                "SECURITY_MODE_COMMAND"
# define SECURITY_MODE_COMPLETE_VAL_XML_STR                               "SECURITY_MODE_COMPLETE"
# define SECURITY_MODE_REJECT_VAL_XML_STR                                 "SECURITY_MODE_REJECT"
# define EMM_STATUS_VAL_XML_STR                                           "EMM_STATUS"
# define EMM_INFORMATION_VAL_XML_STR                                      "EMM_INFORMATION"
# define DOWNLINK_NAS_TRANSPORT_VAL_XML_STR                               "DOWNLINK_NAS_TRANSPORT"
# define UPLINK_NAS_TRANSPORT_VAL_XML_STR                                 "UPLINK_NAS_TRANSPORT"
# define CS_SERVICE_NOTIFICATION_VAL_XML_STR                              "CS_SERVICE_NOTIFICATION"
# define DOWNLINK_GENERIC_NAS_TRANSPORT_VAL_XML_STR                       "DOWNLINK_GENERIC_NAS_TRANSPORT"
# define UPLINK_GENERIC_NAS_TRANSPORT_VAL_XML_STR                         "UPLINK_GENERIC_NAS_TRANSPORT"


// Table 9.8.2: Message types for EPS session management
# define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_VAL_XML_STR          "ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST"
# define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT_VAL_XML_STR           "ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT"
# define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT_VAL_XML_STR           "ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT"
# define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_VAL_XML_STR        "ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST"
# define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT_VAL_XML_STR         "ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT"
# define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT_VAL_XML_STR         "ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT"
# define MODIFY_EPS_BEARER_CONTEXT_REQUEST_VAL_XML_STR                    "MODIFY_EPS_BEARER_CONTEXT_REQUEST"
# define MODIFY_EPS_BEARER_CONTEXT_ACCEPT_VAL_XML_STR                     "MODIFY_EPS_BEARER_CONTEXT_ACCEPT"
# define MODIFY_EPS_BEARER_CONTEXT_REJECT_VAL_XML_STR                     "MODIFY_EPS_BEARER_CONTEXT_REJECT"
# define DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST_VAL_XML_STR                "DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST"
# define DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT_VAL_XML_STR                 "DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT"
# define PDN_CONNECTIVITY_REQUEST_VAL_XML_STR                             "PDN_CONNECTIVITY_REQUEST"
# define PDN_CONNECTIVITY_REJECT_VAL_XML_STR                              "PDN_CONNECTIVITY_REJECT"
# define PDN_DISCONNECT_REQUEST_VAL_XML_STR                               "PDN_DISCONNECT_REQUEST"
# define PDN_DISCONNECT_REJECT_VAL_XML_STR                                "PDN_DISCONNECT_REJECT"
# define BEARER_RESOURCE_ALLOCATION_REQUEST_VAL_XML_STR                   "BEARER_RESOURCE_ALLOCATION_REQUEST"
# define BEARER_RESOURCE_ALLOCATION_REJECT_VAL_XML_STR                    "BEARER_RESOURCE_ALLOCATION_REJECT"
# define BEARER_RESOURCE_MODIFICATION_REQUEST_VAL_XML_STR                 "BEARER_RESOURCE_MODIFICATION_REQUEST"
# define BEARER_RESOURCE_MODIFICATION_REJECT_VAL_XML_STR                  "BEARER_RESOURCE_MODIFICATION_REJECT"
# define ESM_INFORMATION_REQUEST_VAL_XML_STR                              "ESM_INFORMATION_REQUEST"
# define ESM_INFORMATION_RESPONSE_VAL_XML_STR                             "ESM_INFORMATION_RESPONSE"
# define ESM_STATUS_VAL_XML_STR                                           "ESM_STATUS"
#endif /* FILE_3GPP_24_301_XML_SEEN */

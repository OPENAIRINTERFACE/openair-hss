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

#ifndef FILE_3GPP_24_301_EMM_MSG_XML_SEEN
#define FILE_3GPP_24_301_EMM_MSG_XML_SEEN

#include "AttachReject.h"
#include "AttachRequest.h"
#include "AttachAccept.h"
#include "AttachComplete.h"
#include "AuthenticationFailure.h"
#include "AuthenticationReject.h"
#include "AuthenticationRequest.h"
#include "AuthenticationResponse.h"
#include "DetachAccept.h"
#include "DetachRequest.h"
#include "DownlinkNasTransport.h"
#include "EmmInformation.h"
#include "EmmStatus.h"
#include "IdentityRequest.h"
#include "IdentityResponse.h"
#include "NASSecurityModeCommand.h"
#include "NASSecurityModeComplete.h"
#include "SecurityModeReject.h"
#include "ServiceReject.h"
#include "ServiceRequest.h"
#include "TrackingAreaUpdateAccept.h"
#include "TrackingAreaUpdateComplete.h"
#include "TrackingAreaUpdateReject.h"
#include "TrackingAreaUpdateRequest.h"
#include "UplinkNasTransport.h"
#include "emm_msg.h"
//------------------------------------------------------------------------------
bool attach_accept_from_xml (
    xmlDocPtr           xml_doc,
    xmlXPathContextPtr  xpath_ctx,
    attach_accept_msg  * const attach_accept);
//------------------------------------------------------------------------------
int attach_accept_to_xml (
  attach_accept_msg * attach_accept,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool attach_complete_from_xml (
    xmlDocPtr           xml_doc,
    xmlXPathContextPtr  xpath_ctx,
    attach_complete_msg * const attach_complete);
//------------------------------------------------------------------------------
int attach_complete_to_xml (
  attach_complete_msg * attach_complete,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool attach_reject_from_xml (
    xmlDocPtr           xml_doc,
    xmlXPathContextPtr  xpath_ctx,
    attach_reject_msg  * const attach_reject);
//------------------------------------------------------------------------------
int attach_reject_to_xml (
  attach_reject_msg * attach_reject,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool attach_request_from_xml (
    xmlDocPtr           xml_doc,
     xmlXPathContextPtr  xpath_ctx,
    attach_request_msg * const attach_request);
//------------------------------------------------------------------------------
int attach_request_to_xml (
  attach_request_msg * attach_request,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool authentication_failure_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    authentication_failure_msg  * const authentication_failure);
//------------------------------------------------------------------------------
int authentication_failure_to_xml (
  authentication_failure_msg * authentication_failure,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool authentication_reject_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    authentication_reject_msg  * const authentication_reject);
//------------------------------------------------------------------------------
int authentication_reject_to_xml (
  authentication_reject_msg * authentication_reject,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool authentication_request_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    authentication_request_msg  * const authentication_request);
//------------------------------------------------------------------------------
int authentication_request_to_xml (
  authentication_request_msg * authentication_request,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool authentication_response_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    authentication_response_msg  * const authentication_response);
//------------------------------------------------------------------------------
int authentication_response_to_xml (
  authentication_response_msg * authentication_response,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool detach_accept_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    detach_accept_msg  * const detach_accept);
//------------------------------------------------------------------------------
int detach_accept_to_xml (
  detach_accept_msg * detach_accept,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool detach_request_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    detach_request_msg  * const detach_request);
//------------------------------------------------------------------------------
int detach_request_to_xml (
  detach_request_msg * detach_request,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool downlink_nas_transport_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    downlink_nas_transport_msg   * const downlink_nas_transport);
//------------------------------------------------------------------------------
int downlink_nas_transport_to_xml (
  downlink_nas_transport_msg * downlink_nas_transport,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool emm_information_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    emm_information_msg * const emm_information);
//------------------------------------------------------------------------------
int emm_information_to_xml (
  emm_information_msg * emm_information,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool emm_status_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    emm_status_msg * const emm_status);
//------------------------------------------------------------------------------
int emm_status_to_xml (
  emm_status_msg * emm_status,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool identity_request_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    identity_request_msg * const identity_request);
//------------------------------------------------------------------------------
int identity_request_to_xml (
  identity_request_msg * identity_request,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool identity_response_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    identity_response_msg * const identity_response);
//------------------------------------------------------------------------------
int identity_response_to_xml (
  identity_response_msg * identity_response,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool security_mode_command_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    security_mode_command_msg * const security_mode_command);
//------------------------------------------------------------------------------
int security_mode_command_to_xml (
  security_mode_command_msg * security_mode_command,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool security_mode_complete_from_xml (
    xmlDocPtr                  xml_doc,
    xmlXPathContextPtr         xpath_ctx,
    security_mode_complete_msg * const security_mode_complete);
//------------------------------------------------------------------------------
int security_mode_complete_to_xml (
  security_mode_complete_msg * security_mode_complete,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool security_mode_reject_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    security_mode_reject_msg * const security_mode_reject);
//------------------------------------------------------------------------------
int security_mode_reject_to_xml (
  security_mode_reject_msg * security_mode_reject,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool service_reject_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
  service_reject_msg * service_reject);
//------------------------------------------------------------------------------
int service_reject_to_xml (
  service_reject_msg * service_reject,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool service_request_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
  service_request_msg * service_request);
//------------------------------------------------------------------------------
int service_request_to_xml (
  service_request_msg * service_request,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool tracking_area_update_accept_from_xml (
  xmlDocPtr                     xml_doc,
  xmlXPathContextPtr            xpath_ctx,
  tracking_area_update_accept_msg * tracking_area_update_accept);
//------------------------------------------------------------------------------
int tracking_area_update_accept_to_xml (
  tracking_area_update_accept_msg * tracking_area_update_accept,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool tracking_area_update_complete_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
  tracking_area_update_complete_msg * tracking_area_update_complete);
//------------------------------------------------------------------------------
int tracking_area_update_complete_to_xml (
  tracking_area_update_complete_msg * tracking_area_update_complete,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool tracking_area_update_reject_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    tracking_area_update_reject_msg * tracking_area_update_reject);
//------------------------------------------------------------------------------
int tracking_area_update_reject_to_xml (
  tracking_area_update_reject_msg * tracking_area_update_reject,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool tracking_area_update_request_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    tracking_area_update_request_msg * tracking_area_update_request);
//------------------------------------------------------------------------------
int tracking_area_update_request_to_xml (
  tracking_area_update_request_msg * tracking_area_update_request,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool uplink_nas_transport_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    uplink_nas_transport_msg     * const uplink_nas_transport);
//------------------------------------------------------------------------------
int uplink_nas_transport_to_xml (
  uplink_nas_transport_msg * uplink_nas_transport,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool emm_msg_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    EMM_msg                     * emm_msg);
//------------------------------------------------------------------------------
int emm_msg_to_xml (
  EMM_msg * msg,
  uint8_t * buffer,
  uint32_t len,
  xmlTextWriterPtr writer);
//------------------------------------------------------------------------------
bool emm_msg_header_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    emm_msg_header_t * header);

int emm_msg_header_to_xml (
  emm_msg_header_t * header,
  const uint8_t * buffer,
  uint32_t len,
  xmlTextWriterPtr writer);

#endif /* FILE_3GPP_24_301_EMM_MSG_XML_SEEN */

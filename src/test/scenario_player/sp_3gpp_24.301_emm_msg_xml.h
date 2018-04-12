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

#ifndef FILE_SP_3GPP_24_301_EMM_MSG_XML_SEEN
#define FILE_SP_3GPP_24_301_EMM_MSG_XML_SEEN

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
bool sp_attach_accept_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    attach_accept_msg  * const attach_accept);
//------------------------------------------------------------------------------
bool sp_attach_complete_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    attach_complete_msg * const attach_complete);
//------------------------------------------------------------------------------
bool sp_attach_reject_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    attach_reject_msg  * const attach_reject);
//------------------------------------------------------------------------------
bool sp_attach_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    attach_request_msg * const attach_request);
//------------------------------------------------------------------------------
bool sp_authentication_failure_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    authentication_failure_msg  * const authentication_failure);
//------------------------------------------------------------------------------
bool sp_authentication_reject_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    authentication_reject_msg  * const authentication_reject);
//------------------------------------------------------------------------------
bool sp_authentication_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    authentication_request_msg  * const authentication_request);
//------------------------------------------------------------------------------
bool sp_authentication_response_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    authentication_response_msg  * const authentication_response);
//------------------------------------------------------------------------------
bool sp_detach_accept_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    detach_accept_msg  * const detach_accept);
//------------------------------------------------------------------------------
bool sp_detach_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    detach_request_msg  * const detach_request);
//------------------------------------------------------------------------------
bool sp_downlink_nas_transport_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    downlink_nas_transport_msg   * const downlink_nas_transport);
//------------------------------------------------------------------------------
bool sp_emm_information_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    emm_information_msg * const emm_information);
//------------------------------------------------------------------------------
bool emm_status_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    emm_status_msg * const emm_status);
//------------------------------------------------------------------------------
bool sp_identity_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    identity_request_msg * const identity_request);
//------------------------------------------------------------------------------
bool sp_identity_response_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    identity_response_msg * const identity_response);
//------------------------------------------------------------------------------
bool sp_security_mode_command_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    security_mode_command_msg * const security_mode_command);
//------------------------------------------------------------------------------
bool sp_security_mode_complete_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    security_mode_complete_msg * const security_mode_complete);
//------------------------------------------------------------------------------
bool sp_security_mode_reject_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    security_mode_reject_msg * const security_mode_reject);
//------------------------------------------------------------------------------
int sp_service_reject_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
  service_reject_msg * service_reject);
//------------------------------------------------------------------------------
int sp_service_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
  service_request_msg * service_request);
//------------------------------------------------------------------------------
int sp_tracking_area_update_accept_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
  tracking_area_update_accept_msg * tracking_area_update_accept);
//------------------------------------------------------------------------------
int sp_tracking_area_update_complete_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
  tracking_area_update_complete_msg * tracking_area_update_complete);
//------------------------------------------------------------------------------
int sp_tracking_area_update_reject_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    tracking_area_update_reject_msg * tracking_area_update_reject);
//------------------------------------------------------------------------------
int sp_tracking_area_update_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    tracking_area_update_request_msg * tracking_area_update_request);
//------------------------------------------------------------------------------
bool sp_uplink_nas_transport_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    uplink_nas_transport_msg     * const uplink_nas_transport);
//------------------------------------------------------------------------------
bool sp_emm_msg_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    EMM_msg                     * emm_msg);
//------------------------------------------------------------------------------
bool sp_emm_msg_header_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    emm_msg_header_t * header);

#endif /* FILE_SP_3GPP_24_301_EMM_MSG_XML_SEEN */

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

#ifndef FILE_SP_3GPP_24_301_ESM_MSG_XML_SEEN
#define FILE_SP_3GPP_24_301_ESM_MSG_XML_SEEN

#include "AttachReject.h"

#include "esm_msg.h"

//------------------------------------------------------------------------------
bool sp_activate_dedicated_eps_bearer_context_accept_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    activate_dedicated_eps_bearer_context_accept_msg * activate_dedicated_eps_bearer_context_accept);

bool sp_activate_dedicated_eps_bearer_context_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    activate_dedicated_eps_bearer_context_request_msg * const activate_dedicated_eps_bearer_context_request);

bool sp_activate_default_eps_bearer_context_accept_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    activate_default_eps_bearer_context_accept_msg * activate_default_eps_bearer_context_accept);

bool sp_activate_default_eps_bearer_context_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    activate_default_eps_bearer_context_request_msg * activate_default_eps_bearer_context_request);

bool sp_pdn_connectivity_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    pdn_connectivity_request_msg * pdn_connectivity_request);

bool sp_esm_information_request_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    esm_information_request_msg * esm_information_request);

bool sp_esm_information_response_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    esm_information_response_msg * esm_information_response);

bool sp_esm_msg_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    ESM_msg               * const esm_msg);

bool sp_esm_msg_header_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    esm_msg_header_t          * const header);


#endif /* FILE_SP_3GPP_24_301_ESM_MSG_XML_SEEN */

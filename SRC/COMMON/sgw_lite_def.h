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
MESSAGE_DEF(SGW_CREATE_SESSION_REQUEST,  MESSAGE_PRIORITY_MED, itti_sgw_create_session_request_t,  sgwCreateSessionRequest)
MESSAGE_DEF(SGW_CREATE_SESSION_RESPONSE, MESSAGE_PRIORITY_MED, itti_sgw_create_session_response_t, sgwCreateSessionResponse)
MESSAGE_DEF(SGW_MODIFY_BEARER_REQUEST,   MESSAGE_PRIORITY_MED, itti_sgw_modify_bearer_request_t,   sgwModifyBearerRequest)
MESSAGE_DEF(SGW_MODIFY_BEARER_RESPONSE,  MESSAGE_PRIORITY_MED, itti_sgw_modify_bearer_response_t,  sgwModifyBearerResponse)
MESSAGE_DEF(SGW_DELETE_SESSION_REQUEST,  MESSAGE_PRIORITY_MED, itti_sgw_delete_session_request_t,  sgwDeleteSessionRequest)
MESSAGE_DEF(SGW_DELETE_SESSION_RESPONSE, MESSAGE_PRIORITY_MED, itti_sgw_delete_session_response_t, sgwDeleteSessionResponse)
MESSAGE_DEF(SGW_RELEASE_ACCESS_BEARERS_REQUEST, MESSAGE_PRIORITY_MED, itti_sgw_release_access_bearers_request_t, sgwReleaseAccessBearersRequest)
MESSAGE_DEF(SGW_RELEASE_ACCESS_BEARERS_RESPONSE, MESSAGE_PRIORITY_MED, itti_sgw_release_access_bearers_response_t, sgwReleaseAccessBearersResponse)

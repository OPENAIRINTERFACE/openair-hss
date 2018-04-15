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
//WARNING: Do not include this header directly. Use intertask_interface.h instead.
MESSAGE_DEF(S10_FORWARD_RELOCATION_REQUEST,  MESSAGE_PRIORITY_MED, itti_s10_forward_relocation_request_t,  s10_forward_relocation_request)
MESSAGE_DEF(S10_FORWARD_RELOCATION_RESPONSE,  MESSAGE_PRIORITY_MED, itti_s10_forward_relocation_response_t,  s10_forward_relocation_response)

MESSAGE_DEF(S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION,        MESSAGE_PRIORITY_MED, itti_s10_forward_access_context_notification_t,  s10_forward_access_context_notification)
MESSAGE_DEF(S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE,         MESSAGE_PRIORITY_MED, itti_s10_forward_access_context_acknowledge_t,  s10_forward_access_context_acknowledge)

MESSAGE_DEF(S10_FORWARD_RELOCATION_COMPLETE_NOTIFICATION,  MESSAGE_PRIORITY_MED, itti_s10_forward_relocation_complete_notification_t,  s10_forward_relocation_complete_notification)
MESSAGE_DEF(S10_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGE,   MESSAGE_PRIORITY_MED, itti_s10_forward_relocation_complete_acknowledge_t,  s10_forward_relocation_complete_acknowledge)

MESSAGE_DEF(S10_CONTEXT_REQUEST,       MESSAGE_PRIORITY_MED, itti_s10_context_request_t,  s10_context_request)
MESSAGE_DEF(S10_CONTEXT_RESPONSE,      MESSAGE_PRIORITY_MED, itti_s10_context_response_t,  s10_context_response)
MESSAGE_DEF(S10_CONTEXT_ACKNOWLEDGE,   MESSAGE_PRIORITY_MED, itti_s10_context_acknowledge_t,  s10_context_acknowledge)

MESSAGE_DEF(S10_RELOCATION_CANCEL_REQUEST,      MESSAGE_PRIORITY_MED, itti_s10_relocation_cancel_request_t,  s10_relocation_cancel_request)
MESSAGE_DEF(S10_RELOCATION_CANCEL_RESPONSE,     MESSAGE_PRIORITY_MED, itti_s10_relocation_cancel_response_t, s10_relocation_cancel_response)

/** Internal Messages. */
MESSAGE_DEF(S10_REMOVE_UE_TUNNEL,   MESSAGE_PRIORITY_MED, itti_s10_remove_ue_tunnel_t,  s10_remove_ue_tunnel)


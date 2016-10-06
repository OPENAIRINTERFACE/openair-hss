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

#ifndef FILE_XML_MSG_LOAD_ITTI_SEEN
#define FILE_XML_MSG_LOAD_ITTI_SEEN

int xml_msg_load_itti_sctp_new_association(scenario_t * const scenario, scenario_player_msg_t * const msg);
int xml_msg_load_itti_sctp_close_association(scenario_t * const scenario, scenario_player_msg_t * const msg);
int xml_msg_load_itti_s1ap_ue_context_release_req(scenario_t * const scenario, scenario_player_msg_t * const msg);
int xml_msg_load_itti_s1ap_ue_context_release_command(scenario_t * const scenario, scenario_player_msg_t * const msg);
int xml_msg_load_itti_s1ap_ue_context_release_complete(scenario_t * const scenario, scenario_player_msg_t * const msg);
int xml_msg_load_itti_mme_app_initial_ue_message(scenario_t * const scenario, scenario_player_msg_t * const msg);
int xml_msg_load_itti_mme_app_initial_context_setup_rsp(scenario_t * const scenario, scenario_player_msg_t * const msg);
int xml_msg_load_itti_mme_app_connection_establishment_cnf(scenario_t * const scenario, scenario_player_msg_t * const msg);
int xml_msg_load_itti_nas_uplink_data_ind(scenario_t * const scenario, scenario_player_msg_t * const msg);
int xml_msg_load_itti_nas_downlink_data_req(scenario_t * const scenario, scenario_player_msg_t * const msg);
int xml_msg_load_itti_nas_downlink_data_rej(scenario_t * const scenario, scenario_player_msg_t * const msg);
int xml_msg_load_itti_nas_downlink_data_cnf(scenario_t * const scenario, scenario_player_msg_t * const msg);

#endif


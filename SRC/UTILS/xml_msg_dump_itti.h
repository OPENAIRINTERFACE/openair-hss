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

#ifndef FILE_XML_MSG_DUMP_ITTI_SEEN
#define FILE_XML_MSG_DUMP_ITTI_SEEN

char* itti_task_id2itti_task_str(int task_id);

#  if defined(TRACE_XML)
#    include "xml_msg_tags.h"

void xml_msg_dump_itti_sctp_new_association(const sctp_new_peer_t * const itti_msg, int sender, int receiver, xmlTextWriterPtr xml_text_writer);
void xml_msg_dump_itti_sctp_close_association(const sctp_close_association_t * const itti_msg, int sender, int receiver, xmlTextWriterPtr xml_text_writer);
void xml_msg_dump_itti_s1ap_ue_context_release_req(const itti_s1ap_ue_context_release_req_t * const itti_msg, int sender, int receiver, xmlTextWriterPtr xml_text_writer);
void xml_msg_dump_itti_s1ap_ue_context_release_complete(const itti_s1ap_ue_context_release_complete_t * const itti_msg, int sender, int receiver, xmlTextWriterPtr xml_text_writer);
void xml_msg_dump_itti_mme_app_initial_ue_message(const itti_mme_app_initial_ue_message_t * const itti_msg, int sender, int receiver, xmlTextWriterPtr xml_text_writer);
void xml_msg_dump_itti_mme_app_initial_context_setup_rsp(const itti_mme_app_initial_context_setup_rsp_t * const itti_msg, int sender, int receiver, xmlTextWriterPtr xml_text_writer);
void xml_msg_dump_itti_mme_app_connection_establishment_cnf(const itti_mme_app_connection_establishment_cnf_t * const itti_msg, int sender, int receiver, xmlTextWriterPtr xml_text_writer);
void xml_msg_dump_itti_nas_uplink_data_ind(const itti_nas_ul_data_ind_t * const itti_msg, int sender, int receiver, xmlTextWriterPtr xml_text_writer);
void xml_msg_dump_itti_nas_downlink_data_req(const itti_nas_dl_data_req_t * const itti_msg, int sender, int receiver, xmlTextWriterPtr xml_text_writer);
void xml_msg_dump_itti_nas_downlink_data_rej(const itti_nas_dl_data_rej_t * const itti_msg, int sender, int receiver, xmlTextWriterPtr xml_text_writer);
void xml_msg_dump_itti_nas_downlink_data_cnf(const itti_nas_dl_data_cnf_t * const itti_msg, int sender, int receiver, xmlTextWriterPtr xml_text_writer);

#    define XML_MSG_DUMP_ITTI_SCTP_NEW_ASSOCIATION                            xml_msg_dump_itti_sctp_new_association
#    define XML_MSG_DUMP_ITTI_SCTP_CLOSE_ASSOCIATION                          xml_msg_dump_itti_sctp_close_association
#    define XML_MSG_DUMP_ITTI_S1AP_UE_CONTEXT_RELEASE_REQ                     xml_msg_dump_itti_s1ap_ue_context_release_req
#    define XML_MSG_DUMP_ITTI_S1AP_UE_CONTEXT_RELEASE_COMPLETE                xml_msg_dump_itti_s1ap_ue_context_release_complete
#    define XML_MSG_DUMP_ITTI_MME_APP_INITIAL_UE_MESSAGE                      xml_msg_dump_itti_mme_app_initial_ue_message
#    define XML_MSG_DUMP_ITTI_MME_APP_INITIAL_CONTEXT_SETUP_RSP               xml_msg_dump_itti_mme_app_initial_context_setup_rsp
#    define XML_MSG_DUMP_ITTI_MME_APP_CONNECTION_ESTABLISHMENT_CNF            xml_msg_dump_itti_mme_app_connection_establishment_cnf
#    define XML_MSG_DUMP_ITTI_NAS_UPLINK_DATA_IND                             xml_msg_dump_itti_nas_uplink_data_ind
#    define XML_MSG_DUMP_ITTI_NAS_DOWNLINK_DATA_REQ                           xml_msg_dump_itti_nas_downlink_data_req
#    define XML_MSG_DUMP_ITTI_NAS_DOWNLINK_DATA_REJ                           xml_msg_dump_itti_nas_downlink_data_rej
#    define XML_MSG_DUMP_ITTI_NAS_DOWNLINK_DATA_CNF                           xml_msg_dump_itti_nas_downlink_data_cnf
#  else
#    define XML_MSG_DUMP_ITTI_SCTP_NEW_ASSOCIATION(mEsSaGe, sEnDeR, rEcEiVeR, xMlWrItEr)
#    define XML_MSG_DUMP_ITTI_SCTP_CLOSE_ASSOCIATION(mEsSaGe, sEnDeR, rEcEiVeR, xMlWrItEr)
#    define XML_MSG_DUMP_ITTI_S1AP_UE_CONTEXT_RELEASE_REQ(mEsSaGe, sEnDeR, rEcEiVeR, xMlWrItEr)
#    define XML_MSG_DUMP_ITTI_S1AP_UE_CONTEXT_RELEASE_COMPLETE(mEsSaGe, sEnDeR, rEcEiVeR, xMlWrItEr)
#    define XML_MSG_DUMP_ITTI_MME_APP_INITIAL_UE_MESSAGE(mEsSaGe, sEnDeR, rEcEiVeR, xMlWrItEr)
#    define XML_MSG_DUMP_ITTI_MME_APP_INITIAL_CONTEXT_SETUP_RSP(mEsSaGe, sEnDeR, rEcEiVeR, xMlWrItEr)
#    define XML_MSG_DUMP_ITTI_MME_APP_CONNECTION_ESTABLISHMENT_CNF(mEsSaGe, sEnDeR, rEcEiVeR, xMlWrItEr)
#    define XML_MSG_DUMP_ITTI_NAS_UPLINK_DATA_IND(mEsSaGe, sEnDeR, rEcEiVeR, xMlWrItEr)
#    define XML_MSG_DUMP_ITTI_NAS_DOWNLINK_DATA_REQ(mEsSaGe, sEnDeR, rEcEiVeR, xMlWrItEr)
#    define XML_MSG_DUMP_ITTI_NAS_DOWNLINK_DATA_REJ(mEsSaGe, sEnDeR, rEcEiVeR, xMlWrItEr)
#    define XML_MSG_DUMP_ITTI_NAS_DOWNLINK_DATA_CNF(mEsSaGe, sEnDeR, rEcEiVeR, xMlWrItEr)
#  endif
#endif


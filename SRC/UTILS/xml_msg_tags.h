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

#ifndef FILE_XML_MSG_TAGS_SEEN
#define FILE_XML_MSG_TAGS_SEEN

// Related to scenarios
#  define SCENARIOS_FILE_NODE_XML_STR "scenarios_file"
#  define SCENARIOS_NODE_XML_STR      "scenarios"
#  define SCENARIO_FILE_NODE_XML_STR  "scenario_file"
#  define SCENARIO_NODE_XML_STR       "scenario"
#  define VAR_NODE_XML_STR            "var"
#  define SET_VAR_NODE_XML_STR        "set_var"
#  define INCR_VAR_INC_NODE_XML_STR   "incr_var"
#  define DECR_VAR_INC_NODE_XML_STR   "decr_var"
#  define JUMP_ON_COND_XML_STR        "jcond"
#  define NAME_ATTR_XML_STR           "name"
#  define STATUS_ATTR_XML_STR         "status"
#  define SUCCESS_VAL_XML_STR         "success"
#  define FAILED_VAL_XML_STR          "failed"
#  define VAR_NAME_ATTR_XML_STR       "var_name"
#  define COND_ATTR_XML_STR           "cond"
#  define VALUE_ATTR_XML_STR          "value"
#  define HEX_STREAM_VALUE_ATTR_XML_STR "hex_stream_value"
#  define ASCII_STREAM_VALUE_ATTR_XML_STR "ascii_stream_value"
#  define EXIT_ATTR_XML_STR           "exit"
#  define LABEL_ATTR_XML_STR          "label"
#  define MESSAGE_FILE_NODE_XML_STR   "message_file"
#  define ACTION_ATTR_XML_STR         "action"
#  define SEND_VAL_XML_STR            "send"
#  define RECEIVE_VAL_XML_STR         "receive"
#  define TIME_ATTR_XML_STR           "time"
#  define TIME_REF_ATTR_XML_STR       "time_ref"
#  define NOW_VAL_XML_STR             "now"
#  define THIS_VAL_XML_STR            "this"
#  define FILE_ATTR_XML_STR           "file"
#  define SLEEP_NODE_XML_STR          "sleep"
#  define SECONDS_ATTR_XML_STR        "seconds"
#  define USECONDS_ATTR_XML_STR       "useconds"

#  define COMPUTE_AUTHENTICATION_RESPONSE_PARAMETER_NODE_XML_STR    "compute_authentication_response_parameter"
#  define COMPUTE_AUTHENTICATION_SYNC_FAILURE_PARAMETER_NODE_XML_STR "compute_authentication_sync_failure_parameter"

#  define USIM_NODE_XML_STR           "usim"
#  define LTE_K_ATTR_XML_STR          "lte_k"
#  define SQN_MS_ATTR_XML_STR         "sqn_ms"

#  define UPDATE_EMM_SECURITY_CONTEXT_NODE_XML_STR                  "update_emm_security_context"
#  define SELECTED_EEA_ATTR_XML_STR   "seea"
#  define SELECTED_EIA_ATTR_XML_STR   "seia"
#  define UL_COUNT_ATTR_XML_STR       "ul_count"


// Related to ITTI messages
#  define ITTI_SENDER_TASK_XML_STR                          "ITTI_SENDER_TASK"
#  define ITTI_RECEIVER_TASK_XML_STR                        "ITTI_RECEIVER_TASK"
#  define TIMESTAMP_XML_STR                                 "TIMESTAMP"

#  define ACTION_XML_STR                                    "ACTION"
#  define ACTION_SEND_XML_STR                               "SEND"
#  define ACTION_WAIT_RECEIVE_XML_STR                       "WAIT_RECEIVE"

#  define ITTI_MME_APP_CONNECTION_ESTABLISHMENT_CNF_XML_STR "ITTI_MME_APP_CONNECTION_ESTABLISHMENT_CNF"
#  define ITTI_MME_APP_INITIAL_CONTEXT_SETUP_RSP_XML_STR    "ITTI_MME_APP_INITIAL_CONTEXT_SETUP_RSP"
#  define ITTI_NAS_DOWNLINK_DATA_CNF_XML_STR                "ITTI_NAS_DOWNLINK_DATA_CNF"
#  define ITTI_NAS_DOWNLINK_DATA_REJ_XML_STR                "ITTI_NAS_DOWNLINK_DATA_REJ"
#  define ITTI_NAS_DOWNLINK_DATA_REQ_XML_STR                "ITTI_NAS_DOWNLINK_DATA_REQ"
#  define ITTI_NAS_UPLINK_DATA_IND_XML_STR                  "ITTI_NAS_UPLINK_DATA_IND"
#  define ITTI_S11_CREATE_BEARER_REQ_XML_STR                "ITTI_S11_CREATE_BEARER_REQ"
#  define ITTI_S1AP_E_RAB_SETUP_REQ_XML_STR                 "ITTI_S1AP_E_RAB_SETUP_REQ"
#  define ITTI_S1AP_E_RAB_SETUP_RSP_XML_STR                 "ITTI_S1AP_E_RAB_SETUP_RSP"
#  define ITTI_S1AP_INITIAL_UE_MESSAGE_XML_STR              "ITTI_S1AP_INITIAL_UE_MESSAGE"
#  define ITTI_S1AP_UE_CONTEXT_RELEASE_COMMAND_XML_STR      "ITTI_S1AP_UE_CONTEXT_RELEASE_COMMAND"
#  define ITTI_S1AP_UE_CONTEXT_RELEASE_COMPLETE_XML_STR     "ITTI_S1AP_UE_CONTEXT_RELEASE_COMPLETE"
#  define ITTI_S1AP_UE_CONTEXT_RELEASE_REQ_XML_STR          "ITTI_S1AP_UE_CONTEXT_RELEASE_REQ"
#  define ITTI_SCTP_CLOSE_ASSOCIATION_XML_STR               "ITTI_SCTP_CLOSE_ASSOCIATION"
#  define ITTI_SCTP_NEW_ASSOCIATION_XML_STR                 "ITTI_SCTP_NEW_ASSOCIATION"


#  define ALLOCATION_AND_RETENTION_PRIORITY_XML_STR      "allocation_and_retention_priority"
#  define E_RAB_FAILED_TO_SETUP_LIST_XML_STR             "e_rab_failed_to_setup_list"
#  define E_RAB_GUARANTEED_BIT_RATE_DOWNLINK_XML_STR     "e_rab_guaranteed_bit_rate_downlink"
#  define E_RAB_GUARANTEED_BIT_RATE_DOWNLINK_XML_STR     "e_rab_guaranteed_bit_rate_downlink"
#  define E_RAB_GUARANTEED_BIT_RATE_UPLINK_XML_STR       "e_rab_guaranteed_bit_rate_uplink"
#  define E_RAB_ID_XML_FMT                               "0x%"PRIx8
#  define E_RAB_ID_XML_SCAN_FMT                          "%"SCNx8
#  define E_RAB_ID_XML_STR                               "e_rab_id"
#  define E_RAB_ITEM_XML_STR                             "e_rab_item"
#  define E_RAB_LEVEL_QOS_PARAMETERS_XML_STR             "e_rab_level_qos_parameters"
#  define E_RAB_LIST_XML_STR                             "e_rab_list"
#  define E_RAB_MAXIMUM_BIT_RATE_DOWNLINK_XML_STR        "e_rab_maximum_bit_rate_downlink"
#  define E_RAB_MAXIMUM_BIT_RATE_UPLINK_XML_STR          "e_rab_maximum_bit_rate_uplink"
#  define E_RAB_SETUP_ITEM_XML_STR                       "e_rab_setup_item"
#  define E_RAB_SETUP_LIST_XML_STR                       "e_rab_setup_list"
#  define E_RAB_TO_BE_SETUP_ITEM_XML_STR                 "e_rab_to_be_setup_item"
#  define E_RAB_TO_BE_SETUP_LIST_XML_STR                 "e_rab_to_be_setup_list"
#  define GBR_QOS_INFORMATION_XML_STR                    "gbr_qos_information"
#  define KENB_XML_STR                                   "kenb"
#  define NAS_ERROR_CODE_XML_STR                         "nas_error_code"
#  define NUMBER_INBOUND_STREAMS_XML_STR                 "number_inbound_streams"
#  define NUMBER_OUTBOUND_STREAMS_XML_STR                "number_outbound_streams"
#  define NUMBER_STREAMS_FMT                             "%u"
#  define PRE_EMPTION_CAPABILITY_XML_STR                 "pre_emption_capability"
#  define PRE_EMPTION_CAPABILITY_XML_FMT                 "%"PRIu8
#  define PRE_EMPTION_CAPABILITY_XML_SCAN_FMT            "%"SCNu8
#  define PRE_EMPTION_VULNERABILITY_XML_STR              "pre_emption_vulnerability"
#  define PRE_EMPTION_VULNERABILITY_XML_FMT              "%"PRIu8
#  define PRE_EMPTION_VULNERABILITY_XML_SCAN_FMT         "%"SCNu8
#  define PRIORITY_LEVEL_XML_STR                         "priority_level"
#  define PRIORITY_LEVEL_XML_FMT                         "%"PRIu8
#  define PRIORITY_LEVEL_XML_SCAN_FMT                    "%"SCNu8
#  define QCI_XML_STR                                    "qci"
#  define QCI_XML_FMT                                    "%"PRIu8
#  define QCI_XML_SCAN_FMT                               "%"SCNu8
#  define S1AP_CAUSE_XML_STR                             "s1ap_cause"
#  define S1AP_CAUSE_GROUP_XML_STR                       "group"
#  define S1AP_CAUSE_GROUP_CAUSE_FMT                     "%ld"
#  define S1AP_CAUSE_GROUP_CAUSE_SCAN_FMT                "%ld"
#  define S1AP_CAUSE_GROUP_CAUSE_XML_STR                 "group_cause"
#  define S1AP_CAUSE_GROUP_MISC_XML_STR                  "misc"
#  define S1AP_CAUSE_GROUP_NAS_XML_STR                   "nas"
#  define S1AP_CAUSE_GROUP_NOTHING_XML_STR               "nothing"
#  define S1AP_CAUSE_GROUP_PROTOCOL_XML_STR              "protocol"
#  define S1AP_CAUSE_GROUP_RADIO_NETWORK_LAYER_XML_STR   "radio_network_layer"
#  define S1AP_CAUSE_GROUP_TRANSPORT_LAYER_XML_STR       "transport_layer"

#  define SCTP_ASSOC_ID_FMT                              "%x"
#  define SCTP_ASSOC_ID_XML_STR                          "sctp_assoc_id"
#  define TEID_XML_STR                                   "teid"
#  define TEID_XML_FMT                                   "%"PRIx32
#  define TEID_XML_SCAN_FMT                              "%"SCNx32
#  define TRANSPORT_LAYER_ADDRESS_XML_STR                "transport_layer_address"
#  define UE_AGGREGATE_MAXIMUM_BIT_RATE_XML_STR          "ue_aggregate_maximum_bit_rate"
#  define UE_SECURITY_CAPABILITIES_ENCRYPTION_ALGORITHMS_XML_STR  "ue_security_capabilities_encryption_algorithms"
#  define UE_SECURITY_CAPABILITIES_INTEGRITY_PROTECTION_ALGORITHMS_XML_STR  "ue_security_capabilities_integrity_protection_algorithms"
#  define DOWNLINK_XML_STR                               "downlink"
#  define UPLINK_XML_STR                                 "uplink"



#endif


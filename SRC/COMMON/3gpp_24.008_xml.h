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

#ifndef FILE_3GPP_24_008_XML_SEEN
#define FILE_3GPP_24_008_XML_SEEN


#define HEX_DATA_ATTR_XML_STR                          "hex_data"
#define ASCII_DATA_ATTR_XML_STR                        "ascii_data"

//******************************************************************************
// 10.5.1 Common information elements
//******************************************************************************
//------------------------------------------------------------------------------
// 10.5.1.2 Ciphering Key Sequence Number
//------------------------------------------------------------------------------
#define CIPHERING_KEY_SEQUENCE_NUMBER_IE_XML_STR      "ciphering_key_sequence_number"
#define CIPHERING_KEY_SEQUENCE_NUMBER_XML_SCAN_FMT    "%"SCNx8
#define CIPHERING_KEY_SEQUENCE_NUMBER_XML_FMT         "0x%"PRIx8

#define KEY_SEQUENCE_ATTR_XML_STR                     "key_sequence"
//------------------------------------------------------------------------------
// 10.5.1.3 Location Area Identification
//------------------------------------------------------------------------------
#define LOCATION_AREA_IDENTIFICATION_IE_XML_STR       "lai"
#define LOCATION_AREA_CODE_ATTR_XML_STR               "lac"
#define MOBILE_COUNTRY_CODE_ATTR_XML_STR              "mcc"
#define MOBILE_NETWORK_CODE_ATTR_XML_STR              "mnc"
//------------------------------------------------------------------------------
// 10.5.1.4 Mobile Identity
//------------------------------------------------------------------------------
#define MOBILE_IDENTITY_IE_XML_STR                    "mobile_identity"
#define ODDEVEN_ATTR_XML_STR                          "oddeven"
#define TYPE_OF_IDENTITY_ATTR_XML_STR                 "type_of_identity"
#define IDENTITY_DIGIT1_ATTR_XML_STR                  "identity_digit_1"
#define IDENTITY_DIGIT2_ATTR_XML_STR                  "identity_digit_2"
#define IDENTITY_DIGIT3_ATTR_XML_STR                  "identity_digit_3"
#define IDENTITY_DIGIT4_ATTR_XML_STR                  "identity_digit_4"
#define IDENTITY_DIGIT5_ATTR_XML_STR                  "identity_digit_5"
#define IDENTITY_DIGIT6_ATTR_XML_STR                  "identity_digit_6"
#define IDENTITY_DIGIT7_ATTR_XML_STR                  "identity_digit_7"
#define IDENTITY_DIGIT8_ATTR_XML_STR                  "identity_digit_8"
#define IDENTITY_DIGIT9_ATTR_XML_STR                  "identity_digit_9"
#define IDENTITY_DIGIT10_ATTR_XML_STR                 "identity_digit_10"
#define IDENTITY_DIGIT11_ATTR_XML_STR                 "identity_digit_11"
#define IDENTITY_DIGIT12_ATTR_XML_STR                 "identity_digit_12"
#define IDENTITY_DIGIT13_ATTR_XML_STR                 "identity_digit_13"
#define IDENTITY_DIGIT14_ATTR_XML_STR                 "identity_digit_14"
#define IDENTITY_DIGIT15_ATTR_XML_STR                 "identity_digit_15"

#define IMSI_ATTR_XML_STR                             "imsi"
#define TMSI_ATTR_XML_STR                             "tmsi"
#define TAC_ATTR_XML_STR                              "tac"
#define SNR_ATTR_XML_STR                              "snr"
#define CDSD_ATTR_XML_STR                             "cdsd"
#define SVN_ATTR_XML_STR                              "svn"

#define MBMS_SESSION_ID_INDIC_ATTR_XML_STR            "mbms_session_id_indic"
#define MCC_MNC_INDIC_ATTR_XML_STR                    "mcc_mnc_id_indic"
#define MBMS_SERVICE_ID_ATTR_XML_STR                  "mbms_service_id"
#define MBMS_SESSION_ID_ATTR_XML_STR                  "mbms_session_id"

//------------------------------------------------------------------------------
// 10.5.1.6 Mobile Station Classmark 2
//------------------------------------------------------------------------------
#define MOBILE_STATION_CLASSMARK_2_IE_XML_STR         "mobile_station_classmark_2"

#define REVISION_LEVEL_ATTR_XML_STR                   "revision_level"
#define ES_IND_LEVEL_ATTR_XML_STR                     "es_ind"
#define A51_LEVEL_ATTR_XML_STR                        "a51"
#define RF_POWER_CAPABILITY_ATTR_XML_STR              "rf_power_capability"
#define PS_CAPABILITY_ATTR_XML_STR                    "ps_capability"
#define SS_SCREEN_INDICATOR_ATTR_XML_STR              "ss_screen_indicator"
#define SM_CAPABILITY_ATTR_XML_STR                    "sm_capability"
#define VBS_ATTR_XML_STR                              "vbs"
#define VGCS_ATTR_XML_STR                             "vgcs"
#define FC_ATTR_XML_STR                               "fc"
#define CM3_ATTR_XML_STR                              "cm3"
#define LCS_VA_CAP_ATTR_XML_STR                       "lcs_va_cap"
#define UCS2_ATTR_XML_STR                             "ucs2"
#define SOLSA_ATTR_XML_STR                            "solsa"
#define CMSP_ATTR_XML_STR                             "cmsp"
#define A53_ATTR_XML_STR                              "a53"
#define A52_ATTR_XML_STR                              "a52"

//------------------------------------------------------------------------------
// 10.5.1.7 Mobile Station Classmark 3
//------------------------------------------------------------------------------
#define MOBILE_STATION_CLASSMARK_3_IE_XML_STR         "mobile_station_classmark_3"

//------------------------------------------------------------------------------
// 10.5.1.13 PLMN list
//------------------------------------------------------------------------------
#define PLMN_LIST_IE_XML_STR         "plmn_list"
#define PLMN_IE_XML_STR              "plmn"

//------------------------------------------------------------------------------
// 10.5.1.15 MS network feature support
//------------------------------------------------------------------------------
#define MS_NEWORK_FEATURE_SUPPORT_IE_XML_STR         "ms_network_feature_support"
#define EXTENDED_PERIODIC_TIMERS_IE_XML_STR          "extended_periodic_timers"

//------------------------------------------------------------------------------
// 10.5.3.1 Authentication parameter RAND
//------------------------------------------------------------------------------
#define AUTHENTICATION_PARAMETER_RAND_IE_XML_STR     "authentication_parameter_rand"

//------------------------------------------------------------------------------
// 10.5.3.1.1 Authentication Parameter AUTN (UMTS and EPS authentication challenge)
//------------------------------------------------------------------------------
#define AUTHENTICATION_PARAMETER_AUTN_IE_XML_STR     "authentication_parameter_autn"

//------------------------------------------------------------------------------
// 10.5.3.2 Authentication Response parameter
//------------------------------------------------------------------------------
#define AUTHENTICATION_RESPONSE_PARAMETER_IE_XML_STR     "authentication_response_parameter"

//------------------------------------------------------------------------------
// 10.5.3.2.2 Authentication Failure parameter (UMTS and EPS authentication challenge)
//------------------------------------------------------------------------------
#define AUTHENTICATION_FAILURE_PARAMETER_IE_XML_STR     "authentication_failure_parameter"

//------------------------------------------------------------------------------
// 10.5.3.5a Network Name
//------------------------------------------------------------------------------
#define NETWORK_NAME_IE_XML_STR                         "network_name"
#define FULL_NETWORK_NAME_IE_XML_STR                    "full_network_name"
#define SHORT_NETWORK_NAME_IE_XML_STR                   "short_network_name"
#define CODING_SCHEME_ATTR_XML_STR                      "coding_scheme"
#define ADD_CI_ATTR_XML_STR                             "add_ci"
#define NUMBER_OF_SPARE_BITS_IN_LAST_OCTET_ATTR_XML_STR "number_of_spare_bits_in_last_octet"
#define TEXT_STRING_ATTR_XML_STR                        "text_string"

//------------------------------------------------------------------------------
// 10.5.3.8 Time Zone
//------------------------------------------------------------------------------
#define TIME_ZONE_IE_XML_STR "time_zone"

//------------------------------------------------------------------------------
// 10.5.3.9 Time Zone and Time
//------------------------------------------------------------------------------
#define TIME_ZONE_AND_TIME_IE_XML_STR "time_zone_and_time"
#define YEAR_ATTR_XML_STR             "year"
#define MONTH_ATTR_XML_STR            "month"
#define DAY_ATTR_XML_STR              "day"
#define HOUR_ATTR_XML_STR             "hour"
#define MINUTE_ATTR_XML_STR           "minute"
#define SECOND_ATTR_XML_STR           "second"
#define TIME_ZONE_ATTR_XML_STR        TIME_ZONE_IE_XML_STR


//------------------------------------------------------------------------------
// 10.5.3.12 Daylight Saving Time
//------------------------------------------------------------------------------
#define DAYLIGHT_SAVING_TIME_IE_XML_STR                 "daylight_saving_time"

//------------------------------------------------------------------------------
// 10.5.3.13 Emergency Number List
//------------------------------------------------------------------------------
#define EMERGENCY_NUMBER_LIST_IE_XML_STR                     "emergency_number_list"
#define EMERGENCY_NUMBER_LIST_ITEM_XML_STR                   "emergency_number"
#define LENGTH_OF_EMERGENCY_NUMBER_INFORMATION_ATTR_XML_STR  "length_of_emergency_number_information"
#define EMERGENCY_SERVICE_CATEGORY_VALUE_ATTR_XML_STR        "emergency_service_category_value"
#define NUMBER_DIGITS_ATTR_XML_STR                           "number_digits"

//------------------------------------------------------------------------------
// 10.5.4.32 Supported codec list
//------------------------------------------------------------------------------
#define SUPPORTED_CODEC_LIST_IE_XML_STR              "supported_codec_list"

//------------------------------------------------------------------------------
// 10.5.5.4 TMSI status
//------------------------------------------------------------------------------
#define TMSI_STATUS_IE_XML_STR "tmsi_status"

//------------------------------------------------------------------------------
// 10.5.5.6 DRX parameter
//------------------------------------------------------------------------------
#define DRX_PARAMETER_IE_XML_STR                     "drx_parameter"
#define SPLIT_PG_CYCLE_CODE_ATTR_XML_STR             "split_pg_cycle_code"
#define CN_SPECIFIC_DRX_CYCLE_LENGTH_COEFFICIENT_AND_DRX_VALUE_FOR_S1_MODE_ATTR_XML_STR   "cn_specific_drx_cycle_length_coefficient_and_drx_value_for_s1_mode"
#define SPLIT_ON_CCCH_ATTR_XML_STR                   "split_on_ccch"
#define NON_DRX_TIMER_ATTR_XML_STR                   "non_drx_timer"

//------------------------------------------------------------------------------
// 10.5.5.8 P-TMSI signature
//------------------------------------------------------------------------------
#define P_TMSI_SIGNATURE_IE_XML_STR                  "p_tmsi_signature"

//------------------------------------------------------------------------------
// 10.5.5.9 Identity type 2
//------------------------------------------------------------------------------
#define IDENTITY_TYPE_2_IE_XML_STR                   "identity_type_2"

//------------------------------------------------------------------------------
// 10.5.5.10 IMEISV request
//------------------------------------------------------------------------------
#define IMEISV_REQUEST_IE_XML_STR                    "imeisv_request"

//------------------------------------------------------------------------------
// 10.5.5.12 MS network capability
//------------------------------------------------------------------------------
#define MS_NETWORK_CAPABILITY_IE_XML_STR                          "ms_network_capability"
#define GEA1_BITS_ATTR_XML_STR                                    "gea1_bits"
#define SM_CAPABILITIES_VIA_DEDICATED_CHANNELS_ATTR_XML_STR       "sm_capabilities_via_dedicated_channels"
#define SM_CAPABILITIES_VIA_GPRS_CHANNELS_ATTR_XML_STR            "sm_capabilities_via_gprs_channels"
#define UCS2_SUPPORT_ATTR_XML_STR                                 "ucs2_support"
#define SS_SCREENING_INDICATOR_ATTR_XML_STR                       "ss_screening_indicator"
#define SOLSA_CAPABILITY_ATTR_XML_STR                             "solsa_capability"
#define REVISION_LEVEL_INDICATOR_ATTR_XML_STR                     "revision_level_indicator"
#define PFC_FEATURE_MODE_ATTR_XML_STR                             "pfc_feature_mode"
#define EXTENDED_GEA_BITS_ATTR_XML_STR                            "extended_gea_bits"
#define LCS_VA_CAPABILITY_ATTR_XML_STR                            "lcs_va_capability"
#define PS_INTER_RAT_HO_FROM_GERAN_TO_UTRAN_IU_MODE_CAPABILITY_ATTR_XML_STR    "ps_inter_rat_ho_from_geran_to_utran_iu_mode_capability"
#define PS_INTER_RAT_HO_FROM_GERAN_TO_E_UTRAN_S1_MODE_CAPABILITY_ATTR_XML_STR  "ps_inter_rat_ho_from_geran_to_e_utran_s1_mode_capability"
#define EMM_COMBINED_PROCEDURES_CAPABILITY_ATTR_XML_STR           "emm_combined_procedures_capability"
#define ISR_SUPPORT_ATTR_XML_STR                                  "isr_support"
#define SRVCC_TO_GERAN_UTRAN_CAPABILITY_ATTR_XML_STR              "srvcc_to_geran_utran_capability"
#define EPC_CAPABILITY_ATTR_XML_STR                               "epc_capability"
#define NF_CAPABILITY_ATTR_XML_STR                                "nf_capability"
#define SPARE_BITS_ATTR_XML_STR                                   "spare_bits"

//------------------------------------------------------------------------------
// 10.5.5.15 Routing area identification
//------------------------------------------------------------------------------
#define ROUTING_AREA_CODE_IE_XML_STR                  "rac"
#define ROUTING_AREA_IDENTIFICATION_IE_XML_STR        "rai"

//------------------------------------------------------------------------------
// 10.5.5.28 Voice domain preference and UE's usage setting
//------------------------------------------------------------------------------
#define VOICE_DOMAIN_PREFERENCE_AND_UE_USAGE_SETTING_IE_XML_STR "voice_domain_preference_and_ue_usage_setting"
#define UE_USAGE_SETTING_ATTR_XML_STR                           "ue_usage_setting"
#define VOICE_DOMAIN_PREFERENCE_ATTR_XML_STR                    "voice_domain_preference"

//------------------------------------------------------------------------------
// 10.5.6.1 Access Point Name
//------------------------------------------------------------------------------
#define ACCESS_POINT_NAME_IE_XML_STR                         "apn"

//------------------------------------------------------------------------------
// 10.5.6.3 Protocol configuration options
//------------------------------------------------------------------------------
#define PROTOCOL_CONFIGURATION_OPTIONS_IE_XML_STR            "pco"
#define EXTENSION_ATTR_XML_STR                               "extension"
#define CONFIGURATION_PROTOCOL_ATTR_XML_STR                  "configuration_protocol"

#define LENGTH_ATTR_XML_STR                                  "length"

#define PROTOCOL_ATTR_XML_STR                                "protocol"
#define PROTOCOL_ID_ATTR_XML_STR                             "id"
#define PROTOCOL_ID_LCP_VAL_XML_STR                          "LCP"
#define PROTOCOL_ID_PAP_VAL_XML_STR                          "PAP"
#define PROTOCOL_ID_CHAP_VAL_XML_STR                         "CHAP"
#define PROTOCOL_ID_IPCP_VAL_XML_STR                         "IPCP"
#define RAW_CONTENT_ATTR_XML_STR                             "raw"

#define CONTAINER_ATTR_XML_STR                                               "container"
#define CONTAINER_ID_ATTR_XML_STR                                            "id"
/* CONTAINER IDENTIFIER MS to network direction:*/
#define P_CSCF_IPV6_ADDRESS_REQUEST_VAL_XML_STR                              "P_CSCF_IPV6_ADDRESS_REQUEST"
#define DNS_SERVER_IPV6_ADDRESS_REQUEST_VAL_XML_STR                          "DNS_SERVER_IPV6_ADDRESS_REQUEST"
#define MS_SUPPORT_OF_NETWORK_REQUESTED_BEARER_CONTROL_INDICATOR_VAL_XML_STR "MS_SUPPORT_OF_NETWORK_REQUESTED_BEARER_CONTROL_INDICATOR"
#define DSMIPV6_HOME_AGENT_ADDRESS_REQUEST_VAL_XML_STR                       "DSMIPV6_HOME_AGENT_ADDRESS_REQUEST"
#define DSMIPV6_HOME_NETWORK_PREFIX_REQUEST_VAL_XML_STR                      "DSMIPV6_HOME_NETWORK_PREFIX_REQUEST"
#define DSMIPV6_IPV4_HOME_AGENT_ADDRESS_REQUEST_VAL_XML_STR                  "DSMIPV6_IPV4_HOME_AGENT_ADDRESS_REQUEST"
#define IP_ADDRESS_ALLOCATION_VIA_NAS_SIGNALLING_VAL_XML_STR                 "IP_ADDRESS_ALLOCATION_VIA_NAS_SIGNALLING"
#define IPV4_ADDRESS_ALLOCATION_VIA_DHCPV4_VAL_XML_STR                       "IPV4_ADDRESS_ALLOCATION_VIA_DHCPV4"
#define P_CSCF_IPV4_ADDRESS_REQUEST_VAL_XML_STR                              "P_CSCF_IPV4_ADDRESS_REQUEST"
#define DNS_SERVER_IPV4_ADDRESS_REQUEST_VAL_XML_STR                          "DNS_SERVER_IPV4_ADDRESS_REQUEST"
#define MSISDN_REQUEST_VAL_XML_STR                                           "MSISDN_REQUEST"
#define IFOM_SUPPORT_REQUEST_VAL_XML_STR                                     "IFOM_SUPPORT_REQUEST"
#define IPV4_LINK_MTU_REQUEST_VAL_XML_STR                                    "IPV4_LINK_MTU_REQUEST"
/* CONTAINER IDENTIFIER Network to MS direction:*/
#define P_CSCF_IPV6_ADDRESS_VAL_XML_STR                                      "P_CSCF_IPV6_ADDRESS"
#define DNS_SERVER_IPV6_ADDRESS_VAL_XML_STR                                  "DNS_SERVER_IPV6_ADDRESS"
#define POLICY_CONTROL_REJECTION_CODE_VAL_XML_STR                            "POLICY_CONTROL_REJECTION_CODE"
#define SELECTED_BEARER_CONTROL_MODE_VAL_XML_STR                             "SELECTED_BEARER_CONTROL_MODE"
#define DSMIPV6_HOME_AGENT_ADDRESS_VAL_XML_STR                               "DSMIPV6_HOME_AGENT_ADDRESS"
#define DSMIPV6_HOME_NETWORK_PREFIX_VAL_XML_STR                              "DSMIPV6_HOME_NETWORK_PREFIX"
#define DSMIPV6_IPV4_HOME_AGENT_ADDRESS_VAL_XML_STR                          "DSMIPV6_IPV4_HOME_AGENT_ADDRESS"
#define P_CSCF_IPV4_ADDRESS_VAL_XML_STR                                      "P_CSCF_IPV4_ADDRESS"
#define DNS_SERVER_IPV4_ADDRESS_VAL_XML_STR                                  "DNS_SERVER_IPV4_ADDRESS"
#define MSISDN_VAL_XML_STR                                                   "MSISDN"
#define IFOM_SUPPORT_VAL_XML_STR                                             "IFOM_SUPPORT"
#define IPV4_LINK_MTU_VAL_XML_STR                                            "IPV4_LINK_MTU"
/* Both directions:*/
#define IM_CN_SUBSYSTEM_SIGNALING_FLAG_VAL_XML_STR                           "IM_CN_SUBSYSTEM_SIGNALING_FLAG"

//------------------------------------------------------------------------------
// 10.5.6.5 Quality of service
//------------------------------------------------------------------------------
#define QUALITY_OF_SERVICE_IE_XML_STR                    "qos"

#define DELAY_CLASS_VAL_XML_STR                          "delay_class"
#define RELIABILITY_CLASS_VAL_XML_STR                    "reliability_class"
#define PEAK_THROUGHPUT_VAL_XML_STR                      "peak_throughput"
#define PRECEDENCE_CLASS_VAL_XML_STR                     "precedence_class"
#define MEAN_THROUGHPUT_VAL_XML_STR                      "mean_throughput"
#define TRAFIC_CLASS_VAL_XML_STR                         "traffic_class"
#define DELIVERY_ORDER_VAL_XML_STR                       "delivery_order"
#define DELIVERY_OF_ERRONEOUS_SDU_VAL_XML_STR            "delivery_of_erroneous_sdu"
#define MAXIMUM_SDU_SIZE_VAL_XML_STR                     "maximum_sdu_size"
#define MAXIMUM_BIT_RATE_UPLINK_VAL_XML_STR              "maximum_bit_rate_uplink"
#define MAXIMUM_BIT_RATE_DOWNLINK_VAL_XML_STR            "maximum_bit_rate_downlink"
#define RESIDUAL_BER_VAL_XML_STR                         "residual_ber"
#define SDU_RATIO_ERROR_VAL_XML_STR                      "sdu_ratio_error"
#define TRANSFER_DELAY_VAL_XML_STR                       "transfer_delay"
#define TRAFFIC_HANDLING_PRIORITY_VAL_XML_STR            "traffic_handling_priority"
#define GUARANTEED_BITRATE_UPLINK_VAL_XML_STR            "guaranteed_bit_rate_uplink"
#define GUARANTEED_BITRATE_DOWNLINK_VAL_XML_STR          "guaranteed_bit_rate_downlink"
#define SIGNALING_INDICATION_VAL_XML_STR                 "signaling_indication"
#define SOURCE_STATISTICS_DESCRIPTOR_VAL_XML_STR         "source_statistics_descriptor"

//------------------------------------------------------------------------------
// 10.5.6.7 Linked TI
//------------------------------------------------------------------------------
#define LINKED_TI_IE_XML_STR                              "linked_ti"
#define TI_FLAG_IE_XML_STR                                "ti_flag"
#define TI_EXT_IE_XML_STR                                 "ext"
#define TIO_IE_XML_STR                                    "tio"
#define TIE_IE_XML_STR                                    "tie"

//------------------------------------------------------------------------------
// 10.5.6.9 LLC service access point identifier
//------------------------------------------------------------------------------
#define LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IE_XML_STR   "llc_service_access_point_identifier"

//------------------------------------------------------------------------------
// 10.5.6.11 Packet Flow Identifier
//------------------------------------------------------------------------------
#define PACKET_FLOW_IDENTIFIER_IE_XML_STR                "packet_flow_identifier"

//------------------------------------------------------------------------------
// 10.5.6.12 Traffic Flow Template
//------------------------------------------------------------------------------
#define TRAFFIC_FLOW_TEMPLATE_IE_XML_STR                    "traffic_flow_template"
#define TFT_OPERATION_CODE_ATTR_XML_STR                     "tft_operation_code"
#define E_ATTR_XML_STR                                      "e"
#define NUMBER_OF_PACKET_FILTERS_ATTR_XML_STR               "number_of_packet_filters"
#define PACKET_FILTER_IE_XML_STR                            "packet_filter"
#define IDENTIFIER_IE_XML_STR                               "identifier"
#define IDENTIFIER_ATTR_XML_STR                             "identifier"
#define PACKET_FILTER_EVALUATION_PRECEDENCE_ATTR_XML_STR    "evaluation_precedence"
#define PACKET_FILTER_DIRECTION_ATTR_XML_STR                "direction"
#define PACKET_FILTER_DIRECTION_PRE_REL_7_VAL_XML_STR       "pre_rel-7"
#define PACKET_FILTER_DIRECTION_DOWNLINK_ONLY_VAL_XML_STR   "downlink_only"
#define PACKET_FILTER_DIRECTION_UPLINK_ONLY_VAL_XML_STR     "uplink_only"
#define PACKET_FILTER_DIRECTION_BIDIRECTIONAL_VAL_XML_STR   "bidirectional"
#define PARAMETER_ATTR_XML_STR                              "parameter"

#define PACKET_FILTER_CONTENTS_IE_XML_STR                               "packet_filter_contents"

#define PACKET_FILTER_COMPONENT_TYPE_IDENTIFIER_IPV4_REMOTE_ADDRESS_TYPE_IE_XML_STR  "component_type_identifier_ipv4_remote_address_type"
#define PACKET_FILTER_CONTENTS_IPV4_ADDRESS_IE_XML_STR                  "ipv4_address"
#define PACKET_FILTER_CONTENTS_IPV4_ADDRESS_MASK_IE_XML_STR             "ipv4_address_mask"

#define PACKET_FILTER_COMPONENT_TYPE_IDENTIFIER_IPV6_REMOTE_ADDRESS_TYPE_IE_XML_STR  "component_type_identifier_ipv6_remote_address_type"
#define PACKET_FILTER_CONTENTS_IPV6_ADDRESS_IE_XML_STR                  "ipv6_address"
#define PACKET_FILTER_CONTENTS_IPV6_ADDRESS_MASK_IE_XML_STR             "ipv6_address_mask"

#define PACKET_FILTER_COMPONENT_TYPE_IDENTIFIER_PROTOCOL_IDENTIFIER_NEXT_HEADER_IE_XML_STR  "component_type_identifier_protocol_identifier_next_header_type"
#define PACKET_FILTER_CONTENTS_PROTOCOL_IDENTIFIER_NEXT_HEADER_TYPE_IE_XML_STR  "protocol_header"

#define PACKET_FILTER_COMPONENT_TYPE_IDENTIFIER_LOCAL_PORT_RANGE_IE_XML_STR  "component_type_identifier_local_port_range"
#define PACKET_FILTER_CONTENTS_PORT_RANGE_LOW_LIMIT_IE_XML_STR           "low_limit"
#define PACKET_FILTER_CONTENTS_PORT_RANGE_HIGH_LIMIT_IE_XML_STR          "high_limit"

#define PACKET_FILTER_COMPONENT_TYPE_IDENTIFIER_REMOTE_PORT_RANGE_IE_XML_STR  "component_type_identifier_remote_port_range"

#define PACKET_FILTER_COMPONENT_TYPE_IDENTIFIER_SINGLE_LOCAL_PORT_IE_XML_STR  "component_type_identifier_single_local_port"
#define PACKET_FILTER_CONTENTS_SINGLE_PORT_IE_XML_STR                   "port"

#define PACKET_FILTER_COMPONENT_TYPE_IDENTIFIER_SINGLE_REMOTE_PORT_IE_XML_STR  "component_type_identifier_single_remote_port"

#define PACKET_FILTER_COMPONENT_TYPE_IDENTIFIER_SECURITY_PARAMETER_INDEX_IE_XML_STR  "component_type_identifier_security_parameter_index"
#define PACKET_FILTER_CONTENTS_INDEX_IE_XML_STR                          "index"

#define PACKET_FILTER_COMPONENT_TYPE_IDENTIFIER_TYPE_OF_SERVICE_CLASS_IE_XML_STR  "component_type_identifier_type_of_service_traffic_class"
#define PACKET_FILTER_CONTENTS_TYPE_OF_SERVICE_TRAFFIC_CLASS_IE_XML_STR  "value"
#define PACKET_FILTER_CONTENTS_TYPE_OF_SERVICE_TRAFFIC_CLASS_MASK_IE_XML_STR "mask"

#define PACKET_FILTER_COMPONENT_TYPE_IDENTIFIER_FLOW_LABEL_IE_XML_STR    "component_type_identifier_flow_label"
#define PACKET_FILTER_CONTENTS_FLOW_LABEL_IE_XML_STR                     "value"


#define COMPONENT_TYPE_IDENTIFIER_ATTR_XML_STR              "component type identifier"

//------------------------------------------------------------------------------
// 10.5.7.3 GPRS Timer
//------------------------------------------------------------------------------
#define GPRS_TIMER_IE_XML_STR                        "gprs_timer"
#define GPRS_TIMER_T3402_IE_XML_STR                  "gprs_timer_t3402"
#define GPRS_TIMER_T3412_IE_XML_STR                  "gprs_timer_t3412"
#define GPRS_TIMER_T3423_IE_XML_STR                  "gprs_timer_t3423"
#define UNIT_IE_XML_STR                              "unit"
#define TIMER_VALUE_IE_XML_STR                       "timer_value"


#include <stdbool.h>
#include "3gpp_24.008.h"
#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
//******************************************************************************
// 10.5.1 Common information elements
//******************************************************************************
bool ciphering_key_sequence_number_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, ciphering_key_sequence_number_t * const cipheringkeysequencenumber);
void ciphering_key_sequence_number_to_xml ( const ciphering_key_sequence_number_t * const cipheringkeysequencenumber, xmlTextWriterPtr writer);
bool location_area_identification_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, location_area_identification_t * const locationareaidentification);
void location_area_identification_to_xml (const location_area_identification_t * const , xmlTextWriterPtr writer);
bool mobile_identity_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, mobile_identity_t * const mobileidentity);
void mobile_identity_to_xml (const mobile_identity_t * const mobileidentity, xmlTextWriterPtr writer);
bool mobile_station_classmark_2_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, mobile_station_classmark2_t * const mobilestationclassmark2);
void mobile_station_classmark_2_to_xml (const mobile_station_classmark2_t * const mobilestationclassmark2, xmlTextWriterPtr writer);
bool mobile_station_classmark_3_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, mobile_station_classmark3_t * const mobilestationclassmark3);
void mobile_station_classmark_3_to_xml (const mobile_station_classmark3_t * const mobilestationclassmark3, xmlTextWriterPtr writer);
bool plmn_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, plmn_t * const plmn);
void plmn_to_xml (const plmn_t * const plmn, xmlTextWriterPtr writer);
bool plmn_list_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, plmn_list_t * const plmnlist);
void plmn_list_to_xml (const plmn_list_t * const plmnlist, xmlTextWriterPtr writer);
bool ms_network_feature_support_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, ms_network_feature_support_t * const msnetworkfeaturesupport);
void ms_network_feature_support_to_xml(const ms_network_feature_support_t * const msnetworkfeaturesupport, xmlTextWriterPtr writer);

//******************************************************************************
// 10.5.3 Mobility management information elements.
//******************************************************************************
bool authentication_parameter_rand_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, authentication_parameter_rand_t * authenticationparameterrand);
void authentication_parameter_rand_to_xml (authentication_parameter_rand_t authenticationparameterrand, xmlTextWriterPtr writer);
bool authentication_parameter_autn_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, authentication_parameter_autn_t * authenticationparameterautn);
void authentication_parameter_autn_to_xml (authentication_parameter_autn_t authenticationparameterautn, xmlTextWriterPtr writer);
bool authentication_response_parameter_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, authentication_response_parameter_t * authenticationresponseparameter);
void authentication_response_parameter_to_xml (authentication_response_parameter_t authenticationresponseparameter, xmlTextWriterPtr writer);
bool authentication_failure_parameter_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, authentication_failure_parameter_t * authenticationfailureparameter);
void authentication_failure_parameter_to_xml (authentication_failure_parameter_t authenticationfailureparameter, xmlTextWriterPtr writer);
bool network_name_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, const char * const ie, network_name_t * const networkname);
void network_name_to_xml (const char * const ie, const network_name_t * const networkname, xmlTextWriterPtr writer);
bool time_zone_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, time_zone_t * const timezone);
void time_zone_to_xml (const time_zone_t * const timezone, xmlTextWriterPtr writer);
bool time_zone_and_time_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, time_zone_and_time_t * const timezoneandtime);
void time_zone_and_time_to_xml (const time_zone_and_time_t * const timezoneandtime, xmlTextWriterPtr writer);
bool daylight_saving_time_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, daylight_saving_time_t * const daylightsavingtime);
void daylight_saving_time_to_xml (const daylight_saving_time_t * const daylightsavingtime, xmlTextWriterPtr writer);
bool emergency_number_list_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, emergency_number_list_t * const emergencynumberlist);
void emergency_number_list_to_xml (const emergency_number_list_t * const emergencynumberlist, xmlTextWriterPtr writer);

//******************************************************************************
// 10.5.4 Call control information elements.
//******************************************************************************
bool supported_codec_list_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, supported_codec_list_t * supportedcodeclist);
void supported_codec_list_to_xml (const supported_codec_list_t * const supportedcodeclist, xmlTextWriterPtr writer);

//******************************************************************************
// 10.5.5 GPRS mobility management information elements
//******************************************************************************
bool tmsi_status_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, tmsi_status_t * const tmsistatus);
void tmsi_status_to_xml (const tmsi_status_t * const tmsistatus, xmlTextWriterPtr writer);
bool drx_parameter_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, drx_parameter_t * const drxparameter);
void drx_parameter_to_xml (const drx_parameter_t * const drxparameter, xmlTextWriterPtr writer);
bool p_tmsi_signature_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, p_tmsi_signature_t * ptmsisignature);
void p_tmsi_signature_to_xml ( const p_tmsi_signature_t * const ptmsisignature, xmlTextWriterPtr writer);
bool identity_type_2_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, identity_type2_t * const identitytype2);
void identity_type_2_to_xml (const identity_type2_t * const identitytype2, xmlTextWriterPtr writer);
bool imeisv_request_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, imeisv_request_t * const imeisvrequest);
void imeisv_request_to_xml (const imeisv_request_t * const imeisvrequest, xmlTextWriterPtr writer);
bool ms_network_capability_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, ms_network_capability_t * const msnetworkcapability);
void ms_network_capability_to_xml (const ms_network_capability_t  * const msnetworkcapability, xmlTextWriterPtr writer);
bool routing_area_code_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, routing_area_code_t * const rac);
void routing_area_code_to_xml (const routing_area_code_t * const rac, xmlTextWriterPtr writer);
bool routing_area_identification_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, routing_area_identification_t * const rai);
void routing_area_identification_to_xml (const routing_area_identification_t * const rai, xmlTextWriterPtr writer);
bool voice_domain_preference_and_ue_usage_setting_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx,voice_domain_preference_and_ue_usage_setting_t * const voicedomainpreferenceandueusagesetting);
void voice_domain_preference_and_ue_usage_setting_to_xml(const voice_domain_preference_and_ue_usage_setting_t * const voicedomainpreferenceandueusagesetting, xmlTextWriterPtr writer);
//******************************************************************************
// 10.5.6 Session management information elements
//******************************************************************************
bool access_point_name_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, access_point_name_t * access_point_name);
void access_point_name_to_xml (access_point_name_t access_point_name, xmlTextWriterPtr writer);
bool protocol_configuration_options_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, protocol_configuration_options_t * const pco, bool ms2network_direction);
void protocol_configuration_options_to_xml (const protocol_configuration_options_t * const pco, xmlTextWriterPtr writer, bool ms2network_direction);
bool quality_of_service_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, quality_of_service_t * const qualityofservice);
void quality_of_service_to_xml ( const quality_of_service_t * const qualityofservice, xmlTextWriterPtr writer);
bool linked_ti_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, linked_ti_t * const linkedti);
void linked_ti_to_xml ( const linked_ti_t * const linkedti, xmlTextWriterPtr writer);
bool llc_service_access_point_identifier_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, llc_service_access_point_identifier_t * const llc_sap_id);
void llc_service_access_point_identifier_to_xml (const llc_service_access_point_identifier_t * const llc_sap_id, xmlTextWriterPtr writer);
bool packet_flow_identifier_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, packet_flow_identifier_t * const packetflowidentifier);
void packet_flow_identifier_to_xml (const packet_flow_identifier_t * const packetflowidentifier, xmlTextWriterPtr writer);

bool packet_filter_content_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, packet_filter_contents_t * const pfc);
void packet_filter_content_to_xml (const packet_filter_contents_t * const packetfiltercontents, xmlTextWriterPtr writer);
bool packet_filter_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, packet_filter_t * const packetfilter);
void packet_filter_to_xml (const packet_filter_t * const packetfilter, xmlTextWriterPtr writer);
bool packet_filter_identifier_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, packet_filter_identifier_t * const packetfilteridentifier);
void packet_filter_identifier_to_xml (const packet_filter_identifier_t * const packetfilteridentifier, xmlTextWriterPtr writer);
bool traffic_flow_template_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, traffic_flow_template_t * const trafficflowtemplate);
void traffic_flow_template_to_xml (const traffic_flow_template_t * const trafficflowtemplate, xmlTextWriterPtr writer);

//******************************************************************************
// 10.5.7 GPRS Common information elements
//******************************************************************************
bool gprs_timer_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, const char * const ie, gprs_timer_t * const gprstimer);
void gprs_timer_to_xml (const char * const ie_xml_str, const gprs_timer_t * const gprstimer, xmlTextWriterPtr writer);

#endif /* FILE_3GPP_24_008_XML_SEEN */


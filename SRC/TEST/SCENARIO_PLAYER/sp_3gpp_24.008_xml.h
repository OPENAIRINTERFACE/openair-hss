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

#ifndef FILE_SP_3GPP_24_008_XML_SEEN
#define FILE_SP_3GPP_24_008_XML_SEEN
#include "sp_xml_load.h"
//******************************************************************************
// 10.5.1 Common information elements
//******************************************************************************
//******************************************************************************
// 10.5.1 Common information elements
//******************************************************************************
SP_NUM_FROM_XML_PROTOTYPE(ciphering_key_sequence_number);
bool sp_location_area_identification_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, location_area_identification_t * const locationareaidentification);
bool sp_mobile_identity_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, mobile_identity_t * const mobileidentity);
bool sp_mobile_station_classmark_2_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, mobile_station_classmark2_t * const mobilestationclassmark2);
bool sp_mobile_station_classmark_3_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, mobile_station_classmark3_t * const mobilestationclassmark3);
bool sp_plmn_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, plmn_t * const plmn);
bool sp_plmn_list_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, plmn_list_t * const plmnlist);
bool sp_ms_network_feature_support_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, ms_network_feature_support_t * const msnetworkfeaturesupport);

//******************************************************************************
// 10.5.3 Mobility management information elements.
//******************************************************************************
//bool sp_authentication_parameter_rand_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, authentication_parameter_rand_t * authenticationparameterrand);
//bool sp_authentication_parameter_autn_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, authentication_parameter_autn_t * authenticationparameterautn);
//bool sp_authentication_response_parameter_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, authentication_response_parameter_t * authenticationresponseparameter);
//bool sp_authentication_failure_parameter_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, authentication_failure_parameter_t * authenticationfailureparameter);
//bool sp_network_name_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, const char * const ie, network_name_t * const networkname);
//bool sp_time_zone_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, time_zone_t * const timezone);
//bool sp_time_zone_and_time_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, time_zone_and_time_t * const timezoneandtime);
//bool sp_daylight_saving_time_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, daylight_saving_time_t * const daylightsavingtime);
//bool sp_emergency_number_list_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, emergency_number_list_t * const emergencynumberlist);

//******************************************************************************
// 10.5.4 Call control information elements.
//******************************************************************************
//bool sp_supported_codec_list_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, supported_codec_list_t * supportedcodeclist);

//******************************************************************************
// 10.5.5 GPRS mobility management information elements
//******************************************************************************
//bool sp_tmsi_status_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, tmsi_status_t * const tmsistatus);
//bool sp_drx_parameter_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, drx_parameter_t * const drxparameter);
//bool sp_p_tmsi_signature_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, p_tmsi_signature_t * ptmsisignature);
//bool sp_identity_type_2_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, identity_type2_t * const identitytype2);
//bool sp_imeisv_request_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, imeisv_request_t * const imeisvrequest);
//bool sp_ms_network_capability_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, ms_network_capability_t * const msnetworkcapability);
//bool sp_routing_area_code_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, routing_area_code_t * const rac);
//bool sp_routing_area_identification_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, routing_area_identification_t * const rai);
//bool sp_voice_domain_preference_and_ue_usage_setting_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg,voice_domain_preference_and_ue_usage_setting_t * const voicedomainpreferenceandueusagesetting);
//******************************************************************************
// 10.5.6 Session management information elements
//******************************************************************************
bool sp_access_point_name_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, access_point_name_t * access_point_name);
bool sp_protocol_configuration_options_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, protocol_configuration_options_t * const pco, bool ms2network_direction);
bool sp_quality_of_service_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, quality_of_service_t * const qualityofservice);
bool sp_linked_ti_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, linked_ti_t * const linkedti);
bool sp_llc_service_access_point_identifier_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, llc_service_access_point_identifier_t * const llc_sap_id);
bool sp_packet_flow_identifier_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, packet_flow_identifier_t * const packetflowidentifier);
//
bool sp_packet_filter_content_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, packet_filter_contents_t * const pfc);
bool sp_packet_filter_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, packet_filter_t * const packetfilter);
bool sp_packet_filter_identifier_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, packet_filter_identifier_t * const packetfilteridentifier);
bool sp_traffic_flow_template_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, traffic_flow_template_t * const trafficflowtemplate);

//******************************************************************************
// 10.5.7 GPRS Common information elements
//******************************************************************************
//bool sp_gprs_timer_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, const char * const ie, gprs_timer_t * const gprstimer);

#endif /* FILE_SP_3GPP_24_008_XML_SEEN */


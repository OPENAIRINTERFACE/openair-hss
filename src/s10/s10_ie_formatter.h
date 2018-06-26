/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under 
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.  
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */


#ifndef FILE_S10_IE_FORMATTER_SEEN
#define FILE_S10_IE_FORMATTER_SEEN

#include "../gtpv2-c/gtpv2c_ie_formatter/shared/gtpv2c_ie_formatter.h"
#include "3gpp_29.274.h"

int s10_guti_ie_set ( nw_gtpv2c_msg_handle_t * msg, const guti_t * guti);

nw_rc_t s10_guti_ie_get ( uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t * ieValue, void *arg);

nw_rc_t s10_msisdn_ie_get(
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

/* Node Type Information Element
 * 3GPP TS 29.274 #8.34
 * Node type:
 *      * 0 = MME
 *      * 1 = SGSN
 */
nw_rc_t s10_node_type_ie_get(uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

int s10_node_type_ie_set(nw_gtpv2c_msg_handle_t *msg, const node_type_t *node_type);

/**
 * MM EPS Context Information Element
 * 3GPP TS 29.274 #8.38
 */
nw_rc_t s10_ue_mm_eps_context_ie_set ( nw_gtpv2c_msg_handle_t * msg, const mm_context_eps_t * ue_eps_mm_context);

nw_rc_t
s10_mm_ue_context_ie_get (
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t * ieValue, void *arg);

/**
 * PDN Connection Grouped Information Element
 * 3GPP TS 29.274 #8.39
 */
nw_rc_t s10_pdn_connection_ie_set ( nw_gtpv2c_msg_handle_t * msg, void * arg);

nw_rc_t
s10_pdn_connections_ie_get ( uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t * ieValue, void *arg);

/* F-Cause Information Element
 * 3GPP TS 29.274 #8.49
 */
int s10_f_cause_ie_set (nw_gtpv2c_msg_handle_t * msg, const F_Cause_t * arg);

/**
 * Complete Request Message.
 */
nw_rc_t
s10_complete_request_message_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg);

nw_rc_t
s10_f_container_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg);

nw_rc_t
s10_f_container_ie_get_2 (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg);

/**
 * Target Identification Information Element
 * 3GPP TS 29.274 #8.51
 */
int s10_target_identification_ie_set (  nw_gtpv2c_msg_handle_t * msg,  const target_identification_t * target_identification);

/* PDN Type Information Element
 * 3GPP TS 29.274 #8.34
 * PDN type:
 *      * 1 = IPv4
 *      * 2 = IPv6
 *      * 3 = IPv4v6
 */
nw_rc_t s10_pdn_type_ie_get(
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

int s10_pdn_type_ie_set(nw_gtpv2c_msg_handle_t *msg, const pdn_type_t *pdn_type);

/* RAT type Information Element
 * WARNING: the RAT type used in MME and S/P-GW is not the same as the one
 * for S10 interface defined in 3GPP TS 29.274 #8.17.
 */
nw_rc_t s10_rat_type_ie_get(
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

int s10_rat_type_ie_set(nw_gtpv2c_msg_handle_t *msg, const rat_type_t *rat_type);

/* EPS Bearer Id Information Element
 * 3GPP TS 29.274 #8.8
 */
nw_rc_t s10_ebi_ie_get(
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

int s10_ebi_ie_set(nw_gtpv2c_msg_handle_t *msg, const unsigned ebi);

/* Bearer Contexts to Create Information Element as part of Create Session Request
 * 3GPP TS 29.274 Table 7.2.1-2.
 */
//nw_rc_t s10_bearer_context_to_be_created_ie_get (uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t * ieValue, void *arg);

int s10_bearer_context_to_be_modified_ie_set (nw_gtpv2c_msg_handle_t * msg, const bearer_context_to_be_modified_t * bearer_context);

nw_rc_t s10_bearer_context_to_be_modified_ie_get(uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

/* EPS Bearer Id Information Element
 * 3GPP TS 29.274 #8.8
 * ebi is 4 bits long
 */
int s10_ebi_ie_set(nw_gtpv2c_msg_handle_t *msg, const unsigned ebi);

nw_rc_t s10_ebi_ie_get (uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t * ieValue, void *arg);

nw_rc_t s10_ebi_ie_get_list (uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t * ieValue, void *arg);

/** Set the Bearer Context IE. */
int s10_bearer_context_to_create_ie_set (nw_gtpv2c_msg_handle_t * msg, const bearer_contexts_to_be_created_t * bearer_contexts);

nw_rc_t
s10_bearer_context_to_be_created_ie_get ( uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t * ieValue, void *arg);

/* Bearer Context Created grouped Information Element */
nw_rc_t s10_bearer_context_created_ie_get(uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

int s10_bearer_context_created_ie_set(nw_gtpv2c_msg_handle_t *msg, const bearer_context_to_be_created_t *bc_tbc);

/* Serving Network Information Element
 * 3GPP TS 29.274 #8.18
 */
nw_rc_t s10_serving_network_ie_get(
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

int s10_serving_network_ie_set(
  nw_gtpv2c_msg_handle_t     *msg,
  const ServingNetwork_t *serving_network);


/* Fully Qualified TEID (F-TEID) Information Element */
int s10_fteid_ie_set (nw_gtpv2c_msg_handle_t * msg, const fteid_t * fteid);

nw_rc_t s10_fteid_ie_get(uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

/* Protocol Configuration Options Information Element */
nw_rc_t s10_pco_ie_get (uint8_t ieType, uint16_t ieLength,
  uint8_t ieInstance, uint8_t * ieValue, void *arg);

int s10_pco_ie_set (nw_gtpv2c_msg_handle_t * msg,
  const protocol_configuration_options_t * pco);

/* Access Point Name Information Element
 * 3GPP TS 29.274 #8.6
 * NOTE: The APN field is not encoded as a dotted string as commonly used in
 * documentation.
 * The encoding of the APN field follows 3GPP TS 23.003 subclause 9.1
 */
nw_rc_t s10_apn_ie_get(
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, bstring *arg);

int s10_apn_ie_set(nw_gtpv2c_msg_handle_t *msg, const bstring apn);

int s10_apn_plmn_ie_set(nw_gtpv2c_msg_handle_t *msg, const char *apn, const ServingNetwork_t * serving_network);

nw_rc_t s10_mei_ie_get(
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

nw_rc_t s10_uli_ie_get(
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

/* APN restrtiction Information Element */
int s10_apn_restriction_ie_set(
  nw_gtpv2c_msg_handle_t *msg, const uint8_t apn_restriction);

/* Bearer level Qos Information Element
 * 3GPP TS 29.274 #8.15
 */
nw_rc_t s10_bearer_qos_ie_get (uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t * ieValue, void *arg);

int s10_bearer_qos_ie_set(nw_gtpv2c_msg_handle_t *msg, const bearer_qos_t *bearer_qos);

/* IP address Information Element
 * 3GPP TS 29.274 #8.9
 */
//nw_rc_t s10_ip_address_ie_get(
//  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);
//
//int s10_ip_address_ie_set(nw_gtpv2c_msg_handle_t     *msg,
//                          const gtp_ip_address_t *ip_address);

nw_rc_t
s10_pdn_address_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg);

int
s10_pdn_address_ie_set (
    nw_gtpv2c_msg_handle_t * msg,
  NW_IN const struct in_addr  const * ipv4Addr);

int
s10_ipv6_address_ie_set (
    nw_gtpv2c_msg_handle_t * msg,
  NW_IN const struct in6_addr  const * ipv6Addr);

int s10_apn_restriction_ie_get (
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t * ieValue, void *arg);

/* Delay Value Information Element
 * 3GPP TS 29.274 #8.27
 */
nw_rc_t s10_delay_value_ie_get(
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

int s10_delay_value_ie_set(nw_gtpv2c_msg_handle_t *msg,
                           const DelayValue_t *delay_value);

/* UE Time Zone Information Element
 * 3GPP TS 29.274 #8.44
 */
nw_rc_t s10_ue_time_zone_ie_get(
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

int s10_ue_time_zone_ie_set(nw_gtpv2c_msg_handle_t *msg,
                            const UETimeZone_t *ue_time_zone);

/* Target Identification Information Element
 * 3GPP TS 29.274 #8.51
 */
nw_rc_t s10_target_identification_ie_get(
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

/* Bearer Flags Information Element
 * 3GPP TS 29.274 #8.32
 */
nw_rc_t s10_bearer_flags_ie_get(
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

int s10_bearer_flags_ie_set(nw_gtpv2c_msg_handle_t   *msg,
                            const bearer_flags_t *bearer_flags);

/* Indication Element
 * 3GPP TS 29.274 #8.12
 */
nw_rc_t s10_indication_flags_ie_get(
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

int
s10_indication_flags_ie_set (
  nw_gtpv2c_msg_handle_t * msg,
  const indication_flags_t * indication_flags);

#endif /* FILE_S10_IE_FORMATTER_SEEN */

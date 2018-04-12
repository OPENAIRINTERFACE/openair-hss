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

#ifndef FILE_SP_3GPP_24_301_ESM_IES_XML_SEEN
#define FILE_SP_3GPP_24_301_ESM_IES_XML_SEEN
#include "sp_xml_load.h"
//==============================================================================
// 9 General message format and information elements coding
//==============================================================================

//------------------------------------------------------------------------------
// 9.9.4 EPS Session Management (ESM) information elements
//------------------------------------------------------------------------------
// 9.9.4.1 Access point name
// See subclause 10.5.6.1 in 3GPP TS 24.008 [13].

// 9.9.4.2 APN aggregate maximum bit rate
bool sp_apn_aggregate_maximum_bit_rate_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    ApnAggregateMaximumBitRate * const apnaggregatemaximumbitrate);

// 9.9.4.2A Connectivity type
// See subclause 10.5.6.19 in 3GPP TS 24.008 [13].

// 9.9.4.3 EPS quality of service
bool sp_eps_quality_of_service_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    EpsQualityOfService   * const epsqualityofservice);

// 9.9.4.4 ESM cause
SP_NUM_FROM_XML_PROTOTYPE(esm_cause);

// 9.9.4.5 ESM information transfer flag
SP_NUM_FROM_XML_PROTOTYPE(esm_information_transfer_flag);

// 9.9.4.6 Linked EPS bearer identity
SP_NUM_FROM_XML_PROTOTYPE(linked_eps_bearer_identity);

// 9.9.4.7 LLC service access point identifier
// See subclause 10.5.6.9 in 3GPP TS 24.008 [13].

// 9.9.4.7A Notification indicator
// TODO

// 9.9.4.8 Packet flow identifier
// See subclause 10.5.6.11 in 3GPP TS 24.008 [13].

// 9.9.4.9 PDN address
bool sp_pdn_address_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    PdnAddress * const pdnaddress);

// 9.9.4.10 PDN type
bool sp_pdn_type_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    pdn_type_t * const pdntype);

// 9.9.4.11 Protocol configuration options
// See subclause 10.5.6.3 in 3GPP TS 24.008 [13].
// 9.9.4.12 Quality of service
// See subclause 10.5.6.5 in 3GPP TS 24.008 [13].
// 9.9.4.13 Radio priority
// See subclause 10.5.7.2 in 3GPP TS 24.008 [13].

// 9.9.4.14 Request type
// See subclause 10.5.6.17 in 3GPP TS 24.008 [13].
SP_NUM_FROM_XML_PROTOTYPE(request_type);


// 9.9.4.15 Traffic flow aggregate description
// TODO void traffic_flow_aggregate_description_to_xml(TrafficFlowAggregateDescription *trafficflowaggregatedescription, uint8_t iei);

// 9.9.4.16 Traffic flow template
// See subclause 10.5.6.12 in 3GPP TS 24.008 [13].

// 9.9.4.17 Transaction identifier
// The Transaction identifier information element is coded as the Linked TI information element in 3GPP TS 24.008 [13],
// subclause 10.5.6.7.

#endif /* FILE_SP_3GPP_24_301_ESM_IES_XML_SEEN */

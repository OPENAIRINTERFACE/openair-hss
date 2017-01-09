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

#ifndef FILE_SP_3GPP_24_301_EMM_IES_XML_SEEN
#define FILE_SP_3GPP_24_301_EMM_IES_XML_SEEN

#include "sp_xml_load.h"
#include "AdditionalUpdateResult.h"
#include "AdditionalUpdateType.h"
#include "CsfbResponse.h"
#include "DetachType.h"
#include "EmmCause.h"
#include "EpsAttachResult.h"
#include "EpsAttachType.h"
#include "EpsMobileIdentity.h"
#include "EpsNetworkFeatureSupport.h"
#include "EpsUpdateResult.h"
#include "EpsUpdateType.h"
#include "EsmMessageContainer.h"
#include "KsiAndSequenceNumber.h"
#include "NasKeySetIdentifier.h"
#include "NasMessageContainer.h"
#include "NasSecurityAlgorithms.h"
#include "Nonce.h"
#include "PagingIdentity.h"
#include "ServiceType.h"
#include "ShortMac.h"
#include "TrackingAreaIdentity.h"
#include "TrackingAreaIdentityList.h"
#include "3gpp_24.301.h"
#include "UeNetworkCapability.h"
#include "UeRadioCapabilityInformationUpdateNeeded.h"
#include "UeSecurityCapability.h"
#include "Cli.h"
#include "SsCode.h"
#include "LcsIndicator.h"
#include "LcsClientIdentity.h"
#include "GutiType.h"


//==============================================================================
// 9 General message format and information elements coding
//==============================================================================

//------------------------------------------------------------------------------
// 9.9.3 EPS Mobility Management (EMM) information elements
//------------------------------------------------------------------------------
// 9.9.3.0A Additional update result
SP_NUM_FROM_XML_PROTOTYPE( additional_update_result );

// 9.9.3.0B Additional update type
SP_NUM_FROM_XML_PROTOTYPE(additional_update_type);

// 9.9.3.1 Authentication failure parameter
// See subclause 10.5.3.2.2 in 3GPP TS 24.008 [13].

// 9.9.3.2 Authentication parameter AUTN
// See subclause 10.5.3.1.1 in 3GPP TS 24.008 [13].

// 9.9.3.3 Authentication parameter RAND
// See subclause 10.5.3.1 in 3GPP TS 24.008 [13].

// 9.9.3.4 Authentication response parameter
// in 24.008_mm_ies_xml.c

// 9.9.3.4A Ciphering key sequence number
// See subclause 10.5.1.2 in 3GPP TS 24.008 [13].

// 9.9.3.5 CSFB response
SP_NUM_FROM_XML_PROTOTYPE(csfb_response);

// 9.9.3.6 Daylight saving time
// See subclause 10.5.3.12 in 3GPP TS 24.008 [13].

// 9.9.3.7 Detach type
bool sp_detach_type_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    detach_type_t         * const detachtype);


// 9.9.3.8 DRX parameter
// See subclause 10.5.5.6 in 3GPP TS 24.008 [13].

// 9.9.3.9 EMM cause
SP_NUM_FROM_XML_PROTOTYPE(emm_cause);

// 9.9.3.10 EPS attach result
SP_NUM_FROM_XML_PROTOTYPE(eps_attach_result);

// 9.9.3.11 EPS attach type
SP_NUM_FROM_XML_PROTOTYPE(eps_attach_type);

// 9.9.3.12 EPS mobile identity
bool sp_eps_mobile_identity_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    eps_mobile_identity_t * const epsmobileidentity);

// 9.9.3.12A EPS network feature support
SP_NUM_FROM_XML_PROTOTYPE(eps_network_feature_support);

// 9.9.3.13 EPS update result
SP_NUM_FROM_XML_PROTOTYPE(eps_update_result);

// 9.9.3.14 EPS update type
// TODO from_xml

// 9.9.3.15 ESM message container
bool sp_esm_message_container_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    EsmMessageContainer           *esmmessagecontainer);

// 9.9.3.16 GPRS timer
// See subclause 10.5.7.3 in 3GPP TS 24.008 [13].
// 9.9.3.16A GPRS timer 2
// See subclause 10.5.7.4 in 3GPP TS 24.008 [13].
// 9.9.3.16B GPRS timer 3
// See subclause 10.5.7.4a in 3GPP TS 24.008 [13].
// 9.9.3.17 Identity type 2
// See subclause 10.5.5.9 in 3GPP TS 24.008 [13].
// 9.9.3.18 IMEISV request
// See subclause 10.5.5.10 in 3GPP TS 24.008 [13].

// 9.9.3.19 KSI and sequence number

// 9.9.3.20 MS network capability
// See subclause 10.5.5.12 in 3GPP TS 24.008 [13].
// 9.9.3.20A MS network feature support
// See subclause 10.5.1.15 in 3GPP TS 24.008 [13].

// 9.9.3.21 NAS key set identifier
bool sp_nas_key_set_identifier_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    NasKeySetIdentifier   * const naskeysetidentifier);

// 9.9.3.22 NAS message container
#define NAS_MESSAGE_CONTAINER_IE_XML_STR "nas_message_container"
bool sp_nas_message_container_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    NasMessageContainer   * const nasmessagecontainer);

// 9.9.3.23 NAS security algorithms
bool sp_nas_security_algorithms_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    NasSecurityAlgorithms * const nassecurityalgorithms);

// 9.9.3.24 Network name
// See subclause 10.5.3.5a in 3GPP TS 24.008 [13].

// 9.9.3.25 Nonce
bool sp_nonce_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    const char            * const ie,
    nonce_t               * const nonce);

// 9.9.3.25A Paging identity
SP_NUM_FROM_XML_PROTOTYPE(paging_identity);

// 9.9.3.26 P-TMSI signature
// See subclause 10.5.5.8 in 3GPP TS 24.008 [13].

// 9.9.3.27 Service type
SP_NUM_FROM_XML_PROTOTYPE(service_type);

// 9.9.3.28 Short MAC
SP_NUM_FROM_XML_PROTOTYPE(short_mac);

// 9.9.3.29 Time zone
// See subclause 10.5.3.8 in 3GPP TS 24.008 [13].

// 9.9.3.30 Time zone and time
// See subclause 10.5.3.9 in 3GPP TS 24.008 [13].

// 9.9.3.31 TMSI status
// See subclause 10.5.5.4 in 3GPP TS 24.008 [13].

// 9.9.3.32 Tracking area identity
bool sp_tracking_area_identity_from_xml(
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    tai_t * const tai);
bool sp_tracking_area_code_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    tac_t * const tac);

// 9.9.3.33 Tracking area identity list
bool sp_tracking_area_identity_list_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    tai_list_t            * const tai_list);

// 9.9.3.34 UE network capability
bool sp_ue_network_capability_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    ue_network_capability_t * const uenetworkcapability);

// 9.9.3.35 UE radio capability information update needed
SP_NUM_FROM_XML_PROTOTYPE(ue_radio_capability_information_update_needed);

// 9.9.3.36 UE security capability
bool sp_ue_security_capability_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    ue_security_capability_t           * const uesecuritycapability);

// 9.9.3.37 Emergency Number List
// See subclause 10.5.3.13 in 3GPP TS 24.008 [13].

// 9.9.3.38 CLI
// TODO cli_from_xml

// 9.9.3.39 SS Code
SP_NUM_FROM_XML_PROTOTYPE(ss_code);

// 9.9.3.40 LCS indicator
SP_NUM_FROM_XML_PROTOTYPE(lcs_indicator);

// 9.9.3.41 LCS client identity
// TODO lcs_client_identity_from_xml

// 9.9.3.42 Generic message container type
// TODO ?
// 9.9.3.43 Generic message container
// TODO ?
// 9.9.3.44 Voice domain preference and UE's usage setting
// See subclause 10.5.5.28 in 3GPP TS 24.008 [13].

// 9.9.3.45 GUTI type
bool sp_guti_type_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    guti_type_t * const gutitype);


#endif /* FILE_SP_3GPP_24_301_EMM_IES_XML_SEEN */

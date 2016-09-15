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

#ifndef FILE_3GPP_24_301_EMM_IES_XML_SEEN
#define FILE_3GPP_24_301_EMM_IES_XML_SEEN

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

#include "xml_load.h"

//==============================================================================
// 9 General message format and information elements coding
//==============================================================================

//------------------------------------------------------------------------------
// 9.9.3 EPS Mobility Management (EMM) information elements
//------------------------------------------------------------------------------
// 9.9.3.0A Additional update result
#define ADDITIONAL_UPDATE_RESULT_IE_XML_STR           "additional_update_result"
#define ADDITIONAL_UPDATE_RESULT_XML_SCAN_FMT         "%"SCNx8
#define ADDITIONAL_UPDATE_RESULT_XML_FMT              "%"PRIx8
NUM_FROM_XML_PROTOTYPE( additional_update_result );
void additional_update_result_to_xml(additional_update_result_t *additionalupdateresult, xmlTextWriterPtr writer);

// 9.9.3.0B Additional update type
#define ADDITIONAL_UPDATE_TYPE_IE_XML_STR           "additional_update_type"
#define ADDITIONAL_UPDATE_TYPE_XML_SCAN_FMT         "%"SCNx8
#define ADDITIONAL_UPDATE_TYPE_XML_FMT              "%"PRIx8
NUM_FROM_XML_PROTOTYPE(additional_update_type);
void additional_update_type_to_xml(additional_update_type_t *additionalupdatetype, xmlTextWriterPtr writer);

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
#define CSFB_RESPONSE_IE_XML_STR           "csfb_response"
#define CSFB_RESPONSE_XML_SCAN_FMT         "%"SCNx8
#define CSFB_RESPONSE_XML_FMT              "%"PRIx8
NUM_FROM_XML_PROTOTYPE(csfb_response);
void csfb_response_to_xml(csfb_response_t *csfbresponse, xmlTextWriterPtr writer);

// 9.9.3.6 Daylight saving time
// See subclause 10.5.3.12 in 3GPP TS 24.008 [13].

// 9.9.3.7 Detach type
bool detach_type_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    detach_type_t            * const detachtype);

void detach_type_to_xml(detach_type_t *detachtype, xmlTextWriterPtr writer);

// 9.9.3.8 DRX parameter
// See subclause 10.5.5.6 in 3GPP TS 24.008 [13].

// 9.9.3.9 EMM cause
#define EMM_CAUSE_IE_XML_STR           "emm_cause"
#define EMM_CAUSE_XML_SCAN_FMT         "%"SCNx8
#define EMM_CAUSE_XML_FMT              "%"PRIx8
NUM_FROM_XML_PROTOTYPE(emm_cause);
void emm_cause_to_xml (emm_cause_t * emmcause, xmlTextWriterPtr writer);

// 9.9.3.10 EPS attach result
#define EPS_ATTACH_RESULT_IE_XML_STR        "eps_attach_result"
#define EPS_ATTACH_RESULT_XML_SCAN_FMT      "%"SCNx8
#define EPS_ATTACH_RESULT_XML_FMT           "%"PRIx8
NUM_FROM_XML_PROTOTYPE(eps_attach_result);
void eps_attach_result_to_xml (eps_attach_result_t * epsattachresult, xmlTextWriterPtr writer);

// 9.9.3.11 EPS attach type
#define EPS_ATTACH_TYPE_IE_XML_STR        "eps_attach_type"
#define EPS_ATTACH_TYPE_XML_SCAN_FMT      "%"SCNx8
#define EPS_ATTACH_TYPE_XML_FMT           "%"PRIx8
NUM_FROM_XML_PROTOTYPE(eps_attach_type);
void eps_attach_type_to_xml (eps_attach_type_t * epsattachtype, xmlTextWriterPtr writer);

// 9.9.3.12 EPS mobile identity
bool eps_mobile_identity_from_xml (
    xmlDocPtr                              xml_doc,
    xmlXPathContextPtr                     xpath_ctx,
    eps_mobile_identity_t          * const epsmobileidentity);

void eps_mobile_identity_to_xml (eps_mobile_identity_t * epsmobileidentity, xmlTextWriterPtr writer);

// 9.9.3.12A EPS network feature support
#define EPS_NETWORK_FEATURE_SUPPORT_IE_XML_STR                     "eps_network_feature_support"
#define EPS_NETWORK_FEATURE_SUPPORT_XML_SCAN_FMT                   "%"SCNx8
#define EPS_NETWORK_FEATURE_SUPPORT_XML_FMT                        "%"PRIx8
NUM_FROM_XML_PROTOTYPE(eps_network_feature_support);
void eps_network_feature_support_to_xml (eps_network_feature_support_t * epsnetworkfeaturesupport, xmlTextWriterPtr writer);

// 9.9.3.13 EPS update result
#define EPS_UPDATE_RESULT_IE_XML_STR                     "eps_update_result"
#define EPS_UPDATE_RESULT_XML_SCAN_FMT                   "%"SCNx8
#define EPS_UPDATE_RESULT_XML_FMT                        "%"PRIx8
NUM_FROM_XML_PROTOTYPE(eps_update_result);
void eps_update_result_to_xml (eps_update_result_t * epsupdateresult, xmlTextWriterPtr writer);

// 9.9.3.14 EPS update type
#define EPS_UPDATE_TYPE_IE_XML_STR                     "eps_update_type"
#define ACTIVE_FLAG_ATTR_XML_STR                       "active_flag"
#define EPS_UPDATE_TYPE_VALUE_ATTR_XML_STR             "eps_update_type_value"
// TODO from_xml
void eps_update_type_to_xml (EpsUpdateType * epsupdatetype, xmlTextWriterPtr writer);

// 9.9.3.15 ESM message container
#define ESM_MESSAGE_CONTAINER_IE_XML_STR                "esm_message_container"
#define ESM_MESSAGE_CONTAINER_HEX_DUMP_ATTR_XML_STR     "hex_dump"
bool esm_message_container_from_xml (
    xmlDocPtr                              xml_doc,
    xmlXPathContextPtr                     xpath_ctx,
    EsmMessageContainer            esmmessagecontainer);
void esm_message_container_to_xml (EsmMessageContainer esmmessagecontainer, xmlTextWriterPtr writer);

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
#define KSI_AND_SEQUENCE_NUMBER_IE_XML_STR                        "ksi_and_sequence_number"
#define KSI_ATTR_XML_STR                                          "ksi"
#define SEQUENCE_NUMBER_ATTR_XML_STR                              "sequence_number"
void ksi_and_sequence_number_to_xml (KsiAndSequenceNumber * ksiandsequencenumber, xmlTextWriterPtr writer);

// 9.9.3.20 MS network capability
// See subclause 10.5.5.12 in 3GPP TS 24.008 [13].
// 9.9.3.20A MS network feature support
// See subclause 10.5.1.15 in 3GPP TS 24.008 [13].

// 9.9.3.21 NAS key set identifier
#define NAS_KEY_SET_IDENTIFIER_IE_XML_STR   "nas_key_set_identifier"
#define TSC_ATTR_XML_STR                    "tsc"
#define NAS_KEY_SET_IDENTIFIER_ATTR_XML_STR "ksi"
bool nas_key_set_identifier_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, NasKeySetIdentifier * const naskeysetidentifier);
void nas_key_set_identifier_to_xml (NasKeySetIdentifier * naskeysetidentifier, xmlTextWriterPtr writer);

// 9.9.3.22 NAS message container
#define NAS_MESSAGE_CONTAINER_IE_XML_STR "nas_message_container"
bool nas_message_container_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    NasMessageContainer   * const nasmessagecontainer);
void nas_message_container_to_xml (NasMessageContainer nasmessagecontainer, xmlTextWriterPtr writer);

// 9.9.3.23 NAS security algorithms
#define NAS_SECURITY_ALGORITHMS_IE_XML_STR                  "nas_security_algorithms"
#define TYPE_OF_CYPHERING_ALGORITHM_ATTR_XML_STR            "type_of_ciphering_algorithm"
#define TYPE_OF_INTEGRITY_PROTECTION_ALGORITHM_ATTR_XML_STR "type_of_integrity_protection_algorithm"
bool nas_security_algorithms_from_xml (
    xmlDocPtr                              xml_doc,
    xmlXPathContextPtr                     xpath_ctx,
    NasSecurityAlgorithms          * const nassecurityalgorithms);
void nas_security_algorithms_to_xml (NasSecurityAlgorithms * nassecurityalgorithms, xmlTextWriterPtr writer);

// 9.9.3.24 Network name
// See subclause 10.5.3.5a in 3GPP TS 24.008 [13].

// 9.9.3.25 Nonce
#define NONCE_IE_XML_STR             "nonce"
#define REPLAYED_NONCE_UE_IE_XML_STR "replayed_nonce_ue"
#define NONCE_MME_IE_XML_STR         "nonce_mme"
#define NONCE_XML_SCAN_FMT           "%"SCNx32
#define NONCE_XML_FMT                "%"PRIx32
bool nonce_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, const char * const ie, nonce_t * const nonce);
void nonce_to_xml (const char * const ie, nonce_t * nonce, xmlTextWriterPtr writer);

// 9.9.3.25A Paging identity
#define PAGING_IDENTITY_IE_XML_STR      "pagingidentity"
#define PAGING_IDENTITY_XML_SCAN_FMT    "%"SCNx8
#define PAGING_IDENTITY_XML_FMT         "%"PRIx8
NUM_FROM_XML_PROTOTYPE(paging_identity);
void paging_identity_to_xml (paging_identity_t * pagingidentity, xmlTextWriterPtr writer);

// 9.9.3.26 P-TMSI signature
// See subclause 10.5.5.8 in 3GPP TS 24.008 [13].

// 9.9.3.27 Service type
#define SERVICE_TYPE_IE_XML_STR      "service_type"
#define SERVICE_TYPE_XML_SCAN_FMT    "%"SCNx8
#define SERVICE_TYPE_XML_FMT         "%"PRIx8
NUM_FROM_XML_PROTOTYPE(service_type);
void service_type_to_xml (service_type_t * servicetype, xmlTextWriterPtr writer);

// 9.9.3.28 Short MAC
#define SHORT_MAC_IE_XML_STR      "short_mac"
#define SHORT_MAC_XML_SCAN_FMT    "%"SCNx16
#define SHORT_MAC_XML_FMT         "%"PRIx16
NUM_FROM_XML_PROTOTYPE(short_mac);
void short_mac_to_xml (short_mac_t * shortmac, xmlTextWriterPtr writer);

// 9.9.3.29 Time zone
// See subclause 10.5.3.8 in 3GPP TS 24.008 [13].

// 9.9.3.30 Time zone and time
// See subclause 10.5.3.9 in 3GPP TS 24.008 [13].

// 9.9.3.31 TMSI status
// See subclause 10.5.5.4 in 3GPP TS 24.008 [13].

// 9.9.3.32 Tracking area identity
#define TRACKING_AREA_CODE_IE_XML_STR                "tracking_area_code"
#define TRACKING_AREA_IDENTITY_IE_XML_STR            "tracking_area_identity"
bool tracking_area_identity_from_xml(xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, tai_t * const tai);
bool tracking_area_code_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, tac_t * const tac);
void tracking_area_code_to_xml (const tac_t * const tac, xmlTextWriterPtr writer);
void tracking_area_identity_to_xml (const tai_t * const tai, xmlTextWriterPtr writer);

// 9.9.3.33 Tracking area identity list
#define TRACKING_AREA_IDENTITY_LIST_IE_XML_STR                 "tracking_area_identity_list"
#define PARTIAL_TRACKING_AREA_IDENTITY_LIST_IE_XML_STR         "partial_tracking_area_identity_list"
#define PARTIAL_TRACKING_AREA_IDENTITY_LIST_TYPE_IE_XML_STR    "type_of_list"
#define PARTIAL_TRACKING_AREA_IDENTITY_LIST_NUM_IE_XML_STR     "number_of_elements"
bool tracking_area_identity_list_from_xml (
    xmlDocPtr                     xml_doc,
    xmlXPathContextPtr            xpath_ctx,
    tai_list_t            * const tai_list);
void tracking_area_identity_list_to_xml (tai_list_t * trackingareaidentitylist, xmlTextWriterPtr writer);

// 9.9.3.34 UE network capability
#define UE_NETWORK_CAPABILITY_IE_XML_STR                                         "ue_network_capability"
#define UE_NETWORK_CAPABILITY_EPS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR   "eea"
#define UE_NETWORK_CAPABILITY_EPS_INTEGRITY_ALGORITHMS_SUPPORTED_ATTR_XML_STR    "eia"
#define UE_NETWORK_CAPABILITY_UMTS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR  "uea"
#define UE_NETWORK_CAPABILITY_UMTS_INTEGRITY_ALGORITHMS_SUPPORTED_ATTR_XML_STR   "uia"
#define UE_NETWORK_CAPABILITY_UCS2_SUPPORT_ATTR_XML_STR                          "ucs2"
#define UE_NETWORK_CAPABILITY_SPARE_ATTR_XML_STR                                 "spare"
#define UE_NETWORK_CAPABILITY_NF_ATTR_XML_STR                                    "nf"
#define UE_NETWORK_CAPABILITY_1xSRVCC_ATTR_XML_STR                               "1xsrvcc"
#define UE_NETWORK_CAPABILITY_LOCATION_SERVICES_ATTR_XML_STR                     "lcs"
#define UE_NETWORK_CAPABILITY_LTE_POSITIONING_PROTOCOL_ATTR_XML_STR              "lpp"
#define UE_NETWORK_CAPABILITY_ACCESS_CLASS_CONTROL_FOR_CSFB_ATTR_XML_STR         "csfb"
bool ue_network_capability_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, ue_network_capability_t * const uenetworkcapability);
void ue_network_capability_to_xml (ue_network_capability_t * uenetworkcapability, xmlTextWriterPtr writer);

// 9.9.3.35 UE radio capability information update needed
#define UE_RADIO_CAPABILITY_INFORMATION_UPDATE_NEEDED_IE_XML_STR      "ue_radio_capability_information_update_needed"
#define UE_RADIO_CAPABILITY_INFORMATION_UPDATE_NEEDED_XML_SCAN_FMT    "%"SCNx8
#define UE_RADIO_CAPABILITY_INFORMATION_UPDATE_NEEDED_XML_FMT         "%"PRIx8
NUM_FROM_XML_PROTOTYPE(ue_radio_capability_information_update_needed);
void ue_radio_capability_information_update_needed_to_xml (ue_radio_capability_information_update_needed_t * ueradiocapabilityinformationupdateneeded, xmlTextWriterPtr writer);

// 9.9.3.36 UE security capability
#define UE_SECURITY_CAPABILITY_IE_XML_STR "ue_security_capability"
#define UE_SECURITY_CAPABILITY_EPS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR   "eea"
#define UE_SECURITY_CAPABILITY_EPS_INTEGRITY_ALGORITHMS_SUPPORTED_ATTR_XML_STR    "eia"
#define UE_SECURITY_CAPABILITY_UMTS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR  "uea"
#define UE_SECURITY_CAPABILITY_UMTS_INTEGRITY_ALGORITHMS_SUPPORTED_ATTR_XML_STR   "uia"
#define UE_SECURITY_CAPABILITY_GPRS_ENCRYPTION_ALGORITHMS_SUPPORTED_ATTR_XML_STR  "gea"
bool ue_security_capability_from_xml (
    xmlDocPtr                              xml_doc,
    xmlXPathContextPtr                     xpath_ctx,
    ue_security_capability_t           * const uesecuritycapability);

void ue_security_capability_to_xml (ue_security_capability_t * uesecuritycapability, xmlTextWriterPtr writer);

// 9.9.3.37 Emergency Number List
// See subclause 10.5.3.13 in 3GPP TS 24.008 [13].

// 9.9.3.38 CLI
#define CLI_IE_XML_STR "cli"
// TODO cli_from_xml
void cli_to_xml (Cli cli, xmlTextWriterPtr writer);

// 9.9.3.39 SS Code
#define SS_CODE_IE_XML_STR      "ss_code"
#define SS_CODE_XML_SCAN_FMT    "%"SCNx8
#define SS_CODE_XML_FMT         "%"PRIx8
NUM_FROM_XML_PROTOTYPE(ss_code);
void ss_code_to_xml (ss_code_t * sscode, xmlTextWriterPtr writer);

// 9.9.3.40 LCS indicator
#define LCS_INDICATOR_IE_XML_STR      "lcs_indicator"
#define LCS_INDICATOR_XML_SCAN_FMT    "%"SCNx8
#define LCS_INDICATOR_XML_FMT         "%"PRIx8
NUM_FROM_XML_PROTOTYPE(lcs_indicator);
void lcs_indicator_to_xml (lcs_indicator_t * lcsindicator, xmlTextWriterPtr writer);

// 9.9.3.41 LCS client identity
#define LCS_CLIENT_IDENTITY_IE_XML_STR "lcs_client_identity"
// TODO lcs_client_identity_from_xml
void lcs_client_identity_to_xml (LcsClientIdentity  lcsclientidentity, xmlTextWriterPtr writer);

// 9.9.3.42 Generic message container type
// TODO ?
// 9.9.3.43 Generic message container
// TODO ?
// 9.9.3.44 Voice domain preference and UE's usage setting
// See subclause 10.5.5.28 in 3GPP TS 24.008 [13].

// 9.9.3.45 GUTI type
#define GUTI_TYPE_IE_XML_STR                        "guti_type"
bool guti_type_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, guti_type_t * const gutitype);
void guti_type_to_xml (guti_type_t * gutitype, xmlTextWriterPtr writer);


#endif /* FILE_3GPP_24_301_EMM_IES_XML_SEEN */

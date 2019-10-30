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


/** @defgroup _m2ap_impl_ M2AP Layer Reference Implementation
 * @ingroup _ref_implementation_
 * @{
 */

#if HAVE_CONFIG_H_
# include "config.h"
#endif

#ifndef FILE_M2AP_COMMON_SEEN
#define FILE_M2AP_COMMON_SEEN

#include "bstrlib.h"

/* Defined in asn_internal.h */
// extern int asn_debug_indent;
extern int asn_debug;

#if defined(EMIT_ASN_DEBUG_EXTERN)
inline void ASN_DEBUG(const char *fmt, ...);
#endif

#include "M2AP_ProtocolIE-Field.h"
#include "M2AP_M2AP-PDU.h"
#include "M2AP_InitiatingMessage.h"
#include "M2AP_SuccessfulOutcome.h"
#include "M2AP_UnsuccessfulOutcome.h"
#include "M2AP_ProtocolIE-Field.h"
#include "M2AP_ProtocolIE-FieldPair.h"
#include "M2AP_ProtocolIE-ContainerPair.h"
#include "M2AP_ProtocolExtensionField.h"
#include "M2AP_ProtocolExtensionContainer.h"
#include "M2AP_asn_constant.h"

#include "M2AP_Active-MBMS-Session-List.h"
#include "M2AP_AllocatedSubframesEnd.h"
#include "M2AP_AllocationAndRetentionPriority.h"
#include "M2AP_BitRate.h"
#include "M2AP_Cause.h"
#include "M2AP_CauseMisc.h"
#include "M2AP_CauseNAS.h"
#include "M2AP_CauseProtocol.h"
#include "M2AP_CauseRadioNetwork.h"
#include "M2AP_CauseTransport.h"
#include "M2AP_Cell-Information.h"
#include "M2AP_Cell-Information-List.h"
#include "M2AP_Common-Subframe-Allocation-Period.h"
#include "M2AP_CountingResult.h"
#include "M2AP_CriticalityDiagnostics.h"
#include "M2AP_CriticalityDiagnostics-IE-List.h"
#include "M2AP_Criticality.h" // todo: here correct?
#include "M2AP_ECGI.h"
#include "M2AP_ENBConfigurationUpdateAcknowledge.h"
#include "M2AP_ENBConfigurationUpdateFailure.h"
#include "M2AP_ENBConfigurationUpdate.h"
#include "M2AP_ENB-ID.h"
#include "M2AP_ENB-MBMS-Configuration-data-ConfigUpdate-Item.h"
#include "M2AP_ENB-MBMS-Configuration-data-Item.h"
#include "M2AP_ENB-MBMS-Configuration-data-List-ConfigUpdate.h"
#include "M2AP_ENB-MBMS-Configuration-data-List.h"
#include "M2AP_ENB-MBMS-M2AP-ID.h"
#include "M2AP_ENBname.h"
#include "M2AP_ErrorIndication.h"
#include "M2AP_EUTRANCellIdentifier.h"
#include "M2AP_GBR-QosInformation.h"
#include "M2AP_GlobalENB-ID.h"
#include "M2AP_GlobalMCE-ID.h"
#include "M2AP_GTP-TEID.h"
#include "M2AP_IPAddress.h"
#include "M2AP_LCID.h"
#include "M2AP_M2SetupFailure.h"
#include "M2AP_M2SetupRequest.h"
#include "M2AP_M2SetupResponse.h"
#include "M2AP_MBMS-Cell-List.h"
#include "M2AP_MBMS-Counting-Request-Session.h"
#include "M2AP_MBMS-Counting-Request-SessionIE.h"
#include "M2AP_MBMS-Counting-Result.h"
#include "M2AP_MBMS-Counting-Result-List.h"
#include "M2AP_MBMS-E-RAB-QoS-Parameters.h"
#include "M2AP_MbmsOverloadNotification.h"
#include "M2AP_MbmsSchedulingInformation.h"
#include "M2AP_MbmsSchedulingInformationResponse.h"
#include "M2AP_MBMS-Service-Area.h"
#include "M2AP_MBMS-Service-Area-ID-List.h"
#include "M2AP_MBMS-Service-associatedLogicalM2-ConnectionItem.h"
#include "M2AP_MBMS-Service-associatedLogicalM2-ConnectionListResAck.h"
#include "M2AP_MBMS-Service-associatedLogicalM2-ConnectionListRes.h"
#include "M2AP_MbmsServiceCountingFailure.h"
#include "M2AP_MbmsServiceCountingRequest.h"
#include "M2AP_MbmsServiceCountingResponse.h"
#include "M2AP_MbmsServiceCountingResultsReport.h"
#include "M2AP_MBMS-Session-ID.h"
#include "M2AP_MBMSsessionListPerPMCH-Item.h"
#include "M2AP_MBMSsessionsToBeSuspendedListPerPMCH-Item.h"
#include "M2AP_MBMS-Suspension-Notification-Item.h"
#include "M2AP_MBMS-Suspension-Notification-List.h"
#include "M2AP_MBSFN-Area-Configuration-List.h"
#include "M2AP_MBSFN-Area-ID.h"
#include "M2AP_MBSFN-Subframe-Configuration.h"
#include "M2AP_MBSFN-Subframe-ConfigurationList.h"
#include "M2AP_MBSFN-SynchronisationArea-ID.h"
#include "M2AP_MCCHrelatedBCCH-ConfigPerMBSFNArea.h"
#include "M2AP_MCCHrelatedBCCH-ConfigPerMBSFNArea-Item.h"
#include "M2AP_MCCH-Update-Time.h"
#include "M2AP_MCEConfigurationUpdateAcknowledge.h"
#include "M2AP_MCEConfigurationUpdateFailure.h"
#include "M2AP_MCEConfigurationUpdate.h"
#include "M2AP_MCE-ID.h"
#include "M2AP_MCE-MBMS-M2AP-ID.h"
#include "M2AP_MCEname.h"
#include "M2AP_MCH-Scheduling-PeriodExtended2.h"
#include "M2AP_MCH-Scheduling-PeriodExtended.h"
#include "M2AP_MCH-Scheduling-Period.h"
#include "M2AP_Modification-PeriodExtended.h"
#include "M2AP_Modulation-Coding-Scheme2.h"
#include "M2AP_Overload-Status-Per-PMCH-List.h"
#include "M2AP_PLMN-Identity.h"
#include "M2AP_PMCH-Configuration.h"
#include "M2AP_PMCH-Configuration-Item.h"
#include "M2AP_PMCH-Configuration-List.h"
#include "M2AP_PMCH-Overload-Status.h"
#include "M2AP_Pre-emptionCapability.h"
#include "M2AP_Pre-emptionVulnerability.h"
#include "M2AP_Presence.h" // todo: here corect?
#include "M2AP_PriorityLevel.h"
#include "M2AP_PrivateIE-Container.h"
#include "M2AP_PrivateIE-ID.h"
#include "M2AP_PrivateMessage.h"
// todo: uncessary?
//#include "M2AP_ProcedureCode.h"
//#include "M2AP_ProtocolExtensionContainer.h"
//#include "M2AP_ProtocolExtensionField.h"
//#include "M2AP_ProtocolIE-Container.h"
//#include "M2AP_ProtocolIE-ContainerList.h"
//#include "M2AP_ProtocolIE-ContainerPairList.h"
//#include "M2AP_ProtocolIE-Field.h"
//#include "M2AP_ProtocolIE-FieldPair.h"
//#include "M2AP_ProtocolIE-ID.h"
//#include "M2AP_ProtocolIE-Single-Container.h"
#include "M2AP_QCI.h"
#include "M2AP_Repetition-PeriodExtended.h"
#include "M2AP_ResetAcknowledge.h"
#include "M2AP_ResetAll.h"
#include "M2AP_Reset.h"
#include "M2AP_ResetType.h"
#include "M2AP_SC-PTM-Information.h"
#include "M2AP_SessionStartFailure.h"
#include "M2AP_SessionStartRequest.h"
#include "M2AP_SessionStartResponse.h"
#include "M2AP_SessionStopRequest.h"
#include "M2AP_SessionStopResponse.h"
#include "M2AP_SessionUpdateFailure.h"
#include "M2AP_SessionUpdateRequest.h"
#include "M2AP_SessionUpdateResponse.h"
#include "M2AP_SFN.h"
#include "M2AP_Subcarrier-SpacingMBMS.h"
#include "M2AP_SubframeAllocationExtended.h"
#include "M2AP_SuccessfulOutcome.h"
#include "M2AP_TimeToWait.h"
#include "M2AP_TMGI.h"
#include "M2AP_TNL-Information.h"
#include "M2AP_TriggeringMessage.h"
#include "M2AP_TypeOfError.h"

/* Checking version of ASN1C compiler */
#if (ASN1C_ENVIRONMENT_VERSION < ASN1C_MINIMUM_VERSION)
# error "You are compiling M2AP with the wrong version of ASN1C"
#endif


extern int asn_debug;
extern int asn1_xer_print;

# include <stdbool.h>
# include "mme_default_values.h"
# include "3gpp_23.003.h"
# include "3gpp_24.008.h"
# include "3gpp_33.401.h"
# include "3gpp_36.443.h"
# include "security_types.h"
# include "common_types.h"

#define M2AP_FIND_PROTOCOLIE_BY_ID(IE_TYPE, ie, container, IE_ID, mandatory) \
  do {\
    IE_TYPE **ptr; \
    ie = NULL; \
    for (ptr = container->protocolIEs.list.array; \
         ptr < &container->protocolIEs.list.array[container->protocolIEs.list.count]; \
         ptr++) { \
      if((*ptr)->id == IE_ID) { \
        ie = *ptr; \
        break; \
      } \
    } \
    if (ie == NULL ) { \
      if (mandatory)  OAILOG_ERROR (LOG_M2AP, "M2AP_FIND_PROTOCOLIE_BY_ID: %s %d: Mandatory ie is NULL\n",__FILE__,__LINE__);\
      else OAILOG_DEBUG (LOG_M2AP, "M2AP_FIND_PROTOCOLIE_BY_ID: %s %d: Optional ie is NULL\n",__FILE__,__LINE__);\
    } \
    if (mandatory)  DevAssert(ie != NULL); \
  } while(0)
/** \brief Function callback prototype.
 **/
typedef int (*m2ap_message_decoded_callback)(
    const sctp_assoc_id_t         assoc_id,
    const sctp_stream_id_t        stream,
    M2AP_M2AP_PDU_t *pdu
);

/** \brief Handle criticality
 \param criticality Criticality of the IE
 @returns void
 **/
void m2ap_handle_criticality(M2AP_Criticality_t criticality);

#endif /* FILE_M2AP_COMMON_SEEN */

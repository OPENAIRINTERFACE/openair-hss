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

/** @defgroup _s1ap_impl_ S1AP Layer Reference Implementation
 * @ingroup _ref_implementation_
 * @{
 */

#if HAVE_CONFIG_H_
#include "config.h"
#endif

#ifndef FILE_S1AP_COMMON_SEEN
#define FILE_S1AP_COMMON_SEEN

#include "bstrlib.h"

/* Defined in asn_internal.h */
// extern int asn_debug_indent;
extern int asn_debug;

#if defined(EMIT_ASN_DEBUG_EXTERN)
inline void ASN_DEBUG(const char* fmt, ...);
#endif

#include "S1AP_InitiatingMessage.h"
#include "S1AP_ProtocolExtensionContainer.h"
#include "S1AP_ProtocolExtensionField.h"
#include "S1AP_ProtocolIE-ContainerPair.h"
#include "S1AP_ProtocolIE-Field.h"
#include "S1AP_ProtocolIE-FieldPair.h"
#include "S1AP_S1AP-PDU.h"
#include "S1AP_SuccessfulOutcome.h"
#include "S1AP_UnsuccessfulOutcome.h"
#include "S1AP_asn_constant.h"

#include "S1AP_AllocationAndRetentionPriority.h"
#include "S1AP_BPLMNs.h"
#include "S1AP_Bearers-SubjectToStatusTransfer-Item.h"
#include "S1AP_Bearers-SubjectToStatusTransferList.h"
#include "S1AP_BitRate.h"
#include "S1AP_BroadcastCompletedAreaList.h"
#include "S1AP_CGI.h"
#include "S1AP_CI.h"
#include "S1AP_CNDomain.h"
#include "S1AP_COUNTvalue.h"
#include "S1AP_CSFallbackIndicator.h"
#include "S1AP_CSG-Id.h"
#include "S1AP_CSG-IdList-Item.h"
#include "S1AP_CSG-IdList.h"
#include "S1AP_Cause.h"
#include "S1AP_CauseMisc.h"
#include "S1AP_CauseNas.h"
#include "S1AP_CauseProtocol.h"
#include "S1AP_CauseRadioNetwork.h"
#include "S1AP_CauseTransport.h"
#include "S1AP_Cdma2000HORequiredIndication.h"
#include "S1AP_Cdma2000HOStatus.h"
#include "S1AP_Cdma2000OneXMEID.h"
#include "S1AP_Cdma2000OneXMSI.h"
#include "S1AP_Cdma2000OneXPilot.h"
#include "S1AP_Cdma2000OneXRAND.h"
#include "S1AP_Cdma2000OneXSRVCCInfo.h"
#include "S1AP_Cdma2000PDU.h"
#include "S1AP_Cdma2000RATType.h"
#include "S1AP_Cdma2000SectorID.h"
#include "S1AP_Cell-Size.h"
#include "S1AP_CellID-Broadcast-Item.h"
#include "S1AP_CellID-Broadcast.h"
#include "S1AP_CellIdentity.h"
#include "S1AP_CellTrafficTrace.h"
#include "S1AP_CellType.h"
#include "S1AP_CompletedCellinEAI-Item.h"
#include "S1AP_CompletedCellinEAI.h"
#include "S1AP_CompletedCellinTAI-Item.h"
#include "S1AP_CompletedCellinTAI.h"
#include "S1AP_CriticalityDiagnostics-IE-Item.h"
#include "S1AP_CriticalityDiagnostics-IE-List.h"
#include "S1AP_CriticalityDiagnostics.h"
#include "S1AP_DL-Forwarding.h"
#include "S1AP_DataCodingScheme.h"
#include "S1AP_DeactivateTrace.h"
#include "S1AP_Direct-Forwarding-Path-Availability.h"
#include "S1AP_DownlinkNASTransport.h"
#include "S1AP_DownlinkS1cdma2000tunnelling.h"
#include "S1AP_E-RAB-ID.h"
#include "S1AP_E-RABAdmittedItem.h"
#include "S1AP_E-RABAdmittedList.h"
#include "S1AP_E-RABDataForwardingItem.h"
#include "S1AP_E-RABFailedToSetupItemHOReqAck.h"
#include "S1AP_E-RABFailedtoSetupListHOReqAck.h"
#include "S1AP_E-RABInformationList.h"
#include "S1AP_E-RABInformationListItem.h"
#include "S1AP_E-RABItem.h"
#include "S1AP_E-RABLevelQoSParameters.h"
#include "S1AP_E-RABList.h"
#include "S1AP_E-RABModifyItemBearerModRes.h"
#include "S1AP_E-RABModifyListBearerModRes.h"
#include "S1AP_E-RABModifyRequest.h"
#include "S1AP_E-RABModifyResponse.h"
#include "S1AP_E-RABReleaseCommand.h"
#include "S1AP_E-RABReleaseIndication.h"
#include "S1AP_E-RABReleaseItemBearerRelComp.h"
#include "S1AP_E-RABReleaseListBearerRelComp.h"
#include "S1AP_E-RABReleaseResponse.h"
#include "S1AP_E-RABSetupItemBearerSURes.h"
#include "S1AP_E-RABSetupItemCtxtSURes.h"
#include "S1AP_E-RABSetupListBearerSURes.h"
#include "S1AP_E-RABSetupListCtxtSURes.h"
#include "S1AP_E-RABSetupRequest.h"
#include "S1AP_E-RABSetupResponse.h"
#include "S1AP_E-RABSubjecttoDataForwardingList.h"
#include "S1AP_E-RABToBeModifiedItemBearerModReq.h"
#include "S1AP_E-RABToBeModifiedListBearerModReq.h"
#include "S1AP_E-RABToBeSetupItemBearerSUReq.h"
#include "S1AP_E-RABToBeSetupItemCtxtSUReq.h"
#include "S1AP_E-RABToBeSetupItemHOReq.h"
#include "S1AP_E-RABToBeSetupListBearerSUReq.h"
#include "S1AP_E-RABToBeSetupListCtxtSUReq.h"
#include "S1AP_E-RABToBeSetupListHOReq.h"
#include "S1AP_E-RABToBeSwitchedDLItem.h"
#include "S1AP_E-RABToBeSwitchedDLList.h"
#include "S1AP_E-RABToBeSwitchedULItem.h"
#include "S1AP_E-RABToBeSwitchedULList.h"
#include "S1AP_E-UTRAN-Trace-ID.h"
#include "S1AP_ECGIList.h"
#include "S1AP_ENB-ID.h"
#include "S1AP_ENB-StatusTransfer-TransparentContainer.h"
#include "S1AP_ENB-UE-S1AP-ID.h"
#include "S1AP_ENBConfigurationTransfer.h"
#include "S1AP_ENBConfigurationUpdate.h"
#include "S1AP_ENBConfigurationUpdateAcknowledge.h"
#include "S1AP_ENBConfigurationUpdateFailure.h"
#include "S1AP_ENBDirectInformationTransfer.h"
#include "S1AP_ENBStatusTransfer.h"
#include "S1AP_ENBX2TLAs.h"
#include "S1AP_ENBname.h"
#include "S1AP_EPLMNs.h"
#include "S1AP_EUTRAN-CGI.h"
#include "S1AP_EmergencyAreaID-Broadcast-Item.h"
#include "S1AP_EmergencyAreaID-Broadcast.h"
#include "S1AP_EmergencyAreaID.h"
#include "S1AP_EmergencyAreaIDList.h"
#include "S1AP_EncryptionAlgorithms.h"
#include "S1AP_ErrorIndication.h"
#include "S1AP_EventType.h"
#include "S1AP_ExtendedRNC-ID.h"
#include "S1AP_ForbiddenInterRATs.h"
#include "S1AP_ForbiddenLACs.h"
#include "S1AP_ForbiddenLAs-Item.h"
#include "S1AP_ForbiddenLAs.h"
#include "S1AP_ForbiddenTACs.h"
#include "S1AP_ForbiddenTAs-Item.h"
#include "S1AP_ForbiddenTAs.h"
#include "S1AP_GBR-QosInformation.h"
#include "S1AP_GERAN-Cell-ID.h"
#include "S1AP_GTP-TEID.h"
#include "S1AP_GUMMEI.h"
#include "S1AP_GUMMEIType.h"
#include "S1AP_Global-ENB-ID.h"
#include "S1AP_HFN.h"
#include "S1AP_HandoverCancel.h"
#include "S1AP_HandoverCancelAcknowledge.h"
#include "S1AP_HandoverCommand.h"
#include "S1AP_HandoverFailure.h"
#include "S1AP_HandoverNotify.h"
#include "S1AP_HandoverPreparationFailure.h"
#include "S1AP_HandoverRequest.h"
#include "S1AP_HandoverRequestAcknowledge.h"
#include "S1AP_HandoverRequired.h"
#include "S1AP_HandoverRestrictionList.h"
#include "S1AP_HandoverType.h"
#include "S1AP_IMSI.h"
#include "S1AP_InitialContextSetupFailure.h"
#include "S1AP_InitialContextSetupRequest.h"
#include "S1AP_InitialContextSetupResponse.h"
#include "S1AP_InitialUEMessage.h"
#include "S1AP_InitiatingMessage.h"
#include "S1AP_IntegrityProtectionAlgorithms.h"
#include "S1AP_Inter-SystemInformationTransferType.h"
#include "S1AP_InterfacesToTrace.h"
#include "S1AP_L3-Information.h"
#include "S1AP_LAC.h"
#include "S1AP_LAI.h"
#include "S1AP_LastVisitedCell-Item.h"
#include "S1AP_LastVisitedEUTRANCellInformation.h"
#include "S1AP_LastVisitedGERANCellInformation.h"
#include "S1AP_LastVisitedUTRANCellInformation.h"
#include "S1AP_LocationReport.h"
#include "S1AP_LocationReportingControl.h"
#include "S1AP_LocationReportingFailureIndication.h"
#include "S1AP_M-TMSI.h"
#include "S1AP_MME-Code.h"
#include "S1AP_MME-Group-ID.h"
#include "S1AP_MME-UE-S1AP-ID.h"
#include "S1AP_MMEConfigurationTransfer.h"
#include "S1AP_MMEConfigurationUpdate.h"
#include "S1AP_MMEConfigurationUpdateAcknowledge.h"
#include "S1AP_MMEConfigurationUpdateFailure.h"
#include "S1AP_MMEDirectInformationTransfer.h"
#include "S1AP_MMEStatusTransfer.h"
#include "S1AP_MMEname.h"
#include "S1AP_MSClassmark2.h"
#include "S1AP_MSClassmark3.h"
#include "S1AP_MessageIdentifier.h"
#include "S1AP_NAS-PDU.h"
#include "S1AP_NASNonDeliveryIndication.h"
#include "S1AP_NASSecurityParametersfromE-UTRAN.h"
#include "S1AP_NASSecurityParameterstoE-UTRAN.h"
#include "S1AP_NumberOfBroadcasts.h"
#include "S1AP_NumberofBroadcastRequest.h"
#include "S1AP_OldBSS-ToNewBSS-Information.h"
#include "S1AP_OverloadAction.h"
#include "S1AP_OverloadResponse.h"
#include "S1AP_OverloadStart.h"
#include "S1AP_OverloadStop.h"
#include "S1AP_PDCP-SN.h"
#include "S1AP_PLMNidentity.h"
#include "S1AP_Paging.h"
#include "S1AP_PagingDRX.h"
#include "S1AP_PathSwitchRequest.h"
#include "S1AP_PathSwitchRequestAcknowledge.h"
#include "S1AP_PathSwitchRequestFailure.h"
#include "S1AP_Pre-emptionCapability.h"
#include "S1AP_Pre-emptionVulnerability.h"
#include "S1AP_PriorityLevel.h"
#include "S1AP_PrivateMessage.h"
#include "S1AP_QCI.h"
#include "S1AP_RAC.h"
#include "S1AP_RIMInformation.h"
#include "S1AP_RIMRoutingAddress.h"
#include "S1AP_RIMTransfer.h"
#include "S1AP_RNC-ID.h"
#include "S1AP_RRC-Container.h"
#include "S1AP_RRC-Establishment-Cause.h"
#include "S1AP_ReceiveStatusofULPDCPSDUs.h"
#include "S1AP_RelativeMMECapacity.h"
#include "S1AP_RepetitionPeriod.h"
#include "S1AP_ReportArea.h"
#include "S1AP_RequestType.h"
#include "S1AP_Reset.h"
#include "S1AP_ResetAcknowledge.h"
#include "S1AP_ResetType.h"
#include "S1AP_S-TMSI.h"
#include "S1AP_S1SetupFailure.h"
#include "S1AP_S1SetupRequest.h"
#include "S1AP_S1SetupResponse.h"
#include "S1AP_SONConfigurationTransfer.h"
#include "S1AP_SONInformation.h"
#include "S1AP_SONInformationReply.h"
#include "S1AP_SONInformationRequest.h"
#include "S1AP_SRVCCHOIndication.h"
#include "S1AP_SRVCCOperationPossible.h"
#include "S1AP_SecurityContext.h"
#include "S1AP_SecurityKey.h"
#include "S1AP_SerialNumber.h"
#include "S1AP_ServedGUMMEIs.h"
#include "S1AP_ServedGUMMEIsItem.h"
#include "S1AP_ServedGroupIDs.h"
#include "S1AP_ServedMMECs.h"
#include "S1AP_ServedPLMNs.h"
#include "S1AP_Source-ToTarget-TransparentContainer.h"
#include "S1AP_SourceBSS-ToTargetBSS-TransparentContainer.h"
#include "S1AP_SourceRNC-ToTargetRNC-TransparentContainer.h"
#include "S1AP_SourceeNB-ID.h"
#include "S1AP_SourceeNB-ToTargeteNB-TransparentContainer.h"
#include "S1AP_SubscriberProfileIDforRFP.h"
#include "S1AP_SuccessfulOutcome.h"
#include "S1AP_SupportedTAs-Item.h"
#include "S1AP_SupportedTAs.h"
#include "S1AP_TAC.h"
#include "S1AP_TAI-Broadcast-Item.h"
#include "S1AP_TAI-Broadcast.h"
#include "S1AP_TAI.h"
#include "S1AP_TAIItem.h"
#include "S1AP_TAIList.h"
#include "S1AP_TAIListforWarning.h"
#include "S1AP_TBCD-STRING.h"
#include "S1AP_Target-ToSource-TransparentContainer.h"
#include "S1AP_TargetBSS-ToSourceBSS-TransparentContainer.h"
#include "S1AP_TargetID.h"
#include "S1AP_TargetRNC-ID.h"
#include "S1AP_TargetRNC-ToSourceRNC-TransparentContainer.h"
#include "S1AP_TargeteNB-ID.h"
#include "S1AP_TargeteNB-ToSourceeNB-TransparentContainer.h"
#include "S1AP_Time-UE-StayedInCell.h"
#include "S1AP_TimeToWait.h"
#include "S1AP_TraceActivation.h"
#include "S1AP_TraceDepth.h"
#include "S1AP_TraceFailureIndication.h"
#include "S1AP_TraceStart.h"
#include "S1AP_TransportLayerAddress.h"
#include "S1AP_TypeOfError.h"
#include "S1AP_UE-HistoryInformation.h"
#include "S1AP_UE-S1AP-ID-pair.h"
#include "S1AP_UE-S1AP-IDs.h"
#include "S1AP_UE-associatedLogicalS1-ConnectionItem.h"
#include "S1AP_UE-associatedLogicalS1-ConnectionListResAck.h"
#include "S1AP_UEAggregateMaximumBitrate.h"
#include "S1AP_UECapabilityInfoIndication.h"
#include "S1AP_UEContextModificationFailure.h"
#include "S1AP_UEContextModificationRequest.h"
#include "S1AP_UEContextModificationResponse.h"
#include "S1AP_UEContextReleaseCommand.h"
#include "S1AP_UEContextReleaseComplete.h"
#include "S1AP_UEContextReleaseRequest.h"
#include "S1AP_UEIdentityIndexValue.h"
#include "S1AP_UEPagingID.h"
#include "S1AP_UERadioCapability.h"
#include "S1AP_UESecurityCapabilities.h"
#include "S1AP_UnsuccessfulOutcome.h"
#include "S1AP_UplinkNASTransport.h"
#include "S1AP_UplinkS1cdma2000tunnelling.h"
#include "S1AP_WarningAreaList.h"
#include "S1AP_WarningMessageContents.h"
#include "S1AP_WarningSecurityInfo.h"
#include "S1AP_WarningType.h"
#include "S1AP_WriteReplaceWarningRequest.h"
#include "S1AP_WriteReplaceWarningResponse.h"
#include "S1AP_X2TNLConfigurationInfo.h"

// UPDATE RELEASE 9
#include "S1AP_BroadcastCancelledAreaList.h"
#include "S1AP_CSGMembershipStatus.h"
#include "S1AP_CancelledCellinEAI-Item.h"
#include "S1AP_CancelledCellinEAI.h"
#include "S1AP_CancelledCellinTAI-Item.h"
#include "S1AP_CancelledCellinTAI.h"
#include "S1AP_CellAccessMode.h"
#include "S1AP_CellID-Cancelled-Item.h"
#include "S1AP_CellID-Cancelled.h"
#include "S1AP_ConcurrentWarningMessageIndicator.h"
#include "S1AP_Data-Forwarding-Not-Possible.h"
#include "S1AP_DownlinkNonUEAssociatedLPPaTransport.h"
#include "S1AP_DownlinkUEAssociatedLPPaTransport.h"
#include "S1AP_E-RABList.h"
#include "S1AP_EUTRANRoundTripDelayEstimationInfo.h"
#include "S1AP_EmergencyAreaID-Cancelled-Item.h"
#include "S1AP_EmergencyAreaID-Cancelled.h"
#include "S1AP_ExtendedRepetitionPeriod.h"
#include "S1AP_KillRequest.h"
#include "S1AP_KillResponse.h"
#include "S1AP_LPPa-PDU.h"
#include "S1AP_PS-ServiceNotAvailable.h"
#include "S1AP_Routing-ID.h"
#include "S1AP_StratumLevel.h"
#include "S1AP_SynchronisationStatus.h"
#include "S1AP_TAI-Cancelled-Item.h"
#include "S1AP_TAI-Cancelled.h"
#include "S1AP_TimeSynchronisationInfo.h"
#include "S1AP_UplinkNonUEAssociatedLPPaTransport.h"
#include "S1AP_UplinkUEAssociatedLPPaTransport.h"

// UPDATE RELEASE 10
#include "S1AP_GUMMEIList.h"
#include "S1AP_GWContextReleaseIndication.h"
#include "S1AP_MMERelaySupportIndicator.h"
#include "S1AP_ManagementBasedMDTAllowed.h"
#include "S1AP_PagingPriority.h"
#include "S1AP_PrivacyIndicator.h"
#include "S1AP_RelayNode-Indicator.h"
#include "S1AP_TrafficLoadReductionIndication.h"

/* Checking version of ASN1C compiler */
#if (ASN1C_ENVIRONMENT_VERSION < ASN1C_MINIMUM_VERSION)
#error "You are compiling s1ap with the wrong version of ASN1C"
#endif

extern int asn_debug;
extern int asn1_xer_print;

#include <stdbool.h>
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "common_types.h"
#include "mme_default_values.h"
#include "security_types.h"

#define S1AP_FIND_PROTOCOLIE_BY_ID(IE_TYPE, ie, container, IE_ID, mandatory) \
  do {                                                                       \
    IE_TYPE** ptr;                                                           \
    ie = NULL;                                                               \
    for (ptr = container->protocolIEs.list.array;                            \
         ptr < &container->protocolIEs.list                                  \
                    .array[container->protocolIEs.list.count];               \
         ptr++) {                                                            \
      if ((*ptr)->id == IE_ID) {                                             \
        ie = *ptr;                                                           \
        break;                                                               \
      }                                                                      \
    }                                                                        \
    if (ie == NULL) {                                                        \
      if (mandatory)                                                         \
        OAILOG_ERROR(                                                        \
            LOG_S1AP,                                                        \
            "S1AP_FIND_PROTOCOLIE_BY_ID: %s %d: Mandatory ie is NULL\n",     \
            __FILE__, __LINE__);                                             \
      else                                                                   \
        OAILOG_DEBUG(                                                        \
            LOG_S1AP,                                                        \
            "S1AP_FIND_PROTOCOLIE_BY_ID: %s %d: Optional ie is NULL\n",      \
            __FILE__, __LINE__);                                             \
    }                                                                        \
    if (mandatory) DevAssert(ie != NULL);                                    \
  } while (0)
/** \brief Function callback prototype.
 **/
typedef int (*s1ap_message_decoded_callback)(const sctp_assoc_id_t assoc_id,
                                             const sctp_stream_id_t stream,
                                             S1AP_S1AP_PDU_t* pdu);

/** \brief Handle criticality
 \param criticality Criticality of the IE
 @returns void
 **/
void s1ap_handle_criticality(S1AP_Criticality_t criticality);

#endif /* FILE_S1AP_COMMON_SEEN */

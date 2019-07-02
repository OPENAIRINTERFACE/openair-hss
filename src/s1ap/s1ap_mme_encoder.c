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


/*! \file s1ap_mme_encoder.c
   \brief s1ap encode procedures for MME
   \author Sebastien ROUX <sebastien.roux@eurecom.fr>
   \date 2012
   \version 0.1
*/
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>

#include "bstrlib.h"

#include "intertask_interface.h"
#include "mme_api.h"
#include "s1ap_common.h"
#include "s1ap_ies_defs.h"
#include "s1ap_mme_encoder.h"
#include "assertions.h"
#include "log.h"

static inline int                       s1ap_mme_encode_initial_context_setup_request (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length);
static inline int                       s1ap_mme_encode_s1setupresponse (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length);
static inline int                       s1ap_mme_encode_s1setupfailure (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length);
static inline int                       s1ap_mme_encode_ue_context_release_command (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length);
static inline int                       s1ap_mme_encode_downlink_nas_transport (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length);
static inline int                       s1ap_mme_encode_e_rab_setup (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length);

static inline int                       s1ap_mme_encode_e_rab_modify (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length);

static inline int                       s1ap_mme_encode_e_rab_release (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length);

static inline int                       s1ap_mme_encode_paging(
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length);

static inline int                       s1ap_mme_encode_mme_configuration_transfer(
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length);

static inline int                       s1ap_mme_encode_initiating (
  s1ap_message * message_p,
  MessagesIds *message_id,
  uint8_t ** buffer,
  uint32_t * length);
static inline int                       s1ap_mme_encode_successfull_outcome (
  s1ap_message * message_p,
  MessagesIds *message_id,
  uint8_t ** buffer,
  uint32_t * len);
static inline int                       s1ap_mme_encode_unsuccessfull_outcome (
  s1ap_message * message_p,
  MessagesIds *message_id,
  uint8_t ** buffer,
  uint32_t * len);

/** Additions for ENB triggered RESET procedure. */
static inline int s1ap_mme_encode_resetack (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length);

/** Handover additions. */
static inline int s1ap_mme_encode_pathSwitchRequestAcknowledge (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length);

static inline int s1ap_mme_encode_pathSwitchRequestFailure (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length);

static inline int s1ap_mme_encode_handoverPreparationFailure (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length);

static inline int s1ap_mme_encode_handoverCommand (
    s1ap_message * message_p,
    uint8_t ** buffer,
    uint32_t * length);

static inline int s1ap_mme_encode_handoverCancelAck(
    s1ap_message * message_p,
    uint8_t ** buffer,
    uint32_t * length);

static inline int s1ap_mme_encode_handover_resource_allocation (
    s1ap_message * message_p,
    uint8_t ** buffer,
    uint32_t * length);

static inline int s1ap_mme_encode_mme_status_transfer (
    s1ap_message * message_p,
    uint8_t ** buffer,
    uint32_t * length);

//------------------------------------------------------------------------------
static inline int
s1ap_mme_encode_initial_context_setup_request (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{
  S1ap_InitialContextSetupRequest_t       initialContextSetupRequest;
  S1ap_InitialContextSetupRequest_t      *initialContextSetupRequest_p = &initialContextSetupRequest;

  memset (initialContextSetupRequest_p, 0, sizeof (S1ap_InitialContextSetupRequest_t));

  if (s1ap_encode_s1ap_initialcontextsetuprequesties (initialContextSetupRequest_p, &message_p->msg.s1ap_InitialContextSetupRequestIEs) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message (buffer, length, S1ap_ProcedureCode_id_InitialContextSetup, S1ap_Criticality_reject, &asn_DEF_S1ap_InitialContextSetupRequest, initialContextSetupRequest_p);
}

//------------------------------------------------------------------------------
int
s1ap_mme_encode_pdu (
  s1ap_message * message_p,
  MessagesIds *message_id,
  uint8_t ** buffer,
  uint32_t * length)
{

  DevAssert (message_p != NULL);
  DevAssert (buffer != NULL);
  DevAssert (length != NULL);


  switch (message_p->direction) {
  case S1AP_PDU_PR_initiatingMessage:
    return s1ap_mme_encode_initiating (message_p, message_id, buffer, length);

  case S1AP_PDU_PR_successfulOutcome:
    return s1ap_mme_encode_successfull_outcome (message_p, message_id, buffer, length);

  case S1AP_PDU_PR_unsuccessfulOutcome:
    return s1ap_mme_encode_unsuccessfull_outcome (message_p, message_id, buffer, length);

  default:
    OAILOG_NOTICE (LOG_S1AP, "Unknown message outcome (%d) or not implemented", (int)message_p->direction);
    break;
  }

  return -1;
}

//------------------------------------------------------------------------------
int s1ap_free_mme_encode_pdu(
    s1ap_message *message, MessagesIds message_id) {
  switch(message_id) {
  case S1AP_S1_SETUP_LOG:
     return free_s1ap_s1setupresponse(&message->msg.s1ap_S1SetupResponseIEs);
  case S1AP_DOWNLINK_NAS_LOG:
    return free_s1ap_uplinknastransport(&message->msg.s1ap_UplinkNASTransportIEs);
  case S1AP_INITIAL_CONTEXT_SETUP_LOG:
    return free_s1ap_initialcontextsetuprequest(&message->msg.s1ap_InitialContextSetupRequestIEs);
  case S1AP_UE_CONTEXT_RELEASE_LOG:
    return free_s1ap_uecontextreleasecommand(&message->msg.s1ap_UEContextReleaseCommandIEs);
  case S1AP_E_RABSETUP_LOG:
    return free_s1ap_e_rabsetuprequest(&message->msg.s1ap_E_RABSetupRequestIEs);
  case S1AP_E_RABMODIFY_LOG:
    return free_s1ap_e_rabmodifyrequest(&message->msg.s1ap_E_RABModifyRequestIEs);
  case S1AP_E_RABRELEASE_LOG:
    return free_s1ap_e_rabreleasecommand(&message->msg.s1ap_E_RABReleaseCommandIEs);

  /** Free Handover Messages. */
  case S1AP_HANDOVER_REQUEST_LOG:
    return free_s1ap_handoverrequest(&message->msg.s1ap_HandoverRequestIEs);
  case S1AP_MME_STATUS_TRANSFER_LOG:
	return free_s1ap_mmestatustransfer(&message->msg.s1ap_MMEStatusTransferIEs);
  case S1AP_PATH_SWITCH_ACK_LOG:
    return free_s1ap_pathswitchrequestacknowledge(&message->msg.s1ap_PathSwitchRequestAcknowledgeIEs);
  case S1AP_HANDOVER_COMMAND_LOG:
    return free_s1ap_handovercommand(&message->msg.s1ap_HandoverCommandIEs);
  case S1AP_HANDOVER_CANCEL_ACK_LOG:
    return free_s1ap_handovercancelacknowledge(&message->msg.s1ap_HandoverCancelAcknowledgeIEs);
  /** Failure Messages. */
  case S1AP_S1_SETUP_FAILURE_LOG:
    return free_s1ap_s1setupfailure(&message->msg.s1ap_S1SetupFailureIEs);
  case S1AP_HANDOVER_FAILURE_LOG:
    return free_s1ap_handoverpreparationfailure(&message->msg.s1ap_HandoverPreparationFailureIEs);
  case S1AP_PATH_SWITCH_FAILURE_LOG:
    return free_s1ap_pathswitchrequestfailure(&message->msg.s1ap_PathSwitchRequestFailureIEs);

  /** Free Other Messages. */
  case S1AP_PAGING_LOG:
      return free_s1ap_paging(&message->msg.s1ap_PagingIEs);

  case S1AP_MME_CFG_TRANSFER_LOG:
        return free_s1ap_mmeconfigurationtransfer(&message->msg.s1ap_MMEConfigurationTransferIEs);

  default:
    DevAssert(false);

  }
}

//------------------------------------------------------------------------------
static inline int
s1ap_mme_encode_initiating (
  s1ap_message * message_p,
  MessagesIds *message_id,
  uint8_t ** buffer,
  uint32_t * length)
{
  switch (message_p->procedureCode) {
  case S1ap_ProcedureCode_id_downlinkNASTransport:
    *message_id = S1AP_DOWNLINK_NAS_LOG;
    return s1ap_mme_encode_downlink_nas_transport (message_p, buffer, length);

  case S1ap_ProcedureCode_id_InitialContextSetup:
    *message_id = S1AP_INITIAL_CONTEXT_SETUP_LOG;
    return s1ap_mme_encode_initial_context_setup_request (message_p, buffer, length);

  case S1ap_ProcedureCode_id_UEContextRelease:
    *message_id = S1AP_UE_CONTEXT_RELEASE_LOG;
    return s1ap_mme_encode_ue_context_release_command (message_p, buffer, length);

  case S1ap_ProcedureCode_id_E_RABSetup:
    *message_id = S1AP_E_RABSETUP_LOG;
    return s1ap_mme_encode_e_rab_setup (message_p, buffer, length);

  case S1ap_ProcedureCode_id_E_RABModify:
    *message_id = S1AP_E_RABMODIFY_LOG;
    return s1ap_mme_encode_e_rab_modify (message_p, buffer, length);

  case S1ap_ProcedureCode_id_E_RABRelease:
    *message_id = S1AP_E_RABRELEASE_LOG;
    return s1ap_mme_encode_e_rab_release (message_p, buffer, length);

  case S1ap_ProcedureCode_id_HandoverResourceAllocation:
    *message_id = S1AP_HANDOVER_REQUEST_LOG;
    return s1ap_mme_encode_handover_resource_allocation (message_p, buffer, length);

  case S1ap_ProcedureCode_id_MMEStatusTransfer:
    *message_id = S1AP_MME_STATUS_TRANSFER_LOG;
    return s1ap_mme_encode_mme_status_transfer (message_p, buffer, length);

  case S1ap_ProcedureCode_id_Paging:
    *message_id = S1AP_PAGING_LOG;
    return s1ap_mme_encode_paging(message_p, buffer, length);

  case S1ap_ProcedureCode_id_MMEConfigurationTransfer:
    *message_id = S1AP_MME_CFG_TRANSFER_LOG;
    return s1ap_mme_encode_mme_configuration_transfer (message_p, buffer, length);

  default:
    OAILOG_NOTICE (LOG_S1AP, "Unknown procedure ID (%d) for initiating message_p\n", (int)message_p->procedureCode);
    break;
  }

  return -1;
}

//------------------------------------------------------------------------------
static inline int
s1ap_mme_encode_successfull_outcome (
  s1ap_message * message_p,
  MessagesIds *message_id,
  uint8_t ** buffer,
  uint32_t * length)
{
  switch (message_p->procedureCode) {
  case S1ap_ProcedureCode_id_S1Setup:
    *message_id = S1AP_S1_SETUP_LOG;
    return s1ap_mme_encode_s1setupresponse (message_p, buffer, length);

  // Add handover related messages
  case S1ap_ProcedureCode_id_PathSwitchRequest:
    *message_id = S1AP_PATH_SWITCH_ACK_LOG;
    return s1ap_mme_encode_pathSwitchRequestAcknowledge(message_p, buffer, length);

  case S1ap_ProcedureCode_id_HandoverPreparation:
    *message_id = S1AP_HANDOVER_COMMAND_LOG;
    return s1ap_mme_encode_handoverCommand(message_p, buffer, length);

  case S1ap_ProcedureCode_id_HandoverCancel:
    *message_id = S1AP_HANDOVER_CANCEL_ACK_LOG;
    return s1ap_mme_encode_handoverCancelAck(message_p, buffer, length);

  default:
    OAILOG_DEBUG (LOG_S1AP, "Unknown procedure ID (%d) for successfull outcome message\n", (int)message_p->procedureCode);
    break;
  }

  return -1;
}

//------------------------------------------------------------------------------
static inline int
s1ap_mme_encode_unsuccessfull_outcome (
  s1ap_message * message_p,
  MessagesIds *message_id,
  uint8_t ** buffer,
  uint32_t * length)
{
  switch (message_p->procedureCode) {
  case S1ap_ProcedureCode_id_S1Setup:
    *message_id = S1AP_S1_SETUP_FAILURE_LOG;
    return s1ap_mme_encode_s1setupfailure (message_p, buffer, length);
  case S1ap_ProcedureCode_id_PathSwitchRequest:
    *message_id = S1AP_PATH_SWITCH_FAILURE_LOG;
    return s1ap_mme_encode_pathSwitchRequestFailure (message_p, buffer, length);
  case S1ap_ProcedureCode_id_HandoverPreparation:
    *message_id = S1AP_HANDOVER_FAILURE_LOG;
    return s1ap_mme_encode_handoverPreparationFailure(message_p, buffer, length);

  default:
    OAILOG_DEBUG (LOG_S1AP, "Unknown procedure ID (%d) for unsuccessfull outcome message\n", (int)message_p->procedureCode);
    break;
  }

  return -1;
}

//------------------------------------------------------------------------------
static inline int
s1ap_mme_encode_s1setupresponse (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{
  S1ap_S1SetupResponse_t                  s1SetupResponse;
  S1ap_S1SetupResponse_t                 *s1SetupResponse_p = &s1SetupResponse;

  memset (s1SetupResponse_p, 0, sizeof (S1ap_S1SetupResponse_t));

  if (s1ap_encode_s1ap_s1setupresponseies (s1SetupResponse_p, &message_p->msg.s1ap_S1SetupResponseIEs) < 0) {
    return -1;
  }

  return s1ap_generate_successfull_outcome (buffer, length, S1ap_ProcedureCode_id_S1Setup, message_p->criticality, &asn_DEF_S1ap_S1SetupResponse, s1SetupResponse_p);
}

//------------------------------------------------------------------------------
static inline int
s1ap_mme_encode_s1setupfailure (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{
  S1ap_S1SetupFailure_t                   s1SetupFailure;
  S1ap_S1SetupFailure_t                  *s1SetupFailure_p = &s1SetupFailure;

  memset (s1SetupFailure_p, 0, sizeof (S1ap_S1SetupFailure_t));

  if (s1ap_encode_s1ap_s1setupfailureies (s1SetupFailure_p, &message_p->msg.s1ap_S1SetupFailureIEs) < 0) {
    return -1;
  }

  return s1ap_generate_unsuccessfull_outcome (buffer, length, S1ap_ProcedureCode_id_S1Setup, message_p->criticality, &asn_DEF_S1ap_S1SetupFailure, s1SetupFailure_p);
}

static inline int
s1ap_mme_encode_resetack (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{

  S1ap_ResetAcknowledge_t                 s1ResetAck;
  S1ap_ResetAcknowledge_t                 *s1ResetAck_p = &s1ResetAck;

  memset (s1ResetAck_p, 0, sizeof (S1ap_ResetAcknowledge_t));

  if (s1ap_encode_s1ap_resetacknowledgeies (s1ResetAck_p, &message_p->msg.s1ap_ResetAcknowledgeIEs) < 0) {
    return -1;
  }

  return s1ap_generate_successfull_outcome (buffer, length, S1ap_ProcedureCode_id_Reset, message_p->criticality, &asn_DEF_S1ap_ResetAcknowledge, s1ResetAck_p);
}



static inline int
s1ap_mme_encode_pathSwitchRequestAcknowledge(
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{

  S1ap_PathSwitchRequestAcknowledge_t            pathSwitchReqAcknowledge;
  S1ap_PathSwitchRequestAcknowledge_t           *pathSwitchReqAcknowledge_p = &pathSwitchReqAcknowledge;

  memset (pathSwitchReqAcknowledge_p, 0, sizeof (S1ap_PathSwitchRequestAcknowledge_t));

  if (s1ap_encode_s1ap_pathswitchrequestacknowledgeies (pathSwitchReqAcknowledge_p, &message_p->msg.s1ap_PathSwitchRequestAcknowledgeIEs) < 0) {
    return -1;
  }

  return s1ap_generate_successfull_outcome (buffer, length, S1ap_ProcedureCode_id_PathSwitchRequest, message_p->criticality,
      &asn_DEF_S1ap_PathSwitchRequestAcknowledge, pathSwitchReqAcknowledge_p);
}

static inline int
s1ap_mme_encode_pathSwitchRequestFailure (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{

  S1ap_PathSwitchRequestFailure_t                pathSwitchReqFailure;
  S1ap_PathSwitchRequestFailure_t               *pathSwitchReqFailure_p = &pathSwitchReqFailure;

  memset (pathSwitchReqFailure_p, 0, sizeof (S1ap_PathSwitchRequestFailure_t));

  if (s1ap_encode_s1ap_pathswitchrequestfailureies (pathSwitchReqFailure_p, &message_p->msg.s1ap_PathSwitchRequestFailureIEs) < 0) {
    return -1;
  }

  return s1ap_generate_unsuccessfull_outcome (buffer, length, S1ap_ProcedureCode_id_PathSwitchRequest, message_p->criticality, &asn_DEF_S1ap_PathSwitchRequestFailure, pathSwitchReqFailure_p);
}

static inline int
s1ap_mme_encode_handoverPreparationFailure (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{

  S1ap_HandoverPreparationFailure_t              handoverPreparationFailure;
  S1ap_HandoverPreparationFailureIEs_t          *handoverPreparationFailure_p = &handoverPreparationFailure;

  memset (handoverPreparationFailure_p, 0, sizeof (S1ap_HandoverPreparationFailure_t));

  if (s1ap_encode_s1ap_handoverpreparationfailureies(handoverPreparationFailure_p, &message_p->msg.s1ap_HandoverPreparationFailureIEs) < 0) {
    return -1;
  }

  return s1ap_generate_unsuccessfull_outcome (buffer, length, S1ap_ProcedureCode_id_HandoverPreparation, message_p->criticality, &asn_DEF_S1ap_HandoverPreparationFailure, handoverPreparationFailure_p);
}

static inline int
s1ap_mme_encode_handoverCommand(
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{

  S1ap_HandoverCommand_t            handoverCommand;
  S1ap_HandoverCommand_t           *handoverCommand_p = &handoverCommand;

  memset (handoverCommand_p, 0, sizeof (S1ap_HandoverCommand_t));

  if (s1ap_encode_s1ap_handovercommandies(handoverCommand_p, &message_p->msg.s1ap_HandoverCommandIEs) < 0) {
    return -1;
  }

  return s1ap_generate_successfull_outcome (buffer, length, S1ap_ProcedureCode_id_HandoverPreparation, message_p->criticality,
      &asn_DEF_S1ap_HandoverCommand, handoverCommand_p);
}

static inline int
s1ap_mme_encode_handoverCancelAck(
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{

  S1ap_HandoverCancelAcknowledge_t  handoverCancelAcknowledge;
  S1ap_HandoverCancelAcknowledge_t *handoverCancelAcknowledge_p = &handoverCancelAcknowledge;

  memset (handoverCancelAcknowledge_p, 0, sizeof (S1ap_HandoverCancel_t));

  if (s1ap_encode_s1ap_handovercancelacknowledgeies(handoverCancelAcknowledge_p, &message_p->msg.s1ap_HandoverCancelAcknowledgeIEs) < 0) {
    return -1;
  }

  return s1ap_generate_successfull_outcome (buffer, length, S1ap_ProcedureCode_id_HandoverCancel, message_p->criticality,
      &asn_DEF_S1ap_HandoverCancelAcknowledge, handoverCancelAcknowledge_p);
}


static inline int s1ap_mme_encode_handover_resource_allocation (
    s1ap_message * message_p,
    uint8_t ** buffer,
    uint32_t * length)
{
  S1ap_HandoverRequest_t                  s1HandoverRequest;
  S1ap_HandoverRequest_t                 *s1HandoverRequest_p = &s1HandoverRequest;

  memset (s1HandoverRequest_p, 0, sizeof (S1ap_HandoverRequest_t));

  if (s1ap_encode_s1ap_handoverrequesties(s1HandoverRequest_p, &message_p->msg.s1ap_HandoverRequestIEs) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message(buffer, length, S1ap_ProcedureCode_id_HandoverResourceAllocation, S1ap_Criticality_reject,
      &asn_DEF_S1ap_HandoverRequest, s1HandoverRequest_p);
}

static inline int s1ap_mme_encode_mme_status_transfer (
    s1ap_message * message_p,
    uint8_t ** buffer,
    uint32_t * length)
{
  S1ap_MMEStatusTransfer_t                s1MmeStatusTransfer;
  S1ap_MMEStatusTransfer_t               *s1MmeStatusTransfer_p = &s1MmeStatusTransfer;

  memset (s1MmeStatusTransfer_p, 0, sizeof (S1ap_MMEStatusTransfer_t));

  if (s1ap_encode_s1ap_mmestatustransferies(s1MmeStatusTransfer_p, &message_p->msg.s1ap_MMEStatusTransferIEs) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message(buffer, length, S1ap_ProcedureCode_id_MMEStatusTransfer, S1ap_Criticality_ignore,
      &asn_DEF_S1ap_MMEStatusTransfer, s1MmeStatusTransfer_p);
}

static inline int s1ap_mme_encode_mme_configuration_transfer (
    s1ap_message * message_p,
    uint8_t ** buffer,
    uint32_t * length)
{
  S1ap_MMEConfigurationTransfer_t        s1MmeConfigurationTransfer;
  S1ap_MMEConfigurationTransfer_t       *s1MmeConfigurationTransfer_p = &s1MmeConfigurationTransfer;

  memset (s1MmeConfigurationTransfer_p, 0, sizeof (S1ap_MMEConfigurationTransfer_t));

  if (s1ap_encode_s1ap_mmeconfigurationtransferies(s1MmeConfigurationTransfer_p, &message_p->msg.s1ap_MMEConfigurationTransferIEs) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message(buffer, length, S1ap_ProcedureCode_id_MMEConfigurationTransfer, S1ap_Criticality_ignore,
      &asn_DEF_S1ap_MMEConfigurationTransfer, s1MmeConfigurationTransfer_p);
}

//------------------------------------------------------------------------------
static inline int
s1ap_mme_encode_downlink_nas_transport (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{
  S1ap_DownlinkNASTransport_t             downlinkNasTransport;
  S1ap_DownlinkNASTransport_t            *downlinkNasTransport_p = &downlinkNasTransport;

  memset (downlinkNasTransport_p, 0, sizeof (S1ap_DownlinkNASTransport_t));

  /*
   * Convert IE structure into asn1 message_p
   */
  if (s1ap_encode_s1ap_downlinknastransporties (downlinkNasTransport_p, &message_p->msg.s1ap_DownlinkNASTransportIEs) < 0) {
    return -1;
  }

  /*
   * Generate Initiating message_p for the list of IEs
   */
  return s1ap_generate_initiating_message (buffer, length, S1ap_ProcedureCode_id_downlinkNASTransport, message_p->criticality, &asn_DEF_S1ap_DownlinkNASTransport, downlinkNasTransport_p);
}

//------------------------------------------------------------------------------
static inline int
s1ap_mme_encode_ue_context_release_command (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{
  S1ap_UEContextReleaseCommand_t          ueContextReleaseCommand;
  S1ap_UEContextReleaseCommand_t         *ueContextReleaseCommand_p = &ueContextReleaseCommand;

  memset (ueContextReleaseCommand_p, 0, sizeof (S1ap_UEContextReleaseCommand_t));

  /*
   * Convert IE structure into asn1 message_p
   */
  if (s1ap_encode_s1ap_uecontextreleasecommandies (ueContextReleaseCommand_p, &message_p->msg.s1ap_UEContextReleaseCommandIEs) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message (buffer, length, S1ap_ProcedureCode_id_UEContextRelease, message_p->criticality, &asn_DEF_S1ap_UEContextReleaseCommand, ueContextReleaseCommand_p);
}

//------------------------------------------------------------------------------
static inline int
s1ap_mme_encode_e_rab_setup (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{
  S1ap_E_RABSetupRequest_t          e_rab_setup;
  S1ap_E_RABSetupRequest_t         *e_rab_setup_p = &e_rab_setup;

  memset (e_rab_setup_p, 0, sizeof (S1ap_E_RABSetupRequest_t));

  /*
   * Convert IE structure into asn1 message_p
   */
  if (s1ap_encode_s1ap_e_rabsetuprequesties(e_rab_setup_p, &message_p->msg.s1ap_E_RABSetupRequestIEs) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message (buffer, length, S1ap_ProcedureCode_id_E_RABSetup, message_p->criticality, &asn_DEF_S1ap_E_RABSetupRequest, e_rab_setup_p);
}

//------------------------------------------------------------------------------
static inline int
s1ap_mme_encode_e_rab_modify (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{
  S1ap_E_RABModifyRequest_t          e_rab_modify;
  S1ap_E_RABModifyRequest_t         *e_rab_modify_p = &e_rab_modify;

  memset (e_rab_modify_p, 0, sizeof (S1ap_E_RABModifyRequest_t));

  /*
   * Convert IE structure into asn1 message_p
   */
  if (s1ap_encode_s1ap_e_rabmodifyrequesties(e_rab_modify_p, &message_p->msg.s1ap_E_RABModifyRequestIEs) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message (buffer, length, S1ap_ProcedureCode_id_E_RABModify, message_p->criticality, &asn_DEF_S1ap_E_RABModifyRequest, e_rab_modify_p);
}

//------------------------------------------------------------------------------
static inline int
s1ap_mme_encode_e_rab_release (
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{
  S1ap_E_RABReleaseCommand_t        e_rab_release;
  S1ap_E_RABReleaseCommand_t       *e_rab_release_p = &e_rab_release;

  memset (e_rab_release_p, 0, sizeof (S1ap_E_RABReleaseCommand_t));

  /*
   * Convert IE structure into asn1 message_p
   */
  if (s1ap_encode_s1ap_e_rabreleasecommandies(e_rab_release_p, &message_p->msg.s1ap_E_RABReleaseCommandIEs) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message (buffer, length, S1ap_ProcedureCode_id_E_RABRelease, message_p->criticality, &asn_DEF_S1ap_E_RABReleaseCommand, e_rab_release_p);
}

//------------------------------------------------------------------------------
static inline int
s1ap_mme_encode_paging(
  s1ap_message * message_p,
  uint8_t ** buffer,
  uint32_t * length)
{
  S1ap_Paging_t          paging;
  S1ap_Paging_t         *paging_p = &paging;

  memset (paging_p, 0, sizeof (S1ap_Paging_t));

  /*
   * Convert IE structure into asn1 message_p
   */
  if (s1ap_encode_s1ap_pagingies(paging_p, &message_p->msg.s1ap_PagingIEs) < 0) {
    return -1;
  }

  return s1ap_generate_initiating_message (buffer, length, S1ap_ProcedureCode_id_Paging, message_p->criticality, &asn_DEF_S1ap_E_RABSetupRequest, paging_p);
}

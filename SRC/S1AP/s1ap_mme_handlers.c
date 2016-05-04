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


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "mme_config.h"
#include "assertions.h"
#include "conversions.h"
#include "s1ap_common.h"
#include "s1ap_ies_defs.h"
#include "s1ap_mme_encoder.h"
#include "s1ap_mme_handlers.h"
#include "s1ap_mme_nas_procedures.h"
#include "s1ap_mme_itti_messaging.h"
#include "s1ap_mme.h"
#include "s1ap_mme_ta.h"
#include "hashtable.h"
#include "msc.h"
#include "log.h"


extern hash_table_ts_t g_s1ap_enb_coll; // contains eNB_description_s, key is eNB_description_s.assoc_id

static int                              s1ap_generate_s1_setup_response (
  enb_description_t * enb_association);
static int                              s1ap_mme_generate_ue_context_release_command (
  ue_description_t * ue_ref_p);

//Forward declaration
struct s1ap_message_s;

/* Handlers matrix. Only mme related procedures present here.
*/
s1ap_message_decoded_callback           messages_callback[][3] = {
  {0, 0, 0},                    /* HandoverPreparation */
  {0, 0, 0},                    /* HandoverResourceAllocation */
  {0, 0, 0},                    /* HandoverNotification */
  {s1ap_mme_handle_path_switch_request, 0, 0},  /* PathSwitchRequest */
  {0, 0, 0},                    /* HandoverCancel */
  {0, 0, 0},                    /* E_RABSetup */
  {0, 0, 0},                    /* E_RABModify */
  {0, 0, 0},                    /* E_RABRelease */
  {0, 0, 0},                    /* E_RABReleaseIndication */
  {
   0, s1ap_mme_handle_initial_context_setup_response,
   s1ap_mme_handle_initial_context_setup_failure},      /* InitialContextSetup */
  {0, 0, 0},                    /* Paging */
  {0, 0, 0},                    /* downlinkNASTransport */
  {s1ap_mme_handle_initial_ue_message, 0, 0},   /* initialUEMessage */
  {s1ap_mme_handle_uplink_nas_transport, 0, 0}, /* uplinkNASTransport */
  {0, 0, 0},                    /* Reset */
  {0, 0, 0},                    /* ErrorIndication */
  {s1ap_mme_handle_nas_non_delivery, 0, 0},     /* NASNonDeliveryIndication */
  {s1ap_mme_handle_s1_setup_request, 0, 0},     /* S1Setup */
  {s1ap_mme_handle_ue_context_release_request, 0, 0},   /* UEContextReleaseRequest */
  {0, 0, 0},                    /* DownlinkS1cdma2000tunneling */
  {0, 0, 0},                    /* UplinkS1cdma2000tunneling */
  {0, 0, 0},                    /* UEContextModification */
  {s1ap_mme_handle_ue_cap_indication, 0, 0},    /* UECapabilityInfoIndication */
  {
   s1ap_mme_handle_ue_context_release_request,
   s1ap_mme_handle_ue_context_release_complete, 0},     /* UEContextRelease */
  {0, 0, 0},                    /* eNBStatusTransfer */
  {0, 0, 0},                    /* MMEStatusTransfer */
  {0, 0, 0},                    /* DeactivateTrace */
  {0, 0, 0},                    /* TraceStart */
  {0, 0, 0},                    /* TraceFailureIndication */
  {0, 0, 0},                    /* ENBConfigurationUpdate */
  {0, 0, 0},                    /* MMEConfigurationUpdate */
  {0, 0, 0},                    /* LocationReportingControl */
  {0, 0, 0},                    /* LocationReportingFailureIndication */
  {0, 0, 0},                    /* LocationReport */
  {0, 0, 0},                    /* OverloadStart */
  {0, 0, 0},                    /* OverloadStop */
  {0, 0, 0},                    /* WriteReplaceWarning */
  {0, 0, 0},                    /* eNBDirectInformationTransfer */
  {0, 0, 0},                    /* MMEDirectInformationTransfer */
  {0, 0, 0},                    /* PrivateMessage */
  {0, 0, 0},                    /* eNBConfigurationTransfer */
  {0, 0, 0},                    /* MMEConfigurationTransfer */
  {0, 0, 0},                    /* CellTrafficTrace */
// UPDATE RELEASE 9
  {0, 0, 0},                    /* Kill */
  {0, 0, 0},                    /* DownlinkUEAssociatedLPPaTransport  */
  {0, 0, 0},                    /* UplinkUEAssociatedLPPaTransport */
  {0, 0, 0},                    /* DownlinkNonUEAssociatedLPPaTransport */
  {0, 0, 0},                    /* UplinkNonUEAssociatedLPPaTransport */
};

const char                             *s1ap_direction2String[] = {
  "",                           /* Nothing */
  "Originating message",        /* originating message */
  "Successfull outcome",        /* successfull outcome */
  "UnSuccessfull outcome",      /* successfull outcome */
};

//------------------------------------------------------------------------------
int
s1ap_mme_handle_message (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  /*
   * Checking procedure Code and direction of message
   */
  if ((message->procedureCode > (sizeof (messages_callback) / (3 * sizeof (s1ap_message_decoded_callback)))) || (message->direction > S1AP_PDU_PR_unsuccessfulOutcome)) {
    OAILOG_DEBUG (LOG_S1AP, "[SCTP %d] Either procedureCode %d or direction %d exceed expected\n", assoc_id, (int)message->procedureCode, (int)message->direction);
    return -1;
  }

  /*
   * No handler present.
   * * * * This can mean not implemented or no procedure for eNB (wrong message).
   */
  if (messages_callback[message->procedureCode][message->direction - 1] == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "[SCTP %d] No handler for procedureCode %d in %s\n", assoc_id, (int)message->procedureCode, s1ap_direction2String[(int)message->direction]);
    return -2;
  }

  /*
   * Calling the right handler
   */
  return (*messages_callback[message->procedureCode][message->direction - 1]) (assoc_id, stream, message);
}

//------------------------------------------------------------------------------
int
s1ap_mme_set_cause (
  S1ap_Cause_t * cause_p,
  const S1ap_Cause_PR cause_type,
  const long cause_value)
{
  DevAssert (cause_p != NULL);
  cause_p->present = cause_type;

  switch (cause_type) {
  case S1ap_Cause_PR_radioNetwork:
    cause_p->choice.misc = cause_value;
    break;

  case S1ap_Cause_PR_transport:
    cause_p->choice.transport = cause_value;
    break;

  case S1ap_Cause_PR_nas:
    cause_p->choice.nas = cause_value;
    break;

  case S1ap_Cause_PR_protocol:
    cause_p->choice.protocol = cause_value;
    break;

  case S1ap_Cause_PR_misc:
    cause_p->choice.misc = cause_value;
    break;

  default:
    return -1;
  }

  return 0;
}

//------------------------------------------------------------------------------
int
s1ap_mme_generate_s1_setup_failure (
    const sctp_assoc_id_t assoc_id,
    const S1ap_Cause_PR cause_type,
    const long cause_value,
    const long time_to_wait)
{
  uint8_t                                *buffer_p = 0;
  uint32_t                                length = 0;
  s1ap_message                            message = { 0 };
  S1ap_S1SetupFailureIEs_t               *s1_setup_failure_p = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_S1AP);
  s1_setup_failure_p = &message.msg.s1ap_S1SetupFailureIEs;
  message.procedureCode = S1ap_ProcedureCode_id_S1Setup;
  message.direction = S1AP_PDU_PR_unsuccessfulOutcome;
  s1ap_mme_set_cause (&s1_setup_failure_p->cause, cause_type, cause_value);

  /*
   * Include the optional field time to wait only if the value is > -1
   */
  if (time_to_wait > -1) {
    s1_setup_failure_p->presenceMask |= S1AP_S1SETUPFAILUREIES_TIMETOWAIT_PRESENT;
    s1_setup_failure_p->timeToWait = time_to_wait;
  }

  if (s1ap_mme_encode_pdu (&message, &buffer_p, &length) < 0) {
    OAILOG_ERROR (LOG_S1AP, "Failed to encode s1 setup failure\n");
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_S1AP_ENB, NULL, 0, "0 S1Setup/unsuccessfulOutcome  assoc_id %u cause %u value %u", assoc_id, cause_type, cause_value);
  rc =  s1ap_mme_itti_send_sctp_request (buffer_p, length, assoc_id, 0);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

////////////////////////////////////////////////////////////////////////////////
//************************** Management procedures ***************************//
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
int
s1ap_mme_handle_s1_setup_request (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{

  int                                     rc = RETURNok;
  OAILOG_FUNC_IN (LOG_S1AP);
  if (hss_associated) {
    S1ap_S1SetupRequestIEs_t               *s1SetupRequest_p = NULL;
    enb_description_t                      *enb_association = NULL;
    uint32_t                                enb_id = 0;
    char                                   *enb_name = NULL;
    int                                     ta_ret = 0;
    uint16_t                                max_enb_connected = 0;

    DevAssert (message != NULL);
    s1SetupRequest_p = &message->msg.s1ap_S1SetupRequestIEs;
    /*
     * We received a new valid S1 Setup Request on a stream != 0.
     * * * * This should not happen -> reject eNB s1 setup request.
     */
    MSC_LOG_RX_MESSAGE (MSC_S1AP_MME, MSC_S1AP_ENB, NULL, 0, "0 S1Setup/%s assoc_id %u stream %u", s1ap_direction2String[message->direction], assoc_id, stream);

    if (stream != 0) {
      OAILOG_ERROR (LOG_S1AP, "Received new s1 setup request on stream != 0\n");
      /*
       * Send a s1 setup failure with protocol cause unspecified
       */
      rc =  s1ap_mme_generate_s1_setup_failure (assoc_id, S1ap_Cause_PR_protocol, S1ap_CauseProtocol_unspecified, -1);
      OAILOG_FUNC_RETURN (LOG_S1AP, rc);
    }

    log_queue_item_t *  context = NULL;
    OAILOG_MESSAGE_START (OAILOG_LEVEL_DEBUG, LOG_S1AP, (&context), "New s1 setup request incoming from ");

    if (s1SetupRequest_p->presenceMask & S1AP_S1SETUPREQUESTIES_ENBNAME_PRESENT) {
      OAILOG_MESSAGE_ADD (context, "%*s ", s1SetupRequest_p->eNBname.size, s1SetupRequest_p->eNBname.buf);
      enb_name = (char *)s1SetupRequest_p->eNBname.buf;
    }

    if (s1SetupRequest_p->global_ENB_ID.eNB_ID.present == S1ap_ENB_ID_PR_homeENB_ID) {
      // Home eNB ID = 28 bits
      uint8_t                                *enb_id_buf = s1SetupRequest_p->global_ENB_ID.eNB_ID.choice.homeENB_ID.buf;

      if (s1SetupRequest_p->global_ENB_ID.eNB_ID.choice.macroENB_ID.size != 28) {
        //TODO: handle case were size != 28 -> notify ? reject ?
      }

      enb_id = (enb_id_buf[0] << 20) + (enb_id_buf[1] << 12) + (enb_id_buf[2] << 4) + ((enb_id_buf[3] & 0xf0) >> 4);
      OAILOG_MESSAGE_ADD (context, "home eNB id: %07x", enb_id);
    } else {
      // Macro eNB = 20 bits
      uint8_t                                *enb_id_buf = s1SetupRequest_p->global_ENB_ID.eNB_ID.choice.macroENB_ID.buf;

      if (s1SetupRequest_p->global_ENB_ID.eNB_ID.choice.macroENB_ID.size != 20) {
        //TODO: handle case were size != 20 -> notify ? reject ?
      }

      enb_id = (enb_id_buf[0] << 12) + (enb_id_buf[1] << 4) + ((enb_id_buf[2] & 0xf0) >> 4);
      OAILOG_MESSAGE_ADD (context, "macro eNB id: %05x", enb_id);
    }
    OAILOG_MESSAGE_FINISH(context);

    config_read_lock (&mme_config);
    max_enb_connected = mme_config.max_enbs;
    config_unlock (&mme_config);

    if (nb_enb_associated == max_enb_connected) {
      OAILOG_ERROR (LOG_S1AP, "There is too much eNB connected to MME, rejecting the association\n");
      OAILOG_DEBUG (LOG_S1AP, "Connected = %d, maximum allowed = %d\n", nb_enb_associated, max_enb_connected);
      /*
       * Send an overload cause...
       */
      rc = s1ap_mme_generate_s1_setup_failure (assoc_id, S1ap_Cause_PR_misc, S1ap_CauseMisc_control_processing_overload, S1ap_TimeToWait_v20s);
      OAILOG_FUNC_RETURN (LOG_S1AP, rc);
    }

    /* Requirement MME36.413R10_8.7.3.4 Abnormal Conditions
     * If the eNB initiates the procedure by sending a S1 SETUP REQUEST message including the PLMN Identity IEs and
     * none of the PLMNs provided by the eNB is identified by the MME, then the MME shall reject the eNB S1 Setup
     * Request procedure with the appropriate cause value, e.g, “Unknown PLMN”.
     */
    ta_ret = s1ap_mme_compare_ta_lists (&s1SetupRequest_p->supportedTAs);

    /*
     * eNB and MME have no common PLMN
     */
    if (ta_ret != TA_LIST_RET_OK) {
      OAILOG_ERROR (LOG_S1AP, "No Common PLMN with eNB, generate_s1_setup_failure\n");
      rc =  s1ap_mme_generate_s1_setup_failure (assoc_id, S1ap_Cause_PR_misc, S1ap_CauseMisc_unknown_PLMN, S1ap_TimeToWait_v20s);
      OAILOG_FUNC_RETURN (LOG_S1AP, rc);
    }

    OAILOG_DEBUG (LOG_S1AP, "Adding eNB to the list of served eNBs\n");

    if ((enb_association = s1ap_is_enb_id_in_list (enb_id)) == NULL) {
      /*
       * eNB has not been fount in list of associated eNB,
       * * * * Add it to the tail of list and initialize data
       */
      if ((enb_association = s1ap_is_enb_assoc_id_in_list (assoc_id)) == NULL) {
        /*
         * ??
         */
        OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
      } else {
        enb_association->s1_state = S1AP_RESETING;
        enb_association->enb_id = enb_id;
        enb_association->default_paging_drx = s1SetupRequest_p->defaultPagingDRX;

        if (enb_name != NULL) {
          memcpy (enb_association->enb_name, s1SetupRequest_p->eNBname.buf, s1SetupRequest_p->eNBname.size);
          enb_association->enb_name[s1SetupRequest_p->eNBname.size] = '\0';
        }
      }
    } else {
      enb_association->s1_state = S1AP_RESETING;

      /*
       * eNB has been fount in list, consider the s1 setup request as a reset connection,
       * * * * reseting any previous UE state if sctp association is != than the previous one
       */
      if (enb_association->sctp_assoc_id != assoc_id) {
        S1ap_S1SetupFailureIEs_t                s1SetupFailure;

        memset (&s1SetupFailure, 0, sizeof (s1SetupFailure));
        /*
         * Send an overload cause...
         */
        s1SetupFailure.cause.present = S1ap_Cause_PR_misc;      //TODO: send the right cause
        s1SetupFailure.cause.choice.misc = S1ap_CauseMisc_control_processing_overload;
        OAILOG_ERROR (LOG_S1AP, "Rejeting s1 setup request as eNB id %d is already associated to an active sctp association" "Previous known: %d, new one: %d\n", enb_id, enb_association->sctp_assoc_id, assoc_id);
        //             s1ap_mme_encode_s1setupfailure(&s1SetupFailure,
        //                                            receivedMessage->msg.s1ap_sctp_new_msg_ind.assocId);
        OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
      }

      /*
       * TODO: call the reset procedure
       */
    }

    s1ap_dump_enb (enb_association);
    rc =  s1ap_generate_s1_setup_response (enb_association);
    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  } else {
    /*
     * Can not process the request, MME is not connected to HSS
     */
    OAILOG_ERROR (LOG_S1AP, "Rejecting s1 setup request Can not process the request, MME is not connected to HSS\n");
    rc = s1ap_mme_generate_s1_setup_failure (assoc_id, S1ap_Cause_PR_misc, S1ap_CauseMisc_unspecified, -1);
    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  }
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
}

//------------------------------------------------------------------------------
static
  int
s1ap_generate_s1_setup_response (
  enb_description_t * enb_association)
{
  int                                     i,j;
  int                                     enc_rval = 0;
  S1ap_S1SetupResponseIEs_t              *s1_setup_response_p = NULL;
  S1ap_ServedGUMMEIsItem_t                servedGUMMEI;
  s1ap_message                            message = { 0 };
  uint8_t                                *buffer = NULL;
  uint32_t                                length = 0;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_S1AP);
  DevAssert (enb_association != NULL);
  // memset for gcc 4.8.4 instead of {0}, servedGUMMEI.servedPLMNs
  memset(&servedGUMMEI, 0, sizeof(S1ap_ServedGUMMEIsItem_t));
  // Generating response
  s1_setup_response_p = &message.msg.s1ap_S1SetupResponseIEs;
  config_read_lock (&mme_config);
  s1_setup_response_p->relativeMMECapacity = mme_config.relative_capacity;

  /*
   * Use the gummei parameters provided by configuration
   * that should be sorted
   */
  j = 0;
  for (i = 0; i < mme_config.served_tai.nb_tai; i++) {
    bool plmn_added = false;
    for (j=0; j < i; j++) {
      if ((mme_config.served_tai.plmn_mcc[j] == mme_config.served_tai.plmn_mcc[i]) &&
        (mme_config.served_tai.plmn_mnc[j] == mme_config.served_tai.plmn_mnc[i]) &&
        (mme_config.served_tai.plmn_mnc_len[i] == mme_config.served_tai.plmn_mnc_len[i])
        ) {
        plmn_added = true;
        break;
      }
    }
    if (false == plmn_added) {
      S1ap_PLMNidentity_t                    *plmn = NULL;
      /*
       * FIXME: free object from list once encoded
       */
      plmn = CALLOC_CHECK (1, sizeof (*plmn));
      MCC_MNC_TO_PLMNID (mme_config.served_tai.plmn_mcc[i], mme_config.served_tai.plmn_mnc[i], mme_config.served_tai.plmn_mnc_len[i], plmn);
      ASN_SEQUENCE_ADD (&servedGUMMEI.servedPLMNs.list, plmn);
    }
  }

  for (i = 0; i < mme_config.gummei.nb_mme_gid; i++) {
    S1ap_MME_Group_ID_t                    *mme_gid;

    /*
     * FIXME: free object from list once encoded
     */
    mme_gid = CALLOC_CHECK (1, sizeof (*mme_gid));
    INT16_TO_OCTET_STRING (mme_config.gummei.mme_gid[i], mme_gid);
    ASN_SEQUENCE_ADD (&servedGUMMEI.servedGroupIDs.list, mme_gid);
  }

  for (i = 0; i < mme_config.gummei.nb_mmec; i++) {
    S1ap_MME_Code_t                        *mmec;

    /*
     * FIXME: free object from list once encoded
     */
    mmec = CALLOC_CHECK (1, sizeof (*mmec));
    INT8_TO_OCTET_STRING (mme_config.gummei.mmec[i], mmec);
    ASN_SEQUENCE_ADD (&servedGUMMEI.servedMMECs.list, mmec);
  }

  config_unlock (&mme_config);
  /*
   * The MME is only serving E-UTRAN RAT, so the list contains only one element
   */
  ASN_SEQUENCE_ADD (&s1_setup_response_p->servedGUMMEIs, &servedGUMMEI);
  message.procedureCode = S1ap_ProcedureCode_id_S1Setup;
  message.direction = S1AP_PDU_PR_successfulOutcome;
  enc_rval = s1ap_mme_encode_pdu (&message, &buffer, &length);

  /*
   * Failed to encode s1 setup response...
   */
  if (enc_rval < 0) {
    OAILOG_DEBUG (LOG_S1AP, "Removed eNB %d\n", enb_association->sctp_assoc_id);
    s1ap_remove_enb (enb_association);
  } else {
    /*
     * Consider the response as sent. S1AP is ready to accept UE contexts
     */
    enb_association->s1_state = S1AP_READY;
  }

  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_S1AP_ENB, NULL, 0, "0 S1Setup/successfulOutcome assoc_id %u", enb_association->sctp_assoc_id);
  /*
   * Non-UE signalling -> stream 0
   */
  rc = s1ap_mme_itti_send_sctp_request (buffer, length, enb_association->sctp_assoc_id, 0);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_ue_cap_indication (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  ue_description_t                       *ue_ref_p = NULL;
  S1ap_UECapabilityInfoIndicationIEs_t   *ue_cap_p = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_S1AP);
  DevAssert (message != NULL);
  ue_cap_p = &message->msg.s1ap_UECapabilityInfoIndicationIEs;
  MSC_LOG_RX_MESSAGE (MSC_S1AP_MME,
                      MSC_S1AP_ENB,
                      NULL, 0, "0 UECapabilityInfoIndication/%s enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ",
                      s1ap_direction2String[message->direction], ue_cap_p->eNB_UE_S1AP_ID, ue_cap_p->mme_ue_s1ap_id);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (ue_cap_p->mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT "\n", (uint32_t) ue_cap_p->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != ue_cap_p->eNB_UE_S1AP_ID) {
    OAILOG_DEBUG (LOG_S1AP, "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT ", received: " ENB_UE_S1AP_ID_FMT "\n",
                     ue_ref_p->enb_ue_s1ap_id, (uint32_t)ue_cap_p->eNB_UE_S1AP_ID);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  /*
   * Just display a warning when message received over wrong stream
   */
  if (ue_ref_p->sctp_stream_recv != stream) {
    OAILOG_ERROR (LOG_S1AP, "Received ue capability indication for "
                "(MME UE S1AP ID/eNB UE S1AP ID) (" MME_UE_S1AP_ID_FMT "/" ENB_UE_S1AP_ID_FMT ") over wrong stream "
                "expecting %u, received on %u\n", (uint32_t) ue_cap_p->mme_ue_s1ap_id, ue_ref_p->enb_ue_s1ap_id, ue_ref_p->sctp_stream_recv, stream);
  }

  /*
   * Forward the ue capabilities to MME application layer
   */
  {
    MessageDef                             *message_p = NULL;
    itti_s1ap_ue_cap_ind_t                      *ue_cap_ind_p = NULL;

    message_p = itti_alloc_new_message (TASK_S1AP, S1AP_UE_CAPABILITIES_IND);
    DevAssert (message_p != NULL);
    ue_cap_ind_p = &message_p->ittiMsg.s1ap_ue_cap_ind;
    ue_cap_ind_p->enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
    ue_cap_ind_p->mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
    DevCheck (ue_cap_p->ueRadioCapability.size < S1AP_UE_RADIOCAPABILITY_MAX_SIZE, S1AP_UE_RADIOCAPABILITY_MAX_SIZE, ue_cap_p->ueRadioCapability.size, 0);
    memcpy (ue_cap_ind_p->radio_capabilities, ue_cap_p->ueRadioCapability.buf, ue_cap_p->ueRadioCapability.size);
    ue_cap_ind_p->radio_capabilities_length = ue_cap_p->ueRadioCapability.size;
    MSC_LOG_TX_MESSAGE (MSC_S1AP_MME,
                        MSC_MMEAPP_MME,
                        NULL, 0, "0 S1AP_UE_CAPABILITIES_IND enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " len %u",
                        ue_cap_ind_p->enb_ue_s1ap_id, ue_cap_ind_p->mme_ue_s1ap_id, ue_cap_ind_p->radio_capabilities_length);
    rc = itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  }
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

////////////////////////////////////////////////////////////////////////////////
//******************* Context Management procedures **************************//
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
int
s1ap_mme_handle_initial_context_setup_response (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  S1ap_InitialContextSetupResponseIEs_t  *initialContextSetupResponseIEs_p = NULL;
  S1ap_E_RABSetupItemCtxtSURes_t         *eRABSetupItemCtxtSURes_p = NULL;
  ue_description_t                       *ue_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_S1AP);
  initialContextSetupResponseIEs_p = &message->msg.s1ap_InitialContextSetupResponseIEs;
  MSC_LOG_RX_MESSAGE (MSC_S1AP_MME,
                      MSC_S1AP_ENB,
                      NULL, 0,
                      "0 InitialContextSetup/%s enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " len %u",
                      s1ap_direction2String[message->direction], initialContextSetupResponseIEs_p->eNB_UE_S1AP_ID, initialContextSetupResponseIEs_p->mme_ue_s1ap_id);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list ((uint32_t) initialContextSetupResponseIEs_p->mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT " %u(10)\n",
                      (uint32_t) initialContextSetupResponseIEs_p->mme_ue_s1ap_id, (uint32_t) initialContextSetupResponseIEs_p->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != initialContextSetupResponseIEs_p->eNB_UE_S1AP_ID) {
    OAILOG_DEBUG (LOG_S1AP, "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT " %u(10), received: 0x%06x %u(10)\n",
                ue_ref_p->enb_ue_s1ap_id, ue_ref_p->enb_ue_s1ap_id, (uint32_t) initialContextSetupResponseIEs_p->eNB_UE_S1AP_ID, (uint32_t) initialContextSetupResponseIEs_p->eNB_UE_S1AP_ID);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (initialContextSetupResponseIEs_p->e_RABSetupListCtxtSURes.s1ap_E_RABSetupItemCtxtSURes.count != 1) {
    OAILOG_DEBUG (LOG_S1AP, "E-RAB creation has failed\n");
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  ue_ref_p->s1_ue_state = S1AP_UE_CONNECTED;
  message_p = itti_alloc_new_message (TASK_S1AP, MME_APP_INITIAL_CONTEXT_SETUP_RSP);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  memset ((void *)&message_p->ittiMsg.mme_app_initial_context_setup_rsp, 0, sizeof (itti_mme_app_initial_context_setup_rsp_t));
  /*
   * Bad, very bad cast...
   */
  eRABSetupItemCtxtSURes_p = (S1ap_E_RABSetupItemCtxtSURes_t *)
    initialContextSetupResponseIEs_p->e_RABSetupListCtxtSURes.s1ap_E_RABSetupItemCtxtSURes.array[0];
  MME_APP_INITIAL_CONTEXT_SETUP_RSP (message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
  MME_APP_INITIAL_CONTEXT_SETUP_RSP (message_p).eps_bearer_id = eRABSetupItemCtxtSURes_p->e_RAB_ID;
  MME_APP_INITIAL_CONTEXT_SETUP_RSP (message_p).bearer_s1u_enb_fteid.ipv4 = 1;  // TO DO
  MME_APP_INITIAL_CONTEXT_SETUP_RSP (message_p).bearer_s1u_enb_fteid.ipv6 = 0;  // TO DO
  MME_APP_INITIAL_CONTEXT_SETUP_RSP (message_p).bearer_s1u_enb_fteid.interface_type = S1_U_ENODEB_GTP_U;
  MME_APP_INITIAL_CONTEXT_SETUP_RSP (message_p).bearer_s1u_enb_fteid.teid = htonl (*((uint32_t *) eRABSetupItemCtxtSURes_p->gTP_TEID.buf));
  memcpy (&MME_APP_INITIAL_CONTEXT_SETUP_RSP (message_p).bearer_s1u_enb_fteid.ipv4_address, eRABSetupItemCtxtSURes_p->transportLayerAddress.buf, 4);
  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME,
                      MSC_MMEAPP_MME,
                      NULL, 0,
                      "0 MME_APP_INITIAL_CONTEXT_SETUP_RSP mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ebi %u s1u enb teid %u",
                      MME_APP_INITIAL_CONTEXT_SETUP_RSP (message_p).mme_ue_s1ap_id,
                      MME_APP_INITIAL_CONTEXT_SETUP_RSP (message_p).eps_bearer_id,
                      MME_APP_INITIAL_CONTEXT_SETUP_RSP (message_p).bearer_s1u_enb_fteid.teid);
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}


//------------------------------------------------------------------------------
int
s1ap_mme_handle_ue_context_release_request (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  S1ap_UEContextReleaseRequestIEs_t      *ueContextReleaseRequest_p = NULL;
  ue_description_t                       *ue_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_S1AP);
  ueContextReleaseRequest_p = &message->msg.s1ap_UEContextReleaseRequestIEs;
  MSC_LOG_RX_MESSAGE (MSC_S1AP_MME,
                      MSC_S1AP_ENB,
                      NULL, 0,
                      "0 UEContextReleaseRequest/%s enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ",
                      s1ap_direction2String[message->direction], ueContextReleaseRequest_p->eNB_UE_S1AP_ID, ueContextReleaseRequest_p->mme_ue_s1ap_id);

  /*
   * The UE context release procedure is initiated if the cause is != than user inactivity.
   * * * * TS36.413 #8.3.2.2.
   */
  if (ueContextReleaseRequest_p->cause.present == S1ap_Cause_PR_radioNetwork) {
    if (ueContextReleaseRequest_p->cause.choice.radioNetwork == S1ap_CauseRadioNetwork_user_inactivity) {
      OAILOG_DEBUG (LOG_S1AP, "UE_CONTEXT_RELEASE_REQUEST ignored, cause user inactivity\n");
      MSC_LOG_EVENT (MSC_S1AP_MME, "0 UE_CONTEXT_RELEASE_REQUEST ignored, cause user inactivity", ueContextReleaseRequest_p->mme_ue_s1ap_id);
      OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
    }
  }

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (ueContextReleaseRequest_p->mme_ue_s1ap_id)) == NULL) {
    /*
     * MME doesn't know the MME UE S1AP ID provided.
     * * * * TODO
     */
    OAILOG_DEBUG (LOG_S1AP, "UE_CONTEXT_RELEASE_REQUEST ignored cause could not get context with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " %u(10)\n",
           (uint32_t)ueContextReleaseRequest_p->mme_ue_s1ap_id, (uint32_t)ueContextReleaseRequest_p->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_S1AP_MME, "0 UE_CONTEXT_RELEASE_REQUEST ignored, no context mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ", ueContextReleaseRequest_p->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  } else {
    if (ue_ref_p->enb_ue_s1ap_id == (ueContextReleaseRequest_p->eNB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK)) {
      /*
       * Both eNB UE S1AP ID and MME UE S1AP ID match.
       * * * * -> Send a UE context Release Command to eNB.
       * * * * TODO
       */
      //s1ap_mme_generate_ue_context_release_command(ue_ref_p);
      // UE context will be removed when receiving UE_CONTEXT_RELEASE_COMPLETE
      message_p = itti_alloc_new_message (TASK_S1AP, S1AP_UE_CONTEXT_RELEASE_REQ);
      AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
      memset ((void *)&message_p->ittiMsg.s1ap_ue_context_release_req, 0, sizeof (itti_s1ap_ue_context_release_req_t));
      S1AP_UE_CONTEXT_RELEASE_REQ (message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
      S1AP_UE_CONTEXT_RELEASE_REQ (message_p).enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
      MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_MMEAPP_MME, NULL, 0, "0 S1AP_UE_CONTEXT_RELEASE_REQ mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ",
              S1AP_UE_CONTEXT_RELEASE_REQ (message_p).mme_ue_s1ap_id);
      rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
      OAILOG_FUNC_RETURN (LOG_S1AP, rc);
    } else {
      // TODO
      OAILOG_DEBUG (LOG_S1AP, "UE_CONTEXT_RELEASE_REQUEST ignored, cause mismatch enb_ue_s1ap_id: ctxt " ENB_UE_S1AP_ID_FMT " != request " ENB_UE_S1AP_ID_FMT " ",
    		  (uint32_t)ue_ref_p->enb_ue_s1ap_id, (uint32_t)ueContextReleaseRequest_p->eNB_UE_S1AP_ID);
      MSC_LOG_EVENT (MSC_S1AP_MME, "0 UE_CONTEXT_RELEASE_REQUEST ignored, cause mismatch enb_ue_s1ap_id: ctxt " ENB_UE_S1AP_ID_FMT " != request " ENB_UE_S1AP_ID_FMT " ",
              ue_ref_p->enb_ue_s1ap_id, (uint32_t)ueContextReleaseRequest_p->eNB_UE_S1AP_ID);
      OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
    }
  }

  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
static int
s1ap_mme_generate_ue_context_release_command (
  ue_description_t * ue_ref_p)
{
  uint8_t                                *buffer = NULL;
  uint32_t                                length = 0;
  s1ap_message                            message = {0};
  S1ap_UEContextReleaseCommandIEs_t      *ueContextReleaseCommandIEs_p = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_S1AP);
  if (ue_ref_p == NULL) {
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  message.procedureCode = S1ap_ProcedureCode_id_UEContextRelease;
  message.direction = S1AP_PDU_PR_initiatingMessage;
  ueContextReleaseCommandIEs_p = &message.msg.s1ap_UEContextReleaseCommandIEs;
  /*
   * Fill in ID pair
   */
  ueContextReleaseCommandIEs_p->uE_S1AP_IDs.present = S1ap_UE_S1AP_IDs_PR_uE_S1AP_ID_pair;
  ueContextReleaseCommandIEs_p->uE_S1AP_IDs.choice.uE_S1AP_ID_pair.mME_UE_S1AP_ID = ue_ref_p->mme_ue_s1ap_id;
  ueContextReleaseCommandIEs_p->uE_S1AP_IDs.choice.uE_S1AP_ID_pair.eNB_UE_S1AP_ID = ue_ref_p->enb_ue_s1ap_id;
  ueContextReleaseCommandIEs_p->uE_S1AP_IDs.choice.uE_S1AP_ID_pair.iE_Extensions = NULL;
  ueContextReleaseCommandIEs_p->cause.present = S1ap_Cause_PR_radioNetwork;
  ueContextReleaseCommandIEs_p->cause.choice.radioNetwork = S1ap_CauseRadioNetwork_release_due_to_eutran_generated_reason;

  if (s1ap_mme_encode_pdu (&message, &buffer, &length) < 0) {
    MSC_LOG_EVENT (MSC_S1AP_MME, "0 UEContextRelease/initiatingMessage enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " encoding failed",
            ue_ref_p->enb_ue_s1ap_id, ue_ref_p->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_S1AP_ENB, NULL, 0, "0 UEContextRelease/initiatingMessage enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "",
          ue_ref_p->enb_ue_s1ap_id, ue_ref_p->mme_ue_s1ap_id);
  rc = s1ap_mme_itti_send_sctp_request (buffer, length, ue_ref_p->enb->sctp_assoc_id, ue_ref_p->sctp_stream_send);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}


void
s1ap_handle_delete_session_rsp (
  const itti_mme_app_delete_session_rsp_t * const mme_app_delete_session_rsp_p)
{
  ue_description_t                       *ue_ref = NULL;
  OAILOG_FUNC_IN (LOG_S1AP);
  DevAssert (mme_app_delete_session_rsp_p != NULL);
  if ((ue_ref = s1ap_is_ue_mme_id_in_list (mme_app_delete_session_rsp_p->ue_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "This mme ue s1ap id " MME_UE_S1AP_ID_FMT " is not attached to any UE context\n", mme_app_delete_session_rsp_p->ue_id);
  }
  else {
    s1ap_remove_ue (ue_ref);
  }
  OAILOG_FUNC_OUT (LOG_S1AP);
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
int
s1ap_handle_ue_context_release_command (
  const itti_s1ap_ue_context_release_command_t * const ue_context_release_command_pP)
{
  ue_description_t                       *ue_ref_p = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_S1AP);
  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (ue_context_release_command_pP->mme_ue_s1ap_id)) == NULL) {
    /*
     * MME doesn't know the MME UE S1AP ID provided.
     * * * * TODO
     */
    OAILOG_DEBUG (LOG_S1AP, "UE_CONTEXT_RELEASE_COMMAND ignored cause could not get context with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " %u(10)\n", ue_context_release_command_pP->mme_ue_s1ap_id, ue_context_release_command_pP->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_S1AP_MME, "0 UE_CONTEXT_RELEASE_COMMAND ignored, no context mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ", ue_context_release_command_pP->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  } else {
    rc = s1ap_mme_generate_ue_context_release_command (ue_ref_p);
    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  }

  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_ue_context_release_complete (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  S1ap_UEContextReleaseCompleteIEs_t     *ueContextReleaseComplete_p = NULL;
  ue_description_t                       *ue_ref_p = NULL;
  MessageDef                             *message_p = NULL;

  OAILOG_FUNC_IN (LOG_S1AP);
  ueContextReleaseComplete_p = &message->msg.s1ap_UEContextReleaseCompleteIEs;
  MSC_LOG_RX_MESSAGE (MSC_S1AP_MME,
                      MSC_S1AP_ENB,
                      NULL, 0,
                      "0 UEContextRelease/%s enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " len %u",
                      s1ap_direction2String[message->direction], ueContextReleaseComplete_p->eNB_UE_S1AP_ID, ueContextReleaseComplete_p->mme_ue_s1ap_id);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (ueContextReleaseComplete_p->mme_ue_s1ap_id)) == NULL) {
    /*
     * MME doesn't know the MME UE S1AP ID provided.
     * * * * TODO
     */
    MSC_LOG_EVENT (MSC_S1AP_MME, "0 UEContextReleaseComplete ignored, no context mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ", ueContextReleaseComplete_p->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  /*
   * eNB has sent a release complete message. We can safely remove UE context.
   * * * * TODO: inform NAS and remove e-RABS.
   */
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_UE_CONTEXT_RELEASE_COMPLETE);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  memset ((void *)&message_p->ittiMsg.s1ap_ue_context_release_complete, 0, sizeof (itti_s1ap_ue_context_release_complete_t));
  S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_MMEAPP_MME, NULL, 0, "0 S1AP_UE_CONTEXT_RELEASE_COMPLETE mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ", S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).mme_ue_s1ap_id);
  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  s1ap_remove_ue (ue_ref_p);
  OAILOG_DEBUG (LOG_S1AP, "Removed UE " MME_UE_S1AP_ID_FMT "\n", (uint32_t) ueContextReleaseComplete_p->mme_ue_s1ap_id);
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_initial_context_setup_failure (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  S1ap_InitialContextSetupFailureIEs_t   *initialContextSetupFailureIEs_p = NULL;
  ue_description_t                       *ue_ref_p = NULL;

  OAILOG_FUNC_IN (LOG_S1AP);
  initialContextSetupFailureIEs_p = &message->msg.s1ap_InitialContextSetupFailureIEs;
  MSC_LOG_RX_MESSAGE (MSC_S1AP_MME,
                      MSC_S1AP_ENB,
                      NULL, 0,
                      "0 InitialContextSetup/%s enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " len %u",
                      s1ap_direction2String[message->direction], initialContextSetupFailureIEs_p->eNB_UE_S1AP_ID, initialContextSetupFailureIEs_p->mme_ue_s1ap_id);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (initialContextSetupFailureIEs_p->mme_ue_s1ap_id)) == NULL) {
    /*
     * MME doesn't know the MME UE S1AP ID provided.
     */
    MSC_LOG_EVENT (MSC_S1AP_MME, "0 InitialContextSetupFailure ignored, no context mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ", initialContextSetupFailureIEs_p->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != (initialContextSetupFailureIEs_p->eNB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK)) {
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  s1ap_remove_ue (ue_ref_p);
  MSC_LOG_EVENT (MSC_S1AP_MME, "0 Removed UE mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ", initialContextSetupFailureIEs_p->mme_ue_s1ap_id);
  OAILOG_DEBUG (LOG_S1AP, "Removed UE " MME_UE_S1AP_ID_FMT "\n", (uint32_t) initialContextSetupFailureIEs_p->mme_ue_s1ap_id);
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

////////////////////////////////////////////////////////////////////////////////
//************************ Handover signalling *******************************//
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
int
s1ap_mme_handle_path_switch_request (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  S1ap_PathSwitchRequestIEs_t            *pathSwitchRequest_p = NULL;
  ue_description_t                       *ue_ref_p = NULL;
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;

  OAILOG_FUNC_IN (LOG_S1AP);
  pathSwitchRequest_p = &message->msg.s1ap_PathSwitchRequestIEs;
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (pathSwitchRequest_p->eNB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);
  OAILOG_DEBUG (LOG_S1AP, "Path Switch Request message received from eNB UE S1AP ID: " ENB_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (pathSwitchRequest_p->sourceMME_UE_S1AP_ID)) == NULL) {
    /*
     * The MME UE S1AP ID provided by eNB doesn't point to any valid UE.
     * * * * MME replies with a PATH SWITCH REQUEST FAILURE message and start operation
     * * * * as described in TS 36.413 [11].
     * * * * TODO
     */
  } else {
    if (ue_ref_p->enb_ue_s1ap_id != enb_ue_s1ap_id) {
      /*
       * Received unique UE eNB ID mismatch with the one known in MME.
       * * * * Handle this case as defined upper.
       * * * * TODO
       */
      return -1;
    }
    //TODO: Switch the eRABs provided
  }

  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
typedef struct arg_s1ap_send_enb_dereg_ind_s {
  uint         current_ue_index;
  uint         handled_ues;
  MessageDef  *message_p;
}arg_s1ap_send_enb_dereg_ind_t;

//------------------------------------------------------------------------------
static bool s1ap_send_enb_deregistered_ind (
    const hash_key_t keyP,
    void * const dataP,
    void *argP,
    void ** resultP) {

  arg_s1ap_send_enb_dereg_ind_t          *arg = (arg_s1ap_send_enb_dereg_ind_t*) argP;
  ue_description_t                       *ue_ref_p = (ue_description_t*)dataP;
  /*
   * Ask for a release of each UE context associated to the eNB
   */
  if (ue_ref_p) {
    if (arg->current_ue_index == 0) {
      arg->message_p = itti_alloc_new_message (TASK_S1AP, S1AP_ENB_DEREGISTERED_IND);
    }

    AssertFatal(arg->current_ue_index < S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE, "Too many deregistered UEs reported in S1AP_ENB_DEREGISTERED_IND message ");
    S1AP_ENB_DEREGISTERED_IND (arg->message_p).mme_ue_s1ap_id[arg->current_ue_index] = ue_ref_p->mme_ue_s1ap_id;

    // max ues reached
    if (arg->current_ue_index == 0 && arg->handled_ues > 0) {
      S1AP_ENB_DEREGISTERED_IND (arg->message_p).nb_ue_to_deregister = S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE;
      itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, arg->message_p);
      MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_NAS_MME, NULL, 0, "0 S1AP_ENB_DEREGISTERED_IND num ue to deregister %u",
          S1AP_ENB_DEREGISTERED_IND (arg->message_p).nb_ue_to_deregister);
      arg->message_p = NULL;
    }

    arg->handled_ues++;
    arg->current_ue_index = arg->handled_ues % S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE;
    *resultP = arg->message_p;
  } else {
    OAILOG_TRACE (LOG_S1AP, "No valid UE provided in callback: %p\n", ue_ref_p);
  }
  return false;
}
//------------------------------------------------------------------------------
int
s1ap_handle_sctp_deconnection (
    const sctp_assoc_id_t assoc_id)
{
  arg_s1ap_send_enb_dereg_ind_t           arg = {0};
  int                                     i = 0;
  MessageDef                             *message_p = NULL;
  enb_description_t                      *enb_association = NULL;

  OAILOG_FUNC_IN (LOG_S1AP);
  /*
   * Checking that the assoc id has a valid eNB attached to.
   */
  enb_association = s1ap_is_enb_assoc_id_in_list (assoc_id);

  if (enb_association == NULL) {
    OAILOG_ERROR (LOG_S1AP, "No eNB attached to this assoc_id: %d\n", assoc_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  MSC_LOG_EVENT (MSC_S1AP_MME, "0 Event SCTP_CLOSE_ASSOCIATION assoc_id: %d", assoc_id);

  hashtable_ts_apply_callback_on_elements(&enb_association->ue_coll, s1ap_send_enb_deregistered_ind, (void*)&arg, (void**)&message_p);

  if ( (!(arg.handled_ues % S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE)) && (arg.handled_ues) ){
    S1AP_ENB_DEREGISTERED_IND (message_p).nb_ue_to_deregister = arg.current_ue_index;

    for (i = arg.current_ue_index; i < S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE; i++) {
      S1AP_ENB_DEREGISTERED_IND (message_p).mme_ue_s1ap_id[arg.current_ue_index] = 0;
    }
    MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_NAS_MME, NULL, 0, "0 S1AP_ENB_DEREGISTERED_IND num ue to deregister %u", S1AP_ENB_DEREGISTERED_IND (message_p).nb_ue_to_deregister);
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
    message_p = NULL;
  }

  s1ap_remove_enb (enb_association);
  s1ap_dump_enb_list ();
  OAILOG_DEBUG (LOG_S1AP, "Removed eNB attached to assoc_id: %d\n", assoc_id);
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
int
s1ap_handle_new_association (
  sctp_new_peer_t * sctp_new_peer_p)
{
  enb_description_t                      *enb_association = NULL;

  OAILOG_FUNC_IN (LOG_S1AP);
  DevAssert (sctp_new_peer_p != NULL);

  /*
   * Checking that the assoc id has a valid eNB attached to.
   */
  if ((enb_association = s1ap_is_enb_assoc_id_in_list (sctp_new_peer_p->assoc_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "Create eNB context for assoc_id: %d\n", sctp_new_peer_p->assoc_id);
    /*
     * Create new context
     */
    enb_association = s1ap_new_enb ();

    if (enb_association == NULL) {
      /*
       * We failed to allocate memory
       */
      /*
       * TODO: send reject there
       */
      OAILOG_ERROR (LOG_S1AP, "Failed to allocate eNB context for assoc_id: %d\n", sctp_new_peer_p->assoc_id);
    }
    enb_association->sctp_assoc_id = sctp_new_peer_p->assoc_id;
    hashtable_rc_t  hash_rc = hashtable_ts_insert (&g_s1ap_enb_coll, (const hash_key_t)enb_association->sctp_assoc_id, (void *)enb_association);
    if (HASH_TABLE_OK != hash_rc) {
      OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
    }
  } else {
    OAILOG_DEBUG (LOG_S1AP, "eNB context already exists for assoc_id: %d, update it\n", sctp_new_peer_p->assoc_id);
  }

  enb_association->sctp_assoc_id = sctp_new_peer_p->assoc_id;
  /*
   * Fill in in and out number of streams available on SCTP connection.
   */
  enb_association->instreams = sctp_new_peer_p->instreams;
  enb_association->outstreams = sctp_new_peer_p->outstreams;
  /*
   * initialize the next sctp stream to 1 as 0 is reserved for non
   * * * * ue associated signalling.
   */
  enb_association->next_sctp_stream = 1;
  MSC_LOG_EVENT (MSC_S1AP_MME, "0 Event SCTP_NEW_ASSOCIATION assoc_id: %d", enb_association->sctp_assoc_id);
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}


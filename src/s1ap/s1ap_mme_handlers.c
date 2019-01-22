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

/*! \file s1ap_mme_handlers.c
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"

#include "hashtable.h"
#include "log.h"
#include "msc.h"
#include "3gpp_requirements_36.413.h"
#include "assertions.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "s1ap_common.h"
#include "s1ap_ies_defs.h"
#include "s1ap_mme_encoder.h"
#include "s1ap_mme_nas_procedures.h"
#include "s1ap_mme_itti_messaging.h"
#include "s1ap_mme.h"
#include "s1ap_mme_ta.h"
#include "s1ap_mme_gummei.h"
#include "s1ap_mme_handlers.h"
#include "mme_app_statistics.h"
#include "timer.h"
#include "dynamic_memory_check.h"

extern hash_table_ts_t g_s1ap_enb_coll; // contains eNB_description_s, key is eNB_description_s.assoc_id

static const char * const s1_enb_state_str [] = {"S1AP_INIT", "S1AP_RESETTING", "S1AP_READY", "S1AP_SHUTDOWN"};

static int                              s1ap_generate_s1_setup_response (
    enb_description_t * enb_association);

static int                              s1ap_mme_generate_ue_context_release_command (
    ue_description_t * ue_ref_p, mme_ue_s1ap_id_t mme_ue_s1ap_id, enum s1cause, enb_description_t* enb_ref_p);

static int                              s1ap_send_path_switch_request_failure (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    const mme_ue_s1ap_id_t mme_ue_s1ap_id,
    const enb_ue_s1ap_id_t enb_ue_s1ap_id);

//Forward declaration
struct s1ap_message_s;

/* Handlers matrix. Only mme related procedures present here.
*/
s1ap_message_decoded_callback           messages_callback[][3] = {
  {s1ap_mme_handle_handover_preparation, 0, 0},                    /* HandoverPreparation */
  {0, s1ap_mme_handle_handover_resource_allocation_response,              /* HandoverResourceAllocation */
      s1ap_mme_handle_handover_resource_allocation_failure},
  {s1ap_mme_handle_handover_notification, 0, 0},                    /* HandoverNotification */
  {s1ap_mme_handle_path_switch_request, 0, 0},  /* PathSwitchRequest */
  {s1ap_mme_handle_handover_cancel, 0, 0},                    /* HandoverCancel */

  {0, s1ap_mme_handle_erab_setup_response, 0},                    /* E_RABSetup */
  {0, s1ap_mme_handle_erab_modify_response, 0},                    /* E_RABModify */
  {0, s1ap_mme_handle_erab_release_response, 0},                    /* E_RABRelease */
  {s1ap_mme_handle_erab_release_indication, 0, 0},                    /* E_RABReleaseIndication */
  {0, s1ap_mme_handle_initial_context_setup_response,
   s1ap_mme_handle_initial_context_setup_failure},      /* InitialContextSetup */
  {0, 0, 0},                    /* Paging */
  {0, 0, 0},                    /* downlinkNASTransport */
  {s1ap_mme_handle_initial_ue_message, 0, 0},   /* initialUEMessage */
  {s1ap_mme_handle_uplink_nas_transport, 0, 0}, /* uplinkNASTransport */
  {s1ap_mme_handle_enb_reset, 0, 0},                    /* Reset */
  {s1ap_mme_handle_error_ind_message, 0, 0},                    /* ErrorIndication */
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
   {s1ap_mme_handle_enb_status_transfer, 0, 0},                    /* eNBStatusTransfer */
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
  MessagesIds                             message_id = MESSAGES_ID_MAX;
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

  if (s1ap_mme_encode_pdu (&message, &message_id, &buffer_p, &length) < 0) {
    OAILOG_ERROR (LOG_S1AP, "Failed to encode s1 setup failure\n");
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }
  s1ap_free_mme_encode_pdu(&message, message_id);

  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_S1AP_ENB, NULL, 0, "0 S1Setup/unsuccessfulOutcome  assoc_id %u cause %u value %u", assoc_id, cause_type, cause_value);
  bstring b = blk2bstr(buffer_p, length);
  free(buffer_p);
  rc =  s1ap_mme_itti_send_sctp_request (&b, assoc_id, 0, INVALID_MME_UE_S1AP_ID);
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

    shared_log_queue_item_t *  context = NULL;
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

    mme_config_read_lock (&mme_config);
    max_enb_connected = mme_config.max_enbs;
    mme_config_unlock (&mme_config);

    if (nb_enb_associated == max_enb_connected) {
      OAILOG_ERROR (LOG_S1AP, "There is too much eNB connected to MME, rejecting the association\n");
      OAILOG_DEBUG (LOG_S1AP, "Connected = %d, maximum allowed = %d\n", nb_enb_associated, max_enb_connected);
      /*
       * Send an overload cause...
       */
      rc = s1ap_mme_generate_s1_setup_failure (assoc_id, S1ap_Cause_PR_misc, S1ap_CauseMisc_control_processing_overload, S1ap_TimeToWait_v20s);
      OAILOG_FUNC_RETURN (LOG_S1AP, rc);
    }

    if(s1ap_mme_compare_gummei(&s1SetupRequest_p->global_ENB_ID.pLMNidentity) != RETURNok){
      OAILOG_ERROR (LOG_S1AP, "No Common GUMMEI with eNB, generate_s1_setup_failure\n");
      rc =  s1ap_mme_generate_s1_setup_failure (assoc_id, S1ap_Cause_PR_misc, S1ap_CauseMisc_unknown_PLMN, S1ap_TimeToWait_v20s);
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

        OAILOG_ERROR(LOG_S1AP, "Ignoring s1 setup from unknown assoc %u and enbId %u", assoc_id, enb_id);
        OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
      } else {
        enb_association->s1_state = S1AP_RESETING;
        enb_association->enb_id = enb_id;
        enb_association->default_paging_drx = s1SetupRequest_p->defaultPagingDRX;
        s1ap_set_tai(enb_association, &s1SetupRequest_p->supportedTAs);
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
        OAILOG_ERROR (LOG_S1AP, "Rejecting s1 setup request as eNB id %d is already associated to an active sctp association" "Previous known: %d, new one: %d\n", enb_id, enb_association->sctp_assoc_id, assoc_id);
        rc = s1ap_mme_generate_s1_setup_failure (assoc_id, S1ap_Cause_PR_misc, S1ap_CauseMisc_unspecified, -1); /**< eNB should attach again. */

        /** Also remove the old eNB. */
        OAILOG_INFO(LOG_S1AP, "Rejecting the old eNB connection for eNB id %d and old assoc_id: %d\n", enb_id, assoc_id);
        s1ap_dump_enb_list();
        s1ap_handle_sctp_disconnection(enb_association->sctp_assoc_id, false);
        OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
      }

      OAILOG_INFO(LOG_S1AP, "We found the eNB id %d in the current list of enbs with matching sctp associations:" "assoc: %d\n", enb_id, enb_association->sctp_assoc_id);
      /*
       * TODO: call the reset procedure
       */
      s1ap_dump_enb (enb_association);
      rc =  s1ap_generate_s1_setup_response (enb_association);
      OAILOG_FUNC_RETURN (LOG_S1AP, rc);
    }

    s1ap_dump_enb (enb_association);
    rc =  s1ap_generate_s1_setup_response (enb_association);
    if (rc == RETURNok) {
      update_mme_app_stats_connected_enb_add();
    }
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
  S1ap_ServedGUMMEIsItem_t               *servedGUMMEI = NULL;
  s1ap_message                            message = { 0 };
  MessagesIds                             message_id = MESSAGES_ID_MAX;
  uint8_t                                *buffer = NULL;
  uint32_t                                length = 0;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_S1AP);
  DevAssert (enb_association != NULL);
  // memset for gcc 4.8.4 instead of {0}, servedGUMMEI.servedPLMNs
  servedGUMMEI = calloc(1, sizeof *servedGUMMEI);
  // Generating response
  s1_setup_response_p = &message.msg.s1ap_S1SetupResponseIEs;
  mme_config_read_lock (&mme_config);
  s1_setup_response_p->relativeMMECapacity = mme_config.relative_capacity;

  /*
   * Use the gummei parameters provided by configuration
   * that should be sorted
   */
  for (i = 0; i < mme_config.served_tai.nb_tai; i++) {
    bool plmn_added = false;
    for (j=0; j < i; j++) {
      if ((mme_config.served_tai.plmn_mcc[j] == mme_config.served_tai.plmn_mcc[i]) &&
        (mme_config.served_tai.plmn_mnc[j] == mme_config.served_tai.plmn_mnc[i]) &&
        (mme_config.served_tai.plmn_mnc_len[j] == mme_config.served_tai.plmn_mnc_len[i])
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
      plmn = calloc (1, sizeof (*plmn));
      MCC_MNC_TO_PLMNID (mme_config.served_tai.plmn_mcc[i], mme_config.served_tai.plmn_mnc[i], mme_config.served_tai.plmn_mnc_len[i], plmn);
      ASN_SEQUENCE_ADD (&servedGUMMEI->servedPLMNs.list, plmn);
    }
  }

  for (i = 0; i < mme_config.gummei.nb; i++) {
    S1ap_MME_Group_ID_t                    *mme_gid = NULL;
    S1ap_MME_Code_t                        *mmec = NULL;

    /*
     * FIXME: free object from list once encoded
     */
    mme_gid = calloc (1, sizeof (*mme_gid));
    INT16_TO_OCTET_STRING (mme_config.gummei.gummei[i].mme_gid, mme_gid);
    ASN_SEQUENCE_ADD (&servedGUMMEI->servedGroupIDs.list, mme_gid);

    /*
     * FIXME: free object from list once encoded
     */
    mmec = calloc (1, sizeof (*mmec));
    INT8_TO_OCTET_STRING (mme_config.gummei.gummei[i].mme_code, mmec);
    ASN_SEQUENCE_ADD (&servedGUMMEI->servedMMECs.list, mmec);

  }


  mme_config_unlock (&mme_config);
  /*
   * The MME is only serving E-UTRAN RAT, so the list contains only one element
   */
  ASN_SEQUENCE_ADD (&s1_setup_response_p->servedGUMMEIs, servedGUMMEI);
  message.procedureCode = S1ap_ProcedureCode_id_S1Setup;
  message.direction = S1AP_PDU_PR_successfulOutcome;
  enc_rval = s1ap_mme_encode_pdu (&message, &message_id, &buffer, &length);

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
  bstring b = blk2bstr(buffer, length);
  free(buffer);
  s1ap_free_mme_encode_pdu(&message, message_id);
  rc = s1ap_mme_itti_send_sctp_request (&b, enb_association->sctp_assoc_id, 0, INVALID_MME_UE_S1AP_ID);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_ue_cap_indication (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
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
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
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

  if (initialContextSetupResponseIEs_p->e_RABSetupListCtxtSURes.s1ap_E_RABSetupItemCtxtSURes.count < 1) {
    OAILOG_DEBUG (LOG_S1AP, "E-RAB creation has failed\n");
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  /** Check if a context removal is ongoing. */
  if(ue_ref_p->s1_ue_state == S1AP_UE_WAITING_CRR && ue_ref_p->s1ap_ue_context_rel_timer.id != S1AP_TIMER_INACTIVE_ID){
    OAILOG_ERROR(LOG_S1AP, "A context release procedure is goingoing for ueRerefernce with ueId " MME_UE_S1AP_ID_FMT" and enbUeId " ENB_UE_S1AP_ID_FMT ". Ignoring received Setup Response. \n", ue_ref_p->mme_ue_s1ap_id, ue_ref_p->enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  ue_ref_p->s1_ue_state = S1AP_UE_CONNECTED;
  message_p = itti_alloc_new_message (TASK_S1AP, MME_APP_INITIAL_CONTEXT_SETUP_RSP);
  memset ((void *)&message_p->ittiMsg.mme_app_initial_context_setup_rsp, 0, sizeof (itti_mme_app_initial_context_setup_rsp_t));
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");

  itti_mme_app_initial_context_setup_rsp_t * initial_context_setup_rsp = NULL;
  initial_context_setup_rsp = &message_p->ittiMsg.mme_app_initial_context_setup_rsp;

  initial_context_setup_rsp->ue_id = ue_ref_p->mme_ue_s1ap_id;
  /** Add here multiple bearers. */
  initial_context_setup_rsp->bcs_to_be_modified.num_bearer_context = initialContextSetupResponseIEs_p->e_RABSetupListCtxtSURes.s1ap_E_RABSetupItemCtxtSURes.count;
  for (int item = 0; item < initialContextSetupResponseIEs_p->e_RABSetupListCtxtSURes.s1ap_E_RABSetupItemCtxtSURes.count; item++) {
    /*
     * Bad, very bad cast...
     */
    eRABSetupItemCtxtSURes_p = (S1ap_E_RABSetupItemCtxtSURes_t *)
        initialContextSetupResponseIEs_p->e_RABSetupListCtxtSURes.s1ap_E_RABSetupItemCtxtSURes.array[item];
    initial_context_setup_rsp->bcs_to_be_modified.bearer_contexts[item].eps_bearer_id = eRABSetupItemCtxtSURes_p->e_RAB_ID;
    initial_context_setup_rsp->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.teid = htonl (*((uint32_t *) eRABSetupItemCtxtSURes_p->gTP_TEID.buf));
    bstring transport_address = blk2bstr(eRABSetupItemCtxtSURes_p->transportLayerAddress.buf, eRABSetupItemCtxtSURes_p->transportLayerAddress.size);
    /** Set the IP address from the FTEID. */
    if (4 == blength(transport_address)) {
      initial_context_setup_rsp->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv4 = 1;
      memcpy(&initial_context_setup_rsp->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv4_address, transport_address->data, blength(transport_address));
    } else if (16 == blength(transport_address)) {
      initial_context_setup_rsp->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv6 = 1;
      memcpy(&initial_context_setup_rsp->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv6_address, transport_address->data, blength(transport_address));
    } else {
      AssertFatal(0, "TODO IP address %d bytes", blength(transport_address));
    }
    bdestroy_wrapper(&transport_address);
  }

  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME,
                      MSC_MMEAPP_MME,
                      NULL, 0,
                      "0 MME_APP_INITIAL_CONTEXT_SETUP_RSP mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ebi %u s1u enb teid %u",
                      initial_context_setup_rsp->ue_id,
                      initial_context_setup_rsp->bcs_to_be_modified.bearer_contexts[0].eps_bearer_id,
                      initial_context_setup_rsp->bcs_to_be_modified.bearer_contexts[0].s1_eNB_fteid.teid);
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}


//------------------------------------------------------------------------------
int
s1ap_mme_handle_ue_context_release_request (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  S1ap_UEContextReleaseRequestIEs_t      *ueContextReleaseRequest_p = NULL;
  ue_description_t                       *ue_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  S1ap_Cause_PR                           cause_type;
  long                                    cause_value;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_S1AP);
  ueContextReleaseRequest_p = &message->msg.s1ap_UEContextReleaseRequestIEs;
  MSC_LOG_RX_MESSAGE (MSC_S1AP_MME,
                      MSC_S1AP_ENB,
                      NULL, 0,
                      "0 UEContextReleaseRequest/%s enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ",
                      s1ap_direction2String[message->direction], ueContextReleaseRequest_p->eNB_UE_S1AP_ID, ueContextReleaseRequest_p->mme_ue_s1ap_id);
  // Log the Cause Type and Cause value
  cause_type = ueContextReleaseRequest_p->cause.present;

  switch (cause_type) 
  {
    case S1ap_Cause_PR_radioNetwork:
      cause_value = ueContextReleaseRequest_p->cause.choice.radioNetwork;
      OAILOG_DEBUG (LOG_S1AP, "UE CONTEXT RELEASE REQUEST with Cause_Type = Radio Network and Cause_Value = %ld\n", cause_value);
      break;

    case S1ap_Cause_PR_transport:
      cause_value = ueContextReleaseRequest_p->cause.choice.transport;
      OAILOG_DEBUG (LOG_S1AP, "UE CONTEXT RELEASE REQUEST with Cause_Type = Transport and Cause_Value = %ld\n", cause_value);
      break;

    case S1ap_Cause_PR_nas:
      cause_value = ueContextReleaseRequest_p->cause.choice.nas;
      OAILOG_DEBUG (LOG_S1AP, "UE CONTEXT RELEASE REQUEST with Cause_Type = NAS and Cause_Value = %ld\n", cause_value);
      break;

    case S1ap_Cause_PR_protocol:
      cause_value = ueContextReleaseRequest_p->cause.choice.protocol;
      OAILOG_DEBUG (LOG_S1AP, "UE CONTEXT RELEASE REQUEST with Cause_Type = Protocol and Cause_Value = %ld\n", cause_value);
      break;

    case S1ap_Cause_PR_misc:
      cause_value = ueContextReleaseRequest_p->cause.choice.misc;
      OAILOG_DEBUG (LOG_S1AP, "UE CONTEXT RELEASE REQUEST with Cause_Type = MISC and Cause_Value = %ld\n", cause_value);
      break;

    default:
      OAILOG_ERROR (LOG_S1AP, "UE CONTEXT RELEASE REQUEST with Invalid Cause_Type = %d\n", cause_type);
      OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  /* Fix - MME shall handle UE Context Release received from the eNB irrespective of the cause. And MME should release the S1-U bearers for the UE and move the UE to ECM idle mode. 
  Cause can influence whether to preserve GBR bearers or not.Since, as of now EPC doesn't support dedicated bearers, it is don't care scenario till we add support for dedicated bearers.
  */ 

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
      S1AP_UE_CONTEXT_RELEASE_REQ (message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
      S1AP_UE_CONTEXT_RELEASE_REQ (message_p).enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
      S1AP_UE_CONTEXT_RELEASE_REQ (message_p).enb_id = ue_ref_p->enb->enb_id;
      S1AP_UE_CONTEXT_RELEASE_REQ (message_p).cause = ueContextReleaseRequest_p->cause;

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
  ue_description_t * ue_ref_p, mme_ue_s1ap_id_t mme_ue_s1ap_id, enum s1cause cause, enb_description_t* enb_ref_p )
{
  uint8_t                                *buffer = NULL;
  uint32_t                                length = 0;
  s1ap_message                            message = {0};
  S1ap_UEContextReleaseCommandIEs_t      *ueContextReleaseCommandIEs_p = NULL;
  int                                     rc = RETURNok;
  S1ap_Cause_PR                           cause_type;
  long                                    cause_value;
  MessagesIds                             message_id = MESSAGES_ID_MAX;

  bool                                    expect_release_complete = true;
  MessageDef                             *message_p = NULL;

  OAILOG_FUNC_IN (LOG_S1AP);

  message.procedureCode = S1ap_ProcedureCode_id_UEContextRelease;
  message.direction = S1AP_PDU_PR_initiatingMessage;
  ueContextReleaseCommandIEs_p = &message.msg.s1ap_UEContextReleaseCommandIEs;
  /*
   * Fill in ID pair, depending if a UE_REFERENCE exists or not.
   */
  if(ue_ref_p){
    ueContextReleaseCommandIEs_p->uE_S1AP_IDs.present = S1ap_UE_S1AP_IDs_PR_uE_S1AP_ID_pair;
    ueContextReleaseCommandIEs_p->uE_S1AP_IDs.choice.uE_S1AP_ID_pair.mME_UE_S1AP_ID = mme_ue_s1ap_id;
    ueContextReleaseCommandIEs_p->uE_S1AP_IDs.choice.uE_S1AP_ID_pair.eNB_UE_S1AP_ID = ue_ref_p->enb_ue_s1ap_id;
    ueContextReleaseCommandIEs_p->uE_S1AP_IDs.choice.uE_S1AP_ID_pair.iE_Extensions = NULL;
  }else{
    ueContextReleaseCommandIEs_p->uE_S1AP_IDs.present = S1ap_UE_S1AP_IDs_PR_mME_UE_S1AP_ID;
    ueContextReleaseCommandIEs_p->uE_S1AP_IDs.choice.mME_UE_S1AP_ID = mme_ue_s1ap_id;
    ueContextReleaseCommandIEs_p->uE_S1AP_IDs.choice.uE_S1AP_ID_pair.iE_Extensions = NULL;
  }
  switch (cause) {
  case S1AP_NAS_DETACH:cause_type = S1ap_Cause_PR_nas;
    cause_value = S1ap_CauseNas_detach;
    break;
  case S1AP_INVALIDATE_NAS:
  case S1AP_NAS_NORMAL_RELEASE:cause_type = S1ap_Cause_PR_nas;
    cause_value = S1ap_CauseNas_unspecified;
    break;
  case S1AP_RADIO_EUTRAN_GENERATED_REASON:cause_type = S1ap_Cause_PR_radioNetwork;
    cause_value = S1ap_CauseRadioNetwork_release_due_to_eutran_generated_reason;
    break;
  case S1AP_INITIAL_CONTEXT_SETUP_FAILED:cause_type = S1ap_Cause_PR_radioNetwork;
    cause_value = S1ap_CauseRadioNetwork_unspecified;
    break;
  case S1AP_HANDOVER_CANCELLED:cause_type = S1ap_Cause_PR_radioNetwork;
    cause_value = S1ap_CauseRadioNetwork_handover_cancelled;
    if(!ue_ref_p)
      expect_release_complete = false;
    break;
  case S1AP_HANDOVER_FAILED:cause_type = S1ap_Cause_PR_radioNetwork;
    cause_value = S1ap_CauseRadioNetwork_ho_failure_in_target_EPC_eNB_or_target_system;
    break;
  case S1AP_SUCCESSFUL_HANDOVER: cause_type = S1ap_Cause_PR_radioNetwork;
    cause_value = S1ap_CauseRadioNetwork_successful_handover;
    break;
  /**
   * If the cause is an S1AP_NETWORK_ERROR or an S1AP_SYSTEM_FAILURE, we send just a UE-Context-Release-Request (and dismiss the complete).
   * No UE Reference will exist at that point.
   */
  case S1AP_NETWORK_ERROR:
  case S1AP_SYSTEM_FAILURE: cause_type = S1ap_Cause_PR_transport;
    cause_value = S1ap_CauseTransport_unspecified;
    expect_release_complete = false;
    break;
  default:
    AssertFatal(false, "Unknown cause for context release");
    break;
  }
  s1ap_mme_set_cause(&ueContextReleaseCommandIEs_p->cause, cause_type, cause_value);

  if (s1ap_mme_encode_pdu (&message, &message_id, &buffer, &length) < 0) {
    MSC_LOG_EVENT (MSC_S1AP_MME, "0 UEContextRelease/initiatingMessage enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " encoding failed",
            ue_ref_p->enb_ue_s1ap_id, ue_ref_p->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_S1AP_ENB, NULL, 0, "0 UEContextRelease/initiatingMessage enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "",
          (ue_ref_p) ? ue_ref_p->enb_ue_s1ap_id : 0, mme_ue_s1ap_id);

  bstring b = blk2bstr(buffer, length);
  free(buffer);
  s1ap_free_mme_encode_pdu(&message, message_id);

  rc = s1ap_mme_itti_send_sctp_request (&b, enb_ref_p->sctp_assoc_id, (ue_ref_p) ? ue_ref_p->sctp_stream_send : enb_ref_p->next_sctp_stream, mme_ue_s1ap_id);
  if(ue_ref_p){
    ue_ref_p->s1_release_cause = cause;

    if(!expect_release_complete){
      OAILOG_DEBUG (LOG_S1AP, "UE_CONTEXT_RELEASE_REQUEST will be sent for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " with directy UE-Reference removal "
          "(not waiting for RELEASE-COMPLETE due reason %d. \n", ue_ref_p->mme_ue_s1ap_id, cause);
      s1ap_remove_ue (ue_ref_p);
      OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
    }else{
      ue_ref_p->s1_ue_state = S1AP_UE_WAITING_CRR;
      OAILOG_DEBUG (LOG_S1AP, "UE_CONTEXT_RELEASE_REQUEST will be sent for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT ". Waiting for RELEASE-COMPLETE to remove the UE-Reference due reason %d. \n",
          ue_ref_p->mme_ue_s1ap_id, cause);
      // Start timer to track UE context release complete from eNB
      if (timer_setup (ue_ref_p->s1ap_ue_context_rel_timer.sec, 0,
          TASK_S1AP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *)(ue_ref_p), &(ue_ref_p->s1ap_ue_context_rel_timer.id)) < 0) {
        OAILOG_ERROR (LOG_S1AP, "Failed to start UE context release complete timer for UE id %d \n", ue_ref_p->mme_ue_s1ap_id);
        ue_ref_p->s1ap_ue_context_rel_timer.id = S1AP_TIMER_INACTIVE_ID;
      } else {
        OAILOG_DEBUG (LOG_S1AP, "Started S1AP UE context release timer for UE id  %d and timer id 0x%lx  \n", ue_ref_p->mme_ue_s1ap_id, ue_ref_p->s1ap_ue_context_rel_timer.id);
      }
      OAILOG_FUNC_RETURN (LOG_S1AP, rc);
    }
  }else{
    OAILOG_DEBUG (LOG_S1AP, "No UE reference exists for UE id " MME_UE_S1AP_ID_FMT ". Continuing with release complete acknowledge. \n", mme_ue_s1ap_id);
    message_p = itti_alloc_new_message (TASK_S1AP, S1AP_UE_CONTEXT_RELEASE_COMPLETE);
    AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
    memset ((void *)&message_p->ittiMsg.s1ap_ue_context_release_complete, 0, sizeof (itti_s1ap_ue_context_release_complete_t));

    /** Notify the MME_APP layer that error handling context removals can continue. */
    S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).mme_ue_s1ap_id = mme_ue_s1ap_id;
    MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_MMEAPP_MME, NULL, 0, "0 S1AP_UE_CONTEXT_RELEASE_COMPLETE mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ", S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).mme_ue_s1ap_id);
    itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  }
}


//------------------------------------------------------------------------------
int
s1ap_handle_ue_context_release_command (
  const itti_s1ap_ue_context_release_command_t * const ue_context_release_command_pP)
{
  ue_description_t                       *ue_ref_p = NULL;
  enb_description_t                      *enb_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_S1AP);

  /** Get the eNB reference. */
  enb_ref_p = s1ap_is_enb_id_in_list(ue_context_release_command_pP->enb_id);

  if(!enb_ref_p){
    OAILOG_DEBUG (LOG_S1AP, "No enbRef could be found for enb_id %d for releasing the context of ueId " MME_UE_S1AP_ID_FMT " and enbUeS1apId " ENB_UE_S1AP_ID_FMT ". \n",
        ue_context_release_command_pP->enb_id, ue_context_release_command_pP->mme_ue_s1ap_id, ue_context_release_command_pP->enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if ((ue_ref_p = s1ap_is_enb_ue_s1ap_id_in_list_per_enb (ue_context_release_command_pP->enb_ue_s1ap_id, ue_context_release_command_pP->enb_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "No UE reference could be found for enbUeS1apId " ENB_UE_S1AP_ID_FMT " and enbId %d. \n",
                  ue_context_release_command_pP->enb_ue_s1ap_id, ue_context_release_command_pP->enb_id);
    MSC_LOG_EVENT (MSC_S1AP_MME, "0 UE_CONTEXT_RELEASE_COMMAND with mme_ue_s1ap_id only for non existing UE_REFERENCE for mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ", ue_context_release_command_pP->mme_ue_s1ap_id);
    /** We might receive a duplicate one. */
    ue_ref_p = s1ap_is_ue_mme_id_in_list(ue_context_release_command_pP->mme_ue_s1ap_id);
  }

  if(ue_ref_p){
    OAILOG_DEBUG (LOG_S1AP, "UE reference for enbUeS1apId " ENB_UE_S1AP_ID_FMT " and enbId %d is %p. \n",
                      ue_context_release_command_pP->enb_ue_s1ap_id, ue_context_release_command_pP->enb_id, ue_ref_p);
  }

  /**
   * Check the cause. If it is implicit detach or sctp reset/shutdown no need to send UE context release command to eNB.
   * Free UE context locally.
   */
  if (ue_context_release_command_pP->cause == S1AP_IMPLICIT_CONTEXT_RELEASE ||
      ue_context_release_command_pP->cause == S1AP_SCTP_SHUTDOWN_OR_RESET) {
    s1ap_remove_ue (ue_ref_p);
    /** Send release complete back to proceed with the detach procedure. */
    OAILOG_DEBUG (LOG_S1AP, "Removed UE reference for ueId " MME_UE_S1AP_ID_FMT " implicitly. Continuing with release complete. \n", ue_context_release_command_pP->mme_ue_s1ap_id);
    message_p = itti_alloc_new_message (TASK_S1AP, S1AP_UE_CONTEXT_RELEASE_COMPLETE);
    AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
    memset ((void *)&message_p->ittiMsg.s1ap_ue_context_release_complete, 0, sizeof (itti_s1ap_ue_context_release_complete_t));

    /** Notify the MME_APP layer that error handling context removals can continue. */
    S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).mme_ue_s1ap_id = ue_context_release_command_pP->mme_ue_s1ap_id;
    MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_MMEAPP_MME, NULL, 0, "0 S1AP_UE_CONTEXT_RELEASE_COMPLETE mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ", S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).mme_ue_s1ap_id);
    itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  } else {
    /** We send the mme_ue_s1ap_id explicitly, since it may be 0 in some handover complete procedures. */
    rc = s1ap_mme_generate_ue_context_release_command (ue_ref_p, ue_context_release_command_pP->mme_ue_s1ap_id, ue_context_release_command_pP->cause, enb_ref_p);
  }

  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_ue_context_release_complete (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
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

  enb_description_t * enb_ref = s1ap_is_enb_assoc_id_in_list (assoc_id);
  DevAssert(enb_ref);
  if ((ue_ref_p = s1ap_is_ue_enb_id_in_list(enb_ref, ueContextReleaseComplete_p->eNB_UE_S1AP_ID)) == NULL) {
    /*
     * MME doesn't know the MME UE S1AP ID provided.
     * This implies that UE context has already been deleted on the expiry of timer!
     * Ignore this message. The timer will trigger the cleanup in the MME_APP. EMM is again independent.
     */
    OAILOG_DEBUG (LOG_S1AP, " UE Context Release complete:No S1 context. Ignore the message for ueid " MME_UE_S1AP_ID_FMT "\n", (uint32_t) ueContextReleaseComplete_p->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_S1AP_MME, "0 UEContextReleaseComplete ignored, no context mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ", ueContextReleaseComplete_p->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
  }

  /*
   * eNB has sent a release complete message. We can safely remove UE context.
   * TODO: inform NAS and remove e-RABS.
   */
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_UE_CONTEXT_RELEASE_COMPLETE);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).mme_ue_s1ap_id = ueContextReleaseComplete_p->mme_ue_s1ap_id;
  S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).enb_ue_s1ap_id = ueContextReleaseComplete_p->eNB_UE_S1AP_ID;
  S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).enb_id         = ue_ref_p->enb->enb_id;
  S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).sctp_assoc_id  = ue_ref_p->enb->sctp_assoc_id;
  DevAssert(ue_ref_p->s1_ue_state == S1AP_UE_WAITING_CRR);
  s1ap_remove_ue (ue_ref_p);

  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_MMEAPP_MME, NULL, 0, "0 S1AP_UE_CONTEXT_RELEASE_COMPLETE mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ", S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).mme_ue_s1ap_id);

  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_DEBUG (LOG_S1AP, "Removed UE " MME_UE_S1AP_ID_FMT "\n", (uint32_t) ueContextReleaseComplete_p->mme_ue_s1ap_id);
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_initial_context_setup_failure (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  S1ap_InitialContextSetupFailureIEs_t   *initialContextSetupFailureIEs_p = NULL;
  ue_description_t                       *ue_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  S1ap_Cause_PR                          cause_type;
  long                                   cause_value;
  int                                    rc = RETURNok;

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
    OAILOG_INFO (LOG_S1AP, "INITIAL_CONTEXT_SETUP_FAILURE ignored. No context with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ",
           (uint32_t)initialContextSetupFailureIEs_p->mme_ue_s1ap_id, (uint32_t)initialContextSetupFailureIEs_p->eNB_UE_S1AP_ID);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != (initialContextSetupFailureIEs_p->eNB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK)) {
    // abnormal case. No need to do anything. Ignore the message
    OAILOG_DEBUG (LOG_S1AP, "INITIAL_CONTEXT_SETUP_FAILURE ignored, mismatch enb_ue_s1ap_id: ctxt " ENB_UE_S1AP_ID_FMT " != received " ENB_UE_S1AP_ID_FMT " ",
           (uint32_t)ue_ref_p->enb_ue_s1ap_id, (uint32_t)(initialContextSetupFailureIEs_p->eNB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK));
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }
  // Pass this message to MME APP for necessary handling
  // Log the Cause Type and Cause value
  cause_type = initialContextSetupFailureIEs_p->cause.present;

  switch (cause_type)
  {
    case S1ap_Cause_PR_radioNetwork:
      cause_value = initialContextSetupFailureIEs_p->cause.choice.radioNetwork;
      OAILOG_DEBUG (LOG_S1AP, "INITIAL_CONTEXT_SETUP_FAILURE with Cause_Type = Radio Network and Cause_Value = %ld\n", cause_value);
      break;

    case S1ap_Cause_PR_transport:
      cause_value = initialContextSetupFailureIEs_p->cause.choice.transport;
      OAILOG_DEBUG (LOG_S1AP, "INITIAL_CONTEXT_SETUP_FAILURE with Cause_Type = Transport and Cause_Value = %ld\n", cause_value);
      break;

    case S1ap_Cause_PR_nas:
      cause_value = initialContextSetupFailureIEs_p->cause.choice.nas;
      OAILOG_DEBUG (LOG_S1AP, "INITIAL_CONTEXT_SETUP_FAILURE with Cause_Type = NAS and Cause_Value = %ld\n", cause_value);
      break;

    case S1ap_Cause_PR_protocol:
      cause_value = initialContextSetupFailureIEs_p->cause.choice.protocol;
      OAILOG_DEBUG (LOG_S1AP, "INITIAL_CONTEXT_SETUP_FAILURE with Cause_Type = Protocol and Cause_Value = %ld\n", cause_value);
      break;

    case S1ap_Cause_PR_misc:
      cause_value = initialContextSetupFailureIEs_p->cause.choice.misc;
      OAILOG_DEBUG (LOG_S1AP, "INITIAL_CONTEXT_SETUP_FAILURE with Cause_Type = MISC and Cause_Value = %ld\n", cause_value);
      break;

    default:
      OAILOG_ERROR (LOG_S1AP, "INITIAL_CONTEXT_SETUP_FAILURE with Invalid Cause_Type = %d\n", cause_type);
      OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }
  message_p = itti_alloc_new_message (TASK_S1AP, MME_APP_INITIAL_CONTEXT_SETUP_FAILURE);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  memset ((void *)&message_p->ittiMsg.mme_app_initial_context_setup_failure, 0, sizeof (itti_mme_app_initial_context_setup_failure_t));
  MME_APP_INITIAL_CONTEXT_SETUP_FAILURE (message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

////////////////////////////////////////////////////////////////////////////////
//************************ Handover signalling *******************************//
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
int
s1ap_mme_handle_path_switch_request (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  OAILOG_FUNC_IN (LOG_S1AP);

  S1ap_PathSwitchRequestIEs_t            *pathSwitchRequest_p = NULL;
  S1ap_E_RABToBeSwitchedDLItemIEs_t      *eRABToBeSwitchedDlItemIEs_p = NULL;

  ue_description_t                       *ue_ref_p = NULL;
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                        mme_ue_s1ap_id = 0;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;
  ecgi_t                                  ecgi = {.plmn = {0}, .cell_identity = {0}};

  //  todo: The MME shall verify that the UE security
  //  capabilities received from the eNB are the same as the UE security capabilities that the MME has stored. If
  //  there is a mismatch, the MME may log the event and may take additional measures, such as raising an alarm.

  //Request IEs:
  //S1ap-ENB-UE-S1AP-ID
  //S1ap-E-RABToBeSwitchedDLList
  //S1ap-MME-UE-S1AP-ID
  //S1ap-EUTRAN-CGI
  //S1ap-TAI
  //S1ap-UESecurityCapabilities

  //Acknowledge IEs:
  //S1ap-MME-UE-S1AP-ID
  //S1ap-ENB-UE-S1AP-ID
  //S1ap-E-RABToBeSwitchedULList

  pathSwitchRequest_p = &message->msg.s1ap_PathSwitchRequestIEs;
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (pathSwitchRequest_p->eNB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);
  mme_ue_s1ap_id = pathSwitchRequest_p->sourceMME_UE_S1AP_ID;
  OAILOG_DEBUG (LOG_S1AP, "Path Switch Request message received from eNB UE S1AP ID: " ENB_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) == NULL) {
    /*
     * The MME UE S1AP ID provided by eNB doesn't point to any valid UE.
     * * * * MME replies with a PATH SWITCH REQUEST FAILURE message and start operation
     * * * * as described in TS 36.413 [11].
     * * * * TODO
     */
    OAILOG_DEBUG (LOG_S1AP, "MME UE S1AP ID provided by eNB doesn't point to any valid UE: " MME_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id);
    s1ap_send_path_switch_request_failure(assoc_id, stream, mme_ue_s1ap_id, enb_ue_s1ap_id);
  } else {
    /** The enb_ue_s1ap_id will change! **/
    OAILOG_DEBUG (LOG_S1AP, "Removed old ue_reference before handover for MME UE S1AP ID " MME_UE_S1AP_ID_FMT "\n", (uint32_t) ue_ref_p->mme_ue_s1ap_id);
    s1ap_remove_ue (ue_ref_p);

    /*
     * This UE eNB Id has cu1rrently no known s1 association.
     * * * * Create new UE context by associating new mme_ue_s1ap_id.
     * * * * Update eNB UE list.
     * * * * Forward message to NAS.
     */
    if ((ue_ref_p = s1ap_new_ue (assoc_id, enb_ue_s1ap_id)) == NULL) {
        // If we failed to allocate a new UE return -1
        OAILOG_ERROR (LOG_S1AP, "S1AP:Initial UE Message- Failed to allocate S1AP UE Context, eNBUeS1APId:" ENB_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id);
        OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
    }

    ue_ref_p->enb_ue_s1ap_id = enb_ue_s1ap_id;
    // Will be allocated by NAS
    ue_ref_p->mme_ue_s1ap_id = mme_ue_s1ap_id;

    ue_ref_p->s1ap_ue_context_rel_timer.id  = S1AP_TIMER_INACTIVE_ID;
    ue_ref_p->s1ap_ue_context_rel_timer.sec = S1AP_UE_CONTEXT_REL_COMP_TIMER;

    ue_ref_p->s1ap_handover_completion_timer.id  = S1AP_TIMER_INACTIVE_ID;
    ue_ref_p->s1ap_handover_completion_timer.sec = S1AP_HANDOVER_COMPLETION_TIMER;

    // On which stream we received the message
    ue_ref_p->sctp_stream_recv = stream;
    ue_ref_p->sctp_stream_send = ue_ref_p->enb->next_sctp_stream;
    // CGI mandatory IE
    DevAssert (pathSwitchRequest_p->eutran_cgi.pLMNidentity.size == 3);
    TBCD_TO_PLMN_T(&pathSwitchRequest_p->eutran_cgi.pLMNidentity, &ecgi.plmn);
    BIT_STRING_TO_CELL_IDENTITY (&pathSwitchRequest_p->eutran_cgi.cell_ID, ecgi.cell_identity);
    /** Set the ENB Id. */
    ecgi.cell_identity.enb_id = ue_ref_p->enb->enb_id;

    /*
     * Increment the sctp stream for the eNB association.
     * If the next sctp stream is >= instream negociated between eNB and MME, wrap to first stream.
     * TODO: search for the first available stream instead.
     */

    /*
     * TODO task#15456359.
     * Below logic seems to be incorrect , revisit it.
     */
    ue_ref_p->enb->next_sctp_stream += 1;
    if (ue_ref_p->enb->next_sctp_stream >= ue_ref_p->enb->instreams) {
      ue_ref_p->enb->next_sctp_stream = 1;
    }

    /** Set the new association and the new stream. */
    ue_ref_p->enb->sctp_assoc_id = assoc_id;
    ue_ref_p->enb->next_sctp_stream = stream;

    // set the new enb ue id
    ue_ref_p->enb_ue_s1ap_id = enb_ue_s1ap_id;

    message_p = itti_alloc_new_message (TASK_S1AP, S1AP_PATH_SWITCH_REQUEST);
    AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
    memset ((void *)&message_p->ittiMsg.s1ap_path_switch_request, 0, sizeof (itti_s1ap_path_switch_request_t));
    /*
     * Bad, very bad cast...
     */
    itti_s1ap_path_switch_request_t * path_switch_request = NULL;
    path_switch_request = &message_p->ittiMsg.s1ap_path_switch_request;
    path_switch_request->bcs_to_be_modified.num_bearer_context = pathSwitchRequest_p->e_RABToBeSwitchedDLList.s1ap_E_RABToBeSwitchedDLItem.count;
    for (int item = 0; item < pathSwitchRequest_p->e_RABToBeSwitchedDLList.s1ap_E_RABToBeSwitchedDLItem.count; item++) {
      /*
       * Bad, very bad cast...
       */
      eRABToBeSwitchedDlItemIEs_p = (S1ap_E_RABToBeSwitchedDLItemIEs_t *)pathSwitchRequest_p->e_RABToBeSwitchedDLList.s1ap_E_RABToBeSwitchedDLItem.array[item];
      path_switch_request->bcs_to_be_modified.bearer_contexts[item].eps_bearer_id = eRABToBeSwitchedDlItemIEs_p->e_RABToBeSwitchedDLItem.e_RAB_ID;
      path_switch_request->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.teid = htonl (*((uint32_t *) eRABToBeSwitchedDlItemIEs_p->e_RABToBeSwitchedDLItem.gTP_TEID.buf));
      bstring transport_address = blk2bstr(eRABToBeSwitchedDlItemIEs_p->e_RABToBeSwitchedDLItem.transportLayerAddress.buf, eRABToBeSwitchedDlItemIEs_p->e_RABToBeSwitchedDLItem.transportLayerAddress.size);
      /** Set the IP address from the FTEID. */
      if (4 == blength(transport_address)) {
        path_switch_request->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv4 = 1;
        memcpy(&path_switch_request->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv4_address,
            transport_address->data, blength(transport_address));
      } else if (16 == blength(transport_address)) {
        path_switch_request->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv6 = 1;
        memcpy(&path_switch_request->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv6_address,
            transport_address->data, blength(transport_address));
      } else {
        AssertFatal(0, "TODO IP address %d bytes", blength(transport_address));
      }
      bdestroy_wrapper(&transport_address);
    }

    path_switch_request->mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
    path_switch_request->enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
    path_switch_request->sctp_assoc_id  = assoc_id;
    path_switch_request->sctp_stream    = stream;
    path_switch_request->enb_id         = ue_ref_p->enb->enb_id;
    path_switch_request->e_utran_cgi    = ecgi;
    MSC_LOG_TX_MESSAGE (MSC_S1AP_MME,
        MSC_MMEAPP_MME,
        NULL, 0,
        "0 S1AP_PATH_SWITCH_REQUEST mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ebi %u s1u enb teid %u",
        path_switch_request->mme_ue_s1ap_id,
        path_switch_request->bcs_to_be_modified.bearer_contexts[0].eps_bearer_id,
        path_switch_request->bcs_to_be_modified.bearer_contexts[0].s1_eNB_fteid.teid);
    rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
  }
}

static
  int
s1ap_send_path_switch_request_failure (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    const mme_ue_s1ap_id_t mme_ue_s1ap_id,
    const enb_ue_s1ap_id_t enb_ue_s1ap_id)
{
  int                                     enc_rval = 0;
  S1ap_PathSwitchRequestFailureIEs_t     *pathSwitchRequestFailure_p = NULL;
  s1ap_message                            message = { 0 };
  uint8_t                                *buffer = NULL;
  MessagesIds                             message_id = MESSAGES_ID_MAX;
  uint32_t                                length = 0;
  int                                     rc = RETURNok;

  /*
   * Mandatory IEs:
   * S1ap-MME-UE-S1AP-ID
   * S1ap-ENB-UE-S1AP-ID
   * S1ap-Cause
   */

  OAILOG_FUNC_IN (LOG_S1AP);

  pathSwitchRequestFailure_p = &message.msg.s1ap_PathSwitchRequestFailureIEs;
  s1ap_mme_set_cause(&pathSwitchRequestFailure_p->cause, S1ap_Cause_PR_misc, 4);
  pathSwitchRequestFailure_p->eNB_UE_S1AP_ID = enb_ue_s1ap_id;
  pathSwitchRequestFailure_p->mme_ue_s1ap_id = mme_ue_s1ap_id;

  message.procedureCode = S1ap_ProcedureCode_id_PathSwitchRequest;
  message.direction = S1AP_PDU_PR_unsuccessfulOutcome;
  enc_rval = s1ap_mme_encode_pdu (&message, &message_id, &buffer, &length);

  // Failed to encode
  if (enc_rval < 0) {
    OAILOG_ERROR (LOG_S1AP, "Error encoding path switch request failure.\n");
//    free_s1ap_pathswitchrequestfailure(pathSwitchRequestFailure_p);
  }

  bstring b = blk2bstr(buffer, length);
  free(buffer);
  s1ap_free_mme_encode_pdu(&message, message_id);
  rc = s1ap_mme_itti_send_sctp_request (&b, assoc_id, 0, INVALID_MME_UE_S1AP_ID);

//  free_s1ap_pathswitchrequestfailure(pathSwitchRequestFailure_p);

  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

int
s1ap_mme_handle_handover_preparation(const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
                                        struct s1ap_message_s *message)                                         {
  S1ap_HandoverRequiredIEs_t            *handoverRequired_p = NULL;
  S1ap_E_RABInformationListIEs_t        *eRABInformationListIEs_p = NULL;

  ue_description_t                      *ue_ref_p = NULL;
  enb_ue_s1ap_id_t                       enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                       mme_ue_s1ap_id = 0;
  MessageDef                            *message_p = NULL;
  int                                    rc = RETURNok;
  tai_t                                  target_tai;

  //Request IEs:
  //S1ap-ENB-UE-S1AP-ID
  //S1ap-HandoverType
  //S1ap-MME-UE-S1AP-ID
  //S1ap-Cause
  //S1ap-TargetID
  //S1ap-Source-ToTarget-TransparentContainer

  OAILOG_FUNC_IN (LOG_S1AP);
  handoverRequired_p = &message->msg.s1ap_HandoverRequiredIEs;
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (handoverRequired_p->eNB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);
  mme_ue_s1ap_id = (uint32_t) handoverRequired_p->mme_ue_s1ap_id;

  OAILOG_DEBUG (LOG_S1AP, "Handover Required message received from eNB UE S1AP ID: " ENB_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) == NULL) {
    /**
     * The MME UE S1AP ID provided by eNB doesn't point to any valid UE.
     * * * * MME replies with a HANDOVER PREPARATION FAILURE message and start operation.
     * * * * as described in TS 36.413 [11].
     */
    OAILOG_ERROR(LOG_S1AP, "MME UE S1AP ID provided by source eNB doesn't point to any valid UE: " MME_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id);
    rc = s1ap_handover_preparation_failure(assoc_id, mme_ue_s1ap_id, enb_ue_s1ap_id, S1ap_Cause_PR_misc);
    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  }
  /*
   * No timers to be started. Also, the state does not need to be changed from UE_CONNECTED, since TS 36.413 8.4.1.2 specifies that MME may/can initiate some E-RAB Modification signaling towards the source
   * eNB before the Handover Procedure is completed. Handover restart won't be triggered by eNB.
   * Source eNB may restart the Handover. If the UE is still connected (Handover dismissed or still going on, it will send a HO-Preparation-Failure.
   */
  if(ue_ref_p->s1_ue_state  != S1AP_UE_CONNECTED){
    OAILOG_ERROR(LOG_S1AP, "UE: " MME_UE_S1AP_ID_FMT " is not in connected mode, instead %d \n", enb_ue_s1ap_id, ue_ref_p->s1_ue_state);
    rc = s1ap_handover_preparation_failure(assoc_id, mme_ue_s1ap_id, enb_ue_s1ap_id, S1ap_Cause_PR_misc);
    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  }
  /** Get the target tai information. */
  if(!handoverRequired_p->targetID.choice.targeteNB_ID.selected_TAI.pLMNidentity.buf || !handoverRequired_p->targetID.choice.targeteNB_ID.selected_TAI.tAC.buf
      || !handoverRequired_p->targetID.choice.targeteNB_ID.global_ENB_ID.pLMNidentity.buf ){
    OAILOG_ERROR(LOG_S1AP, "Handover Required message for UE: " MME_UE_S1AP_ID_FMT " has no target_tai or enb_id information. \n", enb_ue_s1ap_id);
    rc = s1ap_handover_preparation_failure(assoc_id, mme_ue_s1ap_id, enb_ue_s1ap_id, S1ap_Cause_PR_misc);
    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  }
  memset((void*)&target_tai, 0, sizeof(tai_t));

  TBCD_TO_PLMN_T (&handoverRequired_p->targetID.choice.targeteNB_ID.selected_TAI.pLMNidentity, &target_tai.plmn);
  target_tai.tac = htons(*((uint16_t *) handoverRequired_p->targetID.choice.targeteNB_ID.selected_TAI.tAC.buf));

  /** Not changing any ue_reference properties. */
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_HANDOVER_REQUIRED);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");

  S1AP_HANDOVER_REQUIRED (message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
  S1AP_HANDOVER_REQUIRED (message_p).enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
  /** Selected-TAI. */
  S1AP_HANDOVER_REQUIRED (message_p).selected_tai  = target_tai;

  /** Set SCTP Assoc Id. */
  S1AP_HANDOVER_REQUIRED (message_p).sctp_assoc_id  = assoc_id;

  /** Global-ENB-ID. */
  plmn_t enb_id_plmn;
  memset(&enb_id_plmn, 0, sizeof(plmn_t));
  TBCD_TO_PLMN_T (&handoverRequired_p->targetID.choice.targeteNB_ID.global_ENB_ID.pLMNidentity, &enb_id_plmn);
  S1AP_HANDOVER_REQUIRED (message_p).global_enb_id.plmn = enb_id_plmn;

  // Macro eNB = 20 bits
  /** Get the enb id. */
  uint32_t enb_id = 0;
  if(handoverRequired_p->targetID.choice.targeteNB_ID.global_ENB_ID.eNB_ID.present == S1ap_ENB_ID_PR_homeENB_ID){
    /** Target is a home eNB Id. */
    // Home eNB ID = 28 bits
    uint8_t *enb_id_buf = handoverRequired_p->targetID.choice.targeteNB_ID.global_ENB_ID.eNB_ID.choice.homeENB_ID.buf;
    if (handoverRequired_p->targetID.choice.targeteNB_ID.global_ENB_ID.eNB_ID.choice.homeENB_ID.size != 28) {
      //TODO: handle case were size != 28 -> notify ? reject ?
    }
    enb_id = (enb_id_buf[0] << 20) + (enb_id_buf[1] << 12) + (enb_id_buf[2] << 4) + ((enb_id_buf[3] & 0xf0) >> 4);
    S1AP_HANDOVER_REQUIRED (message_p).target_enb_type = TARGET_ID_HOME_ENB_ID;
    //    OAILOG_MESSAGE_ADD (context, "home eNB id: %07x", target_enb_id);
  }else{
    /** Target is a macro eNB Id (Macro eNB = 20 bits). */
    uint8_t *enb_id_buf = handoverRequired_p->targetID.choice.targeteNB_ID.global_ENB_ID.eNB_ID.choice.macroENB_ID.buf;
    if (handoverRequired_p->targetID.choice.targeteNB_ID.global_ENB_ID.eNB_ID.choice.macroENB_ID.size != 20) {
      //TODO: handle case were size != 28 -> notify ? reject ?
    }
    enb_id = (enb_id_buf[0] << 12) + (enb_id_buf[1] << 4) + ((enb_id_buf[2] & 0xf0) >> 4);
    //    OAILOG_MESSAGE_ADD (context, "macro eNB id: %05x", target_enb_id);
    S1AP_HANDOVER_REQUIRED (message_p).target_enb_type = TARGET_ID_MACRO_ENB_ID;
  }

  S1AP_HANDOVER_REQUIRED (message_p).global_enb_id.cell_identity.enb_id = enb_id;
  OAILOG_DEBUG(LOG_S1AP, "Successfully decoded targetID IE in Handover Required. \n");

  /**  Get the cause. */
  S1ap_Cause_PR s1ap_cause_present = handoverRequired_p->cause.present;
  switch(s1ap_cause_present){
  case S1ap_Cause_PR_radioNetwork:
    S1AP_HANDOVER_REQUIRED (message_p).f_cause_type      = s1ap_cause_present;
    S1AP_HANDOVER_REQUIRED (message_p).f_cause_value     = handoverRequired_p->cause.choice.radioNetwork;
    break;
  default:
    OAILOG_WARNING(LOG_S1AP, "Cause type %d is not evaluated. Setting cause to radio.. \n", s1ap_cause_present);
    S1AP_HANDOVER_REQUIRED (message_p).f_cause_type      = S1ap_Cause_PR_radioNetwork;
    S1AP_HANDOVER_REQUIRED (message_p).f_cause_value     = 16;
    break;
  }
  /**
   * Set the container into a bstring.
   * todo:
   * We don't need to purge the bstring's, etc in MME_APP (of the newly allocated itti). Since it will automatically be purged with: s1ap_free_mme_decode_pdu.
   * All allocated messages will be purged, so either allocated new into bstring's or copy into stacked parameters.
   */
  S1AP_HANDOVER_REQUIRED (message_p).eutran_source_to_target_container = blk2bstr(handoverRequired_p->source_ToTarget_TransparentContainer.buf,
        handoverRequired_p->source_ToTarget_TransparentContainer.size);

//  OCTET_STRING_fromBuf (&S1AP_HANDOVER_REQUIRED (message_p).transparent_container,
//      (char *)(handoverRequired_p->source_ToTarget_TransparentContainer.buf),
//      handoverRequired_p->source_ToTarget_TransparentContainer.size);
  // todo: no need to invoke the freer for the octet string?
  // todo: what does ASM_FREE_CONTEXT_ONLY mean?
//  free_wrapper((void**) &(handover_required_pP->transparent_container.buf));

  /**
   * The transparent container bstr must be deallocated later.
   * No automatic deallocation function for that is present.
   */
  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME,
      MSC_MMEAPP_MME,
      NULL, 0,
      "0 S1AP_HANDOVER_REQUIRED mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT,
      S1AP_HANDOVER_REQUIRED (message_p).mme_ue_s1ap_id);
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

int
s1ap_mme_handle_handover_cancel(const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
                                        struct s1ap_message_s *message)                                         {
  S1ap_HandoverCancelIEs_t            *handoverCancel_p = NULL;

  ue_description_t                     *ue_ref_p = NULL;
  enb_ue_s1ap_id_t                      enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                      mme_ue_s1ap_id = 0;
  MessageDef                           *message_p = NULL;
  int                                   rc = RETURNok;

  //Request IEs:
  //S1ap-ENB-UE-S1AP-ID
  //S1ap-MME-UE-S1AP-ID
  //S1ap-Cause

  OAILOG_FUNC_IN (LOG_S1AP);
  handoverCancel_p = &message->msg.s1ap_HandoverCancelIEs;
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (handoverCancel_p->eNB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);
  mme_ue_s1ap_id = (uint32_t) handoverCancel_p->mme_ue_s1ap_id;

  OAILOG_DEBUG (LOG_S1AP, "Handover Cancel message received from eNB UE S1AP ID: " ENB_UE_S1AP_ID_FMT ". \n", enb_ue_s1ap_id);
  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) == NULL) {
    /*
     * The MME UE S1AP ID provided by eNB doesn't point to any valid UE.
     * * * * MME replies with a HANDOVER PREPARATION FAILURE message and start operation.
     * * * * as described in TS 36.413 [11].
     */
    OAILOG_ERROR(LOG_S1AP, "MME UE S1AP ID provided by source eNB doesn't point to any valid UE: " MME_UE_S1AP_ID_FMT ". Continuing with the HO-Cancellation (impl. detach). \n", enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }
  /**
   * No timers to be started. Also, the state does not need to be changed from UE_CONNECTED, since TS 36.413 8.4.1.2 specifies that MME may/can initiate some E-RAB Modification signaling towards the source
   * eNB before the Handover Procedure is completed. Handover restart won't be triggered by eNB.
   * Source eNB may restart the Handover. If the UE is still connected (Handover dismissed or still going on, it will send a HO-Preparation-Failure.
   */
  if(ue_ref_p->s1_ue_state  != S1AP_UE_CONNECTED){
    OAILOG_ERROR(LOG_S1AP, "UE: " MME_UE_S1AP_ID_FMT " is not in connected mode, instead %d. Continuing with cancellation (impl. detach). \n", enb_ue_s1ap_id, ue_ref_p->s1_ue_state);
    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  }
  /** Not changing any ue_reference properties. */
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_HANDOVER_CANCEL);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  memset ((void *)&message_p->ittiMsg.s1ap_handover_cancel, 0, sizeof (itti_s1ap_handover_cancel_t));
  S1AP_HANDOVER_CANCEL (message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
  S1AP_HANDOVER_CANCEL (message_p).enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
  S1AP_HANDOVER_CANCEL (message_p).assoc_id = ue_ref_p->enb->sctp_assoc_id;
  /** Ignore the cause. */
  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME,
      MSC_MMEAPP_MME,
      NULL, 0,
      "0 S1AP_HANDOVER_CANCEL mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT,
      S1AP_HANDOVER_CANCEL (message_p).mme_ue_s1ap_id);
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

int
s1ap_mme_handle_handover_notification(const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
                                        struct s1ap_message_s *message)                                         {
  S1ap_HandoverNotifyIEs_t              *handoverNotification_p = NULL;

  ue_description_t                      *ue_ref_p = NULL;
  enb_ue_s1ap_id_t                       enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                       mme_ue_s1ap_id = 0;
  MessageDef                            *message_p = NULL;
  int                                    rc = RETURNok;
  tai_t                                  tai = {.plmn = {0}, .tac = INVALID_TAC_0000};
  ecgi_t                                 ecgi = {.plmn = {0}, .cell_identity = {0}};

  //  todo: The MME shall verify that the UE security
  //  capabilities received from the eNB are the same as the UE security capabilities that the MME has stored. If
  //  there is a mismatch, the MME may log the event and may take additional measures, such as raising an alarm.

  //Request IEs:
  //S1ap-ENB-UE-S1AP-ID
  //S1ap-MME-UE-S1AP-ID
  //S1ap-id-TAI
  //S1ap-id-EUTRAN-CGI

  OAILOG_FUNC_IN (LOG_S1AP);
  handoverNotification_p= &message->msg.s1ap_HandoverNotifyIEs;
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (handoverNotification_p->eNB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);

  OAILOG_DEBUG (LOG_S1AP, "Handover Notify message received from eNB UE S1AP ID: " ENB_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id);
  /**
   * Search the UE via the ENB-ID and the ENB_UE_S1AP_ID.
   * The MME_UE_S1AP_ID will not be registered with the UE_REFERECNCE until HANDOVER_NOTIFY is completed.
   */
  enb_description_t * enb_ref = s1ap_is_enb_assoc_id_in_list (assoc_id);
  DevAssert(enb_ref);
  if ((ue_ref_p = s1ap_is_enb_ue_s1ap_id_in_list_per_enb(enb_ue_s1ap_id, enb_ref->enb_id)) == NULL) {
    /*
     * The MME UE S1AP ID provided by eNB doesn't point to any valid UE.
     * * * * MME replies with a PATH SWITCH REQUEST FAILURE message and start operation
     * * * * as described in TS 36.413 [11].
     * * * * TODO
     */
    OAILOG_ERROR(LOG_S1AP, "ENB UE S1AP ID " ENB_UE_S1AP_ID_FMT " provided by target eNB %d doesn't point to any valid UE. \n", enb_ue_s1ap_id, enb_ref->enb_id);
    /**
     * Wait for the S10 Relocation Timer to run up or trigger a cancel by the source side.
     * Nothing extra done in this case.
     */
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }
  /** Not changing any ue_reference properties. The MME_APP layer will eventually stop the S10_HANDOVER_RELOCATION TIMER. */
  /** Get the TAI and the eCGI: Values that are sent with Attach Request. */
  OCTET_STRING_TO_TAC (&handoverNotification_p->tai.tAC, tai.tac);
  DevAssert (handoverNotification_p->tai.pLMNidentity.size == 3);
  TBCD_TO_PLMN_T(&handoverNotification_p->tai.pLMNidentity, &tai.plmn);

  DevAssert (handoverNotification_p->eutran_cgi.pLMNidentity.size == 3);
  TBCD_TO_PLMN_T(&handoverNotification_p->eutran_cgi.pLMNidentity, &ecgi.plmn);
  BIT_STRING_TO_CELL_IDENTITY (&handoverNotification_p->eutran_cgi.cell_ID, ecgi.cell_identity);
  /** Set the ENB Id. */
  ecgi.cell_identity.enb_id = enb_ref->enb_id;

  // Currently just use this message send the MBR to SAE-GW and inform the source MME
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_HANDOVER_NOTIFY);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  memset ((void *)&message_p->ittiMsg.s1ap_handover_notify, 0, sizeof (itti_s1ap_handover_notify_t));
  /*
   * Bad, very bad cast...
   */
  S1AP_HANDOVER_NOTIFY (message_p).mme_ue_s1ap_id = (mme_ue_s1ap_id_t) handoverNotification_p->mme_ue_s1ap_id;
  S1AP_HANDOVER_NOTIFY (message_p).enb_ue_s1ap_id = enb_ue_s1ap_id;
  S1AP_HANDOVER_NOTIFY (message_p).tai                    = tai;       /**< MME will not check if thats the correct target eNB. Just update the UE Context. */
  S1AP_HANDOVER_NOTIFY (message_p).cgi                    = ecgi;
  S1AP_HANDOVER_NOTIFY (message_p).assoc_id               = assoc_id;

  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME,
      MSC_MMEAPP_MME,
      NULL, 0,
      "0 S1AP_HANDOVER_NOTIFY mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT,
      S1AP_HANDOVER_NOTIFY(message_p).mme_ue_s1ap_id);
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_handover_resource_allocation_response(const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
                                        struct s1ap_message_s *message)                                         {

  S1ap_HandoverRequestAcknowledgeIEs_t     *handoverRequestAcknowledgeIEs_p = NULL;
  S1ap_E_RABAdmittedItemIEs_t              *eRABAdmittedItemIEs_p = NULL;
//  S1ap_E_RABSetupItemCtxtSURes_t         *eRABSetupItemCtxtSURes_p = NULL;

  ue_description_t                         *ue_ref_p = NULL;
  enb_ue_s1ap_id_t                          enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                          mme_ue_s1ap_id = 0;
  MessageDef                               *message_p = NULL;
  int                                       rc = RETURNok;

  //  todo: The MME shall verify that the UE security
  //  capabilities received from the eNB are the same as the UE security capabilities that the MME has stored. If
  //  there is a mismatch, the MME may log the event and may take additional measures, such as raising an alarm.

  //Request IEs:
  //S1ap-ENB-UE-S1AP-ID
  //S1ap-E-RABToBeSwitchedDLList
  //S1ap-MME-UE-S1AP-ID
  //S1ap-EUTRAN-CGI
  //S1ap-TAI
  //S1ap-UESecurityCapabilities

  //Acknowledge IEs:
  //S1ap-MME-UE-S1AP-ID
  //S1ap-ENB-UE-S1AP-ID
  //S1ap-E-RABToBeSwitchedULList

  OAILOG_FUNC_IN (LOG_S1AP);
  handoverRequestAcknowledgeIEs_p = &message->msg.s1ap_HandoverRequestAcknowledgeIEs;
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (handoverRequestAcknowledgeIEs_p->eNB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);
  mme_ue_s1ap_id = handoverRequestAcknowledgeIEs_p->mme_ue_s1ap_id;
  OAILOG_DEBUG (LOG_S1AP, "Handover Request Acknowledge received from eNB UE S1AP ID: " ENB_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id);

  /** Will return NULL if an UE-Reference already exists in the UE_MAP of the target-eNB.
   * This UE eNB Id has currently no known s1 association.
   * * * * Create new UE Reference. Associate in the MME_APP to the new mme_ue_s1ap_id.
   * * * * Update eNB UE list.
   * * * * Forward message to NAS.
   */

  enb_description_t * enb_ref = s1ap_is_enb_assoc_id_in_list (assoc_id);
  if(enb_ref == NULL){
    OAILOG_ERROR (LOG_S1AP, "S1AP:Handover Request Message- Failed to get eNB-Reference for assocId: %d. \n", assoc_id);
     /** This is a SYSTEM_FAILURE. No UE_REFERENCE for the target-ENB exists. */
     message_p = itti_alloc_new_message (TASK_S1AP, S1AP_HANDOVER_FAILURE);
     AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
     itti_s1ap_handover_failure_t *handover_failure_p = &message_p->ittiMsg.s1ap_handover_failure;
     memset ((void *)&message_p->ittiMsg.s1ap_handover_failure, 0, sizeof (itti_s1ap_handover_failure_t));
     /** Fill the S1AP Handover Failure elements per hand. */
     handover_failure_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
//     handover_failure_p->enb_ue_s1ap_id = enb_ue_s1ap_id; /**< Set it from the invalid context. */
     handover_failure_p->assoc_id       = assoc_id; /**< Set it from the invalid context. */
     /**
      * Set it to S1AP_SYSTEM_FAILURE for the invalid context. Not waiting for RELEASE-COMPLETE from target-eNB.
      * It may remove the sctp association to the correct MME_UE_S1AP_ID, too. For that, to continue with the handover fail case,
      * to reach the source-ENB, the MME_APP has to re-notify the MME_UE_S1AP_ID/SCTP association.
      */
     handover_failure_p->cause          = S1AP_SYSTEM_FAILURE;
     MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_MMEAPP_MME, NULL, 0, "0 Sending manually S1AP_HANDOVER_FAILURE for mme_ue_s1ap_id  " MME_UE_S1AP_ID_FMT " ", mme_ue_s1ap_id);
     itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
     OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if ((ue_ref_p = s1ap_new_ue (assoc_id, enb_ue_s1ap_id)) == NULL) {
    OAILOG_ERROR (LOG_S1AP, "S1AP:Handover Request Message- Failed to allocate S1AP UE Context, mmeUeS1APId:" MME_UE_S1AP_ID_FMT "\n", mme_ue_s1ap_id);
    /** This is a SYSTEM_FAILURE. No UE_REFERENCE for the target-ENB exists. */
    message_p = itti_alloc_new_message (TASK_S1AP, S1AP_HANDOVER_FAILURE);
    AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
    itti_s1ap_handover_failure_t *handover_failure_p = &message_p->ittiMsg.s1ap_handover_failure;
    memset ((void *)&message_p->ittiMsg.s1ap_handover_failure, 0, sizeof (itti_s1ap_handover_failure_t));
    /** Fill the S1AP Handover Failure elements per hand. */
    handover_failure_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
//    handover_failure_p->enb_ue_s1ap_id = enb_ue_s1ap_id; /**< Set it from the invalid context. */
    handover_failure_p->assoc_id       = assoc_id; /**< Set it from the invalid context. */
    /**
     * Set it to S1AP_SYSTEM_FAILURE for the invalid context. Not waiting for RELEASE-COMPLETE from target-eNB.
     * It may remove the sctp association to the correct MME_UE_S1AP_ID, too. For that, to continue with the handover fail case,
     * to reach the source-ENB, the MME_APP has to re-notify the MME_UE_S1AP_ID/SCTP association.
     */
    handover_failure_p->cause          = S1AP_SYSTEM_FAILURE;
    MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_MMEAPP_MME, NULL, 0, "0 Sending manually S1AP_HANDOVER_FAILURE for mme_ue_s1ap_id  " MME_UE_S1AP_ID_FMT " ", mme_ue_s1ap_id);
    itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
    itti_s1ap_ue_context_release_command_t *ue_context_release_cmd_p = &((itti_s1ap_ue_context_release_command_t){ .enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id, .enb_id = ue_ref_p->enb->enb_id, .cause = S1AP_SYSTEM_FAILURE});
    /** Remove the UE-Reference to the target-eNB implicitly. Don't need to wait for the UE_CONTEXT_REMOVAL_COMMAND_COMPLETE. */
    s1ap_handle_ue_context_release_command(ue_context_release_cmd_p); /**< Send a removal message and remove the context also directly. */
    // There are some race conditions were NAS T3450 timer is stopped and removed at same time
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }
  /**
   * Successfully created a UE-Reference to the target-ENB.
   * Don't register the MME_UE_S1AP_ID to SCTP_ASSOC association yet.
   * Do it with HANDOVER_NOTIFY.
   */
  if (handoverRequestAcknowledgeIEs_p->e_RABAdmittedList.s1ap_E_RABAdmittedItem.count < 1) {
    OAILOG_ERROR(LOG_S1AP, "E-RAB's where not addmitted for S1AP handover. \n");
    OAILOG_ERROR (LOG_S1AP, "S1AP:Handover Request Message- Failed to allocate S1AP UE Context, mmeUeS1APId:" MME_UE_S1AP_ID_FMT "\n", mme_ue_s1ap_id);
    /** This is a SYSTEM_FAILURE. No UE_REFERENCE for the target-ENB exists. */
    message_p = itti_alloc_new_message (TASK_S1AP, S1AP_HANDOVER_FAILURE);
    AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
    itti_s1ap_handover_failure_t *handover_failure_p = &message_p->ittiMsg.s1ap_handover_failure;
    memset ((void *)&message_p->ittiMsg.s1ap_handover_failure, 0, sizeof (itti_s1ap_handover_failure_t));
    /** Fill the S1AP Handover Failure elements per hand. */
    handover_failure_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
//    handover_failure_p->enb_ue_s1ap_id = enb_ue_s1ap_id; /**< Set it from the invalid context. */
    handover_failure_p->assoc_id       = assoc_id; /**< Set it from the invalid context. */
    /**
     * Set it to S1AP_SYSTEM_FAILURE for the invalid context. Not waiting for RELEASE-COMPLETE from target-eNB.
     * It may remove the sctp association to the correct MME_UE_S1AP_ID, too. For that, to continue with the handover fail case,
     * to reach the source-ENB, the MME_APP has to re-notify the MME_UE_S1AP_ID/SCTP association (S1 association).
     */
    handover_failure_p->cause          = S1AP_SYSTEM_FAILURE;
    MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_MMEAPP_MME, NULL, 0, "0 Sending manually S1AP_HANDOVER_FAILURE for mme_ue_s1ap_id  " MME_UE_S1AP_ID_FMT " ", mme_ue_s1ap_id);
    itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
    itti_s1ap_ue_context_release_command_t *ue_context_release_cmd_p = &((itti_s1ap_ue_context_release_command_t){ .enb_ue_s1ap_id = enb_ue_s1ap_id, .enb_id = enb_ref->enb_id, .cause = S1AP_SYSTEM_FAILURE});
    /** Remove the UE-Reference to the target-eNB implicitly. Don't need to wait for the UE_CONTEXT_REMOVAL_COMMAND_COMPLETE. */
    s1ap_handle_ue_context_release_command(ue_context_release_cmd_p); /**< Send a removal message and remove the context also directly. */
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
  }

  /**
   * Only create a new UE_REFERENCE!Set the (new) ue_reference to S1AP_UE_HANDOVER state.
   * todo: not setting to this state if intra-MME S1AP handover is being done.
   */
  // Will be allocated by NAS
  // todo: searching ue_reference just by enb_ue_s1ap id or mme_ue_s1ap_id ?

  ue_ref_p->s1ap_ue_context_rel_timer.id  = S1AP_TIMER_INACTIVE_ID;
  ue_ref_p->s1ap_ue_context_rel_timer.sec = S1AP_UE_CONTEXT_REL_COMP_TIMER;

  ue_ref_p->s1ap_handover_completion_timer.id  = S1AP_TIMER_INACTIVE_ID;
  ue_ref_p->s1ap_handover_completion_timer.sec = S1AP_HANDOVER_COMPLETION_TIMER;

  // On which stream we received the message
  ue_ref_p->sctp_stream_recv = stream;
  /**
   * todo: Lionel: this (if no run condition occurs= should be the same stream where we sent the HANDOVER_REQUEST... might not be if run-condition occurs!
   */
  ue_ref_p->sctp_stream_send = ue_ref_p->enb->next_sctp_stream;

  /*
   * Increment the sctp stream for the eNB association.
   * If the next sctp stream is >= instream negociated between eNB and MME, wrap to first stream.
   * TODO: search for the first available stream instead.
   */

  /*
   * TODO task#15456359.
   * Below logic seems to be incorrect , revisit it.
   *
   * todo: always setting to 0 might work?!
   */
  ue_ref_p->enb->next_sctp_stream += 1;
  if (ue_ref_p->enb->next_sctp_stream >= ue_ref_p->enb->instreams) {
    ue_ref_p->enb->next_sctp_stream = 1;
  }
  s1ap_dump_enb (ue_ref_p->enb);
  /** UE Reference will be in IDLE state. */

  /**
   * E-RAB Admitted List.
   */
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_HANDOVER_REQUEST_ACKNOWLEDGE);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  memset ((void *)&message_p->ittiMsg.s1ap_handover_request_acknowledge, 0, sizeof (itti_s1ap_handover_request_acknowledge_t));
  /*
   * Bad, very bad cast...
   */
  eRABAdmittedItemIEs_p = (S1ap_E_RABAdmittedItemIEs_t *)
    handoverRequestAcknowledgeIEs_p->e_RABAdmittedList.s1ap_E_RABAdmittedItem.array[0];

  S1AP_HANDOVER_REQUEST_ACKNOWLEDGE (message_p).mme_ue_s1ap_id = handoverRequestAcknowledgeIEs_p->mme_ue_s1ap_id;
  S1AP_HANDOVER_REQUEST_ACKNOWLEDGE (message_p).enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;

  itti_s1ap_handover_request_acknowledge_t * s1ap_handover_request_acknowledge = NULL;
  s1ap_handover_request_acknowledge = &message_p->ittiMsg.s1ap_handover_request_acknowledge;
  s1ap_handover_request_acknowledge->bcs_to_be_modified.num_bearer_context = handoverRequestAcknowledgeIEs_p->e_RABAdmittedList.s1ap_E_RABAdmittedItem.count;
  for (int item = 0; item < handoverRequestAcknowledgeIEs_p->e_RABAdmittedList.s1ap_E_RABAdmittedItem.count; item++) {
    /*
     * Bad, very bad cast...
     */
    eRABAdmittedItemIEs_p = (S1ap_E_RABAdmittedItemIEs_t *)handoverRequestAcknowledgeIEs_p->e_RABAdmittedList.s1ap_E_RABAdmittedItem.array[item];
    s1ap_handover_request_acknowledge->bcs_to_be_modified.bearer_contexts[item].eps_bearer_id = eRABAdmittedItemIEs_p->e_RABAdmittedItem.e_RAB_ID;
    s1ap_handover_request_acknowledge->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.teid = htonl (*((uint32_t *) eRABAdmittedItemIEs_p->e_RABAdmittedItem.gTP_TEID.buf));
    bstring transport_address  =
        blk2bstr(eRABAdmittedItemIEs_p->e_RABAdmittedItem.transportLayerAddress.buf, eRABAdmittedItemIEs_p->e_RABAdmittedItem.transportLayerAddress.size);
    /** Set the IP address from the FTEID. */
    if (4 == blength(transport_address)) {
      s1ap_handover_request_acknowledge->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv4 = 1;
      memcpy(&s1ap_handover_request_acknowledge->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv4_address,
          transport_address->data, blength(transport_address));
    } else if (16 == blength(transport_address)) {
      s1ap_handover_request_acknowledge->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv6 = 1;
      memcpy(&s1ap_handover_request_acknowledge->bcs_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv6_address,
          transport_address->data,
          blength(transport_address));
    } else {
      AssertFatal(0, "TODO IP address %d bytes", blength(transport_address));
    }
    bdestroy_wrapper(&transport_address);
  }

  /**
   * Set the transparent container as a bstring.
   * The method ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_S1ap_Target_ToSource_TransparentContainer, &s1ap_HandoverRequestAcknowledgeIEs->target_ToSource_TransparentContainer)
   * is assumed to remove the OCTET_STRING.
   * todo: ask Lionel if thats correct? what is context only?
   */
  S1AP_HANDOVER_REQUEST_ACKNOWLEDGE (message_p).target_to_source_eutran_container = blk2bstr((void*)handoverRequestAcknowledgeIEs_p->target_ToSource_TransparentContainer.buf,
      handoverRequestAcknowledgeIEs_p->target_ToSource_TransparentContainer.size);
  // todo @ Lionel: do we need to run this method? Message deallocated later?
  // free_wrapper((void**) &(handover_request_acknowledge_pP->transparent_container.buf));

  ue_ref_p->s1_ue_state = S1AP_UE_CONNECTED;

  /** Not checking if the Target-to-Source Transparent container exists. */
  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME,
                      MSC_MMEAPP_MME,
                      NULL, 0,
                      "0 S1AP_HANDOVER_REQUEST_ACKNOWLEDGE mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ebi %u s1u enb teid %u",
                      S1AP_HANDOVER_REQUEST_ACKNOWLEDGE (message_p).mme_ue_s1ap_id,
                      S1AP_HANDOVER_REQUEST_ACKNOWLEDGE (message_p).eps_bearer_id,
                      S1AP_HANDOVER_REQUEST_ACKNOWLEDGE (message_p).bearer_s1u_enb_fteid.teid);
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

int
s1ap_mme_handle_handover_resource_allocation_failure(const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
                                        struct s1ap_message_s *message)                                         {
  ue_description_t                         *ue_ref_p = NULL;
  enb_ue_s1ap_id_t                          enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                          mme_ue_s1ap_id = 0;
  MessageDef                               *message_p = NULL;
  int                                       rc = RETURNok;
  S1ap_HandoverFailureIEs_t                *handoverFailure_p = NULL;
  long                                      cause_value;
  OAILOG_FUNC_IN (LOG_S1AP);

  /** Get the Failure Message. */
  handoverFailure_p = &message->msg.s1ap_HandoverFailureIEs;

  /**
   * No UE Reference to the target-eNB expected @ Handover Failure. Only will be created to the target eNB with Handover Request Acknowledge.
   * Send Handover Failure back (manually) to the MME.
   * This will trigger an implicit detach if the UE is not REGISTERED yet (single MME S1AP HO), or will just send a HO-PREP-FAILURE to the MME (if the cause is not S1AP-SYSTEM-FAILURE).
   */
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_HANDOVER_FAILURE);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  itti_s1ap_handover_failure_t *handover_failure_p = &message_p->ittiMsg.s1ap_handover_failure;
  memset ((void *)&message_p->ittiMsg.s1ap_handover_failure, 0, sizeof (itti_s1ap_handover_failure_t));
  /** Fill the S1AP Handover Failure elements per hand. */
  handover_failure_p->mme_ue_s1ap_id = handoverFailure_p->mme_ue_s1ap_id;
//  handover_failure_p->enb_ue_s1ap_id = INVALID_ENB_UE_S1AP_ID;
  /** Choice. */
  S1ap_Cause_PR cause_type = handoverFailure_p->cause.present;

  switch (cause_type)
  {
  case S1ap_Cause_PR_radioNetwork:
    cause_value = handoverFailure_p->cause.choice.radioNetwork;
    OAILOG_DEBUG (LOG_S1AP, "HANDOVER_FAILURE with Cause_Type = Radio Network and Cause_Value = %ld\n", cause_value);
    break;

  case S1ap_Cause_PR_transport:
    cause_value = handoverFailure_p->cause.choice.transport;
    OAILOG_DEBUG (LOG_S1AP, "HANDOVER_FAILURE with Cause_Type = Transport and Cause_Value = %ld\n", cause_value);
    break;

  case S1ap_Cause_PR_nas:
    cause_value = handoverFailure_p->cause.choice.nas;
    OAILOG_DEBUG (LOG_S1AP, "HANDOVER_FAILURE with Cause_Type = NAS and Cause_Value = %ld\n", cause_value);
    break;

  case S1ap_Cause_PR_protocol:
    cause_value = handoverFailure_p->cause.choice.protocol;
    OAILOG_DEBUG (LOG_S1AP, "HANDOVER_FAILURE with Cause_Type = Protocol and Cause_Value = %ld\n", cause_value);
    break;

  case S1ap_Cause_PR_misc:
    cause_value = handoverFailure_p->cause.choice.misc;
    OAILOG_DEBUG (LOG_S1AP, "HANDOVER_FAILURE with Cause_Type = MISC and Cause_Value = %ld\n", cause_value);
    break;

  default:
    OAILOG_ERROR (LOG_S1AP, "HANDOVER_FAILURE with Invalid Cause_Type = %d\n", cause_type);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  /**
   * No match function exists between the S1AP Cause and the decoded cause yet.
   * Set it to anything else thatn SYSTEM_FAILURE, not to trigger any implicit detach due cause (only based on the handover type).
   */
  handover_failure_p->cause = S1AP_HANDOVER_FAILED;

  /** No timer to stop on the target-MME side. */
  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_MMEAPP_MME, NULL, 0, "0 Sending manually S1AP_HANDOVER_FAILURE for mme_ue_s1ap_id  " MME_UE_S1AP_ID_FMT " ", handover_failure_p->mme_ue_s1ap_id);
  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  /** No UE context to release. */
  OAILOG_FUNC_OUT (LOG_S1AP);
}

//------------------------------------------------------------------------------
/**
 * ENB Status Transfer.
 * Contains Transparent Data which is forwarded to the target eNB without state change.
 */
int
s1ap_mme_handle_enb_status_transfer(const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
                                        struct s1ap_message_s *message)   {

  S1ap_ENBStatusTransferIEs_t              *enbStatusTransfer_p = NULL;

  ue_description_t                         *ue_ref_p = NULL;
  enb_ue_s1ap_id_t                          enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                          mme_ue_s1ap_id = 0;
  MessageDef                               *message_p = NULL;
  int                                       rc = RETURNok;

  //Request IEs:
  //S1ap-ENB-UE-S1AP-ID
  //S1ap-MME-UE-S1AP-ID
  //S1ap-enb-StatusTransfer-TransparentContainer

  OAILOG_FUNC_IN (LOG_S1AP);
  enbStatusTransfer_p = &message->msg.s1ap_ENBStatusTransferIEs;
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (enbStatusTransfer_p->eNB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);
  mme_ue_s1ap_id = enbStatusTransfer_p->mme_ue_s1ap_id;
  OAILOG_DEBUG (LOG_S1AP, "Enb Status Transfer received from source eNodeB for UE with eNB UE S1AP ID: " ENB_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id);

  /** In the single-MME case as well, the MME_UE_S1AP_ID should still provide the source eNB. */
  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) == NULL) {
    /*
     * The MME UE S1AP ID provided by eNB doesn't point to any valid UE.
     * Since the message is optional, just disregard the message.
     * No need to remove the UE implicitly, and if it is a intra-MME HO, the s10-relocation completion timer will purge it anyway.
     */
    OAILOG_ERROR( LOG_S1AP, "MME UE S1AP ID provided by eNB doesn't point to any valid UE: " MME_UE_S1AP_ID_FMT " for the ENB_STATUS_TRANFER message. \n", mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }
  /** UE resources will not be released yet. */
  /** ENB Status Transfer message sent to MME_APP layer. It will decide if the target MME lies in the same MME or different MME. */
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_ENB_STATUS_TRANSFER);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  itti_s1ap_status_transfer_t *enb_status_transfer_p = &message_p->ittiMsg.s1ap_enb_status_transfer;
  memset (enb_status_transfer_p, 0, sizeof (itti_s1ap_status_transfer_t));
  enb_status_transfer_p->mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
  enb_status_transfer_p->enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
  /** Set the E-UTRAN container. The S1AP octet string should be purged in the outer method. */
  enb_status_transfer_p->bearerStatusTransferList_buffer = blk2bstr(enbStatusTransfer_p->eNB_StatusTransfer_TransparentContainer.bearers_SubjectToStatusTransferList.list.array[0]->value.buf,
      enbStatusTransfer_p->eNB_StatusTransfer_TransparentContainer.bearers_SubjectToStatusTransferList.list.array[0]->value.size);

  /** Assuming that the OCTET-STRING will be freed automatically. */
  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME,
      MSC_MMEAPP_MME,
      NULL, 0,
      "0 S1AP_ENB_STATUS_TRANSFER mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT,
      enb_status_transfer_p->mme_ue_s1ap_id);
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
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
    __attribute__((unused)) const hash_key_t keyP,
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
    OAILOG_INFO ( LOG_S1AP, "Triggering eNB deregistration indication for UE with enbId " ENB_UE_S1AP_ID_FMT ". \n", ue_ref_p->enb_ue_s1ap_id);

    AssertFatal(arg->current_ue_index < S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE, "Too many deregistered UEs reported in S1AP_ENB_DEREGISTERED_IND message ");
    S1AP_ENB_DEREGISTERED_IND (arg->message_p).mme_ue_s1ap_id[arg->current_ue_index] = ue_ref_p->mme_ue_s1ap_id;
    S1AP_ENB_DEREGISTERED_IND (arg->message_p).enb_ue_s1ap_id[arg->current_ue_index] = ue_ref_p->enb_ue_s1ap_id;

    // max ues reached
    if (arg->current_ue_index == 0 && arg->handled_ues > 0) {
      S1AP_ENB_DEREGISTERED_IND (arg->message_p).nb_ue_to_deregister = S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE;
      itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, arg->message_p);
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
typedef struct arg_s1ap_construct_enb_reset_req_s {
  uint8_t      current_ue_index;
  MessageDef  *message_p;
}arg_s1ap_construct_enb_reset_req_t;
//------------------------------------------------------------------------------
static bool construct_s1ap_mme_full_reset_req (
    __attribute__((unused)) const hash_key_t keyP,
    void * const dataP,
    void *argP,
    void ** resultP)
{
  arg_s1ap_construct_enb_reset_req_t          *arg = (arg_s1ap_construct_enb_reset_req_t*) argP;
  ue_description_t                       *ue_ref_p = (ue_description_t*)dataP;
  enb_ue_s1ap_id_t enb_ue_s1ap_id;
  uint32_t i = arg->current_ue_index;
  if (ue_ref_p) {
    enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
    S1AP_ENB_INITIATED_RESET_REQ (arg->message_p).ue_to_reset_list[i].mme_ue_s1ap_id = &(ue_ref_p->mme_ue_s1ap_id);
    S1AP_ENB_INITIATED_RESET_REQ (arg->message_p).ue_to_reset_list[i].enb_ue_s1ap_id = &enb_ue_s1ap_id;
    arg->current_ue_index++;
    *resultP = arg->message_p;
  } else {
    OAILOG_TRACE (LOG_S1AP, "No valid UE provided in callback: %p\n", ue_ref_p);
    S1AP_ENB_INITIATED_RESET_REQ (arg->message_p).ue_to_reset_list[i].mme_ue_s1ap_id = NULL;
  }
  return false;
}


//------------------------------------------------------------------------------
int
s1ap_handle_sctp_disconnection (
    const sctp_assoc_id_t assoc_id, bool reset)
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

  // First check if we can just reset the eNB state if there are no UEs.
  if (!enb_association->nb_ue_associated) {
    if (reset) {
      enb_association->s1_state = S1AP_INIT;
      OAILOG_INFO(LOG_S1AP, "Moving eNB with association id %u to INIT state\n", assoc_id);
      update_mme_app_stats_connected_enb_sub();
    } else {
      s1ap_remove_enb(enb_association);
      update_mme_app_stats_connected_enb_sub();
      OAILOG_INFO(LOG_S1AP, "Removing eNB with association id %u \n", assoc_id);
    }
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNok);
  }

  enb_association->s1_state = S1AP_SHUTDOWN;
  MSC_LOG_EVENT (MSC_S1AP_MME, "0 Event SCTP_CLOSE_ASSOCIATION assoc_id: %d", assoc_id);
  hashtable_ts_apply_callback_on_elements(&enb_association->ue_coll, s1ap_send_enb_deregistered_ind, (void*)&arg, (void**)&message_p); /**< Just releasing the procedure. */

  /** Release the S1AP UE references implicitly. */
  OAILOG_DEBUG (MSC_S1AP_MME, "Releasing all S1AP UE references related to the ENB with assocId %d. \n", assoc_id);


//  if ( (!(arg.handled_ues % S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE)) && (arg.handled_ues) ){
    S1AP_ENB_DEREGISTERED_IND (message_p).nb_ue_to_deregister = arg.current_ue_index;

    for (i = arg.current_ue_index; i < S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE; i++) {
      S1AP_ENB_DEREGISTERED_IND (message_p).mme_ue_s1ap_id[arg.current_ue_index] = 0;
      S1AP_ENB_DEREGISTERED_IND (message_p).enb_ue_s1ap_id[arg.current_ue_index] = 0;
    }
    MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_NAS_MME, NULL, 0, "0 S1AP_ENB_DEREGISTERED_IND num ue to deregister %u", S1AP_ENB_DEREGISTERED_IND (message_p).nb_ue_to_deregister);
    itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
    message_p = NULL;
//  }

  /** The eNB will be removed, when the UE references are removed. */

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
  } else if ((enb_association->s1_state == S1AP_SHUTDOWN) || (enb_association->s1_state == S1AP_RESETING)) {
    OAILOG_WARNING(LOG_S1AP, "Received new association request on an association that is being %s, ignoring",
                   s1_enb_state_str[enb_association->s1_state]);
    OAILOG_FUNC_RETURN(LOG_S1AP, RETURNerror);
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
  enb_association->s1_state = S1AP_INIT;
  MSC_LOG_EVENT (MSC_S1AP_MME, "0 Event SCTP_NEW_ASSOCIATION assoc_id: %d", enb_association->sctp_assoc_id);
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_erab_setup_response (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  OAILOG_FUNC_IN (LOG_S1AP);
  S1ap_E_RABSetupResponseIEs_t           *s1ap_E_RABSetupResponseIEs_p = NULL;
  ue_description_t                       *ue_ref_p  = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;

  s1ap_E_RABSetupResponseIEs_p = &message->msg.s1ap_E_RABSetupResponseIEs;
  MSC_LOG_RX_MESSAGE (MSC_S1AP_MME,
                      MSC_S1AP_ENB,
                      NULL, 0,
                      "0 E_RABSetupResponse/%s enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " len %u",
                      s1ap_direction2String[message->direction], s1ap_E_RABSetupResponseIEs_p->eNB_UE_S1AP_ID,
                      s1ap_E_RABSetupResponseIEs_p->mme_ue_s1ap_id);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list ((uint32_t) s1ap_E_RABSetupResponseIEs_p->mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT "\n", (mme_ue_s1ap_id_t)s1ap_E_RABSetupResponseIEs_p->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != s1ap_E_RABSetupResponseIEs_p->eNB_UE_S1AP_ID) {
    OAILOG_DEBUG (LOG_S1AP, "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT ", received: " ENB_UE_S1AP_ID_FMT "\n",
                ue_ref_p->enb_ue_s1ap_id, (enb_ue_s1ap_id_t)s1ap_E_RABSetupResponseIEs_p->eNB_UE_S1AP_ID);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_E_RAB_SETUP_RSP);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  S1AP_E_RAB_SETUP_RSP (message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
  S1AP_E_RAB_SETUP_RSP (message_p).enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
  S1AP_E_RAB_SETUP_RSP (message_p).e_rab_setup_list.no_of_items = 0;
  S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.no_of_items = 0;

  if (s1ap_E_RABSetupResponseIEs_p->presenceMask & S1AP_E_RABSETUPRESPONSEIES_E_RABSETUPLISTBEARERSURES_PRESENT) {
    int num_erab = s1ap_E_RABSetupResponseIEs_p->e_RABSetupListBearerSURes.s1ap_E_RABSetupItemBearerSURes.count;
    for (int index = 0; index < num_erab; index++) {
      S1ap_E_RABSetupItemBearerSURes_t * erab_setup_item =
          (S1ap_E_RABSetupItemBearerSURes_t*)s1ap_E_RABSetupResponseIEs_p->e_RABSetupListBearerSURes.s1ap_E_RABSetupItemBearerSURes.array[index];
      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_setup_list.item[index].e_rab_id = erab_setup_item->e_RAB_ID;
      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_setup_list.item[index].transport_layer_address =
          blk2bstr(erab_setup_item->transportLayerAddress.buf, erab_setup_item->transportLayerAddress.size);
      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_setup_list.item[index].gtp_teid = htonl (*((uint32_t *) erab_setup_item->gTP_TEID.buf));
      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_setup_list.no_of_items += 1;
    }
  }

  if (s1ap_E_RABSetupResponseIEs_p->presenceMask & S1AP_E_RABSETUPRESPONSEIES_E_RABFAILEDTOSETUPLISTBEARERSURES_PRESENT) {
    int num_erab = s1ap_E_RABSetupResponseIEs_p->e_RABFailedToSetupListBearerSURes.s1ap_E_RABItem.count;
    for (int index = 0; index < num_erab; index++) {
      S1ap_E_RABItem_t * erab_item = (S1ap_E_RABItem_t *)s1ap_E_RABSetupResponseIEs_p->e_RABFailedToSetupListBearerSURes.s1ap_E_RABItem.array[index];
      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.item[index].e_rab_id = erab_item->e_RAB_ID;
      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.item[index].cause = erab_item->cause;
      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.no_of_items += 1;
    }
  }

  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME,
                      MSC_MMEAPP_MME,
                      NULL, 0,
                      "0 S1AP_E_RAB_SETUP_RSP mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ");
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_erab_modify_response (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  OAILOG_FUNC_IN (LOG_S1AP);
  S1ap_E_RABModifyResponseIEs_t           *s1ap_E_RABModifyResponseIEs_p = NULL;
  ue_description_t                       *ue_ref_p  = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;

  s1ap_E_RABModifyResponseIEs_p = &message->msg.s1ap_E_RABModifyResponseIEs;
  MSC_LOG_RX_MESSAGE (MSC_S1AP_MME,
                      MSC_S1AP_ENB,
                      NULL, 0,
                      "0 E_RABModifyResponse/%s enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " len %u",
                      s1ap_direction2String[message->direction], s1ap_E_RABModifyResponseIEs_p->eNB_UE_S1AP_ID,
                      s1ap_E_RABModifyResponseIEs_p->mme_ue_s1ap_id);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list ((uint32_t) s1ap_E_RABModifyResponseIEs_p->mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT "\n", (mme_ue_s1ap_id_t)s1ap_E_RABModifyResponseIEs_p->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != s1ap_E_RABModifyResponseIEs_p->eNB_UE_S1AP_ID) {
    OAILOG_DEBUG (LOG_S1AP, "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT ", received: " ENB_UE_S1AP_ID_FMT "\n",
                ue_ref_p->enb_ue_s1ap_id, (enb_ue_s1ap_id_t)s1ap_E_RABModifyResponseIEs_p->eNB_UE_S1AP_ID);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_E_RAB_MODIFY_RSP);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  S1AP_E_RAB_MODIFY_RSP (message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
  S1AP_E_RAB_MODIFY_RSP (message_p).enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
  S1AP_E_RAB_MODIFY_RSP (message_p).e_rab_modify_list.no_of_items = 0;
  S1AP_E_RAB_MODIFY_RSP (message_p).e_rab_failed_to_modify_list.no_of_items = 0;

  if (s1ap_E_RABModifyResponseIEs_p->presenceMask & S1AP_E_RABMODIFYRESPONSEIES_E_RABMODIFYLISTBEARERMODRES_PRESENT) {
    int num_erab = s1ap_E_RABModifyResponseIEs_p->e_RABModifyListBearerModRes.s1ap_E_RABModifyItemBearerModRes.count;
    for (int index = 0; index < num_erab; index++) {
      S1ap_E_RABModifyItemBearerModRes_t * erab_modify_item =
          (S1ap_E_RABModifyItemBearerModRes_t*)s1ap_E_RABModifyResponseIEs_p->e_RABModifyListBearerModRes.s1ap_E_RABModifyItemBearerModRes.array[index];
      S1AP_E_RAB_MODIFY_RSP (message_p).e_rab_modify_list.item[index].e_rab_id = erab_modify_item->e_RAB_ID;
      S1AP_E_RAB_MODIFY_RSP (message_p).e_rab_modify_list.no_of_items += 1;
    }
  }

  if (s1ap_E_RABModifyResponseIEs_p->presenceMask & S1AP_E_RABMODIFYRESPONSEIES_E_RABFAILEDTOMODIFYLIST_PRESENT) {
    int num_erab = s1ap_E_RABModifyResponseIEs_p->e_RABFailedToModifyList.s1ap_E_RABItem.count;
    for (int index = 0; index < num_erab; index++) {
      S1ap_E_RABItem_t * erab_item = (S1ap_E_RABItem_t *)s1ap_E_RABModifyResponseIEs_p->e_RABFailedToModifyList.s1ap_E_RABItem.array[index];
      S1AP_E_RAB_MODIFY_RSP (message_p).e_rab_failed_to_modify_list.item[index].e_rab_id = erab_item->e_RAB_ID;
      S1AP_E_RAB_MODIFY_RSP (message_p).e_rab_failed_to_modify_list.item[index].cause = erab_item->cause;
      S1AP_E_RAB_MODIFY_RSP (message_p).e_rab_failed_to_modify_list.no_of_items += 1;
    }
  }

  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME,
                      MSC_MMEAPP_MME,
                      NULL, 0,
                      "0 S1AP_E_RAB_MODIFY_RSP mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ");
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_erab_release_response (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  OAILOG_FUNC_IN (LOG_S1AP);
  S1ap_E_RABReleaseResponseIEs_t         *s1ap_E_RABReleaseResponseIEs_p = NULL;
  ue_description_t                       *ue_ref_p  = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;

  s1ap_E_RABReleaseResponseIEs_p = &message->msg.s1ap_E_RABReleaseResponseIEs;
  MSC_LOG_RX_MESSAGE (MSC_S1AP_MME,
                      MSC_S1AP_ENB,
                      NULL, 0,
                      "0 E_RABReleaseResponse/%s enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " len %u",
                      s1ap_direction2String[message->direction], s1ap_E_RABReleaseResponseIEs_p->eNB_UE_S1AP_ID,
                      s1ap_E_RABReleaseResponseIEs_p->mme_ue_s1ap_id);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list ((uint32_t) s1ap_E_RABReleaseResponseIEs_p->mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT "\n", (mme_ue_s1ap_id_t)s1ap_E_RABReleaseResponseIEs_p->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != s1ap_E_RABReleaseResponseIEs_p->eNB_UE_S1AP_ID) {
    OAILOG_DEBUG (LOG_S1AP, "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT ", received: " ENB_UE_S1AP_ID_FMT "\n",
                ue_ref_p->enb_ue_s1ap_id, (enb_ue_s1ap_id_t)s1ap_E_RABReleaseResponseIEs_p->eNB_UE_S1AP_ID);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  // todo
//  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_E_RAB_RELEASE_RSP);
//  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
//  S1AP_E_RAB_RELEASE_RSP (message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
//  S1AP_E_RAB_RELEASE_RSP (message_p).enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
//  S1AP_E_RAB_RELEASE_RSP (message_p).e_rab_setup_list.no_of_items = 0;
//  S1AP_E_RAB_RELEASE_RSP (message_p).e_rab_failed_to_setup_list.no_of_items = 0;
//
//  if (s1ap_E_RABSetupResponseIEs_p->presenceMask & S1AP_E_RABSETUPRESPONSEIES_E_RABSETUPLISTBEARERSURES_PRESENT) {
//    int num_erab = s1ap_E_RABSetupResponseIEs_p->e_RABSetupListBearerSURes.s1ap_E_RABSetupItemBearerSURes.count;
//    for (int index = 0; index < num_erab; index++) {
//      S1ap_E_RABSetupItemBearerSURes_t * erab_setup_item =
//          (S1ap_E_RABSetupItemBearerSURes_t*)s1ap_E_RABSetupResponseIEs_p->e_RABSetupListBearerSURes.s1ap_E_RABSetupItemBearerSURes.array[index];
//      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_setup_list.item[index].e_rab_id = erab_setup_item->e_RAB_ID;
//      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_setup_list.item[index].transport_layer_address =
//          blk2bstr(erab_setup_item->transportLayerAddress.buf, erab_setup_item->transportLayerAddress.size);
//      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_setup_list.item[index].gtp_teid = htonl (*((uint32_t *) erab_setup_item->gTP_TEID.buf));
//      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_setup_list.no_of_items += 1;
//    }
//  }
//
//  if (s1ap_E_RABSetupResponseIEs_p->presenceMask & S1AP_E_RABSETUPRESPONSEIES_E_RABFAILEDTOSETUPLISTBEARERSURES_PRESENT) {
//    int num_erab = s1ap_E_RABSetupResponseIEs_p->e_RABFailedToSetupListBearerSURes.s1ap_E_RABItem.count;
//    for (int index = 0; index < num_erab; index++) {
//      S1ap_E_RABItem_t * erab_item = (S1ap_E_RABItem_t *)s1ap_E_RABSetupResponseIEs_p->e_RABFailedToSetupListBearerSURes.s1ap_E_RABItem.array[index];
//      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.item[index].e_rab_id = erab_item->e_RAB_ID;
//      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.item[index].cause = erab_item->cause;
//      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.no_of_items += 1;
//    }
//  }
//
//  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME,
//                      MSC_MMEAPP_MME,
//                      NULL, 0,
//                      "0 S1AP_E_RAB_SETUP_RSP mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ");
//  XML_MSG_DUMP_ITTI_S1AP_E_RAB_SETUP_RSP(&S1AP_E_RAB_SETUP_RSP (message_p), TASK_S1AP, TASK_MME_APP, NULL);
//  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_erab_release_indication (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  OAILOG_FUNC_IN (LOG_S1AP);
  S1ap_E_RABReleaseIndicationIEs_t         *s1ap_E_RABReleaseIndicationIEs_p = NULL;
  ue_description_t                       *ue_ref_p  = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;

  s1ap_E_RABReleaseIndicationIEs_p = &message->msg.s1ap_E_RABReleaseIndicationIEs;
  MSC_LOG_RX_MESSAGE (MSC_S1AP_MME,
                      MSC_S1AP_ENB,
                      NULL, 0,
                      "0 E_RABReleaseIndication/%s enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " len %u",
                      s1ap_direction2String[message->direction], s1ap_E_RABReleaseIndicationIEs_p->eNB_UE_S1AP_ID,
                      s1ap_E_RABReleaseIndicationIEs_p->mme_ue_s1ap_id);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list ((uint32_t) s1ap_E_RABReleaseIndicationIEs_p->mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT "\n", (mme_ue_s1ap_id_t)s1ap_E_RABReleaseIndicationIEs_p->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != s1ap_E_RABReleaseIndicationIEs_p->eNB_UE_S1AP_ID) {
    OAILOG_DEBUG (LOG_S1AP, "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT ", received: " ENB_UE_S1AP_ID_FMT "\n",
                ue_ref_p->enb_ue_s1ap_id, (enb_ue_s1ap_id_t)s1ap_E_RABReleaseIndicationIEs_p->eNB_UE_S1AP_ID);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

//  if (s1ap_E_RABSetupResponseIEs_p->presenceMask & S1AP_E_RABSETUPRESPONSEIES_E_RABFAILEDTOSETUPLISTBEARERSURES_PRESENT) {
//    int num_erab = s1ap_E_RABSetupResponseIEs_p->e_RABFailedToSetupListBearerSURes.s1ap_E_RABItem.count;
//    for (int index = 0; index < num_erab; index++) {
//      S1ap_E_RABItem_t * erab_item = (S1ap_E_RABItem_t *)s1ap_E_RABSetupResponseIEs_p->e_RABFailedToSetupListBearerSURes.s1ap_E_RABItem.array[index];
//      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.item[index].e_rab_id = erab_item->e_RAB_ID;
//      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.item[index].cause = erab_item->cause;
//      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.no_of_items += 1;
//    }
//  }
//
//  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME,
//                      MSC_MMEAPP_MME,
//                      NULL, 0,
//                      "0 S1AP_E_RAB_SETUP_RSP mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ");
//  XML_MSG_DUMP_ITTI_S1AP_E_RAB_SETUP_RSP(&S1AP_E_RAB_SETUP_RSP (message_p), TASK_S1AP, TASK_MME_APP, NULL);
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
void
s1ap_mme_handle_ue_context_rel_comp_timer_expiry (void *arg)
{
  MessageDef                             *message_p = NULL;
  OAILOG_FUNC_IN (LOG_S1AP);
  DevAssert (arg != NULL);

  ue_description_t *ue_ref_p  =        (ue_description_t *)arg;

  ue_ref_p->s1ap_ue_context_rel_timer.id = S1AP_TIMER_INACTIVE_ID;
  OAILOG_DEBUG (LOG_S1AP, "Expired- UE Context Release Timer for UE id  %d \n", ue_ref_p->mme_ue_s1ap_id);
  /*
   * Remove UE context and inform MME_APP.
   */
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_UE_CONTEXT_RELEASE_COMPLETE);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  memset ((void *)&message_p->ittiMsg.s1ap_ue_context_release_complete, 0, sizeof (itti_s1ap_ue_context_release_complete_t));
  S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_MMEAPP_MME, NULL, 0, "0 S1AP_UE_CONTEXT_RELEASE_COMPLETE mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ", S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).mme_ue_s1ap_id);
  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  DevAssert(ue_ref_p->s1_ue_state == S1AP_UE_WAITING_CRR);
  OAILOG_DEBUG (LOG_S1AP, "Removed S1AP UE " MME_UE_S1AP_ID_FMT "\n", (uint32_t) ue_ref_p->mme_ue_s1ap_id);
  s1ap_remove_ue (ue_ref_p);
  OAILOG_FUNC_OUT (LOG_S1AP);
}

//------------------------------------------------------------------------------
void
s1ap_mme_handle_mme_mobility_completion_timer_expiry (void *arg)
{
  OAILOG_FUNC_IN (LOG_S1AP);
  DevAssert (arg != NULL);

  ue_description_t *ue_reference_p  =        (ue_description_t *)arg;
  MessageDef                             *message_p = NULL;
  OAILOG_INFO (LOG_S1AP, "Expired- MME Mobility Completion timer for enbUeS1apId " ENB_UE_S1AP_ID_FMT " \n", ue_reference_p->enb_ue_s1ap_id);

  /** Inactivate the timer either here in the timeout or manually when removing the S1AP UE_REFERECE. */
  ue_reference_p->s1ap_handover_completion_timer.id = S1AP_TIMER_INACTIVE_ID;

  /** Perform an UE_CONTEXT_RELEASE and inform the MME_APP afterwards, which will check if the CLR flag is set or not. */
  if(s1ap_mme_generate_ue_context_release_command (ue_reference_p, ue_reference_p->mme_ue_s1ap_id, S1AP_NAS_NORMAL_RELEASE, ue_reference_p->enb) == RETURNerror){
    OAILOG_DEBUG (LOG_S1AP, "Error removing S1AP_UE_REFERENCE FOR ENB_UE_S1AP_ID " ENB_UE_S1AP_ID_FMT " and mmeUeS1apId  " MME_UE_S1AP_ID_FMT " \n", (uint32_t) ue_reference_p->enb_ue_s1ap_id, (uint32_t) ue_reference_p->mme_ue_s1ap_id);
  }else{
    OAILOG_DEBUG (LOG_S1AP, "Removed S1AP_UE_REFERENCE FOR ENB_UE_S1AP_ID " ENB_UE_S1AP_ID_FMT " and mmeUeS1apId  " MME_UE_S1AP_ID_FMT "\n", (uint32_t) ue_reference_p->enb_ue_s1ap_id, (uint32_t) ue_reference_p->mme_ue_s1ap_id);
  }
  OAILOG_FUNC_OUT (LOG_S1AP);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_error_ind_message (const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream, struct s1ap_message_s *message)
{
  MessageDef                             *message_p = NULL;
  S1ap_ErrorIndicationIEs_t              *errorIndication_p = NULL;
  long                                    cause_value;
  OAILOG_FUNC_IN (LOG_S1AP);

  /** Get the Failure Message. */
  errorIndication_p = &message->msg.s1ap_ErrorIndicationIEs;

  OAILOG_FUNC_IN (LOG_S1AP);

  enb_description_t * enb_ref = s1ap_is_enb_assoc_id_in_list (assoc_id);
  DevAssert(enb_ref);

  ue_description_t * ue_ref = s1ap_is_ue_enb_id_in_list (enb_ref, errorIndication_p->eNB_UE_S1AP_ID);
  /** If no UE reference exists, drop the message. */
  if(!ue_ref){
    /** Check that the UE reference exists. */
    OAILOG_WARNING(LOG_S1AP, "No UE reference exists for enbId %d and enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT ". Dropping error indication. \n", enb_ref->enb_id, errorIndication_p->eNB_UE_S1AP_ID);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  /*
   * Handle error indication.
   * Just forward to the MME_APP layer, depending if there is a handover procedure running or not the current handover procedure might be disregarded.
   */
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_ERROR_INDICATION);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  memset ((void *)&message_p->ittiMsg.s1ap_error_indication, 0, sizeof (itti_s1ap_error_indication_t));
  S1AP_ERROR_INDICATION (message_p).mme_ue_s1ap_id = errorIndication_p->mme_ue_s1ap_id;
  S1AP_ERROR_INDICATION (message_p).enb_ue_s1ap_id = errorIndication_p->eNB_UE_S1AP_ID;
  S1AP_ERROR_INDICATION (message_p).enb_id = enb_ref->enb_id;

  /** Choice. */
  S1ap_Cause_PR cause_type = errorIndication_p->cause.present;

  switch (cause_type)
  {
  case S1ap_Cause_PR_radioNetwork:
    cause_value = errorIndication_p->cause.choice.radioNetwork;
    OAILOG_DEBUG (LOG_S1AP, "S1AP_ERROR_INDICATION with Cause_Type = Radio Network and Cause_Value = %ld\n", cause_value);
    break;

  case S1ap_Cause_PR_transport:
    cause_value = errorIndication_p->cause.choice.transport;
    OAILOG_DEBUG (LOG_S1AP, "S1AP_ERROR_INDICATION with Cause_Type = Transport and Cause_Value = %ld\n", cause_value);
    break;

  case S1ap_Cause_PR_nas:
    cause_value = errorIndication_p->cause.choice.nas;
    OAILOG_DEBUG (LOG_S1AP, "S1AP_ERROR_INDICATION with Cause_Type = NAS and Cause_Value = %ld\n", cause_value);
    break;

  case S1ap_Cause_PR_protocol:
    cause_value = errorIndication_p->cause.choice.protocol;
    OAILOG_DEBUG (LOG_S1AP, "S1AP_ERROR_INDICATION with Cause_Type = Protocol and Cause_Value = %ld\n", cause_value);
    break;

  case S1ap_Cause_PR_misc:
    cause_value = errorIndication_p->cause.choice.misc;
    OAILOG_DEBUG (LOG_S1AP, "S1AP_ERROR_INDICATION with Cause_Type = MISC and Cause_Value = %ld\n", cause_value);
    break;

  default:
    OAILOG_ERROR (LOG_S1AP, "S1AP_ERROR_INDICATION with Invalid Cause_Type = %d\n", cause_type);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  /**
   * No match function exists between the S1AP Cause and the decoded cause yet.
   * Set it to anything else thatn SYSTEM_FAILURE, not to trigger any implicit detach due cause (only based on the handover type).
   */
  S1AP_ERROR_INDICATION (message_p).cause = S1AP_HANDOVER_FAILED;

  MSC_LOG_TX_MESSAGE (MSC_S1AP_MME, MSC_MMEAPP_MME, NULL, 0, "0 S1AP_ERROR_INDICATION mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ", S1AP_ERROR_INDICATION (message_p).mme_ue_s1ap_id);
  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------

int
s1ap_mme_handle_enb_reset (
     const sctp_assoc_id_t assoc_id,
     const sctp_stream_id_t stream,
    struct s1ap_message_s *message)
{
  S1ap_ResetIEs_t                         *enb_reset_p = NULL;
  MessageDef                              *message_p  = NULL;
  ue_description_t                        *ue_ref_p = NULL;
  enb_description_t                       *enb_association = NULL;
  s1ap_reset_type_t                       s1ap_reset_type;
  S1ap_UE_associatedLogicalS1_ConnectionItem_t* s1_sig_conn_id_p = NULL;
  arg_s1ap_construct_enb_reset_req_t      arg = {0};
  uint32_t                                i = 0;
  int                                     rc = RETURNok;
  mme_ue_s1ap_id_t  mme_ue_s1ap_id;
  enb_ue_s1ap_id_t  enb_ue_s1ap_id;


  OAILOG_FUNC_IN (LOG_S1AP);
  /*
   * Checking that the assoc id has a valid eNB attached to.
   */
  enb_association = s1ap_is_enb_assoc_id_in_list (assoc_id);

  if (enb_association == NULL) {
    OAILOG_ERROR (LOG_S1AP, "No eNB attached to this assoc_id: %d\n", assoc_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  // Check the state of S1AP
  if (enb_association->s1_state != S1AP_READY) {
    // Ignore the message
    OAILOG_INFO (LOG_S1AP, "S1 setup is not done.Invalid state.Ignoring ENB Initiated Reset.eNB Id = %d , S1AP state = %d \n", enb_association->enb_id, enb_association->s1_state);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
  }

  // Check if there are no UEs connected
  if (!enb_association->nb_ue_associated) {
    // Ignore the message
    OAILOG_INFO (LOG_S1AP, "No UEs is connected.Ignoring ENB Initiated Reset.eNB Id = %d\n", enb_association->enb_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
  }
  // Check the reset type - partial_reset OR reset_all
  enb_reset_p = &message->msg.s1ap_ResetIEs;
  switch (enb_reset_p->resetType.present) {

  case S1ap_ResetType_PR_s1_Interface:
      s1ap_reset_type = RESET_ALL;
    break;
  case S1ap_ResetType_PR_partOfS1_Interface:
      s1ap_reset_type = RESET_PARTIAL;
    break;
  default:
      OAILOG_ERROR (LOG_S1AP, "Reset Request from eNB  with Invalid reset_type = %d\n", enb_reset_p->resetType.present);
      // TBD - Here MME should send Error Indication as it is abnormal scenario.
      OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (s1ap_reset_type == RESET_PARTIAL) {

    if (enb_reset_p->resetType.choice.partOfS1_Interface.list.count == 0) {
      OAILOG_ERROR (LOG_S1AP, "Partial Reset Request without any S1 signaling connection.Ignoring it \n");
      // TBD - Here MME should send Error Indication as it is abnormal scenario.
      OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
    }
    if (enb_reset_p->resetType.choice.partOfS1_Interface.list.count > enb_association->nb_ue_associated) {
      OAILOG_ERROR (LOG_S1AP, "Partial Reset Request. Requested number of UEs %d to be reset is more than connected UEs %d \n",
                          enb_reset_p->resetType.choice.partOfS1_Interface.list.count, enb_association->nb_ue_associated);
      // TBD - Here MME should send Error Indication as it is abnormal scenario.
      OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
    }
  }
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_ENB_INITIATED_RESET_REQ);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  memset ((void *)&message_p->ittiMsg.s1ap_enb_initiated_reset_req, 0, sizeof (itti_s1ap_enb_initiated_reset_req_t));
  S1AP_ENB_INITIATED_RESET_REQ (message_p).s1ap_reset_type = s1ap_reset_type;
  S1AP_ENB_INITIATED_RESET_REQ (message_p).enb_id = enb_association->enb_id;
  S1AP_ENB_INITIATED_RESET_REQ (message_p).sctp_assoc_id = assoc_id;
  S1AP_ENB_INITIATED_RESET_REQ (message_p).sctp_stream_id = stream;

  if (s1ap_reset_type == RESET_ALL) {
    S1AP_ENB_INITIATED_RESET_REQ (message_p).num_ue = enb_association->nb_ue_associated;
    S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list = (s1_sig_conn_id_t*) calloc (enb_association->nb_ue_associated,
                                                                                          sizeof (*(S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list)));
    DevAssert(S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list != NULL);
    arg.message_p = message_p;
    hashtable_ts_apply_callback_on_elements(&enb_association->ue_coll, construct_s1ap_mme_full_reset_req, (void*)&arg, (void**) &message_p);
  } else {
    // Partial Reset
    S1AP_ENB_INITIATED_RESET_REQ (message_p).num_ue = enb_reset_p->resetType.choice.partOfS1_Interface.list.count;
    S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list = (s1_sig_conn_id_t*)
                        calloc (enb_reset_p->resetType.choice.partOfS1_Interface.list.count, sizeof (*(S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list)));
    DevAssert(S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list != NULL);
    for (i = 0; i < enb_reset_p->resetType.choice.partOfS1_Interface.list.count; i++) {
      s1_sig_conn_id_p = (S1ap_UE_associatedLogicalS1_ConnectionItem_t*) enb_reset_p->resetType.choice.partOfS1_Interface.list.array[i];
      DevAssert(s1_sig_conn_id_p != NULL);

      if (s1_sig_conn_id_p->mME_UE_S1AP_ID != NULL) {
        mme_ue_s1ap_id = (mme_ue_s1ap_id_t) *(s1_sig_conn_id_p->mME_UE_S1AP_ID);
        if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) != NULL) {
          if (s1_sig_conn_id_p->eNB_UE_S1AP_ID != NULL) {
            enb_ue_s1ap_id = (enb_ue_s1ap_id_t) *(s1_sig_conn_id_p->eNB_UE_S1AP_ID);
            if (ue_ref_p->enb_ue_s1ap_id == (enb_ue_s1ap_id & ENB_UE_S1AP_ID_MASK)) {
              S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list[i].mme_ue_s1ap_id = &(ue_ref_p->mme_ue_s1ap_id);
              enb_ue_s1ap_id &= ENB_UE_S1AP_ID_MASK;
              S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list[i].enb_ue_s1ap_id = &enb_ue_s1ap_id;
            } else {
              // mismatch in enb_ue_s1ap_id sent by eNB and stored in S1AP ue context in EPC. Abnormal case.
              S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list[i].mme_ue_s1ap_id = NULL;
              S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list[i].enb_ue_s1ap_id = NULL;
              OAILOG_ERROR (LOG_S1AP, "Partial Reset Request:enb_ue_s1ap_id mismatch between id %d sent by eNB and id %d stored in epc for mme_ue_s1ap_id %d \n",
                          enb_ue_s1ap_id, ue_ref_p->enb_ue_s1ap_id, mme_ue_s1ap_id);
            }
          } else {
            S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list[i].mme_ue_s1ap_id = &(ue_ref_p->mme_ue_s1ap_id);
            S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list[i].enb_ue_s1ap_id = NULL;
          }
        } else {
          OAILOG_ERROR (LOG_S1AP, "Partial Reset Request - No UE context found for mme_ue_s1ap_id %d \n", mme_ue_s1ap_id);
          // TBD - Here MME should send Error Indication as it is abnormal scenario.
          OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
        }
      } else {
        if (s1_sig_conn_id_p->eNB_UE_S1AP_ID != NULL) {
          enb_ue_s1ap_id = (enb_ue_s1ap_id_t) *(s1_sig_conn_id_p->eNB_UE_S1AP_ID);
          if ((ue_ref_p = s1ap_is_ue_enb_id_in_list (enb_association, enb_ue_s1ap_id)) != NULL) {
            enb_ue_s1ap_id &= ENB_UE_S1AP_ID_MASK;
            S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list[i].enb_ue_s1ap_id = &enb_ue_s1ap_id;
            if (ue_ref_p->mme_ue_s1ap_id != INVALID_MME_UE_S1AP_ID) {
              S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list[i].mme_ue_s1ap_id = &(ue_ref_p->mme_ue_s1ap_id);
            } else {
              S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list[i].mme_ue_s1ap_id = NULL;
            }
          } else {
              OAILOG_ERROR (LOG_S1AP, "Partial Reset Request without any valid S1 signaling connection.Ignoring it \n");
              // TBD - Here MME should send Error Indication as it is abnormal scenario.
              OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
          }
        } else {
          OAILOG_ERROR (LOG_S1AP, "Partial Reset Request without any valid S1 signaling connection.Ignoring it \n");
          // TBD - Here MME should send Error Indication as it is abnormal scenario.
          OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
        }
      }
    }
  }
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}
//------------------------------------------------------------------------------
int
s1ap_handle_enb_initiated_reset_ack (
  const itti_s1ap_enb_initiated_reset_ack_t * const enb_reset_ack_p)
{
  uint8_t                                *buffer = NULL;
  uint32_t                                length = 0;
  s1ap_message                            message = {0};
  S1ap_ResetAcknowledgeIEs_t * s1ap_ResetAcknowledgeIEs_p = NULL;
  S1ap_UE_associatedLogicalS1_ConnectionItem_t sig_conn_list[MAX_NUM_PARTIAL_S1_CONN_RESET] = {{0}};
  S1ap_MME_UE_S1AP_ID_t mme_ue_id[MAX_NUM_PARTIAL_S1_CONN_RESET] = {0};
  S1ap_ENB_UE_S1AP_ID_t enb_ue_id[MAX_NUM_PARTIAL_S1_CONN_RESET] = {0};
  MessagesIds                             message_id = MESSAGES_ID_MAX;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_S1AP);

  message.procedureCode = S1ap_ProcedureCode_id_Reset;
  message.direction = S1AP_PDU_PR_successfulOutcome;
  s1ap_ResetAcknowledgeIEs_p = &message.msg.s1ap_ResetAcknowledgeIEs;
  s1ap_ResetAcknowledgeIEs_p->presenceMask = 0;

  if (enb_reset_ack_p->s1ap_reset_type == RESET_PARTIAL) {
    DevAssert(enb_reset_ack_p->num_ue > 0);
    s1ap_ResetAcknowledgeIEs_p->presenceMask |= S1AP_RESETACKNOWLEDGEIES_UE_ASSOCIATEDLOGICALS1_CONNECTIONLISTRESACK_PRESENT;
    s1ap_ResetAcknowledgeIEs_p->uE_associatedLogicalS1_ConnectionListResAck.s1ap_UE_associatedLogicalS1_ConnectionItemResAck.count = enb_reset_ack_p->num_ue;
    for (uint32_t i = 0; i < enb_reset_ack_p->num_ue; i++) {
      if (enb_reset_ack_p->ue_to_reset_list[i].mme_ue_s1ap_id != NULL) {
        mme_ue_id[i] = *(enb_reset_ack_p->ue_to_reset_list[i].mme_ue_s1ap_id);
        sig_conn_list[0].mME_UE_S1AP_ID = &mme_ue_id[i];
      } else {
        sig_conn_list[0].mME_UE_S1AP_ID = NULL;
      }
      if (enb_reset_ack_p->ue_to_reset_list[i].enb_ue_s1ap_id != NULL) {
        enb_ue_id[i] = *(enb_reset_ack_p->ue_to_reset_list[i].enb_ue_s1ap_id);
        sig_conn_list[0].eNB_UE_S1AP_ID = &enb_ue_id[i];
      } else {
        sig_conn_list[0].eNB_UE_S1AP_ID = NULL;
      }
      sig_conn_list[0].iE_Extensions = NULL;
    }

    ASN_SEQUENCE_ADD (&s1ap_ResetAcknowledgeIEs_p->uE_associatedLogicalS1_ConnectionListResAck.s1ap_UE_associatedLogicalS1_ConnectionItemResAck, sig_conn_list);
  }
  if (s1ap_mme_encode_pdu (&message, &message_id, &buffer, &length) < 0) {
    OAILOG_ERROR (LOG_S1AP, "Reset Ack encoding failed \n");
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }
  bstring b = blk2bstr(buffer, length);
  free(buffer);
  rc = s1ap_mme_itti_send_sctp_request (&b, enb_reset_ack_p->sctp_assoc_id, enb_reset_ack_p->sctp_stream_id, INVALID_MME_UE_S1AP_ID);
  s1ap_free_mme_encode_pdu(&message, message_id);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

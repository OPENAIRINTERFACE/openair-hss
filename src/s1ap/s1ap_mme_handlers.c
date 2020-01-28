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

#include "3gpp_requirements_36.413.h"
#include "assertions.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "s1ap_common.h"
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
  {s1ap_mme_handle_enb_configuration_transfer, 0, 0},                    /* eNBConfigurationTransfer */
  {0, 0, 0},                    /* MMEConfigurationTransfer */
  // HERE END UPDATES REL 9, 10, 11
  // UPDATE RELEASE 12 :
  {0, 0, 0},                    /* CellTrafficTrace */
  {0, 0, 0},                    /* 43 Kill */
  {0, 0, 0},                    /* 44 DownlinkUEAssociatedLPPaTransport  */
  {0, 0, 0},                    /* 45 UplinkUEAssociatedLPPaTransport */
  {0, 0, 0},                    /* 46 DownlinkNonUEAssociatedLPPaTransport */
  {0, 0, 0},                    /* 47 UplinkNonUEAssociatedLPPaTransport */
  {0, 0, 0},                    /* 48 UERadioCapabilityMatch */
  {0, 0, 0},                    /* 49 PWSRestartIndication */
  // UPDATE RELEASE 13 :
  {s1ap_mme_handle_erab_modification_indication, 0, 0}, /* 50 E-RABModificationIndication */
  {0, 0, 0},                    /* 51 PWSFailureIndication */
  {0, 0, 0},                    /* 52 RerouteNASRequest */
  {0, 0, 0},                    /* 53 UEContextModificationIndication */
  {0, 0, 0},                    /* 54 ConnectionEstablishmentIndication */
  // UPDATE RELEASE 14 :
  {0, 0, 0},                    /* 55 UEContextSuspend */
  {0, 0, 0},                    /* 56 UEContextResume */
  {0, 0, 0},                    /* 57 NASDeliveryIndication */
  {0, 0, 0},                    /* 58 RetrieveUEInformation */
  {0, 0, 0},                    /* 59 UEInformationTransfer */
  // UPDATE RELEASE 15 :
  {0, 0, 0},                    /* 60 eNBCPRelocationIndication */
  {0, 0, 0},                    /* 61 MMECPRelocationIndication */
  {0, 0, 0},                    /* 62 SecondaryRATDataUsageReport */
};

const char                             *s1ap_direction2String[] = {
  "",                           /* Nothing */
  "Originating message",        /* originating message */
  "Successfull outcome",        /* successfull outcome */
  "UnSuccessfull outcome",      /* unsuccessfull outcome */
};

//------------------------------------------------------------------------------
int
s1ap_mme_handle_message (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t *pdu)
{

  /* Checking procedure Code and direction of message */
  if (pdu->choice.initiatingMessage.procedureCode >= sizeof(messages_callback) / (3 * sizeof(s1ap_message_decoded_callback))
      || (pdu->present > S1AP_S1AP_PDU_PR_unsuccessfulOutcome)) {
    OAILOG_DEBUG (LOG_S1AP, "[SCTP %d] Either procedureCode %ld or direction %d exceed expected\n",
               assoc_id, pdu->choice.initiatingMessage.procedureCode, pdu->present);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_S1AP_S1AP_PDU, pdu);
    return -1;
  }

  /* No handler present.
   * This can mean not implemented or no procedure for eNB (wrong direction).
   */
  if (messages_callback[pdu->choice.initiatingMessage.procedureCode][pdu->present - 1] == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "[SCTP %d] No handler for procedureCode %ld in %s\n",
               assoc_id, pdu->choice.initiatingMessage.procedureCode,
               s1ap_direction2String[pdu->present]);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_S1AP_S1AP_PDU, pdu);
    return -1;
  }

  /* Calling the right handler */
  int ret = (*messages_callback[pdu->choice.initiatingMessage.procedureCode][pdu->present - 1])(assoc_id, stream, pdu);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_S1AP_S1AP_PDU, pdu);
  return ret;
}

//------------------------------------------------------------------------------
int
s1ap_mme_set_cause (
  S1AP_Cause_t * cause_p,
  const S1AP_Cause_PR cause_type,
  const long cause_value)
{
  DevAssert (cause_p != NULL);
  cause_p->present = cause_type;

  switch (cause_type) {
  case S1AP_Cause_PR_radioNetwork:
    cause_p->choice.misc = cause_value;
    break;

  case S1AP_Cause_PR_transport:
    cause_p->choice.transport = cause_value;
    break;

  case S1AP_Cause_PR_nas:
    cause_p->choice.nas = cause_value;
    break;

  case S1AP_Cause_PR_protocol:
    cause_p->choice.protocol = cause_value;
    break;

  case S1AP_Cause_PR_misc:
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
    const S1AP_Cause_PR cause_type,
    const long cause_value,
    const long time_to_wait)
{
  uint8_t                                *buffer_p = 0;
  uint32_t                                length = 0;
  S1AP_S1AP_PDU_t                         pdu;
  S1AP_S1SetupFailure_t                  *out;
  S1AP_S1SetupFailureIEs_t               *ie = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_S1AP);

  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1AP_S1AP_PDU_PR_unsuccessfulOutcome;
  pdu.choice.unsuccessfulOutcome.procedureCode = S1AP_ProcedureCode_id_S1Setup;
  pdu.choice.unsuccessfulOutcome.criticality = S1AP_Criticality_reject;
  pdu.choice.unsuccessfulOutcome.value.present = S1AP_UnsuccessfulOutcome__value_PR_S1SetupFailure;
  out = &pdu.choice.unsuccessfulOutcome.value.choice.S1SetupFailure;

  ie = (S1AP_S1SetupFailureIEs_t *)calloc(1, sizeof(S1AP_S1SetupFailureIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_Cause;
  ie->criticality = S1AP_Criticality_ignore;
  ie->value.present = S1AP_S1SetupFailureIEs__value_PR_Cause;
  s1ap_mme_set_cause (&ie->value.choice.Cause, cause_type, cause_value);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /*
   * Include the optional field time to wait only if the value is > -1
   */
  if (time_to_wait > -1) {
    ie = (S1AP_S1SetupFailureIEs_t *)calloc(1, sizeof(S1AP_S1SetupFailureIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_TimeToWait;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_S1SetupFailureIEs__value_PR_TimeToWait;
    ie->value.choice.TimeToWait = time_to_wait;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  if (s1ap_mme_encode_pdu (&pdu, &buffer_p, &length) < 0) {
    OAILOG_ERROR (LOG_S1AP, "Failed to encode s1 setup failure\n");
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

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
    S1AP_S1AP_PDU_t *pdu)
{

  int                                     rc = RETURNok;
  OAILOG_FUNC_IN (LOG_S1AP);
  if (hss_associated) {
    S1AP_S1SetupRequest_t                  *container = NULL;
    S1AP_S1SetupRequestIEs_t               *ie = NULL;
    S1AP_S1SetupRequestIEs_t               *ie_enb_name = NULL;
    S1AP_S1SetupRequestIEs_t               *ie_supported_tas = NULL;
    S1AP_Global_ENB_ID_t	               *global_enb_id = NULL;;

    enb_description_t                      *enb_association = NULL;
    uint32_t                                enb_id = 0;
    char                                   *enb_name = NULL;
    int                                     ta_ret = 0;
    uint16_t                                max_enb_connected = 0;

    DevAssert (pdu != NULL);
    container = &pdu->choice.initiatingMessage.value.choice.S1SetupRequest;
    /*
     * We received a new valid S1 Setup Request on a stream != 0.
     * * * * This should not happen -> reject eNB s1 setup request.
     */

    if (stream != 0) {
      OAILOG_ERROR (LOG_S1AP, "Received new s1 setup request on stream != 0\n");
      /*
       * Send a s1 setup failure with protocol cause unspecified
       */
      rc =  s1ap_mme_generate_s1_setup_failure (assoc_id, S1AP_Cause_PR_protocol, S1AP_CauseProtocol_unspecified, -1);
      OAILOG_FUNC_RETURN (LOG_S1AP, rc);
    }

    shared_log_queue_item_t *  context = NULL;
    OAILOG_MESSAGE_START (OAILOG_LEVEL_DEBUG, LOG_S1AP, (&context), "New s1 setup request incoming from ");

    S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_S1SetupRequestIEs_t, ie_enb_name, container,
                             S1AP_ProtocolIE_ID_id_eNBname, false);
    if (ie) {
      OAILOG_MESSAGE_ADD (context, "%*s ", (int)ie_enb_name->value.choice.ENBname.size, ie_enb_name->value.choice.ENBname.buf);
      enb_name = (char *) ie_enb_name->value.choice.ENBname.buf;
    }

    S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_S1SetupRequestIEs_t, ie, container, S1AP_ProtocolIE_ID_id_Global_ENB_ID, true);
    // TODO make sure ie != NULL

    if (ie->value.choice.Global_ENB_ID.eNB_ID.present == S1AP_ENB_ID_PR_homeENB_ID) {
       // Home eNB ID = 28 bits
      uint8_t *enb_id_buf = ie->value.choice.Global_ENB_ID.eNB_ID.choice.homeENB_ID.buf;

      if (ie->value.choice.Global_ENB_ID.eNB_ID.choice.homeENB_ID.size != 28) {
        //TODO: handle case were size != 28 -> notify ? reject ?
        OAILOG_DEBUG (LOG_S1AP, "S1-Setup-Request homeENB_ID.size %lu (should be 28)\n", ie->value.choice.Global_ENB_ID.eNB_ID.choice.macroENB_ID.size);
      }

      enb_id = (enb_id_buf[0] << 20) + (enb_id_buf[1] << 12) + (enb_id_buf[2] << 4) + ((enb_id_buf[3] & 0xf0) >> 4);
      OAILOG_MESSAGE_ADD (context, "home eNB id: %07x", enb_id);
    } else {
      // Macro eNB = 20 bits
      uint8_t                                *enb_id_buf = ie->value.choice.Global_ENB_ID.eNB_ID.choice.macroENB_ID.buf;

      if (ie->value.choice.Global_ENB_ID.eNB_ID.choice.macroENB_ID.size != 20) {
        //TODO: handle case were size != 20 -> notify ? reject ?
        OAILOG_DEBUG (LOG_S1AP, "S1-Setup-Request macroENB_ID.size %lu (should be 20)\n", ie->value.choice.Global_ENB_ID.eNB_ID.choice.macroENB_ID.size);
     }

      enb_id = (enb_id_buf[0] << 12) + (enb_id_buf[1] << 4) + ((enb_id_buf[2] & 0xf0) >> 4);
      OAILOG_MESSAGE_ADD (context, "macro eNB id: %05x", enb_id);
    }
    OAILOG_MESSAGE_FINISH(context);
    global_enb_id = &ie->value.choice.Global_ENB_ID;

    mme_config_read_lock (&mme_config);
    max_enb_connected = mme_config.max_enbs;
    mme_config_unlock (&mme_config);

    if (nb_enb_associated == max_enb_connected) {
      OAILOG_ERROR (LOG_S1AP, "There is too much eNB connected to MME, rejecting the association\n");
      OAILOG_DEBUG (LOG_S1AP, "Connected = %d, maximum allowed = %d\n", nb_enb_associated, max_enb_connected);
      /*
       * Send an overload cause...
       */
      rc = s1ap_mme_generate_s1_setup_failure (assoc_id, S1AP_Cause_PR_misc, S1AP_CauseMisc_control_processing_overload, S1AP_TimeToWait_v20s);
      OAILOG_FUNC_RETURN (LOG_S1AP, rc);
    }

    if(s1ap_mme_compare_gummei(&global_enb_id->pLMNidentity) != RETURNok){
      OAILOG_ERROR (LOG_S1AP, "No Common GUMMEI with eNB, generate_s1_setup_failure\n");
      rc =  s1ap_mme_generate_s1_setup_failure (assoc_id, S1AP_Cause_PR_misc, S1AP_CauseMisc_unknown_PLMN, S1AP_TimeToWait_v20s);
      OAILOG_FUNC_RETURN (LOG_S1AP, rc);
    }

    /* Requirement MME36.413R10_8.7.3.4 Abnormal Conditions
     * If the eNB initiates the procedure by sending a S1 SETUP REQUEST message including the PLMN Identity IEs and
     * none of the PLMNs provided by the eNB is identified by the MME, then the MME shall reject the eNB S1 Setup
     * Request procedure with the appropriate cause value, e.g, “Unknown PLMN”.
     */
    S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_S1SetupRequestIEs_t, ie_supported_tas, container,
                             S1AP_ProtocolIE_ID_id_SupportedTAs, true);
    ta_ret = s1ap_mme_compare_ta_lists(&ie_supported_tas->value.choice.SupportedTAs);

    /*
     * eNB and MME have no common PLMN
     */
    if (ta_ret != TA_LIST_RET_OK) {
      OAILOG_ERROR (LOG_S1AP, "No Common PLMN with eNB, generate_s1_setup_failure\n");
      rc = s1ap_mme_generate_s1_setup_failure(assoc_id,
                                            S1AP_Cause_PR_misc,
                                            S1AP_CauseMisc_unknown_PLMN,
                                            S1AP_TimeToWait_v20s);
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
        OAILOG_DEBUG (LOG_S1AP, "Adding eNB id %u to the list of served eNBs\n", enb_id);
        enb_association->enb_id = enb_id;

        S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_S1SetupRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_DefaultPagingDRX, true);
        enb_association->default_paging_drx = ie->value.choice.PagingDRX;

        s1ap_set_tai(enb_association, &ie_supported_tas->value.choice.SupportedTAs);
        if (enb_name != NULL) {
          memcpy (enb_association->enb_name, ie_enb_name->value.choice.ENBname.buf, ie_enb_name->value.choice.ENBname.size);
          enb_association->enb_name[ie_enb_name->value.choice.ENBname.size] = '\0';
        }
      }
    } else {
      enb_association->s1_state = S1AP_RESETING;

      /*
       * eNB has been fount in list, consider the s1 setup request as a reset connection,
       * * * * reseting any previous UE state if sctp association is != than the previous one
       */
      if (enb_association->sctp_assoc_id != assoc_id) {
        OAILOG_ERROR (LOG_S1AP, "Rejecting s1 setup request as eNB id %d is already associated to an active sctp association" "Previous known: %d, new one: %d\n", enb_id, enb_association->sctp_assoc_id, assoc_id);
        rc = s1ap_mme_generate_s1_setup_failure (assoc_id, S1AP_Cause_PR_misc, S1AP_CauseMisc_unspecified, -1); /**< eNB should attach again. */

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
    rc = s1ap_mme_generate_s1_setup_failure (assoc_id, S1AP_Cause_PR_misc, S1AP_CauseMisc_unspecified, -1);
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
  S1AP_S1AP_PDU_t                         pdu;
  S1AP_S1SetupResponse_t                 *out;
  S1AP_S1SetupResponseIEs_t              *ie = NULL;
  S1AP_ServedGUMMEIsItem_t               *servedGUMMEI = NULL;

  uint8_t                                *buffer = NULL;
  uint32_t                                length = 0;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_S1AP);
  DevAssert (enb_association != NULL);
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1AP_S1AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = S1AP_ProcedureCode_id_S1Setup;
  pdu.choice.successfulOutcome.criticality = S1AP_Criticality_reject;
  pdu.choice.successfulOutcome.value.present = S1AP_SuccessfulOutcome__value_PR_S1SetupResponse;
  out = &pdu.choice.successfulOutcome.value.choice.S1SetupResponse;

  // Generating response
  mme_config_read_lock (&mme_config);

  ie = (S1AP_S1SetupResponseIEs_t *)calloc(1, sizeof(S1AP_S1SetupResponseIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_RelativeMMECapacity;
  ie->criticality = S1AP_Criticality_ignore;
  ie->value.present = S1AP_S1SetupResponseIEs__value_PR_RelativeMMECapacity;
  ie->value.choice.RelativeMMECapacity = mme_config.relative_capacity;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  ie = (S1AP_S1SetupResponseIEs_t *)calloc(1, sizeof(S1AP_S1SetupResponseIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_ServedGUMMEIs;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_S1SetupResponseIEs__value_PR_ServedGUMMEIs;

  // memset for gcc 4.8.4 instead of {0}, servedGUMMEI.servedPLMNs
  servedGUMMEI = calloc(1, sizeof *servedGUMMEI);

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
      S1AP_PLMNidentity_t                    *plmn = NULL;
      /*
       * FIXME: free object from list once encoded
       */
      plmn = calloc (1, sizeof (*plmn));
      MCC_MNC_TO_PLMNID (mme_config.served_tai.plmn_mcc[i], mme_config.served_tai.plmn_mnc[i], mme_config.served_tai.plmn_mnc_len[i], plmn);
      ASN_SEQUENCE_ADD (&servedGUMMEI->servedPLMNs.list, plmn);
    }
  }

  for (i = 0; i < mme_config.gummei.nb; i++) {
    S1AP_MME_Group_ID_t                    *mme_gid = NULL;
    S1AP_MME_Code_t                        *mmec = NULL;

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
  ASN_SEQUENCE_ADD(&ie->value.choice.ServedGUMMEIs.list, servedGUMMEI);

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  mme_config_unlock (&mme_config);
  /*
   * The MME is only serving E-UTRAN RAT, so the list contains only one element
   */

  enc_rval = s1ap_mme_encode_pdu (&pdu, &buffer, &length);

  /*
   * Failed to encode s1 setup response...
   */
  if (enc_rval < 0) {
    OAILOG_DEBUG (LOG_S1AP, "Removed eNB %d\n", enb_association->sctp_assoc_id);
    hashtable_ts_free (&g_s1ap_enb_coll, enb_association->sctp_assoc_id);
  } else {
    /*
     * Consider the response as sent. S1AP is ready to accept UE contexts
     */
    enb_association->s1_state = S1AP_READY;
  }

  /*
   * Non-UE signalling -> stream 0
   */
  bstring b = blk2bstr(buffer, length);
  free(buffer);
  rc = s1ap_mme_itti_send_sctp_request (&b, enb_association->sctp_assoc_id, 0, INVALID_MME_UE_S1AP_ID);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_ue_cap_indication (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t *pdu)
{
  ue_description_t                       *ue_ref_p = NULL;
  S1AP_UECapabilityInfoIndication_t      *container;
  S1AP_UECapabilityInfoIndicationIEs_t   *ie = NULL;
  int                                     rc = RETURNok;
  mme_ue_s1ap_id_t                        mme_ue_s1ap_id = 0;
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;

  OAILOG_FUNC_IN (LOG_S1AP);
  DevAssert (pdu != NULL);
  container = &pdu->choice.initiatingMessage.value.choice.UECapabilityInfoIndication;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_UECapabilityInfoIndicationIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_UECapabilityInfoIndicationIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);


  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT "\n", (uint32_t) mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != enb_ue_s1ap_id) {
    OAILOG_DEBUG (LOG_S1AP, "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT ", received: " ENB_UE_S1AP_ID_FMT "\n",
                     ue_ref_p->enb_ue_s1ap_id, (uint32_t)enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  /*
   * Just display a warning when message received over wrong stream
   */
  if (ue_ref_p->sctp_stream_recv != stream) {
    OAILOG_ERROR (LOG_S1AP, "Received ue capability indication for "
                "(MME UE S1AP ID/eNB UE S1AP ID) (" MME_UE_S1AP_ID_FMT "/" ENB_UE_S1AP_ID_FMT ") over wrong stream "
                "expecting %u, received on %u\n", (uint32_t) mme_ue_s1ap_id, ue_ref_p->enb_ue_s1ap_id, ue_ref_p->sctp_stream_recv, stream);
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

    S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_UECapabilityInfoIndicationIEs_t, ie, container,
                               S1AP_ProtocolIE_ID_id_UERadioCapability, true);
    ue_cap_ind_p->radio_capabilities_length = ie->value.choice.UERadioCapability.size;
    DevCheck (ue_cap_ind_p->radio_capabilities_length < S1AP_UE_RADIOCAPABILITY_MAX_SIZE, S1AP_UE_RADIOCAPABILITY_MAX_SIZE, ue_cap_ind_p->radio_capabilities_length, 0);
    memcpy (ue_cap_ind_p->radio_capabilities, ie->value.choice.UERadioCapability.buf, ue_cap_ind_p->radio_capabilities_length);

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
    S1AP_S1AP_PDU_t *pdu)
{
  S1AP_InitialContextSetupResponse_t     *container;
  S1AP_InitialContextSetupResponseIEs_t  *ie = NULL;
  S1AP_E_RABSetupItemCtxtSUResIEs_t      *eRABSetupItemCtxtSURes_p = NULL;
  S1AP_E_RABSetupItemCtxtSURes_t	     *e_rab_setup_item_ctxt_su_res = NULL;
  ue_description_t                       *ue_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;
  mme_ue_s1ap_id_t                        mme_ue_s1ap_id = 0;
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;

  OAILOG_FUNC_IN (LOG_S1AP);

  container = &pdu->choice.successfulOutcome.value.choice.InitialContextSetupResponse;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_InitialContextSetupResponseIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_InitialContextSetupResponseIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list ((uint32_t) mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT " %u(10)\n",
                      (uint32_t) mme_ue_s1ap_id, (uint32_t) mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != enb_ue_s1ap_id) {
    OAILOG_DEBUG (LOG_S1AP, "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT " %u(10), received: 0x%06x %u(10)\n",
                ue_ref_p->enb_ue_s1ap_id, ue_ref_p->enb_ue_s1ap_id, (uint32_t) enb_ue_s1ap_id, (uint32_t) enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_InitialContextSetupResponseIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_E_RABSetupListCtxtSURes, true);
  if (ie->value.choice.E_RABSetupListCtxtSURes.list.count < 1) {

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
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  memset ((void *)&message_p->ittiMsg.mme_app_initial_context_setup_rsp, 0, sizeof (itti_mme_app_initial_context_setup_rsp_t));


  itti_mme_app_initial_context_setup_rsp_t * initial_context_setup_rsp = NULL;
  initial_context_setup_rsp = &message_p->ittiMsg.mme_app_initial_context_setup_rsp;

  initial_context_setup_rsp->ue_id = ue_ref_p->mme_ue_s1ap_id;
  /** Add here multiple bearers. */
  bool test = false;
  initial_context_setup_rsp->bcs_to_be_modified.num_bearer_context = ie->value.choice.E_RABSetupListCtxtSURes.list.count;
  for (int item = 0; item < ie->value.choice.E_RABSetupListCtxtSURes.list.count; item++) {
    int item2 = item;
    /*
     * Bad, very bad cast...
     */
    eRABSetupItemCtxtSURes_p = (S1AP_E_RABSetupItemCtxtSUResIEs_t *)
        ie->value.choice.E_RABSetupListCtxtSURes.list.array[item];

    e_rab_setup_item_ctxt_su_res = &eRABSetupItemCtxtSURes_p->value.choice.E_RABSetupItemCtxtSURes;
//
//    if(e_rab_setup_item_ctxt_su_res->e_RAB_ID == 7){
//    	test = true;
//    	initial_context_setup_rsp->bcs_to_be_modified.num_bearer_context--;
//    	continue;
//    }
//    if(test == true){
//    	if(item2 > 0)
//    		item2--;
//    }

    initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].eps_bearer_id = e_rab_setup_item_ctxt_su_res->e_RAB_ID;
    initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.teid = htonl (*((uint32_t *) e_rab_setup_item_ctxt_su_res->gTP_TEID.buf));
    bstring transport_address = blk2bstr(e_rab_setup_item_ctxt_su_res->transportLayerAddress.buf, e_rab_setup_item_ctxt_su_res->transportLayerAddress.size);

    /** Set the IP address from the FTEID. */
    if (4 == blength(transport_address)) {
    	initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv4 = 1;
    	memcpy(&initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv4_address, transport_address->data, blength(transport_address));
    } else if (16 == blength(transport_address)) {
    	initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv6 = 1;
    	memcpy(&initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv6_address, transport_address->data, blength(transport_address));
    } else {
    	AssertFatal(0, "TODO IP address %d bytes", blength(transport_address));
    }
    bdestroy_wrapper(&transport_address);
  }

  /** Get the failed bearers. */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_InitialContextSetupResponseIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_E_RABFailedToSetupListBearerSURes, false);
  if (ie) {
    S1AP_E_RABList_t *s1ap_e_rab_list = &ie->value.choice.E_RABList;
    for (int index = 0; index < s1ap_e_rab_list->list.count; index++) {
      S1AP_E_RABItem_t * erab_item = (S1AP_E_RABItem_t *)s1ap_e_rab_list->list.array[index];
      initial_context_setup_rsp->e_rab_release_list.item[initial_context_setup_rsp->e_rab_release_list.no_of_items].e_rab_id  = erab_item->e_RAB_ID;
      initial_context_setup_rsp->e_rab_release_list.item[initial_context_setup_rsp->e_rab_release_list.no_of_items].cause     = erab_item->cause;
      initial_context_setup_rsp->e_rab_release_list.no_of_items++;
    }
  }
//
//  if(test){
//	  /** Trigger failed bearer  (ebi==7). todo: comment out */
//	  initial_context_setup_rsp->e_rab_release_list.item[0].e_rab_id  = 7;
//	  initial_context_setup_rsp->e_rab_release_list.item[0].cause.present = S1AP_Cause_PR_radioNetwork;
//	  initial_context_setup_rsp->e_rab_release_list.item[0].cause.choice.radioNetwork = S1AP_CauseRadioNetwork_radio_resources_not_available;
//	  initial_context_setup_rsp->e_rab_release_list.no_of_items++;
//  }

  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_ue_context_release_request (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t *pdu)
{
  S1AP_UEContextReleaseRequest_t         *container;
  S1AP_UEContextReleaseRequest_IEs_t     *ie = NULL;
  ue_description_t                       *ue_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  S1AP_Cause_PR                           cause_type;
  long                                    cause_value;
  int                                     rc = RETURNok;
  mme_ue_s1ap_id_t                        mme_ue_s1ap_id = 0;
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;

  OAILOG_FUNC_IN (LOG_S1AP);

  container = &pdu->choice.initiatingMessage.value.choice.UEContextReleaseRequest;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_UEContextReleaseRequest_IEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_UEContextReleaseRequest_IEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);

  // Log the Cause Type and Cause value
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_UEContextReleaseRequest_IEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_Cause, true);
  cause_type = ie->value.choice.Cause.present;

  switch (cause_type)
  {
    case S1AP_Cause_PR_radioNetwork:
      cause_value = ie->value.choice.Cause.choice.radioNetwork;
      OAILOG_DEBUG (LOG_S1AP, "UE CONTEXT RELEASE REQUEST with Cause_Type = Radio Network and Cause_Value = %ld\n", cause_value);
      break;

    case S1AP_Cause_PR_transport:
      cause_value = ie->value.choice.Cause.choice.transport;
      OAILOG_DEBUG (LOG_S1AP, "UE CONTEXT RELEASE REQUEST with Cause_Type = Transport and Cause_Value = %ld\n", cause_value);
      break;

    case S1AP_Cause_PR_nas:
      cause_value = ie->value.choice.Cause.choice.nas;
      OAILOG_DEBUG (LOG_S1AP, "UE CONTEXT RELEASE REQUEST with Cause_Type = NAS and Cause_Value = %ld\n", cause_value);
      break;

    case S1AP_Cause_PR_protocol:
      cause_value = ie->value.choice.Cause.choice.protocol;
      OAILOG_DEBUG (LOG_S1AP, "UE CONTEXT RELEASE REQUEST with Cause_Type = Protocol and Cause_Value = %ld\n", cause_value);
      break;

    case S1AP_Cause_PR_misc:
      cause_value = ie->value.choice.Cause.choice.misc;
      OAILOG_DEBUG (LOG_S1AP, "UE CONTEXT RELEASE REQUEST with Cause_Type = MISC and Cause_Value = %ld\n", cause_value);
      break;

    default:
      OAILOG_ERROR (LOG_S1AP, "UE CONTEXT RELEASE REQUEST with Invalid Cause_Type = %d\n", cause_type);
      OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  /* Fix - MME shall handle UE Context Release received from the eNB irrespective of the cause. And MME should release the S1-U bearers for the UE and move the UE to ECM idle mode.
  Cause can influence whether to preserve GBR bearers or not.Since, as of now EPC doesn't support dedicated bearers, it is don't care scenario till we add support for dedicated bearers.
  */

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) == NULL) {
    /*
     * MME doesn't know the MME UE S1AP ID provided.
     * * * * TODO
     */
    OAILOG_DEBUG (LOG_S1AP, "UE_CONTEXT_RELEASE_REQUEST ignored cause could not get context with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ",
           (uint32_t)mme_ue_s1ap_id, (uint32_t)enb_ue_s1ap_id);

    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  } else {
    if (ue_ref_p->enb_ue_s1ap_id == (enb_ue_s1ap_id & ENB_UE_S1AP_ID_MASK)) {
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
      S1AP_UE_CONTEXT_RELEASE_REQ (message_p).cause = ie->value.choice.Cause;
      rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
      OAILOG_FUNC_RETURN (LOG_S1AP, rc);
    } else {
      // TODO
      OAILOG_DEBUG (LOG_S1AP, "UE_CONTEXT_RELEASE_REQUEST ignored, cause mismatch enb_ue_s1ap_id: ctxt " ENB_UE_S1AP_ID_FMT " != request " ENB_UE_S1AP_ID_FMT " ",
    		  (uint32_t)ue_ref_p->enb_ue_s1ap_id, (uint32_t)enb_ue_s1ap_id);
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
  S1AP_S1AP_PDU_t                         pdu;
  S1AP_UEContextReleaseCommand_t         *out;
  S1AP_UEContextReleaseCommand_IEs_t     *ie = NULL;
  int                                     rc = RETURNok;
  S1AP_Cause_PR                           cause_type;
  long                                    cause_value;

  bool                                    expect_release_complete = true;
  MessageDef                             *message_p = NULL;

  OAILOG_FUNC_IN (LOG_S1AP);

  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1AP_S1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = S1AP_ProcedureCode_id_UEContextRelease;
  pdu.choice.initiatingMessage.criticality = S1AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = S1AP_InitiatingMessage__value_PR_UEContextReleaseCommand;
  out = &pdu.choice.initiatingMessage.value.choice.UEContextReleaseCommand;

  /*
   * Fill in ID pair, depending if a UE_REFERENCE exists or not.
   */
  ie = (S1AP_UEContextReleaseCommand_IEs_t *)calloc(1, sizeof(S1AP_UEContextReleaseCommand_IEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_UE_S1AP_IDs;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_UEContextReleaseCommand_IEs__value_PR_UE_S1AP_IDs;
  if(ue_ref_p){
    ie->value.choice.UE_S1AP_IDs.present = S1AP_UE_S1AP_IDs_PR_uE_S1AP_ID_pair;
    ie->value.choice.UE_S1AP_IDs.choice.uE_S1AP_ID_pair.mME_UE_S1AP_ID = mme_ue_s1ap_id;
    ie->value.choice.UE_S1AP_IDs.choice.uE_S1AP_ID_pair.eNB_UE_S1AP_ID = ue_ref_p->enb_ue_s1ap_id;
    ie->value.choice.UE_S1AP_IDs.choice.uE_S1AP_ID_pair.iE_Extensions = NULL;
  }else{
    ie->value.choice.UE_S1AP_IDs.present = S1AP_UE_S1AP_IDs_PR_mME_UE_S1AP_ID;
    ie->value.choice.UE_S1AP_IDs.choice.mME_UE_S1AP_ID = mme_ue_s1ap_id;
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  ie = (S1AP_UEContextReleaseCommand_IEs_t *)calloc(1, sizeof(S1AP_UEContextReleaseCommand_IEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_Cause;
  ie->criticality = S1AP_Criticality_ignore;
  ie->value.present = S1AP_UEContextReleaseCommand_IEs__value_PR_Cause;
  switch (cause) {
  case S1AP_NAS_DETACH:
    cause_type = S1AP_Cause_PR_nas;
    cause_value = S1AP_CauseNas_detach;
    break;
  case S1AP_INVALIDATE_NAS:
  case S1AP_NAS_NORMAL_RELEASE:
    cause_type = S1AP_Cause_PR_nas;
    cause_value = S1AP_CauseNas_unspecified;
    break;
  case S1AP_RADIO_EUTRAN_GENERATED_REASON:
    cause_type = S1AP_Cause_PR_radioNetwork;
    cause_value = S1AP_CauseRadioNetwork_release_due_to_eutran_generated_reason;
    break;
  case S1AP_RADIO_UNKNOWN_E_RAB_ID:
    cause_type = S1AP_Cause_PR_radioNetwork;
    cause_value = S1AP_CauseRadioNetwork_unknown_E_RAB_ID;
    break;
  case S1AP_INITIAL_CONTEXT_SETUP_FAILED:
    cause_type = S1AP_Cause_PR_radioNetwork;
    cause_value = S1AP_CauseRadioNetwork_unspecified;
    break;
  case S1AP_HANDOVER_CANCELLED:cause_type = S1AP_Cause_PR_radioNetwork;
    cause_value = S1AP_CauseRadioNetwork_handover_cancelled;
    if(!ue_ref_p)
      expect_release_complete = false;
    break;
  case S1AP_HANDOVER_FAILED:cause_type = S1AP_Cause_PR_radioNetwork;
    cause_value = S1AP_CauseRadioNetwork_ho_failure_in_target_EPC_eNB_or_target_system;
    break;
  case S1AP_SUCCESSFUL_HANDOVER: cause_type = S1AP_Cause_PR_radioNetwork;
    cause_value = S1AP_CauseRadioNetwork_successful_handover;
    break;
  /**
   * If the cause is an S1AP_NETWORK_ERROR or an S1AP_SYSTEM_FAILURE, we send just a UE-Context-Release-Request (and dismiss the complete).
   * No UE Reference will exist at that point.
   */
  case S1AP_NETWORK_ERROR:
  case S1AP_SYSTEM_FAILURE: cause_type = S1AP_Cause_PR_transport;
    cause_value = S1AP_CauseTransport_unspecified;
    expect_release_complete = false;
    break;
  default:
    AssertFatal(false, "Unknown cause for context release");
    break;
  }
  s1ap_mme_set_cause(&ie->value.choice.Cause, cause_type, cause_value);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (s1ap_mme_encode_pdu (&pdu, &buffer, &length) < 0) {
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  bstring b = blk2bstr(buffer, length);
  free(buffer);
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
      enb_s1ap_id_key_t enb_ue_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;
      MME_APP_ENB_S1AP_ID_KEY(enb_ue_s1ap_id_key, enb_ref_p->enb_id, ue_ref_p->enb_ue_s1ap_id);
      if (timer_setup (ue_ref_p->s1ap_ue_context_rel_timer.sec, 0,
          TASK_S1AP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void*)enb_ue_s1ap_id_key, &(ue_ref_p->s1ap_ue_context_rel_timer.id)) < 0) {
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
    S1AP_S1AP_PDU_t *pdu)
{
  S1AP_UEContextReleaseComplete_t        *container;
  S1AP_UEContextReleaseComplete_IEs_t    *ie = NULL;
  ue_description_t                       *ue_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  mme_ue_s1ap_id_t                        mme_ue_s1ap_id = 0;
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;

  OAILOG_FUNC_IN (LOG_S1AP);

  container = &pdu->choice.successfulOutcome.value.choice.UEContextReleaseComplete;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_UEContextReleaseComplete_IEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_UEContextReleaseComplete_IEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);

  enb_description_t * enb_ref = s1ap_is_enb_assoc_id_in_list (assoc_id);
  DevAssert(enb_ref);
  if ((ue_ref_p = s1ap_is_ue_enb_id_in_list (enb_ref, enb_ue_s1ap_id)) == NULL) {

    /*
     * MME doesn't know the MME UE S1AP ID provided.
     * This implies that UE context has already been deleted on the expiry of timer!
     * Ignore this message. The timer will trigger the cleanup in the MME_APP. EMM is again independent.
     */
    OAILOG_DEBUG (LOG_S1AP, " UE Context Release commplete:No S1 context. Ignore the message for ueid " MME_UE_S1AP_ID_FMT "\n", (uint32_t) mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
  }

  /*
   * eNB has sent a release complete message. We can safely remove UE context.
   * TODO: inform NAS and remove e-RABS.
   */
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_UE_CONTEXT_RELEASE_COMPLETE);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).mme_ue_s1ap_id = mme_ue_s1ap_id;
  S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).enb_ue_s1ap_id = enb_ue_s1ap_id;
  S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).enb_id         = ue_ref_p->enb->enb_id;
  S1AP_UE_CONTEXT_RELEASE_COMPLETE (message_p).sctp_assoc_id  = ue_ref_p->enb->sctp_assoc_id;
  if(ue_ref_p->s1_ue_state != S1AP_UE_WAITING_CRR){
	  OAILOG_ERROR(LOG_S1AP, " UE " MME_UE_S1AP_ID_FMT " with enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT" not in WAITING_CRR state, but %d.. continuing \n",
			  ue_ref_p->mme_ue_s1ap_id, ue_ref_p->enb_ue_s1ap_id, ue_ref_p->s1_ue_state);
  }
  s1ap_remove_ue (ue_ref_p);

  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_DEBUG (LOG_S1AP, "Removed UE " MME_UE_S1AP_ID_FMT "\n", (uint32_t) mme_ue_s1ap_id);
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_initial_context_setup_failure (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t *pdu)
{
  S1AP_InitialContextSetupFailure_t      *container;
  S1AP_InitialContextSetupFailureIEs_t   *ie = NULL;
  ue_description_t                       *ue_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  S1AP_Cause_PR                           cause_type;
  long                                    cause_value;
  int                                     rc = RETURNok;
  mme_ue_s1ap_id_t                        mme_ue_s1ap_id = 0;
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;

  OAILOG_FUNC_IN (LOG_S1AP);

  container = &pdu->choice.unsuccessfulOutcome.value.choice.InitialContextSetupFailure;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_InitialContextSetupFailureIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_InitialContextSetupFailureIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);


  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) == NULL) {
    /*
     * MME doesn't know the MME UE S1AP ID provided.
     */
    OAILOG_INFO (LOG_S1AP, "INITIAL_CONTEXT_SETUP_FAILURE ignored. No context with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " ",
           (uint32_t)mme_ue_s1ap_id, (uint32_t)enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != (enb_ue_s1ap_id & ENB_UE_S1AP_ID_MASK)) {
    // abnormal case. No need to do anything. Ignore the message
    OAILOG_DEBUG (LOG_S1AP, "INITIAL_CONTEXT_SETUP_FAILURE ignored, mismatch enb_ue_s1ap_id: ctxt " ENB_UE_S1AP_ID_FMT " != received " ENB_UE_S1AP_ID_FMT " ",
    		  (uint32_t)ue_ref_p->enb_ue_s1ap_id, (uint32_t)(enb_ue_s1ap_id & ENB_UE_S1AP_ID_MASK));

    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }
  // Pass this message to MME APP for necessary handling
  // Log the Cause Type and Cause value
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_InitialContextSetupFailureIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_Cause, true);
  cause_type = ie->value.choice.Cause.present;

  switch (cause_type)
  {
    case S1AP_Cause_PR_radioNetwork:
      cause_value = ie->value.choice.Cause.choice.radioNetwork;
      OAILOG_DEBUG (LOG_S1AP, "INITIAL_CONTEXT_SETUP_FAILURE with Cause_Type = Radio Network and Cause_Value = %ld\n", cause_value);
      break;

    case S1AP_Cause_PR_transport:
      cause_value = ie->value.choice.Cause.choice.transport;
      OAILOG_DEBUG (LOG_S1AP, "INITIAL_CONTEXT_SETUP_FAILURE with Cause_Type = Transport and Cause_Value = %ld\n", cause_value);
      break;

    case S1AP_Cause_PR_nas:
      cause_value = ie->value.choice.Cause.choice.nas;
      OAILOG_DEBUG (LOG_S1AP, "INITIAL_CONTEXT_SETUP_FAILURE with Cause_Type = NAS and Cause_Value = %ld\n", cause_value);
      break;

    case S1AP_Cause_PR_protocol:
      cause_value = ie->value.choice.Cause.choice.protocol;
      OAILOG_DEBUG (LOG_S1AP, "INITIAL_CONTEXT_SETUP_FAILURE with Cause_Type = Protocol and Cause_Value = %ld\n", cause_value);
      break;

    case S1AP_Cause_PR_misc:
      cause_value = ie->value.choice.Cause.choice.misc;
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
    S1AP_S1AP_PDU_t *pdu)
{
  OAILOG_FUNC_IN (LOG_S1AP);

  S1AP_PathSwitchRequest_t               *container = NULL;
  S1AP_PathSwitchRequestIEs_t            *ie = NULL;
  S1AP_E_RABToBeSwitchedDLItemIEs_t      *eRABToBeSwitchedDlItemIEs_p = NULL;

  ue_description_t                       *ue_ref_p = NULL;
  mme_ue_s1ap_id_t                        mme_ue_s1ap_id = 0;
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;
  MessageDef                             *message_p = NULL;
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

  container = &pdu->choice.initiatingMessage.value.choice.PathSwitchRequest;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PathSwitchRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_SourceMME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PathSwitchRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);


  OAILOG_DEBUG (LOG_S1AP, "Path Switch Request message received from eNB UE S1AP ID: " ENB_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) == NULL) {
    /*
     * The MME UE S1AP ID provided by eNB doesn't point to any valid UE.
     * * * * MME replies with a PATH SWITCH REQUEST FAILURE message and start operation
     * * * * as described in TS 36.413 [11].
     * * * * TODO
     */
    OAILOG_DEBUG (LOG_S1AP, "MME UE S1AP ID provided by eNB doesn't point to any valid UE: " MME_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id);
    s1ap_send_path_switch_request_failure(assoc_id, mme_ue_s1ap_id, enb_ue_s1ap_id, S1AP_Cause_PR_nas);
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
    S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PathSwitchRequestIEs_t, ie, container, S1AP_ProtocolIE_ID_id_EUTRAN_CGI, true);
    DevAssert(ie);
    DevAssert (ie->value.choice.EUTRAN_CGI.pLMNidentity.size == 3);
    TBCD_TO_PLMN_T(&ie->value.choice.EUTRAN_CGI.pLMNidentity, &ecgi.plmn);
    BIT_STRING_TO_CELL_IDENTITY (&ie->value.choice.EUTRAN_CGI.cell_ID, ecgi.cell_identity);

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

    S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PathSwitchRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_E_RABToBeSwitchedDLList, true);
    DevAssert(ie);
    S1AP_E_RABToBeSwitchedDLList_t	* e_rab_to_be_switched_dl_list = &ie->value.choice.E_RABToBeSwitchedDLList;

    path_switch_request->bcs_to_be_modified.num_bearer_context = e_rab_to_be_switched_dl_list->list.count;
    for (int item = 0; item < e_rab_to_be_switched_dl_list->list.count; item++) {
      /*
       * Bad, very bad cast...
       */
      eRABToBeSwitchedDlItemIEs_p = (S1AP_E_RABToBeSwitchedDLItemIEs_t *)e_rab_to_be_switched_dl_list->list.array[item];
      S1AP_E_RABToBeSwitchedDLItem_t *s1ap_e_rab_to_be_switched_dl_item = &eRABToBeSwitchedDlItemIEs_p->value.choice.E_RABToBeSwitchedDLItem;

      path_switch_request->bcs_to_be_modified.bearer_context[item].eps_bearer_id = s1ap_e_rab_to_be_switched_dl_item->e_RAB_ID;
      path_switch_request->bcs_to_be_modified.bearer_context[item].s1_eNB_fteid.teid = htonl (*((uint32_t *) s1ap_e_rab_to_be_switched_dl_item->gTP_TEID.buf));
      bstring transport_address = blk2bstr(s1ap_e_rab_to_be_switched_dl_item->transportLayerAddress.buf,
                                           s1ap_e_rab_to_be_switched_dl_item->transportLayerAddress.size);
      /** Set the IP address from the FTEID. */
      if (4 == blength(transport_address)) {
        path_switch_request->bcs_to_be_modified.bearer_context[item].s1_eNB_fteid.ipv4 = 1;
        memcpy(&path_switch_request->bcs_to_be_modified.bearer_context[item].s1_eNB_fteid.ipv4_address,
            transport_address->data, blength(transport_address));
      } else if (16 == blength(transport_address)) {
        path_switch_request->bcs_to_be_modified.bearer_context[item].s1_eNB_fteid.ipv6 = 1;
        memcpy(&path_switch_request->bcs_to_be_modified.bearer_context[item].s1_eNB_fteid.ipv6_address,
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

    itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
  }
  return RETURNerror;
}

//------------------------------------------------------------------------------
int s1ap_mme_handle_handover_preparation(const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream, S1AP_S1AP_PDU_t *pdu)
{
  S1AP_HandoverRequired_t               *container = NULL;
  S1AP_HandoverRequiredIEs_t            *ie = NULL;

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
  container = &pdu->choice.initiatingMessage.value.choice.HandoverRequired;


  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverRequiredIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverRequiredIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);


  OAILOG_DEBUG (LOG_S1AP, "Handover Required message received from eNB UE S1AP ID: " ENB_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) == NULL) {
    /**
     * The MME UE S1AP ID provided by eNB doesn't point to any valid UE.
     * * * * MME replies with a HANDOVER PREPARATION FAILURE message and start operation.
     * * * * as described in TS 36.413 [11].
     */
    OAILOG_ERROR(LOG_S1AP, "MME UE S1AP ID provided by source eNB doesn't point to any valid UE: " MME_UE_S1AP_ID_FMT "\n", enb_ue_s1ap_id);
    rc = s1ap_handover_preparation_failure(assoc_id, mme_ue_s1ap_id, enb_ue_s1ap_id, S1AP_Cause_PR_misc);
    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  }
  /*
   * No timers to be started. Also, the state does not need to be changed from UE_CONNECTED, since TS 36.413 8.4.1.2 specifies that MME may/can initiate some E-RAB Modification signaling towards the source
   * eNB before the Handover Procedure is completed. Handover restart won't be triggered by eNB.
   * Source eNB may restart the Handover. If the UE is still connected (Handover dismissed or still going on, it will send a HO-Preparation-Failure.
   */
  if(ue_ref_p->s1_ue_state  != S1AP_UE_CONNECTED){
    OAILOG_ERROR(LOG_S1AP, "UE: " MME_UE_S1AP_ID_FMT " is not in connected mode, instead %d \n", enb_ue_s1ap_id, ue_ref_p->s1_ue_state);
    rc = s1ap_handover_preparation_failure(assoc_id, mme_ue_s1ap_id, enb_ue_s1ap_id, S1AP_Cause_PR_misc);
    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  }
  /** Get the target tai information. */
  // Mandatory
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverRequiredIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_TargetID, true);

  const S1AP_TargetID_t	* const target_id = &ie->value.choice.TargetID;

  if(!target_id->choice.targeteNB_ID.selected_TAI.pLMNidentity.buf || !target_id->choice.targeteNB_ID.selected_TAI.tAC.buf
      || !target_id->choice.targeteNB_ID.global_ENB_ID.pLMNidentity.buf ){
    OAILOG_ERROR(LOG_S1AP, "Handover Required message for UE: " MME_UE_S1AP_ID_FMT " has no target_tai or enb_id information. \n", enb_ue_s1ap_id);
    rc = s1ap_handover_preparation_failure(assoc_id, mme_ue_s1ap_id, enb_ue_s1ap_id, S1AP_Cause_PR_misc);
    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  }
  memset((void*)&target_tai, 0, sizeof(tai_t));

  TBCD_TO_PLMN_T (&target_id->choice.targeteNB_ID.selected_TAI.pLMNidentity, &target_tai.plmn);
  target_tai.tac = htons(*((uint16_t *) target_id->choice.targeteNB_ID.selected_TAI.tAC.buf));

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
  TBCD_TO_PLMN_T (&target_id->choice.targeteNB_ID.global_ENB_ID.pLMNidentity, &enb_id_plmn);
  S1AP_HANDOVER_REQUIRED (message_p).global_enb_id.plmn = enb_id_plmn;

  // Macro eNB = 20 bits
  /** Get the enb id. */
  uint32_t enb_id = 0;
  if(target_id->choice.targeteNB_ID.global_ENB_ID.eNB_ID.present == S1AP_ENB_ID_PR_homeENB_ID){
    /** Target is a home eNB Id. */
    // Home eNB ID = 28 bits
    uint8_t *enb_id_buf = target_id->choice.targeteNB_ID.global_ENB_ID.eNB_ID.choice.homeENB_ID.buf;
    if (target_id->choice.targeteNB_ID.global_ENB_ID.eNB_ID.choice.homeENB_ID.size != 28) {
      //TODO: handle case were size != 28 -> notify ? reject ?
    }
    enb_id = (enb_id_buf[0] << 20) + (enb_id_buf[1] << 12) + (enb_id_buf[2] << 4) + ((enb_id_buf[3] & 0xf0) >> 4);
    S1AP_HANDOVER_REQUIRED (message_p).target_enb_type = TARGET_ID_HOME_ENB_ID;
    //    OAILOG_MESSAGE_ADD (context, "home eNB id: %07x", target_enb_id);
  }else{
    /** Target is a macro eNB Id (Macro eNB = 20 bits). */
    uint8_t *enb_id_buf = target_id->choice.targeteNB_ID.global_ENB_ID.eNB_ID.choice.macroENB_ID.buf;
    if (target_id->choice.targeteNB_ID.global_ENB_ID.eNB_ID.choice.macroENB_ID.size != 20) {
      //TODO: handle case were size != 28 -> notify ? reject ?
    }
    enb_id = (enb_id_buf[0] << 12) + (enb_id_buf[1] << 4) + ((enb_id_buf[2] & 0xf0) >> 4);
    //    OAILOG_MESSAGE_ADD (context, "macro eNB id: %05x", target_enb_id);
    S1AP_HANDOVER_REQUIRED (message_p).target_enb_type = TARGET_ID_MACRO_ENB_ID;
  }

  S1AP_HANDOVER_REQUIRED (message_p).global_enb_id.cell_identity.enb_id = enb_id;
  OAILOG_DEBUG(LOG_S1AP, "Successfully decoded targetID IE in Handover Required. \n");

  /**  Get the cause. */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverRequiredIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_Cause, true);
  S1AP_Cause_PR s1ap_cause_present = ie->value.choice.Cause.present;

  switch(s1ap_cause_present){
  case S1AP_Cause_PR_radioNetwork:
    S1AP_HANDOVER_REQUIRED (message_p).f_cause_type      = s1ap_cause_present;
    S1AP_HANDOVER_REQUIRED (message_p).f_cause_value     = ie->value.choice.Cause.choice.radioNetwork;
    break;
  default:
    OAILOG_WARNING(LOG_S1AP, "Cause type %d is not evaluated. Setting cause to radio.. \n", s1ap_cause_present);
    S1AP_HANDOVER_REQUIRED (message_p).f_cause_type      = S1AP_Cause_PR_radioNetwork;
    S1AP_HANDOVER_REQUIRED (message_p).f_cause_value     = 16;
    break;
  }
  /**
   * Set the container into a bstring.
   * todo:
   * We don't need to purge the bstring's, etc in MME_APP (of the newly allocated itti). Since it will automatically be purged with: s1ap_free_mme_decode_pdu.
   * All allocated messages will be purged, so either allocated new into bstring's or copy into stacked parameters.
   */
  // Mandatory
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverRequiredIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_Source_ToTarget_TransparentContainer, true);
  const S1AP_Source_ToTarget_TransparentContainer_t	 * const source_totarget_transparentcontainer = &ie->value.choice.Source_ToTarget_TransparentContainer;
  S1AP_HANDOVER_REQUIRED (message_p).eutran_source_to_target_container = blk2bstr(source_totarget_transparentcontainer->buf,
        source_totarget_transparentcontainer->size);

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
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_handover_cancel(
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t *pdu)
{
  S1AP_HandoverCancel_t                *container = NULL;
  S1AP_HandoverCancelIEs_t             *ie = NULL;
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
  container = &pdu->choice.initiatingMessage.value.choice.HandoverCancel;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverCancelIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverCancelIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);

  OAILOG_DEBUG (LOG_S1AP, "Handover Cancel message received from eNB UE S1AP ID: " ENB_UE_S1AP_ID_FMT ". \n", enb_ue_s1ap_id);
  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) == NULL) {
    /*
     * The MME UE S1AP ID provided by eNB doesn't point to any valid UE.
     * * * * MME replies with a HANDOVER PREPARATION FAILURE message and start operation.
     * * * * as described in TS 36.413 [11].
     */
    OAILOG_ERROR(LOG_S1AP, "MME UE S1AP ID provided by source eNB doesn't point to any valid UE: " MME_UE_S1AP_ID_FMT ". Continuing with the HO-Cancellation (impl. detach). \n", mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  /** Not changing any ue_reference properties. */
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_HANDOVER_CANCEL);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  memset ((void *)&message_p->ittiMsg.s1ap_handover_cancel, 0, sizeof (itti_s1ap_handover_cancel_t));
  S1AP_HANDOVER_CANCEL (message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
  S1AP_HANDOVER_CANCEL (message_p).enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;
  S1AP_HANDOVER_CANCEL (message_p).assoc_id = ue_ref_p->enb->sctp_assoc_id;
  /** Ignore the cause. */
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_handover_notification(
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t *pdu)
{
  S1AP_HandoverNotify_t                 *container = NULL;
  S1AP_HandoverNotifyIEs_t              *ie = NULL;
  ue_description_t                      *ue_ref_p = NULL;
  enb_ue_s1ap_id_t                       enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                       mme_ue_s1ap_id = 0;
  MessageDef                            *message_p = NULL;
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
  container = &pdu->choice.initiatingMessage.value.choice.HandoverNotify;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverNotifyIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverNotifyIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);


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
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverNotifyIEs_t, ie, container, S1AP_ProtocolIE_ID_id_TAI, true);
  DevAssert(ie);
  OCTET_STRING_TO_TAC (&ie->value.choice.TAI.tAC, tai.tac);
  DevAssert (ie->value.choice.TAI.pLMNidentity.size == 3);
  TBCD_TO_PLMN_T(&ie->value.choice.TAI.pLMNidentity, &tai.plmn);

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverNotifyIEs_t, ie, container, S1AP_ProtocolIE_ID_id_EUTRAN_CGI, true);
  DevAssert(ie);
  DevAssert (ie->value.choice.EUTRAN_CGI.pLMNidentity.size == 3);
  TBCD_TO_PLMN_T(&ie->value.choice.EUTRAN_CGI.pLMNidentity, &ecgi.plmn);
  BIT_STRING_TO_CELL_IDENTITY (&ie->value.choice.EUTRAN_CGI.cell_ID, ecgi.cell_identity);
  /** Set the ENB Id. */
  ecgi.cell_identity.enb_id = enb_ref->enb_id;

  // Currently just use this message send the MBR to SAE-GW and inform the source MME
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_HANDOVER_NOTIFY);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  memset ((void *)&message_p->ittiMsg.s1ap_handover_notify, 0, sizeof (itti_s1ap_handover_notify_t));
  /*
   * Bad, very bad cast...
   */
  S1AP_HANDOVER_NOTIFY (message_p).mme_ue_s1ap_id = mme_ue_s1ap_id;
  S1AP_HANDOVER_NOTIFY (message_p).enb_ue_s1ap_id = enb_ue_s1ap_id;
  S1AP_HANDOVER_NOTIFY (message_p).tai                    = tai;       /**< MME will not check if thats the correct target eNB. Just update the UE Context. */
  S1AP_HANDOVER_NOTIFY (message_p).cgi                    = ecgi;
  S1AP_HANDOVER_NOTIFY (message_p).assoc_id               = assoc_id;

  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
int s1ap_mme_handle_handover_resource_allocation_response(const sctp_assoc_id_t assoc_id,
                                        const sctp_stream_id_t stream,
                                        S1AP_S1AP_PDU_t *pdu)
{
  const S1AP_HandoverRequestAcknowledge_t     *container = NULL;
  const S1AP_HandoverRequestAcknowledgeIEs_t  *ie = NULL;

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
  container = &pdu->choice.successfulOutcome.value.choice.HandoverRequestAcknowledge;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverRequestAcknowledgeIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverRequestAcknowledgeIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);

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

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverRequestAcknowledgeIEs_t, ie, container, S1AP_ProtocolIE_ID_id_E_RABAdmittedList, false);
  const S1AP_E_RABAdmittedList_t * e_rab_admitted_list = NULL; // List of s1ap_E_RABAdmittedItem
	if (ie) {
	  e_rab_admitted_list = &ie->value.choice.E_RABAdmittedList;
	}

  if (!(e_rab_admitted_list) || (e_rab_admitted_list->list.count < 1)) {
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


  S1AP_HANDOVER_REQUEST_ACKNOWLEDGE (message_p).mme_ue_s1ap_id = mme_ue_s1ap_id;
  S1AP_HANDOVER_REQUEST_ACKNOWLEDGE (message_p).enb_ue_s1ap_id = ue_ref_p->enb_ue_s1ap_id;

  itti_s1ap_handover_request_acknowledge_t * s1ap_handover_request_acknowledge = NULL;
  s1ap_handover_request_acknowledge = &message_p->ittiMsg.s1ap_handover_request_acknowledge;
  s1ap_handover_request_acknowledge->bcs_to_be_modified.num_bearer_context = e_rab_admitted_list->list.count;
  for (int item = 0; item < e_rab_admitted_list->list.count; item++) {
    /*
     * Bad, very bad cast...
     */
    const S1AP_E_RABAdmittedItem_t * const s1ap_e_rabadmitteditem =
    		&(((S1AP_E_RABAdmittedItemIEs_t *)e_rab_admitted_list->list.array[item])->value.choice.E_RABAdmittedItem);
    s1ap_handover_request_acknowledge->bcs_to_be_modified.bearer_context[item].eps_bearer_id = s1ap_e_rabadmitteditem->e_RAB_ID;
    s1ap_handover_request_acknowledge->bcs_to_be_modified.bearer_context[item].s1_eNB_fteid.teid = htonl (*((uint32_t *) s1ap_e_rabadmitteditem->gTP_TEID.buf));
    bstring transport_address  =
        blk2bstr(s1ap_e_rabadmitteditem->transportLayerAddress.buf, s1ap_e_rabadmitteditem->transportLayerAddress.size);
    /** Set the IP address from the FTEID. */
    if (4 == blength(transport_address)) {
      s1ap_handover_request_acknowledge->bcs_to_be_modified.bearer_context[item].s1_eNB_fteid.ipv4 = 1;
      memcpy(&s1ap_handover_request_acknowledge->bcs_to_be_modified.bearer_context[item].s1_eNB_fteid.ipv4_address,
          transport_address->data, blength(transport_address));
    } else if (16 == blength(transport_address)) {
      s1ap_handover_request_acknowledge->bcs_to_be_modified.bearer_context[item].s1_eNB_fteid.ipv6 = 1;
      memcpy(&s1ap_handover_request_acknowledge->bcs_to_be_modified.bearer_context[item].s1_eNB_fteid.ipv6_address,
          transport_address->data,
          blength(transport_address));
    } else {
      AssertFatal(0, "TODO IP address %d bytes", blength(transport_address));
    }
    bdestroy_wrapper(&transport_address);
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverRequestAcknowledgeIEs_t, ie, container, S1AP_ProtocolIE_ID_id_E_RABFailedToSetupListHOReqAck, false);
	if (ie) {
	  const S1AP_E_RABFailedtoSetupListHOReqAck_t * const e_rab_failed_to_setup_list = &ie->value.choice.E_RABFailedtoSetupListHOReqAck;
    /** Get the failed bearers. */
    for (int index = 0; index < e_rab_failed_to_setup_list->list.count; index++) {
      const S1AP_E_RABFailedtoSetupItemHOReqAckIEs_t * const e_rab_failed_to_setup_item_ies = (const S1AP_E_RABFailedtoSetupItemHOReqAckIEs_t * const)e_rab_failed_to_setup_list->list.array[index];
      const S1AP_E_RABFailedToSetupItemHOReqAck_t * const e_rab_failed_to_setup_item = &e_rab_failed_to_setup_item_ies->value.choice.E_RABFailedToSetupItemHOReqAck;
      s1ap_handover_request_acknowledge->e_rab_release_list.item[index].e_rab_id  = e_rab_failed_to_setup_item->e_RAB_ID;
      s1ap_handover_request_acknowledge->e_rab_release_list.item[index].cause     = e_rab_failed_to_setup_item->cause;
      s1ap_handover_request_acknowledge->e_rab_release_list.no_of_items++;
    }
  }
  /**
   * Set the transparent container as a bstring.
   * The method ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_S1AP_Target_ToSource_TransparentContainer, &s1ap_HandoverRequestAcknowledgeIEs->target_ToSource_TransparentContainer)
   * is assumed to remove the OCTET_STRING.
   * todo: ask Lionel if thats correct? what is context only?
   */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverRequestAcknowledgeIEs_t, ie, container, S1AP_ProtocolIE_ID_id_Target_ToSource_TransparentContainer, true);
	const S1AP_Target_ToSource_TransparentContainer_t	 * const target_to_source_transparent_container = &ie->value.choice.Target_ToSource_TransparentContainer;

  S1AP_HANDOVER_REQUEST_ACKNOWLEDGE (message_p).target_to_source_eutran_container = blk2bstr((void*)target_to_source_transparent_container->buf,
      target_to_source_transparent_container->size);
  // todo @ Lionel: do we need to run this method? Message deallocated later?
  // free_wrapper((void**) &(handover_request_acknowledge_pP->transparent_container.buf));

  ue_ref_p->s1_ue_state = S1AP_UE_CONNECTED;

  /** Not checking if the Target-to-Source Transparent container exists. */
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_handover_resource_allocation_failure(
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t *pdu)
{
  S1AP_HandoverFailure_t                   *container = NULL;
  S1AP_HandoverFailureIEs_t                *ie = NULL;
  mme_ue_s1ap_id_t                          mme_ue_s1ap_id = 0;
  MessageDef                               *message_p = NULL;
  int                                       rc = RETURNok;
  long                                      cause_value;
  OAILOG_FUNC_IN (LOG_S1AP);

  /** Get the Failure Message. */
  container = &pdu->choice.unsuccessfulOutcome.value.choice.HandoverFailure;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverFailureIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;


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
  handover_failure_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
//  handover_failure_p->enb_ue_s1ap_id = INVALID_ENB_UE_S1AP_ID;

  /** Choice. */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_HandoverFailureIEs_t, ie, container, S1AP_ProtocolIE_ID_id_Cause, true);


  S1AP_Cause_PR cause_type = ie->value.choice.Cause.present;

  switch (cause_type)
  {
  case S1AP_Cause_PR_radioNetwork:
    cause_value = ie->value.choice.Cause.choice.radioNetwork;
    OAILOG_DEBUG (LOG_S1AP, "HANDOVER_FAILURE with Cause_Type = Radio Network and Cause_Value = %ld\n", cause_value);
    break;

  case S1AP_Cause_PR_transport:
    cause_value = ie->value.choice.Cause.choice.transport;
    OAILOG_DEBUG (LOG_S1AP, "HANDOVER_FAILURE with Cause_Type = Transport and Cause_Value = %ld\n", cause_value);
    break;

  case S1AP_Cause_PR_nas:
    cause_value = ie->value.choice.Cause.choice.nas;
    OAILOG_DEBUG (LOG_S1AP, "HANDOVER_FAILURE with Cause_Type = NAS and Cause_Value = %ld\n", cause_value);
    break;

  case S1AP_Cause_PR_protocol:
    cause_value = ie->value.choice.Cause.choice.protocol;
    OAILOG_DEBUG (LOG_S1AP, "HANDOVER_FAILURE with Cause_Type = Protocol and Cause_Value = %ld\n", cause_value);
    break;

  case S1AP_Cause_PR_misc:
    cause_value = ie->value.choice.Cause.choice.misc;
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
  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  /** No UE context to release. */
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
/**
 * ENB Status Transfer.
 * Contains Transparent Data which is forwarded to the target eNB without state change.
 */
int
s1ap_mme_handle_enb_status_transfer(
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t *pdu)
{

  S1AP_ENBStatusTransfer_t                 *container = NULL;
  S1AP_ENBStatusTransferIEs_t              *ie = NULL;

  ue_description_t                         *ue_ref_p = NULL;
  enb_ue_s1ap_id_t                          enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                          mme_ue_s1ap_id = 0;
  MessageDef                               *message_p = NULL;

  //Request IEs:
  //S1ap-ENB-UE-S1AP-ID
  //S1ap-MME-UE-S1AP-ID
  //S1ap-enb-StatusTransfer-TransparentContainer

  OAILOG_FUNC_IN (LOG_S1AP);
  container = &pdu->choice.initiatingMessage.value.choice.ENBStatusTransfer;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_ENBStatusTransferIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_ENBStatusTransferIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);

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

  enb_status_transfer_p->status_transfer_bearer_list = calloc(1, sizeof(status_transfer_bearer_list_t));
  // Mandatory
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_ENBStatusTransferIEs_t, ie, container, S1AP_ProtocolIE_ID_id_eNB_StatusTransfer_TransparentContainer, true);
  const S1AP_ENB_StatusTransfer_TransparentContainer_t	 * const enb_status_transfer_transparent_container = &ie->value.choice.ENB_StatusTransfer_TransparentContainer;

  enb_status_transfer_p->status_transfer_bearer_list->num_bearers = enb_status_transfer_transparent_container->bearers_SubjectToStatusTransferList.list.count;

  for(int i = 0; i < enb_status_transfer_transparent_container->bearers_SubjectToStatusTransferList.list.count; i++) {
	  const S1AP_Bearers_SubjectToStatusTransfer_ItemIEs_t  * const  bearers_subject_to_status_transfer_item_ies =
	                       (const S1AP_Bearers_SubjectToStatusTransfer_ItemIEs_t  * const)enb_status_transfer_transparent_container->bearers_SubjectToStatusTransferList.list.array[i];
	  enb_status_transfer_p->status_transfer_bearer_list->bearerStatusTransferList[i].bsc_ul.hfn_count  = bearers_subject_to_status_transfer_item_ies->value.choice.Bearers_SubjectToStatusTransfer_Item.uL_COUNTvalue.hFN;
	  enb_status_transfer_p->status_transfer_bearer_list->bearerStatusTransferList[i].bsc_ul.pdcp_count = bearers_subject_to_status_transfer_item_ies->value.choice.Bearers_SubjectToStatusTransfer_Item.uL_COUNTvalue.pDCP_SN;
	  enb_status_transfer_p->status_transfer_bearer_list->bearerStatusTransferList[i].bsc_dl.hfn_count  = bearers_subject_to_status_transfer_item_ies->value.choice.Bearers_SubjectToStatusTransfer_Item.dL_COUNTvalue.hFN;
	  enb_status_transfer_p->status_transfer_bearer_list->bearerStatusTransferList[i].bsc_dl.pdcp_count = bearers_subject_to_status_transfer_item_ies->value.choice.Bearers_SubjectToStatusTransfer_Item.dL_COUNTvalue.pDCP_SN;
	  enb_status_transfer_p->status_transfer_bearer_list->bearerStatusTransferList[i].ebi = bearers_subject_to_status_transfer_item_ies->value.choice.Bearers_SubjectToStatusTransfer_Item.e_RAB_ID;
	  if(bearers_subject_to_status_transfer_item_ies->value.choice.Bearers_SubjectToStatusTransfer_Item.receiveStatusofULPDCPSDUs){
		  OAILOG_WARNING(LOG_S1AP, "ENB_STATUS Transfer Bearer Item optionals not decoded!\n");
//		  enb_status_transfer_p->status_transfer_bearer_list->bearerStatusTransferList[i].receiveStatusofULPDCPSDU = blk2bstr(bearers_subject_to_status_transfer_item_ies->value.choice.Bearers_SubjectToStatusTransfer_Item.receiveStatusofULPDCPSDUs->buf,
//				  bearers_subject_to_status_transfer_item_ies->value.choice.Bearers_SubjectToStatusTransfer_Item.receiveStatusofULPDCPSDUs->size);
	  }
  }

  /** Assuming that the OCTET-STRING will be freed automatically. */
  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
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
  uint32_t i = arg->current_ue_index;
  if (ue_ref_p) {
    S1AP_ENB_INITIATED_RESET_REQ (arg->message_p).ue_to_reset_list[i].mme_ue_s1ap_id = &(ue_ref_p->mme_ue_s1ap_id);
    S1AP_ENB_INITIATED_RESET_REQ (arg->message_p).ue_to_reset_list[i].enb_ue_s1ap_id = (enb_ue_s1ap_id_t*)ue_ref_p; /**< Should be the address of the id. */
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
    	hashtable_ts_free (&g_s1ap_enb_coll, enb_association->sctp_assoc_id);
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
  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_erab_setup_response (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t *pdu)
{
  OAILOG_FUNC_IN (LOG_S1AP);
  S1AP_E_RABSetupResponse_t              *container = NULL;
  S1AP_E_RABSetupResponseIEs_t           *ie = NULL;
  ue_description_t                       *ue_ref_p  = NULL;
  MessageDef                             *message_p = NULL;
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                        mme_ue_s1ap_id = 0;
  int                                     rc = RETURNok;

  container = &pdu->choice.successfulOutcome.value.choice.E_RABSetupResponse;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABSetupResponseIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABSetupResponseIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT "\n", mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != enb_ue_s1ap_id) {
    OAILOG_DEBUG (LOG_S1AP, "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT ", received: " ENB_UE_S1AP_ID_FMT "\n",
                ue_ref_p->enb_ue_s1ap_id, enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_E_RAB_SETUP_RSP);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  S1AP_E_RAB_SETUP_RSP (message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
  S1AP_E_RAB_SETUP_RSP (message_p).e_rab_setup_list.no_of_items = 0;
  S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.no_of_items = 0;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABSetupResponseIEs_t, ie, container,
      S1AP_ProtocolIE_ID_id_E_RABSetupListBearerSURes, false);
  if (ie && ie->value.choice.E_RABSetupListBearerSURes.list.size >= 1) {
    int num_erab = ie->value.choice.E_RABSetupListBearerSURes.list.count;
    for (int index = 0; index < num_erab; index++) {
      S1AP_E_RABSetupItemBearerSUResIEs_t * erab_setup_item =
          (S1AP_E_RABSetupItemBearerSUResIEs_t*)ie->value.choice.E_RABSetupListBearerSURes.list.array[index];
      S1AP_E_RABSetupItemBearerSURes_t * e_rab_setup_item_bearer_su_res = &erab_setup_item->value.choice.E_RABSetupItemBearerSURes;

      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_setup_list.item[index].e_rab_id = e_rab_setup_item_bearer_su_res->e_RAB_ID;
      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_setup_list.item[index].transport_layer_address =
          blk2bstr(e_rab_setup_item_bearer_su_res->transportLayerAddress.buf, e_rab_setup_item_bearer_su_res->transportLayerAddress.size);
      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_setup_list.item[index].gtp_teid = htonl (*((uint32_t *) e_rab_setup_item_bearer_su_res->gTP_TEID.buf));
      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_setup_list.no_of_items += 1;
    }
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABSetupResponseIEs_t, ie, container,
      S1AP_ProtocolIE_ID_id_E_RABFailedToSetupListBearerSURes, false);
  if (ie) {
    int num_erab = ie->value.choice.E_RABList.list.count;
    for (int index = 0; index < num_erab; index++) {
      S1AP_E_RABItem_t * erab_item = (S1AP_E_RABItem_t *)ie->value.choice.E_RABList.list.array[index];
      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.item[index].e_rab_id = erab_item->e_RAB_ID;
      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.item[index].cause = erab_item->cause;
      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.no_of_items += 1;
    }
  }

  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_erab_modify_response (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t *pdu)
{
  OAILOG_FUNC_IN (LOG_S1AP);
  S1AP_E_RABModifyResponse_t             *container = NULL;
  S1AP_E_RABModifyResponseIEs_t          *ie = NULL;
  ue_description_t                       *ue_ref_p  = NULL;
  MessageDef                             *message_p = NULL;
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                        mme_ue_s1ap_id = 0;
  int                                     rc = RETURNok;

  container = &pdu->choice.successfulOutcome.value.choice.E_RABModifyResponse;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABModifyResponseIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABModifyResponseIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list ((uint32_t) mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT "\n", (mme_ue_s1ap_id_t)mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != enb_ue_s1ap_id) {
    OAILOG_DEBUG (LOG_S1AP, "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT ", received: " ENB_UE_S1AP_ID_FMT "\n",
                ue_ref_p->enb_ue_s1ap_id, enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_E_RAB_MODIFY_RSP);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  S1AP_E_RAB_MODIFY_RSP (message_p).mme_ue_s1ap_id = ue_ref_p->mme_ue_s1ap_id;
  S1AP_E_RAB_MODIFY_RSP (message_p).e_rab_modify_list.no_of_items = 0;
  S1AP_E_RAB_MODIFY_RSP (message_p).e_rab_failed_to_modify_list.no_of_items = 0;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABModifyResponseIEs_t, ie, container, S1AP_ProtocolIE_ID_id_E_RABModifyListBearerModRes, false);
  if (ie) {
  	const S1AP_E_RABModifyListBearerModRes_t * const e_rab_modify_list_bearer_mod_res = &ie->value.choice.E_RABModifyListBearerModRes;

    int num_erab = e_rab_modify_list_bearer_mod_res->list.count;
    for (int index = 0; index < num_erab; index++) {
      const S1AP_E_RABModifyItemBearerModResIEs_t * const e_rab_setup_item_bearer_su_res_ies = (S1AP_E_RABModifyItemBearerModResIEs_t*)e_rab_modify_list_bearer_mod_res->list.array[index];
      const S1AP_E_RABModifyItemBearerModRes_t * const erab_item = &e_rab_setup_item_bearer_su_res_ies->value.choice.E_RABModifyItemBearerModRes;
      S1AP_E_RAB_MODIFY_RSP (message_p).e_rab_modify_list.item[index].e_rab_id = erab_item->e_RAB_ID;
      S1AP_E_RAB_MODIFY_RSP (message_p).e_rab_modify_list.no_of_items += 1;
    }
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABModifyResponseIEs_t, ie, container, S1AP_ProtocolIE_ID_id_E_RABFailedToModifyList, false);
  if (ie) {
  	const S1AP_E_RABList_t * const e_rab_failed_list = &ie->value.choice.E_RABList;

    int num_erab = e_rab_failed_list->list.count;
    for (int index = 0; index < num_erab; index++) {
      const S1AP_E_RABItemIEs_t * const erab_item_ies = (S1AP_E_RABItemIEs_t *)e_rab_failed_list->list.array[index];
      const S1AP_E_RABItem_t * const erab_item = (S1AP_E_RABItem_t *)&erab_item_ies->value.choice.E_RABItem;
      S1AP_E_RAB_MODIFY_RSP (message_p).e_rab_failed_to_modify_list.item[index].e_rab_id = erab_item->e_RAB_ID;
      S1AP_E_RAB_MODIFY_RSP (message_p).e_rab_failed_to_modify_list.item[index].cause = erab_item->cause;
      S1AP_E_RAB_MODIFY_RSP (message_p).e_rab_failed_to_modify_list.no_of_items += 1;
    }
  }

  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_erab_release_response (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t *pdu)
{
  OAILOG_FUNC_IN (LOG_S1AP);
  S1AP_E_RABReleaseResponse_t            *container = NULL;
  S1AP_E_RABReleaseResponseIEs_t         *ie = NULL;
  ue_description_t                       *ue_ref_p  = NULL;
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                        mme_ue_s1ap_id = 0;
  int                                     rc = RETURNok;

  container = &pdu->choice.successfulOutcome.value.choice.E_RABReleaseResponse;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABReleaseResponseIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABReleaseResponseIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);

  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list ((uint32_t) mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT "\n", (mme_ue_s1ap_id_t)mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != enb_ue_s1ap_id) {
    OAILOG_DEBUG (LOG_S1AP, "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT ", received: " ENB_UE_S1AP_ID_FMT "\n",
                ue_ref_p->enb_ue_s1ap_id, (enb_ue_s1ap_id_t)enb_ue_s1ap_id);
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
//      S1AP_E_RABSetupItemBearerSURes_t * erab_setup_item =
//          (S1AP_E_RABSetupItemBearerSURes_t*)s1ap_E_RABSetupResponseIEs_p->e_RABSetupListBearerSURes.s1ap_E_RABSetupItemBearerSURes.array[index];
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
//      S1AP_E_RABItem_t * erab_item = (S1AP_E_RABItem_t *)s1ap_E_RABSetupResponseIEs_p->e_RABFailedToSetupListBearerSURes.s1ap_E_RABItem.array[index];
//      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.item[index].e_rab_id = erab_item->e_RAB_ID;
//      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.item[index].cause = erab_item->cause;
//      S1AP_E_RAB_SETUP_RSP (message_p).e_rab_failed_to_setup_list.no_of_items += 1;
//    }
//  }
//
//  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_erab_release_indication (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t *pdu)
{
  OAILOG_FUNC_IN (LOG_S1AP);
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                        mme_ue_s1ap_id = 0;
  int                                     rc = RETURNok;
  S1AP_E_RABReleaseIndication_t          *container = NULL;
  S1AP_E_RABReleaseIndicationIEs_t       *ie = NULL;
  ue_description_t                       *ue_ref_p  = NULL;
  MessageDef                             *message_p = NULL;

  container = &pdu->choice.initiatingMessage.value.choice.E_RABReleaseIndication;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABReleaseIndicationIEs_t, ie, container, S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABReleaseIndicationIEs_t, ie, container, S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);


  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT "\n", mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != enb_ue_s1ap_id) {
    OAILOG_DEBUG (LOG_S1AP, "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT ", received: " ENB_UE_S1AP_ID_FMT "\n",
                ue_ref_p->enb_ue_s1ap_id, enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_E_RAB_RELEASE_IND);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  S1AP_E_RAB_RELEASE_IND (message_p).mme_ue_s1ap_id  = ue_ref_p->mme_ue_s1ap_id;
  S1AP_E_RAB_RELEASE_IND (message_p).enb_ue_s1ap_id  = ue_ref_p->enb_ue_s1ap_id;
  S1AP_E_RAB_RELEASE_IND (message_p).enb_id  		 = ue_ref_p->enb->enb_id;

  /** Get the failed bearers. */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABReleaseIndicationIEs_t, ie, container, S1AP_ProtocolIE_ID_id_E_RABReleasedList, true);
  const S1AP_E_RABList_t * const e_rab_list = &ie->value.choice.E_RABList;
  int num_erab = e_rab_list->list.count;
  for (int index = 0; index < num_erab; index++) {
    const S1AP_E_RABItemIEs_t * const erab_item_ies = (S1AP_E_RABItemIEs_t *)e_rab_list->list.array[index];
    const S1AP_E_RABItem_t * const erab_item = (S1AP_E_RABItem_t *)&erab_item_ies->value.choice.E_RABItem;
    S1AP_E_RAB_RELEASE_IND (message_p).e_rab_release_list.item[index].e_rab_id  = erab_item->e_RAB_ID;
    S1AP_E_RAB_RELEASE_IND (message_p).e_rab_release_list.item[index].cause     = erab_item->cause;
    S1AP_E_RAB_RELEASE_IND (message_p).e_rab_release_list.erab_bitmap |= (0x01 << (erab_item->e_RAB_ID -1));
    S1AP_E_RAB_RELEASE_IND (message_p).e_rab_release_list.no_of_items++;
  }
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
s1ap_mme_handle_error_ind_message (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t *pdu)
{
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                        mme_ue_s1ap_id = 0;
  S1AP_ErrorIndication_t                 *container = NULL;
  S1AP_ErrorIndicationIEs_t              *ie = NULL;
  MessageDef                             *message_p = NULL;
  long                                    cause_value;

  OAILOG_FUNC_IN (LOG_S1AP);

  container = &pdu->choice.initiatingMessage.value.choice.ErrorIndication;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_ErrorIndicationIEs_t, ie, container, S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, false);
  if(ie)
	mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_ErrorIndicationIEs_t, ie, container, S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, false);
  // eNB UE S1AP ID is limited to 24 bits
  if(ie)
    enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);

  enb_description_t * enb_ref = s1ap_is_enb_assoc_id_in_list (assoc_id);
  DevAssert(enb_ref);

  ue_description_t * ue_ref = s1ap_is_ue_enb_id_in_list (enb_ref, enb_ue_s1ap_id);
  /** If no UE reference exists, drop the message. */
  if(!ue_ref){
    /** Check that the UE reference exists. */
    OAILOG_WARNING(LOG_S1AP, "No UE reference exists for enbId %d and enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT ". Dropping error indication. \n", enb_ref->enb_id, enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  /*
   * Handle error indication.
   * Just forward to the MME_APP layer, depending if there is a handover procedure running or not the current handover procedure might be disregarded.
   */
  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_ERROR_INDICATION);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  memset ((void *)&message_p->ittiMsg.s1ap_error_indication, 0, sizeof (itti_s1ap_error_indication_t));
  S1AP_ERROR_INDICATION (message_p).mme_ue_s1ap_id = mme_ue_s1ap_id;
  S1AP_ERROR_INDICATION (message_p).enb_ue_s1ap_id = enb_ue_s1ap_id;
  S1AP_ERROR_INDICATION (message_p).enb_id = enb_ref->enb_id;

  /** Choice. */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_ErrorIndicationIEs_t, ie, container, S1AP_ProtocolIE_ID_id_Cause, false);

  if(ie){
	  // todo: unused yet..
	  S1AP_Cause_PR cause_type = ie->value.choice.Cause.present;

	  switch (cause_type)
	  {
	  case S1AP_Cause_PR_radioNetwork:
	    cause_value = ie->value.choice.Cause.choice.radioNetwork;
	    OAILOG_DEBUG (LOG_S1AP, "S1AP_ERROR_INDICATION with Cause_Type = Radio Network and Cause_Value = %ld\n", cause_value);
	    break;

	  case S1AP_Cause_PR_transport:
	    cause_value = ie->value.choice.Cause.choice.transport;
	    OAILOG_DEBUG (LOG_S1AP, "S1AP_ERROR_INDICATION with Cause_Type = Transport and Cause_Value = %ld\n", cause_value);
	    break;

	  case S1AP_Cause_PR_nas:
	    cause_value = ie->value.choice.Cause.choice.nas;
	    OAILOG_DEBUG (LOG_S1AP, "S1AP_ERROR_INDICATION with Cause_Type = NAS and Cause_Value = %ld\n", cause_value);
	    break;

	  case S1AP_Cause_PR_protocol:
	    cause_value = ie->value.choice.Cause.choice.protocol;
	    OAILOG_DEBUG (LOG_S1AP, "S1AP_ERROR_INDICATION with Cause_Type = Protocol and Cause_Value = %ld\n", cause_value);
	    break;

	  case S1AP_Cause_PR_misc:
	    cause_value = ie->value.choice.Cause.choice.misc;
	    OAILOG_DEBUG (LOG_S1AP, "S1AP_ERROR_INDICATION with Cause_Type = MISC and Cause_Value = %ld\n", cause_value);
	    break;

	  default:
	    OAILOG_ERROR (LOG_S1AP, "S1AP_ERROR_INDICATION with Invalid Cause_Type = %d\n", cause_type);
	    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
	  }

  }

  /**
   * No match function exists between the S1AP Cause and the decoded cause yet.
   * Set it to anything else thatn SYSTEM_FAILURE, not to trigger any implicit detach due cause (only based on the handover type).
   */
  S1AP_ERROR_INDICATION (message_p).cause = S1AP_HANDOVER_FAILED;

  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
}

//------------------------------------------------------------------------------
int s1ap_mme_handle_enb_configuration_transfer (const sctp_assoc_id_t assoc_id,
		const sctp_stream_id_t stream, S1AP_S1AP_PDU_t *pdu){

  MessageDef                               *message_p = NULL;
  int                                       rc = RETURNok;
  tai_t                                  	target_tai;
  tai_t                                  	source_tai;
  plmn_t 									enb_id_plmn;
  S1AP_ENBConfigurationTransfer_t		   *container = NULL;
  S1AP_ENBConfigurationTransferIEs_t       *ie = NULL;

  OAILOG_FUNC_IN (LOG_S1AP);
  // Check the reset type - partial_reset OR reset_all
  container = &pdu->choice.initiatingMessage.value.choice.ENBConfigurationTransfer;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_ENBConfigurationTransferIEs_t, ie, container,
		  S1AP_ProtocolIE_ID_id_SONConfigurationTransferECT, false);
  if(!ie){
	  // todo: LIONEL!!
    OAILOG_ERROR(LOG_S1AP, "eNB configuration transfer currently only implementd for 4G. \n");
    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  }
  S1AP_SONConfigurationTransfer_t * sonConfigurationTransfer = &ie->value.choice.SONConfigurationTransfer;

  /** Get the target tai information. */
  if(!sonConfigurationTransfer->targeteNB_ID.global_ENB_ID.pLMNidentity.buf
		  || !sonConfigurationTransfer->targeteNB_ID.selected_TAI.tAC.buf
		  || !sonConfigurationTransfer->targeteNB_ID.global_ENB_ID.pLMNidentity.buf ){
	  OAILOG_ERROR(LOG_S1AP, "eNB configuration transfer message for has no target_tai or enb_id information. \n");
	  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
  }
  memset((void*)&target_tai, 0, sizeof(tai_t));

  TBCD_TO_PLMN_T (&sonConfigurationTransfer->targeteNB_ID.selected_TAI.pLMNidentity, &target_tai.plmn);
  target_tai.tac = htons(*((uint16_t *) sonConfigurationTransfer->targeteNB_ID.selected_TAI.tAC.buf));

  /** Get the source tai information. */
  if(!sonConfigurationTransfer->sourceeNB_ID.global_ENB_ID.pLMNidentity.buf
		  || !sonConfigurationTransfer->sourceeNB_ID.selected_TAI.tAC.buf
			|| !sonConfigurationTransfer->sourceeNB_ID.global_ENB_ID.pLMNidentity.buf ){
		OAILOG_ERROR(LOG_S1AP, "eNB configuration transfer message for has no source_tai or enb_id information. \n");
	    OAILOG_FUNC_RETURN (LOG_S1AP, rc);
	}
	memset((void*)&source_tai, 0, sizeof(tai_t));

	TBCD_TO_PLMN_T (&sonConfigurationTransfer->sourceeNB_ID.selected_TAI.pLMNidentity, &source_tai.plmn);
	source_tai.tac = htons(*((uint16_t *) sonConfigurationTransfer->sourceeNB_ID.selected_TAI.tAC.buf));

	/** Target Global-ENB-ID. */
    ecgi_t target_global_enb_id = {0};
	memset(&enb_id_plmn, 0, sizeof(plmn_t));
	TBCD_TO_PLMN_T (&sonConfigurationTransfer->targeteNB_ID.global_ENB_ID.pLMNidentity, &enb_id_plmn);
	target_global_enb_id.plmn = enb_id_plmn;

	/** Source Global-ENB-ID. */
	ecgi_t source_global_enb_id = {0};
	memset(&enb_id_plmn, 0, sizeof(plmn_t));
	TBCD_TO_PLMN_T (&sonConfigurationTransfer->sourceeNB_ID.global_ENB_ID.pLMNidentity, &enb_id_plmn);
	source_global_enb_id.plmn = enb_id_plmn;

	// Macro eNB = 20 bits
	/** Get the target enb id. */
	uint32_t target_enb_id = 0;
	target_type_t target_enb_type = 0;
	if(sonConfigurationTransfer->targeteNB_ID.global_ENB_ID.eNB_ID.present == S1AP_ENB_ID_PR_homeENB_ID){
		/** Target is a home eNB Id. */
		// Home eNB ID = 28 bits
		uint8_t *enb_id_buf = sonConfigurationTransfer->targeteNB_ID.global_ENB_ID.eNB_ID.choice.homeENB_ID.buf;
	    if (sonConfigurationTransfer->targeteNB_ID.global_ENB_ID.eNB_ID.choice.homeENB_ID.size != 28) {
	      //TODO: handle case were size != 28 -> notify ? reject ?
	    }
	    target_enb_id = (enb_id_buf[0] << 20) + (enb_id_buf[1] << 12) + (enb_id_buf[2] << 4) + ((enb_id_buf[3] & 0xf0) >> 4);
	    target_enb_type = TARGET_ID_HOME_ENB_ID;
	    //    OAILOG_MESSAGE_ADD (context, "home eNB id: %07x", target_enb_id);
	}else{
		/** Target is a macro eNB Id (Macro eNB = 20 bits). */
	    uint8_t *enb_id_buf = sonConfigurationTransfer->targeteNB_ID.global_ENB_ID.eNB_ID.choice.macroENB_ID.buf;
	    if (sonConfigurationTransfer->targeteNB_ID.global_ENB_ID.eNB_ID.choice.macroENB_ID.size != 20) {
	      //TODO: handle case were size != 28 -> notify ? reject ?
	    }
	    target_enb_id = (enb_id_buf[0] << 12) + (enb_id_buf[1] << 4) + ((enb_id_buf[2] & 0xf0) >> 4);
	    //    OAILOG_MESSAGE_ADD (context, "macro eNB id: %05x", target_enb_id);
	    target_enb_type = TARGET_ID_MACRO_ENB_ID;
	}

	// Macro eNB = 20 bits
	/** Get the source enb id. */
	uint32_t source_enb_id = 0;
	target_type_t source_enb_type = 0;
	if(sonConfigurationTransfer->sourceeNB_ID.global_ENB_ID.eNB_ID.present == S1AP_ENB_ID_PR_homeENB_ID){
		/** Source is a home eNB Id. */
		// Home eNB ID = 28 bits
		uint8_t *enb_id_buf = sonConfigurationTransfer->sourceeNB_ID.global_ENB_ID.eNB_ID.choice.homeENB_ID.buf;
	    if (sonConfigurationTransfer->sourceeNB_ID.global_ENB_ID.eNB_ID.choice.homeENB_ID.size != 28) {
	      //TODO: handle case were size != 28 -> notify ? reject ?
	    }
	    source_enb_id = (enb_id_buf[0] << 20) + (enb_id_buf[1] << 12) + (enb_id_buf[2] << 4) + ((enb_id_buf[3] & 0xf0) >> 4);
	    source_enb_type = TARGET_ID_HOME_ENB_ID;
	    //    OAILOG_MESSAGE_ADD (context, "home eNB id: %07x", target_enb_id);
	}else{
		/** Source is a macro eNB Id (Macro eNB = 20 bits). */
	    uint8_t *enb_id_buf = sonConfigurationTransfer->sourceeNB_ID.global_ENB_ID.eNB_ID.choice.macroENB_ID.buf;
	    if (sonConfigurationTransfer->sourceeNB_ID.global_ENB_ID.eNB_ID.choice.macroENB_ID.size != 20) {
	      //TODO: handle case were size != 28 -> notify ? reject ?
	    }
	    source_enb_id = (enb_id_buf[0] << 12) + (enb_id_buf[1] << 4) + ((enb_id_buf[2] & 0xf0) >> 4);
	    //    OAILOG_MESSAGE_ADD (context, "macro eNB id: %05x", target_enb_id);
	    source_enb_type = TARGET_ID_MACRO_ENB_ID;
	}

	target_global_enb_id.cell_identity.enb_id = target_enb_id;
	source_global_enb_id.cell_identity.enb_id = source_enb_id;
	OAILOG_DEBUG(LOG_S1AP, "Successfully decoded sourceID IE in ENB Configuration Transfer. \n");

	enb_conf_reply_t *conf_reply = NULL;
	if(sonConfigurationTransfer->sONInformation.present == S1AP_SONInformation_PR_NOTHING) { // todo: check.. recompile..
		OAILOG_WARNING(LOG_S1AP, "Received invalid S1AP SON information type %d. \n", sonConfigurationTransfer->sONInformation.present);
		OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
	} else if(sonConfigurationTransfer->sONInformation.present == S1AP_SONInformation_PR_sONInformationRequest) {
		OAILOG_DEBUG(LOG_S1AP, "Received S1AP SON information request. \n");
	} else if(sonConfigurationTransfer->sONInformation.present == S1AP_SONInformation_PR_sONInformationReply) {
		OAILOG_DEBUG(LOG_S1AP, "Received S1AP SON information reply. \n");
		/** Create a bstring array for the iterated message. */
		struct S1AP_X2TNLConfigurationInfo	*x2TNLConfigurationInfo = sonConfigurationTransfer->sONInformation.choice.sONInformationReply.x2TNLConfigurationInfo;
		if(x2TNLConfigurationInfo){
			conf_reply = calloc(1, sizeof(enb_conf_reply_t));
			conf_reply->reply_count = x2TNLConfigurationInfo->eNBX2TransportLayerAddresses.list.count;
			for (int item = 0; item < x2TNLConfigurationInfo->eNBX2TransportLayerAddresses.list.count; item++) {
				/*
			     * Bad, very bad cast...
			     */
				S1AP_TransportLayerAddress_t * s1ap_transportLayerAddress_p = (S1AP_TransportLayerAddress_t *)
			        		x2TNLConfigurationInfo->eNBX2TransportLayerAddresses.list.array[item];
				/** Set the IP address from the FTEID. */
				conf_reply->addresses[item] = blk2bstr(s1ap_transportLayerAddress_p->buf, s1ap_transportLayerAddress_p->size);
			}
			OAILOG_INFO(LOG_S1AP, "Successfully decoded X2 configuration transfer reply. \n");
		} else {
			OAILOG_WARNING(LOG_S1AP, "No X2 reply object is received in the eNB status reply.\n");
		}
	} else {
		OAILOG_ERROR(LOG_S1AP, "Received invalid S1AP SON information type %d. \n", sonConfigurationTransfer->sONInformation.choice);
		OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
	}

	/** Not changing any ue_reference properties. */
	message_p = itti_alloc_new_message (TASK_S1AP, S1AP_CONFIGURATION_TRANSFER);
	AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");

	/** Selected-TAI. */
	itti_s1ap_configuration_transfer_t *enb_configuration_transfer_p = &message_p->ittiMsg.s1ap_configuration_transfer;

	/** Set the eNB information. */
	enb_configuration_transfer_p->target_tai  = target_tai;
	enb_configuration_transfer_p->source_tai  = source_tai;
	enb_configuration_transfer_p->target_enb_type = target_enb_type;
	enb_configuration_transfer_p->source_enb_type = source_enb_type;
	memcpy((void*)&enb_configuration_transfer_p->target_global_enb_id, &target_global_enb_id, sizeof(target_global_enb_id));
	memcpy((void*)&enb_configuration_transfer_p->source_global_enb_id, &source_global_enb_id, sizeof(source_global_enb_id));

	enb_configuration_transfer_p->conf_reply = conf_reply;

	/**
	 * The transparent container bstr must be deallocated later.
	 * No automatic deallocation function for that is present.
	 */
	rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
	OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int
s1ap_mme_handle_enb_reset (
     const sctp_assoc_id_t assoc_id,
     const sctp_stream_id_t stream,
     S1AP_S1AP_PDU_t *pdu)
{
  S1AP_ResetIEs_t                         *enb_reset_p = NULL;
  MessageDef                              *message_p  = NULL;
  ue_description_t                        *ue_ref_p = NULL;
  enb_description_t                       *enb_association = NULL;
  s1ap_reset_type_t                        s1ap_reset_type;
  S1AP_Reset_t		          		     *container = NULL;
  S1AP_ResetIEs_t                 		 *ie = NULL;

  arg_s1ap_construct_enb_reset_req_t      arg = {0};
  uint32_t                                item = 0;
  int                                     rc = RETURNok;

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
  // Check the reset type - partial_reset OR reset_all
  container = &pdu->choice.initiatingMessage.value.choice.Reset;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_ResetIEs_t, ie, container,
                           S1AP_ProtocolIE_ID_id_ResetType, true);
  S1AP_ResetType_t * resetType = &ie->value.choice.ResetType;

  switch (resetType->present){
	case S1AP_ResetType_PR_s1_Interface:
	  s1ap_reset_type = RESET_ALL;
	break;
	case S1AP_ResetType_PR_partOfS1_Interface:
	  s1ap_reset_type = RESET_PARTIAL;
	break;
	default:
	  OAILOG_ERROR (LOG_S1AP, "Reset Request from eNB  with Invalid reset_type = %d\n", resetType->present);
	  // TBD - Here MME should send Error Indication as it is abnormal scenario.
	  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }
  /** Create a cleared array of identifiers. */
  int num_ue_assoc = 0;
  // Partial Reset
  if (s1ap_reset_type != RESET_ALL) {
	  // Check if there are no UEs connected
	  if (!enb_association->nb_ue_associated) {
	    // Ignore the message
	    OAILOG_INFO (LOG_S1AP, "No UEs is connected.Ignoring ENB Initiated Reset.eNB Id = %d\n", enb_association->enb_id);
	    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNok);
	  }
	  num_ue_assoc = resetType->choice.partOfS1_Interface.list.count;
	  s1_sig_conn_id_t ue_to_reset_list[num_ue_assoc]; /**< Create a stacked array. */
	  memset(ue_to_reset_list, 0, sizeof(ue_to_reset_list));
	  for (item = 0; item < num_ue_assoc; item++) {
		  /** Decode Protocol IE here. */
//		  if (resetType->choice.partOfS1_Interface.list.array[i]->id != S1AP_ProtocolIE_ID_id_UE_associatedLogicalS1_ConnectionItem) {
//			  OAILOG_ERROR (LOG_S1AP, "Incorrect element in Bearers_SubjectToStatusTransferList: %d\n", resetType->choice.partOfS1_Interface.list.array[i]->id);
//			  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
//		  }
		  S1AP_UE_associatedLogicalS1_ConnectionItemResAck_t *s1_sig_conn_p = (S1AP_UE_associatedLogicalS1_ConnectionItemResAck_t*)
		         ie->value.choice.ResetType.choice.partOfS1_Interface.list.array[item];
		  if(!s1_sig_conn_p) {
			  OAILOG_ERROR (LOG_S1AP, "No logical S1 connection item could be found for the partial connection. "
					  "Ignoring the received partial reset request. \n");
			  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
		  }

		  S1AP_UE_associatedLogicalS1_ConnectionItem_t * s1_sig_conn_item = &s1_sig_conn_p->value.choice.UE_associatedLogicalS1_ConnectionItem;
		  if (s1_sig_conn_item->mME_UE_S1AP_ID != NULL) {
			  mme_ue_s1ap_id_t mme_ue_s1ap_id = (mme_ue_s1ap_id_t) *(s1_sig_conn_item->mME_UE_S1AP_ID);
			  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) != NULL) {
				  if (s1_sig_conn_item->eNB_UE_S1AP_ID != NULL) {
					  enb_ue_s1ap_id_t enb_ue_s1ap_id = (enb_ue_s1ap_id_t) *(s1_sig_conn_item->eNB_UE_S1AP_ID);
					  enb_ue_s1ap_id &= ENB_UE_S1AP_ID_MASK;
					  if (ue_ref_p->enb_ue_s1ap_id == enb_ue_s1ap_id) {
						  ue_to_reset_list[item].mme_ue_s1ap_id = &(ue_ref_p->mme_ue_s1ap_id);
						  ue_to_reset_list[item].enb_ue_s1ap_id = (enb_ue_s1ap_id_t*)ue_ref_p;
					  } else {
						  // mismatch in enb_ue_s1ap_id sent by eNB and stored in S1AP ue context in EPC. Abnormal case.
						  ue_to_reset_list[item].mme_ue_s1ap_id = NULL;
						  ue_to_reset_list[item].enb_ue_s1ap_id = NULL;
						  OAILOG_ERROR (LOG_S1AP, "Partial Reset Request:enb_ue_s1ap_id mismatch between id %d sent by eNB and id %d stored in epc for mme_ue_s1ap_id %d \n",
								  enb_ue_s1ap_id, ue_ref_p->enb_ue_s1ap_id, mme_ue_s1ap_id);
					  }
				  } else {
					  ue_to_reset_list[item].mme_ue_s1ap_id = &(ue_ref_p->mme_ue_s1ap_id);
					  ue_to_reset_list[item].enb_ue_s1ap_id = NULL;
				  }
			  } else {
				  OAILOG_ERROR (LOG_S1AP, "Partial Reset Request - No UE context found for mme_ue_s1ap_id %d \n", mme_ue_s1ap_id);
				  // TBD - Here MME should send Error Indication as it is abnormal scenario.
				 // todo: test leak ASN_STRUCT_FREE(asn_DEF_S1AP_UE_associatedLogicalS1_ConnectionItem, s1_sig_conn_id_p);
				  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
			  }
		  } else {
			  if (s1_sig_conn_item->eNB_UE_S1AP_ID != NULL) {
				  enb_ue_s1ap_id_t enb_ue_s1ap_id = (enb_ue_s1ap_id_t) *(s1_sig_conn_item->eNB_UE_S1AP_ID);
				  enb_ue_s1ap_id &= ENB_UE_S1AP_ID_MASK;
				  if ((ue_ref_p = s1ap_is_ue_enb_id_in_list (enb_association, enb_ue_s1ap_id)) != NULL) {
					  ue_to_reset_list[item].enb_ue_s1ap_id = (enb_ue_s1ap_id_t*)ue_ref_p;
					  if (ue_ref_p->mme_ue_s1ap_id != INVALID_MME_UE_S1AP_ID) {
						  ue_to_reset_list[item].mme_ue_s1ap_id = &(ue_ref_p->mme_ue_s1ap_id);
					  } else {
						  ue_to_reset_list[item].mme_ue_s1ap_id = NULL;
					  }
				  } else {
					  OAILOG_ERROR (LOG_S1AP, "Partial Reset Request without any valid S1 signaling connection.Ignoring it \n");
					  // TBD - Here MME should send Error Indication as it is abnormal scenario.
					  // todo: test leak  ASN_STRUCT_FREE(asn_DEF_S1AP_UE_associatedLogicalS1_ConnectionItem, s1_sig_conn_id_p);
					  OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
	          }
	        } else {
	          OAILOG_ERROR (LOG_S1AP, "Partial Reset Request without any valid S1 signaling connection.Ignoring it.\n");
	          // TBD - Here MME should send Error Indication as it is abnormal scenario.
	          // todo: test leak ASN_STRUCT_FREE(asn_DEF_S1AP_UE_associatedLogicalS1_ConnectionItem, s1_sig_conn_id_p);
	          OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
	        }
	      }
//		  if(s1_sig_conn_id_p)
			  // todo: test leak   ASN_STRUCT_FREE(asn_DEF_S1AP_UE_associatedLogicalS1_ConnectionItem, s1_sig_conn_id_p);
	  }
      OAILOG_INFO (LOG_S1AP, "After iterating all %d partial UEs, informing the MME_APP layer for the received Partial Reset Request.\n", num_ue_assoc);
	  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_ENB_INITIATED_RESET_REQ);
	  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
	  S1AP_ENB_INITIATED_RESET_REQ (message_p).num_ue = num_ue_assoc;
	  /** Allocate a new list and fill it directly. */
	  S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list = (s1_sig_conn_id_t*) calloc (num_ue_assoc, sizeof (s1_sig_conn_id_t));
	  memcpy(S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list, ue_to_reset_list, sizeof(ue_to_reset_list));
  } else {
	  /** Generating full reset request. */
	  OAILOG_INFO (LOG_S1AP, "Received a full reset request. Informing the MME_APP layer.\n");
	  num_ue_assoc = enb_association->nb_ue_associated;
	  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_ENB_INITIATED_RESET_REQ);
	  arg.message_p = message_p;
	  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
	  S1AP_ENB_INITIATED_RESET_REQ (message_p).num_ue = num_ue_assoc;
	  /**
	   * Allocate a new list and let the callback function fill it.
	   * Generating full reset request.
	   */
	  S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list = (s1_sig_conn_id_t*) calloc (enb_association->nb_ue_associated, sizeof (s1_sig_conn_id_t));
	  hashtable_ts_apply_callback_on_elements(&enb_association->ue_coll, construct_s1ap_mme_full_reset_req, (void*)&arg, (void**) &message_p);
  }
  S1AP_ENB_INITIATED_RESET_REQ (message_p).s1ap_reset_type = s1ap_reset_type;
  S1AP_ENB_INITIATED_RESET_REQ (message_p).enb_id = enb_association->enb_id;
  S1AP_ENB_INITIATED_RESET_REQ (message_p).sctp_assoc_id = assoc_id;
  S1AP_ENB_INITIATED_RESET_REQ (message_p).sctp_stream_id = stream;
  DevAssert(S1AP_ENB_INITIATED_RESET_REQ (message_p).ue_to_reset_list != NULL);
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
  S1AP_S1AP_PDU_t                         pdu;
  /** Reset Acknowledgment. */
  S1AP_ResetAcknowledge_t				 *out;
  S1AP_ResetAcknowledgeIEs_t             *ie = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_S1AP);

  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1AP_S1AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = S1AP_ProcedureCode_id_Reset;
  pdu.choice.successfulOutcome.criticality = S1AP_Criticality_ignore;
  pdu.choice.successfulOutcome.value.present = S1AP_SuccessfulOutcome__value_PR_ResetAcknowledge;
  out = &pdu.choice.successfulOutcome.value.choice.ResetAcknowledge;

  if (enb_reset_ack_p->s1ap_reset_type == RESET_PARTIAL) {
    DevAssert(enb_reset_ack_p->num_ue > 0);
    /** Conn Item .*/
    ie = (S1AP_ResetAcknowledgeIEs_t *)calloc(1, sizeof(S1AP_ResetAcknowledgeIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_UE_associatedLogicalS1_ConnectionListResAck;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_ResetAcknowledgeIEs__value_PR_UE_associatedLogicalS1_ConnectionListResAck;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
	/** MME UE S1AP ID. */
    S1AP_UE_associatedLogicalS1_ConnectionListResAck_t * ie_p = &ie->value.choice.UE_associatedLogicalS1_ConnectionListResAck;
    /** Allocate an item for each. */
    for (uint32_t i = 0; i < enb_reset_ack_p->num_ue; i++) {
    	/** MME UE. */
    	S1AP_UE_associatedLogicalS1_ConnectionItemResAck_t * sig_conn_item = calloc (1, sizeof (S1AP_UE_associatedLogicalS1_ConnectionItemResAck_t));
    	sig_conn_item->id = S1AP_ProtocolIE_ID_id_UE_associatedLogicalS1_ConnectionItem;
    	sig_conn_item->criticality = S1AP_Criticality_ignore;
    	sig_conn_item->value.present = S1AP_UE_associatedLogicalS1_ConnectionItemResAck__value_PR_UE_associatedLogicalS1_ConnectionItem;
    	S1AP_UE_associatedLogicalS1_ConnectionItem_t * item = &sig_conn_item->value.choice.UE_associatedLogicalS1_ConnectionItem;

    	if (enb_reset_ack_p->ue_to_reset_list[i].mme_ue_s1ap_id != NULL) {
    		item->mME_UE_S1AP_ID = calloc(1, sizeof(S1AP_MME_UE_S1AP_ID_t));
    		*item->mME_UE_S1AP_ID = *enb_reset_ack_p->ue_to_reset_list[i].mme_ue_s1ap_id;
    	}
    	else {
    		item->mME_UE_S1AP_ID = NULL;
    	}
    	/** ENB UE S1AP ID. */
    	if (enb_reset_ack_p->ue_to_reset_list[i].enb_ue_s1ap_id != NULL) {
    		item->eNB_UE_S1AP_ID = calloc(1, sizeof(S1AP_ENB_UE_S1AP_ID_t));
    		*item->eNB_UE_S1AP_ID = *enb_reset_ack_p->ue_to_reset_list[i].enb_ue_s1ap_id;
    	}
    	else {
    		item->eNB_UE_S1AP_ID = NULL;
    	}
        ASN_SEQUENCE_ADD(&ie_p->list, sig_conn_item);
    }
  }

  if (s1ap_mme_encode_pdu (&pdu, &buffer, &length) < 0) {
	OAILOG_ERROR (LOG_S1AP, "Failed to S1 Reset command \n");
	/** We rely on the handover_notify timeout to remove the UE context. */
	DevAssert(!buffer);
	OAILOG_FUNC_OUT (LOG_S1AP);
  }

  if(buffer) {
	  bstring b = blk2bstr(buffer, length);
	  free(buffer);
	  rc = s1ap_mme_itti_send_sctp_request (&b, enb_reset_ack_p->sctp_assoc_id, enb_reset_ack_p->sctp_stream_id, INVALID_MME_UE_S1AP_ID);
  }
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}

//------------------------------------------------------------------------------
int s1ap_mme_handle_erab_modification_indication (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t *pdu)
{
  OAILOG_FUNC_IN (LOG_S1AP);
  enb_ue_s1ap_id_t                        enb_ue_s1ap_id = 0;
  mme_ue_s1ap_id_t                        mme_ue_s1ap_id = 0;
  int                                     rc = RETURNok;
  S1AP_E_RABModificationIndication_t     *container = NULL;
  S1AP_E_RABModificationIndicationIEs_t  *ie = NULL;
  ue_description_t                       *ue_ref_p  = NULL;
  MessageDef                             *message_p = NULL;

  container = &pdu->choice.initiatingMessage.value.choice.E_RABModificationIndication;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABModificationIndicationIEs_t, ie, container, S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
  mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABModificationIndicationIEs_t, ie, container, S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  // eNB UE S1AP ID is limited to 24 bits
  enb_ue_s1ap_id = (enb_ue_s1ap_id_t) (ie->value.choice.ENB_UE_S1AP_ID & ENB_UE_S1AP_ID_MASK);


  if ((ue_ref_p = s1ap_is_ue_mme_id_in_list (mme_ue_s1ap_id)) == NULL) {
    OAILOG_DEBUG (LOG_S1AP, "No UE is attached to this mme UE s1ap id: " MME_UE_S1AP_ID_FMT "\n", mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  if (ue_ref_p->enb_ue_s1ap_id != enb_ue_s1ap_id) {
    OAILOG_DEBUG (LOG_S1AP, "Mismatch in eNB UE S1AP ID, known: " ENB_UE_S1AP_ID_FMT ", received: " ENB_UE_S1AP_ID_FMT "\n",
                ue_ref_p->enb_ue_s1ap_id, enb_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_S1AP, RETURNerror);
  }

  message_p = itti_alloc_new_message (TASK_S1AP, S1AP_E_RAB_MODIFICATION_IND);
  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
  S1AP_E_RAB_MODIFICATION_IND (message_p).mme_ue_s1ap_id  = ue_ref_p->mme_ue_s1ap_id;
  S1AP_E_RAB_MODIFICATION_IND (message_p).enb_ue_s1ap_id  = ue_ref_p->enb_ue_s1ap_id;

  /** Get the bearers to be modified. */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABModificationIndicationIEs_t, ie, container, S1AP_ProtocolIE_ID_id_E_RABToBeModifiedListBearerModInd, true);
  const S1AP_E_RABToBeModifiedListBearerModInd_t * const e_rab_list = &ie->value.choice.E_RABToBeModifiedListBearerModInd;
  int num_erab = e_rab_list->list.count;
  for (int index = 0; index < num_erab; index++) {
    const S1AP_E_RABToBeModifiedItemBearerModIndIEs_t * const erab_item_ies = (S1AP_E_RABToBeModifiedItemBearerModIndIEs_t *)e_rab_list->list.array[index];
    const S1AP_E_RABToBeModifiedItemBearerModInd_t * const erab_item = (S1AP_E_RABToBeModifiedItemBearerModInd_t *)&erab_item_ies->value.choice.E_RABToBeModifiedItemBearerModInd;
    S1AP_E_RAB_MODIFICATION_IND (message_p).e_rab_to_be_modified_list.item[index].e_rab_id  = erab_item->e_RAB_ID;


    bstring transport_layer_address = blk2bstr(erab_item->transportLayerAddress.buf, erab_item->transportLayerAddress.size);

    S1AP_E_RAB_MODIFICATION_IND (message_p).e_rab_to_be_modified_list.item[index].s1_xNB_fteid.teid = htonl (*((uint32_t *) erab_item->dL_GTP_TEID.buf));

    /** Set the IP address from the FTEID. */
    if (4 == blength(transport_layer_address)) {
      S1AP_E_RAB_MODIFICATION_IND (message_p).e_rab_to_be_modified_list.item[index].s1_xNB_fteid.ipv4 = 1;
      memcpy(&S1AP_E_RAB_MODIFICATION_IND (message_p).e_rab_to_be_modified_list.item[index].s1_xNB_fteid.ipv4_address,
          transport_layer_address->data,
          blength(transport_layer_address));
    } else if (16 == blength(transport_layer_address)) {
      S1AP_E_RAB_MODIFICATION_IND (message_p).e_rab_to_be_modified_list.item[index].s1_xNB_fteid.ipv6 = 1;
      memcpy(&S1AP_E_RAB_MODIFICATION_IND (message_p).e_rab_to_be_modified_list.item[index].s1_xNB_fteid.ipv6_address,
          transport_layer_address->data,
          blength(transport_layer_address));
    } else {
      AssertFatal(0, "TODO IP address %d bytes", blength(transport_layer_address));
    }
    bdestroy_wrapper(&transport_layer_address);

    S1AP_E_RAB_MODIFICATION_IND (message_p).e_rab_to_be_modified_list.no_of_items++;
  }

  /** Get the bearers not to be modified. */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABModificationIndicationIEs_t, ie, container, S1AP_ProtocolIE_ID_id_E_RABNotToBeModifiedListBearerModInd, false);
  if (ie) {
    const S1AP_E_RABNotToBeModifiedListBearerModInd_t * const e_rab_not_mod_list = &ie->value.choice.E_RABNotToBeModifiedListBearerModInd;
    num_erab = e_rab_not_mod_list->list.count;
    for (int index = 0; index < num_erab; index++) {
      const S1AP_E_RABNotToBeModifiedItemBearerModIndIEs_t * const erab_item_ies = (S1AP_E_RABNotToBeModifiedItemBearerModIndIEs_t *)e_rab_not_mod_list->list.array[index];
      const S1AP_E_RABNotToBeModifiedItemBearerModInd_t * const erab_item = (S1AP_E_RABNotToBeModifiedItemBearerModInd_t *)&erab_item_ies->value.choice.E_RABNotToBeModifiedItemBearerModInd;
      S1AP_E_RAB_MODIFICATION_IND (message_p).e_rab_not_to_be_modified_list.item[index].e_rab_id  = erab_item->e_RAB_ID;


      bstring transport_layer_address = blk2bstr(erab_item->transportLayerAddress.buf, erab_item->transportLayerAddress.size);

      S1AP_E_RAB_MODIFICATION_IND (message_p).e_rab_not_to_be_modified_list.item[index].s1_xNB_fteid.teid = htonl (*((uint32_t *) erab_item->dL_GTP_TEID.buf));

      /** Set the IP address from the FTEID. */
      if (4 == blength(transport_layer_address)) {
        S1AP_E_RAB_MODIFICATION_IND (message_p).e_rab_not_to_be_modified_list.item[index].s1_xNB_fteid.ipv4 = 1;
        memcpy(&S1AP_E_RAB_MODIFICATION_IND (message_p).e_rab_not_to_be_modified_list.item[index].s1_xNB_fteid.ipv4_address,
            transport_layer_address->data,
            blength(transport_layer_address));
      } else if (16 == blength(transport_layer_address)) {
        S1AP_E_RAB_MODIFICATION_IND (message_p).e_rab_not_to_be_modified_list.item[index].s1_xNB_fteid.ipv6 = 1;
        memcpy(&S1AP_E_RAB_MODIFICATION_IND (message_p).e_rab_not_to_be_modified_list.item[index].s1_xNB_fteid.ipv6_address,
            transport_layer_address->data,
            blength(transport_layer_address));
      } else {
        AssertFatal(0, "TODO IP address %d bytes", blength(transport_layer_address));
      }
      bdestroy_wrapper(&transport_layer_address);

      S1AP_E_RAB_MODIFICATION_IND (message_p).e_rab_not_to_be_modified_list.no_of_items++;
    }
  }
  rc =  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_S1AP, rc);
}
//------------------------------------------------------------------------------
void
s1ap_mme_generate_erab_modification_confirm( const itti_s1ap_e_rab_modification_cnf_t * const conf){

  uint8_t                                *buffer_p = NULL;
  uint32_t                                length = 0;
  ue_description_t                       *ue_ref = NULL;
  S1AP_S1AP_PDU_t                         pdu = {0};
  S1AP_E_RABModificationConfirm_t        *out;
  S1AP_E_RABModificationConfirmIEs_t     *ie = NULL;

  OAILOG_FUNC_IN (LOG_S1AP);
  DevAssert (conf != NULL);

  ue_ref = s1ap_is_ue_mme_id_in_list (conf->mme_ue_s1ap_id);
  if (!ue_ref) {
    OAILOG_ERROR (LOG_S1AP, "This mme ue s1ap id (" MME_UE_S1AP_ID_FMT ") is not attached to any UE context\n",
        conf->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_S1AP);
  }

  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1AP_S1AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = S1AP_ProcedureCode_id_E_RABModificationIndication;
  pdu.choice.successfulOutcome.criticality = S1AP_Criticality_reject;
  pdu.choice.successfulOutcome.value.present = S1AP_SuccessfulOutcome__value_PR_E_RABModificationConfirm;
  out = &pdu.choice.successfulOutcome.value.choice.E_RABModificationConfirm;

  /* mandatory */
  ie = (S1AP_E_RABModificationConfirmIEs_t *)calloc(1, sizeof(S1AP_E_RABModificationConfirmIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID;
  ie->criticality = S1AP_Criticality_ignore;
  ie->value.present = S1AP_E_RABModificationConfirmIEs__value_PR_MME_UE_S1AP_ID;
  ie->value.choice.MME_UE_S1AP_ID = conf->mme_ue_s1ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (S1AP_E_RABModificationConfirmIEs_t *)calloc(1, sizeof(S1AP_E_RABModificationConfirmIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID;
  ie->criticality = S1AP_Criticality_ignore;
  ie->value.present = S1AP_E_RABModificationConfirmIEs__value_PR_ENB_UE_S1AP_ID;
  ie->value.choice.ENB_UE_S1AP_ID = conf->enb_ue_s1ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (conf->e_rab_modify_list.no_of_items) {

    ie = (S1AP_E_RABModificationConfirmIEs_t *)calloc(1, sizeof(S1AP_E_RABModificationConfirmIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_E_RABModifyListBearerModConf;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_E_RABModificationConfirmIEs__value_PR_E_RABModifyListBearerModConf;
    ASN_SEQUENCE_ADD (&out->protocolIEs.list, ie);

    S1AP_E_RABModifyListBearerModConf_t  *e_rabmodifylistbearermodconf = &ie->value.choice.E_RABModifyListBearerModConf;

    for(int i = 0; i < conf->e_rab_modify_list.no_of_items; i++){
      S1AP_E_RABModifyItemBearerModConfIEs_t     *item =  calloc(1, sizeof(S1AP_E_RABModifyItemBearerModConfIEs_t));

      item->id = S1AP_ProtocolIE_ID_id_E_RABModifyItemBearerModConf;
      item->criticality = S1AP_Criticality_reject;
      item->value.present = S1AP_E_RABModifyItemBearerModConfIEs__value_PR_E_RABModifyItemBearerModConf;

      S1AP_E_RABModifyItemBearerModConf_t  * bearer = &item->value.choice.E_RABModifyItemBearerModConf;
      bearer->e_RAB_ID = conf->e_rab_modify_list.e_rab_id[i];

      ASN_SEQUENCE_ADD (&e_rabmodifylistbearermodconf->list, item);
    }
  }
  /** Encoding without allocating? */
  if (s1ap_mme_encode_pdu (&pdu, &buffer_p, &length) < 0) {
    OAILOG_ERROR (LOG_S1AP, "Failed to encode S1AP_E_RAB_MODIFICATION_CONFIRM for UE " MME_UE_S1AP_ID_FMT ".\n",
        conf->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_S1AP);
  }

  OAILOG_NOTICE (LOG_S1AP, "Send S1AP_E_RAB_MODIFICATION_CONFIRM message MME_UE_S1AP_ID = " MME_UE_S1AP_ID_FMT " \n",
      (mme_ue_s1ap_id_t)conf->mme_ue_s1ap_id);

  bstring b = blk2bstr(buffer_p, length);
  free(buffer_p);
  s1ap_mme_itti_send_sctp_request (&b , ue_ref->enb->sctp_assoc_id, ue_ref->sctp_stream_send, ue_ref->mme_ue_s1ap_id);

  OAILOG_FUNC_OUT (LOG_S1AP);
}

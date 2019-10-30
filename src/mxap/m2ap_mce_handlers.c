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

/*! \file m2ap_mce_handlers.c
  \brief
  \author Dincer BEKEN
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#include "m2ap_mce_handlers.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "3gpp_requirements_36.443.h"
#include "mxap_mce.h"
#include "bstrlib.h"

#include "hashtable.h"
#include "log.h"

#include "assertions.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "mce_app_statistics.h"
#include "timer.h"
#include "dynamic_memory_check.h"
#include "m2ap_common.h"
#include "m2ap_mce_encoder.h"
#include "mxap_mce_itti_messaging.h"
#include "mxap_mce_procedures.h"
#include "mxap_mce.h"
#include "m2ap_mce_mbms_sa.h"
#include "m2ap_mce_handlers.h"

extern hash_table_ts_t g_m2ap_enb_coll; // contains eNB_description_s, key is eNB_description_s.assoc_id

static const char * const m2_enb_state_str [] = {"M2AP_INIT", "M2AP_RESETTING", "M2AP_READY", "M2AP_SHUTDOWN"};

static int                              m2ap_generate_m2_setup_response (
    m2ap_enb_description_t * m2ap_enb_association);

//static int                              m2ap_mme_generate_ue_context_release_command (
//    mbms_description_t * mbms_ref_p, mce_mbms_m2ap_id_t mce_mbms_m2ap_id, enum s1cause, m2ap_enb_description_t* enb_ref_p);

/* Handlers matrix. Only mme related procedures present here.
*/
m2ap_message_decoded_callback           messages_callback[][3] = {
  {0, m2ap_mce_handle_mbms_session_start_response,		/* sessionStart */
		  m2ap_mce_handle_mbms_session_start_failure},
  {0, m2ap_mce_handle_mbms_session_stop_response,      	/* sessionStop */
		  m2ap_mce_handle_mbms_session_stop_failure},
  {0, m2ap_mce_handle_mbms_session_update_response,		/* sessionUpdate */
		  m2ap_mce_handle_mbms_session_update_failure},
  {m2ap_mce_handle_m2_setup_request, 0, 0},     				   /* m2Setup */
  {0, m2ap_mce_handle_mbms_scheduling_information_response, 0},    /* mbmsSchedulingInformation */
  {m2ap_mce_handle_reset_request, 0, 0},                    	   /* reset */
  {0, 0, 0},                    								   /* enbConfigurationUpdate */
  {0, 0, 0},                   									   /* mceConfigurationUpdate */
  {0, m2ap_mce_handle_mbms_service_counting_response, 			   /* mbmsServiceCounting */
		  m2ap_mce_handle_mbms_service_counting_failure},
  {m2ap_mce_handle_error_indication, 0, 0},     				   /* errorIndication */
  {m2ap_mce_handle_mbms_service_counting_result_report, 0, 0},     /* mbmsServiceCountingResultReport */
  {m2ap_mce_handle_mbms_overload_notification, 0, 0},                   								       /* mbmsOverloadNotifiction */
};

const char                             *m2ap_direction2String[] = {
  "",                           /* Nothing */
  "Originating message",        /* originating message */
  "Successful outcome",         /* successful outcome */
  "UnSuccessfull outcome",      /* unsuccessful outcome */
};

//------------------------------------------------------------------------------
int
m2ap_mce_handle_message (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu)
{

  /* Checking procedure Code and direction of message */
  if (pdu->choice.initiatingMessage.procedureCode >= sizeof(messages_callback) / (3 * sizeof(m2ap_message_decoded_callback))
      || (pdu->present > M2AP_M2AP_PDU_PR_unsuccessfulOutcome)) {
    OAILOG_DEBUG (LOG_MXAP, "[SCTP %d] Either procedureCode %ld or direction %d exceed expected\n",
               assoc_id, pdu->choice.initiatingMessage.procedureCode, pdu->present);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_M2AP_M2AP_PDU, pdu);
    return -1;
  }

  /* No handler present.
   * This can mean not implemented or no procedure for eNB (wrong direction).
   */
  if (messages_callback[pdu->choice.initiatingMessage.procedureCode][pdu->present - 1] == NULL) {
    OAILOG_DEBUG (LOG_MXAP, "[SCTP %d] No handler for procedureCode %ld in %s\n",
               assoc_id, pdu->choice.initiatingMessage.procedureCode,
               m2ap_direction2String[pdu->present]);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_M2AP_M2AP_PDU, pdu);
    return -1;
  }

  /* Calling the right handler */
  int ret = (*messages_callback[pdu->choice.initiatingMessage.procedureCode][pdu->present - 1])(assoc_id, stream, pdu);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_M2AP_M2AP_PDU, pdu);
  return ret;
}

//------------------------------------------------------------------------------
int
m2ap_mme_set_cause (
  M2AP_Cause_t * cause_p,
  const M2AP_Cause_PR cause_type,
  const long cause_value)
{
  DevAssert (cause_p != NULL);
  cause_p->present = cause_type;

  switch (cause_type) {
  case M2AP_Cause_PR_radioNetwork:
    cause_p->choice.misc = cause_value;
    break;

  case M2AP_Cause_PR_transport:
    cause_p->choice.transport = cause_value;
    break;

  case M2AP_Cause_PR_nAS:
    cause_p->choice.nAS = cause_value;
    break;

  case M2AP_Cause_PR_protocol:
    cause_p->choice.protocol = cause_value;
    break;

  case M2AP_Cause_PR_misc:
    cause_p->choice.misc = cause_value;
    break;

  default:
    return -1;
  }

  return 0;
}
//------------------------------------------------------------------------------
int
m2ap_mce_generate_m2_setup_failure (
    const sctp_assoc_id_t assoc_id,
    const M2AP_Cause_PR cause_type,
    const long cause_value,
    const long time_to_wait)
{
  uint8_t                                *buffer_p = 0;
  uint32_t                                length = 0;
  M2AP_M2AP_PDU_t                         pdu;
  M2AP_M2SetupFailure_t                  *out;
  M2AP_M2SetupFailure_IEs_t              *ie = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MXAP);

  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_unsuccessfulOutcome;
  pdu.choice.unsuccessfulOutcome.procedureCode = M2AP_ProcedureCode_id_m2Setup;
  pdu.choice.unsuccessfulOutcome.criticality = M2AP_Criticality_reject;
  pdu.choice.unsuccessfulOutcome.value.present = M2AP_UnsuccessfulOutcome__value_PR_M2SetupFailure;
  out = &pdu.choice.unsuccessfulOutcome.value.choice.M2SetupFailure;

  ie = (M2AP_M2SetupFailure_IEs_t *)calloc(1, sizeof(M2AP_M2SetupFailure_IEs_t));
  ie->id = M2AP_ProtocolIE_ID_id_Cause;
  ie->criticality = M2AP_Criticality_ignore;
  ie->value.present = M2AP_M2SetupFailure_IEs__value_PR_Cause;
  m2ap_mme_set_cause (&ie->value.choice.Cause, cause_type, cause_value);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /*
   * Include the optional field time to wait only if the value is > -1
   */
  if (time_to_wait > -1) {
    ie = (M2AP_M2SetupFailure_IEs_t *)calloc(1, sizeof(M2AP_M2SetupFailure_IEs_t));
    ie->id = M2AP_ProtocolIE_ID_id_TimeToWait;
    ie->criticality = M2AP_Criticality_ignore;
    ie->value.present = M2AP_M2SetupFailure_IEs__value_PR_TimeToWait;
    ie->value.choice.TimeToWait = time_to_wait;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  if (m2ap_mme_encode_pdu (&pdu, &buffer_p, &length) < 0) {
    OAILOG_ERROR (LOG_MXAP, "Failed to encode m2 setup failure\n");
    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
  }

  bstring b = blk2bstr(buffer_p, length);
  free(buffer_p);
  rc =  m2ap_mme_itti_send_sctp_request (&b, assoc_id, 0, INVALID_MCE_MBMS_M2AP_ID);
  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}

////////////////////////////////////////////////////////////////////////////////
//************************** Management procedures ***************************//
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
int
m2ap_mce_handle_m2_setup_request (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu)
{

  int                                     rc = RETURNok;
  OAILOG_FUNC_IN (LOG_MXAP);
  M2AP_M2SetupRequest_t                  *container = NULL;
  M2AP_M2SetupRequest_IEs_t              *ie = NULL;
  M2AP_M2SetupRequest_IEs_t              *ie_enb_name = NULL;
  M2AP_M2SetupRequest_IEs_t              *ie_enb_mbms_configuration_per_cell = NULL;
  M2AP_GlobalENB_ID_t	                 *global_enb_id = NULL;;
  ecgi_t                                  ecgi = {.plmn = {0}, .cell_identity = {0}};

  m2ap_enb_description_t                 *m2ap_enb_association = NULL;
  uint32_t                                m2ap_enb_id = 0;
  char                                   *m2ap_enb_name = NULL;
  int                                     mbms_sa_ret = 0;
  uint16_t                                max_m2_enb_connected = 0;

  DevAssert (pdu != NULL);
  container = &pdu->choice.initiatingMessage.value.choice.M2SetupRequest;
    /*
     * We received a new valid M2 Setup Request on a stream != 0.
     * * * * This should not happen -> reject eNB m2 setup request.
     */
  if (stream != 0) {
    OAILOG_ERROR (LOG_MXAP, "Received new m2 setup request on stream != 0\n");
      /*
       * Send a m2 setup failure with protocol cause unspecified
       */
    rc = m2ap_mce_generate_m2_setup_failure (assoc_id, M2AP_Cause_PR_protocol, M2AP_CauseProtocol_unspecified, -1);
    OAILOG_FUNC_RETURN (LOG_MXAP, rc);
  }

  shared_log_queue_item_t *  context = NULL;
  OAILOG_MESSAGE_START (OAILOG_LEVEL_DEBUG, LOG_MXAP, (&context), "New m2 setup request incoming from ");

  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_M2SetupRequest_IEs_t, ie_enb_name, container,
		  M2AP_ProtocolIE_ID_id_ENBname, false);
  if (ie) {
    OAILOG_MESSAGE_ADD (context, "%*s ", (int)ie_enb_name->value.choice.ENBname.size, ie_enb_name->value.choice.ENBname.buf);
    m2ap_enb_name = (char *) ie_enb_name->value.choice.ENBname.buf;
  }

  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_M2SetupRequest_IEs_t, ie, container, M2AP_ProtocolIE_ID_id_GlobalENB_ID, true);
  // TODO make sure ie != NULL

  if (ie->value.choice.GlobalENB_ID.eNB_ID.present == M2AP_ENB_ID_PR_short_Macro_eNB_ID) {
	// todo: Short Macro eNB ID
	OAILOG_DEBUG (LOG_MXAP, "M2-Setup-Request Short Macro ENB_ID is not supported. \n");
    rc = m2ap_mce_generate_m2_setup_failure (assoc_id, M2AP_Cause_PR_protocol, M2AP_CauseProtocol_unspecified, -1);
    OAILOG_FUNC_RETURN (LOG_MXAP, rc);
  } else if (ie->value.choice.GlobalENB_ID.eNB_ID.present == M2AP_ENB_ID_PR_long_Macro_eNB_ID) {
    // todo: Long Macro eNB ID
    OAILOG_DEBUG (LOG_MXAP, "M2-Setup-Request Long Macro ENB_ID is not supported. \n");
    rc = m2ap_mce_generate_m2_setup_failure (assoc_id, M2AP_Cause_PR_protocol, M2AP_CauseProtocol_unspecified, -1);
    OAILOG_FUNC_RETURN (LOG_MXAP, rc);
  } else {
	// Macro eNB = 20 bits
	uint8_t                                *enb_id_buf = ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf;
	if (ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.size != 20) {
	  //TODO: handle case were size != 20 -> notify ? reject ?
	  OAILOG_DEBUG (LOG_MXAP, "M2-Setup-Request macroENB_ID.size %lu (should be 20)\n", ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.size);
	}
	m2ap_enb_id = (enb_id_buf[0] << 12) + (enb_id_buf[1] << 4) + ((enb_id_buf[2] & 0xf0) >> 4);
	OAILOG_MESSAGE_ADD (context, "macro eNB id: %05x", m2ap_enb_id);
  }
  OAILOG_MESSAGE_FINISH(context);
  global_enb_id = &ie->value.choice.GlobalENB_ID;

  mme_config_read_lock (&mme_config);
  max_m2_enb_connected = mme_config.max_m2_enbs;
  mme_config_unlock (&mme_config);

  if (nb_enb_associated == max_m2_enb_connected) {
    OAILOG_ERROR (LOG_MXAP, "There is too much M2 eNB connected to MME, rejecting the association\n");
    OAILOG_DEBUG (LOG_MXAP, "Connected = %d, maximum allowed = %d\n", nb_enb_associated, max_m2_enb_connected);
      /*
       * Send an overload cause...
       */
    rc = m2ap_mce_generate_m2_setup_failure (assoc_id, M2AP_Cause_PR_misc, M2AP_CauseMisc_control_processing_overload, M2AP_TimeToWait_v20s);
    OAILOG_FUNC_RETURN (LOG_MXAP, rc);
  }

  if(m2ap_mme_compare_gummei(&global_enb_id->pLMN_Identity) != RETURNok){
	OAILOG_ERROR (LOG_MXAP, "No Common GUMMEI with eNB, generate_m2_setup_failure\n");
	rc =  m2ap_mce_generate_m2_setup_failure (assoc_id, M2AP_Cause_PR_misc, M2AP_CauseMisc_unspecified, M2AP_TimeToWait_v20s);
	OAILOG_FUNC_RETURN (LOG_MXAP, rc);
  }

  /* Requirement MME36.413R10_8.7.3.4 Abnormal Conditions
     * If the eNB initiates the procedure by sending a M2 SETUP REQUEST message including the PLMN Identity IEs and
     * none of the PLMNs provided by the eNB is identified by the MME, then the MME shall reject the eNB M2 Setup
     * Request procedure with the appropriate cause value, e.g, “Unknown PLMN”.
     */
  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_M2SetupRequest_IEs_t, ie_enb_mbms_configuration_per_cell, container,
		  M2AP_ProtocolIE_ID_id_ENB_MBMS_Configuration_data_Item, true);

  M2AP_ENB_MBMS_Configuration_data_List_t	* enb_mbms_cfg_data_list = &ie_enb_mbms_configuration_per_cell->value.choice.ENB_MBMS_Configuration_data_List;
  /** Take the first cell. */
  if(enb_mbms_cfg_data_list->list.count > 1){
    OAILOG_WARNING (LOG_MXAP, "More than 1 cell is not supported for the eNB MBMS. Evaluating only the first cell. \n");
  }
  M2AP_ENB_MBMS_Configuration_data_Item_t 	* enb_mbms_cfg_item = enb_mbms_cfg_item = enb_mbms_cfg_data_list->list.array[0];
  DevAssert(enb_mbms_cfg_item);
  mbms_sa_ret = m2ap_mce_compare_mbms_enb_configuration_item(enb_mbms_cfg_item);
  /*
   * eNB (first cell) and MCE have no common MBMS SA
   */
  if (mbms_sa_ret != MBMS_SA_LIST_RET_OK) {
    OAILOG_ERROR (LOG_MXAP, "No Common PLMN/MBMS_SA with eNB, generate_m2_setup_failure\n");
    rc = m2ap_mce_generate_m2_setup_failure(assoc_id,
    		M2AP_Cause_PR_misc,
			M2AP_CauseMisc_unspecified,
			M2AP_TimeToWait_v20s);
    // todo: test leaks..
    OAILOG_FUNC_RETURN (LOG_MXAP, rc);
  }

  OAILOG_DEBUG (LOG_MXAP, "Adding eNB with eNB-ID %d to the list of served MBMS eNBs. \n", m2ap_enb_id);
  if ((m2ap_enb_association = m2ap_is_enb_id_in_list (m2ap_enb_id)) == NULL) {
    /*
     * eNB has not been fount in list of associated eNB,
     * * * * Add it to the tail of list and initialize data
     */
    if ((m2ap_enb_association = m2ap_is_enb_assoc_id_in_list (assoc_id)) == NULL) {
      OAILOG_ERROR(LOG_MXAP, "Ignoring m2 setup from unknown assoc %u and enbId %u", assoc_id, m2ap_enb_id);
      OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
    } else {
      m2ap_enb_association->m2_state = M2AP_RESETING;
      OAILOG_DEBUG (LOG_MXAP, "Adding eNB id %u to the list of served eNBs\n", m2ap_enb_id);
      m2ap_enb_association->m2ap_enb_id = m2ap_enb_id;
      /*
       * Bad, very bad cast...
       */
      // todo: handle eCGI, synchronisation, service area
      M2AP_MBSFN_SynchronisationArea_ID_t * mbsfn_synchronisation_area = &enb_mbms_cfg_item->mbsfnSynchronisationArea;
      /** Set it into the MBMS eNB Description. */
      m2ap_set_mbms_sa_list(m2ap_enb_association, &enb_mbms_cfg_item->eCGI, &enb_mbms_cfg_item->mbmsServiceAreaList);
      if (m2ap_enb_name != NULL) {
    	  memcpy (m2ap_enb_association->m2ap_enb_name, ie_enb_name->value.choice.ENBname.buf, ie_enb_name->value.choice.ENBname.size);
    	  m2ap_enb_association->m2ap_enb_name[ie_enb_name->value.choice.ENBname.size] = '\0';
      }
    }
  }	else {
	m2ap_enb_association->m2_state = M2AP_RESETING;

	/*
	 * eNB has been fount in list, consider the m2 setup request as a reset connection,
	 * * * * reseting any previous MBMS state if sctp association is != than the previous one
	 */
	if (m2ap_enb_association->sctp_assoc_id != assoc_id) {
	  OAILOG_ERROR (LOG_MXAP, "Rejecting m2 setup request as eNB id %d is already associated to an active sctp association" "Previous known: %d, new one: %d\n",
		m2ap_enb_id, m2ap_enb_association->sctp_assoc_id, assoc_id);
	  rc = m2ap_mce_generate_m2_setup_failure (assoc_id, M2AP_Cause_PR_misc, M2AP_CauseMisc_unspecified, -1); /**< eNB should attach again. */
	  /** Also remove the old eNB. */
	  OAILOG_INFO(LOG_MXAP, "Rejecting the old eNB connection for eNB id %d and old assoc_id: %d\n", m2ap_enb_id, assoc_id);
	  m2ap_dump_enb_list();
	  m2ap_handle_sctp_disconnection(m2ap_enb_association->sctp_assoc_id, false);
	  OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
	}

	OAILOG_INFO(LOG_MXAP, "We found the eNB id %d in the current list of enbs with matching sctp associations:" "assoc: %d\n", m2ap_enb_id, m2ap_enb_association->sctp_assoc_id);
	/*
	 * TODO: call the reset procedure
	 */
	m2ap_dump_enb (m2ap_enb_association);
	rc =  m2ap_generate_m2_setup_response (m2ap_enb_association);
	OAILOG_FUNC_RETURN (LOG_MXAP, rc);
  }
  m2ap_dump_enb (m2ap_enb_association);
  rc =  m2ap_generate_m2_setup_response (m2ap_enb_association);
  if (rc == RETURNok) {
    update_mme_app_stats_connected_enb_add();
  }
  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}

//------------------------------------------------------------------------------
static
  int
m2ap_generate_m2_setup_response (
  m2ap_enb_description_t * m2ap_enb_association)
{
  int                                     			  i,j;
  M2AP_M2AP_PDU_t                         			  pdu;
  M2AP_M2SetupResponse_t                 			 *out;
  M2AP_M2SetupResponse_IEs_t             			 *ie = NULL;
  M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item_t     *mbsfnCfgItem = NULL;
  uint8_t                                			 *buffer = NULL;
  uint32_t                                			  length = 0;
  int                                     			  rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MXAP);
  DevAssert (m2ap_enb_association != NULL);
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = M2AP_ProcedureCode_id_m2Setup;
  pdu.choice.successfulOutcome.criticality = M2AP_Criticality_reject;
  pdu.choice.successfulOutcome.value.present = M2AP_SuccessfulOutcome__value_PR_M2SetupResponse;
  out = &pdu.choice.successfulOutcome.value.choice.M2SetupResponse;

  // Generating response
  mme_config_read_lock (&mme_config);

  /** mandatory. */
  ie = (M2AP_M2SetupResponse_IEs_t *)calloc(1, sizeof(M2AP_M2SetupResponse_IEs_t));
  ie->id = M2AP_ProtocolIE_ID_id_GlobalMCE_ID;
  ie->criticality = M2AP_Criticality_ignore;
  ie->value.present = M2AP_M2SetupResponse_IEs__value_PR_GlobalMCE_ID;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  M2AP_PLMN_Identity_t                    *plmn = NULL;
  plmn = calloc (1, sizeof (*plmn));
  MCC_MNC_TO_PLMNID (mme_config.served_mbms_sa.plmn_mcc[i], mme_config.served_mbms_sa.plmn_mnc[i], mme_config.served_mbms_sa.plmn_mnc_len[i],
		  &ie->value.choice.GlobalMCE_ID.pLMN_Identity);
  /** Use same MME code for MCE ID for all eNBs. */
  INT16_TO_OCTET_STRING (mme_config.gummei.gummei[0].mme_code, &ie->value.choice.GlobalMCE_ID.mCE_ID);

  /** Skip the MCC name. */

  /** mandatory. **/
  //  MCCH related BCCH Configuration data per MBSFN area

  ie = (M2AP_M2SetupResponse_IEs_t *)calloc(1, sizeof(M2AP_M2SetupResponse_IEs_t));
  ie->id = M2AP_ProtocolIE_ID_id_MCCHrelatedBCCH_ConfigPerMBSFNArea;
  ie->criticality = M2AP_Criticality_reject;
  ie->value.present = M2AP_M2SetupResponse_IEs__value_PR_MCCHrelatedBCCH_ConfigPerMBSFNArea;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /** Only supporting a single MBSFN Area currently. */
  // memset for gcc 4.8.4 instead of {0}, servedGUMMEI.servedPLMNs
  mbsfnCfgItem = calloc(1, sizeof *mbsfnCfgItem);
  ASN_SEQUENCE_ADD(&ie->value.choice.MCCHrelatedBCCH_ConfigPerMBSFNArea.list, mbsfnCfgItem);
  // todo: fill the MBSFN item

  mme_config_unlock (&mme_config);

  if (m2ap_mce_encode_pdu (&pdu, &buffer, &length) < 0) {
    OAILOG_DEBUG (LOG_S1AP, "Removed eNB %d\n", m2ap_enb_association->sctp_assoc_id);
    hashtable_ts_free (&g_m2ap_enb_coll, m2ap_enb_association->sctp_assoc_id);
  } else {
	/*
	 * Consider the response as sent. S1AP is ready to accept UE contexts
	 */
	m2ap_enb_association->m2_state = M2AP_READY;
  }

  /*
   * Non-MBMS signalling -> stream 0
   */
  bstring b = blk2bstr(buffer, length);
  free(buffer);
  rc = m2ap_mce_itti_send_sctp_request (&b, m2ap_enb_association->sctp_assoc_id, 0, INVALID_MCE_MBMS_M2AP_ID);
  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}

////////////////////////////////////////////////////////////////////////////////
//**************** MBMS Service Context Management procedures ****************//
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_session_start_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu)
{
//  M2AP_InitialContextSetupResponse_t     *container;
//  M2AP_InitialContextSetupResponseIEs_t  *ie = NULL;
//  M2AP_E_RABSetupItemCtxtSUResIEs_t      *eRABSetupItemCtxtSURes_p = NULL;
//  M2AP_E_RABSetupItemCtxtSURes_t	     *e_rab_setup_item_ctxt_su_res = NULL;
//  mbms_description_t                       *mbms_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;
  mce_mbms_m2ap_id_t                      mce_mbms_m2ap_id = 0;
  enb_mbms_m2ap_id_t                      enb_mbms_m2ap_id = 0;

  OAILOG_FUNC_IN (LOG_MXAP);

//  container = &pdu->choice.successfulOutcome.value.choice.InitialContextSetupResponse;
//
//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_InitialContextSetupResponseIEs_t, ie, container,
//                             M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID, true);
//  mce_mbms_m2ap_id = ie->value.choice.MCE_MBMS_M2AP_ID;
//
//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_InitialContextSetupResponseIEs_t, ie, container,
//                             M2AP_ProtocolIE_ID_id_eNB_MBMS_M2AP_ID, true);
//  enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t) (ie->value.choice.ENB_MBMS_M2AP_ID & ENB_MBMS_M2AP_ID_MASK);
//
//  if ((mbms_ref_p = m2ap_is_ue_mme_id_in_list ((uint32_t) mce_mbms_m2ap_id)) == NULL) {
//    OAILOG_DEBUG (LOG_MXAP, "No MBMS is attached to this mme MBMS s1ap id: " MCE_MBMS_M2AP_ID_FMT " %u(10)\n",
//                      (uint32_t) mce_mbms_m2ap_id, (uint32_t) mce_mbms_m2ap_id);
//    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//  }
//
//  if (mbms_ref_p->enb_mbms_m2ap_id != enb_mbms_m2ap_id) {
//    OAILOG_DEBUG (LOG_MXAP, "Mismatch in eNB MBMS M2AP ID, known: " ENB_MBMS_M2AP_ID_FMT " %u(10), received: 0x%06x %u(10)\n",
//                mbms_ref_p->enb_mbms_m2ap_id, mbms_ref_p->enb_mbms_m2ap_id, (uint32_t) enb_mbms_m2ap_id, (uint32_t) enb_mbms_m2ap_id);
//    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//  }
//
//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_InitialContextSetupResponseIEs_t, ie, container,
//                             M2AP_ProtocolIE_ID_id_E_RABSetupListCtxtSURes, true);
//  if (ie->value.choice.E_RABSetupListCtxtSURes.list.count < 1) {
//
//    OAILOG_DEBUG (LOG_MXAP, "E-RAB creation has failed\n");
//    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//  }
//
//  /** Check if a context removal is ongoing. */
//  if(mbms_ref_p->s1_ue_state == M2AP_MBMS_WAITING_CRR && mbms_ref_p->m2ap_ue_context_rel_timer.id != M2AP_TIMER_INACTIVE_ID){
//    OAILOG_ERROR(LOG_MXAP, "A context release procedure is goingoing for ueRerefernce with ueId " MCE_MBMS_M2AP_ID_FMT" and enbUeId " ENB_MBMS_M2AP_ID_FMT ". Ignoring received Setup Response. \n", mbms_ref_p->mce_mbms_m2ap_id, mbms_ref_p->enb_mbms_m2ap_id);
//    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//  }
//
//  mbms_ref_p->s1_ue_state = M2AP_MBMS_CONNECTED;
//  message_p = itti_alloc_new_message (TASK_S1AP, MCE_APP_INITIAL_CONTEXT_SETUP_RSP);
//  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
//  memset ((void *)&message_p->ittiMsg.mme_app_initial_context_setup_rsp, 0, sizeof (itti_mme_app_initial_context_setup_rsp_t));
//
//
//  itti_mme_app_initial_context_setup_rsp_t * initial_context_setup_rsp = NULL;
//  initial_context_setup_rsp = &message_p->ittiMsg.mme_app_initial_context_setup_rsp;
//
//  initial_context_setup_rsp->ue_id = mbms_ref_p->mce_mbms_m2ap_id;
//  /** Add here multiple bearers. */
//  bool test = false;
//  initial_context_setup_rsp->bcs_to_be_modified.num_bearer_context = ie->value.choice.E_RABSetupListCtxtSURes.list.count;
//  for (int item = 0; item < ie->value.choice.E_RABSetupListCtxtSURes.list.count; item++) {
//    int item2 = item;
//    /*
//     * Bad, very bad cast...
//     */
//    eRABSetupItemCtxtSURes_p = (M2AP_E_RABSetupItemCtxtSUResIEs_t *)
//        ie->value.choice.E_RABSetupListCtxtSURes.list.array[item];
//
//    e_rab_setup_item_ctxt_su_res = &eRABSetupItemCtxtSURes_p->value.choice.E_RABSetupItemCtxtSURes;
////
////    if(e_rab_setup_item_ctxt_su_res->e_RAB_ID == 7){
////    	test = true;
////    	initial_context_setup_rsp->bcs_to_be_modified.num_bearer_context--;
////    	continue;
////    }
////    if(test == true){
////    	if(item2 > 0)
////    		item2--;
////    }
//
//    initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].eps_bearer_id = e_rab_setup_item_ctxt_su_res->e_RAB_ID;
//    initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.teid = htonl (*((uint32_t *) e_rab_setup_item_ctxt_su_res->gTP_TEID.buf));
//    bstring transport_address = blk2bstr(e_rab_setup_item_ctxt_su_res->transportLayerAddress.buf, e_rab_setup_item_ctxt_su_res->transportLayerAddress.size);
//
//    /** Set the IP address from the FTEID. */
//    if (4 == blength(transport_address)) {
//    	initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv4 = 1;
//    	memcpy(&initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv4_address, transport_address->data, blength(transport_address));
//    } else if (16 == blength(transport_address)) {
//    	initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv6 = 1;
//    	memcpy(&initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv6_address, transport_address->data, blength(transport_address));
//    } else {
//    	AssertFatal(0, "TODO IP address %d bytes", blength(transport_address));
//    }
//    bdestroy_wrapper(&transport_address);
//  }
//
//  /** Get the failed bearers. */
//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_InitialContextSetupResponseIEs_t, ie, container,
//                             M2AP_ProtocolIE_ID_id_E_RABFailedToSetupListBearerSURes, false);
//  if (ie) {
//    M2AP_E_RABList_t *m2ap_e_rab_list = &ie->value.choice.E_RABList;
//    for (int index = 0; index < m2ap_e_rab_list->list.count; index++) {
//      M2AP_E_RABItem_t * erab_item = (M2AP_E_RABItem_t *)m2ap_e_rab_list->list.array[index];
//      initial_context_setup_rsp->e_rab_release_list.item[initial_context_setup_rsp->e_rab_release_list.no_of_items].e_rab_id  = erab_item->e_RAB_ID;
//      initial_context_setup_rsp->e_rab_release_list.item[initial_context_setup_rsp->e_rab_release_list.no_of_items].cause     = erab_item->cause;
//      initial_context_setup_rsp->e_rab_release_list.no_of_items++;
//    }
//  }

  rc =  itti_send_msg_to_task (TASK_MCE_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_session_start_failure (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu)
{
//  M2AP_InitialContextSetupResponse_t     *container;
//  M2AP_InitialContextSetupResponseIEs_t  *ie = NULL;
//  M2AP_E_RABSetupItemCtxtSUResIEs_t      *eRABSetupItemCtxtSURes_p = NULL;
//  M2AP_E_RABSetupItemCtxtSURes_t	     *e_rab_setup_item_ctxt_su_res = NULL;
//  mbms_description_t                       *mbms_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;
  mce_mbms_m2ap_id_t                      mce_mbms_m2ap_id = 0;
  enb_mbms_m2ap_id_t                      enb_mbms_m2ap_id = 0;

  OAILOG_FUNC_IN (LOG_MXAP);


  rc =  itti_send_msg_to_task (TASK_MCE_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}


//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_session_stop_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu)
{
//  M2AP_InitialContextSetupResponse_t     *container;
//  M2AP_InitialContextSetupResponseIEs_t  *ie = NULL;
//  M2AP_E_RABSetupItemCtxtSUResIEs_t      *eRABSetupItemCtxtSURes_p = NULL;
//  M2AP_E_RABSetupItemCtxtSURes_t	     *e_rab_setup_item_ctxt_su_res = NULL;
//  mbms_description_t                       *mbms_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;
  mce_mbms_m2ap_id_t                      mce_mbms_m2ap_id = 0;
  enb_mbms_m2ap_id_t                      enb_mbms_m2ap_id = 0;

  OAILOG_FUNC_IN (LOG_MXAP);

//  container = &pdu->choice.successfulOutcome.value.choice.InitialContextSetupResponse;
//
//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_InitialContextSetupResponseIEs_t, ie, container,
//                             M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID, true);
//  mce_mbms_m2ap_id = ie->value.choice.MCE_MBMS_M2AP_ID;
//
//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_InitialContextSetupResponseIEs_t, ie, container,
//                             M2AP_ProtocolIE_ID_id_eNB_MBMS_M2AP_ID, true);
//  enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t) (ie->value.choice.ENB_MBMS_M2AP_ID & ENB_MBMS_M2AP_ID_MASK);
//
//  if ((mbms_ref_p = m2ap_is_ue_mme_id_in_list ((uint32_t) mce_mbms_m2ap_id)) == NULL) {
//    OAILOG_DEBUG (LOG_MXAP, "No MBMS is attached to this mme MBMS s1ap id: " MCE_MBMS_M2AP_ID_FMT " %u(10)\n",
//                      (uint32_t) mce_mbms_m2ap_id, (uint32_t) mce_mbms_m2ap_id);
//    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//  }
//
//  if (mbms_ref_p->enb_mbms_m2ap_id != enb_mbms_m2ap_id) {
//    OAILOG_DEBUG (LOG_MXAP, "Mismatch in eNB MBMS M2AP ID, known: " ENB_MBMS_M2AP_ID_FMT " %u(10), received: 0x%06x %u(10)\n",
//                mbms_ref_p->enb_mbms_m2ap_id, mbms_ref_p->enb_mbms_m2ap_id, (uint32_t) enb_mbms_m2ap_id, (uint32_t) enb_mbms_m2ap_id);
//    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//  }
//
//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_InitialContextSetupResponseIEs_t, ie, container,
//                             M2AP_ProtocolIE_ID_id_E_RABSetupListCtxtSURes, true);
//  if (ie->value.choice.E_RABSetupListCtxtSURes.list.count < 1) {
//
//    OAILOG_DEBUG (LOG_MXAP, "E-RAB creation has failed\n");
//    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//  }
//
//  /** Check if a context removal is ongoing. */
//  if(mbms_ref_p->s1_ue_state == M2AP_MBMS_WAITING_CRR && mbms_ref_p->m2ap_ue_context_rel_timer.id != M2AP_TIMER_INACTIVE_ID){
//    OAILOG_ERROR(LOG_MXAP, "A context release procedure is goingoing for ueRerefernce with ueId " MCE_MBMS_M2AP_ID_FMT" and enbUeId " ENB_MBMS_M2AP_ID_FMT ". Ignoring received Setup Response. \n", mbms_ref_p->mce_mbms_m2ap_id, mbms_ref_p->enb_mbms_m2ap_id);
//    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//  }
//
//  mbms_ref_p->s1_ue_state = M2AP_MBMS_CONNECTED;
//  message_p = itti_alloc_new_message (TASK_S1AP, MCE_APP_INITIAL_CONTEXT_SETUP_RSP);
//  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
//  memset ((void *)&message_p->ittiMsg.mme_app_initial_context_setup_rsp, 0, sizeof (itti_mme_app_initial_context_setup_rsp_t));
//
//
//  itti_mme_app_initial_context_setup_rsp_t * initial_context_setup_rsp = NULL;
//  initial_context_setup_rsp = &message_p->ittiMsg.mme_app_initial_context_setup_rsp;
//
//  initial_context_setup_rsp->ue_id = mbms_ref_p->mce_mbms_m2ap_id;
//  /** Add here multiple bearers. */
//  bool test = false;
//  initial_context_setup_rsp->bcs_to_be_modified.num_bearer_context = ie->value.choice.E_RABSetupListCtxtSURes.list.count;
//  for (int item = 0; item < ie->value.choice.E_RABSetupListCtxtSURes.list.count; item++) {
//    int item2 = item;
//    /*
//     * Bad, very bad cast...
//     */
//    eRABSetupItemCtxtSURes_p = (M2AP_E_RABSetupItemCtxtSUResIEs_t *)
//        ie->value.choice.E_RABSetupListCtxtSURes.list.array[item];
//
//    e_rab_setup_item_ctxt_su_res = &eRABSetupItemCtxtSURes_p->value.choice.E_RABSetupItemCtxtSURes;
////
////    if(e_rab_setup_item_ctxt_su_res->e_RAB_ID == 7){
////    	test = true;
////    	initial_context_setup_rsp->bcs_to_be_modified.num_bearer_context--;
////    	continue;
////    }
////    if(test == true){
////    	if(item2 > 0)
////    		item2--;
////    }
//
//    initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].eps_bearer_id = e_rab_setup_item_ctxt_su_res->e_RAB_ID;
//    initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.teid = htonl (*((uint32_t *) e_rab_setup_item_ctxt_su_res->gTP_TEID.buf));
//    bstring transport_address = blk2bstr(e_rab_setup_item_ctxt_su_res->transportLayerAddress.buf, e_rab_setup_item_ctxt_su_res->transportLayerAddress.size);
//
//    /** Set the IP address from the FTEID. */
//    if (4 == blength(transport_address)) {
//    	initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv4 = 1;
//    	memcpy(&initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv4_address, transport_address->data, blength(transport_address));
//    } else if (16 == blength(transport_address)) {
//    	initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv6 = 1;
//    	memcpy(&initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv6_address, transport_address->data, blength(transport_address));
//    } else {
//    	AssertFatal(0, "TODO IP address %d bytes", blength(transport_address));
//    }
//    bdestroy_wrapper(&transport_address);
//  }
//
//  /** Get the failed bearers. */
//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_InitialContextSetupResponseIEs_t, ie, container,
//                             M2AP_ProtocolIE_ID_id_E_RABFailedToSetupListBearerSURes, false);
//  if (ie) {
//    M2AP_E_RABList_t *m2ap_e_rab_list = &ie->value.choice.E_RABList;
//    for (int index = 0; index < m2ap_e_rab_list->list.count; index++) {
//      M2AP_E_RABItem_t * erab_item = (M2AP_E_RABItem_t *)m2ap_e_rab_list->list.array[index];
//      initial_context_setup_rsp->e_rab_release_list.item[initial_context_setup_rsp->e_rab_release_list.no_of_items].e_rab_id  = erab_item->e_RAB_ID;
//      initial_context_setup_rsp->e_rab_release_list.item[initial_context_setup_rsp->e_rab_release_list.no_of_items].cause     = erab_item->cause;
//      initial_context_setup_rsp->e_rab_release_list.no_of_items++;
//    }
//  }

  rc =  itti_send_msg_to_task (TASK_MCE_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_session_update_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu)
{
//  M2AP_InitialContextSetupResponse_t     *container;
//  M2AP_InitialContextSetupResponseIEs_t  *ie = NULL;
  mbms_description_t                *mbms_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;
  mce_mbms_m2ap_id_t                      mce_mbms_m2ap_id = 0;
  enb_mbms_m2ap_id_t                      enb_mbms_m2ap_id = 0;

  OAILOG_FUNC_IN (LOG_MXAP);

//  container = &pdu->choice.successfulOutcome.value.choice.InitialContextSetupResponse;
//
//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_InitialContextSetupResponseIEs_t, ie, container,
//                             M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID, true);
//  mce_mbms_m2ap_id = ie->value.choice.MCE_MBMS_M2AP_ID;
//
//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_InitialContextSetupResponseIEs_t, ie, container,
//                             M2AP_ProtocolIE_ID_id_eNB_MBMS_M2AP_ID, true);
//  enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t) (ie->value.choice.ENB_MBMS_M2AP_ID & ENB_MBMS_M2AP_ID_MASK);
//
//  if ((mbms_ref_p = m2ap_is_ue_mme_id_in_list ((uint32_t) mce_mbms_m2ap_id)) == NULL) {
//    OAILOG_DEBUG (LOG_MXAP, "No MBMS is attached to this mme MBMS s1ap id: " MCE_MBMS_M2AP_ID_FMT " %u(10)\n",
//                      (uint32_t) mce_mbms_m2ap_id, (uint32_t) mce_mbms_m2ap_id);
//    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//  }
//
//  if (mbms_ref_p->enb_mbms_m2ap_id != enb_mbms_m2ap_id) {
//    OAILOG_DEBUG (LOG_MXAP, "Mismatch in eNB MBMS M2AP ID, known: " ENB_MBMS_M2AP_ID_FMT " %u(10), received: 0x%06x %u(10)\n",
//                mbms_ref_p->enb_mbms_m2ap_id, mbms_ref_p->enb_mbms_m2ap_id, (uint32_t) enb_mbms_m2ap_id, (uint32_t) enb_mbms_m2ap_id);
//    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//  }
//
//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_InitialContextSetupResponseIEs_t, ie, container,
//                             M2AP_ProtocolIE_ID_id_E_RABSetupListCtxtSURes, true);
//  if (ie->value.choice.E_RABSetupListCtxtSURes.list.count < 1) {
//
//    OAILOG_DEBUG (LOG_MXAP, "E-RAB creation has failed\n");
//    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//  }
//
//  /** Check if a context removal is ongoing. */
//  if(mbms_ref_p->s1_ue_state == M2AP_MBMS_WAITING_CRR && mbms_ref_p->m2ap_ue_context_rel_timer.id != M2AP_TIMER_INACTIVE_ID){
//    OAILOG_ERROR(LOG_MXAP, "A context release procedure is goingoing for ueRerefernce with ueId " MCE_MBMS_M2AP_ID_FMT" and enbUeId " ENB_MBMS_M2AP_ID_FMT ". Ignoring received Setup Response. \n", mbms_ref_p->mce_mbms_m2ap_id, mbms_ref_p->enb_mbms_m2ap_id);
//    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//  }
//
//  mbms_ref_p->s1_ue_state = M2AP_MBMS_CONNECTED;
//  message_p = itti_alloc_new_message (TASK_S1AP, MCE_APP_INITIAL_CONTEXT_SETUP_RSP);
//  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
//  memset ((void *)&message_p->ittiMsg.mme_app_initial_context_setup_rsp, 0, sizeof (itti_mme_app_initial_context_setup_rsp_t));
//
//
//  itti_mme_app_initial_context_setup_rsp_t * initial_context_setup_rsp = NULL;
//  initial_context_setup_rsp = &message_p->ittiMsg.mme_app_initial_context_setup_rsp;
//
//  initial_context_setup_rsp->ue_id = mbms_ref_p->mce_mbms_m2ap_id;
//  /** Add here multiple bearers. */
//  bool test = false;
//  initial_context_setup_rsp->bcs_to_be_modified.num_bearer_context = ie->value.choice.E_RABSetupListCtxtSURes.list.count;
//  for (int item = 0; item < ie->value.choice.E_RABSetupListCtxtSURes.list.count; item++) {
//    int item2 = item;
//    /*
//     * Bad, very bad cast...
//     */
//    eRABSetupItemCtxtSURes_p = (M2AP_E_RABSetupItemCtxtSUResIEs_t *)
//        ie->value.choice.E_RABSetupListCtxtSURes.list.array[item];
//
//    e_rab_setup_item_ctxt_su_res = &eRABSetupItemCtxtSURes_p->value.choice.E_RABSetupItemCtxtSURes;
////
////    if(e_rab_setup_item_ctxt_su_res->e_RAB_ID == 7){
////    	test = true;
////    	initial_context_setup_rsp->bcs_to_be_modified.num_bearer_context--;
////    	continue;
////    }
////    if(test == true){
////    	if(item2 > 0)
////    		item2--;
////    }
//
//    initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].eps_bearer_id = e_rab_setup_item_ctxt_su_res->e_RAB_ID;
//    initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.teid = htonl (*((uint32_t *) e_rab_setup_item_ctxt_su_res->gTP_TEID.buf));
//    bstring transport_address = blk2bstr(e_rab_setup_item_ctxt_su_res->transportLayerAddress.buf, e_rab_setup_item_ctxt_su_res->transportLayerAddress.size);
//
//    /** Set the IP address from the FTEID. */
//    if (4 == blength(transport_address)) {
//    	initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv4 = 1;
//    	memcpy(&initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv4_address, transport_address->data, blength(transport_address));
//    } else if (16 == blength(transport_address)) {
//    	initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv6 = 1;
//    	memcpy(&initial_context_setup_rsp->bcs_to_be_modified.bearer_context[item2].s1_eNB_fteid.ipv6_address, transport_address->data, blength(transport_address));
//    } else {
//    	AssertFatal(0, "TODO IP address %d bytes", blength(transport_address));
//    }
//    bdestroy_wrapper(&transport_address);
//  }
//
//  /** Get the failed bearers. */
//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_InitialContextSetupResponseIEs_t, ie, container,
//                             M2AP_ProtocolIE_ID_id_E_RABFailedToSetupListBearerSURes, false);
//  if (ie) {
//    M2AP_E_RABList_t *m2ap_e_rab_list = &ie->value.choice.E_RABList;
//    for (int index = 0; index < m2ap_e_rab_list->list.count; index++) {
//      M2AP_E_RABItem_t * erab_item = (M2AP_E_RABItem_t *)m2ap_e_rab_list->list.array[index];
//      initial_context_setup_rsp->e_rab_release_list.item[initial_context_setup_rsp->e_rab_release_list.no_of_items].e_rab_id  = erab_item->e_RAB_ID;
//      initial_context_setup_rsp->e_rab_release_list.item[initial_context_setup_rsp->e_rab_release_list.no_of_items].cause     = erab_item->cause;
//      initial_context_setup_rsp->e_rab_release_list.no_of_items++;
//    }
//  }

  rc =  itti_send_msg_to_task (TASK_MCE_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_session_update_failure (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu)
{
  M2AP_SessionUpdateFailure_t       *container;
  M2AP_SessionUpdateFailure_Ies_t   *ie = NULL;
  mbms_description_t                *mbms_ref_p = NULL;
  MessageDef                        *message_p = NULL;
  int                                rc = RETURNok;
  mce_mbms_m2ap_id_t                 mce_mbms_m2ap_id = 0;
  enb_mbms_m2ap_id_t                 enb_mbms_m2ap_id = 0;

  OAILOG_FUNC_IN (LOG_MXAP);

  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_scheduling_information_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu) {
  M2AP_MbmsSchedulingInformationResponse_t        *container;
  M2AP_MbmsSchedulingInformationResponse_IEs_t    *ie = NULL;
  mbms_description_t                *mbms_ref_p = NULL;
  MessageDef                        *message_p = NULL;
  int                                rc = RETURNok;
  mce_mbms_m2ap_id_t                 mce_mbms_m2ap_id = 0;
  enb_mbms_m2ap_id_t                 enb_mbms_m2ap_id = 0;
  OAILOG_FUNC_IN (LOG_MXAP);


  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}

//------------------------------------------------------------------------------
typedef struct arg_m2ap_construct_enb_reset_req_s {
  uint8_t      current_mbms_index;
  MessageDef  *message_p;
}arg_m2ap_construct_enb_reset_req_t;

//------------------------------------------------------------------------------
int
m2ap_mce_handle_reset_request (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu) {
  M2AP_Reset_t					    *container;
  M2AP_Reset_Ies_t				    *ie = NULL;
  mbms_description_t                *mbms_ref_p = NULL;
  MessageDef                        *message_p = NULL;
  int                                rc = RETURNok;
  m2ap_enb_description_t			*m2ap_enb_association = NULL;
  mce_mbms_m2ap_id_t                 mce_mbms_m2ap_id = 0;
  enb_mbms_m2ap_id_t                 enb_mbms_m2ap_id = 0;
  OAILOG_FUNC_IN (LOG_MXAP);

  arg_m2ap_construct_enb_reset_req_t      	 arg = {0};
  uint32_t	                                 item = 0;

  OAILOG_FUNC_IN (LOG_MXAP);

  /*
   * Checking that the assoc id has a valid eNB attached to.
   */
  m2ap_enb_association = m2ap_is_enb_assoc_id_in_list (assoc_id);

  if (m2ap_enb_association == NULL) {
    OAILOG_ERROR (LOG_MXAP, "No eNB attached to this assoc_id: %d\n", assoc_id);
    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
  }

  // Check the state of M2AP
  if (m2ap_enb_association->m2_state != M2AP_READY) {
    // Ignore the message
    OAILOG_INFO (LOG_MXAP, "M2 setup is not done.Invalid state.Ignoring ENB Initiated Reset.eNB Id = %d , M2AP state = %d \n", m2ap_enb_association->m2ap_enb_id, m2ap_enb_association->m2_state);
    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNok);
  }
  // Check the reset type - partial_reset OR M2AP_RESET_ALL
  container = &pdu->choice.initiatingMessage.value.choice.Reset;

  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_Reset_Ies_t, ie, container,
                           M2AP_ProtocolIE_ID_id_ResetType, true);
  M2AP_ResetType_t * m2ap_reset_type = &ie->value.choice.ResetType;

  switch (m2ap_reset_type->present){
    case M2AP_ResetType_PR_m2_Interface:
      m2ap_reset_type = M2AP_RESET_ALL;
	  break;
    case M2AP_ResetType_PR_partOfM2_Interface:
      m2ap_reset_type = M2AP_RESET_PARTIAL;
  	  break;
    default:
	  OAILOG_ERROR (LOG_MXAP, "Reset Request from eNB  with Invalid reset_type = %d\n", m2ap_reset_type->present);
	  // TBD - Here MME should send Error Indication as it is abnormal scenario.
	  OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
  }
  /** Create a cleared array of identifiers. */
  int num_mbms_assoc = 0;
  // Partial Reset
  if (m2ap_reset_type != M2AP_RESET_ALL) {
	  // Check if there are no MBMSs connected
	  if (!m2ap_enb_association->nb_mbms_associated) {
	    // Ignore the message
	    OAILOG_INFO (LOG_MXAP, "No MBMSs is connected.Ignoring ENB Initiated Reset.eNB Id = %d\n", m2ap_enb_association->m2ap_enb_id);
	    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNok);
	  }
	  num_mbms_assoc = m2ap_reset_type->choice.partOfM2_Interface.list.count;
	  m2_sig_conn_id_t mbms_to_reset_list[num_mbms_assoc]; /**< Create a stacked array. */
	  memset(mbms_to_reset_list, 0, sizeof(mbms_to_reset_list));
	  for (item = 0; item < num_mbms_assoc; item++) {
//		  /** Decode Protocol IE here. */
////		  if (resetType->choice.partOfS1_Interface.list.array[i]->id != M2AP_ProtocolIE_ID_id_MBMS_associatedLogicalS1_ConnectionItem) {
////			  OAILOG_ERROR (LOG_MXAP, "Incorrect element in Bearers_SubjectToStatusTransferList: %d\n", resetType->choice.partOfS1_Interface.list.array[i]->id);
////			  OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
////		  }
//		  M2AP_MBMS_associatedLogicalS1_ConnectionItemResAck_t *m2_sig_conn_p = (M2AP_MBMS_associatedLogicalS1_ConnectionItemResAck_t*)
//		         ie->value.choice.ResetType.choice.partOfS1_Interface.list.array[item];
//		  if(!m2_sig_conn_p) {
//			  OAILOG_ERROR (LOG_MXAP, "No logical M2 connection item could be found for the partial connection. "
//					  "Ignoring the received partial reset request. \n");
//			  OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//		  }
//
//		  M2AP_MBMS_associatedLogicalS1_ConnectionItem_t * m2_sig_conn_item = &m2_sig_conn_p->value.choice.UE_associatedLogicalS1_ConnectionItem;
//		  if (m2_sig_conn_item->mCE_MBMS_M2AP_ID != NULL) {
//			  mce_mbms_m2ap_id_t mce_mbms_m2ap_id = (mce_mbms_m2ap_id_t) *(m2_sig_conn_item->mCE_MBMS_M2AP_ID);
//			  if ((mbms_ref_p = m2ap_is_ue_mme_id_in_list (mce_mbms_m2ap_id)) != NULL) {
//				  if (m2_sig_conn_item->eNB_MBMS_M2AP_ID != NULL) {
//					  enb_mbms_m2ap_id_t enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t) *(m2_sig_conn_item->eNB_MBMS_M2AP_ID);
//					  enb_mbms_m2ap_id &= ENB_MBMS_M2AP_ID_MASK;
//					  if (mbms_ref_p->enb_mbms_m2ap_id == enb_mbms_m2ap_id) {
//						  mbms_to_reset_list[item].mce_mbms_m2ap_id = &(mbms_ref_p->mce_mbms_m2ap_id);
//						  mbms_to_reset_list[item].enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t*)mbms_ref_p;
//					  } else {
//						  // mismatch in enb_mbms_m2ap_id sent by eNB and stored in M2AP ue context in EPC. Abnormal case.
//						  mbms_to_reset_list[item].mce_mbms_m2ap_id = NULL;
//						  mbms_to_reset_list[item].enb_mbms_m2ap_id = NULL;
//						  OAILOG_ERROR (LOG_MXAP, "Partial Reset Request:enb_mbms_m2ap_id mismatch between id %d sent by eNB and id %d stored in epc for mce_mbms_m2ap_id %d \n",
//								  enb_mbms_m2ap_id, mbms_ref_p->enb_mbms_m2ap_id, mce_mbms_m2ap_id);
//					  }
//				  } else {
//					  mbms_to_reset_list[item].mce_mbms_m2ap_id = &(mbms_ref_p->mce_mbms_m2ap_id);
//					  mbms_to_reset_list[item].enb_mbms_m2ap_id = NULL;
//				  }
//			  } else {
//				  OAILOG_ERROR (LOG_MXAP, "Partial Reset Request - No MBMS context found for mce_mbms_m2ap_id %d \n", mce_mbms_m2ap_id);
//				  // TBD - Here MME should send Error Indication as it is abnormal scenario.
//				 // todo: test leak ASN_STRUCT_FREE(asn_DEF_M2AP_MBMS_associatedLogicalS1_ConnectionItem, m2_sig_conn_id_p);
//				  OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//			  }
//		  } else {
//			  if (m2_sig_conn_item->eNB_MBMS_M2AP_ID != NULL) {
//				  enb_mbms_m2ap_id_t enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t) *(m2_sig_conn_item->eNB_MBMS_M2AP_ID);
//				  enb_mbms_m2ap_id &= ENB_MBMS_M2AP_ID_MASK;
//				  if ((mbms_ref_p = m2ap_is_ue_enb_id_in_list (m2ap_enb_association, enb_mbms_m2ap_id)) != NULL) {
//					  mbms_to_reset_list[item].enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t*)mbms_ref_p;
//					  if (mbms_ref_p->mce_mbms_m2ap_id != INVALID_MCE_MBMS_M2AP_ID) {
//						  mbms_to_reset_list[item].mce_mbms_m2ap_id = &(mbms_ref_p->mce_mbms_m2ap_id);
//					  } else {
//						  mbms_to_reset_list[item].mce_mbms_m2ap_id = NULL;
//					  }
//				  } else {
//					  OAILOG_ERROR (LOG_MXAP, "Partial Reset Request without any valid M2 signaling connection.Ignoring it \n");
//					  // TBD - Here MME should send Error Indication as it is abnormal scenario.
//					  // todo: test leak  ASN_STRUCT_FREE(asn_DEF_M2AP_MBMS_associatedLogicalS1_ConnectionItem, m2_sig_conn_id_p);
//					  OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//	          }
//	        } else {
//	          OAILOG_ERROR (LOG_MXAP, "Partial Reset Request without any valid M2 signaling connection.Ignoring it.\n");
//	          // TBD - Here MME should send Error Indication as it is abnormal scenario.
//	          // todo: test leak ASN_STRUCT_FREE(asn_DEF_M2AP_MBMS_associatedLogicalS1_ConnectionItem, m2_sig_conn_id_p);
//	          OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//	        }
//	      }
////		  if(m2_sig_conn_id_p)
//			  // todo: test leak   ASN_STRUCT_FREE(asn_DEF_M2AP_MBMS_associatedLogicalS1_ConnectionItem, m2_sig_conn_id_p);
	  }
      OAILOG_INFO (LOG_MXAP, "After iterating all %d partial MBMSs, informing the MCE_APP layer for the received Partial Reset Request.\n", num_mbms_assoc);
	  message_p = itti_alloc_new_message (TASK_S1AP, M3AP_ENB_INITIATED_RESET_REQ);
	  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
	  M3AP_ENB_INITIATED_RESET_REQ (message_p).num_mbms = num_mbms_assoc;
	  /** Allocate a new list and fill it directly. */
//	  M3AP_ENB_INITIATED_RESET_REQ (message_p).mbms_to_reset_list = (m2_sig_conn_id_t*) calloc (num_mbms_assoc, sizeof (m2_sig_conn_id_t));
//	  memcpy(M3AP_ENB_INITIATED_RESET_REQ (message_p).mbms_to_reset_list, mbms_to_reset_list, sizeof(mbms_to_reset_list));
  } else {
	  /** Generating full reset request. */
//	  OAILOG_INFO (LOG_MXAP, "Received a full reset request. Informing the MCE_APP layer.\n");
//	  num_mbms_assoc = m2ap_enb_association->nb_mbms_associated;
//	  message_p = itti_alloc_new_message (TASK_S1AP, M3AP_ENB_INITIATED_RESET_REQ);
//	  arg.message_p = message_p;
//	  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
//	  M3AP_ENB_INITIATED_RESET_REQ (message_p).num_mbms = num_mbms_assoc;
//	  /**
//	   * Allocate a new list and let the callback function fill it.
//	   * Generating full reset request.
//	   */
//	  M3AP_ENB_INITIATED_RESET_REQ (message_p).mbms_to_reset_list = (m2_sig_conn_id_t*) calloc (m2ap_enb_association->nb_mbms_associated, sizeof (m2_sig_conn_id_t));
//	  hashtable_ts_apply_callback_on_elements(&m2ap_enb_association->ue_coll, construct_m2ap_mme_full_reset_req, (void*)&arg, (void**) &message_p);
  }
//  M3AP_ENB_INITIATED_RESET_REQ (message_p).m2ap_reset_type = m2ap_reset_type;
//  M3AP_ENB_INITIATED_RESET_REQ (message_p).m2ap_enb_id = m2ap_enb_association->m2ap_enb_id;
//  M3AP_ENB_INITIATED_RESET_REQ (message_p).sctp_assoc_id = assoc_id;
//  M3AP_ENB_INITIATED_RESET_REQ (message_p).sctp_stream_id = stream;
//  DevAssert(M3AP_ENB_INITIATED_RESET_REQ (message_p).mbms_to_reset_list != NULL);
  rc =  itti_send_msg_to_task (TASK_MCE_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_service_counting_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu) {
  M2AP_MbmsServiceCountingResponse_t	 	*container;
  M2AP_MbmsServiceCountingResponse_Ies_t 	*ie = NULL;
  mbms_description_t               		 	*mbms_ref_p = NULL;
  MessageDef                      		 	*message_p = NULL;
  int                                	  	 rc = RETURNok;
  mce_mbms_m2ap_id_t                 	  	 mce_mbms_m2ap_id = 0;
  enb_mbms_m2ap_id_t                 	  	 enb_mbms_m2ap_id = 0;
  OAILOG_FUNC_IN (LOG_MXAP);

  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_service_counting_failure (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu) {
  M2AP_MbmsServiceCountingFailure_t		 	*container;
  M2AP_MbmsServiceCountingFailure_Ies_t 	*ie = NULL;
  mbms_description_t               		 	*mbms_ref_p = NULL;
  MessageDef                      		 	*message_p = NULL;
  int                                	  	 rc = RETURNok;
  mce_mbms_m2ap_id_t                 	  	 mce_mbms_m2ap_id = 0;
  enb_mbms_m2ap_id_t                 	  	 enb_mbms_m2ap_id = 0;
  OAILOG_FUNC_IN (LOG_MXAP);


  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}


//------------------------------------------------------------------------------
int
m2ap_mce_handle_error_ind_message (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu)
{
  enb_mbms_m2ap_id_t                        enb_mbms_m2ap_id = 0;
  mce_mbms_m2ap_id_t                        mce_mbms_m2ap_id = 0;
  M2AP_ErrorIndication_t             	   *container = NULL;
  M2AP_ErrorIndication_Ies_t 			   *ie = NULL;
  MessageDef                               *message_p = NULL;
  long                                      cause_value;

  OAILOG_FUNC_IN (LOG_MXAP);

  container = &pdu->choice.initiatingMessage.value.choice.ErrorIndication;

//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_ErrorIndicationIEs_t, ie, container, M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID, false);
//  if(ie)
//	mce_mbms_m2ap_id = ie->value.choice.MCE_MBMS_M2AP_ID;
//
//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_ErrorIndicationIEs_t, ie, container, M2AP_ProtocolIE_ID_id_eNB_MBMS_M2AP_ID, false);
//  // eNB MBMS M2AP ID is limited to 24 bits
//  if(ie)
//    enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t) (ie->value.choice.ENB_MBMS_M2AP_ID & ENB_MBMS_M2AP_ID_MASK);
//
//  m2ap_enb_description_t * enb_ref = m2ap_is_enb_assoc_id_in_list (assoc_id);
//  DevAssert(enb_ref);
//
//  mbms_description_t * mbms_ref = m2ap_is_ue_enb_id_in_list (enb_ref, enb_mbms_m2ap_id);
//  /** If no MBMS reference exists, drop the message. */
//  if(!mbms_ref){
//    /** Check that the MBMS reference exists. */
//    OAILOG_WARNING(LOG_MXAP, "No MBMS reference exists for enbId %d and enb_mbms_m2ap_id " ENB_MBMS_M2AP_ID_FMT ". Dropping error indication. \n", enb_ref->m2ap_enb_id, enb_mbms_m2ap_id);
//    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//  }
//
//  /*
//   * Handle error indication.
//   * Just forward to the MCE_APP layer, depending if there is a handover procedure running or not the current handover procedure might be disregarded.
//   */
//  message_p = itti_alloc_new_message (TASK_S1AP, M2AP_ERROR_INDICATION);
//  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
//  memset ((void *)&message_p->ittiMsg.m2ap_error_indication, 0, sizeof (itti_m2ap_error_indication_t));
//  M2AP_ERROR_INDICATION (message_p).mce_mbms_m2ap_id = mce_mbms_m2ap_id;
//  M2AP_ERROR_INDICATION (message_p).enb_mbms_m2ap_id = enb_mbms_m2ap_id;
//  M2AP_ERROR_INDICATION (message_p).m2ap_enb_id = enb_ref->m2ap_enb_id;
//
//  /** Choice. */
//  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_ErrorIndicationIEs_t, ie, container, M2AP_ProtocolIE_ID_id_Cause, false);
//
//  if(ie){
//	  // todo: unused yet..
//	  M2AP_Cause_PR cause_type = ie->value.choice.Cause.present;
//
//	  switch (cause_type)
//	  {
//	  case M2AP_Cause_PR_radioNetwork:
//	    cause_value = ie->value.choice.Cause.choice.radioNetwork;
//	    OAILOG_DEBUG (LOG_MXAP, "M2AP_ERROR_INDICATION with Cause_Type = Radio Network and Cause_Value = %ld\n", cause_value);
//	    break;
//
//	  case M2AP_Cause_PR_transport:
//	    cause_value = ie->value.choice.Cause.choice.transport;
//	    OAILOG_DEBUG (LOG_MXAP, "M2AP_ERROR_INDICATION with Cause_Type = Transport and Cause_Value = %ld\n", cause_value);
//	    break;
//
//	  case M2AP_Cause_PR_nas:
//	    cause_value = ie->value.choice.Cause.choice.nas;
//	    OAILOG_DEBUG (LOG_MXAP, "M2AP_ERROR_INDICATION with Cause_Type = NAS and Cause_Value = %ld\n", cause_value);
//	    break;
//
//	  case M2AP_Cause_PR_protocol:
//	    cause_value = ie->value.choice.Cause.choice.protocol;
//	    OAILOG_DEBUG (LOG_MXAP, "M2AP_ERROR_INDICATION with Cause_Type = Protocol and Cause_Value = %ld\n", cause_value);
//	    break;
//
//	  case M2AP_Cause_PR_misc:
//	    cause_value = ie->value.choice.Cause.choice.misc;
//	    OAILOG_DEBUG (LOG_MXAP, "M2AP_ERROR_INDICATION with Cause_Type = MISC and Cause_Value = %ld\n", cause_value);
//	    break;
//
//	  default:
//	    OAILOG_ERROR (LOG_MXAP, "M2AP_ERROR_INDICATION with Invalid Cause_Type = %d\n", cause_type);
//	    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//	  }
//
//  }
//
//  /**
//   * No match function exists between the M2AP Cause and the decoded cause yet.
//   * Set it to anything else thatn SYSTEM_FAILURE, not to trigger any implicit detach due cause (only based on the handover type).
//   */
//  M2AP_ERROR_INDICATION (message_p).cause = M2AP_TODODODO_FAILED;

  itti_send_msg_to_task (TASK_MCE_APP, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_RETURN (LOG_MXAP, RETURNok);
}


//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_service_counting_result_report (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu) {
  M2AP_MbmsServiceCountingResultsReport_t		*container;
  M2AP_MbmsServiceCountingResultsReport_Ies_t 	*ie = NULL;
  MessageDef                      		 		*message_p = NULL;
  int                                	  	 	 rc = RETURNok;
  OAILOG_FUNC_IN (LOG_MXAP);

  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_overload_notification (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu) {
  M2AP_MbmsOverloadNotification_t				*container;
  M2AP_MbmsOverloadNotification_Ies_t 			*ie = NULL;
  MessageDef                      		 		*message_p = NULL;
  int                                	  	 	 rc = RETURNok;
  OAILOG_FUNC_IN (LOG_MXAP);

  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}

//------------------------------------------------------------------------------
typedef struct arg_m2ap_send_enb_dereg_ind_s {
  uint         current_mbms_index;
  uint         handled_mbmss;
  MessageDef  *message_p;
}arg_m2ap_send_enb_dereg_ind_t;

// TODO: WTF?!?
////------------------------------------------------------------------------------
//static bool m2ap_send_enb_deregistered_ind (
//    __attribute__((unused)) const hash_key_t keyP,
//    void * const dataP,
//    void *argP,
//    void ** resultP) {
//
//  arg_m2ap_send_enb_dereg_ind_t          *arg = (arg_m2ap_send_enb_dereg_ind_t*) argP;
//  mbms_description_t                       *mbms_ref_p = (mbms_description_t*)dataP;
// /*
//   * Ask for a release of each MBMS context associated to the eNB
//   */
//  if (mbms_ref_p) {
//    if (arg->current_ue_index == 0) {
//      arg->message_p = itti_alloc_new_message (TASK_S1AP, M2AP_ENB_DEREGISTERED_IND);
//    }
//    OAILOG_INFO ( LOG_MXAP, "Triggering eNB deregistration indication for MBMS with enbId " ENB_MBMS_M2AP_ID_FMT ". \n", mbms_ref_p->enb_mbms_m2ap_id);
//
//    AssertFatal(arg->current_ue_index < M2AP_ITTI_MBMS_PER_DEREGISTER_MESSAGE, "Too many deregistered MBMSs reported in M2AP_ENB_DEREGISTERED_IND message ");
//    M2AP_ENB_DEREGISTERED_IND (arg->message_p).mce_mbms_m2ap_id[arg->current_ue_index] = mbms_ref_p->mce_mbms_m2ap_id;
//    M2AP_ENB_DEREGISTERED_IND (arg->message_p).enb_mbms_m2ap_id[arg->current_ue_index] = mbms_ref_p->enb_mbms_m2ap_id;
//
//    // max ues reached
//    if (arg->current_ue_index == 0 && arg->handled_ues > 0) {
//      M2AP_ENB_DEREGISTERED_IND (arg->message_p).nb_mbms_to_deregister = M2AP_ITTI_MBMS_PER_DEREGISTER_MESSAGE;
//      itti_send_msg_to_task (TASK_MCE_APP, INSTANCE_DEFAULT, arg->message_p);
//      MSC_LOG_TX_MESSAGE (MSC_M2AP_MME, MSC_NAS_MME, NULL, 0, "0 M2AP_ENB_DEREGISTERED_IND num ue to deregister %u",
//          M2AP_ENB_DEREGISTERED_IND (arg->message_p).nb_mbms_to_deregister);
//      arg->message_p = NULL;
//    }
//
//    arg->handled_ues++;
//    arg->current_ue_index = arg->handled_ues % M2AP_ITTI_MBMS_PER_DEREGISTER_MESSAGE;
//    *resultP = arg->message_p;
//
//  } else {
//    OAILOG_TRACE (LOG_MXAP, "No valid MBMS provided in callback: %p\n", mbms_ref_p);
//  }
//  return false;
//}

//------------------------------------------------------------------------------
static bool construct_m2ap_mce_full_reset_req (
    __attribute__((unused)) const hash_key_t keyP,
    void * const dataP,
    void *argP,
    void ** resultP)
{
//  arg_m2ap_construct_enb_reset_req_t          *arg = (arg_m2ap_construct_enb_reset_req_t*) argP;
//  mbms_bearer_context_t                       *mbms_ref_p = (mbms_bearer_context_t*)dataP;
//  uint32_t i = arg->current_mbms_index;
//  if (mbms_ref_p) {
//    M3AP_ENB_INITIATED_RESET_REQ (arg->message_p).mbms_to_reset_list[i].mce_mbms_m2ap_id = &(mbms_ref_p->mce_mbms_m2ap_id);
//    M3AP_ENB_INITIATED_RESET_REQ (arg->message_p).mbms_to_reset_list[i].enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t*)mbms_ref_p; /**< Should be the address of the id. */
//    arg->current_mbms_index++;
//    *resultP = arg->message_p;
//  } else {
//    OAILOG_TRACE (LOG_MXAP, "No valid MBMS provided in callback: %p\n", mbms_ref_p);
//    M3AP_ENB_INITIATED_RESET_REQ (arg->message_p).mbms_to_reset_list[i].mce_mbms_m2ap_id = NULL;
//  }
  return false;
}

//------------------------------------------------------------------------------
int
m2ap_handle_sctp_disconnection (
    const sctp_assoc_id_t assoc_id, bool reset)
{
  arg_m2ap_send_enb_dereg_ind_t           arg = {0};
  int                                     i = 0;
  MessageDef                             *message_p = NULL;
  m2ap_enb_description_t                 *m2ap_enb_association = NULL;

  OAILOG_FUNC_IN (LOG_MXAP);
  /*
   * Checking that the assoc id has a valid eNB attached to.
   */
  m2ap_enb_association = m2ap_is_enb_assoc_id_in_list (assoc_id);

//  if (m2ap_enb_association == NULL) {
//    OAILOG_ERROR (LOG_MXAP, "No eNB attached to this assoc_id: %d\n", assoc_id);
//    OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
//  }
//
//  // First check if we can just reset the eNB state if there are no MBMSs.
//  if (!m2ap_enb_association->nb_mbms_service_associated) {
//    if (reset) {
//      m2ap_enb_association->m2_state = M2AP_INIT;
//      OAILOG_INFO(LOG_MXAP, "Moving eNB with association id %u to INIT state\n", assoc_id);
//      update_mce_app_stats_connected_enb_sub();
//    } else {
//      hashtable_ts_free (&g_m2ap_enb_coll, m2ap_enb_association->sctp_assoc_id);
//      update_mce_app_stats_connected_enb_sub();
//      OAILOG_INFO(LOG_MXAP, "Removing eNB with association id %u \n", assoc_id);
//    }
//    OAILOG_FUNC_RETURN(LOG_MXAP, RETURNok);
//  }
//
//  m2ap_enb_association->m2_state = M2AP_SHUTDOWN;
//  hashtable_ts_apply_callback_on_elements(&m2ap_enb_association->mbms_coll, m2ap_send_enb_deregistered_ind, (void*)&arg, (void**)&message_p); /**< Just releasing the procedure. */
//
//  /** Release the M2AP MBMS references implicitly. */
//  OAILOG_DEBUG (LOG_MXAP, "Releasing all M2AP MBMS references related to the ENB with assocId %d. \n", assoc_id);
//
////  if ( (!(arg.handled_ues % M2AP_ITTI_MBMS_PER_DEREGISTER_MESSAGE)) && (arg.handled_ues) ){
//    M2AP_ENB_DEREGISTERED_IND (message_p).nb_mbms_to_deregister = arg.current_mbms_index;
//
//    for (i = arg.current_mbms_index; i < M2AP_ITTI_MBMS_PER_DEREGISTER_MESSAGE; i++) {
//      M2AP_ENB_DEREGISTERED_IND (message_p).mce_mbms_m2ap_id[arg.current_mbms_index] = 0;
//      M2AP_ENB_DEREGISTERED_IND (message_p).enb_mbms_m2ap_id[arg.current_mbms_index] = 0;
//    }
//    itti_send_msg_to_task (TASK_MCE_APP, INSTANCE_DEFAULT, message_p);
//    message_p = NULL;
////  }

  /** The eNB will be removed, when the MBMS references are removed. */

  m2ap_dump_enb_list ();
  OAILOG_DEBUG (LOG_MXAP, "Removed eNB attached to assoc_id: %d\n", assoc_id);
  OAILOG_FUNC_RETURN (LOG_MXAP, RETURNok);
}

//------------------------------------------------------------------------------
int
m2ap_handle_new_association (
  sctp_new_peer_t * sctp_new_peer_p)
{
  m2ap_enb_description_t                      *m2ap_enb_association = NULL;

  OAILOG_FUNC_IN (LOG_MXAP);
  DevAssert (sctp_new_peer_p != NULL);

  /*
   * Checking that the assoc id has a valid eNB attached to.
   */
  if ((m2ap_enb_association = m2ap_is_enb_assoc_id_in_list (sctp_new_peer_p->assoc_id)) == NULL) {
    OAILOG_DEBUG (LOG_MXAP, "Create eNB context for assoc_id: %d\n", sctp_new_peer_p->assoc_id);
    /*
     * Create new context
     */
    m2ap_enb_association = m2ap_new_enb ();

    if (m2ap_enb_association == NULL) {
      /*
       * We failed to allocate memory
       */
      /*
       * TODO: send reject there
       */
      OAILOG_ERROR (LOG_MXAP, "Failed to allocate eNB context for assoc_id: %d\n", sctp_new_peer_p->assoc_id);
    }
    m2ap_enb_association->sctp_assoc_id = sctp_new_peer_p->assoc_id;
    hashtable_rc_t  hash_rc = hashtable_ts_insert (&g_m2ap_enb_coll, (const hash_key_t)m2ap_enb_association->sctp_assoc_id, (void *)m2ap_enb_association);
    if (HASH_TABLE_OK != hash_rc) {
      OAILOG_FUNC_RETURN (LOG_MXAP, RETURNerror);
    }
  } else if ((m2ap_enb_association->m2_state == M2AP_SHUTDOWN) || (m2ap_enb_association->m2_state == M2AP_RESETING)) {
    OAILOG_WARNING(LOG_MXAP, "Received new association request on an association that is being %s, ignoring",
                   m2_enb_state_str[m2ap_enb_association->m2_state]);
    OAILOG_FUNC_RETURN(LOG_MXAP, RETURNerror);
  } else {
    OAILOG_DEBUG (LOG_MXAP, "eNB context already exists for assoc_id: %d, update it\n", sctp_new_peer_p->assoc_id);
  }

  m2ap_enb_association->sctp_assoc_id = sctp_new_peer_p->assoc_id;
  /*
   * Fill in in and out number of streams available on SCTP connection.
   */
  m2ap_enb_association->instreams = sctp_new_peer_p->instreams;
  m2ap_enb_association->outstreams = sctp_new_peer_p->outstreams;
  /*
   * initialize the next sctp stream to 1 as 0 is reserved for non
   * * * * ue associated signalling.
   */
  m2ap_enb_association->next_sctp_stream = 1;
  m2ap_enb_association->m2_state = M2AP_INIT;
  OAILOG_FUNC_RETURN (LOG_MXAP, RETURNok);
}

////------------------------------------------------------------------------------
//void
//m2ap_mme_handle_ue_context_rel_comp_timer_expiry (void *arg)
//{
//  MessageDef                             *message_p = NULL;
//  OAILOG_FUNC_IN (LOG_MXAP);
//  DevAssert (arg != NULL);
//
//  mbms_description_t *mbms_ref_p  =        (mbms_description_t *)arg;
//
//  mbms_ref_p->m2ap_ue_context_rel_timer.id = M2AP_TIMER_INACTIVE_ID;
//  OAILOG_DEBUG (LOG_MXAP, "Expired- MBMS Context Release Timer for MBMS id  %d \n", mbms_ref_p->mce_mbms_m2ap_id);
//  /*
//   * Remove MBMS context and inform MCE_APP.
//   */
//  message_p = itti_alloc_new_message (TASK_S1AP, M2AP_MBMS_CONTEXT_RELEASE_COMPLETE);
//  AssertFatal (message_p != NULL, "itti_alloc_new_message Failed");
//  memset ((void *)&message_p->ittiMsg.m2ap_ue_context_release_complete, 0, sizeof (itti_m2ap_ue_context_release_complete_t));
//  M2AP_MBMS_CONTEXT_RELEASE_COMPLETE (message_p).mce_mbms_m2ap_id = mbms_ref_p->mce_mbms_m2ap_id;
//  MSC_LOG_TX_MESSAGE (MSC_M2AP_MME, MSC_MMEAPP_MME, NULL, 0, "0 M2AP_MBMS_CONTEXT_RELEASE_COMPLETE mce_mbms_m2ap_id " MCE_MBMS_M2AP_ID_FMT " ", M2AP_MBMS_CONTEXT_RELEASE_COMPLETE (message_p).mce_mbms_m2ap_id);
//  itti_send_msg_to_task (TASK_MCE_APP, INSTANCE_DEFAULT, message_p);
//  DevAssert(mbms_ref_p->s1_ue_state == M2AP_MBMS_WAITING_CRR);
//  OAILOG_DEBUG (LOG_MXAP, "Removed M2AP MBMS " MCE_MBMS_M2AP_ID_FMT "\n", (uint32_t) mbms_ref_p->mce_mbms_m2ap_id);
//  m2ap_remove_ue (mbms_ref_p);
//  OAILOG_FUNC_OUT (LOG_MXAP);
//}

//------------------------------------------------------------------------------
int
m3ap_handle_enb_initiated_reset_ack (
  const itti_m3ap_enb_initiated_reset_ack_t * const enb_reset_ack_p)
{
  uint8_t                                *buffer = NULL;
  uint32_t                                length = 0;
  M2AP_M2AP_PDU_t                         pdu;
  /** Reset Acknowledgment. */
  M2AP_ResetAcknowledge_t				 *out;
  M2AP_ResetAcknowledge_Ies_t            *ie = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MXAP);

  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = M2AP_ProcedureCode_id_reset;
  pdu.choice.successfulOutcome.criticality = M2AP_Criticality_ignore;
  pdu.choice.successfulOutcome.value.present = M2AP_SuccessfulOutcome__value_PR_ResetAcknowledge;
  out = &pdu.choice.successfulOutcome.value.choice.ResetAcknowledge;

//  if (enb_reset_ack_p->m2ap_reset_type == M2AP_RESET_PARTIAL) {
//    DevAssert(enb_reset_ack_p->num_ue > 0);
//    /** Conn Item .*/
//    ie = (M2AP_ResetAcknowledgeIEs_t *)calloc(1, sizeof(M2AP_ResetAcknowledgeIEs_t));
//    ie->id = M2AP_ProtocolIE_ID_id_MBMS_associatedLogicalS1_ConnectionListResAck;
//    ie->criticality = M2AP_Criticality_ignore;
//    ie->value.present = M2AP_ResetAcknowledgeIEs__value_PR_MBMS_associatedLogicalS1_ConnectionListResAck;
//    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//	/** MCE MBMS M2AP ID. */
//    M2AP_MBMS_associatedLogicalS1_ConnectionListResAck_t * ie_p = &ie->value.choice.UE_associatedLogicalS1_ConnectionListResAck;
//    /** Allocate an item for each. */
//    for (uint32_t i = 0; i < enb_reset_ack_p->num_ue; i++) {
//    	/** MCE MBMS. */
//    	M2AP_MBMS_associatedLogicalS1_ConnectionItemResAck_t * sig_conn_item = calloc (1, sizeof (M2AP_MBMS_associatedLogicalS1_ConnectionItemResAck_t));
//    	sig_conn_item->id = M2AP_ProtocolIE_ID_id_MBMS_associatedLogicalS1_ConnectionItem;
//    	sig_conn_item->criticality = M2AP_Criticality_ignore;
//    	sig_conn_item->value.present = M2AP_MBMS_associatedLogicalS1_ConnectionItemResAck__value_PR_MBMS_associatedLogicalS1_ConnectionItem;
//    	M2AP_MBMS_associatedLogicalS1_ConnectionItem_t * item = &sig_conn_item->value.choice.UE_associatedLogicalS1_ConnectionItem;
//
//    	if (enb_reset_ack_p->mbms_to_reset_list[i].mce_mbms_m2ap_id != NULL) {
//    		item->mCE_MBMS_M2AP_ID = calloc(1, sizeof(M2AP_MCE_MBMS_M2AP_ID_t));
//    		*item->mCE_MBMS_M2AP_ID = *enb_reset_ack_p->mbms_to_reset_list[i].mce_mbms_m2ap_id;
//    	}
//    	else {
//    		item->mCE_MBMS_M2AP_ID = NULL;
//    	}
    	/** ENB MBMS M2AP ID. */
//    	if (enb_reset_ack_p->mbms_to_reset_list[i].enb_mbms_m2ap_id != NULL) {
//    		item->eNB_MBMS_M2AP_ID = calloc(1, sizeof(M2AP_ENB_MBMS_M2AP_ID_t));
//    		*item->eNB_MBMS_M2AP_ID = *enb_reset_ack_p->mbms_to_reset_list[i].enb_mbms_m2ap_id;
//    	}
//    	else {
//    		item->eNB_MBMS_M2AP_ID = NULL;
//    	}
//        ASN_SEQUENCE_ADD(&ie_p->list, sig_conn_item);
//    }
//  }

  if (m2ap_mce_encode_pdu (&pdu, &buffer, &length) < 0) {
	OAILOG_ERROR (LOG_MXAP, "Failed to M2 Reset command \n");
	/** We rely on the handover_notify timeout to remove the MBMS context. */
	DevAssert(!buffer);
	OAILOG_FUNC_OUT (LOG_MXAP);
  }

  if(buffer) {
	  bstring b = blk2bstr(buffer, length);
	  free(buffer);
	  rc = m2ap_mce_itti_send_sctp_request (&b, enb_reset_ack_p->sctp_assoc_id, enb_reset_ack_p->sctp_stream_id, INVALID_MCE_MBMS_M2AP_ID);
  }
  OAILOG_FUNC_RETURN (LOG_MXAP, rc);
}

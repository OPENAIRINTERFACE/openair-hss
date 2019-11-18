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


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "3gpp_requirements_36.443.h"
#include "m2ap_common.h"
#include "m2ap_mce_encoder.h"
#include "m2ap_mce_mbms_sa.h"
#include "m2ap_mce_handlers.h"
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
#include "m2ap_mce.h"
#include "m2ap_mce.h"
#include "m2ap_mce_itti_messaging.h"
#include "m2ap_mce_procedures.h"

extern hash_table_ts_t 					g_m2ap_enb_coll; 	// SCTP Association ID association to M2AP eNB Reference;
extern hash_table_ts_t 					g_m2ap_mbms_coll; 	// MCE MBMS M2AP ID association to MBMS Reference;

static const char * const m2_enb_state_str [] = {"M2AP_INIT", "M2AP_RESETTING", "M2AP_READY", "M2AP_SHUTDOWN"};

static int m2ap_generate_m2_setup_response (m2ap_enb_description_t * m2ap_enb_association, mbsfn_areas_t * mbsfn_areas);

//static int                              m2ap_mme_generate_ue_context_release_command (
//    mbms_description_t * mbms_ref_p, mce_mbms_m2ap_id_t mce_mbms_m2ap_id, enum s1cause, m2ap_enb_description_t* enb_ref_p);

/* Handlers matrix. Only mme related procedures present here.
*/
m2ap_message_decoded_callback           m2ap_messages_callback[][3] = {
  {0, m2ap_mce_handle_mbms_session_start_response,					/* sessionStart */
		  m2ap_mce_handle_mbms_session_start_failure},
  {0, m2ap_mce_handle_mbms_session_stop_response, 0},      	/* sessionStop */
  {0, m2ap_mce_handle_mbms_scheduling_information_response, 0},    /* mbmsSchedulingInformation */
  {m2ap_mce_handle_error_indication, 0, 0},     				   /* errorIndication */
  {m2ap_mce_handle_reset_request, 0, 0},                    	   /* reset */
  {m2ap_mce_handle_m2_setup_request, 0, 0},     				   /* m2Setup */
  {0, 0, 0},                    								   /* enbConfigurationUpdate */
  {0, 0, 0},                   									   /* mceConfigurationUpdate */
  {0, 0, 0},                    								   /* privateMessage */
  {0, m2ap_mce_handle_mbms_session_update_response,				   /* sessionUpdate */
		  m2ap_mce_handle_mbms_session_update_failure},
  {0, m2ap_mce_handle_mbms_service_counting_response, 			   /* mbmsServiceCounting */
		  m2ap_mce_handle_mbms_service_counting_failure},
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
  if (pdu->choice.initiatingMessage.procedureCode >= sizeof(m2ap_messages_callback) / (3 * sizeof(m2ap_message_decoded_callback))
      || (pdu->present > M2AP_M2AP_PDU_PR_unsuccessfulOutcome)) {
    OAILOG_DEBUG (LOG_M2AP, "[SCTP %d] Either procedureCode %ld or direction %d exceed expected\n",
               assoc_id, pdu->choice.initiatingMessage.procedureCode, pdu->present);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_M2AP_M2AP_PDU, pdu);
    return -1;
  }

  /* No handler present.
   * This can mean not implemented or no procedure for eNB (wrong direction).
   */
  if (m2ap_messages_callback[pdu->choice.initiatingMessage.procedureCode][pdu->present - 1] == NULL) {
    OAILOG_DEBUG (LOG_M2AP, "[SCTP %d] No handler for procedureCode %ld in %s\n",
               assoc_id, pdu->choice.initiatingMessage.procedureCode,
               m2ap_direction2String[pdu->present]);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_M2AP_M2AP_PDU, pdu);
    return -1;
  }

  /* Calling the right handler */
  int ret = (*m2ap_messages_callback[pdu->choice.initiatingMessage.procedureCode][pdu->present - 1])(assoc_id, stream, pdu);
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
    const sctp_stream_id_t stream,
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

  OAILOG_FUNC_IN (LOG_M2AP);

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

  if (m2ap_mce_encode_pdu (&pdu, &buffer_p, &length) < 0) {
    OAILOG_ERROR (LOG_M2AP, "Failed to encode m2 setup failure\n");
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }

  bstring b = blk2bstr(buffer_p, length);
  free(buffer_p);
  rc =  m2ap_mce_itti_send_sctp_request (&b, assoc_id, stream, INVALID_MCE_MBMS_M2AP_ID);
  OAILOG_FUNC_RETURN (LOG_M2AP, rc);
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
  OAILOG_FUNC_IN (LOG_M2AP);
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
   * * * * This should not happen -> reject M2AP eNB m2 setup request.
   */
  if (stream != 0) {
    OAILOG_ERROR (LOG_M2AP, "Received new m2 setup request on stream != 0\n");
    /*
     * Send a m2 setup failure with protocol cause unspecified
     */
    rc = m2ap_mce_generate_m2_setup_failure (assoc_id, stream, M2AP_Cause_PR_protocol, M2AP_CauseProtocol_unspecified, -1);
    OAILOG_FUNC_RETURN (LOG_M2AP, rc);
  }

  mme_config_read_lock (&mme_config);
  max_m2_enb_connected = mme_config.mbms.max_m2_enbs;
  mme_config_unlock (&mme_config);

  if (nb_enb_associated == max_m2_enb_connected) {
    OAILOG_ERROR (LOG_M2AP, "There is too much M2 eNB connected to MME, rejecting the association\n");
    OAILOG_DEBUG (LOG_M2AP, "Connected = %d, maximum allowed = %d\n", nb_enb_associated, max_m2_enb_connected);
      /*
       * Send an overload cause...
       */
    rc = m2ap_mce_generate_m2_setup_failure (assoc_id, stream, M2AP_Cause_PR_misc, M2AP_CauseMisc_control_processing_overload, M2AP_TimeToWait_v20s);
    OAILOG_FUNC_RETURN (LOG_M2AP, rc);
  }

  shared_log_queue_item_t *  context = NULL;
  OAILOG_MESSAGE_START (OAILOG_LEVEL_DEBUG, LOG_M2AP, (&context), "New m2 setup request incoming from ");

  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_M2SetupRequest_IEs_t, ie_enb_name, container,
		  M2AP_ProtocolIE_ID_id_ENBname, false);
  if (ie) {
    OAILOG_MESSAGE_ADD (context, "%*s ", (int)ie_enb_name->value.choice.ENBname.size, ie_enb_name->value.choice.ENBname.buf);
    m2ap_enb_name = (char *) ie_enb_name->value.choice.ENBname.buf;
  }

  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_M2SetupRequest_IEs_t, ie, container, M2AP_ProtocolIE_ID_id_GlobalENB_ID, true);
  // TODO make sure ie != NULL

  if (ie->value.choice.GlobalENB_ID.eNB_ID.present == M2AP_ENB_ID_PR_short_Macro_eNB_ID) {
    uint8_t *enb_id_buf = ie->value.choice.GlobalENB_ID.eNB_ID.choice.short_Macro_eNB_ID.buf;
    if (ie->value.choice.GlobalENB_ID.eNB_ID.choice.short_Macro_eNB_ID.size != 18) {
      //TODO: handle case were size != 18 -> notify ? reject ?
      OAILOG_DEBUG (LOG_S1AP, "M2-Setup-Request shortENB_ID.size %lu (should be 18)\n", ie->value.choice.GlobalENB_ID.eNB_ID.choice.short_Macro_eNB_ID.size);
    }
    m2ap_enb_id = (enb_id_buf[0] << 10) + (enb_id_buf[1] << 2) + ((enb_id_buf[2] & 0xc0) >> 6);
    OAILOG_MESSAGE_ADD (context, "short eNB id: %07x", m2ap_enb_id);
  } else if (ie->value.choice.GlobalENB_ID.eNB_ID.present == M2AP_ENB_ID_PR_long_Macro_eNB_ID) {
	uint8_t *enb_id_buf = ie->value.choice.GlobalENB_ID.eNB_ID.choice.long_Macro_eNB_ID.buf;
	if (ie->value.choice.GlobalENB_ID.eNB_ID.choice.long_Macro_eNB_ID.size != 22) {
	  //TODO: handle case were size != 21 -> notify ? reject ?
	  OAILOG_DEBUG (LOG_S1AP, "M2-Setup-Request longENB_ID.size %lu (should be 21)\n", ie->value.choice.GlobalENB_ID.eNB_ID.choice.long_Macro_eNB_ID.size);
	}
	m2ap_enb_id = (enb_id_buf[0] << 13) + (enb_id_buf[1] << 5) + ((enb_id_buf[2] & 0xf8) >> 3);
	OAILOG_MESSAGE_ADD (context, "long eNB id: %07x", m2ap_enb_id);
  } else {
	// Macro eNB = 20 bits
	uint8_t                                *enb_id_buf = ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf;
	if (ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.size != 20) {
	  //TODO: handle case were size != 20 -> notify ? reject ?
	  OAILOG_DEBUG (LOG_M2AP, "M2-Setup-Request macroENB_ID.size %lu (should be 20)\n", ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.size);
	}
	m2ap_enb_id = (enb_id_buf[0] << 12) + (enb_id_buf[1] << 4) + ((enb_id_buf[2] & 0xf0) >> 4);
	OAILOG_MESSAGE_ADD (context, "macro eNB id: %05x", m2ap_enb_id);
  }
  OAILOG_MESSAGE_FINISH(context);
  global_enb_id = &ie->value.choice.GlobalENB_ID;

  /**
   * Check the MCE PLMN.
   */
  if(m2ap_mce_combare_mbms_plmn(&global_enb_id->pLMN_Identity) != MBMS_SA_LIST_RET_OK){
	OAILOG_ERROR (LOG_M2AP, "No Common PLMN with M2AP eNB, generate_m2_setup_failure\n");
	rc =  m2ap_mce_generate_m2_setup_failure (assoc_id, stream, M2AP_Cause_PR_misc, M2AP_CauseMisc_unspecified, M2AP_TimeToWait_v20s);
	OAILOG_FUNC_RETURN (LOG_M2AP, rc);
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
    OAILOG_ERROR(LOG_M2AP, "More than 1 cell is not supported for the eNB MBMS. \n");
	rc =  m2ap_mce_generate_m2_setup_failure (assoc_id, stream, M2AP_Cause_PR_misc, M2AP_CauseMisc_unspecified, M2AP_TimeToWait_v20s);
	OAILOG_FUNC_RETURN (LOG_M2AP, rc);
  }
  M2AP_ENB_MBMS_Configuration_data_Item_t 	* m2ap_enb_mbms_cfg_item = enb_mbms_cfg_data_list->list.array[0];
  DevAssert(m2ap_enb_mbms_cfg_item);
  mbms_sa_ret = m2ap_mce_compare_mbms_enb_configuration_item(m2ap_enb_mbms_cfg_item);

  /*
   * eNB (first cell) and MCE have no common MBMS SA
   */
  if (mbms_sa_ret != MBMS_SA_LIST_RET_OK) {
    OAILOG_ERROR (LOG_M2AP, "No Common PLMN/MBMS_SA with M2AP eNB, generate_m2_setup_failure\n");
    rc = m2ap_mce_generate_m2_setup_failure(assoc_id, stream,
    		M2AP_Cause_PR_misc,
			M2AP_CauseMisc_unspecified,
			M2AP_TimeToWait_v20s);
    // todo: test leaks..
    OAILOG_FUNC_RETURN (LOG_M2AP, rc);
  }

  OAILOG_DEBUG (LOG_M2AP, "Adding M2AP eNB with eNB-ID %d to the list of served MBMS eNBs. \n", m2ap_enb_id);
  if ((m2ap_enb_association = m2ap_is_enb_id_in_list (m2ap_enb_id)) == NULL) {
    /*
     * eNB has not been fount in list of associated eNB,
     * * * * Add it to the tail of list and initialize data
     */
    if ((m2ap_enb_association = m2ap_is_enb_assoc_id_in_list (assoc_id)) == NULL) {
      OAILOG_ERROR(LOG_M2AP, "Ignoring m2 setup from unknown assoc %u and enbId %u", assoc_id, m2ap_enb_id);
      OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
    } else {
      m2ap_enb_association->m2_state = M2AP_RESETING;
      OAILOG_DEBUG (LOG_M2AP, "Adding M2AP eNB id %u to the list of served eNBs\n", m2ap_enb_id);
      m2ap_enb_association->m2ap_enb_id = m2ap_enb_id;
      /**
       * Set the configuration item of the first cell, which is a single MBSFN area.
       * Set all MBMS Service Area Ids. */
      m2ap_enb_association->mbsfn_synch_area_id = m2ap_enb_mbms_cfg_item->mbsfnSynchronisationArea;
      m2ap_set_embms_cfg_item(m2ap_enb_association, &m2ap_enb_mbms_cfg_item->mbmsServiceAreaList);
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
			OAILOG_ERROR (LOG_M2AP, "Rejecting m2 setup request as eNB id %d is already associated to an active sctp association" "Previous known: %d, new one: %d\n",
			m2ap_enb_id, m2ap_enb_association->sctp_assoc_id, assoc_id);
			rc = m2ap_mce_generate_m2_setup_failure (assoc_id, stream, M2AP_Cause_PR_misc, M2AP_CauseMisc_unspecified, -1); /**< eNB should attach again. */
			/** Also remove the old eNB. */
			OAILOG_INFO(LOG_M2AP, "Rejecting the old M2AP eNB connection for eNB id %d and old assoc_id: %d\n", m2ap_enb_id, assoc_id);
			m2ap_dump_enb_list();
			m2ap_handle_sctp_disconnection(m2ap_enb_association->sctp_assoc_id);
			OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
		}

		OAILOG_INFO(LOG_M2AP, "We found the M2AP eNB id %d in the current list of enbs with matching sctp associations:" "assoc: %d\n", m2ap_enb_id, m2ap_enb_association->sctp_assoc_id);
		/*
		 * TODO: call the reset procedure
		 * todo: recheck how it affects the MBSFN areas..
		 */
		/** Forward this to the MCE_APP layer, too. MBSFN info..). */
  }
  /**
   * For the case of a new eNB, forward the M2 Setup Request to MCE_APP.
   * It needs the information to create and manage MBSFN areas.
   */
  m2ap_mce_itti_m3ap_enb_setup_request(assoc_id, m2ap_enb_id, &m2ap_enb_association->mbms_sa_list);
  OAILOG_FUNC_RETURN (LOG_M2AP, rc);
}

//------------------------------------------------------------------------------
static
  int
m2ap_generate_m2_setup_response (
  m2ap_enb_description_t * m2ap_enb_association, mbsfn_areas_t * mbsfn_areas)
{
  int                                     			  i,j;
  M2AP_M2AP_PDU_t                         			  pdu;
  M2AP_M2SetupResponse_t                 			 *out;
  M2AP_M2SetupResponse_IEs_t             			 *ie = NULL;
  M2AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item_t     *mbsfnCfgItem = NULL;
  uint8_t                                			 *buffer = NULL;
  uint32_t                                			  length = 0;
  int                                     			  rc = RETURNok;

  OAILOG_FUNC_IN (LOG_M2AP);
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

//  M2AP_PLMN_Identity_t                    *plmn = NULL;
//  plmn = calloc (1, sizeof (*plmn));
  ie->value.choice.GlobalMCE_ID.pLMN_Identity.buf = calloc(1, 3);
  uint16_t                                mcc = 0;
  uint16_t                                mnc = 0;
  uint16_t                                mnc_len = 0;
  /** Get the integer values from the PLMN. */
  PLMN_T_TO_MCC_MNC ((mme_config.mbms.mce_plmn), mcc, mnc, mnc_len);
  MCC_MNC_TO_PLMNID (mcc, mnc, mnc_len,	&ie->value.choice.GlobalMCE_ID.pLMN_Identity);

  /** Use same MME code for MCE ID for all eNBs. */
  INT16_TO_OCTET_STRING (mme_config.mbms.mce_id, &ie->value.choice.GlobalMCE_ID.mCE_ID);
  INT16_TO_OCTET_STRING (mme_config.mbms.mce_id, &ie->value.choice.GlobalMCE_ID.pLMN_Identity);

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
    OAILOG_DEBUG (LOG_M2AP, "Removed eNB %d\n", m2ap_enb_association->sctp_assoc_id);
    hashtable_ts_free (&g_m2ap_enb_coll, m2ap_enb_association->sctp_assoc_id);
  } else {
	/*
	 * Consider the response as sent. M2AP is ready to accept UE contexts
	 */
	m2ap_enb_association->m2_state = M2AP_READY;
  }

  /*
   * Non-MBMS signalling -> stream 0
   */
  bstring b = blk2bstr(buffer, length);
  free(buffer);
  rc = m2ap_mce_itti_send_sctp_request (&b, m2ap_enb_association->sctp_assoc_id, 0, INVALID_MCE_MBMS_M2AP_ID);
  OAILOG_FUNC_RETURN (LOG_M2AP, rc);
}

//------------------------------------------------------------------------------
int m2ap_handle_m3ap_enb_setup_res(itti_m3ap_enb_setup_res_t * m3ap_enb_setup_res){
	int														rc = RETURNerror;
	m2ap_enb_description_t 			*m2ap_enb_association = NULL;

	OAILOG_FUNC_IN(LOG_M2AP);

	m2ap_enb_association = m2ap_is_enb_assoc_id_in_list(m3ap_enb_setup_res->sctp_assoc);
  if(!m2ap_enb_association) {
    OAILOG_ERROR (LOG_M2AP, "No eNB association found for %d. Cannot trigger M2 setup response. \n", m3ap_enb_setup_res->sctp_assoc);
    OAILOG_FUNC_RETURN(LOG_M2AP, rc);
  }

  /** Nothing extra is to be done for the MBSFNs. MCE_APP takes care of it. */
  if(!m3ap_enb_setup_res->mbsfn_areas.num_mbsfn_areas){
  	OAILOG_ERROR (LOG_M2AP, "No MBSFN area could be associated for eNB %d. M2 Setup failed. \n", m3ap_enb_setup_res->sctp_assoc);
		rc = m2ap_mce_generate_m2_setup_failure (m2ap_enb_association->sctp_assoc_id, M2AP_ENB_SERVICE_SCTP_STREAM_ID,
				M2AP_Cause_PR_misc, M2AP_CauseMisc_unspecified, -1); /**< eNB should attach again. */
		OAILOG_FUNC_RETURN(LOG_M2AP, rc);
  }
	m2ap_dump_enb (m2ap_enb_association);
  OAILOG_INFO(LOG_M2AP, "MBSFN area could be associated for eNB %d. M2 Setup succeeded. \n", m3ap_enb_setup_res->sctp_assoc);
  rc =  m2ap_generate_m2_setup_response (m2ap_enb_association, &m3ap_enb_setup_res->mbsfn_areas);
  if (rc == RETURNok) {
  	update_mme_app_stats_connected_enb_add();
  }
	OAILOG_FUNC_RETURN(LOG_M2AP, rc);
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
  M2AP_SessionStartResponse_t     		 *container;
  M2AP_SessionStartResponse_Ies_t  		 *ie = NULL;
  mbms_description_t                     *mbms_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  mce_mbms_m2ap_id_t                      mce_mbms_m2ap_id = 0;
  enb_mbms_m2ap_id_t                      enb_mbms_m2ap_id = 0;
  m2ap_enb_description_t				 *m2ap_enb_desc = NULL;

  OAILOG_FUNC_IN (LOG_M2AP);
  /**
   * Check that the eNB is not in the SCTP Association Hashmap.
   * Add it into the list. No need to inform the MCE_APP layer.
   */
  container = &pdu->choice.successfulOutcome.value.choice.SessionStartResponse;
  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_SessionStartResponse_Ies_t, ie, container,
		  M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID, true);
  mce_mbms_m2ap_id = ie->value.choice.MCE_MBMS_M2AP_ID;
  mce_mbms_m2ap_id &= MCE_MBMS_M2AP_ID_MASK;

  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_SessionStartResponse_Ies_t, ie, container,
                             M2AP_ProtocolIE_ID_id_ENB_MBMS_M2AP_ID, true);
  enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t) (ie->value.choice.ENB_MBMS_M2AP_ID & ENB_MBMS_M2AP_ID_MASK);

  if ((mbms_ref_p = m2ap_is_mbms_mce_m2ap_id_in_list(mce_mbms_m2ap_id)) == NULL) {
    OAILOG_ERROR(LOG_M2AP, "No MBMS is attached to this MCE MBMS M2AP id: " MCE_MBMS_M2AP_ID_FMT "\n", mce_mbms_m2ap_id);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }

  /** Check that the eNB description also exists. */
  DevAssert(m2ap_enb_desc = m2ap_is_enb_assoc_id_in_list(assoc_id));

  /**
   * Try to insert the received eNB MBMS M2AP ID into the MBMS service.
   */
  hashtable_rc_t  h_rc = hashtable_ts_insert (&mbms_ref_p->g_m2ap_assoc_id2mce_enb_id_coll, (const hash_key_t) assoc_id, (void *)(uintptr_t)enb_mbms_m2ap_id); /**< Add the value as pointer. No need to free. */
  if (h_rc != HASH_TABLE_OK) {
    OAILOG_ERROR(LOG_M2AP, "Error inserting MBMS description with MCE MBMS M2AP ID " MCE_MBMS_M2AP_ID_FMT" eNB MBMS M2AP ID " ENB_MBMS_M2AP_ID_FMT". Hash-Rc (%d). Leaving the MBMS reference. \n",
    	mce_mbms_m2ap_id, enb_mbms_m2ap_id, h_rc);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }
  m2ap_enb_desc->nb_mbms_associated++;
  OAILOG_INFO(LOG_M2AP, "Successfully started MBMS Service on eNB with sctp assoc id (%d) MBMS description with MCE MBMS M2AP ID " MCE_MBMS_M2AP_ID_FMT " eNB MBMS M2AP ID " ENB_MBMS_M2AP_ID_FMT ". "
		  "# of MBMS services on eNodeB (%d). \n", assoc_id, mce_mbms_m2ap_id, enb_mbms_m2ap_id, m2ap_enb_desc->nb_mbms_associated);
  OAILOG_FUNC_RETURN (LOG_M2AP, RETURNok);
}

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_session_start_failure (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu)
{
  M2AP_SessionStartFailure_t     		 *container;
  M2AP_SessionStartFailure_Ies_t  		 *ie = NULL;
  mbms_description_t                     *mbms_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;
  mce_mbms_m2ap_id_t                      mce_mbms_m2ap_id = 0;

  OAILOG_FUNC_IN (LOG_M2AP);

  /**
   * Check that the eNB is not in the SCTP Association Hashmap.
   * Add it into the list. No need to inform the MCE_APP layer.
   */
  container = &pdu->choice.unsuccessfulOutcome.value.choice.SessionStartFailure;
  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_SessionStartFailure_Ies_t, ie, container,
		  M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID, true);
  mce_mbms_m2ap_id = ie->value.choice.MCE_MBMS_M2AP_ID;
  mce_mbms_m2ap_id &= MCE_MBMS_M2AP_ID_MASK;

  if ((mbms_ref_p = m2ap_is_mbms_mce_m2ap_id_in_list((uint32_t) mce_mbms_m2ap_id)) == NULL) {
    OAILOG_ERROR(LOG_M2AP, "No MBMS is attached to this MCE MBMS M2AP id: " MCE_MBMS_M2AP_ID_FMT "\n", mce_mbms_m2ap_id);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }
  OAILOG_ERROR(LOG_M2AP, "Error starting MBMS Service on eNB with sctp assoc id (%d) MBMS description with MCE MBMS M2AP ID " MCE_MBMS_M2AP_ID_FMT ". \n", assoc_id, mce_mbms_m2ap_id);
  /**
   * Check that there is no SCTP association.
   */
  hashtable_rc_t h_rc = hashtable_ts_is_key_exists(&mbms_ref_p->g_m2ap_assoc_id2mce_enb_id_coll, assoc_id);
  DevAssert(h_rc == HASH_TABLE_KEY_NOT_EXISTS);
  OAILOG_FUNC_RETURN (LOG_M2AP, rc);
}


//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_session_stop_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu)
{
  M2AP_SessionStopResponse_t     		 *container;
  M2AP_SessionStopResponse_Ies_t  		 *ie = NULL;
  mbms_description_t                     *mbms_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  mce_mbms_m2ap_id_t                      mce_mbms_m2ap_id = 0;
  enb_mbms_m2ap_id_t                      enb_mbms_m2ap_id = 0;

  OAILOG_FUNC_IN (LOG_M2AP);

  /**
   * Check that the eNB is not in the SCTP Association Hashmap.
   * Add it into the list. No need to inform the MCE_APP layer.
   */
  container = &pdu->choice.successfulOutcome.value.choice.SessionStopResponse;
  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_SessionStopResponse_Ies_t, ie, container,
	M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID, true);
  mce_mbms_m2ap_id = ie->value.choice.MCE_MBMS_M2AP_ID;
  mce_mbms_m2ap_id &= MCE_MBMS_M2AP_ID_MASK;

  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_SessionStopResponse_Ies_t, ie, container,
	M2AP_ProtocolIE_ID_id_ENB_MBMS_M2AP_ID, true);
  enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t) (ie->value.choice.ENB_MBMS_M2AP_ID & ENB_MBMS_M2AP_ID_MASK);

  /**
   * The MBMS Service may or may not exist (removal only in some eNBs).
   * We perform all changes when transmitting the request.
   */
  OAILOG_FUNC_RETURN (LOG_M2AP, RETURNok);
}

//------------------------------------------------------------------------------
int
m2ap_mce_handle_mbms_session_update_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu)
{
  M2AP_SessionUpdateResponse_t     		 *container;
  M2AP_SessionUpdateResponse_Ies_t  	 *ie = NULL;
  mbms_description_t                	 *mbms_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;
  mce_mbms_m2ap_id_t                      mce_mbms_m2ap_id = 0;
  enb_mbms_m2ap_id_t                      enb_mbms_m2ap_id = 0;

  OAILOG_FUNC_IN (LOG_M2AP);

  /**
   * Check that the eNB is not in the SCTP Association Hashmap.
   * Add it into the list. No need to inform the MCE_APP layer.
   */
  container = &pdu->choice.successfulOutcome.value.choice.SessionUpdateResponse;
  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_SessionUpdateResponse_Ies_t, ie, container,
	M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID, true);
  mce_mbms_m2ap_id = ie->value.choice.MCE_MBMS_M2AP_ID;
  mce_mbms_m2ap_id &= MCE_MBMS_M2AP_ID_MASK;

  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_SessionUpdateResponse_Ies_t, ie, container,
	M2AP_ProtocolIE_ID_id_ENB_MBMS_M2AP_ID, true);
  enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t) (ie->value.choice.ENB_MBMS_M2AP_ID & ENB_MBMS_M2AP_ID_MASK);

  if ((mbms_ref_p = m2ap_is_mbms_mce_m2ap_id_in_list ((uint32_t) mce_mbms_m2ap_id)) == NULL) {
    OAILOG_ERROR(LOG_M2AP, "No MBMS is attached to this MCE MBMS M2AP id: " MCE_MBMS_M2AP_ID_FMT "\n", mce_mbms_m2ap_id);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }
  /**
   * For any case, just remove the SCTP association.
   */
  OAILOG_INFO(LOG_M2AP, "Successfully updated MBMS Service on eNB with sctp assoc id (%d) MBMS description with MCE MBMS M2AP ID " MCE_MBMS_M2AP_ID_FMT " eNB MBMS M2AP ID " ENB_MBMS_M2AP_ID_FMT ". \n",
	  assoc_id, mce_mbms_m2ap_id, enb_mbms_m2ap_id);
  OAILOG_FUNC_RETURN (LOG_M2AP, RETURNok);
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
  m2ap_enb_description_t			*m2ap_enb_desc = NULL;
  int                                rc = RETURNok;
  mce_mbms_m2ap_id_t                 mce_mbms_m2ap_id = 0;
  enb_mbms_m2ap_id_t                 enb_mbms_m2ap_id = 0;

  OAILOG_FUNC_IN (LOG_M2AP);

  /**
   * Check that the eNB is not in the SCTP Association Hashmap.
   * Add it into the list. No need to inform the MCE_APP layer.
   */
  container = &pdu->choice.unsuccessfulOutcome.value.choice.SessionUpdateFailure;
  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_SessionUpdateFailure_Ies_t, ie, container,
		  M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID, true);
  mce_mbms_m2ap_id = ie->value.choice.MCE_MBMS_M2AP_ID;
  mce_mbms_m2ap_id &= MCE_MBMS_M2AP_ID_MASK;

  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_SessionUpdateResponse_Ies_t, ie, container,
	M2AP_ProtocolIE_ID_id_ENB_MBMS_M2AP_ID, true);
  enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t) (ie->value.choice.ENB_MBMS_M2AP_ID & ENB_MBMS_M2AP_ID_MASK);

  if ((mbms_ref_p = m2ap_is_mbms_mce_m2ap_id_in_list ((uint32_t) mce_mbms_m2ap_id)) == NULL) {
    OAILOG_ERROR(LOG_M2AP, "No MBMS is attached to this MCE MBMS M2AP id: " MCE_MBMS_M2AP_ID_FMT "\n", mce_mbms_m2ap_id);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }
  m2ap_enb_desc = m2ap_is_enb_assoc_id_in_list(assoc_id);
  if(m2ap_enb_desc) {
    OAILOG_ERROR(LOG_M2AP, "Failed updating MBMS Service on eNB with sctp assoc id (%d) MBMS description with MCE MBMS M2AP ID " MCE_MBMS_M2AP_ID_FMT ". \n",
    	assoc_id, mce_mbms_m2ap_id);
    m2ap_generate_mbms_session_stop_request(mce_mbms_m2ap_id, assoc_id);
    /** Remove the hash key, which should also update the eNB. */
    hashtable_ts_free(&mbms_ref_p->g_m2ap_assoc_id2mce_enb_id_coll, (const hash_key_t)assoc_id);
    if(m2ap_enb_desc->nb_mbms_associated)
    	m2ap_enb_desc->nb_mbms_associated--;
    OAILOG_ERROR(LOG_M2AP, "Removed association after failed update.\n");
  }
  OAILOG_FUNC_RETURN (LOG_M2AP, rc);
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
  OAILOG_FUNC_IN (LOG_M2AP);


  OAILOG_FUNC_RETURN (LOG_M2AP, rc);
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
  M2AP_Reset_t					     *container;
  M2AP_Reset_Ies_t				     *ie = NULL;
  mbms_description_t                 *mbms_ref_p = NULL;
  m2ap_enb_description_t			 *m2ap_enb_association = NULL;
  itti_m3ap_enb_initiated_reset_ack_t m3ap_enb_initiated_reset_ack = {0};

  OAILOG_FUNC_IN (LOG_M2AP);
  /*
   * Checking that the assoc id has a valid eNB attached to.
   */
  m2ap_enb_association = m2ap_is_enb_assoc_id_in_list (assoc_id);
  if (m2ap_enb_association == NULL) {
    OAILOG_ERROR (LOG_M2AP, "No M2AP eNB attached to this assoc_id: %d\n", assoc_id);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }
  // Check the state of M2AP
  if (m2ap_enb_association->m2_state != M2AP_READY) {
    // Ignore the message
    OAILOG_INFO (LOG_M2AP, "M2 setup is not done.Invalid state.Ignoring ENB Initiated Reset.eNB Id = %d , M2AP state = %d \n", m2ap_enb_association->m2ap_enb_id, m2ap_enb_association->m2_state);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNok);
  }
  // Check the reset type - partial_reset OR M2AP_RESET_ALL
  container = &pdu->choice.initiatingMessage.value.choice.Reset;
  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_Reset_Ies_t, ie, container, M2AP_ProtocolIE_ID_id_ResetType, true);
  M2AP_ResetType_t * m2ap_reset_type = &ie->value.choice.ResetType;
  switch (m2ap_reset_type->present){
    case M2AP_ResetType_PR_m2_Interface:
      m3ap_enb_initiated_reset_ack.m2ap_reset_type = M2AP_RESET_ALL;
	  break;
    case M2AP_ResetType_PR_partOfM2_Interface:
    	m3ap_enb_initiated_reset_ack.m2ap_reset_type = M2AP_RESET_PARTIAL;
  	  break;
    default:
	  OAILOG_ERROR (LOG_M2AP, "Reset Request from M2AP eNB  with Invalid reset_type = %d\n", m2ap_reset_type->present);
	  // TBD - Here MME should send Error Indication as it is abnormal scenario.
	  OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }
  m3ap_enb_initiated_reset_ack.sctp_assoc_id 	= assoc_id;
  m3ap_enb_initiated_reset_ack.sctp_stream_id 	= stream;
  /** Create a cleared array of identifiers. */
  if (m3ap_enb_initiated_reset_ack.m2ap_reset_type != M2AP_RESET_ALL) {
	// Check if there are no MBMSs connected
	if (!m2ap_enb_association->nb_mbms_associated) {
	  // Ignore the message
	  OAILOG_INFO (LOG_M2AP, "No MBMSs is active for eNB. Still continuing with eNB Initiated Reset.eNB Id = %d\n",
		  m2ap_enb_association->m2ap_enb_id);
	}
	m3ap_enb_initiated_reset_ack.num_mbms = m2ap_reset_type->choice.partOfM2_Interface.list.count;
	m2_sig_conn_id_t mbms_to_reset_list[m3ap_enb_initiated_reset_ack.num_mbms]; /**< Create a stacked array. */
	memset(mbms_to_reset_list, 0, m3ap_enb_initiated_reset_ack.num_mbms * sizeof(m2_sig_conn_id_t));
	for (int item = 0; item < m3ap_enb_initiated_reset_ack.num_mbms; item++) {
	  M2AP_MBMS_Service_associatedLogicalM2_ConnectionItemResAck_t *m2_sig_conn_p = (M2AP_MBMS_Service_associatedLogicalM2_ConnectionItemResAck_t*)
		ie->value.choice.ResetType.choice.partOfM2_Interface.list.array[item];
	  /** Initialize each element with invalid values. */
	  mbms_to_reset_list[item].mce_mbms_m2ap_id = INVALID_MCE_MBMS_M2AP_ID;
	  mbms_to_reset_list[item].enb_mbms_m2ap_id = INVALID_ENB_MBMS_M2AP_ID;
	  if(!m2_sig_conn_p) {
	    OAILOG_ERROR (LOG_M2AP, "No logical M2 connection item could be found for the partial connection. "
	    		"Ignoring the received partial reset request. \n");
	    /** No need to answer this one, continue to the next. */
	    continue;
	  }
	  /** We will only implicitly remove the MBMS Description, based on the MCE MBMS M2AP ID or the ENB MBMS M2AP ID. */
	  M2AP_MBMS_Service_associatedLogicalM2_ConnectionItem_t * m2_sig_conn_item = &m2_sig_conn_p->value.choice.MBMS_Service_associatedLogicalM2_ConnectionItem;
	  if (m2_sig_conn_item->mCE_MBMS_M2AP_ID != NULL) {
		mbms_to_reset_list[item].mce_mbms_m2ap_id = ((mce_mbms_m2ap_id_t) *(m2_sig_conn_item->mCE_MBMS_M2AP_ID) & MCE_MBMS_M2AP_ID_MASK);
	  }
	  if (m2_sig_conn_item->eNB_MBMS_M2AP_ID != NULL) {
	    mbms_to_reset_list[item].enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t) *(m2_sig_conn_item->eNB_MBMS_M2AP_ID) & ENB_MBMS_M2AP_ID_MASK;
	  }
	  if(mbms_to_reset_list[item].mce_mbms_m2ap_id == INVALID_MCE_MBMS_M2AP_ID && mbms_to_reset_list[item].enb_mbms_m2ap_id != INVALID_ENB_MBMS_M2AP_ID){
		OAILOG_ERROR (LOG_M2AP, "Received invalid MBMS identifiers. Continuing... \n");
		continue;
	  }
	  /**
	   * Only evaluate the MCE MBMS M2AP Id, should exist.
	   */
	  if(mbms_to_reset_list[item].mce_mbms_m2ap_id == INVALID_MCE_MBMS_M2AP_ID){
		OAILOG_ERROR (LOG_M2AP, "Received invalid MCE MBMS M2AP ID in partial reset request. Skipping. \n");
		continue;
	  }
	  if((mbms_ref_p = m2ap_is_mbms_mce_m2ap_id_in_list(mbms_to_reset_list[item].mce_mbms_m2ap_id)) == NULL) {
        /** No MCE MBMS reference is found, skip the partial reset request. */
		OAILOG_ERROR (LOG_M2AP, "No MBMS Reference was found for MCE MBMS M2AP ID "MCE_MBMS_M2AP_ID_FMT" in partial reset request. Skipping (will add response). \n",
		  mbms_to_reset_list[item].mce_mbms_m2ap_id);
		continue;
	  }
	  /** Check the received eNB MBMS Id, if it is valid, check it.*/
	  enb_mbms_m2ap_id_t *current_enb_mbms_m2ap_id_p = NULL;
	  hashtable_ts_get(&mbms_ref_p->g_m2ap_assoc_id2mce_enb_id_coll, assoc_id, (void**)&current_enb_mbms_m2ap_id_p);
	  if(*current_enb_mbms_m2ap_id_p && *current_enb_mbms_m2ap_id_p != INVALID_ENB_MBMS_M2AP_ID){
	    if(*current_enb_mbms_m2ap_id_p != mbms_to_reset_list[item].enb_mbms_m2ap_id) {
	      OAILOG_ERROR (LOG_M2AP, "MBMS Service for MCE MBMS M2AP ID "MCE_MBMS_M2AP_ID_FMT" has ENB MBMS M2AP ID " ENB_MBMS_M2AP_ID_FMT ", "
				"whereas partial reset received for ENB MBMS M2AP ID " ENB_MBMS_M2AP_ID_FMT ". Skipping. \n",
			mbms_to_reset_list[item].mce_mbms_m2ap_id, *current_enb_mbms_m2ap_id_p, mbms_to_reset_list[item].enb_mbms_m2ap_id);
	      continue;
	    }
	  }
	  OAILOG_WARNING(LOG_M2AP, "Removing SCTP association of MBMS Service for MCE MBMS M2AP ID "MCE_MBMS_M2AP_ID_FMT" with ENB MBMS M2AP ID " ENB_MBMS_M2AP_ID_FMT " due"
		"partial reset from eNB with sctp-assoc-id (%d). \n", mbms_to_reset_list[item].mce_mbms_m2ap_id,
		mbms_to_reset_list[item].enb_mbms_m2ap_id, assoc_id);
	  /** Remove the hash key, which should also update the eNB. */
	  hashtable_ts_free(&mbms_ref_p->g_m2ap_assoc_id2mce_enb_id_coll, (const hash_key_t)assoc_id);
	  if(m2ap_enb_association->nb_mbms_associated)
		m2ap_enb_association->nb_mbms_associated--;
	}
	OAILOG_INFO(LOG_M2AP, "Successfully performed partial reset for M2AP eNB with sctp-assoc-id (%d). Sending back M2AP Reset ACK. \n", assoc_id);
	m3ap_enb_initiated_reset_ack.mbsm_to_reset_list = mbms_to_reset_list;
	m3ap_handle_enb_initiated_reset_ack(&m3ap_enb_initiated_reset_ack);	  /**< Array scope. */
 } else {
	  /** Generating full reset request. */
	  m2ap_enb_full_reset((void*)m2ap_enb_association);
	  m3ap_handle_enb_initiated_reset_ack(&m3ap_enb_initiated_reset_ack); /**< Array scope. */
	  OAILOG_INFO (LOG_M2AP, "Received a full reset request from eNB with SCTP-ASSOC (%d). Removing all SCTP associations. Sending RESET ACK.\n", assoc_id);
  }
  OAILOG_FUNC_RETURN (LOG_M2AP, RETURNok);
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
  OAILOG_FUNC_IN (LOG_M2AP);
  OAILOG_INFO(LOG_M2AP, "eNodeB with sctp-assoc (%d) not capable of processing the MBMS Service Counting Request. \n",
		  assoc_id);
  OAILOG_FUNC_RETURN (LOG_M2AP, rc);
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
  OAILOG_FUNC_IN (LOG_M2AP);
  OAILOG_ERROR(LOG_M2AP, "M2AP eNodeB with sctp-assoc (%d) not capable of processing the MBMS Service Counting Request. \n",
		  assoc_id);
  OAILOG_FUNC_RETURN (LOG_M2AP, rc);
}

//------------------------------------------------------------------------------
int
m2ap_mce_handle_error_indication (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    M2AP_M2AP_PDU_t *pdu)
{
  enb_mbms_m2ap_id_t                        enb_mbms_m2ap_id = INVALID_ENB_MBMS_M2AP_ID;
  mce_mbms_m2ap_id_t                        mce_mbms_m2ap_id = INVALID_MCE_MBMS_M2AP_ID;
  M2AP_ErrorIndication_t             	   *container = NULL;
  M2AP_ErrorIndication_Ies_t 			   *ie = NULL;
  MessageDef                               *message_p = NULL;
  long                                      cause_value;

  OAILOG_FUNC_IN (LOG_M2AP);

  container = &pdu->choice.initiatingMessage.value.choice.ErrorIndication;
  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_ErrorIndication_Ies_t, ie, container, M2AP_ProtocolIE_ID_id_MCE_MBMS_M2AP_ID, false);
  if(ie) {
	mce_mbms_m2ap_id = ie->value.choice.MCE_MBMS_M2AP_ID;
	mce_mbms_m2ap_id &= MCE_MBMS_M2AP_ID_MASK;
  }

  M2AP_FIND_PROTOCOLIE_BY_ID(M2AP_ErrorIndication_Ies_t, ie, container, M2AP_ProtocolIE_ID_id_ENB_MBMS_M2AP_ID, false);
  // eNB MBMS M2AP ID is limited to 24 bits
  if(ie)
    enb_mbms_m2ap_id = (enb_mbms_m2ap_id_t) (ie->value.choice.ENB_MBMS_M2AP_ID & ENB_MBMS_M2AP_ID_MASK);

  m2ap_enb_description_t * m2ap_enb_ref = m2ap_is_enb_assoc_id_in_list (assoc_id);
  DevAssert(m2ap_enb_ref);

  if(mce_mbms_m2ap_id == INVALID_MCE_MBMS_M2AP_ID){
    OAILOG_ERROR(LOG_M2AP, "Received invalid mce_mbms_m2ap_id " MCE_MBMS_M2AP_ID_FMT". Dropping error indication. \n", mce_mbms_m2ap_id);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }

  mbms_description_t * mbms_ref = m2ap_is_mbms_mce_m2ap_id_in_list(mce_mbms_m2ap_id);
  /** If no MBMS reference exists, drop the message. */
  if(!mbms_ref){
    /** Check that the MBMS reference exists. */
    OAILOG_ERROR(LOG_M2AP, "No MBMS reference exists for MCE MBMS Id "MCE_MBMS_M2AP_ID_FMT ". Dropping error indication. \n",
    	mce_mbms_m2ap_id);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }

  /**
   * No need to inform the MCE App layer, implicitly remove the MBMS Service session.
   */
  OAILOG_ERROR(LOG_M2AP, "Received Error indication for MBMS Service MCE MBMS M2AP ID " MCE_MBMS_M2AP_ID_FMT " with eNB MBMS M2AP Id " ENB_MBMS_M2AP_ID_FMT " on eNB with sctp assoc id (%d). \n",
		  mce_mbms_m2ap_id, enb_mbms_m2ap_id, assoc_id);
  m2ap_generate_mbms_session_stop_request(mce_mbms_m2ap_id, assoc_id);
  /** Remove the hash key, which should also update the eNB. */
  hashtable_ts_free(&mbms_ref->g_m2ap_assoc_id2mce_enb_id_coll, (const hash_key_t)assoc_id);
  if(m2ap_enb_ref->nb_mbms_associated)
	m2ap_enb_ref->nb_mbms_associated--;
  OAILOG_ERROR(LOG_M2AP, "Removed association after error indication.\n");
  OAILOG_FUNC_RETURN (LOG_M2AP, RETURNok);
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
  OAILOG_FUNC_IN (LOG_M2AP);

  OAILOG_FUNC_RETURN (LOG_M2AP, rc);
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
  OAILOG_FUNC_IN (LOG_M2AP);

  OAILOG_FUNC_RETURN (LOG_M2AP, rc);
}

//------------------------------------------------------------------------------
int
m2ap_handle_sctp_disconnection (
    const sctp_assoc_id_t assoc_id)
{
  int                                     i = 0;
  m2ap_enb_description_t                 *m2ap_enb_association = NULL;

  OAILOG_FUNC_IN (LOG_M2AP);
  /*
   * Checking that the assoc id has a valid eNB attached to.
   */
  m2ap_enb_association = m2ap_is_enb_assoc_id_in_list (assoc_id);

  if (m2ap_enb_association == NULL) {
    OAILOG_ERROR (LOG_M2AP, "No M2AP eNB attached to this assoc_id: %d\n", assoc_id);
    OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
  }
  /**
   * Remove the ENB association, which will also remove all SCTP associations of the MBMS Services.
   */
  hashtable_ts_free (&g_m2ap_enb_coll, m2ap_enb_association->sctp_assoc_id);
  update_mce_app_stats_connected_enb_sub();
  OAILOG_INFO(LOG_M2AP, "Removing M2AP eNB with association id %u \n", assoc_id);
  m2ap_dump_enb_list ();
  OAILOG_DEBUG (LOG_M2AP, "Removed M2AP eNB attached to assoc_id: %d\n", assoc_id);
  OAILOG_FUNC_RETURN (LOG_M2AP, RETURNok);
}

//------------------------------------------------------------------------------
int
m2ap_handle_new_association (
  sctp_new_peer_t * sctp_new_peer_p)
{
  m2ap_enb_description_t                      *m2ap_enb_association = NULL;

  OAILOG_FUNC_IN (LOG_M2AP);
  DevAssert (sctp_new_peer_p != NULL);

  /*
   * Checking that the assoc id has a valid eNB attached to.
   */
  if ((m2ap_enb_association = m2ap_is_enb_assoc_id_in_list (sctp_new_peer_p->assoc_id)) == NULL) {
    OAILOG_DEBUG (LOG_M2AP, "Create eNB context for assoc_id: %d\n", sctp_new_peer_p->assoc_id);
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
      OAILOG_ERROR (LOG_M2AP, "Failed to allocate eNB context for assoc_id: %d\n", sctp_new_peer_p->assoc_id);
    }
    m2ap_enb_association->sctp_assoc_id = sctp_new_peer_p->assoc_id;
    hashtable_rc_t  hash_rc = hashtable_ts_insert (&g_m2ap_enb_coll, (const hash_key_t)m2ap_enb_association->sctp_assoc_id, (void *)m2ap_enb_association);
    if (HASH_TABLE_OK != hash_rc) {
      OAILOG_FUNC_RETURN (LOG_M2AP, RETURNerror);
    }
  } else if ((m2ap_enb_association->m2_state == M2AP_SHUTDOWN) || (m2ap_enb_association->m2_state == M2AP_RESETING)) {
    OAILOG_WARNING(LOG_M2AP, "Received new association request on an association that is being %s, ignoring",
                   m2_enb_state_str[m2ap_enb_association->m2_state]);
    OAILOG_FUNC_RETURN(LOG_M2AP, RETURNerror);
  } else {
    OAILOG_DEBUG (LOG_M2AP, "eNB context already exists for assoc_id: %d, update it\n", sctp_new_peer_p->assoc_id);
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
//  m2ap_enb_association->next_sctp_stream = 1;
  m2ap_enb_association->m2_state = M2AP_INIT;
  OAILOG_FUNC_RETURN (LOG_M2AP, RETURNok);
}

//------------------------------------------------------------------------------
int
m3ap_handle_enb_initiated_reset_ack (
  const itti_m3ap_enb_initiated_reset_ack_t * const m2ap_enb_reset_ack_p)
{
  uint8_t                                *buffer = NULL;
  uint32_t                                length = 0;
  M2AP_M2AP_PDU_t                         pdu;
  /** Reset Acknowledgment. */
  M2AP_ResetAcknowledge_t				 *out;
  M2AP_ResetAcknowledge_Ies_t	    	 *reset_ack_ie = NULL;
  M2AP_ResetAcknowledge_Ies_t	   	     *reset_ack_ie_list = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_M2AP);

  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M2AP_M2AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = M2AP_ProcedureCode_id_reset;
  pdu.choice.successfulOutcome.criticality = M2AP_Criticality_ignore;
  pdu.choice.successfulOutcome.value.present = M2AP_SuccessfulOutcome__value_PR_ResetAcknowledge;
  out = &pdu.choice.successfulOutcome.value.choice.ResetAcknowledge;

  if (m2ap_enb_reset_ack_p->m2ap_reset_type == M2AP_RESET_PARTIAL) {
    /** Add an Item for each counter. */
	if(m2ap_enb_reset_ack_p->mbsm_to_reset_list){
	  reset_ack_ie_list = (M2AP_ResetAcknowledge_Ies_t *)calloc(1, sizeof(M2AP_ResetAcknowledge_Ies_t));
	  reset_ack_ie_list->id = M2AP_ProtocolIE_ID_id_MBMS_Service_associatedLogicalM2_ConnectionListResAck;
	  reset_ack_ie_list->criticality = M2AP_Criticality_ignore;
	  reset_ack_ie_list->value.present = M2AP_ResetAcknowledge_Ies__value_PR_MBMS_Service_associatedLogicalM2_ConnectionListResAck;
	  ASN_SEQUENCE_ADD(&out->protocolIEs.list, reset_ack_ie_list);
	  /** MCE MBMS M2AP ID. */
	  M2AP_MBMS_Service_associatedLogicalM2_ConnectionListResAck_t * ie_p = &reset_ack_ie_list->value.choice.MBMS_Service_associatedLogicalM2_ConnectionListResAck;
	  /** Allocate an item for each. */
	  for (uint32_t i = 0; i < m2ap_enb_reset_ack_p->num_mbms; i++) {
		M2AP_MBMS_Service_associatedLogicalM2_ConnectionItemResAck_t * sig_conn_item = calloc (1, sizeof (M2AP_MBMS_Service_associatedLogicalM2_ConnectionItemResAck_t));
		sig_conn_item->id = M2AP_ProtocolIE_ID_id_MBMS_Service_associatedLogicalM2_ConnectionItem;
		sig_conn_item->criticality = M2AP_Criticality_ignore;
		sig_conn_item->value.present = M2AP_MBMS_Service_associatedLogicalM2_ConnectionItemResAck__value_PR_MBMS_Service_associatedLogicalM2_ConnectionItem;
		M2AP_MBMS_Service_associatedLogicalM2_ConnectionItem_t * item = &sig_conn_item->value.choice.MBMS_Service_associatedLogicalM2_ConnectionItem;
		if (m2ap_enb_reset_ack_p->mbsm_to_reset_list[i].mce_mbms_m2ap_id != INVALID_MCE_MBMS_M2AP_ID) {
		  item->mCE_MBMS_M2AP_ID = calloc(1, sizeof(M2AP_MCE_MBMS_M2AP_ID_t));
		  *item->mCE_MBMS_M2AP_ID = m2ap_enb_reset_ack_p->mbsm_to_reset_list[i].mce_mbms_m2ap_id;
		}
		else {
		  item->mCE_MBMS_M2AP_ID = NULL;
		}
		/** ENB MBMS M2AP ID. */
		if (m2ap_enb_reset_ack_p->mbsm_to_reset_list[i].enb_mbms_m2ap_id != INVALID_ENB_MBMS_M2AP_ID) {
			item->eNB_MBMS_M2AP_ID = calloc(1, sizeof(M2AP_ENB_MBMS_M2AP_ID_t));
			*item->eNB_MBMS_M2AP_ID = m2ap_enb_reset_ack_p->mbsm_to_reset_list[i].enb_mbms_m2ap_id;
		}
		else {
			item->eNB_MBMS_M2AP_ID = NULL;
		}
		ASN_SEQUENCE_ADD(&ie_p->list, sig_conn_item);
	  }
	}
  }

  if (m2ap_mce_encode_pdu (&pdu, &buffer, &length) < 0) {
	OAILOG_ERROR (LOG_M2AP, "Failed to M2 Reset command \n");
	/** We rely on the handover_notify timeout to remove the MBMS context. */
	DevAssert(!buffer);
	OAILOG_FUNC_OUT (LOG_M2AP);
  }

  if(buffer) {
	bstring b = blk2bstr(buffer, length);
	free(buffer);
	rc = m2ap_mce_itti_send_sctp_request (&b, m2ap_enb_reset_ack_p->sctp_assoc_id, m2ap_enb_reset_ack_p->sctp_stream_id, INVALID_MCE_MBMS_M2AP_ID);
  }
  OAILOG_FUNC_RETURN (LOG_M2AP, rc);
}

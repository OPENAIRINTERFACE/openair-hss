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


/*! \file mme_app_bearer.c
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "common_types.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "sgw_ie_defs.h"
#include "common_defs.h"
#include "gcc_diag.h"
#include "mme_app_itti_messaging.h"
#include "xml_msg_dump_itti.h"


//------------------------------------------------------------------------------
int
mme_app_send_s11_release_access_bearers_req (
  struct ue_context_s *const ue_context_pP)
{
  /*
   * Keep the identifier to the default APN
   */
  MessageDef                             *message_p = NULL;
  itti_s11_release_access_bearers_request_t         *release_access_bearers_request_p = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context_pP );
  message_p = itti_alloc_new_message (TASK_MME_APP, S11_RELEASE_ACCESS_BEARERS_REQUEST);
  release_access_bearers_request_p = &message_p->ittiMsg.s11_release_access_bearers_request;
  release_access_bearers_request_p->local_teid = ue_context_pP->mme_s11_teid;
  release_access_bearers_request_p->teid = ue_context_pP->sgw_s11_teid;
  int num_ebi = 0;
  for (int i = 0; i < BEARERS_PER_UE; i++) {
    if (BEARER_STATE_NULL < ue_context_pP->eps_bearers[i].bearer_state) {
      release_access_bearers_request_p->list_of_rabs.ebis[num_ebi] = i;
      num_ebi += 1;
    }
  }
  release_access_bearers_request_p->list_of_rabs.num_ebi = num_ebi;
  release_access_bearers_request_p->originating_node = NODE_TYPE_MME;


  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 S11_RELEASE_ACCESS_BEARERS_REQUEST teid %u ebi %u",
      release_access_bearers_request_p->teid, release_access_bearers_request_p->list_of_rabs.ebis[0]);
  rc = itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}


//------------------------------------------------------------------------------
int
mme_app_send_s11_create_session_req (
  struct ue_context_s *const ue_context_pP)
{
  uint8_t                                 i = 0;

  /*
   * Keep the identifier to the default APN
   */
  context_identifier_t                    context_identifier = 0;
  MessageDef                             *message_p = NULL;
  itti_s11_create_session_request_t      *session_request_p = NULL;
  struct apn_configuration_s             *default_apn_p = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context_pP );
  OAILOG_DEBUG (LOG_MME_APP, "Handling imsi " IMSI_64_FMT "\n", ue_context_pP->imsi);

  if (ue_context_pP->sub_status != SS_SERVICE_GRANTED) {
    /*
     * HSS rejected the bearer creation or roaming is not allowed for this
     * UE. This result will trigger an ESM Failure message sent to UE.
     */
    DevMessage ("Not implemented: ACCESS NOT GRANTED, send ESM Failure to NAS\n");
  }

  message_p = itti_alloc_new_message (TASK_MME_APP, S11_CREATE_SESSION_REQUEST);
  /*
   * WARNING:
   * Some parameters should be provided by NAS Layer:
   * - ue_time_zone
   * - mei
   * - uli
   * - uci
   * Some parameters should be provided by HSS:
   * - PGW address for CP
   * - paa
   * - ambr
   * and by MME Application layer:
   * - selection_mode
   * Set these parameters with random values for now.
   */
  session_request_p = &message_p->ittiMsg.s11_create_session_request;
  /*
   * As the create session request is the first exchanged message and as
   * no tunnel had been previously setup, the distant teid is set to 0.
   * The remote teid will be provided in the response message.
   */
  session_request_p->teid = 0;
  IMSI64_TO_STRING (ue_context_pP->imsi, (char *)session_request_p->imsi.digit);
  // message content was set to 0
  session_request_p->imsi.length = strlen ((const char *)session_request_p->imsi.digit);
  /*
   * Copy the MSISDN
   */
  memcpy (session_request_p->msisdn.digit, ue_context_pP->msisdn, ue_context_pP->msisdn_length);
  session_request_p->msisdn.length = ue_context_pP->msisdn_length;
  session_request_p->rat_type = RAT_EUTRAN;
  /*
   * Copy the subscribed ambr to the sgw create session request message
   */
  memcpy (&session_request_p->ambr, &ue_context_pP->subscribed_ambr, sizeof (ambr_t));

  if (ue_context_pP->apn_profile.nb_apns == 0) {
    DevMessage ("No APN returned by the HSS");
  }

  context_identifier = ue_context_pP->apn_profile.context_identifier;

  for (i = 0; i < ue_context_pP->apn_profile.nb_apns; i++) {
    default_apn_p = &ue_context_pP->apn_profile.apn_configuration[i];

    /*
     * OK we got our default APN
     */
    if (default_apn_p->context_identifier == context_identifier)
      break;
  }

  if (!default_apn_p) {
    /*
     * Unfortunately we didn't find our default APN...
     */
    DevMessage ("No default APN found");
  }

  ue_context_pP->eps_bearers[0].bearer_state   = BEARER_STATE_SGW_CREATE_REQUESTED;
  ue_context_pP->eps_bearers[0].qci            = default_apn_p->subscribed_qos.qci;
  ue_context_pP->eps_bearers[0].priority_level = default_apn_p->subscribed_qos.allocation_retention_priority.priority_level;
  ue_context_pP->eps_bearers[0].preemption_vulnerability = default_apn_p->subscribed_qos.allocation_retention_priority.pre_emp_vulnerability;
  ue_context_pP->eps_bearers[0].preemption_capability    = default_apn_p->subscribed_qos.allocation_retention_priority.pre_emp_capability;

  // Zero because default bearer (see 29.274)
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.gbr.br_ul = 0;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.gbr.br_dl = 0;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.mbr.br_ul = 0;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.mbr.br_dl = 0;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.qci = default_apn_p->subscribed_qos.qci;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pvi = default_apn_p->subscribed_qos.allocation_retention_priority.pre_emp_vulnerability;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pci = default_apn_p->subscribed_qos.allocation_retention_priority.pre_emp_capability;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pl = default_apn_p->subscribed_qos.allocation_retention_priority.priority_level;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].eps_bearer_id = 5;
  session_request_p->bearer_contexts_to_be_created.num_bearer_context = 1;
  /*
   * Asking for default bearer in initial UE message.
   * Use the address of ue_context as unique TEID: Need to find better here
   * and will generate unique id only for 32 bits platforms.
   */
  OAI_GCC_DIAG_OFF(pointer-to-int-cast);
  session_request_p->sender_fteid_for_cp.teid = (teid_t) ue_context_pP;
  OAI_GCC_DIAG_ON(pointer-to-int-cast);
  session_request_p->sender_fteid_for_cp.interface_type = S11_MME_GTP_C;
  mme_config_read_lock (&mme_config);
  session_request_p->sender_fteid_for_cp.ipv4_address = mme_config.ipv4.s11;
  mme_config_unlock (&mme_config);
  session_request_p->sender_fteid_for_cp.ipv4 = 1;

  //ue_context_pP->mme_s11_teid = session_request_p->sender_fteid_for_cp.teid;
  ue_context_pP->sgw_s11_teid = 0;
  mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context_pP,
                                   ue_context_pP->enb_s1ap_id_key,
                                   ue_context_pP->mme_ue_s1ap_id,
                                   ue_context_pP->imsi,
                                   session_request_p->sender_fteid_for_cp.teid,       // mme_s11_teid is new
                                   &ue_context_pP->guti);
  memcpy (session_request_p->apn, default_apn_p->service_selection, default_apn_p->service_selection_length);
  /*
   * Set PDN type for pdn_type and PAA even if this IE is redundant
   */
  session_request_p->pdn_type = default_apn_p->pdn_type;
  session_request_p->paa.pdn_type = default_apn_p->pdn_type;

  if (default_apn_p->nb_ip_address == 0) {
    /*
     * UE DHCPv4 allocated ip address
     */
    memset (session_request_p->paa.ipv4_address, 0, 4);
    memset (session_request_p->paa.ipv6_address, 0, 16);
  } else {
    uint8_t                                 j;

    for (j = 0; j < default_apn_p->nb_ip_address; j++) {
      ip_address_t                           *ip_address;

      ip_address = &default_apn_p->ip_address[j];

      if (ip_address->pdn_type == IPv4) {
        memcpy (session_request_p->paa.ipv4_address, ip_address->address.ipv4_address, 4);
      } else if (ip_address->pdn_type == IPv6) {
        memcpy (session_request_p->paa.ipv6_address, ip_address->address.ipv6_address, 16);
      }
      //             free(ip_address);
    }
  }

  copy_protocol_configuration_options (&session_request_p->pco, &ue_context_pP->pending_pdn_connectivity_req_pco);
  clear_protocol_configuration_options(&ue_context_pP->pending_pdn_connectivity_req_pco);

  mme_config_read_lock (&mme_config);
  session_request_p->peer_ip = mme_config.ipv4.sgw_s11;
  mme_config_unlock (&mme_config);
  session_request_p->serving_network.mcc[0] = ue_context_pP->e_utran_cgi.plmn.mcc_digit1;
  session_request_p->serving_network.mcc[1] = ue_context_pP->e_utran_cgi.plmn.mcc_digit2;
  session_request_p->serving_network.mcc[2] = ue_context_pP->e_utran_cgi.plmn.mcc_digit3;
  session_request_p->serving_network.mnc[0] = ue_context_pP->e_utran_cgi.plmn.mnc_digit1;
  session_request_p->serving_network.mnc[1] = ue_context_pP->e_utran_cgi.plmn.mnc_digit2;
  session_request_p->serving_network.mnc[2] = ue_context_pP->e_utran_cgi.plmn.mnc_digit3;
  session_request_p->selection_mode = MS_O_N_P_APN_S_V;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0,
      "0 S11_CREATE_SESSION_REQUEST imsi " IMSI_64_FMT, ue_context_pP->imsi);
  rc = itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}



//------------------------------------------------------------------------------
int
mme_app_handle_nas_pdn_connectivity_req (
  itti_nas_pdn_connectivity_req_t * const nas_pdn_connectivity_req_pP)
{
  struct ue_context_s                    *ue_context_p = NULL;
  imsi64_t                                imsi64 = INVALID_IMSI64;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (nas_pdn_connectivity_req_pP );
  IMSI_STRING_TO_IMSI64 ((char *)nas_pdn_connectivity_req_pP->imsi, &imsi64);
  OAILOG_DEBUG (LOG_MME_APP, "Received NAS_PDN_CONNECTIVITY_REQ from NAS Handling imsi " IMSI_64_FMT "\n", imsi64);

  if ((ue_context_p = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, imsi64)) == NULL) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "NAS_PDN_CONNECTIVITY_REQ Unknown imsi " IMSI_64_FMT, imsi64);
    OAILOG_ERROR (LOG_MME_APP, "That's embarrassing as we don't know this IMSI\n");
    mme_ue_context_dump_coll_keys();
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /*
   * Consider the UE authenticated
   */
  ue_context_p->imsi_auth = IMSI_AUTHENTICATED;
  // Temp: save request, in near future merge wisely params in context
  memset (ue_context_p->pending_pdn_connectivity_req_imsi, 0, 16);
  AssertFatal ((nas_pdn_connectivity_req_pP->imsi_length > 0)
               && (nas_pdn_connectivity_req_pP->imsi_length < 16), "BAD IMSI LENGTH %d", nas_pdn_connectivity_req_pP->imsi_length);
  AssertFatal ((nas_pdn_connectivity_req_pP->imsi_length > 0)
               && (nas_pdn_connectivity_req_pP->imsi_length < 16), "STOP ON IMSI LENGTH %d", nas_pdn_connectivity_req_pP->imsi_length);
  memcpy (ue_context_p->pending_pdn_connectivity_req_imsi, nas_pdn_connectivity_req_pP->imsi, nas_pdn_connectivity_req_pP->imsi_length);
  ue_context_p->pending_pdn_connectivity_req_imsi_length = nas_pdn_connectivity_req_pP->imsi_length;

  // copy
  if (ue_context_p->pending_pdn_connectivity_req_apn) {
    bdestroy_wrapper (&ue_context_p->pending_pdn_connectivity_req_apn);
  }
  ue_context_p->pending_pdn_connectivity_req_apn =  nas_pdn_connectivity_req_pP->apn;
  nas_pdn_connectivity_req_pP->apn = NULL;

  // copy
  if (ue_context_p->pending_pdn_connectivity_req_pdn_addr) {
    bdestroy_wrapper (&ue_context_p->pending_pdn_connectivity_req_pdn_addr);
  }
  ue_context_p->pending_pdn_connectivity_req_pdn_addr =  nas_pdn_connectivity_req_pP->pdn_addr;
  nas_pdn_connectivity_req_pP->pdn_addr = NULL;

  ue_context_p->pending_pdn_connectivity_req_pti = nas_pdn_connectivity_req_pP->pti;
  ue_context_p->pending_pdn_connectivity_req_ue_id = nas_pdn_connectivity_req_pP->ue_id;
  copy_protocol_configuration_options (&ue_context_p->pending_pdn_connectivity_req_pco, &nas_pdn_connectivity_req_pP->pco);
  clear_protocol_configuration_options(&nas_pdn_connectivity_req_pP->pco);

  memcpy (&ue_context_p->pending_pdn_connectivity_req_qos, &nas_pdn_connectivity_req_pP->qos, sizeof (network_qos_t));
  ue_context_p->pending_pdn_connectivity_req_request_type = nas_pdn_connectivity_req_pP->request_type;
  //if ((nas_pdn_connectivity_req_pP->apn.value == NULL) || (nas_pdn_connectivity_req_pP->apn.length == 0)) {
  /*
   * TODO: Get keys...
   */
  /*
   * Now generate S6A ULR
   */
  rc =  mme_app_send_s6a_update_location_req (ue_context_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
  //} else {
  //return mme_app_send_s11_create_session_req(ue_context_p);
  //}
  //return -1;
}



// sent by NAS
//------------------------------------------------------------------------------
void
mme_app_handle_conn_est_cnf (
  itti_nas_conn_est_cnf_t * const nas_conn_est_cnf_pP)
{
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;
  itti_mme_app_connection_establishment_cnf_t *establishment_cnf_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  OAILOG_DEBUG (LOG_MME_APP, "Received NAS_CONNECTION_ESTABLISHMENT_CNF from NAS\n");
  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, nas_conn_est_cnf_pP->ue_id);

  if (ue_context_p == NULL) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "NAS_CONNECTION_ESTABLISHMENT_CNF Unknown ue %u", nas_conn_est_cnf_pP->ue_id);
    OAILOG_ERROR (LOG_MME_APP, "UE context doesn't exist for UE %06" PRIX32 "/dec%u\n", nas_conn_est_cnf_pP->ue_id, nas_conn_est_cnf_pP->ue_id);
    // memory leak
    bdestroy_wrapper(&nas_conn_est_cnf_pP->nas_msg);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  message_p = itti_alloc_new_message (TASK_MME_APP, MME_APP_CONNECTION_ESTABLISHMENT_CNF);
  establishment_cnf_p = &message_p->ittiMsg.mme_app_connection_establishment_cnf;

  establishment_cnf_p->ue_id = nas_conn_est_cnf_pP->ue_id;

  int j = 0;
  for (int i=0; i < BEARERS_PER_UE; i++) {
    if (BEARER_STATE_SGW_CREATED <= ue_context_p->eps_bearers[i].bearer_state) {
      establishment_cnf_p->e_rab_id[j]                                 = i ;//+ EPS_BEARER_IDENTITY_FIRST;
      establishment_cnf_p->e_rab_level_qos_qci[j]                      = ue_context_p->eps_bearers[i].qci;
      establishment_cnf_p->e_rab_level_qos_priority_level[j]           = ue_context_p->eps_bearers[i].priority_level;
      establishment_cnf_p->e_rab_level_qos_preemption_capability[j]    = ue_context_p->eps_bearers[i].preemption_capability;
      establishment_cnf_p->e_rab_level_qos_preemption_vulnerability[j] = ue_context_p->eps_bearers[i].preemption_vulnerability;
      establishment_cnf_p->transport_layer_address[j]                  = ip_address_to_bstring(&ue_context_p->eps_bearers[i].s_gw_address);
      establishment_cnf_p->gtp_teid[j]                                 = ue_context_p->eps_bearers[i].s_gw_teid;
      if (!j) {
        establishment_cnf_p->nas_pdu[j]                                = nas_conn_est_cnf_pP->nas_msg;
        nas_conn_est_cnf_pP->nas_msg = NULL;
      }
      j=j+1;
    }
  }
  establishment_cnf_p->no_of_e_rabs = j;

//#pragma message  "Check ue_context_p ambr"
  establishment_cnf_p->ue_ambr.br_ul = ue_context_p->subscribed_ambr.br_ul;
  establishment_cnf_p->ue_ambr.br_dl = ue_context_p->subscribed_ambr.br_dl;
  establishment_cnf_p->ue_security_capabilities_encryption_algorithms = nas_conn_est_cnf_pP->selected_encryption_algorithm;
  establishment_cnf_p->ue_security_capabilities_integrity_algorithms = nas_conn_est_cnf_pP->selected_integrity_algorithm;
  memcpy(establishment_cnf_p->kenb, nas_conn_est_cnf_pP->kenb, AUTH_KENB_SIZE);


  OAILOG_DEBUG (LOG_MME_APP, "security_capabilities_encryption_algorithms 0x%04X\n", establishment_cnf_p->ue_security_capabilities_encryption_algorithms);
  OAILOG_DEBUG (LOG_MME_APP, "security_capabilities_integrity_algorithms  0x%04X\n", establishment_cnf_p->ue_security_capabilities_integrity_algorithms);

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0,
                      "0 MME_APP_CONNECTION_ESTABLISHMENT_CNF ebi %u s1u_sgw teid " TEID_FMT " qci %u prio level %u sea 0x%x sia 0x%x",
                      establishment_cnf_p->e_rab_id[0],
                      establishment_cnf_p->gtp_teid[0],
                      establishment_cnf_p->e_rab_level_qos_qci[0],
                      establishment_cnf_p->e_rab_level_qos_priority_level[0],
                      establishment_cnf_p->ue_security_capabilities_encryption_algorithms,
                      establishment_cnf_p->ue_security_capabilities_integrity_algorithms);
  int to_task = (RUN_MODE_SCENARIO_PLAYER == mme_config.run_mode) ? TASK_MME_SCENARIO_PLAYER:TASK_S1AP;
  itti_send_msg_to_task (to_task, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}



// sent by S1AP
//------------------------------------------------------------------------------
void
mme_app_handle_initial_ue_message (
  itti_s1ap_initial_ue_message_t * const initial_pP)
{
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  OAILOG_DEBUG (LOG_MME_APP, "Received MME_APP_INITIAL_UE_MESSAGE from S1AP\n");
  XML_MSG_DUMP_ITTI_S1AP_INITIAL_UE_MESSAGE(initial_pP, TASK_S1AP, TASK_MME_APP, NULL);

  if (INVALID_MME_UE_S1AP_ID != initial_pP->mme_ue_s1ap_id) {
    ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, initial_pP->mme_ue_s1ap_id);
  }

  if (!(ue_context_p)) {
    enb_s1ap_id_key_t enb_s1ap_id_key = 0;
    MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, initial_pP->cgi.cell_identity.enb_id, initial_pP->enb_ue_s1ap_id);
    ue_context_p = mme_ue_context_exists_enb_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, enb_s1ap_id_key);
  }
  if (!(ue_context_p)) {
    OAILOG_DEBUG (LOG_MME_APP, "Unknown  mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT "\n",
        initial_pP->mme_ue_s1ap_id, initial_pP->enb_ue_s1ap_id);

    // MME UE S1AP ID AND NAS UE ID ARE THE SAME
    if (INVALID_MME_UE_S1AP_ID == initial_pP->mme_ue_s1ap_id) {
      if (initial_pP->is_s_tmsi_valid) {
        // try to build a Guti
        guti_t guti = {.gummei.plmn = {0}, .gummei.mme_gid = 0, .gummei.mme_code = 0, .m_tmsi = INVALID_M_TMSI};
        guti.m_tmsi = initial_pP->opt_s_tmsi.m_tmsi;
        guti.gummei.mme_code = initial_pP->opt_s_tmsi.mme_code;

        if (initial_pP->is_gummei_valid) {
          memcpy(&guti.gummei, (const void*)&initial_pP->opt_gummei, sizeof(guti.gummei));
          ue_context_p = mme_ue_context_exists_guti (&mme_app_desc.mme_ue_contexts, &guti);

          if (ue_context_p) {
            if (ue_context_p->enb_ue_s1ap_id == initial_pP->enb_ue_s1ap_id) {
              // update ue_context, the only parameter to update is guti
              mme_ue_context_update_coll_keys( &mme_app_desc.mme_ue_contexts,
                ue_context_p,
                ue_context_p->enb_s1ap_id_key,
                ue_context_p->mme_ue_s1ap_id,
                ue_context_p->imsi,
                ue_context_p->mme_s11_teid,
                &guti);
            } else {
              OAILOG_DEBUG (LOG_MME_APP, "Received MME_APP_INITIAL_UE_MESSAGE from S1AP, previous conflicting S_TMSI context found with provided S_TMSI, GUMMEI\n");
            }
          } else {
            OAILOG_DEBUG (LOG_MME_APP, "Received MME_APP_INITIAL_UE_MESSAGE from S1AP, no previous context found with provided S_TMSI, GUMMEI\n");
          }
        }
      }
    } // no else actually TODO action
  }
  // finally create a new ue context if anything found
  if (!(ue_context_p)) {
    OAILOG_DEBUG (LOG_MME_APP, "UE context doesn't exist -> create one\n");
    if ((ue_context_p = mme_create_new_ue_context ()) == NULL) {
      /*
       * Error during ue context malloc
       */
      DevMessage ("mme_create_new_ue_context");
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
    ue_context_p->mme_ue_s1ap_id    = initial_pP->mme_ue_s1ap_id;
    ue_context_p->enb_ue_s1ap_id    = initial_pP->enb_ue_s1ap_id;
    MME_APP_ENB_S1AP_ID_KEY(ue_context_p->enb_s1ap_id_key, initial_pP->cgi.cell_identity.enb_id, initial_pP->enb_ue_s1ap_id);
    ue_context_p->sctp_assoc_id_key = initial_pP->sctp_assoc_id;

    DevAssert (mme_insert_ue_context (&mme_app_desc.mme_ue_contexts, ue_context_p) == 0);
  }
  ue_context_p->e_utran_cgi = initial_pP->cgi;

  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_INITIAL_UE_MESSAGE);
  // do this because of same message types name but not same struct in different .h
  message_p->ittiMsg.nas_initial_ue_message.nas.ue_id           = ue_context_p->mme_ue_s1ap_id;
  message_p->ittiMsg.nas_initial_ue_message.nas.tai             = initial_pP->tai;
  message_p->ittiMsg.nas_initial_ue_message.nas.cgi             = initial_pP->cgi;
  message_p->ittiMsg.nas_initial_ue_message.nas.as_cause        = initial_pP->rrc_establishment_cause;
  if (initial_pP->is_s_tmsi_valid) {
    message_p->ittiMsg.nas_initial_ue_message.nas.s_tmsi          = initial_pP->opt_s_tmsi;
  } else {
    message_p->ittiMsg.nas_initial_ue_message.nas.s_tmsi.mme_code = 0;
    message_p->ittiMsg.nas_initial_ue_message.nas.s_tmsi.m_tmsi   = INVALID_M_TMSI;
  }
  message_p->ittiMsg.nas_initial_ue_message.nas.initial_nas_msg   =  initial_pP->nas;

  initial_pP->nas = NULL;

  memcpy (&message_p->ittiMsg.nas_initial_ue_message.transparent, (const void*)&initial_pP->transparent, sizeof (message_p->ittiMsg.nas_initial_ue_message.transparent));

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_INITIAL_UE_MESSAGE");
  itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}


//------------------------------------------------------------------------------
void
mme_app_handle_delete_session_rsp (
  const itti_s11_delete_session_response_t * const delete_sess_resp_pP)
//------------------------------------------------------------------------------
{
  struct ue_context_s                    *ue_context_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (delete_sess_resp_pP );
  OAILOG_DEBUG (LOG_MME_APP, "Received S11_DELETE_SESSION_RESPONSE from S+P-GW with the ID " MME_UE_S1AP_ID_FMT "\n ",delete_sess_resp_pP->teid);
  ue_context_p = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, delete_sess_resp_pP->teid);

  if (ue_context_p == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 DELETE_SESSION_RESPONSE local S11 teid " TEID_FMT " ", delete_sess_resp_pP->teid);
    OAILOG_WARNING (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", delete_sess_resp_pP->teid);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  if (delete_sess_resp_pP->cause != REQUEST_ACCEPTED) {
    DevMessage ("Cases where bearer cause != REQUEST_ACCEPTED are not handled\n");
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 DELETE_SESSION_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
    delete_sess_resp_pP->teid, ue_context_p->imsi);
  /*
   * Updating statistics
   */
  mme_app_desc.mme_ue_contexts.nb_bearers_managed--;
  mme_app_desc.mme_ue_contexts.nb_bearers_since_last_stat--;

  /*
   * Remove UE Context
   */
  mme_app_itti_delete_session_rsp(ue_context_p->mme_ue_s1ap_id);
  mme_remove_ue_context(&mme_app_desc.mme_ue_contexts, ue_context_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}


//------------------------------------------------------------------------------
int
mme_app_handle_create_sess_resp (
  itti_s11_create_session_response_t * const create_sess_resp_pP)
{
  struct ue_context_s                    *ue_context_p = NULL;
  bearer_context_t                       *current_bearer_p = NULL;
  MessageDef                             *message_p = NULL;
  int16_t                                 bearer_id =0;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (create_sess_resp_pP );
  OAILOG_DEBUG (LOG_MME_APP, "Received S11_CREATE_SESSION_RESPONSE from S+P-GW\n");
  ue_context_p = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, create_sess_resp_pP->teid);

  if (ue_context_p == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_SESSION_RESPONSE local S11 teid " TEID_FMT " ", create_sess_resp_pP->teid);

    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", create_sess_resp_pP->teid);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_SESSION_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
    create_sess_resp_pP->teid, ue_context_p->imsi);

  /*
   * Store the S-GW teid
   */
  ue_context_p->sgw_s11_teid = create_sess_resp_pP->s11_sgw_teid.teid;
  //---------------------------------------------------------
  // Process itti_sgw_create_session_response_t.bearer_context_created
  //---------------------------------------------------------
  for (int i=0; i < create_sess_resp_pP->bearer_contexts_created.num_bearer_context; i++) {
    bearer_id = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].eps_bearer_id /* - 5 */ ;
    /*
     * Depending on s11 result we have to send reject or accept for bearers
     */
    DevCheck ((bearer_id < BEARERS_PER_UE) && (bearer_id >= 0), bearer_id, BEARERS_PER_UE, 0);

    if (create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].cause != REQUEST_ACCEPTED) {
      DevMessage ("Cases where bearer cause != REQUEST_ACCEPTED are not handled\n");
    }
    DevAssert (create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].s1u_sgw_fteid.interface_type == S1_U_SGW_GTP_U);
    current_bearer_p = &ue_context_p->eps_bearers[bearer_id];
    current_bearer_p->bearer_state = BEARER_STATE_SGW_CREATED;
    /*
     * Updating statistics
     */
    mme_app_desc.mme_ue_contexts.nb_bearers_managed++;
    mme_app_desc.mme_ue_contexts.nb_bearers_since_last_stat++;
    current_bearer_p->s_gw_teid = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].s1u_sgw_fteid.teid;
    FTEID_T_2_IP_ADDRESS_T(&create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].s1u_sgw_fteid, &current_bearer_p->s_gw_address);

    current_bearer_p->p_gw_teid = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].s5_s8_u_pgw_fteid.teid;
    memset (&current_bearer_p->p_gw_address, 0, sizeof (ip_address_t));

    if (create_sess_resp_pP->bearer_contexts_created.bearer_contexts[bearer_id].bearer_level_qos ) {
      current_bearer_p->qci                      = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->qci;
      current_bearer_p->priority_level           = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->pl;
      current_bearer_p->preemption_vulnerability = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->pvi;
      current_bearer_p->preemption_capability    = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->pci;
      // TODO check: free create_sess_resp_pP->bearer_contexts_created.bearer_contexts[bearer_id].bearer_level_qos
      OAILOG_DEBUG (LOG_MME_APP, "Set qci %u in bearer %u\n", current_bearer_p->qci, bearer_id);
    } else {
      // if null, it is not modified
      //current_bearer_p->qci                    = ue_context_p->pending_pdn_connectivity_req_qos.qci;
      //#pragma message  "may force QCI here to 9"
      current_bearer_p->qci                      = QCI_9;
      current_bearer_p->priority_level           = 1;
      current_bearer_p->preemption_vulnerability = PRE_EMPTION_VULNERABILITY_ENABLED;
      current_bearer_p->preemption_capability    = PRE_EMPTION_CAPABILITY_ENABLED;
      OAILOG_DEBUG (LOG_MME_APP, "Set qci %u in bearer %u (qos not modified by S/P-GW)\n", current_bearer_p->qci, bearer_id);
    }
  }
  mme_app_dump_ue_contexts (&mme_app_desc.mme_ue_contexts);
  {
    //uint8_t *keNB = NULL;
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_PDN_CONNECTIVITY_RSP);
    itti_nas_pdn_connectivity_rsp_t *nas_pdn_connectivity_rsp = &message_p->ittiMsg.nas_pdn_connectivity_rsp;
    // moved to NAS_CONNECTION_ESTABLISHMENT_CONF, keNB not handled in NAS MME
    //derive_keNB(ue_context_p->vector_in_use->kasme, 156, &keNB);
    //memcpy(NAS_PDN_CONNECTIVITY_RSP(message_p).keNB, keNB, 32);
    //free(keNB);
    nas_pdn_connectivity_rsp->pti = ue_context_p->pending_pdn_connectivity_req_pti;  // NAS internal ref
    nas_pdn_connectivity_rsp->ue_id = ue_context_p->pending_pdn_connectivity_req_ue_id;      // NAS internal ref

    // TO REWORK:
    if (ue_context_p->pending_pdn_connectivity_req_apn) {
      nas_pdn_connectivity_rsp->apn = bstrcpy (ue_context_p->pending_pdn_connectivity_req_apn);
      OAILOG_DEBUG (LOG_MME_APP, "SET APN FROM NAS PDN CONNECTIVITY CREATE: %s\n", bdata(nas_pdn_connectivity_rsp->apn));
    } else {
      int                                     i;
      context_identifier_t                    context_identifier = ue_context_p->apn_profile.context_identifier;

      for (i = 0; i < ue_context_p->apn_profile.nb_apns; i++) {
        if (ue_context_p->apn_profile.apn_configuration[i].context_identifier == context_identifier) {
          AssertFatal (ue_context_p->apn_profile.apn_configuration[i].service_selection_length > 0, "Bad APN string (len = 0)");

          if (ue_context_p->apn_profile.apn_configuration[i].service_selection_length > 0) {
            nas_pdn_connectivity_rsp->apn = blk2bstr(ue_context_p->apn_profile.apn_configuration[i].service_selection,
                ue_context_p->apn_profile.apn_configuration[i].service_selection_length);
            AssertFatal (ue_context_p->apn_profile.apn_configuration[i].service_selection_length <= ACCESS_POINT_NAME_MAX_LENGTH, "Bad APN string length %d",
                ue_context_p->apn_profile.apn_configuration[i].service_selection_length);

            OAILOG_DEBUG (LOG_MME_APP, "SET APN FROM HSS ULA: %s\n", bdata(nas_pdn_connectivity_rsp->apn));
            break;
          }
        }
      }
    }

    OAILOG_DEBUG (LOG_MME_APP, "APN: %s\n", bdata(nas_pdn_connectivity_rsp->apn));
    nas_pdn_connectivity_rsp->pdn_addr = paa_to_bstring(&create_sess_resp_pP->paa);
    nas_pdn_connectivity_rsp->pdn_type = create_sess_resp_pP->paa.pdn_type;
//#pragma message  "QOS hardcoded here"
    //memcpy(&NAS_PDN_CONNECTIVITY_RSP(message_p).qos,
    //        &ue_context_p->pending_pdn_connectivity_req_qos,
    //        sizeof(network_qos_t));

    // ASSUME NO HO now, so assume 1 bearer only and is default bearer
    bearer_id = 0;

    nas_pdn_connectivity_rsp->qos.gbrUL    = 64;        /*  64 = 64kb/s   Guaranteed Bit Rate for uplink   */
    nas_pdn_connectivity_rsp->qos.gbrDL    = 120;       /* 120 = 512kb/s  Guaranteed Bit Rate for downlink */
    nas_pdn_connectivity_rsp->qos.mbrUL    = 72;        /*  72 = 128kb/s  Maximum Bit Rate for uplink      */
    nas_pdn_connectivity_rsp->qos.mbrDL    = 135;       /* 135 = 1024kb/s Maximum Bit Rate for downlink    */
    nas_pdn_connectivity_rsp->qos.qci      = 9;         /* QoS Class Identifier                            */
    nas_pdn_connectivity_rsp->request_type = ue_context_p->pending_pdn_connectivity_req_request_type;        // NAS internal ref
    ue_context_p->pending_pdn_connectivity_req_request_type = 0;
    // here at this point OctetString are saved in resp, no loss of memory (apn, pdn_addr)
    nas_pdn_connectivity_rsp->ue_id                 = ue_context_p->mme_ue_s1ap_id;
    nas_pdn_connectivity_rsp->ebi                   = bearer_id;
    nas_pdn_connectivity_rsp->qci                   = ue_context_p->eps_bearers[bearer_id].qci;
    nas_pdn_connectivity_rsp->prio_level            = ue_context_p->eps_bearers[bearer_id].priority_level;
    nas_pdn_connectivity_rsp->pre_emp_vulnerability = ue_context_p->eps_bearers[bearer_id].preemption_vulnerability;
    nas_pdn_connectivity_rsp->pre_emp_capability    = ue_context_p->eps_bearers[bearer_id].preemption_capability;
    nas_pdn_connectivity_rsp->sgw_s1u_teid          = ue_context_p->eps_bearers[bearer_id].s_gw_teid;

    memcpy (&nas_pdn_connectivity_rsp->sgw_s1u_address, &ue_context_p->eps_bearers[bearer_id].s_gw_address, sizeof (ip_address_t));
    nas_pdn_connectivity_rsp->ambr.br_ul            = ue_context_p->subscribed_ambr.br_ul;
    nas_pdn_connectivity_rsp->ambr.br_dl            = ue_context_p->subscribed_ambr.br_dl;
    copy_protocol_configuration_options (&nas_pdn_connectivity_rsp->pco, &create_sess_resp_pP->pco);
    clear_protocol_configuration_options(&create_sess_resp_pP->pco);

    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_PDN_CONNECTIVITY_RSP sgw_s1u_teid %u ebi %u qci %u prio %u",
        ue_context_p->eps_bearers[bearer_id].s_gw_teid,
        bearer_id,
        ue_context_p->eps_bearers[bearer_id].qci,
        ue_context_p->eps_bearers[bearer_id].priority_level);

    rc = itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
  }
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}


//------------------------------------------------------------------------------
void
mme_app_handle_initial_context_setup_rsp (
  itti_mme_app_initial_context_setup_rsp_t * const initial_ctxt_setup_rsp_pP)
{
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  OAILOG_DEBUG (LOG_MME_APP, "Received MME_APP_INITIAL_CONTEXT_SETUP_RSP from S1AP\n");
  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, initial_ctxt_setup_rsp_pP->mme_ue_s1ap_id);

  if (ue_context_p == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: %08x %d(dec)\n", initial_ctxt_setup_rsp_pP->mme_ue_s1ap_id, initial_ctxt_setup_rsp_pP->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "MME_APP_INITIAL_CONTEXT_SETUP_RSP Unknown ue %u", initial_ctxt_setup_rsp_pP->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  message_p = itti_alloc_new_message (TASK_MME_APP, S11_MODIFY_BEARER_REQUEST);
  AssertFatal (message_p , "itti_alloc_new_message Failed");
  itti_s11_modify_bearer_request_t *s11_modify_bearer_request = &message_p->ittiMsg.s11_modify_bearer_request;
  s11_modify_bearer_request->peer_ip = mme_config.ipv4.sgw_s11;
  s11_modify_bearer_request->teid = ue_context_p->sgw_s11_teid;
  s11_modify_bearer_request->local_teid = ue_context_p->mme_s11_teid;
  /*
   * Delay Value in integer multiples of 50 millisecs, or zero
   */
  s11_modify_bearer_request->delay_dl_packet_notif_req = 0;  // TO DO

  for (int item = 0; item < initial_ctxt_setup_rsp_pP->no_of_e_rabs; item++) {
    s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[item].eps_bearer_id     = initial_ctxt_setup_rsp_pP->e_rab_id[item];
    s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[item].s1_eNB_fteid.teid = initial_ctxt_setup_rsp_pP->gtp_teid[item];
    s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[item].s1_eNB_fteid.interface_type    = S1_U_ENODEB_GTP_U;
    if (4 == blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item])) {
      s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv4         = 1;
      memcpy(&s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv4_address,
          initial_ctxt_setup_rsp_pP->transport_layer_address[item]->data, blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item]));
    } else if (16 == blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item])) {
      s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv6         = 1;
      memcpy(s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv6_address,
          initial_ctxt_setup_rsp_pP->transport_layer_address[item]->data, blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item]));
    } else {
      AssertFatal(0, "TODO IP address %d bytes", blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item]));
    }
    bdestroy_wrapper (&initial_ctxt_setup_rsp_pP->transport_layer_address[item]);
  }
  s11_modify_bearer_request->bearer_contexts_to_be_modified.num_bearer_context = initial_ctxt_setup_rsp_pP->no_of_e_rabs;

  s11_modify_bearer_request->bearer_contexts_to_be_removed.num_bearer_context = 0;

  s11_modify_bearer_request->mme_fq_csid.node_id_type = GLOBAL_UNICAST_IPv4; // TO DO
  s11_modify_bearer_request->mme_fq_csid.csid = 0;   // TO DO ...
  memset(&s11_modify_bearer_request->indication_flags, 0, sizeof(s11_modify_bearer_request->indication_flags));   // TO DO
  s11_modify_bearer_request->rat_type = RAT_EUTRAN;
  /*
   * S11 stack specific parameter. Not used in standalone epc mode
   */
  s11_modify_bearer_request->trxn = NULL;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME,  MSC_S11_MME ,
                      NULL, 0, "0 S11_MODIFY_BEARER_REQUEST teid %u ebi %u", s11_modify_bearer_request->teid,
                      s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].eps_bearer_id);
  itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_release_access_bearers_resp (
  const itti_s11_release_access_bearers_response_t * const rel_access_bearers_rsp_pP)
{
  MessageDef                             *message_p = NULL;
  struct ue_context_s                    *ue_context_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context_p = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, rel_access_bearers_rsp_pP->teid);

  if (ue_context_p == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 RELEASE_ACCESS_BEARERS_RESPONSE local S11 teid " TEID_FMT " ",
    		rel_access_bearers_rsp_pP->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %" PRIX32 "\n", rel_access_bearers_rsp_pP->teid);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 RELEASE_ACCESS_BEARERS_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
    rel_access_bearers_rsp_pP->teid, ue_context_p->imsi);

  message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_UE_CONTEXT_RELEASE_COMMAND);
  AssertFatal (message_p , "itti_alloc_new_message Failed");
  S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).mme_ue_s1ap_id = ue_context_p->mme_ue_s1ap_id;
  S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).enb_ue_s1ap_id = ue_context_p->enb_ue_s1ap_id;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0, "0 S1AP_UE_CONTEXT_RELEASE_COMMAND mme_ue_s1ap_id %06" PRIX32 " ",
      S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).mme_ue_s1ap_id);
  int to_task = (RUN_MODE_SCENARIO_PLAYER == mme_config.run_mode) ? TASK_MME_SCENARIO_PLAYER:TASK_S1AP;
  itti_send_msg_to_task (to_task, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_create_bearer_req (
    const itti_s11_create_bearer_request_t * const create_bearer_request_pP)
{
  MessageDef                             *message_p = NULL;
  struct ue_context_s                    *ue_context_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context_p = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, create_bearer_request_pP->teid);

  if (ue_context_p == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_BEARERS_REQUEST local S11 teid " TEID_FMT " ",
        create_bearer_request_pP->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %" PRIX32 "\n", create_bearer_request_pP->teid);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_BEARERS_REQUEST local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      create_bearer_request_pP->teid, ue_context_p->imsi);

  /*message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_UE_CONTEXT_RELEASE_COMMAND);
  AssertFatal (message_p , "itti_alloc_new_message Failed");
  S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).mme_ue_s1ap_id = ue_context_p->mme_ue_s1ap_id;
  S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).enb_ue_s1ap_id = ue_context_p->enb_ue_s1ap_id;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0, "0 S1AP_UE_CONTEXT_RELEASE_COMMAND mme_ue_s1ap_id %06" PRIX32 " ",
      S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).mme_ue_s1ap_id);
  int to_task = (RUN_MODE_SCENARIO_PLAYER == mme_config.run_mode) ? TASK_MME_SCENARIO_PLAYER:TASK_S1AP;
  itti_send_msg_to_task (to_task, INSTANCE_DEFAULT, message_p);*/

  OAILOG_FUNC_OUT (LOG_MME_APP);
}


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
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "mme_app_apn_selection.h"
#include "mme_app_pdn_context.h"
#include "mme_app_wrr_selection.h"
#include "mme_app_bearer_context.h"
#include "timer.h"
#include "common_defs.h"
#include "gcc_diag.h"
#include "mme_app_itti_messaging.h"
#include "mme_app_procedures.h"
#include "xml_msg_dump_itti.h"
#include "s1ap_mme.h"
#include "s1ap_mme_ta.h"

//----------------------------------------------------------------------------
// todo: check which one needed
static void mme_app_send_s1ap_path_switch_request_acknowledge(mme_ue_s1ap_id_t mme_ue_s1ap_id);

static void mme_app_send_s1ap_path_switch_request_failure(mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id, sctp_assoc_id_t assoc_id, enum s1cause cause);
static
void mme_app_send_s10_forward_relocation_response_err(teid_t mme_source_s10_teid, struct in_addr mme_source_ipv4_address, void *trxn,  gtpv2c_cause_value_t gtpv2cCause);

static
void mme_app_send_s1ap_handover_request(mme_ue_s1ap_id_t mme_ue_s1ap_id,
    bearer_contexts_to_be_created_t *bcs_tbc,
    uint32_t                enb_id,
    uint16_t                encryption_algorithm_capabilities,
    uint16_t                integrity_algorithm_capabilities,
    uint8_t                 nh[AUTH_NH_SIZE],
    uint8_t                 ncc,
    bstring                 eutran_source_to_target_container);

/** External definitions in MME_APP UE Data Context. */
extern void mme_app_set_ue_eps_mm_context(mm_context_eps_t * ue_eps_mme_context_p, struct ue_context_s *ue_context, emm_data_context_t *ue_nas_ctx);
extern void mme_app_set_pdn_connections(struct mme_ue_eps_pdn_connections_s * pdn_connections, struct ue_context_s * ue_context);
//extern void mme_app_handle_pending_pdn_connectivity_information(ue_context_t *ue_context, pdn_connection_t * pdn_conn_pP);
extern pdn_context_t * mme_app_handle_pdn_connectivity_from_s10(ue_context_t *ue_context, pdn_connection_t * pdn_connection);

//------------------------------------------------------------------------------
static bool mme_app_construct_guti(const plmn_t * const plmn_p, const s_tmsi_t * const s_tmsi_p,  guti_t * const guti_p)
{
  /*
   * This is a helper function to construct GUTI from S-TMSI. It uses PLMN id and MME Group Id of the serving MME for
   * this purpose.
   *
   */

  bool                                    is_guti_valid = false; // Set to true if serving MME is found and GUTI is constructed
  uint8_t                                 num_mme       = 0;     // Number of configured MME in the MME pool
  guti_p->m_tmsi = s_tmsi_p->m_tmsi;
  guti_p->gummei.mme_code = s_tmsi_p->mme_code;
  // Create GUTI by using PLMN Id and MME-Group Id of serving MME
  OAILOG_DEBUG (LOG_MME_APP,
      "Construct GUTI using S-TMSI received form UE and MME Group Id and PLMN id from MME Conf: %u, %u \n",
      s_tmsi_p->m_tmsi, s_tmsi_p->mme_code);
  mme_config_read_lock (&mme_config);
  /*
   * Check number of MMEs in the pool.
   * At present it is assumed that one MME is supported in MME pool but in case there are more
   * than one MME configured then search the serving MME using MME code.
   * Assumption is that within one PLMN only one pool of MME will be configured
   */
  if (mme_config.gummei.nb > 1)
  {
    OAILOG_DEBUG (LOG_MME_APP, "More than one MMEs are configured. \n");
  }
  for (num_mme = 0; num_mme < mme_config.gummei.nb; num_mme++)
  {
    /*Verify that the MME code within S-TMSI is same as what is configured in MME conf*/
    if ((plmn_p->mcc_digit2 == mme_config.gummei.gummei[num_mme].plmn.mcc_digit2) &&
        (plmn_p->mcc_digit1 == mme_config.gummei.gummei[num_mme].plmn.mcc_digit1) &&
        (plmn_p->mnc_digit3 == mme_config.gummei.gummei[num_mme].plmn.mnc_digit3) &&
        (plmn_p->mcc_digit3 == mme_config.gummei.gummei[num_mme].plmn.mcc_digit3) &&
        (plmn_p->mnc_digit2 == mme_config.gummei.gummei[num_mme].plmn.mnc_digit2) &&
        (plmn_p->mnc_digit1 == mme_config.gummei.gummei[num_mme].plmn.mnc_digit1) &&
        (guti_p->gummei.mme_code == mme_config.gummei.gummei[num_mme].mme_code))
    {
      break;
    }
  }
  if (num_mme >= mme_config.gummei.nb)
  {
    OAILOG_DEBUG (LOG_MME_APP, "No MME serves this UE");
  }
  else
  {
    guti_p->gummei.plmn = mme_config.gummei.gummei[num_mme].plmn;
    guti_p->gummei.mme_gid = mme_config.gummei.gummei[num_mme].mme_gid;
    is_guti_valid = true;
  }
  mme_config_unlock (&mme_config);
  return is_guti_valid;
}

//------------------------------------------------------------------------------
static
int mme_app_compare_plmn (
  const plmn_t * const plmn)
{
  int                                     i = 0;
  uint16_t                                mcc = 0;
  uint16_t                                mnc = 0;
  uint16_t                                mnc_len = 0;

  DevAssert (plmn != NULL);
  /** Get the integer values from the PLMN. */
  PLMN_T_TO_MCC_MNC ((*plmn), mcc, mnc, mnc_len);

  mme_config_read_lock (&mme_config);

  for (i = 0; i < mme_config.served_tai.nb_tai; i++) {
    OAILOG_TRACE (LOG_MME_APP, "Comparing plmn_mcc %d/%d, plmn_mnc %d/%d plmn_mnc_len %d/%d\n",
        mme_config.served_tai.plmn_mcc[i], mcc, mme_config.served_tai.plmn_mnc[i], mnc, mme_config.served_tai.plmn_mnc_len[i], mnc_len);

    if ((mme_config.served_tai.plmn_mcc[i] == mcc) &&
        (mme_config.served_tai.plmn_mnc[i] == mnc) &&
        (mme_config.served_tai.plmn_mnc_len[i] == mnc_len))
      /*
       * There is a matching plmn
       */
      return TA_LIST_AT_LEAST_ONE_MATCH;
  }

  mme_config_unlock (&mme_config);
  return TA_LIST_NO_MATCH;
}

//------------------------------------------------------------------------------
/* @brief compare a TAC
*/
static
int mme_app_compare_tac (
  uint16_t tac_value)
{
  int                                     i = 0;

  mme_config_read_lock (&mme_config);

  for (i = 0; i < mme_config.served_tai.nb_tai; i++) {
    OAILOG_TRACE (LOG_MME_APP, "Comparing config tac %d, received tac = %d\n", mme_config.served_tai.tac[i], tac_value);

    if (mme_config.served_tai.tac[i] == tac_value)
      return TA_LIST_AT_LEAST_ONE_MATCH;
  }

  mme_config_unlock (&mme_config);
  return TA_LIST_NO_MATCH;
}

//------------------------------------------------------------------------------
static
 bool mme_app_check_ta_local(const plmn_t * target_plmn, const tac_t target_tac){
  if(TA_LIST_AT_LEAST_ONE_MATCH == mme_app_compare_plmn(target_plmn)){
    if(TA_LIST_AT_LEAST_ONE_MATCH == mme_app_compare_tac(target_tac)){
      OAILOG_DEBUG (LOG_MME_APP, "TAC and PLMN are matching. \n");
      return true;
    }
  }
  OAILOG_DEBUG (LOG_MME_APP, "TAC or PLMN are not matching. \n");
  return;
}

//------------------------------------------------------------------------------
void mme_app_handle_s1ap_enb_deregistered_ind (const itti_s1ap_eNB_deregistered_ind_t * const enb_dereg_ind)
{
  for (int ue_idx = 0; ue_idx < enb_dereg_ind->nb_ue_to_deregister; ue_idx++) {
    struct ue_context_s *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, enb_dereg_ind->mme_ue_s1ap_id[ue_idx]);

    if (ue_context) {
      ue_context->ecm_state = ECM_IDLE;
      mme_app_send_nas_signalling_connection_rel_ind(enb_dereg_ind->mme_ue_s1ap_id[ue_idx]);
    }
  }
}

//------------------------------------------------------------------------------
int
mme_app_handle_nas_pdn_connectivity_req (
  itti_nas_pdn_connectivity_req_t * const nas_pdn_connectivity_req_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    *ue_context   = NULL;
  imsi64_t                                imsi64 = INVALID_IMSI64;
  int                                     rc = RETURNok;
  emm_data_context_t                     *emm_context = NULL;
  pdn_context_t                          *pdn_context = NULL;

  DevAssert (nas_pdn_connectivity_req_pP );
//  IMSI_STRING_TO_IMSI64 ((char *)nas_pdn_connectivity_req_pP->imsi, &imsi64);
//  OAILOG_DEBUG (LOG_MME_APP, "Received NAS_PDN_CONNECTIVITY_REQ from NAS Handling imsi " IMSI_64_FMT "\n", imsi64);
  imsi64 = nas_pdn_connectivity_req_pP->imsi;
  if ((ue_context = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, imsi64)) == NULL) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " NAS_PDN_CONNECTIVITY_REQ Unknown imsi " IMSI_64_FMT, imsi64);
    OAILOG_ERROR (LOG_MME_APP, "That's embarrassing as we don't know this IMSI\n");
    mme_ue_context_dump_coll_keys();
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  DevAssert((emm_context = emm_data_context_get(&_emm_data, ue_context->mme_ue_s1ap_id)));

  // ...
  ue_context->imsi_auth = IMSI_AUTHENTICATED;

  // todo: sending requested PCOs
  //  copy_protocol_configuration_options (&ue_context->pending_pdn_connectivity_req_pco, &nas_pdn_connectivity_req_pP->pco);
  //  clear_protocol_configuration_options(&nas_pdn_connectivity_req_pP->pco);
  //#define TEMPORARY_DEBUG 1
  //#if TEMPORARY_DEBUG
  //  bstring b = protocol_configuration_options_to_xml(&ue_context->pending_pdn_connectivity_req_pco);
  //  OAILOG_DEBUG (LOG_MME_APP, "PCO %s\n", bdata(b));
  //  bdestroy_wrapper(&b);
  //#endif

  mme_app_get_pdn_context(ue_context, nas_pdn_connectivity_req_pP->pdn_cid, nas_pdn_connectivity_req_pP->default_ebi, nas_pdn_connectivity_req_pP->apn, &pdn_context);
  if(!pdn_context){
    OAILOG_ERROR (LOG_MME_APP, "No PDN context found for pdn_cid %d for UE " MME_UE_S1AP_ID_FMT ". \n", nas_pdn_connectivity_req_pP->pdn_cid, nas_pdn_connectivity_req_pP->ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  // todo: get target_tai or so from ue_context!!
  rc = mme_app_send_s11_create_session_req (ue_context, &nas_pdn_connectivity_req_pP->_imsi, pdn_context, &emm_context->originating_tai);

  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
int
mme_app_handle_nas_pdn_disconnect_req (
  itti_nas_pdn_disconnect_req_t * const nas_pdn_disconnect_req_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    *ue_context  = NULL;
  struct pdn_context_s                   *pdn_context = NULL;
  int                                     rc = RETURNok;

  DevAssert (nas_pdn_disconnect_req_pP );

  if ((ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, nas_pdn_disconnect_req_pP->ue_id)) == NULL) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " NAS_PDN_DISCONNECT_REQ Unknown ueId" MME_UE_S1AP_ID_FMT, nas_pdn_disconnect_req_pP->ue_id);
    OAILOG_ERROR (LOG_MME_APP, "That's embarrassing as we don't know this UeId " MME_UE_S1AP_ID_FMT". \n", nas_pdn_disconnect_req_pP->ue_id);
    mme_ue_context_dump_coll_keys();
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /** Check that the PDN session exist. */
//  pdn_context_t *pdn_context = mme_app_get_pdn_context(ue_context, nas_pdn_disconnect_req_pP->pdn_cid, nas_pdn_disconnect_req_pP->default_ebi, NULL);
//  if(!pdn_context){
//    OAILOG_ERROR (LOG_MME_APP, "We could not find the PDN context with pci %d for ueId " MME_UE_S1AP_ID_FMT". \n", nas_pdn_disconnect_req_pP->pdn_cid, nas_pdn_disconnect_req_pP->ue_id);
//    mme_ue_context_dump_coll_keys();
//    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
//  }
//
//  bearer_context_t * bearer_context_to_deactivate = NULL;
//  RB_FOREACH (bearer_context_to_deactivate, BearerPool, &ue_context->bearer_pool) {
//    DevAssert(bearer_context_to_deactivate->bearer_state == BEARER_STATE_ACTIVE); // Cannot be in IDLE mode?
//  }

//  mme_app_get_pdn_context(ue_context, nas_pdn_disconnect_req_pP->pdn_cid, nas_pdn_disconnect_req_pP->default_ebi, nas_pdn_disconnect_req_pP->apn,  &pdn_context);
//  if(!pdn_context){
//    OAILOG_ERROR (LOG_MME_APP, "No PDN context found for pdn_cid %d for UE " MME_UE_S1AP_ID_FMT ". \n", nas_pdn_disconnect_req_pP->pdn_cid, nas_pdn_disconnect_req_pP->ue_id);
//    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
//  }

  /** Don't change the bearer state. Send Delete Session Request to SAE-GW. No transaction needed. */
  rc =  mme_app_send_delete_session_request(ue_context, nas_pdn_disconnect_req_pP->default_ebi, nas_pdn_disconnect_req_pP->saegw_s11_ip_addr, nas_pdn_disconnect_req_pP->saegw_s11_teid);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

// sent by NAS
//------------------------------------------------------------------------------
void
mme_app_handle_conn_est_cnf (
  itti_nas_conn_est_cnf_t * const nas_conn_est_cnf_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p = NULL;
  itti_mme_app_connection_establishment_cnf_t *establishment_cnf_p = NULL;

  OAILOG_DEBUG (LOG_MME_APP, "Received NAS_CONNECTION_ESTABLISHMENT_CNF from NAS\n");
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, nas_conn_est_cnf_pP->ue_id);

  if (ue_context == NULL) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " NAS_CONNECTION_ESTABLISHMENT_CNF Unknown ue " MME_UE_S1AP_ID_FMT " ", nas_conn_est_cnf_pP->ue_id);
    OAILOG_ERROR (LOG_MME_APP, "UE context doesn't exist for UE " MME_UE_S1AP_ID_FMT "\n", nas_conn_est_cnf_pP->ue_id);
    // memory leak
//    bdestroy_wrapper(&nas_conn_est_cnf_pP->nas_msg);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  message_p = itti_alloc_new_message (TASK_MME_APP, MME_APP_CONNECTION_ESTABLISHMENT_CNF);
  establishment_cnf_p = &message_p->ittiMsg.mme_app_connection_establishment_cnf;

  establishment_cnf_p->ue_id = nas_conn_est_cnf_pP->ue_id;  /**< Just set the MME_UE_S1AP_ID as identifier, the S1AP layer will set the enb_ue_s1ap_id from the ue_reference. */

  /*
   * Add the subscribed UE-AMBR values.
   */
  //#pragma message  "Check ue_context ambr"
  establishment_cnf_p->ue_ambr.br_ul = ue_context->subscribed_ue_ambr.br_ul;
  establishment_cnf_p->ue_ambr.br_dl = ue_context->subscribed_ue_ambr.br_dl;
  /*
   * Add the Security capabilities.
   */
  establishment_cnf_p->ue_security_capabilities_encryption_algorithms = nas_conn_est_cnf_pP->encryption_algorithm_capabilities;
  establishment_cnf_p->ue_security_capabilities_integrity_algorithms = nas_conn_est_cnf_pP->integrity_algorithm_capabilities;
  memcpy(establishment_cnf_p->kenb, nas_conn_est_cnf_pP->kenb, AUTH_KASME_SIZE);
  OAILOG_DEBUG (LOG_MME_APP, "security_capabilities_encryption_algorithms 0x%04X\n", establishment_cnf_p->ue_security_capabilities_encryption_algorithms);
  OAILOG_DEBUG (LOG_MME_APP, "security_capabilities_integrity_algorithms  0x%04X\n", establishment_cnf_p->ue_security_capabilities_integrity_algorithms);

  /*
   * The default bearer contains the EMM message with the Attach/TAU-Accept method.
   * The rest contain ESM messages with activate dedicated EPS Bearer Context Request messages!
   * (Implemented correctly below but ESM information fail).
   *
   * The correct mapping must be made from each NAS message to each bearer context.
   * Currently, just set the default bearer.
   *
   * No inner NAS message exists.
   * todo: later also add an array or a list of NAS messages in the nas_itti_establish_cnf
   */
  pdn_context_t * first_pdn = RB_MIN(PdnContexts, &ue_context->pdn_contexts);
  DevAssert(first_pdn);

  bearer_context_t * first_bearer = RB_MIN(SessionBearers, &first_pdn->session_bearers); // todo: @ handover (idle mode tau) this should give us the default ebi!
  if(first_bearer){ // todo: optimize this!
//    if ((BEARER_STATE_SGW_CREATED  || BEARER_STATE_S1_RELEASED) & first_bearer->bearer_state) {    /**< It could be in IDLE mode. */
      establishment_cnf_p->e_rab_id[0]                                 = first_bearer->ebi ;//+ EPS_BEARER_IDENTITY_FIRST;
      establishment_cnf_p->e_rab_level_qos_qci[0]                      = first_bearer->qci;
      establishment_cnf_p->e_rab_level_qos_priority_level[0]           = first_bearer->priority_level;
      establishment_cnf_p->e_rab_level_qos_preemption_capability[0]    = first_bearer->preemption_capability;
      establishment_cnf_p->e_rab_level_qos_preemption_vulnerability[0] = first_bearer->preemption_vulnerability;
      establishment_cnf_p->transport_layer_address[0]                  = fteid_ip_address_to_bstring(&first_bearer->s_gw_fteid_s1u);
      establishment_cnf_p->gtp_teid[0]                                 = first_bearer->s_gw_fteid_s1u.teid;
//      if (!j) { // todo: ESM message may exist --> should match each to the EBI!
      establishment_cnf_p->nas_pdu[0]                                  = nas_conn_est_cnf_pP->nas_msg;
      nas_conn_est_cnf_pP->nas_msg = NULL; /**< Unlink. */
//    }
  }
  establishment_cnf_p->no_of_e_rabs = 1;

  // Copy UE radio capabilities into message if it exists (todo).
//  OAILOG_DEBUG (LOG_MME_APP, "UE radio context already cached: %s\n",
//      ue_context->ue_radio_capabilit_length ? "yes" : "no");
//  establishment_cnf_p->ue_radio_cap_length = ue_context->ue_radio_cap_length;
//  if (establishment_cnf_p->ue_radio_cap_length) {
//    establishment_cnf_p->ue_radio_capabilities =
//        (uint8_t*) calloc (establishment_cnf_p->ue_radio_cap_length, sizeof *establishment_cnf_p->ue_radio_capabilities);
//    memcpy (establishment_cnf_p->ue_radio_capabilities,
//        ue_context->ue_radio_capabilities,
//        establishment_cnf_p->ue_radio_cap_length);
//  }

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

  /*
   * UE is already in ECM_Connected state.
   * */
  if(ue_context->ecm_state == ECM_CONNECTED){
    /** UE is in connected state. Send S1AP message. */


  }else{
    OAILOG_ERROR (LOG_MME_APP, "EMM UE context should be in connected state for UE id %d, insted idle (initial_ctx_setup_cnf). \n", ue_context->mme_ue_s1ap_id);

  }

  /*
   * Move the UE to ECM Connected State.However if S1-U bearer establishment fails then we need to move the UE to idle.
   * S1 Signaling connection gets established via first uplink context establishment (attach, servReq, tau) message, or via inter-MME handover.
   * @Intra-MME handover, the ECM state should stay as ECM_CONNECTED.
   */

  /* Start timer to wait for Initial UE Context Response from eNB
   * If timer expires treat this as failure of ongoing procedure and abort corresponding NAS procedure such as ATTACH
   * or SERVICE REQUEST. Send UE context release command to eNB
   */
  if (timer_setup (ue_context->initial_context_setup_rsp_timer.sec, 0,
      TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *) &(ue_context->mme_ue_s1ap_id), &(ue_context->initial_context_setup_rsp_timer.id)) < 0) {
    OAILOG_ERROR (LOG_MME_APP, "Failed to start initial context setup response timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
    ue_context->initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  } else {
    OAILOG_DEBUG (LOG_MME_APP, "MME APP : Sent Initial context Setup Request and Started guard timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

// sent by S1AP
//------------------------------------------------------------------------------
void
mme_app_handle_initial_ue_message (
  itti_s1ap_initial_ue_message_t * const initial_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p = NULL;
  bool                                    is_guti_valid = false;
  emm_data_context_t                     *ue_nas_ctx = NULL;
  enb_s1ap_id_key_t                       enb_s1ap_id_key = 0;
  void                                   *id = NULL;

  OAILOG_DEBUG (LOG_MME_APP, "Received MME_APP_INITIAL_UE_MESSAGE from S1AP\n");
  XML_MSG_DUMP_ITTI_S1AP_INITIAL_UE_MESSAGE(initial_pP, TASK_S1AP, TASK_MME_APP, NULL);

  MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, initial_pP->ecgi.cell_identity.enb_id, initial_pP->enb_ue_s1ap_id);

  /*
   * Check if there is any existing UE context using S-TMSI/GUTI. If so continue with its MME_APP context.
   * MME_UE_S1AP_ID is the main key.
   */
  if (initial_pP->is_s_tmsi_valid)
  {
    OAILOG_DEBUG (LOG_MME_APP, "INITIAL UE Message: Valid mme_code %u and S-TMSI %u received from eNB.\n",
        initial_pP->opt_s_tmsi.mme_code, initial_pP->opt_s_tmsi.m_tmsi);
    guti_t guti = {.gummei.plmn = {0}, .gummei.mme_gid = 0, .gummei.mme_code = 0, .m_tmsi = INVALID_M_TMSI};
    is_guti_valid = mme_app_construct_guti(&(initial_pP->tai.plmn),&(initial_pP->opt_s_tmsi),&guti);
    if (is_guti_valid)  /**< Can the GUTI belong to this MME. */
    {
      ue_nas_ctx = emm_data_context_get_by_guti (&_emm_data, &guti);
      if (ue_nas_ctx)
      {
        // Get the UE context using mme_ue_s1ap_id
        ue_context =  mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_nas_ctx->ue_id);
        DevAssert(ue_context != NULL);
        if ((ue_context != NULL) && (ue_context->mme_ue_s1ap_id == ue_nas_ctx->ue_id)) {
          initial_pP->mme_ue_s1ap_id = ue_nas_ctx->ue_id;
          if (ue_context->enb_s1ap_id_key != INVALID_ENB_UE_S1AP_ID_KEY)
          {
            /*
             * Check if there is a handover process (& pending removal) of the object.
             * UE has to be in REGISTERED state. Then we can abort the handover procedure (clean the CLR flag) and continue with the initial request.
             *
             * This might happen if the handover to the target side failed and UE is doing an idle-TAU back to the source MME.
             * In that case, the NAS validation will reject the establishment of the NAS request (send just a TAU-Reject back).
             * It should then remove the MME_APP context.
             */
            mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
            if(s10_handover_proc){
              if(ue_context->mm_state == UE_REGISTERED){
                if(s10_handover_proc->pending_clear_location_request){
                  OAILOG_INFO(LOG_MME_APP, "UE_CONTEXT has a handover procedure active with CLR flag set for UE with IMSI " IMSI_64_FMT " and " MME_UE_S1AP_ID_FMT ". "
                      "Aborting the handover procedure and thereby resetting the CLR flag. \n",
                      ue_context->imsi, ue_context->mme_ue_s1ap_id);
                }
                /*
                 * Abort the handover procedure by just deleting it. It should remove the timers, if they exist.
                 * The S1AP UE Reference timer should be independent, run and remove the old ue_referece (cannot use it anymore).
                 */
                mme_app_delete_s10_procedure_mme_handover(ue_context);
              }else{
                OAILOG_INFO(LOG_MME_APP, "UE_CONTEXT has a handover procedure active but is not REGISTERED (flag set for UE with IMSI " IMSI_64_FMT " and " MME_UE_S1AP_ID_FMT ". "
                    "Dropping the received initial context request message (attach should be possible after timeout of procedure has occurred). \n",
                    ue_context->imsi, ue_context->mme_ue_s1ap_id);
                DevAssert(s10_handover_proc->proc.timer.id != MME_APP_TIMER_INACTIVE_ID);
                /*
                 * Error during ue context malloc.
                 * todo: removing the UE reference?!
                 */
                hashtable_rc_t result_deletion = hashtable_ts_remove (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl,
                    (const hash_key_t)enb_s1ap_id_key, (void **)&id);
                OAILOG_ERROR (LOG_MME_APP, "MME_APP_INITAIL_UE_MESSAGE. ERROR***** enb_s1ap_id_key %ld has valid value %ld. Result of deletion %d.\n" ,
                    enb_s1ap_id_key,
                    initial_pP->enb_ue_s1ap_id,
                    result_deletion);
                OAILOG_ERROR (LOG_MME_APP, "Failed to create new MME UE context enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT "\n", initial_pP->enb_ue_s1ap_id);
                OAILOG_FUNC_OUT (LOG_MME_APP);
              }
            }else{
              /** Just continue (might be a battery reject). */
              OAILOG_INFO(LOG_MME_APP, "No handover procedure for valid/registered UE_CONTEXT with IMSI " IMSI_64_FMT " and " MME_UE_S1AP_ID_FMT ". "
                  "Continuing to handle it (might be a battery reject). \n", ue_context->imsi, ue_context->mme_ue_s1ap_id);
            }
            /*
             * This only removed the MME_UE_S1AP_ID from enb_s1ap_id_key, it won't remove the UE_REFERENCE itself.
             * todo: @ lionel:           duplicate_enb_context_detected  flag is not checked anymore (NAS).
             */
            ue_description_t * old_ue_reference = s1ap_is_enb_ue_s1ap_id_in_list_per_enb(ue_context->enb_ue_s1ap_id, ue_context->e_utran_cgi.cell_identity.enb_id);
            if(old_ue_reference){
              OAILOG_ERROR (LOG_MME_APP, "MME_APP_INITAIL_UE_MESSAGE. ERROR***** Found an old UE_REFERENCE with enbUeS1apId " ENB_UE_S1AP_ID_FMT " and enbId %d.\n" ,
                  old_ue_reference->enb_ue_s1ap_id, ue_context->e_utran_cgi.cell_identity.enb_id);
              s1ap_remove_ue(old_ue_reference);
              OAILOG_WARNING (LOG_MME_APP, "MME_APP_INITAIL_UE_MESSAGE. ERROR***** Removed old UE_REFERENCE with enbUeS1apId " ENB_UE_S1AP_ID_FMT " and enbId %d.\n" ,
                  old_ue_reference->enb_ue_s1ap_id, ue_context->e_utran_cgi.cell_identity.enb_id);
            }
            /*
             * Ideally this should never happen. When UE move to IDLE this key is set to INVALID.
             * Note - This can happen if eNB detects RLF late and by that time UE sends Initial NAS message via new RRC
             * connection.
             * However if this key is valid, remove the key from the hashtable.
             */
            hashtable_rc_t result_deletion = hashtable_ts_remove (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl,
                (const hash_key_t)ue_context->enb_s1ap_id_key, (void **)&id);
            OAILOG_ERROR (LOG_MME_APP, "MME_APP_INITAIL_UE_MESSAGE. ERROR***** enb_s1ap_id_key %ld has valid value %ld. Result of deletion %d.\n" ,
                ue_context->enb_s1ap_id_key,
                ue_context->enb_ue_s1ap_id,
                result_deletion);
            ue_context->enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;
          }
          // Update MME UE context with new enb_ue_s1ap_id
          ue_context->enb_ue_s1ap_id = initial_pP->enb_ue_s1ap_id;
          // regenerate the enb_s1ap_id_key as enb_ue_s1ap_id is changed.
          // todo: also here home_enb_id
          // Update enb_s1ap_id_key in hashtable
          mme_ue_context_update_coll_keys( &mme_app_desc.mme_ue_contexts,
              ue_context,
              enb_s1ap_id_key,  /**< Generated first. */
              ue_nas_ctx->ue_id,
              ue_nas_ctx->_imsi64,
              ue_context->mme_teid_s11,
              ue_context->local_mme_teid_s10,
              &guti);
          /** Set the UE in ECM-Connected state. */
          // todo: checking before
          ue_context->ecm_state         = ECM_CONNECTED;

        }
      } else {
        OAILOG_DEBUG (LOG_MME_APP, "MME_APP_INITIAL_UE_MESSAGE with mme code %u and S-TMSI %u:"
            "no EMM UE context found \n", initial_pP->opt_s_tmsi.mme_code, initial_pP->opt_s_tmsi.m_tmsi);
        /** Check that also no MME_APP UE context exists for the given GUTI. */
        // todo: check
        if(mme_ue_context_exists_guti(&mme_app_desc.mme_ue_contexts, &guti) != NULL){
          OAILOG_DEBUG (LOG_MME_APP, "ERROR: UE EXIST WITH GUTI!\n.");
          OAILOG_FUNC_OUT (LOG_MME_APP);
        }
      }
    } else {
      OAILOG_DEBUG (LOG_MME_APP, "No MME is configured with MME code %u received in S-TMSI %u from UE.\n",
          initial_pP->opt_s_tmsi.mme_code, initial_pP->opt_s_tmsi.m_tmsi);
      DevAssert(mme_ue_context_exists_guti(&mme_app_desc.mme_ue_contexts, &guti) == NULL);
    }
  } else {
    OAILOG_DEBUG (LOG_MME_APP, "MME_APP_INITIAL_UE_MESSAGE from S1AP,without S-TMSI. \n"); /**< Continue with new UE context establishment. */
  }

  /*
   * Either we take the existing UE context or create a new one.
   * If a new one is created, it might be a duplicate one, which will be found out in the EMM validations.
   * There we remove the duplicate context synchronously. // todo: not synchronously yet
   */
  if (!(ue_context)) {
    OAILOG_DEBUG (LOG_MME_APP, "UE context doesn't exist -> create one \n");
    if (!(ue_context = mme_create_new_ue_context ())) {
      /*
       * Error during UE context malloc.
       * todo: removing the UE reference?!
       */
      hashtable_rc_t result_deletion = hashtable_ts_remove (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl,
          (const hash_key_t)ue_context->enb_s1ap_id_key, (void **)&id);
      OAILOG_ERROR (LOG_MME_APP, "MME_APP_INITAIL_UE_MESSAGE. ERROR***** enb_s1ap_id_key %ld has valid value %ld. Result of deletion %d.\n" ,
          ue_context->enb_s1ap_id_key,
          ue_context->enb_ue_s1ap_id,
          result_deletion);
      OAILOG_ERROR (LOG_MME_APP, "Failed to create new MME UE context enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT "\n", initial_pP->enb_ue_s1ap_id);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }

    /** Initialize the fields of the MME_APP context. */
    ue_context->ecm_state         = ECM_CONNECTED;
    ue_context->mme_ue_s1ap_id    = INVALID_MME_UE_S1AP_ID;
    ue_context->enb_ue_s1ap_id    = initial_pP->enb_ue_s1ap_id;
    // todo: check if this works for home and macro enb id
    MME_APP_ENB_S1AP_ID_KEY(ue_context->enb_s1ap_id_key, initial_pP->ecgi.cell_identity.enb_id, initial_pP->enb_ue_s1ap_id);
    ue_context->sctp_assoc_id_key = initial_pP->sctp_assoc_id;
    OAILOG_DEBUG (LOG_MME_APP, "Created new MME UE context enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT "\n", initial_pP->enb_ue_s1ap_id);
    /** Since the NAS and MME_APP contexts are split again, we assign a new mme_ue_s1ap_id here. */

//    uintptr_t bearer_context_2 = mme_app_get_ue_bearer_context_2(ue_context, 5);


    ue_context->mme_ue_s1ap_id    = mme_app_ctx_get_new_ue_id ();
    if (ue_context->mme_ue_s1ap_id  == INVALID_MME_UE_S1AP_ID) {
      OAILOG_CRITICAL (LOG_MME_APP, "MME_APP_INITIAL_UE_MESSAGE. MME_UE_S1AP_ID allocation Failed.\n");
      mme_app_ue_context_free_content(ue_context);
      free_wrapper ((void**)&ue_context);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
    OAILOG_DEBUG (LOG_MME_APP, "MME_APP_INITIAL_UE_MESSAGE. Allocated new MME UE context and new mme_ue_s1ap_id. %d\n", ue_context->mme_ue_s1ap_id);
    if (RETURNerror == mme_insert_ue_context (&mme_app_desc.mme_ue_contexts, ue_context)) {
      mme_app_ue_context_free_content(ue_context);
      free_wrapper ((void**)&ue_context);
      OAILOG_ERROR (LOG_MME_APP, "Failed to insert new MME UE context enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT "\n", initial_pP->enb_ue_s1ap_id);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
  }
  ue_context->sctp_assoc_id_key = initial_pP->sctp_assoc_id;
  ue_context->e_utran_cgi = initial_pP->ecgi;
  // Notify S1AP about the mapping between mme_ue_s1ap_id and sctp assoc id + enb_ue_s1ap_id
  notify_s1ap_new_ue_mme_s1ap_id_association (ue_context->sctp_assoc_id_key, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id);
  // Initialize timers to INVALID IDs
  ue_context->mobile_reachability_timer.id = MME_APP_TIMER_INACTIVE_ID;
  ue_context->implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;
  ue_context->initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  ue_context->initial_context_setup_rsp_timer.sec = MME_APP_INITIAL_CONTEXT_SETUP_RSP_TIMER_VALUE;
  /** Inform the NAS layer about the new initial UE context. */
  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_INITIAL_UE_MESSAGE);
  // do this because of same message types name but not same struct in different .h
  message_p->ittiMsg.nas_initial_ue_message.nas.ue_id           = ue_context->mme_ue_s1ap_id;
  message_p->ittiMsg.nas_initial_ue_message.nas.tai             = initial_pP->tai;
  message_p->ittiMsg.nas_initial_ue_message.nas.ecgi            = initial_pP->ecgi;
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

//  uintptr_t bearer_context_3 = mme_app_get_ue_bearer_context_2(ue_context, 5);

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_INITIAL_UE_MESSAGE UE id " MME_UE_S1AP_ID_FMT " ", ue_context->mme_ue_s1ap_id);
  itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_erab_setup_req (itti_erab_setup_req_t * const itti_erab_setup_req)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, itti_erab_setup_req->ue_id);

  if (!ue_context) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " NAS_ERAB_SETUP_REQ Unknown ue " MME_UE_S1AP_ID_FMT " ", itti_erab_setup_req->ue_id);
    OAILOG_ERROR (LOG_MME_APP, "UE context doesn't exist for UE " MME_UE_S1AP_ID_FMT "\n", itti_erab_setup_req->ue_id);
    // memory leak
    bdestroy_wrapper(&itti_erab_setup_req->nas_msg);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  bearer_context_t* bearer_context = NULL;
  mme_app_get_session_bearer_context_from_all(ue_context, itti_erab_setup_req->ebi, &bearer_context);

  if (bearer_context) {
    MessageDef  *message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_E_RAB_SETUP_REQ);
    itti_s1ap_e_rab_setup_req_t *s1ap_e_rab_setup_req = &message_p->ittiMsg.s1ap_e_rab_setup_req;

    s1ap_e_rab_setup_req->mme_ue_s1ap_id = ue_context->mme_ue_s1ap_id;
    s1ap_e_rab_setup_req->enb_ue_s1ap_id = ue_context->enb_ue_s1ap_id;

    // E-RAB to Be Setup List
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.no_of_items = 1;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_id = bearer_context->ebi;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.allocation_and_retention_priority.pre_emption_capability =
        bearer_context->preemption_capability;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.allocation_and_retention_priority.pre_emption_vulnerability =
        bearer_context->preemption_vulnerability;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.allocation_and_retention_priority.priority_level =
        bearer_context->priority_level;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.gbr_qos_information.e_rab_maximum_bit_rate_downlink    = itti_erab_setup_req->mbr_dl;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.gbr_qos_information.e_rab_maximum_bit_rate_uplink      = itti_erab_setup_req->mbr_ul;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.gbr_qos_information.e_rab_guaranteed_bit_rate_downlink = itti_erab_setup_req->gbr_dl;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.gbr_qos_information.e_rab_guaranteed_bit_rate_uplink   = itti_erab_setup_req->gbr_ul;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.qci = bearer_context->qci;

    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].gtp_teid = bearer_context->s_gw_fteid_s1u.teid;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].transport_layer_address = fteid_ip_address_to_bstring(&bearer_context->s_gw_fteid_s1u);

    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].nas_pdu = itti_erab_setup_req->nas_msg;
    itti_erab_setup_req->nas_msg = NULL;

    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0, "0 S1AP_E_RAB_SETUP_REQ ue id " MME_UE_S1AP_ID_FMT " ebi %u teid " TEID_FMT " ",
        ue_context->mme_ue_s1ap_id,
        s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_id,
        s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].gtp_teid);
    int to_task = (RUN_MODE_SCENARIO_PLAYER == mme_config.run_mode) ? TASK_MME_SCENARIO_PLAYER:TASK_S1AP;
    itti_send_msg_to_task (to_task, INSTANCE_DEFAULT, message_p);
  } else {
    OAILOG_DEBUG (LOG_MME_APP, "No bearer context found ue " MME_UE_S1AP_ID_FMT  " ebi %u\n", itti_erab_setup_req->ue_id, itti_erab_setup_req->ebi);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_delete_session_rsp (
  const itti_s11_delete_session_response_t * const delete_sess_resp_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p = NULL;
  void                                   *id = NULL;

  DevAssert (delete_sess_resp_pP );
  OAILOG_DEBUG (LOG_MME_APP, "Received S11_DELETE_SESSION_RESPONSE from S+P-GW with the ID " MME_UE_S1AP_ID_FMT "\n ",delete_sess_resp_pP->teid);
  ue_context = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, delete_sess_resp_pP->teid);

  if (!ue_context) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 DELETE_SESSION_RESPONSE local S11 teid " TEID_FMT " ", delete_sess_resp_pP->teid);
    OAILOG_WARNING (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", delete_sess_resp_pP->teid);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 DELETE_SESSION_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
    delete_sess_resp_pP->teid, ue_context->emm_context._imsi64);
  /*
   * Updating statistics
   */
  mme_app_desc.mme_ue_contexts.nb_bearers_managed--;
  mme_app_desc.mme_ue_contexts.nb_bearers_since_last_stat--;

  /**
   * Object is later removed, not here. For unused keys, this is no problem, just deregistrate the tunnel ids for the MME_APP
   * UE context from the hashtable.
   * If this is not done, later at removal of the MME_APP UE context, the S11 keys will be checked and removed again if still existing.
   */
  // todo: handle this! where to remove the S11 Tunnel?
//  if(ue_context->num_pdns == 1){
//    /** This was the last PDN, removing the S11 TEID. */
//    hashtable_ts_remove(mme_app_desc.mme_ue_contexts.tun11_ue_context_htbl,
//        (const hash_key_t) ue_context->mme_teid_s11, &id);
//    ue_context->mme_teid_s11 = INVALID_TEID;
//    /** SAE-GW TEID will be initialized when PDN context is purged. */
//  }

   if (delete_sess_resp_pP->cause.cause_value != REQUEST_ACCEPTED) { /**< Ignoring currently, SAE-GW needs to check flag. */
     OAILOG_WARNING (LOG_MME_APP, "***WARNING****S11 Delete Session Rsp: NACK received from SPGW : %08x\n", delete_sess_resp_pP->teid);
   }
   MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 DELETE_SESSION_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
     delete_sess_resp_pP->teid, ue_context->imsi);
   /*
    * Updating statistics
    */
   update_mme_app_stats_s1u_bearer_sub();
   update_mme_app_stats_default_bearer_sub();

   /**
    * No recursion needed any more. This will just inform the EMM/ESM that a PDN session has been deactivated.
    * It will determine what to do based on if its a PDN Disconnect Process or an (implicit) detach.
    */
   message_p = itti_alloc_new_message (TASK_MME_APP, NAS_PDN_DISCONNECT_RSP);
   // do this because of same message types name but not same struct in different .h
   message_p->ittiMsg.nas_pdn_disconnect_rsp.ue_id           = ue_context->mme_ue_s1ap_id;
   message_p->ittiMsg.nas_pdn_disconnect_rsp.cause           = REQUEST_ACCEPTED;
   /*
    * We don't have an indicator, the message may come out of order. The only true indicator would be the GTPv2c transaction, which we don't have.
    * We use the esm_proc_data to find the correct PDN in ESM.
    */
   /** The only matching is made in the esm data context (pti specific). */
   itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);

   /** No S1AP release yet. */
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
int
mme_app_handle_create_sess_resp (
  itti_s11_create_session_response_t * const create_sess_resp_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    *ue_context = NULL;
  bearer_context_t                       *current_bearer_p = NULL;
  MessageDef                             *message_p = NULL;
  ebi_t                                   bearer_id = 0;
  mme_app_s10_proc_mme_handover_t        *s10_handover_procedure = NULL;
  nas_ctx_req_proc_t                     *emm_cn_proc_ctx_req = NULL;
  pdn_context_t                          *pdn_context = NULL;

  int                                     rc = RETURNok;

  DevAssert (create_sess_resp_pP );
  OAILOG_DEBUG (LOG_MME_APP, "Received S11_CREATE_SESSION_RESPONSE from S+P-GW\n");
  ue_context = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, create_sess_resp_pP->teid);

  if (ue_context == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_SESSION_RESPONSE local S11 teid " TEID_FMT " ", create_sess_resp_pP->teid);

    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", create_sess_resp_pP->teid);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_SESSION_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      create_sess_resp_pP->teid, ue_context->emm_context._imsi64);

  proc_tid_t  transaction_identifier = 0;
  pdn_cid_t   pdn_cx_id = 0;

  /** Check if there is a S10 handover procedure or CN context request procedure. */
  s10_handover_procedure = mme_app_get_s10_procedure_mme_handover(ue_context);
  /** Check if there is an EMM context. */
  emm_data_context_t * emm_context = emm_data_context_get(&_emm_data, ue_context->mme_ue_s1ap_id);
  if(emm_context)
    emm_cn_proc_ctx_req = get_nas_cn_procedure_ctx_req(emm_context);
  /* Whether SGW has created the session (IP address allocation, local GTP-U end point creation etc.)
   * successfully or not , it is indicated by cause value in create session response message.
   * If cause value is not equal to "REQUEST_ACCEPTED" then this implies that SGW could not allocate the resources for
   * the requested session. In this case, MME-APP sends PDN Connectivity fail message to NAS-ESM with the "cause" received
   * in S11 Session Create Response message.
   * NAS-ESM maps this "S11 cause" to "ESM cause" and sends it in PDN Connectivity Reject message to the UE.
   */
  if (create_sess_resp_pP->cause.cause_value != REQUEST_ACCEPTED) {
    // todo: if handover flag was active.. terminate the forward relocation procedure with a reject + remove the contexts & tunnel endpoints.
    // todo: check that EMM context did not had a TAU_PROCEDURE running, if so send a CN Context Fail
    /*
     * Send PDN CONNECTIVITY FAIL message to NAS layer.
     * For TAU/Attach case, a reject message will be sent and the UE contexts will be terminated.
     */
    if(s10_handover_procedure){
//      mme_remove_ue_context (&mme_app_desc.mme_ue_contexts, ue_context);
      // todo: the handover procedure should be failed --> causing any MME_APP/NAS/Tunnels to be removed..
      /** Assuming that no NAS layer exists. Reject the handover procedure. */
      mme_app_send_s10_forward_relocation_response_err(s10_handover_procedure->remote_mme_teid.teid,
          s10_handover_procedure->remote_mme_teid.ipv4_address,
          s10_handover_procedure->forward_relocation_trxn, RELOCATION_FAILURE);
//      todo: s10_handover_procedure->failure(ue_context); --> Remove the UE context/EMM context and also the S10 Tunnel endpoint
    }else if (emm_cn_proc_ctx_req){
      /** A CN Context Request procedure exists. Also check the remaining PDN contexts to create in the SAE-GW. */
      _mme_app_send_nas_context_response_err(ue_context->mme_ue_s1ap_id, RELOCATION_FAILURE);
    }else{
      /** Inform the NAS layer about the failure. */
      message_p = itti_alloc_new_message (TASK_MME_APP, NAS_PDN_CONNECTIVITY_FAIL);
      itti_nas_pdn_connectivity_fail_t *nas_pdn_connectivity_fail = &message_p->ittiMsg.nas_pdn_connectivity_fail;
      memset ((void *)nas_pdn_connectivity_fail, 0, sizeof (itti_nas_pdn_connectivity_fail_t));
      // todo: nas_pdn_connectivity_fail->pti = ue_context->pending_pdn_connectivity_req_pti;
      nas_pdn_connectivity_fail->ue_id = ue_context->mme_ue_s1ap_id;
      nas_pdn_connectivity_fail->cause = CAUSE_SYSTEM_FAILURE; // (pdn_conn_rsp_cause_t)(create_sess_resp_pP->cause);
      rc = itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
    }
    OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
  }

  //---------------------------------------------------------
  // Process itti_sgw_create_session_response_t.bearer_context_created
  //---------------------------------------------------------

  // todo: for handover with dedicated bearers --> iterate through bearer contexts!
  // todo: the MME will send an S10 message with the filters to the target MME --> which would then create a Create Session Request with multiple bearers..
  // todo: all the bearer contexts in the response should then be handled! (or do it via BRC // meshed PCRF).

  for (int i=0; i < create_sess_resp_pP->bearer_contexts_created.num_bearer_context; i++) {
    bearer_id = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].eps_bearer_id;
    /*
     * Depending on s11 result we have to send reject or accept for bearers
     */
    DevCheck ((bearer_id < BEARERS_PER_UE) && (bearer_id >= 0), bearer_id, BEARERS_PER_UE, 0);

    // todo: handle this case like create bearer request rejects in the SAE-GW, no removal of bearer contexts should be necessary
    if (create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].cause.cause_value != REQUEST_ACCEPTED) {
      DevMessage ("Cases where bearer cause != REQUEST_ACCEPTED are not handled. \n");
    }
    // todo: setting the default bearer id in the pdn context?
    DevAssert (create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].s1u_sgw_fteid.interface_type == S1_U_SGW_GTP_U);
//      current_bearer_p = mme_app_get_bearer_context(ue_context, bearer_id);
//      AssertFatal(current_bearer_p, "Could not get bearer context");
    /*
     * The bearer context needs to be put into the UE pool by
     * Try to get a new bearer context in the UE_Context instead of getting one from an array
     * todo: if cause above was reject for this bearer context, just skip it. */
    mme_app_get_session_bearer_context_from_all(ue_context, bearer_id, &current_bearer_p);
    if(current_bearer_p == NULL) {
      // If we failed to allocate a new bearer context
      OAILOG_ERROR (LOG_MME_APP, "Failed to allocate a new bearer context with EBI %d for mmeUeS1apId:" MME_UE_S1AP_ID_FMT "\n", bearer_id, ue_context->mme_ue_s1ap_id);
      OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
    }
    current_bearer_p->bearer_state |= BEARER_STATE_SGW_CREATED;
    if (!i) {
      // todo: assuming this one is the default_ebi
      /** No context identifier might be set yet (multiple might exist and all might be 0. */
//      AssertFatal((pdn_cx_id >= 0) && (pdn_cx_id < MAX_APN_PER_UE), "Bad pdn id for bearer");
      /** IP address not set here. Will be forwarded and set by ESM layer @ NAS_PDN_CONNECTIVITY_RES. */
      mme_app_get_pdn_context(ue_context, 0, current_bearer_p->ebi, NULL, &pdn_context);
//      ue_context->pdn_contexts[pdn_cx_id]->s_gw_teid_s11_s4 = create_sess_resp_pP->s11_sgw_fteid.teid;
      pdn_context->s_gw_teid_s11_s4 = create_sess_resp_pP->s11_sgw_fteid.teid;
      pdn_context->s_gw_address_s11_s4.address.ipv4_address.s_addr = create_sess_resp_pP->s11_sgw_fteid.ipv4_address.s_addr;
      transaction_identifier = current_bearer_p->transaction_identifier;
    }
    /*
     * Updating statistics
     */
    mme_app_desc.mme_ue_contexts.nb_bearers_managed++;
    mme_app_desc.mme_ue_contexts.nb_bearers_since_last_stat++;
    current_bearer_p->s_gw_fteid_s1u = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].s1u_sgw_fteid; /**< Also copying the IPv4/V6 address. */
    current_bearer_p->p_gw_fteid_s5_s8_up = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].s5_s8_u_pgw_fteid;

    // if modified by pgw
    if (create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos ) {
      current_bearer_p->qci                      = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->qci;
      current_bearer_p->priority_level           = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->pl;
      current_bearer_p->preemption_vulnerability = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->pvi;
      current_bearer_p->preemption_capability    = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->pci;

      //TODO should be set in NAS_PDN_CONNECTIVITY_RSP message
      // dbeken: we don't have NAS context in handover, so we need to do it here.
      current_bearer_p->esm_ebr_context.gbr_dl   = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->gbr.br_dl;
      current_bearer_p->esm_ebr_context.gbr_ul   = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->gbr.br_ul;
      current_bearer_p->esm_ebr_context.mbr_dl   = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->mbr.br_dl;
      current_bearer_p->esm_ebr_context.mbr_ul   = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->mbr.br_ul;
      OAILOG_DEBUG (LOG_MME_APP, "Set qci %u in bearer %u\n", current_bearer_p->qci, bearer_id);
    } else {
      OAILOG_DEBUG (LOG_MME_APP, "Set qci %u in bearer %u (qos not modified by P-GW)\n", current_bearer_p->qci, bearer_id);
    }
    /** Done iterating the established bearers. */
  }

  /*
   * Process the PCOs received from the SAE-GW directly.
   */
  if (!pdn_context->pco) {
    pdn_context->pco = calloc(1, sizeof(protocol_configuration_options_t));
  } else {
    clear_protocol_configuration_options(pdn_context->pco);
  }
  copy_protocol_configuration_options(pdn_context->pco, &create_sess_resp_pP->pco);
  // todo: review the old code!
  //#define TEMPORARY_DEBUG 1
  //#if TEMPORARY_DEBUG
  // bstring b = protocol_configuration_options_to_xml(&ue_context->pending_pdn_connectivity_req_pco);
  // OAILOG_DEBUG (LOG_MME_APP, "PCO %s\n", bdata(b));
  // bdestroy_wrapper(&b);
  //#endif

  // todo: if handovered with multiple bearers, we might need to send  CSR and handle CSResp with multiple bearer contexts!
  // then do we need to send NAS only the default bearer_context or all bearer contexts, and send them all in NAS?
  // todo: for dedicated bearers, we also might need to set GBR/MBR values
  //    nas_pdn_connectivity_rsp->qos.gbrUL = 64;        /* 64=64kb/s   Guaranteed Bit Rate for uplink   */
  //      nas_pdn_connectivity_rsp->qos.gbrDL = 120;       /* 120=512kb/s Guaranteed Bit Rate for downlink */
  //      nas_pdn_connectivity_rsp->qos.mbrUL = 72;        /* 72=128kb/s   Maximum Bit Rate for uplink      */
  //      nas_pdn_connectivity_rsp->qos.mbrDL = 135;       /*135=1024kb/s Maximum Bit Rate for downlink    */

  /** Set the PAA into the PDN Context.  (todo: copy function). */
  /** Decouple the PAA and set it into the PDN context. */
  /** Check if a PAA already exists. If so remove it and set the new one. */
  if(pdn_context->paa){
    free_wrapper((void**)&pdn_context->paa);
  }
  pdn_context->paa = create_sess_resp_pP->paa;
  create_sess_resp_pP->paa = NULL;

  /*
   * Check if there is a handover process ongoing.
   * It can only be at the target-MME at this point.
   */
  if(s10_handover_procedure){
    /*
     * Assume inter-MME handover without a (valid) NAS context.
     * Check for further pending pdn_connections that need to be established via handover.
     * If none existing, continue with handover request.
     */
    OAILOG_INFO(LOG_MME_APP, "Inter MME S10 Handover process exists for UE " MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
    s10_handover_procedure->nas_s10_context.n_pdns++;
    if(s10_handover_procedure->pdn_connections->num_pdn_connections > s10_handover_procedure->next_processed_pdn_connection){
      OAILOG_INFO(LOG_MME_APP, "We have further PDN connections that need to be established via handover for UE " MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
      pdn_connection_t * pdn_connection = &s10_handover_procedure->pdn_connections->pdn_connection[s10_handover_procedure->next_processed_pdn_connection];
      DevAssert(pdn_connection);
      mme_app_handle_pdn_connectivity_from_s10(ue_context, pdn_connection);
      s10_handover_procedure->next_processed_pdn_connection++;
      /*
       * When Create Session Response is received, continue to process the next PDN connection, until all are processed.
       * When all pdn_connections are completed, continue with handover request.
       */
      mme_app_send_s11_create_session_req (ue_context, NULL, pdn_context, &s10_handover_procedure->target_tai);
      OAILOG_INFO(LOG_MME_APP, "Successfully sent CSR for UE " MME_UE_S1AP_ID_FMT ". Waiting for CSResp to continue to process handover on source MME side. \n", ue_context->mme_ue_s1ap_id);
    }else{
      OAILOG_INFO(LOG_MME_APP, "No further PDN connections that need to be established via handover for UE " MME_UE_S1AP_ID_FMT ". Continuing with handover request. \n", ue_context->mme_ue_s1ap_id);
      /** Get all VOs of all session bearers and send handover request with it. */
      bearer_contexts_to_be_created_t bcs_tbc;
      memset((void*)&bcs_tbc, 0, sizeof(bcs_tbc));
      pdn_context_t * registered_pdn_ctx = NULL;
      RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_context->pdn_contexts) {
        DevAssert(registered_pdn_ctx);
        mme_app_get_bearer_contexts_to_be_created(registered_pdn_ctx, &bcs_tbc, BEARER_STATE_NULL);
        /** The number of bearers will be incremented in the method. S10 should just pick the ebi. */
      }
      mme_app_send_s1ap_handover_request(ue_context->mme_ue_s1ap_id,
          &bcs_tbc,
          // todo: check for macro/home enb_id
          s10_handover_procedure->target_id.target_id.macro_enb_id.enb_id,
          s10_handover_procedure->nas_s10_context.mm_eps_ctx->ue_nc.eea,
          s10_handover_procedure->nas_s10_context.mm_eps_ctx->ue_nc.eia,
          s10_handover_procedure->nas_s10_context.mm_eps_ctx->nh,
          s10_handover_procedure->nas_s10_context.mm_eps_ctx->ncc,
          s10_handover_procedure->source_to_target_eutran_f_container.container_value);

      s10_handover_procedure->source_to_target_eutran_f_container.container_value = NULL; /**< Set it to NULL ALWAYS. */
    }
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
  }else if (emm_cn_proc_ctx_req){
    OAILOG_INFO(LOG_MME_APP, "NAS Context Request process exists for UE " MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
    /** Increase the num of established PDNs. */
    emm_cn_proc_ctx_req->nas_s10_context.n_pdns++;
    if(emm_cn_proc_ctx_req->pdn_connections->num_pdn_connections > emm_cn_proc_ctx_req->next_processed_pdn_connection){
      OAILOG_INFO(LOG_MME_APP, "We have further PDN connections that need to be established via handover for UE " MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
      pdn_connection_t * pdn_connection = &emm_cn_proc_ctx_req->pdn_connections->pdn_connection[emm_cn_proc_ctx_req->next_processed_pdn_connection];
      DevAssert(pdn_connection);
      mme_app_handle_pdn_connectivity_from_s10(ue_context, pdn_connection);
      emm_cn_proc_ctx_req->next_processed_pdn_connection++;
      /*
       * When Create Session Response is received, continue to process the next PDN connection, until all are processed.
       * When all pdn_connections are completed, continue with handover request.
       */
      // todo: check target_tai at idle mode
      mme_app_send_s11_create_session_req (ue_context, NULL, pdn_context, &ue_context->tai_last_tau);
      OAILOG_INFO(LOG_MME_APP, "Successfully sent CSR for UE " MME_UE_S1AP_ID_FMT ". Waiting for CSResp to continue to process handover on source MME side. \n", ue_context->mme_ue_s1ap_id);
    }else{
      OAILOG_INFO(LOG_MME_APP, "No further PDN connections that need to be established via idle mode TAU for UE " MME_UE_S1AP_ID_FMT ". "
          "Continuing with NAS context response. \n", ue_context->mme_ue_s1ap_id);
      mme_app_itti_nas_context_response(ue_context, &emm_cn_proc_ctx_req->nas_s10_context);
    }
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
  }
  if(emm_context){
    OAILOG_INFO(LOG_MME_APP, "No NAS Context Request process or handover process exists for UE " MME_UE_S1AP_ID_FMT " Continuing with PDN connectivity response. \n", ue_context->mme_ue_s1ap_id);
    // todo: no multi bearer here!
    bearer_context_t * first_bearer_context = RB_MIN(SessionBearers, &pdn_context->session_bearers);
    DevAssert(first_bearer_context);
    mme_app_itti_nas_pdn_connectivity_response(ue_context,
        pdn_context->paa, &create_sess_resp_pP->pco,
        first_bearer_context);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
  }

  /*
   * No Handover Procedure running and no EMM context existing.
   * Performing an implicit detach. Not processing message further.
   */
  mme_remove_ue_context (&mme_app_desc.mme_ue_contexts, ue_context);
  /** Not sending back failure. */
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
}

//------------------------------------------------------------------------------
int
mme_app_handle_modify_bearer_resp (
  const itti_s11_modify_bearer_response_t * const modify_bearer_resp_pP)
{
  struct ue_context_s                    *ue_context       = NULL;
  struct pdn_context_s                   *pdn_context      = NULL;
  bearer_context_t                       *current_bearer_p = NULL;
  MessageDef                             *message_p = NULL;
  int16_t                                 bearer_id =5;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (modify_bearer_resp_pP );
  OAILOG_DEBUG (LOG_MME_APP, "Received S11_MODIFY_BEARER_RESPONSE from S+P-GW\n");
  ue_context = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, modify_bearer_resp_pP->teid);

  if (ue_context == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 MODIFY_BEARER_RESPONSE local S11 teid " TEID_FMT " ", modify_bearer_resp_pP->teid);

    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", modify_bearer_resp_pP->teid);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 MODIFY_BEARER_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      modify_bearer_resp_pP->teid, ue_context->imsi);
  /*
   * Updating statistics
   */
  if (modify_bearer_resp_pP->cause.cause_value != REQUEST_ACCEPTED) {
    /**
     * Check if it is an X2 Handover procedure, in that case send an X2 Path Switch Request Failure to the target MME.
     * In addition, perform an implicit detach in any case.
     */
    if(ue_context->pending_x2_handover){
      OAILOG_ERROR(LOG_MME_APP, "Error modifying SAE-GW bearers for UE with ueId: " MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
      mme_app_send_s1ap_path_switch_request_failure(ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id, ue_context->sctp_assoc_id_key, S1AP_SYSTEM_FAILURE);
      /** We continue with the implicit detach, since handover already happened. */
    }
    /** Implicitly detach the UE --> If EMM context is missing, still continue with the resource removal. */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->mme_ue_s1ap_id;
    message_p->ittiMsg.nas_implicit_detach_ue_ind.emm_cause = EMM_CAUSE_NETWORK_FAILURE;
    message_p->ittiMsg.nas_implicit_detach_ue_ind.detach_type = 0x02; // Re-Attach Not required;
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
  }
  // todo: check baerer contexts marked for removal
  mme_app_get_session_bearer_context_from_all(ue_context, modify_bearer_resp_pP->bearer_contexts_modified.bearer_contexts[0].eps_bearer_id, &current_bearer_p);
  /** Get the first bearers PDN. */
  // todo: optimize the validation!
  DevAssert(current_bearer_p);
  mme_app_get_pdn_context(ue_context, current_bearer_p->pdn_cx_id, current_bearer_p->ebi, NULL, &pdn_context);
  DevAssert(pdn_context);

  /** Set all bearers of the EBI to valid. */
  bearer_context_t * bc_to_act = NULL;
  RB_FOREACH (bc_to_act, SessionBearers, &pdn_context->session_bearers) {
    DevAssert(bc_to_act);
    /** Add them to the bearers list of the MBR. */
    bc_to_act->bearer_state = BEARER_STATE_ACTIVE;
  }

  // todo: set the downlink teid?
  /** If it is an X2 Handover, send a path switch response back. */
  if(ue_context->pending_x2_handover){
    OAILOG_INFO(LOG_MME_APP, "Sending an S1AP Path Switch Request Acknowledge for for UE with ueId: " MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
    mme_app_send_s1ap_path_switch_request_acknowledge(ue_context->mme_ue_s1ap_id);
    /** Reset the flag. */
    ue_context->pending_x2_handover = false;
    OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
  }
  /** Check for handover procedures. */
  mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
  if(s10_handover_proc){
    /** No matter if it is a INTRA or INTER MME handover, continue with the MBR for other PDNs. */
    pdn_context_t * pdn_context = NULL;
    RB_FOREACH (pdn_context, PdnContexts, &ue_context->pdn_contexts) {
      DevAssert(pdn_context);
      bearer_context_t * first_bearer = RB_MIN(SessionBearers, &pdn_context->session_bearers);
      DevAssert(first_bearer);
      if(first_bearer->bearer_state == BEARER_STATE_ACTIVE){
        /** Continue to next pdn. */
        continue;
      }else{
        /** Found a PDN. Establish the bearer contexts. */
        OAILOG_INFO(LOG_MME_APP, "Establishing the bearers for UE_CONTEXT for UE " MME_UE_S1AP_ID_FMT " triggered by handover notify. \n", ue_context->mme_ue_s1ap_id);
        mme_app_send_s11_modify_bearer_req(ue_context, pdn_context);
        OAILOG_FUNC_OUT (LOG_MME_APP);
      }
    }
    OAILOG_INFO(LOG_MME_APP, "No PDN found to establish the bearers for UE " MME_UE_S1AP_ID_FMT ". Checking for INTER-MME handover. \n", ue_context->mme_ue_s1ap_id);
    if(s10_handover_proc->proc.type == MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER){
      /**
       * UE came from S10 inter-MME handover. Not clear the pending_handover state yet.
       * Sending Forward Relocation Complete Notification and waiting for acknowledgment.
       */
       /** Nothing else left to do on the target side. We don't delete the handover process. It will run and timeout until TAU is complete. */
    }else{
      /** For INTRA-MME handover trigger the timer mentioned in TS 23.401 to remove the UE Context and the old S1AP UE reference to the source eNB. */
      ue_description_t * old_ue_reference = s1ap_is_enb_ue_s1ap_id_in_list_per_enb(s10_handover_proc->source_enb_ue_s1ap_id, s10_handover_proc->source_ecgi.cell_identity.enb_id);
      if(old_ue_reference){
        /** For INTRA-MME handover, start the timer to remove the old UE reference here. No timer should be started for the S10 Handover Process. */
        if (timer_setup (mme_config.mme_mobility_completion_timer, 0,
            TASK_S1AP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *)old_ue_reference, &(old_ue_reference->s1ap_handover_completion_timer.id)) < 0) {
          OAILOG_ERROR (LOG_MME_APP, "Failed to start >s1ap_handover_completion for enbUeS1apId " ENB_UE_S1AP_ID_FMT " for duration %d \n",
              old_ue_reference->enb_ue_s1ap_id, mme_config.mme_mobility_completion_timer);
          old_ue_reference->s1ap_handover_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
          /** Not set the timer for the UE context, since we will not deregister. */
          OAILOG_FUNC_OUT (LOG_MME_APP);
        } else {
          OAILOG_DEBUG (LOG_MME_APP, "MME APP : Completed Handover Procedure at (source) MME side after handling S1AP_HANDOVER_NOTIFY. "
              "Activated the S1AP Handover completion timer enbUeS1apId " ENB_UE_S1AP_ID_FMT ". Removing source eNB resources after timer.. Timer Id %u. Timer duration %d \n",
              old_ue_reference->enb_ue_s1ap_id, old_ue_reference->s1ap_handover_completion_timer.id, mme_config.mme_mobility_completion_timer);
          /** Remove the handover procedure. */
          mme_app_delete_s10_procedure_mme_handover(ue_context);
          /** Not setting the timer for MME_APP UE context. */
          OAILOG_FUNC_OUT (LOG_MME_APP);
        }
      }else{
        OAILOG_DEBUG(LOG_MME_APP, "No old UE_REFERENCE was found for mmeS1apUeId " MME_UE_S1AP_ID_FMT " and enbUeS1apId "ENB_UE_S1AP_ID_FMT ". Not starting a new timer. \n",
            ue_context->enb_ue_s1ap_id, s10_handover_proc->source_ecgi.cell_identity.enb_id);
        OAILOG_FUNC_OUT (LOG_MME_APP);
      }
    }
  }
}

//------------------------------------------------------------------------------
static
void mme_app_send_downlink_data_notification_acknowledge(gtpv2c_cause_value_t cause, teid_t saegw_s11_teid, uint32_t peer_ip, void *trxn){
  OAILOG_FUNC_IN (LOG_MME_APP);

  /** Send a Downlink Data Notification Acknowledge with cause. */
  MessageDef * message_p = itti_alloc_new_message (TASK_MME_APP, S11_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE);
  DevAssert (message_p != NULL);

  itti_s11_downlink_data_notification_acknowledge_t *downlink_data_notification_ack_p = &message_p->ittiMsg.s11_downlink_data_notification_acknowledge;
  memset ((void*)downlink_data_notification_ack_p, 0, sizeof (itti_s11_downlink_data_notification_acknowledge_t));
  // todo: s10 TEID set every time?
  downlink_data_notification_ack_p->teid = saegw_s11_teid; // todo: ue_context->mme_s10_teid;
  /** No Local TEID exists yet.. no local S10 tunnel is allocated. */
  // todo: currently only just a single MME is allowed.
  /** Get the first PDN context. */

  downlink_data_notification_ack_p->peer_ip = peer_ip;
  downlink_data_notification_ack_p->cause.cause_value = cause;
  downlink_data_notification_ack_p->trxn  = trxn;

  /** Deallocate the container in the FORWARD_RELOCATION_REQUEST.  */
  // todo: how is this deallocated

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "MME_APP Sending S11 DOWNLINK_DATA_NOTIFICATION_ACK");

  /** Sending a message to S10. */
  itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_downlink_data_notification(const itti_s11_downlink_data_notification_t * const saegw_dl_data_ntf_pP){
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p = NULL;
  int16_t                                 bearer_id =5;
  int                                     rc = RETURNok;


  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (saegw_dl_data_ntf_pP );
  DevAssert (saegw_dl_data_ntf_pP->trxn);

  OAILOG_DEBUG (LOG_MME_APP, "Received S11_DOWNLINK_DATA_NOTIFICATION from S+P-GW\n");
  ue_context = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, saegw_dl_data_ntf_pP->teid);

  if (ue_context == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "DOWNLINK_DATA_NOTIFICATION FROM local S11 teid " TEID_FMT " ", saegw_dl_data_ntf_pP->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", saegw_dl_data_ntf_pP->teid);
    /** Send a DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE. */
    mme_app_send_downlink_data_notification_acknowledge(CONTEXT_NOT_FOUND, saegw_dl_data_ntf_pP->teid, saegw_dl_data_ntf_pP->peer_ip, saegw_dl_data_ntf_pP->trxn);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "DOWNLINK_DATA_NOTIFICATION for local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      saegw_dl_data_ntf_pP->teid, ue_context->imsi);

  /** Check that the UE is in idle mode!. */
  if (ECM_IDLE != ue_context->ecm_state) {
    OAILOG_ERROR (LOG_MME_APP, "UE_Context with IMSI " IMSI_64_FMT " and mmeUeS1apId: %d. \n is not in ECM_IDLE mode, instead %d. \n",
        ue_context->imsi, ue_context->mme_ue_s1ap_id, ue_context->ecm_state);
    // todo: later.. check this more granularly
    mme_app_send_downlink_data_notification_acknowledge(UE_ALREADY_RE_ATTACHED, saegw_dl_data_ntf_pP->teid, saegw_dl_data_ntf_pP->peer_ip, saegw_dl_data_ntf_pP->trxn);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  OAILOG_INFO(LOG_MME_APP, "MME_MOBILTY_COMPLETION timer is not running. Starting paging procedure for UE with imsi " IMSI_64_FMT ". \n", ue_context->imsi);

  // todo: timeout to wait to ignore further DL_DATA_NOTIF messages->
  mme_app_send_downlink_data_notification_acknowledge(REQUEST_ACCEPTED, saegw_dl_data_ntf_pP->teid, saegw_dl_data_ntf_pP->peer_ip, saegw_dl_data_ntf_pP->trxn);

  /** No need to start paging timeout timer. It will be handled by the Periodic TAU update timer. */
  // todo: no downlink data notification failure and just removing the UE?

  /** Do paging on S1AP interface. */
  message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_PAGING);
  DevAssert (message_p != NULL);
  itti_s1ap_paging_t *s1ap_paging_p = &message_p->ittiMsg.s1ap_paging;

  memset (s1ap_paging_p, 0, sizeof (itti_s1ap_paging_t));
  s1ap_paging_p->mme_ue_s1ap_id = ue_context->mme_ue_s1ap_id; /**< Just MME_UE_S1AP_ID. */
  s1ap_paging_p->ue_identity_index = (ue_context->imsi %1024) & 0xFFFF; /**< Just MME_UE_S1AP_ID. */
  s1ap_paging_p->tmsi = ue_context->guti.m_tmsi;
  // todo: these ones may differ from GUTI?
  s1ap_paging_p->tai.plmn = ue_context->guti.gummei.plmn;
  s1ap_paging_p->tai.tac  = *mme_config.served_tai.tac;

  /** S1AP Paging. */
  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
}

//------------------------------------------------------------------------------
void
mme_app_handle_initial_context_setup_rsp (
  itti_mme_app_initial_context_setup_rsp_t * const initial_ctxt_setup_rsp_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                 *ue_context = NULL;
  MessageDef                          *message_p = NULL;

  OAILOG_DEBUG (LOG_MME_APP, "Received MME_APP_INITIAL_CONTEXT_SETUP_RSP from S1AP\n");
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, initial_ctxt_setup_rsp_pP->ue_id);

  if (ue_context == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", initial_ctxt_setup_rsp_pP->ue_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " MME_APP_INITIAL_CONTEXT_SETUP_RSP Unknown ue %u", initial_ctxt_setup_rsp_pP->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

//  if(ue_context->came_from_tau){
//    OAILOG_DEBUG (LOG_MME_APP, "Sleeping @ MME_APP_INITIAL_CONTEXT_SETUP_RSP from S1AP\n");
//    sleep(1);
//    OAILOG_DEBUG (LOG_MME_APP, "After sleeping @ MME_APP_INITIAL_CONTEXT_SETUP_RSP from S1AP\n");
//  }

  // Stop Initial context setup process guard timer,if running
  if (ue_context->initial_context_setup_rsp_timer.id != MME_APP_TIMER_INACTIVE_ID) {
    if (timer_remove(ue_context->initial_context_setup_rsp_timer.id, NULL)) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to stop Initial Context Setup Rsp timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
    }
    ue_context->initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  }

  pdn_context_t * registered_pdn_ctx = NULL;
  /** Update all bearers and get the pdn context id. */
  RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_context->pdn_contexts) {
    DevAssert(registered_pdn_ctx);

    /**
     * Get the first PDN whose bearers are not established yet.
     * Do the MBR just one PDN at a time.
     */
    bearer_context_t * bearer_context_to_establish = NULL;
    RB_FOREACH (bearer_context_to_establish, SessionBearers, &registered_pdn_ctx->session_bearers) {
      DevAssert(bearer_context_to_establish);
      /** Add them to the bearears list of the MBR. */
      if (bearer_context_to_establish->ebi == initial_ctxt_setup_rsp_pP->e_rab_id[0]){
        goto found_pdn;
      }
    }
    registered_pdn_ctx = NULL;
  }
  OAILOG_INFO(LOG_MME_APP, "No PDN context found with unestablished bearers for mmeUeS1apId " MME_UE_S1AP_ID_FMT ". Dropping Initial Context Setup Response. \n", ue_context->mme_ue_s1ap_id);
  OAILOG_FUNC_OUT (LOG_MME_APP);

found_pdn:
//  /** Save the bearer information as pending or send it directly if UE is registered. */
//  RB_FOREACH (bearer_context_to_establish, SessionBearers, &registered_pdn_ctx->session_bearers) {
//    DevAssert(bearer_context_to_establish);
//    /** Add them to the bearears list of the MBR. */
//    if (bearer_context_to_establish->ebi == initial_ctxt_setup_rsp_pP->e_rab_id[0]){
//      goto found_pdn;
//    }
//  }

  for (int item = 0; item < initial_ctxt_setup_rsp_pP->no_of_e_rabs; item++) {
    ebi_t ebi_to_establish = initial_ctxt_setup_rsp_pP->e_rab_id[item];
    /** Update the bearer context. */
    bearer_context_t* bearer_context_to_setup = mme_app_get_session_bearer_context(registered_pdn_ctx, ebi_to_establish);
    if(bearer_context_to_setup){
      bearer_context_to_setup->enb_fteid_s1u.interface_type = S1_U_ENODEB_GTP_U;
      bearer_context_to_setup->enb_fteid_s1u.teid           = initial_ctxt_setup_rsp_pP->gtp_teid[item];
      bearer_context_to_setup->enb_fteid_s1u.teid           = initial_ctxt_setup_rsp_pP->gtp_teid[item];
      /** Set the IP address. */
      if (4 == blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item])) {
        bearer_context_to_setup->enb_fteid_s1u.ipv4         = 1;
        memcpy(&bearer_context_to_setup->enb_fteid_s1u.ipv4_address,
               initial_ctxt_setup_rsp_pP->transport_layer_address[item]->data, blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item]));
      } else if (16 == blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item])) {
        bearer_context_to_setup->enb_fteid_s1u.ipv6         = 1;
        memcpy(&bearer_context_to_setup->enb_fteid_s1u.ipv6_address,
            initial_ctxt_setup_rsp_pP->transport_layer_address[item]->data,
            blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item]));
      } else {
        AssertFatal(0, "TODO IP address %d bytes", blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item]));
      }
      bearer_context_to_setup->bearer_state |= BEARER_STATE_ENB_CREATED;
      bearer_context_to_setup->bearer_state |= BEARER_STATE_MME_CREATED;
    }
  }
  /** Setting as ACTIVE when MBResp received from SAE-GW. */
  if(ue_context->mm_state == UE_REGISTERED){
    /** Send Modify Bearer Request for the APN. */
    mme_app_send_s11_modify_bearer_req(ue_context, registered_pdn_ctx);
    // todo: check Modify bearer request
  }else{
    /** Will send MBR with MM state change callbacks (with ATTACH_COMPLETE). */
    OAILOG_INFO(LOG_MME_APP, "IMSI " IMSI_64_FMT " is not registered yet. Waiting the UE to register to send the MBR.\n", ue_context->imsi);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_release_access_bearers_resp (
  const itti_s11_release_access_bearers_response_t * const rel_access_bearers_rsp_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    *ue_context = NULL;

  ue_context = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, rel_access_bearers_rsp_pP->teid);

  if (ue_context == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 RELEASE_ACCESS_BEARERS_RESPONSE local S11 teid " TEID_FMT " ",
    		rel_access_bearers_rsp_pP->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %" PRIX32 "\n", rel_access_bearers_rsp_pP->teid);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 RELEASE_ACCESS_BEARERS_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ", rel_access_bearers_rsp_pP->teid, ue_context->emm_context._imsi64);

  S1ap_Cause_t            s1_ue_context_release_cause = {0};
  s1_ue_context_release_cause.present = S1ap_Cause_PR_radioNetwork;
  s1_ue_context_release_cause.choice.radioNetwork = S1ap_CauseRadioNetwork_release_due_to_eutran_generated_reason;

  // Send UE Context Release Command
  mme_app_itti_ue_context_release(ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id, ue_context->s1_ue_context_release_cause, ue_context->e_utran_cgi.cell_identity.enb_id);
  if (ue_context->s1_ue_context_release_cause == S1AP_SCTP_SHUTDOWN_OR_RESET) {
    // Just cleanup the MME APP state associated with s1.
    mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_s11_create_bearer_req (
    const itti_s11_create_bearer_request_t * const create_bearer_request_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  //MessageDef                             *message_p = NULL;
  struct ue_context_s                      *ue_context  = NULL;
  struct pdn_context_s                     *pdn_context = NULL;

  ue_context = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, create_bearer_request_pP->teid);

  if (ue_context == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_BEARERS_REQUEST local S11 teid " TEID_FMT " ",
        create_bearer_request_pP->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %" PRIX32 "\n", create_bearer_request_pP->teid);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  // check if default bearer already created
  ebi_t linked_eps_bearer_id = create_bearer_request_pP->linked_eps_bearer_id;
  /** Check that the default bearer context is a session bearer context. */
  bearer_context_t * linked_bc = NULL;
  mme_app_get_session_bearer_context_from_all(ue_context, linked_eps_bearer_id, &linked_bc);
  if (!linked_bc) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_BEARERS_REQUEST ue id " MME_UE_S1AP_ID_FMT " local S11 teid " TEID_FMT " ",
        ue_context->mme_ue_s1ap_id, create_bearer_request_pP->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find the linked bearer id %" PRIu8 " for UE: " MME_UE_S1AP_ID_FMT "\n",
        linked_eps_bearer_id, ue_context->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Get the PDN context. */
  mme_app_get_pdn_context(ue_context, linked_bc->pdn_cx_id, linked_bc->ebi, NULL, &pdn_context);
  DevAssert(pdn_context);

  pdn_cid_t cid = linked_bc->pdn_cx_id;

  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_BEARERS_REQUEST ue id " MME_UE_S1AP_ID_FMT " PDN id %u IMSI " IMSI_64_FMT " n ebi %u",
      ue_context->mme_ue_s1ap_id, cid, ue_context->imsi, create_bearer_request_pP->bearer_contexts.num_bearer_context);

  /** Create an S11 procedure. */
  mme_app_s11_proc_create_bearer_t* s11_proc_create_bearer = mme_app_create_s11_procedure_create_bearer(ue_context);
  s11_proc_create_bearer->proc.s11_trxn  = (uintptr_t)create_bearer_request_pP->trxn;

  /*
   * Let the ESM layer validate the request and build the pending bearer contexts.
   * Also, send a single message to the eNB.
   * May received multiple back.
   */
  // forward request to NAS
  MessageDef  *message_p = itti_alloc_new_message (TASK_MME_APP, MME_APP_CREATE_DEDICATED_BEARER_REQ);
  AssertFatal (message_p , "itti_alloc_new_message Failed");
  MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).ue_id          = ue_context->mme_ue_s1ap_id;
  MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).cid            = cid;
  MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).linked_ebi     = pdn_context->default_ebi;
  /** Bearer will be allocated in the ESM layer. */
  for (int i = 0; i < create_bearer_request_pP->bearer_contexts.num_bearer_context; i++) {
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // TODO THINK OF BEARER AGGREGATING SEVERAL SDFs, 1 bearer <-> (QCI, ARP)
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    const bearer_context_within_create_bearer_request_t *msg_bc = &create_bearer_request_pP->bearer_contexts.bearer_contexts[i];
//    bearer_context_t *  dedicated_bc = mme_app_create_bearer_context(ue_context, cid, msg_bc->eps_bearer_id, false);

    /*
     * Set the FTEIDs into the process as pending IEs.
     * When Activate Bearer Context Response is arrived from the UE, we will look try to find the FTEID from the S1U teid.
     */

//    MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).bearer_qos[i]          = msg_bc->bearer_level_qos;
    /** Allocate a new BearerFTEIDs structure and copy the FTEIDs into the procedure. */
    struct fteid_set_s *fteid_set = calloc(1, sizeof(struct fteid_set_s));
    DevAssert(fteid_set); // todo:
    memcpy((void*)&fteid_set->s1u_fteid, (void*)&msg_bc->s1u_sgw_fteid, sizeof(fteid_t));
    memcpy((void*)&fteid_set->s5_fteid, (void*)&msg_bc->s5_s8_u_pgw_fteid, sizeof(fteid_t));

//    Assert(!RB_INSERT (BearerFteids, &s11_proc_create_bearer->fteid_set, fteid_set)); /**< Find the correct FTEID later by using the S1U FTEID as key.. */
//    MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).s_gw_fteid_s1u[i]      = msg_bc->s1u_sgw_fteid;
//    MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).p_gw_fteid_s5_s8_up[i] = msg_bc->s5_s8_u_pgw_fteid;
    // Todo: we cannot store them in a map, because when we evaluate the response, EBI is our key, which is not set here. That's why, we need to forward it to ESM.

    MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).fteid_set[i] = fteid_set;

//    s11_proc_create_bearer->bearer_status[EBI_TO_INDEX(msg_bc->eps_bearer_id)] = S11_PROC_BEARER_PENDING;
    s11_proc_create_bearer->bearer_status[i] = S11_PROC_BEARER_PENDING;

//    dedicated_bc->bearer_state   |= BEARER_STATE_SGW_CREATED;
//    dedicated_bc->bearer_state   |= BEARER_STATE_MME_CREATED;

    if (msg_bc->tft.numberofpacketfilters) {
      MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).tfts[i] = calloc(1, sizeof(traffic_flow_template_t));
      copy_traffic_flow_template(MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).tfts[i], &msg_bc->tft);
    }
    if (msg_bc->pco.num_protocol_or_container_id) {
      MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).pcos[i] = calloc(1, sizeof(protocol_configuration_options_t));
      copy_protocol_configuration_options(MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).pcos[i], &msg_bc->pco);
    }

    /** Update the number of bearers. */
    s11_proc_create_bearer->num_bearers++;
    MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).num_bearers++;
  }
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 MME_APP_CREATE_DEDICATED_BEARER_REQ mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " qci %u cid %u",
      MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).ue_id, msg_bc->bearer_level_qos.qci, cid);
  itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_handle_e_rab_setup_rsp (itti_s1ap_e_rab_setup_rsp_t  * const e_rab_setup_rsp)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                 *ue_context = NULL;
  bool                                 send_s11_response = false;
  /** Add the bearers into the procedure. */
  mme_app_s11_proc_create_bearer_t *s11_proc_create_bearer = mme_app_get_s11_procedure_create_bearer(ue_context);
  /** Put it into the procedure. */
  if(s11_proc_create_bearer){
    // todo: memleaks..
    OAILOG_DEBUG (LOG_MME_APP, "No S11 Create Bearer Procedure existing. Dropping the message for UE: " MME_UE_S1AP_ID_FMT "\n", e_rab_setup_rsp->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " S1AP_E_RAB_SETUP_RSP Missing procedure for ue " MME_UE_S1AP_ID_FMT "\n", e_rab_setup_rsp->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, e_rab_setup_rsp->mme_ue_s1ap_id);

  if (ue_context == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", e_rab_setup_rsp->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " S1AP_E_RAB_SETUP_RSP Unknown ue " MME_UE_S1AP_ID_FMT "\n", e_rab_setup_rsp->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Get the first PDN Context. */
  pdn_context_t * pdn_context = RB_MIN(PdnContexts, &ue_context->pdn_contexts);
  DevAssert(pdn_context);
  // todo: s1u_saegw_fteid should be set!


  bool  bearer_contexts_success = false;
  for (int i = 0; i < e_rab_setup_rsp->e_rab_setup_list.no_of_items; i++) {
    e_rab_id_t e_rab_id = e_rab_setup_rsp->e_rab_setup_list.item[i].e_rab_id;
    bearer_context_t * bc = NULL;
    mme_app_get_session_bearer_context_from_all(ue_context, (ebi_t) e_rab_id, &bc);
    if (bc->bearer_state & BEARER_STATE_SGW_CREATED) {


      // todo: need to check the bearer status! if NAS reject is received, put this into failed bearer list.
      // todo: s11_proc_create->bearer_status[EBI_TO_INDEX(ebi)]



      bc->enb_fteid_s1u.teid = e_rab_setup_rsp->e_rab_setup_list.item[i].gtp_teid;
      // Do not process transport_layer_address now
      //bstring e_rab_setup_rsp->e_rab_setup_list.item[i].transport_layer_address;
      ip_address_t enb_ip_address = {0};
      bstring_to_ip_address(e_rab_setup_rsp->e_rab_setup_list.item[i].transport_layer_address, &enb_ip_address);

      bc->enb_fteid_s1u.interface_type = S1_U_ENODEB_GTP_U;
      // TODO better than that later
      switch (enb_ip_address.pdn_type) {
      case IPv4:
        bc->enb_fteid_s1u.ipv4         = 1;
        bc->enb_fteid_s1u.ipv4_address = enb_ip_address.address.ipv4_address;
        break;
      case IPv6:
        bc->enb_fteid_s1u.ipv6         = 1;
        memcpy(&bc->enb_fteid_s1u.ipv6_address,
            &enb_ip_address.address.ipv6_address, sizeof(enb_ip_address.address.ipv6_address));
        break;
      default:
        AssertFatal(0, "Bug enb_ip_address->pdn_type");
      }
      bdestroy_wrapper (&e_rab_setup_rsp->e_rab_setup_list.item[i].transport_layer_address);

      AssertFatal(bc->bearer_state & BEARER_STATE_MME_CREATED, "TO DO check bearer state");
      bc->bearer_state |= BEARER_STATE_ENB_CREATED;
      bearer_contexts_success = true;
      if (ESM_EBR_ACTIVE == bc->esm_ebr_context.status) {
        send_s11_response = true;
        /** Add the bearer context to the list of established bearers. */
        LIST_INSERT_HEAD(s11_proc_create_bearer->bearer_contexts_success, bc, temp_entries);
      }else if (s11_proc_create_bearer->bearer_status[EBI_TO_INDEX(bc->ebi)] == S11_PROC_BEARER_FAILED){
        /** A NAS reject has been received for this bearer. */
        LIST_INSERT_HEAD(s11_proc_create_bearer->bearer_contexts_failed, bc, temp_entries);
      }
    }
  }

  for (int i = 0; i < e_rab_setup_rsp->e_rab_failed_to_setup_list.no_of_items; i++) {
    e_rab_id_t e_rab_id = e_rab_setup_rsp->e_rab_failed_to_setup_list.item[i].e_rab_id;
    bearer_context_t * bc = NULL;
    mme_app_get_session_bearer_context_from_all(ue_context, (ebi_t) e_rab_id, &bc);
    /** Deregister the MME_APP bearer context. */
    if (bc->bearer_state & BEARER_STATE_SGW_CREATED) {
      if(!bearer_contexts_success || send_s11_response) /**< If there were no successfull bearers or NAS response has been received, continue with Create Bearer Response. */
        send_s11_response = true; /**< Send directly a failure response.  todo: not waiting for NAS in this case? */
      //S1ap_Cause_t cause = e_rab_setup_rsp->e_rab_failed_to_setup_list.item[i].cause;
      AssertFatal(bc->bearer_state & BEARER_STATE_MME_CREATED, "TO DO check bearer state");
      bc->bearer_state &= (~BEARER_STATE_ENB_CREATED);
      bc->bearer_state &= (~BEARER_STATE_MME_CREATED);
      /** Add the bearer context to the list of rejected bearers. */
      LIST_INSERT_HEAD(s11_proc_create_bearer->bearer_contexts_failed, bc, temp_entries);
    }
  }

  // check if UE already responded with NAS (may depend on eNB implementation?) -> send response to SGW
  if (send_s11_response) {
    mme_app_send_s11_create_bearer_rsp(ue_context, pdn_context->s_gw_teid_s11_s4, s11_proc_create_bearer->bearer_contexts_success, s11_proc_create_bearer->bearer_contexts_failed);
    /** Delete the procedure if it exists. */
    mme_app_delete_s11_procedure_create_bearer(ue_context);
  }else{
    OAILOG_WARNING(LOG_MME_APP, "Not sending S11 Create Bearer Response to UE: " MME_UE_S1AP_ID_FMT "\n", ue_context->mme_ue_s1ap_id);
    /** With ESM response, we will send the bearers. */
  }

  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_handle_create_dedicated_bearer_rsp (itti_mme_app_create_dedicated_bearer_rsp_t   * const create_dedicated_bearer_rsp)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                 *ue_context = NULL;
  /** Get the first PDN Context. */
  pdn_context_t * pdn_context = RB_MIN(PdnContexts, &ue_context->pdn_contexts);
  DevAssert(pdn_context);

  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, create_dedicated_bearer_rsp->ue_id);

  if (ue_context == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", create_dedicated_bearer_rsp->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /*
   * TS 23.401: 5.4.1: The MME shall be prepared to receive this message either before or after the Session Management Response message (sent in step 9).
   *
   * The itti message will be sent directly by MME APP or by ESM layer, depending on the order of the messages.
   *
   * So we check the state of the bearers.
   * The indication that S11 Create Bearer Response should be sent to the SAE-GW, should be sent by ESM/NAS layer.
   */
  mme_app_s11_proc_create_bearer_t *s11_proc_create_bearer = mme_app_get_s11_procedure_create_bearer(ue_context);
  if (s11_proc_create_bearer) {
    ebi_t ebi = create_dedicated_bearer_rsp->ebi;
    s11_proc_create_bearer->num_status_received++;
    s11_proc_create_bearer->bearer_status[EBI_TO_INDEX(ebi)] = S11_PROC_BEARER_SUCCESS;
    // if received all bearers creation results
    if (s11_proc_create_bearer->num_status_received == s11_proc_create_bearer->num_bearers) { /**< If all NAS responses have arrived. Additionally check if the lists are filled. */
      if(s11_proc_create_bearer->bearer_contexts_failed || s11_proc_create_bearer->bearer_contexts_success){
//        mme_app_s11_procedure_create_bearer_send_response(ue_context, s11_proc_create);
        mme_app_send_s11_create_bearer_rsp(ue_context, pdn_context->s_gw_teid_s11_s4, s11_proc_create_bearer->bearer_contexts_success, s11_proc_create_bearer->bearer_contexts_failed);
        mme_app_delete_s11_procedure_create_bearer(ue_context);
      }else{
        OAILOG_WARNING(LOG_MME_APP, "No response from S1AP received yet for UE: " MME_UE_S1AP_ID_FMT "\n", create_dedicated_bearer_rsp->ue_id);
      }
    }
  }else{
    /** No S11 CBReq procedure exists. We already might have dealt with it. */
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_handle_create_dedicated_bearer_rej (itti_mme_app_create_dedicated_bearer_rej_t   * const create_dedicated_bearer_rej)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                 *ue_context = NULL;
  /** Get the first PDN Context. */
  pdn_context_t * pdn_context = RB_MIN(PdnContexts, &ue_context->pdn_contexts);
  DevAssert(pdn_context);

  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, create_dedicated_bearer_rej->ue_id);

  if (ue_context == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", create_dedicated_bearer_rej->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /*
   * TS 23.401: 5.4.1: The MME shall be prepared to receive this message either before or after the Session Management Response message (sent in step 9).
   *
   * The itti message will be sent directly by MME APP or by ESM layer, depending on the order of the messages.
   *
   * So we check the state of the bearers.
   * The indication that S11 Create Bearer Response should be sent to the SAE-GW, should be sent by ESM/NAS layer.
   */
  mme_app_s11_proc_create_bearer_t *s11_proc_create_bearer = mme_app_get_s11_procedure_create_bearer(ue_context);

  if (s11_proc_create_bearer) {
    ebi_t ebi = create_dedicated_bearer_rej->ebi;
    s11_proc_create_bearer->num_status_received++;
    s11_proc_create_bearer->bearer_status[EBI_TO_INDEX(ebi)] = S11_PROC_BEARER_FAILED;
    // if received all bearers creation results
    if (s11_proc_create_bearer->num_status_received == s11_proc_create_bearer->num_bearers) { /**< If all NAS responses have arrived. Additionally check if the lists are filled. */
      if(s11_proc_create_bearer->bearer_contexts_failed || s11_proc_create_bearer->bearer_contexts_success){
        //        mme_app_s11_procedure_create_bearer_send_response(ue_context, s11_proc_create);
        mme_app_send_s11_create_bearer_rsp(ue_context, pdn_context->s_gw_teid_s11_s4, s11_proc_create_bearer->bearer_contexts_success, s11_proc_create_bearer->bearer_contexts_failed);
        mme_app_delete_s11_procedure_create_bearer(ue_context);
      }else{
        OAILOG_WARNING(LOG_MME_APP, "No response from S1AP received yet for UE: " MME_UE_S1AP_ID_FMT "\n", create_dedicated_bearer_rej->ue_id);
      }
    }
  }else{
    /** No S11 CBReq procedure exists. We already might have dealt with it. */
  }

  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
// See 3GPP TS 23.401 version 10.13.0 Release 10: 5.4.4.2 MME Initiated Dedicated Bearer Deactivation
void mme_app_trigger_mme_initiated_dedicated_bearer_deactivation_procedure (ue_context_t * const ue_context, const pdn_cid_t cid)
{
  OAILOG_DEBUG (LOG_MME_APP, "TODO \n");
}

//------------------------------------------------------------------------------
// HANDOVER MESSAGING ------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 * Send an S1AP Handover Preparation Failure to the S1AP layer.
 * Not triggering release of resources, everything will stay as it it.
 * The MME_APP ITTI message elements though need to be deallocated.
 */
static
void mme_app_send_s1ap_handover_preparation_failure(mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id, sctp_assoc_id_t assoc_id, enum s1cause cause){
  OAILOG_FUNC_IN (LOG_MME_APP);
  /** Send a S1AP HANDOVER PREPARATION FAILURE TO THE SOURCE ENB. */
  MessageDef * message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_HANDOVER_PREPARATION_FAILURE);
  DevAssert (message_p != NULL);
  DevAssert(cause != S1AP_SUCCESSFUL_HANDOVER);

  itti_s1ap_handover_preparation_failure_t *s1ap_handover_preparation_failure_p = &message_p->ittiMsg.s1ap_handover_preparation_failure;
  memset ((void*)s1ap_handover_preparation_failure_p, 0, sizeof (itti_s1ap_handover_preparation_failure_t));

  /** Set the identifiers. */
  s1ap_handover_preparation_failure_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  s1ap_handover_preparation_failure_p->enb_ue_s1ap_id = enb_ue_s1ap_id;
  s1ap_handover_preparation_failure_p->assoc_id = assoc_id;
  /** Set the negative cause. */
  s1ap_handover_preparation_failure_p->cause = cause;

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "MME_APP Sending S1AP HANDOVER_PREPARATION_FAILURE");
  /** Sending a message to S1AP. */
  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Method to send the path switch request acknowledge to the target-eNB.
 * Will not make any changes in the UE context.
 * No timer to be started.
 */
static
void mme_app_send_s1ap_path_switch_request_acknowledge(mme_ue_s1ap_id_t mme_ue_s1ap_id){
  MessageDef * message_p = NULL;
  bearer_context_t                       *current_bearer_p = NULL;
  ebi_t                                   bearer_id = 0;
  ue_context_t                           *ue_context = NULL;
  emm_data_context_t                     *ue_nas_ctx = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id);

  if (ue_context == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "UE context doesn't exist for UE %06" PRIX32 "/dec%u\n", mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Get the EMM Context too for the AS security parameters. */
  ue_nas_ctx = emm_data_context_get(&_emm_data, mme_ue_s1ap_id);
  if(!ue_nas_ctx || ue_nas_ctx->_tai_list.numberoflists == 0){
    DevMessage(" No EMM Data Context exists for UE with mmeUeS1apId " + mme_ue_s1ap_id + ".\n");
  }

  /**
   * Prepare a S1AP ITTI message without changing the UE context.
   */
  message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_PATH_SWITCH_REQUEST_ACKNOWLEDGE);
  DevAssert (message_p != NULL);
  itti_s1ap_path_switch_request_ack_t *path_switch_req_ack_p = &message_p->ittiMsg.s1ap_path_switch_request_ack;
  memset (path_switch_req_ack_p, 0, sizeof (itti_s1ap_path_switch_request_ack_t));
  path_switch_req_ack_p->ue_id= mme_ue_s1ap_id;
  OAILOG_INFO(LOG_MME_APP, "Sending S1AP Path Switch Request Acknowledge to the target eNodeB for UE " MME_UE_S1AP_ID_FMT ". \n", mme_ue_s1ap_id);
  /** The ENB_ID/Stream information in the UE_Context are still the ones for the source-ENB and the SCTP-UE_ID association is not set yet for the new eNB. */
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0, "MME_APP Sending S1AP PATH_SWITCH_REQUEST_ACKNOWLEDGE.");

//  todo: multiApn X2 handover
//  bearer_id = ue_context->default_bearer_id;
//  current_bearer_p =  mme_app_is_bearer_context_in_list(ue_context->mme_ue_s1ap_id, bearer_id);
//  path_switch_req_ack_p->eps_bearer_id = bearer_id;

  /** Set all active bearers to be switched. */
//  path_switch_req_ack_p->bearer_ctx_to_be_switched_list.num_bearer_context = ue_context->nb_ue_bearer_ctxs;
//  path_switch_req_ack_p->bearer_ctx_to_be_switched_list.bearer_contexts[0]= (void*)&ue_context->bearer_ctxs;
//
//  hash_table_ts_t * bearer_contexts1 = (hash_table_ts_t*)path_switch_req_ack_p->bearer_ctx_to_be_switched_list.bearer_ctxs;

  uint16_t encryption_algorithm_capabilities = 0;
  uint16_t integrity_algorithm_capabilities = 0;

  emm_data_context_get_security_parameters(mme_ue_s1ap_id, &encryption_algorithm_capabilities, &integrity_algorithm_capabilities);
  /** Set the new security parameters. */
  path_switch_req_ack_p->security_capabilities_encryption_algorithms = encryption_algorithm_capabilities;
  path_switch_req_ack_p->security_capabilities_integrity_algorithms  = integrity_algorithm_capabilities;

  /** Set the next hop value and the NCC value. */
  memcpy(path_switch_req_ack_p->nh, ue_nas_ctx->_security.nh_conj, AUTH_NH_SIZE);
  path_switch_req_ack_p->ncc = ue_nas_ctx->_security.ncc;

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0,
      "0 S1AP_PATH_SWITCH_REQUEST_ACKNOWLEDGE ebi %u sea 0x%x sia 0x%x ncc %d",
      path_switch_req_ack_p->eps_bearer_id,
      path_switch_req_ack_p->security_capabilities_encryption_algorithms, path_switch_req_ack_p->security_capabilities_integrity_algorithms,
      path_switch_req_ack_p->ncc);
  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);

  /**
   * Change the ECM state to connected.
   * AN UE_Reference should already be created with Path_Switch_Request.
   */
  mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_CONNECTED);

  // todo: timer for path switch request
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Send an S1AP Path Switch Request Failure to the S1AP layer.
 * Not triggering release of resources, everything will stay as it it.
 * The MME_APP ITTI message elements though need to be deallocated.
 */
static
void mme_app_send_s1ap_path_switch_request_failure(mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id, sctp_assoc_id_t assoc_id, enum s1cause cause){
  OAILOG_FUNC_IN (LOG_MME_APP);
  /** Send a S1AP Path Switch Request Failure TO THE TARGET ENB. */
  MessageDef * message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_PATH_SWITCH_REQUEST_FAILURE);
  DevAssert (message_p != NULL);
  DevAssert(cause != S1AP_SUCCESSFUL_HANDOVER);

  itti_s1ap_path_switch_request_failure_t *s1ap_path_switch_request_failure_p = &message_p->ittiMsg.s1ap_path_switch_request_failure;
  memset ((void*)s1ap_path_switch_request_failure_p, 0, sizeof (itti_s1ap_path_switch_request_failure_t));

  /** Set the identifiers. */
  s1ap_path_switch_request_failure_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  s1ap_path_switch_request_failure_p->enb_ue_s1ap_id = enb_ue_s1ap_id;
  s1ap_path_switch_request_failure_p->assoc_id = assoc_id; /**< To whatever the new SCTP association is. */
  /** Set the negative cause. */
  s1ap_path_switch_request_failure_p->cause = cause;

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "MME_APP Sending S1AP PATH_SWITCH_REQUEST_FAILURE");
  /** Sending a message to S1AP. */
  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_path_switch_req(
  const itti_s1ap_path_switch_request_t * const s1ap_path_switch_req
  )
{
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_PATH_SWITCH_REQUEST from S1AP\n");
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, s1ap_path_switch_req->mme_ue_s1ap_id);

  if (ue_context == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: %08x %d(dec)\n", s1ap_path_switch_req->mme_ue_s1ap_id, s1ap_path_switch_req->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_PATH_SWITCH_REQUEST Unknown ue %u", s1ap_path_switch_req->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  enb_s1ap_id_key_t                       enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;

  /** Update the ENB_ID_KEY. */
  MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, s1ap_path_switch_req->enb_id, s1ap_path_switch_req->enb_ue_s1ap_id);
  // Update enb_s1ap_id_key in hashtable
  mme_ue_context_update_coll_keys( &mme_app_desc.mme_ue_contexts,
      ue_context,
      enb_s1ap_id_key,
      ue_context->mme_ue_s1ap_id,
      ue_context->imsi,
      ue_context->mme_teid_s11,
      ue_context->local_mme_teid_s10,
      &ue_context->guti);

  // Set the handover flag, check that no handover exists.
  ue_context->enb_ue_s1ap_id    = s1ap_path_switch_req->enb_ue_s1ap_id;
  ue_context->sctp_assoc_id_key = s1ap_path_switch_req->sctp_assoc_id;
  //  sctp_stream_id_t        sctp_stream;
  uint16_t encryption_algorithm_capabilities;
  uint16_t integrity_algorithm_capabilities;
  // todo: update them from the X2 message!
  if(emm_data_context_update_security_parameters(s1ap_path_switch_req->mme_ue_s1ap_id, &encryption_algorithm_capabilities, &integrity_algorithm_capabilities) != RETURNok){
    OAILOG_ERROR(LOG_MME_APP, "Error updating AS security parameters for UE with ueId: " MME_UE_S1AP_ID_FMT ". \n", s1ap_path_switch_req->mme_ue_s1ap_id);
    mme_app_send_s1ap_path_switch_request_failure(s1ap_path_switch_req->mme_ue_s1ap_id, s1ap_path_switch_req->enb_ue_s1ap_id, s1ap_path_switch_req->sctp_assoc_id, SYSTEM_FAILURE);
    /** Implicitly detach the UE --> If EMM context is missing, still continue with the resource removal. */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->mme_ue_s1ap_id;
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
 OAILOG_INFO(LOG_MME_APP, "Successfully updated AS security parameters for UE with ueId: " MME_UE_S1AP_ID_FMT " for X2 handover. \n", s1ap_path_switch_req->mme_ue_s1ap_id);

 // Stop Initial context setup process guard timer,if running todo: path switch request?
  if (ue_context->path_switch_req_timer.id != MME_APP_TIMER_INACTIVE_ID) {
    if (timer_remove(ue_context->path_switch_req_timer.id, NULL)) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to stop Path Switch Request timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
    }
    ue_context->path_switch_req_timer.id = MME_APP_TIMER_INACTIVE_ID;
  }

  // todo: multiApn X2 handover!
  pdn_context_t * first_pdn = RB_MIN(PdnContexts, &ue_context->pdn_contexts);
  DevAssert(first_pdn);

  message_p = itti_alloc_new_message (TASK_MME_APP, S11_MODIFY_BEARER_REQUEST);
  AssertFatal (message_p , "itti_alloc_new_message Failed");
  itti_s11_modify_bearer_request_t *s11_modify_bearer_request = &message_p->ittiMsg.s11_modify_bearer_request;
  memset ((void *)s11_modify_bearer_request, 0, sizeof (*s11_modify_bearer_request));
  s11_modify_bearer_request->peer_ip.s_addr = first_pdn->s_gw_address_s11_s4.address.ipv4_address.s_addr;
  s11_modify_bearer_request->teid = first_pdn->s_gw_teid_s11_s4;
  s11_modify_bearer_request->local_teid = ue_context->mme_teid_s11;
  /*
   * Delay Value in integer multiples of 50 millisecs, or zero
   */
  s11_modify_bearer_request->delay_dl_packet_notif_req = 0;  // TO DO
  s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].eps_bearer_id = s1ap_path_switch_req->eps_bearer_id;
  memcpy (&s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].s1_eNB_fteid,
      &s1ap_path_switch_req->bearer_s1u_enb_fteid,
      sizeof (s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].s1_eNB_fteid));
  s11_modify_bearer_request->bearer_contexts_to_be_modified.num_bearer_context = 1;

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

  // todo: since PSReq is already received from B-COM just set a flag (ask Lionel how to do it better).
  ue_context->pending_x2_handover = true;
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Send an S10 Forward Relocation response with error cause.
 * It shall not trigger creating a local S10 tunnel.
 * Parameter is the TEID & IP of the SOURCE-MME.
 */
static
void mme_app_send_s10_forward_relocation_response_err(teid_t mme_source_s10_teid, struct in_addr mme_source_ipv4_address, void *trxn,  gtpv2c_cause_value_t gtpv2cCause){
  OAILOG_FUNC_IN (LOG_MME_APP);

  /** Send a Forward Relocation RESPONSE with error cause: RELOCATION_FAILURE. */
  MessageDef * message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_RELOCATION_RESPONSE);
  DevAssert (message_p != NULL);

  itti_s10_forward_relocation_response_t *forward_relocation_response_p = &message_p->ittiMsg.s10_forward_relocation_response;
  memset ((void*)forward_relocation_response_p, 0, sizeof (itti_s10_forward_relocation_response_t));

  /**
   * Set the TEID of the source MME.
   * No need to set local_teid since no S10 tunnel will be created in error case.
   */
  forward_relocation_response_p->teid = mme_source_s10_teid;
  /** Set the IPv4 address of the source MME. */
  forward_relocation_response_p->peer_ip = mme_source_ipv4_address;
  forward_relocation_response_p->cause.cause_value = gtpv2cCause;
  forward_relocation_response_p->trxn  = trxn;

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "MME_APP Sending S10 FORWARD_RELOCATION_RESPONSE_ERR to source TEID " TEID_FMT ". \n", mme_source_s10_teid);

  /** Sending a message to S10. */
  itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_handover_required(
     itti_s1ap_handover_required_t * const handover_required_pP
    )   {
  OAILOG_FUNC_IN (LOG_MME_APP);

  emm_data_context_t                     *ue_nas_ctx = NULL;
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p  = NULL;

  OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_HANDOVER_REQUIRED from S1AP\n");
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, handover_required_pP->mme_ue_s1ap_id);

  if (ue_context == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: %08x %d(dec)\n", handover_required_pP->mme_ue_s1ap_id, handover_required_pP->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_REQUIRED Unknown ue %u", handover_required_pP->mme_ue_s1ap_id);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
    /* Resources will be removed by itti removal function. */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  if (ue_context->mm_state != UE_REGISTERED) {
    OAILOG_ERROR (LOG_MME_APP, "UE with ue_id " MME_UE_S1AP_ID_FMT " is not in UE_REGISTERED state. "
        "Rejecting the Handover Preparation. \n", ue_context->mme_ue_s1ap_id);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
    /** No change in the UE context needed. */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /*
   * Staying in the same state (UE_REGISTERED).
   * If the GUTI is not found, first send a preparation failure, then implicitly detach.
   */
  ue_nas_ctx = emm_data_context_get_by_guti (&_emm_data, &ue_context->guti);
  /** Check that the UE NAS context != NULL. */
  if (!ue_nas_ctx || emm_fsm_get_state (ue_nas_ctx) != EMM_REGISTERED) {
    OAILOG_ERROR (LOG_MME_APP, "EMM context for UE with ue_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " is not in EMM_REGISTERED state or not existing. "
        "Rejecting the Handover Preparation. \n", handover_required_pP->mme_ue_s1ap_id, ue_context->imsi);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /*
   * Check if there exists a handover process.
   * If so Set the target TAI and enb_id, to use them if a Handover-Cancel message comes.
   * Also use the S10 procedures for intra-MME handover.
   */
  mme_app_s10_proc_mme_handover_t * s10_handover_procedure = mme_app_get_s10_procedure_mme_handover(ue_context);
  if(s10_handover_procedure){
    OAILOG_ERROR (LOG_MME_APP, "EMM context for UE with ue_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " in EMM_REGISTERED state has a running handover procedure. "
        "Rejecting further procedures. \n", handover_required_pP->mme_ue_s1ap_id, ue_context->imsi);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /*
   * Update Security Parameters and send them to the target MME or target eNB.
   * This is same next hop procedure as in X2.
   */
  /** Get the updated security parameters from EMM layer directly. Else new states and ITTI messages are necessary. */
  uint16_t encryption_algorithm_capabilities;
  uint16_t integrity_algorithm_capabilities;
  if(emm_data_context_update_security_parameters(handover_required_pP->mme_ue_s1ap_id, &encryption_algorithm_capabilities, &integrity_algorithm_capabilities) != RETURNok){
    OAILOG_ERROR(LOG_MME_APP, "Error updating AS security parameters for UE with ueId: " MME_UE_S1AP_ID_FMT ". \n", handover_required_pP->mme_ue_s1ap_id);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
    /** If UE state is REGISTERED, then we also expect security context to be valid. */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  OAILOG_INFO(LOG_MME_APP, "Successfully updated AS security parameters for UE with ueId: " MME_UE_S1AP_ID_FMT ". \n", handover_required_pP->mme_ue_s1ap_id);
  /** Check if the destination eNodeB is attached at the same or another MME. */
  if (mme_app_check_ta_local(&handover_required_pP->selected_tai.plmn, handover_required_pP->selected_tai.tac)) {
    /** Check if the eNB with the given eNB-ID is served. */
    if(s1ap_is_enb_id_in_list(handover_required_pP->global_enb_id.cell_identity.enb_id) != NULL){
      OAILOG_DEBUG (LOG_MME_APP, "Target ENB_ID %d of target TAI " TAI_FMT " is served by current MME. \n",
          handover_required_pP->global_enb_id.cell_identity.enb_id, TAI_ARG(&handover_required_pP->selected_tai));
      /*
       * Create a new handover procedure and begin processing.
       */
      s10_handover_procedure = mme_app_create_s10_procedure_mme_handover(ue_context, false, MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER);
      if(!s10_handover_procedure){
        OAILOG_ERROR (LOG_MME_APP, "Could not create new handover procedure for UE with ue_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " in EMM_REGISTERED state. Rejecting further procedures. \n",
            handover_required_pP->mme_ue_s1ap_id, ue_context->imsi);
        mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
        OAILOG_FUNC_OUT (LOG_MME_APP);
      }
      /*
       * Fill the target information, and use it if handover cancellation is received.
       * No transaction needed on source side.
       * Keep the source side as it is.
       */
      memcpy((void*)&s10_handover_procedure->target_tai, (void*)&handover_required_pP->selected_tai, sizeof(handover_required_pP->selected_tai));
      memcpy((void*)&s10_handover_procedure->target_ecgi, (void*)&handover_required_pP->global_enb_id, sizeof(handover_required_pP->selected_tai)); /**< Home or macro enb id. */

      /*
       * Set the source values, too. We might need it if the MME_STATUS_TRANSFER has to be sent after Handover Notify (emulator errors).
       * Also, this may be used when removing the old ue_reference later.
       */
      s10_handover_procedure->source_enb_ue_s1ap_id = ue_context->enb_ue_s1ap_id;
      s10_handover_procedure->source_ecgi = ue_context->e_utran_cgi;
      /*
       * No need to store the transparent container. Directly send it to the target eNB.
       * Prepare a Handover Request, keep the transparent container for now, it will be purged together with the free method of the S1AP message.
       */
      /** Get all VOs of all session bearers and send handover request with it. */
      bearer_contexts_to_be_created_t bcs_tbc;
      memset((void*)&bcs_tbc, 0, sizeof(bcs_tbc));
      pdn_context_t * registered_pdn_ctx = NULL;
      RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_context->pdn_contexts) {
        DevAssert(registered_pdn_ctx);
        mme_app_get_bearer_contexts_to_be_created(registered_pdn_ctx, &bcs_tbc, BEARER_STATE_NULL);
        /** The number of bearers will be incremented in the method. S10 should just pick the ebi. */
      }

      mme_app_send_s1ap_handover_request(handover_required_pP->mme_ue_s1ap_id,
          &bcs_tbc,
          handover_required_pP->global_enb_id.cell_identity.enb_id,
          encryption_algorithm_capabilities,
          integrity_algorithm_capabilities,
          ue_nas_ctx->_vector[ue_nas_ctx->_security.vector_index].nh_conj,
          ue_nas_ctx->_security.ncc,
          handover_required_pP->eutran_source_to_target_container);
      /*
       * Unlink the transparent container. bstring is a pointer itself.
       * bdestroy_wrapper should be able to handle it.
       */
      handover_required_pP->eutran_source_to_target_container = NULL;
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }else{
      /** Send a Handover Preparation Failure back. */
      mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
      /** The target eNB-ID is not served by this MME. */
      OAILOG_DEBUG (LOG_MME_APP, "Target ENB_ID %d of target TAI " TAI_FMT " is NOT served by current MME. \n",
          handover_required_pP->global_enb_id.cell_identity.enb_id, TAI_ARG(&handover_required_pP->selected_tai));
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
  }
  OAILOG_DEBUG (LOG_MME_APP, "Target TA  "TAI_FMT " is NOT served by current MME. Searching for a neighboring MME. \n", TAI_ARG(&handover_required_pP->selected_tai));

  struct in_addr neigh_mme_ipv4_addr;
  neigh_mme_ipv4_addr.s_addr = 0;

  if (1) {
    // TODO prototype may change
    mme_app_select_service(&handover_required_pP->selected_tai, &neigh_mme_ipv4_addr);
    //    session_request_p->peer_ip.in_addr = mme_config.ipv4.
    if(neigh_mme_ipv4_addr.s_addr == 0){
      /** Send a Handover Preparation Failure back. */
      mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
      OAILOG_DEBUG (LOG_MME_APP, "The selected TAI " TAI_FMT " is not configured as an S10 MME neighbor. "
          "Not proceeding with the handover formme_ue_s1ap_id in list of UE: %08x %d(dec)\n",
          TAI_ARG(&handover_required_pP->selected_tai), handover_required_pP->mme_ue_s1ap_id, handover_required_pP->mme_ue_s1ap_id);
      MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_REQUIRED Unknown ue %u", handover_required_pP->mme_ue_s1ap_id);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
  }

  /** Prepare a forward relocation message to the TARGET-MME. */
  message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_RELOCATION_REQUEST);
  DevAssert (message_p != NULL);
  itti_s10_forward_relocation_request_t *forward_relocation_request_p = &message_p->ittiMsg.s10_forward_relocation_request;
  memset ((void*)forward_relocation_request_p, 0, sizeof (itti_s10_forward_relocation_request_t));
  forward_relocation_request_p->teid = 0;
  forward_relocation_request_p->peer_ip.s_addr = neigh_mme_ipv4_addr.s_addr;
  /** Add it into the procedure (todo: hardcoded to ipv4). */
//  s10_handover_procedure->remote_mme_teid.ipv4 = 1;
//  s10_handover_procedure->remote_mme_teid.interface_type = S10_MME_GTP_C;
  /** IMSI. */
  memcpy((void*)&forward_relocation_request_p->imsi, &ue_nas_ctx->_imsi, sizeof(imsi_t));
  // message content was set to 0
//  forward_relocation_request_p->imsi.length = strlen ((const char *)forward_relocation_request_p->imsi.u.value);
  // message content was set to 0

  /*
   * Create a new handover procedure and begin processing.
   */
  s10_handover_procedure = mme_app_create_s10_procedure_mme_handover(ue_context, false, MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER);
  if(!s10_handover_procedure){
    OAILOG_ERROR (LOG_MME_APP, "Could not create new handover procedure for UE with ue_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " in EMM_REGISTERED state. Rejecting further procedures. \n",
        handover_required_pP->mme_ue_s1ap_id, ue_context->imsi);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /*
   * Fill the target information, and use it if handover cancellation is received.
   * No transaction needed on source side.
   * Keep the source side as it is.
   */
  memcpy((void*)&s10_handover_procedure->target_tai, (void*)&handover_required_pP->selected_tai, sizeof(handover_required_pP->selected_tai));
  memcpy((void*)&s10_handover_procedure->target_ecgi, (void*)&handover_required_pP->global_enb_id, sizeof(handover_required_pP->selected_tai)); /**< Home or macro enb id. */

  /*
   * Set the source values, too. We might need it if the MME_STATUS_TRANSFER has to be sent after Handover Notify (emulator errors).
   * Also, this may be used when removing the old ue_reference later.
   */
  s10_handover_procedure->source_enb_ue_s1ap_id = ue_context->enb_ue_s1ap_id;
  s10_handover_procedure->source_ecgi = ue_context->e_utran_cgi;

  if(!ue_context->local_mme_teid_s10){
    /** Set the Source MME_S10_FTEID the same as in S11. */
    teid_t local_teid = 0x0;
    do{
      local_teid = (teid_t) (rand() % 0xFFFFFFFF);
    }while(mme_ue_context_exists_s10_teid(&mme_app_desc.mme_ue_contexts, local_teid) != NULL);

    OAI_GCC_DIAG_OFF(pointer-to-int-cast);
    forward_relocation_request_p->s10_source_mme_teid.teid = local_teid;
    OAI_GCC_DIAG_ON(pointer-to-int-cast);
    forward_relocation_request_p->s10_source_mme_teid.interface_type = S10_MME_GTP_C;
    mme_config_read_lock (&mme_config);
    forward_relocation_request_p->s10_source_mme_teid.ipv4_address = mme_config.ipv4.s10;
    mme_config_unlock (&mme_config);
    forward_relocation_request_p->s10_source_mme_teid.ipv4 = 1;
    /**
     * Update the local_s10_key.
     * Not setting the key directly in the  ue_context structure. Only over this function!
     */
    mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
        ue_context->enb_s1ap_id_key,
        ue_context->mme_ue_s1ap_id,
        ue_context->imsi,
        ue_context->mme_teid_s11,       // mme_teid_s11 is new
        local_teid,       // set with forward_relocation_request!
        &ue_context->guti);
  }else{
    OAILOG_INFO (LOG_MME_APP, "A S10 Local TEID " TEID_FMT " already exists. Not reallocating for UE: %08x %d(dec)\n",
        ue_context->local_mme_teid_s10, handover_required_pP->mme_ue_s1ap_id, handover_required_pP->mme_ue_s1ap_id);
    OAI_GCC_DIAG_OFF(pointer-to-int-cast);
    forward_relocation_request_p->s10_source_mme_teid.teid = ue_context->local_mme_teid_s10;
    OAI_GCC_DIAG_ON(pointer-to-int-cast);
    forward_relocation_request_p->s10_source_mme_teid.interface_type = S10_MME_GTP_C;
    mme_config_read_lock (&mme_config);
    forward_relocation_request_p->s10_source_mme_teid.ipv4_address = mme_config.ipv4.s10;
    mme_config_unlock (&mme_config);
    forward_relocation_request_p->s10_source_mme_teid.ipv4 = 1;
  }
  pdn_context_t * first_pdn = RB_MIN(PdnContexts, &ue_context->pdn_contexts);
  DevAssert(first_pdn);
  /** Set the SGW_S11_FTEID the same as in S11. */
  OAI_GCC_DIAG_OFF(pointer-to-int-cast);
  forward_relocation_request_p->s11_sgw_teid.teid = first_pdn->s_gw_teid_s11_s4;
  OAI_GCC_DIAG_ON(pointer-to-int-cast);
  forward_relocation_request_p->s11_sgw_teid.interface_type = S11_MME_GTP_C;
  mme_config_read_lock (&mme_config);
  forward_relocation_request_p->s11_sgw_teid.ipv4_address = mme_config.ipv4.s11;
  mme_config_unlock (&mme_config);
  forward_relocation_request_p->s11_sgw_teid.ipv4 = 1;

  /** Set the F-Cause. */
  forward_relocation_request_p->f_cause.fcause_type      = FCAUSE_S1AP;
  forward_relocation_request_p->f_cause.fcause_s1ap_type = FCAUSE_S1AP_RNL;
  forward_relocation_request_p->f_cause.fcause_value     = (uint8_t)handover_required_pP->f_cause_value;

  /** Set the target identification. */
  forward_relocation_request_p->target_identification.target_type = 1; /**< Macro eNodeB. */
  /** Set the MCC. */
  forward_relocation_request_p->target_identification.mcc[0]  = handover_required_pP->selected_tai.plmn.mcc_digit1;
  forward_relocation_request_p->target_identification.mcc[1]  = handover_required_pP->selected_tai.plmn.mcc_digit2;
  forward_relocation_request_p->target_identification.mcc[2]  = handover_required_pP->selected_tai.plmn.mcc_digit3;
  /** Set the MNC. */
  forward_relocation_request_p->target_identification.mnc[0]  = handover_required_pP->selected_tai.plmn.mnc_digit1;
  forward_relocation_request_p->target_identification.mnc[1]  = handover_required_pP->selected_tai.plmn.mnc_digit2;
  forward_relocation_request_p->target_identification.mnc[2]  = handover_required_pP->selected_tai.plmn.mnc_digit3;
  /* Set the Target Id.
   * todo: macro/home  */
  forward_relocation_request_p->target_identification.target_id.macro_enb_id.tac    = handover_required_pP->selected_tai.tac;
  forward_relocation_request_p->target_identification.target_id.macro_enb_id.enb_id = handover_required_pP->global_enb_id.cell_identity.enb_id;

  /** todo: Set the TAI and and the global-eNB-Id. */
  // memcpy(&forward_relocation_request_p->selected_tai, &handover_required_pP->selected_tai, sizeof(handover_required_pP->selected_tai));
  // memcpy(&forward_relocation_request_p->global_enb_id, &handover_required_pP->global_enb_id, sizeof(handover_required_pP->global_enb_id));

  /** Allocate and set the PDN_CONNECTIONS IE. */
  forward_relocation_request_p->pdn_connections = calloc(1, sizeof(struct mme_ue_eps_pdn_connections_s));
  mme_app_set_pdn_connections(forward_relocation_request_p->pdn_connections, ue_context);

  /** Set the MM_UE_EPS_CONTEXT. */
  forward_relocation_request_p->ue_eps_mm_context = calloc(1, sizeof(mm_context_eps_t));
  mme_app_set_ue_eps_mm_context(forward_relocation_request_p->ue_eps_mm_context, ue_context, ue_nas_ctx);

  /** Put the E-UTRAN transparent container. */
  forward_relocation_request_p->f_container.container_type = 3;
  forward_relocation_request_p->f_container.container_value = handover_required_pP->eutran_source_to_target_container;
  handover_required_pP->eutran_source_to_target_container = NULL;
  /** Send the Forward Relocation Message to S11. */
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME,  MSC_S10_MME ,
      NULL, 0, "0 FORWARD_RELOCATION_REQUEST for UE %d \n", handover_required_pP->mme_ue_s1ap_id);
  itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);

  /** No need to start/stop a timer on the source MME side. */
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_handover_cancel(
     const itti_s1ap_handover_cancel_t * const handover_cancel_pP
    )   {
  OAILOG_FUNC_IN (LOG_MME_APP);

  emm_data_context_t                     *emm_context = NULL;
  struct ue_context_s                    *ue_context  = NULL;
  MessageDef                             *message_p   = NULL;

  OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_HANDOVER_CANCEL from S1AP\n");
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, handover_cancel_pP->mme_ue_s1ap_id);
  if (ue_context == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: %08x %d(dec)\n", handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_CANCEL Unknown ue %u", handover_cancel_pP->mme_ue_s1ap_id);
    /** Ignoring the message. */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Check if there is a handover process. */
  mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
  if(!s10_handover_proc){
    OAILOG_ERROR (LOG_MME_APP, "UE with ue_id " MME_UE_S1AP_ID_FMT " has not an ongoing handover procedure on the (source) MME side. Sending Cancel Ack. \n", ue_context->mme_ue_s1ap_id);
    mme_app_send_s1ap_handover_cancel_acknowledge(handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->enb_ue_s1ap_id, handover_cancel_pP->assoc_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Check that the UE is in registered state. */
  if (ue_context->mm_state != UE_REGISTERED) {
    OAILOG_ERROR (LOG_MME_APP, "UE with ue_id " MME_UE_S1AP_ID_FMT " is not in UE_REGISTERED state on the (source) MME side. Sending Cancel Ack. \n", ue_context->mme_ue_s1ap_id);
    mme_app_send_s1ap_handover_cancel_acknowledge(handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->enb_ue_s1ap_id, handover_cancel_pP->assoc_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /**
   * Staying in the same state (UE_REGISTERED).
   * If the GUTI is not found, first send a preparation failure, then implicitly detach.
   */
  emm_context = emm_data_context_get_by_guti (&_emm_data, &ue_context->guti);
  /** Check that the UE NAS context != NULL. */
  DevAssert(emm_context && emm_fsm_get_state (emm_context) == EMM_REGISTERED);
  /*
   * UE will always stay in EMM-REGISTERED mode until S10 Handover is completed (CANCEL_LOCATION_REQUEST).
   * Just checking S10 is not enough. The UE may have handovered to the target-MME and then handovered to another eNB in the TARGET-MME.
   * Check if there is a target-TAI registered for the UE is in this or another MME.
   */
  /* Check if it is an intra- or inter-MME handover. */
  if(s10_handover_proc->proc.type == MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER){
    /*
     * Check if there already exists a UE-Reference to the target cell.
     * If so, this means that HANDOVER_REQUEST_ACKNOWLEDGE is already received.
     * It is so far gone in the handover process. We will send CANCEL-ACK and implicitly detach the UE.
     */
    if(s10_handover_proc->target_enb_ue_s1ap_id != 0 && ue_context->enb_ue_s1ap_id != s10_handover_proc->target_enb_ue_s1ap_id){
      // todo: macro/home enb_id
      ue_description_t * ue_reference = s1ap_is_enb_ue_s1ap_id_in_list_per_enb(s10_handover_proc->target_enb_ue_s1ap_id, s10_handover_proc->target_id.target_id.macro_enb_id.enb_id);
      if(ue_reference != NULL){
        /** UE Reference to the target eNB found. Sending a UE Context Release to the target MME BEFORE a HANDOVER_REQUEST_ACK arrives. */
        OAILOG_INFO(LOG_MME_APP, "Sending UE-Context-Release-Cmd to the target eNB %d for the UE-ID " MME_UE_S1AP_ID_FMT " and pending_enbUeS1apId " ENB_UE_S1AP_ID_FMT " (current enbUeS1apId) " ENB_UE_S1AP_ID_FMT ". \n.",
            s10_handover_proc->target_id.target_id.macro_enb_id.enb_id, ue_context->mme_ue_s1ap_id, s10_handover_proc->target_enb_ue_s1ap_id, ue_context->enb_ue_s1ap_id);
        ue_context->s1_ue_context_release_cause = S1AP_HANDOVER_CANCELLED;
        mme_app_itti_ue_context_release (ue_context->mme_ue_s1ap_id, s10_handover_proc->target_enb_ue_s1ap_id, S1AP_HANDOVER_CANCELLED, s10_handover_proc->target_id.target_id.macro_enb_id.enb_id);
        /**
         * No pending transparent container expected.
         * We directly respond with CANCEL_ACK, since when UE_CTX_RELEASE_CMPLT comes from targetEnb, we don't know if the current enb is target or source,
         * so we cannot decide there to send CANCEL_ACK without a flag.
         */
      }else{
        OAILOG_INFO(LOG_MME_APP, "No target UE reference is established yet. No S1AP UE context removal needed for UE-ID " MME_UE_S1AP_ID_FMT ". Responding with HO_CANCEL_ACK back to enbUeS1apId " ENB_UE_S1AP_ID_FMT ". \n.",
            ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id);
      }
    }
    /** Send a HO-CANCEL-ACK to the source-MME. */
    mme_app_send_s1ap_handover_cancel_acknowledge(handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->enb_ue_s1ap_id, handover_cancel_pP->assoc_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
    /** Delete the handover procedure. */
    // todo: aborting the handover procedure.
    mme_app_delete_s10_procedure_mme_handover(ue_context);
    /** Keeping the UE context as it is. */
  }else{
    /* Intra MME procedure.
     *
     * Target-TAI was not in the current MME. Sending a S10 Context Release Request.
     *
     * It may be that, the TEID is some other (the old MME it was handovered before from here.
     * So we need to check the TAI and find the correct neighboring MME.#
     * todo: to skip this step, we might set it back to 0 after S10-Complete for the previous Handover.
     */
    // todo: currently only a single neighboring MME supported.
    message_p = itti_alloc_new_message (TASK_MME_APP, S10_RELOCATION_CANCEL_REQUEST);
    DevAssert (message_p != NULL);
    itti_s10_relocation_cancel_request_t *relocation_cancel_request_p = &message_p->ittiMsg.s10_relocation_cancel_request;
    memset ((void*)relocation_cancel_request_p, 0, sizeof (itti_s10_relocation_cancel_request_t));
    relocation_cancel_request_p->teid = s10_handover_proc->remote_mme_teid.teid; /**< May or may not be 0. */
    relocation_cancel_request_p->local_teid = ue_context->local_mme_teid_s10; /**< May or may not be 0. */
    // todo: check the table!
    relocation_cancel_request_p->peer_ip.s_addr = s10_handover_proc->remote_mme_teid.ipv4_address.s_addr;
    /** IMSI. */
    IMSI64_TO_STRING (ue_context->imsi, (char *)relocation_cancel_request_p->imsi.u.value);
    // message content was set to 0
    relocation_cancel_request_p->imsi.length = strlen ((const char *)relocation_cancel_request_p->imsi.u.value);
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 RELOCATION_CANCEL_REQUEST_MESSAGE");
    itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
    OAILOG_DEBUG(LOG_MME_APP, "Successfully sent S10 RELOCATION_CANCEL_REQUEST to the target MME for the UE with IMSI " IMSI_64_FMT " and id " MME_UE_S1AP_ID_FMT ". "
        "Continuing with HO-CANCEL PROCEDURE. \n", ue_context->mme_ue_s1ap_id, ue_context->imsi);
    /** Delete the handover procedure. */
    // todo: aborting the handover procedure.
    mme_app_delete_s10_procedure_mme_handover(ue_context);
  }
  /*
   * Update the local S10 TEID.
   */
  mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
      INVALID_ENB_UE_S1AP_ID_KEY,
      ue_context->mme_ue_s1ap_id,
      ue_context->imsi,      /**< New IMSI. */
      ue_context->mme_teid_s11,
      INVALID_TEID,
      &ue_context->guti);
  /*
   * Not handling S10 response messages, to also drop other previous S10 messages (FW_RELOC_RSP).
   * Send a HO-CANCEL-ACK to the source-MME.
   */
  mme_app_send_s1ap_handover_cancel_acknowledge(handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->enb_ue_s1ap_id, handover_cancel_pP->assoc_id);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Method to send the S1AP MME Status Transfer to the target-eNB.
 * Will not make any changes in the UE context.
 * No F-Container will/needs to be stored temporarily.
 */
static
void mme_app_send_s1ap_mme_status_transfer(mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id, uint32_t enb_id, bstring source_to_target_cont){
  MessageDef * message_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  /**
   * Prepare a S1AP ITTI message without changing the UE context.
   */
  message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_MME_STATUS_TRANSFER);
  DevAssert (message_p != NULL);
  itti_s1ap_status_transfer_t *status_transfer_p = &message_p->ittiMsg.s1ap_mme_status_transfer;
  memset (status_transfer_p, 0, sizeof (itti_s1ap_status_transfer_t));
  status_transfer_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  status_transfer_p->enb_ue_s1ap_id = enb_ue_s1ap_id; /**< Just ENB_UE_S1AP_ID. */
  /** Set the current enb_id. */
  status_transfer_p->enb_id = enb_id;
  /** Set the E-UTRAN Target-To-Source-Transparent-Container. */
  status_transfer_p->bearerStatusTransferList_buffer = source_to_target_cont;
  // todo: what will the enb_ue_s1ap_ids for single mme s1ap handover will be.. ?
  OAILOG_INFO(LOG_MME_APP, "Sending S1AP MME_STATUS_TRANSFER command to the target eNodeB for enbUeS1apId " ENB_UE_S1AP_ID_FMT " and enbId %d. \n", enb_ue_s1ap_id, enb_id);
  /** The ENB_ID/Stream information in the UE_Context are still the ones for the source-ENB and the SCTP-UE_ID association is not set yet for the new eNB. */
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0, "MME_APP Sending S1AP MME_STATUS_TRANSFER.");
  /** Sending a message to S1AP. */
  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_forward_relocation_request(
     itti_s10_forward_relocation_request_t * const forward_relocation_request_pP
    )
{
 MessageDef                             *message_p  = NULL;
 struct ue_context_s                    *ue_context = NULL;
 uint64_t                                imsi = 0;
 mme_app_s10_proc_mme_handover_t        *s10_proc_mme_handover = NULL;
 int                                     rc = RETURNok;

 OAILOG_FUNC_IN (LOG_MME_APP);

 AssertFatal ((forward_relocation_request_pP->imsi.length > 0)
     && (forward_relocation_request_pP->imsi.length < 16), "BAD IMSI LENGTH %d", forward_relocation_request_pP->imsi.length);
 AssertFatal ((forward_relocation_request_pP->imsi.length > 0)
     && (forward_relocation_request_pP->imsi.length < 16), "STOP ON IMSI LENGTH %d", forward_relocation_request_pP->imsi.length);

 imsi = imsi_to_imsi64(&forward_relocation_request_pP->imsi);
 OAILOG_DEBUG (LOG_MME_APP, "Handling FORWARD_RELOCATION REQUEST for imsi " IMSI_64_FMT ". \n", imsi);

 /** Everything stack to this point. */
 /** Check that the TAI & PLMN are actually served. */
 tai_t target_tai;
 memset(&target_tai, 0, sizeof (tai_t));
 target_tai.plmn.mcc_digit1 = forward_relocation_request_pP->target_identification.mcc[0];
 target_tai.plmn.mcc_digit2 = forward_relocation_request_pP->target_identification.mcc[1];
 target_tai.plmn.mcc_digit3 = forward_relocation_request_pP->target_identification.mcc[2];
 target_tai.plmn.mnc_digit1 = forward_relocation_request_pP->target_identification.mnc[0];
 target_tai.plmn.mnc_digit2 = forward_relocation_request_pP->target_identification.mnc[1];
 target_tai.plmn.mnc_digit3 = forward_relocation_request_pP->target_identification.mnc[2];
 // todo: macro/home enb_id
 target_tai.tac = forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac;

 /** Check the target-ENB is reachable. */
 if (mme_app_check_ta_local(&target_tai.plmn, forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac)) {
   /** The target PLMN and TAC are served by this MME. */
   OAILOG_DEBUG (LOG_MME_APP, "Target TAC " TAC_FMT " is served by current MME. \n", forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac);
   /*
    * Currently only a single TA will be served by each MME and we are expecting TAU from the UE side.
    * Check that the eNB is also served, that an SCTP association exists for the eNB.
    */
   if(s1ap_is_enb_id_in_list(forward_relocation_request_pP->target_identification.target_id.macro_enb_id.enb_id) != NULL){
     OAILOG_DEBUG (LOG_MME_APP, "Target ENB_ID %u is served by current MME. \n", forward_relocation_request_pP->target_identification.target_id.macro_enb_id.enb_id);
     /** Continue with the handover establishment. */
   }else{
     /** The target PLMN and TAC are not served by this MME. */
     OAILOG_ERROR(LOG_MME_APP, "Target ENB_ID %u is NOT served by the current MME. \n", forward_relocation_request_pP->target_identification.target_id.macro_enb_id.enb_id);
     mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, forward_relocation_request_pP->s10_source_mme_teid.ipv4_address, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
     OAILOG_FUNC_OUT (LOG_MME_APP);
   }
 }else{
   /** The target PLMN and TAC are not served by this MME. */
   OAILOG_ERROR(LOG_MME_APP, "TARGET TAC " TAC_FMT " is NOT served by current MME. \n", forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac);
   mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, forward_relocation_request_pP->s10_source_mme_teid.ipv4_address, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
   /** No UE context or tunnel endpoint is allocated yet. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** Check if the UE exists. */
 ue_context = mme_ue_context_exists_imsi(&mme_app_desc.mme_ue_contexts, imsi);
 if (ue_context != NULL) {
   OAILOG_WARNING(LOG_MME_APP, "An UE MME context for the UE with IMSI " IMSI_64_FMT " already exists. \n", imsi);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S10_FORWARD_RELOCATION_REQUEST. Already existing UE " IMSI_64_FMT, imsi);
   /** Check if a handover process is ongoing (too quick back handover. */
   s10_proc_mme_handover = mme_app_get_s10_procedure_mme_handover(ue_context);
   if(s10_proc_mme_handover){
     OAILOG_WARNING (LOG_MME_APP, "EMM context for UE with ue_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " in EMM_REGISTERED state has a running handover procedure. "
           "Rejecting further procedures. \n", ue_context->mme_ue_s1ap_id, ue_context->imsi);
     mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, forward_relocation_request_pP->s10_source_mme_teid.ipv4_address, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
     // todo: here abort the procedure! and continue with the handover
     // todo: mme_app_delete_s10_procedure_mme_handover(ue_context); /**< Should remove all pending data. */
     // todo: aborting should just clear all pending information
     // todo: additionally invalidate NAS below (if one exists)
     OAILOG_FUNC_OUT (LOG_MME_APP);
   }
//   /*
//    * Not stopping the MME_MOBILITY COMPLETION timer, if running, it would stop the S1AP release timer for the source MME side.
//    * We will just clear the pending CLR flag with the NAS invalidation. Old UE reference should still be removed but the CLR should be disregarded.
//    * If the CLR flag still arrives after that, we had bad luck, it will remove the context.
//    */
//   if (ue_context->mme_mobility_completion_timer.id != MME_APP_TIMER_INACTIVE_ID) {
//     OAILOG_INFO(LOG_MME_APP, "The MME mobility completion timer was set for UE with IMSI " IMSI_64_FMT " and " MME_UE_S1AP_ID_FMT ". Keeping it. \n", imsi, ue_context->mme_ue_s1ap_id);
//     /** Inform the S1AP layer that the UE context could be timeoutet. */
//   }
   /** Invalidate the enb_ue_s1ap_id and the key. */
   ue_context->enb_ue_s1ap_id = 0;
   /*
    * Update the coll_keys with the IMSI.
    */
   mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
       INVALID_ENB_UE_S1AP_ID_KEY,
       ue_context->mme_ue_s1ap_id,
       ue_context->imsi,      /**< New IMSI. */
       ue_context->mme_teid_s11,
       ue_context->local_mme_teid_s10,
       &ue_context->guti);

   /** Synchronously send a UE context Release Request (implicit - no response expected). */
   // todo: send S1AP_NETWORK_ERROR to UE
   mme_app_itti_ue_context_release (ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id, S1AP_NETWORK_ERROR, ue_context->e_utran_cgi.cell_identity.enb_id); /**< the source side should be ok. */
 }
 /** Check that also an NAS UE context exists. */
 emm_data_context_t *ue_nas_ctx = emm_data_context_get_by_imsi (&_emm_data, imsi);
 if (ue_nas_ctx) {
   OAILOG_INFO(LOG_MME_APP, "A valid  NAS context exist for UE with IMSI " IMSI_64_FMT " and " MME_UE_S1AP_ID_FMT " already, but this re-registration part is not implemented yet. \n", imsi, ue_nas_ctx->ue_id);
   /*
    * Check if an EMM-CN context request procedure exists, if so ignore the handover currently.
    * Aborting current handover procedure and continue with new one.
    * Both procedures won't exist simultaneously. If handover, EMM APP will use the S10 handover procedure, else, only emm_cn_proc_ctx_req will be used.
    * todo: test this!
    */
   nas_ctx_req_proc_t * emm_cn_proc_ctx_req = get_nas_cn_procedure_ctx_req(ue_nas_ctx);
   if(emm_cn_proc_ctx_req){
     OAILOG_WARNING(LOG_MME_APP, "A context request procedure already exists for UE " MME_UE_S1AP_ID_FMT " with IMSI " IMSI_64_FMT "in state %d. "
         "Aborting old context request procedure and continuing with new handover request with the implicit detach. \n.",
         ue_context->mme_ue_s1ap_id, ue_context->imsi, ue_context->mm_state);
     /** Cannot send EMMREG abort procedure signal to EMM, thatswhy implicitly detaching the old UE context. */
   }
   ue_context->s1_ue_context_release_cause = S1AP_INVALIDATE_NAS;  /**< This should remove the NAS context and invalidate the timers. */
   message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
   DevAssert (message_p != NULL);
   itti_nas_implicit_detach_ue_ind_t *nas_implicit_detach_ue_ind_p = &message_p->ittiMsg.nas_implicit_detach_ue_ind;
   memset ((void*)nas_implicit_detach_ue_ind_p, 0, sizeof (itti_nas_implicit_detach_ue_ind_t));
   message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_nas_ctx->ue_id;
   itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
   OAILOG_INFO(LOG_MME_APP, "Informed NAS about the invalidated NAS context. Continuing with handover for UE with IMSI " IMSI_64_FMT " and " MME_UE_S1AP_ID_FMT " already. \n", imsi, ue_nas_ctx->ue_id);
   // Keeps the UE context
 }else{
   OAILOG_WARNING(LOG_MME_APP, "No  valid UE context and NAS context exist for UE with IMSI " IMSI_64_FMT ". Continuing with the handover. \n", imsi);
 }
 OAILOG_INFO(LOG_MME_APP, "Received a FORWARD_RELOCATION_REQUEST for new UE with IMSI " IMSI_64_FMT ". \n", imsi);
 bool new_ue_context = false;
 if(!ue_context){
   /** Establish the UE context. */
   OAILOG_DEBUG (LOG_MME_APP, "Creating a new UE context for the UE with incoming S1AP Handover via S10 for IMSI " IMSI_64_FMT ". \n", imsi);
   if ((ue_context = mme_create_new_ue_context ()) == NULL) {
     /** Send a negative response before crashing. */
     mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, forward_relocation_request_pP->s10_source_mme_teid.ipv4_address, forward_relocation_request_pP->trxn, SYSTEM_FAILURE);
     /**
      * Error during UE context malloc
      */
     DevMessage ("Error while mme_create_new_ue_context");
     OAILOG_FUNC_OUT (LOG_MME_APP);
   }
   ue_context->mme_ue_s1ap_id = mme_app_ctx_get_new_ue_id ();
   if (ue_context->mme_ue_s1ap_id == INVALID_MME_UE_S1AP_ID) {
     OAILOG_CRITICAL (LOG_MME_APP, "MME_APP_FORWARD_RELOCATION_REQUEST. MME_UE_S1AP_ID allocation Failed.\n");
     /** Deallocate the ue context and remove from MME_APP map. */
     mme_remove_ue_context (&mme_app_desc.mme_ue_contexts, ue_context);
     /** Send back failure. */
     mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, forward_relocation_request_pP->s10_source_mme_teid.ipv4_address, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
     OAILOG_FUNC_OUT (LOG_MME_APP);
   }
   OAILOG_DEBUG (LOG_MME_APP, "MME_APP_INITIAL_UE_MESSAGE. Allocated new MME UE context and new mme_ue_s1ap_id. " MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
   /** Register the new MME_UE context into the map. */
   DevAssert (mme_insert_ue_context (&mme_app_desc.mme_ue_contexts, ue_context) == 0);
   /*
    * Update the coll_keys with the IMSI.
    */
   mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
       ue_context->enb_s1ap_id_key,
       ue_context->mme_ue_s1ap_id,
       imsi,      /**< New IMSI. */
       ue_context->mme_teid_s11,
       ue_context->local_mme_teid_s10,
       &ue_context->guti);
   new_ue_context = true;
 }

 /*
  * Create a new handover procedure, no matter a UE context exists or not.
  * Store the transaction in it.
  * Store all pending PDN connections in it.
  * Each time a CSResp for a specific APN arrives, send another CSReq if needed.
  */
 s10_proc_mme_handover = mme_app_create_s10_procedure_mme_handover(ue_context, true, MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER);
 DevAssert(s10_proc_mme_handover);

 /*
  * If we make the procedure a specific procedure for forward relocation request, then it would be removed with forward relocation response,
  * todo: where to save the mm_eps_context and s10 tunnel information,
  * Else, if it is a generic s10 handover procedure, we need to update the transaction with every time.
  */
 /** Fill the values of the s10 handover procedure (and decouple them, such that remain after the ITTI message is removed). */
 memcpy((void*)&s10_proc_mme_handover->target_id, (void*)&forward_relocation_request_pP->target_identification, sizeof(target_identification_t));
 memcpy((void*)&s10_proc_mme_handover->nas_s10_context._imsi, (void*)&forward_relocation_request_pP->imsi, sizeof(imsi_t));
 /** Set the target tai also in the target-MME side. */
 memcpy((void*)&s10_proc_mme_handover->target_tai, &target_tai, sizeof(tai_t));
 /** Set the IMSI. */

 s10_proc_mme_handover->nas_s10_context.mm_eps_ctx = forward_relocation_request_pP->ue_eps_mm_context;
 forward_relocation_request_pP->ue_eps_mm_context = NULL;
 s10_proc_mme_handover->source_to_target_eutran_f_container = forward_relocation_request_pP->f_container;
 forward_relocation_request_pP->f_container.container_value = NULL;

 s10_proc_mme_handover->f_cause = forward_relocation_request_pP->f_cause;

 /** Set the Source FTEID. */
 memcpy((void*)&s10_proc_mme_handover->remote_mme_teid, (void*)&forward_relocation_request_pP->s10_source_mme_teid, sizeof(fteid_t));

 s10_proc_mme_handover->peer_port = forward_relocation_request_pP->peer_port;
 s10_proc_mme_handover->forward_relocation_trxn = forward_relocation_request_pP->trxn;
 /** If it is a new_ue_context, additionally set the pdn_connections IE. */
 if(new_ue_context){
   s10_proc_mme_handover->pdn_connections = forward_relocation_request_pP->pdn_connections;
   forward_relocation_request_pP->pdn_connections = NULL; /**< Unlink the pdn_connections. */
   OAILOG_INFO(LOG_MME_APP, "UE_CONTEXT for UE " MME_UE_S1AP_ID_FMT " is a new UE_Context. Processing the received PDN_CONNECTIONS IEs (continuing with CSR). \n", ue_context->mme_ue_s1ap_id);
   /** Process PDN Connections IE. Will initiate a Create Session Request message for the pending pdn_connections. */
   pdn_connection_t * pdn_connection = &s10_proc_mme_handover->pdn_connections->pdn_connection[s10_proc_mme_handover->next_processed_pdn_connection];
   pdn_context_t * pdn_context = mme_app_handle_pdn_connectivity_from_s10(ue_context, pdn_connection);
   s10_proc_mme_handover->next_processed_pdn_connection++;
   /*
    * When Create Session Response is received, continue to process the next PDN connection, until all are processed.
    * When all pdn_connections are completed, continue with handover request.
    */
   mme_app_send_s11_create_session_req (ue_context, &s10_proc_mme_handover->nas_s10_context._imsi, pdn_context, &target_tai);
   OAILOG_INFO(LOG_MME_APP, "Successfully sent CSR for UE " MME_UE_S1AP_ID_FMT ". Waiting for CSResp to continue to process handover on source MME side. \n", ue_context->mme_ue_s1ap_id);
 }else{
   /** Ignore the received pdn_connections IE and continue straight with handover request. */
   OAILOG_INFO (LOG_MME_APP, "Continuing with already existing MME_APP UE_Context with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "Sending handover request. \n", ue_context->mme_ue_s1ap_id);
   s10_proc_mme_handover->next_processed_pdn_connection = s10_proc_mme_handover->pdn_connections->num_pdn_connections; /**< Just to be safe. */
   /*
    * A handover procedure also exists for this case.
    * A release request must already be sent to the source eNB.
    * Not updating the PDN connections IE for the UE.
    * Assuming that it is a too fast re-handover where it is not needed.
    */
   /** Get all VOs of all session bearers and send handover request with it. */
   bearer_contexts_to_be_created_t bcs_tbc;
   memset((void*)&bcs_tbc, 0, sizeof(bcs_tbc));
   pdn_context_t * registered_pdn_ctx = NULL;
   RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_context->pdn_contexts) {
     DevAssert(registered_pdn_ctx);
     mme_app_get_bearer_contexts_to_be_created(registered_pdn_ctx, &bcs_tbc, BEARER_STATE_NULL);
     /** The number of bearers will be incremented in the method. S10 should just pick the ebi. */
   }
   mme_app_send_s1ap_handover_request(ue_context->mme_ue_s1ap_id,
       &bcs_tbc,
       forward_relocation_request_pP->target_identification.target_id.macro_enb_id.enb_id,
       forward_relocation_request_pP->ue_eps_mm_context->ue_nc.eea,
       forward_relocation_request_pP->ue_eps_mm_context->ue_nc.eia,
       forward_relocation_request_pP->ue_eps_mm_context->nh,
       forward_relocation_request_pP->ue_eps_mm_context->ncc,
       s10_proc_mme_handover->source_to_target_eutran_f_container.container_value);

   s10_proc_mme_handover->source_to_target_eutran_f_container.container_value = NULL; /**< Set it to NULL ALWAYS. */

   /** Unlink the e-utran transparent container. */
 }
 ue_context->imsi_auth = IMSI_AUTHENTICATED;
 OAILOG_INFO (LOG_MME_APP, "Successfully created and initialized an S10 Inter-MME handover procedure for mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
/*
 * Send an S1AP Handover Request.
 */
static
void mme_app_send_s1ap_handover_request(mme_ue_s1ap_id_t mme_ue_s1ap_id,
    bearer_contexts_to_be_created_t *bcs_tbc,
    uint32_t                enb_id,
    uint16_t                encryption_algorithm_capabilities,
    uint16_t                integrity_algorithm_capabilities,
    uint8_t                 nh[AUTH_NH_SIZE],
    uint8_t                 ncc,
    bstring                 eutran_source_to_target_container){

  MessageDef                *message_p = NULL;
  ue_context_t              *ue_context = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);

  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id);
  DevAssert(ue_context); /**< Should always exist. Any mobility issue in which this could occur? */

  /** Get the EMM Context. */
  message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_HANDOVER_REQUEST);

  itti_s1ap_handover_request_t *handover_request_p = &message_p->ittiMsg.s1ap_handover_request;
  /** UE_ID. */
  handover_request_p->ue_id = mme_ue_s1ap_id;
  handover_request_p->macro_enb_id = enb_id;
  /** Handover Type & Cause will be set in the S1AP layer. */
  /** Set the AMBR Parameters. */
  handover_request_p->ambr.br_ul = ue_context->subscribed_ue_ambr.br_ul;
  handover_request_p->ambr.br_dl = ue_context->subscribed_ue_ambr.br_dl;

  /** Set the bearer contexts to be created. Not changing any bearer state. */
  handover_request_p->bearer_ctx_to_be_setup_list = calloc(1, sizeof(bearer_contexts_to_be_created_t));
  memcpy((void*)handover_request_p->bearer_ctx_to_be_setup_list, bcs_tbc, sizeof(*bcs_tbc));

  /** Set the Security Capabilities. */
  handover_request_p->security_capabilities_encryption_algorithms = encryption_algorithm_capabilities;
  handover_request_p->security_capabilities_integrity_algorithms  = integrity_algorithm_capabilities;
  /** Set the Security Context. */
  handover_request_p->ncc = ncc;
  memcpy(handover_request_p->nh, nh, AUTH_NH_SIZE);
  /** Set the Source-to-Target Transparent container from the pending information, which will be removed from the UE_Context. */
  handover_request_p->source_to_target_eutran_container = eutran_source_to_target_container;
  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_DEBUG (LOG_MME_APP, "Sending S1AP Handover Request message for UE "MME_UE_S1AP_ID_FMT ". \n.", mme_ue_s1ap_id);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Method to send the handover command to the source-eNB.
 * Will not make any changes in the UE context.
 * No F-Container will/needs to be stored temporarily.
 * No timer to be started.
 */
static
void mme_app_send_s1ap_handover_command(mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id, uint32_t enb_id, bstring target_to_source_cont){
  MessageDef * message_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  /**
   * Prepare a S1AP ITTI message without changing the UE context.
   */
  message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_HANDOVER_COMMAND);
  DevAssert (message_p != NULL);
  itti_s1ap_handover_command_t *handover_command_p = &message_p->ittiMsg.s1ap_handover_command;
  memset (handover_command_p, 0, sizeof (itti_s1ap_handover_command_t));
  handover_command_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  handover_command_p->enb_ue_s1ap_id = enb_ue_s1ap_id; /**< Just ENB_UE_S1AP_ID. */
  handover_command_p->enb_id = enb_id;
  /** Set the E-UTRAN Target-To-Source-Transparent-Container. */
  handover_command_p->eutran_target_to_source_container = target_to_source_cont;
  // todo: what will the enb_ue_s1ap_ids for single mme s1ap handover will be.. ?
  OAILOG_INFO(LOG_MME_APP, "Sending S1AP handover command to the source eNodeB for UE " MME_UE_S1AP_ID_FMT ". \n", mme_ue_s1ap_id);
  /** The ENB_ID/Stream information in the UE_Context are still the ones for the source-ENB and the SCTP-UE_ID association is not set yet for the new eNB. */
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0, "MME_APP Sending S1AP HANDOVER_COMMAND.");
  /** Sending a message to S1AP. */
  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_forward_relocation_response(
    itti_s10_forward_relocation_response_t* const forward_relocation_response_pP
    )
{
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p  = NULL;
  uint64_t                                imsi = 0;
  int16_t                                 bearer_id =0;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (forward_relocation_response_pP );

  ue_context = mme_ue_context_exists_s10_teid(&mme_app_desc.mme_ue_contexts, forward_relocation_response_pP->teid);

  if (ue_context == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 S10_FORWARD_RELOCATION_RESPONSE local S11 teid " TEID_FMT " ", forward_relocation_response_pP->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", forward_relocation_response_pP->teid);
    /**
     * The HANDOVER_NOTIFY timeout will purge any UE_Contexts, if exists.
     * Not manually purging anything.
     * todo: Not sending anything to the target side? (RELOCATION_FAILURE?)
     */
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 S10_FORWARD_RELOCATION_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      forward_relocation_response_pP->teid, ue_context->imsi);
  /*
   * Check that there is a s10 handover procedure.
   * If not, drop the message, don't care about target-MME.
   */
  mme_app_s10_proc_mme_handover_t * s10_handover_procedure = mme_app_get_s10_procedure_mme_handover(ue_context);
  if(!s10_handover_procedure){
    /** Deal with the error case. */
    OAILOG_ERROR(LOG_MME_APP, "No S10 Handover procedure for UE IMSI " IMSI_64_FMT " and mmeS1apUeId " MME_UE_S1AP_ID_FMT " in state %d. Dropping the ForwardRelResp. \n",
        ue_context->imsi, ue_context->mme_ue_s1ap_id, ue_context->mm_state);
    /** No implicit detach. */
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  if (forward_relocation_response_pP->cause.cause_value != REQUEST_ACCEPTED) {
    /**
     * We are in EMM-REGISTERED state, so we don't need to perform an implicit detach.
     * In the target side, we won't do anything. We assumed everything is taken care of (No Relocation Cancel Request to be sent).
     * No handover timers in the source-MME side exist.
     * We will only send an Handover Preparation message and leave the UE context as it is (including the S10 tunnel endpoint at the source-MME side).
     * The handover notify timer is only at the target-MME side. That's why, its not in the method below.
     */
    mme_app_send_s1ap_handover_preparation_failure(ue_context->mme_ue_s1ap_id,
        ue_context->enb_ue_s1ap_id, ue_context->sctp_assoc_id_key, RELOCATION_FAILURE);

    /** Delete the source handover procedure. Continue with the existing UE reference. */
    mme_app_delete_s10_procedure_mme_handover(ue_context);

    OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
  }
  /** We are the source-MME side. Store the counterpart as target-MME side. */
  memcpy((void*)&s10_handover_procedure->remote_mme_teid, (void*)&forward_relocation_response_pP->s10_target_mme_teid, sizeof(fteid_t));
  //---------------------------------------------------------
  // Process itti_s10_forward_relocation_response_t.bearer_context_admitted
  //---------------------------------------------------------

  /**
   * Iterate through the admitted bearer items.
   * Currently only EBI received.. but nothing is done with that.
   * todo: check that the bearer exists? bearer id may change? used for data forwarding?
   * todo: must check if any bearer exist at all? todo: must check that the number bearers is as expected?
   * todo: DevCheck ((bearer_id < BEARERS_PER_UE) && (bearer_id >= 0), bearer_id, BEARERS_PER_UE, 0);
   */
  bearer_id = forward_relocation_response_pP->handovered_bearers->bearer_contexts[0].eps_bearer_id /* - 5 */ ;
  // todo: what is the dumping doing? printing? needed for s10?

  /**
   * Not doing/storing anything in the NAS layer. Just sending handover command back.
   * Send a S1AP Handover Command to the source eNodeB.
   */
  OAILOG_INFO(LOG_MME_APP, "MME_APP UE context is in REGISTERED state. Sending a Handover Command to the source-ENB with enbId: %d for UE with mmeUeS1APId : " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT " after S10 Forward Relocation Response. \n",
      ue_context->e_utran_cgi.cell_identity.enb_id, ue_context->mme_ue_s1ap_id, ue_context->imsi);
  /** Send a Handover Command. */
  mme_app_send_s1ap_handover_command(ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id, ue_context->e_utran_cgi.cell_identity.enb_id, forward_relocation_response_pP->eutran_container.container_value);
  /** Unlink the container. */
  forward_relocation_response_pP->eutran_container.container_value = NULL;
  /**
   * No new UE identifier. We don't update the coll_keys.
   * As the specification said, we will leave the UE_CONTEXT as it is. Not checking further parameters.
   * No timers are started. Only timers are in the source-ENB and the custom new timer in the source MME.
   * ********************************************
   * The ECM state will not be changed.
   */
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_forward_access_context_notification(
    itti_s10_forward_access_context_notification_t* const forward_access_context_notification_pP
    )
{
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p  = NULL;
  uint64_t                                imsi = 0;
  int16_t                                 bearer_id =0;

  OAILOG_FUNC_IN (LOG_MME_APP);
  OAILOG_DEBUG (LOG_MME_APP, "Received S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION from S10. \n");
  DevAssert (forward_access_context_notification_pP );

  /** Here it checks the local TEID. */
  ue_context = mme_ue_context_exists_s10_teid (&mme_app_desc.mme_ue_contexts, forward_access_context_notification_pP->teid);

  if (ue_context == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION local S11 teid " TEID_FMT " ", forward_access_context_notification_pP->teid);
    /** We cannot send an S10 reject, since we don't have the destination TEID. */
    /**
     * todo: lionel
     * If we ignore the request (for which a transaction exits), and a second request arrives, there is a dev_assert..
     * therefore removing the transaction?
     */
    /** Not performing an implicit detach. The handover timeout should erase the context (incl. the S10 Tunnel Endpoint, which we don't erase here in the error case). */
    OAILOG_ERROR(LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", forward_access_context_notification_pP->teid);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  mme_app_s10_proc_mme_handover_t * s10_handover_process = mme_app_get_s10_procedure_mme_handover(ue_context);
  if(!s10_handover_process){
    /** No S10 Handover Process. */
    OAILOG_ERROR(LOG_MME_APP, "No handover process for UE " MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION local S10 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      forward_access_context_notification_pP->teid, ue_context->imsi);
  /** Send a S1AP MME Status Transfer Message the target eNodeB. */
  message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE);
  DevAssert (message_p != NULL);
  itti_s10_forward_access_context_acknowledge_t *s10_mme_forward_access_context_acknowledge_p =
      &message_p->ittiMsg.s10_forward_access_context_acknowledge;
  s10_mme_forward_access_context_acknowledge_p->teid        = s10_handover_process->remote_mme_teid.teid;  /**< Set the target TEID. */
  s10_mme_forward_access_context_acknowledge_p->local_teid  = ue_context->local_mme_teid_s10;   /**< Set the local TEID. */
  s10_mme_forward_access_context_acknowledge_p->peer_ip     = s10_handover_process->remote_mme_teid.ipv4_address; /**< Set the target TEID. */
  s10_mme_forward_access_context_acknowledge_p->trxn        = forward_access_context_notification_pP->trxn; /**< Set the target TEID. */
  /** Check that there is a pending handover process. */
  DevAssert(ue_context->mm_state == UE_UNREGISTERED);
  /** Deal with the error case. */
  s10_mme_forward_access_context_acknowledge_p->cause.cause_value = REQUEST_ACCEPTED;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "MME_APP Sending S10 FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE.");
  itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);

  /** Send a S1AP MME Status Transfer Message the target eNodeB. */
  // todo: macro/home enb id
  mme_app_send_s1ap_mme_status_transfer(ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id, s10_handover_process->target_id.target_id.macro_enb_id.enb_id,
      forward_access_context_notification_pP->eutran_container.container_value);
  /** Unlink it from the message. */
  forward_access_context_notification_pP->eutran_container.container_value = NULL;

  /*
   * Setting the ECM state to ECM_CONNECTED with Handover Request Acknowledge.
   */
// todo: DevAssert(ue_context->ecm_state == ECM_CONNECTED); /**< Any timeouts here should erase the context, not change the signaling state back to IDLE but leave the context. */
  OAILOG_INFO(LOG_MME_APP, "Sending S1AP MME Status transfer to the target eNodeB %d for UE with enb_ue_s1ap_id: " ENB_UE_S1AP_ID_FMT", mme_ue_s1ap_id. "MME_UE_S1AP_ID_FMT ". \n",
      s10_handover_process->target_id.target_id.macro_enb_id.enb_id, s10_handover_process->target_enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_forward_access_context_acknowledge(
    const itti_s10_forward_access_context_acknowledge_t* const forward_access_context_acknowledge_pP
    )
{
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p  = NULL;
  uint64_t                                imsi = 0;
  int16_t                                 bearer_id =0;

  OAILOG_FUNC_IN (LOG_MME_APP);
  OAILOG_DEBUG (LOG_MME_APP, "Received S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE from S10. \n");
  DevAssert (forward_access_context_acknowledge_pP );

  /** Here it checks the local TEID. */
  ue_context = mme_ue_context_exists_s10_teid (&mme_app_desc.mme_ue_contexts, forward_access_context_acknowledge_pP->teid);

  if (ue_context == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE local S11 teid " TEID_FMT " ", forward_access_context_acknowledge_pP->teid);
    OAILOG_ERROR (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", forward_access_context_acknowledge_pP->teid);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      forward_access_context_acknowledge_pP->teid, ue_context->imsi);

  /** Check that there is a handover process. */
  mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
  if(!s10_handover_proc){
    /** Deal with the error case. */
    OAILOG_ERROR(LOG_MME_APP, "UE MME context with IMSI " IMSI_64_FMT " and mmeS1apUeId: " MME_UE_S1AP_ID_FMT " is not in UE_REGISTERED state, instead %d, when S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE is received. "
        "Ignoring the error and waiting remove the UE contexts triggered by HSS. \n",
        ue_context->imsi, ue_context->mme_ue_s1ap_id, ue_context->mm_state);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_handover_request_acknowledge(
     itti_s1ap_handover_request_acknowledge_t * const handover_request_acknowledge_pP
    )
{
 struct ue_context_s                    *ue_context = NULL;
 MessageDef                             *message_p  = NULL;

 OAILOG_FUNC_IN (LOG_MME_APP);
 OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_HANDOVER_REQUEST_ACKNOWLEDGE from S1AP for enbUeS1apId " ENB_UE_S1AP_ID_FMT". \n", handover_request_acknowledge_pP->enb_ue_s1ap_id);
 /** Just get the S1U-ENB-FTEID. */
 ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, handover_request_acknowledge_pP->mme_ue_s1ap_id);
 if (ue_context == NULL) {
   OAILOG_ERROR(LOG_MME_APP, "No MME_APP UE context for the UE with mmeUeS1APId : " MME_UE_S1AP_ID_FMT ". \n", handover_request_acknowledge_pP->mme_ue_s1ap_id);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_FAILURE. No UE existing mmeS1apUeId %d. \n", handover_request_acknowledge_pP->mme_ue_s1ap_id);
   /** Ignore the message. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_HANDOVER_REQUEST_ACKNOWLEDGE from S1AP (2). UE eNbUeS1aPId " ENB_UE_S1AP_ID_FMT ". \n", ue_context->enb_ue_s1ap_id);

 /*
  * Set the downlink bearers as pending.
  * Will be forwarded to the SAE-GW after the HANDOVER_NOTIFY/S10_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGE.
  * todo: currently only a single bearer will be set.
  * Check if there is a handover procedure running.
  * Depending on the handover procedure, either send handover command or forward_relocation_response.
  * todo: do this via success_notification.
  */
 mme_app_s10_proc_mme_handover_t *s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
 if(!s10_handover_proc){
   // todo: how could this happen?
   OAILOG_INFO(LOG_MME_APP, "No S10 handover procedure for mmeUeS1APId : " MME_UE_S1AP_ID_FMT " and enbUeS1apId " ENB_UE_S1AP_ID_FMT ". \n",
      ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id);
   /** Remove the created UE reference. */
   ue_context->s1_ue_context_release_cause = S1AP_SYSTEM_FAILURE;
   mme_app_itti_ue_context_release(handover_request_acknowledge_pP->mme_ue_s1ap_id, handover_request_acknowledge_pP->enb_ue_s1ap_id, ue_context->s1_ue_context_release_cause, ue_context->e_utran_cgi.cell_identity.enb_id);
   /** Ignore the message. Set the UE to idle mode. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }

 AssertFatal(NULL == s10_handover_proc->source_to_target_eutran_f_container.container_value, "TODO clean pointer");

 /*
  * For both cases, update the S1U eNB FTEIDs.
  */
 s10_handover_proc->target_enb_ue_s1ap_id = handover_request_acknowledge_pP->enb_ue_s1ap_id;

 for(int nb_bearer = 0; nb_bearer < handover_request_acknowledge_pP->no_of_e_rabs; nb_bearer++) {
   ebi_t ebi = handover_request_acknowledge_pP->e_rab_id[nb_bearer];
   /** Get the bearer context. */
   bearer_context_t * bearer_context = NULL;
   mme_app_get_session_bearer_context_from_all(ue_context, ebi, &bearer_context);
   DevAssert(bearer_context);
   /** Update the FTEID of the bearer context and uncheck the established state. */
   bearer_context->enb_fteid_s1u.teid = handover_request_acknowledge_pP->gtp_teid[nb_bearer];
   bearer_context->enb_fteid_s1u.interface_type      = S1_U_ENODEB_GTP_U;
   /** Set the IP address from the FTEID. */
   if (4 == blength(handover_request_acknowledge_pP->transport_layer_address[nb_bearer])) {
     bearer_context->enb_fteid_s1u.ipv4 = 1;
     memcpy(&bearer_context->enb_fteid_s1u.ipv4_address,
         handover_request_acknowledge_pP->transport_layer_address[nb_bearer]->data, blength(handover_request_acknowledge_pP->transport_layer_address[nb_bearer]));
   } else if (16 == blength(handover_request_acknowledge_pP->transport_layer_address[nb_bearer])) {
     bearer_context->enb_fteid_s1u.ipv6 = 1;
     memcpy(&bearer_context->enb_fteid_s1u.ipv6_address,
         handover_request_acknowledge_pP->transport_layer_address[nb_bearer]->data,
         blength(handover_request_acknowledge_pP->transport_layer_address[nb_bearer]));
   } else {
     AssertFatal(0, "TODO IP address %d bytes", blength(handover_request_acknowledge_pP->transport_layer_address[nb_bearer]));
   }
   bearer_context->bearer_state |= BEARER_STATE_ENB_CREATED;
   bearer_context->bearer_state |= BEARER_STATE_MME_CREATED;
 }
 // todo: handle bearer contexts failed.
 // todo: lionel--> checking the type or different success_notif methods?
 //   s10_handover_proc->success_notif(ue_context, handover_request_acknowledge_pP->target_to_source_eutran_container);
 if(s10_handover_proc->proc.type == MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER){
   /** Send Handover Command to the source enb without changing any parameters in the MME_APP UE context. */
   DevAssert(ue_context->mm_state == UE_REGISTERED);
   OAILOG_INFO(LOG_MME_APP, "Intra-MME S10 Handover procedure is ongoing. Sending a Handover Command to the source-ENB with enbId: %d for UE with mmeUeS1APId : " MME_UE_S1AP_ID_FMT " and enbUeS1apId " ENB_UE_S1AP_ID_FMT ". \n",
       ue_context->e_utran_cgi.cell_identity.enb_id, handover_request_acknowledge_pP->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id);
   /*
    * Save the new ENB_UE_S1AP_ID
    * Don't update the coll_keys with the new enb_ue_s1ap_id.
    */
   mme_app_send_s1ap_handover_command(handover_request_acknowledge_pP->mme_ue_s1ap_id,
       ue_context->enb_ue_s1ap_id,
       ue_context->e_utran_cgi.cell_identity.enb_id,
       handover_request_acknowledge_pP->target_to_source_eutran_container);
   /** Unlink the transparent container. */
   handover_request_acknowledge_pP->target_to_source_eutran_container = NULL;
   /**
    * As the specification said, we will leave the UE_CONTEXT as it is. Not checking further parameters.
    * No timers are started. Only timers are in the source-ENB.
    * ********************************************
    * The ECM state will be set to ECM-CONNECTED (the one from the created ue_reference).
    * Not waiting for HANDOVER_NOTIFY to set to ECM_CONNECTED. todo: eventually check this with specifications.
    */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }else{
   DevAssert(ue_context->mm_state == UE_UNREGISTERED); /**< NAS invalidation should have set this to UE_UNREGISTERED. */
   OAILOG_INFO(LOG_MME_APP, "Inter-MME S10 Handover procedure is ongoing. Sending a Forward Relocation Response message to source-MME for ueId: " MME_UE_S1AP_ID_FMT " and enbUeS1apId " ENB_UE_S1AP_ID_FMT ". \n",
       handover_request_acknowledge_pP->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id);
   /*
    * Set the enb_ue_s1ap_id in this case to the UE_Context, too.
    * todo: just let it in the s10_handover_procedure.
    */
   ue_context->enb_ue_s1ap_id = handover_request_acknowledge_pP->enb_ue_s1ap_id;
   /*
    * Update the enb_id_s1ap_key and register it.
    */
   enb_s1ap_id_key_t                       enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;
   MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, s10_handover_proc->target_enb_ue_s1ap_id, handover_request_acknowledge_pP->enb_ue_s1ap_id);
   /*
    * Update the coll_keys with the new s1ap parameters.
    */
   mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
       enb_s1ap_id_key,     /**< New key. */
       ue_context->mme_ue_s1ap_id,
       ue_context->imsi,
       ue_context->mme_teid_s11,
       ue_context->local_mme_teid_s10,
       &ue_context->guti);

   teid_t local_teid = 0x0;
   do{
     local_teid = (teid_t)(rand() % 0xFFFFFFFF);
   }while(mme_ue_context_exists_s10_teid(&mme_app_desc.mme_ue_contexts, local_teid) != NULL);

   /**
    * Update the local_s10_key.
    * Leave the enb_ue_s1ap_id_key as it is. enb_ue_s1ap_id_key will be updated with HANDOVER_NOTIFY and then the registration will be updated.
    * Not setting the key directly in the  ue_context structure. Only over this function!
    */
   mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
       ue_context->enb_s1ap_id_key,   /**< Not updated. */
       ue_context->mme_ue_s1ap_id,
       ue_context->imsi,
       ue_context->mme_teid_s11,       // mme_teid_s11 is new
       local_teid,       // set with forward_relocation_response!
       &ue_context->guti);            /**< No guti exists
   /*
    * UE is in UE_UNREGISTERED state. Assuming inter-MME S1AP Handover was triggered.
    * Sending FW_RELOCATION_RESPONSE.
    */
   mme_app_itti_forward_relocation_response(ue_context, s10_handover_proc, handover_request_acknowledge_pP->target_to_source_eutran_container);
   /** Unlink the transparent container. */
   handover_request_acknowledge_pP->target_to_source_eutran_container = NULL;
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
}

//------------------------------------------------------------------------------
void
mme_app_handle_handover_failure (
     const itti_s1ap_handover_failure_t * const handover_failure_pP
    )
{
 struct ue_context_s                    *ue_context = NULL;
 MessageDef                             *message_p = NULL;
 uint64_t                                imsi = 0;
 mme_app_s10_proc_mme_handover_t        *s10_handover_proc = NULL;

 OAILOG_FUNC_IN (LOG_MME_APP);

 OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_HANDOVER_FAILURE from target eNB for UE_ID " MME_UE_S1AP_ID_FMT ". \n", handover_failure_pP->mme_ue_s1ap_id);

 /** Check that the UE does exist (in both S1AP cases). */
 ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, handover_failure_pP->mme_ue_s1ap_id);
 if (ue_context == NULL) {
   OAILOG_ERROR(LOG_MME_APP, "An UE MME context does not exist for UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT". Ignoring received handover_failure. \n", handover_failure_pP->mme_ue_s1ap_id);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_FAILURE. No UE existing mmeS1apUeId" MME_UE_S1AP_ID_FMT". Ignoring received handover_failure. \n", handover_failure_pP->mme_ue_s1ap_id);
   /** Ignore the message. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** Check if an handover procedure exists, if not drop the message. */
 s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
 if(!s10_handover_proc){
   OAILOG_ERROR(LOG_MME_APP, "No S10 Handover procedure exists for UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT". Ignoring received handover_failure. \n", handover_failure_pP->mme_ue_s1ap_id);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_FAILURE. No S10 Handover procedure exists for UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT". Ignoring received handover_failure. \n", handover_failure_pP->mme_ue_s1ap_id);
   /** Ignore the message. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** Check if the UE is EMM-REGISTERED or not. */
 if(ue_context->mm_state == UE_REGISTERED){
   /**
    * UE is registered, we assume in this case that the source-MME is also attached to the current.
    * In this case, we need to re-notify the MME_UE_S1AP_ID<->SCTP association, because it might be removed with the error handling.
    */
   notify_s1ap_new_ue_mme_s1ap_id_association (ue_context->sctp_assoc_id_key, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id);
   /** We assume a valid enb & sctp id in the UE_Context. */
   mme_app_send_s1ap_handover_preparation_failure(ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id, ue_context->sctp_assoc_id_key, S1AP_HANDOVER_FAILED);
   /**
    * As the specification said, we will leave the UE_CONTEXT as it is. Not checking further parameters.
    * No timers are started. Only timers are in the source-ENB.
    */
   /*
    * Delete the s10 handover procedure.
    */
   mme_app_delete_s10_procedure_mme_handover(ue_context);
   /** Leave the context as it is. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /**
  * UE is in UE_UNREGISTERED state. Assuming inter-MME S1AP Handover was triggered.
  * Sending FW_RELOCATION_RESPONSE with error code and implicit detach.
  */

 // Initiate Implicit Detach for the UE
 message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_RELOCATION_RESPONSE);
 DevAssert (message_p != NULL);
 itti_s10_forward_relocation_response_t *forward_relocation_response_p = &message_p->ittiMsg.s10_forward_relocation_response;
 memset ((void*)forward_relocation_response_p, 0, sizeof (itti_s10_forward_relocation_response_t));
 /** Set the target S10 TEID. */
 forward_relocation_response_p->teid    = s10_handover_proc->remote_mme_teid.teid; /**< Only a single target-MME TEID can exist at a time. */
 /** Get the MME from the origin TAI. */
 forward_relocation_response_p->peer_ip = s10_handover_proc->remote_mme_teid.ipv4_address; /**< todo: Check this is correct. */
 /**
  * Trxn is the only object that has the last seqNum, but we can only search the TRXN in the RB-Tree with the seqNum.
  * We need to store the last seqNum locally.
  */
 forward_relocation_response_p->trxn    = s10_handover_proc->forward_relocation_trxn;
 /** Set the cause. */
 forward_relocation_response_p->cause.cause_value = RELOCATION_FAILURE;

 /** Perform an implicit detach. Will remove all S10 procedures. */
 ue_context->s1_ue_context_release_cause = S1AP_IMPLICIT_CONTEXT_RELEASE;
 message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
 DevAssert (message_p != NULL);
 itti_nas_implicit_detach_ue_ind_t *nas_implicit_detach_ue_ind_p = &message_p->ittiMsg.nas_implicit_detach_ue_ind;
 memset ((void*)nas_implicit_detach_ue_ind_p, 0, sizeof (itti_nas_implicit_detach_ue_ind_t));
 message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->mme_ue_s1ap_id;
 itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);

 /** No timers, etc. is needed. */
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_enb_status_transfer(
     itti_s1ap_status_transfer_t * const s1ap_status_transfer_pP
    )
{
 struct ue_context_s                    *ue_context = NULL;
 MessageDef                             *message_p  = NULL;

 OAILOG_FUNC_IN (LOG_MME_APP);
 OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_ENB_STATUS_TRANSFER from S1AP. \n");

 /** Check that the UE does exist. */
 ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, s1ap_status_transfer_pP->mme_ue_s1ap_id);
 if (ue_context == NULL) {
   OAILOG_ERROR(LOG_MME_APP, "An UE MME context does not exist for UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT" Ignoring ENB_STATUS_TRANSFER. \n", s1ap_status_transfer_pP->mme_ue_s1ap_id);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_ENB_STATUS_TRANSFER. No UE existing mmeS1apUeId " MME_UE_S1AP_ID_FMT" Ignoring ENB_STATUS_TRANSFER. \n", s1ap_status_transfer_pP->mme_ue_s1ap_id);
   /**
    * We don't really expect an error at this point. Just ignore the message.
    */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
 if(!s10_handover_proc){
   OAILOG_ERROR(LOG_MME_APP, "No S10 handover procedure exists for UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT" Ignoring ENB_STATUS_TRANSFER. \n", s1ap_status_transfer_pP->mme_ue_s1ap_id);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_ENB_STATUS_TRANSFER. No UE existing mmeS1apUeId " MME_UE_S1AP_ID_FMT" Ignoring ENB_STATUS_TRANSFER. \n", s1ap_status_transfer_pP->mme_ue_s1ap_id);
   /**
    * We don't really expect an error at this point. Just ignore the message.
    */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 if(s10_handover_proc->proc.type == MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER){
   /**
    * Set the ENB of the pending target-eNB.
    * Even if the HANDOVER_NOTIFY messaged is received simultaneously, the pending enb_ue_s1ap_id field should stay.
    * We do not check that the target-eNB exists. We did not modify any contexts.
    * Concatenate with header (todo: OAI: how to do this properly?)
    */
   char enbStatusPrefix[] = {0x00, 0x00, 0x00, 0x59, 0x40, 0x0b};
   bstring enbStatusPrefixBstr = blk2bstr (enbStatusPrefix, 6);
   bconcat(enbStatusPrefixBstr, s1ap_status_transfer_pP->bearerStatusTransferList_buffer);
   /** No need to unlink here. */
   // todo: macro/home
   mme_app_send_s1ap_mme_status_transfer(ue_context->mme_ue_s1ap_id, s10_handover_proc->target_enb_ue_s1ap_id, s10_handover_proc->target_ecgi.cell_identity.enb_id, enbStatusPrefixBstr);
   /** eNB-Status-Transfer message message will be freed. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }else{
   /* UE is DEREGISTERED. Assuming that it came from S10 inter-MME handover. Forwarding the eNB status information to the target-MME via Forward Access Context Notification. */
   message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION);
   DevAssert (message_p != NULL);
   itti_s10_forward_access_context_notification_t *forward_access_context_notification_p = &message_p->ittiMsg.s10_forward_access_context_notification;
   memset ((void*)forward_access_context_notification_p, 0, sizeof (itti_s10_forward_access_context_notification_t));
   /** Set the target S10 TEID. */
   forward_access_context_notification_p->teid           = s10_handover_proc->remote_mme_teid.teid; /**< Only a single target-MME TEID can exist at a time. */
   forward_access_context_notification_p->local_teid     = ue_context->local_mme_teid_s10; /**< Only a single target-MME TEID can exist at a time. */
   forward_access_context_notification_p->peer_ip.s_addr = s10_handover_proc->remote_mme_teid.ipv4_address.s_addr;
   /** Set the E-UTRAN container. */
   forward_access_context_notification_p->eutran_container.container_type = 3;
   forward_access_context_notification_p->eutran_container.container_value = s1ap_status_transfer_pP->bearerStatusTransferList_buffer;
   /** Unlink. */
   s1ap_status_transfer_pP->bearerStatusTransferList_buffer = NULL;
   if (forward_access_context_notification_p->eutran_container.container_value == NULL){
     OAILOG_ERROR (LOG_MME_APP, " NULL UE transparent container\n" );
     OAILOG_FUNC_OUT (LOG_MME_APP);
   }
   MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "MME_APP Sending S10 FORWARD_ACCESS_CONTEXT_NOTIFICATION to TARGET-MME with TEID " TEID_FMT,
       forward_access_context_notification_p->teid);
   /**
    * Sending a message to S10.
    * Although eNB-Status message is not mandatory, if it is received, it should be forwarded.
    * That's why, we start a timer for the Forward Access Context Acknowledge.
    */
   itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
}

//------------------------------------------------------------------------------
void
mme_app_handle_s1ap_handover_notify(
     const itti_s1ap_handover_notify_t * const handover_notify_pP
    )
{
 struct ue_context_s                    *ue_context = NULL;
 MessageDef                             *message_p  = NULL;
 enb_s1ap_id_key_t                       enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;

 OAILOG_FUNC_IN (LOG_MME_APP);
 OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_HANDOVER_NOTIFY from S1AP SCTP ASSOC_ID %d. \n", handover_notify_pP->assoc_id);

 /** Check that the UE does exist. */
 ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, handover_notify_pP->mme_ue_s1ap_id);
 if (ue_context == NULL) {
   OAILOG_ERROR(LOG_MME_APP, "An UE MME context does not exist for UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT". \n", handover_notify_pP->mme_ue_s1ap_id);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_NOTIFY. No UE existing mmeS1apUeId " MME_UE_S1AP_ID_FMT". \n", handover_notify_pP->mme_ue_s1ap_id);
   /** Removing the S1ap context directly. */
   mme_app_itti_ue_context_release(handover_notify_pP->mme_ue_s1ap_id, handover_notify_pP->enb_ue_s1ap_id, SYSTEM_FAILURE, handover_notify_pP->cgi.cell_identity.enb_id);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /**
  * Check that there is an s10 handover process running, if not discard the message.
  */
 mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
 if(!s10_handover_proc){
   OAILOG_ERROR(LOG_MME_APP, "No Handover Procedure is runnning for UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT" in state %d. Discarding the message\n", ue_context->mme_ue_s1ap_id, ue_context->mm_state);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }

 /**
  * No need to signal the NAS layer the completion of the handover.
  * The ECGI & TAI will also be sent with TAU if its UPLINK_NAS_TRANSPORT.
  * Here we just update the MME_APP UE_CONTEXT parameters.
  */

 /**
  * When Handover Notify is received, we update the eNB associations (SCTP, enb_ue_s1ap_id, enb_id,..). The main eNB is the new ENB now.
  * ToDo: If this has an error with 2 eNBs, we need to remove the first eNB first (and send the UE Context Release Command only with MME_UE_S1AP_ID).
  * Update the coll-keys.
  */
 ue_context->sctp_assoc_id_key = handover_notify_pP->assoc_id;
 ue_context->e_utran_cgi = handover_notify_pP->cgi;
 /** Update the enbUeS1apId (again). */
 ue_context->enb_ue_s1ap_id = handover_notify_pP->enb_ue_s1ap_id; /**< Updating the enb_ue_s1ap_id here. */
 // regenerate the enb_s1ap_id_key as enb_ue_s1ap_id is changed.
 MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, handover_notify_pP->cgi.cell_identity.enb_id, handover_notify_pP->enb_ue_s1ap_id);

 /**
  * Update the coll_keys with the new s1ap parameters.
  */
 mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
     enb_s1ap_id_key,     /**< New key. */
     ue_context->mme_ue_s1ap_id,
     ue_context->imsi,
     ue_context->mme_teid_s11,
     ue_context->local_mme_teid_s10,
     &ue_context->guti);

 OAILOG_INFO(LOG_MME_APP, "Setting UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT " to enbUeS1apId " ENB_UE_S1AP_ID_FMT" @handover_notify. \n", ue_context->mme_ue_s1ap_id, ue_context->enb_ue_s1ap_id);

 /**
  * This will overwrite the association towards the old eNB if single MME S1AP handover.
  * The old eNB will be referenced by the enb_ue_s1ap_id.
  */
 notify_s1ap_new_ue_mme_s1ap_id_association (ue_context->sctp_assoc_id_key, ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id);

 /**
  * Check the UE status:
  *
  * If we are in UE_REGISTERED state (intra-MME HO), start the timer for UE resource deallocation at the source eNB.
  * todo: if TAU is performed, same timer should be checked and not restarted if its already running.
  * The registration to the new MME is performed with the TAU (UE is in UE_UNREGISTERED/EMM_DEREGISTERED states).
  * If the timer runs up and no S6A_CLReq is received from the MME, we assume an intra-MME handover and just remove the resources in the source eNB, else we perform an implicit detach (we don't check the MME UE status).
  * We need to store now the enb_ue_s1ap_id and the enb_id towards the source enb as pending.
  * No timers to stop in this step.
  *
  * Update the bearers in the SAE-GW for INTRA and INTER MME handover.
  */

 if(s10_handover_proc->proc.type == MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER){
   OAILOG_DEBUG(LOG_MME_APP, "UE MME context with imsi " IMSI_64_FMT " and mmeS1apUeId " MME_UE_S1AP_ID_FMT " has successfully completed intra-MME handover process after HANDOVER_NOTIFY. \n",
       ue_context->imsi, handover_notify_pP->mme_ue_s1ap_id);
   /** Update the PDN session information directly in the new UE_Context. */
   pdn_context_t * pdn_context = NULL;
   RB_FOREACH (pdn_context, PdnContexts, &ue_context->pdn_contexts) {
     DevAssert(pdn_context);
     bearer_context_t * first_bearer = RB_MIN(SessionBearers, &pdn_context->session_bearers);
     DevAssert(first_bearer);
     if(first_bearer->bearer_state == BEARER_STATE_ACTIVE){
       /** Continue to next pdn. */
       continue;
     }else{
       /** Found a PDN. Establish the bearer contexts. */
       OAILOG_INFO(LOG_MME_APP, "Establishing the bearers for UE_CONTEXT for UE " MME_UE_S1AP_ID_FMT " triggered by handover notify. Successfully handled handover notify. \n", ue_context->mme_ue_s1ap_id);
       mme_app_send_s11_modify_bearer_req(ue_context, pdn_context);
       OAILOG_FUNC_OUT (LOG_MME_APP);
     }
   }
   OAILOG_ERROR(LOG_MME_APP, "No PDN found to establish the bearers for UE " MME_UE_S1AP_ID_FMT " due handover notify. \n", ue_context->mme_ue_s1ap_id);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }else{
   OAILOG_DEBUG(LOG_MME_APP, "UE MME context with imsi " IMSI_64_FMT " and mmeS1apUeId " MME_UE_S1AP_ID_FMT " has successfully completed inter-MME handover process after HANDOVER_NOTIFY. \n",
       ue_context->imsi, handover_notify_pP->mme_ue_s1ap_id);
   /**
    * UE came from S10 inter-MME handover. Not clear the pending_handover state yet.
    * Sending Forward Relocation Complete Notification and waiting for acknowledgment.
    */
   message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_RELOCATION_COMPLETE_NOTIFICATION);
   DevAssert (message_p != NULL);
   itti_s10_forward_relocation_complete_notification_t *forward_relocation_complete_notification_p = &message_p->ittiMsg.s10_forward_relocation_complete_notification;
   /** Set the destination TEID. */
   forward_relocation_complete_notification_p->teid = s10_handover_proc->remote_mme_teid.teid;       /**< Target S10-MME TEID. todo: what if multiple? */
   /** Set the local TEID. */
   forward_relocation_complete_notification_p->local_teid = ue_context->local_mme_teid_s10;        /**< Local S10-MME TEID. */
   forward_relocation_complete_notification_p->peer_ip = s10_handover_proc->remote_mme_teid.ipv4_address; /**< Set the target TEID. */
   OAILOG_INFO(LOG_MME_APP, "Sending FW_RELOC_COMPLETE_NOTIF TO %X with remote S10-TEID " TEID_FMT ". \n.",
       forward_relocation_complete_notification_p->peer_ip, forward_relocation_complete_notification_p->teid);

   // todo: remove this and set at correct position!
   mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_CONNECTED);

   /**
    * Sending a message to S10. Not changing any context information!
    * This message actually implies that the handover is finished. Resetting the flags and statuses here of after Forward Relocation Complete AcknowledgE?! (MBR)
    */
   itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
 }
}

//------------------------------------------------------------------------------
void
mme_app_handle_forward_relocation_complete_notification(
     const itti_s10_forward_relocation_complete_notification_t* const forward_relocation_complete_notification_pP
    )
{
 struct ue_context_s                    *ue_context = NULL;
 MessageDef                             *message_p  = NULL;

 OAILOG_FUNC_IN (LOG_MME_APP);
 OAILOG_DEBUG (LOG_MME_APP, "Received S10_FORWARD_RELOCATION_COMPLETE_NOTIFICATION from S10. \n");

 /** Check that the UE does exist. */
 ue_context = mme_ue_context_exists_s10_teid (&mme_app_desc.mme_ue_contexts, forward_relocation_complete_notification_pP->teid); /**< Get the UE context from the local TEID. */
 if (ue_context == NULL) {
   MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 FORWARD_RELOCATION_COMPLETE_NOTIFICATION local S10 teid " TEID_FMT,
       forward_relocation_complete_notification_pP->teid);
   OAILOG_ERROR (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", forward_relocation_complete_notification_pP->teid);
   OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
 }
 MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 FORWARD_RELOCATION_COMPLETE_NOTIFICATION local S10 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
     forward_relocation_complete_notification_pP->teid, ue_context->imsi);

 /** Check that there is a pending handover process. */
 mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
 if(!s10_handover_proc || s10_handover_proc->proc.type == MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER){
   /** Deal with the error case. */
   OAILOG_ERROR(LOG_MME_APP, "UE MME context with IMSI " IMSI_64_FMT " and mmeS1apUeId: " MME_UE_S1AP_ID_FMT " in state %d has not a valid INTER-MME S10 procedure running. "
       "Ignoring received S10_FORWARD_RELOCATION_COMPLETE_NOTIFICATION. \n", ue_context->imsi, ue_context->mme_ue_s1ap_id, ue_context->mm_state);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** Send S10 Forward Relocation Complete Notification. */
 /**< Will stop all existing NAS timers.. todo: Any other timers than NAS timers? What about the UE transactions? */
 message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGE);
 DevAssert (message_p != NULL);
 itti_s10_forward_relocation_complete_acknowledge_t *forward_relocation_complete_acknowledge_p = &message_p->ittiMsg.s10_forward_relocation_complete_acknowledge;
 memset ((void*)forward_relocation_complete_acknowledge_p, 0, sizeof (itti_s10_forward_relocation_complete_acknowledge_t));
 /** Set the destination TEID. */
 forward_relocation_complete_acknowledge_p->teid       = s10_handover_proc->remote_mme_teid.teid;      /**< Target S10-MME TEID. */
 /** Set the local TEID. */
 forward_relocation_complete_acknowledge_p->local_teid = ue_context->local_mme_teid_s10;      /**< Local S10-MME TEID. */
 /** Set the cause. */
 forward_relocation_complete_acknowledge_p->cause.cause_value      = REQUEST_ACCEPTED;                       /**< Check the cause.. */
 /** Set the peer IP. */
 forward_relocation_complete_acknowledge_p->peer_ip.s_addr = s10_handover_proc->remote_mme_teid.ipv4_address.s_addr; /**< Set the target TEID. */
 /** Set the transaction. */
 forward_relocation_complete_acknowledge_p->trxn = forward_relocation_complete_notification_pP->trxn; /**< Set the target TEID. */
 itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
 /** ECM is in connected state.. UE will be detached implicitly. */
 ue_context->s1_ue_context_release_cause = S1AP_SUCCESSFUL_HANDOVER; /**< How mapped to correct radio-Network cause ?! */

 /**
  * Start timer to wait the handover/TAU procedure to complete.
  * A Clear_Location_Request message received from the HSS will cause the resources to be removed.
  * If it was not a handover but a context request/response (TAU), the MME_MOBILITY_COMPLETION timer will be started here, else @ FW-RELOC-COMPLETE @ Handover.
  * Resources will not be removed if that is not received (todo: may it not be received or must it always come
  * --> TS.23.401 defines for SGSN "remove after CLReq" explicitly).
  */
 ue_description_t * old_ue_reference = s1ap_is_enb_ue_s1ap_id_in_list_per_enb(ue_context->enb_ue_s1ap_id, ue_context->e_utran_cgi.cell_identity.enb_id);
 if(old_ue_reference){
   /** Start the timer of the procedure (only if target-mme for inter-MME S1ap handover). */
   if (timer_setup (mme_config.mme_mobility_completion_timer, 0,
       TASK_S1AP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *)old_ue_reference, &(old_ue_reference->s1ap_handover_completion_timer.id)) < 0) {
     OAILOG_ERROR (LOG_MME_APP, "Failed to start >s1ap_handover_completion for enbUeS1apId " ENB_UE_S1AP_ID_FMT " for duration %d \n", old_ue_reference->enb_ue_s1ap_id, mme_config.mme_mobility_completion_timer);
     old_ue_reference->s1ap_handover_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
     s10_handover_proc->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;
   } else {
     OAILOG_DEBUG (LOG_MME_APP, "MME APP : Completed Handover Procedure at (source) MME side after handling S1AP_HANDOVER_NOTIFY. "
         "Activated the S1AP Handover completion timer enbUeS1apId " ENB_UE_S1AP_ID_FMT ". Removing source eNB resources after timer.. Timer Id %u. Timer duration %d \n",
         old_ue_reference->enb_ue_s1ap_id, old_ue_reference->s1ap_handover_completion_timer.id, mme_config.mme_mobility_completion_timer);
     /** For the case of the S10 handover, add the timer ID to the MME_APP UE context to remove the UE context. */
     s10_handover_proc->proc.timer.id = old_ue_reference->s1ap_handover_completion_timer.id;
   }
 }else{
   OAILOG_DEBUG(LOG_MME_APP, "No old UE_REFERENCE was found for mmeS1apUeId " MME_UE_S1AP_ID_FMT " and enbUeS1apId "ENB_UE_S1AP_ID_FMT ". Not starting a new timer. \n",
       ue_context->enb_ue_s1ap_id, ue_context->e_utran_cgi.cell_identity.enb_id);
 }
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_forward_relocation_complete_acknowledge(
     const itti_s10_forward_relocation_complete_acknowledge_t * const forward_relocation_complete_acknowledgement_pP
    )
{
 struct ue_context_s                    *ue_context = NULL;
 MessageDef                             *message_p = NULL;

 OAILOG_FUNC_IN (LOG_MME_APP);
 OAILOG_DEBUG (LOG_MME_APP, "Received S10_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGEMENT from S10 for TEID  %d. \n", forward_relocation_complete_acknowledgement_pP->teid);

 /** Check that the UE does exist. */
 ue_context = mme_ue_context_exists_s10_teid(&mme_app_desc.mme_ue_contexts, forward_relocation_complete_acknowledgement_pP->teid);
 if (ue_context == NULL) {
   OAILOG_ERROR(LOG_MME_APP, "An UE MME context does not exist for UE with s10 teid %d. \n", forward_relocation_complete_acknowledgement_pP->teid);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S10_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGEMENT. No UE existing teid %d. \n", forward_relocation_complete_acknowledgement_pP->teid);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }

 /*
  * Not stopping MME Handover Completion timer. It will be stopped with the removed handover procedure on the target MME side for inter-mme s1ap handover.
  * (When TAU-Complete is received // UE is registered).
  */
 pdn_context_t * pdn_context = NULL;
 RB_FOREACH (pdn_context, PdnContexts, &ue_context->pdn_contexts) {
   DevAssert(pdn_context);
   bearer_context_t * first_bearer = RB_MIN(SessionBearers, &pdn_context->session_bearers);
   DevAssert(first_bearer);
   if(first_bearer->bearer_state == BEARER_STATE_ACTIVE){
     /** Continue to next pdn. */
     continue;
   }else{
     /** Found a PDN. Establish the bearer contexts. */
     OAILOG_INFO(LOG_MME_APP, "Establishing the bearers for UE_CONTEXT for UE " MME_UE_S1AP_ID_FMT " triggered by handover notify. \n", ue_context->mme_ue_s1ap_id);
     mme_app_send_s11_modify_bearer_req(ue_context, pdn_context);
     OAILOG_FUNC_OUT (LOG_MME_APP);
   }
 }

 /** S1AP inter-MME handover is complete. */
 OAILOG_INFO(LOG_MME_APP, "UE_Context with IMSI " IMSI_64_FMT " and mmeUeS1apId: " MME_UE_S1AP_ID_FMT " successfully completed handover procedure! \n",
     ue_context->imsi, ue_context->mme_ue_s1ap_id);
 OAILOG_FUNC_OUT (LOG_MME_APP);
}


//------------------------------------------------------------------------------
void
mme_app_handle_mobile_reachability_timer_expiry (struct ue_context_s *ue_context)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context != NULL);
  ue_context->mobile_reachability_timer.id = MME_APP_TIMER_INACTIVE_ID;
  OAILOG_INFO (LOG_MME_APP, "Expired- Mobile Reachability Timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
  // Start Implicit Detach timer
  if (timer_setup (ue_context->implicit_detach_timer.sec, 0,
                TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *)&(ue_context->mme_ue_s1ap_id), &(ue_context->implicit_detach_timer.id)) < 0) {
    OAILOG_ERROR (LOG_MME_APP, "Failed to start Implicit Detach timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
    ue_context->implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;
  } else {
    OAILOG_DEBUG (LOG_MME_APP, "Started Implicit Detach timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_implicit_detach_timer_expiry (struct ue_context_s *ue_context)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context != NULL);
  MessageDef                             *message_p = NULL;
  OAILOG_INFO (LOG_MME_APP, "Expired- Implicit Detach timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
  ue_context->implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;

  // Initiate Implicit Detach for the UE
  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
  DevAssert (message_p != NULL);
  message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->mme_ue_s1ap_id;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
  itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

// todo: handle this inside normal mme_app_procs timeout handler
//------------------------------------------------------------------------------
void
mme_app_handle_mobility_completion_timer_expiry (struct ue_context_s *ue_context)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context != NULL);
  MessageDef                             *message_p = NULL;
  OAILOG_INFO (LOG_MME_APP, "Expired- MME Mobility Completion timer for UE " MME_UE_S1AP_ID_FMT " run out. \n", ue_context->mme_ue_s1ap_id);
  /** Get the S10 Handover Procedure. */
  mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
  if(!s10_handover_proc){
    OAILOG_ERROR(LOG_MME_APP, "No S10 Handover procedure exists for UE " MME_UE_S1AP_ID_FMT ". Ignoring Mobility Completion Timer expiry. \n", ue_context->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  if(s10_handover_proc->pending_clear_location_request){
//    OAILOG_INFO (LOG_MME_APP, "CLR flag is set for UE " MME_UE_S1AP_ID_FMT ". Performing implicit detach. \n", ue_context->mme_ue_s1ap_id);
//    ue_context->mme_mobility_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
//    ue_context->s1_ue_context_release_cause = S1AP_NAS_DETACH;
//    /** Check if the CLR flag has been set. */
//    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
//    DevAssert (message_p != NULL);
//    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->mme_ue_s1ap_id; /**< We don't send a Detach Type such that no Detach Request is sent to the UE if handover is performed. */
//    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
//    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  }else{
    OAILOG_WARNING(LOG_MME_APP, "CLR flag is NOT set for UE " MME_UE_S1AP_ID_FMT ". Not performing implicit detach. \n", ue_context->mme_ue_s1ap_id);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_initial_context_setup_rsp_timer_expiry (struct ue_context_s *ue_context)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context != NULL);
  MessageDef                             *message_p = NULL;
  OAILOG_INFO (LOG_MME_APP, "Expired- Initial context setup rsp timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
  ue_context->initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  /* *********Abort the ongoing procedure*********
   * Check if UE is registered already that implies service request procedure is active. If so then release the S1AP
   * context and move the UE back to idle mode. Otherwise if UE is not yet registered that implies attach procedure is
   * active. If so,then abort the attach procedure and release the UE context.
   */
  ue_context->s1_ue_context_release_cause = S1AP_INITIAL_CONTEXT_SETUP_FAILED;
  if (ue_context->mm_state == UE_UNREGISTERED) {
    // Initiate Implicit Detach for the UE
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->mme_ue_s1ap_id;
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  } else {
    // Release S1-U bearer and move the UE to idle mode
    mme_app_send_s11_release_access_bearers_req(ue_context);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_initial_context_setup_failure (
  const itti_mme_app_initial_context_setup_failure_t * const initial_ctxt_setup_failure_pP)
{
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p  = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  OAILOG_DEBUG (LOG_MME_APP, "Received MME_APP_INITIAL_CONTEXT_SETUP_FAILURE from S1AP\n");
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, initial_ctxt_setup_failure_pP->mme_ue_s1ap_id);

  if (ue_context == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: %d \n", initial_ctxt_setup_failure_pP->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  // Stop Initial context setup process guard timer,if running
  if (ue_context->initial_context_setup_rsp_timer.id != MME_APP_TIMER_INACTIVE_ID) {
    if (timer_remove(ue_context->initial_context_setup_rsp_timer.id, NULL)) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to stop Initial Context Setup Rsp timer for UE id  %d \n", ue_context->mme_ue_s1ap_id);
    }
    ue_context->initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  }
  /* *********Abort the ongoing procedure*********
   * Check if UE is registered already that implies service request procedure is active. If so then release the S1AP
   * context and move the UE back to idle mode. Otherwise if UE is not yet registered that implies attach procedure is
   * active. If so,then abort the attach procedure and release the UE context.
   */
  ue_context->s1_ue_context_release_cause = S1AP_INITIAL_CONTEXT_SETUP_FAILED;
  if (ue_context->mm_state == UE_UNREGISTERED) {
    // Initiate Implicit Detach for the UE
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->mme_ue_s1ap_id;
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  } else {
    // Release S1-U bearer and move the UE to idle mode
    mme_app_send_s11_release_access_bearers_req(ue_context);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

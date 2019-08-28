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
  \author Sebastien ROUX, Lionel Gauthier, Dincer Beken
  \company Eurecom, Blackned GmbH
  \email: lionel.gauthier@eurecom.fr
  \email: dbeken@blackned.de
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

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
#include "mme_app_esm_procedures.h"
#include "s1ap_mme.h"
#include "s1ap_mme_ta.h"

static teid_t                           mme_app_teid_generator = 0x00000001;

//----------------------------------------------------------------------------
static void mme_app_send_s1ap_path_switch_request_acknowledge(mme_ue_s1ap_id_t mme_ue_s1ap_id,
    uint16_t encryption_algorithm_capabilities, uint16_t integrity_algorithm_capabilities,
    bearer_contexts_to_be_created_t * bcs_tbc);

/**
 * E-RAB handling.
 */
static void mme_app_handle_e_rab_setup_rsp_dedicated_bearer(const itti_s1ap_e_rab_setup_rsp_t * e_rab_setup_rsp);
static void mme_app_handle_e_rab_setup_rsp_pdn_connectivity(const mme_ue_s1ap_id_t mme_ue_s1ap_id, const e_rab_setup_item_t * e_rab_setup_item, const ebi_t failed_ebi);

static
void mme_app_send_s1ap_handover_request(mme_ue_s1ap_id_t mme_ue_s1ap_id,
    bearer_contexts_to_be_created_t *bcs_tbc,
    ambr_t                  *total_used_apn_ambr,
    uint32_t                enb_id,
    uint16_t                encryption_algorithm_capabilities,
    uint16_t                integrity_algorithm_capabilities,
    uint8_t                 nh[AUTH_NH_SIZE],
    uint8_t                 ncc,
    bstring                 eutran_source_to_target_container);

/** External definitions in MME_APP UE Data Context. */
extern void mme_app_set_ue_eps_mm_context(mm_context_eps_t * ue_eps_mme_context_p,  const struct ue_context_s * const ue_context, const struct ue_session_pool_s * const ue_session_pool, emm_data_context_t *ue_nas_ctx);
extern void mme_app_set_pdn_connections(struct mme_ue_eps_pdn_connections_s * pdn_connections, struct ue_session_pool_s * ue_session_pool);
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
int
mme_app_handle_nas_pdn_disconnect_req (
  itti_nas_pdn_disconnect_req_t * const nas_pdn_disconnect_req_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    *ue_context  = NULL;
  int                                     rc = RETURNok;

  DevAssert (nas_pdn_disconnect_req_pP );

  if ((ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, nas_pdn_disconnect_req_pP->ue_id)) == NULL) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " NAS_PDN_DISCONNECT_REQ Unknown ueId" MME_UE_S1AP_ID_FMT, nas_pdn_disconnect_req_pP->ue_id);
    OAILOG_WARNING (LOG_MME_APP, "That's embarrassing as we don't know this UeId " MME_UE_S1AP_ID_FMT". Still sending a DSReq (PDN context infomation exists in the MME) and informing NAS positively. \n", nas_pdn_disconnect_req_pP->ue_id);
    mme_ue_context_dump_coll_keys();
    /** Send the DSR anyway but don't expect a response. Directly continue. */

    /*
     * Updating statistics
     */
    mme_app_desc.mme_ue_session_pools.nb_bearers_managed--;
    mme_app_desc.mme_ue_session_pools.nb_bearers_since_last_stat--;
    update_mme_app_stats_s1u_bearer_sub();
    update_mme_app_stats_default_bearer_sub();

    /**
     * No recursion needed any more. This will just inform the EMM/ESM that a PDN session has been deactivated.
     * It will determine what to do based on if its a PDN Disconnect Process or an (implicit) detach.
     */
    MessageDef * message_p = itti_alloc_new_message (TASK_MME_APP, NAS_PDN_DISCONNECT_RSP);
    // do this because of same message types name but not same struct in different .h
    message_p->ittiMsg.nas_pdn_disconnect_rsp.ue_id           = nas_pdn_disconnect_req_pP->ue_id;
    message_p->ittiMsg.nas_pdn_disconnect_rsp.cause           = REQUEST_ACCEPTED;
    /*
     * We don't have an indicator, the message may come out of order. The only true indicator would be the GTPv2c transaction, which we don't have.
     * We use the esm_proc_data to find the correct PDN in ESM.
     */
    /** The only matching is made in the esm data context (pti specific). */
    itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);

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
//    DevAssert(bearer_context_to_deactivate->bearer_state & BEARER_STATE_ACTIVE); // Cannot be in IDLE mode?
//  }

//  mme_app_get_pdn_context(ue_context, nas_pdn_disconnect_req_pP->pdn_cid, nas_pdn_disconnect_req_pP->default_ebi, nas_pdn_disconnect_req_pP->apn,  &pdn_context);
//  if(!pdn_context){
//    OAILOG_ERROR (LOG_MME_APP, "No PDN context found for pdn_cid %d for UE " MME_UE_S1AP_ID_FMT ". \n", nas_pdn_disconnect_req_pP->pdn_cid, nas_pdn_disconnect_req_pP->ue_id);
//    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
//  }

  /**
   * Check if the UE was deregisterd when the message was sent.
   * TAU might be sent in deregistered, but the S11 response might be received in REGISTERED state (response currently not used).
   */
  /** Don't change the bearer state. Send Delete Session Request to SAE-GW. No transaction needed. */
  if(((struct sockaddr*)&nas_pdn_disconnect_req_pP->saegw_s11_ip_addr)->sa_family != 0){
	uint8_t internal_flags = (ue_context->privates.fields.mm_state == UE_UNREGISTERED) ? INTERNAL_FLAG_SKIP_RESPONSE : INTERNAL_FLAG_NULL;
    rc =  mme_app_send_delete_session_request(ue_context, nas_pdn_disconnect_req_pP->default_ebi, &nas_pdn_disconnect_req_pP->saegw_s11_ip_addr,
    		nas_pdn_disconnect_req_pP->saegw_s11_teid, nas_pdn_disconnect_req_pP->noDelete,
    		nas_pdn_disconnect_req_pP->handover, internal_flags);
  } else {
    OAILOG_WARNING(LOG_MME_APP, "NO S11 SAE-GW S11 IPv4 address in nas_pdn_connectivity of ueId : " MME_UE_S1AP_ID_FMT "\n", nas_pdn_disconnect_req_pP->ue_id);
  }
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

// sent by NAS
//------------------------------------------------------------------------------
void
mme_app_handle_conn_est_cnf (
  itti_nas_conn_est_cnf_t * const nas_conn_est_cnf_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s               *ue_session_pool = NULL;
  struct ue_context_s               	 *ue_context = NULL;
  MessageDef                             *message_p = NULL;
  itti_mme_app_connection_establishment_cnf_t *establishment_cnf_p = NULL;

  OAILOG_DEBUG (LOG_MME_APP, "Received NAS_CONNECTION_ESTABLISHMENT_CNF from NAS\n");
  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, nas_conn_est_cnf_pP->ue_id);
  ue_context 	 = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, nas_conn_est_cnf_pP->ue_id);

  if (ue_session_pool == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "UE session pool doesn't exist for UE " MME_UE_S1AP_ID_FMT "\n", nas_conn_est_cnf_pP->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  if (ue_context == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "UE context doesn't exist for UE " MME_UE_S1AP_ID_FMT "\n", nas_conn_est_cnf_pP->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  message_p = itti_alloc_new_message (TASK_MME_APP, MME_APP_CONNECTION_ESTABLISHMENT_CNF);
  establishment_cnf_p = &message_p->ittiMsg.mme_app_connection_establishment_cnf;
  establishment_cnf_p->ue_id = nas_conn_est_cnf_pP->ue_id;  /**< Just set the MME_UE_S1AP_ID as identifier, the S1AP layer will set the enb_ue_s1ap_id from the ue_reference. */
  /*
   * Add the subscribed UE-AMBR values.
   */
  //#pragma message  "Check ue_context ambr"
  // todo: fix..
  ambr_t total_ue_ambr = mme_app_total_p_gw_apn_ambr(ue_session_pool);
  establishment_cnf_p->ue_ambr.br_dl = total_ue_ambr.br_dl;
  establishment_cnf_p->ue_ambr.br_ul = total_ue_ambr.br_ul; /**< No conversion needed. */
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

  pdn_context_t * established_pdn = NULL;
  RB_FOREACH (established_pdn, PdnContexts, &ue_session_pool->pdn_contexts) {
    DevAssert(established_pdn);
    bearer_context_new_t * bc_context = NULL;
    STAILQ_FOREACH(bc_context, &established_pdn->session_bearers, entries) { // todo: @ handover (idle mode tau) this should give us the default ebi!
      if(bc_context){ // todo: check for error cases in handover.. if this can ever be null..
    	  /** Only add ESM active bearers (CBR might be pending). */
    	  if(ue_context->privates.fields.mm_state == UE_REGISTERED){
    		  if(bc_context->ebi != established_pdn->default_ebi){
    			  if(bc_context->esm_ebr_context.status != ESM_EBR_ACTIVE){
    				  OAILOG_WARNING(LOG_MME_APP, "Skipping dedicated bearer with ebi %d from establishment for UE " MME_UE_S1AP_ID_FMT", since ESM is not in ACTIVE state, but %d.\n",
    						  bc_context->ebi, ue_context->privates.mme_ue_s1ap_id, bc_context->esm_ebr_context.status);
    				  continue;
    			  }
    		  }
    	  }
//        if ((BEARER_STATE_SGW_CREATED  || BEARER_STATE_S1_RELEASED) & bc_session->bearer_state) {    /**< It could be in IDLE mode. */
          establishment_cnf_p->e_rab_id[establishment_cnf_p->no_of_e_rabs]                                 = bc_context->ebi ;//+ EPS_BEARER_IDENTITY_FIRST;
          establishment_cnf_p->e_rab_level_qos_qci[establishment_cnf_p->no_of_e_rabs]                      = bc_context->bearer_level_qos.qci;
          establishment_cnf_p->e_rab_level_qos_priority_level[establishment_cnf_p->no_of_e_rabs]           = bc_context->bearer_level_qos.pl;
          establishment_cnf_p->e_rab_level_qos_preemption_capability[establishment_cnf_p->no_of_e_rabs]    = bc_context->bearer_level_qos.pci == 0 ? 1 : 0;
          establishment_cnf_p->e_rab_level_qos_preemption_vulnerability[establishment_cnf_p->no_of_e_rabs] = bc_context->bearer_level_qos.pvi == 0 ? 1 : 0;
          establishment_cnf_p->transport_layer_address[establishment_cnf_p->no_of_e_rabs]                  = fteid_ip_address_to_bstring(&bc_context->s_gw_fteid_s1u);
          establishment_cnf_p->gtp_teid[establishment_cnf_p->no_of_e_rabs]                                 = bc_context->s_gw_fteid_s1u.teid;
          //      if (!j) { // todo: ESM message may exist --> should match each to the EBI!
          if(establishment_cnf_p->no_of_e_rabs == 0){
            establishment_cnf_p->nas_pdu[establishment_cnf_p->no_of_e_rabs]                                  = nas_conn_est_cnf_pP->nas_msg;
            nas_conn_est_cnf_pP->nas_msg = NULL; /**< Unlink. */
            //    }
          }
          establishment_cnf_p->no_of_e_rabs++;
//        }
      }
    }
  }

//  pdn_context_t * first_pdn = RB_MIN(PdnContexts, &ue_context->pdn_contexts);
//  DevAssert(first_pdn);
//
//  bearer_context_t * first_bearer = RB_MIN(SessionBearers, &first_pdn->session_bearers); // todo: @ handover (idle mode tau) this should give us the default ebi!
//  if(first_bearer){ // todo: optimize this!
////    if ((BEARER_STATE_SGW_CREATED  || BEARER_STATE_S1_RELEASED) & first_bearer->bearer_state) {    /**< It could be in IDLE mode. */
//      establishment_cnf_p->e_rab_id[0]                                 = first_bearer->ebi ;//+ EPS_BEARER_IDENTITY_FIRST;
//      establishment_cnf_p->e_rab_level_qos_qci[0]                      = first_bearer->qci;
//      establishment_cnf_p->e_rab_level_qos_priority_level[0]           = first_bearer->priority_level;
//      establishment_cnf_p->e_rab_level_qos_preemption_capability[0]    = first_bearer->preemption_capability;
//      establishment_cnf_p->e_rab_level_qos_preemption_vulnerability[0] = first_bearer->preemption_vulnerability;
//      establishment_cnf_p->transport_layer_address[0]                  = fteid_ip_address_to_bstring(&first_bearer->s_gw_fteid_s1u);
//      establishment_cnf_p->gtp_teid[0]                                 = first_bearer->s_gw_fteid_s1u.teid;
////      if (!j) { // todo: ESM message may exist --> should match each to the EBI!
//      establishment_cnf_p->nas_pdu[0]                                  = nas_conn_est_cnf_pP->nas_msg;
//      nas_conn_est_cnf_pP->nas_msg = NULL; /**< Unlink. */
////    }
//  }

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
  if(ue_context->privates.fields.ecm_state == ECM_CONNECTED){
    /** UE is in connected state. Send S1AP message. */
  }else{
    OAILOG_ERROR (LOG_MME_APP, "EMM UE context should be in connected state for UE id %d, insted idle (initial_ctx_setup_cnf). \n", ue_context->privates.mme_ue_s1ap_id);

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
  if (timer_setup (ue_context->privates.initial_context_setup_rsp_timer.sec, 0,
      TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *) &(ue_context->privates.mme_ue_s1ap_id), &(ue_context->privates.initial_context_setup_rsp_timer.id)) < 0) {
    OAILOG_ERROR (LOG_MME_APP, "Failed to start initial context setup response timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
    ue_context->privates.initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  } else {
    OAILOG_DEBUG (LOG_MME_APP, "MME APP : Sent Initial context Setup Request and Started guard timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
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

  MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, initial_pP->ecgi.cell_identity.enb_id, initial_pP->enb_ue_s1ap_id);

  if(mme_app_desc.nb_ue_attached == mme_config.max_ues ){
	  OAILOG_WARNING(LOG_MME_APP, "Max number of UEs (%d) is reached.. rejecting further UE with " ENB_UE_S1AP_ID_FMT". \n",
			  mme_app_desc.nb_ue_attached, initial_pP->enb_ue_s1ap_id);
	  mme_app_itti_ue_context_release(0, initial_pP->enb_ue_s1ap_id, S1AP_SYSTEM_FAILURE, initial_pP->ecgi.cell_identity.enb_id);
	  OAILOG_FUNC_OUT (LOG_MME_APP);
  }
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
        /*
         * The EMM context may have been removed, due to some erroneous conditions (NAS-NON Delivery, etc.).
         * In this case, we invalidate the EMM context for now.
         */
        if(!ue_context){
          /** todo: later fix this. */
          OAILOG_WARNING(LOG_MME_APP, "ToDo: An EMM context for the given UE exists, but not NAS context. "
              "This is not implemented now and will be added later (reattach without security). Currently invalidating NAS context. Continuing with the initial UE message. \n");

          /** Remove the UE reference implicitly, and then the old context. */
          ue_description_t * temp_ue_reference = s1ap_is_enb_ue_s1ap_id_in_list_per_enb(initial_pP->enb_ue_s1ap_id, initial_pP->ecgi.cell_identity.enb_id);
          if(temp_ue_reference){
            OAILOG_ERROR (LOG_MME_APP, "MME_APP_INITAIL_UE_MESSAGE. ERROR***** Removing the newly created s1ap UE reference with enbUeS1apId " ENB_UE_S1AP_ID_FMT " and enbId %d.\n" ,
                initial_pP->enb_ue_s1ap_id, initial_pP->ecgi.cell_identity.enb_id);
            s1ap_remove_ue(temp_ue_reference);
          }
          message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
          DevAssert (message_p != NULL);
          itti_nas_implicit_detach_ue_ind_t *nas_implicit_detach_ue_ind_p = &message_p->ittiMsg.nas_implicit_detach_ue_ind;
          memset ((void*)nas_implicit_detach_ue_ind_p, 0, sizeof (itti_nas_implicit_detach_ue_ind_t));
          message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_nas_ctx->ue_id;
          itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
          OAILOG_INFO(LOG_MME_APP, "Informed NAS about the invalidated NAS context. Dropping the initial UE request for enbUeS1apId " ENB_UE_S1AP_ID_FMT". \n", initial_pP->enb_ue_s1ap_id);
          OAILOG_FUNC_OUT (LOG_MME_APP);
        }

        if ((ue_context != NULL) && (ue_context->privates.mme_ue_s1ap_id == ue_nas_ctx->ue_id)) {
          initial_pP->mme_ue_s1ap_id = ue_nas_ctx->ue_id;
          if (ue_context->privates.enb_s1ap_id_key != INVALID_ENB_UE_S1AP_ID_KEY)
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
              if(ue_context->privates.fields.mm_state == UE_REGISTERED){
                if(s10_handover_proc->pending_clear_location_request){
                  OAILOG_INFO(LOG_MME_APP, "UE_CONTEXT has a handover procedure active with CLR flag set for UE with IMSI " IMSI_64_FMT " and " MME_UE_S1AP_ID_FMT ". "
                      "Aborting the handover procedure and thereby resetting the CLR flag. \n",
                      ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id);
                }
                /*
                 * Abort the handover procedure by just deleting it. It should remove the timers, if they exist.
                 * The S1AP UE Reference timer should be independent, run and remove the old ue_referece (cannot use it anymore).
                 */
                mme_app_delete_s10_procedure_mme_handover(ue_context);
              }else{
                OAILOG_INFO(LOG_MME_APP, "UE_CONTEXT has a handover procedure active but is not REGISTERED (flag set for UE with IMSI " IMSI_64_FMT " and " MME_UE_S1AP_ID_FMT ". "
                    "Dropping the received initial context request message (attach should be possible after timeout of procedure has occurred). \n",
                    ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id);
                DevAssert(s10_handover_proc->proc.timer.id != MME_APP_TIMER_INACTIVE_ID);
                /*
                 * Error during ue context malloc.
                 * todo: removing the UE reference?!
                 */
                hashtable_rc_t result_deletion = hashtable_uint64_ts_remove (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl,
                    (const hash_key_t)enb_s1ap_id_key);
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
                  "Continuing to handle it (might be a battery reject). \n", ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id);
            }
            /*
             * This only removed the MME_UE_S1AP_ID from enb_s1ap_id_key, it won't remove the UE_REFERENCE itself.
             * todo: @ lionel:           duplicate_enb_context_detected  flag is not checked anymore (NAS).
             */
            ue_description_t * old_ue_reference = s1ap_is_enb_ue_s1ap_id_in_list_per_enb(ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id);
            if(old_ue_reference){
              OAILOG_ERROR (LOG_MME_APP, "MME_APP_INITIAL_UE_MESSAGE. ERROR***** Found an old UE_REFERENCE with enbUeS1apId " ENB_UE_S1AP_ID_FMT " and enbId %d.\n" ,
                  old_ue_reference->enb_ue_s1ap_id, ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id);
              s1ap_remove_ue(old_ue_reference);
//              OAILOG_WARNING (LOG_MME_APP, "MME_APP_INITAIL_UE_MESSAGE. ERROR***** Removed old UE_REFERENCE with enbUeS1apId " ENB_UE_S1AP_ID_FMT " and enbId %d.\n" ,
//                  old_ue_reference->enb_ue_s1ap_id, ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id);
            }
            /*
             * Ideally this should never happen. When UE move to IDLE this key is set to INVALID.
             * Note - This can happen if eNB detects RLF late and by that time UE sends Initial NAS message via new RRC
             * connection.
             * However if this key is valid, remove the key from the hashtable.
             */
            hashtable_rc_t result_deletion = hashtable_uint64_ts_remove (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl,
                (const hash_key_t)ue_context->privates.enb_s1ap_id_key);
            OAILOG_ERROR (LOG_MME_APP, "MME_APP_INITAIL_UE_MESSAGE. ERROR***** enb_s1ap_id_key %ld has valid value %ld. Result of deletion %d.\n" ,
                ue_context->privates.enb_s1ap_id_key,
                ue_context->privates.fields.enb_ue_s1ap_id,
                result_deletion);
            ue_context->privates.enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;
          }
          // Update MME UE context with new enb_ue_s1ap_id
          ue_context->privates.fields.enb_ue_s1ap_id = initial_pP->enb_ue_s1ap_id;
          // regenerate the enb_s1ap_id_key as enb_ue_s1ap_id is changed.
          // todo: also here home_enb_id
          // Update enb_s1ap_id_key in hashtable
          mme_ue_context_update_coll_keys( &mme_app_desc.mme_ue_contexts,
              ue_context,
              enb_s1ap_id_key,  /**< Generated first. */
              ue_nas_ctx->ue_id,
              ue_nas_ctx->_imsi64,
              ue_context->privates.fields.mme_teid_s11,
              ue_context->privates.fields.local_mme_teid_s10,
              &guti);
          /** Set the UE in ECM-Connected state. */
          // todo: checking before
          mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_CONNECTED);
        }
      } else {
        OAILOG_DEBUG (LOG_MME_APP, "MME_APP_INITIAL_UE_MESSAGE with mme code %u and S-TMSI %u:"
            "no EMM UE context found \n", initial_pP->opt_s_tmsi.mme_code, initial_pP->opt_s_tmsi.m_tmsi);
        /** Check that also no MME_APP UE context exists for the given GUTI. */
        // todo: check
        if(mme_ue_context_exists_guti(&mme_app_desc.mme_ue_contexts, &guti) != NULL){
          OAILOG_ERROR (LOG_MME_APP, "UE EXIST WITH GUTI!\n.");
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

  // todo: checking UE session pool, too
  if (!(ue_context)) {
    OAILOG_DEBUG (LOG_MME_APP, "UE context doesn't exist -> create one \n");

    if (!(ue_context = get_new_ue_context ())) {
      /*
       * Error during UE context malloc.
       * todo: removing the UE reference?!
       */
    	mme_app_itti_ue_context_release(0, initial_pP->enb_ue_s1ap_id, S1AP_SYSTEM_FAILURE, initial_pP->ecgi.cell_identity.enb_id);

    	OAILOG_ERROR (LOG_MME_APP, "Failed to create new MME UE context enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT "\n", initial_pP->enb_ue_s1ap_id);
    	OAILOG_FUNC_OUT (LOG_MME_APP);
    }
    /** Initialize the fields of the MME_APP context. */
    ue_context->privates.fields.enb_ue_s1ap_id    = initial_pP->enb_ue_s1ap_id;
    // todo: check if this works for home and macro enb id
    MME_APP_ENB_S1AP_ID_KEY(ue_context->privates.enb_s1ap_id_key, initial_pP->ecgi.cell_identity.enb_id, initial_pP->enb_ue_s1ap_id);
    ue_context->privates.fields.sctp_assoc_id_key = initial_pP->sctp_assoc_id;
    OAILOG_DEBUG (LOG_MME_APP, "Created new MME UE context enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT "\n", initial_pP->enb_ue_s1ap_id);

    mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_CONNECTED);

    teid_t teid = __sync_fetch_and_add (&mme_app_teid_generator, 0x00000001);
    mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
   		 ue_context->privates.enb_s1ap_id_key,
   		 ue_context->privates.mme_ue_s1ap_id,
   		 ue_context->privates.fields.imsi,
   		 teid,       // mme_s11_teid is new
   		 ue_context->privates.fields.local_mme_teid_s10,
   		 &ue_context->privates.fields.guti);

    /** Try to get a new UE session pool, too (before starting with the registrations). */
    ue_session_pool_t * ue_session_pool = get_new_session_pool(ue_context->privates.mme_ue_s1ap_id);
    if(!ue_session_pool){
    	OAILOG_CRITICAL (LOG_MME_APP, "Could not get a new UE session pool for UE " MME_UE_S1AP_ID_FMT".\n", ue_context->privates.mme_ue_s1ap_id);
    	/** Deallocate the ue context and remove from MME_APP map. */
    	mme_remove_ue_context(&mme_app_desc.mme_ue_contexts, ue_context);
    	/** Send back failure. */
    	mme_app_itti_ue_context_release(ue_context->privates.mme_ue_s1ap_id, initial_pP->enb_ue_s1ap_id, S1AP_SYSTEM_FAILURE, initial_pP->ecgi.cell_identity.enb_id);
		OAILOG_FUNC_OUT (LOG_MME_APP);
    }
    /** Update the session keys of the UE session pool, too. */
    mme_ue_session_pool_update_coll_keys(&mme_app_desc.mme_ue_session_pools, ue_session_pool,
    		ue_session_pool->privates.mme_ue_s1ap_id,
			teid       // mme_s11_teid is new
    );
  }
  ue_context->privates.fields.sctp_assoc_id_key = initial_pP->sctp_assoc_id;
  ue_context->privates.fields.e_utran_cgi = initial_pP->ecgi;
  // Notify S1AP about the mapping between mme_ue_s1ap_id and sctp assoc id + enb_ue_s1ap_id
  notify_s1ap_new_ue_mme_s1ap_id_association (ue_context->privates.fields.sctp_assoc_id_key, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);
  // Initialize timers to INVALID IDs
  ue_context->privates.mobile_reachability_timer.id = MME_APP_TIMER_INACTIVE_ID;
  ue_context->privates.implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;
  ue_context->privates.initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  ue_context->privates.initial_context_setup_rsp_timer.sec = MME_APP_INITIAL_CONTEXT_SETUP_RSP_TIMER_VALUE;
  /** Inform the NAS layer about the new initial UE context. */
  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_INITIAL_UE_MESSAGE);
  // do this because of same message types name but not same struct in different .h
  message_p->ittiMsg.nas_initial_ue_message.nas.ue_id           = ue_context->privates.mme_ue_s1ap_id;
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

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_INITIAL_UE_MESSAGE UE id " MME_UE_S1AP_ID_FMT " ", ue_context->privates.mme_ue_s1ap_id);
  itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_bearer_ctx_retry(mme_ue_s1ap_id_t ue_id){
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s               *ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id( &mme_app_desc.mme_ue_session_pools, ue_id);
  MessageDef                             *message_p = NULL;

  mme_app_s11_proc_t * s11_proc = mme_app_get_s11_procedure(ue_session_pool); /**< Currently, assuming that only one could exist. */

  /** Before retrying, we check if the default bearer has been activated (MBR) procedure completed). */

  if(s11_proc){
	  if(s11_proc->proc.in_progress){
		  /** Check if the number of unhandled bearers is 0 (might not be sent due to waiting of MBR). */
		  if(s11_proc->num_bearers_unhandled){
			  OAILOG_INFO(LOG_MME_APP, "S11 procedure with type %d for UE " MME_UE_S1AP_ID_FMT " is already as in progress (and still some bearers left unhandled). Not retriggering S11 procedure.\n",
					  s11_proc->proc.type, ue_id);
			  OAILOG_FUNC_OUT (LOG_MME_APP);
		  }
	  }
	  switch(s11_proc->type){
	  case MME_APP_S11_PROC_TYPE_CREATE_BEARER:{
		  /** Check if the default bearer is active, in any case. */
		  mme_app_s11_proc_create_bearer_t * s11_proc_cbr = (mme_app_s11_proc_create_bearer_t *)s11_proc;
		  /** Check if any cause value is set. */
		  pdn_context_t * pdn_context = NULL;
		  mme_app_get_pdn_context(ue_id, s11_proc_cbr->pci, s11_proc_cbr->linked_ebi, NULL, &pdn_context);
		  if(pdn_context) {
			  bearer_context_new_t * default_bc = mme_app_get_session_bearer_context(pdn_context, s11_proc_cbr->linked_ebi);
			  if(default_bc) {
			    if(default_bc->bearer_state & BEARER_STATE_ACTIVE) {
			      if(!s11_proc->num_bearers_unhandled) {
			    	  mme_app_send_s11_create_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11,
			    			  ue_session_pool->privates.fields.saegw_teid_s11, 0,
							  s11_proc_cbr->proc.s11_trxn, s11_proc_cbr->bcs_tbc);
			    	  OAILOG_WARNING(LOG_MME_APP, "S11-CBR procedure for UE " MME_UE_S1AP_ID_FMT " had no unhandled bearers left. Retriggered response after MBResp & removing procedure.\n",ue_id);
			    	  mme_app_delete_s11_procedure_create_bearer(ue_session_pool);
			    	  OAILOG_FUNC_OUT (LOG_MME_APP);
			      }
			  	  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_ACTIVATE_EPS_BEARER_CTX_REQ);
			  	  AssertFatal (message_p , "itti_alloc_new_message Failed");

			  	  /** Reset the procedure. */
			  	  s11_proc_cbr->proc.num_bearers_unhandled = s11_proc_cbr->bcs_tbc->num_bearer_context;

			  	  itti_nas_activate_eps_bearer_ctx_req_t *nas_activate_eps_bearer_ctx_req = &message_p->ittiMsg.nas_activate_eps_bearer_ctx_req;
			  	  nas_activate_eps_bearer_ctx_req->bcs_to_be_created_ptr = (uintptr_t)s11_proc_cbr->bcs_tbc;
			  	  nas_activate_eps_bearer_ctx_req->retry 			  = true;
			  	  /** MME_APP Create Bearer Request. */
			  	  nas_activate_eps_bearer_ctx_req->ue_id              = ue_session_pool->privates.mme_ue_s1ap_id;
			  	  nas_activate_eps_bearer_ctx_req->linked_ebi         = s11_proc_cbr->linked_ebi;
			  	  /** Copy the BC to be created. */
			  	  /** Might be UE triggered. */
			  	  nas_activate_eps_bearer_ctx_req->pti                = s11_proc_cbr->proc.pti;
			  	  nas_activate_eps_bearer_ctx_req->cid                = s11_proc_cbr->pci;
			  	  /** No need to set bearer states, we won't establish the bearers yet. */
			  	  itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
			  	  OAILOG_FUNC_OUT (LOG_MME_APP);
			    }
			  }
		  }
		  OAILOG_ERROR (LOG_MME_APP, "Default bearer context of UE " MME_UE_S1AP_ID_FMT " is not in active state. Not retriggering CBR procedure.\n", ue_id);
		  OAILOG_FUNC_OUT (LOG_MME_APP);
	  }
	  break;
	  case MME_APP_S11_PROC_TYPE_UPDATE_BEARER:{
		  mme_app_s11_proc_update_bearer_t * s11_proc_ubr = (mme_app_s11_proc_update_bearer_t *)s11_proc;
		  pdn_context_t * pdn_context = NULL;
		  mme_app_get_pdn_context(ue_id, s11_proc_ubr->pci, s11_proc_ubr->linked_ebi, NULL, &pdn_context);
		  if(pdn_context) {
			  bearer_context_new_t * default_bc = mme_app_get_session_bearer_context(pdn_context, s11_proc_ubr->linked_ebi);
			  if(default_bc) {
				  if(default_bc->bearer_state & BEARER_STATE_ACTIVE) {
				      if(!s11_proc->num_bearers_unhandled) {
				    	  mme_app_send_s11_update_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11,
				    			  ue_session_pool->privates.fields.saegw_teid_s11, 0,
								  s11_proc_ubr->proc.s11_trxn, s11_proc_ubr->bcs_tbu);
				    	  OAILOG_WARNING(LOG_MME_APP, "S11-UBR procedure for UE " MME_UE_S1AP_ID_FMT " had no unhandled bearers left. Retriggered response after MBResp & removing procedure.\n",ue_id);
				    	  mme_app_delete_s11_procedure_update_bearer(ue_session_pool);
				    	  OAILOG_FUNC_OUT (LOG_MME_APP);
				      }

					  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_MODIFY_EPS_BEARER_CTX_REQ);
					  AssertFatal (message_p , "itti_alloc_new_message Failed");
					  /** Reset the procedure. */
					  s11_proc_ubr->proc.num_bearers_unhandled = s11_proc_ubr->bcs_tbu->num_bearer_context;

					  itti_nas_modify_eps_bearer_ctx_req_t *nas_modify_eps_bearer_ctx_req = &message_p->ittiMsg.nas_modify_eps_bearer_ctx_req;
					  nas_modify_eps_bearer_ctx_req->bcs_to_be_updated_ptr = (uintptr_t)s11_proc_ubr->bcs_tbu;
					  nas_modify_eps_bearer_ctx_req->retry    			= true;
				  	  /** NAS Update Bearer Request. The ESM layer will also check the APN-AMBR. */
					  nas_modify_eps_bearer_ctx_req->ue_id              = ue_session_pool->privates.mme_ue_s1ap_id;
					  /** Might be UE triggered. */
					  nas_modify_eps_bearer_ctx_req->pti                = s11_proc_ubr->proc.pti;
					  nas_modify_eps_bearer_ctx_req->apn_ambr           = s11_proc_ubr->apn_ambr;
					  /** No need to set bearer states, we won't establish the bearers yet. */
					  itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
					  OAILOG_FUNC_OUT (LOG_MME_APP);
				  }
			  }
	  	  }
		  OAILOG_ERROR (LOG_MME_APP, "Default bearer context of UE " MME_UE_S1AP_ID_FMT " is not in active state. Not retriggering UBR procedure.\n", ue_id);
		  OAILOG_FUNC_OUT (LOG_MME_APP);
	  }
	  break;
	  case MME_APP_S11_PROC_TYPE_DELETE_BEARER:{
		  mme_app_s11_proc_delete_bearer_t * s11_proc_dbr = (mme_app_s11_proc_delete_bearer_t *)s11_proc;
		  bearer_context_new_t * ded_bc = NULL;
		  if(s11_proc_dbr->ebis.num_ebi){
			  mme_app_get_session_bearer_context_from_all(ue_session_pool, s11_proc_dbr->ebis.ebis[0], &ded_bc);
			  if(ded_bc) {
				  /** Check the default bearer of it. */
				  bearer_context_new_t * default_bearer = NULL;
				  mme_app_get_session_bearer_context_from_all(ue_session_pool, ded_bc->linked_ebi, &default_bearer);
				  if(default_bearer->bearer_state & BEARER_STATE_ACTIVE) {
					  if(!s11_proc->num_bearers_unhandled) {
						  mme_app_send_s11_delete_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11,
								  ue_session_pool->privates.fields.saegw_teid_s11, REQUEST_ACCEPTED,
								  s11_proc_dbr->proc.s11_trxn, &s11_proc_dbr->ebis);
						  OAILOG_WARNING(LOG_MME_APP, "S11-DBR procedure for UE " MME_UE_S1AP_ID_FMT " had no unhandled bearers left. Retriggered response after MBResp & removing procedure.\n",ue_id);
						  mme_app_delete_s11_procedure_delete_bearer(ue_session_pool);
						  OAILOG_FUNC_OUT (LOG_MME_APP);
					  }

					  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_DEACTIVATE_EPS_BEARER_CTX_REQ);
					  AssertFatal (message_p , "itti_alloc_new_message Failed");
					  /** Reset the procedure. */
					  s11_proc_dbr->proc.num_bearers_unhandled = s11_proc_dbr->ebis.num_ebi;

					  itti_nas_deactivate_eps_bearer_ctx_req_t *nas_deactivate_eps_bearer_ctx_req = &message_p->ittiMsg.nas_deactivate_eps_bearer_ctx_req;
					  nas_deactivate_eps_bearer_ctx_req->ue_id              = ue_session_pool->privates.mme_ue_s1ap_id;
					  nas_deactivate_eps_bearer_ctx_req->retry    			= true;
					  nas_deactivate_eps_bearer_ctx_req->def_ebi            = default_bearer->linked_ebi;
					  memcpy(&nas_deactivate_eps_bearer_ctx_req->ebis, &s11_proc_dbr->ebis, sizeof(s11_proc_dbr->ebis));
					  /** Might be UE triggered. */
					  nas_deactivate_eps_bearer_ctx_req->pti                = s11_proc_dbr->proc.pti;
					  /** No need to set bearer states, we won't remove the bearers yet. */
					  itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
					  OAILOG_FUNC_OUT (LOG_MME_APP);
				  }
			  }
		  }
		  OAILOG_ERROR (LOG_MME_APP, "Could not find first dedicated bearer context of UE " MME_UE_S1AP_ID_FMT " in active state. Not retriggering DBR procedure.\n", ue_id);
		  OAILOG_FUNC_OUT (LOG_MME_APP);
	  }
	  break;
	  default:
		  DevMessage("Procedure " + s11_proc->type + "could not be identified as a valid S11 procedure for for UE with mmeUeS1apId " + ue_session_pool->privates.mme_ue_s1ap_id+ ".\n");
	  }
  }
  OAILOG_INFO(LOG_MME_APP, "No S11 procedure could be found for UE " MME_UE_S1AP_ID_FMT". \n", ue_session_pool->privates.mme_ue_s1ap_id);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_nas_erab_setup_req (itti_nas_erab_setup_req_t * const itti_nas_erab_setup_req)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s                    *ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id( &mme_app_desc.mme_ue_session_pools, itti_nas_erab_setup_req->ue_id);

  if (!ue_session_pool) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " NAS_ERAB_SETUP_REQ Unknown ue session pool " MME_UE_S1AP_ID_FMT " ", itti_nas_erab_setup_req->ue_id);
    OAILOG_ERROR (LOG_MME_APP, "UE context doesn't exist for UE Session Pool " MME_UE_S1AP_ID_FMT "\n", itti_nas_erab_setup_req->ue_id);
    // memory leak
    bdestroy_wrapper(&itti_nas_erab_setup_req->nas_msg);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  bearer_context_new_t* bearer_context = NULL;
  mme_app_get_session_bearer_context_from_all(ue_session_pool, itti_nas_erab_setup_req->ebi, &bearer_context);

  if (bearer_context) {
    MessageDef  *message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_E_RAB_SETUP_REQ);
    itti_s1ap_e_rab_setup_req_t *s1ap_e_rab_setup_req = &message_p->ittiMsg.s1ap_e_rab_setup_req;

    s1ap_e_rab_setup_req->mme_ue_s1ap_id = ue_session_pool->privates.mme_ue_s1ap_id;

    // E-RAB to Be Setup List
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.no_of_items = 1;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_id = bearer_context->ebi;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.allocation_and_retention_priority.pre_emption_capability =
        bearer_context->bearer_level_qos.pci == 0 ? 1 : 0;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.allocation_and_retention_priority.pre_emption_vulnerability =
        bearer_context->bearer_level_qos.pvi == 0 ? 1 : 0;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.allocation_and_retention_priority.priority_level =
        bearer_context->bearer_level_qos.pl;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.gbr_qos_information.e_rab_maximum_bit_rate_downlink    = itti_nas_erab_setup_req->mbr_dl;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.gbr_qos_information.e_rab_maximum_bit_rate_uplink      = itti_nas_erab_setup_req->mbr_ul;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.gbr_qos_information.e_rab_guaranteed_bit_rate_downlink = itti_nas_erab_setup_req->gbr_dl;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.gbr_qos_information.e_rab_guaranteed_bit_rate_uplink   = itti_nas_erab_setup_req->gbr_ul;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_level_qos_parameters.qci = bearer_context->bearer_level_qos.qci;

    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].gtp_teid = bearer_context->s_gw_fteid_s1u.teid;
    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].transport_layer_address = fteid_ip_address_to_bstring(&bearer_context->s_gw_fteid_s1u);

    s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].nas_pdu = itti_nas_erab_setup_req->nas_msg;
    itti_nas_erab_setup_req->nas_msg = NULL;

    /**
     * Check if there is a dedicated bearer procedure ongoing.
     * If not this is multi apn. In that case check the new resulting UE-AMBR.
     */
    mme_app_s11_proc_create_bearer_t *s11_proc_cbr = NULL;
    if(!(s11_proc_cbr = mme_app_get_s11_procedure_create_bearer(ue_session_pool))){
      /** No S11 procedure --> multi APN. */
      ambr_t total_apn_ambr = mme_app_total_p_gw_apn_ambr(ue_session_pool);
      /**
       * Should be updated at this point. If the default bearer cannot be set.. pdn will be removed anyway.
       * The actually used AMBR will be sent. It can be higher or lower than the actual used UE-AMBR.
       * Set the new UE-AMBR.
       */
      s1ap_e_rab_setup_req->ue_aggregate_maximum_bit_rate_present = true;
      s1ap_e_rab_setup_req->ue_aggregate_maximum_bit_rate.dl = total_apn_ambr.br_dl;
      s1ap_e_rab_setup_req->ue_aggregate_maximum_bit_rate.ul = total_apn_ambr.br_ul;
      /** Will recalculate these values and set them when the response arrives. */
    }else {
      /** Not setting the updated UE-AMBR value for dedicated bearers. */
      /** Check the default bearer status. */
      bearer_context_new_t * default_bc = NULL;
      mme_app_get_session_bearer_context_from_all(ue_session_pool, bearer_context->linked_ebi, &default_bc);
      if(!default_bc || !(default_bc->bearer_state & BEARER_STATE_ACTIVE)){
    	  OAILOG_ERROR( LOG_MME_APP, "Not triggering dedicated bearer context with ebi %d for UE " MME_UE_S1AP_ID_FMT ", since default bearer is not yet active. \n",
    			  bearer_context->ebi, ue_session_pool->privates.mme_ue_s1ap_id);
    	  OAILOG_FUNC_OUT (LOG_MME_APP);
      }
      if(!s11_proc_cbr->proc.proc.in_progress){
    	  /**< The first time, it will always be sent with an E-RAB message (not downlink NAS). */
    	  OAILOG_INFO( LOG_MME_APP, "Setting S11 Create Bearer process for UE " MME_UE_S1AP_ID_FMT " as in progress. \n", ue_session_pool->privates.mme_ue_s1ap_id);
    	  s11_proc_cbr->proc.proc.in_progress = true;
      }
    }
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0, "0 S1AP_E_RAB_SETUP_REQ ue id " MME_UE_S1AP_ID_FMT " ebi %u teid " TEID_FMT " ",
        ue_context->privates.mme_ue_s1ap_id,
        s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_id,
        s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].gtp_teid);
    int to_task = (RUN_MODE_SCENARIO_PLAYER == mme_config.run_mode) ? TASK_MME_SCENARIO_PLAYER:TASK_S1AP;
    itti_send_msg_to_task (to_task, INSTANCE_DEFAULT, message_p);
  } else {
    OAILOG_DEBUG (LOG_MME_APP, "No bearer context found ue " MME_UE_S1AP_ID_FMT  " ebi %u\n", itti_nas_erab_setup_req->ue_id, itti_nas_erab_setup_req->ebi);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_nas_erab_modify_req (itti_nas_erab_modify_req_t * const itti_nas_erab_modify_req)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s                    *ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, itti_nas_erab_modify_req->ue_id);

  if (!ue_session_pool) {
    OAILOG_ERROR (LOG_MME_APP, "UE context doesn't exist for UE " MME_UE_S1AP_ID_FMT "\n", itti_nas_erab_modify_req->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** Currently only if an S11 procedure exists. */
  mme_app_s11_proc_update_bearer_t * s11_proc_update_bearer = mme_app_get_s11_procedure_update_bearer(ue_session_pool);
  DevAssert(s11_proc_update_bearer);

  bearer_context_new_t* bearer_context = NULL;
  mme_app_get_session_bearer_context_from_all(ue_session_pool, itti_nas_erab_modify_req->ebi, &bearer_context);

  if (bearer_context) {
    /** Check if the bearer is active. */
	/**< The first time, it will always be sent with an E-RAB message (not downlink NAS). */
	if(bearer_context->bearer_state & BEARER_STATE_ACTIVE){
		if(!s11_proc_update_bearer->proc.proc.in_progress){
			OAILOG_INFO( LOG_MME_APP, "Setting S11 Update Bearer process for UE " MME_UE_S1AP_ID_FMT " as in progress. \n", ue_session_pool->privates.mme_ue_s1ap_id);
			s11_proc_update_bearer->proc.proc.in_progress = true;
		}
	    MessageDef  *message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_E_RAB_MODIFY_REQ);
	    itti_s1ap_e_rab_modify_req_t *s1ap_e_rab_modify_req = &message_p->ittiMsg.s1ap_e_rab_modify_req;

	    s1ap_e_rab_modify_req->mme_ue_s1ap_id = itti_nas_erab_modify_req->ue_id;
	    // E-RAB to Be Setup List
	    s1ap_e_rab_modify_req->e_rab_to_be_modified_list.no_of_items = 1;
	    s1ap_e_rab_modify_req->e_rab_to_be_modified_list.item[0].e_rab_id = bearer_context->ebi;
	    s1ap_e_rab_modify_req->e_rab_to_be_modified_list.item[0].e_rab_level_qos_parameters.allocation_and_retention_priority.pre_emption_capability =
	        bearer_context->bearer_level_qos.pci == 0 ? 1 : 0;
	    s1ap_e_rab_modify_req->e_rab_to_be_modified_list.item[0].e_rab_level_qos_parameters.allocation_and_retention_priority.pre_emption_vulnerability =
	        bearer_context->bearer_level_qos.pvi == 0 ? 1 : 0;
	    s1ap_e_rab_modify_req->e_rab_to_be_modified_list.item[0].e_rab_level_qos_parameters.allocation_and_retention_priority.priority_level =
	        bearer_context->bearer_level_qos.pl == 0 ? 1 : 0;
	    s1ap_e_rab_modify_req->e_rab_to_be_modified_list.item[0].e_rab_level_qos_parameters.gbr_qos_information.e_rab_maximum_bit_rate_downlink    = itti_nas_erab_modify_req->mbr_dl;
	    s1ap_e_rab_modify_req->e_rab_to_be_modified_list.item[0].e_rab_level_qos_parameters.gbr_qos_information.e_rab_maximum_bit_rate_uplink      = itti_nas_erab_modify_req->mbr_ul;
	    s1ap_e_rab_modify_req->e_rab_to_be_modified_list.item[0].e_rab_level_qos_parameters.gbr_qos_information.e_rab_guaranteed_bit_rate_downlink = itti_nas_erab_modify_req->gbr_dl;
	    s1ap_e_rab_modify_req->e_rab_to_be_modified_list.item[0].e_rab_level_qos_parameters.gbr_qos_information.e_rab_guaranteed_bit_rate_uplink   = itti_nas_erab_modify_req->gbr_ul;
	    s1ap_e_rab_modify_req->e_rab_to_be_modified_list.item[0].e_rab_level_qos_parameters.qci = bearer_context->bearer_level_qos.qci;

	    /**
	     * UE AMBR: We may have new authorized values in the procedure. Setting them (may be lower or higher).
	     */
	    s1ap_e_rab_modify_req->ue_aggregate_maximum_bit_rate_present = true;
	    s1ap_e_rab_modify_req->ue_aggregate_maximum_bit_rate.dl = s11_proc_update_bearer->new_used_ue_ambr.br_dl;
	    s1ap_e_rab_modify_req->ue_aggregate_maximum_bit_rate.ul = s11_proc_update_bearer->new_used_ue_ambr.br_ul;

	    /** This field is mandatory, always set it. */
	    s1ap_e_rab_modify_req->e_rab_to_be_modified_list.item[0].nas_pdu = itti_nas_erab_modify_req->nas_msg;
	    itti_nas_erab_modify_req->nas_msg = NULL;

	    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0, "0 S1AP_E_RAB_MODIFY_REQ ue id " MME_UE_S1AP_ID_FMT " ebi %u teid " TEID_FMT " ",
	        itti_nas_erab_modify_req->ue_id,
	        s1ap_e_rab_modify_req->e_rab_to_be_modified_list.item[0].e_rab_id,
	        s1ap_e_rab_modify_req->e_rab_to_be_modified_list.item[0].gtp_teid);
	    int to_task = (RUN_MODE_SCENARIO_PLAYER == mme_config.run_mode) ? TASK_MME_SCENARIO_PLAYER:TASK_S1AP;
	    itti_send_msg_to_task (to_task, INSTANCE_DEFAULT, message_p);
	}
  } else {
    OAILOG_DEBUG (LOG_MME_APP, "No bearer context found ue " MME_UE_S1AP_ID_FMT  " ebi %u\n", itti_nas_erab_modify_req->ue_id, itti_nas_erab_modify_req->ebi);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_nas_erab_release_req (mme_ue_s1ap_id_t ue_id, ebi_t ebi, bstring nas_msg)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s                    *ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, ue_id);

  if (!ue_session_pool) {
    OAILOG_ERROR (LOG_MME_APP, "UE context doesn't exist for UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  mme_app_s11_proc_delete_bearer_t * s11_proc_delete_bearer = mme_app_get_s11_procedure_delete_bearer(ue_session_pool);
  if(s11_proc_delete_bearer){
	  /**
	   * Check if it is a dedicated bearer procedure.
	   * The first time, it will always be sent with an E-RAB message (not downlink NAS).
	   */
	  bearer_context_new_t * default_bc = NULL;
	  mme_app_get_session_bearer_context_from_all(ue_session_pool, ebi, &default_bc);
	  if(!default_bc || (default_bc->ebi == ebi) || !(default_bc->bearer_state & BEARER_STATE_ACTIVE)){
		OAILOG_ERROR( LOG_MME_APP, "Not continuing to remove D11 Delete Bearer process for UE " MME_UE_S1AP_ID_FMT " for ebi %d, since default bearer is not active yet. \n",
				ue_session_pool->privates.mme_ue_s1ap_id, ebi);
		OAILOG_FUNC_OUT (LOG_MME_APP);
	  }
	  if(!s11_proc_delete_bearer->proc.proc.in_progress){
		  OAILOG_INFO( LOG_MME_APP, "Setting S11 Delete Bearer process for UE " MME_UE_S1AP_ID_FMT " as in progress. \n", ue_session_pool->privates.mme_ue_s1ap_id);
		  s11_proc_delete_bearer->proc.proc.in_progress = true;
	  }
  }

  /** Send it anyway. */
  MessageDef  *message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_E_RAB_RELEASE_REQ);
  itti_s1ap_e_rab_release_req_t *s1ap_e_rab_release_req = &message_p->ittiMsg.s1ap_e_rab_release_req;

  s1ap_e_rab_release_req->mme_ue_s1ap_id = ue_id;
  // E-RAB to Be Setup List
  s1ap_e_rab_release_req->e_rab_to_be_release_list.no_of_items = 1;
  s1ap_e_rab_release_req->e_rab_to_be_release_list.item[0].e_rab_id = ebi;

  /** Will only be set for the first message if multiple deactivations are to be sent. */
  if(nas_msg){
    s1ap_e_rab_release_req->nas_pdu = bstrcpy(nas_msg);
  }

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0, "0 S1AP_E_RAB_RELEASE_REQ ue id " MME_UE_S1AP_ID_FMT " ebi %u ",
      ue_id,
      s1ap_e_rab_release_req->e_rab_to_be_release_list.item[0].e_rab_id);
  int to_task = (RUN_MODE_SCENARIO_PLAYER == mme_config.run_mode) ? TASK_MME_SCENARIO_PLAYER:TASK_S1AP;
  itti_send_msg_to_task (to_task, INSTANCE_DEFAULT, message_p);
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
  mme_app_desc.mme_ue_session_pools.nb_bearers_managed--;
  mme_app_desc.mme_ue_session_pools.nb_bearers_since_last_stat--;

  /**
   * Object is later removed, not here. For unused keys, this is no problem, just deregistrate the tunnel ids for the MME_APP
   * UE context from the hashtable.
   * If this is not done, later at removal of the MME_APP UE context, the S11 keys will be checked and removed again if still existing.
   */
  // todo: handle this! where to remove the S11 Tunnel?
//  if(ue_context->num_pdns == 1){
//    /** This was the last PDN, removing the S11 TEID. */
//    hashtable_uint64_ts_remove(mme_app_desc.mme_ue_contexts.tun11_ue_context_htbl,
//        (const hash_key_t) ue_context->privates.fields.mme_teid_s11, &id);
//    ue_context->privates.fields.mme_teid_s11 = INVALID_TEID;
//    /** SAE-GW TEID will be initialized when PDN context is purged. */
//  }

   if (delete_sess_resp_pP->cause.cause_value != REQUEST_ACCEPTED) { /**< Ignoring currently, SAE-GW needs to check flag. */
     OAILOG_WARNING (LOG_MME_APP, "***WARNING****S11 Delete Session Rsp: NACK received from SPGW : %08x\n", delete_sess_resp_pP->teid);
   }
   MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 DELETE_SESSION_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
     delete_sess_resp_pP->teid, ue_context->privates.fields.imsi);
   /*
    * Updating statistics
    */
   update_mme_app_stats_s1u_bearer_sub();
   update_mme_app_stats_default_bearer_sub();

   /**
    * No (mobility) flags should have been set at the time of the creation.
    * todo: network triggered pdn/bearer deactivation?!
    */
   if(ue_context->privates.fields.mm_state == UE_REGISTERED && !(delete_sess_resp_pP->internal_flags & INTERNAL_FLAG_SKIP_RESPONSE)) {
     /*
      * No recursion needed any more. This will just inform the EMM/ESM that a PDN session has been deactivated.
      * It will determine what to do based on if its a PDN Disconnect Process or an (implicit) detach.
      */
     message_p = itti_alloc_new_message (TASK_MME_APP, NAS_PDN_DISCONNECT_RSP);
     // do this because of same message types name but not same struct in different .h
     message_p->ittiMsg.nas_pdn_disconnect_rsp.ue_id           = ue_context->privates.mme_ue_s1ap_id;
     message_p->ittiMsg.nas_pdn_disconnect_rsp.cause           = REQUEST_ACCEPTED;
     /*
      * We don't have an indicator, the message may come out of order. The only true indicator would be the GTPv2c transaction, which we don't have.
      * We use the esm_proc_data to find the correct PDN in ESM.
      */
     /** The only matching is made in the esm data context (pti specific). */
     itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
   } else {
     OAILOG_INFO (LOG_MME_APP, " Not forwarding S11 DSResp message for UNREGISTERED UE " MME_UE_S1AP_ID_FMT ". \n", ue_context->privates.mme_ue_s1ap_id);
   }

   /** No S1AP release yet. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
int
mme_app_handle_create_sess_resp (
  itti_s11_create_session_response_t * const create_sess_resp_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    * ue_context = NULL;
  struct ue_session_pool_s               * ue_session_pool = NULL;
  MessageDef                             * message_p = NULL;
  mme_app_s10_proc_mme_handover_t        * s10_handover_procedure;
  mme_ue_s1ap_id_t                         mme_ue_s1ap_id;

  DevAssert (create_sess_resp_pP );
  ue_context = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, create_sess_resp_pP->teid);
  if (ue_context == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_SESSION_RESPONSE local S11 teid " TEID_FMT " ", create_sess_resp_pP->teid);
    OAILOG_ERROR (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", create_sess_resp_pP->teid);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  mme_ue_s1ap_id = ue_context->privates.mme_ue_s1ap_id;

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_session_pools, mme_ue_s1ap_id);
  if(!ue_session_pool){
	  OAILOG_ERROR(LOG_MME_APP, "Aborting CSR procedure for UE " MME_UE_S1AP_ID_FMT " (no UE session pool exists). \n", mme_ue_s1ap_id);
	  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  /** S10 Procedure. */
  s10_handover_procedure = mme_app_get_s10_procedure_mme_handover(ue_context);
  /** Idle TAU procedure. */
  emm_data_context_t * emm_context = emm_data_context_get(&_emm_data, mme_ue_s1ap_id);
  nas_ctx_req_proc_t *emm_cn_proc_ctx_req = NULL;
  if(emm_context){ /**< Might be asynchronously removed. */
    emm_cn_proc_ctx_req = get_nas_cn_procedure_ctx_req(emm_context);
  }
  /** Either continue with another CSReq or signal it to the EMM layer. */
  mme_ue_eps_pdn_connections_t * pdn_connections = NULL;
  if(s10_handover_procedure) {
	  pdn_connections = s10_handover_procedure->pdn_connections;
  } else {
	  if(emm_cn_proc_ctx_req) {
		  pdn_connections = emm_cn_proc_ctx_req->pdn_connections;
	  } else {
		  pdn_connections = NULL;
	  }
  }

  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_SESSION_RESPONSE local S11 teid " TEID_FMT " ",
      create_sess_resp_pP->teid);
  /** Process the received CSResp, equally for handover/TAU or not. If success, the pdn context will be edited, if not it will be freed. */
  if(mme_app_pdn_process_session_creation(mme_ue_s1ap_id, ue_context->privates.fields.imsi,
	  ue_context->privates.fields.mm_state,
	  ue_session_pool->privates.fields.subscribed_ue_ambr,
	  create_sess_resp_pP->linked_eps_bearer_id,
      &create_sess_resp_pP->s11_sgw_fteid,
      &create_sess_resp_pP->cause,
      &create_sess_resp_pP->bearer_contexts_created,
      &create_sess_resp_pP->ambr,
      &create_sess_resp_pP->paa,
      &create_sess_resp_pP->pco) == RETURNerror) {
    OAILOG_ERROR(LOG_MME_APP, "Aborting the CSR procedure due internal error for UE " MME_UE_S1AP_ID_FMT ". Assuming an implicit detach procedure is ongoing (cause_value=%d). \n", mme_ue_s1ap_id, create_sess_resp_pP->cause.cause_value);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  /** If it is a non-handover/idle-TAU case, just process it. */
  if(!pdn_connections){
    /** Normal attach or multi-APN procedure. No need to set the PTI. */
    // todo: an attach procedure might be ongoing, removing emm context and procedures.. this message should have no effect, the implicit detach should be ongoing
    mme_app_itti_nas_pdn_connectivity_response(mme_ue_s1ap_id, create_sess_resp_pP->bearer_contexts_created.bearer_contexts[0].eps_bearer_id, create_sess_resp_pP->cause.cause_value);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
  }

  OAILOG_INFO(LOG_MME_APP, "Processing the CSResp for mobility procedure for UE " MME_UE_S1AP_ID_FMT " (cause_value=%d). \n", mme_ue_s1ap_id, create_sess_resp_pP->cause.cause_value);
  /** Check if there are other PDN contexts to be established. */
  pdn_connections->num_processed_pdn_connections++;
  if(pdn_connections->num_pdn_connections > pdn_connections->num_processed_pdn_connections) {
    OAILOG_INFO(LOG_MME_APP, "We have %d further PDN connections that need to be established via mobile of UE " MME_UE_S1AP_ID_FMT ". \n", (pdn_connections->num_pdn_connections - pdn_connections->num_processed_pdn_connections), mme_ue_s1ap_id);
    pdn_connection_t * pdn_connection = &pdn_connections->pdn_connection[pdn_connections->num_processed_pdn_connections];
    /*
     * When Create Session Response is received, continue to process the next PDN connection, until all are processed.
     * When all pdn_connections are completed, continue with handover request.
     */
    // todo: check target_tai at idle mode
    tai_t  * target_tai = (s10_handover_procedure) ? (&s10_handover_procedure->target_tai) : &emm_context->originating_tai;
    imsi_t * imsi_p     = (s10_handover_procedure) ? (&s10_handover_procedure->nas_s10_context._imsi) : &emm_cn_proc_ctx_req->nas_s10_context._imsi;
    DevAssert(imsi_p && target_tai);
    /** Create a new PDN context with all dedicated bearers in ESM_EBR_ACTIVE state. */
    pdn_context_t * pdn_context = mme_app_handle_pdn_connectivity_from_s10(ue_session_pool, pdn_connection);
    if(pdn_context){
      mme_app_send_s11_create_session_req (ue_context->privates.mme_ue_s1ap_id, imsi_p, pdn_context, target_tai, pdn_context->pco, (!s10_handover_procedure));
      OAILOG_INFO(LOG_MME_APP, "Successfully sent consecutive CSR for APN \"%s\" for UE " MME_UE_S1AP_ID_FMT " due mobility. \n", bdata(pdn_context->apn_subscribed), mme_ue_s1ap_id);
      OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
    } else {
      OAILOG_ERROR(LOG_MME_APP, "Aborting CSR procedure for UE " MME_UE_S1AP_ID_FMT " (assuming detach process is ongoing). \n", mme_ue_s1ap_id);
      OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
    }
  }

  /** Continue with the mobility procedure. */
  pdn_context_t * first_pdn = RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
  if(first_pdn){ /**< If any session success was received in this or prios cases, its important for mobility. */
    OAILOG_INFO(LOG_MME_APP, "At least PDN \"%s\" exists for UE " MME_UE_S1AP_ID_FMT ". Continuing with the mobility procedure. \n",
        bdata(first_pdn->apn_subscribed), mme_ue_s1ap_id);
    if(s10_handover_procedure){
      /** Send a Handover Request to the target eNB. */
      bearer_contexts_to_be_created_t bcs_tbc;
      memset((void*)&bcs_tbc, 0, sizeof(bcs_tbc));
      pdn_context_t * registered_pdn_ctx = NULL;
      RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_session_pool->pdn_contexts) {
        DevAssert(registered_pdn_ctx);
        mme_app_get_bearer_contexts_to_be_created(registered_pdn_ctx, &bcs_tbc, BEARER_STATE_NULL); /**< Actual number of bearers established in the SAE-GW. */
      }
      uint16_t encryption_algorithm_capabilities = (uint16_t)0;
      uint16_t integrity_algorithm_capabilities  = (uint16_t)0;
      /** Update the security parameters of the MM context of the S10 procedure. */
      mm_ue_eps_context_update_security_parameters(mme_ue_s1ap_id, s10_handover_procedure->nas_s10_context.mm_eps_ctx, &encryption_algorithm_capabilities, &integrity_algorithm_capabilities);
      ambr_t total_apn_ambr = mme_app_total_p_gw_apn_ambr(ue_session_pool);
      mme_app_send_s1ap_handover_request(mme_ue_s1ap_id,
          &bcs_tbc,
          &total_apn_ambr,
          // todo: check for macro/home enb_id
          s10_handover_procedure->target_id.target_id.macro_enb_id.enb_id,
          encryption_algorithm_capabilities,
          integrity_algorithm_capabilities,
          s10_handover_procedure->nas_s10_context.mm_eps_ctx->nh,
          s10_handover_procedure->nas_s10_context.mm_eps_ctx->ncc,
          s10_handover_procedure->source_to_target_eutran_f_container.container_value);
      s10_handover_procedure->source_to_target_eutran_f_container.container_value = NULL; /**< Set it to NULL ALWAYS. */
      OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
    } else {
      /** Continue with the idle TAU or attach/multi-APN procedure. */
      mme_app_itti_nas_context_response(ue_context, &emm_cn_proc_ctx_req->nas_s10_context);
      OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
    }
  }
  /** No session could be established at all. */
  OAILOG_ERROR(LOG_MME_APP, "No PDN connectivity could be established for handovered UE " MME_UE_S1AP_ID_FMT ". \n", mme_ue_s1ap_id);
  if(s10_handover_procedure){
    /** No NAS layer exists, because CSR is only sent for S10 handover. */
    mme_app_send_s10_forward_relocation_response_err(s10_handover_procedure->remote_mme_teid.teid,
        s10_handover_procedure->proc.peer_ip,
        s10_handover_procedure->forward_relocation_trxn, RELOCATION_FAILURE);
    /** Perform an implicit NAS detach. */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = mme_ue_s1ap_id;
    itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
  }else {
    /** Respond to the EMM layer about the failed context request. */
    _mme_app_send_nas_context_response_err(mme_ue_s1ap_id, RELOCATION_FAILURE);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
  }
}

//------------------------------------------------------------------------------
int
mme_app_handle_modify_bearer_resp (
  const itti_s11_modify_bearer_response_t * const modify_bearer_resp_pP)
{
  struct ue_context_s                    *ue_context       = NULL;
  struct ue_session_pool_s               *ue_session_pool  = NULL;
  struct pdn_context_s                   *pdn_context      = NULL;
  bearer_context_new_t                   *current_bearer_p = NULL;
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

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_session_pools, ue_context->privates.mme_ue_s1ap_id);
  if(!ue_session_pool){
	  OAILOG_ERROR(LOG_MME_APP, "Aborting CSR procedure for UE " MME_UE_S1AP_ID_FMT " (no UE session pool exists). \n", ue_context->privates.mme_ue_s1ap_id);
	  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  /*
   * Updating statistics
   */
  if (modify_bearer_resp_pP->cause.cause_value != REQUEST_ACCEPTED) {
    /**
     * Check if it is an X2 Handover procedure, in that case send an X2 Path Switch Request Failure to the target MME.
     * In addition, perform an implicit detach in any case.
     */
    if(modify_bearer_resp_pP->internal_flags & INTERNAL_FLAG_X2_HANDOVER){
      OAILOG_ERROR(LOG_MME_APP, "Error modifying SAE-GW bearers for UE with ueId: " MME_UE_S1AP_ID_FMT " (no implicit detach - waiting explicit detach.). \n", ue_context->privates.mme_ue_s1ap_id);
      /** Remove any idles bearers for all the PDNs. */
      pdn_context_t    		* pdn_context = NULL;
      bearer_context_new_t  * pBearerCtx  = NULL;
      /** Set all FTEIDs, also those not in the list to 0. */
      mme_app_send_s1ap_path_switch_request_failure(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.fields.sctp_assoc_id_key, S1ap_Cause_PR_misc);
      /** We continue with the implicit detach, since handover already happened. */
    }
    /** Implicitly detach the UE --> If EMM context is missing, still continue with the resource removal. */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->privates.mme_ue_s1ap_id;
    message_p->ittiMsg.nas_implicit_detach_ue_ind.emm_cause = EMM_CAUSE_NETWORK_FAILURE;
    message_p->ittiMsg.nas_implicit_detach_ue_ind.detach_type = 0x02; // Re-Attach Not required;
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
  }
  mme_app_get_session_bearer_context_from_all(ue_session_pool, modify_bearer_resp_pP->bearer_contexts_modified.bearer_contexts[0].eps_bearer_id, &current_bearer_p);
  /** Get the first bearers PDN. */
  /** In this case, we will ignore the MBR and assume a detach process is ongoing. */
  if(!current_bearer_p){
    OAILOG_INFO(LOG_MME_APP, "We could not find the first bearer with eBI %d in the set of bearers. Ignoring the MBResp for UE with ueId: " MME_UE_S1AP_ID_FMT ". \n",
    		modify_bearer_resp_pP->bearer_contexts_modified.bearer_contexts[0].eps_bearer_id, ue_context->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
  }
  mme_app_get_pdn_context(ue_context->privates.mme_ue_s1ap_id, current_bearer_p->pdn_cx_id, current_bearer_p->linked_ebi, NULL, &pdn_context);
  /** If a too fast detach happens this is null. */
  DevAssert(pdn_context); /**< Should exist if bearers exist (todo: lock for this). */
  /** Set all bearers of the pdn context to valid. */
  /*
   * Remove any idles bearers, also in the case of (S10) S1 handover.
   * The PDN Connectivity element would always be more or equal than the actual number of established bearers.
   */
  bearer_context_new_t * bc_to_act = NULL;
  STAILQ_FOREACH (bc_to_act, &pdn_context->session_bearers, entries) {
    DevAssert(bc_to_act);
    // todo: should be in lock.
    if(bc_to_act->bearer_state & BEARER_STATE_ENB_CREATED)
      bc_to_act->bearer_state |= BEARER_STATE_ACTIVE;
  }
  // todo: set the downlink teid?
  /** No matter if there is an handover procedure or not, continue with the MBR for other PDNs. */
  pdn_context = NULL;
  RB_FOREACH (pdn_context, PdnContexts, &ue_session_pool->pdn_contexts) {
    DevAssert(pdn_context);
    bearer_context_new_t * first_bearer = STAILQ_FIRST(&pdn_context->session_bearers);
    DevAssert(first_bearer);
    // todo: here check, that it is not a deactivated bearer..
    if(first_bearer->bearer_state & BEARER_STATE_ACTIVE){
      /** Continue to next pdn. */
      continue;
    }else{
      if(first_bearer->bearer_state & BEARER_STATE_ENB_CREATED){
        /** Found a PDN. Establish the bearer contexts. */
        OAILOG_INFO(LOG_MME_APP, "Establishing the bearers for UE_CONTEXT for UE " MME_UE_S1AP_ID_FMT " triggered by handover notify (not active but ENB Created). \n", ue_context->privates.mme_ue_s1ap_id);
        /** Add the same flags back in. */
        mme_app_send_s11_modify_bearer_req(ue_session_pool, pdn_context, modify_bearer_resp_pP->internal_flags);
        OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
      }
    }
  }
  /** If it is an X2 Handover, send a path switch response back. */
  if(modify_bearer_resp_pP->internal_flags & INTERNAL_FLAG_X2_HANDOVER) {
    OAILOG_INFO(LOG_MME_APP, "Sending an S1AP Path Switch Request Acknowledge for UE with ueId: " MME_UE_S1AP_ID_FMT ". \n", ue_context->privates.mme_ue_s1ap_id);
    uint16_t encryption_algorithm_capabilities = 0;
    uint16_t integrity_algorithm_capabilities = 0;
    if(emm_data_context_update_security_parameters(ue_context->privates.mme_ue_s1ap_id, &encryption_algorithm_capabilities, &integrity_algorithm_capabilities) != RETURNok){
      OAILOG_ERROR(LOG_MME_APP, "Error updating AS security parameters for UE with ueId: " MME_UE_S1AP_ID_FMT ". \n", ue_context->privates.mme_ue_s1ap_id);
      mme_app_send_s1ap_path_switch_request_failure(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id,
          ue_context->privates.fields.sctp_assoc_id_key, S1ap_Cause_PR_nas);
      /** Implicitly detach the UE --> If EMM context is missing, still continue with the resource removal. */
      message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
      DevAssert (message_p != NULL);
      message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->privates.mme_ue_s1ap_id;
      MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
      itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
    OAILOG_INFO(LOG_MME_APP, "Successfully updated AS security parameters for UE with ueId: " MME_UE_S1AP_ID_FMT " for X2 handover. \n", ue_context->privates.mme_ue_s1ap_id);
    bearer_contexts_created_t  bcs_tbs;
    memset((void*)&bcs_tbs, 0, sizeof(bcs_tbs));
    pdn_context_t * registered_pdn_ctx = NULL;
    RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_session_pool->pdn_contexts) {
      DevAssert(registered_pdn_ctx);
      mme_app_get_bearer_contexts_to_be_created(registered_pdn_ctx, &bcs_tbs, BEARER_STATE_NULL);
      /** The number of bearers will be incremented in the method. S10 should just pick the ebi. */
    }
    mme_app_send_s1ap_path_switch_request_acknowledge(ue_context->privates.mme_ue_s1ap_id, encryption_algorithm_capabilities, integrity_algorithm_capabilities, &bcs_tbs);
  }
  /** If an S10 Handover procedure is ongoing, directly check for released bearers. */
  mme_app_s10_proc_mme_handover_t * s10_handover_procedure = mme_app_get_s10_procedure_mme_handover(ue_context);
  if(s10_handover_procedure){
    if(s10_handover_procedure->failed_ebi_list.num_ebi){
      pdn_context_t * registered_pdn_ctx = RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
      if(registered_pdn_ctx){
          mme_app_send_s11_delete_bearer_cmd(ue_context->privates.fields.mme_teid_s11, registered_pdn_ctx->s_gw_teid_s11_s4,
        		  &registered_pdn_ctx->s_gw_addr_s11_s4, &s10_handover_procedure->failed_ebi_list);
      }
    }
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
  }
  /** Nothing special to be done for S1 handover. Just trigger a Delete Bearer Command, if there are idle bearers. No need to check for per-pdn. */
  ebi_list_t ebi_list;
  memset(&ebi_list, 0, sizeof(ebi_list_t));
  RB_FOREACH (pdn_context, PdnContexts, &ue_session_pool->pdn_contexts) {
	STAILQ_FOREACH (current_bearer_p, &pdn_context->session_bearers, entries) {
      if((!(current_bearer_p->bearer_state & BEARER_STATE_ENB_CREATED)) && !current_bearer_p->enb_fteid_s1u.teid){
    	/** Check that it is not a default bearer. */
    	if(current_bearer_p->ebi == pdn_context->default_ebi){
    		  OAILOG_WARNING(LOG_MME_APP, "Not triggering a delete bearer request for non-established default bearer %d for UE: " MME_UE_S1AP_ID_FMT ".\n",
					  current_bearer_p->ebi, ue_context->privates.mme_ue_s1ap_id);
    		  continue;
    	}
    	/* If the ESM context is not active, assume an ESM/S11 dedicated bearer procedure is running, skip it, too. */
    	if(current_bearer_p->esm_ebr_context.status != ESM_EBR_ACTIVE){
    		  OAILOG_WARNING(LOG_MME_APP, "Not triggering a delete bearer request for non-established dedicated bearer %d for UE: " MME_UE_S1AP_ID_FMT ", since ESM status is not ACTIVE but %d.\n",
    				  current_bearer_p->ebi, ue_context->privates.mme_ue_s1ap_id, current_bearer_p->esm_ebr_context.status);
    		  continue;
    	}
        /** Trigger a Delete Bearer Command. */
        ebi_list.ebis[ebi_list.num_ebi] = current_bearer_p->ebi;
        ebi_list.num_ebi++;
      }
    }
  }
  if(!ebi_list.num_ebi){
    OAILOG_INFO(LOG_MME_APP, "No pending removal of bearers for ueId: " MME_UE_S1AP_ID_FMT ". Checking any pending bearer requests. \n", ue_context->privates.mme_ue_s1ap_id);
    mme_app_handle_bearer_ctx_retry(ue_context->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
  }
  /** If no S11 procedure is ongoing, remove the bearer. */
  mme_app_s11_proc_t * s11_procedure = mme_app_get_s11_procedure(ue_session_pool);
  if(s11_procedure) {
	  OAILOG_ERROR(LOG_MME_APP, "An S11 procedure is ongoing for the UE with ueId: " MME_UE_S1AP_ID_FMT " (pending bearer request). "
			  "But since we have bearers which could not be established, removing the procedure and rejecting the bearer modification. \n",
			  ue_context->privates.mme_ue_s1ap_id);
	  /**
	   * Reject & delete the bearer modification procedure.
	   * In multi-bearer handover, all kind of bearer requests could happen.
	   */
	  switch(s11_procedure->type) {
	  /**
	   * The PGW is expected to be in the final state, so we assume that unhandled messages won't trigger an implicit detach.
	   * No need to wait.
	   */
	  case MME_APP_S11_PROC_TYPE_CREATE_BEARER:
		  mme_app_send_s11_create_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11, ue_session_pool->privates.fields.saegw_teid_s11, REQUEST_REJECTED, s11_procedure->s11_trxn,
				  ((mme_app_s11_proc_create_bearer_t *)s11_procedure)->bcs_tbc);
		  break;
	  case MME_APP_S11_PROC_TYPE_UPDATE_BEARER:
		  mme_app_send_s11_update_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11, ue_session_pool->privates.fields.saegw_teid_s11, REQUEST_REJECTED, s11_procedure->s11_trxn,
				  ((mme_app_s11_proc_update_bearer_t*)s11_procedure)->bcs_tbu);
		  break;
	  case MME_APP_S11_PROC_TYPE_DELETE_BEARER:
		  mme_app_send_s11_delete_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11, ue_session_pool->privates.fields.saegw_teid_s11, REQUEST_REJECTED, s11_procedure->s11_trxn,
				  &((mme_app_s11_proc_delete_bearer_t*)s11_procedure)->ebis);
		  break;
	  default:
		  DevMessage("S11 Bearer Context type " + s11_procedure->type + " is unhandled for UE with mmeUeS1apId " + mme_ue_s1ap_id + " (MBR).\n");
	  }
  }
  OAILOG_INFO(LOG_MME_APP, "%d bearers pending for removal for ueId: " MME_UE_S1AP_ID_FMT ". Triggering a Delete Bearer Command. \n", ebi_list.num_ebi, ue_context->privates.mme_ue_s1ap_id);
  /** Trigger a Delete Bearer Command. */
  pdn_context_t * registered_pdn_ctx = RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
  if(registered_pdn_ctx){
	  mme_app_send_s11_delete_bearer_cmd(ue_session_pool->privates.fields.mme_teid_s11, registered_pdn_ctx->s_gw_teid_s11_s4,
			  &registered_pdn_ctx->s_gw_addr_s11_s4, &ebi_list);
  }
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
static
void mme_app_send_downlink_data_notification_acknowledge(gtpv2c_cause_value_t cause, teid_t saegw_s11_teid, teid_t local_s11_teid, struct sockaddr* peer_ip, void *trxn){
  OAILOG_FUNC_IN (LOG_MME_APP);

  /** Send a Downlink Data Notification Acknowledge with cause. */
  MessageDef * message_p = itti_alloc_new_message (TASK_MME_APP, S11_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE);
  DevAssert (message_p != NULL);

  itti_s11_downlink_data_notification_acknowledge_t *downlink_data_notification_ack_p = &message_p->ittiMsg.s11_downlink_data_notification_acknowledge;
  memset ((void*)downlink_data_notification_ack_p, 0, sizeof (itti_s11_downlink_data_notification_acknowledge_t));
  // todo: s10 TEID set every time?
  downlink_data_notification_ack_p->teid = saegw_s11_teid;
  downlink_data_notification_ack_p->local_teid = local_s11_teid;
  /** Get the first PDN context. */

  downlink_data_notification_ack_p->peer_ip = peer_ip;
  downlink_data_notification_ack_p->cause.cause_value = cause;
  downlink_data_notification_ack_p->trxn  = trxn;

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "MME_APP Sending S11 DOWNLINK_DATA_NOTIFICATION_ACK");

  /** Sending a message to S10. */
  itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
int
mme_app_handle_downlink_data_notification(const itti_s11_downlink_data_notification_t * const saegw_dl_data_ntf_pP){
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p = NULL;


  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (saegw_dl_data_ntf_pP );
  DevAssert (saegw_dl_data_ntf_pP->trxn);

  ue_context = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, saegw_dl_data_ntf_pP->teid);
  if (ue_context == NULL) {
    OAILOG_ERROR(LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", saegw_dl_data_ntf_pP->teid);
    /** Send a DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE. */
   	mme_app_send_downlink_data_notification_acknowledge(CONTEXT_NOT_FOUND, saegw_dl_data_ntf_pP->teid, INVALID_TEID, saegw_dl_data_ntf_pP->peer_ip, saegw_dl_data_ntf_pP->trxn);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /** Get the last known tac. */
  emm_data_context_t * emm_context = emm_data_context_get(&_emm_data, ue_context->privates.mme_ue_s1ap_id);
  if(!emm_context || emm_context->_tai_list.numberoflists == 0){
	  OAILOG_ERROR (LOG_MME_APP, "No EMM data context exists for UE_ID " MME_UE_S1AP_ID_FMT " or no tai list. \n", ue_context->privates.mme_ue_s1ap_id);
	  mme_app_send_downlink_data_notification_acknowledge(CONTEXT_NOT_FOUND, saegw_dl_data_ntf_pP->teid, INVALID_TEID, saegw_dl_data_ntf_pP->peer_ip, saegw_dl_data_ntf_pP->trxn);
	  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /** Check if the UE context is registered. */
  if (ue_context->privates.fields.mm_state != UE_REGISTERED) {
	OAILOG_ERROR(LOG_MME_APP, "UE " MME_UE_S1AP_ID_FMT" is not in registered state. Not initiating paging. \n", ue_context->privates.mme_ue_s1ap_id);
	/** Send a DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE. */
	mme_app_send_downlink_data_notification_acknowledge(CONTEXT_NOT_FOUND, saegw_dl_data_ntf_pP->teid, INVALID_TEID, saegw_dl_data_ntf_pP->peer_ip, saegw_dl_data_ntf_pP->trxn);
	OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /** Check that the UE is in idle mode!. */
  if (ECM_IDLE != ue_context->privates.fields.ecm_state) {
    OAILOG_ERROR (LOG_MME_APP, "UE_Context with IMSI " IMSI_64_FMT " and mmeUeS1apId: %d. \n is not in ECM_IDLE mode, instead %d. \n",
        ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.ecm_state);
    // todo: later.. check this more granularly
    mme_app_send_downlink_data_notification_acknowledge(UE_ALREADY_RE_ATTACHED, saegw_dl_data_ntf_pP->teid, ue_context->privates.fields.mme_teid_s11, saegw_dl_data_ntf_pP->peer_ip, saegw_dl_data_ntf_pP->trxn);
    /** Ignore this case for signaling.. wait for the MBR to be completed. */
   	OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
  }

  OAILOG_INFO(LOG_MME_APP, "MME_MOBILTY_COMPLETION timer is not running. Starting paging procedure for UE with imsi " IMSI_64_FMT ". \n", ue_context->privates.fields.imsi);
  // todo: timeout to wait to ignore further DL_DATA_NOTIF messages->
  mme_app_send_downlink_data_notification_acknowledge(REQUEST_ACCEPTED, saegw_dl_data_ntf_pP->teid, ue_context->privates.fields.mme_teid_s11, saegw_dl_data_ntf_pP->peer_ip, saegw_dl_data_ntf_pP->trxn);

  /** No need to start paging timeout timer. It will be handled by the Periodic TAU update timer. */
  // todo: no downlink data notification failure and just removing the UE?

  /** Do paging on S1AP interface. */
  message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_PAGING);
  DevAssert (message_p != NULL);
  itti_s1ap_paging_t *s1ap_paging_p = &message_p->ittiMsg.s1ap_paging;

  memset (s1ap_paging_p, 0, sizeof (itti_s1ap_paging_t));
  s1ap_paging_p->mme_ue_s1ap_id = ue_context->privates.mme_ue_s1ap_id; /**< Just MME_UE_S1AP_ID. */
  /** Send the latest SCTP. */
//  s1ap_paging_p->sctp_assoc_id_key = ue_context->privates.fields.sctp_assoc_id_key;
  s1ap_paging_p->tac = emm_context->originating_tai.tac;
  s1ap_paging_p->ue_identity_index = (uint16_t)((ue_context->privates.fields.imsi %1024) & 0xFFFF); /**< Just MME_UE_S1AP_ID. */
  s1ap_paging_p->tmsi = ue_context->privates.fields.guti.m_tmsi;
  OAILOG_INFO(LOG_MME_APP, "Calculated ue_identity index value for UE with imsi " IMSI_64_FMT " and ueId " MME_UE_S1AP_ID_FMT" is %d. \n", ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id, s1ap_paging_p->ue_identity_index);

  /** S1AP Paging. */
  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
int
mme_app_trigger_paging_due_signaling(const mme_ue_s1ap_id_t ue_id){
  struct ue_context_s                    *ue_context = NULL;
  MessageDef                             *message_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);

  ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if (ue_context == NULL) {
    OAILOG_ERROR(LOG_MME_APP, "We didn't find this UE " MME_UE_S1AP_ID_FMT" in list. \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /** Get the last known tac. */
  emm_data_context_t * emm_context = emm_data_context_get(&_emm_data, ue_id);
  if(!emm_context || emm_context->_tai_list.numberoflists == 0){
	  OAILOG_ERROR (LOG_MME_APP, "No EMM data context exists for UE_ID " MME_UE_S1AP_ID_FMT " or no tai list. \n", ue_context->privates.mme_ue_s1ap_id);
	  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /** Check if the UE context is registered. */
  if (ue_context->privates.fields.mm_state != UE_REGISTERED) {
	OAILOG_ERROR(LOG_MME_APP, "UE " MME_UE_S1AP_ID_FMT" is not in registered state. Not initiating paging. \n", ue_context->privates.mme_ue_s1ap_id);
	/** Send a DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE. */
	OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /** Check that the UE is in idle mode!. */
  if (ECM_IDLE != ue_context->privates.fields.ecm_state) {
    OAILOG_ERROR (LOG_MME_APP, "UE_Context with IMSI " IMSI_64_FMT " and mmeUeS1apId: %d. \n is not in ECM_IDLE mode, instead %d. \n",
        ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.ecm_state);
    // todo: later.. check this more granularly
    /** Ignore this case for signaling.. wait for the MBR to be completed. */
    OAILOG_WARNING(LOG_MME_APP, "UE " MME_UE_S1AP_ID_FMT" is not in idle state, but due signaling will initiate a new paging. \n", ue_context->privates.mme_ue_s1ap_id);
  }

  OAILOG_INFO(LOG_MME_APP, "MME_MOBILTY_COMPLETION timer is not running. Starting paging procedure for UE with imsi " IMSI_64_FMT ". \n", ue_context->privates.fields.imsi);
  // todo: timeout to wait to ignore further DL_DATA_NOTIF messages->

  /** No need to start paging timeout timer. It will be handled by the Periodic TAU update timer. */
  // todo: no downlink data notification failure and just removing the UE?

  /** Do paging on S1AP interface. */
  message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_PAGING);
  DevAssert (message_p != NULL);
  itti_s1ap_paging_t *s1ap_paging_p = &message_p->ittiMsg.s1ap_paging;

  memset (s1ap_paging_p, 0, sizeof (itti_s1ap_paging_t));
  s1ap_paging_p->mme_ue_s1ap_id = ue_context->privates.mme_ue_s1ap_id; /**< Just MME_UE_S1AP_ID. */
  /** Send the latest SCTP. */
//  s1ap_paging_p->sctp_assoc_id_key = ue_context->privates.fields.sctp_assoc_id_key;
  s1ap_paging_p->tac = emm_context->originating_tai.tac;
  s1ap_paging_p->ue_identity_index = (uint16_t)((ue_context->privates.fields.imsi %1024) & 0xFFFF); /**< Just MME_UE_S1AP_ID. */
  s1ap_paging_p->tmsi = ue_context->privates.fields.guti.m_tmsi;
  OAILOG_INFO(LOG_MME_APP, "Calculated ue_identity index value for UE with imsi " IMSI_64_FMT " and ueId " MME_UE_S1AP_ID_FMT" is %d. \n", ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id, s1ap_paging_p->ue_identity_index);

  /** S1AP Paging. */
  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
void
mme_app_handle_initial_context_setup_rsp (
  itti_mme_app_initial_context_setup_rsp_t * const initial_ctxt_setup_rsp_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                 *ue_context = NULL;
  struct ue_session_pool_s            *ue_session_pool = NULL;
  MessageDef                          *message_p = NULL;
  ebi_list_t                           ebi_list;

  OAILOG_DEBUG (LOG_MME_APP, "Received MME_APP_INITIAL_CONTEXT_SETUP_RSP from S1AP\n");
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, initial_ctxt_setup_rsp_pP->ue_id);

  if (ue_context == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", initial_ctxt_setup_rsp_pP->ue_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " MME_APP_INITIAL_CONTEXT_SETUP_RSP Unknown ue %u", initial_ctxt_setup_rsp_pP->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  // Stop Initial context setup process guard timer,if running
  if (ue_context->privates.initial_context_setup_rsp_timer.id != MME_APP_TIMER_INACTIVE_ID) {
    if (timer_remove(ue_context->privates.initial_context_setup_rsp_timer.id, NULL)) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to stop Initial Context Setup Rsp timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
    }
    ue_context->privates.initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  }

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, initial_ctxt_setup_rsp_pP->ue_id);
  if (ue_session_pool == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find the UE session pool of UE: " MME_UE_S1AP_ID_FMT "\n", initial_ctxt_setup_rsp_pP->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

//  pdn_context_t * registered_pdn_ctx = NULL;
//  /** Update all bearers and get the pdn context id. */
//  RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_context->pdn_contexts) {
//    DevAssert(registered_pdn_ctx);
//
//    /**
//     * Get the first PDN whose bearers are not established yet.
//     * Do the MBR just one PDN at a time.
//     */
//    bearer_context_t * bearer_context_to_establish = NULL;
//    RB_FOREACH (bearer_context_to_establish, SessionBearers, &registered_pdn_ctx->session_bearers) {
//      DevAssert(bearer_context_to_establish);
//      /** Add them to the bearears list of the MBR. */
//      if (bearer_context_to_establish->ebi == initial_ctxt_setup_rsp_pP->e_rab_id[0]){
//        goto found_pdn;
//      }
//    }
//    registered_pdn_ctx = NULL;
//  }
//  OAILOG_INFO(LOG_MME_APP, "No PDN context found with unestablished bearers for mmeUeS1apId " MME_UE_S1AP_ID_FMT ". Dropping Initial Context Setup Response. \n", ue_context->privates.mme_ue_s1ap_id);
//  OAILOG_FUNC_OUT (LOG_MME_APP);
//
//found_pdn:
////  /** Save the bearer information as pending or send it directly if UE is registered. */
////  RB_FOREACH (bearer_context_to_establish, SessionBearers, &registered_pdn_ctx->session_bearers) {
////    DevAssert(bearer_context_to_establish);
////    /** Add them to the bearears list of the MBR. */
////    if (bearer_context_to_establish->ebi == initial_ctxt_setup_rsp_pP->e_rab_id[0]){
////      goto found_pdn;
////    }
////  }
  /** Process the failed bearers (for all APNs). */
  memset(&ebi_list, 0, sizeof(ebi_list_t));
  mme_app_release_bearers(initial_ctxt_setup_rsp_pP->ue_id, &initial_ctxt_setup_rsp_pP->e_rab_release_list, &ebi_list);

  if(mme_app_modify_bearers(initial_ctxt_setup_rsp_pP->ue_id, &initial_ctxt_setup_rsp_pP->bcs_to_be_modified) != RETURNok){
    OAILOG_ERROR (LOG_MME_APP, "Error while initial context setup response handling for UE: " MME_UE_S1AP_ID_FMT "\n", initial_ctxt_setup_rsp_pP->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  pdn_context_t * registered_pdn_ctx = RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
  if(!registered_pdn_ctx){
	  OAILOG_ERROR (LOG_MME_APP, "Error while initial context setup response handling for UE: " MME_UE_S1AP_ID_FMT ". No PDN context could be found. \n", initial_ctxt_setup_rsp_pP->ue_id);
	  OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  uint8_t flags = 0;
  mme_app_send_s11_modify_bearer_req(ue_session_pool, registered_pdn_ctx, flags);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_release_access_bearers_resp (
  const itti_s11_release_access_bearers_response_t * const rel_access_bearers_rsp_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);

  struct ue_context_s * ue_context = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, rel_access_bearers_rsp_pP->teid);
  if (ue_context == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %" PRIX32 "\n", rel_access_bearers_rsp_pP->teid);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 RELEASE_ACCESS_BEARERS_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ", rel_access_bearers_rsp_pP->teid, ue_context->emm_context._imsi64);

  S1ap_Cause_t            s1_ue_context_release_cause = {0};
  s1_ue_context_release_cause.present = S1ap_Cause_PR_radioNetwork;
  s1_ue_context_release_cause.choice.radioNetwork = S1ap_CauseRadioNetwork_release_due_to_eutran_generated_reason;

  // Send UE Context Release Command
  if(ue_context->privates.s1_ue_context_release_cause == S1AP_INVALID_CAUSE)
    ue_context->privates.s1_ue_context_release_cause = S1AP_NAS_NORMAL_RELEASE;
  mme_app_itti_ue_context_release(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.s1_ue_context_release_cause, ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id);
  if (ue_context->privates.s1_ue_context_release_cause == S1AP_SCTP_SHUTDOWN_OR_RESET) {
    // Just cleanup the MME APP state associated with s1.
    mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_s11_create_bearer_req (
    itti_s11_create_bearer_request_t *  create_bearer_request_pP)
{
  MessageDef                               *message_p   	= NULL;
  struct ue_session_pool_s                 *ue_session_pool = NULL;
  struct pdn_context_s                     *pdn_context = NULL;
  bearer_context_new_t                     *default_bc  = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);

  ue_session_pool = mme_ue_session_pool_exists_s11_teid(&mme_app_desc.mme_ue_session_pools, create_bearer_request_pP->teid);
  if (!ue_session_pool) {
	  OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %" PRIX32 "\n", create_bearer_request_pP->teid);
	  OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  mme_app_get_session_bearer_context_from_all(ue_session_pool, create_bearer_request_pP->linked_eps_bearer_id, &default_bc);
  if(!default_bc){
    OAILOG_ERROR(LOG_MME_APP, "Default ebi %d not found in list of UE: %" PRIX32 "\n", create_bearer_request_pP->linked_eps_bearer_id, create_bearer_request_pP->teid);
    mme_app_send_s11_create_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11, ue_session_pool->privates.fields.saegw_teid_s11, REQUEST_REJECTED, (uintptr_t)create_bearer_request_pP->trxn, create_bearer_request_pP->bearer_contexts);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  bearer_context_new_t * bc_ded = NULL;
  /** Check that the remaining bearers are not existing. */
  for(int num_bc = 0; num_bc < create_bearer_request_pP->bearer_contexts->num_bearer_context ; num_bc++){
    mme_app_get_session_bearer_context_from_all(ue_session_pool, create_bearer_request_pP->bearer_contexts->bearer_contexts[num_bc].eps_bearer_id, &bc_ded);
    if(bc_ded){
      OAILOG_ERROR(LOG_MME_APP, "Ded ebi %d already existing in list of UE with ueId " MME_UE_S1AP_ID_FMT" \n",
          create_bearer_request_pP->bearer_contexts->bearer_contexts[num_bc].eps_bearer_id, ue_session_pool->privates.mme_ue_s1ap_id);
      mme_app_send_s11_create_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11, ue_session_pool->privates.fields.saegw_teid_s11, REQUEST_REJECTED, (uintptr_t)create_bearer_request_pP->trxn, create_bearer_request_pP->bearer_contexts);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
  }

  /** Create an S11 procedure. */
  mme_app_s11_proc_create_bearer_t* s11_proc_create_bearer = mme_app_create_s11_procedure_create_bearer(ue_session_pool);
  if(!s11_proc_create_bearer){
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_BEARER_REQUEST local S11 teid " TEID_FMT " ",
        create_bearer_request_pP->teid);
    OAILOG_ERROR(LOG_MME_APP, "No CBR procedure could be created for of UE with ueId " MME_UE_S1AP_ID_FMT ". Ignoring request.. might be a duplicate. \n", ue_session_pool->privates.mme_ue_s1ap_id);
    // mme_app_send_s11_create_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11, ue_session_pool->privates.fields.saegw_teid_s11, TEMP_REJECT_HO_IN_PROGRESS, (uintptr_t)create_bearer_request_pP->trxn, create_bearer_request_pP->bearer_contexts);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  s11_proc_create_bearer->proc.s11_trxn         = (uintptr_t)create_bearer_request_pP->trxn;
  s11_proc_create_bearer->proc.num_bearers_unhandled = create_bearer_request_pP->bearer_contexts->num_bearer_context;
  s11_proc_create_bearer->linked_ebi            = default_bc->linked_ebi;
  s11_proc_create_bearer->pci                   = default_bc->pdn_cx_id;
  s11_proc_create_bearer->proc.pti              = create_bearer_request_pP->pti;
  s11_proc_create_bearer->bcs_tbc               = create_bearer_request_pP->bearer_contexts;
  create_bearer_request_pP->bearer_contexts	    = NULL;

  /**
   * Wait until the MBR of the default bearers has been accomplished.
   * The PDN contexts should exist already.
   */
  pdn_context_t * registered_pdn_ctx = NULL;
  RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_session_pool->pdn_contexts) {
	  DevAssert(registered_pdn_ctx);
	  /** The number of bearers will be incremented in the method. S10 should just pick the ebi. */
	  bearer_context_new_t * bc_session = mme_app_get_session_bearer_context(registered_pdn_ctx, registered_pdn_ctx->default_ebi);
	  if(!(bc_session->bearer_state & BEARER_STATE_ACTIVE)) {
		  OAILOG_WARNING(LOG_MME_APP, "Bearer %d for UE " MME_UE_S1AP_ID_FMT" is in state %d. Not processing dedicated bearer requests until "
				  "session establishment has been completed successfully. \n", bc_session->ebi, ue_session_pool->privates.mme_ue_s1ap_id, bc_session->bearer_state);
		  /** Trigger paging, if the UE context is REGISTERED. */
		  if(mme_app_trigger_paging_due_signaling(ue_session_pool->privates.mme_ue_s1ap_id) == RETURNok){
			  OAILOG_INFO(LOG_MME_APP, "Successfully triggered paging for UE " MME_UE_S1AP_ID_FMT" due CBR. Not processing dedicated bearer requests until "
					  "session establishment has been completed successfully. \n", ue_session_pool->privates.mme_ue_s1ap_id);
			  OAILOG_FUNC_OUT (LOG_MME_APP);
		  } else {
			  OAILOG_WARNING(LOG_MME_APP, "Could not trigger paging for UE " MME_UE_S1AP_ID_FMT" due CBR. Processing dedicated bearer requests (may need to set active-flag for idle-TAU). \n",
					  ue_session_pool->privates.mme_ue_s1ap_id);
			  break;
		  }
		  OAILOG_FUNC_OUT (LOG_MME_APP);
	  }
  }

  /*
   * Let the ESM layer validate the request and build the pending bearer contexts.
   * Also, send a single message to the eNB.
   * May received multiple back.
   */
  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_ACTIVATE_EPS_BEARER_CTX_REQ);
  AssertFatal (message_p , "itti_alloc_new_message Failed");

  itti_nas_activate_eps_bearer_ctx_req_t *nas_activate_eps_bearer_ctx_req = &message_p->ittiMsg.nas_activate_eps_bearer_ctx_req;
  nas_activate_eps_bearer_ctx_req->bcs_to_be_created_ptr = (uintptr_t)s11_proc_create_bearer->bcs_tbc;
  /** MME_APP Create Bearer Request. */
  nas_activate_eps_bearer_ctx_req->ue_id              = ue_session_pool->privates.mme_ue_s1ap_id;
  nas_activate_eps_bearer_ctx_req->linked_ebi         = create_bearer_request_pP->linked_eps_bearer_id;
  /** Copy the BC to be created. */
  /** Might be UE triggered. */
  nas_activate_eps_bearer_ctx_req->pti                = create_bearer_request_pP->pti;
  nas_activate_eps_bearer_ctx_req->cid                = default_bc->pdn_cx_id;

  /** Set it to NULL, such that it is not deallocated. */
  create_bearer_request_pP->bearer_contexts     = NULL;

  /** No need to set bearer states, we won't establish the bearers yet. */
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_ACTIVATE_EPS_BEARER_CTX_REQ mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ",
      nas_activate_eps_bearer_ctx_req->ue_id);
  itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_s11_update_bearer_req (
    itti_s11_update_bearer_request_t *  update_bearer_request_pP)
{
  MessageDef                               *message_p  	 	= NULL;
  struct ue_session_pool_s                 *ue_session_pool = NULL;
  emm_data_context_t 			           *emm_context 	= NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);

  ue_session_pool = mme_ue_session_pool_exists_s11_teid(&mme_app_desc.mme_ue_session_pools, update_bearer_request_pP->teid);
  if (!ue_session_pool) {
	OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %" PRIX32 "\n", update_bearer_request_pP->teid);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  ebi_t linked_ebi = 0;
  pdn_cid_t cid = 0;
  /** No default EBI will be sent. Need to check all dedicated EBIs. */
  for(int num_bearer = 0; num_bearer < update_bearer_request_pP->bearer_contexts->num_bearer_context; num_bearer++){
	bearer_context_new_t * ded_bc = NULL;
    mme_app_get_session_bearer_context_from_all(ue_session_pool, update_bearer_request_pP->bearer_contexts->bearer_contexts[num_bearer].eps_bearer_id, &ded_bc);
    if(!ded_bc || ded_bc->esm_ebr_context.status != ESM_EBR_ACTIVE){ /**< Status is active if it is idle TAU. */
      MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 UPDATE_BEARER_REQUEST local S11 teid " TEID_FMT " ",
          update_bearer_request_pP->teid);
      OAILOG_ERROR(LOG_MME_APP, "We could not find an (ACTIVE) dedicated bearer for ebi %d in bearer list of UE: %" MME_UE_S1AP_ID_FMT". \n",
          update_bearer_request_pP->bearer_contexts->bearer_contexts[num_bearer].eps_bearer_id, ue_session_pool->privates.mme_ue_s1ap_id);
      mme_app_send_s11_update_bearer_rsp(ue_session_pool->privates.mme_ue_s1ap_id, ue_session_pool->privates.fields.saegw_teid_s11, 0, (uintptr_t)update_bearer_request_pP->trxn, update_bearer_request_pP->bearer_contexts);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
    linked_ebi = ded_bc->linked_ebi;
    cid = ded_bc->pdn_cx_id;
    DevAssert(linked_ebi);
  }

  /**
   * Check the received new APN-AMBR, if exists.
   * If the subscribed UE-APN-AMBR is exceeded, we reject the update request directly.
   * todo: also need to check each apn with the subscribed APN-AMBR..
   */
  ambr_t new_total_apn_ambr = mme_app_total_p_gw_apn_ambr_rest(ue_session_pool, cid);
  if((new_total_apn_ambr.br_dl += update_bearer_request_pP->apn_ambr.br_dl)  > ue_session_pool->privates.fields.subscribed_ue_ambr.br_dl){
    OAILOG_ERROR(LOG_MME_APP, "New total APN-AMBR exceeds the subscribed APN-AMBR (DL) for ueId " MME_UE_S1AP_ID_FMT". \n", ue_session_pool->privates.mme_ue_s1ap_id);
    mme_app_send_s11_update_bearer_rsp(update_bearer_request_pP->teid, ue_session_pool->privates.fields.saegw_teid_s11, 0, (uintptr_t)update_bearer_request_pP->trxn, update_bearer_request_pP->bearer_contexts);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  if((new_total_apn_ambr.br_ul += update_bearer_request_pP->apn_ambr.br_ul)  > ue_session_pool->privates.fields.subscribed_ue_ambr.br_ul){
    OAILOG_ERROR(LOG_MME_APP, "New total APN-AMBR exceeds the subscribed APN-AMBR (UL) for ueId " MME_UE_S1AP_ID_FMT". \n", ue_session_pool->privates.mme_ue_s1ap_id);
    mme_app_send_s11_update_bearer_rsp(update_bearer_request_pP->teid, ue_session_pool->privates.fields.saegw_teid_s11, 0, (uintptr_t)update_bearer_request_pP->trxn, update_bearer_request_pP->bearer_contexts);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** Create an S11 procedure for the UBR. */
  mme_app_s11_proc_update_bearer_t* s11_proc_update_bearer = mme_app_create_s11_procedure_update_bearer(ue_session_pool);
  if(!s11_proc_update_bearer){
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 UPDATE_BEARER_REQUEST local S11 teid " TEID_FMT " ",
          delete_bearer_request_pP->teid);
    OAILOG_ERROR(LOG_MME_APP, "We could not create an UBR procedure for UE with ueId " MME_UE_S1AP_ID_FMT". \n",ue_session_pool->privates.mme_ue_s1ap_id);
    // todo: ignore currently.. mme_app_send_s11_update_bearer_rsp(update_bearer_request_pP->teid, ue_session_pool->privates.fields.saegw_teid_s11, TEMP_REJECT_HO_IN_PROGRESS, (uintptr_t)update_bearer_request_pP->trxn, update_bearer_request_pP->bearer_contexts);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  s11_proc_update_bearer->proc.s11_trxn         = (uintptr_t)update_bearer_request_pP->trxn;
  s11_proc_update_bearer->proc.num_bearers_unhandled = update_bearer_request_pP->bearer_contexts->num_bearer_context;
  s11_proc_update_bearer->bcs_tbu               = update_bearer_request_pP->bearer_contexts;
  s11_proc_update_bearer->pci                   = cid;
  s11_proc_update_bearer->new_used_ue_ambr      = new_total_apn_ambr; /**< Use this (actualized) value in the E-RAB Modify Request. */
  s11_proc_update_bearer->apn_ambr              = update_bearer_request_pP->apn_ambr;
  s11_proc_update_bearer->proc.pti              = update_bearer_request_pP->pti;
  s11_proc_update_bearer->linked_ebi            = linked_ebi;
  update_bearer_request_pP->bearer_contexts     = NULL;

  // todo: PCOs
  /**
   * Wait until the MBR of the default bearers has been accomplished.
   * The PDN contexts should exist already.
   */
  pdn_context_t * registered_pdn_ctx = NULL;
  RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_session_pool->pdn_contexts) {
	  DevAssert(registered_pdn_ctx);
	  /** The number of bearers will be incremented in the method. S10 should just pick the ebi. */
	  bearer_context_new_t * bc_session = mme_app_get_session_bearer_context(registered_pdn_ctx, registered_pdn_ctx->default_ebi);
	  if(!(bc_session->bearer_state & BEARER_STATE_ACTIVE)) {
		  OAILOG_WARNING(LOG_MME_APP, "Bearer %d for UE " MME_UE_S1AP_ID_FMT" is in state %d. Not processing dedicated bearer requests until "
				  "session establishment has been completed successfully. \n", bc_session->ebi, ue_session_pool->privates.mme_ue_s1ap_id, bc_session->bearer_state);
		  /** Trigger paging, if the UE context is REGISTERED. */
		  if(mme_app_trigger_paging_due_signaling(ue_session_pool->privates.mme_ue_s1ap_id) == RETURNok){
			  OAILOG_INFO(LOG_MME_APP, "Successfully triggered paging for UE " MME_UE_S1AP_ID_FMT" due UBR. Not processing dedicated bearer requests until "
					  "session establishment has been completed successfully. \n", ue_session_pool->privates.mme_ue_s1ap_id);
			  OAILOG_FUNC_OUT (LOG_MME_APP);
		  } else {
			  OAILOG_WARNING(LOG_MME_APP, "Could not trigger paging for UE " MME_UE_S1AP_ID_FMT" due UBR. Processing dedicated bearer requests (may need to set active-flag for idle-TAU). \n",
					  ue_session_pool->privates.mme_ue_s1ap_id);
			  break;
		  }
	  }
  }

  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_MODIFY_EPS_BEARER_CTX_REQ);
  AssertFatal (message_p , "itti_alloc_new_message Failed");

  itti_nas_modify_eps_bearer_ctx_req_t *nas_modify_eps_bearer_ctx_req = &message_p->ittiMsg.nas_modify_eps_bearer_ctx_req;
  nas_modify_eps_bearer_ctx_req->bcs_to_be_updated_ptr = (uintptr_t)s11_proc_update_bearer->bcs_tbu;
  /** NAS Update Bearer Request. The ESM layer will also check the APN-AMBR. */
  nas_modify_eps_bearer_ctx_req->ue_id              = ue_session_pool->privates.mme_ue_s1ap_id;
  /** Might be UE triggered. */
  nas_modify_eps_bearer_ctx_req->pti                = update_bearer_request_pP->pti;
  nas_modify_eps_bearer_ctx_req->apn_ambr           = update_bearer_request_pP->apn_ambr;
  nas_modify_eps_bearer_ctx_req->linked_ebi         = s11_proc_update_bearer->linked_ebi;
  nas_modify_eps_bearer_ctx_req->cid                = s11_proc_update_bearer->pci;

  /** Set it to NULL, such that it is not deallocated. */
  update_bearer_request_pP->bearer_contexts     	= NULL;

  /** No need to set bearer states, we won't establish the bearers yet. */
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_MODIFY_EPS_BEARER_CTX_REQ mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ",  nas_modify_eps_bearer_ctx_req->ue_id);
  itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_s11_delete_bearer_req (
    itti_s11_delete_bearer_request_t *  delete_bearer_request_pP)
{
  MessageDef                               *message_p   	= NULL;
  struct ue_session_pool_s                 *ue_session_pool = NULL;
  struct pdn_context_s                     *pdn_context 	= NULL;
  emm_data_context_t					   *emm_context 	= NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);

  ue_session_pool = mme_ue_session_pool_exists_s11_teid(&mme_app_desc.mme_ue_session_pools, delete_bearer_request_pP->teid);
  if (!ue_session_pool) {
   OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %" PRIX32 "\n", delete_bearer_request_pP->teid);
   OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** Check if the linked ebi is existing. */
  if(delete_bearer_request_pP->linked_eps_bearer_id){
    OAILOG_ERROR(LOG_MME_APP, "Default bearer deactivation via Delete Bearer Request not implemented yet for UE with ueId " MME_UE_S1AP_ID_FMT". \n", ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** Create an S11 procedure. */
  mme_app_s11_proc_delete_bearer_t* s11_proc_delete_bearer = mme_app_create_s11_procedure_delete_bearer(ue_session_pool);
  if(!s11_proc_delete_bearer){
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 DELETE_BEARER_REQUEST local S11 teid " TEID_FMT " ",
          delete_bearer_request_pP->teid);
    OAILOG_ERROR(LOG_MME_APP, "We could not create a DBR procedure for UE with ueId " MME_UE_S1AP_ID_FMT". \n", ue_session_pool->privates.mme_ue_s1ap_id);
    // todo: mme_app_send_s11_delete_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11, ue_session_pool->privates.fields.saegw_teid_s11, TEMP_REJECT_HO_IN_PROGRESS, (uintptr_t)delete_bearer_request_pP->trxn, &delete_bearer_request_pP->ebi_list);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** Respond success if bearers are not existing. */
  s11_proc_delete_bearer->proc.s11_trxn         = (uintptr_t)delete_bearer_request_pP->trxn;
  s11_proc_delete_bearer->proc.num_bearers_unhandled = delete_bearer_request_pP->ebi_list.num_ebi;
  s11_proc_delete_bearer->linked_ebi            = delete_bearer_request_pP->linked_eps_bearer_id;
  s11_proc_delete_bearer->proc.pti 				= delete_bearer_request_pP->pti;

//  s11_proc_delete_bearer->linked_eps_bearer_id  = delete_bearer_request_pP->linked_eps_bearer_id; /**< Only if it is in the request. */
  memcpy(&s11_proc_delete_bearer->ebis, &delete_bearer_request_pP->ebi_list, sizeof(delete_bearer_request_pP->ebi_list));
  // todo: failed bearer contexts not handled yet (failed from those in DBC)
//  memcpy(&s11_proc_delete_bearer->bcs_failed, &delete_bearer_request_pP->to_be_removed_bearer_contexts, sizeof(delete_bearer_request_pP->to_be_removed_bearer_contexts));

  // todo: PCOs
  /**
   * Wait until the MBR of the default bearers has been accomplished.
   * The PDN contexts should exist already.
   */
  pdn_context_t * registered_pdn_ctx = NULL;
  RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_session_pool->pdn_contexts) {
	  DevAssert(registered_pdn_ctx);
	  /** The number of bearers will be incremented in the method. S10 should just pick the ebi. */
	  bearer_context_new_t * bc_session = mme_app_get_session_bearer_context(registered_pdn_ctx, registered_pdn_ctx->default_ebi);
	  if(!(bc_session->bearer_state & BEARER_STATE_ACTIVE)) {
		  OAILOG_WARNING(LOG_MME_APP, "Bearer %d for UE " MME_UE_S1AP_ID_FMT" is in state %d. Not processing dedicated bearer requests until "
				  "session establishment has been completed successfully. \n", bc_session->ebi, ue_session_pool->privates.mme_ue_s1ap_id, bc_session->bearer_state);
		  if(mme_app_trigger_paging_due_signaling(ue_session_pool->privates.mme_ue_s1ap_id) == RETURNok){
			  OAILOG_INFO(LOG_MME_APP, "Successfully triggered paging for UE " MME_UE_S1AP_ID_FMT" due DBR. Not processing dedicated bearer requests until "
					  "session establishment has been completed successfully. \n", ue_session_pool->privates.mme_ue_s1ap_id);
			  OAILOG_FUNC_OUT (LOG_MME_APP);
		  } else {
			  OAILOG_WARNING(LOG_MME_APP, "Could not trigger paging for UE " MME_UE_S1AP_ID_FMT" due DBR. Processing dedicated bearer requests (may need to set active-flag for idle-TAU). \n",
					  ue_session_pool->privates.mme_ue_s1ap_id);
			  break;
		  }
		  OAILOG_FUNC_OUT (LOG_MME_APP);
	  }
  }
  /*
   * Let the ESM layer validate the request and deactivate the bearers.
   */
  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_DEACTIVATE_EPS_BEARER_CTX_REQ);
  AssertFatal (message_p , "itti_alloc_new_message Failed");

  itti_nas_deactivate_eps_bearer_ctx_req_t *nas_deactivate_eps_bearer_ctx_req = &message_p->ittiMsg.nas_deactivate_eps_bearer_ctx_req;
  nas_deactivate_eps_bearer_ctx_req->ue_id              = ue_session_pool->privates.mme_ue_s1ap_id;
  nas_deactivate_eps_bearer_ctx_req->def_ebi            = delete_bearer_request_pP->linked_eps_bearer_id;
  memcpy(&nas_deactivate_eps_bearer_ctx_req->ebis, &delete_bearer_request_pP->ebi_list, sizeof(delete_bearer_request_pP->ebi_list));
  /** Might be UE triggered. */
  nas_deactivate_eps_bearer_ctx_req->pti                = delete_bearer_request_pP->pti;
  nas_deactivate_eps_bearer_ctx_req->def_ebi            = s11_proc_delete_bearer->linked_ebi;

  /** Set it to NULL, such that it is not deallocated. */

  /** No need to set bearer states, we won't remove the bearers yet. */
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_DEACTIVATE_EPS_BEARER_CTX_REQ mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " ",
      nas_deactivate_eps_bearer_ctx_req->ue_id);
  itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_delete_bearer_failure_indication (itti_s11_delete_bearer_failure_indication_t  * const delete_bearer_failure_ind){
  MessageDef                               *message_p   = NULL;
  struct ue_session_pool_s                 *ue_session_pool = NULL;
  struct pdn_context_s                     *pdn_context = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);

  ue_session_pool = mme_ue_session_pool_exists_s11_teid( &mme_app_desc.mme_ue_session_pools, delete_bearer_failure_ind->teid);

  if (ue_session_pool == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 DELETE_BEARER_REQUEST local S11 teid " TEID_FMT " ",
        delete_bearer_failure_ind->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %" PRIX32 "\n", delete_bearer_failure_ind->teid);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** Check the cause is context not found, perform an implicit detach. */
  if(delete_bearer_failure_ind->cause.cause_value == CONTEXT_NOT_FOUND) {
    OAILOG_DEBUG (LOG_MME_APP, "Received cause \"Context not found\" for ue_id " MME_UE_S1AP_ID_FMT" (Delete Bearer Failure Indication)."
        "  Implicitly removing the UE context. \n", ue_session_pool->privates.mme_ue_s1ap_id);
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_session_pool->privates.mme_ue_s1ap_id;
    message_p->ittiMsg.nas_implicit_detach_ue_ind.emm_cause = EMM_CAUSE_NETWORK_FAILURE;
    message_p->ittiMsg.nas_implicit_detach_ue_ind.detach_type = 0x02; // Re-Attach Not required;
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  OAILOG_DEBUG (LOG_MME_APP, "Implicitly removing the idle bearer of UE context for ue_id " MME_UE_S1AP_ID_FMT " for received error cause %d (DBFI). \n",
		  ue_session_pool->privates.mme_ue_s1ap_id, delete_bearer_failure_ind->cause.cause_value);
  /** Check if the ebis, failed to be released are existing. Locally remove them. */
  for(ebi_t num_ebi = 0; num_ebi < delete_bearer_failure_ind->bcs_failed.num_bearer_context; num_ebi++) {
    mme_app_release_bearer_context(ue_session_pool->privates.mme_ue_s1ap_id, NULL, EPS_BEARER_IDENTITY_UNASSIGNED,
        delete_bearer_failure_ind->bcs_failed.bearer_contexts[num_ebi].eps_bearer_id);
    /** No procedures should exist. */
  }

  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_handle_e_rab_setup_rsp (itti_s1ap_e_rab_setup_rsp_t  * const e_rab_setup_rsp)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s            *ue_session_pool = NULL;

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, e_rab_setup_rsp->mme_ue_s1ap_id);
  if (ue_session_pool == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", e_rab_setup_rsp->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " S1AP_E_RAB_SETUP_RSP Unknown ue " MME_UE_S1AP_ID_FMT "\n", e_rab_setup_rsp->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  // todo: here registered flag of ue session pool
//  if(ue_context->privates.fields.mm_state != UE_REGISTERED){
//    OAILOG_WARNING(LOG_MME_APP, "Ignoring received E-RAB Setup response for UNREGISTERED UE: " MME_UE_S1AP_ID_FMT "\n", e_rab_setup_rsp->mme_ue_s1ap_id);
//    OAILOG_FUNC_OUT (LOG_MME_APP);
//  }

  mme_app_s11_proc_create_bearer_t * s11_proc_create_bearer = mme_app_get_s11_procedure_create_bearer(ue_session_pool);
  if(!s11_proc_create_bearer){
    OAILOG_DEBUG( LOG_MME_APP, "No S11 Create Bearer process for UE with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " was found. \n", e_rab_setup_rsp->mme_ue_s1ap_id);
    /** Multi APN or failed dedicated bearer (NAS Rejects came in): get the ebi and check if it is a default ebi. */
    if(e_rab_setup_rsp->e_rab_setup_list.no_of_items + e_rab_setup_rsp->e_rab_failed_to_setup_list.no_of_items == 1){
      /** Received a single bearer, check if it is the default ebi. */
      if(e_rab_setup_rsp->e_rab_setup_list.no_of_items){
        ebi_t ebi_success = e_rab_setup_rsp->e_rab_setup_list.item[0].e_rab_id;
        bearer_context_new_t * bc_success = NULL;
        mme_app_get_session_bearer_context_from_all(ue_session_pool, ebi_success, &bc_success);
        /** Check if it is a default ebi. */
        if(bc_success && bc_success->linked_ebi == bc_success->ebi){
          /** Returned a response for a successful bearer establishment for a pdn creation. */
          // todo: handle Multi apn success
          mme_app_handle_e_rab_setup_rsp_pdn_connectivity(e_rab_setup_rsp->mme_ue_s1ap_id, &e_rab_setup_rsp->e_rab_setup_list.item[0], 0);
          OAILOG_FUNC_OUT(LOG_MME_APP);
        }
      }else{
        ebi_t ebi_failed = e_rab_setup_rsp->e_rab_failed_to_setup_list.item[0].e_rab_id;
        bearer_context_new_t * bc_failed = NULL;
        mme_app_get_session_bearer_context_from_all(ue_session_pool, ebi_failed, &bc_failed);
        if(bc_failed && bc_failed->linked_ebi == bc_failed->ebi){
          /** Returned a response for a failed bearer establishment for a pdn creation. */
          // todo: handle Multi apn failure
          mme_app_handle_e_rab_setup_rsp_pdn_connectivity(e_rab_setup_rsp->mme_ue_s1ap_id, NULL, ebi_failed);
          OAILOG_FUNC_OUT(LOG_MME_APP);
        }
      }
    }
    OAILOG_ERROR( LOG_MME_APP, "Received E-RAB response for multiple/non-default bearers, though no attach, multi-apn, or dedicated procedure "
        "is running for UE with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ". Ignoring received message (Ignoring NAS failed completely before S1AP reply). \n", e_rab_setup_rsp->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }else {
    /** Handle E-RAB Setup Response for the dedicated bearer case. */
    mme_app_handle_e_rab_setup_rsp_dedicated_bearer(e_rab_setup_rsp);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
}

//------------------------------------------------------------------------------
static void mme_app_handle_e_rab_setup_rsp_dedicated_bearer(const itti_s1ap_e_rab_setup_rsp_t * e_rab_setup_rsp)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s               *ue_session_pool = NULL;
  struct pdn_context_s                   *pdn_context 	  = NULL;

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_session_pools, e_rab_setup_rsp->mme_ue_s1ap_id);
  if (ue_session_pool == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", e_rab_setup_rsp->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " S1AP_E_RAB_SETUP_RSP Unknown ue " MME_UE_S1AP_ID_FMT "\n", e_rab_setup_rsp->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  mme_app_s11_proc_create_bearer_t * s11_proc_create_bearer = mme_app_get_s11_procedure_create_bearer(ue_session_pool);
  if(!s11_proc_create_bearer){
    OAILOG_ERROR( LOG_MME_APP, "No S11 CBR process for UE with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " was found. Ignoring the message. \n", e_rab_setup_rsp->mme_ue_s1ap_id);
    /** All ESM messages may have returned immediately negative and the CBResp might have been sent (removing the S11 session). */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Get the PDN context. */
  mme_app_get_pdn_context(ue_session_pool->privates.mme_ue_s1ap_id, s11_proc_create_bearer->pci, s11_proc_create_bearer->linked_ebi, NULL, &pdn_context);
  if(!pdn_context){
    OAILOG_ERROR( LOG_MME_APP, "No PDN context (cid=%d,def_ebi=%d) could be found for UE " MME_UE_S1AP_ID_FMT ". Ignoring the message. \n",
        s11_proc_create_bearer->pci, s11_proc_create_bearer->linked_ebi, e_rab_setup_rsp->mme_ue_s1ap_id);
    /** All ESM messages may have returned immediately negative and the CBResp might have been sent (removing the S11 session). */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  for (int i = 0; i < e_rab_setup_rsp->e_rab_setup_list.no_of_items; i++) {
    e_rab_id_t e_rab_id = e_rab_setup_rsp->e_rab_setup_list.item[i].e_rab_id;
    bearer_context_new_t * bc_success = NULL;
    bearer_context_to_be_created_t * bc_tbc = NULL;
    for(int num_bc = 0; num_bc < s11_proc_create_bearer->bcs_tbc->num_bearer_context; num_bc ++){
      if(s11_proc_create_bearer->bcs_tbc->bearer_contexts[num_bc].eps_bearer_id == e_rab_id){
        bc_tbc = &s11_proc_create_bearer->bcs_tbc->bearer_contexts[num_bc];
        break;
      }
    }
    if(!bc_tbc){
    	OAILOG_ERROR(LOG_MME_APP, "The established ebi %d could not be found in the s11 procedure for ueId : " MME_UE_S1AP_ID_FMT ". "
    			"Skipping EBI in the received e_rab setup response. \n", e_rab_id , e_rab_setup_rsp->mme_ue_s1ap_id);
    	continue;
    }

    /** Check if the message needs to be processed (if it has already failed, in that case the number of unhandled bearers already will be reduced). */
    if(bc_tbc->cause.cause_value != 0 && bc_tbc->cause.cause_value != REQUEST_ACCEPTED){
      OAILOG_DEBUG (LOG_MME_APP, "The ebi %d has already a negative error cause %d for ueId : " MME_UE_S1AP_ID_FMT "\n", bc_tbc->eps_bearer_id, bc_tbc->cause.cause_value, e_rab_setup_rsp->mme_ue_s1ap_id);
      /** No need to register the transport layer information or to inform the ESM layer (the session bearer already should be removed). */
      continue;
    }
    /** Cause is either not set or negative. It must be in the session bearers. */
    int rc = mme_app_cn_update_bearer_context(e_rab_setup_rsp->mme_ue_s1ap_id, e_rab_id, &e_rab_setup_rsp->e_rab_setup_list.item[i], NULL);
    if(rc == RETURNerror){
      /* Error updating the bearer context. */
      OAILOG_ERROR(LOG_MME_APP, "The ebi %d could not be updated from the eNB for ueId : " MME_UE_S1AP_ID_FMT "\n", e_rab_id , e_rab_setup_rsp->mme_ue_s1ap_id);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
    bc_success = mme_app_get_session_bearer_context(pdn_context, e_rab_id);
    /** If the ESM EBR context is active. */
    if(bc_success){
      memcpy((void*)&bc_tbc->s1u_enb_fteid, (void*)&bc_success->enb_fteid_s1u, sizeof(bc_success->enb_fteid_s1u));
      if(bc_success->esm_ebr_context.status == ESM_EBR_ACTIVE){ /**< ESM session management messages completed successfully (transactions completed and no negative GTP cause). */
        if(bc_tbc->cause.cause_value == 0){
          /** Should be on the way. */
          OAILOG_DEBUG (LOG_MME_APP, "The cause is not set yet for ebi %d for ueId although NAS is accepted (not reducing num pending bearers yet): " MME_UE_S1AP_ID_FMT "\n",
              bc_tbc->eps_bearer_id, e_rab_setup_rsp->mme_ue_s1ap_id);
          /** Setting it as accepted such that we don't wait for the E-RAB-Setup Rsp which arrived. */
          bc_tbc->cause.cause_value = REQUEST_ACCEPTED;
        } else{
          DevAssert(bc_tbc->cause.cause_value == REQUEST_ACCEPTED);
          /*
           * Reduce the number of unhandled bearers.
           * We keep the ESM procedure (no ESM procedure for this bearer is expected at this point).
           */
          s11_proc_create_bearer->proc.num_bearers_unhandled--;
          /** Set the state of the bearer context as active. */

        }
      }else{
        /*
         * Not reducing the number of unhandled bearers. We will check this cause later when NAS response arrives.
         * We will not trigger a CBResp with this.
         */
        OAILOG_DEBUG (LOG_MME_APP, "Setting the cause as ACCEPTED for ebi %d for ueId : " MME_UE_S1AP_ID_FMT "\n", bc_tbc->eps_bearer_id, e_rab_setup_rsp->mme_ue_s1ap_id);
        bc_tbc->cause.cause_value = REQUEST_ACCEPTED;
      }
    }
    /** Iterated through all bearer contexts. */
  }

  /** Iterate through the failed bearers. */
  for (int i = 0; i < e_rab_setup_rsp->e_rab_failed_to_setup_list.no_of_items; i++) {
    e_rab_id_t e_rab_id = e_rab_setup_rsp->e_rab_failed_to_setup_list.item[i].e_rab_id;
    bearer_context_new_t * bc_failed = NULL;
    bearer_context_to_be_created_t* bc_tbc = NULL;
    for(int num_bc = 0; num_bc < s11_proc_create_bearer->bcs_tbc->num_bearer_context; num_bc ++){
      if(s11_proc_create_bearer->bcs_tbc->bearer_contexts[num_bc].eps_bearer_id == e_rab_id){
        bc_tbc = &s11_proc_create_bearer->bcs_tbc->bearer_contexts[num_bc];
        break;
      }
    }
    if(!bc_tbc){
    	OAILOG_ERROR(LOG_MME_APP, "The failed to be established ebi %d could not be found in the s11 procedure for ueId : " MME_UE_S1AP_ID_FMT ". "
    			"Skipping EBI in the received e_rab setup response. \n", e_rab_id , e_rab_setup_rsp->mme_ue_s1ap_id);
    	continue;
    }

    /** Check if there is already a negative result. */
    if(bc_tbc->cause.cause_value && bc_tbc->cause.cause_value != REQUEST_ACCEPTED){
      OAILOG_DEBUG (LOG_MME_APP, "The ebi %d has already a negative error cause %d for ueId : " MME_UE_S1AP_ID_FMT "\n", bc_tbc->eps_bearer_id, e_rab_setup_rsp->mme_ue_s1ap_id);
      /** The number of negative values should already be decreased. */
      continue;
    }

    /* Release the ESM procedure, the received NAS message should be rejected. */
    nas_esm_proc_bearer_context_t *esm_proc_bearer_context = mme_app_nas_esm_get_bearer_context_procedure(e_rab_setup_rsp->mme_ue_s1ap_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, bc_tbc->eps_bearer_id);
    if(esm_proc_bearer_context){
      /* Finalize the bearer context and terminate the ESM procedure. The response should not be regarded. */
      pti_t pti = esm_proc_bearer_context->esm_base_proc.pti;
      ebi_t ebi = esm_proc_bearer_context->bearer_ebi;
      mme_app_nas_esm_delete_bearer_context_proc(&esm_proc_bearer_context);
      OAILOG_DEBUG (LOG_MME_APP, "Freed the NAS ESM procedure for bearer activation (ebi=%d, pti=%d) for ueId : " MME_UE_S1AP_ID_FMT "\n",
                ebi, pti, e_rab_setup_rsp->mme_ue_s1ap_id);
    }
    mme_app_release_bearer_context(e_rab_setup_rsp->mme_ue_s1ap_id, &pdn_context->context_identifier, s11_proc_create_bearer->linked_ebi, e_rab_id);
    /** Set cause as rejected. */
    bc_tbc->cause.cause_value = REQUEST_REJECTED;
    /** The cause is either not set yet or positive. In either case we will reduce the number of unhandled bearers. */
    s11_proc_create_bearer->proc.num_bearers_unhandled--;
    /**
     * No need to check the result code, the bearer is not established in any case.
     * Also no need to wait for the ESM layer to continue with the CBResp (but sill informing the ESM layer to abort the session management procedure prematurely).
     */
  }
  if(s11_proc_create_bearer->proc.num_bearers_unhandled){
    OAILOG_DEBUG (LOG_MME_APP, "For the S11 dedicated bearer procedure, still %d pending bearers exist for UE " MME_UE_S1AP_ID_FMT ". "
        "Waiting with CBResp. \n", s11_proc_create_bearer->proc.num_bearers_unhandled, ue_session_pool->privates.mme_ue_s1ap_id);
  }else{
	/** Check if the MBR has been done. */
	bearer_context_new_t * default_bc = mme_app_get_session_bearer_context(pdn_context, pdn_context->default_ebi);
	if(default_bc && (default_bc->bearer_state & BEARER_STATE_ACTIVE)){
	    OAILOG_DEBUG (LOG_MME_APP, "For the S11 dedicated bearer procedure, no pending bearers left (either success or failure) & default bearer activated for UE " MME_UE_S1AP_ID_FMT ". "
	        "Sending CBResp immediately. \n", ue_session_pool->privates.mme_ue_s1ap_id);
	    mme_app_send_s11_create_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11, ue_session_pool->privates.fields.saegw_teid_s11, 0, s11_proc_create_bearer->proc.s11_trxn, s11_proc_create_bearer->bcs_tbc);
	    /** Delete the procedure if it exists. */
	    mme_app_delete_s11_procedure_create_bearer(ue_session_pool);
	}
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
static void mme_app_handle_e_rab_setup_rsp_pdn_connectivity(const mme_ue_s1ap_id_t mme_ue_s1ap_id, const e_rab_setup_item_t * e_rab_setup_item, const ebi_t failed_ebi) {
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s            *ue_session_pool = NULL;
  pdn_cid_t                            pdn_cid = PDN_CONTEXT_IDENTIFIER_UNASSIGNED;
  ebi_t                                def_ebi = 0;

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_session_pools, mme_ue_s1ap_id);
  if (ue_session_pool == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " S1AP_E_RAB_SETUP_RSP Unknown ue " MME_UE_S1AP_ID_FMT "\n", mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  if(e_rab_setup_item) {
	bearer_context_new_t * bc_success = NULL;
    pdn_context_t * pdn_context   = NULL;
    mme_app_get_session_bearer_context_from_all(ue_session_pool, (ebi_t) e_rab_setup_item->e_rab_id, &bc_success);
    if(bc_success){
      mme_app_get_pdn_context(ue_session_pool->privates.mme_ue_s1ap_id, bc_success->pdn_cx_id, bc_success->linked_ebi, NULL, &pdn_context);
      if(pdn_context){
        DevAssert(bc_success->bearer_state & BEARER_STATE_SGW_CREATED);
        pdn_cid = bc_success->pdn_cx_id;
        def_ebi = bc_success->linked_ebi;
        bc_success->enb_fteid_s1u.interface_type = S1_U_ENODEB_GTP_U;
        bc_success->enb_fteid_s1u.teid           = e_rab_setup_item->gtp_teid;
        /** Set the IP address. */
        if (4 == blength(e_rab_setup_item->transport_layer_address)) {
          bc_success->enb_fteid_s1u.ipv4         = 1;
          memcpy(&bc_success->enb_fteid_s1u.ipv4_address, e_rab_setup_item->transport_layer_address->data, blength(e_rab_setup_item->transport_layer_address));
        } else if (16 == blength(e_rab_setup_item->transport_layer_address)) {
          bc_success->enb_fteid_s1u.ipv6         = 1;
          memcpy(&bc_success->enb_fteid_s1u.ipv6_address, e_rab_setup_item->transport_layer_address->data, blength(e_rab_setup_item->transport_layer_address));
        } else {
          AssertFatal(0, "TODO IP address %d bytes", blength(e_rab_setup_item->transport_layer_address));
        }
        bc_success->bearer_state |= BEARER_STATE_ENB_CREATED;
        bc_success->bearer_state |= BEARER_STATE_MME_CREATED;
        /*
         * The APN-AMBR value will already be set.
         * Independently from the ESM (not checking the ESM_EBR_STATE), send the MBR to the PGW.
         * If the ESM procedure is rejected or gets into timeout, we must remove the session PGW session via ESM separately.
         */
        mme_app_send_s11_modify_bearer_req(ue_session_pool, pdn_context, 0);
      } else{
        OAILOG_DEBUG (LOG_MME_APP, "For established default bearer with linked_(ebi=%d), no pdn context exists for UE: " MME_UE_S1AP_ID_FMT "\n", bc_success->linked_ebi, mme_ue_s1ap_id);
      }
    }else{
      /**
       * We assume that some error happened in the ESM layer during attach, and therefore the bearer context does not exist in the session bearers.
       */
      OAILOG_ERROR(LOG_MME_APP, "No bearer context was found for default ebi %d for UE: " MME_UE_S1AP_ID_FMT "\n", e_rab_setup_item->e_rab_id, ue_session_pool->privates.mme_ue_s1ap_id);
      OAILOG_FUNC_OUT(LOG_MME_APP);
    }
  }else {
    /** Failed to establish the default bearer. */
	  bearer_context_new_t * bc_failed = NULL;
    mme_app_get_session_bearer_context_from_all(ue_session_pool, failed_ebi, &bc_failed);
    if(bc_failed){
      pdn_cid = bc_failed->pdn_cx_id;
      if(bc_failed->bearer_state & BEARER_STATE_MME_CREATED)
        bc_failed->bearer_state &= (~BEARER_STATE_MME_CREATED);
      if(bc_failed->bearer_state & BEARER_STATE_ENB_CREATED)
        bc_failed->bearer_state &= (~BEARER_STATE_ENB_CREATED);
      pdn_context_t * pdn_context = NULL;
      mme_app_get_pdn_context(ue_session_pool->privates.mme_ue_s1ap_id, pdn_cid, failed_ebi, NULL, &pdn_context);
      /** Send a Delete Session Request. The response will trigger removal of the pdn & bearer resources. */
      OAILOG_WARNING(LOG_MME_APP, "After failed establishment of default bearer with default ebi %d for UE: " MME_UE_S1AP_ID_FMT ", "
          "triggering implicit PDN context removal. \n", failed_ebi, ue_session_pool->privates.mme_ue_s1ap_id);
      /** Remove the ESM procedure if exists. */
      /* Release the ESM procedure, the received NAS message should be rejected. */
      nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity = mme_app_nas_esm_get_pdn_connectivity_procedure(ue_session_pool->privates.mme_ue_s1ap_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED);
      if(esm_proc_pdn_connectivity){
        /* Finalize the bearer context and terminate the ESM procedure. The response should not be regarded. */
        mme_app_nas_esm_delete_pdn_connectivity_proc(&esm_proc_pdn_connectivity);
        OAILOG_DEBUG (LOG_MME_APP, "Freed the NAS ESM procedure for PDN connectivity ueId : " MME_UE_S1AP_ID_FMT "\n", ue_session_pool->privates.mme_ue_s1ap_id);
      }
      /** Set cause as rejected. */
      if(((struct sockaddr*)&pdn_context->s_gw_addr_s11_s4)->sa_family != 0)
        mme_app_send_delete_session_request(ue_session_pool, bc_failed->linked_ebi, &pdn_context->s_gw_addr_s11_s4, pdn_context->s_gw_teid_s11_s4, true,
            false, INTERNAL_FLAG_NULL); /**< Don't delete the S11 Tunnel endpoint (still need for the default apn). */
      else {
        OAILOG_WARNING(LOG_MME_APP, "NO S11 SAE-GW Ipv4 in PDN context of ueId : " MME_UE_S1AP_ID_FMT "\n", ue_session_pool->privates.mme_ue_s1ap_id);
      }
      /** Release the PDN context. */
      mme_app_esm_delete_pdn_context(ue_session_pool->privates.mme_ue_s1ap_id, pdn_context->apn_subscribed, pdn_context->context_identifier, pdn_context->default_ebi);
      OAILOG_FUNC_OUT(LOG_MME_APP);
    }else {
      OAILOG_ERROR(LOG_MME_APP, "No bearer context was found for failed default ebi %d for UE: " MME_UE_S1AP_ID_FMT "\n", failed_ebi, ue_session_pool->privates.mme_ue_s1ap_id);
      OAILOG_FUNC_OUT(LOG_MME_APP);
    }
  }
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_handle_e_rab_modify_rsp (itti_s1ap_e_rab_modify_rsp_t  * const e_rab_modify_rsp)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s            *ue_session_pool = NULL;
  struct pdn_context_s                   *pdn_context = NULL;
  pdn_cid_t pdn_cid = PDN_CONTEXT_IDENTIFIER_UNASSIGNED;
  ebi_t     def_ebi = 0;

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, e_rab_modify_rsp->mme_ue_s1ap_id);
  if (ue_session_pool == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", e_rab_modify_rsp->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " S1AP_E_RAB_MODIFY_RSP Unknown ue " MME_UE_S1AP_ID_FMT "\n", e_rab_modify_rsp->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  mme_app_s11_proc_update_bearer_t * s11_proc_update_bearer = mme_app_get_s11_procedure_update_bearer(ue_session_pool);
  if(!s11_proc_update_bearer){
    OAILOG_ERROR( LOG_MME_APP, "No S11 Update Bearer process for UE with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " was found. Ignoring the message. \n", e_rab_modify_rsp->mme_ue_s1ap_id);
    /** All ESM messages may have returned immediately negative and the UBResp might have been sent (removing the S11 session). */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Get the PDN context. */
  mme_app_get_pdn_context(ue_session_pool->privates.mme_ue_s1ap_id, s11_proc_update_bearer->pci, s11_proc_update_bearer->linked_ebi, NULL, &pdn_context);
  if(!pdn_context){
    OAILOG_ERROR( LOG_MME_APP, "No PDN context (cid=%d,def_ebi=%d) could be found for UE " MME_UE_S1AP_ID_FMT ". Ignoring the message. \n",
        s11_proc_update_bearer->pci, s11_proc_update_bearer->linked_ebi, e_rab_modify_rsp->mme_ue_s1ap_id);
    /** All ESM messages may have returned immediately negative and the CBResp might have been sent (removing the S11 session). */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** Handle the bearer contexts for multi-APN and dedicated bearer cases. */
  for (int i = 0; i < e_rab_modify_rsp->e_rab_modify_list.no_of_items; i++) {
    e_rab_id_t e_rab_id = e_rab_modify_rsp->e_rab_modify_list.item[i].e_rab_id;
    bearer_context_new_t * bc_success = NULL;
    bearer_context_to_be_updated_t * bc_tbu = NULL;
    for(int num_bc = 0; num_bc < s11_proc_update_bearer->bcs_tbu->num_bearer_context; num_bc ++){
      if(s11_proc_update_bearer->bcs_tbu->bearer_contexts[num_bc].eps_bearer_id == e_rab_id){
        bc_tbu = &s11_proc_update_bearer->bcs_tbu->bearer_contexts[num_bc];
        break;
      }
    }
    if(!bc_tbu){
    	OAILOG_ERROR(LOG_MME_APP, "The updated ebi %d could not be found in the s11 procedure for ueId : " MME_UE_S1AP_ID_FMT ". "
    			"Skipping EBI in the received e_rab setup response. \n", e_rab_id , e_rab_modify_rsp->mme_ue_s1ap_id);
      	continue;
    }
    /*
     * Modifications (QoS, TFT) on the bearer context itself will be done by the ESM layer.
     * If UE accepts with success, but eNB does not, we may have a discrepancy.
     */

    /** Check if the message needs to be processed (if it has already failed, in that case the number of unhandled bearers already will be reduced). */
    if(bc_tbu->cause.cause_value != 0 && bc_tbu->cause.cause_value != REQUEST_ACCEPTED){
      OAILOG_DEBUG (LOG_MME_APP, "The ebi %d has already a negative error cause %d for ueId : " MME_UE_S1AP_ID_FMT "\n", bc_tbu->eps_bearer_id, e_rab_modify_rsp->mme_ue_s1ap_id);
      continue;
    }
    /** Cause is either not set or negative. It must be in the session bearers. */
    bc_success = mme_app_get_session_bearer_context(pdn_context, e_rab_id);
    DevAssert(bc_success);
    /** If the ESM EBR context is active. */
    if(bc_success->esm_ebr_context.status == ESM_EBR_ACTIVE){ /**< ESM session management messages completed successfully (transactions completed and no negative GTP cause). */
      /** If 1 bearer is active, update the APN AMBR.*/
      if(bc_tbu->cause.cause_value == 0){
        OAILOG_DEBUG (LOG_MME_APP, "The cause is not set as accepted for ebi %d for ueId although NAS is accepted (not reducing num pending bearers yet): " MME_UE_S1AP_ID_FMT "\n",
        		bc_tbu->eps_bearer_id, e_rab_modify_rsp->mme_ue_s1ap_id);
        /** Setting it as accepted such that we don't wait for the E-RAB which arrived. */
        bc_tbu->cause.cause_value = REQUEST_ACCEPTED;
      } else{
        DevAssert(bc_tbu->cause.cause_value == REQUEST_ACCEPTED);
        /** Reduce the number of unhandled bearers. */
        s11_proc_update_bearer->proc.num_bearers_unhandled--;
      }
    }else{
      /**
       * Not reducing the number of unhandled bearers. We will check this cause later when NAS response arrives.
       * We will not trigger a UBResp with this.
       */
      bc_tbu->cause.cause_value = REQUEST_ACCEPTED;
    }
  }

  /** Iterate through the failed bearers. */
  for (int i = 0; i < e_rab_modify_rsp->e_rab_failed_to_modify_list.no_of_items; i++) {
    e_rab_id_t e_rab_id = e_rab_modify_rsp->e_rab_failed_to_modify_list.item[i].e_rab_id;
    bearer_context_new_t * bc_failed = NULL;
    bearer_context_to_be_updated_t * bc_tbu = NULL;
    for(int num_bc = 0; num_bc < s11_proc_update_bearer->bcs_tbu->num_bearer_context; num_bc ++){
      if(s11_proc_update_bearer->bcs_tbu->bearer_contexts[num_bc].eps_bearer_id == e_rab_id){
        bc_tbu = &s11_proc_update_bearer->bcs_tbu->bearer_contexts[num_bc];
        break;
      }
    }
    if(!bc_tbu){
       	OAILOG_ERROR(LOG_MME_APP, "The failed to be updated ebi %d could not be found in the s11 procedure for ueId : " MME_UE_S1AP_ID_FMT ". "
       			"Skipping EBI in the received e_rab setup response. \n", e_rab_id , e_rab_modify_rsp->mme_ue_s1ap_id);
       	continue;
    }

    /** Check if there is already a negative result. */
    if(bc_tbu->cause.cause_value != 0 && bc_tbu->cause.cause_value != REQUEST_ACCEPTED){
      OAILOG_DEBUG (LOG_MME_APP, "The ebi %d has already a negative error cause %d for ueId : " MME_UE_S1AP_ID_FMT "\n", bc_tbu->eps_bearer_id, e_rab_modify_rsp->mme_ue_s1ap_id);
      /** The number of negative values should already be decreased. */
      continue;
    }
    /** The cause is either not set yet or positive. In either case we will reduce the number of unhandled bearers. */
    s11_proc_update_bearer->proc.num_bearers_unhandled--;
    /** Check the result code, and inform the ESM layer of removal, if necessary.. */
    if((e_rab_modify_rsp->e_rab_failed_to_modify_list.item[i].cause.present == S1ap_Cause_PR_radioNetwork)
          && (e_rab_modify_rsp->e_rab_failed_to_modify_list.item[i].cause.choice.radioNetwork == S1ap_CauseRadioNetwork_unknown_E_RAB_ID)){
      /** No EBI was found. Check if a session bearer exist, if so trigger implicit removal by setting correct cause. */
      bc_tbu->cause.cause_value = NO_RESOURCES_AVAILABLE;
      /** Bearer was implicitly removed in the access network. */
      mme_app_release_bearer_context(e_rab_modify_rsp->mme_ue_s1ap_id, &pdn_context->context_identifier, s11_proc_update_bearer->linked_ebi, e_rab_id);
    }else{
      /** Set cause as rejected. */
      bc_tbu->cause.cause_value = REQUEST_REJECTED;
      /** No need to inform the ESM layer. */
      mme_app_finalize_bearer_context(e_rab_modify_rsp->mme_ue_s1ap_id, pdn_context->context_identifier, s11_proc_update_bearer->linked_ebi, e_rab_id, NULL, NULL, NULL, NULL);
    }
    /* Release the ESM procedure, the received NAS message should be rejected. */
    nas_esm_proc_bearer_context_t *esm_proc_bearer_context = mme_app_nas_esm_get_bearer_context_procedure(e_rab_modify_rsp->mme_ue_s1ap_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED,
        bc_tbu->eps_bearer_id);
    if(esm_proc_bearer_context){
      /* Finalize the bearer context and terminate the ESM procedure. The response should not be regarded. */
      mme_app_nas_esm_delete_bearer_context_proc(&esm_proc_bearer_context);
      OAILOG_DEBUG (LOG_MME_APP, "Freed the NAS ESM procedure for bearer activation (ebi=%d) for ueId : " MME_UE_S1AP_ID_FMT "\n", bc_tbu->eps_bearer_id, e_rab_modify_rsp->mme_ue_s1ap_id);
    }
  }
  // todo: MBReq/MBResp for E-RAB modify?
  if(s11_proc_update_bearer->proc.num_bearers_unhandled){
    OAILOG_DEBUG (LOG_MME_APP, "For the S11 dedicated bearer procedure, still %d pending bearers exist for UE " MME_UE_S1AP_ID_FMT ". "
        "Waiting with UBResp. \n", s11_proc_update_bearer->proc.num_bearers_unhandled, ue_session_pool->privates.mme_ue_s1ap_id);
  }else{
    OAILOG_DEBUG (LOG_MME_APP, "For the S11 dedicated bearer procedure, no pending bearers left (either success or failure) for UE " MME_UE_S1AP_ID_FMT ". "
        "Sending UBResp immediately. \n", ue_session_pool->privates.mme_ue_s1ap_id);

    bearer_context_new_t * default_bc = mme_app_get_session_bearer_context(pdn_context, pdn_context->default_ebi);
    if(default_bc && (default_bc->bearer_state & BEARER_STATE_ACTIVE)){
    	OAILOG_DEBUG (LOG_MME_APP, "For the S11 update bearer procedure, no pending bearers left (either success or failure) & default bearer activated for UE " MME_UE_S1AP_ID_FMT ". "
    			"Sending UBResp immediately. \n", ue_session_pool->privates.mme_ue_s1ap_id);
        mme_app_send_s11_update_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11, ue_session_pool->privates.fields.saegw_teid_s11, 0, s11_proc_update_bearer->proc.s11_trxn, s11_proc_update_bearer->bcs_tbu);
        /*
         * We update the ESM bearer context when the NAS ESM confirmation arrives.
         * We don't care for the E-RAB message.
         */
        mme_app_delete_s11_procedure_update_bearer(ue_session_pool);
    }
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_handle_e_rab_release_ind (const itti_s1ap_e_rab_release_ind_t   * const e_rab_release_ind) {
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s			  *ue_session_pool 	= NULL;
  struct pdn_context_s                *pdn_context 		= NULL;
  ebi_list_t                          ebi_list;
  MessageDef                          *message_p 		= NULL;

  ue_session_pool = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, e_rab_release_ind->mme_ue_s1ap_id);
  if (ue_session_pool == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", e_rab_release_ind->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** The bearers here are sorted. */
  RB_FOREACH (pdn_context, PdnContexts, &ue_session_pool->pdn_contexts) {
	  DevAssert(pdn_context);
	  if(e_rab_release_ind->e_rab_release_list.erab_bitmap & (0x01 << (pdn_context->default_ebi -1))){
		  OAILOG_WARNING(LOG_MME_APP, "Default bearer (ebi=%d) for PDN \"%s\" cannot be released via E-RAB release indication for UE: " MME_UE_S1AP_ID_FMT ". "
				  "Triggering UE context release. \n", pdn_context->default_ebi, bdata(pdn_context->apn_subscribed), e_rab_release_ind->mme_ue_s1ap_id);
		  /** Trigger an implicit detach. */
		  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
		  DevAssert (message_p != NULL);
		  message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_session_pool->privates.mme_ue_s1ap_id;
		  message_p->ittiMsg.nas_implicit_detach_ue_ind.emm_cause = EMM_CAUSE_NETWORK_FAILURE;
		  message_p->ittiMsg.nas_implicit_detach_ue_ind.detach_type = 0x02; // Re-Attach Not required;
		  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
		  itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
		  OAILOG_FUNC_OUT (LOG_MME_APP);
	  }
  }

  /** If the list contains a default bearer, force idle mode. */
  memset(&ebi_list, 0, sizeof(ebi_list_t));
  /** Check the status of the bearers. If they are active & ENB_CREATED, trigger a Delete Bearer Command message. Else ignore. */
  mme_app_release_bearers(e_rab_release_ind->mme_ue_s1ap_id, &e_rab_release_ind->e_rab_release_list, &ebi_list);
  if(ebi_list.num_ebi){
    pdn_context = RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
    if(pdn_context){
      /** Trigger a Delete Bearer Command. */
      mme_app_send_s11_delete_bearer_cmd(ue_session_pool->privates.fields.mme_teid_s11, pdn_context->s_gw_teid_s11_s4, &pdn_context->s_gw_addr_s11_s4, &ebi_list);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
  }
  OAILOG_WARNING (LOG_MME_APP, "Nothing triggered from released e_rab indication for UE: " MME_UE_S1AP_ID_FMT "\n", e_rab_release_ind->mme_ue_s1ap_id);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_handle_activate_eps_bearer_ctx_cnf (itti_nas_activate_eps_bearer_ctx_cnf_t   * const activate_eps_bearer_ctx_cnf)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s            *ue_session_pool 	= NULL;
  struct pdn_context_s                *pdn_context 		= NULL;

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id( &mme_app_desc.mme_ue_session_pools, activate_eps_bearer_ctx_cnf->ue_id);
  if (ue_session_pool == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", activate_eps_bearer_ctx_cnf->ue_id);
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
  mme_app_s11_proc_create_bearer_t * s11_proc_create_bearer = mme_app_get_s11_procedure_create_bearer(ue_session_pool);
  if(!s11_proc_create_bearer){
    /** Assuming all EBIs failed by eNB. */
    OAILOG_ERROR(LOG_MME_APP, "No S11 CBR procedure exists for UE: " MME_UE_S1AP_ID_FMT ". Not sending CBResp back. \n",
    		ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Get the PDN context. */
  mme_app_get_pdn_context(ue_session_pool->privates.mme_ue_s1ap_id, s11_proc_create_bearer->pci, s11_proc_create_bearer->linked_ebi, NULL, &pdn_context);
  if (!pdn_context) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find the pdn context with cid %d and default ebi %d for UE: " MME_UE_S1AP_ID_FMT "\n",
        s11_proc_create_bearer->pci, s11_proc_create_bearer->linked_ebi, activate_eps_bearer_ctx_cnf->ue_id);
     OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Get the bearer context in question. */
  bearer_context_to_be_created_t * bc_tbc = NULL;
  for(int num_bc = 0; num_bc < s11_proc_create_bearer->bcs_tbc->num_bearer_context; num_bc++){
    if(s11_proc_create_bearer->bcs_tbc->bearer_contexts[num_bc].s1u_sgw_fteid.teid == activate_eps_bearer_ctx_cnf->saegw_s1u_teid){
      bc_tbc = &s11_proc_create_bearer->bcs_tbc->bearer_contexts[num_bc];
      break;
    }
  }
  if(!bc_tbc){
	  OAILOG_ERROR(LOG_MME_APP, "The bearer with s1u saegw teid" TEID_FMT " could not be found in the s11 procedure for ueId : " MME_UE_S1AP_ID_FMT ". "
			  "Skipping EBI in the received e_rab setup response. \n", activate_eps_bearer_ctx_cnf->saegw_s1u_teid, activate_eps_bearer_ctx_cnf->ue_id);
	  OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** Update the pending bearer contexts in the answer. */
  // todo: here a minimal lock may be ok (for the cause setting - like atomic boolean)
  if(bc_tbc->cause.cause_value == 0){
    /** No response received yet from E-RAB. Will just set it as SUCCESS, but not trigger an CBResp. */
    OAILOG_INFO(LOG_MME_APP, "Received NAS response before E-RAB setup response for ebi %d for UE: " MME_UE_S1AP_ID_FMT ". "
        "Not triggering a CBResp. \n", activate_eps_bearer_ctx_cnf->ebi, ue_session_pool->privates.mme_ue_s1ap_id);
    bc_tbc->cause.cause_value = REQUEST_ACCEPTED;
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }else if (bc_tbc->cause.cause_value != REQUEST_ACCEPTED) {
    OAILOG_INFO(LOG_MME_APP, "Received NAS response after E-RAB reject for ebi %d for UE: " MME_UE_S1AP_ID_FMT ". "
        "Not reducing number of unhandled bearers (assuming already done). \n", activate_eps_bearer_ctx_cnf->ebi, ue_session_pool->privates.mme_ue_s1ap_id);
    /*
     * E-RAB Reject will be send as an ITTI message to the NAS layer, removing the bearer.
     * No lock is needed.
     */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  OAILOG_INFO(LOG_MME_APP, "Received NAS response after E-RAB setup success for ebi %d for UE: " MME_UE_S1AP_ID_FMT ". "
      "Will reduce number of unhandled bearers and eventually triggering a CBResp. \n", activate_eps_bearer_ctx_cnf->ebi, ue_session_pool->privates.mme_ue_s1ap_id);
  s11_proc_create_bearer->proc.num_bearers_unhandled--;
  bc_tbc->eps_bearer_id = activate_eps_bearer_ctx_cnf->ebi;
  if(s11_proc_create_bearer->proc.num_bearers_unhandled){
    OAILOG_INFO(LOG_MME_APP, "Still %d unhandled bearers exist for CBR for UE: " MME_UE_S1AP_ID_FMT ". Not sending CBResp back yet. \n",
        s11_proc_create_bearer->proc.num_bearers_unhandled, ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  OAILOG_INFO(LOG_MME_APP, "No unhandled bearers left for s11 CBR procedure for UE: " MME_UE_S1AP_ID_FMT ". "
      "Sending CBResp back immediately. \n", ue_session_pool->privates.mme_ue_s1ap_id);

  bearer_context_new_t * default_bc = mme_app_get_session_bearer_context(pdn_context, pdn_context->default_ebi);
  if(default_bc && (default_bc->bearer_state & BEARER_STATE_ACTIVE)){
	  OAILOG_DEBUG (LOG_MME_APP, "For the S11 dedicated bearer procedure, no pending bearers left (either success or failure) & default bearer activated for UE " MME_UE_S1AP_ID_FMT ". "
			  "Sending CBResp immediately. \n", ue_session_pool->privates.mme_ue_s1ap_id);
	  mme_app_send_s11_create_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11, ue_session_pool->privates.fields.saegw_teid_s11, 0, s11_proc_create_bearer->proc.s11_trxn, s11_proc_create_bearer->bcs_tbc);
	  /** Delete the procedure if it exists. */
	  mme_app_delete_s11_procedure_create_bearer(ue_session_pool);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_handle_activate_eps_bearer_ctx_rej (itti_nas_activate_eps_bearer_ctx_rej_t   * const activate_eps_bearer_ctx_rej)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s            *ue_session_pool 	= NULL;
  /** Get the first PDN Context. */
  pdn_context_t                       *pdn_context  	= NULL;

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, activate_eps_bearer_ctx_rej->ue_id);
  if (!ue_session_pool) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", activate_eps_bearer_ctx_rej->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  DevAssert(activate_eps_bearer_ctx_rej->cause_value != REQUEST_ACCEPTED);
  /** Get the MME_APP Dedicated Bearer procedure, */
  mme_app_s11_proc_create_bearer_t * s11_proc_create_bearer = mme_app_get_s11_procedure_create_bearer(ue_session_pool);
  if(!s11_proc_create_bearer){
    OAILOG_ERROR(LOG_MME_APP, "No S11 dedicated bearer procedure exists for UE: " MME_UE_S1AP_ID_FMT ". Not sending CBResp back. \n", ue_session_pool->privates.mme_ue_s1ap_id);
    /** Assuming all E-RAB failures came first. */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  mme_app_get_pdn_context(ue_session_pool->privates.mme_ue_s1ap_id, s11_proc_create_bearer->pci, s11_proc_create_bearer->linked_ebi, NULL, &pdn_context);
  if (!pdn_context) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find the pdn context with cid %d and default ebi %d for UE: " MME_UE_S1AP_ID_FMT "\n",
        s11_proc_create_bearer->pci, s11_proc_create_bearer->linked_ebi, activate_eps_bearer_ctx_rej->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /**
   * TS 23.401: 5.4.1: The MME shall be prepared to receive this message either before or after the Session Management Response message (sent in step 9).
   *
   * The itti message will be sent directly by MME APP or by ESM layer, depending on the order of the messages.
   *
   * So we check the state of the bearers.
   * The indication that S11 Create Bearer Response should be sent to the SAE-GW, should be sent by ESM/NAS layer.
   *
   * No locks should be needed for this.
   */
  bearer_context_to_be_created_t * bc_tbc = NULL;
  for(int num_bc = 0; num_bc < s11_proc_create_bearer->bcs_tbc->num_bearer_context; num_bc++ ){
    if(s11_proc_create_bearer->bcs_tbc->bearer_contexts[num_bc].s1u_sgw_fteid.teid == activate_eps_bearer_ctx_rej->saegw_s1u_teid){
      bc_tbc = &s11_proc_create_bearer->bcs_tbc->bearer_contexts[num_bc];
      break;
    }
  }
  if(!bc_tbc){
	  OAILOG_ERROR(LOG_MME_APP, "The failed to be activated bearer with s1u saegw teid " TEID_FMT " could not be found in the s11 procedure for ueId : " MME_UE_S1AP_ID_FMT ". "
			  "Skipping EBI in the received e_rab setup response. \n", activate_eps_bearer_ctx_rej->saegw_s1u_teid, activate_eps_bearer_ctx_rej->ue_id);
	  OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** The bearer is assumed to be removed from the session bearers by the ESM layer. */
  if (bc_tbc->cause.cause_value != 0 && bc_tbc->cause.cause_value != REQUEST_ACCEPTED) {
    OAILOG_INFO(LOG_MME_APP, "Received NAS reject after E-RAB activation reject for ebi %d occurred for UE: " MME_UE_S1AP_ID_FMT ". Not reducing number of unhandled bearers (assuming already done). \n",
        bc_tbc->eps_bearer_id, ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** No response received yet from E-RAB or positive response. Will just set it as SUCCESS, but not trigger an cBResp. */
  OAILOG_INFO(LOG_MME_APP, "Received NAS reject before E-RAB activation response, or after positive E-RAB response for UE: " MME_UE_S1AP_ID_FMT ". Reducing number of unhandled bearers. \n", ue_session_pool->privates.mme_ue_s1ap_id);
  /** If we received a positive response, remove the bearer. */
  if(bc_tbc->cause.cause_value == REQUEST_ACCEPTED){
    OAILOG_INFO(LOG_MME_APP, "Received NAS reject after successful E-RAB activation responsefor UE: " MME_UE_S1AP_ID_FMT ". "
        "Removing the bearer in the RAT. \n", ue_session_pool->privates.mme_ue_s1ap_id);
    mme_app_handle_nas_erab_release_req(activate_eps_bearer_ctx_rej->ue_id, bc_tbc->eps_bearer_id, NULL);
  }

  bc_tbc->cause.cause_value = activate_eps_bearer_ctx_rej->cause_value ? activate_eps_bearer_ctx_rej->cause_value : REQUEST_REJECTED;
  s11_proc_create_bearer->proc.num_bearers_unhandled--;
  if(s11_proc_create_bearer->proc.num_bearers_unhandled){
    OAILOG_INFO(LOG_MME_APP, "Still %d unhandled bearers exist for s11 create bearer procedure for UE: " MME_UE_S1AP_ID_FMT ". Not sending CBResp back yet. \n", s11_proc_create_bearer->proc.num_bearers_unhandled, ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  OAILOG_INFO(LOG_MME_APP, "No unhandled bearers left for s11 create bearer procedure for UE: " MME_UE_S1AP_ID_FMT ". Sending CBResp back immediately. \n", ue_session_pool->privates.mme_ue_s1ap_id);
  bearer_context_new_t * default_bc = mme_app_get_session_bearer_context(pdn_context, pdn_context->default_ebi);
  if(default_bc && (default_bc->bearer_state & BEARER_STATE_ACTIVE)){
	  OAILOG_DEBUG (LOG_MME_APP, "For the S11 dedicated bearer procedure, no pending bearers left (either success or failure) & default bearer activated for UE " MME_UE_S1AP_ID_FMT ". "
			  "Sending CBResp immediately. \n", ue_session_pool->privates.mme_ue_s1ap_id);
	  mme_app_send_s11_create_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11, ue_session_pool->privates.fields.saegw_teid_s11, 0, s11_proc_create_bearer->proc.s11_trxn, s11_proc_create_bearer->bcs_tbc);
	  /** Delete the procedure if it exists. No ESM procedure expected to be removed (should not exist here). */
	  mme_app_delete_s11_procedure_create_bearer(ue_session_pool);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_handle_modify_eps_bearer_ctx_cnf (itti_nas_modify_eps_bearer_ctx_cnf_t   * const modify_eps_bearer_ctx_cnf)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s            *ue_session_pool = NULL;
  struct pdn_context_s                *pdn_context = NULL;
  struct bearer_context_new_s         *bearer_context = NULL;
  MessageDef                          *message_p = NULL;

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id( &mme_app_desc.mme_ue_session_pools, modify_eps_bearer_ctx_cnf->ue_id);
  if (ue_session_pool == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", modify_eps_bearer_ctx_cnf->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /*
   * TS 23.401: 5.4.2: The MME shall be prepared to receive this message either before or after the Session Management Response message (sent in step 9).
   *
   * The itti message will be sent directly by MME APP or by ESM layer, depending on the order of the messages.
   *
   * So we check the state of the bearers.
   * The indication that S11 Update Bearer Response should be sent to the SAE-GW, should be sent by ESM/NAS layer.
   */
  mme_app_s11_proc_update_bearer_t * s11_proc_update_bearer = mme_app_get_s11_procedure_update_bearer(ue_session_pool);
  if(!s11_proc_update_bearer){
    OAILOG_ERROR(LOG_MME_APP, "No S11 update bearer procedure exists for UE: " MME_UE_S1AP_ID_FMT ". Not sending UBResp back. \n",
    		ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** Get the PDN context. */
  mme_app_get_pdn_context(ue_session_pool->privates.mme_ue_s1ap_id, s11_proc_update_bearer->pci, s11_proc_update_bearer->linked_ebi, NULL, &pdn_context);
  if (!pdn_context) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find the pdn context with cid %d and default ebi %d for UE: " MME_UE_S1AP_ID_FMT "\n",
        s11_proc_update_bearer->pci, s11_proc_update_bearer->linked_ebi, modify_eps_bearer_ctx_cnf->ue_id);
     OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  bearer_context = mme_app_get_session_bearer_context(pdn_context, modify_eps_bearer_ctx_cnf->ebi);
  if(!bearer_context){
    OAILOG_ERROR(LOG_MME_APP, "Could not find the bearer context for ebi %d and UE: " MME_UE_S1AP_ID_FMT ". Disregarding positive ESM response. \n",
        modify_eps_bearer_ctx_cnf->ebi, ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** Get the bearer context in question. */
  bearer_context_to_be_updated_t * bc_tbu = NULL;
  for(int num_bc = 0; num_bc < s11_proc_update_bearer->bcs_tbu->num_bearer_context; num_bc ++){
    if(s11_proc_update_bearer->bcs_tbu->bearer_contexts[num_bc].eps_bearer_id == modify_eps_bearer_ctx_cnf->ebi){
      bc_tbu = &s11_proc_update_bearer->bcs_tbu->bearer_contexts[num_bc];
      break;
    }
  }
  if(!bc_tbu){
	  OAILOG_ERROR(LOG_MME_APP, "The modified ebi %d could not be found in the s11 procedure for ueId : " MME_UE_S1AP_ID_FMT ". "
			  "Skipping EBI in the received e_rab setup response. \n", modify_eps_bearer_ctx_cnf->ebi, modify_eps_bearer_ctx_cnf->ue_id);
	  OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** Update the pending bearer contexts in the answer. */
  if(bc_tbu->cause.cause_value == 0){
    /** No response received yet from E-RAB. Will just set it as SUCCESS, but not trigger an UBResp. */
     /** Check if a QoS informaiton was received. */
    bc_tbu->cause.cause_value = REQUEST_ACCEPTED;
    if(bc_tbu->bearer_level_qos) {
    	OAILOG_INFO(LOG_MME_APP, "Received NAS response before E-RAB modification response for (ebi=%d) occurred for UE: " MME_UE_S1AP_ID_FMT ". Not triggering a UBResp. \n",
    			bc_tbu->eps_bearer_id, ue_session_pool->privates.mme_ue_s1ap_id);
    	OAILOG_FUNC_OUT (LOG_MME_APP);
    } else {
    	// todo: nothing expected..
    	OAILOG_INFO(LOG_MME_APP, "No QoS information received for E-RAB modification (ebi=%d) occurred for UE: " MME_UE_S1AP_ID_FMT ". Continuing to handle it. \n",
    			bc_tbu->eps_bearer_id, ue_session_pool->privates.mme_ue_s1ap_id);
        bc_tbu->cause.cause_value = REQUEST_ACCEPTED;
   }
  }else if (bc_tbu->cause.cause_value != REQUEST_ACCEPTED) {
    OAILOG_INFO(LOG_MME_APP, "Received NAS response after E-RAB modification reject for ebi %d occurred for UE: " MME_UE_S1AP_ID_FMT ". Not reducing number of unhandled bearers (assuming already done). \n",
        bc_tbu->eps_bearer_id, ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  OAILOG_INFO(LOG_MME_APP, "Received NAS response after positive E-RAB modification response for ebi %d occurred for UE: " MME_UE_S1AP_ID_FMT ". Will reduce number of unhandled bearers and eventually triggering a UBResp. \n",
      bc_tbu->eps_bearer_id, ue_session_pool->privates.mme_ue_s1ap_id);
  s11_proc_update_bearer->proc.num_bearers_unhandled--;

  if(s11_proc_update_bearer->proc.num_bearers_unhandled){
    OAILOG_INFO(LOG_MME_APP, "Still %d unhandled bearers exist for s11 update bearer procedure for UE: " MME_UE_S1AP_ID_FMT ". Not sending UBResp back yet. \n",
        s11_proc_update_bearer->proc.num_bearers_unhandled, ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  OAILOG_INFO(LOG_MME_APP, "No unhandled bearers left for s11 update bearer procedure for UE: " MME_UE_S1AP_ID_FMT ". Sending UBResp back immediately. \n", ue_session_pool->privates.mme_ue_s1ap_id);
  bearer_context_new_t * default_bc = mme_app_get_session_bearer_context(pdn_context, pdn_context->default_ebi);
  if(default_bc && (default_bc->bearer_state & BEARER_STATE_ACTIVE)){
  	OAILOG_DEBUG (LOG_MME_APP, "For the S11 update bearer procedure, no pending bearers left (either success or failure) & default bearer activated for UE " MME_UE_S1AP_ID_FMT ". "
  			"Sending UBResp immediately. \n", ue_session_pool->privates.mme_ue_s1ap_id);
    mme_app_send_s11_update_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11, ue_session_pool->privates.fields.saegw_teid_s11, 0, s11_proc_update_bearer->proc.s11_trxn, s11_proc_update_bearer->bcs_tbu);
    /*
     * We update the ESM bearer context when the NAS ESM confirmation arrives.
     * We don't care for the E-RAB message.
     */
    mme_app_delete_s11_procedure_update_bearer(ue_session_pool);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_handle_modify_eps_bearer_ctx_rej (itti_nas_modify_eps_bearer_ctx_rej_t   * const modify_eps_bearer_ctx_rej)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s            *ue_session_pool	= NULL;
  pdn_context_t                       *pdn_context  	= NULL;

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id( &mme_app_desc.mme_ue_session_pools, modify_eps_bearer_ctx_rej->ue_id);
  if (!ue_session_pool) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", modify_eps_bearer_ctx_rej->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Get the MME_APP Update Bearer procedure. */
  mme_app_s11_proc_update_bearer_t * s11_proc_update_bearer = mme_app_get_s11_procedure_update_bearer(ue_session_pool);
  if(!s11_proc_update_bearer){
    OAILOG_ERROR(LOG_MME_APP, "No S11 update bearer procedure exists for UE: " MME_UE_S1AP_ID_FMT ". Not sending UBResp back. \n",
    			ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /*
   * TS 23.401: 5.4.2: The MME shall be prepared to receive this message either before or after the Session Management Response message (sent in step 9).
   *
   * The itti message will be sent directly by MME APP or by ESM layer, depending on the order of the messages.
   *
   * So we check the state of the bearers.
   * The indication that S11 Update Bearer Response should be sent to the SAE-GW, should be sent by ESM/NAS layer.
   */
  /** Get the PDN context. */
  mme_app_get_pdn_context(ue_session_pool->privates.mme_ue_s1ap_id, s11_proc_update_bearer->pci, s11_proc_update_bearer->linked_ebi, NULL, &pdn_context);
  if (!pdn_context) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find the pdn context with cid %d and default ebi %d for UE: " MME_UE_S1AP_ID_FMT "\n",
        s11_proc_update_bearer->pci, s11_proc_update_bearer->linked_ebi, modify_eps_bearer_ctx_rej->ue_id);
     OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Assert that the bearer context is already removed. */
  bearer_context_to_be_updated_t * bc_tbu = NULL;
  for(int num_bc = 0; num_bc < s11_proc_update_bearer->bcs_tbu->num_bearer_context; num_bc++ ){
    if(s11_proc_update_bearer->bcs_tbu->bearer_contexts[num_bc].eps_bearer_id == modify_eps_bearer_ctx_rej->ebi){
      bc_tbu = &s11_proc_update_bearer->bcs_tbu->bearer_contexts[num_bc];
      break;
    }
  }
  if(!bc_tbu){
	  OAILOG_ERROR(LOG_MME_APP, "failed to be modified ebi %d could not be found in the s11 procedure for ueId : " MME_UE_S1AP_ID_FMT ". "
			  "Skipping EBI in the received e_rab setup response. \n", modify_eps_bearer_ctx_rej->ebi, modify_eps_bearer_ctx_rej->ue_id);
	  OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** Update the pending bearer contexts in the answer. */
  if (bc_tbu->cause.cause_value != 0 && bc_tbu->cause.cause_value != REQUEST_ACCEPTED) {
    OAILOG_INFO(LOG_MME_APP, "Received NAS reject after E-RAB modification reject for ebi %d occurred for UE: " MME_UE_S1AP_ID_FMT ". Not reducing number of unhandled bearers (assuming already done). \n",
        bc_tbu->eps_bearer_id, ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** No response received yet from E-RAB. Will just set it as SUCCESS, but not trigger an UBResp. */
  OAILOG_INFO(LOG_MME_APP, "Received NAS reject before E-RAB modification response, or after positive E-RAB response for ebi %d for UE: " MME_UE_S1AP_ID_FMT ". Reducing number of unhandled bearers. \n",
      bc_tbu->eps_bearer_id, ue_session_pool->privates.mme_ue_s1ap_id);
  bc_tbu->cause.cause_value = modify_eps_bearer_ctx_rej->cause_value ? modify_eps_bearer_ctx_rej->cause_value : REQUEST_REJECTED;

  s11_proc_update_bearer->proc.num_bearers_unhandled--;
  if(s11_proc_update_bearer->proc.num_bearers_unhandled){
	OAILOG_INFO(LOG_MME_APP, "Still %d unhandled bearers exist for s11 update bearer procedure for UE: " MME_UE_S1AP_ID_FMT ". Not sending UBResp back yet. \n",
        s11_proc_update_bearer->proc.num_bearers_unhandled, ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  OAILOG_INFO(LOG_MME_APP, "No unhandled bearers left for s11 update bearer procedure for UE: " MME_UE_S1AP_ID_FMT ". Sending UBResp back immediately. \n", ue_session_pool->privates.mme_ue_s1ap_id);
  /** Delete the procedure if it exists. */
  if(modify_eps_bearer_ctx_rej->cause_value != TEMP_REJECT_HO_IN_PROGRESS){
    bearer_context_new_t * default_bc = mme_app_get_session_bearer_context(pdn_context, pdn_context->default_ebi);
    if(default_bc && (default_bc->bearer_state & BEARER_STATE_ACTIVE)){
      OAILOG_DEBUG (LOG_MME_APP, "For the S11 update bearer procedure, no pending bearers left (either success or failure) & default bearer activated for UE " MME_UE_S1AP_ID_FMT ". "
    		  "Sending UBResp immediately. \n", ue_session_pool->privates.mme_ue_s1ap_id);
      mme_app_send_s11_update_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11, ue_session_pool->privates.fields.saegw_teid_s11, 0, s11_proc_update_bearer->proc.s11_trxn, s11_proc_update_bearer->bcs_tbu);      /*
       * We update the ESM bearer context when the NAS ESM confirmation arrives.
       * We don't care for the E-RAB message.
       */
      mme_app_delete_s11_procedure_update_bearer(ue_session_pool);
    }
  } else {
	  OAILOG_WARNING(LOG_MME_APP, "Not removing/sending the s11 ubr procedure, since HO is in progress for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_session_pool->privates.mme_ue_s1ap_id);
  }

  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_handle_deactivate_eps_bearer_ctx_cnf (itti_nas_deactivate_eps_bearer_ctx_cnf_t   * const deactivate_eps_bearer_ctx_cnf)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_session_pool_s	                *ue_session_pool = NULL;

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id( &mme_app_desc.mme_ue_session_pools, deactivate_eps_bearer_ctx_cnf->ue_id);
  if (ue_session_pool == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", deactivate_eps_bearer_ctx_cnf->ue_id);
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
  /** Get the MME_APP Dedicated Bearer procedure, */
  mme_app_s11_proc_delete_bearer_t * s11_proc_delete_bearer = mme_app_get_s11_procedure_delete_bearer(ue_session_pool);
  if(!s11_proc_delete_bearer){
    OAILOG_ERROR(LOG_MME_APP, "No S11 dedicated bearer procedure exists for UE: " MME_UE_S1AP_ID_FMT ". Not sending DBResp back. \n",
    		ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Check if the bearer context exists as a session bearer. */
  bearer_context_new_t * bc = NULL;
  mme_app_get_session_bearer_context_from_all(ue_session_pool, deactivate_eps_bearer_ctx_cnf->ded_ebi, &bc);
  if(deactivate_eps_bearer_ctx_cnf->cause_value == REQUEST_ACCEPTED)
	  DevAssert(!bc);

  /** Not checking S1U E-RAB Release Response. */
  s11_proc_delete_bearer->proc.num_bearers_unhandled--;
  if(s11_proc_delete_bearer->proc.num_bearers_unhandled){
    OAILOG_INFO(LOG_MME_APP, "Still %d unhandled bearers exist for s11 delete dedicated bearer procedure for UE: " MME_UE_S1AP_ID_FMT ". Not sending DBResp back yet. \n",
        s11_proc_delete_bearer->proc.num_bearers_unhandled, ue_session_pool->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  OAILOG_INFO(LOG_MME_APP, "No unhandled bearers left for s11 delete dedicated bearer procedure for UE: " MME_UE_S1AP_ID_FMT ". Sending DBResp back immediately. \n", ue_session_pool->privates.mme_ue_s1ap_id);
  if(!deactivate_eps_bearer_ctx_cnf->cause_value)
	  deactivate_eps_bearer_ctx_cnf->cause_value = REQUEST_ACCEPTED;
  if(deactivate_eps_bearer_ctx_cnf->cause_value != TEMP_REJECT_HO_IN_PROGRESS){
	  mme_app_send_s11_delete_bearer_rsp(ue_session_pool->privates.fields.mme_teid_s11, ue_session_pool->privates.fields.saegw_teid_s11, deactivate_eps_bearer_ctx_cnf->cause_value, s11_proc_delete_bearer->proc.s11_trxn, &s11_proc_delete_bearer->ebis);
	  /** Delete the procedure if it exists. */
	  mme_app_delete_s11_procedure_delete_bearer(ue_session_pool);
  } else {
	  OAILOG_WARNING(LOG_MME_APP, "Not removing/sending the s11 dbr procedure, since HO is in progress for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_session_pool->privates.mme_ue_s1ap_id);
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
 * Method to send the path switch request acknowledge to the target-eNB.
 * Will not make any changes in the UE context.
 * No timer to be started.
 */
static
void mme_app_send_s1ap_path_switch_request_acknowledge(mme_ue_s1ap_id_t mme_ue_s1ap_id,
    uint16_t encryption_algorithm_capabilities, uint16_t integrity_algorithm_capabilities,
    bearer_contexts_to_be_created_t * bcs_tbc){
  MessageDef * message_p = NULL;
  bearer_context_new_t                   *current_bearer_p = NULL;
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


  /** Set the bearer contexts to be created. Not changing any bearer state. */
  path_switch_req_ack_p->bearer_ctx_to_be_switched_list = calloc(1, sizeof(bearer_contexts_to_be_created_t));
  memcpy((void*)path_switch_req_ack_p->bearer_ctx_to_be_switched_list, bcs_tbc, sizeof(*bcs_tbc));

  /** Set the new security parameters. */
  path_switch_req_ack_p->security_capabilities_encryption_algorithms = encryption_algorithm_capabilities;
  path_switch_req_ack_p->security_capabilities_integrity_algorithms  = integrity_algorithm_capabilities;

  /** Set the next hop value and the NCC value. */
  memcpy(path_switch_req_ack_p->nh, ue_nas_ctx->_vector[ue_nas_ctx->_security.vector_index].nh_conj, AUTH_NH_SIZE);
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
void
mme_app_handle_path_switch_req(
  const itti_s1ap_path_switch_request_t * const s1ap_path_switch_req
  )
{
  OAILOG_FUNC_IN (LOG_MME_APP);

  struct ue_context_s                    *ue_context 		= NULL;
  struct ue_session_pool_s               *ue_session_pool 	= NULL;
  MessageDef                             *message_p 		= NULL;
  ebi_list_t                              ebi_list;

  OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_PATH_SWITCH_REQUEST from S1AP\n");
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, s1ap_path_switch_req->mme_ue_s1ap_id);
  if (ue_context == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: %08x %d(dec)\n", s1ap_path_switch_req->mme_ue_s1ap_id, s1ap_path_switch_req->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_PATH_SWITCH_REQUEST Unknown ue %u", s1ap_path_switch_req->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, s1ap_path_switch_req->mme_ue_s1ap_id);
  if (ue_session_pool == NULL) {
	  OAILOG_ERROR (LOG_MME_APP, "We didn't find an UE session pool for UE "MME_UE_S1AP_ID_FMT". \n", s1ap_path_switch_req->mme_ue_s1ap_id);
      OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  enb_s1ap_id_key_t                       enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;
  // todo: ideally, this should also be locked..
  /** Update the ENB_ID_KEY. */
  MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, s1ap_path_switch_req->enb_id, s1ap_path_switch_req->enb_ue_s1ap_id);
  // Update enb_s1ap_id_key in hashtable
  mme_ue_context_update_coll_keys( &mme_app_desc.mme_ue_contexts,
      ue_context,
      enb_s1ap_id_key,
      ue_context->privates.mme_ue_s1ap_id,
      ue_context->privates.fields.imsi,
      ue_context->privates.fields.mme_teid_s11,
      ue_context->privates.fields.local_mme_teid_s10,
      &ue_context->privates.fields.guti);

  // Set the handover flag, check that no handover exists.
  ue_context->privates.fields.enb_ue_s1ap_id    = s1ap_path_switch_req->enb_ue_s1ap_id;
  ue_context->privates.fields.sctp_assoc_id_key = s1ap_path_switch_req->sctp_assoc_id;

  // todo: set the enb and cell id
  ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id  = s1ap_path_switch_req->e_utran_cgi.cell_identity.enb_id;
  ue_context->privates.fields.e_utran_cgi.cell_identity.cell_id = s1ap_path_switch_req->e_utran_cgi.cell_identity.cell_id;
  //  sctp_stream_id_t        sctp_stream;

  /*
   * 36.413: 8.4.4.2
   * If the E-RAB To Be Switched in Downlink List IE in the PATH SWITCH REQUEST message does not include all E-RABs previously included
   * in the UE Context, the MME shall consider the non included E-RABs as implicitly released by the eNB.
   * We do not have to inform the ESM layer about it.
   * todo: Currently, we assume that default bearers are not removed.
   */
  /** All bearers to be switched, perform without ESM layer. */
  memset(&ebi_list, 0, sizeof(ebi_list_t));

  /** Release all bearers. */
  mme_app_release_bearers(s1ap_path_switch_req->mme_ue_s1ap_id, NULL, &ebi_list);
  if (mme_app_modify_bearers(s1ap_path_switch_req->mme_ue_s1ap_id, &s1ap_path_switch_req->bcs_to_be_modified) == RETURNerror) {
    OAILOG_ERROR (LOG_MME_APP, "Error updating the bearers based on X2 path switch request for UE " MME_UE_S1AP_ID_FMT ". \n", s1ap_path_switch_req->mme_ue_s1ap_id);
    mme_app_send_s1ap_path_switch_request_failure(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.fields.sctp_assoc_id_key, S1ap_Cause_PR_misc);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  /** Updated the bearers, send an S11 Modify Bearer Request on the updated bearers. */
  OAILOG_INFO(LOG_MME_APP, "Sending MBR due to Patch Switch Request for UE " MME_UE_S1AP_ID_FMT " . \n", ue_context->privates.mme_ue_s1ap_id);
  /**
   * Get the first PDN context for sending MBRs, too.
   * The FTEIDs with eNB-FTEID 0 will be sent as bearer contexts to be removed.
   */
  pdn_context_t * first_pdn = RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
  uint8_t flags = 0;
  flags |= INTERNAL_FLAG_X2_HANDOVER;
  mme_app_send_s11_modify_bearer_req(ue_session_pool, first_pdn, flags);
  /** Check for any bearers removed, inform the ESM layer. */
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_s1ap_handover_required(
     itti_s1ap_handover_required_t * const handover_required_pP
    )   {
  OAILOG_FUNC_IN (LOG_MME_APP);

  emm_data_context_t                     *ue_nas_ctx 	  = NULL;
  struct ue_context_s                    *ue_context 	  = NULL;
  struct ue_session_pool_s               *ue_session_pool = NULL;
  MessageDef                             *message_p  	  = NULL;

  OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_HANDOVER_REQUIRED from S1AP\n");
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, handover_required_pP->mme_ue_s1ap_id);
  if (!ue_context) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: %08x %d(dec)\n", handover_required_pP->mme_ue_s1ap_id, handover_required_pP->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_REQUIRED Unknown ue %u", handover_required_pP->mme_ue_s1ap_id);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
    /* Resources will be removed by itti removal function. */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id( &mme_app_desc.mme_ue_session_pools, handover_required_pP->mme_ue_s1ap_id);
  if (!ue_session_pool) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find UE session pool for mme_ue_s1ap_id in list of UE: %08x %d(dec)\n", handover_required_pP->mme_ue_s1ap_id, handover_required_pP->mme_ue_s1ap_id);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
    /* Resources will be removed by itti removal function. */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  if (ue_context->privates.fields.mm_state != UE_REGISTERED) {
    OAILOG_ERROR (LOG_MME_APP, "UE with ue_id " MME_UE_S1AP_ID_FMT " is not in UE_REGISTERED state. Rejecting the Handover Preparation. \n", ue_context->privates.mme_ue_s1ap_id);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
    /** No change in the UE context needed. */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /*
   * Staying in the same state (UE_REGISTERED).
   * If the GUTI is not found, first send a preparation failure, then implicitly detach.
   */
  ue_nas_ctx = emm_data_context_get_by_guti (&_emm_data, &ue_context->privates.fields.guti);
  /** Check that the UE NAS context != NULL. */
  if (!ue_nas_ctx || emm_fsm_get_state (ue_nas_ctx) != EMM_REGISTERED) {
    OAILOG_ERROR (LOG_MME_APP, "EMM context for UE with ue_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " is not in EMM_REGISTERED state or not existing. "
        "Rejecting the Handover Preparation. \n", handover_required_pP->mme_ue_s1ap_id, ue_context->privates.fields.imsi);
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
        "Rejecting further procedures. \n", handover_required_pP->mme_ue_s1ap_id, ue_context->privates.fields.imsi);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  /** Check if there are bearers/pdn contexts (a detach procedure might be ongoing in parallel). */
  pdn_context_t * first_pdn = RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
  if(!first_pdn) {
    OAILOG_ERROR (LOG_MME_APP, "EMM context UE with ue_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " in EMM_REGISTERED state has no PDN contexts. "
        "Rejecting further procedures. \n", handover_required_pP->mme_ue_s1ap_id, ue_context->privates.fields.imsi);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /*
   * Update Security Parameters and send them to the target MME or target eNB.
   * This is same next hop procedure as in X2.
   *
   * Check if the destination eNodeB is attached at the same or another MME.
   */
  if (mme_app_check_ta_local(&handover_required_pP->selected_tai.plmn, handover_required_pP->selected_tai.tac)) {
    /** Check if the eNB with the given eNB-ID is served. */
    if(s1ap_is_enb_id_in_list(handover_required_pP->global_enb_id.cell_identity.enb_id) != NULL){
      OAILOG_DEBUG (LOG_MME_APP, "Target ENB_ID %d of target TAI " TAI_FMT " is served by current MME. \n",
          handover_required_pP->global_enb_id.cell_identity.enb_id, TAI_ARG(&handover_required_pP->selected_tai));
      /*
       * Create a new handover procedure and begin processing.
       */
      s10_handover_procedure = mme_app_create_s10_procedure_mme_handover(ue_context, false, MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER, NULL);
      if(!s10_handover_procedure){
        OAILOG_ERROR (LOG_MME_APP, "Could not create new handover procedure for UE with ue_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " in EMM_REGISTERED state. Rejecting further procedures. \n",
            handover_required_pP->mme_ue_s1ap_id, ue_context->privates.fields.imsi);
        mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
        OAILOG_FUNC_OUT (LOG_MME_APP);
      }
      /*
       * Fill the target information, and use it if handover cancellation is received.
       * No transaction needed on source side.
       * Keep the source side as it is.
       */
      memcpy((void*)&s10_handover_procedure->target_tai, (void*)&handover_required_pP->selected_tai, sizeof(handover_required_pP->selected_tai));
      memcpy((void*)&s10_handover_procedure->target_ecgi, (void*)&handover_required_pP->global_enb_id, sizeof(handover_required_pP->global_enb_id)); /**< Home or macro enb id. */

      /*
       * Set the source values, too. We might need it if the MME_STATUS_TRANSFER has to be sent after Handover Notify (emulator errors).
       * Also, this may be used when removing the old ue_reference later.
       */
      s10_handover_procedure->source_enb_ue_s1ap_id = ue_context->privates.fields.enb_ue_s1ap_id;
      s10_handover_procedure->source_ecgi = ue_context->privates.fields.e_utran_cgi;
      /*
       * No need to store the transparent container. Directly send it to the target eNB.
       * Prepare a Handover Request, keep the transparent container for now, it will be purged together with the free method of the S1AP message.
       */
      /** Get all VOs of all session bearers and send handover request with it. */
      bearer_contexts_to_be_created_t bcs_tbc;
      memset((void*)&bcs_tbc, 0, sizeof(bcs_tbc));
      pdn_context_t * registered_pdn_ctx = NULL;
      RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_session_pool->pdn_contexts) {
        DevAssert(registered_pdn_ctx);
        mme_app_get_bearer_contexts_to_be_created(registered_pdn_ctx, &bcs_tbc, BEARER_STATE_NULL);
        /** The number of bearers will be incremented in the method. S10 should just pick the ebi. */
      }
      /** Check the number of bc's. If != 0, update the security parameters send the handover request. */
      if(!bcs_tbc.num_bearer_context){
        OAILOG_INFO (LOG_MME_APP, "No BC context exist. Reject the handover for mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ". \n", ue_context->privates.mme_ue_s1ap_id);
        /** Send a handover preparation failure with error. */
        mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
        OAILOG_FUNC_OUT (LOG_MME_APP);
      }
      uint16_t encryption_algorithm_capabilities = (uint16_t)0;
      uint16_t integrity_algorithm_capabilities  = (uint16_t)0;
      /** Update the security parameters. */
      if(emm_data_context_update_security_parameters(ue_context->privates.mme_ue_s1ap_id, &encryption_algorithm_capabilities, &integrity_algorithm_capabilities) != RETURNok){
        OAILOG_ERROR(LOG_MME_APP, "Error updating AS security parameters for UE with ueId: " MME_UE_S1AP_ID_FMT ". \n", handover_required_pP->mme_ue_s1ap_id);
        mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
        /** If UE state is REGISTERED, then we also expect security context to be valid. */
        OAILOG_FUNC_OUT (LOG_MME_APP);
      }
      OAILOG_INFO(LOG_MME_APP, "Successfully updated AS security parameters for UE with ueId: " MME_UE_S1AP_ID_FMT ". Continuing handover request for INTRA-MME handover. \n", handover_required_pP->mme_ue_s1ap_id);
      ambr_t total_apn_ambr = mme_app_total_p_gw_apn_ambr(ue_session_pool);
      mme_app_send_s1ap_handover_request(handover_required_pP->mme_ue_s1ap_id,
          &bcs_tbc,
          &total_apn_ambr,
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
//      mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
      /** The target eNB-ID is not served by this MME. */
      OAILOG_WARNING(LOG_MME_APP, "Target ENB_ID %d of target TAI " TAI_FMT " is NOT served by current MME. Checking for neighboring MMEs. \n",
          handover_required_pP->global_enb_id.cell_identity.enb_id, TAI_ARG(&handover_required_pP->selected_tai));
    }
  }
  OAILOG_DEBUG (LOG_MME_APP, "Target TA  "TAI_FMT " is NOT served by current MME. Searching for a neighboring MME. \n", TAI_ARG(&handover_required_pP->selected_tai));

  struct sockaddr *neigh_mme_ip_addr = NULL;
  if (1) {
    // TODO prototype may change
    mme_app_select_service(&handover_required_pP->selected_tai, &neigh_mme_ip_addr, S10_MME_GTP_C);
    //    session_request_p->peer_ip.in_addr = mme_config.ipv4.
    if(!neigh_mme_ip_addr){
      /** Send a Handover Preparation Failure back. */
      mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
      OAILOG_ERROR (LOG_MME_APP, "The selected TAI " TAI_FMT " is not configured as an S10 MME neighbor. "
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
  memcpy((void*)&forward_relocation_request_p->peer_ip, neigh_mme_ip_addr,
   		  (neigh_mme_ip_addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

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
  s10_handover_procedure = mme_app_create_s10_procedure_mme_handover(ue_context, false, MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER, neigh_mme_ip_addr);
  if(!s10_handover_procedure){
    OAILOG_ERROR (LOG_MME_APP, "Could not create new handover procedure for UE with ue_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " in EMM_REGISTERED state. Rejecting further procedures. \n",
        handover_required_pP->mme_ue_s1ap_id, ue_context->privates.fields.imsi);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, S1AP_SYSTEM_FAILURE);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /*
   * Fill the target information, and use it if handover cancellation is received.
   * No transaction needed on source side.
   * Keep the source side as it is.
   */
  memcpy((void*)&s10_handover_procedure->target_tai, (void*)&handover_required_pP->selected_tai, sizeof(handover_required_pP->selected_tai));
  memcpy((void*)&s10_handover_procedure->target_ecgi, (void*)&handover_required_pP->global_enb_id, sizeof(handover_required_pP->global_enb_id)); /**< Home or macro enb id. */
  memcpy((void*)&s10_handover_procedure->imsi, &ue_nas_ctx->_imsi, sizeof(imsi_t));

  /** Set the eNB type. */
  //  s10_handover_procedure->target_enb_type = handover_required_pP->target_enb_type;

  /*
   * Set the source values, too. We might need it if the MME_STATUS_TRANSFER has to be sent after Handover Notify (emulator errors).
   * Also, this may be used when removing the old ue_reference later.
   */
  s10_handover_procedure->source_enb_ue_s1ap_id = ue_context->privates.fields.enb_ue_s1ap_id;
  s10_handover_procedure->source_ecgi = ue_context->privates.fields.e_utran_cgi;



  if(!ue_context->privates.fields.local_mme_teid_s10){
    /** Set the Source MME_S10_FTEID the same as in S11. */
    teid_t local_teid = 0x0;
    do{
      local_teid = (teid_t) (rand() % 0xFFFFFFFF);
    }while(mme_ue_context_exists_s10_teid(&mme_app_desc.mme_ue_contexts, local_teid) != NULL);
    OAI_GCC_DIAG_OFF(pointer-to-int-cast);
    forward_relocation_request_p->s10_source_mme_teid.teid = local_teid;
    OAI_GCC_DIAG_ON(pointer-to-int-cast);
    /**
     * Update the local_s10_key.
     * Not setting the key directly in the  ue_context structure. Only over this function!
     */
    mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
        ue_context->privates.enb_s1ap_id_key,
        ue_context->privates.mme_ue_s1ap_id,
        ue_context->privates.fields.imsi,
        ue_context->privates.fields.mme_teid_s11,       // mme_teid_s11 is new
        local_teid,       // set with forward_relocation_request!
        &ue_context->privates.fields.guti);
  }else{
    OAILOG_INFO (LOG_MME_APP, "A S10 Local TEID " TEID_FMT " already exists. Not reallocating for UE: %08x %d(dec)\n",
        ue_context->privates.fields.local_mme_teid_s10, handover_required_pP->mme_ue_s1ap_id, handover_required_pP->mme_ue_s1ap_id);
    OAI_GCC_DIAG_OFF(pointer-to-int-cast);
    forward_relocation_request_p->s10_source_mme_teid.teid = ue_context->privates.fields.local_mme_teid_s10;
    OAI_GCC_DIAG_ON(pointer-to-int-cast);
  }

  forward_relocation_request_p->s10_source_mme_teid.interface_type = S10_MME_GTP_C;
  /** Set the MME IPv4 and IPv6 addresses in the FTEID. */
  mme_config_read_lock (&mme_config);
  if(mme_config.ip.s10_mme_v4.s_addr){
	  forward_relocation_request_p->s10_source_mme_teid.ipv4_address = mme_config.ip.s10_mme_v4;
	  forward_relocation_request_p->s10_source_mme_teid.ipv4 = 1;
  }
  if(memcmp(&mme_config.ip.s10_mme_v6.s6_addr, (void*)&in6addr_any, sizeof(mme_config.ip.s10_mme_v6.s6_addr)) != 0) {
	  	  memcpy(forward_relocation_request_p->s10_source_mme_teid.ipv6_address.s6_addr, mme_config.ip.s10_mme_v6.s6_addr,  sizeof(mme_config.ip.s10_mme_v6.s6_addr));
	  	  forward_relocation_request_p->s10_source_mme_teid.ipv6 = 1;
  }
  mme_config_unlock (&mme_config);


  /** Set the SGW_S11_FTEID the same as in S11. */
  OAI_GCC_DIAG_OFF(pointer-to-int-cast);
  forward_relocation_request_p->s11_sgw_teid.teid = first_pdn->s_gw_teid_s11_s4;
  OAI_GCC_DIAG_ON(pointer-to-int-cast);
  forward_relocation_request_p->s11_sgw_teid.interface_type = S11_MME_GTP_C;
  /** Set the SGW CP IPv4 and IPv6 addresses from the PDN context. */
  pdn_context_t * pdn_context_to_forward = RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
  if(pdn_context_to_forward){
	  /** Set the PGW soure side CP IP addresses. */
	  if(pdn_context_to_forward->s_gw_addr_s11_s4.ipv4_addr.sin_addr.s_addr){
		  forward_relocation_request_p->s11_sgw_teid.ipv4_address = pdn_context_to_forward->s_gw_addr_s11_s4.ipv4_addr.sin_addr;
		  forward_relocation_request_p->s11_sgw_teid.ipv4 = 1;
	  }
	  if(memcmp(&pdn_context_to_forward->s_gw_addr_s11_s4.ipv6_addr.sin6_addr.s6_addr, (void*)&in6addr_any, sizeof(pdn_context_to_forward->s_gw_addr_s11_s4.ipv6_addr.sin6_addr.s6_addr)) != 0) {
		  memcpy(forward_relocation_request_p->s11_sgw_teid.ipv6_address.s6_addr, pdn_context_to_forward->s_gw_addr_s11_s4.ipv6_addr.sin6_addr.s6_addr,  sizeof(pdn_context_to_forward->s_gw_addr_s11_s4.ipv6_addr.sin6_addr.s6_addr));
		  forward_relocation_request_p->s11_sgw_teid.ipv6 = 1;
	  }
  }
  /** Set the F-Cause. */
  forward_relocation_request_p->f_cause.fcause_type      = FCAUSE_S1AP;
  forward_relocation_request_p->f_cause.fcause_s1ap_type = FCAUSE_S1AP_RNL;
  forward_relocation_request_p->f_cause.fcause_value     = (uint8_t)handover_required_pP->f_cause_value;

  /** Set the MCC. */
  forward_relocation_request_p->target_identification.mcc[0]      = handover_required_pP->selected_tai.plmn.mcc_digit1;
  forward_relocation_request_p->target_identification.mcc[1]      = handover_required_pP->selected_tai.plmn.mcc_digit2;
  forward_relocation_request_p->target_identification.mcc[2]      = handover_required_pP->selected_tai.plmn.mcc_digit3;
  /** Set the MNC. */
  forward_relocation_request_p->target_identification.mnc[0]      = handover_required_pP->selected_tai.plmn.mnc_digit1;
  forward_relocation_request_p->target_identification.mnc[1]      = handover_required_pP->selected_tai.plmn.mnc_digit2;
  forward_relocation_request_p->target_identification.mnc[2]      = handover_required_pP->selected_tai.plmn.mnc_digit3;
  /* Set the Target Id with the correct type. */
  forward_relocation_request_p->target_identification.target_type = handover_required_pP->target_enb_type;

  if(forward_relocation_request_p->target_identification.target_type == TARGET_ID_HOME_ENB_ID){
    forward_relocation_request_p->target_identification.target_id.home_enb_id.tac    = handover_required_pP->selected_tai.tac;
    forward_relocation_request_p->target_identification.target_id.home_enb_id.enb_id = handover_required_pP->global_enb_id.cell_identity.enb_id;
  }else{
    /** We assume macro enb id. */
    forward_relocation_request_p->target_identification.target_id.macro_enb_id.tac    = handover_required_pP->selected_tai.tac;
    forward_relocation_request_p->target_identification.target_id.macro_enb_id.enb_id = handover_required_pP->global_enb_id.cell_identity.enb_id;
  }

  /** Allocate and set the PDN_CONNECTIONS IE (for all bearers incl. all TFTs). */
  forward_relocation_request_p->pdn_connections = calloc(1, sizeof(struct mme_ue_eps_pdn_connections_s));
  mme_app_set_pdn_connections(forward_relocation_request_p->pdn_connections, ue_session_pool);

  /** Set the MM_UE_EPS_CONTEXT. */
  forward_relocation_request_p->ue_eps_mm_context = calloc(1, sizeof(mm_context_eps_t));
  mme_app_set_ue_eps_mm_context(forward_relocation_request_p->ue_eps_mm_context, ue_context, ue_session_pool, ue_nas_ctx);

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
    OAILOG_WARNING(LOG_MME_APP, "UE with ue_id " MME_UE_S1AP_ID_FMT " has not an ongoing handover procedure on the (source) MME side. Sending Cancel Ack. \n", ue_context->privates.mme_ue_s1ap_id);
    mme_app_send_s1ap_handover_cancel_acknowledge(handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->enb_ue_s1ap_id, handover_cancel_pP->assoc_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Check that the UE is in registered state. */
  if (ue_context->privates.fields.mm_state != UE_REGISTERED) {
    OAILOG_ERROR (LOG_MME_APP, "UE with ue_id " MME_UE_S1AP_ID_FMT " is not in UE_REGISTERED state on the (source) MME side. Sending Cancel Ack. \n", ue_context->privates.mme_ue_s1ap_id);
    mme_app_send_s1ap_handover_cancel_acknowledge(handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->enb_ue_s1ap_id, handover_cancel_pP->assoc_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /**
   * Staying in the same state (UE_REGISTERED).
   * If the GUTI is not found, first send a preparation failure, then implicitly detach.
   */
  emm_context = emm_data_context_get_by_guti (&_emm_data, &ue_context->privates.fields.guti);
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
    if(s10_handover_proc->target_enb_ue_s1ap_id != 0 && ue_context->privates.fields.enb_ue_s1ap_id != s10_handover_proc->target_enb_ue_s1ap_id){
      // todo: macro/home enb_id
      ue_description_t * ue_reference = s1ap_is_enb_ue_s1ap_id_in_list_per_enb(s10_handover_proc->target_enb_ue_s1ap_id, s10_handover_proc->target_ecgi.cell_identity.enb_id);
      if(ue_reference != NULL){
        /** UE Reference to the target eNB found. Sending a UE Context Release to the target MME BEFORE a HANDOVER_REQUEST_ACK arrives. */
        OAILOG_INFO(LOG_MME_APP, "Sending UE-Context-Release-Cmd to the target eNB %d for the UE-ID " MME_UE_S1AP_ID_FMT " and pending_enbUeS1apId " ENB_UE_S1AP_ID_FMT " (current enbUeS1apId) " ENB_UE_S1AP_ID_FMT ". \n.",
            s10_handover_proc->target_ecgi.cell_identity.enb_id, ue_context->privates.mme_ue_s1ap_id, s10_handover_proc->target_enb_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id);
        ue_context->privates.s1_ue_context_release_cause = S1AP_HANDOVER_CANCELLED;
        mme_app_itti_ue_context_release (ue_context->privates.mme_ue_s1ap_id, s10_handover_proc->target_enb_ue_s1ap_id, S1AP_HANDOVER_CANCELLED, s10_handover_proc->target_ecgi.cell_identity.enb_id);
        /**
         * No pending transparent container expected.
         * We directly respond with CANCEL_ACK, since when UE_CTX_RELEASE_CMPLT comes from targetEnb, we don't know if the current enb is target or source,
         * so we cannot decide there to send CANCEL_ACK without a flag.
         */
        OAILOG_FUNC_OUT (LOG_MME_APP);
      }else{
        OAILOG_INFO(LOG_MME_APP, "No target UE reference is established yet. No S1AP UE context removal needed for UE-ID " MME_UE_S1AP_ID_FMT ". Responding with HO_CANCEL_ACK back to enbUeS1apId " ENB_UE_S1AP_ID_FMT ". \n.",
            ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id);
      }
    }
    /** Send a HO-CANCEL-ACK to the source-MME. */
    mme_app_send_s1ap_handover_cancel_acknowledge(handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->enb_ue_s1ap_id, handover_cancel_pP->assoc_id);
    /** Delete the handover procedure. */
    // todo: aborting the handover procedure.
    mme_app_delete_s10_procedure_mme_handover(ue_context);
    /** Keeping the UE context as it is. */
    OAILOG_FUNC_OUT (LOG_MME_APP);
 }else{
    /* Inter MME procedure.
     *
     * Target-TAI was not in the current MME. Sending a S10 Context Release Request.
     *
     * It may be that, the TEID is some other (the old MME it was handovered before from here.
     * So we need to check the TAI and find the correct neighboring MME.#
     * todo: to skip this step, we might set it back to 0 after S10-Complete for the previous Handover.
     */
    // todo: currently only a single neighboring MME supported.
   /** Get the neighboring MME IP. */

   if (1) {
     // TODO prototype may change
//     mme_app_select_service(&s10_handover_proc->target_tai, &neigh_mme_ipv4_addr);
     //    session_request_p->peer_ip.in_addr = mme_config.ipv4.
     if(!s10_handover_proc->proc.peer_ip){
       /** Send a Handover Preparation Failure back. */
       mme_app_send_s1ap_handover_cancel_acknowledge(handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->enb_ue_s1ap_id, handover_cancel_pP->assoc_id);
       mme_app_delete_s10_procedure_mme_handover(ue_context);
       OAILOG_ERROR(LOG_MME_APP, "The selected TAI " TAI_FMT " is not configured as an S10 MME neighbor for UE. "
           "Accepting Handover cancel and removing handover procedure. \n",
           TAI_ARG(&s10_handover_proc->target_tai), s10_handover_proc->mme_ue_s1ap_id);
       MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_REQUIRED Unknown ue %u", handover_required_pP->mme_ue_s1ap_id);
       OAILOG_FUNC_OUT (LOG_MME_APP);
     }
   }

   message_p = itti_alloc_new_message (TASK_MME_APP, S10_RELOCATION_CANCEL_REQUEST);
   DevAssert (message_p != NULL);
   itti_s10_relocation_cancel_request_t *relocation_cancel_request_p = &message_p->ittiMsg.s10_relocation_cancel_request;
   memset ((void*)relocation_cancel_request_p, 0, sizeof (itti_s10_relocation_cancel_request_t));
   relocation_cancel_request_p->teid = s10_handover_proc->remote_mme_teid.teid; /**< May or may not be 0. */
   relocation_cancel_request_p->local_teid = ue_context->privates.fields.local_mme_teid_s10; /**< May or may not be 0. */
   // todo: check the table!
   memcpy((void*)&relocation_cancel_request_p->peer_ip, s10_handover_proc->proc.peer_ip,
 		  (s10_handover_proc->proc.peer_ip->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

   /** IMSI. */
   memcpy((void*)&relocation_cancel_request_p->imsi, &emm_context->_imsi, sizeof(imsi_t));
   MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 RELOCATION_CANCEL_REQUEST_MESSAGE");
   itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
   OAILOG_DEBUG(LOG_MME_APP, "Successfully sent S10 RELOCATION_CANCEL_REQUEST to the target MME for the UE with IMSI " IMSI_64_FMT " and id " MME_UE_S1AP_ID_FMT ". "
       "Continuing with HO-CANCEL PROCEDURE. \n", ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.imsi);
   /** Delete the handover procedure. */
   // todo: aborting the handover procedure.
//    mme_app_delete_s10_procedure_mme_handover(ue_context);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
  /*
   * Update the local S10 TEID.
   */
  mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
      INVALID_ENB_UE_S1AP_ID_KEY,
      ue_context->privates.mme_ue_s1ap_id,
      ue_context->privates.fields.imsi,      /**< New IMSI. */
      ue_context->privates.fields.mme_teid_s11,
      INVALID_TEID,
      &ue_context->privates.fields.guti);
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
void mme_app_send_s1ap_mme_status_transfer(mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id, uint32_t enb_id, status_transfer_bearer_list_t * status_transfer){
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
  status_transfer_p->status_transfer_bearer_list = status_transfer;
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
 struct ue_session_pool_s               *ue_session_pool 		= NULL;
 mme_app_s10_proc_mme_handover_t        *s10_proc_mme_handover 	= NULL;
 bearer_contexts_to_be_created_t         bcs_tbc;
 uint16_t encryption_algorithm_capabilities = (uint16_t)0;
 uint16_t integrity_algorithm_capabilities  = (uint16_t)0;
 int                                     rc = RETURNok;

 OAILOG_FUNC_IN (LOG_MME_APP);

 AssertFatal ((forward_relocation_request_pP->imsi.length > 0)
     && (forward_relocation_request_pP->imsi.length < 16), "BAD IMSI LENGTH %d", forward_relocation_request_pP->imsi.length);
 AssertFatal ((forward_relocation_request_pP->imsi.length > 0)
     && (forward_relocation_request_pP->imsi.length < 16), "STOP ON IMSI LENGTH %d", forward_relocation_request_pP->imsi.length);

 memset((void*)&bcs_tbc, 0, sizeof(bcs_tbc));

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
 target_tai.tac = forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac;

// if(1){
//	 mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, &forward_relocation_request_pP->peer_ip, forward_relocation_request_pP->trxn, REQUEST_REJECTED);
//	 OAILOG_FUNC_OUT (LOG_MME_APP);
// }

 /** Get the eNB Id. */
 target_type_t target_type = forward_relocation_request_pP->target_identification.target_type;
 uint32_t enb_id = 0;
 switch(forward_relocation_request_pP->target_identification.target_type){
   case TARGET_ID_MACRO_ENB_ID:
     enb_id = forward_relocation_request_pP->target_identification.target_id.macro_enb_id.enb_id;
     break;

   case TARGET_ID_HOME_ENB_ID:
     enb_id = forward_relocation_request_pP->target_identification.target_id.macro_enb_id.enb_id;
     break;

   default:
     OAILOG_DEBUG (LOG_MME_APP, "Target ENB type %d cannot be handovered to. Rejecting S10 handover request.. \n",
         forward_relocation_request_pP->target_identification.target_type);
     mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid,
         &forward_relocation_request_pP->peer_ip, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
     OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** Check the target-ENB is reachable. */
 if (mme_app_check_ta_local(&target_tai.plmn, forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac)) {
   /** The target PLMN and TAC are served by this MME. */
   OAILOG_DEBUG (LOG_MME_APP, "Target TAC " TAC_FMT " is served by current MME. \n", forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac);
   /*
    * Currently only a single TA will be served by each MME and we are expecting TAU from the UE side.
    * Check that the eNB is also served, that an SCTP association exists for the eNB.
    */
   if(s1ap_is_enb_id_in_list(enb_id) != NULL){
     OAILOG_DEBUG (LOG_MME_APP, "Target ENB_ID %u is served by current MME. \n", enb_id);
     /** Continue with the handover establishment. */
   }else{
     /** The target PLMN and TAC are not served by this MME. */
     OAILOG_ERROR(LOG_MME_APP, "Target ENB_ID %u is NOT served by the current MME. \n", enb_id);
     mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, &forward_relocation_request_pP->peer_ip, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
     OAILOG_FUNC_OUT (LOG_MME_APP);
   }
 }else{
   /** The target PLMN and TAC are not served by this MME. */
   OAILOG_ERROR(LOG_MME_APP, "TARGET TAC " TAC_FMT " is NOT served by current MME. \n", forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac);
   mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid,
		   &forward_relocation_request_pP->peer_ip, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
   /** No UE context or tunnel endpoint is allocated yet. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** We should only send the handover request and not deal with anything else. */
 if ((ue_context = get_new_ue_context()) == NULL) {
   /** Send a negative response before crashing. */
   mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, &forward_relocation_request_pP->peer_ip, forward_relocation_request_pP->trxn, SYSTEM_FAILURE);
   /**
    * Error during UE context malloc
    */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** Try to get a new UE session pool, too (before starting with the registrations). */
 /** Create a new UE session and register it. */
 ue_session_pool = get_new_session_pool(ue_context->privates.mme_ue_s1ap_id);
 if(!ue_session_pool){
   OAILOG_CRITICAL (LOG_MME_APP, "Could not get a new UE session pool for UE " MME_UE_S1AP_ID_FMT".\n", ue_context->privates.mme_ue_s1ap_id);
   /** Deallocate the ue context and remove from MME_APP map. */
   mme_remove_ue_context(&mme_app_desc.mme_ue_contexts, ue_context);
   /** Send back failure. */
   mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, &forward_relocation_request_pP->peer_ip, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }

 OAILOG_DEBUG (LOG_MME_APP, "MME_APP_INITIAL_UE_MESSAGE. Allocated new MME UE context and new mme_ue_s1ap_id. " MME_UE_S1AP_ID_FMT ". \n", ue_context->privates.mme_ue_s1ap_id);
 /** Register the new MME_UE context into the map. Don't register the IMSI yet. Leave it for the NAS layer. */

 imsi64_t imsi = imsi_to_imsi64(&forward_relocation_request_pP->imsi);
 teid_t teid = __sync_fetch_and_add (&mme_app_teid_generator, 0x00000001);
 /** Get the old context and invalidate its IMSI relation. */
 ue_context_t * old_ue_context = mme_ue_context_exists_imsi(&mme_app_desc.mme_ue_contexts, imsi);
 if(old_ue_context){
   // todo: any locks here?
   OAILOG_WARNING(LOG_MME_APP, "An old UE context with mmeUeId " MME_UE_S1AP_ID_FMT " already exists for IMSI " IMSI_64_FMT ". \n", old_ue_context->privates.mme_ue_s1ap_id, imsi);
   mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, old_ue_context,
        old_ue_context->privates.enb_s1ap_id_key,
        old_ue_context->privates.mme_ue_s1ap_id,
        INVALID_IMSI64,            /**< Invalidate the IMSI relationshop to the old UE context, nothing else.. */
        old_ue_context->privates.fields.mme_teid_s11,
        old_ue_context->privates.fields.local_mme_teid_s10,
        &old_ue_context->privates.fields.guti);
 }
 mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
      ue_context->privates.enb_s1ap_id_key,
      ue_context->privates.mme_ue_s1ap_id,
      imsi,            /**< Invalidate the IMSI relationship to the old UE context, nothing else.. */
	  teid,       // mme_s11_teid is new
      ue_context->privates.fields.local_mme_teid_s10,
      &ue_context->privates.fields.guti);

 /** Update the session keys of the UE session pool, too. */
 mme_ue_session_pool_update_coll_keys(&mme_app_desc.mme_ue_session_pools, ue_session_pool,
       ue_session_pool->privates.mme_ue_s1ap_id,
       teid       // mme_s11_teid is new
	   );

 /*
  * Create a new handover procedure, no matter a UE context exists or not.
  * Store the transaction in it.
  * Store all pending PDN connections in it.
  * Each time a CSResp for a specific APN arrives, send another CSReq if needed.
  */
 s10_proc_mme_handover = mme_app_create_s10_procedure_mme_handover(ue_context, true, MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER, &forward_relocation_request_pP->peer_ip);
 DevAssert(s10_proc_mme_handover);

 /*
  * If we make the procedure a specific procedure for forward relocation request, then it would be removed with forward relocation response,
  * todo: where to save the mm_eps_context and s10 tunnel information,
  * Else, if it is a generic s10 handover procedure, we need to update the transaction with every time.
  */
 /** Fill the values of the s10 handover procedure (and decouple them, such that remain after the ITTI message is removed). */
 memcpy((void*)&s10_proc_mme_handover->target_id, (void*)&forward_relocation_request_pP->target_identification, sizeof(target_identification_t));
 /** Set the IMSI. */
 memcpy((void*)&s10_proc_mme_handover->nas_s10_context._imsi, (void*)&forward_relocation_request_pP->imsi, sizeof(imsi_t));
 s10_proc_mme_handover->nas_s10_context.imsi = imsi;
 /** Set the target tai also in the target-MME side. */
 memcpy((void*)&s10_proc_mme_handover->target_tai, &target_tai, sizeof(tai_t));
 /** Set the subscribed AMBR values. */
 ue_session_pool->privates.fields.subscribed_ue_ambr.br_dl = forward_relocation_request_pP->ue_eps_mm_context->subscribed_ue_ambr.br_dl;
 ue_session_pool->privates.fields.subscribed_ue_ambr.br_ul = forward_relocation_request_pP->ue_eps_mm_context->subscribed_ue_ambr.br_ul;

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
 /** Set the eNB Id. */
 ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id = enb_id;

 s10_proc_mme_handover->pdn_connections = forward_relocation_request_pP->pdn_connections;
 forward_relocation_request_pP->pdn_connections = NULL; /**< Unlink the pdn_connections. */
 OAILOG_DEBUG(LOG_MME_APP, "UE_CONTEXT for UE " MME_UE_S1AP_ID_FMT " is a new UE_Context. Processing the received PDN_CONNECTIONS IEs (continuing with CSR). \n", ue_context->privates.mme_ue_s1ap_id);
 /** Process PDN Connections IE. Will initiate a Create Session Request message for the pending pdn_connections. */
 pdn_connection_t * pdn_connection = &s10_proc_mme_handover->pdn_connections->pdn_connection[0];
 pdn_context_t * pdn_context = mme_app_handle_pdn_connectivity_from_s10(ue_session_pool, pdn_connection);
 /*
  * When Create Session Response is received, continue to process the next PDN connection, until all are processed.
  * When all pdn_connections are completed, continue with handover request.
  */
 mme_app_send_s11_create_session_req (ue_context->privates.mme_ue_s1ap_id, &s10_proc_mme_handover->nas_s10_context._imsi, pdn_context, &target_tai, (protocol_configuration_options_t*)NULL, false);
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
/*
 * Send an S1AP Handover Request.
 */
static
void mme_app_send_s1ap_handover_request(mme_ue_s1ap_id_t mme_ue_s1ap_id,
    bearer_contexts_to_be_created_t *bcs_tbc,
    ambr_t                  *total_used_apn_ambr,
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
  handover_request_p->ambr.br_ul = total_used_apn_ambr->br_ul;
  handover_request_p->ambr.br_dl = total_used_apn_ambr->br_dl;

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
void mme_app_send_s1ap_handover_command(mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id, uint32_t enb_id,
    bearer_contexts_to_be_created_t * bcs_tbc, bstring target_to_source_cont){
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

  /** Set the bearer contexts to be created. Not changing any bearer state. */
  handover_command_p->bearer_ctx_to_be_forwarded_list = calloc(1, sizeof(bearer_contexts_to_be_created_t));
  memcpy((void*)handover_command_p->bearer_ctx_to_be_forwarded_list, bcs_tbc, sizeof(*bcs_tbc));

  /** Set the E-UTRAN Target-To-Source-Transparent-Container. */
  handover_command_p->eutran_target_to_source_container = target_to_source_cont;
  // todo: what will the enb_ue_s1ap_ids for single mme s1ap handover will be.. ?
  OAILOG_INFO(LOG_MME_APP, "Sending S1AP handover command to the source eNodeB for UE " MME_UE_S1AP_ID_FMT " to source enbId %d. \n", mme_ue_s1ap_id, enb_id);
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
  struct ue_session_pool_s               *ue_session_pool = NULL;
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

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, ue_context->privates.mme_ue_s1ap_id);
  if (ue_session_pool == NULL) {
      OAILOG_DEBUG (LOG_MME_APP, "We didn't find an UE session id for UE: " MME_UE_S1AP_ID_FMT".\n", ue_context->privates.mme_ue_s1ap_id);
      OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /*
   * Check that there is a s10 handover procedure.
   * If not, drop the message, don't care about target-MME.
   */
  mme_app_s10_proc_mme_handover_t * s10_handover_procedure = mme_app_get_s10_procedure_mme_handover(ue_context);
  if(!s10_handover_procedure){
    /** Deal with the error case. */
    OAILOG_ERROR(LOG_MME_APP, "No S10 Handover procedure for UE IMSI " IMSI_64_FMT " and mmeS1apUeId " MME_UE_S1AP_ID_FMT " in state %d. Dropping the ForwardRelResp. \n",
        ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.mm_state);
    /** No implicit detach. */
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  /** Check if bearers/pdn is present (optionally: relocation cancel request). */
  pdn_context_t * first_pdn = RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
  if(!first_pdn){
    OAILOG_ERROR(LOG_MME_APP, "UE context for IMSI " IMSI_64_FMT " and mmeS1apUeId " MME_UE_S1AP_ID_FMT " does not have any pdn/Bearer context. "
    		"Not continuing with handover commond (prep-failure) & sending relocation cancel request. \n",
        ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id);

    /** Send prep-failure to source enb. No need to send relocation cancel request to source. Timers should handle such abnormalities. */
    mme_app_send_s1ap_handover_preparation_failure(ue_context->privates.mme_ue_s1ap_id,
          ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.fields.sctp_assoc_id_key, RELOCATION_FAILURE);
    /** Not manually stopping the handover procedure.. implicit detach is assumed to be ongoing. */
    OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
  }
  if (forward_relocation_response_pP->cause.cause_value != REQUEST_ACCEPTED) {
    /**
     * We are in EMM-REGISTERED state, so we don't need to perform an implicit detach.
     * In the target side, we won't do anything. We assumed everything is taken care of (No Relocation Cancel Request to be sent).
     * No handover timers in the source-MME side exist.
     * We will only send an Handover Preparation message and leave the UE context as it is (including the S10 tunnel endpoint at the source-MME side).
     * The handover notify timer is only at the target-MME side. That's why, its not in the method below.
     */
    mme_app_send_s1ap_handover_preparation_failure(ue_context->privates.mme_ue_s1ap_id,
        ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.fields.sctp_assoc_id_key, RELOCATION_FAILURE);

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
  bearer_id = forward_relocation_response_pP->handovered_bearers.bearer_contexts[0].eps_bearer_id /* - 5 */ ;
  // todo: what is the dumping doing? printing? needed for s10?

  /**
   * Not doing/storing anything in the NAS layer. Just sending handover command back.
   * Send a S1AP Handover Command to the source eNodeB.
   */
  OAILOG_INFO(LOG_MME_APP, "MME_APP UE context is in REGISTERED state. Sending a Handover Command to the source-ENB with enbId: %d for UE with mmeUeS1APId : " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT " after S10 Forward Relocation Response. \n",
      ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.imsi);

  bearer_contexts_to_be_created_t bcs_tbf;
  memset((void*)&bcs_tbf, 0, sizeof(bcs_tbf));
  pdn_context_t * registered_pdn_ctx = NULL;
  RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_session_pool->pdn_contexts) {
    DevAssert(registered_pdn_ctx);
    mme_app_get_bearer_contexts_to_be_created(registered_pdn_ctx, &bcs_tbf, BEARER_STATE_NULL);
    /** The number of bearers will be incremented in the method. S10 should just pick the ebi. */
  }

  /** Send a Handover Command. */
  mme_app_send_s1ap_handover_command(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id,
      s10_handover_procedure->source_ecgi.cell_identity.enb_id,
      // ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id,
      &bcs_tbf,
      forward_relocation_response_pP->eutran_container.container_value);
  s10_handover_procedure->ho_command_sent = true;
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
    OAILOG_ERROR(LOG_MME_APP, "No handover process for UE " MME_UE_S1AP_ID_FMT ". \n", ue_context->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION local S10 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      forward_access_context_notification_pP->teid, ue_context->privates.fields.imsi);
  /** Send a S1AP MME Status Transfer Message the target eNodeB. */
  message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE);
  DevAssert (message_p != NULL);
  itti_s10_forward_access_context_acknowledge_t *s10_mme_forward_access_context_acknowledge_p =
      &message_p->ittiMsg.s10_forward_access_context_acknowledge;
  s10_mme_forward_access_context_acknowledge_p->teid        = s10_handover_process->remote_mme_teid.teid;  /**< Set the target TEID. */
  s10_mme_forward_access_context_acknowledge_p->local_teid  = ue_context->privates.fields.local_mme_teid_s10;   /**< Set the local TEID. */
  memcpy((void*)&s10_mme_forward_access_context_acknowledge_p->peer_ip, s10_handover_process->proc.peer_ip, s10_handover_process->proc.peer_ip->sa_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

  s10_mme_forward_access_context_acknowledge_p->trxn        = forward_access_context_notification_pP->trxn; /**< Set the target TEID. */
  /** Check that there is a pending handover process. */
  if(ue_context->privates.fields.mm_state != UE_UNREGISTERED){
	  OAILOG_ERROR(LOG_MME_APP, "UE with enb_ue_s1ap_id: " ENB_UE_S1AP_ID_FMT", mme_ue_s1ap_id. "MME_UE_S1AP_ID_FMT " was REGISTERED when S10 Forward Access Context Notification was received (tester-bug: ignoring).. \n",
			  s10_handover_process->target_enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);
  }
  /** Deal with the error case. */
  s10_mme_forward_access_context_acknowledge_p->cause.cause_value = REQUEST_ACCEPTED;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "MME_APP Sending S10 FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE.");
  itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);

  /** Send a S1AP MME Status Transfer Message the target eNodeB. */
  // todo: macro/home enb id
  mme_app_send_s1ap_mme_status_transfer(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, s10_handover_process->target_id.target_id.macro_enb_id.enb_id,
      forward_access_context_notification_pP->status_transfer_bearer_list);
  /** Unlink it from the message. */
  forward_access_context_notification_pP->status_transfer_bearer_list = NULL;

  /*
   * Setting the ECM state to ECM_CONNECTED with Handover Request Acknowledge.
   */
// todo: DevAssert(ue_context->privates.fields.ecm_state == ECM_CONNECTED); /**< Any timeouts here should erase the context, not change the signaling state back to IDLE but leave the context. */
  OAILOG_INFO(LOG_MME_APP, "Sending S1AP MME Status transfer to the target eNodeB %d for UE with enb_ue_s1ap_id: " ENB_UE_S1AP_ID_FMT", mme_ue_s1ap_id. "MME_UE_S1AP_ID_FMT ". \n",
      s10_handover_process->target_id.target_id.macro_enb_id.enb_id, s10_handover_process->target_enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);

  /** (Tester malfunctions : Handover Notify is received). */
  if(s10_handover_process->received_early_ho_notify){
	  /** Trigger the Ho-Relocation Complete message. */
	  message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_RELOCATION_COMPLETE_NOTIFICATION);
	  DevAssert (message_p != NULL);
	  itti_s10_forward_relocation_complete_notification_t *forward_relocation_complete_notification_p = &message_p->ittiMsg.s10_forward_relocation_complete_notification;
	  /** Set the destination TEID. */
	  forward_relocation_complete_notification_p->teid = s10_handover_process->remote_mme_teid.teid;       /**< Target S10-MME TEID. todo: what if multiple? */
	  /** Set the local TEID. */
	  forward_relocation_complete_notification_p->local_teid = ue_context->privates.fields.local_mme_teid_s10;        /**< Local S10-MME TEID. */
	  memcpy((void*)&forward_relocation_complete_notification_p->peer_ip, s10_handover_process->proc.peer_ip,
			  s10_handover_process->proc.peer_ip->sa_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

	  OAILOG_INFO(LOG_MME_APP, "Sending FW_RELOC_COMPLETE_NOTIF TO %X with remote S10-TEID " TEID_FMT ". \n.",
			  forward_relocation_complete_notification_p->peer_ip, forward_relocation_complete_notification_p->teid);

	  // todo: remove this and set at correct position!
	  mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_CONNECTED);

	  /**
	   * Sending a message to S10. Not changing any context information!
	   * This message actually implies that the handover is finished. Resetting the flags and statuses here of after Forward Relocation Complete AcknowledgE?! (MBR)
	   */
	  itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
  } else {
	  s10_handover_process->mme_status_context_handled = true;
  }
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
      forward_access_context_acknowledge_pP->teid, ue_context->privates.fields.imsi);

  /** Check that there is a handover process. */
  mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
  if(!s10_handover_proc){
    /** Deal with the error case. */
    OAILOG_ERROR(LOG_MME_APP, "UE MME context with IMSI " IMSI_64_FMT " and mmeS1apUeId: " MME_UE_S1AP_ID_FMT " is not in UE_REGISTERED state, instead %d, when S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE is received. "
        "Ignoring the error and waiting remove the UE contexts triggered by HSS. \n",
        ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.mm_state);
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
 struct ue_context_s                    *ue_context 	 = NULL;
 struct ue_session_pool_s               *ue_session_pool = NULL;

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

 ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_session_pools, handover_request_acknowledge_pP->mme_ue_s1ap_id);
 if (ue_session_pool == NULL) {
   OAILOG_ERROR(LOG_MME_APP, "No MME_APP UE session pool for the UE with mmeUeS1APId : " MME_UE_S1AP_ID_FMT ". \n", handover_request_acknowledge_pP->mme_ue_s1ap_id);
   /** Ignore the message. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
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
      ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id);
   /** Remove the created UE reference. */
   ue_context->privates.s1_ue_context_release_cause = S1AP_SYSTEM_FAILURE;
   mme_app_itti_ue_context_release(handover_request_acknowledge_pP->mme_ue_s1ap_id, handover_request_acknowledge_pP->enb_ue_s1ap_id, ue_context->privates.s1_ue_context_release_cause, ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id);
   /** Ignore the message. Set the UE to idle mode. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 AssertFatal(NULL == s10_handover_proc->source_to_target_eutran_f_container.container_value, "TODO clean pointer");
 /*
  * Check bearers, which could not be established in the target cell (from the ones, remaining after the CSReq to target SAE-GW).
  * We don't actually need this step, but just do it to be sure. We are not using the ebi_list.
  */
 memset(&s10_handover_proc->failed_ebi_list, 0, sizeof(ebi_list_t));
 mme_app_release_bearers(ue_context->privates.mme_ue_s1ap_id, &handover_request_acknowledge_pP->e_rab_release_list, &s10_handover_proc->failed_ebi_list);
 /*
  * For both cases, update the S1U eNB FTEIDs and the bearer state.
  */
 s10_handover_proc->target_enb_ue_s1ap_id = handover_request_acknowledge_pP->enb_ue_s1ap_id;
 mme_app_modify_bearers(ue_context->privates.mme_ue_s1ap_id, &handover_request_acknowledge_pP->bcs_to_be_modified);
 // todo: lionel--> checking the type or different success_notif methods?
 //   s10_handover_proc->success_notif(ue_context, handover_request_acknowledge_pP->target_to_source_eutran_container);
 if(s10_handover_proc->proc.type == MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER) {
   /** Send Handover Command to the source enb without changing any parameters in the MME_APP UE context. */
   if(ue_context->privates.fields.mm_state == UE_UNREGISTERED){
     /** If the UE is unregistered, discard the message. */
     OAILOG_WARNING(LOG_MME_APP, "Discarding intra HO-Request Ack for unregistered UE: " MME_UE_S1AP_ID_FMT " and enbUeS1apId " ENB_UE_S1AP_ID_FMT ". \n", handover_request_acknowledge_pP->mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id);
     OAILOG_FUNC_OUT (LOG_MME_APP);
   }
   OAILOG_INFO(LOG_MME_APP, "Intra-MME S10 Handover procedure is ongoing. Sending a Handover Command to the source-ENB with enbId: %d for UE with mmeUeS1APId : " MME_UE_S1AP_ID_FMT " and enbUeS1apId " ENB_UE_S1AP_ID_FMT ". \n",
       ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id, handover_request_acknowledge_pP->mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id);
   /**
    * Bearer will be in inactive state till HO-Notify triggers MBR.
    * Save the new ENB_UE_S1AP_ID
    * Don't update the coll_keys with the new enb_ue_s1ap_id.
    */
   bearer_contexts_to_be_created_t bcs_tbf;
   memset((void*)&bcs_tbf, 0, sizeof(bcs_tbf));
   pdn_context_t * registered_pdn_ctx = NULL;
   RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_session_pool->pdn_contexts) {
	   DevAssert(registered_pdn_ctx);
	   mme_app_get_bearer_contexts_to_be_created(registered_pdn_ctx, &bcs_tbf, BEARER_STATE_NULL);
	   /** The number of bearers will be incremented in the method. S10 should just pick the ebi. */
   }

   /** Check if there are any bearer contexts to be removed.. add them into the procedure. */
   mme_app_send_s1ap_handover_command(handover_request_acknowledge_pP->mme_ue_s1ap_id,
       ue_context->privates.fields.enb_ue_s1ap_id,
       s10_handover_proc->source_ecgi.cell_identity.enb_id,
       &bcs_tbf,
       handover_request_acknowledge_pP->target_to_source_eutran_container);
   s10_handover_proc->ho_command_sent = true;
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
   DevAssert(ue_context->privates.fields.mm_state == UE_UNREGISTERED); /**< NAS invalidation should have set this to UE_UNREGISTERED. */
    /*
    * Set the enb_ue_s1ap_id in this case to the UE_Context, too.
    * todo: just let it in the s10_handover_procedure.
    */
   ue_context->privates.fields.enb_ue_s1ap_id = handover_request_acknowledge_pP->enb_ue_s1ap_id;
   /*
    * Update the enb_id_s1ap_key and register it.
    */
   enb_s1ap_id_key_t                       enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;
   // Todo; do it for home
   MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, s10_handover_proc->target_id.target_id.macro_enb_id.enb_id, handover_request_acknowledge_pP->enb_ue_s1ap_id);
   /*
    * Update the coll_keys with the new s1ap parameters.
    */
   mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
       enb_s1ap_id_key,     /**< New key. */
       ue_context->privates.mme_ue_s1ap_id,
       ue_context->privates.fields.imsi,
       ue_context->privates.fields.mme_teid_s11,
       ue_context->privates.fields.local_mme_teid_s10,
       &ue_context->privates.fields.guti);

   OAILOG_INFO(LOG_MME_APP, "Inter-MME S10 Handover procedure is ongoing. Sending a Forward Relocation Response message to source-MME for ueId: " MME_UE_S1AP_ID_FMT " and enbUeS1apId " ENB_UE_S1AP_ID_FMT ". \n",
        handover_request_acknowledge_pP->mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id);

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
       ue_context->privates.enb_s1ap_id_key,   /**< Not updated. */
       ue_context->privates.mme_ue_s1ap_id,
       ue_context->privates.fields.imsi,
       ue_context->privates.fields.mme_teid_s11,       // mme_teid_s11 is new
       local_teid,       // set with forward_relocation_response!
       &ue_context->privates.fields.guti);            /**< No guti exists
   /*
    * UE is in UE_UNREGISTERED state. Assuming inter-MME S1AP Handover was triggered.
    * Sending FW_RELOCATION_RESPONSE.
    */
   mme_app_itti_forward_relocation_response(ue_context, &ue_session_pool->pdn_contexts, s10_handover_proc, handover_request_acknowledge_pP->target_to_source_eutran_container);
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
 if(ue_context->privates.fields.mm_state == UE_REGISTERED){
   /**
    * UE is registered, we assume in this case that the source-MME is also attached to the current.
    * In this case, we need to re-notify the MME_UE_S1AP_ID<->SCTP association, because it might be removed with the error handling.
    */
   notify_s1ap_new_ue_mme_s1ap_id_association (ue_context->privates.fields.sctp_assoc_id_key, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);
   /** We assume a valid enb & sctp id in the UE_Context. */
   mme_app_send_s1ap_handover_preparation_failure(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.fields.sctp_assoc_id_key, S1AP_HANDOVER_FAILED);
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

 /** Set a local TEID. */
 if(!ue_context->privates.fields.local_mme_teid_s10){
   /** Set the Source MME_S10_FTEID the same as in S11. */
   teid_t local_teid = 0x0;
   do{
     local_teid = (teid_t) (rand() % 0xFFFFFFFF);
   }while(mme_ue_context_exists_s10_teid(&mme_app_desc.mme_ue_contexts, local_teid) != NULL);

   mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
       ue_context->privates.enb_s1ap_id_key,
       ue_context->privates.mme_ue_s1ap_id,
       ue_context->privates.fields.imsi,
       ue_context->privates.fields.mme_teid_s11,       // mme_teid_s11 is new
       local_teid,       // set with forward_relocation_request!
       &ue_context->privates.fields.guti);
 }else{
   OAILOG_INFO (LOG_MME_APP, "A S10 Local TEID " TEID_FMT " already exists. Not reallocating for UE: %08x %d(dec)\n",
       ue_context->privates.fields.local_mme_teid_s10, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);
 }

 /**
  * UE is in UE_UNREGISTERED state. Assuming inter-MME S1AP Handover was triggered.
  * Sending FW_RELOCATION_RESPONSE with error code and implicit detach.
  */
 mme_app_send_s10_forward_relocation_response_err(s10_handover_proc->remote_mme_teid.teid, s10_handover_proc->proc.peer_ip, s10_handover_proc->forward_relocation_trxn, RELOCATION_FAILURE);
 /** Trigger an implicit detach. */
 mme_app_delete_s10_procedure_mme_handover(ue_context);
 message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
 DevAssert (message_p != NULL);
 message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->privates.mme_ue_s1ap_id;
 itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
 /** No timers, etc. is needed. */
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_s1ap_error_indication(const itti_s1ap_error_indication_t * const s1ap_error_indication_pP    ){
  struct ue_context_s                    *ue_context = NULL;
  uint64_t                                imsi = 0;

  OAILOG_FUNC_IN (LOG_MME_APP);

  OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_ERROR_INDICATION from eNB for UE_ID " MME_UE_S1AP_ID_FMT ". \n", s1ap_error_indication_pP->mme_ue_s1ap_id);

  /** Don't check the UE conetxt. */
  OAILOG_ERROR(LOG_MME_APP, "Sending UE Context release request for UE context with mmeS1apUeId " MME_UE_S1AP_ID_FMT". \n", s1ap_error_indication_pP->mme_ue_s1ap_id);

  /** No matter if handover procedure exists or not, perform an UE context release. */
  mme_app_itti_ue_context_release(s1ap_error_indication_pP->mme_ue_s1ap_id, s1ap_error_indication_pP->enb_ue_s1ap_id, S1AP_NAS_NORMAL_RELEASE, s1ap_error_indication_pP->enb_id);

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
   OAILOG_WARNING(LOG_MME_APP, "No S10 handover procedure exists for UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT". \n",
		   s1ap_status_transfer_pP->mme_ue_s1ap_id);
   /**
    * We don't really expect an error at this point. Just forward the message to the target enb. (happens just with ng4t).
    * Header on both bearer items.
    */
   /** No need to unlink here. */
   // todo: macro/home
   mme_app_send_s1ap_mme_status_transfer(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id,
		   s1ap_status_transfer_pP->status_transfer_bearer_list);
   s1ap_status_transfer_pP->status_transfer_bearer_list = NULL;
   /** eNB-Status-Transfer message message will be freed. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 if(s10_handover_proc->proc.type == MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER){
   /**
    * Set the ENB of the pending target-eNB.
    * Even if the HANDOVER_NOTIFY messaged is received simultaneously, the pending enb_ue_s1ap_id field should stay.
    * We do not check that the target-eNB exists. We did not modify any contexts.
    * Concatenate with header (todo: OAI: how to do this properly?)
    */
   /** No need to unlink here. */
   // todo: macro/home
   mme_app_send_s1ap_mme_status_transfer(ue_context->privates.mme_ue_s1ap_id, s10_handover_proc->target_enb_ue_s1ap_id, s10_handover_proc->target_ecgi.cell_identity.enb_id,
		   s1ap_status_transfer_pP->status_transfer_bearer_list);
   s1ap_status_transfer_pP->status_transfer_bearer_list = NULL;
   /** eNB-Status-Transfer message message will be freed. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }else{
   if (s1ap_status_transfer_pP->status_transfer_bearer_list == NULL){
	   OAILOG_ERROR (LOG_MME_APP, " NULL UE transparent container\n" );
	   OAILOG_FUNC_OUT (LOG_MME_APP);
   }
   /* UE is DEREGISTERED. Assuming that it came from S10 inter-MME handover. Forwarding the eNB status information to the target-MME via Forward Access Context Notification. */
   message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION);
   DevAssert (message_p != NULL);
   itti_s10_forward_access_context_notification_t *forward_access_context_notification_p = &message_p->ittiMsg.s10_forward_access_context_notification;
   memset ((void*)forward_access_context_notification_p, 0, sizeof (itti_s10_forward_access_context_notification_t));
   /** Set the target S10 TEID. */
   forward_access_context_notification_p->teid           = s10_handover_proc->remote_mme_teid.teid; /**< Only a single target-MME TEID can exist at a time. */
   forward_access_context_notification_p->local_teid     = ue_context->privates.fields.local_mme_teid_s10; /**< Only a single target-MME TEID can exist at a time. */
   memcpy((void*)&forward_access_context_notification_p->peer_ip, s10_handover_proc->proc.peer_ip,
    		  (s10_handover_proc->proc.peer_ip->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

   /** Set the E-UTRAN container. */
   forward_access_context_notification_p->status_transfer_bearer_list = s1ap_status_transfer_pP->status_transfer_bearer_list;
   /** Unlink. */
   s1ap_status_transfer_pP->status_transfer_bearer_list = NULL;

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
   mme_app_itti_ue_context_release(handover_notify_pP->mme_ue_s1ap_id, handover_notify_pP->enb_ue_s1ap_id, S1AP_SYSTEM_FAILURE, handover_notify_pP->cgi.cell_identity.enb_id);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /**
  * Check that there is an s10 handover process running, if not discard the message.
  */
 mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
 if(!s10_handover_proc){
   OAILOG_ERROR(LOG_MME_APP, "No Handover Procedure is runnning for UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT" in state %d. Discarding the message\n", ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.mm_state);
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
 ue_context->privates.fields.sctp_assoc_id_key = handover_notify_pP->assoc_id;
 ue_context->privates.fields.e_utran_cgi.cell_identity.cell_id = handover_notify_pP->cgi.cell_identity.cell_id;
 ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id  = handover_notify_pP->cgi.cell_identity.enb_id;
 /** Update the enbUeS1apId (again). */
 ue_context->privates.fields.enb_ue_s1ap_id = handover_notify_pP->enb_ue_s1ap_id; /**< Updating the enb_ue_s1ap_id here. */
 // regenerate the enb_s1ap_id_key as enb_ue_s1ap_id is changed.
 MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, handover_notify_pP->cgi.cell_identity.enb_id, handover_notify_pP->enb_ue_s1ap_id);

 /**
  * Update the coll_keys with the new s1ap parameters.
  */
 mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
     enb_s1ap_id_key,     /**< New key. */
     ue_context->privates.mme_ue_s1ap_id,
     ue_context->privates.fields.imsi,
     ue_context->privates.fields.mme_teid_s11,
     ue_context->privates.fields.local_mme_teid_s10,
     &ue_context->privates.fields.guti);

 OAILOG_INFO(LOG_MME_APP, "Setting UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT " to enbUeS1apId " ENB_UE_S1AP_ID_FMT" @handover_notify. \n", ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id);

 /**
  * This will overwrite the association towards the old eNB if single MME S1AP handover.
  * The old eNB will be referenced by the enb_ue_s1ap_id.
  */
 notify_s1ap_new_ue_mme_s1ap_id_association (ue_context->privates.fields.sctp_assoc_id_key, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.mme_ue_s1ap_id);

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
   /**
    * Complete the registration of the UE.
    * This should trigger an activation of the bearers.
    */
   mme_app_mobility_complete(ue_context->privates.mme_ue_s1ap_id, true);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }else{
   OAILOG_DEBUG(LOG_MME_APP, "UE MME context with imsi " IMSI_64_FMT " and mmeS1apUeId " MME_UE_S1AP_ID_FMT " has successfully completed inter-MME handover process after HANDOVER_NOTIFY. \n",
       ue_context->privates.fields.imsi, handover_notify_pP->mme_ue_s1ap_id);
   /**
    * UE came from S10 inter-MME handover. Not clear the pending_handover state yet.
    * Sending Forward Relocation Complete Notification and waiting for acknowledgment.
    */
   /** Only if MME_Status Context has been received (tester malfunctions). */
   if(s10_handover_proc->mme_status_context_handled){
	   message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_RELOCATION_COMPLETE_NOTIFICATION);
	      DevAssert (message_p != NULL);
	      itti_s10_forward_relocation_complete_notification_t *forward_relocation_complete_notification_p = &message_p->ittiMsg.s10_forward_relocation_complete_notification;
	      /** Set the destination TEID. */
	      forward_relocation_complete_notification_p->teid = s10_handover_proc->remote_mme_teid.teid;       /**< Target S10-MME TEID. todo: what if multiple? */
	      /** Set the local TEID. */
	      forward_relocation_complete_notification_p->local_teid = ue_context->privates.fields.local_mme_teid_s10;        /**< Local S10-MME TEID. */
	      memcpy((void*)&forward_relocation_complete_notification_p->peer_ip, s10_handover_proc->proc.peer_ip,
	        		  (s10_handover_proc->proc.peer_ip->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

	      // todo: remove this and set at correct position!
	      mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_CONNECTED);

	      /**
	       * Sending a message to S10. Not changing any context information!
	       * This message actually implies that the handover is finished. Resetting the flags and statuses here of after Forward Relocation Complete AcknowledgE?! (MBR)
	       */
	      itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
   } else{
	   /** Late Ho-Notify. */
	   s10_handover_proc->received_early_ho_notify = true;
   }
 }
}

//------------------------------------------------------------------------------
void
mme_app_handle_s1ap_enb_configuration_transfer(
     itti_s1ap_configuration_transfer_t * const const enb_conf_transfer_pP
    )
{
 MessageDef                             *message_p  = NULL;

 OAILOG_FUNC_IN (LOG_MME_APP);

 /** Check if the target eNB is local or remote. */
 if (mme_app_check_ta_local(&enb_conf_transfer_pP->target_tai.plmn, enb_conf_transfer_pP->target_tai.tac)) {
   /** Check if the eNB with the given eNB-ID is served. */
   if(s1ap_is_enb_id_in_list(enb_conf_transfer_pP->target_global_enb_id.cell_identity.enb_id) != NULL){
	   OAILOG_DEBUG (LOG_MME_APP, "Target ENB_ID %d of target TAI " TAI_FMT " is served by current MME. Forwarding eNB configuration transfer. \n",
			   enb_conf_transfer_pP->target_global_enb_id.cell_identity.enb_id, TAI_ARG(&enb_conf_transfer_pP->target_tai));

	   /** ENB is local, directly forward the request to the target eNB. */
	   mme_app_send_s1ap_mme_configuration_transfer(enb_conf_transfer_pP->target_enb_type, &enb_conf_transfer_pP->target_tai,
			   &enb_conf_transfer_pP->target_global_enb_id,
			   enb_conf_transfer_pP->source_enb_type, &enb_conf_transfer_pP->source_tai,
			   &enb_conf_transfer_pP->source_global_enb_id,
			   enb_conf_transfer_pP->conf_reply);
	   enb_conf_transfer_pP->conf_reply = NULL; /**< Unlink. */
	   OAILOG_FUNC_OUT (LOG_MME_APP);
   } else {
     /*
      * Create a new handover procedure and begin processing.
      */
	   OAILOG_ERROR(LOG_MME_APP, "The selected eNB Id %d is not known for local TAI " TAI_FMT " is not configured as an S10 MME neighbor. "
			   "Not proceeding with the enb configuration transfer. \n", enb_conf_transfer_pP->target_global_enb_id.cell_identity.enb_id,
			   TAI_ARG(&enb_conf_transfer_pP->target_tai));
	   OAILOG_FUNC_OUT (LOG_MME_APP);
   }
 } else {
	 /** Get the target MME. */
	 struct sockaddr *neigh_mme_ip_addr = NULL;
	 if (1) {
		 // TODO prototype may change
	    mme_app_select_service(&enb_conf_transfer_pP->target_tai, &neigh_mme_ip_addr, S10_MME_GTP_C);
	    //    session_request_p->peer_ip.in_addr = mme_config.ipv4.
	    if(!neigh_mme_ip_addr){
	      OAILOG_ERROR(LOG_MME_APP, "The selected TAI " TAI_FMT " is not configured as an S10 MME neighbor. "
	          "Not proceeding with the enb configuration transfer. \n", TAI_ARG(&enb_conf_transfer_pP->target_tai));
	      OAILOG_FUNC_OUT (LOG_MME_APP);
	    }
	    OAILOG_INFO(LOG_MME_APP, "Target ENB_ID %d of target TAI " TAI_FMT " is served by current MME. Forwarding eNB configuration transfer. \n",
	    		enb_conf_transfer_pP->target_global_enb_id.cell_identity.enb_id, TAI_ARG(&enb_conf_transfer_pP->target_tai));
	    OAILOG_WARNING(LOG_MME_APP, "ENB CONFIG TRANSFER VIA S10 NOT IMPLEMENTED YET!\n"); // todo: implement? fcontainer?
	  }
	 OAILOG_FUNC_OUT (LOG_MME_APP);
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
     forward_relocation_complete_notification_pP->teid, ue_context->privates.fields.imsi);

 /** Check that there is a pending handover process. */
 mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
 if(!s10_handover_proc || s10_handover_proc->proc.type == MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER){
   /** Deal with the error case. */
   OAILOG_ERROR(LOG_MME_APP, "UE MME context with IMSI " IMSI_64_FMT " and mmeS1apUeId: " MME_UE_S1AP_ID_FMT " in state %d has not a valid INTER-MME S10 procedure running. "
       "Ignoring received S10_FORWARD_RELOCATION_COMPLETE_NOTIFICATION. \n", ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.mm_state);
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
 forward_relocation_complete_acknowledge_p->local_teid = ue_context->privates.fields.local_mme_teid_s10;      /**< Local S10-MME TEID. */
 /** Set the cause. */
 forward_relocation_complete_acknowledge_p->cause.cause_value      = REQUEST_ACCEPTED;                       /**< Check the cause.. */
 /** Set the peer IP. */
// forward_relocation_complete_acknowledge_p->peer_ip = s10_handover_proc->proc.peer_ip; /**< Set the target TEID. */
 /** Set the transaction. */
 forward_relocation_complete_acknowledge_p->trxn = forward_relocation_complete_notification_pP->trxn; /**< Set the target TEID. */
 itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
 /** ECM is in connected state.. UE will be detached implicitly. */
 ue_context->privates.s1_ue_context_release_cause = S1AP_SUCCESSFUL_HANDOVER; /**< How mapped to correct radio-Network cause ?! */

 /**
  * Start timer to wait the handover/TAU procedure to complete.
  * A Clear_Location_Request message received from the HSS will cause the resources to be removed.
  * If it was not a handover but a context request/response (TAU), the MME_MOBILITY_COMPLETION timer will be started here, else @ FW-RELOC-COMPLETE @ Handover.
  * Resources will not be removed if that is not received (todo: may it not be received or must it always come
  * --> TS.23.401 defines for SGSN "remove after CLReq" explicitly).
  */
 ue_description_t * old_ue_reference = s1ap_is_enb_ue_s1ap_id_in_list_per_enb(ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id);
 if(old_ue_reference){
   /** Stop the timer of the handover procedure first. */
   if (s10_handover_proc->proc.timer.id != MME_APP_TIMER_INACTIVE_ID) {
     if (timer_remove(s10_handover_proc->proc.timer.id, NULL)) {
       OAILOG_ERROR (LOG_MME_APP, "Failed to stop handover procedure timer for the INTRA-MME handover for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
     }
     s10_handover_proc->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;
   }
   /*
    * Start the timer of the ue-reference and also set it to the procedure (only if target-mme for inter-MME S1ap handover).
    * Timeout will occur in S1AP layer.
    */
   if (timer_setup (mme_config.mme_mobility_completion_timer, 0,
       TASK_S1AP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *)ue_context->privates.enb_s1ap_id_key, &(old_ue_reference->s1ap_handover_completion_timer.id)) < 0) {
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
       ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id);
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
  * Complete the handover procedure (register).
  * We will again enter the method when TAU is complete.
  */
 mme_app_mobility_complete(ue_context->privates.mme_ue_s1ap_id, true);
 /** S1AP inter-MME handover is complete. */
 OAILOG_INFO(LOG_MME_APP, "UE_Context with IMSI " IMSI_64_FMT " and mmeUeS1apId: " MME_UE_S1AP_ID_FMT " successfully completed INTER-MME (S10) handover procedure! \n",
     ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id);
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_mobile_reachability_timer_expiry (struct ue_context_s *ue_context)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context != NULL);
  ue_context->privates.mobile_reachability_timer.id = MME_APP_TIMER_INACTIVE_ID;
  OAILOG_INFO (LOG_MME_APP, "Expired- Mobile Reachability Timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
  // Start Implicit Detach timer
  if (timer_setup (ue_context->privates.implicit_detach_timer.sec, 0,
                TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *)&(ue_context->privates.mme_ue_s1ap_id), &(ue_context->privates.implicit_detach_timer.id)) < 0) {
    OAILOG_ERROR (LOG_MME_APP, "Failed to start Implicit Detach timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
    ue_context->privates.implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;
  } else {
    OAILOG_DEBUG (LOG_MME_APP, "Started Implicit Detach timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
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
  OAILOG_INFO (LOG_MME_APP, "Expired- Implicit Detach timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
  ue_context->privates.implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;

  // Initiate Implicit Detach for the UE
  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
  DevAssert (message_p != NULL);
  message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->privates.mme_ue_s1ap_id;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
  itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_initial_context_setup_rsp_timer_expiry (struct ue_context_s *ue_context)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context != NULL);
  MessageDef                             *message_p = NULL;
  OAILOG_INFO (LOG_MME_APP, "Expired- Initial context setup rsp timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
  ue_context->privates.initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  /* *********Abort the ongoing procedure*********
   * Check if UE is registered already that implies service request procedure is active. If so then release the S1AP
   * context and move the UE back to idle mode. Otherwise if UE is not yet registered that implies attach procedure is
   * active. If so,then abort the attach procedure and release the UE context.
   */
  ue_context->privates.s1_ue_context_release_cause = S1AP_INITIAL_CONTEXT_SETUP_FAILED;
  if (ue_context->privates.fields.mm_state == UE_UNREGISTERED) {
    // Initiate Implicit Detach for the UE
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->privates.mme_ue_s1ap_id;
    itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
  } else {
    // Release S1-U bearer and move the UE to idle mode
    mme_app_send_s11_release_access_bearers_req(ue_context->privates.mme_ue_s1ap_id);
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
  if (ue_context->privates.initial_context_setup_rsp_timer.id != MME_APP_TIMER_INACTIVE_ID) {
    if (timer_remove(ue_context->privates.initial_context_setup_rsp_timer.id, NULL)) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to stop Initial Context Setup Rsp timer for UE id  %d \n", ue_context->privates.mme_ue_s1ap_id);
    }
    ue_context->privates.initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  }
  /* *********Abort the ongoing procedure*********
   * Check if UE is registered already that implies service request procedure is active. If so then release the S1AP
   * context and move the UE back to idle mode. Otherwise if UE is not yet registered that implies attach procedure is
   * active. If so,then abort the attach procedure and release the UE context.
   */
  ue_context->privates.s1_ue_context_release_cause = S1AP_INITIAL_CONTEXT_SETUP_FAILED;
  if (ue_context->privates.fields.mm_state == UE_UNREGISTERED) {
    // Initiate Implicit Detach for the UE
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context->privates.mme_ue_s1ap_id;
    itti_send_msg_to_task (TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
  } else {
    // Release S1-U bearer and move the UE to idle mode
    mme_app_send_s11_release_access_bearers_req(initial_ctxt_setup_failure_pP->mme_ue_s1ap_id);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

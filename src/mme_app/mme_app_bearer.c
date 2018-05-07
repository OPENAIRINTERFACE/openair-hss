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
#include "mme_app_sgw_selection.h"
#include "mme_app_bearer_context.h"
#include "common_defs.h"
#include "gcc_diag.h"
#include "mme_app_itti_messaging.h"
#include "mme_app_procedures.h"
#include "xml_msg_dump_itti.h"


//----------------------------------------------------------------------------
// todo: check which one needed
//static void mme_app_send_s1ap_path_switch_request_acknowledge(mme_ue_s1ap_id_t mme_ue_s1ap_id);
//
//static void mme_app_send_s1ap_path_switch_request_failure(mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id, sctp_assoc_id_t assoc_id, MMECause_t mmeCause);
//
//static void mme_app_send_s1ap_handover_request(mme_ue_s1ap_id_t mme_ue_s1ap_id, uint32_t                enb_id,
//    uint16_t                encryption_algorithm_capabilities,
//    uint16_t                integrity_algorithm_capabilities,
//    uint8_t                 nh[AUTH_NH_SIZE],
//    uint8_t                 ncc);
//
///** External definitions in MME_APP UE Data Context. */
//extern int mme_app_set_ue_eps_mm_context(mm_context_eps_t * ue_eps_mme_context_p, struct ue_context_s *ue_context_p, emm_data_context_t *ue_nas_ctx);
//extern int mme_app_set_pdn_connections(struct mme_ue_eps_pdn_connections_s * pdn_connections, struct ue_context_s * ue_context_p);
//extern void mme_app_handle_pending_pdn_connectivity_information(ue_context_t *ue_context_p, pdn_connection_t * pdn_conn_pP);
//extern bearer_context_t* mme_app_create_new_bearer_context(ue_context_t *ue_context_p, ebi_t bearer_id);

//------------------------------------------------------------------------------
static bool mme_app_construct_guti(const plmn_t * const plmn_p, const as_stmsi_t * const s_tmsi_p,  guti_t * const guti_p)
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
    struct ue_context_s *ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, enb_dereg_ind->mme_ue_s1ap_id[ue_idx]);

    if (ue_context_p) {
      ue_context_p->ecm_state = ECM_IDLE;
      ue_context_p->is_s1_ue_context_release = false;
      mme_app_send_nas_signalling_connection_rel_ind(enb_dereg_ind->mme_ue_s1ap_id[ue_idx]);
    }
  }
}

/**
 * Callback method called if UE is registered.
 * This method could be also put somewhere else.
 */
int EmmCbS1apDeregistered(mme_ue_s1ap_id_t ueId){

  MessageDef                    *message_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);

  /** Find the UE context. */
  ue_context_t * ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ueId);
  DevAssert(ue_context_p); /**< Should always exist. Any mobility issue in which this could occur? */

  OAILOG_INFO(LOG_MME_APP, "Entered callback handler for UE-DEREGISTERED state of UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT ". Not implicitly removing resources with state change "
      "(must send an EMM_AS signal with ESTABLISH_REJ.. etc.). \n", ueId);

  /*
   * Reset the flags of the UE.
   */
  ue_context->subscription_known = SUBSCRIPTION_UNKNOWN;
  if(ue_context->pending_clear_location_request){
    OAILOG_INFO(LOG_MME_APP, "Clearing the pending CLR for UE id " MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
    ue_context->pending_clear_location_request = false;
  }




  OAILOG_FUNC_OUT (LOG_MME_APP);

}

/**
 * Callback method called if UE is registered.
 * This method could be also put somewhere else.
 */
int EmmCbS1apRegistered(mme_ue_s1ap_id_t ueId){

  MessageDef                    *message_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);

  /** Find the UE context. */
  ue_context_t * ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ueId);
  DevAssert(ue_context_p); /**< Should always exist. Any mobility issue in which this could occur? */

  OAILOG_INFO(LOG_MME_APP, "Entered callback handler for UE-REGISTERED state of UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT ". \n", ueId);
  /** Trigger a Create Session Request. */
  imsi64_t                                imsi64 = INVALID_IMSI64;
  int                                     rc = RETURNok;

  /**
   * Consider the UE authenticated. */
  ue_context_p->imsi_auth = IMSI_AUTHENTICATED;

  /** Check if there is a pending deactivation flag is set. */
  if(ue_context_p->pending_bearer_deactivation){
    OAILOG_INFO(LOG_MME_APP, "After UE entered UE_REGISTERED state, initiating bearer deactivation for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT ". \n", ueId);
    ue_context_p->pending_bearer_deactivation = false;
    ue_context_p->ue_context_rel_cause = S1AP_NAS_NORMAL_RELEASE;
    /** Reset any pending downlink bearers. */
    memset(&ue_context_p->pending_s1u_downlink_bearer, 0, sizeof(ue_context_p)->pending_s1u_downlink_bearer);
    ue_context_p->pending_s1u_downlink_bearer_ebi = 0;

    // Notify S1AP to send UE Context Release Command to eNB.
    mme_app_itti_ue_context_release (ue_context_p->mme_ue_s1ap_id, ue_context_p->enb_ue_s1ap_id, ue_context_p->ue_context_rel_cause, ue_context_p->e_utran_cgi.cell_identity.enb_id);
  }else{
    /** No pending bearer deactivation. Check if there is a pending downlink bearer and send the DL-GTP Tunnel Information to the SAE-GW. */
    if(ue_context_p->pending_s1u_downlink_bearer.teid != (teid_t)0){
      message_p = itti_alloc_new_message (TASK_MME_APP, S11_MODIFY_BEARER_REQUEST);
         AssertFatal (message_p , "itti_alloc_new_message Failed");
         itti_s11_modify_bearer_request_t *s11_modify_bearer_request = &message_p->ittiMsg.s11_modify_bearer_request;
         memset ((void *)s11_modify_bearer_request, 0, sizeof (itti_s11_modify_bearer_request_t));
         s11_modify_bearer_request->peer_ip = mme_config.ipv4.sgw_s11;
         s11_modify_bearer_request->teid = ue_context_p->sgw_s11_teid;
         s11_modify_bearer_request->local_teid = ue_context_p->mme_s11_teid;
         /*
          * Delay Value in integer multiples of 50 millisecs, or zero
          */
         // todo: multiple bearers!
         s11_modify_bearer_request->delay_dl_packet_notif_req = 0;  // TO DO
         s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].eps_bearer_id = ue_context_p->pending_s1u_downlink_bearer_ebi;
         memcpy (&s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].s1_eNB_fteid,
             &ue_context_p->pending_s1u_downlink_bearer,
             sizeof (ue_context_p->pending_s1u_downlink_bearer));
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

         /** Reset any pending downlink bearers. */
         memset(&ue_context_p->pending_s1u_downlink_bearer, 0, sizeof(ue_context_p)->pending_s1u_downlink_bearer);
         ue_context_p->pending_s1u_downlink_bearer_ebi = 0;
    }else{
      OAILOG_INFO(LOG_MME_APP, "No downlink S10-eNB-TEID is set for mmeUeS1apId " MME_UE_S1AP_ID_FMT ". \n", ueId);

    }
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
int
mme_app_handle_nas_pdn_connectivity_req (
  itti_nas_pdn_connectivity_req_t * const nas_pdn_connectivity_req_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                 *ue_context_p = NULL;
  imsi64_t                                imsi64 = INVALID_IMSI64;
  int                                     rc = RETURNok;

  DevAssert (nas_pdn_connectivity_req_pP );
  IMSI_STRING_TO_IMSI64 ((char *)nas_pdn_connectivity_req_pP->imsi, &imsi64);
  OAILOG_DEBUG (LOG_MME_APP, "Received NAS_PDN_CONNECTIVITY_REQ from NAS Handling imsi " IMSI_64_FMT "\n", imsi64);

  if ((ue_context_p = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, imsi64)) == NULL) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " NAS_PDN_CONNECTIVITY_REQ Unknown imsi " IMSI_64_FMT, imsi64);
    OAILOG_ERROR (LOG_MME_APP, "That's embarrassing as we don't know this IMSI\n");
    mme_ue_context_dump_coll_keys();
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

//  if (!ue_context_p->is_s1_ue_context_release) {
    // ...
    ue_context_p->imsi_auth = IMSI_AUTHENTICATED;

    rc =  mme_app_send_s11_create_session_req (ue_context_p, nas_pdn_connectivity_req_pP->pdn_cid);
//  }
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

// todo: nas pdn connectivity request handling (the old one, we may also do this inside MME_APP without asking ESM layer.
////------------------------------------------------------------------------------
//int
//mme_app_handle_nas_pdn_connectivity_req (
//  itti_nas_pdn_connectivity_req_t * const nas_pdn_connectivity_req_pP)
//{
//  struct ue_context_s                    *ue_context_p = NULL;
//  imsi64_t                                imsi64 = INVALID_IMSI64;
//  int                                     rc = RETURNok;
//
//  OAILOG_FUNC_IN (LOG_MME_APP);
//  DevAssert (nas_pdn_connectivity_req_pP );
//  IMSI_STRING_TO_IMSI64 ((char *)nas_pdn_connectivity_req_pP->imsi, &imsi64);
//  OAILOG_DEBUG (LOG_MME_APP, "Received NAS_PDN_CONNECTIVITY_REQ from NAS Handling imsi " IMSI_64_FMT "\n", imsi64);
//
//  if ((ue_context_p = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, imsi64)) == NULL) {
//    MSC_LOG_EVENT (MSC_MMEAPP_MME, "NAS_PDN_CONNECTIVITY_REQ Unknown imsi " IMSI_64_FMT, imsi64);
//    OAILOG_ERROR (LOG_MME_APP, "That's embarrassing as we don't know this IMSI\n");
//    mme_ue_context_dump_coll_keys();
//    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
//  }
//
//  /**
//   * Consider the UE authenticated
//   * todo: done here?!
//   */
//  ue_context_p->imsi_auth = IMSI_AUTHENTICATED;
//
//  /** Not entering this state in case its not handover (assumed). */
//  // Temp: save request, in near future merge wisely params in context
//  memset (ue_context_p->pending_pdn_connectivity_req_imsi, 0, 16);
//  AssertFatal ((nas_pdn_connectivity_req_pP->imsi_length > 0)
//      && (nas_pdn_connectivity_req_pP->imsi_length < 16), "BAD IMSI LENGTH %d", nas_pdn_connectivity_req_pP->imsi_length);
//  AssertFatal ((nas_pdn_connectivity_req_pP->imsi_length > 0)
//      && (nas_pdn_connectivity_req_pP->imsi_length < 16), "STOP ON IMSI LENGTH %d", nas_pdn_connectivity_req_pP->imsi_length);
//  memcpy (ue_context_p->pending_pdn_connectivity_req_imsi, nas_pdn_connectivity_req_pP->imsi, nas_pdn_connectivity_req_pP->imsi_length);
//  ue_context_p->pending_pdn_connectivity_req_imsi_length = nas_pdn_connectivity_req_pP->imsi_length;
//
//  // copy
//  if (ue_context_p->pending_pdn_connectivity_req_apn) {
//    bdestroy (ue_context_p->pending_pdn_connectivity_req_apn);
//    ue_context_p->pending_pdn_connectivity_req_apn = NULL;
//  }
//  ue_context_p->pending_pdn_connectivity_req_apn =  nas_pdn_connectivity_req_pP->apn;
//  nas_pdn_connectivity_req_pP->apn = NULL;
//
//  // copy
//  if (ue_context_p->pending_pdn_connectivity_req_pdn_addr) {
//    bdestroy (ue_context_p->pending_pdn_connectivity_req_pdn_addr);
//  }
//  ue_context_p->pending_pdn_connectivity_req_pdn_addr =  nas_pdn_connectivity_req_pP->pdn_addr;
//  nas_pdn_connectivity_req_pP->pdn_addr = NULL;
//
//  ue_context_p->pending_pdn_connectivity_req_pti = nas_pdn_connectivity_req_pP->pti;
//  ue_context_p->pending_pdn_connectivity_req_ue_id = nas_pdn_connectivity_req_pP->ue_id;
//  copy_protocol_configuration_options (&ue_context_p->pending_pdn_connectivity_req_pco, &nas_pdn_connectivity_req_pP->pco);
//  clear_protocol_configuration_options(&nas_pdn_connectivity_req_pP->pco);
//#define TEMPORARY_DEBUG 1
//#if TEMPORARY_DEBUG
//  bstring b = protocol_configuration_options_to_xml(&ue_context_p->pending_pdn_connectivity_req_pco);
//  OAILOG_DEBUG (LOG_MME_APP, "PCO %s\n", bdata(b));
//  bdestroy(b);
//#endif
//
//  memcpy (&ue_context_p->pending_pdn_connectivity_req_qos, &nas_pdn_connectivity_req_pP->qos, sizeof (network_qos_t));
//  ue_context_p->pending_pdn_connectivity_req_proc_data = nas_pdn_connectivity_req_pP->proc_data;
//  nas_pdn_connectivity_req_pP->proc_data = NULL;
//  ue_context_p->pending_pdn_connectivity_req_request_type = nas_pdn_connectivity_req_pP->request_type;
//  //if ((nas_pdn_connectivity_req_pP->apn.value == NULL) || (nas_pdn_connectivity_req_pP->apn.length == 0)) {
//  /*
//   * TODO: Get keys...
//   */
//  /*
//   * Now generate S6A ULR
//   */
//  rc =  mme_app_send_s6a_update_location_req (ue_context_p);
//  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
//}


// sent by NAS
//------------------------------------------------------------------------------
void
mme_app_handle_conn_est_cnf (
  itti_nas_conn_est_cnf_t * const nas_conn_est_cnf_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;
  itti_mme_app_connection_establishment_cnf_t *establishment_cnf_p = NULL;

  OAILOG_DEBUG (LOG_MME_APP, "Received NAS_CONNECTION_ESTABLISHMENT_CNF from NAS\n");
  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, nas_conn_est_cnf_pP->ue_id);

  if (ue_context_p == NULL) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " NAS_CONNECTION_ESTABLISHMENT_CNF Unknown ue " MME_UE_S1AP_ID_FMT " ", nas_conn_est_cnf_pP->ue_id);
    OAILOG_ERROR (LOG_MME_APP, "UE context doesn't exist for UE " MME_UE_S1AP_ID_FMT "\n", nas_conn_est_cnf_pP->ue_id);
    // memory leak
    bdestroy_wrapper(&nas_conn_est_cnf_pP->nas_msg);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

//  if (!ue_context_p->is_s1_ue_context_release) {
    message_p = itti_alloc_new_message (TASK_MME_APP, MME_APP_CONNECTION_ESTABLISHMENT_CNF);
    establishment_cnf_p = &message_p->ittiMsg.mme_app_connection_establishment_cnf;

    establishment_cnf_p->ue_id = nas_conn_est_cnf_pP->ue_id;
    // todo: check all memcpy (&establishment_cnf_p->nas_conn_est_cnf, nas_conn_est_cnf_pP, sizeof (itti_nas_conn_est_cnf_t));


    // Copy UE radio capabilities into message if it exists
//      OAILOG_DEBUG (LOG_MME_APP, "UE radio context already cached: %s\n",
//                   ue_context_p->ue_radio_cap_length ? "yes" : "no");
//      establishment_cnf_p->ue_radio_cap_length = ue_context_p->ue_radio_cap_length;
//      if (establishment_cnf_p->ue_radio_cap_length) {
//        establishment_cnf_p->ue_radio_capabilities =
//                    (uint8_t*) calloc (establishment_cnf_p->ue_radio_cap_length, sizeof *establishment_cnf_p->ue_radio_capabilities);
//        memcpy (establishment_cnf_p->ue_radio_capabilities,
//                ue_context_p->ue_radio_capabilities,
//                establishment_cnf_p->ue_radio_cap_length);
//      }

    int j = 0;
    for (int i = 0; i < BEARERS_PER_UE; i++) {
      bearer_context_t *bc = ue_context_p->bearer_contexts[i];
      if (bc) {
        if (BEARER_STATE_SGW_CREATED & bc->bearer_state) {
          establishment_cnf_p->e_rab_id[j]                                 = bc->ebi ;//+ EPS_BEARER_IDENTITY_FIRST;
          establishment_cnf_p->e_rab_level_qos_qci[j]                      = bc->qci;
          establishment_cnf_p->e_rab_level_qos_priority_level[j]           = bc->priority_level;
          establishment_cnf_p->e_rab_level_qos_preemption_capability[j]    = bc->preemption_capability;
          establishment_cnf_p->e_rab_level_qos_preemption_vulnerability[j] = bc->preemption_vulnerability;
          establishment_cnf_p->transport_layer_address[j]                  = fteid_ip_address_to_bstring(&bc->s_gw_fteid_s1u);
          establishment_cnf_p->gtp_teid[j]                                 = bc->s_gw_fteid_s1u.teid;
          if (!j) {
            establishment_cnf_p->nas_pdu[j]                                = nas_conn_est_cnf_pP->nas_msg;
            nas_conn_est_cnf_pP->nas_msg = NULL;
          }
          j=j+1;
        }
      }
    }
    establishment_cnf_p->no_of_e_rabs = j;

  //#pragma message  "Check ue_context_p ambr"
    establishment_cnf_p->ue_ambr.br_ul = ue_context_p->suscribed_ue_ambr.br_ul;
    establishment_cnf_p->ue_ambr.br_dl = ue_context_p->suscribed_ue_ambr.br_dl;
    establishment_cnf_p->ue_security_capabilities_encryption_algorithms = nas_conn_est_cnf_pP->encryption_algorithm_capabilities;
    establishment_cnf_p->ue_security_capabilities_integrity_algorithms = nas_conn_est_cnf_pP->integrity_algorithm_capabilities;
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

    // todo: we don't move the UE in connected state, it is moved into connected state once at the beginning @ initial context setup request..

// todo: timer setup for initial context setup rsp && setting ECM state? (can use same timer for handover)? timer inside another process?
//    /*
//      * Move the UE to ECM Connected State.However if S1-U bearer establishment fails then we need to move the UE to idle.
//      * S1 Signaling connection gets established via first DL NAS Trasnport message in some scenarios so check the state
//      * first
//      */
//     if (ue_context_p->ecm_state != ECM_CONNECTED)  /**< It may be that ATTACH_ACCEPT is set directly or when the UE goes back from IDLE mode to active mode.
//     Else, the first downlink message sets the UE state to connected, which don't does registration, just stops any inactivity timer.
//     Deactivation always should remove the ENB_ID key. */
//     {
//       mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts,ue_context_p,ECM_CONNECTED);
//     }
//
//     /* Start timer to wait for Initial UE Context Response from eNB
//      * If timer expires treat this as failure of ongoing procedure and abort corresponding NAS procedure such as ATTACH
//      * or SERVICE REQUEST. Send UE context release command to eNB
//      */
//     if (timer_setup (ue_context_p->initial_context_setup_rsp_timer.sec, 0,
//                   TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *) &(ue_context_p->mme_ue_s1ap_id), &(ue_context_p->initial_context_setup_rsp_timer.id)) < 0) {
//       OAILOG_ERROR (LOG_MME_APP, "Failed to start initial context setup response timer for UE id  %d \n", ue_context_p->mme_ue_s1ap_id);
//       ue_context_p->initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
//     } else {
//       OAILOG_DEBUG (LOG_MME_APP, "MME APP : Sent Initial context Setup Request and Started guard timer for UE id  %d \n", ue_context_p->mme_ue_s1ap_id);
//     }



//  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

// sent by S1AP
//------------------------------------------------------------------------------
void
mme_app_handle_initial_ue_message (
  itti_s1ap_initial_ue_message_t * const initial_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                 *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;
  bool                                    is_guti_valid = false;
  emm_data_context_t                     *ue_nas_ctx = NULL;
    enb_s1ap_id_key_t                     enb_s1ap_id_key = 0;

  OAILOG_DEBUG (LOG_MME_APP, "Received MME_APP_INITIAL_UE_MESSAGE from S1AP\n");
  XML_MSG_DUMP_ITTI_S1AP_INITIAL_UE_MESSAGE(initial_pP, TASK_S1AP, TASK_MME_APP, NULL);



  // todo: this is an error case, we don't expect to find an MME_APP context via enb_ue_s1ap_id. If we find an S1AP UE reference, we will remove it.
//  MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, initial_pP->ecgi.cell_identity.enb_id, initial_pP->enb_ue_s1ap_id);
//  ue_context_p = mme_ue_context_exists_enb_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, enb_s1ap_id_key);
//  if (!(ue_context_p)) {
//    OAILOG_DEBUG (LOG_MME_APP, "Unknown enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT "\n", initial_pP->enb_ue_s1ap_id);
//  }
//
//  if ((ue_context_p) && (ue_context_p->is_s1_ue_context_release)) {
//    OAILOG_FUNC_OUT (LOG_MME_APP);
//  }

  // Check if there is any existing UE context using S-TMSI/GUTI. If so continue with its MME_APP context.
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
        ue_context_p =  mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts,ue_nas_ctx->ue_id);
        DevAssert(ue_context_p != NULL);
        if ((ue_context_p != NULL) && (ue_context_p->mme_ue_s1ap_id == ue_nas_ctx->ue_id)) {
          initial_pP->mme_ue_s1ap_id = ue_nas_ctx->ue_id;
          if (ue_context_p->enb_s1ap_id_key != INVALID_ENB_UE_S1AP_ID_KEY)
          {
            /** Check if there is a pending removal of the object. */
            /** If a CLR flag is existing, reset it. */
            // todo: check for a process on the source side
            if(ue_context_p->pending_clear_location_request){
              OAILOG_INFO(LOG_MME_APP, "UE_CONTEXT already had a CLR flag set for UE with IMSI " IMSI_64_FMT " and " MME_UE_S1AP_ID_FMT " already. Resetting. \n", ue_context_p->imsi, ue_context_p->mme_ue_s1ap_id);
              ue_context_p->pending_clear_location_request = false;
            }

            /*
             * This only removed the MME_UE_S1AP_ID from enb_s1ap_id_key, it won't remove the UE_REFERENCE itself.
             * todo: @ lionel:           duplicate_enb_context_detected  flag is not checked anymore (NAS).
             */
            ue_description_t * old_ue_reference = s1ap_is_enb_ue_s1ap_id_in_list_per_enb(ue_context_p->enb_ue_s1ap_id, ue_context_p->e_utran_cgi.cell_identity.enb_id);
            if(old_ue_reference){
              OAILOG_ERROR (LOG_MME_APP, "MME_APP_INITAIL_UE_MESSAGE. ERROR***** Found an old UE_REFERENCE with enbUeS1apId " ENB_UE_S1AP_ID_FMT " and enbId %d.\n" , old_ue_reference->enb_ue_s1ap_id,
                  ue_context_p->e_utran_cgi.cell_identity.enb_id);
              s1ap_remove_ue(old_ue_reference);
              OAILOG_WARNING (LOG_MME_APP, "MME_APP_INITAIL_UE_MESSAGE. ERROR***** Removed old UE_REFERENCE with enbUeS1apId " ENB_UE_S1AP_ID_FMT " and enbId %d.\n" , old_ue_reference->enb_ue_s1ap_id,
                  ue_context_p->e_utran_cgi.cell_identity.enb_id);
            }
            /*
             * Ideally this should never happen. When UE move to IDLE this key is set to INVALID.
             * Note - This can happen if eNB detects RLF late and by that time UE sends Initial NAS message via new RRC
             * connection.
             * However if this key is valid, remove the key from the hashtable.
             */
            hashtable_rc_t result_deletion = hashtable_ts_remove (mme_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl, (const hash_key_t)ue_context_p->enb_s1ap_id_key, (void **)&id);
            OAILOG_ERROR (LOG_MME_APP, "MME_APP_INITAIL_UE_MESSAGE. ERROR***** enb_s1ap_id_key %ld has valid value %ld. Result of deletion %d.\n" ,
                ue_context_p->enb_s1ap_id_key,
                ue_context_p->enb_ue_s1ap_id,
                result_deletion);
            ue_context_p->enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;
          }
          // Update MME UE context with new enb_ue_s1ap_id
          ue_context_p->enb_ue_s1ap_id = initial_pP->enb_ue_s1ap_id;
          // regenerate the enb_s1ap_id_key as enb_ue_s1ap_id is changed.
          // todo: also here home_enb_id
          MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, initial_pP->ecgi.cell_identity.enb_id, initial_pP->enb_ue_s1ap_id);
          // Update enb_s1ap_id_key in hashtable
          mme_ue_context_update_coll_keys( &mme_app_desc.mme_ue_contexts,
                ue_context_p,
                enb_s1ap_id_key,
                ue_nas_ctx->ue_id,
                ue_nas_ctx->_imsi64,
                ue_context_p->mme_teid_s11,
                ue_context_p->local_mme_teid_s10,
                &guti);
        }
      } else {
          OAILOG_DEBUG (LOG_MME_APP, "MME_APP_INITIAL_UE_MESSAGE with mme code %u and S-TMSI %u:"
            "no UE context found \n", initial_pP->opt_s_tmsi.mme_code, initial_pP->opt_s_tmsi.m_tmsi);
          /** Check that also no MME_APP UE context exists for the given GUTI. */
          DevAssert(mme_ue_context_exists_guti(&mme_app_desc.mme_ue_contexts, &guti) == NULL);
      }
    } else {
      OAILOG_DEBUG (LOG_MME_APP, "No MME is configured with MME code %u received in S-TMSI %u from UE.\n",
                    initial_pP->opt_s_tmsi.mme_code, initial_pP->opt_s_tmsi.m_tmsi);
      DevAssert(mme_ue_context_exists_guti(&mme_app_desc.mme_ue_contexts, &guti) == NULL);
    }
  } else {
    OAILOG_DEBUG (LOG_MME_APP, "MME_APP_INITIAL_UE_MESSAGE from S1AP,without S-TMSI. \n"); /**< Continue with new UE context establishment. */
  }

//  if (duplicate_enb_context_detected) {
//    if (is_initial) {
//      // remove new context
//      ue_context = mme_api_duplicate_enb_ue_s1ap_id_detected (enb_ue_s1ap_id_key,ue_context->mme_ue_s1ap_id, REMOVE_NEW_CONTEXT);
//      duplicate_enb_context_detected = false; // Problem solved
//      OAILOG_TRACE (LOG_NAS_EMM,
//          "EMM-PROC  - ue_context now enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
//          ue_context->enb_ue_s1ap_id, ue_context->mme_ue_s1ap_id);
//    }
//  }
  // finally create a new ue context if anything found
  if (!(ue_context_p)) {
    OAILOG_DEBUG (LOG_MME_APP, "UE context doesn't exist -> create one \n");
    if (!(ue_context_p = mme_create_new_ue_context ())) {
      /*
       * Error during ue context malloc
       */
      OAILOG_ERROR (LOG_MME_APP, "Failed to create new MME UE context enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT "\n", initial_pP->enb_ue_s1ap_id);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
    OAILOG_DEBUG (LOG_MME_APP, "Created new MME UE context enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT "\n", initial_pP->enb_ue_s1ap_id);
    /** Since the NAS and MME_APP contexts are split again, we assign a new mme_ue_s1ap_id here. */
    ue_context_p->mme_ue_s1ap_id    = mme_app_ctx_get_new_ue_id ();
    if (ue_context_p->mme_ue_s1ap_id  == INVALID_MME_UE_S1AP_ID) {
      OAILOG_CRITICAL (LOG_MME_APP, "MME_APP_INITIAL_UE_MESSAGE. MME_UE_S1AP_ID allocation Failed.\n");
      mme_remove_ue_context (&mme_app_desc.mme_ue_contexts, ue_context_p);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
    OAILOG_DEBUG (LOG_MME_APP, "MME_APP_INITAIL_UE_MESSAGE.Allocated new MME UE context and new mme_ue_s1ap_id. %d\n",ue_context_p->mme_ue_s1ap_id);
    ue_context_p->enb_ue_s1ap_id    = initial_pP->enb_ue_s1ap_id;
    MME_APP_ENB_S1AP_ID_KEY(ue_context_p->enb_s1ap_id_key, initial_pP->ecgi.cell_identity.enb_id, initial_pP->enb_ue_s1ap_id);
    DevAssert (mme_insert_ue_context (&mme_app_desc.mme_ue_contexts, ue_context_p) == 0);

    ue_context_p->ecm_state         = ECM_CONNECTED;
    ue_context_p->mme_ue_s1ap_id    = INVALID_MME_UE_S1AP_ID;
    ue_context_p->enb_ue_s1ap_id    = initial_pP->enb_ue_s1ap_id;
    MME_APP_ENB_S1AP_ID_KEY(ue_context_p->enb_s1ap_id_key, initial_pP->ecgi.cell_identity.enb_id, initial_pP->enb_ue_s1ap_id);
    ue_context_p->sctp_assoc_id_key = initial_pP->sctp_assoc_id;

    if (RETURNerror == mme_insert_ue_context (&mme_app_desc.mme_ue_contexts, ue_context_p)) {
      mme_app_ue_context_free_content(ue_context_p);
      free_wrapper ((void**)&ue_context_p);
      OAILOG_ERROR (LOG_MME_APP, "Failed to insert new MME UE context enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT "\n", initial_pP->enb_ue_s1ap_id);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
  }
  ue_context_p->e_utran_cgi = initial_pP->ecgi;
// todo: where to set the timers and notify the s1ap association?
  // Notify S1AP about the mapping between mme_ue_s1ap_id and sctp assoc id + enb_ue_s1ap_id
  notify_s1ap_new_ue_mme_s1ap_id_association (ue_context_p->sctp_assoc_id_key, ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id);
  // Initialize timers to INVALID IDs
  ue_context_p->mobile_reachability_timer.id = MME_APP_TIMER_INACTIVE_ID;
  ue_context_p->implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;
  ue_context_p->initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  ue_context_p->initial_context_setup_rsp_timer.sec = MME_APP_INITIAL_CONTEXT_SETUP_RSP_TIMER_VALUE;

  /** Inform the NAS layer about the new initial UE context. */
  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_INITIAL_UE_MESSAGE);
  // do this because of same message types name but not same struct in different .h
  message_p->ittiMsg.nas_initial_ue_message.nas.ue_id           = INVALID_MME_UE_S1AP_ID;
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

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_INITIAL_UE_MESSAGE ue id " MME_UE_S1AP_ID_FMT " ", ue_context_p->mme_ue_s1ap_id);
  itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_erab_setup_req (itti_erab_setup_req_t * const itti_erab_setup_req)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    *ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, itti_erab_setup_req->ue_id);

  if (!ue_context_p) {
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " NAS_ERAB_SETUP_REQ Unknown ue " MME_UE_S1AP_ID_FMT " ", itti_erab_setup_req->ue_id);
    OAILOG_ERROR (LOG_MME_APP, "UE context doesn't exist for UE " MME_UE_S1AP_ID_FMT "\n", itti_erab_setup_req->ue_id);
    // memory leak
    bdestroy_wrapper(&itti_erab_setup_req->nas_msg);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  if (!ue_context_p->is_s1_ue_context_release) {
    bearer_context_t* bearer_context = mme_app_get_bearer_context(ue_context_p, itti_erab_setup_req->ebi);

    if (bearer_context) {
      MessageDef  *message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_E_RAB_SETUP_REQ);
      itti_s1ap_e_rab_setup_req_t *s1ap_e_rab_setup_req = &message_p->ittiMsg.s1ap_e_rab_setup_req;

      s1ap_e_rab_setup_req->mme_ue_s1ap_id = ue_context_p->mme_ue_s1ap_id;
      s1ap_e_rab_setup_req->enb_ue_s1ap_id = ue_context_p->enb_ue_s1ap_id;

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
          ue_context_p->mme_ue_s1ap_id,
          s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].e_rab_id,
          s1ap_e_rab_setup_req->e_rab_to_be_setup_list.item[0].gtp_teid);
      int to_task = (RUN_MODE_SCENARIO_PLAYER == mme_config.run_mode) ? TASK_MME_SCENARIO_PLAYER:TASK_S1AP;
      itti_send_msg_to_task (to_task, INSTANCE_DEFAULT, message_p);
    } else {
      OAILOG_DEBUG (LOG_MME_APP, "No bearer context found ue " MME_UE_S1AP_ID_FMT  " ebi %u\n", itti_erab_setup_req->ue_id, itti_erab_setup_req->ebi);
    }
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_delete_session_rsp (
  const itti_s11_delete_session_response_t * const delete_sess_resp_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    *ue_context_p = NULL;

  DevAssert (delete_sess_resp_pP );
  OAILOG_DEBUG (LOG_MME_APP, "Received S11_DELETE_SESSION_RESPONSE from S+P-GW with the ID " MME_UE_S1AP_ID_FMT "\n ",delete_sess_resp_pP->teid);
  ue_context_p = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, delete_sess_resp_pP->teid);

  if (!ue_context_p) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 DELETE_SESSION_RESPONSE local S11 teid " TEID_FMT " ", delete_sess_resp_pP->teid);
    OAILOG_WARNING (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", delete_sess_resp_pP->teid);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  if (delete_sess_resp_pP->cause.cause_value != REQUEST_ACCEPTED) {
    DevMessage ("Cases where bearer cause != REQUEST_ACCEPTED are not handled\n");
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 DELETE_SESSION_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
    delete_sess_resp_pP->teid, ue_context_p->emm_context._imsi64);
  /*
   * Updating statistics
   */
  mme_app_desc.mme_ue_contexts.nb_bearers_managed--;
  mme_app_desc.mme_ue_contexts.nb_bearers_since_last_stat--;
  // todo: we should handle the delete session response // pdn disconnectivity stricktly in NAS (also not suitable for multi APN)
//  ue_context_p->mm_state = UE_UNREGISTERED;

  S1ap_Cause_t            s1_ue_context_release_cause = {0};
  s1_ue_context_release_cause.present = S1ap_Cause_PR_nas;
  s1_ue_context_release_cause.choice.nas = S1ap_CauseNas_detach;



  /**
    * Object is later removed, not here. For unused keys, this is no problem, just deregistrate the tunnel ids for the MME_APP
    * UE context from the hashtable.
    * If this is not done, later at removal of the MME_APP UE context, the S11 keys will be checked and removed again if still existing.
    *
    * todo: For multi-apn, checking if more APNs exist or removing later?
    */
   hashtable_ts_remove(mme_app_desc.mme_ue_contexts.tun11_ue_context_htbl,
                       (const hash_key_t) ue_context_p->mme_s11_teid, &id);
   ue_context_p->mme_s11_teid = 0;
   ue_context_p->sgw_s11_teid = 0;

 //  if (delete_sess_resp_pP->cause != REQUEST_ACCEPTED) {
 //    OAILOG_WARNING (LOG_MME_APP, "***WARNING****S11 Delete Session Rsp: NACK received from SPGW : %08x\n", delete_sess_resp_pP->teid);
 //  }
 //  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 DELETE_SESSION_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
 //    delete_sess_resp_pP->teid, ue_context_p->imsi);
 //  /*
 //   * Updating statistics
 //   */
 //  update_mme_app_stats_s1u_bearer_sub();
 //  update_mme_app_stats_default_bearer_sub();
 //
   /**
    * No recursion needed any more. This will just inform the EMM/ESM that a PDN session has been deactivated.
    * It will determine if its a PDN Disconnectivity or detach.
    */
   message_p = itti_alloc_new_message (TASK_MME_APP, NAS_PDN_DISCONNECT_RSP);
   // do this because of same message types name but not same struct in different .h
   message_p->ittiMsg.nas_pdn_disconnect_rsp.ue_id           = ue_context_p->mme_ue_s1ap_id;
   message_p->ittiMsg.nas_pdn_disconnect_rsp.cause           = REQUEST_ACCEPTED;
   message_p->ittiMsg.nas_pdn_disconnect_rsp.pdn_default_ebi = 5; /**< An indicator!. */
 //  message_p->ittiMsg.nas_pdn_disconnect_rsp.pti             = PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED; /**< todo: set the PTI received by UE (store in transaction). */
   // todo: later add default ebi id or pdn name to check which PDN that was!
   itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);



//  mme_app_itti_ue_context_release(ue_context_p, s1_ue_context_release_cause /* cause */);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
int
mme_app_handle_create_sess_resp (
  itti_s11_create_session_response_t * const create_sess_resp_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                 *ue_context_p = NULL;
  bearer_context_t                       *current_bearer_p = NULL;
  MessageDef                             *message_p = NULL;
  ebi_t                                   bearer_id = 0;
  int                                     rc = RETURNok;

  DevAssert (create_sess_resp_pP );
  OAILOG_DEBUG (LOG_MME_APP, "Received S11_CREATE_SESSION_RESPONSE from S+P-GW\n");
  ue_context_p = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, create_sess_resp_pP->teid);

  if (ue_context_p == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_SESSION_RESPONSE local S11 teid " TEID_FMT " ", create_sess_resp_pP->teid);

    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", create_sess_resp_pP->teid);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_SESSION_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      create_sess_resp_pP->teid, ue_context_p->emm_context._imsi64);

    proc_tid_t  transaction_identifier = 0;
    pdn_cid_t   pdn_cx_id = 0;



    /* Whether SGW has created the session (IP address allocation, local GTP-U end point creation etc.)
     * successfully or not , it is indicated by cause value in create session response message.
     * If cause value is not equal to "REQUEST_ACCEPTED" then this implies that SGW could not allocate the resources for
     * the requested session. In this case, MME-APP sends PDN Connectivity fail message to NAS-ESM with the "cause" received
     * in S11 Session Create Response message.
     * NAS-ESM maps this "S11 cause" to "ESM cause" and sends it in PDN Connectivity Reject message to the UE.
     */

    if (create_sess_resp_pP->cause != REQUEST_ACCEPTED) {
      // todo: if handover flag was active.. terminate the forward relocation procedure with a reject + remove the contexts & tunnel endpoints.
      // todo: check that EMM context did not had a TAU_PROCEDURE running, if so send a CN Context Fail
      /**
       * Send PDN CONNECTIVITY FAIL message to NAS layer.
       * For TAU/Attach case, a reject message will be sent and the UE contexts will be terminated.
       */
      message_p = itti_alloc_new_message (TASK_MME_APP, NAS_PDN_CONNECTIVITY_FAIL);
      itti_nas_pdn_connectivity_fail_t *nas_pdn_connectivity_fail = &message_p->ittiMsg.nas_pdn_connectivity_fail;
      memset ((void *)nas_pdn_connectivity_fail, 0, sizeof (itti_nas_pdn_connectivity_fail_t));
      nas_pdn_connectivity_fail->pti = ue_context_p->pending_pdn_connectivity_req_pti;
      nas_pdn_connectivity_fail->ue_id = ue_context_p->pending_pdn_connectivity_req_ue_id;
      nas_pdn_connectivity_fail->cause = (pdn_conn_rsp_cause_t)(create_sess_resp_pP->cause);
      rc = itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
      OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
    }


    //---------------------------------------------------------
    // Process itti_sgw_create_session_response_t.bearer_context_created
    //---------------------------------------------------------

    // todo: for handover with dedicated bearers --> iterate through bearer contexts!
    // todo: the MME will send an S10 message with the filters to the target MME --> which would then create a Create Session Request with multiple bearers..
    // todo: all the bearer contexts in the response should then be handled! (or do it via BRC // meshed PCRF).

    for (int i=0; i < create_sess_resp_pP->bearer_contexts_created.num_bearer_context; i++) {
      bearer_id = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].eps_bearer_id /* - 5 */ ;
      /*
       * Depending on s11 result we have to send reject or accept for bearers
       */
      DevCheck ((bearer_id < BEARERS_PER_UE) && (bearer_id >= 0), bearer_id, BEARERS_PER_UE, 0);

      // todo: handle this case like create bearer request rejects in the SAE-GW, no removal of bearer contexts should be necessary
      if (create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].cause.cause_value != REQUEST_ACCEPTED) {
        DevMessage ("Cases where bearer cause != REQUEST_ACCEPTED are not handled\n");
      }

      // todo: setting the default bearer id in the pdn context?


      DevAssert (create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].s1u_sgw_fteid.interface_type == S1_U_SGW_GTP_U);

//      current_bearer_p = mme_app_get_bearer_context(ue_context_p, bearer_id);
//      AssertFatal(current_bearer_p, "Could not get bearer context");

      /**
       * Try to get a new bearer context in the UE_Context instead of getting one from an array
       * todo: if cause above was reject for this bearer context, just skip it. */
      if ((current_bearer_p = mme_app_create_new_bearer_context(ue_context_p, bearer_id)) == NULL) {
          // If we failed to allocate a new bearer context
          OAILOG_ERROR (LOG_MME_APP, "Failed to allocate a new bearer context with EBI %d for mmeUeS1apId:" MME_UE_S1AP_ID_FMT "\n", bearer_id, ue_context_p->mme_ue_s1ap_id);
          OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
      }

      current_bearer_p->bearer_state |= BEARER_STATE_SGW_CREATED;
      if (!i) {
        pdn_cx_id = current_bearer_p->pdn_cx_id;
        /*
         * Store the S-GW teid
         */
        AssertFatal((pdn_cx_id >= 0) && (pdn_cx_id < MAX_APN_PER_UE), "Bad pdn id for bearer");
        ue_context_p->pdn_contexts[pdn_cx_id]->s_gw_teid_s11_s4 = create_sess_resp_pP->s11_sgw_fteid.teid;
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
        current_bearer_p->esm_ebr_context.gbr_dl   = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->gbr.br_dl;
        current_bearer_p->esm_ebr_context.gbr_ul   = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->gbr.br_ul;
        current_bearer_p->esm_ebr_context.mbr_dl   = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->mbr.br_dl;
        current_bearer_p->esm_ebr_context.mbr_ul   = create_sess_resp_pP->bearer_contexts_created.bearer_contexts[i].bearer_level_qos->mbr.br_ul;
        OAILOG_DEBUG (LOG_MME_APP, "Set qci %u in bearer %u\n", current_bearer_p->qci, bearer_id);
      } else {
        OAILOG_DEBUG (LOG_MME_APP, "Set qci %u in bearer %u (qos not modified by P-GW)\n", current_bearer_p->qci, bearer_id);

      }
    }

    // todo: update the PCO's received from the SAE-GW.
    //    if (pco) {
    //      if (!pdn_context->pco) {
    //        pdn_context->pco = calloc(1, sizeof(protocol_configuration_options_t));
    //      } else {
    //           clear_protocol_configuration_options(pdn_context->pco);
    //      }
    //      copy_protocol_configuration_options(pdn_context->pco, pco);
    //    }

    // todo: review the old code!
    //#define TEMPORARY_DEBUG 1
    //#if TEMPORARY_DEBUG
    // bstring b = protocol_configuration_options_to_xml(&ue_context_p->pending_pdn_connectivity_req_pco);
    // OAILOG_DEBUG (LOG_MME_APP, "PCO %s\n", bdata(b));
    // bdestroy(b);
    //#endif

    /*
     * todo:Here either inform the UE NAS layer about the PDN connectivity or continue with NAS Context Resp or Handover Request.
     * We should check for running processes, and act respectively. --> MOBILTIY process.. !
     * todo: also not send NAS PDN Connectivity if emm exists but TAU! in case of idle mode tau, we should not send this to NAS!
     */

    //uint8_t *keNB = NULL;

    // todo: if handovered with multiple bearers, we might need to send  CSR and handle CSResp with multiple bearer contexts!
    // then do we need to send NAS only the default bearer_context or all bearer contexts, and send them all in NAS?
    // todo: for dedicated bearers, we also might need to set GBR/MBR values
//    nas_pdn_connectivity_rsp->qos.gbrUL = 64;        /* 64=64kb/s   Guaranteed Bit Rate for uplink   */
//      nas_pdn_connectivity_rsp->qos.gbrDL = 120;       /* 120=512kb/s Guaranteed Bit Rate for downlink */
//      nas_pdn_connectivity_rsp->qos.mbrUL = 72;        /* 72=128kb/s   Maximum Bit Rate for uplink      */
//      nas_pdn_connectivity_rsp->qos.mbrDL = 135;       /*135=1024kb/s Maximum Bit Rate for downlink    */

    emm_data_context_t *ue_nas_ctx = emm_data_context_get_by_imsi (&_emm_data, ue_context_p->imsi);
    if (ue_nas_ctx) {
      nas_ctx_req_proc_t * emm_cn_proc_ctx_req = get_nas_cn_procedure_ctx_req(emm_context);
      if(emm_cn_proc_ctx_req){
        /** Send NAS context response procedure.
         * Build a NAS_CONTEXT_INFO message and fill it.
         * Depending on the cause, NAS layer can perform an TAU_REJECT or move on with the TAU validation.
         * NAS layer.
         *
         * todo: nas_ctx_fail!
         */
        message_p = itti_alloc_new_message (TASK_MME_APP, NAS_CONTEXT_RES);
        itti_nas_context_res_t *nas_context_res = &message_p->ittiMsg.nas_context_res; // todo: mme app handover reject
        /** Set the cause. */
        /** Set the UE identifiers. */
        nas_context_res->ue_id = ue_context_p->mme_ue_s1ap_id;
        /** Fill the elements of the NAS message from S10 CONTEXT_RESPONSE. */

        /** todo: Convert the GTPv2c IMSI struct to the NAS IMSI struct. */
        nas_context_res->imsi= ue_context_p->imsi;
        //  memset (&(ue_context_p->pending_pdn_connectivity_req_imsi), 0, 16); /**< IMSI in create session request. */
        //  memcpy (&(ue_context_p->pending_pdn_connectivity_req_imsi), &(s10_context_response_pP->imsi.digit), s10_context_response_pP->imsi.length);
        //  ue_context_p->pending_pdn_connectivity_req_imsi_length = s10_context_response_pP->imsi.length;

        /** When sending NAS context response, inform it also about the number established PDN sessions and bearers in the ESM layer. */
        pdn_context_t * registered_pdn_ctx = NULL;
        RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_context_p->pdn_contexts) {
          DevAssert(registered_pdn_ctx);
          nas_context_res->n_pdns++;
          bearer_context_t * bearer_contexts = NULL;
          RB_FOREACH (bearer_contexts, BearerPool, &registered_pdn_ctx->session_bearers) {
            DevAssert(registered_pdn_ctx);
            nas_context_res->n_bearers++;
          }
        }
        MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_CONTEXT_RES sgw_s1u_teid %u ebi %u qci %u prio %u",
            current_bearer_p->s_gw_fteid_s1u.teid,
            bearer_id,
            current_bearer_p->qci,
            current_bearer_p->priority_level);
        rc = itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
      }else{
        OAILOG_INFO (LOG_MME_APP, "Informing the NAS layer about the received CREATE_SESSION_REQUEST for UE " MME_UE_S1AP_ID_FMT ". \n", ue_context_p->mme_ue_s1ap_id);
        //uint8_t *keNB = NULL;
        message_p = itti_alloc_new_message (TASK_MME_APP, NAS_PDN_CONNECTIVITY_RSP);
        itti_nas_pdn_connectivity_rsp_t *nas_pdn_connectivity_rsp = &message_p->ittiMsg.nas_pdn_connectivity_rsp;

        nas_pdn_connectivity_rsp->pdn_cid = pdn_cx_id;
        nas_pdn_connectivity_rsp->pti = transaction_identifier;  // NAS internal ref
        nas_pdn_connectivity_rsp->ue_id = ue_context_p->mme_ue_s1ap_id;      // NAS internal ref

        nas_pdn_connectivity_rsp->pdn_addr = paa_to_bstring(&create_sess_resp_pP->paa);
        // todo: mme_app ue_context does not has a PAA?
  //      memcpy(ue_context_p->paa.ipv4_address, create_sess_resp_pP->paa.ipv4_address, 4);
        nas_pdn_connectivity_rsp->pdn_type = create_sess_resp_pP->paa.pdn_type;
        // ASSUME NO HO now, so assume 1 bearer only and is default bearer
  //      nas_pdn_connectivity_rsp->request_type = ue_context_p->pending_pdn_connectivity_req_request_type;        // NAS internal ref
  //      ue_context_p->pending_pdn_connectivity_req_request_type = 0;
        // here at this point OctetString are saved in resp, no loss of memory (apn, pdn_addr)
        nas_pdn_connectivity_rsp->ue_id                 = ue_context_p->mme_ue_s1ap_id;
        nas_pdn_connectivity_rsp->ebi                   = bearer_id;
        nas_pdn_connectivity_rsp->qci                   = current_bearer_p->qci;
        nas_pdn_connectivity_rsp->prio_level            = current_bearer_p->priority_level;
        nas_pdn_connectivity_rsp->pre_emp_vulnerability = current_bearer_p->preemption_vulnerability;
        nas_pdn_connectivity_rsp->pre_emp_capability    = current_bearer_p->preemption_capability;
        nas_pdn_connectivity_rsp->sgw_s1u_fteid         = current_bearer_p->s_gw_fteid_s1u;
        // optional IE
        nas_pdn_connectivity_rsp->ambr.br_ul            = ue_context_p->suscribed_ue_ambr.br_ul;
        nas_pdn_connectivity_rsp->ambr.br_dl            = ue_context_p->suscribed_ue_ambr.br_dl;
        // This IE is not applicable for TAU/RAU/Handover. If PGW decides to return PCO to the UE, PGW shall send PCO to
        // SGW. If SGW receives the PCO IE, SGW shall forward it to MME/SGSN.
        if (create_sess_resp_pP->pco.num_protocol_or_container_id) {
          copy_protocol_configuration_options (&nas_pdn_connectivity_rsp->pco, &create_sess_resp_pP->pco);
          clear_protocol_configuration_options(&create_sess_resp_pP->pco);
        }
        MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_PDN_CONNECTIVITY_RSP sgw_s1u_teid %u ebi %u qci %u prio %u",
            current_bearer_p->s_gw_fteid_s1u.teid,
            bearer_id,
            current_bearer_p->qci,
            current_bearer_p->priority_level);

        rc = itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
      }
    } else{
      OAILOG_INFO(LOG_MME_APP, "NO EMM_CONTEXT exists for UE " MME_UE_S1AP_ID_FMT ". \n", ue_context_p->mme_ue_s1ap_id);
      mme_app_get_handover_procedure = mme_app_get_ho_proc();

      // todo: We have the indicator of handover in the CREATE_SESSION_REQUEST operational flags.
      // todo: check that handover procedure is running or not.
      if(handover_proc->pending_s10_response_trxn){
        OAILOG_INFO(LOG_MME_APP, "UE " MME_UE_S1AP_ID_FMT " is performing an S10 handover. Sending an S1AP_HANDOVER_REQUEST. \n", ue_context_p->mme_ue_s1ap_id);
        mme_app_send_s1ap_handover_request(ue_context_p->mme_ue_s1ap_id, ue_context_p->pending_handover_target_enb_id,
            handover_proc->pending_mm_ue_eps_context->ue_nc.eea,
            handover_proc->pending_mm_ue_eps_context->ue_nc.eia,
            handover_proc->pending_mm_ue_eps_context->nh,
            handover_proc->pending_mm_ue_eps_context->ncc);
        OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
      }else{
        OAILOG_CRITICAL(LOG_MME_APP, "CREATE_SESSION_RESPONSE received for invalid UE " MME_UE_S1AP_ID_FMT ". \n", ue_context_p->mme_ue_s1ap_id);
        /** Deallocate the ue context and remove from MME_APP map. */
        mme_remove_ue_context (&mme_app_desc.mme_ue_contexts, ue_context_p);
        /** Not sending back failure. */
        OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
      }
    }
    OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
int
mme_app_handle_modify_bearer_resp (
  const itti_s11_modify_bearer_response_t * const modify_bearer_resp_pP)
{
  struct ue_context_s                    *ue_context_p = NULL;
  bearer_context_t                       *current_bearer_p = NULL;
  MessageDef                             *message_p = NULL;
  int16_t                                 bearer_id =5;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (modify_bearer_resp_pP );
  OAILOG_DEBUG (LOG_MME_APP, "Received S11_MODIFY_BEARER_RESPONSE from S+P-GW\n");
  ue_context_p = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, modify_bearer_resp_pP->teid);

  if (ue_context_p == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 MODIFY_BEARER_RESPONSE local S11 teid " TEID_FMT " ", modify_bearer_resp_pP->teid);

    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", modify_bearer_resp_pP->teid);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 MODIFY_BEARER_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      modify_bearer_resp_pP->teid, ue_context_p->imsi);
  /*
   * Updating statistics
   */
  if (modify_bearer_resp_pP->cause != REQUEST_ACCEPTED) {
    /**
     * Check if it is an X2 Handover procedure, in that case send an X2 Path Switch Request Failure to the target MME.
     * In addition, perform an implicit detach in any case.
     */
    if(ue_context_p->pending_x2_handover){
      OAILOG_ERROR(LOG_MME_APP, "Error modifying SAE-GW bearers for UE with ueId: " MME_UE_S1AP_ID_FMT ". \n", ue_context_p->mme_ue_s1ap_id);
      mme_app_send_s1ap_path_switch_request_failure(ue_context_p->mme_ue_s1ap_id, ue_context_p->enb_ue_s1ap_id, ue_context_p->sctp_assoc_id_key, SYSTEM_FAILURE);
      /** We continue with the implicit detach, since handover already happened. */
    }
    /** Implicitly detach the UE --> If EMM context is missing, still continue with the resource removal. */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context_p->mme_ue_s1ap_id;
    message_p->ittiMsg.nas_implicit_detach_ue_ind.emm_cause = EMM_CAUSE_NETWORK_FAILURE;
    message_p->ittiMsg.nas_implicit_detach_ue_ind.detach_type = 0x02; // Re-Attach Not required;
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
  }
  current_bearer_p =  mme_app_is_bearer_context_in_list(ue_context_p->mme_ue_s1ap_id, ue_context_p->default_bearer_id);
  // todo: set the downlink teid?
  /** If it is an X2 Handover, send a path switch response back. */
  if(ue_context_p->pending_x2_handover){
    OAILOG_INFO(LOG_MME_APP, "Sending an S1AP Path Switch Request Acknowledge for for UE with ueId: " MME_UE_S1AP_ID_FMT ". \n", ue_context_p->mme_ue_s1ap_id);
    mme_app_send_s1ap_path_switch_request_acknowledge(ue_context_p->mme_ue_s1ap_id);
  }
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
void
mme_app_handle_downlink_data_notification(const itti_s11_downlink_data_notification_t * const saegw_dl_data_ntf_pP){
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;
  int16_t                                 bearer_id =5;
  int                                     rc = RETURNok;

  SGWCause_t                              cause;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (saegw_dl_data_ntf_pP );
  DevAssert (saegw_dl_data_ntf_pP->trxn);

  OAILOG_DEBUG (LOG_MME_APP, "Received S11_DOWNLINK_DATA_NOTIFICATION from S+P-GW\n");
  ue_context_p = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, saegw_dl_data_ntf_pP->teid);

  if (ue_context_p == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "DOWNLINK_DATA_NOTIFICATION FROM local S11 teid " TEID_FMT " ", saegw_dl_data_ntf_pP->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", saegw_dl_data_ntf_pP->teid);
    /** Send a DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE. */
    mme_app_send_downlink_data_notification_acknowledge(CONTEXT_NOT_FOUND, saegw_dl_data_ntf_pP->teid, saegw_dl_data_ntf_pP->trxn);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "DOWNLINK_DATA_NOTIFICATION for local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      saegw_dl_data_ntf_pP->teid, ue_context_p->imsi);

  /** Check that the UE is in idle mode!. */
  if (ECM_IDLE != ue_context_p->ecm_state) {
    OAILOG_ERROR (LOG_MME_APP, "UE_Context with IMSI " IMSI_64_FMT " and mmeUeS1apId: %d. \n is not in ECM_IDLE mode, insted %d. \n",
        ue_context_p->imsi, ue_context_p->mme_ue_s1ap_id, ue_context_p->ecm_state);
    // todo: later.. check this more granularly
    mme_app_send_downlink_data_notification_acknowledge(UE_ALREADY_REATTACHED, saegw_dl_data_ntf_pP->teid, saegw_dl_data_ntf_pP->trxn);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  OAILOG_INFO(LOG_MME_APP, "MME_MOBILTY_COMPLETION timer is not running. Starting paging procedure for UE with imsi " IMSI_64_FMT ". \n", ue_context_p->imsi);

  // todo: timeout to wait to ignore further DL_DATA_NOTIF messages->
  mme_app_send_downlink_data_notification_acknowledge(REQUEST_ACCEPTED, saegw_dl_data_ntf_pP->teid, saegw_dl_data_ntf_pP->trxn);

  /** No need to start paging timeout timer. It will be handled by the Periodic TAU update timer. */
  // todo: no downlink data notification failure and just removing the UE?

  /** Do paging on S1AP interface. */
  message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_PAGING);
  DevAssert (message_p != NULL);
  itti_s1ap_paging_t *s1ap_paging_p = &message_p->ittiMsg.s1ap_paging;

  memset (s1ap_paging_p, 0, sizeof (itti_s1ap_paging_t));
  s1ap_paging_p->mme_ue_s1ap_id = ue_context_p->mme_ue_s1ap_id; /**< Just MME_UE_S1AP_ID. */
  s1ap_paging_p->ue_identity_index = (ue_context_p->imsi %1024) & 0xFFFF; /**< Just MME_UE_S1AP_ID. */
  s1ap_paging_p->tmsi = ue_context_p->guti.m_tmsi;
  // todo: these ones may differ from GUTI?
  s1ap_paging_p->tai.plmn = ue_context_p->guti.gummei.plmn;
  s1ap_paging_p->tai.tac  = *mme_config.served_tai.tac;

  /** S1AP Paging. */
  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
}

//------------------------------------------------------------------------------
void
mme_app_send_downlink_data_notification_acknowledge(SGWCause_t cause, teid_t saegw_s11_teid, void *trxn){
  OAILOG_FUNC_IN (LOG_MME_APP);

  /** Send a Downlink Data Notification Acknowledge with cause. */
  MessageDef * message_p = itti_alloc_new_message (TASK_MME_APP, S11_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE);
  DevAssert (message_p != NULL);

  itti_s11_downlink_data_notification_acknowledge_t *downlink_data_notification_ack_p = &message_p->ittiMsg.s11_downlink_data_notification_acknowledge;
  memset ((void*)downlink_data_notification_ack_p, 0, sizeof (itti_s11_downlink_data_notification_acknowledge_t));
  // todo: s10 TEID set every time?
  downlink_data_notification_ack_p->teid = saegw_s11_teid; // todo: ue_context_pP->mme_s10_teid;
  /** No Local TEID exists yet.. no local S10 tunnel is allocated. */
  // todo: currently only just a single MME is allowed.
  downlink_data_notification_ack_p->peer_ip = mme_config.ipv4.sgw_s11;

  downlink_data_notification_ack_p->cause = cause;
  downlink_data_notification_ack_p->trxn  = trxn;

  /** Deallocate the contaier in the FORWARD_RELOCATION_REQUEST.  */
  // todo: how is this deallocated

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "MME_APP Sending S11 DOWNLINK_DATA_NOTIFICATION_ACK");

  /** Sending a message to S10. */
  itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_initial_context_setup_rsp (
  itti_mme_app_initial_context_setup_rsp_t * const initial_ctxt_setup_rsp_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                 *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;

  OAILOG_DEBUG (LOG_MME_APP, "Received MME_APP_INITIAL_CONTEXT_SETUP_RSP from S1AP\n");
  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, initial_ctxt_setup_rsp_pP->ue_id);

  if (ue_context_p == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", initial_ctxt_setup_rsp_pP->ue_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " MME_APP_INITIAL_CONTEXT_SETUP_RSP Unknown ue %u", initial_ctxt_setup_rsp_pP->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }


//  if (!ue_context_p->is_s1_ue_context_release) {

  // todo: where are these timers?
  // Stop Initial context setup process guard timer,if running
  if (ue_context_p->initial_context_setup_rsp_timer.id != MME_APP_TIMER_INACTIVE_ID) {
    if (timer_remove(ue_context_p->initial_context_setup_rsp_timer.id)) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to stop Initial Context Setup Rsp timer for UE id  %d \n", ue_context_p->mme_ue_s1ap_id);
    }
    ue_context_p->initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  }


  /** Save the bearer information as pending or send it directly if UE is registered. */
  if(ue_context_p->mm_state == UE_REGISTERED){
    /** Send the DL-GTP Tunnel Information to the SAE-GW. */
    message_p = itti_alloc_new_message (TASK_MME_APP, S11_MODIFY_BEARER_REQUEST);
    AssertFatal (message_p , "itti_alloc_new_message Failed");
    itti_s11_modify_bearer_request_t *s11_modify_bearer_request = &message_p->ittiMsg.s11_modify_bearer_request;
    s11_modify_bearer_request->local_teid = ue_context_p->mme_teid_s11;
    /*
     * Delay Value in integer multiples of 50 millisecs, or zero
     */
    s11_modify_bearer_request->delay_dl_packet_notif_req = 0;  // TODO

    for (int item = 0; item < initial_ctxt_setup_rsp_pP->no_of_e_rabs; item++) {
      s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[item].eps_bearer_id     = initial_ctxt_setup_rsp_pP->e_rab_id[item];
      s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[item].s1_eNB_fteid.teid = initial_ctxt_setup_rsp_pP->gtp_teid[item];
      s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[item].s1_eNB_fteid.interface_type    = S1_U_ENODEB_GTP_U;

      if (!item) {
        ebi_t             ebi = initial_ctxt_setup_rsp_pP->e_rab_id[item];
        pdn_cid_t         cid = ue_context_p->bearer_contexts[EBI_TO_INDEX(ebi)]->pdn_cx_id;
        pdn_context_t    *pdn_context = ue_context_p->pdn_contexts[cid];

        s11_modify_bearer_request->peer_ip = pdn_context->s_gw_address_s11_s4.address.ipv4_address;
        s11_modify_bearer_request->teid    = pdn_context->s_gw_teid_s11_s4;
      }
      if (4 == blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item])) {
        s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv4         = 1;
        memcpy(&s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv4_address,
            initial_ctxt_setup_rsp_pP->transport_layer_address[item]->data, blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item]));
      } else if (16 == blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item])) {
        s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv6         = 1;
        memcpy(&s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[item].s1_eNB_fteid.ipv6_address,
            initial_ctxt_setup_rsp_pP->transport_layer_address[item]->data,
            blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item]));
      } else {
        AssertFatal(0, "TODO IP address %d bytes", blength(initial_ctxt_setup_rsp_pP->transport_layer_address[item]));
      }
      bdestroy_wrapper (&initial_ctxt_setup_rsp_pP->transport_layer_address[item]);
    }
    s11_modify_bearer_request->bearer_contexts_to_be_modified.num_bearer_context = initial_ctxt_setup_rsp_pP->no_of_e_rabs;

    // todo: implement this if after service request, etc.. bearers are to be removed (in addition to delete bearer command and bearer resource command).
    s11_modify_bearer_request->bearer_contexts_to_be_removed.num_bearer_context = 0;

    s11_modify_bearer_request->mme_fq_csid.node_id_type = GLOBAL_UNICAST_IPv4; // TODO
    s11_modify_bearer_request->mme_fq_csid.csid = 0;   // TODO ...
    memset(&s11_modify_bearer_request->indication_flags, 0, sizeof(s11_modify_bearer_request->indication_flags));   // TODO
    s11_modify_bearer_request->rat_type = RAT_EUTRAN;
    /*
     * S11 stack specific parameter. Not used in standalone epc mode
     */
    s11_modify_bearer_request->trxn = NULL;
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME,  MSC_S11_MME ,
                        NULL, 0, "0 S11_MODIFY_BEARER_REQUEST teid %u ebi %u", s11_modify_bearer_request->teid,
                        s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].eps_bearer_id);
    itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
  }else{
    /** Will send MBR with MM state change callbacks (with ATTACH_COMPLETE). */
    OAILOG_INFO(LOG_MME_APP, "IMSI " IMSI_64_FMT " is not registered yet. Waiting the UE to register to send the MBR.\n", ue_context_p->imsi);
    memcpy(&ue_context_p->pending_s1u_downlink_bearer, &initial_ctxt_setup_rsp_pP->bearer_s1u_enb_fteid, sizeof(FTeid_t));
    ue_context_p->pending_s1u_downlink_bearer_ebi = initial_ctxt_setup_rsp_pP->eps_bearer_id;
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_release_access_bearers_resp (
  const itti_s11_release_access_bearers_response_t * const rel_access_bearers_rsp_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                    *ue_context_p = NULL;

  ue_context_p = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, rel_access_bearers_rsp_pP->teid);

  if (ue_context_p == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 RELEASE_ACCESS_BEARERS_RESPONSE local S11 teid " TEID_FMT " ",
    		rel_access_bearers_rsp_pP->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %" PRIX32 "\n", rel_access_bearers_rsp_pP->teid);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 RELEASE_ACCESS_BEARERS_RESPONSE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ", rel_access_bearers_rsp_pP->teid, ue_context_p->emm_context._imsi64);

  S1ap_Cause_t            s1_ue_context_release_cause = {0};
  s1_ue_context_release_cause.present = S1ap_Cause_PR_radioNetwork;
  s1_ue_context_release_cause.choice.radioNetwork = S1ap_CauseRadioNetwork_release_due_to_eutran_generated_reason;

  // Send UE Context Release Command
  mme_app_itti_ue_context_release(ue_context_p->mme_ue_s1ap_id, ue_context_p->enb_ue_s1ap_id, ue_context_p->ue_context_rel_cause, ue_context_p->e_utran_cgi.cell_identity.enb_id);
  if (ue_context_p->ue_context_rel_cause == S1AP_SCTP_SHUTDOWN_OR_RESET) {
    // Just cleanup the MME APP state associated with s1.
    mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context_p, ECM_IDLE);
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
  struct ue_context_s                    *ue_context_p = NULL;

  ue_context_p = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, create_bearer_request_pP->teid);

  if (ue_context_p == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_BEARERS_REQUEST local S11 teid " TEID_FMT " ",
        create_bearer_request_pP->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %" PRIX32 "\n", create_bearer_request_pP->teid);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  if (!ue_context_p->is_s1_ue_context_release) {
    // check if default bearer already created
    ebi_t linked_eps_bearer_id = create_bearer_request_pP->linked_eps_bearer_id;
    bearer_context_t * linked_bc = mme_app_get_bearer_context(ue_context_p, linked_eps_bearer_id);
    if (!linked_bc) {
      // May create default EPS bearer ?
      MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_BEARERS_REQUEST ue id " MME_UE_S1AP_ID_FMT " local S11 teid " TEID_FMT " ",
          ue_context_p->mme_ue_s1ap_id, create_bearer_request_pP->teid);
      OAILOG_DEBUG (LOG_MME_APP, "We didn't find the linked bearer id %" PRIu8 " for UE: " MME_UE_S1AP_ID_FMT "\n",
          linked_eps_bearer_id, ue_context_p->mme_ue_s1ap_id);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }

    pdn_cid_t cid = linked_bc->pdn_cx_id;

    MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 CREATE_BEARERS_REQUEST ue id " MME_UE_S1AP_ID_FMT " PDN id %u IMSI " IMSI_64_FMT " n ebi %u",
        ue_context_p->mme_ue_s1ap_id, cid, ue_context_p->emm_context._imsi64, create_bearer_request_pP->bearer_contexts.num_bearer_context);

    mme_app_s11_proc_create_bearer_t* s11_proc_create_bearer = mme_app_create_s11_procedure_create_bearer(ue_context_p);
    s11_proc_create_bearer->proc.s11_trxn  = (uintptr_t)create_bearer_request_pP->trxn;

    for (int i = 0; i < create_bearer_request_pP->bearer_contexts.num_bearer_context; i++) {
      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      // TODO THINK OF BEARER AGGREGATING SEVERAL SDFs, 1 bearer <-> (QCI, ARP)
      // TODO DELEGATE TO NAS THE CREATION OF THE BEARER
      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      const bearer_context_within_create_bearer_request_t *msg_bc = &create_bearer_request_pP->bearer_contexts.bearer_contexts[i];
      bearer_context_t *  dedicated_bc = mme_app_create_bearer_context(ue_context_p, cid, msg_bc->eps_bearer_id, false);

      s11_proc_create_bearer->num_bearers++;
      s11_proc_create_bearer->bearer_status[EBI_TO_INDEX(dedicated_bc->ebi)] = S11_PROC_BEARER_PENDING;

      dedicated_bc->bearer_state   |= BEARER_STATE_SGW_CREATED;
      dedicated_bc->bearer_state   |= BEARER_STATE_MME_CREATED;

      dedicated_bc->s_gw_fteid_s1u      = msg_bc->s1u_sgw_fteid;
      dedicated_bc->p_gw_fteid_s5_s8_up = msg_bc->s5_s8_u_pgw_fteid;

      dedicated_bc->qci                      = msg_bc->bearer_level_qos.qci;
      dedicated_bc->priority_level           = msg_bc->bearer_level_qos.pl;
      dedicated_bc->preemption_vulnerability = msg_bc->bearer_level_qos.pvi;
      dedicated_bc->preemption_capability    = msg_bc->bearer_level_qos.pci;

      // forward request to NAS
      MessageDef  *message_p = itti_alloc_new_message (TASK_MME_APP, MME_APP_CREATE_DEDICATED_BEARER_REQ);
      AssertFatal (message_p , "itti_alloc_new_message Failed");
      MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).ue_id          = ue_context_p->mme_ue_s1ap_id;
      MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).cid            = cid;
      MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).ebi            = dedicated_bc->ebi;
      MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).linked_ebi     = ue_context_p->pdn_contexts[cid]->default_ebi;
      MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).bearer_qos     = msg_bc->bearer_level_qos;
      if (msg_bc->tft.numberofpacketfilters) {
        MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).tft = calloc(1, sizeof(traffic_flow_template_t));
        copy_traffic_flow_template(MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).tft, &msg_bc->tft);
      }
      if (msg_bc->pco.num_protocol_or_container_id) {
        MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).pco = calloc(1, sizeof(protocol_configuration_options_t));
        copy_protocol_configuration_options(MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).pco, &msg_bc->pco);
      }

      MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 MME_APP_CREATE_DEDICATED_BEARER_REQ mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " qci %u ebi %u cid %u",
          MME_APP_CREATE_DEDICATED_BEARER_REQ (message_p).ue_id, dedicated_bc->qci, dedicated_bc->ebi, cid);
      itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);

    }
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_handle_e_rab_setup_rsp (itti_s1ap_e_rab_setup_rsp_t  * const e_rab_setup_rsp)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                 *ue_context_p = NULL;
  bool                                    send_s11_response = false;

  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, e_rab_setup_rsp->mme_ue_s1ap_id);

  if (ue_context_p == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", e_rab_setup_rsp->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, " S1AP_E_RAB_SETUP_RSP Unknown ue " MME_UE_S1AP_ID_FMT "\n", e_rab_setup_rsp->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  if (!ue_context_p->is_s1_ue_context_release) {

    for (int i = 0; i < e_rab_setup_rsp->e_rab_setup_list.no_of_items; i++) {
      e_rab_id_t e_rab_id = e_rab_setup_rsp->e_rab_setup_list.item[i].e_rab_id;
      bearer_context_t * bc = mme_app_get_bearer_context(ue_context_p, (ebi_t) e_rab_id);
      if (bc->bearer_state & BEARER_STATE_SGW_CREATED) {
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

        if (ESM_EBR_ACTIVE == bc->esm_ebr_context.status) {
          send_s11_response = true;
        }
      }
    }
    for (int i = 0; i < e_rab_setup_rsp->e_rab_failed_to_setup_list.no_of_items; i++) {
      e_rab_id_t e_rab_id = e_rab_setup_rsp->e_rab_failed_to_setup_list.item[i].e_rab_id;
      bearer_context_t * bc = mme_app_get_bearer_context(ue_context_p, (ebi_t) e_rab_id);
      if (bc->bearer_state & BEARER_STATE_SGW_CREATED) {
        send_s11_response = true;
        //S1ap_Cause_t cause = e_rab_setup_rsp->e_rab_failed_to_setup_list.item[i].cause;
        AssertFatal(bc->bearer_state & BEARER_STATE_MME_CREATED, "TO DO check bearer state");
        bc->bearer_state &= (~BEARER_STATE_ENB_CREATED);
        bc->bearer_state &= (~BEARER_STATE_MME_CREATED);
      }
    }

    // check if UE already responded with NAS (may depend on eNB implementation?) -> send response to SGW
    if (send_s11_response) {
      MessageDef  *message_p = itti_alloc_new_message (TASK_MME_APP, S11_CREATE_BEARER_RESPONSE);
      AssertFatal (message_p , "itti_alloc_new_message Failed");
      itti_s11_create_bearer_response_t *s11_create_bearer_response = &message_p->ittiMsg.s11_create_bearer_response;
      s11_create_bearer_response->local_teid = ue_context_p->mme_teid_s11;
      s11_create_bearer_response->trxn = NULL;
      s11_create_bearer_response->cause.cause_value = 0;
      int msg_bearer_index = 0;

      for (int i = 0; i < e_rab_setup_rsp->e_rab_setup_list.no_of_items; i++) {
        e_rab_id_t e_rab_id = e_rab_setup_rsp->e_rab_setup_list.item[i].e_rab_id;
        bearer_context_t * bc = mme_app_get_bearer_context(ue_context_p, (ebi_t) e_rab_id);
        if (bc->bearer_state & BEARER_STATE_ENB_CREATED) {
          s11_create_bearer_response->cause.cause_value = REQUEST_ACCEPTED;
          s11_create_bearer_response->bearer_contexts.bearer_contexts[msg_bearer_index].eps_bearer_id = e_rab_id;
          s11_create_bearer_response->bearer_contexts.bearer_contexts[msg_bearer_index].cause.cause_value = REQUEST_ACCEPTED;
          //  FTEID eNB
          s11_create_bearer_response->bearer_contexts.bearer_contexts[msg_bearer_index].s1u_enb_fteid = bc->enb_fteid_s1u;

          // FTEID SGW S1U
          s11_create_bearer_response->bearer_contexts.bearer_contexts[msg_bearer_index].s1u_sgw_fteid = bc->s_gw_fteid_s1u;       ///< This IE shall be sent on the S11 interface. It shall be used
          s11_create_bearer_response->bearer_contexts.num_bearer_context++;
        }
      }

      for (int i = 0; i < e_rab_setup_rsp->e_rab_setup_list.no_of_items; i++) {
        e_rab_id_t e_rab_id = e_rab_setup_rsp->e_rab_setup_list.item[i].e_rab_id;
        bearer_context_t * bc = mme_app_get_bearer_context(ue_context_p, (ebi_t) e_rab_id);
        if (bc->bearer_state & BEARER_STATE_MME_CREATED) {
          if (REQUEST_ACCEPTED == s11_create_bearer_response->cause.cause_value) {
            s11_create_bearer_response->cause.cause_value = REQUEST_ACCEPTED_PARTIALLY;
          } else {
            s11_create_bearer_response->cause.cause_value = REQUEST_REJECTED;
          }
          s11_create_bearer_response->bearer_contexts.bearer_contexts[msg_bearer_index].eps_bearer_id = e_rab_id;
          s11_create_bearer_response->bearer_contexts.bearer_contexts[msg_bearer_index].cause.cause_value = REQUEST_REJECTED; // TODO translation of S1AP cause to SGW cause
          s11_create_bearer_response->bearer_contexts.num_bearer_context++;
          bc->bearer_state = BEARER_STATE_NULL;
        }
      }

      MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME,  MSC_S11_MME ,
                        NULL, 0, "0 S11_CREATE_BEARER_RESPONSE teid %u", s11_create_bearer_response->teid);
      itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
    } else {
      // not send S11 response
      // TODO create a procedure with bearers to receive a response from NAS
    }
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_handle_create_dedicated_bearer_rsp (itti_mme_app_create_dedicated_bearer_rsp_t   * const create_dedicated_bearer_rsp)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                 *ue_context_p = NULL;

  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, create_dedicated_bearer_rsp->ue_id);

  if (ue_context_p == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", create_dedicated_bearer_rsp->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  if (!ue_context_p->is_s1_ue_context_release) {

    // TODO:
    // Actually do it simple, because it appear we have to wait for NAS procedure reworking (work in progress on another branch)
    // for responding to S11 without mistakes (may be the create bearer procedure can be impacted by a S1 ue context release or
    // a UE originating  NAS procedure)
    mme_app_s11_proc_create_bearer_t *s11_proc_create = mme_app_get_s11_procedure_create_bearer(ue_context_p);
    if (s11_proc_create) {
      ebi_t ebi = create_dedicated_bearer_rsp->ebi;

      s11_proc_create->num_status_received++;
      s11_proc_create->bearer_status[EBI_TO_INDEX(ebi)] = S11_PROC_BEARER_SUCCESS;
      // if received all bearers creation results
      if (s11_proc_create->num_status_received == s11_proc_create->num_bearers) {
        mme_app_s11_procedure_create_bearer_send_response(ue_context_p, s11_proc_create);
        mme_app_delete_s11_procedure_create_bearer(ue_context_p);
      }
    }
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_handle_create_dedicated_bearer_rej (itti_mme_app_create_dedicated_bearer_rej_t   * const create_dedicated_bearer_rej)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  struct ue_context_s                 *ue_context_p = NULL;

  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, create_dedicated_bearer_rej->ue_id);

  if (ue_context_p == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: " MME_UE_S1AP_ID_FMT "\n", create_dedicated_bearer_rej->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  if (!ue_context_p->is_s1_ue_context_release) {

    // TODO:
    // Actually do it simple, because it appear we have to wait for NAS procedure reworking (work in progress on another branch)
    // for responding to S11 without mistakes (may be the create bearer procedure can be impacted by a S1 ue context release or
    // a UE originating  NAS procedure)
    mme_app_s11_proc_create_bearer_t *s11_proc_create = mme_app_get_s11_procedure_create_bearer(ue_context_p);
    if (s11_proc_create) {
      ebi_t ebi = create_dedicated_bearer_rej->ebi;

      s11_proc_create->num_status_received++;
      s11_proc_create->bearer_status[EBI_TO_INDEX(ebi)] = S11_PROC_BEARER_FAILED;
      // if received all bearers creation results
      if (s11_proc_create->num_status_received == s11_proc_create->num_bearers) {
        mme_app_s11_procedure_create_bearer_send_response(ue_context_p, s11_proc_create);
        mme_app_delete_s11_procedure_create_bearer(ue_context_p);
      }
    }
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
void mme_app_send_s1ap_handover_preparation_failure(mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id, sctp_assoc_id_t assoc_id, MMECause_t mmeCause){
  OAILOG_FUNC_IN (LOG_MME_APP);
  /** Send a S1AP HANDOVER PREPARATION FAILURE TO THE SOURCE ENB. */
  MessageDef * message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_HANDOVER_PREPARATION_FAILURE);
  DevAssert (message_p != NULL);
  DevAssert(mmeCause != REQUEST_ACCEPTED);

  itti_s1ap_handover_preparation_failure_t *s1ap_handover_preparation_failure_p = &message_p->ittiMsg.s1ap_handover_preparation_failure;
  memset ((void*)s1ap_handover_preparation_failure_p, 0, sizeof (itti_s1ap_handover_preparation_failure_t));

  /** Set the identifiers. */
  s1ap_handover_preparation_failure_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  s1ap_handover_preparation_failure_p->enb_ue_s1ap_id = enb_ue_s1ap_id;
  s1ap_handover_preparation_failure_p->assoc_id = assoc_id;
  /** Set the negative cause. */
  s1ap_handover_preparation_failure_p->cause = mmeCause;

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
  ue_context_t                           *ue_context_p = NULL;
  emm_data_context_t                     *ue_nas_ctx = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id);

  if (ue_context_p == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "UE context doesn't exist for UE %06" PRIX32 "/dec%u\n", mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Get the EMM Context too for the AS security parameters. */
  ue_nas_ctx = emm_data_context_get(&_emm_data, mme_ue_s1ap_id);
  if(!ue_nas_ctx || ue_nas_ctx->_tai_list.n_tais == 0){
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

  bearer_id = ue_context_p->default_bearer_id;
  current_bearer_p =  mme_app_is_bearer_context_in_list(ue_context_p->mme_ue_s1ap_id, bearer_id);
  path_switch_req_ack_p->eps_bearer_id = bearer_id;

  /** Set all active bearers to be switched. */
  path_switch_req_ack_p->bearer_ctx_to_be_switched_list.n_bearers   = ue_context_p->nb_ue_bearer_ctxs;
  path_switch_req_ack_p->bearer_ctx_to_be_switched_list.bearer_ctxs = (void*)&ue_context_p->bearer_ctxs;

  hash_table_ts_t * bearer_contexts1 = (hash_table_ts_t*)path_switch_req_ack_p->bearer_ctx_to_be_switched_list.bearer_ctxs;

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
  mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context_p, ECM_CONNECTED);

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
void mme_app_send_s1ap_path_switch_request_failure(mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id, sctp_assoc_id_t assoc_id, MMECause_t mmeCause){
  OAILOG_FUNC_IN (LOG_MME_APP);
  /** Send a S1AP Path Switch Request Failure TO THE TARGET ENB. */
  MessageDef * message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_PATH_SWITCH_REQUEST_FAILURE);
  DevAssert (message_p != NULL);
  DevAssert(mmeCause != REQUEST_ACCEPTED);

  itti_s1ap_path_switch_request_failure_t *s1ap_path_switch_request_failure_p = &message_p->ittiMsg.s1ap_path_switch_request_failure;
  memset ((void*)s1ap_path_switch_request_failure_p, 0, sizeof (itti_s1ap_path_switch_request_failure_t));

  /** Set the identifiers. */
  s1ap_path_switch_request_failure_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  s1ap_path_switch_request_failure_p->enb_ue_s1ap_id = enb_ue_s1ap_id;
  s1ap_path_switch_request_failure_p->assoc_id = assoc_id; /**< To whatever the new SCTP association is. */
  /** Set the negative cause. */
  s1ap_path_switch_request_failure_p->cause = mmeCause;

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "MME_APP Sending S1AP PATH_SWITCH_REQUEST_FAILURE");
  /** Sending a message to S1AP. */
  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_path_switch_req(
  const itti_mme_app_path_switch_req_t * const path_switch_req_pP
  )
{
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  OAILOG_DEBUG (LOG_MME_APP, "Received MME_APP_PATH_SWITCH_REQ from S1AP\n");
  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, path_switch_req_pP->mme_ue_s1ap_id);

  if (ue_context_p == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: %08x %d(dec)\n", path_switch_req_pP->mme_ue_s1ap_id, path_switch_req_pP->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "MME_APP_PATH_SWITCH_REQ Unknown ue %u", path_switch_req_pP->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  enb_s1ap_id_key_t                       enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;

  /** Update the ENB_ID_KEY. */
  MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, path_switch_req_pP->enb_id, path_switch_req_pP->enb_ue_s1ap_id);
  // Update enb_s1ap_id_key in hashtable
  mme_ue_context_update_coll_keys( &mme_app_desc.mme_ue_contexts,
      ue_context_p,
      enb_s1ap_id_key,
      ue_context_p->mme_ue_s1ap_id,
      ue_context_p->imsi,
      ue_context_p->mme_s11_teid,
      ue_context_p->local_mme_s10_teid,
      &ue_context_p->guti);

  // Set the handover flag, check that no handover exists.
  ue_context_p->enb_ue_s1ap_id    = path_switch_req_pP->enb_ue_s1ap_id;
  ue_context_p->sctp_assoc_id_key = path_switch_req_pP->sctp_assoc_id;
  //  sctp_stream_id_t        sctp_stream;
  uint16_t encryption_algorithm_capabilities;
  uint16_t integrity_algorithm_capabilities;
  // todo: update them from the X2 message!
  if(emm_data_context_update_security_parameters(path_switch_req_pP->mme_ue_s1ap_id, &encryption_algorithm_capabilities, &integrity_algorithm_capabilities) != RETURNok){
    OAILOG_ERROR(LOG_MME_APP, "Error updating AS security parameters for UE with ueId: " MME_UE_S1AP_ID_FMT ". \n", path_switch_req_pP->mme_ue_s1ap_id);
    mme_app_send_s1ap_path_switch_request_failure(path_switch_req_pP->mme_ue_s1ap_id, path_switch_req_pP->enb_ue_s1ap_id, path_switch_req_pP->sctp_assoc_id, SYSTEM_FAILURE);
    /** Implicitly detach the UE --> If EMM context is missing, still continue with the resource removal. */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context_p->mme_ue_s1ap_id;
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
 OAILOG_INFO(LOG_MME_APP, "Successfully updated AS security parameters for UE with ueId: " MME_UE_S1AP_ID_FMT " for X2 handover. \n", path_switch_req_pP->mme_ue_s1ap_id);

 // Stop Initial context setup process guard timer,if running todo: path switch request?
  if (ue_context_p->path_switch_req_timer.id != MME_APP_TIMER_INACTIVE_ID) {
    if (timer_remove(ue_context_p->path_switch_req_timer.id)) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to stop Path Switch Request timer for UE id  %d \n", ue_context_p->mme_ue_s1ap_id);
    }
    ue_context_p->path_switch_req_timer.id = MME_APP_TIMER_INACTIVE_ID;
  }
  message_p = itti_alloc_new_message (TASK_MME_APP, S11_MODIFY_BEARER_REQUEST);
  AssertFatal (message_p , "itti_alloc_new_message Failed");
  itti_s11_modify_bearer_request_t *s11_modify_bearer_request = &message_p->ittiMsg.s11_modify_bearer_request;
  memset ((void *)s11_modify_bearer_request, 0, sizeof (*s11_modify_bearer_request));
  s11_modify_bearer_request->peer_ip = mme_config.ipv4.sgw_s11;
  s11_modify_bearer_request->teid = ue_context_p->sgw_s11_teid;
  s11_modify_bearer_request->local_teid = ue_context_p->mme_s11_teid;
  /*
   * Delay Value in integer multiples of 50 millisecs, or zero
   */
  s11_modify_bearer_request->delay_dl_packet_notif_req = 0;  // TO DO
  s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].eps_bearer_id = path_switch_req_pP->eps_bearer_id;
  memcpy (&s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].s1_eNB_fteid,
      &path_switch_req_pP->bearer_s1u_enb_fteid,
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
  ue_context_p->pending_x2_handover = true;
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Send an S10 Forward Relocation response with error cause.
 * It shall not trigger creating a local S10 tunnel.
 * Parameter is the TEID & IP of the SOURCE-MME.
 */
static
void mme_app_send_s10_forward_relocation_response_err(teid_t mme_source_s10_teid, uint32_t mme_source_ipv4_address, void *trxn,  MMECause_t mmeCause){
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
  forward_relocation_response_p->cause = mmeCause;
  forward_relocation_response_p->trxn  = trxn;

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "MME_APP Sending S10 FORWARD_RELOCATION_RESPONSE_ERR to source TEID " TEID_FMT ". \n", mme_source_s10_teid);

  /** Sending a message to S10. */
  itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_handover_required(
     const itti_s1ap_handover_required_t * const handover_required_pP
    )   {
  OAILOG_FUNC_IN (LOG_MME_APP);

  emm_data_context_t                     *ue_nas_ctx = NULL;
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;

  OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_HANDOVER_REQUIRED from S1AP\n");
  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, handover_required_pP->mme_ue_s1ap_id);

  if (ue_context_p == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: %08x %d(dec)\n", handover_required_pP->mme_ue_s1ap_id, handover_required_pP->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_REQUIRED Unknown ue %u", handover_required_pP->mme_ue_s1ap_id);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, CONTEXT_NOT_FOUND);
    /** Remove the allocated resources in the ITTI message (bstrings). */
    bdestroy(handover_required_pP->eutran_source_to_target_container);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  if (ue_context_p->mm_state != UE_REGISTERED) {
    OAILOG_ERROR (LOG_MME_APP, "UE with ue_id " MME_UE_S1AP_ID_FMT " is not in UE_REGISTERED state. "
        "Rejecting the Handover Preparation. \n", ue_context_p->mme_ue_s1ap_id);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, REQUEST_REJECTED);
    /** No change in the UE context needed. */
    /** Remove the allocated resources in the ITTI message (bstrings). */
    bdestroy(handover_required_pP->eutran_source_to_target_container);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  /**
   * Staying in the same state (UE_REGISTERED).
   * If the GUTI is not found, first send a preparation failure, then implicitly detach.
   */
  ue_nas_ctx = emm_data_context_get_by_guti (&_emm_data, &ue_context_p->guti);
  /** Check that the UE NAS context != NULL. */
  if (!ue_nas_ctx || ue_nas_ctx->_emm_fsm_status != EMM_REGISTERED) {
    OAILOG_ERROR (LOG_MME_APP, "EMM context for UE with ue_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " is not in EMM_REGISTERED state or not existing. "
        "Rejecting the Handover Preparation. \n", handover_required_pP->mme_ue_s1ap_id, (ue_nas_ctx) ? ue_nas_ctx->_imsi64 : "NULL");
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, SYSTEM_FAILURE);
    /** Implicitly detach the UE --> If EMM context is missing, still continue with the resource removal. */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context_p->mme_ue_s1ap_id;
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
    /** Remove the allocated resources in the ITTI message (bstrings). */
    bdestroy(handover_required_pP->eutran_source_to_target_container);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Set the target TAI and enb_id, to use them if a Handover-Cancel message comes. */
  memcpy(&ue_context_p->pending_handover_target_tai, &handover_required_pP->selected_tai, sizeof(tai_t));
  ue_context_p->pending_handover_target_enb_id = handover_required_pP->global_enb_id.cell_identity.enb_id;
  ue_context_p->pending_handover_source_enb_id = ue_context_p->e_utran_cgi.cell_identity.enb_id;

  /** Update Security Parameters and send them to the target MME or target eNB. */
  /** Get the updated security parameters from EMM layer directly. Else new states and ITTI messages are necessary. */
  uint16_t encryption_algorithm_capabilities;
  uint16_t integrity_algorithm_capabilities;
  if(emm_data_context_update_security_parameters(handover_required_pP->mme_ue_s1ap_id, &encryption_algorithm_capabilities, &integrity_algorithm_capabilities) != RETURNok){
    OAILOG_ERROR(LOG_MME_APP, "Error updating AS security parameters for UE with ueId: " MME_UE_S1AP_ID_FMT ". \n", handover_required_pP->mme_ue_s1ap_id);
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, SYSTEM_FAILURE);
    /** Implicitly detach the UE --> If EMM context is missing, still continue with the resource removal. */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context_p->mme_ue_s1ap_id;
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
    /** Remove the allocated resources in the ITTI message (bstrings). */
    bdestroy(handover_required_pP->eutran_source_to_target_container);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  OAILOG_INFO(LOG_MME_APP, "Successfully updated AS security parameters for UE with ueId: " MME_UE_S1AP_ID_FMT ". \n", handover_required_pP->mme_ue_s1ap_id);

  /** Check if the destination eNodeB is attached at the same or another MME. */
  if (mme_app_check_ta_local(&handover_required_pP->selected_tai.plmn, handover_required_pP->selected_tai.tac)) {
    /** Check if the eNB with the given eNB-ID is served. */
    if(s1ap_is_enb_id_in_list(handover_required_pP->global_enb_id.cell_identity.enb_id) != NULL){
      OAILOG_DEBUG (LOG_MME_APP, "Target ENB_ID %d of target TAI " TAI_FMT " is served by current MME. \n", handover_required_pP->global_enb_id.cell_identity.enb_id, handover_required_pP->selected_tai);
      ue_context_p->pending_s1ap_source_to_target_handover_container = handover_required_pP->eutran_source_to_target_container;
      /** Prepare a Handover Request, keep the transparent container for now, it will be purged together with the free method of the S1AP message. */
      mme_app_send_s1ap_handover_request(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->global_enb_id.cell_identity.enb_id,
          encryption_algorithm_capabilities,
          integrity_algorithm_capabilities,
          ue_nas_ctx->_vector[ue_nas_ctx->_security.vector_index].nh_conj,
          ue_nas_ctx->_security.ncc);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }else{
      /** The target eNB-ID is not served by this MME. */
      OAILOG_DEBUG (LOG_MME_APP, "Target ENB_ID %d of target TAI " TAI_FMT " is NOT served by current MME. \n", handover_required_pP->global_enb_id.cell_identity.enb_id, handover_required_pP->selected_tai);
      /** Send a Handover Preparation Failure back. */
      mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, SYSTEM_FAILURE);
      bdestroy(handover_required_pP->eutran_source_to_target_container);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
  }
  OAILOG_DEBUG (LOG_MME_APP, "Target TA  "TAI_FMT " is NOT served by current MME. Searching for a neighboring MME. \n", handover_required_pP->selected_tai);


  int ngh_index = mme_app_check_target_tai_neighboring_mme(&handover_required_pP->selected_tai);
  if(ngh_index == -1){
    OAILOG_DEBUG (LOG_MME_APP, "The selected TAI " TAI_FMT " is not configured as an S10 MME neighbor. "
        "Not proceeding with the handover formme_ue_s1ap_id in list of UE: %08x %d(dec)\n",
        handover_required_pP->selected_tai, handover_required_pP->mme_ue_s1ap_id, handover_required_pP->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_REQUIRED Unknown ue %u", handover_required_pP->mme_ue_s1ap_id);
    /** Send a Handover Preparation Failure back. */
    mme_app_send_s1ap_handover_preparation_failure(handover_required_pP->mme_ue_s1ap_id, handover_required_pP->enb_ue_s1ap_id, handover_required_pP->sctp_assoc_id, SYSTEM_FAILURE);
    bdestroy(handover_required_pP->eutran_source_to_target_container);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /** Prepare a forward relocation message to the TARGET-MME. */
  message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_RELOCATION_REQUEST);
  DevAssert (message_p != NULL);
  itti_s10_forward_relocation_request_t *forward_relocation_request_p = &message_p->ittiMsg.s10_forward_relocation_request;
  memset ((void*)forward_relocation_request_p, 0, sizeof (itti_s10_forward_relocation_request_t));
  forward_relocation_request_p->teid = 0;
  forward_relocation_request_p->peer_ip = mme_config.nghMme.nghMme[ngh_index].ipAddr;
  /** IMSI. */
  IMSI64_TO_STRING (ue_context_p->imsi, (char *)forward_relocation_request_p->imsi.digit);
  // message content was set to 0
  forward_relocation_request_p->imsi.length = strlen ((const char *)forward_relocation_request_p->imsi.digit);
  // message content was set to 0
  /** Set the Source MME_S10_FTEID the same as in S11. */
  teid_t local_teid = 0x0;
  do{
    local_teid = (teid_t) (rand() % 0xFFFFFFFF);
  }while(mme_ue_context_exists_s10_teid(&mme_app_desc.mme_ue_contexts, local_teid) != NULL);

  if(!ue_context_p->local_mme_s10_teid){
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
    mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context_p,
        ue_context_p->enb_s1ap_id_key,
        ue_context_p->mme_ue_s1ap_id,
        ue_context_p->imsi,
        ue_context_p->mme_s11_teid,       // mme_s11_teid is new
        forward_relocation_request_p->s10_source_mme_teid.teid,       // set with forward_relocation_request!
        &ue_context_p->guti);
  }else{
    OAILOG_INFO (LOG_MME_APP, "A S10 Local TEID " TEID_FMT " already exists. Not reallocating for UE: %08x %d(dec)\n",
        ue_context_p->local_mme_s10_teid, handover_required_pP->mme_ue_s1ap_id, handover_required_pP->mme_ue_s1ap_id);
    OAI_GCC_DIAG_OFF(pointer-to-int-cast);
    forward_relocation_request_p->s10_source_mme_teid.teid = ue_context_p->local_mme_s10_teid;
    OAI_GCC_DIAG_ON(pointer-to-int-cast);
    forward_relocation_request_p->s10_source_mme_teid.interface_type = S10_MME_GTP_C;
    mme_config_read_lock (&mme_config);
    forward_relocation_request_p->s10_source_mme_teid.ipv4_address = mme_config.ipv4.s10;
    mme_config_unlock (&mme_config);
    forward_relocation_request_p->s10_source_mme_teid.ipv4 = 1;
  }
  /** Set the SGW_S11_FTEID the same as in S11. */
  OAI_GCC_DIAG_OFF(pointer-to-int-cast);
  forward_relocation_request_p->s11_sgw_teid.teid = ue_context_p->sgw_s11_teid;
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
  /** Set the Target Id. */
  forward_relocation_request_p->target_identification.target_id.macro_enb_id.tac    = handover_required_pP->selected_tai.tac;
  forward_relocation_request_p->target_identification.target_id.macro_enb_id.enb_id = handover_required_pP->global_enb_id.cell_identity.enb_id;

  /** todo: Set the TAI and and the global-eNB-Id. */
  // memcpy(&forward_relocation_request_p->selected_tai, &handover_required_pP->selected_tai, sizeof(handover_required_pP->selected_tai));
  // memcpy(&forward_relocation_request_p->global_enb_id, &handover_required_pP->global_enb_id, sizeof(handover_required_pP->global_enb_id));

  /** Set the PDN connections. */

  /** Set the PDN_CONNECTIONS IE. */
  DevAssert(mme_app_set_pdn_connections(&forward_relocation_request_p->pdn_connections, ue_context_p) == RETURNok);

  /** Set the MM_UE_EPS_CONTEXT. */
  DevAssert(mme_app_set_ue_eps_mm_context(&forward_relocation_request_p->ue_eps_mm_context, ue_context_p, ue_nas_ctx) == RETURNok);

  /** Put the E-Utran transparent container. */
  forward_relocation_request_p->eutran_container.container_type = 3;
  forward_relocation_request_p->eutran_container.container_value = handover_required_pP->eutran_source_to_target_container;
  if (forward_relocation_request_p->eutran_container.container_value == NULL){
    OAILOG_ERROR (LOG_MME_APP, " NULL UE transparent container\n" );
    OAILOG_FUNC_OUT (LOG_MME_APP);
    // todo: does it set the size parameter?
  }
  /** Will be deallocated later after S10 message is encoded. */
  /** Send the Forward Relocation Message to S11. */
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME,  MSC_S10_MME ,
      NULL, 0, "0 NAS_UE_RELOCATION_REQ for UE %d \n", handover_required_pP->mme_ue_s1ap_id);
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

  emm_data_context_t                     *ue_nas_ctx = NULL;
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;

  OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_HANDOVER_CANCEL from S1AP\n");
  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, handover_cancel_pP->mme_ue_s1ap_id);
  if (ue_context_p == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: %08x %d(dec)\n", handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->mme_ue_s1ap_id);
    MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_CANCEL Unknown ue %u", handover_cancel_pP->mme_ue_s1ap_id);
    /** Ignoring the message. */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  if (ue_context_p->mm_state != UE_REGISTERED) {
    OAILOG_ERROR (LOG_MME_APP, "UE with ue_id " MME_UE_S1AP_ID_FMT " is not in UE_REGISTERED state. "
        "Sending Cancel Acknowledgement and implicitly removing the UE. \n", ue_context_p->mme_ue_s1ap_id);
    mme_app_send_s1ap_handover_cancel_acknowledge(handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->enb_ue_s1ap_id, handover_cancel_pP->assoc_id);
    /** Purge the invalid UE context. */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context_p->mme_ue_s1ap_id;
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
    /** Remove the allocated resources in the ITTI message (bstrings). */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /**
   * Staying in the same state (UE_REGISTERED).
   * If the GUTI is not found, first send a preparation failure, then implicitly detach.
   */
  ue_nas_ctx = emm_data_context_get_by_guti (&_emm_data, &ue_context_p->guti);
  /** Check that the UE NAS context != NULL. */
  if (!ue_nas_ctx || ue_nas_ctx->_emm_fsm_status != EMM_REGISTERED) {
    OAILOG_ERROR (LOG_MME_APP, "EMM context for UE with ue_id " MME_UE_S1AP_ID_FMT " IMSI " IMSI_64_FMT " is not in EMM_REGISTERED state or not existing. "
        "Sending Cancel Acknowledge back and implicitly detaching the UE. \n", handover_cancel_pP->mme_ue_s1ap_id, (ue_nas_ctx) ? ue_nas_ctx->_imsi64 : INVALID_IMSI64);
    mme_app_send_s1ap_handover_cancel_acknowledge(handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->enb_ue_s1ap_id, handover_cancel_pP->assoc_id);
    /** Implicitly detach the UE --> If EMM context is missing, still continue with the resource removal. */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context_p->mme_ue_s1ap_id;
    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
    /** Remove the allocated resources in the ITTI message (bstrings). */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  /**
   * UE will always stay in EMM-REGISTERED mode until S10 Handover is completed (CANCEL_LOCATION_REQUEST).
   * Just checking S10 is not enough. The UE may have handovered to the target-MME and then handovered to another eNB in the TARGET-MME.
   * Check if there is a target-TAI registered for the UE is in this or another MME.
   */
  /** Check if the destination eNodeB is attached at the same or another MME. */
  if (mme_app_check_ta_local(&ue_context_p->pending_handover_target_tai.plmn, ue_context_p->pending_handover_target_tai.tac)){
    /** Check if the eNB with the given eNB-ID is served. */
    if(s1ap_is_enb_id_in_list(ue_context_p->pending_handover_target_enb_id) != NULL){
      OAILOG_DEBUG (LOG_MME_APP, "Target ENB_ID %d of target TAI " TAI_FMT " is served by current MME. \n",
          ue_context_p->pending_handover_target_enb_id, ue_context_p->pending_handover_target_tai);
      /**
       * Check if there already exists a UE-Reference to the target cell.
       * If so, this means that HANDOVER_REQUEST_ACKNOWLEDGE is already received.
       * It is so far gone in the handover process. We will send CANCEL-ACK and implicitly detach the UE.
       */
      if(ue_context_p->pending_handover_enb_ue_s1ap_id != 0 && ue_context_p->enb_ue_s1ap_id != ue_context_p->pending_handover_enb_ue_s1ap_id){
        ue_description_t * ue_reference = s1ap_is_enb_ue_s1ap_id_in_list_per_enb(ue_context_p->pending_handover_enb_ue_s1ap_id, ue_context_p->pending_handover_target_enb_id);
        if(ue_reference != NULL){
          /** UE Reference to the target eNB found. Sending a UE Context Release to the target MME BEFORE a HANDOVER_REQUEST_ACK arrives. */
          OAILOG_INFO(LOG_MME_APP, "Sending UE-Context-Release-Cmd to the target eNB %d for the UE-ID " MME_UE_S1AP_ID_FMT " and pending_enbUeS1apId " ENB_UE_S1AP_ID_FMT " (current enbUeS1apId) " ENB_UE_S1AP_ID_FMT ". \n.",
              ue_context_p->pending_handover_target_enb_id, ue_context_p->mme_ue_s1ap_id, ue_context_p->pending_handover_enb_ue_s1ap_id, ue_context_p->enb_ue_s1ap_id);
          mme_app_itti_ue_context_release (ue_context_p->mme_ue_s1ap_id, ue_context_p->pending_handover_enb_ue_s1ap_id, S1AP_HANDOVER_CANCELLED, ue_context_p->pending_handover_target_enb_id);
          ue_context_p->pending_handover_enb_ue_s1ap_id = 0;
          ue_context_p->ue_context_rel_cause = S1AP_HANDOVER_CANCELLED;
          /**
           * No pending transparent container expected.
           * We directly respond with CANCEL_ACK, since when UE_CTX_RELEASE_CMPLT comes from targetEnb, we don't know if the current enb is target or source,
           * so we cannot decide there to send CANCEL_ACK without a flag.
           */
          OAILOG_FUNC_OUT (LOG_MME_APP);
        }
        ue_context_p->pending_handover_enb_ue_s1ap_id = 0;
      }
      OAILOG_INFO(LOG_MME_APP, "No target UE reference is established yet. No S1AP UE context removal needed for UE-ID " MME_UE_S1AP_ID_FMT ". Responding with HO_CANCEL_ACK back to enbUeS1apId " ENB_UE_S1AP_ID_FMT ". \n.",
          ue_context_p->mme_ue_s1ap_id, ue_context_p->enb_ue_s1ap_id);
      /** Send a HO-CANCEL-ACK to the source-MME. */
      mme_app_send_s1ap_handover_cancel_acknowledge(handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->enb_ue_s1ap_id, handover_cancel_pP->assoc_id);
      OAILOG_FUNC_OUT (LOG_MME_APP);
      /** Keeping the UE context as it is. */
    }else{
      OAILOG_ERROR(LOG_MME_APP, "No registered eNB found with target eNB-ID %d in target-TAI " TAI_FMT ". "
          "Cannot release resources in the target-ENB for UE-ID " MME_UE_S1AP_ID_FMT "."
          "Sending CANCEL-ACK back and leaving the UE as it is. \n",
          ue_context_p->pending_handover_target_enb_id, ue_context_p->pending_handover_target_tai, ue_context_p->mme_ue_s1ap_id);
      //todo: eventually perform an implicit detach!
      mme_app_send_s1ap_handover_cancel_acknowledge(handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->enb_ue_s1ap_id, handover_cancel_pP->assoc_id);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
  }

  /** Check for a neighboring MME. */
  int ngh_index = mme_app_check_target_tai_neighboring_mme(&ue_context_p->pending_handover_target_tai);
  if(ngh_index == -1){
    OAILOG_DEBUG (LOG_MME_APP, "The selected TAI " TAI_FMT " is not configured as an S10 MME neighbor. "
        "Not sending a S10 Relocation Cancel Request to the target MME for UE: " MME_UE_S1AP_ID_FMT ". \n",
        handover_cancel_pP->mme_ue_s1ap_id);
    /** Send a Handover Preparation Failure back. */
    mme_app_send_s1ap_handover_cancel_acknowledge(handover_cancel_pP->mme_ue_s1ap_id, handover_cancel_pP->enb_ue_s1ap_id, handover_cancel_pP->assoc_id);
    if(ue_context_p->pending_s1ap_source_to_target_handover_container)
      bdestroy(ue_context_p->pending_s1ap_source_to_target_handover_container);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  OAILOG_DEBUG (LOG_MME_APP, "The selected TAI " TAI_FMT " is configured as an S10 MME neighbor. "
      "Proceeding with S10 Relocation Cancel Request to the target MME for UE: " MME_UE_S1AP_ID_FMT ". \n",
      handover_cancel_pP->mme_ue_s1ap_id);

  /**
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
  relocation_cancel_request_p->teid = ue_context_p->remote_mme_s10_teid; /**< May or may not be 0. */
  relocation_cancel_request_p->local_teid = ue_context_p->local_mme_s10_teid; /**< May or may not be 0. */
  // todo: check the table!
  relocation_cancel_request_p->peer_ip = mme_config.nghMme.nghMme[ngh_index].ipAddr;
  /** IMSI. */
  IMSI64_TO_STRING (ue_context_p->imsi, (char *)relocation_cancel_request_p->imsi.digit);
  // message content was set to 0
  relocation_cancel_request_p->imsi.length = strlen ((const char *)relocation_cancel_request_p->imsi.digit);
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 RELOCATION_CANCEL_REQUEST_MESSAGE");
  itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
  OAILOG_DEBUG(LOG_MME_APP, "Successfully sent S10 RELOCATION_CANCEL_REQUEST to the target MME for the UE with IMSI " IMSI_64_FMT " and id " MME_UE_S1AP_ID_FMT ". "
      "Continuing with HO-CANCEL PROCEDURE. \n", ue_context_p->mme_ue_s1ap_id, ue_context_p->imsi);

  // todo: remove the pending container!

  /**
   * Update the coll_keys with the IMSI.
   */
  mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context_p,
      INVALID_ENB_UE_S1AP_ID_KEY,
      ue_context_p->mme_ue_s1ap_id,
      ue_context_p->imsi,      /**< New IMSI. */
      ue_context_p->mme_s11_teid,
      0,
      &ue_context_p->guti);

  /**
   * Not handling S10 response messages, to also drop other previous S10 messages (FW_RELOC_RSP).
   * Send a HO-CANCEL-ACK to the source-MME. */
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
     const itti_s10_forward_relocation_request_t * const forward_relocation_request_pP
    )
{
 MessageDef                             *message_p = NULL;
 struct ue_context_s                    *ue_context_p = NULL;
 uint64_t                                imsi = 0;
 int                                     rc = RETURNok;

 OAILOG_FUNC_IN (LOG_MME_APP);

 IMSI_STRING_TO_IMSI64 (&forward_relocation_request_pP->imsi, &imsi);
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
     bdestroy(forward_relocation_request_pP->eutran_container.container_value);
     OAILOG_FUNC_OUT (LOG_MME_APP);
   }
 }else{
   /** The target PLMN and TAC are not served by this MME. */
   OAILOG_ERROR(LOG_MME_APP, "TARGET TAC " TAC_FMT " is NOT served by current MME. \n", forward_relocation_request_pP->target_identification.target_id.macro_enb_id.tac);
   mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, forward_relocation_request_pP->s10_source_mme_teid.ipv4_address, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
   bdestroy(forward_relocation_request_pP->eutran_container.container_value);
   /** No UE context or tunnel endpoint is allocated yet. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** Check if the UE exists. */
 ue_context_p = mme_ue_context_exists_imsi(&mme_app_desc.mme_ue_contexts, imsi);
 if (ue_context_p != NULL) {
   OAILOG_WARNING(LOG_MME_APP, "An UE MME context for the UE with IMSI " IMSI_64_FMT " already exists. \n", imsi);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S10_FORWARD_RELOCATION_REQUEST. Already existing UE " IMSI_64_FMT, imsi);
   /** If a CLR flag is existing, reset it. */
   if(ue_context_p->pending_clear_location_request){
     OAILOG_INFO(LOG_MME_APP, "UE_CONTEXT already had a CLR flag set for UE with IMSI " IMSI_64_FMT " and " MME_UE_S1AP_ID_FMT " already. Resetting. \n", imsi, ue_context_p->mme_ue_s1ap_id);
     ue_context_p->pending_clear_location_request = false;
   }
   /**
    * Not stopping the MME_MOBILITY COMPLETION timer, if running, it would stop the S1AP release timer for the source MME side.
    * We will just clear the pending CLR flag with the NAS invalidation. UE reference should stil be removed but the CLR should be disregarded.
    * If the CLR flag still arrives after that, we had bad luck, it will remove the context.
    */
   if (ue_context_p->mme_mobility_completion_timer.id != MME_APP_TIMER_INACTIVE_ID) {
     OAILOG_INFO(LOG_MME_APP, "The MME mobility completion timer was set for UE with IMSI " IMSI_64_FMT " and " MME_UE_S1AP_ID_FMT ". Keeping it. \n", imsi, ue_context_p->mme_ue_s1ap_id);
     /** Inform the S1AP layer that the UE context could be timeoutet. */

   }
   /** Invalidate the enb_ue_s1ap_id and the key. */
   ue_context_p->enb_ue_s1ap_id = 0;
   /**
     * Update the coll_keys with the IMSI.
     */
    mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context_p,
        INVALID_ENB_UE_S1AP_ID_KEY,
        ue_context_p->mme_ue_s1ap_id,
        ue_context_p->imsi,      /**< New IMSI. */
        ue_context_p->mme_teid_s11,
        &ue_context_p->guti);

    /** Synchronously send a UE context Release Request (implicit - no response expected). */
    // todo: send S1AP_NETWORK_ERROR to UE
    mme_app_itti_ue_context_release (ue_context_p->mme_ue_s1ap_id, ue_context_p->enb_ue_s1ap_id, S1AP_NETWORK_ERROR, ue_context_p->e_utran_cgi.cell_identity.enb_id); /**< the source side should be ok. */
 }
 /** Check that also an NAS UE context exists. */
 emm_data_context_t *ue_nas_ctx = NULL;
// if(ue_context_p){
   ue_nas_ctx = emm_data_context_get_by_imsi (&_emm_data, imsi);
// }
 if (ue_nas_ctx) {
   OAILOG_INFO(LOG_MME_APP, "A valid  NAS context exist for UE with IMSI " IMSI_64_FMT " and " MME_UE_S1AP_ID_FMT " already, but this re-registration part is not implemented yet. \n", imsi, ue_nas_ctx->ue_id);
   if(ue_context_p){
     /* Check if a handover procedure exists. */
     if(is_handover_procedure(ue_context_p)){
       OAILOG_WARNING(LOG_MME_APP, "A handover procedure already exists for UE " MME_UE_S1AP_ID_FMT " in state %d. "
           "Dropping newly received handover request. \n.",
           ue_context_p->mme_ue_s1ap_id, ue_context_p->mm_state);
       /*
        * The received message should be removed automatically.
        * todo: the message will have a new transaction, removing it not that it causes an assert?
        */
       OAILOG_FUNC_OUT(LOG_MME_APP);
     }else{
       OAILOG_INFO(LOG_MME_APP, "No handover procedure exists for the existing UE context with ueId " MME_UE_S1AP_ID_FMT". \n", ue_context_p->mme_ue_s1ap_id);
     }
     ue_context_p->s1_ue_context_release_cause = S1AP_INVALIDATE_NAS;  /**< This should remove the NAS context and invalidate the timers. */
   }
   message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
   DevAssert (message_p != NULL);
   itti_nas_implicit_detach_ue_ind_t *nas_implicit_detach_ue_ind_p = &message_p->ittiMsg.nas_implicit_detach_ue_ind;
   memset ((void*)nas_implicit_detach_ue_ind_p, 0, sizeof (itti_nas_implicit_detach_ue_ind_t));
   message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_nas_ctx->ue_id;
   itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
   OAILOG_INFO(LOG_MME_APP, "Informed NAS about the invalidated NAS context. Continuing with handover for UE with IMSI " IMSI_64_FMT " and " MME_UE_S1AP_ID_FMT " already. \n", imsi, ue_nas_ctx->ue_id);
 }else{
   OAILOG_WARNING(LOG_MME_APP, "No  valid UE context and NAS context exist for UE with IMSI " IMSI_64_FMT ". Continuing with the handover. \n", imsi);
   if(ue_context_p)
     ue_context_p->pending_clear_location_request = false;
 }
 OAILOG_INFO(LOG_MME_APP, "Received a FORWARD_RELOCATION_REQUEST for new UE with IMSI " IMSI_64_FMT ". \n", imsi);
 /*
  * Create a new handover procedure, no matter a UE context exists or not.
  * Store the transaction in it.
  * todo: also check if a handover procedure exists, if so drop the next received forward relocation request (no check for equivalence).
  */
 mme_app_create_handover_procedure(&forward_relocation_request_pP->s10_source_mme_teid,
     &forward_relocation_request_pP->target_identification,
     &forward_relocation_request_pP->eutran_container,
     &forward_relocation_request_pP->f_cause,
     &forward_relocation_request_pP->peer_ip, // todo: if they differ etc..
     forward_relocation_request_pP->peer_port,
     &forward_relocation_request_pP->imsi,
     forward_relocation_request_pP->trxn);
 // todo: check with asserts  that procedure exists

 /* From the PDN Connections IE allocate a PDN session for the UE conetxt and also create bearers.
  * Use the bearer to send later CSR to SAE-GW.
  * Get and handle the PDN Connection element as pending PDN connection element.
  */
 if(!ue_context_p){
   /** Establish the UE context. */
    OAILOG_DEBUG (LOG_MME_APP, "Creating a new UE context for the UE with incoming S1AP Handover via S10 for IMSI " IMSI_64_FMT ". \n", imsi);
    if ((ue_context_p = mme_create_new_ue_context ()) == NULL) {
      /** Send a negative response before crashing. */
      mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, forward_relocation_request_pP->s10_source_mme_teid.ipv4_address, forward_relocation_request_pP->trxn, SYSTEM_FAILURE);
      bdestroy(forward_relocation_request_pP->eutran_container.container_value);
      /**
       * Error during UE context malloc
       */
      DevMessage ("Error while mme_create_new_ue_context");
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
    ue_context_p->mme_ue_s1ap_id = mme_app_ctx_get_new_ue_id ();
    if (ue_context_p->mme_ue_s1ap_id == INVALID_MME_UE_S1AP_ID) {
      OAILOG_CRITICAL (LOG_MME_APP, "MME_APP_FORWARD_RELOCATION_REQUEST. MME_UE_S1AP_ID allocation Failed.\n");
      /** Deallocate the ue context and remove from MME_APP map. */
      mme_remove_ue_context (&mme_app_desc.mme_ue_contexts, ue_context_p);
      /** Send back failure. */
      mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, forward_relocation_request_pP->s10_source_mme_teid.ipv4_address, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
      bdestroy(forward_relocation_request_pP->eutran_container.container_value);
      OAILOG_FUNC_OUT (LOG_MME_APP);
    }
    OAILOG_DEBUG (LOG_MME_APP, "MME_APP_INITIAL_UE_MESSAGE. Allocated new MME UE context and new mme_ue_s1ap_id. " MME_UE_S1AP_ID_FMT ". \n", ue_context_p->mme_ue_s1ap_id);
    /** Register the new MME_UE context into the map. */
    DevAssert (mme_insert_ue_context (&mme_app_desc.mme_ue_contexts, ue_context_p) == 0);
    /**
     * Update the coll_keys with the IMSI.
     */
    mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context_p,
        ue_context_p->enb_s1ap_id_key,
        ue_context_p->mme_ue_s1ap_id,
        imsi,      /**< New IMSI. */
        ue_context_p->mme_teid_s11,
        &ue_context_p->guti);
    /** Use the received PDN connectivity information to update the session/bearer information with the PDN connections IE. */
    for(int num_pdn = 0; num_pdn < forward_relocation_request_pP->pdn_connections.num_pdn_connections; num_pdn++){
      /** Update the PDN session information directly in the new UE_Context. */
      pdn_context_t * pdn_context = mme_app_handle_pdn_connectivity_from_s10(ue_context_p, &forward_relocation_request_pP->pdn_connections.pdn_connection[num_pdn]);
      if(pdn_context){
        OAILOG_INFO (LOG_MME_APP, "Successfully updated the PDN connections for ueId " MME_UE_S1AP_ID_FMT " for pdn %s. \n",
            ue_context_p->mme_ue_s1ap_id, forward_relocation_request_pP->pdn_connections.pdn_connection[num_pdn].apn);
        // todo: @ multi apn handover sending them all together without waiting?
        /*
         * Leave the UE context in UNREGISTERED state.
         * No subscription information at this point.
         * Not informing the NAS layer at this point. It will be done at the TAU step later on.
         * We also did not receive any NAS message until yet.
         *
         * Just store the received pending MM_CONTEXT and PDN information as pending.
         * Will check on  them @TAU, before sending S10_CONTEXT_REQUEST to source MME.
         * The pending TAU information is already stored.
         */
        OAILOG_INFO(LOG_MME_APP, "UE_CONTEXT for UE " MME_UE_S1AP_ID_FMT " does not has a default bearer %d. Continuing with CSR. \n", ue_context_p->mme_ue_s1ap_id, ue_context_p->default_bearer_id);
        if(mme_app_send_s11_create_session_req(ue_context_p, pdn_context, &target_tai) != RETURNok) {
          OAILOG_CRITICAL (LOG_MME_APP, "MME_APP_FORWARD_RELOCATION_REQUEST. Sending CSR to SAE-GW failed for UE " MME_UE_S1AP_ID_FMT ". \n", ue_context_p->mme_ue_s1ap_id);
          /*
           * Deallocate the ue context and remove from MME_APP map.
           * NAS context should not exis or be invalidated*/
          mme_remove_ue_context (&mme_app_desc.mme_ue_contexts, ue_context_p);
          /** Send back failure. */
          mme_app_send_s10_forward_relocation_response_err(forward_relocation_request_pP->s10_source_mme_teid.teid, forward_relocation_request_pP->s10_source_mme_teid.ipv4_address, forward_relocation_request_pP->trxn, RELOCATION_FAILURE);
          bdestroy(forward_relocation_request_pP->eutran_container.container_value);
          OAILOG_FUNC_OUT (LOG_MME_APP);
        }
      }else{
        OAILOG_ERROR(LOG_MME_APP, "Error updating PDN connection for ueId " MME_UE_S1AP_ID_FMT " for pdn %s. Skipping the PDN.\n",
            ue_context_p->mme_ue_s1ap_id, forward_relocation_request_pP->pdn_connections.pdn_connection[num_pdn].apn);
      }
    }
    // todo: check at least one PDN created, else send a failure message back!
    DevAssert(num_pdn);
 }else{
   OAILOG_INFO (LOG_MME_APP, "Continuing with already existing MME_APP UE_Context with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ". "
       "Sending handover request. \n", ue_context_p->mme_ue_s1ap_id);
   /*
    * A handover procedure also exists for this case.
    * todo: a release request must already be sent to the source eNB.
    * todo: eventually sending it here?
    * Not updating the PDN connections IE for the UE.
    * Assuming that it is a too fast re-handover where it is not needed.
    */
   // todo: enb_id type
   mme_app_send_s1ap_handover_request(ue_context_p->mme_ue_s1ap_id, forward_relocation_request_pP->target_identification.target_id.macro_enb_id,
       forward_relocation_request_pP->ue_eps_mm_context.ue_nc.eea,
       forward_relocation_request_pP->ue_eps_mm_context.ue_nc.eia,
       forward_relocation_request_pP->ue_eps_mm_context.nh,
       forward_relocation_request_pP->ue_eps_mm_context.ncc);
 }
 /** The timers on the target side will be initialized with the handover procedure. */
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Send an S1AP Handover Request.
 */
static
void mme_app_send_s1ap_handover_request(mme_ue_s1ap_id_t mme_ue_s1ap_id,
    uint32_t                enb_id,
    uint16_t                encryption_algorithm_capabilities,
    uint16_t                integrity_algorithm_capabilities,
    uint8_t                 nh[AUTH_NH_SIZE],
    uint8_t                 ncc){

  MessageDef                *message_p = NULL;
  ue_context_t              *ue_context_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);

  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id);
  DevAssert(ue_context_p); /**< Should always exist. Any mobility issue in which this could occur? */

  /** Get the EMM Context. */
  message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_HANDOVER_REQUEST);

  itti_s1ap_handover_request_t *handover_request_p = &message_p->ittiMsg.s1ap_handover_request;
  /** UE_ID. */
  handover_request_p->ue_id = mme_ue_s1ap_id;
  handover_request_p->macro_enb_id = enb_id;
  /** Handover Type & Cause will be set in the S1AP layer. */
  /** Set the AMBR Parameters. */
  handover_request_p->ambr.br_ul = ue_context_p->subscribed_ambr.br_ul;
  handover_request_p->ambr.br_dl = ue_context_p->subscribed_ambr.br_dl;
  /** Set all active bearers to be setup. */
  handover_request_p->bearer_ctx_to_be_setup_list.n_bearers   = ue_context_p->nb_ue_bearer_ctxs;
  handover_request_p->bearer_ctx_to_be_setup_list.bearer_ctxs = (void*)&ue_context_p->bearer_ctxs;

  hash_table_ts_t * bearer_contexts1 = (hash_table_ts_t*)handover_request_p->bearer_ctx_to_be_setup_list.bearer_ctxs;

  /** Set the Security Capabilities. */
  handover_request_p->security_capabilities_encryption_algorithms = encryption_algorithm_capabilities;
  handover_request_p->security_capabilities_integrity_algorithms  = integrity_algorithm_capabilities;
  /** Set the Security Context. */
  handover_request_p->ncc = ncc;
  memcpy(handover_request_p->nh, nh, AUTH_NH_SIZE);
  /** Set the Source-to-Target Transparent container from the pending information, which will be removed from the UE_Context. */
  handover_request_p->source_to_target_eutran_container = ue_context_p->pending_s1ap_source_to_target_handover_container;
  ue_context_p->pending_s1ap_source_to_target_handover_container = NULL;
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
    const itti_s10_forward_relocation_response_t* const forward_relocation_response_pP
    )
{
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;
  uint64_t                                imsi = 0;
  int16_t                                 bearer_id =0;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (forward_relocation_response_pP );

  ue_context_p = mme_ue_context_exists_s10_teid (&mme_app_desc.mme_ue_contexts, forward_relocation_response_pP->teid);

  if (ue_context_p == NULL) {
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
      forward_relocation_response_pP->teid, ue_context_p->imsi);
  /**
   * Check that the UE_Context is in correct (EMM_REGISTERED) state.
   */
  if(ue_context_p->mm_state != UE_REGISTERED){
    /** Deal with the error case. */
    OAILOG_ERROR(LOG_MME_APP, "UE MME context with IMSI " IMSI_64_FMT " and mmeS1apUeId " MME_UE_S1AP_ID_FMT " is not in REGISTERED state, instead %d. Doing an implicit detach. \n",
        ue_context_p->imsi, ue_context_p->mme_ue_s1ap_id, ue_context_p->mm_state);
    /** Purge the container. */
    bdestroy(forward_relocation_response_pP->eutran_container.container_value);

    /**
     * Don't send an S10 Relocation Failure to the target side. Let the target side do an implicit detach with timeout.
     * RCR will be sent only with HANDOVER_CANCELLATION.
     */
    /** Perform an implicit detach. Wrong UE state to send a preparation failure. */
    ue_context_p->ue_context_rel_cause = S1AP_IMPLICIT_CONTEXT_RELEASE;
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    itti_nas_implicit_detach_ue_ind_t *nas_implicit_detach_ue_ind_p = &message_p->ittiMsg.nas_implicit_detach_ue_ind;
    memset ((void*)nas_implicit_detach_ue_ind_p, 0, sizeof (itti_nas_implicit_detach_ue_ind_t));
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context_p->mme_ue_s1ap_id;
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  if (forward_relocation_response_pP->cause != REQUEST_ACCEPTED) {
    /** Purge the container. */
    bdestroy(forward_relocation_response_pP->eutran_container.container_value);

    /**
     * We are in EMM-REGISTERED state, so we don't need to perform an implicit detach.
     * In the target side, we won't do anything. We assumed everything is taken care of (No Relocation Cancel Request to be sent).
     * No handover timers in the source-MME side exist.
     * We will only send an Handover Preparation message and leave the UE context as it is (including the S10 tunnel endpoint at the source-MME side).
     * The handover notify timer is only at the target-MME side. That's why, its not in the method below.
     */
    mme_app_send_s1ap_handover_preparation_failure(ue_context_p->mme_ue_s1ap_id,
        ue_context_p->enb_ue_s1ap_id, ue_context_p->sctp_assoc_id_key, RELOCATION_FAILURE);
    OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
  }
  /** We are the source-MME side. Store the counterpart as target-MME side. */
  ue_context_p->remote_mme_s10_teid = forward_relocation_response_pP->s10_target_mme_teid.teid;
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
  bearer_id = forward_relocation_response_pP->list_of_bearers.bearer_contexts[0].eps_bearer_id /* - 5 */ ;
  // todo: what is the dumping doing? printing? needed for s10?

   /**
    * Not doing/storing anything in the NAS layer. Just sending handover command back.
    * Send a S1AP Handover Command to the source eNodeB.
    */
   OAILOG_INFO(LOG_MME_APP, "MME_APP UE context is in REGISTERED state. Sending a Handover Command to the source-ENB with enbId: %d for UE with mmeUeS1APId : " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT " after S10 Forward Relocation Response. \n",
       ue_context_p->e_utran_cgi.cell_identity.enb_id, ue_context_p->mme_ue_s1ap_id, ue_context_p->imsi);
   /** Send a Handover Command. */
   mme_app_send_s1ap_handover_command(ue_context_p->mme_ue_s1ap_id, ue_context_p->enb_ue_s1ap_id, ue_context_p->e_utran_cgi.cell_identity.enb_id, forward_relocation_response_pP->eutran_container.container_value);
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
    const itti_s10_forward_access_context_notification_t* const forward_access_context_notification_pP
    )
{
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;
  uint64_t                                imsi = 0;
  int16_t                                 bearer_id =0;

  OAILOG_FUNC_IN (LOG_MME_APP);
  OAILOG_DEBUG (LOG_MME_APP, "Received S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION from S10. \n");
  DevAssert (forward_access_context_notification_pP );

  /** Here it checks the local TEID. */
  ue_context_p = mme_ue_context_exists_s10_teid (&mme_app_desc.mme_ue_contexts, forward_access_context_notification_pP->teid);

  if (ue_context_p == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION local S11 teid " TEID_FMT " ", forward_access_context_notification_pP->teid);
    /** We cannot send an S10 reject, since we don't have the destination TEID. */
    /**
     * todo: lionel
     * If we ignore the request (for which a transaction exits), and a second request arrives, there is a dev_assert..
     * therefore removing the transaction?
     */
    /** Not performing an implicit detach. The handover timeout should erase the context (incl. the S10 Tunnel Endpoint, which we don't erase here in the error case). */
    OAILOG_ERROR(LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", forward_access_context_notification_pP->teid);
    bdestroy(forward_access_context_notification_pP->eutran_container.container_value);
    OAILOG_FUNC_OUT (LOG_MME_APP);

  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION local S10 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      forward_access_context_notification_pP->teid, ue_context_p->imsi);
  /** Send a S1AP MME Status Transfer Message the target eNodeB. */
  message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE);
  DevAssert (message_p != NULL);
  itti_s10_forward_access_context_acknowledge_t *s10_mme_forward_access_context_acknowledge_p =
      &message_p->ittiMsg.s10_forward_access_context_acknowledge;
  s10_mme_forward_access_context_acknowledge_p->teid        = ue_context_p->remote_mme_s10_teid;  /**< Set the target TEID. */
  s10_mme_forward_access_context_acknowledge_p->local_teid  = ue_context_p->local_mme_s10_teid;   /**< Set the local TEID. */
  s10_mme_forward_access_context_acknowledge_p->peer_ip = ue_context_p->remote_mme_s10_peer_ip; /**< Set the target TEID. */
  s10_mme_forward_access_context_acknowledge_p->trxn = forward_access_context_notification_pP->trxn; /**< Set the target TEID. */
  /** Check that there is a pending handover process. */
  if(ue_context_p->mm_state != UE_UNREGISTERED){
    /** Deal with the error case. */
    OAILOG_ERROR(LOG_MME_APP, "UE MME context with IMSI " IMSI_64_FMT " and mmeS1apUeId " MME_UE_S1AP_ID_FMT " is not in UNREGISTERED state. "
        "Sending reject back and performing an implicit detach. \n",
        ue_context_p->imsi, ue_context_p->mme_ue_s1ap_id);
    s10_mme_forward_access_context_acknowledge_p->cause =  SYSTEM_FAILURE;
    bdestroy(forward_access_context_notification_pP->eutran_container.container_value);
    itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
    /** Sending 2 ITTI messages back to back to perform an implicit detach on the target-MME side. */
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  s10_mme_forward_access_context_acknowledge_p->cause = REQUEST_ACCEPTED;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "MME_APP Sending S10 FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE.");
  itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);

  /** Send a S1AP MME Status Transfer Message the target eNodeB. */
  mme_app_send_s1ap_mme_status_transfer(ue_context_p->mme_ue_s1ap_id, ue_context_p->enb_ue_s1ap_id, ue_context_p->pending_handover_target_enb_id, forward_access_context_notification_pP->eutran_container.container_value);
  /**
   * Todo: Lionel
   * Setting the ECM state with the first message to the ENB (HANDOVER_REQUEST - no enb_ue_s1ap_id exists yet then) or with this one?
   */
  if (ue_context_p->ecm_state != ECM_CONNECTED)
  {
    OAILOG_DEBUG (LOG_MME_APP, "MME_APP:MME_STATUS_TRANSFER. Establishing S1 sig connection. mme_ue_s1ap_id = %d,enb_ue_s1ap_id = %d \n", ue_context_p->mme_ue_s1ap_id,
        ue_context_p->pending_handover_enb_ue_s1ap_id);
    mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context_p, ECM_CONNECTED);
  }

  OAILOG_INFO(LOG_MME_APP, "Sending S1AP MME Status transfer to the target eNodeB for UE "
      "with enb_ue_s1ap_id: %d, mme_ue_s1ap_id. %d. \n", ue_context_p->pending_handover_enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_forward_access_context_acknowledge(
    const itti_s10_forward_access_context_acknowledge_t* const forward_access_context_acknowledge_pP
    )
{
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;
  uint64_t                                imsi = 0;
  int16_t                                 bearer_id =0;

  OAILOG_FUNC_IN (LOG_MME_APP);
  OAILOG_DEBUG (LOG_MME_APP, "Received S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE from S10. \n");
  DevAssert (forward_access_context_acknowledge_pP );

  /** Here it checks the local TEID. */
  ue_context_p = mme_ue_context_exists_s10_teid (&mme_app_desc.mme_ue_contexts, forward_access_context_acknowledge_pP->teid);

  if (ue_context_p == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE local S11 teid " TEID_FMT " ", forward_access_context_acknowledge_pP->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", forward_access_context_acknowledge_pP->teid);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE local S11 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
      forward_access_context_acknowledge_pP->teid, ue_context_p->imsi);

  /** Check that there is a pending handover process. */
  if(ue_context_p->mm_state != UE_REGISTERED){
    /** Deal with the error case. */
    OAILOG_ERROR(LOG_MME_APP, "UE MME context with IMSI " IMSI_64_FMT " and mmeS1apUeId: " MME_UE_S1AP_ID_FMT " is not in UE_REGISTERED state, instead %d, when S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE is received. "
        "Ignoring the error and waiting remove the UE contexts triggered by HSS. \n",
        ue_context_p->imsi, ue_context_p->mme_ue_s1ap_id, ue_context_p->mm_state);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_handover_request_acknowledge(
     const itti_s1ap_handover_request_acknowledge_t * const handover_request_acknowledge_pP
    )
{
 struct ue_context_s                    *ue_context_p = NULL;
 MessageDef                             *message_p = NULL;

 OAILOG_FUNC_IN (LOG_MME_APP);
 OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_HANDOVER_REQUEST_ACKNOWLEDGE from S1AP for enbUeS1apId " ENB_UE_S1AP_ID_FMT". \n", handover_request_acknowledge_pP->enb_ue_s1ap_id);
 /** Just get the S1U-ENB-FTEID. */
 ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, handover_request_acknowledge_pP->mme_ue_s1ap_id);
 if (ue_context_p == NULL) {
   OAILOG_ERROR(LOG_MME_APP, "No MME_APP UE context for the UE with mmeUeS1APId : " MME_UE_S1AP_ID_FMT ". \n", handover_request_acknowledge_pP->mme_ue_s1ap_id);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_FAILURE. No UE existing mmeS1apUeId %d. \n", handover_request_acknowledge_pP->mme_ue_s1ap_id);
   /** Ignore the message. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_HANDOVER_REQUEST_ACKNOWLEDGE from S1AP (2). UE eNbUeS1aPId " ENB_UE_S1AP_ID_FMT ". \n", ue_context_p->enb_ue_s1ap_id);


 /** Not updating the enb_ue_s1ap_id yet, since we are missing enb_id, etc... */
//
//   enb_s1ap_id_key_t                       enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;
//   MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, handover_request_acknowledge_pP->cgi.cell_identity.enb_id, handover_notify_pP->enb_ue_s1ap_id);
//    /**
//     * Update the coll_keys with the new s1ap parameters.
//     */
//    mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context_p,
//        enb_s1ap_id_key,     /**< New key. */
//        ue_context_p->mme_ue_s1ap_id,
//        ue_context_p->imsi,
//        ue_context_p->mme_s11_teid,
//        ue_context_p->local_mme_s10_teid,
//        &ue_context_p->guti);

 /**
  * Set the downlink bearers as pending.
  * Will be forwarded to the SAE-GW after the HANDOVER_NOTIFY/S10_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGE.
  * todo: currently only a single bearer will be set.
  */
 memcpy(&ue_context_p->pending_s1u_downlink_bearer, &handover_request_acknowledge_pP->bearer_s1u_enb_fteid, sizeof(FTeid_t));
 ue_context_p->pending_s1u_downlink_bearer_ebi = handover_request_acknowledge_pP->eps_bearer_id;

 /** Check if the UE is EMM-REGISTERED or not. */
 if(ue_context_p->mm_state == UE_REGISTERED){
   /**
    * UE is registered, we assume in this case that the source-MME is also attached to the current Send a handover command to the source-ENB.
    * The SCTP-assoc to MME_UE_S1AP_ID association is still to the old one, continue using it.
    */
   OAILOG_INFO(LOG_MME_APP, "MME_APP UE context is in REGISTERED state. Sending a Handover Command to the source-ENB with enbId: %d for UE with mmeUeS1APId : " MME_UE_S1AP_ID_FMT " and enbUeS1apId " ENB_UE_S1AP_ID_FMT ". \n",
      ue_context_p->e_utran_cgi.cell_identity.enb_id, handover_request_acknowledge_pP->mme_ue_s1ap_id, ue_context_p->enb_ue_s1ap_id);
   /** Send a Handover Command to the current source eNB. */
   mme_app_send_s1ap_handover_command(handover_request_acknowledge_pP->mme_ue_s1ap_id, ue_context_p->enb_ue_s1ap_id, ue_context_p->e_utran_cgi.cell_identity.enb_id, handover_request_acknowledge_pP->target_to_source_eutran_container);
   ue_context_p->pending_handover_enb_ue_s1ap_id = handover_request_acknowledge_pP->enb_ue_s1ap_id;
   /**
    * Save the new ENB_UE_S1AP_ID
    * Don't update the coll_keys with the new enb_ue_s1ap_id.
    */
   /**
    * As the specification said, we will leave the UE_CONTEXT as it is. Not checking further parameters.
    * No timers are started. Only timers are in the source-ENB.
    * ********************************************
    * The ECM state will be left as ECM-IDLE (the one from the created ue_reference).
    * With the first S1AP message to the eNB (MME_STATUS) or with the received HANDOVER_NOTIFY, we will set to ECM_CONNECTED.
    */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }else{
   ue_context_p->enb_ue_s1ap_id = handover_request_acknowledge_pP->enb_ue_s1ap_id;

 }

 /**
  * UE is in UE_UNREGISTERED state. Assuming inter-MME S1AP Handover was triggered.
  * Sending FW_RELOCATION_RESPONSE.
  */
 message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_RELOCATION_RESPONSE);
 DevAssert (message_p != NULL);
 itti_s10_forward_relocation_response_t *forward_relocation_response_p = &message_p->ittiMsg.s10_forward_relocation_response;
 memset ((void*)forward_relocation_response_p, 0, sizeof (itti_s10_forward_relocation_response_t));
 /** Set the target S10 TEID. */
 forward_relocation_response_p->teid    = ue_context_p->remote_mme_s10_teid; /**< Only a single target-MME TEID can exist at a time. */
 /**
  * todo: Get the MME from the origin TAI.
  * Currently only one MME is supported.
  */
 forward_relocation_response_p->peer_ip = ue_context_p->remote_mme_s10_peer_ip; /**< todo: Check this is correct. */
 /**
  * Trxn is the only object that has the last seqNum, but we can only search the TRXN in the RB-Tree with the seqNum.
  * We need to store the last seqNum locally.
  * todo: Lionel, any better ideas on this one?
  */
 forward_relocation_response_p->trxn    = ue_context_p->pending_s10_response_trxn;
 /** Set the cause. */
 forward_relocation_response_p->cause = REQUEST_ACCEPTED;
 // todo: no indicator set on the number of modified bearer contexts
 /** Not updating anything in the EMM/ESM layer. No new timers needed. */
 forward_relocation_response_p->list_of_bearers.bearer_contexts[0].eps_bearer_id = ue_context_p->pending_s1u_downlink_bearer_ebi;
 forward_relocation_response_p->list_of_bearers.num_bearer_context = 1;

 teid_t local_teid = 0x0;
 do{
   local_teid = (teid_t)(rand() % 0xFFFFFFFF);
 }while(mme_ue_context_exists_s10_teid(&mme_app_desc.mme_ue_contexts, local_teid) != NULL);

 /** Set the Source MME_S10_FTEID the same as in S11. */
 OAI_GCC_DIAG_OFF(pointer-to-int-cast);
 forward_relocation_response_p->s10_target_mme_teid.teid = local_teid; /**< This one also sets the context pointer. */
 OAI_GCC_DIAG_ON(pointer-to-int-cast);
 forward_relocation_response_p->s10_target_mme_teid.interface_type = S10_MME_GTP_C;
 mme_config_read_lock (&mme_config);
 forward_relocation_response_p->s10_target_mme_teid.ipv4_address = mme_config.ipv4.s10;
 mme_config_unlock (&mme_config);
 forward_relocation_response_p->s10_target_mme_teid.ipv4 = 1;

 /**
  * Update the local_s10_key.
  * Leave the enb_ue_s1ap_id_key as it is. enb_ue_s1ap_id_key will be updated with HANDOVER_NOTIFY and then the registration will be updated.
  * Not setting the key directly in the  ue_context structure. Only over this function!
  */
 mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context_p,
     ue_context_p->enb_s1ap_id_key,   /**< Not updated. */
     ue_context_p->mme_ue_s1ap_id,
     ue_context_p->imsi,
     ue_context_p->mme_s11_teid,       // mme_s11_teid is new
     forward_relocation_response_p->s10_target_mme_teid.teid,       // set with forward_relocation_response!
     &ue_context_p->guti);            /**< No guti exists
 /** Set S10 F-Cause. */
 forward_relocation_response_p->f_cause.fcause_type      = FCAUSE_S1AP;
 forward_relocation_response_p->f_cause.fcause_s1ap_type = FCAUSE_S1AP_RNL;
 forward_relocation_response_p->f_cause.fcause_value     = 0; // todo: set these values later.. currently just RNL

 /** Set the E-UTRAN container. */
 forward_relocation_response_p->eutran_container.container_type = 3;
 /** Just link the bstring. Will be purged in the S10 message. */
 forward_relocation_response_p->eutran_container.container_value = handover_request_acknowledge_pP->target_to_source_eutran_container;

 MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "MME_APP Sending S10 FORWARD_RELOCATION_RESPONSE");

 /**
  * Sending a message to S10.
  * No changes in the contexts, flags, timers, etc.. needed.
  */
 itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_handover_failure (
     const itti_s1ap_handover_failure_t * const handover_failure_pP
    )
{
 struct ue_context_s                    *ue_context_p = NULL;
 MessageDef                             *message_p = NULL;
 uint64_t                                imsi = 0;

 OAILOG_FUNC_IN (LOG_MME_APP);

 OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_HANDOVER_FAILURE from target eNB for UE_ID " MME_UE_S1AP_ID_FMT ". \n", handover_failure_pP->mme_ue_s1ap_id);

 /** Check that the UE does exist (in both S1AP cases). */
 ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, handover_failure_pP->mme_ue_s1ap_id);
 if (ue_context_p == NULL) {
   OAILOG_ERROR(LOG_MME_APP, "An UE MME context does not exist for UE with mmeS1apUeId %d. \n", handover_failure_pP->mme_ue_s1ap_id);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_FAILURE. No UE existing mmeS1apUeId %d. \n", handover_failure_pP->mme_ue_s1ap_id);
   /** Ignore the message. */
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** Check if the UE is EMM-REGISTERED or not. */
 if(ue_context_p->mm_state == UE_REGISTERED){
   /**
    * UE is registered, we assume in this case that the source-MME is also attached to the current.
    * In this case, we need to re-notify the MME_UE_S1AP_ID<->SCTP association, because it might be removed with the error handling.
    */
   notify_s1ap_new_ue_mme_s1ap_id_association (ue_context_p->sctp_assoc_id_key, ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id);
   /** We assume a valid enb & sctp id in the UE_Context. */
   mme_app_send_s1ap_handover_preparation_failure(ue_context_p->mme_ue_s1ap_id, ue_context_p->enb_ue_s1ap_id, ue_context_p->sctp_assoc_id_key, S1AP_HANDOVER_FAILED);
   /**
    * As the specification said, we will leave the UE_CONTEXT as it is. Not checking further parameters.
    * No timers are started. Only timers are in the source-ENB.
    */
   /** In case a system error has occurred, purge the E-UTRAN container. */
   bdestroy(ue_context_p->pending_s1ap_source_to_target_handover_container);

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
 forward_relocation_response_p->teid    = ue_context_p->remote_mme_s10_teid; /**< Only a single target-MME TEID can exist at a time. */
 /** Get the MME from the origin TAI. */
 forward_relocation_response_p->peer_ip = ue_context_p->remote_mme_s10_peer_ip; /**< todo: Check this is correct. */
 /**
  * Trxn is the only object that has the last seqNum, but we can only search the TRXN in the RB-Tree with the seqNum.
  * We need to store the last seqNum locally.
  */
 forward_relocation_response_p->trxn    = ue_context_p->pending_s10_response_trxn;
 /** Set the cause. */
 forward_relocation_response_p->cause = RELOCATION_FAILURE;

 /** Perform an implicit detach. */
 ue_context_p->ue_context_rel_cause = S1AP_IMPLICIT_CONTEXT_RELEASE;
 message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
 DevAssert (message_p != NULL);
 itti_nas_implicit_detach_ue_ind_t *nas_implicit_detach_ue_ind_p = &message_p->ittiMsg.nas_implicit_detach_ue_ind;
 memset ((void*)nas_implicit_detach_ue_ind_p, 0, sizeof (itti_nas_implicit_detach_ue_ind_t));
 message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context_p->mme_ue_s1ap_id;
 itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);

 /** No timers, etc. is needed. */
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_enb_status_transfer(
     const itti_s1ap_status_transfer_t * const s1ap_status_transfer_pP
    )
{
 struct ue_context_s                    *ue_context_p = NULL;
 MessageDef                             *message_p = NULL;

 OAILOG_FUNC_IN (LOG_MME_APP);
 OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_ENB_STATUS_TRANSFER from S1AP. \n");

 /** Check that the UE does exist. */
 ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, s1ap_status_transfer_pP->mme_ue_s1ap_id);
 if (ue_context_p == NULL) {
   OAILOG_ERROR(LOG_MME_APP, "An UE MME context does not exist for UE with mmeS1apUeId %d. \n", s1ap_status_transfer_pP->mme_ue_s1ap_id);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_ENB_STATUS_TRANSFER. No UE existing mmeS1apUeId %d. \n", s1ap_status_transfer_pP->mme_ue_s1ap_id);
   /**
    * We don't really expect an error at this point. Just ignore the message.
    */
   bdestroy(s1ap_status_transfer_pP->bearerStatusTransferList_buffer);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /** Check if the destination eNodeB is attached at the same or another MME. */
 if (mme_app_check_ta_local(&ue_context_p->pending_handover_target_tai.plmn, ue_context_p->pending_handover_target_tai.tac)) {
   /** Check if the eNB with the given eNB-ID is served. */
   if(s1ap_is_enb_id_in_list(ue_context_p->pending_handover_target_enb_id) != NULL){
     OAILOG_DEBUG (LOG_MME_APP, "Target ENB_ID %d of target TAI " TAI_FMT " is served by current MME. \n", ue_context_p->pending_handover_target_enb_id, ue_context_p->pending_handover_target_tai);
     /**
      * Set the ENB of the pending target-eNB.
      * Even if the HANDOVER_NOTIFY messaged is received simultaneously, the pending enb_ue_s1ap_id field should stay.
      * We do not check that the target-eNB exists. We did not modify any contexts.
      */


     /**
      * Concatenate with header (todo: OAI: how to do this properly?)
      */
     char enbStatusPrefix[] = {0x00, 0x00, 0x00, 0x59, 0x40, 0x0b};
     bstring enbStatusPrefixBstr = blk2bstr (enbStatusPrefix, 6);
     bconcat(enbStatusPrefixBstr, s1ap_status_transfer_pP->bearerStatusTransferList_buffer);

     /** Destroy the container. */
     bdestroy(s1ap_status_transfer_pP->bearerStatusTransferList_buffer);

     enb_ue_s1ap_id_t target_enb_ue_s1ap_id = ue_context_p->enb_ue_s1ap_id;
     if(ue_context_p->enb_ue_s1ap_id == s1ap_status_transfer_pP->enb_ue_s1ap_id){
       target_enb_ue_s1ap_id = ue_context_p->pending_handover_enb_ue_s1ap_id;
     }

     mme_app_send_s1ap_mme_status_transfer(ue_context_p->mme_ue_s1ap_id, target_enb_ue_s1ap_id, ue_context_p->pending_handover_target_enb_id /*e_utran_cgi.cell_identity.enb_id*/, enbStatusPrefixBstr);
     ue_context_p->pending_handover_enb_ue_s1ap_id = 0;
     OAILOG_FUNC_OUT (LOG_MME_APP);
   }else{
     /** The target eNB-ID is not served by this MME. */
     OAILOG_DEBUG (LOG_MME_APP, "Target ENB_ID %d of target TAI " TAI_FMT " is NOT served by current MME. \n", ue_context_p->pending_handover_target_enb_id, ue_context_p->pending_handover_target_tai);
     bdestroy(s1ap_status_transfer_pP->bearerStatusTransferList_buffer);
     OAILOG_FUNC_OUT (LOG_MME_APP);
   }
 }

 int ngh_index = mme_app_check_target_tai_neighboring_mme(&ue_context_p->pending_handover_target_tai);
 if(ngh_index == -1){
   OAILOG_ERROR(LOG_MME_APP, "Could not find a neighboring MME to send FORWARD_ACCESS_CONTEXT_NOTIFICATION for UE " MME_UE_S1AP_ID_FMT". \n", ue_context_p->mme_ue_s1ap_id);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }


 OAILOG_DEBUG (LOG_MME_APP, "Target ENB_ID %d of target TAI " TAI_FMT " is served by neighboring MME. \n", ue_context_p->pending_handover_target_enb_id, ue_context_p->pending_handover_target_tai);
 /* UE is DEREGISTERED. Assuming that it came from S10 inter-MME handover. Forwarding the eNB status information to the target-MME via Forward Access Context Notification. */
 message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION);
 DevAssert (message_p != NULL);
 itti_s10_forward_access_context_notification_t *forward_access_context_notification_p = &message_p->ittiMsg.s10_forward_access_context_notification;
 memset ((void*)forward_access_context_notification_p, 0, sizeof (itti_s10_forward_access_context_notification_t));
 /** Set the target S10 TEID. */
 forward_access_context_notification_p->teid       = ue_context_p->remote_mme_s10_teid; /**< Only a single target-MME TEID can exist at a time. */
 forward_access_context_notification_p->local_teid = ue_context_p->local_mme_s10_teid; /**< Only a single target-MME TEID can exist at a time. */
 forward_access_context_notification_p->peer_ip    = mme_config.nghMme.nghMme[ngh_index].ipAddr;
 /** Set the E-UTRAN container. */
 forward_access_context_notification_p->eutran_container.container_type = 3;
 forward_access_context_notification_p->eutran_container.container_value = s1ap_status_transfer_pP->bearerStatusTransferList_buffer;
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

//------------------------------------------------------------------------------
void
mme_app_handle_s1ap_handover_notify(
     const itti_s1ap_handover_notify_t * const handover_notify_pP
    )
{
 struct ue_context_s                    *ue_context_p = NULL;
 MessageDef                             *message_p = NULL;
 enb_s1ap_id_key_t                       enb_s1ap_id_key = INVALID_ENB_UE_S1AP_ID_KEY;

 OAILOG_FUNC_IN (LOG_MME_APP);
 OAILOG_DEBUG (LOG_MME_APP, "Received S1AP_HANDOVER_NOTIFY from S1AP SCTP ASSOC_ID %d. \n", handover_notify_pP->assoc_id);

 /** Check that the UE does exist. */
 ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, handover_notify_pP->mme_ue_s1ap_id);
 if (ue_context_p == NULL) {
   OAILOG_ERROR(LOG_MME_APP, "An UE MME context does not exist for UE with mmeS1apUeId %d. \n", handover_notify_pP->mme_ue_s1ap_id);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S1AP_HANDOVER_NOTIFY. No UE existing mmeS1apUeId %d. \n", handover_notify_pP->mme_ue_s1ap_id);
   // todo: appropriate error handling for this case.. removing the UE? continuing with the handover or aborting?
   // todo: removing the S1ap context directly or via NOTIFY?
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 /**
  * No need to signal the NAS layer the completion of the handover.
  * The ECGI & TAI will also be sent with TAU if its UPLINK_NAS_TRANSPORT.
  * Here we just update the MME_APP UE_CONTEXT parameters.
  */

 /**
  * Set the values to the old source enb as pending (enb_id, enb_ue_s1ap_id).
  * Also get the old UE_REFERENCE.
  */
 ue_description_t * old_ue_reference = s1ap_is_enb_ue_s1ap_id_in_list_per_enb(ue_context_p->enb_ue_s1ap_id, ue_context_p->e_utran_cgi.cell_identity.enb_id);
 if(ue_context_p->mm_state == UE_REGISTERED && (ue_context_p->e_utran_cgi.cell_identity.enb_id != 0)){
   ue_context_p->pending_handover_enb_ue_s1ap_id = ue_context_p->enb_ue_s1ap_id;
 }
 /**
  * When Handover Notify is received, we update the eNB associations (SCTP, enb_ue_s1ap_id, enb_id,..). The main eNB is the new ENB now.
  * ToDo: If this has an error with 2 eNBs, we need to remove the first eNB first (and send the UE Context Release Command only with MME_UE_S1AP_ID).
  * Update the coll-keys.
  */
 ue_context_p->sctp_assoc_id_key = handover_notify_pP->assoc_id;
 ue_context_p->e_utran_cgi = handover_notify_pP->cgi;
 /** Update the enbUeS1apId. */
 ue_context_p->enb_ue_s1ap_id = handover_notify_pP->enb_ue_s1ap_id;
 // regenerate the enb_s1ap_id_key as enb_ue_s1ap_id is changed.
 MME_APP_ENB_S1AP_ID_KEY(enb_s1ap_id_key, handover_notify_pP->cgi.cell_identity.enb_id, handover_notify_pP->enb_ue_s1ap_id);

 /**
  * Update the coll_keys with the new s1ap parameters.
  */
 mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context_p,
     enb_s1ap_id_key,     /**< New key. */
     ue_context_p->mme_ue_s1ap_id,
     ue_context_p->imsi,
     ue_context_p->mme_s11_teid,
     ue_context_p->local_mme_s10_teid,
     &ue_context_p->guti);

 OAILOG_INFO(LOG_MME_APP, "Setting UE with mmeS1apUeId " MME_UE_S1AP_ID_FMT " to enbUeS1apId " ENB_UE_S1AP_ID_FMT" @handover_notify. \n", ue_context_p->mme_ue_s1ap_id, ue_context_p->enb_ue_s1ap_id);

 /**
  * This will overwrite the association towards the old eNB if single MME S1AP handover.
  * The old eNB will be referenced by the enb_ue_s1ap_id.
  */
 notify_s1ap_new_ue_mme_s1ap_id_association (ue_context_p->sctp_assoc_id_key, ue_context_p->enb_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id);

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
  * If we are in UE_UNREGISTERED state, we assume an inter-MME handover. Again update all enb related information and send an S10_FORWARD_RELOCATION_COMPLETE_NOTIFICATION
  * towards the source MME. No timer will be started on the target-MME side.
  */
 if(ue_context_p->mm_state == UE_REGISTERED){
   OAILOG_DEBUG(LOG_MME_APP, "UE MME context with imsi " IMSI_64_FMT " and mmeS1apUeId " MME_UE_S1AP_ID_FMT " has successfully completed intra-MME handover process after HANDOVER_NOTIFY. \n",
       ue_context_p->imsi, handover_notify_pP->mme_ue_s1ap_id);
   /**
    * Send the MBR to the SAE-GW.
    * Handle the MBResp normally.
    * S1AP F-TEID's received beforehand in HO_REQ_ACK.
    */
   message_p = itti_alloc_new_message (TASK_MME_APP, S11_MODIFY_BEARER_REQUEST);
   AssertFatal (message_p , "itti_alloc_new_message Failed");
   itti_s11_modify_bearer_request_t *s11_modify_bearer_request = &message_p->ittiMsg.s11_modify_bearer_request;
   memset ((void *)s11_modify_bearer_request, 0, sizeof (*s11_modify_bearer_request));
   s11_modify_bearer_request->peer_ip    = mme_config.ipv4.sgw_s11;
   s11_modify_bearer_request->teid       = ue_context_p->sgw_s11_teid;
   s11_modify_bearer_request->local_teid = ue_context_p->mme_s11_teid;
   s11_modify_bearer_request->trxn       = NULL;

   /** Delay Value in integer multiples of 50 millisecs, or zero. */
   s11_modify_bearer_request->delay_dl_packet_notif_req = 0;  // TO DO
   s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].eps_bearer_id = ue_context_p->pending_s1u_downlink_bearer_ebi;
   memcpy (&s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].s1_eNB_fteid,
       &ue_context_p->pending_s1u_downlink_bearer, sizeof (ue_context_p->pending_s1u_downlink_bearer));
   s11_modify_bearer_request->bearer_contexts_to_be_modified.num_bearer_context = 1;

   s11_modify_bearer_request->bearer_contexts_to_be_removed.num_bearer_context = 0;

   s11_modify_bearer_request->mme_fq_csid.node_id_type = GLOBAL_UNICAST_IPv4; // TO DO
   s11_modify_bearer_request->mme_fq_csid.csid = 0;   // TO DO ...
   memset(&s11_modify_bearer_request->indication_flags, 0, sizeof(s11_modify_bearer_request->indication_flags));   // TO DO
   s11_modify_bearer_request->rat_type = RAT_EUTRAN;
   /*
    * S11 stack specific parameter. Not used in standalone epc mode
    */
   MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME,  MSC_S11_MME ,
       NULL, 0, "0 S11_MODIFY_BEARER_REQUEST teid %u ebi %u", s11_modify_bearer_request->teid,
       s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].eps_bearer_id);
   itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
   /**
    * Start timer to wait the handover/TAU procedure to complete.
    * This will only be started in the source MME, not in the target MME.
    */
   /**
    * Get the old UE_REFERENCE and provide it as an argument.
    */
   if(old_ue_reference){
     if (timer_setup (mme_config.mme_mobility_completion_timer, 0,
                   TASK_S1AP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *)old_ue_reference, &(old_ue_reference->s1ap_handover_completion_timer.id)) < 0) {
       OAILOG_ERROR (LOG_MME_APP, "Failed to start >s1ap_handover_completion for enbUeS1apId " ENB_UE_S1AP_ID_FMT " for duration %d \n", old_ue_reference->enb_ue_s1ap_id, mme_config.mme_mobility_completion_timer);
       old_ue_reference->s1ap_handover_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
       /** Not set the timer for the UE context, since we will not deregister. */
     } else {
       OAILOG_DEBUG (LOG_MME_APP, "MME APP : Completed Handover Procedure at (source) MME side after handling S1AP_HANDOVER_NOTIFY. "
           "Activated the S1AP Handover completion timer enbUeS1apId " ENB_UE_S1AP_ID_FMT ". Removing source eNB resources after timer.. Timer Id %u. Timer duration %d \n",
           old_ue_reference->enb_ue_s1ap_id, old_ue_reference->s1ap_handover_completion_timer.id, mme_config.mme_mobility_completion_timer);
       /** Not setting the timer for MME_APP UE context. */
     }
   }else{
     OAILOG_DEBUG(LOG_MME_APP, "No old UE_REFERENCE was found for mmeS1apUeId " MME_UE_S1AP_ID_FMT " and enbUeS1apId "ENB_UE_S1AP_ID_FMT ". Not starting a new timer. \n",
         ue_context_p->enb_ue_s1ap_id, ue_context_p->e_utran_cgi.cell_identity.enb_id);
   }
 }else{
   /**
    * UE came from S10 inter-MME handover. Not clear the pending_handover state yet.
    * Sending Forward Relocation Complete Notification and waiting for acknowledgment.
    */
   message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_RELOCATION_COMPLETE_NOTIFICATION);
   DevAssert (message_p != NULL);
   itti_s10_forward_relocation_complete_notification_t *forward_relocation_complete_notification_p = &message_p->ittiMsg.s10_forward_relocation_complete_notification;
   memset ((void*)forward_relocation_complete_notification_p, 0, sizeof (itti_s10_forward_relocation_complete_notification_t));
   /** Set the destination TEID. */
   forward_relocation_complete_notification_p->teid = ue_context_p->remote_mme_s10_teid;       /**< Target S10-MME TEID. todo: what if multiple? */
   /** Set the local TEID. */
   forward_relocation_complete_notification_p->local_teid = ue_context_p->local_mme_s10_teid;        /**< Local S10-MME TEID. */
   forward_relocation_complete_notification_p->peer_ip = ue_context_p->remote_mme_s10_peer_ip; /**< Set the target TEID. */
   OAILOG_INFO(LOG_MME_APP, "Sending FW_RELOC_COMPLETE_NOTIF TO %X with remote S10-TEID " TEID_FMT ". \n.", forward_relocation_complete_notification_p->peer_ip, forward_relocation_complete_notification_p->teid);

   // todo: remove this and set at correct position!
   mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context_p, ECM_CONNECTED);

   /**
    * Sending a message to S10. Not changing any context information!
    * This message actually implies that the handover is finished. Resetting the flags and statuses here of after Forward Relocation Complete AcknowledgE?! (MBR)
    */
   itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
 }
 OAILOG_DEBUG(LOG_MME_APP, "UE MME context with imsi " IMSI_64_FMT " and mmeUeS1apId " MME_UE_S1AP_ID_FMT " has successfully handled HANDOVER_NOTIFY. \n",
     ue_context_p->imsi, handover_notify_pP->mme_ue_s1ap_id);
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_forward_relocation_complete_notification(
     const itti_s10_forward_relocation_complete_notification_t* const forward_relocation_complete_notification_pP
    )
{
 struct ue_context_s                    *ue_context_p = NULL;
 MessageDef                             *message_p = NULL;

 OAILOG_FUNC_IN (LOG_MME_APP);
 OAILOG_DEBUG (LOG_MME_APP, "Received S10_FORWARD_RELOCATION_COMPLETE_NOTIFICATION from S10. \n");

 /** Check that the UE does exist. */
 ue_context_p = mme_ue_context_exists_s10_teid (&mme_app_desc.mme_ue_contexts, forward_relocation_complete_notification_pP->teid); /**< Get the UE context from the local TEID. */
 if (ue_context_p == NULL) {
   MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 FORWARD_RELOCATION_COMPLETE_NOTIFICATION local S10 teid " TEID_FMT,
       forward_relocation_complete_notification_pP->teid);
   OAILOG_ERROR (LOG_MME_APP, "We didn't find this teid in list of UE: %08x\n", forward_relocation_complete_notification_pP->teid);
   OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
 }
 MSC_LOG_RX_MESSAGE (MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0, "0 FORWARD_RELOCATION_COMPLETE_NOTIFICATION local S10 teid " TEID_FMT " IMSI " IMSI_64_FMT " ",
     forward_relocation_complete_notification_pP->teid, ue_context_p->imsi);

 /** Check that there is a pending handover process. */
 if(ue_context_p->mm_state != UE_REGISTERED){
   /** Deal with the error case. */
   OAILOG_ERROR(LOG_MME_APP, "UE MME context with IMSI " IMSI_64_FMT " and mmeS1apUeId: " MME_UE_S1AP_ID_FMT " is not in UE_REGISTERED state, instead %d, when S10_FORWARD_RELOCATION_COMPLETE_NOTIFICATION is received. "
       "Ignoring the error, responding with ACK and still starting the timer to remove the UE context triggered by HSS. \n",
       ue_context_p->imsi, ue_context_p->mme_ue_s1ap_id, ue_context_p->mm_state);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }
 int ngh_index = mme_app_check_target_tai_neighboring_mme(&ue_context_p->pending_handover_target_tai);
 if(ngh_index == -1){
   OAILOG_ERROR(LOG_MME_APP, "Could not find a neighboring MME to send FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGE for UE " MME_UE_S1AP_ID_FMT". \n", ue_context_p->mme_ue_s1ap_id);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }

 /** Send S10 Forward Relocation Complete Notification. */
 /**< Will stop all existing NAS timers.. todo: Any other timers than NAS timers? What about the UE transactions? */
 message_p = itti_alloc_new_message (TASK_MME_APP, S10_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGE);
 DevAssert (message_p != NULL);
 itti_s10_forward_relocation_complete_acknowledge_t *forward_relocation_complete_acknowledge_p = &message_p->ittiMsg.s10_forward_relocation_complete_acknowledge;
 memset ((void*)forward_relocation_complete_acknowledge_p, 0, sizeof (itti_s10_forward_relocation_complete_acknowledge_t));
 /** Set the destination TEID. */
 forward_relocation_complete_acknowledge_p->teid       = ue_context_p->remote_mme_s10_teid;      /**< Target S10-MME TEID. */
 /** Set the local TEID. */
 forward_relocation_complete_acknowledge_p->local_teid = ue_context_p->local_mme_s10_teid;      /**< Local S10-MME TEID. */
 /** Set the cause. */
 forward_relocation_complete_acknowledge_p->cause      = REQUEST_ACCEPTED;                       /**< Check the cause.. */
 /** Set the peer IP. */
 forward_relocation_complete_acknowledge_p->peer_ip = mme_config.nghMme.nghMme[ngh_index].ipAddr; /**< Set the target TEID. */
 /** Set the transaction. */
 forward_relocation_complete_acknowledge_p->trxn = forward_relocation_complete_notification_pP->trxn; /**< Set the target TEID. */
 itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);
 /** ECM is in connected state.. UE will be detached implicitly. */
 ue_context_p->ue_context_rel_cause = S1AP_SUCCESSFUL_HANDOVER; /**< How mapped to correct radio-Network cause ?! */

 /**
  * Start timer to wait the handover/TAU procedure to complete.
  * A Clear_Location_Request message received from the HSS will cause the resources to be removed.
  * If it was not a handover but a context request/response (TAU), the MME_MOBILITY_COMPLETION timer will be started here, else @ FW-RELOC-COMPLETE @ Handover.
  * Resources will not be removed if that is not received (todo: may it not be received or must it always come
  * --> TS.23.401 defines for SGSN "remove after CLReq" explicitly).
  */
 ue_description_t * old_ue_reference = s1ap_is_enb_ue_s1ap_id_in_list_per_enb(ue_context_p->enb_ue_s1ap_id, ue_context_p->e_utran_cgi.cell_identity.enb_id);
 if(old_ue_reference){
   if (timer_setup (mme_config.mme_mobility_completion_timer, 0,
       TASK_S1AP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *)old_ue_reference, &(old_ue_reference->s1ap_handover_completion_timer.id)) < 0) {
     OAILOG_ERROR (LOG_MME_APP, "Failed to start >s1ap_handover_completion for enbUeS1apId " ENB_UE_S1AP_ID_FMT " for duration %d \n", old_ue_reference->enb_ue_s1ap_id, mme_config.mme_mobility_completion_timer);
     old_ue_reference->s1ap_handover_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
     ue_context_p->mme_mobility_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
   } else {
     OAILOG_DEBUG (LOG_MME_APP, "MME APP : Completed Handover Procedure at (source) MME side after handling S1AP_HANDOVER_NOTIFY. "
         "Activated the S1AP Handover completion timer enbUeS1apId " ENB_UE_S1AP_ID_FMT ". Removing source eNB resources after timer.. Timer Id %u. Timer duration %d \n",
         old_ue_reference->enb_ue_s1ap_id, old_ue_reference->s1ap_handover_completion_timer.id, mme_config.mme_mobility_completion_timer);
     /** For the case of the S10 handover, add the timer ID to the MME_APP UE context to remove the UE context. */
     ue_context_p->mme_mobility_completion_timer.id = old_ue_reference->s1ap_handover_completion_timer.id;
   }
 }else{
   OAILOG_DEBUG(LOG_MME_APP, "No old UE_REFERENCE was found for mmeS1apUeId " MME_UE_S1AP_ID_FMT " and enbUeS1apId "ENB_UE_S1AP_ID_FMT ". Not starting a new timer. \n",
       ue_context_p->enb_ue_s1ap_id, ue_context_p->e_utran_cgi.cell_identity.enb_id);
 }
 OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_forward_relocation_complete_acknowledge(
     const itti_s10_forward_relocation_complete_acknowledge_t * const forward_relocation_complete_acknowledgement_pP
    )
{
 struct ue_context_s                    *ue_context_p = NULL;
 MessageDef                             *message_p = NULL;

 OAILOG_FUNC_IN (LOG_MME_APP);
 OAILOG_DEBUG (LOG_MME_APP, "Received S10_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGEMENT from S10. \n");

 /** Check that the UE does exist. */
 ue_context_p = mme_ue_context_exists_s10_teid(&mme_app_desc.mme_ue_contexts, forward_relocation_complete_acknowledgement_pP->teid);
 if (ue_context_p == NULL) {
   OAILOG_ERROR(LOG_MME_APP, "An UE MME context does not exist for UE with s10 teid %d. \n", forward_relocation_complete_acknowledgement_pP->teid);
   MSC_LOG_EVENT (MSC_MMEAPP_MME, "S10_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGEMENT. No UE existing teid %d. \n", forward_relocation_complete_acknowledgement_pP->teid);
   OAILOG_FUNC_OUT (LOG_MME_APP);
 }

 /** Stop the MME S10 Handover Completion timer. */
 if (ue_context_p->mme_s10_handover_completion_timer.id != MME_APP_TIMER_INACTIVE_ID) {
   if (timer_remove(ue_context_p->mme_s10_handover_completion_timer.id)) {
     OAILOG_ERROR (LOG_MME_APP, "Failed to stop MME S10 Handover Completion timer for UE id  %d \n", ue_context_p->mme_ue_s1ap_id);
   }
   ue_context_p->mme_s10_handover_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
 }

 /**
  * S1AP inter-MME handover is complete now. Modify the bearers with the new downlink tunnel IDs of the MME.
  */
 message_p = itti_alloc_new_message (TASK_MME_APP, S11_MODIFY_BEARER_REQUEST);
 AssertFatal (message_p , "itti_alloc_new_message Failed");
 itti_s11_modify_bearer_request_t *s11_modify_bearer_request = &message_p->ittiMsg.s11_modify_bearer_request;
 memset ((void *)s11_modify_bearer_request, 0, sizeof (*s11_modify_bearer_request));
 s11_modify_bearer_request->peer_ip    = mme_config.ipv4.sgw_s11;
 s11_modify_bearer_request->teid       = ue_context_p->sgw_s11_teid;
 s11_modify_bearer_request->local_teid = ue_context_p->mme_s11_teid;
 s11_modify_bearer_request->trxn       = NULL;

 /** Delay Value in integer multiples of 50 millisecs, or zero. */
 s11_modify_bearer_request->delay_dl_packet_notif_req = 0;  // TO DO
 s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].eps_bearer_id = ue_context_p->pending_s1u_downlink_bearer_ebi;
 memcpy (&s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].s1_eNB_fteid,
     &ue_context_p->pending_s1u_downlink_bearer, sizeof (ue_context_p->pending_s1u_downlink_bearer));
 s11_modify_bearer_request->bearer_contexts_to_be_modified.num_bearer_context = 1;

 s11_modify_bearer_request->bearer_contexts_to_be_removed.num_bearer_context = 0;


 s11_modify_bearer_request->mme_fq_csid.node_id_type = GLOBAL_UNICAST_IPv4; // TO DO
 s11_modify_bearer_request->mme_fq_csid.csid = 0;   // TO DO ...
 memset(&s11_modify_bearer_request->indication_flags, 0, sizeof(s11_modify_bearer_request->indication_flags));   // TO DO
 s11_modify_bearer_request->rat_type = RAT_EUTRAN;

 /**
  * S11 stack specific parameter. Not used in standalone epc mode
  * todo: what is this? MBR trxn not set? should also be set to NULL for S10 messages?! */
 s11_modify_bearer_request->trxn = NULL;
 MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME,  MSC_S11_MME ,
     NULL, 0, "0 S11_MODIFY_BEARER_REQUEST after FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGE teid %u ebi %u", s11_modify_bearer_request->teid,
     s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[0].eps_bearer_id);
 itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
//  DevAssert(mme_app_complete_inter_mme_handover(ue_context_p, REQUEST_ACCEPTED) == RETURNok); /**< todo: later make this un-crash.*/

 /** S1AP inter-MME handover is complete. */
 OAILOG_INFO(LOG_MME_APP, "UE_Context with IMSI " IMSI_64_FMT " and mmeUeS1apId: " MME_UE_S1AP_ID_FMT " successfully completed handover procedure! \n",
     ue_context_p->imsi, ue_context_p->mme_ue_s1ap_id);
 OAILOG_FUNC_OUT (LOG_MME_APP);
}


//------------------------------------------------------------------------------
void
mme_app_handle_mobile_reachability_timer_expiry (struct ue_context_s *ue_context_p)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context_p != NULL);
  ue_context_p->mobile_reachability_timer.id = MME_APP_TIMER_INACTIVE_ID;
  OAILOG_INFO (LOG_MME_APP, "Expired- Mobile Reachability Timer for UE id  %d \n", ue_context_p->mme_ue_s1ap_id);
  // Start Implicit Detach timer
  if (timer_setup (ue_context_p->implicit_detach_timer.sec, 0,
                TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void *)&(ue_context_p->mme_ue_s1ap_id), &(ue_context_p->implicit_detach_timer.id)) < 0) {
    OAILOG_ERROR (LOG_MME_APP, "Failed to start Implicit Detach timer for UE id  %d \n", ue_context_p->mme_ue_s1ap_id);
    ue_context_p->implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;
  } else {
    OAILOG_DEBUG (LOG_MME_APP, "Started Implicit Detach timer for UE id  %d \n", ue_context_p->mme_ue_s1ap_id);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_implicit_detach_timer_expiry (struct ue_context_s *ue_context_p)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context_p != NULL);
  MessageDef                             *message_p = NULL;
  OAILOG_INFO (LOG_MME_APP, "Expired- Implicit Detach timer for UE id  %d \n", ue_context_p->mme_ue_s1ap_id);
  ue_context_p->implicit_detach_timer.id = MME_APP_TIMER_INACTIVE_ID;

  // Initiate Implicit Detach for the UE
  message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
  DevAssert (message_p != NULL);
  message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context_p->mme_ue_s1ap_id;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
  itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_mme_s10_handover_completion_timer_expiry (struct ue_context_s *ue_context_p)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context_p != NULL);
  MessageDef                             *message_p = NULL;
  OAILOG_INFO (LOG_MME_APP, "Expired- MME S10 Handover Completion timer for UE " MME_UE_S1AP_ID_FMT " run out. "
      "Performing S1AP UE Context Release Command and successive NAS implicit detach. \n", ue_context_p->mme_ue_s1ap_id);
  ue_context_p->mme_mobility_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
  ue_context_p->ue_context_rel_cause = S1AP_HANDOVER_CANCELLED;
  /** Send a UE Context Release Command which would trigger a context release. */
  mme_app_itti_ue_context_release(ue_context_p->mme_ue_s1ap_id, ue_context_p->enb_ue_s1ap_id, ue_context_p->ue_context_rel_cause, ue_context_p->pending_handover_target_enb_id);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_mobility_completion_timer_expiry (struct ue_context_s *ue_context_p)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context_p != NULL);
  MessageDef                             *message_p = NULL;
  OAILOG_INFO (LOG_MME_APP, "Expired- MME Mobility Completion timer for UE " MME_UE_S1AP_ID_FMT " run out. \n", ue_context_p->mme_ue_s1ap_id);
  if(ue_context_p->pending_clear_location_request){
    OAILOG_INFO (LOG_MME_APP, "CLR flag is set for UE " MME_UE_S1AP_ID_FMT ". Performing implicit detach. \n", ue_context_p->mme_ue_s1ap_id);
    ue_context_p->mme_mobility_completion_timer.id = MME_APP_TIMER_INACTIVE_ID;
    ue_context_p->ue_context_rel_cause = S1AP_NAS_DETACH;
    /** Check if the CLR flag has been set. */
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context_p->mme_ue_s1ap_id; /**< We don't send a Detach Type such that no Detach Request is sent to the UE if handover is performed. */

    MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  }else{
    OAILOG_WARNING(LOG_MME_APP, "CLR flag is NOT set for UE " MME_UE_S1AP_ID_FMT ". Not performing implicit detach. \n", ue_context_p->mme_ue_s1ap_id);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_initial_context_setup_rsp_timer_expiry (struct ue_context_s *ue_context_p)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context_p != NULL);
  MessageDef                             *message_p = NULL;
  OAILOG_INFO (LOG_MME_APP, "Expired- Initial context setup rsp timer for UE id  %d \n", ue_context_p->mme_ue_s1ap_id);
  ue_context_p->initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  /* *********Abort the ongoing procedure*********
   * Check if UE is registered already that implies service request procedure is active. If so then release the S1AP
   * context and move the UE back to idle mode. Otherwise if UE is not yet registered that implies attach procedure is
   * active. If so,then abort the attach procedure and release the UE context.
   */
  ue_context_p->ue_context_rel_cause = S1AP_INITIAL_CONTEXT_SETUP_FAILED;
  if (ue_context_p->mm_state == UE_UNREGISTERED) {
    // Initiate Implicit Detach for the UE
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context_p->mme_ue_s1ap_id;
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  } else {
    // Release S1-U bearer and move the UE to idle mode
    mme_app_send_s11_release_access_bearers_req(ue_context_p);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
void
mme_app_handle_initial_context_setup_failure (
  const itti_mme_app_initial_context_setup_failure_t * const initial_ctxt_setup_failure_pP)
{
  struct ue_context_s                    *ue_context_p = NULL;
  MessageDef                             *message_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);
  OAILOG_DEBUG (LOG_MME_APP, "Received MME_APP_INITIAL_CONTEXT_SETUP_FAILURE from S1AP\n");
  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, initial_ctxt_setup_failure_pP->mme_ue_s1ap_id);

  if (ue_context_p == NULL) {
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this mme_ue_s1ap_id in list of UE: %d \n", initial_ctxt_setup_failure_pP->mme_ue_s1ap_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }
  // Stop Initial context setup process guard timer,if running
  if (ue_context_p->initial_context_setup_rsp_timer.id != MME_APP_TIMER_INACTIVE_ID) {
    if (timer_remove(ue_context_p->initial_context_setup_rsp_timer.id)) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to stop Initial Context Setup Rsp timer for UE id  %d \n", ue_context_p->mme_ue_s1ap_id);
    }
    ue_context_p->initial_context_setup_rsp_timer.id = MME_APP_TIMER_INACTIVE_ID;
  }
  /* *********Abort the ongoing procedure*********
   * Check if UE is registered already that implies service request procedure is active. If so then release the S1AP
   * context and move the UE back to idle mode. Otherwise if UE is not yet registered that implies attach procedure is
   * active. If so,then abort the attach procedure and release the UE context.
   */
  ue_context_p->ue_context_rel_cause = S1AP_INITIAL_CONTEXT_SETUP_FAILED;
  if (ue_context_p->mm_state == UE_UNREGISTERED) {
    // Initiate Implicit Detach for the UE
    message_p = itti_alloc_new_message (TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert (message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id = ue_context_p->mme_ue_s1ap_id;
    itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  } else {
    // Release S1-U bearer and move the UE to idle mode
    mme_app_send_s11_release_access_bearers_req(ue_context_p);
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

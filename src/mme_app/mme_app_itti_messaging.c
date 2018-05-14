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

/*! \file mme_app_itti_messaging.c
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
#include "gcc_diag.h"
#include "mme_config.h"
#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "mme_app_apn_selection.h"
#include "mme_app_pdn_context.h"
#include "mme_app_bearer_context.h"
#include "sgw_ie_defs.h"
#include "common_defs.h"
#include "mme_app_itti_messaging.h"
#include "mme_app_sgw_selection.h"


//------------------------------------------------------------------------------
void mme_app_itti_ue_context_release (
    mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id, enum s1cause cause, target_identification_t *target_id)
{
  MessageDef *message_p;
  OAILOG_FUNC_IN (LOG_MME_APP);

  message_p = itti_alloc_new_message(TASK_MME_APP, S1AP_UE_CONTEXT_RELEASE_COMMAND);
  memset ((void *)&message_p->ittiMsg.s1ap_ue_context_release_command, 0, sizeof (itti_s1ap_ue_context_release_command_t));
  S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).mme_ue_s1ap_id = mme_ue_s1ap_id;
  S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).enb_ue_s1ap_id = enb_ue_s1ap_id;
  S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).enb_id = enb_id;
  S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).cause = cause;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0, "0 S1AP_UE_CONTEXT_RELEASE_COMMAND enb_ue_s1ap_id %06" PRIX32 " ",
                      S1AP_UE_CONTEXT_RELEASE_COMMAND (message_p).enb_ue_s1ap_id);
  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
int mme_app_send_nas_signalling_connection_rel_ind(const mme_ue_s1ap_id_t ue_id)
{
  OAILOG_FUNC_IN(LOG_MME_APP);
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;

  message_p = itti_alloc_new_message(TASK_MME_APP, NAS_SIGNALLING_CONNECTION_REL_IND);

  NAS_SIGNALLING_CONNECTION_REL_IND(message_p).ue_id = ue_id;

  MSC_LOG_TX_MESSAGE(
                MSC_MMEAPP_MME,
                MSC_NAS_MME,
                NULL,0,
                "0 NAS_SIGNALLING_CONNECTION_REL_IND ue id "MME_UE_S1AP_ID_FMT" ", ue_id);

  rc = itti_send_msg_to_task(TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
int mme_app_send_s11_release_access_bearers_req (struct ue_context_s *const ue_context, const pdn_cid_t pdn_index)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  /*
   * Keep the identifier to the default APN
   */
  MessageDef                             *message_p = NULL;
  itti_s11_release_access_bearers_request_t         *release_access_bearers_request_p = NULL;
  int                                     rc = RETURNok;

  DevAssert (ue_context );
  message_p = itti_alloc_new_message (TASK_MME_APP, S11_RELEASE_ACCESS_BEARERS_REQUEST);
  release_access_bearers_request_p = &message_p->ittiMsg.s11_release_access_bearers_request;
  memset ((void*)release_access_bearers_request_p, 0, sizeof (itti_s11_release_access_bearers_request_t));

  // todo: multi apn, send m
  release_access_bearers_request_p->local_teid = ue_context->mme_teid_s11;
  pdn_context_t * pdn_connection = ue_context->pdn_contexts[pdn_index];
  release_access_bearers_request_p->teid = pdn_connection->s_gw_teid_s11_s4;
  release_access_bearers_request_p->peer_ip = pdn_connection->s_gw_address_s11_s4.address.ipv4_address; /**< Not reading from the MME config. */

  release_access_bearers_request_p->originating_node = NODE_TYPE_MME;


  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 S11_RELEASE_ACCESS_BEARERS_REQUEST teid %u", release_access_bearers_request_p->teid);
  rc = itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
int
mme_app_send_s11_create_session_req (
  struct ue_context_s *const ue_context, pdn_context_t * pdn_context, tai_t * serving_tai)
{
  uint8_t                                 i = 0;

  /*
   * Keep the identifier to the default APN
   */
  context_identifier_t                    context_identifier = 0;
  MessageDef                             *message_p = NULL;
  itti_s11_create_session_request_t      *session_request_p = NULL;
  int                                     rc = RETURNok;

  // todo: handover flag in operation-identifier?!

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context);
  DevAssert (pdn_context);
  OAILOG_DEBUG (LOG_MME_APP, "Sending CSR for imsi " IMSI_64_FMT "\n", ue_context->imsi);

  if (ue_context->sub_status != SS_SERVICE_GRANTED) {
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
  memset (session_request_p, 0, sizeof (itti_s11_create_session_request_t));
  /*
   * As the create session request is the first exchanged message and as
   * no tunnel had been previously setup, the distant teid is set to 0.
   * The remote teid will be provided in the response message.
   */
  session_request_p->teid = 0;
  IMSI64_TO_STRING (ue_context->imsi, (char *)session_request_p->imsi.digit);
//  memcpy(&session_request_p->imsi, &ue_context->emm_context._imsi, sizeof(session_request_p->imsi));
 // message content was set to 0
  session_request_p->imsi.length = 15; // todo: optimize!
  /*
   * Copy the MSISDN
   */
  memcpy (session_request_p->msisdn.digit, ue_context->msisdn, blength(ue_context->msisdn));
  session_request_p->msisdn.length = blength(ue_context->msisdn);
  session_request_p->rat_type = RAT_EUTRAN;
  /**
   * Set the indication flag.
   */
  memset(&session_request_p->indication_flags, 0, sizeof(session_request_p->indication_flags));   // TO DO
  session_request_p->indication_flags.oi = 0x1;

  /*
   * Copy the subscribed APN-AMBR to the sgw create session request message
   */
  memcpy (&session_request_p->ambr, &pdn_context->subscribed_apn_ambr, sizeof (ambr_t));

  /*
   * Default EBI
   */
  session_request_p->default_ebi = pdn_context->default_ebi;

  bearer_context_t    *bearer_context_to_setup = NULL;
  RB_FOREACH (bearer_context_to_setup, BearerPool, &pdn_context->session_bearers) {
    DevAssert(bearer_context_to_setup);

    // todo: make it selective for multi PDN!
    /*
     * Set the bearer of the pdn context to establish.
     * Set regardless of GBR/NON-GBR QCI the MBR/GBR values.
     * They are already set to zero if non-gbr in the registration of the bearer contexts.
     */
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.gbr.br_ul = bearer_context_to_setup->esm_ebr_context.gbr_ul;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.gbr.br_dl = bearer_context_to_setup->esm_ebr_context.gbr_dl;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.mbr.br_ul = bearer_context_to_setup->esm_ebr_context.mbr_ul;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.mbr.br_dl = bearer_context_to_setup->esm_ebr_context.gbr_dl;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.qci = bearer_context_to_setup->qci;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pvi = bearer_context_to_setup->preemption_vulnerability;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pci = bearer_context_to_setup->preemption_capability;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pl  = bearer_context_to_setup->priority_level;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].eps_bearer_id = bearer_context_to_setup->ebi;
    session_request_p->bearer_contexts_to_be_created.num_bearer_context++;
    /** Update the bearer state. */
    // todo: setting the bearer state as MME_CREATED when successfull CSResp is received and as ACTIVE after successfull MBResp is received!!
    bearer_context_to_setup->bearer_state   |= BEARER_STATE_MME_CREATED;
    // todo: set TFTs for dedicated bearers (currently PCRF triggered).
  }
  // todo: apn restrictions!
  /*
   * Asking for default bearer in initial UE message.
   * Use the address of ue_context as unique TEID: Need to find better here
   * and will generate unique id only for 32 bits platforms.
   */
  OAI_GCC_DIAG_OFF(pointer-to-int-cast);
  session_request_p->sender_fteid_for_cp.teid = (teid_t) ue_context;
  OAI_GCC_DIAG_ON(pointer-to-int-cast);
  session_request_p->sender_fteid_for_cp.interface_type = S11_MME_GTP_C;
  mme_config_read_lock (&mme_config);
  session_request_p->sender_fteid_for_cp.ipv4_address = mme_config.ipv4.s11;
  mme_config_unlock (&mme_config);
  session_request_p->sender_fteid_for_cp.ipv4 = 1;

  //ue_context->mme_teid_s11 = session_request_p->sender_fteid_for_cp.teid;
  pdn_context->s_gw_teid_s11_s4 = 0;

  mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context,
                                   ue_context->enb_s1ap_id_key,
                                   ue_context->mme_ue_s1ap_id,
                                   ue_context->imsi,
                                   session_request_p->sender_fteid_for_cp.teid,       // mme_s11_teid is new
                                   &ue_context->guti);

  memcpy (session_request_p->apn, pdn_context->apn_in_use->data, pdn_context->apn_in_use->slen);
//  memcpy (session_request_p->apn, ue_context_p->pending_pdn_connectivity_req_apn->data, ue_context_p->pending_pdn_connectivity_req_apn->slen);

  /*
   * Set PDN type for pdn_type and PAA even if this IE is redundant
   */
  session_request_p->pdn_type = pdn_context->pdn_type;
  session_request_p->paa.pdn_type = pdn_context->pdn_type;

  if (!pdn_context->paa) {
    /*
     * UE DHCPv4 allocated ip address
     */
    session_request_p->paa.ipv4_address.s_addr = INADDR_ANY;
    session_request_p->paa.ipv6_address = in6addr_any;
  } else {
    //       memcpy (session_request_p->paa.ipv4_address, ue_context_p->pending_pdn_connectivity_req_pdn_addr->data, 4); /**< String to array. */
    session_request_p->paa.ipv4_address.s_addr = ip_address->address.ipv4_address.s_addr;
    memcpy (&session_request_p->paa.ipv6_address, &ip_address->address.ipv6_address, sizeof(session_request_p->paa.ipv6_address));
    session_request_p->paa.ipv6_prefix_length = pdn_context->paa->ipv6_prefix_length;
  }

//  session_request_p->apn_restriction = 0x00; todo: set them where?
  if(pdn_context->pco){ /**< todo: Should not exist in handover, where to get them?. */
    copy_protocol_configuration_options (&session_request_p->pco, pdn_context->pco);
  }

//  mme_config_read_lock (&mme_config);
//  session_request_p->peer_ip = mme_config.ipv4.sgw_s11;
//  mme_config_unlock (&mme_config);
  // TODO perform SGW selection
  // Actually, since S and P GW are bundled together, there is no PGW selection (based on PGW id in ULA, or DNS query based on FQDN)
  if (1) {
    // TODO prototype may change
    mme_app_select_sgw(serving_tai, &session_request_p->peer_ip);
  }

  session_request_p->serving_network.mcc[0] = serving_tai->plmn.mcc_digit1;
  session_request_p->serving_network.mcc[1] = serving_tai->plmn.mcc_digit2;
  session_request_p->serving_network.mcc[2] = serving_tai->plmn.mcc_digit3;
  session_request_p->serving_network.mnc[0] = serving_tai->plmn.mnc_digit1;
  session_request_p->serving_network.mnc[1] = serving_tai->plmn.mnc_digit2;
  session_request_p->serving_network.mnc[2] = serving_tai->plmn.mnc_digit3;
  session_request_p->selection_mode = MS_O_N_P_APN_S_V;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0,
      "0 S11_CREATE_SESSION_REQUEST imsi " IMSI_64_FMT, ue_context_pP->imsi);
  rc = itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
int
mme_app_send_s11_modify_bearer_req(
  struct ue_context_s *const ue_context, pdn_context_t * pdn_context)
{
  uint8_t                                 i = 0;
  /*
   * Keep the identifier to the default APN
   */
  context_identifier_t                    context_identifier = 0;
  MessageDef                             *message_p = NULL;
  itti_s11_modify_bearer_request_t       *s11_modify_bearer_request = NULL;
  int                                     rc = RETURNok;
  // todo: handover flag in operation-identifier?!
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context);
  DevAssert (pdn_context);
  OAILOG_DEBUG (LOG_MME_APP, "Sending MBR for imsi " IMSI_64_FMT "\n", ue_context->imsi);

  message_p = itti_alloc_new_message (TASK_MME_APP, S11_MODIFY_BEARER_REQUEST);
  AssertFatal (message_p , "itti_alloc_new_message Failed");
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
  s11_modify_bearer_request = &message_p->ittiMsg.s11_modify_bearer_request;
//  memset (s11_modify_bearer_request, 0, sizeof (itti_s11_modify_bearer_request_t));
  /*
   * As the create session request is the first exchanged message and as
   * no tunnel had been previously setup, the distant teid is set to 0.
   * The remote teid will be provided in the response message.
   */

  s11_modify_bearer_request->local_teid = ue_context->mme_teid_s11;
  /*
   * Delay Value in integer multiples of 50 millisecs, or zero
   */
  s11_modify_bearer_request->delay_dl_packet_notif_req = 0;  // TO DO
  s11_modify_bearer_request->peer_ip.s_addr = pdn_context->s_gw_address_s11_s4.address.ipv4_address.s_addr;
  // todo: IPv6
  s11_modify_bearer_request->teid           = pdn_context->s_gw_teid_s11_s4;

  /** Add the bearers to establish. */
  bearer_context_t * bearer_context_to_establish = NULL;
  RB_FOREACH (bearer_context_to_establish, BearerPool, &registered_pdn_ctx->session_bearers) {
    DevAssert(registered_pdn_ctx);
    /** Add them to the bearears list of the MBR. */
    s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[s11_modify_bearer_request->bearer_contexts_to_be_modified.num_bearer_context].eps_bearer_id =
        bearer_context_to_establish->ebi;
    memcpy (&s11_modify_bearer_request->bearer_contexts_to_be_modified.bearer_contexts[s11_modify_bearer_request->bearer_contexts_to_be_modified.num_bearer_context].s1_eNB_fteid,
        &bearer_context_to_establish->enb_fteid_s1u, sizeof(bearer_context_to_establish->enb_fteid_s1u));
    s11_modify_bearer_request->bearer_contexts_to_be_modified.num_bearer_context++;
  }
  s11_modify_bearer_request->bearer_contexts_to_be_removed.num_bearer_context = 0; // todo: also at REGISTRATION no congestion related removals expected
  s11_modify_bearer_request->mme_fq_csid.node_id_type = GLOBAL_UNICAST_IPv4; // TO DO
  s11_modify_bearer_request->mme_fq_csid.csid = 0;   // TO DO ...
  memset(&s11_modify_bearer_request->indication_flags, 0, sizeof(s11_modify_bearer_request->indication_flags));   // TO DO
  s11_modify_bearer_request->rat_type = RAT_EUTRAN;

  /*
   * S11 stack specific parameter. Not used in standalone epc mode
   */
  s11_modify_bearer_request->trxn = NULL;
  /** Update the bearer state with Modify Bearer Response, not here. */
  // todo: apn restrictions!
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0,
      "0 S11_MODIFY_BEARER_REQUEST imsi " IMSI_64_FMT, ue_context_pP->imsi);
  rc = itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
int mme_app_remove_s10_tunnel_endpoint(teid_t local_teid, teid_t remote_teid, struct in_addr peer_ip){
  OAILOG_FUNC_IN(LOG_MME_APP);

  MessageDef *message_p = itti_alloc_new_message (TASK_MME_APP, S10_REMOVE_UE_TUNNEL);
  DevAssert (message_p != NULL);
  message_p->ittiMsg.s10_remove_ue_tunnel.remote_teid  = remote_teid;
  message_p->ittiMsg.s10_remove_ue_tunnel.local_teid   = local_teid;
  message_p->ittiMsg.s10_remove_ue_tunnel.peer_ip      = peer_ip;
  message_p->ittiMsg.s10_remove_ue_tunnel.cause = LOCAL_DETACH;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
  itti_send_msg_to_task (TASK_S10, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
int mme_app_send_delete_session_request (struct ue_context_s * const ue_context_p, const ebi_t ebi, const pdn_context_t* pdn_context)
{
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;
  OAILOG_FUNC_IN (LOG_MME_APP);

  message_p = itti_alloc_new_message (TASK_MME_APP, S11_DELETE_SESSION_REQUEST);
  AssertFatal (message_p , "itti_alloc_new_message Failed");
  S11_DELETE_SESSION_REQUEST (message_p).local_teid = ue_context_p->mme_teid_s11;
  S11_DELETE_SESSION_REQUEST (message_p).teid       = pdn_context->s_gw_teid_s11_s4;
  S11_DELETE_SESSION_REQUEST (message_p).lbi        = ebi; //default bearer

  OAI_GCC_DIAG_OFF(pointer-to-int-cast);
  S11_DELETE_SESSION_REQUEST (message_p).sender_fteid_for_cp.teid = (teid_t) ue_context_p;
  OAI_GCC_DIAG_ON(pointer-to-int-cast);
  S11_DELETE_SESSION_REQUEST (message_p).sender_fteid_for_cp.interface_type = S11_MME_GTP_C;
  mme_config_read_lock (&mme_config);
  S11_DELETE_SESSION_REQUEST (message_p).sender_fteid_for_cp.ipv4_address = mme_config.ipv4.s11;
  mme_config_unlock (&mme_config);
  S11_DELETE_SESSION_REQUEST (message_p).sender_fteid_for_cp.ipv4 = 1;

  S11_DELETE_SESSION_REQUEST (message_p).indication_flags.oi = 1;

  /*
   * S11 stack specific parameter. Not used in standalone epc mode
   */
  S11_DELETE_SESSION_REQUEST  (message_p).trxn = NULL;
  mme_config_read_lock (&mme_config);
  S11_DELETE_SESSION_REQUEST (message_p).peer_ip = pdn_context->s_gw_address_s11_s4.address.ipv4_address;
  mme_config_unlock (&mme_config);

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME,
                      NULL, 0, "0  S11_DELETE_SESSION_REQUEST teid %u lbi %u",
                      S11_DELETE_SESSION_REQUEST  (message_p).teid,
                      S11_DELETE_SESSION_REQUEST  (message_p).lbi);

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0,
      "0 S11_DELETE_SESSION_REQUEST imsi " IMSI_64_FMT, ue_context_pP->imsi);
  rc = itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
/**
 * Send an S1AP Handover Cancel Acknowledge to the S1AP layer.
 */
static inline void mme_app_send_s1ap_handover_cancel_acknowledge(mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id, sctp_assoc_id_t assoc_id){
  OAILOG_FUNC_IN (LOG_MME_APP);
  MessageDef * message_p = itti_alloc_new_message (TASK_MME_APP, S1AP_HANDOVER_CANCEL_ACKNOWLEDGE);
  DevAssert (message_p != NULL);

  itti_s1ap_handover_cancel_acknowledge_t *s1ap_handover_cancel_acknowledge_p = &message_p->ittiMsg.s1ap_handover_cancel_acknowledge;
  memset ((void*)s1ap_handover_cancel_acknowledge_p, 0, sizeof (itti_s1ap_handover_cancel_acknowledge_t));

  /** Set the identifiers. */
  s1ap_handover_cancel_acknowledge_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  s1ap_handover_cancel_acknowledge_p->enb_ue_s1ap_id = enb_ue_s1ap_id;
  s1ap_handover_cancel_acknowledge_p->assoc_id = assoc_id;

  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "MME_APP Sending S1AP HANDOVER_CANCEL_ACKNOWLEDGE");
  /** Sending a message to S10. */
  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
static void notify_s1ap_new_ue_mme_s1ap_id_association (const sctp_assoc_id_t   assoc_id,
    const enb_ue_s1ap_id_t  enb_ue_s1ap_id,
    const mme_ue_s1ap_id_t  mme_ue_s1ap_id)
{
  MessageDef                             *message_p = NULL;
  itti_mme_app_s1ap_mme_ue_id_notification_t *notification_p = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);

  message_p = itti_alloc_new_message (TASK_MME_APP, MME_APP_S1AP_MME_UE_ID_NOTIFICATION);
  notification_p = &message_p->ittiMsg.mme_app_s1ap_mme_ue_id_notification;
  memset (notification_p, 0, sizeof (itti_mme_app_s1ap_mme_ue_id_notification_t));
  notification_p->enb_ue_s1ap_id = enb_ue_s1ap_id;
  notification_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  notification_p->sctp_assoc_id  = assoc_id;

  itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_DEBUG (LOG_MME_APP, " Sent MME_APP_S1AP_MME_UE_ID_NOTIFICATION to S1AP for UE Id %u and enbUeS1apId %u\n", notification_p->mme_ue_s1ap_id, notification_p->enb_ue_s1ap_id);
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

//------------------------------------------------------------------------------
int
mme_app_send_s11_create_bearer_rsp (
  struct ue_context_s *const ue_context,
//  struct in_addr  peer_ip,
  teid_t          saegw_s11_teid,
  LIST_HEAD(bearer_contexts_sucess_s, bearer_context_s) *bearer_contexts_success,
  LIST_HEAD(bearer_contexts_failed_s, bearer_context_s) *bearer_contexts_failed)
{
  /*
   * Keep the identifier to the default APN
   */
  context_identifier_t                    context_identifier = 0;
  MessageDef                             *message_p = NULL;
  itti_s11_create_session_request_t      *session_request_p = NULL;
  int                                     rc = RETURNok;

  // todo: handover flag in operation-identifier?!

  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (ue_context);
  OAILOG_DEBUG (LOG_MME_APP, "Sending Create Bearer Response for imsi " IMSI_64_FMT "\n", ue_context->imsi);

  message_p = itti_alloc_new_message (TASK_MME_APP, S11_CREATE_BEARER_RESPONSE);
  AssertFatal (message_p , "itti_alloc_new_message Failed");
  itti_s11_create_bearer_response_t *s11_create_bearer_response = &message_p->ittiMsg.s11_create_bearer_response;
  s11_create_bearer_response->local_teid = ue_context->mme_teid_s11;
  s11_create_bearer_response->trxn = NULL;
  s11_create_bearer_response->cause.cause_value = 0;

  int msg_bearer_index = 0;
  bearer_context_t *bearer_context = NULL;
  if(bearer_contexts_success){
    LIST_FOREACH(bearer_context, bearer_contexts_success, temp_entries) {
//
////  RB_FOREAC(for (int i = 0; i < e_rab_setup_rsp->e_rab_setup_list.no_of_items; i++) {
////    e_rab_id_t e_rab_id = e_rab_setup_rsp->e_rab_setup_list.item[i].e_rab_id;
////    bearer_context_t * bc = mme_app_get_session_bearer_context_from_all(ue_context_p, (ebi_t) e_rab_id);
//    if (bearer_context_to_establish->bearer_state & BEARER_STATE_ENB_CREATED) {
      s11_create_bearer_response->cause.cause_value = REQUEST_ACCEPTED;
      s11_create_bearer_response->bearer_contexts.bearer_contexts[msg_bearer_index].eps_bearer_id = bearer_context->ebi;
      s11_create_bearer_response->bearer_contexts.bearer_contexts[msg_bearer_index].cause.cause_value = REQUEST_ACCEPTED;
      //  FTEID eNB
      s11_create_bearer_response->bearer_contexts.bearer_contexts[msg_bearer_index].s1u_enb_fteid = bearer_context->enb_fteid_s1u;

      // FTEID SGW S1U
      s11_create_bearer_response->bearer_contexts.bearer_contexts[msg_bearer_index].s1u_sgw_fteid = bearer_context->s_gw_fteid_s1u;       ///< This IE shall be sent on the S11 interface. It shall be used
      s11_create_bearer_response->bearer_contexts.num_bearer_context++;
      msg_bearer_index++;
    }
  }

  /** Install the failed bearer contexts. */
  bearer_context = NULL;
  /** Update the result. */
  if(bearer_contexts_failed){
    if (REQUEST_ACCEPTED == s11_create_bearer_response->cause.cause_value) {
      s11_create_bearer_response->cause.cause_value = REQUEST_ACCEPTED_PARTIALLY;
    } else {
      s11_create_bearer_response->cause.cause_value = REQUEST_REJECTED;
    }
    /* Set the beaer contexts. Continue with the last actual msg_bearer_index. */
    LIST_FOREACH(bearer_context, bearer_contexts_failed, temp_entries) {
      s11_create_bearer_response->bearer_contexts.bearer_contexts[msg_bearer_index].eps_bearer_id = bearer_context->ebi;
      s11_create_bearer_response->bearer_contexts.bearer_contexts[msg_bearer_index].cause.cause_value = REQUEST_REJECTED;

      // FTEID SGW S1U (also in reject case)
      s11_create_bearer_response->bearer_contexts.bearer_contexts[msg_bearer_index].s1u_sgw_fteid = bearer_context->s_gw_fteid_s1u;       ///< This IE shall be sent on the S11 interface. It shall be used
      s11_create_bearer_response->bearer_contexts.num_bearer_context++;
      msg_bearer_index++;
    }
  }

  s11_create_bearer_response->teid = saegw_s11_teid;
////  s11_create_bearer_response->
//
//  ////  mme_config_read_lock (&mme_config);
//////  session_request_p->peer_ip = mme_config.ipv4.sgw_s11;
//////  mme_config_unlock (&mme_config);
////  // TODO perform SGW selection
////  // Actually, since S and P GW are bundled together, there is no PGW selection (based on PGW id in ULA, or DNS query based on FQDN)
//  if (1) {
//    // TODO prototype may change
//    mme_app_select_sgw(serving_tai, &session_request_p->peer_ip);
//  }


  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME,  MSC_S11_MME ,
      NULL, 0, "0 S11_CREATE_BEARER_RESPONSE teid %u", s11_create_bearer_response->teid);
  itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

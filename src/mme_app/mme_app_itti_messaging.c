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

  bearer_context_t    bearer_context_to_setup = NULL;
  RB_FOREACH (bearer_context_to_setup, BearerPool, &pdn_context->session_bearers) {
    DevAssert(bearer_context_to_setup);
    /*
     * Set the bearer of the pdn context to establish.
     * Set regardless of GBR/NON-GBR QCI the MBR/GBR values.
     * They are already set to zero if non-gbr in the registration of the bearer contexts.
     */
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.gbr.br_ul = bearer_context_to_setup.esm_ebr_context.gbr_ul;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.gbr.br_dl = bearer_context_to_setup.esm_ebr_context.gbr_dl;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.mbr.br_ul = bearer_context_to_setup.esm_ebr_context.mbr_ul;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.mbr.br_dl = bearer_context_to_setup.esm_ebr_context.gbr_dl;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.qci = bearer_context_to_setup.qci;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pvi = bearer_context_to_setup.preemption_vulnerability;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pci = bearer_context_to_setup.preemption_capability;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pl  = bearer_context_to_setup.priority_level;
    session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].eps_bearer_id = bearer_context_to_setup.ebi;
    session_request_p->bearer_contexts_to_be_created.num_bearer_context++;
    /** Update the bearer state. */
    bearer_context_to_setup->bearer_state   |= BEARER_STATE_MME_CREATED;

    // todo: set TFTs for dedicated bearers

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

  // todo: where to set this from? config?
  //  session_request_p->apn_restriction = 0x00;
//  copy_protocol_configuration_options (&session_request_p->pco, &ue_context->pending_pdn_connectivity_req_pco);
//  clear_protocol_configuration_options(&ue_context->pending_pdn_connectivity_req_pco);
//  if (ue_context->pdn_contexts[pdn_cid]->pco) {
//    copy_protocol_configuration_options (&session_request_p->pco, ue_context->pdn_contexts[pdn_cid]->pco);
//  }


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



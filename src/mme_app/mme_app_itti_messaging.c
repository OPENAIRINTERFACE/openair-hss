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
    mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id, enum s1cause cause, uint32_t enb_id)
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
int mme_app_send_s11_release_access_bearers_req (struct ue_mm_context_s *const ue_mm_context, const pdn_cid_t pdn_index)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  /*
   * Keep the identifier to the default APN
   */
  MessageDef                             *message_p = NULL;
  itti_s11_release_access_bearers_request_t         *release_access_bearers_request_p = NULL;
  int                                     rc = RETURNok;

  DevAssert (ue_mm_context );
  message_p = itti_alloc_new_message (TASK_MME_APP, S11_RELEASE_ACCESS_BEARERS_REQUEST);
  release_access_bearers_request_p = &message_p->ittiMsg.s11_release_access_bearers_request;
  memset ((void*)release_access_bearers_request_p, 0, sizeof (itti_s11_release_access_bearers_request_t));

  // todo: multi apn, send m
  release_access_bearers_request_p->local_teid = ue_mm_context->mme_teid_s11;
  pdn_context_t * pdn_connection = ue_mm_context->pdn_contexts[pdn_index];
  release_access_bearers_request_p->teid = pdn_connection->s_gw_teid_s11_s4;
  release_access_bearers_request_p->peer_ip = pdn_connection->s_gw_address_s11_s4.address.ipv4_address; /**< Not reading from the MME config. */

  release_access_bearers_request_p->originating_node = NODE_TYPE_MME;


  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 S11_RELEASE_ACCESS_BEARERS_REQUEST teid %u", release_access_bearers_request_p->teid);
  rc = itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}


//------------------------------------------------------------------------------
int mme_app_send_s11_create_session_req (struct ue_mm_context_s *const ue_mm_context, const pdn_cid_t pdn_cid)
{
  OAILOG_FUNC_IN (LOG_MME_APP);

  /*
   * Keep the identifier to the default APN
   */
  MessageDef                             *message_p = NULL;
  itti_s11_create_session_request_t      *session_request_p = NULL;
  int                                     rc = RETURNok;

  DevAssert (ue_mm_context );
  OAILOG_DEBUG (LOG_MME_APP, "Handling imsi " IMSI_64_FMT "\n", ue_mm_context->emm_context._imsi64);

  if (ue_mm_context->subscriber_status != SS_SERVICE_GRANTED) {
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
  memcpy(&session_request_p->imsi, &ue_mm_context->emm_context._imsi, sizeof(session_request_p->imsi));

  /*
   * Copy the MSISDN
   */
  if (ue_mm_context->msisdn) {
    memcpy (session_request_p->msisdn.digit, ue_mm_context->msisdn->data, ue_mm_context->msisdn->slen);
    session_request_p->msisdn.length = ue_mm_context->msisdn->slen;
  } else {
    session_request_p->msisdn.length = 0;
  }
  session_request_p->rat_type = RAT_EUTRAN;
  /*
   * Copy the subscribed ambr to the sgw create session request message
   */
  memcpy (&session_request_p->ambr, &ue_mm_context->suscribed_ue_ambr, sizeof (ambr_t));


  // default bearer already created by NAS
  bearer_context_t *bc = mme_app_get_bearer_context(ue_mm_context, ue_mm_context->pdn_contexts[pdn_cid]->default_ebi);

  bc->bearer_state   |= BEARER_STATE_MME_CREATED;

  // Zero because default bearer (see 29.274)
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.gbr.br_ul = bc->esm_ebr_context.gbr_ul;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.gbr.br_dl = bc->esm_ebr_context.gbr_dl;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.mbr.br_ul = bc->esm_ebr_context.mbr_ul;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.mbr.br_dl = bc->esm_ebr_context.mbr_dl;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.qci       = bc->qci;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pvi       = bc->preemption_vulnerability;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pci       = bc->preemption_capability;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pl        = bc->priority_level;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].eps_bearer_id              = bc->ebi;
  session_request_p->bearer_contexts_to_be_created.num_bearer_context = 1;
  /*
   * Asking for default bearer in initial UE message.
   * Use the address of ue_context as unique TEID: Need to find better here
   * and will generate unique id only for 32 bits platforms.
   */
  OAI_GCC_DIAG_OFF(pointer-to-int-cast);
  session_request_p->sender_fteid_for_cp.teid = (teid_t) ue_mm_context;
  OAI_GCC_DIAG_ON(pointer-to-int-cast);
  session_request_p->sender_fteid_for_cp.interface_type = S11_MME_GTP_C;
  mme_config_read_lock (&mme_config);
  session_request_p->sender_fteid_for_cp.ipv4_address.s_addr = mme_config.ipv4.s11.s_addr;
  mme_config_unlock (&mme_config);
  session_request_p->sender_fteid_for_cp.ipv4 = 1;

  //ue_mm_context->mme_teid_s11 = session_request_p->sender_fteid_for_cp.teid;
  ue_mm_context->pdn_contexts[pdn_cid]->s_gw_teid_s11_s4 = 0;
  mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_mm_context,
                                   ue_mm_context->enb_s1ap_id_key,
                                   ue_mm_context->mme_ue_s1ap_id,
                                   ue_mm_context->emm_context._imsi64,
                                   session_request_p->sender_fteid_for_cp.teid,       // mme_teid_s11 is new
                                   &ue_mm_context->emm_context._guti);
  struct apn_configuration_s *selected_apn_config_p = mme_app_get_apn_config(ue_mm_context, ue_mm_context->pdn_contexts[pdn_cid]->context_identifier);

  memcpy (session_request_p->apn, selected_apn_config_p->service_selection, selected_apn_config_p->service_selection_length);
  /*
   * Set PDN type for pdn_type and PAA even if this IE is redundant
   */
  session_request_p->pdn_type = selected_apn_config_p->pdn_type;
  session_request_p->paa.pdn_type = selected_apn_config_p->pdn_type;

  if (selected_apn_config_p->nb_ip_address == 0) {
    /*
     * UE DHCPv4 allocated ip address
     */
    session_request_p->paa.ipv4_address.s_addr = INADDR_ANY;
    session_request_p->paa.ipv6_address = in6addr_any;
  } else {
    uint8_t                                 j;

    for (j = 0; j < selected_apn_config_p->nb_ip_address; j++) {
      ip_address_t                           *ip_address = &selected_apn_config_p->ip_address[j];

      if (ip_address->pdn_type == IPv4) {
        session_request_p->paa.ipv4_address.s_addr = ip_address->address.ipv4_address.s_addr;
      } else if (ip_address->pdn_type == IPv6) {
        memcpy (&session_request_p->paa.ipv6_address, &ip_address->address.ipv6_address, sizeof(session_request_p->paa.ipv6_address));
      }
    }
  }

  if (ue_mm_context->pdn_contexts[pdn_cid]->pco) {
    copy_protocol_configuration_options (&session_request_p->pco, ue_mm_context->pdn_contexts[pdn_cid]->pco);
  }

  // TODO perform SGW selection
  // Actually, since S and P GW are bundled together, there is no PGW selection (based on PGW id in ULA, or DNS query based on FQDN)
  if (1) {
    // TODO prototype may change
    mme_app_select_sgw(&ue_mm_context->emm_context.originating_tai, &session_request_p->peer_ip);
  }

  session_request_p->serving_network.mcc[0] = ue_mm_context->e_utran_cgi.plmn.mcc_digit1;
  session_request_p->serving_network.mcc[1] = ue_mm_context->e_utran_cgi.plmn.mcc_digit2;
  session_request_p->serving_network.mcc[2] = ue_mm_context->e_utran_cgi.plmn.mcc_digit3;
  session_request_p->serving_network.mnc[0] = ue_mm_context->e_utran_cgi.plmn.mnc_digit1;
  session_request_p->serving_network.mnc[1] = ue_mm_context->e_utran_cgi.plmn.mnc_digit2;
  session_request_p->serving_network.mnc[2] = ue_mm_context->e_utran_cgi.plmn.mnc_digit3;
  session_request_p->selection_mode = MS_O_N_P_APN_S_V;
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0,
      "0 S11_CREATE_SESSION_REQUEST imsi " IMSI_64_FMT, ue_mm_context->emm_context._imsi64);
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
  memset (session_request_p, 0, sizeof (itti_s11_create_session_request_t));
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
     * This could be a Handover message. In that case, fill the elements of the bearer contexts from NAS_PDN_CONNECTIVITY_MSG.
     *
     */
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

  }

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
                                   ue_context_pP->local_mme_s10_teid,       // set to 0
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

  // todo: where to set this from? config?
  session_request_p->apn_restriction = 0x00;


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
// todo: no default_apn set at the moment since no subscription data!
// todo: setting the apn_configuration
// todo: combine this and the other s11_csr method to a single one (w/wo default_apn_config) --> one should overwrite the values of another
int
mme_app_send_s11_create_session_req_from_handover_tau (
    mme_ue_s1ap_id_t ueId)
{
  uint8_t                                 i = 0;
  context_identifier_t                    context_identifier = 0;
  MessageDef                             *message_p = NULL;
  itti_s11_create_session_request_t      *session_request_p = NULL;
  imsi64_t                                imsi64 = INVALID_IMSI64;
  int                                     rc = RETURNok;
  emm_data_context_t                     *ue_nas_ctx = NULL;

  OAILOG_FUNC_IN (LOG_MME_APP);

  /** Find the UE context. */
  ue_context_t * ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ueId);
  DevAssert(ue_context_p); /**< Should always exist. Any mobility issue in which this could occur? */

  /** Not getting the NAS EMM context. */
  OAILOG_INFO(LOG_MME_APP, "Sending CSR for UE in Handover/TAU procedure with mmeUeS1apId " MME_UE_S1AP_ID_FMT ". \n", ueId);
  ue_context_p->imsi_auth = IMSI_AUTHENTICATED;

  /**
   * Trigger a Create Session Request.
   * Keep the identifier to the default APN
   */
  if (ue_context_p->sub_status != SS_SERVICE_GRANTED) {
    /*
     * HSS rejected the bearer creation or roaming is not allowed for this
     * UE. This result will trigger an ESM Failure message sent to UE.
     */
    DevMessage ("Not implemented: ACCESS NOT GRANTED, send ESM Failure to NAS\n");
  }

  /**
   * Check if there are already bearer contexts in the MME_APP UE context,
   * if so no need to send S11 CSReq. Directly respond to the NAS layer.
   */
  bearer_context_t* bearer_ctx = mme_app_is_bearer_context_in_list(ue_context_p->mme_ue_s1ap_id, ue_context_p->default_bearer_id);
  if(bearer_ctx){
    OAILOG_INFO (LOG_MME_APP, "A bearer context is already established for default bearer EBI %d for UE " MME_UE_S1AP_ID_FMT ". \n", ue_context_p->default_bearer_id, ue_context_p->mme_ue_s1ap_id);
    ue_nas_ctx = emm_data_context_get_by_imsi (&_emm_data, ue_context_p->imsi);
      if (ue_nas_ctx) {
        OAILOG_INFO (LOG_MME_APP, "Informing the NAS layer about the received CREATE_SESSION_REQUEST for UE " MME_UE_S1AP_ID_FMT ". \n", ue_context_p->mme_ue_s1ap_id);
        //uint8_t *keNB = NULL;
        message_p = itti_alloc_new_message (TASK_MME_APP, NAS_PDN_CONNECTIVITY_RSP);
        itti_nas_pdn_connectivity_rsp_t *nas_pdn_connectivity_rsp = &message_p->ittiMsg.nas_pdn_connectivity_rsp;
        memset ((void *)nas_pdn_connectivity_rsp, 0, sizeof (itti_nas_pdn_connectivity_rsp_t));
        // moved to NAS_CONNECTION_ESTABLISHMENT_CONF, keNB not handled in NAS MME
        //derive_keNB(ue_context_p->vector_in_use->kasme, 156, &keNB);
        //memcpy(NAS_PDN_CONNECTIVITY_RSP(message_p).keNB, keNB, 32);
        //free(keNB);
        /** Check if this is a handover procedure, set the flag. Don't reset the MME_APP UE context flag till HANDOVER_NOTIFY is received. */
        // todo: states don't match for handover!
    //    if(ue_context_p->mm_state == UE_REGISTERED && (ue_context_p->handover_info != NULL)){
    //      nas_pdn_connectivity_rsp->pending_mobility = true;
    //      /**
    //       * The Handover Information will still be kept in the UE context and not used until HANDOVER_REQUEST is sent to the target ENB..
    //       * Sending CSR may only imply S1AP momentarily.. X2AP is assumed not to switch SAE-GWs (todo: not supported yet).
    //       */
    //    }
        nas_pdn_connectivity_rsp->pti = 0;  // NAS internal ref
        nas_pdn_connectivity_rsp->ue_id = ue_context_p->mme_ue_s1ap_id;      // NAS internal ref

        // TO REWORK:
        if (ue_context_p->pending_pdn_connectivity_req_apn) {
          nas_pdn_connectivity_rsp->apn = bstrcpy (ue_context_p->pending_pdn_connectivity_req_apn);
          bdestroy(ue_context_p->pending_pdn_connectivity_req_apn);
          ue_context_p->pending_pdn_connectivity_req_apn = NULL;
          OAILOG_DEBUG (LOG_MME_APP, "SET APN FROM NAS PDN CONNECTIVITY CREATE: %s\n", bdata(nas_pdn_connectivity_rsp->apn));
        }
        //else {
        int                                     i;
        context_identifier_t                    context_identifier = ue_context_p->apn_profile.context_identifier;

        // todo: for the s1ap handover case, no apn configuration exists yet..
        for (i = 0; i < ue_context_p->apn_profile.nb_apns; i++) {
          if (ue_context_p->apn_profile.apn_configuration[i].context_identifier == context_identifier) {
            AssertFatal (ue_context_p->apn_profile.apn_configuration[i].service_selection_length > 0, "Bad APN string (len = 0)");

            if (ue_context_p->apn_profile.apn_configuration[i].service_selection_length > 0) {
              nas_pdn_connectivity_rsp->apn = blk2bstr(ue_context_p->apn_profile.apn_configuration[i].service_selection,
                  ue_context_p->apn_profile.apn_configuration[i].service_selection_length);
              AssertFatal (ue_context_p->apn_profile.apn_configuration[i].service_selection_length <= APN_MAX_LENGTH, "Bad APN string length %d",
                  ue_context_p->apn_profile.apn_configuration[i].service_selection_length);

              OAILOG_DEBUG (LOG_MME_APP, "SET APN FROM HSS ULA: %s\n", bdata(nas_pdn_connectivity_rsp->apn));
              break;
            }
          }
        }
        //    }
        OAILOG_DEBUG (LOG_MME_APP, "APN: %s\n", bdata(nas_pdn_connectivity_rsp->apn));
        switch (ue_context_p->pending_pdn_connectivity_req_pdn_type) {
        case IPv4:
          nas_pdn_connectivity_rsp->pdn_addr = blk2bstr(ue_context_p->pending_pdn_connectivity_req_pdn_addr, 4);
          DevAssert (nas_pdn_connectivity_rsp->pdn_addr);
          break;

          // todo:
//        case IPv6:
//          DevAssert (create_sess_resp_pP->paa.ipv6_prefix_length == 64);    // NAS seems to only support 64 bits
//          nas_pdn_connectivity_rsp->pdn_addr = blk2bstr(create_sess_resp_pP->paa.ipv6_address, create_sess_resp_pP->paa.ipv6_prefix_length / 8);
//          DevAssert (nas_pdn_connectivity_rsp->pdn_addr);
//          break;
//
//        case IPv4_AND_v6:
//          DevAssert (create_sess_resp_pP->paa.ipv6_prefix_length == 64);    // NAS seems to only support 64 bits
//          nas_pdn_connectivity_rsp->pdn_addr = blk2bstr(create_sess_resp_pP->paa.ipv4_address, 4 + create_sess_resp_pP->paa.ipv6_prefix_length / 8);
//          DevAssert (nas_pdn_connectivity_rsp->pdn_addr);
//          bcatblk(nas_pdn_connectivity_rsp->pdn_addr, create_sess_resp_pP->paa.ipv6_address, create_sess_resp_pP->paa.ipv6_prefix_length / 8);
//          break;
//
//        case IPv4_OR_v6:
//          nas_pdn_connectivity_rsp->pdn_addr = blk2bstr(create_sess_resp_pP->paa.ipv4_address, 4);
//          DevAssert (nas_pdn_connectivity_rsp->pdn_addr);
//          break;

        default:
          DevAssert (0);
        }
        // todo: IP address strings are not cleared

        nas_pdn_connectivity_rsp->pdn_type = ue_context_p->pending_pdn_connectivity_req_pdn_type;
        nas_pdn_connectivity_rsp->proc_data = ue_context_p->pending_pdn_connectivity_req_proc_data;      // NAS internal ref
        ue_context_p->pending_pdn_connectivity_req_proc_data = NULL;
    //#pragma message  "QOS hardcoded here"
        //memcpy(&NAS_PDN_CONNECTIVITY_RSP(message_p).qos,
        //        &ue_context_p->pending_pdn_connectivity_req_qos,
        //        sizeof(network_qos_t));
        nas_pdn_connectivity_rsp->qos.gbrUL = 64;        /* 64=64kb/s   Guaranteed Bit Rate for uplink   */
        nas_pdn_connectivity_rsp->qos.gbrDL = 120;       /* 120=512kb/s Guaranteed Bit Rate for downlink */
        nas_pdn_connectivity_rsp->qos.mbrUL = 72;        /* 72=128kb/s   Maximum Bit Rate for uplink      */
        nas_pdn_connectivity_rsp->qos.mbrDL = 135;       /*135=1024kb/s Maximum Bit Rate for downlink    */
        /*
         * Note : Above values are insignificant because bearer with QCI = 9 is NON-GBR bearer and ESM would not include GBR and MBR values
         * in Activate Default EPS Bearer Context Setup Request message
         */
        nas_pdn_connectivity_rsp->qos.qci = 9;   /* QoS Class Identifier                           */
        nas_pdn_connectivity_rsp->request_type = ue_context_p->pending_pdn_connectivity_req_request_type;        // NAS internal ref
        ue_context_p->pending_pdn_connectivity_req_request_type = 0;
        // here at this point OctetString are saved in resp, no loss of memory (apn, pdn_addr)
        nas_pdn_connectivity_rsp->ue_id = ue_context_p->mme_ue_s1ap_id;
        nas_pdn_connectivity_rsp->ebi = ue_context_p->default_bearer_id;
        nas_pdn_connectivity_rsp->qci = bearer_ctx->qci;
        nas_pdn_connectivity_rsp->prio_level = bearer_ctx->prio_level;
        nas_pdn_connectivity_rsp->pre_emp_vulnerability = bearer_ctx->pre_emp_vulnerability;
        nas_pdn_connectivity_rsp->pre_emp_capability = bearer_ctx->pre_emp_capability;
        nas_pdn_connectivity_rsp->sgw_s1u_teid = bearer_ctx->s_gw_teid;
        memcpy (&nas_pdn_connectivity_rsp->sgw_s1u_address, &bearer_ctx->s_gw_address, sizeof (ip_address_t));
        nas_pdn_connectivity_rsp->ambr.br_ul = ue_context_p->subscribed_ambr.br_ul;
        nas_pdn_connectivity_rsp->ambr.br_dl = ue_context_p->subscribed_ambr.br_dl;
        copy_protocol_configuration_options (&nas_pdn_connectivity_rsp->pco, &ue_context_p->pending_pdn_connectivity_req_pco);
        clear_protocol_configuration_options(&ue_context_p->pending_pdn_connectivity_req_pco);

        MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0, "0 NAS_PDN_CONNECTIVITY_RSP sgw_s1u_teid %u ebi %u qci %u prio %u", bearer_ctx->s_gw_teid, ue_context_p->default_bearer_id, bearer_ctx->qci, bearer_ctx->prio_level);
        rc = itti_send_msg_to_task (TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
        OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
      }else{
        OAILOG_ERROR(LOG_MME_APP, "Bearer context exists but no NAS EMM context exists for UE " MME_UE_S1AP_ID_FMT"\n", ue_context_p->mme_ue_s1ap_id);
        // todo: this case could happen?
        OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
      }
  }else{
    OAILOG_INFO(LOG_MME_APP, "No bearer context exists for UE " MME_UE_S1AP_ID_FMT". Continuing with CSReq. \n", ue_context_p->mme_ue_s1ap_id);
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

  memset (&session_request_p->imsi.digit, 0, 16); /**< IMSI in create session request. */
  memcpy (&session_request_p->imsi.digit, &(ue_context_p->pending_pdn_connectivity_req_imsi), ue_context_p->pending_pdn_connectivity_req_imsi_length);
  session_request_p->imsi.length = strlen ((const char *)session_request_p->imsi.digit);

  /*
   * Copy the MSISDN
   */
  memcpy (session_request_p->msisdn.digit, ue_context_p->msisdn, ue_context_p->msisdn_length);
  session_request_p->msisdn.length = ue_context_p->msisdn_length;
  session_request_p->rat_type = RAT_EUTRAN;

  /**
   * Set the indication flag.
   */
  memset(&session_request_p->indication_flags, 0, sizeof(session_request_p->indication_flags));   // TO DO
  session_request_p->indication_flags.oi = 0x1;

  /*
   * Copy the subscribed ambr to the sgw create session request message
   */
  memcpy (&session_request_p->ambr, &ue_context_p->subscribed_ambr, sizeof (ambr_t));

//  if (ue_context_pP->apn_profile.nb_apns == 0) {
//    DevMessage ("No APN returned by the HSS");
//  }

  context_identifier = ue_context_p->apn_profile.context_identifier;

  /** Set the IPv4 Address from the msg. */
  // todo: currently only 1 IPv4 address is expected.
  //  for (i = 0; i < ue_context_pP->apn_profile.nb_apns; i++) {
  //    default_apn_p = &ue_context_pP->apn_profile.apn_configuration[i];

  // Zero because default bearer (see 29.274) todo: also for handover?
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.gbr.br_ul = 0;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.gbr.br_dl = 0;

  // todo: why should MBR be 0 ?
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.mbr.br_ul = ue_context_p->pending_pdn_connectivity_req_qos.mbrUL;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.mbr.br_dl = ue_context_p->pending_pdn_connectivity_req_qos.mbrDL;
  /** QCI & Bearer ARP. */
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.qci = ue_context_p->pending_pdn_connectivity_req_qos.qci;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pvi = ue_context_p->pending_pdn_connectivity_req_qos.pvi;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pl  = ue_context_p->pending_pdn_connectivity_req_qos.pl;
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].bearer_level_qos.pci = ue_context_p->pending_pdn_connectivity_req_qos.pci;

  /** Set EBI. */
  session_request_p->bearer_contexts_to_be_created.bearer_contexts[0].eps_bearer_id        = ue_context_p->pending_pdn_connectivity_req_ebi;

  session_request_p->bearer_contexts_to_be_created.num_bearer_context = 1;    /**< Multi-PDN Handover. */

  session_request_p->apn_restriction = ue_context_p->pending_pdn_connectivity_req_apn_restriction;

  /*
   * Asking for default bearer in initial UE message.
   * Use the address of ue_context as unique TEID: Need to find better here
   * and will generate unique id only for 32 bits platforms.
   */
  OAI_GCC_DIAG_OFF(pointer-to-int-cast);
  session_request_p->sender_fteid_for_cp.teid = (teid_t) ue_context_p;
  OAI_GCC_DIAG_ON(pointer-to-int-cast);
  session_request_p->sender_fteid_for_cp.interface_type = S11_MME_GTP_C;
  mme_config_read_lock (&mme_config);
  session_request_p->sender_fteid_for_cp.ipv4_address = mme_config.ipv4.s11;
  mme_config_unlock (&mme_config);
  session_request_p->sender_fteid_for_cp.ipv4 = 1;

  //ue_context_pP->mme_s11_teid = session_request_p->sender_fteid_for_cp.teid;
  ue_context_p->sgw_s11_teid = 0;
  mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_context_p,
      ue_context_p->enb_s1ap_id_key,
      ue_context_p->mme_ue_s1ap_id,
      ue_context_p->imsi, /**< Set the IMSI from the EMM data context. */
      session_request_p->sender_fteid_for_cp.teid,       // mme_s11_teid is new
      ue_context_p->local_mme_s10_teid,       // set to 0
      &ue_context_p->guti); /**< Set the invalid context as it is. */

  memcpy (session_request_p->apn, ue_context_p->pending_pdn_connectivity_req_apn->data, ue_context_p->pending_pdn_connectivity_req_apn->slen);
  /*
   * Set PDN type for pdn_type and PAA even if this IE is redundant
   */
  session_request_p->pdn_type = ue_context_p->pending_pdn_connectivity_req_pdn_type;
  session_request_p->paa.pdn_type = ue_context_p->pending_pdn_connectivity_req_pdn_type;

//  session_request_pnas_pdn_connectivity_req_pP->(default_apn_p->nb_ip_address == 0) {
  /*
   * Set the UE IPv4 address
   */
  memset (session_request_p->paa.ipv4_address, 0, 4);
  memset (session_request_p->paa.ipv6_address, 0, 16);
  if (ue_context_p->pending_pdn_connectivity_req_pdn_type == IPv4) {
    /** Copy from IP address. */
    memcpy (session_request_p->paa.ipv4_address, ue_context_p->pending_pdn_connectivity_req_pdn_addr->data, 4); /**< String to array. */
  } else if (ue_context_p->pending_pdn_connectivity_req_pdn_type == IPv6) {
    // todo: UE IPV6 not implemented yet. memcpy (session_request_p->paa.ipv6_address, ip_address->address.ipv6_address, 16);
  }
  // todo: user location information
  // todo: set the serving network from ue_context and mnc/mcc configurations.
//  copy_protocol_configuration_options (&session_request_p->pco, &ue_context_pP->pending_pdn_connectivity_req_pco);
//  clear_protocol_configuration_options(&ue_context_pP->pending_pdn_connectivity_req_pco);

  mme_config_read_lock (&mme_config);
  session_request_p->peer_ip = mme_config.ipv4.sgw_s11;
  mme_config_unlock (&mme_config);

  /**
   * For TAU/Attach Request use the last TAI.
   */
  /** Get the EMM DATA context. */
  ue_nas_ctx = emm_data_context_get(&_emm_data, ueId);
  if(!ue_nas_ctx || ue_nas_ctx->_tai_list.n_tais == 0){
    OAILOG_INFO(LOG_MME_APP, "No EMM Data Context or TAI list exists for UE with mmeUeS1apId " MME_UE_S1AP_ID_FMT " Sending pending TAI.\n", ueId);
    session_request_p->serving_network.mcc[0] = ue_context_p->pending_handover_target_tai.plmn.mcc_digit1;
    session_request_p->serving_network.mcc[1] = ue_context_p->pending_handover_target_tai.plmn.mcc_digit2;
    session_request_p->serving_network.mcc[2] = ue_context_p->pending_handover_target_tai.plmn.mcc_digit3;
    session_request_p->serving_network.mnc[0] = ue_context_p->pending_handover_target_tai.plmn.mnc_digit1;
    session_request_p->serving_network.mnc[1] = ue_context_p->pending_handover_target_tai.plmn.mnc_digit2;
    session_request_p->serving_network.mnc[2] = ue_context_p->pending_handover_target_tai.plmn.mnc_digit3;
    session_request_p->selection_mode = MS_O_N_P_APN_S_V;
  }
  else{
    /// TODO: IMPLEMENT THIS FOR TAU?
  }
  MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0,
       "0 S11_CREATE_SESSION_REQUEST imsi " IMSI_64_FMT, ue_context_p->imsi);
  rc = itti_send_msg_to_task (TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);

}


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



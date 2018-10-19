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

/*! \file itti_free_defined_msg.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "security_types.h"
#include "common_types.h"
#include "common_defs.h"
#include "intertask_interface.h"
#include "itti_free_defined_msg.h"
#include "intertask_messages_types.h"
#include "timer_messages_types.h"
#include "gtpv1_u_messages_types.h"
#include "s11_messages_types.h"
#if MME_BUILD
#include "s1ap_messages_types.h"
#include "nas_messages_types.h"
#include "s6a_messages_types.h"
#include "sctp_messages_types.h"
#include "mme_app_messages_types.h"
#endif
#if SPGW_BUILD
#include "sgw_messages_types.h"
#include "ip_forward_messages_types.h"
#endif
#include "udp_messages_types.h"
#include "async_system_messages_types.h"

#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
void itti_free_msg_content (MessageDef * const message_p)
{
  switch (ITTI_MSG_ID (message_p)) {
  case ASYNC_SYSTEM_COMMAND:{
      if (ASYNC_SYSTEM_COMMAND (message_p)->system_command) {
        bdestroy_wrapper(&ASYNC_SYSTEM_COMMAND (message_p)->system_command);
      }
    }
    break;

  case GTPV1U_CREATE_TUNNEL_REQ:
  case GTPV1U_CREATE_TUNNEL_RESP:
  case GTPV1U_UPDATE_TUNNEL_REQ:
  case GTPV1U_UPDATE_TUNNEL_RESP:
  case GTPV1U_DELETE_TUNNEL_REQ:
  case GTPV1U_DELETE_TUNNEL_RESP:
    // DO nothing
    break;

  case GTPV1U_TUNNEL_DATA_IND:
  case GTPV1U_TUNNEL_DATA_REQ:
    // UNUSED actually
    break;

#if SPGW_BUILD
  case SGI_CREATE_ENDPOINT_REQUEST:
    break;

  case SGI_CREATE_ENDPOINT_RESPONSE: {
    clear_protocol_configuration_options(&SGI_CREATE_ENDPOINT_RESPONSE(message_p)->pco);
  }
  break;

  case SGI_UPDATE_ENDPOINT_REQUEST:
  case SGI_UPDATE_ENDPOINT_RESPONSE:
  case SGI_DELETE_ENDPOINT_REQUEST:
  case SGI_DELETE_ENDPOINT_RESPONSE:
    // DO nothing
    break;
#endif

#if MME_BUILD
  case MME_APP_CONNECTION_ESTABLISHMENT_CNF:
  break;

  case MME_APP_INITIAL_CONTEXT_SETUP_RSP:
  break;

  case MME_APP_DELETE_SESSION_RSP:
    // DO nothing
    break;

  case NAS_PDN_CONNECTIVITY_REQ:{
    clear_protocol_configuration_options(&NAS_PDN_CONNECTIVITY_REQ(message_p)->pco);
    bdestroy_wrapper (&NAS_PDN_CONNECTIVITY_REQ(message_p)->apn);
    bdestroy_wrapper (&NAS_PDN_CONNECTIVITY_REQ(message_p)->pdn_addr);
    AssertFatal(NULL == NAS_PDN_CONNECTIVITY_REQ(message_p)->pdn_addr, "TODO clean pointer");
  }
  break;

  case NAS_INITIAL_UE_MESSAGE:
    bdestroy_wrapper (&message_p->ittiMsg.nas_initial_ue_message.nas.initial_nas_msg);
    AssertFatal(NULL == message_p->ittiMsg.nas_initial_ue_message.nas.initial_nas_msg, "TODO clean pointer");
    break;

  case NAS_CONNECTION_ESTABLISHMENT_CNF:
    bdestroy_wrapper (&NAS_CONNECTION_ESTABLISHMENT_CNF(message_p)->nas_msg);
    AssertFatal(NULL == NAS_CONNECTION_ESTABLISHMENT_CNF(message_p)->nas_msg, "TODO clean pointer");
    break;

  case NAS_CONNECTION_RELEASE_IND:
    // DO nothing
    break;

  case NAS_UPLINK_DATA_IND:
    bdestroy_wrapper (&NAS_UPLINK_DATA_IND(message_p)->nas_msg);
    AssertFatal(NULL == NAS_UPLINK_DATA_IND(message_p)->nas_msg, "TODO clean pointer");
    break;

  case NAS_DOWNLINK_DATA_REQ:
    bdestroy_wrapper (&NAS_DOWNLINK_DATA_REQ(message_p)->nas_msg);
    AssertFatal(NULL == NAS_DOWNLINK_DATA_REQ(message_p)->nas_msg, "TODO clean pointer");
    break;

  case NAS_DOWNLINK_DATA_CNF:
    // DO nothing
    break;

  case NAS_DOWNLINK_DATA_REJ:
    bdestroy_wrapper (&NAS_DOWNLINK_DATA_REJ(message_p)->nas_msg);
    AssertFatal(NULL == NAS_DOWNLINK_DATA_REJ(message_p)->nas_msg, "TODO clean pointer");
    break;

  case NAS_AUTHENTICATION_PARAM_REQ:
  case NAS_DETACH_REQ:
    // DO nothing
    break;

  case NAS_PDN_CONNECTIVITY_RSP:{
    clear_protocol_configuration_options(&NAS_PDN_CONNECTIVITY_RSP(message_p)->pco);
    bdestroy_wrapper (&NAS_PDN_CONNECTIVITY_RSP(message_p)->pdn_addr);
    AssertFatal(NULL == NAS_PDN_CONNECTIVITY_RSP(message_p)->pdn_addr, "TODO clean pointer");
  }
  break;

  case NAS_PDN_CONNECTIVITY_FAIL:
    // DO nothing
    break;
#endif

  case S11_CREATE_SESSION_REQUEST: {
    clear_protocol_configuration_options(&S11_CREATE_SESSION_REQUEST(message_p)->pco);
  }
  break;

  case S11_CREATE_SESSION_RESPONSE: {
    clear_protocol_configuration_options(&S11_CREATE_SESSION_RESPONSE(message_p)->pco);
  }
  break;

  case S11_CREATE_BEARER_REQUEST: {
    clear_protocol_configuration_options(&S11_CREATE_BEARER_REQUEST(message_p)->pco);
  }
  break;

  case S11_CREATE_BEARER_RESPONSE: {
    clear_protocol_configuration_options(&S11_CREATE_BEARER_RESPONSE(message_p)->pco);
  }
  break;

  case S11_MODIFY_BEARER_REQUEST:
  case S11_MODIFY_BEARER_RESPONSE:
  case S11_DELETE_SESSION_REQUEST:
    // DO nothing (trxn)
    break;

  case S11_DELETE_SESSION_RESPONSE: {
    clear_protocol_configuration_options(&S11_DELETE_SESSION_RESPONSE(message_p)->pco);
  }
  break;

  case S11_RELEASE_ACCESS_BEARERS_REQUEST:
  case S11_RELEASE_ACCESS_BEARERS_RESPONSE:
    // DO nothing (trxn)
    break;

#if MME_BUILD
  case S1AP_UPLINK_NAS_LOG:
  case S1AP_UE_CAPABILITY_IND_LOG:
  case S1AP_INITIAL_CONTEXT_SETUP_LOG:
  case S1AP_NAS_NON_DELIVERY_IND_LOG:
  case S1AP_DOWNLINK_NAS_LOG:
  case S1AP_S1_SETUP_LOG:
  case S1AP_INITIAL_UE_MESSAGE_LOG:
  case S1AP_UE_CONTEXT_RELEASE_REQ_LOG:
  case S1AP_UE_CONTEXT_RELEASE_COMMAND_LOG:
  case S1AP_UE_CONTEXT_RELEASE_LOG:
    // DO nothing
    break;

  case S1AP_INITIAL_UE_MESSAGE:
    bdestroy_wrapper (&message_p->ittiMsg.s1ap_initial_ue_message.nas);
    break;

  case S1AP_E_RAB_SETUP_REQ: {
      for (int i = 0; i < message_p->ittiMsg.s1ap_e_rab_setup_req.e_rab_to_be_setup_list.no_of_items; i++) {
        bdestroy_wrapper (&message_p->ittiMsg.s1ap_e_rab_setup_req.e_rab_to_be_setup_list.item[i].nas_pdu);
        bdestroy_wrapper (&message_p->ittiMsg.s1ap_e_rab_setup_req.e_rab_to_be_setup_list.item[i].transport_layer_address);
      }
    }
    break;


  case S1AP_UE_CAPABILITIES_IND:
  case S1AP_ENB_DEREGISTERED_IND:
  case S1AP_DEREGISTER_UE_REQ:
  case S1AP_UE_CONTEXT_RELEASE_REQ:
  case S1AP_UE_CONTEXT_RELEASE_COMMAND:
  case S1AP_UE_CONTEXT_RELEASE_COMPLETE:
    // DO nothing
    break;

  case S6A_UPDATE_LOCATION_REQ:
  case S6A_UPDATE_LOCATION_ANS:
  case S6A_AUTH_INFO_REQ:
  case S6A_AUTH_INFO_ANS:
    // DO nothing
    break;

  case SCTP_INIT_MSG:
    // DO nothing (ipv6_address statically allocated)
    break;

  case SCTP_DATA_REQ:
    bdestroy_wrapper (&SCTP_DATA_REQ(message_p)->payload);
    break;

  case SCTP_DATA_IND:
    bdestroy_wrapper (&SCTP_DATA_IND(message_p)->payload);
    break;

  case SCTP_DATA_CNF:
  case SCTP_NEW_ASSOCIATION:
  case SCTP_CLOSE_ASSOCIATION:
    // DO nothing
    break;
#endif

  case UDP_INIT:
  case UDP_DATA_REQ:
  case UDP_DATA_IND:
    // TODO
   break;
  default:
    ;
  }
  itti_free (ITTI_MSG_ORIGIN_ID (message_p), message_p->itti_msg);
  message_p->itti_msg = NULL;
}

#ifdef __cplusplus
}
#endif

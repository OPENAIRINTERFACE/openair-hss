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

/*! \file s10_mme_session_manager.c
* \brief
* \author Dincer Beken
* \company Blackned GmbH
* \email: dbeken@blackned.de
*/



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "bstrlib.h"

#include "hashtable.h"
#include "obj_hashtable.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "intertask_interface.h"
#include "hashtable.h"
#include "msc.h"

#include "NwGtpv2c.h"
#include "NwGtpv2cIe.h"
#include "NwGtpv2cMsg.h"
#include "NwGtpv2cMsgParser.h"

#include "s10_common.h"
#include "s10_mme_session_manager.h"

#include "gtpv2c_ie_formatter.h"
#include "s10_ie_formatter.h"

#include "mme_ie_defs.h"


extern hash_table_ts_t                        *s10_mme_teid_2_gtv2c_teid_handle;

//------------------------------------------------------------------------------
int
s10_mme_forward_relocation_request (
    nw_gtpv2c_stack_handle_t *stack_p,
    itti_s10_forward_relocation_request_t *req_p)
{
  nw_gtpv2c_ulp_api_t                         ulp_req;
  nw_rc_t                                   rc;
  uint8_t                                 restart_counter = 0;

  DevAssert (stack_p );
  DevAssert (req_p );
  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
  ulp_req.apiType = NW_GTPV2C_ULP_API_INITIAL_REQ;
  /*
   * Prepare a new Forward Relocation Request msg
   */
  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_FORWARD_RELOCATION_REQ, req_p->teid, 0, &(ulp_req.hMsg));
  ulp_req.u_api_info.initialReqInfo.peerIp     = req_p->peer_ip;
  ulp_req.u_api_info.initialReqInfo.teidLocal  = req_p->s10_source_mme_teid.teid;
  ulp_req.u_api_info.initialReqInfo.hUlpTunnel = 0; /** Will create a local tunnel with type INIT_REQ. */
  ulp_req.u_api_info.initialReqInfo.hTunnel    = 0;
  /*
   * Add recovery if contacting the peer for the first time
   */
  rc = nwGtpv2cMsgAddIe ((ulp_req.hMsg), NW_GTPV2C_IE_RECOVERY, 1, 0, (uint8_t *) & restart_counter);
  DevAssert (NW_OK == rc);

  /*
   * Putting the information Elements
   */
  /** IMSI. */
  gtpv2c_imsi_ie_set (&(ulp_req.hMsg), &req_p->imsi);
  /** F-Cause. */
  s10_f_cause_ie_set(&(ulp_req.hMsg), &req_p->f_cause);
  /** APN Restriction. */
//  s10_apn_restriction_ie_set(&(ulp_req.hMsg), 0x01);

  /*
   * Source MME F-TEID for Control Plane (MME S10)
   */
  rc = nwGtpv2cMsgAddIeFteid ((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO,
                              S10_MME_GTP_C,
                              req_p->s10_source_mme_teid.teid,
                              req_p->s10_source_mme_teid.ipv4 ? &req_p->s10_source_mme_teid.ipv4_address : 0,
                                  req_p->s10_source_mme_teid.ipv6 ? &req_p->s10_source_mme_teid.ipv6_address : NULL);

  /*
   * The P-GW TEID should be present on the S10 interface.
   * * * * In case of an initial attach it should be set to 0...
   */
/*  rc = nwGtpv2cMsgAddIeFteid ((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ONE,
                              S5_S8_PGW_GTP_C,
                              req_p->pgw_address_for_cp.teid,
                              req_p->pgw_address_for_cp.ipv4 ? ntohl(req_p->pgw_address_for_cp.ipv4_address) : 0,
                              req_p->pgw_address_for_cp.ipv6 ? req_p->pgw_address_for_cp.ipv6_address : NULL);
*/

  /**
   * Target Identification.
   */
  s10_target_identification_ie_set (&(ulp_req.hMsg), &req_p->target_identification);

  /*
    * Source S10 SAEGW F-TEID for Control Plane
    */
   rc = nwGtpv2cMsgAddIeFteid ((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ONE,
                               S11_SGW_GTP_C,
                               req_p->s11_sgw_teid.teid,
                               req_p->s11_sgw_teid.ipv4 ? &req_p->s11_sgw_teid.ipv4_address : 0,
                                   req_p->s11_sgw_teid.ipv6 ? &req_p->s11_sgw_teid.ipv6_address : NULL);
   DevAssert (NW_OK == rc);

   /**
    * Set the Transparent F-Container.
    */
   rc = nwGtpv2cMsgAddIeFContainer((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO,
       (uint8_t*)req_p->eutran_container.container_value->data,
       blength(req_p->eutran_container.container_value),
       req_p->eutran_container.container_type);
   /** Destroy the container. */
//   const uint8_t container_map[] = {
//       0x40, 0x80, 0xc0, 0x0f, 0x10, 0x3f, 0xc5, 0x9a, 0xd4, 0x80, 0x01, 0x06, 0x0e, 0x4d, 0x2a, 0x64,
//          0xea, 0x04, 0x48, 0xff, 0x8b, 0xfc, 0x8f, 0xf8, 0xbf, 0xc8, 0xff, 0x8b, 0xfc, 0x8f, 0xf8, 0xbf,
//          0xc8, 0xff, 0x8b, 0xfc, 0x8f, 0xf8, 0xbf, 0xc8, 0xff, 0x8b, 0xfc, 0x8f, 0xf8, 0xbf, 0xc8, 0xff,
//          0x8b, 0xff, 0xfe, 0x7f, 0xf5, 0xf4, 0x40, 0x40, 0x70, 0xca, 0x74, 0xa9, 0x3b, 0xbe, 0x06, 0x9c,
//          0x08, 0x00, 0x00, 0x06, 0xc0, 0x15, 0xe0, 0x00, 0x10, 0x06, 0x0e, 0x5a, 0xf0, 0x00, 0x14, 0x80,
//          0x43, 0xcb, 0x80, 0x00, 0x02, 0x32, 0x11, 0x41, 0xbd, 0x80, 0x9e, 0x18, 0xe0, 0x18, 0x3c, 0x1c,
//          0x09, 0xe1, 0x8e, 0x01, 0x83, 0xc0, 0x1f, 0x51, 0x34, 0x81, 0x68, 0x3a, 0x0c, 0x0e, 0x1a, 0x67,
//          0xcc, 0x01, 0xe9, 0x2c, 0x7f, 0xe8, 0x00, 0x8c, 0x30, 0x64, 0x81, 0x4c, 0x02, 0x05, 0x44, 0x0d,
//          0x9c, 0x00, 0x05, 0x45, 0x0a, 0x50, 0xd0, 0x00, 0x29, 0x00, 0x40, 0x70, 0x2a, 0x58, 0x00, 0x28,
//          0x40, 0x24, 0x13, 0x02, 0x59, 0x00, 0x0c, 0xd0, 0x19, 0x00, 0x6a, 0x30, 0x40, 0x08, 0x80, 0x17,
//          0xa5, 0x14, 0x09, 0x09, 0x2a, 0xb7, 0x8d, 0x10, 0x00, 0x1a, 0x05, 0xa1, 0x40, 0x80, 0x00, 0x01,
//          0x00, 0x1c, 0x2b, 0xec, 0x8a, 0xa2, 0x46, 0x00, 0xd4, 0x9b, 0x28, 0x3a, 0x18, 0x38, 0x40, 0x80,
//          0x08, 0x14, 0x00, 0x00, 0x00, 0x4e, 0x40, 0x01, 0x05, 0x00, 0x00, 0xf9, 0x90, 0x00, 0x0d, 0x00,
//          0x14, 0x00, 0x00, 0xf9, 0x90, 0x00, 0x0c, 0xd0, 0x10, 0x80, 0x00, 0x18, 0x00, 0x00, 0xf9, 0x90,
//          0x00, 0x0d, 0x00, 0x10, 0x80, 0x00, 0x0e, 0x00, 0x00, 0xf9, 0x90, 0x00, 0x0c, 0xd0, 0x10, 0x80,
//          0x00, 0x1c, 0x00, 0x00, 0xf9, 0x90, 0x00, 0x0d, 0x00, 0x10, 0x80, 0x00, 0x77, 0x00, 0x00, 0xf9,
//          0x90, 0x00, 0x0c, 0xd0, 0x10, 0x80, 0x00, 0x4d
//   };
//   bstring new_long_container = blk2bstr(container_map, 264);
//   rc = nwGtpv2cMsgAddIeFContainer((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO,
//       (uint8_t*)new_long_container->data,
//       blength(new_long_container),
//       req_p->eutran_container.container_type);
   bdestroy(req_p->eutran_container.container_value);
//   bdestroy(new_long_container);


   DevAssert( NW_OK == rc );


//  s10_serving_network_ie_set (&(ulp_req.hMsg), &req_p->serving_network);
   s10_pdn_connection_ie_set (&(ulp_req.hMsg), &req_p->pdn_connections);

  /**
   * Set the MM EPS UE Context.
   */
  s10_ue_mm_eps_context_ie_set(&(ulp_req.hMsg), &req_p->ue_eps_mm_context);

  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
  DevAssert (NW_OK == rc);
  MSC_LOG_TX_MESSAGE (MSC_S10_MME, MSC_S10_MME, NULL, 0, "0 FORWARD_RELOCATION_REQUEST local S10 teid " TEID_FMT " num pdn connections %u",
    req_p->s10_source_mme_teid.teid, req_p->pdn_connections.num_pdn_connections);

  hashtable_rc_t hash_rc = hashtable_ts_insert(s10_mme_teid_2_gtv2c_teid_handle,
      (hash_key_t) req_p->s10_source_mme_teid.teid,
      (void *)ulp_req.u_api_info.initialReqInfo.hTunnel);
  if (HASH_TABLE_OK == hash_rc) {
    return RETURNok;
  } else {
    OAILOG_WARNING (LOG_S10, "Could not save GTPv2-C hTunnel %p for local teid %X\n", (void*)ulp_req.u_api_info.initialReqInfo.hTunnel, ulp_req.u_api_info.initialReqInfo.teidLocal);
    return RETURNerror;
  }
}

//------------------------------------------------------------------------------
int
s10_mme_handle_forward_relocation_request(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  nw_rc_t                                   rc = NW_OK;
  uint8_t                                 offendingIeType,
                                          offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_s10_forward_relocation_request_t  *req_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                     *pMsgParser;

  DevAssert (stack_p );
  /** Allocating the Signal once at the sender (MME_APP --> S10) and once at the receiver (S10-->MME_APP). */
  message_p = itti_alloc_new_message (TASK_S10, S10_FORWARD_RELOCATION_REQUEST);
  req_p = &message_p->ittiMsg.s10_forward_relocation_request;
  memset(req_p, 0, sizeof(*req_p));

  req_p->teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);
  req_p->trxn = (void *)pUlpApi->u_api_info.initialReqIndInfo.hTrxn;

  /** Check the destination TEID is 0. */
  if(req_p->teid != (teid_t)0){
    OAILOG_WARNING (LOG_S10, "Destination TEID of S10 Forward Relocation Request is not 0, insted " TEID_FMT ". Ignoring S10 Forward Relocation Request. \n", req_p->teid);
    return RETURNerror;
  }

  /*
   * Create a new message parser for the S10 FORWARD RELOCATION REQUEST.
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_FORWARD_RELOCATION_REQ, s10_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);

  /*
   * Sender (Source MME) FTEID for CP IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_FTEID, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
      gtpv2c_fteid_ie_get, &req_p->s10_source_mme_teid);
  DevAssert (NW_OK == rc);

  /*
   * Sender (Source SAE-GW) FTEID for CP IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_FTEID, NW_GTPV2C_IE_INSTANCE_ONE, NW_GTPV2C_IE_PRESENCE_MANDATORY,
      gtpv2c_fteid_ie_get, &req_p->s11_sgw_teid);
  DevAssert (NW_OK == rc);

  /*
   * IMSI IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_IMSI, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      gtpv2c_imsi_ie_get, &req_p->imsi);
  DevAssert (NW_OK == rc);

  /*
   * PDN Connection IE : Several can exist
   * todo: multiple pdn connection IEs can exist with instance 0.
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_PDN_CONNECTION, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
       s10_pdn_connection_ie_get, &req_p->pdn_connections);
  DevAssert (NW_OK == rc);

  /*
   * MME UE MM Context.
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_MM_EPS_CONTEXT, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
       s10_mm_ue_context_ie_get, &req_p->ue_eps_mm_context);
  DevAssert (NW_OK == rc);

  /**
   * E-UTRAN container (F-Container) Information Element.
   * Instance Zero is E-UTRAN. Only E-UTRAN will be supported.
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_F_CONTAINER, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
       s10_f_container_ie_get, &req_p->eutran_container);
  DevAssert (NW_OK == rc);

  /**
   * Target Identifier Information Element.
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_TARGET_IDENTIFICATION, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
       s10_target_identification_ie_get, &req_p->target_identification);
  DevAssert (NW_OK == rc);

  // todo: F-CAUSE not handled!
  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "0 FORWARD_RELOCATION_REQUEST local S10 teid " TEID_FMT " ", req_p->teid);
    /*
     * TODO: handle this case
     */
    itti_free (ITTI_MSG_ORIGIN_ID (message_p), message_p);
    message_p = NULL;
    rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
    DevAssert (NW_OK == rc);
    rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
    DevAssert (NW_OK == rc);
    return RETURNerror;
  }

  rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
  DevAssert (NW_OK == rc);
  rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
  DevAssert (NW_OK == rc);

  MSC_LOG_RX_MESSAGE (MSC_S10_MME, MSC_MMEAPP_MME, NULL, 0, "0 FORWARD_RELOCATION_REQUEST local S10 teid " TEID_FMT " num pdn connections %u", req_p->teid,
      req_p->pdn_connections.num_pdn_connections);
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
s10_mme_forward_relocation_response (
    nw_gtpv2c_stack_handle_t *stack_p,
    itti_s10_forward_relocation_response_t *forward_relocation_response_p)
{

   nw_rc_t                                 rc;
   nw_gtpv2c_ulp_api_t                     ulp_req;
   nw_gtpv2c_trxn_handle_t                 trxn;
   gtpv2c_cause_t                          cause;
   OAILOG_FUNC_IN (LOG_S10);

   DevAssert (forward_relocation_response_p );
   DevAssert (stack_p );
   trxn = (nw_gtpv2c_trxn_handle_t) forward_relocation_response_p->trxn;
   DevAssert (trxn );
   memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));

   /**
    * Create a tunnel for the GTPv2-C stack if the result is success.
    */
   if(forward_relocation_response_p->cause.cause_value == REQUEST_ACCEPTED){
     memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
     ulp_req.apiType = NW_GTPV2C_ULP_CREATE_LOCAL_TUNNEL; /**< Create a Tunnel Endpoint for the S10. */
     ulp_req.u_api_info.createLocalTunnelInfo.teidLocal = forward_relocation_response_p->s10_target_mme_teid.teid;
     ulp_req.u_api_info.createLocalTunnelInfo.peerIp = forward_relocation_response_p->peer_ip;
     ulp_req.u_api_info.createLocalTunnelInfo.hUlpTunnel = 0;
     ulp_req.u_api_info.createLocalTunnelInfo.hTunnel    = 0;
     rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);

     // todo: DevAssert (NW_OK == rc);
     if(NW_OK != rc){
       OAILOG_ERROR(LOG_S10, "Error, we could not setup local tunnel while sending FW_RELOC_RESPONSE!. \n");
       return RETURNerror;
     }


     OAILOG_INFO(LOG_S10, "Successfully created tunnel while FW_RELOCA_RESP!. \n");

     hashtable_rc_t hash_rc = hashtable_ts_insert(s10_mme_teid_2_gtv2c_teid_handle,
         (hash_key_t) ulp_req.u_api_info.createLocalTunnelInfo.teidLocal,
         (void *)ulp_req.u_api_info.createLocalTunnelInfo.hTunnel);

     hash_rc = hashtable_ts_get(s10_mme_teid_2_gtv2c_teid_handle,
         (hash_key_t) ulp_req.u_api_info.createLocalTunnelInfo.teidLocal, (void **)(uintptr_t)&ulp_req.u_api_info.createLocalTunnelInfo.hTunnel);
     DevAssert(hash_rc == HASH_TABLE_OK);
   }else{
     OAILOG_WARNING (LOG_S10, "The cause is not REQUEST_ACCEPTED but %d for S10_FORWARD_RELOCATION_RESPONSE. "
         "Not creating a local S10 Tunnel. \n", forward_relocation_response_p->cause);
   }

   /**
    * Prepare a forward relocation response to send to MME.
    */
   memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
   memset (&cause, 0, sizeof (gtpv2c_cause_t));
   ulp_req.apiType = NW_GTPV2C_ULP_API_TRIGGERED_RSP;
   ulp_req.u_api_info.triggeredRspInfo.hTrxn = trxn;
   rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_FORWARD_RELOCATION_RSP, 0, 0, &(ulp_req.hMsg));
   DevAssert (NW_OK == rc);

   /**
    * Set the destination (source-MME) TEID.
    */
   rc = nwGtpv2cMsgSetTeid (ulp_req.hMsg, forward_relocation_response_p->teid);
   DevAssert (NW_OK == rc);
   /** Add the S10 Cause : Not setting offending IE type now. */
   rc = nwGtpv2cMsgAddIeCause((ulp_req.hMsg), 0, forward_relocation_response_p->cause.cause_value, 0, 0, 0);
   DevAssert( NW_OK == rc );

   if(forward_relocation_response_p->cause.cause_value == REQUEST_ACCEPTED){
     /** Add the S10 target-MME FTEID. */
     rc = nwGtpv2cMsgAddIeFteid ((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO, S10_MME_GTP_C,
         forward_relocation_response_p->s10_target_mme_teid.teid, /**< FTEID of the TARGET_MME. */
         forward_relocation_response_p->s10_target_mme_teid.ipv4 ? &forward_relocation_response_p->s10_target_mme_teid.ipv4_address : 0,
         forward_relocation_response_p->s10_target_mme_teid.ipv6 ? &forward_relocation_response_p->s10_target_mme_teid.ipv6_address : NULL);

     /** F-Cause to be added. */
     rc = nwGtpv2cMsgAddIeFCause((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO,
         forward_relocation_response_p->f_cause.fcause_type, forward_relocation_response_p->f_cause.fcause_value);
     DevAssert( NW_OK == rc );

     /** F-Container. */
     if (forward_relocation_response_p->eutran_container.container_value) {
       rc = nwGtpv2cMsgAddIeFContainer((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO,
           (uint8_t*)forward_relocation_response_p->eutran_container.container_value->data,
           blength(forward_relocation_response_p->eutran_container.container_value),
           forward_relocation_response_p->eutran_container.container_type);
       /** Destroy the container. */
       bdestroy(forward_relocation_response_p->eutran_container.container_value);
       DevAssert( NW_OK == rc );
     }

     /** Setting the Bearer Context to Setup. Just EBI needed. */
     for (int i = 0; i < forward_relocation_response_p->handovered_bearers.num_bearer_context; i++) {
       s10_bearer_context_created_ie_set( &(ulp_req.hMsg), &forward_relocation_response_p->handovered_bearers.bearer_contexts[i]);
     }
   }
   /** No allocated context remains. */
   rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
   DevAssert (NW_OK == rc);
   return RETURNok;
}

//------------------------------------------------------------------------------
int
s10_mme_handle_forward_relocation_response(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{

  nw_rc_t                                   rc = NW_OK;
  uint8_t                                 offendingIeType,
                                            offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_s10_forward_relocation_response_t *resp_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                     *pMsgParser;

  DevAssert (stack_p );
  message_p = itti_alloc_new_message (TASK_S10, S10_FORWARD_RELOCATION_RESPONSE);
  resp_p = &message_p->ittiMsg.s10_forward_relocation_response;
  memset(resp_p, 0, sizeof(*resp_p));

  /** Set the destination TEID. */
  resp_p->teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);

/** Create a new message parser.     */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_FORWARD_RELOCATION_RSP, s10_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);

  /*
   * Cause IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_CAUSE, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
      gtpv2c_cause_ie_get, &resp_p->cause);
  DevAssert (NW_OK == rc);

  /**
   * F-Container.
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_F_CONTAINER, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      s10_f_container_ie_get, &resp_p->eutran_container);
  DevAssert (NW_OK == rc);

  /*
   * Sender FTEID for CP IE
   * todo: TEID not saved in the tunnel endpoint. Why not saving in the S10 tunnel endpoints at all ?
   * It will be saved in the mme_ue_context in the MME_APP layer.
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_FTEID, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      gtpv2c_fteid_ie_get, &resp_p->s10_target_mme_teid);
  DevAssert (NW_OK == rc);

  /*
   * Bearer Contexts Created IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_BEARER_CONTEXT, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      s10_bearer_context_created_ie_get, &resp_p->handovered_bearers);
  DevAssert (NW_OK == rc);
  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "0 FORWARD_RELOCATION_RESPONSE local S10 teid " TEID_FMT " ", resp_p->teid);
    /*
     * TODO: handle this case
     */
    itti_free (ITTI_MSG_ORIGIN_ID (message_p), message_p);
    message_p = NULL;
    rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
    DevAssert (NW_OK == rc);
    rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
    DevAssert (NW_OK == rc);
    return RETURNerror; /**< Timers will take care. */
  }

  rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
  DevAssert (NW_OK == rc);
  rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
  DevAssert (NW_OK == rc);

  MSC_LOG_RX_MESSAGE (MSC_S10_MME, MSC_MMEAPP_MME, NULL, 0, "0 FORWARD_RELOCATION_RESPONSE local S10 teid " TEID_FMT " num bearer ctxt setup %u", resp_p->teid,
    resp_p->list_of_bearers.num_bearer_context);
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
s10_mme_forward_access_context_notification(nw_gtpv2c_stack_handle_t *stack_p,
    itti_s10_forward_access_context_notification_t *forward_access_context_notif_p){
  nw_rc_t                                   rc;
  nw_gtpv2c_ulp_api_t                         ulp_req;
  uint8_t                                 restart_counter = 0;

  DevAssert (forward_access_context_notif_p );
  DevAssert (stack_p );
  /**
   * Create a tunnel for the GTPv2-C stack
   */
  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
  ulp_req.u_api_info.initialReqInfo.teidLocal = forward_access_context_notif_p->local_teid;
  ulp_req.apiType = NW_GTPV2C_ULP_API_INITIAL_REQ; /**< Sending Side. */
  ulp_req.u_api_info.initialReqInfo.peerIp     = forward_access_context_notif_p->peer_ip;
  hashtable_rc_t hash_rc = hashtable_ts_get(s10_mme_teid_2_gtv2c_teid_handle,
      (hash_key_t) ulp_req.u_api_info.initialReqInfo.teidLocal, (void **)(uintptr_t)&ulp_req.u_api_info.initialReqInfo.hTunnel);
  if (HASH_TABLE_OK != hash_rc) {
    OAILOG_WARNING (LOG_S10, "Could not get GTPv2-C hTunnel for local TEID %X on S10 MME interface. \n", ulp_req.u_api_info.initialReqInfo.teidLocal);
    return RETURNerror;
  }
  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_FORWARD_ACCESS_CONTEXT_NTF, 0, 0, &(ulp_req.hMsg));
  DevAssert (NW_OK == rc);
  /*
   * Set the remote TEID
   */
  rc = nwGtpv2cMsgSetTeid (ulp_req.hMsg, forward_access_context_notif_p->teid);
  DevAssert (NW_OK == rc);


  /**
   * Concatenate with header (todo: OAI: how to do this properly?)
   */
  char enbStatusPrefix[] = {0x00, 0x00, 0x00, 0x59, 0x40, 0x0b};
  bstring enbStatusPrefixBstr = blk2bstr (enbStatusPrefix, 6);
  bconcat(enbStatusPrefixBstr, forward_access_context_notif_p->eutran_container.container_value);

  /** F-Container. */
  rc = nwGtpv2cMsgAddIeFContainer((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO,
      (uint8_t*)enbStatusPrefixBstr->data,
      blength(enbStatusPrefixBstr) + 1,
      forward_access_context_notif_p->eutran_container.container_type);
  DevAssert( NW_OK == rc );
  /** Destroy the container. */
  bdestroy(forward_access_context_notif_p->eutran_container.container_value);
  bdestroy(enbStatusPrefixBstr);

  /** Send the message. */
  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
  DevAssert (NW_OK == rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
int
s10_mme_handle_forward_access_context_notification( nw_gtpv2c_stack_handle_t * stack_p, nw_gtpv2c_ulp_api_t * pUlpApi){
  nw_rc_t                                   rc = NW_OK;
  uint8_t                                 offendingIeType,
                                            offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_s10_forward_access_context_notification_t  *notif_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                     *pMsgParser;

  DevAssert (stack_p );
  /** Allocating the Signal once at the sender (MME_APP --> S10) and once at the receiver (S10-->MME_APP). */
  message_p = itti_alloc_new_message (TASK_S10, S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION);
  notif_p = &message_p->ittiMsg.s10_forward_access_context_notification;
  memset(notif_p, 0, sizeof(*notif_p));
  notif_p->teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);
  notif_p->trxn = (void *)pUlpApi->u_api_info.initialReqIndInfo.hTrxn;
  /*
   * Create a new message parser for the S10 FORWARD ACCESS CONTEXT NOTIFICATION.
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_FORWARD_ACCESS_CONTEXT_NTF, s10_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);
  /**
   * E-UTRAN container (F-Container) Information Element.
   * Instance Zero is E-UTRAN. Only E-UTRAN will be supported.
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_F_CONTAINER, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      s10_f_container_ie_get_2, &notif_p->eutran_container);
  DevAssert (NW_OK == rc);

  /**
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "0 FORWARD_ACCESS_CONTEXT_NOTIFICATION local S10 teid " TEID_FMT " ", notif_p->teid);
    /*
     * TODO: handle this case
     */
    itti_free (ITTI_MSG_ORIGIN_ID (message_p), message_p);
    message_p = NULL;
    rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
    DevAssert (NW_OK == rc);
    rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
    DevAssert (NW_OK == rc);
    return RETURNerror;
  }
  // todo: deletion of s10 message and parser!
  rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
  DevAssert (NW_OK == rc);
  rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
  DevAssert (NW_OK == rc);
  MSC_LOG_RX_MESSAGE (MSC_S10_MME, MSC_MMEAPP_MME, NULL, 0, "0 FORWARD_ACCESS_CONTEXT_NOTIFICATION local S10 teid " TEID_FMT , notif_p->teid);
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
s10_mme_forward_access_context_acknowledge(nw_gtpv2c_stack_handle_t *stack_p,
    itti_s10_forward_access_context_acknowledge_t *forward_access_context_ack_p){
  nw_rc_t                                   rc;
  nw_gtpv2c_ulp_api_t                         ulp_ack;
  nw_gtpv2c_trxn_handle_t                     trxn;
  gtpv2c_cause_t                             cause;
  // todo: restart counter

  DevAssert (forward_access_context_ack_p );
  DevAssert (stack_p );
  trxn = (nw_gtpv2c_trxn_handle_t) forward_access_context_ack_p->trxn;
  DevAssert (trxn );
  /*
   * Create a tunnel for the GTPv2-C stack
   */
  memset (&ulp_ack, 0, sizeof (nw_gtpv2c_ulp_api_t));

  ulp_ack.u_api_info.triggeredRspInfo.teidLocal = forward_access_context_ack_p->local_teid; // todo: check which teid is the local one, which the remote..
  //  ulp_req.u_api_info.triggeredRspInfo.peerIp = forward_access_context_ack_p->peer_ip;
  ulp_ack.apiType = NW_GTPV2C_ULP_API_TRIGGERED_RSP; /**< Sending Side. */
  ulp_ack.u_api_info.triggeredRspInfo.hUlpTunnel = 0;
  ulp_ack.u_api_info.triggeredRspInfo.hTunnel    = 0;
  ulp_ack.u_api_info.triggeredRspInfo.hTrxn = trxn;
  //  ulp_ack.u_api_info.triggeredRspInfo.peerIp = forward_access_context_ack_p->peer_ip;

  hashtable_rc_t hash_rc = hashtable_ts_get(s10_mme_teid_2_gtv2c_teid_handle,
      (hash_key_t) ulp_ack.u_api_info.triggeredRspInfo.teidLocal, (void **)(uintptr_t)&ulp_ack.u_api_info.triggeredRspInfo.hTunnel);

  if (HASH_TABLE_OK != hash_rc) {
    OAILOG_WARNING (LOG_S10, "Could not get GTPv2-C hTunnel for local TEID %X on S10 MME interface. \n", ulp_ack.u_api_info.triggeredRspInfo.teidLocal);
    return RETURNerror;
  }
  // todo: hTrxn ?!?

  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_FORWARD_ACCESS_CONTEXT_ACK, 0, 0, &(ulp_ack.hMsg));
  DevAssert (NW_OK == rc);
  /*
   * Set the remote TEID
   * todo: local or remote?
   */
  rc = nwGtpv2cMsgSetTeid (ulp_ack.hMsg, forward_access_context_ack_p->teid);
  DevAssert (NW_OK == rc);
  /*
   * Putting the information Elements
   */
  gtpv2c_cause_ie_set(&(ulp_ack.hMsg), &forward_access_context_ack_p->cause);

  /** Send the message. */
  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_ack);
  DevAssert (NW_OK == rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
int
s10_mme_handle_forward_access_context_acknowledge( nw_gtpv2c_stack_handle_t * stack_p, nw_gtpv2c_ulp_api_t * pUlpApi){
  nw_rc_t                                   rc = NW_OK;
  uint8_t                                   offendingIeType,
                                            offendingIeInstance;
  uint16_t                                  offendingIeLength;
  itti_s10_forward_access_context_acknowledge_t  *ack_p;
  MessageDef                               *message_p;
  nw_gtpv2c_msg_parser_t                   *pMsgParser;

  DevAssert (stack_p );
  /** Allocating the Signal once at the sender (MME_APP --> S10) and once at the receiver (S10-->MME_APP). */
  message_p = itti_alloc_new_message (TASK_S10, S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE);
  ack_p = &message_p->ittiMsg.s10_forward_access_context_acknowledge;
  memset(ack_p, 0, sizeof(*ack_p));

  ack_p->teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);

  /*
   * Create a new message parser for the S10 FORWARD ACCESS CONTEXT NOTIFICATION.
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_FORWARD_ACCESS_CONTEXT_NTF, s10_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);
  /*
   * Cause IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_CAUSE, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
      gtpv2c_cause_ie_get, &ack_p->cause);
  DevAssert (NW_OK == rc);
  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "0 FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE local S10 teid " TEID_FMT " ", ack_p->teid);
    /*
     * TODO: handle this case
     */
    itti_free (ITTI_MSG_ORIGIN_ID (message_p), message_p);
    message_p = NULL;
    rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
    DevAssert (NW_OK == rc);
    rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
    DevAssert (NW_OK == rc);
    return RETURNerror;
  }
  // todo: deletion of s10 message and parser!
  rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
  DevAssert (NW_OK == rc);
  rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
  DevAssert (NW_OK == rc);
  MSC_LOG_RX_MESSAGE (MSC_S10_MME, MSC_S10_MME, NULL, 0, "0 FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE local S10 teid " TEID_FMT , ack_p->teid);
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

/**
 * FORWARD RELOCATION COMPLETE NOTIFICATION
 */
//------------------------------------------------------------------------------
/* @brief Create a new Forward Relocation Complete Notification and send it to provided target MME. */
int
s10_mme_forward_relocation_complete_notification(
    nw_gtpv2c_stack_handle_t *stack_p,
    itti_s10_forward_relocation_complete_notification_t *notif_p)
{
  nw_gtpv2c_ulp_api_t                         ulp_req;
  nw_rc_t                                   rc;
  uint8_t                                 restart_counter = 0;

  DevAssert (stack_p );
  DevAssert (notif_p);
  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
  ulp_req.apiType = NW_GTPV2C_ULP_API_INITIAL_REQ; // todo: check if notification is an initial request.

  /** Setting the destination TEID from MME_APP. */
  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_FORWARD_RELOCATION_COMPLETE_NTF, notif_p->teid, 0, &(ulp_req.hMsg));
  ulp_req.u_api_info.initialReqInfo.peerIp     = notif_p->peer_ip;

  OAILOG_INFO(LOG_S10, "Sending FW_RELOC_COMPLETE_NOTIF TO %x. \n", ulp_req.u_api_info.initialReqInfo.peerIp);

  /** Setting the local teid twice, once here once later. */
  ulp_req.u_api_info.initialReqInfo.teidLocal  = notif_p->local_teid;  /**< Used to get the local tunnel... */
  /** Get the already existing local tunnel info. */
  hashtable_rc_t hash_rc = hashtable_ts_get(s10_mme_teid_2_gtv2c_teid_handle,
      (hash_key_t) ulp_req.u_api_info.initialReqInfo.teidLocal, (void **)(uintptr_t)&ulp_req.u_api_info.initialReqInfo.hTunnel);

  if (HASH_TABLE_OK != hash_rc) {
    OAILOG_ERROR(LOG_S10, "Could not get GTPv2-C hTunnel for local teid for handover notification %X\n", ulp_req.u_api_info.initialReqInfo.teidLocal);
    return RETURNerror;
  }

  MSC_LOG_TX_MESSAGE (MSC_S10_MME, MSC_S10_MME, NULL, 0, "0 FORWARD_RELOCATION_COMPLETE_NOTIFICATION local S10 teid " TEID_FMT,
      notif_p->local_teid);

  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
  DevAssert (NW_OK == rc);
  return RETURNok;

}

/* @brief Handle a Forward Relocation Complete Notification received from source MME. */
//------------------------------------------------------------------------------
int
s10_mme_handle_forward_relocation_complete_notification(
    nw_gtpv2c_stack_handle_t * stack_p,
    nw_gtpv2c_ulp_api_t * pUlpApi)
{
  nw_rc_t                                   rc = NW_OK;
  itti_s10_forward_relocation_complete_notification_t *notif_p;
  MessageDef                             *message_p;

  DevAssert (stack_p );
  message_p = itti_alloc_new_message (TASK_S10, S10_FORWARD_RELOCATION_COMPLETE_NOTIFICATION);
  notif_p = &message_p->ittiMsg.s10_forward_relocation_complete_notification;
  memset(notif_p, 0, sizeof(*notif_p));
  notif_p->teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg); /**< When the message is sent, this is the field, where the MME_APP sets the destination TEID.
  In this case, at reception and decoding, it is the local TEID, used to find the MME_APP ue_context. */
  notif_p->trxn = (void *)pUlpApi->u_api_info.initialReqIndInfo.hTrxn;

  MSC_LOG_RX_MESSAGE (MSC_S10_MME, MSC_S10_MME, NULL, 0, "0 FORWARD_RELOCATION_COMPLETE_NOTIFICATION local S10 teid " TEID_FMT , notif_p->teid);
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

/**
 * FORWARD RELOCATION COMPLETE ACKNOWLEDGE
 */
//------------------------------------------------------------------------------
/* @brief Create a new Forward Relocation Complete Acknowledge and send it to provided source MME. */
int
s10_mme_forward_relocation_complete_acknowledge(
    nw_gtpv2c_stack_handle_t *stack_p,
    itti_s10_forward_relocation_complete_acknowledge_t *forward_reloc_complete_ack_p)
{
  nw_gtpv2c_ulp_api_t                         ulp_ack;
  nw_rc_t                                   rc;
  uint8_t                                 restart_counter = 0;
  nw_gtpv2c_msg_parser_t                     *pMsgParser;
  nw_gtpv2c_trxn_handle_t                     trxn;

  DevAssert (forward_reloc_complete_ack_p );
  DevAssert (stack_p );
  trxn = (nw_gtpv2c_trxn_handle_t) forward_reloc_complete_ack_p->trxn;
  DevAssert (trxn );
  /*
   * Create a tunnel for the GTPv2-C stack
   */
  memset (&ulp_ack, 0, sizeof (nw_gtpv2c_ulp_api_t));

  ulp_ack.u_api_info.triggeredRspInfo.teidLocal = forward_reloc_complete_ack_p->local_teid;
  ulp_ack.apiType = NW_GTPV2C_ULP_API_TRIGGERED_RSP; /**< Sending Side. */
  ulp_ack.u_api_info.triggeredRspInfo.hUlpTunnel = 0;
  ulp_ack.u_api_info.triggeredRspInfo.hTunnel    = 0;
  ulp_ack.u_api_info.triggeredRspInfo.hTrxn = trxn;

  hashtable_rc_t hash_rc = hashtable_ts_get(s10_mme_teid_2_gtv2c_teid_handle,
      (hash_key_t) ulp_ack.u_api_info.triggeredRspInfo.teidLocal, (void **)(uintptr_t)&ulp_ack.u_api_info.triggeredRspInfo.hTunnel);

  if (HASH_TABLE_OK != hash_rc) {
    OAILOG_WARNING (LOG_S10, "Could not get GTPv2-C hTunnel for local TEID %X on S10 MME interface. \n", ulp_ack.u_api_info.triggeredRspInfo.teidLocal);
    return RETURNerror;
  }

  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_FORWARD_RELOCATION_COMPLETE_ACK, 0, 0, &(ulp_ack.hMsg));
  DevAssert (NW_OK == rc);
  /*
   * Set the remote TEID.
   */
  rc = nwGtpv2cMsgSetTeid (ulp_ack.hMsg, forward_reloc_complete_ack_p->teid);
  DevAssert (NW_OK == rc);
  /*
   * Putting the information Elements
   */
  gtpv2c_cause_ie_set(&(ulp_ack.hMsg), &forward_reloc_complete_ack_p->cause);

  /** Send the message. */
  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_ack);
  DevAssert (NW_OK == rc);
  return RETURNok;

}

//------------------------------------------------------------------------------
int
s10_mme_handle_forward_relocation_complete_acknowledge(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  nw_rc_t                                   rc = NW_OK;
  uint8_t                                 offendingIeType,
                                          offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_s10_forward_relocation_complete_acknowledge_t *ack_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                     *pMsgParser;

  DevAssert (stack_p );
  message_p = itti_alloc_new_message (TASK_S10, S10_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGE);
  ack_p = &message_p->ittiMsg.s10_forward_relocation_complete_acknowledge;
  memset(ack_p, 0, sizeof(*ack_p));

  ack_p->teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);

  /*
   * Create a new message parser
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_FORWARD_RELOCATION_COMPLETE_ACK, s10_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);
  /*
   * Cause IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_CAUSE, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
      gtpv2c_cause_ie_get, &ack_p->cause);
  DevAssert (NW_OK == rc);

  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "0 FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGE local S10 teid " TEID_FMT " ", ack_p->teid);
    /*
     * TODO: handle this case
     */
    itti_free (ITTI_MSG_ORIGIN_ID (message_p), message_p);
    message_p = NULL;
    rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
    DevAssert (NW_OK == rc);
    rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
    DevAssert (NW_OK == rc);
    return RETURNerror;
  }

  rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
  DevAssert (NW_OK == rc);
  rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
  DevAssert (NW_OK == rc);

  MSC_LOG_RX_MESSAGE (MSC_S10_MME, MSC_S10_MME, NULL, 0, "0 FORWARD_RELOCATION_COMPLETE_ACKOWLEDGE local S10 teid " TEID_FMT , ack_p->teid);
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
s10_mme_context_request (
    nw_gtpv2c_stack_handle_t *stack_p,
    itti_s10_context_request_t *req_p)
{
  nw_gtpv2c_ulp_api_t                         ulp_req;
  nw_rc_t                                   rc;

  DevAssert (stack_p );
  DevAssert (req_p );
  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
  ulp_req.apiType = NW_GTPV2C_ULP_API_INITIAL_REQ;
  /**< Initial Request will create a local S10 tunnel in the ULP handler, if none existing.
   * Below the ULP tunnel will be inserted into the hash map with local S10 teid as key.
   */

  /*
   * Prepare a new Context Request msg
   */
  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_CONTEXT_REQ, req_p->teid, 0, &(ulp_req.hMsg));
  ulp_req.u_api_info.initialReqInfo.peerIp     = req_p->peer_ip;
  ulp_req.u_api_info.initialReqInfo.teidLocal  = req_p->s10_target_mme_teid.teid;
  ulp_req.u_api_info.initialReqInfo.hUlpTunnel = 0;
  ulp_req.u_api_info.initialReqInfo.hTunnel    = 0;

  /**
   * Putting the information Elements.
   */
  /** todo: later add IMSI if source can authenticate UEs before S10. */
  //  gtpv2c_imsi_ie_set (&(ulp_req.hMsg), &req_p->imsi);

  /** GUTI. */
  s10_guti_ie_set (&(ulp_req.hMsg), &req_p->old_guti);

  /** RAT-Type. */
  s10_rat_type_ie_set (&(ulp_req.hMsg), &req_p->rat_type);

  /**
   * Source MME F-TEID for Control Plane (MME S10)
   */
  rc = nwGtpv2cMsgAddIeFteid ((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO,
      S10_MME_GTP_C,
      req_p->s10_target_mme_teid.teid,
      req_p->s10_target_mme_teid.ipv4 ? &req_p->s10_target_mme_teid.ipv4_address : 0,
          req_p->s10_target_mme_teid.ipv6 ? &req_p->s10_target_mme_teid.ipv6_address : NULL);
  /**
   * Serving Network.
   */
  s10_serving_network_ie_set (&(ulp_req.hMsg), &req_p->serving_network);

  /**
   * Complete Request Message (TAU or attach).
   */
  rc = nwGtpv2cMsgAddIeCompleteRequestMessage((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO,
      (uint8_t*)req_p->complete_request_message.request_value->data,
      blength(req_p->complete_request_message.request_value),
      req_p->complete_request_message.request_type);
  /** Destroy the container. */
  bdestroy(req_p->complete_request_message.request_value);
  DevAssert( NW_OK == rc );

  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req); /**< Creates an ULP tunnel if none existing. */
  DevAssert (NW_OK == rc);
  MSC_LOG_TX_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, " CONTEXT_REQUEST local S10 teid " TEID_FMT,
      req_p->s10_target_mme_teid.teid);

  /**
   * Just an int is stored. When the value is removed from the tunnel pool, nothing will be deallocated.
   * Free method does nothing!
   */
  hashtable_rc_t hash_rc = hashtable_ts_insert(s10_mme_teid_2_gtv2c_teid_handle,
      (hash_key_t) req_p->s10_target_mme_teid.teid,
      (void *)ulp_req.u_api_info.initialReqInfo.hTunnel);
  if (HASH_TABLE_OK == hash_rc) {
    return RETURNok;
  } else {
    OAILOG_WARNING (LOG_S10, "Could not save GTPv2-C hTunnel %p for local teid %X\n", (void*)ulp_req.u_api_info.initialReqInfo.hTunnel, ulp_req.u_api_info.initialReqInfo.teidLocal);
    return RETURNerror;
  }
}

//------------------------------------------------------------------------------
int
s10_mme_handle_context_request(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  nw_rc_t                                   rc = NW_OK;
  uint8_t                                 offendingIeType,
                                          offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_s10_context_request_t             *req_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                     *pMsgParser;

  OAILOG_FUNC_IN (LOG_S10);

  DevAssert (stack_p );
  /** Allocating the Signal once at the sender (MME_APP --> S10) and once at the receiver (S10-->MME_APP). */
  message_p = itti_alloc_new_message (TASK_S10, S10_CONTEXT_REQUEST);
  req_p = &message_p->ittiMsg.s10_context_request;
  memset(req_p, 0, sizeof(*req_p));

  req_p->teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);
  req_p->trxn = (void *)pUlpApi->u_api_info.initialReqIndInfo.hTrxn;

  /** Check the destination TEID is 0. */
  if(req_p->teid != (teid_t)0){
    OAILOG_WARNING (LOG_S10, "Destination TEID of S10 Context Request is not 0, insted " TEID_FMT ". Ignoring s10 context requetst. \n", req_p->teid);
    return RETURNerror;
  }
  /*
   * Create a new message parser for the S10 CONTEXT REQUEST.
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_CONTEXT_REQ, s10_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);

  /*
   * Sender (Target MME) FTEID for CP IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_FTEID, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
      gtpv2c_fteid_ie_get, &req_p->s10_target_mme_teid);
  DevAssert (NW_OK == rc);

  /*
   * IMSI IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_IMSI, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      gtpv2c_imsi_ie_get, &req_p->imsi);
  DevAssert (NW_OK == rc);

  /*
   * GUTI IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_GUTI, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      s10_guti_ie_get, &req_p->old_guti);
  DevAssert (NW_OK == rc);

  /* RAT-Type. */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_RAT_TYPE, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      s10_rat_type_ie_get, &req_p->rat_type);
  DevAssert (NW_OK == rc);

  /**
   * Serving Network.
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_SERVING_NETWORK, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      s10_serving_network_ie_get, &req_p->serving_network);
  DevAssert (NW_OK == rc);

  /**
   * Get the complete request message.
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_COMPLETE_REQUEST_MESSAGE, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      s10_complete_request_message_ie_get, &req_p->complete_request_message);
  DevAssert (NW_OK == rc);

  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "CONTEXT_REQUEST local S10 teid " TEID_FMT " ", req_p->teid);
    /*
     * TODO: handle this case
     */
    itti_free (ITTI_MSG_ORIGIN_ID (message_p), message_p);
    message_p = NULL;
    rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
    DevAssert (NW_OK == rc);
    rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
    DevAssert (NW_OK == rc);
    return RETURNerror;
  }

  rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
  DevAssert (NW_OK == rc);
  rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
  DevAssert (NW_OK == rc);

  MSC_LOG_RX_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "CONTEXT_REQUEST local S10 teid " TEID_FMT, req_p->teid);
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
s10_mme_context_response (
    nw_gtpv2c_stack_handle_t *stack_p,
    itti_s10_context_response_t *rsp_p)
{
  nw_gtpv2c_ulp_api_t                         ulp_req;
  nw_rc_t                                   rc;
  uint8_t                                 restart_counter = 0;
  nw_gtpv2c_trxn_handle_t                     trxn;
  gtpv2c_cause_t                             cause;

  DevAssert (rsp_p );
  DevAssert (stack_p );
  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
  memset (&cause, 0, sizeof (gtpv2c_cause_t));

  trxn = (nw_gtpv2c_trxn_handle_t) rsp_p->trxn;
  DevAssert (trxn);


  /**
   * Prepare a context response to send to target MME.
   */
  ulp_req.apiType = NW_GTPV2C_ULP_API_TRIGGERED_RSP;
  ulp_req.u_api_info.triggeredRspInfo.hTrxn = trxn;
  ulp_req.u_api_info.triggeredRspInfo.teidLocal = rsp_p->s10_source_mme_teid.teid;
  ulp_req.u_api_info.triggeredRspInfo.hUlpTunnel = 0;
  ulp_req.u_api_info.triggeredRspInfo.hTunnel    = 0;

  /**
   * Create a tunnel for the GTPv2-C stack if its a positive response.
   */
  if(rsp_p->cause.cause_value == REQUEST_ACCEPTED){
    ulp_req.apiType |= NW_GTPV2C_ULP_API_FLAG_CREATE_LOCAL_TUNNEL;
  }else{
    OAILOG_WARNING (LOG_S10, "The cause is not REQUEST_ACCEPTED but %d for S10_CONTEXT_RESPONSE. "
        "Not creating a local S10 Tunnel @ triggered response! \n", rsp_p->cause);
  }

  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_CONTEXT_RSP, 0, 0, &(ulp_req.hMsg));
  DevAssert (NW_OK == rc);
  /*
   * Set the destination TEID
   */
  rc = nwGtpv2cMsgSetTeid (ulp_req.hMsg, rsp_p->teid);
  DevAssert (NW_OK == rc);

  /** Add the S10 Cause : Not setting offending IE type now. */
  rc = nwGtpv2cMsgAddIeCause((ulp_req.hMsg), 0, rsp_p->cause.cause_value, 0, 0, 0);
  DevAssert( NW_OK == rc );

  if(rsp_p->cause.cause_value == REQUEST_ACCEPTED){
    /** Add the S10 source-MME FTEID. */
    rc = nwGtpv2cMsgAddIeFteid ((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO, S10_MME_GTP_C,
        rsp_p->s10_source_mme_teid.teid, /**< FTEID of the TARGET_MME. */
        rsp_p->s10_source_mme_teid.ipv4 ? &rsp_p->s10_source_mme_teid.ipv4_address : 0,
            rsp_p->s10_source_mme_teid.ipv6 ? &rsp_p->s10_source_mme_teid.ipv6_address : NULL);

    /** Add the S10 source-MME SAE-GW FTEID. */
    rc = nwGtpv2cMsgAddIeFteid ((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ONE, S11_SGW_GTP_C,
        rsp_p->s11_sgw_teid.teid, /**< FTEID of the TARGET_MME. */
        rsp_p->s11_sgw_teid.ipv4 ? &rsp_p->s11_sgw_teid.ipv4_address : 0,
            rsp_p->s11_sgw_teid.ipv6 ? &rsp_p->s11_sgw_teid.ipv6_address : NULL);
    DevAssert (NW_OK == rc);

    /** IMSI. */
    gtpv2c_imsi_ie_set (&(ulp_req.hMsg), &rsp_p->imsi);

    /** PDN Connection IE. */
    s10_pdn_connection_ie_set (&(ulp_req.hMsg), &rsp_p->pdn_connections);

    /** Set the MM EPS UE Context. */
    s10_ue_mm_eps_context_ie_set(&(ulp_req.hMsg), &rsp_p->ue_eps_mm_context);
  }
  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
  DevAssert (NW_OK == rc);

  /**
   * Create a tunnel for the GTPv2-C stack if its a positive response.
   */
  if(rsp_p->cause.cause_value == REQUEST_ACCEPTED){
    hashtable_rc_t hash_rc = hashtable_ts_insert(s10_mme_teid_2_gtv2c_teid_handle, /**< Directly register the created tunnel. */
        (hash_key_t) ulp_req.u_api_info.createLocalTunnelInfo.teidLocal,
        (void *)ulp_req.u_api_info.createLocalTunnelInfo.hTunnel); /**< Just store the value as int (no free method) after allocating the S10 GTPv2c Tunnel from the tunnel pool. */
    hash_rc = hashtable_ts_get(s10_mme_teid_2_gtv2c_teid_handle,
        (hash_key_t) ulp_req.u_api_info.createLocalTunnelInfo.teidLocal, (void **)(uintptr_t)&ulp_req.u_api_info.createLocalTunnelInfo.hTunnel);
    DevAssert(hash_rc == HASH_TABLE_OK);
  }else{
    OAILOG_WARNING (LOG_S10, "The cause is not REQUEST_ACCEPTED but %d for S10_CONTEXT_RESPONSE. "
        "Not creating a local S10 Tunnel. \n", rsp_p->cause);
  }


  return RETURNok;
}

//------------------------------------------------------------------------------
int
s10_mme_handle_context_response(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  nw_rc_t                                   rc = NW_OK;
  uint8_t                                 offendingIeType,
                                          offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_s10_context_response_t            *resp_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                     *pMsgParser;

  DevAssert (stack_p );
  message_p = itti_alloc_new_message (TASK_S10, S10_CONTEXT_RESPONSE);
  resp_p = &message_p->ittiMsg.s10_context_response;
  memset(resp_p, 0, sizeof(*resp_p));

  /** Set the destination TEID. */
  resp_p->teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);

  /** Set the transaction for the triggered acknowledgement. */
  resp_p->trxn = (void *)pUlpApi->u_api_info.triggeredRspIndInfo.hUlpTrxn;

  /** Create a new message parser.     */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_CONTEXT_RSP, s10_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);

  /**
   * Cause IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_CAUSE, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
      gtpv2c_cause_ie_get, &resp_p->cause);
  DevAssert (NW_OK == rc);

  /**
   * Sender FTEID for CP IE
   * todo: TEID not saved in the tunnel endpoint. Why not saving in the S10 tunnel endpoints at all ?
   * It will be saved in the mme_ue_context in the MME_APP layer.
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_FTEID, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      gtpv2c_fteid_ie_get, &resp_p->s10_source_mme_teid);
  DevAssert (NW_OK == rc);

  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_FTEID, NW_GTPV2C_IE_INSTANCE_ONE, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      gtpv2c_fteid_ie_get, &resp_p->s11_sgw_teid);
  DevAssert (NW_OK == rc);

  /*
   * IMSI IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_IMSI, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      gtpv2c_imsi_ie_get, &resp_p->imsi);
  DevAssert (NW_OK == rc);

  /*
   * PDN Connection IE : Several can exist
   * todo: multiple pdn connection IEs can exist with instance 0.
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_PDN_CONNECTION, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      s10_pdn_connection_ie_get, &resp_p->pdn_connections);
  DevAssert (NW_OK == rc);

   /*
    * MME UE MM Context.
    */
   rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_MM_EPS_CONTEXT, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
        s10_mm_ue_context_ie_get, &resp_p->ue_eps_mm_context);
   DevAssert (NW_OK == rc);

  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "CONTEXT_RESPONSE local S10 teid " TEID_FMT " ", resp_p->teid);
    /*
     * TODO: handle this case
     */
    itti_free (ITTI_MSG_ORIGIN_ID (message_p), message_p);
    message_p = NULL;
    rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
    DevAssert (NW_OK == rc);
    rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
    DevAssert (NW_OK == rc);
    return RETURNerror;
  }

  rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
  DevAssert (NW_OK == rc);
  rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
  DevAssert (NW_OK == rc);

  MSC_LOG_RX_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "0 CONTEXT_RESPONSE local S10 teid " TEID_FMT " num pdn connections %u", resp_p->teid,
    resp_p->pdn_connections.num_pdn_connections);
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
s10_mme_context_acknowledge (
    nw_gtpv2c_stack_handle_t *stack_p,
    itti_s10_context_acknowledge_t *ack_p)
{
  nw_gtpv2c_ulp_api_t                         ulp_ack;
  nw_rc_t                                   rc;
  uint8_t                                 restart_counter = 0;

  /**
   * Responses do not have replies except when a "Context Acknowledge" is required as a reply to "Context Response" message as specified in relevant Stage 2 procedures.
   * Context Acknowledge is always triggered message and does not have a reply.
   * NOTE 2: The "Context Acknowledge" message is sent only if the "Context Response" message is received with the acceptance cause.
   */
  DevAssert (stack_p );
  DevAssert (ack_p );
  memset (&ulp_ack, 0, sizeof (nw_gtpv2c_ulp_api_t));
  ulp_ack.apiType = NW_GTPV2C_ULP_API_TRIGGERED_ACK;
  ulp_ack.u_api_info.triggeredAckInfo.peerIp = ack_p->peer_ip;
  ulp_ack.u_api_info.triggeredAckInfo.peerPort = ack_p->peer_port;

  /*
   * Prepare a context ack to send to target MME.
   */
  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_CONTEXT_ACK, 0, ack_p->trxnId, &(ulp_ack.hMsg));
  DevAssert (NW_OK == rc);

  /*
   * Set the destination TEID
   */
  rc = nwGtpv2cMsgSetTeid (ulp_ack.hMsg, ack_p->teid);
  DevAssert (NW_OK == rc);

  /** Add the S10 Cause : Not setting offending IE type now. */
  rc = nwGtpv2cMsgAddIeCause((ulp_ack.hMsg), 0, ack_p->cause.cause_value, 0, 0, 0);
  DevAssert( NW_OK == rc );

  /**
   * No timer will be started, just the existing transaction will be further used.
   * The seq_no and peer details will be pulled from the transaction.
   * The S10 Tunnel will not be removed. Only with implicit detach.
   */
  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_ack);
  DevAssert (NW_OK == rc);
  MSC_LOG_TX_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "CONTEXT_ACKNOWLEDGE with cause %d ", ack_p->cause);

  return RETURNok;
}

//------------------------------------------------------------------------------
// todo: evaluate later the error cause in the removal!! --> eventually if something goes wrong here.. check the reason!
int
s10_mme_remove_ue_tunnel (
    nw_gtpv2c_stack_handle_t *stack_p,
    itti_s10_remove_ue_tunnel_t *remove_ue_tunnel_p)
{
  nw_rc_t                                   rc = NW_OK;
  hashtable_rc_t                          hash_rc = HASH_TABLE_OK;
  DevAssert (stack_p );
  DevAssert (remove_ue_tunnel_p );
  MSC_LOG_RX_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "Removing S10 UE Tunnels for local S10 teid " TEID_FMT " ",
      remove_ue_tunnel_p->local_teid);
  // delete local s10 tunnel
  nw_gtpv2c_ulp_api_t                         ulp_req;
  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));

  hash_rc = hashtable_ts_get(s10_mme_teid_2_gtv2c_teid_handle,
      (hash_key_t) remove_ue_tunnel_p->local_teid,
      (void **)(uintptr_t)&ulp_req.u_api_info.deleteLocalTunnelInfo.hTunnel);
  if (HASH_TABLE_OK != hash_rc) {

//    NwGtpv2cTunnelT                        *pLocalTunnel = NULL,
//                                             keyTunnel = {0};
//    /** Check if a tunnel already exists depending on the flag. */
//    keyTunnel.teid = remove_ue_tunnel_p->remote_teid;
//    keyTunnel.ipv4AddrRemote = remove_ue_tunnel_p->peer_ip;
//    pLocalTunnel = RB_FIND (NwGtpv2cTunnelMap, &(thiz->tunnelMap), &keyTunnel);


    OAILOG_ERROR (LOG_S10, "Could not get GTPv2-C hTunnel for local teid %X\n", remove_ue_tunnel_p->local_teid);
    MSC_LOG_EVENT (MSC_S10_MME, "Failed to deleted local teid " TEID_FMT "", remove_ue_tunnel_p->local_teid);
    // todo: error in error handling.. asserting?! extreme error handling?
    // Currently ignoring and continue to remove the remains of the tunnel.

//    ulp_req.apiType = NW_GTPV2C_ULP_FIND_LOCAL_TUNNEL;
//    ulp_req.u_api_info.findLocalTunnelInfo.teidLocal = remove_ue_tunnel_p->local_teid;
//    ulp_req.u_api_info.findLocalTunnelInfo.peerIp = remove_ue_tunnel_p->peer_ip;
//    rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
//    DevAssert (NW_OK == rc);
//    if(ulp_req.u_api_info.findLocalTunnelInfo.hTunnel){
//      OAILOG_ERROR (LOG_S10, "Could FIND A GTPv2-C hTunnel for local teid %X @ DELETION \n", remove_ue_tunnel_p->local_teid);
//    }else{
//      OAILOG_ERROR (LOG_S10, "Could NOT FIND A GTPv2-C hTunnel for local teid %X @ DELETION \n", remove_ue_tunnel_p->local_teid);
//    }
    return RETURNerror;

  } else{

    ulp_req.apiType = NW_GTPV2C_ULP_DELETE_LOCAL_TUNNEL;

    rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
    DevAssert (NW_OK == rc);
    OAILOG_INFO(LOG_S10, "DELETED local S10 teid (TEID FOUND IN HASH_MAP)" TEID_FMT " \n", remove_ue_tunnel_p->local_teid);

//    ulp_req.apiType = NW_GTPV2C_ULP_FIND_LOCAL_TUNNEL;
//    ulp_req.u_api_info.findLocalTunnelInfo.teidLocal = remove_ue_tunnel_p->local_teid;
//    ulp_req.u_api_info.findLocalTunnelInfo.peerIp = remove_ue_tunnel_p->peer_ip;
//    rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
//    DevAssert (NW_OK == rc);
//    if(ulp_req.u_api_info.findLocalTunnelInfo.hTunnel){
//      OAILOG_WARNING (LOG_S10, "Could FIND A GTPv2-C hTunnel for local teid %X @ DELETION (2) \n", remove_ue_tunnel_p->local_teid);
//    }else{
//      OAILOG_WARNING(LOG_S10, "Could NOT FIND A GTPv2-C hTunnel for local teid %X @ DELETION  (2) \n", remove_ue_tunnel_p->local_teid);
//    }

    /**
     * hash_free_int_func is set as the freeing function.
     * The value is removed from the map. But the value itself (int) is not freed.
     * The Tunnels are not deallocated but just set back to the Tunnel pool.
     */
    hash_rc = hashtable_ts_free(s10_mme_teid_2_gtv2c_teid_handle, (hash_key_t) remove_ue_tunnel_p->local_teid);
    DevAssert (HASH_TABLE_OK == hash_rc);

  }
  OAILOG_DEBUG(LOG_S10, "Successfully removed S10 Tunnel local teid %X\n", remove_ue_tunnel_p->local_teid);
  return RETURNok;
}

//------------------------------------------------------------------------------
int
s10_mme_handle_context_acknowledgement(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  nw_rc_t                                   rc = NW_OK;
  uint8_t                                 offendingIeType,
                                          offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_s10_context_acknowledge_t         *ack_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                     *pMsgParser;

  DevAssert (stack_p );
  message_p = itti_alloc_new_message (TASK_S10, S10_CONTEXT_ACKNOWLEDGE);
  ack_p = &message_p->ittiMsg.s10_context_acknowledge;
  memset(ack_p, 0, sizeof(*ack_p));

  ack_p->teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);

  /*
   * Create a new message parser
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_CONTEXT_RSP, s10_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);
  /*
   * Cause IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_CAUSE, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
      gtpv2c_cause_ie_get, &ack_p->cause);
  DevAssert (NW_OK == rc);

  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "0 CONTEXT_ACKNOWLEDGE local S10 teid " TEID_FMT " ", ack_p->teid);
    /*
     * TODO: handle this case
     */
    itti_free (ITTI_MSG_ORIGIN_ID (message_p), message_p);
    message_p = NULL;
    rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
    DevAssert (NW_OK == rc);
    rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
    DevAssert (NW_OK == rc);
    return RETURNerror;
  }

  rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
  DevAssert (NW_OK == rc);
  rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
  DevAssert (NW_OK == rc);

  MSC_LOG_RX_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "0 CONTEXT_ACKNOWLEDGE local S10 teid " TEID_FMT " with cause %d ", ack_p->teid, ack_p->cause);
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------+
int
s10_mme_relocation_cancel_request(
  nw_gtpv2c_stack_handle_t * stack_p,
  itti_s10_relocation_cancel_request_t * req_p)
{
  nw_gtpv2c_ulp_api_t                         ulp_req;
  nw_rc_t                                   rc;
  uint8_t                                 restart_counter = 0;

  OAILOG_FUNC_IN (LOG_S10);

  DevAssert (stack_p );
  DevAssert (req_p);
  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
  ulp_req.apiType = NW_GTPV2C_ULP_API_INITIAL_REQ;
  /** Setting the local teid twice, once here once later. */
  ulp_req.u_api_info.initialReqInfo.teidLocal = req_p->local_teid;  /**< Used to get the local tunnel... */

  hashtable_rc_t hash_rc = hashtable_ts_get(s10_mme_teid_2_gtv2c_teid_handle,
      (hash_key_t) ulp_req.u_api_info.initialReqInfo.teidLocal, (void **)(uintptr_t)&ulp_req.u_api_info.initialReqInfo.hTunnel);
  if (HASH_TABLE_OK != hash_rc) {
    OAILOG_WARNING (LOG_S10, "Could not get GTPv2-C hTunnel for local TEID %X on S10 MME interface. \n", ulp_req.u_api_info.initialReqInfo.teidLocal);
    return RETURNerror;
  }

  /** Setting the destination TEID from MME_APP. */
  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_RELOCATION_CANCEL_REQ, req_p->teid, 0, &(ulp_req.hMsg));
  ulp_req.u_api_info.initialReqInfo.peerIp     = req_p->peer_ip;

  /*
   * Set the destination TEID
   */
  rc = nwGtpv2cMsgSetTeid (ulp_req.hMsg, req_p->teid);
  DevAssert (NW_OK == rc);

  /** IMSI. */
  gtpv2c_imsi_ie_set (&(ulp_req.hMsg), &req_p->imsi);

  OAILOG_WARNING (LOG_S10, "Sending RELOCATION_CANCEL_REQUEST TO %x and removing local S10 Tunnel endpoint. Transactions will timeout and should be ignored. \n", ulp_req.u_api_info.initialReqInfo.peerIp);
  /** We assume that we already send the FW_RELOCATION_REQ. */
  MSC_LOG_TX_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "0 RELOCATION_CANCEL_REQUEST local S10 teid " TEID_FMT,
      req_p->local_teid);

  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
  DevAssert (NW_OK == rc);

  /**
   * Remove the S10 Tunnel Endpoint to the peer.
   * Ignore received FW_RELOC_RSP and RELOC_CANCEL_RSP messages.
   */
  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
  ulp_req.apiType = NW_GTPV2C_ULP_DELETE_LOCAL_TUNNEL;

  hash_rc = hashtable_ts_get(s10_mme_teid_2_gtv2c_teid_handle,
       (hash_key_t) req_p->local_teid,
       (void **)(uintptr_t)&ulp_req.u_api_info.deleteLocalTunnelInfo.hTunnel);

  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
  DevAssert (NW_OK == rc);
  OAILOG_INFO(LOG_S10, "DELETED local S10 teid (TEID FOUND IN HASH_MAP)" TEID_FMT " \n", req_p->local_teid);

//  ulp_req.apiType = NW_GTPV2C_ULP_FIND_LOCAL_TUNNEL;
//  ulp_req.u_api_info.findLocalTunnelInfo.teidLocal = req_p->local_teid;
//  ulp_req.u_api_info.findLocalTunnelInfo.peerIp = req_p->peer_ip;
//  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
//  DevAssert (NW_OK == rc);
//  if(ulp_req.u_api_info.findLocalTunnelInfo.hTunnel){
//    OAILOG_WARNING (LOG_S10, "Could FIND A GTPv2-C hTunnel for local teid %X @ DELETION (2) \n", req_p->local_teid);
//  }else{
//    OAILOG_WARNING(LOG_S10, "Could NOT FIND A GTPv2-C hTunnel for local teid %X @ DELETION  (2) \n", req_p->local_teid);
//  }

  /**
   * hash_free_int_func is set as the freeing function.
   * The value is removed from the map. But the value itself (int) is not freed.
   * The Tunnels are not deallocated but just set back to the Tunnel pool.
   */
  hash_rc = hashtable_ts_free(s10_mme_teid_2_gtv2c_teid_handle, (hash_key_t) req_p->local_teid);
  DevAssert (HASH_TABLE_OK == hash_rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
int
s10_mme_handle_relocation_cancel_request(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  nw_rc_t                                 rc = NW_OK;
  uint8_t                                 offendingIeType,
                                          offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_s10_relocation_cancel_request_t   *req_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                 *pMsgParser;

  DevAssert (stack_p );
  message_p = itti_alloc_new_message (TASK_S10, S10_RELOCATION_CANCEL_REQUEST);
  req_p = &message_p->ittiMsg.s10_context_acknowledge;
  memset(req_p, 0, sizeof(*req_p));

  req_p->teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);
  req_p->trxn = (void *)pUlpApi->u_api_info.initialReqIndInfo.hTrxn;

  /*
   * Create a new message parser
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_RELOCATION_CANCEL_REQ, s10_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);
  /*
   * IMSI IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_IMSI, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
      gtpv2c_imsi_ie_get, &req_p->imsi);
  DevAssert (NW_OK == rc);

  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  /*
   * Add the peer-IP just in case.
   */
  req_p->peer_ip.s_addr = pUlpApi->u_api_info.initialReqIndInfo.peerIp.s_addr;
  if (rc != NW_OK) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "0 RELOCATION_CANCEL_REQUEST local S10 teid " TEID_FMT " ", req_p->teid);
    /*
     * TODO: handle this case
     */
    itti_free (ITTI_MSG_ORIGIN_ID (message_p), message_p);
    message_p = NULL;
    rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
    DevAssert (NW_OK == rc);
    rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
    DevAssert (NW_OK == rc);
    return RETURNerror;
  }

  rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
  DevAssert (NW_OK == rc);
  rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
  DevAssert (NW_OK == rc);

  /** A tunnel endpoint might or might not be created for that. */
  MSC_LOG_RX_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "0 RELOCATION_CANCEL_REQUEST local S10 teid " TEID_FMT, req_p->teid);
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
s10_mme_relocation_cancel_response(
    nw_gtpv2c_stack_handle_t *stack_p,
    itti_s10_relocation_cancel_response_t * relocation_cancel_resp_p)
{
  nw_gtpv2c_ulp_api_t                     ulp_rsp;
  nw_rc_t                                 rc;
  uint8_t                                 restart_counter = 0;
  nw_gtpv2c_msg_parser_t                 *pMsgParser;
  nw_gtpv2c_trxn_handle_t                 trxn;
  gtpv2c_cause_t                          cause;

  OAILOG_FUNC_IN (LOG_S10);

  DevAssert (relocation_cancel_resp_p);
  DevAssert (stack_p );
  trxn = (nw_gtpv2c_trxn_handle_t) relocation_cancel_resp_p->trxn;
  DevAssert (trxn );

  /**
   * Prepare a context response to send to target MME.
   */
  memset (&ulp_rsp, 0, sizeof (nw_gtpv2c_ulp_api_t));
  memset (&cause, 0, sizeof (gtpv2c_cause_t));
  ulp_rsp.apiType = NW_GTPV2C_ULP_API_TRIGGERED_RSP;
  ulp_rsp.u_api_info.triggeredRspInfo.hTrxn = trxn;
  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_RELOCATION_CANCEL_RSP, 0, 0, &(ulp_rsp.hMsg));
  DevAssert (NW_OK == rc);
  /*
   * Set the destination TEID
   */
  rc = nwGtpv2cMsgSetTeid (ulp_rsp.hMsg, relocation_cancel_resp_p->teid);
  DevAssert (NW_OK == rc);

  /** Add the S10 Cause : Not setting offending IE type now. */
  rc = nwGtpv2cMsgAddIeCause((ulp_rsp.hMsg), 0, relocation_cancel_resp_p->cause.cause_value, 0, 0, 0);
  DevAssert( NW_OK == rc );

  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_rsp);
  DevAssert (NW_OK == rc);

  /** Depending if there is a GTPv2c Tunnel or not, remove the tunnel. */
  memset (&ulp_rsp, 0, sizeof (nw_gtpv2c_ulp_api_t));
  ulp_rsp.apiType = NW_GTPV2C_ULP_DELETE_LOCAL_TUNNEL;

  hashtable_rc_t hash_rc = hashtable_ts_get(s10_mme_teid_2_gtv2c_teid_handle,
      (hash_key_t) relocation_cancel_resp_p->local_teid, (void **)(uintptr_t)&ulp_rsp.u_api_info.deleteLocalTunnelInfo.hTunnel);
  if (HASH_TABLE_OK != hash_rc) {
    OAILOG_WARNING (LOG_S10, "Could not get GTPv2-C hTunnel for local TEID %X on S10 MME interface. \n", relocation_cancel_resp_p->local_teid);
    return RETURNok;
  }

  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_rsp);
  DevAssert (NW_OK == rc);
  OAILOG_INFO(LOG_S10, "DELETED local S10 teid (TEID FOUND IN HASH_MAP)" TEID_FMT " \n", relocation_cancel_resp_p->local_teid);

  /**
   * hash_free_int_func is set as the freeing function.
   * The value is removed from the map. But the value itself (int) is not freed.
   * The Tunnels are not deallocated but just set back to the Tunnel pool.
   */
  hash_rc = hashtable_ts_free(s10_mme_teid_2_gtv2c_teid_handle, (hash_key_t) relocation_cancel_resp_p->local_teid);
  DevAssert (HASH_TABLE_OK == hash_rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
int
s10_mme_handle_relocation_cancel_response(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  nw_rc_t                                 rc = NW_OK;
  uint8_t                                 offendingIeType,
                                          offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_s10_relocation_cancel_response_t  *rsp_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                 *pMsgParser;

  DevAssert (stack_p );
  message_p = itti_alloc_new_message (TASK_S10, S10_RELOCATION_CANCEL_RESPONSE);
  rsp_p = &message_p->ittiMsg.s10_relocation_cancel_response;
  memset(rsp_p, 0, sizeof(*rsp_p));

  rsp_p->teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);

  /*
   * Create a new message parser
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_RELOCATION_CANCEL_RSP, s10_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);
  /*
   * Cause IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_CAUSE, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
      gtpv2c_cause_ie_get, &rsp_p->cause);
  DevAssert (NW_OK == rc);

  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "0 RELOCATION_CANCEL_RESPONSE local S10 teid " TEID_FMT " ", rsp_p->teid);
    /*
     * TODO: handle this case
     */
    itti_free (ITTI_MSG_ORIGIN_ID (message_p), message_p);
    message_p = NULL;
    rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
    DevAssert (NW_OK == rc);
    rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
    DevAssert (NW_OK == rc);
    return RETURNerror;
  }

  rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
  DevAssert (NW_OK == rc);
  rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
  DevAssert (NW_OK == rc);

  MSC_LOG_RX_MESSAGE (MSC_S10_MME, MSC_SGW, NULL, 0, "0 RELOCATION_CANCEL_RESPONSE local S10 teid " TEID_FMT , rsp_p->teid);
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
s10_mme_handle_ulp_error_indicatior(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  /** Get the failed transaction. */
  /** Check the message type. */

  nw_gtpv2c_msg_type_t msgType = pUlpApi->u_api_info.rspFailureInfo.msgType;
  MessageDef * message_p = NULL;
  switch(msgType){
  case NW_GTP_CONTEXT_REQ:
  {
    itti_s10_context_response_t            *resp_p;
    /** Respond with an S10 Context Response Failure. */
    message_p = itti_alloc_new_message (TASK_S10, S10_CONTEXT_RESPONSE);
    resp_p = &message_p->ittiMsg.s10_context_response;
    memset(resp_p, 0, sizeof(*resp_p));
    /** Set the destination TEID (our TEID). */
    resp_p->teid = pUlpApi->u_api_info.rspFailureInfo.teidLocal;
    /** Set the transaction for the triggered acknowledgement. */
    resp_p->trxn = (void *)pUlpApi->u_api_info.rspFailureInfo.hUlpTrxn;
    /** Set the cause. */
    resp_p->cause.cause_value = SYSTEM_FAILURE; /**< Would mean that this message either did not come at all or could not be dealt with properly. */
  }
    break;
  case NW_GTP_CONTEXT_RSP:
  {
    itti_s10_context_acknowledge_t            *ack_p;
    /**
     * If CTX_RSP is sent but no context acknowledge is received
     */
    message_p = itti_alloc_new_message (TASK_S10, S10_CONTEXT_ACKNOWLEDGE);
    ack_p = &message_p->ittiMsg.s10_context_acknowledge;
    memset(ack_p, 0, sizeof(*ack_p));
    /** Set the destination TEID (our TEID). */
    ack_p->teid = pUlpApi->u_api_info.rspFailureInfo.teidLocal;
    /** Set the transaction for the triggered acknowledgement. */
    ack_p->trxnId = (uint32_t)pUlpApi->u_api_info.rspFailureInfo.hUlpTrxn;
    /** Set the cause. */
    ack_p->cause.cause_value = SYSTEM_FAILURE; /**< Would mean that this message either did not come at all or could not be dealt with properly. */
  }
    break;
   /** S10 Handover Timeouts. */
  case NW_GTP_FORWARD_RELOCATION_REQ:
   {
     itti_s10_forward_relocation_response_t            *resp_p;
     /** Respond with an S10 Forward Relocation Response Failure. */
     message_p = itti_alloc_new_message (TASK_S10, S10_FORWARD_RELOCATION_RESPONSE);
     resp_p = &message_p->ittiMsg.s10_forward_relocation_response;
     memset(resp_p, 0, sizeof(*resp_p));
     /** Set the destination TEID (our TEID). */
     resp_p->teid = pUlpApi->u_api_info.rspFailureInfo.teidLocal;
     /** Set the transaction for the triggered acknowledgement. */
     resp_p->trxn = (void *)pUlpApi->u_api_info.rspFailureInfo.hUlpTrxn;
     /** Set the cause. */
     resp_p->cause.cause_value = SYSTEM_FAILURE; /**< Would mean that this message either did not come at all or could not be dealt with properly. */
   }
     break;
  case NW_GTP_FORWARD_ACCESS_CONTEXT_NTF:
  {
    itti_s10_forward_access_context_acknowledge_t            *ack_p;
    /** Respond with an S10 Forward Access Context Acknowledgment Failure. */
    message_p = itti_alloc_new_message (TASK_S10, S10_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE);
    ack_p = &message_p->ittiMsg.s10_forward_access_context_acknowledge;
    memset(ack_p, 0, sizeof(*ack_p));
    /** Set the destination TEID (our TEID). */
    ack_p->teid = pUlpApi->u_api_info.rspFailureInfo.teidLocal;
    /** Set the transaction for the triggered acknowledgement. */
    ack_p->trxn = (void *)pUlpApi->u_api_info.rspFailureInfo.hUlpTrxn;
    /** Set the cause. */
    ack_p->cause.cause_value = SYSTEM_FAILURE; /**< Would mean that this message either did not come at all or could not be dealt with properly. */
  }
  break;
  case NW_GTP_FORWARD_RELOCATION_COMPLETE_NTF:
  {
    itti_s10_forward_access_context_acknowledge_t            *ack_p;
    /** Respond with an S10 Forward Relocation Complete Acknowledgment Failure. */
    message_p = itti_alloc_new_message (TASK_S10, S10_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGE);
    ack_p = &message_p->ittiMsg.s10_forward_relocation_complete_acknowledge;
    memset(ack_p, 0, sizeof(*ack_p));
    /** Set the destination TEID (our TEID). */
    ack_p->teid = pUlpApi->u_api_info.rspFailureInfo.teidLocal;
    /** Set the transaction for the triggered acknowledgement. */
    ack_p->trxn = (void *)pUlpApi->u_api_info.rspFailureInfo.hUlpTrxn;
    /** Set the cause. */
    ack_p->cause.cause_value = SYSTEM_FAILURE; /**< Would mean that this message either did not come at all or could not be dealt with properly. */
  }
  break;
  case NW_GTP_RELOCATION_CANCEL_REQ:
   {
     /** Respond with an S10 Relocation Cancel Response Failure. */
     OAILOG_WARNING (LOG_S10, "Not handling timeout for relocation cancel request for local S10-TEID " TEID_FMT ". \n", pUlpApi->u_api_info.rspFailureInfo.teidLocal);
     OAILOG_FUNC_RETURN (LOG_S10, RETURNok);
   }
   break;
  default:
    return RETURNerror;
  }
  OAILOG_WARNING (LOG_S10, "Received an error indicator for the local S10-TEID " TEID_FMT " and message type %d. \n",
      pUlpApi->u_api_info.rspFailureInfo.teidLocal, pUlpApi->u_api_info.rspFailureInfo.msgType);
  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

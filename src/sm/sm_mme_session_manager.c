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

/*! \file sm_mme_session_manager.c
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

#include "sm_mme_session_manager.h"

#include "gtpv2c_ie_formatter.h"
#include "mme_ie_defs.h"
#include "sm_common.h"
#include "sm_ie_formatter.h"


extern hash_table_ts_t                        *sm_mme_teid_2_gtv2c_teid_handle;

//------------------------------------------------------------------------------
int
sm_mme_handle_mbms_session_start_request(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  nw_rc_t                                   rc = NW_OK;
  uint8_t                                 offendingIeType,
                                          offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_sm_mbms_session_start_request_t   *req_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                 *pMsgParser;

  DevAssert (stack_p );

  teid_t teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);
  /** Check the destination TEID is 0. */
  if(teid != (teid_t)0){
    OAILOG_WARNING (LOG_SM, "Destination TEID of MBMS Session Start Request is not 0, instead " TEID_FMT ". Ignoring MBMS Session Start Request. \n", req_p->teid);
    return RETURNerror;
  }

  /** Allocating the Signal once at the sender (MME_APP --> SM) and once at the receiver (SM-->MME_APP). */
  message_p = itti_alloc_new_message (TASK_SM, SM_MBMS_SESSION_START_REQUEST);
  req_p = &message_p->ittiMsg.sm_mbms_session_start_request;
  memset(req_p, 0, sizeof(*req_p));
  req_p->teid = teid;

  req_p->trxn = (void *)pUlpApi->u_api_info.initialReqIndInfo.hTrxn;
  memcpy((void*)&req_p->peer_ip, pUlpApi->u_api_info.initialReqIndInfo.peerIp, pUlpApi->u_api_info.initialReqIndInfo.peerIp->sa_family == AF_INET ?
		  sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

  /*
   * Create a new message parser for the SM MBMS SESSION START REQUEST.
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_MBMS_SESSION_START_REQ, sm_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);

  /*
   * Sender (Source MME) FTEID for CP IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_FTEID, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
      gtpv2c_fteid_ie_get, &req_p->sm_mbms_teid);
  DevAssert (NW_OK == rc);


  // todo: PARSE SM STUFF

  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
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

  return itti_send_msg_to_task (TASK_MCE_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
sm_mme_mbms_session_start_response (
    nw_gtpv2c_stack_handle_t *stack_p,
    itti_sm_mbms_session_start_response_t *mbms_session_start_response_p)
{

   nw_rc_t                                 rc;
   nw_gtpv2c_ulp_api_t                     ulp_req;
   nw_gtpv2c_trxn_handle_t                 trxn;
   gtpv2c_cause_t                          cause;
   OAILOG_FUNC_IN (LOG_SM);

   DevAssert (mbms_session_start_response_p );
   DevAssert (stack_p );
   trxn = (nw_gtpv2c_trxn_handle_t) mbms_session_start_response_p->trxn;
   DevAssert (trxn );
   memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));

   /**
    * Create a tunnel for the GTPv2-C stack if the result is success.
    */
   if(mbms_session_start_response_p->cause.cause_value == REQUEST_ACCEPTED){
     memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
     ulp_req.apiType = NW_GTPV2C_ULP_CREATE_LOCAL_TUNNEL; /**< Create a Tunnel Endpoint for the SM. */
     ulp_req.u_api_info.createLocalTunnelInfo.teidLocal = mbms_session_start_response_p->sm_mme_teid.teid;
     ulp_req.u_api_info.createLocalTunnelInfo.peerIp = &mbms_session_start_response_p->peer_ip;
     ulp_req.u_api_info.createLocalTunnelInfo.hUlpTunnel = 0;
     ulp_req.u_api_info.createLocalTunnelInfo.hTunnel    = 0;
     rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);

     // todo: DevAssert (NW_OK == rc);
     if(NW_OK != rc){
       OAILOG_ERROR(LOG_SM, "Error, we could not setup local tunnel while sending MBMS_SESSION_START_RESPONSE!. \n");
       return RETURNerror;
     }

     OAILOG_INFO(LOG_SM, "Successfully created tunnel while MBMS_SESSION_START_RESPONSE!. \n");
     OAILOG_INFO (LOG_SM, "INSERTING INTO SM HASHTABLE (3) teid " TEID_FMT " and tunnel object %p. \n", ulp_req.u_api_info.createLocalTunnelInfo.teidLocal, ulp_req.u_api_info.createLocalTunnelInfo.hTunnel);

     hashtable_rc_t hash_rc = hashtable_ts_insert(sm_mme_teid_2_gtv2c_teid_handle,
         (hash_key_t) ulp_req.u_api_info.createLocalTunnelInfo.teidLocal,
         (void *)ulp_req.u_api_info.createLocalTunnelInfo.hTunnel);

     hash_rc = hashtable_ts_get(sm_mme_teid_2_gtv2c_teid_handle,
         (hash_key_t) ulp_req.u_api_info.createLocalTunnelInfo.teidLocal, (void **)(uintptr_t)&ulp_req.u_api_info.createLocalTunnelInfo.hTunnel);
     DevAssert(hash_rc == HASH_TABLE_OK);
   }else{
     OAILOG_WARNING (LOG_SM, "The cause is not REQUEST_ACCEPTED but %d for MBMS_SESSION_START_RESPONSE. "
         "Not creating a local SM Tunnel. \n", mbms_session_start_response_p->cause);
   }

   /**
    * Prepare an MBMS Session Start Response to send to MBMS-GW.
    */
   memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
   memset (&cause, 0, sizeof (gtpv2c_cause_t));
   ulp_req.apiType = NW_GTPV2C_ULP_API_TRIGGERED_RSP;
   ulp_req.u_api_info.triggeredRspInfo.hTrxn = trxn;
   rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_MBMS_SESSION_START_RSP, 0, 0, &(ulp_req.hMsg));
   DevAssert (NW_OK == rc);

   /**
    * Set the destination (MBMS-GW) TEID.
    */
   rc = nwGtpv2cMsgSetTeid (ulp_req.hMsg, mbms_session_start_response_p->teid);
   DevAssert (NW_OK == rc);

   /** Add the SM Cause : Not setting offending IE type now. */
   rc = nwGtpv2cMsgAddIeCause((ulp_req.hMsg), 0, mbms_session_start_response_p->cause.cause_value, 0, 0, 0);
   DevAssert( NW_OK == rc );

   if(mbms_session_start_response_p->cause.cause_value == REQUEST_ACCEPTED){
     /** Add the SM target-MME FTEID. */
     rc = nwGtpv2cMsgAddIeFteid ((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO, SM_MME_GTP_C,
    		 mbms_session_start_response_p->sm_mme_teid.teid, /**< FTEID of the TARGET_MME. */
			 mbms_session_start_response_p->sm_mme_teid.ipv4 ? &mbms_session_start_response_p->sm_mme_teid.ipv4_address : 0,
			 mbms_session_start_response_p->sm_mme_teid.ipv6 ? &mbms_session_start_response_p->sm_mme_teid.ipv6_address : NULL);
//
//     /** F-Cause to be added. */
//     rc = nwGtpv2cMsgAddIeFCause((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO,
//    		 mbms_session_start_response_p->f_cause.fcause_type, mbms_session_start_response_p->f_cause.fcause_value);
//     DevAssert( NW_OK == rc );
   }
   /** No allocated context remains. */
   rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
   DevAssert (NW_OK == rc);

   return RETURNok;
}

//------------------------------------------------------------------------------
int
sm_mme_handle_mbms_session_update_request(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  nw_rc_t                                   rc = NW_OK;
  uint8_t                                 offendingIeType,
                                          offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_sm_mbms_session_update_request_t  *req_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                 *pMsgParser;

  DevAssert (stack_p );

  teid_t teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);
  /** Check the destination TEID is 0. */
  if(teid != (teid_t)0){
    OAILOG_WARNING (LOG_SM, "Destination TEID of MBMS Session Update Request is not 0, instead " TEID_FMT ". Ignoring MBMS Session Update Request. \n", req_p->teid);
    return RETURNerror;
  }

  /** Allocating the Signal once at the sender (MME_APP --> SM) and once at the receiver (SM-->MME_APP). */
  message_p = itti_alloc_new_message (TASK_SM, SM_MBMS_SESSION_UPDATE_REQUEST);
  req_p = &message_p->ittiMsg.sm_mbms_session_update_request;
  memset(req_p, 0, sizeof(*req_p));
  req_p->teid = teid;

  req_p->trxn = (void *)pUlpApi->u_api_info.initialReqIndInfo.hTrxn;
  memcpy((void*)&req_p->peer_ip, pUlpApi->u_api_info.initialReqIndInfo.peerIp, pUlpApi->u_api_info.initialReqIndInfo.peerIp->sa_family == AF_INET ?
		  sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

  /*
   * Create a new message parser for the SM MBMS SESSION UPDATE REQUEST.
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_MBMS_SESSION_UPDATE_REQ, sm_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);

  /*
   * Sender (MBMS-GW) FTEID for CP IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_FTEID, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
      gtpv2c_fteid_ie_get, &req_p->sm_mbms_teid);
  DevAssert (NW_OK == rc);


  // todo: PARSE SM STUFF

  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
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

  return itti_send_msg_to_task (TASK_MCE_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
sm_mme_mbms_session_update_response (
    nw_gtpv2c_stack_handle_t *stack_p,
    itti_sm_mbms_session_update_response_t *mbms_session_update_response_p)
{

   nw_rc_t                                 rc;
   nw_gtpv2c_ulp_api_t                     ulp_req;
   nw_gtpv2c_trxn_handle_t                 trxn;
   gtpv2c_cause_t                          cause;
   OAILOG_FUNC_IN (LOG_SM);

   DevAssert (mbms_session_update_response_p );
   DevAssert (stack_p );
   trxn = (nw_gtpv2c_trxn_handle_t) mbms_session_update_response_p->trxn;
   DevAssert (trxn );
   memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));

   /**
    * Create a tunnel for the GTPv2-C stack if the result is success.
    */
   if(mbms_session_update_response_p->cause.cause_value == REQUEST_ACCEPTED){
     if(NW_OK != rc){
       OAILOG_ERROR(LOG_SM, "Error, we could not setup local tunnel while sending MBMS_SESSION_UPDATE_RESPONSE!. \n");
       return RETURNerror;
     }

     OAILOG_INFO(LOG_SM, "Successfully created tunnel while MBMS_SESSION_UPDATE_RESPONSE!. \n");
     OAILOG_INFO (LOG_SM, "INSERTING INTO SM HASHTABLE (3) teid " TEID_FMT " and tunnel object %p. \n", ulp_req.u_api_info.createLocalTunnelInfo.teidLocal, ulp_req.u_api_info.createLocalTunnelInfo.hTunnel);

     hashtable_rc_t hash_rc = hashtable_ts_insert(sm_mme_teid_2_gtv2c_teid_handle,
         (hash_key_t) ulp_req.u_api_info.createLocalTunnelInfo.teidLocal,
         (void *)ulp_req.u_api_info.createLocalTunnelInfo.hTunnel);

     hash_rc = hashtable_ts_get(sm_mme_teid_2_gtv2c_teid_handle,
         (hash_key_t) ulp_req.u_api_info.createLocalTunnelInfo.teidLocal, (void **)(uintptr_t)&ulp_req.u_api_info.createLocalTunnelInfo.hTunnel);
     DevAssert(hash_rc == HASH_TABLE_OK);
   }else{
     OAILOG_WARNING (LOG_SM, "The cause is not REQUEST_ACCEPTED but %d for MBMS_SESSION_UPDATE_RESPONSE. "
         "Not creating a local SM Tunnel. \n", mbms_session_update_response_p->cause);
   }

   /**
    * Prepare an MBMS Session Update Response to send to MBMS-GW.
    */
   memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
   memset (&cause, 0, sizeof (gtpv2c_cause_t));
   ulp_req.apiType = NW_GTPV2C_ULP_API_TRIGGERED_RSP;
   ulp_req.u_api_info.triggeredRspInfo.hTrxn = trxn;
   rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_MBMS_SESSION_UPDATE_RSP, 0, 0, &(ulp_req.hMsg));
   DevAssert (NW_OK == rc);

   /**
    * Set the destination (MBMS-GW) TEID.
    */
   rc = nwGtpv2cMsgSetTeid (ulp_req.hMsg, mbms_session_update_response_p->teid);
   DevAssert (NW_OK == rc);

   /** Add the SM Cause : Not setting offending IE type now. */
   rc = nwGtpv2cMsgAddIeCause((ulp_req.hMsg), 0, mbms_session_update_response_p->cause.cause_value, 0, 0, 0);
   DevAssert( NW_OK == rc );

   if(mbms_session_update_response_p->cause.cause_value == REQUEST_ACCEPTED){
     /** Add the SM target-MME FTEID. */
     rc = nwGtpv2cMsgAddIeFteid ((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO, SM_MME_GTP_C,
    		 mbms_session_update_response_p->sm_mme_teid.teid, /**< FTEID of the TARGET_MME. */
			 mbms_session_update_response_p->sm_mme_teid.ipv4 ? &mbms_session_update_response_p->sm_mme_teid.ipv4_address : 0,
			 mbms_session_update_response_p->sm_mme_teid.ipv6 ? &mbms_session_update_response_p->sm_mme_teid.ipv6_address : NULL);

//     /** F-Cause to be added. */
//     rc = nwGtpv2cMsgAddIeFCause((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO,
//    		 mbms_session_update_response_p->f_cause.fcause_type, mbms_session_update_response_p->f_cause.fcause_value);
//     DevAssert( NW_OK == rc );
   }
   /** No allocated context remains. */
   rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
   DevAssert (NW_OK == rc);

   return RETURNok;
}

//------------------------------------------------------------------------------
int
sm_mme_handle_mbms_session_stop_request(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  nw_rc_t                                   rc = NW_OK;
  uint8_t                                 offendingIeType,
                                          offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_sm_mbms_session_stop_request_t   *req_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                 *pMsgParser;

  DevAssert (stack_p );

  teid_t teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);
  /** Check the destination TEID is 0. */
  if(teid != (teid_t)0){
    OAILOG_WARNING (LOG_SM, "Destination TEID of MBMS Session Stop Request is not 0, instead " TEID_FMT ". Ignoring MBMS Session Stop Request. \n", req_p->teid);
    return RETURNerror;
  }

  /** Allocating the Signal once at the sender (MME_APP --> SM) and once at the receiver (SM-->MME_APP). */
  message_p = itti_alloc_new_message (TASK_SM, SM_MBMS_SESSION_STOP_REQUEST);
  req_p = &message_p->ittiMsg.sm_mbms_session_stop_request;
  memset(req_p, 0, sizeof(*req_p));
  req_p->teid = teid;

  req_p->trxn = (void *)pUlpApi->u_api_info.initialReqIndInfo.hTrxn;
  memcpy((void*)&req_p->peer_ip, pUlpApi->u_api_info.initialReqIndInfo.peerIp, pUlpApi->u_api_info.initialReqIndInfo.peerIp->sa_family == AF_INET ?
		  sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

  /*
   * Create a new message parser for the SM MBMS SESSION STOP REQUEST.
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_MBMS_SESSION_STOP_REQ, sm_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);

  // todo: PARSE SM STUFF

  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
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

  return itti_send_msg_to_task (TASK_MCE_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
sm_mme_mbms_session_stop_response (
    nw_gtpv2c_stack_handle_t *stack_p,
    itti_sm_mbms_session_stop_response_t *mbms_session_stop_response_p)
{

   nw_rc_t                                 rc;
   nw_gtpv2c_ulp_api_t                     ulp_req;
   nw_gtpv2c_trxn_handle_t                 trxn;
   gtpv2c_cause_t                          cause;
   OAILOG_FUNC_IN (LOG_SM);

   DevAssert (mbms_session_stop_response_p );
   DevAssert (stack_p );
   trxn = (nw_gtpv2c_trxn_handle_t) mbms_session_stop_response_p->trxn;
   DevAssert (trxn );
   memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));

   /**
    * Create a tunnel for the GTPv2-C stack if the result is success.
    */
   if(mbms_session_stop_response_p->cause.cause_value == REQUEST_ACCEPTED){
     if(NW_OK != rc){
       OAILOG_ERROR(LOG_SM, "Error, we could not setup local tunnel while sending MBMS_SESSION_STOP_RESPONSE!. \n");
       return RETURNerror;
     }

     OAILOG_INFO(LOG_SM, "Successfully created tunnel while MBMS_SESSION_STOP_RESPONSE!. \n");
     OAILOG_INFO (LOG_SM, "INSERTING INTO SM HASHTABLE (3) teid " TEID_FMT " and tunnel object %p. \n", ulp_req.u_api_info.createLocalTunnelInfo.teidLocal, ulp_req.u_api_info.createLocalTunnelInfo.hTunnel);

     hashtable_rc_t hash_rc = hashtable_ts_insert(sm_mme_teid_2_gtv2c_teid_handle,
         (hash_key_t) ulp_req.u_api_info.createLocalTunnelInfo.teidLocal,
         (void *)ulp_req.u_api_info.createLocalTunnelInfo.hTunnel);

     hash_rc = hashtable_ts_get(sm_mme_teid_2_gtv2c_teid_handle,
         (hash_key_t) ulp_req.u_api_info.createLocalTunnelInfo.teidLocal, (void **)(uintptr_t)&ulp_req.u_api_info.createLocalTunnelInfo.hTunnel);
     DevAssert(hash_rc == HASH_TABLE_OK);
   }else{
     OAILOG_WARNING (LOG_SM, "The cause is not REQUEST_ACCEPTED but %d for MBMS_SESSION_STOP_RESPONSE. "
         "Not creating a local SM Tunnel. \n", mbms_session_stop_response_p->cause);
   }

   /**
    * Prepare an MBMS Session Stop Response to send to MBMS-GW.
    */
   memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
   memset (&cause, 0, sizeof (gtpv2c_cause_t));
   ulp_req.apiType = NW_GTPV2C_ULP_API_TRIGGERED_RSP;
   ulp_req.u_api_info.triggeredRspInfo.hTrxn = trxn;
   rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_MBMS_SESSION_STOP_RSP, 0, 0, &(ulp_req.hMsg));
   DevAssert (NW_OK == rc);

   /**
    * Set the destination (MBMS-GW) TEID.
    */
   rc = nwGtpv2cMsgSetTeid (ulp_req.hMsg, mbms_session_stop_response_p->teid);
   DevAssert (NW_OK == rc);

   /** Add the SM Cause : Not setting offending IE type now. */
   rc = nwGtpv2cMsgAddIeCause((ulp_req.hMsg), 0, mbms_session_stop_response_p->cause.cause_value, 0, 0, 0);
   DevAssert( NW_OK == rc );

   if(mbms_session_stop_response_p->cause.cause_value == REQUEST_ACCEPTED){
//     /** F-Cause to be added. */
//     rc = nwGtpv2cMsgAddIeFCause((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO,
//    		 mbms_session_stop_response_p->f_cause.fcause_type, mbms_session_stop_response_p->f_cause.fcause_value);
//     DevAssert( NW_OK == rc );
   }
   /** No allocated context remains. */
   rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
   DevAssert (NW_OK == rc);

   return RETURNok;
}

//------------------------------------------------------------------------------
// todo: evaluate later the error cause in the removal!! --> eventually if something goes wrong here.. check the reason!
int
sm_mme_remove_ue_tunnel (
    nw_gtpv2c_stack_handle_t *stack_p, itti_sm_remove_ue_tunnel_t * remove_ue_tunnel_p)
{
  OAILOG_FUNC_IN (LOG_SM);
  nw_rc_t                                   rc = NW_OK;
  hashtable_rc_t                          hash_rc = HASH_TABLE_OK;
  DevAssert (stack_p );
  DevAssert (remove_ue_tunnel_p );
  MSC_LOG_RX_MESSAGE (MSC_SM_MME, MSC_SGW, NULL, 0, "Removing SM UE Tunnels for local SM teid " TEID_FMT " ",
      remove_ue_tunnel_p->local_teid);
  // delete local sm tunnel
  nw_gtpv2c_ulp_api_t                         ulp_req;
  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));

  if(remove_ue_tunnel_p->local_teid == (teid_t)0){
    OAILOG_ERROR (LOG_SM, "Cannot remove SM tunnel endpoint for local teid 0. \n");
    OAILOG_FUNC_RETURN(LOG_SM, rc);
  }

  hash_rc = hashtable_ts_get(sm_mme_teid_2_gtv2c_teid_handle,
      (hash_key_t) remove_ue_tunnel_p->local_teid,
      (void **)(uintptr_t)&ulp_req.u_api_info.deleteLocalTunnelInfo.hTunnel);
  if (HASH_TABLE_OK != hash_rc) {

//    NwGtpv2cTunnelT                        *pLocalTunnel = NULL,
//                                             keyTunnel = {0};
//    /** Check if a tunnel already exists depending on the flag. */
//    keyTunnel.teid = remove_ue_tunnel_p->remote_teid;
//    keyTunnel.ipv4AddrRemote = remove_ue_tunnel_p->peer_ip;
//    pLocalTunnel = RB_FIND (NwGtpv2cTunnelMap, &(thiz->tunnelMap), &keyTunnel);


    OAILOG_ERROR (LOG_SM, "Could not get GTPv2-C hTunnel for local teid " TEID_FMT". \n", remove_ue_tunnel_p->local_teid);
    MSC_LOG_EVENT (MSC_SM_MME, "Failed to deleted local teid " TEID_FMT "", remove_ue_tunnel_p->local_teid);
    // todo: error in error handling.. asserting?! extreme error handling?
    // Currently ignoring and continue to remove the remains of the tunnel.

    ulp_req.apiType = NW_GTPV2C_ULP_FIND_LOCAL_TUNNEL;
    ulp_req.u_api_info.findLocalTunnelInfo.teidLocal = remove_ue_tunnel_p->local_teid;
    ulp_req.u_api_info.findLocalTunnelInfo.edns_peer_ip = &remove_ue_tunnel_p->peer_ip;
    rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
    DevAssert (NW_OK == rc);
    if(ulp_req.u_api_info.findLocalTunnelInfo.hTunnel){
      OAILOG_ERROR (LOG_SM, "Could FIND A GTPv2-C hTunnel for local teid " TEID_FMT " @ DELETION \n", remove_ue_tunnel_p->local_teid);
    }else{
      OAILOG_ERROR (LOG_SM, "Could NOT FIND A GTPv2-C hTunnel for local teid " TEID_FMT " @ DELETION \n", remove_ue_tunnel_p->local_teid);
    }
    OAILOG_FUNC_RETURN(LOG_SM, RETURNerror);

  } else{

    ulp_req.apiType = NW_GTPV2C_ULP_DELETE_LOCAL_TUNNEL;

    rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
    DevAssert (NW_OK == rc);
    OAILOG_INFO(LOG_SM, "DELETED local SM teid (TEID FOUND IN HASH_MAP)" TEID_FMT " \n", remove_ue_tunnel_p->local_teid);

    memset(&ulp_req, 0, sizeof(ulp_req));

    ulp_req.apiType = NW_GTPV2C_ULP_FIND_LOCAL_TUNNEL;
    ulp_req.u_api_info.findLocalTunnelInfo.teidLocal = remove_ue_tunnel_p->local_teid;
    ulp_req.u_api_info.findLocalTunnelInfo.edns_peer_ip = &remove_ue_tunnel_p->peer_ip;
    rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
    DevAssert (NW_OK == rc);
    if(ulp_req.u_api_info.findLocalTunnelInfo.hTunnel){
      OAILOG_WARNING (LOG_SM, "Could FIND A GTPv2-C hTunnel for local teid " TEID_FMT " @ DELETION (2) \n", remove_ue_tunnel_p->local_teid);
    }else{
      OAILOG_WARNING(LOG_SM, "Could NOT FIND A GTPv2-C hTunnel for local teid " TEID_FMT " @ DELETION  (2) \n", remove_ue_tunnel_p->local_teid);
    }

    /**
     * hash_free_int_func is set as the freeing function.
     * The value is removed from the map. But the value itself (int) is not freed.
     * The Tunnels are not deallocated but just set back to the Tunnel pool.
     */
    hash_rc = hashtable_ts_free(sm_mme_teid_2_gtv2c_teid_handle, (hash_key_t) remove_ue_tunnel_p->local_teid);
    DevAssert (HASH_TABLE_OK == hash_rc);

    OAILOG_DEBUG(LOG_SM, "Successfully removed SM Tunnel local teid " TEID_FMT ".\n", remove_ue_tunnel_p->local_teid);
    OAILOG_FUNC_RETURN(LOG_SM, RETURNok);
  }
}

//------------------------------------------------------------------------------
int
sm_mme_handle_ulp_error_indicatior(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  /** Get the failed transaction. */
  /** Check the message type. */

  nw_gtpv2c_msg_type_t msgType = pUlpApi->u_api_info.rspFailureInfo.msgType;
  MessageDef * message_p = NULL;
  switch(msgType){
//  case NW_GTP_CONTEXT_REQ:
//  {
//    itti_sm_context_response_t            *resp_p;
//    /** Respond with an SM Context Response Failure. */
//    message_p = itti_alloc_new_message (TASK_SM, SM_CONTEXT_RESPONSE);
//    resp_p = &message_p->ittiMsg.sm_context_response;
//    memset(resp_p, 0, sizeof(*resp_p));
//    /** Set the destination TEID (our TEID). */
//    resp_p->teid = pUlpApi->u_api_info.rspFailureInfo.teidLocal;
//    /** Set the transaction for the triggered acknowledgement. */
//    resp_p->trxnId = (void *)pUlpApi->u_api_info.rspFailureInfo.hUlpTrxn;
//    /** Set the cause. */
//    resp_p->cause.cause_value = SYSTEM_FAILURE; /**< Would mean that this message either did not come at all or could not be dealt with properly. */
//  }
//    break;
//  case NW_GTP_CONTEXT_RSP:
//  {
//    itti_sm_context_acknowledge_t            *ack_p;
//    /**
//     * If CTX_RSP is sent but no context acknowledge is received
//     */
//    message_p = itti_alloc_new_message (TASK_SM, SM_CONTEXT_ACKNOWLEDGE);
//    ack_p = &message_p->ittiMsg.sm_context_acknowledge;
//    memset(ack_p, 0, sizeof(*ack_p));
//    /** Set the destination TEID (our TEID). */
//    ack_p->teid = pUlpApi->u_api_info.rspFailureInfo.teidLocal;
//    /** Set the transaction for the triggered acknowledgement. */
//    ack_p->trxnId = (uint32_t)pUlpApi->u_api_info.rspFailureInfo.hUlpTrxn;
//    /** Set the cause. */
//    ack_p->cause.cause_value = SYSTEM_FAILURE; /**< Would mean that this message either did not come at all or could not be dealt with properly. */
//  }
//    break;
//   /** SM Handover Timeouts. */
//  case NW_GTP_FORWARD_RELOCATION_REQ:
//   {
//     itti_sm_forward_relocation_response_t            *resp_p;
//     /** Respond with an SM Forward Relocation Response Failure. */
//     message_p = itti_alloc_new_message (TASK_SM, SM_FORWARD_RELOCATION_RESPONSE);
//     resp_p = &message_p->ittiMsg.sm_forward_relocation_response;
//     memset(resp_p, 0, sizeof(*resp_p));
//     /** Set the destination TEID (our TEID). */
//     resp_p->teid = pUlpApi->u_api_info.rspFailureInfo.teidLocal;
//     /** Set the transaction for the triggered acknowledgement. */
//     resp_p->trxn = (void *)pUlpApi->u_api_info.rspFailureInfo.hUlpTrxn;
//     /** Set the cause. */
//     resp_p->cause.cause_value = SYSTEM_FAILURE; /**< Would mean that this message either did not come at all or could not be dealt with properly. */
//   }
//     break;
//  case NW_GTP_FORWARD_ACCESS_CONTEXT_NTF:
//  {
//    itti_sm_forward_access_context_acknowledge_t            *ack_p;
//    /** Respond with an SM Forward Access Context Acknowledgment Failure. */
//    message_p = itti_alloc_new_message (TASK_SM, SM_FORWARD_ACCESS_CONTEXT_ACKNOWLEDGE);
//    ack_p = &message_p->ittiMsg.sm_forward_access_context_acknowledge;
//    memset(ack_p, 0, sizeof(*ack_p));
//    /** Set the destination TEID (our TEID). */
//    ack_p->teid = pUlpApi->u_api_info.rspFailureInfo.teidLocal;
//    /** Set the transaction for the triggered acknowledgement. */
//    ack_p->trxn = (void *)pUlpApi->u_api_info.rspFailureInfo.hUlpTrxn;
//    /** Set the cause. */
//    ack_p->cause.cause_value = SYSTEM_FAILURE; /**< Would mean that this message either did not come at all or could not be dealt with properly. */
//  }
//  break;
//  case NW_GTP_FORWARD_RELOCATION_COMPLETE_NTF:
//  {
//    itti_sm_forward_access_context_acknowledge_t            *ack_p;
//    /** Respond with an SM Forward Relocation Complete Acknowledgment Failure. */
//    message_p = itti_alloc_new_message (TASK_SM, SM_FORWARD_RELOCATION_COMPLETE_ACKNOWLEDGE);
//    ack_p = &message_p->ittiMsg.sm_forward_relocation_complete_acknowledge;
//    memset(ack_p, 0, sizeof(*ack_p));
//    /** Set the destination TEID (our TEID). */
//    ack_p->teid = pUlpApi->u_api_info.rspFailureInfo.teidLocal;
//    /** Set the transaction for the triggered acknowledgement. */
//    ack_p->trxn = (void *)pUlpApi->u_api_info.rspFailureInfo.hUlpTrxn;
//    /** Set the cause. */
//    ack_p->cause.cause_value = SYSTEM_FAILURE; /**< Would mean that this message either did not come at all or could not be dealt with properly. */
//  }
//  break;
//  case NW_GTP_RELOCATION_CANCEL_REQ:
//   {
//     /** Respond with an SM Relocation Cancel Response Success. */
////     OAILOG_WARNING (LOG_SM, "Not handling timeout for relocation cancel request for local SM-TEID " TEID_FMT ". \n", pUlpApi->u_api_info.rspFailureInfo.teidLocal);
//     itti_sm_relocation_cancel_response_t            *resp_p;
//     message_p = itti_alloc_new_message (TASK_SM, SM_RELOCATION_CANCEL_RESPONSE);
//     resp_p = &message_p->ittiMsg.sm_relocation_cancel_response;
//     memset(resp_p, 0, sizeof(*resp_p));
//     /** Set the destination TEID (our TEID). */
//     resp_p->teid = pUlpApi->u_api_info.rspFailureInfo.teidLocal;
//     /** Set the transaction for the triggered acknowledgement. */
//     resp_p->trxn = (void *)pUlpApi->u_api_info.rspFailureInfo.hUlpTrxn;
//     /** Set the cause. */
//     resp_p->cause.cause_value = REQUEST_ACCEPTED; /**< Would mean that this message either did not come at all or could not be dealt with properly. */
//     OAILOG_FUNC_RETURN (LOG_SM, RETURNok);
//   }
//   break;
  default:
    return RETURNerror;
  }
  OAILOG_WARNING (LOG_SM, "Received an error indicator for the local SM-TEID " TEID_FMT " and message type %d. \n",
      pUlpApi->u_api_info.rspFailureInfo.teidLocal, pUlpApi->u_api_info.rspFailureInfo.msgType);
  return itti_send_msg_to_task (TASK_MCE_APP, INSTANCE_DEFAULT, message_p);
}

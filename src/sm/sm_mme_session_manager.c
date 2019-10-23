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
#include "sm_common.h"
#include "sm_ie_formatter.h"

extern hash_table_ts_t                        *sm_mme_teid_2_gtv2c_teid_handle;

//------------------------------------------------------------------------------
int
sm_mme_handle_mbms_session_start_request(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  nw_rc_t                                 rc = NW_OK;
  uint8_t                                 offendingIeType,
                                          offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_sm_mbms_session_start_request_t   *req_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                 *pMsgParser;

  DevAssert (stack_p );

  teid_t teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);

  /** Allocating the Signal once at the sender (MME_APP --> SM) and once at the receiver (SM-->MME_APP). */
  message_p = itti_alloc_new_message (TASK_SM, SM_MBMS_SESSION_START_REQUEST);
  req_p = &message_p->ittiMsg.sm_mbms_session_start_request;
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
   * Sender (MBMS-GW) FTEID for CP IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_FTEID, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
      gtpv2c_fteid_ie_get, &req_p->sm_mbms_fteid);
  DevAssert (NW_OK == rc);

  /*
   * TMGI
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_TMGI, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
	  sm_tmgi_ie_get, &req_p->tmgi);
  DevAssert (NW_OK == rc);

  /*
   * MBMS Session Duration
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_MBMS_SESSION_DURATION, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
	  sm_mbms_session_duration_ie_get, &req_p->mbms_session_duration);
  DevAssert (NW_OK == rc);

  /*
   * MBMS Service Area
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_MBMS_SERVICE_AREA, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
	  sm_mbms_service_area_ie_get, &req_p->mbms_service_area);
  DevAssert (NW_OK == rc);

  /** Skip Session Identifier. */

  /*
   * MBMS Flow Identifier
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_MBMS_FLOW_IDENTIFIER, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
	  sm_mbms_flow_identifier_ie_get, &req_p->mbms_flow_id);
  DevAssert (NW_OK == rc);

  /*
   * QoS Profile
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_BEARER_LEVEL_QOS, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
	  gtpv2c_bearer_qos_ie_get, &req_p->mbms_bearer_level_qos);
  DevAssert (NW_OK == rc);

  /*
   * MBMS IP Multicast Distribution Address
   */
  req_p->mbms_ip_mc_address = calloc(1, sizeof(mbms_ip_multicast_distribution_t));
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_MBMS_IP_MULTICAST_DISTRIBUTION, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
	  sm_mbms_ip_multicast_distribution_ie_get, req_p->mbms_ip_mc_address);
  DevAssert (NW_OK == rc);

  /*
   * MBMS Absolute Time of Data Transfer Start
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_ABSOLUTE_TIME_MBMS_DATA_TRANSFER, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL,
	  sm_mbms_data_transfer_start_ie_get, &req_p->abs_start_time);
  DevAssert (NW_OK == rc);

  /*
   * MBMS-Flags
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_MBMS_FLAGS, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL,
	  sm_mbms_flags_ie_get, &req_p->mbms_flags);
  DevAssert (NW_OK == rc);

  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);
  if (rc != NW_OK) {
	if(req_p->mbms_ip_mc_address)
	  free_wrapper(&req_p->mbms_ip_mc_address);

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

   /** Add the SM target-MME FTEID. In case cause is not null, TEID should be 0. */
   rc = nwGtpv2cMsgAddIeFteid ((ulp_req.hMsg), NW_GTPV2C_IE_INSTANCE_ZERO, SM_MME_GTP_C,
		   mbms_session_start_response_p->sm_mme_teid.teid, /**< FTEID of the TARGET_MME. */
		   mbms_session_start_response_p->sm_mme_teid.ipv4 ? &mbms_session_start_response_p->sm_mme_teid.ipv4_address : 0,
		   mbms_session_start_response_p->sm_mme_teid.ipv6 ? &mbms_session_start_response_p->sm_mme_teid.ipv6_address : NULL);

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
  nw_rc_t                                 rc = NW_OK;
  uint8_t                                 offendingIeType,
                                          offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_sm_mbms_session_update_request_t  *req_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                 *pMsgParser;

  DevAssert (stack_p );

  teid_t teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);

  /** Allocating the Signal once at the sender (MME_APP --> SM) and once at the receiver (SM-->MME_APP). */
  message_p = itti_alloc_new_message (TASK_SM, SM_MBMS_SESSION_UPDATE_REQUEST);
  req_p = &message_p->ittiMsg.sm_mbms_session_update_request;
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
   * MBMS Service Area
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_MBMS_SERVICE_AREA, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
	  sm_mbms_service_area_ie_get, &req_p->mbms_service_area);
  DevAssert (NW_OK == rc);

  /*
   * TMGI
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_TMGI, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
	  sm_tmgi_ie_get, &req_p->tmgi);
  DevAssert (NW_OK == rc);

  /*
   * Sender (MBMS-GW) FTEID for CP IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_FTEID, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL,
      gtpv2c_fteid_ie_get, &req_p->sm_mbms_teid);
  DevAssert (NW_OK == rc);

  /*
   * MBMS Session Duration
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_MBMS_SESSION_DURATION, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
	  sm_mbms_session_duration_ie_get, &req_p->mbms_session_duration);
  DevAssert (NW_OK == rc);

  /*
   * QoS Profile
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_BEARER_LEVEL_QOS, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY,
	  gtpv2c_bearer_qos_ie_get, &req_p->mbms_bearer_level_qos);
  DevAssert (NW_OK == rc);

  /** Skip Session Identifier. */

  /*
   * MBMS Flow Identifier
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_MBMS_FLOW_IDENTIFIER, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
	  sm_mbms_flow_identifier_ie_get, &req_p->mbms_flow_id);
  DevAssert (NW_OK == rc);

  /*
   * MBMS Absolute Time of Data Transfer Start
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_ABSOLUTE_TIME_MBMS_DATA_TRANSFER, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL,
	  sm_mbms_data_transfer_start_ie_get, &req_p->abs_update_time);
  DevAssert (NW_OK == rc);

  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
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
   nw_gtpv2c_ulp_api_t                     ulp_rsp;
   nw_gtpv2c_trxn_handle_t                 trxn;
   gtpv2c_cause_t                          cause;
   OAILOG_FUNC_IN (LOG_SM);

   DevAssert (mbms_session_update_response_p );
   DevAssert (stack_p );
   trxn = (nw_gtpv2c_trxn_handle_t) mbms_session_update_response_p->trxn;
   DevAssert (trxn );
   /**
    * Prepare an MBMS Session Update Response to send to MBMS-GW.
    */
   memset (&ulp_rsp, 0, sizeof (nw_gtpv2c_ulp_api_t));
   memset (&cause, 0, sizeof (gtpv2c_cause_t));
   ulp_rsp.u_api_info.triggeredRspInfo.teidLocal = mbms_session_update_response_p->mms_sm_teid;
   ulp_rsp.apiType = NW_GTPV2C_ULP_API_TRIGGERED_RSP;
   ulp_rsp.u_api_info.triggeredRspInfo.hUlpTunnel = 0;
   ulp_rsp.u_api_info.triggeredRspInfo.hTunnel    = 0;
   ulp_rsp.u_api_info.triggeredRspInfo.hTrxn = trxn;

   hashtable_rc_t hash_rc = hashtable_ts_get(sm_mme_teid_2_gtv2c_teid_handle,
       (hash_key_t) ulp_rsp.u_api_info.triggeredRspInfo.teidLocal, (void **)(uintptr_t)&ulp_rsp.u_api_info.triggeredRspInfo.hTunnel);

   if (HASH_TABLE_OK != hash_rc) {
     OAILOG_WARNING (LOG_SM, "Could not get GTPv2-C hTunnel for local TEID %X on Sm MME interface. \n", ulp_rsp.u_api_info.triggeredRspInfo.teidLocal);
     return RETURNerror;
   }

   rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_MBMS_SESSION_UPDATE_RSP, 0, 0, &(ulp_rsp.hMsg));
   DevAssert (NW_OK == rc);

   /**
    * Set the destination (MBMS-GW) TEID.
    */
   rc = nwGtpv2cMsgSetTeid (ulp_rsp.hMsg, mbms_session_update_response_p->teid);
   DevAssert (NW_OK == rc);

   /** Add the SM Cause : Not setting offending IE type now. */
   rc = nwGtpv2cMsgAddIeCause((ulp_rsp.hMsg), 0, mbms_session_update_response_p->cause.cause_value, 0, 0, 0);
   DevAssert( NW_OK == rc );

   /** No allocated context remains. */
   rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_rsp);
   DevAssert (NW_OK == rc);

   return RETURNok;
}

//------------------------------------------------------------------------------
int
sm_mme_handle_mbms_session_stop_request(
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  nw_rc_t                                 rc = NW_OK;
  uint8_t                                 offendingIeType,
	                                          offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_sm_mbms_session_stop_request_t  *req_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                 *pMsgParser;

  DevAssert (stack_p );

  teid_t teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);

  /** Allocating the Signal once at the sender (MME_APP --> SM) and once at the receiver (SM-->MME_APP). */
  message_p = itti_alloc_new_message (TASK_SM, SM_MBMS_SESSION_STOP_REQUEST);
  req_p = &message_p->ittiMsg.sm_mbms_session_stop_request;
  req_p->teid = teid;
  req_p->trxn = (void *)pUlpApi->u_api_info.initialReqIndInfo.hTrxn;
  memcpy((void*)&req_p->peer_ip, pUlpApi->u_api_info.initialReqIndInfo.peerIp, pUlpApi->u_api_info.initialReqIndInfo.peerIp->sa_family == AF_INET ?
		  sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

  /*
   * Create a new message parser for the SM MBMS SESSION STOP REQUEST.
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_MBMS_SESSION_STOP_REQ, sm_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);

  /*
   * MBMS Flow Identifier
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_MBMS_FLOW_IDENTIFIER, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
	  sm_mbms_flow_identifier_ie_get, &req_p->mbms_flow_id);
  DevAssert (NW_OK == rc);

  /*
   * MBMS Absolute Time of Data Transfer Stop
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_ABSOLUTE_TIME_MBMS_DATA_TRANSFER, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL,
		  sm_mbms_data_transfer_start_ie_get, &req_p->abs_stop_time);
  DevAssert (NW_OK == rc);

  /*
   * MBMS-Flags
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_MBMS_FLAGS, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL,
	  sm_mbms_flags_ie_get, &req_p->mbms_flags);
  DevAssert (NW_OK == rc);

  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
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
  nw_gtpv2c_ulp_api_t                     ulp_rsp;
  nw_gtpv2c_trxn_handle_t                 trxn;
  gtpv2c_cause_t                          cause;
  OAILOG_FUNC_IN (LOG_SM);

  DevAssert (mbms_session_stop_response_p );
  DevAssert (stack_p );
  trxn = (nw_gtpv2c_trxn_handle_t) mbms_session_stop_response_p->trxn;
  DevAssert (trxn );
  /**
   * Prepare an MBMS Session Stop Response to send to MBMS-GW.
   */
  memset (&ulp_rsp, 0, sizeof (nw_gtpv2c_ulp_api_t));
  memset (&cause, 0, sizeof (gtpv2c_cause_t));
  ulp_rsp.u_api_info.triggeredRspInfo.teidLocal = mbms_session_stop_response_p->mms_sm_teid;
  ulp_rsp.apiType = NW_GTPV2C_ULP_API_TRIGGERED_RSP;
  ulp_rsp.u_api_info.triggeredRspInfo.hUlpTunnel = 0;
  ulp_rsp.u_api_info.triggeredRspInfo.hTunnel    = 0;
  ulp_rsp.u_api_info.triggeredRspInfo.hTrxn = trxn;

  hashtable_rc_t hash_rc = hashtable_ts_get(sm_mme_teid_2_gtv2c_teid_handle,
	(hash_key_t) ulp_rsp.u_api_info.triggeredRspInfo.teidLocal, (void **)(uintptr_t)&ulp_rsp.u_api_info.triggeredRspInfo.hTunnel);

  if (HASH_TABLE_OK != hash_rc) {
	  OAILOG_WARNING (LOG_SM, "Could not get GTPv2-C hTunnel for local TEID %X on Sm MME interface. \n", ulp_rsp.u_api_info.triggeredRspInfo.teidLocal);
	  return RETURNerror;
  }

  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_MBMS_SESSION_STOP_RSP, 0, 0, &(ulp_rsp.hMsg));
  DevAssert (NW_OK == rc);

  /**
   * Set the destination (MBMS-GW) TEID.
   */
  rc = nwGtpv2cMsgSetTeid (ulp_rsp.hMsg, mbms_session_stop_response_p->teid);
  DevAssert (NW_OK == rc);

  /** Add the SM Cause : Not setting offending IE type now. */
  rc = nwGtpv2cMsgAddIeCause((ulp_rsp.hMsg), 0, mbms_session_stop_response_p->cause.cause_value, 0, 0, 0);
  DevAssert( NW_OK == rc );

  /** No allocated context remains. */
  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_rsp);
  DevAssert (NW_OK == rc);

  /** Try to remove the tunnel directly. */
  OAILOG_INFO (LOG_SM, "Deleting the local Sm tunnel. \n");
  nw_gtpv2c_ulp_api_t                         ulp_req;
  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
  ulp_req.apiType = NW_GTPV2C_ULP_DELETE_LOCAL_TUNNEL;
  hash_rc = hashtable_ts_get(sm_mme_teid_2_gtv2c_teid_handle,
		  (hash_key_t) mbms_session_stop_response_p->teid,
		  (void **)(uintptr_t)&ulp_req.u_api_info.deleteLocalTunnelInfo.hTunnel);
  if (HASH_TABLE_OK != hash_rc) {
	OAILOG_ERROR (LOG_SM, "Could not get GTPv2-C hTunnel for local teid %X (skipping deletion of tunnel)\n", mbms_session_stop_response_p->teid);
  } else {
	rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
	DevAssert (NW_OK == rc);
	hash_rc = hashtable_ts_free(sm_mme_teid_2_gtv2c_teid_handle, (hash_key_t) mbms_session_stop_response_p->teid);
	DevAssert (HASH_TABLE_OK == hash_rc);
  }

  return RETURNok;
}

//------------------------------------------------------------------------------
// todo: evaluate later the error cause in the removal!! --> eventually if something goes wrong here.. check the reason!
int
sm_mme_remove_tunnel (
    nw_gtpv2c_stack_handle_t *stack_p, itti_sm_remove_tunnel_t * remove_tunnel_p)
{
  OAILOG_FUNC_IN (LOG_SM);
  nw_rc_t                                   rc = NW_OK;
  hashtable_rc_t                          hash_rc = HASH_TABLE_OK;
  DevAssert (stack_p );
  DevAssert (remove_tunnel_p );
  MSC_LOG_RX_MESSAGE (MSC_SM_MME, MSC_SGW, NULL, 0, "Removing SM Tunnels for local SM teid " TEID_FMT " ",
      remove_tunnel_p->local_teid);
  // delete local sm tunnel
  nw_gtpv2c_ulp_api_t                         ulp_req;
  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));

  if(remove_tunnel_p->local_teid == (teid_t)0){
    OAILOG_ERROR (LOG_SM, "Cannot remove SM tunnel endpoint for local teid 0. \n");
    OAILOG_FUNC_RETURN(LOG_SM, rc);
  }

  hash_rc = hashtable_ts_get(sm_mme_teid_2_gtv2c_teid_handle,
      (hash_key_t) remove_tunnel_p->local_teid,
      (void **)(uintptr_t)&ulp_req.u_api_info.deleteLocalTunnelInfo.hTunnel);
  if (HASH_TABLE_OK != hash_rc) {

//    NwGtpv2cTunnelT                        *pLocalTunnel = NULL,
//                                             keyTunnel = {0};
//    /** Check if a tunnel already exists depending on the flag. */
//    keyTunnel.teid = remove_tunnel_p->remote_teid;
//    keyTunnel.ipv4AddrRemote = remove_tunnel_p->peer_ip;
//    pLocalTunnel = RB_FIND (NwGtpv2cTunnelMap, &(thiz->tunnelMap), &keyTunnel);


    OAILOG_ERROR (LOG_SM, "Could not get GTPv2-C hTunnel for local teid " TEID_FMT". \n", remove_tunnel_p->local_teid);
    MSC_LOG_EVENT (MSC_SM_MME, "Failed to deleted local teid " TEID_FMT "", remove_tunnel_p->local_teid);
    // todo: error in error handling.. asserting?! extreme error handling?
    // Currently ignoring and continue to remove the remains of the tunnel.

    ulp_req.apiType = NW_GTPV2C_ULP_FIND_LOCAL_TUNNEL;
    ulp_req.u_api_info.findLocalTunnelInfo.teidLocal = remove_tunnel_p->local_teid;
    ulp_req.u_api_info.findLocalTunnelInfo.edns_peer_ip = &remove_tunnel_p->peer_ip;
    rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
    DevAssert (NW_OK == rc);
    if(ulp_req.u_api_info.findLocalTunnelInfo.hTunnel){
      OAILOG_ERROR (LOG_SM, "Could FIND A GTPv2-C hTunnel for local teid " TEID_FMT " @ DELETION \n", remove_tunnel_p->local_teid);
    }else{
      OAILOG_ERROR (LOG_SM, "Could NOT FIND A GTPv2-C hTunnel for local teid " TEID_FMT " @ DELETION \n", remove_tunnel_p->local_teid);
    }
    OAILOG_FUNC_RETURN(LOG_SM, RETURNerror);

  } else{

    ulp_req.apiType = NW_GTPV2C_ULP_DELETE_LOCAL_TUNNEL;

    rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
    DevAssert (NW_OK == rc);
    OAILOG_INFO(LOG_SM, "DELETED local SM teid (TEID FOUND IN HASH_MAP)" TEID_FMT " \n", remove_tunnel_p->local_teid);

    memset(&ulp_req, 0, sizeof(ulp_req));

    ulp_req.apiType = NW_GTPV2C_ULP_FIND_LOCAL_TUNNEL;
    ulp_req.u_api_info.findLocalTunnelInfo.teidLocal = remove_tunnel_p->local_teid;
    ulp_req.u_api_info.findLocalTunnelInfo.edns_peer_ip = &remove_tunnel_p->peer_ip;
    rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
    DevAssert (NW_OK == rc);
    if(ulp_req.u_api_info.findLocalTunnelInfo.hTunnel){
      OAILOG_WARNING (LOG_SM, "Could FIND A GTPv2-C hTunnel for local teid " TEID_FMT " @ DELETION (2) \n", remove_tunnel_p->local_teid);
    }else{
      OAILOG_WARNING(LOG_SM, "Could NOT FIND A GTPv2-C hTunnel for local teid " TEID_FMT " @ DELETION  (2) \n", remove_tunnel_p->local_teid);
    }

    /**
     * hash_free_int_func is set as the freeing function.
     * The value is removed from the map. But the value itself (int) is not freed.
     * The Tunnels are not deallocated but just set back to the Tunnel pool.
     */
    hash_rc = hashtable_ts_free(sm_mme_teid_2_gtv2c_teid_handle, (hash_key_t) remove_tunnel_p->local_teid);
    DevAssert (HASH_TABLE_OK == hash_rc);

    OAILOG_DEBUG(LOG_SM, "Successfully removed SM Tunnel local teid " TEID_FMT ".\n", remove_tunnel_p->local_teid);
    OAILOG_FUNC_RETURN(LOG_SM, RETURNok);
  }
}

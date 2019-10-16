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

/*! \file sm_mme_task.c
* \brief
* \author Dincer Beken
* \company Blackned GmbH
* \email: dbeken@blackned.de
*
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "assertions.h"
#include "hashtable.h"
#include "log.h"
#include "msc.h"
#include "mme_config.h"
#include "intertask_interface.h"
#include "itti_free_defined_msg.h"
#include "timer.h"
#include "NwLog.h"
#include "NwGtpv2c.h"
#include "NwGtpv2cMsg.h"
#include "sm_mme_session_manager.h"
#include "sm_mme.h"

static nw_gtpv2c_stack_handle_t             sm_mme_stack_handle = 0;
// Store the GTPv2-C teid handle
hash_table_ts_t                        *sm_mme_teid_2_gtv2c_teid_handle = NULL;
static void sm_exit(void);

//------------------------------------------------------------------------------
static nw_rc_t
sm_mme_log_wrapper (

  nw_gtpv2c_log_mgr_handle_t hLogMgr,
  uint32_t logLevel,
  char * file,
  uint32_t line,
  char * logStr)
{
  OAILOG_DEBUG (LOG_SM, "%s\n", logStr);
  return NW_OK;
}

//------------------------------------------------------------------------------
static nw_rc_t
sm_mme_ulp_process_stack_req_cb (
  nw_gtpv2c_ulp_handle_t hUlp,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  //     nw_rc_t rc = NW_OK;
  int                                     ret = 0;

  DevAssert (pUlpApi );

  switch (pUlpApi->apiType) {
  case NW_GTPV2C_ULP_API_INITIAL_REQ_IND:
    OAILOG_DEBUG (LOG_SM, "Received triggered response indication\n");

    switch (pUlpApi->u_api_info.initialReqIndInfo.msgType) {
    case NW_GTP_MBMS_SESSION_START_REQ:
      ret = sm_mme_handle_mbms_session_start_request(&sm_mme_stack_handle, pUlpApi);
      break;
    case NW_GTP_MBMS_SESSION_UPDATE_REQ:
      ret = sm_mme_handle_mbms_session_update_request(&sm_mme_stack_handle, pUlpApi);
      break;

    case NW_GTP_MBMS_SESSION_STOP_REQ:
      ret = sm_mme_handle_mbms_session_stop_request(&sm_mme_stack_handle, pUlpApi);
      break;

    default:
      OAILOG_WARNING (LOG_SM, "Received unhandled message type %d\n", pUlpApi->u_api_info.triggeredRspIndInfo.msgType);
      break;
    }
    break;

  case NW_GTPV2C_ULP_API_TRIGGERED_RSP_IND:
    OAILOG_ERROR (LOG_SM, "Received triggered response indication from MBMS-GW! NOT PROCESSED. \n");
    break;

  /** Timeout Handler */
  case NW_GTPV2C_ULP_API_RSP_FAILURE_IND:
    ret = sm_mme_handle_ulp_error_indicatior(&sm_mme_stack_handle, pUlpApi);
    break;

  default:
    break;
  }

  return ret == 0 ? NW_OK : NW_FAILURE;
}

//------------------------------------------------------------------------------
static nw_rc_t
sm_mme_send_udp_msg (
  nw_gtpv2c_udp_handle_t udpHandle,
  uint8_t * buffer,
  uint32_t buffer_len,
  uint16_t localPort,
  struct sockaddr *peerIpAddr,
  uint16_t peerPort)
{
  // Create and alloc new message
  MessageDef                             *message_p;
  udp_data_req_t                         *udp_data_req_p;
  int                                     ret = 0;

  message_p = itti_alloc_new_message (TASK_SM, UDP_DATA_REQ);
  udp_data_req_p = &message_p->ittiMsg.udp_data_req;
  udp_data_req_p->local_port = localPort;
  udp_data_req_p->peer_address = peerIpAddr;
  udp_data_req_p->peer_port = peerPort;
  udp_data_req_p->buffer = buffer;
  udp_data_req_p->buffer_length = buffer_len;

  ret = itti_send_msg_to_task (TASK_UDP, INSTANCE_DEFAULT, message_p);
  return ((ret == 0) ? NW_OK : NW_FAILURE);
}

//------------------------------------------------------------------------------
static nw_rc_t
sm_mme_start_timer_wrapper (
  nw_gtpv2c_timer_mgr_handle_t tmrMgrHandle,
  uint32_t timeoutSec,
  uint32_t timeoutUsec,
  uint32_t tmrType,
  void *timeoutArg,
  nw_gtpv2c_timer_handle_t * hTmr)
{
  long                                    timer_id;
  int                                     ret = 0;

  if (tmrType == NW_GTPV2C_TMR_TYPE_REPETITIVE) {
    ret = timer_setup (timeoutSec, timeoutUsec, TASK_SM, INSTANCE_DEFAULT, TIMER_PERIODIC, timeoutArg, &timer_id);
  } else {
    ret = timer_setup (timeoutSec, timeoutUsec, TASK_SM, INSTANCE_DEFAULT, TIMER_ONE_SHOT, timeoutArg, &timer_id);
  }

  *hTmr = (nw_gtpv2c_timer_handle_t) timer_id;
  return ((ret == 0) ? NW_OK : NW_FAILURE);
}

//------------------------------------------------------------------------------
static nw_rc_t
sm_mme_stop_timer_wrapper (
  nw_gtpv2c_timer_mgr_handle_t tmrMgrHandle,
  nw_gtpv2c_timer_handle_t tmrHandle)
{
  long                                    timer_id;
  void                                   *timeoutArg = NULL;

  timer_id = (long)tmrHandle;
  return ((timer_remove (timer_id, &timeoutArg) == 0) ? NW_OK : NW_FAILURE);
}

static void                            *
sm_mme_thread (
  void *args)
{
  itti_mark_task_ready (TASK_SM);
//  OAILOG_START_USE ();
//  MSC_START_USE ();

  while (1) {
    MessageDef                             *received_message_p = NULL;

    itti_receive_msg (TASK_SM, &received_message_p);
    assert (received_message_p );

    switch (ITTI_MSG_ID (received_message_p)) {
    /** Only the signals to send. */

    case SM_MBMS_SESSION_START_RESPONSE:{
      sm_mme_mbms_session_start_response(&sm_mme_stack_handle, &received_message_p->ittiMsg.sm_mbms_session_start_response);
    }
    break;

    case SM_MBMS_SESSION_UPDATE_RESPONSE:{
      sm_mme_mbms_session_update_response(&sm_mme_stack_handle, &received_message_p->ittiMsg.sm_mbms_session_update_response);
    }
    break;

    case SM_MBMS_SESSION_STOP_RESPONSE:{
      sm_mme_mbms_session_stop_response(&sm_mme_stack_handle, &received_message_p->ittiMsg.sm_mbms_session_stop_response);
    }
    break;

    /**
     * Use this message in case of an error to remove the SM Local Tunnel endpoints.
     * No response to MME_APP is sent/expected.
     */
    case SM_REMOVE_UE_TUNNEL:{
      sm_mme_remove_ue_tunnel(&sm_mme_stack_handle, &received_message_p->ittiMsg.sm_remove_ue_tunnel);
    }
    break;

    case UDP_DATA_IND:{
      /*
       * We received new data to handle from the UDP layer
       */
      nw_rc_t                                   rc;
      udp_data_ind_t                         *udp_data_ind;

      udp_data_ind = &received_message_p->ittiMsg.udp_data_ind;
      rc = nwGtpv2cProcessUdpReq (sm_mme_stack_handle, udp_data_ind->msgBuf, udp_data_ind->buffer_length, udp_data_ind->local_port,
    		  udp_data_ind->peer_port, &udp_data_ind->sock_addr);
      DevAssert (rc == NW_OK);
    }
    break;

    case TIMER_HAS_EXPIRED:{
        OAILOG_DEBUG (LOG_SM, "Processing timeout for timer_id 0x%lx and arg %p\n", received_message_p->ittiMsg.timer_has_expired.timer_id, received_message_p->ittiMsg.timer_has_expired.arg);
        DevAssert (nwGtpv2cProcessTimeout (received_message_p->ittiMsg.timer_has_expired.arg) == NW_OK);
      }
    break;

    case TERMINATE_MESSAGE: {
      sm_exit();
      itti_exit_task ();
      break;
    }

    default:{
    	OAILOG_ERROR (LOG_SM, "Unknown message ID %d:%s\n", ITTI_MSG_ID (received_message_p), ITTI_MSG_NAME (received_message_p));
    }
    break;
    }
    itti_free_msg_content(received_message_p);
    itti_free (ITTI_MSG_ORIGIN_ID (received_message_p), received_message_p);
    received_message_p = NULL;

  }
  return NULL;
}

//------------------------------------------------------------------------------
static int
sm_send_init_udp (
  struct in_addr *address,
  struct in6_addr *address6,
  uint16_t port_number)
{
  MessageDef                             *message_p = itti_alloc_new_message (TASK_SM, UDP_INIT);
  if (message_p == NULL) {
    return RETURNerror;
  }
  message_p->ittiMsg.udp_init.port = port_number;
  if(address && address->s_addr){
	  message_p->ittiMsg.udp_init.in_addr = address;
	  char ipv4[INET_ADDRSTRLEN];
	  inet_ntop (AF_INET, (void*)message_p->ittiMsg.udp_init.in_addr, ipv4, INET_ADDRSTRLEN);
	  OAILOG_DEBUG (LOG_SM, "Tx UDP_INIT IP addr %s:%" PRIu16 "\n", ipv4, message_p->ittiMsg.udp_init.port);
  }
  if(address6 && memcmp(address6->s6_addr, (void*)&in6addr_any, 16) != 0){
	  message_p->ittiMsg.udp_init.in6_addr = address6;
	  char ipv6[INET6_ADDRSTRLEN];
	  inet_ntop (AF_INET6, (void*)&message_p->ittiMsg.udp_init.in6_addr, ipv6, INET6_ADDRSTRLEN);
	  OAILOG_DEBUG (LOG_SM, "Tx UDP_INIT IPv6 addr %s:%" PRIu16 "\n", ipv6, message_p->ittiMsg.udp_init.port);
  }
  return itti_send_msg_to_task (TASK_UDP, INSTANCE_DEFAULT, message_p);
}


//------------------------------------------------------------------------------
int
sm_mme_init (
  const mme_config_t * mme_config_p)
{
  int                                     ret = 0;
  nw_gtpv2c_ulp_entity_t                  ulp;
  nw_gtpv2c_udp_entity_t                  udp;
  nw_gtpv2c_timer_mgr_entity_t            tmrMgr;
  nw_gtpv2c_log_mgr_entity_t              logMgr;
  struct in_addr                          addr;
  char                                   *sm_address_str = NULL;

  OAILOG_DEBUG (LOG_SM, "Initializing Sm interface\n");

  if (nwGtpv2cInitialize (&sm_mme_stack_handle) != NW_OK) {
    OAILOG_ERROR (LOG_SM, "Failed to initialize gtpv2-c stack\n");
    goto fail;
  }

  /*
   * Set ULP entity
   */
  ulp.hUlp = (nw_gtpv2c_ulp_handle_t) NULL;
  ulp.ulpReqCallback = sm_mme_ulp_process_stack_req_cb;
  DevAssert (NW_OK == nwGtpv2cSetUlpEntity (sm_mme_stack_handle, &ulp));
  /*
   * Set UDP entity
   */
  udp.hUdp = (nw_gtpv2c_udp_handle_t) NULL;
  mme_config_read_lock (&mme_config);
  udp.gtpv2cStandardPort = mme_config.ip.port_sm;
  mme_config_unlock (&mme_config);
  udp.udpDataReqCallback = sm_mme_send_udp_msg;
  DevAssert (NW_OK == nwGtpv2cSetUdpEntity (sm_mme_stack_handle, &udp));
  /*
   * Set Timer entity
   */
  tmrMgr.tmrMgrHandle = (nw_gtpv2c_timer_mgr_handle_t) NULL;
  tmrMgr.tmrStartCallback = sm_mme_start_timer_wrapper;
  tmrMgr.tmrStopCallback = sm_mme_stop_timer_wrapper;
  DevAssert (NW_OK == nwGtpv2cSetTimerMgrEntity (sm_mme_stack_handle, &tmrMgr));
  logMgr.logMgrHandle = 0;
  logMgr.logReqCallback = sm_mme_log_wrapper;
  DevAssert (NW_OK == nwGtpv2cSetLogMgrEntity (sm_mme_stack_handle, &logMgr));

  if (itti_create_task (TASK_SM, &sm_mme_thread, NULL) < 0) {
    OAILOG_ERROR (LOG_SM, "gtpv2c phtread_create: %s\n", strerror (errno));
    goto fail;
  }

  DevAssert (NW_OK == nwGtpv2cSetLogLevel (sm_mme_stack_handle, NW_LOG_LEVEL_DEBG));
  /** Create 2 sockets, one for 2123 (received initial requests), another high port. */
  sm_send_init_udp(&mme_config.ip.sm_mme_v4, &mme_config.ip.sm_mme_v6, udp.gtpv2cStandardPort);
  sm_send_init_udp(&mme_config.ip.sm_mme_v4, &mme_config.ip.sm_mme_v6, 0);

  bstring b = bfromcstr("sm_mme_teid_2_gtv2c_teid_handle");
  sm_mme_teid_2_gtv2c_teid_handle = hashtable_ts_create(mme_config_p->max_ues, HASH_TABLE_DEFAULT_HASH_FUNC, hash_free_int_func, b);
  bdestroy_wrapper(&b);

  OAILOG_DEBUG (LOG_SM, "Initializing SM interface: DONE\n");
  return ret;
fail:
  OAILOG_DEBUG (LOG_SM, "Initializing SM interface: FAILURE\n");
  return RETURNerror;
}


static void sm_exit(void)
{
  if (nwGtpv2cFinalize(sm_mme_stack_handle) != NW_OK) {
    OAI_FPRINTF_ERR ("An error occurred during tear down of nwGtp sm stack.\n");
  }
  if (hashtable_ts_destroy(sm_mme_teid_2_gtv2c_teid_handle) != HASH_TABLE_OK) {
    OAI_FPRINTF_ERR("An error occured while destroying sm teid hash table");
  }
}

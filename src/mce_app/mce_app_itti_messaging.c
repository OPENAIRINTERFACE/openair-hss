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

/*! \file mce_app_itti_messaging.c
  \brief
  \author Dincer Beken
  \company Blackned GmbH
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
#include "gcc_diag.h"
#include "esm_cause.h"
#include "mme_config.h"
#include "common_defs.h"
#include "mce_app_itti_messaging.h"

#include "mce_app_defs.h"
#include "mce_app_extern.h"

//------------------------------------------------------------------------------
int mce_app_remove_sm_tunnel_endpoint(teid_t local_teid, struct sockaddr *peer_ip){
  OAILOG_FUNC_IN(LOG_MCE_APP);

  MessageDef *message_p = itti_alloc_new_message (TASK_MCE_APP, SM_REMOVE_TUNNEL);
  DevAssert (message_p != NULL);
  message_p->ittiMsg.sm_remove_tunnel.local_teid   = local_teid;

  memcpy((void*)&message_p->ittiMsg.sm_remove_tunnel.mbms_peer_ip, peer_ip,
   		  (peer_ip->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

//  message_p->ittiMsg.sm_remove_tunnel.cause = LOCAL_DETACH;
  if(local_teid == (teid_t)0 ){
    OAILOG_DEBUG (LOG_MCE_APP, "Sending remove tunnel request for with null teid! \n");
  } else if (!peer_ip->sa_family){
    OAILOG_DEBUG (LOG_MCE_APP, "Sending remove tunnel request for with null peer ip! \n");
  }
  itti_send_msg_to_task (TASK_SM, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN(LOG_MCE_APP, RETURNok);
}

//------------------------------------------------------------------------------
void mce_app_itti_sm_mbms_session_start_response(teid_t mme_sm_teid, teid_t mbms_sm_teid, struct sockaddr *mbms_ip_address, void *trxn,  gtpv2c_cause_value_t gtpv2cCause){

  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MCE_APP);
  message_p = itti_alloc_new_message (TASK_MCE_APP, SM_MBMS_SESSION_START_RESPONSE);
  DevAssert (message_p != NULL);

  itti_sm_mbms_session_start_response_t *mbms_session_start_response_p = &message_p->ittiMsg.sm_mbms_session_start_response;

  /** Set the target SM TEID. */
  mbms_session_start_response_p->teid = mbms_sm_teid; /**< Only a single target-MME TEID can exist at a time. */
  mbms_session_start_response_p->trxn    = trxn;
  /** Set the cause. */
  mbms_session_start_response_p->cause.cause_value = gtpv2cCause;

  OAI_GCC_DIAG_OFF(pointer-to-int-cast);
  mbms_session_start_response_p->sm_mme_teid.teid = mme_sm_teid; /**< This one also sets the context pointer. */
  OAI_GCC_DIAG_ON(pointer-to-int-cast);
  mbms_session_start_response_p->sm_mme_teid.interface_type = SM_MME_GTP_C;
  if(mme_sm_teid != INVALID_TEID){
    mme_config_read_lock (&mme_config);
    if(mme_config.mbms.ip.mc_mme_v4.s_addr){
    	mbms_session_start_response_p->sm_mme_teid.ipv4_address= mme_config.mbms.ip.mc_mme_v4;
    	mbms_session_start_response_p->sm_mme_teid.ipv4 = 1;
    }
    if(memcmp(&mme_config.mbms.ip.mc_mme_v6.s6_addr, (void*)&in6addr_any, sizeof(mme_config.mbms.ip.mc_mme_v6.s6_addr)) != 0) {
    	memcpy(mbms_session_start_response_p->sm_mme_teid.ipv6_address.s6_addr, mme_config.mbms.ip.mc_mme_v6.s6_addr,  sizeof(mme_config.mbms.ip.mc_mme_v6.s6_addr));
    	mbms_session_start_response_p->sm_mme_teid.ipv6 = 1;
    }
    mme_config_unlock (&mme_config);
  }

  /** Set the MME Sm address. */
  memcpy((void*)&mbms_session_start_response_p->mbms_peer_ip, mbms_ip_address, mbms_ip_address->sa_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));
  /**
   * Sending a message to SM.
   * No changes in the contexts, flags, timers, etc.. needed.
   */
  itti_send_msg_to_task (TASK_SM, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
void mce_app_itti_sm_mbms_session_update_response(teid_t mme_sm_teid, teid_t mbms_sm_teid, struct sockaddr *mbms_ip_address, void *trxn,  gtpv2c_cause_value_t gtpv2cCause){
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MCE_APP);

  message_p = itti_alloc_new_message (TASK_MCE_APP, SM_MBMS_SESSION_UPDATE_RESPONSE);
  DevAssert (message_p != NULL);

  itti_sm_mbms_session_update_response_t *mbms_session_update_response_p = &message_p->ittiMsg.sm_mbms_session_update_response;

  /** Set the target SM TEID. */
  mbms_session_update_response_p->teid = mbms_sm_teid; /**< Only a single target-MME TEID can exist at a time. */
  mbms_session_update_response_p->mms_sm_teid = mme_sm_teid; /**< Only a single target-MME TEID can exist at a time. */
  mbms_session_update_response_p->trxn    = trxn;
  /** Set the cause. */
  mbms_session_update_response_p->cause.cause_value = gtpv2cCause;
  /** Set the MBMS Sm address. */
  memcpy((void*)&mbms_session_update_response_p->mbms_peer_ip, mbms_ip_address, mbms_ip_address->sa_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));
  /**
   * Sending a message to SM.
   * No changes in the contexts, flags, timers, etc.. needed.
   */
  itti_send_msg_to_task (TASK_SM, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
void mce_app_itti_sm_mbms_session_stop_response(teid_t mme_sm_teid, teid_t mbms_sm_teid, struct sockaddr *mbms_ip_address, void *trxn,  gtpv2c_cause_value_t gtpv2cCause){
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_MCE_APP);

  message_p = itti_alloc_new_message (TASK_MCE_APP, SM_MBMS_SESSION_STOP_RESPONSE);
  DevAssert (message_p != NULL);

  itti_sm_mbms_session_stop_response_t *mbms_session_stop_response_p = &message_p->ittiMsg.sm_mbms_session_stop_response;

  /** Set the target SM TEID. */
  mbms_session_stop_response_p->teid = mbms_sm_teid; /**< Only a single target-MME TEID can exist at a time. */
  mbms_session_stop_response_p->mms_sm_teid = mme_sm_teid; /**< Only a single target-MME TEID can exist at a time. */
  mbms_session_stop_response_p->trxn    = trxn;
  /** Set the cause. */
  mbms_session_stop_response_p->cause.cause_value = gtpv2cCause;
  /** Set the MBMS Sm address. */
  memcpy((void*)&mbms_session_stop_response_p->mbms_peer_ip, mbms_ip_address, mbms_ip_address->sa_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));
  /**
   * Sending a message to SM.
   * No changes in the contexts, flags, timers, etc.. needed.
   */
  itti_send_msg_to_task (TASK_SM, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}


//------------------------------------------------------------------------------
/**
 * M3AP Session Start Request.
 * Forward the bearer qos and absolute start time to the M2AP layer, which will decide on the scheduling.
 */
void mce_app_itti_m3ap_mbms_session_start_request(tmgi_t * tmgi, mbms_service_area_id_t mbms_service_area_id,
  bearer_qos_t * mbms_bearer_qos, mbms_ip_multicast_distribution_t * mbms_ip_mc_dist, const uint32_t abs_start_time_sec, const uint32_t abs_start_time_usec)
{
  MessageDef                             *message_p = NULL;
  int                                     rc 		= RETURNok;

  OAILOG_FUNC_IN (LOG_MCE_APP);
  message_p = itti_alloc_new_message (TASK_MCE_APP, M3AP_MBMS_SESSION_START_REQUEST);
  DevAssert (message_p != NULL);
  itti_m3ap_mbms_session_start_req_t *m3ap_mbms_session_start_req_p = &message_p->ittiMsg.m3ap_mbms_session_start_req;
  memcpy((void*)&m3ap_mbms_session_start_req_p->tmgi, tmgi, sizeof(tmgi_t));
  m3ap_mbms_session_start_req_p->mbms_service_area_id = mbms_service_area_id;
  memcpy((void*)&m3ap_mbms_session_start_req_p->mbms_bearer_tbc.bc_tbc.bearer_level_qos, mbms_bearer_qos, sizeof(bearer_qos_t));
  memcpy((void*)&m3ap_mbms_session_start_req_p->mbms_bearer_tbc.mbms_ip_mc_dist, mbms_ip_mc_dist, sizeof(mbms_ip_multicast_distribution_t));
  m3ap_mbms_session_start_req_p->abs_start_time_sec  = abs_start_time_sec;
  m3ap_mbms_session_start_req_p->abs_start_time_usec = abs_start_time_usec;
  itti_send_msg_to_task (TASK_M2AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}


//------------------------------------------------------------------------------
/** M3AP Session Update Request. */
void mce_app_itti_m3ap_mbms_session_update_request(tmgi_t * tmgi, const mbms_service_area_id_t new_mbms_service_area_id, const mbms_service_area_id_t old_mbms_service_area_id,
  bearer_qos_t * mbms_bearer_qos, mbms_ip_multicast_distribution_t * mbms_ip_mc_dist, const uint32_t abs_update_time_sec, const uint32_t abs_update_time_usec)
{
  MessageDef                             *message_p = NULL;
  int                                     rc 		= RETURNok;

  OAILOG_FUNC_IN (LOG_MCE_APP);
  message_p = itti_alloc_new_message (TASK_MCE_APP, M3AP_MBMS_SESSION_UPDATE_REQUEST);
  DevAssert (message_p != NULL);
  itti_m3ap_mbms_session_update_req_t *m3ap_mbms_session_update_req_p = &message_p->ittiMsg.m3ap_mbms_session_update_req;
  memcpy((void*)&m3ap_mbms_session_update_req_p->tmgi, tmgi, sizeof(tmgi_t));
  m3ap_mbms_session_update_req_p->new_mbms_service_area_id = new_mbms_service_area_id;
  m3ap_mbms_session_update_req_p->old_mbms_service_area_id = old_mbms_service_area_id;
  memcpy((void*)&m3ap_mbms_session_update_req_p->mbms_bearer_tbc.bc_tbc.bearer_level_qos, mbms_bearer_qos, sizeof(bearer_qos_t));
  memcpy((void*)&m3ap_mbms_session_update_req_p->mbms_bearer_tbc.mbms_ip_mc_dist, mbms_ip_mc_dist, sizeof(mbms_ip_multicast_distribution_t));
  m3ap_mbms_session_update_req_p->abs_update_time_sec = abs_update_time_sec;
  m3ap_mbms_session_update_req_p->abs_update_time_usec = abs_update_time_usec;
  itti_send_msg_to_task (TASK_M2AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
/** M3AP Session Stop Request. */
void mce_app_itti_m3ap_mbms_session_stop_request(tmgi_t *tmgi, mbms_service_area_id_t mbms_sa_id, const bool inform_enbs){
  MessageDef                             *message_p = NULL;
  int                                     rc 		= RETURNok;

  OAILOG_FUNC_IN (LOG_MCE_APP);

  message_p = itti_alloc_new_message (TASK_MCE_APP, M3AP_MBMS_SESSION_STOP_REQUEST);
  DevAssert (message_p != NULL);

  itti_m3ap_mbms_session_stop_req_t *m3ap_mbms_session_stop_req_p = &message_p->ittiMsg.m3ap_mbms_session_stop_req;
  memcpy((void*)&m3ap_mbms_session_stop_req_p->tmgi, (void*)tmgi, sizeof(tmgi_t));
  m3ap_mbms_session_stop_req_p->mbms_service_area_id = mbms_sa_id;
  m3ap_mbms_session_stop_req_p->inform_enbs = inform_enbs;
  /** No absolute stop time will be sent. That will be done immediately. */
  itti_send_msg_to_task (TASK_M2AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
/**
 * M3AP eNB Setup Response.
 * After handling MBSFN area creation/update, respond to an M2AP eNB setup request.
 */
void mce_app_itti_m3ap_enb_setup_response(mbsfn_areas_t * mbsfn_areas_p, uint32_t m2_enb_id, sctp_assoc_id_t assoc_id)
{
  MessageDef                             *message_p = NULL;
  int                                     rc 		= RETURNok;

  OAILOG_FUNC_IN (LOG_MCE_APP);
  message_p = itti_alloc_new_message (TASK_MCE_APP, M3AP_ENB_SETUP_RESPONSE);
  DevAssert (message_p != NULL);
  itti_m3ap_enb_setup_res_t *m3ap_enb_setup_res_p = &message_p->ittiMsg.m3ap_enb_setup_res;
  memcpy((void*)&m3ap_enb_setup_res_p->mbsfn_areas, mbsfn_areas_p, sizeof(mbsfn_areas_t));
  m3ap_enb_setup_res_p->sctp_assoc = assoc_id;
  m3ap_enb_setup_res_p->m2_enb_id  = m2_enb_id;
  itti_send_msg_to_task (TASK_M2AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

//------------------------------------------------------------------------------
/**
 * M3AP MBMS Scheduling Information
 * Send for expired MBSFN areas, before the beginning of their MCCH modification periods the, updated, CSA pattern.
 */
void mce_app_itti_m3ap_send_mbms_scheduling_info(mbsfn_areas_t * mbsfn_areas_p, long mcch_rep_rf)
{
  MessageDef                             *message_p = NULL;
  int                                     rc 		= RETURNok;

  OAILOG_FUNC_IN (LOG_MCE_APP);
  message_p = itti_alloc_new_message (TASK_MCE_APP, M3AP_MBMS_SCHEDULING_INFORMATION);
  DevAssert (message_p != NULL);
  itti_m3ap_mbms_scheduling_info_t *m3ap_mbms_scheduling_info_p = &message_p->ittiMsg.m3ap_mbms_scheduling_info;
  memcpy((void*)&m3ap_mbms_scheduling_info_p->mbsfn_areas, mbsfn_areas_p, sizeof(mbsfn_areas_t));
  m3ap_mbms_scheduling_info_p->mcch_rep_rf = mcch_rep_rf;
  itti_send_msg_to_task (TASK_M2AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT (LOG_MCE_APP);
}

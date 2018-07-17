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

/*! \file s11_sgw_session_manager.c
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "bstrlib.h"

#include "assertions.h"
#include "intertask_interface.h"
#include "queue.h"
#include "hashtable.h"
#include "NwLog.h"
#include "NwGtpv2c.h"
#include "NwGtpv2cIe.h"
#include "NwGtpv2cMsg.h"
#include "NwGtpv2cMsgParser.h"
#include "sgw_ie_defs.h"
#include "s11_common.h"
#include "s11_sgw_session_manager.h"
#include "s11_ie_formatter.h"
#include "log.h"
#include "s11_messages_types.h"
#include "gtpv2c_ie_formatter.h"

#ifdef __cplusplus
extern "C" {
#endif

extern hash_table_ts_t                        *s11_sgw_teid_2_gtv2c_teid_handle;

//------------------------------------------------------------------------------
int
s11_sgw_handle_downlink_data_notification (
  nw_gtpv2c_stack_handle_t * stack_p,
  itti_s11_downlink_data_notification_t * notif_p)
{
  nw_rc_t                                   rc;
  nw_gtpv2c_ulp_api_t                       ulp_req;

  OAILOG_DEBUG (LOG_S11, "Received S11_DOWNLINK_DATA_NOTIFICATION\n");
  DevAssert (notif_p );
  DevAssert (stack_p );

  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
  ulp_req.apiType = NW_GTPV2C_ULP_API_INITIAL_REQ;


  ulp_req.u_api_info.initialReqInfo.peerIp = notif_p->peer_ip;
  ulp_req.u_api_info.initialReqInfo.teidLocal  = notif_p->local_teid;

  hashtable_rc_t hash_rc = hashtable_ts_get(s11_sgw_teid_2_gtv2c_teid_handle,
      (hash_key_t) ulp_req.u_api_info.initialReqInfo.teidLocal, (void **)(uintptr_t)&ulp_req.u_api_info.initialReqInfo.hTunnel);

  if (HASH_TABLE_OK != hash_rc) {
    OAILOG_WARNING (LOG_S11, "Could not get GTPv2-C hTunnel for local TEID %X on S11 SGW interface. \n", ulp_req.u_api_info.initialReqInfo.teidLocal);
    return RETURNerror;
  }
  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_DOWNLINK_DATA_NOTIFICATION, notif_p->teid, 0, &(ulp_req.hMsg));
  DevAssert (NW_OK == rc);
  /*
   * Set the remote TEID
   */
  rc = nwGtpv2cMsgSetTeid (ulp_req.hMsg, notif_p->teid);
  DevAssert (NW_OK == rc);

  if (notif_p->ie_presence_mask & DOWNLINK_DATA_NOTIFICATION_PR_IE_CAUSE) {
    gtpv2c_cause_ie_set (&(ulp_req.hMsg), &notif_p->cause);
  }

  // TODO conditional
  if (notif_p->ie_presence_mask & DOWNLINK_DATA_NOTIFICATION_PR_IE_EPS_BEARER_ID) {
    gtpv2c_ebi_ie_set (&(ulp_req.hMsg), (unsigned)notif_p->ebi);
  }

  if (notif_p->ie_presence_mask & DOWNLINK_DATA_NOTIFICATION_PR_IE_ARP) {
    gtpv2c_arp_ie_set (&(ulp_req.hMsg), &notif_p->arp);
  }

  if (notif_p->ie_presence_mask & DOWNLINK_DATA_NOTIFICATION_PR_IE_IMSI) {
    gtpv2c_imsi_ie_set(&(ulp_req.hMsg), &notif_p->imsi);
  }

  // Sender F-TEID for Control Plane, indication flags, SGW's node level Load Control Information, SGW's Overload
  // Control Information, Paging and Service Information

  /** Send the message. */
  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
  DevAssert (NW_OK == rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
int
s11_sgw_handle_downlink_data_notification_ack (
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi)
{
  nw_rc_t                                 rc = NW_OK;
  uint8_t                                 offendingIeType,
                                          offendingIeInstance;
  uint16_t                                offendingIeLength;
  itti_s11_downlink_data_notification_acknowledge_t *resp_p;
  MessageDef                             *message_p;
  nw_gtpv2c_msg_parser_t                 *pMsgParser;

  DevAssert (stack_p );
  message_p = itti_alloc_new_message_sized (TASK_S11, S11_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE, sizeof(itti_s11_downlink_data_notification_acknowledge_t));
  resp_p = S11_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE(message_p);

  resp_p->teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg);

  /*
   * Create a new message parser
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_DOWNLINK_DATA_NOTIFICATION_ACK, s11_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);
  /*
   * Cause IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_CAUSE, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY, gtpv2c_cause_ie_get,
      &resp_p->cause);
  DevAssert (NW_OK == rc);


  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_IMSI, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL,
      gtpv2c_imsi_ie_get,
      &resp_p->imsi);
  DevAssert (NW_OK == rc);


  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_S11_MME, MSC_SGW, NULL, 0, "0 DOWNLINK_DATA_NOTIFICATION_ACK local S11 teid " TEID_FMT " ", resp_p->teid);
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

  MSC_LOG_RX_MESSAGE (MSC_S11_MME, MSC_SGW, NULL, 0, "0 DOWNLINK_DATA_NOTIFICATION_ACK local S11 teid " TEID_FMT " cause %u",
    resp_p->teid, resp_p->cause);

  rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
  DevAssert (NW_OK == rc);
  rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
  DevAssert (NW_OK == rc);
  return itti_send_msg_to_task (TASK_SPGW_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
int
s11_sgw_handle_downlink_data_notification_failure_ind (
    nw_gtpv2c_stack_handle_t * stack_p,
    nw_gtpv2c_ulp_api_t * pUlpApi)
  {
    nw_rc_t                                 rc = NW_OK;
    uint8_t                                 offendingIeType,
                                            offendingIeInstance;
    uint16_t                                offendingIeLength;
    itti_s11_downlink_data_notification_failure_indication_t   *initial_p;
    MessageDef                             *message_p;
    nw_gtpv2c_msg_parser_t                 *pMsgParser;

    DevAssert (stack_p );
    message_p = itti_alloc_new_message_sized (TASK_S11, S11_DOWNLINK_DATA_NOTIFICATION_FAILURE_INDICATION, sizeof(itti_s11_downlink_data_notification_failure_indication_t));
    initial_p = S11_DOWNLINK_DATA_NOTIFICATION_FAILURE_INDICATION(message_p);

    /*
    * Create a new message parser
    */
   rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_DOWNLINK_DATA_NOTIFICATION_FAILURE_IND, s11_ie_indication_generic, NULL, &pMsgParser);
   DevAssert (NW_OK == rc);

   /*
    * Cause IE
    */
   rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_CAUSE, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY, gtpv2c_cause_ie_get,
       &initial_p->cause);
   if (NW_OK == rc) initial_p->ie_presence_mask |= DOWNLINK_DATA_NOTIFICATION_FAILURE_IND_PR_IE_CAUSE;
   DevAssert (NW_OK == rc);

   /*
    * Imsi IE
    */
   rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_IMSI, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
       gtpv2c_imsi_ie_get, &initial_p->imsi);
   if (NW_OK == rc) initial_p->ie_presence_mask |= DOWNLINK_DATA_NOTIFICATION_FAILURE_IND_PR_IE_IMSI;
   DevAssert (NW_OK == rc);

   DevAssert (NW_OK == rc);
   initial_p->teid = nwGtpv2cMsgGetTeid (pUlpApi->hMsg);
   initial_p->trxn = (void *)pUlpApi->u_api_info.initialReqIndInfo.hTrxn;
   initial_p->peer_ip = pUlpApi->u_api_info.initialReqIndInfo.peerIp;
   rc = nwGtpv2cMsgParserRun (pMsgParser, pUlpApi->hMsg, &offendingIeType, &offendingIeInstance, &offendingIeLength);

   if (rc != NW_OK) {
     MSC_LOG_RX_DISCARDED_MESSAGE (MSC_S11_MME, MSC_SGW, NULL, 0,
         "0 DOWNLINK_DATA_NOTIFICATION_FAILURE_NOTIFICATION local S11 teid " TEID_FMT " ", initial_p->teid);
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
   return itti_send_msg_to_task (TASK_SPGW_APP, INSTANCE_DEFAULT, message_p);
 }


#ifdef __cplusplus
}
#endif


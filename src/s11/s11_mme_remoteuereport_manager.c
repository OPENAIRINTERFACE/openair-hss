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

/*! \file s11_mme_remoteuereport_manager.c
  \brief
  \author Sebastien ROUX, Lionel Gauthier, Mohit Vyas
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
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
#include "msc.h"

#include "NwGtpv2c.h"
#include "NwGtpv2cIe.h"
#include "NwGtpv2cMsg.h"
#include "NwGtpv2cMsgParser.h"

#include "s11_common.h"
#include "s11_mme_remoteuereport_manager.h"
#include "gtpv2c_ie_formatter.h"
#include "s11_ie_formatter.h"

extern hash_table_ts_t                        *s11_mme_teid_2_gtv2c_teid_handle;

//------------------------------------------------------------------------------
int s11_mme_remote_ue_report_notification(nw_gtpv2c_stack_handle_t *stack_p, 
itti_s11_remote_ue_report_notification_t *ntf_p)
{
  nw_gtpv2c_ulp_api_t                       ulp_req;
  nw_rc_t                                   rc;
  uint8_t                                   restart_counter = 0;
  OAILOG_FUNC_IN (LOG_S11);
  
  DevAssert (stack_p );
  DevAssert (ntf_p );
  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
  ulp_req.apiType = NW_GTPV2C_ULP_API_INITIAL_REQ;

  /*
   * Prepare a new Remote UE Report notification msg
   */
  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_REMOTE_UE_REPORT_NOTIFICATION, ntf_p->teid, 0, &(ulp_req.hMsg));
  ulp_req.u_api_info.initialReqInfo.edns_peer_ip = &ntf_p->edns_peer_ip;
  ulp_req.u_api_info.initialReqInfo.teidLocal  = ntf_p->local_teid;
  ulp_req.u_api_info.initialReqInfo.hUlpTunnel = 0;
  ulp_req.u_api_info.initialReqInfo.hTunnel    = 0;

     
   MSC_LOG_TX_MESSAGE (MSC_S11_MME, MSC_SGW, NULL, 0, "0 REMOTE UE REPORT NOTIFICATION S11 teid " TEID_FMT " num bearers ctx %u",
   ntf_p->local_teid);
      
  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);

  hashtable_rc_t hash_rc = hashtable_ts_get(s11_mme_teid_2_gtv2c_teid_handle,
  (hash_key_t) ulp_req.u_api_info.initialReqInfo.teidLocal, (void **)(uintptr_t)&ulp_req.u_api_info.initialReqInfo.hTunnel);

  if (HASH_TABLE_OK != hash_rc) {
    OAILOG_WARNING (LOG_S11, "Could not get GTPv2-C hTunnel for local teid %X\n", ulp_req.u_api_info.initialReqInfo.teidLocal);
    rc = nwGtpv2cMsgDelete (*stack_p, (ulp_req.hMsg));
    DevAssert (NW_OK == rc);
    return RETURNerror;
  }  
  DevAssert (NW_OK == rc);
  return RETURNok;
   OAILOG_FUNC_RETURN (LOG_S11, RETURNok);
}

//------------------------------------------------------------------------------

int s11_mme_remote_ue_report_acknowledge (nw_gtpv2c_stack_handle_t * stack_p, 
nw_gtpv2c_ulp_api_t * pUlpApi)
{
nw_rc_t                                        rc = NW_OK;
  itti_s11_remote_ue_report_acknowledge_t      *ack_p;
  uint8_t                                      offendingIeType,
                                               offendingIeInstance;
  uint16_t                                     offendingIeLength;
  MessageDef                                   *message_p;
  nw_gtpv2c_msg_parser_t                       *pMsgParser;

DevAssert (stack_p );
  message_p = itti_alloc_new_message (TASK_S10, S11_REMOTE_UE_REPORT_ACKNOWLEDGE);
  ack_p = &message_p->ittiMsg.s11_remote_ue_report_acknowledge;
  memset(ack_p, 0, sizeof(*ack_p));
  ack_p->teid = nwGtpv2cMsgGetTeid(pUlpApi->hMsg); /**< When the message is sent, this is the field, where the MME_APP sets the destination TEID.

  /*
   * Create a new message parser
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_GTP_REMOTE_UE_REPORT_ACK, s11_ie_indication_generic, NULL, &pMsgParser);

  /*
   * Cause IE
   */
  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_CAUSE, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY, gtpv2c_cause_ie_get,
		  &ack_p->cause);
  DevAssert (NW_OK == rc);

  /*
   * Run the parser
   */
  rc = nwGtpv2cMsgParserRun (pMsgParser, (pUlpApi->hMsg), &offendingIeType, &offendingIeInstance, &offendingIeLength);

  if (rc != NW_OK) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_S11_MME, MSC_SGW, NULL, 0, "0 REMOTE_UE_REPORT_ACKNOWLEDGE local S11 teid " TEID_FMT " ", ack_p->teid);
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

MSC_LOG_RX_MESSAGE (MSC_S11_MME, MSC_SGW, NULL, 0, "0 REMOTE UE REPORT ACKNOWLEDGE local S11 teid " TEID_FMT " cause %u",
    resp_p->teid, resp_p->cause);
  rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
  DevAssert (NW_OK == rc);
  rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
  DevAssert (NW_OK == rc);

  /** Check the cause. */
  if(ack_p->cause.cause_value == LATE_OVERLAPPING_REQUEST){
	  pUlpApi->u_api_info.triggeredRspIndInfo.trx_flags |= LATE_OVERLAPPING_REQUEST;
	  OAILOG_WARNING (LOG_S11, "Received a late overlapping request (MBR). Not forwarding message to MME_APP layer. \n");
	  itti_free (ITTI_MSG_ORIGIN_ID (message_p), message_p);
	  message_p = NULL;
	  return RETURNok;
  }

  return itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}


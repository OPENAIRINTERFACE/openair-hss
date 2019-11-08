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

/*! \file s11_sgw_remoteuereport_manager.c
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
#include "s11_sgw_bearer_manager.h"
#include "s11_sgw_remoteuereport_manager.h"
#include "s11_ie_formatter.h"
#include "log.h"

extern hash_table_ts_t                        *s11_sgw_teid_2_gtv2c_teid_handle;

//------------------------------------------------------------------------------
int s11_sgw_handle_remote_ue_report_notification(nw_gtpv2c_stack_handle_t *stack_p, 
nw_gtpv2c_ulp_api_t   *pUlpApi)

{
  nw_rc_t                                    rc = NW_OK;
  uint8_t                                    offendingIeType,
                                             offendingIeInstance;
  uint16_t                                   offendingIeLength;
  itti_s11_remote_ue_report_notification_t   *ntif_p;
  MessageDef                                 *message_p;
  nw_gtpv2c_msg_parser_t                     *pMsgParser;

  DevAssert (stack_p );
  message_p = itti_alloc_new_message (TASK_S11, S11_REMOTE_UE_REPORT_NOTIFICATION);
  ntif_p = &message_p->ittiMsg.s11_remote_ue_report_notification;
  //request_p->trxn = (void *)pUlpApi->u_api_info.initialReqIndInfo.hTrxn;
  ntif_p->teid = nwGtpv2cMsgGetTeid (pUlpApi->hMsg);
  
  /*
   * Create a new message parser
   */
  rc = nwGtpv2cMsgParserNew (*stack_p, NW_GTP_REMOTE_UE_REPORT_NOTIFICATION, s11_ie_indication_generic, NULL, &pMsgParser);
  DevAssert (NW_OK == rc);

   /*
   * Remote UE Context Connected IE
   */

  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_REMOTE_UE_CONTEXT, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      gtpv2c_delay_value_ie_get, &ntif_p->remoteuecontext_connected);
  DevAssert (NW_OK == rc);
   
   /*
   * Remote UE Context Disonnected IE
   */

  rc = nwGtpv2cMsgParserAddIe (pMsgParser, NW_GTPV2C_IE_REMOTE_UE_CONTEXT, NW_GTPV2C_IE_INSTANCE_ONE, NW_GTPV2C_IE_PRESENCE_CONDITIONAL,
      gtpv2c_delay_value_ie_get, &ntif_p->remoteuecontext_disconnected);
  DevAssert (NW_OK == rc);

rc = nwGtpv2cMsgParserRun (pMsgParser, pUlpApi->hMsg, &offendingIeType, &offendingIeInstance, &offendingIeLength);

  
  rc = nwGtpv2cMsgParserDelete (*stack_p, pMsgParser);
  DevAssert (NW_OK == rc);
  rc = nwGtpv2cMsgDelete (*stack_p, (pUlpApi->hMsg));
  DevAssert (NW_OK == rc);

  return itti_send_msg_to_task (TASK_SPGW_APP, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------

int s11_sgw_handle_remote_ue_report_acknowledge(nw_gtpv2c_stack_handle_t *stack_p, 
itti_s11_remote_ue_report_acknowledge_t *remote_ue_report_acknowledge_p)

{
  gtpv2c_cause_t                              cause;
  nw_rc_t                                     rc;
  nw_gtpv2c_ulp_api_t                         ulp_req;
  nw_gtpv2c_trxn_handle_t                     trxn;

  DevAssert (stack_p );
  DevAssert (remote_ue_report_acknowledge_p );

  /*
   * Prepare a Remote UE Report acknowledge message to send to MME.
   */

  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
  memset (&cause, 0, sizeof (gtpv2c_cause_t));
  ulp_req.apiType = NW_GTPV2C_ULP_API_TRIGGERED_ACK;
  ulp_req.u_api_info.triggeredRspInfo.hTrxn = trxn;
  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_GTP_REMOTE_UE_REPORT_ACK, 0, 0, &(ulp_req.hMsg));
  DevAssert (NW_OK == rc);

  /*
   * Set the remote TEID
   */
  rc = nwGtpv2cMsgSetTeid (ulp_req.hMsg, remote_ue_report_acknowledge_p->teid);
  DevAssert (NW_OK == rc);
  //TODO relay cause
  cause = remote_ue_report_acknowledge_p->cause;
  gtpv2c_cause_ie_set (&(ulp_req.hMsg), &cause);
  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
  DevAssert (NW_OK == rc);
  return RETURNok;
}






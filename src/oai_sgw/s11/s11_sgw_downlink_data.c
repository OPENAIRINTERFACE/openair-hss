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
  itti_s11_downlink_data_notification_t * downlink_data_notification_p)
{
  nw_rc_t                                   rc;
  nw_gtpv2c_ulp_api_t                       ulp_req;

  OAILOG_DEBUG (LOG_S11, "Received S11_DOWNLINK_DATA_NOTIFICATION\n");
  DevAssert (downlink_data_notification_p );
  DevAssert (stack_p );
  /**
   * Create a tunnel for the GTPv2-C stack
   */
  memset (&ulp_req, 0, sizeof (nw_gtpv2c_ulp_api_t));
  ulp_req.u_api_info.initialReqInfo.teidLocal = downlink_data_notification_p->local_teid;
  ulp_req.apiType = NW_GTPV2C_ULP_API_INITIAL_REQ; /**< Sending Side. */
  ulp_req.u_api_info.initialReqInfo.peerIp     = downlink_data_notification_p->peer_ip;
  hashtable_rc_t hash_rc = hashtable_ts_get(s11_sgw_teid_2_gtv2c_teid_handle,
      (hash_key_t) ulp_req.u_api_info.initialReqInfo.teidLocal, (void **)(uintptr_t)&ulp_req.u_api_info.initialReqInfo.hTunnel);
  if (HASH_TABLE_OK != hash_rc) {
    OAILOG_WARNING (LOG_S11, "Could not get GTPv2-C hTunnel for local TEID %X on S11 SGW interface. \n", ulp_req.u_api_info.initialReqInfo.teidLocal);
    return RETURNerror;
  }
  rc = nwGtpv2cMsgNew (*stack_p, true, NW_GTP_DOWNLINK_DATA_NOTIFICATION, 0, 0, &(ulp_req.hMsg));
  DevAssert (NW_OK == rc);
  /*
   * Set the remote TEID
   */
  rc = nwGtpv2cMsgSetTeid (ulp_req.hMsg, downlink_data_notification_p->teid);
  DevAssert (NW_OK == rc);

  if (downlink_data_notification_p->ie_presence_mask & DOWNLINK_DATA_NOTIFICATION_IE_CAUSE_PRESENT) {
    gtpv2c_cause_ie_set (&(ulp_req.hMsg), &downlink_data_notification_p->cause);
  }

  // TODO conditional
  if (downlink_data_notification_p->ie_presence_mask & DOWNLINK_DATA_NOTIFICATION_IE_EPS_BEARER_ID_PRESENT) {
    gtpv2c_ebi_ie_set (&(ulp_req.hMsg), (unsigned)downlink_data_notification_p->ebi);
  }

  if (downlink_data_notification_p->ie_presence_mask & DOWNLINK_DATA_NOTIFICATION_IE_ARP_PRESENT) {
    gtpv2c_arp_ie_set (&(ulp_req.hMsg), &downlink_data_notification_p->arp);
  }

  if (downlink_data_notification_p->ie_presence_mask & DOWNLINK_DATA_NOTIFICATION_IE_IMSI_PRESENT) {
    gtpv2c_imsi_ie_set(&(ulp_req.hMsg), &downlink_data_notification_p->imsi);
  }

  // Sender F-TEID for Control Plane, indication flags, SGW's node level Load Control Information, SGW's Overload
  // Control Information, Paging and Service Information

  /** Send the message. */
  rc = nwGtpv2cProcessUlpReq (*stack_p, &ulp_req);
  DevAssert (NW_OK == rc);
  return RETURNok;
}


#ifdef __cplusplus
}
#endif


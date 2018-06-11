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

/*! \file sgw_handler_gtpu.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#define SGW
#define S11_HANDLER_GTPU_C

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <netinet/in.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "assertions.h"
#include "conversions.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "common_defs.h"
#include "intertask_interface.h"
#include "log.h"
#include "sgw_ie_defs.h"
#include "3gpp_23.401.h"
#include "common_types.h"
#include "mme_config.h"
#include "sgw_defs.h"
#include "sgw.h"


#include "gtpv1_u_messages_types.h"
#include "s11_messages_types.h"
#include "sgw_context_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

extern sgw_app_t                        sgw_app;

//------------------------------------------------------------------------------
int
sgw_handle_gtpu_downlink_data_notification (
  const Gtpv1uDownlinkDataNotification * const gtpu_dl_data_notif)
{
  OAILOG_FUNC_IN(LOG_SPGW_APP);
  char *imsi_str = NULL;
  teid_t  s11lteid = INVALID_TEID;
  int rc = RETURNerror;


  // in SGW split key would be S5/S8 teid instead of ue_ip
  if ((rc = sgw_get_subscriber_id_from_ipv4(&gtpu_dl_data_notif->ue_ip, &imsi_str, &s11lteid))) {

    s_plus_p_gw_eps_bearer_context_information_t *s_plus_p_gw_eps_bearer_ctxt_info_p = NULL;
    hashtable_rc_t                          hash_rc = HASH_TABLE_OK;

    hash_rc = hashtable_ts_get (sgw_app.s11_bearer_context_information_hashtable, s11lteid, (void **)&s_plus_p_gw_eps_bearer_ctxt_info_p);

    if (HASH_TABLE_OK == hash_rc) {
      MessageDef  *message_p = itti_alloc_new_message_sized (TASK_SPGW_APP, S11_DOWNLINK_DATA_NOTIFICATION,
          sizeof(itti_s11_downlink_data_notification_t));

      if (message_p) {
        itti_s11_downlink_data_notification_t *s11_downlink_data_notification = S11_DOWNLINK_DATA_NOTIFICATION(message_p);

        // TODO EBI
        s11_downlink_data_notification->ie_presence_mask |= DOWNLINK_DATA_NOTIFICATION_IE_EPS_BEARER_ID_PRESENT;
        s11_downlink_data_notification->ebi = gtpu_dl_data_notif->eps_bearer_id;

        // TODO ARP
//        s11_downlink_data_notification->ie_presence_mask |= DOWNLINK_DATA_NOTIFICATION_IE_ARP_PRESENT;
//        s11_downlink_data_notification->arp.pre_emp_capability = s_plus_p_gw_eps_bearer_ctxt_info_p->sgw_eps_bearer_context_information.pdn_connection.sgw_eps_bearers_array[0]->eps_bearer_qos.pci;
//        s11_downlink_data_notification->arp.pre_emp_vulnerability = s_plus_p_gw_eps_bearer_ctxt_info_p->sgw_eps_bearer_context_information.pdn_connection.sgw_eps_bearers_array[0]->eps_bearer_qos.pvi;
//        s11_downlink_data_notification->arp.priority_level = s_plus_p_gw_eps_bearer_ctxt_info_p->sgw_eps_bearer_context_information.pdn_connection.sgw_eps_bearers_array[0]->eps_bearer_qos.pl;

        // NO NEED FOR IMSI FOR SIMPLE CASE
        //s11_create_bearer_request->trxn = s_plus_p_gw_eps_bearer_ctxt_info_p->sgw_eps_bearer_context_information.trxn;
        s11_downlink_data_notification->peer_ip.s_addr = s_plus_p_gw_eps_bearer_ctxt_info_p->sgw_eps_bearer_context_information.mme_ip_address_S11.address.ipv4_address.s_addr;
        s11_downlink_data_notification->local_teid = s_plus_p_gw_eps_bearer_ctxt_info_p->sgw_eps_bearer_context_information.s_gw_teid_S11_S4;
      }
      OAILOG_FUNC_RETURN(LOG_SPGW_APP, rc);
    }
  }
  OAILOG_FUNC_RETURN(LOG_SPGW_APP, RETURNerror);
}



#ifdef __cplusplus
}
#endif

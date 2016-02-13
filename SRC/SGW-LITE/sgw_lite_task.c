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

/*! \file sgw_lite_task.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#define SGW_LITE
#define SGW_LITE_TASK_C

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "queue.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "sgw_lite_defs.h"
#include "sgw_lite_handlers.h"
#include "sgw_lite.h"
#include "hashtable.h"
#include "spgw_config.h"
#include "pgw_lite_paa.h"
#include "msc.h"
#include "log.h"

spgw_config_t                           spgw_config;
sgw_app_t                               sgw_app;
pgw_app_t                               pgw_app;

static void sgw_lite_exit(void);

//------------------------------------------------------------------------------
static void                            *
sgw_lite_intertask_interface (
  void *args_p)
{
  itti_mark_task_ready (TASK_SPGW_APP);
  LOG_START_USE ();
  MSC_START_USE ();

  while (1) {
    MessageDef                             *received_message_p = NULL;

    itti_receive_msg (TASK_SPGW_APP, &received_message_p);

    switch (ITTI_MSG_ID (received_message_p)) {
    case SGW_CREATE_SESSION_REQUEST:{
        /*
         * We received a create session request from MME (with GTP abstraction here)
         * * * * procedures might be:
         * * * *      E-UTRAN Initial Attach
         * * * *      UE requests PDN connectivity
         */
        sgw_lite_handle_create_session_request (&received_message_p->ittiMsg.sgwCreateSessionRequest);
      }
      break;

    case SGW_MODIFY_BEARER_REQUEST:{
        sgw_lite_handle_modify_bearer_request (&received_message_p->ittiMsg.sgwModifyBearerRequest);
      }
      break;

    case SGW_RELEASE_ACCESS_BEARERS_REQUEST:{
        sgw_lite_handle_release_access_bearers_request (&received_message_p->ittiMsg.sgwReleaseAccessBearersRequest);
      }
      break;

    case SGW_DELETE_SESSION_REQUEST:{
        sgw_lite_handle_delete_session_request (&received_message_p->ittiMsg.sgwDeleteSessionRequest);
      }
      break;

    case GTPV1U_CREATE_TUNNEL_RESP:{
        LOG_DEBUG (LOG_SPGW_APP, "Received teid for S1-U: %u and status: %s\n", received_message_p->ittiMsg.gtpv1uCreateTunnelResp.S1u_teid, received_message_p->ittiMsg.gtpv1uCreateTunnelResp.status == 0 ? "Success" : "Failure");
        sgw_lite_handle_gtpv1uCreateTunnelResp (&received_message_p->ittiMsg.gtpv1uCreateTunnelResp);
      }
      break;

    case GTPV1U_UPDATE_TUNNEL_RESP:{
        sgw_lite_handle_gtpv1uUpdateTunnelResp (&received_message_p->ittiMsg.gtpv1uUpdateTunnelResp);
      }
      break;

    case SGI_CREATE_ENDPOINT_RESPONSE:{
        sgw_lite_handle_sgi_endpoint_created (&received_message_p->ittiMsg.sgiCreateEndpointResp);
      }
      break;

    case SGI_UPDATE_ENDPOINT_RESPONSE:{
        sgw_lite_handle_sgi_endpoint_updated (&received_message_p->ittiMsg.sgiUpdateEndpointResp);
      }
      break;

    case TERMINATE_MESSAGE:{
        sgw_lite_exit();
        itti_exit_task ();
      }
      break;

    case MESSAGE_TEST:
      LOG_DEBUG (LOG_SPGW_APP, "Received MESSAGE_TEST\n");
      break;

    default:{
        LOG_DEBUG (LOG_SPGW_APP, "Unkwnon message ID %d:%s\n", ITTI_MSG_ID (received_message_p), ITTI_MSG_NAME (received_message_p));
      }
      break;
    }

    itti_free (ITTI_MSG_ORIGIN_ID (received_message_p), received_message_p);
    received_message_p = NULL;
  }

  return NULL;
}

//------------------------------------------------------------------------------
int sgw_lite_init (
  char *config_file_name_pP)
{
  LOG_DEBUG (LOG_SPGW_APP, "Initializing SPGW-APP  task interface\n");
  spgw_system ("modprobe ip_tables", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("modprobe x_tables", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("iptables -P INPUT ACCEPT", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("iptables -F INPUT", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("iptables -P OUTPUT ACCEPT", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("iptables -F OUTPUT", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("iptables -P FORWARD ACCEPT", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("iptables -F FORWARD", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("iptables -t nat    -F", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("iptables -t mangle -F", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("iptables -t filter -F", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("iptables -t raw    -F", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("iptables -t nat    -Z", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("iptables -t mangle -Z", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("iptables -t filter -Z", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("iptables -t raw    -Z", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("ip route flush cache", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("rmmod iptable_raw    > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("rmmod iptable_mangle > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("rmmod iptable_nat    > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("rmmod iptable_filter > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("rmmod ip_tables      > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("rmmod xt_state xt_mark xt_tcpudp xt_connmark ipt_LOG ipt_MASQUERADE > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("rmmod x_tables       > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("rmmod nf_conntrack_netlink nfnetlink nf_nat nf_conntrack_ipv4 nf_conntrack  > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("modprobe ip_tables", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("modprobe iptable_filter", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("modprobe iptable_mangle", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("modprobe iptable_nat", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("modprobe iptable_raw", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("modprobe ipt_MASQUERADE", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("modprobe ipt_LOG", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("modprobe nf_conntrack", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("modprobe nf_conntrack_ipv4", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("modprobe nf_nat", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("modprobe x_tables", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("modprobe udp_tunnel", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("modprobe ip6_udp_tunnel", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("sysctl -w net.ipv4.ip_forward=1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("sysctl -w net.ipv4.conf.all.accept_local=1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("sysctl -w net.ipv4.conf.all.log_martians=1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("sysctl -w net.ipv4.conf.all.route_localnet=1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("sysctl -w net.ipv4.conf.all.rp_filter=0", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system ("sync", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_config_init (config_file_name_pP, &spgw_config);
  pgw_lite_load_pool_ip_addresses ();
  sgw_app.s11teid2mme_hashtable = hashtable_ts_create (8192, NULL, NULL, "sgw_s11teid2mme_hashtable");

  if (sgw_app.s11teid2mme_hashtable == NULL) {
    perror ("hashtable_ts_create");
    LOG_ALERT (LOG_SPGW_APP, "Initializing SPGW-APP task interface: ERROR\n");
    return RETURNerror;
  }

  /*sgw_app.s1uteid2enb_hashtable = hashtable_ts_create (8192, NULL, NULL, "sgw_s1uteid2enb_hashtable");

  if (sgw_app.s1uteid2enb_hashtable == NULL) {
    perror ("hashtable_ts_create");
    LOG_ALERT (LOG_SPGW_APP, "Initializing SPGW-APP task interface: ERROR\n");
    return RETURNerror;
  }*/

  sgw_app.s11_bearer_context_information_hashtable = hashtable_ts_create (8192, NULL,
          (void (*)(void*))sgw_lite_cm_free_s_plus_p_gw_eps_bearer_context_information,
          "sgw_s11_bearer_context_information_hashtable");

  if (sgw_app.s11_bearer_context_information_hashtable == NULL) {
    perror ("hashtable_ts_create");
    LOG_ALERT (LOG_SPGW_APP, "Initializing SPGW-APP task interface: ERROR\n");
    return RETURNerror;
  }

  sgw_app.sgw_interface_name_for_S1u_S12_S4_up = spgw_config.sgw_config.ipv4.sgw_interface_name_for_S1u_S12_S4_up;
  sgw_app.sgw_ip_address_for_S1u_S12_S4_up = spgw_config.sgw_config.ipv4.sgw_ipv4_address_for_S1u_S12_S4_up;
  sgw_app.sgw_interface_name_for_S11_S4 = spgw_config.sgw_config.ipv4.sgw_interface_name_for_S11;
  sgw_app.sgw_ip_address_for_S11_S4 = spgw_config.sgw_config.ipv4.sgw_ipv4_address_for_S11;
  //sgw_app.sgw_ip_address_for_S5_S8_cp          = spgw_config.sgw_config.ipv4.sgw_ip_address_for_S5_S8_cp;
  sgw_app.sgw_ip_address_for_S5_S8_up = spgw_config.sgw_config.ipv4.sgw_ipv4_address_for_S5_S8_up;

  if (itti_create_task (TASK_SPGW_APP, &sgw_lite_intertask_interface, NULL) < 0) {
    perror ("pthread_create");
    LOG_ALERT (LOG_SPGW_APP, "Initializing SPGW-APP task interface: ERROR\n");
    return RETURNerror;
  }

  LOG_DEBUG (LOG_SPGW_APP, "Initializing SPGW-APP task interface: DONE\n");
  return RETURNok;
}

//------------------------------------------------------------------------------
static void sgw_lite_exit(void)
{
  if (sgw_app.s11teid2mme_hashtable) {
    hashtable_destroy (sgw_app.s11teid2mme_hashtable);
  }
  /*if (sgw_app.s1uteid2enb_hashtable) {
    hashtable_destroy (sgw_app.s1uteid2enb_hashtable);
  }*/
  if (sgw_app.s11_bearer_context_information_hashtable) {
    hashtable_destroy (sgw_app.s11_bearer_context_information_hashtable);
  }

  //P-GW code
  struct pgw_lite_conf_ipv4_list_elm_s   *conf_ipv4_p = NULL;

  while ((conf_ipv4_p = STAILQ_FIRST (&spgw_config.pgw_config.pgw_lite_ipv4_pool_list))) {
    STAILQ_REMOVE_HEAD (&spgw_config.pgw_config.pgw_lite_ipv4_pool_list, ipv4_entries);
    FREE_CHECK (conf_ipv4_p);
  }

  struct pgw_lite_conf_ipv6_list_elm_s   *conf_ipv6_p = NULL;
  while ((conf_ipv6_p = STAILQ_FIRST (&spgw_config.pgw_config.pgw_lite_ipv6_pool_list))) {
    STAILQ_REMOVE_HEAD (&spgw_config.pgw_config.pgw_lite_ipv6_pool_list, ipv6_entries);
    FREE_CHECK (conf_ipv6_p);
  }
}

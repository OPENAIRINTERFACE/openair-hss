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

#include <netinet/ip.h>
//#include <netinet/ether.h>
//#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include "OpenflowController.h"
#include "PagingApplication.h"

extern "C" {
  #include "log.h"
  #include "pgw_lite_paa.h"
  #include "common_defs.h"
  #include "sgw_downlink_data_notification.h"
}

using namespace fluid_msg;

namespace openflow {

PagingApplication::PagingApplication(PacketInSwitchApplication& pin_sw_app) : pin_sw_app_(pin_sw_app) {};


void PagingApplication::packet_in_callback(const PacketInEvent& pin_ev,
    of13::PacketIn& ofpi,
    const OpenflowMessenger& messenger) {

  OAILOG_DEBUG(LOG_GTPV1U, "Handling packet-in message in paging app\n");
  trigger_dl_data_notification(pin_ev.get_connection(),
      static_cast<uint8_t*>(ofpi.data()),
      messenger);
}


void PagingApplication::event_callback(const ControllerEvent& ev,
                                       const OpenflowMessenger& messenger) {
  if (ev.get_type() == EVENT_PACKET_IN) {
    OAILOG_DEBUG(LOG_GTPV1U, "Handling packet-in message in paging app\n");
    const PacketInEvent& pi = static_cast<const PacketInEvent&>(ev);
    of13::PacketIn ofpi;
    ofpi.unpack(const_cast<uint8_t*>(pi.get_data()));

    trigger_dl_data_notification(ev.get_connection(),
        static_cast<uint8_t*>(ofpi.data()),
        messenger);

  }
  else if (ev.get_type() == EVENT_STOP_DL_DATA_NOTIFICATION) {
    const StopDLDataNotificationEvent& ce = static_cast<const StopDLDataNotificationEvent&>(ev);
    int pool_id = get_paa_ipv4_pool_id(ce.get_ue_ip());
    clamp_dl_data_notification(ev.get_connection(), messenger,
        ce.get_ue_ip(), pool_id, ce.get_time_out());
  }
  else if (ev.get_type() == EVENT_SWITCH_UP) {
    install_default_flow(ev.get_connection(), messenger);
    //install_test_flow(ev.get_connection(), messenger);
  }
}

void PagingApplication::clamp_dl_data_notification(
    fluid_base::OFConnection* ofconn,
    const OpenflowMessenger& messenger,
    const struct in_addr ue_ip,
    const int pool_id,
    const uint16_t clamp_time_out) {


#if DEBUG_IS_ON
  char* dest_ip_str = inet_ntoa(ue_ip);
  OAILOG_DEBUG(LOG_GTPV1U, "Throttle DL data notification for IP %s for %d seconds\n",
             dest_ip_str, clamp_time_out);
#endif

  /*
   * Clamp on this ip for configured amount of time
   * Priority is above default paging flow, but below gtp flow. This way when
   * paging succeeds, this flow will be ignored.
   * The clamping time is necessary to prevent packets from continually hitting
   * userspace, and as a retry time if paging fails
   */
  of13::FlowMod fm = messenger.create_default_flow_mod(OF_TABLE_PAGING_UE_IN_PROGRESS+pool_id, of13::OFPFC_ADD,
    OF_PRIO_PAGING_UE_IN_PROGRESS);
  fm.hard_timeout(clamp_time_out);
  of13::EthType type_match(IP_ETH_TYPE);
  fm.add_oxm_field(type_match);

  of13::IPv4Dst ip_match(ue_ip.s_addr);
  fm.add_oxm_field(ip_match);

  // No actions mean packet is dropped
  messenger.send_of_msg(fm, ofconn);
  return;
}

void PagingApplication::trigger_dl_data_notification(
    fluid_base::OFConnection* ofconn,
    uint8_t* data,
    const OpenflowMessenger& messenger) {
  // send paging request to MME
  struct ip* ip_header = (struct ip*) (data + ETH_HEADER_LENGTH);
  struct in_addr dest_ip;
  bstring imsi = NULL;
  memcpy(&dest_ip, &ip_header->ip_dst, sizeof(struct in_addr));

  sgw_notify_downlink_data(dest_ip, 0 /* TODO ebi */);

#if DEBUG_IS_ON
  char* dest_ip_str = inet_ntoa(dest_ip);
  OAILOG_DEBUG(LOG_GTPV1U, "Initiating paging procedure for IP %s\n",
             dest_ip_str);
#endif

  /*
   * Clamp on this ip for configured amount of time
   * Priority is above default paging flow, but below gtp flow. This way when
   * paging succeeds, this flow will be ignored.
   * The clamping time is necessary to prevent packets from continually hitting
   * userspace, and as a retry time if paging fails
   */
  int pool_id  = get_paa_ipv4_pool_id(dest_ip);

  if (RETURNerror != pool_id) {
    clamp_dl_data_notification(ofconn, messenger, dest_ip, pool_id, UNCONFIRMED_CLAMPING_TIMEOUT);
  } else {
#if !DEBUG_IS_ON
    char* dest_ip_str = inet_ntoa(dest_ip);
#endif
    OAILOG_NOTICE(LOG_GTPV1U, "Could not clamp DL Data notification for UE %s, unknown pool id\n",
               dest_ip_str);
  }
  return;
}

void PagingApplication::install_default_flow(
    fluid_base::OFConnection* ofconn,
    const OpenflowMessenger& messenger) {
  // Get assigned IP block from mobilityd
  struct in_addr netaddr;
  struct in_addr netmask;
  int      num_addr_pools = get_num_paa_ipv4_pool();

  uint64_t cookie = this->pin_sw_app_.generate_cookie();
  this->pin_sw_app_.register_for_cookie(this, cookie);

  for (int pidx = 0; pidx < num_addr_pools; pidx++) {
    int ret = get_paa_ipv4_pool(pidx, NULL, NULL, &netaddr, &netmask, NULL);
    // Convert to string for logging
    char ip_str[INET_ADDRSTRLEN];
    char net_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(netaddr.s_addr), ip_str, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(netmask.s_addr), net_str, INET_ADDRSTRLEN);
    OAILOG_INFO(LOG_GTPV1U,
                "Setting default paging flow for UE IP block %s/%s\n",
                ip_str, net_str);

    of13::FlowMod fm = messenger.create_default_flow_mod(OF_TABLE_PAGING_POOL+pidx, of13::OFPFC_ADD,
        OF_PRIO_PAGING_UE_POOL);

    fm.cookie(cookie);

    // IP eth type
    of13::EthType type_match(IP_ETH_TYPE);
    fm.add_oxm_field(type_match);

    // Match on ip dest equalling assigned ue ip block
    of13::IPv4Dst ip_match(netaddr.s_addr, netmask.s_addr);
    fm.add_oxm_field(ip_match);

    // Output to controller
    of13::OutputAction act(of13::OFPP_CONTROLLER, of13::OFPCML_NO_BUFFER);
    of13::ApplyActions inst;
    inst.add_action(act);
    fm.add_instruction(inst);

    messenger.send_of_msg(fm, ofconn);
  }
}

//void PagingApplication::install_test_flow(
//    fluid_base::OFConnection* ofconn,
//    const OpenflowMessenger& messenger) {
//
//    of13::FlowMod fm = messenger.create_default_flow_mod(TABLE, of13::OFPFC_ADD,
//                                                              PAGING_POOL_PRIORITY);
//
//    // Output to controller
//    of13::OutputAction act(of13::OFPP_LOCAL, of13::OFPCML_NO_BUFFER);
//    of13::ApplyActions inst;
//    inst.add_action(act);
//    fm.add_instruction(inst);
//
//    messenger.send_of_msg(fm, ofconn);
//}

}

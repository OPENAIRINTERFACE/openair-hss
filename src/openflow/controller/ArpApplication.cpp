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
#include <arpa/inet.h>

#include "ArpApplication.h"

extern "C" {
  #include "log.h"
  #include "spgw_config.h"
}

using namespace fluid_msg;

namespace openflow {
    

ArpApplication::ArpApplication(PacketInSwitchApplication& pin_sw_app, const int in_port, const std::string l2,  const struct in_addr l3) :
    pin_sw_app_(pin_sw_app), in_port_(in_port), l2_(l2), l3_(l3) {};


void ArpApplication::packet_in_callback(const PacketInEvent& pin_ev,
    of13::PacketIn& ofpi,
    const OpenflowMessenger& messenger) {
  OAILOG_DEBUG(LOG_GTPV1U, "Handling packet-in message in arp app\n");
  size_t size = pin_ev.get_length();
  const uint8_t *data = reinterpret_cast<const uint8_t*>(pin_ev.get_data());
  const uint8_t *eth_frame = &data[42];

  // Fill ARP REPLY HERE
  const ethhdr_t *ethhdr = reinterpret_cast<const ethhdr_t*>(eth_frame);
  const ether_arp_t *arp = reinterpret_cast<const ether_arp_t*>(&eth_frame[ETH_HEADER_LENGTH]);

  OAILOG_DEBUG(LOG_GTPV1U, "PACKET_IN 1/3: ARP REQUEST ETH SRC %02X:%02X:%02X:%02X:%02X:%02X ETH DST %02X:%02X:%02X:%02X:%02X:%02X PROTO %04X\n",
      ethhdr->h_source[0],ethhdr->h_source[1],ethhdr->h_source[2],ethhdr->h_source[3],ethhdr->h_source[4],ethhdr->h_source[5],
      ethhdr->h_dest[0],ethhdr->h_dest[1],ethhdr->h_dest[2],ethhdr->h_dest[3],ethhdr->h_dest[4],ethhdr->h_dest[5],
      ntohs(ethhdr->h_proto));
  OAILOG_DEBUG(LOG_GTPV1U, "PACKET_IN 2/3: ARP REQUEST HW SRC %02X:%02X:%02X:%02X:%02X:%02X IP SRC %d.%d.%d.%d\n",
      arp->arp_sha[0], arp->arp_sha[1], arp->arp_sha[2], arp->arp_sha[3], arp->arp_sha[4], arp->arp_sha[5],
      arp->arp_spa[0], arp->arp_spa[1], arp->arp_spa[2], arp->arp_spa[3]);
  OAILOG_DEBUG(LOG_GTPV1U, "PACKET_IN 3/3: ARP REQUEST HW DST %02X:%02X:%02X:%02X:%02X:%02X IP DST %d.%d.%d.%d\n",
      arp->arp_tha[0], arp->arp_tha[1], arp->arp_tha[2], arp->arp_tha[3], arp->arp_tha[4], arp->arp_tha[5],
      arp->arp_tpa[0], arp->arp_tpa[1], arp->arp_tpa[2], arp->arp_tpa[3]);

  send_arp_reply(pin_ev, ofpi, messenger);
}


void ArpApplication::send_arp_reply(const PacketInEvent& pin_ev, of13::PacketIn& ofpi, const OpenflowMessenger& messenger) {

  size_t size = pin_ev.get_length();
  const uint8_t *data = reinterpret_cast<const uint8_t*>(pin_ev.get_data());
  const uint8_t *eth_frame = &data[42];

  const ethhdr_t *ethhdr = reinterpret_cast<const ethhdr_t*>(eth_frame);
  const ether_arp_t *arp = reinterpret_cast<const ether_arp_t*>(&eth_frame[ETH_HEADER_LENGTH]);

  of13::FlowMod arp_fm = messenger.create_default_flow_mod(
      OF_TABLE_ARP,
      of13::OFPFC_ADD,
      OF_PRIO_ARP_IF);


  arp_fm.idle_timeout(180);
  arp_fm.hard_timeout(360);

  of13::InPort port_match(in_port_);
  arp_fm.add_oxm_field(port_match);

  of13::EthType type_match(ARP_ETH_TYPE);
  arp_fm.add_oxm_field(type_match);

  of13::ARPTPA tpa(l3_);
  arp_fm.add_oxm_field(tpa);

  // may be omitted
  of13::ARPTHA tha(EthAddress("0:0:0:0:0:0"));
  arp_fm.add_oxm_field(tha);

  of13::ApplyActions apply_inst;

  of13::SetFieldAction set_eth_src(new of13::EthSrc(l2_));
  apply_inst.add_action(set_eth_src);

  of13::SetFieldAction set_eth_dst(new of13::EthDst(ethhdr->h_source));
  apply_inst.add_action(set_eth_dst);

  of13::SetFieldAction set_arp_op(new of13::ARPOp(ARPOP_REPLY));
  apply_inst.add_action(set_arp_op);

  of13::SetFieldAction set_arp_src_hw(new of13::ARPSHA(l2_));
  apply_inst.add_action(set_arp_src_hw);

  of13::SetFieldAction set_arp_dst_hw(new of13::ARPTHA(ethhdr->h_source));
  apply_inst.add_action(set_arp_dst_hw);

  of13::SetFieldAction set_arp_src_pro(new of13::ARPSPA(l3_));
  apply_inst.add_action(set_arp_src_pro);

  of13::SetFieldAction set_arp_dst_pro(new of13::ARPTPA(arp->arp_spa));
  apply_inst.add_action(set_arp_dst_pro);

  // Output to controller
  of13::OutputAction act(of13::OFPP_IN_PORT, of13::OFPCML_NO_BUFFER);
  apply_inst.add_action(act);
  arp_fm.add_instruction(apply_inst);

  // Finally, send flow mod
  messenger.send_of_msg(arp_fm, pin_ev.get_connection());
  OAILOG_DEBUG(LOG_GTPV1U, "Arp flow added\n");
}

void ArpApplication::event_callback(const ControllerEvent& ev,
                                       const OpenflowMessenger& messenger) {
  if (ev.get_type() == EVENT_SWITCH_UP) {
    install_switch_arp_flow(ev.get_connection(), messenger);
    install_arp_flow(ev.get_connection(), messenger);
  }
}


void ArpApplication::install_switch_arp_flow(fluid_base::OFConnection* ofconn,
    const OpenflowMessenger& messenger) {

  of13::FlowMod fm = messenger.create_default_flow_mod(OF_TABLE_SWITCH, of13::OFPFC_ADD, OF_PRIO_SWITCH_GOTO_ARP_TABLE);

  // ARP eth type
  of13::EthType type_match(ARP_ETH_TYPE);
  fm.add_oxm_field(type_match);

  of13::ARPOp op_match(ARPOP_REQUEST);
  fm.add_oxm_field(op_match);

  // Output to next table
  of13::GoToTable inst(OF_TABLE_ARP);
  fm.add_instruction(inst);

  OAILOG_INFO(LOG_GTPV1U, "Setting switch flow for ARP Application\n");
   messenger.send_of_msg(fm, ofconn);
}

void ArpApplication::install_arp_flow(
    fluid_base::OFConnection* ofconn,
    const OpenflowMessenger& messenger) {


  // TODO: remove the sent to controller, install mod flow here

  uint64_t cookie = this->pin_sw_app_.generate_cookie();
  this->pin_sw_app_.register_for_cookie(this, cookie);

  of13::FlowMod fm = messenger.create_default_flow_mod(OF_TABLE_ARP, of13::OFPFC_ADD, 0);

  fm.cookie(cookie);

  of13::InPort port_match(in_port_);
  fm.add_oxm_field(port_match);

  // IP eth type
  of13::EthType type_match(ARP_ETH_TYPE);
  fm.add_oxm_field(type_match);

  of13::ARPTPA ip_match(l3_, std::string("255.255.255.255"));
  fm.add_oxm_field(ip_match);

  // Output to controller
  of13::OutputAction act(of13::OFPP_CONTROLLER, of13::OFPCML_NO_BUFFER);
  of13::ApplyActions inst;
  inst.add_action(act);
  fm.add_instruction(inst);

  OAILOG_INFO(LOG_GTPV1U, "Setting arp flow for ARP Application cookie %ld\n", cookie);
  messenger.send_of_msg(fm, ofconn);

}


}

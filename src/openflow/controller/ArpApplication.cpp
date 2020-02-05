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

#include <arpa/inet.h>
#include <netinet/ip.h>

#include "ArpApplication.h"

extern "C" {
#include "log.h"
#include "pgw_lite_paa.h"
#include "spgw_config.h"
}

using namespace fluid_msg;

namespace openflow {

//------------------------------------------------------------------------------
ArpApplication::ArpApplication(
    PacketInSwitchApplication& pin_sw_app, const int in_port,
    const std::string l2, const struct in_addr l3,
    const sgi_arp_boot_cache_t* const sgi_arp_boot_cache)
    : pin_sw_app_(pin_sw_app),
      in_port_(in_port),
      l2_(l2),
      l3_(l3),
      sgi_arp_boot_cache_(sgi_arp_boot_cache){};

//------------------------------------------------------------------------------
void ArpApplication::packet_in_callback(const PacketInEvent& pin_ev,
                                        of13::PacketIn& ofpi,
                                        const OpenflowMessenger& messenger) {
  size_t size = pin_ev.get_length();
  const uint8_t* data = reinterpret_cast<const uint8_t*>(pin_ev.get_data());
  const uint8_t* eth_frame = &data[42];

  // Fill ARP REPLY HERE
  const ethhdr_t* ethhdr = reinterpret_cast<const ethhdr_t*>(eth_frame);
  const ether_arp_t* arp =
      reinterpret_cast<const ether_arp_t*>(&eth_frame[ETH_HEADER_LENGTH]);

  if (ARPOP_REQUEST == ntohs(arp->ea_hdr.ar_op)) {
    // OAILOG_DEBUG(LOG_GTPV1U, "Handling packet-in message in arp app:
    // ARPOP_REQUEST\n");
    learn_neighbour_from_arp_request(pin_ev, messenger, arp);

    unsigned char buf_in_addr[sizeof(struct in6_addr)];
    char buf_ip_addr[INET6_ADDRSTRLEN];
    char buf_eth_addr[6 * 2 + 5 + 1];
    struct in_addr inaddr;

    // source host
    if (snprintf(buf_ip_addr, INET6_ADDRSTRLEN, "%d.%d.%d.%d", arp->arp_tpa[0],
                 arp->arp_tpa[1], arp->arp_tpa[2], arp->arp_tpa[3]) >= 7) {
      if (inet_pton(AF_INET, buf_ip_addr, buf_in_addr) == 1) {
        memcpy(&inaddr, buf_in_addr, sizeof(struct in_addr));
        if (spgw_config.pgw_config.arp_ue_oai) {
          if (get_paa_ipv4_pool_id(inaddr) >= 0) {
            // OAILOG_DEBUG(LOG_GTPV1U, "send_arp_reply() (target UE %s) port
            // %d\n", buf_ip_addr, in_port_);
            flow_mod_arp_reply(pin_ev, ofpi, messenger, inaddr);
            send_arp_reply(ofpi, pin_ev.get_connection(), in_port_, inaddr);
          } else {
            OAILOG_DEBUG(LOG_GTPV1U, "Ignoring ARP request for %s\n",
                         buf_ip_addr);
          }
        }
        if (inaddr.s_addr == l3_.s_addr) {
          // OAILOG_DEBUG(LOG_GTPV1U, "send_arp_reply() (SGi %s) port %d\n",
          // buf_ip_addr, in_port_);
          flow_mod_arp_reply(pin_ev, ofpi, messenger, inaddr);
          send_arp_reply(ofpi, pin_ev.get_connection(), in_port_, inaddr);
        }
      } else {
        OAILOG_DEBUG(LOG_GTPV1U, "Error in inet_pton()\n");
      }
    } else {
      OAILOG_DEBUG(LOG_GTPV1U, "Error in snprintf()\n");
    }
  } else if (ARPOP_REPLY == ntohs(arp->ea_hdr.ar_op)) {
    // source host
    unsigned char buf_in_addr[sizeof(struct in6_addr)];
    char buf_ip_addr[INET6_ADDRSTRLEN];
    struct in_addr inaddr;
    if (snprintf(buf_ip_addr, INET6_ADDRSTRLEN, "%d.%d.%d.%d", arp->arp_spa[0],
                 arp->arp_spa[1], arp->arp_spa[2], arp->arp_spa[3]) >= 7) {
      if (inet_pton(AF_INET, buf_ip_addr, buf_in_addr) == 1) {
        memcpy(&inaddr, buf_in_addr, sizeof(struct in_addr));
        if (spgw_config.pgw_config.arp_ue_oai) {
          if (get_paa_ipv4_pool_id(inaddr) < 0) {
            OAILOG_DEBUG(
                LOG_GTPV1U,
                "TODO: Smash out packet-in message in arp app: ARPOP_REPLY "
                "(Can happen if Action TABLE is used for sending ARP reply)\n");
          } else {
            // OAILOG_DEBUG(LOG_GTPV1U, "Handling packet-in message in arp app:
            // ARPOP_REPLY\n");
            learn_neighbour_from_arp_reply(pin_ev, messenger);
          }
        } else {
          learn_neighbour_from_arp_reply(pin_ev, messenger);
        }
      }
    } else {
      OAILOG_DEBUG(LOG_GTPV1U, "Error in snprintf()\n");
    }
  }
}

//------------------------------------------------------------------------------
void ArpApplication::send_arp_reply(of13::PacketIn& pi,
                                    fluid_base::OFConnection* ofconn,
                                    uint32_t in_port, struct in_addr& spa) {
  uint8_t* buf;
  of13::PacketOut po(pi.xid(), pi.buffer_id(), in_port);

  /*Add Packet in data if the packet was not buffered*/
  if (pi.buffer_id() == -1) {
    // OAILOG_DEBUG(LOG_GTPV1U, "send_arp_reply() packet was not buffered\n");
    po.data(pi.data(), pi.data_len());
  }

  uint8_t* data_in = reinterpret_cast<uint8_t*>(pi.data());
  uint8_t* eth_frame_in = &data_in[0];

  ethhdr_t* const ethhdr_in = reinterpret_cast<ethhdr_t*>(eth_frame_in);
  ether_arp_t* const arp_in =
      reinterpret_cast<ether_arp_t*>(&eth_frame_in[ETH_HEADER_LENGTH]);

  ActionList action_list;

  of13::SetFieldAction set_eth_src(new of13::EthSrc(l2_));
  action_list.add_action(set_eth_src);

  of13::SetFieldAction set_eth_dst(new of13::EthDst(ethhdr_in->h_source));
  action_list.add_action(set_eth_dst);

  of13::SetFieldAction set_arp_op(new of13::ARPOp(ARPOP_REPLY));
  action_list.add_action(set_arp_op);

  of13::SetFieldAction set_arp_src_hw(new of13::ARPSHA(l2_));
  action_list.add_action(set_arp_src_hw);

  of13::SetFieldAction set_arp_dst_hw(new of13::ARPTHA(ethhdr_in->h_source));
  action_list.add_action(set_arp_dst_hw);

  of13::SetFieldAction set_arp_src_pro(new of13::ARPSPA(spa));
  action_list.add_action(set_arp_src_pro);

  of13::SetFieldAction set_arp_dst_pro(new of13::ARPTPA(arp_in->arp_spa));
  action_list.add_action(set_arp_dst_pro);

  // of13::OutputAction act(of13::OFPP_TABLE, of13::OFPCML_NO_BUFFER); // = new
  // of13::OutputAction();
  of13::OutputAction act(of13::OFPP_IN_PORT, of13::OFPCML_NO_BUFFER);
  action_list.add_action(act);
  po.actions(action_list);

  buf = po.pack();

  ofconn->send(buf, po.length());
  OFMsg::free_buffer(buf);
}

//------------------------------------------------------------------------------
void ArpApplication::flow_mod_arp_reply(const PacketInEvent& pin_ev,
                                        of13::PacketIn& ofpi,
                                        const OpenflowMessenger& messenger,
                                        struct in_addr& spa) {
  size_t size = pin_ev.get_length();
  const uint8_t* data = reinterpret_cast<const uint8_t*>(pin_ev.get_data());
  const uint8_t* eth_frame = &data[42];

  const ethhdr_t* ethhdr = reinterpret_cast<const ethhdr_t*>(eth_frame);
  const ether_arp_t* arp =
      reinterpret_cast<const ether_arp_t*>(&eth_frame[ETH_HEADER_LENGTH]);

  //OAILOG_DEBUG(LOG_GTPV1U, "PACKET_IN 1/3: ARP REQUEST ETH SRC %02X:%02X:%02X:%02X:%02X:%02X ETH DST %02X:%02X:%02X:%02X:%02X:%02X PROTO %04X\n", \
    ethhdr->h_source[0],ethhdr->h_source[1],ethhdr->h_source[2],ethhdr->h_source[3],ethhdr->h_source[4],ethhdr->h_source[5], \
    ethhdr->h_dest[0],ethhdr->h_dest[1],ethhdr->h_dest[2],ethhdr->h_dest[3],ethhdr->h_dest[4],ethhdr->h_dest[5], \
    ntohs(ethhdr->h_proto));
  //OAILOG_DEBUG(LOG_GTPV1U, "PACKET_IN 2/3: ARP REQUEST HW SRC %02X:%02X:%02X:%02X:%02X:%02X IP SRC %d.%d.%d.%d\n", \
    arp->arp_sha[0], arp->arp_sha[1], arp->arp_sha[2], arp->arp_sha[3], arp->arp_sha[4], arp->arp_sha[5], \
    arp->arp_spa[0], arp->arp_spa[1], arp->arp_spa[2], arp->arp_spa[3]);
  //OAILOG_DEBUG(LOG_GTPV1U, "PACKET_IN 3/3: ARP REQUEST HW DST %02X:%02X:%02X:%02X:%02X:%02X IP DST %d.%d.%d.%d\n", \
    arp->arp_tha[0], arp->arp_tha[1], arp->arp_tha[2], arp->arp_tha[3], arp->arp_tha[4], arp->arp_tha[5], \
    arp->arp_tpa[0], arp->arp_tpa[1], arp->arp_tpa[2], arp->arp_tpa[3]);

  of13::FlowMod arp_fm = messenger.create_default_flow_mod(
      OF_TABLE_ARP, of13::OFPFC_ADD, OF_PRIO_ARP_IF);

  arp_fm.idle_timeout(360);
  arp_fm.hard_timeout(0);

  of13::InPort port_match(in_port_);
  arp_fm.add_oxm_field(port_match);

  of13::EthType type_match(ARP_ETH_TYPE);
  arp_fm.add_oxm_field(type_match);

  of13::ARPTPA tpa(spa);
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

  of13::SetFieldAction set_arp_src_pro(new of13::ARPSPA(spa));
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

//------------------------------------------------------------------------------
void ArpApplication::event_callback(const ControllerEvent& ev,
                                    const OpenflowMessenger& messenger) {
  if (ev.get_type() == EVENT_SWITCH_UP) {
    install_switch_arp_flow(ev.get_connection(), messenger);
    install_arp_flow(ev.get_connection(), messenger);
    add_default_sgi_out_flow(ev.get_connection(),
                             messenger);  // no update of l2 dest addr
    add_sgi_arp_cache_out_flow(ev.get_connection(), messenger);
  }
}

//------------------------------------------------------------------------------
void ArpApplication::install_switch_arp_flow(
    fluid_base::OFConnection* ofconn, const OpenflowMessenger& messenger) {
  of13::FlowMod fm = messenger.create_default_flow_mod(
      OF_TABLE_SWITCH, of13::OFPFC_ADD, OF_PRIO_SWITCH_GOTO_ARP_TABLE);

  // ARP eth type
  of13::EthType type_match(ARP_ETH_TYPE);
  fm.add_oxm_field(type_match);

  // Output to next table
  of13::GoToTable inst(OF_TABLE_ARP);
  fm.add_instruction(inst);

  OAILOG_INFO(LOG_GTPV1U, "Setting switch flow for ARP Application\n");
  messenger.send_of_msg(fm, ofconn);
}

//------------------------------------------------------------------------------
void ArpApplication::install_arp_flow(fluid_base::OFConnection* ofconn,
                                      const OpenflowMessenger& messenger) {
  // TODO: remove the sent to controller, install mod flow here

  uint64_t cookie = this->pin_sw_app_.generate_cookie();
  this->pin_sw_app_.register_for_cookie(this, cookie);

  of13::FlowMod fm = messenger.create_default_flow_mod(
      OF_TABLE_ARP, of13::OFPFC_ADD, OF_PRIO_ARP_LOWER_PRIORITY);

  fm.cookie(cookie);

  of13::InPort port_match(in_port_);
  fm.add_oxm_field(port_match);

  // IP eth type
  of13::EthType type_match(ARP_ETH_TYPE);
  fm.add_oxm_field(type_match);

  // Output to controller
  of13::OutputAction act(of13::OFPP_CONTROLLER, 1024);
  of13::ApplyActions inst;
  inst.add_action(act);
  fm.add_instruction(inst);

  OAILOG_INFO(LOG_GTPV1U, "Setting arp flow for ARP Application cookie %ld\n",
              cookie);
  messenger.send_of_msg(fm, ofconn);
}

//------------------------------------------------------------------------------
void ArpApplication::add_default_sgi_out_flow(
    fluid_base::OFConnection* ofconn, const OpenflowMessenger& messenger) {
  of13::FlowMod fm = messenger.create_default_flow_mod(
      OF_TABLE_SGI_OUT, of13::OFPFC_ADD, OF_PRIO_SGI_OUT_LOWER_PRIORITY);
  fm.buffer_id(0xffffffff);
  fm.out_port(in_port_);
  fm.out_group(0);
  fm.flags(0);

  of13::ApplyActions apply_inst;
  fluid_msg::of13::OutputAction act(in_port_, 1024);
  apply_inst.add_action(act);
  fm.add_instruction(apply_inst);

  uint8_t* buffer = fm.pack();
  ofconn->send(buffer, fm.length());
  fluid_msg::OFMsg::free_buffer(buffer);

  OAILOG_DEBUG(LOG_GTPV1U, "Default SGi out flow added\n");
}

//------------------------------------------------------------------------------
void ArpApplication::add_sgi_arp_cache_out_flow(
    fluid_base::OFConnection* ofconn, const OpenflowMessenger& messenger) {
  if (sgi_arp_boot_cache_) {
    for (int i = 0; i < sgi_arp_boot_cache_->num_entries; i++) {
      std::string mac(bdata(sgi_arp_boot_cache_->mac[i]));
      add_update_dst_l2_flow(ofconn, messenger, sgi_arp_boot_cache_->ip[i],
                             mac);
    }
  }
}

//------------------------------------------------------------------------------
void ArpApplication::add_update_dst_l2_flow(
    fluid_base::OFConnection* of_connexion, const OpenflowMessenger& messenger,
    const struct in_addr& dst_addr, const std::string dst_mac) {
  OAILOG_DEBUG(LOG_GTPV1U, "Egress: Update MAC dest for %s -> %s\n",
               inet_ntoa(dst_addr), dst_mac.c_str());

  of13::FlowMod fm = messenger.create_default_flow_mod(
      OF_TABLE_SGI_OUT, of13::OFPFC_ADD, OF_PRIO_SGI_OUT_PRIORITY);
  fm.buffer_id(0xffffffff);
  fm.out_port(in_port_);
  fm.out_group(0);
  fm.flags(0);

  of13::EthType type_match(IP_ETH_TYPE);
  fm.add_oxm_field(type_match);

  of13::IPv4Dst ip_dst_match(dst_addr.s_addr);
  fm.add_oxm_field(ip_dst_match);

  of13::ApplyActions apply_inst;

  EthAddress dst_address(dst_mac);
  of13::SetFieldAction set_eth_dst(new of13::EthDst(dst_address));
  apply_inst.add_action(set_eth_dst);

  fluid_msg::of13::OutputAction act(in_port_, 1024);
  apply_inst.add_action(act);
  fm.add_instruction(apply_inst);

  messenger.send_of_msg(fm, of_connexion);
}

//------------------------------------------------------------------------------
void ArpApplication::learn_neighbour_from_arp_reply(
    const PacketInEvent& pin_ev, const OpenflowMessenger& messenger) {
  unsigned char buf_in_addr[sizeof(struct in6_addr)];
  char buf_ip_addr[INET6_ADDRSTRLEN];
  char buf_eth_addr[6 * 2 + 5 + 1];
  struct in_addr inaddr;

  const uint8_t* data = reinterpret_cast<const uint8_t*>(pin_ev.get_data());
  const uint8_t* eth_frame = &data[42];

  const ethhdr_t* ethhdr = reinterpret_cast<const ethhdr_t*>(eth_frame);
  const ether_arp_t* arp =
      reinterpret_cast<const ether_arp_t*>(&eth_frame[ETH_HEADER_LENGTH]);

  //OAILOG_DEBUG(LOG_GTPV1U, "Learning from ARP REPLY %d.%d.%d.%d -> %d.%d.%d.%d\n", \
      arp->arp_spa[0], arp->arp_spa[1], arp->arp_spa[2], arp->arp_spa[3], \
      arp->arp_tpa[0], arp->arp_tpa[1], arp->arp_tpa[2], arp->arp_tpa[3]);

  //OAILOG_DEBUG(LOG_GTPV1U, "Learning from ARP REPLY HW SRC %02X:%02X:%02X:%02X:%02X:%02X -> %02X:%02X:%02X:%02X:%02X:%02X\n", \
      arp->arp_sha[0], arp->arp_sha[1], arp->arp_sha[2], arp->arp_sha[3], arp->arp_sha[4], arp->arp_sha[5], \
      arp->arp_tha[0], arp->arp_tha[1], arp->arp_tha[2], arp->arp_tha[3], arp->arp_tha[4], arp->arp_tha[5]);

  // source host
  if (snprintf(buf_ip_addr, INET6_ADDRSTRLEN, "%d.%d.%d.%d", arp->arp_spa[0],
               arp->arp_spa[1], arp->arp_spa[2], arp->arp_spa[3]) >= 7) {
    if (inet_pton(AF_INET, buf_ip_addr, buf_in_addr) == 1) {
      memcpy(&inaddr, buf_in_addr, sizeof(struct in_addr));

      if (snprintf(buf_eth_addr, sizeof(buf_eth_addr),
                   "%02X:%02X:%02X:%02X:%02X:%02X", arp->arp_sha[0],
                   arp->arp_sha[1], arp->arp_sha[2], arp->arp_sha[3],
                   arp->arp_sha[4], arp->arp_sha[5]) > 0) {
        std::string mac(buf_eth_addr);
        // populate or update
        update_neighbours(pin_ev, messenger, inaddr.s_addr, mac);
      }
    }
  }

  // Destination host
  if (snprintf(buf_ip_addr, INET6_ADDRSTRLEN, "%d.%d.%d.%d", arp->arp_tpa[0],
               arp->arp_tpa[1], arp->arp_tpa[2], arp->arp_tpa[3]) >= 7) {
    if (inet_pton(AF_INET, buf_ip_addr, buf_in_addr) == 1) {
      memcpy(&inaddr, buf_in_addr, sizeof(struct in_addr));

      if (snprintf(buf_eth_addr, sizeof(buf_eth_addr),
                   "%02X:%02X:%02X:%02X:%02X:%02X", arp->arp_tha[0],
                   arp->arp_tha[1], arp->arp_tha[2], arp->arp_tha[3],
                   arp->arp_tha[4], arp->arp_tha[5]) > 0) {
        std::string mac(buf_eth_addr);
        // populate or update
        update_neighbours(pin_ev, messenger, inaddr.s_addr, mac);
      }
    }
  }
}

//------------------------------------------------------------------------------
void ArpApplication::learn_neighbour_from_arp_request(
    const PacketInEvent& pin_ev, const OpenflowMessenger& messenger,
    const ether_arp_t* const arp) {
  unsigned char buf_in_addr[sizeof(struct in6_addr)];
  char buf_ip_addr[INET6_ADDRSTRLEN];
  char buf_eth_addr[6 * 2 + 5 + 1];
  struct in_addr inaddr;

  // source host
  if (snprintf(buf_ip_addr, INET6_ADDRSTRLEN, "%d.%d.%d.%d", arp->arp_spa[0],
               arp->arp_spa[1], arp->arp_spa[2], arp->arp_spa[3]) >= 7) {
    if (inet_pton(AF_INET, buf_ip_addr, buf_in_addr) == 1) {
      memcpy(&inaddr, buf_in_addr, sizeof(struct in_addr));

      if (snprintf(buf_eth_addr, sizeof(buf_eth_addr),
                   "%02X:%02X:%02X:%02X:%02X:%02X", arp->arp_sha[0],
                   arp->arp_sha[1], arp->arp_sha[2], arp->arp_sha[3],
                   arp->arp_sha[4], arp->arp_sha[5]) > 0) {
        std::string mac(buf_eth_addr);
        // populate or update
        update_neighbours(pin_ev, messenger, inaddr.s_addr, mac);
      }
    }
  }
}

//------------------------------------------------------------------------------
void ArpApplication::update_neighbours(const PacketInEvent& pin_ev,
                                       const OpenflowMessenger& messenger,
                                       in_addr_t l3, std::string l2) {
  std::string mac = learning_arp[l3];
  if (l2.compare(mac)) {
    learning_arp[l3] = l2;
    struct in_addr dst_addr;
    dst_addr.s_addr = l3;
    add_update_dst_l2_flow(pin_ev.get_connection(), messenger, dst_addr, l2);
  }
}

}  // namespace openflow

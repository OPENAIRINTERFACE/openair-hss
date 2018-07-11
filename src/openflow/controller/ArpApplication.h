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

#pragma once
#include <memory>
#include <unordered_map>

#include "OpenflowController.h"
#include "PacketInSwitchApplication.h"

namespace openflow {
#define ETH_ALEN           6
#define ETH_P_ARP 0x0806    /* Address Resolution packet  */
#define ETH_HEADER_LENGTH 14
#define ARP_REPLY_LENGTH  28
#define ARPHRD_ETHER  1   /* Ethernet 10/100Mbps.  */
#define ETH_P_IP  0x0800    /* Internet Protocol packet */

typedef struct __attribute__((__packed__)) ethhdr {
  unsigned char h_dest[ETH_ALEN]; /* destination eth addr */
  unsigned char h_source[ETH_ALEN]; /* source ether addr  */
  uint16_t    h_proto;    /* packet type ID field  big endian */
} ethhdr_t;

typedef struct __attribute__((__packed__)) arphdr {
  unsigned short int ar_hrd;    /* Format of hardware address.  */
  unsigned short int ar_pro;    /* Format of protocol address.  */
  unsigned char ar_hln;   /* Length of hardware address.  */
  unsigned char ar_pln;   /* Length of protocol address.  */
  unsigned short int ar_op;   /* ARP opcode (command).  */
} arphdr_t;

typedef struct __attribute__((__packed__)) ether_arp {
  struct  arphdr ea_hdr;    /* fixed-size header */
  u_int8_t arp_sha[ETH_ALEN]; /* sender hardware address */
  u_int8_t arp_spa[4];    /* sender protocol address */
  u_int8_t arp_tha[ETH_ALEN]; /* target hardware address */
  u_int8_t arp_tpa[4];    /* target protocol address */
} ether_arp_t;


class ArpApplication: public PacketInApplication {

public:
  ArpApplication(PacketInSwitchApplication& pin_sw_app, const int in_port, const std::string l2, const struct in_addr l3);

private:

  void packet_in_callback(const PacketInEvent& pin_ev,
      of13::PacketIn& ofpi,
      const OpenflowMessenger& messenger);

  void  send_arp_reply(const PacketInEvent& pi, of13::PacketIn& ofpi, const OpenflowMessenger& messenger, struct in_addr& spa);


  /**
   * Main callback event required by inherited Application class. Whenever
   * the controller gets an event like packet in or switch up, it will pass
   * it to the application here
   *
   * @param ev (in) - pointer to some subclass of ControllerEvent that occurred
   */
  void event_callback(const ControllerEvent& ev,
                              const OpenflowMessenger& messenger);

  /**
   * Handles downlink data intended for a UE in idle mode, then forwards the
   * paging request to SPGW. After initiating the paging process, it also clamps
   * on the destination IP, to prevent multiple packet-in messages
   *
   * @param ofconn (in) - given connection to OVS switch
   * @param data (in) - the ethernet packet received by the switch
   */
  void handle_arp_request_message(fluid_base::OFConnection* ofconn, uint8_t* data,
                             const OpenflowMessenger& messenger);

  /**
   * Creates the flow
   */
  void install_switch_arp_flow(fluid_base::OFConnection* ofconn,
      const OpenflowMessenger& messenger);

  void install_arp_flow(fluid_base::OFConnection* ofconn,
      const OpenflowMessenger& messenger);

  void add_default_sgi_out_flow(
      fluid_base::OFConnection* ofconn,
      const OpenflowMessenger& messenger);

  void add_update_dst_l2_flow(const PacketInEvent& pin_ev,
      const OpenflowMessenger& messenger,
      const struct in_addr& dst_addr,
      const std::string dst_mac);

  void learn_neighbour_from_arp_reply(const PacketInEvent& pin_ev,
      const OpenflowMessenger& messenger, const ether_arp_t * const ether_arp);

  void learn_neighbour_from_arp_request(const PacketInEvent& pin_ev,
      const OpenflowMessenger& messenger, const ether_arp_t * const ether_arp);

  void update_neighbours(const PacketInEvent& pin_ev,
      const OpenflowMessenger& messenger,
      in_addr_t l3,
      std::string l2);

  PacketInSwitchApplication& pin_sw_app_;
  const int in_port_; // ARP coming in through this port
  struct in_addr l3_; // IP address of network device linked to this port
  std::string l2_;    // L2 address of network device linked to this port
  //uint8_t arp_ha_[ETH_ALEN];
  //uint8_t arp_pa_[4];
  std::unordered_map<in_addr_t, std::string> learning_arp;
  std::set<in_addr_t, std::string> ue_arp;
};

}

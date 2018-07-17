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

#include <gmp.h> // gross but necessary to link spgw_config.h

#include "OpenflowController.h"

namespace openflow {

/**
 * GTPApplication handles external callbacks to add/delete tunnel flows for a
 * UE when it connects
 */
class GTPApplication: public Application {
public:
  GTPApplication(
    const std::string& uplink_mac,
    const struct in_addr l3_ingress_port,
    const uint32_t gtp_port_num,
    const struct in_addr l3_egress_port,
    const std::string l2_egress_port,
    const uint32_t egress_port_num);
private:

  /**
   * Main callback event required by inherited Application class. Whenever
   * the controller gets an event like packet in or switch up, it will pass
   * it to the application here
   *
   * @param ev (in) - pointer to some subclass of ControllerEvent that occurred
   */
  virtual void event_callback(const ControllerEvent& ev,
      const OpenflowMessenger& messenger);


  void install_switch_gtp_flow(fluid_base::OFConnection* ofconn,
      const OpenflowMessenger& messenger);
  /*
   * Add uplink flow from UE to internet
   * @param ev - AddGTPTunnelEvent containing ue ip, enb ip, and tunnel id's
   */
  void add_uplink_tunnel_flow(const AddGTPTunnelEvent& ev,
      const OpenflowMessenger& messenger);

  void add_downlink_match(of13::FlowMod& downlink_fm,
      const struct in_addr& ue_ip,
      const sdf_filter_t &sdf_filter);

  void delete_ue_paging_flow(
      const AddGTPTunnelEvent& ev,
      const OpenflowMessenger& messenger,
      const int pool_id);

  /*
   * Add downlink flow from internet to UE
   * @param ev - AddGTPTunnelEvent containing ue ip, enb ip, and tunnel id's
   */
  void add_downlink_tunnel_flow(const AddGTPTunnelEvent& ev,
      const OpenflowMessenger& messenger);

  /*
   * Remove uplink tunnel flow on disconnect
   * @param ev - DeleteGTPTunnelEvent containing ue ip, and inbound tei
   */
  void delete_uplink_tunnel_flow(const DeleteGTPTunnelEvent& ev,
      const OpenflowMessenger& messenger);

  /*
   * Remove downlink tunnel flow on disconnect
   * @param ev - DeleteGTPTunnelEvent containing ue ip, and inbound tei
   */
  void delete_downlink_tunnel_flow(const DeleteGTPTunnelEvent& ev,
      const OpenflowMessenger& messenger);

  void install_loop_flow(fluid_base::OFConnection* ofconn,
      const OpenflowMessenger& messenger);

  void add_sgi_out_flow(fluid_base::OFConnection* ofconn,
      const OpenflowMessenger& messenger);
  
  void add_pdn_loop(const AddGTPTunnelEvent& ev, 
      const OpenflowMessenger& messenger,
      const uint16_t flow_priority,
      const uint32_t goto_table);

  void delete_pdn_loop(
      const DeleteGTPTunnelEvent& ev,
      const OpenflowMessenger& messenger,
      const uint16_t flow_priority,
      const uint32_t goto_table);
private:
  static const std::string GTP_PORT_MAC;

  // ingress ~ S1-U for SGW, SPGW, S5 for PGW
  // egress  ~ SGi for SPGW, PGW, S5 for SGW
  const std::string uplink_mac_;
  const struct in_addr l3_ingress_port_;
  const uint32_t gtp_port_num_;
  const struct in_addr l3_egress_port_;
  const std::string l2_egress_port_;
  const uint32_t egress_port_num_;
};

}

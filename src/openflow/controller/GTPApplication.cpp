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
#include <string>
#include <iostream>

#include "GTPApplication.h"
#include "IMSIEncoder.h"
#include <fluid/of13/openflow-13.h>

extern "C" {
  #include "log.h"
  #include "bstrlib.h"
  #include "spgw_config.h"
  #include "pgw_lite_paa.h"
}

using namespace fluid_msg;

namespace openflow {

const std::string GTPApplication::GTP_PORT_MAC = "02:00:00:00:00:01";


GTPApplication::GTPApplication(
  const std::string& uplink_mac,
  const struct in_addr l3_ingress_port,
  const uint32_t gtp_port_num,
  const struct in_addr l3_egress_port,
  const std::string l2_egress_port,
  const uint32_t egress_port_num)
  : uplink_mac_(uplink_mac), l3_ingress_port_(l3_ingress_port), gtp_port_num_(gtp_port_num),
    l3_egress_port_(l3_egress_port), l2_egress_port_(l2_egress_port), egress_port_num_(egress_port_num) {}

void GTPApplication::event_callback(const ControllerEvent& ev,
                                    const OpenflowMessenger& messenger) {
  if (ev.get_type() == EVENT_PACKET_IN) {
    // TODO REMOVE
    OAILOG_DEBUG(LOG_GTPV1U, "Handling packet-in message in gtp app\n");
    const PacketInEvent& pi = static_cast<const PacketInEvent&>(ev);
    of13::PacketIn ofpi;
    ofpi.unpack(const_cast<uint8_t*>(pi.get_data()));
    size_t size = pi.get_length();
    OAILOG_STREAM_HEX(OAILOG_LEVEL_INFO, LOG_GTPV1U, "For Debug", (reinterpret_cast<const char*>(pi.get_data())), size);
  } else if (ev.get_type() == EVENT_ADD_GTP_TUNNEL) {
    auto add_tunnel_event = static_cast<const AddGTPTunnelEvent&>(ev);
    add_uplink_tunnel_flow(add_tunnel_event, messenger);
    add_downlink_tunnel_flow(add_tunnel_event, messenger);
  } else if (ev.get_type() == EVENT_DELETE_GTP_TUNNEL) {
    auto del_tunnel_event = static_cast<const DeleteGTPTunnelEvent&>(ev);
    delete_uplink_tunnel_flow(del_tunnel_event, messenger);
    delete_downlink_tunnel_flow(del_tunnel_event, messenger);
  } else if (ev.get_type() == EVENT_SWITCH_UP) {
    install_switch_gtp_flow(ev.get_connection(), messenger);
    install_loop_flow(ev.get_connection(), messenger);
  }
}



void GTPApplication::install_switch_gtp_flow(fluid_base::OFConnection* ofconn,
                                           const OpenflowMessenger& messenger) {

  int      num_addr_pools = get_num_paa_ipv4_pool();
  struct in_addr netaddr;
  struct in_addr netmask;
  const struct ipv4_list_elm_s *out_of_scope = NULL;

  for (int pidx = 0; pidx < num_addr_pools; pidx++) {
    int ret = get_paa_ipv4_pool(pidx, NULL, NULL, &netaddr, &netmask, &out_of_scope);

    //--------------------------------------------------------------------------
    // DL GTP flow
    //--------------------------------------------------------------------------
    of13::FlowMod fmd = messenger.create_default_flow_mod(OF_TABLE_SWITCH, of13::OFPFC_ADD, OF_PRIO_SWITCH_GOTO_DL_GTP_TABLE);
    of13::InPort egress_port_match(egress_port_num_);
    fmd.add_oxm_field(egress_port_match);

    // IP eth type
    of13::EthType type_match(IP_ETH_TYPE);
    fmd.add_oxm_field(type_match);

    // Match on ip dest equalling assigned ue ip block
    of13::IPv4Dst ip_match(netaddr.s_addr, netmask.s_addr);
    fmd.add_oxm_field(ip_match);

    // Output to next table
    of13::GoToTable dl_inst(OF_TABLE_DL_GTPU+pidx);
    fmd.add_instruction(dl_inst);
    char ip_str[INET_ADDRSTRLEN];
    char net_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(netaddr.s_addr), ip_str, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(netmask.s_addr), net_str, INET_ADDRSTRLEN);
    OAILOG_INFO(LOG_GTPV1U,
                "Setting switch flow for UE IP block %s/%s for DL in GTP Application\n",
                ip_str, net_str);
    messenger.send_of_msg(fmd, ofconn);

    //--------------------------------------------------------------------------
    // UL GTP flow
    //--------------------------------------------------------------------------
    {
      of13::FlowMod fmu = messenger.create_default_flow_mod(OF_TABLE_SWITCH, of13::OFPFC_ADD, OF_PRIO_SWITCH_GOTO_UL_GTP_TABLE);
      of13::InPort ingress_port_match(gtp_port_num_);
      fmu.add_oxm_field(ingress_port_match);

      // IP eth type
      of13::EthType type_match(IP_ETH_TYPE);
      fmu.add_oxm_field(type_match);

      of13::IPv4Src ip_src_match(netaddr.s_addr, netmask.s_addr);
      fmu.add_oxm_field(ip_src_match);

      // Output to next table
      of13::GoToTable ul_inst(OF_TABLE_UL_GTPU+pidx);
      fmu.add_instruction(ul_inst);
      OAILOG_INFO(LOG_GTPV1U,
                  "Setting switch flow for UE IP block %s/%s for UL in GTP Application\n",
                  ip_str, net_str);
      messenger.send_of_msg(fmu, ofconn);
    }
  }
}

void GTPApplication::install_loop_flow(fluid_base::OFConnection* ofconn,
                                           const OpenflowMessenger& messenger) {

  int      num_addr_pools = get_num_paa_ipv4_pool();
  struct in_addr netaddr;
  struct in_addr netmask;

  add_sgi_out_flow(ofconn, messenger);
}

/*
 * Helper method to add matching for adding/deleting the uplink flow
 */
void add_uplink_match(of13::FlowMod& uplink_fm, const uint32_t gtp_port,
                      uint32_t i_tei) {
  // Match on tunnel id and gtp in port
  of13::InPort gtp_port_match(gtp_port);
  uplink_fm.add_oxm_field(gtp_port_match);
  of13::TUNNELId in_tunnel_id(i_tei);
  uplink_fm.add_oxm_field(in_tunnel_id);
}

/*
 * Helper method to add imsi as metadata to the packet
 */
void add_imsi_metadata(of13::ApplyActions& apply_actions,
                       const std::string& imsi) {
  auto metadata_field = new of13::Metadata(IMSIEncoder::compact_imsi(imsi));
  of13::SetFieldAction set_metadata(metadata_field);
  apply_actions.add_action(set_metadata);
}

void GTPApplication::add_uplink_tunnel_flow(
    const AddGTPTunnelEvent& ev,
    const OpenflowMessenger& messenger) {
    
  if (INVALID_TEID != ev.get_in_tei()) {
    int pool_id  = get_paa_ipv4_pool_id(ev.get_ue_ip());
    OAILOG_INFO(LOG_GTPV1U, "add_uplink_tunnel_flow() found pool_id %d\n", pool_id);

    of13::FlowMod uplink_fm = messenger.create_default_flow_mod(
        OF_TABLE_UL_GTPU + pool_id,
      of13::OFPFC_ADD,
      OF_PRIO_GTPU);

    add_uplink_match(uplink_fm, gtp_port_num_, ev.get_in_tei());

    // Set eth src and dst
    of13::ApplyActions apply_ul_inst;
    EthAddress gtp_port(l2_egress_port_);
    // libfluid handles memory freeing of fields
    of13::SetFieldAction set_eth_src(new of13::EthSrc(gtp_port));
    apply_ul_inst.add_action(set_eth_src);

    EthAddress uplink_port(uplink_mac_);
    of13::SetFieldAction set_eth_dst(new of13::EthDst(uplink_port));
    apply_ul_inst.add_action(set_eth_dst);

    // add imsi to packet metadata to pass to other tables
    add_imsi_metadata(apply_ul_inst, ev.get_imsi());

    uplink_fm.add_instruction(apply_ul_inst);

    // Output to inout table
    of13::GoToTable goto_inst(OF_TABLE_LOOP);
    uplink_fm.add_instruction(goto_inst);

    // Finally, send flow mod
    messenger.send_of_msg(uplink_fm, ev.get_connection());
    OAILOG_DEBUG(LOG_GTPV1U, "Uplink flow added\n");
  }
#if DEBUG_IS_ON
  else {
    OAILOG_DEBUG(LOG_GTPV1U, "No Uplink flow added, cause invalid teid\n");
  }
#endif
}

void GTPApplication::add_sgi_out_flow(
    fluid_base::OFConnection* ofconn,
    const OpenflowMessenger& messenger) {
  of13::FlowMod fm = messenger.create_default_flow_mod(
    OF_TABLE_LOOP,
    of13::OFPFC_ADD,
    OF_PRIO_LOOP_LOWER_PRIORITY);
  fm.buffer_id(0xffffffff);
  fm.out_port(egress_port_num_);
  fm.out_group(0);
  fm.flags(0);

  of13::GoToTable inst(OF_TABLE_SGI_OUT);
  fm.add_instruction(inst);


  uint8_t* buffer = fm.pack();
  ofconn->send(buffer, fm.length());
  fluid_msg::OFMsg::free_buffer(buffer);

  OAILOG_DEBUG(LOG_GTPV1U, "Default SGi out flow added\n");
}

void GTPApplication::add_pdn_loop(
    const AddGTPTunnelEvent& ev,
    const OpenflowMessenger& messenger,
    const uint16_t flow_priority,
    const uint32_t goto_table) {
  of13::FlowMod fm = messenger.create_default_flow_mod(
      OF_TABLE_LOOP,
      of13::OFPFC_ADD,
      flow_priority);

  // IP eth type
  of13::EthType type_match(IP_ETH_TYPE);
  fm.add_oxm_field(type_match);

  struct in_addr ue_ip = ev.get_ue_ip();
  of13::IPv4Dst ip_match(ue_ip.s_addr);
  fm.add_oxm_field(ip_match);

  // Set eth src and dst
  of13::ApplyActions apply_ul_inst;
  EthAddress gtp_port(GTP_PORT_MAC);

  of13::SetFieldAction set_eth_src(new of13::EthSrc(gtp_port));
  apply_ul_inst.add_action(set_eth_src);

  fm.add_instruction(apply_ul_inst);

  // loop
  of13::GoToTable goto_inst(goto_table);
  fm.add_instruction(goto_inst);

  // Finally, send flow mod
  messenger.send_of_msg(fm, ev.get_connection());
  OAILOG_DEBUG(LOG_GTPV1U, "Loop flow added\n");
}


void GTPApplication::delete_uplink_tunnel_flow(
    const DeleteGTPTunnelEvent &ev,
    const OpenflowMessenger& messenger) {
  if (INVALID_TEID != ev.get_in_tei()) {
    of13::FlowMod uplink_fm = messenger.create_default_flow_mod(
        OF_TABLE_UL_GTPU,
        of13::OFPFC_DELETE,
        0);
    // match all ports and groups
    uplink_fm.out_port(of13::OFPP_ANY);
    uplink_fm.out_group(of13::OFPG_ANY);

    add_uplink_match(uplink_fm, gtp_port_num_, ev.get_in_tei());

    messenger.send_of_msg(uplink_fm, ev.get_connection());
  }
#if DEBUG_IS_ON
  else {
    OAILOG_DEBUG(LOG_GTPV1U, "No Uplink flow deleted, cause invalid teid\n");
  }
#endif
}


void sdf_filter2of_matching_rule(of13::FlowMod& fm, const sdf_filter_t& sdf_filter) {
  uint8_t next_header = 0;
  if (TRAFFIC_FLOW_TEMPLATE_IPV4_REMOTE_ADDR_FLAG & sdf_filter.packetfiltercontents.flags) {
    bstring ip_addr = bformat("%d.%d.%d.%d",
      sdf_filter.packetfiltercontents.ipv4remoteaddr[0], sdf_filter.packetfiltercontents.ipv4remoteaddr[1],
      sdf_filter.packetfiltercontents.ipv4remoteaddr[2], sdf_filter.packetfiltercontents.ipv4remoteaddr[3]);
    if ((TRAFFIC_FLOW_TEMPLATE_DOWNLINK_ONLY == sdf_filter.direction) || (TRAFFIC_FLOW_TEMPLATE_BIDIRECTIONAL == sdf_filter.direction)){
      of13::IPv4Src match(IPAddress(bdata(ip_addr)));
      fm.add_oxm_field(match);
    }
    bdestroy(ip_addr);
  }
  if (TRAFFIC_FLOW_TEMPLATE_IPV4_LOCAL_ADDR_FLAG & sdf_filter.packetfiltercontents.flags) {
    bstring ip_addr = bformat("%d.%d.%d.%d",
      sdf_filter.packetfiltercontents.ipv4localaddr[0], sdf_filter.packetfiltercontents.ipv4localaddr[1],
      sdf_filter.packetfiltercontents.ipv4localaddr[2], sdf_filter.packetfiltercontents.ipv4localaddr[3]);
    if ((TRAFFIC_FLOW_TEMPLATE_DOWNLINK_ONLY == sdf_filter.direction) || (TRAFFIC_FLOW_TEMPLATE_BIDIRECTIONAL == sdf_filter.direction)){
      of13::IPv4Dst match(IPAddress(bdata(ip_addr)));
      fm.add_oxm_field(match);
    }
    bdestroy(ip_addr);
  }
  if (TRAFFIC_FLOW_TEMPLATE_IPV6_REMOTE_ADDR_FLAG & sdf_filter.packetfiltercontents.flags) {
    // TODO
  }
  if (TRAFFIC_FLOW_TEMPLATE_IPV6_REMOTE_ADDR_PREFIX_FLAG & sdf_filter.packetfiltercontents.flags) {
    // TODO
  }
  if (TRAFFIC_FLOW_TEMPLATE_IPV6_LOCAL_ADDR_PREFIX_FLAG & sdf_filter.packetfiltercontents.flags) {
    // TODO
  }
  if (TRAFFIC_FLOW_TEMPLATE_PROTOCOL_NEXT_HEADER_FLAG & sdf_filter.packetfiltercontents.flags) {
    of13::IPProto match(sdf_filter.packetfiltercontents.protocolidentifier_nextheader);
    fm.add_oxm_field(match);
    next_header = sdf_filter.packetfiltercontents.protocolidentifier_nextheader;
  }
  if ((TRAFFIC_FLOW_TEMPLATE_SINGLE_LOCAL_PORT_FLAG & sdf_filter.packetfiltercontents.flags) && (next_header)) {
    if ((TRAFFIC_FLOW_TEMPLATE_DOWNLINK_ONLY == sdf_filter.direction) || (TRAFFIC_FLOW_TEMPLATE_BIDIRECTIONAL == sdf_filter.direction)){
      switch (next_header) {
      case IPPROTO_TCP: {
          of13::TCPDst tcp_match(sdf_filter.packetfiltercontents.singlelocalport);
          fm.add_oxm_field(tcp_match);
        }
        break;
      case IPPROTO_UDP: {
          of13::UDPDst udp_match(sdf_filter.packetfiltercontents.singlelocalport);
          fm.add_oxm_field(udp_match);
        }
        break;
      case IPPROTO_SCTP: {
          of13::SCTPDst sctp_match(sdf_filter.packetfiltercontents.singlelocalport);
          fm.add_oxm_field(sctp_match);
        }
        break;
      default:;
      }
    }
  }
  if (TRAFFIC_FLOW_TEMPLATE_LOCAL_PORT_RANGE_FLAG & sdf_filter.packetfiltercontents.flags) {
    // TODO
  }
  if (TRAFFIC_FLOW_TEMPLATE_SINGLE_REMOTE_PORT_FLAG & sdf_filter.packetfiltercontents.flags) {
    switch (next_header) {
    case IPPROTO_TCP: {
        of13::TCPSrc tcp_match(sdf_filter.packetfiltercontents.singleremoteport);
        fm.add_oxm_field(tcp_match);
      }
      break;
    case IPPROTO_UDP: {
        of13::UDPSrc udp_match(sdf_filter.packetfiltercontents.singleremoteport);
        fm.add_oxm_field(udp_match);
      }
      break;
    case IPPROTO_SCTP: {
        of13::SCTPSrc sctp_match(sdf_filter.packetfiltercontents.singleremoteport);
        fm.add_oxm_field(sctp_match);
      }
      break;
    default:;
    }
  }
  if (TRAFFIC_FLOW_TEMPLATE_REMOTE_PORT_RANGE_FLAG & sdf_filter.packetfiltercontents.flags) {
    // TODO
  }
  if (TRAFFIC_FLOW_TEMPLATE_SECURITY_PARAMETER_INDEX_FLAG & sdf_filter.packetfiltercontents.flags) {
    // TODO
  }
  if (TRAFFIC_FLOW_TEMPLATE_TYPE_OF_SERVICE_TRAFFIC_CLASS_FLAG & sdf_filter.packetfiltercontents.flags) {
    // TODO
  }
  if (TRAFFIC_FLOW_TEMPLATE_FLOW_LABEL_FLAG & sdf_filter.packetfiltercontents.flags) {
    of13::IPV6Flabel match(sdf_filter.packetfiltercontents.flowlabel);
    fm.add_oxm_field(match);
  }
}


/*
 * Helper method to add matching for adding/deleting the downlink flow
 */
void GTPApplication::add_downlink_match(of13::FlowMod& downlink_fm,
    const struct in_addr& ue_ip,
    const sdf_filter_t &sdf_filter) {
  // Set match on uplink port and IP eth type
  of13::InPort downlink_port_match(egress_port_num_);
  downlink_fm.add_oxm_field(downlink_port_match);
  of13::EthType ip_type(0x0800);
  downlink_fm.add_oxm_field(ip_type);

  // Match UE IP destination
  of13::IPv4Dst ip_match(ue_ip.s_addr);
  downlink_fm.add_oxm_field(ip_match);

  sdf_filter2of_matching_rule(downlink_fm, sdf_filter);
}


void GTPApplication::add_downlink_tunnel_flow(
    const AddGTPTunnelEvent &ev,
    const OpenflowMessenger& messenger) {

  if (INVALID_TEID != ev.get_out_tei()) {
    const pcc_rule_t *const rule = ev.get_rule();
    int pool_id  = get_paa_ipv4_pool_id(ev.get_ue_ip());
    OAILOG_INFO(LOG_GTPV1U, "add_downlink_tunnel_flow() found pool_id %d\n", pool_id);

    for (int sdff_i = 0; sdff_i < rule->sdf_template.number_of_packet_filters; sdff_i++) {

      uint16_t fprio = OF_PRIO_GTPU + (255 - rule->sdf_template.sdf_filter[sdff_i].eval_precedence);
      of13::FlowMod downlink_fm = messenger.create_default_flow_mod(
          OF_TABLE_DL_GTPU + pool_id,
          of13::OFPFC_ADD,
          fprio);

      add_downlink_match(downlink_fm, ev.get_ue_ip(), rule->sdf_template.sdf_filter[sdff_i]);

      of13::ApplyActions apply_dl_inst;

      // Set outgoing tunnel id and tunnel destination ip
      of13::SetFieldAction set_out_tunnel(new of13::TUNNELId(ev.get_out_tei()));
      apply_dl_inst.add_action(set_out_tunnel);
      of13::SetFieldAction set_tunnel_dst(
        new of13::TunnelIPv4Dst(ev.get_enb_ip().s_addr));
      apply_dl_inst.add_action(set_tunnel_dst);

      // add imsi to packet metadata to pass to other tables
      add_imsi_metadata(apply_dl_inst, ev.get_imsi());

      fluid_msg::of13::OutputAction act(gtp_port_num_, of13::OFPCML_NO_BUFFER);
      apply_dl_inst.add_action(act);

      downlink_fm.add_instruction(apply_dl_inst);

      // Finally, send flow mod
      messenger.send_of_msg(downlink_fm, ev.get_connection());
      OAILOG_DEBUG(LOG_GTPV1U, "Downlink flow added\n");

      //--------------------------------------------------------------------------
      // LOOP table
      of13::FlowMod fml = messenger.create_default_flow_mod(
          OF_TABLE_LOOP,
          of13::OFPFC_ADD,
          fprio);

      // IP eth type
      of13::EthType type_match(IP_ETH_TYPE);
      fml.add_oxm_field(type_match);

      struct in_addr ue_ip = ev.get_ue_ip();
      of13::IPv4Dst ip_match(ue_ip.s_addr);
      fml.add_oxm_field(ip_match);

      // Set eth src and dst
      of13::ApplyActions apply_loop_inst;
      EthAddress gtp_port(GTP_PORT_MAC);

      of13::SetFieldAction set_eth_src(new of13::EthSrc(gtp_port));
      apply_loop_inst.add_action(set_eth_src);

      fml.add_instruction(apply_loop_inst);

      // loop
      of13::GoToTable goto_inst(OF_TABLE_DL_GTPU + pool_id);
      fml.add_instruction(goto_inst);

      // Finally, send flow mod
      messenger.send_of_msg(fml, ev.get_connection());
      OAILOG_DEBUG(LOG_GTPV1U, "Loop flow added\n");

    }
  }
#if DEBUG_IS_ON
  else
    OAILOG_DEBUG(LOG_GTPV1U, "No Downlink flow added, cause invalid teid\n");
#endif
}

void GTPApplication::delete_downlink_tunnel_flow(
    const DeleteGTPTunnelEvent &ev,
    const OpenflowMessenger& messenger) {

  if (INVALID_TEID != ev.get_in_tei()) {
    const pcc_rule_t *const rule = ev.get_rule();

    for (int sdff_i = 0; sdff_i < rule->sdf_template.number_of_packet_filters; sdff_i++) {
      of13::FlowMod downlink_fm = messenger.create_default_flow_mod(
          OF_TABLE_DL_GTPU,
          of13::OFPFC_DELETE,
          0);
      // match all ports and groups
      downlink_fm.out_port(of13::OFPP_ANY);
      downlink_fm.out_group(of13::OFPG_ANY);

      add_downlink_match(downlink_fm, ev.get_ue_ip(), rule->sdf_template.sdf_filter[sdff_i]);
      messenger.send_of_msg(downlink_fm, ev.get_connection());
    }
  }
#if DEBUG_IS_ON
  else {
    OAILOG_DEBUG(LOG_GTPV1U, "No Downlink flow deleted, cause invalid teid\n");
  }
#endif
}

}

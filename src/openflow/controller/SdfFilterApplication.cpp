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

#include "SdfFilterApplication.h"
#include "IMSIEncoder.h"
#include <fluid/of13/openflow-13.h>

extern "C" {
  #include "log.h"
  #include "bstrlib.h"
  #include "pgw_lite_paa.h"
}

using namespace fluid_msg;


namespace openflow {

extern uint32_t prefix2mask(int prefix);

SdfFilterApplication::SdfFilterApplication(uint32_t sgi_port_num) : sgi_port_num_(sgi_port_num) {}


void SdfFilterApplication::event_callback(const ControllerEvent& ev,
                                    const OpenflowMessenger& messenger) {
  if (ev.get_type() == EVENT_ADD_SDF_FILTER) {
    auto add_sdf_event = static_cast<const AddSdfFilterEvent&>(ev);
    add_sdf_filter(add_sdf_event, messenger);
  } else if (ev.get_type() == EVENT_DELETE_SDF_FILTER) {
    auto del_sdf_event = static_cast<const DeleteSdfFilterEvent&>(ev);
    delete_sdf_filter(del_sdf_event, messenger);
  }
}

void sdf_filter2of_match(of13::FlowMod& fm, packet_filter_contents_t * const packetfiltercontents, uint8_t direction) {

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
      sdf_filter.packetfiltercontents.ipv4remoteaddr[0], sdf_filter.packetfiltercontents.ipv4remoteaddr[1],
      sdf_filter.packetfiltercontents.ipv4remoteaddr[2], sdf_filter.packetfiltercontents.ipv4remoteaddr[3]);
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

void pcc_rule2of_matching_rule(of13::FlowMod& fm,
    const sdf_id_t sdf_id,
    const sdf_template_t& sdf_template,
    const bearer_qos_t& bearer_qos,
    const uint32_t precedence)
{
  for (int sdff_i = 0; sdff_i < sdf_template.number_of_packet_filters; sdff_i++) {
    sdf_filter2of_matching_rule(fm, sdf_template.sdf_filter[sdff_i]);
  }
}

/*
 * Helper method to add matching for filtering the downlink flow
 */
void add_downlink_sdf_filter_match(of13::FlowMod& fm, const AddSdfFilterEvent& ev, const uint32_t sgi_port) {
  // Match on SGi in port
  of13::InPort sgi_port_match(sgi_port);
  fm.add_oxm_field(sgi_port_match);

  // Assume if DSCP set, it is a SDF filter (may be set by external module)
  of13::IPDSCP dscp_match(0);
  fm.add_oxm_field(dscp_match);

  pcc_rule2of_matching_rule(fm, ev.get_sdf_id(), ev.get_sdf_template(), ev.get_bearer_qos(), ev.get_precedence());

  // Get assigned IP block from mobilityd
  struct in_addr netaddr;
  uint32_t prefix;
  int ret = get_assigned_ipv4_block(0, &netaddr, &prefix);

  // Convert to string for logging
  char ip_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(netaddr.s_addr), ip_str, INET_ADDRSTRLEN);
  OAILOG_INFO(LOG_GTPV1U,
              "Setting SDF filter for UE IP block %s/%d\n",
              ip_str, prefix);

  // TODO warning
  // IP eth type
  of13::EthType type_match(IP_ETH_TYPE);
  fm.add_oxm_field(type_match);

  // Match on ip dest equaling assigned ue ip block
  of13::IPv4Dst ip_match(netaddr.s_addr, prefix2mask(prefix));
  fm.add_oxm_field(ip_match);
}

/*
 * Helper method to add imsi as metadata to the packet
 */
void add_sdf_id_metadata(of13::ApplyActions& apply_actions,
                       const int sdf_id) {
  auto metadata_field = new of13::Metadata(sdf_id);
  of13::SetFieldAction set_metadata(metadata_field);
  apply_actions.add_action(set_metadata);
}

void SdfFilterApplication::add_sdf_filter(
    const AddSdfFilterEvent& ev,
    const OpenflowMessenger& messenger) {
  of13::FlowMod fm = messenger.create_default_flow_mod(
      SdfFilterApplication::TABLE,
      of13::OFPFC_ADD,
      SdfFilterApplication::DEFAULT_PRIORITY+ev.get_precedence());
  add_downlink_sdf_filter_match(fm, ev, sgi_port_num_);

  of13::ApplyActions apply_ul_inst;

  // add imsi to packet metadata to pass to other tables
  add_sdf_id_metadata(apply_ul_inst, ev.get_sdf_id());


  //apply_ul_inst.add_action(new fluid_msg::of13::SetFieldAction(new fluid_msg::of13::IPDSCP(ev.get_sdf_id())));

  fm.add_instruction(apply_ul_inst);

  // Output to inout table
  of13::GoToTable goto_inst(SdfFilterApplication::NEXT_TABLE);
  fm.add_instruction(goto_inst);

  // Finally, send flow mod
  messenger.send_of_msg(fm, ev.get_connection());
  OAILOG_DEBUG(LOG_GTPV1U, "SDF filter %u added\n", ev.get_sdf_id());
}

void SdfFilterApplication::delete_sdf_filter(
    const DeleteSdfFilterEvent &ev,
    const OpenflowMessenger& messenger) {
  // TODO it later
}


}

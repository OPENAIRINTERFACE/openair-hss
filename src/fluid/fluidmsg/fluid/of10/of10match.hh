#ifndef OF10OPENFLOW_MATCH_H
#define OF10OPENFLOW_MATCH_H 1

#include <string>
#include <vector>
#include "fluid/util/ethaddr.hh"
#include "fluid/util/ipaddr.hh"
#include "fluid/util/util.h"
#include "openflow-10.h"

namespace fluid_msg {

namespace of10 {

class Match {
 private:
  uint32_t wildcards_;  /* Wildcard fields. */
  uint16_t in_port_;    /* Input switch port. */
  EthAddress dl_src_;   /* Ethernet source address. */
  EthAddress dl_dst_;   /* Ethernet destination address. */
  uint16_t dl_vlan_;    /* Input VLAN id. */
  uint8_t dl_vlan_pcp_; /* Input VLAN priority. */
  uint16_t dl_type_;    /* Ethernet frame type. */
  uint8_t nw_tos_;      /* IP ToS (actually DSCP field, 6 bits). */
  uint8_t nw_proto_;    /* IP protocol or lower 8 bits of
                         * ARP opcode. */
  IPAddress nw_src_;    /* IP source address. */
  IPAddress nw_dst_;    /* IP destination address. */
  uint16_t tp_src_;     /* TCP/UDP source port. */
  uint16_t tp_dst_;     /* TCP/UDP destination port. */
 public:
  Match();
  ~Match(){};
  bool operator==(const Match &other) const;
  bool operator!=(const Match &other) const;
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  uint32_t wildcards() { return this->wildcards_; }
  uint16_t in_port() { return this->in_port_; }
  EthAddress dl_src() { return this->dl_src_; }
  EthAddress dl_dst() { return this->dl_dst_; }
  uint16_t dl_vlan() { return this->dl_vlan_; }
  uint8_t dl_vlan_pcp() { return this->dl_vlan_pcp_; }
  uint16_t dl_type() { return this->dl_type_; }
  uint8_t nw_tos() { return this->nw_tos_; }
  uint8_t nw_proto() { return this->nw_proto_; }
  IPAddress nw_src() { return this->nw_src_; }
  IPAddress nw_dst() { return this->nw_dst_; }
  uint16_t tp_src() { return this->tp_src_; }
  uint16_t tp_dst() { return this->tp_dst_; }

  void wildcards(uint32_t wildcards);
  void in_port(uint16_t in_port);
  void dl_src(const EthAddress &dl_src);
  void dl_dst(const EthAddress &dl_dst);
  void dl_vlan(uint16_t dl_vlan);
  void dl_vlan_pcp(uint8_t dl_vlan_pcp);
  void dl_type(uint16_t dl_type);
  void nw_tos(uint8_t nw_tos);
  void nw_proto(uint8_t nw_proto);
  void nw_src(const IPAddress &nw_src);
  void nw_dst(const IPAddress &nw_dst);
  void nw_src(const IPAddress &nw_src, uint32_t prefix);
  void nw_dst(const IPAddress &nw_src, uint32_t prefix);
  void tp_src(uint16_t tp_src);
  void tp_dst(uint16_t tp_dst);
};

}  // End of Namespace of10
}  // End of namespace fluid_msg
#endif

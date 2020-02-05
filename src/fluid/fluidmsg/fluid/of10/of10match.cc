#include "of10match.hh"

namespace fluid_msg {

namespace of10 {

Match::Match()
    : in_port_(0),
      dl_vlan_(0),
      dl_vlan_pcp_(0),
      dl_type_(0),
      nw_tos_(0),
      nw_proto_(0),
      tp_src_(0),
      tp_dst_(0),
      dl_src_(),
      dl_dst_(),
      nw_src_((uint32_t)0),
      nw_dst_((uint32_t)0),
      wildcards_(of10::OFPFW_ALL) {}

bool Match::operator==(const Match &other) const {
  return (
      (this->in_port_ == other.in_port_) &&
      (this->wildcards_ == other.wildcards_) &&
      (this->dl_vlan_ == other.dl_vlan_) &&
      (this->dl_vlan_pcp_ == other.dl_vlan_pcp_) &&
      (this->dl_type_ == other.dl_type_) && (this->nw_tos_ == other.nw_tos_) &&
      (this->nw_proto_ == other.nw_proto_) &&
      (this->tp_src_ == other.tp_src_) && (this->tp_dst_ == other.tp_dst_) &&
      (this->nw_src_ == other.nw_src_) && (this->nw_dst_ == other.nw_dst_) &&
      (this->dl_src_ == other.dl_src_) && (this->dl_dst_ == other.dl_dst_));
}

bool Match::operator!=(const Match &other) const { return !(*this == other); }

void Match::wildcards(uint32_t wildcards) { this->wildcards_ = wildcards; }

void Match::in_port(uint16_t in_port) {
  this->in_port_ = in_port;
  this->wildcards_ &= ~of10::OFPFW_IN_PORT;
}

void Match::dl_src(const EthAddress &dl_src) {
  this->dl_src_ = dl_src;
  this->wildcards_ &= ~of10::OFPFW_DL_SRC;
}

void Match::dl_dst(const EthAddress &dl_dst) {
  this->dl_dst_ = dl_dst;
  this->wildcards_ &= ~of10::OFPFW_DL_DST;
}

void Match::dl_vlan(uint16_t dl_vlan) {
  this->dl_vlan_ = dl_vlan;
  this->wildcards_ &= ~of10::OFPFW_DL_VLAN;
}

void Match::dl_vlan_pcp(uint8_t dl_vlan_pcp) {
  this->dl_vlan_pcp_ = dl_vlan_pcp;
  this->wildcards_ &= ~of10::OFPFW_DL_VLAN_PCP;
}

void Match::dl_type(uint16_t dl_type) {
  this->dl_type_ = dl_type;
  this->wildcards_ &= ~of10::OFPFW_DL_TYPE;
}

void Match::nw_tos(uint8_t nw_tos) {
  this->nw_tos_ = nw_tos;
  this->wildcards_ &= ~of10::OFPFW_NW_TOS;
}

void Match::nw_proto(uint8_t nw_proto) {
  this->nw_proto_ = nw_proto;
  this->wildcards_ &= ~of10::OFPFW_NW_PROTO;
}

void Match::nw_src(const IPAddress &nw_src) {
  this->nw_src_ = nw_src;
  this->wildcards_ &= ~of10::OFPFW_NW_SRC_MASK;
}

void Match::nw_dst(const IPAddress &nw_dst) {
  this->nw_dst_ = nw_dst;
  this->wildcards_ &= ~of10::OFPFW_NW_DST_MASK;
}

void Match::nw_src(const IPAddress &nw_src, uint32_t prefix) {
  this->nw_src_ = nw_src;
  uint32_t index = 32 - prefix;
  this->wildcards_ &= ~of10::OFPFW_NW_SRC_MASK;
  this->wildcards_ |= (index << of10::OFPFW_NW_SRC_SHIFT);
}

void Match::nw_dst(const IPAddress &nw_dst, uint32_t prefix) {
  this->nw_dst_ = nw_dst;
  uint32_t index = 32 - prefix;
  this->wildcards_ &= ~of10::OFPFW_NW_DST_MASK;
  this->wildcards_ |= (index << of10::OFPFW_NW_DST_SHIFT);
}

void Match::tp_src(uint16_t tp_src) {
  this->tp_src_ = tp_src;
  this->wildcards_ &= ~of10::OFPFW_TP_SRC;
}

void Match::tp_dst(uint16_t tp_dst) {
  this->tp_dst_ = tp_dst;
  this->wildcards_ &= ~of10::OFPFW_TP_DST;
}

size_t Match::pack(uint8_t *buffer) {
  struct of10::ofp_match *m = (struct ofp_match *)buffer;
  m->wildcards = hton32(this->wildcards_);
  m->in_port = hton16(this->in_port_);
  memcpy(m->dl_src, this->dl_src_.get_data(), OFP_ETH_ALEN);
  memcpy(m->dl_dst, this->dl_dst_.get_data(), OFP_ETH_ALEN);
  m->dl_vlan = hton16(this->dl_vlan_);
  m->dl_vlan_pcp = this->dl_vlan_pcp_;
  memset(m->pad1, 0x0, 1);
  m->dl_type = hton16(this->dl_type_);
  m->nw_tos = this->nw_tos_;
  m->nw_proto = this->nw_proto_;
  memset(m->pad2, 0x0, 2);
  m->nw_src = hton32(this->nw_src_.getIPv4());
  m->nw_dst = hton32(this->nw_dst_.getIPv4());
  m->tp_src = hton16(this->tp_src_);
  m->tp_dst = hton16(this->tp_dst_);
  return 0;
}

of_error Match::unpack(uint8_t *buffer) {
  struct of10::ofp_match *m = (struct ofp_match *)buffer;
  this->wildcards_ = ntoh32(m->wildcards);
  this->in_port_ = ntoh16(m->in_port);
  this->dl_src_.set_data(m->dl_src);
  this->dl_dst_.set_data(m->dl_dst);
  this->dl_vlan_ = ntoh16(m->dl_vlan);
  this->dl_vlan_pcp_ = m->dl_vlan_pcp;
  this->dl_type_ = ntoh16(m->dl_type);
  this->nw_tos_ = m->nw_tos;
  this->nw_proto_ = m->nw_proto;
  this->nw_src_.setIPv4(ntoh32(m->nw_src));
  this->nw_dst_.setIPv4(ntoh32(m->nw_dst));
  this->tp_src_ = ntoh16(m->tp_src);
  this->tp_dst_ = ntoh16(m->tp_dst);
  return 0;
}

}  // End of namespace of10
}  // End of namespace fluid_msg

#include "of13match.hh"
#include <algorithm>
#include "fluid/util/util.h"

namespace fluid_msg {

namespace of13 {

MatchHeader::MatchHeader() : type_(0), length_(0) {}

MatchHeader::MatchHeader(uint16_t type, uint16_t length) {
  this->type_ = type;
  this->length_ = length;
}

bool MatchHeader::operator==(const MatchHeader &other) const {
  return ((this->type_ == other.type_) && (this->length_ == other.length_));
}

bool MatchHeader::operator!=(const MatchHeader &other) const {
  return !(*this == other);
}

size_t MatchHeader::pack(uint8_t *buffer) {
  struct of13::ofp_match *m = (struct of13::ofp_match *)buffer;
  m->type = hton16(this->type_);
  m->length = hton16(this->length_);
  return 0;
}

of_error MatchHeader::unpack(uint8_t *buffer) {
  struct of13::ofp_match *m = (struct of13::ofp_match *)buffer;
  this->type_ = ntoh16(m->type);
  this->length_ = ntoh16(m->length);
  if (this->length_ < sizeof(struct of13::ofp_match)) {
    return openflow_error(of13::OFPET_BAD_MATCH, of13::OFPBMC_BAD_LEN);
  }
  return 0;
}

OXMTLV::OXMTLV() : class__(0), field_(0), has_mask_(0), length_(0) {}

OXMTLV::OXMTLV(uint16_t class_, uint8_t field, bool has_mask, uint8_t length)
    : class__(class_),
      field_(field),
      has_mask_(has_mask),
      length_(has_mask ? length << 1 : length) {}

bool OXMTLV::equals(const OXMTLV &other) {
  return ((this->class__ == other.class__) && (this->field_ == other.field_) &&
          (this->has_mask_ == other.has_mask_) &&
          (this->length_ == other.length_));
}

bool OXMTLV::operator==(const OXMTLV &other) const {
  return ((this->class__ == other.class__) && (this->field_ == other.field_) &&
          (this->has_mask_ && other.has_mask_) &&
          (this->length_ == other.length_));
}

bool OXMTLV::operator!=(const OXMTLV &other) const { return !(*this == other); }

OXMTLV &OXMTLV::operator=(const OXMTLV &field) {
  this->class__ = field.class__;
  this->field_ = field.field_;
  this->has_mask_ = field.has_mask_;
  this->length_ = field.length_;
  return *this;
}

void OXMTLV::create_oxm_req(uint16_t eth_type1, uint16_t eth_type2,
                            uint8_t ip_proto, uint8_t icmp) {
  this->reqs.eth_type_req[0] = eth_type1;
  this->reqs.eth_type_req[1] = eth_type2;
  this->reqs.ip_proto_req = ip_proto;
  this->reqs.icmp_req = icmp;
}

size_t OXMTLV::pack(uint8_t *buffer) {
  uint32_t header = hton32(OXMTLV::make_header(this->class__, this->field_,
                                               this->has_mask_, this->length_));
  memcpy(buffer, &header, sizeof(uint32_t));
  return 0;
}

of_error OXMTLV::unpack(uint8_t *buffer) {
  uint32_t header = ntoh32(*((uint32_t *)buffer));
  this->class__ = oxm_class(header);
  this->field_ = oxm_field(header);
  this->has_mask_ = oxm_has_mask(header);
  this->length_ = oxm_length(header);
  return 0;
}

uint32_t OXMTLV::make_header(uint16_t class_, uint8_t field, bool has_mask,
                             uint8_t length) {
  return (((class_) << 16) | ((field) << 9) | ((has_mask ? 1 : 0) << 8) |
          (length));
}

uint16_t OXMTLV::oxm_class(uint32_t header) { return ((header) >> 16); }

uint8_t OXMTLV::oxm_field(uint32_t header) { return (((header) >> 9) & 0x7f); }

bool OXMTLV::oxm_has_mask(uint32_t header) { return (((header) >> 8) & 1); }

uint8_t OXMTLV::oxm_length(uint32_t header) { return ((header)&0xff); }

InPort::InPort()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IN_PORT, false,
             of13::OFP_OXM_IN_PORT_LEN) {
  create_oxm_req(0, 0, 0, 0);
}

InPort::InPort(uint32_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IN_PORT, false,
             of13::OFP_OXM_IN_PORT_LEN) {
  this->value_ = value;
  create_oxm_req(0, 0, 0, 0);
}

bool InPort::equals(const OXMTLV &other) {
  if (const InPort *field = dynamic_cast<const InPort *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &InPort::operator=(const OXMTLV &field) {
  const InPort &port = dynamic_cast<const InPort &>(field);
  OXMTLV::operator=(field);
  this->value_ = port.value_;
  return *this;
};

size_t InPort::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  uint32_t value = hton32(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, this->length_);
  return 0;
}

of_error InPort::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh32(*((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  return 0;
}

InPhyPort::InPhyPort()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IN_PHY_PORT, false,
             of13::OFP_OXM_IN_PHY_PORT_LEN) {
  create_oxm_req(0, 0, 0, 0);
}

InPhyPort::InPhyPort(uint32_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IN_PHY_PORT, false,
             of13::OFP_OXM_IN_PHY_PORT_LEN) {
  this->value_ = value;
  create_oxm_req(0, 0, 0, 0);
}

bool InPhyPort::equals(const OXMTLV &other) {
  if (const InPhyPort *field = dynamic_cast<const InPhyPort *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &InPhyPort::operator=(const OXMTLV &field) {
  const InPhyPort &phy_port = dynamic_cast<const InPhyPort &>(field);
  OXMTLV::operator=(field);
  this->value_ = phy_port.value_;
  return *this;
};

size_t InPhyPort::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  uint32_t value = hton32(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, this->length_);
  return 0;
}

of_error InPhyPort::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh32(*((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  return 0;
}

Metadata::Metadata()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_METADATA, false,
             of13::OFP_OXM_METADATA_LEN) {
  create_oxm_req(0, 0, 0, 0);
}

Metadata::Metadata(uint64_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_METADATA, false,
             of13::OFP_OXM_METADATA_LEN) {
  this->value_ = value;
  create_oxm_req(0, 0, 0, 0);
}

Metadata::Metadata(uint64_t value, uint64_t mask)
    : mask_(0),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_METADATA, true,
             of13::OFP_OXM_METADATA_LEN) {
  this->value_ = value;
  this->mask_ = mask;
  create_oxm_req(0, 0, 0, 0);
}

bool Metadata::equals(const OXMTLV &other) {
  if (const Metadata *field = dynamic_cast<const Metadata *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &Metadata::operator=(const OXMTLV &field) {
  const Metadata &meta = dynamic_cast<const Metadata &>(field);
  OXMTLV::operator=(field);
  this->value_ = meta.value_;
  this->mask_ = meta.mask_;
  return *this;
};

size_t Metadata::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    uint64_t mask = hton64(this->mask_);
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), &mask, len);
  }
  uint64_t value = hton64(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, len);
  return 0;
}

of_error Metadata::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh64(*((uint64_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    this->mask_ =
        ntoh64(*((uint64_t *)(buffer + of13::OFP_OXM_HEADER_LEN + len)));
  }
  return 0;
}

EthDst::EthDst()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ETH_DST, false,
             OFP_ETH_ALEN) {
  create_oxm_req(0, 0, 0, 0);
}

EthDst::EthDst(EthAddress value)
    : mask_("ff:ff:ff:ff:ff:ff"),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ETH_DST, false,
             OFP_ETH_ALEN) {
  this->value_ = value;
  create_oxm_req(0, 0, 0, 0);
}

EthDst::EthDst(EthAddress value, EthAddress mask)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ETH_DST, true,
             OFP_ETH_ALEN) {
  this->value_ = value;
  this->mask_ = mask;
  create_oxm_req(0, 0, 0, 0);
}

bool EthDst::equals(const OXMTLV &other) {
  if (const EthDst *field = dynamic_cast<const EthDst *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &EthDst::operator=(const OXMTLV &field) {
  const EthDst &dst = dynamic_cast<const EthDst &>(field);
  OXMTLV::operator=(field);
  this->value_ = dst.value_;
  this->mask_ = dst.mask_;
  return *this;
};

size_t EthDst::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), this->mask_.get_data(),
           len);
  }
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, this->value_.get_data(), len);
  return 0;
}

of_error EthDst::unpack(uint8_t *buffer) {
  uint8_t v[OFP_ETH_ALEN];
  OXMTLV::unpack(buffer);
  memcpy(v, buffer + of13::OFP_OXM_HEADER_LEN, OFP_ETH_ALEN);
  this->value_ = EthAddress(v);
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    memcpy(v, buffer + of13::OFP_OXM_HEADER_LEN + len, OFP_ETH_ALEN);
    this->mask_ = EthAddress(v);
  }
  return 0;
}

EthSrc::EthSrc()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ETH_SRC, false,
             OFP_ETH_ALEN) {
  create_oxm_req(0, 0, 0, 0);
}

EthSrc::EthSrc(EthAddress value)
    : mask_("ff:ff:ff:ff:ff:ff"),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ETH_SRC, false,
             OFP_ETH_ALEN) {
  this->value_ = value;
  create_oxm_req(0, 0, 0, 0);
}

EthSrc::EthSrc(EthAddress value, EthAddress mask)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ETH_SRC, true,
             OFP_ETH_ALEN) {
  this->value_ = value;
  this->mask_ = mask;
  create_oxm_req(0, 0, 0, 0);
}

bool EthSrc::equals(const OXMTLV &other) {
  if (const EthSrc *field = dynamic_cast<const EthSrc *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &EthSrc::operator=(const OXMTLV &field) {
  const EthSrc &src = dynamic_cast<const EthSrc &>(field);
  OXMTLV::operator=(field);
  this->value_ = src.value_;
  this->mask_ = src.mask_;
  return *this;
};

size_t EthSrc::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), this->mask_.get_data(),
           len);
  }
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, this->value_.get_data(), len);
  return 0;
}

of_error EthSrc::unpack(uint8_t *buffer) {
  uint8_t v[OFP_ETH_ALEN];
  OXMTLV::unpack(buffer);
  memcpy(v, buffer + of13::OFP_OXM_HEADER_LEN, OFP_ETH_ALEN);
  this->value_ = EthAddress(v);
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    memcpy(v, buffer + of13::OFP_OXM_HEADER_LEN + len, OFP_ETH_ALEN);
    this->mask_ = EthAddress(v);
  }
  return 0;
}

EthType::EthType()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ETH_TYPE, false,
             of13::OFP_OXM_ETH_TYPE_LEN) {
  create_oxm_req(0, 0, 0, 0);
}

EthType::EthType(uint16_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ETH_TYPE, false,
             of13::OFP_OXM_ETH_TYPE_LEN) {
  this->value_ = value;
  create_oxm_req(0, 0, 0, 0);
}

bool EthType::equals(const OXMTLV &other) {
  if (const EthType *field = dynamic_cast<const EthType *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &EthType::operator=(const OXMTLV &field) {
  const EthType &type = dynamic_cast<const EthType &>(field);
  OXMTLV::operator=(field);
  this->value_ = type.value_;
  return *this;
};

size_t EthType::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  uint16_t value = hton16(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, this->length_);
  return 0;
}

of_error EthType::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh16(*((uint16_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  return 0;
}

VLANVid::VLANVid()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_VLAN_VID, false,
             of13::OFP_OXM_VLAN_VID_LEN) {
  create_oxm_req(0, 0, 0, 0);
}

VLANVid::VLANVid(uint16_t value)
    : mask_(0),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_VLAN_VID, false,
             of13::OFP_OXM_VLAN_VID_LEN) {
  this->value_ = value;
  create_oxm_req(0, 0, 0, 0);
}

VLANVid::VLANVid(uint16_t value, uint16_t mask)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_VLAN_VID, true,
             of13::OFP_OXM_VLAN_VID_LEN) {
  this->mask_ = mask;
  this->value_ = value;
  create_oxm_req(0, 0, 0, 0);
}

bool VLANVid::equals(const OXMTLV &other) {
  if (const VLANVid *field = dynamic_cast<const VLANVid *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &VLANVid::operator=(const OXMTLV &field) {
  const VLANVid &id = dynamic_cast<const VLANVid &>(field);
  OXMTLV::operator=(field);
  this->value_ = id.value_;
  this->mask_ = id.mask_;
  return *this;
};

size_t VLANVid::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    uint16_t mask = hton16(this->mask_);
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), &mask, len);
  }
  uint16_t value = hton16(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, len);
  return 0;
}

of_error VLANVid::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh16(*((uint16_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    this->mask_ =
        ntoh16(*((uint16_t *)(buffer + (of13::OFP_OXM_HEADER_LEN + len))));
  }
  return 0;
}

VLANPcp::VLANPcp()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_VLAN_PCP, false,
             of13::OFP_OXM_VLAN_PCP_LEN) {
  create_oxm_req(0, 0, 0, 0);
}

VLANPcp::VLANPcp(uint8_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_VLAN_PCP, false,
             of13::OFP_OXM_VLAN_PCP_LEN) {
  this->value_ = value;
  create_oxm_req(0, 0, 0, 0);
}

bool VLANPcp::equals(const OXMTLV &other) {
  if (const VLANPcp *field = dynamic_cast<const VLANPcp *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &VLANPcp::operator=(const OXMTLV &field) {
  const VLANPcp &pcp = dynamic_cast<const VLANPcp &>(field);
  OXMTLV::operator=(field);
  this->value_ = pcp.value_;
  return *this;
};

size_t VLANPcp::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &this->value_, this->length_);
  return 0;
}

of_error VLANPcp::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = *((uint8_t *)(buffer + of13::OFP_OXM_HEADER_LEN));
  return 0;
}

IPDSCP::IPDSCP()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IP_DSCP, false,
             of13::OFP_OXM_IP_DSCP_LEN) {
  create_oxm_req(0x0800, 0x86dd, 0, 0);
}

IPDSCP::IPDSCP(uint8_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IP_DSCP, false,
             of13::OFP_OXM_IP_DSCP_LEN) {
  this->value_ = value;
  create_oxm_req(0x0800, 0x86dd, 0, 0);
}

bool IPDSCP::equals(const OXMTLV &other) {
  if (const IPDSCP *field = dynamic_cast<const IPDSCP *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &IPDSCP::operator=(const OXMTLV &field) {
  const IPDSCP &dscp = dynamic_cast<const IPDSCP &>(field);
  OXMTLV::operator=(field);
  this->value_ = dscp.value_;
  return *this;
};

size_t IPDSCP::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &this->value_, this->length_);
  return 0;
}

of_error IPDSCP::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = *((uint8_t *)(buffer + of13::OFP_OXM_HEADER_LEN));
  return 0;
}

IPECN::IPECN()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IP_ECN, false,
             of13::OFP_OXM_IP_DSCP_LEN) {
  create_oxm_req(0x0800, 0x86dd, 0, 0);
}

IPECN::IPECN(uint8_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IP_ECN, false,
             of13::OFP_OXM_IP_DSCP_LEN) {
  this->value_ = value;
  create_oxm_req(0x0800, 0x86dd, 0, 0);
}

bool IPECN::equals(const OXMTLV &other) {
  if (const IPECN *field = dynamic_cast<const IPECN *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &IPECN::operator=(const OXMTLV &field) {
  const IPECN &ecn = dynamic_cast<const IPECN &>(field);
  OXMTLV::operator=(field);
  this->value_ = ecn.value_;
  return *this;
};

size_t IPECN::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &this->value_, this->length_);
  return 0;
}

of_error IPECN::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = *((uint8_t *)(buffer + of13::OFP_OXM_HEADER_LEN));
  return 0;
}

IPProto::IPProto()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IP_PROTO, false,
             of13::OFP_OXM_IP_PROTO_LEN) {
  create_oxm_req(0x0800, 0x86dd, 0, 0);
}

IPProto::IPProto(uint8_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IP_PROTO, false,
             of13::OFP_OXM_IP_PROTO_LEN) {
  this->value_ = value;
  create_oxm_req(0x0800, 0x86dd, 0, 0);
}

bool IPProto::equals(const OXMTLV &other) {
  if (const IPProto *field = dynamic_cast<const IPProto *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &IPProto::operator=(const OXMTLV &field) {
  const IPProto &proto = dynamic_cast<const IPProto &>(field);
  OXMTLV::operator=(field);
  this->value_ = proto.value_;
  return *this;
};

size_t IPProto::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &this->value_, this->length_);
  return 0;
}

of_error IPProto::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = *(buffer + of13::OFP_OXM_HEADER_LEN);
  return 0;
}

IPv4Src::IPv4Src()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV4_SRC, false,
             of13::OFP_OXM_IPV4_LEN),
      value_((uint32_t)0),
      mask_((uint32_t)0) {
  create_oxm_req(0x0800, 0, 0, 0);
}

IPv4Src::IPv4Src(IPAddress value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV4_SRC, false,
             of13::OFP_OXM_IPV4_LEN),
      value_(value),
      mask_((uint32_t)0) {
  // this->value_  = value;
  create_oxm_req(0x0800, 0, 0, 0);
}

IPv4Src::IPv4Src(IPAddress value, IPAddress mask)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV4_SRC, true,
             of13::OFP_OXM_IPV4_LEN),
      value_(value),
      mask_(mask) {
  // this->value_  = value;
  // this->mask_  = mask;
  create_oxm_req(0x0800, 0, 0, 0);
}

bool IPv4Src::equals(const OXMTLV &other) {
  if (const IPv4Src *field = dynamic_cast<const IPv4Src *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &IPv4Src::operator=(const OXMTLV &field) {
  const IPv4Src &src = dynamic_cast<const IPv4Src &>(field);
  OXMTLV::operator=(field);
  this->value_ = src.value_;
  this->mask_ = src.mask_;
  return *this;
};

size_t IPv4Src::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    uint32_t ip_mask = this->mask_.getIPv4();
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), &ip_mask, len);
  }
  uint32_t ip = this->value_.getIPv4();
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &ip, len);
  return 0;
}

of_error IPv4Src::unpack(uint8_t *buffer) {
  uint32_t ip = *((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN));
  OXMTLV::unpack(buffer);
  this->value_ = IPAddress(ip);
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    uint32_t ip_mask = *((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN + len));
    this->mask_ = IPAddress(ip_mask);
  }
  return 0;
}

IPv4Dst::IPv4Dst()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV4_DST, false,
             of13::OFP_OXM_IPV4_LEN),
      value_((uint32_t)0),
      mask_((uint32_t)0) {
  create_oxm_req(0x0800, 0, 0, 0);
}

IPv4Dst::IPv4Dst(IPAddress value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV4_DST, false,
             of13::OFP_OXM_IPV4_LEN),
      value_(value),
      mask_((uint32_t)0) {
  // this->value_  = value;
  create_oxm_req(0x0800, 0, 0, 0);
}

IPv4Dst::IPv4Dst(IPAddress value, IPAddress mask)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV4_DST, true,
             of13::OFP_OXM_IPV4_LEN),
      value_(value),
      mask_(mask) {
  // this->value_  = value;
  // this->mask_  = mask;
  create_oxm_req(0x0800, 0, 0, 0);
}

bool IPv4Dst::equals(const OXMTLV &other) {
  if (const IPv4Dst *field = dynamic_cast<const IPv4Dst *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &IPv4Dst::operator=(const OXMTLV &field) {
  const IPv4Dst &dst = dynamic_cast<const IPv4Dst &>(field);
  OXMTLV::operator=(field);
  this->value_ = dst.value_;
  this->mask_ = dst.mask_;
  return *this;
};

size_t IPv4Dst::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    uint32_t ip_mask = this->mask_.getIPv4();
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), &ip_mask, len);
  }
  uint32_t ip = this->value_.getIPv4();
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &ip, len);
  return 0;
}

of_error IPv4Dst::unpack(uint8_t *buffer) {
  uint32_t ip = *((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN));
  OXMTLV::unpack(buffer);
  this->value_ = IPAddress(ip);
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    uint32_t ip_mask = *((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN + len));
    this->mask_ = IPAddress(ip_mask);
  }
  return 0;
}

TCPSrc::TCPSrc()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_TCP_SRC, false,
             of13::OFP_OXM_TP_LEN) {
  create_oxm_req(0x0800, 0x86dd, 6, 0);
}

TCPSrc::TCPSrc(uint16_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_TCP_SRC, false,
             of13::OFP_OXM_TP_LEN) {
  this->value_ = value;
  create_oxm_req(0x0800, 0x86dd, 6, 0);
}

bool TCPSrc::equals(const OXMTLV &other) {
  if (const TCPSrc *field = dynamic_cast<const TCPSrc *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &TCPSrc::operator=(const OXMTLV &field) {
  const TCPSrc &tp = dynamic_cast<const TCPSrc &>(field);
  OXMTLV::operator=(field);
  this->value_ = tp.value_;
  return *this;
};

size_t TCPSrc::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  uint16_t value = hton16(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, this->length_);
  return 0;
}

of_error TCPSrc::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh16(*((uint16_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  return 0;
}

TCPDst::TCPDst()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_TCP_DST, false,
             of13::OFP_OXM_TP_LEN) {
  create_oxm_req(0x0800, 0x86dd, 6, 0);
}

TCPDst::TCPDst(uint16_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_TCP_DST, false,
             of13::OFP_OXM_TP_LEN) {
  this->value_ = value;
  create_oxm_req(0x0800, 0x86dd, 6, 0);
}

bool TCPDst::equals(const OXMTLV &other) {
  if (const TCPDst *field = dynamic_cast<const TCPDst *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &TCPDst::operator=(const OXMTLV &field) {
  const TCPDst &tp = dynamic_cast<const TCPDst &>(field);
  OXMTLV::operator=(field);
  this->value_ = tp.value_;
  return *this;
};

size_t TCPDst::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  uint16_t value = hton16(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, this->length_);
  return 0;
}

of_error TCPDst::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh16(*((uint16_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  return 0;
}

UDPSrc::UDPSrc()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_UDP_SRC, false,
             of13::OFP_OXM_TP_LEN) {
  create_oxm_req(0x0800, 0x86dd, 17, 0);
}

UDPSrc::UDPSrc(uint16_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_UDP_SRC, false,
             of13::OFP_OXM_TP_LEN) {
  this->value_ = value;
  create_oxm_req(0x0800, 0x86dd, 17, 0);
}

bool UDPSrc::equals(const OXMTLV &other) {
  if (const UDPSrc *field = dynamic_cast<const UDPSrc *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &UDPSrc::operator=(const OXMTLV &field) {
  const UDPSrc &tp = dynamic_cast<const UDPSrc &>(field);
  OXMTLV::operator=(field);
  this->value_ = tp.value_;
  return *this;
};

size_t UDPSrc::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  uint16_t value = hton16(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, this->length_);
  return 0;
}

of_error UDPSrc::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh16(*((uint16_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  return 0;
}

UDPDst::UDPDst()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_UDP_DST, false,
             of13::OFP_OXM_TP_LEN) {
  create_oxm_req(0x0800, 0x86dd, 17, 0);
}

UDPDst::UDPDst(uint16_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_UDP_DST, false,
             of13::OFP_OXM_TP_LEN) {
  this->value_ = value;
  create_oxm_req(0x0800, 0x86dd, 17, 0);
}

bool UDPDst::equals(const OXMTLV &other) {
  if (const UDPDst *field = dynamic_cast<const UDPDst *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &UDPDst::operator=(const OXMTLV &field) {
  const UDPDst &tp = dynamic_cast<const UDPDst &>(field);
  OXMTLV::operator=(field);
  this->value_ = tp.value_;
  return *this;
};

size_t UDPDst::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  uint16_t value = hton16(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, this->length_);
  return 0;
}

of_error UDPDst::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh16(*((uint16_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  return 0;
}

SCTPSrc::SCTPSrc()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_SCTP_SRC, false,
             of13::OFP_OXM_TP_LEN) {
  create_oxm_req(0x0800, 0x86dd, 132, 0);
}

SCTPSrc::SCTPSrc(uint16_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_SCTP_SRC, false,
             of13::OFP_OXM_TP_LEN) {
  this->value_ = value;
  create_oxm_req(0x0800, 0x86dd, 132, 0);
}

bool SCTPSrc::equals(const OXMTLV &other) {
  if (const SCTPSrc *field = dynamic_cast<const SCTPSrc *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &SCTPSrc::operator=(const OXMTLV &field) {
  const SCTPSrc &tp = dynamic_cast<const SCTPSrc &>(field);
  OXMTLV::operator=(field);
  this->value_ = tp.value_;
  return *this;
};

size_t SCTPSrc::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  uint16_t value = hton16(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, this->length_);
  return 0;
}

of_error SCTPSrc::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh16(*((uint16_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  return 0;
}

SCTPDst::SCTPDst()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_SCTP_DST, false,
             of13::OFP_OXM_TP_LEN) {
  create_oxm_req(0x0800, 0x86dd, 132, 0);
}

SCTPDst::SCTPDst(uint16_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_SCTP_DST, false,
             of13::OFP_OXM_TP_LEN) {
  this->value_ = value;
  create_oxm_req(0x0800, 0x86dd, 132, 0);
}

bool SCTPDst::equals(const OXMTLV &other) {
  if (const SCTPDst *field = dynamic_cast<const SCTPDst *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &SCTPDst::operator=(const OXMTLV &field) {
  const SCTPDst &tp = dynamic_cast<const SCTPDst &>(field);
  OXMTLV::operator=(field);
  this->value_ = tp.value_;
  return *this;
};

size_t SCTPDst::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  uint16_t value = hton16(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, this->length_);
  return 0;
}

of_error SCTPDst::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh16(*((uint16_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  return 0;
}

ICMPv4Type::ICMPv4Type()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ICMPV4_CODE, false,
             of13::OFP_OXM_ICMP_TYPE_LEN) {
  create_oxm_req(0x0800, 0, 1, 0);
}

ICMPv4Type::ICMPv4Type(uint8_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ICMPV4_TYPE, false,
             of13::OFP_OXM_ICMP_TYPE_LEN) {
  this->value_ = value;
  create_oxm_req(0x0800, 0, 1, 0);
}

bool ICMPv4Type::equals(const OXMTLV &other) {
  if (const ICMPv4Type *field = dynamic_cast<const ICMPv4Type *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &ICMPv4Type::operator=(const OXMTLV &field) {
  const ICMPv4Type &type = dynamic_cast<const ICMPv4Type &>(field);
  OXMTLV::operator=(field);
  this->value_ = type.value_;
  return *this;
};

size_t ICMPv4Type::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &this->value_, this->length_);
  return 0;
}

of_error ICMPv4Type::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = *((uint8_t *)(buffer + of13::OFP_OXM_HEADER_LEN));
  return 0;
}

ICMPv4Code::ICMPv4Code()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ICMPV4_CODE, false,
             of13::OFP_OXM_ICMP_CODE_LEN) {
  create_oxm_req(0x0800, 0, 1, 0);
}

ICMPv4Code::ICMPv4Code(uint8_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ICMPV4_CODE, false,
             of13::OFP_OXM_ICMP_CODE_LEN) {
  this->value_ = value;
  create_oxm_req(0x0800, 0, 1, 0);
}

bool ICMPv4Code::equals(const OXMTLV &other) {
  if (const ICMPv4Code *field = dynamic_cast<const ICMPv4Code *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &ICMPv4Code::operator=(const OXMTLV &field) {
  const ICMPv4Code &code = dynamic_cast<const ICMPv4Code &>(field);
  OXMTLV::operator=(field);
  this->value_ = code.value_;
  return *this;
};

size_t ICMPv4Code::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &this->value_, this->length_);
  return 0;
}

of_error ICMPv4Code::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = *((uint8_t *)(buffer + of13::OFP_OXM_HEADER_LEN));
  return 0;
}

ARPOp::ARPOp()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ARP_OP, false,
             of13::OFP_OXM_ARP_OP_LEN) {
  create_oxm_req(0x0806, 0, 0, 0);
}

ARPOp::ARPOp(uint16_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ARP_OP, false,
             of13::OFP_OXM_ARP_OP_LEN) {
  this->value_ = value;
  create_oxm_req(0x0806, 0, 0, 0);
}

bool ARPOp::equals(const OXMTLV &other) {
  if (const ARPOp *field = dynamic_cast<const ARPOp *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &ARPOp::operator=(const OXMTLV &field) {
  const ARPOp &op = dynamic_cast<const ARPOp &>(field);
  OXMTLV::operator=(field);
  this->value_ = op.value_;
  return *this;
};

size_t ARPOp::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  uint16_t value = hton16(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, this->length_);
  return 0;
}

of_error ARPOp::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh16(*((uint16_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  return 0;
}

ARPSPA::ARPSPA()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ARP_SPA, false,
             of13::OFP_OXM_IPV4_LEN) {
  create_oxm_req(0x0806, 0, 0, 0);
}

ARPSPA::ARPSPA(IPAddress value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ARP_SPA, false,
             of13::OFP_OXM_IPV4_LEN) {
  this->value_ = value;
  create_oxm_req(0x0806, 0, 0, 0);
}

ARPSPA::ARPSPA(IPAddress value, IPAddress mask)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ARP_SPA, true,
             of13::OFP_OXM_IPV4_LEN) {
  this->value_ = value;
  this->mask_ = mask;
}

bool ARPSPA::equals(const OXMTLV &other) {
  if (const ARPSPA *field = dynamic_cast<const ARPSPA *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &ARPSPA::operator=(const OXMTLV &field) {
  const ARPSPA &arp = dynamic_cast<const ARPSPA &>(field);
  OXMTLV::operator=(field);
  this->value_ = arp.value_;
  this->mask_ = arp.mask_;
  return *this;
};

size_t ARPSPA::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    uint32_t ip_mask = this->mask_.getIPv4();
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), &ip_mask, len);
  }
  uint32_t ip = this->value_.getIPv4();
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &ip, len);
  return 0;
}

of_error ARPSPA::unpack(uint8_t *buffer) {
  uint32_t ip = *((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN));
  OXMTLV::unpack(buffer);
  this->value_ = IPAddress(ip);
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    uint32_t ip_mask = *((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN + len));
    this->mask_ = IPAddress(ip_mask);
  }
  return 0;
}

ARPTPA::ARPTPA()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ARP_TPA, false,
             of13::OFP_OXM_IPV4_LEN) {
  create_oxm_req(0x0806, 0, 0, 0);
}

ARPTPA::ARPTPA(IPAddress value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ARP_TPA, false,
             of13::OFP_OXM_IPV4_LEN) {
  this->value_ = value;
  create_oxm_req(0x0806, 0, 0, 0);
}

ARPTPA::ARPTPA(IPAddress value, IPAddress mask)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ARP_TPA, true,
             of13::OFP_OXM_IPV4_LEN) {
  this->value_ = value;
  this->mask_ = mask;
  create_oxm_req(0x0806, 0, 0, 0);
}

bool ARPTPA::equals(const OXMTLV &other) {
  if (const ARPTPA *field = dynamic_cast<const ARPTPA *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &ARPTPA::operator=(const OXMTLV &field) {
  const ARPTPA &arp = dynamic_cast<const ARPTPA &>(field);
  OXMTLV::operator=(field);
  this->value_ = arp.value_;
  this->mask_ = arp.mask_;
  return *this;
};

size_t ARPTPA::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    uint32_t ip_mask = this->mask_.getIPv4();
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), &ip_mask, len);
  }
  uint32_t ip = this->value_.getIPv4();
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &ip, len);
  return 0;
}

of_error ARPTPA::unpack(uint8_t *buffer) {
  uint32_t ip = *((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN));
  OXMTLV::unpack(buffer);
  this->value_ = IPAddress(ip);
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    uint32_t ip_mask = *((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN + len));
    this->mask_ = IPAddress(ip_mask);
  }
  return 0;
}

ARPSHA::ARPSHA()
    : mask_("ff:ff:ff:ff:ff:ff"),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ARP_SHA, false,
             OFP_ETH_ALEN) {
  create_oxm_req(0x0806, 0, 0, 0);
}

ARPSHA::ARPSHA(EthAddress value)
    : mask_("ff:ff:ff:ff:ff:ff"),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ARP_SHA, false,
             OFP_ETH_ALEN) {
  this->value_ = value;
  create_oxm_req(0x0806, 0, 0, 0);
}

ARPSHA::ARPSHA(EthAddress value, EthAddress mask)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ARP_SHA, true,
             OFP_ETH_ALEN) {
  this->value_ = value;
  this->mask_ = mask;
  create_oxm_req(0x0806, 0, 0, 0);
}

bool ARPSHA::equals(const OXMTLV &other) {
  if (const ARPSHA *field = dynamic_cast<const ARPSHA *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &ARPSHA::operator=(const OXMTLV &field) {
  const ARPSHA &arp = dynamic_cast<const ARPSHA &>(field);
  OXMTLV::operator=(field);
  this->value_ = arp.value_;
  this->mask_ = arp.mask_;
  return *this;
};

size_t ARPSHA::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), this->mask_.get_data(),
           len);
  }
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, this->value_.get_data(), len);
  return 0;
}

of_error ARPSHA::unpack(uint8_t *buffer) {
  uint8_t v[OFP_ETH_ALEN];
  OXMTLV::unpack(buffer);
  memcpy(v, buffer + of13::OFP_OXM_HEADER_LEN, OFP_ETH_ALEN);
  this->value_ = EthAddress(v);
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    memcpy(v, buffer + of13::OFP_OXM_HEADER_LEN + len, OFP_ETH_ALEN);
    this->mask_ = EthAddress(v);
  }
  return 0;
}

ARPTHA::ARPTHA()
    : mask_("ff:ff:ff:ff:ff:ff"),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ARP_THA, false,
             OFP_ETH_ALEN) {
  create_oxm_req(0x0806, 0, 0, 0);
}

ARPTHA::ARPTHA(EthAddress value)
    : mask_("ff:ff:ff:ff:ff:ff"),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ARP_THA, false,
             OFP_ETH_ALEN) {
  this->value_ = value;
  create_oxm_req(0x0806, 0, 0, 0);
}

ARPTHA::ARPTHA(EthAddress value, EthAddress mask)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ARP_THA, true,
             OFP_ETH_ALEN) {
  this->value_ = value;
  this->mask_ = mask;
  create_oxm_req(0x0806, 0, 0, 0);
}

bool ARPTHA::equals(const OXMTLV &other) {
  if (const ARPTHA *field = dynamic_cast<const ARPTHA *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &ARPTHA::operator=(const OXMTLV &field) {
  const ARPTHA &arp = dynamic_cast<const ARPTHA &>(field);
  OXMTLV::operator=(field);
  this->value_ = arp.value_;
  this->mask_ = arp.mask_;
  return *this;
};

size_t ARPTHA::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), this->mask_.get_data(),
           len);
  }
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, this->value_.get_data(), len);
  return 0;
}

of_error ARPTHA::unpack(uint8_t *buffer) {
  uint8_t v[OFP_ETH_ALEN];
  OXMTLV::unpack(buffer);
  memcpy(v, buffer + of13::OFP_OXM_HEADER_LEN, OFP_ETH_ALEN);
  this->value_ = EthAddress(v);
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    memcpy(v, buffer + of13::OFP_OXM_HEADER_LEN + len, OFP_ETH_ALEN);
    this->mask_ = EthAddress(v);
  }
  return 0;
}

IPv6Src::IPv6Src()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_SRC, false,
             of13::OFP_OXM_IPV6_LEN) {
  create_oxm_req(0, 0x86dd, 0, 0);
}

IPv6Src::IPv6Src(IPAddress value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_SRC, false,
             of13::OFP_OXM_IPV6_LEN) {
  this->value_ = value;
  create_oxm_req(0, 0x86dd, 0, 0);
}

IPv6Src::IPv6Src(IPAddress value, IPAddress mask)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_SRC, true,
             of13::OFP_OXM_IPV6_LEN) {
  this->value_ = value;
  this->mask_ = mask;
  create_oxm_req(0, 0x86dd, 0, 0);
}

bool IPv6Src::equals(const OXMTLV &other) {
  if (const IPv6Src *field = dynamic_cast<const IPv6Src *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &IPv6Src::operator=(const OXMTLV &field) {
  const IPv6Src &ipv6 = dynamic_cast<const IPv6Src &>(field);
  OXMTLV::operator=(field);
  this->value_ = ipv6.value_;
  this->mask_ = ipv6.mask_;
  return *this;
};

size_t IPv6Src::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), this->mask_.getIPv6(),
           len);
  }
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, this->value_.getIPv6(), len);
  return 0;
}

of_error IPv6Src::unpack(uint8_t *buffer) {
  // uint8_t *ip = buffer + of13::OFP_OXM_HEADER_LEN;
  struct in6_addr *ip = (struct in6_addr *)(buffer + of13::OFP_OXM_HEADER_LEN);
  OXMTLV::unpack(buffer);
  this->value_ = IPAddress(*ip);
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    ip += 1;
    this->mask_ = IPAddress(*ip);
  }
  return 0;
}

IPv6Dst::IPv6Dst()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_DST, false,
             of13::OFP_OXM_IPV6_LEN) {
  create_oxm_req(0, 0x86dd, 0, 0);
}

IPv6Dst::IPv6Dst(IPAddress value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_DST, false,
             of13::OFP_OXM_IPV6_LEN) {
  this->value_ = value;
  create_oxm_req(0, 0x86dd, 0, 0);
}

IPv6Dst::IPv6Dst(IPAddress value, IPAddress mask)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_DST, true,
             of13::OFP_OXM_IPV6_LEN) {
  this->value_ = value;
  this->mask_ = mask;
  create_oxm_req(0, 0x86dd, 0, 0);
}

bool IPv6Dst::equals(const OXMTLV &other) {
  if (const IPv6Dst *field = dynamic_cast<const IPv6Dst *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &IPv6Dst::operator=(const OXMTLV &field) {
  const IPv6Dst &ipv6 = dynamic_cast<const IPv6Dst &>(field);
  OXMTLV::operator=(field);
  this->value_ = ipv6.value_;
  this->mask_ = ipv6.mask_;
  return *this;
};

size_t IPv6Dst::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), this->mask_.getIPv6(),
           len);
  }
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, this->value_.getIPv6(), len);
  return 0;
}

of_error IPv6Dst::unpack(uint8_t *buffer) {
  struct in6_addr *ip = (struct in6_addr *)(buffer + of13::OFP_OXM_HEADER_LEN);
  // uint8_t *ip = buffer + of13::OFP_OXM_HEADER_LEN;
  OXMTLV::unpack(buffer);
  this->value_ = IPAddress(*ip);
  if (this->has_mask_) {
    ip += 1;
    this->mask_ = IPAddress(*ip);
  }
  return 0;
}

IPV6Flabel::IPV6Flabel()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_FLABEL, false,
             of13::OFP_OXM_IPV6_FLABEL_LEN) {
  create_oxm_req(0, 0x86dd, 0, 0);
}

IPV6Flabel::IPV6Flabel(uint32_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_FLABEL, false,
             of13::OFP_OXM_IPV6_FLABEL_LEN) {
  this->value_ = value;
  create_oxm_req(0, 0x86dd, 0, 0);
}

IPV6Flabel::IPV6Flabel(uint32_t value, uint32_t mask)
    : mask_(0),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_FLABEL, true,
             of13::OFP_OXM_IPV6_FLABEL_LEN) {
  this->value_ = value;
  this->mask_ = mask;
  create_oxm_req(0, 0x86dd, 0, 0);
}

bool IPV6Flabel::equals(const OXMTLV &other) {
  if (const IPV6Flabel *field = dynamic_cast<const IPV6Flabel *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &IPV6Flabel::operator=(const OXMTLV &field) {
  const IPV6Flabel &ipv6 = dynamic_cast<const IPV6Flabel &>(field);
  OXMTLV::operator=(field);
  this->value_ = ipv6.value_;
  this->mask_ = ipv6.mask_;
  return *this;
};

size_t IPV6Flabel::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    uint32_t mask = hton32(this->mask_);
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), &mask, len);
  }
  uint32_t value = hton32(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, len);
  return 0;
}

of_error IPV6Flabel::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh32(*((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    this->mask_ =
        ntoh32(*((uint32_t *)(buffer + (of13::OFP_OXM_HEADER_LEN + len))));
  }
  return 0;
}

ICMPv6Type::ICMPv6Type()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ICMPV6_TYPE, false,
             of13::OFP_OXM_ICMP_TYPE_LEN) {
  create_oxm_req(0, 0x86dd, 58, 0);
}

ICMPv6Type::ICMPv6Type(uint8_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ICMPV6_TYPE, false,
             of13::OFP_OXM_ICMP_TYPE_LEN) {
  this->value_ = value;
  create_oxm_req(0, 0x86dd, 58, 0);
}

bool ICMPv6Type::equals(const OXMTLV &other) {
  if (const ICMPv6Type *field = dynamic_cast<const ICMPv6Type *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &ICMPv6Type::operator=(const OXMTLV &field) {
  const ICMPv6Type &ipv6 = dynamic_cast<const ICMPv6Type &>(field);
  OXMTLV::operator=(field);
  this->value_ = ipv6.value_;
  return *this;
};

size_t ICMPv6Type::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &this->value_, this->length_);
  return 0;
}

of_error ICMPv6Type::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = *((uint8_t *)(buffer + of13::OFP_OXM_HEADER_LEN));
  return 0;
}

ICMPv6Code::ICMPv6Code()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ICMPV6_CODE, false,
             of13::OFP_OXM_ICMP_CODE_LEN) {
  create_oxm_req(0, 0x86dd, 58, 0);
}

ICMPv6Code::ICMPv6Code(uint8_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_ICMPV6_CODE, false,
             of13::OFP_OXM_ICMP_CODE_LEN) {
  this->value_ = value;
  create_oxm_req(0, 0x86dd, 58, 0);
}

bool ICMPv6Code::equals(const OXMTLV &other) {
  if (const ICMPv6Code *field = dynamic_cast<const ICMPv6Code *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &ICMPv6Code::operator=(const OXMTLV &field) {
  const ICMPv6Code &ipv6 = dynamic_cast<const ICMPv6Code &>(field);
  OXMTLV::operator=(field);
  this->value_ = ipv6.value_;
  return *this;
};

size_t ICMPv6Code::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &this->value_, this->length_);
  return 0;
}

of_error ICMPv6Code::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = *((uint8_t *)(buffer + of13::OFP_OXM_HEADER_LEN));
  return 0;
}

IPv6NDTarget::IPv6NDTarget()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_ND_TARGET,
             false, of13::OFP_OXM_IPV6_LEN) {
  create_oxm_req(0, 0x86dd, 58, 135);
}

IPv6NDTarget::IPv6NDTarget(IPAddress value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_ND_TARGET,
             false, of13::OFP_OXM_IPV6_LEN) {
  this->value_ = value;
  create_oxm_req(0, 0x86dd, 58, 135);
}

bool IPv6NDTarget::equals(const OXMTLV &other) {
  if (const IPv6NDTarget *field = dynamic_cast<const IPv6NDTarget *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &IPv6NDTarget::operator=(const OXMTLV &field) {
  const IPv6NDTarget &ipv6 = dynamic_cast<const IPv6NDTarget &>(field);
  OXMTLV::operator=(field);
  this->value_ = ipv6.value_;
  return *this;
};

size_t IPv6NDTarget::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, this->value_.getIPv6(),
         this->length_);
  return 0;
}

of_error IPv6NDTarget::unpack(uint8_t *buffer) {
  struct in6_addr *ip = (struct in6_addr *)(buffer + of13::OFP_OXM_HEADER_LEN);
  OXMTLV::unpack(buffer);
  this->value_ = IPAddress(*ip);
  return 0;
}

IPv6NDTLL::IPv6NDTLL()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_ND_TLL, false,
             OFP_ETH_ALEN) {
  create_oxm_req(0, 0x86dd, 58, 136);
}

IPv6NDTLL::IPv6NDTLL(EthAddress value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_ND_TLL, false,
             OFP_ETH_ALEN) {
  this->value_ = value;
  create_oxm_req(0, 0x86dd, 58, 136);
}

bool IPv6NDTLL::equals(const OXMTLV &other) {
  if (const IPv6NDTLL *field = dynamic_cast<const IPv6NDTLL *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &IPv6NDTLL::operator=(const OXMTLV &field) {
  const IPv6NDTLL &ipv6 = dynamic_cast<const IPv6NDTLL &>(field);
  OXMTLV::operator=(field);
  this->value_ = ipv6.value_;
  return *this;
};

size_t IPv6NDTLL::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, this->value_.get_data(),
         this->length_);
  return 0;
}

of_error IPv6NDTLL::unpack(uint8_t *buffer) {
  uint8_t v[OFP_ETH_ALEN];
  OXMTLV::unpack(buffer);
  memcpy(v, buffer + of13::OFP_OXM_HEADER_LEN, OFP_ETH_ALEN);
  this->value_ = EthAddress(v);
  return 0;
}

IPv6NDSLL::IPv6NDSLL()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_ND_SLL, false,
             OFP_ETH_ALEN) {
  create_oxm_req(0, 0x86dd, 58, 136);
}

IPv6NDSLL::IPv6NDSLL(EthAddress value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_ND_SLL, false,
             OFP_ETH_ALEN) {
  this->value_ = value;
  create_oxm_req(0, 0x86dd, 58, 136);
}

bool IPv6NDSLL::equals(const OXMTLV &other) {
  if (const IPv6NDSLL *field = dynamic_cast<const IPv6NDSLL *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &IPv6NDSLL::operator=(const OXMTLV &field) {
  const IPv6NDSLL &ipv6 = dynamic_cast<const IPv6NDSLL &>(field);
  OXMTLV::operator=(field);
  this->value_ = ipv6.value_;
  return *this;
};

size_t IPv6NDSLL::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, this->value_.get_data(),
         this->length_);
  return 0;
}

of_error IPv6NDSLL::unpack(uint8_t *buffer) {
  uint8_t v[OFP_ETH_ALEN];
  OXMTLV::unpack(buffer);
  memcpy(v, buffer + of13::OFP_OXM_HEADER_LEN, OFP_ETH_ALEN);
  this->value_ = EthAddress(v);
  return 0;
}

MPLSLabel::MPLSLabel()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_MPLS_LABEL, false,
             of13::OFP_OXM_MPLS_LABEL_LEN) {
  create_oxm_req(0x8847, 0x8848, 0, 0);
}

MPLSLabel::MPLSLabel(uint32_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_MPLS_LABEL, false,
             of13::OFP_OXM_MPLS_LABEL_LEN) {
  this->value_ = value;
  create_oxm_req(0x8847, 0x8848, 0, 0);
}

bool MPLSLabel::equals(const OXMTLV &other) {
  if (const MPLSLabel *field = dynamic_cast<const MPLSLabel *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &MPLSLabel::operator=(const OXMTLV &field) {
  const MPLSLabel &mpls = dynamic_cast<const MPLSLabel &>(field);
  OXMTLV::operator=(field);
  this->value_ = mpls.value_;
  return *this;
};

size_t MPLSLabel::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  uint32_t value = hton32(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, this->length_);
  return 0;
}

of_error MPLSLabel::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh32(*((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  return 0;
}

MPLSTC::MPLSTC()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_MPLS_TC, false,
             of13::OFP_OXM_MPLS_TC_LEN) {
  create_oxm_req(0x8847, 0x8848, 0, 0);
}

MPLSTC::MPLSTC(uint8_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_MPLS_TC, false,
             of13::OFP_OXM_MPLS_TC_LEN) {
  this->value_ = value;
  create_oxm_req(0x8847, 0x8848, 0, 0);
}

bool MPLSTC::equals(const OXMTLV &other) {
  if (const MPLSTC *field = dynamic_cast<const MPLSTC *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &MPLSTC::operator=(const OXMTLV &field) {
  const MPLSTC &mpls = dynamic_cast<const MPLSTC &>(field);
  OXMTLV::operator=(field);
  this->value_ = mpls.value_;
  return *this;
};

size_t MPLSTC::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &this->value_, this->length_);
  return 0;
}

of_error MPLSTC::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = *((uint8_t *)(buffer + of13::OFP_OXM_HEADER_LEN));
  return 0;
}

MPLSBOS::MPLSBOS()
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_MPLS_BOS, false,
             of13::OFP_OXM_MPLS_BOS_LEN) {
  create_oxm_req(0x8847, 0x8848, 0, 0);
}

MPLSBOS::MPLSBOS(uint8_t value)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_MPLS_BOS, false,
             of13::OFP_OXM_MPLS_BOS_LEN) {
  this->value_ = value;
  create_oxm_req(0x8847, 0x8848, 0, 0);
}

bool MPLSBOS::equals(const OXMTLV &other) {
  if (const MPLSBOS *field = dynamic_cast<const MPLSBOS *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_));
  } else {
    return false;
  }
}

OXMTLV &MPLSBOS::operator=(const OXMTLV &field) {
  const MPLSBOS &mpls = dynamic_cast<const MPLSBOS &>(field);
  OXMTLV::operator=(field);
  this->value_ = mpls.value_;
  return *this;
};

size_t MPLSBOS::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &this->value_, this->length_);
  return 0;
}

of_error MPLSBOS::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = *((uint8_t *)(buffer + of13::OFP_OXM_HEADER_LEN));
  return 0;
}

PBBIsid::PBBIsid()
    : mask_(0),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_PBB_ISID, false,
             of13::OFP_OXM_IPV6_PBB_ISID_LEN) {
  create_oxm_req(0x88E7, 0, 0, 0);
}

PBBIsid::PBBIsid(uint32_t value)
    : mask_(0),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_PBB_ISID, false,
             of13::OFP_OXM_IPV6_PBB_ISID_LEN) {
  this->value_ = value;
  create_oxm_req(0x88E7, 0, 0, 0);
}

PBBIsid::PBBIsid(uint32_t value, uint32_t mask)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_PBB_ISID, true,
             of13::OFP_OXM_IPV6_PBB_ISID_LEN) {
  this->value_ = value;
  this->mask_ = mask;
  create_oxm_req(0x88E7, 0, 0, 0);
}

bool PBBIsid::equals(const OXMTLV &other) {
  if (const PBBIsid *field = dynamic_cast<const PBBIsid *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &PBBIsid::operator=(const OXMTLV &field) {
  const PBBIsid &pbb = dynamic_cast<const PBBIsid &>(field);
  OXMTLV::operator=(field);
  this->value_ = pbb.value_;
  this->mask_ = pbb.mask_;
  return *this;
};

size_t PBBIsid::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    uint32_t mask = hton32(this->mask_);
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), &mask, len);
  }
  uint32_t value = hton32(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, len);
  return 0;
}

of_error PBBIsid::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh32(*((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    this->mask_ =
        ntoh32(*((uint32_t *)(buffer + (of13::OFP_OXM_HEADER_LEN + len))));
  }
  return 0;
}

TUNNELId::TUNNELId()
    : mask_(0),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_TUNNEL_ID, false,
             of13::OFP_OXM_TUNNEL_ID_LEN) {
  create_oxm_req(0, 0, 0, 0);
}

TUNNELId::TUNNELId(uint64_t value)
    : mask_(0),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_TUNNEL_ID, false,
             of13::OFP_OXM_TUNNEL_ID_LEN) {
  this->value_ = value;
  create_oxm_req(0, 0, 0, 0);
}

TUNNELId::TUNNELId(uint64_t value, uint64_t mask)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFP_OXM_TUNNEL_ID_LEN, true,
             of13::OFP_OXM_TUNNEL_ID_LEN) {
  this->value_ = value;
  this->mask_ = mask;
  create_oxm_req(0, 0, 0, 0);
}

bool TUNNELId::equals(const OXMTLV &other) {
  if (const TUNNELId *field = dynamic_cast<const TUNNELId *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &TUNNELId::operator=(const OXMTLV &field) {
  const TUNNELId &tunnel = dynamic_cast<const TUNNELId &>(field);
  OXMTLV::operator=(field);
  this->value_ = tunnel.value_;
  this->mask_ = tunnel.mask_;
  return *this;
}

size_t TUNNELId::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    uint64_t mask = hton64(this->mask_);
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), &mask, len);
  }
  uint64_t value = hton64(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, len);
  return 0;
}

of_error TUNNELId::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh64(*((uint64_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    this->mask_ =
        ntoh64(*((uint64_t *)(buffer + (of13::OFP_OXM_HEADER_LEN + len))));
  }
  return 0;
}

TunnelIPv4Dst::TunnelIPv4Dst()
    : OXMTLV(of13::OFPXMC_NXM_1, of13::NXM_TUNNEL_IPV4_DST, false,
             of13::OFP_OXM_IPV4_LEN),
      value_((uint32_t)0),
      mask_((uint32_t)0) {
  create_oxm_req(0x0800, 0, 0, 0);
}

TunnelIPv4Dst::TunnelIPv4Dst(IPAddress value)
    : OXMTLV(of13::OFPXMC_NXM_1, of13::NXM_TUNNEL_IPV4_DST, false,
             of13::OFP_OXM_IPV4_LEN),
      value_(value),
      mask_((uint32_t)0) {
  create_oxm_req(0x0800, 0, 0, 0);
}

TunnelIPv4Dst::TunnelIPv4Dst(IPAddress value, IPAddress mask)
    : OXMTLV(of13::OFPXMC_NXM_1, of13::NXM_TUNNEL_IPV4_DST, true,
             of13::OFP_OXM_IPV4_LEN),
      value_(value),
      mask_(mask) {
  create_oxm_req(0x0800, 0, 0, 0);
}

bool TunnelIPv4Dst::equals(const OXMTLV &other) {
  if (const TunnelIPv4Dst *field =
          dynamic_cast<const TunnelIPv4Dst *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &TunnelIPv4Dst::operator=(const OXMTLV &field) {
  const TunnelIPv4Dst &dst = dynamic_cast<const TunnelIPv4Dst &>(field);
  OXMTLV::operator=(field);
  this->value_ = dst.value_;
  this->mask_ = dst.mask_;
  return *this;
}

size_t TunnelIPv4Dst::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    uint32_t ip_mask = this->mask_.getIPv4();
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), &ip_mask, len);
  }
  uint32_t ip = this->value_.getIPv4();
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &ip, len);
  return 0;
}

of_error TunnelIPv4Dst::unpack(uint8_t *buffer) {
  uint32_t ip = *((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN));
  OXMTLV::unpack(buffer);
  this->value_ = IPAddress(ip);
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    uint32_t ip_mask = *((uint32_t *)(buffer + of13::OFP_OXM_HEADER_LEN + len));
    this->mask_ = IPAddress(ip_mask);
  }
  return 0;
}

IPv6Exthdr::IPv6Exthdr()
    : mask_(0),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_EXTHDR, false,
             of13::OFP_OXM_IPV6_EXTHDR_LEN) {
  create_oxm_req(0, 0x86dd, 0, 0);
}

IPv6Exthdr::IPv6Exthdr(uint16_t value)
    : mask_(0),
      OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_EXTHDR, false,
             of13::OFP_OXM_IPV6_EXTHDR_LEN) {
  this->value_ = value;
  create_oxm_req(0, 0x86dd, 0, 0);
}

IPv6Exthdr::IPv6Exthdr(uint16_t value, uint16_t mask)
    : OXMTLV(of13::OFPXMC_OPENFLOW_BASIC, of13::OFPXMT_OFB_IPV6_EXTHDR, true,
             of13::OFP_OXM_IPV6_EXTHDR_LEN) {
  this->mask_ = mask;
  this->value_ = value;
  create_oxm_req(0, 0x86dd, 0, 0);
}

bool IPv6Exthdr::equals(const OXMTLV &other) {
  if (const IPv6Exthdr *field = dynamic_cast<const IPv6Exthdr *>(&other)) {
    return ((OXMTLV::equals(other)) && (this->value_ == field->value_) &&
            (this->has_mask_ ? this->mask_ == field->mask_ : true));
  } else {
    return false;
  }
}

OXMTLV &IPv6Exthdr::operator=(const OXMTLV &field) {
  const IPv6Exthdr &hdr = dynamic_cast<const IPv6Exthdr &>(field);
  OXMTLV::operator=(field);
  this->value_ = hdr.value_;
  this->mask_ = hdr.mask_;
  return *this;
};

size_t IPv6Exthdr::pack(uint8_t *buffer) {
  OXMTLV::pack(buffer);
  size_t len = this->length_;
  if (this->has_mask_) {
    len = this->length_ / 2;
    uint16_t mask = hton16(this->mask_);
    memcpy(buffer + (of13::OFP_OXM_HEADER_LEN + len), &mask, len);
  }
  uint16_t value = hton16(this->value_);
  memcpy(buffer + of13::OFP_OXM_HEADER_LEN, &value, len);
  return 0;
}

of_error IPv6Exthdr::unpack(uint8_t *buffer) {
  OXMTLV::unpack(buffer);
  this->value_ = ntoh16(*((uint16_t *)(buffer + of13::OFP_OXM_HEADER_LEN)));
  if (this->has_mask_) {
    size_t len = this->length_ / 2;
    this->mask_ =
        ntoh16(*((uint16_t *)(buffer + (of13::OFP_OXM_HEADER_LEN + len))));
  }
  return 0;
}

Match::Match() {
  //: oxm_tlvs_(OXM_NUM) {
  this->type_ = of13::OFPMT_OXM;
  this->length_ = sizeof(struct of13::ofp_match) - 4;
  memset(oxm_tlvs_, 0, sizeof(oxm_tlvs_));
}

Match::Match(const Match &match) {
  this->type_ = match.type_;
  this->length_ = 4;
  memset(oxm_tlvs_, 0, sizeof(oxm_tlvs_));
  // this->oxm_tlvs_.reserve(OXM_NUM);
  for (std::vector<uint8_t>::const_iterator it = match.curr_tlvs_.begin();
       it != match.curr_tlvs_.end(); ++it) {
    this->curr_tlvs_.push_back((*it));
    this->oxm_tlvs_[*it] = match.oxm_tlvs_[*it]->clone();
    this->length_ += of13::OFP_OXM_HEADER_LEN + this->oxm_tlvs_[*it]->length();
  }
}

Match &Match::operator=(Match other) {
  swap(*this, other);
  return *this;
}

void Match::swap(Match &first, Match &second) {
  std::swap(first.type_, second.type_);
  std::swap(first.length_, second.length_);
  std::swap(first.oxm_tlvs_, second.oxm_tlvs_);
  std::swap(first.curr_tlvs_, second.curr_tlvs_);
}

Match::~Match() {
  for (std::vector<uint8_t>::iterator it = this->curr_tlvs_.begin();
       it != this->curr_tlvs_.end(); ++it) {
    delete this->oxm_tlvs_[*it];
  }
}

OXMTLV *Match::make_oxm_tlv(uint8_t field) {
  switch (field) {
    case (of13::OFPXMT_OFB_IN_PORT): {
      return new InPort();
    }
    case (of13::OFPXMT_OFB_IN_PHY_PORT): {
      return new InPhyPort();
    }
    case (of13::OFPXMT_OFB_METADATA): {
      return new Metadata();
    }
    case (of13::OFPXMT_OFB_ETH_SRC): {
      return new EthSrc();
    }
    case (of13::OFPXMT_OFB_ETH_DST): {
      return new EthDst();
    }
    case (of13::OFPXMT_OFB_ETH_TYPE): {
      return new EthType();
    }
    case (of13::OFPXMT_OFB_VLAN_VID): {
      return new VLANVid();
    }
    case (of13::OFPXMT_OFB_VLAN_PCP): {
      return new VLANPcp();
    }
    case (of13::OFPXMT_OFB_IP_DSCP): {
      return new IPDSCP();
    }
    case (of13::OFPXMT_OFB_IP_ECN): {
      return new IPECN();
    }
    case (of13::OFPXMT_OFB_IP_PROTO): {
      return new IPProto();
    }
    case (of13::OFPXMT_OFB_IPV4_SRC): {
      return new IPv4Src();
    }
    case (of13::OFPXMT_OFB_IPV4_DST): {
      return new IPv4Dst();
    }
    case (of13::OFPXMT_OFB_TCP_SRC): {
      return new TCPSrc();
    }
    case (of13::OFPXMT_OFB_TCP_DST): {
      return new TCPDst();
    }
    case (of13::OFPXMT_OFB_UDP_SRC): {
      return new UDPSrc();
    }
    case (of13::OFPXMT_OFB_UDP_DST): {
      return new UDPDst();
    }
    case (of13::OFPXMT_OFB_SCTP_SRC): {
      return new SCTPSrc();
    }
    case (of13::OFPXMT_OFB_SCTP_DST): {
      return new SCTPDst();
    }
    case (of13::OFPXMT_OFB_ICMPV4_TYPE): {
      return new ICMPv4Type();
    }
    case (of13::OFPXMT_OFB_ICMPV4_CODE): {
      return new ICMPv4Code();
    }
    case (of13::OFPXMT_OFB_ARP_OP): {
      return new ARPOp();
    }
    case (of13::OFPXMT_OFB_ARP_SPA): {
      return new ARPSPA();
    }
    case (of13::OFPXMT_OFB_ARP_TPA): {
      return new ARPTPA();
    }
    case (of13::OFPXMT_OFB_ARP_SHA): {
      return new ARPSHA();
    }
    case (of13::OFPXMT_OFB_ARP_THA): {
      return new ARPTHA();
    }
    case (of13::OFPXMT_OFB_IPV6_SRC): {
      return new IPv6Src();
    }
    case (of13::OFPXMT_OFB_IPV6_DST): {
      return new IPv6Dst();
    }
    case (of13::OFPXMT_OFB_IPV6_FLABEL): {
      return new IPV6Flabel();
    }
    case (of13::OFPXMT_OFB_ICMPV6_TYPE): {
      return new ICMPv6Type();
    }
    case (of13::OFPXMT_OFB_ICMPV6_CODE): {
      return new ICMPv6Code();
    }
    case (of13::OFPXMT_OFB_IPV6_ND_TARGET): {
      return new IPv6NDTarget();
    }
    case (of13::OFPXMT_OFB_IPV6_ND_SLL): {
      return new IPv6NDSLL();
    }
    case (of13::OFPXMT_OFB_IPV6_ND_TLL): {
      return new IPv6NDTLL();
    }
    case (of13::OFPXMT_OFB_MPLS_LABEL): {
      return new MPLSLabel();
    }
    case (of13::OFPXMT_OFB_MPLS_TC): {
      return new MPLSTC();
    }
    case (of13::OFPXMT_OFB_MPLS_BOS): {
      return new MPLSBOS();
    }
    case (of13::OFPXMT_OFB_PBB_ISID): {
      return new PBBIsid();
    }
    case (of13::OFPXMT_OFB_TUNNEL_ID): {
      return new TUNNELId();
    }
    case (of13::OFPXMT_OFB_IPV6_EXTHDR): {
      return new IPv6Exthdr();
    }
  }
  return NULL;
}

bool Match::operator==(const Match &other) const {
  for (std::vector<uint8_t>::const_iterator it = this->curr_tlvs_.begin();
       it != this->curr_tlvs_.end(); ++it) {
    OXMTLV *tlv1 = this->oxm_tlvs_[*it];
    OXMTLV *tlv2 = other.oxm_tlvs_[*it];
    if (tlv2) {
      if (!tlv1->equals(*tlv2)) {
        return false;
      }
    } else {
      return false;
    }
  }
  return MatchHeader::operator==(other);
}

bool Match::operator!=(const Match &other) const { return !(*this == other); }

size_t Match::pack(uint8_t *buffer) {
  MatchHeader::pack(buffer);
  uint8_t *p = buffer + (sizeof(struct of13::ofp_match) - 4);
  std::sort(this->curr_tlvs_.begin(), this->curr_tlvs_.end());
  for (std::vector<uint8_t>::iterator it = this->curr_tlvs_.begin();
       it != this->curr_tlvs_.end(); ++it) {
    this->oxm_tlvs_[*it]->pack(p);
    p += of13::OFP_OXM_HEADER_LEN + this->oxm_tlvs_[*it]->length();
  }
  return 0;
}

of_error Match::unpack(uint8_t *buffer) {
  MatchHeader::unpack(buffer);
  size_t len = this->length_ - (sizeof(struct of13::ofp_match) - 4);
  uint8_t *p = buffer + (sizeof(struct of13::ofp_match) - 4);
  OXMTLV *oxm_tlv;
  while (len) {
    uint32_t header = ntoh32(*((uint32_t *)p));
    oxm_tlv = make_oxm_tlv(oxm_tlv->oxm_field(header));
    if (!oxm_tlv) {
      return openflow_error(OFPET_BAD_MATCH, OFPBMC_BAD_FIELD);
    }
    oxm_tlv->unpack(p);
    if (check_dup(oxm_tlv)) {
      return openflow_error(OFPET_BAD_MATCH, OFPBMC_DUP_FIELD);
    }
    if (!check_pre_req(oxm_tlv)) {
      return openflow_error(OFPET_BAD_MATCH, OFPBMC_BAD_PREREQ);
    }
    this->curr_tlvs_.push_back(oxm_tlv->field());
    this->oxm_tlvs_[oxm_tlv->field()] = oxm_tlv;
    len -= of13::OFP_OXM_HEADER_LEN + oxm_tlv->length();
    p += of13::OFP_OXM_HEADER_LEN + oxm_tlv->length();
  }
  if (len) {
    return openflow_error(OFPET_BAD_MATCH, OFPBMC_BAD_LEN);
  }
  return 0;
}

OXMTLV *Match::oxm_field(uint8_t field) { return this->oxm_tlvs_[field]; }

bool Match::check_pre_req(OXMTLV *tlv) {
  /*Check ICMP type*/
  struct oxm_req r = tlv->oxm_reqs();
  if (r.icmp_req) {
    ICMPv6Type *icmp_type = icmpv6_type();
    if (icmp_type) {
      if (icmp_type->value() != r.icmp_req) {
        return false;
      }
    } else {
      return false;
    }
  }
  if (r.ip_proto_req) {
    IPProto *proto = ip_proto();
    if (proto) {
      if (proto->value() != r.ip_proto_req) {
        return false;
      }
    } else {
      return false;
    }
  }

  /* Check for eth_type */
  if (!r.eth_type_req[0]) {
    return true;
  } else {
    EthType *type = eth_type();
    if (type) {
      if (type->value() == r.eth_type_req[0]) {
        return true;
      } else if (r.eth_type_req[1] && type->value() == r.eth_type_req[1]) {
        return true;
      }
    } else {
      return false;
    }
  }
  return false;
}

bool Match::check_dup(OXMTLV *tlv) {
  if (this->oxm_tlvs_[tlv->field()]) {
    return true;
  }
  return false;
}

void Match::add_oxm_field(OXMTLV &tlv) {
  if (check_dup(&tlv)) return;
  this->curr_tlvs_.push_back(tlv.field());
  this->oxm_tlvs_[tlv.field()] = tlv.clone();
  this->length_ += of13::OFP_OXM_HEADER_LEN + tlv.length();
}

void Match::add_oxm_field(OXMTLV *tlv) {
  if (check_dup(tlv)) return;
  this->curr_tlvs_.push_back(tlv->field());
  this->oxm_tlvs_[tlv->field()] = tlv;
  this->length_ += of13::OFP_OXM_HEADER_LEN + tlv->length();
}

InPort *Match::in_port() {
  return static_cast<InPort *>(oxm_field(of13::OFPXMT_OFB_IN_PORT));
}

InPhyPort *Match::in_phy_port() {
  return static_cast<InPhyPort *>(oxm_field(of13::OFPXMT_OFB_IN_PHY_PORT));
}

Metadata *Match::metadata() {
  return static_cast<Metadata *>(oxm_field(of13::OFPXMT_OFB_METADATA));
}

EthSrc *Match::eth_src() {
  return static_cast<EthSrc *>(oxm_field(of13::OFPXMT_OFB_ETH_SRC));
}

EthDst *Match::eth_dst() {
  return static_cast<EthDst *>(oxm_field(of13::OFPXMT_OFB_ETH_DST));
}

EthType *Match::eth_type() {
  return static_cast<EthType *>(oxm_field(of13::OFPXMT_OFB_ETH_TYPE));
}

VLANVid *Match::vlan_vid() {
  return static_cast<VLANVid *>(oxm_field(of13::OFPXMT_OFB_VLAN_VID));
}

VLANPcp *Match::vlan_pcp() {
  return static_cast<VLANPcp *>(oxm_field(of13::OFPXMT_OFB_VLAN_PCP));
}

IPDSCP *Match::ip_dscp() {
  return static_cast<IPDSCP *>(oxm_field(of13::OFPXMT_OFB_IP_DSCP));
}

IPECN *Match::ip_ecn() {
  return static_cast<IPECN *>(oxm_field(of13::OFPXMT_OFB_IP_ECN));
}

IPProto *Match::ip_proto() {
  return static_cast<IPProto *>(oxm_field(of13::OFPXMT_OFB_IP_PROTO));
}

IPv4Src *Match::ipv4_src() {
  return static_cast<IPv4Src *>(oxm_field(of13::OFPXMT_OFB_IPV4_SRC));
}

IPv4Dst *Match::ipv4_dst() {
  return static_cast<IPv4Dst *>(oxm_field(of13::OFPXMT_OFB_IPV4_DST));
}

TCPSrc *Match::tcp_src() {
  return static_cast<TCPSrc *>(oxm_field(of13::OFPXMT_OFB_TCP_SRC));
}

TCPDst *Match::tcp_dst() {
  return static_cast<TCPDst *>(oxm_field(of13::OFPXMT_OFB_TCP_DST));
}

UDPSrc *Match::udp_src() {
  return static_cast<UDPSrc *>(oxm_field(of13::OFPXMT_OFB_UDP_SRC));
}

UDPDst *Match::udp_dst() {
  return static_cast<UDPDst *>(oxm_field(of13::OFPXMT_OFB_UDP_DST));
}

SCTPSrc *Match::sctp_src() {
  return static_cast<SCTPSrc *>(oxm_field(of13::OFPXMT_OFB_SCTP_SRC));
}

SCTPDst *Match::sctp_dst() {
  return static_cast<SCTPDst *>(oxm_field(of13::OFPXMT_OFB_SCTP_DST));
}

ICMPv4Type *Match::icmpv4_type() {
  return static_cast<ICMPv4Type *>(oxm_field(of13::OFPXMT_OFB_ICMPV4_TYPE));
}

ICMPv4Code *Match::icmpv4_code() {
  return static_cast<ICMPv4Code *>(oxm_field(of13::OFPXMT_OFB_ICMPV4_CODE));
}

ARPOp *Match::arp_op() {
  return static_cast<ARPOp *>(oxm_field(of13::OFPXMT_OFB_ARP_OP));
}

ARPSPA *Match::arp_spa() {
  return static_cast<ARPSPA *>(oxm_field(of13::OFPXMT_OFB_ARP_SPA));
}

ARPTPA *Match::arp_tpa() {
  return static_cast<ARPTPA *>(oxm_field(of13::OFPXMT_OFB_ARP_TPA));
}

ARPSHA *Match::arp_sha() {
  return static_cast<ARPSHA *>(oxm_field(of13::OFPXMT_OFB_ARP_SHA));
}

ARPTHA *Match::arp_tha() {
  return static_cast<ARPTHA *>(oxm_field(of13::OFPXMT_OFB_ARP_THA));
}

IPv6Src *Match::ipv6_src() {
  return static_cast<IPv6Src *>(oxm_field(of13::OFPXMT_OFB_IPV6_SRC));
}

IPv6Dst *Match::ipv6_dst() {
  return static_cast<IPv6Dst *>(oxm_field(of13::OFPXMT_OFB_IPV6_DST));
}

IPV6Flabel *Match::ipv6_flabel() {
  return static_cast<IPV6Flabel *>(oxm_field(of13::OFPXMT_OFB_IPV6_FLABEL));
}

ICMPv6Type *Match::icmpv6_type() {
  return static_cast<ICMPv6Type *>(oxm_field(of13::OFPXMT_OFB_ICMPV6_TYPE));
}

ICMPv6Code *Match::icmpv6_code() {
  return static_cast<ICMPv6Code *>(oxm_field(of13::OFPXMT_OFB_ICMPV6_CODE));
}

IPv6NDTarget *Match::ipv6_nd_target() {
  return static_cast<IPv6NDTarget *>(
      oxm_field(of13::OFPXMT_OFB_IPV6_ND_TARGET));
}

IPv6NDSLL *Match::ipv6_nd_sll() {
  return static_cast<IPv6NDSLL *>(oxm_field(of13::OFPXMT_OFB_IPV6_ND_SLL));
}

IPv6NDTLL *Match::ipv6_nd_tll() {
  return static_cast<IPv6NDTLL *>(oxm_field(of13::OFPXMT_OFB_IPV6_ND_TLL));
}

MPLSLabel *Match::mpls_label() {
  return static_cast<MPLSLabel *>(oxm_field(of13::OFPXMT_OFB_MPLS_LABEL));
}

MPLSTC *Match::mpls_tc() {
  return static_cast<MPLSTC *>(oxm_field(of13::OFPXMT_OFB_MPLS_TC));
}

MPLSBOS *Match::mpls_bos() {
  return static_cast<MPLSBOS *>(oxm_field(of13::OFPXMT_OFB_MPLS_BOS));
}

PBBIsid *Match::pbb_isid() {
  return static_cast<PBBIsid *>(oxm_field(of13::OFPXMT_OFB_PBB_ISID));
}

TUNNELId *Match::tunnel_id() {
  return static_cast<TUNNELId *>(oxm_field(of13::OFPXMT_OFB_TUNNEL_ID));
}

IPv6Exthdr *Match::ipv6_exthdr() {
  return static_cast<IPv6Exthdr *>(oxm_field(of13::OFPXMT_OFB_IPV6_EXTHDR));
}

uint16_t Match::oxm_fields_len() {
  uint16_t len = 0;
  for (std::vector<uint8_t>::const_iterator it = this->curr_tlvs_.begin();
       it != this->curr_tlvs_.end(); ++it) {
    len += of13::OFP_OXM_HEADER_LEN + this->oxm_tlvs_[*it]->length();
  }
  return len;
}

}  // End of namespace of13
}  // End of namespace fluid_msg

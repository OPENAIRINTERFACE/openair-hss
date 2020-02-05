#include "of10msg.hh"

namespace fluid_msg {

namespace of10 {

Hello::Hello() : OFMsg(of10::OFP_VERSION, of10::OFPT_HELLO) {}

Hello::Hello(uint32_t xid) : OFMsg(of10::OFP_VERSION, of10::OFPT_HELLO, xid) {}

uint8_t *Hello::pack() { return OFMsg::pack(); }

of_error Hello::unpack(uint8_t *buffer) {
  OFMsg::unpack(buffer);
  if (this->length_ < sizeof(struct ofp_header)) {
    return openflow_error(of10::OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  }
  return 0;
}

Error::Error() : ErrorCommon(of10::OFP_VERSION, of10::OFPT_ERROR) {
  this->length_ = sizeof(struct ofp_error_msg);
}

Error::Error(uint32_t xid, uint16_t err_type, uint16_t code)
    : ErrorCommon(of10::OFP_VERSION, of10::OFPT_ERROR, xid, err_type, code) {}

Error::Error(uint32_t xid, uint16_t err_type, uint16_t code, uint8_t *data,
             size_t data_len)
    : ErrorCommon(of10::OFP_VERSION, of10::OFPT_ERROR, xid, err_type, code,
                  data, data_len) {}

EchoRequest::EchoRequest(uint32_t xid)
    : EchoCommon(of10::OFP_VERSION, of10::OFPT_ECHO_REQUEST, xid) {}

EchoReply::EchoReply(uint32_t xid)
    : EchoCommon(of10::OFP_VERSION, of10::OFPT_ECHO_REPLY, xid) {}

Vendor::Vendor() : OFMsg(of10::OFP_VERSION, of10::OFPT_VENDOR), vendor_(0) {
  this->length_ = sizeof(struct of10::ofp_vendor_header);
}

Vendor::Vendor(uint32_t xid, uint32_t vendor)
    : OFMsg(of10::OFP_VERSION, of10::OFPT_VENDOR, xid), vendor_(vendor) {
  this->length_ = sizeof(struct of10::ofp_vendor_header);
}

uint8_t *Vendor::pack() {
  uint8_t *buffer = OFMsg::pack();
  struct of10::ofp_vendor_header *v = (struct of10::ofp_vendor_header *)buffer;
  v->vendor = hton32(this->vendor_);
  return buffer;
}

of_error Vendor::unpack(uint8_t *buffer) {
  struct of10::ofp_vendor_header *v = (struct of10::ofp_vendor_header *)buffer;
  OFMsg::unpack(buffer);
  if (this->length_ < sizeof(struct of10::ofp_vendor_header)) {
    return openflow_error(of10::OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  }
  this->vendor_ = ntoh32(v->vendor);
  return 0;
}

FeaturesRequest::FeaturesRequest()
    : OFMsg(of10::OFP_VERSION, of10::OFPT_FEATURES_REQUEST) {}

FeaturesRequest::FeaturesRequest(uint32_t xid)
    : OFMsg(of10::OFP_VERSION, of10::OFPT_FEATURES_REQUEST, xid) {}

uint8_t *FeaturesRequest::pack() { return OFMsg::pack(); }

of_error FeaturesRequest::unpack(uint8_t *buffer) {
  OFMsg::unpack(buffer);
  if (this->length_ < sizeof(struct ofp_header)) {
    return openflow_error(of10::OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  }
  return 0;
}

FeaturesReply::FeaturesReply()
    : FeaturesReplyCommon(of10::OFP_VERSION, of10::OFPT_FEATURES_REPLY, 0, 0, 0,
                          0, 0) {
  this->length_ = sizeof(struct of10::ofp_switch_features);
}

FeaturesReply::FeaturesReply(uint32_t xid, uint64_t datapath_id,
                             uint32_t n_buffers, uint8_t n_tables,
                             uint32_t capabilities, uint32_t actions)
    : FeaturesReplyCommon(of10::OFP_VERSION, of10::OFPT_FEATURES_REPLY, xid,
                          datapath_id, n_buffers, n_tables, capabilities) {
  this->actions_ = actions;
  this->length_ = sizeof(struct of10::ofp_switch_features);
}

FeaturesReply::FeaturesReply(uint32_t xid, uint64_t datapath_id,
                             uint32_t n_buffers, uint8_t n_tables,
                             uint32_t capabilities, uint32_t actions,
                             std::vector<of10::Port> ports)
    : FeaturesReplyCommon(of10::OFP_VERSION, of10::OFPT_FEATURES_REPLY, xid,
                          datapath_id, n_buffers, n_tables, capabilities) {
  this->actions_ = actions;
  this->ports_ = ports;
  this->length_ = sizeof(struct of10::ofp_switch_features) + ports_length();
}

bool FeaturesReply::operator==(const FeaturesReply &other) const {
  return ((FeaturesReplyCommon::operator==(other)) &&
          (this->actions_ == other.actions_) && (this->ports_ == other.ports_));
}

bool FeaturesReply::operator!=(const FeaturesReply &other) const {
  return !(*this == other);
}

uint8_t *FeaturesReply::pack() {
  uint8_t *buffer = OFMsg::pack();
  struct of10::ofp_switch_features *fr =
      (struct of10::ofp_switch_features *)buffer;
  fr->datapath_id = hton64(this->datapath_id_);
  fr->n_buffers = hton32(this->n_buffers_);
  fr->n_tables = this->n_tables_;
  memset(fr->pad, 0x0, 3);
  fr->capabilities = hton32(this->capabilities_);
  fr->actions = hton32(this->actions_);
  uint8_t *p = buffer + sizeof(struct of10::ofp_switch_features);
  for (std::vector<of10::Port>::iterator it = this->ports_.begin(),
                                         end = this->ports_.end();
       it != end; ++it) {
    it->pack(p);
    p += sizeof(struct of10::ofp_phy_port);
  }
  return buffer;
}

of_error FeaturesReply::unpack(uint8_t *buffer) {
  OFMsg::unpack(buffer);
  struct of10::ofp_switch_features *fr =
      (struct of10::ofp_switch_features *)buffer;
  this->datapath_id_ = ntoh64(fr->datapath_id);
  this->n_buffers_ = ntoh32(fr->n_buffers);
  this->n_tables_ = fr->n_tables;
  this->capabilities_ = ntoh32(fr->capabilities);
  this->actions_ = ntoh32(fr->actions);
  size_t len = this->length_ - sizeof(struct of10::ofp_switch_features);
  uint8_t *p = buffer + sizeof(struct of10::ofp_switch_features);
  while (len) {
    of10::Port port;
    port.unpack(p);
    len -= sizeof(struct of10::ofp_phy_port);
    this->ports_.push_back(port);
    p += sizeof(struct of10::ofp_phy_port);
  }
  return 0;
}

void FeaturesReply::ports(std::vector<of10::Port> ports) {
  this->ports_ = ports;
  this->length_ += ports_length();
}

size_t FeaturesReply::ports_length() {
  return this->ports_.size() * sizeof(struct of10::ofp_phy_port);
}

void FeaturesReply::add_port(of10::Port port) {
  this->ports_.push_back(port);
  this->length_ += sizeof(struct of10::ofp_phy_port);
}

GetConfigRequest::GetConfigRequest()
    : OFMsg(of10::OFP_VERSION, of10::OFPT_GET_CONFIG_REQUEST) {}

GetConfigRequest::GetConfigRequest(uint32_t xid)
    : OFMsg(of10::OFP_VERSION, of10::OFPT_GET_CONFIG_REQUEST, xid) {}

uint8_t *GetConfigRequest::pack() { return OFMsg::pack(); }

of_error GetConfigRequest::unpack(uint8_t *buffer) {
  OFMsg::unpack(buffer);
  if (this->length_ < sizeof(struct ofp_header)) {
    return openflow_error(of10::OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  }
  return 0;
}

GetConfigReply::GetConfigReply()
    : SwitchConfigCommon(of10::OFP_VERSION, of10::OFPT_GET_CONFIG_REPLY, 0, 0,
                         0) {}

GetConfigReply::GetConfigReply(uint32_t xid, uint16_t flags,
                               uint16_t miss_send_len)
    : SwitchConfigCommon(of10::OFP_VERSION, of10::OFPT_GET_CONFIG_REPLY, xid,
                         flags, miss_send_len) {}

SetConfig::SetConfig()
    : SwitchConfigCommon(of10::OFP_VERSION, of10::OFPT_SET_CONFIG){};

SetConfig::SetConfig(uint32_t xid, uint16_t flags, uint16_t miss_send_len)
    : SwitchConfigCommon(of10::OFP_VERSION, of10::OFPT_SET_CONFIG, xid, flags,
                         miss_send_len) {}

FlowMod::FlowMod()
    : FlowModCommon(of10::OFP_VERSION, of10::OFPT_FLOW_MOD),
      command_(0),
      out_port_(0) {
  this->length_ = sizeof(struct of10::ofp_flow_mod);
}

FlowMod::FlowMod(uint32_t xid, uint64_t cookie, uint16_t command,
                 uint16_t idle_timeout, uint16_t hard_timeout,
                 uint16_t priority, uint32_t buffer_id, uint16_t out_port,
                 uint16_t flags)
    : FlowModCommon(of10::OFP_VERSION, of10::OFPT_FLOW_MOD, xid, cookie,
                    idle_timeout, hard_timeout, priority, buffer_id, flags),
      command_(command),
      out_port_(out_port) {
  this->length_ = sizeof(struct of10::ofp_flow_mod);
}

FlowMod::FlowMod(uint32_t xid, uint64_t cookie, uint16_t command,
                 uint16_t idle_timeout, uint16_t hard_timeout,
                 uint16_t priority, uint32_t buffer_id, uint16_t out_port,
                 uint16_t flags, of10::Match match)
    : FlowModCommon(of10::OFP_VERSION, of10::OFPT_FLOW_MOD, xid, cookie,
                    idle_timeout, hard_timeout, priority, buffer_id, flags),
      command_(command),
      out_port_(out_port),
      match_(match) {
  this->length_ = sizeof(struct of10::ofp_flow_mod);
}

FlowMod::FlowMod(uint32_t xid, uint64_t cookie, uint16_t command,
                 uint16_t idle_timeout, uint16_t hard_timeout,
                 uint16_t priority, uint32_t buffer_id, uint16_t out_port,
                 uint16_t flags, of10::Match match, ActionList actions)
    : FlowModCommon(of10::OFP_VERSION, of10::OFPT_FLOW_MOD, xid, cookie,
                    idle_timeout, hard_timeout, priority, buffer_id, flags),
      command_(command),
      out_port_(out_port),
      match_(match),
      actions_(actions) {
  this->length_ = sizeof(struct of10::ofp_flow_mod) + actions.length();
}

bool FlowMod::operator==(const FlowMod &other) const {
  return ((OFMsg::operator==(other)) && (this->command_ == other.command_) &&
          (this->out_port_ == other.out_port_) &&
          (this->match_ == other.match_) && (this->actions_ == other.actions_));
}

bool FlowMod::operator!=(const FlowMod &other) const {
  return !(*this == other);
}

uint8_t *FlowMod::pack() {
  uint8_t *buffer = OFMsg::pack();
  struct of10::ofp_flow_mod *fm = (struct of10::ofp_flow_mod *)buffer;
  this->match_.pack(buffer + sizeof(struct ofp_header));
  fm->cookie = hton64(this->cookie_);
  fm->command = hton16(this->command_);
  fm->idle_timeout = hton16(this->idle_timeout_);
  fm->hard_timeout = hton16(this->hard_timeout_);
  fm->priority = hton16(this->priority_);
  fm->buffer_id = hton32(this->buffer_id_);
  fm->out_port = hton16(this->out_port_);
  fm->flags = hton16(this->flags_);
  this->actions_.pack(buffer + sizeof(struct of10::ofp_flow_mod));
  return buffer;
}

of_error FlowMod::unpack(uint8_t *buffer) {
  OFMsg::unpack(buffer);
  struct of10::ofp_flow_mod *fm = (struct of10::ofp_flow_mod *)buffer;
  if (fm->header.length < sizeof(struct of10::ofp_flow_mod)) {
    return openflow_error(of10::OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  }
  this->match_.unpack(buffer + sizeof(struct ofp_header));
  this->cookie_ = ntoh64(fm->cookie);
  this->command_ = ntoh16(fm->command);
  this->idle_timeout_ = ntoh16(fm->idle_timeout);
  this->hard_timeout_ = ntoh16(fm->hard_timeout);
  this->priority_ = ntoh16(fm->priority);
  this->buffer_id_ = ntoh32(fm->buffer_id);
  this->out_port_ = ntoh16(fm->out_port);
  this->flags_ = ntoh16(fm->flags);
  this->actions_.length(this->length_ - sizeof(struct of10::ofp_flow_mod));
  this->actions_.unpack10(buffer + sizeof(struct of10::ofp_flow_mod));
  return 0;
}

void FlowMod::actions(const ActionList &actions) {
  this->actions_ = actions;
  this->length_ += this->actions_.length();
}

void FlowMod::add_action(Action &action) {
  this->actions_.add_action(action);
  this->length_ += action.length();
}

void FlowMod::add_action(Action *action) {
  this->actions_.add_action(action);
  this->length_ += action->length();
}

PacketOut::PacketOut()
    : PacketOutCommon(of10::OFP_VERSION, of10::OFPT_PACKET_OUT), in_port_(0) {
  this->length_ = sizeof(struct of10::ofp_packet_out);
}

PacketOut::PacketOut(uint32_t xid, uint32_t buffer_id, uint16_t in_port)
    : PacketOutCommon(of10::OFP_VERSION, of10::OFPT_PACKET_OUT, xid, buffer_id),
      in_port_(in_port) {
  this->length_ = sizeof(struct of10::ofp_packet_out);
}

bool PacketOut::operator==(const PacketOut &other) const {
  return ((PacketOutCommon::operator==(other)) &&
          (this->in_port_ == other.in_port_));
}

bool PacketOut::operator!=(const PacketOut &other) const {
  return !(*this == other);
}

uint8_t *PacketOut::pack() {
  uint8_t *buffer = OFMsg::pack();
  struct of10::ofp_packet_out *po = (struct of10::ofp_packet_out *)buffer;
  po->buffer_id = hton32(this->buffer_id_);
  po->in_port = hton16(this->in_port_);
  po->actions_len = hton16(this->actions_len_);
  this->actions_.pack(buffer + sizeof(struct of10::ofp_packet_out));
  this->data_len_ = this->length_ -
                    (sizeof(struct of10::ofp_packet_out) + this->actions_len_);
  if (this->buffer_id_ == of10::OFP_NO_BUFFER) {
    uint8_t *p =
        buffer + sizeof(struct of10::ofp_packet_out) + this->actions_len_;
    memcpy(p, this->data_, this->data_len_);
  }
  return buffer;
}

of_error PacketOut::unpack(uint8_t *buffer) {
  struct of10::ofp_packet_out *po = (struct of10::ofp_packet_out *)buffer;
  OFMsg::unpack(buffer);
  if (this->length_ < sizeof(struct of10::ofp_packet_out)) {
    return openflow_error(of10::OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  }
  this->buffer_id_ = ntoh32(po->buffer_id);
  this->in_port_ = ntoh16(po->in_port);
  this->actions_len_ = ntoh16(po->actions_len);
  this->actions_.length(this->actions_len_);
  uint8_t *p = buffer + sizeof(struct of10::ofp_packet_out);
  this->actions_.unpack10(p);
  this->data_len_ = this->length_ -
                    (sizeof(struct of10::ofp_packet_out) + this->actions_len_);
  if (this->buffer_id_ == of10::OFP_NO_BUFFER) {
    /*Reuse p to calculate the packet data position */
    p = buffer + sizeof(struct of10::ofp_packet_out) + this->actions_len_;
    if (this->data_len_) {
      this->data_ = ::operator new(this->data_len_);
      memcpy(this->data_, p, this->data_len_);
    }
  }
  return 0;
}

PacketIn::PacketIn()
    : PacketInCommon(of10::OFP_VERSION, of10::OFPT_PACKET_IN), in_port_(0) {
  this->length_ = sizeof(struct of10::ofp_packet_in) - 2;
}

PacketIn::PacketIn(uint32_t xid, uint32_t buffer_id, uint16_t in_port,
                   uint16_t total_len, uint8_t reason)
    : PacketInCommon(of10::OFP_VERSION, of10::OFPT_PACKET_IN, xid, buffer_id,
                     total_len, reason),
      in_port_(in_port) {
  this->length_ = sizeof(struct of10::ofp_packet_in) - 2;
}

bool PacketIn::operator==(const PacketIn &other) const {
  return ((PacketInCommon::operator==(other)) &&
          (this->in_port_ == other.in_port_));
}

bool PacketIn::operator!=(const PacketIn &other) const {
  return !(*this == other);
}

uint8_t *PacketIn::pack() {
  uint8_t *buffer = OFMsg::pack();
  struct of10::ofp_packet_in *pi = (struct of10::ofp_packet_in *)buffer;
  pi->buffer_id = hton32(this->buffer_id_);
  pi->total_len = hton16(this->total_len_);
  pi->in_port = hton16(this->in_port_);
  pi->reason = this->reason_;
  memset(&pi->pad, 0x0, 1);
  if (this->data_len_) {
    memcpy(pi->data, this->data_, this->data_len_);
  }
  return buffer;
}

of_error PacketIn::unpack(uint8_t *buffer) {
  struct of10::ofp_packet_in *pi = (struct of10::ofp_packet_in *)buffer;
  OFMsg::unpack(buffer);
  this->buffer_id_ = ntoh32(pi->buffer_id);
  this->total_len_ = ntoh16(pi->total_len);
  this->in_port_ = ntoh16(pi->in_port);
  this->reason_ = pi->reason;
  this->data_len_ = this->length_ - (sizeof(struct of10::ofp_packet_in) - 2);
  if (this->data_len_) {
    this->data_ = ::operator new(this->data_len_);
    memcpy(this->data_, pi->data, this->data_len_);
  }
  return 0;
}

FlowRemoved::FlowRemoved()
    : FlowRemovedCommon(of10::OFP_VERSION, of10::OFPT_FLOW_REMOVED) {
  this->length_ = sizeof(struct of10::ofp_flow_removed);
}

FlowRemoved::FlowRemoved(uint32_t xid, uint64_t cookie, uint16_t priority,
                         uint8_t reason, uint32_t duration_sec,
                         uint32_t duration_nsec, uint16_t idle_timeout,
                         uint64_t packet_count, uint64_t byte_count)
    : FlowRemovedCommon(of10::OFP_VERSION, of10::OFPT_FLOW_REMOVED, xid, cookie,
                        priority, reason, duration_sec, duration_nsec,
                        idle_timeout, packet_count, byte_count) {
  this->length_ = sizeof(struct of10::ofp_flow_removed);
}

FlowRemoved::FlowRemoved(uint32_t xid, uint64_t cookie, uint16_t priority,
                         uint8_t reason, uint32_t duration_sec,
                         uint32_t duration_nsec, uint16_t idle_timeout,
                         uint64_t packet_count, uint64_t byte_count,
                         of10::Match match)
    : FlowRemovedCommon(of10::OFP_VERSION, of10::OFPT_FLOW_REMOVED, xid, cookie,
                        priority, reason, duration_sec, duration_nsec,
                        idle_timeout, packet_count, byte_count),
      match_(match) {
  this->length_ = sizeof(struct of10::ofp_flow_removed);
}

bool FlowRemoved::operator==(const FlowRemoved &other) const {
  return ((FlowRemovedCommon::operator==(other)) &&
          (this->match_ == other.match_));
}

bool FlowRemoved::operator!=(const FlowRemoved &other) const {
  return !(*this == other);
}

uint8_t *FlowRemoved::pack() {
  uint8_t *buffer = OFMsg::pack();
  struct of10::ofp_flow_removed *fr = (struct of10::ofp_flow_removed *)buffer;
  this->match_.pack(buffer + sizeof(struct ofp_header));
  fr->cookie = hton64(this->cookie_);
  fr->priority = hton16(this->priority_);
  fr->reason = this->reason_;
  memset(&fr->pad, 0x0, 1);
  fr->duration_sec = hton32(this->duration_sec_);
  fr->duration_nsec = hton32(this->duration_nsec_);
  fr->idle_timeout = hton16(this->idle_timeout_);
  memset(fr->pad, 0x0, 2);
  fr->packet_count = hton64(this->packet_count_);
  fr->byte_count = hton64(this->byte_count_);
  return buffer;
}

of_error FlowRemoved::unpack(uint8_t *buffer) {
  struct of10::ofp_flow_removed *fr = (struct of10::ofp_flow_removed *)buffer;
  OFMsg::unpack(buffer);
  this->match_.unpack(buffer + sizeof(struct ofp_header));
  this->cookie_ = ntoh64(fr->cookie);
  this->priority_ = ntoh16(fr->priority);
  this->reason_ = fr->reason;
  this->duration_sec_ = ntoh32(fr->duration_sec);
  this->duration_nsec_ = ntoh32(fr->duration_nsec);
  this->idle_timeout_ = ntoh16(fr->idle_timeout);
  this->packet_count_ = ntoh64(fr->packet_count);
  this->byte_count_ = ntoh64(fr->byte_count);
  return 0;
}

PortStatus::PortStatus()
    : PortStatusCommon(of10::OFP_VERSION, of10::OFPT_PORT_STATUS) {
  this->length_ = sizeof(struct of10::ofp_port_status);
}

PortStatus::PortStatus(uint32_t xid, uint8_t reason, of10::Port desc)
    : PortStatusCommon(of10::OFP_VERSION, of10::OFPT_PORT_STATUS, xid, reason),
      desc_(desc) {
  this->length_ = sizeof(struct of10::ofp_port_status);
}

bool PortStatus::operator==(const PortStatus &other) const {
  return ((PortStatusCommon::operator==(other)) &&
          (this->desc_ == other.desc_));
}

bool PortStatus::operator!=(const PortStatus &other) const {
  return !(*this == other);
}

uint8_t *PortStatus::pack() {
  uint8_t *buffer = OFMsg::pack();
  struct of10::ofp_port_status *ps = (struct of10::ofp_port_status *)buffer;
  ps->reason = this->reason_;
  memset(ps->pad, 0x0, 7);
  this->desc_.pack(buffer + (sizeof(struct ofp_header) + 8));
  return buffer;
}

of_error PortStatus::unpack(uint8_t *buffer) {
  struct of10::ofp_port_status *ps = (struct of10::ofp_port_status *)buffer;
  OFMsg::unpack(buffer);
  this->reason_ = ps->reason;
  this->desc_.unpack(buffer + (sizeof(struct ofp_header) + 8));
  return 0;
}

PortMod::PortMod()
    : PortModCommon(of10::OFP_VERSION, of10::OFPT_PORT_MOD), port_no_(0) {
  this->length_ = sizeof(struct of10::ofp_port_mod);
};

PortMod::PortMod(uint32_t xid, uint16_t port_no, EthAddress hw_addr,
                 uint32_t config, uint32_t mask, uint32_t advertise)
    : PortModCommon(of10::OFP_VERSION, of10::OFPT_PORT_MOD, xid, hw_addr,
                    config, mask, advertise),
      port_no_(port_no) {
  this->length_ = sizeof(struct of10::ofp_port_mod);
}

bool PortMod::operator==(const PortMod &other) const {
  return ((PortModCommon::operator==(other)) &&
          (this->port_no_ == other.port_no_));
}

bool PortMod::operator!=(const PortMod &other) const {
  return !(*this == other);
}

uint8_t *PortMod::pack() {
  uint8_t *buffer = OFMsg::pack();
  struct of10::ofp_port_mod *pm = (struct of10::ofp_port_mod *)buffer;
  pm->port_no = hton16(this->port_no_);
  memcpy(pm->hw_addr, hw_addr_.get_data(), OFP_ETH_ALEN);
  pm->config = hton32(this->config_);
  pm->mask = hton32(this->mask_);
  pm->advertise = hton32(this->advertise_);
  memset(pm->pad, 0x0, 4);
  return buffer;
}

of_error PortMod::unpack(uint8_t *buffer) {
  struct of10::ofp_port_mod *pm = (struct of10::ofp_port_mod *)buffer;
  if (pm->header.length < sizeof(struct of10::ofp_port_mod)) {
    return openflow_error(of10::OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  }
  OFMsg::unpack(buffer);
  this->port_no_ = ntoh16(pm->port_no);
  this->hw_addr_ = EthAddress(pm->hw_addr);
  this->config_ = ntoh32(pm->config);
  this->mask_ = ntoh32(pm->mask);
  this->advertise_ = ntoh32(pm->advertise);
  return 0;
}

StatsRequest::StatsRequest()
    : OFMsg(of10::OFP_VERSION, of10::OFPT_STATS_REQUEST),
      stats_type_(0),
      flags_(0) {
  this->length_ = sizeof(struct of10::ofp_stats_request);
}

StatsRequest::StatsRequest(uint16_t type)
    : OFMsg(of10::OFP_VERSION, of10::OFPT_STATS_REQUEST),
      stats_type_(type),
      flags_(0) {
  this->length_ = sizeof(struct of10::ofp_stats_request);
}

StatsRequest::StatsRequest(uint32_t xid, uint16_t type, uint16_t flags)
    : OFMsg(of10::OFP_VERSION, of10::OFPT_STATS_REQUEST, xid),
      stats_type_(type),
      flags_(flags) {
  this->length_ = sizeof(struct of10::ofp_stats_request);
}

bool StatsRequest::operator==(const StatsRequest &other) const {
  return ((OFMsg::operator==(other)) &&
          (this->stats_type_ == other.stats_type_) &&
          (this->flags_ == other.flags_));
}

bool StatsRequest::operator!=(const StatsRequest &other) const {
  return !(*this == other);
}

uint8_t *StatsRequest::pack() {
  uint8_t *buffer = OFMsg::pack();
  struct of10::ofp_stats_request *sr = (struct of10::ofp_stats_request *)buffer;

  sr->type = hton16(this->stats_type_);
  sr->flags = hton16(this->flags_);
  return buffer;
}

of_error StatsRequest::unpack(uint8_t *buffer) {
  struct of10::ofp_stats_request *sr = (struct of10::ofp_stats_request *)buffer;
  OFMsg::unpack(buffer);
  if (this->length_ < sizeof(struct of10::ofp_stats_request)) {
    return openflow_error(of10::OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  }
  this->stats_type_ = ntoh16(sr->type);
  this->flags_ = ntoh16(sr->flags);
  return 0;
}

StatsReply::StatsReply()
    : OFMsg(of10::OFP_VERSION, of10::OFPT_STATS_REPLY),
      stats_type_(0),
      flags_(0) {
  this->length_ = sizeof(struct of10::ofp_stats_reply);
}

StatsReply::StatsReply(uint16_t type)
    : OFMsg(of10::OFP_VERSION, of10::OFPT_STATS_REPLY),
      stats_type_(type),
      flags_(0) {
  this->length_ = sizeof(struct of10::ofp_stats_reply);
}

StatsReply::StatsReply(uint32_t xid, uint16_t type, uint16_t flags)
    : OFMsg(of10::OFP_VERSION, of10::OFPT_STATS_REPLY, xid) {
  this->stats_type_ = type;
  this->flags_ = flags;
  this->length_ = sizeof(struct of10::ofp_stats_reply);
}

bool StatsReply::operator==(const StatsReply &other) const {
  return ((OFMsg::operator==(other)) &&
          (this->stats_type_ == other.stats_type_) &&
          (this->flags_ == other.flags_));
}

bool StatsReply::operator!=(const StatsReply &other) const {
  return !(*this == other);
}

uint8_t *StatsReply::pack() {
  uint8_t *buffer = OFMsg::pack();
  struct of10::ofp_stats_reply *sr = (struct of10::ofp_stats_reply *)buffer;
  sr->type = hton16(this->stats_type_);
  sr->flags = hton16(this->flags_);
  return buffer;
}

of_error StatsReply::unpack(uint8_t *buffer) {
  struct of10::ofp_stats_reply *sr = (struct of10::ofp_stats_reply *)buffer;
  OFMsg::unpack(buffer);
  this->stats_type_ = ntoh16(sr->type);
  this->flags_ = ntoh16(sr->flags);
  return 0;
}

StatsRequestDesc::StatsRequestDesc() : StatsRequest(OFPST_DESC) {}

StatsRequestDesc::StatsRequestDesc(uint32_t xid, uint16_t flags)
    : StatsRequest(xid, of10::OFPST_DESC, flags) {}

uint8_t *StatsRequestDesc::pack() {
  uint8_t *buffer = StatsRequest::pack();
  return buffer;
}

of_error StatsRequestDesc::unpack(uint8_t *buffer) {
  return StatsRequest::unpack(buffer);
}

StatsReplyDesc::StatsReplyDesc() : StatsReply(OFPST_DESC) {
  this->length_ += sizeof(struct ofp_desc);
}

StatsReplyDesc::StatsReplyDesc(uint32_t xid, uint16_t flags, SwitchDesc desc)
    : StatsReply(xid, of10::OFPST_DESC, flags), desc_(desc) {
  this->length_ += sizeof(struct ofp_desc);
}

StatsReplyDesc::StatsReplyDesc(uint32_t xid, uint16_t flags,
                               std::string mfr_desc, std::string hw_desc,
                               std::string sw_desc, std::string serial_num,
                               std::string dp_desc)
    : StatsReply(xid, of10::OFPST_DESC, flags),
      desc_(mfr_desc, hw_desc, sw_desc, serial_num, dp_desc) {
  this->length_ += sizeof(struct ofp_desc);
}

bool StatsReplyDesc::operator==(const StatsReplyDesc &other) const {
  return ((StatsReply::operator==(other)) && (this->desc_ == other.desc_));
}

bool StatsReplyDesc::operator!=(const StatsReplyDesc &other) const {
  return !(*this == other);
}

uint8_t *StatsReplyDesc::pack() {
  uint8_t *buffer = StatsReply::pack();
  this->desc_.pack(buffer + sizeof(struct of10::ofp_stats_request));
  return buffer;
}

of_error StatsReplyDesc::unpack(uint8_t *buffer) {
  StatsReply::unpack(buffer);
  return this->desc_.unpack(buffer + sizeof(struct of10::ofp_stats_reply));
}

void StatsReplyDesc::desc(SwitchDesc desc) {
  this->desc_ = desc;
  this->length_ += sizeof(struct ofp_desc);
}

StatsRequestFlow::StatsRequestFlow() : StatsRequest(OFPST_FLOW) {
  this->length_ += sizeof(struct of10::ofp_flow_stats_request);
}

StatsRequestFlow::StatsRequestFlow(uint32_t xid, uint16_t flags,
                                   uint8_t table_id, uint16_t out_port)
    : StatsRequest(xid, of10::OFPST_FLOW, flags),
      table_id_(table_id),
      out_port_(out_port) {
  this->length_ += sizeof(struct of10::ofp_flow_stats_request);
}

StatsRequestFlow::StatsRequestFlow(uint32_t xid, uint16_t flags,
                                   of10::Match match, uint8_t table_id,
                                   uint16_t out_port)
    : StatsRequest(xid, of10::OFPST_FLOW, flags),
      table_id_(table_id),
      out_port_(out_port),
      match_(match) {
  this->length_ += sizeof(struct of10::ofp_flow_stats_request);
}

bool StatsRequestFlow::operator==(const StatsRequestFlow &other) const {
  return ((StatsRequest::operator==(other)) &&
          (this->table_id_ == other.table_id_) &&
          (this->out_port_ == other.out_port_) &&
          (this->match_ == other.match_));
}

bool StatsRequestFlow::operator!=(const StatsRequestFlow &other) const {
  return !(*this == other);
}

uint8_t *StatsRequestFlow::pack() {
  uint8_t *buffer = StatsRequest::pack();
  struct of10::ofp_flow_stats_request *fs =
      (struct of10::ofp_flow_stats_request
           *)(buffer + sizeof(struct of10::ofp_stats_request));
  this->match_.pack(buffer + sizeof(struct of10::ofp_stats_request));
  fs->table_id = this->table_id_;
  memset(&fs->pad, 0x0, 1);
  fs->out_port = hton16(this->out_port_);
  return buffer;
}

of_error StatsRequestFlow::unpack(uint8_t *buffer) {
  struct of10::ofp_flow_stats_request *fs =
      (struct of10::ofp_flow_stats_request
           *)(buffer + sizeof(struct of10::ofp_stats_request));
  StatsRequest::unpack(buffer);
  if (this->length_ <
      sizeof(ofp_stats_request) + sizeof(of10::ofp_flow_stats_request)) {
    return openflow_error(OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  }
  this->table_id_ = fs->table_id;
  this->out_port_ = hton16(fs->out_port);
  return this->match_.unpack(buffer + sizeof(struct of10::ofp_stats_request));
}

StatsReplyFlow::StatsReplyFlow() : StatsReply(OFPST_FLOW) {}

StatsReplyFlow::StatsReplyFlow(uint32_t xid, uint16_t flags)
    : StatsReply(xid, of10::OFPST_FLOW, flags) {}
StatsReplyFlow::StatsReplyFlow(uint32_t xid, uint16_t flags,
                               std::vector<of10::FlowStats> flow_stats)
    : StatsReply(xid, of10::OFPST_FLOW, flags), flow_stats_(flow_stats) {
  this->length_ += flow_stats.size() * sizeof(struct ofp_flow_stats);
}

bool StatsReplyFlow::operator==(const StatsReplyFlow &other) const {
  return ((StatsReply::operator==(other)) &&
          (this->flow_stats_ == other.flow_stats_));
}

bool StatsReplyFlow::operator!=(const StatsReplyFlow &other) const {
  return !(*this == other);
}

uint8_t *StatsReplyFlow::pack() {
  uint8_t *buffer = StatsReply::pack();
  uint8_t *p = buffer + sizeof(struct ofp_stats_request);
  for (std::vector<FlowStats>::iterator it = this->flow_stats_.begin();
       it != this->flow_stats_.end(); ++it) {
    it->pack(p);
    p += it->length();
  }
  return buffer;
}

of_error StatsReplyFlow::unpack(uint8_t *buffer) {
  StatsReply::unpack(buffer);
  size_t len = this->length_ - sizeof(struct ofp_stats_reply);
  uint8_t *p = buffer + sizeof(struct ofp_stats_reply);
  while (len) {
    FlowStats stat;
    stat.unpack(p);
    this->flow_stats_.push_back(stat);
    p += stat.length();
    len -= stat.length();
  }
  return 0;
}

void StatsReplyFlow::flow_stats(std::vector<of10::FlowStats> flow_stats) {
  this->flow_stats_ = flow_stats;
  this->length_ += this->flow_stats_.size() * sizeof(struct ofp_flow_stats);
}

void StatsReplyFlow::add_flow_stats(of10::FlowStats stats) {
  this->flow_stats_.push_back(stats);
  this->length_ += stats.length();
}

StatsRequestAggregate::StatsRequestAggregate() : StatsRequest(OFPST_AGGREGATE) {
  this->length_ += sizeof(struct of10::ofp_aggregate_stats_request);
}

StatsRequestAggregate::StatsRequestAggregate(uint32_t xid, uint16_t flags,
                                             uint8_t table_id,
                                             uint16_t out_port)
    : StatsRequest(xid, of10::OFPST_AGGREGATE, flags) {
  this->length_ += sizeof(struct of10::ofp_aggregate_stats_request);
}

StatsRequestAggregate::StatsRequestAggregate(uint32_t xid, uint16_t flags,
                                             of10::Match match,
                                             uint8_t table_id,
                                             uint16_t out_port)
    : StatsRequest(xid, of10::OFPST_AGGREGATE, flags),
      table_id_(table_id),
      out_port_(out_port),
      match_(match) {
  this->length_ += sizeof(struct of10::ofp_aggregate_stats_request);
}

bool StatsRequestAggregate::operator==(
    const StatsRequestAggregate &other) const {
  return ((StatsRequest::operator==(other)) &&
          (this->table_id_ == other.table_id_) &&
          (this->out_port_ == other.out_port_) &&
          (this->match_ == other.match_));
}

bool StatsRequestAggregate::operator!=(
    const StatsRequestAggregate &other) const {
  return !(*this == other);
}

uint8_t *StatsRequestAggregate::pack() {
  uint8_t *buffer = StatsRequest::pack();
  struct of10::ofp_aggregate_stats_request *fs =
      (struct of10::ofp_aggregate_stats_request
           *)(buffer + sizeof(struct of10::ofp_stats_request));
  this->match_.pack(buffer + sizeof(struct of10::ofp_stats_request));
  fs->table_id = this->table_id_;
  memset(&fs->pad, 0x0, 1);
  fs->out_port = hton16(this->out_port_);
  return buffer;
}

of_error StatsRequestAggregate::unpack(uint8_t *buffer) {
  struct of10::ofp_aggregate_stats_request *fs =
      (struct of10::ofp_aggregate_stats_request
           *)(buffer + sizeof(struct of10::ofp_stats_request));
  StatsRequest::unpack(buffer);
  if (this->length_ <
      sizeof(ofp_stats_request) + sizeof(of10::ofp_aggregate_stats_request)) {
    return openflow_error(OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  }
  this->table_id_ = fs->table_id;
  this->out_port_ = hton16(fs->out_port);
  return this->match_.unpack(buffer + sizeof(struct of10::ofp_stats_request));
}

StatsReplyAggregate::StatsReplyAggregate() : StatsReply(OFPST_AGGREGATE) {
  this->length_ += sizeof(struct of10::ofp_aggregate_stats_reply);
}

StatsReplyAggregate::StatsReplyAggregate(uint32_t xid, uint16_t flags,
                                         uint64_t packet_count,
                                         uint64_t byte_count,
                                         uint32_t flow_count)
    : StatsReply(xid, of10::OFPST_AGGREGATE, flags),
      packet_count_(packet_count),
      byte_count_(byte_count),
      flow_count_(flow_count) {
  this->length_ += sizeof(struct of10::ofp_aggregate_stats_reply);
}

bool StatsReplyAggregate::operator==(const StatsReplyAggregate &other) const {
  return ((StatsReply::operator==(other)) &&
          (this->packet_count_ == other.packet_count_) &&
          (this->byte_count_ == other.byte_count_) &&
          (this->flow_count_ == other.flow_count_));
}

bool StatsReplyAggregate::operator!=(const StatsReplyAggregate &other) const {
  return !(*this == other);
}

uint8_t *StatsReplyAggregate::pack() {
  uint8_t *buffer = StatsReply::pack();
  struct of10::ofp_aggregate_stats_reply *ar =
      (struct of10::ofp_aggregate_stats_reply
           *)(buffer + sizeof(struct of10::ofp_stats_reply));
  ar->packet_count = hton64(this->packet_count_);
  ar->byte_count = hton64(this->byte_count_);
  ar->flow_count = hton32(this->flow_count_);
  memset(ar->pad, 0x0, 4);
  return buffer;
}

of_error StatsReplyAggregate::unpack(uint8_t *buffer) {
  struct of10::ofp_aggregate_stats_reply *ar =
      (struct of10::ofp_aggregate_stats_reply
           *)(buffer + sizeof(struct of10::ofp_stats_reply));
  StatsReply::unpack(buffer);
  this->packet_count_ = ntoh64(ar->packet_count);
  this->byte_count_ = ntoh64(ar->byte_count);
  this->flow_count_ = ntoh32(ar->flow_count);
  return 0;
}

StatsRequestTable::StatsRequestTable() : StatsRequest(OFPST_TABLE) {}

StatsRequestTable::StatsRequestTable(uint32_t xid, uint16_t flags)
    : StatsRequest(xid, of10::OFPST_TABLE, flags) {}

uint8_t *StatsRequestTable::pack() {
  uint8_t *buffer = StatsRequest::pack();
  return buffer;
}

of_error StatsRequestTable::unpack(uint8_t *buffer) {
  return StatsRequest::unpack(buffer);
}

StatsReplyTable::StatsReplyTable() : StatsReply(OFPST_TABLE) {}

StatsReplyTable::StatsReplyTable(uint32_t xid, uint16_t flags)
    : StatsReply(xid, of10::OFPST_TABLE, flags) {}

StatsReplyTable::StatsReplyTable(uint32_t xid, uint16_t flags,
                                 std::vector<of10::TableStats> table_stats)
    : StatsReply(xid, of10::OFPST_TABLE, flags), table_stats_(table_stats) {
  this->length_ += table_stats.size() * sizeof(struct of10::ofp_table_stats);
}

bool StatsReplyTable::operator==(const StatsReplyTable &other) const {
  return ((StatsReply::operator==(other)) &&
          (this->table_stats_ == other.table_stats_));
}

bool StatsReplyTable::operator!=(const StatsReplyTable &other) const {
  return !(*this == other);
}

uint8_t *StatsReplyTable::pack() {
  uint8_t *buffer = StatsReply::pack();
  uint8_t *p = buffer + sizeof(struct ofp_stats_reply);
  for (std::vector<TableStats>::iterator it = this->table_stats_.begin();
       it != this->table_stats_.end(); ++it) {
    it->pack(p);
    p += sizeof(struct of10::ofp_table_stats);
  }
  return buffer;
}

of_error StatsReplyTable::unpack(uint8_t *buffer) {
  StatsReply::unpack(buffer);
  uint8_t len = this->length_ - sizeof(struct ofp_stats_reply);
  uint8_t *p = buffer + sizeof(struct ofp_stats_reply);
  while (len) {
    TableStats stat;
    stat.unpack(p);
    this->table_stats_.push_back(stat);
    p += sizeof(struct of10::ofp_table_stats);
    len -= sizeof(struct of10::ofp_table_stats);
  }
  return 0;
}

void StatsReplyTable::table_stats(std::vector<of10::TableStats> table_stats) {
  this->table_stats_ = table_stats;
  this->length_ += table_stats.size() * sizeof(struct of10::ofp_table_stats);
}

void StatsReplyTable::add_table_stat(of10::TableStats stat) {
  this->table_stats_.push_back(stat);
  this->length_ += sizeof(struct of10::ofp_table_stats);
}

StatsRequestPort::StatsRequestPort() : StatsRequest(OFPST_PORT) {
  this->length_ += sizeof(struct of10::ofp_port_stats_request);
}

StatsRequestPort::StatsRequestPort(uint32_t xid, uint16_t flags,
                                   uint16_t port_no)
    : StatsRequest(xid, of10::OFPST_PORT, flags), port_no_(port_no) {
  this->length_ += sizeof(struct of10::ofp_port_stats_request);
};

bool StatsRequestPort::operator==(const StatsRequestPort &other) const {
  return ((StatsRequest::operator==(other)) &&
          (this->port_no_ == other.port_no_));
}

bool StatsRequestPort::operator!=(const StatsRequestPort &other) const {
  return !(*this == other);
}

uint8_t *StatsRequestPort::pack() {
  uint8_t *buffer = StatsRequest::pack();
  struct of10::ofp_port_stats_request *ps =
      (struct of10::ofp_port_stats_request
           *)(buffer + sizeof(struct of10::ofp_stats_request));
  ps->port_no = hton16(this->port_no_);
  memset(ps->pad, 0x0, 6);
  return buffer;
}

of_error StatsRequestPort::unpack(uint8_t *buffer) {
  struct of10::ofp_port_stats_request *ps =
      (struct of10::ofp_port_stats_request
           *)(buffer + sizeof(struct of10::ofp_stats_request));
  StatsRequest::unpack(buffer);
  if (this->length_ <
      sizeof(ofp_stats_request) + sizeof(of10::ofp_port_stats_request)) {
    return openflow_error(OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  }
  this->port_no_ = ntoh16(ps->port_no);
  return 0;
}

StatsReplyPort::StatsReplyPort() : StatsReply(OFPST_PORT) {}

StatsReplyPort::StatsReplyPort(uint32_t xid, uint16_t flags)
    : StatsReply(xid, of10::OFPST_PORT, flags) {}

StatsReplyPort::StatsReplyPort(uint32_t xid, uint16_t flags,
                               std::vector<of10::PortStats> port_stats)
    : StatsReply(xid, of10::OFPST_PORT, flags), port_stats_(port_stats) {
  this->length_ += port_stats.size() * sizeof(struct of10::ofp_port_stats);
}

bool StatsReplyPort::operator==(const StatsReplyPort &other) const {
  return ((StatsReply::operator==(other)) &&
          (this->port_stats_ == other.port_stats_));
}

bool StatsReplyPort::operator!=(const StatsReplyPort &other) const {
  return !(*this == other);
}

uint8_t *StatsReplyPort::pack() {
  uint8_t *buffer = StatsReply::pack();
  uint8_t *p = buffer + sizeof(struct ofp_stats_reply);
  for (std::vector<of10::PortStats>::iterator it = this->port_stats_.begin();
       it != this->port_stats_.end(); ++it) {
    it->pack(p);
    p += sizeof(struct of10::ofp_port_stats);
  }
  return buffer;
}

of_error StatsReplyPort::unpack(uint8_t *buffer) {
  StatsReply::unpack(buffer);
  uint8_t len = this->length_ - sizeof(struct ofp_stats_reply);
  uint8_t *p = buffer + sizeof(struct ofp_stats_request);
  while (len) {
    of10::PortStats stat;
    stat.unpack(p);
    this->port_stats_.push_back(stat);
    p += sizeof(struct of10::ofp_port_stats);
    len -= sizeof(struct of10::ofp_port_stats);
  }
  return 0;
}

void StatsReplyPort::port_stats(std::vector<of10::PortStats> port_stats) {
  this->port_stats_ = port_stats;
  this->length_ += port_stats.size() * sizeof(struct of10::ofp_port_stats);
}

void StatsReplyPort::add_port_stat(of10::PortStats stat) {
  this->port_stats_.push_back(stat);
  this->length_ += sizeof(struct of10::ofp_port_stats);
}

StatsRequestQueue::StatsRequestQueue() : StatsRequest(OFPST_QUEUE) {
  this->length_ += sizeof(struct of10::ofp_queue_stats_request);
}

StatsRequestQueue::StatsRequestQueue(uint32_t xid, uint16_t flags,
                                     uint16_t port_no, uint32_t queue_id)
    : StatsRequest(xid, of10::OFPST_QUEUE, flags),
      port_no_(port_no),
      queue_id_(queue_id) {
  this->length_ += sizeof(struct of10::ofp_queue_stats_request);
}

bool StatsRequestQueue::operator==(const StatsRequestQueue &other) const {
  return ((StatsRequest::operator==(other)) &&
          (this->port_no_ == other.port_no_) &&
          (this->queue_id_ == other.queue_id_));
}

bool StatsRequestQueue::operator!=(const StatsRequestQueue &other) const {
  return !(*this == other);
}

uint8_t *StatsRequestQueue::pack() {
  uint8_t *buffer = StatsRequest::pack();
  struct of10::ofp_queue_stats_request *qs =
      (of10::ofp_queue_stats_request *)(buffer +
                                        sizeof(of10::ofp_stats_request));
  qs->port_no = hton16(this->port_no_);
  memset(qs->pad, 0x0, 2);
  qs->queue_id = hton32(this->queue_id_);
  return buffer;
}

of_error StatsRequestQueue::unpack(uint8_t *buffer) {
  struct of10::ofp_queue_stats_request *qs =
      (of10::ofp_queue_stats_request *)(buffer +
                                        sizeof(of10::ofp_stats_request));
  StatsRequest::unpack(buffer);
  if (this->length_ <
      sizeof(ofp_stats_request) + sizeof(of10::ofp_queue_stats_request)) {
    return openflow_error(of10::OFPET_BAD_REQUEST, of10::OFPBRC_BAD_LEN);
  }
  this->port_no_ = hton16(qs->port_no);
  this->queue_id_ = hton32(qs->queue_id);
  return 0;
}

StatsReplyQueue::StatsReplyQueue() : StatsReply(OFPST_QUEUE) {}

StatsReplyQueue::StatsReplyQueue(uint32_t xid, uint16_t flags)
    : StatsReply(xid, of10::OFPST_QUEUE, flags) {}

StatsReplyQueue::StatsReplyQueue(uint32_t xid, uint16_t flags,
                                 std::vector<of10::QueueStats> queue_stats)
    : StatsReply(xid, of10::OFPST_QUEUE, flags), queue_stats_(queue_stats) {
  this->length_ += sizeof(struct of10::ofp_queue_stats);
}

bool StatsReplyQueue::operator==(const StatsReplyQueue &other) const {
  return ((StatsReply::operator==(other)) &&
          (this->queue_stats_ == other.queue_stats_));
}

bool StatsReplyQueue::operator!=(const StatsReplyQueue &other) const {
  return !(*this == other);
}

uint8_t *StatsReplyQueue::pack() {
  uint8_t *buffer = StatsReply::pack();
  uint8_t *p = buffer + sizeof(struct ofp_stats_reply);
  for (std::vector<of10::QueueStats>::iterator it = this->queue_stats_.begin();
       it != this->queue_stats_.end(); ++it) {
    it->pack(p);
    p += sizeof(struct of10::ofp_queue_stats);
  }
  return buffer;
}

of_error StatsReplyQueue::unpack(uint8_t *buffer) {
  StatsReply::unpack(buffer);
  uint8_t len = this->length_ - sizeof(struct ofp_stats_reply);
  uint8_t *p = buffer + sizeof(struct ofp_stats_reply);
  while (len) {
    of10::QueueStats stat;
    stat.unpack(p);
    this->queue_stats_.push_back(stat);
    p += sizeof(struct of10::ofp_queue_stats);
    len -= sizeof(struct of10::ofp_queue_stats);
  }
  return 0;
}

void StatsReplyQueue::queue_stats(std::vector<of10::QueueStats> queue_stats) {
  this->queue_stats_ = queue_stats;
  this->length_ += queue_stats.size() * sizeof(struct of10::ofp_queue_stats);
}

void StatsReplyQueue::add_queue_stat(of10::QueueStats stat) {
  this->queue_stats_.push_back(stat);
  this->length_ += sizeof(struct of10::ofp_queue_stats);
}

StatsRequestVendor::StatsRequestVendor() : StatsRequest(OFPST_VENDOR) {
  this->length_ += 4;
}

StatsRequestVendor::StatsRequestVendor(uint32_t xid, uint16_t flags,
                                       uint32_t vendor)
    : StatsRequest(xid, of10::OFPST_VENDOR, flags), vendor_(vendor) {
  this->length_ += 4;
}

bool StatsRequestVendor::operator==(const StatsRequestVendor &other) const {
  return ((StatsRequest::operator==(other)) &&
          (this->vendor_ == other.vendor_));
}

bool StatsRequestVendor::operator!=(const StatsRequestVendor &other) const {
  return !(*this == other);
}

uint8_t *StatsRequestVendor::pack() {
  uint8_t *buffer = StatsRequest::pack();
  uint32_t vendor = hton32(this->vendor_);
  memcpy(buffer + sizeof(struct of10::ofp_stats_request), &vendor,
         sizeof(uint32_t));
  return buffer;
}

of_error StatsRequestVendor::unpack(uint8_t *buffer) {
  StatsRequest::unpack(buffer);
  uint32_t vendor;
  memcpy(&vendor, buffer + sizeof(struct of10::ofp_stats_request),
         sizeof(uint32_t));
  this->vendor_ = ntoh32(this->vendor_);
  return 0;
}

StatsReplyVendor::StatsReplyVendor() : StatsReply(OFPST_VENDOR) {
  this->length_ += 4;
}

StatsReplyVendor::StatsReplyVendor(uint32_t xid, uint16_t flags,
                                   uint32_t vendor)
    : StatsReply(xid, of10::OFPST_VENDOR, flags), vendor_(vendor) {
  this->length_ += 4;
}

bool StatsReplyVendor::operator==(const StatsReplyVendor &other) const {
  return ((StatsReply::operator==(other)) && (this->vendor_ == other.vendor_));
}

bool StatsReplyVendor::operator!=(const StatsReplyVendor &other) const {
  return !(*this == other);
}

uint8_t *StatsReplyVendor::pack() {
  uint8_t *buffer = StatsReply::pack();
  uint32_t vendor = hton32(this->vendor_);
  memcpy(buffer + sizeof(struct of10::ofp_stats_reply), &vendor,
         sizeof(uint32_t));
  return buffer;
}

of_error StatsReplyVendor::unpack(uint8_t *buffer) {
  StatsReply::unpack(buffer);
  uint32_t vendor;
  memcpy(&vendor, buffer + sizeof(struct of10::ofp_stats_reply),
         sizeof(uint32_t));
  this->vendor_ = ntoh32(this->vendor_);
  return 0;
}

QueueGetConfigRequest::QueueGetConfigRequest()
    : OFMsg(of10::OFP_VERSION, of10::OFPT_QUEUE_GET_CONFIG_REQUEST) {
  this->length_ = sizeof(struct of10::ofp_queue_get_config_request);
}

QueueGetConfigRequest::QueueGetConfigRequest(uint32_t xid, uint16_t port)
    : OFMsg(of10::OFP_VERSION, of10::OFPT_QUEUE_GET_CONFIG_REQUEST, xid),
      port_(port) {
  this->length_ = sizeof(struct of10::ofp_queue_get_config_request);
}

bool QueueGetConfigRequest::operator==(
    const QueueGetConfigRequest &other) const {
  return ((OFMsg::operator==(other)) && (this->port_ == other.port_));
}

bool QueueGetConfigRequest::operator!=(
    const QueueGetConfigRequest &other) const {
  return !(*this == other);
}

uint8_t *QueueGetConfigRequest::pack() {
  uint8_t *buffer = OFMsg::pack();
  struct of10::ofp_queue_get_config_request *qc =
      (struct of10::ofp_queue_get_config_request *)buffer;
  qc->port = hton16(this->port_);
  memset(qc->pad, 0x0, 2);
  return buffer;
}

of_error QueueGetConfigRequest::unpack(uint8_t *buffer) {
  struct of10::ofp_queue_get_config_request *qc =
      (struct of10::ofp_queue_get_config_request *)buffer;
  OFMsg::unpack(buffer);
  if (this->length_ < sizeof(ofp_queue_get_config_request)) {
    return openflow_error(OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  }
  this->port_ = ntoh16(qc->port);
  return 0;
}

QueueGetConfigReply::QueueGetConfigReply()
    : OFMsg(of10::OFP_VERSION, of10::OFPT_QUEUE_GET_CONFIG_REPLY) {
  this->length_ = sizeof(struct of10::ofp_queue_get_config_reply);
}

QueueGetConfigReply::QueueGetConfigReply(uint32_t xid, uint16_t port)
    : OFMsg(of10::OFP_VERSION, of10::OFPT_QUEUE_GET_CONFIG_REPLY, xid),
      port_(port) {
  this->length_ = sizeof(struct of10::ofp_queue_get_config_reply);
}

QueueGetConfigReply::QueueGetConfigReply(uint32_t xid, uint16_t port,
                                         std::list<of10::PacketQueue> queues)
    : OFMsg(of10::OFP_VERSION, of10::OFPT_QUEUE_GET_CONFIG_REPLY, xid),
      port_(port),
      queues_(queues) {
  this->length_ =
      sizeof(struct of10::ofp_queue_get_config_reply) + queues_len();
}

bool QueueGetConfigReply::operator==(const QueueGetConfigReply &other) const {
  return ((OFMsg::operator==(other)) && (this->port_ == other.port_) &&
          (this->queues_ == other.queues_));
}

bool QueueGetConfigReply::operator!=(const QueueGetConfigReply &other) const {
  return !(*this == other);
}

uint8_t *QueueGetConfigReply::pack() {
  uint8_t *buffer = OFMsg::pack();
  struct of10::ofp_queue_get_config_reply *qr =
      (struct of10::ofp_queue_get_config_reply *)buffer;
  qr->port = hton16(this->port_);
  memset(qr->pad, 0x0, 6);
  uint8_t *p = buffer + sizeof(struct of10::ofp_queue_get_config_reply);
  for (std::list<PacketQueue>::iterator it = this->queues_.begin(),
                                        end = this->queues_.end();
       it != end; ++it) {
    it->pack(p);
    p += it->len();
  }
  return buffer;
}

of_error QueueGetConfigReply::unpack(uint8_t *buffer) {
  struct of10::ofp_queue_get_config_reply *qr =
      (struct of10::ofp_queue_get_config_reply *)buffer;
  OFMsg::unpack(buffer);
  this->port_ = ntoh16(qr->port);
  uint8_t *p = buffer + sizeof(struct of10::ofp_queue_get_config_reply);
  size_t len = this->length_ - sizeof(struct of10::ofp_queue_get_config_reply);
  while (len) {
    PacketQueue pq;
    pq.unpack(p);
    this->queues_.push_back(pq);
    p += pq.len();
    len -= pq.len();
  }
  return 0;
}

void QueueGetConfigReply::queues(std::list<of10::PacketQueue> queues) {
  this->queues_ = queues;
  this->length_ += queues_len();
}

void QueueGetConfigReply::add_queue(PacketQueue queue) {
  this->queues_.push_back(queue);
  this->length_ += queue.len();
}

size_t QueueGetConfigReply::queues_len() {
  size_t len;
  for (std::list<PacketQueue>::iterator it = this->queues_.begin(),
                                        end = this->queues_.end();
       it != end; ++it) {
    len += it->len();
  }
  return len;
}

BarrierRequest::BarrierRequest()
    : OFMsg(of10::OFP_VERSION, of10::OFPT_BARRIER_REQUEST) {}

BarrierRequest::BarrierRequest(uint32_t xid)
    : OFMsg(of10::OFP_VERSION, of10::OFPT_BARRIER_REQUEST, xid) {}

uint8_t *BarrierRequest::pack() { return OFMsg::pack(); }

of_error BarrierRequest::unpack(uint8_t *buffer) {
  OFMsg::unpack(buffer);
  if (this->length_ < sizeof(struct ofp_header)) {
    return openflow_error(of10::OFPET_BAD_REQUEST, of10::OFPBRC_BAD_LEN);
  }
  return 0;
}

BarrierReply::BarrierReply()
    : OFMsg(of10::OFP_VERSION, of10::OFPT_BARRIER_REPLY) {}

BarrierReply::BarrierReply(uint32_t xid)
    : OFMsg(of10::OFP_VERSION, of10::OFPT_BARRIER_REPLY, xid) {}

uint8_t *BarrierReply::pack() { return OFMsg::pack(); }

of_error BarrierReply::unpack(uint8_t *buffer) {
  OFMsg::unpack(buffer);
  return 0;
}

}  // End namespace of10.
}  // End of namespace fluid_msg.

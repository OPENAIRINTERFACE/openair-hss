#include "msg.hh"
#include "fluid/util/util.h"

namespace fluid_msg {

/*OpenFlow message header class constructor*/
OFMsg::OFMsg(uint8_t version, uint8_t type)
    : version_(version),
      type_(type),
      length_(sizeof(struct ofp_header)),
      xid_(0) {}

/*OpenFlow message header class constructor*/
OFMsg::OFMsg(uint8_t version, uint8_t type, uint32_t xid)
    : version_(version),
      type_(type),
      length_(sizeof(struct ofp_header)),
      xid_(xid) {}

uint8_t *OFMsg::pack() {
  uint8_t *buffer = new uint8_t[this->length_];
  memset(buffer, 0x0, this->length_);
  struct ofp_header *oh = (struct ofp_header *)buffer;
  oh->version = this->version_;
  oh->type = this->type_;
  oh->length = hton16(this->length_);
  oh->xid = hton32(this->xid_);
  return buffer;
}

of_error OFMsg::unpack(uint8_t *buffer) {
  struct ofp_header *oh = (struct ofp_header *)buffer;
  this->version_ = oh->version;
  this->type_ = oh->type;
  this->length_ = ntoh16(oh->length);
  this->xid_ = ntoh32(oh->xid);
  return 0;
}

bool OFMsg::operator==(const OFMsg &other) const {
  return ((this->version_ == other.version_) && (this->type_ == other.type_) &&
          (this->length_ == other.length_) && (this->xid_ == other.xid_));
}

bool OFMsg::operator!=(const OFMsg &other) const { return !(*this == other); }

EchoCommon::EchoCommon(uint8_t version, uint8_t type)
    : OFMsg(version, type), data_(NULL), data_len_(0) {}
uint8_t *EchoCommon::pack() {
  uint8_t *buffer = OFMsg::pack();
  memcpy(buffer + sizeof(struct ofp_header), this->data_, this->data_len_);
  return buffer;
}

of_error EchoCommon::unpack(uint8_t *buffer) {
  OFMsg::unpack(buffer);
  this->data_len_ = this->length_ - sizeof(struct ofp_header);
  if (this->data_len_) {
    this->data_ = ::operator new(this->data_len_);
    memcpy(this->data_, buffer + sizeof(struct ofp_header), this->data_len_);
  } else
    this->data_ = NULL;
  return 0;
}

bool EchoCommon::operator==(const EchoCommon &other) const {
  return ((OFMsg::operator==(other)) && (this->data_len_ == other.data_len_) &&
          (!memcmp(this->data_, other.data_, this->data_len_)));
}

bool EchoCommon::operator!=(const EchoCommon &other) const {
  return !(*this == other);
}

void EchoCommon::data(void *data, size_t data_len) {
  this->data_ = ::operator new(data_len);
  memcpy(this->data_, data, data_len);
  this->length_ += data_len;
  this->data_len_ = data_len;
}

EchoCommon::~EchoCommon() {
  if (this->data_len_) {
    ::operator delete(this->data_);
  }
}

ErrorCommon::ErrorCommon(uint8_t version, uint8_t type)
    : OFMsg(version, type), data_(NULL), data_len_(0), err_type_(0), code_(0){};

ErrorCommon::ErrorCommon(uint8_t version, uint8_t type, uint32_t xid,
                         uint16_t err_type, uint16_t code)
    : OFMsg(version, type, xid), data_(NULL), data_len_(0) {
  this->length_ = sizeof(struct ofp_error_msg);
  this->err_type_ = err_type;
  this->code_ = code;
}

ErrorCommon::ErrorCommon(uint8_t version, uint8_t type, uint32_t xid,
                         uint16_t err_type, uint16_t code, void *data,
                         size_t data_len)
    : OFMsg(version, type, xid) {
  this->length_ =
      sizeof(struct ofp_error_msg) + (data_len <= 64 ? data_len : 64);
  this->err_type_ = err_type;
  this->code_ = code;
  this->data_len_ = data_len;
  if (this->data_len_) {
    this->data_ = ::operator new(this->data_len_);
    memcpy(this->data_, data, this->data_len_);
  } else
    this->data_ = NULL;
}

ErrorCommon::~ErrorCommon() {
  if (this->data_len_) {
    ::operator delete(this->data_);
  }
}

bool ErrorCommon::operator==(const ErrorCommon &other) const {
  return ((OFMsg::operator==(other)) && (this->err_type_ == other.err_type_) &&
          (this->code_ == other.code_) &&
          (this->data_len_ == other.data_len_) &&
          (!memcmp(this->data_, other.data_, this->data_len_)));
}

bool ErrorCommon::operator!=(const ErrorCommon &other) const {
  return !(*this == other);
}

void ErrorCommon::data(void *data, size_t data_len) {
  this->data_ = ::operator new(data_len);
  memcpy(this->data_, data, data_len);
  this->data_len_ = data_len <= 64 ? data_len : 64;
  this->length_ += this->data_len_;
}

uint8_t *ErrorCommon::pack() {
  uint8_t *buffer = OFMsg::pack();
  struct ofp_error_msg *err = (struct ofp_error_msg *)buffer;
  err->type = hton16(this->err_type_);
  err->code = hton16(this->code_);
  memcpy(err->data, this->data_, this->data_len_);
  return buffer;
}

of_error ErrorCommon::unpack(uint8_t *buffer) {
  struct ofp_error_msg *err = (struct ofp_error_msg *)buffer;
  OFMsg::unpack(buffer);
  this->data_len_ = this->length_ - sizeof(struct ofp_error_msg);
  this->err_type_ = ntoh16(err->type);
  this->code_ = ntoh16(err->code);
  if (this->data_len_) {
    this->data_ = ::operator new(this->data_len_);
    memcpy(this->data_, buffer + sizeof(struct ofp_error_msg), this->data_len_);
  } else
    this->data_ = NULL;
  return 0;
}

FeaturesReplyCommon::FeaturesReplyCommon(uint8_t version, uint8_t type)
    : OFMsg(version, type),
      datapath_id_(0),
      n_buffers_(0),
      n_tables_(0),
      capabilities_(0) {}

FeaturesReplyCommon::FeaturesReplyCommon(uint8_t version, uint8_t type,
                                         uint32_t xid, uint64_t datapath_id,
                                         uint32_t n_buffers, uint8_t n_tables,
                                         uint32_t capabilities)
    : OFMsg(version, type, xid),
      datapath_id_(datapath_id),
      n_buffers_(n_buffers),
      n_tables_(n_tables),
      capabilities_(capabilities) {}

bool FeaturesReplyCommon::operator==(const FeaturesReplyCommon &other) const {
  return ((OFMsg::operator==(other)) &&
          (this->datapath_id_ == other.datapath_id_) &&
          (this->n_buffers_ == other.n_buffers_) &&
          (this->n_tables_ == other.n_tables_) &&
          (this->capabilities_ == other.capabilities_));
}

bool FeaturesReplyCommon::operator!=(const FeaturesReplyCommon &other) const {
  return !(*this == other);
}

SwitchConfigCommon::SwitchConfigCommon(uint8_t version, uint8_t type)
    : OFMsg(version, type), flags_(0x0), miss_send_len_(0) {}

SwitchConfigCommon::SwitchConfigCommon(uint8_t version, uint8_t type,
                                       uint32_t xid, uint16_t flags,
                                       uint16_t miss_send_len)
    : OFMsg(version, type, xid), flags_(flags), miss_send_len_(miss_send_len) {
  this->length_ = sizeof(struct ofp_switch_config);
}

bool SwitchConfigCommon::operator==(const SwitchConfigCommon &other) const {
  return ((OFMsg::operator==(other)) && (this->flags_ == other.flags_) &&
          (this->miss_send_len_ == other.miss_send_len_));
}

bool SwitchConfigCommon::operator!=(const SwitchConfigCommon &other) const {
  return !(*this == other);
}

uint8_t *SwitchConfigCommon::pack() {
  uint8_t *buffer = OFMsg::pack();
  struct ofp_switch_config *conf = (struct ofp_switch_config *)buffer;
  conf->flags = hton16(this->flags_);
  conf->miss_send_len = hton16(this->miss_send_len_);
  return buffer;
}

of_error SwitchConfigCommon::unpack(uint8_t *buffer) {
  struct ofp_switch_config *conf = (struct ofp_switch_config *)buffer;
  OFMsg::unpack(buffer);
  // if(this->length_ < sizeof(struct ofp_switch_config)){
  // 	return openflow_error(of10::OFPET_BAD_REQUEST, of10::OFPBRC_BAD_LEN);
  // }
  this->flags_ = ntoh16(conf->flags);
  this->miss_send_len_ = ntoh16(conf->miss_send_len);
  return 0;
}

FlowModCommon::FlowModCommon(uint8_t version, uint8_t type)
    : OFMsg(version, type),
      cookie_(0),
      idle_timeout_(0),
      hard_timeout_(0),
      priority_(0),
      buffer_id_(0),
      flags_(0) {}

FlowModCommon::FlowModCommon(uint8_t version, uint8_t type, uint32_t xid,
                             uint64_t cookie, uint16_t idle_timeout,
                             uint16_t hard_timeout, uint16_t priority,
                             uint32_t buffer_id, uint16_t flags)
    : OFMsg(version, type, xid),
      cookie_(cookie),
      idle_timeout_(idle_timeout),
      hard_timeout_(hard_timeout),
      priority_(priority),
      buffer_id_(buffer_id),
      flags_(flags) {}

bool FlowModCommon::operator==(const FlowModCommon &other) const {
  return ((OFMsg::operator==(other)) && (this->cookie_ == other.cookie_) &&
          (this->idle_timeout_ == other.idle_timeout_) &&
          (this->hard_timeout_ == other.hard_timeout_) &&
          (this->priority_ == other.priority_) &&
          (this->buffer_id_ == other.buffer_id_) &&
          (this->flags_ == other.flags_));
}

bool FlowModCommon::operator!=(const FlowModCommon &other) const {
  return !(*this == other);
}

PacketOutCommon::PacketOutCommon(uint8_t version, uint8_t type)
    : OFMsg(version, type),
      data_(NULL),
      data_len_(0),
      buffer_id_(0),
      actions_len_(0) {}

PacketOutCommon::PacketOutCommon(uint8_t version, uint16_t type, uint32_t xid,
                                 uint32_t buffer_id)
    : OFMsg(version, type, xid),
      data_(NULL),
      data_len_(0),
      buffer_id_(buffer_id),
      actions_len_(0) {}

PacketOutCommon::~PacketOutCommon() {
  if (this->data_len_) {
    ::operator delete(this->data_);
  }
}

bool PacketOutCommon::operator==(const PacketOutCommon &other) const {
  return ((OFMsg::operator==(other)) &&
          (this->buffer_id_ == other.buffer_id_) &&
          (this->actions_len_ == other.actions_len_) &&
          (this->actions_ == other.actions_) &&
          (!memcmp(this->data_, other.data_, this->data_len_)) &&
          (this->data_len_ == other.data_len_));
}

bool PacketOutCommon::operator!=(const PacketOutCommon &other) const {
  return !(*this == other);
}

void PacketOutCommon::actions(ActionList actions) {
  this->actions_ = actions;
  this->actions_len_ = actions.length();
  this->length_ += actions_len();
}

void PacketOutCommon::add_action(Action &action) {
  this->actions_.add_action(action);
  this->length_ += action.length();
  this->actions_len_ += action.length();
}

void PacketOutCommon::add_action(Action *action) {
  this->actions_.add_action(action);
  this->length_ += action->length();
  this->actions_len_ += action->length();
}

void PacketOutCommon::data(void *data, size_t len) {
  this->data_ = ::operator new(len);
  memcpy(this->data_, data, len);
  this->data_len_ = len;
  this->length_ += len;
}

PacketInCommon::PacketInCommon(uint8_t version, uint8_t type)
    : OFMsg(version, type),
      data_(NULL),
      data_len_(0),
      buffer_id_(0),
      total_len_(0),
      reason_(0) {}

PacketInCommon::PacketInCommon(uint8_t version, uint8_t type, uint32_t xid,
                               uint32_t buffer_id, uint16_t total_len,
                               uint8_t reason)
    : OFMsg(version, type, xid),
      data_(NULL),
      data_len_(0),
      buffer_id_(buffer_id),
      total_len_(total_len),
      reason_(reason) {}

bool PacketInCommon::operator==(const PacketInCommon &other) const {
  return ((OFMsg::operator==(other)) &&
          (this->buffer_id_ == other.buffer_id_) &&
          (this->total_len_ == other.total_len_) &&
          (this->reason_ == other.reason_) &&
          (!memcmp(this->data_, other.data_, this->data_len_)) &&
          (this->data_len_ == other.data_len_));
}

bool PacketInCommon::operator!=(const PacketInCommon &other) const {
  return !(*this == other);
}

void PacketInCommon::data(void *data, size_t len) {
  this->data_ = ::operator new(len);
  memcpy(this->data_, data, len);
  this->length_ += len;
  this->data_len_ = len;
}

PacketInCommon::~PacketInCommon() {
  if (this->data_len_) {
    ::operator delete(this->data_);
  }
}

FlowRemovedCommon::FlowRemovedCommon(uint8_t version, uint8_t type)
    : OFMsg(version, type),
      cookie_(0),
      priority_(0),
      reason_(0),
      duration_sec_(0),
      duration_nsec_(0),
      idle_timeout_(0),
      packet_count_(0),
      byte_count_(0) {}

FlowRemovedCommon::FlowRemovedCommon(uint8_t version, uint8_t type,
                                     uint32_t xid, uint64_t cookie,
                                     uint16_t priority, uint8_t reason,
                                     uint32_t duration_sec,
                                     uint32_t duration_nsec,
                                     uint16_t idle_timeout,
                                     uint64_t packet_count, uint64_t byte_count)
    : OFMsg(version, type, xid),
      cookie_(cookie),
      priority_(priority),
      reason_(reason),
      duration_sec_(duration_sec),
      duration_nsec_(duration_nsec),
      idle_timeout_(idle_timeout),
      packet_count_(packet_count),
      byte_count_(byte_count) {}

bool FlowRemovedCommon::operator==(const FlowRemovedCommon &other) const {
  return ((OFMsg::operator==(other)) && (this->cookie_ == other.cookie_) &&
          (this->priority_ == other.priority_) &&
          (this->reason_ == other.reason_) &&
          (this->duration_sec_ == other.duration_sec_) &&
          (this->duration_nsec_ == other.duration_nsec_) &&
          (this->idle_timeout_ == other.idle_timeout_) &&
          (this->packet_count_ == other.packet_count_) &&
          (this->byte_count_ == other.byte_count_));
}

bool FlowRemovedCommon::operator!=(const FlowRemovedCommon &other) const {
  return !(*this == other);
}

PortStatusCommon::PortStatusCommon(uint8_t version, uint8_t type)
    : OFMsg(version, type), reason_(0) {}

PortStatusCommon::PortStatusCommon(uint8_t version, uint8_t type, uint32_t xid,
                                   uint8_t reason)
    : OFMsg(version, type, xid), reason_(reason) {}

bool PortStatusCommon::operator==(const PortStatusCommon &other) const {
  return ((OFMsg::operator==(other)) && (this->reason_ == other.reason_));
}

bool PortStatusCommon::operator!=(const PortStatusCommon &other) const {
  return !(*this == other);
}

PortModCommon::PortModCommon(uint8_t version, uint8_t type)
    : OFMsg(version, type), config_(0), mask_(0), advertise_(0) {}

PortModCommon::PortModCommon(uint8_t version, uint8_t type, uint32_t xid,
                             EthAddress hw_addr, uint32_t config, uint32_t mask,
                             uint32_t advertise)
    : OFMsg(version, type, xid),
      hw_addr_(hw_addr),
      config_(config),
      mask_(mask),
      advertise_(advertise) {}

bool PortModCommon::operator==(const PortModCommon &other) const {
  return ((OFMsg::operator==(other)) && (this->hw_addr_ == other.hw_addr_) &&
          (this->config_ == other.config_) && (this->mask_ == other.mask_) &&
          (this->advertise_ == other.advertise_));
}

bool PortModCommon::operator!=(const PortModCommon &other) const {
  return !(*this == other);
}

}  // End of namespace fluid_msg

#ifndef MSG_H
#define MSG_H 1

#include <fluid/util/ethaddr.hh>
#include "action.hh"
#include "openflow-common.hh"

namespace fluid_msg {
class Action;
}

namespace fluid_msg {
/**
 Base class for OpenFlow messages.
 */
class OFMsg {
 protected:
  uint8_t version_;
  uint8_t type_;
  uint16_t length_;
  uint32_t xid_;

 public:
  OFMsg(uint8_t version, uint8_t type);
  OFMsg(uint8_t version, uint8_t type, uint32_t xid);
  OFMsg(uint8_t *buffer) { unpack(buffer); }
  virtual ~OFMsg() {}
  virtual uint8_t *pack();
  virtual of_error unpack(uint8_t *buffer);
  bool operator==(const OFMsg &other) const;
  bool operator!=(const OFMsg &other) const;
  uint8_t version() { return this->version_; }
  uint8_t type() { return this->type_; }
  // Length is virtual because we need to override
  // it in some classes where the length is padding
  // dependent (e.g OpenFlow 1.3 Flow Mod).
  virtual uint16_t length() { return this->length_; }
  uint32_t xid() { return this->xid_; }
  void version(uint8_t version) { this->version_ = version; }
  void msg_type(uint8_t type) { this->type_ = type; }
  void length(uint16_t length) { this->length_ = length; }
  void xid(uint32_t xid) { this->xid_ = xid; }
  static void free_buffer(uint8_t *buffer) { delete[] buffer; }
};

/**
 Base class for OpenFlow 1.0 and 1.3 Echo messages.
 */
class EchoCommon : public OFMsg {
 private:
  void *data_;
  size_t data_len_;

 public:
  EchoCommon(uint8_t version, uint8_t type);
  EchoCommon(uint8_t version, uint8_t type, uint32_t xid)
      : OFMsg(version, type, xid), data_(NULL), data_len_(0) {}
  virtual ~EchoCommon();
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  bool operator==(const EchoCommon &other) const;
  bool operator!=(const EchoCommon &other) const;
  void *data() { return this->data_; }
  size_t data_len() { return this->data_len_; }
  void data(void *data, size_t data_len);
};

/**
 Base class for OpenFlow 1.0 and 1.3 Error messages.
 */
class ErrorCommon : public OFMsg {
 protected:
  uint16_t err_type_;
  uint16_t code_;
  void *data_;
  size_t data_len_;

 public:
  ErrorCommon(uint8_t version, uint8_t type);
  ErrorCommon(uint8_t version, uint8_t type, uint32_t xid, uint16_t err_type,
              uint16_t code);
  ErrorCommon(uint8_t version, uint8_t type, uint32_t xid, uint16_t err_type,
              uint16_t code, void *data, size_t data_len);
  virtual ~ErrorCommon();
  bool operator==(const ErrorCommon &other) const;
  bool operator!=(const ErrorCommon &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t err_type() { return this->err_type_; }
  uint16_t code() { return this->code_; }
  void *data() { return this->data_; }
  size_t data_len() { return this->data_len_; }
  void err_type(uint16_t err_type) { this->err_type_ = err_type; }
  void code(uint16_t code) { this->code_ = code; }
  void data(void *data, size_t data_len);
};

/**
 Base class for OpenFlow 1.0 and 1.3 Features Reply messages.
 */
class FeaturesReplyCommon : public OFMsg {
 protected:
  uint64_t datapath_id_;
  uint32_t n_buffers_;
  uint8_t n_tables_;
  uint32_t capabilities_;

 public:
  FeaturesReplyCommon(uint8_t version, uint8_t type);
  FeaturesReplyCommon(uint8_t version, uint8_t type, uint32_t xid,
                      uint64_t datapath_id, uint32_t n_buffers,
                      uint8_t n_tables, uint32_t capabilities);
  virtual ~FeaturesReplyCommon() {}
  bool operator==(const FeaturesReplyCommon &other) const;
  bool operator!=(const FeaturesReplyCommon &other) const;
  uint64_t datapath_id() { return this->datapath_id_; }
  uint32_t n_buffers() { return this->n_buffers_; }
  uint8_t n_tables() { return this->n_tables_; }
  uint32_t capabilities() { return this->capabilities_; }
  void datapath_id(uint64_t datapath_id) { this->datapath_id_ = datapath_id; }
  void n_buffers(uint32_t n_buffers) { this->n_buffers_ = n_buffers; }
  void n_tables(uint8_t n_tables) { this->n_tables_ = n_tables; }
  void capabilities(uint32_t capabilities) {
    this->capabilities_ = capabilities;
  }
};

/**
 Base class for OpenFlow 1.0 and 1.3 Features Switch Config messages.
 */
class SwitchConfigCommon : public OFMsg {
 private:
  uint16_t flags_;
  uint16_t miss_send_len_;

 public:
  SwitchConfigCommon(uint8_t version, uint8_t type);
  SwitchConfigCommon(uint8_t version, uint8_t type, uint32_t xid,
                     uint16_t flags, uint16_t miss_send_len);
  virtual ~SwitchConfigCommon() {}
  bool operator==(const SwitchConfigCommon &other) const;
  bool operator!=(const SwitchConfigCommon &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t flags() { return this->flags_; }
  uint16_t miss_send_len() { return this->miss_send_len_; }
  void flags(uint16_t flags) { this->flags_ = flags; }
  void miss_send_len(uint16_t miss_send_len) {
    this->miss_send_len_ = miss_send_len;
  }
};

/**
 Base class for OpenFlow 1.0 and 1.3 Flow Mod messages.
 */
class FlowModCommon : public OFMsg {
 protected:
  uint64_t cookie_;
  uint16_t idle_timeout_;
  uint16_t hard_timeout_;
  uint16_t priority_;
  uint32_t buffer_id_;
  uint16_t flags_;

 public:
  FlowModCommon(uint8_t version, uint8_t type);
  FlowModCommon(uint8_t version, uint8_t type, uint32_t xid, uint64_t cookie,
                uint16_t idle_timeout, uint16_t hard_timeout, uint16_t priority,
                uint32_t buffer_id, uint16_t flags);
  virtual ~FlowModCommon(){};
  bool operator==(const FlowModCommon &other) const;
  bool operator!=(const FlowModCommon &other) const;
  uint64_t cookie() { return this->cookie_; }
  uint16_t idle_timeout() { return this->idle_timeout_; }
  uint16_t hard_timeout() { return this->hard_timeout_; }
  uint16_t priority() { return this->priority_; }
  uint32_t buffer_id() { return this->buffer_id_; }
  uint16_t flags() { return this->flags_; }
  void cookie(uint64_t cookie) { this->cookie_ = cookie; }
  void idle_timeout(uint16_t idle_timeout) {
    this->idle_timeout_ = idle_timeout;
  }
  void hard_timeout(uint16_t hard_timeout) {
    this->hard_timeout_ = hard_timeout;
  }
  void priority(uint16_t priority) { this->priority_ = priority; }
  void buffer_id(uint32_t buffer_id) { this->buffer_id_ = buffer_id; }
  void flags(uint16_t flags) { this->flags_ = flags; }
};

/**
 Base class for OpenFlow 1.0 and 1.3 Packet Out messages.
 */
class PacketOutCommon : public OFMsg {
 protected:
  uint32_t buffer_id_;
  uint16_t actions_len_;
  ActionList actions_;
  void *data_;
  size_t data_len_;

 public:
  PacketOutCommon(uint8_t version, uint8_t type);
  PacketOutCommon(uint8_t version, uint16_t type, uint32_t xid,
                  uint32_t buffer_id);
  virtual ~PacketOutCommon();
  bool operator==(const PacketOutCommon &other) const;
  bool operator!=(const PacketOutCommon &other) const;
  uint32_t buffer_id() { return this->buffer_id_; }
  uint16_t actions_len() { return this->actions_len_; }
  ActionList actions() { return this->actions_; }
  size_t data_len() { return this->data_len_; }
  void *data() { return this->data_; }
  void buffer_id(uint32_t buffer_id) { this->buffer_id_ = buffer_id; }
  void actions(ActionList actions);
  void add_action(Action &action);
  void add_action(Action *action);
  void data(void *data, size_t len);
};

/**
 Base class for OpenFlow 1.0 and 1.3 Packet In messages.
 */
class PacketInCommon : public OFMsg {
 protected:
  uint32_t buffer_id_;
  uint16_t total_len_;
  uint8_t reason_;
  size_t data_len_;
  void *data_;

 public:
  PacketInCommon(uint8_t version, uint8_t type);
  PacketInCommon(uint8_t version, uint8_t type, uint32_t xid,
                 uint32_t buffer_id, uint16_t total_len, uint8_t reason);
  virtual ~PacketInCommon();
  bool operator==(const PacketInCommon &other) const;
  bool operator!=(const PacketInCommon &other) const;
  uint32_t buffer_id() const { return this->buffer_id_; }
  uint16_t total_len() { return this->total_len_; }
  uint8_t reason() const { return this->reason_; }
  void *data() const { return this->data_; }
  size_t data_len() const { return this->data_len_; }
  void buffer_id(uint32_t buffer_id) { this->buffer_id_ = buffer_id; }
  void total_len(uint16_t total_len) { this->total_len_ = total_len; }
  void reason(uint8_t reason) { this->reason_ = reason; }
  void data(void *data, size_t len);
};

/**
 Base class for OpenFlow 1.0 and 1.3 Flow Removed messages.
 */
class FlowRemovedCommon : public OFMsg {
 protected:
  uint64_t cookie_;
  uint16_t priority_;
  uint8_t reason_;
  uint32_t duration_sec_;
  uint32_t duration_nsec_;
  uint16_t idle_timeout_;
  uint64_t packet_count_;
  uint64_t byte_count_;

 public:
  FlowRemovedCommon(uint8_t version, uint8_t type);
  FlowRemovedCommon(uint8_t version, uint8_t type, uint32_t xid,
                    uint64_t cookie, uint16_t priority, uint8_t reason,
                    uint32_t duration_sec, uint32_t duration_nsec,
                    uint16_t idle_timeout, uint64_t packet_count,
                    uint64_t byte_count);
  virtual ~FlowRemovedCommon() {}
  bool operator==(const FlowRemovedCommon &other) const;
  bool operator!=(const FlowRemovedCommon &other) const;
  uint64_t cookie() { return this->cookie_; }
  uint16_t priority() { return this->priority_; }
  uint8_t reason() { return this->reason_; }
  uint32_t duration_sec() { return this->duration_sec_; }
  uint32_t duration_nsec() { return this->duration_nsec_; }
  uint64_t packet_count() { return this->packet_count_; }
  uint64_t byte_count() { return this->byte_count_; }
  void cookie(uint64_t cookie) { this->cookie_ = cookie; }
  void priority(uint16_t priority) { this->priority_ = priority; }
  void reason(uint8_t reason) { this->reason_ = reason; }
  void duration_sec(uint32_t duration_sec) {
    this->duration_sec_ = duration_sec;
  }
  void duration_nsec(uint32_t duration_nsec) {
    this->duration_nsec_ = duration_nsec;
  }
  void idle_timeout(uint16_t idle_timeout) {
    this->idle_timeout_ = idle_timeout;
  }
  void packet_count(uint64_t packet_count) {
    this->packet_count_ = packet_count;
  }
  void byte_count(uint64_t byte_count) { this->byte_count_ = byte_count; }
};

/**
 Base class for OpenFlow 1.0 and 1.3 Port Status messages.
 */
class PortStatusCommon : public OFMsg {
 protected:
  uint8_t reason_;

 public:
  PortStatusCommon(uint8_t version, uint8_t type);
  PortStatusCommon(uint8_t version, uint8_t type, uint32_t xid, uint8_t reason);
  virtual ~PortStatusCommon() {}
  bool operator==(const PortStatusCommon &other) const;
  bool operator!=(const PortStatusCommon &other) const;
  uint8_t reason() { return this->reason_; }
  void reason(uint8_t reason) { this->reason_ = reason; }
};

/**
 Base class for OpenFlow 1.0 and 1.3 Port Mod messages.
 */
class PortModCommon : public OFMsg {
 protected:
  EthAddress hw_addr_;
  uint32_t config_;
  uint32_t mask_;
  uint32_t advertise_;

 public:
  PortModCommon(uint8_t version, uint8_t type);
  PortModCommon(uint8_t version, uint8_t type, uint32_t xid, EthAddress hw_addr,
                uint32_t config, uint32_t mask, uint32_t advertise);
  virtual ~PortModCommon() {}
  bool operator==(const PortModCommon &other) const;
  bool operator!=(const PortModCommon &other) const;
  EthAddress hw_addr() { return this->hw_addr_; }
  uint32_t config() { return this->config_; }
  uint32_t mask() { return this->mask_; }
  uint32_t advertise() { return this->advertise_; }
  void hw_addr(EthAddress hw_addr) { this->hw_addr_ = hw_addr; }
  void config(uint32_t config) { this->config_ = config; }
  void mask(uint32_t mask) { this->mask_ = mask; }
  void advertise(uint32_t advertise) { this->advertise_ = advertise; }
};

}  // End of namespace fluid_msg
#endif

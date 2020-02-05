#ifndef OF10MSG_H
#define OF10MSG_H 1

#include "fluid/ofcommon/msg.hh"
#include "of10/of10action.hh"
#include "of10/of10common.hh"

/**
 Classes for creating and parsing OpenFlow messages.
 */
namespace fluid_msg {

/**
 Classes for creating and parsing OpenFlow 1.0 messages.
 */
namespace of10 {

/**
 OpenFlow 1.0 OFPT_HELLO message.
 */
class Hello : public OFMsg {
 public:
  Hello();
  Hello(uint32_t xid);
  ~Hello() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.0 OFPT_ERROR message.
 */
class Error : public ErrorCommon {
 public:
  Error();
  Error(uint32_t xid, uint16_t err_type, uint16_t code);
  Error(uint32_t xid, uint16_t err_type, uint16_t code, uint8_t *data,
        size_t data_len);
  ~Error() {}
};

/**
 OpenFlow 1.0 OFPT_ECHO_REQUEST message.
 */
class EchoRequest : public EchoCommon {
 public:
  EchoRequest() : EchoCommon(of10::OFP_VERSION, of10::OFPT_ECHO_REQUEST) {}
  EchoRequest(uint32_t xid);
  ~EchoRequest() {}
};

/**
 OpenFlow 1.0 OFPT_ECHO_REPLY message.
 */
class EchoReply : public EchoCommon {
 public:
  EchoReply() : EchoCommon(of10::OFP_VERSION, of10::OFPT_ECHO_REPLY) {}
  EchoReply(uint32_t xid);
  ~EchoReply() {}
};

/**
 OpenFlow 1.0 OFPT_VENDOR message.
 Vendor messages should inherit from this class.
 */
class Vendor : public OFMsg {
 protected:
  uint32_t vendor_;

 public:
  Vendor();
  Vendor(uint32_t xid, uint32_t vendor);
  ~Vendor() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t vendor() { return this->vendor_; }
  void vendor(uint32_t vendor) { this->vendor_ = vendor; }
};

/**
 OpenFlow 1.0 OFPT_FEATURES_REQUEST message.
 */
class FeaturesRequest : public OFMsg {
 public:
  FeaturesRequest();
  FeaturesRequest(uint32_t xid);
  ~FeaturesRequest() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.0 OFPT_FEATURES_REPLY message.
 */
class FeaturesReply : public FeaturesReplyCommon {
 private:
  uint32_t actions_;
  std::vector<of10::Port> ports_;

 public:
  FeaturesReply();
  FeaturesReply(uint32_t xid, uint64_t datapath_id, uint32_t n_buffers,
                uint8_t n_tables, uint32_t capabilities, uint32_t actions);
  FeaturesReply(uint32_t xid, uint64_t datapath_id, uint32_t n_buffers,
                uint8_t n_tables, uint32_t capabilities, uint32_t actions,
                std::vector<of10::Port> ports);
  bool operator==(const FeaturesReply &other) const;
  bool operator!=(const FeaturesReply &other) const;
  ~FeaturesReply() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t actions() { return this->actions_; }
  std::vector<of10::Port> ports() { return this->ports_; }
  void actions(uint32_t actions) { this->actions_ = actions; }
  void ports(std::vector<of10::Port> ports);
  size_t ports_length();
  void add_port(of10::Port port);
};

/**
 OpenFlow 1.0 OFPT_GET_CONFIG_REQUEST message.
 */
class GetConfigRequest : public OFMsg {
 public:
  GetConfigRequest();
  GetConfigRequest(uint32_t xid);
  ~GetConfigRequest() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.0 OFPT_GET_CONFIG_REPLY message.
 */
class GetConfigReply : public SwitchConfigCommon {
 public:
  GetConfigReply();
  GetConfigReply(uint32_t xid, uint16_t flags, uint16_t miss_send_len);
  ~GetConfigReply() {}
};

/**
 OpenFlow 1.0 OFPT_SET_CONFIG message.
 */
class SetConfig : public SwitchConfigCommon {
 public:
  SetConfig();
  SetConfig(uint32_t xid, uint16_t flags, uint16_t miss_send_len);
  ~SetConfig() {}
};

/**
 OpenFlow 1.0 OFPT_FLOW_MOD message.
 */
class FlowMod : public FlowModCommon {
 private:
  uint16_t command_;
  uint16_t out_port_;
  of10::Match match_;
  ActionList actions_;

 public:
  FlowMod();
  FlowMod(uint32_t xid, uint64_t cookie, uint16_t command,
          uint16_t idle_timeout, uint16_t hard_timeout, uint16_t priority,
          uint32_t buffer_id, uint16_t out_port, uint16_t flags);
  FlowMod(uint32_t xid, uint64_t cookie, uint16_t command,
          uint16_t idle_timeout, uint16_t hard_timeout, uint16_t priority,
          uint32_t buffer_id, uint16_t out_port, uint16_t flags,
          of10::Match match);
  FlowMod(uint32_t xid, uint64_t cookie, uint16_t command,
          uint16_t idle_timeout, uint16_t hard_timeout, uint16_t priority,
          uint32_t buffer_id, uint16_t out_port, uint16_t flags,
          of10::Match match, ActionList actions);
  ~FlowMod() {}
  bool operator==(const FlowMod &other) const;
  bool operator!=(const FlowMod &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t command() { return this->command_; }
  of10::Match match() { return this->match_; }
  ActionList actions() { return this->actions_; }
  uint16_t out_port() { return this->out_port_; }
  void command(uint16_t command) { this->command_ = command; }
  void out_port(uint16_t out_port) { this->out_port_ = out_port; }
  void match(const of10::Match &match) { this->match_ = match; }
  void actions(const ActionList &actions);
  void add_action(Action &action);
  void add_action(Action *action);
};

/**
 OpenFlow 1.0 OFPT_PACKET_OUT message.
 */
class PacketOut : public PacketOutCommon {
 private:
  uint16_t in_port_;

 public:
  PacketOut();
  PacketOut(uint32_t xid, uint32_t buffer_id, uint16_t in_port);
  ~PacketOut() {}
  bool operator==(const PacketOut &other) const;
  bool operator!=(const PacketOut &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t in_port() { return this->in_port_; }
  void in_port(uint16_t in_port) { this->in_port_ = in_port; }
};

/**
 OpenFlow 1.0 OFPT_PACKET_IN message.
 */
class PacketIn : public PacketInCommon {
 private:
  uint16_t in_port_;

 public:
  PacketIn();
  PacketIn(uint32_t xid, uint32_t buffer_id, uint16_t in_port,
           uint16_t total_len, uint8_t reason);
  ~PacketIn() {}
  bool operator==(const PacketIn &other) const;
  bool operator!=(const PacketIn &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t in_port() { return this->in_port_; }
  void in_port(uint16_t in_port) { this->in_port_ = in_port; }
};

/**
 OpenFlow 1.0 OFPT_FLOW_REMOVED message.
 */
class FlowRemoved : public FlowRemovedCommon {
 private:
  of10::Match match_;

 public:
  FlowRemoved();
  FlowRemoved(uint32_t xid, uint64_t cookie, uint16_t priority, uint8_t reason,
              uint32_t duration_sec, uint32_t duration_nsec,
              uint16_t idle_timeout, uint64_t packet_count,
              uint64_t byte_count);
  FlowRemoved(uint32_t xid, uint64_t cookie, uint16_t priority, uint8_t reason,
              uint32_t duration_sec, uint32_t duration_nsec,
              uint16_t idle_timeout, uint64_t packet_count, uint64_t byte_count,
              of10::Match match);
  ~FlowRemoved() {}
  bool operator==(const FlowRemoved &other) const;
  bool operator!=(const FlowRemoved &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  of10::Match match() { return this->match_; }
  void match(of10::Match match) { this->match_ = match; }
};

/**
 OpenFlow 1.0 OFPT_PORT_STATUS message.
 */
class PortStatus : public PortStatusCommon {
 private:
  of10::Port desc_;

 public:
  PortStatus();
  PortStatus(uint32_t xid, uint8_t reason, of10::Port desc);
  ~PortStatus() {}
  bool operator==(const PortStatus &other) const;
  bool operator!=(const PortStatus &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  of10::Port desc() { return this->desc_; }
  void desc(of10::Port desc) { this->desc_ = desc; }
};

/**
 OpenFlow 1.0 OFPT_PORT_MOD message.
 */
class PortMod : public PortModCommon {
 private:
  uint16_t port_no_;

 public:
  PortMod();
  PortMod(uint32_t xid, uint16_t port_no, EthAddress hw_addr, uint32_t config,
          uint32_t mask, uint32_t advertise);
  ~PortMod() {}
  bool operator==(const PortMod &other) const;
  bool operator!=(const PortMod &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t port_no() { return this->port_no_; }
  void port_no(uint16_t port_no) { this->port_no_ = port_no; }
};

/**
 OpenFlow 1.0 OFPT_STATS_REQUEST message header. Stats request
 messages should inherit from this class.
 */
class StatsRequest : public OFMsg {
 protected:
  uint16_t stats_type_;
  uint16_t flags_;

 public:
  StatsRequest();
  StatsRequest(uint16_t);
  StatsRequest(uint32_t xid, uint16_t type, uint16_t flags);
  ~StatsRequest() {}
  bool operator==(const StatsRequest &other) const;
  bool operator!=(const StatsRequest &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t stats_type() { return this->stats_type_; }
  uint16_t flags() { return this->flags_; }
  void stats_type(uint16_t type) { this->stats_type_ = type; }
  void flags(uint16_t flags) { this->flags_ = flags; }
};

/**
 OpenFlow 1.0 OFPT_STATS_REPLY message header. Stats reply
 messages should inherit from this class.
 */
class StatsReply : public OFMsg {
 protected:
  uint16_t stats_type_;
  uint16_t flags_;

 public:
  StatsReply();
  StatsReply(uint16_t type);
  StatsReply(uint32_t xid, uint16_t type, uint16_t flags);
  ~StatsReply() {}
  bool operator==(const StatsReply &other) const;
  bool operator!=(const StatsReply &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t stats_type() { return this->stats_type_; }
  uint16_t flags() { return this->flags_; }
  void stats_type(uint16_t type) { this->stats_type_ = type; }
  void flags(uint16_t flags) { this->flags_ = flags; }
};

/**
 OpenFlow 1.0 OFPST_DESC multipart request.
 */
class StatsRequestDesc : public StatsRequest {
 public:
  StatsRequestDesc();
  StatsRequestDesc(uint32_t xid, uint16_t flags);
  ~StatsRequestDesc() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.0 OFPST_DESC multipart reply.
 */
class StatsReplyDesc : public StatsReply {
 private:
  SwitchDesc desc_;

 public:
  StatsReplyDesc();
  StatsReplyDesc(uint32_t xid, uint16_t flags, SwitchDesc desc);
  StatsReplyDesc(uint32_t xid, uint16_t flags, std::string mfr_desc,
                 std::string hw_desc, std::string sw_desc,
                 std::string serial_num, std::string dp_desc);
  ~StatsReplyDesc() {}
  bool operator==(const StatsReplyDesc &other) const;
  bool operator!=(const StatsReplyDesc &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  SwitchDesc desc() { return this->desc_; }
  void desc(SwitchDesc desc);
};

/**
 OpenFlow 1.0 OFPST_FLOW multipart request.
 */
class StatsRequestFlow : public StatsRequest {
 private:
  of10::Match match_;
  uint8_t table_id_;
  uint16_t out_port_;

 public:
  StatsRequestFlow();
  StatsRequestFlow(uint32_t xid, uint16_t flags, uint8_t table_id,
                   uint16_t out_port);
  StatsRequestFlow(uint32_t xid, uint16_t flags, of10::Match match,
                   uint8_t table_id, uint16_t out_port);
  virtual ~StatsRequestFlow() {}
  bool operator==(const StatsRequestFlow &other) const;
  bool operator!=(const StatsRequestFlow &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  of10::Match match() { return this->match_; }
  uint8_t table_id() { return this->table_id_; }
  uint16_t out_port() { return this->out_port_; }
  void match(of10::Match match) { this->match_ = match; }
  void table_id(uint8_t table_id) { this->table_id_ = table_id; }
  void out_port(uint16_t out_port) { this->out_port_ = out_port; }
};

/**
 OpenFlow 1.0 OFPST_FLOW multipart reply.
 */
class StatsReplyFlow : public StatsReply {
 private:
  std::vector<of10::FlowStats> flow_stats_;

 public:
  StatsReplyFlow();
  StatsReplyFlow(uint32_t xid, uint16_t flags);
  StatsReplyFlow(uint32_t xid, uint16_t flags,
                 std::vector<of10::FlowStats> flow_stats);
  virtual ~StatsReplyFlow() {}
  bool operator==(const StatsReplyFlow &other) const;
  bool operator!=(const StatsReplyFlow &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<of10::FlowStats> flow_stats() { return this->flow_stats_; }
  void flow_stats(std::vector<of10::FlowStats> flow_stats);
  void add_flow_stats(of10::FlowStats);
};

/**
 OpenFlow 1.0 OFPST_AGGREGATE multipart request.
 */
class StatsRequestAggregate : public StatsRequest {
 private:
  of10::Match match_;
  uint8_t table_id_;
  uint16_t out_port_;

 public:
  StatsRequestAggregate();
  StatsRequestAggregate(uint32_t xid, uint16_t flags, uint8_t table_id,
                        uint16_t out_port);
  StatsRequestAggregate(uint32_t xid, uint16_t flags, of10::Match match,
                        uint8_t table_id, uint16_t out_port);
  ~StatsRequestAggregate() {}
  bool operator==(const StatsRequestAggregate &other) const;
  bool operator!=(const StatsRequestAggregate &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  of10::Match match() { return this->match_; }
  uint8_t table_id() { return this->table_id_; }
  uint16_t out_port() { return this->out_port_; }
  void match(of10::Match match) { this->match_ = match; }
  void table_id(uint8_t table_id) { this->table_id_ = table_id; }
  void out_port(uint16_t out_port) { this->out_port_ = out_port; }
};

/**
 OpenFlow 1.0 OFPST_AGGREGATE multipart reply.
 */
class StatsReplyAggregate : public StatsReply {
 private:
  uint64_t packet_count_;
  uint64_t byte_count_;
  uint32_t flow_count_;

 public:
  StatsReplyAggregate();
  StatsReplyAggregate(uint32_t xid, uint16_t flags, uint64_t packet_count,
                      uint64_t byte_count, uint32_t flow_count);
  ~StatsReplyAggregate() {}
  bool operator==(const StatsReplyAggregate &other) const;
  bool operator!=(const StatsReplyAggregate &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint64_t packet_count() { return this->packet_count_; }
  uint64_t byte_count() { return this->byte_count_; }
  uint32_t flow_count() { return this->flow_count_; }
  void packet_count(uint64_t packet_count) {
    this->packet_count_ = packet_count;
  }
  void byte_count(uint64_t byte_count) { this->byte_count_ = byte_count; }
  void flow_count(uint32_t flow_count) { this->flow_count_ = flow_count; }
};

/**
 OpenFlow 1.0 OFPST_TABLE multipart request.
 */
class StatsRequestTable : public StatsRequest {
 public:
  StatsRequestTable();
  StatsRequestTable(uint32_t xid, uint16_t flags);
  ~StatsRequestTable() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.0 OFPST_TABLE multipart reply.
 */
class StatsReplyTable : public StatsReply {
 private:
  std::vector<of10::TableStats> table_stats_;

 public:
  StatsReplyTable();
  StatsReplyTable(uint32_t xid, uint16_t flags);
  StatsReplyTable(uint32_t xid, uint16_t flags,
                  std::vector<of10::TableStats> table_stats);
  ~StatsReplyTable() {}
  bool operator==(const StatsReplyTable &other) const;
  bool operator!=(const StatsReplyTable &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<of10::TableStats> table_stats() { return this->table_stats_; }
  void table_stats(std::vector<of10::TableStats> table_stats);
  void add_table_stat(of10::TableStats stat);
};

/**
 OpenFlow 1.0 OFPST_PORT_STATS multipart request.
 */
class StatsRequestPort : public StatsRequest {
 private:
  uint16_t port_no_;

 public:
  StatsRequestPort();
  StatsRequestPort(uint32_t xid, uint16_t flags, uint16_t port_no);
  ~StatsRequestPort() {}
  bool operator==(const StatsRequestPort &other) const;
  bool operator!=(const StatsRequestPort &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t port_no() { return this->port_no_; }
  void port_no(uint16_t port_no) { this->port_no_ = port_no; }
};

/**
 OpenFlow 1.0 OFPST_PORT_STATS multipart reply.
 */
class StatsReplyPort : public StatsReply {
 private:
  std::vector<PortStats> port_stats_;

 public:
  StatsReplyPort();
  StatsReplyPort(uint32_t xid, uint16_t flags);
  StatsReplyPort(uint32_t xid, uint16_t flags,
                 std::vector<of10::PortStats> port_stats);
  ~StatsReplyPort() {}
  bool operator==(const StatsReplyPort &other) const;
  bool operator!=(const StatsReplyPort &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<of10::PortStats> port_stats() { return this->port_stats_; }
  void port_stats(std::vector<of10::PortStats> port_stats);
  void add_port_stat(of10::PortStats stat);
};

/**
 OpenFlow 1.0 OFPST_QUEUE multipart request.
 */
class StatsRequestQueue : public StatsRequest {
 private:
  uint16_t port_no_;
  uint32_t queue_id_;

 public:
  StatsRequestQueue();
  StatsRequestQueue(uint32_t xid, uint16_t flags, uint16_t port_no,
                    uint32_t queue_id);
  ~StatsRequestQueue() {}
  bool operator==(const StatsRequestQueue &other) const;
  bool operator!=(const StatsRequestQueue &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t port_no() { return this->port_no_; }
  uint32_t queue_id() { return this->queue_id_; }
  void port_no(uint16_t port_no) { this->port_no_ = port_no; }
  void queue_id(uint32_t queue_id) { this->queue_id_ = queue_id; }
};

/**
 OpenFlow 1.0 OFPST_QUEUE multipart reply.
 */
class StatsReplyQueue : public StatsReply {
 private:
  std::vector<of10::QueueStats> queue_stats_;

 public:
  StatsReplyQueue();
  StatsReplyQueue(uint32_t xid, uint16_t flags);
  StatsReplyQueue(uint32_t xid, uint16_t flags,
                  std::vector<of10::QueueStats> queue_stats);
  ~StatsReplyQueue() {}
  bool operator==(const StatsReplyQueue &other) const;
  bool operator!=(const StatsReplyQueue &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<of10::QueueStats> queue_stats() { return this->queue_stats_; }
  void queue_stats(std::vector<of10::QueueStats> queue_stats_);
  void add_queue_stat(of10::QueueStats stat);
};

/**
 OpenFlow 1.0 OFPST_VENDOR stats request.
 Vendor stats request messages should inherit from this class.
 */
class StatsRequestVendor : public StatsRequest {
 protected:
  uint32_t vendor_;

 public:
  StatsRequestVendor();
  StatsRequestVendor(uint32_t xid, uint16_t flags, uint32_t vendor);
  virtual ~StatsRequestVendor() {}
  bool operator==(const StatsRequestVendor &other) const;
  bool operator!=(const StatsRequestVendor &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t vendor() { return this->vendor_; }
  void vendor(uint32_t vendor) { this->vendor_ = vendor; }
};

/**
 OpenFlow 1.0 OFPST_VENDOR stats reply.
 Vendor stats reply messages should inherit from this class.
 */
class StatsReplyVendor : public StatsReply {
 protected:
  uint32_t vendor_;

 public:
  StatsReplyVendor();
  StatsReplyVendor(uint32_t xid, uint16_t flags, uint32_t vendor);
  virtual ~StatsReplyVendor() {}
  bool operator==(const StatsReplyVendor &other) const;
  bool operator!=(const StatsReplyVendor &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t vendor() { return this->vendor_; }
  void vendor(uint32_t vendor) { this->vendor_ = vendor; }
};

/**
 OpenFlow 1.0 OFPT_QUEUE_GET_CONFIG_REQUEST message.
 */
class QueueGetConfigRequest : public OFMsg {
 private:
  uint16_t port_;

 public:
  QueueGetConfigRequest();
  QueueGetConfigRequest(uint32_t xid, uint16_t port);
  ~QueueGetConfigRequest() {}
  bool operator==(const QueueGetConfigRequest &other) const;
  bool operator!=(const QueueGetConfigRequest &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t port() { return this->port_; }
  void port(uint16_t port) { this->port_ = port; }
};

/**
 OpenFlow 1.0 OFPT_QUEUE_GET_CONFIG_REPLY message.
 */
class QueueGetConfigReply : public OFMsg {
 private:
  uint16_t port_;
  std::list<of10::PacketQueue> queues_;

 public:
  QueueGetConfigReply();
  QueueGetConfigReply(uint32_t xid, uint16_t port);
  QueueGetConfigReply(uint32_t xid, uint16_t port,
                      std::list<of10::PacketQueue> queues);
  ~QueueGetConfigReply() {}
  bool operator==(const QueueGetConfigReply &other) const;
  bool operator!=(const QueueGetConfigReply &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t port() { return this->port_; }
  std::list<of10::PacketQueue> queues() { return this->queues_; }
  void port(uint32_t port) { this->port_ = port; }
  void queues(std::list<PacketQueue> queues);
  void add_queue(PacketQueue queue);
  size_t queues_len();
};

/**
 OpenFlow 1.0 OFPT_BARRIER_REQUEST message
 */
class BarrierRequest : public OFMsg {
 public:
  BarrierRequest();
  BarrierRequest(uint32_t xid);
  ~BarrierRequest() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.0 OFPT_BARRIER_REPLY message*/
class BarrierReply : public OFMsg {
 public:
  BarrierReply();
  BarrierReply(uint32_t xid);
  ~BarrierReply() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

}  // end of namespace of10
}  // End of namespace fluid_msg

#endif

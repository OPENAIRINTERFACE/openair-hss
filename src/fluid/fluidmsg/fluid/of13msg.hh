#ifndef OF13MSG_H
#define OF13MSG_H 1

#include "fluid/ofcommon/msg.hh"
#include "of13/of13action.hh"
#include "of13/of13common.hh"
#include "of13/of13meter.hh"

namespace fluid_msg {

/**
 Base class for OpenFlow 1.3 Role messages.
 */
class RoleCommon : public OFMsg {
 private:
  uint32_t role_;
  uint64_t generation_id_;

 public:
  RoleCommon(uint8_t version, uint8_t type);
  RoleCommon(uint8_t version, uint8_t type, uint32_t xid, uint32_t role,
             uint64_t generation_id);
  virtual ~RoleCommon() {}
  bool operator==(const RoleCommon &other) const;
  bool operator!=(const RoleCommon &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t role() { return this->role_; }
  uint64_t generation_id() { return this->generation_id_; }
  void role(uint32_t role) { this->role_ = role; }
  void generation_id(uint64_t generation_id) {
    this->generation_id_ = generation_id;
  }
};

/**
 Base class for OpenFlow 1.3 Async Config messages.
 */
class AsyncConfigCommon : public OFMsg {
 protected:
  std::vector<uint32_t> packet_in_mask_;
  std::vector<uint32_t> port_status_mask_;
  std::vector<uint32_t> flow_removed_mask_;

 public:
  AsyncConfigCommon(uint8_t version, uint8_t type);
  AsyncConfigCommon(uint8_t version, uint8_t type, uint32_t xid);
  AsyncConfigCommon(uint8_t version, uint8_t type, uint32_t xid,
                    std::vector<uint32_t> packet_in_mask,
                    std::vector<uint32_t> port_status_mask,
                    std::vector<uint32_t> flow_removed_mask);
  virtual ~AsyncConfigCommon() {}
  bool operator==(const AsyncConfigCommon &other) const;
  bool operator!=(const AsyncConfigCommon &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  /*TODO check for specific message type */
  uint32_t master_equal_packet_in_mask() { return this->packet_in_mask_[0]; }
  uint32_t master_equal_port_status_mask() {
    return this->port_status_mask_[0];
  }
  uint32_t master_equal_flow_removed_mask() {
    return this->port_status_mask_[0];
  }
  uint32_t slave_packet_in_mask() { return this->packet_in_mask_[1]; }
  uint32_t slave_port_status_mask() { return this->port_status_mask_[1]; }
  uint32_t slave_flow_removed_mask() { return this->port_status_mask_[1]; }
  void master_equal_packet_in_mask(uint32_t mask) {
    this->packet_in_mask_[0] = mask;
  }
  void master_equal_port_status_mask(uint32_t mask) {
    this->port_status_mask_[0] = mask;
  }
  void master_equal_flow_removed_mask(uint32_t mask) {
    this->port_status_mask_[0] = mask;
  }
  void slave_packet_in_mask(uint32_t mask) { this->packet_in_mask_[1] = mask; }
  void slave_port_status_mask(uint32_t mask) {
    this->port_status_mask_[1] = mask;
  }
  void slave_flow_removed_mask(uint32_t mask) {
    this->port_status_mask_[1] = mask;
  }
};

/**
 Classes for creating and parsing OpenFlow 1.3 messages.
 */
namespace of13 {

/**
 OpenFlow 1.3 OFPT_HELLO message
 */
class Hello : public OFMsg {
 private:
  std::list<HelloElemVersionBitmap> elements_;

 public:
  Hello();
  Hello(uint32_t xid);
  Hello(uint32_t xid, std::list<HelloElemVersionBitmap> elements);
  ~Hello() {}
  bool operator==(const Hello &other) const;
  bool operator!=(const Hello &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::list<HelloElemVersionBitmap> elements() { return this->elements_; }
  void elements(std::list<HelloElemVersionBitmap> elements);
  void add_element(HelloElemVersionBitmap element);
  uint32_t elements_len();
};

/**
 OpenFlow 1.3 OFPT_ERROR message.
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
 OpenFlow 1.3 OFPT_ECHO_REQUEST message.
 */
class EchoRequest : public EchoCommon {
 public:
  EchoRequest();
  EchoRequest(uint32_t xid);
  ~EchoRequest() {}
};

/**
 OpenFlow 1.3 OFPT_ECHO_REPLY message.
 */
class EchoReply : public EchoCommon {
 public:
  EchoReply();
  EchoReply(uint32_t xid);
  ~EchoReply() {}
};

/**
 OpenFlow 1.3 OFPT_EXPERIMENTER message.
 Experimenter messages should inherit from this class.
 */
class Experimenter : public OFMsg {
 protected:
  uint32_t experimenter_;
  uint32_t exp_type_;

 public:
  Experimenter();
  Experimenter(uint32_t xid, uint32_t experimenter, uint32_t exp_type);
  virtual ~Experimenter() {}
  bool operator==(const Experimenter &other) const;
  bool operator!=(const Experimenter &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t experimenter() { return this->experimenter_; }
  uint32_t exp_type() { return this->exp_type_; }
};

/**
 OpenFlow 1.3 OFPT_FEATURES_REQUEST message.
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
 OpenFlow 1.3 OFPT_FEATURES_REPLY message.
 */
class FeaturesReply : public FeaturesReplyCommon {
  uint8_t auxiliary_id_;

 public:
  FeaturesReply();
  FeaturesReply(uint32_t xid, uint64_t datapath_id, uint32_t n_buffers,
                uint8_t n_tables, uint8_t auxiliary_id, uint32_t capabilities);
  ~FeaturesReply() {}
  bool operator==(const FeaturesReply &other) const;
  bool operator!=(const FeaturesReply &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint8_t auxiliary_id() { return this->auxiliary_id_; }
  void auxiliary_id(uint8_t auxiliary_id) {
    this->auxiliary_id_ = auxiliary_id;
  }
};

/**
 OpenFlow 1.3 OFPT_GET_CONFIG_REQUEST message.
 */
class GetConfigRequest : public OFMsg {
 public:
  GetConfigRequest();
  GetConfigRequest(uint32_t xid);
  ~GetConfigRequest(){};
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.3 OFPT_GET_CONFIG_REPLY message.
 */
class GetConfigReply : public SwitchConfigCommon {
 public:
  GetConfigReply();
  GetConfigReply(uint32_t xid, uint16_t flags, uint16_t miss_send_len);
  ~GetConfigReply(){};
};

/**
 OpenFlow 1.3 OFPT_SET_CONFIG_REPLY message.
 */
class SetConfig : public SwitchConfigCommon {
 public:
  SetConfig();
  SetConfig(uint32_t xid, uint16_t flags, uint16_t miss_send_len);
  ~SetConfig(){};
};

/**
 OpenFlow 1.3 OFPT_PACKET_OUT message.
 */
class PacketOut : public PacketOutCommon {
 private:
  uint32_t in_port_;

 public:
  PacketOut();
  PacketOut(uint32_t xid, uint32_t buffer_id, uint32_t in_port);
  bool operator==(const PacketOut &other) const;
  bool operator!=(const PacketOut &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t in_port() { return this->in_port_; }
  void in_port(uint32_t in_port) { this->in_port_ = in_port; }
};

/**
 OpenFlow 1.3 OFPT_PACKET_IN message.
 */
class PacketIn : public PacketInCommon {
 private:
  uint8_t table_id_;
  uint64_t cookie_;
  of13::Match match_;

 public:
  PacketIn();
  PacketIn(uint32_t xid, uint32_t buffer_id, uint16_t total_len, uint8_t reason,
           uint8_t table_id, uint64_t cookie);
  ~PacketIn(){};
  virtual uint16_t length();
  bool operator==(const PacketIn &other) const;
  bool operator!=(const PacketIn &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint8_t table_id() const { return this->table_id_; }
  uint64_t cookie() const { return this->cookie_; }
  of13::Match &match() { return this->match_; }
  const of13::Match &match() const { return this->match_; };
  OXMTLV *get_oxm_field(uint8_t field);
  void table_id(uint8_t table_id) { this->table_id_ = table_id; }
  void cookie(uint64_t cookie) { this->cookie_ = cookie; }
  void add_oxm_field(OXMTLV &field);
  void add_oxm_field(OXMTLV *field);
};

/**
 OpenFlow 1.3 OFPT_FLOW_MOD message.
 */
class FlowMod : public FlowModCommon {
 private:
  uint8_t command_;
  uint64_t cookie_mask_;
  uint8_t table_id_;
  uint32_t out_port_;
  uint32_t out_group_;
  of13::Match match_;
  InstructionSet instructions_;

 public:
  FlowMod();
  FlowMod(uint32_t xid, uint64_t cookie, uint64_t cookie_mask, uint8_t table_id,
          uint8_t command, uint16_t idle_timeout, uint16_t hard_timeout,
          uint16_t priority, uint32_t buffer_id, uint32_t out_port,
          uint32_t out_group, uint16_t flags);
  ~FlowMod() {}
  bool operator==(const FlowMod &other) const;
  bool operator!=(const FlowMod &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  virtual uint16_t length();
  uint8_t command() { return this->command_; }
  uint64_t cookie_mask() { return this->cookie_mask_; }
  uint8_t table_id() { return this->table_id_; }
  uint32_t out_port() { return this->out_port_; }
  uint32_t out_group() { return this->out_group_; }
  of13::Match match() { return this->match_; }
  of13::InstructionSet instructions() { return this->instructions_; }
  OXMTLV *get_oxm_field(uint8_t field);
  void command(uint8_t command) { this->command_ = command; }
  void cookie_mask(uint64_t cookie_mask) { this->cookie_mask_ = cookie_mask; }
  void table_id(uint8_t table_id) { this->table_id_ = table_id; }
  void out_port(uint32_t out_port) { this->out_port_ = out_port; }
  void out_group(uint32_t out_group) { this->out_group_ = out_group; }
  void match(of13::Match match);
  void add_oxm_field(OXMTLV &field);
  void add_oxm_field(OXMTLV *field);
  void instructions(InstructionSet instructions);
  void add_instruction(Instruction &inst);
  void add_instruction(Instruction *inst);
};

/**
 OpenFlow 1.3 OFPT_FLOW_REMOVED message.
 */
class FlowRemoved : public FlowRemovedCommon {
 private:
  uint8_t table_id_;
  uint16_t hard_timeout_;
  of13::Match match_;

 public:
  FlowRemoved();
  FlowRemoved(uint32_t xid, uint64_t cookie, uint16_t priority, uint8_t reason,
              uint8_t table_id, uint32_t duration_sec, uint32_t duration_nsec,
              uint16_t idle_timeout, uint16_t hard_timeout,
              uint64_t packet_count, uint64_t byte_count);
  FlowRemoved(uint32_t xid, uint64_t cookie, uint16_t priority, uint8_t reason,
              uint8_t table_id, uint32_t duration_sec, uint32_t duration_nsec,
              uint16_t idle_timeout, uint16_t hard_timeout,
              uint64_t packet_count, uint64_t byte_count, of13::Match);
  ~FlowRemoved() {}
  virtual uint16_t length();
  bool operator==(const FlowRemoved &other) const;
  bool operator!=(const FlowRemoved &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint8_t table_id() { return this->table_id_; }
  uint16_t hard_timeout() { return this->hard_timeout_; }
  void table_id(uint8_t table_id) { this->table_id_ = table_id; }
  void hard_timeout(uint16_t hard_timeout) {
    this->hard_timeout_ = hard_timeout;
  }
  of13::Match match() { return this->match_; }
  void match(of13::Match match) { this->match_ = match; }
};

/**
 OpenFlow 1.3 OFPT_PORT_STATUS message.
 */
class PortStatus : public PortStatusCommon {
 private:
  of13::Port desc_;

 public:
  PortStatus();
  PortStatus(uint32_t xid, uint8_t reason, of13::Port desc);
  ~PortStatus() {}
  bool operator==(const PortStatus &other) const;
  bool operator!=(const PortStatus &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  of13::Port desc() { return this->desc_; }
  void desc(of13::Port desc) { this->desc_ = desc; }
};

/**
 OpenFlow 1.3 OFPT_PORT_MOD message.
 */
class PortMod : public PortModCommon {
 private:
  uint32_t port_no_;

 public:
  PortMod();
  PortMod(uint32_t xid, uint32_t port_no, EthAddress hw_addr, uint32_t config,
          uint32_t mask, uint32_t advertise);
  ~PortMod() {}
  bool operator==(const PortMod &other) const;
  bool operator!=(const PortMod &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t port_no() { return this->port_no_; }
  void port_no(uint32_t port_no) { this->port_no_ = port_no; }
};

/**
 OpenFlow 1.3 OFPT_GROUP_MOD message.
 */
class GroupMod : public OFMsg {
 private:
  uint16_t command_;
  uint8_t group_type_;
  uint32_t group_id_;
  std::vector<Bucket> buckets_;

 public:
  GroupMod();
  GroupMod(uint32_t xid, uint16_t command, uint8_t type, uint32_t group_id);
  GroupMod(uint32_t xid, uint16_t command, uint8_t type, uint32_t group_id,
           std::vector<Bucket> buckets);
  ~GroupMod() {}
  bool operator==(const GroupMod &other) const;
  bool operator!=(const GroupMod &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t command() { return this->command_; }
  uint8_t type() { return this->type_; }
  uint32_t group_id() { return this->group_id_; }
  std::vector<Bucket> buckets() { return this->buckets_; }
  void command(uint16_t command) { this->command_ = command; }
  void group_type(uint8_t type) { this->group_type_ = type; }
  void group_id(uint32_t group_id) { this->group_id_ = group_id; }
  void buckets(std::vector<Bucket> buckets);
  void add_bucket(Bucket bucket);
  size_t buckets_len();
};

/**
 OpenFlow 1.3 OFPT_TABLE_MOD message.
 */
class TableMod : public OFMsg {
 private:
  uint8_t table_id_;
  uint32_t config_;

 public:
  TableMod();
  TableMod(uint32_t xid, uint8_t table_id, uint32_t config);
  ~TableMod() {}
  bool operator==(const TableMod &other) const;
  bool operator!=(const TableMod &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint8_t table_id() { return this->table_id_; }
  uint32_t config() { return this->config_; }
  void table_id(uint8_t table_id) { this->table_id_ = table_id; }
  void config(uint32_t config) { this->config_ = config; }
};

/**
 OpenFlow 1.3 OFPT_MULTIPART_REQUEST message header. Multipart request
 messages should inherit from this class.
 */
class MultipartRequest : public OFMsg {
 protected:
  uint16_t mpart_type_;
  uint16_t flags_;

 public:
  MultipartRequest();
  MultipartRequest(uint16_t type);
  MultipartRequest(uint32_t xid, uint16_t type, uint16_t flags);
  virtual ~MultipartRequest() {}
  bool operator==(const MultipartRequest &other) const;
  bool operator!=(const MultipartRequest &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t mpart_type() { return this->mpart_type_; }
  uint16_t flags() { return this->flags_; }
  void type(uint16_t type) { this->mpart_type_ = type; }
  void flags(uint16_t flags) { this->flags_ = flags; }
};

/**
 OpenFlow 1.3 OFPT_MULTIPART_REPLY message header. Multipart reply
 messages should inherit from this class.
 */
class MultipartReply : public OFMsg {
 protected:
  uint16_t mpart_type_;
  uint16_t flags_;

 public:
  MultipartReply();
  MultipartReply(uint16_t type);
  MultipartReply(uint32_t xid, uint16_t type, uint16_t flags);
  virtual ~MultipartReply() {}
  bool operator==(const MultipartReply &other) const;
  bool operator!=(const MultipartReply &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t mpart_type() { return this->mpart_type_; }
  uint16_t flags() { return this->flags_; }
  void type(uint16_t type) { this->mpart_type_ = type; }
  void flags(uint16_t flags) { this->flags_ = flags; }
};

/**
 OpenFlow 1.3 OFPMP_DESC multipart request.
 */
class MultipartRequestDesc : public MultipartRequest {
 public:
  MultipartRequestDesc();
  MultipartRequestDesc(uint32_t xid, uint16_t flags);
  ~MultipartRequestDesc() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.3 OFPMP_DESC multipart reply.
 */
class MultipartReplyDesc : public MultipartReply {
 private:
  SwitchDesc desc_;

 public:
  MultipartReplyDesc();
  MultipartReplyDesc(uint32_t xid, uint16_t flags, SwitchDesc desc);
  MultipartReplyDesc(uint32_t xid, uint16_t flags, std::string mfr_desc,
                     std::string hw_desc, std::string sw_desc,
                     std::string serial_num, std::string dp_desc);
  ~MultipartReplyDesc() {}
  bool operator==(const MultipartReplyDesc &other) const;
  bool operator!=(const MultipartReplyDesc &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  SwitchDesc desc() { return this->desc_; }
  void set_desc(SwitchDesc desc) { this->desc_ = desc; }
};

/**
 OpenFlow 1.3 OFPMP_FLOW multipart request.
 */
class MultipartRequestFlow : public MultipartRequest {
 private:
  uint8_t table_id_;
  uint32_t out_port_;
  uint32_t out_group_;
  uint64_t cookie_;
  uint64_t cookie_mask_;
  of13::Match match_;

 public:
  MultipartRequestFlow();
  MultipartRequestFlow(uint32_t xid, uint16_t flags, uint8_t table_id,
                       uint32_t out_port, uint32_t out_group, uint64_t cookie,
                       uint64_t cookie_mask);
  MultipartRequestFlow(uint32_t xid, uint16_t flags, uint8_t table_id,
                       uint32_t out_port, uint32_t out_group, uint64_t cookie,
                       uint64_t cookie_mask, of13::Match match);
  ~MultipartRequestFlow() {}
  bool operator==(const MultipartRequestFlow &other) const;
  bool operator!=(const MultipartRequestFlow &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  of13::Match match() { return this->match_; }
  uint8_t table_id() { return this->table_id_; }
  uint32_t out_port() { return this->out_port_; }
  uint32_t out_group() { return this->out_group_; }
  uint64_t cookie() { return this->cookie_; }
  uint64_t cookie_mask() { return this->cookie_mask_; }
  void match(of13::Match match);
  void add_oxm_field(OXMTLV &field);
  void add_oxm_field(OXMTLV *field);
  void table_id(uint8_t table_id) { this->table_id_ = table_id; }
  void out_port(uint16_t out_port) { this->out_port_ = out_port; }
  void out_group(uint32_t out_group) { this->out_group_ = out_group; }
  void cookie(uint64_t cookie) { this->cookie_ = cookie; }
  void cookie_mask(uint64_t cookie_mask) { this->cookie_mask_ = cookie_mask; }
};

/**
 OpenFlow 1.3 OFPMP_FLOW multipart reply.
 */
class MultipartReplyFlow : public MultipartReply {
 private:
  std::vector<of13::FlowStats> flow_stats_;

 public:
  MultipartReplyFlow();
  MultipartReplyFlow(uint32_t xid, uint16_t flags);
  MultipartReplyFlow(uint32_t xid, uint16_t flags,
                     std::vector<of13::FlowStats> flow_stats);
  ~MultipartReplyFlow() {}
  bool operator==(const MultipartReplyFlow &other) const;
  bool operator!=(const MultipartReplyFlow &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<of13::FlowStats> flow_stats() { return this->flow_stats_; }
  void flow_stats(std::vector<of13::FlowStats> flow_stats);
  void add_flow_stats(of13::FlowStats);
};

/**
 OpenFlow 1.3 OFPMP_AGGREGATE multipart request.
 */
class MultipartRequestAggregate : public MultipartRequest {
 private:
  uint8_t table_id_;
  uint32_t out_port_;
  uint32_t out_group_;
  uint64_t cookie_;
  uint64_t cookie_mask_;
  of13::Match match_;

 public:
  MultipartRequestAggregate();
  MultipartRequestAggregate(uint32_t xid, uint16_t flags, uint8_t table_id,
                            uint32_t out_port, uint32_t out_group,
                            uint64_t cookie, uint64_t cookie_mask);
  MultipartRequestAggregate(uint32_t xid, uint16_t flags, uint8_t table_id,
                            uint32_t out_port, uint32_t out_group,
                            uint64_t cookie, uint64_t cookie_mask,
                            of13::Match match);
  ~MultipartRequestAggregate() {}
  virtual uint16_t length();
  bool operator==(const MultipartRequestAggregate &other) const;
  bool operator!=(const MultipartRequestAggregate &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  of13::Match match() { return this->match_; }
  uint8_t table_id() { return this->table_id_; }
  uint32_t out_port() { return this->out_port_; }
  uint32_t out_group() { return this->out_group_; }
  uint64_t cookie() { return this->cookie_; }
  uint64_t cookie_mask() { return this->cookie_mask_; }
  void match(of13::Match match);
  void add_oxm_field(OXMTLV &field);
  void add_oxm_field(OXMTLV *field);
  void table_id(uint8_t table_id) { this->table_id_ = table_id; }
  void out_port(uint32_t out_port) { this->out_port_ = out_port; }
  void out_group(uint32_t out_group) { this->out_group_ = out_group; }
  void cookie(uint64_t cookie) { this->cookie_ = cookie; }
  void cookie_mask(uint64_t cookie_mask) { this->cookie_mask_ = cookie_mask; }
};

/**
 OpenFlow 1.3 OFPMP_AGGREGATE multipart reply.
 */
class MultipartReplyAggregate : public MultipartReply {
 private:
  uint64_t packet_count_;
  uint64_t byte_count_;
  uint32_t flow_count_;

 public:
  MultipartReplyAggregate();
  MultipartReplyAggregate(uint32_t xid, uint16_t flags, uint64_t packet_count,
                          uint64_t byte_count, uint32_t flow_count);
  ~MultipartReplyAggregate() {}
  bool operator==(const MultipartReplyAggregate &other) const;
  bool operator!=(const MultipartReplyAggregate &other) const;
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
 OpenFlow 1.3 OFPMP_TABLE multipart request.
 */
class MultipartRequestTable : public MultipartRequest {
 public:
  MultipartRequestTable();
  MultipartRequestTable(uint32_t xid, uint16_t flags);
  ~MultipartRequestTable(){};
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.3 OFPMP_TABLE multipart reply.
 */
class MultipartReplyTable : public MultipartReply {
 private:
  std::vector<of13::TableStats> table_stats_;

 public:
  MultipartReplyTable();
  MultipartReplyTable(uint32_t xid, uint16_t flags);
  MultipartReplyTable(uint32_t xid, uint16_t flags,
                      std::vector<of13::TableStats> table_stats);
  ~MultipartReplyTable() {}
  bool operator==(const MultipartReplyTable &other) const;
  bool operator!=(const MultipartReplyTable &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<of13::TableStats> table_stats() { return this->table_stats_; }
  void table_stats(std::vector<of13::TableStats> table_stats);
  void add_table_stat(of13::TableStats stat);
};

/**
 OpenFlow 1.3 OFPMP_PORT_STATS multipart request.
 */
class MultipartRequestPortStats : public MultipartRequest {
 private:
  uint32_t port_no_;

 public:
  MultipartRequestPortStats();
  MultipartRequestPortStats(uint32_t xid, uint16_t flags, uint32_t port_no);
  ~MultipartRequestPortStats() {}
  bool operator==(const MultipartRequestPortStats &other) const;
  bool operator!=(const MultipartRequestPortStats &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t port_no() { return this->port_no_; }
  void port_no(uint32_t port_no) { this->port_no_ = port_no; }
};

/**
 OpenFlow 1.3 OFPMP_PORT_STATS multipart reply.
 */
class MultipartReplyPortStats : public MultipartReply {
 private:
  std::vector<of13::PortStats> port_stats_;

 public:
  MultipartReplyPortStats();
  MultipartReplyPortStats(uint32_t xid, uint16_t flags);
  MultipartReplyPortStats(uint32_t xid, uint16_t flags,
                          std::vector<of13::PortStats> port_stats);
  ~MultipartReplyPortStats() {}
  bool operator==(const MultipartReplyPortStats &other) const;
  bool operator!=(const MultipartReplyPortStats &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<of13::PortStats> port_stats() { return this->port_stats_; }
  void port_stats(std::vector<of13::PortStats> port_stats);
  void add_port_stat(of13::PortStats stat);
};

/**
 OpenFlow 1.3 OFPMP_QUEUE multipart request.
 */
class MultipartRequestQueue : public MultipartRequest {
 private:
  uint32_t port_no_;
  uint32_t queue_id_;

 public:
  MultipartRequestQueue();
  MultipartRequestQueue(uint32_t xid, uint16_t flags, uint32_t port_no,
                        uint32_t queue_id);
  ~MultipartRequestQueue() {}
  bool operator==(const MultipartRequestQueue &other) const;
  bool operator!=(const MultipartRequestQueue &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t port_no() { return this->port_no_; }
  uint32_t queue_id() { return this->queue_id_; }
  void port_no(uint32_t port_no) { this->port_no_ = port_no; }
  void queue_id(uint32_t queue_id) { this->queue_id_ = queue_id; }
};

/**
 OpenFlow 1.3 OFPMP_QUEUE multipart reply.
 */
class MultipartReplyQueue : public MultipartReply {
 private:
  std::vector<of13::QueueStats> queue_stats_;

 public:
  MultipartReplyQueue();
  MultipartReplyQueue(uint32_t xid, uint16_t flags);
  MultipartReplyQueue(uint32_t xid, uint16_t flags,
                      std::vector<of13::QueueStats> queue_stats);
  ~MultipartReplyQueue() {}
  bool operator==(const MultipartReplyQueue &other) const;
  bool operator!=(const MultipartReplyQueue &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<of13::QueueStats> queue_stats() { return this->queue_stats_; }
  void queue_stats(std::vector<of13::QueueStats> queue_stats_);
  void add_queue_stat(of13::QueueStats stat);
};

/**
 OpenFlow 1.3 OFPMP_GROUP multipart request.
 */
class MultipartRequestGroup : public MultipartRequest {
 private:
  uint32_t group_id_;

 public:
  MultipartRequestGroup();
  MultipartRequestGroup(uint32_t xid, uint16_t flags, uint32_t group_id);
  ~MultipartRequestGroup() {}
  bool operator==(const MultipartRequestGroup &other) const;
  bool operator!=(const MultipartRequestGroup &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t group_id() { return this->group_id_; }
  void group_id(uint32_t group_id) { this->group_id_ = group_id; }
};

/**
 OpenFlow 1.3 OFPMP_GROUP multipart reply.
 */
class MultipartReplyGroup : public MultipartReply {
 private:
  std::vector<of13::GroupStats> group_stats_;

 public:
  MultipartReplyGroup();
  MultipartReplyGroup(uint32_t xid, uint16_t flags);
  MultipartReplyGroup(uint32_t xid, uint16_t flags,
                      std::vector<of13::GroupStats> group_stats);
  ~MultipartReplyGroup() {}
  bool operator==(const MultipartReplyGroup &other) const;
  bool operator!=(const MultipartReplyGroup &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<of13::GroupStats> queue_stats() { return this->group_stats_; }
  void group_stats(std::vector<of13::GroupStats> group_stats);
  void add_group_stats(of13::GroupStats stat);
  size_t group_stats_len();
};

/**
 OpenFlow 1.3 OFPMP_GROUP_DESC multipart request.
 */
class MultipartRequestGroupDesc : public MultipartRequest {
 public:
  MultipartRequestGroupDesc();
  MultipartRequestGroupDesc(uint32_t xid, uint16_t flags);
  ~MultipartRequestGroupDesc() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.3 OFPMP_GROUP_DESC multipart reply.
 */
class MultipartReplyGroupDesc : public MultipartReply {
 private:
  std::vector<of13::GroupDesc> group_desc_;

 public:
  MultipartReplyGroupDesc();
  MultipartReplyGroupDesc(uint32_t xid, uint16_t flags);
  MultipartReplyGroupDesc(uint32_t xid, uint16_t flags,
                          std::vector<of13::GroupDesc> group_desc);
  ~MultipartReplyGroupDesc() {}
  bool operator==(const MultipartReplyGroupDesc &other) const;
  bool operator!=(const MultipartReplyGroupDesc &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<of13::GroupDesc> queue_stats() { return this->group_desc_; }
  void group_desc(std::vector<of13::GroupDesc> group_desc);
  void add_group_desc(of13::GroupDesc stat);
  size_t desc_len();
};

/**
 OpenFlow 1.3 OFPMP_GROUP_FEATURES multipart request.
 */
class MultipartRequestGroupFeatures : public MultipartRequest {
 public:
  MultipartRequestGroupFeatures();
  MultipartRequestGroupFeatures(uint32_t xid, uint16_t flags);
  ~MultipartRequestGroupFeatures() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.3 OFPMP_GROUP_FEATURES multipart reply.
 */
class MultipartReplyGroupFeatures : public MultipartReply {
 private:
  of13::GroupFeatures features_;

 public:
  MultipartReplyGroupFeatures();
  MultipartReplyGroupFeatures(uint32_t xid, uint16_t flags,
                              of13::GroupFeatures features);
  ~MultipartReplyGroupFeatures() {}
  bool operator==(const MultipartReplyGroupFeatures &other) const;
  bool operator!=(const MultipartReplyGroupFeatures &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  of13::GroupFeatures features() { return this->features_; }
  void features(of13::GroupFeatures features) { this->features_ = features; }
};

/**
 OpenFlow 1.3 OFPMP_METER multipart request.
 */
class MultipartRequestMeter : public MultipartRequest {
 private:
  uint32_t meter_id_;

 public:
  MultipartRequestMeter();
  MultipartRequestMeter(uint32_t xid, uint16_t flags, uint32_t meter_id);
  bool operator==(const MultipartRequestMeter &other) const;
  bool operator!=(const MultipartRequestMeter &other) const;
  ~MultipartRequestMeter() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t meter_id() { return this->meter_id_; }
  void meter_id(uint32_t meter_id) { this->meter_id_ = meter_id; }
};

/**
 OpenFlow 1.3 OFPMP_METER multipart reply.
 */
class MultipartReplyMeter : public MultipartReply {
 private:
  std::vector<of13::MeterStats> meter_stats_;

 public:
  MultipartReplyMeter();
  MultipartReplyMeter(uint32_t xid, uint16_t flags);
  MultipartReplyMeter(uint32_t xid, uint16_t flags,
                      std::vector<of13::MeterStats> meter_stats);
  ~MultipartReplyMeter() {}
  bool operator==(const MultipartReplyMeter &other) const;
  bool operator!=(const MultipartReplyMeter &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<of13::MeterStats> meter_stats() { return this->meter_stats_; }
  void meter_stats(std::vector<of13::MeterStats> meter_stats);
  void add_meter_stats(MeterStats stats);
  size_t meter_stats_len();
};

/**
 OpenFlow 1.3 OFPMP_METER_CONFIG multipart request.
 */
class MultipartRequestMeterConfig : public MultipartRequest {
 private:
  uint32_t meter_id_;

 public:
  MultipartRequestMeterConfig();
  MultipartRequestMeterConfig(uint32_t xid, uint16_t flags, uint32_t meter_id);
  bool operator==(const MultipartRequestMeterConfig &other) const;
  bool operator!=(const MultipartRequestMeterConfig &other) const;
  ~MultipartRequestMeterConfig() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t meter_id() { return this->meter_id_; }
  void meter_id(uint32_t meter_id) { this->meter_id_ = meter_id; }
};

/**
 OpenFlow 1.3 OFPMP_METER_CONFIG multipart reply.
 */
class MultipartReplyMeterConfig : public MultipartReply {
  std::vector<MeterConfig> meter_config_;

 public:
  MultipartReplyMeterConfig();
  MultipartReplyMeterConfig(uint32_t xid, uint16_t flags);
  MultipartReplyMeterConfig(uint32_t xid, uint16_t flags,
                            std::vector<MeterConfig> meter_config);
  ~MultipartReplyMeterConfig() {}
  bool operator==(const MultipartReplyMeterConfig &other) const;
  bool operator!=(const MultipartReplyMeterConfig &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<MeterConfig> meter_config() { return this->meter_config_; }
  void meter_config(std::vector<MeterConfig> meter_config);
  void add_meter_config(MeterConfig config);
  size_t meter_config_len();
};

/**
 OpenFlow 1.3 OFPMP_METER_FEATURES multipart request.
 */
class MultipartRequestMeterFeatures : public MultipartRequest {
 public:
  MultipartRequestMeterFeatures();
  MultipartRequestMeterFeatures(uint32_t xid, uint16_t flags);
  ~MultipartRequestMeterFeatures() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.3 OFPMP_METER_FEATURES multipart reply.
 */
class MultipartReplyMeterFeatures : public MultipartReply {
 private:
  MeterFeatures meter_features_;

 public:
  MultipartReplyMeterFeatures();
  MultipartReplyMeterFeatures(uint32_t xid, uint16_t flags,
                              MeterFeatures features);
  ~MultipartReplyMeterFeatures() {}
  bool operator==(const MultipartReplyMeterFeatures &other) const;
  bool operator!=(const MultipartReplyMeterFeatures &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  MeterFeatures meter_features() { return this->meter_features_; }
  void meter_features(MeterFeatures meter_features) {
    this->meter_features_ = meter_features;
  }
};

/**
 OpenFlow 1.3 OFPMP_TABLE_FEATURES multipart request.
 */
class MultipartRequestTableFeatures : public MultipartRequest {
 private:
  std::vector<TableFeatures> tables_features_;

 public:
  MultipartRequestTableFeatures();
  MultipartRequestTableFeatures(uint32_t xid, uint16_t flags);
  MultipartRequestTableFeatures(uint32_t xid, uint16_t flags,
                                std::vector<TableFeatures> table_features);
  ~MultipartRequestTableFeatures() {}
  bool operator==(const MultipartRequestTableFeatures &other) const;
  bool operator!=(const MultipartRequestTableFeatures &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<TableFeatures> tables_features() {
    return this->tables_features_;
  }
  void tables_features(std::vector<TableFeatures> tables_features);
  void add_table_features(TableFeatures table_feature);
};

/**
 OpenFlow 1.3 OFPMP_TABLE_FEATURES multipart reply.
 */
class MultipartReplyTableFeatures : public MultipartReply {
 private:
  std::vector<TableFeatures> tables_features_;

 public:
  MultipartReplyTableFeatures();
  MultipartReplyTableFeatures(uint32_t xid, uint16_t flags);
  MultipartReplyTableFeatures(uint32_t xid, uint16_t flags,
                              std::vector<TableFeatures> table_features);
  ~MultipartReplyTableFeatures() {}
  bool operator==(const MultipartReplyTableFeatures &other) const;
  bool operator!=(const MultipartReplyTableFeatures &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<TableFeatures> table_features() { return this->tables_features_; }
  void tables_features(std::vector<TableFeatures> tables_features);
  void add_table_features(TableFeatures table_feature);
};

/**
 OpenFlow 1.3 OFPMP_PORT_DESC multipart request.
 */
class MultipartRequestPortDescription : public MultipartRequest {
 public:
  MultipartRequestPortDescription();
  MultipartRequestPortDescription(uint32_t xid, uint16_t flags);
  ~MultipartRequestPortDescription() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.3 OFPMP_PORT_DESC multipart reply.
 */
class MultipartReplyPortDescription : public MultipartReply {
 private:
  std::vector<of13::Port> ports_;

 public:
  MultipartReplyPortDescription();
  MultipartReplyPortDescription(uint32_t xid, uint16_t flags);
  MultipartReplyPortDescription(uint32_t xid, uint16_t flags,
                                std::vector<of13::Port> ports);
  ~MultipartReplyPortDescription() {}
  bool operator==(const MultipartReplyPortDescription &other) const;
  bool operator!=(const MultipartReplyPortDescription &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  std::vector<of13::Port> ports() { return this->ports_; }
  void ports(std::vector<of13::Port> ports);
  void add_port(of13::Port);
};

/**
 OpenFlow 1.3 OFPMP_EXPERIMENTER multipart request.
 Multipart request experimenter messages should inherit from this class.
 */
class MultipartRequestExperimenter : public MultipartRequest {
 protected:
  uint32_t experimenter_;
  uint32_t exp_type_;

 public:
  MultipartRequestExperimenter();
  MultipartRequestExperimenter(uint32_t xid, uint16_t flags,
                               uint32_t experimenter, uint32_t exp_type);
  virtual ~MultipartRequestExperimenter() {}
  bool operator==(const MultipartRequestExperimenter &other) const;
  bool operator!=(const MultipartRequestExperimenter &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t experimenter() { return this->experimenter_; }
  uint32_t exp_type() { return this->exp_type_; }
};

/**
 OpenFlow 1.3 OFPMP_EXPERIMENTER multipart reply.
 Multipart reply experimenter messages should inherit from this class.
 */
class MultipartReplyExperimenter : public MultipartReply {
 protected:
  uint32_t experimenter_;
  uint32_t exp_type_;

 public:
  MultipartReplyExperimenter();
  MultipartReplyExperimenter(uint32_t xid, uint16_t flags,
                             uint32_t experimenter, uint32_t exp_type);
  virtual ~MultipartReplyExperimenter() {}
  bool operator==(const MultipartReplyExperimenter &other) const;
  bool operator!=(const MultipartReplyExperimenter &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t experimenter() { return this->experimenter_; }
  uint32_t exp_type() { return this->exp_type_; }
};

/**
 OpenFlow 1.3 OFPT_BARRIER_REQUEST message.
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
 OpenFlow 1.3 OFPT_BARRIER_REPLY message*/
class BarrierReply : public OFMsg {
 public:
  BarrierReply();
  BarrierReply(uint32_t xid);
  ~BarrierReply() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.3 OFPT_QUEUE_GET_CONFIG_REQUEST message.
 */
class QueueGetConfigRequest : public OFMsg {
 private:
  uint32_t port_;

 public:
  QueueGetConfigRequest();
  QueueGetConfigRequest(uint32_t xid, uint32_t port);
  ~QueueGetConfigRequest() {}
  bool operator==(const QueueGetConfigRequest &other) const;
  bool operator!=(const QueueGetConfigRequest &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t port() { return this->port_; }
  void port(uint32_t port) { this->port_ = port; }
};

/**
 OpenFlow 1.3 OFPT_QUEUE_GET_CONFIG_REPLY message.
 */
class QueueGetConfigReply : public OFMsg {
 private:
  uint32_t port_;
  std::list<of13::PacketQueue> queues_;

 public:
  QueueGetConfigReply();
  QueueGetConfigReply(uint32_t xid, uint32_t port);
  QueueGetConfigReply(uint32_t xid, uint16_t port,
                      std::list<of13::PacketQueue> queues);
  ~QueueGetConfigReply() {}
  bool operator==(const QueueGetConfigReply &other) const;
  bool operator!=(const QueueGetConfigReply &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint32_t port() { return this->port_; }
  std::list<of13::PacketQueue> queues() { return this->queues_; }
  void port(uint32_t port) { this->port_ = port; }
  void queues(std::list<of13::PacketQueue> queues);
  void add_queue(PacketQueue queue);
  size_t queues_len();
};

/**
 OpenFlow 1.3 OFPT_ROLE_REQUEST message.
 */
class RoleRequest : public RoleCommon {
 public:
  RoleRequest();
  RoleRequest(uint32_t xid, uint32_t role, uint64_t generation_id);
  ~RoleRequest() {}
};

/**
 OpenFlow 1.3 OFPT_ROLE_REPLY message.
 */
class RoleReply : public RoleCommon {
 public:
  RoleReply();
  RoleReply(uint32_t xid, uint32_t role, uint64_t generation_id);
  ~RoleReply() {}
};

/**
 OpenFlow 1.3 OFPT_GET_ASYNC_REQUEST message.
 */
class GetAsyncRequest : public OFMsg {
 public:
  GetAsyncRequest();
  GetAsyncRequest(uint32_t xid);
  ~GetAsyncRequest() {}
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
};

/**
 OpenFlow 1.3 OFPT_GET_ASYNC_REPLY message.
 */
class GetAsyncReply : public AsyncConfigCommon {
 public:
  GetAsyncReply();
  GetAsyncReply(uint32_t xid);
  GetAsyncReply(uint32_t xid, std::vector<uint32_t> packet_in_mask,
                std::vector<uint32_t> port_status_mask,
                std::vector<uint32_t> flow_removed_mask);
  ~GetAsyncReply() {}
};

/**
 OpenFlow 1.3 OFPT_SET_ASYNC message.
 */
class SetAsync : public AsyncConfigCommon {
 public:
  SetAsync();
  SetAsync(uint32_t xid);
  SetAsync(uint32_t xid, std::vector<uint32_t> packet_in_mask,
           std::vector<uint32_t> port_status_mask,
           std::vector<uint32_t> flow_removed_mask);
  ~SetAsync() {}
};

class MeterMod : public OFMsg {
 private:
  uint16_t command_;
  uint16_t flags_;
  uint32_t meter_id_;
  MeterBandList bands_;

 public:
  MeterMod();
  MeterMod(uint32_t xid, uint16_t command, uint16_t flags, uint32_t meter_id);
  MeterMod(uint32_t xid, uint16_t command, uint16_t flags, uint32_t meter_id,
           MeterBandList bands);
  ~MeterMod() {}
  bool operator==(const MeterMod &other) const;
  bool operator!=(const MeterMod &other) const;
  uint8_t *pack();
  of_error unpack(uint8_t *buffer);
  uint16_t command() { return this->command_; }
  uint16_t flags() { return this->flags_; }
  uint32_t meter_id() { return this->meter_id_; }
  MeterBandList bands() { return this->bands_; }
  void command(uint16_t command) { this->command_ = command; }
  void flags(uint16_t flags) { this->flags_ = flags; }
  void meter_id(uint32_t meter_id) { this->meter_id_ = meter_id; }
  void bands(MeterBandList bands);
  void add_band(MeterBand *band);
};

}  // end of namespace of13
}  // End of namespace fluid_msg
#endif

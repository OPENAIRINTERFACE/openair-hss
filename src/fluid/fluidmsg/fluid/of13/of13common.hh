#ifndef OF13OPENFLOW_COMMON_H
#define OF13OPENFLOW_COMMON_H 1

#include <string>
#include <vector>

#include "fluid/ofcommon/common.hh"
#include "fluid/util/util.h"
#include "of13action.hh"
#include "of13instruction.hh"
#include "openflow-13.h"

namespace fluid_msg {

namespace of13 {

class HelloElem {
 protected:
  uint16_t type_;
  uint16_t length_;

 public:
  HelloElem() {}
  HelloElem(uint16_t type, uint16_t length);
  ~HelloElem() {}
  bool operator==(const HelloElem& other) const;
  bool operator!=(const HelloElem& other) const;
  uint16_t type() { return this->type_; }
  uint16_t length() { return this->length_; }
  void type(uint16_t type) { this->type_ = type; }
  void length(uint16_t length) { this->length_ = length; }
};

class HelloElemVersionBitmap : public HelloElem {
 private:
  std::list<uint32_t> bitmaps_;

 public:
  HelloElemVersionBitmap()
      : HelloElem(of13::OFPHET_VERSIONBITMAP,
                  sizeof(struct of13::ofp_hello_elem_versionbitmap)) {}
  HelloElemVersionBitmap(std::list<uint32_t> bitmap);
  bool operator==(const HelloElemVersionBitmap& other) const;
  bool operator!=(const HelloElemVersionBitmap& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* data);
  std::list<uint32_t> bitmaps() { return this->bitmaps_; }
  void bitmaps(std::list<uint32_t> bitmaps) { this->bitmaps_ = bitmaps; }
  void add_bitmap(uint32_t bitmap);
};

class Port : public PortCommon {
 private:
  uint32_t port_no_;
  uint32_t curr_speed_;
  uint32_t max_speed_;

 public:
  Port();
  Port(uint32_t port_no, EthAddress hw_addr, std::string name, uint32_t config,
       uint32_t state, uint32_t curr, uint32_t advertised, uint32_t supported,
       uint32_t peer, uint32_t curr_speed, uint32_t max_speed);
  ~Port() {}
  bool operator==(const Port& other) const;
  bool operator!=(const Port& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint32_t port_no() { return this->port_no_; }
  uint32_t curr_speed() { return this->curr_speed_; }
  uint32_t max_speed() { return this->max_speed_; }
  void port_no(uint32_t port_no) { this->port_no_ = port_no; }
  void curr_speed(uint32_t curr_speed) { this->curr_speed_ = curr_speed; }
  void max_speed(uint32_t max_speed) { this->max_speed_ = max_speed; }
};

class QueuePropMinRate : public QueuePropRate {
 public:
  QueuePropMinRate() : QueuePropRate(of13::OFPQT_MIN_RATE) {}
  QueuePropMinRate(uint16_t rate);
  ~QueuePropMinRate() {}
  virtual bool equals(const QueueProperty& other);
  virtual QueuePropMinRate* clone() { return new QueuePropMinRate(*this); }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
};

class QueuePropMaxRate : public QueuePropRate {
 public:
  QueuePropMaxRate() : QueuePropRate(of13::OFPQT_MAX_RATE) {}
  QueuePropMaxRate(uint16_t rate);
  ~QueuePropMaxRate() {}
  virtual bool equals(const QueueProperty& other);
  virtual QueuePropMaxRate* clone() { return new QueuePropMaxRate(*this); }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
};

class QueueExperimenter : public QueueProperty {
 protected:
  uint32_t experimenter_;

 public:
  QueueExperimenter() {}
  QueueExperimenter(uint32_t experimenter);
  ~QueueExperimenter() {}
  virtual QueueExperimenter* clone() { return new QueueExperimenter(*this); }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint32_t get_experimenter() { return this->experimenter_; }
  void set_experimenter(uint32_t experimenter) {
    this->experimenter_ = experimenter;
  }
};

/* Queue description*/
class PacketQueue : public PacketQueueCommon {
 private:
  uint32_t port_;

 public:
  PacketQueue();
  PacketQueue(uint32_t queue_id, uint32_t port);
  PacketQueue(uint32_t queue_id, uint32_t port, QueuePropertyList properties);
  ~PacketQueue() {}
  bool operator==(const PacketQueue& other) const;
  bool operator!=(const PacketQueue& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint32_t port() { return this->port_; }
  void port(uint32_t port) { this->port_ = port; }
};

class Bucket {
 private:
  uint16_t length_;
  uint16_t weight_;
  uint32_t watch_port_;
  uint32_t watch_group_;
  ActionSet actions_;

 public:
  Bucket();
  Bucket(uint16_t weight, uint32_t watch_port, uint32_t watch_group);
  Bucket(uint16_t weight, uint32_t watch_port, uint32_t watch_group,
         ActionSet actions);
  ~Bucket() {}
  bool operator==(const Bucket& other) const;
  bool operator!=(const Bucket& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint16_t len() { return this->length_; }
  uint16_t weight() { return this->weight_; }
  uint32_t watch_port() { return this->watch_port_; }
  uint32_t watch_group() { return this->watch_group_; }
  ActionSet get_actions() { return this->actions_; }
  void weight(uint16_t weight) { this->weight_ = weight; }
  void watch_port(uint32_t watch_port) { this->watch_port_ = watch_port; }
  void watch_group(uint32_t watch_group) { this->watch_group_ = watch_group; }
  void actions(ActionSet actions);
  void add_action(Action& action);
  void add_action(Action* action);
};

class FlowStats : public FlowStatsCommon {
 private:
  uint16_t flags_;
  of13::Match match_;
  InstructionSet instructions_;

 public:
  FlowStats();
  FlowStats(uint8_t table_id, uint32_t duration_sec, uint32_t duration_nsec,
            uint16_t priority, uint16_t idle_timeout, uint16_t hard_timeout,
            uint16_t flags, uint64_t cookie, uint64_t packet_count,
            uint64_t byte_count);
  ~FlowStats() {}
  bool operator==(const FlowStats& other) const;
  bool operator!=(const FlowStats& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint16_t get_flags() { return this->flags_; }
  void set_flags(uint16_t flags) { this->flags_ = flags; }
  void match(of13::Match match);
  of13::Match match() { return this->match_; }
  OXMTLV* get_oxm_field(uint8_t field);
  void instructions(InstructionSet instructions);
  void add_instruction(Instruction* inst);
};

class TableStats : public TableStatsCommon {
 public:
  TableStats();
  TableStats(uint8_t table_id, uint32_t active_count, uint64_t lookup_count,
             uint64_t matched_count);
  ~TableStats() {}
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
};

class PortStats : public PortStatsCommon {
 private:
  uint32_t port_no_;
  uint32_t duration_sec_;
  uint32_t duration_nsec_;

 public:
  PortStats();
  PortStats(uint32_t port_no, struct port_rx_tx_stats tx_stats,
            struct port_err_stats err_stats, uint64_t collisions,
            uint32_t duration_sec, uint32_t duration_nsec);
  ~PortStats() {}
  bool operator==(const PortStats& other) const;
  bool operator!=(const PortStats& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint32_t port_no() { return this->port_no_; }
  uint32_t duration_sec() { return this->duration_sec_; }
  uint32_t duration_nsec() { return this->duration_nsec_; }
  void port_no(uint32_t port_no) { this->port_no_ = port_no; }
  void duration_sec(uint32_t duration_sec) {
    this->duration_sec_ = duration_sec;
  }
  void duration_nsec(uint32_t duration_nsec) {
    this->duration_nsec_ = duration_nsec;
  }
};

class QueueStats : public QueueStatsCommon {
 private:
  uint32_t port_no_;
  uint32_t duration_sec_;
  uint32_t duration_nsec_;

 public:
  QueueStats();
  QueueStats(uint32_t port_no, uint32_t queue_id, uint64_t tx_bytes,
             uint64_t tx_packets, uint64_t tx_errors, uint32_t duration_sec,
             uint32_t duration_nsec);
  ~QueueStats() {}
  bool operator==(const QueueStats& other) const;
  bool operator!=(const QueueStats& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint32_t port_no() { return this->port_no_; }
  uint32_t duration_sec() { return this->duration_sec_; }
  uint32_t duration_nsec() { return this->duration_nsec_; }
  void port_no(uint32_t port_no) { this->port_no_ = port_no; }
  void duration_sec(uint32_t duration_sec) {
    this->duration_sec_ = duration_sec;
  }
  void duration_nsec(uint32_t duration_nsec) {
    this->duration_nsec_ = duration_nsec;
  }
};

class BucketStats {
 private:
  uint64_t packet_count_;
  uint64_t byte_count_;

 public:
  BucketStats();
  BucketStats(uint64_t packet_count, uint64_t byte_count);
  ~BucketStats() {}
  bool operator==(const BucketStats& other) const;
  bool operator!=(const BucketStats& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint64_t packet_count() { return this->packet_count_; }
  uint64_t byte_count() { return this->byte_count_; }
};

class GroupStats {
 private:
  uint16_t length_;
  uint32_t group_id_;
  uint32_t ref_count_;
  uint64_t packet_count_;
  uint64_t byte_count_;
  uint32_t duration_sec_;
  uint32_t duration_nsec_;
  std::vector<BucketStats> bucket_stats_;

 public:
  GroupStats() : length_(sizeof(struct of13::ofp_group_stats)) {}
  GroupStats(uint32_t group_id, uint32_t ref_count, uint64_t packet_count,
             uint64_t byte_count, uint32_t duration_sec,
             uint32_t duration_nsec);
  GroupStats(uint32_t group_id, uint32_t ref_count, uint64_t packet_count,
             uint64_t byte_count, uint32_t duration_sec, uint32_t duration_nsec,
             std::vector<BucketStats> bucket_stats);
  ~GroupStats() {}
  bool operator==(const GroupStats& other) const;
  bool operator!=(const GroupStats& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint16_t length() { return this->length_; }
  uint32_t group_id() { return this->group_id_; }
  uint32_t ref_count() { return this->ref_count_; }
  uint64_t packet_count() { return this->packet_count_; }
  uint64_t byte_count() { return this->byte_count_; }
  uint32_t duration_sec() { return this->duration_sec_; }
  uint32_t duration_nsec() { return this->duration_nsec_; }
  void group_id(uint32_t group_id) { this->group_id_ = group_id; }
  void ref_count(uint32_t ref_count) { this->ref_count_ = ref_count; }
  void packet_count(uint64_t packet_count) {
    this->packet_count_ = packet_count;
  }
  void byte_count(uint64_t byte_count) { this->byte_count_ = byte_count; }
  void duration_sec(uint32_t duration_sec) {
    this->duration_sec_ = duration_sec;
  }
  void duration_nsec(uint64_t duration_nsec) {
    this->duration_nsec_ = duration_nsec;
  }
  void bucket_stats(std::vector<BucketStats> bucket_stats);
  void add_bucket_stat(BucketStats stat);
};

class GroupDesc {
 private:
  uint16_t length_;
  uint8_t type_;
  uint32_t group_id_;
  std::vector<of13::Bucket> buckets_;

 public:
  GroupDesc() : length_(sizeof(struct of13::ofp_group_desc_stats)) {}
  GroupDesc(uint8_t type, uint32_t group_id);
  GroupDesc(uint8_t type, uint32_t group_id, std::vector<of13::Bucket> buckets);
  ~GroupDesc() {}
  bool operator==(const GroupDesc& other) const;
  bool operator!=(const GroupDesc& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint16_t length() { return this->length_; }
  uint8_t type() { return this->type_; }
  uint32_t group_id() { return this->group_id_; }
  std::vector<of13::Bucket> buckets() { return this->buckets_; }
  void type(uint8_t type) { this->type_ = type; }
  void group_id(uint32_t group_id) { this->group_id_ = group_id; }
  void buckets(std::vector<Bucket> buckets);
  void add_bucket(Bucket bucket);
  size_t buckets_len();
};

class GroupFeatures {
 private:
  uint32_t types_;
  uint32_t capabilities_;
  uint32_t max_groups_[4];
  uint32_t actions_[4];

 public:
  GroupFeatures() {}
  GroupFeatures(uint32_t types, uint32_t capabilities, uint32_t max_groups[4],
                uint32_t actions[4]);
  ~GroupFeatures() {}
  bool operator==(const GroupFeatures& other) const;
  bool operator!=(const GroupFeatures& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint32_t types() { return this->types_; }
  uint32_t capabilities() { return this->capabilities_; }
  uint32_t* max_groups() { return this->max_groups_; }
  uint32_t* actions() { return this->actions_; }
  void types(uint32_t types) { this->types_ = types; }
  void capabilities(uint32_t capabilities) {
    this->capabilities_ = capabilities;
  }
  void max_groups(uint32_t max_groups[4]) {
    memcpy(this->max_groups_, max_groups, 16);
  }
  void actions(uint32_t actions[4]) { memcpy(this->actions_, actions, 16); }
};

class TableFeatureProp {
 protected:
  uint16_t type_;
  uint16_t length_;
  uint8_t padding_;

 public:
  TableFeatureProp()
      : length_(sizeof(struct ofp_table_feature_prop_header)), padding_(4) {}
  TableFeatureProp(uint16_t type);
  virtual ~TableFeatureProp() {}
  virtual bool equals(const TableFeatureProp& other);
  virtual bool operator==(const TableFeatureProp& other) const;
  virtual bool operator!=(const TableFeatureProp& other) const;
  virtual TableFeatureProp* clone() { return new TableFeatureProp(*this); }
  virtual size_t pack(uint8_t* buffer);
  virtual of_error unpack(uint8_t* buffer);
  uint16_t type() { return this->type_; }
  uint16_t length() { return this->length_; }
  uint8_t padding() { return this->padding_; }
  void type(uint16_t type) { this->type_ = type; }
  void length(uint16_t length) { this->length_ = length; }
  static bool delete_all(TableFeatureProp* prop) {
    delete prop;
    return true;
  }
};

class TableFeaturePropInstruction : public TableFeatureProp {
 private:
  std::vector<Instruction> instruction_ids_;

 public:
  TableFeaturePropInstruction() : TableFeatureProp() {}
  TableFeaturePropInstruction(uint16_t type) : TableFeatureProp(type) {}
  TableFeaturePropInstruction(uint16_t type,
                              std::vector<Instruction> instruction_ids);
  ~TableFeaturePropInstruction() {}
  virtual bool equals(const TableFeatureProp& other);
  virtual TableFeaturePropInstruction* clone() {
    return new TableFeaturePropInstruction(*this);
  }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  std::vector<Instruction> instruction_ids() { return this->instruction_ids_; }
  void instruction_ids(std::vector<Instruction> instruction_ids);
};

class TableFeaturePropNextTables : public TableFeatureProp {
 private:
  std::vector<uint8_t> next_table_ids_;

 public:
  TableFeaturePropNextTables() : TableFeatureProp() {}
  TableFeaturePropNextTables(uint16_t type) : TableFeatureProp(type) {}
  TableFeaturePropNextTables(uint16_t type,
                             std::vector<uint8_t> next_table_ids);
  ~TableFeaturePropNextTables() {}
  virtual bool equals(const TableFeatureProp& other);
  virtual TableFeaturePropNextTables* clone() {
    return new TableFeaturePropNextTables(*this);
  }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  std::vector<uint8_t> next_table_ids() { return this->next_table_ids_; }
  void table_ids(std::vector<uint8_t> table_ids);
};

class TableFeaturePropActions : public TableFeatureProp {
 private:
  std::vector<Action> action_ids_;

 public:
  TableFeaturePropActions() : TableFeatureProp() {}
  TableFeaturePropActions(uint16_t type) : TableFeatureProp(type) {}
  TableFeaturePropActions(uint16_t type, std::vector<Action> action_ids);
  ~TableFeaturePropActions() {}
  virtual bool equals(const TableFeatureProp& other);
  virtual TableFeaturePropActions* clone() {
    return new TableFeaturePropActions(*this);
  }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  std::vector<Action> action_ids() { return this->action_ids_; }
  void action_ids(std::vector<Action> action_ids);
};

class TableFeaturePropOXM : public TableFeatureProp {
 private:
  std::vector<uint32_t> oxm_ids_;

 public:
  TableFeaturePropOXM() : TableFeatureProp() {}
  TableFeaturePropOXM(uint16_t type) : TableFeatureProp(type) {}
  TableFeaturePropOXM(uint16_t type, std::vector<uint32_t> oxm_ids);
  ~TableFeaturePropOXM() {}
  virtual bool equals(const TableFeatureProp& other);
  virtual TableFeaturePropOXM* clone() {
    return new TableFeaturePropOXM(*this);
  }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  void oxm_ids(std::vector<uint32_t> oxm_ids);
};

class TableFeaturePropExperimenter : public TableFeatureProp {
 protected:
  uint32_t experimenter_;
  uint32_t exp_type_;

 public:
  TableFeaturePropExperimenter() : TableFeatureProp() {}
  TableFeaturePropExperimenter(uint16_t type) : TableFeatureProp(type) {}
  TableFeaturePropExperimenter(uint16_t type, uint32_t experimenter,
                               uint32_t exp_type);
  ~TableFeaturePropExperimenter() {}
  virtual bool equals(const TableFeatureProp& other);
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
};

class TablePropertiesList {
 private:
  uint16_t length_;
  std::list<TableFeatureProp*> property_list_;

 public:
  TablePropertiesList() : length_(0) {}
  TablePropertiesList(std::list<TableFeatureProp*> property_list);
  TablePropertiesList(const TablePropertiesList& other);
  TablePropertiesList& operator=(TablePropertiesList other);
  ~TablePropertiesList();
  bool operator==(const TablePropertiesList& other) const;
  bool operator!=(const TablePropertiesList& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  friend void swap(TablePropertiesList& first, TablePropertiesList& second);
  uint16_t length() { return this->length_; }
  std::list<TableFeatureProp*> property_list() { return this->property_list_; }
  void property_list(std::list<TableFeatureProp*> property_list);
  void length(uint16_t length) { this->length_ = length; }
  void add_property(TableFeatureProp* prop);
};

class TableFeatures {
 private:
  uint16_t length_;
  uint8_t table_id_;
  std::string name_;
  uint64_t metadata_match_;
  uint64_t metadata_write_;
  uint32_t config_;
  uint32_t max_entries_;
  TablePropertiesList properties_;

 public:
  TableFeatures() : length_(sizeof(struct of13::ofp_table_features)) {}
  TableFeatures(uint8_t table_id, std::string name, uint64_t metadata_match,
                uint64_t metadata_write, uint32_t config, uint32_t max_entries);
  ~TableFeatures() {}
  bool operator==(const TableFeatures& other) const;
  bool operator!=(const TableFeatures& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint16_t length();
  uint8_t table_id() { return this->table_id_; }
  std::string name() { return this->name_; }
  uint64_t metadata_match() { return this->metadata_match_; }
  uint64_t metadata_write() { return this->metadata_write_; }
  uint32_t config() { return this->config_; }
  uint32_t max_entries() { return this->max_entries_; }
  TablePropertiesList properties() { return this->properties_; }
  void table_id(uint8_t table_id) { this->table_id_ = table_id; }
  void name(std::string name) { this->name_ = name; }
  void metadata_match(uint64_t metadata_match) {
    this->metadata_match_ = metadata_match;
  }
  void properties(TablePropertiesList properties);
  void add_table_prop(TableFeatureProp* prop);
  static TableFeatureProp* make_table_feature_prop(uint16_t type);
};

}  // End of namespace of13
}  // End of namespace fluid_msg

#endif

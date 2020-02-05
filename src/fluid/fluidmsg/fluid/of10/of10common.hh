#ifndef OF10OPENFLOW_COMMON_H
#define OF10OPENFLOW_COMMON_H 1

#include <string>
#include <vector>
#include "fluid/ofcommon/common.hh"
#include "fluid/util/util.h"
#include "of10action.hh"
#include "of10match.hh"
#include "openflow-10.h"

namespace fluid_msg {

namespace of10 {

class Port : public PortCommon {
 private:
  uint16_t port_no_;

 public:
  Port() {}
  Port(uint16_t port_no, EthAddress hw_addr, std::string name, uint32_t config,
       uint32_t state, uint32_t curr, uint32_t advertised, uint32_t supported,
       uint32_t peer);
  ~Port() {}
  bool operator==(const Port& other) const;
  bool operator!=(const Port& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  void port_no(uint16_t port_no) { this->port_no_ = port_no; }
  uint16_t port_no() { return this->port_no_; }
};

class QueuePropMinRate : public QueuePropRate {
 public:
  QueuePropMinRate() : QueuePropRate(of10::OFPQT_MIN_RATE) {}
  QueuePropMinRate(uint16_t rate);
  ~QueuePropMinRate() {}
  virtual bool equals(const QueueProperty& other);
  virtual QueuePropMinRate* clone() { return new QueuePropMinRate(*this); }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
};

/* Queue description*/
class PacketQueue : public PacketQueueCommon {
 public:
  PacketQueue() {}
  PacketQueue(uint32_t queue_id);
  PacketQueue(uint32_t queue_id, QueuePropertyList properties);
  ~PacketQueue() {}
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
};

class FlowStats : public FlowStatsCommon {
 private:
  of10::Match match_;
  ActionList actions_;

 public:
  FlowStats() {}
  FlowStats(uint8_t table_id, uint32_t duration_sec, uint32_t duration_nsec,
            uint16_t priority, uint16_t idle_timeout, uint16_t hard_timeout,
            uint64_t cookie, uint64_t packet_count, uint64_t byte_count);
  FlowStats(uint8_t table_id, uint32_t duration_sec, uint32_t duration_nsec,
            uint16_t priority, uint16_t idle_timeout, uint16_t hard_timeout,
            uint64_t cookie, uint64_t packet_count, uint64_t byte_count,
            of10::Match match, ActionList actions);
  ~FlowStats() {}

  bool operator==(const FlowStats& other) const;
  bool operator!=(const FlowStats& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  of10::Match match() { return this->match_; }
  ActionList actions() { return this->actions_; }

  void match(of10::Match match) { this->match_ = match; }
  void actions(ActionList actions);
  void add_action(Action& action);
  void add_action(Action* action);
};

class TableStats : public TableStatsCommon {
 private:
  std::string name_;
  uint32_t wildcards_;
  uint32_t max_entries_;

 public:
  TableStats() {}

  TableStats(uint8_t table_id, std::string name, uint32_t wildcards,
             uint32_t max_entries, uint32_t active_count, uint64_t lookup_count,
             uint64_t matched_count);
  ~TableStats() {}

  bool operator==(const TableStats& other) const;
  bool operator!=(const TableStats& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  std::string name() { return this->name_; }
  uint32_t wildcards() { return this->wildcards_; }
  uint32_t max_entries() { return this->max_entries_; }
  void name(std::string name) { this->name_ = name; }
  void wildcards(uint32_t wildcards) { this->wildcards_ = wildcards; }
  void max_entries(uint32_t max_entries) { this->max_entries_ = max_entries; }
};

class PortStats : public PortStatsCommon {
 private:
  uint16_t port_no_;

 public:
  PortStats() {}

  PortStats(uint16_t port_no, struct port_rx_tx_stats tx_stats,
            struct port_err_stats err_stats, uint64_t collisions);
  ~PortStats() {}

  bool operator==(const PortStats& other) const;
  bool operator!=(const PortStats& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint16_t port_no() { return this->port_no_; }
  void port_no(uint16_t port_no) { this->port_no_ = port_no; }
};

class QueueStats : public QueueStatsCommon {
 private:
  uint16_t port_no_;

 public:
  QueueStats() {}

  QueueStats(uint16_t port_no, uint32_t queue_id, uint64_t tx_bytes,
             uint64_t tx_packets, uint64_t tx_errors);
  ~QueueStats() {}

  bool operator==(const QueueStats& other) const;
  bool operator!=(const QueueStats& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint16_t port_no() { return this->port_no_; }
  void port_no(uint16_t port_no) { this->port_no_ = port_no; }
};

}  // End of namespace of10
}  // End of namespace fluid_msg

#endif

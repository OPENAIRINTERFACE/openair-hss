#ifndef OPENFLOW_COMMON_H
#define OPENFLOW_COMMON_H 1

#include <string>
#include <vector>
#include "action.hh"
#include "fluid/util/ethaddr.hh"
#include "fluid/util/ipaddr.hh"
#include "fluid/util/util.h"

namespace fluid_msg {

class PortCommon {
 protected:
  EthAddress hw_addr_;
  std::string name_;
  uint32_t config_;
  uint32_t state_;
  uint32_t curr_;
  uint32_t advertised_;
  uint32_t supported_;
  uint32_t peer_;

 public:
  PortCommon();
  PortCommon(EthAddress hw_addr, std::string name, uint32_t config,
             uint32_t state, uint32_t curr, uint32_t advertised,
             uint32_t supported, uint32_t peer);
  ~PortCommon() {}
  bool operator==(const PortCommon &other) const;
  bool operator!=(const PortCommon &other) const;
  EthAddress hw_addr() { return this->hw_addr_; }
  std::string name() { return this->name_; }
  uint32_t config() { return this->config_; }
  uint32_t state() { return this->state_; }
  uint32_t curr() { return this->curr_; }
  uint32_t advertised() { return this->advertised_; }
  uint32_t supported() { return this->supported_; }
  uint32_t peer() { return this->peer_; }
  void hw_addr(EthAddress hw_addr) { this->hw_addr_ = hw_addr; }
  void name(std::string name) { this->name_ = name; }
  void config(uint32_t config) { this->config_ = config; }
  void state(uint32_t state) { this->state_ = state; }
  void curr(uint32_t curr) { this->curr_ = curr; }
  void advertised(uint32_t advertised) { this->advertised_ = advertised; }
  void supported(uint32_t supported) { this->supported_ = supported; }
  void peer(uint32_t peer) { this->peer_ = peer; }
};

class QueueProperty {
 protected:
  uint16_t property_;
  uint16_t len_;

 public:
  QueueProperty();
  QueueProperty(uint16_t property);
  virtual ~QueueProperty(){};
  virtual bool equals(const QueueProperty &other);
  virtual bool operator==(const QueueProperty &other) const;
  virtual bool operator!=(const QueueProperty &other) const;
  virtual QueueProperty *clone() { return new QueueProperty(*this); }
  virtual size_t pack(uint8_t *buffer);
  virtual of_error unpack(uint8_t *buffer);
  uint16_t property() { return this->property_; }
  uint16_t len() { return this->len_; }
  void property(uint16_t property) { this->property_ = property; }
  static QueueProperty *make_queue_of10_property(uint16_t property);
  static QueueProperty *make_queue_of13_property(uint16_t property);
  static bool delete_all(QueueProperty *prop) {
    delete prop;
    return true;
  }
};

class QueuePropertyList {
 private:
  uint16_t length_;
  std::list<QueueProperty *> property_list_;

 public:
  QueuePropertyList() : length_(0){};
  QueuePropertyList(std::list<QueueProperty *> prop_list);
  QueuePropertyList(const QueuePropertyList &other);
  QueuePropertyList &operator=(QueuePropertyList other);
  ~QueuePropertyList();
  bool operator==(const QueuePropertyList &other) const;
  bool operator!=(const QueuePropertyList &other) const;
  size_t pack(uint8_t *buffer);
  of_error unpack10(uint8_t *buffer);
  of_error unpack13(uint8_t *buffer);
  friend void swap(QueuePropertyList &first, QueuePropertyList &second);
  uint16_t length() { return this->length_; }
  std::list<QueueProperty *> property_list() { return this->property_list_; }
  void add_property(QueueProperty *prop);
  void length(uint16_t length) { this->length_ = length; }
};

class QueuePropRate : public QueueProperty {
 protected:
  uint16_t rate_;

 public:
  QueuePropRate();
  QueuePropRate(uint16_t property);
  QueuePropRate(uint16_t property, uint16_t rate);
  ~QueuePropRate(){};
  virtual bool equals(const QueueProperty &other);
  virtual QueuePropRate *clone() { return new QueuePropRate(*this); }
  uint16_t rate() { return this->rate_; }
  void rate(uint16_t rate) { this->rate_ = rate; }
};

class SwitchDesc {
 private:
  std::string mfr_desc_;
  std::string hw_desc_;
  std::string sw_desc_;
  std::string serial_num_;
  std::string dp_desc_;

 public:
  SwitchDesc(){};
  SwitchDesc(std::string mfr_desc, std::string hw_desc, std::string sw_desc,
             std::string serial_num, std::string dp_desc);
  ~SwitchDesc(){};
  bool operator==(const SwitchDesc &other) const;
  bool operator!=(const SwitchDesc &other) const;
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  std::string mfr_desc() { return this->mfr_desc_; }
  std::string hw_desc() { return this->hw_desc_; }
  std::string sw_desc() { return this->sw_desc_; }
  std::string serial_num() { return this->serial_num_; }
  std::string dp_desc() { return this->dp_desc_; }
  void mfr_desc(std::string mfr_desc) { this->mfr_desc_ = mfr_desc; }
  void hw_desc(std::string hw_desc) { this->hw_desc_ = hw_desc; }
  void sw_desc(std::string sw_desc) { this->sw_desc_ = sw_desc; }
  void serial_num(std::string serial_num) { this->serial_num_ = serial_num; }
  void dp_desc(std::string dp_desc) { this->dp_desc_ = dp_desc; }
};

/* Queue description*/
class PacketQueueCommon {
 protected:
  uint32_t queue_id_;
  uint16_t len_;
  QueuePropertyList properties_;

 public:
  PacketQueueCommon();
  PacketQueueCommon(uint32_t queue_id);
  virtual ~PacketQueueCommon(){};
  bool operator==(const PacketQueueCommon &other) const;
  bool operator!=(const PacketQueueCommon &other) const;
  uint32_t queue_id() { return this->queue_id_; }
  uint16_t len() { return this->len_; }
  void queue_id(uint32_t queue_id) { this->queue_id_ = queue_id; }
  void property(QueuePropertyList properties);
  void add_property(QueueProperty *qp);
};

class FlowStatsCommon {
 protected:
  uint16_t length_;
  uint8_t table_id_;
  uint32_t duration_sec_;
  uint32_t duration_nsec_;
  uint16_t priority_;
  uint16_t idle_timeout_;
  uint16_t hard_timeout_;
  uint64_t cookie_;
  uint64_t packet_count_;
  uint64_t byte_count_;

 public:
  FlowStatsCommon();
  FlowStatsCommon(uint8_t table_id, uint32_t duration_sec,
                  uint32_t duration_nsec, uint16_t priority,
                  uint16_t idle_timeout, uint16_t hard_timeout, uint64_t cookie,
                  uint64_t packet_count, uint64_t byte_count);
  ~FlowStatsCommon(){};
  bool operator==(const FlowStatsCommon &other) const;
  bool operator!=(const FlowStatsCommon &other) const;
  uint16_t length() { return this->length_; }
  uint8_t table_id() { return this->table_id_; }
  uint32_t duration_sec() { return this->duration_sec_; }
  uint32_t duration_nsec() { return this->duration_nsec_; }
  uint16_t priority() { return this->priority_; }
  uint16_t idle_timeout() { return this->idle_timeout_; }
  uint16_t hard_timeout() { return this->hard_timeout_; }
  uint64_t cookie() { return this->cookie_; }
  uint64_t packet_count() { return this->packet_count_; }
  uint64_t byte_count() { return this->byte_count_; }
  void table_id(uint8_t table_id) { this->table_id_ = table_id; }
  void duration_sec(uint32_t duration_sec) {
    this->duration_sec_ = duration_sec;
  }
  void duration_nsec(uint32_t duration_nsec) {
    this->duration_nsec_ = duration_nsec;
  }
  void priority(uint16_t priority) { this->priority_ = priority; }
  void idle_timeout(uint16_t idle_timeout) {
    this->idle_timeout_ = idle_timeout;
  }
  void hard_timeout(uint16_t hard_timeout) {
    this->hard_timeout_ = hard_timeout;
  }
  void cookie(uint64_t cookie) { this->cookie_ = cookie; }
  void packet_count(uint16_t packet_count) {
    this->packet_count_ = packet_count;
  }
  void byte_count(uint64_t byte_count) { this->byte_count_ = byte_count; }
};

class TableStatsCommon {
 protected:
  uint8_t table_id_;
  uint32_t active_count_;
  uint64_t lookup_count_;
  uint64_t matched_count_;

 public:
  TableStatsCommon();
  TableStatsCommon(uint8_t table_id, uint32_t active_count,
                   uint64_t lookup_count, uint64_t matched_count);
  ~TableStatsCommon(){};
  bool operator==(const TableStatsCommon &other) const;
  bool operator!=(const TableStatsCommon &other) const;
  uint8_t table_id() { return this->table_id_; }
  uint32_t active_count() { return this->active_count_; }
  uint64_t lookup_count() { return this->lookup_count_; }
  uint64_t matched_count() { return this->matched_count_; }
  void table_id(uint8_t table_id) { this->table_id_ = table_id; }
  void active_count(uint32_t active_count) {
    this->active_count_ = active_count;
  }
  void lookup_count(uint64_t lookup_count) {
    this->lookup_count_ = lookup_count;
  }
  void matched_count(uint64_t matched_count) {
    this->matched_count_ = matched_count;
  }
};

struct port_rx_tx_stats {
  uint64_t rx_packets; /* Number of received packets. */
  uint64_t tx_packets; /* Number of transmitted packets. */
  uint64_t rx_bytes;   /* Number of received bytes. */
  uint64_t tx_bytes;   /* Number of transmitted bytes. */
  uint64_t rx_dropped; /* Number of packets dropped by RX. */
  uint64_t tx_dropped; /* Number of packets dropped by TX. */

  bool operator==(const struct port_rx_tx_stats other) const {
    return ((this->rx_packets == other.rx_packets) &&
            (this->tx_packets == other.tx_packets) &&
            (this->rx_bytes == other.rx_bytes) &&
            (this->tx_bytes == other.tx_bytes) &&
            (this->rx_dropped == other.rx_dropped) &&
            (this->tx_dropped == other.tx_dropped));
  }
};

struct port_err_stats {
  uint64_t rx_errors;    /* Number of receive errors. This is a super-set
      of more specific receive errors and should be
      greater than or equal to the sum of all
      rx_*_err values. */
  uint64_t tx_errors;    /* Number of transmit errors. This is a super-set
      of more specific transmit errors and should be
      greater than or equal to the sum of all
      tx_*_err values (none currently defined.) */
  uint64_t rx_frame_err; /* Number of frame alignment errors. */
  uint64_t rx_over_err;  /* Number of packets with RX overrun. */
  uint64_t rx_crc_err;   /* Number of CRC errors. */

  bool operator==(const struct port_err_stats other) const {
    return ((this->rx_errors == other.rx_errors) &&
            (this->tx_errors == other.tx_errors) &&
            (this->rx_frame_err == other.rx_frame_err) &&
            (this->rx_over_err == other.rx_over_err) &&
            (this->rx_crc_err == other.rx_crc_err));
  }
};

class PortStatsCommon {
 protected:
  struct port_rx_tx_stats rx_tx_stats;
  struct port_err_stats err_stats;
  uint64_t collisions_; /* Number of collisions. */
 public:
  PortStatsCommon();
  PortStatsCommon(struct port_rx_tx_stats rx_tx_stats,
                  struct port_err_stats err_stats, uint64_t collisions);
  ~PortStatsCommon(){};
  bool operator==(const PortStatsCommon &other) const;
  bool operator!=(const PortStatsCommon &other) const;
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  uint64_t rx_packets() { return this->rx_tx_stats.rx_packets; }
  uint64_t tx_packets() { return this->rx_tx_stats.tx_packets; }
  uint64_t rx_bytes() { return this->rx_tx_stats.rx_bytes; }
  uint64_t tx_bytes() { return this->rx_tx_stats.tx_bytes; }
  uint64_t rx_dropped() { return this->rx_tx_stats.rx_dropped; }
  uint64_t tx_dropped() { return this->rx_tx_stats.tx_dropped; }
  uint64_t rx_errors() { return this->err_stats.rx_errors; }
  uint64_t tx_errors() { return this->err_stats.tx_errors; }
  uint64_t rx_frame_err() { return this->err_stats.rx_frame_err; }
  uint64_t rx_over_err() { return this->err_stats.rx_over_err; }
  uint64_t rx_crc_err() { return this->err_stats.rx_crc_err; }
  uint64_t collisions() { return this->collisions_; }
  void rx_packets(uint64_t rx_packets) {
    this->rx_tx_stats.rx_packets = rx_packets;
  }
  void tx_packets(uint64_t tx_packets) {
    this->rx_tx_stats.tx_packets = tx_packets;
  }
  void rx_bytes(uint64_t rx_bytes) { this->rx_tx_stats.rx_bytes = rx_bytes; }
  void tx_bytes(uint64_t tx_bytes) { this->rx_tx_stats.tx_bytes = tx_bytes; }
  void rx_dropped(uint64_t rx_dropped) {
    this->rx_tx_stats.rx_dropped = rx_dropped;
  }
  void tx_dropped(uint64_t tx_dropped) {
    this->rx_tx_stats.tx_dropped = tx_dropped;
  }
  void rx_errors(uint64_t rx_errors) { this->err_stats.rx_errors = rx_errors; }
  void tx_errors(uint64_t tx_errors) { this->err_stats.tx_errors = tx_errors; }
  void rx_frame_err(uint64_t rx_frame_err) {
    this->err_stats.rx_frame_err = rx_frame_err;
  }
  void rx_over_err(uint64_t rx_over_err) {
    this->err_stats.rx_over_err = rx_over_err;
  }
  void rx_crc_err(uint64_t rx_crc_err) {
    this->err_stats.rx_crc_err = rx_crc_err;
  }
  void collisions(uint64_t collisions) { this->collisions_ = collisions; }
};

class QueueStatsCommon {
 protected:
  uint32_t queue_id_;
  uint64_t tx_bytes_;
  uint64_t tx_packets_;
  uint64_t tx_errors_;

 public:
  QueueStatsCommon();
  QueueStatsCommon(uint32_t queue_id, uint64_t tx_bytes, uint64_t tx_packets,
                   uint64_t tx_errors);
  ~QueueStatsCommon(){};
  bool operator==(const QueueStatsCommon &other) const;
  bool operator!=(const QueueStatsCommon &other) const;
  uint32_t queue_id() { return this->queue_id_; }
  uint64_t tx_bytes() { return this->tx_bytes_; }
  uint64_t tx_packets() { return this->tx_packets_; }
  uint64_t tx_errors() { return this->tx_errors_; }
  void queue_id(uint32_t queue_id) { this->queue_id_ = queue_id; }
  void tx_bytes(uint64_t tx_bytes) { this->tx_bytes_ = tx_bytes; }
  void tx_packets(uint64_t tx_packets) { this->tx_packets_ = tx_packets; }
  void tx_errors(uint64_t tx_errors) { this->tx_errors_ = tx_errors; }
};

}  // End of namespace fluid_msg

#endif

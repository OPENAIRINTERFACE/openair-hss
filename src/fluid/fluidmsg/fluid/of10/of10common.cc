#include "of10common.hh"

#include "fluid/util/util.h"

namespace fluid_msg {

namespace of10 {

Port::Port(uint16_t port_no, EthAddress hw_addr, std::string name,
           uint32_t config, uint32_t state, uint32_t curr, uint32_t advertised,
           uint32_t supported, uint32_t peer)
    : PortCommon(hw_addr, name, config, state, curr, advertised, supported,
                 peer) {
  this->port_no_ = port_no;
}

bool Port::operator==(const Port &other) const {
  return (PortCommon::operator==(other) && (this->port_no_ == other.port_no_));
}

bool Port::operator!=(const Port &other) const { return !(*this == other); }

size_t Port::pack(uint8_t *buffer) {
  struct of10::ofp_phy_port *port = (struct of10::ofp_phy_port *)buffer;
  port->port_no = hton16(this->port_no_);
  memcpy(port->hw_addr, this->hw_addr_.get_data(), OFP_ETH_ALEN);
  memset(port->name, 0x0, OFP_MAX_PORT_NAME_LEN);
  memcpy(port->name, this->name_.c_str(),
         this->name_.size() < OFP_MAX_PORT_NAME_LEN ? this->name_.size()
                                                    : OFP_MAX_PORT_NAME_LEN);
  port->config = hton32(this->config_);
  port->state = hton32(this->state_);
  port->curr = hton32(this->curr_);
  port->advertised = hton32(this->advertised_);
  port->supported = hton32(this->supported_);
  port->peer = hton32(this->peer_);
  return 0;
}
of_error Port::unpack(uint8_t *buffer) {
  struct of10::ofp_phy_port *port = (struct of10::ofp_phy_port *)buffer;
  this->port_no_ = ntoh16(port->port_no);
  this->hw_addr_ = EthAddress(port->hw_addr);
  this->name_ = std::string(port->name);
  this->config_ = ntoh32(port->config);
  this->state_ = ntoh32(port->state);
  this->curr_ = ntoh32(port->curr);
  this->advertised_ = ntoh32(port->advertised);
  this->supported_ = ntoh32(port->supported);
  this->peer_ = ntoh32(port->peer);
  return 0;
}

QueuePropMinRate::QueuePropMinRate(uint16_t rate)
    : QueuePropRate(of10::OFPQT_MIN_RATE, rate) {
  this->len_ = sizeof(struct of10::ofp_queue_prop_min_rate);
}

bool QueuePropMinRate::equals(const QueueProperty &other) {
  if (const QueuePropMinRate *prop =
          dynamic_cast<const QueuePropMinRate *>(&other)) {
    return ((QueuePropRate::equals(other)));
  } else {
    return false;
  }
}

size_t QueuePropMinRate::pack(uint8_t *buffer) {
  struct of10::ofp_queue_prop_min_rate *qp =
      (struct of10::ofp_queue_prop_min_rate *)buffer;
  QueueProperty::pack(buffer);
  qp->rate = hton16(this->rate_);
  memset(qp->pad, 0x0, 6);
  return this->len_;
}

of_error QueuePropMinRate::unpack(uint8_t *buffer) {
  struct of10::ofp_queue_prop_min_rate *qp =
      (struct of10::ofp_queue_prop_min_rate *)buffer;
  QueueProperty::unpack(buffer);
  this->rate_ = ntoh16(qp->rate);
  return 0;
}

PacketQueue::PacketQueue(uint32_t queue_id) : PacketQueueCommon(queue_id) {
  this->len_ = sizeof(struct of10::ofp_packet_queue);
}

PacketQueue::PacketQueue(uint32_t queue_id, QueuePropertyList properties)
    : PacketQueueCommon(queue_id) {
  this->properties_ = properties;
  this->len_ = sizeof(struct of10::ofp_packet_queue) + properties.length();
}

size_t PacketQueue::pack(uint8_t *buffer) {
  struct of10::ofp_packet_queue *pq = (struct of10::ofp_packet_queue *)buffer;
  pq->queue_id = hton32(this->queue_id_);
  pq->len = hton16(this->len_);
  memset(pq->pad, 0x0, 2);
  uint8_t *p = buffer + sizeof(struct of10::ofp_packet_queue);
  this->properties_.pack(p);
  return this->len_;
}

of_error PacketQueue::unpack(uint8_t *buffer) {
  struct of10::ofp_packet_queue *pq = (struct of10::ofp_packet_queue *)buffer;
  this->queue_id_ = ntoh32(pq->queue_id);
  this->len_ = ntoh16(pq->len);
  uint8_t *p = buffer + sizeof(struct of10::ofp_packet_queue);
  this->properties_.length(this->len_ - sizeof(struct of10::ofp_packet_queue));
  this->properties_.unpack10(p);
  return 0;
}

FlowStats::FlowStats(uint8_t table_id, uint32_t duration_sec,
                     uint32_t duration_nsec, uint16_t priority,
                     uint16_t idle_timeout, uint16_t hard_timeout,
                     uint64_t cookie, uint64_t packet_count,
                     uint64_t byte_count)
    : FlowStatsCommon(table_id, duration_sec, duration_nsec, priority,
                      idle_timeout, hard_timeout, cookie, packet_count,
                      byte_count) {
  this->length_ = sizeof(struct of10::ofp_flow_stats);
}

FlowStats::FlowStats(uint8_t table_id, uint32_t duration_sec,
                     uint32_t duration_nsec, uint16_t priority,
                     uint16_t idle_timeout, uint16_t hard_timeout,
                     uint64_t cookie, uint64_t packet_count,
                     uint64_t byte_count, of10::Match match, ActionList actions)
    : FlowStatsCommon(table_id, duration_sec, duration_nsec, priority,
                      idle_timeout, hard_timeout, cookie, packet_count,
                      byte_count) {
  this->match_ = match;
  this->actions_ = actions;
  this->length_ = sizeof(struct of10::ofp_flow_stats) + actions.length();
}

bool FlowStats::operator==(const FlowStats &other) const {
  return ((FlowStatsCommon::operator==(other)) &&
          (this->actions_ == other.actions_) && (this->match_ == other.match_));
}

bool FlowStats::operator!=(const FlowStats &other) const {
  return !(*this == other);
}

size_t FlowStats::pack(uint8_t *buffer) {
  struct of10::ofp_flow_stats *fs = (struct of10::ofp_flow_stats *)buffer;
  this->match_.pack(buffer + 4);
  fs->length = hton16(this->length_);
  fs->table_id = this->table_id_;
  memset(&fs->pad, 0x0, 1);
  fs->duration_sec = hton32(this->duration_sec_);
  fs->duration_nsec = hton32(this->duration_nsec_);
  fs->priority = hton16(this->priority_);
  fs->idle_timeout = hton16(this->idle_timeout_);
  fs->hard_timeout = hton16(this->hard_timeout_);
  memset(fs->pad2, 0x0, 6);
  fs->cookie = hton64(this->cookie_);
  fs->packet_count = hton64(this->packet_count_);
  fs->byte_count = hton64(this->byte_count_);
  uint8_t *p = buffer + sizeof(struct of10::ofp_flow_stats);
  this->actions_.pack(p);
  return this->length_;
}

of_error FlowStats::unpack(uint8_t *buffer) {
  struct of10::ofp_flow_stats *fs = (struct of10::ofp_flow_stats *)buffer;
  this->match_.unpack(buffer + 4);
  this->length_ = ntoh16(fs->length);
  this->table_id_ = fs->table_id;
  this->duration_sec_ = ntoh32(fs->duration_sec);
  this->duration_nsec_ = ntoh32(fs->duration_nsec);
  this->priority_ = ntoh16(fs->priority);
  this->idle_timeout_ = ntoh16(fs->idle_timeout);
  this->hard_timeout_ = ntoh16(fs->hard_timeout);
  this->cookie_ = ntoh64(fs->cookie);
  this->packet_count_ = ntoh64(fs->packet_count);
  this->byte_count_ = ntoh64(fs->byte_count);
  this->actions_.length(this->length_ - sizeof(struct of10::ofp_flow_stats));
  uint8_t *p = buffer + sizeof(struct of10::ofp_flow_stats);
  this->actions_.unpack10(p);
  return 0;
}

void FlowStats::actions(ActionList actions) {
  this->actions_ = actions;
  this->length_ += actions.length();
}

void FlowStats::add_action(Action &action) {
  this->actions_.add_action(action);
  this->length_ += action.length();
}

void FlowStats::add_action(Action *action) {
  this->actions_.add_action(action);
  this->length_ += action->length();
}

TableStats::TableStats(uint8_t table_id, std::string name, uint32_t wildcards,
                       uint32_t max_entries, uint32_t active_count,
                       uint64_t lookup_count, uint64_t matched_count)
    : TableStatsCommon(table_id, active_count, lookup_count, matched_count) {
  this->name_ = name;
  this->wildcards_ = wildcards;
  this->max_entries_ = max_entries;
}

bool TableStats::operator==(const TableStats &other) const {
  return ((TableStatsCommon::operator==(other)) &&
          (this->name_ == other.name_) &&
          (this->wildcards_ == other.wildcards_) &&
          (this->max_entries_ == other.max_entries_));
}

bool TableStats::operator!=(const TableStats &other) const {
  return !(*this == other);
}

size_t TableStats::pack(uint8_t *buffer) {
  struct of10::ofp_table_stats *ts = (struct of10::ofp_table_stats *)buffer;
  ts->table_id = this->table_id_;
  memset(ts->pad, 0x0, 3);
  memset(ts->name, 0x0, OFP_MAX_TABLE_NAME_LEN);
  memcpy(ts->name, this->name_.c_str(),
         this->name_.size() < OFP_MAX_TABLE_NAME_LEN ? this->name_.size()
                                                     : OFP_MAX_TABLE_NAME_LEN);
  ts->wildcards = hton32(this->wildcards_);
  ts->max_entries = hton32(this->max_entries_);
  ts->active_count = hton32(this->active_count_);
  ts->lookup_count = hton64(this->lookup_count_);
  ts->matched_count = hton64(this->matched_count_);
  return 0;
}

of_error TableStats::unpack(uint8_t *buffer) {
  struct of10::ofp_table_stats *ts = (struct of10::ofp_table_stats *)buffer;
  this->table_id_ = ts->table_id;
  this->name_ = std::string(ts->name);
  this->wildcards_ = ntoh32(ts->wildcards);
  this->max_entries_ = ntoh32(ts->max_entries);
  this->active_count_ = ntoh32(ts->active_count);
  this->lookup_count_ = ntoh64(ts->lookup_count);
  this->matched_count_ = ntoh64(ts->matched_count);
  return 0;
}

PortStats::PortStats(uint16_t port_no, struct port_rx_tx_stats rx_tx_stats,
                     struct port_err_stats err_stats, uint64_t collisions)
    : PortStatsCommon(rx_tx_stats, err_stats, collisions) {
  this->port_no_ = port_no;
}

bool PortStats::operator==(const PortStats &other) const {
  return ((PortStatsCommon::operator==(other)) &&
          (this->port_no_ == other.port_no_));
}

bool PortStats::operator!=(const PortStats &other) const {
  return !(*this == other);
}

size_t PortStats::pack(uint8_t *buffer) {
  struct of10::ofp_port_stats *ps = (struct of10::ofp_port_stats *)buffer;
  ps->port_no = hton16(this->port_no_);
  memset(ps->pad, 0x0, 6);
  PortStatsCommon::pack(buffer + 8);
  ps->collisions = hton64(this->collisions_);
  return 0;
}

of_error PortStats::unpack(uint8_t *buffer) {
  struct of10::ofp_port_stats *ps = (struct of10::ofp_port_stats *)buffer;
  this->port_no_ = ntoh16(ps->port_no);
  PortStatsCommon::unpack(buffer + 8);
  this->collisions_ = ntoh64(ps->collisions);
  return 0;
}

QueueStats::QueueStats(uint16_t port_no, uint32_t queue_id, uint64_t tx_bytes,
                       uint64_t tx_packets, uint64_t tx_errors)
    : QueueStatsCommon(queue_id, tx_bytes, tx_packets, tx_errors) {
  this->port_no_ = port_no;
}

bool QueueStats::operator==(const QueueStats &other) const {
  return ((QueueStatsCommon::operator==(other)) &&
          (this->port_no_ == other.port_no_));
}

bool QueueStats::operator!=(const QueueStats &other) const {
  return !(*this == other);
}

size_t QueueStats::pack(uint8_t *buffer) {
  struct of10::ofp_queue_stats *qs = (struct of10::ofp_queue_stats *)buffer;
  qs->port_no = hton16(this->port_no_);
  memset(qs->pad, 0x0, 2);
  qs->queue_id = hton32(this->queue_id_);
  qs->tx_bytes = hton64(this->tx_bytes_);
  qs->tx_packets = hton64(this->tx_packets_);
  qs->tx_errors = hton64(this->tx_errors_);
  return 0;
}

of_error QueueStats::unpack(uint8_t *buffer) {
  struct of10::ofp_queue_stats *qs = (struct of10::ofp_queue_stats *)buffer;
  this->port_no_ = ntoh16(qs->port_no);
  this->queue_id_ = ntoh32(qs->queue_id);
  this->tx_bytes_ = ntoh64(qs->tx_bytes);
  this->tx_packets_ = ntoh64(qs->tx_packets);
  this->tx_errors_ = ntoh64(qs->tx_errors);
  return 0;
}

}  // namespace of10

QueueProperty *QueueProperty::make_queue_of10_property(uint16_t property) {
  switch (property) {
    case (of10::OFPQT_NONE): {
      return new QueuePropRate();
    }
    case (of10::OFPQT_MIN_RATE): {
      return new of10::QueuePropMinRate();
    }
  }
  return NULL;
}

}  // End of namespace fluid_msg

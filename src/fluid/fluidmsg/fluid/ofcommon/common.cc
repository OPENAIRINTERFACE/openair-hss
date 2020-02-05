#include "common.hh"

namespace fluid_msg {

PortCommon::PortCommon()
    : hw_addr_(),
      name_(),
      config_(0),
      state_(0),
      curr_(0),
      advertised_(0),
      supported_(0),
      peer_(0) {}

PortCommon::PortCommon(EthAddress hw_addr, std::string name, uint32_t config,
                       uint32_t state, uint32_t curr, uint32_t advertised,
                       uint32_t supported, uint32_t peer)
    : hw_addr_(hw_addr),
      name_(name),
      config_(config),
      state_(state),
      curr_(curr),
      advertised_(advertised),
      supported_(supported),
      peer_(peer) {}

bool PortCommon::operator==(const PortCommon &other) const {
  return ((this->hw_addr_ == other.hw_addr_) && (this->name_ == other.name_) &&
          (this->config_ == other.config_) && (this->state_ == other.state_) &&
          (this->curr_ == other.curr_) &&
          (this->advertised_ == other.advertised_) &&
          (this->supported_ == other.supported_) &&
          (this->peer_ == other.peer_));
}

bool PortCommon::operator!=(const PortCommon &other) const {
  return !(*this == other);
}

QueueProperty::QueueProperty() : property_(0), len_(0) {}

QueueProperty::QueueProperty(uint16_t property)
    : property_(property), len_(sizeof(struct ofp_queue_prop_header)) {}

bool QueueProperty::equals(const QueueProperty &other) {
  return ((*this == other));
}

bool QueueProperty::operator==(const QueueProperty &other) const {
  return ((this->property_ == other.property_) && (this->len_ == other.len_));
}

bool QueueProperty::operator!=(const QueueProperty &other) const {
  return !(*this == other);
}

size_t QueueProperty::pack(uint8_t *buffer) {
  struct ofp_queue_prop_header *qp = (struct ofp_queue_prop_header *)buffer;
  qp->property = hton16(this->property_);
  qp->len = hton16(this->len_);
  return this->len_;
}

of_error QueueProperty::unpack(uint8_t *buffer) {
  struct ofp_queue_prop_header *qp = (struct ofp_queue_prop_header *)buffer;
  this->property_ = ntoh16(qp->property);
  this->len_ = ntoh16(qp->len);
  return 0;
}

QueuePropertyList::QueuePropertyList(std::list<QueueProperty *> property_list) {
  this->property_list_ = property_list_;
  for (std::list<QueueProperty *>::const_iterator it = property_list.begin();
       it != property_list.end(); ++it) {
    this->length_ += (*it)->len();
  }
}

QueuePropertyList::QueuePropertyList(const QueuePropertyList &other) {
  this->length_ = other.length_;
  for (std::list<QueueProperty *>::const_iterator it =
           other.property_list_.begin();
       it != other.property_list_.end(); ++it) {
    this->property_list_.push_back((*it)->clone());
  }
}

QueuePropertyList::~QueuePropertyList() {
  this->property_list_.remove_if(QueueProperty::delete_all);
}

bool QueuePropertyList::operator==(const QueuePropertyList &other) const {
  std::list<QueueProperty *>::const_iterator ot = other.property_list_.begin();
  for (std::list<QueueProperty *>::const_iterator it =
           this->property_list_.begin();
       it != this->property_list_.end(); ++it, ++ot) {
    if (!((*it)->equals(**ot))) {
      return false;
    }
  }
  return true;
}

bool QueuePropertyList::operator!=(const QueuePropertyList &other) const {
  return !(*this == other);
}

size_t QueuePropertyList::pack(uint8_t *buffer) {
  uint8_t *p = buffer;
  for (std::list<QueueProperty *>::iterator it = this->property_list_.begin(),
                                            end = this->property_list_.end();
       it != end; it++) {
    (*it)->pack(p);
    p += (*it)->len();
  }
  return 0;
}

of_error QueuePropertyList::unpack10(uint8_t *buffer) {
  uint8_t *p = buffer;
  size_t len = this->length_;
  QueueProperty *prop;
  while (len) {
    uint16_t type = ntoh16(*((uint16_t *)p));
    prop = QueueProperty::make_queue_of10_property(type);
    prop->unpack(p);
    this->property_list_.push_back(prop);
    len -= prop->len();
    p += prop->len();
  }
  return 0;
}

of_error QueuePropertyList::unpack13(uint8_t *buffer) {
  uint8_t *p = buffer;
  size_t len = this->length_;
  QueueProperty *prop;
  while (len) {
    uint16_t type = ntoh16(*((uint16_t *)p));
    prop = QueueProperty::make_queue_of13_property(type);
    prop->unpack(p);
    this->property_list_.push_back(prop);
    len -= prop->len();
    p += prop->len();
  }
  return 0;
}

QueuePropertyList &QueuePropertyList::operator=(QueuePropertyList other) {
  swap(*this, other);
  return *this;
}

void swap(QueuePropertyList &first, QueuePropertyList &second) {
  std::swap(first.length_, second.length_);
  std::swap(first.property_list_, second.property_list_);
}

void QueuePropertyList::add_property(QueueProperty *prop) {
  this->property_list_.push_back(prop);
  this->length_ += prop->len();
}

QueuePropRate::QueuePropRate() : QueueProperty(), rate_(0) {}

QueuePropRate::QueuePropRate(uint16_t property)
    : QueueProperty(property), rate_(0){};

QueuePropRate::QueuePropRate(uint16_t property, uint16_t rate)
    : QueueProperty(property), rate_(rate) {}

bool QueuePropRate::equals(const QueueProperty &other) {
  if (const QueuePropRate *prop = dynamic_cast<const QueuePropRate *>(&other)) {
    return ((QueueProperty::equals(other)) && (this->rate_ == prop->rate_));
  } else {
    return false;
  }
}

PacketQueueCommon::PacketQueueCommon() : len_(0), queue_id_(0), properties_() {}

PacketQueueCommon::PacketQueueCommon(uint32_t queue_id)
    : len_(0), queue_id_(queue_id) {}

void PacketQueueCommon::property(QueuePropertyList properties) {
  this->properties_ = properties;
  this->len_ += properties.length();
}

bool PacketQueueCommon::operator==(const PacketQueueCommon &other) const {
  return ((this->properties_ == other.properties_) &&
          (this->len_ == other.len_));
}

bool PacketQueueCommon::operator!=(const PacketQueueCommon &other) const {
  return !(*this == other);
}

void PacketQueueCommon::add_property(QueueProperty *qp) {
  this->properties_.add_property(qp);
  this->len_ += qp->len();
}

SwitchDesc::SwitchDesc(std::string mfr_desc, std::string hw_desc,
                       std::string sw_desc, std::string serial_num,
                       std::string dp_desc) {
  this->mfr_desc_ = mfr_desc;
  this->hw_desc_ = hw_desc;
  this->sw_desc_ = sw_desc;
  this->serial_num_ = serial_num;
  this->dp_desc_ = dp_desc;
}

bool SwitchDesc::operator==(const SwitchDesc &other) const {
  return ((this->mfr_desc_ == other.mfr_desc_) &&
          (this->hw_desc_ == other.hw_desc_) &&
          (this->sw_desc_ == other.sw_desc_) &&
          (this->serial_num_ == other.serial_num_) &&
          (this->dp_desc_ == other.dp_desc_));
}

bool SwitchDesc::operator!=(const SwitchDesc &other) const {
  return !(*this == other);
}

size_t SwitchDesc::pack(uint8_t *buffer) {
  struct ofp_desc *ds = (struct ofp_desc *)buffer;
  memset(ds->mfr_desc, 0x0, DESC_STR_LEN);
  memset(ds->hw_desc, 0x0, DESC_STR_LEN);
  memset(ds->sw_desc, 0x0, DESC_STR_LEN);
  memset(ds->serial_num, 0x0, SERIAL_NUM_LEN);
  memset(ds->dp_desc, 0x0, DESC_STR_LEN);
  memcpy(ds->mfr_desc, this->mfr_desc_.c_str(),
         this->mfr_desc_.size() < DESC_STR_LEN ? this->mfr_desc_.size()
                                               : DESC_STR_LEN);
  memcpy(ds->hw_desc, this->hw_desc_.c_str(),
         this->hw_desc_.size() < DESC_STR_LEN ? this->hw_desc_.size()
                                              : DESC_STR_LEN);
  memcpy(ds->sw_desc, this->sw_desc_.c_str(),
         this->sw_desc_.size() < DESC_STR_LEN ? this->sw_desc_.size()
                                              : DESC_STR_LEN);
  memcpy(ds->serial_num, this->serial_num_.c_str(),
         this->serial_num_.size() < SERIAL_NUM_LEN ? this->serial_num_.size()
                                                   : SERIAL_NUM_LEN);
  memcpy(ds->dp_desc, this->dp_desc_.c_str(),
         this->dp_desc_.size() < DESC_STR_LEN ? this->dp_desc_.size()
                                              : DESC_STR_LEN);
  return 0;
}

of_error SwitchDesc::unpack(uint8_t *buffer) {
  struct ofp_desc *ds = (struct ofp_desc *)buffer;
  this->mfr_desc_ = std::string(ds->mfr_desc);
  this->hw_desc_ = std::string(ds->hw_desc);
  this->sw_desc_ = std::string(ds->sw_desc);
  this->serial_num_ = std::string(ds->serial_num);
  this->dp_desc_ = std::string(ds->dp_desc);

  return 0;
}

FlowStatsCommon::FlowStatsCommon()
    : length_(0),
      table_id_(0),
      duration_sec_(0),
      duration_nsec_(0),
      priority_(0),
      idle_timeout_(0),
      hard_timeout_(0),
      cookie_(0),
      packet_count_(0),
      byte_count_(0) {}

FlowStatsCommon::FlowStatsCommon(uint8_t table_id, uint32_t duration_sec,
                                 uint32_t duration_nsec, uint16_t priority,
                                 uint16_t idle_timeout, uint16_t hard_timeout,
                                 uint64_t cookie, uint64_t packet_count,
                                 uint64_t byte_count)
    : length_(0),
      table_id_(table_id),
      duration_sec_(duration_sec),
      duration_nsec_(duration_nsec),
      priority_(priority),
      idle_timeout_(idle_timeout),
      hard_timeout_(hard_timeout),
      cookie_(cookie),
      packet_count_(packet_count),
      byte_count_(byte_count) {}

bool FlowStatsCommon::operator==(const FlowStatsCommon &other) const {
  return ((this->table_id_ == other.table_id_) &&
          (this->duration_sec_ == other.duration_sec_) &&
          (this->duration_nsec_ == other.duration_nsec_) &&
          (this->priority_ == other.priority_) &&
          (this->idle_timeout_ == other.idle_timeout_) &&
          (this->hard_timeout_ == other.hard_timeout_) &&
          (this->cookie_ == other.cookie_) &&
          (this->packet_count_ == other.packet_count_) &&
          (this->byte_count_ == other.byte_count_));
}

bool FlowStatsCommon::operator!=(const FlowStatsCommon &other) const {
  return !(*this == other);
}

TableStatsCommon::TableStatsCommon()
    : table_id_(0), active_count_(0), lookup_count_(0), matched_count_(0) {}

TableStatsCommon::TableStatsCommon(uint8_t table_id, uint32_t active_count,
                                   uint64_t lookup_count,
                                   uint64_t matched_count)
    : table_id_(table_id),
      active_count_(active_count),
      lookup_count_(lookup_count),
      matched_count_(matched_count) {}

bool TableStatsCommon::operator==(const TableStatsCommon &other) const {
  return ((this->table_id_ == other.table_id_) &&
          (this->active_count_ == other.active_count_) &&
          (this->lookup_count_ == other.lookup_count_) &&
          (this->matched_count_ == other.matched_count_));
}

bool TableStatsCommon::operator!=(const TableStatsCommon &other) const {
  return !(*this == other);
}

PortStatsCommon::PortStatsCommon() : collisions_(0) {}

PortStatsCommon::PortStatsCommon(struct port_rx_tx_stats rx_tx_stats,
                                 struct port_err_stats err_stats,
                                 uint64_t collisions) {
  this->rx_tx_stats = rx_tx_stats;
  this->err_stats = err_stats;
  this->collisions_ = collisions;
}

bool PortStatsCommon::operator==(const PortStatsCommon &other) const {
  return ((this->rx_tx_stats == other.rx_tx_stats) &&
          (this->err_stats == other.err_stats) &&
          (this->collisions_ == other.collisions_));
}

bool PortStatsCommon::operator!=(const PortStatsCommon &other) const {
  return !(*this == other);
}

size_t PortStatsCommon::pack(uint8_t *buffer) {
  struct port_rx_tx_stats *rt = (struct port_rx_tx_stats *)buffer;
  struct port_err_stats *es =
      (struct port_err_stats *)(buffer + sizeof(struct port_rx_tx_stats));
  rt->rx_packets = hton64(this->rx_tx_stats.rx_packets);
  rt->tx_packets = hton64(this->rx_tx_stats.tx_packets);
  rt->rx_bytes = hton64(this->rx_tx_stats.rx_bytes);
  rt->tx_bytes = hton64(this->rx_tx_stats.tx_bytes);
  rt->rx_dropped = hton64(this->rx_tx_stats.rx_dropped);
  rt->tx_dropped = hton64(this->rx_tx_stats.tx_dropped);
  es->rx_errors = hton64(this->err_stats.rx_errors);
  es->tx_errors = hton64(this->err_stats.tx_errors);
  es->rx_frame_err = hton64(this->err_stats.rx_frame_err);
  es->rx_over_err = hton64(this->err_stats.rx_over_err);
  es->rx_crc_err = hton64(this->err_stats.rx_crc_err);
  return 0;
}

of_error PortStatsCommon::unpack(uint8_t *buffer) {
  struct port_rx_tx_stats *rt = (struct port_rx_tx_stats *)buffer;
  struct port_err_stats *es =
      (struct port_err_stats *)(buffer + sizeof(struct port_rx_tx_stats));
  this->rx_tx_stats.rx_packets = hton64(rt->rx_packets);
  this->rx_tx_stats.tx_packets = hton64(rt->tx_packets);
  this->rx_tx_stats.rx_bytes = hton64(rt->rx_bytes);
  this->rx_tx_stats.tx_bytes = hton64(rt->tx_bytes);
  this->rx_tx_stats.rx_dropped = hton64(rt->rx_dropped);
  this->rx_tx_stats.tx_dropped = hton64(rt->tx_dropped);
  this->err_stats.rx_errors = hton64(es->rx_errors);
  this->err_stats.tx_errors = hton64(es->tx_errors);
  this->err_stats.rx_frame_err = hton64(es->rx_frame_err);
  this->err_stats.rx_over_err = hton64(es->rx_over_err);
  this->err_stats.rx_crc_err = hton64(es->rx_crc_err);
  return 0;
}

QueueStatsCommon::QueueStatsCommon()
    : queue_id_(0), tx_bytes_(0), tx_packets_(0), tx_errors_(0) {}

QueueStatsCommon::QueueStatsCommon(uint32_t queue_id, uint64_t tx_bytes,
                                   uint64_t tx_packets, uint64_t tx_errors)
    : queue_id_(queue_id),
      tx_bytes_(tx_bytes),
      tx_packets_(tx_packets),
      tx_errors_(tx_errors) {}

bool QueueStatsCommon::operator==(const QueueStatsCommon &other) const {
  return ((this->queue_id_ == other.queue_id_) &&
          (this->tx_bytes_ == other.tx_bytes_) &&
          (this->tx_packets_ == other.tx_packets_) &&
          (this->tx_errors_ == other.tx_errors_));
}

bool QueueStatsCommon::operator!=(const QueueStatsCommon &other) const {
  return !(*this == other);
}

}  // End of namespace fluid_msg

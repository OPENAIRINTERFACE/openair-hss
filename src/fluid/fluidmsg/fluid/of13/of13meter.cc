#include "of13meter.hh"
#include "util/util.h"

namespace fluid_msg {

namespace of13 {

MeterBand::MeterBand()
    : type_(0),
      rate_(0),
      burst_size_(0),
      len_(sizeof(struct of13::ofp_meter_band_header)) {}

MeterBand::MeterBand(uint16_t type, uint32_t rate, uint32_t burst_size)
    : type_(type),
      rate_(rate),
      burst_size_(burst_size),
      len_(sizeof(struct of13::ofp_meter_band_header)) {}

bool MeterBand::equals(const MeterBand &other) {
  return ((this->type_ == other.type_) && (this->rate_ == other.rate_) &&
          (this->burst_size_ == other.burst_size_));
}

size_t MeterBand::pack(uint8_t *buffer) {
  struct of13::ofp_meter_band_header *bh =
      (struct of13::ofp_meter_band_header *)buffer;
  bh->type = hton16(this->type_);
  bh->len = hton16(this->len_);
  bh->rate = hton32(this->rate_);
  bh->burst_size = hton32(this->burst_size_);
  return this->len_;
}

of_error MeterBand::unpack(uint8_t *buffer) {
  struct of13::ofp_meter_band_header *bh =
      (struct of13::ofp_meter_band_header *)buffer;
  this->type_ = hton16(bh->type);
  this->len_ = hton16(bh->len);
  this->rate_ = hton32(bh->rate);
  this->burst_size_ = hton32(bh->burst_size);
  return 0;
}

MeterBand *MeterBand::make_meter_band(uint16_t type) {
  switch (type) {
    case (of13::OFPMBT_DROP): {
      return new MeterBandDrop();
    }
    case (of13::OFPMBT_DSCP_REMARK): {
      return new MeterBandDSCPRemark();
    }
    case (of13::OFPMBT_EXPERIMENTER): {
      return new MeterBandExperimenter();
    }
  }
  return NULL;
}

MeterBandList::MeterBandList(std::list<MeterBand *> band_list) {
  this->band_list_ = band_list_;
  for (std::list<MeterBand *>::const_iterator it = band_list.begin();
       it != band_list.end(); ++it) {
    this->length_ += (*it)->len();
  }
}

MeterBandList::MeterBandList(const MeterBandList &other) {
  this->length_ = other.length_;
  for (std::list<MeterBand *>::const_iterator it = other.band_list_.begin();
       it != other.band_list_.end(); ++it) {
    this->band_list_.push_back((*it)->clone());
  }
}

MeterBandList::~MeterBandList() {
  this->band_list_.remove_if(MeterBand::delete_all);
}

bool MeterBandList::operator==(const MeterBandList &other) const {
  std::list<MeterBand *>::const_iterator ot = other.band_list_.begin();
  for (std::list<MeterBand *>::const_iterator it = this->band_list_.begin();
       it != this->band_list_.end(); ++it, ++ot) {
    if (!((*it)->equals(**ot))) {
      return false;
    }
  }
  return true;
}

bool MeterBandList::operator!=(const MeterBandList &other) const {
  return !(*this == other);
}

size_t MeterBandList::pack(uint8_t *buffer) {
  uint8_t *p = buffer;
  for (std::list<MeterBand *>::iterator it = this->band_list_.begin(),
                                        end = this->band_list_.end();
       it != end; it++) {
    (*it)->pack(p);
    p += (*it)->len();
  }
  return 0;
}

of_error MeterBandList::unpack(uint8_t *buffer) {
  uint8_t *p = buffer;
  size_t len = this->length_;
  MeterBand *band;
  while (len) {
    uint16_t type = ntoh16(*((uint16_t *)p));
    band = MeterBand::make_meter_band(type);
    band->unpack(p);
    this->band_list_.push_back(band);
    len -= band->len();
    p += band->len();
  }
  return 0;
}

MeterBandList &MeterBandList::operator=(MeterBandList other) {
  swap(*this, other);
  return *this;
}

void swap(MeterBandList &first, MeterBandList &second) {
  std::swap(first.length_, second.length_);
  std::swap(first.band_list_, second.band_list_);
}

void MeterBandList::add_band(MeterBand *band) {
  this->band_list_.push_back(band);
  this->length_ += band->len();
}

MeterBandDrop::MeterBandDrop() : MeterBand(of13::OFPMBT_DROP, 0, 0) {
  this->len_ = sizeof(struct of13::ofp_meter_band_drop);
}

MeterBandDrop::MeterBandDrop(uint32_t rate, uint32_t burst_size)
    : MeterBand(of13::OFPMBT_DROP, rate, burst_size) {
  this->len_ = sizeof(struct of13::ofp_meter_band_drop);
}

size_t MeterBandDrop::pack(uint8_t *buffer) {
  struct of13::ofp_meter_band_drop *bd =
      (struct of13::ofp_meter_band_drop *)buffer;
  MeterBand::pack(buffer);
  memset(bd->pad, 0x0, 4);
  return this->len_;
}

of_error MeterBandDrop::unpack(uint8_t *buffer) {
  MeterBand::unpack(buffer);
  return 0;
}

MeterBandDSCPRemark::MeterBandDSCPRemark()
    : MeterBand(of13::OFPMBT_DSCP_REMARK, 0, 0), prec_level_(0) {
  this->len_ = sizeof(struct of13::ofp_meter_band_dscp_remark);
}

MeterBandDSCPRemark::MeterBandDSCPRemark(uint32_t rate, uint32_t burst_size,
                                         uint8_t prec_level)
    : MeterBand(of13::OFPMBT_DSCP_REMARK, rate, burst_size) {
  this->prec_level_ = prec_level;
  this->len_ = sizeof(struct of13::ofp_meter_band_dscp_remark);
}

bool MeterBandDSCPRemark::equals(const MeterBand &other) {
  if (const MeterBandDSCPRemark *band =
          dynamic_cast<const MeterBandDSCPRemark *>(&other)) {
    return ((MeterBand::equals(other)) &&
            (this->prec_level_ == band->prec_level_));
  } else {
    return false;
  }
}

size_t MeterBandDSCPRemark::pack(uint8_t *buffer) {
  struct of13::ofp_meter_band_dscp_remark *bd =
      (struct of13::ofp_meter_band_dscp_remark *)buffer;
  MeterBand::pack(buffer);
  bd->prec_level = this->prec_level_;
  memset(bd->pad, 0x0, 3);
  return this->len_;
}

of_error MeterBandDSCPRemark::unpack(uint8_t *buffer) {
  struct of13::ofp_meter_band_dscp_remark *bd =
      (struct of13::ofp_meter_band_dscp_remark *)buffer;
  MeterBand::unpack(buffer);
  this->prec_level_ = bd->prec_level;
  return 0;
}

MeterBandExperimenter::MeterBandExperimenter() : experimenter_(0) {
  this->len_ = sizeof(struct of13::ofp_meter_band_experimenter);
}

MeterBandExperimenter::MeterBandExperimenter(uint32_t rate, uint32_t burst_size,
                                             uint32_t experimenter)
    : MeterBand(of13::OFPMBT_EXPERIMENTER, rate, burst_size) {
  this->experimenter_ = experimenter;
  this->len_ = sizeof(struct of13::ofp_meter_band_experimenter);
}

bool MeterBandExperimenter::equals(const MeterBand &other) {
  if (const MeterBandExperimenter *band =
          dynamic_cast<const MeterBandExperimenter *>(&other)) {
    return ((MeterBand::equals(other)) &&
            (this->experimenter_ == band->experimenter_));
  } else {
    return false;
  }
}

size_t MeterBandExperimenter::pack(uint8_t *buffer) {
  struct of13::ofp_meter_band_experimenter *be =
      (struct of13::ofp_meter_band_experimenter *)buffer;
  MeterBand::pack(buffer);
  be->experimenter = hton32(this->experimenter_);
  return this->len_;
}

of_error MeterBandExperimenter::unpack(uint8_t *buffer) {
  struct of13::ofp_meter_band_experimenter *be =
      (struct of13::ofp_meter_band_experimenter *)buffer;
  MeterBand::unpack(buffer);
  this->experimenter_ = ntoh32(be->experimenter);
  return 0;
}

MeterConfig::MeterConfig()
    : flags_(0), meter_id_(0), length_(sizeof(struct of13::ofp_meter_config)) {}

MeterConfig::MeterConfig(uint16_t flags, uint32_t meter_id)
    : flags_(flags),
      meter_id_(meter_id),
      length_(sizeof(struct of13::ofp_meter_config)) {}

MeterConfig::MeterConfig(uint16_t flags, uint32_t meter_id, MeterBandList bands)
    : bands_(bands) {
  this->flags_ = flags;
  this->meter_id_ = meter_id;
  this->length_ = sizeof(struct of13::ofp_meter_config) + bands.length();
}

bool MeterConfig::operator==(const MeterConfig &other) const {
  return ((this->flags_ == other.flags_) &&
          (this->meter_id_ == other.meter_id_) &&
          (this->bands_ == other.bands_));
}

bool MeterConfig::operator!=(const MeterConfig &other) const {
  return !(*this == other);
}

size_t MeterConfig::pack(uint8_t *buffer) {
  struct of13::ofp_meter_config *mc = (struct of13::ofp_meter_config *)buffer;
  mc->length = hton16(this->length_);
  mc->flags = hton16(this->flags_);
  mc->meter_id = hton32(this->meter_id_);
  uint8_t *p = buffer + sizeof(struct of13::ofp_meter_config);
  this->bands_.pack(p);
  return this->length_;
}

of_error MeterConfig::unpack(uint8_t *buffer) {
  struct of13::ofp_meter_config *mc = (struct of13::ofp_meter_config *)buffer;
  this->length_ = ntoh16(mc->length);
  this->flags_ = ntoh16(mc->flags);
  this->meter_id_ = ntoh32(mc->meter_id);
  this->bands_.length(this->length_ - sizeof(struct of13::ofp_meter_config));
  uint8_t *p = buffer + sizeof(struct of13::ofp_meter_config);
  this->bands_.unpack(p);
  return 0;
}

void MeterConfig::bands(MeterBandList bands) {
  this->bands_ = bands;
  this->length_ += bands.length();
}

void MeterConfig::add_band(MeterBand *band) {
  this->bands_.add_band(band);
  this->length_ += band->len();
}

MeterFeatures::MeterFeatures()
    : max_meter_(0),
      band_types_(0),
      capabilities_(0),
      max_bands_(0),
      max_color_(0) {}

MeterFeatures::MeterFeatures(uint32_t max_meter, uint32_t band_types,
                             uint32_t capabilities, uint8_t max_bands,
                             uint8_t max_color)
    : max_meter_(max_meter),
      band_types_(band_types),
      capabilities_(capabilities),
      max_bands_(max_bands),
      max_color_(max_color) {}

bool MeterFeatures::operator==(const MeterFeatures &other) const {
  return ((this->max_meter_ == other.max_meter_) &&
          (this->band_types_ == other.band_types_) &&
          (this->capabilities_ == other.capabilities_) &&
          (this->max_bands_ == other.max_bands_) &&
          (this->max_color_ == other.max_color_));
}

bool MeterFeatures::operator!=(const MeterFeatures &other) const {
  return !(*this == other);
}

size_t MeterFeatures::pack(uint8_t *buffer) {
  struct of13::ofp_meter_features *mf =
      (struct of13::ofp_meter_features *)buffer;
  mf->max_meter = hton32(this->max_meter_);
  mf->band_types = hton32(this->band_types_);
  mf->capabilities = hton32(this->capabilities_);
  mf->max_bands = this->max_bands_;
  mf->max_color = this->max_color_;
  memset(mf->pad, 0x0, 2);
  return 0;
}

of_error MeterFeatures::unpack(uint8_t *buffer) {
  struct of13::ofp_meter_features *mf =
      (struct of13::ofp_meter_features *)buffer;
  this->max_meter_ = ntoh32(mf->max_meter);
  this->band_types_ = ntoh32(mf->band_types);
  this->capabilities_ = ntoh32(mf->capabilities);
  this->max_bands_ = mf->max_bands;
  this->max_color_ = mf->max_color;
  return 0;
}

BandStats::BandStats() : packet_band_count_(0), byte_band_count_(0) {}

BandStats::BandStats(uint64_t packet_band_count, uint64_t byte_band_count) {
  this->packet_band_count_ = packet_band_count;
  this->byte_band_count_ = byte_band_count;
}

bool BandStats::operator==(const BandStats &other) const {
  return ((this->packet_band_count_ == other.packet_band_count_) &&
          (this->byte_band_count_ == other.byte_band_count_));
}

bool BandStats::operator!=(const BandStats &other) const {
  return !(*this == other);
}

size_t BandStats::pack(uint8_t *buffer) {
  struct of13::ofp_meter_band_stats *mb =
      (struct of13::ofp_meter_band_stats *)buffer;
  mb->packet_band_count = hton64(this->packet_band_count_);
  mb->byte_band_count = hton64(this->byte_band_count_);
  return 0;
}

of_error BandStats::unpack(uint8_t *buffer) {
  struct of13::ofp_meter_band_stats *mb =
      (struct of13::ofp_meter_band_stats *)buffer;
  this->packet_band_count_ = ntoh64(mb->packet_band_count);
  this->byte_band_count_ = ntoh64(mb->byte_band_count);
  return 0;
}

MeterStats::MeterStats()
    : meter_id_(0),
      flow_count_(0),
      packet_in_count_(0),
      byte_in_count_(0),
      duration_sec_(0),
      duration_nsec_(0),
      len_(sizeof(struct of13::ofp_meter_stats)) {}

MeterStats::MeterStats(uint32_t meter_id, uint32_t flow_count,
                       uint64_t packet_in_count, uint64_t byte_in_count,
                       uint32_t duration_sec, uint32_t duration_nsec)
    : meter_id_(meter_id),
      flow_count_(flow_count),
      packet_in_count_(packet_in_count),
      byte_in_count_(byte_in_count),
      duration_sec_(duration_sec),
      duration_nsec_(duration_nsec),
      len_(sizeof(struct of13::ofp_meter_stats)) {}

MeterStats::MeterStats(uint32_t meter_id, uint32_t flow_count,
                       uint64_t packet_in_count, uint64_t byte_in_count,
                       uint32_t duration_sec, uint32_t duration_nsec,
                       std::vector<BandStats> band_stats) {
  this->meter_id_ = meter_id;
  this->flow_count_ = flow_count;
  this->packet_in_count_ = packet_in_count;
  this->byte_in_count_ = byte_in_count;
  this->duration_sec_ = duration_sec;
  this->duration_nsec_ = duration_nsec;
  this->len_ = sizeof(struct of13::ofp_meter_stats) +
               band_stats.size() * sizeof(struct of13::ofp_meter_band_stats);
}

bool MeterStats::operator==(const MeterStats &other) const {
  return ((this->meter_id_ == other.meter_id_) &&
          (this->flow_count_ == other.flow_count_) &&
          (this->packet_in_count_ == other.packet_in_count_) &&
          (this->byte_in_count_ == other.byte_in_count_) &&
          (this->duration_sec_ == other.duration_sec_) &&
          (this->duration_nsec_ == other.duration_nsec_));
}

bool MeterStats::operator!=(const MeterStats &other) const {
  return !(*this == other);
}

size_t MeterStats::pack(uint8_t *buffer) {
  struct of13::ofp_meter_stats *ms = (struct of13::ofp_meter_stats *)buffer;
  ms->meter_id = hton32(this->meter_id_);
  ms->len = hton16(this->len_);
  ms->flow_count = hton32(this->flow_count_);
  ms->packet_in_count = hton64(this->packet_in_count_);
  ms->byte_in_count = hton64(this->byte_in_count_);
  ms->duration_sec = hton32(this->duration_sec_);
  ms->duration_nsec = hton32(this->duration_nsec_);
  uint8_t *p = buffer + sizeof(struct of13::ofp_meter_stats);
  for (std::vector<BandStats>::iterator it = this->band_stats_.begin();
       it != this->band_stats_.end(); ++it) {
    it->pack(p);
    p += sizeof(struct of13::ofp_meter_band_stats);
  }
  return 0;
}

of_error MeterStats::unpack(uint8_t *buffer) {
  struct of13::ofp_meter_stats *ms = (struct of13::ofp_meter_stats *)buffer;
  this->meter_id_ = ntoh32(ms->meter_id);
  this->len_ = ntoh16(ms->len);
  this->flow_count_ = hton32(ms->flow_count);
  this->packet_in_count_ = ntoh64(ms->packet_in_count);
  this->byte_in_count_ = ntoh64(ms->byte_in_count);
  this->duration_sec_ = ntoh32(ms->duration_sec);
  this->duration_nsec_ = ntoh32(ms->duration_nsec);
  uint8_t *p = buffer + sizeof(struct of13::ofp_meter_stats);
  size_t len = this->len_ - sizeof(struct of13::ofp_meter_stats);
  while (len) {
    BandStats stats;
    stats.unpack(p);
    this->band_stats_.push_back(stats);
    p += sizeof(struct of13::ofp_meter_band_stats);
    len -= sizeof(struct of13::ofp_meter_band_stats);
  }
  return 0;
}

void MeterStats::band_stats(std::vector<BandStats> band_stats) {
  this->band_stats_ = band_stats;
  this->len_ += band_stats.size() * sizeof(struct of13::ofp_meter_band_stats);
}

void MeterStats::add_band_stats(BandStats stats) {
  this->band_stats_.push_back(stats);
  this->len_ += sizeof(struct of13::ofp_meter_band_stats);
}

}  // namespace of13

}  // namespace fluid_msg

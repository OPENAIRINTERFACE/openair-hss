#ifndef OPENFLOW_METER_H
#define OPENFLOW_METER_H

#include <stddef.h>
#include <list>
#include <vector>
#include "openflow-13.h"

namespace fluid_msg {

namespace of13 {

class MeterBand {
 protected:
  uint16_t type_;
  uint16_t len_;
  uint32_t rate_;
  uint32_t burst_size_;

 public:
  MeterBand();
  MeterBand(uint16_t type, uint32_t rate, uint32_t burst_size);
  virtual ~MeterBand() {}
  virtual bool equals(const MeterBand& other);
  virtual MeterBand* clone() { return new MeterBand(*this); }
  virtual size_t pack(uint8_t* buffer);
  virtual of_error unpack(uint8_t* buffer);
  uint16_t type() { return this->type_; }
  uint16_t len() { return this->len_; }
  uint32_t rate() { return this->rate_; }
  uint32_t burst_size() { return this->burst_size_; }
  void type(uint16_t type) { this->type_ = type; }
  void rate(uint32_t rate) { this->rate_ = rate; }
  void burst_size(uint32_t burst_size) { this->burst_size_ = burst_size; }
  static bool delete_all(MeterBand* band) {
    delete band;
    return true;
  }
  static MeterBand* make_meter_band(uint16_t type);
};

class MeterBandList {
 private:
  uint16_t length_;
  std::list<MeterBand*> band_list_;

 public:
  MeterBandList() : length_(0) {}
  MeterBandList(std::list<MeterBand*> band_list);
  MeterBandList(const MeterBandList& other);
  MeterBandList& operator=(MeterBandList other);
  ~MeterBandList();
  bool operator==(const MeterBandList& other) const;
  bool operator!=(const MeterBandList& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  std::vector<MeterBand*> meter_bands() const {
    return std::vector<MeterBand*>(band_list_.begin(), band_list_.end());
  }
  friend void swap(MeterBandList& first, MeterBandList& second);
  uint16_t length() { return this->length_; }
  void add_band(MeterBand* band);
  void length(uint16_t length) { this->length_ = length; }
};

class MeterBandDrop : public MeterBand {
 public:
  MeterBandDrop();
  MeterBandDrop(uint32_t rate, uint32_t burst_size);
  ~MeterBandDrop() {}
  virtual MeterBandDrop* clone() { return new MeterBandDrop(*this); }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
};

class MeterBandDSCPRemark : public MeterBand {
 private:
  uint8_t prec_level_;

 public:
  MeterBandDSCPRemark();
  MeterBandDSCPRemark(uint32_t rate, uint32_t burst_size, uint8_t prec_level);
  ~MeterBandDSCPRemark() {}
  virtual bool equals(const MeterBand& other);
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  virtual MeterBandDSCPRemark* clone() {
    return new MeterBandDSCPRemark(*this);
  }
  uint8_t prec_level() { return this->prec_level_; }
  void prec_level(uint8_t prec_level) { this->prec_level_ = prec_level; }
};

class MeterBandExperimenter : public MeterBand {
 protected:
  uint32_t experimenter_;

 public:
  MeterBandExperimenter();
  MeterBandExperimenter(uint32_t rate, uint32_t burst_size,
                        uint32_t experimenter);
  ~MeterBandExperimenter() {}
  virtual bool equals(const MeterBand& other);
  virtual MeterBandExperimenter* clone() {
    return new MeterBandExperimenter(*this);
  }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint32_t experimenter() { return this->experimenter_; }
  void experimenter(uint32_t experimenter) {
    this->experimenter_ = experimenter;
  }
};

class MeterConfig {
 private:
  uint16_t length_;
  uint16_t flags_;
  uint32_t meter_id_;
  MeterBandList bands_;

 public:
  MeterConfig();
  MeterConfig(uint16_t flags, uint32_t meter_id);
  MeterConfig(uint16_t flags, uint32_t meter_id, MeterBandList bands);
  ~MeterConfig() {}
  bool operator==(const MeterConfig& other) const;
  bool operator!=(const MeterConfig& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint16_t length() { return this->length_; }
  uint16_t flags() { return this->flags_; }
  uint32_t meter_id() { return this->meter_id_; }
  MeterBandList bands() { return this->bands_; }
  void bands(MeterBandList bands);
  void add_band(MeterBand* band);
};

class MeterFeatures {
 private:
  uint32_t max_meter_;
  uint32_t band_types_;
  uint32_t capabilities_;
  uint8_t max_bands_;
  uint8_t max_color_;

 public:
  MeterFeatures();
  MeterFeatures(uint32_t max_meter, uint32_t band_types, uint32_t capabilities,
                uint8_t max_bands, uint8_t max_color);
  ~MeterFeatures() {}
  bool operator==(const MeterFeatures& other) const;
  bool operator!=(const MeterFeatures& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint32_t max_meter() { return this->max_meter_; }
  uint32_t band_types() { return this->band_types_; }
  uint32_t capabilities() { return this->capabilities_; }
  uint8_t max_bands() { return this->max_bands_; }
  uint8_t max_color() { return this->max_color_; }
  void max_meter(uint32_t max_meter) { this->max_meter_ = max_meter; }
  void band_types(uint32_t band_types) { this->band_types_ = band_types; }
  void capabilities(uint32_t capabilities) {
    this->capabilities_ = capabilities;
  }
  void max_bands(uint8_t max_bands) { this->max_bands_ = max_bands; }
  void max_color(uint8_t max_color) { this->max_color_ = max_color; }
};

class BandStats {
 private:
  uint64_t packet_band_count_;
  uint64_t byte_band_count_;

 public:
  BandStats();
  BandStats(uint64_t packet_band_count, uint64_t byte_band_count);
  ~BandStats() {}
  bool operator==(const BandStats& other) const;
  bool operator!=(const BandStats& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint64_t packet_band_count() { return this->packet_band_count_; }
  uint64_t byte_band_count() { return this->byte_band_count_; }
};

class MeterStats {
 private:
  uint32_t meter_id_;
  uint16_t len_;
  uint32_t flow_count_;
  uint64_t packet_in_count_;
  uint64_t byte_in_count_;
  uint32_t duration_sec_;
  uint32_t duration_nsec_;
  std::vector<BandStats> band_stats_;

 public:
  MeterStats();
  MeterStats(uint32_t meter_id, uint32_t flow_count, uint64_t packet_in_count,
             uint64_t byte_in_count, uint32_t duration_sec,
             uint32_t duration_nsec);
  MeterStats(uint32_t meter_id, uint32_t flow_count, uint64_t packet_in_count,
             uint64_t byte_in_count, uint32_t duration_sec,
             uint32_t duration_nsec, std::vector<BandStats> band_stats);
  ~MeterStats() {}
  bool operator==(const MeterStats& other) const;
  bool operator!=(const MeterStats& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint32_t meter_id() { return this->meter_id_; }
  uint16_t len() { return this->len_; }
  uint64_t packet_in_count() { return this->packet_in_count_; }
  uint64_t byte_in_count() { return this->byte_in_count_; }
  uint32_t duration_sec() { return this->duration_sec_; }
  uint32_t duration_nsec() { return this->duration_nsec_; }
  std::vector<BandStats> band_stats() { return this->band_stats_; }
  void meter_id(uint32_t meter_id) { this->meter_id_ = meter_id; }
  void packet_in_count(uint64_t packet_in_count) {
    this->packet_in_count_ = packet_in_count;
  }
  void byte_in_count(uint64_t byte_in_count) {
    this->byte_in_count_ = byte_in_count;
  }
  void duration_sec(uint32_t duration_sec) {
    this->duration_sec_ = duration_sec;
  }
  void duration_nsec(uint64_t duration_nsec) {
    this->duration_nsec_ = duration_nsec;
  }
  void band_stats(std::vector<BandStats> band_stats);

  void add_band_stats(BandStats stats);
};

}  // End of namespace of13

}  // End of namespace fluid_msg
#endif

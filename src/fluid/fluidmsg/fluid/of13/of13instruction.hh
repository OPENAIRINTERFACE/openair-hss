#ifndef OPENFLOW_INSTRUCTION_H
#define OPENFLOW_INSTRUCTION_H

#include <list>
#include <set>
#include "of13action.hh"
#include "openflow-13.h"

namespace fluid_msg {

namespace of13 {

class Instruction {
 protected:
  uint16_t type_;
  uint16_t length_;

 public:
  Instruction();
  Instruction(uint16_t type, uint16_t length);
  virtual ~Instruction() {}
  virtual bool equals(const Instruction& other);
  virtual bool operator==(const Instruction& other) const;
  virtual bool operator!=(const Instruction& other) const;
  virtual uint16_t set_order() const { return 0; }
  virtual Instruction* clone() { return new Instruction(*this); }
  virtual size_t pack(uint8_t* buffer);
  virtual of_error unpack(uint8_t* buffer);
  uint16_t type() { return this->type_; }
  uint16_t length() { return this->length_; }
  void type(uint16_t type) { this->type_ = type; }
  void length(uint16_t length) { this->length_ = length; }
  static Instruction* make_instruction(uint16_t type);
};

struct comp_inst_set_order {
  bool operator()(Instruction* lhs, Instruction* rhs) const {
    return lhs->set_order() < rhs->set_order();
  }
};

class InstructionSet {
 private:
  uint16_t length_;
  std::set<Instruction*, comp_inst_set_order> instruction_set_;

 public:
  InstructionSet() : length_(0) {}
  InstructionSet(std::set<Instruction*> instruction_set);
  InstructionSet(const InstructionSet& other);
  InstructionSet& operator=(InstructionSet other);
  ~InstructionSet();
  bool operator==(const InstructionSet& other) const;
  bool operator!=(const InstructionSet& other) const;
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  friend void swap(InstructionSet& first, InstructionSet& second);
  uint16_t length() { return this->length_; }
  std::set<Instruction*, comp_inst_set_order> instruction_set() {
    return this->instruction_set_;
  }
  void add_instruction(Instruction& inst);
  void add_instruction(Instruction* inst);
  void length(uint16_t length) { this->length_ = length; }
};

class GoToTable : public Instruction {
 private:
  uint8_t table_id_;
  const uint16_t set_order_;

 public:
  GoToTable()
      : Instruction(of13::OFPIT_GOTO_TABLE,
                    sizeof(struct of13::ofp_instruction_goto_table)),
        set_order_(60) {}
  GoToTable(uint8_t table_id);
  ~GoToTable() {}
  uint16_t set_order() const { return this->set_order_; }
  virtual bool equals(const Instruction& other);
  virtual GoToTable* clone() { return new GoToTable(*this); }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint8_t table_id() { return this->table_id_; }
  void table_id(uint8_t table_id) { this->table_id_ = table_id; }
};

class WriteMetadata : public Instruction {
 private:
  uint64_t metadata_;
  uint64_t metadata_mask_;
  const uint16_t set_order_;

 public:
  WriteMetadata()
      : Instruction(of13::OFPIT_WRITE_METADATA,
                    sizeof(struct of13::ofp_instruction_write_metadata)),
        set_order_(50) {}
  WriteMetadata(uint64_t metadata, uint64_t metadata_mask);
  ~WriteMetadata() {}
  uint16_t set_order() const { return this->set_order_; }
  virtual bool equals(const Instruction& other);
  virtual WriteMetadata* clone() { return new WriteMetadata(*this); }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint64_t metadata() { return this->metadata_; }
  uint64_t metadata_mask() { return this->metadata_mask_; }
  void metadata(uint64_t metadata) { this->metadata_ = metadata; }
  void metadata_mask(uint64_t metadata_mask) {
    this->metadata_mask_ = metadata_mask;
  }
};

class WriteActions : public Instruction {
 private:
  ActionSet actions_;
  const uint16_t set_order_;

 public:
  WriteActions()
      : Instruction(of13::OFPIT_WRITE_ACTIONS,
                    sizeof(struct of13::ofp_instruction_actions)),
        set_order_(40) {}
  WriteActions(ActionSet actions);
  ~WriteActions() {}
  uint16_t set_order() const { return this->set_order_; }
  virtual bool equals(const Instruction& other);
  size_t pack(uint8_t* buffer);
  virtual WriteActions* clone() { return new WriteActions(*this); }
  of_error unpack(uint8_t* buffer);
  ActionSet actions() { return this->actions_; }
  void actions(ActionSet actions);
  void add_action(Action& action);
  void add_action(Action* action);
};

class ApplyActions : public Instruction {
 private:
  ActionList actions_;
  const uint16_t set_order_;

 public:
  ApplyActions()
      : Instruction(of13::OFPIT_APPLY_ACTIONS,
                    sizeof(struct of13::ofp_instruction_actions)),
        set_order_(20) {}
  ApplyActions(ActionList actions);
  ~ApplyActions() {}
  uint16_t set_order() const { return this->set_order_; }
  virtual bool equals(const Instruction& other);
  virtual ApplyActions* clone() { return new ApplyActions(*this); }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  ActionList actions() { return this->actions_; }
  void actions(ActionList actions);
  void add_action(Action& action);
  void add_action(Action* action);
};

class ClearActions : public Instruction {
 private:
  const uint16_t set_order_;

 public:
  ClearActions()
      : Instruction(of13::OFPIT_CLEAR_ACTIONS,
                    sizeof(struct of13::ofp_instruction)),
        set_order_(30) {}
  ~ClearActions() {}
  uint16_t set_order() const { return this->set_order_; }
  virtual ClearActions* clone() { return new ClearActions(*this); }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
};

class Meter : public Instruction {
 private:
  uint32_t meter_id_;
  const uint16_t set_order_;

 public:
  Meter()
      : Instruction(of13::OFPIT_METER,
                    sizeof(struct of13::ofp_instruction_meter)),
        set_order_(10) {}
  Meter(uint32_t meter_id);
  ~Meter() {}
  uint16_t set_order() const { return this->set_order_; }
  virtual bool equals(const Instruction& other);
  virtual Meter* clone() { return new Meter(*this); }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint32_t meter_id() { return this->meter_id_; }
  void meter_id(uint32_t meter_id) { this->meter_id_ = meter_id; }
};

class InstructionExperimenter : public Instruction {
 protected:
  uint32_t experimenter_;

 public:
  InstructionExperimenter() {}
  InstructionExperimenter(uint32_t experimenter);
  ~InstructionExperimenter() {}
  virtual bool equals(const Instruction& other);
  virtual InstructionExperimenter* clone() {
    return new InstructionExperimenter(*this);
  }
  size_t pack(uint8_t* buffer);
  of_error unpack(uint8_t* buffer);
  uint32_t experimenter() { return this->experimenter_; }
  void experimenter(uint32_t experimenter) {
    this->experimenter_ = experimenter;
  }
};

}  // namespace of13

}  // End of namespace fluid_msg
#endif

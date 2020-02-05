#include "of13instruction.hh"

namespace fluid_msg {

namespace of13 {

Instruction::Instruction() : type_(0), length_(0) {}

Instruction::Instruction(uint16_t type, uint16_t length)
    : type_(type), length_(length) {}

bool Instruction::equals(const Instruction &other) { return (*this == other); }

bool Instruction::operator==(const Instruction &other) const {
  return ((this->type_ == other.type_) && (this->length_ == other.length_));
}

bool Instruction::operator!=(const Instruction &other) const {
  return !(*this == other);
}

InstructionSet::InstructionSet(std::set<Instruction *> instruction_set) {
  this->instruction_set_ = instruction_set_;
  for (std::set<Instruction *>::const_iterator it = instruction_set.begin();
       it != instruction_set.end(); ++it) {
    this->length_ += (*it)->length();
  }
}

InstructionSet::InstructionSet(const InstructionSet &other) {
  this->length_ = other.length_;
  for (std::set<Instruction *>::const_iterator it =
           other.instruction_set_.begin();
       it != other.instruction_set_.end(); ++it) {
    this->instruction_set_.insert((*it)->clone());
  }
}

bool InstructionSet::operator==(const InstructionSet &other) const {
  std::set<Instruction *>::const_iterator ot = other.instruction_set_.begin();
  for (std::set<Instruction *>::const_iterator it =
           this->instruction_set_.begin();
       it != this->instruction_set_.end(); ++it, ++ot) {
    if (!((*it)->equals(**ot))) {
      return false;
    }
  }
  return true;
}

bool InstructionSet::operator!=(const InstructionSet &other) const {
  return !(*this == other);
}

InstructionSet::~InstructionSet() {
  for (std::set<Instruction *>::const_iterator it =
           this->instruction_set_.begin();
       it != this->instruction_set_.end(); ++it) {
    delete *it;
  }
}

size_t InstructionSet::pack(uint8_t *buffer) {
  uint8_t *p = buffer;
  for (std::set<Instruction *>::iterator it = this->instruction_set_.begin(),
                                         end = this->instruction_set_.end();
       it != end; it++) {
    (*it)->pack(p);
    p += (*it)->length();
  }
  return 0;
}

of_error InstructionSet::unpack(uint8_t *buffer) {
  uint8_t *p = buffer;
  size_t len = this->length_;
  Instruction *inst;
  while (len) {
    uint16_t type = ntoh16(*((uint16_t *)p));
    inst = Instruction::make_instruction(type);
    inst->unpack(p);
    this->instruction_set_.insert(inst);
    len -= inst->length();
    p += inst->length();
  }
  return 0;
}

InstructionSet &InstructionSet::operator=(InstructionSet other) {
  swap(*this, other);
  return *this;
}

void swap(InstructionSet &first, InstructionSet &second) {
  std::swap(first.length_, second.length_);
  std::swap(first.instruction_set_, second.instruction_set_);
}

void InstructionSet::add_instruction(Instruction &inst) {
  Instruction *instp = inst.clone();
  this->instruction_set_.insert(instp);
  this->length_ += inst.length();
}

void InstructionSet::add_instruction(Instruction *inst) {
  this->instruction_set_.insert(inst);
  this->length_ += inst->length();
}

Instruction *Instruction::make_instruction(uint16_t type) {
  switch (type) {
    case (of13::OFPIT_GOTO_TABLE): {
      return new GoToTable();
    }
    case (of13::OFPIT_WRITE_METADATA): {
      return new WriteMetadata();
    }
    case (of13::OFPIT_CLEAR_ACTIONS): {
      return new ClearActions();
    }
    case (of13::OFPIT_WRITE_ACTIONS): {
      return new WriteActions();
    }
    case (of13::OFPIT_APPLY_ACTIONS): {
      return new ApplyActions();
    }
    case (of13::OFPIT_METER): {
      return new Meter();
    }
    case (of13::OFPIT_EXPERIMENTER): {
      return new InstructionExperimenter();
    }
  }
  return NULL;
}

size_t Instruction::pack(uint8_t *buffer) {
  struct of13::ofp_instruction *in = (struct of13::ofp_instruction *)buffer;
  in->type = hton16(this->type_);
  in->len = hton16(this->length_);
  return 0;
}

of_error Instruction::unpack(uint8_t *buffer) {
  struct of13::ofp_instruction *in = (struct of13::ofp_instruction *)buffer;
  this->type_ = ntoh16(in->type);
  this->length_ = ntoh16(in->len);
  return 0;
}

GoToTable::GoToTable(uint8_t table_id)
    : Instruction(of13::OFPIT_GOTO_TABLE,
                  sizeof(struct of13::ofp_instruction_goto_table)),
      set_order_(60) {
  this->table_id_ = table_id;
}

bool GoToTable::equals(const Instruction &other) {
  if (const GoToTable *inst = dynamic_cast<const GoToTable *>(&other)) {
    return ((Instruction::equals(other)) &&
            (this->table_id_ == inst->table_id_));
  } else {
    return false;
  }
}

size_t GoToTable::pack(uint8_t *buffer) {
  struct of13::ofp_instruction_goto_table *go =
      (struct of13::ofp_instruction_goto_table *)buffer;
  Instruction::pack(buffer);
  go->table_id = this->table_id_;
  memset(go->pad, 0x0, 3);
  return 0;
}

of_error GoToTable::unpack(uint8_t *buffer) {
  struct of13::ofp_instruction_goto_table *go =
      (struct of13::ofp_instruction_goto_table *)buffer;
  Instruction::unpack(buffer);
  this->table_id_ = go->table_id;
  return 0;
}

WriteMetadata::WriteMetadata(uint64_t metadata, uint64_t metadata_mask)
    : Instruction(of13::OFPIT_WRITE_METADATA,
                  sizeof(struct of13::ofp_instruction_write_metadata)),
      set_order_(50) {
  this->metadata_ = metadata;
  this->metadata_mask_ = metadata_mask;
}

bool WriteMetadata::equals(const Instruction &other) {
  if (const WriteMetadata *inst = dynamic_cast<const WriteMetadata *>(&other)) {
    return ((Instruction::equals(other)) &&
            (this->metadata_mask_ == inst->metadata_mask_));
  } else {
    return false;
  }
}

size_t WriteMetadata::pack(uint8_t *buffer) {
  struct of13::ofp_instruction_write_metadata *wm =
      (struct of13::ofp_instruction_write_metadata *)buffer;
  Instruction::pack(buffer);
  memset(wm->pad, 0x0, 4);
  wm->metadata = hton64(this->metadata_);
  wm->metadata_mask = hton64(this->metadata_mask_);
  return 0;
}

of_error WriteMetadata::unpack(uint8_t *buffer) {
  struct of13::ofp_instruction_write_metadata *wm =
      (struct of13::ofp_instruction_write_metadata *)buffer;
  Instruction::unpack(buffer);
  this->metadata_ = ntoh64(wm->metadata);
  this->metadata_mask_ = ntoh64(wm->metadata_mask);
  return 0;
}

WriteActions::WriteActions(ActionSet actions_)
    : Instruction(of13::OFPIT_WRITE_ACTIONS,
                  sizeof(struct of13::ofp_instruction_actions)),
      set_order_(40) {
  actions(actions_);
}

bool WriteActions::equals(const Instruction &other) {
  if (const WriteActions *inst = dynamic_cast<const WriteActions *>(&other)) {
    return ((Instruction::equals(other)) && (this->actions_ == inst->actions_));
  } else {
    return false;
  }
}

void WriteActions::actions(ActionSet actions) {
  this->actions_ = actions;
  this->length_ += actions.length();
}

void WriteActions::add_action(Action &action) {
  if (this->actions_.add_action(action)) {
    this->length_ += action.length();
  }
}

void WriteActions::add_action(Action *action) {
  if (this->actions_.add_action(action)) {
    this->length_ += action->length();
  }
}

size_t WriteActions::pack(uint8_t *buffer) {
  struct of13::ofp_instruction_actions *ia =
      (struct of13::ofp_instruction_actions *)buffer;
  Instruction::pack(buffer);
  memset(ia->pad, 0x0, 4);
  uint8_t *p = buffer + sizeof(struct of13::ofp_instruction_actions);
  this->actions_.pack(p);
  return 0;
}

of_error WriteActions::unpack(uint8_t *buffer) {
  Instruction::unpack(buffer);
  uint8_t *p = buffer + sizeof(struct of13::ofp_instruction_actions);
  this->actions_.length(this->length_ -
                        sizeof(struct of13::ofp_instruction_actions));
  this->actions_.unpack(p);
  return 0;
}

ApplyActions::ApplyActions(ActionList actions_)
    : Instruction(of13::OFPIT_APPLY_ACTIONS,
                  sizeof(struct of13::ofp_instruction_actions)),
      set_order_(20) {
  actions(actions_);
}

bool ApplyActions::equals(const Instruction &other) {
  if (const ApplyActions *inst = dynamic_cast<const ApplyActions *>(&other)) {
    return ((Instruction::equals(other)) && (this->actions_ == inst->actions_));
  } else {
    return false;
  }
}

void ApplyActions::actions(ActionList actions) {
  this->actions_ = actions;
  this->length_ += actions.length();
}

void ApplyActions::add_action(Action &action) {
  this->actions_.add_action(action);
  this->length_ += action.length();
}

void ApplyActions::add_action(Action *action) {
  this->actions_.add_action(action);
  this->length_ += action->length();
}

size_t ApplyActions::pack(uint8_t *buffer) {
  struct of13::ofp_instruction_actions *ia =
      (struct of13::ofp_instruction_actions *)buffer;
  Instruction::pack(buffer);
  memset(ia->pad, 0x0, 4);
  uint8_t *p = buffer + sizeof(struct of13::ofp_instruction_actions);
  this->actions_.pack(p);
  return 0;
}

of_error ApplyActions::unpack(uint8_t *buffer) {
  Instruction::unpack(buffer);
  uint8_t *p = buffer + sizeof(struct of13::ofp_instruction_actions);
  this->actions_.length(this->length_ -
                        sizeof(struct of13::ofp_instruction_actions));
  this->actions_.unpack13(p);
  return 0;
}

size_t ClearActions::pack(uint8_t *buffer) {
  struct of13::ofp_instruction *ia = (struct of13::ofp_instruction *)buffer;
  Instruction::pack(buffer);
  memset(ia->pad, 0x0, 4);
  return 0;
}

of_error ClearActions::unpack(uint8_t *buffer) {
  struct of13::ofp_instruction *ia = (struct of13::ofp_instruction *)buffer;
  Instruction::unpack(buffer);
  return 0;
}

Meter::Meter(uint32_t meter_id)
    : Instruction(of13::OFPIT_METER,
                  sizeof(struct of13::ofp_instruction_meter)),
      set_order_(10) {
  this->meter_id_ = meter_id;
}

bool Meter::equals(const Instruction &other) {
  if (const Meter *inst = dynamic_cast<const Meter *>(&other)) {
    return ((Instruction::equals(other)) &&
            (this->meter_id_ == inst->meter_id_));
  } else {
    return false;
  }
}

size_t Meter::pack(uint8_t *buffer) {
  struct of13::ofp_instruction_meter *im =
      (struct of13::ofp_instruction_meter *)buffer;
  Instruction::pack(buffer);
  im->meter_id = hton32(this->meter_id_);
  return 0;
}

of_error Meter::unpack(uint8_t *buffer) {
  struct of13::ofp_instruction_meter *im =
      (struct of13::ofp_instruction_meter *)buffer;
  Instruction::unpack(buffer);
  this->meter_id_ = ntoh32(im->meter_id);
  return 0;
}

InstructionExperimenter::InstructionExperimenter(uint32_t experimenter)
    : Instruction(of13::OFPIT_EXPERIMENTER,
                  sizeof(struct of13::ofp_instruction_experimenter)) {
  this->experimenter_ = experimenter;
}

bool InstructionExperimenter::equals(const Instruction &other) {
  if (const InstructionExperimenter *inst =
          dynamic_cast<const InstructionExperimenter *>(&other)) {
    return ((Instruction::equals(other)) &&
            (this->experimenter_ == inst->experimenter_));
  } else {
    return false;
  }
}

size_t InstructionExperimenter::pack(uint8_t *buffer) {
  struct of13::ofp_instruction_experimenter *ie =
      (struct of13::ofp_instruction_experimenter *)buffer;
  Instruction::pack(buffer);
  ie->experimenter = hton32(this->experimenter_);
  return 0;
}

of_error InstructionExperimenter::unpack(uint8_t *buffer) {
  struct of13::ofp_instruction_experimenter *ie =
      (struct of13::ofp_instruction_experimenter *)buffer;
  Instruction::unpack(buffer);
  this->experimenter_ = ntoh32(ie->experimenter);
  return 0;
}

}  // namespace of13

}  // End of namespace fluid_msg

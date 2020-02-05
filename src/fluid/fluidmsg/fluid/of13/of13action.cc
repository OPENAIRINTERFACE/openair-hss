#include "of13action.hh"

namespace fluid_msg {

namespace of13 {

OutputAction::OutputAction()
    : set_order_(230),
      Action(of13::OFPAT_OUTPUT, sizeof(struct of13::ofp_action_output)) {}

bool OutputAction::equals(const Action &other) {
  if (const OutputAction *act = dynamic_cast<const OutputAction *>(&other)) {
    return ((Action::equals(other)) && (this->port_ == act->port_) &&
            (this->max_len_ == act->max_len_));
  } else {
    return false;
  }
}

OutputAction::OutputAction(uint32_t port, uint16_t max_len)
    : set_order_(230),
      Action(of13::OFPAT_OUTPUT, sizeof(struct of13::ofp_action_output)) {
  this->port_ = port;
  this->max_len_ = max_len;
}

size_t OutputAction::pack(uint8_t *buffer) {
  struct of13::ofp_action_output *ao = (struct of13::ofp_action_output *)buffer;
  Action::pack(buffer);
  ao->port = hton32(this->port_);
  ao->max_len = hton16(this->max_len_);
  memset(ao->pad, 0x0, 6);
  return 0;
}

of_error OutputAction::unpack(uint8_t *buffer) {
  struct of13::ofp_action_output *ao = (struct of13::ofp_action_output *)buffer;
  Action::unpack(buffer);
  this->port_ = ntoh32(ao->port);
  this->max_len_ = ntoh16(ao->max_len);
  return 0;
}

CopyTTLInAction::CopyTTLInAction()
    : set_order_(10),
      Action(of13::OFPAT_COPY_TTL_IN, sizeof(struct ofp_action_header)) {}

size_t CopyTTLInAction::pack(uint8_t *buffer) {
  struct ofp_action_header *act = (struct ofp_action_header *)buffer;
  Action::pack(buffer);
  memset(act->pad, 0x0, 4);
  return 0;
}

of_error CopyTTLInAction::unpack(uint8_t *buffer) {
  Action::unpack(buffer);
  return 0;
}

CopyTTLOutAction::CopyTTLOutAction()
    : set_order_(110),
      Action(of13::OFPAT_COPY_TTL_OUT, sizeof(struct ofp_action_header)) {}

size_t CopyTTLOutAction::pack(uint8_t *buffer) {
  struct ofp_action_header *act = (struct ofp_action_header *)buffer;
  Action::pack(buffer);
  memset(act->pad, 0x0, 4);
  return 0;
}

of_error CopyTTLOutAction::unpack(uint8_t *buffer) {
  Action::unpack(buffer);
  return 0;
}

SetMPLSTTLAction::SetMPLSTTLAction()
    : set_order_(140),
      Action(of13::OFPAT_SET_MPLS_TTL,
             sizeof(struct of13::ofp_action_mpls_ttl)) {}

SetMPLSTTLAction::SetMPLSTTLAction(uint8_t mpls_ttl)
    : set_order_(140),
      Action(of13::OFPAT_SET_MPLS_TTL,
             sizeof(struct of13::ofp_action_mpls_ttl)) {
  this->mpls_ttl_ = mpls_ttl;
}

bool SetMPLSTTLAction::equals(const Action &other) {
  if (const SetMPLSTTLAction *act =
          dynamic_cast<const SetMPLSTTLAction *>(&other)) {
    return ((Action::equals(other)) && (this->mpls_ttl_ == act->mpls_ttl_));
  } else {
    return false;
  }
}

size_t SetMPLSTTLAction::pack(uint8_t *buffer) {
  struct of13::ofp_action_mpls_ttl *mt =
      (struct of13::ofp_action_mpls_ttl *)buffer;
  Action::pack(buffer);
  mt->mpls_ttl = this->mpls_ttl_;
  memset(mt->pad, 0x0, 3);
  return 0;
}

of_error SetMPLSTTLAction::unpack(uint8_t *buffer) {
  struct of13::ofp_action_mpls_ttl *mt =
      (struct of13::ofp_action_mpls_ttl *)buffer;
  Action::unpack(buffer);
  this->mpls_ttl_ = mt->mpls_ttl;
  return 0;
}

DecMPLSTTLAction::DecMPLSTTLAction()
    : set_order_(120),
      Action(of13::OFPAT_DEC_MPLS_TTL, sizeof(struct ofp_action_header)) {}

size_t DecMPLSTTLAction::pack(uint8_t *buffer) {
  struct ofp_action_header *act = (struct ofp_action_header *)buffer;
  Action::pack(buffer);
  memset(act->pad, 0x0, 4);
  return 0;
}

of_error DecMPLSTTLAction::unpack(uint8_t *buffer) {
  Action::unpack(buffer);
  return 0;
}

PushVLANAction::PushVLANAction()
    : set_order_(100),
      Action(of13::OFPAT_PUSH_VLAN, sizeof(struct of13::ofp_action_push)) {}

PushVLANAction::PushVLANAction(uint16_t ethertype)
    : set_order_(100),
      Action(of13::OFPAT_PUSH_VLAN, sizeof(struct of13::ofp_action_push)) {
  this->ethertype_ = ethertype;
}

bool PushVLANAction::equals(const Action &other) {
  if (const PushVLANAction *act =
          dynamic_cast<const PushVLANAction *>(&other)) {
    return ((Action::equals(other)) && (this->ethertype_ == act->ethertype_));
  } else {
    return false;
  }
}

size_t PushVLANAction::pack(uint8_t *buffer) {
  struct of13::ofp_action_push *ap = (struct of13::ofp_action_push *)buffer;
  Action::pack(buffer);
  ap->ethertype = hton16(this->ethertype_);
  memset(ap->pad, 0x0, 2);
  return 0;
}

of_error PushVLANAction::unpack(uint8_t *buffer) {
  struct of13::ofp_action_push *ap = (struct of13::ofp_action_push *)buffer;
  Action::unpack(buffer);
  this->ethertype_ = ntoh16(ap->ethertype);
  return 0;
}

PopVLANAction::PopVLANAction()
    : set_order_(30),
      Action(of13::OFPAT_POP_VLAN, sizeof(struct ofp_action_header)) {}

size_t PopVLANAction::pack(uint8_t *buffer) {
  struct ofp_action_header *act = (struct ofp_action_header *)buffer;
  Action::pack(buffer);
  memset(act->pad, 0x0, 4);
  return 0;
}

of_error PopVLANAction::unpack(uint8_t *buffer) {
  Action::unpack(buffer);
  return 0;
}

PushMPLSAction::PushMPLSAction()
    : set_order_(80),
      Action(of13::OFPAT_PUSH_MPLS, sizeof(struct ofp_action_header)) {}

PushMPLSAction::PushMPLSAction(uint16_t ethertype)
    : set_order_(80),
      Action(of13::OFPAT_PUSH_MPLS, sizeof(struct ofp_action_header)) {
  this->ethertype_ = ethertype;
}

bool PushMPLSAction::equals(const Action &other) {
  if (const PushMPLSAction *act =
          dynamic_cast<const PushMPLSAction *>(&other)) {
    return ((Action::equals(other)) && (this->ethertype_ == act->ethertype_));
  } else {
    return false;
  }
}

size_t PushMPLSAction::pack(uint8_t *buffer) {
  struct of13::ofp_action_push *ap = (struct of13::ofp_action_push *)buffer;
  Action::pack(buffer);
  ap->ethertype = hton16(this->ethertype_);
  memset(ap->pad, 0x0, 2);
  return 0;
}

of_error PushMPLSAction::unpack(uint8_t *buffer) {
  struct of13::ofp_action_push *ap = (struct of13::ofp_action_push *)buffer;
  Action::unpack(buffer);
  this->ethertype_ = ntoh16(ap->ethertype);
  return 0;
}

PopMPLSAction::PopMPLSAction()
    : set_order_(40),
      Action(of13::OFPAT_POP_MPLS, sizeof(struct of13::ofp_action_pop_mpls)) {}

PopMPLSAction::PopMPLSAction(uint16_t ethertype)
    : set_order_(40),
      Action(of13::OFPAT_POP_MPLS, sizeof(struct of13::ofp_action_pop_mpls)) {
  this->ethertype_ = ethertype;
}

bool PopMPLSAction::equals(const Action &other) {
  if (const PopMPLSAction *act = dynamic_cast<const PopMPLSAction *>(&other)) {
    return ((Action::equals(other)) && (this->ethertype_ == act->ethertype_));
  } else {
    return false;
  }
}

size_t PopMPLSAction::pack(uint8_t *buffer) {
  struct of13::ofp_action_pop_mpls *pm =
      (struct of13::ofp_action_pop_mpls *)buffer;
  Action::pack(buffer);
  pm->ethertype = hton16(this->ethertype_);
  memset(pm->pad, 0x0, 2);
  return 0;
}

of_error PopMPLSAction::unpack(uint8_t *buffer) {
  struct of13::ofp_action_pop_mpls *pm =
      (struct of13::ofp_action_pop_mpls *)buffer;
  Action::unpack(buffer);
  this->ethertype_ = ntoh16(pm->ethertype);
  return 0;
}

SetQueueAction::SetQueueAction()
    : set_order_(170),
      Action(of13::OFPAT_SET_QUEUE, sizeof(struct of13::ofp_action_set_queue)) {
}

SetQueueAction::SetQueueAction(uint32_t queue_id)
    : set_order_(170),
      Action(of13::OFPAT_SET_QUEUE, sizeof(struct of13::ofp_action_set_queue)) {
  this->queue_id_ = queue_id;
}

bool SetQueueAction::equals(const Action &other) {
  if (const SetQueueAction *act =
          dynamic_cast<const SetQueueAction *>(&other)) {
    return ((Action::equals(other)) && (this->queue_id_ == act->queue_id_));
  } else {
    return false;
  }
}

size_t SetQueueAction::pack(uint8_t *buffer) {
  struct of13::ofp_action_set_queue *aq =
      (struct of13::ofp_action_set_queue *)buffer;
  Action::pack(buffer);
  aq->queue_id = hton32(this->queue_id_);
  return 0;
}

of_error SetQueueAction::unpack(uint8_t *buffer) {
  struct of13::ofp_action_set_queue *aq =
      (struct of13::ofp_action_set_queue *)buffer;
  Action::unpack(buffer);
  this->queue_id_ = ntoh32(aq->queue_id);
  return 0;
}

GroupAction::GroupAction()
    : set_order_(220),
      Action(of13::OFPAT_GROUP, sizeof(struct of13::ofp_action_group)) {}

GroupAction::GroupAction(uint32_t group_id)
    : set_order_(220),
      Action(of13::OFPAT_GROUP, sizeof(struct of13::ofp_action_group)) {
  this->group_id_ = group_id;
}

bool GroupAction::equals(const Action &other) {
  if (const GroupAction *act = dynamic_cast<const GroupAction *>(&other)) {
    return ((Action::equals(other)) && (this->group_id_ == act->group_id_));
  } else {
    return false;
  }
}

size_t GroupAction::pack(uint8_t *buffer) {
  struct of13::ofp_action_group *ag = (struct of13::ofp_action_group *)buffer;
  Action::pack(buffer);
  ag->group_id = hton32(this->group_id_);
  return 0;
}

of_error GroupAction::unpack(uint8_t *buffer) {
  struct ofp_action_group *ag = (struct ofp_action_group *)buffer;
  Action::unpack(buffer);
  this->group_id_ = ntoh32(ag->group_id);
  return 0;
}

SetNWTTLAction::SetNWTTLAction()
    : set_order_(150),
      Action(of13::OFPAT_SET_NW_TTL, sizeof(struct of13::ofp_action_nw_ttl)) {}

SetNWTTLAction::SetNWTTLAction(uint8_t nw_ttl)
    : set_order_(150),
      Action(of13::OFPAT_SET_NW_TTL, sizeof(struct of13::ofp_action_nw_ttl)) {
  this->nw_ttl_ = nw_ttl;
}

bool SetNWTTLAction::equals(const Action &other) {
  if (const SetNWTTLAction *act =
          dynamic_cast<const SetNWTTLAction *>(&other)) {
    return ((Action::equals(other)) && (this->nw_ttl_ == act->nw_ttl_));
  } else {
    return false;
  }
}

size_t SetNWTTLAction::pack(uint8_t *buffer) {
  struct of13::ofp_action_nw_ttl *nt = (struct of13::ofp_action_nw_ttl *)buffer;
  Action::pack(buffer);
  nt->nw_ttl = this->nw_ttl_;
  memset(nt->pad, 0x0, 3);
  return 0;
}

of_error SetNWTTLAction::unpack(uint8_t *buffer) {
  struct of13::ofp_action_nw_ttl *nt = (struct of13::ofp_action_nw_ttl *)buffer;
  Action::unpack(buffer);
  this->nw_ttl_ = nt->nw_ttl;
  return 0;
}

DecNWTTLAction::DecNWTTLAction()
    : set_order_(130),
      Action(of13::OFPAT_DEC_NW_TTL, sizeof(struct ofp_action_header)) {}

size_t DecNWTTLAction::pack(uint8_t *buffer) {
  struct ofp_action_header *act = (struct ofp_action_header *)buffer;
  Action::pack(buffer);
  memset(act->pad, 0x0, 4);
  return 0;
}

of_error DecNWTTLAction::unpack(uint8_t *buffer) {
  Action::unpack(buffer);
  return 0;
}

SetFieldAction::SetFieldAction()
    : set_order_(160),
      Action(of13::OFPAT_SET_FIELD, sizeof(struct of13::ofp_action_set_field)) {

}

SetFieldAction::SetFieldAction(OXMTLV *field)
    : set_order_(160),
      Action(of13::OFPAT_SET_FIELD, sizeof(struct of13::ofp_action_set_field)) {
  this->field(field);
}

SetFieldAction::SetFieldAction(const SetFieldAction &other) : set_order_(160) {
  this->type_ = other.type_;
  this->length_ = other.length_;
  this->field_ = other.field_->clone();
}

void SetFieldAction::field(OXMTLV *field) {
  this->field_ = field;
  this->length_ += ROUND_UP(field->length(), 8);
}

SetFieldAction::~SetFieldAction() { delete this->field_; }

void swap(SetFieldAction &first, SetFieldAction &second) {
  std::swap(first.type_, second.type_);
  std::swap(first.length_, second.length_);
  std::swap(*(first.field_), *(second.field_));
}

SetFieldAction &SetFieldAction::operator=(SetFieldAction other) {
  swap(*this, other);
  return *this;
}

bool SetFieldAction::equals(const Action &other) {
  const SetFieldAction *action;
  if (const SetFieldAction *act =
          dynamic_cast<const SetFieldAction *>(&other)) {
    return ((Action::equals(other)) && (this->field_->equals(*act->field_)) &&
            (this->length_ == act->length_));
  } else {
    return false;
  }
}

OXMTLV *SetFieldAction::field() { return this->field_; }

size_t SetFieldAction::pack(uint8_t *buffer) {
  size_t padding = ROUND_UP(this->field_->length(), 8) - this->field_->length();
  Action::pack(buffer);
  this->field_->pack(buffer + (sizeof(struct of13::ofp_action_set_field) - 4));
  memset(buffer + sizeof(struct of13::ofp_action_set_field) +
             this->field_->length(),
         0x0, padding);
  return 0;
}

of_error SetFieldAction::unpack(uint8_t *buffer) {
  uint8_t *p = buffer + sizeof(struct of13::ofp_action_set_field) - 4;
  size_t padding;
  Action::unpack(buffer);
  uint32_t header = ntoh32(*((uint32_t *)p));
  this->field_ = of13::Match::make_oxm_tlv(this->field_->oxm_field(header));
  this->field_->unpack(p);
  return 0;
}

PushPBBAction::PushPBBAction()
    : set_order_(90),
      Action(of13::OFPAT_PUSH_PBB, sizeof(struct of13::ofp_action_push)) {}

PushPBBAction::PushPBBAction(uint16_t ethertype)
    : set_order_(90),
      Action(of13::OFPAT_PUSH_PBB, sizeof(struct of13::ofp_action_push)) {
  this->ethertype_ = ethertype;
}

bool PushPBBAction::equals(const Action &other) {
  if (const PushPBBAction *act = dynamic_cast<const PushPBBAction *>(&other)) {
    return ((Action::equals(other)) && (this->ethertype_ == act->ethertype_));
  } else {
    return false;
  }
}

size_t PushPBBAction::pack(uint8_t *buffer) {
  struct of13::ofp_action_push *ap = (struct of13::ofp_action_push *)buffer;
  Action::pack(buffer);
  ap->ethertype = hton16(this->ethertype_);
  memset(ap->pad, 0x0, 2);
  return 0;
}

of_error PushPBBAction::unpack(uint8_t *buffer) {
  struct of13::ofp_action_push *ap = (struct of13::ofp_action_push *)buffer;
  Action::unpack(buffer);
  this->ethertype_ = ntoh16(ap->ethertype);
  return 0;
}

PopPBBAction::PopPBBAction()
    : set_order_(50),
      Action(of13::OFPAT_POP_PBB, sizeof(struct of13::ofp_action_push)) {}

size_t PopPBBAction::pack(uint8_t *buffer) {
  struct ofp_action_header *act = (struct ofp_action_header *)buffer;
  Action::pack(buffer);
  memset(act->pad, 0x0, 4);
  return 0;
}

of_error PopPBBAction::unpack(uint8_t *buffer) {
  Action::unpack(buffer);
  return 0;
}

ExperimenterAction::ExperimenterAction()
    : Action(of13::OFPAT_EXPERIMENTER,
             sizeof(struct of13::ofp_action_experimenter_header)) {}

ExperimenterAction::ExperimenterAction(uint32_t experimenter)
    : Action(of13::OFPAT_EXPERIMENTER,
             sizeof(struct of13::ofp_action_experimenter_header)) {
  this->experimenter_ = experimenter;
}

bool ExperimenterAction::equals(const Action &other) {
  if (const ExperimenterAction *act =
          dynamic_cast<const ExperimenterAction *>(&other)) {
    return ((Action::equals(other)) &&
            (this->experimenter_ == act->experimenter_));
  } else {
    return false;
  }
}

size_t ExperimenterAction::pack(uint8_t *buffer) {
  struct ofp_action_experimenter_header *ae =
      (struct ofp_action_experimenter_header *)buffer;
  Action::pack(buffer);
  ae->experimenter = hton32(this->experimenter_);
  return 0;
}

of_error ExperimenterAction::unpack(uint8_t *buffer) {
  struct ofp_action_experimenter_header *ae =
      (struct ofp_action_experimenter_header *)buffer;
  Action::unpack(buffer);
  this->experimenter_ = ntoh32(ae->experimenter);
  return 0;
}

}  // End of namespace of13

Action *Action::make_of13_action(uint16_t type) {
  switch (type) {
    case (of13::OFPAT_OUTPUT): {
      return new of13::OutputAction();
    }
    case (of13::OFPAT_COPY_TTL_OUT): {
      return new of13::CopyTTLOutAction();
    }
    case (of13::OFPAT_COPY_TTL_IN): {
      return new of13::CopyTTLInAction();
    }
    case (of13::OFPAT_SET_MPLS_TTL): {
      return new of13::SetMPLSTTLAction();
    }
    case (of13::OFPAT_DEC_MPLS_TTL): {
      return new of13::DecMPLSTTLAction();
    }
    case (of13::OFPAT_PUSH_VLAN): {
      return new of13::PushVLANAction();
    }
    case (of13::OFPAT_POP_VLAN): {
      return new of13::PopVLANAction();
    }
    case (of13::OFPAT_PUSH_MPLS): {
      return new of13::PushMPLSAction();
    }
    case (of13::OFPAT_POP_MPLS): {
      return new of13::PopMPLSAction();
    }
    case (of13::OFPAT_SET_QUEUE): {
      return new of13::SetQueueAction();
    }
    case (of13::OFPAT_GROUP): {
      return new of13::GroupAction();
    }
    case (of13::OFPAT_SET_NW_TTL): {
      return new of13::SetNWTTLAction();
    }
    case (of13::OFPAT_DEC_NW_TTL): {
      return new of13::DecNWTTLAction();
    }
    case (of13::OFPAT_SET_FIELD): {
      return new of13::SetFieldAction();
    }
    case (of13::OFPAT_PUSH_PBB): {
      return new of13::PushPBBAction();
    }
    case (of13::OFPAT_POP_PBB): {
      return new of13::PopPBBAction();
    }
    case (of13::OFPAT_EXPERIMENTER): {
      return new of13::ExperimenterAction();
    }
  }
  return NULL;
}

}  // End of namespace fluid_msg

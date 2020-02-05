#include "action.hh"

namespace fluid_msg {

Action::Action() : type_(0), length_(0) {}

Action::Action(uint16_t type, uint16_t length) {
  this->type_ = type;
  this->length_ = length;
}

bool Action::equals(const Action &other) { return ((*this == other)); }

bool Action::operator==(const Action &other) const {
  return ((this->type_ == other.type_) && (this->length_ == other.length_));
}

bool Action::operator!=(const Action &other) const { return !(*this == other); }

size_t Action::pack(uint8_t *buffer) {
  struct ofp_action_header *ac = (struct ofp_action_header *)buffer;
  ac->type = hton16(this->type_);
  ac->len = hton16(this->length_);
  memset(ac->pad, 0x0, 4);
  return 0;
}

of_error Action::unpack(uint8_t *buffer) {
  struct ofp_action_header *ac = (struct ofp_action_header *)buffer;
  this->type_ = ntoh16(ac->type);
  this->length_ = ntoh16(ac->len);
  return 0;
}

ActionList::ActionList(std::list<Action *> action_list) {
  this->action_list_ = action_list_;
  for (std::list<Action *>::const_iterator it = action_list.begin();
       it != action_list.end(); ++it) {
    this->length_ += (*it)->length();
  }
}

ActionList::ActionList(const ActionList &other) {
  this->length_ = other.length_;
  for (std::list<Action *>::const_iterator it = other.action_list_.begin();
       it != other.action_list_.end(); ++it) {
    this->action_list_.push_back((*it)->clone());
  }
}

ActionList::~ActionList() { this->action_list_.remove_if(Action::delete_all); }

bool ActionList::operator==(const ActionList &other) const {
  std::list<Action *>::const_iterator ot = other.action_list_.begin();
  for (std::list<Action *>::const_iterator it = this->action_list_.begin();
       it != this->action_list_.end(); ++it, ++ot) {
    if (!((*it)->equals(**ot))) {
      return false;
    }
  }
  return true;
}

bool ActionList::operator!=(const ActionList &other) const {
  return !(*this == other);
}

size_t ActionList::pack(uint8_t *buffer) {
  uint8_t *p = buffer;
  for (std::list<Action *>::iterator it = this->action_list_.begin(),
                                     end = this->action_list_.end();
       it != end; ++it) {
    (*it)->pack(p);
    p += (*it)->length();
  }
  return 0;
}

of_error ActionList::unpack10(uint8_t *buffer) {
  uint8_t *p = buffer;
  size_t len = this->length_;
  Action *act;
  while (len) {
    uint16_t type = ntoh16(*((uint16_t *)p));
    act = Action::make_of10_action(type);
    act->unpack(p);
    this->action_list_.push_back(act);
    len -= act->length();
    p += act->length();
  }
  return 0;
}

of_error ActionList::unpack13(uint8_t *buffer) {
  uint8_t *p = buffer;
  size_t len = this->length_;
  Action *act;
  while (len) {
    uint16_t type = ntoh16(*((uint16_t *)p));
    act = Action::make_of13_action(type);
    act->unpack(p);
    this->action_list_.push_back(act);
    len -= act->length();
    p += act->length();
  }
  return 0;
}

ActionList &ActionList::operator=(ActionList other) {
  swap(*this, other);
  return *this;
}

void swap(ActionList &first, ActionList &second) {
  std::swap(first.length_, second.length_);
  first.action_list_.swap(second.action_list_);
}

void ActionList::add_action(Action &act) {
  Action *actn = act.clone();
  this->action_list_.push_back(actn);
  this->length_ += act.length();
}

void ActionList::add_action(Action *act) {
  this->action_list_.push_back(act);
  this->length_ += act->length();
}

ActionSet::ActionSet(std::set<Action *> action_set) {
  this->action_set_ = action_set_;
  for (std::set<Action *>::const_iterator it = action_set.begin();
       it != action_set.end(); ++it) {
    this->length_ += (*it)->length();
  }
}

ActionSet::ActionSet(const ActionSet &other) {
  this->length_ = other.length_;
  for (std::set<Action *>::const_iterator it = other.action_set_.begin();
       it != other.action_set_.end(); ++it) {
    this->action_set_.insert((*it)->clone());
  }
}

ActionSet::~ActionSet() {
  for (std::set<Action *>::const_iterator it = this->action_set_.begin();
       it != this->action_set_.end(); ++it) {
    delete *it;
  }
}

bool ActionSet::operator==(const ActionSet &other) const {
  std::set<Action *>::const_iterator ot = other.action_set_.begin();
  for (std::set<Action *>::const_iterator it = this->action_set_.begin();
       it != this->action_set_.end(); ++it, ++ot) {
    if (!((*it)->equals(**ot))) {
      return false;
    }
  }
  return true;
}

bool ActionSet::operator!=(const ActionSet &other) const {
  return !(*this == other);
}

size_t ActionSet::pack(uint8_t *buffer) {
  uint8_t *p = buffer;
  for (std::set<Action *>::iterator it = this->action_set_.begin(),
                                    end = this->action_set_.end();
       it != end; ++it) {
    (*it)->pack(p);
    p += (*it)->length();
  }
  return 0;
}

/*OpenFlow 1.0 doesn't have actions sets, so we do not
 * need to implement two unpack versions like we did for
 * the ActionList */
of_error ActionSet::unpack(uint8_t *buffer) {
  uint8_t *p = buffer;
  size_t len = this->length_;
  Action *act;
  while (len) {
    uint16_t type = ntoh16(*((uint16_t *)p));
    act = Action::make_of13_action(type);
    act->unpack(p);
    this->action_set_.insert(act);
    len -= act->length();
    p += act->length();
  }
  return 0;
}

ActionSet &ActionSet::operator=(ActionSet other) {
  swap(*this, other);
  return *this;
}

void swap(ActionSet &first, ActionSet &second) {
  std::swap(first.length_, second.length_);
  std::swap(first.action_set_, second.action_set_);
}

bool ActionSet::add_action(Action &act) {
  Action *actn = act.clone();
  return this->add_action(actn);
}

bool ActionSet::add_action(Action *act) {
  // Set items are unique.  Only update length if
  // actually inserted.
  bool added = this->action_set_.insert(act).second;
  if (added) {
    this->length_ += act->length();
  }

  return added;
}

}  // End of namespace fluid_msg

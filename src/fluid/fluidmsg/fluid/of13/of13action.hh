#ifndef OF13ACTION_H
#define OF13ACTION_H

#include "fluid/ofcommon/action.hh"
#include "of13match.hh"

namespace fluid_msg {

namespace of13 {

class OutputAction : public Action {
 private:
  uint32_t port_;
  uint16_t max_len_;
  const uint16_t set_order_;

 public:
  OutputAction();
  OutputAction(uint32_t port, uint16_t max_len);
  ~OutputAction() {}
  virtual bool equals(const Action &other);
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual OutputAction *clone() { return new OutputAction(*this); }
  uint32_t port() { return this->port_; }
  void port(uint32_t port) { this->port_ = port; }
  uint16_t max_len() { return this->max_len_; }
  void max_len(uint16_t max_len) { this->max_len_ = max_len; }
};

class CopyTTLOutAction : public Action {
 private:
  const uint16_t set_order_;

 public:
  CopyTTLOutAction();
  ~CopyTTLOutAction() {}
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual CopyTTLOutAction *clone() { return new CopyTTLOutAction(*this); }
};

class CopyTTLInAction : public Action {
 private:
  const uint16_t set_order_;

 public:
  CopyTTLInAction();
  ~CopyTTLInAction() {}
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual CopyTTLInAction *clone() { return new CopyTTLInAction(*this); }
};

class SetMPLSTTLAction : public Action {
 private:
  uint8_t mpls_ttl_;
  const uint16_t set_order_;

 public:
  SetMPLSTTLAction();
  SetMPLSTTLAction(uint8_t mpls_ttl);
  ~SetMPLSTTLAction() {}
  virtual bool equals(const Action &other);
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  uint8_t mpls_ttl() { return this->mpls_ttl_; }
  void mpls_ttl(uint8_t mpls_ttl) { this->mpls_ttl_ = mpls_ttl; }
  virtual SetMPLSTTLAction *clone() { return new SetMPLSTTLAction(*this); }
};

class DecMPLSTTLAction : public Action {
 private:
  const uint16_t set_order_;

 public:
  DecMPLSTTLAction();
  ~DecMPLSTTLAction() {}
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual DecMPLSTTLAction *clone() { return new DecMPLSTTLAction(*this); }
};

class PushVLANAction : public Action {
 private:
  uint16_t ethertype_;
  const uint16_t set_order_;

 public:
  PushVLANAction();
  PushVLANAction(uint16_t ethertype);
  ~PushVLANAction() {}
  virtual bool equals(const Action &other);
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual PushVLANAction *clone() { return new PushVLANAction(*this); }
  uint16_t ethertype() { return this->ethertype_; }
  void ethertype(uint16_t ethertype) { this->ethertype_ = ethertype; }
};

class PopVLANAction : public Action {
 private:
  const uint16_t set_order_;

 public:
  PopVLANAction();
  ~PopVLANAction() {}
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual PopVLANAction *clone() { return new PopVLANAction(*this); }
};

class PushMPLSAction : public Action {
 private:
  uint16_t ethertype_;
  const uint16_t set_order_;

 public:
  PushMPLSAction();
  PushMPLSAction(uint16_t ethertype);
  ~PushMPLSAction() {}
  virtual bool equals(const Action &other);
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual PushMPLSAction *clone() { return new PushMPLSAction(*this); }
  uint16_t ethertype() { return this->ethertype_; }
  void ethertype(uint16_t ethertype) { this->ethertype_ = ethertype; }
};

class PopMPLSAction : public Action {
 private:
  uint16_t ethertype_;
  const uint16_t set_order_;

 public:
  PopMPLSAction();
  PopMPLSAction(uint16_t ethertype);
  ~PopMPLSAction() {}
  virtual bool equals(const Action &other);
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual PopMPLSAction *clone() { return new PopMPLSAction(*this); }
  uint16_t ethertype() { return this->ethertype_; }
  void ethertype(uint16_t ethertype) { this->ethertype_ = ethertype; }
};

class SetQueueAction : public Action {
 private:
  uint32_t queue_id_;
  const uint16_t set_order_;

 public:
  SetQueueAction();
  SetQueueAction(uint32_t queue_id);
  ~SetQueueAction() {}
  virtual bool equals(const Action &other);
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual SetQueueAction *clone() { return new SetQueueAction(*this); }
  uint32_t queue_id() { return this->queue_id_; }
  void queue_id(uint32_t queue_id) { this->queue_id_ = queue_id; }
};

class GroupAction : public Action {
 private:
  uint32_t group_id_;
  const uint16_t set_order_;

 public:
  GroupAction();
  GroupAction(uint32_t group_id);
  ~GroupAction() {}
  virtual bool equals(const Action &other);
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual GroupAction *clone() { return new GroupAction(*this); }
  uint32_t group_id() { return this->group_id_; }
  void group_id(uint32_t group_id) { this->group_id_ = group_id; }
};

class SetNWTTLAction : public Action {
 private:
  uint8_t nw_ttl_;
  const uint16_t set_order_;

 public:
  SetNWTTLAction();
  SetNWTTLAction(uint8_t nw_ttl);
  ~SetNWTTLAction(){};
  virtual bool equals(const Action &other);
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual SetNWTTLAction *clone() { return new SetNWTTLAction(*this); }
  uint8_t nw_ttl() { return this->nw_ttl_; }
  void nw_ttl(uint8_t nw_ttl) { this->nw_ttl_ = nw_ttl; }
};

class DecNWTTLAction : public Action {
 private:
  const uint16_t set_order_;

 public:
  DecNWTTLAction();
  ~DecNWTTLAction() {}
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual DecNWTTLAction *clone() { return new DecNWTTLAction(*this); }
};

class SetFieldAction : public Action {
 private:
  OXMTLV *field_;
  const uint16_t set_order_;

 public:
  SetFieldAction();
  SetFieldAction(OXMTLV *field);
  SetFieldAction(const SetFieldAction &other);
  ~SetFieldAction();
  virtual bool equals(const Action &other);
  SetFieldAction &operator=(SetFieldAction other);
  uint16_t set_order() const { return this->set_order_; }
  virtual uint16_t set_sub_order() const {
    return this->field_ ? this->field_->field() : 0;
  }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual SetFieldAction *clone() { return new SetFieldAction(*this); }
  OXMTLV *field();
  void field(OXMTLV *field);
  friend void swap(SetFieldAction &first, SetFieldAction &second);
};

class PushPBBAction : public Action {
 private:
  uint16_t ethertype_;
  const uint16_t set_order_;

 public:
  PushPBBAction();
  PushPBBAction(uint16_t ethertype);
  ~PushPBBAction() {}
  virtual bool equals(const Action &other);
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual PushPBBAction *clone() { return new PushPBBAction(*this); }
  uint16_t ethertype() { return this->ethertype_; }
  void ethertype(uint16_t ethertype) { this->ethertype_ = ethertype; }
};

class PopPBBAction : public Action {
 private:
  const uint16_t set_order_;

 public:
  PopPBBAction();
  ~PopPBBAction() {}
  uint16_t set_order() const { return this->set_order_; }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual PopPBBAction *clone() { return new PopPBBAction(*this); }
};

class ExperimenterAction : public Action {
 protected:
  uint32_t experimenter_;

 public:
  ExperimenterAction();
  ExperimenterAction(uint32_t experimenter);
  ~ExperimenterAction() {}
  virtual bool equals(const Action &other);
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual ExperimenterAction *clone() { return new ExperimenterAction(*this); }
  uint32_t experimenter() { return this->experimenter_; }
  void experimenter(uint32_t experimenter) {
    this->experimenter_ = experimenter;
  }
};

}  // End of namespace of13

}  // End of namespace fluid_msg
#endif

#ifndef OF10ACTION_H
#define OF10ACTION_H

#include "fluid/ofcommon/action.hh"
#include "fluid/util/ethaddr.hh"
#include "fluid/util/ipaddr.hh"

namespace fluid_msg {

namespace of10 {

class OutputAction : public Action {
 private:
  uint16_t port_;
  uint16_t max_len_;

 public:
  OutputAction();
  OutputAction(uint16_t port, uint16_t max_len);
  ~OutputAction() {}
  OutputAction *clone() { return new OutputAction(*this); }
  virtual bool equals(const Action &other);
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  uint16_t port() { return this->port_; }
  void port(uint32_t port) { this->port_ = port; }
  uint16_t max_len() { return this->max_len_; }
  void max_len(uint16_t max_len) { this->max_len_ = max_len; }
};

class SetVLANVIDAction : public Action {
 private:
  uint16_t vlan_vid_;

 public:
  SetVLANVIDAction();
  SetVLANVIDAction(uint16_t vlan_vid);
  ~SetVLANVIDAction() {}
  virtual SetVLANVIDAction *clone() { return new SetVLANVIDAction(*this); }
  virtual bool equals(const Action &other);
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  uint16_t vlan_vid() { return this->vlan_vid_; }
  void vlan_vid(uint16_t vlan_vid) { this->vlan_vid_ = vlan_vid; }
};

class SetVLANPCPAction : public Action {
 private:
  uint8_t vlan_pcp_;

 public:
  SetVLANPCPAction();
  SetVLANPCPAction(uint8_t vlan_pcp);
  ~SetVLANPCPAction() {}
  virtual SetVLANPCPAction *clone() { return new SetVLANPCPAction(*this); }
  virtual bool equals(const Action &other);
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  uint16_t vlan_pcp() { return this->vlan_pcp_; }
  void vlan_pcp(uint8_t vlan_pcp) { this->vlan_pcp_ = vlan_pcp; }
};

class StripVLANAction : public Action {
 public:
  StripVLANAction();
  ~StripVLANAction() {}
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual StripVLANAction *clone() { return new StripVLANAction(*this); }
};

class SetDLSrcAction : public Action {
 private:
  EthAddress dl_addr_;

 public:
  SetDLSrcAction();
  SetDLSrcAction(EthAddress dl_addr);
  ~SetDLSrcAction() {}
  virtual bool equals(const Action &other);
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual SetDLSrcAction *clone() { return new SetDLSrcAction(*this); }
  EthAddress dl_addr() { return this->dl_addr_; }
  void dl_addr(const EthAddress &dl_addr) { this->dl_addr_ = dl_addr; }
};

class SetDLDstAction : public Action {
 private:
  EthAddress dl_addr_;

 public:
  SetDLDstAction();
  SetDLDstAction(EthAddress dl_addr);
  ~SetDLDstAction() {}
  virtual bool equals(const Action &other);
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual SetDLDstAction *clone() { return new SetDLDstAction(*this); }
  EthAddress dl_addr() { return this->dl_addr_; }
  void dl_addr(const EthAddress &dl_addr) { this->dl_addr_ = dl_addr; }
};

class SetNWSrcAction : public Action {
 private:
  IPAddress nw_addr_;

 public:
  SetNWSrcAction();
  SetNWSrcAction(IPAddress nw_addr);
  ~SetNWSrcAction() {}
  virtual bool equals(const Action &other);
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual SetNWSrcAction *clone() { return new SetNWSrcAction(*this); }
  IPAddress nw_addr() { return this->nw_addr_; }
  void nw_addr(const IPAddress &nw_addr) { this->nw_addr_ = nw_addr; }
};

class SetNWDstAction : public Action {
 private:
  IPAddress nw_addr_;

 public:
  SetNWDstAction();
  SetNWDstAction(IPAddress nw_addr);
  ~SetNWDstAction() {}
  virtual bool equals(const Action &other);
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual SetNWDstAction *clone() { return new SetNWDstAction(*this); }
  IPAddress nw_addr() { return this->nw_addr_; }
  void nw_addr(const IPAddress &nw_addr) { this->nw_addr_ = nw_addr; }
};

class SetNWTOSAction : public Action {
 private:
  uint8_t nw_tos_;

 public:
  SetNWTOSAction();
  SetNWTOSAction(uint8_t nw_tos);
  ~SetNWTOSAction() {}
  virtual bool equals(const Action &other);
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual SetNWTOSAction *clone() { return new SetNWTOSAction(*this); }
  uint8_t nw_tos() { return this->nw_tos_; }
  void nw_tos(uint8_t nw_tos) { this->nw_tos_ = nw_tos; }
};

class SetTPSrcAction : public Action {
 private:
  uint16_t tp_port_;

 public:
  SetTPSrcAction();
  SetTPSrcAction(uint16_t tp_port);
  ~SetTPSrcAction() {}
  virtual bool equals(const Action &other);
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual SetTPSrcAction *clone() { return new SetTPSrcAction(*this); }
  IPAddress tp_port() { return this->tp_port_; }
  void tp_port(uint16_t tp_port) { this->tp_port_ = tp_port; }
};

class SetTPDstAction : public Action {
 private:
  uint16_t tp_port_;

 public:
  SetTPDstAction();
  SetTPDstAction(uint16_t tp_port);
  ~SetTPDstAction() {}
  virtual bool equals(const Action &other);
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual SetTPDstAction *clone() { return new SetTPDstAction(*this); }
  IPAddress tp_port() { return this->tp_port_; }
  void tp_port(uint16_t tp_port) { this->tp_port_ = tp_port; }
};

class EnqueueAction : public Action {
 private:
  uint16_t port_;
  uint32_t queue_id_;

 public:
  EnqueueAction();
  EnqueueAction(uint16_t port, uint32_t queue_id);
  ~EnqueueAction() {}
  virtual bool equals(const Action &other);
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  virtual EnqueueAction *clone() { return new EnqueueAction(*this); }
  uint16_t port() { return this->port_; }
  uint32_t queue_id() { return this->queue_id_; }
  void port(uint16_t port) { this->port_ = port; }
  void queue_id(uint32_t queue_id) { this->queue_id_ = queue_id; }
};

class VendorAction : public Action {
 private:
  uint32_t vendor_;

 public:
  VendorAction();
  VendorAction(uint32_t vendor);
  ~VendorAction() {}
  virtual bool equals(const Action &other);
  virtual VendorAction *clone() { return new VendorAction(*this); }
  size_t pack(uint8_t *buffer);
  of_error unpack(uint8_t *buffer);
  IPAddress vendor() { return this->vendor_; }
  void vendor(uint32_t vendor) { this->vendor_ = vendor; }
};

}  // End of namespace of10

}  // End of namespace fluid_msg
#endif

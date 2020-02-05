#include "of10action.hh"
#include "openflow-10.h"

namespace fluid_msg {

namespace of10 {

OutputAction::OutputAction()
    : Action(of10::OFPAT_OUTPUT, sizeof(struct of10::ofp_action_output)) {}

OutputAction::OutputAction(uint16_t port, uint16_t max_len)
    : Action(of10::OFPAT_OUTPUT, sizeof(struct of10::ofp_action_output)) {
  this->port_ = port;
  this->max_len_ = max_len;
}

bool OutputAction::equals(const Action &other) {
  if (const OutputAction *act = dynamic_cast<const OutputAction *>(&other)) {
    return ((Action::equals(other)) && (this->port_ == act->port_) &&
            (this->max_len_ == act->max_len_));
  } else {
    return false;
  }
}

size_t OutputAction::pack(uint8_t *buffer) {
  struct of10::ofp_action_output *oa = (struct of10::ofp_action_output *)buffer;
  Action::pack(buffer);
  oa->port = hton16(this->port_);
  oa->max_len = hton16(this->max_len_);
  return 0;
}

of_error OutputAction::unpack(uint8_t *buffer) {
  struct of10::ofp_action_output *oa = (struct of10::ofp_action_output *)buffer;
  Action::unpack(buffer);
  this->port_ = ntoh16(oa->port);
  this->max_len_ = ntoh16(oa->max_len);
  return 0;
}

SetVLANVIDAction::SetVLANVIDAction()
    : Action(of10::OFPAT_SET_VLAN_VID,
             sizeof(struct of10::ofp_action_vlan_vid)) {}

SetVLANVIDAction::SetVLANVIDAction(uint16_t vlan_vid)
    : Action(of10::OFPAT_SET_VLAN_VID,
             sizeof(struct of10::ofp_action_vlan_vid)) {
  this->vlan_vid_ = vlan_vid;
}

bool SetVLANVIDAction::equals(const Action &other) {
  if (const SetVLANVIDAction *act =
          dynamic_cast<const SetVLANVIDAction *>(&other)) {
    return ((Action::equals(other)) && (this->vlan_vid_ == act->vlan_vid_));
  } else {
    return false;
  }
}

size_t SetVLANVIDAction::pack(uint8_t *buffer) {
  struct of10::ofp_action_vlan_vid *oa =
      (struct of10::ofp_action_vlan_vid *)buffer;
  Action::pack(buffer);
  oa->vlan_vid = hton16(this->vlan_vid_);
  memset(oa->pad, 0x0, 2);
  return 0;
}

of_error SetVLANVIDAction::unpack(uint8_t *buffer) {
  struct of10::ofp_action_vlan_vid *oa =
      (struct of10::ofp_action_vlan_vid *)buffer;
  Action::unpack(buffer);
  this->vlan_vid_ = ntoh16(oa->vlan_vid);
  return 0;
}

SetVLANPCPAction::SetVLANPCPAction()
    : Action(of10::OFPAT_SET_VLAN_PCP,
             sizeof(struct of10::ofp_action_vlan_pcp)) {}

SetVLANPCPAction::SetVLANPCPAction(uint8_t vlan_pcp)
    : Action(of10::OFPAT_SET_VLAN_PCP,
             sizeof(struct of10::ofp_action_vlan_pcp)) {
  this->vlan_pcp_ = vlan_pcp;
}

bool SetVLANPCPAction::equals(const Action &other) {
  if (const SetVLANPCPAction *act =
          dynamic_cast<const SetVLANPCPAction *>(&other)) {
    return ((Action::equals(other)) && (this->vlan_pcp_ == act->vlan_pcp_));
  } else {
    return false;
  }
}

size_t SetVLANPCPAction::pack(uint8_t *buffer) {
  struct of10::ofp_action_vlan_pcp *oa =
      (struct of10::ofp_action_vlan_pcp *)buffer;
  Action::pack(buffer);
  oa->vlan_pcp = this->vlan_pcp_;
  memset(oa->pad, 0x0, 3);
  return 0;
}

of_error SetVLANPCPAction::unpack(uint8_t *buffer) {
  struct of10::ofp_action_vlan_pcp *oa =
      (struct of10::ofp_action_vlan_pcp *)buffer;
  Action::unpack(buffer);
  this->vlan_pcp_ = oa->vlan_pcp;
  return 0;
}

StripVLANAction::StripVLANAction()
    : Action(of10::OFPAT_STRIP_VLAN, sizeof(struct of10::ofp_action_header)) {}

size_t StripVLANAction::pack(uint8_t *buffer) { return Action::pack(buffer); }

of_error StripVLANAction::unpack(uint8_t *buffer) {
  return Action::unpack(buffer);
}

SetDLSrcAction::SetDLSrcAction()
    : Action(of10::OFPAT_SET_DL_SRC, sizeof(struct of10::ofp_action_dl_addr)) {}

SetDLSrcAction::SetDLSrcAction(EthAddress dl_addr)
    : Action(of10::OFPAT_SET_DL_SRC, sizeof(struct of10::ofp_action_dl_addr)),
      dl_addr_(dl_addr) {}

bool SetDLSrcAction::equals(const Action &other) {
  if (const SetDLSrcAction *act =
          dynamic_cast<const SetDLSrcAction *>(&other)) {
    return ((Action::equals(other)) && (this->dl_addr_ == act->dl_addr_));
  } else {
    return false;
  }
}

size_t SetDLSrcAction::pack(uint8_t *buffer) {
  struct of10::ofp_action_dl_addr *oa =
      (struct of10::ofp_action_dl_addr *)buffer;
  Action::pack(buffer);
  memcpy(oa->dl_addr, this->dl_addr_.get_data(), OFP_ETH_ALEN);
  memset(oa->pad, 0x0, OFP_ETH_ALEN);
  return 0;
}

of_error SetDLSrcAction::unpack(uint8_t *buffer) {
  struct of10::ofp_action_dl_addr *oa =
      (struct of10::ofp_action_dl_addr *)buffer;
  Action::unpack(buffer);
  this->dl_addr_ = EthAddress(oa->dl_addr);
  return 0;
}

SetDLDstAction::SetDLDstAction()
    : Action(of10::OFPAT_SET_DL_DST, sizeof(struct of10::ofp_action_dl_addr)) {}

SetDLDstAction::SetDLDstAction(EthAddress dl_addr)
    : Action(of10::OFPAT_SET_DL_DST, sizeof(struct of10::ofp_action_dl_addr)),
      dl_addr_(dl_addr) {}

bool SetDLDstAction::equals(const Action &other) {
  if (const SetDLDstAction *act =
          dynamic_cast<const SetDLDstAction *>(&other)) {
    return ((Action::equals(other)) && (this->dl_addr_ == act->dl_addr_));
  } else {
    return false;
  }
}

size_t SetDLDstAction::pack(uint8_t *buffer) {
  struct of10::ofp_action_dl_addr *oa =
      (struct of10::ofp_action_dl_addr *)buffer;
  Action::pack(buffer);
  memcpy(oa->dl_addr, this->dl_addr_.get_data(), OFP_ETH_ALEN);
  memset(oa->pad, 0x0, OFP_ETH_ALEN);
  return 0;
}

of_error SetDLDstAction::unpack(uint8_t *buffer) {
  struct of10::ofp_action_dl_addr *oa =
      (struct of10::ofp_action_dl_addr *)buffer;
  Action::unpack(buffer);
  this->dl_addr_ = EthAddress(oa->dl_addr);
  return 0;
}

SetNWSrcAction::SetNWSrcAction()
    : Action(of10::OFPAT_SET_NW_SRC, sizeof(struct of10::ofp_action_nw_addr)) {}

SetNWSrcAction::SetNWSrcAction(IPAddress nw_addr)
    : Action(of10::OFPAT_SET_NW_SRC, sizeof(struct of10::ofp_action_nw_addr)),
      nw_addr_(nw_addr) {}

bool SetNWSrcAction::equals(const Action &other) {
  if (const SetNWSrcAction *act =
          dynamic_cast<const SetNWSrcAction *>(&other)) {
    return ((Action::equals(other)) && (this->nw_addr_ == act->nw_addr_));
  } else {
    return false;
  }
}

size_t SetNWSrcAction::pack(uint8_t *buffer) {
  struct of10::ofp_action_nw_addr *act =
      (struct of10::ofp_action_nw_addr *)buffer;
  Action::pack(buffer);
  act->nw_addr = hton32(this->nw_addr_.getIPv4());
  return 0;
}

of_error SetNWSrcAction::unpack(uint8_t *buffer) {
  struct of10::ofp_action_nw_addr *act =
      (struct of10::ofp_action_nw_addr *)buffer;
  Action::unpack(buffer);
  this->nw_addr_.setIPv4(ntoh32(act->nw_addr));
  return 0;
}

SetNWDstAction::SetNWDstAction()
    : Action(of10::OFPAT_SET_NW_DST, sizeof(struct of10::ofp_action_nw_addr)) {}

SetNWDstAction::SetNWDstAction(IPAddress nw_addr)
    : Action(of10::OFPAT_SET_NW_DST, sizeof(struct of10::ofp_action_nw_addr)),
      nw_addr_(nw_addr) {}

bool SetNWDstAction::equals(const Action &other) {
  if (const SetNWDstAction *act =
          dynamic_cast<const SetNWDstAction *>(&other)) {
    return ((Action::equals(other)) && (this->nw_addr_ == act->nw_addr_));
  } else {
    return false;
  }
}

size_t SetNWDstAction::pack(uint8_t *buffer) {
  struct of10::ofp_action_nw_addr *act =
      (struct of10::ofp_action_nw_addr *)buffer;
  Action::pack(buffer);
  act->nw_addr = hton32(this->nw_addr_.getIPv4());
  return 0;
}

of_error SetNWDstAction::unpack(uint8_t *buffer) {
  struct of10::ofp_action_nw_addr *act =
      (struct of10::ofp_action_nw_addr *)buffer;
  Action::unpack(buffer);
  this->nw_addr_.setIPv4(ntoh32(act->nw_addr));
  return 0;
}

SetNWTOSAction::SetNWTOSAction()
    : Action(of10::OFPAT_SET_NW_TOS, sizeof(struct of10::ofp_action_nw_tos)) {}

SetNWTOSAction::SetNWTOSAction(uint8_t nw_tos)
    : Action(of10::OFPAT_SET_NW_TOS, sizeof(struct of10::ofp_action_nw_tos)) {
  this->nw_tos_ = nw_tos;
}

bool SetNWTOSAction::equals(const Action &other) {
  if (const SetNWTOSAction *act =
          dynamic_cast<const SetNWTOSAction *>(&other)) {
    return ((Action::equals(other)) && (this->nw_tos_ == act->nw_tos_));
  } else {
    return false;
  }
}

size_t SetNWTOSAction::pack(uint8_t *buffer) {
  struct of10::ofp_action_nw_tos *oa = (struct of10::ofp_action_nw_tos *)buffer;
  Action::pack(buffer);
  oa->nw_tos = this->nw_tos_;
  memset(oa->pad, 0x0, 3);
  return 0;
}

of_error SetNWTOSAction::unpack(uint8_t *buffer) {
  struct of10::ofp_action_nw_tos *oa = (struct of10::ofp_action_nw_tos *)buffer;
  Action::unpack(buffer);
  this->nw_tos_ = oa->nw_tos;
  return 0;
}

SetTPSrcAction::SetTPSrcAction()
    : Action(of10::OFPAT_SET_TP_SRC, sizeof(struct of10::ofp_action_tp_port)) {}

SetTPSrcAction::SetTPSrcAction(uint16_t tp_port)
    : Action(of10::OFPAT_SET_TP_SRC, sizeof(struct of10::ofp_action_tp_port)) {
  this->tp_port_ = tp_port;
}

bool SetTPSrcAction::equals(const Action &other) {
  if (const SetTPSrcAction *act =
          dynamic_cast<const SetTPSrcAction *>(&other)) {
    return ((Action::equals(other)) && (this->tp_port_ == act->tp_port_));
  } else {
    return false;
  }
}

size_t SetTPSrcAction::pack(uint8_t *buffer) {
  struct of10::ofp_action_tp_port *oa =
      (struct of10::ofp_action_tp_port *)buffer;
  Action::pack(buffer);
  oa->tp_port = hton16(this->tp_port_);
  memset(oa->pad, 0x0, 2);
  return 0;
}

of_error SetTPSrcAction::unpack(uint8_t *buffer) {
  struct of10::ofp_action_tp_port *oa =
      (struct of10::ofp_action_tp_port *)buffer;
  Action::unpack(buffer);
  this->tp_port_ = ntoh16(oa->tp_port);
  return 0;
}

SetTPDstAction::SetTPDstAction()
    : Action(of10::OFPAT_SET_TP_DST, sizeof(struct of10::ofp_action_tp_port)) {}

SetTPDstAction::SetTPDstAction(uint16_t tp_port)
    : Action(of10::OFPAT_SET_TP_DST, sizeof(struct of10::ofp_action_tp_port)) {
  this->tp_port_ = tp_port;
}

bool SetTPDstAction::equals(const Action &other) {
  if (const SetTPDstAction *act =
          dynamic_cast<const SetTPDstAction *>(&other)) {
    return ((Action::equals(other)) && (this->tp_port_ == act->tp_port_));
  } else {
    return false;
  }
}

size_t SetTPDstAction::pack(uint8_t *buffer) {
  struct of10::ofp_action_tp_port *oa =
      (struct of10::ofp_action_tp_port *)buffer;
  Action::pack(buffer);
  oa->tp_port = hton16(this->tp_port_);
  memset(oa->pad, 0x0, 2);
  return 0;
}

of_error SetTPDstAction::unpack(uint8_t *buffer) {
  struct of10::ofp_action_tp_port *oa =
      (struct of10::ofp_action_tp_port *)buffer;
  Action::unpack(buffer);
  this->tp_port_ = ntoh16(oa->tp_port);
  return 0;
}

EnqueueAction::EnqueueAction()
    : Action(of10::OFPAT_ENQUEUE, sizeof(struct of10::ofp_action_enqueue)) {}

EnqueueAction::EnqueueAction(uint16_t port, uint32_t queue_id)
    : Action(of10::OFPAT_ENQUEUE, sizeof(struct of10::ofp_action_enqueue)) {
  this->port_ = port;
  this->queue_id_ = queue_id;
}

bool EnqueueAction::equals(const Action &other) {
  if (const EnqueueAction *act = dynamic_cast<const EnqueueAction *>(&other)) {
    return ((Action::equals(other)) && (this->port_ == act->port_) &&
            (this->queue_id_ == act->queue_id_));
  } else {
    return false;
  }
}

size_t EnqueueAction::pack(uint8_t *buffer) {
  struct of10::ofp_action_enqueue *oa =
      (struct of10::ofp_action_enqueue *)buffer;
  Action::pack(buffer);
  oa->port = hton16(this->port_);
  memset(oa->pad, 0x0, 6);
  oa->queue_id = hton32(this->queue_id_);
  return 0;
}

of_error EnqueueAction::unpack(uint8_t *buffer) {
  struct of10::ofp_action_enqueue *oa =
      (struct of10::ofp_action_enqueue *)buffer;
  Action::unpack(buffer);
  this->port_ = ntoh16(oa->port);
  this->queue_id_ = ntoh32(oa->queue_id);
  return 0;
}

VendorAction::VendorAction()
    : Action(of10::OFPAT_VENDOR,
             sizeof(struct of10::ofp_action_vendor_header)) {}

VendorAction::VendorAction(uint32_t vendor)
    : Action(of10::OFPAT_VENDOR,
             sizeof(struct of10::ofp_action_vendor_header)) {
  this->vendor_ = vendor;
}

bool VendorAction::equals(const Action &other) {
  if (const VendorAction *act = dynamic_cast<const VendorAction *>(&other)) {
    return ((Action::equals(other)) && (this->vendor_ == act->vendor_));
  } else {
    return false;
  }
}

size_t VendorAction::pack(uint8_t *buffer) {
  struct of10::ofp_action_vendor_header *oa =
      (struct of10::ofp_action_vendor_header *)buffer;
  Action::pack(buffer);
  oa->vendor = hton32(this->vendor_);
  return 0;
}

of_error VendorAction::unpack(uint8_t *buffer) {
  struct of10::ofp_action_vendor_header *oa =
      (struct of10::ofp_action_vendor_header *)buffer;
  Action::unpack(buffer);
  this->vendor_ = ntoh32(oa->vendor);
  return 0;
}

}  // End of namespace of10

Action *Action::make_of10_action(uint16_t type) {
  switch (type) {
    case (of10::OFPAT_OUTPUT): {
      return new of10::OutputAction();
    }
    case (of10::OFPAT_SET_VLAN_VID): {
      return new of10::SetVLANVIDAction();
    }
    case (of10::OFPAT_SET_VLAN_PCP): {
      return new of10::SetVLANPCPAction();
    }
    case (of10::OFPAT_STRIP_VLAN): {
      return new of10::StripVLANAction();
    }
    case (of10::OFPAT_SET_DL_SRC): {
      return new of10::SetDLSrcAction();
    }
    case (of10::OFPAT_SET_DL_DST): {
      return new of10::SetDLDstAction();
    }
    case (of10::OFPAT_SET_NW_SRC): {
      return new of10::SetNWSrcAction();
    }
    case (of10::OFPAT_SET_NW_DST): {
      return new of10::SetNWDstAction();
    }
    case (of10::OFPAT_SET_NW_TOS): {
      return new of10::SetNWTOSAction();
    }
    case (of10::OFPAT_SET_TP_SRC): {
      return new of10::SetTPSrcAction();
    }
    case (of10::OFPAT_SET_TP_DST): {
      return new of10::SetTPDstAction();
    }
    case (of10::OFPAT_ENQUEUE): {
      return new of10::EnqueueAction();
    }
    case (of10::OFPAT_VENDOR): {
      return new of10::VendorAction();
    }
  }
  return NULL;
}

}  // End of namespace fluid_msg

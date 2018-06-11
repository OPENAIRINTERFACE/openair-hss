/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#pragma once

#include <arpa/inet.h>
#include <fluid/OFServer.hh>
#include <fluid/ofcommon/openflow-common.hh>
#include "pgw_pcef_emulation.h"

using namespace fluid_msg;

namespace openflow {

enum ControllerEventType {
  EVENT_PACKET_IN,
  EVENT_SWITCH_DOWN,
  EVENT_SWITCH_UP,
  EVENT_ERROR,
  EVENT_ADD_GTP_TUNNEL,
  EVENT_DELETE_GTP_TUNNEL,
  EVENT_ADD_SDF_FILTER,
  EVENT_DELETE_SDF_FILTER
};

/**
 * Superclass for all controller events. These classes are used to pass info
 * like the connection from the controller or any external source to the
 * application
 */
class ControllerEvent {
public:
  ControllerEvent(
    fluid_base::OFConnection* ofconn,
    const ControllerEventType type);

  virtual ~ControllerEvent() {}

  fluid_base::OFConnection* get_connection() const;

  const ControllerEventType get_type() const;

private:
  const ControllerEventType type_;
protected:
  fluid_base::OFConnection* ofconn_;
};

/**
 * Superclass for any event that gets data passed in through the event
 */
class DataEvent : public ControllerEvent {
public:
  DataEvent(
    fluid_base::OFConnection* ofconn,
    fluid_base::OFHandler& ofhandler,
    const void* data,
    const size_t len,
    const ControllerEventType type);

  ~DataEvent();

  const uint8_t* get_data() const;
  const size_t get_length() const;

private:
  fluid_base::OFHandler& ofhandler_;
  const uint8_t* data_;
  const size_t len_;
};

/**
 * Event triggered when a packet gets pushed to user space
 */
class PacketInEvent : public DataEvent {
public:
  PacketInEvent(
    fluid_base::OFConnection* ofconn,
    fluid_base::OFHandler& ofhandler,
    const void* data,
    const size_t len);
};

/**
 * Event triggered when the controller connects with the switch
 */
class SwitchUpEvent : public DataEvent {
public:
  SwitchUpEvent(
    fluid_base::OFConnection* ofconn,
    fluid_base::OFHandler& ofhandler,
    const void* data,
    const size_t len);
};

/**
 * Event triggered when the controller loses connection with the switch
 */
class SwitchDownEvent : public ControllerEvent {
public:
  SwitchDownEvent(fluid_base::OFConnection* ofconn);
};

/**
 * Event triggered when there is an openflow error reported from the switch
 */
class ErrorEvent : public ControllerEvent {
public:
  ErrorEvent(
    fluid_base::OFConnection* ofconn,
    const struct ofp_error_msg* error_msg);

  const uint16_t get_error_type() const;
  const uint16_t get_error_code() const;

private:
  const uint16_t error_type_;
  const uint16_t error_code_;
};

/*
 * Event triggered externally, so it allows for delayed assignment of the
 * openflow connection. This way, the controller can set the latest known
 * connection, instead of an external file
 */
class ExternalEvent : public ControllerEvent {
public:
  ExternalEvent(const ControllerEventType type);

  void set_of_connection(fluid_base::OFConnection* ofconn);
};

/*
 * Event triggered by SPGW to add a GTP tunnel for a UE
 */
class AddGTPTunnelEvent : public ExternalEvent {
public:
  AddGTPTunnelEvent(
    const struct in_addr ue_ip,
    const struct in_addr enb_ip,
    const uint32_t in_tei,
    const uint32_t out_tei,
    const char* imsi,
    const pcc_rule_t *const rule);

    const struct in_addr& get_ue_ip() const;
    const struct in_addr& get_enb_ip() const;
    const uint32_t get_in_tei() const;
    const uint32_t get_out_tei() const;
    const std::string& get_imsi() const;
    const pcc_rule_t *const  get_rule() const;

private:
  const struct in_addr ue_ip_;
  const struct in_addr enb_ip_;
  const uint32_t in_tei_;
  const uint32_t out_tei_;
  const std::string imsi_;
  const pcc_rule_t *const rule_;
};

/*
 * Event triggered by SPGW to remove a GTP tunnel for a UE on detach
 */
class DeleteGTPTunnelEvent : public ExternalEvent {
public:
  DeleteGTPTunnelEvent(
    const struct in_addr ue_ip,
    const uint32_t in_tei,
    const pcc_rule_t *const rule);

    const struct in_addr& get_ue_ip() const;
    const uint32_t get_in_tei() const;
    const pcc_rule_t *const  get_rule() const;

private:
  const struct in_addr ue_ip_;
  const uint32_t in_tei_;
  const pcc_rule_t *const rule_;
};

}

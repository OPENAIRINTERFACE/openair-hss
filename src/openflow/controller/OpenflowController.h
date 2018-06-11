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

#include  <memory>
#include <unordered_map>
#include <list>

#include <fluid/OFServer.hh>

#include "ControllerEvents.h"
#include "OpenflowMessenger.h"

namespace openflow {

#define IP_ETH_TYPE 0x0800
#define ARP_ETH_TYPE 0x0806
#define ARPOP_REQUEST   1               /* ARP request                  */
#define ARPOP_REPLY     2               /* ARP reply                    */

#define MAX_UE_POOL_PDN 96

#define OF_TABLE_SWITCH                 0
#define OF_TABLE_ARP                    1
#define OF_TABLE_UL_GTPU                2
#define OF_TABLE_LOOP                  (OF_TABLE_UL_GTPU + MAX_UE_POOL_PDN)
// should be + MAX_UE_POOLS
#define OF_TABLE_DL_GTPU               (OF_TABLE_LOOP + 1)
#define OF_TABLE_PAGING_POOL            OF_TABLE_DL_GTPU
#define OF_TABLE_PAGING_UE_IN_PROGRESS  OF_TABLE_PAGING_POOL
#define OF_TABLE_FINAL                  OF_TABLE_PAGING_UE_IN_PROGRESS+MAX_UE_POOL_PDN

enum OF_FLOW_PRIORITIES {
  OF_PRIO_SWITCH_LOWER_PRIORITY = 0,
  OF_PRIO_LOOP_LOWER_PRIORITY = 0,
  OF_PRIO_SWITCH_GOTO_ARP_TABLE = 1,
  OF_PRIO_SWITCH_GOTO_DL_GTP_TABLE = 11,
  OF_PRIO_SWITCH_GOTO_UL_GTP_TABLE = 10,
  OF_PRIO_ARP_UE_POOL = 5,
  OF_PRIO_ARP_IF = 6,
  OF_PRIO_PAGING_UE_POOL = 5,
  OF_PRIO_PAGING_UE_IN_PROGRESS = 6,
  OF_PRIO_GTPU     = 10,
};

class Application {
public:
  virtual void event_callback(const ControllerEvent& ev,
                              const OpenflowMessenger& messenger) {}
  virtual ~Application() {}
};

enum OF_MESSAGE_TYPES {
  OFPT_ERROR = 1,
  OFPT_FEATURES_REPLY_TYPE = 6,
  OFPT_PACKET_IN_TYPE = 10
};

class OpenflowController : public fluid_base::OFServer {
public:
  static const uint8_t OF_13_VERSION = 4;
public:
  OpenflowController(
    const char* address,
    const int port,
    const int n_workers,
    bool secure);

  /*
   * Used to specify the specific class to send openflow messages, for instance
   * during testing
   */
  OpenflowController(
    const char* address,
    const int port,
    const int n_workers,
    bool secure,
    std::shared_ptr<OpenflowMessenger> messenger);

  /**
   * Remove all table 0 flows on connection. Right now, all applications
   * modify table 0, so this method is owned by the controller and not the app
   */
  void initialize_on_connection(fluid_base::OFConnection* ofconn);

  /**
   * Callback for any messages like PACKET_IN. Parameters are set by super
   * class OFServer
   */
  void message_callback(
    fluid_base::OFConnection* ofconn,
    uint8_t type,
    void* data,
    size_t len);

  /**
   * Callback for any new/removed connections. Parameters are set by super
   * class OFServer
   */
  void connection_callback(
      fluid_base::OFConnection* ofconn,
      fluid_base::OFConnection::Event type);

  /**
   * Register an application to get called when an event type happens. For
   * example, you could trigger a callback in an application when a packet in
   * occurs
   *
   * @param app (in) - Application subclass the event should be registered to
   * @param event_type - ControllerEventType representing what event it is.
   *                     Should be consistent across controller and application
   */
  void register_for_event(Application* app, ControllerEventType event_type);

  /**
   * Stop the controller from running
   */
  void stop();

  /**
   * Send an event to all applications. This can be used outside of the event
   * loop as well to trigger external events.
   *
   * @param ev - reference to ControllerEvent subclass that just occurred
   */
  void dispatch_event(const ControllerEvent& ev);

  /**
   * This function can be called by another thread to inject an external event
   * into the main event loop. This can be used for non-standard openflow events
   * like adding a gtp tunnel flow
   * @param ev - shared_ptr to ExternalEvent subclass that is to be handled by
   *             the event loop. This needs to be a pointer because it will be
   *             handled indirectly by another thread.
   * @param cb - A callback function to be called by the event loop when the
   *             event is handled.
   *
   */
   void inject_external_event(
       std::shared_ptr<ExternalEvent> ev,
       void* (*cb)(std::shared_ptr<void>));

private:
  std::shared_ptr<OpenflowMessenger> messenger_;
  std::unordered_map<uint32_t, std::vector<Application*>> event_listeners;
  bool running_;
  fluid_base::OFConnection* latest_ofconn_;
};

}

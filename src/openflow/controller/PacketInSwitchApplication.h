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
#include <unordered_map>
#include <mutex>

#include "OpenflowController.h"

namespace openflow {
#define ETH_HEADER_LENGTH 14

class PacketInApplication : public Application {
public:
  virtual void packet_in_callback(const PacketInEvent& pin_ev,
      of13::PacketIn& ofpi,
      const OpenflowMessenger& messenger) = 0;
  virtual ~PacketInApplication() {};
};

class PacketInSwitchApplication: public Application {

public:
  PacketInSwitchApplication(void);

  void register_for_cookie(
      PacketInApplication* app,
      uint64_t cookie);

  uint64_t generate_cookie(void);

private:

  /**
   * Main callback event required by inherited Application class. Whenever
   * the controller gets an event like packet in or switch up, it will pass
   * it to the application here
   *
   * @param ev (in) - pointer to some subclass of ControllerEvent that occurred
   */
  virtual void event_callback(const ControllerEvent& ev,
                              const OpenflowMessenger& messenger);


private:
  std::unordered_map<uint64_t, PacketInApplication*> packet_in_event_listeners;
  uint64_t uid_cookie;
  std::mutex    muid;
};

}

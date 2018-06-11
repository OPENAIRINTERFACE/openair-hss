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

#include "OpenflowController.h"
#include "PacketInSwitchApplication.h"

extern "C" {
  #include "log.h"
}

using namespace fluid_msg;

namespace openflow {

PacketInSwitchApplication::PacketInSwitchApplication(void)
  : uid_cookie(0) {}

void PacketInSwitchApplication::event_callback(const ControllerEvent& ev,
                                       const OpenflowMessenger& messenger) {
  if (ev.get_type() == EVENT_PACKET_IN) {
    const PacketInEvent& pi_ev = static_cast<const PacketInEvent&>(ev);
    of13::PacketIn ofpi;
    ofpi.unpack(const_cast<uint8_t*>(pi_ev.get_data()));
    OAILOG_DEBUG(LOG_GTPV1U, "Handling packet-in message in PacketInSwitchApplication, cookie %ld\n", ofpi.cookie());

    PacketInApplication* app = packet_in_event_listeners[ofpi.cookie()];
    if (app) {
      app->packet_in_callback(pi_ev, ofpi, messenger);
    }
  }
}


void PacketInSwitchApplication::register_for_cookie(
    PacketInApplication* app,
    uint64_t cookie) {
  OAILOG_DEBUG(LOG_GTPV1U, "Register application with cookie %ld\n", cookie);
  packet_in_event_listeners[cookie] = app;
}


uint64_t PacketInSwitchApplication::generate_cookie(void) {
  std::unique_lock<std::mutex> lck {muid};
  uid_cookie++;
  return uid_cookie;
}

}

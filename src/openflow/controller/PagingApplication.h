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

#include <memory>

#include "OpenflowController.h"
#include "PacketInSwitchApplication.h"

namespace openflow {
#define ETH_HEADER_LENGTH 14

#define UNCONFIRMED_CLAMPING_TIMEOUT 30

class PagingApplication: public PacketInApplication {

public:
  PagingApplication(PacketInSwitchApplication& pin_sw_app);

private:

  virtual void packet_in_callback(const PacketInEvent& pin_ev,
      of13::PacketIn& ofpi,
      const OpenflowMessenger& messenger);


  /**
   * Main callback event required by inherited Application class. Whenever
   * the controller gets an event like packet in or switch up, it will pass
   * it to the application here
   *
   * @param ev (in) - pointer to some subclass of ControllerEvent that occurred
   */
  virtual void event_callback(const ControllerEvent& ev,
                              const OpenflowMessenger& messenger);

  void clamp_dl_data_notification(
      fluid_base::OFConnection* ofconn,
      const OpenflowMessenger& messenger,
      const struct in_addr ue_ip,
      const int pool_id,
      const uint16_t clamp_time_out);

  /**
   * Handles downlink data intended for a UE in idle mode, then forwards the
   * paging request to SPGW. After initiating the paging process, it also clamps
   * on the destination IP, to prevent multiple packet-in messages
   *
   * @param ofconn (in) - given connection to OVS switch
   * @param data (in) - the ethernet packet received by the switch
   */
  void trigger_dl_data_notification(fluid_base::OFConnection* ofconn, uint8_t* data,
                             const OpenflowMessenger& messenger);

  /**
   * Creates the default paging flow, which sends a packet intended for an
   * idle UE to this application
   */
  void install_default_flow(fluid_base::OFConnection* ofconn,
                            const OpenflowMessenger& messenger);
  void install_test_flow(fluid_base::OFConnection* ofconn,
                            const OpenflowMessenger& messenger);

  PacketInSwitchApplication& pin_sw_app_;
};

}

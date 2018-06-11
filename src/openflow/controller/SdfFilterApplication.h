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

#include <gmp.h> // gross but necessary to link spgw_config.h

#include "OpenflowController.h"

namespace openflow {

/**
 * SdfFilterApplication handles external callbacks to add/delete SDF filters
 **/
class SdfFilterApplication: public Application {
public:
  SdfFilterApplication(uint32_t sgi_port_num);
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

  /*
   * Add uplink flow from UE to internet
   * @param ev - AddGTPTunnelEvent containing ue ip, enb ip, and tunnel id's
   */
  void add_sdf_filter(const AddSdfFilterEvent& ev,
                              const OpenflowMessenger& messenger);


  /*
   * Remove downlink tunnel flow on disconnect
   * @param ev - DeleteGTPTunnelEvent containing ue ip, and inbound tei
   */
  void delete_sdf_filter(const DeleteSdfFilterEvent& ev,
                                   const OpenflowMessenger& messenger);

private:
  static const uint32_t DEFAULT_PRIORITY = 100;
  static const uint16_t TABLE = 0;
  static const uint16_t NEXT_TABLE = 1;

  const uint32_t sgi_port_num_;
};

}

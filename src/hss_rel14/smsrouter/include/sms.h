/*
 * Copyright (c) 2017 Sprint
 *
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the terms found in the LICENSE file in the root of this source tree.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SMSROUTER_H
#define __SMSROUTER_H

#include "s6c_impl.h"
#include "sgd_impl.h"

class SMSRouter {
 public:
  SMSRouter();
  ~SMSRouter();

  bool init();
  void uninit();

  void waitForShutdown();

  s6c::Application& s6cApp() { return *m_s6c; }
  sgd::Application& sgdApp() { return *m_sgd; }

 private:
  FDEngine m_diameter;

  s6c::Application* m_s6c;
  sgd::Application* m_sgd;
  bool m_repetitive;
};

#endif  // #define __SMSROUTER_H

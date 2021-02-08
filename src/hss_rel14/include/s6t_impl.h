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

#ifndef __S6T_IMPL_H
#define __S6T_IMPL_H

#ifdef __cplusplus

#include "s6t.h"

class DataAccess;

namespace s6t {

// Member functions that customize the individual application
class Application : public ApplicationBase {
  friend COIRreq;
  friend REIRreq;
  friend NIIRreq;

 public:
  Application(DataAccess& dbobj);
  ~Application();

  COIRcmd& getCOIRcmd() { return m_cmd_coir; }
  // REIRcmd &getREIRcmd() { return m_cmd_reir; }
  NIIRcmd& getNIIRcmd() { return m_cmd_niir; }

  // Parameters for sendXXXreq, if present below, may be changed
  // based upon processing needs
  bool sendCOIRreq(FDPeer& peer);
  bool sendREIRreq(FDPeer& peer);
  bool sendNIIRreq(FDPeer& peer);

  DataAccess& getDbObj();

 private:
  void registerHandlers();
  COIRcmd m_cmd_coir;
  // REIRcmd m_cmd_reir;
  NIIRcmd m_cmd_niir;

  // the parameters for createXXXreq, if present below, may be
  // changed based processing needs
  COIRreq* createCOIRreq(FDPeer& peer);
  REIRreq* createREIRreq(FDPeer& peer);
  NIIRreq* createNIIRreq(FDPeer& peer);

  DataAccess& m_dbobj;
};

}  // namespace s6t

#endif
#endif  // __S6T_IMPL_H

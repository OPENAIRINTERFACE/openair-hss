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

#ifndef __SSYSLOG_H
#define __SSYSLOG_H

#include <syslog.h>
#include <string>
#include <cstdarg>
#include "ssync.h"

class SSysLog {
 public:
  SSysLog(const std::string& identity);
  SSysLog(const std::string& identity, int option);
  SSysLog(const std::string& identity, int option, int facility);

  ~SSysLog();

  std::string& getIdentity() { return m_ident; }
  std::string& setIdentity(std::string& v) {
    m_ident = v;
    return getIdentity();
  }
  int getOption() { return m_option; }
  int setOption(int v) {
    m_option = v;
    return getOption();
  }
  int getFacility() { return m_facility; }
  int setFacility(int v) {
    m_facility = v;
    return getFacility();
  }

  void syslog(int priority, const char* format, ...);
  void syslogs(const std::string& val);

 protected:
 private:
  SSysLog();

  void openSysLog();
  void closeSysLog();

  std::string m_ident;
  int m_option;
  int m_facility;
  bool m_isopen;

  SMutex m_mutex;
};

#endif  //#define __SSYSLOG_H

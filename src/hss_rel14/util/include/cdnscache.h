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

#ifndef __CDNSCACHE_H
#define __CDNSCACHE_H

#include "cdnsquery.h"
#include "ssync.h"

namespace CachedDNS {
extern "C" typedef void (*CachedDNSQueryCallback)(
    Query* q, bool cacheHit, void* data);

class QueryProcessor;

class Cache {
  friend QueryProcessor;

 public:
  static Cache& getInstance();

  Query* query(ns_type rtype, const std::string& domain, bool& cacheHit);
  void query(
      ns_type rtype, const std::string& domain, CachedDNSQueryCallback cb,
      void* data = NULL);

 protected:
  Query* processQuery(ns_type rtype, const std::string& domain);

 private:
  Cache();
  ~Cache();

  Query* lookupQuery(ns_type rtype, const std::string& domain);
  bool processQuery(
      ns_type rtype, const std::string& domain, CachedDNSQueryCallback cb);

  static void ares_callback(
      void* arg, int status, int timeouts, unsigned char* abuf, int alen);

  QueryCache m_cache;
  SMutex m_cachemutex;
};
}  // namespace CachedDNS

#endif  // #ifndef __CDNSCACHE_H

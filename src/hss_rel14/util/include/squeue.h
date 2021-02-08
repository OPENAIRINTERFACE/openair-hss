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

#ifndef __SQUEUE_H
#define __SQUEUE_H

#include <queue>

#include "ssync.h"

class SQueueMessage {
 public:
  SQueueMessage(uint16_t id);
  virtual ~SQueueMessage();

  uint16_t getId() { return m_id; }
  uint16_t setId(uint16_t id) { return m_id = id; }

 private:
  SQueueMessage();

  uint16_t m_id;
};

class SQueue {
 public:
  SQueue();
  ~SQueue();

  bool push(uint16_t msgid, bool wait = true);
  bool push(SQueueMessage* msg, bool wait = true);

  SQueueMessage* pop(bool wait = true);

 private:
  SMutex m_mutex;
  SSemaphore m_sem;
  std::queue<SQueueMessage*> m_queue;
};

#endif  // #define __SQUEUE_H

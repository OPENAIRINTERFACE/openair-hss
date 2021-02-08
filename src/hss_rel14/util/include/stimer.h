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

#ifndef __STIMER_H
#define __STIMER_H

typedef long long int stime_t;

class STimerElapsed {
 public:
  STimerElapsed();
  STimerElapsed(STimerElapsed& a);
  STimerElapsed(stime_t t);
  ~STimerElapsed();

  void Start();
  void Stop();
  void Set(stime_t a);
  stime_t MilliSeconds(bool bRestart = false);
  stime_t MicroSeconds(bool bRestart = false);

  STimerElapsed& operator=(STimerElapsed& a);
  STimerElapsed& operator=(stime_t t);

  operator stime_t() { return _time; }

 private:
  stime_t _time;
  stime_t _endtime;
};

#endif  // #define __STIMER_H

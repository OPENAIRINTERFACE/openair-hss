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

#ifndef __TIMER_H
#define __TIMER_H

#include <time.h>

typedef long long int stimer_t;

#define STIMER_GET_CURRENT_TP(__now__)                                         \
  ({                                                                           \
    struct timespec __ts__;                                                    \
    __now__ = clock_gettime(CLOCK_REALTIME, &__ts__) ?                         \
                  -1 :                                                         \
                  (((stimer_t) __ts__.tv_sec) * 1000000000) +                  \
                      ((stimer_t) __ts__.tv_nsec);                             \
    __now__;                                                                   \
  })

#define STIMER_GET_ELAPSED_NS(_start_)                                         \
  ({                                                                           \
    stimer_t __ns__;                                                           \
    STIMER_GET_CURRENT_TP(__ns__);                                             \
    if (__ns__ != -1) __ns__ -= _start_;                                       \
    __ns__;                                                                    \
  })

#define STIMER_GET_ELAPSED_US(__start__)                                       \
  ({                                                                           \
    stimer_t __us__ = STIMER_GET_ELAPSED_NS(__start__);                        \
    if (__us__ != -1)                                                          \
      __us__ = (__us__ / 1000) + (__us__ % 1000 >= 500 ? 1 : 0);               \
    __us__;                                                                    \
  })

#define STIMER_GET_ELAPSED_MS(___start___)                                     \
  ({                                                                           \
    stimer_t __ms__ = STIMER_GET_ELAPSED_NS(__start__);                        \
    if (__ms__ != -1)                                                          \
      __ms__ = (__ms__ / 1000000) + (__ms__ % 1000000 >= 500000 ? 1 : 0);      \
    __ms__;                                                                    \
  })

#define STIMER_GET_CURRENT_TIME                                                \
  ({                                                                           \
    struct timespec __ts__;                                                    \
    stimer_t __now__;                                                          \
    __now__ = clock_gettime(CLOCK_REALTIME, &__ts__) ?                         \
                  -1 :                                                         \
                  (((stimer_t) __ts__.tv_sec) * 1000000000) +                  \
                      ((stimer_t) __ts__.tv_nsec);                             \
    __now__;                                                                   \
  })

#endif

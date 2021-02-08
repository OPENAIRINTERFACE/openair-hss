/*
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
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#ifndef FILE_LOG_SEEN
#define FILE_LOG_SEEN

#include <stdarg.h>

#if DAEMONIZE
#include <syslog.h>
#define FPRINTF_ERROR(...)                                                     \
  do {                                                                         \
    syslog(LOG_ERR, ##__VA_ARGS__);                                            \
  } while (0)
#define FPRINTF_NOTICE(...)                                                    \
  do {                                                                         \
    syslog(LOG_NOTICE, ##__VA_ARGS__);                                         \
  } while (0)
#define FPRINTF_INFO(...)                                                      \
  do {                                                                         \
    syslog(LOG_INFO, ##__VA_ARGS__);                                           \
  } while (0)
#define VFPRINTF_ERR(...)                                                      \
  do {                                                                         \
    vsyslog(LOG_ERR, ##__VA_ARGS__);                                           \
  } while (0)
#define VFPRINTF_INFO(...)                                                     \
  do {                                                                         \
    vsyslog(LOG_INFO, ##__VA_ARGS__);                                          \
  } while (0)
#ifdef NODEBUG
#define FPRINTF_DEBUG(...)
#define VFPRINTF_DEBUG(...)
#else
#define FPRINTF_DEBUG(...)                                                     \
  do {                                                                         \
    syslog(LOG_DEBUG, ##__VA_ARGS__);                                          \
  } while (0)
#define VFPRINTF_DEBUG(...)                                                    \
  do {                                                                         \
    vsyslog(LOG_DEBUG, ##__VA_ARGS__);                                         \
  } while (0)
#endif
#else
#define FPRINTF_ERROR(...)                                                     \
  do {                                                                         \
    fprintf(stderr, ##__VA_ARGS__);                                            \
  } while (0)
#define FPRINTF_NOTICE(...)                                                    \
  do {                                                                         \
    fprintf(stdout, ##__VA_ARGS__);                                            \
  } while (0)
#define FPRINTF_INFO(...)                                                      \
  do {                                                                         \
    fprintf(stdout, ##__VA_ARGS__);                                            \
  } while (0)
#define VFPRINTF_ERROR(...)                                                    \
  do {                                                                         \
    vfprintf(stderr, ##__VA_ARGS__);                                           \
  } while (0)
#define VFPRINTF_INFO(...)                                                     \
  do {                                                                         \
    vfprintf(stdout, ##__VA_ARGS__);                                           \
  } while (0)
#ifdef NODEBUG
#define FPRINTF_DEBUG(...)
#define VFPRINTF_DEBUG(...)
#else
#define FPRINTF_DEBUG(...)                                                     \
  do {                                                                         \
    fprintf(stdout, ##__VA_ARGS__);                                            \
  } while (0)
#define VFPRINTF_DEBUG(...)                                                    \
  do {                                                                         \
    vfprintf(stdout, ##__VA_ARGS__);                                           \
  } while (0)
#endif
#endif
#endif /* FILE_LOG_SEEN */

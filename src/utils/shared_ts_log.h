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

/*! \file shared_ts_log.h
   \brief
   \author  Lionel GAUTHIER
   \date 2016
   \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_SHARED_TS_LOG_SEEN
#define FILE_SHARED_TS_LOG_SEEN

#include <sys/time.h>
#include <liblfds710.h>
#include "msc.h"
#include "log.h"

typedef enum {
  MIN_SH_TS_LOG_CLIENT = 0,
  SH_TS_LOG_TXT = MIN_SH_TS_LOG_CLIENT,
  SH_TS_LOG_MSC,
  MAX_SH_TS_LOG_CLIENT,
} sh_ts_log_app_id_t;

/*! \struct  shared_log_queue_item_t
* \brief Structure containing a string to be logged.
* This structure is pushed in thread safe queues by thread producers of logs.
* This structure is then popped by a dedicated thread that will send back this message
* to the logger producer in a thread safe manner.
*/
typedef struct shared_log_queue_item_s {
  struct lfds710_stack_element              se;
  sh_ts_log_app_id_t                        app_id;    /*!< \brief application identifier. */
  bstring                                   bstr;      /*!< \brief string containing the message. */
  union {
    msc_private_t                           msc;       /*!< \brief string containing the message. */
    log_private_t                           log;       /*!< \brief string containing the message. */
  } u_app_log;
} shared_log_queue_item_t;

/*! \struct  log_config_t
* \brief Structure containing the dynamically configurable parameters of the Logging facilities.
* This structure is filled by configuration facilities when parsing a configuration file.
*/

//------------------------------------------------------------------------------
int shared_log_get_start_time_sec (void);
void shared_log_reuse_item(shared_log_queue_item_t * item_p);
shared_log_queue_item_t * get_new_log_queue_item(sh_ts_log_app_id_t app_id);
void shared_log_get_elapsed_time_since_start(struct timeval * const elapsed_time);
int shared_log_init (const int max_threadsP);
void shared_log_itti_connect(void);
void shared_log_start_use (void);
void shared_log_flush_messages (void);
void shared_log_exit (void);
void shared_log_item(shared_log_queue_item_t * messageP);
#endif /* FILE_SHARED_TS_LOG_SEEN */

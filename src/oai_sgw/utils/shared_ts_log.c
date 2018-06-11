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

/*! \file shared_ts_log.c
   \brief
   \author  Lionel GAUTHIER
   \date 2016
   \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>

#include "bstrlib.h"

#include "hashtable.h"
#include "obj_hashtable.h"
#include "common_types.h"
#include "intertask_interface.h"
#include "timer.h"
#include "hashtable.h"
#include "log.h"
#include "msc.h"
#include "shared_ts_log.h"
#include "assertions.h"
#include "dynamic_memory_check.h"
#include "gcc_diag.h"
//-------------------------------
#define LOG_MAX_QUEUE_ELEMENTS                2048
#define LOG_MESSAGE_MIN_ALLOC_SIZE             256

#define LOG_FLUSH_PERIOD_SEC                     0
#define LOG_FLUSH_PERIOD_MICRO_SEC           50000
//-------------------------------

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long                   log_message_number_t;


/*! \struct  oai_shared_log_t
* \brief Structure containing all the logging utility internal variables.
*/
typedef struct oai_shared_log_s {
  // may be good to use stream instead of file descriptor when
  // logging somewhere else of the console.

  int                                     log_start_time_second;                                       /*!< \brief Logging utility reference time              */

  log_message_number_t                    log_message_number;                                          /*!< \brief Counter of log message        */
  struct lfds710_queue_bmm_element       *qbmme;
  struct lfds710_queue_bmm_state          log_message_queue;                                         /*!< \brief Thread safe log message queue */
  struct lfds710_stack_state              log_free_message_queue;                                      /*!< \brief Thread safe memory pool       */

  hash_table_ts_t                        *thread_context_htbl;                                         /*!< \brief Container for log_thread_ctxt_t */

  void (*logger_callback[MAX_SH_TS_LOG_CLIENT])(shared_log_queue_item_t*);
  bool                                    running;
} oai_shared_log_t;

static oai_shared_log_t g_shared_log={0};    /*!< \brief  logging utility internal variables global var definition*/


//------------------------------------------------------------------------------
int shared_log_get_start_time_sec (void)
{
  return g_shared_log.log_start_time_second;
}

//------------------------------------------------------------------------------
void shared_log_reuse_item(shared_log_queue_item_t * item_p)
{
#if defined(SHARED_LOG_PREALLOC_STRING_BUFFERS)
  btrunc(item_p->bstr, 0);
#else
  bdestroy_wrapper(&item_p->bstr);
#endif
  LFDS710_STACK_SET_VALUE_IN_ELEMENT( item_p->se, item_p );
  lfds710_stack_push( &g_shared_log.log_free_message_queue, &item_p->se );
}

//------------------------------------------------>-------------------------------
static shared_log_queue_item_t * create_new_log_queue_item(sh_ts_log_app_id_t app_id)
{
  shared_log_queue_item_t * item_p = calloc(1, sizeof(shared_log_queue_item_t));
  AssertFatal((item_p), "Allocation of log container failed");
  AssertFatal((app_id >= MIN_SH_TS_LOG_CLIENT), "Allocation of log container failed");
  AssertFatal((app_id < MAX_SH_TS_LOG_CLIENT), "Allocation of log container failed");
  item_p->app_id = app_id;
#if defined(SHARED_LOG_PREALLOC_STRING_BUFFERS)
  item_p->bstr = bfromcstralloc(LOG_MESSAGE_MIN_ALLOC_SIZE, "");
  AssertFatal((item_p->bstr), "Allocation of buf in log container failed");
#endif
  return item_p;
}

//------------------------------------------------------------------------------
shared_log_queue_item_t * get_new_log_queue_item(sh_ts_log_app_id_t app_id)
{
  shared_log_queue_item_t        *item_p = NULL;
  struct lfds710_stack_element   *se = NULL;

  lfds710_stack_pop( &g_shared_log.log_free_message_queue, &se );
  if (!se) {
    shared_log_flush_messages();
    lfds710_stack_pop( &g_shared_log.log_free_message_queue, &se );
  }
  if (se) {
    item_p = LFDS710_STACK_GET_VALUE_FROM_ELEMENT( *se );

    if (!item_p) {
      item_p = create_new_log_queue_item(app_id);
      AssertFatal(item_p,  "Out of memory error");
    } else {
      item_p->app_id = app_id;
#if defined(SHARED_LOG_PREALLOC_STRING_BUFFERS)
      btrunc(item_p->bstr, 0);
#endif
    }
#if !defined(SHARED_LOG_PREALLOC_STRING_BUFFERS)
    item_p->bstr = bfromcstralloc(LOG_MESSAGE_MIN_ALLOC_SIZE, "");
    AssertFatal((item_p->bstr), "Allocation of buf in log container failed");
#endif
  } else {
    OAI_FPRINTF_ERR("Could not get memory for logging\n");
  }
  return item_p;
}
//------------------------------------------------------------------------------
void* shared_log_task (__attribute__ ((unused)) void *args_p)
{
  MessageDef                             *received_message_p = NULL;
  long                                    timer_id = -1;
  int                                     rc = 0;
  int                                     exit_count = 2;

  itti_mark_task_ready (TASK_SHARED_TS_LOG);
  shared_log_start_use ();
  timer_setup (LOG_FLUSH_PERIOD_SEC,
               LOG_FLUSH_PERIOD_MICRO_SEC,
               TASK_SHARED_TS_LOG, INSTANCE_DEFAULT, TIMER_ONE_SHOT, NULL, &timer_id);

  while (1) {
    itti_receive_msg (TASK_SHARED_TS_LOG, &received_message_p);

    if (received_message_p != NULL) {

      switch (ITTI_MSG_ID (received_message_p)) {
      case TIMER_HAS_EXPIRED:{
        shared_log_flush_messages ();
        timer_setup (LOG_FLUSH_PERIOD_SEC,
            LOG_FLUSH_PERIOD_MICRO_SEC,
            TASK_SHARED_TS_LOG, INSTANCE_DEFAULT, TIMER_ONE_SHOT, NULL, &timer_id);
        }
        break;

      case TERMINATE_MESSAGE:{
          exit_count--;
          if (!exit_count) {
            g_shared_log.running = false;
            if (-1 != timer_id) {
              timer_remove (timer_id, NULL);
              timer_id = -1;
            }
            shared_log_exit ();
            itti_exit_task ();
          }
        }
        break;

      default:{
        }
        break;
      }
      // Freeing the memory allocated from the memory pool
      rc = itti_free (ITTI_MSG_ORIGIN_ID (received_message_p), received_message_p);
      AssertFatal (rc == EXIT_SUCCESS, "Failed to free memory (%d)!\n", rc);
      received_message_p = NULL;
    }
  }

  OAI_FPRINTF_ERR("Task Log exiting\n");
  return NULL;
}

//------------------------------------------------------------------------------
void shared_log_get_elapsed_time_since_start(struct timeval * const elapsed_time)
{
  // no thread safe but do not matter a lot
  gettimeofday(elapsed_time, NULL);
  // no timersub call for fastest operations
  elapsed_time->tv_sec = elapsed_time->tv_sec - g_shared_log.log_start_time_second;
}


//------------------------------------------------------------------------------
int shared_log_init (const int max_threadsP)
{
  shared_log_queue_item_t                *item_p = NULL;
  struct timeval                          start_time = {.tv_sec=0, .tv_usec=0};

  OAI_FPRINTF_INFO("Initializing shared logging\n");
  gettimeofday(&start_time, NULL);
  g_shared_log.log_start_time_second = start_time.tv_sec;
  g_shared_log.logger_callback[SH_TS_LOG_TXT] = log_flush_message;
#if MESSAGE_CHART_GENERATOR
  g_shared_log.logger_callback[SH_TS_LOG_MSC] = msc_flush_message;
#endif

  bstring b = bfromcstr("Logging thread context hashtable");
  g_shared_log.thread_context_htbl = hashtable_ts_create (LOG_MESSAGE_MIN_ALLOC_SIZE, NULL, free_wrapper, b);
  bdestroy_wrapper (&b);
  AssertFatal (NULL != g_shared_log.thread_context_htbl, "Could not create hashtable for Log!\n");
  g_shared_log.thread_context_htbl->log_enabled = false;


  log_thread_ctxt_t *thread_ctxt = calloc(1, sizeof(log_thread_ctxt_t));
  AssertFatal(NULL != thread_ctxt, "Error Could not create log thread context\n");
  pthread_t p = pthread_self();
  thread_ctxt->tid = p;
  hashtable_rc_t hash_rc = hashtable_ts_insert(g_shared_log.thread_context_htbl, (hash_key_t) p, thread_ctxt);
  if (HASH_TABLE_OK != hash_rc) {
    OAI_FPRINTF_ERR("Error Could not register log thread context\n");
    free_wrapper((void**)&thread_ctxt);
  }

  lfds710_stack_init_valid_on_current_logical_core( &g_shared_log.log_free_message_queue, NULL );
  g_shared_log.qbmme = calloc(LOG_MAX_QUEUE_ELEMENTS, sizeof(*g_shared_log.qbmme));
  lfds710_queue_bmm_init_valid_on_current_logical_core( &g_shared_log.log_message_queue, g_shared_log.qbmme, LOG_MAX_QUEUE_ELEMENTS, NULL );

  shared_log_start_use ();

  for (int i = 0; i < max_threadsP * 30; i++) {
    item_p = create_new_log_queue_item(MIN_SH_TS_LOG_CLIENT); // any logger
    LFDS710_STACK_SET_VALUE_IN_ELEMENT( item_p->se, item_p );
    lfds710_stack_push( &g_shared_log.log_free_message_queue, &item_p->se );
  }


  OAI_FPRINTF_INFO("Initializing shared logging Done\n");

  g_shared_log.running = true;

  return 0;
}

//------------------------------------------------------------------------------
void shared_log_itti_connect(void)
{
  int                                     rv = 0;
  rv = itti_create_task (TASK_SHARED_TS_LOG, shared_log_task, NULL);
  AssertFatal (rv == 0, "Create task for OAI logging failed!\n");
}

//------------------------------------------------------------------------------
void shared_log_start_use (void)
{
  pthread_t      p       = pthread_self();
  hashtable_rc_t hash_rc = hashtable_ts_is_key_exists (g_shared_log.thread_context_htbl, (hash_key_t) p);
  if (HASH_TABLE_KEY_NOT_EXISTS == hash_rc) {

    LFDS710_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

    log_thread_ctxt_t *thread_ctxt = calloc(1, sizeof(log_thread_ctxt_t));
    if (thread_ctxt) {
      thread_ctxt->tid = p;
      hash_rc = hashtable_ts_insert(g_shared_log.thread_context_htbl, (hash_key_t) p, thread_ctxt);
      if (HASH_TABLE_OK != hash_rc) {
        OAI_FPRINTF_ERR("Error Could not register log thread context\n");
        free_wrapper((void**)&thread_ctxt);
      }
    } else {
      OAI_FPRINTF_ERR("Error Could not create log thread context\n");
    }
  }
}

//------------------------------------------------------------------------------
void shared_log_flush_messages (void)
{
  int                                     rv = 0;
  shared_log_queue_item_t                *item_p = NULL;

  while ((rv = lfds710_queue_bmm_dequeue(&g_shared_log.log_message_queue, NULL, (void **)&item_p)) == 1) {
    if ((item_p->app_id >= MIN_SH_TS_LOG_CLIENT) && (item_p->app_id < MAX_SH_TS_LOG_CLIENT)) {
      (*g_shared_log.logger_callback[item_p->app_id])(item_p);
    } else {
      OAI_FPRINTF_ERR("Error bad logger identifier: %d\n", item_p->app_id);
    }
    shared_log_reuse_item(item_p);
  }
}

//------------------------------------------------------------------------------
static void shared_log_element_dequeue_cleanup_callback(struct lfds710_queue_bmm_state *qbmms, void *key, void *value)
{
  shared_log_queue_item_t        *item_p = (shared_log_queue_item_t*)value;

  if (item_p) {
    if (item_p->bstr) {
      bdestroy_wrapper(&item_p->bstr);
    }
    free_wrapper((void**)&item_p);
  }
}
//------------------------------------------------------------------------------
static void shared_log_element_pop_cleanup_callback(struct lfds710_stack_state *ss, struct lfds710_stack_element *se)
{
  shared_log_queue_item_t        *item_p = (shared_log_queue_item_t*)se->value;

  if (item_p) {
    if (item_p->bstr) {
      bdestroy_wrapper(&item_p->bstr);
    }
    free_wrapper((void**)&item_p);
  }
}
//------------------------------------------------------------------------------
void shared_log_exit (void)
{
  OAI_FPRINTF_INFO("[TRACE] Entering %s\n", __FUNCTION__);
  shared_log_flush_messages ();
  hashtable_ts_destroy(g_shared_log.thread_context_htbl);
  lfds710_queue_bmm_cleanup( &g_shared_log.log_message_queue, shared_log_element_dequeue_cleanup_callback);
  lfds710_stack_cleanup( &g_shared_log.log_free_message_queue, shared_log_element_pop_cleanup_callback );
  free_wrapper((void**)&g_shared_log.qbmme);
  OAI_FPRINTF_INFO("[TRACE] Leaving %s\n", __FUNCTION__);
}

//------------------------------------------------------------------------------
void shared_log_item(shared_log_queue_item_t * messageP)
{
  if (messageP) {
    if (g_shared_log.running) {
      shared_log_start_use();
      lfds710_queue_bmm_enqueue( &g_shared_log.log_message_queue, NULL, messageP );
    } else {
      if (messageP->bstr) {
        bdestroy_wrapper(&messageP->bstr);
      }
      free_wrapper((void**)&messageP);
    }
  }
}

#ifdef __cplusplus
}
#endif

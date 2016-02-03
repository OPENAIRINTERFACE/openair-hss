/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */

/*! \file log.c
   \brief Message chart generator logging utility (generated files to processed by a script to produce a loggen input file for generating a sequence diagram document)
   \author  Lionel GAUTHIER
   \date 2015
   \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/time.h>

#include "liblfds611.h"
#include "intertask_interface.h"
#include "timer.h"

#include "log.h"
#include "assertions.h"
#include "dynamic_memory_check.h"

#if HAVE_CONFIG_H
#  include "config.h"
#endif

//-------------------------------
#define LOG_MAX_QUEUE_ELEMENTS    1024
#define LOG_MAX_PROTO_NAME_LENGTH 16
#define LOG_MAX_MESSAGE_LENGTH    512

#define LOG_MAX_FILENAME_LENGTH       50
#define LOG_MAX_LOG_LEVEL_NAME_LENGTH 8
//-------------------------------

FILE                                   *g_log_fd = NULL;
char                                    g_log_proto2str[MAX_LOG_PROTOS][LOG_MAX_PROTO_NAME_LENGTH];
char                                    g_log_level2str[MAX_LOG_PROTOS][LOG_MAX_LOG_LEVEL_NAME_LENGTH];
int                                     g_log_start_time_second = 0;
log_level_t                             g_log_level[MAX_LOG_PROTOS];


typedef unsigned long                   log_message_number_t;
typedef struct log_queue_item_s {
  char                                   *message_str;
  uint32_t                                message_str_size;
  uint8_t                                *message_bin;
  uint32_t                                message_bin_size;
} log_queue_item_t;

log_message_number_t                    g_log_message_number = 0;
struct lfds611_queue_state             *g_log_message_queue_p = NULL;
struct lfds611_stack_state             *g_log_memory_stack_p = NULL;


//------------------------------------------------------------------------------
void                                   *
log_task (
  void *args_p)
//------------------------------------------------------------------------------
{
  MessageDef                             *received_message_p = NULL;
  long                                    timer_id = 0;

  itti_mark_task_ready (TASK_LOG);
  log_start_use ();
  timer_setup (0,               // seconds
               50000,           // usec
               TASK_LOG, INSTANCE_DEFAULT, TIMER_PERIODIC, NULL, &timer_id);

  while (1) {
    itti_receive_msg (TASK_LOG, &received_message_p);

    if (received_message_p != NULL) {

      switch (ITTI_MSG_ID (received_message_p)) {
      case TIMER_HAS_EXPIRED:{
          log_flush_messages ();
        }
        break;

      case TERMINATE_MESSAGE:{
          timer_remove (timer_id);
          log_end ();
          itti_exit_task ();
        }
        break;

      case MESSAGE_TEST:{
        }
        break;

      default:{
        }
        break;
      }
    }
  }

  fprintf (stderr, "Task Log exiting\n");
  return NULL;
}


static void log_get_elapsed_time_since_start(struct timeval * const elapsed_time)
{
  // no thread safe but do not matter a lot
  gettimeofday(elapsed_time, NULL);
  // no timersub call for fastest operations
  elapsed_time->tv_sec = elapsed_time->tv_sec - g_log_start_time_second;
}


//------------------------------------------------------------------------------
int
log_init (
  const log_env_t envP,
  const log_level_t default_log_levelP,
  const int max_threadsP)
//------------------------------------------------------------------------------
{
  int                                     i = 0;
  int                                     rv = 0;
  void                                   *pointer_p = NULL;
  char                                    log_filename[256];
  struct timeval                          start_time = {.tv_sec=0, .tv_usec=0};

  rv = gettimeofday(&start_time, NULL);
  g_log_start_time_second = start_time.tv_sec;

  fprintf (stderr, "Initializing OAI Logging\n");
  rv = snprintf (log_filename, 256, "/tmp/openair.log.%u.log", envP);   // TODO NAME

  if ((0 >= rv) || (256 < rv)) {
    fprintf (stderr, "Error in Log file name");
  }

/*
 *MEMO
 *if (output & OUT_CONSOLE) {
 *setvbuf(stdout, NULL, _IONBF, 0);
 *setvbuf(stderr, NULL, _IONBF, 0);
 *}...
 */

  // use File now, may use stream later
  g_log_fd = fopen (log_filename, "w");
  AssertFatal (g_log_fd != NULL, "Could not open log file %s : %s", log_filename, strerror (errno));
  rv = lfds611_stack_new (&g_log_memory_stack_p, (lfds611_atom_t) max_threadsP + 2);

  if (0 >= rv) {
    AssertFatal (0, "lfds611_stack_new failed!\n");
  }

  rv = lfds611_queue_new (&g_log_message_queue_p, (lfds611_atom_t) LOG_MAX_QUEUE_ELEMENTS);
  AssertFatal (rv, "lfds611_queue_new failed!\n");
  AssertFatal (g_log_message_queue_p != NULL, "g_log_message_queue_p is NULL!\n");
  log_start_use ();

  for (i = 0; i < max_threadsP * 30; i++) {
    pointer_p = MALLOC_CHECK (LOG_MAX_MESSAGE_LENGTH);
    AssertFatal (pointer_p, "MALLOC_CHECK failed!\n");
    rv = lfds611_stack_guaranteed_push (g_log_memory_stack_p, pointer_p);
    AssertFatal (rv, "lfds611_stack_guaranteed_push failed for item %u\n", i);
  }

  rv = snprintf (&g_log_proto2str[LOG_UDP][0], LOG_MAX_PROTO_NAME_LENGTH, "UDP");
  rv = snprintf (&g_log_proto2str[LOG_GTPV1U][0], LOG_MAX_PROTO_NAME_LENGTH, "GTPv1-U");
  rv = snprintf (&g_log_proto2str[LOG_GTPV2C][0], LOG_MAX_PROTO_NAME_LENGTH, "GTPv2-C");
  rv = snprintf (&g_log_proto2str[LOG_S1AP][0], LOG_MAX_PROTO_NAME_LENGTH, "S1AP");
  rv = snprintf (&g_log_proto2str[LOG_MME_APP][0], LOG_MAX_PROTO_NAME_LENGTH, "MME-APP");
  rv = snprintf (&g_log_proto2str[LOG_NAS][0], LOG_MAX_PROTO_NAME_LENGTH, "NAS");
  rv = snprintf (&g_log_proto2str[LOG_NAS_EMM][0], LOG_MAX_PROTO_NAME_LENGTH, "NAS-EMM");
  rv = snprintf (&g_log_proto2str[LOG_NAS_ESM][0], LOG_MAX_PROTO_NAME_LENGTH, "NAS-ESM");
  rv = snprintf (&g_log_proto2str[LOG_SPGW_APP][0], LOG_MAX_PROTO_NAME_LENGTH, "SPGW-APP");
  rv = snprintf (&g_log_proto2str[LOG_S11][0], LOG_MAX_PROTO_NAME_LENGTH, "S11");
  rv = snprintf (&g_log_proto2str[LOG_S6A][0], LOG_MAX_PROTO_NAME_LENGTH, "S6A");
  rv = snprintf (&g_log_proto2str[LOG_UTIL][0], LOG_MAX_PROTO_NAME_LENGTH, "UTIL");
  rv = snprintf (&g_log_proto2str[LOG_CONFIG][0], LOG_MAX_PROTO_NAME_LENGTH, "CONFIG");
  rv = snprintf (&g_log_proto2str[LOG_MSC][0], LOG_MAX_PROTO_NAME_LENGTH, "MSC");

  rv = snprintf (&g_log_level2str[LOG_LEVEL_TRACE][0], LOG_MAX_LOG_LEVEL_NAME_LENGTH, "TRACE");
  rv = snprintf (&g_log_level2str[LOG_LEVEL_DEBUG][0], LOG_MAX_LOG_LEVEL_NAME_LENGTH, "DEBUG");
  rv = snprintf (&g_log_level2str[LOG_LEVEL_INFO][0], LOG_MAX_LOG_LEVEL_NAME_LENGTH, "INFO");
  rv = snprintf (&g_log_level2str[LOG_LEVEL_NOTICE][0], LOG_MAX_LOG_LEVEL_NAME_LENGTH, "NOTICE");
  rv = snprintf (&g_log_level2str[LOG_LEVEL_WARNING][0], LOG_MAX_LOG_LEVEL_NAME_LENGTH, "WARNING");
  rv = snprintf (&g_log_level2str[LOG_LEVEL_ERROR][0], LOG_MAX_LOG_LEVEL_NAME_LENGTH, "ERROR");
  rv = snprintf (&g_log_level2str[LOG_LEVEL_CRITICAL][0], LOG_MAX_LOG_LEVEL_NAME_LENGTH, "CRITICAL");
  rv = snprintf (&g_log_level2str[LOG_LEVEL_ALERT][0], LOG_MAX_LOG_LEVEL_NAME_LENGTH, "ALERT");
  rv = snprintf (&g_log_level2str[LOG_LEVEL_EMERGENCY][0], LOG_MAX_LOG_LEVEL_NAME_LENGTH, "EMERGENCY");

  for (i=MIN_LOG_PROTOS; i < MAX_LOG_PROTOS; i++) {
    g_log_level[i] = default_log_levelP;
  }
  for (i=MIN_LOG_LEVEL; i < MAX_LOG_LEVEL; i++) {
    g_log_level2str[i][LOG_MAX_LOG_LEVEL_NAME_LENGTH-1]     = '\0';
  }

  rv = itti_create_task (TASK_LOG, log_task, NULL);
  AssertFatal (rv == 0, "Create task for OAI logging failed!\n");
  log_start_use(); // yes this thread also
  fprintf (stderr, "Initializing OAI logging Done\n");
  return 0;
}

//------------------------------------------------------------------------------
void
log_start_use (
  void)
//------------------------------------------------------------------------------
{
  lfds611_queue_use (g_log_message_queue_p);
  lfds611_stack_use (g_log_memory_stack_p);
}


//------------------------------------------------------------------------------
void
log_flush_messages (
  void)
//------------------------------------------------------------------------------
{
  int                                     rv;
  log_queue_item_t                       *item_p = NULL;

  while ((rv = lfds611_queue_dequeue (g_log_message_queue_p, (void **)&item_p)) == 1) {
    if (NULL != item_p->message_str) {
      fputs (item_p->message_str, g_log_fd);
      // TODO BIN DATA
      rv = lfds611_stack_guaranteed_push (g_log_memory_stack_p, item_p->message_str);
    }
    // TODO FREE_CHECK BIN DATA
    FREE_CHECK (item_p);
  }

  fflush (g_log_fd);
}

//------------------------------------------------------------------------------
void
log_end (
  void)
//------------------------------------------------------------------------------
{
  int                                     rv;

  if (NULL != g_log_fd) {
    log_flush_messages ();
    rv = fflush (g_log_fd);

    if (rv != 0) {
      fprintf (stderr, "Error while flushing stream of Log file: %s", strerror (errno));
    }

    rv = fclose (g_log_fd);

    if (rv != 0) {
      fprintf (stderr, "Error while closing Log file: %s", strerror (errno));
    }
  }
}

//------------------------------------------------------------------------------
void
log_message (
  const log_level_t log_levelP,
  const log_proto_t protoP,
  const char *const source_fileP,
  const unsigned int line_numP,
  char *format,
  ...)
//------------------------------------------------------------------------------
{
  va_list                                 args;
  int                                     rv;
  int                                     rv2;
  int                                     filename_length = 0;
  log_queue_item_t                       *new_item_p = NULL;
  char                                   *char_message_p = NULL;

  if ((MIN_LOG_PROTOS > protoP) || (MAX_LOG_PROTOS <= protoP)) {
    return;
  }
  if ((MIN_LOG_LEVEL > log_levelP) || (MAX_LOG_LEVEL <= log_levelP)) {
    return;
  }
  if (log_levelP > g_log_level[protoP]) {
    return;
  }
  new_item_p = MALLOC_CHECK (sizeof (log_queue_item_t));

  if (NULL != new_item_p) {
    rv = lfds611_stack_pop (g_log_memory_stack_p, (void **)&char_message_p);

    if (0 == rv) {
      log_flush_messages ();
      rv = lfds611_stack_pop (g_log_memory_stack_p, (void **)&char_message_p);
    }

    if (1 == rv) {
      struct timeval elapsed_time;
      log_get_elapsed_time_since_start(&elapsed_time);
      filename_length = strlen(source_fileP);
      if (filename_length > LOG_MAX_FILENAME_LENGTH) {
        rv = snprintf (char_message_p, LOG_MAX_MESSAGE_LENGTH, "%06" PRIu64 "|%04ld:%06ld|%.6s|%.6s|%s:%u| ",
            __sync_fetch_and_add (&g_log_message_number, 1), elapsed_time.tv_sec, elapsed_time.tv_usec,
            &g_log_level2str[log_levelP][0], &g_log_proto2str[protoP][0],
            &source_fileP[filename_length-LOG_MAX_FILENAME_LENGTH], line_numP);
      } else {
        rv = snprintf (char_message_p, LOG_MAX_MESSAGE_LENGTH, "%06" PRIu64 "|%04ld:%06ld|%.6s|%.6s|%s:%u| ",
            __sync_fetch_and_add (&g_log_message_number, 1), elapsed_time.tv_sec, elapsed_time.tv_usec,
            &g_log_level2str[log_levelP][0], &g_log_proto2str[protoP][0], source_fileP, line_numP);
      }

      if ((0 > rv) || (LOG_MAX_MESSAGE_LENGTH < rv)) {
        fprintf (stderr, "Error while logging LOG message : %s", &g_log_proto2str[protoP][0]);
        goto error_event;
      }

      va_start (args, format);
      rv2 = vsnprintf (&char_message_p[rv], LOG_MAX_MESSAGE_LENGTH - rv, format, args);
      va_end (args);

      if ((0 > rv2) || ((LOG_MAX_MESSAGE_LENGTH - rv) < rv2)) {
        fprintf (stderr, "Error while logging LOG message : %s", &g_log_proto2str[protoP][0]);
        goto error_event;
      }

      rv += rv2;
      rv2 = snprintf (&char_message_p[rv], LOG_MAX_MESSAGE_LENGTH - rv, "\n");

      if ((0 > rv2) || ((LOG_MAX_MESSAGE_LENGTH - rv) < rv2)) {
        fprintf (stderr, "Error while logging LOG message : %s", &g_log_proto2str[protoP][0]);
        goto error_event;
      }

      rv += rv2;
      new_item_p->message_str = char_message_p;
      new_item_p->message_str_size = rv;
      new_item_p->message_bin = NULL;   // TO DO
      new_item_p->message_bin_size = 0; // TO DO
      rv = lfds611_queue_enqueue (g_log_message_queue_p, new_item_p);

      if (0 == rv) {
        fprintf (stderr, "Error while lfds611_queue_guaranteed_enqueue message %s in LOG", char_message_p);
        rv = lfds611_stack_guaranteed_push (g_log_memory_stack_p, char_message_p);
        FREE_CHECK (new_item_p);
      }
    } else {
      FREE_CHECK (new_item_p);
      fprintf (stderr, "Error while lfds611_stack_pop()\n");
      log_flush_messages ();
    }
  }

  return;
error_event:
  rv = lfds611_stack_guaranteed_push (g_log_memory_stack_p, char_message_p);
  FREE_CHECK (new_item_p);
}



/*
int
log_init (
  const mme_config_t * mme_config_p,
  log_specific_init_t specific_init)
{
  if (mme_config_p->verbosity_level == 1) {
    log_enabled = 1;
  } else if (mme_config_p->verbosity_level == 2) {
    log_enabled = 1;
  } else {
    log_enabled = 0;
  }

  return specific_init (mme_config_p->verbosity_level);
}
*/

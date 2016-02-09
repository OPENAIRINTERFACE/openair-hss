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
   \brief Thread safe logging utility, log output can be redirected to stdout, file or remote host through TCP.
   \author  Lionel GAUTHIER
   \date 2015
   \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>

#include "liblfds611.h"
#include "intertask_interface.h"
#include "timer.h"
#include "hashtable.h"

#include "log.h"
#include "assertions.h"
#include "dynamic_memory_check.h"

#if HAVE_CONFIG_H
#  include "config.h"
#endif

//-------------------------------
#define LOG_MAX_QUEUE_ELEMENTS                1024
#define LOG_MAX_PROTO_NAME_LENGTH               16
#define LOG_FLUSH_PERIOD_SEC                     0
#define LOG_FLUSH_PERIOD_MICRO_SEC          100000

#define LOG_DISPLAYED_FILENAME_MAX_LENGTH       32
#define LOG_DISPLAYED_LOG_LEVEL_NAME_MAX_LENGTH  5
#define LOG_DISPLAYED_PROTO_NAME_MAX_LENGTH      6
#define LOG_FUNC_INDENT_SPACES                   3
#define LOG_LEVEL_NAME_MAX_LENGTH               10
#define LOG_ANSI_CODE_MAX_LENGTH                15
#define LOG_MAX_SERVER_ADDRESS_LENGTH           96
#define LOG_MAX_PORT_NUM_LENGTH                  6
//-------------------------------

typedef unsigned long                   log_message_number_t;

typedef enum {
  MIN_LOG_TCP_STATE = 0,
  LOG_TCP_STATE_DISABLED = MIN_LOG_TCP_STATE,
  LOG_TCP_STATE_NOT_CONNECTED,
  LOG_TCP_STATE_CONNECTING,
  LOG_TCP_STATE_CONNECTED,
  MAX_LOG_TCP_STATE
} log_tcp_state_t;


/*! \struct  oai_log_t
* \brief Structure containing all the logging utility internal variables.
*/
typedef struct oai_log_s {
  // may be good to use stream instead of file descriptor when
  // logging somewhere else of the console.
  FILE                                   *log_fd;                                               /*!< \brief output stream */

  char                                    server_address[LOG_MAX_SERVER_ADDRESS_LENGTH];        /*!< \brief TCP remote (or local) server hostname */
  char                                    server_port[LOG_MAX_PORT_NUM_LENGTH];                 /*!< \brief TCP remote (or local) server port     */
  log_tcp_state_t                         tcp_state;                                            /*!< \brief State of the client TCP connection           */

  char                                    log_proto2str[MAX_LOG_PROTOS][LOG_MAX_PROTO_NAME_LENGTH];    /*!< \brief Convert log client (protocol/layer) id into human readable log user name */
  char                                    log_level2str[MAX_LOG_LEVEL][LOG_LEVEL_NAME_MAX_LENGTH]; /*!< \brief Convert log level id into human readable log level string */
  int                                     log_start_time_second;                                       /*!< \brief Logging utility reference time              */
  log_level_t                             log_level[MAX_LOG_PROTOS];                                   /*!< \brief Loglevel id of each client (protocol/layer) */

  log_message_number_t                    log_message_number;                                          /*!< \brief Counter of log message        */
  struct lfds611_queue_state             *log_message_queue_p;                                         /*!< \brief Thread safe log message queue */
  struct lfds611_stack_state             *log_free_message_queue_p;                                          /*!< \brief Thread safe memory pool       */

  hash_table_t                           *thread_context_htbl;                                         /*!< \brief Container for log_thread_ctxt_t */
} oai_log_t;

static oai_log_t g_oai_log={0};    /*!< \brief  logging utility internal variables global var definition*/

inline static void log_reuse_item(log_queue_item_t * item_p) __attribute__((always_inline));


int log_get_start_time_sec (void)
{
  return g_oai_log.log_start_time_second;
}

static void log_reuse_item(log_queue_item_t * item_p)
{
  int         rv = 0;
#if LOG_OAI_CLEAN_HARD
  memset((void*)item_p, 0, sizeof(* item_p));
#else
  item_p->len = 0;
#endif

  rv = lfds611_stack_guaranteed_push (g_oai_log.log_free_message_queue_p, item_p);
  if (0 == rv) {
    FREE_CHECK (item_p);
  }
}

//------------------------------------------------------------------------------
void* log_task (__attribute__ ((unused)) void *args_p)
{
  MessageDef                             *received_message_p = NULL;
  long                                    timer_id = 0;

  itti_mark_task_ready (TASK_LOG);
  log_start_use ();
  timer_setup (LOG_FLUSH_PERIOD_SEC,
               LOG_FLUSH_PERIOD_MICRO_SEC,
               TASK_LOG, INSTANCE_DEFAULT, TIMER_ONE_SHOT, NULL, &timer_id);

  while (1) {
    itti_receive_msg (TASK_LOG, &received_message_p);

    if (received_message_p != NULL) {

      switch (ITTI_MSG_ID (received_message_p)) {
      case TIMER_HAS_EXPIRED:{
          //test
          LOG_TRACE(LOG_UTIL, "TASK_LOG received TIMER_HAS_EXPIRED\n"); {
            MessageDef                             *message_p = NULL;
            message_p = itti_alloc_new_message (TASK_LOG, MESSAGE_TEST);
            itti_send_msg_to_task (TASK_SPGW_APP, INSTANCE_DEFAULT, message_p);
          }

          // if tcp logging is enabled
          if (LOG_TCP_STATE_NOT_CONNECTED == g_oai_log.tcp_state) {
            log_connect_to_server();
          }
          log_flush_messages ();

          timer_setup (LOG_FLUSH_PERIOD_SEC,
                       LOG_FLUSH_PERIOD_MICRO_SEC,
                       TASK_LOG, INSTANCE_DEFAULT, TIMER_ONE_SHOT, NULL, &timer_id);
        }
        break;

      case TERMINATE_MESSAGE:{
          timer_remove (timer_id);
          log_exit ();
          itti_exit_task ();
        }
        break;

      default:{
        }
        break;
      }
    }
  }

  fprintf (stdout, "Task Log exiting\n");
  return NULL;
}

//------------------------------------------------------------------------------
void log_connect_to_server(void)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sfd, s;

  g_oai_log.tcp_state = LOG_TCP_STATE_CONNECTING;

  // man getaddrinfo:
  /* Obtain address(es) matching host/port */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* Stream socket */
  hints.ai_flags = 0;
  hints.ai_protocol = IPPROTO_TCP;          /* TCP protocol */

   s = getaddrinfo(g_oai_log.server_address, g_oai_log.server_port, &hints, &result);
   if (s != 0) {
     g_oai_log.tcp_state = LOG_TCP_STATE_NOT_CONNECTED;
     fprintf(stderr, "Could not connect to log server: getaddrinfo: %s\n", gai_strerror(s));
     return;
   }

   /* getaddrinfo() returns a list of address structures.
      Try each address until we successfully connect(2).
      If socket(2) (or connect(2)) fails, we (close the socket
      and) try the next address. */
   for (rp = result; rp != NULL; rp = rp->ai_next) {
     sfd = socket(rp->ai_family, rp->ai_socktype,
         rp->ai_protocol);
     if (sfd == -1)
       continue;

     if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
       break;                  /* Success */

     close(sfd);
   }

   if (rp == NULL) {               /* No address succeeded */
     g_oai_log.tcp_state = LOG_TCP_STATE_NOT_CONNECTED;
     fprintf(stderr, "Could not connect to log server %s:%s\n", g_oai_log.server_address, g_oai_log.server_port);
     return;
   }
   freeaddrinfo(result);           /* No longer needed */

   fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL, 0)|O_NONBLOCK);

   g_oai_log.log_fd= fdopen (sfd, "w");
   if (NULL == g_oai_log.log_fd) {
     g_oai_log.tcp_state = LOG_TCP_STATE_NOT_CONNECTED;
     close(sfd);
     fprintf(stderr, "ERROR: Could not associate a stream with the TCP socket file descriptor\n");
     fprintf(stderr, "ERROR: errno %d: %s\n", errno, strerror(errno));
     return;
   }
   fprintf(stdout, "Connected to log server %s:%s\n", g_oai_log.server_address, g_oai_log.server_port);
   g_oai_log.tcp_state = LOG_TCP_STATE_CONNECTED;
}

//------------------------------------------------------------------------------
void log_set_config(const log_config_t * const config)
{
  if (config) {
    if ((MAX_LOG_LEVEL > config->udp_log_level) && (MIN_LOG_LEVEL <= config->udp_log_level))         g_oai_log.log_level[LOG_UDP] = config->udp_log_level;
    if ((MAX_LOG_LEVEL > config->gtpv1u_log_level) && (MIN_LOG_LEVEL <= config->gtpv1u_log_level))   g_oai_log.log_level[LOG_GTPV1U]   = config->gtpv1u_log_level;
    if ((MAX_LOG_LEVEL > config->gtpv2c_log_level) && (MIN_LOG_LEVEL <= config->gtpv2c_log_level))   g_oai_log.log_level[LOG_GTPV2C]   = config->gtpv2c_log_level;
    if ((MAX_LOG_LEVEL > config->sctp_log_level) && (MIN_LOG_LEVEL <= config->sctp_log_level))       g_oai_log.log_level[LOG_SCTP]     = config->sctp_log_level;
    if ((MAX_LOG_LEVEL > config->s1ap_log_level) && (MIN_LOG_LEVEL <= config->s1ap_log_level))       g_oai_log.log_level[LOG_S1AP]     = config->s1ap_log_level;
    if ((MAX_LOG_LEVEL > config->mme_app_log_level) && (MIN_LOG_LEVEL <= config->mme_app_log_level)) g_oai_log.log_level[LOG_MME_APP]  = config->mme_app_log_level;
    if ((MAX_LOG_LEVEL > config->nas_log_level) && (MIN_LOG_LEVEL <= config->nas_log_level)) {
      g_oai_log.log_level[LOG_NAS]      = config->nas_log_level;
      g_oai_log.log_level[LOG_NAS_EMM]  = config->nas_log_level;
      g_oai_log.log_level[LOG_NAS_ESM]  = config->nas_log_level;
    }
    if ((MAX_LOG_LEVEL > config->spgw_app_log_level) && (MIN_LOG_LEVEL <= config->spgw_app_log_level)) g_oai_log.log_level[LOG_SPGW_APP] = config->spgw_app_log_level;
    if ((MAX_LOG_LEVEL > config->s11_log_level) && (MIN_LOG_LEVEL <= config->s11_log_level))           g_oai_log.log_level[LOG_S11]      = config->s11_log_level;
    if ((MAX_LOG_LEVEL > config->s6a_log_level) && (MIN_LOG_LEVEL <= config->s6a_log_level))           g_oai_log.log_level[LOG_S6A]      = config->s6a_log_level;
    if ((MAX_LOG_LEVEL > config->util_log_level) && (MIN_LOG_LEVEL <= config->util_log_level))         g_oai_log.log_level[LOG_UTIL]     = config->util_log_level;
    if ((MAX_LOG_LEVEL > config->msc_log_level) && (MIN_LOG_LEVEL <= config->msc_log_level))           g_oai_log.log_level[LOG_MSC]      = config->msc_log_level;
    if ((MAX_LOG_LEVEL > config->itti_log_level) && (MIN_LOG_LEVEL <= config->itti_log_level))         g_oai_log.log_level[LOG_ITTI]     = config->itti_log_level;

    if (config->output) {
      if (strcasecmp("CONSOLE", config->output)){
        // if seems to be a file path
        if ((config->output[0] == '.') || (config->output[0] == '/')) {
          g_oai_log.log_fd = fopen (config->output, "w");
          AssertFatal (NULL != g_oai_log.log_fd, "Could not open log file %s : %s", config->output, strerror (errno));
        } else {
          // may be a TCP server address host:portnum
          int  len       = strlen(config->output);
          int  i         = 0;
          int  j         = 0;
          int  server_port = 0;
          // extract server address
          while ((config->output[i] != ':') && (i < len)) {
            g_oai_log.server_address[i] = config->output[i];
            i += 1;
          }
          AssertFatal(':' == config->output[i], "Bad format");
          g_oai_log.server_address[i] = '\0';
          i += 1; // ':'
          AssertFatal(i<len, "Server address %s bad format", config->output);
          // extract port number
          j = 0;
          while ((i < len) && (j < LOG_MAX_PORT_NUM_LENGTH-1)){
            AssertFatal(isdigit(config->output[i]), "%c not a digit (TCP port number) server address %s",
                config->output[i], config->output);
            g_oai_log.server_port[j++] = config->output[i++];
          }
          g_oai_log.server_port[j] = '\0';
          server_port = atoi(g_oai_log.server_port);

          AssertFatal(1024 <= server_port, "Invalid Server TCP port %d/%s", server_port, g_oai_log.server_port);
          AssertFatal(65535 >= server_port, "Invalid Server TCP port %d/%s", server_port, g_oai_log.server_port);
          g_oai_log.tcp_state = LOG_TCP_STATE_NOT_CONNECTED;
          log_connect_to_server();
        }
      } else {
        // default case
        setvbuf(stdout, NULL, _IONBF, 0);
        g_oai_log.log_fd = stdout;
      }
    }
  }

}

//------------------------------------------------------------------------------
const char * log_level_int2str(const log_level_t log_level)
{
  if ((MAX_LOG_LEVEL > log_level) && (MIN_LOG_LEVEL <= log_level)) {
    return g_oai_log.log_level2str[log_level];
  }
  return "INVALID_LOG_LEVEL";
}

//------------------------------------------------------------------------------
log_level_t log_level_str2int(const char * const log_level_str)
{
  int log_level;

  if (log_level_str) {
    for (log_level = MIN_LOG_LEVEL; log_level < MAX_LOG_LEVEL; log_level++) {
      if (0 == strcasecmp(log_level_str, &g_oai_log.log_level2str[log_level][0])) {
        return log_level;
      }
    }
  }
  // By default
  return MAX_LOG_LEVEL; // == invalid
}

//------------------------------------------------------------------------------
static void log_get_elapsed_time_since_start(struct timeval * const elapsed_time)
{
  // no thread safe but do not matter a lot
  gettimeofday(elapsed_time, NULL);
  // no timersub call for fastest operations
  elapsed_time->tv_sec = elapsed_time->tv_sec - g_oai_log.log_start_time_second;
}

//------------------------------------------------------------------------------
void log_signal_callback_handler(int signum){
  fprintf(stderr, "Caught signal SIGPIPE %d\n",signum);
  if (LOG_TCP_STATE_DISABLED != g_oai_log.tcp_state) {
    // Let ITTI LOG Timer do the reconnection
    g_oai_log.tcp_state = LOG_TCP_STATE_NOT_CONNECTED;
    return;
  }
}

//------------------------------------------------------------------------------
int
log_init (
  __attribute__ ((unused))const log_env_t envP,
  const log_level_t default_log_levelP,
  const int max_threadsP)
{
  int                                     i = 0;
  int                                     rv = 0;
  log_queue_item_t                       *item_p = NULL;
  struct timeval                          start_time = {.tv_sec=0, .tv_usec=0};

  signal(SIGPIPE, log_signal_callback_handler);

  g_oai_log.log_fd = NULL;

  rv = gettimeofday(&start_time, NULL);
  g_oai_log.log_start_time_second = start_time.tv_sec;


  fprintf (stdout, "Initializing OAI Logging\n");

  fprintf (stdout, "Initializing OAI logging @-3\n");fflush(stdout);fflush(stderr);
  g_oai_log.thread_context_htbl = hashtable_ts_create (128, NULL, FREE_CHECK, "Logging thread context hashtable");
  fprintf (stdout, "Initializing OAI logging @-2\n");fflush(stdout);fflush(stderr);
  AssertFatal (NULL != g_oai_log.thread_context_htbl, "Could not create hashtable for Log!\n");
  fprintf (stdout, "Initializing OAI logging @-1\n");fflush(stdout);fflush(stderr);

  log_thread_ctxt_t *thread_ctxt = CALLOC_CHECK(1, sizeof(log_thread_ctxt_t));
  AssertFatal(NULL != thread_ctxt, "Error Could not create log thread context\n");
  pthread_t p = pthread_self();
  thread_ctxt->tid = p;
  hashtable_rc_t hash_rc = hashtable_ts_insert(g_oai_log.thread_context_htbl, (hash_key_t) p, thread_ctxt);
  if (HASH_TABLE_OK != hash_rc) {
    fprintf (stderr, "Error Could not register log thread context\n");
    FREE_CHECK(thread_ctxt);
  }

  fprintf (stdout, "Initializing OAI logging @0\n");fflush(stdout);fflush(stderr);

  rv = lfds611_stack_new (&g_oai_log.log_free_message_queue_p, (lfds611_atom_t) max_threadsP + 2);

  if (0 >= rv) {
    AssertFatal (0, "lfds611_stack_new failed!\n");
  }

  rv = lfds611_queue_new (&g_oai_log.log_message_queue_p, (lfds611_atom_t) LOG_MAX_QUEUE_ELEMENTS);
  AssertFatal (rv, "lfds611_queue_new failed!\n");
  AssertFatal (g_oai_log.log_message_queue_p != NULL, "g_oai_log.log_message_queue_p is NULL!\n");
  fprintf (stdout, "Initializing OAI logging @1\n");fflush(stdout);
  log_start_use ();

  fprintf (stdout, "Initializing OAI logging @2\n");fflush(stdout);
  for (i = 0; i < max_threadsP * 30; i++) {
    item_p = CALLOC_CHECK (1, sizeof(log_queue_item_t));
    AssertFatal (item_p, "MALLOC_CHECK failed!\n");
    rv = lfds611_stack_guaranteed_push (g_oai_log.log_free_message_queue_p, item_p);
    AssertFatal (rv, "lfds611_stack_guaranteed_push failed for item %u\n", i);
  }
  fprintf (stdout, "Initializing OAI logging @3\n");fflush(stdout);

  rv = snprintf (&g_oai_log.log_proto2str[LOG_SCTP][0], LOG_MAX_PROTO_NAME_LENGTH, "SCTP");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_UDP][0], LOG_MAX_PROTO_NAME_LENGTH, "UDP");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_GTPV1U][0], LOG_MAX_PROTO_NAME_LENGTH, "GTPv1-U");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_GTPV2C][0], LOG_MAX_PROTO_NAME_LENGTH, "GTPv2-C");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_S1AP][0], LOG_MAX_PROTO_NAME_LENGTH, "S1AP");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_MME_APP][0], LOG_MAX_PROTO_NAME_LENGTH, "MME-APP");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_NAS][0], LOG_MAX_PROTO_NAME_LENGTH, "NAS");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_NAS_EMM][0], LOG_MAX_PROTO_NAME_LENGTH, "NAS-EMM");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_NAS_ESM][0], LOG_MAX_PROTO_NAME_LENGTH, "NAS-ESM");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_SPGW_APP][0], LOG_MAX_PROTO_NAME_LENGTH, "SPGW-APP");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_S11][0], LOG_MAX_PROTO_NAME_LENGTH, "S11");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_S6A][0], LOG_MAX_PROTO_NAME_LENGTH, "S6A");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_UTIL][0], LOG_MAX_PROTO_NAME_LENGTH, "UTIL");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_CONFIG][0], LOG_MAX_PROTO_NAME_LENGTH, "CONFIG");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_MSC][0], LOG_MAX_PROTO_NAME_LENGTH, "MSC");
  rv = snprintf (&g_oai_log.log_proto2str[LOG_ITTI][0], LOG_MAX_PROTO_NAME_LENGTH, "ITTI");

  rv = snprintf (&g_oai_log.log_level2str[LOG_LEVEL_TRACE][0], LOG_LEVEL_NAME_MAX_LENGTH, "TRACE");
  rv = snprintf (&g_oai_log.log_level2str[LOG_LEVEL_DEBUG][0], LOG_LEVEL_NAME_MAX_LENGTH, "DEBUG");
  rv = snprintf (&g_oai_log.log_level2str[LOG_LEVEL_INFO][0], LOG_LEVEL_NAME_MAX_LENGTH, "INFO");
  rv = snprintf (&g_oai_log.log_level2str[LOG_LEVEL_NOTICE][0], LOG_LEVEL_NAME_MAX_LENGTH, "NOTICE");
  rv = snprintf (&g_oai_log.log_level2str[LOG_LEVEL_WARNING][0], LOG_LEVEL_NAME_MAX_LENGTH, "WARNING");
  rv = snprintf (&g_oai_log.log_level2str[LOG_LEVEL_ERROR][0], LOG_LEVEL_NAME_MAX_LENGTH, "ERROR");
  rv = snprintf (&g_oai_log.log_level2str[LOG_LEVEL_CRITICAL][0], LOG_LEVEL_NAME_MAX_LENGTH, "CRITICAL");
  rv = snprintf (&g_oai_log.log_level2str[LOG_LEVEL_ALERT][0], LOG_LEVEL_NAME_MAX_LENGTH, "ALERT");
  rv = snprintf (&g_oai_log.log_level2str[LOG_LEVEL_EMERGENCY][0], LOG_LEVEL_NAME_MAX_LENGTH, "EMERGENCY");

  for (i=MIN_LOG_PROTOS; i < MAX_LOG_PROTOS; i++) {
    g_oai_log.log_level[i] = default_log_levelP;
  }
  // did not check return value of snprintf...
  for (i=MIN_LOG_LEVEL; i < MAX_LOG_LEVEL; i++) {
    g_oai_log.log_level2str[i][LOG_LEVEL_NAME_MAX_LENGTH-1]     = '\0';
  }

  fprintf (stderr, "Initializing OAI logging Done\n");fflush(stdout);
  log_message (thread_ctxt, LOG_LEVEL_INFO, LOG_UTIL, __FILE__, __LINE__, "Initializing OAI logging Done\n");
  fprintf (stderr, "Initializing OAI logging Done2\n");fflush(stdout);
  return 0;
}

//------------------------------------------------------------------------------
// listen to ITTI events
void log_itti_connect(
  void)
{
  int                                     rv = 0;
  rv = itti_create_task (TASK_LOG, log_task, NULL);
  AssertFatal (rv == 0, "Create task for OAI logging failed!\n");
}

//------------------------------------------------------------------------------
void
log_start_use (
  void)
{
  pthread_t      p       = pthread_self();
  hashtable_rc_t hash_rc = hashtable_ts_is_key_exists (g_oai_log.thread_context_htbl, (hash_key_t) p);
  if (HASH_TABLE_KEY_NOT_EXISTS == hash_rc) {
    lfds611_queue_use (g_oai_log.log_message_queue_p);
    lfds611_stack_use (g_oai_log.log_free_message_queue_p);
    log_thread_ctxt_t *thread_ctxt = CALLOC_CHECK(1, sizeof(log_thread_ctxt_t));
    if (thread_ctxt) {
      thread_ctxt->tid = p;
      hash_rc = hashtable_ts_insert(g_oai_log.thread_context_htbl, (hash_key_t) p, thread_ctxt);
      if (HASH_TABLE_OK != hash_rc) {
        fprintf (stderr, "Error Could not register log thread context\n");
        FREE_CHECK(thread_ctxt);
      }
    } else {
      fprintf (stderr, "Error Could not create log thread context\n");
    }
  }
}

//------------------------------------------------------------------------------
void
log_flush_messages (
  void)
{
  int                                     rv = 0;
  int                                     rv_put = 0;
  log_queue_item_t                       *item_p = NULL;

  if (g_oai_log.log_fd) {
    while ((rv = lfds611_queue_dequeue (g_oai_log.log_message_queue_p, (void **)&item_p)) == 1) {
      if (item_p->len > 0) {
        rv_put = fputs (item_p->str, g_oai_log.log_fd);
        // TODO BIN DATA
        item_p->len = 0;
        rv = lfds611_stack_guaranteed_push (g_oai_log.log_free_message_queue_p, item_p);
      }
      if (rv_put < 0) {
        // error occured
        fprintf (stderr, "Error while writing log %d\n", rv_put);
        rv = fclose (g_oai_log.log_fd);
        if (rv != 0) {
          fprintf (stderr, "Error while closing Log file stream: %s\n", strerror (errno));
        }
        // do not exit
        if (LOG_TCP_STATE_DISABLED != g_oai_log.tcp_state) {
          // Let ITTI LOG Timer do the reconnection
          g_oai_log.tcp_state = LOG_TCP_STATE_NOT_CONNECTED;
          return;
        }
      }
    }
    fflush (g_oai_log.log_fd);
  }
}

//------------------------------------------------------------------------------
void
log_exit (
  void)
{
  int                                     rv = 0;

  fprintf(stdout, "[TRACE] Entering %s\n", __FUNCTION__);
  if (NULL != g_oai_log.log_fd) {
    log_flush_messages ();
    rv = fflush (g_oai_log.log_fd);

    if (rv != 0) {
      fprintf (stderr, "Error while flushing stream of Log file: %s", strerror (errno));
    }

    rv = fclose (g_oai_log.log_fd);

    if (rv != 0) {
      fprintf (stderr, "Error while closing Log file: %s", strerror (errno));
    }
  }
  fprintf(stdout, "[TRACE] Leaving %s\n", __FUNCTION__);
}

//------------------------------------------------------------------------------
void log_stream_hex(
  const log_level_t log_levelP,
  const log_proto_t protoP,
  const char *const source_fileP,
  const unsigned int line_numP,
  const char *const messageP,
  const char *const streamP,
  const size_t sizeP)
{
  log_queue_item_t  * message = NULL;
  size_t              octet_index = 0;
  int                 rv = 0;
  log_thread_ctxt_t  *thread_ctxt = NULL;
  hashtable_rc_t      hash_rc = HASH_TABLE_OK;

  pthread_t             p           = pthread_self();
  hash_rc = hashtable_ts_get (g_oai_log.thread_context_htbl, (hash_key_t) p, (void **)&thread_ctxt);
  if (HASH_TABLE_KEY_NOT_EXISTS == hash_rc) {
    // make the thread safe LFDS collections usable by this thread
    log_start_use();
  }
  hash_rc = hashtable_ts_get (g_oai_log.thread_context_htbl, (hash_key_t) p, (void **)&thread_ctxt);
  AssertFatal(NULL != thread_ctxt, "Could not get new log thread context\n");
  if (messageP) {
    log_message_start(thread_ctxt, log_levelP, protoP, &message, source_fileP, line_numP, "%s (hexa)", messageP);
  } else {
    log_message_start(thread_ctxt, log_levelP, protoP, &message, source_fileP, line_numP, "(hexa):");
  }
  if ((streamP) && (message)) {
    for (octet_index = 0; octet_index < sizeP; octet_index++) {
      // do not call log_message_add(), too much overhead for sizeP*3chars
      rv = snprintf (&message->str[message->len], LOG_MAX_MESSAGE_LENGTH - message->len, " %02x", streamP[octet_index]);

      if ((0 > rv) || ((LOG_MAX_MESSAGE_LENGTH - message->len) < rv)) {
        fprintf (stderr, "Error while logging message\n");
      } else {
        message->len += rv;
      }
    }
    log_message_finish(message);
  }
}

//------------------------------------------------------------------------------
void log_stream_hex_array(
  const log_level_t log_levelP,
  const log_proto_t protoP,
  const char *const source_fileP,
  const unsigned int line_numP,
  const char *const messageP,
  const char *const streamP,
  const size_t      sizeP)
{
  log_queue_item_t *  message = NULL;
  size_t              octet_index = 0;
  size_t              index = 0;
  log_thread_ctxt_t  *thread_ctxt = NULL;
  hashtable_rc_t      hash_rc = HASH_TABLE_OK;

  pthread_t             p           = pthread_self();
  hash_rc = hashtable_ts_get (g_oai_log.thread_context_htbl, (hash_key_t) p, (void **)&thread_ctxt);
  if (HASH_TABLE_KEY_NOT_EXISTS == hash_rc) {
    // make the thread safe LFDS collections usable by this thread
    log_start_use();
  }
  hash_rc = hashtable_ts_get (g_oai_log.thread_context_htbl, (hash_key_t) p, (void **)&thread_ctxt);
  AssertFatal(NULL != thread_ctxt, "Could not get new log thread context\n");

  if (messageP) {
    log_message(thread_ctxt, log_levelP, protoP, source_fileP, line_numP, "%s", messageP);
  }
  log_message(thread_ctxt, log_levelP, protoP, source_fileP, line_numP, "------+-------------------------------------------------|\n");
  log_message(thread_ctxt, log_levelP, protoP, source_fileP, line_numP, "      |  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f |\n");
  log_message(thread_ctxt, log_levelP, protoP, source_fileP, line_numP, "------+-------------------------------------------------|\n");

  if (streamP) {
    for (octet_index = 0; octet_index < sizeP; octet_index++) {
      if ((octet_index % 16) == 0) {
        if (octet_index != 0) {
          log_message_add(message, " |");
          log_message_finish(message);
        }
        log_message_start(thread_ctxt, log_levelP, protoP, &message, source_fileP, line_numP, " %04ld |", octet_index);
      }

      /*
       * Print every single octet in hexadecimal form
       */
      log_message_add(message, " %02x", streamP[octet_index]);
    }
    /*
     * Append enough spaces and put final pipe
     */
    for (index = octet_index; index < 16; ++index) {
      log_message_add(message, "   ");
    }
    log_message_add(message, " |");
    log_message_finish(message);
  }
}

//------------------------------------------------------------------------------
void
log_message_add (
  log_queue_item_t * messageP,
  char *format,
  ...)
{
  va_list                                 args;
  int                                     rv = 0;

  if (messageP) {
    if (messageP->len > 0) {
      va_start (args, format);
      rv = vsnprintf (&messageP->str[messageP->len], LOG_MAX_MESSAGE_LENGTH - messageP->len, format, args);
      va_end (args);

      if ((0 > rv) || ((LOG_MAX_MESSAGE_LENGTH - messageP->len) < rv)) {
        fprintf (stderr, "Error while logging message\n");
      } else {
        messageP->len += rv;
      }
    }
  }
}
//------------------------------------------------------------------------------
void
log_message_finish (
  log_queue_item_t * messageP)
{
  int                                     rv = 0;

  if (messageP) {
    rv = snprintf (&messageP->str[messageP->len], LOG_MAX_MESSAGE_LENGTH - messageP->len, "\n");

    if ((0 > rv) || ((LOG_MAX_MESSAGE_LENGTH - messageP->len) < rv)) {
      messageP->str[LOG_MAX_MESSAGE_LENGTH-1] = '\0';
      fprintf (stderr, "Error while logging message\n");
    } else {
      messageP->len += rv;
    }
    // send message
    rv = lfds611_queue_enqueue (g_oai_log.log_message_queue_p, messageP);

    if (0 == rv) {
      fprintf (stderr, "Error while lfds611_queue_guaranteed_enqueue message %s in LOG\n", messageP->str);
      messageP->len = 0;
      rv = lfds611_queue_enqueue (g_oai_log.log_message_queue_p, messageP);
      FREE_CHECK (messageP);
    }
  }
}

//------------------------------------------------------------------------------
void
log_message_start (
  log_thread_ctxt_t *thread_ctxtP,
  const log_level_t log_levelP,
  const log_proto_t protoP,
  log_queue_item_t ** messageP, // Out parameter
  const char *const source_fileP,
  const unsigned int line_numP,
  char *format,
  ...)
{
  va_list                                 args;
  int                                     rv              = 0;
  int                                     rv2             = 0;
  int                                     filename_length = 0;
  log_thread_ctxt_t                      *thread_ctxt     = thread_ctxtP;
  hashtable_rc_t                          hash_rc         = HASH_TABLE_OK;

  if ((MIN_LOG_PROTOS > protoP) || (MAX_LOG_PROTOS <= protoP)) {
    return;
  }
  if ((MIN_LOG_LEVEL > log_levelP) || (MAX_LOG_LEVEL <= log_levelP)) {
    return;
  }
  if (log_levelP > g_oai_log.log_level[protoP]) {
    return;
  }

  if (NULL == thread_ctxt){
    pthread_t             p           = pthread_self();
    hash_rc = hashtable_ts_get (g_oai_log.thread_context_htbl, (hash_key_t) p, (void **)&thread_ctxt);
    if (HASH_TABLE_KEY_NOT_EXISTS == hash_rc) {
      // make the thread safe LFDS collections usable by this thread
      log_start_use();
      hash_rc = hashtable_ts_get (g_oai_log.thread_context_htbl, (hash_key_t) p, (void **)&thread_ctxt);
      AssertFatal(NULL != thread_ctxt, "Could not get new log thread context\n");
    }
  }

  if (! *messageP) {
    rv = lfds611_stack_pop (g_oai_log.log_free_message_queue_p, (void **)messageP);

    if (0 == rv) {
      *messageP = CALLOC_CHECK(1, sizeof(log_queue_item_t));
      AssertFatal(*messageP, "Out of memory error");
      fprintf (stderr, "Warning allocating an extra log_queue_item_t in LOG\n");
      rv = 1;
    }

    if (1 == rv) {
      struct timeval elapsed_time;
      log_get_elapsed_time_since_start(&elapsed_time);
      filename_length = strlen(source_fileP);
      if (filename_length > LOG_DISPLAYED_FILENAME_MAX_LENGTH) {
        rv = snprintf ((*messageP)->str, LOG_MAX_MESSAGE_LENGTH, "%06" PRIu64 " %05ld:%06ld %08lX %-*.*s %-*.*s %-*.*s:%04u   %*s",
            __sync_fetch_and_add (&g_oai_log.log_message_number, 1), elapsed_time.tv_sec, elapsed_time.tv_usec,
            thread_ctxt->tid,
            LOG_DISPLAYED_LOG_LEVEL_NAME_MAX_LENGTH, LOG_DISPLAYED_LOG_LEVEL_NAME_MAX_LENGTH, &g_oai_log.log_level2str[log_levelP][0],
            LOG_DISPLAYED_PROTO_NAME_MAX_LENGTH, LOG_DISPLAYED_PROTO_NAME_MAX_LENGTH, &g_oai_log.log_proto2str[protoP][0],
            LOG_DISPLAYED_FILENAME_MAX_LENGTH, LOG_DISPLAYED_FILENAME_MAX_LENGTH, &source_fileP[filename_length-LOG_DISPLAYED_FILENAME_MAX_LENGTH], line_numP,
            thread_ctxt->indent, " ");
      } else {
        rv = snprintf ((*messageP)->str, LOG_MAX_MESSAGE_LENGTH, "%06" PRIu64 " %05ld:%06ld %08lX %-*.*s %-*.*s %-*.*s:%04u   %*s",
            __sync_fetch_and_add (&g_oai_log.log_message_number, 1), elapsed_time.tv_sec, elapsed_time.tv_usec,
            thread_ctxt->tid,
            LOG_DISPLAYED_LOG_LEVEL_NAME_MAX_LENGTH, LOG_DISPLAYED_LOG_LEVEL_NAME_MAX_LENGTH, &g_oai_log.log_level2str[log_levelP][0],
            LOG_DISPLAYED_PROTO_NAME_MAX_LENGTH, LOG_DISPLAYED_PROTO_NAME_MAX_LENGTH, &g_oai_log.log_proto2str[protoP][0],
            LOG_DISPLAYED_FILENAME_MAX_LENGTH, LOG_DISPLAYED_FILENAME_MAX_LENGTH, source_fileP, line_numP,
            thread_ctxt->indent, " ");
      }

      if ((0 > rv) || (LOG_MAX_MESSAGE_LENGTH < rv)) {
        fprintf (stderr, "Error while logging message : %s", &g_oai_log.log_proto2str[protoP][0]);
        goto error_event_start;
      }

      va_start (args, format);
      rv2 = vsnprintf (&((*messageP)->str[rv]), LOG_MAX_MESSAGE_LENGTH - rv, format, args);
      va_end (args);

      if ((0 > rv2) || ((LOG_MAX_MESSAGE_LENGTH - rv) < rv2)) {
        fprintf (stderr, "Error while logging message : %s", &g_oai_log.log_proto2str[protoP][0]);
        goto error_event_start;
      }

      rv += rv2;
      (*messageP)->len = rv;
      return;
    }
  }
  return;
error_event_start:
  // put in memory pool the message buffer
  (*messageP)->len = 0;
  rv = lfds611_stack_guaranteed_push (g_oai_log.log_free_message_queue_p, *messageP);
  return;
}

//------------------------------------------------------------------------------
// hard-coded to use LOG_LEVEL_TRACE
void
log_func (
  bool  is_enteringP,
  const log_proto_t protoP,
  const char *const source_fileP,
  const unsigned int line_numP,
  const char *const functionP)
{
  log_thread_ctxt_t        *thread_ctxt = NULL;
  hashtable_rc_t            hash_rc     = HASH_TABLE_OK;
  pthread_t                 p           = pthread_self();

  hash_rc = hashtable_ts_get (g_oai_log.thread_context_htbl, (hash_key_t) p, (void **)&thread_ctxt);
  if (HASH_TABLE_KEY_NOT_EXISTS == hash_rc) {
    // make the thread safe LFDS collections usable by this thread
    log_start_use();
  }
  hash_rc = hashtable_ts_get (g_oai_log.thread_context_htbl, (hash_key_t) p, (void **)&thread_ctxt);
  AssertFatal(NULL != thread_ctxt, "Could not get new log thread context\n");
  if (true == is_enteringP) {
    log_message(thread_ctxt, LOG_LEVEL_TRACE, protoP, source_fileP, line_numP, "Entering %s()\n", functionP);
    thread_ctxt->indent += LOG_FUNC_INDENT_SPACES;
  } else {
    log_message(thread_ctxt, LOG_LEVEL_TRACE, protoP, source_fileP, line_numP, "Leaving %s()\n", functionP);
    thread_ctxt->indent -= LOG_FUNC_INDENT_SPACES;
    if (thread_ctxt->indent < 0) thread_ctxt->indent = 0;
  }
}
//------------------------------------------------------------------------------
void
log_message (
  log_thread_ctxt_t * thread_ctxtP,
  const log_level_t log_levelP,
  const log_proto_t protoP,
  const char *const source_fileP,
  const unsigned int line_numP,
  char *format,
  ...)
{
  va_list                                 args;
  int                                     rv              = 0;
  int                                     rv2             = 0;
  int                                     filename_length = 0;
  log_queue_item_t                       *new_item_p      = NULL;
  log_thread_ctxt_t                      *thread_ctxt     = thread_ctxtP;
  hashtable_rc_t                          hash_rc         = HASH_TABLE_OK;

  if ((MIN_LOG_PROTOS > protoP) || (MAX_LOG_PROTOS <= protoP)) {
    return;
  }
  if ((MIN_LOG_LEVEL > log_levelP) || (MAX_LOG_LEVEL <= log_levelP)) {
    return;
  }
  if (log_levelP > g_oai_log.log_level[protoP]) {
    return;
  }
  if (NULL == thread_ctxt){
    pthread_t             p           = pthread_self();
    hash_rc = hashtable_ts_get (g_oai_log.thread_context_htbl, (hash_key_t) p, (void **)&thread_ctxt);
    if (HASH_TABLE_KEY_NOT_EXISTS == hash_rc) {
      fprintf (stderr, "TEMP Not registered thread %lX\n", p);
      fflush(stderr);
      // make the thread safe LFDS collections usable by this thread
      log_start_use();
      hash_rc = hashtable_ts_get (g_oai_log.thread_context_htbl, (hash_key_t) p, (void **)&thread_ctxt);
      AssertFatal(NULL != thread_ctxt, "Could not get new log thread context\n");
    }
  }

  rv = lfds611_stack_pop (g_oai_log.log_free_message_queue_p, (void **)&new_item_p);

  if (0 == rv) {
    new_item_p = CALLOC_CHECK(1, sizeof(log_queue_item_t));
    AssertFatal(new_item_p, "Out of memory error");
    fprintf (stderr, "Warning allocating an extra log_queue_item_t in LOG\n");
    rv = 1;
  }

  if (new_item_p) {
    if (1 == rv) {
      struct timeval elapsed_time;
      log_get_elapsed_time_since_start(&elapsed_time);
      filename_length = strlen(source_fileP);
      if (filename_length > LOG_DISPLAYED_FILENAME_MAX_LENGTH) {
        rv = snprintf (new_item_p->str, LOG_MAX_MESSAGE_LENGTH, "%06" PRIu64 " %05ld:%06ld %08lX %-*.*s %-*.*s %-*.*s:%04u   %*s",
            __sync_fetch_and_add (&g_oai_log.log_message_number, 1), elapsed_time.tv_sec, elapsed_time.tv_usec,
            thread_ctxt->tid,
            LOG_DISPLAYED_LOG_LEVEL_NAME_MAX_LENGTH, LOG_DISPLAYED_LOG_LEVEL_NAME_MAX_LENGTH, &g_oai_log.log_level2str[log_levelP][0],
            LOG_DISPLAYED_PROTO_NAME_MAX_LENGTH, LOG_DISPLAYED_PROTO_NAME_MAX_LENGTH, &g_oai_log.log_proto2str[protoP][0],
            LOG_DISPLAYED_FILENAME_MAX_LENGTH, LOG_DISPLAYED_FILENAME_MAX_LENGTH, &source_fileP[filename_length-LOG_DISPLAYED_FILENAME_MAX_LENGTH], line_numP,
            thread_ctxt->indent, " ");
      } else {
        rv = snprintf (new_item_p->str, LOG_MAX_MESSAGE_LENGTH, "%06" PRIu64 " %05ld:%06ld %08lX %-*.*s %-*.*s %-*.*s:%04u   %*s",
            __sync_fetch_and_add (&g_oai_log.log_message_number, 1), elapsed_time.tv_sec, elapsed_time.tv_usec,
            thread_ctxt->tid,
            LOG_DISPLAYED_LOG_LEVEL_NAME_MAX_LENGTH, LOG_DISPLAYED_LOG_LEVEL_NAME_MAX_LENGTH, &g_oai_log.log_level2str[log_levelP][0],
            LOG_DISPLAYED_PROTO_NAME_MAX_LENGTH, LOG_DISPLAYED_PROTO_NAME_MAX_LENGTH, &g_oai_log.log_proto2str[protoP][0],
            LOG_DISPLAYED_FILENAME_MAX_LENGTH, LOG_DISPLAYED_FILENAME_MAX_LENGTH, source_fileP, line_numP,
            thread_ctxt->indent, " ");
      }

      if ((0 > rv) || (LOG_MAX_MESSAGE_LENGTH < rv)) {
        fprintf (stderr, "Error while logging LOG message : %s", &g_oai_log.log_proto2str[protoP][0]);
        goto error_event;
      }

      va_start (args, format);
      rv2 = vsnprintf (&new_item_p->str[rv], LOG_MAX_MESSAGE_LENGTH - rv, format, args);
      va_end (args);

      if ((0 > rv2) || ((LOG_MAX_MESSAGE_LENGTH - rv) < rv2)) {
        fprintf (stderr, "Error while logging LOG message : %s", &g_oai_log.log_proto2str[protoP][0]);
        goto error_event;
      }

      rv += rv2;

      new_item_p->len = rv;
      new_item_p->str[rv] = 0;
      rv = lfds611_queue_enqueue (g_oai_log.log_message_queue_p, new_item_p);

      if (0 == rv) {
        new_item_p->len = 0;
        rv = lfds611_stack_guaranteed_push (g_oai_log.log_free_message_queue_p, new_item_p);
        if (0 == rv) {
          FREE_CHECK (new_item_p);
        }
      }
    } else {
      FREE_CHECK (new_item_p);
      fprintf (stderr, "Error while lfds611_stack_pop()\n");
      //log_flush_messages ();
    }
  }

  return;
error_event:
  new_item_p->len = 0;
  rv = lfds611_stack_guaranteed_push (g_oai_log.log_free_message_queue_p, new_item_p);
  if (0 == rv) {
    FREE_CHECK (new_item_p);
  }
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

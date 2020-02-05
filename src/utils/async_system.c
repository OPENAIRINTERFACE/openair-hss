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

/*! \file async_system.c
   \brief
   \author  Lionel GAUTHIER
   \date 2017
   \email: lionel.gauthier@eurecom.fr
*/
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "bstrlib.h"

#include "assertions.h"
#include "async_system.h"
#include "common_defs.h"
#include "dynamic_memory_check.h"
#include "intertask_interface.h"
#include "itti_free_defined_msg.h"
#include "log.h"
//-------------------------------
void async_system_exit(void);
void *async_system_task(__attribute__((unused)) void *args_p);

//------------------------------------------------------------------------------
void *async_system_task(__attribute__((unused)) void *args_p) {
  MessageDef *received_message_p = NULL;
  int rc = 0;

  itti_mark_task_ready(TASK_ASYNC_SYSTEM);

  while (1) {
    itti_receive_msg(TASK_ASYNC_SYSTEM, &received_message_p);

    if (received_message_p != NULL) {
      switch (ITTI_MSG_ID(received_message_p)) {
        case ASYNC_SYSTEM_COMMAND: {
          rc = 0;
          OAILOG_DEBUG(
              LOG_ASYNC_SYSTEM, "C system() call: %s\n",
              bdata(ASYNC_SYSTEM_COMMAND(received_message_p).system_command));
          rc = system(
              bdata(ASYNC_SYSTEM_COMMAND(received_message_p).system_command));

          if (rc) {
            OAILOG_ERROR(
                LOG_ASYNC_SYSTEM, "ERROR in system command %s: %d\n",
                bdata(ASYNC_SYSTEM_COMMAND(received_message_p).system_command),
                rc);
            if (ASYNC_SYSTEM_COMMAND(received_message_p).is_abort_on_error) {
              bdestroy_wrapper(
                  &ASYNC_SYSTEM_COMMAND(received_message_p).system_command);
              exit(-1);  // may be not exit
            }
            bdestroy_wrapper(
                &ASYNC_SYSTEM_COMMAND(received_message_p).system_command);
          }
        } break;

        case TERMINATE_MESSAGE: {
          async_system_exit();
          itti_exit_task();
        } break;

        default: { } break; }
      // Freeing the memory allocated from the memory pool
      itti_free_msg_content(received_message_p);
      itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), received_message_p);
      received_message_p = NULL;
    }
  }
  return NULL;
}

//------------------------------------------------------------------------------
int async_system_init(void) {
  OAI_FPRINTF_INFO("Initializing ASYNC_SYSTEM\n");
  if (itti_create_task(TASK_ASYNC_SYSTEM, &async_system_task, NULL) < 0) {
    perror("pthread_create");
    OAILOG_ALERT(LOG_ASYNC_SYSTEM,
                 "Initializing ASYNC_SYSTEM task interface: ERROR\n");
    return RETURNerror;
  }
  OAI_FPRINTF_INFO("Initializing ASYNC_SYSTEM Done\n");
  return RETURNok;
}

//------------------------------------------------------------------------------
int async_system_command(int sender_itti_task, bool is_abort_on_error,
                         char *format, ...) {
  va_list args;
  int rv = 0;
  bstring bstr = NULL;
  va_start(args, format);
  bstr = bfromcstralloc(1024, " ");
  btrunc(bstr, 0);
  rv = bvcformata(bstr, 1024, format, args);  // big number, see bvcformata
  va_end(args);

  if (NULL == bstr) {
    OAILOG_ERROR(LOG_ASYNC_SYSTEM, "Error while formatting system command");
    return RETURNerror;
  }
  MessageDef *message_p = NULL;
  message_p = itti_alloc_new_message(sender_itti_task, ASYNC_SYSTEM_COMMAND);
  AssertFatal(message_p, "itti_alloc_new_message Failed");
  ASYNC_SYSTEM_COMMAND(message_p).system_command = bstr;
  ASYNC_SYSTEM_COMMAND(message_p).is_abort_on_error = is_abort_on_error;
  rv = itti_send_msg_to_task(TASK_ASYNC_SYSTEM, INSTANCE_DEFAULT, message_p);
  return rv;
}

//------------------------------------------------------------------------------
void async_system_exit(void) {
  OAI_FPRINTF_INFO("TASK_ASYNC_SYSTEM terminated");
}

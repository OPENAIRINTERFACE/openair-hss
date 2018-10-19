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

/*! \file async_system.h
   \brief We still use some unix commands for convenience, and we did not have to replace them by system calls
   \ Instead of calling C system(...) that can take a lot of time (creation of a process, etc), in many cases
   \ it doesn't hurt to do this asynchronously, may be we must tweak thread priority, pin it to a CPU, etc (TODO later)
   \author  Lionel GAUTHIER
   \date 2017
   \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_ASYNC_SYSTEM_SEEN
#define FILE_ASYNC_SYSTEM_SEEN

#ifdef __cplusplus
extern "C" {
#endif

int async_system_init (void);
int async_system_command (int sender_itti_task, bool is_abort_on_error, char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* FILE_SHARED_TS_LOG_SEEN */

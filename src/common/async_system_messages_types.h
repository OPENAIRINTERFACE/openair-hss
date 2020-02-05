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

/*! \file async_system_messages_types.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_ASYNC_SYSTEM_MESSAGES_TYPES_SEEN
#define FILE_ASYNC_SYSTEM_MESSAGES_TYPES_SEEN

#include <stdbool.h>

#include "bstrlib.h"

#define ASYNC_SYSTEM_COMMAND(mSGpTR) (mSGpTR)->ittiMsg.async_system_command

typedef struct itti_async_system_command_s {
  bstring system_command;
  bool is_abort_on_error;
} itti_async_system_command_t;

#endif /* FILE_ASYNC_SYSTEM_MESSAGES_TYPES_SEEN */

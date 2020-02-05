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

/*! \file dynamic_memory_check.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdlib.h>

#include "bstrlib.h"

#include "assertions.h"
#include "dynamic_memory_check.h"

//------------------------------------------------------------------------------
void free_wrapper(void **ptr) {
  // for debug only
  AssertFatal(ptr, "Trying to free NULL *ptr, ptr=%p", ptr);
  if (ptr) {
    // for debug only
    AssertFatal(*ptr, "Trying to free NULL *ptr, ptr=%p", ptr);
    free(*ptr);
    *ptr = NULL;
  }
}

//------------------------------------------------------------------------------
void bdestroy_wrapper(bstring *b) {
  if ((b) && (*b)) {
    bdestroy(*b);
    *b = NULL;
  }
}

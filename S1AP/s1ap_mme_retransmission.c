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


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "tree.h"
#include "assertions.h"
#include "intertask_interface.h"
#include "timer.h"
#include "s1ap_common.h"
#include "s1ap_mme_retransmission.h"
#include "dynamic_memory_check.h"
#include "log.h"

inline int                              s1ap_mme_timer_map_compare_id (
  struct s1ap_timer_map_s *p1,
  struct s1ap_timer_map_s *p2);

/* Reference to tree root element */
RB_HEAD (s1ap_timer_map, s1ap_timer_map_s) s1ap_timer_tree = RB_INITIALIZER ();

/* RB tree functions for s1ap timer map are not exposed to the rest of the code
   only declare prototypes here.
*/
RB_PROTOTYPE (s1ap_timer_map, s1ap_timer_map_s, entries, s1ap_mme_timer_map_compare_id);

RB_GENERATE (s1ap_timer_map, s1ap_timer_map_s, entries, s1ap_mme_timer_map_compare_id);

     int                                     s1ap_mme_timer_map_compare_id (
  struct s1ap_timer_map_s *p1,
  struct s1ap_timer_map_s *p2)
{
  if (p1->mme_ue_s1ap_id > 0) {
    if (p1->mme_ue_s1ap_id > p2->mme_ue_s1ap_id) {
      return 1;
    }

    if (p1->mme_ue_s1ap_id < p2->mme_ue_s1ap_id) {
      return -1;
    }

    return 0;
  }

  if (p1->timer_id > p2->timer_id) {
    return 1;
  }

  if (p1->timer_id < p2->timer_id) {
    return -1;
  }

  /*
   * Match -> return 0
   */
  return 0;
}

int
s1ap_timer_insert (
  uint32_t mme_ue_s1ap_id,
  long timer_id)
{
  struct s1ap_timer_map_s                *new;

  new = MALLOC_CHECK (sizeof (struct s1ap_timer_map_s));
  new->timer_id = timer_id;
  new->mme_ue_s1ap_id = mme_ue_s1ap_id;

  if (RB_INSERT (s1ap_timer_map, &s1ap_timer_tree, new) != NULL) {
    S1AP_ERROR ("Timer with id 0x%lx already exists\n", timer_id);
    FREE_CHECK (new);
    return -1;
  }

  return 0;
}

int
s1ap_handle_timer_expiry (
  timer_has_expired_t * timer_has_expired)
{
  struct s1ap_timer_map_s                *find;
  struct s1ap_timer_map_s                 elm;

  DevAssert (timer_has_expired != NULL);
  memset (&elm, 0, sizeof (elm));
  elm.timer_id = timer_has_expired->timer_id;

  if ((find = RB_FIND (s1ap_timer_map, &s1ap_timer_tree, &elm)) == NULL) {
    S1AP_WARN ("Timer id 0x%lx has not been found in tree. Maybe the timer " "reference has been removed before receiving timer signal\n", timer_has_expired->timer_id);
    return 0;
  }

  /*
   * Remove the timer from the map
   */
  RB_REMOVE (s1ap_timer_map, &s1ap_timer_tree, find);
  /*
   * Destroy the element
   */
  FREE_CHECK (find);
  /*
   * TODO: notify NAS and remove ue context
   */
  return 0;
}

int
s1ap_timer_remove_ue (
  uint32_t mme_ue_s1ap_id)
{
  struct s1ap_timer_map_s                *find;

  S1AP_DEBUG ("Removing timer associated with UE " S1AP_UE_ID_FMT "\n", mme_ue_s1ap_id);
  DevAssert (mme_ue_s1ap_id != 0);
  RB_FOREACH (find, s1ap_timer_map, &s1ap_timer_tree) {
    if (find->mme_ue_s1ap_id == mme_ue_s1ap_id) {
      timer_remove (find->timer_id);
      /*
       * Remove the timer from the map
       */
      RB_REMOVE (s1ap_timer_map, &s1ap_timer_tree, find);
      /*
       * Destroy the element
       */
      FREE_CHECK (find);
    }
  }
  return 0;
}

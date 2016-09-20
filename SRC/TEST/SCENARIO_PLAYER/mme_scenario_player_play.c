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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "mme_scenario_player.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "3gpp_24.301.h"
#include "security_types.h"
#include "common_types.h"
#include "emm_msg.h"
#include "mme_api.h"
#include "emm_data.h"
#include "esm_msg.h"
#include "intertask_interface.h"
#include "timer.h"
#include "dynamic_memory_check.h"
#include "common_defs.h"
#include "xml_msg_dump_itti.h"
#include "usim_authenticate.h"

extern scenario_player_t g_msp_scenarios;

//------------------------------------------------------------------------------
void msp_scenario_tick(scenario_t * const scenario)
{
  // send SP_SCENARIO_TICK message
  AssertFatal(scenario, "Must have a scenario context");
  MessageDef *message_p = itti_alloc_new_message (TASK_MME_SCENARIO_PLAYER, SP_SCENARIO_TICK);
  message_p->ittiMsg.scenario_tick.scenario = scenario;
  itti_send_msg_to_task (TASK_MME_SCENARIO_PLAYER, INSTANCE_DEFAULT, message_p);
}

//------------------------------------------------------------------------------
void scenario_set_status(scenario_t * const scenario, const scenario_status_t scenario_status)
{
  scenario->status = scenario_status;
  if (SCENARIO_STATUS_PLAY_SUCCESS == scenario_status) {
    OAILOG_NOTICE (LOG_MME_SCENARIO_PLAYER, "Run scenario %s SUCCESS\n", bdata(scenario->name));
  } else if (SCENARIO_STATUS_PLAY_FAILED == scenario_status) {
    OAILOG_ERROR (LOG_MME_SCENARIO_PLAYER, "Run scenario %s FAILED\n", bdata(scenario->name));
  } else if (SCENARIO_STATUS_LOAD_FAILED == scenario_status) {
    OAILOG_ERROR (LOG_MME_SCENARIO_PLAYER, "Load scenario %s FAILED\n", bdata(scenario->name));
  }
}

//------------------------------------------------------------------------------
static void msp_clear_processed_flags(scenario_t * const scenario)
{
  scenario_player_item_t *item = scenario->head_item;
  while (item) {
    if (SCENARIO_PLAYER_ITEM_ITTI_MSG == item->item_type) {
      item->u.msg.is_processed = false;
      item->u.msg.time_stamp.tv_sec = 0;
      item->u.msg.time_stamp.tv_usec = 0;
    }
    item = item->next_item;
  }
}
//------------------------------------------------------------------------------
void msp_handle_timer_expiry (struct timer_has_expired_s * const timer_has_expired)
{
  if (timer_has_expired->arg) {
    scenario_player_timer_arg_t *arg = (scenario_player_timer_arg_t*)timer_has_expired->arg;

    pthread_mutex_lock(&arg->scenario->lock);
    arg->scenario->num_timers    -= 1;

    if (SCENARIO_STATUS_PAUSED == arg->scenario->status) {
      scenario_set_status(arg->scenario, SCENARIO_STATUS_PLAYING);
    }

    if (SCENARIO_PLAYER_ITEM_ITTI_MSG == arg->item->item_type) {
      // we do not use timer for tx message anymore
      if (arg->item->u.msg.is_tx) {
        msp_send_tx_message_no_delay(arg->scenario, arg->item);
      } else {
        // rx message

        // obviously if everything is ok, this timer should have been deleted, but
        // timer_remove do not free the arg parameter, pffff, so let it run...
        if (!(arg->item->u.msg.is_processed)) {
          // scenario fails
          scenario_set_status(arg->scenario, SCENARIO_STATUS_PLAY_FAILED);
        }
      }
    }
    free_wrapper((void**)&arg);

    // trigger continuation of the scenario
    pthread_mutex_unlock(&arg->scenario->lock);
    msp_scenario_tick(arg->scenario);
  }
}

//------------------------------------------------------------------------------
void msp_get_elapsed_time_since_scenario_start(scenario_t * const scenario, struct timeval * const elapsed_time)
{
  struct timeval now = {0};
  // no thread safe but do not matter a lot
  gettimeofday(&now, NULL);
  timersub(&now, &scenario->started, elapsed_time);
}

//------------------------------------------------------------------------------
void msp_process_vars_to_load(scenario_t * const scenario, scenario_player_item_t * const msg_rx_item)
{
  AssertFatal (0, "TODO");
}

//------------------------------------------------------------------------------
bool msp_send_tx_message_no_delay(scenario_t * const scenario, scenario_player_item_t * const item)
{
  if (item->u.msg.xml_dump2struct_needed) {
    // TODO dump XML to ITTI
    item->u.msg.xml_dump2struct_needed = false;
    int rc = msp_reload_message (scenario, item);
    AssertFatal(RETURNok == rc, "Could not reload message");
  } else {
    // finally reload message because of the free of item->u.msg.itti_msg
    int rc = msp_reload_message (scenario, item);
    AssertFatal(RETURNok == rc, "Could not reload message");
  }
  item->u.msg.is_processed = true;
  msp_get_elapsed_time_since_scenario_start(scenario, &item->u.msg.time_stamp);
  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "ITTI msg id %d -> task %s\n", item->u.msg.itti_msg->ittiMsgHeader.messageId,
      itti_task_id2itti_task_str(item->u.msg.itti_receiver_task));
  int rc = itti_send_msg_to_task (item->u.msg.itti_receiver_task, INSTANCE_DEFAULT, item->u.msg.itti_msg);

  item->u.msg.itti_msg = NULL;
  scenario->last_played_item = item;
  return (RETURNok == rc);
}

//------------------------------------------------------------------------------
// return true if we can continue playing the scenario (no timer)
bool msp_play_tx_message(scenario_t * const scenario, scenario_player_item_t * const item)
{
  // time-out relative to itself
  if (item->u.msg.time_out_relative_to_msg_uid == item->uid) {
    item->u.msg.is_processed = false;
    // finally prefer sleep instead of timer+mutex/cond synch
    if (0 < item->u.msg.time_out.tv_sec) {
      sleep(item->u.msg.time_out.tv_sec);
    }
    if (0 < item->u.msg.time_out.tv_usec) {
      usleep(item->u.msg.time_out.tv_usec);
    }
    return msp_send_tx_message_no_delay(scenario, item);
  // time-out relative to another item in the scenario
  } else {
    scenario_player_item_t * ref = NULL;
    // find the relative item
    hashtable_rc_t hrc = hashtable_ts_get (scenario->scenario_items,
        (hash_key_t)item->u.msg.time_out_relative_to_msg_uid, (void **)&ref);

    AssertFatal ((HASH_TABLE_OK == hrc) && (ref), "Could not find relative item UID %d", item->u.msg.time_out_relative_to_msg_uid);
    AssertFatal (SCENARIO_PLAYER_ITEM_ITTI_MSG == ref->item_type, "Bad type relative item UID %d", item->u.msg.time_out_relative_to_msg_uid);

    if (ref->u.msg.is_processed) {
      // compute time
      struct timeval now = {0};
      struct timeval elapsed_time = {0};
      // no thread safe but do not matter a lot
      msp_get_elapsed_time_since_scenario_start(scenario, &now);
      timersub(&now, &ref->u.msg.time_stamp, &elapsed_time);
      if (
          (elapsed_time.tv_sec > item->u.msg.time_out.tv_sec) ||
          ((elapsed_time.tv_sec == item->u.msg.time_out.tv_sec) &&
           (elapsed_time.tv_usec >= item->u.msg.time_out.tv_usec))
         ) {
        return msp_send_tx_message_no_delay(scenario, item);
      } else {
        struct timeval timer_val = {0};
        timersub(&elapsed_time, &item->u.msg.time_out, &timer_val);
        // finally prefer sleep instead of timer+mutex/cond synch
        if (0 < timer_val.tv_sec) {
          sleep(timer_val.tv_sec);
        }
        if (0 < timer_val.tv_usec) {
          usleep(timer_val.tv_usec);
        }
        return msp_send_tx_message_no_delay(scenario, item);
      }
    } else {
      return false;
    }
  }
  return true;
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario (no timer)
bool msp_play_rx_message(scenario_t * const scenario, scenario_player_item_t * const item)
{
  if (item->u.msg.is_processed) {
    return true;
  }

  if (item->u.msg.time_out_relative_to_msg_uid != item->uid) {
    scenario_player_item_t * ref = NULL;
    // find the relative item
    hashtable_rc_t hrc = hashtable_ts_get (scenario->scenario_items,
        (hash_key_t)item->u.msg.time_out_relative_to_msg_uid, (void **)&ref);

    AssertFatal ((HASH_TABLE_OK == hrc) && (ref), "Could not find relative item UID %d", item->u.msg.time_out_relative_to_msg_uid);
    AssertFatal (SCENARIO_PLAYER_ITEM_ITTI_MSG == ref->item_type, "Bad type relative item UID %d", item->u.msg.time_out_relative_to_msg_uid);
    if (ref->u.msg.is_processed) {
      struct timeval now = {0};
      struct timeval elapsed_time = {0};
      // no thread safe but do not matter a lot
      msp_get_elapsed_time_since_scenario_start(scenario, &now);
      timersub(&now, &ref->u.msg.time_stamp, &elapsed_time);
      if (
          (elapsed_time.tv_sec > item->u.msg.time_out.tv_sec) ||
          ((elapsed_time.tv_sec == item->u.msg.time_out.tv_sec) &&
           (elapsed_time.tv_usec >= item->u.msg.time_out.tv_usec))
         ) {
        scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED);
        return false;
      } else {
        scenario_set_status(scenario, SCENARIO_STATUS_PAUSED);

        struct timeval timer_val = {0};
        int ret      = RETURNerror;
        timersub(&elapsed_time, &item->u.msg.time_out, &timer_val);
        scenario_player_timer_arg_t *arg = calloc(1, sizeof (*arg));
        if (arg) {
          arg->item = item;
          arg->scenario = scenario;
          ret = timer_setup (timer_val.tv_sec, timer_val.tv_usec,
                     TASK_MME_SCENARIO_PLAYER, INSTANCE_DEFAULT,
                     TIMER_ONE_SHOT, (void*)arg, &item->u.msg.timer_id);
        }
        AssertFatal(RETURNok == ret, "Error setting timer item %d", item->uid);
        return false;
      }
    } else {
      scenario_set_status(scenario, SCENARIO_STATUS_PAUSED);
      return false;
    }
  } else {
    if ((0 < item->u.msg.time_out.tv_sec) ||
        (0 < item->u.msg.time_out.tv_usec)) {
      scenario_player_timer_arg_t *arg = calloc(1, sizeof (*arg));
      if (arg) {
        arg->item = item;
        arg->scenario = scenario;
        int ret = timer_setup (item->u.msg.time_out.tv_sec, item->u.msg.time_out.tv_usec,
                   TASK_MME_SCENARIO_PLAYER, INSTANCE_DEFAULT,
                   TIMER_ONE_SHOT, (void*)arg, &item->u.msg.timer_id);
        AssertFatal(0 == ret, "Error setting timer item %d", item->uid);
      }
      return true;
    } else {
      scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED);
      return false;
    }
  }
  return true;
}
//------------------------------------------------------------------------------
void msp_var_notify_listeners (scenario_player_item_t * const item)
{
  AssertFatal (SCENARIO_PLAYER_ITEM_VAR == item->item_type, "Bad type var item type %d", item->item_type);
  struct list_item_s *value_changed_subscribers = item->u.var.value_changed_subscribers;
  while (value_changed_subscribers) {
    struct scenario_player_item_s *item = value_changed_subscribers->item;
    if (SCENARIO_PLAYER_ITEM_ITTI_MSG == item->item_type) {
      item->u.msg.xml_dump2struct_needed = true;
      OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Notify scenario player item UID %u to be reloaded\n", item->uid);
    }
    value_changed_subscribers = value_changed_subscribers->next;
  }
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_var(scenario_t * const scenario, scenario_player_item_t * const item)
{
  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "var %s\n", item->u.var.name->data);
  if (item->u.var.var_ref_uid) {
    // get ref var value
    scenario_player_item_t * spi = NULL;
    hashtable_rc_t hrc = hashtable_ts_get (scenario->scenario_items,
        (hash_key_t)item->u.var.var_ref_uid, (void **)&spi);
    AssertFatal ((HASH_TABLE_OK == hrc) && (spi), "Could not find var item UID %d", item->u.var.var_ref_uid);
    AssertFatal (SCENARIO_PLAYER_ITEM_VAR == spi->item_type, "Bad type %d var item UID %d", spi->item_type, item->u.var.var_ref_uid);
    AssertFatal (spi->u.var.value_type == item->u.var.value_type, "var types to not match %d != %d, discouraged to do so in scenario", spi->u.var.value_type, item->u.var.value_type);
    if (VAR_VALUE_TYPE_INT64 == spi->u.var.value_type) {
      if (item->u.var.value.value_64 != spi->u.var.value.value_64) {
        item->u.var.value_changed = true;
        item->u.var.value.value_64 = spi->u.var.value.value_64;
      }
    } else if (VAR_VALUE_TYPE_BSTR == spi->u.var.value_type) {

      if ((item->u.var.value.value_bstr) && (spi->u.var.value.value_bstr)) {
        if (blength(item->u.var.value.value_bstr) != blength(spi->u.var.value.value_bstr)) {
          item->u.var.value_changed = true;
        } else if (memcmp(item->u.var.value.value_bstr->data, spi->u.var.value.value_bstr->data, blength(item->u.var.value.value_bstr))) {
          item->u.var.value_changed = true;
        }
        if (item->u.var.value_changed) {
          bassign(item->u.var.value.value_bstr, spi->u.var.value.value_bstr);
        }
      } else if (!(item->u.var.value.value_bstr) && (spi->u.var.value.value_bstr)) {
        item->u.var.value_changed = true;
        item->u.var.value.value_bstr = bstrcpy(spi->u.var.value.value_bstr);
      } else {
        AssertFatal(0, "This case should not happen");
      }
    } else if (spi->u.var.var_ref_uid) {
      AssertFatal(0, "TODO but discouraged to do so in scenario");
    } else {
      AssertFatal(0, "Unknown var value type %d", spi->u.var.value_type);
    }
  }
  if (item->u.var.value_changed) {
    msp_var_notify_listeners(item);
    item->u.var.value_changed = false;
  }
  scenario->last_played_item = item;
  return true;
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_set_var(scenario_t * const scenario, scenario_player_item_t * const item)
{
  scenario_player_item_t * var_item = NULL;

  // find the relative item
  hashtable_rc_t hrc = hashtable_ts_get (scenario->scenario_items,
      (hash_key_t)item->u.set_var.var_uid, (void **)&var_item);
  AssertFatal ((HASH_TABLE_OK == hrc) && (var_item), "Could not find var item UID %d", item->u.set_var.var_uid);
  AssertFatal (SCENARIO_PLAYER_ITEM_VAR == var_item->item_type, "Bad type var item UID %d", var_item->item_type);

  if (item->u.set_var.var_ref_uid) {
    // get ref var value
    scenario_player_item_t * var_ref = NULL;
    hashtable_rc_t hrc = hashtable_ts_get (scenario->scenario_items,
        (hash_key_t)item->u.set_var.var_ref_uid, (void **)&var_ref);
    AssertFatal ((HASH_TABLE_OK == hrc) && (var_ref), "Could not find var ref item UID %d", item->u.set_var.var_ref_uid);
    AssertFatal (SCENARIO_PLAYER_ITEM_VAR == var_ref->item_type, "Bad type var ref item UID %d", var_ref->item_type);
    AssertFatal (var_ref->u.var.value_type == var_item->u.var.value_type, "var types to not match %d != %d, discouraged to do so in scenario", var_ref->u.var.value_type, var_item->u.var.value_type);
    if (VAR_VALUE_TYPE_INT64 == var_ref->u.var.value_type) {
      if (var_item->u.var.value.value_64 != var_ref->u.var.value.value_64) {
        var_item->u.var.value_changed = true;
        var_item->u.var.value.value_64 = var_ref->u.var.value.value_64;
        OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "set var %s=%"PRIx64"\n", var_item->u.var.name->data, var_item->u.var.value.value_64);
      } else {
        OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "set var %s=%"PRIx64" (unchanged)\n", var_item->u.var.name->data, var_item->u.var.value.value_64);
      }
    } else if (VAR_VALUE_TYPE_BSTR == var_ref->u.var.value_type) {

      if ((var_item->u.var.value.value_bstr) && (var_ref->u.var.value.value_bstr)) {
        if (blength(var_item->u.var.value.value_bstr) != blength(var_ref->u.var.value.value_bstr)) {
          var_item->u.var.value_changed = true;
        } else if (memcmp(var_item->u.var.value.value_bstr->data, var_ref->u.var.value.value_bstr->data, blength(var_item->u.var.value.value_bstr))) {
          var_item->u.var.value_changed = true;
        }
        if (var_item->u.var.value_changed) {
          bassign(var_item->u.var.value.value_bstr, var_ref->u.var.value.value_bstr);
        }
      } else if (!(var_item->u.var.value.value_bstr) && (var_ref->u.var.value.value_bstr)) {
        var_item->u.var.value_changed = true;
        var_item->u.var.value.value_bstr = bstrcpy(var_ref->u.var.value.value_bstr);
      } else {
        AssertFatal(0, "This case should not happen");
      }
    } else if (var_ref->u.var.var_ref_uid) {
      AssertFatal(0, "TODO but discouraged to do so in scenario");
    } else {
      AssertFatal(0, "Unknown var value type %d", var_ref->u.var.value_type);
    }
  }
  if (var_item->u.var.value_changed) {
    msp_var_notify_listeners(var_item);
    var_item->u.var.value_changed = false;
  }
  scenario->last_played_item = item;
  return true;
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_incr_var(scenario_t * const scenario, scenario_player_item_t * const item)
{
  scenario_player_item_t * ref = NULL;
  // find the relative item
  hashtable_rc_t hrc = hashtable_ts_get (scenario->scenario_items,
      (hash_key_t)item->u.uid_decr_var, (void **)&ref);
  AssertFatal ((HASH_TABLE_OK == hrc) && (ref), "Could not find var item UID %d", item->u.uid_incr_var);
  AssertFatal (SCENARIO_PLAYER_ITEM_VAR == ref->item_type, "Bad type var item UID %d", ref->item_type);
  AssertFatal ((VAR_VALUE_TYPE_INT64 == ref->u.var.value_type), "Bad var type %d", ref->u.var.value_type);

  ref->u.var.value.value_u64 += 1;
  ref->u.var.value_changed = true;

  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "incr var %s=%"PRIu64 "\n", ref->u.var.name->data, ref->u.var.value.value_u64);
  msp_var_notify_listeners(ref);
  scenario->last_played_item = item;
  return true;
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_decr_var(scenario_t * const scenario, scenario_player_item_t * const item)
{
  scenario_player_item_t * ref = NULL;
  // find the relative item
  hashtable_rc_t hrc = hashtable_ts_get (scenario->scenario_items,
      (hash_key_t)item->u.uid_decr_var, (void **)&ref);
  AssertFatal ((HASH_TABLE_OK == hrc) && (ref), "Could not find var item UID %d", item->u.uid_decr_var);
  AssertFatal (SCENARIO_PLAYER_ITEM_VAR == ref->item_type, "Bad type var item UID %d", ref->item_type);
  AssertFatal ((VAR_VALUE_TYPE_INT64 == ref->u.var.value_type), "Bad var type %d", ref->u.var.value_type);

  ref->u.var.value.value_u64 -= 1;
  ref->u.var.value_changed = true;

  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "decr var %s=%"PRIu64 "\n", ref->u.var.name->data, ref->u.var.value.value_u64);
  msp_var_notify_listeners(ref);
  scenario->last_played_item = item;
  return true;
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_sleep(scenario_t * const scenario, scenario_player_item_t * const item)
{

  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "sleep %d.%d seconds\n", item->u.sleep.seconds, item->u.sleep.useconds);
  if (0 < item->u.sleep.seconds) {
    sleep(item->u.sleep.seconds);
  }
  if (0 < item->u.sleep.useconds) {
    sleep(item->u.sleep.useconds);
  }
  scenario->last_played_item = item;
  return true;
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_jump_cond(scenario_t * const scenario, scenario_player_item_t * const item)
{
  scenario_player_item_t * ref = NULL;
  // find the relative item
  hashtable_rc_t hrc = hashtable_ts_get (scenario->scenario_items,
      (hash_key_t)item->u.cond.var_uid, (void **)&ref);

  AssertFatal ((HASH_TABLE_OK == hrc) && (ref), "Could not find var item UID %d", item->u.cond.var_uid);
  AssertFatal (SCENARIO_PLAYER_ITEM_VAR == ref->item_type, "Bad type var item UID %d", ref->item_type);
  bool test = false;
  switch (item->u.cond.test_operator) {
  case TEST_EQ:
    test = (ref->u.var.value.value_u64 == item->u.cond.var_test_value);
    break;
  case TEST_NE:
    test = (ref->u.var.value.value_u64 != item->u.cond.var_test_value);
    break;
  case TEST_GT:
    test = (ref->u.var.value.value_u64 > item->u.cond.var_test_value);
   break;
  case TEST_GE:
    test = (ref->u.var.value.value_u64 >= item->u.cond.var_test_value);
    break;
  case TEST_LT:
    test = (ref->u.var.value.value_u64 < item->u.cond.var_test_value);
    break;
  case TEST_LE:
    test = (ref->u.var.value.value_u64 <= item->u.cond.var_test_value);
    break;
  default:
    AssertFatal(0, "TEST unknown");
  }
  if (test) {
    // jump
    hrc = hashtable_ts_get (scenario->scenario_items,
        (hash_key_t)item->u.cond.jump_label_uid, (void **)&ref);
    AssertFatal ((HASH_TABLE_OK == hrc) && (ref), "Could not find var item UID %d", item->u.cond.jump_label_uid);
    AssertFatal (SCENARIO_PLAYER_ITEM_LABEL == ref->item_type, "Bad type var item UID %d", ref->item_type);
    scenario->last_played_item = ref;

    // if backward jump, clear processed flag for all items
    if (item->uid > ref->uid) {
      msp_clear_processed_flags (scenario);
    }

    OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "jump to label %s\n", ref->u.label->data);
  } else {
    scenario->last_played_item = item;
  }
  return true;
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_compute_authentication_response_parameter(scenario_t * const scenario, scenario_player_item_t * const item)
{
  scenario_player_item_t * var_rand = NULL;
  scenario_player_item_t * var_autn = NULL;
  scenario_player_item_t * var_auth_param = NULL;

  void                   * vuid_rand = NULL;
  void                   * vuid_autn = NULL;
  void                   * vuid_auth_param = NULL;


  hashtable_rc_t hrc = obj_hashtable_ts_get (scenario->var_items, "AUTN", strlen("AUTN"), (void **)&vuid_autn);
  AssertFatal(HASH_TABLE_OK == hrc, "Error in getting var AUTN in hashtable");
  int uid_autn = (int)(uintptr_t)vuid_autn;
  hrc = hashtable_ts_get (scenario->scenario_items, uid_autn, (void **)&var_autn);
  AssertFatal ((HASH_TABLE_OK == hrc) && (var_autn), "Could not find var item UID %d", uid_autn);

  hrc = obj_hashtable_ts_get (scenario->var_items, "RAND", strlen("RAND"), (void **)&vuid_rand);
  AssertFatal(HASH_TABLE_OK == hrc, "Error in getting var RAND in hashtable");
  int uid_rand = (int)(uintptr_t)vuid_rand;
  hrc = hashtable_ts_get (scenario->scenario_items, uid_rand, (void **)&var_rand);
  AssertFatal ((HASH_TABLE_OK == hrc) && (var_rand), "Could not find var item UID %d", uid_rand);

  hrc = obj_hashtable_ts_get (scenario->var_items, "AUTHENTICATION_RESPONSE_PARAMETER", strlen("AUTHENTICATION_RESPONSE_PARAMETER"), (void **)&vuid_auth_param);
  AssertFatal(HASH_TABLE_OK == hrc, "Error in getting var AUTHENTICATION_RESPONSE_PARAMETER in hashtable");
  int uid_auth_param = (int)(uintptr_t)vuid_auth_param;
  hrc = hashtable_ts_get (scenario->scenario_items, uid_auth_param, (void **)&var_auth_param);
  AssertFatal ((HASH_TABLE_OK == hrc) && (var_rand), "Could not find var item UID %d", uid_auth_param);

  memcpy(scenario->usim_data.autn, var_autn->u.var.value.value_bstr->data, USIM_AUTN_SIZE);
  memcpy(scenario->usim_data.rand, var_rand->u.var.value.value_bstr->data, USIM_RAND_SIZE);
  memset(scenario->usim_data.res, 0, USIM_RES_SIZE);

  int rc = usim_authenticate(&scenario->usim_data,
      scenario->usim_data.rand,
      scenario->usim_data.autn,
      scenario->usim_data.auts,
      scenario->usim_data.res,
      ((emm_security_context_t*)scenario->ue_emulated_emm_security_context)->knas_enc,
      ((emm_security_context_t*)scenario->ue_emulated_emm_security_context)->knas_int);
  if (RETURNok == rc) {
    bdestroy_wrapper(&var_auth_param->u.var.value.value_bstr);
    var_auth_param->u.var.value.value_bstr = blk2bstr(scenario->usim_data.res, 8);
    msp_var_notify_listeners(var_auth_param);
    scenario->last_played_item = item;
    OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "msp_play_compute_authentication_response_parameter succeeded\n");
    return true;
  } else {
    scenario->last_played_item = item;
    OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "msp_play_compute_authentication_response_parameter failed\n");
    return false;
  }
}

//------------------------------------------------------------------------------
void msp_init_scenario(scenario_t * const s)
{
  if (s) {
    s->last_played_item      = NULL;
    int rc = gettimeofday(&s->started, NULL);
    AssertFatal(0 == rc, "gettimeofday()");
    pthread_mutex_init(&s->lock, NULL);
    s->scenario_items = hashtable_ts_create (128, HASH_TABLE_DEFAULT_HASH_FUNC, NULL /* TODO*/, NULL);
    s->var_items      = obj_hashtable_ts_create (32, HASH_TABLE_DEFAULT_HASH_FUNC, NULL , hash_free_int_func, NULL);
    s->label_items    = obj_hashtable_ts_create (8, HASH_TABLE_DEFAULT_HASH_FUNC, NULL , hash_free_int_func, NULL);
    s->ue_emulated_emm_security_context = calloc(1, sizeof(emm_security_context_t));
    scenario_set_status(s, SCENARIO_STATUS_NULL);
    s->num_timers    = 0;
  }
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario (no timer)
bool msp_play_item(scenario_t * const scenario, scenario_player_item_t * const item)
{
  if (item) {
    if ((SCENARIO_STATUS_PLAY_FAILED == scenario->status)  || (SCENARIO_STATUS_PLAY_SUCCESS == scenario->status)) {
      return false;
    }
    OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Play item UID %u\n", item->uid);

    if (SCENARIO_PLAYER_ITEM_ITTI_MSG == item->item_type) {
      scenario_player_msg_t * msg = &item->u.msg;
      if (msg->is_tx) {
        return msp_play_tx_message(scenario, item);
      } else {
        return msp_play_rx_message(scenario, item);
      }
    } else if (SCENARIO_PLAYER_ITEM_LABEL == item->item_type) {
      //DO NOTHING
      scenario->last_played_item = item;
      return true;
    }  else if (SCENARIO_PLAYER_ITEM_EXIT == item->item_type) {
      //TODO refine exit process for scenario
      scenario->last_played_item = item;
      OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Exiting scenario %s\n", bdata(scenario->name));
      if (item->u.exit) {
        scenario_set_status(scenario, item->u.exit);
      }
      return false;
    } else if (SCENARIO_PLAYER_ITEM_VAR == item->item_type) {
      return msp_play_var(scenario, item);
    } else if (SCENARIO_PLAYER_ITEM_VAR_SET == item->item_type) {
      return msp_play_set_var(scenario, item);
    } else if (SCENARIO_PLAYER_ITEM_VAR_INCR == item->item_type) {
      return msp_play_incr_var(scenario, item);
    } else if (SCENARIO_PLAYER_ITEM_VAR_DECR == item->item_type) {
      return msp_play_decr_var(scenario, item);
    } else if (SCENARIO_PLAYER_ITEM_JUMP_COND == item->item_type) {
      return msp_play_jump_cond(scenario, item);
    } else if (SCENARIO_PLAYER_ITEM_SLEEP == item->item_type) {
      return msp_play_sleep(scenario, item);
    } else if (SCENARIO_PLAYER_ITEM_COMPUTE_AUTHENTICATION_RESPONSE_PARAMETER == item->item_type) {
      return msp_play_compute_authentication_response_parameter(scenario, item);
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void msp_run_scenario(scenario_t * const scenario)
{
  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Run scenario %s %p\n", bdata(scenario->name), scenario);
  if (scenario) {
    if ((SCENARIO_STATUS_PLAY_FAILED != scenario->status) && (SCENARIO_STATUS_PLAY_SUCCESS != scenario->status)) {
      pthread_mutex_lock(&scenario->lock);
      g_msp_scenarios.current_scenario = scenario;
      scenario_player_item_t *item = NULL;

      if (SCENARIO_STATUS_PLAYING == scenario->status) {
        item = scenario->last_played_item->next_item;
        OAILOG_NOTICE (LOG_MME_SCENARIO_PLAYER, "Continue playing scenario %s %p\n", bdata(scenario->name), scenario);
      } else if (SCENARIO_STATUS_LOADED == scenario->status) {
        item = scenario->head_item;
        scenario->status = SCENARIO_STATUS_PLAYING;
        OAILOG_NOTICE (LOG_MME_SCENARIO_PLAYER, "Playing scenario %s %p\n", bdata(scenario->name), scenario);
      }
      if (item) {
        bool res = msp_play_item(scenario, item);
        pthread_mutex_unlock(&scenario->lock);
        if (SCENARIO_STATUS_PAUSED != scenario->status) {
          msp_scenario_tick(scenario);
        }
      } else {
        if (!(scenario->num_timers) && (SCENARIO_STATUS_PLAYING == scenario->status)) {
          scenario->status = SCENARIO_STATUS_PLAY_SUCCESS;
          OAILOG_NOTICE (LOG_MME_SCENARIO_PLAYER, "Run scenario %s SUCCESS\n", bdata(scenario->name));
          pthread_mutex_unlock(&scenario->lock);
          if (scenario->next_scenario) {
            msp_scenario_tick(scenario->next_scenario);
          } else {
            // end of scenario player, no more scenarios to play
            itti_send_terminate_message (TASK_MME_SCENARIO_PLAYER);
          }
        } else { // wait a RX message ?
          pthread_mutex_unlock(&scenario->lock);
        }
      }
    } else {
      if (scenario->next_scenario) {
        msp_scenario_tick(scenario->next_scenario);
      } else {
        // end of scenario player, no more scenarios to play
        //itti_send_terminate_message (TASK_MME_SCENARIO_PLAYER);
        itti_terminate_tasks (TASK_MME_SCENARIO_PLAYER);
      }
    }
  }
}


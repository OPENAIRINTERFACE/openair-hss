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
#include "mme_scenario_player.h"
#include "xml_msg_dump_itti.h"
#include "usim_authenticate.h"
#include "secu_defs.h"
#include "mme_config.h"
#include "mme_scenario_player_task.h"
#include "sp_xml_load.h"

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
void scenario_set_status(scenario_t * const scenario, const scenario_status_t scenario_status, char* caller_file, int caller_line)
{
  scenario->status = scenario_status;
  if (SCENARIO_STATUS_PLAY_SUCCESS == scenario_status) {
    OAILOG_NOTICE (LOG_MME_SCENARIO_PLAYER, "Result Run scenario %s SUCCESS\n", bdata(scenario->name));
  } else if (SCENARIO_STATUS_PLAY_FAILED == scenario_status) {
    OAILOG_ERROR (LOG_MME_SCENARIO_PLAYER, "Result Run scenario %s FAILED set by %s:%d\n", bdata(scenario->name), caller_file, caller_line);
  } else if (SCENARIO_STATUS_LOAD_FAILED == scenario_status) {
    OAILOG_ERROR (LOG_MME_SCENARIO_PLAYER, "Result Load scenario %s FAILED set by %s:%d\n", bdata(scenario->name), caller_file, caller_line);
  } else if (SCENARIO_STATUS_PAUSED == scenario_status) {
    OAILOG_NOTICE (LOG_MME_SCENARIO_PLAYER, "Run scenario %s PAUSED set by %s:%d\n", bdata(scenario->name), caller_file, caller_line);
  } else if (SCENARIO_STATUS_LOADED == scenario_status) {
    OAILOG_NOTICE (LOG_MME_SCENARIO_PLAYER, "Run scenario %s LOADED\n", bdata(scenario->name));
  }
}

//------------------------------------------------------------------------------
static void msp_clear_processed_flags(scenario_t * const scenario)
{
  scenario_player_item_t *item = scenario->head_item;
  while (item) {
    item->is_played = false;
    if (SCENARIO_PLAYER_ITEM_ITTI_MSG == item->item_type) {
      item->u.msg.time_stamp.tv_sec = 0;
      item->u.msg.time_stamp.tv_usec = 0;
      item->u.msg.timer_id = 0;
      item->u.msg.timer_arg.item = NULL;
      item->u.msg.timer_arg.scenario = NULL;
    } else if (SCENARIO_PLAYER_ITEM_SCENARIO == item->item_type) {
      msp_clear_processed_flags(item->u.scenario);
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


    if (SCENARIO_PLAYER_ITEM_ITTI_MSG == arg->item->item_type) {
      // we do not use timer for tx message anymore
      if (arg->item->u.msg.is_tx) {
        if (SCENARIO_STATUS_PAUSED == arg->scenario->status) {
          scenario_set_status(arg->scenario, SCENARIO_STATUS_PLAYING, __FILE__, __LINE__);
        }
        msp_send_tx_message_no_delay(arg->scenario, arg->item);
        // trigger continuation of the scenario
        pthread_mutex_unlock(&arg->scenario->lock);
        msp_scenario_tick(arg->scenario);
      } else {
        // rx message
        // obviously if everything is ok, this timer should have been deleted
        if (!arg->item->is_played) {
          OAILOG_ERROR (LOG_MME_SCENARIO_PLAYER, "ITTI msg uid %d timed-out(%ld.%06ld sec)\n",
              arg->item->uid, arg->item->u.msg.time_out.tv_sec, arg->item->u.msg.time_out.tv_usec);
          scenario_set_status(arg->scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
          // trigger continuation of the scenario
          pthread_mutex_unlock(&arg->scenario->lock);
          msp_scenario_tick(arg->scenario);
        } else {
          pthread_mutex_unlock(&arg->scenario->lock);
        }
        arg->item->u.msg.timer_id = 0;
      }
    }
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
bool msp_send_tx_message_no_delay(scenario_t * const scenario, scenario_player_item_t * const item)
{

  int rc = msp_reload_message (scenario, item);
  AssertFatal(RETURNok == rc, "Could not reload message");
  item->is_played = true;
  scenario->last_played_item = item;
  msp_get_elapsed_time_since_scenario_start(scenario, &item->u.msg.time_stamp);
  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "ITTI msg id %d -> task %s\n", item->u.msg.itti_msg->ittiMsgHeader.messageId,
      itti_task_id2itti_task_str(item->u.msg.itti_receiver_task));
  rc = itti_send_msg_to_task (item->u.msg.itti_receiver_task, INSTANCE_DEFAULT, item->u.msg.itti_msg);

  item->u.msg.itti_msg = NULL;
  return (RETURNok == rc);
}

//------------------------------------------------------------------------------
// return true if we can continue playing the scenario (no timer)
bool msp_play_tx_message(scenario_t * const scenario, scenario_player_item_t * const item)
{
  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Play item TX message  UID %u\n", item->uid);
  // time-out relative to itself
  if (item->u.msg.time_out_relative_to_msg_uid == item->uid) {
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

    if (!ref->is_played) {
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
  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Play item RX message  UID %u\n", item->uid);
  if (item->is_played) {
    OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "item RX message  UID %u already played\n", item->uid);
    scenario_set_status(scenario, SCENARIO_STATUS_PAUSED, __FILE__, __LINE__);
    return true;
  }
  if (item->u.msg.timer_id) {
    OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "item RX message  UID %u timer set (%lx)\n", item->uid, item->u.msg.timer_id);
    return false;
  }

  if (item->u.msg.time_out_relative_to_msg_uid != item->uid) {
    scenario_player_item_t * ref = NULL;
    // find the relative item
    hashtable_rc_t hrc = hashtable_ts_get (scenario->scenario_items,
        (hash_key_t)item->u.msg.time_out_relative_to_msg_uid, (void **)&ref);

    AssertFatal ((HASH_TABLE_OK == hrc) && (ref), "Could not find relative item UID %d", item->u.msg.time_out_relative_to_msg_uid);
    AssertFatal (SCENARIO_PLAYER_ITEM_ITTI_MSG == ref->item_type, "Bad type relative item UID %d", item->u.msg.time_out_relative_to_msg_uid);
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
      scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
      return false;
    } else {

      struct timeval timer_val = {0};
      int ret      = RETURNerror;
      timersub(&item->u.msg.time_out, &elapsed_time, &timer_val);
      item->u.msg.timer_arg.item = item;
      item->u.msg.timer_arg.scenario = scenario;
      ret = timer_setup (timer_val.tv_sec, timer_val.tv_usec,
          TASK_MME_SCENARIO_PLAYER, INSTANCE_DEFAULT,
          TIMER_ONE_SHOT, (void*)&item->u.msg.timer_arg, &item->u.msg.timer_id);

      scenario_set_status(scenario, SCENARIO_STATUS_PAUSED, __FILE__, __LINE__);
      AssertFatal(RETURNok == ret, "Error setting timer item %d", item->uid);
      OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Setting timer %lx %ld.%06ld sec\n", item->u.msg.timer_id, timer_val.tv_sec, timer_val.tv_usec);
      return false;
    }
  } else {
    if ((0 < item->u.msg.time_out.tv_sec) ||
        (0 < item->u.msg.time_out.tv_usec)) {
      item->u.msg.timer_arg.item = item;
      item->u.msg.timer_arg.scenario = scenario;
      int ret = timer_setup (item->u.msg.time_out.tv_sec, item->u.msg.time_out.tv_usec,
          TASK_MME_SCENARIO_PLAYER, INSTANCE_DEFAULT,
          TIMER_ONE_SHOT, (void*)&item->u.msg.timer_arg, &item->u.msg.timer_id);
      scenario_set_status(scenario, SCENARIO_STATUS_PAUSED, __FILE__, __LINE__);
      AssertFatal(0 == ret, "Error setting timer item %d", item->uid);
      OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Setting timer %lx %ld.%06ld sec\n", item->u.msg.timer_id, item->u.msg.time_out.tv_sec, item->u.msg.time_out.tv_usec);
      return true;
    } else {
      scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED, __FILE__, __LINE__);
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
void msp_display_var(scenario_player_item_t * const var)
{
  if (VAR_VALUE_TYPE_INT64 == var->u.var.value_type) {
    OAILOG_DEBUG (LOG_MME_SCENARIO_PLAYER, "Var %s=%"PRIx64"\n", var->u.var.name->data, var->u.var.value.value_64);
  } else if (VAR_VALUE_TYPE_ASCII_STREAM == var->u.var.value_type) {
    if (var->u.var.value.value_bstr)
      OAILOG_DEBUG (LOG_MME_SCENARIO_PLAYER, "Var %s=\"%s\"\n", var->u.var.name->data, var->u.var.value.value_bstr->data);
    else
      OAILOG_DEBUG (LOG_MME_SCENARIO_PLAYER, "Var %s=\"\"\n", var->u.var.name->data);
  } else if (VAR_VALUE_TYPE_HEX_STREAM == var->u.var.value_type) {
    int length = blength(var->u.var.value.value_bstr);
    char *buffer=malloc(blength(var->u.var.value.value_bstr)*2+1);
    if (buffer) {
      hexa_to_ascii ((uint8_t *)bdata(var->u.var.value.value_bstr), buffer,length);
      buffer[2*length]=0;
      OAILOG_DEBUG (LOG_MME_SCENARIO_PLAYER, "Var %s=%s (hex stream)\n", var->u.var.name->data, buffer);
      free_wrapper((void**)&buffer);
    }
  } else
    OAILOG_DEBUG (LOG_MME_SCENARIO_PLAYER, "Var %s unknown type %d\n", var->u.var.name->data, var->u.var.value_type);
}

//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_var(scenario_t * const scenario, scenario_player_item_t * const item)
{
  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Play item var name %s  UID %u\n", item->u.var.name->data, item->uid);
  if (item->u.var.var_ref_uid) {
    // get ref var value
    scenario_player_item_t * spi = NULL;
    bool value_changed = false;
    hashtable_rc_t hrc = hashtable_ts_get (scenario->scenario_items,
        (hash_key_t)item->u.var.var_ref_uid, (void **)&spi);
    AssertFatal ((HASH_TABLE_OK == hrc) && (spi), "Could not find var item UID %d", item->u.var.var_ref_uid);
    AssertFatal (SCENARIO_PLAYER_ITEM_VAR == spi->item_type, "Bad type %d var item UID %d", spi->item_type, item->u.var.var_ref_uid);
    AssertFatal (spi->u.var.value_type == item->u.var.value_type, "var types to not match %d != %d, discouraged to do so in scenario",
                 spi->u.var.value_type, item->u.var.value_type);
    if (VAR_VALUE_TYPE_INT64 == spi->u.var.value_type) {
      if (item->u.var.value.value_64 != spi->u.var.value.value_64) {
        value_changed = true;
        item->u.var.value.value_64 = spi->u.var.value.value_64;
      }
    } else if ((VAR_VALUE_TYPE_HEX_STREAM == spi->u.var.value_type) || (VAR_VALUE_TYPE_ASCII_STREAM == spi->u.var.value_type)) {

      if ((item->u.var.value.value_bstr) && (spi->u.var.value.value_bstr)) {
        if (blength(item->u.var.value.value_bstr) != blength(spi->u.var.value.value_bstr)) {
          value_changed = true;
        } else if (memcmp(item->u.var.value.value_bstr->data, spi->u.var.value.value_bstr->data, blength(item->u.var.value.value_bstr))) {
          value_changed = true;
        }
        if (value_changed) {
          bassign(item->u.var.value.value_bstr, spi->u.var.value.value_bstr);
        }
      } else if (!(item->u.var.value.value_bstr) && (spi->u.var.value.value_bstr)) {
        value_changed = true;
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

  item->is_played = true;
  scenario->last_played_item = item;
  msp_display_var(item);
  return true;
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_set_var(scenario_t * const scenario, scenario_player_item_t * const item)
{
  scenario_player_item_t * var_item = NULL;
  bool value_changed = false;

  // find the relative item
  // Check if var already exists
  var_item = sp_get_var(scenario, (unsigned char *)bdata(item->u.set_var.name));
  AssertFatal(var_item, "var %s not found", bdata(item->u.set_var.name));
  AssertFatal((item->u.set_var.var_uid == var_item->uid), "var %s uid do not match %d/%d", bdata(item->u.set_var.name), item->u.set_var.var_uid, var_item->uid);

  if (item->u.set_var.var_ref_uid) {
    OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Play item set var name %s  UID %u from var UID %u\n", item->u.set_var.name->data, var_item->uid, item->u.set_var.var_ref_uid);
    // get ref var value
    scenario_player_item_t * var_ref = sp_get_var_by_uid(scenario, (hash_key_t)item->u.set_var.var_ref_uid);
    AssertFatal ( (var_ref), "Could not find var ref item UID %d", item->u.set_var.var_ref_uid);
    AssertFatal (SCENARIO_PLAYER_ITEM_VAR == var_ref->item_type, "Bad type var ref item UID %d", var_ref->item_type);
    AssertFatal (var_ref->u.var.value_type == var_item->u.var.value_type, "var types to not match %d != %d, discouraged to do so in scenario", var_ref->u.var.value_type, var_item->u.var.value_type);
    if (VAR_VALUE_TYPE_INT64 == var_ref->u.var.value_type) {
      if (var_item->u.var.value.value_64 != var_ref->u.var.value.value_64) {
        value_changed = true;
        var_item->u.var.value.value_64 = var_ref->u.var.value.value_64;
      }
    } else if ((VAR_VALUE_TYPE_HEX_STREAM == var_ref->u.var.value_type) || (VAR_VALUE_TYPE_ASCII_STREAM == var_ref->u.var.value_type)) {

      if ((var_item->u.var.value.value_bstr) && (var_ref->u.var.value.value_bstr)) {
        if (blength(var_item->u.var.value.value_bstr) != blength(var_ref->u.var.value.value_bstr)) {
          value_changed = true;
        } else if (memcmp(var_item->u.var.value.value_bstr->data, var_ref->u.var.value.value_bstr->data, blength(var_item->u.var.value.value_bstr))) {
          value_changed = true;
        }
        if (value_changed) {
          bassign(var_item->u.var.value.value_bstr, var_ref->u.var.value.value_bstr);
        }
      } else if (!(var_item->u.var.value.value_bstr) && (var_ref->u.var.value.value_bstr)) {
        value_changed = true;
        var_item->u.var.value.value_bstr = bstrcpy(var_ref->u.var.value.value_bstr);
      } else {
        AssertFatal(0, "This case should not happen");
      }
    } else if (var_ref->u.var.var_ref_uid) {
      AssertFatal(0, "TODO but discouraged to do so in scenario");
    } else {
      AssertFatal(0, "Unknown var value type %d", var_ref->u.var.value_type);
    }
  } else {
    AssertFatal (var_item->u.var.value_type == item->u.set_var.value_type, "var type to not match %d != %d, discouraged to do so in scenario",
        var_item->u.var.value_type, item->u.set_var.value_type);

    if (VAR_VALUE_TYPE_INT64 == var_item->u.var.value_type) {
      OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Play item set var name %s  UID %u from value int 64 %" PRIx64"\n", item->u.set_var.name->data, var_item->uid, item->u.set_var.value.value_u64);
      var_item->u.var.value.value_u64 = item->u.set_var.value.value_u64;
    } else if ((VAR_VALUE_TYPE_HEX_STREAM == var_item->u.var.value_type) || (VAR_VALUE_TYPE_ASCII_STREAM == var_item->u.var.value_type)) {

      if ((var_item->u.var.value.value_bstr) && (item->u.set_var.value.value_bstr)) {
        if (blength(var_item->u.var.value.value_bstr) != blength(item->u.set_var.value.value_bstr)) {
          value_changed = true;
        } else if (memcmp(var_item->u.var.value.value_bstr->data,item->u.set_var.value.value_bstr->data, blength(var_item->u.var.value.value_bstr))) {
          value_changed = true;
        }
        if (value_changed) {
          bassign(var_item->u.var.value.value_bstr, item->u.set_var.value.value_bstr);
        }
      } else if (!(var_item->u.var.value.value_bstr) && (item->u.set_var.value.value_bstr)) {
        value_changed = true;
        var_item->u.var.value.value_bstr = bstrcpy(item->u.set_var.value.value_bstr);
      } else {
        AssertFatal(0, "This case should not happen");
      }
    } else {
      AssertFatal(0, "Unknown var value type %d", item->u.set_var.value_type);
    }
  }

  item->is_played = true;
  scenario->last_played_item = item;
  msp_display_var(var_item);
  return true;
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_incr_var(scenario_t * const scenario, scenario_player_item_t * const item)
{
  scenario_player_item_t * ref = NULL;
  // find the relative item
  ref = sp_get_var_by_uid(scenario, (hash_key_t)item->u.uid_incr_var);
  AssertFatal ((ref), "Could not find var item UID %d", item->u.uid_incr_var);
  AssertFatal (SCENARIO_PLAYER_ITEM_VAR == ref->item_type, "Bad type var item UID %d", ref->item_type);
  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Play item incr var %s UID %u\n", ref->u.var.name->data, ref->uid);

  switch (ref->u.var.value_type) {
    case VAR_VALUE_TYPE_INT64:
      ref->u.var.value.value_u64 += 1;
      break;

    case VAR_VALUE_TYPE_ASCII_STREAM: {
        // may be useful for string representing IMSI
        int index  = ref->u.var.value.value_bstr->slen - 1;
        char c = ref->u.var.value.value_bstr->data[index];

        while ((0 <= index) && ('0' <= c) && ( '9' >= c)) {
          c = c + 1;
          if (('0' <= c) && ( '9' >= c)) {
            ref->u.var.value.value_bstr->data[index] = c;
            break;
          } else {
            ref->u.var.value.value_bstr->data[index] = '0';
          }
          index--;
          c = ref->u.var.value.value_bstr->data[index];
        }
      }
      break;

    case VAR_VALUE_TYPE_HEX_STREAM: {
        AssertFatal (0, "TODO incr VAR_VALUE_TYPE_HEX_STREAM");
      }
      break;

    default:
      AssertFatal (0, "TODO incr var type %d", ref->u.var.value_type);
  }

  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "incr var %s=%"PRIu64 "\n", ref->u.var.name->data, ref->u.var.value.value_u64);
  item->is_played = true;
  scenario->last_played_item = item;
  return true;
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_decr_var(scenario_t * const scenario, scenario_player_item_t * const item)
{
  scenario_player_item_t * ref = NULL;
  // find the relative item
  ref = sp_get_var_by_uid(scenario, (hash_key_t)item->u.uid_decr_var);
  AssertFatal ((ref), "Could not find var item UID %d", item->u.uid_decr_var);
  AssertFatal (SCENARIO_PLAYER_ITEM_VAR == ref->item_type, "Bad type var item UID %d", ref->item_type);
  AssertFatal ((VAR_VALUE_TYPE_INT64 == ref->u.var.value_type), "Bad var type %d", ref->u.var.value_type);
  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Play item decr var %s UID %u\n", ref->u.var.name->data, ref->uid);

  ref->u.var.value.value_u64 -= 1;

  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "decr var %s=%"PRIu64 "\n", ref->u.var.name->data, ref->u.var.value.value_u64);
  item->is_played = true;
  scenario->last_played_item = item;
  return true;
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_sleep(scenario_t * const scenario, scenario_player_item_t * const item)
{

  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Play item sleep %d.%d seconds UID %u\n", item->u.sleep.seconds, item->u.sleep.useconds, item->uid);
  if (0 < item->u.sleep.seconds) {
    sleep(item->u.sleep.seconds);
  }
  if (0 < item->u.sleep.useconds) {
    usleep(item->u.sleep.useconds);
  }
  item->is_played = true;
  scenario->last_played_item = item;
  return true;
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_jump_cond(scenario_t * const scenario, scenario_player_item_t * const item)
{
  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Play item jump cond  UID %u\n", item->uid);
  // find the relative item
  scenario_player_item_t * ref = sp_get_var_by_uid(scenario, item->u.cond.var_uid);
  AssertFatal ((ref), "Could not find var item UID %d", item->u.cond.var_uid);

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
    hashtable_rc_t hrc;
    // may be jump forward label (defined lines after the jump condition)
    if (-1 == item->u.cond.jump_label_uid) {
      void                                   *uid = NULL;
      hrc = obj_hashtable_ts_get (scenario->label_items, (const void *)bdata(item->u.cond.jump_label), blength(item->u.cond.jump_label), (void **)&uid);
      AssertFatal ((HASH_TABLE_OK == hrc), "Could not find (supposed) forward label item %s", bdata(item->u.cond.jump_label));
      item->u.cond.jump_label_uid = (int)(uintptr_t)uid;
    }

    // jump
    hrc = hashtable_ts_get (scenario->scenario_items, (hash_key_t)item->u.cond.jump_label_uid, (void **)&ref);
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
  item->is_played = true;
  return true;
}
//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_compute_authentication_response_parameter(scenario_t * const scenario, scenario_player_item_t * const item)
{
  scenario_player_item_t * var_rand = NULL;
  scenario_player_item_t * var_autn = NULL;
  scenario_player_item_t * var_auth_param = NULL;
  scenario_player_item_t * var_selected_plmn = NULL;

  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Play item compute authentication response parameter UID %u\n", item->uid);

  var_autn = sp_get_var(scenario, (unsigned char *)"AUTN");
  AssertFatal ((var_autn), "Could not find var AUTN");

  var_rand = sp_get_var(scenario, (unsigned char *)"RAND");
  AssertFatal ((var_rand), "Could not find var RAND");

  var_auth_param = sp_get_var(scenario, (unsigned char *)"AUTHENTICATION_RESPONSE_PARAMETER");
  AssertFatal ((var_auth_param), "Could not find var AUTHENTICATION_RESPONSE_PARAMETER");

  var_selected_plmn = sp_get_var(scenario, (unsigned char *)"SELECTED_PLMN");
  AssertFatal ((var_selected_plmn), "Could not find var SELECTED_PLMN");
  AssertFatal (3 == blength(var_selected_plmn->u.var.value.value_bstr), "Bad PLMN hex stream");

  usim_data_t * usim_data = msp_get_usim_data(scenario) ;
  memcpy(usim_data->autn, var_autn->u.var.value.value_bstr->data, USIM_AUTN_SIZE);
  memcpy(usim_data->rand, var_rand->u.var.value.value_bstr->data, USIM_RAND_SIZE);
  memset(usim_data->res, 0, USIM_RES_SIZE);
  memcpy(&usim_data->selected_plmn, var_selected_plmn->u.var.value.value_bstr->data, min(blength(var_selected_plmn->u.var.value.value_bstr), 3));

  int rc = usim_authenticate(usim_data,
      usim_data->rand,
      usim_data->autn,
      usim_data->auts,
      usim_data->res,
      usim_data->ck,
      usim_data->ik);

  item->is_played = true;

  if (RETURNok == rc) {

    rc = usim_generate_kasme(usim_data->autn,
        usim_data->ck,
        usim_data->ik,
        &usim_data->selected_plmn,
        usim_data->kasme);

    bdestroy_wrapper(&var_auth_param->u.var.value.value_bstr);
    var_auth_param->u.var.value.value_bstr = blk2bstr(usim_data->res, 8);
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
// return true if we can continue playing the scenario
bool msp_play_compute_authentication_sync_failure_parameter(scenario_t * const scenario, scenario_player_item_t * const item)
{
  scenario_player_item_t * var_rand = NULL;
  scenario_player_item_t * var_autn = NULL;
  scenario_player_item_t * var_auth_param = NULL;
  scenario_player_item_t * var_selected_plmn = NULL;

  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Play item compute sync failure parameter UID %u\n", item->uid);

  var_autn = sp_get_var (scenario, (unsigned char *)"AUTN");
  AssertFatal ((var_autn), "Could not find var AUTN");

  var_rand = sp_get_var (scenario, (unsigned char *)"RAND");
  AssertFatal ((var_rand), "Could not find var RAND");

  var_auth_param = sp_get_var (scenario, (unsigned char *)"AUTHENTICATION_FAILURE_PARAMETER");
  AssertFatal ((var_auth_param), "Could not find var AUTHENTICATION_FAILURE_PARAMETER");

  var_selected_plmn = sp_get_var (scenario, (unsigned char *)"SELECTED_PLMN");
  AssertFatal ((var_selected_plmn), "Could not find var SELECTED_PLMN");
  AssertFatal (3 == blength(var_selected_plmn->u.var.value.value_bstr), "Bad PLMN hex stream");

  usim_data_t * usim_data = msp_get_usim_data(scenario) ;
  memcpy(usim_data->autn, var_autn->u.var.value.value_bstr->data, USIM_AUTN_SIZE);
  memcpy(usim_data->rand, var_rand->u.var.value.value_bstr->data, USIM_RAND_SIZE);
  memset(usim_data->res, 0, USIM_RES_SIZE);
  memcpy(&usim_data->selected_plmn, var_selected_plmn->u.var.value.value_bstr->data, min(blength(var_selected_plmn->u.var.value.value_bstr), 3));

  int rc = usim_authenticate_and_generate_sync_failure(usim_data,
      usim_data->rand,
      usim_data->autn,
      usim_data->auts,
      usim_data->res,
      usim_data->ck,
      usim_data->ik);

  item->is_played = true;
  scenario->last_played_item = item;

  if (RETURNerror == rc) {

    bdestroy_wrapper(&var_auth_param->u.var.value.value_bstr);
    var_auth_param->u.var.value.value_bstr = blk2bstr(usim_data->auts, USIM_SQNMS_SIZE + USIM_XMAC_SIZE);
    OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "msp_play_compute_authentication_sync_failure_parameter succeeded\n");
    return true;
  } else {
    OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "msp_play_compute_authentication_sync_failure_parameter failed\n");
    return false;
  }
}

//------------------------------------------------------------------------------
// return true if we can continue playing the scenario
bool msp_play_update_emm_security_context(scenario_t * const scenario, scenario_player_item_t * const item)
{
  bool                             update = false;
  scenario_player_update_emm_sc_t *update_emm_sc = &item->u.updata_emm_sc;

  emm_security_context_t * ue_emulated_emm_security_context = msp_get_ue_emulated_emm_security_context(scenario);

  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Play item update emm security context UID %u\n", item->uid);
  if (update_emm_sc->is_seea_present) {
    if (update_emm_sc->is_seea_a_uid) {
      scenario_player_item_t * var = sp_get_var_by_uid(scenario, update_emm_sc->seea.uid);
      AssertFatal((var), "Error in getting var uid %d", update_emm_sc->seea.uid);
      AssertFatal(SCENARIO_PLAYER_ITEM_VAR == var->item_type, "Error uid %d ref is not a var: %d", update_emm_sc->seea.uid, var->item_type);
      AssertFatal(VAR_VALUE_TYPE_INT64 == var->u.var.value_type, "Error var %s type %d is not VAR_VALUE_TYPE_INT64", bdata(var->u.var.name), var->u.var.value_type);
      ue_emulated_emm_security_context->selected_algorithms.encryption = (uint8_t)var->u.var.value.value_u64;
    } else {
      ue_emulated_emm_security_context->selected_algorithms.encryption = update_emm_sc->seea.value_u8;
    }
    update = true;
    OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Set UE security context SEEA 0x%x\n", ue_emulated_emm_security_context->selected_algorithms.encryption);
  }

  if (update_emm_sc->is_seia_present) {
    if (update_emm_sc->is_seia_a_uid) {
      scenario_player_item_t * var = sp_get_var_by_uid (scenario, update_emm_sc->seia.uid);
      AssertFatal((var), "Error in getting var uid %d", update_emm_sc->seia.uid);
      AssertFatal(SCENARIO_PLAYER_ITEM_VAR == var->item_type, "Error uid %d ref is not a var: %d", update_emm_sc->seia.uid, var->item_type);
      AssertFatal(VAR_VALUE_TYPE_INT64 == var->u.var.value_type, "Error var %s type %d is not VAR_VALUE_TYPE_INT64", bdata(var->u.var.name), var->u.var.value_type);
      ue_emulated_emm_security_context->selected_algorithms.integrity = (uint8_t)var->u.var.value.value_u64;
    } else {
      ue_emulated_emm_security_context->selected_algorithms.integrity = update_emm_sc->seia.value_u8;
    }
    update = true;
    OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Set UE security context SEIA 0x%x\n", ue_emulated_emm_security_context->selected_algorithms.integrity);
  }

  if (update_emm_sc->is_ul_count_present) {
    uint32_t ul_count = 0;
    if (update_emm_sc->is_ul_count_a_uid) {
      scenario_player_item_t * var = sp_get_var_by_uid (scenario, update_emm_sc->ul_count.uid);
      AssertFatal((var), "Error in getting var uid %d", update_emm_sc->ul_count.uid);
      AssertFatal(SCENARIO_PLAYER_ITEM_VAR == var->item_type, "Error uid %d ref is not a var: %d", update_emm_sc->ul_count.uid, var->item_type);
      AssertFatal(VAR_VALUE_TYPE_INT64 == var->u.var.value_type, "Error var %s type %d is not VAR_VALUE_TYPE_INT64", bdata(var->u.var.name), var->u.var.value_type);
      ul_count = (uint32_t)var->u.var.value.value_u64;

    } else {
      ul_count = update_emm_sc->ul_count.value_u32;
    }
    ue_emulated_emm_security_context->ul_count.seq_num  = (uint8_t)ul_count;
    ue_emulated_emm_security_context->ul_count.overflow = (uint16_t)((ul_count >> 8) & 0xFFFF);
    ue_emulated_emm_security_context->ul_count.spare    = (uint8_t)(ul_count >> 24);
    OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Set UE security context UL count 0x%x\n", ul_count);
  }
  if (update) {
    usim_data_t * usim_data = msp_get_usim_data(scenario) ;
    derive_key_nas (NAS_INT_ALG,
        ue_emulated_emm_security_context->selected_algorithms.integrity,
        usim_data->kasme,
        ue_emulated_emm_security_context->knas_int);

    derive_key_nas (NAS_ENC_ALG,
        ue_emulated_emm_security_context->selected_algorithms.encryption,
        usim_data->kasme,
        ue_emulated_emm_security_context->knas_enc);
  }
  scenario->last_played_item = item;
  return true;
}

//------------------------------------------------------------------------------
void msp_init_scenario(scenario_t * const s)
{
  if (s) {
    s->last_played_item      = NULL;
    int rc = gettimeofday(&s->started, NULL);
    AssertFatal(0 == rc, "gettimeofday()");
    pthread_mutex_init(&s->lock, NULL);
    s->scenario_items = hashtable_ts_create (128, HASH_TABLE_DEFAULT_HASH_FUNC, hash_free_int_func /* items are also stored in a list, so do not free them in the hashtable*/, NULL);
    s->var_items      = obj_hashtable_ts_create (32, HASH_TABLE_DEFAULT_HASH_FUNC, NULL , hash_free_int_func, NULL);
    s->label_items    = obj_hashtable_ts_create (8, HASH_TABLE_DEFAULT_HASH_FUNC, NULL , hash_free_int_func, NULL);
    scenario_set_status(s, SCENARIO_STATUS_NULL, __FILE__, __LINE__);
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

    if (SCENARIO_PLAYER_ITEM_SCENARIO == item->item_type) {
      msp_run_scenario(item->u.scenario);
      //....
      scenario->last_played_item = item;
      return true;
    } else if (SCENARIO_PLAYER_ITEM_ITTI_MSG == item->item_type) {
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
        scenario_set_status(scenario, item->u.exit, __FILE__, __LINE__);
      } else {
        scenario_set_status(scenario, SCENARIO_STATUS_PLAY_SUCCESS, __FILE__, __LINE__);
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
    } else if (SCENARIO_PLAYER_ITEM_COMPUTE_AUTHENTICATION_SYNC_FAILURE_PARAMETER == item->item_type) {
      return msp_play_compute_authentication_sync_failure_parameter(scenario, item);
    } else if (SCENARIO_PLAYER_ITEM_UPDATE_EMM_SECURITY_CONTEXT == item->item_type) {
      return msp_play_update_emm_security_context(scenario, item);
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void msp_run_scenario(scenario_t * const scenario)
{
  if (scenario) {
    pthread_mutex_lock(&scenario->lock);
    msp_clear_processed_flags(scenario);

    g_msp_scenarios.current_scenario = scenario;
    scenario_player_item_t *item = NULL;

    switch (scenario->status) {
      case SCENARIO_STATUS_LOADED:
        item = scenario->head_item;
        scenario_set_status(scenario, SCENARIO_STATUS_PLAYING, __FILE__, __LINE__);
        OAILOG_INFO (LOG_MME_SCENARIO_PLAYER, "Playing scenario %s\n", bdata(scenario->name));
        break;

      case SCENARIO_STATUS_PLAYING:
        item = scenario->last_played_item->next_item;
        OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Continue playing scenario %s\n", bdata(scenario->name));
        break;

      case SCENARIO_STATUS_LOAD_FAILED:
      case SCENARIO_STATUS_PLAY_FAILED:
      case SCENARIO_STATUS_PLAY_SUCCESS:
        pthread_mutex_unlock(&scenario->lock);
        if (!(scenario->num_timers) && (scenario->parent)) {
          OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Continue playing parent scenario %s of scenario %s\n", bdata(scenario->parent->name), bdata(scenario->name));
          msp_scenario_tick(scenario->parent);
        } else if (!scenario->parent) {
          // end of scenario player, no more scenarios to play
          //itti_send_terminate_message (TASK_MME_SCENARIO_PLAYER);
          itti_terminate_tasks (TASK_MME_SCENARIO_PLAYER);
        }
        return;
        break;

      case SCENARIO_STATUS_PAUSED:
        pthread_mutex_unlock(&scenario->lock);
        return;
        break;


      case SCENARIO_STATUS_NULL:
      case SCENARIO_STATUS_LOADING:
      default:
        AssertFatal(0, "Invalid Scenario status %d", scenario->status);
    }

    if (item) {
      pthread_mutex_unlock(&scenario->lock);
      msp_play_item(scenario, item);
      if ((SCENARIO_PLAYER_ITEM_SCENARIO != item->item_type)  && (SCENARIO_STATUS_PAUSED != scenario->status)) {
        msp_scenario_tick(scenario);
      }
    } else {
      if (!(scenario->num_timers) && (SCENARIO_STATUS_PLAYING == scenario->status)) {
        scenario_set_status(scenario, SCENARIO_STATUS_PLAY_SUCCESS, __FILE__, __LINE__);
        OAILOG_NOTICE (LOG_MME_SCENARIO_PLAYER, "Result Run scenario %s SUCCESS\n", bdata(scenario->name));
        pthread_mutex_unlock(&scenario->lock);
        if (scenario->parent) {
          msp_scenario_tick(scenario->parent);
        } else {
          // end of scenario player, no more scenarios to play
          itti_send_terminate_message (TASK_MME_SCENARIO_PLAYER);
        }
      } else { // wait a RX message ?
        pthread_mutex_unlock(&scenario->lock);
      }
    }
  } else {
    OAILOG_ERROR (LOG_MME_SCENARIO_PLAYER, "No scenario to run\n");
  }
}


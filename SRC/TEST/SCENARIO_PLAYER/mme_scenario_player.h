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

#ifndef FILE_MME_SCENARIO_PLAYER_SEEN
#  define FILE_MME_SCENARIO_PLAYER_SEEN

#include "hashtable.h"
#include "obj_hashtable.h"
#include "usim_authenticate.h"
#include "emm_data.h"

typedef enum {
  SCENARIO_PLAYER_ITEM_NULL = 0,
  SCENARIO_PLAYER_ITEM_SCENARIO,
  SCENARIO_PLAYER_ITEM_ITTI_MSG,
  SCENARIO_PLAYER_ITEM_LABEL,
  SCENARIO_PLAYER_ITEM_VAR,
  SCENARIO_PLAYER_ITEM_VAR_SET,
  SCENARIO_PLAYER_ITEM_VAR_INCR,
  SCENARIO_PLAYER_ITEM_VAR_DECR,
  SCENARIO_PLAYER_ITEM_JUMP_COND,
  SCENARIO_PLAYER_ITEM_SLEEP,
  SCENARIO_PLAYER_ITEM_EXIT,
  SCENARIO_PLAYER_ITEM_COMPUTE_AUTHENTICATION_RESPONSE_PARAMETER,
  SCENARIO_PLAYER_ITEM_COMPUTE_AUTHENTICATION_SYNC_FAILURE_PARAMETER,
  SCENARIO_PLAYER_ITEM_UPDATE_EMM_SECURITY_CONTEXT
} scenario_player_item_type_t;

struct scenario_player_item_s;

struct scenario_s;

struct MessageDef_s;

typedef struct scenario_player_timer_arg_s {
  //instance_t              *instance;
  struct scenario_s              *scenario;
  struct scenario_player_item_s  *item;
} scenario_player_timer_arg_t;

typedef struct scenario_player_msg_s {
  // specified
  bool                     is_tx; // is this message is sent by scenario player ?
  int                      itti_sender_task;
  int                      itti_receiver_task;
  int                      time_out_relative_to_msg_uid;
  struct timeval           time_out;
  xmlDocPtr                xml_doc;
  xmlXPathContextPtr       xpath_ctx;

  struct MessageDef_s     *itti_msg;

  // collected
  struct timeval              time_stamp;
  long                        timer_id;
  scenario_player_timer_arg_t timer_arg;
} scenario_player_msg_t;



typedef bstring scenario_player_label_t;


typedef int scenario_player_exit_t;


typedef enum {
  VAR_VALUE_TYPE_NULL = 0,
  VAR_VALUE_TYPE_INT64,
  VAR_VALUE_TYPE_HEX_STREAM,
  VAR_VALUE_TYPE_ASCII_STREAM
} var_value_type_t;


typedef struct scenario_player_var_s {
  bstring             name;
  var_value_type_t    value_type;
  int                 var_ref_uid;
  union {
    uint64_t    value_u64;
    int64_t     value_64;
    bstring     value_bstr;
  } value;
} scenario_player_var_t;


typedef struct scenario_player_set_var_s {
  bstring             name;
  var_value_type_t    value_type;
  int                 var_uid;
  int                 var_ref_uid;
  union {
    uint64_t    value_u64;
    int64_t     value_64;
    bstring     value_bstr;
  } value;
} scenario_player_set_var_t;

typedef enum {
  TEST_EQ = 0,
  TEST_NE,
  TEST_GT,
  TEST_GE,
  TEST_LT,
  TEST_LE
} test_operator_t;

typedef struct scenario_player_cond_s {
  // specified
  int              var_uid;
  uint64_t         var_test_value;
  test_operator_t  test_operator;
  int              jump_label_uid;
  bstring          jump_label;
} scenario_player_cond_t;

typedef struct scenario_player_sleep_s {
  // specified
  int              seconds;
  int              useconds;
} scenario_player_sleep_t;

typedef struct scenario_player_update_emm_sc_s {
  bool is_seea_present;
  bool is_seea_a_uid;
  union {
    uint8_t     value_u8;
    int         uid;
  } seea;

  bool is_seia_present;
  bool is_seia_a_uid;
  union {
    uint8_t     value_u8;
    int         uid;
  } seia;

  bool is_ul_count_present;
  bool is_ul_count_a_uid;
  union {
    uint32_t    value_u32;
    int         uid;
  } ul_count;
} scenario_player_update_emm_sc_t;

struct scenario_s;

typedef struct scenario_player_item_s {
  scenario_player_item_type_t    item_type;
  int                            uid;
  struct scenario_player_item_s *previous_item;
  struct scenario_player_item_s *next_item;
  bool                           is_played;

  union {
    scenario_player_msg_t           msg;
    scenario_player_label_t         label;
    scenario_player_exit_t          exit;
    scenario_player_var_t           var;
    scenario_player_set_var_t       set_var;
    int                             uid_incr_var;
    int                             uid_decr_var;
    scenario_player_cond_t          cond;
    scenario_player_sleep_t         sleep;
    scenario_player_update_emm_sc_t updata_emm_sc;
    struct scenario_s              *scenario;
  } u;
} scenario_player_item_t;

typedef enum {
  SCENARIO_STATUS_NULL = 0,
  SCENARIO_STATUS_LOADING,
  SCENARIO_STATUS_LOADED,
  SCENARIO_STATUS_LOAD_FAILED,
  SCENARIO_STATUS_PLAYING,
  SCENARIO_STATUS_PAUSED,
  SCENARIO_STATUS_PLAY_FAILED,
  SCENARIO_STATUS_PLAY_SUCCESS
} scenario_status_t;

typedef enum {
  SCENARIO_REASON_NONE = 0,
  SCENARIO_XML_FILE_LOAD_SUCCEED,
  SCENARIO_XML_FILE_LOAD_FAILED,
  SCENARIO_XML_ITEM_MSG_FILE_LOAD_FAILED,
  SCENARIO_XML_ITEM_EXIT_LOAD_FAILED,
  SCENARIO_XML_ITEM_LABEL_LOAD_FAILED,
  SCENARIO_XML_ITEM_VAR_LOAD_FAILED,
  SCENARIO_XML_ITEM_SET_VAR_LOAD_FAILED,
  SCENARIO_XML_ITEM_INCR_VAR_LOAD_FAILED,
  SCENARIO_XML_ITEM_DECR_VAR_LOAD_FAILED,
  SCENARIO_XML_ITEM_JUMP_ON_COND_LOAD_FAILED,
  SCENARIO_XML_ITEM_SLEEP_LOAD_FAILED,
  SCENARIO_XML_USIM_LOAD_FAILED,
  SCENARIO_XML_ITEM_COMPUTE_AUTHENTICATION_RESPONSE_PARAMETER_LOAD_FAILED,
  SCENARIO_XML_ITEM_COMPUTE_AUTHENTICATION_SYNC_FAILURE_PARAMETER_LOAD_FAILED,
  SCENARIO_XML_ITEM_UPDATE_EMM_SECURITY_CONTEXT_LOAD_FAILED,
  SCENARIO_XML_FILE_DUMP_SUCCEED,
  SCENARIO_XML_FILE_DUMP_FAILED,
  SCENARIO_XML_COMPARISON_SUCCEED,
  SCENARIO_XML_COMPARISON_FAILED,
  SCENARIO_XML_WRONG_MESSAGE_TYPE,
  SCENARIO_TX_MSG_NO_DELAY,
  SCENARIO_RX_MSG_TIMED_OUT,
  SCENARIO_RX_MSG_PLAYED,
  SCENARIO_WAIT_RX_MSG,
  SCENARIO_WAS_PAUSED,
  SCENARIO_EXIT_ITEM,
  SCENARIO_NO_MORE_ITEM
} scenario_reason_t;


typedef struct scenario_s {
  scenario_status_t       status;
  scenario_player_item_t *head_item;
  scenario_player_item_t *tail_item;
  bstring                 name;
  xmlDocPtr               xml_doc;
  struct scenario_s      *parent;
  struct scenario_s      *next; // only to maintain a list, but nothing to do with the playing sequence
  pthread_mutex_t         lock;

  // scenario play vars
  int                         num_timers;
  struct timeval              started;
  hash_table_ts_t            *scenario_items;
  obj_hash_table_t           *var_items;
  obj_hash_table_t           *label_items;
  scenario_player_item_t     *last_played_item;
  emm_security_context_t     *ue_emulated_emm_security_context;
  usim_data_t                *usim_data;
} scenario_t;

typedef struct scenario_player_s {
  scenario_t         *head_scenarios;
  scenario_t         *tail_scenarios;
  scenario_t         *current_scenario; // no scenarios in // (no multi-threads) for a MME instance
  FILE               *result_fd;
} scenario_player_t;





usim_data_t * msp_get_usim_data(scenario_t * const scenario);
emm_security_context_t * msp_get_ue_emulated_emm_security_context(scenario_t * const scenario);
int msp_load_usim_data (scenario_t * const scenario, xmlDocPtr const xml_doc, xmlXPathContextPtr  xpath_ctx, xmlNodePtr node);
scenario_player_item_t *msp_load_update_emm_security_context (scenario_t * const scenario, xmlDocPtr const xml_doc, xmlXPathContextPtr  xpath_ctx, xmlNodePtr node);
scenario_player_item_t* msp_load_var (scenario_t * const scenario, xmlDocPtr const xml_doc, xmlXPathContextPtr  xpath_ctx, xmlNodePtr node);
scenario_player_item_t* msp_load_set_var (scenario_t * const scenario, xmlDocPtr const xml_doc, xmlXPathContextPtr  xpath_ctx, xmlNodePtr node);
scenario_player_item_t* msp_load_incr_var (scenario_t * const scenario, xmlDocPtr const xml_doc, xmlXPathContextPtr  xpath_ctx, xmlNodePtr node);
scenario_player_item_t* msp_load_decr_var (scenario_t * const scenario, xmlDocPtr const xml_doc, xmlXPathContextPtr  xpath_ctx, xmlNodePtr node);
scenario_player_item_t* msp_load_sleep (scenario_t * const scenario, xmlDocPtr const xml_doc, xmlXPathContextPtr  xpath_ctx, xmlNodePtr node);
scenario_player_item_t* msp_load_compute_authentication_response_parameter (scenario_t * const scenario, xmlDocPtr const xml_doc, xmlXPathContextPtr  xpath_ctx, xmlNodePtr node);
scenario_player_item_t* msp_load_compute_authentication_sync_failure_parameter (scenario_t * const scenario, xmlDocPtr const xml_doc, xmlXPathContextPtr  xpath_ctx, xmlNodePtr node);
scenario_player_item_t* msp_load_label (scenario_t * const scenario, xmlDocPtr const xml_doc, xmlXPathContextPtr  xpath_ctx, xmlNodePtr node);
scenario_player_item_t* msp_load_exit (scenario_t * const scenario, xmlDocPtr const xml_doc, xmlXPathContextPtr  xpath_ctx, xmlNodePtr node);
scenario_player_item_t* msp_load_jcond (scenario_t * const scenario, xmlDocPtr const xml_doc, xmlXPathContextPtr  xpath_ctx, xmlNodePtr node);

void msp_msg_add_var_to_be_loaded(scenario_t * const scenario, scenario_player_msg_t * const msg, scenario_player_item_t * const var_item);
void msp_msg_add_var_listener(scenario_t * const scenario, scenario_player_item_t * const msg_item, scenario_player_item_t * const var_item);
scenario_player_item_t* msp_load_message_file (scenario_t * const scenario, xmlDocPtr const xml_doc, xmlXPathContextPtr  xpath_ctx, xmlNodePtr node, bstring scenario_file_path);
int  msp_load_message (scenario_t * const scenario, bstring file_path, scenario_player_item_t * const item);
// XML content changed (XML vars)
int msp_reload_message (scenario_t * const scenario, scenario_player_item_t * const scenario_player_item);
void msp_free_message_content (scenario_player_msg_t * msg);
void msp_scenario_add_item(scenario_t * const scenario, scenario_player_item_t * const item);
void msp_scenario_player_add_scenario(scenario_player_t * const scenario_player, scenario_t * const scenario);
bool msp_exist_scenario(scenario_player_t * const scenario_player, const char * const name);

scenario_player_item_t *  msp_load_scenario (bstring file_path, scenario_player_t * const scenario_player, scenario_t * parent_scenario);
void msp_free_scenario_player_item (scenario_player_item_t * item);
void msp_free_scenario (scenario_t * scenario);
int  msp_load_scenarios (bstring file_path, scenario_player_t * const scenario_player);
void msp_fsm(void);

struct timer_has_expired_s;

void msp_scenario_tick(scenario_t * const scenario);
bstring scenario_get_scenario_full_path_name(scenario_t * const scenario);
void scenario_set_status(scenario_t * const scenario, const scenario_status_t scenario_status, const scenario_reason_t scenario_reason, char* hint);
void msp_handle_timer_expiry (struct timer_has_expired_s * const timer_has_expired);
void msp_get_elapsed_time_since_scenario_start(scenario_t * const scenario, struct timeval * const elapsed_time);
bool msp_send_tx_message_no_delay(scenario_t * const scenario, scenario_player_item_t * const item);
bool msp_play_tx_message(scenario_t * const scenario, scenario_player_item_t * const item);
bool msp_play_rx_message(scenario_t * const scenario, scenario_player_item_t * const item);
void msp_display_var(scenario_player_item_t * const var);
bool msp_play_var(scenario_t * const scenario, scenario_player_item_t * const item);
bool msp_play_set_var(scenario_t * const scenario, scenario_player_item_t * const item);
bool msp_play_incr_var(scenario_t * const scenario, scenario_player_item_t * const item);
bool msp_play_decr_var(scenario_t * const scenario, scenario_player_item_t * const item);
bool msp_play_sleep(scenario_t * const scenario, scenario_player_item_t * const item);
bool msp_play_jump_cond(scenario_t * const scenario, scenario_player_item_t * const item);
bool msp_play_compute_authentication_response_parameter(scenario_t * const scenario, scenario_player_item_t * const item);
bool msp_play_update_emm_security_context(scenario_t * const scenario, scenario_player_item_t * const item);
bool msp_play_scenario(scenario_t * const parent_scenario, scenario_player_item_t * const item);

bool msp_play_item(scenario_t * const scenario, scenario_player_item_t * const item);
bool msp_schedule_next_message(scenario_t * const s);
void msp_init_scenario(scenario_t * const s);
void msp_run_scenario(scenario_t * const scenario);

#endif



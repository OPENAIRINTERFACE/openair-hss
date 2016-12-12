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
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "log.h"
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
#include "common_defs.h"
#include "common_types.h"
#include "mme_scenario_player.h"
#include "xml_msg_tags.h"
#include "sp_common_xml.h"

//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( qci, QCI );
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( priority_level, PRIORITY_LEVEL );
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( pre_emption_capability, PRE_EMPTION_CAPABILITY );
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( pre_emption_vulnerability, PRE_EMPTION_VULNERABILITY );
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( teid, TEID );

//------------------------------------------------------------------------------
int sp_assign_value_to_var(scenario_t *scenario, char* var_name, char* str_value)
{
  void                                   *uid = NULL;

  OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Assigning to var %s value %s\n", var_name, str_value);
  hashtable_rc_t rc = obj_hashtable_ts_get (scenario->var_items, (const void *)var_name, strlen(var_name), (void **)&uid);
  AssertFatal(HASH_TABLE_OK == rc, "Error in getting var %s in hashtable", var_name);
  int uid_int = (int)(uintptr_t)uid;

  scenario_player_item_t * var = NULL;
  // find the relative item
  hashtable_rc_t hrc = hashtable_ts_get (scenario->scenario_items,
      (hash_key_t)uid_int, (void **)&var);
  AssertFatal ((HASH_TABLE_OK == hrc) && (var), "Could not find var item UID %d", uid_int);
  AssertFatal (SCENARIO_PLAYER_ITEM_VAR == var->item_type, "Bad type var item UID %d", var->item_type);

  if (VAR_VALUE_TYPE_INT64 == var->u.var.value_type) {
    int ret = sscanf((const char*)str_value, "%"SCNx64, &var->u.var.value.value_u64);
    if (1 == ret) {
      OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Set var %s=0x%" PRIx64 "\n", var_name, var->u.var.value.value_u64);
    } else if (str_value[0] == '-') {
      int ret = sscanf((const char*)str_value, "%"SCNi64, &var->u.var.value.value_64);
      OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Set var %s=%" PRIi64 "\n", var_name, var->u.var.value.value_u64);
      if (1 != ret) return RETURNerror;
    } else {
      int ret = sscanf((const char*)str_value, "%"SCNu64, &var->u.var.value.value_u64);
      OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Set var %s=%" PRIu64 "\n", var_name, var->u.var.value.value_u64);
      if (1 != ret) return RETURNerror;
    }
  } else if (VAR_VALUE_TYPE_HEX_STREAM == var->u.var.value_type) {
    if (var->u.var.value.value_bstr) {
      bdestroy_wrapper(&var->u.var.value.value_bstr);
    }
    OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Set hex stream var %s=%s\n", var_name, str_value);
    int len = strlen((const char*)str_value);
    uint8_t hex[len/2];
    int ret = ascii_to_hex ((uint8_t *) hex, (const char *)str_value);
    if (!ret) return RETURNerror;
    var->u.var.value.value_bstr = blk2bstr(hex,len/2);
  } else  if (VAR_VALUE_TYPE_ASCII_STREAM == var->u.var.value_type) {
    if (var->u.var.value.value_bstr) {
      bdestroy_wrapper(&var->u.var.value.value_bstr);
    }
    var->u.var.value.value_bstr = bfromcstr(str_value);
    OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Set ascii stream var %s=%s\n", var_name, str_value);
    if (!var->u.var.value.value_bstr) return RETURNerror;
  } else {
    AssertFatal (0, "Bad var value type %d", var->u.var.value_type);
    return RETURNerror;
  }
  return RETURNok;
}


//------------------------------------------------------------------------------
int sp_compare_string_value_with_var(scenario_t *scenario, char* var_name, char* str_value)
{
  void                                   *uid = NULL;
  hashtable_rc_t rc = obj_hashtable_ts_get (scenario->var_items, (const void *)var_name, strlen(var_name), (void **)&uid);
  AssertFatal(HASH_TABLE_OK == rc, "Error in getting var %s in hashtable", var_name);
  int uid_int = (int)(uintptr_t)uid;

  scenario_player_item_t * var = NULL;
  // find the relative item
  hashtable_rc_t hrc = hashtable_ts_get (scenario->scenario_items,
      (hash_key_t)uid_int, (void **)&var);
  AssertFatal ((HASH_TABLE_OK == hrc) && (var), "Could not find var item UID %d", uid_int);
  AssertFatal (SCENARIO_PLAYER_ITEM_VAR == var->item_type, "Bad type var item UID %d", var->item_type);

  if (VAR_VALUE_TYPE_INT64 == var->u.var.value_type) {
    uint64_t uintvar;
    int ret = sscanf((const char*)str_value, "%"SCNx64, &uintvar);
    // by default a value is dumped in hexa
    if (1 == ret) {
      if (var->u.var.value.value_u64 != uintvar) {
        OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Compare var %s failed %s!=0x%" PRIx64 "\n", var->u.var.name->data, str_value, var->u.var.value.value_u64);
        return RETURNerror;
      }
    } else if (str_value[0] == '-') {
      int64_t intvar;
      int ret = sscanf((const char*)str_value, "%"SCNi64, &intvar);
      if ((var->u.var.value.value_64 != intvar) || (1 != ret)) {
        OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Compare var %s failed %s!=%" PRIi64 "\n", var->u.var.name->data, str_value, var->u.var.value.value_64);
        return RETURNerror;
      }
    } else {
      uint64_t uintvar;
      int ret = sscanf((const char*)str_value, "%"SCNu64, &uintvar);
      if ((var->u.var.value.value_u64 != uintvar) || (1 != ret)) {
        OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Compare var %s failed %s!=0x%" PRIu64 "\n", var->u.var.name->data, str_value, var->u.var.value.value_u64);
        return RETURNerror;
      }
    }
  } else if (VAR_VALUE_TYPE_HEX_STREAM == var->u.var.value_type) {
    int len = strlen((const char*)str_value);
    if ((len/2) == blength(var->u.var.value.value_bstr)) {
      uint8_t hex[len/2];
      int ret = ascii_to_hex ((uint8_t *) hex, (const char *)str_value);
      if (!ret) return RETURNerror;
      if (memcmp(hex, var->u.var.value.value_bstr->data, len/2)) {
        OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Compare var %s failed (binary bstring) %s\n", var_name, str_value);
        return RETURNerror;
      }
    } else {
      OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Compare var %s failed len %d != %d\n",
          var_name, len, blength(var->u.var.value.value_bstr));
      return RETURNerror;
    }
  } else  if (VAR_VALUE_TYPE_ASCII_STREAM == var->u.var.value_type) {
    if ( (blength(var->u.var.value.value_bstr) != strlen(str_value)) ||
        (memcmp(str_value, var->u.var.value.value_bstr->data, blength(var->u.var.value.value_bstr)))) {
      OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Compare var %s failed %s!=%s\n", var_name, str_value, bdata(var->u.var.value.value_bstr));
      return RETURNerror;
    }
  } else {
    AssertFatal (0, "Bad var value type %d", var->u.var.value_type);
    return RETURNerror;
  }
  return RETURNok;
}

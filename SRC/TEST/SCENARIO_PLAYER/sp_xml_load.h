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

#ifndef FILE_SP_XML_LOAD_SEEN
#define FILE_SP_XML_LOAD_SEEN


#define SP_NUM_FROM_XML_PROTOTYPE(name_lower) \
bool sp_ ## name_lower ## _from_xml (\
    scenario_t            * const scenario,\
    scenario_player_msg_t * const msg,\
    name_lower ## _t        * const name_lower)

#define SP_NUM_FROM_XML_GENERATE( name_lower, name_upper ) \
bool sp_ ## name_lower ## _from_xml (\
    scenario_t            * const scenario,\
    scenario_player_msg_t * const msg,\
    name_lower##_t        * const name_lower)\
{\
  bstring xpath_expr = bformat("./%s",name_upper ## _IE_XML_STR);\
  bstring xml_var = NULL;\
  bool res = xml_load_leaf_tag(msg->xml_doc, msg->xpath_ctx, xpath_expr, name_upper ## _XML_SCAN_FMT, (void*)name_lower, &xml_var);\
  if (!res && xml_var) {\
    if ('$' == xml_var->data[0]) {\
      if (BSTR_OK == bdelete(xml_var, 0, 1)) { \
        void           *uid = NULL;\
\
        hashtable_rc_t rc = obj_hashtable_ts_get (scenario->var_items, bdata(xml_var), blength(xml_var), (void**)&uid);\
        if (HASH_TABLE_OK == rc) {\
          scenario_player_item_t * var_item = NULL;\
          rc = hashtable_ts_get (scenario->scenario_items, (const hash_key_t)(uintptr_t)uid, (void **)&var_item);\
          if ((HASH_TABLE_OK == rc) && (SCENARIO_PLAYER_ITEM_VAR == var_item->item_type)) {\
            *name_lower = (name_lower ## _t)var_item->u.var.value.value_u64;\
            AssertFatal (var_item->u.var.value_type == VAR_VALUE_TYPE_INT64, "Bad var type %d", var_item->u.var.value_type);\
            res = true;\
            OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Set %s=" name_upper ## _XML_FMT " from var uid=0x%lx\n",\
                name_upper ## _IE_XML_STR, *name_lower, (uintptr_t)uid);\
          } else {\
            AssertFatal (0, "Could not find %s var uid, should have been declared in scenario\n", name_upper ## _IE_XML_STR);\
          }\
        } else {\
          AssertFatal (0, "Could not find %s var, should have been declared in scenario\n", name_upper ## _IE_XML_STR);\
        }\
      }\
    } else if ('#' == xml_var->data[0]) {\
      if (BSTR_OK == bdelete(xml_var, 0, 1)) { \
        void           *uid = NULL;\
\
        hashtable_rc_t rc = obj_hashtable_ts_get (scenario->var_items, bdata(xml_var), blength(xml_var), (void**)&uid);\
        if (HASH_TABLE_OK == rc) {\
          scenario_player_item_t * var_item = NULL;\
          rc = hashtable_ts_get (scenario->scenario_items, (const hash_key_t)(uintptr_t)uid, (void **)&var_item);\
          if ((HASH_TABLE_OK == rc) && (SCENARIO_PLAYER_ITEM_VAR == var_item->item_type)) {\
            res = true;\
            *name_lower = (name_lower ## _t)var_item->u.var.value.value_u64;\
            AssertFatal (var_item->u.var.value_type == VAR_VALUE_TYPE_INT64, "Bad var type %d", var_item->u.var.value_type);\
            OAILOG_TRACE (LOG_MME_SCENARIO_PLAYER, "Set %s=" name_upper ## _XML_FMT " to be loaded\n", name_upper ## _IE_XML_STR, *name_lower);\
          } else {\
            AssertFatal (0, "Could not find %s var uid, should have been declared in scenario\n", name_upper ## _IE_XML_STR);\
          }\
        } else {\
          AssertFatal (0, "Could not find %s var, should have been declared in scenario\n", name_upper ## _IE_XML_STR);\
        }\
      }\
    }\
    bdestroy_wrapper (&xml_var);\
  }\
  bdestroy_wrapper (&xpath_expr);\
  return res;\
}\

bool sp_u64_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    uint64_t              * const container,
    char                  * const xml_tag_str);

bool sp_xml_load_hex_stream_leaf_tag(
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    bstring                       xpath_expr,
    bstring               * const container);

bool sp_xml_load_ascii_stream_leaf_tag(
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    bstring                       xpath_expr,
    bstring               * const container);
#endif


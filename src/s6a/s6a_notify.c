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
#include <stdint.h>


#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "log.h"
#include "msc.h"
#include "mme_config.h"

#include "assertions.h"
#include "conversions.h"

#include "common_types.h"
#include "common_defs.h"


#include "intertask_interface.h"
#include "s6a_defs.h"
#include "s6a_messages.h"
#include "msc.h"

int
s6a_na_cb (
  struct msg **msg,
  struct avp *paramavp,
  struct session *sess,
  void *opaque,
  enum disp_action *act)
{
  struct msg                             *ans = NULL;
  struct msg                             *qry = NULL;
  struct avp                             *avp = NULL;
  struct avp_hdr                         *hdr = NULL;
  MessageDef                             *message_p = NULL;
  s6a_notify_ans_t                       *s6a_notify_ans_p = NULL;

  DevAssert (msg );
  ans = *msg;
  /*
   * Retrieve the original query associated with the asnwer
   */
  CHECK_FCT (fd_msg_answ_getq (ans, &qry));
  DevAssert (qry );
  message_p = itti_alloc_new_message (TASK_S6A, S6A_NOTIFY_ANS);
  s6a_notify_ans_p = &message_p->ittiMsg.s6a_notify_ans;
  OAILOG_DEBUG (LOG_S6A, "Received S6A Notify Answer (NA)\n");
//  CHECK_FCT (fd_msg_search_avp (qry, s6a_fd_cnf.dataobj_s6a_user_name, &avp));
//
//  if (avp) {
//    CHECK_FCT (fd_msg_avp_hdr (avp, &hdr));
//    snprintf (s6a_auth_info_ans_p->imsi, (int)hdr->avp_value->os.len + 1,
//              "%*s", (int)hdr->avp_value->os.len, hdr->avp_value->os.data);
//  } else {
//    DevMessage ("Query has been freed before we received the answer\n");
//  }

  /*
   * Retrieve the result-code
   */
  CHECK_FCT (fd_msg_search_avp (ans, s6a_fd_cnf.dataobj_s6a_result_code, &avp));

  if (avp) {
    CHECK_FCT (fd_msg_avp_hdr (avp, &hdr));
    s6a_notify_ans_p->result.present = S6A_RESULT_BASE;
    s6a_notify_ans_p->result.choice.base = hdr->avp_value->u32;
    MSC_LOG_TX_MESSAGE (MSC_S6A_MME, MSC_NAS_MME, NULL, 0, "0 S6A_NOTIFY_ANS %s", retcode_2_string (s6a_notif_ans_p->result.choice.base));

    if (hdr->avp_value->u32 != ER_DIAMETER_SUCCESS) {
      OAILOG_ERROR (LOG_S6A, "Got error %u:%s\n", hdr->avp_value->u32, retcode_2_string (hdr->avp_value->u32));
    } else {
      OAILOG_DEBUG (LOG_S6A, "Received S6A Result code %u:%s\n", s6a_notify_ans_p->result.choice.base, retcode_2_string (s6a_notify_ans_p->result.choice.base));
    }
  } else {
    /*
     * The result-code is not present, may be it is an experimental result
     * * * * avp indicating a 3GPP specific failure.
     */
    CHECK_FCT (fd_msg_search_avp (ans, s6a_fd_cnf.dataobj_s6a_experimental_result, &avp));

    if (avp) {
      /*
       * The procedure has failed within the HSS.
       * * * * NOTE: contrary to result-code, the experimental-result is a grouped
       * * * * AVP and requires parsing its childs to get the code back.
       */
      s6a_notify_ans_p->result.present = S6A_RESULT_EXPERIMENTAL;
      s6a_parse_experimental_result (avp, &s6a_notify_ans_p->result.choice.experimental);
      MSC_LOG_TX_MESSAGE (MSC_S6A_MME, MSC_NAS_MME, NULL, 0, "0 S6A_NOTIFY_ANS %s", s6a_notify_ans_p->imsi, experimental_retcode_2_string (s6a_notify_ans_p->result.choice.experimental));
    } else {
      /*
       * Neither result-code nor experimental-result is present ->
       * * * * totally incorrect behaviour here.
       */
      MSC_LOG_TX_MESSAGE_FAILED (MSC_S6A_MME, MSC_NAS_MME, NULL, 0, "0 S6A_NOTIFY_ANS", s6a_notify_ans_p->imsi);
      OAILOG_ERROR (LOG_S6A, "Experimental-Result and Result-Code are absent: " "This is not a correct behaviour\n");
      goto err;
    }
  }

  OAILOG_INFO(LOG_S6A, "Successfully parsed Notify Answer from HSS. \n");
  fd_msg_free(*msg);
  *msg = NULL;

err:
  return RETURNok;
}

int
s6a_generate_notify_req (
  s6a_notify_req_t * nr_p)
{
  struct avp                             *avp;
  struct msg                             *msg;
  struct session                         *sess;
  union avp_value                         value;

  DevAssert (nr_p );
  /*
   * Create the new notify request
   */
  CHECK_FCT (fd_msg_new (s6a_fd_cnf.dataobj_s6a_nr, 0, &msg));
  /*
   * Create a new session
   */
  CHECK_FCT (fd_sess_new (&sess, fd_g_config->cnf_diamid, fd_g_config->cnf_diamid_len, (os0_t) "apps6a", 6));
  {
    os0_t                                   sid;
    size_t                                  sidlen;

    CHECK_FCT (fd_sess_getsid (sess, &sid, &sidlen));
    CHECK_FCT (fd_msg_avp_new (s6a_fd_cnf.dataobj_s6a_session_id, 0, &avp));
    value.os.data = sid;
    value.os.len = sidlen;
    CHECK_FCT (fd_msg_avp_setvalue (avp, &value));
    CHECK_FCT (fd_msg_avp_add (msg, MSG_BRW_FIRST_CHILD, avp));
  }
  CHECK_FCT (fd_msg_avp_new (s6a_fd_cnf.dataobj_s6a_auth_session_state, 0, &avp));
  /*
   * No State maintained
   */
  value.i32 = 1;
  CHECK_FCT (fd_msg_avp_setvalue (avp, &value));
  CHECK_FCT (fd_msg_avp_add (msg, MSG_BRW_LAST_CHILD, avp));
  /*
   * Add Origin_Host & Origin_Realm
   */
  CHECK_FCT (fd_msg_add_origin (msg, 0));
  mme_config_read_lock (&mme_config);
  /*
   * Destination Host
   */
  {
    bstring                                 host = bstrcpy(mme_config.s6a_config.hss_host_name);

    bconchar(host, '.');
    bconcat (host, mme_config.realm);
    CHECK_FCT (fd_msg_avp_new (s6a_fd_cnf.dataobj_s6a_destination_host, 0, &avp));
    value.os.data = (unsigned char *)bdata(host);
    value.os.len = blength(host);
    CHECK_FCT (fd_msg_avp_setvalue (avp, &value));
    CHECK_FCT (fd_msg_avp_add (msg, MSG_BRW_LAST_CHILD, avp));
    bdestroy_wrapper(&host);
  }
  /*
   * Destination_Realm
   */
  {
    CHECK_FCT (fd_msg_avp_new (s6a_fd_cnf.dataobj_s6a_destination_realm, 0, &avp));
    value.os.data = (unsigned char *)bdata(mme_config.realm);
    value.os.len = blength(mme_config.realm);
    CHECK_FCT (fd_msg_avp_setvalue (avp, &value));
    CHECK_FCT (fd_msg_avp_add (msg, MSG_BRW_LAST_CHILD, avp));
  }
  mme_config_unlock (&mme_config);
  /*
   * Adding the User-Name (IMSI)
   */
  CHECK_FCT (fd_msg_avp_new (s6a_fd_cnf.dataobj_s6a_user_name, 0, &avp));
  value.os.data = (unsigned char *)nr_p->imsi;
  value.os.len = strlen (nr_p->imsi);
  CHECK_FCT (fd_msg_avp_setvalue (avp, &value));
  CHECK_FCT (fd_msg_avp_add (msg, MSG_BRW_LAST_CHILD, avp));

  /*
   * Adding the visited plmn id
   */
  {
    uint8_t                                 plmn[3] = { 0x00, 0x00, 0x00 };     //{ 0x02, 0xF8, 0x29 };
    plmn_t                                  plmn_mme;

    plmn_mme = mme_config.gummei.gummei[0].plmn;
    CHECK_FCT (fd_msg_avp_new (s6a_fd_cnf.dataobj_s6a_visited_plmn_id, 0, &avp));
    PLMN_T_TO_TBCD (plmn_mme,
                    plmn,
                    mme_config_find_mnc_length (nr_p->visited_plmn.mcc_digit1, nr_p->visited_plmn.mcc_digit2,
                        nr_p->visited_plmn.mcc_digit3, nr_p->visited_plmn.mnc_digit1, nr_p->visited_plmn.mnc_digit2, nr_p->visited_plmn.mnc_digit3)
    );
    value.os.data = plmn;
    value.os.len = 3;
    CHECK_FCT (fd_msg_avp_setvalue (avp, &value));
    CHECK_FCT (fd_msg_avp_add (msg, MSG_BRW_LAST_CHILD, avp));
    OAILOG_DEBUG (LOG_S6A, "%s NR- new plmn: %02X%02X%02X\n", __FUNCTION__, plmn[0], plmn[1], plmn[2]);
    OAILOG_DEBUG (LOG_S6A, "%s NR- new visited_plmn: %02X%02X%02X\n", __FUNCTION__, value.os.data[0], value.os.data[1], value.os.data[2]);
  }

  /*
   * NOR Flags
   */
  CHECK_FCT (fd_msg_avp_new (s6a_fd_cnf.dataobj_s6a_nr_flags, 0, &avp));
  value.u32 = 0;
  /*
   * Set the nr-flags as indicated by upper layer
   */
  if (nr_p->single_registration_indiction) {
    FLAGS_SET (value.u32, NR_SINGLE_REGISTRATION_IND);
  }

  CHECK_FCT (fd_msg_avp_setvalue (avp, &value));
  CHECK_FCT (fd_msg_avp_add (msg, MSG_BRW_LAST_CHILD, avp));

  CHECK_FCT (fd_msg_send (&msg, NULL, NULL));
  return RETURNok;
}

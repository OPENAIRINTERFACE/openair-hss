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

#include <stdint.h>
#include <stdio.h>

#include "assertions.h"
#include "common_defs.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "log.h"
#include "mme_config.h"
#include "msc.h"
#include "s6a_defs.h"
#include "s6a_messages.h"

#define IMSI_LENGTH 15

int s6a_rr_cb(struct msg **msg, struct avp *paramavp, struct session *sess,
              void *opaque, enum disp_action *act) {
  struct msg *ans, *qry;
  struct avp *avp_p, *origin_host, *origin_realm;
  struct avp *failed_avp = NULL;
  struct avp_hdr *origin_host_hdr, *origin_realm_hdr;
  struct avp_hdr *hdr_p;
  union avp_value value;
  int ret = 0;
  int result_code = ER_DIAMETER_SUCCESS;
  int experimental = 0;
  uint32_t clr_flags = 0;

  MessageDef *message_p = NULL;
  s6a_reset_req_t *s6a_reset_req_p = NULL;

  message_p = itti_alloc_new_message(TASK_S6A, S6A_RESET_REQ);
  s6a_reset_req_p = &message_p->ittiMsg.s6a_reset_req;
  memset(s6a_reset_req_p, 0, sizeof(s6a_reset_req_t));

  if (msg == NULL) {
    return EINVAL;
  }
  OAILOG_NOTICE(LOG_S6A, "Received new s6a reset request\n");
  qry = *msg;

  /*
   * Create the answer immediately.
   */
  CHECK_FCT(fd_msg_new_answer_from_req(fd_g_config->cnf_dict, msg, 0));
  ans = *msg;

  /*
   * Always respond with success.
   */
  result_code = DIAMETER_SUCCESS;
  experimental = 0;

  /*
   * Add the Auth-Session-State AVP
   */
  CHECK_FCT(fd_msg_search_avp(qry, s6a_fd_cnf.dataobj_s6a_auth_session_state,
                              &avp_p));
  CHECK_FCT(fd_msg_avp_hdr(avp_p, &hdr_p));
  CHECK_FCT(
      fd_msg_avp_new(s6a_fd_cnf.dataobj_s6a_auth_session_state, 0, &avp_p));
  CHECK_FCT(fd_msg_avp_setvalue(avp_p, hdr_p->avp_value));
  CHECK_FCT(fd_msg_avp_add(ans, MSG_BRW_LAST_CHILD, avp_p));
  /*
   * Append the result code to the answer
   */
  CHECK_FCT(
      s6a_add_result_code_mme(ans, failed_avp, result_code, experimental));
  CHECK_FCT(fd_msg_send(msg, NULL, NULL));

  /** Just informing the MME_APP. */
  itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_DEBUG(LOG_S6A, "Sending S6A_REQUEST_REQUEST to task MME_APP\n");
  return RETURNok;
}

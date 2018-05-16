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

#include "mme_config.h"
#include "assertions.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "s6a_defs.h"
#include "s6a_messages.h"
#include "msc.h"
#include "log.h"

#define IMSI_LENGTH 15

int
s6a_clr_cb (
  struct msg **msg,
  struct avp *paramavp,
  struct session *sess,
  void *opaque,
  enum disp_action *act)
{
  struct msg                             *ans,
                                         *qry;
  struct avp                             *avp_p,
                                         *origin_host,
                                         *origin_realm;
  struct avp                             *failed_avp = NULL;
  struct avp_hdr                         *origin_host_hdr,
                                         *origin_realm_hdr;
  struct avp_hdr                         *hdr_p;
  union avp_value                         value;
  int                                     ret = 0;
  int                                     result_code = ER_DIAMETER_SUCCESS;
  int                                     experimental = 0;
  uint32_t                                clr_flags = 0;
//  mysql_ul_ans_t                          mysql_ans;
//  mysql_ul_push_t                         mysql_push;

  MessageDef                             *message_p = NULL;
  s6a_cancel_location_req_t              *s6a_cancel_location_req_p = NULL;

  message_p = itti_alloc_new_message (TASK_S6A, S6A_CANCEL_LOCATION_REQ);
  s6a_cancel_location_req_p = &message_p->ittiMsg.s6a_cancel_location_req;
  memset (s6a_cancel_location_req_p, 0, sizeof (s6a_cancel_location_req_t));

  if (msg == NULL) {
    return EINVAL;
  }

//  memset (&mysql_push, 0, sizeof (mysql_ul_push_t));
//  memset (&mysql_ans, 0, sizeof (mysql_ul_ans_t));
  OAILOG_NOTICE(LOG_S6A, "Received new clear location request\n");
  qry = *msg;
//  memcpy((void*)&s6a_cancel_location_req_p->req_msg_p, (void*)qry, sizeof(struct msg));
 s6a_cancel_location_req_p->req_msg_p = (void*)qry;

 OAILOG_NOTICE(LOG_S6A, "CLR : 1 1 1 \n");

 //  /*
//   * Create the answer
//   */
//  CHECK_FCT (fd_msg_new_answer_from_req (fd_g_config->cnf_dict, msg, 0));
//  ans = *msg;
  /*
   * Retrieving IMSI AVP
   */
  CHECK_FCT (fd_msg_search_avp (qry, s6a_fd_cnf.dataobj_s6a_user_name, &avp_p));

  OAILOG_NOTICE(LOG_S6A, "CLR : 2 2 2 \n");

  if (avp_p) {
     CHECK_FCT (fd_msg_avp_hdr (avp_p, &hdr_p));
     if (hdr_p->avp_value->os.len > IMSI_LENGTH) {
       OAILOG_NOTICE (LOG_S6A, "IMSI_LENGTH ER_DIAMETER_INVALID_AVP_VALUE\n");
       result_code = ER_DIAMETER_INVALID_AVP_VALUE;
       goto err;
     }else{
       memcpy (s6a_cancel_location_req_p->imsi, hdr_p->avp_value->os.data, hdr_p->avp_value->os.len);
       s6a_cancel_location_req_p->imsi[hdr_p->avp_value->os.len] = '\0';
       s6a_cancel_location_req_p->imsi_length = hdr_p->avp_value->os.len;
       OAILOG_DEBUG (LOG_S6A, "Received s6a ula for imsi=%*s\n", (int)hdr_p->avp_value->os.len, hdr_p->avp_value->os.data);
     }
   } else {
     OAILOG_ERROR (LOG_S6A, "Cannot get IMSI AVP which is mandatory\n");
     result_code = ER_DIAMETER_MISSING_AVP;
     goto err;
   }

  /*
   * Retrieving Cancellation Type AVP
   */
  CHECK_FCT (fd_msg_search_avp (qry, s6a_fd_cnf.dataobj_s6a_cancellation_type, &avp_p));

  if (avp_p) {
    CHECK_FCT (fd_msg_avp_hdr (avp_p, &hdr_p));
    s6a_cancel_location_req_p->cancellation_type = hdr_p->avp_value->u32;

    /*
     * As we are in E-UTRAN stand-alone HSS, we have to reject incoming
     * * * * location request with a RAT-Type != than E-UTRAN.
     * * * * The user may be disallowed to use the specified RAT, check the access
     * * * * restriction bit mask received from DB.
     */
//    if ((hdr->avp_value->u32 != 1004) || (FLAG_IS_SET (mysql_ans.access_restriction, E_UTRAN_NOT_ALLOWED))) {
//      experimental = 1;
//      FPRINTF_ERROR ( "access_restriction DIAMETER_ERROR_RAT_NOT_ALLOWED\n");
//      result_code = DIAMETER_ERROR_RAT_NOT_ALLOWED;
//      goto out;
//    }
  } else {
    OAILOG_ERROR (LOG_S6A, "rat_type ER_DIAMETER_MISSING_AVP\n");
    result_code = ER_DIAMETER_MISSING_AVP;
    goto err;
  }

  /*
   * Retrieving Origin host AVP
   */
//  CHECK_FCT (fd_msg_search_avp (qry, s6a_fd_cnf.dataobj_s6a_origin_host, &origin_host));
//
//  if (!origin_host) {
//    OAILOG_ERROR (LOG_S6A, "origin_host ER_DIAMETER_MISSING_AVP\n");
//    result_code = ER_DIAMETER_MISSING_AVP;
//    goto err;
//  }

//  /*
//   * Retrieving Origin realm AVP
//   */
//  CHECK_FCT (fd_msg_search_avp (qry, s6a_fd_cnf.dataobj_s6a_origin_realm, &origin_realm));
//
//  if (!origin_realm) {
//    FPRINTF_ERROR ( "origin_realm ER_DIAMETER_MISSING_AVP\n");
//    result_code = ER_DIAMETER_MISSING_AVP;
//    goto err;
//  }

  /*
   * Retrieve the header from origin host and realm avps
   */
//  CHECK_FCT (fd_msg_avp_hdr (origin_host, &origin_host_hdr));
//  CHECK_FCT (fd_msg_avp_hdr (origin_realm, &origin_realm_hdr));
//  sprintf (mysql_push.mme_identity.mme_host, "%*s", (int)origin_host_hdr->avp_value->os.len, (char *)origin_host_hdr->avp_value->os.data);
//  sprintf (mysql_push.mme_identity.mme_realm, "%*s", (int)origin_realm_hdr->avp_value->os.len, (char *)origin_realm_hdr->avp_value->os.data);

  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
  OAILOG_DEBUG (LOG_S6A, "Sending S6A_CANCEL_LOCATION_REQUEST to task MME_APP\n");
  return RETURNok;
  /*
   * Todo: Retrieving CLR Flags AVP
   */
//  CHECK_FCT (fd_msg_search_avp (qry, s6a_cnf.dataobj_s6a_clr_flags, &avp));



//
//out:
//  /*
//   * Add the Auth-Session-State AVP
//   */
//  CHECK_FCT (fd_msg_search_avp (qry, s6a_cnf.dataobj_s6a_auth_session_state, &avp));
//  CHECK_FCT (fd_msg_avp_hdr (avp, &hdr));
//  CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_auth_session_state, 0, &avp));
//  CHECK_FCT (fd_msg_avp_setvalue (avp, hdr->avp_value));
//  CHECK_FCT (fd_msg_avp_add (ans, MSG_BRW_LAST_CHILD, avp));
//  /*
//   * Append the result code to the answer
//   */
//  CHECK_FCT (s6a_add_result_code (ans, failed_avp, result_code, experimental));
//  CHECK_FCT (fd_msg_send (msg, NULL, NULL));
//  return 0;
//}

  err:
//    req_p = NULL;
    itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
    OAILOG_DEBUG (LOG_S6A, "Sending S6A_CANCEL_LOCATION_REQUEST to task MME_APP\n");
    return RETURNok;
}

int
s6a_generate_cancel_location_ans (
    s6a_cancel_location_ans_t * cla_pP)
{
  struct msg                             *ans = NULL,
                                         *qry = NULL;
  struct avp                             *avp = NULL,
                                         *failed_avp = NULL;
  struct avp_hdr                         *hdr = NULL;
  union avp_value                         value;

  int                                     ret = 0;
  int                                     result_code = ER_DIAMETER_SUCCESS;
  int                                     experimental = 0;

  if (cla_pP->req== NULL) {
    return EINVAL;
  }

  qry = cla_pP->req;
  /*
   * Create the answer
   */
  CHECK_FCT (fd_msg_new_answer_from_req (fd_g_config->cnf_dict, &cla_pP->req, 0));
  ans = cla_pP->req;

  /*
   * Database query failed...
   */
  result_code = DIAMETER_SUCCESS;
  experimental = 0;

  /*
   * Add the Auth-Session-State AVP
   */
  CHECK_FCT (fd_msg_search_avp (qry, s6a_fd_cnf.dataobj_s6a_auth_session_state, &avp));
  CHECK_FCT (fd_msg_avp_hdr (avp, &hdr));
  CHECK_FCT (fd_msg_avp_new (s6a_fd_cnf.dataobj_s6a_auth_session_state, 0, &avp));
  CHECK_FCT (fd_msg_avp_setvalue (avp, hdr->avp_value));
  CHECK_FCT (fd_msg_avp_add (ans, MSG_BRW_LAST_CHILD, avp));
  /*
   * Append the result code to the answer
   */
  CHECK_FCT (s6a_add_result_code_mme (ans, failed_avp, result_code, experimental));
  CHECK_FCT (fd_msg_send (&cla_pP->req, NULL, NULL));
  return ret;
}




//#define IMSI_LENGTH 15
//
//int
//s6a_clr_cb (
//  struct msg **msg,
//  struct avp *paramavp,
//  struct session *sess,
//  void *opaque,
//  enum disp_action *act)
//{
//  struct msg                             *ans,
//                                         *qry;
//  struct avp                             *avp_p,
//                                         *origin_host,
//                                         *origin_realm;
//  struct avp                             *failed_avp = NULL;
//  struct avp_hdr                         *origin_host_hdr,
//                                         *origin_realm_hdr;
//  struct avp_hdr                         *hdr_p;
//  union avp_value                         value;
//  int                                     ret = 0;
//  int                                     result_code = ER_DIAMETER_SUCCESS;
//  int                                     experimental = 0;
//  uint32_t                                clr_flags = 0;
//
//  if (msg == NULL) {
//    return EINVAL;
//  }
//  qry = *msg;
//  OAILOG_NOTICE(LOG_S6A, "Received new s6a cancel location request\n");
//
//  /*
//   * Create the answer immediately.
//   */
//  CHECK_FCT (fd_msg_new_answer_from_req (fd_g_config->cnf_dict, msg, 0));
//  ans = *msg;
//
//  /** Now deal with this in the MME_APP. */
//  MessageDef                             *message_p = NULL;
//  s6a_cancel_location_req_t              *s6a_cancel_location_req_p = NULL;
//
//  message_p = itti_alloc_new_message (TASK_S6A, S6A_CANCEL_LOCATION_REQ);
//  s6a_cancel_location_req_p = &message_p->ittiMsg.s6a_cancel_location_req;
//  memset (s6a_cancel_location_req_p, 0, sizeof (s6a_cancel_location_req_t));
//  /*
//   * Retrieving IMSI AVP
//   */
//  CHECK_FCT (fd_msg_search_avp (qry, s6a_fd_cnf.dataobj_s6a_user_name, &avp_p));
//
//  if (avp_p) {
//    CHECK_FCT (fd_msg_avp_hdr (avp_p, &hdr_p));
//    if (hdr_p->avp_value->os.len > IMSI_LENGTH) {
//      OAILOG_NOTICE (LOG_S6A, "IMSI_LENGTH ER_DIAMETER_INVALID_AVP_VALUE\n");
//      result_code = ER_DIAMETER_INVALID_AVP_VALUE;
//      goto err;
//    }else{
//      memcpy (s6a_cancel_location_req_p->imsi, hdr_p->avp_value->os.data, hdr_p->avp_value->os.len);
//      s6a_cancel_location_req_p->imsi[hdr_p->avp_value->os.len] = '\0';
//      s6a_cancel_location_req_p->imsi_length = hdr_p->avp_value->os.len;
//      OAILOG_DEBUG (LOG_S6A, "Received s6a ula for imsi=%*s\n", (int)hdr_p->avp_value->os.len, hdr_p->avp_value->os.data);
//    }
//  } else {
//    OAILOG_ERROR (LOG_S6A, "Cannot get IMSI AVP which is mandatory\n");
//    result_code = ER_DIAMETER_MISSING_AVP;
//    goto err;
//  }
//
//  /*
//   * Retrieving Cancellation Type AVP
//   */
//  CHECK_FCT (fd_msg_search_avp (qry, s6a_fd_cnf.dataobj_s6a_cancellation_type, &avp_p));
//
//  if (avp_p) {
//    CHECK_FCT (fd_msg_avp_hdr (avp_p, &hdr_p));
//    s6a_cancel_location_req_p->cancellation_type = hdr_p->avp_value->u32;
//
//    /*
//     * As we are in E-UTRAN stand-alone HSS, we have to reject incoming
//     * * * * location request with a RAT-Type != than E-UTRAN.
//     * * * * The user may be disallowed to use the specified RAT, check the access
//     * * * * restriction bit mask received from DB.
//     */
////    if ((hdr->avp_value->u32 != 1004) || (FLAG_IS_SET (mysql_ans.access_restriction, E_UTRAN_NOT_ALLOWED))) {
////      experimental = 1;
////      FPRINTF_ERROR ( "access_restriction DIAMETER_ERROR_RAT_NOT_ALLOWED\n");
////      result_code = DIAMETER_ERROR_RAT_NOT_ALLOWED;
////      goto out;
////    }
//  } else {
//    OAILOG_ERROR (LOG_S6A, "rat_type ER_DIAMETER_MISSING_AVP\n");
//    result_code = ER_DIAMETER_MISSING_AVP;
//    goto err;
//  }
//
//  /*
//   * Retrieving CLR Flags AVP.
//   */
//  CHECK_FCT (fd_msg_search_avp (qry, s6a_fd_cnf.dataobj_s6a_clr_flags, &avp_p));
//  if (avp_p) {
//    CHECK_FCT (fd_msg_avp_hdr (avp_p, &hdr_p));
//    s6a_cancel_location_req_p->clr_flags = hdr_p->avp_value->u32;
//  }
//
//
//   /*
//    * Always respond with success.
//    */
//   result_code = DIAMETER_SUCCESS;
//   experimental = 0;
//
//   /*
//    * Add the Auth-Session-State AVP
//    */
//   CHECK_FCT (fd_msg_search_avp (qry, s6a_fd_cnf.dataobj_s6a_auth_session_state, &avp_p));
//   CHECK_FCT (fd_msg_avp_hdr (avp_p, &hdr_p));
//   CHECK_FCT (fd_msg_avp_new (s6a_fd_cnf.dataobj_s6a_auth_session_state, 0, &avp_p));
//   CHECK_FCT (fd_msg_avp_setvalue (avp_p, hdr_p->avp_value));
//   CHECK_FCT (fd_msg_avp_add (ans, MSG_BRW_LAST_CHILD, avp_p));
//   /*
//    * Append the result code to the answer
//    */
//   CHECK_FCT (s6a_add_result_code_mme (ans, failed_avp, result_code, experimental));
//   CHECK_FCT (fd_msg_send (msg, NULL, NULL));
//
//
//  err:
//  itti_send_msg_to_task (TASK_MME_APP, INSTANCE_DEFAULT, message_p);
//  OAILOG_DEBUG (LOG_S6A, "Sending S6A_CANCEL_LOCATION_REQUEST to task MME_APP - error case\n");
//  return RETURNok;
//}
//

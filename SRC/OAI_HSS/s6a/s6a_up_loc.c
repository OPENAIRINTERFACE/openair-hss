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


/*! \file s6a_up_loc.c
   \brief Handle an update location message and create the answer.
   \author Sebastien ROUX <sebastien.roux@eurecom.fr>
   \date 2013
   \version 0.1
*/

#include <freeDiameter/freeDiameter-host.h>
#include <freeDiameter/libfdproto.h>
#include "hss_config.h"
#include "db_proto.h"
#include "s6a_proto.h"
#include "access_restriction.h"
#include "log.h"


int
s6a_generate_cancel_location_req(char *previous_mme_host, char *previous_mme_realm, char *imsi)
{
  struct avp                             *avp_p = NULL;
  struct msg                             *msg_p = NULL;
  struct session                         *sess_p = NULL;
  union avp_value                         value;


  printf("Inside s6a_generate_cancel_location_req \n"); 
  printf("previous_mme_host = %s previous_mme_realm = %s \n",previous_mme_host, previous_mme_realm);
  //DevAssert (clr_pP );
  /*
   * Create the new clear location request message
   */
  CHECK_FCT (fd_msg_new (s6a_cnf.dataobj_s6a_cancel_loc_req, 0, &msg_p));
  /*
   * Create a new session
   */
  CHECK_FCT (fd_sess_new (&sess_p, fd_g_config->cnf_diamid, fd_g_config->cnf_diamid_len, (os0_t) "apps6a", 6));
  {
    os0_t                                   sid;
    size_t                                  sidlen;

    CHECK_FCT (fd_sess_getsid (sess_p, &sid, &sidlen));
    CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_session_id, 0, &avp_p));
     //printf("sid = %" PRId8 "\n", *sid);
     //printf("sidlen = %zu \n",sidlen);
    value.os.data = sid;
    value.os.len = sidlen;
  printf("At the line %d \n",__LINE__);
    CHECK_FCT (fd_msg_avp_setvalue (avp_p, &value));
  printf("At the line %d \n",__LINE__);
    CHECK_FCT (fd_msg_avp_add (msg_p, MSG_BRW_FIRST_CHILD, avp_p));
  }
  CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_auth_session_state, 0, &avp_p));
  /*
   * No State maintained
   */
  value.i32 = 1;
  CHECK_FCT (fd_msg_avp_setvalue (avp_p, &value));
  CHECK_FCT (fd_msg_avp_add (msg_p, MSG_BRW_LAST_CHILD, avp_p));
  /*
   * Add Origin_Host & Origin_Realm
   */
  CHECK_FCT (fd_msg_add_origin (msg_p, 0));
  //mme_config_read_lock (&mme_config);
  /*
   * Destination Host
   */
  {
    //bstring host = bstrcpy(mme_config.s6a_config.hss_host_name);

    //bconchar(host, '.');
    //bconcat (host, mme_config.realm);

    CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_destination_host, 0, &avp_p));
    value.os.data = (unsigned char *)(previous_mme_host);
    value.os.len = strlen(previous_mme_host);
    CHECK_FCT (fd_msg_avp_setvalue (avp_p, &value));
    CHECK_FCT (fd_msg_avp_add (msg_p, MSG_BRW_LAST_CHILD, avp_p));
    //bdestroy_wrapper (&host);
  }
  /*
   * Destination_Realm
   */
  {
    CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_destination_realm, 0, &avp_p));
    value.os.data = (unsigned char *)(previous_mme_realm);
    value.os.len = strlen(previous_mme_realm);
    CHECK_FCT (fd_msg_avp_setvalue (avp_p, &value));
    CHECK_FCT (fd_msg_avp_add (msg_p, MSG_BRW_LAST_CHILD, avp_p));
  }
  //mme_config_unlock (&mme_config);
  /*
   * Adding the User-Name (IMSI)
   */
  CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_user_name, 0, &avp_p));
  value.os.data = (unsigned char *)imsi;
  value.os.len = strlen(imsi);
  CHECK_FCT (fd_msg_avp_setvalue (avp_p, &value));
  CHECK_FCT (fd_msg_avp_add (msg_p, MSG_BRW_LAST_CHILD, avp_p));


  CHECK_FCT(fd_msg_avp_new(s6a_cnf.dataobj_s6a_cancellation_type, 0, &avp_p));
  value.i32 = 4; // value for Cancellation-type INITIAL_ATTACH_PROCEDURE
  CHECK_FCT (fd_msg_avp_setvalue (avp_p, &value));
  CHECK_FCT (fd_msg_avp_add (msg_p, MSG_BRW_LAST_CHILD, avp_p));
  

  CHECK_FCT (fd_msg_send (&msg_p, NULL, NULL));
  FPRINTF_DEBUG ("Sending s6a clr for imsi=%s\n", imsi);
  return 0;
}


int 
s6a_clear_info_cb (
  struct msg **msg,
  struct avp *paramavp,
  struct session *sess,
  void *opaque,
  enum disp_action *act)
{
       FPRINTF_DEBUG("Received the Clear Location Answer from MME\n");


}


int
s6a_up_loc_cb (
  struct msg **msg,
  struct avp *paramavp,
  struct session *sess,
  void *opaque,
  enum disp_action *act)
{
  struct msg                             *ans,
                                         *qry;
  struct avp                             *avp,
                                         *origin_host,
                                         *origin_realm;
  struct avp                             *failed_avp = NULL;
  struct avp_hdr                         *origin_host_hdr,
                                         *origin_realm_hdr;
  struct avp_hdr                         *hdr;
  union avp_value                         value;
  int                                     ret = 0;
  int                                     result_code = ER_DIAMETER_SUCCESS;
  int                                     experimental = 0;
  uint32_t                                ulr_flags = 0;
  cassandra_ul_ans_t                      cass_ans;
  cassandra_ul_push_t                     cass_push;

  if (msg == NULL) {
    return EINVAL;
  }


  dump(*msg);
  memset (&cass_push, 0, sizeof (cassandra_ul_push_t));
  memset (&cass_ans, 0, sizeof (cassandra_ul_ans_t));
  FPRINTF_NOTICE ( "Received new update location request\n");
  qry = *msg;
  /*
   * Create the answer
   */
  CHECK_FCT (fd_msg_new_answer_from_req (fd_g_config->cnf_dict, msg, 0));
  ans = *msg;
  /*
   * Retrieving IMSI AVP
   */
  CHECK_FCT (fd_msg_search_avp (qry, s6a_cnf.dataobj_s6a_imsi, &avp));

  if (avp) {
    CHECK_FCT (fd_msg_avp_hdr (avp, &hdr));

    if (hdr->avp_value->os.len > IMSI_LENGTH) {
      FPRINTF_NOTICE ( "IMSI_LENGTH ER_DIAMETER_INVALID_AVP_VALUE\n");
      result_code = ER_DIAMETER_INVALID_AVP_VALUE;
      goto out;
    }
    // 3GPP TS 29.272-910 / 5.2.1.1.3 Detailed behaviour of the HSS
    // When receiving an Update Location request the HSS shall check whether the IMSI is known.
    // If it is not known, a Result Code of DIAMETER_ERROR_USER_UNKNOWN shall be returned.
    // If it is known, but the subscriber has no EPS subscription, the HSS may (as an operator option)
    //     return a Result Code of DIAMETER_ERROR_UNKNOWN_EPS_SUBSCRIPTION.
    // If the Update Location Request is received over the S6a interface, and the subscriber has not
    //     any APN configuration, the HSS shall return a Result Code of DIAMETER_ERROR_UNKNOWN_EPS_SUBSCRIPTION.
    // The HSS shall check whether the RAT type the UE is using  is allowed. If it is not,
    //     a Result Code of DIAMETER_ERROR_RAT_NOT_ALLOWED shall be returned.
    // ...
    sprintf (cass_push.imsi, "%*s", (int)hdr->avp_value->os.len, (char *)hdr->avp_value->os.data);

    if ((ret = hss_cassandra_update_loc (cass_push.imsi, &cass_ans)) != 0) {
      /*
       * We failed to find the IMSI in the database. Replying to the request
       * * * * with the user unknown cause.
       */
      experimental = 1;
      FPRINTF_NOTICE ( "IMSI %s DIAMETER_ERROR_USER_UNKNOWN\n", cass_push.imsi);
      result_code = DIAMETER_ERROR_USER_UNKNOWN;
      goto out;
    }
  } else {
    FPRINTF_ERROR ( "Cannot get IMSI AVP which is mandatory\n");
    result_code = ER_DIAMETER_MISSING_AVP;
    goto out;
  }

  /*
   * Retrieving Origin host AVP
   */
  CHECK_FCT (fd_msg_search_avp (qry, s6a_cnf.dataobj_s6a_origin_host, &origin_host));

  if (!origin_host) {
    FPRINTF_ERROR ( "origin_host ER_DIAMETER_MISSING_AVP\n");
    result_code = ER_DIAMETER_MISSING_AVP;
    goto out;
  }

  /*
   * Retrieving Origin realm AVP
   */
  CHECK_FCT (fd_msg_search_avp (qry, s6a_cnf.dataobj_s6a_origin_realm, &origin_realm));

  if (!origin_realm) {
    FPRINTF_ERROR ( "origin_realm ER_DIAMETER_MISSING_AVP\n");
    result_code = ER_DIAMETER_MISSING_AVP;
    goto out;
  }

  /*
   * Retrieve the header from origin host and realm avps
   */
  CHECK_FCT (fd_msg_avp_hdr (origin_host, &origin_host_hdr));
  CHECK_FCT (fd_msg_avp_hdr (origin_realm, &origin_realm_hdr));
  sprintf (cass_push.mme_identity.mme_host, "%*s", (int)origin_host_hdr->avp_value->os.len, (char *)origin_host_hdr->avp_value->os.data);
  sprintf (cass_push.mme_identity.mme_realm, "%*s", (int)origin_realm_hdr->avp_value->os.len, (char *)origin_realm_hdr->avp_value->os.data);
  /*
   * Retrieving RAT type AVP
   */
  CHECK_FCT (fd_msg_search_avp (qry, s6a_cnf.dataobj_s6a_rat_type, &avp));

  if (avp) {
    CHECK_FCT (fd_msg_avp_hdr (avp, &hdr));

    /*
     * As we are in E-UTRAN stand-alone HSS, we have to reject incoming
     * * * * location request with a RAT-Type != than E-UTRAN.
     * * * * The user may be disallowed to use the specified RAT, check the access
     * * * * restriction bit mask received from DB.
     */
     printf("rat type avp value = %d\n",hdr->avp_value->u32);
     printf("cass_ans.access_restriction = %d \n",cass_ans.access_restriction);
    if ((hdr->avp_value->u32 != 1004) || (FLAG_IS_SET (cass_ans.access_restriction, E_UTRAN_NOT_ALLOWED))) {
      experimental = 1;
      FPRINTF_ERROR ( "access_restriction DIAMETER_ERROR_RAT_NOT_ALLOWED\n");
      result_code = DIAMETER_ERROR_RAT_NOT_ALLOWED;
      goto out;
    }
  } else {
    FPRINTF_ERROR ( "rat_type ER_DIAMETER_MISSING_AVP\n");
    result_code = ER_DIAMETER_MISSING_AVP;
    goto out;
  }

  /*
   * Retrieving ULR Flags AVP
   */
  CHECK_FCT (fd_msg_search_avp (qry, s6a_cnf.dataobj_s6a_ulr_flags, &avp));

  if (avp) {
    CHECK_FCT (fd_msg_avp_hdr (avp, &hdr));
    ulr_flags = hdr->avp_value->u32;

    /*
     * Check the flags received
     */
    if (FLAG_IS_SET (ulr_flags, ULR_SINGLE_REGISTRATION_IND)) {
      /*
       * We don't handle cases where we have to inform SGSN
       */
      result_code = ER_DIAMETER_INVALID_AVP_VALUE;
      FPRINTF_ERROR ( "ULR single registration bit set (SGSN to MME): " "not handled by standalone E-UTRAN HSS\n");
      goto out;
    }

    if (FLAG_IS_SET (ulr_flags, ULR_S6A_S6D_INDICATOR)) {
      /*
       * The request is coming from s6d interface (SGSN).
       */
      result_code = ER_DIAMETER_INVALID_AVP_VALUE;
      FPRINTF_ERROR ( "ULR S6D bit set: " "not handled by standalone E-UTRAN HSS\n");
      goto out;
    }

    if (FLAG_IS_SET (ulr_flags, ULR_NODE_TYPE_IND)) {
      /*
       * Request coming from combined SGSN/MME.
       */
      result_code = ER_DIAMETER_INVALID_AVP_VALUE;
      FPRINTF_ERROR ( "ULR conbined SGSN/MME bit set: " "not handled by standalone E-UTRAN HSS\n");
      goto out;
    }

    if (FLAG_IS_SET (ulr_flags, ULR_INITIAL_ATTACH_IND)) {
      /*
       * This bit, when set, indicates that the HSS shall send Cancel
       * * * * Location to the MME or SGSN if there is the MME or SGSN
       * * * * registration.
       */
      // TODO: check if an MME is already registered, serving the UE.
      // If so, it should be informed.
 
     //send the CLR command to MME 
     FPRINTF_ERROR ("ULR flag is ULR_INITIAL_ATTACH_IND\n");
    
      if((cass_ans.mme_identity.mme_host != NULL) && (cass_ans.mme_identity.mme_realm != NULL)){

	 printf("cass_ans.mme_identity.mme_host = %s  cass_ans.mme_identity.mme_real = %s \n",cass_ans.mme_identity.mme_host, cass_ans.mme_identity.mme_realm);
          /*if((s6a_generate_cancel_location_req(cass_ans.mme_identity.mme_host,cass_ans.mme_identity.mme_realm,cass_ans.imsi) != 0))
          {
                FPRINTF_ERROR("ERROR in sending the CLR to previous MME");
                goto out;
          }*/

      }

      /*
       * The identity of the MME will be added to db
       */
      cass_push.mme_identity_present = MME_IDENTITY_PRESENT;
    } else {
      /*
       * The bit is not set, we are expecting that the mme contained in db
       * * * * matches the original MME.
       */
      if ((cass_ans.mme_identity.mme_host != NULL) && (cass_ans.mme_identity.mme_realm != NULL)) {
        /*
         * Compare if values match expected
         */
        if (memcmp (cass_ans.mme_identity.mme_host, origin_host_hdr->avp_value->os.data,
                    origin_host_hdr->avp_value->os.len > strlen (cass_ans.mme_identity.mme_host) ? strlen (cass_ans.mme_identity.mme_host) : origin_host_hdr->avp_value->os.len) != 0) {
          experimental = 1;
          FPRINTF_ERROR ( "DIAMETER_ERROR_UNKOWN_SERVING_NODE (host)\n");
          result_code = DIAMETER_ERROR_UNKOWN_SERVING_NODE;
          goto out;
        }

        if (memcmp (cass_ans.mme_identity.mme_realm, origin_realm_hdr->avp_value->os.data,
                    origin_realm_hdr->avp_value->os.len > strlen (cass_ans.mme_identity.mme_realm) ? strlen (cass_ans.mme_identity.mme_realm) : origin_realm_hdr->avp_value->os.len) != 0) {
          experimental = 1;
          FPRINTF_ERROR ( "DIAMETER_ERROR_UNKOWN_SERVING_NODE (realm)\n");
          result_code = DIAMETER_ERROR_UNKOWN_SERVING_NODE;
          goto out;
        }
      } else {
        /*
         * Failed to retrieve current serving MME and the ULR is not
         * * * * marked as an initial attach indication...
         */
        experimental = 1;
        FPRINTF_ERROR ( "DIAMETER_ERROR_UNKOWN_SERVING_NODE\n");
        result_code = DIAMETER_ERROR_UNKOWN_SERVING_NODE;
        goto out;
      }
    }

    if (!ULR_PAD_VALID (ulr_flags)) {
      /*
       * Padding is not zero'ed, may be the MME/SGSN supports newer
       * * * * release. Inform it.
       */
      FPRINTF_ERROR ( "ULR flags ER_DIAMETER_INVALID_AVP_VALUE\n");
      result_code = ER_DIAMETER_INVALID_AVP_VALUE;
      goto out;
    }
  } else {
    FPRINTF_ERROR ( "ULR flags ER_DIAMETER_MISSING_AVP\n");
    result_code = ER_DIAMETER_MISSING_AVP;
    goto out;
  }

  /*
   * Retrieving Visited-PLMN-Id AVP
   */
  CHECK_FCT (fd_msg_search_avp (qry, s6a_cnf.dataobj_s6a_visited_plmn_id, &avp));

  if (avp) {
    CHECK_FCT (fd_msg_avp_hdr (avp, &hdr));

    /*
     * Roaming cases are not allowed for now.
     * * * * Reject the connectivity if PLMN visited and IMSI PLMN disagree.
     */
    /*
     * TODO
     */
    if (hdr->avp_value->os.len == 3) {
      //             if (apply_access_restriction(mysql_push.imsi, hdr->avp_value->os.data) != 0) {
      //                 /* We found that user is roaming and has no right to do it ->
      //                 * reject the connection
      //                 */
      //                 result_code = DIAMETER_ERROR_ROAMING_NOT_ALLOWED;
      //                 experimental = 1;
      //                 goto out;
      //             }
    } else {
      FPRINTF_ERROR ( "PLMN ID ER_DIAMETER_INVALID_AVP_VALUE\n");
      result_code = ER_DIAMETER_INVALID_AVP_VALUE;
      goto out;
    }
  } else {
    FPRINTF_ERROR ( "PLMN ID ER_DIAMETER_MISSING_AVP\n");
    result_code = ER_DIAMETER_MISSING_AVP;
    goto out;
  }

  /*
   * Retrieving Terminal-Information AVP
   */
  CHECK_FCT (fd_msg_search_avp (qry, s6a_cnf.dataobj_s6a_terminal_info, &avp));

  if (avp) {
    struct avp                             *child_avp;

    /*
     * Walk through childs avp
     */
    CHECK_FCT (fd_msg_browse (avp, MSG_BRW_FIRST_CHILD, &child_avp, NULL));

    while (child_avp) {
      /*
       * Retrieve the header of the child avp
       */
      CHECK_FCT (fd_msg_avp_hdr (child_avp, &hdr));

      switch (hdr->avp_code) {
      case AVP_CODE_IMEI:{
          /*
           * Check that we do not exceed the maximum size for IMEI
           */
          if (hdr->avp_value->os.len > IMEI_LENGTH) {
            FPRINTF_ERROR ( "terminal info ER_DIAMETER_INVALID_AVP_VALUE\n");
            result_code = ER_DIAMETER_INVALID_AVP_VALUE;
            failed_avp = child_avp;
            goto out;
          }

          sprintf (cass_push.imei, "%*s", (int)hdr->avp_value->os.len, hdr->avp_value->os.data);
          cass_push.imei_present = IMEI_PRESENT;
        }
        break;

      case AVP_CODE_SOFTWARE_VERSION:{
          /*
           * Check the size for SV
           */
          if (hdr->avp_value->os.len != SV_LENGTH) {
            FPRINTF_ERROR ( "software version ER_DIAMETER_INVALID_AVP_VALUE\n");
            result_code = ER_DIAMETER_INVALID_AVP_VALUE;
            failed_avp = child_avp;
            goto out;
          }

          cass_push.software_version[0] = hdr->avp_value->os.data[0];
          cass_push.software_version[1] = hdr->avp_value->os.data[1];
          cass_push.sv_present = SV_PRESENT;
        }
        break;

      default:                 /* Fall through */
      case AVP_CODE_3GPP2_MEID:{
          /*
           * This AVP is not expected on s6a interface
           */
          FPRINTF_ERROR ( "AVP_CODE_3GPP2_MEID ER_DIAMETER_AVP_UNSUPPORTED\n");
          result_code = ER_DIAMETER_AVP_UNSUPPORTED;
          failed_avp = child_avp;
          goto out;
        }
      }

      /*
       * Go to next AVP in the grouped AVP
       */
      CHECK_FCT (fd_msg_browse (child_avp, MSG_BRW_NEXT, &child_avp, NULL));
    }
  }

  /*
   * Retrieving UE-SRVCC AVP
   */
  CHECK_FCT (fd_msg_search_avp (qry, s6a_cnf.dataobj_s6a_ue_srvcc_cap, &avp));

  if (avp) {
    CHECK_FCT (fd_msg_avp_hdr (avp, &hdr));
    cass_push.ue_srvcc_present = UE_SRVCC_PRESENT;
    cass_push.ue_srvcc = hdr->avp_value->u32;
  }

  /*
   * Retrieving Supported features list AVP
   */
  CHECK_FCT (fd_msg_search_avp (qry, s6a_cnf.dataobj_s6a_supported_features, &avp));

  if (avp) {
    struct avp                             *child_avp;

    /*
     * Walk through childs avp
     */
    CHECK_FCT (fd_msg_browse (avp, MSG_BRW_FIRST_CHILD, &child_avp, NULL));

    while (child_avp) {
      /*
       * Retrieve the header of the child avp
       */
      CHECK_FCT (fd_msg_avp_hdr (child_avp, &hdr));

      switch (hdr->avp_code) {
      case AVP_CODE_VENDOR_ID:{
          if (hdr->avp_value->u32 != VENDOR_3GPP) {
            /*
             * features from a vendor other than 3GPP is not supported
             */
            FPRINTF_ERROR ( "Cannot interpret features list with vendor id " "different than 3GPP(%d)\n", VENDOR_3GPP);
            continue;
          }
        }
        break;

      case AVP_CODE_FEATURE_LIST:{
          cass_push.mme_supported_features_present = MME_SUPPORTED_FEATURES_PRESENT;
          cass_push.mme_supported_features = hdr->avp_value->u32;
        }
        break;
      }

      /*
       * Go to next AVP in the grouped AVP
       */
      CHECK_FCT (fd_msg_browse (child_avp, MSG_BRW_NEXT, &child_avp, NULL));
    }
  }

  cassandra_push_up_loc (&cass_push);
  /*
   * ULA flags
   */
  CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_ula_flags, 0, &avp));
  value.u32 = 1;
  CHECK_FCT (fd_msg_avp_setvalue (avp, &value));
  CHECK_FCT (fd_msg_avp_add (ans, MSG_BRW_LAST_CHILD, avp));

  /*
   * Only add the subscriber data if not marked as skipped by MME
   */
  if (!FLAG_IS_SET (ulr_flags, ULR_SKIP_SUBSCRIBER_DATA)) {
    /*if (s6a_add_subscription_data_avp (ans, &cass_ans) != 0) {
      FPRINTF_ERROR ( "ULR_SKIP_SUBSCRIBER_DATA DIAMETER_ERROR_UNKNOWN_EPS_SUBSCRIPTION\n");
      result_code = DIAMETER_ERROR_UNKNOWN_EPS_SUBSCRIPTION;
      experimental = 1;
      goto out;
    }*/
  }

out:
  /*
   * Add the Auth-Session-State AVP
   */
  CHECK_FCT (fd_msg_search_avp (qry, s6a_cnf.dataobj_s6a_auth_session_state, &avp));
  CHECK_FCT (fd_msg_avp_hdr (avp, &hdr));
  CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_auth_session_state, 0, &avp));
  CHECK_FCT (fd_msg_avp_setvalue (avp, hdr->avp_value));
  CHECK_FCT (fd_msg_avp_add (ans, MSG_BRW_LAST_CHILD, avp));
  /*
   * Append the result code to the answer
   */
  CHECK_FCT (s6a_add_result_code (ans, failed_avp, result_code, experimental));
  CHECK_FCT (fd_msg_send (msg, NULL, NULL));
  return 0;
}

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
s6a_generate_insert_subscriber_req(char *previous_mme_host, char *previous_mme_realm, char *imsi)
{
  struct avp                             *avp_p = NULL,*child_avp_p=NULL;
  struct msg                             *msg_p = NULL;
  struct session                         *sess_p = NULL;
  union avp_value                         value;

  //DevAssert (clr_pP );
  /*
   * Create the new clear location request message
   */
  CHECK_FCT (fd_msg_new (s6a_cnf.dataobj_s6a_ins_subsc_data_req, 0, &msg_p));
  /*
   * Create a new session
   */
  CHECK_FCT (fd_sess_new (&sess_p, fd_g_config->cnf_diamid, fd_g_config->cnf_diamid_len, (os0_t) "apps6a", 6));
  {
    os0_t                                   sid;
    size_t                                  sidlen;

    CHECK_FCT (fd_sess_getsid (sess_p, &sid, &sidlen));
    CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_session_id, 0, &avp_p));
    value.os.data = sid;
    value.os.len = sidlen;
    CHECK_FCT (fd_msg_avp_setvalue (avp_p, &value));
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
  CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_imsi, 0, &avp_p));
  value.os.data = (unsigned char *)imsi;
  value.os.len = strlen(imsi);
  CHECK_FCT (fd_msg_avp_setvalue (avp_p, &value));
  CHECK_FCT (fd_msg_avp_add (msg_p, MSG_BRW_LAST_CHILD, avp_p));


  /*Adding subscription data*/

  CHECK_FCT(fd_msg_avp_new(s6a_cnf.dataobj_s6a_subscription_data, 0, &avp_p));

  /*Adding subscriber status child  avp*/
  CHECK_FCT(fd_msg_avp_new(s6a_cnf.dataobj_s6a_subscriber_status, 0, &child_avp_p));
  value.i32 = 0; //Value for SERVICE_GRANTED in subscriber status
  CHECK_FCT(fd_msg_avp_setvalue(child_avp_p,&value));
  CHECK_FCT(fd_msg_avp_add(avp_p,MSG_BRW_LAST_CHILD,child_avp_p));
 
  /*To-Do - Check for Subscriber-Status AVP value and add child avps like HPLMN-ODB AVP*/


  /* Adding access restriction data child avp*/
  CHECK_FCT(fd_msg_avp_new(s6a_cnf.dataobj_s6a_access_restriction_data, 0, &child_avp_p));
  //value.u32 =  
  CHECK_FCT(fd_msg_avp_setvalue(child_avp_p,&value));
  CHECK_FCT(fd_msg_avp_add(avp_p,MSG_BRW_LAST_CHILD,child_avp_p));
 
  /*Adding APN-OI-Replacement child AVP */
  CHECK_FCT(fd_msg_avp_new(s6a_cnf.dataobj_s6a_apn_oi_replacement, 0,&child_avp_p));
  //value.os.data =
  //value.os.len = 
  CHECK_FCT(fd_msg_avp_setvalue(child_avp_p, &value));
  CHECK_FCT(fd_msg_avp_add(avp_p,MSG_BRW_LAST_CHILD, child_avp_p));

  /*Adding APN-Configuration-Profile child AVP */
  //CHECK_FCT(fd_msg_avp_new(s6a_cnf.dataobj_s6a_apn_configuration_profile, 0,&child_avp_p));
  //CHECK_FCT(fd_msg_avp_new(s6a_cnf.dataobj_s6a_context_identifier, 0,&subchild_avp_p));
 
  //CHECK_FCT(fd_msg_avp_new(s6a_cnf.dataobj_s6a_));
  
  
  /*Adding more AVPS if there are any more needed*/ 

  
 
  CHECK_FCT (fd_msg_avp_add (msg_p, MSG_BRW_LAST_CHILD, avp_p));
  CHECK_FCT (fd_msg_send (&msg_p, NULL, NULL));
  FPRINTF_DEBUG ("Sending s6a IDR for imsi=%s\n", imsi);
  return 0;
}




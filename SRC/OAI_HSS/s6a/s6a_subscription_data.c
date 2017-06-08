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


#include "hss_config.h"
#include "db_proto.h"
#include "s6a_proto.h"
#include "fdjson.h"

/*! \file s6a_subscription_data.c
   \brief Add the subscription data to a message. Data are retrieved from database
   \author Sebastien ROUX <sebastien.roux@eurecom.fr>
   \date 2013
   \version 0.1
*/

int
s6a_add_subscription_data_avp (
  struct msg *message,
  cassandra_ul_ans_t * cass_ans)
{
  int                                     ret = -1,
    i = 0;
  cassandra_pdn_t                         pdns;
  struct avp                              *avp = NULL,
    *child_avp = NULL;
  union avp_value                         value;

  
  printf("Inside s6a_add_subscription_data_avp \n");
  if (cass_ans == NULL) {
    return -1;
  }

  memset(&pdns, 0, sizeof(cassandra_pdn_t)); 

  ret = hss_cassandra_query_pdns (cass_ans->imsi, &pdns);

  if (ret != 0) {
    /*
     * mysql query failed:
     * * * * - maybe no more memory
     * * * * - maybe user is not known (should have failed before)
     * * * * - maybe imsi has no EPS subscribed
     * No PDN for this user -> DIAMETER_ERROR_UNKNOWN_EPS_SUBSCRIPTION
     */
    goto out;
  }

  /*
   * Create the Subscription-Data AVP
   */
  CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_subscription_data, 0, &avp));
  {
    uint8_t                                 msisdn_len = strlen (cass_ans->msisdn);

    /*
     * The MSISDN is known in the HSS, add it to the subscription data
     */
    if (msisdn_len > 0) {
      CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_msisdn, 0, &child_avp));
      uint8_t msisdn_tbcd[16] = {0};
      for (int i = 0; i < msisdn_len; i++) {
        if (i & 0x01) {
          msisdn_tbcd[i>>1] = msisdn_tbcd[i>>1] & ((cass_ans->msisdn[i] - 0x30) | 0xF0);
        } else {
          msisdn_tbcd[i>>1] = ((cass_ans->msisdn[i] - 0x30) << 4) | 0x0F;
        }
      }
      value.os.data = (uint8_t *) msisdn_tbcd;
      value.os.len = (msisdn_len + 1) /2;
      CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
      CHECK_FCT (fd_msg_avp_add (avp, MSG_BRW_LAST_CHILD, child_avp));
    }
  }

  /*
   * We have to include the acess-restriction-data if the value stored in DB
   * * * * indicates that at least one restriction is applied to the USER.
   */
  if (cass_ans->access_restriction != 0) {
    CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_access_restriction_data, 0, &child_avp));
    value.u32 = (uint32_t) cass_ans->access_restriction;
    CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
    CHECK_FCT (fd_msg_avp_add (avp, MSG_BRW_LAST_CHILD, child_avp));
  }

  /*
   * Add the Subscriber-Status to the list of AVP.
   * * * * It shall indicate if the service is barred or granted.
   * * * * TODO: normally this parameter comes from DB...
   */
  CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_subscriber_status, 0, &child_avp));
  /*
   * SERVICE_GRANTED
   */
  value.u32 = 0;
  CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
  CHECK_FCT (fd_msg_avp_add (avp, MSG_BRW_LAST_CHILD, child_avp));
  /*
   * Add the Network-Access-Mode to the list of AVP.
   * * * * LTE Standalone HSS/MME: ONLY_PACKET.
   */
  CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_network_access_mode, 0, &child_avp));
  /*
   * SERVICE_GRANTED
   */
  value.u32 = 2;
  CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
  CHECK_FCT (fd_msg_avp_add (avp, MSG_BRW_LAST_CHILD, child_avp));
  /*
   * Add the AMBR to list of AVPs
   */
  {
    struct avp                             *bandwidth;

    CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_ambr, 0, &child_avp));
    /*
     * Uplink bandwidth
     */
    CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_max_bandwidth_ul, 0, &bandwidth));
    value.u32 = cass_ans->aggr_ul;
    CHECK_FCT (fd_msg_avp_setvalue (bandwidth, &value));
    CHECK_FCT (fd_msg_avp_add (child_avp, MSG_BRW_LAST_CHILD, bandwidth));
    /*
     * Downlink bandwidth
     */
    CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_max_bandwidth_dl, 0, &bandwidth));
    value.u32 = cass_ans->aggr_dl;
    CHECK_FCT (fd_msg_avp_setvalue (bandwidth, &value));
    CHECK_FCT (fd_msg_avp_add (child_avp, MSG_BRW_LAST_CHILD, bandwidth));
    CHECK_FCT (fd_msg_avp_add (avp, MSG_BRW_LAST_CHILD, child_avp));
  }

  /*
   * Add the APN-Configuration-Profile 
   */
    struct avp                             *apn_profile;

    CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_apn_configuration_profile, 0, &apn_profile));
    /*
     * Context-Identifier
     */
    CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_context_identifier, 0, &child_avp));
    value.u32 = 0;
    /*
     * TODO: this is the reference to the default APN...
     */
    CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
    CHECK_FCT (fd_msg_avp_add (apn_profile, MSG_BRW_LAST_CHILD, child_avp));
    /*
     * All-APN-Configurations-Included-Indicator
     */
    CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_all_apn_conf_inc_ind, 0, &child_avp));
    value.u32 = 0;
    CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
    CHECK_FCT (fd_msg_avp_add (apn_profile, MSG_BRW_LAST_CHILD, child_avp));

      struct avp                             *apn_configuration;

      /*
       * APN-Configuration
       */
      CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_apn_configuration, 0, &apn_configuration));
      /*
       * Context-Identifier
       */
      CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_context_identifier, 0, &child_avp));
      value.u32 = 0;
      CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
      CHECK_FCT (fd_msg_avp_add (apn_configuration, MSG_BRW_LAST_CHILD, child_avp));
      /*
       * PDN-Type
       */
      CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_pdn_type, 0, &child_avp));
      value.u32 = pdns.pdn_type;
      CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
      CHECK_FCT (fd_msg_avp_add (apn_configuration, MSG_BRW_LAST_CHILD, child_avp));

      if ((pdns.pdn_type == IPV4) || (pdns.pdn_type == IPV4_OR_IPV6) || (pdns.pdn_type == IPV4V6)) {
        s6a_add_ipv4_address (apn_configuration, pdns.pdn_address.ipv4_address);
      }

      if ((pdns.pdn_type == IPV6) || (pdns.pdn_type == IPV4_OR_IPV6) || (pdns.pdn_type == IPV4V6)) {
        s6a_add_ipv6_address (apn_configuration, pdns.pdn_address.ipv6_address);
      }

      /*
       * Service-Selection
       */
      CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_service_selection, 0, &child_avp));
      printf("loading pdns.apn into Service_Selection AVP \n");
      printf(" pdns.apn --> %s \n",pdns.apn);
      value.os.data = (uint8_t *) pdns.apn;
      value.os.len = strlen (pdns.apn);
      CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
      CHECK_FCT (fd_msg_avp_add (apn_configuration, MSG_BRW_LAST_CHILD, child_avp));
      /*
       * Add the eps subscribed qos profile
       */
      {
        struct avp                             *qos_profile,
                                               *allocation_priority;

        CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_eps_subscribed_qos_profile, 0, &qos_profile));
        CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_qos_class_identifier, 0, &child_avp));
        /*
         * For a QCI_1
         */
        value.u32 = (uint32_t) pdns.qci;
        CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
        CHECK_FCT (fd_msg_avp_add (qos_profile, MSG_BRW_LAST_CHILD, child_avp));
        /*
         * Allocation retention priority
         */
        {
          CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_allocation_retention_priority, 0, &allocation_priority));
          /*
           * Priority level
           */
          CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_priority_level, 0, &child_avp));
          value.u32 = (uint32_t) pdns.priority_level;
          CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
          CHECK_FCT (fd_msg_avp_add (allocation_priority, MSG_BRW_LAST_CHILD, child_avp));
          /*
           * Pre-emption-capability
           */
          CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_pre_emption_capability, 0, &child_avp));
          value.u32 = (uint32_t) pdns.pre_emp_cap;
          CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
          CHECK_FCT (fd_msg_avp_add (allocation_priority, MSG_BRW_LAST_CHILD, child_avp));
          /*
           * Pre-emption-vulnerability
           */
          CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_pre_emption_vulnerability, 0, &child_avp));
          value.u32 = (uint32_t) pdns.pre_emp_vul;
          CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
          CHECK_FCT (fd_msg_avp_add (allocation_priority, MSG_BRW_LAST_CHILD, child_avp));
          CHECK_FCT (fd_msg_avp_add (qos_profile, MSG_BRW_LAST_CHILD, allocation_priority));
        }
        CHECK_FCT (fd_msg_avp_add (apn_configuration, MSG_BRW_LAST_CHILD, qos_profile));
      }
      /*
       * Add the AMBR to list of AVPs
       */
      {
        struct avp                             *bandwidth;

        CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_ambr, 0, &bandwidth));
        /*
         * Uplink bandwidth
         */
        CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_max_bandwidth_ul, 0, &child_avp));
        value.u32 = (uint32_t) pdns.aggr_ul;
        CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
        CHECK_FCT (fd_msg_avp_add (bandwidth, MSG_BRW_LAST_CHILD, child_avp));
        /*
         * Downlink bandwidth
         */
        CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_max_bandwidth_dl, 0, &child_avp));
        value.u32 = (uint32_t) pdns.aggr_dl;
        CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
        CHECK_FCT (fd_msg_avp_add (bandwidth, MSG_BRW_LAST_CHILD, child_avp));
        CHECK_FCT (fd_msg_avp_add (apn_configuration, MSG_BRW_LAST_CHILD, bandwidth));
      }
      CHECK_FCT (fd_msg_avp_add (apn_profile, MSG_BRW_LAST_CHILD, apn_configuration));

    CHECK_FCT (fd_msg_avp_add (avp, MSG_BRW_LAST_CHILD, apn_profile));

  /*
   * Subscribed-Periodic-RAU-TAU-Timer
   */
  CHECK_FCT (fd_msg_avp_new (s6a_cnf.dataobj_s6a_subscribed_rau_tau_timer, 0, &child_avp));
  /*
   * Request an RAU/TAU update every x seconds
   */
  value.u32 = (uint32_t) cass_ans->rau_tau;
  CHECK_FCT (fd_msg_avp_setvalue (child_avp, &value));
  CHECK_FCT (fd_msg_avp_add (avp, MSG_BRW_LAST_CHILD, child_avp));
  /*
   * Add the AVP to the message
   */
  CHECK_FCT (fd_msg_avp_add (message, MSG_BRW_LAST_CHILD, avp));

out:

  return ret;
}


void display_error_message(const char *err_msg)
{
	printf("inside display_error_message\n");
	printf("%s\n",err_msg);
}

int s6a_add_subscription_json_data_avp(
	struct msg *message,
	char *json_string){

	printf("******* subscription data in json string format ****\n\n");
	printf("%s\n",json_string);
	printf(" ************************\n\n");
	printf("%s:%d\n", __FILE__, __LINE__ );
	fdJsonAddAvps(json_string, message, &display_error_message );




	return 0;

}

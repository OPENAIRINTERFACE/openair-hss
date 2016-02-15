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


/*! \file s6a_peers.c
   \brief Add a new entity to the list of peers to connect
   \author Sebastien ROUX <sebastien.roux@eurecom.fr>
   \date 2013
   \version 0.1
*/

#include <stdint.h>
#include <unistd.h>

#include "common_types.h"
#include "intertask_interface.h"
#include "s6a_defs.h"
#include "s6a_messages.h"
#include "assertions.h"
#include "dynamic_memory_check.h"
#include "log.h"


void
s6a_peer_connected_cb (
  struct peer_info *info,
  void *arg)
{
  if (info == NULL) {
    LOG_ERROR (LOG_S6A, "Failed to connect to HSS entity\n");
  } else {
    MessageDef                             *message_p;

    LOG_DEBUG (LOG_S6A, "Peer %*s is now connected...\n", (int)info->pi_diamidlen, info->pi_diamid);
    /*
     * Inform S1AP that connection to HSS is established
     */
    message_p = itti_alloc_new_message (TASK_S6A, ACTIVATE_MESSAGE);
    itti_send_msg_to_task (TASK_S1AP, INSTANCE_DEFAULT, message_p);
  }

  /*
   * For test
   */
#if 0
  s6a_auth_info_req_t                     s6a_air;

  memset (&s6a_air, 0, sizeof (s6a_auth_info_req_t));
  sprintf (s6a_air.imsi, "%14llu", 20834123456789ULL);
  s6a_air.nb_of_vectors = 1;
  s6a_air.visited_plmn.MCCdigit2 = 0,
    s6a_air.visited_plmn.MCCdigit1 = 8, s6a_air.visited_plmn.MCCdigit3 = 2, s6a_air.visited_plmn.MNCdigit1 = 0, s6a_air.visited_plmn.MNCdigit2 = 3, s6a_air.visited_plmn.MNCdigit3 = 4, s6a_generate_authentication_info_req (&s6a_air);
  // #else
  //     s6a_update_location_req_t s6a_ulr;
  //
  //     memset(&s6a_ulr, 0, sizeof(s6a_update_location_req_t));
  //
  //     sprintf(s6a_ulr.imsi, "%14llu", 20834123456789ULL);
  //     s6a_ulr.initial_attach = INITIAL_ATTACH;
  //     s6a_ulr.rat_type = RAT_EUTRAN;
  //     s6a_generate_update_location(&s6a_ulr);
#endif
}

int
s6a_fd_new_peer (
  void)
{
  char                                    host_name[100];
  size_t                                  host_name_len = 0;
  char                                   *hss_name = NULL;
  int                                     ret = 0;
  struct peer_info                        info;

  memset (&info, 0, sizeof (struct peer_info));

  if (config_read_lock (&mme_config) ) {
    LOG_ERROR (LOG_S6A, "Failed to lock configuration for reading\n");
    return RETURNerror;
  }

  if (fd_g_config->cnf_diamid ) {
    free (fd_g_config->cnf_diamid);
    fd_g_config->cnf_diamid_len = 0;
  }

  DevAssert (gethostname (host_name, 100) == 0);
  host_name_len = strlen (host_name);
  host_name[host_name_len] = '.';
  host_name[host_name_len + 1] = '\0';
  strcat (host_name, mme_config.realm);
  fd_g_config->cnf_diamid = STRDUP_CHECK (host_name);
  fd_g_config->cnf_diamid_len = strlen (fd_g_config->cnf_diamid);
  LOG_DEBUG (LOG_S6A, "Diameter identity of MME: %s with length: %zd\n", fd_g_config->cnf_diamid, fd_g_config->cnf_diamid_len);
  hss_name = CALLOC_CHECK (1, 100);
  strcat (hss_name, mme_config.s6a_config.hss_host_name);
  strcat (hss_name, ".");
  strcat (hss_name, mme_config.realm);
  info.pi_diamid = hss_name;
  info.pi_diamidlen = strlen (info.pi_diamid);
  LOG_DEBUG (LOG_S6A, "Diameter identity of HSS: %s with length: %zd\n", info.pi_diamid, info.pi_diamidlen);
  info.config.pic_flags.sec = PI_SEC_NONE;
  info.config.pic_flags.pro4 = PI_P4_SCTP;
  info.config.pic_flags.persist = PI_PRST_NONE;
  CHECK_FCT (fd_peer_add (&info, "", s6a_peer_connected_cb, NULL));

  if (config_unlock (&mme_config) ) {
    LOG_ERROR (LOG_S6A, "Failed to unlock configuration\n");
    return RETURNerror;
  }

  return ret;
}

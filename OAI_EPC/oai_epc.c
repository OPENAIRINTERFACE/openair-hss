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


/*! @file  oai_epc.c
    @brief Entry point for EPC environement gathering MME, S-GW and P-GW in
           a single executable
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#if HAVE_CONFIG_H
#  include "config.h"
#endif
#include "mme_config.h"
#include "gtpv1u_sgw_defs.h"
#include "assertions.h"
#include "intertask_interface_init.h"
#include "sctp_primitives_server.h"
#include "udp_primitives_server.h"
#include "s1ap_mme.h"
#include "log.h"
#include "timer.h"
#include "sgw_lite_defs.h"
#include "mme_app_extern.h"
#include "nas_defs.h"
/* FreeDiameter headers for support of S6A interface */
#include <freeDiameter/freeDiameter-host.h>
#include <freeDiameter/libfdcore.h>
#include "s6a_defs.h"
#include "oai_epc.h"
#include "msc.h"

int
main (
  int argc,
  char *argv[])
{
  DYN_MEM_CHECK_INIT();
  CHECK_INIT_RETURN (LOG_INIT (LOG_MME_GW_ENV, LOG_LEVEL_TRACE, MAX_LOG_PROTOS*2));
  /*
   * Parse the command line for options and set the mme_config accordingly.
   */
  CHECK_INIT_RETURN (config_parse_opt_line (argc, argv, &mme_config) < 0);
  CHECK_INIT_RETURN (itti_init (TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info, messages_definition_xml, mme_config.itti_config.log_file));
  MSC_INIT (MSC_MME_GW, THREAD_MAX + TASK_MAX);
  /*
   * Calling each layer init function
   */
  CHECK_INIT_RETURN (nas_init (&mme_config));
  CHECK_INIT_RETURN (sctp_init (&mme_config));
  CHECK_INIT_RETURN (udp_init (&mme_config));
  CHECK_INIT_RETURN (gtpv1u_init (&mme_config));
  CHECK_INIT_RETURN (s1ap_mme_init (&mme_config));
  CHECK_INIT_RETURN (mme_app_init (&mme_config));
  CHECK_INIT_RETURN (s6a_init (&mme_config));
  CHECK_INIT_RETURN (sgw_lite_init (mme_config.config_file));
  /*
   * Handle signals here
   */
  itti_wait_tasks_end ();
  return 0;
}

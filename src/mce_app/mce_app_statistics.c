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

/*! \file mce_app_statistics.c
  \brief
  \author Dincer BEKEN
  \company Blackned GmbH
*/

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"

#include "log.h"
#include "intertask_interface.h"
#include "mce_app_statistics.h"
#include "mce_app_defs.h"



int mce_app_statistics_display (
  void)
{
  OAILOG_DEBUG (LOG_MCE_APP, "======================================= STATISTICS ============================================\n\n");
  OAILOG_DEBUG (LOG_MCE_APP, "               |   Current Status| Added since last display|  Removed since last display |\n");
  OAILOG_DEBUG (LOG_MCE_APP, "Connected eNBs | %10u      |     %10u              |    %10u               |\n",mce_app_desc.nb_enb_connected,
                                          mce_app_desc.nb_enb_connected_since_last_stat, mce_app_desc.nb_enb_released_since_last_stat);
  OAILOG_DEBUG (LOG_MCE_APP, "MBMS Services  | %10u      |     %10u              |    %10u               |\n",mce_app_desc.nb_mbms_services,
                                          mce_app_desc.nb_mbms_services_added_since_last_stat,mce_app_desc.nb_mbms_services_removed_since_last_stat);
  OAILOG_DEBUG (LOG_MCE_APP, "MBMS M1-U Bearers   | %10u      |     %10u              |    %10u               |\n\n",mce_app_desc.nb_mbms_m1u_bearers,
                                          mce_app_desc.nb_mbms_m1u_bearers_established_since_last_stat,mce_app_desc.nb_mbms_m1u_bearers_released_since_last_stat);
  OAILOG_DEBUG (LOG_MCE_APP, "======================================= STATISTICS ============================================\n\n");

  mce_stats_write_lock (&mce_app_desc);

  // resetting stats for next display
  mce_app_desc.nb_enb_connected_since_last_stat = 0;
  mce_app_desc.nb_enb_released_since_last_stat  = 0;
  mce_app_desc.nb_mbms_services_added_since_last_stat  = 0;
  mce_app_desc.nb_mbms_services_removed_since_last_stat  = 0;
  mce_app_desc.nb_mbms_m1u_bearers_established_since_last_stat = 0;
  mce_app_desc.nb_mbms_m1u_bearers_released_since_last_stat = 0;

  mce_stats_unlock(&mce_app_desc);

  return 0;
}

/*********************************** Utility Functions to update Statistics**************************************/

// Number of Connected eNBs
void update_mce_app_stats_connected_enb_add(void)
{
  mce_stats_write_lock (&mce_app_desc);
  (mce_app_desc.nb_enb_connected)++;
  (mce_app_desc.nb_enb_connected_since_last_stat)++;
  mce_stats_unlock(&mce_app_desc);
  return;
}
void update_mce_app_stats_connected_enb_sub(void)
{
  mce_stats_write_lock (&mce_app_desc);
  if (mce_app_desc.nb_enb_connected !=0)
    (mce_app_desc.nb_enb_connected)--;
  (mce_app_desc.nb_enb_released_since_last_stat)--;
  mce_stats_unlock(&mce_app_desc);
  return;
}

/*****************************************************/
// Number of Connected UEs
void update_mce_app_stats_mbms_services_add(void)
{
  mce_stats_write_lock (&mce_app_desc);
  (mce_app_desc.nb_mbms_services)++;
  (mce_app_desc.nb_mbms_services_added_since_last_stat)++;
  mce_stats_unlock(&mce_app_desc);
  return;
}
void update_mce_app_stats_mbms_services_sub(void)
{
  mce_stats_write_lock (&mce_app_desc);
  if (mce_app_desc.nb_mbms_services !=0)
    (mce_app_desc.nb_mbms_services)--;
  (mce_app_desc.nb_mbms_services_removed_since_last_stat)++;
  mce_stats_unlock(&mce_app_desc);
  return;
}

/*****************************************************/
// Number of S1U Bearers
void update_mce_app_stats_mbms_m1u_bearer_add(void)
{
  mce_stats_write_lock (&mce_app_desc);
  (mce_app_desc.nb_mbms_m1u_bearers)++;
  (mce_app_desc.nb_mbms_m1u_bearers_established_since_last_stat)++;
  mce_stats_unlock(&mce_app_desc);
  return;
}
void update_mce_app_stats_mbms_m1u_bearer_sub(void)
{
  mce_stats_write_lock (&mce_app_desc);
  if (mce_app_desc.nb_mbms_m1u_bearers !=0)
    (mce_app_desc.nb_mbms_m1u_bearers)--;
  (mce_app_desc.nb_mbms_m1u_bearers_released_since_last_stat)++;
  mce_stats_unlock(&mce_app_desc);
  return;
}
/*****************************************************/


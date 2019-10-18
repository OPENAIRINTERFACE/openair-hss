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

/*! \file mce_app_statistics.h
  \brief
  \author Dincer BEKEN
  \company Blackned GmbH
*/

#ifndef FILE_MCE_APP_STATISTICS_SEEN
#define FILE_MCE_APP_STATISTICS_SEEN

int mce_app_statistics_display(void);

/*********************************** Utility Functions to update Statistics**************************************/
void update_mce_app_stats_connected_enb_add(void);
void update_mce_app_stats_connected_enb_sub(void);
void update_mce_app_stats_active_mbms_service_add(void);
void update_mce_app_stats_active_mbms_service_sub(void);
void update_mce_app_stats_m1u_bearer_add(void);
void update_mce_app_stats_m1u_bearer_sub(void);

#endif /* FILE_MCE_APP_STATISTICS_SEEN */

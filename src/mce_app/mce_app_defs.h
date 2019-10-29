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

/*! \file mce_app_defs.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

/* This file contains definitions related to mme applicative layer and should
 * not be included within other layers.
 * Use mce_app_extern.h to expose mme applicative layer procedures/data.
 */


#ifndef FILE_MCE_APP_DEFS_SEEN
#define FILE_MCE_APP_DEFS_SEEN
#include "intertask_interface.h"
#include "mce_app_mbms_service_context.h"
#define CHANGEABLE_VALUE 256

#define MAX_MBMS_BEARER mme_config.max_mbms_service
typedef struct mce_app_desc_s {
  /* MBMS Service contexts + some statistics variables */
  mce_mbms_services_t		 mce_mbms_service_contexts;

  long statistic_timer_id;
  uint32_t statistic_timer_period;

  /** Create an array of MBMS service pools. */
  mbms_service_t 			mbms_services[CHANGEABLE_VALUE];
  STAILQ_HEAD(mbms_services_list_s, mbms_service_s)  mce_mbms_services_list;

  /* Reader/writer lock */
  pthread_rwlock_t rw_lock;

  /* ***************Statistics*************
   * number of active MBMS Services
   * number of active M1U MBMS bearers,
   */

  uint32_t               nb_enb_connected;
  uint32_t               nb_mbms_services;
  uint32_t               nb_mbms_m1u_bearers;

  /* ***************Changes in Statistics**************/

  // todo: recheck these..
  uint32_t               nb_mbms_services_added_since_last_stat;
  uint32_t               nb_mbms_services_removed_since_last_stat;
  uint32_t               nb_mbms_m1u_bearers_established_since_last_stat;
  uint32_t               nb_mbms_m1u_bearers_released_since_last_stat;
  uint32_t               nb_enb_connected_since_last_stat;
  uint32_t               nb_enb_released_since_last_stat;
} mce_app_desc_t;

extern mce_app_desc_t mce_app_desc;

//void mce_app_handle_m2ap_enb_deregistered_ind (const itti_m2ap_eNB_deregistered_ind_t * const enb_dereg_ind);

bool mce_app_dump_mbms_service(const hash_key_t keyP, void *const mbms_service, void *unused_param_pP, void **unused_result_pP);

/** Handling Sm Messages. */
void mce_app_handle_mbms_session_start_request( itti_sm_mbms_session_start_request_t * const mbms_session_start_pP );
void mce_app_handle_mbms_session_update_request( itti_sm_mbms_session_update_request_t * const mbms_session_update_pP );
void mce_app_handle_mbms_session_stop_request( itti_sm_mbms_session_stop_request_t * const mbms_session_stop_pP );

void mce_app_handle_mbms_session_duration_timer_expiry (const struct tmgi_s *tmgi, const mbms_service_area_id_t mbms_service_area_id);

#define mce_stats_read_lock(mCEsTATS)  pthread_rwlock_rdlock(&(mCEsTATS)->rw_lock)
#define mce_stats_write_lock(mCEsTATS) pthread_rwlock_wrlock(&(mCEsTATS)->rw_lock)
#define mce_stats_unlock(mCEsTATS)     pthread_rwlock_unlock(&(mCEsTATS)->rw_lock)

#endif /* MCE_APP_DEFS_H_ */

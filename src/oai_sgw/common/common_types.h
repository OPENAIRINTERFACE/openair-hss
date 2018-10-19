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

/*! \file common_types.c
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_COMMON_TYPES_SEEN
#define FILE_COMMON_TYPES_SEEN

#include "common_root_types.h"
#include "3gpp_33.401.h"
#include "3gpp_36.401.h"
#include "3gpp_24.008.h"
#include "3gpp_24.007.h"
#include "3gpp_29.274.h"
#include "security_types.h"

#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ipv4_list_elm_s {
  STAILQ_ENTRY(ipv4_list_elm_s) ipv4_entries;
  struct in_addr  addr;
};

typedef struct ebi_list_s {
  uint32_t   num_ebi;
  #define RELEASE_ACCESS_BEARER_MAX_BEARERS   8
  ebi_t      ebis[RELEASE_ACCESS_BEARER_MAX_BEARERS]  ;
} ebi_list_t;

typedef struct apn_configuration_s {
  context_identifier_t context_identifier;

  /* Each APN configuration can have 0, 1, or 2 ip address:
   * - 0 means subscribed is dynamically allocated by P-GW depending on the
   * pdn_type
   * - 1 Only one type of IP address is returned by HSS
   * - 2 IPv4 and IPv6 address are returned by HSS and are statically
   * allocated
   */
  uint8_t nb_ip_address;
  ip_address_t ip_address[2];

#ifdef ACCESS_POINT_NAME_MAX_LENGTH
#define SERVICE_SELECTION_MAX_LENGTH ACCESS_POINT_NAME_MAX_LENGTH
#else
#define SERVICE_SELECTION_MAX_LENGTH 100
#endif
  pdn_type_t  pdn_type;
  char        service_selection[SERVICE_SELECTION_MAX_LENGTH];
  int         service_selection_length;
  eps_subscribed_qos_profile_t subscribed_qos;
  ambr_t ambr;
} apn_configuration_t;

typedef enum {
  ALL_APN_CONFIGURATIONS_INCLUDED = 0,
  MODIFIED_ADDED_APN_CONFIGURATIONS_INCLUDED = 1,
  ALL_APN_MAX,
} all_apn_conf_ind_t;

typedef struct {
  context_identifier_t context_identifier;
  all_apn_conf_ind_t   all_apn_conf_ind;
  /* Number of APNs provided */
  uint8_t nb_apns;
  /* List of APNs configuration 1 to n elements */
  struct apn_configuration_s apn_configuration[MAX_APN_PER_UE];
} apn_config_profile_t;

typedef struct {
  subscriber_status_t   subscriber_status;
  char                  msisdn[MSISDN_LENGTH + 1];
  uint8_t               msisdn_length;
  network_access_mode_t access_mode;
  access_restriction_t  access_restriction;
  ambr_t                subscribed_ambr;
  apn_config_profile_t  apn_config_profile;
  rau_tau_timer_t       rau_tau_timer;
} subscription_data_t;

typedef struct authentication_info_s {
  uint8_t         nb_of_vectors;
  eutran_vector_t eutran_vector[MAX_EPS_AUTH_VECTORS];
} authentication_info_t;

#ifdef __cplusplus
}
#endif
#endif /* FILE_COMMON_TYPES_SEEN */

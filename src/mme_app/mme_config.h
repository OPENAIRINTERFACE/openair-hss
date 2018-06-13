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

/*! \file mme_config.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_MME_CONFIG_SEEN
#define FILE_MME_CONFIG_SEEN
#include <arpa/inet.h>

#include "mme_default_values.h"
#include "3gpp_23.003.h"
#include "common_dim.h"
#include "common_types.h"
#include "bstrlib.h"
#include "log.h"

#define MAX_GUMMEI                2

#define MME_CONFIG_STRING_MME_CONFIG                     "MME"
#define MME_CONFIG_STRING_PID_DIRECTORY                  "PID_DIRECTORY"

// todo: what is run mode?
#define MME_CONFIG_STRING_RUN_MODE                       "RUN_MODE"
#define MME_CONFIG_STRING_RUN_MODE_TEST                  "TEST"

#define MME_CONFIG_STRING_REALM                          "REALM"
#define MME_CONFIG_STRING_MAXENB                         "MAXENB"
#define MME_CONFIG_STRING_MAXUE                          "MAXUE"
#define MME_CONFIG_STRING_RELATIVE_CAPACITY              "RELATIVE_CAPACITY"
#define MME_CONFIG_STRING_STATISTIC_TIMER                "MME_STATISTIC_TIMER"
#define MME_CONFIG_STRING_MME_MOBILITY_COMPLETION_TIMER  "MME_MOBILITY_COMPLETION_TIMER"
#define MME_CONFIG_STRING_MME_S10_HANDOVER_COMPLETION_TIMER  "MME_S10_HANDOVER_COMPLETION_TIMER"

#define MME_CONFIG_STRING_EMERGENCY_ATTACH_SUPPORTED     "EMERGENCY_ATTACH_SUPPORTED"
#define MME_CONFIG_STRING_UNAUTHENTICATED_IMSI_SUPPORTED "UNAUTHENTICATED_IMSI_SUPPORTED"
#define MME_CONFIG_STRING_DUMMY_HANDOVER_FORWARDING_ENABLED "DUMMY_HANDOVER_FORWARDING_ENABLED"

#define EPS_NETWORK_FEATURE_SUPPORT_IMS_VOICE_OVER_PS_SESSION_IN_S1       "EPS_NETWORK_FEATURE_SUPPORT_IMS_VOICE_OVER_PS_SESSION_IN_S1"
#define EPS_NETWORK_FEATURE_SUPPORT_EMERGENCY_BEARER_SERVICES_IN_S1_MODE  "EPS_NETWORK_FEATURE_SUPPORT_EMERGENCY_BEARER_SERVICES_IN_S1_MODE"
#define EPS_NETWORK_FEATURE_SUPPORT_LOCATION_SERVICES_VIA_EPC             "EPS_NETWORK_FEATURE_SUPPORT_LOCATION_SERVICES_VIA_EPC"
#define EPS_NETWORK_FEATURE_SUPPORT_EXTENDED_SERVICE_REQUEST              "EPS_NETWORK_FEATURE_SUPPORT_EXTENDED_SERVICE_REQUEST"


#define MME_CONFIG_STRING_INTERTASK_INTERFACE_CONFIG     "INTERTASK_INTERFACE"
#define MME_CONFIG_STRING_INTERTASK_INTERFACE_QUEUE_SIZE "ITTI_QUEUE_SIZE"

#define MME_CONFIG_STRING_S6A_CONFIG                     "S6A"
#define MME_CONFIG_STRING_S6A_CONF_FILE_PATH             "S6A_CONF"
#define MME_CONFIG_STRING_S6A_HSS_HOSTNAME               "HSS_HOSTNAME"
#define MME_CONFIG_STRING_S6A_MME_HOSTNAME               "MME_HOSTNAME"

#define MME_CONFIG_STRING_SCTP_CONFIG                    "SCTP"
#define MME_CONFIG_STRING_SCTP_INSTREAMS                 "SCTP_INSTREAMS"
#define MME_CONFIG_STRING_SCTP_OUTSTREAMS                "SCTP_OUTSTREAMS"


#define MME_CONFIG_STRING_S1AP_CONFIG                    "S1AP"
#define MME_CONFIG_STRING_S1AP_OUTCOME_TIMER             "S1AP_OUTCOME_TIMER"
#define MME_CONFIG_STRING_S1AP_PORT                      "S1AP_PORT"

#define MME_CONFIG_STRING_GUMMEI_LIST                    "GUMMEI_LIST"
#define MME_CONFIG_STRING_MME_CODE                       "MME_CODE"
#define MME_CONFIG_STRING_MME_GID                        "MME_GID"
#define MME_CONFIG_STRING_TAI_LIST                       "TAI_LIST"
#define MME_CONFIG_STRING_MCC                            "MCC"
#define MME_CONFIG_STRING_MNC                            "MNC"
#define MME_CONFIG_STRING_TAC                            "TAC"
#define MME_CONFIG_STRING_NGHB_MME_IPV4_ADDR             "NGHB_MME_IPV4_ADDR"

#define MME_CONFIG_STRING_NETWORK_INTERFACES_CONFIG      "NETWORK_INTERFACES"
#define MME_CONFIG_STRING_INTERFACE_NAME_FOR_S1_MME      "MME_INTERFACE_NAME_FOR_S1_MME"
#define MME_CONFIG_STRING_IPV4_ADDRESS_FOR_S1_MME        "MME_IPV4_ADDRESS_FOR_S1_MME"
#define MME_CONFIG_STRING_INTERFACE_NAME_FOR_S11_MME     "MME_INTERFACE_NAME_FOR_S11_MME"
#define MME_CONFIG_STRING_IPV4_ADDRESS_FOR_S11_MME       "MME_IPV4_ADDRESS_FOR_S11_MME"
#define MME_CONFIG_STRING_MME_PORT_FOR_S11               "MME_PORT_FOR_S11_MME"
#define MME_CONFIG_STRING_INTERFACE_NAME_FOR_S10_MME     "MME_INTERFACE_NAME_FOR_S10_MME"
#define MME_CONFIG_STRING_IPV4_ADDRESS_FOR_S10_MME       "MME_IPV4_ADDRESS_FOR_S10_MME"
#define MME_CONFIG_STRING_MME_PORT_FOR_S10               "MME_PORT_FOR_S10_MME"


#define MME_CONFIG_STRING_NAS_CONFIG                     "NAS"
#define MME_CONFIG_STRING_NAS_SUPPORTED_INTEGRITY_ALGORITHM_LIST  "ORDERED_SUPPORTED_INTEGRITY_ALGORITHM_LIST"
#define MME_CONFIG_STRING_NAS_SUPPORTED_CIPHERING_ALGORITHM_LIST  "ORDERED_SUPPORTED_CIPHERING_ALGORITHM_LIST"

#define MME_CONFIG_STRING_NAS_T3346_TIMER                "T3346"
#define MME_CONFIG_STRING_NAS_T3402_TIMER                "T3402"
#define MME_CONFIG_STRING_NAS_T3412_TIMER                "T3412"
#define MME_CONFIG_STRING_NAS_T3422_TIMER                "T3422"
#define MME_CONFIG_STRING_NAS_T3450_TIMER                "T3450"
#define MME_CONFIG_STRING_NAS_T3460_TIMER                "T3460"
#define MME_CONFIG_STRING_NAS_T3470_TIMER                "T3470"
#define MME_CONFIG_STRING_NAS_T3485_TIMER                "T3485"
#define MME_CONFIG_STRING_NAS_T3486_TIMER                "T3486"
#define MME_CONFIG_STRING_NAS_T3489_TIMER                "T3489"
#define MME_CONFIG_STRING_NAS_T3346_TIMER                "T3346"
#define MME_CONFIG_STRING_NAS_T3495_TIMER                "T3495"

#define MME_CONFIG_STRING_NAS_DISABLE_ESM_INFORMATION_PROCEDURE    "DISABLE_ESM_INFORMATION_PROCEDURE"
#define MME_CONFIG_STRING_NAS_FORCE_PUSH_DEDICATED_BEARER "FORCE_PUSH_DEDICATED_BEARER"


//#define MME_CONFIG_STRING_NAS_FORCE_PUSH_DEDICATED_BEARER "FORCE_PUSH_DEDICATED_BEARER"
#define MME_CONFIG_STRING_MME_IPV4_ADDRESS_FOR_S10        "MME_IPV4_ADDRESS_FOR_S10"

#define MME_CONFIG_STRING_ASN1_VERBOSITY                 "ASN1_VERBOSITY"
#define MME_CONFIG_STRING_ASN1_VERBOSITY_NONE            "none"
#define MME_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING        "annoying"
#define MME_CONFIG_STRING_ASN1_VERBOSITY_INFO            "info"

#define MME_CONFIG_STRING_WRR_LIST_SELECTION             "WRR_LIST_SELECTION"
///** MME S10 List --> todo: later FULL WRR : Finding MME via eNB. */
//#define MME_CONFIG_STRING_MME_LIST_SELECTION             "MME_LIST_SELECTION"

#define MME_CONFIG_STRING_ID                             "ID"

typedef enum {
   RUN_MODE_BASIC,
   RUN_MODE_SCENARIO_PLAYER
} run_mode_t;

// todo: run mode?
//typedef enum {
//  RUN_MODE_TEST = 0,
//  RUN_MODE_OTHER
//} run_mode_t;

typedef struct mme_config_s {
  /* Reader/writer lock for this configuration */
  pthread_rwlock_t rw_lock;


  bstring config_file;
  bstring pid_dir;
  bstring realm;

  run_mode_t  run_mode;

  uint32_t max_enbs;
  uint32_t max_ues;

  uint8_t relative_capacity;

  uint32_t mme_statistic_timer;
  uint32_t mme_mobility_completion_timer;
  uint32_t mme_s10_handover_completion_timer;

  uint8_t unauthenticated_imsi_supported;
  uint8_t dummy_handover_forwarding_enabled;

  struct {
    uint8_t ims_voice_over_ps_session_in_s1;
    uint8_t emergency_bearer_services_in_s1_mode;
    uint8_t location_services_via_epc;
    uint8_t extended_service_request;
  } eps_network_feature_support;

  struct {
    int      nb;
    gummei_t gummei[MAX_GUMMEI];
  } gummei;


#define TRACKING_AREA_IDENTITY_LIST_TYPE_ONE_PLMN_NON_CONSECUTIVE_TACS 0x00
#define TRACKING_AREA_IDENTITY_LIST_TYPE_ONE_PLMN_CONSECUTIVE_TACS     0x01
#define TRACKING_AREA_IDENTITY_LIST_TYPE_MANY_PLMNS                    0x02
  struct {
    uint8_t   list_type;
    uint8_t   nb_tai;
    uint16_t *plmn_mcc;
    uint16_t *plmn_mnc;
    uint16_t *plmn_mnc_len;
    uint16_t *tac;
  } served_tai;

  struct {
    uint16_t in_streams;
    uint16_t out_streams;
  } sctp_config;

  struct {
    uint16_t port_number;
    uint8_t  outcome_drop_timer_sec;
  } s1ap_config;

  struct {
    bstring    if_name_s1_mme;
    struct in_addr s1_mme;
    int        netmask_s1_mme;

    bstring    if_name_s11;
    struct in_addr s11;
    int        netmask_s11;
    uint16_t   port_s11;

    bstring    if_name_s10;
    struct in_addr s10;
    int        netmask_s10;
    uint16_t   port_s10;

    // todo: sgw_s11 removed, finding it dynamically?
//    ipv4_nbo_t sgw_s11;

  } ipv4;

  struct {
    bstring conf_file;
    bstring hss_host_name;
    bstring mme_host_name;
  } s6a_config;

  struct {
    uint32_t  queue_size;
    bstring   log_file;
  } itti_config;

  struct {
    uint8_t  prefered_integrity_algorithm[8];
    uint8_t  prefered_ciphering_algorithm[8];
    uint32_t t3346_sec;
    uint32_t t3402_min;
    uint32_t t3412_min;
    uint32_t t3422_sec;
    uint32_t t3450_sec;
    uint32_t t3460_sec;
    uint32_t t3470_sec;
    uint32_t t3485_sec;
    uint32_t t3486_sec;
    uint32_t t3489_sec;
    uint32_t t3495_sec;

    // non standart features
    bool     force_reject_tau;
    bool     force_reject_sr;
    bool     disable_esm_information;
  } nas_config;

  struct {
    int nb_service_entries;
//#define MME_CONFIG_MAX_SGW 16
#define MME_CONFIG_MAX_SERVICE 64

    bstring        service_id[MME_CONFIG_MAX_SERVICE];
    struct in_addr service_ip_addr[MME_CONFIG_MAX_SERVICE];
    /** MME entries. */

  } e_dns_emulation;

#if TRACE_XML
  struct {
    bstring scenario_file;
    bool    stop_on_error;
  } scenario_player_config;
#endif

  log_config_t log_config;
} mme_config_t;

extern mme_config_t mme_config;

int mme_config_find_mnc_length(const char mcc_digit1P,
                               const char mcc_digit2P,
                               const char mcc_digit3P,
                               const char mnc_digit1P,
                               const char mnc_digit2P,
                               const char mnc_digit3P);
int mme_config_parse_opt_line(int argc, char *argv[], mme_config_t *mme_config);

void mme_config_exit (void);

#define mme_config_read_lock(mMEcONFIG)  pthread_rwlock_rdlock(&(mMEcONFIG)->rw_lock)
#define mme_config_write_lock(mMEcONFIG) pthread_rwlock_wrlock(&(mMEcONFIG)->rw_lock)
#define mme_config_unlock(mMEcONFIG)     pthread_rwlock_unlock(&(mMEcONFIG)->rw_lock)

#endif /* FILE_MME_CONFIG_SEEN */

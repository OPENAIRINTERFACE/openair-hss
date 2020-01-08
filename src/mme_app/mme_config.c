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

/*! \file mme_config.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#if HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <libconfig.h>

#include <arpa/inet.h>          /* To provide inet_addr */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "assertions.h"
#include "dynamic_memory_check.h"
#include "log.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "common_defs.h"
#include "mme_config.h"
#include "s1ap_mme_ta.h"

struct mme_config_s                       mme_config = {.rw_lock = PTHREAD_RWLOCK_INITIALIZER, 0};

//------------------------------------------------------------------------------
int mme_config_find_mnc_length (
  const char mcc_digit1P,
  const char mcc_digit2P,
  const char mcc_digit3P,
  const char mnc_digit1P,
  const char mnc_digit2P,
  const char mnc_digit3P)
{
  uint16_t                                mcc = 100 * mcc_digit1P + 10 * mcc_digit2P + mcc_digit3P;
  uint16_t                                mnc3 = 100 * mnc_digit1P + 10 * mnc_digit2P + mnc_digit3P;
  uint16_t                                mnc2 = 10 * mnc_digit1P + mnc_digit2P;
  int                                     plmn_index = 0;

  AssertFatal ((mcc_digit1P >= 0) && (mcc_digit1P <= 9)
               && (mcc_digit2P >= 0) && (mcc_digit2P <= 9)
               && (mcc_digit3P >= 0) && (mcc_digit3P <= 9), "BAD MCC PARAMETER (%d%d%d)!\n", mcc_digit1P, mcc_digit2P, mcc_digit3P);
  AssertFatal ((mnc_digit2P >= 0) && (mnc_digit2P <= 9)
               && (mnc_digit1P >= 0) && (mnc_digit1P <= 9), "BAD MNC PARAMETER (%d.%d.%d)!\n", mnc_digit1P, mnc_digit2P, mnc_digit3P);

  while (plmn_index < mme_config.served_tai.nb_tai) {
    if (mme_config.served_tai.plmn_mcc[plmn_index] == mcc) {
      if ((mme_config.served_tai.plmn_mnc[plmn_index] == mnc2) && (mme_config.served_tai.plmn_mnc_len[plmn_index] == 2)) {
        return 2;
      } else if ((mme_config.served_tai.plmn_mnc[plmn_index] == mnc3) && (mme_config.served_tai.plmn_mnc_len[plmn_index] == 3)) {
        return 3;
      }
    }
    plmn_index += 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
static
int mme_app_compare_plmn (
  const plmn_t * const plmn)
{
  int                                     i = 0;
  uint16_t                                mcc = 0;
  uint16_t                                mnc = 0;
  uint16_t                                mnc_len = 0;

  DevAssert (plmn != NULL);
  /** Get the integer values from the PLMN. */
  PLMN_T_TO_MCC_MNC ((*plmn), mcc, mnc, mnc_len);

  mme_config_read_lock (&mme_config);

  for (i = 0; i < mme_config.served_tai.nb_tai; i++) {
    OAILOG_TRACE (LOG_MME_APP, "Comparing plmn_mcc %d/%d, plmn_mnc %d/%d plmn_mnc_len %d/%d\n",
        mme_config.served_tai.plmn_mcc[i], mcc, mme_config.served_tai.plmn_mnc[i], mnc, mme_config.served_tai.plmn_mnc_len[i], mnc_len);

    if ((mme_config.served_tai.plmn_mcc[i] == mcc) &&
        (mme_config.served_tai.plmn_mnc[i] == mnc) &&
        (mme_config.served_tai.plmn_mnc_len[i] == mnc_len))
      /*
       * There is a matching plmn
       */
      return TA_LIST_AT_LEAST_ONE_MATCH;
  }

  mme_config_unlock (&mme_config);
  return TA_LIST_NO_MATCH;
}

//------------------------------------------------------------------------------
/* @brief compare a TAC
*/
static
int mme_app_compare_tac (
  uint16_t tac_value)
{
  int                                     i = 0;

  mme_config_read_lock (&mme_config);

  for (i = 0; i < mme_config.served_tai.nb_tai; i++) {
    OAILOG_TRACE (LOG_MME_APP, "Comparing config tac %d, received tac = %d\n", mme_config.served_tai.tac[i], tac_value);

    if (mme_config.served_tai.tac[i] == tac_value)
      return TA_LIST_AT_LEAST_ONE_MATCH;
  }

  mme_config_unlock (&mme_config);
  return TA_LIST_NO_MATCH;
}

//------------------------------------------------------------------------------
bool mme_app_check_ta_local(const plmn_t * target_plmn, const tac_t target_tac){
  if(TA_LIST_AT_LEAST_ONE_MATCH == mme_app_compare_plmn(target_plmn)){
    if(TA_LIST_AT_LEAST_ONE_MATCH == mme_app_compare_tac(target_tac)){
      OAILOG_DEBUG (LOG_MME_APP, "TAC and PLMN are matching. \n");
      return true;
    }
  }
  OAILOG_DEBUG (LOG_MME_APP, "TAC or PLMN are not matching. \n");
  return false;
}

//------------------------------------------------------------------------------
static void mme_config_init (mme_config_t * config_pP)
{
  memset(&mme_config, 0, sizeof(mme_config));
  pthread_rwlock_init (&config_pP->rw_lock, NULL);
  config_pP->log_config.output             = NULL;
  config_pP->log_config.is_output_thread_safe = false;
  config_pP->log_config.color              = false;
  config_pP->log_config.udp_log_level      = MAX_LOG_LEVEL; // Means invalid
  config_pP->log_config.gtpv1u_log_level   = MAX_LOG_LEVEL; // will not overwrite existing log levels if MME and S-GW bundled in same executable
  config_pP->log_config.gtpv2c_log_level   = MAX_LOG_LEVEL;
  config_pP->log_config.sctp_log_level     = MAX_LOG_LEVEL;

  config_pP->log_config.nas_log_level      = MAX_LOG_LEVEL;
  config_pP->log_config.mme_app_log_level  = MAX_LOG_LEVEL;
  config_pP->log_config.spgw_app_log_level = MAX_LOG_LEVEL;
  config_pP->log_config.s11_log_level      = MAX_LOG_LEVEL;
  config_pP->log_config.s10_log_level      = MAX_LOG_LEVEL;
  config_pP->log_config.s6a_log_level      = MAX_LOG_LEVEL;
  config_pP->log_config.secu_log_level     = MAX_LOG_LEVEL;
  config_pP->log_config.util_log_level     = MAX_LOG_LEVEL;
  config_pP->log_config.s1ap_log_level     = MAX_LOG_LEVEL;

  config_pP->log_config.msc_log_level      = MAX_LOG_LEVEL;
  config_pP->log_config.xml_log_level      = MAX_LOG_LEVEL;
  config_pP->log_config.mme_scenario_player_log_level = MAX_LOG_LEVEL;
  config_pP->log_config.itti_log_level     = MAX_LOG_LEVEL;

  config_pP->log_config.asn1_verbosity_level = 0;
  config_pP->config_file = NULL;
  config_pP->max_s1_enbs 		= 2;
  config_pP->max_ues     		= 2;
  config_pP->unauthenticated_imsi_supported = 0;
  config_pP->dummy_handover_forwarding_enabled = 1;
  config_pP->run_mode    = RUN_MODE_BASIC;

  /*
   * EPS network feature support
   */
  config_pP->eps_network_feature_support.emergency_bearer_services_in_s1_mode = 0;
  config_pP->eps_network_feature_support.extended_service_request = 0;
  config_pP->eps_network_feature_support.ims_voice_over_ps_session_in_s1 = 0;
  config_pP->eps_network_feature_support.location_services_via_epc = 0;

  config_pP->s1ap_config.port_number = S1AP_PORT_NUMBER;
  /*
   * IP configuration
   */
  config_pP->ip.if_name_s1_mme = NULL;
  config_pP->ip.s1_mme_v4.s_addr = INADDR_ANY;
  config_pP->ip.s1_mme_v6 = in6addr_any;
  config_pP->ip.if_name_s11 = NULL;
  config_pP->ip.s11_mme_v4.s_addr = INADDR_ANY;
  config_pP->ip.s11_mme_v6 = in6addr_any;
  config_pP->ip.port_s11 = 2123;
  config_pP->ip.s10_mme_v4.s_addr = INADDR_ANY;
  config_pP->ip.s10_mme_v6 = in6addr_any;
  config_pP->ip.port_s10 = 2123;
  config_pP->ip.s10_mme_v4.s_addr = INADDR_ANY;
  config_pP->ip.s10_mme_v6 = in6addr_any;
  config_pP->ip.port_s10 = 2123;

  config_pP->s6a_config.conf_file = bfromcstr(S6A_CONF_FILE);
  config_pP->itti_config.queue_size = ITTI_QUEUE_MAX_ELEMENTS;
  config_pP->itti_config.log_file = NULL;
  config_pP->sctp_config.in_streams = SCTP_IN_STREAMS;
  config_pP->sctp_config.out_streams = SCTP_OUT_STREAMS;
  config_pP->relative_capacity = RELATIVE_CAPACITY;
  config_pP->mme_statistic_timer = MME_STATISTIC_TIMER_S;

  // todo: sgw address?
//  config_pP->ipv4.sgw_s11 = 0;

  /** Add the timers for handover/idle-TAU completion on both sides. */
  config_pP->mme_mobility_completion_timer = MME_MOBILITY_COMPLETION_TIMER_S;
  config_pP->mme_s10_handover_completion_timer = MME_S10_HANDOVER_COMPLETION_TIMER_S;

  config_pP->gummei.nb = 1;
  config_pP->gummei.gummei[0].mme_code = MMEC;
  config_pP->gummei.gummei[0].mme_gid = MMEGID;
  config_pP->gummei.gummei[0].plmn.mcc_digit1 = 0;
  config_pP->gummei.gummei[0].plmn.mcc_digit2 = 0;
  config_pP->gummei.gummei[0].plmn.mcc_digit3 = 1;
  config_pP->gummei.gummei[0].plmn.mcc_digit1 = 0;
  config_pP->gummei.gummei[0].plmn.mcc_digit2 = 1;
  config_pP->gummei.gummei[0].plmn.mcc_digit3 = 0x0F;

  config_pP->nas_config.t3346_sec = T3346_DEFAULT_VALUE;
  config_pP->nas_config.t3402_min = T3402_DEFAULT_VALUE;
  config_pP->nas_config.t3412_min = T3412_DEFAULT_VALUE;
  config_pP->nas_config.t3422_sec = T3422_DEFAULT_VALUE;
  config_pP->nas_config.t3450_sec = T3450_DEFAULT_VALUE;
  config_pP->nas_config.t3460_sec = T3460_DEFAULT_VALUE;
  config_pP->nas_config.t3470_sec = T3470_DEFAULT_VALUE;
  config_pP->nas_config.t3485_sec = T3485_DEFAULT_VALUE;
  config_pP->nas_config.t3486_sec = T3486_DEFAULT_VALUE;
  config_pP->nas_config.t3489_sec = T3489_DEFAULT_VALUE;
  config_pP->nas_config.t3495_sec = T3495_DEFAULT_VALUE;
  config_pP->nas_config.force_tau = MME_FORCE_TAU_S;
  config_pP->nas_config.force_reject_sr  = true;
  config_pP->nas_config.disable_esm_information = false;

  /*
   * Set the TAI
   */
  config_pP->served_tai.nb_tai = 1;
  config_pP->served_tai.plmn_mcc = calloc (1, sizeof (*config_pP->served_tai.plmn_mcc));
  config_pP->served_tai.plmn_mnc = calloc (1, sizeof (*config_pP->served_tai.plmn_mnc));
  config_pP->served_tai.plmn_mnc_len = calloc (1, sizeof (*config_pP->served_tai.plmn_mnc_len));
  config_pP->served_tai.tac = calloc (1, sizeof (*config_pP->served_tai.tac));
  config_pP->served_tai.plmn_mcc[0] = PLMN_MCC;
  config_pP->served_tai.plmn_mnc[0] = PLMN_MNC;
  config_pP->served_tai.plmn_mnc_len[0] = PLMN_MNC_LEN;
  config_pP->served_tai.tac[0] = PLMN_TAC;
  config_pP->s1ap_config.outcome_drop_timer_sec = S1AP_OUTCOME_TIMER_DEFAULT;
}

//------------------------------------------------------------------------------
void mme_config_exit (void)
{
  pthread_rwlock_destroy (&mme_config.rw_lock);
  bdestroy_wrapper(&mme_config.log_config.output);
  bdestroy_wrapper(&mme_config.realm);
  bdestroy_wrapper(&mme_config.config_file);

  /*
   * IP configuration
   */
  bdestroy_wrapper(&mme_config.ip.if_name_s1_mme);
  bdestroy_wrapper(&mme_config.ip.if_name_s11);
  bdestroy_wrapper(&mme_config.ip.if_name_s10);
  bdestroy_wrapper(&mme_config.s6a_config.conf_file);
  bdestroy_wrapper(&mme_config.s6a_config.hss_host_name);
  bdestroy_wrapper(&mme_config.itti_config.log_file);

  free_wrapper((void**)&mme_config.served_tai.plmn_mcc);
  free_wrapper((void**)&mme_config.served_tai.plmn_mnc);
  free_wrapper((void**)&mme_config.served_tai.plmn_mnc_len);
  free_wrapper((void**)&mme_config.served_tai.tac);

  for (int i = 0; i < mme_config.e_dns_emulation.nb_service_entries; i++) {
    bdestroy_wrapper(&mme_config.e_dns_emulation.service_id[i]);
  }

#if TRACE_XML
  bdestroy_wrapper(&mme_config.scenario_player_config.scenario_file);
#endif
}
//------------------------------------------------------------------------------
static int mme_config_parse_file (mme_config_t * config_pP)
{
  config_t                                cfg = {0};
  config_setting_t                       *setting_mme = NULL;
  config_setting_t                       *setting = NULL;
  config_setting_t                       *subsetting = NULL;
  config_setting_t                       *sub2setting = NULL;
  int                                     aint = 0;
  int                                     adouble  = 0;
  int                                     aint_s10 = 0;
  int                                     aint_s11 = 0;
  int                                     aint_mc = 0;

  int                                     i = 0,n = 0,
                                          stop_index = 0,
                                          num = 0;
  const char                             *astring = NULL;
  const char                             *tac = NULL;
  const char                             *mcc = NULL;
  const char                             *mnc = NULL;
  char                                   *if_name_s1_mme = NULL;
  char                                   *s1_mme_v4 = NULL;
  char                                   *s1_mme_v6 = NULL;
  char                                   *if_name_s11 = NULL;
  char                                   *s11_mme_v4 = NULL;
  char                                   *s11_mme_v6 = NULL;
  char                                   *sgw_ip_address_for_s11 = NULL;
  char                                   *mme_ip_address_for_s10 = NULL;

  char                                   *if_name_s10 = NULL;
  char                                   *s10_mme_v4 = NULL;
  char                                   *s10_mme_v6 = NULL;
  char                                   *ngh_s10 = NULL;

  char                                   *if_name_mc = NULL;
  char                                   *mc_mme_v4 = NULL;
  char                                   *mc_mme_v6 = NULL;

  bool                                    swap = false;
  bstring                                 address = NULL;
  bstring                                 cidr = NULL;
  bstring                                 mask = NULL;
  struct in_addr                          in_addr_var = {0};
  struct in6_addr                         in6_addr_var = {0};

  config_init (&cfg);

  if (config_pP->config_file != NULL) {
    /*
     * Read the file. If there is an error, report it and exit.
     */
    if (!config_read_file (&cfg, bdata(config_pP->config_file))) {
      OAILOG_ERROR (LOG_CONFIG, ": %s:%d - %s\n", bdata(config_pP->config_file), config_error_line (&cfg), config_error_text (&cfg));
      config_destroy (&cfg);
      AssertFatal (1 == 0, "Failed to parse MME configuration file %s!\n", bdata(config_pP->config_file));
    }
  } else {
    OAILOG_ERROR (LOG_CONFIG, " No MME configuration file provided!\n");
    config_destroy (&cfg);
    AssertFatal (0, "No MME configuration file provided!\n");
  }

  setting_mme = config_lookup (&cfg, MME_CONFIG_STRING_MME_CONFIG);

  if (setting_mme != NULL) {

    // LOGGING setting
    setting = config_setting_get_member (setting_mme, LOG_CONFIG_STRING_LOGGING);
    if (setting != NULL) {
      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_OUTPUT, (const char **)&astring)) {
        if (astring != NULL) {
          if (config_pP->log_config.output) {
            bassigncstr(config_pP->log_config.output , astring);
          } else {
            config_pP->log_config.output = bfromcstr(astring);
          }
        }
      }

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_OUTPUT_THREAD_SAFE, (const char **)&astring)) {
        if (astring != NULL) {
          if (strcasecmp (astring, "yes") == 0) {
            config_pP->log_config.is_output_thread_safe = true;
          } else {
            config_pP->log_config.is_output_thread_safe = false;
          }
        }
      }

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_COLOR, (const char **)&astring)) {
        if (0 == strcasecmp("yes", astring)) config_pP->log_config.color = true;
        else config_pP->log_config.color = false;
      }

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_SCTP_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.sctp_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_S1AP_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.s1ap_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_NAS_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.nas_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_MME_APP_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.mme_app_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_S6A_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.s6a_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_SECU_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.secu_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_GTPV2C_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.gtpv2c_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_UDP_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.udp_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_S11_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.s11_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_S10_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.s10_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_UTIL_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.util_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_MSC_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.msc_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_XML_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.xml_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_MME_SCENARIO_PLAYER_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.mme_scenario_player_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_ITTI_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.itti_log_level = OAILOG_LEVEL_STR2INT (astring);

      if ((config_setting_lookup_string (setting_mme, MME_CONFIG_STRING_ASN1_VERBOSITY, (const char **)&astring))) {
        if (strcasecmp (astring, MME_CONFIG_STRING_ASN1_VERBOSITY_NONE) == 0)
          config_pP->log_config.asn1_verbosity_level = 0;
        else if (strcasecmp (astring, MME_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING) == 0)
          config_pP->log_config.asn1_verbosity_level = 2;
        else if (strcasecmp (astring, MME_CONFIG_STRING_ASN1_VERBOSITY_INFO) == 0)
          config_pP->log_config.asn1_verbosity_level = 1;
        else
          config_pP->log_config.asn1_verbosity_level = 0;
      }
    }

    // GENERAL MME SETTINGS

    // todo:
//    if ((config_setting_lookup_string (setting_mme, MME_CONFIG_STRING_RUN_MODE, (const char **)&astring))) {
//      if (strcasecmp (astring, MME_CONFIG_STRING_RUN_MODE_TEST) == 0)
//        config_pP->run_mode = RUN_MODE_TEST;
//      else
//        config_pP->run_mode = RUN_MODE_OTHER;
//    }
    if ((config_setting_lookup_string (setting_mme, MME_CONFIG_STRING_REALM, (const char **)&astring))) {
      config_pP->realm = bfromcstr (astring);
    }

    if ((config_setting_lookup_string (setting_mme,
                                       MME_CONFIG_STRING_PID_DIRECTORY,
                                       (const char **)&astring))) {
      config_pP->pid_dir = bfromcstr (astring);
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_INSTANCE, &aint))) {
      config_pP->instance = (uint32_t) aint;
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_S1_MAXENB, &aint))) {
      config_pP->max_s1_enbs = (uint32_t) aint;
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_MAXUE, &aint))) {
      config_pP->max_ues = (uint32_t) aint;
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_RELATIVE_CAPACITY, &aint))) {
      config_pP->relative_capacity = (uint8_t) aint;
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_STATISTIC_TIMER, &aint))) {
      config_pP->mme_statistic_timer = (uint32_t) aint;
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_MME_MOBILITY_COMPLETION_TIMER, &aint))) {
      config_pP->mme_mobility_completion_timer = (uint32_t) aint;
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_MME_S10_HANDOVER_COMPLETION_TIMER, &aint))) {
      config_pP->mme_s10_handover_completion_timer = (uint32_t) aint;
    }

    /**
     * Other configurations..
     */

    if ((config_setting_lookup_string (setting_mme, EPS_NETWORK_FEATURE_SUPPORT_EMERGENCY_BEARER_SERVICES_IN_S1_MODE, (const char **)&astring))) {
      if (strcasecmp (astring, "yes") == 0)
        config_pP->eps_network_feature_support.emergency_bearer_services_in_s1_mode = 1;
      else
        config_pP->eps_network_feature_support.emergency_bearer_services_in_s1_mode = 0;
    }
    if ((config_setting_lookup_string (setting_mme, EPS_NETWORK_FEATURE_SUPPORT_EXTENDED_SERVICE_REQUEST, (const char **)&astring))) {
      if (strcasecmp (astring, "yes") == 0)
        config_pP->eps_network_feature_support.extended_service_request = 1;
      else
        config_pP->eps_network_feature_support.extended_service_request = 0;
    }
    if ((config_setting_lookup_string (setting_mme, EPS_NETWORK_FEATURE_SUPPORT_IMS_VOICE_OVER_PS_SESSION_IN_S1, (const char **)&astring))) {
      if (strcasecmp (astring, "yes") == 0)
        config_pP->eps_network_feature_support.ims_voice_over_ps_session_in_s1 = 1;
      else
        config_pP->eps_network_feature_support.ims_voice_over_ps_session_in_s1 = 0;
    }
    if ((config_setting_lookup_string (setting_mme, EPS_NETWORK_FEATURE_SUPPORT_LOCATION_SERVICES_VIA_EPC, (const char **)&astring))) {
      if (strcasecmp (astring, "yes") == 0)
        config_pP->eps_network_feature_support.location_services_via_epc = 1;
      else
        config_pP->eps_network_feature_support.location_services_via_epc = 0;
    }

    if ((config_setting_lookup_string (setting_mme, MME_CONFIG_STRING_UNAUTHENTICATED_IMSI_SUPPORTED, (const char **)&astring))) {
      if (strcasecmp (astring, "yes") == 0)
        config_pP->unauthenticated_imsi_supported = 1;
      else
        config_pP->unauthenticated_imsi_supported = 0;
    }

    if ((config_setting_lookup_string (setting_mme, MME_CONFIG_STRING_DUMMY_HANDOVER_FORWARDING_ENABLED, (const char **)&astring))) {
      if (strcasecmp (astring, "yes") == 0)
        config_pP->dummy_handover_forwarding_enabled = 1;
      else
        config_pP->dummy_handover_forwarding_enabled = 0;
    }

    // ITTI SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_INTERTASK_INTERFACE_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_INTERTASK_INTERFACE_QUEUE_SIZE, &aint))) {
        config_pP->itti_config.queue_size = (uint32_t) aint;
      }
    }
    // S6A SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_S6A_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_string (setting, MME_CONFIG_STRING_S6A_CONF_FILE_PATH, (const char **)&astring))) {
        if (astring != NULL) {
          if (config_pP->s6a_config.conf_file) {
            bassigncstr(config_pP->s6a_config.conf_file , astring);
          } else {
            config_pP->s6a_config.conf_file = bfromcstr(astring);
          }
        }
      }

      if ((config_setting_lookup_string (setting, MME_CONFIG_STRING_S6A_HSS_HOSTNAME, (const char **)&astring))) {
        if (astring != NULL) {
          if (config_pP->s6a_config.hss_host_name) {
            bassigncstr(config_pP->s6a_config.hss_host_name , astring);
          } else {
            config_pP->s6a_config.hss_host_name = bfromcstr(astring);
          }
        } else
          AssertFatal (1 == 0, "You have to provide a valid HSS hostname %s=...\n", MME_CONFIG_STRING_S6A_HSS_HOSTNAME);
      }

      // todo: check if MME hostname is needed
      if ((config_setting_lookup_string (setting, MME_CONFIG_STRING_S6A_MME_HOSTNAME, (const char **)&astring))) {
        if (astring != NULL) {
          if (config_pP->s6a_config.mme_host_name) {
            bassigncstr(config_pP->s6a_config.mme_host_name , astring);
          } else {
            config_pP->s6a_config.mme_host_name = bfromcstr(astring);
          }
        } else
          AssertFatal (1 == 0, "You have to provide a valid MME hostname %s=...\n", MME_CONFIG_STRING_S6A_MME_HOSTNAME);
      }
    }
    // SCTP SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_SCTP_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_SCTP_INSTREAMS, &aint))) {
        config_pP->sctp_config.in_streams = (uint16_t) aint;
      }

      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_SCTP_OUTSTREAMS, &aint))) {
        config_pP->sctp_config.out_streams = (uint16_t) aint;
      }
    }
    // S1AP SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_S1AP_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_S1AP_OUTCOME_TIMER, &aint))) {
        config_pP->s1ap_config.outcome_drop_timer_sec = (uint8_t) aint;
      }

      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_S1AP_PORT, &aint))) {
        config_pP->s1ap_config.port_number = (uint16_t) aint;
      }
    }
    // TAI list setting
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_TAI_LIST);
    if (setting != NULL) {
      num = config_setting_length (setting);

      if (config_pP->served_tai.nb_tai != num) {
        if (config_pP->served_tai.plmn_mcc != NULL)
          free_wrapper ((void**)&config_pP->served_tai.plmn_mcc);

        if (config_pP->served_tai.plmn_mnc != NULL)
          free_wrapper ((void**)&config_pP->served_tai.plmn_mnc);

        if (config_pP->served_tai.plmn_mnc_len != NULL)
          free_wrapper ((void**)&config_pP->served_tai.plmn_mnc_len);

        if (config_pP->served_tai.tac != NULL)
          free_wrapper ((void**)&config_pP->served_tai.tac);

        config_pP->served_tai.plmn_mcc = calloc (num, sizeof (*config_pP->served_tai.plmn_mcc));
        config_pP->served_tai.plmn_mnc = calloc (num, sizeof (*config_pP->served_tai.plmn_mnc));
        config_pP->served_tai.plmn_mnc_len = calloc (num, sizeof (*config_pP->served_tai.plmn_mnc_len));
        config_pP->served_tai.tac = calloc (num, sizeof (*config_pP->served_tai.tac));
      }

      config_pP->served_tai.nb_tai = num;
      AssertFatal(64 >= num , "Too many TAIs configured %d", num);

      for (i = 0; i < num; i++) {
        sub2setting = config_setting_get_elem (setting, i);

        if (sub2setting != NULL) {
          if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_MCC, &mcc))) {
            config_pP->served_tai.plmn_mcc[i] = (uint16_t) atoi (mcc);
          }

          if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_MNC, &mnc))) {
            config_pP->served_tai.plmn_mnc[i] = (uint16_t) atoi (mnc);
            config_pP->served_tai.plmn_mnc_len[i] = strlen (mnc);
            AssertFatal ((config_pP->served_tai.plmn_mnc_len[i] == 2) || (config_pP->served_tai.plmn_mnc_len[i] == 3),
                "Bad MNC length %u, must be 2 or 3", config_pP->served_tai.plmn_mnc_len[i]);
          }

          if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_TAC, &tac))) {
            config_pP->served_tai.tac[i] = (uint16_t) atoi (tac);
            AssertFatal(TAC_IS_VALID(config_pP->served_tai.tac[i]), "Invalid TAC value "TAC_FMT, config_pP->served_tai.tac[i]);
          }
        }
      }
      // sort TAI list
      n = config_pP->served_tai.nb_tai;
      do {
        stop_index = 0;
        for (i = 1; i < n; i++) {
          swap = false;
          if (config_pP->served_tai.plmn_mcc[i-1] > config_pP->served_tai.plmn_mcc[i]) {
            swap = true;
          } else if (config_pP->served_tai.plmn_mcc[i-1] == config_pP->served_tai.plmn_mcc[i]) {
            if (config_pP->served_tai.plmn_mnc[i-1] > config_pP->served_tai.plmn_mnc[i]) {
              swap = true;
            } else  if (config_pP->served_tai.plmn_mnc[i-1] == config_pP->served_tai.plmn_mnc[i]) {
              if (config_pP->served_tai.tac[i-1] > config_pP->served_tai.tac[i]) {
                swap = true;
              }
            }
          }
          if (true == swap) {
            uint16_t swap16;
            swap16 = config_pP->served_tai.plmn_mcc[i-1];
            config_pP->served_tai.plmn_mcc[i-1] = config_pP->served_tai.plmn_mcc[i];
            config_pP->served_tai.plmn_mcc[i]   = swap16;

            swap16 = config_pP->served_tai.plmn_mnc[i-1];
            config_pP->served_tai.plmn_mnc[i-1] = config_pP->served_tai.plmn_mnc[i];
            config_pP->served_tai.plmn_mnc[i]   = swap16;

            swap16 = config_pP->served_tai.tac[i-1];
            config_pP->served_tai.tac[i-1] = config_pP->served_tai.tac[i];
            config_pP->served_tai.tac[i]   = swap16;

            stop_index = i;
          }
        }
        n = stop_index;
      } while (0 != n);

      for (i = 1; i < config_pP->served_tai.nb_tai; i++) {
        if ((config_pP->served_tai.plmn_mcc[i-1] == config_pP->served_tai.plmn_mcc[i]) &&
            (config_pP->served_tai.plmn_mnc[i-1] == config_pP->served_tai.plmn_mnc[i]) &&
            (config_pP->served_tai.tac[i-1] == config_pP->served_tai.tac[i])) {

          for (int j = i+1; j < config_pP->served_tai.nb_tai; j++) {
            config_pP->served_tai.plmn_mcc[j-1] = config_pP->served_tai.plmn_mcc[j];
            config_pP->served_tai.plmn_mnc[j-1] = config_pP->served_tai.plmn_mnc[j];
            config_pP->served_tai.tac[j-1] = config_pP->served_tai.tac[j];
          }
          config_pP->served_tai.plmn_mcc[config_pP->served_tai.nb_tai-1] = 0;
          config_pP->served_tai.plmn_mnc[config_pP->served_tai.nb_tai-1] = 0;
          config_pP->served_tai.tac[config_pP->served_tai.nb_tai-1] = 0;
          config_pP->served_tai.nb_tai--;
          i--; //tricky
        }
      }
      // helper for determination of list type (global view), we could make sublists with different types, but keep things simple for now
      config_pP->served_tai.list_type = TRACKING_AREA_IDENTITY_LIST_TYPE_ONE_PLMN_CONSECUTIVE_TACS;
      for (i = 1; i < config_pP->served_tai.nb_tai; i++) {
        if ((config_pP->served_tai.plmn_mcc[i] != config_pP->served_tai.plmn_mcc[0]) ||
            (config_pP->served_tai.plmn_mnc[i] != config_pP->served_tai.plmn_mnc[0])){
          config_pP->served_tai.list_type = TRACKING_AREA_IDENTITY_LIST_TYPE_MANY_PLMNS;
          break;
        } else if ((config_pP->served_tai.plmn_mcc[i] != config_pP->served_tai.plmn_mcc[i-1]) ||
                   (config_pP->served_tai.plmn_mnc[i] != config_pP->served_tai.plmn_mnc[i-1])) {
          config_pP->served_tai.list_type = TRACKING_AREA_IDENTITY_LIST_TYPE_MANY_PLMNS;
          break;
        }
        if (config_pP->served_tai.tac[i] != (config_pP->served_tai.tac[i-1] + 1)) {
          config_pP->served_tai.list_type = TRACKING_AREA_IDENTITY_LIST_TYPE_ONE_PLMN_NON_CONSECUTIVE_TACS;
        }
      }
    }

    // GUMMEI SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_GUMMEI_LIST);
    config_pP->gummei.nb = 0;
    if (setting != NULL) {
      num = config_setting_length (setting);
      AssertFatal(num == 1, "Only one GUMMEI supported for this version of MME");
      for (i = 0; i < num; i++) {
        sub2setting = config_setting_get_elem (setting, i);

        if (sub2setting != NULL) {
          if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_MCC, &mcc))) {
            AssertFatal( 3 == strlen(mcc), "Bad MCC length, it must be 3 digit ex: 001");
            char c[2] = { mcc[0], 0};
            config_pP->gummei.gummei[i].plmn.mcc_digit1 = (uint8_t) atoi (c);
            c[0] = mcc[1];
            config_pP->gummei.gummei[i].plmn.mcc_digit2 = (uint8_t) atoi (c);
            c[0] = mcc[2];
            config_pP->gummei.gummei[i].plmn.mcc_digit3 = (uint8_t) atoi (c);
          }

          if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_MNC, &mnc))) {
            AssertFatal( (3 == strlen(mnc)) || (2 == strlen(mnc)) , "Bad MNC length, it must be 3 digit ex: 001");
            char c[2] = { mnc[0], 0};
            config_pP->gummei.gummei[i].plmn.mnc_digit1 = (uint8_t) atoi (c);
            c[0] = mnc[1];
            config_pP->gummei.gummei[i].plmn.mnc_digit2 = (uint8_t) atoi (c);
            if (3 == strlen(mnc)) {
              c[0] = mnc[2];
              config_pP->gummei.gummei[i].plmn.mnc_digit3 = (uint8_t) atoi (c);
            } else {
              config_pP->gummei.gummei[i].plmn.mnc_digit3 = 0x0F;
            }
          }

          if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_MME_GID, &mnc))) {
            config_pP->gummei.gummei[i].mme_gid = (uint16_t) atoi (mnc);
          }
          if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_MME_CODE, &mnc))) {
            config_pP->gummei.gummei[i].mme_code = (uint8_t) atoi (mnc);
          }
          config_pP->gummei.nb += 1;
        }
      }
    }

    // NETWORK INTERFACE SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);

    config_setting_lookup_string (setting, MME_CONFIG_STRING_IPV4_ADDRESS_FOR_S1_MME, (const char **)&s1_mme_v4);
	config_setting_lookup_string (setting, MME_CONFIG_STRING_IPV6_ADDRESS_FOR_S1_MME, (const char **)&s1_mme_v6);
	config_setting_lookup_string (setting, MME_CONFIG_STRING_IPV4_ADDRESS_FOR_S11, (const char **)&s11_mme_v4);
	config_setting_lookup_string (setting, MME_CONFIG_STRING_IPV6_ADDRESS_FOR_S11, (const char **)&s11_mme_v6);
	config_setting_lookup_string (setting, MME_CONFIG_STRING_IPV4_ADDRESS_FOR_S10, (const char **)&s10_mme_v4);
	config_setting_lookup_string (setting, MME_CONFIG_STRING_IPV6_ADDRESS_FOR_S10, (const char **)&s10_mme_v6);

    if (setting != NULL) {
    	/** Process Mandatory Interfaces. */
        if (
        	(
        		/** S1. */
        		config_setting_lookup_string (setting, MME_CONFIG_STRING_INTERFACE_NAME_FOR_S1_MME, (const char **)&if_name_s1_mme) && (s1_mme_v4 || s1_mme_v6)
				/** S11. */
				&& config_setting_lookup_string (setting, MME_CONFIG_STRING_INTERFACE_NAME_FOR_S11, (const char **)&if_name_s11) && (s11_mme_v4 || s11_mme_v6)
				&& config_setting_lookup_int (setting, MME_CONFIG_STRING_MME_PORT_FOR_S11, &aint_s11)
            )
          ) {
        	config_pP->ip.port_s11 = (uint16_t)aint_s11;
        	struct bstrList *list = NULL;

        	/** S1: IPv4. */
        	config_pP->ip.if_name_s1_mme = bfromcstr(if_name_s1_mme);
        	if(s1_mme_v4) {
        		cidr = bfromcstr (s1_mme_v4);
        		list = bsplit (cidr, '/');
        		AssertFatal(2 == list->qty, "Bad CIDR address %s", bdata(cidr));
        		address = list->entry[0];
        		mask    = list->entry[1];
        		IPV4_STR_ADDR_TO_INADDR (bdata(address), config_pP->ip.s1_mme_v4, "BAD IP ADDRESS FORMAT FOR S1-MME !\n");
        		config_pP->ip.s1_mme_cidrv4 = atoi ((const char*)mask->data);
        		bstrListDestroy(list);
        		in_addr_var.s_addr = config_pP->ip.s1_mme_v4.s_addr;
        		OAILOG_INFO (LOG_MME_APP, "Parsing configuration file found S1-MME: %s/%d on %s\n",
        				inet_ntoa (in_addr_var), config_pP->ip.s1_mme_cidrv4, bdata(config_pP->ip.if_name_s1_mme));
        		bdestroy_wrapper(&cidr);
        	}
        	/** S1: IPv6. */
        	if(s1_mme_v6) {
        		cidr = bfromcstr (s1_mme_v6);
        		list = bsplit (cidr, '/');
        		AssertFatal(2 == list->qty, "Bad CIDR address %s", bdata(cidr));
        		address = list->entry[0];
        		mask    = list->entry[1];
        		IPV6_STR_ADDR_TO_INADDR (bdata(address), config_pP->ip.s1_mme_v6, "BAD IPV6 ADDRESS FORMAT FOR S1-MME !\n");
        		//    	config_pP->ipv6.netmask_s1_mme = atoi ((const char*)mask->data);
        		bstrListDestroy(list);
        		//    	memcpy(in6_addr_var.s6_addr, config_pP->ipv6.s1_mme.s6_addr, 16);

        		char strv6[16];
        		OAILOG_INFO (LOG_MME_APP, "Parsing configuration file found S1-MME: %s/%d on %s\n",
        				inet_ntop(AF_INET6, &in6_addr_var, strv6, 16), config_pP->ip.s1_mme_cidrv6, bdata(config_pP->ip.if_name_s1_mme));
        		bdestroy_wrapper(&cidr);
        	}
        	/** S11: IPv4. */
        	config_pP->ip.if_name_s11 = bfromcstr(if_name_s11);
        	if(s11_mme_v4) {
        		cidr = bfromcstr (s11_mme_v4);
        		list = bsplit (cidr, '/');
        		AssertFatal(2 == list->qty, "Bad CIDR address %s", bdata(cidr));
        		address = list->entry[0];
        		mask    = list->entry[1];
            IPV4_STR_ADDR_TO_INADDR (bdata(address), config_pP->ip.s11_mme_v4, "BAD IP ADDRESS FORMAT FOR S11 !\n");
            config_pP->ip.s11_mme_cidrv4 = atoi ((const char*)mask->data);
            bstrListDestroy(list);
            bdestroy_wrapper(&cidr);
            in_addr_var.s_addr = config_pP->ip.s11_mme_v4.s_addr;
            OAILOG_INFO (LOG_MME_APP, "Parsing configuration file found S11: %s/%d on %s\n",
            		inet_ntoa (in_addr_var), config_pP->ip.s11_mme_cidrv4, bdata(config_pP->ip.if_name_s11));
        }
        /** S11: IPv6. */
        if(s11_mme_v6) {
      	  config_pP->ip.port_s11 = (uint16_t)aint_s11;
      	  cidr = bfromcstr (s11_mme_v6);
      	  list = bsplit (cidr, '/');
      	  AssertFatal(2 == list->qty, "Bad CIDR address %s", bdata(cidr));
      	  address = list->entry[0];
      	  mask    = list->entry[1];
      	  IPV6_STR_ADDR_TO_INADDR (bdata(address), config_pP->ip.s11_mme_v6, "BAD IPV6 ADDRESS FORMAT FOR S11-MME !\n");
      	  //    	config_pP->ipv6.netmask_s1_mme = atoi ((const char*)mask->data);
      	  bstrListDestroy(list);
  //    	  memcpy(in6_addr_var.s6_addr, config_pP->ipv6.s1_mme.s6_addr, 16);
      	  char strv6[16];
      	  OAILOG_INFO (LOG_MME_APP, "Parsing configuration file found S11-MME: %s/%d on %s\n",
      			  inet_ntop(AF_INET6, &in6_addr_var, strv6, 16), config_pP->ip.s11_mme_cidrv6, bdata(config_pP->ip.if_name_s11));
      	  bdestroy_wrapper(&cidr);
  //      	config_pP->ipv6.netmask_s11 = atoi ((const char*)mask->data);
        }
      }

      /** Process Optional Interfaces. */
      /** S10. */
      if (
          (
        	/** S10. */
        	config_setting_lookup_string (setting, MME_CONFIG_STRING_INTERFACE_NAME_FOR_S10, (const char **)&if_name_s10) && (s10_mme_v4 || s10_mme_v6)
			&& config_setting_lookup_int (setting, MME_CONFIG_STRING_MME_PORT_FOR_S10, &aint_s10)
          )
      ) {
      	config_pP->ip.port_s10 = (uint16_t)aint_s10;
    	  struct bstrList *list = NULL;

    	  /** S10: IPv4. */
          config_pP->ip.if_name_s10 = bfromcstr(if_name_s10);
          if(s10_mme_v4) {
              cidr = bfromcstr (s10_mme_v4);
              list = bsplit (cidr, '/');
              AssertFatal(2 == list->qty, "Bad CIDR address %s", bdata(cidr));
              address = list->entry[0];
              mask    = list->entry[1];
              IPV4_STR_ADDR_TO_INADDR (bdata(address), config_pP->ip.s10_mme_v4, "BAD IP ADDRESS FORMAT FOR S10 !\n");
              config_pP->ip.s10_mme_cidrv4 = atoi ((const char*)mask->data);
              bstrListDestroy(list);
              bdestroy_wrapper(&cidr);
              in_addr_var.s_addr = config_pP->ip.s10_mme_v4.s_addr;
              OAILOG_INFO (LOG_MME_APP, "Parsing configuration file found S10: %s/%d on %s\n",
                             inet_ntoa (in_addr_var), config_pP->ip.s10_mme_cidrv4, bdata(config_pP->ip.if_name_s10));
          }
          /** S10: IPv6. */
          if(s10_mme_v6){
          	/** S10. */
          	config_pP->ip.port_s10 = (uint16_t)aint_s10;
          	cidr = bfromcstr (s10_mme_v6);
          	list = bsplit (cidr, '/');
          	AssertFatal(2 == list->qty, "Bad CIDR address %s", bdata(cidr));
          	address = list->entry[0];
          	mask    = list->entry[1];
          	IPV6_STR_ADDR_TO_INADDR (bdata(address), config_pP->ip.s10_mme_v6, "BAD IPV6 ADDRESS FORMAT FOR S10-MME !\n");
          	//    	config_pP->ipv6.netmask_s1_mme = atoi ((const char*)mask->data);
          	bstrListDestroy(list);
          	//    	  memcpy(in6_addr_var.s6_addr, config_pP->ipv6.s1_mme.s6_addr, 16);
          	char strv6[16];
          	OAILOG_INFO (LOG_MME_APP, "Parsing configuration file found S10-MME: %s/%d on %s\n",
          			inet_ntop(AF_INET6, &in6_addr_var, strv6, 16), config_pP->ip.s10_mme_cidrv6, bdata(config_pP->ip.if_name_s10));
          	bdestroy_wrapper(&cidr);
          	//      	config_pP->ipv6.netmask_s11 = atoi ((const char*)mask->data);
          }
      }
    }
    // NAS SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_NAS_CONFIG);

    if (setting != NULL) {
      subsetting = config_setting_get_member (setting, MME_CONFIG_STRING_NAS_SUPPORTED_INTEGRITY_ALGORITHM_LIST);

      if (subsetting != NULL) {
        num = config_setting_length (subsetting);

        if (num <= 8) {
          for (i = 0; i < num; i++) {
            astring = config_setting_get_string_elem (subsetting, i);

            if (strcmp ("EIA0", astring) == 0)
              config_pP->nas_config.prefered_integrity_algorithm[i] = EIA0_ALG_ID;
            else if (strcmp ("EIA1", astring) == 0)
              config_pP->nas_config.prefered_integrity_algorithm[i] = EIA1_128_ALG_ID;
            else if (strcmp ("EIA2", astring) == 0)
              config_pP->nas_config.prefered_integrity_algorithm[i] = EIA2_128_ALG_ID;
            else
              config_pP->nas_config.prefered_integrity_algorithm[i] = EIA0_ALG_ID;
          }

          for (i = num; i < 8; i++) {
            config_pP->nas_config.prefered_integrity_algorithm[i] = EIA0_ALG_ID;
          }
        }
      }

      subsetting = config_setting_get_member (setting, MME_CONFIG_STRING_NAS_SUPPORTED_CIPHERING_ALGORITHM_LIST);

      if (subsetting != NULL) {
        num = config_setting_length (subsetting);

        if (num <= 8) {
          for (i = 0; i < num; i++) {
            astring = config_setting_get_string_elem (subsetting, i);

            if (strcmp ("EEA0", astring) == 0)
              config_pP->nas_config.prefered_ciphering_algorithm[i] = EEA0_ALG_ID;
            else if (strcmp ("EEA1", astring) == 0)
              config_pP->nas_config.prefered_ciphering_algorithm[i] = EEA1_128_ALG_ID;
            else if (strcmp ("EEA2", astring) == 0)
              config_pP->nas_config.prefered_ciphering_algorithm[i] = EEA2_128_ALG_ID;
            else
              config_pP->nas_config.prefered_ciphering_algorithm[i] = EEA0_ALG_ID;
          }

          for (i = num; i < 8; i++) {
            config_pP->nas_config.prefered_ciphering_algorithm[i] = EEA0_ALG_ID;
          }
        }
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3346_TIMER, &aint))) {
        config_pP->nas_config.t3346_sec = (uint8_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3402_TIMER, &aint))) {
        config_pP->nas_config.t3402_min = (uint32_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3412_TIMER, &aint))) {
        config_pP->nas_config.t3412_min = (uint32_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3422_TIMER, &aint))) {
        config_pP->nas_config.t3422_sec = (uint32_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3450_TIMER, &aint))) {
        config_pP->nas_config.t3450_sec = (uint32_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3460_TIMER, &aint))) {
        config_pP->nas_config.t3460_sec = (uint32_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3470_TIMER, &aint))) {
        config_pP->nas_config.t3470_sec = (uint32_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3485_TIMER, &aint))) {
        config_pP->nas_config.t3485_sec = (uint32_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3486_TIMER, &aint))) {
        config_pP->nas_config.t3486_sec = (uint32_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3489_TIMER, &aint))) {
        config_pP->nas_config.t3489_sec = (uint32_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3495_TIMER, &aint))) {
        config_pP->nas_config.t3495_sec = (uint32_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_FORCE_TAU, &aint))) {
    	  config_pP->nas_config.force_tau = ((uint32_t) aint);
      }
//      if ((config_setting_lookup_string (setting, MME_CONFIG_STRING_NAS_FORCE_REJECT_SR, (const char **)&astring))) {
//        if (strcasecmp (astring, "yes") == 0)
//          config_pP->nas_config.force_reject_sr = true;
//        else
//          config_pP->nas_config.force_reject_sr = false;
//      }
      if ((config_setting_lookup_string (setting, MME_CONFIG_STRING_NAS_DISABLE_ESM_INFORMATION_PROCEDURE, (const char **)&astring))) {
        if (strcasecmp (astring, "yes") == 0)
          config_pP->nas_config.disable_esm_information = true;
        else
          config_pP->nas_config.disable_esm_information = false;
      }
    }
  }

  // todo: selection instead of config!
  setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_WRR_LIST_SELECTION);
  if (setting != NULL) {
    num = config_setting_length (setting);

    AssertFatal(num <= MME_CONFIG_MAX_SERVICE, "Too many service entries defined (%d>%d)", num, MME_CONFIG_MAX_SERVICE);

    config_pP->e_dns_emulation.nb_service_entries = 0;
    for (i = 0; i < num; i++) {
      sub2setting = config_setting_get_elem (setting, i);

      if (sub2setting != NULL) {
        const char                             *id = NULL;
        if (!(config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_ID, &id))) {
          OAILOG_ERROR (LOG_SPGW_APP, "Could not get service ID item %d in %s\n", i, MME_CONFIG_STRING_WRR_LIST_SELECTION);
          break;
        }
        config_pP->e_dns_emulation.service_id[i] = bfromcstr(id);

        /** Check S11 Endpoint (service="x-3gpp-sgw:x-s11"). */
        if ((config_setting_lookup_string (sub2setting, SGW_CONFIG_STRING_SGW_IP_ADDRESS_FOR_S11, (const char **)&sgw_ip_address_for_s11)
            )
          )
        {
        	address = bfromcstr (sgw_ip_address_for_s11);
          struct addrinfo hints, *result, *rp;
          int s;
          memset(&hints, 0, sizeof(struct addrinfo));
          hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
          hints.ai_socktype = SOCK_DGRAM; /* Stream socket */
          hints.ai_flags = 0;
          hints.ai_protocol = IPPROTO_UDP;          /* TCP protocol */
          s = getaddrinfo(bdata(address), NULL, &hints, &result);
          if (s != 0) {
             return 0;
          }
          for (rp = result; rp != NULL; rp = rp->ai_next) {
        	  /** Check the type. */
        	  AssertFatal(!rp->ai_next, "A single IPv4 address should be per neighboring SGW node.");
        	  if(rp->ai_family == AF_INET){
        		  if (INADDR_ANY == ((struct sockaddr_in*)rp->ai_addr)->sin_addr.s_addr) {
        			  continue;
        		  }
        		  OAILOG_INFO (LOG_MME_APP, "Parsing configuration file found S-GW S11: %s\n", inet_ntoa (((struct sockaddr_in*)rp->ai_addr)->sin_addr));
            	  memcpy((struct sockaddr_in*)&config_pP->e_dns_emulation.sockaddr[i], (struct sockaddr_in*)rp->ai_addr, rp->ai_addrlen);
        	  } else if (rp->ai_family == AF_INET6) {
        		  if(memcmp(&((struct sockaddr_in6*)rp->ai_addr)->sin6_addr, (void*)&in6addr_any, rp->ai_addrlen) == 0) {
        			  // Do not halt the config process
        			  continue;
        		  }
        		  char strv6[16];
        		  OAILOG_INFO (LOG_MME_APP, "Parsing configuration file found S11-SGW: %s. \n", inet_ntop(AF_INET6, &((struct sockaddr_in6*)rp->ai_addr)->sin6_addr, strv6, 16));
            	  memcpy((struct sockaddr_in6*)&config_pP->e_dns_emulation.sockaddr[i], (struct sockaddr_in6*)rp->ai_addr, rp->ai_addrlen);
        	  }
        	  config_pP->e_dns_emulation.interface_type[i] = S11_SGW_GTP_C;
        	  ((struct sockaddr*)&config_pP->e_dns_emulation.sockaddr[i])->sa_family = rp->ai_family;
          }
          freeaddrinfo(result);
          bdestroy_wrapper(&address);

        }

        /** Check S10 Endpoint (service="x-3gpp-mme:x-s10"). */
        if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_PEER_MME_IP_ADDRESS_FOR_S10, (const char **)&mme_ip_address_for_s10)
          )
        )
        {
        	address = bfromcstr (mme_ip_address_for_s10);
        	struct addrinfo hints, *result, *rp;
        	int s;
        	memset(&hints, 0, sizeof(struct addrinfo));
        	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        	hints.ai_socktype = SOCK_DGRAM; /* Stream socket */
        	hints.ai_flags = 0;
        	hints.ai_protocol = IPPROTO_UDP;          /* TCP protocol */
        	s = getaddrinfo(bdata(address), NULL, &hints, &result);
        	if (s != 0) {
        		return 0;
        	}
        	for (rp = result; rp != NULL; rp = rp->ai_next) {
        		/** Check the type. */
        		AssertFatal(!rp->ai_next, "A single IPv4 address should be per neighboring MME node.");
        		if(rp->ai_family == AF_INET){
        			if (INADDR_ANY == ((struct sockaddr_in*)rp)->sin_addr.s_addr) {
        				continue;
        			}
        			OAILOG_INFO (LOG_MME_APP, "Parsing configuration file found MME S10: %s\n", inet_ntoa (((struct sockaddr_in*)rp)->sin_addr));
        			memcpy((struct sockaddr_in*)&config_pP->e_dns_emulation.sockaddr[i], (struct sockaddr_in*)rp->ai_addr, rp->ai_addrlen);
        		} else if (rp->ai_family == AF_INET6) {
        			if(memcmp(&((struct sockaddr_in6*)rp)->sin6_addr, (void*)&in6addr_any, rp->ai_addrlen) == 0) {
        				// Do not halt the config process
        				continue;
        			}
        			char strv6[16];
        			OAILOG_INFO (LOG_MME_APP, "Parsing configuration file found S10-MME: %s. \n", inet_ntop(AF_INET6, &((struct sockaddr_in6*)rp)->sin6_addr, strv6, 16));
        			memcpy((struct sockaddr_in6*)&config_pP->e_dns_emulation.sockaddr[i], (struct sockaddr_in6*)rp->ai_addr, rp->ai_addrlen);
        		}
        		config_pP->e_dns_emulation.interface_type[i] = S10_MME_GTP_C;
        		((struct sockaddr*)&config_pP->e_dns_emulation.sockaddr[i])->sa_family = rp->ai_family;
        	}
        	freeaddrinfo(result);
        	bdestroy_wrapper(&address);
        }
        config_pP->e_dns_emulation.nb_service_entries++;
      }
    }
  }

  OAILOG_SET_CONFIG(&config_pP->log_config);
  config_destroy (&cfg);
  return 0;
}


//------------------------------------------------------------------------------
static void mme_config_display (mme_config_t * config_pP)
{
  int                                     j;

  OAILOG_INFO (LOG_CONFIG, "==== EURECOM %s v%s ====\n", PACKAGE_NAME, PACKAGE_VERSION);
#if DEBUG_IS_ON
  OAILOG_DEBUG (LOG_CONFIG, "Built with CMAKE_BUILD_TYPE ................: %s\n", CMAKE_BUILD_TYPE);
  OAILOG_DEBUG (LOG_CONFIG, "Built with DISABLE_ITTI_DETECT_SUB_TASK_ID .: %d\n", DISABLE_ITTI_DETECT_SUB_TASK_ID);
  OAILOG_DEBUG (LOG_CONFIG, "Built with ITTI_TASK_STACK_SIZE ............: %d\n", ITTI_TASK_STACK_SIZE);
  OAILOG_DEBUG (LOG_CONFIG, "Built with ITTI_LITE .......................: %d\n", ITTI_LITE);
  OAILOG_DEBUG (LOG_CONFIG, "Built with LOG_OAI .........................: %d\n", LOG_OAI);
  OAILOG_DEBUG (LOG_CONFIG, "Built with LOG_OAI_CLEAN_HARD ..............: %d\n", LOG_OAI_CLEAN_HARD);
  OAILOG_DEBUG (LOG_CONFIG, "Built with MESSAGE_CHART_GENERATOR .........: %d\n", MESSAGE_CHART_GENERATOR);
  OAILOG_DEBUG (LOG_CONFIG, "Built with PACKAGE_NAME ....................: %s\n", PACKAGE_NAME);
  OAILOG_DEBUG (LOG_CONFIG, "Built with S1AP_DEBUG_LIST .................: %d\n", S1AP_DEBUG_LIST);
  OAILOG_DEBUG (LOG_CONFIG, "Built with SECU_DEBUG ......................: %d\n", SECU_DEBUG);
  OAILOG_DEBUG (LOG_CONFIG, "Built with SCTP_DUMP_LIST ..................: %d\n", SCTP_DUMP_LIST);
  OAILOG_DEBUG (LOG_CONFIG, "Built with TRACE_HASHTABLE .................: %d\n", TRACE_HASHTABLE);
  OAILOG_DEBUG (LOG_CONFIG, "Built with TRACE_3GPP_SPEC .................: %d\n", TRACE_3GPP_SPEC);
#endif
  OAILOG_INFO (LOG_CONFIG, "Configuration:\n");
  OAILOG_INFO (LOG_CONFIG, "- File .................................: %s\n", bdata(config_pP->config_file));
  OAILOG_INFO (LOG_CONFIG, "- Realm ................................: %s\n", bdata(config_pP->realm));
  OAILOG_INFO (LOG_CONFIG, "- Run mode .............................: %s\n", (RUN_MODE_BASIC == config_pP->run_mode) ? "BASIC":(RUN_MODE_SCENARIO_PLAYER == config_pP->run_mode) ? "SCENARIO_PLAYER":"UNKNOWN");
  OAILOG_INFO (LOG_CONFIG, "- Max S1 eNBs ..........................: %u\n", config_pP->max_s1_enbs);
  OAILOG_INFO (LOG_CONFIG, "- Max UEs ..............................: %u\n", config_pP->max_ues);
  OAILOG_INFO (LOG_CONFIG, "- IMS voice over PS session in S1 ......: %s\n", config_pP->eps_network_feature_support.ims_voice_over_ps_session_in_s1 == 0 ? "false" : "true");
  OAILOG_INFO (LOG_CONFIG, "- Emergency bearer services in S1 mode .: %s\n", config_pP->eps_network_feature_support.emergency_bearer_services_in_s1_mode == 0 ? "false" : "true");
  OAILOG_INFO (LOG_CONFIG, "- Location services via epc ............: %s\n", config_pP->eps_network_feature_support.location_services_via_epc == 0 ? "false" : "true");
  OAILOG_INFO (LOG_CONFIG, "- Extended service request .............: %s\n", config_pP->eps_network_feature_support.extended_service_request == 0 ? "false" : "true");
  OAILOG_INFO (LOG_CONFIG, "- Unauth IMSI support ..................: %s\n", config_pP->unauthenticated_imsi_supported == 0 ? "false" : "true");
  OAILOG_INFO (LOG_CONFIG, "- Relative capa ........................: %u\n", config_pP->relative_capacity);
  OAILOG_INFO (LOG_CONFIG, "- Statistics timer .....................: %u (seconds)\n\n", config_pP->mme_statistic_timer);
  OAILOG_INFO (LOG_CONFIG, "- S1-MME:\n");
  OAILOG_INFO (LOG_CONFIG, "    port number ......: %d\n", config_pP->s1ap_config.port_number);
  OAILOG_INFO (LOG_CONFIG, "- IP:\n");
  // todo: print ipv6 addresses
  OAILOG_INFO (LOG_CONFIG, "    s1-MME iface .....: %s\n", bdata(config_pP->ip.if_name_s1_mme));
  OAILOG_INFO (LOG_CONFIG, "    s1-MME ip ........: %s\n", inet_ntoa (*((struct in_addr *)&config_pP->ip.s1_mme_v4)));
  OAILOG_INFO (LOG_CONFIG, "    s11 MME iface ....: %s\n", bdata(config_pP->ip.if_name_s11));
  OAILOG_INFO (LOG_CONFIG, "    s11 MME port .....: %d\n", config_pP->ip.port_s11);
  OAILOG_INFO (LOG_CONFIG, "    s11 MME ip .......: %s\n", inet_ntoa (*((struct in_addr *)&config_pP->ip.s11_mme_v4)));
  OAILOG_INFO (LOG_CONFIG, "    s10 MME iface ....: %s\n", bdata(config_pP->ip.if_name_s10));
  OAILOG_INFO (LOG_CONFIG, "    s10 MME port .....: %d\n", config_pP->ip.port_s10);
  OAILOG_INFO (LOG_CONFIG, "    s10 MME ip .......: %s\n", inet_ntoa (*((struct in_addr *)&config_pP->ip.s10_mme_v4)));
  OAILOG_INFO (LOG_CONFIG, "- ITTI:\n");
  OAILOG_INFO (LOG_CONFIG, "    queue size .......: %u (bytes)\n", config_pP->itti_config.queue_size);
  OAILOG_INFO (LOG_CONFIG, "    log file .........: %s\n", bdata(config_pP->itti_config.log_file));
  OAILOG_INFO (LOG_CONFIG, "- SCTP:\n");
  OAILOG_INFO (LOG_CONFIG, "    in streams .......: %u\n", config_pP->sctp_config.in_streams);
  OAILOG_INFO (LOG_CONFIG, "    out streams ......: %u\n", config_pP->sctp_config.out_streams);
  OAILOG_INFO (LOG_CONFIG, "- GUMMEIs (PLMN|MMEGI|MMEC):\n");
  for (j = 0; j < config_pP->gummei.nb; j++) {
    OAILOG_INFO (LOG_CONFIG, "            " PLMN_FMT "|%u|%u \n",
        PLMN_ARG(&config_pP->gummei.gummei[j].plmn), config_pP->gummei.gummei[j].mme_gid, config_pP->gummei.gummei[j].mme_code);
  }

  OAILOG_INFO (LOG_CONFIG, "- TAIs : (mcc.mnc:tac)\n");
  switch (config_pP->served_tai.list_type) {
  case TRACKING_AREA_IDENTITY_LIST_TYPE_ONE_PLMN_CONSECUTIVE_TACS:
    OAILOG_INFO (LOG_CONFIG, "- TAI list type one PLMN consecutive TACs\n");
    break;
  case TRACKING_AREA_IDENTITY_LIST_TYPE_ONE_PLMN_NON_CONSECUTIVE_TACS:
    OAILOG_INFO (LOG_CONFIG, "- TAI list type one PLMN non consecutive TACs\n");
    break;
  case TRACKING_AREA_IDENTITY_LIST_TYPE_MANY_PLMNS:
    OAILOG_INFO (LOG_CONFIG, "- TAI list type multiple PLMNs\n");
    break;
  }
  for (j = 0; j < config_pP->served_tai.nb_tai; j++) {
    if (config_pP->served_tai.plmn_mnc_len[j] == 2) {
      OAILOG_INFO (LOG_CONFIG, "            %3u.%3u:%u\n",
          config_pP->served_tai.plmn_mcc[j], config_pP->served_tai.plmn_mnc[j], config_pP->served_tai.tac[j]);
    } else {
      OAILOG_INFO (LOG_CONFIG, "            %3u.%03u:%u\n",
          config_pP->served_tai.plmn_mcc[j], config_pP->served_tai.plmn_mnc[j], config_pP->served_tai.tac[j]);
    }
  }

  OAILOG_INFO (LOG_CONFIG, "- NAS:\n");
  OAILOG_INFO (LOG_CONFIG, "    Prefered Integrity Algorithms .: EIA%d EIA%d EIA%d EIA%d (decreasing priority)\n",
      config_pP->nas_config.prefered_integrity_algorithm[0],
      config_pP->nas_config.prefered_integrity_algorithm[1],
      config_pP->nas_config.prefered_integrity_algorithm[2],
      config_pP->nas_config.prefered_integrity_algorithm[3]);
  OAILOG_INFO (LOG_CONFIG, "    Prefered Integrity Algorithms .: EEA%d EEA%d EEA%d EEA%d (decreasing priority)\n",
      config_pP->nas_config.prefered_ciphering_algorithm[0],
      config_pP->nas_config.prefered_ciphering_algorithm[1],
      config_pP->nas_config.prefered_ciphering_algorithm[2],
      config_pP->nas_config.prefered_ciphering_algorithm[3]);
  OAILOG_INFO (LOG_CONFIG, "    T3346 ....: %d min\n", config_pP->nas_config.t3346_sec);
  OAILOG_INFO (LOG_CONFIG, "    T3402 ....: %d min\n", config_pP->nas_config.t3402_min);
  OAILOG_INFO (LOG_CONFIG, "    T3412 ....: %d min\n", config_pP->nas_config.t3412_min);
  OAILOG_INFO (LOG_CONFIG, "    T3422 ....: %d sec\n", config_pP->nas_config.t3422_sec);
  OAILOG_INFO (LOG_CONFIG, "    T3450 ....: %d sec\n", config_pP->nas_config.t3450_sec);
  OAILOG_INFO (LOG_CONFIG, "    T3460 ....: %d sec\n", config_pP->nas_config.t3460_sec);
  OAILOG_INFO (LOG_CONFIG, "    T3470 ....: %d sec\n", config_pP->nas_config.t3470_sec);
  OAILOG_INFO (LOG_CONFIG, "    T3485 ....: %d sec\n", config_pP->nas_config.t3485_sec);
  OAILOG_INFO (LOG_CONFIG, "    T3486 ....: %d sec\n", config_pP->nas_config.t3486_sec);
  OAILOG_INFO (LOG_CONFIG, "    T3489 ....: %d sec\n", config_pP->nas_config.t3489_sec);
  OAILOG_INFO (LOG_CONFIG, "    T3470 ....: %d sec\n", config_pP->nas_config.t3470_sec);
  OAILOG_INFO (LOG_CONFIG, "    T3495 ....: %d sec\n", config_pP->nas_config.t3495_sec);
  OAILOG_INFO (LOG_CONFIG, "    NAS non standart features .:\n");
  OAILOG_INFO (LOG_CONFIG, "      Force TAU ...................: %s\n", (config_pP->nas_config.force_tau) ? "true":"false");
  OAILOG_INFO (LOG_CONFIG, "      Force reject SR .............: %s\n", (config_pP->nas_config.force_reject_sr) ? "true":"false");
  OAILOG_INFO (LOG_CONFIG, "      Disable Esm information .....: %s\n", (config_pP->nas_config.disable_esm_information) ? "true":"false");

  OAILOG_INFO (LOG_CONFIG, "- S6A:\n");
  OAILOG_INFO (LOG_CONFIG, "    conf file ........: %s\n", bdata(config_pP->s6a_config.conf_file));
  OAILOG_INFO (LOG_CONFIG, "- Logging:\n");
  OAILOG_INFO (LOG_CONFIG, "    Output ..............: %s\n", bdata(config_pP->log_config.output));
  OAILOG_INFO (LOG_CONFIG, "    Output thread safe ..: %s\n", (config_pP->log_config.is_output_thread_safe) ? "true":"false");
  OAILOG_INFO (LOG_CONFIG, "    Output with color ...: %s\n", (config_pP->log_config.color) ? "true":"false");
  OAILOG_INFO (LOG_CONFIG, "    UDP log level........: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.udp_log_level));
  OAILOG_INFO (LOG_CONFIG, "    GTPV2-C log level....: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.gtpv2c_log_level));
  OAILOG_INFO (LOG_CONFIG, "    SCTP log level.......: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.sctp_log_level));
  OAILOG_INFO (LOG_CONFIG, "    S1AP log level.......: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.s1ap_log_level));
  OAILOG_INFO (LOG_CONFIG, "    ASN1 Verbosity level : %d\n", config_pP->log_config.asn1_verbosity_level);
  OAILOG_INFO (LOG_CONFIG, "    NAS log level........: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.nas_log_level));
  OAILOG_INFO (LOG_CONFIG, "    MME_APP log level....: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.mme_app_log_level));
  OAILOG_INFO (LOG_CONFIG, "    S10 log level........: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.s10_log_level));
  OAILOG_INFO (LOG_CONFIG, "    S11 log level........: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.s11_log_level));
  OAILOG_INFO (LOG_CONFIG, "    S6a log level........: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.s6a_log_level));
  OAILOG_INFO (LOG_CONFIG, "    UTIL log level.......: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.util_log_level));
  OAILOG_INFO (LOG_CONFIG, "    MSC log level........: %s (MeSsage Chart)\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.msc_log_level));
  OAILOG_INFO (LOG_CONFIG, "    ITTI log level.......: %s (InTer-Task Interface)\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.itti_log_level));
  OAILOG_INFO (LOG_CONFIG, "    XML log level........: %s (XML dump/load of messages)\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.xml_log_level));
#if TRACE_XML
  if (RUN_MODE_SCENARIO_PLAYER == config_pP->run_mode) {
    OAILOG_INFO (LOG_CONFIG, "    MME SP log level.....: %s (MME scenario player)\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.mme_scenario_player_log_level));
  }
#endif
}

//------------------------------------------------------------------------------
static void usage (char *target)
{
  OAI_FPRINTF_INFO( "==== EURECOM %s version: %s ====\n", PACKAGE_NAME, PACKAGE_VERSION);
  OAI_FPRINTF_INFO( "Please report any bug to: %s\n", PACKAGE_BUGREPORT);
  OAI_FPRINTF_INFO( "Usage: %s [options]\n", target);
  OAI_FPRINTF_INFO( "Available options:\n");
  OAI_FPRINTF_INFO( "-h      Print this help and return\n");
  OAI_FPRINTF_INFO( "-c<path>\n");
  OAI_FPRINTF_INFO( "        Set the configuration file for mme\n");
  OAI_FPRINTF_INFO( "        See template in UTILS/CONF\n");
#if TRACE_XML
  OAI_FPRINTF_INFO( "-s<path>\n");
  OAI_FPRINTF_INFO( "        Set the scenario file for mme scenario player\n");
#endif
  OAI_FPRINTF_INFO( "-V      Print %s version and return\n", PACKAGE_NAME);
  OAI_FPRINTF_INFO( "-v[1-2] Debug level:\n");
  OAI_FPRINTF_INFO( "            1 -> ASN1 XER printf on and ASN1 debug off\n");
  OAI_FPRINTF_INFO( "            2 -> ASN1 XER printf on and ASN1 debug on\n");
}

//------------------------------------------------------------------------------
int
mme_config_parse_opt_line (
  int argc,
  char *argv[],
  mme_config_t * config_pP)
{
  int                                     c;
  char                                   *config_file = NULL;

  mme_config_init (config_pP);

  /*
   * Parsing command line
   */
  while ((c = getopt (argc, argv, "c:hs:S:v:V")) != -1) {
    switch (c) {
    case 'c':{
        /*
         * Store the given configuration file. If no file is given,
         * * * * then the default values will be used.
         */
        config_pP->config_file = blk2bstr(optarg, strlen(optarg));
        OAI_FPRINTF_INFO ("%s mme_config.config_file %s\n", __FUNCTION__, bdata(config_pP->config_file));
      }
      break;

    case 'v':{
        config_pP->log_config.asn1_verbosity_level = atoi (optarg);
      }
      break;

    case 'V':{
      OAI_FPRINTF_INFO ("==== EURECOM %s v%s ====" "Please report any bug to: %s\n", PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_BUGREPORT);
      }
      break;

#if TRACE_XML
    case 'S':
      config_pP->run_mode = RUN_MODE_SCENARIO_PLAYER;
      config_pP->scenario_player_config.scenario_file = blk2bstr(optarg, strlen(optarg));
      config_pP->scenario_player_config.stop_on_error = true;
      OAI_FPRINTF_INFO ("%s mme_config.scenario_player_config.scenario_file %s\n", __FUNCTION__, bdata(config_pP->scenario_player_config.scenario_file));
      break;
    case 's':
      config_pP->run_mode = RUN_MODE_SCENARIO_PLAYER;
      config_pP->scenario_player_config.scenario_file = blk2bstr(optarg, strlen(optarg));
      config_pP->scenario_player_config.stop_on_error = false;
      OAI_FPRINTF_INFO ("%s mme_config.scenario_player_config.scenario_file %s\n", __FUNCTION__, bdata(config_pP->scenario_player_config.scenario_file));
      break;
#else
    case 's':
    case 'S':
      OAI_FPRINTF_ERR ("Should have compiled mme executable with TRACE_XML set in CMakeLists.template\n");
      exit (0);
      break;
#endif

    case 'h':                  /* Fall through */
    default:
      OAI_FPRINTF_ERR ("Unknown command line option %c\n", c);
      usage (argv[0]);
      exit (0);
    }
  }

  if (!config_pP->config_file) {
    config_file = getenv("CONFIG_FILE");
    if (config_file) {
      config_pP->config_file            = bfromcstr(config_file);
    } else {
      OAILOG_ERROR (LOG_CONFIG, "No config file provided through arg -c, or env variable CONFIG_FILE, exiting\n");
      return RETURNerror;
    }
  }
  OAILOG_DEBUG (LOG_CONFIG, "Config file is %s\n", config_file);
  /*
   * Parse the configuration file using libconfig
   */
  if (mme_config_parse_file (config_pP) != 0) {
    return -1;
  }

  /*
   * Display the configuration
   */
  mme_config_display (config_pP);
  return 0;
}

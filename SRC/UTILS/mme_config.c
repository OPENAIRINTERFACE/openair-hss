/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
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
#include <errno.h>
#include <libconfig.h>

#include <arpa/inet.h>          /* To provide inet_addr */

#include "intertask_interface.h"
#include "assertions.h"
#include "msc.h"
#include "mme_config.h"
#include "spgw_config.h"
#include "3gpp_33.401.h"
#include "common_types.h"
#include "intertask_interface_conf.h"
#include "dynamic_memory_check.h"
#include "log.h"

mme_config_t                            mme_config;

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
static void mme_config_init (mme_config_t * mme_config_p)
{
  memset(&mme_config, 0, sizeof(mme_config));
  pthread_rwlock_init (&mme_config_p->rw_lock, NULL);
  mme_config_p->log_config.output             = NULL;
  mme_config_p->log_config.color              = false;
  mme_config_p->log_config.udp_log_level      = MAX_LOG_LEVEL; // Means invalid
  mme_config_p->log_config.gtpv1u_log_level   = MAX_LOG_LEVEL; // will not overwrite existing log levels if MME and S-GW bundled in same executable
  mme_config_p->log_config.gtpv2c_log_level   = MAX_LOG_LEVEL;
  mme_config_p->log_config.sctp_log_level     = MAX_LOG_LEVEL;
  mme_config_p->log_config.s1ap_log_level     = MAX_LOG_LEVEL;
  mme_config_p->log_config.nas_log_level      = MAX_LOG_LEVEL;
  mme_config_p->log_config.mme_app_log_level  = MAX_LOG_LEVEL;
  mme_config_p->log_config.spgw_app_log_level = MAX_LOG_LEVEL;
  mme_config_p->log_config.s11_log_level      = MAX_LOG_LEVEL;
  mme_config_p->log_config.s6a_log_level      = MAX_LOG_LEVEL;
  mme_config_p->log_config.util_log_level     = MAX_LOG_LEVEL;
  mme_config_p->log_config.msc_log_level      = MAX_LOG_LEVEL;
  mme_config_p->log_config.itti_log_level     = MAX_LOG_LEVEL;

  mme_config_p->log_config.asn1_verbosity_level = 0;
  mme_config_p->config_file = NULL;
  mme_config_p->max_enbs = MAX_NUMBER_OF_ENB;
  mme_config_p->max_ues = MAX_NUMBER_OF_UE;
  mme_config_p->unauthenticated_imsi_supported = 0;
  /*
   * EPS network feature support
   */
  mme_config_p->eps_network_feature_support.emergency_bearer_services_in_s1_mode = 0;
  mme_config_p->eps_network_feature_support.extended_service_request = 0;
  mme_config_p->eps_network_feature_support.ims_voice_over_ps_session_in_s1 = 0;
  mme_config_p->eps_network_feature_support.location_services_via_epc = 0;

  mme_config_p->s1ap_config.port_number = S1AP_PORT_NUMBER;
  /*
   * IP configuration
   */
  mme_config_p->ipv4.mme_interface_name_for_s1_mme = "none";
  mme_config_p->ipv4.mme_ip_address_for_s1_mme = 0;
  mme_config_p->ipv4.mme_interface_name_for_s11 = "none";
  mme_config_p->ipv4.mme_ip_address_for_s11 = 0;
  mme_config_p->ipv4.sgw_ip_address_for_s11 = 0;
  mme_config_p->s6a_config.conf_file = S6A_CONF_FILE;
  mme_config_p->itti_config.queue_size = ITTI_QUEUE_MAX_ELEMENTS;
  mme_config_p->itti_config.log_file = NULL;
  mme_config_p->sctp_config.in_streams = SCTP_IN_STREAMS;
  mme_config_p->sctp_config.out_streams = SCTP_OUT_STREAMS;
  mme_config_p->relative_capacity = RELATIVE_CAPACITY;
  mme_config_p->mme_statistic_timer = MME_STATISTIC_TIMER_S;
  mme_config_p->gummei.nb = 1;
  mme_config_p->gummei.gummei[0].mme_code = MMEC;
  mme_config_p->gummei.gummei[0].mme_gid = MMEGID;
  mme_config_p->gummei.gummei[0].plmn.mcc_digit1 = 0;
  mme_config_p->gummei.gummei[0].plmn.mcc_digit2 = 0;
  mme_config_p->gummei.gummei[0].plmn.mcc_digit3 = 1;
  mme_config_p->gummei.gummei[0].plmn.mcc_digit1 = 0;
  mme_config_p->gummei.gummei[0].plmn.mcc_digit2 = 1;
  mme_config_p->gummei.gummei[0].plmn.mcc_digit3 = 0x0F;

  /*
   * Set the TAI
   */
  mme_config_p->served_tai.nb_tai = 1;
  mme_config_p->served_tai.plmn_mcc = calloc (1, sizeof (*mme_config_p->served_tai.plmn_mcc));
  mme_config_p->served_tai.plmn_mnc = calloc (1, sizeof (*mme_config_p->served_tai.plmn_mnc));
  mme_config_p->served_tai.plmn_mnc_len = calloc (1, sizeof (*mme_config_p->served_tai.plmn_mnc_len));
  mme_config_p->served_tai.tac = calloc (1, sizeof (*mme_config_p->served_tai.tac));
  mme_config_p->served_tai.plmn_mcc[0] = PLMN_MCC;
  mme_config_p->served_tai.plmn_mnc[0] = PLMN_MNC;
  mme_config_p->served_tai.plmn_mnc_len[0] = PLMN_MNC_LEN;
  mme_config_p->served_tai.tac[0] = PLMN_TAC;
  mme_config_p->s1ap_config.outcome_drop_timer_sec = S1AP_OUTCOME_TIMER_DEFAULT;
}

//------------------------------------------------------------------------------
int mme_system (char *command_pP, int abort_on_errorP)
{
  int                                     ret = -1;

  if (command_pP) {
    OAILOG_DEBUG (LOG_CONFIG, "system command: %s\n", command_pP);
    ret = system (command_pP);

    if (ret < 0) {
      OAILOG_ERROR (LOG_CONFIG, "ERROR in system command %s: %d\n", command_pP, ret);

      if (abort_on_errorP) {
        exit (-1);              // may be not exit
      }
    }
  }

  return ret;
}


//------------------------------------------------------------------------------
static int mme_config_parse_file (mme_config_t * mme_config_p)
{
  config_t                                cfg = {0};
  config_setting_t                       *setting_mme = NULL;
  config_setting_t                       *setting = NULL;
  config_setting_t                       *subsetting = NULL;
  config_setting_t                       *sub2setting = NULL;
  int                                     aint = 0;
  int                                     i = 0,n = 0,
                                          stop_index = 0,
                                          num = 0;
  const char                             *astring = NULL;
  char                                   *address = NULL;
  char                                   *cidr = NULL;
  const char                             *tac = NULL;
  const char                             *mcc = NULL;
  const char                             *mnc = NULL;
  char                                   *mme_interface_name_for_s1_mme = NULL;
  char                                   *mme_ip_address_for_s1_mme = NULL;
  char                                   *mme_interface_name_for_s11 = NULL;
  char                                   *mme_ip_address_for_s11 = NULL;
  char                                   *sgw_ip_address_for_s11 = NULL;
  bool                                    swap = false;

  config_init (&cfg);

  if (mme_config_p->config_file != NULL) {
    /*
     * Read the file. If there is an error, report it and exit.
     */
    if (!config_read_file (&cfg, mme_config_p->config_file)) {
      OAILOG_ERROR (LOG_CONFIG, ": %s:%d - %s\n", mme_config_p->config_file, config_error_line (&cfg), config_error_text (&cfg));
      config_destroy (&cfg);
      AssertFatal (1 == 0, "Failed to parse MME configuration file %s!\n", mme_config_p->config_file);
    }
  } else {
    OAILOG_ERROR (LOG_CONFIG, " No MME configuration file provided!\n");
    config_destroy (&cfg);
    AssertFatal (0, "No MME configuration file provided!\n");
  }

  setting_mme = config_lookup (&cfg, MME_CONFIG_STRING_MME_CONFIG);

  if (setting_mme != NULL) {
    // LOGGING setting
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_LOGGING);


    if (setting != NULL) {
      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_OUTPUT, (const char **)&astring))
        mme_config_p->log_config.output = strdup (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_COLOR, (const char **)&astring)) {
        if (0 == strcasecmp("true", astring)) mme_config_p->log_config.color = true;
        else mme_config_p->log_config.color = false;
      }

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_SCTP_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.sctp_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_S1AP_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.s1ap_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_NAS_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.nas_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_MME_APP_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.mme_app_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_S6A_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.s6a_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_UTIL_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.util_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_MSC_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.msc_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_ITTI_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.itti_log_level = OAILOG_LEVEL_STR2INT (astring);

      if ((config_setting_lookup_string (setting_mme, MME_CONFIG_STRING_ASN1_VERBOSITY, (const char **)&astring))) {
        if (strcasecmp (astring, MME_CONFIG_STRING_ASN1_VERBOSITY_NONE) == 0)
          mme_config_p->log_config.asn1_verbosity_level = 0;
        else if (strcasecmp (astring, MME_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING) == 0)
          mme_config_p->log_config.asn1_verbosity_level = 2;
        else if (strcasecmp (astring, MME_CONFIG_STRING_ASN1_VERBOSITY_INFO) == 0)
          mme_config_p->log_config.asn1_verbosity_level = 1;
        else
          mme_config_p->log_config.asn1_verbosity_level = 0;
      }
    }

    // GENERAL MME SETTINGS
    if ((config_setting_lookup_string (setting_mme, MME_CONFIG_STRING_RUN_MODE, (const char **)&astring))) {
      if (strcasecmp (astring, MME_CONFIG_STRING_RUN_MODE_TEST) == 0)
        mme_config_p->run_mode = RUN_MODE_TEST;
      else
        mme_config_p->run_mode = RUN_MODE_OTHER;
    }

    if ((config_setting_lookup_string (setting_mme, MME_CONFIG_STRING_REALM, (const char **)&astring))) {
      mme_config_p->realm = strdup (astring);
      mme_config_p->realm_length = strlen (mme_config_p->realm);
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_MAXENB, &aint))) {
      mme_config_p->max_enbs = (uint32_t) aint;
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_MAXUE, &aint))) {
      mme_config_p->max_ues = (uint32_t) aint;
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_RELATIVE_CAPACITY, &aint))) {
      mme_config_p->relative_capacity = (uint8_t) aint;
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_STATISTIC_TIMER, &aint))) {
      mme_config_p->mme_statistic_timer = (uint32_t) aint;
    }

    if ((config_setting_lookup_string (setting_mme, EPS_NETWORK_FEATURE_SUPPORT_EMERGENCY_BEARER_SERVICES_IN_S1_MODE, (const char **)&astring))) {
      if (strcasecmp (astring, "yes") == 0)
        mme_config_p->eps_network_feature_support.emergency_bearer_services_in_s1_mode = 1;
      else
        mme_config_p->eps_network_feature_support.emergency_bearer_services_in_s1_mode = 0;
    }
    if ((config_setting_lookup_string (setting_mme, EPS_NETWORK_FEATURE_SUPPORT_EXTENDED_SERVICE_REQUEST, (const char **)&astring))) {
      if (strcasecmp (astring, "yes") == 0)
        mme_config_p->eps_network_feature_support.extended_service_request = 1;
      else
        mme_config_p->eps_network_feature_support.extended_service_request = 0;
    }
    if ((config_setting_lookup_string (setting_mme, EPS_NETWORK_FEATURE_SUPPORT_IMS_VOICE_OVER_PS_SESSION_IN_S1, (const char **)&astring))) {
      if (strcasecmp (astring, "yes") == 0)
        mme_config_p->eps_network_feature_support.ims_voice_over_ps_session_in_s1 = 1;
      else
        mme_config_p->eps_network_feature_support.ims_voice_over_ps_session_in_s1 = 0;
    }
    if ((config_setting_lookup_string (setting_mme, EPS_NETWORK_FEATURE_SUPPORT_LOCATION_SERVICES_VIA_EPC, (const char **)&astring))) {
      if (strcasecmp (astring, "yes") == 0)
        mme_config_p->eps_network_feature_support.location_services_via_epc = 1;
      else
        mme_config_p->eps_network_feature_support.location_services_via_epc = 0;
    }

    if ((config_setting_lookup_string (setting_mme, MME_CONFIG_STRING_UNAUTHENTICATED_IMSI_SUPPORTED, (const char **)&astring))) {
      if (strcasecmp (astring, "yes") == 0)
        mme_config_p->unauthenticated_imsi_supported = 1;
      else
        mme_config_p->unauthenticated_imsi_supported = 0;
    }

    // ITTI SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_INTERTASK_INTERFACE_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_INTERTASK_INTERFACE_QUEUE_SIZE, &aint))) {
        mme_config_p->itti_config.queue_size = (uint32_t) aint;
      }
    }
    // S6A SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_S6A_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_string (setting, MME_CONFIG_STRING_S6A_CONF_FILE_PATH, (const char **)&astring))) {
        if (astring != NULL)
          mme_config_p->s6a_config.conf_file = strdup (astring);
      }

      if ((config_setting_lookup_string (setting, MME_CONFIG_STRING_S6A_HSS_HOSTNAME, (const char **)&astring))) {
        if (astring != NULL)
          mme_config_p->s6a_config.hss_host_name = strdup (astring);
        else
          AssertFatal (1 == 0, "You have to provide a valid HSS hostname %s=...\n", MME_CONFIG_STRING_S6A_HSS_HOSTNAME);
      }
    }
    // SCTP SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_SCTP_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_SCTP_INSTREAMS, &aint))) {
        mme_config_p->sctp_config.in_streams = (uint16_t) aint;
      }

      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_SCTP_OUTSTREAMS, &aint))) {
        mme_config_p->sctp_config.out_streams = (uint16_t) aint;
      }
    }
    // S1AP SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_S1AP_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_S1AP_OUTCOME_TIMER, &aint))) {
        mme_config_p->s1ap_config.outcome_drop_timer_sec = (uint8_t) aint;
      }

      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_S1AP_PORT, &aint))) {
        mme_config_p->s1ap_config.port_number = (uint16_t) aint;
      }
    }
    // TAI list setting
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_TAI_LIST);
    if (setting != NULL) {
      num = config_setting_length (setting);

      if (mme_config_p->served_tai.nb_tai != num) {
        if (mme_config_p->served_tai.plmn_mcc != NULL)
          free_wrapper (mme_config_p->served_tai.plmn_mcc);

        if (mme_config_p->served_tai.plmn_mnc != NULL)
          free_wrapper (mme_config_p->served_tai.plmn_mnc);

        if (mme_config_p->served_tai.plmn_mnc_len != NULL)
          free_wrapper (mme_config_p->served_tai.plmn_mnc_len);

        if (mme_config_p->served_tai.tac != NULL)
          free_wrapper (mme_config_p->served_tai.tac);

        mme_config_p->served_tai.plmn_mcc = calloc (num, sizeof (*mme_config_p->served_tai.plmn_mcc));
        mme_config_p->served_tai.plmn_mnc = calloc (num, sizeof (*mme_config_p->served_tai.plmn_mnc));
        mme_config_p->served_tai.plmn_mnc_len = calloc (num, sizeof (*mme_config_p->served_tai.plmn_mnc_len));
        mme_config_p->served_tai.tac = calloc (num, sizeof (*mme_config_p->served_tai.tac));
      }

      mme_config_p->served_tai.nb_tai = num;
      AssertFatal(16 >= num , "Too many TAIs configured %d", num);

      for (i = 0; i < num; i++) {
        sub2setting = config_setting_get_elem (setting, i);

        if (sub2setting != NULL) {
          if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_MCC, &mcc))) {
            mme_config_p->served_tai.plmn_mcc[i] = (uint16_t) atoi (mcc);
          }

          if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_MNC, &mnc))) {
            mme_config_p->served_tai.plmn_mnc[i] = (uint16_t) atoi (mnc);
            mme_config_p->served_tai.plmn_mnc_len[i] = strlen (mnc);
            AssertFatal ((mme_config_p->served_tai.plmn_mnc_len[i] == 2) || (mme_config_p->served_tai.plmn_mnc_len[i] == 3),
                "Bad MNC length %u, must be 2 or 3", mme_config_p->served_tai.plmn_mnc_len[i]);
          }

          if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_TAC, &tac))) {
            mme_config_p->served_tai.tac[i] = (uint16_t) atoi (tac);
            AssertFatal(TAC_IS_VALID(mme_config_p->served_tai.tac[i]), "Invalid TAC value "TAC_FMT, mme_config_p->served_tai.tac[i]);
          }
        }
      }
      // sort TAI list
      n = mme_config_p->served_tai.nb_tai;
      do {
        stop_index = 0;
        for (i = 1; i < n; i++) {
          swap = false;
          if (mme_config_p->served_tai.plmn_mcc[i-1] > mme_config_p->served_tai.plmn_mcc[i]) {
            swap = true;
          } else if (mme_config_p->served_tai.plmn_mcc[i-1] == mme_config_p->served_tai.plmn_mcc[i]) {
            if (mme_config_p->served_tai.plmn_mnc[i-1] > mme_config_p->served_tai.plmn_mnc[i]) {
              swap = true;
            } else  if (mme_config_p->served_tai.plmn_mnc[i-1] == mme_config_p->served_tai.plmn_mnc[i]) {
              if (mme_config_p->served_tai.tac[i-1] > mme_config_p->served_tai.tac[i]) {
                swap = true;
              }
            }
          }
          if (true == swap) {
            uint16_t swap16;
            swap16 = mme_config_p->served_tai.plmn_mcc[i-1];
            mme_config_p->served_tai.plmn_mcc[i-1] = mme_config_p->served_tai.plmn_mcc[i];
            mme_config_p->served_tai.plmn_mcc[i]   = swap16;

            swap16 = mme_config_p->served_tai.plmn_mnc[i-1];
            mme_config_p->served_tai.plmn_mnc[i-1] = mme_config_p->served_tai.plmn_mnc[i];
            mme_config_p->served_tai.plmn_mnc[i]   = swap16;

            swap16 = mme_config_p->served_tai.tac[i-1];
            mme_config_p->served_tai.tac[i-1] = mme_config_p->served_tai.tac[i];
            mme_config_p->served_tai.tac[i]   = swap16;

            stop_index = i;
          }
        }
        n = stop_index;
      } while (0 != n);
      // helper for determination of list type (global view), we could make sublists with different types, but keep things simple for now
      mme_config_p->served_tai.list_type = TRACKING_AREA_IDENTITY_LIST_TYPE_ONE_PLMN_CONSECUTIVE_TACS;
      for (i = 1; i < mme_config_p->served_tai.nb_tai; i++) {
        if ((mme_config_p->served_tai.plmn_mcc[i] != mme_config_p->served_tai.plmn_mcc[0]) ||
            (mme_config_p->served_tai.plmn_mnc[i] != mme_config_p->served_tai.plmn_mnc[0])){
          mme_config_p->served_tai.list_type = TRACKING_AREA_IDENTITY_LIST_TYPE_MANY_PLMNS;
          break;
        } else if ((mme_config_p->served_tai.plmn_mcc[i] != mme_config_p->served_tai.plmn_mcc[i-1]) ||
                   (mme_config_p->served_tai.plmn_mnc[i] != mme_config_p->served_tai.plmn_mnc[i-1])) {
          mme_config_p->served_tai.list_type = TRACKING_AREA_IDENTITY_LIST_TYPE_MANY_PLMNS;
          break;
        }
        if (mme_config_p->served_tai.tac[i] != (mme_config_p->served_tai.tac[i-1] + 1)) {
          mme_config_p->served_tai.list_type = TRACKING_AREA_IDENTITY_LIST_TYPE_ONE_PLMN_NON_CONSECUTIVE_TACS;
        }
      }
    }

    // GUMMEI SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_GUMMEI_LIST);
    mme_config_p->gummei.nb = 0;
    if (setting != NULL) {
      num = config_setting_length (setting);
      AssertFatal(num == 1, "Only one GUMMEI supported for this version of MME");
      for (i = 0; i < num; i++) {
        sub2setting = config_setting_get_elem (setting, i);

        if (sub2setting != NULL) {
          if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_MCC, &mcc))) {
            AssertFatal( 3 == strlen(mcc), "Bad MCC length, it must be 3 digit ex: 001");
            char c[2] = { mcc[0], 0};
            mme_config_p->gummei.gummei[i].plmn.mcc_digit1 = (uint8_t) atoi (c);
            c[0] = mcc[1];
            mme_config_p->gummei.gummei[i].plmn.mcc_digit2 = (uint8_t) atoi (c);
            c[0] = mcc[2];
            mme_config_p->gummei.gummei[i].plmn.mcc_digit3 = (uint8_t) atoi (c);
          }

          if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_MNC, &mnc))) {
            AssertFatal( (3 == strlen(mnc)) || (2 == strlen(mnc)) , "Bad MCC length, it must be 3 digit ex: 001");
            char c[2] = { mnc[0], 0};
            mme_config_p->gummei.gummei[i].plmn.mnc_digit1 = (uint8_t) atoi (c);
            c[0] = mnc[1];
            mme_config_p->gummei.gummei[i].plmn.mnc_digit2 = (uint8_t) atoi (c);
            if (3 == strlen(mnc)) {
              c[0] = mnc[2];
              mme_config_p->gummei.gummei[i].plmn.mnc_digit3 = (uint8_t) atoi (c);
            } else {
              mme_config_p->gummei.gummei[i].plmn.mnc_digit3 = 0x0F;
            }
          }

          if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_MME_GID, &mnc))) {
            mme_config_p->gummei.gummei[i].mme_gid = (uint16_t) atoi (mnc);
          }
          if ((config_setting_lookup_string (sub2setting, MME_CONFIG_STRING_MME_CODE, &mnc))) {
            mme_config_p->gummei.gummei[i].mme_code = (uint8_t) atoi (mnc);
          }
          mme_config_p->gummei.nb += 1;
        }
      }
    }
    // NETWORK INTERFACE SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_string (setting, MME_CONFIG_STRING_INTERFACE_NAME_FOR_S1_MME, (const char **)&mme_interface_name_for_s1_mme)
           && config_setting_lookup_string (setting, MME_CONFIG_STRING_IPV4_ADDRESS_FOR_S1_MME, (const char **)&mme_ip_address_for_s1_mme)
           && config_setting_lookup_string (setting, MME_CONFIG_STRING_INTERFACE_NAME_FOR_S11_MME, (const char **)&mme_interface_name_for_s11)
           && config_setting_lookup_string (setting, MME_CONFIG_STRING_IPV4_ADDRESS_FOR_S11_MME, (const char **)&mme_ip_address_for_s11)
           && config_setting_lookup_string (setting, SGW_CONFIG_STRING_SGW_IPV4_ADDRESS_FOR_S11, (const char **)&sgw_ip_address_for_s11)
          )
        ) {
        mme_config_p->ipv4.mme_interface_name_for_s1_mme = strdup (mme_interface_name_for_s1_mme);
        cidr = strdup (mme_ip_address_for_s1_mme);
        address = strtok (cidr, "/");
        IPV4_STR_ADDR_TO_INT_NWBO (address, mme_config_p->ipv4.mme_ip_address_for_s1_mme, "BAD IP ADDRESS FORMAT FOR MME S1_MME !\n");
        free_wrapper (cidr);

        mme_config_p->ipv4.mme_interface_name_for_s11 = strdup (mme_interface_name_for_s11);
        cidr = strdup (mme_ip_address_for_s11);
        address = strtok (cidr, "/");
        IPV4_STR_ADDR_TO_INT_NWBO (address, mme_config_p->ipv4.mme_ip_address_for_s11, "BAD IP ADDRESS FORMAT FOR MME S11 !\n");
        free_wrapper (cidr);

        cidr = strdup (sgw_ip_address_for_s11);
        address = strtok (cidr, "/");
        IPV4_STR_ADDR_TO_INT_NWBO (address, mme_config_p->ipv4.sgw_ip_address_for_s11, "BAD IP ADDRESS FORMAT FOR SGW S11 !\n");
        free_wrapper (cidr);

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
              mme_config_p->nas_config.prefered_integrity_algorithm[i] = EIA0_ALG_ID;
            else if (strcmp ("EIA1", astring) == 0)
              mme_config_p->nas_config.prefered_integrity_algorithm[i] = EIA1_128_ALG_ID;
            else if (strcmp ("EIA2", astring) == 0)
              mme_config_p->nas_config.prefered_integrity_algorithm[i] = EIA2_128_ALG_ID;
            else
              mme_config_p->nas_config.prefered_integrity_algorithm[i] = EIA0_ALG_ID;
          }

          for (i = num; i < 8; i++) {
            mme_config_p->nas_config.prefered_integrity_algorithm[i] = EIA0_ALG_ID;
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
              mme_config_p->nas_config.prefered_ciphering_algorithm[i] = EEA0_ALG_ID;
            else if (strcmp ("EEA1", astring) == 0)
              mme_config_p->nas_config.prefered_ciphering_algorithm[i] = EEA1_128_ALG_ID;
            else if (strcmp ("EEA2", astring) == 0)
              mme_config_p->nas_config.prefered_ciphering_algorithm[i] = EEA2_128_ALG_ID;
            else
              mme_config_p->nas_config.prefered_ciphering_algorithm[i] = EEA0_ALG_ID;
          }

          for (i = num; i < 8; i++) {
            mme_config_p->nas_config.prefered_ciphering_algorithm[i] = EEA0_ALG_ID;
          }
        }
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3402_TIMER, &aint))) {
        mme_config_p->nas_config.t3402_min = (uint8_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3412_TIMER, &aint))) {
        mme_config_p->nas_config.t3412_min = (uint8_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3485_TIMER, &aint))) {
        mme_config_p->nas_config.t3485_sec = (uint8_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3486_TIMER, &aint))) {
        mme_config_p->nas_config.t3486_sec = (uint8_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3489_TIMER, &aint))) {
        mme_config_p->nas_config.t3489_sec = (uint8_t) aint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3495_TIMER, &aint))) {
        mme_config_p->nas_config.t3495_sec = (uint8_t) aint;
      }
    }
  }

  setting = config_lookup (&cfg, SGW_CONFIG_STRING_SGW_CONFIG);

  if (setting != NULL) {
    // LOGGING setting
    subsetting = config_setting_get_member (setting, SGW_CONFIG_STRING_LOGGING);

    if (subsetting != NULL) {
      // already filled in MME section ?
      if (NULL == mme_config_p->log_config.output) {
        config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_OUTPUT, (const char **)&astring);
        mme_config_p->log_config.output = strdup (astring);
      }

      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_UDP_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.udp_log_level = OAILOG_LEVEL_STR2INT (astring);
      }

      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_SPGW_APP_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.spgw_app_log_level = OAILOG_LEVEL_STR2INT (astring);
      }

      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_GTPV1U_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.gtpv1u_log_level = OAILOG_LEVEL_STR2INT (astring);
      }

      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_GTPV2C_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.gtpv2c_log_level = OAILOG_LEVEL_STR2INT (astring);
      }

      // may be not present (may be filled in MME section)
      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_UTIL_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.util_log_level = OAILOG_LEVEL_STR2INT (astring);
      }

      // may be not present (may be filled in MME section)
      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_S11_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.s11_log_level = OAILOG_LEVEL_STR2INT (astring);
      }

      // may be not present (may be filled in MME section)
      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_MSC_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.msc_log_level = OAILOG_LEVEL_STR2INT (astring);
      }

      // may be not present (may be filled in MME section)
      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_ITTI_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.itti_log_level = OAILOG_LEVEL_STR2INT (astring);
      }
    }
  }

  OAILOG_SET_CONFIG(&mme_config_p->log_config);
  config_destroy (&cfg);
  return 0;
}


//------------------------------------------------------------------------------
static void mme_config_display (mme_config_t * mme_config_p)
{
  int                                     j;

  OAILOG_INFO (LOG_CONFIG, "==== EURECOM %s v%s ====\n", PACKAGE_NAME, PACKAGE_VERSION);
  OAILOG_INFO (LOG_CONFIG, "Configuration:\n");
  OAILOG_INFO (LOG_CONFIG, "- File .................................: %s\n", mme_config_p->config_file);
  OAILOG_INFO (LOG_CONFIG, "- Realm ................................: %s\n", mme_config_p->realm);
  OAILOG_INFO (LOG_CONFIG, "- Run mode .............................: %s\n", (RUN_MODE_TEST == mme_config_p->run_mode) ? "TEST":"NORMAL");
  OAILOG_INFO (LOG_CONFIG, "- Max eNBs .............................: %u\n", mme_config_p->max_enbs);
  OAILOG_INFO (LOG_CONFIG, "- Max UEs ..............................: %u\n", mme_config_p->max_ues);
  OAILOG_INFO (LOG_CONFIG, "- IMS voice over PS session in S1 ......: %s\n", mme_config_p->eps_network_feature_support.ims_voice_over_ps_session_in_s1 == 0 ? "false" : "true");
  OAILOG_INFO (LOG_CONFIG, "- Emergency bearer services in S1 mode .: %s\n", mme_config_p->eps_network_feature_support.emergency_bearer_services_in_s1_mode == 0 ? "false" : "true");
  OAILOG_INFO (LOG_CONFIG, "- Location services via epc ............: %s\n", mme_config_p->eps_network_feature_support.location_services_via_epc == 0 ? "false" : "true");
  OAILOG_INFO (LOG_CONFIG, "- Extended service request .............: %s\n", mme_config_p->eps_network_feature_support.extended_service_request == 0 ? "false" : "true");
  OAILOG_INFO (LOG_CONFIG, "- Unauth IMSI support ..................: %s\n", mme_config_p->unauthenticated_imsi_supported == 0 ? "false" : "true");
  OAILOG_INFO (LOG_CONFIG, "- Relative capa ........................: %u\n", mme_config_p->relative_capacity);
  OAILOG_INFO (LOG_CONFIG, "- Statistics timer .....................: %u (seconds)\n\n", mme_config_p->mme_statistic_timer);
  OAILOG_INFO (LOG_CONFIG, "- S1-MME:\n");
  OAILOG_INFO (LOG_CONFIG, "    port number ......: %d\n", mme_config_p->s1ap_config.port_number);
  OAILOG_INFO (LOG_CONFIG, "- IP:\n");
  OAILOG_INFO (LOG_CONFIG, "    s1-MME iface .....: %s\n", mme_config_p->ipv4.mme_interface_name_for_s1_mme);
  OAILOG_INFO (LOG_CONFIG, "    s1-MME ip ........: %s\n", inet_ntoa (*((struct in_addr *)&mme_config_p->ipv4.mme_ip_address_for_s1_mme)));
  OAILOG_INFO (LOG_CONFIG, "    s11 MME iface ....: %s\n", mme_config_p->ipv4.mme_interface_name_for_s11);
  OAILOG_INFO (LOG_CONFIG, "    s11 S-GW ip ......: %s\n", inet_ntoa (*((struct in_addr *)&mme_config_p->ipv4.mme_ip_address_for_s11)));
  OAILOG_INFO (LOG_CONFIG, "- ITTI:\n");
  OAILOG_INFO (LOG_CONFIG, "    queue size .......: %u (bytes)\n", mme_config_p->itti_config.queue_size);
  OAILOG_INFO (LOG_CONFIG, "    log file .........: %s\n", mme_config_p->itti_config.log_file);
  OAILOG_INFO (LOG_CONFIG, "- SCTP:\n");
  OAILOG_INFO (LOG_CONFIG, "    in streams .......: %u\n", mme_config_p->sctp_config.in_streams);
  OAILOG_INFO (LOG_CONFIG, "    out streams ......: %u\n", mme_config_p->sctp_config.out_streams);
  OAILOG_INFO (LOG_CONFIG, "- GUMMEIs (PLMN|MMEGI|MMEC):\n");
  for (j = 0; j < mme_config_p->gummei.nb; j++) {
    OAILOG_INFO (LOG_CONFIG, "            " PLMN_FMT "|%u|%u \n",
        PLMN_ARG(&mme_config_p->gummei.gummei[j].plmn), mme_config_p->gummei.gummei[j].mme_gid, mme_config_p->gummei.gummei[j].mme_code);
  }
  OAILOG_INFO (LOG_CONFIG, "- TAIs : (mcc.mnc:tac)\n");
  switch (mme_config_p->served_tai.list_type) {
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
  for (j = 0; j < mme_config_p->served_tai.nb_tai; j++) {
    if (mme_config_p->served_tai.plmn_mnc_len[j] == 2) {
      OAILOG_INFO (LOG_CONFIG, "            %3u.%3u:%u\n",
          mme_config_p->served_tai.plmn_mcc[j], mme_config_p->served_tai.plmn_mnc[j], mme_config_p->served_tai.tac[j]);
    } else {
      OAILOG_INFO (LOG_CONFIG, "            %3u.%03u:%u\n",
          mme_config_p->served_tai.plmn_mcc[j], mme_config_p->served_tai.plmn_mnc[j], mme_config_p->served_tai.tac[j]);
    }
  }

  OAILOG_INFO (LOG_CONFIG, "- S6A:\n");
  OAILOG_INFO (LOG_CONFIG, "    conf file ........: %s\n", mme_config_p->s6a_config.conf_file);
  OAILOG_INFO (LOG_CONFIG, "- Logging:\n");
  OAILOG_INFO (LOG_CONFIG, "    Output ..............: %s\n", mme_config_p->log_config.output);
  OAILOG_INFO (LOG_CONFIG, "    UDP log level........: %s\n", OAILOG_LEVEL_INT2STR(mme_config_p->log_config.udp_log_level));
  OAILOG_INFO (LOG_CONFIG, "    GTPV1-U log level....: %s\n", OAILOG_LEVEL_INT2STR(mme_config_p->log_config.gtpv1u_log_level));
  OAILOG_INFO (LOG_CONFIG, "    GTPV2-C log level....: %s\n", OAILOG_LEVEL_INT2STR(mme_config_p->log_config.gtpv2c_log_level));
  OAILOG_INFO (LOG_CONFIG, "    SCTP log level.......: %s\n", OAILOG_LEVEL_INT2STR(mme_config_p->log_config.sctp_log_level));
  OAILOG_INFO (LOG_CONFIG, "    S1AP log level.......: %s\n", OAILOG_LEVEL_INT2STR(mme_config_p->log_config.s1ap_log_level));
  OAILOG_INFO (LOG_CONFIG, "    ASN1 Verbosity level : %d\n", mme_config_p->log_config.asn1_verbosity_level);
  OAILOG_INFO (LOG_CONFIG, "    NAS log level........: %s\n", OAILOG_LEVEL_INT2STR(mme_config_p->log_config.nas_log_level));
  OAILOG_INFO (LOG_CONFIG, "    MME_APP log level....: %s\n", OAILOG_LEVEL_INT2STR(mme_config_p->log_config.mme_app_log_level));
  OAILOG_INFO (LOG_CONFIG, "    S/P-GW APP log level.: %s\n", OAILOG_LEVEL_INT2STR(mme_config_p->log_config.spgw_app_log_level));
  OAILOG_INFO (LOG_CONFIG, "    S11 log level........: %s\n", OAILOG_LEVEL_INT2STR(mme_config_p->log_config.s11_log_level));
  OAILOG_INFO (LOG_CONFIG, "    S6a log level........: %s\n", OAILOG_LEVEL_INT2STR(mme_config_p->log_config.s6a_log_level));
  OAILOG_INFO (LOG_CONFIG, "    UTIL log level.......: %s\n", OAILOG_LEVEL_INT2STR(mme_config_p->log_config.util_log_level));
  OAILOG_INFO (LOG_CONFIG, "    MSC log level........: %s (MeSsage Chart)\n", OAILOG_LEVEL_INT2STR(mme_config_p->log_config.msc_log_level));
  OAILOG_INFO (LOG_CONFIG, "    ITTI log level.......: %s (InTer-Task Interface)\n", OAILOG_LEVEL_INT2STR(mme_config_p->log_config.itti_log_level));
}

//------------------------------------------------------------------------------
static void usage (char *target)
{
  OAILOG_INFO (LOG_CONFIG, "==== EURECOM %s version: %s ====\n", PACKAGE_NAME, PACKAGE_VERSION);
  OAILOG_INFO (LOG_CONFIG, "Please report any bug to: %s\n", PACKAGE_BUGREPORT);
  OAILOG_INFO (LOG_CONFIG, "Usage: %s [options]\n", target);
  OAILOG_INFO (LOG_CONFIG, "Available options:\n");
  OAILOG_INFO (LOG_CONFIG, "-h      Print this help and return\n");
  OAILOG_INFO (LOG_CONFIG, "-c<path>\n");
  OAILOG_INFO (LOG_CONFIG, "        Set the configuration file for mme\n");
  OAILOG_INFO (LOG_CONFIG, "        See template in UTILS/CONF\n");
  OAILOG_INFO (LOG_CONFIG, "-K<file>\n");
  OAILOG_INFO (LOG_CONFIG, "        Output intertask messages to provided file\n");
  OAILOG_INFO (LOG_CONFIG, "-V      Print %s version and return\n", PACKAGE_NAME);
  OAILOG_INFO (LOG_CONFIG, "-v[1-2] Debug level:\n");
  OAILOG_INFO (LOG_CONFIG, "            1 -> ASN1 XER printf on and ASN1 debug off\n");
  OAILOG_INFO (LOG_CONFIG, "            2 -> ASN1 XER printf on and ASN1 debug on\n");
}

//------------------------------------------------------------------------------
int
mme_config_parse_opt_line (
  int argc,
  char *argv[],
  mme_config_t * mme_config_p)
{
  int                                     c;

  mme_config_init (mme_config_p);

  /*
   * Parsing command line
   */
  while ((c = getopt (argc, argv, "c:hi:K:v:V")) != -1) {
    switch (c) {
    case 'c':{
        /*
         * Store the given configuration file. If no file is given,
         * * * * then the default values will be used.
         */
        int                                     config_file_len = 0;

        config_file_len = strlen (optarg);
        mme_config_p->config_file = malloc (sizeof (char) * (config_file_len + 1));
        memcpy (mme_config_p->config_file, optarg, config_file_len);
        mme_config_p->config_file[config_file_len] = '\0';
        OAILOG_DEBUG (LOG_CONFIG, "%s mme_config.config_file %s\n", __FUNCTION__, mme_config_p->config_file);
      }
      break;

    case 'v':{
        mme_config_p->log_config.asn1_verbosity_level = atoi (optarg);
      }
      break;

    case 'V':{
        OAILOG_DEBUG (LOG_CONFIG, "==== EURECOM %s v%s ====" "Please report any bug to: %s\n", PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_BUGREPORT);
      }
      break;

    case 'K':
      mme_config_p->itti_config.log_file = strdup (optarg);
      OAILOG_DEBUG (LOG_CONFIG, "%s mme_config.itti_config.log_file %s\n", __FUNCTION__, mme_config_p->itti_config.log_file);
      break;

    case 'h':                  /* Fall through */
    default:
      usage (argv[0]);
      exit (0);
    }
  }

  /*
   * Parse the configuration file using libconfig
   */
  if (!mme_config_p->config_file) {
    mme_config_p->config_file = strdup("/usr/local/etc/oai/mme.conf");
  }
  if (mme_config_parse_file (mme_config_p) != 0) {
    return -1;
  }

  /*
   * Display the configuration
   */
  mme_config_display (mme_config_p);
  return 0;
}

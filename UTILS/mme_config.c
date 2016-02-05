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
#include "intertask_interface_conf.h"
#include "dynamic_memory_check.h"
#include "log.h"

mme_config_t                            mme_config={0};

int
mme_config_find_mnc_length (
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


static
  void
mme_config_init (
  mme_config_t * mme_config_p)
{
  pthread_rwlock_init (&mme_config_p->rw_lock, NULL);
  mme_config_p->log_config.output               = NULL;
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
  mme_config_p->max_eNBs = MAX_NUMBER_OF_ENB;
  mme_config_p->max_ues = MAX_NUMBER_OF_UE;
  mme_config_p->unauthenticated_imsi_supported = 0;
  /*
   * EPS network feature support
   */
  mme_config_p->eps_network_feature_support.emergency_bearer_services_in_s1_mode = 0;
  mme_config_p->eps_network_feature_support.extended_service_request = 0;
  mme_config_p->eps_network_feature_support.ims_voice_over_ps_session_in_s1 = 0;
  mme_config_p->eps_network_feature_support.location_services_via_epc = 0;
  /*
   * Timer configuration
   */
  mme_config_p->gtpv1u_config.port_number = GTPV1_U_PORT_NUMBER;
  mme_config_p->s1ap_config.port_number = S1AP_PORT_NUMBER;
  /*
   * IP configuration
   */
  mme_config_p->ipv4.sgw_ip_address_for_S1u_S12_S4_up = inet_addr (DEFAULT_SGW_IP_ADDRESS_FOR_S1U_S12_S4_UP);
  mme_config_p->ipv4.mme_interface_name_for_S1_MME = DEFAULT_MME_INTERFACE_NAME_FOR_S1_MME;
  mme_config_p->ipv4.mme_ip_address_for_S1_MME = inet_addr (DEFAULT_MME_IP_ADDRESS_FOR_S1_MME);
  mme_config_p->ipv4.mme_interface_name_for_S11 = DEFAULT_MME_INTERFACE_NAME_FOR_S11;
  mme_config_p->ipv4.mme_ip_address_for_S11 = inet_addr (DEFAULT_MME_IP_ADDRESS_FOR_S11);
  mme_config_p->ipv4.sgw_ip_address_for_S11 = inet_addr (DEFAULT_SGW_IP_ADDRESS_FOR_S11);
  mme_config_p->s6a_config.conf_file = S6A_CONF_FILE;
  mme_config_p->itti_config.queue_size = ITTI_QUEUE_MAX_ELEMENTS;
  mme_config_p->itti_config.log_file = NULL;
  mme_config_p->sctp_config.in_streams = SCTP_IN_STREAMS;
  mme_config_p->sctp_config.out_streams = SCTP_OUT_STREAMS;
  mme_config_p->relative_capacity = RELATIVE_CAPACITY;
  mme_config_p->mme_statistic_timer = MME_STATISTIC_TIMER_S;
  mme_config_p->gummei.nb_mme_gid = 1;
  mme_config_p->gummei.mme_gid = CALLOC_CHECK (1, sizeof (*mme_config_p->gummei.mme_gid));
  mme_config_p->gummei.mme_gid[0] = MMEGID;
  mme_config_p->gummei.nb_mmec = 1;
  mme_config_p->gummei.mmec = CALLOC_CHECK (1, sizeof (*mme_config_p->gummei.mmec));
  mme_config_p->gummei.mmec[0] = MMEC;
  /*
   * Set the TAI
   */
  mme_config_p->served_tai.nb_tai = 1;
  mme_config_p->served_tai.plmn_mcc = CALLOC_CHECK (1, sizeof (*mme_config_p->served_tai.plmn_mcc));
  mme_config_p->served_tai.plmn_mnc = CALLOC_CHECK (1, sizeof (*mme_config_p->served_tai.plmn_mnc));
  mme_config_p->served_tai.plmn_mnc_len = CALLOC_CHECK (1, sizeof (*mme_config_p->served_tai.plmn_mnc_len));
  mme_config_p->served_tai.tac = CALLOC_CHECK (1, sizeof (*mme_config_p->served_tai.tac));
  mme_config_p->served_tai.plmn_mcc[0] = PLMN_MCC;
  mme_config_p->served_tai.plmn_mnc[0] = PLMN_MNC;
  mme_config_p->served_tai.plmn_mnc_len[0] = PLMN_MNC_LEN;
  mme_config_p->served_tai.tac[0] = PLMN_TAC;
  mme_config_p->s1ap_config.outcome_drop_timer_sec = S1AP_OUTCOME_TIMER_DEFAULT;
}

int
mme_system (
  char *command_pP,
  int abort_on_errorP)
{
  int                                     ret = -1;

  if (command_pP) {
    LOG_DEBUG (LOG_CONFIG, "system command: %s\n", command_pP);
    ret = system (command_pP);

    if (ret < 0) {
      LOG_ERROR (LOG_CONFIG, "ERROR in system command %s: %d\n", command_pP, ret);

      if (abort_on_errorP) {
        exit (-1);              // may be not exit
      }
    }
  }

  return ret;
}


static int
config_parse_file (
  mme_config_t * mme_config_p)
{
  config_t                                cfg;
  config_setting_t                       *setting_mme = NULL;
  config_setting_t                       *setting = NULL;
  config_setting_t                       *subsetting = NULL;
  config_setting_t                       *sub2setting = NULL;
  long int                                alongint;
  int                                     i,n,
                                          stop_index,
                                          num;
  char                                   *astring = NULL;
  char                                   *address = NULL;
  char                                   *cidr = NULL;
  const char                             *tac = NULL;
  const char                             *mcc = NULL;
  const char                             *mnc = NULL;
  char                                   *sgw_ip_address_for_S1u_S12_S4_up = NULL;
  char                                   *mme_interface_name_for_S1_MME = NULL;
  char                                   *mme_ip_address_for_S1_MME = NULL;
  char                                   *mme_interface_name_for_S11 = NULL;
  char                                   *mme_ip_address_for_S11 = NULL;
  char                                   *sgw_ip_address_for_S11 = NULL;
  char                                    system_cmd[256];
  boolean_t                               swap = FALSE;

  config_init (&cfg);

  if (mme_config_p->config_file != NULL) {
    /*
     * Read the file. If there is an error, report it and exit.
     */
    if (!config_read_file (&cfg, mme_config_p->config_file)) {
      LOG_ERROR (LOG_CONFIG, ": %s:%d - %s\n", mme_config_p->config_file, config_error_line (&cfg), config_error_text (&cfg));
      config_destroy (&cfg);
      AssertFatal (1 == 0, "Failed to parse MME configuration file %s!\n", mme_config_p->config_file);
    }
  } else {
    LOG_ERROR (LOG_CONFIG, " No MME configuration file provided!\n");
    config_destroy (&cfg);
    AssertFatal (0, "No MME configuration file provided!\n");
  }

  setting_mme = config_lookup (&cfg, MME_CONFIG_STRING_MME_CONFIG);

  if (setting_mme != NULL) {
    // LOGGING setting
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_LOGGING);


    mme_config_p->log_config.udp_log_level      = LOG_LEVEL_ERROR;
    mme_config_p->log_config.gtpv1u_log_level   = LOG_LEVEL_ERROR;
    mme_config_p->log_config.gtpv2c_log_level   = LOG_LEVEL_ERROR;
    mme_config_p->log_config.sctp_log_level     = LOG_LEVEL_ERROR;
    mme_config_p->log_config.s1ap_log_level     = LOG_LEVEL_ERROR;
    mme_config_p->log_config.nas_log_level      = LOG_LEVEL_ERROR;
    mme_config_p->log_config.mme_app_log_level  = LOG_LEVEL_ERROR;
    mme_config_p->log_config.spgw_app_log_level = LOG_LEVEL_ERROR;
    mme_config_p->log_config.s11_log_level      = LOG_LEVEL_ERROR;
    mme_config_p->log_config.s6a_log_level      = LOG_LEVEL_ERROR;
    mme_config_p->log_config.util_log_level     = LOG_LEVEL_ERROR;
    mme_config_p->log_config.msc_log_level      = LOG_LEVEL_ERROR;
    mme_config_p->log_config.itti_log_level     = LOG_LEVEL_ERROR;

    if (setting != NULL) {
      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_OUTPUT, (const char **)&astring))
        mme_config_p->log_config.output = STRDUP_CHECK (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_SCTP_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.sctp_log_level = LOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_S1AP_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.s1ap_log_level = LOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_NAS_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.nas_log_level = LOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_MME_APP_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.mme_app_log_level = LOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_S6A_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.s6a_log_level = LOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_UTIL_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.util_log_level = LOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_MSC_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.msc_log_level = LOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, MME_CONFIG_STRING_ITTI_LOG_LEVEL, (const char **)&astring))
        mme_config_p->log_config.itti_log_level = LOG_LEVEL_STR2INT (astring);

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
    if ((config_setting_lookup_string (setting_mme, MME_CONFIG_STRING_REALM, (const char **)&astring))) {
      mme_config_p->realm = STRDUP_CHECK (astring);
      mme_config_p->realm_length = strlen (mme_config_p->realm);
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_MAXENB, &alongint))) {
      mme_config_p->max_eNBs = (uint32_t) alongint;
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_MAXUE, &alongint))) {
      mme_config_p->max_ues = (uint32_t) alongint;
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_RELATIVE_CAPACITY, &alongint))) {
      mme_config_p->relative_capacity = (uint8_t) alongint;
    }

    if ((config_setting_lookup_int (setting_mme, MME_CONFIG_STRING_STATISTIC_TIMER, &alongint))) {
      mme_config_p->mme_statistic_timer = (uint32_t) alongint;
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
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_INTERTASK_INTERFACE_QUEUE_SIZE, &alongint))) {
        mme_config_p->itti_config.queue_size = (uint32_t) alongint;
      }
    }
    // S6A SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_S6A_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_string (setting, MME_CONFIG_STRING_S6A_CONF_FILE_PATH, (const char **)&astring))) {
        if (astring != NULL)
          mme_config_p->s6a_config.conf_file = STRDUP_CHECK (astring);
      }

      if ((config_setting_lookup_string (setting, MME_CONFIG_STRING_S6A_HSS_HOSTNAME, (const char **)&astring))) {
        if (astring != NULL)
          mme_config_p->s6a_config.hss_host_name = STRDUP_CHECK (astring);
        else
          AssertFatal (1 == 0, "You have to provide a valid HSS hostname %s=...\n", MME_CONFIG_STRING_S6A_HSS_HOSTNAME);
      }
    }
    // SCTP SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_SCTP_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_SCTP_INSTREAMS, &alongint))) {
        mme_config_p->sctp_config.in_streams = (uint16_t) alongint;
      }

      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_SCTP_OUTSTREAMS, &alongint))) {
        mme_config_p->sctp_config.out_streams = (uint16_t) alongint;
      }
    }
    // S1AP SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_S1AP_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_S1AP_OUTCOME_TIMER, &alongint))) {
        mme_config_p->s1ap_config.outcome_drop_timer_sec = (uint8_t) alongint;
      }

      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_S1AP_PORT, &alongint))) {
        mme_config_p->s1ap_config.port_number = (uint16_t) alongint;
      }
    }
    // TAI list setting
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_TAI_LIST);
    if (setting != NULL) {
      num = config_setting_length (setting);

      if (mme_config_p->served_tai.nb_tai != num) {
        if (mme_config_p->served_tai.plmn_mcc != NULL)
          FREE_CHECK (mme_config_p->served_tai.plmn_mcc);

        if (mme_config_p->served_tai.plmn_mnc != NULL)
          FREE_CHECK (mme_config_p->served_tai.plmn_mnc);

        if (mme_config_p->served_tai.plmn_mnc_len != NULL)
          FREE_CHECK (mme_config_p->served_tai.plmn_mnc_len);

        if (mme_config_p->served_tai.tac != NULL)
          FREE_CHECK (mme_config_p->served_tai.tac);

        mme_config_p->served_tai.plmn_mcc = CALLOC_CHECK (num, sizeof (*mme_config_p->served_tai.plmn_mcc));
        mme_config_p->served_tai.plmn_mnc = CALLOC_CHECK (num, sizeof (*mme_config_p->served_tai.plmn_mnc));
        mme_config_p->served_tai.plmn_mnc_len = CALLOC_CHECK (num, sizeof (*mme_config_p->served_tai.plmn_mnc_len));
        mme_config_p->served_tai.tac = CALLOC_CHECK (num, sizeof (*mme_config_p->served_tai.tac));
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
            AssertFatal(0x0000 != mme_config_p->served_tai.tac[i], "Reserved TAC value 0x0000");
            AssertFatal(0xFFFE != mme_config_p->served_tai.tac[i], "Reserved TAC value 0xFFFE");
          }
        }
      }
      // sort TAI list
      n = mme_config_p->served_tai.nb_tai;
      do {
        stop_index = 0;
        for (i = 1; i < n; i++) {
          swap = FALSE;
          if (mme_config_p->served_tai.plmn_mcc[i-1] > mme_config_p->served_tai.plmn_mcc[i]) {
            swap = TRUE;
          } else if (mme_config_p->served_tai.plmn_mcc[i-1] == mme_config_p->served_tai.plmn_mcc[i]) {
            if (mme_config_p->served_tai.plmn_mnc[i-1] > mme_config_p->served_tai.plmn_mnc[i]) {
              swap = TRUE;
            } else  if (mme_config_p->served_tai.plmn_mnc[i-1] == mme_config_p->served_tai.plmn_mnc[i]) {
              if (mme_config_p->served_tai.tac[i-1] > mme_config_p->served_tai.tac[i]) {
                swap = TRUE;
              }
            }
          }
          if (TRUE == swap) {
            uint16_t swap;
            swap = mme_config_p->served_tai.plmn_mcc[i-1];
            mme_config_p->served_tai.plmn_mcc[i-1] = mme_config_p->served_tai.plmn_mcc[i];
            mme_config_p->served_tai.plmn_mcc[i]   = swap;

            swap = mme_config_p->served_tai.plmn_mnc[i-1];
            mme_config_p->served_tai.plmn_mnc[i-1] = mme_config_p->served_tai.plmn_mnc[i];
            mme_config_p->served_tai.plmn_mnc[i]   = swap;

            swap = mme_config_p->served_tai.tac[i-1];
            mme_config_p->served_tai.tac[i-1] = mme_config_p->served_tai.tac[i];
            mme_config_p->served_tai.tac[i]   = swap;

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
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_GUMMEI_CONFIG);

    if (setting != NULL) {
      subsetting = config_setting_get_member (setting, MME_CONFIG_STRING_MME_CODE);

      if (subsetting != NULL) {
        num = config_setting_length (subsetting);

        if (mme_config_p->gummei.nb_mmec != num) {
          if (mme_config_p->gummei.mmec != NULL) {
            FREE_CHECK (mme_config_p->gummei.mmec);
          }

          mme_config_p->gummei.mmec = CALLOC_CHECK (num, sizeof (*mme_config_p->gummei.mmec));
        }

        mme_config_p->gummei.nb_mmec = num;

        for (i = 0; i < num; i++) {
          mme_config_p->gummei.mmec[i] = config_setting_get_int_elem (subsetting, i);
        }
      }

      subsetting = config_setting_get_member (setting, MME_CONFIG_STRING_MME_GID);

      if (subsetting != NULL) {
        num = config_setting_length (subsetting);

        if (mme_config_p->gummei.nb_mme_gid != num) {
          if (mme_config_p->gummei.mme_gid != NULL) {
            FREE_CHECK (mme_config_p->gummei.mme_gid);
          }

          mme_config_p->gummei.mme_gid = CALLOC_CHECK (num, sizeof (*mme_config_p->gummei.mme_gid));
        }

        mme_config_p->gummei.nb_mme_gid = num;

        for (i = 0; i < num; i++) {
          mme_config_p->gummei.mme_gid[i] = config_setting_get_int_elem (subsetting, i);
        }
      }
    }
    // NETWORK INTERFACE SETTING
    setting = config_setting_get_member (setting_mme, MME_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_string (setting, MME_CONFIG_STRING_INTERFACE_NAME_FOR_S1_MME, (const char **)&mme_interface_name_for_S1_MME)
           && config_setting_lookup_string (setting, MME_CONFIG_STRING_IPV4_ADDRESS_FOR_S1_MME, (const char **)&mme_ip_address_for_S1_MME)
           && config_setting_lookup_string (setting, MME_CONFIG_STRING_INTERFACE_NAME_FOR_S11_MME, (const char **)&mme_interface_name_for_S11)
           && config_setting_lookup_string (setting, MME_CONFIG_STRING_IPV4_ADDRESS_FOR_S11_MME, (const char **)&mme_ip_address_for_S11)
          )
        ) {
        mme_config_p->ipv4.mme_interface_name_for_S1_MME = STRDUP_CHECK (mme_interface_name_for_S1_MME);
        cidr = STRDUP_CHECK (mme_ip_address_for_S1_MME);
        address = strtok (cidr, "/");
        IPV4_STR_ADDR_TO_INT_NWBO (address, mme_config_p->ipv4.mme_ip_address_for_S1_MME, "BAD IP ADDRESS FORMAT FOR MME S1_MME !\n")
          FREE_CHECK (cidr);
        mme_config_p->ipv4.mme_interface_name_for_S11 = STRDUP_CHECK (mme_interface_name_for_S11);
        cidr = STRDUP_CHECK (mme_ip_address_for_S11);
        address = strtok (cidr, "/");
        IPV4_STR_ADDR_TO_INT_NWBO (address, mme_config_p->ipv4.mme_ip_address_for_S11, "BAD IP ADDRESS FORMAT FOR MME S11 !\n")
          FREE_CHECK (cidr);
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
              mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA0;
            else if (strcmp ("EIA1", astring) == 0)
              mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA1;
            else if (strcmp ("EIA2", astring) == 0)
              mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA2;
            else if (strcmp ("EIA3", astring) == 0)
              mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA0;
            else if (strcmp ("EIA4", astring) == 0)
              mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA0;
            else if (strcmp ("EIA5", astring) == 0)
              mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA0;
            else if (strcmp ("EIA6", astring) == 0)
              mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA0;
            else if (strcmp ("EIA7", astring) == 0)
              mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA0;
          }

          for (i = num; i < 8; i++) {
            mme_config_p->nas_config.prefered_integrity_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EIA0;
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
              mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA0;
            else if (strcmp ("EEA1", astring) == 0)
              mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA1;
            else if (strcmp ("EEA2", astring) == 0)
              mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA2;
            else if (strcmp ("EEA3", astring) == 0)
              mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA0;
            else if (strcmp ("EEA4", astring) == 0)
              mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA0;
            else if (strcmp ("EEA5", astring) == 0)
              mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA0;
            else if (strcmp ("EEA6", astring) == 0)
              mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA0;
            else if (strcmp ("EEA7", astring) == 0)
              mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA0;
          }

          for (i = num; i < 8; i++) {
            mme_config_p->nas_config.prefered_ciphering_algorithm[i] = NAS_CONFIG_SECURITY_ALGORITHMS_EEA0;
          }
        }
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3402_TIMER, &alongint))) {
        mme_config_p->nas_config.t3402_min = (uint8_t) alongint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3412_TIMER, &alongint))) {
        mme_config_p->nas_config.t3412_min = (uint8_t) alongint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3485_TIMER, &alongint))) {
        mme_config_p->nas_config.t3485_sec = (uint8_t) alongint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3486_TIMER, &alongint))) {
        mme_config_p->nas_config.t3486_sec = (uint8_t) alongint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3489_TIMER, &alongint))) {
        mme_config_p->nas_config.t3489_sec = (uint8_t) alongint;
      }
      if ((config_setting_lookup_int (setting, MME_CONFIG_STRING_NAS_T3495_TIMER, &alongint))) {
        mme_config_p->nas_config.t3495_sec = (uint8_t) alongint;
      }
    }
  }

  setting = config_lookup (&cfg, SGW_CONFIG_STRING_SGW_CONFIG);

  if (setting != NULL) {
    // LOGGING setting
    setting = config_setting_get_member (setting, SGW_CONFIG_STRING_LOGGING);

    if (setting != NULL) {
      // already filled in MME section ?
      if (NULL == mme_config_p->log_config.output) {
        config_setting_lookup_string (setting, SGW_CONFIG_STRING_OUTPUT, (const char **)&astring);
        mme_config_p->log_config.output = STRDUP_CHECK (astring);
      }

      if (config_setting_lookup_string (setting, SGW_CONFIG_STRING_UDP_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.udp_log_level = LOG_LEVEL_STR2INT (astring);
      }

      if (config_setting_lookup_string (setting, SGW_CONFIG_STRING_SPGW_APP_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.spgw_app_log_level = LOG_LEVEL_STR2INT (astring);
      }

      if (config_setting_lookup_string (setting, SGW_CONFIG_STRING_GTPV1U_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.gtpv1u_log_level = LOG_LEVEL_STR2INT (astring);
      }

      if (config_setting_lookup_string (setting, SGW_CONFIG_STRING_GTPV2C_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.gtpv2c_log_level = LOG_LEVEL_STR2INT (astring);
      }

      // may be not present (may be filled in MME section)
      if (config_setting_lookup_string (setting, SGW_CONFIG_STRING_UTIL_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.util_log_level = LOG_LEVEL_STR2INT (astring);
      }

      // may be not present (may be filled in MME section)
      if (config_setting_lookup_string (setting, SGW_CONFIG_STRING_S11_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.s11_log_level = LOG_LEVEL_STR2INT (astring);
      }

      // may be not present (may be filled in MME section)
      if (config_setting_lookup_string (setting, SGW_CONFIG_STRING_MSC_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.msc_log_level = LOG_LEVEL_STR2INT (astring);
      }

      // may be not present (may be filled in MME section)
      if (config_setting_lookup_string (setting, SGW_CONFIG_STRING_ITTI_LOG_LEVEL, (const char **)&astring)) {
        mme_config_p->log_config.itti_log_level = LOG_LEVEL_STR2INT (astring);
      }
    }

    subsetting = config_setting_get_member (setting, SGW_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);

    if (subsetting != NULL) {
      if ((config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_SGW_IPV4_ADDRESS_FOR_S1U_S12_S4_UP, (const char **)&sgw_ip_address_for_S1u_S12_S4_up)
           && config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_SGW_IPV4_ADDRESS_FOR_S11, (const char **)&sgw_ip_address_for_S11)
           && config_setting_lookup_int (subsetting, SGW_CONFIG_STRING_SGW_PORT_FOR_S1U_S12_S4_UP, &alongint)
          )
        ) {
        cidr = STRDUP_CHECK (sgw_ip_address_for_S1u_S12_S4_up);
        address = strtok (cidr, "/");
        IPV4_STR_ADDR_TO_INT_NWBO (address, mme_config_p->ipv4.sgw_ip_address_for_S1u_S12_S4_up, "BAD IP ADDRESS FORMAT FOR SGW S1u_S12_S4 !\n")
          FREE_CHECK (cidr);
        cidr = STRDUP_CHECK (sgw_ip_address_for_S11);
        address = strtok (cidr, "/");
        IPV4_STR_ADDR_TO_INT_NWBO (address, mme_config_p->ipv4.sgw_ip_address_for_S11, "BAD IP ADDRESS FORMAT FOR SGW S11 !\n")
          FREE_CHECK (cidr);
        mme_config_p->gtpv1u_config.port_number = (uint16_t) alongint;
      }
    }
  }

  LOG_SET_CONFIG(&mme_config_p->log_config);
  return 0;
}

#define DISPLAY_ARRAY(size, format, args...)                        \
  do {                                                                \
    int i;                                                          \
    for (i = 0; i < size; i++) {                                    \
      fprintf(stdout, format, args);                              \
      if ((i != (size - 1)) && ((i + 1) % 10 == 0))               \
      {                                                           \
        fprintf(stdout, "\n        ");                          \
      }                                                           \
    }                                                               \
    if (i > 0)                                                      \
      fprintf(stdout, "\n");                                      \
  } while(0)

static void
config_display (
  mme_config_t * mme_config_p)
{
  int                                     j;

  LOG_INFO (LOG_CONFIG, "==== EURECOM %s v%s ====\n", PACKAGE_NAME, PACKAGE_VERSION);
  LOG_INFO (LOG_CONFIG, "Configuration:\n");
  LOG_INFO (LOG_CONFIG, "- File .................................: %s\n", mme_config_p->config_file);
  LOG_INFO (LOG_CONFIG, "- Realm ................................: %s\n", mme_config_p->realm);
  LOG_INFO (LOG_CONFIG, "- Max eNBs .............................: %u\n", mme_config_p->max_eNBs);
  LOG_INFO (LOG_CONFIG, "- Max UEs ..............................: %u\n", mme_config_p->max_ues);
  LOG_INFO (LOG_CONFIG, "- IMS voice over PS session in S1 ......: %s\n", mme_config_p->eps_network_feature_support.ims_voice_over_ps_session_in_s1 == 0 ? "FALSE" : "TRUE");
  LOG_INFO (LOG_CONFIG, "- Emergency bearer services in S1 mode .: %s\n", mme_config_p->eps_network_feature_support.emergency_bearer_services_in_s1_mode == 0 ? "FALSE" : "TRUE");
  LOG_INFO (LOG_CONFIG, "- Location services via epc ............: %s\n", mme_config_p->eps_network_feature_support.location_services_via_epc == 0 ? "FALSE" : "TRUE");
  LOG_INFO (LOG_CONFIG, "- Extended service request .............: %s\n", mme_config_p->eps_network_feature_support.extended_service_request == 0 ? "FALSE" : "TRUE");
  LOG_INFO (LOG_CONFIG, "- Unauth IMSI support ..................: %s\n", mme_config_p->unauthenticated_imsi_supported == 0 ? "FALSE" : "TRUE");
  LOG_INFO (LOG_CONFIG, "- Relative capa ........................: %u\n", mme_config_p->relative_capacity);
  LOG_INFO (LOG_CONFIG, "- Statistics timer .....................: %u (seconds)\n\n", mme_config_p->mme_statistic_timer);
  LOG_INFO (LOG_CONFIG, "- S1-U:\n");
  LOG_INFO (LOG_CONFIG, "    port number ......: %d\n", mme_config_p->gtpv1u_config.port_number);
  LOG_INFO (LOG_CONFIG, "- S1-MME:\n");
  LOG_INFO (LOG_CONFIG, "    port number ......: %d\n", mme_config_p->s1ap_config.port_number);
  LOG_INFO (LOG_CONFIG, "- IP:\n");
  LOG_INFO (LOG_CONFIG, "    s1-u ip ..........: %s\n", inet_ntoa (*((struct in_addr *)&mme_config_p->ipv4.sgw_ip_address_for_S1u_S12_S4_up)));
  LOG_INFO (LOG_CONFIG, "    s1-MME iface .....: %s\n", mme_config_p->ipv4.mme_interface_name_for_S1_MME);
  LOG_INFO (LOG_CONFIG, "    s1-MME ip ........: %s\n", inet_ntoa (*((struct in_addr *)&mme_config_p->ipv4.mme_ip_address_for_S1_MME)));
  LOG_INFO (LOG_CONFIG, "    s11 MME iface ....: %s\n", mme_config_p->ipv4.mme_interface_name_for_S11);
  LOG_INFO (LOG_CONFIG, "    s11 S-GW ip ......: %s\n", inet_ntoa (*((struct in_addr *)&mme_config_p->ipv4.mme_ip_address_for_S11)));
  LOG_INFO (LOG_CONFIG, "- ITTI:\n");
  LOG_INFO (LOG_CONFIG, "    queue size .......: %u (bytes)\n", mme_config_p->itti_config.queue_size);
  LOG_INFO (LOG_CONFIG, "    log file .........: %s\n", mme_config_p->itti_config.log_file);
  LOG_INFO (LOG_CONFIG, "- SCTP:\n");
  LOG_INFO (LOG_CONFIG, "    in streams .......: %u\n", mme_config_p->sctp_config.in_streams);
  LOG_INFO (LOG_CONFIG, "    out streams ......: %u\n", mme_config_p->sctp_config.out_streams);
  LOG_INFO (LOG_CONFIG, "- GUMMEI:\n");
  LOG_INFO (LOG_CONFIG, "    mme group ids ....:        \n");
  DISPLAY_ARRAY (mme_config_p->gummei.nb_mme_gid, "| %u \n", mme_config_p->gummei.mme_gid[i]);
  LOG_INFO (LOG_CONFIG, "    mme codes ........:        \n");
  DISPLAY_ARRAY (mme_config_p->gummei.nb_mmec, "| %u \n", mme_config_p->gummei.mmec[i]);
  LOG_INFO (LOG_CONFIG, "- TAIs : (mcc.mnc:tac)\n");
  switch (mme_config_p->served_tai.list_type) {
  case TRACKING_AREA_IDENTITY_LIST_TYPE_ONE_PLMN_CONSECUTIVE_TACS:
    LOG_INFO (LOG_CONFIG, "- TAI list type one PLMN consecutive TACs\n");
    break;
  case TRACKING_AREA_IDENTITY_LIST_TYPE_ONE_PLMN_NON_CONSECUTIVE_TACS:
    LOG_INFO (LOG_CONFIG, "- TAI list type one PLMN non consecutive TACs\n");
    break;
  case TRACKING_AREA_IDENTITY_LIST_TYPE_MANY_PLMNS:
    LOG_INFO (LOG_CONFIG, "- TAI list type multiple PLMNs\n");
    break;
  }
  for (j = 0; j < mme_config_p->served_tai.nb_tai; j++) {
    if (mme_config_p->served_tai.plmn_mnc_len[j] == 2) {
      LOG_INFO (LOG_CONFIG, "            %3u.%3u:%u\n",
          mme_config_p->served_tai.plmn_mcc[j], mme_config_p->served_tai.plmn_mnc[j], mme_config_p->served_tai.tac[j]);
    } else {
      LOG_INFO (LOG_CONFIG, "            %3u.%03u:%u\n",
          mme_config_p->served_tai.plmn_mcc[j], mme_config_p->served_tai.plmn_mnc[j], mme_config_p->served_tai.tac[j]);
    }
  }

  LOG_INFO (LOG_CONFIG, "- S6A:\n");
  LOG_INFO (LOG_CONFIG, "    conf file ........: %s\n", mme_config_p->s6a_config.conf_file);
  LOG_INFO (LOG_CONFIG, "- Logging:\n");
  LOG_INFO (LOG_CONFIG, "    Output ..............: %s\n", mme_config_p->log_config.output);
  LOG_INFO (LOG_CONFIG, "    UDP log level........: %s\n", LOG_LEVEL_INT2STR(mme_config_p->log_config.udp_log_level));
  LOG_INFO (LOG_CONFIG, "    GTPV1-U log level....: %s\n", LOG_LEVEL_INT2STR(mme_config_p->log_config.gtpv1u_log_level));
  LOG_INFO (LOG_CONFIG, "    GTPV2-C log level....: %s\n", LOG_LEVEL_INT2STR(mme_config_p->log_config.gtpv2c_log_level));
  LOG_INFO (LOG_CONFIG, "    SCTP log level.......: %s\n", LOG_LEVEL_INT2STR(mme_config_p->log_config.sctp_log_level));
  LOG_INFO (LOG_CONFIG, "    S1AP log level.......: %s\n", LOG_LEVEL_INT2STR(mme_config_p->log_config.s1ap_log_level));
  LOG_INFO (LOG_CONFIG, "    ASN1 Verbosity level : %d\n", LOG_LEVEL_INT2STR(mme_config_p->log_config.asn1_verbosity_level));
  LOG_INFO (LOG_CONFIG, "    NAS log level........: %s\n", LOG_LEVEL_INT2STR(mme_config_p->log_config.nas_log_level));
  LOG_INFO (LOG_CONFIG, "    MME_APP log level....: %s\n", LOG_LEVEL_INT2STR(mme_config_p->log_config.mme_app_log_level));
  LOG_INFO (LOG_CONFIG, "    S/P-GW APP log level.: %s\n", LOG_LEVEL_INT2STR(mme_config_p->log_config.spgw_app_log_level));
  LOG_INFO (LOG_CONFIG, "    S11 log level........: %s\n", LOG_LEVEL_INT2STR(mme_config_p->log_config.s11_log_level));
  LOG_INFO (LOG_CONFIG, "    S6a log level........: %s\n", LOG_LEVEL_INT2STR(mme_config_p->log_config.s6a_log_level));
  LOG_INFO (LOG_CONFIG, "    UTIL log level.......: %s\n", LOG_LEVEL_INT2STR(mme_config_p->log_config.util_log_level));
  LOG_INFO (LOG_CONFIG, "    MSC log level........: %s (MeSsage Chart)\n", LOG_LEVEL_INT2STR(mme_config_p->log_config.msc_log_level));
  LOG_INFO (LOG_CONFIG, "    ITTI log level.......: %s (InTer-Task Interface)\n", LOG_LEVEL_INT2STR(mme_config_p->log_config.itti_log_level));
}

static void
usage (
  void)
{
  LOG_INFO (LOG_CONFIG, "==== EURECOM %s version: %s ====\n", PACKAGE_NAME, PACKAGE_VERSION);
  LOG_INFO (LOG_CONFIG, "Please report any bug to: %s\n", PACKAGE_BUGREPORT);
  LOG_INFO (LOG_CONFIG, "Usage: oaisim_mme [options]\n");
  LOG_INFO (LOG_CONFIG, "Available options:\n");
  LOG_INFO (LOG_CONFIG, "-h      Print this help and return\n");
  LOG_INFO (LOG_CONFIG, "-c<path>\n");
  LOG_INFO (LOG_CONFIG, "        Set the configuration file for mme\n");
  LOG_INFO (LOG_CONFIG, "        See template in UTILS/CONF\n");
  LOG_INFO (LOG_CONFIG, "-K<file>\n");
  LOG_INFO (LOG_CONFIG, "        Output intertask messages to provided file\n");
  LOG_INFO (LOG_CONFIG, "-V      Print %s version and return\n", PACKAGE_NAME);
  LOG_INFO (LOG_CONFIG, "-v[1-2] Debug level:\n");
  LOG_INFO (LOG_CONFIG, "            1 -> ASN1 XER printf on and ASN1 debug off\n");
  LOG_INFO (LOG_CONFIG, "            2 -> ASN1 XER printf on and ASN1 debug on\n");
}

extern void
                                        nwGtpv1uDisplayBanner (
  void);

int
config_parse_opt_line (
  int argc,
  char *argv[],
  mme_config_t * mme_config_p)
{
  int                                     c;

  mme_config_init (mme_config_p);

  /*
   * Parsing command line
   */
  while ((c = getopt (argc, argv, "O:c:hi:K:v:V")) != -1) {
    switch (c) {
    case 'O':
    case 'c':{
        /*
         * Store the given configuration file. If no file is given,
         * * * * then the default values will be used.
         */
        int                                     config_file_len = 0;

        config_file_len = strlen (optarg);
        mme_config_p->config_file = MALLOC_CHECK (sizeof (char) * (config_file_len + 1));
        memcpy (mme_config_p->config_file, optarg, config_file_len);
        mme_config_p->config_file[config_file_len] = '\0';
        LOG_DEBUG (LOG_CONFIG, "%s mme_config.config_file %s\n", __FUNCTION__, mme_config_p->config_file);
      }
      break;

    case 'v':{
        mme_config_p->log_config.asn1_verbosity_level = atoi (optarg);
      }
      break;

    case 'V':{
        LOG_DEBUG (LOG_CONFIG, "==== EURECOM %s v%s ====" "Please report any bug to: %s\n", PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_BUGREPORT);
        exit (0);
        nwGtpv1uDisplayBanner ();
      }
      break;

    case 'K':
      mme_config_p->itti_config.log_file = STRDUP_CHECK (optarg);
      LOG_DEBUG (LOG_CONFIG, "%s mme_config.itti_config.log_file %s\n", __FUNCTION__, mme_config_p->itti_config.log_file);
      break;

    case 'h':                  /* Fall through */
    default:
      usage ();
      exit (0);
    }
  }

  /*
   * Parse the configuration file using libconfig
   */
  if (config_parse_file (mme_config_p) != 0) {
    return -1;
  }

  /*
   * Display the configuration
   */
  config_display (mme_config_p);
  return 0;
}

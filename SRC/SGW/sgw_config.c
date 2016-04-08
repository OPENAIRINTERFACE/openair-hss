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

/*! \file spgw_config.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#define PGW
#define PGW_CONFIG_C

#include <string.h>
#include <libconfig.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <netdb.h>

#include "assertions.h"
#include "dynamic_memory_check.h"
#include "log.h"
#include "intertask_interface.h"
#include "sgw_config.h"

#ifdef LIBCONFIG_LONG
#  define libconfig_int long
#else
#  define libconfig_int int
#endif

#ifdef LIBCONFIG_LONG
#  define libconfig_int long
#else
#  define libconfig_int int
#endif


//------------------------------------------------------------------------------
static int sgw_system (
  bstring command_pP,
  bool is_abort_on_errorP,
  const char *const file_nameP,
  const int line_numberP)
{
  int                                     ret = RETURNerror;

  if (command_pP) {
    OAILOG_INFO (LOG_SPGW_APP, "system command: %s\n", bdata(command_pP));
    // The value returned is -1 on error (e.g., fork(2) failed), and the return status of the command otherwise.
    // This latter return status is in the format specified in wait(2).  Thus, the exit code
    // of the command will be WEXITSTATUS(status).
    // In case /bin/sh could not be executed, the exit status will be that of a command that does exit(127).
    ret = system (bdata(command_pP));
    if (ret != 0) {
      OAILOG_ERROR (LOG_SPGW_APP, "ERROR in system command %s: %d at %s:%u\n", bdata(command_pP), ret, file_nameP, line_numberP);

      if (is_abort_on_errorP) {
        exit (-1);              // may be not exit
      }
    }
  }
  return ret;
}

//------------------------------------------------------------------------------
void sgw_config_init (sgw_config_t * config_pP)
{
  memset(config_pP, 0, sizeof(*config_pP));
  pthread_rwlock_init (&config_pP->rw_lock, NULL);
}
//------------------------------------------------------------------------------
int sgw_config_process (sgw_config_t * config_pP)
{
  bstring                                 system_cmd = NULL;
  struct in_addr                          inaddr = {0};
  int                                     ret = RETURNok;

  inaddr.s_addr = config_pP->ipv4.S1u_S12_S4_up;

  if (0 != biseqcstrcaseless (config_pP->ipv4.if_name_S1u_S12_S4_up, "lo")) {
    config_pP->local_to_eNB = true;
  } else {
    config_pP->local_to_eNB = false;

    system_cmd = bfromcstr("");
    bassignformat (system_cmd, "modprobe xt_GTPUSP gtpu_enb_port=2152 gtpu_sgw_port=%u sgw_addr=\"%s\" ",
        config_pP->udp_port_S1u_S12_S4_up, inet_ntoa (inaddr));
    sgw_system (system_cmd, SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
    bdestroy(system_cmd);
  }

  if (config_pP->local_to_eNB == true) {
    system_cmd = bfromcstr("");
    bassignformat (system_cmd, "iptables -t filter -I INPUT -i lo -d %s --protocol sctp -j DROP", inet_ntoa (inaddr));
    sgw_system (system_cmd, SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);

    bassignformat (system_cmd, "iptables -t filter -I INPUT -i lo -s %s --protocol sctp -j DROP", inet_ntoa (inaddr));
    sgw_system (system_cmd, SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);

    bassignformat (system_cmd, "modprobe xt_GTPUSP  gtpu_enb_port=2153 gtpu_sgw_port=%u sgw_addr=\"%s\" ",
        config_pP->udp_port_S1u_S12_S4_up, inet_ntoa (inaddr));
    sgw_system (system_cmd, SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
    bdestroy(system_cmd);
  }

  system_cmd = bfromcstr("echo 0 > /proc/sys/net/ipv4/conf/all/send_redirects");
  sgw_system (system_cmd, SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  bdestroy(system_cmd);

  return ret;
}

//------------------------------------------------------------------------------
int sgw_config_parse_file (sgw_config_t * config_pP)

{
  config_t                                cfg = {0};
  config_setting_t                       *setting_sgw = NULL;
  char                                   *sgw_if_name_S1u_S12_S4_up = NULL;
  char                                   *S1u_S12_S4_up = NULL;
  char                                   *sgw_if_name_S5_S8_up = NULL;
  char                                   *S5_S8_up = NULL;
  char                                   *sgw_if_name_S11 = NULL;
  char                                   *S11 = NULL;
  libconfig_int                           sgw_udp_port_S1u_S12_S4_up = 2152;
  config_setting_t                       *subsetting = NULL;
  const char                             *astring = NULL;
  bstring                                 address = NULL;
  bstring                                 cidr = NULL;
  bstring                                 mask = NULL;
  struct in_addr                          in_addr_var = {0};

  config_init (&cfg);

  if (config_pP->config_file) {
    /*
     * Read the file. If there is an error, report it and exit.
     */
    if (!config_read_file (&cfg, bdata(config_pP->config_file))) {
      OAILOG_ERROR (LOG_SPGW_APP, "%s:%d - %s\n", bdata(config_pP->config_file), config_error_line (&cfg), config_error_text (&cfg));
      config_destroy (&cfg);
      AssertFatal (1 == 0, "Failed to parse SP-GW configuration file %s!\n", bdata(config_pP->config_file));
    }
  } else {
    OAILOG_ERROR (LOG_SPGW_APP, "No SP-GW configuration file provided!\n");
    config_destroy (&cfg);
    AssertFatal (0, "No SP-GW configuration file provided!\n");
  }

  OAILOG_INFO (LOG_SPGW_APP, "Parsing configuration file provided %s\n", bdata(config_pP->config_file));
  setting_sgw = config_lookup (&cfg, SGW_CONFIG_STRING_SGW_CONFIG);

  if (setting_sgw) {

    // LOGGING setting
    subsetting = config_setting_get_member (setting_sgw, SGW_CONFIG_STRING_LOGGING);

    config_pP->log_config.udp_log_level      = MAX_LOG_LEVEL; // Means invalid
    config_pP->log_config.gtpv1u_log_level   = MAX_LOG_LEVEL; // will not overwrite existing log levels if MME and S-GW bundled in same executable
    config_pP->log_config.gtpv2c_log_level   = MAX_LOG_LEVEL;
    config_pP->log_config.sctp_log_level     = MAX_LOG_LEVEL;
    config_pP->log_config.s1ap_log_level     = MAX_LOG_LEVEL;
    config_pP->log_config.nas_log_level      = MAX_LOG_LEVEL;
    config_pP->log_config.mme_app_log_level  = MAX_LOG_LEVEL;
    config_pP->log_config.spgw_app_log_level = MAX_LOG_LEVEL;
    config_pP->log_config.s11_log_level      = MAX_LOG_LEVEL;
    config_pP->log_config.s6a_log_level      = MAX_LOG_LEVEL;
    config_pP->log_config.util_log_level     = MAX_LOG_LEVEL;
    config_pP->log_config.msc_log_level      = MAX_LOG_LEVEL;
    config_pP->log_config.itti_log_level     = MAX_LOG_LEVEL;
    if (subsetting) {
      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_COLOR, (const char **)&astring)) {
        if (!strcasecmp("true", astring)) config_pP->log_config.color = true;
        else config_pP->log_config.color = false;
      }
      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_UDP_LOG_LEVEL, (const char **)&astring)) {
        config_pP->log_config.udp_log_level = OAILOG_LEVEL_STR2INT (astring);
      }

      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_GTPV1U_LOG_LEVEL, (const char **)&astring)) {
        config_pP->log_config.gtpv1u_log_level = OAILOG_LEVEL_STR2INT (astring);
      }

      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_GTPV2C_LOG_LEVEL, (const char **)&astring)) {
        config_pP->log_config.gtpv2c_log_level = OAILOG_LEVEL_STR2INT (astring);
      }

      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_SPGW_APP_LOG_LEVEL, (const char **)&astring)) {
        config_pP->log_config.spgw_app_log_level = OAILOG_LEVEL_STR2INT (astring);
      }

      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_S11_LOG_LEVEL, (const char **)&astring)) {
        config_pP->log_config.s11_log_level = OAILOG_LEVEL_STR2INT (astring);
      }

      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_UTIL_LOG_LEVEL, (const char **)&astring)) {
        config_pP->log_config.util_log_level = OAILOG_LEVEL_STR2INT (astring);
      }

      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_MSC_LOG_LEVEL, (const char **)&astring)) {
        config_pP->log_config.msc_log_level = OAILOG_LEVEL_STR2INT (astring);
      }

      if (config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_ITTI_LOG_LEVEL, (const char **)&astring)) {
        config_pP->log_config.itti_log_level = OAILOG_LEVEL_STR2INT (astring);
      }
    }
    OAILOG_SET_CONFIG(&config_pP->log_config);

    subsetting = config_setting_get_member (setting_sgw, SGW_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);

    if (subsetting) {
      if ((config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP, (const char **)&sgw_if_name_S1u_S12_S4_up)
           && config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_SGW_IPV4_ADDRESS_FOR_S1U_S12_S4_UP, (const char **)&S1u_S12_S4_up)
           && config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_SGW_INTERFACE_NAME_FOR_S5_S8_UP, (const char **)&sgw_if_name_S5_S8_up)
           && config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_SGW_IPV4_ADDRESS_FOR_S5_S8_UP, (const char **)&S5_S8_up)
           && config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_SGW_INTERFACE_NAME_FOR_S11, (const char **)&sgw_if_name_S11)
           && config_setting_lookup_string (subsetting, SGW_CONFIG_STRING_SGW_IPV4_ADDRESS_FOR_S11, (const char **)&S11)
          )
        ) {
        config_pP->ipv4.if_name_S1u_S12_S4_up = bfromcstr (sgw_if_name_S1u_S12_S4_up);
        cidr = bfromcstr (S1u_S12_S4_up);
        struct bstrList *list = bsplit (cidr, '/');
        AssertFatal(2 == list->qty, "Bad CIDR address %s", bdata(cidr));
        address = list->entry[0];
        mask    = list->entry[1];
        IPV4_STR_ADDR_TO_INT_NWBO (bdata(address), config_pP->ipv4.S1u_S12_S4_up, "BAD IP ADDRESS FORMAT FOR S1u_S12_S4 !\n");
        config_pP->ipv4.netmask_S1u_S12_S4_up = atoi ((const char*)mask->data);
        bstrListDestroy(list);
        in_addr_var.s_addr = config_pP->ipv4.S1u_S12_S4_up;
        OAILOG_INFO (LOG_SPGW_APP, "Parsing configuration file found S1u_S12_S4_up: %s/%d on %s\n",
                       inet_ntoa (in_addr_var), config_pP->ipv4.netmask_S1u_S12_S4_up, bdata(config_pP->ipv4.if_name_S1u_S12_S4_up));

        config_pP->ipv4.if_name_S5_S8_up = bfromcstr (sgw_if_name_S5_S8_up);
        cidr = bfromcstr (S5_S8_up);
        list = bsplit (cidr, '/');
        AssertFatal(2 == list->qty, "Bad CIDR address %s", bdata(cidr));
        address = list->entry[0];
        mask    = list->entry[1];
        IPV4_STR_ADDR_TO_INT_NWBO (bdata(address), config_pP->ipv4.S5_S8_up, "BAD IP ADDRESS FORMAT FOR S5_S8 !\n");
        config_pP->ipv4.netmask_S5_S8_up = atoi ((const char*)mask->data);
        bstrListDestroy(list);
        in_addr_var.s_addr = config_pP->ipv4.S5_S8_up;
        OAILOG_INFO (LOG_SPGW_APP, "Parsing configuration file found S5_S8_up: %s/%d on %s\n",
                       inet_ntoa (in_addr_var), config_pP->ipv4.netmask_S5_S8_up, bdata(config_pP->ipv4.if_name_S5_S8_up));

        config_pP->ipv4.if_name_S11 = bfromcstr (sgw_if_name_S11);
        cidr = bfromcstr (S11);
        list = bsplit (cidr, '/');
        AssertFatal(2 == list->qty, "Bad CIDR address %s", bdata(cidr));
        address = list->entry[0];
        mask    = list->entry[1];
        IPV4_STR_ADDR_TO_INT_NWBO (bdata(address), config_pP->ipv4.S11, "BAD IP ADDRESS FORMAT FOR S11 !\n");
        config_pP->ipv4.netmask_S11 = atoi ((const char*)mask->data);
        bstrListDestroy(list);
        in_addr_var.s_addr = config_pP->ipv4.S11;
        OAILOG_INFO (LOG_SPGW_APP, "Parsing configuration file found S11: %s/%d on %s\n",
            inet_ntoa (in_addr_var), config_pP->ipv4.netmask_S11, bdata(config_pP->ipv4.if_name_S11));
      }

      if (config_setting_lookup_int (subsetting, SGW_CONFIG_STRING_SGW_PORT_FOR_S1U_S12_S4_UP, &sgw_udp_port_S1u_S12_S4_up)
        ) {
        config_pP->udp_port_S1u_S12_S4_up = sgw_udp_port_S1u_S12_S4_up;
      } else {
        config_pP->udp_port_S1u_S12_S4_up = sgw_udp_port_S1u_S12_S4_up;
      }
    }
  }

  config_destroy (&cfg);
  OAILOG_SET_CONFIG(&config_pP->log_config);
  return RETURNok;
}

//------------------------------------------------------------------------------
void sgw_config_display (sgw_config_t * config_p)
{

  OAILOG_INFO (LOG_CONFIG, "==== EURECOM %s v%s ====\n", PACKAGE_NAME, PACKAGE_VERSION);
  OAILOG_INFO (LOG_CONFIG, "Configuration:\n");
  OAILOG_INFO (LOG_CONFIG, "- File .................................: %s\n", bdata(config_p->config_file));

  OAILOG_INFO (LOG_CONFIG, "- S1-U:\n");
  OAILOG_INFO (LOG_CONFIG, "    port number ......: %d\n", config_p->udp_port_S1u_S12_S4_up);
  OAILOG_INFO (LOG_CONFIG, "    S1u_S12_S4 iface .....: %s\n", bdata(config_p->ipv4.if_name_S1u_S12_S4_up));
  OAILOG_INFO (LOG_CONFIG, "    S1u_S12_S4 ip ........: %s/%u\n", inet_ntoa (*((struct in_addr *)&config_p->ipv4.S1u_S12_S4_up)), config_p->ipv4.netmask_S1u_S12_S4_up);
  OAILOG_INFO (LOG_CONFIG, "- S5-S8:\n");
  OAILOG_INFO (LOG_CONFIG, "    S5_S8 iface ..........: %s\n", bdata(config_p->ipv4.if_name_S5_S8_up));
  OAILOG_INFO (LOG_CONFIG, "    S5_S8 ip .............: %s/%u\n", inet_ntoa (*((struct in_addr *)&config_p->ipv4.S5_S8_up)), config_p->ipv4.netmask_S5_S8_up);
  OAILOG_INFO (LOG_CONFIG, "- S11:\n");
  OAILOG_INFO (LOG_CONFIG, "    S11 iface ............: %s\n", bdata(config_p->ipv4.if_name_S11));
  OAILOG_INFO (LOG_CONFIG, "    S11 ip ...............: %s/%u\n", inet_ntoa (*((struct in_addr *)&config_p->ipv4.S11)), config_p->ipv4.netmask_S11);
  OAILOG_INFO (LOG_CONFIG, "- ITTI:\n");
  OAILOG_INFO (LOG_CONFIG, "    queue size .......: %u (bytes)\n", config_p->itti_config.queue_size);
  OAILOG_INFO (LOG_CONFIG, "    log file .........: %s\n", bdata(config_p->itti_config.log_file));

  OAILOG_INFO (LOG_CONFIG, "- Logging:\n");
  OAILOG_INFO (LOG_CONFIG, "    Output ..............: %s\n", config_p->log_config.output);
  OAILOG_INFO (LOG_CONFIG, "    UDP log level........: %s\n", OAILOG_LEVEL_INT2STR(config_p->log_config.udp_log_level));
  OAILOG_INFO (LOG_CONFIG, "    GTPV1-U log level....: %s\n", OAILOG_LEVEL_INT2STR(config_p->log_config.gtpv1u_log_level));
  OAILOG_INFO (LOG_CONFIG, "    GTPV2-C log level....: %s\n", OAILOG_LEVEL_INT2STR(config_p->log_config.gtpv2c_log_level));
  OAILOG_INFO (LOG_CONFIG, "    S/P-GW APP log level.: %s\n", OAILOG_LEVEL_INT2STR(config_p->log_config.spgw_app_log_level));
  OAILOG_INFO (LOG_CONFIG, "    S11 log level........: %s\n", OAILOG_LEVEL_INT2STR(config_p->log_config.s11_log_level));
  OAILOG_INFO (LOG_CONFIG, "    UTIL log level.......: %s\n", OAILOG_LEVEL_INT2STR(config_p->log_config.util_log_level));
  OAILOG_INFO (LOG_CONFIG, "    MSC log level........: %s (MeSsage Chart)\n", OAILOG_LEVEL_INT2STR(config_p->log_config.msc_log_level));
  OAILOG_INFO (LOG_CONFIG, "    ITTI log level.......: %s (InTer-Task Interface)\n", OAILOG_LEVEL_INT2STR(config_p->log_config.itti_log_level));
}

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
#include "pgw_config.h"

#ifdef LIBCONFIG_LONG
#  define libconfig_int long
#else
#  define libconfig_int int
#endif

#define SYSTEM_CMD_MAX_STR_SIZE 512

//------------------------------------------------------------------------------
static int pgw_system (
  bstring command_pP,
  bool    is_abort_on_errorP,
  const char *const file_nameP,
  const int line_numberP)
{
  int                                     ret = RETURNerror;

  if (command_pP) {
#if DISABLE_EXECUTE_SHELL_COMMAND
    ret = 0;
    OAILOG_INFO (LOG_SPGW_APP, "Not executing system command: %s\n", bdata(command_pP));
#else
    OAILOG_INFO (LOG_SPGW_APP, "system command: %s\n", bdata(command_pP));
    ret = system (bdata(command_pP));

    if (ret != 0) {
      OAILOG_ERROR (LOG_SPGW_APP, "ERROR in system command %s: %d at %s:%u\n", bdata(command_pP), ret, file_nameP, line_numberP);

      if (is_abort_on_errorP) {
        exit (-1);              // may be not exit
      }
    }
#endif
  }
  return ret;
}

//------------------------------------------------------------------------------
void pgw_config_init (pgw_config_t * config_pP)
{
  memset ((char *)config_pP, 0, sizeof (*config_pP));
  pthread_rwlock_init (&config_pP->rw_lock, NULL);
  STAILQ_INIT (&config_pP->ipv4_pool_list);
}

//------------------------------------------------------------------------------
int pgw_config_parse_file (pgw_config_t * config_pP, sgw_config_t * sgw_config_pP)
{
  config_t                                cfg = {0};
  config_setting_t                       *setting_pgw = NULL;
  config_setting_t                       *subsetting = NULL;
  config_setting_t                       *sub2setting = NULL;
  char                                   *if_S5_S8 = NULL;
  char                                   *S5_S8 = NULL;
  char                                   *if_SGI = NULL;
  char                                   *SGI = NULL;
  char                                   *masquerade_SGI = NULL;
  char                                   *default_dns = NULL;
  char                                   *default_dns_sec = NULL;
  const char                             *astring = NULL;
  bstring                                 address = NULL;
  bstring                                 cidr = NULL;
  bstring                                 mask = NULL;
  int                                     num = 0;
  int                                     i = 0;
  int                                     prefix_mask = 0;
  uint64_t                                counter64 = 0;
  unsigned char                           buf_in_addr[sizeof (struct in_addr)];
  struct in_addr                          addr_start,
                                          in_addr_var;
  struct in_addr                          addr_mask;
  conf_ipv4_list_elm_t                   *ip4_ref = NULL;
  bstring                                 system_cmd = NULL;
  libconfig_int                           mtu = 0;


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

  system_cmd = bfromcstr("");

  setting_pgw = config_lookup (&cfg, PGW_CONFIG_STRING_PGW_CONFIG);

  if (setting_pgw) {
    subsetting = config_setting_get_member (setting_pgw, PGW_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);

    if (subsetting) {
      if ((config_setting_lookup_string (subsetting, PGW_CONFIG_STRING_PGW_INTERFACE_NAME_FOR_S5_S8, (const char **)&if_S5_S8)
           && config_setting_lookup_string (subsetting, PGW_CONFIG_STRING_PGW_IPV4_ADDRESS_FOR_S5_S8, (const char **)&S5_S8)
           && config_setting_lookup_string (subsetting, PGW_CONFIG_STRING_PGW_INTERFACE_NAME_FOR_SGI, (const char **)&if_SGI)
           && config_setting_lookup_string (subsetting, PGW_CONFIG_STRING_PGW_IPV4_ADDR_FOR_SGI, (const char **)&SGI)
           && config_setting_lookup_string (subsetting, PGW_CONFIG_STRING_PGW_MASQUERADE_SGI, (const char **)&masquerade_SGI)
          )
        ) {
        config_pP->ipv4.if_name_S5_S8 = bfromcstr(if_S5_S8);
        cidr = bfromcstr (S5_S8);
        struct bstrList *list = bsplit (cidr, '/');
        AssertFatal(2 == list->qty, "Bad CIDR address %s", bdata(cidr));
        address = list->entry[0];
        mask    = list->entry[1];
        IPV4_STR_ADDR_TO_INT_NWBO (bdata(address), config_pP->ipv4.S5_S8, "BAD IP ADDRESS FORMAT FOR S5_S8 !\n");
        config_pP->ipv4.netmask_S5_S8 = atoi ((const char *)mask->data);
        bstrListDestroy(list);
        bdestroy(cidr);
        in_addr_var.s_addr = config_pP->ipv4.S5_S8;
        OAILOG_INFO (LOG_SPGW_APP, "Parsing configuration file found S5_S8: %s/%d on %s\n",
            inet_ntoa (in_addr_var), config_pP->ipv4.netmask_S5_S8, bdata(config_pP->ipv4.if_name_S5_S8));


        config_pP->ipv4.if_name_SGI = bfromcstr (if_SGI);
        cidr = bfromcstr (SGI);
        list = bsplit (cidr, '/');
        AssertFatal(2 == list->qty, "Bad CIDR address %s", bdata(cidr));
        address = list->entry[0];
        mask    = list->entry[1];
        IPV4_STR_ADDR_TO_INT_NWBO (bdata(address), config_pP->ipv4.SGI, "BAD IP ADDRESS FORMAT FOR SGI !\n");
        config_pP->ipv4.netmask_SGI = atoi ((const char *)mask->data);
        bstrListDestroy(list);
        bdestroy(cidr);
        in_addr_var.s_addr = config_pP->ipv4.SGI;
        OAILOG_INFO (LOG_SPGW_APP, "Parsing configuration file found SGI: %s/%d on %s\n",
            inet_ntoa (in_addr_var), config_pP->ipv4.netmask_SGI, bdata(config_pP->ipv4.if_name_SGI));

        if (strcasecmp (masquerade_SGI, "yes") == 0) {
          config_pP->masquerade_SGI = true;
        } else {
          config_pP->masquerade_SGI = false;
          OAILOG_INFO (LOG_SPGW_APP, "No masquerading for SGI\n");
        }
      } else {
        OAILOG_WARNING (LOG_SPGW_APP, "CONFIG P-GW / NETWORK INTERFACES parsing failed\n");
      }
    } else {
      OAILOG_WARNING (LOG_SPGW_APP, "CONFIG P-GW / NETWORK INTERFACES not found\n");
    }

    //!!!------------------------------------!!!
    bassignformat (system_cmd, "ethtool -K %s tso off ", config_pP->ipv4.if_name_SGI);
    pgw_system (system_cmd, SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
    btrunc(system_cmd, 0);
    bassignformat (system_cmd, "ethtool -K %s gso off", config_pP->ipv4.if_name_SGI);
    pgw_system (system_cmd, SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
    btrunc(system_cmd, 0);
    bassignformat (system_cmd, "ethtool -K %s gro off", config_pP->ipv4.if_name_SGI);
    pgw_system (system_cmd, SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
    btrunc(system_cmd, 0);

    //!!!------------------------------------!!!
    subsetting = config_setting_get_member (setting_pgw, PGW_CONFIG_STRING_IP_ADDRESS_POOL);

    if (subsetting) {
      sub2setting = config_setting_get_member (subsetting, PGW_CONFIG_STRING_IPV4_ADDRESS_LIST);

      if (sub2setting) {
        num = config_setting_length (sub2setting);

        for (i = 0; i < num; i++) {
          astring = config_setting_get_string_elem (sub2setting, i);

          if (astring) {
            cidr = bfromcstr (astring);
            AssertFatal(BSTR_OK == btrimws(cidr), "Error in PGW_CONFIG_STRING_IPV4_ADDRESS_LIST %s", astring);
            struct bstrList *list = bsplit (cidr, PGW_CONFIG_STRING_IPV4_PREFIX_DELIMITER);
            AssertFatal(2 == list->qty, "Bad CIDR address %s", bdata(cidr));

            address = list->entry[0];
            mask    = list->entry[1];

            if (inet_pton (AF_INET, bdata(address), buf_in_addr) == 1) {
              memcpy (&addr_start, buf_in_addr, sizeof (struct in_addr));
              // valid address
              prefix_mask = atoi ((const char *)mask->data);

              bassignformat (system_cmd,
                  "iptables -I PREROUTING -t mangle -i %s -d %s/%s ! --protocol sctp   -j CONNMARK --restore-mark",
                  bdata(config_pP->ipv4.if_name_SGI), bdata(address), bdata(mask));
              pgw_system (system_cmd, PGW_ABORT_ON_ERROR, __FILE__, __LINE__);
              btrunc(system_cmd, 0);


              bassignformat (system_cmd,
                  "iptables -I OUTPUT -t mangle -s %s/%s -m mark  ! --mark 0 -j CONNMARK --save-mark",
                  bdata(address), bdata(mask));
              pgw_system (system_cmd, PGW_ABORT_ON_ERROR, __FILE__, __LINE__);
              btrunc(system_cmd, 0);

              bassignformat (system_cmd,
                  "ip route add  %s/%s dev %s",
                  bdata(address), bdata(mask), bdata(sgw_config_pP->ipv4.if_name_S1u_S12_S4_up));
              pgw_system (system_cmd, SPGW_WARN_ON_ERROR, __FILE__, __LINE__);

              if ((prefix_mask >= 2) && (prefix_mask < 32)) {
                memcpy (&addr_start, buf_in_addr, sizeof (struct in_addr));
                memcpy (&addr_mask, buf_in_addr, sizeof (struct in_addr));
                addr_mask.s_addr = addr_mask.s_addr & htonl (0xFFFFFFFF << (32 - prefix_mask));

                if (memcmp (&addr_start, &addr_mask, sizeof (struct in_addr)) ) {
                  AssertFatal (0, "BAD IPV4 ADDR CONFIG/MASK PAIRING %s/%d addr %X mask %X\n",
                      bdata(address), prefix_mask, addr_start.s_addr, addr_mask.s_addr);
                }

                counter64 = 0x00000000FFFFFFFF >> prefix_mask;  // address Prefix_mask/0..0 not valid
                counter64 = counter64 - 2;

                do {
                  addr_start.s_addr = addr_start.s_addr + htonl (2);
                  ip4_ref = calloc (1, sizeof (conf_ipv4_list_elm_t));
                  ip4_ref->addr = addr_start;
                  STAILQ_INSERT_TAIL (&config_pP->ipv4_pool_list, ip4_ref, ipv4_entries);
                  counter64 = counter64 - 1;
                } while (counter64 > 0);

                //---------------
                if (config_pP->masquerade_SGI) {
                  in_addr_var.s_addr = config_pP->ipv4.SGI;

                  bassignformat (system_cmd,
                                "iptables -t nat -I POSTROUTING -s %s/%s -o %s  ! --protocol sctp -j SNAT --to-source %s",
                                bdata(address), bdata(mask),
                                bdata(config_pP->ipv4.if_name_SGI),
                                inet_ntoa (in_addr_var));
                    pgw_system (system_cmd, PGW_ABORT_ON_ERROR, __FILE__, __LINE__);
                    btrunc(system_cmd, 0);

                }
              } else {
                OAILOG_ERROR (LOG_SPGW_APP, "CONFIG POOL ADDR IPV4: BAD MASQ: %d\n", prefix_mask);
              }
            }
          }
        }
      } else {
        OAILOG_WARNING (LOG_SPGW_APP, "CONFIG POOL ADDR IPV4: NO IPV4 ADDRESS FOUND\n");
      }

      if (config_setting_lookup_string (setting_pgw, PGW_CONFIG_STRING_DEFAULT_DNS_IPV4_ADDRESS, (const char **)&default_dns)
          && config_setting_lookup_string (setting_pgw, PGW_CONFIG_STRING_DEFAULT_DNS_SEC_IPV4_ADDRESS, (const char **)&default_dns_sec)) {
        config_pP->ipv4.if_name_S5_S8 = bfromcstr (if_S5_S8);
        IPV4_STR_ADDR_TO_INT_NWBO (default_dns, config_pP->ipv4.default_dns, "BAD IPv4 ADDRESS FORMAT FOR DEFAULT DNS !\n");
        IPV4_STR_ADDR_TO_INT_NWBO (default_dns_sec, config_pP->ipv4.default_dns_sec, "BAD IPv4 ADDRESS FORMAT FOR DEFAULT DNS SEC!\n");
        OAILOG_INFO (LOG_SPGW_APP, "Parsing configuration file default primary DNS IPv4 address: %x\n", config_pP->ipv4.default_dns);
        OAILOG_INFO (LOG_SPGW_APP, "Parsing configuration file default secondary DNS IPv4 address: %x\n", config_pP->ipv4.default_dns_sec);
      } else {
        OAILOG_WARNING (LOG_SPGW_APP, "NO DNS CONFIGURATION FOUND\n");
      }
    }
    if (config_setting_lookup_string (setting_pgw, PGW_CONFIG_STRING_NAS_FORCE_PUSH_PCO, (const char **)&astring)) {
      if (strcasecmp (astring, "yes") == 0) {
        config_pP->force_push_pco = true;
        OAILOG_INFO (LOG_SPGW_APP, "Protocol configuration options: push MTU, push DNS, IP address allocation via NAS signalling\n");
      } else {
        config_pP->force_push_pco = false;
      }
    }
    if (config_setting_lookup_int (setting_pgw, PGW_CONFIG_STRING_UE_MTU, (const char **)&mtu)) {
      config_pP->ue_mtu = mtu;
    } else {
      config_pP->ue_mtu = 1463;
    }
    OAILOG_INFO (LOG_SPGW_APP, "UE MTU : %u\n", config_pP->ue_mtu);
  } else {
    OAILOG_WARNING (LOG_SPGW_APP, "CONFIG P-GW not found\n");
  }
  bdestroy(system_cmd);
  config_destroy (&cfg);
  return RETURNok;
}

//------------------------------------------------------------------------------
void pgw_config_display (pgw_config_t * config_p)
{
  OAILOG_INFO (LOG_CONFIG, "==== EURECOM %s v%s ====\n", PACKAGE_NAME, PACKAGE_VERSION);
  OAILOG_INFO (LOG_CONFIG, "Configuration:\n");
  OAILOG_INFO (LOG_CONFIG, "- File .................................: %s\n", bdata(config_p->config_file));

  OAILOG_INFO (LOG_CONFIG, "- S5-S8:\n");
  OAILOG_INFO (LOG_CONFIG, "    S5_S8 iface ..........: %s\n", bdata(config_p->ipv4.if_name_S5_S8));
  OAILOG_INFO (LOG_CONFIG, "    S5_S8 ip .............: %s/%u\n", inet_ntoa (*((struct in_addr *)&config_p->ipv4.S5_S8)), config_p->ipv4.netmask_S5_S8);
  OAILOG_INFO (LOG_CONFIG, "- SGi:\n");
  OAILOG_INFO (LOG_CONFIG, "    SGi iface ............: %s\n", bdata(config_p->ipv4.if_name_SGI));
  OAILOG_INFO (LOG_CONFIG, "    SGi ip ...............: %s/%u\n", inet_ntoa (*((struct in_addr *)&config_p->ipv4.SGI)), config_p->ipv4.netmask_SGI);

  OAILOG_INFO (LOG_CONFIG, "- Masquerading: ..........: %d\n", config_p->masquerade_SGI);
  OAILOG_INFO (LOG_CONFIG, "- Push PCO: ..............: %d\n", config_p->force_push_pco);
}

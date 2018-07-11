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

/*! \file pgw_config.h
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_PGW_CONFIG_SEEN
#define FILE_PGW_CONFIG_SEEN
#include <sys/socket.h> // inet_aton
#include <netinet/in.h> // inet_aton
#include <arpa/inet.h>  // inet_aton
#include "queue.h"
#include "bstrlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PGW_CONFIG_STRING_PGW_CONFIG                            "P-GW"
#define PGW_CONFIG_STRING_NETWORK_INTERFACES_CONFIG             "NETWORK_INTERFACES"
#define PGW_CONFIG_STRING_PGW_INTERFACE_NAME_FOR_S5_S8          "PGW_INTERFACE_NAME_FOR_S5_S8"
#define PGW_CONFIG_STRING_PGW_INTERFACE_NAME_FOR_SGI            "PGW_INTERFACE_NAME_FOR_SGI"
#define PGW_CONFIG_STRING_PGW_IPV4_ADDRESS_FOR_SGI              "PGW_IPV4_ADDRESS_FOR_SGI"
#define PGW_CONFIG_STRING_PGW_MASQUERADE_SGI                    "PGW_MASQUERADE_SGI"
#define PGW_CONFIG_STRING_UE_TCP_MSS_CLAMPING                   "UE_TCP_MSS_CLAMPING"
#define PGW_CONFIG_STRING_NAS_FORCE_PUSH_PCO                    "FORCE_PUSH_PROTOCOL_CONFIGURATION_OPTIONS"

#define PGW_CONFIG_STRING_IP_ADDRESS_POOL                       "IP_ADDRESS_POOL"
#define PGW_CONFIG_STRING_ARP_UE                                "ARP_UE"
#define PGW_CONFIG_STRING_ARP_UE_CHOICE_NO                      "NO"
#define PGW_CONFIG_STRING_ARP_UE_CHOICE_LINUX                   "LINUX"
#define PGW_CONFIG_STRING_ARP_UE_CHOICE_OAI                     "OAI"
#define PGW_CONFIG_STRING_IPV4_ADDRESS_LIST                     "IPV4_LIST"
#define PGW_CONFIG_STRING_IPV4_ADDRESS_RANGE_DELIMITER          '-'
#define PGW_CONFIG_STRING_DEFAULT_DNS_IPV4_ADDRESS              "DEFAULT_DNS_IPV4_ADDRESS"
#define PGW_CONFIG_STRING_DEFAULT_DNS_SEC_IPV4_ADDRESS          "DEFAULT_DNS_SEC_IPV4_ADDRESS"
#define PGW_CONFIG_STRING_UE_MTU                                "UE_MTU"
#define PGW_CONFIG_STRING_GTPV1U_REALIZATION                    "GTPV1U_REALIZATION"
#define PGW_CONFIG_STRING_NO_GTP_KERNEL_AVAILABLE               "NO_GTP_KERNEL_AVAILABLE"
#define PGW_CONFIG_STRING_GTP_KERNEL_MODULE                     "GTP_KERNEL_MODULE"
#define PGW_CONFIG_STRING_GTP_KERNEL                            "GTP_KERNEL"

#define PGW_CONFIG_STRING_INTERFACE_DISABLED                    "none"

#define PGW_CONFIG_STRING_PCEF                                  "PCEF"
#define PGW_CONFIG_STRING_PCEF_ENABLED                          "PCEF_ENABLED"
#define PGW_CONFIG_STRING_TCP_ECN_ENABLED                       "TCP_ECN_ENABLED"
#define PGW_CONFIG_STRING_AUTOMATIC_PUSH_DEDICATED_BEARER_PCC_RULE  "AUTOMATIC_PUSH_DEDICATED_BEARER_PCC_RULE"
#define PGW_CONFIG_STRING_DEFAULT_BEARER_STATIC_PCC_RULE        "DEFAULT_BEARER_STATIC_PCC_RULE"
#define PGW_CONFIG_STRING_PUSH_STATIC_PCC_RULES                 "PUSH_STATIC_PCC_RULES"
#define PGW_CONFIG_STRING_APN_AMBR_UL                           "APN_AMBR_UL"
#define PGW_CONFIG_STRING_APN_AMBR_DL                           "APN_AMBR_DL"
#define PGW_ABORT_ON_ERROR true
#define PGW_WARN_ON_ERROR  false

#define PGW_CONFIG_STRING_OVS_CONFIG                            "OVS"
#define PGW_CONFIG_STRING_OVS_BRIDGE_NAME                       "BRIDGE_NAME"
#define PGW_CONFIG_STRING_OVS_EGRESS_PORT_NUM                   "EGRESS_PORT_NUM"
#define PGW_CONFIG_STRING_OVS_GTP_PORT_NUM                      "GTP_PORT_NUM"
#define PGW_CONFIG_STRING_OVS_L2_EGRESS_PORT                    "L2_EGRESS_PORT"
#define PGW_CONFIG_STRING_OVS_UPLINK_MAC                        "UPLINK_MAC"
#define PGW_CONFIG_STRING_OVS_UDP_PORT_FOR_S1U                  "UDP_PORT_FOR_S1U"

// may be more
#define PGW_MAX_ALLOCATED_PDN_ADDRESSES 1024


typedef struct conf_ipv4_list_elm_s {
  STAILQ_ENTRY(conf_ipv4_list_elm_s) ipv4_entries;
  struct in_addr  addr;
} conf_ipv4_list_elm_t;


typedef struct spgw_ovs_config_s {
  int      gtpu_udp_port_num;
  bstring  bridge_name;
  int      egress_port_num;
  bstring  l2_egress_port;
  int      gtp_port_num;
  bstring  uplink_mac; // next (first) hop
} spgw_ovs_config_t;

#include "pgw_pcef_emulation.h"

typedef struct pgw_config_s {
  /* Reader/writer lock for this configuration */
  pthread_rwlock_t rw_lock;
  bstring          config_file;

  struct {
    bstring        if_name_S5_S8;
    struct in_addr S5_S8;
    uint32_t       mtu_S5_S8; // read from system
    struct in_addr addr_S5_S8;// read from system
    uint8_t        mask_S5_S8;// read from system

    bstring        if_name_SGI;
    struct in_addr SGI;
    uint32_t       mtu_SGI; // read from system
    struct in_addr addr_sgi;// read from system
    uint8_t        mask_sgi;// read from system

    struct in_addr default_dns;
    struct in_addr default_dns_sec;
  } ipv4;

#if ENABLE_LIBGTPNL
  bool      ue_tcp_mss_clamp; // for UE TCP traffic
  bool      masquerade_SGI;
#endif

  int              num_ue_pool;
#define PGW_NUM_UE_POOL_MAX 96
  struct in_addr   ue_pool_range_low[PGW_NUM_UE_POOL_MAX];
  struct in_addr   ue_pool_range_high[PGW_NUM_UE_POOL_MAX];

  struct in_addr   ue_pool_network[PGW_NUM_UE_POOL_MAX]; // computed from config
  struct in_addr   ue_pool_netmask[PGW_NUM_UE_POOL_MAX]; // computed from config
  //computed from config, UE IP adresses that matches ue_pool_network[]/ue_pool_netmask[] but do not match ue_pool_range_low[] - ue_pool_range_high[]
  // The problem here is that OpenFlow do not deal with ip ranges but with netmasks
  STAILQ_HEAD(ipv4_list_excl_from_pool_head_s,     ipv4_list_elm_s)  ue_pool_excluded[PGW_NUM_UE_POOL_MAX];

  bool             arp_ue_linux;
  bool             arp_ue_oai;

  bool      force_push_pco;
  uint16_t  ue_mtu;
  bool      use_gtp_kernel_module;
  bool      enable_loading_gtp_kernel_module;

  struct {
    bool      enabled;
    bool      tcp_ecn_enabled;           // test for CoDel qdisc
    sdf_id_t  default_bearer_sdf_identifier;
    sdf_id_t  automatic_push_dedicated_bearer_sdf_identifier;
    sdf_id_t  preload_static_sdf_identifiers[SDF_ID_MAX-1];
    uint64_t  apn_ambr_ul;
    uint64_t  apn_ambr_dl;
  } pcef;

#if ENABLE_OPENFLOW
  spgw_ovs_config_t ovs_config;
#endif

  STAILQ_HEAD(ipv4_pool_head_s, conf_ipv4_list_elm_s) ipv4_pool_list;
} pgw_config_t;


int pgw_config_process(pgw_config_t* config_pP);
void pgw_config_init(pgw_config_t* config_pP);
int pgw_config_parse_file (pgw_config_t * config_pP);
void pgw_config_display (pgw_config_t * config_p);

#define pgw_config_read_lock(pGWcONFIG)  pthread_rwlock_rdlock(&(pGWcONFIG)->rw_lock)
#define pgw_config_write_lock(pGWcONFIG) pthread_rwlock_wrlock(&(pGWcONFIG)->rw_lock)
#define pgw_config_unlock(pGWcONFIG)     pthread_rwlock_unlock(&(pGWcONFIG)->rw_lock)

#ifdef __cplusplus
}
#endif

#endif /* FILE_PGW_CONFIG_SEEN */

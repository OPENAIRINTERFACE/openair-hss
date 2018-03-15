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

/* 
 * A Topology Manager is an extension of the current OAI code that it has been 
 * implemented ot gather topological information in a LTE network.

 * Acknowledge to H2020-ICT-2014-2/671672, SELFNET (Framework for Self-Organized 
 * Network Management in Virtualized and Software Defined Networks)

 * Authors
 * -------
 * Ricardo Marco Alaez, University of the West of Scotland              Ricardo.MarcoAlaez@uws.ac.uk
 * Jose Maria Alcaraz-Calero, University of the West of Scotland        Jose.Alcaraz-Calero@uws.ac.uk
 * Qi Wang, University of the West of Scotland                          Qi.Wang@uws.ac.uk 
*/

/*! \file sgw_config.h
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_SGW_CONFIG_SEEN
#define FILE_SGW_CONFIG_SEEN
#include <stdint.h>
#include <stdbool.h>
#include "log.h"
#include "bstrlib.h"
#include "common_types.h"


#define SGW_CONFIG_STRING_SGW_CONFIG                            "S-GW"
#define SGW_CONFIG_STRING_NETWORK_INTERFACES_CONFIG             "NETWORK_INTERFACES"
#define SGW_CONFIG_STRING_SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP  "SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP"
#define SGW_CONFIG_STRING_SGW_IPV4_ADDRESS_FOR_S1U_S12_S4_UP    "SGW_IPV4_ADDRESS_FOR_S1U_S12_S4_UP"
#define SGW_CONFIG_STRING_SGW_PORT_FOR_S1U_S12_S4_UP            "SGW_IPV4_PORT_FOR_S1U_S12_S4_UP"
#define SGW_CONFIG_STRING_SGW_INTERFACE_NAME_FOR_S5_S8_UP       "SGW_INTERFACE_NAME_FOR_S5_S8_UP"
#define SGW_CONFIG_STRING_SGW_IPV4_ADDRESS_FOR_S5_S8_UP         "SGW_IPV4_ADDRESS_FOR_S5_S8_UP"
#define SGW_CONFIG_STRING_SGW_INTERFACE_NAME_FOR_S11            "SGW_INTERFACE_NAME_FOR_S11"
#define SGW_CONFIG_STRING_SGW_IPV4_ADDRESS_FOR_S11              "SGW_IPV4_ADDRESS_FOR_S11"

/**********************UWS**********************/
#define SGW_CONFIG_STRING_SGW_TOPOLOGY_CONFIG                   "TOPOLOGY"
#define SGW_CONFIG_STRING_SGW_TOPOLOGY_FILEPATH                 "SGW_TOPOLOGY_FILEPATH"
#define SGW_CONFIG_STRING_SGW_TOPOLOGY_ENABLE                   "SGW_ENABLE_TOPOLOGY"
/**********************UWS**********************/

#define SPGW_ABORT_ON_ERROR true
#define SPGW_WARN_ON_ERROR false

typedef struct sgw_config_s {
  /* Reader/writer lock for this configuration */
  pthread_rwlock_t rw_lock;

  struct {
    uint32_t  queue_size;
    bstring   log_file;
  } itti_config;

  struct {
    bstring        if_name_S1u_S12_S4_up;
    struct in_addr S1u_S12_S4_up;
    int            netmask_S1u_S12_S4_up;

    bstring        if_name_S5_S8_up;
    struct in_addr S5_S8_up;
    int            netmask_S5_S8_up;

    bstring        if_name_S11;
    struct in_addr S11;
    int            netmask_S11;
  } ipv4;

/**********************UWS**********************/
  bstring      topologyfilepath;
  bool         topologyenable;
/**********************UWS**********************/

  uint16_t     udp_port_S1u_S12_S4_up;

  bool         local_to_eNB;

  log_config_t log_config;

  bstring      config_file;
} sgw_config_t;

void sgw_config_init (sgw_config_t * config_pP);
int sgw_config_process (sgw_config_t * config_pP);
int sgw_config_parse_file (sgw_config_t * config_pP);
void sgw_config_display (sgw_config_t * config_p);

#define sgw_config_read_lock(sGWcONFIG)  do { pthread_rwlock_rdlock(&(sGWcONFIG)->rw_lock);} while(0)
#define sgw_config_write_lock(sGWcONFIG) do { pthread_rwlock_wrlock(&(sGWcONFIG)->rw_lock);} while(0)
#define sgw_config_unlock(sGWcONFIG)     do { pthread_rwlock_unlock(&(sGWcONFIG)->rw_lock);} while(0)

#endif /* FILE_SGW_CONFIG_SEEN */

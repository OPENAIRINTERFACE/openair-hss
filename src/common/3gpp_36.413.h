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

/*! \file 3gpp_36.413.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_3GPP_36_413_SEEN
#define FILE_3GPP_36_413_SEEN


// 9.2.1.60 Allocation and Retention Priority
// This IE specifies the relative importance compared to other E-RABs for allocation and retention of the E-UTRAN
// Radio Access Bearer.
typedef struct allocation_and_retention_priority_s  {
  priority_level_t            priority_level; // INTEGER (0..15)
  pre_emption_capability_t    pre_emption_capability;
  pre_emption_vulnerability_t pre_emption_vulnerability;
} allocation_and_retention_priority_t;


// 9.2.1.19 Bit Rate
// This IE indicates the number of bits delivered by E-UTRAN in UL or to E-UTRAN in DL within a period of time,
// divided by the duration of the period. It is used, for example, to indicate the maximum or guaranteed bit rate for a GBR
// bearer, or an aggregated maximum bit rate.

typedef uint64_t bit_rate_t;

// 9.2.1.20 UE Aggregate Maximum Bit Rate
// The UE Aggregate Maximum Bitrate is applicable for all Non-GBR bearers per UE which is defined for the Downlink
//and the Uplink direction and provided by the MME to the eNB.
// Applicable for non-GBR E-RABs
typedef struct ue_aggregate_maximum_bit_rate_s  {
  bit_rate_t dl;
  bit_rate_t ul;
} ue_aggregate_maximum_bit_rate_t;


// 9.2.1.18 GBR QoS Information
// This IE indicates the maximum and guaranteed bit rates of a GBR bearer for downlink and uplink.
typedef struct gbr_qos_information_s  {
  bit_rate_t e_rab_maximum_bit_rate_downlink;
  bit_rate_t e_rab_maximum_bit_rate_uplink;
  bit_rate_t e_rab_guaranteed_bit_rate_downlink;
  bit_rate_t e_rab_guaranteed_bit_rate_uplink;
} gbr_qos_information_t;

// 9.2.1.15 E-RAB Level QoS Parameters
// This IE defines the QoS to be applied to an E-RAB.
typedef struct e_rab_level_qos_parameters_s  {
  qci_t                               qci;
  allocation_and_retention_priority_t allocation_and_retention_priority;
  gbr_qos_information_t gbr_qos_information;
} e_rab_level_qos_parameters_t;

// 9.2.1.2 E-RAB ID
typedef ebi_t e_rab_id_t;

// 9.1.3.1 E-RAB SETUP REQUEST
typedef struct e_rab_to_be_setup_item_s {
  e_rab_id_t                       e_rab_id;
  e_rab_level_qos_parameters_t     e_rab_level_qos_parameters;
  bstring                          transport_layer_address;
  teid_t                           gtp_teid;
  bstring                          nas_pdu;
  // Correlation ID TODO if necessary
} e_rab_to_be_setup_item_t;

typedef struct e_rab_to_be_setup_list_s {
  uint16_t                      no_of_items;
#define MAX_NO_OF_E_RABS 16 /* Spec says 256 */
  e_rab_to_be_setup_item_t      item[MAX_NO_OF_E_RABS];
} e_rab_to_be_setup_list_t;

// 9.1.3.2 E-RAB SETUP RESPONSE
typedef struct e_rab_setup_item_s {
  e_rab_id_t                       e_rab_id;
  bstring                          transport_layer_address;
  teid_t                           gtp_teid;
} e_rab_setup_item_t;

// 9.1.3.3 E-RAB MODIFY REQUEST
typedef struct e_rab_to_be_modified_item_s {
  e_rab_id_t                       e_rab_id;
  e_rab_level_qos_parameters_t     e_rab_level_qos_parameters;
  bstring                          transport_layer_address;
  teid_t                           gtp_teid;
  bstring                          nas_pdu;
  // Correlation ID TODO if necessary
} e_rab_to_be_modified_item_t;

typedef struct e_rab_to_be_modified_list_s {
  uint16_t                      no_of_items;
#define MAX_NO_OF_E_RABS 16 /* Spec says 256 */
  e_rab_to_be_modified_item_t      item[MAX_NO_OF_E_RABS];
} e_rab_to_be_modified_list_t;

typedef struct e_rab_setup_list_s {
  uint16_t                      no_of_items;
  e_rab_setup_item_t            item[MAX_NO_OF_E_RABS];
} e_rab_setup_list_t;

typedef struct e_rab_modify_list_s {
  uint16_t                      no_of_items;
  e_rab_setup_item_t            item[MAX_NO_OF_E_RABS];
} e_rab_modify_list_t;

#include "S1AP_Cause.h"
typedef struct  bearer_status_count_s{
  const char 			  empty;
  long                    pdcp_count;
  long                    hfn_count;
}__attribute__((__packed__)) bearer_status_count_t;

typedef struct status_transfer_bearer_item_s{
#define MSG_STATUS_TRANSFER_MAX_BEARER_CONTEXTS   11
	ebi_t		          ebi;
	bearer_status_count_t bsc_ul;
	bearer_status_count_t bsc_dl;
}__attribute__((__packed__)) status_transfer_bearer_item_t;

typedef struct status_transfer_bearer_list_s{
#define MSG_STATUS_TRANSFER_MAX_BEARER_CONTEXTS   11
	  int					  num_bearers;
	  status_transfer_bearer_item_t    bearerStatusTransferList[MSG_STATUS_TRANSFER_MAX_BEARER_CONTEXTS]; /**< Target-ToSource Transparent Container bearer items. */
}status_transfer_bearer_list_t;

typedef struct e_rab_item_s {
  e_rab_id_t                       e_rab_id;
  S1AP_Cause_t                     cause;
} e_rab_item_t;

typedef struct e_rab_list_s {
  uint16_t              no_of_items;
  e_rab_item_t          item[MAX_NO_OF_E_RABS];
  uint8_t				erab_bitmap;
} e_rab_list_t;

#endif /* FILE_3GPP_36_413_SEEN */

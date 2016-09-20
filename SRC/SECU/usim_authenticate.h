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

#ifndef FILE_USIM_AUTHENTICATE_SEEN
#define FILE_USIM_AUTHENTICATE_SEEN

#define USIM_LTE_K_SIZE  16
#define USIM_AK_SIZE     6
#define USIM_SQN_SIZE    USIM_AK_SIZE
#define USIM_SQNMS_SIZE  USIM_SQN_SIZE
#define USIM_XMAC_SIZE   8

#define USIM_RAND_SIZE 16
#define USIM_AUTN_SIZE 16
#define USIM_AUTS_SIZE 16
#define USIM_RES_SIZE 16

typedef struct usim_data_s {
  uint8_t  lte_k[USIM_LTE_K_SIZE];

  // Highest sequence number the USIM has ever accepted
  uint8_t sqn_ms[USIM_SQNMS_SIZE];

  // List of the last used sequence numbers
#define USIM_SQN_LIST_SIZE  32
  uint8_t  num_sqns;
  uint64_t sqn[USIM_SQN_LIST_SIZE];

  // latests inputs
  uint8_t  rand[USIM_RAND_SIZE];
  uint8_t  autn[USIM_AUTN_SIZE];

  // latests outputs
  uint8_t  auts[USIM_AUTS_SIZE];
  uint8_t  res[USIM_RES_SIZE];

} usim_data_t;


int usim_authenticate(usim_data_t * const usim_data,
                      uint8_t * const rand,
                      uint8_t * const autn,
                      uint8_t * const auts,
                      uint8_t * const res,
                      uint8_t * const ck,
                      uint8_t * const ik);


#endif /* FILE_USIM_AUTHENTICATE_SEEN */

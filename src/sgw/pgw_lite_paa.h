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

/*! \file pgw_lite_paa.h
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/
#ifndef FILE_PGW_LITE_PAA_SEEN
#define FILE_PGW_LITE_PAA_SEEN

#ifdef __cplusplus
extern "C" {
#endif

void pgw_load_pool_ip_addresses       (void);
int pgw_get_free_ipv4_paa_address     (struct in_addr * const addr_P);
int pgw_release_free_ipv4_paa_address (const struct in_addr * const addr_P);
int get_assigned_ipv4_block(const int block, struct in_addr * const netaddr, uint32_t * const prefix);
int get_num_paa_ipv4_pool(void);
int get_paa_ipv4_pool(const int block, struct in_addr * const netaddr, struct in_addr * const mask);
int get_paa_ipv4_pool_id(const struct in_addr ue_addr);


#ifdef __cplusplus
}
#endif

#endif

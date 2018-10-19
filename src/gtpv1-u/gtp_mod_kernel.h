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
#ifndef FILE_GTP_MOD_KERNEL_SEEN
#define FILE_GTP_MOD_KERNEL_SEEN

bool gtp_mod_kernel_enabled(void);

int gtp_mod_kernel_tunnel_add(struct in_addr ue, struct in_addr gw, uint32_t i_tei, uint32_t o_tei, uint8_t bearer_id);
int gtp_mod_kernel_tunnel_del(uint32_t i_tei, uint32_t o_tei);

int gtp_mod_kernel_init(int *fd0, int *fd1u, struct in_addr *ue_net, int mask, int gtp_dev_mtu);
void gtp_mod_kernel_stop(void);

#endif /* FILE_GTP_MOD_KERNEL_SEEN */


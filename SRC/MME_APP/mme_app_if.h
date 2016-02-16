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



#ifndef FILE_MME_APP_IF_SEEN
#define FILE_MME_APP_IF_SEEN


/*
 * IF method called by lower layers (S1AP) delivering the content of initial NAS UE message
 */
int itf_mme_app_nas_initial_ue_message(
    const mme_ue_s1ap_id_t  mme_ue_s1ap_id,
    const uint8_t * const nas_msg,
    const uint32_t  nas_msg_length,
    const long      cause,
    const uint8_t   tai_plmn[3],
    const uint16_t  tai_tac,
    const uint64_t  s_tmsi);


#endif /* FILE_MME_APP_IF_SEEN */

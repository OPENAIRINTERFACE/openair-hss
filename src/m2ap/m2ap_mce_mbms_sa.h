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

/*! \file m2ap_mbms_sa.h
  \brief
  \author Dincer Beken
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#ifndef FILE_M2AP_MBMS_SA_SEEN
#define FILE_M2AP_MBMS_SA_SEEN

enum {
  MBMS_SA_LIST_UNKNOWN_SA = -2,
  MBMS_SA_LIST_UNKNOWN_PLMN = -1,
  MBMS_SA_LIST_RET_OK = 0,
  MBMS_SA_LIST_NO_MATCH = 0x1,
  MBMS_SA_LIST_AT_LEAST_ONE_MATCH = 0x2,
  MBMS_SA_LIST_COMPLETE_MATCH = 0x3,
};

int
m2ap_mce_combare_mbms_plmn(
  const M2AP_PLMN_Identity_t* const tbcd_plmn);

int m2ap_mce_compare_mbms_enb_configuration_item(M2AP_ENB_MBMS_Configuration_data_Item_t *enb_mbms_cfg_item);

#endif /* FILE_M2AP_MBMS_SA_SEEN */

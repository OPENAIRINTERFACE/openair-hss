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

/*! \file m2ap_mce_mbms_sa.c
  \brief
  \author Dincer Beken
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "m2ap_common.h"
#include "m2ap_mce_mbms_sa.h"
#include "bstrlib.h"

#include "log.h"
#include "assertions.h"
#include "conversions.h"
#include "mme_config.h"


static
  int
m2ap_mce_compare_plmn (
  const M2AP_PLMN_Identity_t * const plmn)
{
  int                                     i = 0;
  uint16_t                                mcc = 0;
  uint16_t                                mnc = 0;
  uint16_t                                mnc_len = 0;

  DevAssert (plmn != NULL);
  TBCD_TO_MCC_MNC (plmn, mcc, mnc, mnc_len);
  mme_config_read_lock (&mme_config);

  for (i = 0; i < mme_config.served_mbms_sa.nb_mbms_sa; i++) {
    OAILOG_TRACE (LOG_S1AP, "Comparing plmn_mcc %d/%d, plmn_mnc %d/%d plmn_mnc_len %d/%d\n",
        mme_config.served_mbms_sa.plmn_mcc[i], mcc, mme_config.served_mbms_sa.plmn_mnc[i], mnc, mme_config.served_mbms_sa.plmn_mnc_len[i], mnc_len);

    if ((mme_config.served_tai.plmn_mcc[i] == mcc) &&
        (mme_config.served_tai.plmn_mnc[i] == mnc) &&
        (mme_config.served_tai.plmn_mnc_len[i] == mnc_len))
      /*
       * There is a matching plmn
       */
      return MBMS_SA_LIST_AT_LEAST_ONE_MATCH;
  }

  mme_config_unlock (&mme_config);
  return MBMS_SA_LIST_NO_MATCH;
}

/* @brief compare the MBMS Service Area List
*/
static
  int
m2ap_mce_compare_mbms_sa_list(
  const M2AP_MBMS_Service_Area_ID_List_t * const mbms_sa_list)
{
  int                                     i = 0, j = 0;
  uint16_t                                mbms_sa_value = 0;
  DevAssert (mbms_sa_list != NULL);
  mme_config_read_lock (&mme_config);
  for (i = 0; i < mme_config.served_mbms_sa.nb_mbms_sa; i++) {
    for (j = 0; j < mbms_sa_list->list.count; j++) {
      M2AP_MBMS_Service_Area_t * mbms_sa = &mbms_sa_list->list.array[j];
      OCTET_STRING_TO_MBMS_SA (mbms_sa, mbms_sa_value);
      OAILOG_TRACE (LOG_M2AP, "Comparing config MBMS SA %d, received MBMS SA = %d\n", mme_config.served_mbms_sa.mbms_sa_list[i], mbms_sa_value);
      if (mme_config.served_mbms_sa.mbms_sa_list[i] == mbms_sa_value){
    	/**
    	 * At least one of the eNBs offered SA's is supported by the MME.
    	 * We will accept this eNB, and refer to it in the set of the MBMS Service Area.
    	 * We only will regard MBMS Service Areas, which match.
    	 */
    	return MBMS_SA_LIST_AT_LEAST_ONE_MATCH;
      }
    }
  }
  mme_config_unlock (&mme_config);
  return MBMS_SA_LIST_NO_MATCH;
}

/* @brief compare a given MBMS eNB MBMS Configuration item for a single cell, against the one provided by MCE configuration.
   @param enb_mbms_cfg_item
   @return - MBMS_SA_LIST_UNKNOWN_PLMN if at least one MBMS Service Area match and no PLMN match
           - MBMS_SA_LIST_UNKNOWN_MBMS_SA if at least one PLMN match and no MBMS Service Area match
           - MBMS_SA_LIST_RET_OK if both MBMS Service Area and plmn match at least one element
*/
int
m2ap_mce_compare_mbms_enb_configuration_item(
	M2AP_ENB_MBMS_Configuration_data_Item_t	 * enb_mbms_cfg_item)
{
  int                                     i;
  int                                     mbms_sa_ret,
                                          mbms_plmn_ret;

  DevAssert (enb_mbms_cfg_item != NULL);

  /*
   * Parse every item in the list and try to find matching parameters
   */
  /** Compare the PLMN. */
  mbms_plmn_ret = m2ap_mce_compare_plmn (&enb_mbms_cfg_item->eCGI.pLMN_Identity);
  /** Compare the MBMS Service Area List, with the mme configuration, that at least one MBMS SA is supported. */
  mbms_sa_ret = m2ap_mce_compare_mbms_sa_list(&enb_mbms_cfg_item->mbmsServiceAreaList);
  if (mbms_sa_ret == MBMS_SA_LIST_NO_MATCH && mbms_plmn_ret == MBMS_SA_LIST_NO_MATCH) {
	  return MBMS_SA_LIST_UNKNOWN_PLMN + MBMS_SA_LIST_UNKNOWN_SA;
  } else {
	  if (mbms_sa_ret > MBMS_SA_LIST_NO_MATCH && mbms_plmn_ret == MBMS_SA_LIST_NO_MATCH) {
		 return MBMS_SA_LIST_UNKNOWN_PLMN;
	  } else if (mbms_sa_ret == MBMS_SA_LIST_NO_MATCH && mbms_plmn_ret > MBMS_SA_LIST_NO_MATCH) {
		  return MBMS_SA_LIST_UNKNOWN_SA;
	  }
  }
  /** Successfully verified the MBMS eNB configuration item for the cell. */
  return MBMS_SA_LIST_RET_OK;
}

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
  for (j = 0; j < mbms_sa_list->list.count; j++) {
    M2AP_MBMS_Service_Area_t * mbms_sa = &mbms_sa_list->list.array[j];
    OCTET_STRING_TO_MBMS_SA (mbms_sa, mbms_sa_value);
    /** Check if it is a global MBMS SAI. */
    if(mbms_sa_value <= mme_config.mbms.mbms_global_service_area_types) {
      /** Global MBMS Service Area Id received. */
      OAILOG_INFO(LOG_MME_APP, "Found a matching global MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT ". \n", mbms_sa_value);
      return MBMS_SA_LIST_AT_LEAST_ONE_MATCH;
    }
    /** Check if it is in bounds for the local service areas. */
    int val = mbms_sa_value - mme_config.mbms.mbms_global_service_area_types + 1;
    int local_area = val / mme_config.mbms.mbms_local_service_area_types;
    int local_area_type = val % mme_config.mbms.mbms_local_service_area_types;
    if(local_area < mme_config.mbms.mbms_local_service_areas){
    	OAILOG_INFO(LOG_MME_APP, "Found a valid MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT ". \n", mbms_sa_value);
    	return MBMS_SA_LIST_AT_LEAST_ONE_MATCH;
    }
  }
  mme_config_unlock (&mme_config);
  return MBMS_SA_LIST_NO_MATCH;
}

int
m2ap_mce_combare_mbms_plmn(
  const M2AP_PLMN_Identity_t* const tbcd_plmn)
{
  int rc = MBMS_SA_LIST_UNKNOWN_PLMN;
  plmn_t                                  plmn = {0};
  TBCD_TO_PLMN_T (tbcd_plmn, &plmn);

  mme_config_read_lock (&mme_config);
  /*Verify that the MME code within S-TMSI is same as what is configured in MME conf*/
  if ((plmn.mcc_digit2 == mme_config.mbms.mce_plmn.mcc_digit2) &&
  		(plmn.mcc_digit1 == mme_config.mbms.mce_plmn.mcc_digit1) &&
			(plmn.mnc_digit3 == mme_config.mbms.mce_plmn.mnc_digit3) &&
			(plmn.mcc_digit3 == mme_config.mbms.mce_plmn.mcc_digit3) &&
			(plmn.mnc_digit2 == mme_config.mbms.mce_plmn.mnc_digit2) &&
			(plmn.mnc_digit1 == mme_config.mbms.mce_plmn.mnc_digit1))
  {
	  rc = MBMS_SA_LIST_RET_OK;
  }
  mme_config_unlock (&mme_config);
  return rc;
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
  int                                     mbms_sa_ret;

  DevAssert (enb_mbms_cfg_item != NULL);

  /*
   * Parse every item in the list and try to find matching parameters
   */
  /**
   * We don't compare the PLMN.
   * Currently, no extra configuration field.
   * Compare the MBMS Service Area List, with the mme configuration, that at least one MBMS SA is supported.
   */
  mbms_sa_ret = m2ap_mce_compare_mbms_sa_list(&enb_mbms_cfg_item->mbmsServiceAreaList);
  if (mbms_sa_ret == MBMS_SA_LIST_NO_MATCH)
    return MBMS_SA_LIST_UNKNOWN_SA;
  /** Successfully verified the MBMS eNB configuration item for the cell. */
  return MBMS_SA_LIST_RET_OK;
}

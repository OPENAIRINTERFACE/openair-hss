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

/*! \file s1ap_mme_ta.c
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "assertions.h"
#include "common_defs.h"
#include "conversions.h"
#include "log.h"
#include "mme_config.h"
#include "s1ap_common.h"
#include "s1ap_mme_gummei.h"

int s1ap_mme_compare_gummei(const S1AP_PLMNidentity_t* const tbcd_plmn) {
  int num_gummei;
  int rc = RETURNerror;
  int i = 0;
  plmn_t plmn;

  DevAssert(tbcd_plmn != NULL);

  memset((void*)&plmn, 0, sizeof(plmn_t));
  TBCD_TO_PLMN_T(tbcd_plmn, &plmn);

  mme_config_read_lock(&mme_config);
  /*
   * Check number of MMEs in the pool.
   * At present it is assumed that one MME is supported in MME pool but in case
   * there are more than one MME configured then search the serving MME using
   * MME code. Assumption is that within one PLMN only one pool of MME will be
   * configured
   */
  if (mme_config.gummei.nb > 1) {
    OAILOG_WARNING(LOG_S1AP, "More than one GUMMEI is configured. \n");
  }
  for (num_gummei = 0; num_gummei < mme_config.gummei.nb; num_gummei++) {
    /*Verify that the MME code within S-TMSI is same as what is configured in
     * MME conf*/
    if ((plmn.mcc_digit2 ==
         mme_config.gummei.gummei[num_gummei].plmn.mcc_digit2) &&
        (plmn.mcc_digit1 ==
         mme_config.gummei.gummei[num_gummei].plmn.mcc_digit1) &&
        (plmn.mnc_digit3 ==
         mme_config.gummei.gummei[num_gummei].plmn.mnc_digit3) &&
        (plmn.mcc_digit3 ==
         mme_config.gummei.gummei[num_gummei].plmn.mcc_digit3) &&
        (plmn.mnc_digit2 ==
         mme_config.gummei.gummei[num_gummei].plmn.mnc_digit2) &&
        (plmn.mnc_digit1 ==
         mme_config.gummei.gummei[num_gummei].plmn.mnc_digit1)) {
      rc = RETURNok;
      break;
    }
  }
  mme_config_unlock(&mme_config);
  if (num_gummei >= mme_config.gummei.nb) {
    OAILOG_ERROR(LOG_S1AP, "No MME-GUMMEI serves this eNB");
  }
  OAILOG_FUNC_RETURN(LOG_S1AP, rc);
}

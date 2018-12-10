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


/*! \file mme_app_apn_selection.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "common_types.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "common_defs.h"
#include "mme_app_apn_selection.h"
#include "mme_app_defs.h"


//------------------------------------------------------------------------------
int
mme_app_select_apn(imsi64_t imsi, const_bstring const ue_selected_apn, apn_configuration_t **apn_configuration)
{
  OAILOG_FUNC_IN(LOG_MME_APP);
  /** Get the APN config profile from the IMSI. */
  apn_config_profile_t   apn_config_profile;                  // set by S6A UPDATE LOCATION ANSWER
  /** Subscription profile. */
  subscription_data_t   *subscription_data = subscription_data_exists_imsi(&mme_app_desc.mme_ue_contexts, imsi);

  if(subscription_data == NULL){
    *apn_configuration = NULL;
    OAILOG_INFO (LOG_MME_APP, "No subscription data is received for IMSI " IMSI_64_FMT ". \n", imsi);
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
  }

  context_identifier_t          default_context_identifier = subscription_data->apn_config_profile.context_identifier;
  int                           index;

  for (index = 0; index < subscription_data->apn_config_profile.nb_apns; index++) {
    if (!ue_selected_apn) {
      /*
       * OK we got our default APN
       */
      if (subscription_data->apn_config_profile.apn_configuration[index].context_identifier == default_context_identifier) {
        OAILOG_DEBUG (LOG_MME_APP, "Selected APN %s for UE " IMSI_64_FMT "\n",
            subscription_data->apn_config_profile.apn_configuration[index].service_selection, imsi);
        *apn_configuration = &subscription_data->apn_config_profile.apn_configuration[index];
        OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
      }
    } else {
      /*
       * OK we got the UE selected APN
       */
      if (biseqcaselessblk (ue_selected_apn,
          subscription_data->apn_config_profile.apn_configuration[index].service_selection,
          strlen(subscription_data->apn_config_profile.apn_configuration[index].service_selection)) == 1) {
          OAILOG_DEBUG (LOG_MME_APP, "Selected APN %s for UE " IMSI_64_FMT "\n",
              subscription_data->apn_config_profile.apn_configuration[index].service_selection, imsi);
          *apn_configuration = &subscription_data->apn_config_profile.apn_configuration[index];
          OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
      }
    }
  }
  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
}

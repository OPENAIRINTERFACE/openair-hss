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



#include "as_message.h"
#include "common_types.h"
#include "s6a_defs.h"
#include "mme_app_defs.h"

nas_cause_t
s6a_error_2_nas_cause (
  uint32_t s6a_error,
  int experimental)
{
  if (experimental == 0) {
    /*
     * Base protocol errors
     */
    switch (s6a_error) {
      /*
       * 3002
       */
    case ER_DIAMETER_UNABLE_TO_DELIVER:        /* Fall through */

      /*
       * 3003
       */
    case ER_DIAMETER_REALM_NOT_SERVED: /* Fall through */

      /*
       * 5003
       */
    case ER_DIAMETER_AUTHORIZATION_REJECTED:
      return NAS_CAUSE_NO_SUITABLE_CELLS_IN_TRACKING_AREA;

      /*
       * 5012
       */
    case ER_DIAMETER_UNABLE_TO_COMPLY: /* Fall through */

      /*
       * 5004
       */
    case ER_DIAMETER_INVALID_AVP_VALUE:        /* Fall through */

      /*
       * Any other permanent errors from the diameter base protocol
       */
    default:
      break;
    }
  } else {
    switch (s6a_error) {
      /*
       * 5001
       */
    case DIAMETER_ERROR_USER_UNKNOWN:
      return NAS_CAUSE_EPS_SERVICES_AND_NON_EPS_SERVICES_NOT_ALLOWED;

      /*
       * TODO: distinguish GPRS_DATA_SUBSCRIPTION
       */
      /*
       * 5420
       */
    case DIAMETER_ERROR_UNKNOWN_EPS_SUBSCRIPTION:
      return NAS_CAUSE_NO_SUITABLE_CELLS_IN_TRACKING_AREA;

      /*
       * 5421
       */
    case DIAMETER_ERROR_RAT_NOT_ALLOWED:
      /*
       * One of the following parameter can be sent depending on
       * operator preference:
       * ROAMING_NOT_ALLOWED_IN_THIS_TRACKING_AREA
       * TRACKING_AREA_NOT_ALLOWED
       * NO_SUITABLE_CELLS_IN_TRACKING_AREA
       */
      return NAS_CAUSE_TRACKING_AREA_NOT_ALLOWED;

      /*
       * 5004 without error diagnostic
       */
    case DIAMETER_ERROR_ROAMING_NOT_ALLOWED:
      return NAS_CAUSE_PLMN_NOT_ALLOWED;

      /*
       * TODO: 5004 with error diagnostic of ODB_HPLMN_APN or
       * ODB_VPLMN_APN
       */
      /*
       * TODO: 5004 with error diagnostic of ODB_ALL_APN
       */
    default:
      break;
    }
  }

  return NAS_CAUSE_NETWORK_FAILURE;
}

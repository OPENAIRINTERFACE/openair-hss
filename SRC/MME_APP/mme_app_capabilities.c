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


/*! \file mme_app_capabilities.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intertask_interface.h"
#include "mme_config.h"
#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "secu_defs.h"
#include "assertions.h"
#include "common_types.h"
#include "log.h"

int
mme_app_handle_s1ap_ue_capabilities_ind (
  const itti_s1ap_ue_cap_ind_t const *s1ap_ue_cap_ind_pP)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  DevAssert (s1ap_ue_cap_ind_pP );
  //unsigned enb_ue_s1ap_id:24;
  //uint32_t mme_ue_s1ap_id;
  //uint8_t  radio_capabilities[100];
  //uint32_t radio_capabilities_length;
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}

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

/*****************************************************************************
  Source      esm_data_context.c

  Version     0.1

  Date        2016/01/29

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Lionel Gauthier


*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "assertions.h"
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "common_defs.h"
#include "networkDef.h"
#include "log.h"
#include "dynamic_memory_check.h"

#include "common_defs.h"

#include "mme_app_ue_context.h"
#include "mme_config.h"

#include "emm_data.h"
#include "esm_ebr_context.h"
#include "esm_proc.h"
#include "esm_data.h"
#include "nas_timer.h"

// free allocated structs
//------------------------------------------------------------------------------
void free_esm_bearer_context(esm_ebr_context_t * esm_ebr_context)
{
  if (esm_ebr_context) {
    if (esm_ebr_context->pco) {
      free_protocol_configuration_options(&esm_ebr_context->pco);
    }
    if (esm_ebr_context->tft) {
      free_traffic_flow_template(&esm_ebr_context->tft);
    }
    if (NAS_TIMER_INACTIVE_ID != esm_ebr_context->timer.id) {
      esm_ebr_timer_data_t * esm_ebr_timer_data = NULL;
      esm_ebr_context->timer.id = nas_timer_stop (esm_ebr_context->timer.id, (void**)&esm_ebr_timer_data);
      /*
       * Release the retransmisison timer parameters
       */
      if (esm_ebr_timer_data) {
        if (esm_ebr_timer_data->msg) {
          bdestroy_wrapper (&esm_ebr_timer_data->msg);
        }
        free_wrapper ((void**)&esm_ebr_timer_data);
      }
    }
  }
}

//------------------------------------------------------------------------------
void esm_bearer_context_init(esm_ebr_context_t * esm_ebr_context)
{
  if (esm_ebr_context) {
    memset(esm_ebr_context, 0, sizeof(*esm_ebr_context));
    esm_ebr_context->status   = ESM_EBR_INACTIVE;
    esm_ebr_context->timer.id = NAS_TIMER_INACTIVE_ID;
  }
}

// free allocated structs
//------------------------------------------------------------------------------
//void free_esm_pdn(esm_pdn_t * pdn)
//{
//  if (pdn) {
//    bdestroy_wrapper (&pdn->apn);
//    unsigned int i;
//    for (i=0; i < ESM_DATA_EPS_BEARER_MAX; i++) {
//      free_esm_bearer(pdn->bearer[i]);
//    }
//    free_wrapper((void**)&pdn);
//  }
//}

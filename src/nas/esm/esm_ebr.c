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
  Source      esm_ebr.c

  Version     0.1

  Date        2013/01/29

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Frederic Maurel

  Description Defines functions used to handle state of EPS bearer contexts
        and manage ESM messages re-transmission.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "log.h"
#include "msc.h"
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "assertions.h"
#include "common_defs.h"
#include "mme_app_ue_context.h"
#include "mme_api.h"
#include "mme_app_defs.h"
#include "emm_data.h"
#include "mme_app_esm_procedures.h"
#include "esm_ebr.h"
#include "esm_proc.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/


#define ESM_EBR_NB_UE_MAX   (MME_API_NB_UE_MAX + 1)

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* String representation of EPS bearer context status */
static const char                      *_esm_ebr_state_str[ESM_EBR_STATE_MAX] = {
  "BEARER CONTEXT INACTIVE",
  "BEARER CONTEXT ACTIVE",
  "BEARER CONTEXT INACTIVE PENDING",
  "BEARER CONTEXT MODIFY PENDING",
  "BEARER CONTEXT ACTIVE PENDING"
};

/*
   ----------------------
   User notification data
   ----------------------
*/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/
// todo: fit somewhere better..
//------------------------------------------------------------------------------
const char * esm_ebr_state2string(esm_ebr_state esm_ebr_state)
{
  switch (esm_ebr_state) {
    case ESM_EBR_INACTIVE:
      return "ESM_EBR_INACTIVE";
    case ESM_EBR_ACTIVE:
      return "ESM_EBR_ACTIVE";
    case ESM_EBR_INACTIVE_PENDING:
      return "ESM_EBR_INACTIVE_PENDING";
    case ESM_EBR_MODIFY_PENDING:
      return "ESM_EBR_MODIFY_PENDING";
    case ESM_EBR_ACTIVE_PENDING:
      return "ESM_EBR_ACTIVE_PENDING";
    default:
      return "UNKNOWN";
  }
}

/*
 * Bearer Context Procedures
 */
//-----------------------------------------------------------------------------
nas_esm_proc_bearer_context_t *_esm_proc_create_bearer_context_procedure(mme_ue_s1ap_id_t ue_id, imsi_t *imsi, pti_t pti, ebi_t ebi, bstring apn)
{
  nas_esm_proc_bearer_context_t  *esm_proc_bearer_context = mme_app_nas_esm_create_bearer_context_procedure(ue_id, pti, ebi);
  AssertFatal(esm_bearer_context_proc, "TODO Handle this");
  memcpy((void*)&esm_proc_bearer_context->imsi, imsi, sizeof(imsi_t));
  if(apn)
    esm_proc_bearer_context->subscribed_apn = bstrcpy(apn);
  /** Timeout notifier set separately. */
  return esm_proc_bearer_context;
}

//-----------------------------------------------------------------------------
void _esm_proc_free_bearer_context_procedure(nas_esm_proc_bearer_context_t ** esm_proc_bearer_context){
  // free content
  void *unused = NULL;
  /** Forget the name of the timer.. only one can exist (also used for activate default EPS bearer context.. */
  nas_stop_esm_timer((*esm_proc_bearer_context)->esm_base_proc.ue_id,
      &((*esm_proc_bearer_context)->esm_base_proc.esm_proc_timer));
  mme_app_nas_esm_free_bearer_context_proc(esm_proc_bearer_context);
}

//-----------------------------------------------------------------------------
nas_esm_proc_bearer_context_t _esm_proc_get_bearer_context_procedure(mme_ue_s1ap_id_t ue_id, pti_t pti, ebi_t ebi){
  return mme_app_nas_esm_get_bearer_context_procedure(ue_id, pti, ebi);
}

// todo: fix this
//-----------------------------------------------------------------------------
esm_ebr_state _esm_ebr_get_status(mme_ue_s1ap_id_t ue_id, ebi_t ebi){
  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(ue_context){
    bearer_context_t * bearer_context = NULL;
    mme_app_get_session_bearer_context_from_all(ue_context, ebi, &bearer_context);
    if(bearer_context){
      return bearer_context->esm_ebr_context.status;
    }
  }
  return ESM_EBR_INACTIVE;
}

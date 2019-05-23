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

/*! \file mme_app_procedures.c
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
#include "timer.h"
#include "mme_config.h"
#include "mme_app_extern.h"
#include "mme_app_defs.h"
#include "common_defs.h"
#include "mme_app_esm_procedures.h"

/*---------------------------------------------------------------------------
   FTEID Set RBTree Search Data Structure
  --------------------------------------------------------------------------*/
//
///**
//  Comparator function for comparing two fteids.
//
//  @param[in] a: Pointer to bearer context a.
//  @param[in] b: Pointer to bearer context b.
//  @return  An integer greater than, equal to or less than zero according to whether the
//  object pointed to by a is greater than, equal to or less than the object pointed to by b.
//*/
//
//static
//inline int32_t                    mme_app_compare_bearer_context(
//    struct fteid_set_s *a,
//    struct fteid_set_s *b) {
//    if (a->s1u_fteid.teid > b->s1u_fteid.teid)
//      return 1;
//
//    if (a->s1u_fteid.teid < b->s1u_fteid.teid )
//      return -1;
//
//    /* Not more field to compare. */
//    return 0;
//}
//
//RB_GENERATE (BearerF0teids, fteid_set_s, fteid_set_rbt_Node, fteid_set_compare_s1u_saegw)

static void mme_app_nas_esm_free_bearer_context_proc(nas_esm_proc_bearer_context_t **esm_proc_bearer_context);
static void mme_app_nas_esm_free_pdn_connectivity_proc(nas_esm_proc_pdn_connectivity_t **esm_proc_pdn_connectivity);

//------------------------------------------------------------------------------
void mme_app_nas_esm_free_pdn_connectivity_procedures(ue_context_t * const ue_context)
{
  // todo: LOCK UE CONTEXT?
  if (ue_context->esm_procedures.pdn_connectivity_procedures) {
    nas_esm_proc_pdn_connectivity_t *esm_pdn_connectivity_proc1 = NULL;
    nas_esm_proc_pdn_connectivity_t *esm_pdn_connectivity_proc2 = NULL;

    esm_pdn_connectivity_proc1 = LIST_FIRST(ue_context->esm_procedures.pdn_connectivity_procedures);                 /* Faster List Deletion. */
    while (esm_pdn_connectivity_proc1) {
      esm_pdn_connectivity_proc2 = LIST_NEXT(esm_pdn_connectivity_proc1, entries);
      mme_app_nas_esm_free_pdn_connectivity_proc(&esm_pdn_connectivity_proc1);
      esm_pdn_connectivity_proc1 = esm_pdn_connectivity_proc2;
    }
    LIST_INIT(ue_context->esm_procedures.pdn_connectivity_procedures);
    free_wrapper((void**)&ue_context->esm_procedures.pdn_connectivity_procedures);
  }
}

//------------------------------------------------------------------------------
nas_esm_proc_pdn_connectivity_t* mme_app_nas_esm_create_pdn_connectivity_procedure(mme_ue_s1ap_id_t ue_id, pti_t pti)
{
  OAILOG_FUNC_IN(LOG_MME_APP);
  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
  }
  /* Check the first pdn connectivity procedure, if it has another PTI, reject the request. */
  nas_esm_proc_pdn_connectivity_t *esm_proc_pdn_connectivity = NULL;
  if (ue_context->esm_procedures.pdn_connectivity_procedures) {
    LIST_FOREACH(esm_proc_pdn_connectivity, ue_context->esm_procedures.pdn_connectivity_procedures, entries) {
      if(esm_proc_pdn_connectivity){
        if(esm_proc_pdn_connectivity->esm_base_proc.pti != pti){ /**< PTI may be invalid for idle TAU. */
          OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
        } else {
          /** We may have other pdn connectivity procedure with the same PTI and other EBI. Continuing. */
        }
      }
    }
  }

  // TODO: LOCK_UE_CONTEXT
  esm_proc_pdn_connectivity = calloc(1, sizeof(nas_esm_proc_pdn_connectivity_t));
  esm_proc_pdn_connectivity->esm_base_proc.pti  = pti;
  esm_proc_pdn_connectivity->esm_base_proc.type = ESM_PROC_PDN_CONTEXT;
  esm_proc_pdn_connectivity->esm_base_proc.ue_id = ue_id;
  /* Set the timeout directly. Set the callback argument as the ue_id. */
//  esm_proc_pdn_connectivity->esm_base_proc.esm_proc_timer.id = nas_esm_timer_start(timeout_sec, timeout_usec, arg);
//  esm_proc_pdn_connectivity->esm_base_proc.timeout_notif = timeout_notif;

  /** Initialize the of the procedure. */
  if (!ue_context->esm_procedures.pdn_connectivity_procedures) {
    ue_context->esm_procedures.pdn_connectivity_procedures = calloc(1, sizeof(struct nas_esm_proc_pdn_connectivity_s));
    LIST_INIT(ue_context->esm_procedures.pdn_connectivity_procedures);
  }
  LIST_INSERT_HEAD((ue_context->esm_procedures.pdn_connectivity_procedures), esm_proc_pdn_connectivity, entries);

  return esm_proc_pdn_connectivity;
}

//------------------------------------------------------------------------------
nas_esm_proc_bearer_context_t* mme_app_nas_esm_create_bearer_context_procedure(mme_ue_s1ap_id_t ue_id, pti_t pti, ebi_t ebi, int timeout_sec, int timeout_usec, esm_timeout_cb_t timeout_notif)
{
  OAILOG_FUNC_IN(LOG_MME_APP);
  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
  }
  /* Check the first bearer context procedure, if it has another PTI, reject the request. */
  nas_esm_proc_bearer_context_t *esm_proc_bearer_context = NULL;
  if (ue_context->esm_procedures.bearer_context_procedures) {
    LIST_FOREACH(esm_proc_bearer_context, ue_context->esm_procedures.bearer_context_procedures, entries) {
      if(esm_proc_bearer_context){
        if(esm_proc_bearer_context->esm_base_proc.pti != pti){
          OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
        } else if (esm_proc_bearer_context->bearer_ebi == ebi ) {
          OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
        } else {
          /** We may have other bearer context procedure with the same PTI and other EBI. Continuing. */
        }
      }
    }
  }

  // TODO: LOCK_UE_CONTEXT
  esm_proc_bearer_context = calloc(1, sizeof(nas_esm_proc_bearer_context_t));
  esm_proc_bearer_context->esm_base_proc.ue_id = ue_id;
  esm_proc_bearer_context->esm_base_proc.pti   = pti;
  esm_proc_bearer_context->esm_base_proc.type  = ESM_PROC_EPS_BEARER_CONTEXT;
  /* Set the SAE-GW information. */
  esm_proc_bearer_context->mme_s11_teid = ue_context->mme_teid_s11;
  pdn_context_t * pdn_context = RB_MIN(PdnContexts, &ue_context->pdn_contexts);
  DevAssert(pdn_context);
  esm_proc_bearer_context->saegw_s11_fteid.teid                = pdn_context->s_gw_teid_s11_s4;
  if(((struct sockaddr*)&pdn_context->s_gw_addr_s11_s4)->sa_family == AF_INET){
	  esm_proc_bearer_context->saegw_s11_fteid.ipv4 = 1;
	  esm_proc_bearer_context->saegw_s11_fteid.ipv4_address.s_addr = ((struct sockaddr_in*)&pdn_context->s_gw_addr_s11_s4)->sin_addr.s_addr;
  } else {
	  esm_proc_bearer_context->saegw_s11_fteid.ipv6 = 1;
	  memcpy(&esm_proc_bearer_context->saegw_s11_fteid.ipv6_address, &((struct sockaddr_in6*)&pdn_context->s_gw_addr_s11_s4)->sin6_addr,
			  sizeof(((struct sockaddr_in6*)&pdn_context->s_gw_addr_s11_s4)->sin6_addr));
  }
  /* Set the timeout directly. Set the callback argument as the ue_id. */
  if(timeout_sec || timeout_usec){
    esm_proc_bearer_context->esm_base_proc.esm_proc_timer.id = nas_esm_timer_start(timeout_sec, timeout_usec, (void*)&esm_proc_bearer_context->esm_base_proc);
    esm_proc_bearer_context->esm_base_proc.timeout_notif = timeout_notif;
  }

  /** Initialize the of the procedure. */
  if (!ue_context->esm_procedures.bearer_context_procedures) {
    ue_context->esm_procedures.bearer_context_procedures = calloc(1, sizeof(struct nas_esm_proc_bearer_context_s));
    LIST_INIT(ue_context->esm_procedures.bearer_context_procedures);
  }
  LIST_INSERT_HEAD((ue_context->esm_procedures.bearer_context_procedures), esm_proc_bearer_context, entries);
  // todo: UNLOCK_UE_CONTEXT!
  return esm_proc_bearer_context;
}

//------------------------------------------------------------------------------
nas_esm_proc_pdn_connectivity_t* mme_app_nas_esm_get_pdn_connectivity_procedure(mme_ue_s1ap_id_t ue_id, pti_t pti)
{
  OAILOG_FUNC_IN(LOG_MME_APP);
  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
  }

  if (ue_context->esm_procedures.pdn_connectivity_procedures) {
    nas_esm_proc_pdn_connectivity_t *esm_pdn_connectivity_proc = NULL;

    LIST_FOREACH(esm_pdn_connectivity_proc, ue_context->esm_procedures.pdn_connectivity_procedures, entries) {
      /** Search by PTI only. */
      if (pti == PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED || pti == esm_pdn_connectivity_proc->esm_base_proc.pti) {
        return esm_pdn_connectivity_proc;
      }
    }
  }
  return NULL;
}

//------------------------------------------------------------------------------
void mme_app_nas_esm_delete_pdn_connectivity_proc(nas_esm_proc_pdn_connectivity_t **esm_proc_pdn_connectivity)
{
  // todo: ADD LOCKS
  LIST_REMOVE((*esm_proc_pdn_connectivity), entries);
  mme_app_nas_esm_free_pdn_connectivity_proc(esm_proc_pdn_connectivity);

}

//------------------------------------------------------------------------------
static void mme_app_nas_esm_free_pdn_connectivity_proc(nas_esm_proc_pdn_connectivity_t **esm_proc_pdn_connectivity)
{
  // DO here specific releases (memory,etc)
  /** Remove the bearer contexts to be setup. */
  nas_stop_esm_timer((*esm_proc_pdn_connectivity)->esm_base_proc.ue_id,
      &((*esm_proc_pdn_connectivity)->esm_base_proc.esm_proc_timer));
  /**
   * Free components of the PDN connectivity procedure.
   */
  if((*esm_proc_pdn_connectivity)->subscribed_apn)
    bdestroy_wrapper(&((*esm_proc_pdn_connectivity)->subscribed_apn));

  /** Clear the protocol configuration options. */
  clear_protocol_configuration_options(&((*esm_proc_pdn_connectivity)->pco));

  free_wrapper((void**)esm_proc_pdn_connectivity);
}


/*
 * ESM Bearer Context.
 */
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void mme_app_nas_esm_free_bearer_context_procedures(ue_context_t * const ue_context)
{
  // todo: LOCK UE CONTEXT?
  if (ue_context->esm_procedures.bearer_context_procedures) {
    nas_esm_proc_bearer_context_t *esm_bearer_context_proc1 = NULL;
    nas_esm_proc_bearer_context_t *esm_bearer_context_proc2 = NULL;

    esm_bearer_context_proc1 = LIST_FIRST(ue_context->esm_procedures.bearer_context_procedures);                 /* Faster List Deletion. */
    while (esm_bearer_context_proc1) {
      esm_bearer_context_proc2 = LIST_NEXT(esm_bearer_context_proc1, entries);
      mme_app_nas_esm_free_bearer_context_proc(&esm_bearer_context_proc1);
      esm_bearer_context_proc1 = esm_bearer_context_proc2;
    }
    LIST_INIT(ue_context->esm_procedures.bearer_context_procedures);
    free_wrapper((void**)&ue_context->esm_procedures.bearer_context_procedures);
  }
}

//------------------------------------------------------------------------------
nas_esm_proc_bearer_context_t* mme_app_nas_esm_get_bearer_context_procedure(mme_ue_s1ap_id_t ue_id, pti_t pti, ebi_t ebi)
{
  OAILOG_FUNC_IN(LOG_MME_APP);
  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
  }

  if (ue_context->esm_procedures.bearer_context_procedures) {
    nas_esm_proc_bearer_context_t *esm_proc_bearer_context = NULL;

    LIST_FOREACH(esm_proc_bearer_context, ue_context->esm_procedures.bearer_context_procedures, entries) {
    /** Search by EBI only. */
    if ((PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED == pti || esm_proc_bearer_context->esm_base_proc.pti == pti)
        && (EPS_BEARER_IDENTITY_UNASSIGNED == ebi || esm_proc_bearer_context->bearer_ebi == ebi)){
        OAILOG_FUNC_RETURN(LOG_MME_APP, esm_proc_bearer_context);
      }
    }
  }
  OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
}

//------------------------------------------------------------------------------
void mme_app_nas_esm_delete_bearer_context_proc(nas_esm_proc_bearer_context_t **esm_proc_bearer_context)
{
  // todo: LOCK_UE_CONTEXT
  /** Remove the bearer contexts to be setup. */
  /**
   * Free components of the bearer context procedure.
   */
  LIST_REMOVE((*esm_proc_bearer_context), entries);
  mme_app_nas_esm_free_bearer_context_proc(esm_proc_bearer_context);
  // todo: UNLOCK_UE_CONTEXT
}

//------------------------------------------------------------------------------
static void mme_app_nas_esm_free_bearer_context_proc(nas_esm_proc_bearer_context_t **esm_proc_bearer_context)
{
  // todo: LOCK_UE_CONTEXT
  /** Remove the bearer contexts to be setup. */
  /**
   * Free components of the bearer context procedure.
   */
  nas_stop_esm_timer((*esm_proc_bearer_context)->esm_base_proc.ue_id,
      &((*esm_proc_bearer_context)->esm_base_proc.esm_proc_timer));
  if((*esm_proc_bearer_context)->tft){
    free_traffic_flow_template(&((*esm_proc_bearer_context)->tft));
  }
  free_wrapper((void**)esm_proc_bearer_context);
  // todo: UNLOCK_UE_CONTEXT
}

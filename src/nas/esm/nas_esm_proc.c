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
  Source      nas_esm_proc.c

  Version     0.1

  Date        2018/12/01

  Product     NAS ESM stack

  Subsystem   NAS ESM main process

  Author      Frederic Maurel, Lionel GAUTHIER, Dincer Beken

  Description NAS ESM procedure call manager. This will be the main service access point.

*****************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "msc.h"
#include "s6a_defs.h"
#include "dynamic_memory_check.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "esm_main.h"
#include "esm_cause.h"
#include "esm_sap.h"
#include "nas_esm_proc.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

static nas_cause_t s6a_error_2_nas_cause (uint32_t s6a_error,int experimental);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/


/*
   --------------------------------------------------------------------------
                Timer handlers
   --------------------------------------------------------------------------
*/

//------------------------------------------------------------------------------
void nas_stop_esm_timer(mme_ue_s1ap_id_t ue_id, nas_timer_t * const nas_timer)
{
  if ((nas_timer) && (nas_timer->id != NAS_TIMER_INACTIVE_ID)) {
    void *nas_timer_callback_args;
    nas_timer->id = nas_timer_stop (nas_timer->id, (void**)&nas_timer_callback_args);
    if (NAS_TIMER_INACTIVE_ID == nas_timer->id) {
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 ESM_TIMERstopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
      OAILOG_INFO (LOG_NAS_EMM, "ESM_TIMER stopped UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
    } else {
      OAILOG_ERROR (LOG_NAS_EMM, "Could not stop ESM_TIMER UE " MME_UE_S1AP_ID_FMT "\n", ue_id);
    }
  }
}

/****************************************************************************
 **                                                                        **
 ** Name:    _esm_proc_timeout_handler ()                                  **
 **                                                                        **
 ** Description: Generic timeout handler                                   **
 **                                                                        **
 **              3GPP TS 24.301, section 6.4.4.5, case a                   **
 **      On the first expiry of the timer TXXXX, the MME shall re- **
 **      send the DEACTIVATE EPS BEARER CONTEXT REQUEST and shall  **
 **      reset and restart timer T3489. This retransmission is     **
 **      repeated four times, i.e. on the fifth expiry of timer    **
 **      T3489, the MME shall abort the procedure and deactivate   **
 **      the EPS bearer context locally.                           **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void _nas_proc_esm_timeout_handler (void *args)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;

  /*
   * Send an internal ESM signal to handle the timeout.
   */
  memset(&esm_sap, 0, sizeof(esm_sap_t));
  esm_sap.primitive = ESM_TIMEOUT_IND;
  esm_sap.data.esm_proc_timeout = (uintptr_t)args;
  esm_sap_signal(&esm_sap, &rsp);
  if(rsp){
    /**
     * Will get and lock the EMM context to set the security header if there is a valid EMM context.
     * No changes in the state of the EMM context.
     */
    lowerlayer_data_req(esm_sap.ue_id, rsp); // todo: esm_cause
    bdestroy_wrapper(&rsp);
  }
  OAILOG_FUNC_OUT (LOG_NAS_ESM);
}

/*
   --------------------------------------------------------------------------
            NAS ESM procedures triggered by the user
   --------------------------------------------------------------------------
*/

//------------------------------------------------------------------------------
int
nas_esm_proc_data_ind (
  itti_nas_esm_data_ind_t * esm_data_ind)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  bstring                                 rsp = NULL;
  int                                     rc = RETURNok;

  _esm_sap_recv(esm_data_ind->ue_id, &esm_data_ind->imsi, &esm_data_ind->visited_tai, esm_data_ind->req, &rsp);
  /** We don't check for the error.. If a response message is there, we directly transmit it over the lower layers.. */
  if(rsp){
    /**
     * Will get and lock the EMM context to set the security header if there is a valid EMM context.
     * No changes in the state of the EMM context.
     */
    rc = lowerlayer_data_req(esm_data_ind->ue_id, rsp);
    bdestroy_wrapper(&rsp);
  }
  /* Destroy the received message immediately. */
  bdestroy_wrapper(&esm_data_ind->req);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int
nas_esm_proc_esm_detach(
  itti_nas_esm_detach_ind_t * esm_detach)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNok;

  esm_sap.primitive = ESM_DETACH_IND;
  esm_sap.ue_id     = esm_detach->ue_id;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_DETACH_IND " MME_UE_S1AP_ID_FMT " ", esm_detach->ue_id);
  /** Handle each bearer context separately. */
  /** Get the bearer contexts to be updated. */
  esm_sap_signal(&esm_sap, &rsp);
  DevAssert(!rsp);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int
nas_esm_proc_pdn_config_res (
    esm_cn_pdn_config_res_t * pdn_config_res)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNok;

  esm_sap.primitive = ESM_PDN_CONFIG_RES;
  esm_sap.ue_id = pdn_config_res->ue_id;
  esm_sap.data.pdn_config_res = pdn_config_res;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_PDN_CONFIG_RES ue_id " MME_UE_S1AP_ID_FMT " ", pdn_config_res->ue_id);
  esm_sap_signal(&esm_sap, &rsp);
  if(rsp){
    /**
     * Will get and lock the EMM context to set the security header if there is a valid EMM context.
     * No changes in the state of the EMM context.
     */
    rc = lowerlayer_data_req(esm_sap.ue_id, rsp);
    bdestroy_wrapper(&rsp);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int
nas_esm_proc_pdn_config_fail (
    esm_cn_pdn_config_fail_t * pdn_config_fail)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNok;

  esm_sap.primitive = ESM_PDN_CONFIG_FAIL;
  esm_sap.ue_id = pdn_config_fail->ue_id;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_PDN_CONFIG_FAIL ue_id " MME_UE_S1AP_ID_FMT " ", pdn_config_fail->ue_id);
  esm_sap_signal(&esm_sap, &rsp); // todo: esm_cause
  if(rsp){
    /**
     * Will get and lock the EMM context to set the security header if there is a valid EMM context.
     * No changes in the state of the EMM context.
     */
    rc = lowerlayer_data_req(esm_sap.ue_id, rsp);
    bdestroy_wrapper(&rsp);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int
nas_esm_proc_pdn_connectivity_res (
  esm_cn_pdn_connectivity_res_t * pdn_conn_res)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNok;

  esm_sap.primitive = (pdn_conn_res->esm_cause == ESM_CAUSE_SUCCESS) ? ESM_PDN_CONNECTIVITY_CNF : ESM_PDN_CONNECTIVITY_REJ;
  esm_sap.ue_id = pdn_conn_res->ue_id;
  esm_sap.data.pdn_connectivity_res = pdn_conn_res;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_PDN_CONNECTIVITY_RES ue_id " MME_UE_S1AP_ID_FMT " ", pdn_conn_res->ue_id);
  esm_sap_signal(&esm_sap, &rsp); // todo: esm_cause
  if(rsp){
    if(pdn_conn_res->esm_cause == ESM_CAUSE_SUCCESS && esm_sap.esm_cause == ESM_CAUSE_SUCCESS) {
      if(esm_sap.is_attach)
        rc = _emm_wrapper_attach_accept(pdn_conn_res->ue_id, rsp);
      else {
        rc = lowerlayer_activate_bearer_req(pdn_conn_res->ue_id, esm_sap.data.pdn_connectivity_res->linked_ebi, 0, 0, 0, 0, rsp);
      }
    } else {
      /**
       * Will get and lock the EMM context to set the security header if there is a valid EMM context.
       * No changes in the state of the EMM context.
       */
      rc = lowerlayer_data_req(esm_sap.ue_id, rsp);
    }
    bdestroy_wrapper(&rsp);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int
nas_esm_proc_pdn_disconnect_res(
    esm_cn_pdn_disconnect_res_t * pdn_disconn_res)
{
  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap.primitive = ESM_PDN_DISCONNECT_CNF;
  esm_sap.ue_id = pdn_disconn_res->ue_id;
  esm_sap.data.pdn_disconnect_res = pdn_disconn_res;
  esm_sap.data.pdn_disconnect_res->default_ebi = EPS_BEARER_IDENTITY_UNASSIGNED;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_PDN_DISCONNECT_RES ue_id " MME_UE_S1AP_ID_FMT " ", pdn_disconn_res->ue_id);
  esm_sap_signal(&esm_sap, &rsp);
  if(rsp){
     if(esm_sap.esm_cause == ESM_CAUSE_SUCCESS) {
       rc = lowerlayer_deactivate_bearer_req(pdn_disconn_res->ue_id, esm_sap.data.pdn_disconnect_res->default_ebi, rsp);
       bdestroy_wrapper(&rsp);
       int num_ded_ebi = 0;
       while(num_ded_ebi < esm_sap.data.pdn_disconnect_res->ded_ebis.num_ebi && rc == RETURNok){
         rc = lowerlayer_deactivate_bearer_req(pdn_disconn_res->ue_id, esm_sap.data.pdn_disconnect_res->ded_ebis.ebis[num_ded_ebi], NULL);
         num_ded_ebi++;
       }
     } else {
       /**
        * Will get and lock the EMM context to set the security header if there is a valid EMM context.
        * No changes in the state of the EMM context.
        */
       rc = lowerlayer_data_req(esm_sap.ue_id, rsp);
     }
     bdestroy_wrapper(&rsp);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

/* No disconnect reject is expected. */

/*
 *  DEDICATED BEARER HANDLING.
 */
//------------------------------------------------------------------------------
int nas_esm_proc_activate_eps_bearer_ctx(esm_eps_activate_eps_bearer_ctx_req_t * esm_cn_activate)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNok;

  esm_sap.primitive                                   = ESM_EPS_BEARER_CONTEXT_ACTIVATE_REQ;
  esm_sap.ue_id                                       = esm_cn_activate->ue_id;
  esm_sap.data.eps_bearer_context_activate.pdn_cid    = esm_cn_activate->cid;
  esm_sap.data.eps_bearer_context_activate.linked_ebi = esm_cn_activate->linked_ebi;
  /* Process each bearer context to be created separately. */
  bearer_contexts_to_be_created_t * bcs_tbc = (bearer_contexts_to_be_created_t*)esm_cn_activate->bcs_to_be_created_ptr;
  for(int num_bc = 0; num_bc < bcs_tbc->num_bearer_context; num_bc++) {
    esm_sap.data.eps_bearer_context_activate.bc_tbc   = &bcs_tbc->bearer_contexts[num_bc];
    esm_sap_signal(&esm_sap, &rsp);
    if(rsp){
      rc = lowerlayer_activate_bearer_req(esm_cn_activate->ue_id,
          esm_sap.data.eps_bearer_context_activate.bc_tbc->eps_bearer_id,
          esm_sap.data.eps_bearer_context_activate.bc_tbc->bearer_level_qos.mbr.br_dl,
          esm_sap.data.eps_bearer_context_activate.bc_tbc->bearer_level_qos.mbr.br_ul,
          esm_sap.data.eps_bearer_context_activate.bc_tbc->bearer_level_qos.gbr.br_dl,
          esm_sap.data.eps_bearer_context_activate.bc_tbc->bearer_level_qos.gbr.br_ul,
          rsp);
      bdestroy_wrapper(&rsp);
    }
    esm_sap.data.eps_bearer_context_activate.bc_tbc = NULL;
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int nas_esm_proc_modify_eps_bearer_ctx(esm_eps_modify_esm_bearer_ctxs_req_t * esm_cn_modify)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNok;

  esm_sap.primitive = ESM_EPS_BEARER_CONTEXT_MODIFY_REQ;
  esm_sap.ue_id     = esm_cn_modify->ue_id;
  esm_sap.data.eps_bearer_context_modify.pti = esm_cn_modify->pti;
  esm_sap.data.eps_bearer_context_modify.linked_ebi = esm_cn_modify->linked_ebi;
  esm_sap.data.eps_bearer_context_modify.pdn_cid    = esm_cn_modify->cid;
  esm_sap.data.eps_bearer_context_modify.apn_ambr.br_dl = esm_cn_modify->apn_ambr.br_dl;
  esm_sap.data.eps_bearer_context_modify.apn_ambr.br_ul = esm_cn_modify->apn_ambr.br_ul;
  /* Handle each bearer context separately. */
  bearer_contexts_to_be_updated_t* bcs_tbu = (bearer_contexts_to_be_updated_t*)esm_cn_modify->bcs_to_be_updated_ptr;
  for(int num_bc = 0; num_bc < bcs_tbu->num_bearer_context; num_bc++){
    esm_sap.data.eps_bearer_context_modify.bc_tbu       = &bcs_tbu->bearer_contexts[num_bc];
    /* Set the APN-AMBR for the first bearer context. */
    esm_sap_signal(&esm_sap, &rsp);
    if(rsp){
      rc = lowerlayer_modify_bearer_req(esm_cn_modify->ue_id, esm_sap.data.eps_bearer_context_modify.bc_tbu->eps_bearer_id,
          esm_sap.data.eps_bearer_context_modify.bc_tbu->bearer_level_qos->mbr.br_dl,
          esm_sap.data.eps_bearer_context_modify.bc_tbu->bearer_level_qos->mbr.br_ul,
          esm_sap.data.eps_bearer_context_modify.bc_tbu->bearer_level_qos->gbr.br_dl,
          esm_sap.data.eps_bearer_context_modify.bc_tbu->bearer_level_qos->gbr.br_ul,
          rsp);
      bdestroy_wrapper(&rsp);
    }
    /* Remove the APN-AMBR after the first iteration. */
    memset(&esm_sap.data.eps_bearer_context_modify.apn_ambr, 0, sizeof(ambr_t));
    esm_sap.data.eps_bearer_context_modify.bc_tbu = NULL;
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int nas_esm_proc_deactivate_eps_bearer_ctx(esm_eps_deactivate_eps_bearer_ctx_req_t * esm_cn_deactivate)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNok;

  esm_sap.primitive = ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ;
  esm_sap.ue_id     = esm_cn_deactivate->ue_id;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ " MME_UE_S1AP_ID_FMT " ", esm_cn_deactivate->ue_id);
  /** Handle each bearer context separately. */
  /** Get the bearer contexts to be updated. */
  for(int num_ebi= 0; num_ebi < esm_cn_deactivate->ebis.num_ebi; num_ebi++){
    esm_sap.data.eps_bearer_context_deactivate.ded_ebi = esm_cn_deactivate->ebis.ebis[num_ebi];
    esm_sap_signal(&esm_sap, &rsp);
    if(rsp){
      rc = lowerlayer_deactivate_bearer_req(esm_cn_deactivate->ue_id, esm_sap.data.eps_bearer_context_deactivate.ded_ebi,rsp);
      bdestroy_wrapper(&rsp);
    }
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int nas_esm_proc_bearer_resource_failure_indication(itti_s11_bearer_resource_failure_indication_t * s11_bearer_resource_failure_ind)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNok;

  /** Try to find the UE context from the TEID. */
  ue_context_t * ue_context = mme_ue_context_exists_s11_teid (&mme_app_desc.mme_ue_contexts, s11_bearer_resource_failure_ind->teid);

  if (ue_context == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0, "0 BEARER_RESOURCE_FAILRE_INDICATION local S11 teid " TEID_FMT " ",
        s11_bearer_resource_failure_ind->teid);
    OAILOG_DEBUG (LOG_MME_APP, "We didn't find this teid in list of UE: %" PRIX32 "\n", s11_bearer_resource_failure_ind->teid);
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }
  esm_sap.primitive = ESM_EPS_BEARER_RESOURCE_FAILURE_IND;
  esm_sap.ue_id     = ue_context->mme_ue_s1ap_id;
  esm_sap.pti       = s11_bearer_resource_failure_ind->pti;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_EPS_BEARER_RESOURCE_FAILURE_IND " MME_UE_S1AP_ID_FMT " ", esm_sap.ue_id);
  /** Get the bearer contexts to be updated. */
  esm_sap_signal(&esm_sap, &rsp);
  if(rsp){
    /** If a message is received, just send it to ESM layer. */
    rc = lowerlayer_data_req(esm_sap.ue_id, rsp);
    bdestroy_wrapper(&rsp);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

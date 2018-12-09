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
void _nas_proc_pdn_connectivity_timeout_handler (void *args)
{
  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  /*
   * Send an internal ESM signal to handle the timeout.
   */
  esm_sap.primitive = ESM_TIMEOUT_IND;
  esm_sap.ue_id = (mme_ue_s1ap_id_t)args;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_TIMEOUT_IND ue_id " MME_UE_S1AP_ID_FMT " ", pdn_config_res->ue_id);
  rc = esm_sap_signal((mme_ue_s1ap_id_t)args, &esm_cause, &rsp);
  if(rsp){
    /**
     * Will get and lock the EMM context to set the security header if there is a valid EMM context.
     * No changes in the state of the EMM context.
     */
    rc = lowerlayer_data_req((mme_ue_s1ap_id_t)args, esm_cause, rsp);
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
  /** Try to get the ESM Data context. */
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_ESM);
  _esm_sap_recv(esm_data_ind->ue_id, &esm_data_ind->imsi, &esm_data_ind->visited_tai, esm_data_ind->req, &esm_cause, &rsp);
  /** We don't check for the error.. If a response message is there, we directly transmit it over the lower layers.. */
  if(rsp){
    /**
     * Will get and lock the EMM context to set the security header if there is a valid EMM context.
     * No changes in the state of the EMM context.
     */
    rc = lowerlayer_data_req(esm_data_ind->ue_id, rsp);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int
nas_esm_proc_pdn_config_res (
    esm_cn_pdn_config_res_t * pdn_config_res)
{
  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap.primitive = ESM_PDN_CONFIG_RES;
  esm_sap.ue_id = pdn_config_res->ue_id;
  esm_sap.data.pdn_config_res = pdn_config_res;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_PDN_CONFIG_RES ue_id " MME_UE_S1AP_ID_FMT " ", pdn_config_res->ue_id);
  rc = esm_sap_signal(pdn_config_res->ue_id, &esm_cause, &rsp);
  if(rsp){
    /**
     * Will get and lock the EMM context to set the security header if there is a valid EMM context.
     * No changes in the state of the EMM context.
     */
    rc = lowerlayer_data_req(pdn_config_res->ue_id, esm_cause, rsp);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int
nas_esm_proc_pdn_config_fail (
    esm_cn_pdn_config_fail_t * pdn_config_fail)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  esm_sap.primitive = ESM_PDN_CONFIG_FAIL;
  esm_sap.data.pdn_config_fail = pdn_config_fail;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_PDN_CONFIG_FAIL ue_id " MME_UE_S1AP_ID_FMT " ", pdn_config_fail->ue_id);
  rc = esm_sap_signal(pdn_config_fail->ue_id, &esm_cause, &rsp);
  if(rsp){
    /**
     * Will get and lock the EMM context to set the security header if there is a valid EMM context.
     * No changes in the state of the EMM context.
     */
    rc = lowerlayer_data_req(esm_sap.data.pdn_config_fail, esm_cause, rsp);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int
nas_esm_proc_pdn_connectivity_res (
  esm_cn_pdn_connectivity_res_t * pdn_conn_res)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;

  esm_sap.primitive = ESM_PDN_CONNECTIVITY_CNF;
  esm_sap.ue_id = pdn_conn_res->ue_id;
  esm_sap.data.pdn_connectivity_res = pdn_conn_res;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_PDN_CONNECTIVITY_RES ue_id " MME_UE_S1AP_ID_FMT " ", pdn_conn_res->ue_id);
  rc = esm_sap_signal(pdn_conn_res->ue_id, &esm_cause, &rsp);
  if(rsp){
    /**
     * Will get and lock the EMM context to set the security header if there is a valid EMM context.
     * No changes in the state of the EMM context.
     */
    rc = lowerlayer_data_req(pdn_conn_res->ue_id, esm_cause, rsp);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int
nas_esm_proc_pdn_connectivity_fail (
    esm_cn_pdn_connectivity_fail_t * pdn_conn_fail)
{
  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap.primitive = ESM_PDN_CONNECTIVITY_REJ;
  esm_sap.ue_id = pdn_conn_fail->ue_id;
  esm_sap.data.pdn_connectivity_fail = pdn_conn_fail;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_PDN_CONNECTIVITY_FAIL ue_id " MME_UE_S1AP_ID_FMT " ", pdn_conn_fail->ue_id);
  rc = esm_sap_signal(pdn_conn_fail->ue_id, &esm_cause, &rsp);
  if(rsp){
    /**
     * Will get and lock the EMM context to set the security header if there is a valid EMM context.
     * No changes in the state of the EMM context.
     */
    rc = lowerlayer_data_req(pdn_conn_fail->ue_id, esm_cause, rsp);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

/*
 *  DEDICATED BEARER HANDLING.
 */
//------------------------------------------------------------------------------
int nas_esm_proc_activate_eps_bearer_ctx(esm_eps_activate_eps_bearer_ctx_req_t * esm_cn_activate)
{
  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNerror;
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap.primitive = ESM_EPS_BEARER_CONTEXT_ACTIVATE_REQ;
  esm_sap.ue_id     = esm_cn_activate->ue_id;
  esm_sap.data.eps_bearer_context_activate = esm_cn_activate;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_EPS_BEARER_CONTEXT_ACTIVATE_REQ " MME_UE_S1AP_ID_FMT " ", esm_cn_activate->ue_id);
  /** Process each bearer context to be created separately. */

  /** Handle each bearer context separately. */
  /** Get the bearer contexts to be created. */
  bearer_contexts_to_be_created_t * bcs_tbc = (bearer_contexts_to_be_created_t*)esm_cn_activate->bcs_to_be_created_ptr;
  for(int num_bc = 0; num_bc < bcs_tbc->num_bearer_context; num_bc++){
    bearer_context_to_be_created_t * bc_tbc = &bcs_tbc.bearer_contexts[num_bc];
    esm_sap.data.eps_bearer_context_activate.pdn_cid    = esm_cn_activate->cid;
    esm_sap.data.eps_bearer_context_activate.linked_ebi = esm_cn_activate->linked_ebi;
    esm_sap.data.eps_bearer_context_activate.bc_tbc     = bc_tbc;
    rc = esm_sap_signal(esm_cn_activate->ue_id, &esm_cause, &rsp);
    if(rsp){
      rc = lowerlayer_activate_bearer_req(esm_cn_activate->ue_id, esm_sap.data.eps_bearer_context_activate.bc_tbc->eps_bearer_id,
          bc_tbc->bearer_level_qos->mbr.br_dl,
          bc_tbc->bearer_level_qos->mbr.br_ul,
          bc_tbc->bearer_level_qos->gbr.br_dl,
          bc_tbc->bearer_level_qos->gbr.br_ul,
          rsp);
     }
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int nas_esm_proc_modify_eps_bearer_ctx(esm_eps_update_esm_bearer_ctxs_req_t * esm_cn_update)
{
  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap.primitive = ESM_EPS_BEARER_CONTEXT_MODIFY_REQ;
  esm_sap.ue_id     = esm_cn_update->ue_id;
  esm_sap.data.eps_bearer_context_modify = esm_cn_update;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_EPS_BEARER_CONTEXT_MODIFY_REQ " MME_UE_S1AP_ID_FMT " ", esm_cn_update->ue_id);
  /** Handle each bearer context separately. */
  /** Get the bearer contexts to be updated. */
  bearer_contexts_to_be_updated_t* bcs_tbu = (bearer_contexts_to_be_updated_t*)esm_cn_update->bcs_to_be_updated;
  for(int num_bc = 0; num_bc < bcs_tbu->num_bearer_context; num_bc++){
    bearer_context_to_be_updated_t  * bc_tbu = &bcs_tbu.bearer_contexts[num_bc];
    esm_sap.data.eps_bearer_context_modify.bc_tbu     = bc_tbu;
    esm_sap.data.eps_bearer_context_modify.apn_ambr   = esm_cn_update->apn_ambr;
    rc = esm_sap_signal(esm_cn_update->ue_id, &esm_cause, &rsp);
    if(rsp){
      rc = lowerlayer_modify_bearer_req(esm_cn_update->ue_id, esm_sap.data.eps_bearer_context_modify.bc_tbu->eps_bearer_id,
          bc_tbu->bearer_level_qos->mbr.br_dl,
          bc_tbu->bearer_level_qos->mbr.br_ul,
          bc_tbu->bearer_level_qos->gbr.br_dl,
          bc_tbu->bearer_level_qos->gbr.br_ul,
          rsp);
     }
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int nas_esm_proc_deactivate_eps_bearer_ctx(esm_eps_deactivate_eps_bearer_ctx_req_t * esm_cn_deactivate)
{
  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  esm_sap.primitive = ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ;
  esm_sap.ue_id     = esm_cn_deactivate->ue_id;
  esm_sap.data.eps_bearer_context_deactivate = esm_cn_deactivate;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ " MME_UE_S1AP_ID_FMT " ", esm_cn_deactivate->ue_id);
  /** Handle each bearer context separately. */
  /** Get the bearer contexts to be updated. */
  for(int num_ebi= 0; num_ebi < esm_cn_deactivate->ebis.num_ebi; num_ebi++){
    esm_sap.data.eps_bearer_context_deactivate.ded_ebi = esm_cn_deactivate->ebis[num_ebi];
    rc = esm_sap_signal(esm_cn_deactivate->ue_id, &esm_cause, &rsp);
    if(rsp){
      rc = lowerlayer_deactivate_bearer_req(esm_cn_deactivate->ue_id, esm_sap.data.eps_bearer_context_deactivate.ded_ebi,rsp);
     }
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int nas_proc_establish_bearer_update(esm_eps_update_esm_bearer_ctxs_req_t * esm_update_esm_bearer_ctxs)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};
//  emm_sap.primitive = _ESMCN_UPDATE_ESM_BEARERS_REQ;
//  emm_sap.u.emm_cn.u.emm_cn_update_esm_bearer_ctxs_req = emm_cn_update_esm_bearer_ctxs;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_CN_UPDATE_ESM_BEARER_CTXS_REQ" MME_UE_S1AP_ID_FMT " ", esm_update_esm_bearer_ctxs->ue_id);
  rc = esm_sap_send (&esm_sap);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int nas_proc_e_rab_failure(mme_ue_s1ap_id_t ue_id, ebi_t ebi, bool modify, bool remove)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};
//  emm_sap.u.emm_as.u.erab_setup_rej.ue_id = ue_id;
//  emm_sap.u.emm_as.u.erab_setup_rej.ebi   = ebi;
//  if(!modify){
//    emm_sap.primitive = _EMMAS_ERAB_SETUP_REJ;
//    MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 EMM_AS_ERAB_SETUP_REJ " MME_UE_S1AP_ID_FMT " ", ue_id);
//  }else{
//    emm_sap.primitive = _EMMAS_ERAB_MODIFY_REJ;
//    emm_sap.u.emm_as.u.erab_modify_rej.remove_bearer = remove; /**< Union stuff. */
    MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_AS_ERAB_MODIFY_REJ " MME_UE_S1AP_ID_FMT " ", ue_id);
//  }
  rc = esm_sap_send (&esm_sap);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

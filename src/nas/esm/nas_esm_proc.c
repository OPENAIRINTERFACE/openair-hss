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
  uint8_t                             esm_sap_buffer[ESM_SAP_BUFFER_SIZE];
  mme_ue_s1ap_id_t                    ue_id = (mme_ue_s1ap_id_t)args;
  nas_esm_pdn_connectivity_proc_t    *esm_pdn_connectivity_proc = NULL;
  ESM_msg esm_msg;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  memset(&esm_msg, 0, sizeof(ESM_msg));
  memcpy(esm_sap_buffer, 0, ESM_SAP_BUFFER_SIZE);
  /*
   * Get the procedure of the timer.
   */
  esm_pdn_connectivity_proc = esm_data_get_procedure(ue_id);
  if(!esm_pdn_connectivity_proc){
    OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - No procedure for UE " MME_UE_S1AP_ID_FMT " found (T3489).\n", ue_id);
    OAILOG_FUNC_OUT(LOG_NAS_ESM);
  }
  OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Timer expired for transaction (type=%d, ue_id=" MME_UE_S1AP_ID_FMT "), " "retransmission counter = %d\n",
      esm_pdn_connectivity_proc->trx_base_proc.trx_type, ue_id, esm_pdn_connectivity_proc->trx_base_proc.esm_proc_timer.count);
  /*
   * Remove the ESM procedure and stop the ESM timer if running.
   */
  rc = nas_stop_esm_timer(ue_id, &esm_pdn_connectivity_proc->trx_base_proc.esm_proc_timer);

  /**
   * Process the timeout.
   * Encode the returned message. Currently, building and encoding a new message every time.
   */
  rc = esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.timeout_notif(esm_pdn_connectivity_proc, &esm_msg);
  if(rc == RETURNok){
    /** Encode the received message.
     * todo: may need to add trx type --> Bearer Context Transactions to the specific lower layer function
     */
    int size = esm_msg_encode (&esm_msg, (uint8_t *) esm_sap_buffer, ESM_SAP_BUFFER_SIZE);
    if (size > 0) {
      bstring resp = blk2bstr(esm_sap_buffer, size);
      rc = esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.lowerlayer_cb(ue_id, ESM_CAUSE_SUCCESS, resp);
      if(rc != RETURNok){
        /** Remove the transaction. */
        esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.free_cb(esm_pdn_connectivity_proc);
        OAILOG_ERROR(LOG_NAS_ESM, "ESM-SAP   - Error sending the transaction message for UE " MME_UE_S1AP_ID_FMT " found (timeout). "
            "Removed the transaction (pti=%d).\n", ue_id, esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.pti);
      }
      OAILOG_FUNC_OUT (LOG_NAS_ESM);
    }
  }else {
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - Error while handling the timeout callback for (pti=%d, ue_id=" MME_UE_S1AP_ID_FMT "), " "retransmission counter = %d. Removing the transaction. \n",
        esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.pti, ue_id, esm_pdn_connectivity_proc->trx_base_proc.esm_proc_timer.count);
    esm_pdn_connectivity_proc->trx_base_proc.esm_proc.base_proc.free_cb(esm_pdn_connectivity_proc);
    OAILOG_FUNC_OUT (LOG_NAS_ESM);
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
nas_esm_proc_attach_ind (
  itti_nas_esm_attach_ind_t * attach_ind)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;

  /** Try to get the ESM Data context. */
  esm_sap_t                               esm_sap   = {0};
  bstring                                 rsp       = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;

  esm_sap.primitive = ESM_ATTACH_IND;
  esm_sap.ue_id = attach_ind->ue_id;
  esm_sap.data.attach_ind = attach_ind;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_ATTACH_IND ue_id " MME_UE_S1AP_ID_FMT " ", attach_ind->ue_id);
  rc = esm_sap_signal(attach_ind->ue_id, attach_ind->esm_msg_p, &esm_cause, &rsp);
  /** For the error case, the attach_reject callback should be sent. */
  if(rc == RETURNok && rsp){
    /**
     * Will get and lock the EMM context to set the security header if there is a valid EMM context.
     * No changes in the state of the EMM context.
     */
    rc = lowerlayer_data_req(attach_ind->ue_id, esm_cause, rsp);
  } else if (rc == RETURNerror) {
    /** No EMS transaction should be created, but we should use the failure callback. */
    rc = _emm_attach_reject(attach_ind->ue_id, rsp);
  }
  /** Do nothing. */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int
nas_esm_proc_pdn_config_res (
    esm_cn_pdn_config_res_t * pdn_config_res)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;

  esm_sap.primitive = ESM_PDN_CONFIG_RES;
  esm_sap.ue_id = pdn_config_res->ue_id;
  esm_sap.data.pdn_config_res = pdn_config_res;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_PDN_CONFIG_RES ue_id " MME_UE_S1AP_ID_FMT " ", pdn_config_res->ue_id);
  rc = esm_sap_signal(pdn_config_res->ue_id, &esm_cause, &rsp);
  if(rc == RETURNok && rsp){
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
nas_esm_proc_data_ind (
  itti_nas_esm_data_ind_t * esm_data_ind)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;

  /** Try to get the ESM Data context. */
  bstring                                rsp = NULL;
  esm_cause_t                            esm_cause = ESM_CAUSE_SUCCESS;
//  lower_layer_cb_t                       ll_cb = lowerlayer_data_req; /**< Default lower layer callback method. May use other lower layer callback methods for dedicated bearers (not for attach). */
  rc = _esm_sap_recv(esm_data_ind->ue_id, esm_data_ind->esm_msg_p, *ll_cb, &esm_cause, &rsp);
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
  if(rc == RETURNok && rsp){
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
  if(rc == RETURNok && rsp){
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
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};
  bstring                                 rsp = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;

  esm_sap.primitive = ESM_PDN_CONNECTIVITY_REJ;
  esm_sap.ue_id = pdn_conn_fail->ue_id;
  esm_sap.data.pdn_connectivity_fail = pdn_conn_fail;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_PDN_CONNECTIVITY_FAIL ue_id " MME_UE_S1AP_ID_FMT " ", pdn_conn_fail->ue_id);
  rc = esm_sap_signal(pdn_conn_fail->ue_id, &esm_cause, &rsp);
  if(rc == RETURNok && rsp){
    /**
     * Will get and lock the EMM context to set the security header if there is a valid EMM context.
     * No changes in the state of the EMM context.
     */
    rc = lowerlayer_data_req(pdn_conn_fail->ue_id, esm_cause, rsp);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int
nas_proc_pdn_disconnect_res (
  esm_pdn_disconnect_t * esm_cn_pdn_disc_res)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};

//  emm_sap.primitive = ESMCN_PDN_DISCONNECT_RES;
//  emm_sap.u.emm_cn.u.emm_cn_pdn_disconnect_res = emm_cn_pdn_disc_res;
//  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESMCN_PDN_DISCONNECT_RES ue_id " MME_UE_S1AP_ID_FMT " ", emm_cn_pdn_disc_res->ue_id);
  rc = esm_sap_send (&esm_sap);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int nas_proc_activate_dedicated_bearer(esm_eps_activate_eps_bearer_ctx_req_t * esm_cn_activate)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};
//  esm_sap.primitive = _ESMCN_ACTIVATE_DEDICATED_BEARER_REQ;
//  emm_sap.u.emm_cn.u.emm_cn_activate_dedicated_bearer_req = emm_cn_activate;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_ACTIVATE_DEDICATED_BEARER_REQ " MME_UE_S1AP_ID_FMT " ", esm_cn_activate->ue_id);
  rc = esm_sap_send (&esm_sap);

  // todo: lower_layer_is_directly_activate_dedicated_bearers!

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int nas_proc_modify_eps_bearer_ctx(esm_eps_modify_eps_bearer_ctx_req_t * esm_cn_modify_eps_bearer_ctx)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};
//  esm_sap.primitive = _ESMCN_MODIFY_EPS_BEARER_CTX_REQ;
//  esm_sap.u.emm_cn.u.emm_cn_modify_eps_bearer_ctx_req = emm_cn_modify_eps_bearer_ctx;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_MODIFY_EPS_BEARER_CTX_REQ " MME_UE_S1AP_ID_FMT " ", esm_modify->ue_id);
  rc = esm_sap_send (&esm_sap);

  // todo: lower_layer_is_directly_modify_activate_dedicated_bearers!

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int nas_proc_deactivate_dedicated_bearer(esm_eps_deactivate_eps_bearer_ctx_req_t * esm_deactivate_eps_bearer)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};
  //  emm_sap.primitive = _ESMCN_DEACTIVATE_DEDICATED_BEARER_REQ;
  //  emm_sap.u.emm_cn.u.emm_cn_deactivate_dedicated_bearer_req = emm_cn_deactivate;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_DEACTIVATE_DEDICATED_BEARER_REQ " MME_UE_S1AP_ID_FMT " ", esm_deactivate_eps_bearer->ue_id);
  rc = esm_sap_send (&esm_sap);

  // todo: lower_layer_is_directly_modify_activate_dedicated_bearers!

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

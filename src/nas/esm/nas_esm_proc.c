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

  Description NAS ESM procedure call manager

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
            NAS ESM procedures triggered by the user
   --------------------------------------------------------------------------
*/

//------------------------------------------------------------------------------
int
nas_proc_pdn_connectivity_res (
  esm_pdn_connectivity_t * esm_cn_pdn_res)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};

//  esm_sap.primitive = ESMCN_PDN_CONNECTIVITY_RES;
  // todo: esm_sap.u.emm_cn.u.emm_cn_pdn_res = emm_cn_pdn_res;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMCN_PDN_CONNECTIVITY_RES ue_id " MME_UE_S1AP_ID_FMT " ", emm_cn_pdn_res->ue_id);
  rc = esm_sap_send (&esm_sap);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int
nas_proc_pdn_connectivity_fail (
  esm_pdn_connectivity_t * esm_cn_pdn_fail)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};

//  emm_sap.primitive = EMMCN_PDN_CONNECTIVITY_FAIL;
//  emm_sap.u.emm_cn.u.emm_cn_pdn_fail = emm_cn_pdn_fail;
//  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMCN_PDN_CONNECTIVITY_FAIL ue_id " MME_UE_S1AP_ID_FMT " ", emm_cn_pdn_fail->ue_id);
  rc = esm_sap_send (&esm_sap);
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

//  emm_sap.primitive = EMMCN_PDN_DISCONNECT_RES;
//  emm_sap.u.emm_cn.u.emm_cn_pdn_disconnect_res = emm_cn_pdn_disc_res;
//  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMCN_PDN_DISCONNECT_RES ue_id " MME_UE_S1AP_ID_FMT " ", emm_cn_pdn_disc_res->ue_id);
  rc = esm_sap_send (&esm_sap);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int nas_proc_activate_dedicated_bearer(esm_eps_activate_eps_bearer_ctx_req_t * esm_cn_activate)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};
//  esm_sap.primitive = _EMMCN_ACTIVATE_DEDICATED_BEARER_REQ;
//  emm_sap.u.emm_cn.u.emm_cn_activate_dedicated_bearer_req = emm_cn_activate;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_EMM_MME, NULL, 0, "0 ESM_ACTIVATE_DEDICATED_BEARER_REQ " MME_UE_S1AP_ID_FMT " ", esm_cn_activate->ue_id);
  rc = esm_sap_send (&esm_sap);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int nas_proc_modify_eps_bearer_ctx(esm_eps_modify_eps_bearer_ctx_req_t * esm_cn_modify_eps_bearer_ctx)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};
//  esm_sap.primitive = _EMMCN_MODIFY_EPS_BEARER_CTX_REQ;
//  esm_sap.u.emm_cn.u.emm_cn_modify_eps_bearer_ctx_req = emm_cn_modify_eps_bearer_ctx;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_EMM_MME, NULL, 0, "0 ESM_MODIFY_EPS_BEARER_CTX_REQ " MME_UE_S1AP_ID_FMT " ", esm_modify->ue_id);
  rc = esm_sap_send (&esm_sap);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int nas_proc_deactivate_dedicated_bearer(esm_eps_deactivate_eps_bearer_ctx_req_t * esm_deactivate_eps_bearer)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};
//  emm_sap.primitive = _EMMCN_DEACTIVATE_DEDICATED_BEARER_REQ;
//  emm_sap.u.emm_cn.u.emm_cn_deactivate_dedicated_bearer_req = emm_cn_deactivate;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_EMM_MME, NULL, 0, "0 ESM_DEACTIVATE_DEDICATED_BEARER_REQ " MME_UE_S1AP_ID_FMT " ", esm_deactivate_eps_bearer->ue_id);
  rc = esm_sap_send (&esm_sap);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

//------------------------------------------------------------------------------
int nas_proc_establish_bearer_update(esm_eps_update_esm_bearer_ctxs_req_t * esm_update_esm_bearer_ctxs)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  esm_sap_t                               esm_sap = {0};
//  emm_sap.primitive = _EMMCN_UPDATE_ESM_BEARERS_REQ;
//  emm_sap.u.emm_cn.u.emm_cn_update_esm_bearer_ctxs_req = emm_cn_update_esm_bearer_ctxs;
  MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_EMM_MME, NULL, 0, "0 ESM_CN_UPDATE_ESM_BEARER_CTXS_REQ" MME_UE_S1AP_ID_FMT " ", esm_update_esm_bearer_ctxs->ue_id);
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
//    MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMM_AS_ERAB_SETUP_REJ " MME_UE_S1AP_ID_FMT " ", ue_id);
//  }else{
//    emm_sap.primitive = _EMMAS_ERAB_MODIFY_REJ;
//    emm_sap.u.emm_as.u.erab_modify_rej.remove_bearer = remove; /**< Union stuff. */
    MSC_LOG_TX_MESSAGE (MSC_NAS_MME, MSC_NAS_EMM_MME, NULL, 0, "0 ESM_AS_ERAB_MODIFY_REJ " MME_UE_S1AP_ID_FMT " ", ue_id);
//  }
  rc = esm_sap_send (&esm_sap);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

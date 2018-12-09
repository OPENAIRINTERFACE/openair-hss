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
Source      nas_esm_proc.h

Version     0.1

Date        2012/09/20

Product     NAS ESM stack

Subsystem   NAS ESM main process

Author      Frederic Maurel, Lionel GAUTHIER, Dincer Beken

Description NAS ESM procedure call manager

*****************************************************************************/
#ifndef FILE_NAS_ESM_PROC_SEEN
#define FILE_NAS_ESM_PROC_SEEN

#include "common_defs.h"
#include "mme_config.h"
#include "networkDef.h"
#include "esm_sapDef.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *          NAS procedures triggered by the user
 * --------------------------------------------------------------------------
 */


/*
 * --------------------------------------------------------------------------
 *      NAS procedures triggered by the network
 * --------------------------------------------------------------------------
 */
void nas_stop_esm_timer(mme_ue_s1ap_id_t ue_id, nas_timer_t * const nas_timer);

void _nas_proc_pdn_connectivity_timeout_handler (void *args);

int nas_proc_establish_ind(const mme_ue_s1ap_id_t ue_id,
                            const tai_t originating_tai,
                            const ecgi_t ecgi,
                            const as_cause_t as_cause,
                            STOLEN_REF bstring *msg);
/*
 * --------------------------------------------------------------------------
 *      NAS ESM procedures triggered by the mme applicative layer
 * --------------------------------------------------------------------------
 */
/** Messages sent by the EMM layer (directly from UE). */
int nas_esm_proc_data_ind (itti_nas_esm_data_ind_t * esm_data_ind);

/** Messages triggered by the core network. */
int nas_esm_proc_pdn_config_res (esm_cn_pdn_config_res_t * emm_cn_pdn_config_res);
int nas_esm_proc_pdn_config_fail (esm_cn_pdn_config_fail_t * emm_cn_pdn_config_fail);
int nas_esm_proc_pdn_connectivity_res(esm_cn_pdn_connectivity_res_t *esm_cn_pdn_connectivity_rsp);
int nas_esm_proc_pdn_connectivity_fail(esm_cn_pdn_connectivity_fail_t *esm_cn_pdn_connectivity_fail);

int nas_esm_proc_activate_eps_bearer_ctx(esm_eps_activate_eps_bearer_ctx_req_t * esm_cn_activate);
int nas_esm_proc_modify_eps_bearer_ctx(esm_eps_update_esm_bearer_ctxs_req_t * esm_cn_update);
int nas_proc_deactivate_dedicated_bearer(esm_eps_deactivate_eps_bearer_ctx_req_t * esm_cn_deactivate);
int nas_proc_establish_bearer_update(esm_eps_update_esm_bearer_ctxs_req_t * esm_cn_update_esm_bearer_ctxs);
int nas_proc_e_rab_failure(mme_ue_s1ap_id_t ue_id, ebi_t ebi, bool modify, bool remove);
#endif /* FILE_NAS_PROC_SEEN*/

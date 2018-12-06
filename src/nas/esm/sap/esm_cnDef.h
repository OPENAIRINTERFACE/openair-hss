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

Source      esm_cnDef.h

Version     0.1

Date        2018/12/03

Product     NAS stack

Subsystem   EPS Core Network

Author      Sebastien Roux, Lionel GAUTHIER, Dincer Beken

Description

*****************************************************************************/


#ifndef FILE_ESM_CNDEF_SEEN
#define FILE_ESM_CNDEF_SEEN
#include "esm_proc.h"
#include "esm_sapDef.h"
#include "intertask_interface.h"

typedef enum esmcn_primitive_s {
  _ESMCN_START = 400,
  _ESMCN_PDN_CONFIG_RES, // LG
  _ESMCN_PDN_CONFIG_FAIL, // DB
  _ESMCN_PDN_CONNECTIVITY_RES, // LG
  _ESMCN_PDN_CONNECTIVITY_FAIL,// LG
  _ESMCN_PDN_DISCONNECT_RES,// LG
  _ESMCN_ACTIVATE_DEDICATED_BEARER_REQ,// LG
  _ESMCN_MODIFY_EPS_BEARER_CTX_REQ,// LG
  _ESMCN_DEACTIVATE_DEDICATED_BEARER_REQ,// LG
  _ESMCN_UPDATE_ESM_BEARERS_REQ,// LG
  _ESMCN_END
} esm_cn_primitive_t;

struct itti_nas_pdn_config_rsp_s;
struct itti_nas_pdn_connectivity_rsp_s;
struct itti_nas_pdn_connectivity_fail_s;

typedef struct esm_mme_ul_s {
  esm_cn_primitive_t primitive;
  union {
    esm_cn_pdn_config_res_t  *esm_cn_pdn_config_res;
    esm_cn_pdn_config_fail_t *esm_cn_pdn_config_fail;
    esm_cn_pdn_res_t        *esm_cn_pdn_res;
    esm_cn_pdn_fail_t       *esm_cn_pdn_fail;
    esm_cn_pdn_disconnect_res_t *esm_cn_pdn_disconnect_res;
    esm_eps_activate_eps_bearer_ctx_req_t *esm_cn_activate_dedicated_bearer_req;
    esm_eps_modify_eps_bearer_ctx_req_t *esm_cn_modify_eps_bearer_ctx_req;
    esm_eps_deactivate_eps_bearer_ctx_req_t *esm_cn_deactivate_dedicated_bearer_req;
    esm_eps_update_esm_bearer_ctxs_req_t *esm_cn_update_esm_bearer_ctxs_req;
  } u;
} esm_cn_t;

#endif /* FILE_ESM_CNDEF_SEEN */

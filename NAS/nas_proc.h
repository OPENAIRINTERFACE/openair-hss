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
Source      nas_proc.h

Version     0.1

Date        2012/09/20

Product     NAS stack

Subsystem   NAS main process

Author      Frederic Maurel

Description NAS procedure call manager

*****************************************************************************/
#ifndef __NAS_PROC_H__
#define __NAS_PROC_H__

#include "mme_config.h"
#include "emm_cnDef.h"

#include "commonDef.h"
#include "networkDef.h"

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


void nas_proc_initialize(mme_config_t *mme_config_p);

void nas_proc_cleanup(void);

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



int nas_proc_establish_ind(const uint32_t ueid, const uint8_t plmn[3], const tac_t tac, const Byte_t *data, const uint32_t len);

int nas_proc_dl_transfer_cnf(uint32_t ueid);
int nas_proc_dl_transfer_rej(uint32_t ueid);
int nas_proc_ul_transfer_ind(uint32_t ueid, const Byte_t *data, uint32_t len);

/*
 * --------------------------------------------------------------------------
 *      NAS procedures triggered by the mme applicative layer
 * --------------------------------------------------------------------------
 */
int nas_proc_auth_param_res(emm_cn_auth_res_t *emm_cn_auth_res);
int nas_proc_auth_param_fail(emm_cn_auth_fail_t *emm_cn_auth_fail);
int nas_proc_deregister_ue(uint32_t ue_id);
int nas_proc_pdn_connectivity_res(nas_pdn_connectivity_rsp_t *nas_pdn_connectivity_rsp);
int nas_proc_pdn_connectivity_fail(nas_pdn_connectivity_fail_t *nas_pdn_connectivity_fail);

#endif /* __NAS_PROC_H__*/

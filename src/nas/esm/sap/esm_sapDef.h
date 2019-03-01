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
Source      esm_sapDef.h

Version     0.1

Date        2012/11/21

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines the ESM Service Access Point that provides EPS
        bearer context handling and resources allocation procedures.

*****************************************************************************/


#ifndef __ESM_SAPDEF_H__
#define __ESM_SAPDEF_H__

#include "bstrlib.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * EPS Session Management primitives
 * ---------------------------------
 */
typedef enum esm_primitive_s {
  ESM_START = 0,

  /* Transaction related procedures (initiated by the UE) */
  ESM_PDN_CONFIG_RES,
  ESM_PDN_CONFIG_FAIL,
  ESM_PDN_CONNECTIVITY_CNF,
  ESM_PDN_CONNECTIVITY_REJ,
  ESM_PDN_DISCONNECT_CNF,
  ESM_PDN_DISCONNECT_REJ,

  /* Bearer Request Mesages. */
  ESM_EPS_BEARER_CONTEXT_ACTIVATE_REQ,
  ESM_EPS_BEARER_CONTEXT_MODIFY_REQ,
  ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ,

  ESM_EPS_BEARER_RESOURCE_FAILURE_IND,

  ESM_DETACH_IND,

  /* Internal signal. */
  ESM_TIMEOUT_IND,

  ESM_END
} esm_primitive_t;

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

typedef struct itti_nas_pdn_config_rsp_s                    esm_cn_pdn_config_res_t;
typedef struct itti_nas_pdn_config_fail_s                   esm_cn_pdn_config_fail_t;
typedef struct itti_nas_pdn_connectivity_rsp_s              esm_cn_pdn_connectivity_res_t;
typedef struct itti_nas_pdn_disconnect_rsp_s                esm_cn_pdn_disconnect_res_t;

typedef struct itti_nas_activate_eps_bearer_ctx_req_s       esm_eps_activate_eps_bearer_ctx_req_t;
typedef struct itti_nas_modify_eps_bearer_ctx_req_s         esm_eps_modify_esm_bearer_ctxs_req_t;
typedef struct itti_nas_deactivate_eps_bearer_ctx_req_s     esm_eps_deactivate_eps_bearer_ctx_req_t;

/*
 * ESM primitive for EPS bearer context procedure
 * ---------------------------------------------------------------
 */
typedef struct esm_activate_eps_bearer_context_s {
  pdn_cid_t                        pdn_cid;
  ebi_t                            linked_ebi;
  bearer_context_to_be_created_t  *bc_tbc;
} esm_activate_eps_bearer_context_t;

typedef struct esm_modify_eps_bearer_context_s {
  pdn_cid_t                        pdn_cid;
  ebi_t                            linked_ebi;
  bearer_context_to_be_updated_t  *bc_tbu;
  ambr_t                           apn_ambr;
  pti_t                            pti;
} esm_modify_eps_bearer_context_t;

typedef struct esm_deactivate_eps_bearer_context_s {
  pdn_cid_t                        pdn_cid;
  ebi_t                            linked_ebi;
  ebi_t                            ded_ebi;
  pti_t                            pti;
} esm_deactivate_eps_bearer_context_t;


typedef struct esm_bearer_resource_allocate_rej_s{
  ebi_t             ebi;
}esm_bearer_resource_allocate_rej_t;

/*
 * ESM primitive for PDN disconnect procedure
 * ------------------------------------------
 */
typedef struct esm_pdn_disconnect_s {
  ebi_t     default_ebi;        /* Default EBI of PDN context */
  pdn_cid_t cid;                /* PDN connection local identifier      */
  bool      local_delete;       /* PDN connection local identifier      */
} esm_pdn_disconnect_t;

///*
// * ESM bearer context creation
// * ---------------------------------------------------------
// */
//typedef struct esm_eps_dedicated_bearer_context_s {
//  ebi_t                    linked_ebi;
//  qci_t                    qci;
//  bitrate_t                gbr_ul;
//  bitrate_t                gbr_dl;
//  bitrate_t                mbr_ul;
//  bitrate_t                mbr_dl;
//  traffic_flow_template_t *tft;
//  protocol_configuration_options_t*pco;
//} esm_eps_dedicated_bearer_context_t;
//
///*
// * ESM primitive for activate dedicated EPS bearer context procedure
// * ---------------------------------------------------------
// */
//typedef struct esm_eps_dedicated_bearer_context_activate_s {
//  pdn_cid_t                cid;        /* PDN connection local identifier      */
//  ebi_t                    linked_ebi;
//  esm_eps_dedicated_bearer_context_t esm_eps_dedicated_bearer_context[NUM_MAX_BEARER_UE];
//} esm_eps_dedicated_bearer_context_activate_t;

/*
 * ------------------------------
 * Structure of ESM-SAP primitive
 * ------------------------------
 */
typedef union {
//  esm_pdn_disconnect_t pdn_disconnect;
//  esm_eps_deactivate_eps_bearer_ctx_req_t   eps_dedicated_bearer_context_deactivate;
//  esm_bearer_resource_allocate_rej_t        esm_bearer_resource_allocate_rej;
//  esm_bearer_resource_modify_rej_t          esm_bearer_resource_modify_rej;

  /** From here on just pointers. */
  esm_cn_pdn_config_res_t        *pdn_config_res;
  esm_cn_pdn_connectivity_res_t  *pdn_connectivity_res;
  esm_cn_pdn_disconnect_res_t    *pdn_disconnect_res;
  uintptr_t                       esm_proc_timeout;

  /** Non Pointer structures. */
  esm_activate_eps_bearer_context_t         eps_bearer_context_activate;
  esm_modify_eps_bearer_context_t           eps_bearer_context_modify;
  esm_deactivate_eps_bearer_context_t       eps_bearer_context_deactivate;
} esm_sap_data_t;

struct emm_data_context_s;

typedef struct esm_sap_s {
  esm_primitive_t primitive;      /* ESM-SAP primitive to process                                       */
  unsigned int        ue_id;      /* Local UE identifier                                                */
  bool                is_attach_tau;  /* Define if it is an attach (may be edited inside the method).       */
  pti_t               pti;        /* Procedure transaction Id (needed for BRC - at least.               */
  const_bstring       recv;       /* Encoded ESM message received                                       */
  esm_sap_data_t      data;       /* ESM message data parameters                                        */
  esm_cause_t         esm_cause;
  bool                clr;
  uint16_t			  active_ebrs;
} esm_sap_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

#endif /* __ESM_SAPDEF_H__*/

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
Source      esm_message.h

Version     0.1

Date        2013/02/11

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions executed at the ESM Service Access
        Point to send EPS Session Management messages to the
        EPS Mobility Management sublayer.

*****************************************************************************/
#ifndef __ESM_MESSAGES_H__
#define __ESM_MESSAGES_H__

#include "common_defs.h"
#include "ActivateDedicatedEpsBearerContextRequest.h"
#include "ActivateDefaultEpsBearerContextRequest.h"
#include "BearerResourceAllocationReject.h"
#include "BearerResourceModificationReject.h"
#include "DeactivateEpsBearerContextRequest.h"
#include "EsmInformationRequest.h"
#include "EsmStatus.h"
#include "ModifyEpsBearerContextRequest.h"
#include "PdnConnectivityReject.h"
#include "PdnConnectivityReject.h"
#include "PdnDisconnectReject.h"

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
 * Functions executed by the MME to send ESM message to the UE
 * --------------------------------------------------------------------------
 */
int esm_send_status(pti_t pti, ebi_t ebi, esm_status_msg *msg, int esm_cause);

/*
 * Transaction related messages
 * ----------------------------
 */

int esm_send_pdn_connectivity_reject (
  pti_t pti,
  ESM_msg * esm_msg,
  esm_cause_t esm_cause);

void esm_send_pdn_disconnect_reject(pti_t pti, ESM_msg *esm_rsp_msg, int esm_cause);

/*
 * Messages related to EPS bearer contexts
 * ---------------------------------------
 */

void _default_eps_bearer_activate_t3485_handler (nas_esm_proc_pdn_connectivity_t * esm_pdn_conn_procedure, ESM_msg * esm_resp_msg);

void
esm_send_activate_default_eps_bearer_context_request (
  nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity,
  ambr_t * apn_ambr,
  bearer_qos_t * bearer_level_qos,
  paa_t   * paa,
  ESM_msg * esm_resp_msg);

void esm_send_activate_dedicated_eps_bearer_context_request(pti_t pti, ebi_t ebi,
    activate_dedicated_eps_bearer_context_request_msg *msg, ebi_t linked_ebi,
    const EpsQualityOfService *qos,
    traffic_flow_template_t *tft,
    protocol_configuration_options_t *pco);

void esm_send_modify_eps_bearer_context_request(pti_t pti, ebi_t ebi,
    modify_eps_bearer_context_request_msg *msg,
    const EpsQualityOfService *qos,
    traffic_flow_template_t *tft,
    ambr_t *ambr,
    protocol_configuration_options_t *pco);

void esm_send_deactivate_eps_bearer_context_request(pti_t pti, ebi_t ebi,
    ESM_msg * esm_rsp_msg, esm_cause_t esm_cause);

#endif /* __ESM_MESSAGES_H__*/

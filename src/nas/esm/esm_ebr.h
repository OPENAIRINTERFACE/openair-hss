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
Source      esm_ebr.h

Version     0.1

Date        2013/01/29

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions used to handle state of EPS bearer contexts
        and manage ESM messages re-transmission.

*****************************************************************************/
#ifndef ESM_EBR_SEEN
#define ESM_EBR_SEEN
// todo: remove
#include "mme_app_esm_procedures.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/* Unassigned EPS bearer identity value */
#define ESM_EBI_UNASSIGNED  (EPS_BEARER_IDENTITY_UNASSIGNED)

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

const char * esm_ebr_state2string(esm_ebr_state esm_ebr_state);

/*
 * Bearer Context Procedures
 */
//-----------------------------------------------------------------------------
nas_esm_proc_bearer_context_t *_esm_proc_create_bearer_context_procedure(mme_ue_s1ap_id_t ue_id, imsi_t *imsi, pti_t pti, ebi_t ebi, bstring apn);

//-----------------------------------------------------------------------------
void _esm_proc_free_bearer_context_procedure(nas_esm_proc_bearer_context_t ** esm_proc_bearer_context);

//-----------------------------------------------------------------------------
nas_esm_proc_bearer_context_t *_esm_proc_get_bearer_context_procedure(mme_ue_s1ap_id_t ue_id, pti_t pti, ebi_t ebi);

esm_ebr_state _esm_ebr_get_status(mme_ue_s1ap_id_t ue_id, ebi_t ebi);

#endif /* ESM_EBR_SEEN */

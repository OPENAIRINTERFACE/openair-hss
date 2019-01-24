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
  Source      BearerResourceModificationRequest.c

  Version     0.1

  Date        2019/01/23

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Dincer Beken

  Description Defines the handling of the UE triggered bearer resource modification.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "log.h"
#include "dynamic_memory_check.h"
#include "common_types.h"
#include "assertions.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "mme_app_ue_context.h"
#include "common_defs.h"
#include "mme_config.h"
#include "nas_itti_messaging.h"
#include "mme_app_defs.h"
#include "emm_data.h"
#include "emm_sap.h"
#include "esm_ebr.h"
#include "esm_proc.h"
#include "esm_cause.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
   Internal data handled by the dedicated EPS bearer context activation
   procedure in the MME
   --------------------------------------------------------------------------
*/
/*
   No timer handlers necessary.
*/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_bearer_resource_modification_reject()                        **
 **                                                                        **
 ** Description: Builds Bearer Resource Modification Reject message                    **
 **                                                                        **
 **      The Bearer Resource Modification Reject message is sent by the net-   **
 **      work to the UE to reject a Bearer Resource Modification procedure.    **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity             **
 **      esm_cause: ESM cause code                             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void
esm_send_bearer_resource_modification_reject (
  pti_t pti,
  ESM_msg * esm_msg,
  esm_cause_t esm_cause)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  memset(esm_msg, 0, sizeof(ESM_msg));
  /*
   * Mandatory - ESM message header
   */
  esm_msg->bearer_resource_modification_reject.protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  esm_msg->bearer_resource_modification_reject.epsbeareridentity = EPS_BEARER_IDENTITY_UNASSIGNED;
  esm_msg->bearer_resource_modification_reject.messagetype = BEARER_RESOURCE_MODIFICATION_REJECT;
  esm_msg->bearer_resource_modification_reject.proceduretransactionidentity = pti;
  /*
   * Mandatory - ESM cause code
   */
  esm_msg->bearer_resource_modification_reject.esmcause = esm_cause;
  /*
   * Optional IEs
   */
  esm_msg->bearer_resource_modification_reject.presencemask = 0;
  OAILOG_DEBUG (LOG_NAS_ESM, "ESM-SAP   - Send Bearer Resource Modification Reject message " "(pti=%d, ebi=%d)\n",
      esm_msg->bearer_resource_modification_reject.proceduretransactionidentity, esm_msg->bearer_resource_modification_reject.epsbeareridentity);
  OAILOG_FUNC_OUT(LOG_NAS_ESM);
}

/*
   --------------------------------------------------------------------------
      Bearer Resource Modification Request processing by MME
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_bearer_resource_modification_request()               **
 **                                                                        **
 ** Description: Triggers a Bearer Resource Command message to the SAE-GW
 **      for bearer modification.                                  **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **          pid:       PDN connection identifier                  **
 **      esm_qos:   EPS bearer level QoS parameters            **
 **      tft:       Traffic flow template parameters           **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     ebi:       EPS bearer identity assigned to the new    **
 **             dedicated bearer context                   **
 **      default_ebi:   EPS bearer identity of the associated de-  **
 **             fault EPS bearer context                   **
 **      esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_proc_bearer_resource_modification_request(
  mme_ue_s1ap_id_t   ue_id,
  const proc_tid_t   pti,
  ebi_t              ebi,
  const traffic_flow_aggregate_description_t * tft,
  esm_cause_t          esm_cause_received,
  ESM_msg            * const esm_rsp_msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  ebi_t                                   ded_ebi = 0;
  pdn_context_t                          *pdn_context = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Bearer Resource Modification Request " "(ebi=%d ue_id=" MME_UE_S1AP_ID_FMT ", pti=%d)\n", ebi, ue_id, pti);

//  /**
//   * Get the UE triggered modification in the TFTs.
//   * This may cause a bearer modification or a release.
//   */
//  mme_app_get_pdn_context(ue_id, pdn_cid, linked_ebi, NULL, &pdn_context);
//  if(!pdn_context){
//    OAILOG_ERROR(LOG_NAS_EMM, "EMMCN-SAP  - " "No PDN context was found for UE " MME_UE_S1AP_ID_FMT" for cid %d and default ebi %d to assign dedicated bearers.\n",
//        ue_id, pdn_cid, linked_ebi);
//    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_PDN_CONNECTION_DOES_NOT_EXIST);
//  }
//
//  nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_connectivity = _esm_proc_get_pdn_connectivity_procedure(ue_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED);
//  if(esm_proc_pdn_connectivity){
//    if(esm_proc_pdn_connectivity->default_ebi == linked_ebi){
//      OAILOG_ERROR(LOG_NAS_EMM, "EMMCN-SAP  - " "A PDN procedure for default ebi %d exists for UE " MME_UE_S1AP_ID_FMT" (cid=%d). Rejecting the establishment of the dedicated bearer.\n",
//          linked_ebi, ue_id, esm_proc_pdn_connectivity->pdn_cid);
//      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
//    } else {
//      OAILOG_WARNING(LOG_NAS_EMM, "EMMCN-SAP  - " "A PDN procedure for default ebi %d exists for UE " MME_UE_S1AP_ID_FMT" (cid=%d). Continuing with establishment of dedicated bearers.\n",
//          esm_proc_pdn_connectivity->default_ebi, ue_id, esm_proc_pdn_connectivity->pdn_cid);
//    }
//  }
//
//  /*
//   * Register a new EPS bearer context into the MME.
//   * This should only be for dedicated bearers (todo: handover with dedicated bearers).
//   */
//  // todo: PCO handling
//  traffic_flow_template_t * tft = bc_tbc->tft;
//  esm_cause = mme_app_register_dedicated_bearer_context(ue_id, ESM_EBR_ACTIVE_PENDING, pdn_context->context_identifier, pdn_context->default_ebi, bc_tbc);  /**< We set the ESM state directly to active in handover. */
//  if(esm_cause != ESM_CAUSE_SUCCESS){
//    OAILOG_ERROR(LOG_NAS_ESM, "ESM-PROC  - Error assigning bearer context for ue " MME_UE_S1AP_ID_FMT ". \n", ue_id);
//    OAILOG_FUNC_RETURN (LOG_NAS_ESM, esm_cause);
//  }
//
//  /*
//   * No need to check for PDN connectivity procedures.
//   * They should be handled together.
//   * Create a new EPS bearer context transaction and starts the timer, since no further CN operation necessary for dedicated bearers.
//   */
//  nas_esm_proc_bearer_context_t * esm_proc_bearer_context = _esm_proc_create_bearer_context_procedure(ue_id, pti, linked_ebi, pdn_cid, bc_tbc->eps_bearer_id, bc_tbc->s1u_sgw_fteid.teid,
//      mme_config.nas_config.t3485_sec, 0 /*usec*/, _dedicated_eps_bearer_activate_t3485_handler);
//  if(!esm_proc_bearer_context){
//    OAILOG_ERROR(LOG_NAS_ESM, "ESM-PROC  - Error creating a new procedure for the bearer context (ebi=%d) for UE " MME_UE_S1AP_ID_FMT ". \n", bc_tbc->eps_bearer_id, ue_id);
//    mme_app_release_bearer_context(ue_id, &pdn_context->context_identifier, pdn_context->default_ebi, bc_tbc->eps_bearer_id);
//    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
//  }
//  /** Send the response message back. */
//  EpsQualityOfService eps_qos = {0};
//  /** Sending a EBR-Request per bearer context. */
//  memset((void*)&eps_qos, 0, sizeof(eps_qos));
//  /** Set the EPS QoS. */
//  qos_params_to_eps_qos(bc_tbc->bearer_level_qos.qci,
//      bc_tbc->bearer_level_qos.mbr.br_dl, bc_tbc->bearer_level_qos.mbr.br_ul,
//      bc_tbc->bearer_level_qos.gbr.br_dl, bc_tbc->bearer_level_qos.gbr.br_ul,
//      &eps_qos, false);
//  esm_send_activate_dedicated_eps_bearer_context_request (
//      PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, bc_tbc->eps_bearer_id,
//      esm_rsp_msg,
//      linked_ebi, &eps_qos,
//      tft,
//      &bc_tbc->pco);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
                Timer handlers
   --------------------------------------------------------------------------
*/

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/


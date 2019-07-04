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
static esm_cause_t _bearer_resource_modification_timeout_handler (nas_esm_proc_t * esm_base_proc, ESM_msg *esm_resp_msg);

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
 **      bearer_level_qos : processed qos values prepared  **
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
  esm_cause_t        esm_cause_received,
//  const traffic_flow_aggregate_description_t * const tad,
  traffic_flow_aggregate_description_t * const tad,
  const EpsQualityOfService                  * const new_flow_qos
  )
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  ebi_t                                   linked_ebi = 0;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  flow_qos_t                              flow_qos;
  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Bearer Resource Modification Request " "(ebi=%d, ue_id=" MME_UE_S1AP_ID_FMT ", pti=%d, esm_cause=%d)\n", ebi, ue_id, pti, esm_cause_received);

  memset(&flow_qos, 0, sizeof(flow_qos_t));

  /** Process the message. */
  if(new_flow_qos && new_flow_qos->qci && (new_flow_qos->bitRatesPresent | new_flow_qos->bitRatesExtPresent)) {
    if(new_flow_qos->bitRatesPresent){
      flow_qos.gbr.br_dl = eps_qos_bit_rate_value(new_flow_qos->bitRates.guarBitRateForDL);
      flow_qos.gbr.br_ul = eps_qos_bit_rate_value(new_flow_qos->bitRates.guarBitRateForUL);
      flow_qos.mbr.br_dl = eps_qos_bit_rate_value(new_flow_qos->bitRates.maxBitRateForDL);
      flow_qos.mbr.br_ul = eps_qos_bit_rate_value(new_flow_qos->bitRates.maxBitRateForUL);
    }
    if(new_flow_qos->bitRatesExtPresent){
      flow_qos.gbr.br_dl += eps_qos_bit_rate_ext_value(new_flow_qos->bitRates.guarBitRateForDL);
      flow_qos.gbr.br_ul += eps_qos_bit_rate_ext_value(new_flow_qos->bitRates.guarBitRateForUL);
      flow_qos.mbr.br_dl += eps_qos_bit_rate_ext_value(new_flow_qos->bitRates.maxBitRateForDL);
      flow_qos.mbr.br_ul += eps_qos_bit_rate_ext_value(new_flow_qos->bitRates.maxBitRateForUL);
    }
    flow_qos.qci = new_flow_qos->qci; /**< If not changed, must be the QCI of the current bearer. */
  }

  /** Verify the received the bearer resource modification request. */
  esm_cause = mme_app_validate_bearer_resource_modification(ue_id, ebi, &linked_ebi, tad, flow_qos.qci ? &flow_qos : NULL);
  if(esm_cause != ESM_CAUSE_SUCCESS) {
     /** If the error is due QoS, we have a valid TFT. And can continue to process it. */
     if(esm_cause != ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION && esm_cause != ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION){
       /** Any other ESM causes should be rejected. */
       OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - Failed to verify the received Bearer Resource Modification Request " "(ebi=%d ue_id=" MME_UE_S1AP_ID_FMT ", pti=%d) with esm_cause %d. \n", ebi, ue_id, pti, esm_cause);
       OAILOG_FUNC_RETURN(LOG_MME_APP, esm_cause);
     }
  }

  /** Create an ESM bearer context but no timer. */
  nas_esm_proc_bearer_context_t * esm_proc_bearer_context = _esm_proc_create_bearer_context_procedure(ue_id, pti, linked_ebi,
       PDN_CONTEXT_IDENTIFIER_UNASSIGNED, ebi, INVALID_TEID, 3, 0, _bearer_resource_modification_timeout_handler);
  if(!esm_proc_bearer_context) {
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - Error creating bearer context ESM procedure for the received Bearer Resource Modification Request " "(ebi=%d, ue_id=" MME_UE_S1AP_ID_FMT ", pti=%d). \n",
        ebi, ue_id, pti);
    OAILOG_FUNC_RETURN(LOG_MME_APP, ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED);
  }

  if(esm_cause != ESM_CAUSE_SUCCESS) {
     /** If the error is due QoS, we have a valid TFT. And can continue to process it. */
     if(esm_cause == ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION){
       DevAssert(!esm_proc_bearer_context->tft);
       esm_proc_bearer_context->tft = calloc(1, sizeof(traffic_flow_template_t));
       copy_traffic_flow_template(esm_proc_bearer_context->tft, tad);
       OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - Since we have a valid TFT operation, ignoring the received flow-qos modification error" "(ebi=%d ue_id=" MME_UE_S1AP_ID_FMT ", pti=%d) with esm_cause %d. \n",
           ebi, ue_id, pti, esm_cause);
       /** Continue, but, in addition to those filters which are in the req/res message, we may also have filters in the TFT.. Need to remove them in the UE, as well.. */
     } else { /* if (esm_cause == ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION) { */
    	 /** Error, we rendered the bearer empty.. continue. */
     }
  }

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Successfully verified the received Bearer Resource Modification Request " "(ebi=%d ue_id=" MME_UE_S1AP_ID_FMT ", pti=%d). \n",
      ebi, ue_id, pti);
  /** Don't start the timer on the ESM procedure. Trigger a Delete Bearer Command message. */
  nas_itti_s11_bearer_resource_cmd(pti, esm_proc_bearer_context->linked_ebi, esm_proc_bearer_context->mme_s11_teid,
      &esm_proc_bearer_context->saegw_s11_fteid, ebi, tad, &flow_qos);
  OAILOG_FUNC_RETURN(LOG_MME_APP, ESM_CAUSE_SUCCESS);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_bearer_resource_failure()               **
 **                                                                        **
 ** Description: Handles Bearer Resource Failure indication from the       **
 **              SAE-GW.                                                   **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                        **
 **          pti:       Procedure Transaction Id                    **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:        None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void
esm_proc_bearer_resource_failure(
  mme_ue_s1ap_id_t           ue_id,
  const proc_tid_t           pti,
  ESM_msg            * const esm_rsp_msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  ebi_t                                   linked_ebi = 0;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Bearer Resource Failure Indication " "(ue_id=" MME_UE_S1AP_ID_FMT ", pti=%d)\n", ue_id, pti);

  nas_esm_proc_bearer_context_t * esm_proc_bearer_context = _esm_proc_get_bearer_context_procedure(ue_id, pti, ESM_EBI_UNASSIGNED);
  if(esm_proc_bearer_context){
    /** Prepare a response message. */
    esm_send_bearer_resource_modification_reject(esm_proc_bearer_context->esm_base_proc.pti, esm_rsp_msg, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
    /** Delete the procedure. */
    _esm_proc_free_bearer_context_procedure(&esm_proc_bearer_context);
    OAILOG_WARNING(LOG_NAS_ESM, "ESM-PROC  - Removed the current ESM procedure for the received Bearer Resource Failure Indication " "(ue_id=" MME_UE_S1AP_ID_FMT ", pti=%d)\n", ue_id, pti);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  OAILOG_WARNING(LOG_NAS_ESM, "ESM-PROC  - No ESM procedure could be found for the received Bearer Resource Failure Indication " "(ue_id=" MME_UE_S1AP_ID_FMT ", pti=%d)\n", ue_id, pti);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
                Timer handlers
   --------------------------------------------------------------------------
*/

/****************************************************************************
 **                                                                        **
 ** Name:    _bearer_resource_modification_timeout_handler()                    **
 **                                                                        **
 ** Description: BRM  timeout handler                                      **
 **                                                                        **
 ** Custom timer, when nothing from the BRM came.
 ** We might have an inconsistent state. 								   **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static esm_cause_t _bearer_resource_modification_timeout_handler (nas_esm_proc_t * esm_base_proc, ESM_msg *esm_resp_msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  nas_esm_proc_bearer_context_t * esm_proc_bearer_context = (nas_esm_proc_bearer_context_t*) esm_base_proc;

  /*
   * The maximum number of activate dedicated EPS bearer context request
   * message retransmission has exceed
   */
  pdn_cid_t                               pid = MAX_APN_PER_UE;
  /*
   * Finalize the bearer (set to ESM_EBR_STATE ACTIVE).
   */
  esm_send_bearer_resource_modification_reject(esm_base_proc->pti, esm_resp_msg, ESM_CAUSE_NETWORK_FAILURE);

  /*
   * Release the transactional PDN connectivity procedure.
   */
  _esm_proc_free_bearer_context_procedure(&esm_proc_bearer_context);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
}

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/

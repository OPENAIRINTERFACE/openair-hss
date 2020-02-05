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
  Source      BearerResourceAllocation.c

  Version     0.1

  Date        2019/01/23

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Dincer Beken

  Description Defines the handling of the UE triggered bearer resource
allocation.

*****************************************************************************/
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bstrlib.h"

#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "assertions.h"
#include "common_defs.h"
#include "common_types.h"
#include "dynamic_memory_check.h"
#include "emm_data.h"
#include "emm_sap.h"
#include "esm_cause.h"
#include "esm_proc.h"
#include "log.h"
#include "mme_app_defs.h"
#include "mme_config.h"

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
 ** Name:    esm_send_bearer_resource_allocation_reject() **
 **                                                                        **
 ** Description: Builds Bearer Resource Allocation Reject message **
 **                                                                        **
 **      The Bearer Resource Allocation Reject message is sent by the net-   **
 **      work to the UE to reject a Bearer Resource Allocation procedure.    **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity             **
 **      esm_cause: ESM cause code                             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void esm_send_bearer_resource_allocation_reject(pti_t pti, ESM_msg *esm_msg,
                                                esm_cause_t esm_cause) {
  OAILOG_FUNC_IN(LOG_NAS_ESM);

  memset(esm_msg, 0, sizeof(ESM_msg));
  /*
   * Mandatory - ESM message header
   */
  esm_msg->bearer_resource_allocation_reject.protocoldiscriminator =
      EPS_SESSION_MANAGEMENT_MESSAGE;
  esm_msg->bearer_resource_allocation_reject.epsbeareridentity =
      EPS_BEARER_IDENTITY_UNASSIGNED;
  esm_msg->bearer_resource_allocation_reject.messagetype =
      BEARER_RESOURCE_ALLOCATION_REJECT;
  esm_msg->bearer_resource_allocation_reject.proceduretransactionidentity = pti;
  /*
   * Mandatory - ESM cause code
   */
  esm_msg->bearer_resource_allocation_reject.esmcause = esm_cause;
  /*
   * Optional IEs
   */
  esm_msg->bearer_resource_allocation_reject.presencemask = 0;
  OAILOG_DEBUG(
      LOG_NAS_ESM,
      "ESM-SAP   - Send Bearer Resource Allocation Reject message "
      "(pti=%d, ebi=%d)\n",
      esm_msg->bearer_resource_allocation_reject.proceduretransactionidentity,
      esm_msg->bearer_resource_allocation_reject.epsbeareridentity);
  OAILOG_FUNC_OUT(LOG_NAS_ESM);
}

/*
   --------------------------------------------------------------------------
      Bearer Resource Allocation Request processing by MME
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_bearer_resource_allocation_request()               **
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
esm_cause_t esm_proc_bearer_resource_allocation_request(
    mme_ue_s1ap_id_t ue_id, const proc_tid_t pti, ebi_t ebi,
    const traffic_flow_aggregate_description_t *const tft) {
  OAILOG_FUNC_IN(LOG_NAS_ESM);
  ebi_t ded_ebi = 0;
  pdn_context_t *pdn_context = NULL;
  esm_cause_t esm_cause = ESM_CAUSE_SUCCESS;
  OAILOG_INFO(LOG_NAS_ESM,
              "ESM-PROC  - Bearer Resource Modification Request "
              "(ebi=%d ue_id=" MME_UE_S1AP_ID_FMT ", pti=%d)\n",
              ebi, ue_id, pti);

  OAILOG_ERROR(LOG_NAS_ESM,
               "ESM-SAP   - Not handling any UE triggered Bearer Resource "
               "Allocation messages (ue_id=" MME_UE_S1AP_ID_FMT "). \n",
               ue_id);

  OAILOG_FUNC_RETURN(LOG_NAS_ESM, ESM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED);
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

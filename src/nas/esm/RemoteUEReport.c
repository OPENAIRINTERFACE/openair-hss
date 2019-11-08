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
Source      RemoteUEReport.c

  Version     0.1

  Date        2019/10/08

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Vyas Mohit

  Description Defines the Remote UE Report procedure executed by the
        Non-Access Stratum.

        The Remote UE Report procedure is used by the UE to inform the 
        network about a newly connected/disconnected off-network UE.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "assertions.h"
#include "log.h"
#include "common_types.h"
#include "conversions.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "3gpp_36.401.h"
#include "mme_app_session_context.h"
#include "common_defs.h"
#include "mme_api.h"
#include "mme_app_apn_selection.h"
#include "mme_app_pdn_context.h"
#include "mme_app_bearer_context.h"
#include "mme_app_defs.h"
#include "esm_data.h"
#include "esm_ebr.h"
#include "esm_proc.h"
#include "esm_pt.h"
#include "esm_cause.h"
#include "esm_sapDef.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
        Remote UE Report procedure executed by the MME
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_remote_ue_report()                                   **
 **                                                                        **
 ** Description: Performs Remote UE Report procedure requested by the UE.  **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.1.3                           **
 **      Upon receipt of the REMOTE UE REPORT message, the                 **
 **      MME checks the off-network UE which got 
 **        connected or disconnected to the network.                       **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                               **
 **      esm_pdn_connectivity_proc: ESM PDN connectivity procedure.        **
 **      Others:    apn_configuration                                      **
 **                                                                        **
 **      Return:    The identifier of the PDN connection if                **
 **             successfully created;                                      **
 **             RETURNerror otherwise.                                     **
 **      Others:                                                  **
 **                                                                        **
 ***************************************************************************/

esm_cause_t
esm_proc_remote_ue_report (
  mme_ue_s1ap_id_t              ue_id,
  const proc_tid_t              pti,
  ebi_t                         ebi,
  //tai_t                       *visited_tai,
  nas_esm_proc_remote_ue_report_t * esm_proc_remote_ue_report_req
  )
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Remote UE Report requested by the UE ""(ebi=%d, ue_id=" MME_UE_S1AP_ID_FMT ", pti=%d)\n", ue_id, pti, ebi);
  OAILOG_ERROR (LOG_NAS_ESM , "ESM-SAP   - Not handling any UE triggered Bearer Resource Allocation messages (ue_id=" MME_UE_S1AP_ID_FMT "). \n", ue_id);

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED);


/*
* Inform the MME_APP procedure to establish a session in the SAE-GW.
*/
  mme_app_send_s11_create_session_req(ue_id, esm_proc_remote_ue_report_req -> bearer_ebi);
  /** No message returned and no timeout set for the ESM procedure. */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);

}

/*
   --------------------------------------------------------------------------
    Internal data handled by the Remote UE Report procedure in the MME
   --------------------------------------------------------------------------
*/

nas_esm_proc_remote_ue_report_t *_esm_proc_get_remote_ue_report_procedure(mme_ue_s1ap_id_t ue_id, pti_t pti){
  return mme_app_nas_esm_get_remote_ue_report_procedure(ue_id, pti);
}

/*
   --------------------------------------------------------------------------
        Remote UE Report Response procedure executed by the MME
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_remote_ue_report_response()                                   **
 **                                                                        **
 ** Description: Performs Remote UE Report Response procedure requested by **
                 the UE.                                                    **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.1.3                           **
 **      Upon receipt of the REMOTE UE REPORT message, the                 **
 **      MME returns REMOTE UE REPORT RESPONSE message.                       **
 **                                                                        **
 ** Inputs:  ue_id:      UE local identifier                               **
 **      esm_pdn_connectivity_proc: ESM PDN connectivity procedure.        **
 **      Others:    apn_configuration                                      **
 **                                                                        **
 **      Return:    The identifier of the PDN connection if                **
 **             successfully created;                                      **
 **             RETURNerror otherwise.                                     **
 **      Others:                                                  **
 **                                                                        **
 ***************************************************************************/
esm_send_remote_ue_report_response ( pti_t pti, ebi_t ebi, ESM_msg * esm_msg )
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  memset(esm_msg, 0, sizeof(ESM_msg));
  /*
   * Mandatory - ESM message header
   */
  esm_msg->remote_ue_report_response.protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  esm_msg->remote_ue_report_response.epsbeareridentity = ebi;
  esm_msg->remote_ue_report_response.messagetype = REMOTE_UE_REPORT_RESPONSE;
  esm_msg->remote_ue_report_response.proceduretransactionidentity = pti;

  /*
   * Optional IEs
   */
  //esm_msg->pdn_connectivity_reject.presencemask = 0;
  OAILOG_DEBUG (LOG_NAS_ESM, "ESM-SAP   - Send Remote UE Report Response message " "(pti=%d, ebi=%d)\n",
      esm_msg->remote_ue_report_response.proceduretransactionidentity, esm_msg->remote_ue_report_response.epsbeareridentity);
  OAILOG_FUNC_OUT(LOG_NAS_ESM);
}
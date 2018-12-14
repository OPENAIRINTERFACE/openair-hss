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
  Source      PdnDisconnect.c

  Version     0.1

  Date        2013/05/15

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Frederic Maurel

  Description Defines the PDN disconnect ESM procedure executed by the
        Non-Access Stratum.

        The PDN disconnect procedure is used by the UE to request
        disconnection from one PDN.

        All EPS bearer contexts established towards this PDN, inclu-
        ding the default EPS bearer context, are released.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "3gpp_36.401.h"
#include "common_defs.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "common_defs.h"
#include "log.h"
#include "assertions.h"

#include "mme_app_pdn_context.h"
#include "emm_sap.h"
#include "esm_data.h"
#include "esm_proc.h"
#include "esm_pt.h"
#include "esm_cause.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
    Internal data handled by the PDN disconnect procedure in the MME
   --------------------------------------------------------------------------
*/
static void _eps_pdn_deactivate_t3495_handler (nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_disconnect, ESM_msg * esm_rsp_msg);

/* Maximum value of the deactivate EPS bearer context request
   retransmission counter */
#define EPS_PDN_CONTEXT_DEACTIVATE_COUNTER_MAX 5

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    esm_send_pdn_disconnect_reject()                          **
 **                                                                        **
 ** Description: Builds PDN Disconnect Reject message                      **
 **                                                                        **
 **      The PDN disconnect reject message is sent by the network  **
 **      to the UE to reject release of a PDN connection.          **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity             **
 **      esm_cause: ESM cause code                             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The ESM message to be sent                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
esm_send_pdn_disconnect_reject (
  pti_t pti,
  ESM_msg * esm_rsp_msg,
  int esm_cause)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  memset(esm_rsp_msg, 0, sizeof(ESM_msg));
  /*
   * Mandatory - ESM message header
   */
  esm_rsp_msg->pdn_disconnect_reject.protocoldiscriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  esm_rsp_msg->pdn_disconnect_reject.epsbeareridentity = EPS_BEARER_IDENTITY_UNASSIGNED;
  esm_rsp_msg->pdn_disconnect_reject.messagetype = PDN_DISCONNECT_REJECT;
  esm_rsp_msg->pdn_disconnect_reject.proceduretransactionidentity = pti;
  /*
   * Mandatory - ESM cause code
   */
  esm_rsp_msg->pdn_disconnect_reject.esmcause = esm_cause;
  /*
   * Optional IEs
   */
  esm_rsp_msg->pdn_disconnect_reject.presencemask = 0;
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Send PDN Disconnect Reject message " "(pti=%d, ebi=%d)\n", esm_rsp_msg->pdn_disconnect_reject.proceduretransactionidentity, esm_rsp_msg->pdn_disconnect_reject.epsbeareridentity);
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
}

/*
   --------------------------------------------------------------------------
          PDN disconnect procedure executed by the MME
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_disconnect_request()                         **
 **                                                                        **
 ** Description: Performs PDN disconnect procedure requested by the UE.    **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.2.3                           **
 **      Upon receipt of the PDN DISCONNECT REQUEST message, if it **
 **      is accepted by the network, the MME shall initiate the    **
 **      bearer context deactivation procedure.                    **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      pti:       Identifies the PDN disconnect procedure    **
 **             requested by the UE                        **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    The identifier of the PDN connection to be **
 **             released, if it exists;                    **
 **             RETURNerror otherwise.                     **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_proc_pdn_disconnect_request (
  mme_ue_s1ap_id_t ue_id,
  proc_tid_t pti,
  pdn_cid_t  pdn_cid,
  nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_disconnect)
{

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - PDN disconnect requested by the UE " "(ue_id=" MME_UE_S1AP_ID_FMT ", default_ebi %d, pti=%d)\n", ue_id, default_ebi, pti);

  pdn_context_t * pdn_context = NULL;
  mme_app_get_pdn_context(ue_id, PDN_CONTEXT_IDENTIFIER_UNASSIGNED, esm_proc_pdn_disconnect->default_ebi, NULL, &pdn_context);
  if(!pdn_context){
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - No PDN context found (after update) (ebi=%d, pti=%d) for UE " MME_UE_S1AP_ID_FMT ".\n", default_ebi, pti, ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_PDN_CONNECTION_DOES_NOT_EXIST);
  }

  /** Update the PDN connectivity procedure with the PDN context information. */
  esm_proc_pdn_disconnect->esm_base_proc.pti = pti;
  esm_proc_pdn_disconnect->pdn_cid = pdn_context->context_identifier;
  if(!esm_proc_pdn_disconnect->subscribed_apn)
    esm_proc_pdn_disconnect->subscribed_apn = bstrcpy(pdn_context->apn_subscribed);

  /*
   * Send deactivate EPS bearer context request message and
   * * * * start timer T3495/T3492
   */
  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Starting T3492 for Deactivate Default EPS bearer context deactivation (ue_id=" MME_UE_S1AP_ID_FMT ", context_identifier=%d)\n",
      ue_id, esm_proc_pdn_disconnect->pdn_cid);
  /** Stop any timer if running. */
  nas_stop_esm_timer(ue_id, &esm_proc_pdn_disconnect->esm_base_proc.esm_proc_timer);
  /** Start the T3485 timer for additional PDN connectivity. */
  esm_proc_pdn_disconnect->esm_base_proc.esm_proc_timer.id = nas_timer_start (esm_proc_pdn_disconnect->esm_base_proc.esm_proc_timer.sec, 0 /*usec*/, false,
      _nas_proc_pdn_connectivity_timeout_handler, ue_id); /**< Address field should be big enough to save an ID. */

  /* Set the timeout handler as the PDN Disconnection handler. */
  esm_proc_pdn_disconnect->esm_base_proc.timeout_notif = _eps_pdn_deactivate_t3495_handler;

  /*
   * Trigger an S11 Delete Session Request to the SAE-GW.
   * No need to process the response.
   */
  nas_itti_pdn_disconnect_req(ue_id, esm_proc_pdn_disconnect->default_ebi, esm_proc_pdn_disconnect->esm_base_proc.pti,
      pdn_context->s_gw_address_s11_s4.address.ipv4_address, pdn_context->s_gw_teid_s11_s4,
      esm_proc_pdn_disconnect);

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_disconnect_accept()                          **
 **                                                                        **
 ** Description: Performs PDN disconnect procedure accepted by the UE.     **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.2.3                           **
 **      On reception of DEACTIVATE EPS BEARER CONTEXT ACCEPT mes- **
 **      sage from the UE, the MME releases all the resources re-  **
 **      served for the PDN in the network.                        **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      pid:       Identifier of the PDN connection to be     **
 **             released                                   **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_cause_t
esm_proc_pdn_disconnect_accept (
  mme_ue_s1ap_id_t ue_id,
  pdn_cid_t pid,
  ebi_t     default_ebi,
  bstring   apn)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - PDN disconnect accepted by the UE " "(ue_id=" MME_UE_S1AP_ID_FMT ", pid=%d)\n", ue_id, pid);
  /*
   * Release the connectivity with the requested PDN
   */
  int                                     rc = mme_api_unsubscribe (NULL);

  /*
   * Delete the MME_APP PDN context entry.
   */
  mme_app_pdn_context_delete(ue_id, pid, default_ebi, apn);

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

/****************************************************************************
 **                                                                        **
 ** Name:    _eps_pdn_deactivate_t3495_handler()                    **
 **                                                                        **
 ** Description: T3495 timeout handler                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 6.4.4.5, case a                   **
 **      On the first expiry of the timer T3495, the MME shall re- **
 **      send the DEACTIVATE EPS BEARER CONTEXT REQUEST and shall  **
 **      reset and restart timer T3495. This retransmission is     **
 **      repeated four times, i.e. on the fifth expiry of timer    **
 **      T3495, the MME shall abort the procedure and deactivate   **
 **      the EPS bearer context locally.                           **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void _eps_pdn_deactivate_t3495_handler (nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_disconnect, ESM_msg * esm_rsp_msg)
{
  OAILOG_FUNC_IN(LOG_NAS_ESM);

  int rc = RETURNok;

  pdn_context_t * pdn_context = NULL;
  mme_app_get_pdn_context(esm_proc_pdn_disconnect->esm_base_proc.ue_id, esm_proc_pdn_disconnect->pdn_cid,
      esm_proc_pdn_disconnect->default_ebi, esm_proc_pdn_disconnect->subscribed_apn, &pdn_context);

  esm_proc_pdn_disconnect->esm_base_proc.retx_count+= 1;
  if (esm_proc_pdn_disconnect->esm_base_proc.retx_count < EPS_PDN_CONTEXT_DEACTIVATE_COUNTER_MAX) {
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - T3492 timer expired (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d), " "retransmission counter = %d\n",
        esm_proc_pdn_disconnect->esm_base_proc.ue_id, esm_proc_pdn_disconnect->default_ebi, esm_proc_pdn_disconnect->esm_base_proc.retx_count);

    /*
     * Create a new ESM-Information request and restart the timer.
     * Keep the ESM transaction.
     */
    if(pdn_context){
      bearer_context_t * bearer_context = NULL;
      mme_app_get_session_bearer_context(pdn_context, esm_proc_pdn_disconnect->default_ebi);
      if(bearer_context){
        rc = esm_send_deactivate_eps_bearer_context_request(esm_proc_pdn_disconnect->esm_base_proc.pti,
            esm_proc_pdn_disconnect->default_ebi, esm_rsp_msg, ESM_CAUSE_REGULAR_DEACTIVATION);
        if (rc != RETURNerror) {
          rc = esm_proc_pdn_disconnect_request (esm_proc_pdn_disconnect->esm_base_proc.ue_id, esm_proc_pdn_disconnect->default_ebi, esm_proc_pdn_disconnect);
          OAILOG_FUNC_RETURN(LOG_NAS_ESM, rc);
        }
      }
    }
  }

  OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - T3492 timer expired (ue_id=" MME_UE_S1AP_ID_FMT ", ebi=%d), " "retransmission counter = %d\n",
       esm_proc_pdn_disconnect->esm_base_proc.ue_id, esm_proc_pdn_disconnect->default_ebi, esm_proc_pdn_disconnect->esm_base_proc.retx_count);

  /*
   * Check if it is the default EBI, if so remove the PDN context.
   */
  /* Deactivate the bearer/pdn context implicitly. */
  esm_proc_pdn_disconnect_accept(esm_proc_pdn_disconnect->esm_base_proc.ue_id, esm_proc_pdn_disconnect->pdn_cid, esm_proc_pdn_disconnect->default_ebi,
      esm_proc_pdn_disconnect->subscribed_apn);
  _esm_proc_free_pdn_connectivity_procedure(&esm_proc_pdn_disconnect);
  OAILOG_FUNC_OUT (LOG_NAS_ESM);
}

/*
  ---------------------------------------------------------------------------
                PDN disconnection handlers
  ---------------------------------------------------------------------------
*/

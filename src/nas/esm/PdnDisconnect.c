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
void
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
  OAILOG_FUNC_OUT(LOG_NAS_ESM);
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
  ebi_t linked_ebi)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  nas_esm_proc_pdn_connectivity_t * esm_proc_pdn_disconnect = NULL;
  pdn_context_t * pdn_context = NULL;

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - PDN disconnect requested by the UE " "(ue_id=" MME_UE_S1AP_ID_FMT ", default_ebi %d, pti=%d)\n", ue_id, linked_ebi, pti);
  mme_app_get_pdn_context(ue_id, pdn_cid, linked_ebi, NULL, &pdn_context);
  if(!pdn_context){
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - No PDN context found (after update) (ebi=%d, pti=%d) for UE " MME_UE_S1AP_ID_FMT ".\n", linked_ebi, pti, ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_PDN_CONNECTION_DOES_NOT_EXIST);
  }

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Creating an ESM procedure and starting T3495 for Deactivate Default EPS bearer context deactivation (ue_id=" MME_UE_S1AP_ID_FMT ", context_identifier=%d)\n",
      ue_id, pdn_context->context_identifier);
  /** Create a procedure, but don't start the timer yet. */
  esm_proc_pdn_disconnect = _esm_proc_create_pdn_connectivity_procedure(ue_id, NULL, pti);
  esm_proc_pdn_disconnect->default_ebi = pdn_context->default_ebi;
  /** Update the PDN connectivity procedure with the PDN context information. */
  esm_proc_pdn_disconnect->pdn_cid = pdn_context->context_identifier;
  if(esm_proc_pdn_disconnect->subscribed_apn)
    bdestroy_wrapper(&esm_proc_pdn_disconnect->subscribed_apn);
  esm_proc_pdn_disconnect->subscribed_apn = bstrcpy(pdn_context->apn_subscribed);
  /*
   * Trigger an S11 Delete Session Request to the SAE-GW.
   * No need to process the response.
   */

  struct in_addr saegw_peer_ipv4 = pdn_context->s_gw_address_s11_s4.address.ipv4_address;
  if(saegw_peer_ipv4.s_addr == 0){
    mme_app_select_service(&esm_proc_pdn_disconnect->visited_tai, &saegw_peer_ipv4);
  }

  nas_itti_pdn_disconnect_req(ue_id, pdn_context->default_ebi, pti, false, false,
      saegw_peer_ipv4, pdn_context->s_gw_teid_s11_s4,
      pdn_cid);

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_CAUSE_SUCCESS);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_detach_request()                         **
 **                                                                        **
 ** Description: Performs a local detach, removing all PDN contexts.       **
 **                                                                        **
 **              Triggers an internal local detach procedure for all ESM   **
 **              contexts.                                                 **
 **                                                                 **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      Others:    None                                            **
 **                                                                        **
 ** Outputs:     None                                               **
 **                                                                        **
 ***************************************************************************/
void
esm_proc_detach_request (
  mme_ue_s1ap_id_t ue_id, bool clr)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Detach requested by the UE " "(ue_id=" MME_UE_S1AP_ID_FMT ")\n", ue_id);

  ue_context_t        * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  pdn_context_t       * pdn_context = NULL;

  if(!ue_context){
    OAILOG_WARNING(LOG_MME_APP, "No UE context could be found for UE: " MME_UE_S1AP_ID_FMT " to release ESM contexts. \n", ue_id);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }

  /**
   * Trigger all S11 messages, wihtouth removing the context.
   * Todo: check what happens, if the transactions stay but s11 tunnel is removed.
   */
  RB_FOREACH (pdn_context, PdnContexts, &ue_context->pdn_contexts) {
    DevAssert(pdn_context);
    // todo: check this!
    bool deleteTunnel = (RB_MIN(PdnContexts, &ue_context->pdn_contexts)== pdn_context);

    /*
     * Trigger an S11 Delete Session Request to the SAE-GW.
     * No need to process the response.
     */
    if(pdn_context->s_gw_address_s11_s4.address.ipv4_address.s_addr != 0){
      nas_itti_pdn_disconnect_req(ue_id, pdn_context->default_ebi, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, deleteTunnel, clr,
          pdn_context->s_gw_address_s11_s4.address.ipv4_address, pdn_context->s_gw_teid_s11_s4, pdn_context->context_identifier);
    }
  }
  OAILOG_INFO(LOG_MME_APP, "Triggered session deletion for all session. Removing ESM context of UE: " MME_UE_S1AP_ID_FMT " . \n", ue_id);
  mme_app_esm_detach(ue_id);

  // Notify MME APP to remove the remaining MME_APP and S1AP contexts.. The tunnel endpoints might or might not be deleted at this time.
  nas_itti_detach_req(ue_id);

  OAILOG_FUNC_OUT(LOG_NAS_ESM);
}
/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/


/*
  ---------------------------------------------------------------------------
                PDN disconnection handlers
  ---------------------------------------------------------------------------
*/

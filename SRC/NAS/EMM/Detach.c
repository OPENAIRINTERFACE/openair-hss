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
  Source      Detach.c

  Version     0.1

  Date        2013/05/07

  Product     NAS stack

  Subsystem   EPS Mobility Management

  Author      Frederic Maurel

  Description Defines the detach related EMM procedure executed by the
        Non-Access Stratum.

        The detach procedure is used by the UE to detach for EPS servi-
        ces, to disconnect from the last PDN it is connected to; by the
        network to inform the UE that it is detached for EPS services
        or non-EPS services or both, to disconnect the UE from the last
        PDN to which it is connected and to inform the UE to re-attach
        to the network and re-establish all PDN connections.

*****************************************************************************/

#include <stdlib.h>
#include "log.h"
#include "msc.h"
#include "dynamic_memory_check.h"
#include "nas_timer.h"
#include "emmData.h"
#include "emm_proc.h"
#include "emm_sap.h"
#include "esm_sap.h"
#include "nas_itti_messaging.h" 


/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* String representation of the detach type */
static const char                      *_emm_detach_type_str[] = {
  "EPS", "IMSI", "EPS/IMSI",
  "RE-ATTACH REQUIRED", "RE-ATTACH NOT REQUIRED", "RESERVED"
};

/*
   --------------------------------------------------------------------------
        Internal data handled by the detach procedure in the UE
   --------------------------------------------------------------------------
*/


/*
   --------------------------------------------------------------------------
        Internal data handled by the detach procedure in the MME
   --------------------------------------------------------------------------
*/


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
            Detach procedure executed by the UE
   --------------------------------------------------------------------------
*/

/*
   --------------------------------------------------------------------------
            Detach procedure executed by the MME
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_detach()                                         **
 **                                                                        **
 ** Description: Initiate the detach procedure to inform the UE that it is **
 **      detached for EPS services, or to re-attach to the network **
 **      and re-establish all PDN connections.                     **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.2.3.1                         **
 **      In state EMM-REGISTERED the network initiates the detach  **
 **      procedure by sending a DETACH REQUEST message to the UE,  **
 **      starting timer T3422 and entering state EMM-DEREGISTERED- **
 **      INITIATED.                                                **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      type:      Type of the requested detach               **
 **      Others:    _emm_detach_type_str                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3422                                      **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_detach (
  mme_ue_s1ap_id_t ue_id,
  emm_proc_detach_type_t type)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Initiate detach type = %s (%d)", _emm_detach_type_str[type], type);
  /*
   * TODO
   */
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_detach_request()                                 **
 **                                                                        **
 ** Description: Performs the UE initiated detach procedure for EPS servi- **
 **      ces only When the DETACH REQUEST message is received by   **
 **      the network.                                              **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.2.2.2                         **
 **      Upon receiving the DETACH REQUEST message the network     **
 **      shall send a DETACH ACCEPT message to the UE and store    **
 **      the current EPS security context, if the detach type IE   **
 **      does not indicate "switch off". Otherwise, the procedure  **
 **      is completed when the network receives the DETACH REQUEST **
 **      message.                                                  **
 **      The network shall deactivate the EPS bearer context(s)    **
 **      for this UE locally without peer-to-peer signalling and   **
 **      shall enter state EMM-DEREGISTERED.                       **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      type:      Type of the requested detach               **
 **      switch_off:    Indicates whether the detach is required   **
 **             because the UE is switched off or not      **
 **      native_ksi:    true if the security context is of type    **
 **             native                                     **
 **      ksi:       The NAS ket sey identifier                 **
 **      guti:      The GUTI if provided by the UE             **
 **      imsi:      The IMSI if provided by the UE             **
 **      imei:      The IMEI if provided by the UE             **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_detach_request (
  mme_ue_s1ap_id_t ue_id,
  emm_proc_detach_type_t type,
  int switch_off,
  ksi_t native_ksi,
  ksi_t ksi,
  guti_t * guti,
  imsi_t * imsi,
  imei_t * imei)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc;
  emm_data_context_t                     *emm_ctx = NULL;

  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Detach type = %s (%d) requested (ue_id=" MME_UE_S1AP_ID_FMT ")", _emm_detach_type_str[type], type, ue_id);
  /*
   * Get the UE context
   */
  emm_ctx = emm_data_context_get (&_emm_data, ue_id);

  if (emm_ctx == NULL) {
    OAILOG_WARNING (LOG_NAS_EMM, "No EMM context exists for the UE (ue_id=" MME_UE_S1AP_ID_FMT ")", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
  }

  if (switch_off) {
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 Removing UE context ue id " MME_UE_S1AP_ID_FMT " ", ue_id);

    /*
     * The UE is switched off
     */
    emm_ctx_clear_old_guti(emm_ctx);
    emm_ctx_clear_guti(emm_ctx);
    emm_ctx_clear_imsi(emm_ctx);
    emm_ctx_clear_imei(emm_ctx);
    emm_ctx_clear_auth_vectors(emm_ctx);
    emm_ctx_clear_security(emm_ctx);
    emm_ctx_clear_non_current_security(emm_ctx);


    if (emm_ctx->esm_msg) {
      bdestroy (emm_ctx->esm_msg);
    }

    /*
     * Stop timer T3450
     */
    if (emm_ctx->T3450.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3450 (%d)", emm_ctx->T3450.id);
      emm_ctx->T3450.id = nas_timer_stop (emm_ctx->T3450.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3450 stopped UE " MME_UE_S1AP_ID_FMT " ", emm_ctx->ue_id);
    }

    /*
     * Stop timer T3460
     */
    if (emm_ctx->T3460.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3460 (%d)", emm_ctx->T3460.id);
      emm_ctx->T3460.id = nas_timer_stop (emm_ctx->T3460.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3460 stopped UE " MME_UE_S1AP_ID_FMT " ", emm_ctx->ue_id);
    }

    /*
     * Stop timer T3470
     */
    if (emm_ctx->T3470.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3470 (%d)", emm_ctx->T3460.id);
      emm_ctx->T3470.id = nas_timer_stop (emm_ctx->T3470.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 T3470 stopped UE " MME_UE_S1AP_ID_FMT " ", emm_ctx->ue_id);
    }

    /*
     * Release the EMM context
     */
    emm_data_context_remove (&_emm_data, emm_ctx);
    free_wrapper (emm_ctx);

    rc = RETURNok;
  } else {
    /*
     * Normal detach without UE switch-off
     */
    emm_sap_t                               emm_sap = {0};
    emm_as_data_t                          *emm_as = &emm_sap.u.emm_as.u.data;

    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMM_AS_NAS_INFO_DETACH ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    /*
     * Setup NAS information message to transfer
     */
    emm_as->nas_info = EMM_AS_NAS_INFO_DETACH;
    emm_as->nas_msg = NULL;
    /*
     * Set the UE identifier
     */
    emm_ctx_clear_old_guti(emm_ctx);
    emm_ctx_clear_guti(emm_ctx);
    emm_as->ue_id = ue_id;
    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data (&emm_as->sctx, &emm_ctx->_security, false, true);
    /*
     * Notify EMM-AS SAP that Detach Accept message has to
     * be sent to the network
     */
    emm_sap.primitive = EMMAS_DATA_REQ;
    rc = emm_sap_send (&emm_sap);
  }

  if (rc != RETURNerror) {
    /*
     * Notify ESM that all EPS bearer contexts allocated for this UE have
     * to be locally deactivated
     */
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    esm_sap_t                               esm_sap = {0};

    esm_sap.primitive = ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ;
    esm_sap.ue_id = ue_id;
    esm_sap.ctx = emm_ctx;
    esm_sap.data.eps_bearer_context_deactivate.ebi = ESM_SAP_ALL_EBI;
    rc = esm_sap_send (&esm_sap);

    if (rc != RETURNerror) {
      emm_sap_t                               emm_sap = {0};

      /*
       * Notify EMM that the UE has been implicitly detached
       */
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_DETACH_REQ ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
      emm_sap.primitive = EMMREG_DETACH_REQ;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx = emm_ctx;
      rc = emm_sap_send (&emm_sap);
    }
	
    nas_itti_detach_req(ue_id);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

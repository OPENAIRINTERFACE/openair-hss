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
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"
#include "log.h"
#include "msc.h"
#include "gcc_diag.h"
#include "dynamic_memory_check.h"
#include "assertions.h"
#include "common_types.h"

#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"

#include "emm_data.h"
#include "emm_proc.h"
#include "emm_sap.h"
#include "esm_sap.h"
#include "nas_timer.h"

#include "mme_app_ue_context.h"
#include "nas_itti_messaging.h" 
#include "mme_app_defs.h"

static void _emm_proc_create_procedure_detach_request(emm_data_context_t * const emm_context, emm_detach_request_ies_t * const ies);

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

//#include "s1ap_mme.h"

void
_clear_emm_ctxt(mme_ue_s1ap_id_t ue_id) {
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_data_context_t                     *emm_context = emm_data_context_get (&_emm_data, ue_id);
  if(!emm_context){
    OAILOG_DEBUG(LOG_NAS_EMM, "No EMM context was found to clear for ue_id " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_OUT(LOG_NAS_EMM);
  }
  if(!emm_context->is_dynamic) {
    OAILOG_DEBUG(LOG_NAS_EMM, "Cannot clear not-dynamic EMM context for ue_id " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_OUT(LOG_NAS_EMM);
  }
//  if(s1ap_is_ue_mme_id_in_list(emm_context->ue_id)ue_ref){
//    OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC  -  * * * * * (0) ueREF %p has mmeId " MME_UE_S1AP_ID_FMT ", enbId " ENB_UE_S1AP_ID_FMT " state %d and eNB_ref %p. \n",
//             ue_ref, ue_ref->mme_ue_s1ap_id, ue_ref->enb_ue_s1ap_id, ue_ref->s1_ue_state, ue_ref->enb);
//  }

//  LOCK_EMM_CONTEXT(emm_context);
  // todo: check if necessary!
  nas_delete_all_emm_procedures(emm_context);
//  if(ue_ref){
//    OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC  -  * * * * * (1) ueREF %p has mmeId " MME_UE_S1AP_ID_FMT ", enbId " ENB_UE_S1AP_ID_FMT " state %d and eNB_ref %p. \n",
//            ue_ref, ue_ref->mme_ue_s1ap_id, ue_ref->enb_ue_s1ap_id, ue_ref->s1_ue_state, ue_ref->enb);
//  }

  emm_data_context_remove(&_emm_data, emm_context);
  emm_ctx_clear_old_guti(emm_context);
  emm_ctx_clear_guti(emm_context);
  emm_ctx_clear_imsi(emm_context);
  emm_ctx_clear_imei(emm_context);
  emm_ctx_clear_auth_vectors(emm_context);
  emm_ctx_clear_security(emm_context);
  emm_ctx_clear_non_current_security(emm_context);
  /*
   * Release the EMM context
   */
  // todo: need to unlock freed context?!
//  UNLOCK_EMM_CONTEXT(emm_context);
  free_wrapper((void **) &emm_context);
  OAILOG_FUNC_OUT(LOG_NAS_EMM);
}

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
  emm_proc_detach_type_t detach_type,
  int emm_cause,
  bool clr)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Initiate detach type = %s (%d) for ueId " MME_UE_S1AP_ID_FMT " \n.", _emm_detach_type_str[detach_type], detach_type, ue_id);

  emm_data_context_t                     *emm_context = emm_data_context_get (&_emm_data, ue_id);
  ue_context_t                           *ue_context  = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ue_id);
  // todo: review this for implicit detach
  if(!ue_context){
    if(emm_context){
      OAILOG_ERROR(LOG_NAS_EMM, "EMM-PROC  -  * * * * * NO UE CONTEXT FOUND FOR IMPLICIT DETACH OF UE WITH ueID " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT ". \n", emm_context->ue_id, emm_context->_imsi64);
    }else{
      OAILOG_ERROR(LOG_NAS_EMM, "EMM-PROC  -  * * * * * NO EMM CONTEXT FOUND FOR IMPLICIT DETACH OF UE WITH ueID " MME_UE_S1AP_ID_FMT " and IMSI " IMSI_64_FMT ". \n", ue_id, NULL);
    }
  }

  /*
   * Get the UE context
   */
  if (emm_context == NULL) {
    OAILOG_WARNING (LOG_NAS_EMM, "No EMM context exists for the UE (ue_id=" MME_UE_S1AP_ID_FMT ")", ue_id);
    // There may be MME APP Context. Trigger clean up in MME APP
    nas_itti_esm_detach_ind(ue_id, clr);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
  }
  /*
   * Set state as DEREGISTER initiated
   * todo: check if this state change is valid!
   */
  if(emm_fsm_set_state (ue_id, emm_context, EMM_DEREGISTERED_INITIATED) == RETURNerror){
    OAILOG_WARNING (LOG_NAS_EMM, "An implicit detach procedure is already ongoing for EMM context of the UE (ue_id=" MME_UE_S1AP_ID_FMT "). Aborting new implicit detach.", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
  }

  /**
    * Although MME_APP and EMM contexts are separated, doing it like this causes the recursion problem.
    *
    * TS 23.401: 5.4.4.1 -> (Although dedicated bearers, works for me).
    *
    * If all the bearers belonging to a UE are released, the MME shall change the MM state of the UE to EMM-
    * DEREGISTERED and the MME sends the S1 Release Command to the eNodeB, which initiates the release of the RRC
    * connection for the given UE if it is not released yet, and returns an S1 Release Complete message to the MME.
    *
    * Don't do it recursively over MME_APP, do it over the EMM/ESM, that's what they're for.
    * So just triggering the ESM (before purging the context, to remove/purge all the PDN connections.
    *
    */

  /** Check if there are any active sessions, if so terminate them all. */
  OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Detach UE \n", emm_context->ue_id);
  /*
   * 3GPP TS 24.401, Figure 5.3.2.1-1, point 5a
   * At this point, all NAS messages shall be protected by the NAS security
   * functions (integrity and ciphering) indicated by the MME unless the UE
   * is emergency attached and not successfully authenticated.
   */

  if(detach_type) {
    OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Detach type is set for UE. We will send detach request. \n", emm_context->ue_id);

    emm_sap_t                               emm_sap = {0};
    emm_as_data_t                          *emm_as = &emm_sap.u.emm_as.u.data;

    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMM_AS_NAS_INFO_DETACH_REQUEST ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    /*
     * Setup NAS information message to transfer
     */
    emm_as->nas_info = EMM_AS_NAS_INFO_DETACH_REQ;
    emm_as->nas_msg  = NULL;
    /*
     * Set the UE identifier
     */
    emm_as->ue_id = ue_id;
    /*
     * Detach Type
     */
    emm_as->type = detach_type;
    /*
     * EMM Cause
     */
    emm_as->emm_cause = emm_cause;

    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data (&emm_as->sctx, &emm_context->_security, false, true);
    /*
     * Notify EMM-AS SAP that Detach Accept message has to
     * be sent to the network
     */
    emm_sap.primitive = EMMAS_DATA_REQ;
    rc = emm_sap_send (&emm_sap);
  }else{
    OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - No detach type is set for UE. We will not send detach request. \n", emm_context->ue_id);
    rc = RETURNok;
  }
  /*
   * Release ESM PDN and bearer context
   */

  // todo: this might be in success_notif of detach_proc
  if (rc != RETURNerror) {
    /** Confirm the detach procedure and inform MME_APP. */

    emm_sap_t                               emm_sap = {0};

    /*
     * Notify EMM FSM that the UE has been implicitly detached
      */
     MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_DETACH_CNF ue id " MME_UE_S1AP_ID_FMT " ", emm_context->ue_id);
     OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Sending EMMREG_DETACH_CNF. \n", emm_context->ue_id);
     emm_sap.primitive = EMMREG_DETACH_CNF;
     emm_sap.u.emm_reg.ue_id = emm_context->ue_id;
     emm_sap.u.emm_reg.ctx = emm_context;
     rc = emm_sap_send (&emm_sap);
     // Notify MME APP to remove the remaining MME_APP and S1AP contexts..
     // todo: review unlock

     /** Signal detach Free all ESM procedure, don't care about the rest. */
     nas_itti_esm_detach_ind(ue_id, clr);

     //     unlock_ue_contexts(ue_context);
     OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
     // todo: review this
   }
   //  unlock_ue_contexts(ue_context);
   OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
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
  emm_detach_request_ies_t * params)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc;
  bool                                    switch_off = params->switch_off;

  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Detach type = %s (%d) requested (ue_id=" MME_UE_S1AP_ID_FMT ")\n", _emm_detach_type_str[params->type], params->type, ue_id);
  /*
   * Get the UE context
   */
  emm_data_context_t *emm_context = emm_data_context_get( &_emm_data, ue_id);
  ue_context_t       *ue_context  = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ue_id);

  /** First, always send a Detach Accept back if not switch-off. */
  if (switch_off) {
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "0 Removing UE context ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
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
    emm_as->ue_id = ue_id;
    /*
     * Setup EPS NAS security data
     */
    if(emm_context){
      emm_as_set_security_data (&emm_as->sctx, &emm_context->_security, false, true);
    }else{
      OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Not setting security context for detach accept. \n", ue_id);
    }
    /*
     * Notify EMM-AS SAP that Detach Accept message has to
     * be sent to the network
     */
    emm_sap.primitive = EMMAS_DATA_REQ;
    rc = emm_sap_send (&emm_sap);
  }

  if (emm_context) {
    DevAssert(ue_context); /**< Might be implicitly detached. */
    /*
     * Validate, just check if a specific procedure exist, if so disregard the detach request.
     * Assuming that DSR failed, with the timeout, we should send one without a security header.
     */
    nas_emm_specific_proc_t * specific_proc = get_nas_specific_procedure(emm_context);
    if(specific_proc){
      /** A specific procedure exists, abort the detach request, wait the specific procedure to complete. */

      if(specific_proc->emm_proc.type == EMM_SPEC_PROC_TYPE_ATTACH){
        OAILOG_INFO (LOG_NAS_EMM, " EMM-PROC  - An attach procedure for ueId " MME_UE_S1AP_ID_FMT " exists. "
            "Aborting the attach procedure and continuing with the detach request. \n", emm_context->ue_id, specific_proc->type);

        emm_sap_t                               emm_sap = {0};
        emm_sap.primitive = EMMREG_ATTACH_ABORT;
        emm_sap.u.emm_reg.ue_id = ue_id;
        emm_sap.u.emm_reg.ctx = emm_context;
        emm_sap.u.emm_reg.u.attach.proc = specific_proc;
        rc = emm_sap_send (&emm_sap);
        //     unlock_ue_contexts(ue_context);
        free_emm_detach_request_ies(&params);
        OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);

      } else if(specific_proc->emm_proc.type == EMM_SPEC_PROC_TYPE_TAU){
        OAILOG_INFO (LOG_NAS_EMM, " EMM-PROC  - A TAU procedure for ueId " MME_UE_S1AP_ID_FMT " exists. "
            "Aborting the attach procedure and continuing with the detach request. \n", emm_context->ue_id, specific_proc->type);
          // todo
        DevAssert(0);
      } else {
        OAILOG_ERROR(LOG_NAS_EMM, " EMM-PROC  - A detach procedure for ueId " MME_UE_S1AP_ID_FMT " exists. "
            "This should not happen. \n", emm_context->ue_id, specific_proc->type);
        // todo
        DevAssert(0);
      }
    }
    /** Create a specific procedure for detach request. */
    _emm_proc_create_procedure_detach_request(emm_context, params);
  }else{
    /** No EMM context existing. Need to remove the procedure IEs immediately. */
    rc = RETURNok;
    // free content
    free_emm_detach_request_ies(&params);
  }

  /** Confirm the detach procedure and inform MME_APP. */
  emm_sap_t                               emm_sap = {0};
  /*
   * Notify EMM FSM that the UE has been implicitly detached
   */
  MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_DETACH_CNF ue id " MME_UE_S1AP_ID_FMT " ", ue_id);

  OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Sending EMMREG_DETACH_CNF. \n", ue_id);

  emm_sap.primitive = EMMREG_DETACH_CNF;
  emm_sap.u.emm_reg.ue_id = ue_id;
  emm_sap.u.emm_reg.ctx = emm_context;
  rc = emm_sap_send (&emm_sap);

  /** Signal detach Free all ESM procedure, don't care about the rest. */
  nas_itti_esm_detach_ind(ue_id, false);

  //     unlock_ue_contexts(ue_context);
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
void free_emm_detach_request_ies(emm_detach_request_ies_t ** const ies)
{
  if ((*ies)->guti) {
    free_wrapper((void**)&(*ies)->guti);
  }
  if ((*ies)->imsi) {
    free_wrapper((void**)&(*ies)->imsi);
  }
  if ((*ies)->imei) {
    free_wrapper((void**)&(*ies)->imei);
  }
  free_wrapper((void**)ies);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

static void _emm_proc_create_procedure_detach_request(emm_data_context_t * const emm_context, emm_detach_request_ies_t * const ies)
{
  nas_emm_detach_proc_t *detach_proc = nas_new_detach_procedure(emm_context);
  AssertFatal(detach_proc, "TODO Handle this");
  if ((detach_proc)) {
    detach_proc->ies = ies;
    /** No success notification needed. Everything will be handled in the states with EMMREG messages. */
  }
}

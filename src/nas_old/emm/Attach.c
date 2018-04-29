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

  Source      Attach.c

  Version     0.1

  Date        2012/12/04

  Product     NAS stack

  Subsystem   EPS Mobility Management

  Author      Frederic Maurel

  Description Defines the attach related EMM procedure executed by the
        Non-Access Stratum.

        To get internet connectivity from the network, the network
        have to know about the UE. When the UE is switched on, it
        has to initiate the attach procedure to get initial access
        to the network and register its presence to the Evolved
        Packet Core (EPC) network in order to receive EPS services.

        As a result of a successful attach procedure, a context is
        created for the UE in the MME, and a default bearer is esta-
        blished between the UE and the PDN-GW. The UE gets the home
        agent IPv4 and IPv6 addresses and full connectivity to the
        IP network.

        The network may also initiate the activation of additional
        dedicated bearers for the support of a specific service.

*****************************************************************************/

#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "gcc_diag.h"
#include "dynamic_memory_check.h"
#include "assertions.h"
#include "log.h"
#include "msc.h"
#include "nas_timer.h"
#include "common_types.h"
#include "3gpp_24.008.h"
#include "3gpp_36.401.h"
#include "3gpp_29.274.h"
#include "conversions.h"
#include "3gpp_requirements_24.301.h"
#include "nas_message.h"
#include "as_message.h"
#include "mme_app_ue_context.h"
#include "emm_proc.h"
#include "networkDef.h"
#include "emm_sap.h"
#include "mme_api.h"
#include "emm_data.h"
#include "esm_proc.h"
#include "esm_sapDef.h"
#include "esm_sap.h"
#include "emm_cause.h"
#include "NasSecurityAlgorithms.h"
#include "mme_config.h"
#include "nas_itti_messaging.h"
#include "mme_app_defs.h"


/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* String representation of the EPS attach type */
static const char                      *_emm_attach_type_str[] = {
  "EPS", "IMSI", "EMERGENCY", "RESERVED"
};


/*
   --------------------------------------------------------------------------
        Internal data handled by the attach procedure in the MME
   --------------------------------------------------------------------------
*/
/*
   Timer handlers
*/
static void                            *_emm_attach_t3450_handler (void *);

/*
   Functions that may initiate EMM common procedures
*/
static int                              _emm_attach_identify (emm_data_context_t *emm_context);
static int                              _emm_attach_security (emm_data_context_t *emm_context);
static int                              _emm_attach (emm_data_context_t *emm_context);

/*
   Abnormal case attach procedures
*/
static int                              _emm_attach_release (emm_data_context_t *emm_context);
int                                     _emm_attach_reject (emm_data_context_t *emm_context);
static int                              _emm_attach_abort (emm_data_context_t *emm_context);

static int                              _emm_attach_have_changed (
  const emm_data_context_t * ctx,
  emm_proc_attach_type_t type,
  ksi_t ksi,
  guti_t * guti,
  imsi_t * imsi,
  imei_t * imei,
  int eea,
  int eia,
  int ucs2,
  int uea,
  int uia,
  int gea,
  int umts_present,
  int gprs_present);

static int
_emm_attach_update (
  emm_data_context_t * emm_context,
  mme_ue_s1ap_id_t ue_id,
  emm_proc_attach_type_t type,
  ksi_t ksi,
  bool is_native_guti,
  guti_t * guti,
  imsi_t * imsi,
  imei_t * imei,
  const tai_t   * const originating_tai,
  const int eea,
  const int eia,
  const int ucs2,
  const int uea,
  const int uia,
  const int gea,
  const bool umts_present,
  const bool gprs_present,
  const_bstring esm_msg_pP);


static int _emm_attach_accept (emm_data_context_t * emm_context);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/


/*
   --------------------------------------------------------------------------
            Attach procedure executed by the MME
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_attach_request()                                 **
 **                                                                        **
 ** Description: Performs the UE requested attach procedure                **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.3                         **
 **      The network may initiate EMM common procedures, e.g. the  **
 **      identification, authentication and security mode control  **
 **      procedures during the attach procedure, depending on the  **
 **      information received in the ATTACH REQUEST message (e.g.  **
 **      IMSI, GUTI and KSI).                                      **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      type:      Type of the requested attach               **
 **      native_ksi:    true if the security context is of type    **
 **             native (for KSIASME)                       **
 **      ksi:       The NAS ket sey identifier                 **
 **      native_guti:   true if the provided GUTI is native GUTI   **
 **      guti:      The GUTI if provided by the UE             **
 **      imsi:      The IMSI if provided by the UE             **
 **      imei:      The IMEI if provided by the UE             **
 **      last_visited_registered_tai:       Identifies the last visited tracking area  **
 **             the UE is registered to                    **
 **      eea:       Supported EPS encryption algorithms        **
 **      eia:       Supported EPS integrity algorithms         **
 **      esm_msg_pP:   PDN connectivity request ESM message       **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_attach_request (
  mme_ue_s1ap_id_t ue_id,
  emm_proc_attach_type_t type,
  AdditionalUpdateType additional_update_type,
  bool is_native_ksi,
  ksi_t     ksi,
  bool is_native_guti,
  guti_t * guti,
  imsi_t * imsi,
  imei_t * imei,
  tai_t * last_visited_registered_tai,
  const tai_t   * const originating_tai,
  const ecgi_t   * const originating_ecgi,
  const int eea,
  const int eia,
  const int ucs2,
  const int uea,
  const int uia,
  const int gea,
  const bool umts_present,
  const bool gprs_present,
  const_bstring esm_msg_pP,
  const nas_message_decode_status_t  * const decode_status)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  emm_fsm_state_t                         fsm_state = EMM_DEREGISTERED;
  bool                                    duplicate_enb_context_detected = false;
  emm_data_context_t                           ue_ctx  = NULL;
  emm_data_context_t                           *new_emm_ctx  = NULL;
  ue_mm_context_t                         *ue_mm_ctx_p = NULL;
  imsi64_t                                imsi64  = INVALID_IMSI64;

  if (imsi) {
    imsi64 = imsi_to_imsi64(imsi);
  }

  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  ATTACH - EPS attach type = %s (%d) initial %u requested (ue_id=" MME_UE_S1AP_ID_FMT ")\n",
      _emm_attach_type_str[type], type, is_initial, ue_id);
  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - umts_present = %u gprs_present = %u\n", umts_present, gprs_present);

  /*
   * Initialize the temporary UE context and check if the IMEI was accepted or not.
   * We can proceed with the rest.
   */
  memset (&ue_ctx, 0, sizeof (emm_data_context_t));
  ue_ctx.is_dynamic = false;
  ue_ctx.ue_id= ue_id;

  /**
   * First check if the MME_APP UE context is valid.
   */
  if (INVALID_MME_UE_S1AP_ID == ue_id) {
    /** Received an invalid UE_ID. */
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  ATTACH - Received an invalid ue_id. Not continuing with the attach request. \n", );
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }
  /** Retrieve the MME_APP UE context. */
  ue_mm_ctx_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ue_id);
  DevAssert(ue_mm_context_t);

  /*
   * Requirement MME24.301R10_5.5.1.1_1
   * MME not configured to support attach for emergency bearer services
   * shall reject any request to attach with an attach type set to "EPS
   * emergency attach".
   */
  if (!(_emm_data.conf.eps_network_feature_support & EPS_NETWORK_FEATURE_SUPPORT_EMERGENCY_BEARER_SERVICES_IN_S1_MODE_SUPPORTED) &&
      (type == EMM_ATTACH_TYPE_EMERGENCY)) {
    REQUIREMENT_3GPP_24_301(R10_5_5_1__1);
    // TODO: update this if/when emergency attach is supported
    ue_ctx.emm_cause =
      imei ? EMM_CAUSE_IMEI_NOT_ACCEPTED : EMM_CAUSE_NOT_AUTHORIZED_IN_PLMN;
    /*
     * Do not accept the UE to attach for emergency services
     */
    rc = _emm_attach_reject (&ue_ctx);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }
  /*
   * Get the UE's EMM context if it exists by MME_UE_S1AP_ID, IMSI and GUTI and proceed with it.
   * Check if the ID is valid and proceed with it.
   */
  new_emm_ctx = emm_data_context_get(&_emm_data, ue_id);
  if (!new_emm_ctx && guti) { // no need for  && (is_native_guti)
    // todo: handle this case.
    new_emm_ctx = emm_data_context_get_by_guti(&_emm_data, guti);
    if (new_emm_ctx) {
      /** We found an EMM context. Don't clean it up. */
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - We found an EMM context from GUTI " GUTI_FMT " with old mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT". \n",
          GUTI_ARG(&new_emm_ctx->_guti), new_emm_ctx->ue_id);
      /** Continue to check for EMM context and their validity. */
    } else if ((!new_emm_ctx) && (imsi)) { /**< If we could not find one per IMSI. */
      new_emm_ctx = emm_data_context_get_by_imsi(&_emm_data, imsi64);
      if (new_emm_ctx) {
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - We found an EMM context from IMSI " IMSI_64_FMT " with old mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT". \n",
            new_emm_ctx->_imsi64, new_emm_ctx->ue_id);
      }
    }
  }
  /**
   * Now we may or may not have received an UE context. Continue to validate it or discard it.
   * If the new_emm_ctx is invalid we remove it.
   */
  if (new_emm_ctx) {
    mme_ue_s1ap_id_t old_mme_ue_id = new_emm_ctx->ue_id;
    /** Check the validity of the existing EMM context. */
    if(new_emm_ctx->ue_id != ue_id){
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The UE_ID EMM context from IMSI " IMSI_64_FMT " with old mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " will be updated to " MME_UE_S1AP_ID_FMT". \n",
            new_emm_ctx->_imsi64, new_emm_ctx->ue_id, ue_id);
      new_emm_ctx->ue_id = ue_id;
    }
    /** Get the latest state. The MME_APP context will be kept. */
    fsm_state = emm_fsm_get_state (new_emm_ctx);
    new_emm_ctx->attach_type = type;
    new_emm_ctx->additional_update_type = additional_update_type;

    /*
     * We don't need to check the identification procedure. If it is not part of an attach procedure (no specific attach procedure is running - assuming all common procedures
     * belong to the specific procedure), we let it run. It may independently trigger a timeout with consequences.
     *
     * If there is a specific attach procedure running, see below. All running common procedures will assumed to be part of the specific procedure and will be aborted together
     * with the specific procedure.
     */
    if (emm_ctx_is_common_procedure_running(new_emm_ctx, EMM_CTXT_COMMON_PROC_IDENT)) {
      /*
       * We just check if a collision with the same specific procedure is occurring for log reasons.
       * If a collision with a running specific procedure occurs below, all common procedures will be assumed to be part of the specific procedure and aborted accordingly, anyway.
       */
      if (emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_ATTACH) || emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_TAU)){
        REQUIREMENT_3GPP_24_301(R10_5_4_4_6_d__2); /**< Will be aborted eventually below. */
        REQUIREMENT_3GPP_24_301(R10_5_4_4_6_d__1);
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The EMM context from IMSI " IMSI_64_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " has an an identification procedure as part of a specific procedure. "
            "Common identification procedure will be aborted (as part of the possibly aborted specific procedure below, if the specific procedure is to be aborted. \n", new_emm_ctx->_imsi64, new_emm_ctx->ue_id);
      }else{
        REQUIREMENT_3GPP_24_301(R10_5_4_4_6_c); // continue
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The EMM context from IMSI " IMSI_64_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " has an identification procedure running but no specific procedure. "
            "Letting the procedure run currently. \n", new_emm_ctx->_imsi64, new_emm_ctx->ue_id);
        /**
         * Specification does not say, that we need to abort the identification procedure.
         * This may cause that a missing response detaches the UE after this attach complete successfully!
         * Unlike SMC, no abort.
         */
      }
    }

    /*
     * Check if there is an ongoing SMC procedure.
     * In any case, we just abort the current SMC procedure, which will keep the context, state, other commons and bearers, etc..
     * We continue with the specific procedure. If the SMC was part of the specific procedure, we don't care here. Thats the collision between the specific procedures.
     * No matter what the state is and if the received message differs or not, remove the EMM_CTX (incl. security), bearers, all other common procedures and abort any running
     * specific procedures.
     */
    if (emm_ctx_is_common_prcocedure_running(new_emm_ctx, EMM_CTXT_COMMON_PROC_SMC)) {
      REQUIREMENT_3GPP_24_301(R10_5_4_3_7_c);
      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMREG_PROC_ABORT;
      emm_sap.u.emm_reg.ue_id = new_emm_ctx->ue_id; /**< This should be enough to get the EMM context. */
      emm_sap.u.emm_reg.ctx = new_emm_ctx; /**< This should be enough to get the EMM context. */
      // TODOdata->notify_failure = true;
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_PROC_ABORT (SMC) ue id " MME_UE_S1AP_ID_FMT " ", new_emm_ctx->mme_ue_s1ap_id);
      rc = emm_sap_send (&emm_sap);
      /** Keep all the states, context, bearers and other common and specific procedures. */
      DevAssert(rc == RETURNok);
    }

    /*
     * Check if there is an Authentication Procedure running.
     * No collisions are mentioned, but just abort the current authentication procedure, if it was triggered by MME/OAM interface.
     * Consider last UE/EPS_Security context to be valid, keep the state and everything.
     * If it was part of a specific procedure, we will deal with it too, below, anyway.
     */
    if (emm_ctx_is_common_prcocedure_running(new_emm_ctx, EMM_CTXT_COMMON_PROC_AUTH)) {
      if (emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_ATTACH) || emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_TAU)){
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The EMM context from IMSI " IMSI_64_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " has an an authentication procedure as part of a specific procedure. "
            "Common identification procedure will be aborted (as part of the possibly aborted specific procedure below, if the specific procedure is to be aborted. \n", new_emm_ctx->_imsi64, new_emm_ctx->ue_id);
      } else{
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The EMM context from IMSI " IMSI_64_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " has an authentication procedure running but no specific procedure. "
            "Aborting the authentication procedure run currently. \n", new_emm_ctx->_imsi64, new_emm_ctx->ue_id);
        emm_sap_t                               emm_sap = {0};
        emm_sap.primitive = EMMREG_PROC_ABORT;
        emm_sap.u.emm_reg.ue_id = new_emm_ctx->ue_id; /**< This should be enough to get the EMM context. */
        emm_sap.u.emm_reg.ctx = new_emm_ctx; /**< This should be enough to get the EMM context. */
        // TODOdata->notify_failure = true;
        MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_PROC_ABORT (AUTH) ue id " MME_UE_S1AP_ID_FMT " ", new_emm_ctx->mme_ue_s1ap_id);
        rc = emm_sap_send (&emm_sap);
        /** Keep all the states, context, bearers and other common and specific procedures. */
      }
    }
    /*
     * UE may or may not be in COMMON-PROCEDURE-INITIATED state (common procedure active or implicit GUTI reallocation), DEREGISTERED or already REGISTERED.
     * Further check on collision with other specific procedures.
     * Checking if EMM_REGISTERED or already has been registered.
     * Already been registered only makes sense if  COMMON procedures are activated.
     */
    if ((new_emm_ctx) && ((EMM_REGISTERED == fsm_state) || new_emm_ctx->is_has_been_attached)) {
      REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_f);
      OAILOG_TRACE (LOG_NAS_EMM, "EMM-PROC  - the new ATTACH REQUEST is progressed and the current registered EMM context, "
          "together with the states, common procedures and bearers will be destroyed. \n");
      DevAssert(!emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_ATTACH));
      /*
       * Perform an implicit detach on the current context.
       * This will check if there are any common procedures are running, and destroy them, too.
       * We don't need to check if a TAU procedure is running, too. It will be destroyed in the implicit detach.
       * It will destroy any EMM context. EMM_CTX wil be nulled (not EMM_DEREGISTERED).
       */
      // Trigger clean up
      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;
      emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = new_emm_ctx->ue_id;
      /** Don't send the detach type, such that no NAS Detach Request is sent to the UE. */
      rc = emm_sap_send (&emm_sap);
      /** Additionally, after performing a NAS implicit detach, reset all the EMM context (the UE is in DEREGISTERED state, we reset it to invalid state. */
    }
    /*
     * Check for other running specific procedures for collisions.
     * We will define a specific procedure running/pending if a REQUEST is received and an ACCEPT/REJECT has to be sent yet.
     */
    else if ((new_emm_ctx) && emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_ATTACH_ACCEPT_SENT)) { // && (!ue_mm_context->is_attach_complete_received): implicit
      DevAssert(!emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_ATTACH));
      /** Check for a retransmission first. */
//      new_emm_ctx->num_attach_request++;
      if (_emm_attach_have_changed (new_emm_ctx, type, ksi, guti, imsi, imei, eea, eia, ucs2, uea, uia, gea, umts_present, gprs_present)) {
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Attach parameters have changed\n");
        REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_d__1);
        /*
         * Perform an Implicit Detach. It may not stop the specific procedures but the common ones (GUTI reallocation).
         */
        emm_sap_t                               emm_sap = {0};
        emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;     /**< Unmark all common and specific procedures. */
        emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = new_emm_ctx->ue_id;
        /** Don't send the detach type, such that no NAS Detach Request is sent to the UE. */
        rc = emm_sap_send (&emm_sap);
        // todo: check for rc
      }else{
        REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_d__2);
        // todo: SPECIFIC PROCEDURE --> ATTACH :: means that ATTACH_ACCEPT IS ALREADY SET? CSR//BEARER are already established?
        /*
         * - if the information elements do not differ, then the ATTACH ACCEPT message shall be resent and the timer
         * T3450 shall be restarted if an ATTACH COMPLETE message is expected. In that case, the retransmission
         * counter related to T3450 is not incremented.
         */
        _emm_attach_accept_retx(new_emm_ctx);
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
      }
    }
    /** Check for running specific ATTACH and TAU procedures. */
    else if ((new_emm_ctx) /* && (0 < new_emm_ctx->num_attach_request) */ &&
        (emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_ATTACH))){
      DevAssert(!emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_ATTACH_ACCEPT_SENT));
      DevAssert(!emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_ATTACH_REJECT_SENT));
      //      new_emm_ctx->num_attach_request++;
      if (_emm_attach_have_changed (new_emm_ctx, type, ksi, guti, imsi, imei, eea, eia, ucs2, uea, uia, gea, umts_present, gprs_present)) {
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Attach parameters have changed while specific attach procedure is running. \n");
        REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_e__1);
        /*
         * If one or more of the information elements in the ATTACH REQUEST message differs from the ones
         * received within the previous ATTACH REQUEST message, the previously initiated attach procedure shall
         * be aborted and the new attach procedure shall be executed;
         */
        emm_sap_t                               emm_sap = {0};
        emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;     /**< Unmark all common and specific procedures, remove bearers and emm contexts. */
        emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = new_emm_ctx->ue_id;
        /** Don't send the detach type, such that no NAS Detach Request is sent to the UE. */
        rc = emm_sap_send (&emm_sap);
        // todo: check for rc
      } else {
        REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_e__2);
        /*
         * if the information elements do not differ, then the network shall continue with the previous attach procedure
         * and shall ignore the second ATTACH REQUEST message.
         */
        //is_attach_procedure_progressed = false; // for clarity
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Received duplicated Attach Request\n");
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
      }
    }
    /** Check for collisions with tracking area update. */
    else if ((new_emm_ctx) && emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_TAU_ACCEPT_SENT)) { // && (!ue_mm_context->is_attach_complete_received): implicit
      // && (!ue_mm_context->is_attach_complete_received): implicit
      DevAssert(!emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_TAU));
      /** Check for a retransmission first. */
      //      new_emm_ctx->num_attach_request++;
      /** Implicitly detach the UE context, which will remove all common and specific procedures. */
      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;     /**< Unmark all common and specific procedures, remove bearers and emm contexts. */
      emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = new_emm_ctx->ue_id;
      /** Don't send the detach type, such that no NAS Detach Request is sent to the UE. */
      rc = emm_sap_send (&emm_sap);
      // todo: check RC and continue with attach
    }
    else if ((new_emm_ctx) /* && (0 < new_emm_ctx->num_attach_request) */ &&
        (emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_TAU))){
      DevAssert(!emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_TAU_ACCEPT_SENT));
      DevAssert(!emm_ctx_is_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_TAU_REJECT_SENT));
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Received Attach Request while TAU procedure is ongoing. Implicitly detaching the UE context and continuing with the "
          "attach request for the UE_ID " MME_UE_S1AP_ID_FMT ". \n", new_emm_ctx->ue_id);
      /** Implicitly detach the UE context, which will remove all common and specific procedures. */
      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;     /**< Unmark all common and specific procedures, remove bearers and emm contexts. */
      emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = new_emm_ctx->ue_id;
      /** Don't send the detach type, such that no NAS Detach Request is sent to the UE. */
      rc = emm_sap_send (&emm_sap);
      // todo: check RC and continue with attach
      //
      /** No collision with a specific procedure and UE is in EMM_DEREGISTERED state (only state where we can continue with the UE context. */
    } else if(new_emm_ctx && (EMM_DEREGISTERED == fsm_state)){
      // todo: is this an undefined state?
      /*
       * Silently discard continue with the DEREGISTERED EMM context.
       * Must be sure, that no COMPLETE message or so is received.
       */
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Received Attach Request for a DEREGISTERED UE_ID " MME_UE_S1AP_ID_FMT ". "
          "Will continue with the existing EMM_CTX's and let current common procedures run. Silently discarding all specific procedures (XXX_ACCEPT sent, etc.. ). \n", new_emm_ctx->ue_id);
      // todo: to be sure, just silently discard all specific procedures, is this necessary or should we assert?
      emm_context_silently_reset_procedures(new_emm_ctx); /**< Discard all pending COMPLETEs, here (specific: XX_accept sent already). The complete message will then be discarded when specific XX_ACCEPT flag is off! */
      /*
       * Assuming nothing to reset when continuing with DEREGISTERED EMM context. Continue using its EPS security context.
       * Will ask for new subscription data in the HSS with ULR (also to register the MME again).
       */
    } else { //else  ((ue_mm_context) && ((EMM_DEREGISTERED < fsm_state ) && (EMM_REGISTERED != fsm_state)))
      /*
       * This should also consider a Service Request Procedure.
       */
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Received Attach Request for a UE_ID " MME_UE_S1AP_ID_FMT " in an unhandled state. Implicitly detaching the UE context and continuing with the new attach request. \n", new_emm_ctx->ue_id);
      /** Implicitly detach the UE context, which will remove all common and specific procedures. */
      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;     /**< Unmark all common and specific procedures, remove bearers and emm contexts. */
      emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = new_emm_ctx->ue_id;
      /** Don't send the detach type, such that no NAS Detach Request is sent to the UE. */
      rc = emm_sap_send (&emm_sap);
      // todo: check RC and continue with attach
    }
  }
  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Continuing for Attach Request for UE_ID " MME_UE_S1AP_ID_FMT " after validation of the attach request. \n",
      new_emm_ctx->ue_id);
  if (new_emm_ctx) {
    DevAssert(new_emm_ctx->_emm_fsm_state == EMM_DEREGISTERED);
    OAILOG_NOTICE (LOG_NAS_EMM, "EMM-PROC  - An EMM context for ue_id = " MME_UE_S1AP_ID_FMT " with IMSI " IMSI_64_FMT " already "
        "exists in EMM_DEREGISTERED mode. Continuing with it. \n", ue_id, new_emm_ctx->_imsi64);
    /** Check if any parameters have changed, if so mark the context as invalid and continue new. */
    if (_emm_attach_have_changed (new_emm_ctx, type, ksi, guti, imsi, imei, eea, eia, ucs2, uea, uia, gea, umts_present, gprs_present)) {
      OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - The parameters for the EMM context for ue_id = " MME_UE_S1AP_ID_FMT " with IMSI " IMSI_64_FMT " "
          "have changed to the new attach request. Invalidating the current EMM context (impl. detach) and continuing with a fresh EMM context. \n",
          ue_id, new_emm_ctx->_imsi64);
      // todo: 5G has something like REGISTRATION UPDATE!
      /** Implicitly detach the UE context, which will remove all common and specific procedures. */
      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;     /**< Unmark all common and specific procedures, remove bearers and emm contexts. */
      emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = new_emm_ctx->ue_id;
      /** Don't send the detach type, such that no NAS Detach Request is sent to the UE. */
      rc = emm_sap_send (&emm_sap);
      // todo: check RC and continue with attach
    }else{
      OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - The parameters for the EMM context for ue_id = " MME_UE_S1AP_ID_FMT " with IMSI " IMSI_64_FMT " "
          "have NOT changed to the new attach request. Continuing with the existing EMM context. \n", ue_id, new_emm_ctx->_imsi64);
    }
  }
  if(!new_emm_ctx) {
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - No valid EMM context was found for UE_ID " MME_UE_S1AP_ID_FMT ". \n", new_emm_ctx->ue_id);
    /*
     * Create UE's EMM context
     */
    new_emm_ctx = (emm_data_context_t *) calloc (1, sizeof (emm_data_context_t));
    if (!new_emm_ctx) {
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to create EMM context for ueId " MME_UE_S1AP_ID_FMT ". \n", ue_ctx.ue_id);
      ue_ctx.emm_cause = EMM_CAUSE_ILLEGAL_UE; /**< Send a ATTACH_REJECT with the temporary context. */
      /*
       * Do not accept the UE to attach to the network
       */
      rc = _emm_attach_reject (&ue_ctx);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc); /**< Return with an error. */
    }
    /** Initialize the EMM context only in this case, else we have a DEREGISTERED but valid EMM context. */
    /** Check if the EMM context has been initialized, if so set the values. */
    // todo: num_attach_request?!
    new_emm_ctx->is_dynamic = true;
    bdestroy(new_emm_ctx->esm_msg);
    OAILOG_NOTICE (LOG_NAS_EMM, "EMM-PROC  - Create EMM context ue_id = " MME_UE_S1AP_ID_FMT "\n", ue_id);
    new_emm_ctx->attach_type = type;
    new_emm_ctx->additional_update_type = additional_update_type;
    new_emm_ctx->emm_cause       = EMM_CAUSE_SUCCESS;
    new_emm_ctx->_emm_fsm_state = EMM_INVALID;
    new_emm_ctx->T3450.id        = NAS_TIMER_INACTIVE_ID;
    new_emm_ctx->T3450.sec       = mme_config.nas_config.t3450_sec;
    new_emm_ctx->T3460.id        = NAS_TIMER_INACTIVE_ID;
    new_emm_ctx->T3460.sec       = mme_config.nas_config.t3460_sec;
    new_emm_ctx->T3470.id        = NAS_TIMER_INACTIVE_ID;
    new_emm_ctx->T3470.sec       = mme_config.nas_config.t3470_sec;
    emm_fsm_set_status (ue_id, &new_emm_ctx, EMM_DEREGISTERED);

    emm_ctx_clear_guti(&new_emm_ctx);
    emm_ctx_clear_old_guti(&new_emm_ctx);
    emm_ctx_clear_imsi(&new_emm_ctx);
    emm_ctx_clear_imei(&new_emm_ctx);
    emm_ctx_clear_imeisv(&new_emm_ctx);
    emm_ctx_clear_lvr_tai(&new_emm_ctx);
    emm_ctx_clear_security(&new_emm_ctx);
    emm_ctx_clear_non_current_security(&new_emm_ctx);
    emm_ctx_clear_auth_vectors(&new_emm_ctx);
    emm_ctx_clear_ms_nw_cap(&new_emm_ctx);
    emm_ctx_clear_ue_nw_cap_ie(&new_emm_ctx);
    emm_ctx_clear_current_drx_parameter(&new_emm_ctx);
    emm_ctx_clear_pending_current_drx_parameter(&new_emm_ctx);
    emm_ctx_clear_eps_bearer_context_status(&new_emm_ctx);

    new_emm_ctx->ue_id = ue_id;
    if (RETURNok != emm_data_context_add (&_emm_data, new_emm_ctx)) {
      OAILOG_CRITICAL(LOG_NAS_EMM, "EMM-PROC  - Attach EMM Context could not be inserted in hastables\n");
      rc = _emm_attach_reject (new_emm_ctx);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }

    if (last_visited_registered_tai) {
      emm_ctx_set_valid_lvr_tai(new_emm_ctx, last_visited_registered_tai);
    } else {
      emm_ctx_clear_lvr_tai(new_emm_ctx);
    }
  }
  /**
   * Mark the specific procedure for attach.
   * todo: creating a context for the specific procedure?! Binding common procedures to it?
   */
  emm_ctx_mark_specific_procedure(emm_ctx, EMM_CTXT_SPEC_PROC_ATTACH); /**< Just marking and not setting timer (will be set later with TAU_ACCEPT). */
  DevAssert(!new_emm_ctx->specific_proc_data); /**< Assert that it does not exist. */
  // todo: not initializing the specific procedure data, thrown out?
  //  new_emm_ctx->specific_proc = (emm_specific_procedure_data_t *) calloc (1, sizeof (*new_emm_ctx->specific_proc));
  //  if (new_emm_ctx->specific_proc ) {
  //    /*
  //     * Setup ongoing EMM procedure callback functions
  //     * todo:
  //     */
  //    rc = emm_proc_specific_initialize (new_emm_ctx, EMM_SPECIFIC_PROC_TYPE_ATTACH, new_emm_ctx->specific_proc, NULL, NULL, _emm_attach_abort);
  //    emm_ctx_mark_specific_procedure_running (new_emm_ctx, EMM_CTXT_SPEC_PROC_ATTACH);
  //    new_emm_ctx->specific_proc->arg.u.attach_data.ue_id = ue_id;
  //
  //    if (rc != RETURNok) {
  //      OAILOG_WARNING (LOG_NAS_EMM, "Failed to initialize EMM callback functions");
  //      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  //    }
  //  }

  /*
   * Update the EMM context with the current attach procedure parameters
   */
  rc = _emm_attach_update (new_emm_ctx, ue_id, type, ksi, is_native_guti, guti, imsi, imei, originating_tai, eea, eia, ucs2, uea, uia, gea, umts_present, gprs_present, esm_msg_pP);
  if (rc != RETURNok) {
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to update EMM context\n");
    /*
     * Do not accept the UE to attach to the network
     */
    new_emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    rc = _emm_attach_reject (new_emm_ctx);
  } else {
    /*
     * Performs the sequence: UE identification, authentication, security mode with the updated parameters.
     */
    rc = _emm_attach_identify (new_emm_ctx);
  }
}

/****************************************************************************
 **                                                                        **
 ** Name:        emm_proc_attach_reject()                                  **
 **                                                                        **
 ** Description: Performs the protocol error abnormal case                 **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.7, case b                 **
 **              If the ATTACH REQUEST message is received with a protocol **
 **              error, the network shall return an ATTACH REJECT message. **
 **                                                                        **
 ** Inputs:  ue_id:              UE lower layer identifier                  **
 **                  emm_cause: EMM cause code to be reported              **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    RETURNok, RETURNerror                      **
 **                  Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_attach_reject (
  mme_ue_s1ap_id_t ue_id,
  emm_cause_t emm_cause)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  /*
   * Create temporary UE context
   */
  ue_mm_context_t                      ue_ctx;

  memset (&ue_ctx, 0, sizeof (ue_mm_context_t));
  ue_ctx.emm_context.is_dynamic = false;
  ue_ctx.mme_ue_s1ap_id = ue_id;
  /*
   * Update the EMM cause code
   */

  if (ue_id > 0) {
    ue_ctx.emm_context.emm_cause = emm_cause;
  } else {
    ue_ctx.emm_context.emm_cause = EMM_CAUSE_ILLEGAL_UE;
  }

  /*
   * Do not accept attach request with protocol error
   */
  rc = _emm_attach_reject (&ue_ctx.emm_context);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_attach_complete()                                **
 **                                                                        **
 ** Description: Terminates the attach procedure upon receiving Attach     **
 **      Complete message from the UE.                             **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.4                         **
 **      Upon receiving an ATTACH COMPLETE message, the MME shall  **
 **      stop timer T3450, enter state EMM-REGISTERED and consider **
 **      the GUTI sent in the ATTACH ACCEPT message as valid.      **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      esm_msg_pP:   Activate default EPS bearer context accept **
 **             ESM message                                **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_data, T3450                           **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_attach_complete (
  mme_ue_s1ap_id_t ue_id,
  const_bstring esm_msg_pP)
{
  ue_mm_context_t                          *ue_mm_context = NULL;
  int                                     rc = RETURNerror;
  emm_sap_t                               emm_sap = {0};
  esm_sap_t                               esm_sap = {0};

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - EPS attach complete (ue_id=" MME_UE_S1AP_ID_FMT ")\n", ue_id);

  /*
   * Get the UE context
   */
  ue_mm_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ue_id);

  if (ue_mm_context) {
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__20);
    /*
     * Release retransmission timer parameters
     */

    /*
     * Upon receiving an ATTACH COMPLETE message, the MME shall stop timer T3450
     */
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3450 (%lx)\n", ue_mm_context->emm_context.T3450.id);
    ue_mm_context->emm_context.T3450.id = nas_timer_stop (ue_mm_context->emm_context.T3450.id);
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    emm_proc_specific_cleanup(&ue_mm_context->emm_context.specific_proc);

    /*
     * Upon receiving an ATTACH COMPLETE message, the MME shall enter state EMM-REGISTERED
     * and consider the GUTI sent in the ATTACH ACCEPT message as valid.
     */
    emm_ctx_set_attribute_valid(&ue_mm_context->emm_context, EMM_CTXT_MEMBER_GUTI);
    //OAI_GCC_DIAG_OFF(int-to-pointer-cast);
    mme_ue_context_update_coll_keys ( &mme_app_desc.mme_ue_contexts, ue_mm_context, ue_mm_context->enb_s1ap_id_key, ue_mm_context->mme_ue_s1ap_id,
        ue_mm_context->emm_context._imsi64, ue_mm_context->mme_teid_s11, &ue_mm_context->emm_context._guti);
    //OAI_GCC_DIAG_ON(int-to-pointer-cast);
    emm_ctx_clear_old_guti(&ue_mm_context->emm_context);

    /*
     * Forward the Activate Default EPS Bearer Context Accept message
     * to the EPS session management sublayer
     */
    esm_sap.primitive = ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_CNF;
    esm_sap.is_standalone = false;
    esm_sap.ue_id = ue_id;
    esm_sap.recv = esm_msg_pP;
    esm_sap.ctx = &ue_mm_context->emm_context;
    rc = esm_sap_send (&esm_sap);
  } else {
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - No EMM context exists\n");
  }


  if ((rc != RETURNerror) && (esm_sap.err == ESM_SAP_SUCCESS)) {
    /*
     * Set the network attachment indicator
     */
    ue_mm_context->emm_context.is_attached = true;
    ue_mm_context->emm_context.is_has_been_attached = true;
    /*
     * Notify EMM that attach procedure has successfully completed
     */
    emm_sap.primitive = EMMREG_ATTACH_CNF;
    emm_sap.u.emm_reg.ue_id = ue_id;
    emm_sap.u.emm_reg.ctx = &ue_mm_context->emm_context;
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_ATTACH_CNF ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    rc = emm_sap_send (&emm_sap);
  } else if (esm_sap.err != ESM_SAP_DISCARDED) {
    /*
     * Notify EMM that attach procedure failed
     */
    emm_sap.primitive = EMMREG_ATTACH_REJ;
    emm_sap.u.emm_reg.ue_id = ue_id;
    emm_sap.u.emm_reg.ctx = &ue_mm_context->emm_context;
    rc = emm_sap_send (&emm_sap);
  } else {
    /*
     * ESM procedure failed and, received message has been discarded or
     * Status message has been returned; ignore ESM procedure failure
     */
    rc = RETURNok;
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/



/*
 * --------------------------------------------------------------------------
 * Timer handlers
 * --------------------------------------------------------------------------
 */

/*
 *
 * Name:    _emm_attach_t3450_handler()
 *
 * Description: T3450 timeout handler
 *
 *              3GPP TS 24.301, section 5.5.1.2.7, case c
 *      On the first expiry of the timer T3450, the network shall
 *      retransmit the ATTACH ACCEPT message and shall reset and
 *      restart timer T3450. This retransmission is repeated four
 *      times, i.e. on the fifth expiry of timer T3450, the at-
 *      tach procedure shall be aborted and the MME enters state
 *      EMM-DEREGISTERED.
 *
 * Inputs:  args:      handler parameters
 *      Others:    None
 *
 * Outputs:     None
 *      Return:    None
 *      Others:    None
 *
 */
static void *_emm_attach_t3450_handler (void *args)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_data_context_t                          *emm_context = (emm_data_context_t *) (args);

  if (emm_context) {
    emm_context->T3450.id = NAS_TIMER_INACTIVE_ID;
  }
  if (emm_ctx_is_specific_procedure_running (emm_context, EMM_CTXT_SPEC_PROC_ATTACH)){
    attach_data_t                          *data =  &emm_context->specific_proc->arg.u.attach_data;

    /*
     * Increment the retransmission counter
     */
    data->retransmission_count += 1;
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - T3450 timer expired, retransmission " "counter = %d\n", data->retransmission_count);


    if (data->retransmission_count < ATTACH_COUNTER_MAX) {
      REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_c__1);
      /*
       * On the first expiry of the timer, the network shall retransmit the ATTACH ACCEPT message and shall reset and
       * restart timer T3450.
       */
      _emm_attach_accept (emm_context);
    } else {
      REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_c__2);
      /*
       * Abort the attach procedure
       */
      _emm_attach_abort (emm_context);
    }
    // TODO REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_c__3) not coded
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, NULL);
}

/*
 * --------------------------------------------------------------------------
 * Abnormal cases in the MME
 * --------------------------------------------------------------------------
 */

/*
 *
 * Name:    _emm_attach_release()
 *
 * Description: Releases the UE context data.
 *
 * Inputs:  args:      Data to be released
 *      Others:    None
 *
 * Outputs:     None
 *      Return:    None
 *      Others:    None
 *
 */
static int _emm_attach_release (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  if (emm_context) {
    mme_ue_s1ap_id_t      ue_id = PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context)->mme_ue_s1ap_id;
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Release UE context data (ue_id=" MME_UE_S1AP_ID_FMT ")\n", ue_id);

    emm_ctx_clear_old_guti(emm_context);
    emm_ctx_clear_guti(emm_context);
    emm_ctx_clear_imsi(emm_context);
    emm_ctx_clear_imei(emm_context);
    emm_ctx_clear_auth_vectors(emm_context);
    emm_ctx_clear_security(emm_context);
    emm_ctx_clear_non_current_security(emm_context);

    bdestroy_wrapper (&emm_context->esm_msg);


    /*
     * Stop timer T3450
     */
    if (emm_context->T3450.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3450 (%lx)\n", emm_context->T3450.id);
      emm_context->T3450.id = nas_timer_stop (emm_context->T3450.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    }

    /*
     * Stop timer T3460
     */
    if (emm_context->T3460.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3460 (%lx)\n", emm_context->T3460.id);
      emm_context->T3460.id = nas_timer_stop (emm_context->T3460.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3460 stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    }

    /*
     * Stop timer T3470
     */
    if (emm_context->T3470.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3470 (%lx)\n", emm_context->T3470.id);
      emm_context->T3470.id = nas_timer_stop (emm_context->T3470.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3470 stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    }

    /*
     * Release the EMM context
     */
#warning "TODO think about emm_context_remove"
//    emm_context_remove (&_emm_data, ue_mm_context);
    /*
     * Notify EMM that the attach procedure is aborted
     */
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_PROC_ABORT ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    emm_sap_t                               emm_sap = {0};


    emm_sap.primitive = EMMREG_PROC_ABORT;
    emm_sap.u.emm_reg.ue_id = ue_id;
    emm_sap.u.emm_reg.ctx = emm_context;
    rc = emm_sap_send (&emm_sap);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/*
 *
 * Name:    _emm_attach_reject()
 *
 * Description: Performs the attach procedure not accepted by the network.
 *
 *              3GPP TS 24.301, section 5.5.1.2.5
 *      If the attach request cannot be accepted by the network,
 *      the MME shall send an ATTACH REJECT message to the UE in-
 *      including an appropriate EMM cause value.
 *
 * Inputs:  args:      UE context data
 *      Others:    None
 *
 * Outputs:     None
 *      Return:    RETURNok, RETURNerror
 *      Others:    None
 *
 */
int _emm_attach_reject (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  if (emm_context) {
    emm_sap_t                               emm_sap = {0};
    mme_ue_s1ap_id_t      ue_id = PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context)->mme_ue_s1ap_id;

    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - EMM attach procedure not accepted " "by the network (ue_id=" MME_UE_S1AP_ID_FMT ", cause=%d)\n",
            ue_id, emm_context->emm_cause);
    /*
     * Notify EMM-AS SAP that Attach Reject message has to be sent
     * onto the network
     */
    emm_sap.primitive = EMMAS_ESTABLISH_REJ;
    emm_sap.u.emm_as.u.establish.ue_id = ue_id;
    emm_sap.u.emm_as.u.establish.eps_id.guti = NULL;

    if (emm_context->emm_cause == EMM_CAUSE_SUCCESS) {
      emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    }

    emm_sap.u.emm_as.u.establish.emm_cause = emm_context->emm_cause;
    emm_sap.u.emm_as.u.establish.nas_info = EMM_AS_NAS_INFO_ATTACH;

    if (emm_context->emm_cause != EMM_CAUSE_ESM_FAILURE) {
      emm_sap.u.emm_as.u.establish.nas_msg = NULL;
    } else if (emm_context->esm_msg) {
      emm_sap.u.emm_as.u.establish.nas_msg = emm_context->esm_msg;
    } else {
      OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - ESM message is missing\n");
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }

    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data (&emm_sap.u.emm_as.u.establish.sctx, &emm_context->_security, false, true);
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMAS_ESTABLISH_REJ ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    rc = emm_sap_send (&emm_sap);

    /*
     * Release the UE context, even if the network failed to send the
     * ATTACH REJECT message
     */
    if (emm_context->is_dynamic) {
      rc = _emm_attach_release (emm_context);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/*
 *
 * Name:    _emm_attach_abort()
 *
 * Description: Aborts the attach procedure
 *
 * Inputs:  args:      Attach procedure data to be released
 *      Others:    None
 *
 * Outputs:     None
 *      Return:    RETURNok, RETURNerror
 *      Others:    T3450
 *
 */
static int _emm_attach_abort (emm_data_context_t *emm_context)
{
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_EMM);

  if (emm_ctx_is_specific_procedure_running (emm_context, EMM_CTXT_SPEC_PROC_ATTACH)) {
    attach_data_t                          *data = &emm_context->specific_proc->arg.u.attach_data;
    mme_ue_s1ap_id_t                        ue_id = PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context)->mme_ue_s1ap_id;
    esm_sap_t                               esm_sap = {0};

    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Abort the attach procedure (ue_id=" MME_UE_S1AP_ID_FMT ")\n", ue_id);

    AssertFatal(EMM_SPECIFIC_PROC_TYPE_ATTACH == emm_context->specific_proc->type, "Mismatch in specific proc arg type %d UE id" MME_UE_S1AP_ID_FMT "\n",
        emm_context->specific_proc->type, ue_id);
    AssertFatal(ue_id == data->ue_id, "Mismatch in UE ids: ctx UE id" MME_UE_S1AP_ID_FMT " data UE id" MME_UE_S1AP_ID_FMT "\n", ue_id, data->ue_id);
//    /*
//     * Stop timer T3450
//     */
//    if (emm_context->T3450.id != NAS_TIMER_INACTIVE_ID) {
//      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3450 (%lx)\n", emm_context->T3450.id);
//      emm_context->T3450.id = nas_timer_stop (emm_context->T3450.id);
//      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
//    }

//    /*
//     * Release retransmission timer parameters
//     */
//    bdestroy_wrapper (&data->esm_msg);
    /*
     * Notify ESM that the network locally refused PDN connectivity
     * to the UE
     */
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_PDN_CONNECTIVITY_REJ ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    esm_sap.primitive = ESM_PDN_CONNECTIVITY_REJ;
    esm_sap.ue_id = ue_id;
    esm_sap.ctx = emm_context;
    esm_sap.recv = NULL;
    rc = esm_sap_send (&esm_sap);

    if (rc != RETURNerror) {
      /*
       * Notify EMM that EPS attach procedure failed
       */
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_ESM_MME, NULL, 0, "0 EMMREG_ATTACH_REJ ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
      emm_sap_t                               emm_sap = {0};

      emm_sap.primitive = EMMREG_ATTACH_REJ;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx = emm_context;
      rc = emm_sap_send (&emm_sap);

      if (rc != RETURNerror) {
        /*
         * Release the UE context
         */
        rc = _emm_attach_release (emm_context);
      }
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/*
 * --------------------------------------------------------------------------
 * Functions that may initiate EMM common procedures
 * --------------------------------------------------------------------------
 */

/*
 * Name:    _emm_attach_identify()
 *
 * Description: Performs UE's identification. May initiates identification, authentication and security mode control EMM common procedures.
 *
 * Inputs:  args:      Identification argument parameters
 *      Others:    None
 *
 * Outputs:     None
 *      Return:    RETURNok, RETURNerror
 *      Others:    _emm_data
 *
 */
static int _emm_attach_identify (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  REQUIREMENT_3GPP_24_301(R10_5_5_1_2_3__1);
  mme_ue_s1ap_id_t                        ue_id = emm_context->ue_id;
  OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Identify incoming UE using %s\n",
      ue_id,
      IS_EMM_CTXT_VALID_IMSI(emm_context)   ? "IMSI" :
      IS_EMM_CTXT_PRESENT_GUTI(emm_context) ? "GUTI" :
      IS_EMM_CTXT_VALID_IMEI(emm_context)   ? "IMEI" : "none");

  /*
   * UE's identification
   * -------------------
   */
  if (IS_EMM_CTXT_PRESENT_IMSI(emm_context)) {
    // The UE identifies itself using an IMSI
    if (!IS_EMM_CTXT_PRESENT_AUTH_VECTORS(emm_context)) {
      // Ask upper layer to fetch new security context
      plmn_t visited_plmn = {0};
      visited_plmn.mcc_digit1 = emm_context->originating_tai.plmn.mcc_digit1;
      visited_plmn.mcc_digit2 = emm_context->originating_tai.plmn.mcc_digit2;
      visited_plmn.mcc_digit3 = emm_context->originating_tai.plmn.mcc_digit3;
      visited_plmn.mnc_digit1 = emm_context->originating_tai.plmn.mnc_digit1;
      visited_plmn.mnc_digit2 = emm_context->originating_tai.plmn.mnc_digit2;
      visited_plmn.mnc_digit3 = emm_context->originating_tai.plmn.mnc_digit3;

      nas_itti_auth_info_req (ue_id, &emm_context->_imsi, true, &visited_plmn, MAX_EPS_AUTH_VECTORS, NULL);
      rc = RETURNok;
    } else {
      ksi_t                                   eksi = 0;
      int                                     vindex = 0;

      if (emm_context->_security.eksi != KSI_NO_KEY_AVAILABLE) {
        REQUIREMENT_3GPP_24_301(R10_5_4_2_4__2);
        eksi = (emm_context->_security.eksi + 1) % (EKSI_MAX_VALUE + 1);
      }
      for (vindex = 0; vindex < MAX_EPS_AUTH_VECTORS; vindex++) {
        if (IS_EMM_CTXT_PRESENT_AUTH_VECTOR(emm_context, vindex)) {
          break;
        }
      }
      // eksi should always be 0
      /*if (!IS_EMM_CTXT_PRESENT_AUTH_VECTORS(ue_mm_context)) {
        // Ask upper layer to fetch new security context
        nas_itti_auth_info_req (ue_mm_context->mme_ue_s1ap_id, ue_mm_context->_imsi64, true, &ue_mm_context->originating_tai.plmn, MAX_EPS_AUTH_VECTORS, NULL);
        rc = RETURNok;
      } else */{
        emm_ctx_set_security_vector_index(emm_context, vindex);
        rc = emm_proc_authentication (emm_context, ue_id, eksi,
          emm_context->_vector[vindex].rand, emm_context->_vector[vindex].autn, emm_attach_security, _emm_attach_reject, NULL);
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
      }
    }
  } else if (IS_EMM_CTXT_PRESENT_GUTI(emm_context)) {
    // The UE identifies itself using a GUTI
    //LG Force identification here
    emm_ctx_clear_attribute_valid(emm_context, EMM_CTXT_MEMBER_AUTH_VECTORS);
    OAILOG_WARNING (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Failed to identify the UE using provided GUTI (tmsi=%u)\n",
         ue_id, emm_context->_guti.m_tmsi);
    /*
     * 3GPP TS 24.401, Figure 5.3.2.1-1, point 4
     * The UE was attempting to attach to the network using a GUTI
     * that is not known by the network; the MME shall initiate an
     * identification procedure to retrieve the IMSI from the UE.
     */
    rc = emm_proc_identification (emm_context, EMM_IDENT_TYPE_IMSI, _emm_attach_identify, _emm_attach_release, _emm_attach_release);

    if (rc != RETURNok) {
      // Failed to initiate the identification procedure
      OAILOG_WARNING (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT "EMM-PROC  - Failed to initiate identification procedure\n", ue_id);
      emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      // Do not accept the UE to attach to the network
      rc = _emm_attach_reject (emm_context);
    }

    // Relevant callback will be executed when identification
    // procedure completes
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);

  } else if (IS_EMM_CTXT_PRESENT_OLD_GUTI(emm_context)) {

    OAILOG_WARNING (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Force to identify the UE using provided old GUTI "GUTI_FMT"\n",
        ue_id, GUTI_ARG(&emm_context->_old_guti));
    /*
     * 3GPP TS 24.401, Figure 5.3.2.1-1, point 4
     * The UE was attempting to attach to the network using a GUTI
     * that is not known by the network; the MME shall initiate an
     * identification procedure to retrieve the IMSI from the UE.
     */
    rc = emm_proc_identification (emm_context, EMM_IDENT_TYPE_IMSI, _emm_attach_identify, _emm_attach_release, _emm_attach_release);

    if (rc != RETURNok) {
      // Failed to initiate the identification procedure
      OAILOG_WARNING (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT "EMM-PROC  - Failed to initiate identification procedure\n", ue_id);
      emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      // Do not accept the UE to attach to the network
      rc = _emm_attach_reject (emm_context);
    }

    // Relevant callback will be executed when identification
    // procedure completes
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  } else if ((IS_EMM_CTXT_VALID_IMEI(emm_context)) && (emm_context->is_emergency)) {
    /*
     * The UE is attempting to attach to the network for emergency
     * services using an IMEI
     */
     AssertFatal(0 != 0, "TODO emergency services...");
    if (rc != RETURNok) {
      emm_ctx_clear_auth_vectors(emm_context);
      OAILOG_WARNING (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - " "Failed to identify the UE using provided IMEI\n", ue_id);
      emm_context->emm_cause = EMM_CAUSE_IMEI_NOT_ACCEPTED;
    }
  } else {
    OAILOG_WARNING (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - UE's identity is not available\n", ue_id);
    emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
  }

  /*
   * UE's authentication
   * -------------------
   */
  if (rc != RETURNerror) {
    if (IS_EMM_CTXT_VALID_SECURITY(emm_context)) {
      /*
       * A security context exists for the UE in the network;
       * proceed with the attach procedure.
       */
      rc = _emm_attach (emm_context);
    } else if ((emm_context->is_emergency) && (_emm_data.conf.features & MME_API_UNAUTHENTICATED_IMSI)) {
      /*
       * 3GPP TS 24.301, section 5.5.1.2.3
       * 3GPP TS 24.401, Figure 5.3.2.1-1, point 5a
       * MME configured to support Emergency Attach for unauthenticated
       * IMSIs may choose to skip the authentication procedure even if
       * no EPS security context is available and proceed directly to the
       * execution of the security mode control procedure.
       */
      rc = _emm_attach_security (emm_context);
    }
  }

  if (rc != RETURNok) {
    // Do not accept the UE to attach to the network
    rc = _emm_attach_reject (emm_context);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:        _emm_attach_security()                                    **
 **                                                                        **
 ** Description: Initiates security mode control EMM common procedure.     **
 **                                                                        **
 ** Inputs:          args:      security argument parameters               **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    RETURNok, RETURNerror                      **
 **                  Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
int emm_attach_security (struct emm_context_s *emm_context)
{
  return _emm_attach_security (emm_context);
}

//------------------------------------------------------------------------------
static int _emm_attach_security (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  mme_ue_s1ap_id_t                        ue_id = PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context)->mme_ue_s1ap_id;

  REQUIREMENT_3GPP_24_301(R10_5_5_1_2_3__1);
  OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Setup NAS security\n", ue_id);

  /*
   * Create new NAS security context
   */
  emm_ctx_clear_security(emm_context);

  /*
   * Initialize the security mode control procedure
   */
  rc = emm_proc_security_mode_control (ue_id, emm_context->ue_ksi,
                                       _emm_attach, _emm_attach_release, _emm_attach_release);

  if (rc != RETURNok) {
    /*
     * Failed to initiate the security mode control procedure
     */
    OAILOG_WARNING (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT "EMM-PROC  - Failed to initiate security mode control procedure\n", ue_id);
    emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    /*
     * Do not accept the UE to attach to the network
     */
    rc = _emm_attach_reject (emm_context);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach()                                             **
 **                                                                        **
 ** Description: Performs the attach signalling procedure while a context  **
 **      exists for the incoming UE in the network.                **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.4                         **
 **      Upon receiving the ATTACH REQUEST message, the MME shall  **
 **      send an ATTACH ACCEPT message to the UE and start timer   **
 **      T3450.                                                    **
 **                                                                        **
 ** Inputs:  args:      attach argument parameters                 **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
static int _emm_attach (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  esm_sap_t                               esm_sap = {0};
  int                                     rc = RETURNerror;
  mme_ue_s1ap_id_t                        ue_id = PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context)->mme_ue_s1ap_id;

  OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Attach UE \n", ue_id);
  /*
   * 3GPP TS 24.401, Figure 5.3.2.1-1, point 5a
   * At this point, all NAS messages shall be protected by the NAS security
   * functions (integrity and ciphering) indicated by the MME unless the UE
   * is emergency attached and not successfully authenticated.
   */
  /*
   * Notify ESM that PDN connectivity is requested
   */
  MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_ESM_MME, NULL, 0, "0 ESM_PDN_CONNECTIVITY_REQ ue id " MME_UE_S1AP_ID_FMT " ", ue_id);

  esm_sap.primitive = ESM_PDN_CONNECTIVITY_REQ;
  esm_sap.is_standalone = false;
  esm_sap.ue_id = ue_id;
  esm_sap.ctx = emm_context;
  esm_sap.recv = emm_context->esm_msg;
  rc = esm_sap_send (&esm_sap);

  if ((rc != RETURNerror) && (esm_sap.err == ESM_SAP_SUCCESS)) {
    /*
     * The attach request is accepted by the network
     */
    /*
     * Delete the stored UE radio capability information, if any
     */
    /*
     * Store the UE network capability
     */
    /*
     * Assign the TAI list the UE is registered to
     */
    /*
     * Allocate parameters of the retransmission timer callback
     */
    rc = RETURNok;
  } else if (esm_sap.err != ESM_SAP_DISCARDED) {
    /*
     * The attach procedure failed due to an ESM procedure failure
     */
    emm_context->emm_cause = EMM_CAUSE_ESM_FAILURE;

    /*
     * Setup the ESM message container to include PDN Connectivity Reject
     * message within the Attach Reject message
     */
    bdestroy_wrapper (&emm_context->esm_msg);
    emm_context->esm_msg = esm_sap.send;
    rc = _emm_attach_reject (emm_context);
  } else {
    /*
     * ESM procedure failed and, received message has been discarded or
     * Status message has been returned; ignore ESM procedure failure
     */
    rc = RETURNok;
  }

  if (rc != RETURNok) {
    /*
     * The attach procedure failed
     */
    OAILOG_WARNING (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Failed to respond to Attach Request\n", ue_id);
    emm_context->emm_cause = EMM_CAUSE_PROTOCOL_ERROR;
    /*
     * Do not accept the UE to attach to the network
     */
    rc = _emm_attach_reject (emm_context);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

int emm_cn_wrapper_attach_accept (emm_data_context_t * emm_context)
{
  return _emm_attach_accept (emm_context);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_accept()                                      **
 **                                                                        **
 ** Description: Sends ATTACH ACCEPT message and start timer T3450         **
 **                                                                        **
 ** Inputs:  data:      Attach accept retransmission data          **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3450                                      **
 **                                                                        **
 ***************************************************************************/
static int _emm_attach_accept (emm_data_context_t * emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_sap_t                               emm_sap = {0};
  int                                     rc = RETURNerror;
  mme_ue_s1ap_id_t                        ue_id = PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context)->mme_ue_s1ap_id;


  // may be caused by timer not stopped when deleted context
  if (emm_context) {
    /*
     * Notify EMM-AS SAP that Attach Accept message together with an Activate
     * Default EPS Bearer Context Request message has to be sent to the UE
     */
    emm_sap.primitive = EMMAS_ESTABLISH_CNF;
    emm_sap.u.emm_as.u.establish.ue_id = ue_id;
    emm_sap.u.emm_as.u.establish.nas_info = EMM_AS_NAS_INFO_ATTACH;

    NO_REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__3);
    //----------------------------------------
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__4);
    emm_ctx_set_attribute_valid(emm_context, EMM_CTXT_MEMBER_UE_NETWORK_CAPABILITY_IE);
    emm_ctx_set_attribute_valid(emm_context, EMM_CTXT_MEMBER_MS_NETWORK_CAPABILITY_IE);
    //----------------------------------------
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__5);
    emm_ctx_set_valid_current_drx_parameter(emm_context, &emm_context->_pending_drx_parameter);
    emm_ctx_clear_pending_current_drx_parameter(emm_context);
    //----------------------------------------
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__9);
    // the set of emm_sap.u.emm_as.u.establish.new_guti is for including the GUTI in the attach accept message
    //ONLY ONE MME NOW NO S10
    if (!IS_EMM_CTXT_PRESENT_GUTI(emm_context)) {
      // Sure it is an unknown GUTI in this MME
      guti_t old_guti = emm_context->_old_guti;
      guti_t guti     = {.gummei.plmn = {0},
                         .gummei.mme_gid = 0,
                         .gummei.mme_code = 0,
                         .m_tmsi = INVALID_M_TMSI};
      clear_guti(&guti);

      rc = mme_api_new_guti (&emm_context->_imsi, &old_guti, &guti, &emm_context->originating_tai, &emm_context->_tai_list);
      if ( RETURNok == rc) {
        emm_ctx_set_guti(emm_context, &guti);
        emm_ctx_set_attribute_valid(emm_context, EMM_CTXT_MEMBER_TAI_LIST);
        //----------------------------------------
        REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__6);
        REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__10);
        memcpy(&emm_sap.u.emm_as.u.establish.tai_list, &emm_context->_tai_list, sizeof(tai_list_t));
      } else {
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
      }
    }

    emm_sap.u.emm_as.u.establish.eps_id.guti = &emm_context->_guti;

    if (!IS_EMM_CTXT_VALID_GUTI(emm_context) &&
         IS_EMM_CTXT_PRESENT_GUTI(emm_context) &&
         IS_EMM_CTXT_PRESENT_OLD_GUTI(emm_context)) {
      /*
       * Implicit GUTI reallocation;
       * include the new assigned GUTI in the Attach Accept message
       */
      OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Implicit GUTI reallocation, include the new assigned GUTI in the Attach Accept message\n",
          ue_id);
      emm_sap.u.emm_as.u.establish.new_guti    = &emm_context->_guti;
    } else if (!IS_EMM_CTXT_VALID_GUTI(emm_context) &&
        IS_EMM_CTXT_PRESENT_GUTI(emm_context)) {
      /*
       * include the new assigned GUTI in the Attach Accept message
       */
      OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Include the new assigned GUTI in the Attach Accept message\n", ue_id);
      emm_sap.u.emm_as.u.establish.new_guti    = &emm_context->_guti;
    } else { // IS_EMM_CTXT_VALID_GUTI(ue_mm_context) is true
      emm_sap.u.emm_as.u.establish.new_guti  = NULL;
    }
    //----------------------------------------
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__14);
    emm_sap.u.emm_as.u.establish.eps_network_feature_support = &_emm_data.conf.eps_network_feature_support;

    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data (&emm_sap.u.emm_as.u.establish.sctx, &emm_context->_security, false, true);
    emm_sap.u.emm_as.u.establish.encryption = emm_context->_security.selected_algorithms.encryption;
    emm_sap.u.emm_as.u.establish.integrity = emm_context->_security.selected_algorithms.integrity;
    OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - encryption = 0x%X (0x%X)\n",
        ue_id, emm_sap.u.emm_as.u.establish.encryption, emm_context->_security.selected_algorithms.encryption);
    OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - integrity  = 0x%X (0x%X)\n",
        ue_id, emm_sap.u.emm_as.u.establish.integrity, emm_context->_security.selected_algorithms.integrity);
    /*
     * Get the activate default EPS bearer context request message to
     * transfer within the ESM container of the attach accept message
     */
    attach_data_t         *attach_data = &emm_context->specific_proc->arg.u.attach_data;
    emm_sap.u.emm_as.u.establish.nas_msg = attach_data->esm_msg;
    OAILOG_TRACE (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - nas_msg  src size = %d nas_msg  dst size = %d \n",
        ue_id, blength(attach_data->esm_msg), blength(emm_sap.u.emm_as.u.establish.nas_msg));

    // Send T3402
    emm_sap.u.emm_as.u.establish.t3402 = &mme_config.nas_config.t3402_min;

    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__2);
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMAS_ESTABLISH_CNF ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    rc = emm_sap_send (&emm_sap);

    if (RETURNerror != rc) {
      if (emm_context->T3450.id != NAS_TIMER_INACTIVE_ID) {
        /*
         * Re-start T3450 timer
         */
        emm_context->T3450.id = nas_timer_stop (emm_context->T3450.id);
        MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 stopped UE " MME_UE_S1AP_ID_FMT "", ue_id);
      }
      /*
       * Start T3450 timer
       */
      emm_context->T3450.id = nas_timer_start (emm_context->T3450.sec, 0 /*usec*/,_emm_attach_t3450_handler, emm_context);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 started UE " MME_UE_S1AP_ID_FMT " ", ue_id);

      OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "Timer T3450 (%lx) expires in %ld seconds\n",
          ue_id, emm_context->T3450.id, emm_context->T3450.sec);
    }
  } else {
    OAILOG_WARNING (LOG_NAS_EMM, "ue_mm_context NULL\n");
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_have_changed()                                **
 **                                                                        **
 ** Description: Check whether the given attach parameters differs from    **
 **      those previously stored when the attach procedure has     **
 **      been initiated.                                           **
 **                                                                        **
 ** Inputs:  ctx:       EMM context of the UE in the network       **
 **      type:      Type of the requested attach               **
 **      ksi:       Security ket sey identifier                **
 **      guti:      The GUTI provided by the UE                **
 **      imsi:      The IMSI provided by the UE                **
 **      imei:      The IMEI provided by the UE                **
 **      eea:       Supported EPS encryption algorithms        **
 **      eia:       Supported EPS integrity algorithms         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    true if at least one of the parameters     **
 **             differs; false otherwise.                  **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int
_emm_attach_have_changed (
  const emm_data_context_t * ctx,
  emm_proc_attach_type_t type,
  ksi_t ksi,
  guti_t * guti,
  imsi_t * imsi,
  imei_t * imei,
  int eea,
  int eia,
  int ucs2,
  int uea,
  int uia,
  int gea,
  int umts_present,
  int gprs_present)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);

  /*
   * Emergency bearer services indicator
   */
  if ((type == EMM_ATTACH_TYPE_EMERGENCY) != ctx->is_emergency) {
    OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: type EMM_ATTACH_TYPE_EMERGENCY \n", ctx->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  /*
   * Security key set identifier
   */
  if (ksi != ctx->ue_ksi) {
    OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: ue_ksi %d -> %d \n", ctx->ue_id, ctx->ue_ksi, ksi);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  /*
   * Supported EPS encryption algorithms
   */
  if (eea != ctx->eea) {
    OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: eea 0x%x -> 0x%x \n", ctx->ue_id, ctx->eea, eea);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  /*
   * Supported EPS integrity algorithms
   */
  if (eia != ctx->eia) {
    OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: eia 0x%x -> 0x%x \n", ctx->ue_id, ctx->eia, eia);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if (umts_present != ctx->umts_present) {
    OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: umts_present %d -> %d \n", ctx->ue_id, ctx->umts_present, umts_present);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((ctx->umts_present) && (umts_present)) {
    if (ucs2 != ctx->ucs2) {
      OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: ucs2 %u -> %u \n", ctx->ue_id, ctx->ucs2, ucs2);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }

    /*
     * Supported UMTS encryption algorithms
     */
    if (uea != ctx->uea) {
      OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: uea %u -> %u \n", ctx->ue_id, ctx->uea, uea);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }

    /*
     * Supported UMTS integrity algorithms
     */
    if (uia != ctx->uia) {
      OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: uia %u -> %u \n", ctx->ue_id, ctx->uia, uia);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  if (gprs_present != ctx->gprs_present) {
    OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: gprs_present %u -> %u \n", ctx->ue_id, ctx->gprs_present, gprs_present);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((ctx->gprs_present) && (gprs_present)) {
    if (gea != ctx->gea) {
      OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: gea 0x%X -> 0x%X \n", ctx->ue_id, ctx->gea, gea);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  /*
   * The GUTI if provided by the UE
   */
  if ((guti) && (!IS_EMM_CTXT_PRESENT_OLD_GUTI(ctx))) {
    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed:  guti None ->  " GUTI_FMT "\n", ctx->ue_id, GUTI_ARG(guti));
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((!guti) && (IS_EMM_CTXT_PRESENT_GUTI(ctx))) {
    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed:  guti " GUTI_FMT " -> None\n", ctx->ue_id, GUTI_ARG(guti));
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((guti) && (IS_EMM_CTXT_PRESENT_GUTI(ctx))) {
    if (guti->m_tmsi != ctx->_guti.m_tmsi) {
      OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed:  guti/tmsi " GUTI_FMT " -> " GUTI_FMT "\n", ctx->ue_id, GUTI_ARG(&ctx->_guti), GUTI_ARG(guti));
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
    if ((guti->gummei.mme_code != ctx->_guti.gummei.mme_code) ||
        (guti->gummei.mme_gid != ctx->_guti.gummei.mme_gid) ||
        (guti->gummei.plmn.mcc_digit1 != ctx->_guti.gummei.plmn.mcc_digit1) ||
        (guti->gummei.plmn.mcc_digit2 != ctx->_guti.gummei.plmn.mcc_digit2) ||
        (guti->gummei.plmn.mcc_digit3 != ctx->_guti.gummei.plmn.mcc_digit3) ||
        (guti->gummei.plmn.mnc_digit1 != ctx->_guti.gummei.plmn.mnc_digit1) ||
        (guti->gummei.plmn.mnc_digit2 != ctx->_guti.gummei.plmn.mnc_digit2) ||
        (guti->gummei.plmn.mnc_digit3 != ctx->_guti.gummei.plmn.mnc_digit3)) {
      OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed:  guti/tmsi " GUTI_FMT " -> " GUTI_FMT "\n", ctx->ue_id, GUTI_ARG(&ctx->_guti), GUTI_ARG(guti));
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  /*
   * The IMSI if provided by the UE
   */
  if ((imsi) && (!IS_EMM_CTXT_VALID_IMSI(ctx))) {
    char                                    imsi_str[16];

    IMSI_TO_STRING (imsi, imsi_str, 16);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imsi %s/NULL (ctxt)\n", imsi_str);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((!imsi) && (IS_EMM_CTXT_VALID_IMSI(ctx))) {
    char                                    imsi_str[16];

    IMSI_TO_STRING (&ctx->_imsi, imsi_str, 16);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imsi NULL/%s (ctxt)\n", imsi_str);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((imsi) && (IS_EMM_CTXT_VALID_IMSI(ctx))) {
    if (memcmp (imsi, &ctx->_imsi, sizeof (ctx->_imsi)) != 0) {
      char                                    imsi_str[16];
      char                                    imsi2_str[16];

      IMSI_TO_STRING (imsi, imsi_str, 16);
      IMSI_TO_STRING (&ctx->_imsi, imsi2_str, 16);
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imsi %s/%s (ctxt)\n", imsi_str, imsi2_str);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  /*
   * The IMEI if provided by the UE
   */
  if ((imei) && (!IS_EMM_CTXT_VALID_IMEI(ctx))) {
    char                                    imei_str[16];

    IMEI_TO_STRING (imei, imei_str, 16);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imei %s/NULL (ctxt)\n", imei_str);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((!imei) && (IS_EMM_CTXT_VALID_IMEI(ctx))) {
    char                                    imei_str[16];

    IMEI_TO_STRING (&ctx->_imei, imei_str, 16);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imei NULL/%s (ctxt)\n", imei_str);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((imei) && (IS_EMM_CTXT_VALID_IMEI(ctx))) {
    if (memcmp (imei, &ctx->_imei, sizeof (ctx->_imei)) != 0) {
      char                                    imei_str[16];
      char                                    imei2_str[16];

      IMEI_TO_STRING (imei, imei_str, 16);
      IMEI_TO_STRING (&ctx->_imei, imei2_str, 16);
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imei %s/%s (ctxt)\n", imei_str, imei2_str);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, false);
}


/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_update()                                      **
 **                                                                        **
 ** Description: Update the EMM context with the given attach procedure    **
 **      parameters.                                               **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      type:      Type of the requested attach               **
 **      ksi:       Security ket sey identifier                **
 **      guti:      The GUTI provided by the UE                **
 **      imsi:      The IMSI provided by the UE                **
 **      imei:      The IMEI provided by the UE                **
 **      eea:       Supported EPS encryption algorithms        **
 **      originating_tai Originating TAI (from eNB TAI)        **
 **      eia:       Supported EPS integrity algorithms         **
 **      esm_msg_pP:   ESM message contained with the attach re-  **
 **             quest                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     ctx:       EMM context of the UE in the network       **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int
_emm_attach_update (
  emm_data_context_t * ctx,
  mme_ue_s1ap_id_t ue_id,
  emm_proc_attach_type_t type,
  ksi_t ksi,
  bool is_native_guti,
  guti_t * guti,
  imsi_t * imsi,
  imei_t * imei,
  const tai_t   * const originating_tai,
  const int eea,
  const int eia,
  const int ucs2,
  const int uea,
  const int uia,
  const int gea,
  const bool umts_present,
  const bool gprs_present,
  const_bstring esm_msg_pP)
{

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  /*
   * UE identifier.
   * The notification happens in the MME_APP layer.
   */
  ctx->ue_id = ue_id;

  /*
   * Emergency bearer services indicator
   */
  ctx->is_emergency = (type == EMM_ATTACH_TYPE_EMERGENCY);
  /*
   * Security key set identifier
   */
  if (ctx->ue_ksi != ksi) {
    OAILOG_DEBUG(LOG_NAS_EMM, "UE id " MME_UE_S1AP_ID_FMT " Update ue ksi %d -> %d. Invalidating the current security context. \n", emm_context->ue_id, emm_context->ue_ksi, ksi);
    ctx->ue_ksi = ksi;
    /** Invalidate the security context. */

  }
  /*
   * Supported EPS encryption algorithms
   */
  ctx->eea = eea;

  /*
   * Supported EPS integrity algorithms
   */
  ctx->eia = eia;
  ctx->ucs2 = ucs2;
  ctx->uea = uea;
  ctx->uia = uia;
  ctx->gea = gea;
  ctx->umts_present = umts_present;
  ctx->gprs_present = gprs_present;

  ctx->originating_tai = *originating_tai;
  ctx->is_guti_based_attach = false;

  /*
   * The GUTI if provided by the UE. Trigger UE Identity Procedure to fetch IMSI
   */
  if (guti) {
   ctx->is_guti_based_attach = true;
  }
  /*
   * The IMSI if provided by the UE
   */
  if (imsi) {
    imsi64_t new_imsi64 = INVALID_IMSI64;
    IMSI_TO_IMSI64(imsi,new_imsi64);
    if (new_imsi64 != ctx->_imsi64) {
      emm_ctx_set_valid_imsi(ctx, imsi, new_imsi64);
      emm_data_context_add_imsi (&_emm_data, ctx);
    }
  }

  /*
   * The IMEI if provided by the UE
   */
  if (imei) {
    emm_ctx_set_valid_imei(ctx, imei);
  }

  /*
   * The ESM message contained within the attach request
   */
  if (esm_msg_pP) {
    bdestroy(ctx->esm_msg);
    if (!(ctx->esm_msg = bstrcpy(esm_msg_pP))) {
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }
  }
  /*
   * Attachment indicator
   */
  ctx->is_attached = false;
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

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
  Source      TrackingAreaUpdate.c

  Version     0.1

  Date        2013/05/07

  Product     NAS stack

  Subsystem   EPS Mobility Management

  Author      Frederic Maurel

  Description Defines the tracking area update EMM procedure executed by the
        Non-Access Stratum.

        The tracking area updating procedure is always initiated by the
        UE and is used to update the registration of the actual tracking
        area of a UE in the network, to periodically notify the availa-
        bility of the UE to the network, for MME load balancing, to up-
        date certain UE specific parameters in the network.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "dynamic_memory_check.h"
#include "assertions.h"
#include "log.h"
#include "msc.h"
#include "nas_timer.h"
#include "3gpp_requirements_24.301.h"
#include "common_types.h"
#include "common_defs.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "mme_app_ue_context.h"
#include "emm_proc.h"
#include "common_defs.h"
#include "emm_data.h"
#include "emm_sap.h"
#include "emm_cause.h"
#include "mme_config.h"
#include "mme_app_defs.h"
#include "mme_config.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* TODO Commented some function declarations below since these were called from the code that got removed from TAU request
 * handling function. Reason this code was removed: This portion of code was incomplete and was related to handling of
 * some optional IEs /scenarios that were not relevant for the TAU periodic update handling and might have resulted in 
 * unexpected behaviour/instability.
 * At present support for TAU is limited to handling of periodic TAU request only  mandatory IEs .
 * Other aspects of TAU are TODOs for future.
 */

static int _emm_tracking_area_update_reject( const mme_ue_s1ap_id_t ue_id, const int emm_cause);
static int _emm_tracking_area_update_accept (nas_emm_tau_proc_t * const tau_proc);
static int _emm_tracking_area_update_abort (struct emm_data_context_s *emm_context, struct nas_base_proc_s * base_proc);
static void _emm_tracking_area_update_t3450_handler (void *args);

static int _emm_tracking_area_update_success_identification_cb (emm_data_context_t *emm_context);
static int _emm_tracking_area_update_failure_identification_cb (emm_data_context_t *emm_context);
static int _emm_tracking_area_update_success_authentication_cb (emm_data_context_t *emm_context);
static int _emm_tracking_area_update_failure_authentication_cb (emm_data_context_t *emm_context);
static int _emm_tracking_area_update_success_security_cb (emm_data_context_t *emm_context);
static int _emm_tracking_area_update_failure_security_cb (emm_data_context_t *emm_context);

static int _context_req_proc_success_cb (emm_data_context_t *emm_context);
static int _emm_tracking_area_update_failure_context_res_cb (emm_data_context_t *emm_context);

static int _emm_tracking_area_update_run_procedure(emm_data_context_t *emm_context);
static void _emm_proc_create_procedure_tracking_area_update_request(emm_data_context_t * const ue_context, emm_tau_request_ies_t * const ies);
static bool _emm_tracking_area_update_ies_have_changed (mme_ue_s1ap_id_t ue_id, emm_tau_request_ies_t * const tau_ies1, emm_tau_request_ies_t * const tau_ies2);

static int _emm_tracking_area_update_success_identification_cb (emm_data_context_t *emm_context);
static int _emm_tracking_area_update_failure_identification_cb (emm_data_context_t *emm_context);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

//------------------------------------------------------------------------------
int emm_proc_tracking_area_update_request (
  const mme_ue_s1ap_id_t ue_id,
  emm_tau_request_ies_t *ies,
  const int gea,
  const bool gprs_present,
  const bstring nas_msg,
  int *emm_cause)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  ue_context_t                        *ue_context_p = NULL;
  emm_data_context_t                     *emm_ctx_p = NULL;
  emm_fsm_state_t                         fsm_state = EMM_DEREGISTERED;
  nas_emm_attach_proc_t                  *attach_procedure = NULL;
  nas_emm_tau_proc_t                     *tau_procedure = NULL;

  *emm_cause = EMM_CAUSE_SUCCESS;
  /*
   * Get the UE's EMM context if it exists
   * First check if the MME_APP UE context is valid.
   */
  if (INVALID_MME_UE_S1AP_ID == ue_id) {
    /** Received an invalid UE_ID. */
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  TAU - Received an invalid ue_id. Not continuing with the tracking area update request. \n");
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }
  /** Retrieve the MME_APP UE context. */
  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ue_id);
  DevAssert(ue_context_p);

  OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC-  Tracking Area Update request for new UeId " MME_UE_S1AP_ID_FMT ". TAU_Type=%d, active_flag=%d)\n", ue_id,
      ies->eps_update_type.eps_update_type_value, ies->eps_update_type.active_flag);
  /*
   * Try to retrieve the correct EMM context.
   */
  emm_ctx_p = emm_data_context_get(&_emm_data, ue_id);
  if(!emm_ctx_p){
    /*
     * Get it via GUTI (S-TMSI not set, getting via GUTI).
     * Not */
    if(!(INVALID_M_TMSI != ies->old_guti.m_tmsi)){
      if((emm_ctx_p = emm_data_context_get_by_guti (&_emm_data, &ies->old_guti) != NULL)){ /**< May be set if S-TMSI is set. */
        OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC-  Found a valid UE with correct GUTI " GUTI_FMT " and (old) ue_id " MME_UE_S1AP_ID_FMT ". "
            "Continuing with the Tracking Area Update Request. \n", GUTI_ARG(&emm_ctx_p->_guti), emm_ctx_p->ue_id);
      }
    }
  }else{
    OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC-  Found a valid UE with new mmeUeS1apId " MME_UE_S1AP_ID_FMT " and GUTI " GUTI_FMT ". "
        "Continuing with the Tracking Area Update Request. \n", ue_id, GUTI_ARG(&emm_ctx_p->_guti));
  }
  if(emm_ctx_p){
    mme_ue_s1ap_id_t old_mme_ue_id = emm_ctx_p->ue_id;

    if(emm_ctx_p->ue_id != ue_id){
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The UE_ID EMM context from IMSI " IMSI_64_FMT " with old mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " is not equal to new ueId " MME_UE_S1AP_ID_FMT". \n",
          new_emm_ctx->_imsi64, new_emm_ctx->ue_id, ue_id);
//       new_emm_ue_ctx->ue_id = ue_id; /**< The old UE_ID may still refer to another MME_APP context, with which we may like to continue. */
    }
    /** Start to check the validity of the message and the existing EMM context. */
    fsm_state = emm_fsm_get_state (emm_ctx_p);
    /** Check for any existing specific attach and TAU procedures. */
    if(is_nas_specific_procedure_attach_running(emm_ctx_p)){
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The UE_ID EMM context from IMSI " IMSI_64_FMT " with old mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " has an active attach procedure running. \n",
          new_emm_ctx->_imsi64, new_emm_ctx->ue_id);
      attach_procedure = get_nas_specific_procedure_attach (emm_ctx_p);
    }else{
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The UE_ID EMM context from IMSI " IMSI_64_FMT " with old mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " has NO active attach procedure running. \n",
          new_emm_ctx->_imsi64, new_emm_ctx->ue_id);
    }
    if(is_nas_specific_procedure_tau_running(emm_ctx_p)){
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The UE_ID EMM context from IMSI " IMSI_64_FMT " with old mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " has already an active TAU procedure running. \n",
                new_emm_ctx->_imsi64, new_emm_ctx->ue_id);
      tau_procedure = get_nas_specific_procedure_tau(emm_ctx_p);
    }else{
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The UE_ID EMM context from IMSI " IMSI_64_FMT " with old mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " has NO active TAU procedure running. \n",
          new_emm_ctx->_imsi64, new_emm_ctx->ue_id);
    }

    /*
     * Like in the attach procedure, check for all common procedures without specific procedures, alone.
     */

    /*
     * Check if there is an ongoing SMC procedure.
     * In any case, we just abort the current SMC procedure, which will keep the context, state, other commons and bearers, etc..
     * We continue with the specific procedure. If the SMC was part of the specific procedure, we don't care here. Thats the collision between the specific procedures.
     * No matter what the state is and if the received message differs or not, remove the EMM_CTX (incl. security), bearers, all other common procedures and abort any running specific procedures.
     */
    if (is_nas_common_procedure_smc_running(emm_ctx_p)) {
      REQUIREMENT_3GPP_24_301(R10_5_4_3_7_c);
      nas_emm_smc_proc_t * smc_proc = get_nas_common_procedure_smc(emm_ctx_p);
      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMREG_COMMON_PROC_ABORT;
      emm_sap.u.emm_reg.ue_id     = ue_id;
      emm_sap.u.emm_reg.ctx       = emm_ctx_p;
      emm_sap.u.emm_reg.notify    = false;
      emm_sap.u.emm_reg.free_proc = true;
      emm_sap.u.emm_reg.u.common.common_proc = &smc_proc->emm_com_proc;
      emm_sap.u.emm_reg.u.common.previous_emm_fsm_state = smc_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_PROC_ABORT (SMC) ue id " MME_UE_S1AP_ID_FMT " ", emm_ctx_p->mme_ue_s1ap_id);
      rc = emm_sap_send (&emm_sap);
      // Allocate new context and process the new request as fresh attach request
      DevAssert(rc == RETURNok);
    }

    /*
     * Check if there is an Authentication Procedure running.
     * No collisions are mentioned, but just abort the current authentication procedure, if it was triggered by MME/OAM interface.
     * Consider last UE/EPS_Security context to be valid, keep the state and everything.
     * If it was part of a specific procedure, we will deal with it too, below, anyway.
     */
    if (is_nas_common_procedure_authentication_running(emm_ctx_p)) {
      if (attach_procedure || tau_procedure){
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The EMM context from IMSI " IMSI_64_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " has an an authentication procedure as part of a specific procedure. "
            "Common identification procedure will be aborted (as part of the possibly aborted specific procedure below, if the specific procedure is to be aborted. \n", emm_ctx_p->_imsi64, emm_ctx_p->ue_id);
      } else{
        /** Not defined explicitly in the specifications, but we will abort the common procedure. */
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The EMM context from IMSI " IMSI_64_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " has an authentication procedure running but no specific procedure. "
            "Aborting the authentication procedure running currently. \n", emm_ctx_p->_imsi64, emm_ctx_p->ue_id);
        nas_emm_auth_proc_t * auth_proc = get_nas_common_procedure_authentication(emm_ctx_p);
        emm_sap_t                               emm_sap = {0};
        emm_sap.primitive = EMMREG_COMMON_PROC_ABORT;
        emm_sap.u.emm_reg.ue_id = emm_ctx_p->ue_id; /**< This should be enough to get the EMM context. */
        emm_sap.u.emm_reg.ctx = emm_ctx_p; /**< This should be enough to get the EMM context. */
        emm_sap.u.emm_reg.notify    = false;
        emm_sap.u.emm_reg.free_proc = true;
        emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
        emm_sap.u.emm_reg.u.common.previous_emm_fsm_state = auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
        // TODOdata->notify_failure = true;
        MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_PROC_ABORT (AUTH) ue id " MME_UE_S1AP_ID_FMT " ", emm_ctx_p->mme_ue_s1ap_id);
        rc = emm_sap_send (&emm_sap);
        /** Keep all the states, context, bearers and other common and specific procedures. Continue to check collision with other specific procedures to implicitly detach the old UE context, etc.. */
      }
    }
    /** Check for an identification procedure. */
    if (is_nas_common_procedure_identification_running(emm_ctx_p)) {
       /** If an identification procedure is running, leave it as it is. Continue as the ue_context is valid. */
       REQUIREMENT_3GPP_24_301(R10_5_4_4_6_f); // continue
       OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Leaving the old UE context in IDENTIFICATION procedure as it is for IMSI " IMSI_64_FMT " and & continuing with the TAU request for ue_id = " MME_UE_S1AP_ID_FMT "\n",
           emm_ctx_px->_imsi64, ue_id);
       /** This case will also be handled as part of the specific TAU procedure, although the specific TAU procedure is not set as parent process. */
       // todo: setting the specific TAU procedure as parent is necessary?
     }
    /** Check for collisions with specific procedures. */
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_g);
    if(attach_procedure){
      // todo: When attach reject is sent, attach_procedure will be removed?
      if(is_nas_attach_accept_sent(attach_procedure) || is_nas_attach_reject_sent(attach_procedure)) {// && (!){ /**< Could be in DEREGISTERED or COMMON-PROCEDURE-INITIATED state. Will be in DEREGISTERED state only afterwards. */
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - TAU request received for IMSI " IMSI_64_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " while an Attach Procedure is running (Attach Complete pending). \n", emm_ctx_p->_imsi64, emm_ctx_p->ue_id);
        /** GUTI must be validated. */
        emm_ctx_set_attribute_valid(emm_ctx_p, EMM_CTXT_MEMBER_GUTI);
        /*
         * Abort the specific attach procedure and perform an implicit NAS detach, without sending a Detach Request to the UE (stops the T3450 timer).
         * Aborting an attach procedure currently performs an IMPLICIT Detach (as defined in the specification), but also removes the EMM context.
         * todo: Making optional such that EMM context is only DEREGISTERED with validated GUTI?
         *
         * Currently, in the IMPLICIT DETACH procedure, the EMM context will be removed completely, so
         * validating GUTI for the next attach is meaningless here.
         */
        emm_sap_t                               emm_sap = {0};
        emm_sap.primitive = EMMREG_ATTACH_ABORT;
        emm_sap.u.emm_reg.ue_id = attach_procedure->ue_id;
        emm_sap.u.emm_reg.ctx   = emm_ctx_p;
        emm_sap.u.emm_reg.notify= true;
        emm_sap.u.emm_reg.free_proc = true; /**< Will remove the T3450 timer. */
        emm_sap.u.emm_reg.u.attach.proc   = attach_procedure;
        rc = emm_sap_send (&emm_sap);
        /** Send a TAU-Reject back, removing the new MME_APP UE context, too. */
        rc = emm_proc_tracking_area_update_reject(ue_id, EMM_CAUSE_IMPLICITLY_DETACHED); /**< Will remove the contexts for the TAU-Req. */
        free_emm_tau_request_ies(&ies);
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
      }else{
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - TAU request received for IMSI " IMSI_64_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " while an Attach Procedure is running (Attach Accept/Reject not sent yet). Discarding TAU Request. \n",
            emm_ctx_p->_imsi64, emm_ctx_p->ue_id);
        /** TAU before ATTACH_ACCEPT sent should be rejected directly. */
        rc = emm_proc_tracking_area_update_reject(ue_id, EMM_CAUSE_MSC_NOT_REACHABLE); /**< Will remove the contexts for the TAU-Req. */
        free_emm_tau_request_ies(&ies);
        /** Not touching the ATTACH procedure. */
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);

      }
    /** TAU after attach complete should be handled. */
    }
    /** Check for collisions with TAU procedure. */
    // todo: either tau or attach should exist, so use here else_if instead of if. */
    else if (tau_procedure){
      if(is_nas_tau_accept_sent(tau_procedure) || is_nas_tau_reject_sent(tau_procedure)) {// && (!){
        if (_emm_tracking_area_update_ies_have_changed (emm_ctx_p->ue_id, tau_procedure->ies, ies)) {
          OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - TAU Request parameters have changed\n");
          REQUIREMENT_3GPP_24_301(R10_5_5_3_2_7_d__1);
          /*
           * If one or more of the information elements in the TRACKING AREA UPDATE REQUEST message differ
           * from the ones received within the previous TRACKING AREA UPDATE REQUEST message, the
           * previously initiated tracking area updating procedure shall be aborted if the TRACKING AREA UPDATE COMPLETE message has not
           * been received and the new tracking area updating procedure shall be progressed;
           */
          emm_sap_t                               emm_sap = {0};
          emm_sap.primitive = EMMREG_TAU_ABORT;
          emm_sap.u.emm_reg.ue_id = tau_procedure->ue_id;
          emm_sap.u.emm_reg.ctx   = emm_ctx_p;
          emm_sap.u.emm_reg.notify= true;
          emm_sap.u.emm_reg.free_proc = true;
          emm_sap.u.emm_reg.u.tau.proc   = tau_procedure;
          rc = emm_sap_send (&emm_sap);
          // trigger clean up
//          emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;
//          emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = emm_ctx_p->ue_id;
//          rc = emm_sap_send (&emm_sap);
          // Allocate new context and process the new request as fresh attach request
 //             clear_emm_ctxt = true;
          *emm_ctx_p = NULL;
          /** Continue with a clean emm_ctx. */
        } else {
          REQUIREMENT_3GPP_24_301(R10_5_5_3_2_7_d__2);
          /*
           * - if the information elements do not differ, then the TRACKING AREA UPDATE ACCEPT message shall be resent and the timer
           * T3450 shall be restarted if an ATTACH COMPLETE message is expected. In that case, the retransmission
           * counter related to T3450 is not incremented.
           */
          _emm_tracking_area_update_accept_retx(emm_ctx_p); /**< Resend the TAU_ACCEPT with the old EMM context (with old UE IDs). */
          // Clean up new UE context that was created to handle new tau request
          nas_itti_detach_req(ue_id);
          free_emm_tau_request_ies(&ies);
          OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Received duplicated TAU Request\n");
          OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
        }
      } else if (!is_nas_tau_accept_sent(tau_procedure) && (!is_nas_tau_reject_sent(tau_procedure))) {  /**< TAU accept or reject not sent yet. */
        REQUIREMENT_3GPP_24_301(R10_5_5_3_2_7_e__1);
        if (_emm_tracking_area_update_ies_have_changed (emm_ctx_p->ue_id, tau_procedure->ies, ies)) {
          OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - TAU parameters have changed in an ongoing TAU Request handling. \n");
          /**
           * If one or more of the information elements in the TAU REQUEST message differs from the ones
           * received within the previous TAU REQUEST message, the previously initiated attach procedure shall
           * be aborted and the new tau procedure shall be executed;
           * The TAU accept/reject has not been sent yet. Check if this UE has been attached to the current MME or is trying to do an inter-MME TAU.
           */
          /** The UE has not been attached to the current MME. Abort the process and purge the current context. */
          emm_sap_t                               emm_sap = {0};
          emm_sap.primitive = EMMREG_TAU_ABORT;
          emm_sap.u.emm_reg.ue_id = tau_procedure->ue_id;
          emm_sap.u.emm_reg.ctx   = emm_ctx_p;
          emm_sap.u.emm_reg.notify= true;
          emm_sap.u.emm_reg.free_proc = true;
          emm_sap.u.emm_reg.u.tau.proc   = tau_procedure;
          rc = emm_sap_send (&emm_sap);
          // trigger clean up
//          emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;
//          emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = emm_ctx_p->ue_id;
//          rc = emm_sap_send (&emm_sap);
          /** Continue with the TAU request fresh. */

          // Allocate new context and process the new request as fresh attach request
          //    /** Continue with the TAU request fresh. */
          *emm_ctx_p = NULL;
        } else {
          REQUIREMENT_3GPP_24_301(R10_5_5_3_2_7_e__2);
          /*
           * If the information elements do not differ, then the network shall continue with the previous tau procedure
           * and shall ignore the second TAU REQUEST message.
           */
          // Clean up new UE context that was created to handle new attach request
          nas_itti_detach_req(ue_id);
          free_emm_tau_request_ies(&ies);
          OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Received duplicated Attach Request\n");
          OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
        }
      }
    }
    /** If the TAU request was received in EMM_DEREGISTERED or EMM_COMMON_PROCEDURE_INITIATED state, we should remove the context implicitly. */
    if (EMM_REGISTERED != fsm_state || EMM_COMMON_PROCEDURE_INITIATED != fsm_state){
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Received TAU Request while state is not in EMM_REGISTERED, instead %d. \n.", fsm_state);
      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE; /**< todo: assuming, will delete all common and specific procedures. */
      emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = emm_ctx_p->ue_id;
      rc = emm_sap_send (&emm_sap);
      *emm_ctx_p = NULL;
      /** Continue to handle the TAU Request inside a new EMM context. */
    }
    /** Check that the UE has a valid security context and that the message could be successfully decoded. */
    if (!(IS_EMM_CTXT_PRESENT_SECURITY(emm_ctx_p)
            && S_EMM_CTXT_PRESENT_SECURITY(emm_ctx_p)
            && ies->decode_status.integrity_protected_message
            && ies->decode_status.mac_matched)){
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Received TAU Request while UE context does not have a valid security context. Performing an implicit detach. \n");
      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE; /**< todo: assuming, will delete all common and specific procedures. */
      emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = emm_ctx_p->ue_id;
      rc = emm_sap_send (&emm_sap);
      *emm_ctx_p = NULL;
    }
  }
  /** Check if the old emm_ctx is still existing. */
  if(emm_ctx_p){
    /** We still have the old emm_ctx (was not invalidated/implicitly detached). */
    if(emm_ctx_p->ue_id != ue_id){
      /*
       * Clean up new UE context that was created to handle new tau request.
       * The old EMM context should still have a valid MME_APP context.
       */
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - For TAU-Request handling, removing the old ue_id " MME_UE_S1AP_ID_FMT ". \n", ue_id);
      nas_itti_detach_req(ue_id);  /**< Not sending TAU Reject back. */
    }
  } else{
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - For TAU-Request handling, we removed the old UE with ue_id " MME_UE_S1AP_ID_FMT " or no old EMM context was found. \n", old_mme_ue_id);
    /*
     * In both cases, if the the EMM_UE_CONTEXT is removed (old already existing one), check its ue_id. If its the same as the new the new ue_id, then ignore the message,
     * since we purged the MME_APP UE context to answer the message.
     * If the old UE_Context stays and the mme_ue_s1ap_id differs from the new one, just remove the contexts of the new one.
     * The response (or reaction) will be through the old EMM_CTX.
     */
    /** Create a new EMM context .*/
    OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - Creating a new EMM_CTX for mmeUeS1apId " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    //    if(decode_status->integrity_protected_message != 0 && decode_status->mac_matched == 0){
    /** No matter if it is a security protected message or not, MAC will not be matched. If TAU-Req is handled, the last TAI, should point to the current MME. */
    /** If a security context is available, the message will be discarded. */
    OAILOG_NOTICE (LOG_NAS_EMM, "EMM-PROC  - NAS message IS existing//TAU_MAC not set. Will create a new UE NAS EMM context and forward request to source MME for ue_id = " MME_UE_S1AP_ID_FMT "\n", ue_id);
    /**
     * Before creating UE context check that the originating TAI is reachable.
     * If so, create the UE context, if not send directly an TAU reject back and abort the TAU procedure.
     * No matter if the original NAS message is in or not.
     */
    if(!ies->last_visited_registered_tai){
      OAILOG_ERROR(LOG_NAS_EMM, "EMM-PROC  - Last visited TAI " TAI_FMT " is not present in TAU-Request for UE ue_id = " MME_UE_S1AP_ID_FMT ", where UE has no UE context. "
          "Context needs to be created via an initial attach. Cannot get UE context via S10. \n", TAI_ARG(ies->last_visited_registered_tai), ue_id);
      rc = emm_proc_tracking_area_update_reject (ue_id, EMM_CAUSE_ILLEGAL_UE);
      free_emm_tau_request_ies(&ies);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    }
    /** Check that the origin TAI is not local. */
    if(mme_api_check_tai_local_mme(ies->last_visited_registered_tai)){
      OAILOG_ERROR(LOG_NAS_EMM, "EMM-PROC  - Originating TAI " TAI_FMT " is configured as the current MME but no UE context exists. Proceeding with TAU_Reject for ue_id = " MME_UE_S1AP_ID_FMT "\n",
          TAI_ARG(ies->last_visited_registered_tai), ue_id);
      rc = emm_proc_tracking_area_update_reject (ue_id, EMM_CAUSE_IMPLICITLY_DETACHED);
      free_emm_tau_request_ies(&ies);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    }
    OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - Creating a new EMM context for ueId " MME_UE_S1AP_ID_FMT ". \n", emm_ctx_p->ue_id);
    /** Create a new UE EMM context (may be removed in an error case). */
    emm_ctx_p = (emm_data_context_t *) calloc (1, sizeof (emm_data_context_t));
    if (!emm_ctx_p) {
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to create EMM context\n");
      emm_ctx_p->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      /** Do not accept the UE to attach to the network. */
      rc = _emm_tracking_area_update_reject(ue_id, EMM_CAUSE_NETWORK_FAILURE);
      free_emm_tau_request_ies(&ies);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    }
    /** Initialize the newly created UE context for TAU!. */
//    emm_ctx_p->num_attach_request = 0; /**< Don't increment number of attach requests. */
    emm_ctx_p->_security.ncc = 0;  /**< Since it did not came from handover, the ENB status is initialized.. NCC can begin from 0. */
    emm_ctx_p->ue_id = ue_id;
    OAILOG_NOTICE (LOG_NAS_EMM, "EMM-PROC  - Create EMM context ue_id = " MME_UE_S1AP_ID_FMT "\n", ue_id);
    emm_ctx_p->is_dynamic = true;
    emm_ctx_p->attach_type = EMM_ATTACH_TYPE_EPS; // todo: can this value be retrieved via S10? Forcing to EPS attach.
    emm_ctx_p->additional_update_type = *ies->additional_updatetype;
    emm_ctx_p->emm_cause = EMM_CAUSE_SUCCESS;
    emm_fsm_set_status (ue_id, &emm_ctx_p, EMM_DEREGISTERED);
    /** Clear GUTI, which will be newly allocated and sent with TAU-Accept. */
    emm_ctx_clear_guti(emm_ctx_p);
    emm_ctx_clear_old_guti(emm_ctx_p); /**< Set the old GUTI. */
    emm_ctx_clear_imsi(emm_ctx_p);     // todo
    emm_ctx_clear_imei(emm_ctx_p);
    emm_ctx_clear_imeisv(emm_ctx_p);
    emm_ctx_clear_lvr_tai(emm_ctx_p);  /**< Will be set later in this method. */


    // SETTING EKSI TO 7
    emm_ctx_clear_security(emm_ctx_p); /**< Will be set with the S10 Context Response. Todo: HSS authentication. */
    emm_ctx_clear_non_current_security(emm_ctx_p);
    emm_ctx_clear_auth_vectors(emm_ctx_p);
    emm_ctx_clear_ms_nw_cap(emm_ctx_p);              /**< Will be set in this method & validated with TAU_ACCEPT (not sent to the UE). */
    emm_ctx_clear_ue_nw_cap_ie(emm_ctx_p);           /**< Will be set in this method & validated with TAU_ACCEPT (not sent to the UE). */
    emm_ctx_clear_current_drx_parameter(emm_ctx_p);  /**< Will be set in this method & validated with TAU_ACCEPT (not sent to the UE). */
    emm_ctx_clear_pending_current_drx_parameter(emm_ctx_p); // todo: unknown parameter!
    emm_ctx_clear_eps_bearer_context_status(emm_ctx_p);    /**< Will be set present in this method and validated in TAU_COMPLETE (resent with TAU_ACCEPT). */

    /** ESM init context. */
    esm_init_context(&emm_ctx_p->esm_ctx);
    /*
     * Register the newly created EMM_DATA context.
     * The key in the EMM_DATA_CONTEXT hash table will be the MME_UE_S1AP_ID.
     * Later IMSI will be added, will refer to the ID.
     */
    if (RETURNok != emm_data_context_add (&_emm_data, emm_ctx_p)) {
      OAILOG_CRITICAL(LOG_NAS_EMM, "EMM-PROC  - Tracking Area Update EMM Context could not be inserted in hastables for ueId " MME_UE_S1AP_ID_FMT ". \n", emm_ctx_p->ue_id);
      rc = emm_proc_tracking_area_update_reject (emm_ctx_p, EMM_CAUSE_NETWORK_FAILURE);
      free_emm_tau_request_ies(&ies);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }
    OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC-  Will request security context for newly created UE context by Tracking Area Update for ue_id = " MME_UE_S1AP_ID_FMT "from originating TAI " TAI_FMT, "! \n", ue_id, *last_visited_registered_tai);
  }
  /*
   * Start processing the TAU request with an exist or none EMM context (S10 will be triggered).
   * If no context exist, create and initialize a context.
   * After the validations, create a specific procedure, eve.
   *
   * Mark the specific procedure TAU.
   * todo: Unmark when Reject is sent or timer TAU_ACCEPT runs out (before TAU_COMPLETE)
   * or TAU_COMPLETE arrives. */

  /*
   * Continue with the new or existing EMM context.
   * No running specific TAU procedure is expected.
   */
  _emm_proc_create_procedure_attach_request(emm_ctx_p, ies);
  rc = _emm_tracking_area_update_run_procedure(emm_ctx_p);
  if (rc != RETURNok) {
    OAILOG_WARNING (LOG_NAS_EMM, "Failed to initialize EMM callback functions");
    emm_ctx_p->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    rc = emm_proc_tracking_area_update_reject (emm_ctx_p, EMM_CAUSE_NETWORK_FAILURE);
    free_emm_tau_request_ies(&ies);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }
  /** Keep the IEs in the TAU procedure. Not freeing the IEs. */
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:        emm_proc_tracking_area_update_reject()                    **
 **                                                                        **
 ** Description:                                                           **
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
int emm_proc_tracking_area_update_reject (
  const mme_ue_s1ap_id_t ue_id,
  const int emm_cause)
{
  int                                     rc = RETURNerror;
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  rc = _emm_tracking_area_update_reject (ue_id, emm_cause);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_tracking_area_update_complete()                      **
 **                                                                        **
 ** Description: Terminates the TAU procedure upon receiving TAU           **
 **      Complete message from the UE.                             **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.3.2.4                         **
 **      Upon receiving an TRACKING AREA UPDATE COMPLETE message, the MME  **
 **      shall stop timer T3450, enter state EMM-REGISTERED and consider   **
 **      the GUTI sent in the TRACKING AREA UPDATE ACCEPT message as valid.**
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_data, T3450                           **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_tracking_area_update_complete (
  mme_ue_s1ap_id_t ue_id)
{
  emm_data_context_t                     *emm_context = NULL;
  int                                     rc = RETURNerror;
  emm_sap_t                               emm_sap = {0};
  nas_emm_tau_proc_t                     *tau_proc = NULL;





  OAILOG_FUNC_IN (LOG_NAS_EMM);
  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - EPS attach complete (ue_id=" MME_UE_S1AP_ID_FMT ")\n", ue_id);





  /*
   * Get the UE context
   */
  emm_context = emm_data_context_get(&_emm_data, ue_id);

  if (emm_context) {
    if (is_nas_specific_procedure_tau_running(emm_context)) {
      tau_proc = (nas_emm_tau_proc_t*)emm_context->emm_procedures->emm_specific_proc;

      /*
       * Upon receiving an TRACKING AREA UPDATE COMPLETE message, the MME shall enter state EMM-REGISTERED
       * and consider the GUTI sent in the TRACKING AREA UPDATE ACCEPT message as valid.
       */
      REQUIREMENT_3GPP_24_301(R10_5_5_3_2_4__23);
      emm_ctx_set_attribute_valid(emm_context, EMM_CTXT_MEMBER_GUTI);
      // TODO LG REMOVE emm_context_add_guti(&_emm_data, &ue_context->emm_context);
      emm_ctx_clear_old_guti(emm_context);
      // todo emm_data_context_add_guti(&_emm_data, emm_ctx);

      /*
       * Upon receiving an ATTACH COMPLETE message, the MME shall stop timer T3450
       */
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3450 (%d)\n", emm_ctx->T3450.id);
      tau_proc->T3450.id = nas_timer_stop (tau_proc->T3450.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
      /*
       * Upon receiving an ATTACH COMPLETE message, the MME shall enter state EMM-REGISTERED
       * and consider the GUTI sent in the ATTACH ACCEPT message as valid.
       */

      /** No ESM-SAP message. */

    } else {
      NOT_REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__20);
      OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " TRACKING AREA UPDATE COMPLETE discarded (EMM procedure not found)\n", ue_id);
    }
  } else {
    NOT_REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__20);
    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " TRACKING AREA UPDATE COMPLETE discarded (context not found)\n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }

  /*
   * Set the network attachment indicator
   */
//    emm_data_context->is_attached = true;
  /*
   * Notify EMM that attach procedure has successfully completed
   */
  emm_sap.primitive = EMMREG_TAU_CNF;
  emm_sap.u.emm_reg.ue_id = ue_id;
  emm_sap.u.emm_reg.ctx = emm_context;
  emm_sap.u.emm_reg.notify = true;
  emm_sap.u.emm_reg.free_proc = true;
  emm_sap.u.emm_reg.u.tau.proc = tau_proc;
  rc = emm_sap_send (&emm_sap);

//  unlock_ue_contexts(ue_context);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
 }

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/


static void _emm_proc_create_procedure_tracking_area_update_request(emm_data_context_t * const emm_context, emm_tau_request_ies_t * const ies)
{
  nas_emm_tau_proc_t *tau_proc = nas_new_tau_procedure(&emm_context);
  AssertFatal(tau_proc, "TODO Handle this");
  if (tau_proc) {
    tau_proc->ies = ies;
    tau_proc->ue_id = emm_context->ue_id;
    ((nas_base_proc_t*)tau_proc)->abort = _emm_tracking_area_update_abort;
    ((nas_base_proc_t*)tau_proc)->fail_in = NULL; // No parent procedure
    ((nas_base_proc_t*)tau_proc)->time_out = _emm_tracking_area_update_t3450_handler;

  }
}

/* TODO - Compiled out this function to remove compiler warnings since we don't expect TAU Complete from UE as we dont support implicit 
 * GUTI re-allocation during TAU procedure.
 */
#if 0
static int _emm_tracking_area_update (void *args)
...
#endif
/*
 * --------------------------------------------------------------------------
 * Timer handlers
 * --------------------------------------------------------------------------
 */

/** \fn void _emm_tau_t3450_handler(void *args);
\brief T3450 timeout handler
On the first expiry of the timer, the network shall retransmit the TRACKING AREA UPDATE ACCEPT
message and shall reset and restart timer T3450. The retransmission is performed four times, i.e. on the fifth
expiry of timer T3450, the tracking area updating procedure is aborted. Both, the old and the new GUTI shall be
considered as valid until the old GUTI can be considered as invalid by the network (see subclause 5.4.1.4).
During this period the network acts as described for case a above.
@param [in]args TAU accept data
*/
//------------------------------------------------------------------------------
static void _emm_tracking_area_update_t3450_handler (void *args)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_data_context_t                       *emm_context = (emm_data_context_t *) (args);

  if (!(emm_context)) {
    OAILOG_ERROR (LOG_NAS_EMM, "T3450 timer expired No EMM context\n");
    OAILOG_FUNC_OUT (LOG_NAS_EMM);
  }
  nas_emm_tau_proc_t    *tau_proc = get_nas_specific_procedure_tau(emm_context);

  if (tau_proc){

  // Requirement MME24.301R10_5.5.3.2.7_c Abnormal cases on the network side - T3450 time-out
  /*
   * Increment the retransmission counter
   */
    tau_proc->retransmission_count += 1;
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - T3450 timer expired, retransmission counter = %d", tau_proc->retransmission_count);
    /*
     * Get the UE's EMM context
     */

    if (tau_proc->retransmission_count < TAU_COUNTER_MAX) {
      /*
       * Send attach accept message to the UE
       */
      _emm_tracking_area_update_accept (tau_proc);
    } else {
      /*
       * Abort the TAU procedure
       */
      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMREG_TAU_ABORT;
      emm_sap.u.emm_reg.ue_id     = tau_proc->ue_id;
      emm_sap.u.emm_reg.ctx       = emm_context;
      emm_sap.u.emm_reg.notify    = true;
      emm_sap.u.emm_reg.free_proc = true;
      emm_sap.u.emm_reg.u.attach.is_emergency = false;
      emm_sap_send (&emm_sap);
    }
  }
  OAILOG_FUNC_OUT (LOG_NAS_EMM);
}

/* TODO - Compiled out this function to remove compiler warnings since we don't support reauthetication and change in
 * security context during periodic TAU procedure.
  */
#if 0
/** \fn void _emm_tracking_area_update_security(void *args);
    \brief Performs the tracking area update procedure not accepted by the network.
     @param [in]args UE EMM context data
     @returns status of operation
*/
//------------------------------------------------------------------------------
static int _emm_tracking_area_update_security (emm_data_context_t * emm_context)
...
#endif

/** \fn  _emm_tracking_area_update_reject();
    \brief Performs the tracking area update procedure not accepted by the network.
     @param [in]args UE EMM context data
     @returns status of operation
*/
//------------------------------------------------------------------------------
static int _emm_tracking_area_update_reject( const mme_ue_s1ap_id_t ue_id, const int emm_cause)
  
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNok;
  emm_sap_t                               emm_sap = {0};
  emm_data_context_t                     *emm_context = NULL;

  OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC- Sending Tracking Area Update Reject. ue_id=" MME_UE_S1AP_ID_FMT ", cause=%d)\n",
      ue_id, emm_cause);
  /*
   * Notify EMM-AS SAP that Tracking Area Update Reject message has to be sent
   * onto the network
   */
  emm_sap.primitive = EMMAS_ESTABLISH_REJ;
  emm_sap.u.emm_as.u.establish.ue_id = ue_id;
  emm_sap.u.emm_as.u.establish.eps_id.guti = NULL;

  emm_sap.u.emm_as.u.establish.emm_cause = emm_cause;
  emm_sap.u.emm_as.u.establish.nas_info = EMM_AS_NAS_INFO_TAU;
  emm_sap.u.emm_as.u.establish.nas_msg = NULL;
  /*
   * Setup EPS NAS security data
   */
  emm_context = emm_data_context_get(&_emm_data, ue_id);

  if (emm_context) {
     emm_as_set_security_data (&emm_sap.u.emm_as.u.establish.sctx, &emm_context->_security, false, false);
  } else {
      emm_as_set_security_data (&emm_sap.u.emm_as.u.establish.sctx, NULL, false, false);
  }
  rc = emm_sap_send (&emm_sap);

  // Release EMM context 
  if (emm_context) {
    if(emm_context->is_dynamic) {
      _clear_emm_ctxt(emm_context);
    }
  }

//  unlock_ue_contexts(ue_context);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
int emm_cn_wrapper_tracking_area_update_accept (emm_data_context_t * emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);

  DevAssert(emm_context);
  nas_emm_tau_proc_t                     *tau_proc = get_nas_specific_procedure_tau(emm_context);
  int                                     rc = RETURNerror;

  if(!tau_proc){
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }

  rc = _emm_tracking_area_update_accept (tau_proc);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/** \fn void _emm_tracking_area_update_accept (emm_data_context_t * emm_context,tau_data_t * data);
    \brief Sends ATTACH ACCEPT message and start timer T3450.
     @param [in]emm_context UE EMM context data
     @param [in]data    UE TAU accept data
     @returns status of operation (RETURNok, RETURNerror)
*/

//------------------------------------------------------------------------------
static int _emm_send_tracking_area_update_accept(emm_data_context_t * const emm_context, nas_emm_tau_proc_t * const tau_proc){

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNok;
  emm_sap_t                               emm_sap = {0};
  int                                     i = 0;

  DevAssert(emm_context);
  memset((void*)&emm_sap, 0, sizeof(emm_sap_t)); /**< Set all to 0. */

  /** Get the ECM mode. */
  struct ue_context_s * ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, emm_context->ue_id);
  DevAssert(ue_context_p);

  /**
   * Fill the "data" IE (instead establish).
   * Should work fine, since everything also included in data.
   *
   * If its an ESTABLISH or DATA message does not depend on the active flag.
   * It depends on the ECM connection state.
   */

  /* If active flag is not set to true in TAU request then just send TAU accept. After sending TAU accept initiate
   * S1 context release procedure for the UE if new GUTI is not sent in TAU accept message. Note - At present implicit GUTI
   * reallocation is not supported and hence GUTI is not sent in TAU accept message.
   */
  if(ue_context_p->ecm_state != ECM_CONNECTED){
    /**
     * Check the active flag. If false, set a notification to release the bearers after TAU_ACCEPT/COMPLETE (depending on the EMM state).
     */
    if(!tau_proc->ies->active_flag){
      ue_context_p->pending_bearer_deactivation = true; /**< No matter if we send GUTI and wait for TAU_COMPLETE or not. */
      emm_sap.primitive = EMMAS_DATA_REQ;
    }else{
      /**
       * Notify EMM-AS SAP that Tracking Area Update Accept message together with an Activate
       * Default EPS Bearer Context Request message has to be sent to the UE.
       *
       * When the "Active flag" is not set in the TAU Request message and the Tracking Area Update was not initiated
       * in ECM-CONNECTED state, the new MME releases the signaling connection with UE, according to clause 5.3.5.
       */
      emm_sap.primitive = EMMAS_ESTABLISH_CNF;
      ue_context_p->pending_bearer_deactivation = false; /**< No matter if we send GUTI and wait for TAU_COMPLETE or not. */
    }
  }else{
    emm_sap.primitive = EMMAS_DATA_REQ; /**< We also check the current ECM state to handle active flag. */
    ue_context_p->pending_bearer_deactivation = false; /**< No matter if we send GUTI and wait for TAU_COMPLETE or not. */
  }
  /** Set the rest as data. */
  emm_sap.u.emm_as.u.data.ue_id = emm_context->ue_id; /**< These should also set for data. */
  emm_sap.u.emm_as.u.data.nas_info = EMM_AS_NAS_DATA_TAU;

  NO_REQUIREMENT_3GPP_24_301(R10_5_5_3_2_4__3);
  //----------------------------------------
  REQUIREMENT_3GPP_24_301(R10_5_5_3_2_4__4);
  //----------------------------------------
  emm_ctx_set_valid_ue_nw_cap(emm_context, &tau_proc->ies->ue_network_capability);

  if (tau_proc->ies->ms_network_capability) {
    emm_ctx_set_valid_ms_nw_cap(emm_context, tau_proc->ies->ms_network_capability);
  } else {
    // optional IE
    emm_ctx_clear_ms_nw_cap(emm_context);
  }

  //----------------------------------------
  if (tau_proc->ies->drx_parameter) {
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__5);
    emm_ctx_set_valid_drx_parameter(emm_context, tau_proc->ies->drx_parameter);
  }
  emm_ctx_clear_pending_current_drx_parameter(emm_context);

  //----------------------------------------
  /*
   * Set the GUTI.
   */
  //----------------------------------------
  REQUIREMENT_3GPP_24_301(R10_5_5_3_2_4__1b);
  if (!IS_EMM_CTXT_PRESENT_GUTI(emm_context)) {
    // Sure it is an unknown GUTI in this MME
    guti_t old_guti = emm_context->_old_guti;
    // todo: this is cool
    guti_t guti     = {.gummei.plmn = {0},
        .gummei.mme_gid = 0,
        .gummei.mme_code = 0,
        .m_tmsi = INVALID_M_TMSI};
    clear_guti(&guti);
    /** New GUTI is allocated when ATTACH_ACCEPT/TAU_ACCEPT is sent. */
    rc = mme_api_new_guti (&emm_context->_imsi, &old_guti, &guti, &emm_context->originating_tai, &emm_context->_tai_list); /**< This will increment/update the TAI_LIST. */
    if ( RETURNok != rc) {
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC- Error allocating GUTI @ TAU_ACCEPT for ue_id=" MME_UE_S1AP_ID_FMT ", \n", emm_context->ue_id);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    }
    emm_ctx_set_guti(emm_context, &guti); /**< Set the GUTI as present and continue. */
    emm_ctx_set_attribute_valid(emm_context, EMM_CTXT_MEMBER_TAI_LIST);

    /** Set the GUTI fields. */
    emm_sap.u.emm_as.u.data.eps_id.guti = &emm_context->_guti;


  }
  //----------------------------------------
  /**
   * Set the TAI_LIST valid with TAI_ACCEPT. todo: not checking what it was?
   */
  REQUIREMENT_3GPP_24_301(R10_5_5_3_2_4__1c);
  memcpy(&emm_sap.u.emm_as.u.data.tai_list, &emm_context->_tai_list, sizeof(tai_list_t)); /**< Updated in the EMM context with new GUTI reallocation. */

  /** An old GUTI must always exist. */
  if (!IS_EMM_CTXT_VALID_GUTI(emm_context) &&
      IS_EMM_CTXT_PRESENT_GUTI(emm_context) &&
      IS_EMM_CTXT_PRESENT_OLD_GUTI(emm_context)) {
    /*
     * Implicit GUTI reallocation;
     * include the new assigned GUTI in the Tracking Area Update Accept message
     */
    OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Implicit GUTI reallocation, include the new assigned GUTI in the Tracking Area Update Accept message\n",
        emm_context->ue_id);
    emm_sap.u.emm_as.u.data.new_guti    = &emm_context->_guti;
    emm_sap.u.emm_as.u.data.eps_id.guti = &emm_context->_guti;
  } else {
    OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - UE with IMSI " IMSI_64_FMT " has already a valid GUTI " GUTI_FMT ". "
        "Not including new GUTI in Tracking Area Update Accept message\n", emm_context->_imsi, GUTI_ARG(&emm_context->_guti));
    emm_sap.u.emm_as.u.data.new_guti  = NULL;
    emm_sap.u.emm_as.u.data.eps_id.guti = &emm_context->_guti;
  }
  //----------------------------------------
  REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__14);
  emm_sap.u.emm_as.u.data.eps_network_feature_support = &_emm_data.conf.eps_network_feature_support;
  emm_sap.u.emm_as.u.data.eps_update_result = tau_proc->ies->eps_update_type; /**< Set the UPDATE_RESULT irrelevant of data/establish. */
  emm_sap.u.emm_as.u.data.nas_msg = NULL;

  // TODO : Not setting these values..
//      emm_sap.u.emm_as.u.data.eps_bearer_context_status = NULL;
//      emm_sap.u.emm_as.u.data.location_area_identification = NULL;
//    emm_sap.u.emm_as.u.data.combined_tau_emm_cause = NULL;


    // todo: check these (from develop)
//    emm_sap.u.emm_as.u.establish.eps_bearer_context_status = NULL;
//    emm_sap.u.emm_as.u.establish.location_area_identification = NULL;
//    emm_sap.u.emm_as.u.establish.combined_tau_emm_cause = NULL;
//
//
//    emm_sap.u.emm_as.u.establish.t3423 = NULL;
//    emm_sap.u.emm_as.u.establish.t3412 = NULL;
//    emm_sap.u.emm_as.u.establish.t3402 = NULL;
//    // TODO Reminder
//    emm_sap.u.emm_as.u.establish.equivalent_plmns = NULL;
//    emm_sap.u.emm_as.u.establish.emergency_number_list = NULL;
//
//    emm_sap.u.emm_as.u.establish.eps_network_feature_support = NULL;
//    emm_sap.u.emm_as.u.establish.additional_update_result = NULL;
//    emm_sap.u.emm_as.u.establish.t3412_extended = NULL;
//    emm_sap.u.emm_as.u.establish.nas_msg = NULL; // No ESM container message in TAU Accept message



  /*
   * Setup EPS NAS security data
   */
  emm_as_set_security_data (&emm_sap.u.emm_as.u.data.sctx, &emm_context->_security, false, true);
  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - encryption = 0x%X ", emm_sap.u.emm_as.u.data.encryption);
  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - integrity  = 0x%X ", emm_sap.u.emm_as.u.data.integrity);
  emm_sap.u.emm_as.u.data.encryption = emm_context->_security.selected_algorithms.encryption;
  emm_sap.u.emm_as.u.data.integrity = emm_context->_security.selected_algorithms.integrity;
  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - encryption = 0x%X (0x%X)", emm_sap.u.emm_as.u.data.encryption, emm_context->_security.selected_algorithms.encryption);
  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - integrity  = 0x%X (0x%X)", emm_sap.u.emm_as.u.data.integrity, emm_context->_security.selected_algorithms.integrity);

  //----------------------------------------
  REQUIREMENT_3GPP_24_301(R10_5_5_3_2_4__20);
  emm_sap.u.emm_as.u.data.eps_network_feature_support = &_emm_data.conf.eps_network_feature_support;

  /** Increment the number of tau_accept's sent. */
  tau_proc->tau_accept_sent++;
  rc = emm_sap_send (&emm_sap); /**< This may be a DL_DATA_REQ or an ESTABLISH_CNF, initiating an S1AP connection. */

  if (rc != RETURNerror) {
    if (emm_sap.u.emm_as.u.establish.new_guti != NULL) { /**< A new GUTI was included. Start the timer to wait for TAU_COMPLETE. */
      /*
       * Re-start T3450 timer
       */
      void * timer_callback_arg = NULL;
      nas_stop_T3450(tau_proc->ue_id, &tau_proc->T3450, timer_callback_arg);
      nas_start_T3450 (tau_proc->ue_id, &tau_proc->T3450, tau_proc->emm_spec_proc.emm_proc.base_proc.time_out, emm_context);
//      unlock_ue_contexts(ue_context);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);

//        OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Timer T3450 (%d) expires in %ld seconds (TAU)", emm_ctx->T3450.id, emm_ctx->T3450.sec);
//      }
    }
  } else {
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - emm_ctx NULL");
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_tracking_area_update_accept (nas_emm_tau_proc_t * const tau_proc)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  emm_sap_t                               emm_sap = {0};
  emm_data_context_t                     *emm_context = NULL;

  if ((tau_proc) && (tau_proc->ies)) {

    emm_context = emm_data_context_get(&_emm_data, tau_proc->ue_id);
    if (emm_context) {
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    }

    /**
     * Check the EPS Update type:
     * If it is PERIODIC, no common procedure needs to be activated.
     * UE will stay in the same EMM (EMM-REGISTERED) state.
     *
     * Else allocate a new GUTI.
     */
    if(tau_proc->ies->eps_update_type == EPS_UPDATE_TYPE_PERIODIC_UPDATING){
      /**
       * No common procedure needs to be triggered, and no TAU_ACCEPT_DATA needs to be stored.
       * UE will be in the same EMM_REGISTERED state.
       */
      if(emm_context->_emm_fsm_state != EMM_REGISTERED){
        OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - IMSI " IMSI_64_FMT " is not in EMM_REGISTERED mode but instead %d for PERIODIC_TAU_ACCEPT. "
            "Sending TAU REJECT back and implicitly removing contexts. \n", emm_context->_imsi64, emm_context->_emm_fsm_state);
        rc = _emm_tracking_area_update_reject (emm_context->ue_id, SYSTEM_FAILURE);
      }else{
        /** Send a periodic TAU_ACCEPT back without GUTI, COMMON_PROCEDURE initiation or expecting a TAU_COMPLETE. */
        // todo: GUTI for periodic TAU?!
        rc = _emm_send_tracking_area_update_accept(emm_context, tau_proc);
        nas_delete_tau_procedure(tau_proc);
        /**
         * All parameters must be validated at this point since valid GUTI.
         * Since we area in EMM_REGISTERED state, check the pending session releasion flag and release the state.
         * Sending 2 consecutive SAP messages are anyway possible.
         */
//        DevAssert(emm_ctx_p->_emm_fsm_status == EMM_REGISTERED);
        // todo: when to set TAU_ACCEPT_SENT or remove TAU specific procedure?
      }
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    }

    /**
     * If no valid GUTI is present, a new GUTI will be allocated in the following tau_accept method.
     * It will determine if COMMON procedure is activated or the timer T3450 will be started.
     * If a GUTI exists, the UE must be in EMM-REGISTERED state, else we send a reject and implicitly detach the UE context.
     */
    if (IS_EMM_CTXT_VALID_GUTI(emm_context)) { /**< If its invalid --> directly enter EMM-REGISTERED state from EMM-DEREGISTERED state, no T3450, no COMMON_PROCEDURE. */
      /** Assert that the UE is in REGISTERED state. */
      if(emm_context->_emm_fsm_state != EMM_REGISTERED){
        OAILOG_ERROR(LOG_NAS_EMM, "EMM-PROC  - IMSI " IMSI_64_FMT " has already a valid GUTI "GUTI_FMT " but is not in EMM-REGISTERED state, instead %d. \n",
            emm_context->_imsi64, GUTI_ARG(&emm_context->_guti), emm_context->_emm_fsm_state);
        rc = _emm_tracking_area_update_reject (emm_context->ue_id, SYSTEM_FAILURE);
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
      }
      OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - IMSI " IMSI_64_FMT " has already a valid GUTI "GUTI_FMT ". A new GUTI will not be allocated and send. UE staying in EMM_REGISTERED state. \n",
          emm_ctx_p->_imsi64, GUTI_ARG(&emm_ctx_p->_guti));
      /**
       * Not allocating GUTI. No parameters should be validated here.
       * All the parameters should already be validated with the TAU_COMPLETE/ATTACH_COMPLETE before!
       * todo: check that there are no new parameters (DRX, UE/MS NC,).
       */
      DevAssert(IS_EMM_CTXT_VALID_UE_NETWORK_CAPABILITY(emm_context));
      DevAssert(IS_EMM_CTXT_VALID_MS_NETWORK_CAPABILITY(emm_context));
      DevAssert(IS_EMM_CTXT_VALID_CURRENT_DRX_PARAMETER(emm_context));

      /**
       * Send a TAU-Accept without GUTI.
       * Not waiting for TAU-Complete.
       * Might release bearers depending on the ECM state and the active flag without waiting for TAU-Complete.
       */
      /** Send the TAU accept. */
      // todo: GUTI for periodic TAU?!
      rc = _emm_send_tracking_area_update_accept (emm_context, tau_proc);
//      emm_ctx_unmark_specific_procedure(emm_context, EMM_CTXT_SPEC_PROC_TAU); /**< Just marking and not setting timer (will be set later with TAU_ACCEPT). */
      nas_delete_tau_procedure(tau_proc);

      /** No GUTI will be set. State should not be changed. */
      /**
       * All parameters must be validated at this point since valid GUTI.
       * Since we area in EMM_REGISTERED state, check the pending session releasion flag and release the state.
       * Sending 2 consecutive SAP messages are anyway possible.
       */
//      DevAssert(emm_ctx_p->_emm_fsm_status == EMM_REGISTERED);
      /**
       * The NAS_INFO (EMM_AS_NAS_DATA_TAU) --> will trigger a bearer removal if the UE is in EMM_REGISTERED state, later in (_emm_as_data_req),
       * depending on the pending bearer removal flag.
       */

      // todo: if later a new GUTI is still send, although a VALID GUTI exists, invalidate the existing valid GUTI of the emm_ctx before sending tau_accept with new_guti.
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);


    }else if (IS_EMM_CTXT_PRESENT_GUTI(emm_context)){
      /** This case should be handled in the old_context verification when the TAU_REQUEST is arrived. */
      OAILOG_ERROR(LOG_NAS_EMM, "EMM-PROC  - IMSI " IMSI_64_FMT " has a PRESENT but INVALID GUTI "GUTI_FMT ". This indicated a TAU/ATTACH Request "
          "message is received before the complete message for the request before is received by the MME. UE_State %d. \n",
          emm_context->_imsi64, GUTI_ARG(&emm_context->_guti), emm_context->_emm_fsm_state);
      rc = _emm_tracking_area_update_reject (emm_context->ue_id, SYSTEM_FAILURE);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    } else if (!IS_EMM_CTXT_PRESENT_OLD_GUTI(emm_context)) {  /**< Cleared with TAU/ATTACH Complete. Should be received with TAU_REQUEST. */
      /** If we received a TAU-Request where no OLD-GUTI is present and also no VALID GUTI, its an error. */
      OAILOG_ERROR(LOG_NAS_EMM, "EMM-PROC  - IMSI " IMSI_64_FMT " has not valid GUTI and OLD-GUTI is not present. UE_State %d. \n",
          emm_context->_imsi64, emm_context->_emm_fsm_state);
      rc = _emm_tracking_area_update_reject (emm_context->ue_id, SYSTEM_FAILURE);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }
    /**
     * Neither valid nor present GUTI at the MME. Allocating a new GUTI, entering the COMMON_PROCEDURE_INITIATEd state and starting the T3450 timer.
     * No common process needs to be allocated. No common procedure needs to be initialized. It will be done outside of this in emm_cn.
     */
    /**
     * Set the active_flag indicator and t
     */
//    tau_accept_data_p->active_flag = epsupdatetype->activeflag;
//    tau_accept_data_p->eps_update_type = epsupdatetype->epsupdatetypevalue;
     /**
     * No need to free the tau_data, since in this case, we always send new GUTI and expect TAU_COMPLETE!
     * Inside the TAU_ACCEPT procedure, it will be checked if a new GUTI is/will be allocated (currently depends on active_flag.
     * If so, the T3450 timer will be started in the TAU_ACCEPT method already.
     *
     * The pending bearer removal flag may or may not be set. If the state is COMMON, the bearer will be idled when TAU_COMPLETE received and
     * UE enters EMM_REGISTERED state (in the callback function).
     * If TAU_COMPLETE does not arrive, the T3450 timer will implicitly detach the UE anyway.
     *
     * A new GUTI must be allocated at this point (PRESENT).
     * OLD_GUTI will be set with TAU_REQUEST (S10/initial).
     */
    if (IS_EMM_CTXT_PRESENT_OLD_GUTI(emm_context) &&
        (memcmp(&emm_context->_old_guti, &emm_context->_guti, sizeof(emm_context->_guti)))) {
      /** Send TAU accept to the UE. */
       rc = _emm_send_tracking_area_update_accept(emm_context, tau_proc);

       /**
       * Implicit GUTI reallocation;
       * * * * Notify EMM that common procedure has been initiated
       */
      emm_sap_t                               emm_sap = {0};

      emm_sap.primitive = EMMREG_COMMON_PROC_REQ;
      emm_sap.u.emm_reg.ue_id = emm_context->ue_id;
      emm_sap.u.emm_reg.ctx  = emm_context;
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_COMMON_PROC_REQ ue id " MME_UE_S1AP_ID_FMT " ", msg_pP->ue_id);
      rc = emm_sap_send (&emm_sap);
    } else{
      OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - No new GUTI could be allocated for IMSI " IMSI_64_FMT". \n", emm_context->_imsi64);
      rc = _emm_tracking_area_update_reject (emm_context->ue_id, SYSTEM_FAILURE);
    }
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_tracking_area_update_abort (struct emm_data_context_s *emm_context, struct nas_base_proc_s * base_proc)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  if (emm_context) {
    nas_emm_tau_proc_t    *tau_proc = get_nas_specific_procedure_tau(emm_context);

    if (tau_proc) {
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Abort the TAU procedure (ue_id=" MME_UE_S1AP_ID_FMT ")", emm_context->ue_id);

      /*
       * Stop timer T3450 (if exists).
       */
      void * timer_callback_args = NULL;
      nas_stop_T3450(tau_proc->ue_id, &tau_proc->T3450, timer_callback_args);

      /*
       * TAU only should be aborted. No explicit REJECT has to be sent.
       * Currently, for _emm_attach_abort() and for _emm_tracking_area_update_abort() we implicitly detach the UE.
       * todo: should this not be  done with DEREGISTER? (leaving the EMM/Security context)
       * todo: how to define in NAS_IMPLICIT_DETACH if EMM/Security context should stay or not?
       */
      emm_sap_t                               emm_sap = {0};

      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;
      emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = emm_context->ue_id;
      rc = emm_sap_send (&emm_sap);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}


/*
 * Description: Check whether the given tracking area update parameters differs from
 *      those previously stored when the tracking area update procedure has
 *      been initiated.
 *
 * Outputs:     None
 *      Return:    true if at least one of the parameters
 *             differs; false otherwise.
 *      Others:    None
 *
 */
//-----------------------------------------------------------------------------
static bool _emm_tracking_area_update_ies_have_changed (mme_ue_s1ap_id_t ue_id, emm_tau_request_ies_t * const ies1, emm_tau_request_ies_t * const ies2)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);

//  if (ies1->type != ies2->type) {
//    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: type EMM_ATTACH_TYPE\n", ue_id);
//    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
//  }
//  if (ies1->is_native_sc != ies2->is_native_sc) {
//    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: Is native security context\n", ue_id);
//    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
//  }

  /*
   * Security key set identifier
   */
  if (ies1->ksi != ies2->ksi) {
    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: KSI %d -> %d \n", ue_id, ies1->ksi, ies2->ksi);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  /*
   * The GUTI if provided by the UE
   */
  if (*ies1->old_guti_type != *ies2->old_guti_type) {
    OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: Native GUTI %d -> %d \n", ue_id, *ies1->old_guti_type, *ies2->old_guti_type);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }
  if ((ies1->old_guti) && (!ies2->old_guti)) {
    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed:  GUTI " GUTI_FMT " -> None\n", ue_id, GUTI_ARG(ies1->guti));
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((!ies1->old_guti) && (ies2->old_guti)) {
    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed:  GUTI None ->  " GUTI_FMT "\n", ue_id, GUTI_ARG(ies2->guti));
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((ies1->old_guti) && (ies2->old_guti)) {
    if (memcmp(ies1->old_guti, ies2->old_guti, sizeof(*ies1->old_guti))) {
      OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed:  guti/tmsi " GUTI_FMT " -> " GUTI_FMT "\n", ue_id,
          GUTI_ARG(ies1->old_guti), GUTI_ARG(ies2->old_guti));
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);

      // todo: verify, else
//      * If the EMM context has a GUTI, the received OLD GUTI should be equal to the stored GUTI.
//      */
//     if ((old_guti) && (IS_EMM_CTXT_PRESENT_GUTI(ctx))) {
//       if (old_guti->m_tmsi != ctx->_guti.m_tmsi) {
//         OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  tau changed:  old_guti/tmsi " GUTI_FMT " -> " GUTI_FMT "\n", ctx->ue_id, GUTI_ARG(&ctx->_guti), GUTI_ARG(old_guti));
//         OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
//       }
//       if ((old_guti->gummei.mme_code != ctx->_guti.gummei.mme_code) ||
//           (old_guti->gummei.mme_gid != ctx->_guti.gummei.mme_gid) ||
//           (old_guti->gummei.plmn.mcc_digit1 != ctx->_guti.gummei.plmn.mcc_digit1) ||
//           (old_guti->gummei.plmn.mcc_digit2 != ctx->_guti.gummei.plmn.mcc_digit2) ||
//           (old_guti->gummei.plmn.mcc_digit3 != ctx->_guti.gummei.plmn.mcc_digit3) ||
//           (old_guti->gummei.plmn.mnc_digit1 != ctx->_guti.gummei.plmn.mnc_digit1) ||
//           (old_guti->gummei.plmn.mnc_digit2 != ctx->_guti.gummei.plmn.mnc_digit2) ||
//           (old_guti->gummei.plmn.mnc_digit3 != ctx->_guti.gummei.plmn.mnc_digit3)) {
//         OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  tau changed:  old_guti/tmsi " GUTI_FMT " -> " GUTI_FMT "\n", ctx->ue_id, GUTI_ARG(&ctx->_guti), GUTI_ARG(old_guti));
//         OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
//       }
//     }

    }
  }

  /*
   * The Last visited registered TAI if provided by the UE
   */
  if ((ies1->last_visited_registered_tai) && (!ies2->last_visited_registered_tai)) {
    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: LVR TAI " TAI_FMT "/NULL\n", ue_id, TAI_ARG(ies1->last_visited_registered_tai));
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((!ies1->last_visited_registered_tai) && (ies2->last_visited_registered_tai)) {
    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: LVR TAI NULL/" TAI_FMT "\n", ue_id, TAI_ARG(ies2->last_visited_registered_tai));
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((ies1->last_visited_registered_tai) && (ies2->last_visited_registered_tai)) {
    if (memcmp (ies1->last_visited_registered_tai, ies2->last_visited_registered_tai, sizeof (*ies2->last_visited_registered_tai)) != 0) {
      OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: LVR TAI " TAI_FMT "/" TAI_FMT "\n", ue_id,
          TAI_ARG(ies1->last_visited_registered_tai), TAI_ARG(ies2->last_visited_registered_tai));
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  /*
   * todo: from S1AP?
   * Originating TAI
   */
//  if ((ies1->originating_tai) && (!ies2->originating_tai)) {
//    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: orig TAI " TAI_FMT "/NULL\n", ue_id, TAI_ARG(ies1->originating_tai));
//    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
//  }
//
//  if ((!ies1->originating_tai) && (ies2->originating_tai)) {
//    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: orig TAI NULL/" TAI_FMT "\n", ue_id, TAI_ARG(ies2->originating_tai));
//    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
//  }
//
//  if ((ies1->originating_tai) && (ies2->originating_tai)) {
//    if (memcmp (ies1->originating_tai, ies2->originating_tai, sizeof (*ies2->originating_tai)) != 0) {
//      OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: orig TAI " TAI_FMT "/" TAI_FMT "\n", ue_id,
//          TAI_ARG(ies1->originating_tai), TAI_ARG(ies2->originating_tai));
//      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
//    }
//  }

//  /*
//   * todo: from S1AP?
//   * Originating ECGI
//   */
//  if ((ies1->originating_ecgi) && (!ies2->originating_ecgi)) {
//    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: orig ECGI\n", ue_id);
//    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
//  }
//
//  if ((!ies1->originating_ecgi) && (ies2->originating_ecgi)) {
//    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: orig ECGI\n", ue_id);
//    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
//  }
//
//  if ((ies1->originating_ecgi) && (ies2->originating_ecgi)) {
//    if (memcmp (ies1->originating_ecgi, ies2->originating_ecgi, sizeof (*ies2->originating_ecgi)) != 0) {
//      OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: orig ECGI\n", ue_id);
//      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
//    }
//  }

  /*
   * UE network capability
   */
  if (memcmp(&ies1->ue_network_capability, &ies2->ue_network_capability, sizeof(ies1->ue_network_capability))) {
    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: UE network capability\n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  /*
   * MS network capability
   */
  if ((ies1->ms_network_capability) && (!ies2->ms_network_capability)) {
    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: MS network capability\n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((!ies1->ms_network_capability) && (ies2->ms_network_capability)) {
    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: MS network capability\n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((ies1->ms_network_capability) && (ies2->ms_network_capability)) {
    if (memcmp (ies1->ms_network_capability, ies2->ms_network_capability, sizeof (*ies2->ms_network_capability)) != 0) {
      OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: MS network capability\n", ue_id);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, false);
}


/*
 * --------------------------------------------------------------------------
 * Functions that may initiate EMM common procedures
 * --------------------------------------------------------------------------
 */

//------------------------------------------------------------------------------
static int _emm_tracking_area_update_run_procedure(emm_data_context_t *emm_ctx_p)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  nas_emm_tau_proc_t                     *tau_proc = get_nas_specific_procedure_tau(emm_context);
  ue_context_t                        *ue_context_p = NULL;

  /** Retrieve the MME_APP UE context. */
  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_ctx_p->ue_id);
  DevAssert(ue_context_p);

  if (tau_proc) {
    REQUIREMENT_3GPP_24_301(R10_5_5_3_2_3__2);

    /** Updated the last visited TA via the new_tai received from the S1AP IEs. */
    if(tau_proc->ies->last_visited_registered_tai){
      OAILOG_INFO (LOG_NAS_EMM, "TrackingAreaUpdate - UPDATING LAST VISITED REGISTERED TAI\n");
      emm_ctx_set_valid_lvr_tai(emm_ctx_p, tau_proc->ies->last_visited_registered_tai);
      OAILOG_INFO (LOG_NAS_EMM, "TrackingAreaUpdate - UPDATED LAST VISITED REGISTERED TAI\n");
    }else{
      OAILOG_ERROR (LOG_NAS_EMM, "TrackingAreaUpdate - No LAST VISITED REGISTERED TAI PRESENT IN TAU!\n", tau_proc->ue_id);
      /** Deal with it.. */
      emm_ctx_clear_lvr_tai(emm_ctx_p);
    }

    emm_ctx_p->originating_tai = *originating_tai;

    /** Setting the UE & MS network capabilities as present and validating them after replaying / successfully SMC. */
    emm_ctx_set_ue_nw_cap(emm_context, &tau_proc->ies->ue_network_capability);

    /**
     * Requirements MME24.301R10_5.5.3.2.4_2
     */
    if (tau_proc->ies->ue_network_capability) {
      emm_ctx_set_ue_nw_cap((emm_data_context_t * const)emm_ctx_p, tau_proc->ies->ue_network_capability);
    }else{
      emm_ctx_clear_ue_nw_cap(emm_ctx_p);
    }
    if (tau_proc->ies->ms_network_capability) {
      emm_ctx_set_ms_nw_cap(emm_ctx_p, tau_proc->ies->ms_network_capability);
    }else{
      emm_ctx_clear_ms_nw_cap(emm_ctx_p);
//      emm_ctx_p->gea = (msg->msnetworkcapability.gea1 << 6)| msg->msnetworkcapability.egea;
//      emm_ctx_p->gprs_present = true; /**< Todo: how to find this out? */
    }
    //    emm_ctx_p->gea = (msg->msnetworkcapability.gea1 << 6)| msg->msnetworkcapability.egea;
    //    emm_ctx_p->gprs_present = true; /**< Todo: how to find this out? */

    /** Not working with TAU_REQUEST number. */
    OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC-  Num TAU_REQUESTs for UE with GUTI " GUTI_FMT " and ue_id " MME_UE_S1AP_ID_FMT " is: %d. \n", GUTI_ARG(&emm_ctx->_guti), ue_id, emm_ctx->num_tau_request);

    /** Check for TAU Reject after validating the old context or creating a new one! */
    /*
     * Requirement MME24.301R10_5.5.3.2.4_5a
     */
    if(tau_proc->ies->eps_bearer_context_status){
      // todo: handle this with dedicated bearers.
      // todo: need to send TAU_REJECT?
      //    emm_ctx_set_eps_bearer_context_status(emm_ctx, &msg->epsbearercontextstatus); /**< This will indicate bearers in active mode. */
      //    //#pragma message  "TODO Requirement MME24.301R10_5.5.3.2.4_5a: TAU Request: Deactivate EPS bearers if necessary (S11 Modify Bearer Request)"
      //    // todo:
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC- Tracking Area Update Request: Bearer context update not implemented.\n");
    }

    /** Update the TAI List with the new TAI parameters in the S1AP message. */
    // todo: not sure if needed, tai list updated with new guti
//    rc = mme_api_add_tai(&emm_ctx->ue_id, &new_tai, &emm_ctx->_tai_list);
//    if ( RETURNok == rc) {
//      OAILOG_INFO(LOG_NAS_EMM, "TrackingAreaUpdate - Successfully updated TAI list of EMM context!\n");
//    } else {
//      OAILOG_ERROR(LOG_NAS_EMM, "TrackingAreaUpdate - Error updating TAI list of EMM context!\n");
//      rc = emm_proc_tracking_area_update_reject (ue_id, EMM_CAUSE_TA_NOT_ALLOWED);
//      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
//    }

    /** Set the Old-GUTI (event is context there) as present. */
    emm_ctx_set_old_guti((emm_data_context_t * const)emm_ctx_p, &tau_proc->ies->old_guti);
    /** The UE context may have a valid GUTI. It might be equal to the old GUTI? .*/

    /** Set the GUTI-Type (event is context there). */
    if (tau_proc->ies->old_guti_type) {
      /** Old GUTI Type present. Not setting anything in the EMM_CTX. */
    }else{
    }
    /*
     * Requirements MME24.301R10_5.5.3.2.4_4
     */
    if (tau_proc->ies->drx_parameter) {
      emm_ctx_set_drx_parameter(emm_ctx_p, tau_proc->ies->drx_parameter);
    }

    /**
     * Requirement MME24.301R10_5.5.3.2.4_6:
     * todo: location area not handled
     *
     * MME24.301R10_5.5.3.2.4_6 Normal and periodic tracking area updating procedure accepted by the network UE - EPS update type
     * If the EPS update type IE included in the TRACKING AREA UPDATE REQUEST message indicates "periodic updating", and the UE was
     * previously successfully attached for EPS and non-EPS services, subject to operator policies the MME should allocate a TAI
     * list that does not span more than one location area.
     */

    /*
     * Requirements MME24.301R10_5.5.3.2.4_3
     */
    if (tau_proc->ies->is_ue_radio_capability_information_update_needed) {
      // Note: this is safe from double-free errors because it sets to NULL
      // after freeing, which free treats as a no-op.
      // todo: ue_radio_capability from MME_APP to NAS.
      bdestroy_wrapper(&ue_context_p->ue_radio_capability);
    }

    /** A security context exist, checking if UE was correctly authenticated. */
    if (IS_EMM_CTXT_PRESENT_SECURITY(emm_ctx_p)
        && S_EMM_CTXT_PRESENT_SECURITY(emm_ctx_p)) {
      /** Consider the ESM security context of the UE as valid and continue to handle the ESM message with making a ULR at the HSS. */
      OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - EMM context for the ue_id=" MME_UE_S1AP_ID_FMT " has a valid and active EPS security context. "
          "Continuing with the tracking area update request. \n", emm_ctx_p->ue_id);
      /*
       * No AUTH or SMC is needed, directly perform ULR/Subscription (and always perform it
       * due to MeshFlow reasons). Assume that the UE/MS network capabilites are not changed and don't need to be
       * replayed.
       */
      /** Check if any S6a Subscription information exist, if not pull them from the HSS. */
      OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC- THE UE with ue_id=" MME_UE_S1AP_ID_FMT ", has already a security context. Checking for a subscription profile. \n", emm_ctx_p->ue_id);
      if(ue_context_p->subscription_known == SUBSCRIPTION_UNKNOWN) { /**< Means, that the MM UE context is received from the sourc MME already due HO (and only due HO). */
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC- THE UE with ue_id=" MME_UE_S1AP_ID_FMT ", does not have a subscription profile set. Requesting a new subscription profile. \n",
            emm_ctx_p->ue_id, EMM_CAUSE_IE_NOT_IMPLEMENTED);
        /** The EPS update type will be stored as pending IEs. */
//       todo: inform the ESM to send ULR!  rc = mme_app_send_s6a_update_location_req(ue_context_p);
      } else{
        OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC- Sending Tracking Area Update Accept for UE with valid subscription ue_id=" MME_UE_S1AP_ID_FMT ", active flag=%d)\n", ue_id, active_flag);
        /* Check the state of the EMM context. If it is REGISTERED, send an TAU_ACCEPT back and remove the tau procedure. */
        if(emm_context->_emm_fsm_state == EMM_REGISTERED){
          rc = _emm_tracking_area_update_accept (tau_proc);
          nas_delete_tau_procedure(emm_ctx_p);
        }else{
          OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - EMM context for the ue_id=" MME_UE_S1AP_ID_FMT " has a valid and active EPS security context and subscription but is not in EMM_REGISTERED state. Instead %d. "
              "Continuing with the tracking area update reject. \n", emm_ctx_p->ue_id);
          rc = emm_proc_tracking_area_update_reject (emm_ctx_p, EMM_CAUSE_NETWORK_FAILURE);
          OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
        }
      }
      /** Not finishing the TAU procedure. */
    }else{
      /** Check if the origin TAI is a neighboring MME where we can request the UE MM context. */
      if(mme_app_check_target_tai_neighboring_mme(tau_proc->ies->last_visited_registered_tai) == -1){
        OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - For UE " MME_UE_S1AP_ID_FMT " the last visited TAI " TAI_FMT " is not configured as a MME S10 neighbor. "
            "Proceeding with identification procedure. \n", TAI_FMT(ies->last_visited_registered_tai), emm_ctx_p->ue_id);
        /** Invalidate the EMM context. */
        OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - EMM context for the ue_id=" MME_UE_S1AP_ID_FMT " missing a valid security context. \n", emm_ctx_p->ue_id);
        rc = emm_proc_identification (emm_ctx_p, (nas_emm_proc_t *)tau_proc, IDENTITY_TYPE_2_IMSI, _emm_tracking_area_update_success_identification_cb, _emm_tracking_area_update_failure_identification_cb);
      }else{
        /*
         * Originating TAI is configured as an MME neighbor.
         * Will create the UE context and send an S10 UE Context Request. */
        OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - Originating TAI " TAI_FMT " is configured as a MME S10 neighbor. Will request UE context from source MME for ue_id = " MME_UE_S1AP_ID_FMT ". "
            "Creating a new EMM context. \n", TAI_ARG(ies->last_visited_registered_tai), ue_id);
        /*
         * Check if there is a S10 handover procedure running.
         * (We may have received the signal as NAS uplink data request, without any intermission from MME_APP layer).
         */
        mme_app_s10_proc_inter_mme_handover_t * s10_proc_inter_mme_handover = mme_app_get_s10_procedure_inter_mme_handover(ue_context_p);
        if(s10_proc_inter_mme_handover){
          DevAssert(s10_proc_inter_mme_handover->mm_eps_context);
          OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - We have receive the TAU as part of an S10 procedure for ue_id = " MME_UE_S1AP_ID_FMT ". "
              "Continuing with the pending MM EPS Context. \n", ue_id);
          /*
           * No need to start a new EMM_CN_CONTEXT_REQ procedure.
           * Will update the current EMM and ESM context with the pending values and continue with the registration in the HSS (ULR).
           */
        }

        /**
         * Send an S10 context request. Send the PLMN together (will be serving network IE). New TAI not need to be sent.
         * Set the new TAI and the new PLMN to the UE context. In case of a TAC-Accept, will be sent and registered.
         */
        OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC- Security context for EMM_DATA_CONTEXT is missing. Sending a S10 context request for UE with ue_id=" MME_UE_S1AP_ID_FMT " to old MME. \n", emm_ctx_p->ue_id);
        /**
         * Directly inform the MME_APP of the new context.
         * Depending on if there is a pending MM_EPS_CONTEXT or not, it should send S10 message..
         * MME_APP needs to register the MME_APP context with the IMSI before sending S6a ULR.
         */
        rc = _start_context_request_procedure(emm_ctx_p, tau_proc,
            _context_req_proc_success_cb, emm_proc_identification);
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
      }
    }
    /** Not freeing the IEs of the TAU procedure. */
  }else{
    OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - For UE " MME_UE_S1AP_ID_FMT " no TAU specific procedure is running. Not proceeding with TAU Request. ", emm_ctx_p->ue_id);
    rc = emm_proc_tracking_area_update_reject (emm_ctx_p, EMM_CAUSE_NETWORK_FAILURE);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

//------------------------------------------------------------------------------
static int _context_req_proc_success_cb (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  /** Get the specific and the CN procedure. */
  nas_emm_tau_proc_t                    *tau_proc = get_nas_specific_procedure_tau(emm_context);
  // todo: unify later the two procedures (all CN procedures (MME_APP, NAS,... into one).

  nas_s10_context_t                     *nas_s10_ctx = NULL;

  DevAssert(tau_proc);

  nas_ctx_req_proc_t *nas_ctx_req_proc = get_nas_cn_procedure_ctx_req(emm_context);
  if(!nas_ctx_req_proc){
    ue_context_t *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);
    DevAssert(ue_context);
    mme_app_s10_proc_inter_mme_handover_t *s10_inter_mme_handover_proc = mme_app_get_s10_procedure_inter_mme_handover(ue_context);
    DevAssert(s10_inter_mme_handover_proc); // todo: optimize this as well, either s10 handover procedure or nas context request procedure should exist
    nas_s10_ctx = &s10_inter_mme_handover_proc->nas_s10_context;
  }else{
    nas_s10_ctx = &nas_ctx_req_proc->nas_s10_context;
  }

  /*
   * Set the identity values (IMSI) valid and present.
   * Assuming IMSI is always returned with S10 Context Response and the IMSI hastable registration method validates the received IMSI.
   */
  clear_imsi(&emm_context->_imsi);
  emm_ctx_set_valid_imsi(emm_context, &nas_s10_ctx->_imsi, nas_s10_ctx->imsi);
  emm_data_context_upsert_imsi(&_emm_data, emm_context); /**< Register the IMSI in the hash table. */

  /*
   * Update the security context & security vectors of the UE independent of TAU/Attach here (set fields valid/present).
   * Then inform the MME_APP that the context was successfully authenticated. Trigger a CSR.
   */
  emm_ctx_update_from_mm_eps_context(emm_context, nas_s10_ctx->mm_eps_ctx);
  OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - " "Successfully updated the EMM context with ueId " MME_UE_S1AP_ID_FMT " from the received MM_EPS_Context from the MME for UE with imsi: " IMSI_64_FMT ". \n",
      emm_ctx_p->ue_id, emm_ctx_p->_imsi64);

  /*
   * Update the ESM context (what was in esm_proc_default_eps_bearer_context).
   */
  emm_context->esm_ctx.n_active_ebrs += nas_s10_ctx->n_active_ebrs;
  emm_context->esm_ctx.n_pdns        += nas_s10_ctx->n_pdns;
  // todo: num_active_pdns not used.
  // todo: is_emergency not set
  // todo; rest of ESM context will be set by MME_APP

  /** Delete the CN procedure, if exists. */
  if(nas_ctx_req_proc)
    nas_delete_cn_procedure(emm_context, nas_ctx_req_proc);

  if (rc != RETURNok) { /**< This would delete the common procedure, but since none exist, we just make an TAU-Reject. */
    emm_sap_t                               emm_sap = {0};
    emm_sap.primitive = EMMREG_TAU_REJ;
    emm_sap.u.emm_reg.ue_id     = emm_context->ue_id;
    emm_sap.u.emm_reg.ctx       = emm_context;
    emm_sap.u.emm_reg.notify    = true;
    emm_sap.u.emm_reg.free_proc = true;
    emm_sap.u.emm_reg.u.tau.proc = tau_proc;
    rc = emm_sap_send (&emm_sap);
  }

  /** ESM context will not change, no ESM_PROC data will be created. */
  rc = nas_itti_pdn_config_req (emm_context->ue_id, emm_context->_imsi, NULL, 0);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_tracking_area_update_failure_context_res_cb (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  nas_emm_tau_proc_t                    *tau_proc = get_nas_specific_procedure_tau(emm_context);
  if (tau_proc) {
    // todo: requiremnt to continue with identification, auth and SMC if context res fails!
    rc = emm_proc_identification (emm_context, (nas_emm_proc_t *)tau_proc, IDENTITY_TYPE_2_IMSI, _emm_tracking_area_update_success_identification_cb, _emm_tracking_area_update_failure_identification_cb);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_tracking_area_update_success_identification_cb (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  nas_emm_tau_proc_t                    *tau_proc = get_nas_specific_procedure_tau(emm_context);

  if (tau_proc) {
    REQUIREMENT_3GPP_24_301(R10_5_5_3_2_3__2);
    rc = _emm_start_tracking_area_update_proc_authentication (emm_context, tau_proc);//, IDENTITY_TYPE_2_IMSI, _emm_attach_authentified, _emm_attach_release);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_tracking_area_update_failure_identification_cb (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  AssertFatal(0, "Cannot happen...\n");
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_start_tracking_area_update_proc_authentication (emm_data_context_t *emm_context, nas_emm_tau_proc_t* tau_proc)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  if ((emm_context) && (tau_proc)) {
    rc = emm_proc_authentication (emm_context, &tau_proc->emm_spec_proc, _emm_tracking_area_update_success_authentication_cb, _emm_tracking_Area_update_failure_authentication_cb);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_tracking_area_update_success_authentication_cb (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  /*
   * Only continue with SMC if a specific procedure is running.
   * todo: why don't we check parent procedure, only? In this way, it is evaluated as it is
   * part of the specific procedure, even if no parent procedure exists (common initiated after specific, and
   * then some specific occurs).
   */
  nas_emm_tau_proc_t                  *tau_proc = get_nas_specific_procedure_tau(emm_context);


  if (tau_proc) {
    REQUIREMENT_3GPP_24_301(R10_5_5_3_2_3__2);
    rc = _emm_start_tracking_area_update_proc_security (emm_context, tau_proc);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_tracking_area_update_failure_authentication_cb (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  nas_emm_tau_proc_t                     *tau_proc = get_nas_specific_procedure_tau(emm_context);

  if (tau_proc) {
    tau_proc->emm_cause = emm_context->emm_cause;

    // TODO could be in callback of attach procedure triggered by EMMREG_TAU_REJ
//    rc = _emm_tracking_area_update_reject(emm_context, &tau_proc->emm_spec_proc.emm_proc.base_proc);

    emm_sap_t emm_sap                      = {0};
    emm_sap.primitive                      = EMMREG_TAU_REJ;
    emm_sap.u.emm_reg.ue_id                = tau_proc->ue_id;
    emm_sap.u.emm_reg.ctx                  = emm_context;
    emm_sap.u.emm_reg.notify               = true;
    emm_sap.u.emm_reg.free_proc            = true;
    emm_sap.u.emm_reg.u.tau.proc = tau_proc;
    // don't care emm_sap.u.emm_reg.u.tau.is_emergency = false;
    rc = emm_sap_send (&emm_sap);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_start_tracking_area_update_proc_security (emm_data_context_t *emm_context, nas_emm_tau_proc_t* tau_proc)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  if ((emm_context) && (tau_proc)) {
    REQUIREMENT_3GPP_24_301(R10_5_5_3_2_3__2);

   /*
    * Create new NAS security context
    */
    emm_ctx_clear_security(emm_context);
    rc = emm_proc_security_mode_control (emm_context, &tau_proc->emm_spec_proc, tau_proc->ksi, _emm_tracking_area_update_success_security_cb, _emm_tracking_area_update_failure_security_cb);
    if (rc != RETURNok) {
      /*
       * Failed to initiate the security mode control procedure
       */
      OAILOG_WARNING (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT "EMM-PROC  - Failed to initiate security mode control procedure\n", ue_id);
      tau_proc->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      /*
       * Do not accept the UE to attach to the network
       */
      emm_sap_t emm_sap                      = {0};
      emm_sap.primitive                      = EMMREG_TAU_REJ;
      emm_sap.u.emm_reg.ue_id                = emm_context->ue_id;
      emm_sap.u.emm_reg.ctx                  = emm_context;
      emm_sap.u.emm_reg.notify               = true;
      emm_sap.u.emm_reg.free_proc            = true;
      emm_sap.u.emm_reg.u.tau.proc    = tau_proc;
      // dont care emm_sap.u.emm_reg.u.tau.is_emergency = false;
      rc = emm_sap_send (&emm_sap);
    }
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_tracking_area_update_success_security_cb (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  nas_emm_tau_proc_t                  *tau_proc = get_nas_specific_procedure_tau(emm_context);

  if (tau_proc) {
    rc = _emm_tracking_area_update(emm_context);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_tracking_area_update_failure_security_cb (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  nas_emm_tau_proc_t                     *tau_proc = get_nas_specific_procedure_tau(emm_context);

  if (tau_proc) {
    _emm_tracking_area_update_release(emm_context);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
void free_emm_tau_request_ies(emm_tau_request_ies_t ** const ies)
{
  if ((*ies)->additional_guti) {
    free_wrapper((void**)&((*ies)->additional_guti));
  }
  if ((*ies)->ue_network_capability) {
    free_wrapper((void**)&((*ies)->ue_network_capability));
  }
  if ((*ies)->last_visited_registered_tai) {
    free_wrapper((void**)&((*ies)->last_visited_registered_tai));
  }
  if ((*ies)->last_visited_registered_tai) {
    free_wrapper((void**)&((*ies)->last_visited_registered_tai));
  }
  if ((*ies)->drx_parameter) {
    free_wrapper((void**)&((*ies)->drx_parameter));
  }
  if ((*ies)->eps_bearer_context_status) {
    free_wrapper((void**)&((*ies)->eps_bearer_context_status));
  }
  if ((*ies)->ms_network_capability) {
    free_wrapper((void**)&((*ies)->ms_network_capability));
  }
  if ((*ies)->tmsi_status) {
    free_wrapper((void**)&((*ies)->tmsi_status));
  }
  if ((*ies)->mobile_station_classmark2) {
    free_wrapper((void**)&((*ies)->mobile_station_classmark2));
  }
  if ((*ies)->mobile_station_classmark3) {
    free_wrapper((void**)&((*ies)->mobile_station_classmark3));
  }
  if ((*ies)->supported_codecs) {
    free_wrapper((void**)&((*ies)->supported_codecs));
  }
  if ((*ies)->additional_updatetype) {
    free_wrapper((void**)&((*ies)->additional_updatetype));
  }
  if ((*ies)->old_guti_type) {
    free_wrapper((void**)&((*ies)->old_guti_type));
  }
  free_wrapper((void**)ies);
}

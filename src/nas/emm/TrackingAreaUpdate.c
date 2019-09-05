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

  Date        2018/05/07

  Product     NAS stack

  Subsystem   EPS Mobility Management

  Author      Dincer Beken

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
#include "3gpp_requirements_24.301.h"
#include "conversions.h"
#include "bstrlib.h"

#include "common_types.h"
#include "common_defs.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"

#include "mme_app_ue_context.h"
#include "mme_app_session_context.h"
#include "emm_data.h"
#include "emm_proc.h"
#include "emm_cause.h"
#include "emm_sap.h"
#include "esm_proc.h"
#include "nas_timer.h"
#include "common_defs.h"
#include "mme_app_defs.h"
#include "mme_config.h"
#include "mme_app_procedures.h"
#include "mme_app_wrr_selection.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
   Functions that may initiate EMM common procedures
*/
static int _emm_start_tracking_area_update_proc_authentication (emm_data_context_t *emm_context, nas_emm_tau_proc_t* tau_proc);
static int _emm_start_tracking_area_update_proc_security (emm_data_context_t *emm_context, nas_emm_tau_proc_t* tau_proc);

/* TODO Commented some function declarations below since these were called from the code that got removed from TAU request
 * handling function. Reason this code was removed: This portion of code was incomplete and was related to handling of
 * some optional IEs /scenarios that were not relevant for the TAU periodic update handling and might have resulted in 
 * unexpected behaviour/instability.
 * At present support for TAU is limited to handling of periodic TAU request only  mandatory IEs .
 * Other aspects of TAU are TODOs for future.
 */

static int _emm_tracking_area_update_reject( const mme_ue_s1ap_id_t ue_id, const int emm_cause);
static int _emm_tracking_area_update_accept (nas_emm_tau_proc_t * const tau_proc);
static int _emm_tracking_area_update_abort (struct emm_data_context_s *emm_context, struct nas_emm_base_proc_s * emm_base_proc);
static void _emm_tracking_area_update_t3450_handler (void *args);

static int _emm_tracking_area_update_success_identification_cb (emm_data_context_t *emm_context);
static int _emm_tracking_area_update_failure_identification_cb (emm_data_context_t *emm_context);
static int _emm_tracking_area_update_success_authentication_cb (emm_data_context_t *emm_context);
static int _emm_tracking_area_update_failure_authentication_cb (emm_data_context_t *emm_context);
static int _emm_tracking_area_update_success_security_cb (emm_data_context_t *emm_context);
static int _emm_tracking_area_update_failure_security_cb (emm_data_context_t *emm_context);

static int _context_req_proc_success_cb (emm_data_context_t *emm_context);
static int _context_req_proc_failure_cb (emm_data_context_t *emm_context);

static int _emm_tracking_area_update_run_procedure(emm_data_context_t *emm_context);
static void _emm_proc_create_procedure_tracking_area_update_request(emm_data_context_t * const ue_context, emm_tau_request_ies_t * const ies, retry_cb_t retry_cb);
static bool _emm_tracking_area_update_ies_have_changed (mme_ue_s1ap_id_t ue_id, emm_tau_request_ies_t * const tau_ies1, emm_tau_request_ies_t * const tau_ies2);

static int _emm_tracking_area_update_success_identification_cb (emm_data_context_t *emm_context);
static int _emm_tracking_area_update_failure_identification_cb (emm_data_context_t *emm_context);

static int _emm_tracking_area_update_accept_retx (emm_data_context_t * emm_context);

static int emm_proc_tracking_area_update_request_validity(emm_data_context_t * emm_context, mme_ue_s1ap_id_t new_ue_id, emm_tau_request_ies_t * const ies );

static void _emm_tracking_area_update_registration_complete(emm_data_context_t *emm_context);

static int _emm_tau_retry_procedure(mme_ue_s1ap_id_t ue_id);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

//------------------------------------------------------------------------------
int emm_proc_tracking_area_update_request (
  const mme_ue_s1ap_id_t ue_id,
  emm_tau_request_ies_t *ies,
  const int gea,
  const bool gprs_present,
  int *emm_cause,
  emm_data_context_t ** duplicate_emm_ue_ctx_pP)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
//  emm_data_context_t                      temp_emm_ue_ctx;
  ue_context_t                           *ue_context = NULL;
  emm_data_context_t                     *new_emm_ue_context = NULL;
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
  // todo: add a lock here before processing the ue contexts
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  TAU - For ueId " MME_UE_S1AP_ID_FMT " no UE context exists. \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }

  OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC-  Tracking Area Update request for new UeId " MME_UE_S1AP_ID_FMT ". TAU_Type=%d, active_flag=%d)\n", ue_id,
      ies->eps_update_type.eps_update_type_value, ies->eps_update_type.active_flag);
  /*
   * Try to retrieve the correct EMM context.
   */
  (*duplicate_emm_ue_ctx_pP) = emm_data_context_get(&_emm_data, ue_id);
  if(!(*duplicate_emm_ue_ctx_pP)){
    /*
     * Get it via GUTI (S-TMSI not set, getting via GUTI).
     * GUTI will always be there. Checking for validity of the GUTI via the validity of the TMSI. */
    if((INVALID_M_TMSI != ies->old_guti.m_tmsi)){
      if(((*duplicate_emm_ue_ctx_pP) = emm_data_context_get_by_guti (&_emm_data, &ies->old_guti)) != NULL){ /**< May be set if S-TMSI is set. */
        OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC-  Found a valid UE with correct GUTI " GUTI_FMT " and (old) ue_id " MME_UE_S1AP_ID_FMT ". "
            "Continuing with the Tracking Area Update Request. \n", GUTI_ARG(&(*duplicate_emm_ue_ctx_pP)->_guti), (*duplicate_emm_ue_ctx_pP)->ue_id);
      }
    }
    OAILOG_DEBUG(LOG_NAS_EMM, "CHECKING DUPLICATE CONTEXT BY IMSI \n");
    if(!(*duplicate_emm_ue_ctx_pP)){
      /** Check if there is an ongoing handover procedure and if the IMSI already exists. */
      if(!ies->is_initial){
        mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
        if(s10_handover_proc){
          DevAssert(s10_handover_proc->nas_s10_context.mm_eps_ctx);
          OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - We have receive the TAU as part of an S10 procedure for ue_id = " MME_UE_S1AP_ID_FMT ". "
              "Checking for duplicate EMM contexts for the IMSI "IMSI_64_FMT ". \n", ue_id, s10_handover_proc->nas_s10_context.imsi);
          if(((*duplicate_emm_ue_ctx_pP) = emm_data_context_get_by_imsi(&_emm_data, s10_handover_proc->nas_s10_context.imsi)) != NULL){ /**< May be set if S-TMSI is set. */
            OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC-  Found a valid UE with IMSI " IMSI_64_FMT " and (old) ue_id " MME_UE_S1AP_ID_FMT ". "
                "Continuing with the Tracking Area Update Request. \n", s10_handover_proc->nas_s10_context.imsi, (*duplicate_emm_ue_ctx_pP)->ue_id);
          }
        }
      }
    }
  }else{
    OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC-  Found a valid UE with new mmeUeS1apId " MME_UE_S1AP_ID_FMT " and GUTI " GUTI_FMT ". "
        "Continuing with the Tracking Area Update Request. \n", ue_id, GUTI_ARG(&(*duplicate_emm_ue_ctx_pP)->_guti));
  }
  /** No IMSI as IE @ TAU-Req. Check the validity of the EMM context and the request. */
  if ((*duplicate_emm_ue_ctx_pP)) {  /**< We found a matching EMM context via IMSI or S-TMSI. */
    /** Check the validity of the existing EMM context It may or may not have another MME_APP UE context. */
    mme_ue_s1ap_id_t old_mme_ue_id = (*duplicate_emm_ue_ctx_pP)->ue_id;
    rc = emm_proc_tracking_area_update_request_validity((*duplicate_emm_ue_ctx_pP), ue_id, ies);
    (*duplicate_emm_ue_ctx_pP) = emm_data_context_get(&_emm_data, old_mme_ue_id);
    /** Check if the old emm_ctx is still existing. */
    if(rc != RETURNok){
      /** Not continuing with the Attach-Request (it might be rejected, a previous attach accept might have been resent or just ignored). */
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Not continuing with the Tracking-Area-Update-Request. \n"); /**< All rejects should be sent and everything should be handled by now. */
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    }
  }
  /** After validation. Continuing with an existing or new to be created EMM context. */
  OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - Continuing to handle the new Tracking-Area-Update-Request. \n");

  /** Before creating a new EMM context (may be copied from an old one. */

  if(!(*duplicate_emm_ue_ctx_pP)) {
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - No old EMM context was found for UE_ID " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    /*
     * Create UE's EMM context todo: THIS IS NOT OK --> WE MIGHT NOT BE ABLE TO CREATE NEW CONTEXT!
     */
    new_emm_ue_context= (emm_data_context_t *) calloc (1, sizeof (emm_data_context_t));
    if (!new_emm_ue_context) {
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to create EMM context for ueId " MME_UE_S1AP_ID_FMT ". \n", ue_id);
      /*
       * Do not accept the UE to attach to the network
       */
      rc = _emm_tracking_area_update_reject(ue_id, EMM_CAUSE_ILLEGAL_UE);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc); /**< Return with an error. */
    }
    /** Initialize the EMM context only in this case, else we have a DEREGISTERED but valid EMM context. */
    /** Check if the EMM context has been initialized, if so set the values. */
    new_emm_ue_context->ue_id = ue_id;
    new_emm_ue_context->is_dynamic = true;
    OAILOG_NOTICE (LOG_NAS_EMM, "EMM-PROC  - Create EMM context ue_id = " MME_UE_S1AP_ID_FMT "\n", ue_id);
//    emm_context->attach_type = ies->type;
//    emm_context->additional_update_type = ies->additional_update_type;
    new_emm_ue_context->emm_cause       = EMM_CAUSE_SUCCESS;
    emm_init_context(new_emm_ue_context);  /**< Initialize the context, we might do it again if the security was not verified. No IMSI, GUTI set so adding should work. */
    DevAssert(RETURNok == emm_data_context_add (&_emm_data, new_emm_ue_context));
  } else if((*duplicate_emm_ue_ctx_pP)->emm_cause != EMM_CAUSE_SUCCESS) {
	  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - No valid old EMM context was found for UE_ID " MME_UE_S1AP_ID_FMT ". \n", ue_id);
	  /** If the UE-IDs match, we cannot continue with the current TAU. */
	  if(ue_id == (*duplicate_emm_ue_ctx_pP)->ue_id){
		OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - The old failed EMM context has same ueId " MME_UE_S1AP_ID_FMT ". Rejecting TAU (should also implicitly detach). \n", ue_id);
		rc = _emm_tracking_area_update_reject(ue_id, EMM_CAUSE_IMPLICITLY_DETACHED);
		(*duplicate_emm_ue_ctx_pP) = emm_data_context_get(&_emm_data, ue_id); /**< Refresh reference. */
		OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
	  }
	  /*
	   * Create UE's EMM context (ue_id differs).
	   */
	  OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - The old failed EMM context has a different ueId " MME_UE_S1AP_ID_FMT " than the new one " MME_UE_S1AP_ID_FMT ". Continuing with TAU. \n", (*duplicate_emm_ue_ctx_pP)->ue_id, ue_id);
	  new_emm_ue_context= (emm_data_context_t *) calloc (1, sizeof (emm_data_context_t));
	  if (!new_emm_ue_context) {
		OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to create EMM context for ueId " MME_UE_S1AP_ID_FMT ". \n", ue_id);
		/*
		 * Do not accept the UE to attach to the network.
		 * Also implicitly detach the old duplicate EMM context.
		 */
		rc = _emm_tracking_area_update_reject(ue_id, EMM_CAUSE_IMPLICITLY_DETACHED);
		(*duplicate_emm_ue_ctx_pP) = emm_data_context_get(&_emm_data, ue_id); /**< Refresh reference. */
		OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc); /**< Return with an error. */
	  }
	  /** Initialize the EMM context only in this case, else we have a DEREGISTERED but valid EMM context. */
	  /** Check if the EMM context has been initialized, if so set the values. */
	  // todo: num_attach_request?!
	  new_emm_ue_context->ue_id = ue_id;
	  new_emm_ue_context->is_dynamic = true;
	  OAILOG_NOTICE (LOG_NAS_EMM, "EMM-PROC  - Create EMM context ue_id = " MME_UE_S1AP_ID_FMT "\n", ue_id);
	  //    emm_context->attach_type = ies->type;
	  //    emm_context->additional_update_type = ies->additional_update_type;
	  /**
	   * BY NO MEANS WE CAN ADD HERE AN EMM CONTEXT WITH SAME UE_ID!!!!! OVERWRITING INEVITABLE AND CAUSES LEAKS!!!
	   */
	  new_emm_ue_context->emm_cause       = EMM_CAUSE_SUCCESS;
	  /**
	   * Initialize the context, we might do it again if the security was not verified.
	   * IMSI and GUTI are cleared, so we should have no problems with adding the EMM context.
	   * The old duplicate context should still be there and intact. Will be removed outside this function and new emm_context will continue @ retry.
	   */
	  emm_init_context(new_emm_ue_context);
	  DevAssert(RETURNok == emm_data_context_add (&_emm_data, new_emm_ue_context));
  } else {
    OAILOG_WARNING (LOG_NAS_EMM, "We have a valid EMM context with IMSI " IMSI_64_FMT " from a previous registration with ueId " MME_UE_S1AP_ID_FMT ". \n",
        (*duplicate_emm_ue_ctx_pP)->_imsi64, (*duplicate_emm_ue_ctx_pP)->ue_id);
    /*
     * We have an EMM context with GUTI from a previous registration.
     * Validate it, if the message has been received with a security header and if the security header can be validated we can continue using it.
     */
     if((*duplicate_emm_ue_ctx_pP)->ue_id != ue_id) {
    	 new_emm_ue_context = calloc(1, sizeof(emm_data_context_t));
    	 memcpy(new_emm_ue_context, (*duplicate_emm_ue_ctx_pP), sizeof(**duplicate_emm_ue_ctx_pP));
    	 new_emm_ue_context->_emm_fsm_state = EMM_DEREGISTERED;

    	mme_ue_s1ap_id_t old_ue_id = (*duplicate_emm_ue_ctx_pP)->ue_id;
        /** Remove the (old) EMM context from the tables, update the EMM context ue_id. */
        new_emm_ue_context->ue_id = ue_id;
        /*
         * The old UE Id can not already be in REGISTERED state.
         * If there are some differences of the current structure to the received message, the context will be
         * implicitly detached (together with all its layers).
         * Else, if we can use it, we can also continue with the newly created UE references and don't
         * have to re-link any of them to the old UE context.
         */
    	OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - Updated the EMM Context with IMSI " IMSI_64_FMT" from old ueId " MME_UE_S1AP_ID_FMT " to new ueId " MME_UE_S1AP_ID_FMT ". \n",
    			new_emm_ue_context->_imsi64, old_ue_id, ue_id);
    	/** Remove the subscription profile and add it into the TAU procedure. */
        subscription_data_t * subscription_data = mme_api_remove_subscription_data(new_emm_ue_context->_imsi64);
        if(subscription_data){
            if(ies->subscription_data)
            	free_wrapper((void**)&(ies->subscription_data));
            ies->subscription_data = subscription_data;
        }
    	emm_sap_t                               emm_sap = {0};
    	emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE; /**< UE context will be purged. */
    	emm_sap.u.emm_cn.u.emm_cn_implicit_detach.emm_cause   = 0; /**< Not sending detach type. */
    	emm_sap.u.emm_cn.u.emm_cn_implicit_detach.detach_type = 0; /**< Not sending detach type. */
    	emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = old_ue_id;
    	/*
    	 * Don't send the detach type, such that no NAS Detach Request is sent to the UE.
    	 * Depending on the cause, the MME_APP will check and inform the NAS layer to continue with the procedure, before the timer expires.
    	 */
    	emm_sap_send (&emm_sap);

        /** Set the new attach procedure into pending mode and continue with it after the completion of the duplicate removal. */
        void *unused = NULL;
        _emm_proc_create_procedure_tracking_area_update_request(new_emm_ue_context, ies, _emm_tau_retry_procedure);
        nas_emm_tau_proc_t * tau_proc = get_nas_specific_procedure_tau(new_emm_ue_context);
        nas_stop_T_retry_specific_procedure(new_emm_ue_context->ue_id, &tau_proc->emm_spec_proc.retry_timer, unused);
        nas_start_T_retry_specific_procedure(new_emm_ue_context->ue_id, &tau_proc->emm_spec_proc.retry_timer, tau_proc->emm_spec_proc.retry_cb, (void*)new_emm_ue_context->ue_id);
        /** Set the old mme_ue_s1ap id which will be checked. */
        tau_proc->emm_spec_proc.old_ue_id = old_ue_id;
        *duplicate_emm_ue_ctx_pP = NULL;
        /*
         * Nothing else to do with the current TAU procedure. Leave it on hold.
         * Continue to handle the new TAU procedure.
         */
        /*
         * Perform implicit detach on the old UE with cause of duplicate detection.
         * Not continuing with the new TAU procedure until the old one has been successfully deallocated.
         */
        // todo: this probably will not work, we need to update the enb_ue_s1ap_id of the old UE context to continue to work with it!
        //              unlock_ue_contexts(ue_context);
        //             unlock_ue_contexts(imsi_ue_mm_ctx);
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
    } else{
    	 new_emm_ue_context = (*duplicate_emm_ue_ctx_pP);
    	 /** Unlink the new procedure. */
    	 (*duplicate_emm_ue_ctx_pP) = NULL;
    }
  }
  /*
   * Continue with the new or existing EMM context.
   * Unlink the NAS message.
   *
   * todo: EMM CONTEXT MAY BE NULL!
   */
  DevAssert(new_emm_ue_context);
  DevAssert(new_emm_ue_context->emm_cause == EMM_CAUSE_SUCCESS);
  _emm_proc_create_procedure_tracking_area_update_request(new_emm_ue_context, ies, _emm_tau_retry_procedure); /**< Created TAU procedure might be floating (not registered to any list yet). */
  if((*duplicate_emm_ue_ctx_pP)){
    /** Set the new tau procedure into pending mode and continue with it after the completion of the duplicate removal. */
    void *unused = NULL;
    nas_emm_tau_proc_t * tau_proc = get_nas_specific_procedure_tau(new_emm_ue_context);
    nas_stop_T_retry_specific_procedure(new_emm_ue_context->ue_id, &tau_proc->emm_spec_proc.retry_timer, unused);
    nas_start_T_retry_specific_procedure(new_emm_ue_context->ue_id, &tau_proc->emm_spec_proc.retry_timer, tau_proc->emm_spec_proc.retry_cb, (void*)new_emm_ue_context->ue_id);
    /** Set the old mme_ue_s1ap id which will be checked. */
    tau_proc->emm_spec_proc.old_ue_id =(*duplicate_emm_ue_ctx_pP)->ue_id;
    /*
     * Nothing else to do with the current TAU procedure. Leave it on hold.
     * Continue to handle the new TAU procedure.
     */
    /*
     * Perform implicit detach on the old UE with cause of duplicate detection.
     * Not continuing with the new TAU procedure until the old one has been successfully deallocated.
     */
    // todo: this probably will not work, we need to update the enb_ue_s1ap_id of the old UE context to continue to work with it!
    //              unlock_ue_contexts(ue_context);
    //             unlock_ue_contexts(imsi_ue_mm_ctx);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);

  }else{
    rc = _emm_tracking_area_update_run_procedure (new_emm_ue_context);
    if (rc != RETURNok) {
      OAILOG_WARNING (LOG_NAS_EMM, "Failed to initialize EMM callback functions");
      new_emm_ue_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      rc = _emm_tracking_area_update_reject(new_emm_ue_context->ue_id, new_emm_ue_context->emm_cause);
      //    free_emm_tau_request_ies(&ies);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:        emm_wrapper_tracking_area_update_reject()                    **
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
int emm_wrapper_tracking_area_update_reject(
  const mme_ue_s1ap_id_t ue_id,
  const emm_cause_t emm_cause)
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
  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - EPS TAU complete (ue_id=" MME_UE_S1AP_ID_FMT ")\n", ue_id);

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
      // todo: REQUIREMENT_3GPP_24_301(R10_5_5_3_2_4__23);
      emm_ctx_set_attribute_valid(emm_context, EMM_CTXT_MEMBER_GUTI);
      // TODO LG REMOVE emm_context_add_guti(&_emm_data, &ue_context->emm_context);
      emm_ctx_clear_old_guti(emm_context);
      emm_data_context_add_guti(&_emm_data, emm_context);
      // TODO LG REMOVE emm_context_add_guti(&_emm_data, &ue_context->emm_context);
      emm_ctx_clear_old_guti(emm_context);

      /*
       * Upon receiving an ATTACH COMPLETE message, the MME shall stop timer T3450
       */
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3450 (%d)\n", tau_proc->T3450.id);
//      tau_proc->T3450.id = nas_timer_stop (tau_proc->T3450.id);
      void * unused = NULL;
      nas_stop_T3450(emm_context->ue_id, &tau_proc->T3450, unused);

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
  emm_sap.u.emm_reg.notify = true;
  emm_sap.u.emm_reg.free_proc = true;
  emm_sap.u.emm_reg.u.tau.proc = tau_proc;
  rc = emm_sap_send (&emm_sap);

  /*
   * Check if the UE is in registered state.
   */
  if(emm_context && emm_context->_emm_fsm_state != EMM_REGISTERED){
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - EMM Context for ueId " MME_UE_S1AP_ID_FMT " is still not in EMM_REGISTERED state although TAU_CNF has arrived. "
        "Removing failed EMM context implicitly.. \n", ue_id);
    emm_sap_t                               emm_sap = {0};
    emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;
    emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = ue_id;
    emm_sap_send (&emm_sap);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
 }

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/
/**
 * Returns SUCCESS --> if the Attach-Request could be validated/not rejected/no retransmission of a previous attach-accept. In that we continue with the handling of the Attach_Request.
 * Return  ERROR   --> ignore the request (might be rejected or the previous attach accept might be retransmitted).
 *
 * No matter on what is returned, we might or might not also implicitly detach the EMM context.
 */
static
int emm_proc_tracking_area_update_request_validity(emm_data_context_t * emm_context, mme_ue_s1ap_id_t new_ue_id, emm_tau_request_ies_t * const ies ){

  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int                      rc = RETURNerror;
//  emm_data_context_t      *emm_context = *emm_context_pP;

  if(emm_context->ue_id != new_ue_id){
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The UE_ID EMM context from IMSI " IMSI_64_FMT " with old mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " it not equal to new ueId" MME_UE_S1AP_ID_FMT". \n",
        emm_context->_imsi64, emm_context->ue_id, new_ue_id);
  }
  emm_fsm_state_t fsm_state = emm_fsm_get_state (emm_context);

  nas_emm_attach_proc_t * attach_procedure = get_nas_specific_procedure_attach (emm_context);
  nas_emm_tau_proc_t * tau_procedure = get_nas_specific_procedure_tau(emm_context);

  // todo: may these parameters change?
  /*
   * Check if there is an ongoing SMC procedure.
   * In any case, we just abort the current SMC procedure, which will keep the context, state, other commons and bearers, etc..
   * We continue with the specific procedure. If the SMC was part of the specific procedure, we don't care here. Thats the collision between the specific procedures.
   * No matter what the state is and if the received message differs or not, remove the EMM_CTX (incl. security), bearers, all other common procedures and abort any running
   * specific procedures.
   */
  if (is_nas_common_procedure_smc_running(emm_context)) {
    REQUIREMENT_3GPP_24_301(R10_5_4_3_7_c);
    nas_emm_smc_proc_t * smc_proc = get_nas_common_procedure_smc(emm_context);
    emm_sap_t                               emm_sap = {0};
    emm_sap.primitive = EMMREG_COMMON_PROC_ABORT;    /**< May cause an implicit detach, which may purge the EMM context. */
    emm_sap.u.emm_reg.ue_id     = emm_context->ue_id;
    emm_sap.u.emm_reg.notify    = false;   /**< Because SMC itself did not fail, some other event (some external event like a collision) occurred. Aborting the common procedure. Not high severity of error. */
    emm_sap.u.emm_reg.free_proc = true;  /**< We will not restart the procedure again. */
    emm_sap.u.emm_reg.u.common.common_proc = &smc_proc->emm_com_proc;
    emm_sap.u.emm_reg.u.common.previous_emm_fsm_state = smc_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_PROC_ABORT (SMC) ueId" MME_UE_S1AP_ID_FMT " ", emm_context->mme_ue_s1ap_id);
    rc = emm_sap_send (&emm_sap);
    /** Continue with the specific procedure. May or may not implicitly detach the EMM context. */
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
    /** We don't need to check the EMM context further. It is deallocated, but continue with processing the TAU Request., return. */ // todo: check that the emm_ctx is nulled. (else, need to do it here).
  }
  /*
   * Check if there is an Authentication Procedure running.
   * No collisions are mentioned, but just abort the current authentication procedure, if it was triggered by MME/OAM interface.
   * Consider last UE/EPS_Security context to be valid, keep the state and everything.
   * If it was part of a specific procedure, we will deal with it too, below, anyway.
   *
   * We don't need to check if an EMM context exists or not. It must exist if SMC was not aborted.
   */
  if (is_nas_common_procedure_authentication_running(emm_context)) {
    if (attach_procedure || tau_procedure){
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The EMM context from IMSI " IMSI_64_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " has an an authentication procedure as part of a specific procedure. "
          "Common identification procedure will be aborted (as part of the possibly aborted specific procedure below, if the specific procedure is to be aborted. \n", emm_context->_imsi64, emm_context->ue_id);
    } else{
      /** Not defined explicitly in the specifications, but we will abort the common procedure. */
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The EMM context from IMSI " IMSI_64_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " has an authentication procedure running but no specific procedure. "
          "Aborting the authentication procedure run currently. \n", emm_context->_imsi64, emm_context->ue_id);
      nas_emm_auth_proc_t * auth_proc = get_nas_common_procedure_authentication(emm_context);
      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMREG_COMMON_PROC_ABORT;
      emm_sap.u.emm_reg.ue_id = emm_context->ue_id; /**< This should be enough to get the EMM context. */
      emm_sap.u.emm_reg.notify    = false;  /**< Again, the authentification procedure itself not considered as failed but aborted from outside. */
      emm_sap.u.emm_reg.free_proc = true;
      emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
      emm_sap.u.emm_reg.u.common.previous_emm_fsm_state = auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
      // TODOdata->notify_failure = true;
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_PROC_ABORT (AUTH) ue id " MME_UE_S1AP_ID_FMT " ", emm_context->mme_ue_s1ap_id);
      rc = emm_sap_send (&emm_sap);
      /** Keep all the states, context, bearers and other common and specific procedures, just stopping the timer and deallocating the common procedure. */
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    }
  }
  /*
   * We don't need to check the identification procedure. If it is not part of an tau procedure (no specific tau procedure is running - assuming all common procedures
   * belong to the specific procedure), we let it run. It may independently trigger a timeout with consequences.
   *
   * If there is a specific attach procedure running, see below. All running common procedures will assumed to be part of the specific procedure and will be aborted together
   * with the specific procedure.
   */
  if (is_nas_common_procedure_identification_running(emm_context)) {
    /*
     * We just check if a collision with the same specific procedure is occurring for log reasons.
     * If a collision with a running specific procedure occurs below, all common procedures will be assumed to be part of the specific procedure and aborted accordingly, anyway.
     */
    if (attach_procedure || tau_procedure){
      REQUIREMENT_3GPP_24_301(R10_5_4_4_6_d__2); /**< Will be aborted eventually below. */
      REQUIREMENT_3GPP_24_301(R10_5_4_4_6_d__1);
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The EMM context from IMSI " IMSI_64_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " has an an identification procedure as part of a specific procedure. "
          "Common identification procedure will be aborted (as part of the possibly aborted specific procedure below, if the specific procedure is to be aborted. \n", emm_context->_imsi64, emm_context->ue_id);
    }else{
      REQUIREMENT_3GPP_24_301(R10_5_4_4_6_c); // continue
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - The EMM context from IMSI " IMSI_64_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " has an identification procedure running but no specific procedure. "
          "Letting the procedure run currently. \n", emm_context->_imsi64, emm_context->ue_id);
      /**
       * Specification does not say, that we need to abort the identification procedure.
       * This may cause that a missing response detaches the UE after this attach complete successfully!
       * Unlike SMC, no abort.
       */
    }
  }
  /** Again, also in the new version, we can make a clean cut. */
//  /*
//   * UE may or may not be in COMMON-PROCEDURE-INITIATED state (common procedure active or implicit GUTI reallocation), DEREGISTERED or already REGISTERED.
//   * Further check on collision with other specific procedures.
//   * Checking if EMM_REGISTERED or already has been registered.
//   * Already been registered only makes sense if  COMMON procedures are activated.
//   */
//  if ((EMM_REGISTERED == fsm_state) || emm_context->is_has_been_attached) {
//    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_f);
//    OAILOG_TRACE (LOG_NAS_EMM, "EMM-PROC  - the new ATTACH REQUEST is progressed and the current registered EMM context, "
//        "together with the states, common procedures and bearers will be destroyed. \n");
//    DevAssert(!get_nas_specific_procedure_attach (emm_context));
//    /*
//     * Perform an implicit detach on the current context.
//     * This will check if there are any common procedures are running, and destroy them, too.
//     * We don't need to check if a TAU procedure is running, too. It will be destroyed in the implicit detach.
//     * It will destroy any EMM context. EMM_CTX wil be nulled (not EMM_DEREGISTERED).
//     */
//    // Trigger clean up
//    emm_sap_t                               emm_sap = {0};
//    emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE; /**< UE context will be purged. */
//    emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = emm_context->ue_id;
//    /** Don't send the detach type, such that no NAS Detach Request is sent to the UE. */
//    rc = emm_sap_send (&emm_sap);
//    /** Additionally, after performing a NAS implicit detach, reset all the EMM context (the UE is in DEREGISTERED state, we reset it to invalid state. */
//    // todo: cleaning up!             clear_emm_ctxt = true;
//    OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc); /**< Return with the deallocated (NULL) EMM context and continue to process the Attach Request. */
//  }
  /*
   * Check collisions with specific procedures.
   * Specific attach and tau procedures run until attach/tau_complete!
   * No attach completed yet.
   */
  if (attach_procedure){
    if(is_nas_attach_accept_sent(attach_procedure)) {// && (!emm_ctx->is_attach_complete_received): implicit
    	REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_g);
    	OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - TAU request received for IMSI " IMSI_64_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " while an Attach Procedure is running (Attach Complete pending). \n", emm_context->_imsi64, emm_context->ue_id);
      /** GUTI must be validated. */
      emm_ctx_set_attribute_valid(emm_context, EMM_CTXT_MEMBER_GUTI);
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
      //        emm_sap.u.emm_reg.notify= true;
      emm_sap.u.emm_reg.free_proc = true; /**< Will remove the T3450 timer. */
      emm_sap.u.emm_reg.u.attach.proc   = attach_procedure;
      rc = emm_sap_send (&emm_sap);

      emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;

      /** Send a TAU-Reject back, removing the new MME_APP UE context, too. */
      rc = emm_wrapper_tracking_area_update_reject(new_ue_id, EMM_CAUSE_IMPLICITLY_DETACHED); /**< Will remove the contexts for the TAU-Req. */
      free_emm_tau_request_ies(&ies);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }else{
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - TAU request received for IMSI " IMSI_64_FMT " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " while an Attach Procedure is running (Attach Accept/Reject not sent yet). Discarding TAU Request. \n",
          emm_context->_imsi64, emm_context->ue_id);
      /** TAU before ATTACH_ACCEPT sent should be rejected directly. */
//      rc = emm_wrapper_tracking_area_update_reject(attach_procedure->ue_id, EMM_CAUSE_MSC_NOT_REACHABLE); /**< Will remove the contexts for the TAU-Req. */
      if(new_ue_id != emm_context->ue_id)
        nas_itti_esm_detach_ind(new_ue_id, false);
      free_emm_tau_request_ies(&ies);
      /** Not touching the ATTACH procedure. */
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }
  }
  /** Check for collisions with TAU procedure. */
  // todo: either tau or attach should exist, so use here else_if instead of if. */
  if (tau_procedure){
    if(is_nas_tau_accept_sent(tau_procedure) /* || is_nas_tau_reject_sent(tau_procedure) */) {// && (!){
      if (_emm_tracking_area_update_ies_have_changed (emm_context->ue_id, tau_procedure->ies, ies)) {
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - TAU Request parameters have changed\n");
//        REQUIREMENT_3GPP_24_301(R10_5_5_3_2_7_d__1);
        /*
         * If one or more of the information elements in the TRACKING AREA UPDATE REQUEST message differ
         * from the ones received within the previous TRACKING AREA UPDATE REQUEST message, the
         * previously initiated tracking area updating procedure shall be aborted if the TRACKING AREA UPDATE COMPLETE message has not
         * been received and the new tracking area updating procedure shall be progressed;
         */
        emm_sap_t                               emm_sap = {0};
        emm_sap.primitive = EMMREG_TAU_ABORT;
        emm_sap.u.emm_reg.ue_id = tau_procedure->ue_id;
        emm_sap.u.emm_reg.notify= true;
        emm_sap.u.emm_reg.free_proc = true;
        emm_sap.u.emm_reg.u.tau.proc   = tau_procedure;
        rc = emm_sap_send (&emm_sap);
        // trigger clean up
        /** Set the EMM cause to invalid. We process the new request with new parameters --> create a new EMM context. */
        emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
        /** Continue with a clean emm_ctx. */
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
      } else {
//        REQUIREMENT_3GPP_24_301(R10_5_5_3_2_7_d__2);
        /*
         * - if the information elements do not differ, then the TRACKING AREA UPDATE ACCEPT message shall be resent and the timer
         * T3450 shall be restarted if an ATTACH COMPLETE message is expected. In that case, the retransmission
         * counter related to T3450 is not incremented.
         */
        _emm_tracking_area_update_accept_retx(emm_context); /**< Resend the TAU_ACCEPT with the old EMM context (with old UE IDs). */
        // Clean up new UE context that was created to handle new tau request
        if(new_ue_id != emm_context->ue_id)
          nas_itti_esm_detach_ind(new_ue_id, false);
        free_emm_tau_request_ies(&ies);

        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Received duplicated TAU Request\n");
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
      }
    } else if (!is_nas_tau_accept_sent(tau_procedure) && (!is_nas_tau_reject_sent(tau_procedure))) {  /**< TAU accept or reject not sent yet. */
//       REQUIREMENT_3GPP_24_301(R10_5_5_3_2_7_e__1);
       if (_emm_tracking_area_update_ies_have_changed (emm_context->ue_id, tau_procedure->ies, ies)) {
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
         emm_sap.u.emm_reg.notify= false;
         emm_sap.u.emm_reg.free_proc = true;
         emm_sap.u.emm_reg.u.tau.proc   = tau_procedure;
         rc = emm_sap_send (&emm_sap);
         // trigger clean up
         /** Set the EMM cause to invalid. */
         emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
         OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
       } else {
//         REQUIREMENT_3GPP_24_301(R10_5_5_3_2_7_e__2);
         /*
          * If the information elements do not differ, then the network shall continue with the previous tau procedure
          * and shall ignore the second TAU REQUEST message.
          */
         // Clean up new UE context that was created to handle new attach request
         if(new_ue_id != emm_context->ue_id)
           nas_itti_esm_detach_ind(new_ue_id, false);
         free_emm_tau_request_ies(&ies);

         OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Received duplicated TAU Request\n");
         OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
       }
     }
  }
  /** If the TAU request was received in EMM_DEREGISTERED or EMM_COMMON_PROCEDURE_INITIATED state, we should remove the context implicitly. */
  if (!(EMM_REGISTERED == fsm_state || EMM_COMMON_PROCEDURE_INITIATED == fsm_state)){
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Received TAU Request while state is not in EMM_REGISTERED/EMM_COMMON_PROCEDURE_INITIATED, instead %d. \n.", fsm_state);
    /** Set the EMM cause to invalid. */
    emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    /** Continue to handle the TAU Request inside a new EMM context. */
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
  }
  /** Check that the UE has a valid security context and that the message could be successfully decoded. */

  if (!(IS_EMM_CTXT_PRESENT_SECURITY(emm_context)
      && ies->decode_status.integrity_protected_message
      && ies->decode_status.mac_matched)){

	  nas_message_t                           nas_msg = {.security_protected.header = {0},
	                                                       .security_protected.plain.emm.header = {0},
	                                                       .security_protected.plain.esm.header = {0}};
	  memset (&nas_msg,       0, sizeof (nas_msg));
	  /*
	   * Decode the received message
	   */
	  int                                     decoder_rc = 0;
	  decoder_rc = nas_message_decode (ies->complete_tau_request->data, &nas_msg, blength(ies->complete_tau_request), &emm_context->_security, NULL, &ies->decode_status);

	  if(decoder_rc < 0){
		  OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Received TAU Request while UE context does not have a valid security context (could not decode, rc=%d). Performing an implicit detach. \n", decoder_rc);
		  /** Set the EMM cause to invalid. */
		  emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
		  /** Continue to handle the TAU Request inside a new EMM context. */
		  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
	  }
	  if(!ies->decode_status.mac_matched){
		  OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Received TAU Request while UE context does not have a valid security context (mac not matched). Performing an implicit detach. \n");
		  /** Set the EMM cause to invalid. */
		  emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
		  /** Continue to handle the TAU Request inside a new EMM context. */
		  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
	  }
  }
  /** Return Ok: Can process with the TAU request (and EMM context is validated). */
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

static void _emm_proc_create_procedure_tracking_area_update_request(emm_data_context_t * const emm_context, emm_tau_request_ies_t * const ies, retry_cb_t retry_cb)
{
  nas_emm_tau_proc_t *tau_proc = nas_new_tau_procedure(emm_context);
  AssertFatal(tau_proc, "TODO Handle this");
  if (tau_proc) {
    tau_proc->ies = ies;
    tau_proc->ue_id = emm_context->ue_id;
    ((nas_emm_base_proc_t*)tau_proc)->abort = _emm_tracking_area_update_abort;
    ((nas_emm_base_proc_t*)tau_proc)->fail_in = NULL; // No parent procedure
    ((nas_emm_base_proc_t*)tau_proc)->time_out = _emm_tracking_area_update_t3450_handler;
    /** Set the MME_APP registration complete procedure for callback. */
    ((nas_emm_base_proc_t*)tau_proc)->success_notif = _emm_tracking_area_update_registration_complete;
    ((nas_emm_specific_proc_t*)tau_proc)->retry_cb= retry_cb;
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
  mme_ue_s1ap_id_t                          ue_id = (mme_ue_s1ap_id_t) (args);

  emm_data_context_t * emm_context = emm_data_context_get(&_emm_data, ue_id);
  if (!(emm_context)) {
    OAILOG_ERROR (LOG_NAS_EMM, "T3450 timer expired No EMM context for UE " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    nas_itti_esm_detach_ind(ue_id, false);
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
      emm_sap.u.emm_reg.notify    = true;
      emm_sap.u.emm_reg.free_proc = true;
      emm_sap.u.emm_reg.u.attach.is_emergency = false;
      emm_sap_send (&emm_sap);

      /** todo: Should definitely trigger a detach. */
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
  emm_context = emm_data_context_get(&_emm_data, ue_id);
  if (emm_context) {
    if(emm_context->is_dynamic) {
      _clear_emm_ctxt(emm_context->ue_id);
    }
  }

//  unlock_ue_contexts(ue_context);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
int emm_wrapper_tracking_area_update_accept (mme_ue_s1ap_id_t ue_id, eps_bearer_context_status_t ebr_status)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);

  emm_data_context_t 				     *emm_context = NULL;
  nas_emm_tau_proc_t                     *tau_proc    = NULL;

  emm_context = emm_data_context_get(&_emm_data, ue_id);
  if(!emm_context) {
	  OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - No EMM context for ue_id " MME_UE_S1AP_ID_FMT " exists. Cannot proceed with TAU.", ue_id);
	  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }
  tau_proc = get_nas_specific_procedure_tau(emm_context);
  if(!tau_proc) {
  	  OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - No TAU procedure for ue_id " MME_UE_S1AP_ID_FMT " exists. Cannot proceed with TAU. Ignoring the message.", ue_id);
  	  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }
  /** Update the EPS bearer contexts in the TAU procedure. */
  if(!tau_proc->ies->eps_bearer_context_status)
	  tau_proc->ies->eps_bearer_context_status = calloc(1, sizeof(eps_bearer_context_status_t));
  *tau_proc->ies->eps_bearer_context_status = ebr_status;
  int rc = _emm_tracking_area_update_accept (tau_proc);
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
  mme_ue_s1ap_id_t                        ue_id = emm_context->ue_id;

  memset((void*)&emm_sap, 0, sizeof(emm_sap_t)); /**< Set all to 0. */

  /** Get the ECM mode. */
  struct ue_context_s * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, emm_context->ue_id);
  DevAssert(ue_context);

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
  if(tau_proc->ies->is_initial){
    /**
     * Check the active flag. If false, set a notification to release the bearers after TAU_ACCEPT/COMPLETE (depending on the EMM state).
     */
    if(!tau_proc->ies->eps_update_type.active_flag){
    	if(!tau_proc->pending_qos) {
        	emm_sap.primitive = EMMAS_DATA_REQ;
        	emm_sap.u.emm_as.u.data.pending_deac = true; /**< Only set for emm_as.data. */
    	} else {
    	  	  OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - TAU procedure for ue_id " MME_UE_S1AP_ID_FMT " has pending QoS. Overwriting the active flag.", ue_context->privates.mme_ue_s1ap_id);
    	  	  tau_proc->ies->eps_update_type.active_flag = true;
    	  	  emm_sap.primitive = EMMAS_ESTABLISH_CNF;
    	}
    }else{
      /**
       * Notify EMM-AS SAP that Tracking Area Update Accept message together with an Activate
       * Default EPS Bearer Context Request message has to be sent to the UE.
       *
       * When the "Active flag" is not set in the TAU Request message and the Tracking Area Update was not initiated
       * in ECM-CONNECTED state, the new MME releases the signaling connection with UE, according to clause 5.3.5.
       */
      emm_sap.primitive = EMMAS_ESTABLISH_CNF;
    }
  }else{
    emm_sap.primitive = EMMAS_DATA_REQ; /**< We also check the current ECM state to handle active flag. */
  }

  /** Set the rest as data. */
  emm_sap.u.emm_as.u.data.ue_id = emm_context->ue_id; /**< These should also set for data. */
  emm_sap.u.emm_as.u.data.nas_info = EMM_AS_NAS_DATA_TAU;

  NO_REQUIREMENT_3GPP_24_301(R10_5_5_3_2_4__3);
  //----------------------------------------
  REQUIREMENT_3GPP_24_301(R10_5_5_3_2_4__4);
  //----------------------------------------

  if (tau_proc->ies->ms_network_capability) {
    emm_ctx_set_valid_ms_nw_cap(emm_context, tau_proc->ies->ms_network_capability);
    /** Not clearing them, we might have received it from S10. */
  }

  if (tau_proc->ies->ue_network_capability) {
    emm_ctx_set_valid_ue_nw_cap(emm_context, tau_proc->ies->ue_network_capability);
  } else {
    // optional IE
    /** Not clearing them, we might have received it from S10. */
  }

  //----------------------------------------
  if (tau_proc->ies->drx_parameter) {
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__5);
    emm_ctx_set_valid_drx_parameter(emm_context, tau_proc->ies->drx_parameter);
  }
  // todo: review this
//  emm_ctx_clear_pending_current_drx_parameter(emm_context);

  //----------------------------------------
  /*
   * Set the GUTI.
   */
  //----------------------------------------
  REQUIREMENT_3GPP_24_301(R10_5_5_3_2_4__1b);
  if (!IS_EMM_CTXT_PRESENT_GUTI(emm_context)) {
    // Sure it is an unknown GUTI in this MME
    guti_t old_guti = emm_context->_old_guti;
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
  } else {
	  //----------------------------------------
	  /**
	   * Set the TAI_LIST valid with TAI_ACCEPT.
	   * The TAU may be changed, so update the TAI-List.
	   */
	  REQUIREMENT_3GPP_24_301(R10_5_5_3_2_4__1c);
	  if(_emm_data.conf.force_tau){
		  /** Only give the current TAC in the list. */
		  OAILOG_INFO (LOG_NAS, "UE " MME_UE_S1AP_ID_FMT " has an intra-MME TAU. Only updating the TAI-List to new TAC " TAC_FMT ".\n",
				  ue_context->privates.mme_ue_s1ap_id, emm_context->originating_tai.tac);
		  emm_context->_tai_list.numberoflists = 1;
		  emm_context->_tai_list.partial_tai_list[0].numberofelements = 0; /**< + 1. */
		  emm_context->_tai_list.partial_tai_list[0].typeoflist = TRACKING_AREA_IDENTITY_LIST_ONE_PLMN_CONSECUTIVE_TACS;
		  emm_context->_tai_list.partial_tai_list[0].u.tai_one_plmn_consecutive_tacs.tac = emm_context->originating_tai.tac;
		  /** PLMN may not differ. */
	  }
  }
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
        "Not including new GUTI in Tracking Area Update Accept message\n", emm_context->ue_id, emm_context->_imsi, GUTI_ARG(&emm_context->_guti));
    emm_sap.u.emm_as.u.data.new_guti  = NULL;
    emm_sap.u.emm_as.u.data.eps_id.guti = &emm_context->_guti;
  }
  //----------------------------------------
  REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__14);
  emm_sap.u.emm_as.u.data.eps_network_feature_support = &_emm_data.conf.eps_network_feature_support;
  emm_sap.u.emm_as.u.data.eps_update_result = ( tau_proc->ies->eps_update_type.eps_update_type_value == 1 || tau_proc->ies->eps_update_type.eps_update_type_value == 2) ? 1 : 0; /**< Set the UPDATE_RESULT irrelevant of data/establish. */
  if(tau_proc->ies->eps_bearer_context_status)
	  emm_sap.u.emm_as.u.data.eps_bearer_context_status = *(tau_proc->ies->eps_bearer_context_status);

  emm_sap.u.emm_as.u.data.nas_msg = NULL;

  /*
   * Setup EPS NAS security data
   */
  emm_as_set_security_data (&emm_sap.u.emm_as.u.data.sctx_data, &emm_context->_security, false, true);
  OAILOG_TRACE (LOG_NAS_EMM, "EMM-PROC  - encryption = 0x%X \n ", emm_sap.u.emm_as.u.data.encryption);
  OAILOG_TRACE (LOG_NAS_EMM, "EMM-PROC  - integrity  = 0x%X \n ", emm_sap.u.emm_as.u.data.integrity);
  emm_sap.u.emm_as.u.data.encryption = emm_context->_security.selected_algorithms.encryption;
  emm_sap.u.emm_as.u.data.integrity = emm_context->_security.selected_algorithms.integrity;
  OAILOG_TRACE (LOG_NAS_EMM, "EMM-PROC  - encryption = 0x%X (0x%X) \n ", emm_sap.u.emm_as.u.data.encryption, emm_context->_security.selected_algorithms.encryption);
  OAILOG_TRACE (LOG_NAS_EMM, "EMM-PROC  - integrity  = 0x%X (0x%X) \n ", emm_sap.u.emm_as.u.data.integrity, emm_context->_security.selected_algorithms.integrity);

  //----------------------------------------
  REQUIREMENT_3GPP_24_301(R10_5_5_3_2_4__20);
  emm_sap.u.emm_as.u.data.eps_network_feature_support = &_emm_data.conf.eps_network_feature_support;

  /** Increment the number of tau_accept's sent. */
  tau_proc->tau_accept_sent++;
  rc = emm_sap_send (&emm_sap); /**< This may be a DL_DATA_REQ or an ESTABLISH_CNF, initiating an S1AP connection. */
  if (rc != RETURNerror) {
    // todo: REMOVE THIS WITH LOCK!
	emm_data_context_t * emm_context1               = emm_data_context_get(&_emm_data, ue_id);
	nas_emm_tau_proc_t * tau_proc1  	            = NULL;
	if(emm_context1)
	  tau_proc1					  = get_nas_specific_procedure_tau(emm_context1);
	if(emm_context1 && tau_proc1){
	  if (emm_sap.u.emm_as.u.establish.new_guti != NULL) { /**< A new GUTI was included. Start the timer to wait for TAU_COMPLETE. */
		/*
		 * Re-start T3450 timer
		 */
 	    void * unused = NULL;
 	    nas_stop_T3450(ue_id, &tau_proc1->T3450, unused);
 	    nas_start_T3450 (ue_id, &tau_proc1->T3450, tau_proc1->emm_spec_proc.emm_proc.base_proc.time_out, (void*)ue_id);
	  	//      unlock_ue_contexts(ue_context);
 	    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);

	  	//        OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Timer T3450 (%d) expires in %ld seconds (TAU)", emm_ctx->T3450.id, emm_ctx->T3450.sec);
	  	//      }
	  }
	}
  } else {
	OAILOG_WARNING (LOG_NAS_EMM, "ERROR WHILE SENDING TAU_ACCEPT! \n");
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
    if (!emm_context) {
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    }

    /**
     * Check the EPS Update type:
     * If it is PERIODIC, no common procedure needs to be activated.
     * UE will stay in the same EMM (EMM-REGISTERED) state.
     *
     * Else allocate a new GUTI.
     */
    if(tau_proc->ies->eps_update_type.eps_update_type_value == EPS_UPDATE_TYPE_PERIODIC_UPDATING){
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
        nas_delete_tau_procedure(emm_context);
        /**
         * All parameters must be validated at this point since valid GUTI.
         * Since we area in EMM_REGISTERED state, check the pending session releasion flag and release the state.
         * Sending 2 consecutive SAP messages are anyway possible.
         */
//        DevAssert(emm_context->_emm_fsm_status == EMM_REGISTERED);
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
//      if(emm_context->_emm_fsm_state != EMM_REGISTERED){
//        OAILOG_ERROR(LOG_NAS_EMM, "EMM-PROC  - IMSI " IMSI_64_FMT " has already a valid GUTI "GUTI_FMT " but is not in EMM-REGISTERED state, instead %d. \n",
//            emm_context->_imsi64, GUTI_ARG(&emm_context->_guti), emm_context->_emm_fsm_state);
//        rc = _emm_tracking_area_update_reject (emm_context->ue_id, SYSTEM_FAILURE);
//        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
//      }
      OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - IMSI " IMSI_64_FMT " has already a valid GUTI "GUTI_FMT ". A new GUTI will not be allocated and send. UE staying in EMM_REGISTERED state. \n",
          emm_context->_imsi64, GUTI_ARG(&emm_context->_guti));
      /**
       * Not allocating GUTI. No parameters should be validated here.
       * All the parameters should already be validated with the TAU_COMPLETE/ATTACH_COMPLETE before!
       * todo: check that there are no new parameters (DRX, UE/MS NC,).
       */
      DevAssert(IS_EMM_CTXT_VALID_UE_NETWORK_CAPABILITY(emm_context));
      /** No assert on the MS network capabilites.. taking all as 0. */

      /**
       * Send a TAU-Accept without GUTI.
       * Not waiting for TAU-Complete.
       * Might release bearers depending on the ECM state and the active flag without waiting for TAU-Complete.
       */

      /** We are in the DEREGISTERED state.. we should be in REGISTERED state. */
      if(emm_context->_emm_fsm_state != EMM_REGISTERED){
    	  OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - IMSI " IMSI_64_FMT " for UE_ID " MME_UE_S1AP_ID_FMT " not in EMM-REGISTERED state but in state %d. \n",
    	          emm_context->_imsi64, emm_context->ue_id, emm_context->_emm_fsm_state);
    	  emm_sap.primitive = EMMREG_TAU_CNF;
    	  emm_sap.u.emm_reg.ue_id = emm_context->ue_id;
    	  MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_COMMON_PROC_REQ ue id " MME_UE_S1AP_ID_FMT " ", msg_pP->ue_id);
    	  rc = emm_sap_send (&emm_sap);
      }
      /** Send the TAU accept. */
      // todo: GUTI for periodic TAU?!
      rc = _emm_send_tracking_area_update_accept (emm_context, tau_proc);
      nas_delete_tau_procedure(emm_context);

      /** No GUTI will be set. State should not be changed. */
      /**
       * All parameters must be validated at this point since valid GUTI.
       * Since we area in EMM_REGISTERED state, check the pending session releasion flag and release the state.
       * Sending 2 consecutive SAP messages are anyway possible.
       */
//      DevAssert(emm_context->_emm_fsm_status == EMM_REGISTERED);
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
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_COMMON_PROC_REQ ue id " MME_UE_S1AP_ID_FMT " ", msg_pP->ue_id);
      rc = emm_sap_send (&emm_sap);
    } else{
      OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - No new GUTI could be allocated for IMSI " IMSI_64_FMT". \n", emm_context->_imsi64);
      rc = _emm_tracking_area_update_reject (emm_context->ue_id, SYSTEM_FAILURE);
    }
  }else{
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - No attach process running for IMSI " IMSI_64_FMT" with ueId " MME_UE_S1AP_ID_FMT". \n", emm_context->_imsi64, emm_context->ue_id);
    rc = _emm_tracking_area_update_reject (emm_context->ue_id, SYSTEM_FAILURE);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_tracking_area_update_abort (struct emm_data_context_s *emm_context, struct nas_emm_base_proc_s * emm_base_proc)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  nas_emm_tau_proc_t    *tau_proc = get_nas_specific_procedure_tau(emm_context);

  if (tau_proc) {
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Abort the TAU procedure (ue_id=" MME_UE_S1AP_ID_FMT ")", emm_context->ue_id);

    /*
     * Stop timer T3450 (if exists).
     */
    void * unused = NULL;
    nas_stop_T3450(tau_proc->ue_id, &tau_proc->T3450, unused);

//    /*
//     * TAU only should be aborted. No explicit REJECT has to be sent.
//     * Currently, for _emm_attach_abort() and for _emm_tracking_area_update_abort() we implicitly detach the UE.
//     * todo: should this not be  done with DEREGISTER? (leaving the EMM/Security context)
//     * todo: how to define in NAS_IMPLICIT_DETACH if EMM/Security context should stay or not?
//     */
//    emm_sap_t                               emm_sap = {0};
//
//    emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;
//    emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = emm_context->ue_id;
//    rc = emm_sap_send (&emm_sap);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}


/*
 *
 * Name:    _emm_attach_accept_retx()
 *
 * Description: Retransmit ATTACH ACCEPT message and restart timer T3450
 *
 * Inputs:  data:      Attach accept retransmission data
 *      Others:    None
 * Outputs:     None
 *      Return:    RETURNok, RETURNerror
 *      Others:    T3450
 *
 */
//------------------------------------------------------------------------------
static int _emm_tracking_area_update_accept_retx (emm_data_context_t * emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_sap_t                               emm_sap = {0};
  int                                     rc = RETURNerror;
  int                                     i = 0;


  if (!emm_context) {
    OAILOG_WARNING (LOG_NAS_EMM, "emm_ctx NULL\n");
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }
  if (!IS_EMM_CTXT_PRESENT_GUTI(emm_context)) {
    OAILOG_WARNING (LOG_NAS_EMM, " No GUTI present in emm_ctx. Abormal case. Skipping Retx of TAU Accept NULL\n");
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }

  mme_ue_s1ap_id_t                        ue_id = emm_context->ue_id;
  nas_emm_tau_proc_t                     *tau_proc = get_nas_specific_procedure_tau(emm_context);

  if (tau_proc) {
    /*
     * Notify EMM-AS SAP that Tracking Area Update Accept message has to be sent to the UE.
     * Retx of Tracking Area Update Accept needs to be done via DL NAS Transport S1AP message
     */
    emm_sap.primitive = EMMAS_DATA_REQ;
    emm_sap.u.emm_as.u.data.ue_id = ue_id;
    emm_sap.u.emm_as.u.data.nas_info = EMM_AS_NAS_DATA_TAU;
    emm_sap.u.emm_as.u.data.tai_list.numberoflists    = emm_context->_tai_list.numberoflists;
    for (i = 0; i < emm_context->_tai_list.numberoflists; i++) {
      memcpy(&emm_sap.u.emm_as.u.data.tai_list.partial_tai_list[i], &emm_context->_tai_list.partial_tai_list[i], sizeof(tai_t));
    }

    emm_sap.u.emm_as.u.data.eps_id.guti = &emm_context->_guti;
    OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Include the same GUTI in the TAU Retx message\n", ue_id);
    emm_sap.u.emm_as.u.data.new_guti    = &emm_context->_guti;
    emm_sap.u.emm_as.u.data.eps_network_feature_support = &_emm_data.conf.eps_network_feature_support;

//    /** If the GUTI is not validated, add the GUTI, else don't add the GUTI, not to enter COMMON state. */
//      if(!IS_EMM_CTXT_VALID_GUTI(emm_ctx)){
//        emm_sap.u.emm_as.u.data.eps_id.guti = &emm_ctx->_guti;
//        OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Include the same GUTI " GUTI_FMT " in the Tracking Area Update Accept Retx message. \n", emm_ctx->ue_id, GUTI_ARG(&emm_ctx->_guti));
//        emm_sap.u.emm_as.u.data.new_guti    = &emm_ctx->_guti;
//      }else{
//        OAILOG_DEBUG (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Not including the validated GUTI " GUTI_FMT " in the Tracking Area Update Accept Retx message. \n", emm_ctx->ue_id, GUTI_ARG(&emm_ctx->_guti));
//      }
    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data (&emm_sap.u.emm_as.u.data.sctx_data, &emm_context->_security, false, true);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - encryption = 0x%X ", emm_sap.u.emm_as.u.data.encryption);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - integrity  = 0x%X ", emm_sap.u.emm_as.u.data.integrity);
    emm_sap.u.emm_as.u.data.encryption = emm_context->_security.selected_algorithms.encryption;
    emm_sap.u.emm_as.u.data.integrity = emm_context->_security.selected_algorithms.integrity;
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - encryption = 0x%X ", emm_sap.u.emm_as.u.data.encryption);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - integrity  = 0x%X ", emm_sap.u.emm_as.u.data.integrity);

    rc = emm_sap_send (&emm_sap);

    if (RETURNerror != rc) {
    	// TODO: REMOVE THIS AFTER FIXING LOCKS FOR TAU ACCEPT
      emm_data_context_t * emm_context1               = emm_data_context_get(&_emm_data, ue_id);
      if(emm_context1){
        tau_proc = get_nas_specific_procedure_tau(emm_context1);
        if(tau_proc){
    	  OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  -Sent Retx TAU Accept message\n", ue_id);
    	  /*
    	   * Re-start T3450 timer
    	   */
    	  void * unused = NULL;
    	  nas_stop_T3450(ue_id, &tau_proc->T3450, unused);
    	  nas_start_T3450(ue_id, &tau_proc->T3450, tau_proc->emm_spec_proc.emm_proc.base_proc.time_out, (void*)tau_proc->ue_id);
        } else {
        	OAILOG_WARNING (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Send failed- Retx TAU Accept message\n", ue_id);
        }
      }
    }
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
    if ((ies1->old_guti_type) && (ies2->old_guti_type)) {
      if (*ies1->old_guti_type != *ies2->old_guti_type) {
         OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed: Native GUTI %d -> %d \n", ue_id, *ies1->old_guti_type, *ies2->old_guti_type);
         OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
      }
    }
 //  if ((ies1->old_guti) && (!ies2->old_guti)) {
//    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed:  GUTI " GUTI_FMT " -> None\n", ue_id, GUTI_ARG(&ies1->old_guti));
//    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
//  }

//  if ((!ies1->old_guti) && (ies2->old_guti)) {
//    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" TAU IEs changed:  GUTI None ->  " GUTI_FMT "\n", ue_id, GUTI_ARG(&ies2->old_guti));
//    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
//  }

//  if ((ies1->old_guti) && (ies2->old_guti)) {
    if (memcmp(&ies1->old_guti, &ies2->old_guti, sizeof(ies1->old_guti))) {
      OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT" GUTI IEs changed:  guti/tmsi " GUTI_FMT " -> " GUTI_FMT "\n", ue_id,
          GUTI_ARG(&ies1->old_guti), GUTI_ARG(&ies2->old_guti));
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

//    }
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
  if (memcmp(ies1->ue_network_capability, ies2->ue_network_capability, sizeof(*ies1->ue_network_capability))) {
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
static int _emm_tau_retry_procedure(mme_ue_s1ap_id_t ue_id){
  /** Validate that no old EMM context exists. */
  OAILOG_FUNC_IN (LOG_NAS_EMM);

  int                                      rc = RETURNerror;
  emm_data_context_t                     * emm_context                 = emm_data_context_get(&_emm_data, ue_id);
  if(!emm_context){
	  OAILOG_ERROR(LOG_NAS_EMM, "EMM-PROC  - Attach retry: no EMM context for UE " MME_UE_S1AP_ID_FMT". Triggering implicit detach. \n", ue_id);
	  rc = _emm_tracking_area_update_reject(ue_id, EMM_CAUSE_ILLEGAL_UE);
	  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }

  nas_emm_tau_proc_t                     * tau_proc = (nas_emm_tau_proc_t*)get_nas_specific_procedure(emm_context);
  emm_data_context_t                     * duplicate_emm_context       = emm_data_context_get(&_emm_data, tau_proc->emm_spec_proc.old_ue_id);
  ue_context_t                           * duplicate_ue_context        = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, tau_proc->emm_spec_proc.old_ue_id);

  /** Get the attach procedure. */

  if(duplicate_emm_context){
    /** Send an attach reject back. */
    OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - An old EMM context for ue_id=" MME_UE_S1AP_ID_FMT " still existing. Aborting the TAU procedure. \n", tau_proc->emm_spec_proc.old_ue_id);
    rc = _emm_tracking_area_update_reject(tau_proc->ue_id, EMM_CAUSE_ILLEGAL_UE);

//    emm_sap_t emm_sap                      = {0};
//    emm_sap.primitive                      = EMMREG_TAU_REJ;
//    emm_sap.u.emm_reg.ue_id                = tau_proc->ue_id;
//    emm_sap.u.emm_reg.notify               = true;
//    emm_sap.u.emm_reg.free_proc            = true;
//    emm_sap.u.emm_reg.u.attach.proc        = tau_proc;
//    // don't care emm_sap.u.emm_reg.u.tau.is_emergency = false;
//    rc = emm_sap_send (&emm_sap);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }
  /* Check that no old MME_APP UE context exists. */
  if(duplicate_ue_context){
    OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - An old UE context for ue_id=" MME_UE_S1AP_ID_FMT " still existing. Aborting the TAU procedure. \n", tau_proc->emm_spec_proc.old_ue_id);
    /** Send an attach reject back. */
    rc = _emm_tracking_area_update_reject(tau_proc->ue_id, EMM_CAUSE_ILLEGAL_UE);

//    emm_sap_t emm_sap                      = {0};
//    emm_sap.primitive                      = EMMREG_TAU_REJ;
//    emm_sap.u.emm_reg.ue_id                = tau_proc->ue_id;
//    emm_sap.u.emm_reg.notify               = true;
//    emm_sap.u.emm_reg.free_proc            = true;
//    emm_sap.u.emm_reg.u.attach.proc        = tau_proc;
//    // don't care emm_sap.u.emm_reg.u.attach.is_emergency = false;
//    rc = emm_sap_send (&emm_sap);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);

  }
  OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - No old EMM/UE context exists for ue_id=" MME_UE_S1AP_ID_FMT ". Continuing with TAU procedure for new ueId " MME_UE_S1AP_ID_FMT ". \n",
      tau_proc->emm_spec_proc.old_ue_id, emm_context->ue_id);

  if(!emm_data_context_get(&_emm_data, emm_context->ue_id)){
	  if (RETURNok != emm_data_context_add (&_emm_data, emm_context)) {
		  rc = _emm_tracking_area_update_reject(emm_context->ue_id, EMM_CAUSE_ILLEGAL_UE);
		  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
	  }
  }

  rc = _emm_tracking_area_update_run_procedure(emm_context);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_tracking_area_update_run_procedure(emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);

  DevAssert(emm_context);

  int                                     rc = RETURNerror;
  nas_emm_tau_proc_t                     *tau_proc = get_nas_specific_procedure_tau(emm_context);
  ue_context_t                           *ue_context = NULL;

  /** Retrieve the MME_APP UE context. */
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);
  if(!ue_context){
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  TAU - For ueId " MME_UE_S1AP_ID_FMT " no ue context exists (while running TAU procedure). Removing the EMM context without sending attach reject.. \n", emm_context->ue_id);
    // Release EMM context
    if(emm_context->is_dynamic) {
    	_clear_emm_ctxt(emm_context->ue_id);
    }
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }

  if (tau_proc->ies->last_visited_registered_tai) {
    emm_ctx_set_valid_lvr_tai(emm_context, tau_proc->ies->last_visited_registered_tai);
  } else {
    emm_ctx_clear_lvr_tai(emm_context);
  }

  if (tau_proc) {
    REQUIREMENT_3GPP_24_301(R10_5_5_3_2_3__2);

    /*
     * We don't need to check the security context or if the MAC was matched.
     * That's done already in the TAU validation.
     */
    emm_context->originating_tai = *tau_proc->ies->originating_tai;

    /**
     * Requirements MME24.301R10_5.5.3.2.4_2
     * Setting the UE & MS network capabilities as present and validating them after replaying / successfully SMC.
     */
    if (tau_proc->ies->ue_network_capability) {
      emm_ctx_set_ue_nw_cap((emm_data_context_t * const)emm_context, tau_proc->ies->ue_network_capability);
    }else{
      /** Not clearing the attributes. We will keep them from the attach. */
    }
    if (tau_proc->ies->ms_network_capability) {
      emm_ctx_set_ms_nw_cap(emm_context, tau_proc->ies->ms_network_capability);
    }else{
//      emm_ctx_clear_ms_nw_cap(emm_context);
//      emm_context->gea = (msg->msnetworkcapability.gea1 << 6)| msg->msnetworkcapability.egea;
//      emm_context->gprs_present = true; /**< Todo: how to find this out? */
    }
    //    emm_context->gea = (msg->msnetworkcapability.gea1 << 6)| msg->msnetworkcapability.egea;
    //    emm_context->gprs_present = true; /**< Todo: how to find this out? */

    /** Not working with TAU_REQUEST number. */
//    OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC-  Num TAU_REQUESTs for UE with GUTI " GUTI_FMT " and ue_id " MME_UE_S1AP_ID_FMT " is: %d. \n", GUTI_ARG(&emm_context->_guti), emm_context->ue_id, tau_proc->->num_tau_request);

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
//      rc = emm_wrapper_tracking_area_update_reject (ue_id, EMM_CAUSE_TA_NOT_ALLOWED);
//      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
//    }

    /** Set the Old-GUTI (event is context there) as present. */
    emm_ctx_set_old_guti((emm_data_context_t * const)emm_context, &tau_proc->ies->old_guti);
    /** The UE context may have a valid GUTI. It might be equal to the old GUTI? .*/

    /** Set the GUTI-Type (event is context there). */
    if (tau_proc->ies->old_guti_type) {
      /** Old GUTI Type present. Not setting anything in the EMM_CTX. */
    }
    /*
     * Requirements MME24.301R10_5.5.3.2.4_4
     */
    if (tau_proc->ies->drx_parameter) {
      emm_ctx_set_drx_parameter(emm_context, tau_proc->ies->drx_parameter);
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
      bdestroy_wrapper(&ue_context->privates.fields.ue_radio_capability);
    }

    /** A security context exist, checking if UE was correctly authenticated. */
    if (IS_EMM_CTXT_PRESENT_SECURITY(emm_context)) {
      /** Consider the ESM security context of the UE as valid and continue to handle the ESM message with making a ULR at the HSS. */
      OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - EMM context for the ue_id=" MME_UE_S1AP_ID_FMT " has a valid and active EPS security context. "
          "Continuing with the tracking area update request. \n", emm_context->ue_id);
      /*
       * No AUTH or SMC is needed, directly perform ULR/Subscription (and always perform it
       * due to MeshFlow reasons). Assume that the UE/MS network capabilities are not changed and don't need to be
       * replayed.
       */
      /** Check if any S6a Subscription information exist, if not pull them from the HSS. */
      OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC- THE UE with ue_id=" MME_UE_S1AP_ID_FMT ", has already a security context. Checking for a subscription profile. \n", emm_context->ue_id);

      /** Check for subscription data. */
      subscription_data_t *subscription_data = mme_ue_subscription_data_exists_imsi(&mme_app_desc.mme_ue_contexts, emm_context->_imsi64);
      if(!subscription_data) { /**< Means, that the MM UE context is received from the sourc MME already due HO (and only due HO). */
        if(tau_proc->ies->subscription_data){
        	subscription_data = tau_proc->ies->subscription_data;
            mme_insert_subscription_profile(&mme_app_desc.mme_ue_contexts, emm_context->_imsi64, subscription_data);
        	mme_app_update_ue_subscription(emm_context->ue_id, tau_proc->ies->subscription_data);
        	tau_proc->ies->subscription_data = NULL;
        	/** Send the S6a message. */
        	MessageDef * message_p = itti_alloc_new_message (TASK_MME_APP, NAS_PDN_CONFIG_RSP);
        	message_p->ittiMsg.nas_pdn_config_rsp.ue_id  = ue_context->privates.mme_ue_s1ap_id;
        	message_p->ittiMsg.nas_pdn_config_rsp.imsi64 = emm_context->_imsi64;
        	imsi64_t imsi64_2 = imsi_to_imsi64(&emm_context->_imsi);
        	memcpy(&message_p->ittiMsg.nas_pdn_config_rsp.imsi, &emm_context->_imsi, sizeof(imsi_t));
        	memcpy(&message_p->ittiMsg.nas_pdn_config_rsp.target_tai, &emm_context->originating_tai, sizeof(tai_t));
        	/** Check if an attach procedure is running, if so send it to the ESM layer, if not, we assume it is triggered by a TAU request, send it to the EMM. */
        	/** For error codes, use nas_pdn_cfg_fail. */
        	MSC_LOG_TX_MESSAGE (MSC_MMEAPP_MME, MSC_NAS_ESM, NULL, 0, "0 NAS_PDN_CONFIG_RESP IMSI " IMSI_64_FMT, emm_context->_imsi64);
        	rc =  itti_send_msg_to_task (TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
        	OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
    	} else {
            OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC- THE UE with ue_id=" MME_UE_S1AP_ID_FMT ", does not have a subscription profile set for IMSI " IMSI_64_FMT". "
            		"Requesting a new subscription profile. \n", emm_context->ue_id, emm_context->_imsi64);
            /**
             * Always use initial to trigger a CLR.
             */
            rc = nas_itti_pdn_config_req(emm_context->ue_id, &emm_context->_imsi, REQUEST_TYPE_INITIAL_REQUEST, &emm_context->originating_tai.plmn);
            OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    	}
      } else{
        OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC- Sending Tracking Area Update Accept for UE with valid subscription ue_id=" MME_UE_S1AP_ID_FMT ", active flag=%d)\n", emm_context->ue_id, tau_proc->ies->eps_update_type.active_flag);
        /* Check the state of the EMM context. If it is REGISTERED, send an TAU_ACCEPT back and remove the tau procedure. */
        if(emm_context->_emm_fsm_state == EMM_REGISTERED){
          rc = _emm_tracking_area_update_accept (tau_proc); /**< Will remove the TAU procedure. */
          nas_delete_tau_procedure(emm_context);
        }else{
          OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - EMM context for the ue_id=" MME_UE_S1AP_ID_FMT " has a valid and active EPS security context and subscription but is not in EMM_REGISTERED state. Instead %d. "
              "Continuing with the tracking area update reject. \n", emm_context->ue_id);
          rc = emm_wrapper_tracking_area_update_reject (emm_context, EMM_CAUSE_NETWORK_FAILURE);
          OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
        }
      }

      /** Not finishing the TAU procedure. */
    }else{
      /** Check if the origin TAI is belonging to the same MME (RF issues). */
      if (mme_app_check_ta_local(&tau_proc->ies->last_visited_registered_tai->plmn, tau_proc->ies->last_visited_registered_tai->tac)) {
        OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - For UE " MME_UE_S1AP_ID_FMT " the last visited TAI " TAI_FMT " is attached to current MME (assuming RF issues). "
            "Proceeding with identification procedure. \n", TAI_ARG(tau_proc->ies->last_visited_registered_tai), emm_context->ue_id);
        rc = emm_proc_identification (emm_context, (nas_emm_proc_t *)tau_proc, IDENTITY_TYPE_2_IMSI, _emm_tracking_area_update_success_identification_cb, _emm_tracking_area_update_failure_identification_cb);
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
      }

      /** Check if the origin TAI is a neighboring MME where we can request the UE MM context. */
//      struct in_addr neigh_mme_ipv4_addr;
//      neigh_mme_ipv4_addr.s_addr = 0;

      struct sockaddr *edns_neigh_mme_ipv4_addr = NULL;
//      edns_neigh_mme_ipv4_addr.addr.ipv4_addr.s_addr = 0;

      mme_app_select_service(tau_proc->ies->last_visited_registered_tai, &edns_neigh_mme_ipv4_addr, S10_MME_GTP_C);
      if(!edns_neigh_mme_ipv4_addr) {
    	  OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - For UE " MME_UE_S1AP_ID_FMT " the last visited TAI " TAI_FMT ", no neighboring object for MME S10 could be found. \n",
    			  emm_context->ue_id, TAI_ARG(tau_proc->ies->last_visited_registered_tai));
          rc = emm_proc_identification (emm_context, (nas_emm_proc_t *)tau_proc, IDENTITY_TYPE_2_IMSI, _emm_tracking_area_update_success_identification_cb, _emm_tracking_area_update_failure_identification_cb);
          OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
      }
      if(edns_neigh_mme_ipv4_addr->sa_family ==0){
        OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - For UE " MME_UE_S1AP_ID_FMT " the last visited TAI " TAI_FMT " is not configured as a MME S10 neighbor. "
            "Proceeding with identification procedure. \n", emm_context->ue_id, TAI_ARG(tau_proc->ies->last_visited_registered_tai));
        rc = emm_proc_identification (emm_context, (nas_emm_proc_t *)tau_proc, IDENTITY_TYPE_2_IMSI, _emm_tracking_area_update_success_identification_cb, _emm_tracking_area_update_failure_identification_cb);
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
      }else{
        /*
         * Originating TAI is configured as an MME neighbor.
         * Will create the UE context and send an S10 UE Context Request. */
        OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - Originating TAI " TAI_FMT " is configured as a MME S10 neighbor. Will request UE context from source MME for ue_id = " MME_UE_S1AP_ID_FMT ". "
            "Creating a new EMM context. \n", TAI_ARG(tau_proc->ies->last_visited_registered_tai), emm_context->ue_id);
        /*
         * Check if there is a S10 handover procedure running.
         * (We may have received the signal as NAS uplink data request, without any intermission from MME_APP layer).
         */
        mme_app_s10_proc_mme_handover_t * s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
        if(s10_handover_proc){
          DevAssert(s10_handover_proc->nas_s10_context.mm_eps_ctx);
          OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - We have receive the TAU as part of an S10 procedure for ue_id = " MME_UE_S1AP_ID_FMT ". "
              "Continuing with the pending MM EPS Context. \n", emm_context->ue_id);
          /*
           * No need to start a new EMM_CN_CONTEXT_REQ procedure.
           * Will update the current EMM and ESM context with the pending values and continue with the registration in the HSS (ULR).
           */
          // todo: set the active flag as true
          rc = _context_req_proc_success_cb(emm_context);
          OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
        }else{
          if(!tau_proc->ies->is_initial){
            OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC- No handover procedure exists for non-initial TAU request for UE with ue_id=" MME_UE_S1AP_ID_FMT ". \n", emm_context->ue_id);
            rc = emm_wrapper_tracking_area_update_reject (emm_context, EMM_CAUSE_NETWORK_FAILURE);
            OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
          }
          /**
           * Send an S10 context request. Send the PLMN together (will be serving network IE). New TAI not need to be sent.
           * Set the new TAI and the new PLMN to the UE context. In case of a TAC-Accept, will be sent and registered.
           */
          OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC- Security context for EMM_DATA_CONTEXT is missing & no S10 procedu. Sending a S10 context request for UE with ue_id=" MME_UE_S1AP_ID_FMT " to old MME. \n", emm_context->ue_id);
          /**
           * Directly inform the MME_APP of the new context.
           * Depending on if there is a pending MM_EPS_CONTEXT or not, it should send S10 message..
           * MME_APP needs to register the MME_APP context with the IMSI before sending S6a ULR.
           *
           * A S10 Context Request process may already have been started.
           */
          rc = _start_context_request_procedure(emm_context, tau_proc,
              _context_req_proc_success_cb, _context_req_proc_failure_cb);
          /** Do S10 Context Request. */
          nas_itti_ctx_req(emm_context->ue_id, &emm_context->_old_guti,
              tau_proc->ies->originating_tai,
              tau_proc->ies->last_visited_registered_tai,
              tau_proc->ies->complete_tau_request);
          OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
        }
      }
    }
    /** Not freeing the IEs of the TAU procedure. */
  }else{
    OAILOG_WARNING(LOG_NAS_EMM, "EMM-PROC  - For UE " MME_UE_S1AP_ID_FMT " no TAU specific procedure is running. Not proceeding with TAU Request. ", emm_context->ue_id);
    rc = emm_wrapper_tracking_area_update_reject (emm_context, EMM_CAUSE_NETWORK_FAILURE);
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
    mme_app_s10_proc_mme_handover_t *s10_handover_proc = mme_app_get_s10_procedure_mme_handover(ue_context);
    DevAssert(s10_handover_proc); // todo: optimize this as well, either s10 handover procedure or nas context request procedure should exist
    nas_s10_ctx = &s10_handover_proc->nas_s10_context;
  }else{
    nas_s10_ctx = &nas_ctx_req_proc->nas_s10_context;
  }

  /*
   * Set the identity values (IMSI) valid and present.
   * Assuming IMSI is always returned with S10 Context Response and the IMSI hastable registration method validates the received IMSI.
   */
  clear_imsi(&emm_context->_imsi);
  nas_s10_ctx->imsi = imsi_to_imsi64(&nas_s10_ctx->_imsi);
  emm_ctx_set_valid_imsi(emm_context, &nas_s10_ctx->_imsi, nas_s10_ctx->imsi);
  emm_data_context_upsert_imsi(&_emm_data, emm_context); /**< Register the IMSI in the hash table. */

  /*
   * Update the security context & security vectors of the UE independent of TAU/Attach here (set fields valid/present).
   * Then inform the MME_APP that the context was successfully authenticated. Trigger a CSR.
   */
  emm_context->_security.ul_count.seq_num = tau_proc->ies->nas_ul_count;
  emm_ctx_update_from_mm_eps_context(emm_context, nas_s10_ctx->mm_eps_ctx);
  OAILOG_INFO(LOG_NAS_EMM, "EMM-PROC  - " "Successfully updated the EMM context with ueId " MME_UE_S1AP_ID_FMT " from the received MM_EPS_Context from the MME for UE with imsi: " IMSI_64_FMT ". \n",
      emm_context->ue_id, emm_context->_imsi64);
  /** No need to touch the ESM context. No ESM procedure is running. */

  /** Delete the CN procedure, if exists. */
  if(nas_ctx_req_proc)
    nas_delete_cn_procedure(emm_context, nas_ctx_req_proc);

  /** ESM context will not change, no ESM_PROC data will be created. */
  rc = nas_itti_pdn_config_req(emm_context->ue_id, &emm_context->_imsi, REQUEST_TYPE_INITIAL_REQUEST, &emm_context->originating_tai.plmn);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _context_req_proc_failure_cb (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  nas_ctx_req_proc_t *nas_ctx_req_proc = get_nas_cn_procedure_ctx_req(emm_context);
  nas_emm_tau_proc_t                    *tau_proc = get_nas_specific_procedure_tau(emm_context);
  /** Delete the context request procedure. */
  if(nas_ctx_req_proc)
     nas_delete_cn_procedure(emm_context, nas_ctx_req_proc);

  if (tau_proc) {
    // todo: requirement to continue with identification, auth and SMC if context res fails!
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
    rc = emm_proc_authentication (emm_context, &tau_proc->emm_spec_proc, _emm_tracking_area_update_success_authentication_cb, _emm_tracking_area_update_failure_authentication_cb);
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
    rc = _emm_tracking_area_update_reject(emm_context->ue_id, tau_proc->emm_cause);

//    emm_sap_t emm_sap                      = {0};
//    emm_sap.primitive                      = EMMREG_TAU_REJ;
//    emm_sap.u.emm_reg.ue_id                = tau_proc->ue_id;
//    emm_sap.u.emm_reg.notify               = true;
//    emm_sap.u.emm_reg.free_proc            = true;
//    emm_sap.u.emm_reg.u.tau.proc = tau_proc;
//    // don't care emm_sap.u.emm_reg.u.tau.is_emergency = false;
//    rc = emm_sap_send (&emm_sap);
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
      OAILOG_WARNING (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT "EMM-PROC  - Failed to initiate security mode control procedure\n", emm_context->ue_id);
      tau_proc->emm_cause = EMM_CAUSE_ILLEGAL_UE;
//      /*
//       * Do not accept the UE to attach to the network
//       */
//      emm_sap_t emm_sap                      = {0};
//      emm_sap.primitive                      = EMMREG_TAU_REJ;
//      emm_sap.u.emm_reg.ue_id                = emm_context->ue_id;
//      emm_sap.u.emm_reg.notify               = true;
//      emm_sap.u.emm_reg.free_proc            = true;
//      emm_sap.u.emm_reg.u.tau.proc    = tau_proc;
//      // dont care emm_sap.u.emm_reg.u.tau.is_emergency = false;
//      rc = emm_sap_send (&emm_sap);

      rc = _emm_tracking_area_update_reject(tau_proc->ue_id, EMM_CAUSE_ILLEGAL_UE);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);

    }
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_tracking_area_update_success_security_cb (emm_data_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  nas_emm_tau_proc_t                     *tau_proc = get_nas_specific_procedure_tau(emm_context);
//  esm_sap_t                               esm_sap = {0};

  if (tau_proc) {
	/**
	 * Not checking if any ESM process is running, should not.
	 * Ask for subscription information.
	 */
    rc = nas_itti_pdn_config_req(emm_context->ue_id, &emm_context->_imsi, REQUEST_TYPE_INITIAL_REQUEST, &emm_context->originating_tai.plmn);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
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
    tau_proc->emm_cause = emm_context->emm_cause;

     // TODO could be in callback of attach procedure triggered by EMMREG_ATTACH_REJ
     rc = _emm_tracking_area_update_reject(emm_context->ue_id, emm_context->emm_cause);

 //    emm_sap_t emm_sap                      = {0};
 //    emm_sap.primitive                      = EMMREG_ATTACH_REJ;
 //    emm_sap.u.emm_reg.ue_id                = attach_proc->ue_id;
 //    emm_sap.u.emm_reg.notify               = true;
 //    emm_sap.u.emm_reg.free_proc            = true;
 //    emm_sap.u.emm_reg.u.attach.proc = attach_proc;
 //    // don't care emm_sap.u.emm_reg.u.attach.is_emergency = false;
 //    rc = emm_sap_send (&emm_sap);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static void _emm_tracking_area_update_registration_complete(emm_data_context_t *emm_context){
  OAILOG_FUNC_IN (LOG_MME_APP);

  MessageDef                    *message_p = NULL;
  int                            rc = RETURNerror;

  nas_emm_tau_proc_t                     *tau_proc = get_nas_specific_procedure_tau(emm_context);
  ue_context_t 						     *ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, emm_context->ue_id);
  DevAssert(tau_proc);

  /**
   * Check for an S10 procedure. If so release it.
   * If no procedure exists (idle-TAU), check for pending deactivation.
   */
  bool active_flag = tau_proc->ies->eps_update_type.active_flag;
  bool pending_qos = tau_proc->pending_qos;
  nas_delete_tau_procedure(emm_context);
  emm_context->is_has_been_attached = true;

  mme_app_s10_proc_mme_handover_t * s10_proc_handover = mme_app_get_s10_procedure_mme_handover(ue_context);

  if(!s10_proc_handover){
	  if(!active_flag) {
		  /**
		   * No MBR should be send because no InitialContextSetupResponse is received.
		   * Also no CBR is received and waiting in pending state.
		   */
		  OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - EMM Context for ueId " MME_UE_S1AP_ID_FMT " has done an IDLE-TAU without active flag. Triggering UE context release. \n", emm_context->ue_id);
		  mme_app_itti_ue_context_release(ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, S1AP_NAS_NORMAL_RELEASE, ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id);
	  } else if (pending_qos) {
		  /** Check if a pending QoS procedure exists. */
		  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - EMM Context for ueId " MME_UE_S1AP_ID_FMT " has done an IDLE-TAU without active flag but bearer QoS modification is pending. "
				  "Not releasing UE context. MBResp should retrigger qos operation. \n", emm_context->ue_id); /**< MBR not happened yet for IDLE TAU. */
	  }
  } else {
	  mme_app_delete_s10_procedure_mme_handover(ue_context);
	  /**
	   * We try S11 retry here. (MBR may already be handled).
	   * We should check in the MME_APP layer, if the default bearer has been already activated. Else, if we sent a CBResp before the MBR procedure completes, we might trigger an initial detach in the PGW.
	   */
	  nas_itti_s11_retry_ind(ue_context->privates.mme_ue_s1ap_id);
  }
  //  unlock_ue_contexts(ue_context);

  OAILOG_FUNC_OUT (LOG_MME_APP);
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
  if ((*ies)->ms_network_capability) {
    free_wrapper((void**)&((*ies)->ms_network_capability));
  }
  if ((*ies)->last_visited_registered_tai) {
    free_wrapper((void**)&((*ies)->last_visited_registered_tai));
  }
  if ((*ies)->last_visited_registered_tai) {
    free_wrapper((void**)&((*ies)->last_visited_registered_tai));
  }
  if((*ies)->originating_ecgi){
	free_wrapper((void**)&((*ies)->originating_ecgi));
  }
  if((*ies)->originating_tai){
	free_wrapper((void**)&((*ies)->originating_tai));
  }
  if((*ies)->complete_tau_request){
	bdestroy_wrapper(&(*ies)->complete_tau_request);
  }
  if ((*ies)->drx_parameter) {
    free_wrapper((void**)&((*ies)->drx_parameter));
  }
  if ((*ies)->eps_bearer_context_status) {
    free_wrapper((void**)&((*ies)->eps_bearer_context_status));
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
  if((*ies)->subscription_data){
	free_wrapper((void**)&((*ies)->subscription_data));
  }
  free_wrapper((void**)ies);
}

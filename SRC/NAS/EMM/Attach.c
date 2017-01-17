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
static int                              _emm_attach_identify (emm_context_t *emm_context);
static int                              _emm_attach_security (emm_context_t *emm_context);
static int                              _emm_attach (emm_context_t *emm_context);

/*
   Abnormal case attach procedures
*/
static int                              _emm_attach_release (emm_context_t *emm_context);
int                                     _emm_attach_reject (emm_context_t *emm_context);
static int                              _emm_attach_abort (emm_context_t *emm_context);

static int                              _emm_attach_have_changed (
  const emm_context_t * ctx,
  emm_proc_attach_type_t type,
  ksi_t ksi,
  guti_t * guti,
  imsi_t * imsi,
  imei_t * imei,
  const ue_network_capability_t * const ue_network_capability,
  const ms_network_capability_t * const ms_network_capability);

static int                              _emm_attach_update (
  emm_context_t * ctx,
  mme_ue_s1ap_id_t ue_id,
  emm_proc_attach_type_t type,
  ksi_t ksi,
  bool is_native_guti,
  guti_t * guti,
  imsi_t * imsi,
  imei_t * imei,
  const tai_t   * const originating_tai,
  const ue_network_capability_t * const ue_network_capability,
  const ms_network_capability_t * const ms_network_capability,
  const_bstring esm_msg_pP);



static int _emm_attach_accept (emm_context_t * emm_context);

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
  enb_s1ap_id_key_t  enb_ue_s1ap_id_key,
  mme_ue_s1ap_id_t ue_id,
  const emm_proc_attach_type_t type,
  const bool is_native_ksi,
  const ksi_t     ksi,
  const bool is_native_guti,
  guti_t * guti,
  imsi_t * imsi,
  imei_t * imei,
  tai_t * last_visited_registered_tai,
  const tai_t   * const originating_tai,
  const ecgi_t   * const originating_ecgi,
  const ue_network_capability_t * const ue_network_capability,
  const ms_network_capability_t * const ms_network_capability,
  const_bstring esm_msg_pP,
  const nas_message_decode_status_t  * const decode_status)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  ue_mm_context_t                         ue_ctx;
  emm_fsm_state_t                         fsm_state = EMM_DEREGISTERED;
  bool                                    create_new_emm_ctxt = false;
  bool                                    duplicate_enb_context_detected = false;
  ue_mm_context_t                         * ue_mm_context = NULL;
  imsi64_t                                imsi64  = INVALID_IMSI64;

  if (imsi) {
    imsi64 = imsi_to_imsi64(imsi);
  }

  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - EPS attach type = %s (%d) requested (ue_id=" MME_UE_S1AP_ID_FMT ")\n", _emm_attach_type_str[type], type, ue_id);
  /*
   * Initialize the temporary UE context
   */

  /*
   * Get the UE's EMM context if it exists
   */
  // if ue_id is valid (sent by eNB), we should always find the context
  if (INVALID_MME_UE_S1AP_ID != ue_id) {
    ue_mm_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ue_id);
  } else {
    if (guti) { // no need for  && (is_native_guti)
      ue_mm_context = mme_ue_context_exists_guti (&mme_app_desc.mme_ue_contexts, guti);
      if (ue_mm_context) {
        ue_id = ue_mm_context->mme_ue_s1ap_id;
        if (ue_mm_context->enb_s1ap_id_key != enb_ue_s1ap_id_key) {
          duplicate_enb_context_detected = true;
          OAILOG_TRACE (LOG_NAS_EMM, "EMM-PROC  - Found old ue_mm_context matching GUTI in ATTACH_REQUEST\n");
        }
      }
    }
    if ((!ue_mm_context) && (imsi)) {
      ue_mm_context = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, imsi64);
      if (ue_mm_context) {
        ue_id = ue_mm_context->mme_ue_s1ap_id;
        if (ue_mm_context->enb_s1ap_id_key != enb_ue_s1ap_id_key) {
          OAILOG_TRACE (LOG_NAS_EMM, "EMM-PROC  - Found old ue_mm_context matching IMSI in ATTACH_REQUEST\n");
          duplicate_enb_context_detected = true;
        }
      }
    }
    if (!ue_mm_context) {
      ue_mm_context = mme_ue_context_exists_enb_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, enb_ue_s1ap_id_key);
      if (ue_mm_context) {
        OAILOG_TRACE (LOG_NAS_EMM, "EMM-PROC  - Found old ue_mm_context matching enb_ue_s1ap_id in ATTACH_REQUEST\n");
        ue_id = emm_ctx_get_new_ue_id(&ue_mm_context->emm_context);
        mme_api_notified_new_ue_s1ap_id_association (ue_mm_context->enb_ue_s1ap_id, originating_ecgi->cell_identity.enb_id, ue_id);
      }
    }
  }

  memset (&ue_ctx, 0, sizeof (ue_mm_context_t));
  ue_ctx.emm_context.is_dynamic = false;
  ue_ctx.mme_ue_s1ap_id = ue_id;


  /*
   * Requirement MME24.301R10_5.5.1.1_1
   * MME not configured to support attach for emergency bearer services
   * shall reject any request to attach with an attach type set to "EPS
   * emergency attach".
   */
  if (!(_emm_data.conf.eps_network_feature_support & EPS_NETWORK_FEATURE_SUPPORT_EMERGENCY_BEARER_SERVICES_IN_S1_MODE_SUPPORTED) &&
      (type == EMM_ATTACH_TYPE_EMERGENCY)) {
    REQUIREMENT_3GPP_24_301(R10_5_5_1__1);
    ue_ctx.emm_context.emm_cause = EMM_CAUSE_IMEI_NOT_ACCEPTED;
    /*
     * Do not accept the UE to attach for emergency services
     */
    rc = _emm_attach_reject (&ue_ctx.emm_context);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }

  fsm_state = emm_fsm_get_state (&ue_mm_context->emm_context);

  if (ue_mm_context) {
    if (emm_ctx_is_common_procedure_running(&ue_mm_context->emm_context, EMM_CTXT_COMMON_PROC_SMC)) {
      REQUIREMENT_3GPP_24_301(R10_5_4_3_7_c);
      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMREG_PROC_ABORT;
      emm_sap.u.emm_reg.ue_id = ue_mm_context->mme_ue_s1ap_id;
      emm_sap.u.emm_reg.ctx   = &ue_mm_context->emm_context;
      // TODOdata->notify_failure = true;
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_PROC_ABORT (SMC) ue id " MME_UE_S1AP_ID_FMT " ", ue_mm_context->mme_ue_s1ap_id);
      rc = emm_sap_send (&emm_sap);
    }
    if (emm_ctx_is_common_procedure_running(&ue_mm_context->emm_context, EMM_CTXT_COMMON_PROC_IDENT)) {

      if (emm_ctx_is_specific_procedure_running (&ue_mm_context->emm_context, EMM_CTXT_SPEC_PROC_ATTACH)) {
        if (emm_ctx_is_specific_procedure_running (&ue_mm_context->emm_context, EMM_CTXT_SPEC_PROC_ATTACH_ACCEPT_SENT | EMM_CTXT_SPEC_PROC_ATTACH_REJECT_SENT)) {
          REQUIREMENT_3GPP_24_301(R10_5_4_4_6_c); // continue
        } else {
          if (_emm_attach_have_changed(&ue_mm_context->emm_context, type, ksi, guti, imsi, imei,
              ue_network_capability, ms_network_capability)) {
            REQUIREMENT_3GPP_24_301(R10_5_4_4_6_d__1);
            emm_sap_t                               emm_sap = {0};
            emm_sap.primitive = EMMREG_PROC_ABORT;
            emm_sap.u.emm_reg.ue_id = ue_mm_context->mme_ue_s1ap_id;
            emm_sap.u.emm_reg.ctx   = &ue_mm_context->emm_context;
            // TODOdata->notify_failure = true;
            rc = emm_sap_send (&emm_sap);

            emm_context_silently_reset_procedures(&ue_mm_context->emm_context);
          } else {
            REQUIREMENT_3GPP_24_301(R10_5_4_4_6_d__2);
            // No need to report an error
            OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
          }
        }
      } else {
        REQUIREMENT_3GPP_24_301(R10_5_4_4_6_c); // continue
      }
    }
  }

  if ((ue_mm_context) && (EMM_REGISTERED == fsm_state)) {
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_f);
    if (ue_mm_context->emm_context.is_has_been_attached) {
      OAILOG_TRACE (LOG_NAS_EMM, "EMM-PROC  - the new ATTACH REQUEST is progressed\n");
      emm_context_silently_reset_procedures(&ue_mm_context->emm_context); // TEST
      emm_fsm_set_state(ue_id, &ue_mm_context->emm_context, EMM_DEREGISTERED);
      // "TODO think about emm_context_remove"
      //ue_mm_context = emm_context_remove (&_emm_data, ue_mm_context);
      if (duplicate_enb_context_detected) {
        ue_mm_context = mme_api_duplicate_enb_ue_s1ap_id_detected (enb_ue_s1ap_id_key,ue_mm_context->mme_ue_s1ap_id, REMOVE_OLD_CONTEXT);
      }
      emm_context_free_content(&ue_mm_context->emm_context);

      create_new_emm_ctxt = true;
    }
  } else if ((ue_mm_context) && emm_ctx_is_specific_procedure_running (&ue_mm_context->emm_context, EMM_CTXT_SPEC_PROC_ATTACH_ACCEPT_SENT)) {// && (!ue_mm_context->is_attach_complete_received): implicit
    ue_mm_context->emm_context.num_attach_request++;
    if (_emm_attach_have_changed(&ue_mm_context->emm_context, type, ksi, guti, imsi, imei, ue_network_capability, ms_network_capability)) {
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Attach parameters have changed\n");
      REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_d__1);
      /*
       * If one or more of the information elements in the ATTACH REQUEST message differ from the ones
       * received within the previous ATTACH REQUEST message, the previously initiated attach procedure shall
       * be aborted if the ATTACH COMPLETE message has not been received and the new attach procedure shall
       * be progressed;
       */
      emm_sap_t                               emm_sap = {0};

      emm_sap.primitive = EMMREG_PROC_ABORT;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx = &ue_mm_context->emm_context;
      rc = emm_sap_send (&emm_sap);

      if (RETURNerror == rc) {
        emm_ctx_unmark_specific_procedure_running(&ue_mm_context->emm_context, EMM_CTXT_SPEC_PROC_ATTACH_REJECT_SENT);
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - sent of ATTACH REJECT failed\n");
      } else {
        emm_ctx_mark_specific_procedure_running (&ue_mm_context->emm_context, EMM_CTXT_SPEC_PROC_ATTACH_REJECT_SENT);
      }
      if (duplicate_enb_context_detected) {
        ue_mm_context = mme_api_duplicate_enb_ue_s1ap_id_detected (enb_ue_s1ap_id_key,ue_mm_context->mme_ue_s1ap_id, REMOVE_OLD_CONTEXT);
      }
    } else {
      REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_d__2);
      /*
       * - if the information elements do not differ, then the ATTACH ACCEPT message shall be resent and the timer
       * T3450 shall be restarted if an ATTACH COMPLETE message is expected. In that case, the retransmission
       * counter related to T3450 is not incremented.
       */
      _emm_attach_accept(&ue_mm_context->emm_context);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
    }

  } else if ((ue_mm_context) && (0 < ue_mm_context->emm_context.num_attach_request) &&
             (!emm_ctx_is_specific_procedure_running (&ue_mm_context->emm_context, EMM_CTXT_SPEC_PROC_ATTACH_ACCEPT_SENT)) &&
             (!emm_ctx_is_specific_procedure_running (&ue_mm_context->emm_context, EMM_CTXT_SPEC_PROC_ATTACH_REJECT_SENT))) {

    ue_mm_context->emm_context.num_attach_request++;

    if (_emm_attach_have_changed(&ue_mm_context->emm_context, type, ksi, guti, imsi, imei, ue_network_capability, ms_network_capability)) {
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Attach parameters have changed\n");
      REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_e__1);
      /*
       * If one or more of the information elements in the ATTACH REQUEST message differs from the ones
       * received within the previous ATTACH REQUEST message, the previously initiated attach procedure shall
       * be aborted and the new attach procedure shall be executed;
       */
      emm_sap_t                               emm_sap = {0};

      emm_sap.primitive = EMMREG_PROC_ABORT;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx = &ue_mm_context->emm_context;
      rc = emm_sap_send (&emm_sap);


      if (RETURNerror == rc) {
        emm_ctx_unmark_specific_procedure_running(&ue_mm_context->emm_context, EMM_CTXT_SPEC_PROC_ATTACH_REJECT_SENT);
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - sent of ATTACH REJECT failed\n");
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
      } else {
        emm_ctx_mark_specific_procedure_running (&ue_mm_context->emm_context, EMM_CTXT_SPEC_PROC_ATTACH_REJECT_SENT);
      }
      if (duplicate_enb_context_detected) {
        ue_mm_context = mme_api_duplicate_enb_ue_s1ap_id_detected (enb_ue_s1ap_id_key,ue_mm_context->mme_ue_s1ap_id, REMOVE_NEW_CONTEXT);
      }
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
  } else { //else  ((ue_mm_context) && ((EMM_DEREGISTERED < fsm_state ) && (EMM_REGISTERED != fsm_state)))
    if (!ue_mm_context) {
      create_new_emm_ctxt = true;
    } else {
      ue_mm_context->emm_context.num_attach_request++;
      if (duplicate_enb_context_detected) {
        ue_mm_context = mme_api_duplicate_enb_ue_s1ap_id_detected (enb_ue_s1ap_id_key,ue_mm_context->mme_ue_s1ap_id,REMOVE_OLD_CONTEXT);
      }
    }
  }

  if (create_new_emm_ctxt) {
    AssertFatal(0, "Should not go here now");
//    /*
//     * Create UE's EMM context
//     */
//    ue_mm_context = (ue_mm_context_t *) calloc (1, sizeof (ue_mm_context_t));
//
//    if (!ue_mm_context) {
//      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to create EMM context\n");
//      ue_ctx.emm_context.emm_cause = EMM_CAUSE_ILLEGAL_UE;
//      /*
//       * Do not accept the UE to attach to the network
//       */
//      rc = _emm_attach_reject (&ue_ctx);
//      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
//    }
//    ue_mm_context->mme_ue_s1ap_id = emm_ctx_get_new_ue_id(&ue_mm_context->emm_context);
//    ue_id = ue_mm_context->mme_ue_s1ap_id;
//    OAILOG_NOTICE (LOG_NAS_EMM, "EMM-PROC  - Create EMM context ue_id = " MME_UE_S1AP_ID_FMT "\n", ue_id);
//    emm_context->is_dynamic = true;
//    bdestroy_wrapper (&emm_context->esm_msg);
//    emm_context->emm_cause       = EMM_CAUSE_SUCCESS;
//    emm_context->_emm_fsm_status = EMM_INVALID;
//    emm_context->T3450.id        = NAS_TIMER_INACTIVE_ID;
//    emm_context->T3450.sec       = mme_config.nas_config.t3450_sec;
//    emm_context->T3460.id        = NAS_TIMER_INACTIVE_ID;
//    emm_context->T3460.sec       = mme_config.nas_config.t3460_sec;
//    emm_context->T3470.id        = NAS_TIMER_INACTIVE_ID;
//    emm_context->T3470.sec       = mme_config.nas_config.t3470_sec;
//    emm_fsm_set_status (ue_id, &ue_mm_context->emm_context, EMM_DEREGISTERED);
//
//    emm_ctx_clear_guti(&ue_mm_context->emm_context);
//    emm_ctx_clear_old_guti(&ue_mm_context->emm_context);
//    emm_ctx_clear_imsi(&ue_mm_context->emm_context);
//    emm_ctx_clear_imei(&ue_mm_context->emm_context);
//    emm_ctx_clear_imeisv(&ue_mm_context->emm_context);
//    emm_ctx_clear_lvr_tai(&ue_mm_context->emm_context);
//    emm_ctx_clear_security(&ue_mm_context->emm_context);
//    emm_ctx_clear_non_current_security(&ue_mm_context->emm_context);
//    emm_ctx_clear_auth_vectors(&ue_mm_context->emm_context);
//    emm_ctx_clear_ms_nw_cap(&ue_mm_context->emm_context);
//    emm_ctx_clear_ue_nw_cap_ie(&ue_mm_context->emm_context);
//    emm_ctx_clear_current_drx_parameter(&ue_mm_context->emm_context);
//    emm_ctx_clear_pending_current_drx_parameter(&ue_mm_context->emm_context);
//    emm_ctx_clear_eps_bearer_context_status(&ue_mm_context->emm_context);
//
//    mme_api_notified_new_ue_s1ap_id_association (ue_mm_context->enb_ue_s1ap_id, originating_ecgi->cell_identity.enb_id, ue_mm_context->mme_ue_s1ap_id);
  }


  if (!ue_mm_context->emm_context.specific_proc) {
    ue_mm_context->emm_context.specific_proc = (emm_specific_procedure_data_t *) calloc (1, sizeof (*ue_mm_context->emm_context.specific_proc));

    if (ue_mm_context->emm_context.specific_proc ) {
      /*
       * Setup ongoing EMM procedure callback functions
       */
      rc = emm_proc_specific_initialize (&ue_mm_context->emm_context, EMM_SPECIFIC_PROC_TYPE_ATTACH, ue_mm_context->emm_context.specific_proc, NULL, NULL, _emm_attach_abort);
      emm_ctx_mark_specific_procedure_running (&ue_mm_context->emm_context, EMM_CTXT_SPEC_PROC_ATTACH);
      ue_mm_context->emm_context.specific_proc->arg.u.attach_data.ue_id = ue_id;

      if (rc != RETURNok) {
        OAILOG_WARNING (LOG_NAS_EMM, "Failed to initialize EMM callback functions");
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
      }
    }
  }

  if (last_visited_registered_tai) {
    emm_ctx_set_valid_lvr_tai(&ue_mm_context->emm_context, last_visited_registered_tai);
  } else {
    emm_ctx_clear_lvr_tai(&ue_mm_context->emm_context);
  }

  /*
   * Update the EMM context with the current attach procedure parameters
   */
  rc = _emm_attach_update (&ue_mm_context->emm_context, ue_id, type, ksi, is_native_guti, guti, imsi, imei, originating_tai,
      ue_network_capability, ms_network_capability, esm_msg_pP);

  if (rc != RETURNok) {
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to update EMM context\n");
    /*
     * Do not accept the UE to attach to the network
     */
    ue_mm_context->emm_context.emm_cause = EMM_CAUSE_ILLEGAL_UE;
    rc = _emm_attach_reject (&ue_mm_context->emm_context);
  } else {
    /*
     * Performs the sequence: UE identification, authentication, security mode
     */
    rc = _emm_attach_identify (&ue_mm_context->emm_context);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
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
  emm_context_t                          *emm_context = (emm_context_t *) (args);

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
static int _emm_attach_release (emm_context_t *emm_context)
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
int _emm_attach_reject (emm_context_t *emm_context)
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
static int _emm_attach_abort (emm_context_t *emm_context)
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
    /*
     * Stop timer T3450
     */
    if (emm_context->T3450.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3450 (%lx)\n", emm_context->T3450.id);
      emm_context->T3450.id = nas_timer_stop (emm_context->T3450.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
    }

    /*
     * Release retransmission timer parameters
     */
    bdestroy_wrapper (&data->esm_msg);
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
static int _emm_attach_identify (emm_context_t *emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  REQUIREMENT_3GPP_24_301(R10_5_5_1_2_3__1);
  mme_ue_s1ap_id_t                        ue_id = PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context)->mme_ue_s1ap_id;
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
      visited_plmn.mcc_digit1 = emm_context->originating_tai.mcc_digit1;
      visited_plmn.mcc_digit2 = emm_context->originating_tai.mcc_digit2;
      visited_plmn.mcc_digit3 = emm_context->originating_tai.mcc_digit3;
      visited_plmn.mnc_digit1 = emm_context->originating_tai.mnc_digit1;
      visited_plmn.mnc_digit2 = emm_context->originating_tai.mnc_digit2;
      visited_plmn.mnc_digit3 = emm_context->originating_tai.mnc_digit3;

      nas_itti_auth_info_req (ue_id, emm_context->_imsi64, true, &visited_plmn, MAX_EPS_AUTH_VECTORS, NULL);
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
static int _emm_attach_security (emm_context_t *emm_context)
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
static int _emm_attach (emm_context_t *emm_context)
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

int emm_cn_wrapper_attach_accept (emm_context_t * emm_context)
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
static int _emm_attach_accept (emm_context_t * emm_context)
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
  const emm_context_t * emm_context,
  emm_proc_attach_type_t type,
  ksi_t ksi,
  guti_t * guti,
  imsi_t * imsi,
  imei_t * imei,
  const ue_network_capability_t * const ue_network_capability,
  const ms_network_capability_t * const ms_network_capability)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  mme_ue_s1ap_id_t                        ue_id = PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context)->mme_ue_s1ap_id;

  /*
   * Emergency bearer services indicator
   */
  if ((type == EMM_ATTACH_TYPE_EMERGENCY) != emm_context->is_emergency) {
    OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: type EMM_ATTACH_TYPE_EMERGENCY \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  /*
   * Security key set identifier
   */
  if (ksi != emm_context->ue_ksi) {
    OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: ue_ksi %d -> %d \n", ue_id, emm_context->ue_ksi, ksi);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  /*
   * Supported EPS encryption algorithms
   */
  if (memcmp(ue_network_capability, &emm_context->_ue_network_capability, sizeof(emm_context->_ue_network_capability))) {
    OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: UE network capabilities \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  // optional IE
  if (ms_network_capability) {
    if (IS_EMM_CTXT_PRESENT_MS_NETWORK_CAPABILITY(emm_context)) {
      if (memcmp(ms_network_capability, &emm_context->_ms_network_capability, sizeof(emm_context->_ms_network_capability))) {
        OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: MS network capabilities \n", ue_id);
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
      }
    } else {
      OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: MS network capabilities \n", ue_id);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  } else {
    if (IS_EMM_CTXT_PRESENT_MS_NETWORK_CAPABILITY(emm_context)) {
      OAILOG_DEBUG (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed: MS network capabilities \n", ue_id);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }


  /*
   * The GUTI if provided by the UE
   */
  if ((guti) && (!IS_EMM_CTXT_PRESENT_OLD_GUTI(emm_context))) {
    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed:  guti None ->  " GUTI_FMT "\n", ue_id, GUTI_ARG(guti));
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((!guti) && (IS_EMM_CTXT_PRESENT_GUTI(emm_context))) {
    OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed:  guti " GUTI_FMT " -> None\n", ue_id, GUTI_ARG(&emm_context->_guti));
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((guti) && (IS_EMM_CTXT_PRESENT_GUTI(emm_context))) {
    if (guti->m_tmsi != emm_context->_guti.m_tmsi) {
      OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed:  guti/tmsi " GUTI_FMT " -> " GUTI_FMT "\n",
          ue_id, GUTI_ARG(&emm_context->_guti), GUTI_ARG(guti));
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
    if ((guti->gummei.mme_code != emm_context->_guti.gummei.mme_code) ||
        (guti->gummei.mme_gid != emm_context->_guti.gummei.mme_gid) ||
        (guti->gummei.plmn.mcc_digit1 != emm_context->_guti.gummei.plmn.mcc_digit1) ||
        (guti->gummei.plmn.mcc_digit2 != emm_context->_guti.gummei.plmn.mcc_digit2) ||
        (guti->gummei.plmn.mcc_digit3 != emm_context->_guti.gummei.plmn.mcc_digit3) ||
        (guti->gummei.plmn.mnc_digit1 != emm_context->_guti.gummei.plmn.mnc_digit1) ||
        (guti->gummei.plmn.mnc_digit2 != emm_context->_guti.gummei.plmn.mnc_digit2) ||
        (guti->gummei.plmn.mnc_digit3 != emm_context->_guti.gummei.plmn.mnc_digit3)) {
      OAILOG_INFO (LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT "  attach changed:  guti/tmsi " GUTI_FMT " -> " GUTI_FMT "\n",
          ue_id, GUTI_ARG(&emm_context->_guti), GUTI_ARG(guti));
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  /*
   * The IMSI if provided by the UE
   */
  if ((imsi) && (!IS_EMM_CTXT_VALID_IMSI(emm_context))) {
    char                                    imsi_str[16];

    IMSI_TO_STRING (imsi, imsi_str, 16);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imsi %s/NULL (ctxt)\n", imsi_str);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((!imsi) && (IS_EMM_CTXT_VALID_IMSI(emm_context))) {
    char                                    imsi_str[16];

    IMSI_TO_STRING (&emm_context->_imsi, imsi_str, 16);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imsi NULL/%s (ctxt)\n", imsi_str);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((imsi) && (IS_EMM_CTXT_VALID_IMSI(emm_context))) {
    if (memcmp (imsi, &emm_context->_imsi, sizeof (emm_context->_imsi)) != 0) {
      char                                    imsi_str[16];
      char                                    imsi2_str[16];

      IMSI_TO_STRING (imsi, imsi_str, 16);
      IMSI_TO_STRING (&emm_context->_imsi, imsi2_str, 16);
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imsi %s/%s (ctxt)\n", imsi_str, imsi2_str);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  /*
   * The IMEI if provided by the UE
   */
  if ((imei) && (!IS_EMM_CTXT_VALID_IMEI(emm_context))) {
    char                                    imei_str[16];

    IMEI_TO_STRING (imei, imei_str, 16);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imei %s/NULL (ctxt)\n", imei_str);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((!imei) && (IS_EMM_CTXT_VALID_IMEI(emm_context))) {
    char                                    imei_str[16];

    IMEI_TO_STRING (&emm_context->_imei, imei_str, 16);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imei NULL/%s (ctxt)\n", imei_str);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((imei) && (IS_EMM_CTXT_VALID_IMEI(emm_context))) {
    if (memcmp (imei, &emm_context->_imei, sizeof (emm_context->_imei)) != 0) {
      char                                    imei_str[16];
      char                                    imei2_str[16];

      IMEI_TO_STRING (imei, imei_str, 16);
      IMEI_TO_STRING (&emm_context->_imei, imei2_str, 16);
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
  emm_context_t * emm_context,
  mme_ue_s1ap_id_t ue_id,
  emm_proc_attach_type_t type,
  ksi_t ksi,
  bool is_native_guti,
  guti_t * guti,
  imsi_t * imsi,
  imei_t * imei,
  const tai_t   * const originating_tai,
  const ue_network_capability_t * const ue_network_capability,
  const ms_network_capability_t * const ms_network_capability,
  const_bstring esm_msg_pP)
{

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  ue_mm_context_t *ue_mm_context = PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context);
  /*
   * UE identifier
   */
  ue_mm_context->mme_ue_s1ap_id = ue_id;
  /*
   * Emergency bearer services indicator
   */
  emm_context->is_emergency = (type == EMM_ATTACH_TYPE_EMERGENCY);
  /*
   * Security key set identifier
   */
  OAILOG_TRACE (LOG_NAS_EMM, "UE id " MME_UE_S1AP_ID_FMT " Update ksi %d -> %d\n", ue_mm_context->mme_ue_s1ap_id, emm_context->ue_ksi, ksi);
  emm_context->ue_ksi = ksi;
  /*
   * Supported EPS encryption algorithms
   */
  if (ue_network_capability) {
    emm_ctx_set_valid_ue_nw_cap(emm_context, ue_network_capability);
  }
  if (ms_network_capability) {
    emm_ctx_set_valid_ms_nw_cap(emm_context, ms_network_capability);
  } else {
    // optional IE
    emm_ctx_clear_ms_nw_cap(emm_context);
  }

  emm_context->originating_tai = *originating_tai;

  /*
   * The GUTI if provided by the UE
   */
  if (guti) {
    if (memcmp(guti, &emm_context->_old_guti, sizeof(emm_context->_old_guti))) {
      //TODO remove previous guti entry in coll if was present
      emm_ctx_set_old_guti(emm_context, guti);
      // emm_context_add_old_guti (&_emm_data, ctx); changed into ->
      mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_mm_context ,
        ue_mm_context->enb_s1ap_id_key,
        ue_mm_context->mme_ue_s1ap_id,
        emm_context->_imsi64,
        ue_mm_context->mme_teid_s11, &emm_context->_old_guti);
    }
  }

  /*
   * The IMSI if provided by the UE
   */
  if (imsi) {
    imsi64_t new_imsi64 = imsi_to_imsi64(imsi);
    if (new_imsi64 != emm_context->_imsi64) {
      emm_ctx_set_valid_imsi(emm_context, imsi, new_imsi64);

      //emm_context_add_imsi (&_emm_data, ctx); changed into ->
      mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts, ue_mm_context ,
        ue_mm_context->enb_s1ap_id_key,
        ue_mm_context->mme_ue_s1ap_id,
        emm_context->_imsi64,
        ue_mm_context->mme_teid_s11, NULL);
    }
  }

  /*
   * The IMEI if provided by the UE
   */
  if (imei) {
    emm_ctx_set_valid_imei(emm_context, imei);
  }

  /*
   * The ESM message contained within the attach request
   */
  if (esm_msg_pP) {
    bdestroy_wrapper (&emm_context->esm_msg);
    if (!(emm_context->esm_msg = bstrcpy(esm_msg_pP))) {
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }
  }
  /*
   * Attachment indicator
   */
  emm_context->is_attached = false;
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

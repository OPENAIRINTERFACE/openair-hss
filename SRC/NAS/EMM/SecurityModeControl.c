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
  Source      SecurityModeControl.c

  Version     0.1

  Date        2013/04/22

  Product     NAS stack

  Subsystem   Template body file

  Author      Frederic Maurel

  Description Defines the security mode control EMM procedure executed by the
        Non-Access Stratum.

        The purpose of the NAS security mode control procedure is to
        take an EPS security context into use, and initialise and start
        NAS signalling security between the UE and the MME with the
        corresponding EPS NAS keys and EPS security algorithms.

        Furthermore, the network may also initiate a SECURITY MODE COM-
        MAND in order to change the NAS security algorithms for a cur-
        rent EPS security context already in use.

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
#include "3gpp_requirements_24.301.h"
#include "3gpp_24.007.h"
#include "mme_app_ue_context.h"
#include "emm_proc.h"
#include "common_defs.h"
#include "nas_timer.h"
#include "emm_data.h"
#include "emm_sap.h"
#include "emm_cause.h"
#include "secu_defs.h"
#include "mme_app_defs.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
    Internal data handled by the security mode control procedure in the UE
   --------------------------------------------------------------------------
*/

/*
   --------------------------------------------------------------------------
    Internal data handled by the security mode control procedure in the MME
   --------------------------------------------------------------------------
*/
/*
   Timer handlers
*/
static void                            *_security_t3460_handler (void *);
static int _security_ll_failure (emm_context_t *emm_context);
static int _security_non_delivered (emm_context_t *emm_context);

/*
   Function executed whenever the ongoing EMM procedure that initiated
   the security mode control procedure is aborted or the maximum value of the
   retransmission timer counter is exceed
*/
static int                              _security_abort (emm_context_t *emm_context);
static int                              _security_select_algorithms (
  const int ue_eiaP,
  const int ue_eeaP,
  int *const mme_eiaP,
  int *const mme_eeaP);



static int                              _security_request (security_data_t * data);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/


/*
   --------------------------------------------------------------------------
        Security mode control procedure executed by the MME
   --------------------------------------------------------------------------
*/


/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_security_mode_control()                          **
 **                                                                        **
 ** Description: Initiates the security mode control procedure.            **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.3.2                           **
 **      The MME initiates the NAS security mode control procedure **
 **      by sending a SECURITY MODE COMMAND message to the UE and  **
 **      starting timer T3460. The message shall be sent unciphe-  **
 **      red but shall be integrity protected using the NAS inte-  **
 **      grity key based on KASME.                                 **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      ksi:       NAS key set identifier                     **
 **      eea:       Replayed EPS encryption algorithms         **
 **      eia:       Replayed EPS integrity algorithms          **
 **      success:   Callback function executed when the secu-  **
 **             rity mode control procedure successfully   **
 **             completes                                  **
 **      reject:    Callback function executed when the secu-  **
 **             rity mode control procedure fails or is    **
 **             rejected                                   **
 **      failure:   Callback function executed whener a lower  **
 **             layer failure occured before the security  **
 **             mode control procedure completes          **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_security_mode_control (
  const mme_ue_s1ap_id_t ue_id,
  ksi_t ksi,
  emm_common_success_callback_t success,
  emm_common_reject_callback_t reject,
  emm_common_failure_callback_t failure)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  bool                                    security_context_is_new = false;
  int                                     mme_eea = NAS_SECURITY_ALGORITHMS_EEA0;
  int                                     mme_eia = NAS_SECURITY_ALGORITHMS_EIA0;
  ue_mm_context_t                        *ue_mm_context = NULL;
  emm_context_t                          *emm_ctx = NULL;
  /*
   * Get the UE context
   */

  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Initiate security mode control procedure " "KSI = %d\n", ksi);

  ue_mm_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ue_id);
  if (ue_mm_context) {
    emm_ctx = &ue_mm_context->emm_context;
  } else {
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }

  //TODO better than that (quick fixes)
  if (KSI_NO_KEY_AVAILABLE == ksi) {
    ksi = 0;
  }
  if (EMM_SECURITY_VECTOR_INDEX_INVALID == emm_ctx->_security.vector_index) {
    emm_ctx_set_security_vector_index(emm_ctx, 0);
  }
  /*
   * Allocate parameters of the retransmission timer callback
   */
  if (emm_ctx->common_proc) {
    emm_common_cleanup(&emm_ctx->common_proc);
  }
  emm_ctx->common_proc = (emm_common_data_t *) calloc (1, sizeof (*emm_ctx->common_proc));

  if ((emm_ctx) &&(emm_ctx->common_proc)) {
    // TODO check for removing test (emm_ctx->_security.sc_type == SECURITY_CTX_TYPE_NOT_AVAILABLE)
    if ((emm_ctx->_security.sc_type == SECURITY_CTX_TYPE_NOT_AVAILABLE) &&
       !(emm_ctx->common_proc_mask & EMM_CTXT_COMMON_PROC_SMC)) {

      emm_ctx->common_proc->common_arg.u.security_data.saved_selected_eea = emm_ctx->_security.selected_algorithms.encryption;
      emm_ctx->common_proc->common_arg.u.security_data.saved_selected_eia = emm_ctx->_security.selected_algorithms.integrity;
      emm_ctx->common_proc->common_arg.u.security_data.saved_eksi         = emm_ctx->_security.eksi;
      emm_ctx->common_proc->common_arg.u.security_data.saved_overflow     = emm_ctx->_security.dl_count.overflow;
      emm_ctx->common_proc->common_arg.u.security_data.saved_seq_num      = emm_ctx->_security.dl_count.seq_num;
      emm_ctx->common_proc->common_arg.u.security_data.saved_sc_type      = emm_ctx->_security.sc_type;
      /*
       * The security mode control procedure is initiated to take into use
       * * * * the EPS security context created after a successful execution of
       * * * * the EPS authentication procedure
       */
      //emm_ctx->_security.sc_type = SECURITY_CTX_TYPE_PARTIAL_NATIVE;
      emm_ctx_set_security_eksi(emm_ctx, ksi);
      REQUIREMENT_3GPP_24_301(R10_5_4_3_2__2);
      emm_ctx->_security.dl_count.overflow = 0;
      emm_ctx->_security.dl_count.seq_num = 0;


      /*
       *  Compute NAS cyphering and integrity keys
       */

      rc = _security_select_algorithms (emm_ctx->_ue_network_capability.eia, emm_ctx->_ue_network_capability.eea, &mme_eia, &mme_eea);
      emm_ctx->_security.selected_algorithms.encryption = mme_eea;
      emm_ctx->_security.selected_algorithms.integrity = mme_eia;

      if (rc == RETURNerror) {
        OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to select security algorithms\n");
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
      }

      emm_ctx_set_security_type(emm_ctx, SECURITY_CTX_TYPE_FULL_NATIVE);
      AssertFatal(EMM_SECURITY_VECTOR_INDEX_INVALID != emm_ctx->_security.vector_index, "Vector index not initialized");
      AssertFatal(MAX_EPS_AUTH_VECTORS >  emm_ctx->_security.vector_index, "Vector index outbound value %d/%d", emm_ctx->_security.vector_index, MAX_EPS_AUTH_VECTORS);
      derive_key_nas (NAS_INT_ALG, emm_ctx->_security.selected_algorithms.integrity,  emm_ctx->_vector[emm_ctx->_security.vector_index].kasme, emm_ctx->_security.knas_int);
      derive_key_nas (NAS_ENC_ALG, emm_ctx->_security.selected_algorithms.encryption, emm_ctx->_vector[emm_ctx->_security.vector_index].kasme, emm_ctx->_security.knas_enc);
      /*
       * Set new security context indicator
       */
      security_context_is_new = true;
      emm_ctx_set_attribute_present(emm_ctx, EMM_CTXT_MEMBER_SECURITY);
    }
  } else {
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - No EPS security context exists\n");
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }


  if (emm_ctx->common_proc ) {
    /*
     * Setup ongoing EMM procedure callback functions
     */
    rc = emm_proc_common_initialize (emm_ctx, EMM_COMMON_PROC_TYPE_SECURITY_MODE_CONTROL, emm_ctx->common_proc,
        success, reject, failure, _security_ll_failure, _security_non_delivered, _security_abort);

    if (rc != RETURNok) {
      OAILOG_WARNING (LOG_NAS_EMM, "Failed to initialize EMM callback functions\n");
      emm_common_cleanup (&emm_ctx->common_proc);
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }

    /*
     * Set the UE identifier
     */
    emm_ctx->common_proc->common_arg.u.security_data.ue_id = ue_id;
    /*
     * Reset the retransmission counter
     */
    emm_ctx->common_proc->common_arg.u.security_data.retransmission_count = 0;
    /*
     * Set the key set identifier
     */
    emm_ctx->common_proc->common_arg.u.security_data.ksi = ksi;
    /*
     * Set the EPS encryption algorithms to be replayed to the UE
     */
    emm_ctx->common_proc->common_arg.u.security_data.eea = emm_ctx->_ue_network_capability.eea;
    /*
     * Set the EPS integrity algorithms to be replayed to the UE
     */
    emm_ctx->common_proc->common_arg.u.security_data.eia = emm_ctx->_ue_network_capability.eia;
    emm_ctx->common_proc->common_arg.u.security_data.ucs2 = emm_ctx->_ue_network_capability.ucs2;
    /*
     * Set the UMTS encryption algorithms to be replayed to the UE
     */
    emm_ctx->common_proc->common_arg.u.security_data.uea = emm_ctx->_ue_network_capability.uea;
    /*
     * Set the UMTS integrity algorithms to be replayed to the UE
     */
    emm_ctx->common_proc->common_arg.u.security_data.uia = emm_ctx->_ue_network_capability.uia;
    /*
     * Set the GPRS integrity algorithms to be replayed to the UE
     */

    uint8_t gea = emm_ctx->_ms_network_capability.gea1;
    if (gea) {
      gea = (gea << 6) | emm_ctx->_ms_network_capability.egea;
    }
    emm_ctx->common_proc->common_arg.u.security_data.gea = gea;
    emm_ctx->common_proc->common_arg.u.security_data.umts_present = emm_ctx->_ue_network_capability.umts_present;
    emm_ctx->common_proc->common_arg.u.security_data.gprs_present = (gea >= (MS_NETWORK_CAPABILITY_GEA1 >> 1));
    /*
     * Set the EPS encryption algorithms selected to the UE
     */
    emm_ctx->common_proc->common_arg.u.security_data.selected_eea = emm_ctx->_security.selected_algorithms.encryption;
    /*
     * Set the EPS integrity algorithms selected to the UE
     */
    emm_ctx->common_proc->common_arg.u.security_data.selected_eia = emm_ctx->_security.selected_algorithms.integrity;

    emm_ctx->common_proc->common_arg.u.security_data.is_new = security_context_is_new;

    /*
     * Send security mode command message to the UE
     */
    rc = _security_request (&emm_ctx->common_proc->common_arg.u.security_data);

    if (rc != RETURNerror) {
      emm_ctx_mark_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_SMC);
      /*
       * Notify EMM that common procedure has been initiated
       */
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_REQ ue id " MME_UE_S1AP_ID_FMT " (security mode control)",ue_id);
      emm_sap_t                               emm_sap = {0};

      emm_sap.primitive = EMMREG_COMMON_PROC_REQ;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx = emm_ctx;
      rc = emm_sap_send (&emm_sap);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_security_mode_complete()                         **
 **                                                                        **
 ** Description: Performs the security mode control completion procedure   **
 **      executed by the network.                                  **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.3.4                           **
 **      Upon receiving the SECURITY MODE COMPLETE message, the    **
 **      MME shall stop timer T3460.                               **
 **      From this time onward the MME shall integrity protect and **
 **      encipher all signalling messages with the selected NAS    **
 **      integrity and ciphering algorithms.                       **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_security_mode_complete (
  mme_ue_s1ap_id_t ue_id)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  ue_mm_context_t                        *ue_mm_context = NULL;
  emm_context_t                          *emm_ctx = NULL;
  int                                     rc = RETURNerror;
  emm_sap_t                               emm_sap = {0};

  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Security mode complete (ue_id=" MME_UE_S1AP_ID_FMT ")\n", ue_id);
  /*
   * Get the UE context
   */

  ue_mm_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ue_id);
  if (ue_mm_context) {
    emm_ctx = &ue_mm_context->emm_context;
  } else {
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }


  if (emm_ctx) {
    /*
     * Stop timer T3460
     */
    REQUIREMENT_3GPP_24_301(R10_5_4_3_4__1);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3460 (%lx)\n", emm_ctx->T3460.id);
    emm_ctx->T3460.id = nas_timer_stop (emm_ctx->T3460.id);
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3460 stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
  }

  /*
   * Release retransmission timer parameters
   */

  if (emm_ctx && IS_EMM_CTXT_PRESENT_SECURITY(emm_ctx)) {
    /*
     * Notify EMM that the authentication procedure successfully completed
     */
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_CNF ue id " MME_UE_S1AP_ID_FMT " (security mode complete)", ue_id);
    emm_sap.primitive = EMMREG_COMMON_PROC_CNF;
    emm_sap.u.emm_reg.ue_id = ue_id;
    emm_sap.u.emm_reg.ctx = emm_ctx;
    emm_sap.u.emm_reg.u.common.is_attached = emm_ctx->is_attached;
    REQUIREMENT_3GPP_24_301(R10_5_4_3_4__2);
    emm_ctx_set_attribute_valid(emm_ctx, EMM_CTXT_MEMBER_SECURITY);
  } else {
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - No EPS security context exists\n");
    /*
     * Notify EMM that the authentication procedure failed
     */
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_REJ ue id " MME_UE_S1AP_ID_FMT " (security mode complete)", ue_id);
    emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
    emm_sap.u.emm_reg.ue_id = ue_id;
    emm_sap.u.emm_reg.ctx = emm_ctx;
  }

  emm_ctx_unmark_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_SMC);

  rc = emm_sap_send (&emm_sap);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_security_mode_reject()                           **
 **                                                                        **
 ** Description: Performs the security mode control not accepted by the UE **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.3.5                           **
 **      Upon receiving the SECURITY MODE REJECT message, the MME  **
 **      shall stop timer T3460 and abort the ongoing procedure    **
 **      that triggered the initiation of the NAS security mode    **
 **      control procedure.                                        **
 **      The MME shall apply the EPS security context in use befo- **
 **      re the initiation of the security mode control procedure, **
 **      if any, to protect any subsequent messages.               **
 **                                                                        **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_security_mode_reject (
  mme_ue_s1ap_id_t ue_id)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  ue_mm_context_t                        *ue_mm_context = NULL;
  emm_context_t                          *emm_ctx = NULL;
  int                                     rc = RETURNerror;

  OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Security mode command not accepted by the UE" "(ue_id=" MME_UE_S1AP_ID_FMT ")\n", ue_id);
  /*
   * Get the UE context
   */

  ue_mm_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ue_id);
  if (ue_mm_context) {
    emm_ctx = &ue_mm_context->emm_context;
  } else {
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }

  if (emm_ctx) {
    /*
     * Stop timer T3460
     */
    REQUIREMENT_3GPP_24_301(R10_5_4_3_5__2);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3460 (%lx)\n", emm_ctx->T3460.id);
    emm_ctx->T3460.id = nas_timer_stop (emm_ctx->T3460.id);
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3460 stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);

    if ((emm_ctx->common_proc) && (EMM_COMMON_PROC_TYPE_SECURITY_MODE_CONTROL == emm_ctx->common_proc->type)) {
      // restore previous values
      REQUIREMENT_3GPP_24_301(R10_5_4_3_5__3);
      emm_ctx->_security.selected_algorithms.encryption = emm_ctx->common_proc->common_arg.u.security_data.saved_selected_eea;
      emm_ctx->_security.selected_algorithms.integrity  = emm_ctx->common_proc->common_arg.u.security_data.saved_selected_eia;
      emm_ctx_set_security_eksi(emm_ctx, emm_ctx->common_proc->common_arg.u.security_data.saved_eksi);
      emm_ctx->_security.dl_count.overflow              = emm_ctx->common_proc->common_arg.u.security_data.saved_overflow;
      emm_ctx->_security.dl_count.seq_num               = emm_ctx->common_proc->common_arg.u.security_data.saved_seq_num;
      emm_ctx_set_security_type(emm_ctx, emm_ctx->common_proc->common_arg.u.security_data.saved_sc_type);
      /*
       * Release retransmission timer parameters
       */
    }
  }


  /*
   * Notify EMM that the authentication procedure failed
   */
  MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_REJ ue id " MME_UE_S1AP_ID_FMT " (security mode reject)", ue_id);
  emm_sap_t                               emm_sap = {0};

  REQUIREMENT_3GPP_24_301(R10_5_4_3_5__2);
  emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
  emm_sap.u.emm_reg.ue_id = ue_id;
  emm_sap.u.emm_reg.ctx = emm_ctx;
  rc = emm_sap_send (&emm_sap);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
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
 ** Name:    _security_t3460_handler()                                 **
 **                                                                        **
 ** Description: T3460 timeout handler                                     **
 **      Upon T3460 timer expiration, the security mode command    **
 **      message is retransmitted and the timer restarted. When    **
 **      retransmission counter is exceed, the MME shall abort the **
 **      security mode control procedure.                          **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.3.7, case b                   **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void* _security_t3460_handler (void *args)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_context_t * emm_ctx = NULL;
  ue_mm_context_t * ue_mm_context = (ue_mm_context_t *) (args);
  if (ue_mm_context) {
    emm_ctx = &ue_mm_context->emm_context;
  }

  if (emm_ctx_is_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_SMC)){
    security_data_t                  *data = &emm_ctx->common_proc->common_arg.u.security_data;
    /*
     * Increment the retransmission counter
     */
    data->retransmission_count += 1;
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - T3460 timer expired, retransmission " "counter = %d\n", data->retransmission_count);

    if (data->retransmission_count < SECURITY_COUNTER_MAX) {
      REQUIREMENT_3GPP_24_301(R10_5_4_3_7_b__1);
      /*
       * Send security mode command message to the UE
       */
      _security_request (data);
    } else {
      REQUIREMENT_3GPP_24_301(R10_5_4_3_7_b__2);
      /*
       * Abort the security mode control procedure
       */
      _security_abort (emm_ctx);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, NULL);
}

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/

/****************************************************************************
 **                                                                        **
 ** Name:    _security_request()                                       **
 **                                                                        **
 ** Description: Sends SECURITY MODE COMMAND message and start timer T3460 **
 **                                                                        **
 ** Inputs:  data:      Security mode control internal data        **
 **      is_new:    Indicates whether a new security context   **
 **             has just been taken into use               **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3460                                      **
 **                                                                        **
 ***************************************************************************/
int _security_request (security_data_t * data)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  ue_mm_context_t                        *ue_mm_context = NULL;
  struct emm_context_s                   *emm_ctx = NULL;
  emm_sap_t                               emm_sap = {0};
  int                                     rc = RETURNerror;

  if (data) {
    /*
     * Notify EMM-AS SAP that Security Mode Command message has to be sent
     * to the UE
     */
    REQUIREMENT_3GPP_24_301(R10_5_4_3_2__14);
    emm_sap.primitive = EMMAS_SECURITY_REQ;
    emm_sap.u.emm_as.u.security.guti = NULL;
    emm_sap.u.emm_as.u.security.ue_id = data->ue_id;
    emm_sap.u.emm_as.u.security.msg_type = EMM_AS_MSG_TYPE_SMC;
    emm_sap.u.emm_as.u.security.ksi = data->ksi;
    emm_sap.u.emm_as.u.security.eea = data->eea;
    emm_sap.u.emm_as.u.security.eia = data->eia;
    emm_sap.u.emm_as.u.security.ucs2 = data->ucs2;
    emm_sap.u.emm_as.u.security.uea = data->uea;
    emm_sap.u.emm_as.u.security.uia = data->uia;
    emm_sap.u.emm_as.u.security.gea = data->gea;
    emm_sap.u.emm_as.u.security.umts_present = data->umts_present;
    emm_sap.u.emm_as.u.security.gprs_present = data->gprs_present;
    emm_sap.u.emm_as.u.security.selected_eea = data->selected_eea;
    emm_sap.u.emm_as.u.security.selected_eia = data->selected_eia;

    ue_mm_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, data->ue_id);
    if (ue_mm_context) {
      emm_ctx = &ue_mm_context->emm_context;
    } else {
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }

      /*
   * Setup EPS NAS security data
   */
    emm_as_set_security_data (&emm_sap.u.emm_as.u.security.sctx, &emm_ctx->_security, data->is_new, false);
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMAS_SECURITY_REQ ue id " MME_UE_S1AP_ID_FMT " ", data->ue_id);
    rc = emm_sap_send (&emm_sap);

    if (rc != RETURNerror) {
      REQUIREMENT_3GPP_24_301(R10_5_4_3_2__1);
      if (emm_ctx->T3460.id != NAS_TIMER_INACTIVE_ID) {
        /*
         * Re-start T3460 timer
         */
        emm_ctx->T3460.id = nas_timer_stop (emm_ctx->T3460.id);
        OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stopped Timer T3460 (%lx) expires in %ld seconds\n", emm_ctx->T3460.id, emm_ctx->T3460.sec);
        MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3460 Stopped UE " MME_UE_S1AP_ID_FMT " ", data->ue_id);
        AssertFatal(NAS_TIMER_INACTIVE_ID != emm_ctx->T3460.id, "Failed to stop T3460");
      }
      /*
       * Start T3460 timer
       */
      emm_ctx->T3460.id = nas_timer_start (emm_ctx->T3460.sec, 0  /*usec*/, _security_t3460_handler, ue_mm_context);
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Started Timer T3460 (%lx) expires in %ld seconds\n", emm_ctx->T3460.id, emm_ctx->T3460.sec);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3460 started UE " MME_UE_S1AP_ID_FMT " ", data->ue_id);
      AssertFatal(NAS_TIMER_INACTIVE_ID != emm_ctx->T3460.id, "Failed to start T3460");
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _security_ll_failure (emm_context_t * emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  if (emm_ctx_is_common_procedure_running(emm_context, EMM_CTXT_COMMON_PROC_SMC)){
    security_data_t                  *data = &emm_context->common_proc->common_arg.u.security_data;
    REQUIREMENT_3GPP_24_301(R10_5_4_3_7_a);
    emm_sap_t                               emm_sap = {0};
    mme_ue_s1ap_id_t                        ue_id = PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context)->mme_ue_s1ap_id;

    emm_sap.primitive = EMMREG_PROC_ABORT;
    emm_sap.u.emm_reg.ue_id = ue_id;
    emm_sap.u.emm_reg.ctx   = emm_context;
    data->notify_failure = true;
    rc = emm_sap_send (&emm_sap);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}


//------------------------------------------------------------------------------
static int _security_non_delivered (emm_context_t * emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  if (emm_ctx_is_common_procedure_running(emm_context, EMM_CTXT_COMMON_PROC_SMC)){
    security_data_t                  *data = &emm_context->common_proc->common_arg.u.security_data;
    REQUIREMENT_3GPP_24_301(R10_5_4_3_7_e);
    data->is_new = false;
    rc = _security_request(data);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _security_abort()                                         **
 **                                                                        **
 ** Description: Aborts the security mode control procedure currently in   **
 **      progress                                                  **
 **                                                                        **
 ** Inputs:  args:      Security mode control data to be released  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3460                                      **
 **                                                                        **
 ***************************************************************************/
static int _security_abort (emm_context_t * emm_context)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  if (emm_ctx_is_common_procedure_running(emm_context, EMM_CTXT_COMMON_PROC_SMC)){
    security_data_t                  *data = &emm_context->common_proc->common_arg.u.security_data;
    mme_ue_s1ap_id_t                  ue_id = data->ue_id;

    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Abort security mode control procedure " "(ue_id=" MME_UE_S1AP_ID_FMT ")\n", ue_id);

    {
      emm_ctx_unmark_common_procedure_running(emm_context, EMM_CTXT_COMMON_PROC_SMC);

      /*
       * Stop timer T3460
       */
      if (emm_context->T3460.id != NAS_TIMER_INACTIVE_ID) {
        OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3460 (%lx)\n", emm_context->T3460.id);
        emm_context->T3460.id = nas_timer_stop (emm_context->T3460.id);
        MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3460 stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);
      }
    }

    if (data->notify_failure) {
      /*
       * Notify EMM that the security mode control procedure failed
       */
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_REJ ue id " MME_UE_S1AP_ID_FMT " (security abort)", data->ue_id);
      emm_sap_t                               emm_sap = {0};

      emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx   = emm_context;
      rc = emm_sap_send (&emm_sap);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}


/****************************************************************************
 **                                                                        **
 ** Name:    _security_select_algorithms()                                 **
 **                                                                        **
 ** Description: Select int and enc algorithms based on UE capabilities and**
 **      MME capabilities and MME preferences                              **
 **                                                                        **
 ** Inputs:  ue_eia:      integrity algorithms supported by UE             **
 **          ue_eea:      ciphering algorithms supported by UE             **
 **                                                                        **
 ** Outputs: mme_eia:     integrity algorithms supported by MME            **
 **          mme_eea:     ciphering algorithms supported by MME            **
 **                                                                        **
 **      Return:    RETURNok, RETURNerror                                  **
 **      Others:    None                                                   **
 **                                                                        **
 ***************************************************************************/
static int
_security_select_algorithms (
  const int ue_eiaP,
  const int ue_eeaP,
  int *const mme_eiaP,
  int *const mme_eeaP)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     preference_index;

  *mme_eiaP = NAS_SECURITY_ALGORITHMS_EIA0;
  *mme_eeaP = NAS_SECURITY_ALGORITHMS_EEA0;

  for (preference_index = 0; preference_index < 8; preference_index++) {
    if (ue_eiaP & (0x80 >> _emm_data.conf.prefered_integrity_algorithm[preference_index])) {
      OAILOG_DEBUG (LOG_NAS_EMM, "Selected  NAS_SECURITY_ALGORITHMS_EIA%d (choice num %d)\n", _emm_data.conf.prefered_integrity_algorithm[preference_index], preference_index);
      *mme_eiaP = _emm_data.conf.prefered_integrity_algorithm[preference_index];
      break;
    }
  }

  for (preference_index = 0; preference_index < 8; preference_index++) {
    if (ue_eeaP & (0x80 >> _emm_data.conf.prefered_ciphering_algorithm[preference_index])) {
      OAILOG_DEBUG (LOG_NAS_EMM, "Selected  NAS_SECURITY_ALGORITHMS_EEA%d (choice num %d)\n", _emm_data.conf.prefered_ciphering_algorithm[preference_index], preference_index);
      *mme_eeaP = _emm_data.conf.prefered_ciphering_algorithm[preference_index];
      break;
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

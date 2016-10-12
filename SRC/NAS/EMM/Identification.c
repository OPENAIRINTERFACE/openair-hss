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
  Source      Identification.c

  Version     0.1

  Date        2013/04/09

  Product     NAS stack

  Subsystem   EPS Mobility Management

  Author      Frederic Maurel

  Description Defines the identification EMM procedure executed by the
        Non-Access Stratum.

        The identification procedure is used by the network to request
        a particular UE to provide specific identification parameters
        (IMSI, IMEI).

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
#include "msc.h"
#include "common_defs.h"
#include "nas_timer.h"
#include "3gpp_requirements_24.301.h"
#include "emm_proc.h"
#include "emm_data.h"
#include "emm_sap.h"
#include "conversions.h"


/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* String representation of the requested identity type */
static const char                      *_emm_identity_type_str[] = {
  "NOT AVAILABLE", "IMSI", "IMEI", "IMEISV", "TMSI"
};


/*
   --------------------------------------------------------------------------
    Internal data handled by the identification procedure in the MME
   --------------------------------------------------------------------------
*/
/*
   Timer handlers
*/
static void *_identification_t3470_handler (void *);


static int _identification_ll_failure (emm_context_t *emm_ctx);
static int _identification_non_delivered_ho (emm_context_t *emm_ctx);

/*
   Function executed whenever the ongoing EMM procedure that initiated
   the identification procedure is aborted or the maximum value of the
   retransmission timer counter is exceed
*/
static int _identification_abort (emm_context_t *emm_ctx);


static int _identification_request (identification_data_t * data);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/


/*
   --------------------------------------------------------------------------
        Identification procedure executed by the MME
   --------------------------------------------------------------------------
*/
/********************************************************************
 **                                                                **
 ** Name:    emm_proc_identification()                             **
 **                                                                **
 ** Description: Initiates an identification procedure.            **
 **                                                                **
 **              3GPP TS 24.301, section 5.4.4.2                   **
 **      The network initiates the identification procedure by     **
 **      sending an IDENTITY REQUEST message to the UE and star-   **
 **      ting the timer T3470. The IDENTITY REQUEST message speci- **
 **      fies the requested identification parameters in the Iden- **
 **      tity type information element.                            **
 **                                                                **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      type:      Type of the requested identity                 **
 **      success:   Callback function executed when the identi-    **
 **             fication procedure successfully completes          **
 **      reject:    Callback function executed when the identi-    **
 **             fication procedure fails or is rejected            **
 **      failure:   Callback function executed whener a lower      **
 **             layer failure occured before the identifi-         **
 **             cation procedure completes                         **
 **      Others:    None                                           **
 **                                                                **
 ** Outputs:     None                                              **
 **      Return:    RETURNok, RETURNerror                          **
 **      Others:    _emm_data                                      **
 **                                                                **
 ********************************************************************/
int
emm_proc_identification (
  emm_context_t * emm_ctx,
  emm_proc_identity_type_t type,
  emm_common_success_callback_t success,
  emm_common_reject_callback_t reject,
  emm_common_failure_callback_t failure)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  if (emm_ctx) {
    REQUIREMENT_3GPP_24_301(R10_5_4_4_1);

    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Initiate identification type = %s (%d), ctx = %p\n", _emm_identity_type_str[type], type, emm_ctx);
    /*
     * Allocate parameters of the retransmission timer callback
     */
    if (emm_ctx->common_proc) {
      emm_common_cleanup(&emm_ctx->common_proc);
    }
    emm_ctx->common_proc = (emm_common_data_t *) calloc (1, sizeof (*emm_ctx->common_proc));

    if (emm_ctx->common_proc) {
      /*
       * Setup ongoing EMM procedure callback functions
       */
      rc = emm_proc_common_initialize (emm_ctx, EMM_COMMON_PROC_TYPE_IDENTIFICATION, emm_ctx->common_proc,
          success, reject, failure, _identification_ll_failure, _identification_non_delivered_ho, _identification_abort);

      if (rc != RETURNok) {
        OAILOG_WARNING (LOG_NAS_EMM, "Failed to initialize EMM callback functions\n");
        free_wrapper ((void**)&emm_ctx->common_proc);
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
      }

      /*
       * Set the UE identifier
       */
      emm_ctx->common_proc->common_arg.u.identification_data.ue_id = emm_ctx->ue_id;
      /*
       * Reset the retransmission counter
       */
      emm_ctx->common_proc->common_arg.u.identification_data.retransmission_count = 0;
      /*
       * Set the type of the requested identity
       */
      emm_ctx->common_proc->common_arg.u.identification_data.type = type;
      /*
       * Set the failure notification indicator
       */
      emm_ctx->common_proc->common_arg.u.identification_data.notify_failure = false;
      /*
       * Send identity request message to the UE
       */
      rc = _identification_request (&emm_ctx->common_proc->common_arg.u.identification_data);

      if (rc != RETURNerror) {
        /*
         * Notify EMM that common procedure has been initiated
         */
        MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_REQ ue id " MME_UE_S1AP_ID_FMT " (identification)", emm_ctx->ue_id);
        emm_sap_t                               emm_sap = {0};

        emm_sap.primitive = EMMREG_COMMON_PROC_REQ;
        emm_sap.u.emm_reg.ue_id = emm_ctx->ue_id;
        emm_sap.u.emm_reg.ctx = emm_ctx;
        rc = emm_sap_send (&emm_sap);
      }
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_identification_complete()                            **
 **                                                                        **
 ** Description: Performs the identification completion procedure executed **
 **      by the network.                                                   **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.4.4                           **
 **      Upon receiving the IDENTITY RESPONSE message, the MME             **
 **      shall stop timer T3470.                                           **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                          **
 **      imsi:      The IMSI received from the UE                          **
 **      imei:      The IMEI received from the UE                          **
 **      tmsi:      The TMSI received from the UE                          **
 **      Others:    None                                                   **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                                  **
 **      Others:    _emm_data, T3470                                       **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_identification_complete (
  const mme_ue_s1ap_id_t ue_id,
  imsi_t   * const imsi,
  imei_t   * const imei,
  imeisv_t * const imeisv,
  uint32_t * const tmsi)
{
  int                                     rc = RETURNerror;
  emm_sap_t                               emm_sap = {0};
  emm_context_t                     *emm_ctx = NULL;

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Identification complete (ue_id=" MME_UE_S1AP_ID_FMT ")\n", ue_id);
  /*
   * Release retransmission timer parameters
   */
  //identification_data_t                  *data = (identification_data_t *) (emm_proc_common_get_args (ue_id));

  // Get the UE context
  emm_ctx = emm_context_get (&_emm_data, ue_id);

  if (emm_ctx) {
    REQUIREMENT_3GPP_24_301(R10_5_4_4_4);
    /*
     * Stop timer T3470
     */
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3470 (%ld)\n", emm_ctx->T3470.id);
    emm_ctx->T3470.id = nas_timer_stop (emm_ctx->T3470.id);

    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3470 stopped UE " MME_UE_S1AP_ID_FMT " ", ue_id);

    if (imsi) {
      /*
       * Update the IMSI
       */
      imsi64_t imsi64 = INVALID_IMSI64;
      IMSI_TO_IMSI64(imsi,imsi64);
      emm_ctx_set_valid_imsi(emm_ctx, imsi, imsi64);
    } else if (imei) {
      /*
       * Update the IMEI
       */
      emm_ctx_set_valid_imei(emm_ctx, imei);
    } else if (imeisv) {
      /*
       * Update the IMEISV
       */
      emm_ctx_set_valid_imeisv(emm_ctx, imeisv);
    } else if (tmsi) {
      /*
       * Update the GUTI
       */
      AssertFatal(false, "TODO, should not happen because this type of identity is not requested by MME");
    }

    /*
     * Notify EMM that the identification procedure successfully completed
     */
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_CNF ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    emm_sap.primitive = EMMREG_COMMON_PROC_CNF;
    emm_sap.u.emm_reg.ue_id = ue_id;
    emm_sap.u.emm_reg.ctx = emm_ctx;
    emm_sap.u.emm_reg.u.common.is_attached = emm_ctx->is_attached;
    emm_ctx_unmark_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_IDENT);
  } else {
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - No EMM context exists\n");
    /*
     * Notify EMM that the identification procedure failed
     */
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_REJ ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
    emm_sap.u.emm_reg.ue_id = ue_id;
    emm_sap.u.emm_reg.ctx = emm_ctx;
  }

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
 ** Name:    _identification_t3470_handler()                           **
 **                                                                        **
 ** Description: T3470 timeout handler                                     **
 **      Upon T3470 timer expiration, the identification request   **
 **      message is retransmitted and the timer restarted. When    **
 **      retransmission counter is exceed, the MME shall abort the **
 **      identification procedure and any ongoing EMM procedure.   **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.4.6, case b                   **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void *_identification_t3470_handler (void *args)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_context_t                     *emm_ctx = (emm_context_t*)args;

  if (emm_ctx) {
    OAILOG_WARNING (LOG_NAS_EMM, "T3470 timer (%ld) expired ue id " MME_UE_S1AP_ID_FMT " \n",
        emm_ctx->T3470.id, emm_ctx->ue_id);
    emm_ctx->T3470.id = NAS_TIMER_INACTIVE_ID;
  }
  if (emm_ctx_is_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_IDENT)){
    identification_data_t                  *data = &emm_ctx->common_proc->common_arg.u.identification_data;

    /*
     * Increment the retransmission counter
     */
    data->retransmission_count += 1;
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - T3470 (%ld) retransmission counter = %d ue id " MME_UE_S1AP_ID_FMT " \n",
        emm_ctx->T3470.id, data->retransmission_count, emm_ctx->ue_id);

    if (data->retransmission_count < IDENTIFICATION_COUNTER_MAX) {
      REQUIREMENT_3GPP_24_301(R10_5_4_4_6_b__1);
      /*
       * Send identity request message to the UE
       */
      _identification_request (data);
    } else {

      /*
      * Abort the identification procedure
      */
      REQUIREMENT_3GPP_24_301(R10_5_4_4_6_b__2);
      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMREG_PROC_ABORT;
      emm_sap.u.emm_reg.ue_id = emm_ctx->ue_id;
      emm_sap.u.emm_reg.ctx   = emm_ctx;
      data->notify_failure = true;
      emm_sap_send (&emm_sap);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, NULL);
}

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/

/*
 * Description: Sends IDENTITY REQUEST message and start timer T3470.
 *
 * Inputs:  args:      handler parameters
 *      Others:    None
 *
 * Outputs:     None
 *      Return:    None
 *      Others:    T3470
 */
int _identification_request (identification_data_t * data)
{
  emm_sap_t                          emm_sap = {0};
  int                                     rc = RETURNok;
  struct emm_context_s              *emm_ctx = NULL;

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  /*
   * Notify EMM-AS SAP that Identity Request message has to be sent
   * to the UE
   */
  MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMAS_SECURITY_REQ ue id " MME_UE_S1AP_ID_FMT " IDENTIFICATION", data->ue_id);
  emm_sap.primitive = EMMAS_SECURITY_REQ;
  emm_sap.u.emm_as.u.security.guti = NULL;
  emm_sap.u.emm_as.u.security.ue_id = data->ue_id;
  emm_sap.u.emm_as.u.security.msg_type = EMM_AS_MSG_TYPE_IDENT;
  emm_sap.u.emm_as.u.security.ident_type = data->type;

  emm_ctx = emm_context_get (&_emm_data, data->ue_id);

  /*
   * Setup EPS NAS security data
   */
  emm_as_set_security_data (&emm_sap.u.emm_as.u.security.sctx, &emm_ctx->_security, false, true);
  rc = emm_sap_send (&emm_sap);

  if (rc != RETURNerror) {
    emm_ctx_mark_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_IDENT);

    REQUIREMENT_3GPP_24_301(R10_5_4_4_2);

    if (emm_ctx->T3470.id != NAS_TIMER_INACTIVE_ID) {
      /*
       * Re-start T3470 timer
       */
      emm_ctx->T3470.id = nas_timer_stop (emm_ctx->T3470.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3470 restarted UE " MME_UE_S1AP_ID_FMT " ", data->ue_id);
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Timer T3470 (%ld) stopped, expires in %ld seconds\n", emm_ctx->T3470.id, emm_ctx->T3470.sec);
    }
    /*
     * Start T3470 timer
     */
    emm_ctx->T3470.id = nas_timer_start (emm_ctx->T3470.sec, 0 /*usec*/, _identification_t3470_handler, emm_ctx);
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3470 started UE " MME_UE_S1AP_ID_FMT " ", data->ue_id);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Timer T3470 (%ld) started, expires in %ld seconds\n", emm_ctx->T3470.id, emm_ctx->T3470.sec);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}


//------------------------------------------------------------------------------
static int _identification_ll_failure (emm_context_t *emm_ctx)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  if (emm_ctx_is_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_IDENT)){
    identification_data_t                  *data = &emm_ctx->common_proc->common_arg.u.identification_data;
    REQUIREMENT_3GPP_24_301(R10_5_4_4_6_a);
    emm_sap_t                               emm_sap = {0};

    emm_sap.primitive = EMMREG_PROC_ABORT;
    emm_sap.u.emm_reg.ue_id = emm_ctx->ue_id;
    emm_sap.u.emm_reg.ctx   = emm_ctx;
    data->notify_failure = true;
    rc = emm_sap_send (&emm_sap);
  }
  // abort ANY ongoing EMM procedure (R10_5_4_4_6_a)
  emm_proc_specific_abort(&emm_ctx->specific_proc);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _identification_non_delivered_ho (emm_context_t *emm_ctx)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  if (emm_ctx_is_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_IDENT)){
    identification_data_t                  *data = &emm_ctx->common_proc->common_arg.u.identification_data;
    REQUIREMENT_3GPP_24_301(R10_5_4_2_7_j);
    rc = _identification_request(data);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}


//------------------------------------------------------------------------------
/*
 * Description: Aborts the identification procedure currently in progress
 *
 * Inputs:  args:      Identification data to be released
 *      Others:    None
 *
 * Outputs:     None
 *      Return:    None
 *      Others:    T3470
 */
static int _identification_abort (emm_context_t *emm_ctx)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  if (emm_ctx_is_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_IDENT)){
    identification_data_t                  *data = &emm_ctx->common_proc->common_arg.u.identification_data;
    unsigned int                            ue_id = data->ue_id;
    int                                     notify_failure = data->notify_failure;

    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Abort identification procedure " "(ue_id=" MME_UE_S1AP_ID_FMT ")\n", ue_id);

    /*
     * Stop timer T3470
     */
    emm_ctx_unmark_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_IDENT);

    if (emm_ctx->T3470.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3470 (%ld)\n", emm_ctx->T3470.id);
      emm_ctx->T3470.id = nas_timer_stop (emm_ctx->T3470.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3470 stopped UE " MME_UE_S1AP_ID_FMT " ", data->ue_id);
    }

    /*
     * Notify EMM that the identification procedure failed
     */
    if (notify_failure) {
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_REJ ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
      emm_sap_t                               emm_sap = {0};

      emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx   = emm_ctx;
      rc = emm_sap_send (&emm_sap);
    } else {
      rc = RETURNok;
    }
    /*
     * Release retransmission timer parameters
     */
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

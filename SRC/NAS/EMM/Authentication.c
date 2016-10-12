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
  Source      Authentication.c

  Version     0.1

  Date        2013/03/04

  Product     NAS stack

  Subsystem   EPS Mobility Management

  Author      Frederic Maurel

  Description Defines the authentication EMM procedure executed by the
        Non-Access Stratum.

        The purpose of the EPS authentication and key agreement (AKA)
        procedure is to provide mutual authentication between the user
        and the network and to agree on a key KASME. The procedure is
        always initiated and controlled by the network. However, the
        UE can reject the EPS authentication challenge sent by the
        network.

        A partial native EPS security context is established in the
        UE and the network when an EPS authentication is successfully
        performed. The computed key material KASME is used as the
        root for the EPS integrity protection and ciphering key
        hierarchy.

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
#include "emm_proc.h"
#include "nas_timer.h"
#include "emm_data.h"
#include "emm_sap.h"
#include "emm_cause.h"
#include "nas_itti_messaging.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
    Internal data handled by the authentication procedure in the UE
   --------------------------------------------------------------------------
*/

/*
   --------------------------------------------------------------------------
    Internal data handled by the authentication procedure in the MME
   --------------------------------------------------------------------------
*/
//   Timer handler
static void *_authentication_t3460_handler (void *);

// Function executed when occurs a lower layer failure
static int _authentication_ll_failure (emm_context_t *emm_ctx);

// Function executed when occurs a lower layer non delivered indication
static int _authentication_non_delivered_ho (emm_context_t *emm_ctx);

/*
   Function executed whenever the ongoing EMM procedure that initiated
   the authentication procedure is aborted or the maximum value of the
   retransmission timer counter is exceed
*/
static int _authentication_abort (emm_context_t *emm_ctx);



static int                              _authentication_check_imsi_5_4_2_5__1 (emm_context_t *emm_ctx);
static int                              _authentication_request (authentication_data_t * data);
static int                              _authentication_reject (emm_context_t *emm_ctx);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/


/*
   --------------------------------------------------------------------------
        Authentication procedure executed by the MME
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_authentication()                                 **
 **                                                                        **
 ** Description: Initiates authentication procedure to establish partial   **
 **      native EPS security context in the UE and the MME.        **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.2                           **
 **      The network initiates the authentication procedure by     **
 **      sending an AUTHENTICATION REQUEST message to the UE and   **
 **      starting the timer T3460. The AUTHENTICATION REQUEST mes- **
 **      sage contains the parameters necessary to calculate the   **
 **      authentication response.                                  **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      ksi:       NAS key set identifier                     **
 **      rand:      Random challenge number                    **
 **      autn:      Authentication token                       **
 **      success:   Callback function executed when the authen-**
 **             tication procedure successfully completes  **
 **      reject:    Callback function executed when the authen-**
 **             tication procedure fails or is rejected    **
 **      failure:   Callback function executed whener a lower  **
 **             layer failure occured before the authenti- **
 **             cation procedure comnpletes                **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_authentication (
  emm_context_t *emm_ctx,
  mme_ue_s1ap_id_t ue_id,
  ksi_t ksi,
  const uint8_t   * const rand,
  const uint8_t   * const autn,
  emm_common_success_callback_t success,
  emm_common_reject_callback_t reject,
  emm_common_ll_failure_callback_t failure)
{
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Initiate authentication KSI = %d, ctx = %p\n", ue_id, ksi, emm_ctx);
  /*
   * Allocate parameters of the retransmission timer callback
   */
  if (emm_ctx->common_proc) {
    emm_common_cleanup(&emm_ctx->common_proc);
  }
  emm_ctx->common_proc = (emm_common_data_t *) calloc (1, sizeof (*emm_ctx->common_proc));

  if (emm_ctx->common_proc ) {
    // Setup ongoing EMM procedure callback functions
    rc = emm_proc_common_initialize (emm_ctx, EMM_COMMON_PROC_TYPE_AUTHENTICATION, emm_ctx->common_proc,
        success, reject, failure, _authentication_ll_failure, _authentication_non_delivered_ho, _authentication_abort);

    if (rc != RETURNok) {
      OAILOG_WARNING (LOG_NAS_EMM, "Failed to initialize EMM callback functions\n");
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }

    // Set the UE identifier
    emm_ctx->common_proc->common_arg.u.authentication_data.ue_id = ue_id;
    // Reset the retransmission counter
    emm_ctx->common_proc->common_arg.u.authentication_data.retransmission_count = 0;
    // Set the key set identifier
    emm_ctx->common_proc->common_arg.u.authentication_data.ksi = ksi;
    // Set the authentication random challenge number
    if (rand) {
        memcpy (emm_ctx->common_proc->common_arg.u.authentication_data.rand, rand, AUTH_RAND_SIZE);
    }
    // Set the authentication token
    if (autn) {
        memcpy (emm_ctx->common_proc->common_arg.u.authentication_data.autn, autn, AUTH_AUTN_SIZE);
    }

    /*
     * Send authentication request message to the UE
     */
    rc = _authentication_request (&emm_ctx->common_proc->common_arg.u.authentication_data);

    if (rc != RETURNerror) {
      /*
       * Notify EMM that common procedure has been initiated
       */
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_REQ ue id " MME_UE_S1AP_ID_FMT " (authentication)", ue_id);
      emm_sap_t                               emm_sap = {0};

      emm_sap.primitive = EMMREG_COMMON_PROC_REQ;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx   = emm_ctx;
      rc = emm_sap_send (&emm_sap);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
int emm_proc_authentication_failure (
  mme_ue_s1ap_id_t ue_id,
  int emm_cause,
  const_bstring auts)
{
  int                                     rc = RETURNerror;
  emm_sap_t                               emm_sap = {0};

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Authentication failure (ue_id=" MME_UE_S1AP_ID_FMT ", cause=%d)\n", ue_id, emm_cause);

  // Get the UE context
  emm_context_t *emm_ctx = emm_context_get (&_emm_data, ue_id);


  if (emm_ctx) {
    // Stop timer T3460
    REQUIREMENT_3GPP_24_301(R10_5_4_2_4__3);
    emm_ctx->T3460.id = nas_timer_stop (emm_ctx->T3460.id);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3460 (%ld) UE " MME_UE_S1AP_ID_FMT "\n", emm_ctx->T3460.id, emm_ctx->ue_id);
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3460 stopped UE " MME_UE_S1AP_ID_FMT " ", emm_ctx->ue_id);
  } else {
      OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to authentify the UE\n");
      emm_cause = EMM_CAUSE_ILLEGAL_UE;
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }


  switch (emm_cause) {
  case EMM_CAUSE_SYNCH_FAILURE:
    /*
     * USIM has detected a mismatch in SQN.
     *  Ask for a new vector.
     */
    REQUIREMENT_3GPP_24_301(R10_5_4_2_4__3);
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "SQN SYNCH_FAILURE ue id " MME_UE_S1AP_ID_FMT " ", ue_id);

    emm_ctx->auth_sync_fail_count += 1;
    if (EMM_AUTHENTICATION_SYNC_FAILURE_MAX > emm_ctx->auth_sync_fail_count) {
      OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC  - USIM has detected a mismatch in SQN Ask for new vector(s)\n");

      REQUIREMENT_3GPP_24_301(R10_5_4_2_7_e__3);
      emm_ctx_clear_auth_vectors(emm_ctx);

      REQUIREMENT_3GPP_24_301(R10_5_4_2_7_e__2);
      plmn_t visited_plmn = {0};
      visited_plmn.mcc_digit1 = emm_ctx->originating_tai.mcc_digit1;
      visited_plmn.mcc_digit2 = emm_ctx->originating_tai.mcc_digit2;
      visited_plmn.mcc_digit3 = emm_ctx->originating_tai.mcc_digit3;
      visited_plmn.mnc_digit1 = emm_ctx->originating_tai.mnc_digit1;
      visited_plmn.mnc_digit2 = emm_ctx->originating_tai.mnc_digit2;
      visited_plmn.mnc_digit3 = emm_ctx->originating_tai.mnc_digit3;

      nas_itti_auth_info_req (ue_id, emm_ctx->_imsi64, false, &visited_plmn, MAX_EPS_AUTH_VECTORS, auts);
      rc = RETURNok;
      if (EMM_COMMON_PROC_TYPE_AUTHENTICATION == emm_ctx->common_proc->type) {
        // Release retransmission timer parameters
        memset(&emm_ctx->common_proc->common_arg, 0, sizeof(emm_ctx->common_proc->common_arg));
      }
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    } else {
      AssertFatal (EMM_COMMON_PROC_TYPE_AUTHENTICATION == emm_ctx->common_proc->type,
          "mismatch in common_proc.type = %d, should be EMM_COMMON_PROC_TYPE_AUTHENTICATION=%d", emm_ctx->common_proc->type, EMM_COMMON_PROC_TYPE_AUTHENTICATION);
      REQUIREMENT_3GPP_24_301(R10_5_4_2_7_e__NOTE3);
      emm_ctx->emm_cause = EMM_CAUSE_SYNCH_FAILURE;
      rc = _authentication_reject(emm_ctx);
    }
    break;

  case EMM_CAUSE_MAC_FAILURE:
    emm_ctx->auth_sync_fail_count = 0;
    if (!IS_EMM_CTXT_PRESENT_IMSI(emm_ctx)) { // VALID means received in IDENTITY RESPONSE
      REQUIREMENT_3GPP_24_301(R10_5_4_2_7_c__2);
      rc = emm_proc_identification (emm_ctx, EMM_IDENT_TYPE_IMSI,
          _authentication_check_imsi_5_4_2_5__1, _authentication_reject, _authentication_ll_failure);

      if (rc != RETURNok) {
        REQUIREMENT_3GPP_24_301(R10_5_4_2_7_c__NOTE1); // more or less this case...
        // Failed to initiate the identification procedure
        OAILOG_WARNING (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT "EMM-PROC  - Failed to initiate identification procedure\n", emm_ctx->ue_id);
        emm_ctx->emm_cause = EMM_CAUSE_MAC_FAILURE;// EMM_CAUSE_ILLEGAL_UE;
        // Do not accept the UE to attach to the network
        AssertFatal (EMM_COMMON_PROC_TYPE_AUTHENTICATION == emm_ctx->common_proc->type,
            "mismatch in common_proc.type = %d, should be EMM_COMMON_PROC_TYPE_AUTHENTICATION=%d", emm_ctx->common_proc->type, EMM_COMMON_PROC_TYPE_AUTHENTICATION);
        rc = _authentication_reject(emm_ctx);
      }
    } else {
      AssertFatal (EMM_COMMON_PROC_TYPE_AUTHENTICATION == emm_ctx->common_proc->type,
          "mismatch in common_proc.type = %d, should be EMM_COMMON_PROC_TYPE_AUTHENTICATION=%d", emm_ctx->common_proc->type, EMM_COMMON_PROC_TYPE_AUTHENTICATION);
      REQUIREMENT_3GPP_24_301(R10_5_4_2_5__2);
      emm_ctx->emm_cause = EMM_CAUSE_MAC_FAILURE; //EMM_CAUSE_ILLEGAL_UE;
      // Do not accept the UE to attach to the network
      rc = _authentication_reject(emm_ctx);
    }
    break;

  case EMM_CAUSE_NON_EPS_AUTH_UNACCEPTABLE:
    emm_ctx->auth_sync_fail_count = 0;
    REQUIREMENT_3GPP_24_301(R10_5_4_2_7_d__1);
    rc = emm_proc_identification (emm_ctx, EMM_IDENT_TYPE_IMSI,
        _authentication_check_imsi_5_4_2_5__1, _authentication_reject, _authentication_ll_failure);

    if (rc != RETURNok) {
      REQUIREMENT_3GPP_24_301(R10_5_4_2_7_d__NOTE2); // more or less this case...
      // Failed to initiate the identification procedure
      OAILOG_WARNING (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT "EMM-PROC  - Failed to initiate identification procedure\n", emm_ctx->ue_id);
      emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      // Do not accept the UE to attach to the network
      rc = _authentication_reject(emm_ctx);
      //MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_REJ ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
      //emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
      //emm_sap.u.emm_reg.ue_id = ue_id;
      //emm_sap.u.emm_reg.ctx = emm_ctx;
      //rc = emm_sap_send (&emm_sap);
    }
    break;

  default:
    emm_ctx->auth_sync_fail_count = 0;
    OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC  - The MME received an unknown EMM CAUSE %d\n", emm_cause);

  }
  if (emm_ctx->common_proc) {
    AssertFatal (EMM_COMMON_PROC_TYPE_AUTHENTICATION == emm_ctx->common_proc->type,
        "mismatch in common_proc.type = %d, should be EMM_COMMON_PROC_TYPE_AUTHENTICATION=%d", emm_ctx->common_proc->type, EMM_COMMON_PROC_TYPE_AUTHENTICATION);
    // Release retransmission timer parameters
    memset(&emm_ctx->common_proc->common_arg, 0, sizeof(emm_ctx->common_proc->common_arg));
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}


/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_authentication_complete()                            **
 **                                                                        **
 ** Description: Performs the authentication completion procedure executed **
 **      by the network.                                                   **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.4                           **
 **      Upon receiving the AUTHENTICATION RESPONSE message, the           **
 **      MME shall stop timer T3460 and check the correctness of           **
 **      the RES parameter.                                                **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                          **
 **      emm_cause: Authentication failure EMM cause code                  **
 **      res:       Authentication response parameter. or auts             **
 **                 in case of sync failure                                **
 **      Others:    None                                                   **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                                  **
 **      Others:    _emm_data, T3460                                       **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_authentication_complete (
  mme_ue_s1ap_id_t ue_id,
  int emm_cause,
  const_bstring const res)
{
  int                                     rc = RETURNerror;
  emm_sap_t                               emm_sap = {0};

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Authentication complete (ue_id=" MME_UE_S1AP_ID_FMT ", cause=%d)\n", ue_id, emm_cause);
  // Get the UE context
  emm_context_t *emm_ctx = emm_context_get (&_emm_data, ue_id);

  if (emm_ctx) {
    // Stop timer T3460
    REQUIREMENT_3GPP_24_301(R10_5_4_2_4__1);
    emm_ctx->T3460.id = nas_timer_stop (emm_ctx->T3460.id);
    OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3460 (%ld) UE " MME_UE_S1AP_ID_FMT "\n", emm_ctx->T3460.id, emm_ctx->ue_id);
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3460 stopped UE " MME_UE_S1AP_ID_FMT " ", emm_ctx->ue_id);
  } else {
    // Release retransmission timer parameters
    memset(&emm_ctx->common_proc->common_arg, 0, sizeof(emm_ctx->common_proc->common_arg));
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to authentify the UE\n");
    emm_cause = EMM_CAUSE_ILLEGAL_UE;
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }


  emm_ctx->auth_sync_fail_count = 0;

  AssertFatal(emm_ctx->common_proc, "No common proc");
  AssertFatal(EMM_COMMON_PROC_TYPE_AUTHENTICATION == emm_ctx->common_proc->type, "Common proc mismatch %d != EMM_COMMON_PROC_TYPE_AUTHENTICATION", emm_ctx->common_proc->type);

  if (emm_cause == EMM_CAUSE_SUCCESS) {
    /*
     * Check the received RES parameter
     */

    REQUIREMENT_3GPP_24_301(R10_5_4_2_4__1);
    if (!biseqblk (res, emm_ctx->_vector[emm_ctx->_security.vector_index].xres, blength(res))) {
      REQUIREMENT_3GPP_24_301(R10_5_4_2_5__1);
      // bias: IMSI present but do not know if identification was really done with GUTI
      if (IS_EMM_CTXT_PRESENT_IMSI(emm_ctx)) {
        REQUIREMENT_3GPP_24_301(R10_5_4_2_5__2);
        /*
         * The MME received an authentication failure message or the RES
         * * * * contained in the Authentication Response message received from
         * * * * the UE does not match the XRES parameter computed by the network
         */
        (void)_authentication_reject (emm_ctx);
        /*
         * Notify EMM that the authentication procedure failed
         */
        //MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_REJ ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
        //emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
        //emm_sap.u.emm_reg.ue_id = ue_id;
        //emm_sap.u.emm_reg.ctx = emm_ctx;
      } else {
        plmn_t visited_plmn = {0};
        visited_plmn.mcc_digit1 = emm_ctx->originating_tai.mcc_digit1;
        visited_plmn.mcc_digit2 = emm_ctx->originating_tai.mcc_digit2;
        visited_plmn.mcc_digit3 = emm_ctx->originating_tai.mcc_digit3;
        visited_plmn.mnc_digit1 = emm_ctx->originating_tai.mnc_digit1;
        visited_plmn.mnc_digit2 = emm_ctx->originating_tai.mnc_digit2;
        visited_plmn.mnc_digit3 = emm_ctx->originating_tai.mnc_digit3;
        nas_itti_auth_info_req (ue_id, emm_ctx->_imsi64, false, &visited_plmn, MAX_EPS_AUTH_VECTORS, res);

        // Release retransmission timer parameters
        memset(&emm_ctx->common_proc->common_arg, 0, sizeof(emm_ctx->common_proc->common_arg));

        rc = RETURNok;
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
      }

    } else if (emm_ctx) {
      REQUIREMENT_3GPP_24_301(R10_5_4_2_4__2);
      emm_ctx_set_security_eksi(emm_ctx, emm_ctx->common_proc->common_arg.u.authentication_data.ksi);
      OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC  - Success to authentify the UE  RESP XRES == XRES UE CONTEXT\n");
      /*
       * Notify EMM that the authentication procedure successfully completed
       */
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_CNF ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
      OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC  - Notify EMM that the authentication procedure successfully completed\n");
      emm_sap.primitive = EMMREG_COMMON_PROC_CNF;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx = emm_ctx;
      emm_sap.u.emm_reg.u.common.is_attached = emm_ctx->is_attached;
    }
  } else {
    switch (emm_cause) {
    case EMM_CAUSE_SYNCH_FAILURE:
      /*
       * USIM has detected a mismatch in SQN.
       * * * * Ask for a new vector.
       */
      REQUIREMENT_3GPP_24_301(R10_5_4_2_4__3);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "SQN SYNCH_FAILURE ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
      OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC  - USIM has detected a mismatch in SQN Ask for new vector(s)\n");
      plmn_t visited_plmn = {0};
      visited_plmn.mcc_digit1 = emm_ctx->originating_tai.mcc_digit1;
      visited_plmn.mcc_digit2 = emm_ctx->originating_tai.mcc_digit2;
      visited_plmn.mcc_digit3 = emm_ctx->originating_tai.mcc_digit3;
      visited_plmn.mnc_digit1 = emm_ctx->originating_tai.mnc_digit1;
      visited_plmn.mnc_digit2 = emm_ctx->originating_tai.mnc_digit2;
      visited_plmn.mnc_digit3 = emm_ctx->originating_tai.mnc_digit3;
      nas_itti_auth_info_req (ue_id, emm_ctx->_imsi64, false, &visited_plmn, MAX_EPS_AUTH_VECTORS, res);

      // Release retransmission timer parameters
      memset(&emm_ctx->common_proc->common_arg, 0, sizeof(emm_ctx->common_proc->common_arg));
      rc = RETURNok;
      OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
      break;

    default:
      OAILOG_DEBUG (LOG_NAS_EMM, "EMM-PROC  - The MME received an authentication failure message or the RES does not match the XRES parameter computed by the network\n");
      /*
       * The MME received an authentication failure message or the RES
       * * * * contained in the Authentication Response message received from
       * * * * the UE does not match the XRES parameter computed by the network
       */
      (void)_authentication_reject (emm_ctx);
      /*
       * Notify EMM that the authentication procedure failed
       */
      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_REJ ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
      emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx = emm_ctx;
      break;
    }
  }

  // Release retransmission timer parameters
  // memset(&emm_ctx->common_proc->common_arg, 0, sizeof(emm_ctx->common_proc->common_arg));

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
 ** Name:    _authentication_t3460_handler()                           **
 **                                                                        **
 ** Description: T3460 timeout handler                                     **
 **      Upon T3460 timer expiration, the authentication request   **
 **      message is retransmitted and the timer restarted. When    **
 **      retransmission counter is exceed, the MME shall abort the **
 **      authentication procedure and any ongoing EMM specific     **
 **      procedure and release the NAS signalling connection.      **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.7, case b                   **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void  *_authentication_t3460_handler (void *args)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  emm_context_t                     *emm_ctx = (emm_context_t*)args;

  if (emm_ctx) {
    emm_ctx->T3460.id = NAS_TIMER_INACTIVE_ID;
  }
  if (emm_ctx_is_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_AUTH)){
    authentication_data_t                  *data = &emm_ctx->common_proc->common_arg.u.authentication_data;

    /*
     * Increment the retransmission counter
     */
    REQUIREMENT_3GPP_24_301(R10_5_4_2_7_b);
    // TODO the network shall abort any ongoing EMM specific procedure.

    data->retransmission_count += 1;
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - T3460 timer expired, retransmission " "counter = %d\n", data->retransmission_count);

    if (data->retransmission_count < AUTHENTICATION_COUNTER_MAX) {
      /*
       * Send authentication request message to the UE
       */
      rc = _authentication_request (data);
    } else {
      /*
       * Release the NAS signalling connection
       */
      emm_sap_t                               emm_sap = {0};
      emm_sap.primitive = EMMAS_RELEASE_REQ;
      emm_sap.u.emm_as.u.release.guti = NULL;
      emm_sap.u.emm_as.u.release.ue_id = emm_ctx->ue_id;
      emm_sap.u.emm_as.u.release.cause = EMM_AS_CAUSE_AUTHENTICATION;
      rc = emm_sap_send (&emm_sap);

      /*
       * Abort the authentication procedure
       */
      memset((void*)&emm_sap, 0, sizeof(emm_sap));
      emm_sap.primitive = EMMREG_PROC_ABORT;
      emm_sap.u.emm_reg.ue_id = emm_ctx->ue_id;
      emm_sap.u.emm_reg.ctx   = emm_ctx;
      data->notify_failure = false;
      rc = emm_sap_send (&emm_sap);

      // abort ANY ongoing EMM procedure (R10_5_4_2_7_b)
      rc = emm_proc_specific_abort(&emm_ctx->specific_proc);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, NULL);
}

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/

int _authentication_check_imsi_5_4_2_5__1 (emm_context_t *emm_ctx) {
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  if (emm_ctx_is_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_AUTH)){
    REQUIREMENT_3GPP_24_301(R10_5_4_2_5__1);
    if (IS_EMM_CTXT_VALID_IMSI(emm_ctx)) { // VALID means received in IDENTITY RESPONSE
      if (emm_ctx->_imsi64 != emm_ctx->saved_imsi64) {
        plmn_t visited_plmn = {0};
        visited_plmn.mcc_digit1 = emm_ctx->originating_tai.mcc_digit1;
        visited_plmn.mcc_digit2 = emm_ctx->originating_tai.mcc_digit2;
        visited_plmn.mcc_digit3 = emm_ctx->originating_tai.mcc_digit3;
        visited_plmn.mnc_digit1 = emm_ctx->originating_tai.mnc_digit1;
        visited_plmn.mnc_digit2 = emm_ctx->originating_tai.mnc_digit2;
        visited_plmn.mnc_digit3 = emm_ctx->originating_tai.mnc_digit3;
        nas_itti_auth_info_req (emm_ctx->ue_id, emm_ctx->_imsi64, false, &visited_plmn, MAX_EPS_AUTH_VECTORS, NULL);
        OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
      }
    }
    rc = _authentication_reject(emm_ctx);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_request()                                 **
 **                                                                        **
 ** Description: Sends AUTHENTICATION REQUEST message and start timer T3460**
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3460                                      **
 **                                                                        **
 ***************************************************************************/
int _authentication_request (authentication_data_t * data)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_sap_t                               emm_sap = {0};
  int                                     rc = RETURNerror;
  struct emm_context_s              *emm_ctx = NULL;

  if (data) {
    /*
     * Notify EMM-AS SAP that Authentication Request message has to be sent
     * to the UE
     */
    emm_sap.primitive = EMMAS_SECURITY_REQ;
    emm_sap.u.emm_as.u.security.guti = NULL;
    emm_sap.u.emm_as.u.security.ue_id = data->ue_id;
    emm_sap.u.emm_as.u.security.msg_type = EMM_AS_MSG_TYPE_AUTH;
    emm_sap.u.emm_as.u.security.ksi = data->ksi;
    memcpy(emm_sap.u.emm_as.u.security.rand, data->rand, sizeof (emm_sap.u.emm_as.u.security.rand));
    memcpy(emm_sap.u.emm_as.u.security.autn, data->autn, sizeof (emm_sap.u.emm_as.u.security.autn));
    /*
     * TODO: check for pointer validity
     */
    emm_ctx = emm_context_get (&_emm_data, data->ue_id);
    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data (&emm_sap.u.emm_as.u.security.sctx, &emm_ctx->_security, false, true);
    REQUIREMENT_3GPP_24_301(R10_5_4_2_2);
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMAS_SECURITY_REQ ue id " MME_UE_S1AP_ID_FMT " ", data->ue_id);
    rc = emm_sap_send (&emm_sap);

    if (rc != RETURNerror) {
      if (emm_ctx) {
        // mark this common procedure as running
        emm_ctx_mark_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_AUTH);

        if (emm_ctx->T3460.id != NAS_TIMER_INACTIVE_ID) {
          /*
           * Re-start T3460 timer
           */
          emm_ctx->T3460.id = nas_timer_stop (emm_ctx->T3460.id);
          MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3460 restarted UE " MME_UE_S1AP_ID_FMT " ", data->ue_id);
          OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Timer T3460 (%ld) restarted, expires in %ld seconds\n", emm_ctx->T3460.id, emm_ctx->T3460.sec);
        }
        /*
         * Start T3460 timer
         */
        emm_ctx->T3460.id = nas_timer_start (emm_ctx->T3460.sec, 0  /*usec*/, _authentication_t3460_handler, emm_ctx);
        MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3460 started UE " MME_UE_S1AP_ID_FMT " ", data->ue_id);
        OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Timer T3460 (%ld) started, expires in %ld seconds\n", emm_ctx->T3460.id, emm_ctx->T3460.sec);
      }
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_reject()                                  **
 **                                                                        **
 ** Description: Sends AUTHENTICATION REJECT message                       **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _authentication_reject (emm_context_t *emm_ctx)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_sap_t                               emm_sap = {0};
  emm_sap_t                               emm_sap_rej = {0};
  int                                     rc = RETURNerror;
  if (emm_ctx_is_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_AUTH)){
    authentication_data_t                  *data = &emm_ctx->common_proc->common_arg.u.authentication_data;
    /*
     * Notify EMM-AS SAP that Authentication Reject message has to be sent
     * to the UE
     */
    emm_sap.primitive = EMMAS_SECURITY_REJ;
    emm_sap.u.emm_as.u.security.guti = NULL;
    emm_sap.u.emm_as.u.security.ue_id = data->ue_id;
    emm_sap.u.emm_as.u.security.msg_type = EMM_AS_MSG_TYPE_AUTH;

    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data (&emm_sap.u.emm_as.u.security.sctx, &emm_ctx->_security, false, true);
    rc = emm_sap_send (&emm_sap);

    /*
     * Notify EMM that the authentication procedure failed
     */
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_REJ ue id " MME_UE_S1AP_ID_FMT " ", data->ue_id);
    emm_sap_rej.primitive = EMMREG_COMMON_PROC_REJ;
    emm_sap_rej.u.emm_reg.ue_id = data->ue_id;
    emm_sap_rej.u.emm_reg.ctx = emm_ctx;
    rc = emm_sap_send (&emm_sap_rej);

  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_ll_failure()                                   **
 **                                                                        **
 ** Description: Aborts the authentication procedure currently in progress **
 ** Inputs:  args:      Authentication data to be released         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3460                                      **
 **                                                                        **
 ***************************************************************************/
static int _authentication_ll_failure (emm_context_t *emm_ctx)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  if (emm_ctx_is_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_AUTH)){
    authentication_data_t                  *data = &emm_ctx->common_proc->common_arg.u.authentication_data;
    REQUIREMENT_3GPP_24_301(R10_5_4_2_7_a);
    emm_sap_t                               emm_sap = {0};

    emm_sap.primitive = EMMREG_PROC_ABORT;
    emm_sap.u.emm_reg.ue_id = emm_ctx->ue_id;
    emm_sap.u.emm_reg.ctx   = emm_ctx;
    data->notify_failure    = true;
    rc = emm_sap_send (&emm_sap);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_non_delivered()                                   **
 **                                                                        **
 ** Description:  **
 ** Inputs:  args:      Authentication data to be released         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3460                                      **
 **                                                                        **
 ***************************************************************************/
static int _authentication_non_delivered_ho (emm_context_t *emm_ctx)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  if (emm_ctx_is_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_AUTH)){
    authentication_data_t                  *data = &emm_ctx->common_proc->common_arg.u.authentication_data;
    REQUIREMENT_3GPP_24_301(R10_5_4_2_7_j);
    rc = _authentication_request(data);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}


/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_abort()                                   **
 **                                                                        **
 ** Description: Aborts the authentication procedure currently in progress **
 **                                                                        **
 ** Inputs:  args:      Authentication data to be released         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3460                                      **
 **                                                                        **
 ***************************************************************************/
static int _authentication_abort (emm_context_t *emm_ctx)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  if (emm_ctx_is_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_AUTH)){
    authentication_data_t                  *data = &emm_ctx->common_proc->common_arg.u.authentication_data;

    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Abort authentication procedure " "(ue_id=" MME_UE_S1AP_ID_FMT ")\n", emm_ctx->ue_id);

    emm_ctx_unmark_common_procedure_running(emm_ctx, EMM_CTXT_COMMON_PROC_AUTH);
    /*
     * Stop timer T3460
     */
    if (emm_ctx->T3460.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3460 (%ld)\n", emm_ctx->T3460.id);
      emm_ctx->T3460.id = nas_timer_stop (emm_ctx->T3460.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3460 stopped UE " MME_UE_S1AP_ID_FMT " ", emm_ctx->ue_id);
    }

    if (data->notify_failure) {
      /*
       * Notify EMM that the authentication procedure failed
       */
      emm_sap_t                               emm_sap = {0};

      emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
      emm_sap.u.emm_reg.ue_id = emm_ctx->ue_id;
      emm_sap.u.emm_reg.ctx   = emm_ctx;
      rc = emm_sap_send (&emm_sap);
    }

    /*
     * Release retransmission timer parameters
     * Do it after emm_sap_send
     */
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

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

#include "emm_proc.h"
#include "networkDef.h"
#include "log.h"
#include "nas_timer.h"

#include "emmData.h"

#include "emm_sap.h"
#include "esm_sap.h"
#include "emm_cause.h"

#include "NasSecurityAlgorithms.h"

#include "mme_api.h"
#include "mme_config.h"
#include "nas_itti_messaging.h"
#include "msc.h"
#include "obj_hashtable.h"
#include "gcc_diag.h"

#include <string.h>             // memcmp, memcpy
#include <stdlib.h>             // MALLOC_CHECK, FREE_CHECK

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
static void                            *_emm_attach_t3450_handler (
  void *);

/*
   Functions that may initiate EMM common procedures
*/
static int                              _emm_attach_identify (
  void *);
static int                              _emm_attach_security (
  void *);
static int                              _emm_attach (
  void *);

/*
   Abnormal case attach procedures
*/
static int                              _emm_attach_release (
  void *);
static int                              _emm_attach_reject (
  void *);
static int                              _emm_attach_abort (
  void *);

static int                              _emm_attach_have_changed (
  const emm_data_context_t * ctx,
  emm_proc_attach_type_t type,
  int ksi,
  GUTI_t * guti,
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

static int                              _emm_attach_update (
  emm_data_context_t * ctx,
  mme_ue_s1ap_id_t ue_id,
  emm_proc_attach_type_t type,
  int ksi,
  GUTI_t * guti,
  imsi_t * imsi,
  imei_t * imei,
  const tai_t   * const originating_tai,
  int eea,
  int eia,
  int ucs2,
  int uea,
  int uia,
  int gea,
  int umts_present,
  int gprs_present,
  const OctetString * esm_msg_pP);

/*
   Internal data used for attach procedure
*/
typedef struct attach_data_s {
  unsigned int                            ue_id; /* UE identifier        */
#define ATTACH_COUNTER_MAX  5
  unsigned int                            retransmission_count; /* Retransmission counter   */
  OctetString                             esm_msg;      /* ESM message to be sent within
                                                         * the Attach Accept message    */
} attach_data_t;

static int                              _emm_attach_accept (
  emm_data_context_t * emm_ctx,
  attach_data_t * data);

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
  bool is_native_ksi,
  ksi_t     ksi,
  bool is_native_guti,
  GUTI_t * guti,
  imsi_t * imsi,
  imei_t * imei,
  tai_t * last_visited_registered_tai,
  const tai_t   * const originating_tai,
  int eea,
  int eia,
  int ucs2,
  int uea,
  int uia,
  int gea,
  int umts_present,
  int gprs_present,
  const OctetString * esm_msg_pP,
  const nas_message_decode_status_t  * const decode_status)
{
  LOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  emm_data_context_t                      ue_ctx;
  emm_fsm_state_t                         fsm_state = EMM_DEREGISTERED;

  LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - EPS attach type = %s (%d) requested (ue_id=" NAS_UE_ID_FMT ")\n", _emm_attach_type_str[type], type, ue_id);
  LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - umts_present = %u umts_present = %u\n", umts_present, gprs_present);
  /*
   * Initialize the temporary UE context
   */

  /*
   * Get the UE's EMM context if it exists
   */
  // if ue_id is valid (sent by eNB), we should always find the context
  emm_data_context_t *emm_ctx = emm_data_context_get (&_emm_data, ue_id);
  if (!emm_ctx) {
    if (guti) {
      emm_ctx = emm_data_context_get_by_guti (&_emm_data, guti);
    }
  }

  memset (&ue_ctx, 0, sizeof (emm_data_context_t));
  ue_ctx.is_dynamic = false;
  if (INVALID_MME_UE_S1AP_ID == ue_id) {
    if (emm_ctx) {
      ue_ctx.ue_id = emm_ctx->ue_id;
    } else {
      ue_ctx.ue_id = ue_id;
    }
  } else  {
    ue_ctx.ue_id = ue_id;
  }

  /*
   * Requirement MME24.301R10_5.5.1.1_1
   * MME not configured to support attach for emergency bearer services
   * shall reject any request to attach with an attach type set to "EPS
   * emergency attach".
   */
  if (!(_emm_data.conf.eps_network_feature_support & EPS_NETWORK_FEATURE_SUPPORT_EMERGENCY_BEARER_SERVICES_IN_S1_MODE_SUPPORTED) &&
      (type == EMM_ATTACH_TYPE_EMERGENCY)) {
    memset (&ue_ctx, 0, sizeof (emm_data_context_t));
    ue_ctx.is_dynamic = false;
    ue_ctx.ue_id = ue_id;

    ue_ctx.emm_cause = EMM_CAUSE_IMEI_NOT_ACCEPTED;
    /*
     * Do not accept the UE to attach for emergency services
     */
    rc = _emm_attach_reject (&ue_ctx);
    LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }



  fsm_state = emm_fsm_get_status (ue_id, emm_ctx);
  if ((emm_ctx) && (EMM_REGISTERED == fsm_state)) {
    /*
     * MME24.301R10_5.5.1.2.7_f ATTACH REQUEST received in state EMM-REGISTERED
     * If an ATTACH REQUEST message is received in state EMM-REGISTERED the network may initiate the
     * EMM common procedures; if it turned out that the ATTACH REQUEST message was sent by a UE that has
     * already been attached, the EMM context, EPS bearer contexts, if any, are deleted and the new ATTACH
     *  REQUEST is progressed.
     */

    // delete EMM context : what does it mean exactly ?
    // EMM context: An EMM context is established in the UE and the MME when an attach procedure is successfully completed. /* great */

    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Initiate new attach procedure\n");
    rc = emm_proc_attach_request (ue_id, type, is_native_ksi, ksi, is_native_guti, guti, imsi, imei, last_visited_registered_tai, originating_tai,
        eea, eia, ucs2, uea, uia, gea, umts_present, gprs_present, esm_msg_pP, decode_status);
  } else if ((emm_ctx) && ((EMM_DEREGISTERED < fsm_state ) && (EMM_REGISTERED != fsm_state))) {
    /*
     * Requirement MME24.301R10_5.5.1.2.7_e
     * More than one ATTACH REQUEST received and no ATTACH ACCEPT or ATTACH REJECT message has been sent
     * - If one or more of the information elements in the ATTACH REQUEST message differs from the ones
     * received within the previous ATTACH REQUEST message, the previously initiated attach procedure shall
     * be aborted and the new attach procedure shall be executed;
     * - if the information elements do not differ, then the network shall continue with the previous attach procedure
     * and shall ignore the second ATTACH REQUEST message.
     */
    if (_emm_attach_have_changed (emm_ctx, type, ksi, guti, imsi, imei, eea, eia, ucs2, uea, uia, gea, umts_present, gprs_present)) {
      LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Attach parameters have changed\n");
      /*
       * Notify EMM that the attach procedure is aborted
       */
      emm_sap_t                               emm_sap = {0};

      emm_sap.primitive = EMMREG_PROC_ABORT;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx = emm_ctx;
      rc = emm_sap_send (&emm_sap);

      if (rc != RETURNerror) {
        /*
         * Process new attach procedure
         */
        LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Initiate new attach procedure\n");
        rc = emm_proc_attach_request (ue_id, type, is_native_ksi, ksi, is_native_guti, guti, imsi, imei, last_visited_registered_tai,originating_tai,
            eea, eia, ucs2, uea, uia, gea, umts_present, gprs_present, esm_msg_pP, decode_status);
      }

      LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    } else {
      /*
       * Continue with the previous attach procedure
       */
      LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Received duplicated Attach Request\n");
      LOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
    }
  } else { //else  ((emm_ctx) && ((EMM_DEREGISTERED < fsm_state ) && (EMM_REGISTERED != fsm_state)))
    if (!emm_ctx) {
      /*
       * Create UE's EMM context
       */
      emm_ctx = (emm_data_context_t *) CALLOC_CHECK (1, sizeof (emm_data_context_t));

      if (!emm_ctx) {
        LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to create EMM context\n");
        ue_ctx.emm_cause = EMM_CAUSE_ILLEGAL_UE;
        /*
         * Do not accept the UE to attach to the network
         */
        rc = _emm_attach_reject (&ue_ctx);
        LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
      }

      emm_ctx->is_dynamic = true;
      emm_ctx->guti = NULL;
      emm_ctx->old_guti = NULL;
      emm_ctx->imsi = NULL;
      emm_ctx->imei = NULL;
      emm_ctx->security = NULL;
      emm_ctx->esm_msg.length = 0;
      emm_ctx->esm_msg.value = NULL;
      emm_ctx->emm_cause = EMM_CAUSE_SUCCESS;
      emm_ctx->_emm_fsm_status = EMM_INVALID;
      emm_ctx->ue_id = (mme_ue_s1ap_id_t)((uintptr_t)emm_ctx);
      /*
       * Initialize EMM timers
       */
      emm_ctx->T3450.id = NAS_TIMER_INACTIVE_ID;
      emm_ctx->T3450.sec = T3450_DEFAULT_VALUE;
      emm_ctx->T3460.id = NAS_TIMER_INACTIVE_ID;
      emm_ctx->T3460.sec = T3460_DEFAULT_VALUE;
      emm_ctx->T3470.id = NAS_TIMER_INACTIVE_ID;
      emm_ctx->T3470.sec = T3470_DEFAULT_VALUE;
      emm_fsm_set_status (ue_id, emm_ctx, EMM_DEREGISTERED);
      emm_data_context_add (&_emm_data, emm_ctx);
    }


    if (last_visited_registered_tai) {
      LOG_TRACE (LOG_NAS_EMM, "EMM-PROC  - last_visited_registered_tai available \n");
      memcpy(&emm_ctx->last_visited_registered_tai, &last_visited_registered_tai, sizeof(tai_t));
    } else {
      memset(&emm_ctx->last_visited_registered_tai, 0, sizeof(tai_t));
      LOG_TRACE (LOG_NAS_EMM, "EMM-PROC  - last_visited_registered_tai not available \n");
    }
  }

  /*
   * Update the EMM context with the current attach procedure parameters
   */
  rc = _emm_attach_update (emm_ctx, ue_id, type, ksi, guti, imsi, imei, originating_tai, eea, eia, ucs2, uea, uia, gea, umts_present, gprs_present, esm_msg_pP);

  if (rc != RETURNok) {
    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to update EMM context\n");
    /*
     * Do not accept the UE to attach to the network
     */
    emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    rc = _emm_attach_reject (emm_ctx);
  } else {
    /*
     * Performs UE identification
     */
    rc = _emm_attach_identify (emm_ctx);
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
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
  int emm_cause)
{
  LOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  /*
   * Create temporary UE context
   */
  emm_data_context_t                      ue_ctx;

  memset (&ue_ctx, 0, sizeof (emm_data_context_t));
  ue_ctx.is_dynamic = false;
  ue_ctx.ue_id = ue_id;
  /*
   * Update the EMM cause code
   */

  if (ue_id > 0) {
    ue_ctx.emm_cause = emm_cause;
  } else {
    ue_ctx.emm_cause = EMM_CAUSE_ILLEGAL_UE;
  }

  /*
   * Do not accept attach request with protocol error
   */
  rc = _emm_attach_reject (&ue_ctx);
  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
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
  const OctetString * esm_msg_pP)
{
  emm_data_context_t                     *emm_ctx = NULL;
  int                                     rc = RETURNerror;
  emm_sap_t                               emm_sap = {0};
  esm_sap_t                               esm_sap = {0};

  LOG_FUNC_IN (LOG_NAS_EMM);
  LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - EPS attach complete (ue_id=" NAS_UE_ID_FMT ")\n", ue_id);
  /*
   * Release retransmission timer parameters
   */
  attach_data_t                          *data = (attach_data_t *) (emm_proc_common_get_args (ue_id));

  if (data) {
    if (data->esm_msg.length > 0) {
      FREE_CHECK (data->esm_msg.value);
    }
    FREE_CHECK (data);
  }

  /*
   * Get the UE context
   */
  emm_ctx = emm_data_context_get (&_emm_data, ue_id);

  if (emm_ctx) {
    /*
     * Stop timer T3450
     */
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3450 (%d)\n", emm_ctx->T3450.id);
    emm_ctx->T3450.id = nas_timer_stop (emm_ctx->T3450.id);
    MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 stopped UE " NAS_UE_ID_FMT " ", ue_id);
    /*
     * Delete the old GUTI and consider the GUTI sent in the Attach
     * Accept message as valid
     */
    emm_ctx->guti_is_new = false;
    emm_ctx->old_guti = NULL;
    /*
     * Forward the Activate Default EPS Bearer Context Accept message
     * to the EPS session management sublayer
     */
    esm_sap.primitive = ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_CNF;
    esm_sap.is_standalone = false;
    esm_sap.ue_id = ue_id;
    esm_sap.recv = esm_msg_pP;
    esm_sap.ctx = emm_ctx;
    rc = esm_sap_send (&esm_sap);
  } else {
    LOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - No EMM context exists\n");
  }

  if ((rc != RETURNerror) && (esm_sap.err == ESM_SAP_SUCCESS)) {
    /*
     * Set the network attachment indicator
     */
    emm_ctx->is_attached = true;
    /*
     * Notify EMM that attach procedure has successfully completed
     */
    emm_sap.primitive = EMMREG_ATTACH_CNF;
    emm_sap.u.emm_reg.ue_id = ue_id;
    emm_sap.u.emm_reg.ctx = emm_ctx;
    rc = emm_sap_send (&emm_sap);
  } else if (esm_sap.err != ESM_SAP_DISCARDED) {
    /*
     * Notify EMM that attach procedure failed
     */
    emm_sap.primitive = EMMREG_ATTACH_REJ;
    emm_sap.u.emm_reg.ue_id = ue_id;
    emm_sap.u.emm_reg.ctx = emm_ctx;
    rc = emm_sap_send (&emm_sap);
  } else {
    /*
     * ESM procedure failed and, received message has been discarded or
     * Status message has been returned; ignore ESM procedure failure
     */
    rc = RETURNok;
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
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
static void                            *
_emm_attach_t3450_handler (
  void *args)
{
  LOG_FUNC_IN (LOG_NAS_EMM);
  attach_data_t                          *data = (attach_data_t *) (args);

  /*
   * Increment the retransmission counter
   */
  data->retransmission_count += 1;
  LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - T3450 timer expired, retransmission " "counter = %d\n", data->retransmission_count);
  /*
   * Get the UE's EMM context
   */
  emm_data_context_t                     *emm_ctx = NULL;

  emm_ctx = emm_data_context_get (&_emm_data, data->ue_id);

  if (data->retransmission_count < ATTACH_COUNTER_MAX) {
    /*
     * Send attach accept message to the UE
     */
    _emm_attach_accept (emm_ctx, data);
  } else {
    /*
     * Abort the attach procedure
     */
    _emm_attach_abort (data);
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, NULL);
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
static int
_emm_attach_release (
  void *args)
{
  LOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  emm_data_context_t                     *emm_ctx = (emm_data_context_t *) (args);

  if (emm_ctx) {
    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Release UE context data (ue_id=" NAS_UE_ID_FMT ")\n", emm_ctx->ue_id);
    unsigned int                            ue_id = emm_ctx->ue_id;

    if (emm_ctx->guti) {
      FREE_CHECK (emm_ctx->guti);
      emm_ctx->guti = NULL;
    }

    if (emm_ctx->imsi) {
      FREE_CHECK (emm_ctx->imsi);
      emm_ctx->imsi = NULL;
    }

    if (emm_ctx->imei) {
      FREE_CHECK (emm_ctx->imei);
      emm_ctx->imei = NULL;
    }

    if (emm_ctx->esm_msg.length > 0) {
      FREE_CHECK (emm_ctx->esm_msg.value);
      emm_ctx->esm_msg.value = NULL;
    }

    /*
     * Release NAS security context
     */
    if (emm_ctx->security) {
      emm_security_context_t                 *security = emm_ctx->security;

      if (security->kasme.value) {
        FREE_CHECK (security->kasme.value);
        security->kasme.value = NULL;
        security->kasme.length = 0;
      }

      if (security->knas_enc.value) {
        FREE_CHECK (security->knas_enc.value);
        security->knas_enc.value = NULL;
        security->knas_enc.length = 0;
      }

      if (security->knas_int.value) {
        FREE_CHECK (security->knas_int.value);
        security->knas_int.value = NULL;
        security->knas_int.length = 0;
      }

      FREE_CHECK (emm_ctx->security);
      emm_ctx->security = NULL;
    }

    /*
     * Stop timer T3450
     */
    if (emm_ctx->T3450.id != NAS_TIMER_INACTIVE_ID) {
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3450 (%d)\n", emm_ctx->T3450.id);
      emm_ctx->T3450.id = nas_timer_stop (emm_ctx->T3450.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 stopped UE " NAS_UE_ID_FMT " ", emm_ctx->ue_id);
    }

    /*
     * Stop timer T3460
     */
    if (emm_ctx->T3460.id != NAS_TIMER_INACTIVE_ID) {
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3460 (%d)\n", emm_ctx->T3460.id);
      emm_ctx->T3460.id = nas_timer_stop (emm_ctx->T3460.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3460 stopped UE " NAS_UE_ID_FMT " ", emm_ctx->ue_id);
    }

    /*
     * Stop timer T3470
     */
    if (emm_ctx->T3470.id != NAS_TIMER_INACTIVE_ID) {
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3470 (%d)\n", emm_ctx->T3460.id);
      emm_ctx->T3470.id = nas_timer_stop (emm_ctx->T3470.id);
      MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3470 stopped UE " NAS_UE_ID_FMT " ", emm_ctx->ue_id);
    }

    /*
     * Release the EMM context
     */
    emm_data_context_remove (&_emm_data, emm_ctx);
    /*
     * Notify EMM that the attach procedure is aborted
     */
    emm_sap_t                               emm_sap = {0};


    emm_sap.primitive = EMMREG_PROC_ABORT;
    emm_sap.u.emm_reg.ue_id = ue_id;
    emm_sap.u.emm_reg.ctx = emm_ctx;
    rc = emm_sap_send (&emm_sap);
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
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
static int
_emm_attach_reject (
  void *args)
{
  LOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  emm_data_context_t                     *emm_ctx = (emm_data_context_t *) (args);

  if (emm_ctx) {
    emm_sap_t                               emm_sap = {0};

    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - EMM attach procedure not accepted " "by the network (ue_id=" NAS_UE_ID_FMT ", cause=%d)\n", emm_ctx->ue_id, emm_ctx->emm_cause);
    /*
     * Notify EMM-AS SAP that Attach Reject message has to be sent
     * onto the network
     */
    emm_sap.primitive = EMMAS_ESTABLISH_REJ;
    emm_sap.u.emm_as.u.establish.ue_id = emm_ctx->ue_id;
    emm_sap.u.emm_as.u.establish.eps_id.guti = NULL;

    if (emm_ctx->emm_cause == EMM_CAUSE_SUCCESS) {
      emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    }

    emm_sap.u.emm_as.u.establish.emm_cause = emm_ctx->emm_cause;
    emm_sap.u.emm_as.u.establish.nas_info = EMM_AS_NAS_INFO_ATTACH;

    if (emm_ctx->emm_cause != EMM_CAUSE_ESM_FAILURE) {
      emm_sap.u.emm_as.u.establish.nas_msg.length = 0;
      emm_sap.u.emm_as.u.establish.nas_msg.value = NULL;
    } else if (emm_ctx->esm_msg.length > 0) {
      emm_sap.u.emm_as.u.establish.nas_msg = emm_ctx->esm_msg;
    } else {
      LOG_ERROR (LOG_NAS_EMM, "EMM-PROC  - ESM message is missing\n");
      LOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }

    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data (&emm_sap.u.emm_as.u.establish.sctx, emm_ctx->security, false, true);
    rc = emm_sap_send (&emm_sap);

    /*
     * Release the UE context, even if the network failed to send the
     * ATTACH REJECT message
     */
    if (emm_ctx->is_dynamic) {
      rc = _emm_attach_release (emm_ctx);
    }
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
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
static int
_emm_attach_abort (
  void *args)
{
  int                                     rc = RETURNerror;
  emm_data_context_t                     *ctx = NULL;
  attach_data_t                          *data;

  LOG_FUNC_IN (LOG_NAS_EMM);
  data = (attach_data_t *) (args);

  if (data) {
    unsigned int                            ue_id = data->ue_id;
    esm_sap_t                               esm_sap = {0};

    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Abort the attach procedure (ue_id=" NAS_UE_ID_FMT ")\n", ue_id);
    ctx = emm_data_context_get (&_emm_data, ue_id);

    if (ctx) {
      /*
       * Stop timer T3450
       */
      if (ctx->T3450.id != NAS_TIMER_INACTIVE_ID) {
        LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3450 (%d)\n", ctx->T3450.id);
        ctx->T3450.id = nas_timer_stop (ctx->T3450.id);
        MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 stopped UE " NAS_UE_ID_FMT " ", data->ue_id);
      }
    }

    /*
     * Release retransmission timer parameters
     */
    if (data->esm_msg.length > 0) {
      FREE_CHECK (data->esm_msg.value);
    }

    FREE_CHECK (data);
    /*
     * Notify ESM that the network locally refused PDN connectivity
     * to the UE
     */
    esm_sap.primitive = ESM_PDN_CONNECTIVITY_REJ;
    esm_sap.ue_id = ue_id;
    esm_sap.ctx = ctx;
    esm_sap.recv = NULL;
    rc = esm_sap_send (&esm_sap);

    if (rc != RETURNerror) {
      /*
       * Notify EMM that EPS attach procedure failed
       */
      emm_sap_t                               emm_sap = {0};

      emm_sap.primitive = EMMREG_ATTACH_REJ;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx = ctx;
      rc = emm_sap_send (&emm_sap);

      if (rc != RETURNerror) {
        /*
         * Release the UE context
         */
        rc = _emm_attach_release (ctx);
      }
    }
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
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
static int
_emm_attach_identify (
  void *args)
{
  int                                     rc = RETURNerror;
  emm_data_context_t                     *emm_ctx = (emm_data_context_t *) (args);
  int                                     guti_allocation = false;

  LOG_FUNC_IN (LOG_NAS_EMM);
  LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Identify incoming UE (ue_id=" NAS_UE_ID_FMT ") using %s\n", emm_ctx->ue_id, (emm_ctx->imsi) ? "IMSI" : (emm_ctx->guti) ? "GUTI" : (emm_ctx->imei) ? "IMEI" : "none");

  /*
   * UE's identification
   * -------------------
   */
  if (emm_ctx->imsi) {
    /*
     * The UE identifies itself using an IMSI
     */
    if (!emm_ctx->security) {
      /*
       * Ask upper layer to fetch new security context
       */
      nas_itti_auth_info_req (emm_ctx->ue_id, emm_ctx->imsi, 1, NULL);
      rc = RETURNok;
    } else {
      rc = mme_api_identify_imsi (emm_ctx->imsi, &emm_ctx->vector);

      if (rc != RETURNok) {
        LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to identify the UE using provided IMSI\n");
        emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      }

      guti_allocation = true;
    }
  } else if (emm_ctx->guti) {
    /*
     * The UE identifies itself using a GUTI
     */
    rc = mme_api_identify_guti (emm_ctx->guti, &emm_ctx->vector);

//#pragma message  "LG Temp. Force identification here"
    //LG Force identification here if (rc != RETURNok) {
      LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to identify the UE using provided GUTI (tmsi=%u)\n", emm_ctx->guti->m_tmsi);
      /*
       * 3GPP TS 24.401, Figure 5.3.2.1-1, point 4
       * The UE was attempting to attach to the network using a GUTI
       * that is not known by the network; the MME shall initiate an
       * identification procedure to retrieve the IMSI from the UE.
       */
      rc = emm_proc_identification (emm_ctx->ue_id, emm_ctx, EMM_IDENT_TYPE_IMSI, _emm_attach_identify, _emm_attach_release, _emm_attach_release);

      if (rc != RETURNok) {
        /*
         * Failed to initiate the identification procedure
         */
        LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to initiate identification procedure\n");
        emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
        /*
         * Do not accept the UE to attach to the network
         */
        rc = _emm_attach_reject (emm_ctx);
      }

      /*
       * Relevant callback will be executed when identification
       * procedure completes
       */
      LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    //LG Force identification here}
  } else if ((emm_ctx->imei) && (emm_ctx->is_emergency)) {
    /*
     * The UE is attempting to attach to the network for emergency
     * services using an IMEI
     */
    rc = mme_api_identify_imei (emm_ctx->imei, &emm_ctx->vector);

    if (rc != RETURNok) {
      LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - " "Failed to identify the UE using provided IMEI\n");
      emm_ctx->emm_cause = EMM_CAUSE_IMEI_NOT_ACCEPTED;
    }
  } else {
    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - UE's identity is not available\n");
    emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
  }

  /*
   * GUTI reallocation
   * -----------------
   */
  if ((rc != RETURNerror) && guti_allocation) {
    /*
     * Release the old GUTI
     */
    if (emm_ctx->old_guti) {
      FREE_CHECK (emm_ctx->old_guti);
    }

    /*
     * Save the GUTI previously used by the UE to identify itself
     */
    emm_ctx->old_guti = emm_ctx->guti;
    /*
     * Allocate a new GUTI
     */
    emm_ctx->guti = (GUTI_t *) MALLOC_CHECK (sizeof (GUTI_t));
    /*
     * Request the MME to assign a GUTI to the UE
     */
    rc = mme_api_new_guti (emm_ctx->imsi, emm_ctx->guti, &emm_ctx->tai_list);

    if (rc != RETURNok) {
      LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to assign new GUTI\n");
      emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    } else {
      LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - New GUTI assigned to the UE (tmsi=%u)\n", emm_ctx->guti->m_tmsi);
      /*
       * Update the GUTI indicator as new
       */
      emm_ctx->guti_is_new = true;
    }
  }

  /*
   * UE's authentication
   * -------------------
   */
  if (rc != RETURNerror) {
    if (emm_ctx->security) {
      /*
       * A security context exists for the UE in the network;
       * proceed with the attach procedure.
       */
      rc = _emm_attach (emm_ctx);
    } else if ((emm_ctx->is_emergency) && (_emm_data.conf.features & MME_API_UNAUTHENTICATED_IMSI)) {
      /*
       * 3GPP TS 24.301, section 5.5.1.2.3
       * 3GPP TS 24.401, Figure 5.3.2.1-1, point 5a
       * MME configured to support Emergency Attach for unauthenticated
       * IMSIs may choose to skip the authentication procedure even if
       * no EPS security context is available and proceed directly to the
       * execution of the security mode control procedure.
       */
      rc = _emm_attach_security (emm_ctx);
    }
#if 0
//    else {
//      /*
//       * 3GPP TS 24.401, Figure 5.3.2.1-1, point 5a
//       * No EMM context exists for the UE in the network; authentication
//       * and NAS security setup to activate integrity protection and NAS
//       * ciphering are mandatory.
//       */
//      auth_vector_t                          *auth = &emm_ctx->vector;
//      const OctetString                       loc_rand = { AUTH_RAND_SIZE, (uint8_t *) auth->rand };
//      const OctetString                       autn = { AUTH_AUTN_SIZE, (uint8_t *) auth->autn };
//      rc = emm_proc_authentication (emm_ctx, emm_ctx->ue_id, 0,  // TODO: eksi != 0
//                                    &loc_rand, &autn, _emm_attach_security, _emm_attach_release, _emm_attach_release);
//
//      if (rc != RETURNok) {
//        /*
//         * Failed to initiate the authentication procedure
//         */
//        LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to initiate authentication procedure\n");
//        emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
//      }
//    }
#endif
  }

  if (rc != RETURNok) {
    /*
     * Do not accept the UE to attach to the network
     */
    rc = _emm_attach_reject (emm_ctx);
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
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
int
emm_attach_security (
  void *args)
{
  return _emm_attach_security (args);
}

static int
_emm_attach_security (
  void *args)
{
  LOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  emm_data_context_t                     *emm_ctx = (emm_data_context_t *) (args);

  LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Setup NAS security (ue_id=" NAS_UE_ID_FMT ")\n", emm_ctx->ue_id);

  /*
   * Create new NAS security context
   */
  if (!emm_ctx->security) {
    emm_ctx->security = (emm_security_context_t *) CALLOC_CHECK (1, sizeof (emm_security_context_t));
  } else {
    memset (emm_ctx->security, 0, sizeof (emm_security_context_t));
  }

  if (emm_ctx->security) {
    emm_ctx->security->type = EMM_KSI_NOT_AVAILABLE;
    emm_ctx->security->selected_algorithms.encryption = NAS_SECURITY_ALGORITHMS_EEA0;
    emm_ctx->security->selected_algorithms.integrity = NAS_SECURITY_ALGORITHMS_EIA0;
  } else {
    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to create security context\n");
    emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    /*
     * Do not accept the UE to attach to the network
     */
    rc = _emm_attach_reject (emm_ctx);
    LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }

  /*
   * Initialize the security mode control procedure
   */
  rc = emm_proc_security_mode_control (emm_ctx->ue_id, 0,        // TODO: eksi != 0
                                       emm_ctx->eea, emm_ctx->eia, emm_ctx->ucs2, emm_ctx->uea,
                                       emm_ctx->uia, emm_ctx->gea, emm_ctx->umts_present, emm_ctx->gprs_present,
                                       _emm_attach, _emm_attach_release, _emm_attach_release);

  if (rc != RETURNok) {
    /*
     * Failed to initiate the security mode control procedure
     */
    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to initiate security mode control procedure\n");
    emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    /*
     * Do not accept the UE to attach to the network
     */
    rc = _emm_attach_reject (emm_ctx);
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
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
static int
_emm_attach (
  void *args)
{
  LOG_FUNC_IN (LOG_NAS_EMM);
  esm_sap_t                               esm_sap = {0};
  int                                     rc = RETURNerror;
  emm_data_context_t                     *emm_ctx = (emm_data_context_t *) (args);

  LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Attach UE (ue_id=" NAS_UE_ID_FMT ")\n", emm_ctx->ue_id);
  /*
   * 3GPP TS 24.401, Figure 5.3.2.1-1, point 5a
   * At this point, all NAS messages shall be protected by the NAS security
   * functions (integrity and ciphering) indicated by the MME unless the UE
   * is emergency attached and not successfully authenticated.
   */
  /*
   * Notify ESM that PDN connectivity is requested
   */
  esm_sap.primitive = ESM_PDN_CONNECTIVITY_REQ;
  esm_sap.is_standalone = false;
  esm_sap.ue_id = emm_ctx->ue_id;
  esm_sap.ctx = emm_ctx;
  esm_sap.recv = &emm_ctx->esm_msg;
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
    attach_data_t                          *data = (attach_data_t *) CALLOC_CHECK (1, sizeof (attach_data_t));

    if (data) {
      /*
       * Setup ongoing EMM procedure callback functions
       */
      rc = emm_proc_common_initialize (emm_ctx->ue_id, NULL, NULL, NULL, _emm_attach_abort, data);

      if (rc != RETURNok) {
        LOG_WARNING (LOG_NAS_EMM, "Failed to initialize EMM callback functions\n");
        FREE_CHECK (data);
        LOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
      }

      /*
       * Set the UE identifier
       */
      data->ue_id = emm_ctx->ue_id;
      /*
       * Reset the retransmission counter
       */
      data->retransmission_count = 0;
#if ORIGINAL_CODE
      /*
       * Setup the ESM message container
       */
      data->esm_msg.value = (uint8_t *) MALLOC_CHECK (esm_sap.send.length);

      if (data->esm_msg.value) {
        data->esm_msg.length = esm_sap.send.length;
        memcpy (data->esm_msg.value, esm_sap.send.value, esm_sap.send.length);
      } else {
        data->esm_msg.length = 0;
      }

      /*
       * Send attach accept message to the UE
       */
      rc = _emm_attach_accept (emm_ctx, data);

      if (rc != RETURNerror) {
        if (emm_ctx->guti_is_new && emm_ctx->old_guti) {
          /*
           * Implicit GUTI reallocation;
           * Notify EMM that common procedure has been initiated
           */
          emm_sap_t                               emm_sap = {0};

          emm_sap.primitive = EMMREG_COMMON_PROC_REQ;
          emm_sap.u.emm_reg.ue_id = data->ue_id;
          rc = emm_sap_send (&emm_sap);
        }
      }
#else
      rc = RETURNok;
#endif
    }
  } else if (esm_sap.err != ESM_SAP_DISCARDED) {
    /*
     * The attach procedure failed due to an ESM procedure failure
     */
    emm_ctx->emm_cause = EMM_CAUSE_ESM_FAILURE;

    /*
     * Setup the ESM message container to include PDN Connectivity Reject
     * message within the Attach Reject message
     */
    if (emm_ctx->esm_msg.length > 0) {
      FREE_CHECK (emm_ctx->esm_msg.value);
    }

    emm_ctx->esm_msg.value = (uint8_t *) MALLOC_CHECK (esm_sap.send.length);

    if (emm_ctx->esm_msg.value) {
      emm_ctx->esm_msg.length = esm_sap.send.length;
      memcpy (emm_ctx->esm_msg.value, esm_sap.send.value, esm_sap.send.length);
      /*
       * Send Attach Reject message
       */
      rc = _emm_attach_reject (emm_ctx);
    } else {
      emm_ctx->esm_msg.length = 0;
    }
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
    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to respond to Attach Request\n");
    emm_ctx->emm_cause = EMM_CAUSE_PROTOCOL_ERROR;
    /*
     * Do not accept the UE to attach to the network
     */
    rc = _emm_attach_reject (emm_ctx);
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

int
emm_cn_wrapper_attach_accept (
  emm_data_context_t * emm_ctx,
  void *data)
{
  return _emm_attach_accept (emm_ctx, (attach_data_t *) data);
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
static int
_emm_attach_accept (
  emm_data_context_t * emm_ctx,
  attach_data_t * data)
{
  LOG_FUNC_IN (LOG_NAS_EMM);
  emm_sap_t                               emm_sap = {0};
  int                                     i = 0;
  int                                     rc = RETURNerror;


  // may be caused by timer not stopped when deleted context
  if (emm_ctx) {
    /*
     * Notify EMM-AS SAP that Attach Accept message together with an Activate
     * Default EPS Bearer Context Request message has to be sent to the UE
     */
    emm_sap.primitive = EMMAS_ESTABLISH_CNF;
    emm_sap.u.emm_as.u.establish.ue_id = emm_ctx->ue_id;

    if (emm_ctx->guti_is_new && emm_ctx->old_guti) {
      /*
       * Implicit GUTI reallocation;
       * include the new assigned GUTI in the Attach Accept message
       */
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Implicit GUTI reallocation, include the new assigned GUTI in the Attach Accept message\n");
      emm_sap.u.emm_as.u.establish.eps_id.guti = emm_ctx->old_guti;
      emm_sap.u.emm_as.u.establish.new_guti = emm_ctx->guti;
    } else if (emm_ctx->guti_is_new && emm_ctx->guti) {
      /*
       * include the new assigned GUTI in the Attach Accept message
       */
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Include the new assigned GUTI in the Attach Accept message\n");
      emm_sap.u.emm_as.u.establish.eps_id.guti = emm_ctx->guti;
      emm_sap.u.emm_as.u.establish.new_guti = emm_ctx->guti;
    } else {
      emm_sap.u.emm_as.u.establish.eps_id.guti = emm_ctx->guti;
//#pragma message  "TEST LG FORCE GUTI IE IN ATTACH ACCEPT"
      emm_sap.u.emm_as.u.establish.new_guti = emm_ctx->guti;
      //emm_sap.u.emm_as.u.establish.new_guti  = NULL;
    }

    mme_api_notify_new_guti (emm_ctx->ue_id, emm_ctx->guti);     // LG
    emm_sap.u.emm_as.u.establish.tai_list.list_type = emm_ctx->tai_list.list_type;
    emm_sap.u.emm_as.u.establish.tai_list.n_tais    = emm_ctx->tai_list.n_tais;
    for (i=0; i < emm_ctx->tai_list.n_tais; i++) {
      memcpy(&emm_sap.u.emm_as.u.establish.tai_list.tai[i], &emm_ctx->tai_list.tai[i], sizeof(tai_t));
    }
    emm_sap.u.emm_as.u.establish.nas_info = EMM_AS_NAS_INFO_ATTACH;
    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data (&emm_sap.u.emm_as.u.establish.sctx, emm_ctx->security, false, true);
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - encryption = 0x%X \n", emm_sap.u.emm_as.u.establish.encryption);
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - integrity  = 0x%X \n", emm_sap.u.emm_as.u.establish.integrity);
    emm_sap.u.emm_as.u.establish.encryption = emm_ctx->security->selected_algorithms.encryption;
    emm_sap.u.emm_as.u.establish.integrity = emm_ctx->security->selected_algorithms.integrity;
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - encryption = 0x%X (0x%X)\n", emm_sap.u.emm_as.u.establish.encryption, emm_ctx->security->selected_algorithms.encryption);
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - integrity  = 0x%X (0x%X)\n", emm_sap.u.emm_as.u.establish.integrity, emm_ctx->security->selected_algorithms.integrity);
    /*
     * Get the activate default EPS bearer context request message to
     * transfer within the ESM container of the attach accept message
     */
    emm_sap.u.emm_as.u.establish.nas_msg = data->esm_msg;
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - nas_msg  src size = %d nas_msg  dst size = %d \n", data->esm_msg.length, emm_sap.u.emm_as.u.establish.nas_msg.length);
    rc = emm_sap_send (&emm_sap);

    if (RETURNerror != rc) {
      if (emm_ctx->T3450.id != NAS_TIMER_INACTIVE_ID) {
        /*
         * Re-start T3450 timer
         */
        emm_ctx->T3450.id = nas_timer_restart (emm_ctx->T3450.id);
        MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 restarted UE " NAS_UE_ID_FMT "", data->ue_id);
      } else {
        /*
         * Start T3450 timer
         */
        emm_ctx->T3450.id = nas_timer_start (emm_ctx->T3450.sec, _emm_attach_t3450_handler, data);
        MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 started UE " NAS_UE_ID_FMT " ", data->ue_id);
      }

      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Timer T3450 (%d) expires in %ld seconds\n", emm_ctx->T3450.id, emm_ctx->T3450.sec);
    }
  } else {
    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - emm_ctx NULL\n");
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
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
  int ksi,
  GUTI_t * guti,
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
  LOG_FUNC_IN (LOG_NAS_EMM);

  /*
   * Emergency bearer services indicator
   */
  if ((type == EMM_ATTACH_TYPE_EMERGENCY) != ctx->is_emergency) {
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: EMM_ATTACH_TYPE_EMERGENCY \n");
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  /*
   * Security key set identifier
   */
  if (ksi != ctx->ksi) {
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: ksi %u/%u (ctxt)\n", ksi, ctx->ksi);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  /*
   * Supported EPS encryption algorithms
   */
  if (eea != ctx->eea) {
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: eea 0x%x/0x%x (ctxt)\n", eea, ctx->eea);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  /*
   * Supported EPS integrity algorithms
   */
  if (eia != ctx->eia) {
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: eia 0x%x/0x%x (ctxt)\n", eia, ctx->eia);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if (umts_present != ctx->umts_present) {
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: umts_present %u/%u (ctxt)\n", umts_present, ctx->umts_present);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((ctx->umts_present) && (umts_present)) {
    if (ucs2 != ctx->ucs2) {
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: ucs2 %u/%u (ctxt)\n", ucs2, ctx->ucs2);
      LOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }

    /*
     * Supported UMTS encryption algorithms
     */
    if (uea != ctx->uea) {
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: uea 0x%x/0x%x (ctxt)\n", uea, ctx->uea);
      LOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }

    /*
     * Supported UMTS integrity algorithms
     */
    if (uia != ctx->uia) {
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: uia 0x%x/0x%x (ctxt)\n", uia, ctx->uia);
      LOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  if (gprs_present != ctx->gprs_present) {
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: gprs_present %u/%u (ctxt)\n", gprs_present, ctx->gprs_present);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((ctx->gprs_present) && (gprs_present)) {
    if (gea != ctx->gea) {
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: gea 0x%x/0x%x (ctxt)\n", gea, ctx->gea);
      LOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  /*
   * The GUTI if provided by the UE
   */
  if ((guti) && (!ctx->guti)) {
    char                                    guti_str[GUTI2STR_MAX_LENGTH];

    GUTI2STR (guti, guti_str, GUTI2STR_MAX_LENGTH);
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: guti %s/NULL (ctxt)\n", guti_str);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((!guti) && (ctx->guti)) {
    char                                    guti_str[GUTI2STR_MAX_LENGTH];

    GUTI2STR (guti, guti_str, GUTI2STR_MAX_LENGTH);
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: guti NULL/%s (ctxt)\n", guti_str);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((guti) && (ctx->guti)) {
    if (guti->m_tmsi != ctx->guti->m_tmsi) {
      char                                    guti_str[GUTI2STR_MAX_LENGTH];
      char                                    guti2_str[GUTI2STR_MAX_LENGTH];

      GUTI2STR (guti, guti_str, GUTI2STR_MAX_LENGTH);
      GUTI2STR (ctx->guti, guti2_str, GUTI2STR_MAX_LENGTH);
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: guti/m_tmsi %s/%s (ctxt)\n", guti_str, guti2_str);
      LOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
    // prob with memcmp
    //memcmp(&guti->gummei, &ctx->guti->gummei, sizeof(gummei_t)) != 0 ) {
    if ((guti->gummei.mme_code != ctx->guti->gummei.mme_code) ||
        (guti->gummei.mme_gid != ctx->guti->gummei.mme_gid) ||
        (guti->gummei.plmn.mcc_digit1 != ctx->guti->gummei.plmn.mcc_digit1) ||
        (guti->gummei.plmn.mcc_digit2 != ctx->guti->gummei.plmn.mcc_digit2) ||
        (guti->gummei.plmn.mcc_digit3 != ctx->guti->gummei.plmn.mcc_digit3) ||
        (guti->gummei.plmn.mnc_digit1 != ctx->guti->gummei.plmn.mnc_digit1) || (guti->gummei.plmn.mnc_digit2 != ctx->guti->gummei.plmn.mnc_digit2) || (guti->gummei.plmn.mnc_digit3 != ctx->guti->gummei.plmn.mnc_digit3)) {
      char                                    guti_str[GUTI2STR_MAX_LENGTH];
      char                                    guti2_str[GUTI2STR_MAX_LENGTH];

      GUTI2STR (guti, guti_str, GUTI2STR_MAX_LENGTH);
      GUTI2STR (ctx->guti, guti2_str, GUTI2STR_MAX_LENGTH);
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: guti/gummei %s/%s (ctxt)\n", guti_str, guti2_str);
      LOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  /*
   * The IMSI if provided by the UE
   */
  if ((imsi) && (!ctx->imsi)) {
    char                                    imsi_str[16];

    NAS_IMSI2STR (imsi, imsi_str, 16);
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imsi %s/NULL (ctxt)\n", imsi_str);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((!imsi) && (ctx->imsi)) {
    char                                    imsi_str[16];

    NAS_IMSI2STR (ctx->imsi, imsi_str, 16);
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imsi NULL/%s (ctxt)\n", imsi_str);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((imsi) && (ctx->imsi)) {
    if (memcmp (imsi, ctx->imsi, sizeof (imsi_t)) != 0) {
      char                                    imsi_str[16];
      char                                    imsi2_str[16];

      NAS_IMSI2STR (imsi, imsi_str, 16);
      NAS_IMSI2STR (ctx->imsi, imsi2_str, 16);
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imsi %s/%s (ctxt)\n", imsi_str, imsi2_str);
      LOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  /*
   * The IMEI if provided by the UE
   */
  if ((imei) && (!ctx->imei)) {
    char                                    imei_str[16];

    NAS_IMEI2STR (imei, imei_str, 16);
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imei %s/NULL (ctxt)\n", imei_str);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((!imei) && (ctx->imei)) {
    char                                    imei_str[16];

    NAS_IMEI2STR (ctx->imei, imei_str, 16);
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imei NULL/%s (ctxt)\n", imei_str);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((imei) && (ctx->imei)) {
    if (memcmp (imei, ctx->imei, sizeof (imei_t)) != 0) {
      char                                    imei_str[16];
      char                                    imei2_str[16];

      NAS_IMEI2STR (imei, imei_str, 16);
      NAS_IMEI2STR (ctx->imei, imei2_str, 16);
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_attach_have_changed: imei %s/%s (ctxt)\n", imei_str, imei2_str);
      LOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, false);
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
  int ksi,
  GUTI_t * guti,
  imsi_t * imsi,
  imei_t * imei,
  const tai_t   * const originating_tai,
  int eea,
  int eia,
  int ucs2,
  int uea,
  int uia,
  int gea,
  int umts_present,
  int gprs_present,
  const OctetString * esm_msg_pP)
{
  int                                     mnc_length = 0;
  int                                     i, j;

  LOG_FUNC_IN (LOG_NAS_EMM);
  /*
   * UE identifier
   */
  ctx->ue_id = ue_id;
  /*
   * Emergency bearer services indicator
   */
  ctx->is_emergency = (type == EMM_ATTACH_TYPE_EMERGENCY);
  /*
   * Security key set identifier
   */
  ctx->ksi = ksi;
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

  /*
   * The GUTI if provided by the UE
   */
  if (guti) {
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - GUTI NOT NULL\n");

    if (ctx->guti == NULL) {
      ctx->guti = (GUTI_t *) CALLOC_CHECK (1, sizeof (GUTI_t));
      memcpy (ctx->guti, guti, sizeof (GUTI_t));
      OAI_GCC_DIAG_OFF(int-to-pointer-cast);
      obj_hashtable_ts_insert (_emm_data.ctx_coll_guti, (const void *const)ctx->guti, sizeof (*guti), (void *)ctx->ue_id);
      OAI_GCC_DIAG_ON(int-to-pointer-cast);
      LOG_INFO (LOG_NAS_EMM,
                 "EMM-CTX - put in ctx_coll_guti  guti provided by UE, UE id "
                 NAS_UE_ID_FMT " PLMN    %x%x%x%x%x%x\n", ctx->ue_id,
                 ctx->guti->gummei.plmn.mcc_digit1, ctx->guti->gummei.plmn.mcc_digit2, ctx->guti->gummei.plmn.mnc_digit3, ctx->guti->gummei.plmn.mnc_digit1, ctx->guti->gummei.plmn.mnc_digit2, ctx->guti->gummei.plmn.mcc_digit3);
      LOG_INFO (LOG_NAS_EMM, "EMM-CTX - put in ctx_coll_guti  guti provided by UE, UE id " NAS_UE_ID_FMT " mme_gid  %04x\n", ctx->ue_id, ctx->guti->gummei.mme_gid);
      LOG_INFO (LOG_NAS_EMM, "EMM-CTX - put in ctx_coll_guti  guti provided by UE, UE id " NAS_UE_ID_FMT " mme_code %01x\n", ctx->ue_id, ctx->guti->gummei.mme_code);
      LOG_INFO (LOG_NAS_EMM, "EMM-CTX - put in ctx_coll_guti  guti provided by UE, UE id " NAS_UE_ID_FMT " m_tmsi  %08x\n", ctx->ue_id, ctx->guti->m_tmsi);
    } else {
      // TODO Think about what is _emm_data.ctx_coll_guti
      memcpy (ctx->guti, guti, sizeof (GUTI_t));
    }
  } else {
    if (!ctx->guti ) {
      ctx->guti = (GUTI_t *) CALLOC_CHECK (1, sizeof (GUTI_t));
    } else {
      unsigned int                           *emm_ue_id = NULL;

      obj_hashtable_ts_remove (_emm_data.ctx_coll_guti, (const void *)(ctx->guti), sizeof (*ctx->guti), (void **)&emm_ue_id);
    }

    if ((ctx->guti) && (imsi)) {
      //ctx->tac = mme_config.served_tai.tac[0];
      ctx->guti->gummei.mme_code = mme_config.gummei.mmec[0];
      ctx->guti->gummei.mme_gid = mme_config.gummei.mme_gid[0];
      ctx->guti->m_tmsi = (uintptr_t)ctx;

      //
      if (originating_tai) {
        ctx->guti->gummei.plmn.mcc_digit1 = originating_tai->plmn.mcc_digit1;
        ctx->guti->gummei.plmn.mcc_digit2 = originating_tai->plmn.mcc_digit2;
        ctx->guti->gummei.plmn.mcc_digit3 = originating_tai->plmn.mcc_digit3;
        ctx->guti->gummei.plmn.mnc_digit1 = originating_tai->plmn.mnc_digit1;
        ctx->guti->gummei.plmn.mnc_digit2 = originating_tai->plmn.mnc_digit2;
        ctx->guti->gummei.plmn.mnc_digit3 = originating_tai->plmn.mnc_digit3;
        ctx->guti_is_new = true;
        LOG_INFO (LOG_NAS_EMM,
                   "EMM-PROC  - Assign GUTI from originating TAI %01X%01X%01X.%01X%01X.%04X.%02X.%08X to emm_data_context\n",
                   ctx->guti->gummei.plmn.mcc_digit1, ctx->guti->gummei.plmn.mcc_digit2, ctx->guti->gummei.plmn.mcc_digit3,
                   ctx->guti->gummei.plmn.mnc_digit1, ctx->guti->gummei.plmn.mnc_digit2,
                   ctx->guti->gummei.mme_gid, ctx->guti->gummei.mme_code, ctx->guti->m_tmsi);
      } else  {
        mnc_length = mme_config_find_mnc_length (imsi->u.num.digit1, imsi->u.num.digit2, imsi->u.num.digit3, imsi->u.num.digit4, imsi->u.num.digit5, imsi->u.num.digit6);

        if ((mnc_length == 2) || (mnc_length == 3)) {
          ctx->guti->gummei.plmn.mcc_digit1 = imsi->u.num.digit1;
          ctx->guti->gummei.plmn.mcc_digit2 = imsi->u.num.digit2;
          ctx->guti->gummei.plmn.mcc_digit3 = imsi->u.num.digit3;

          if (mnc_length == 2) {
            ctx->guti->gummei.plmn.mnc_digit1 = imsi->u.num.digit4;
            ctx->guti->gummei.plmn.mnc_digit2 = imsi->u.num.digit5;
            ctx->guti->gummei.plmn.mnc_digit3 = 15;
           LOG_WARNING (LOG_NAS_EMM,
               "EMM-PROC  - Assign GUTI from IMSI %01X%01X%01X.%01X%01X.%04X.%02X.%08X to emm_data_context\n",
               ctx->guti->gummei.plmn.mcc_digit1, ctx->guti->gummei.plmn.mcc_digit2, ctx->guti->gummei.plmn.mcc_digit3,
               ctx->guti->gummei.plmn.mnc_digit1, ctx->guti->gummei.plmn.mnc_digit2, ctx->guti->gummei.mme_gid,
               ctx->guti->gummei.mme_code, ctx->guti->m_tmsi);
          } else {
            ctx->guti->gummei.plmn.mnc_digit1 = imsi->u.num.digit5;
            ctx->guti->gummei.plmn.mnc_digit2 = imsi->u.num.digit6;
            ctx->guti->gummei.plmn.mnc_digit3 = imsi->u.num.digit4;
            LOG_WARNING (LOG_NAS_EMM,
                "EMM-PROC  - Assign GUTI from IMSI %01X%01X%01X.%01X%01X%01X.%04X.%02X.%08X to emm_data_context\n",
                ctx->guti->gummei.plmn.mcc_digit1, ctx->guti->gummei.plmn.mcc_digit2, ctx->guti->gummei.plmn.mcc_digit3,
                ctx->guti->gummei.plmn.mnc_digit1, ctx->guti->gummei.plmn.mnc_digit2, ctx->guti->gummei.plmn.mnc_digit3,
                ctx->guti->gummei.mme_gid, ctx->guti->gummei.mme_code, ctx->guti->m_tmsi);
          }
        } else {
          // QUICK FIX: PICK THE FIRST PLMN SERVED BY THE MME
          ctx->guti->gummei.plmn.mcc_digit1 = _emm_data.conf.tai_list.tai[0].plmn.mcc_digit1;
          ctx->guti->gummei.plmn.mcc_digit2 = _emm_data.conf.tai_list.tai[0].plmn.mcc_digit2;
          ctx->guti->gummei.plmn.mcc_digit3 = _emm_data.conf.tai_list.tai[0].plmn.mcc_digit3;
          ctx->guti->gummei.plmn.mnc_digit1 = _emm_data.conf.tai_list.tai[0].plmn.mnc_digit1;
          ctx->guti->gummei.plmn.mnc_digit2 = _emm_data.conf.tai_list.tai[0].plmn.mnc_digit2;
          ctx->guti->gummei.plmn.mnc_digit3 = _emm_data.conf.tai_list.tai[0].plmn.mnc_digit3;
        }
      }
      obj_hashtable_ts_insert (_emm_data.ctx_coll_guti, (const void *const)(ctx->guti), sizeof (*ctx->guti), (void *)((uintptr_t)ctx->ue_id));
      LOG_INFO (LOG_NAS_EMM,
          "EMM-CTX - put in ctx_coll_guti guti generated by NAS, UE id "
          NAS_UE_ID_FMT " PLMN    %x%x%x%x%x%x\n", ctx->ue_id,
          ctx->guti->gummei.plmn.mcc_digit1, ctx->guti->gummei.plmn.mcc_digit2, ctx->guti->gummei.plmn.mnc_digit3, ctx->guti->gummei.plmn.mnc_digit1, ctx->guti->gummei.plmn.mnc_digit2, ctx->guti->gummei.plmn.mcc_digit3);
      LOG_INFO (LOG_NAS_EMM, "EMM-CTX - put in ctx_coll_guti guti generated by NAS, UE id " NAS_UE_ID_FMT " mme_gid  %04x\n", ctx->ue_id, ctx->guti->gummei.mme_gid);
      LOG_INFO (LOG_NAS_EMM, "EMM-CTX - put in ctx_coll_guti guti generated by NAS, UE id " NAS_UE_ID_FMT " mme_code %01x\n", ctx->ue_id, ctx->guti->gummei.mme_code);
      LOG_INFO (LOG_NAS_EMM, "EMM-CTX - put in ctx_coll_guti guti generated by NAS, UE id " NAS_UE_ID_FMT " m_tmsi  %08x\n", ctx->ue_id, ctx->guti->m_tmsi);
      LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Set ctx->guti_is_new to emm_data_context\n");

    } else {
      LOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }
  }

  // build TAI list
  if (0 == ctx->tai_list.n_tais) {
    j = 0;
    for (i=0; i < _emm_data.conf.tai_list.n_tais; i++) {
      if ((_emm_data.conf.tai_list.tai[i].plmn.mcc_digit1 == ctx->guti->gummei.plmn.mcc_digit1) &&
          (_emm_data.conf.tai_list.tai[i].plmn.mcc_digit2 == ctx->guti->gummei.plmn.mcc_digit2) &&
          (_emm_data.conf.tai_list.tai[i].plmn.mcc_digit3 == ctx->guti->gummei.plmn.mcc_digit3) &&
          (_emm_data.conf.tai_list.tai[i].plmn.mnc_digit1 == ctx->guti->gummei.plmn.mnc_digit1) &&
          (_emm_data.conf.tai_list.tai[i].plmn.mnc_digit2 == ctx->guti->gummei.plmn.mnc_digit2) &&
          (_emm_data.conf.tai_list.tai[i].plmn.mnc_digit3 == ctx->guti->gummei.plmn.mnc_digit3) ) {

      }
      ctx->tai_list.tai[j].plmn.mcc_digit1 = ctx->guti->gummei.plmn.mcc_digit1;
      ctx->tai_list.tai[j].plmn.mcc_digit2 = ctx->guti->gummei.plmn.mcc_digit2;
      ctx->tai_list.tai[j].plmn.mcc_digit3 = ctx->guti->gummei.plmn.mcc_digit3;
      ctx->tai_list.tai[j].plmn.mnc_digit1 = ctx->guti->gummei.plmn.mnc_digit1;
      ctx->tai_list.tai[j].plmn.mnc_digit2 = ctx->guti->gummei.plmn.mnc_digit2;
      ctx->tai_list.tai[j].plmn.mnc_digit3 = ctx->guti->gummei.plmn.mnc_digit3;
      ctx->tai_list.tai[j].tac            = _emm_data.conf.tai_list.tai[i].tac;
      j += 1;
      if (15 == ctx->guti->gummei.plmn.mnc_digit3) { // mnc length 2
        LOG_INFO (LOG_NAS_EMM, "UE registered to TAC %d%d%d.%d%d:%d\n",
            ctx->guti->gummei.plmn.mcc_digit1, ctx->guti->gummei.plmn.mcc_digit2, ctx->guti->gummei.plmn.mcc_digit3,
            ctx->guti->gummei.plmn.mnc_digit1, ctx->guti->gummei.plmn.mnc_digit2, _emm_data.conf.tai_list.tai[i].tac);
      } else {
        LOG_INFO (LOG_NAS_EMM, "UE registered to TAC %d%d%d.%d%d%d:%d\n",
            ctx->guti->gummei.plmn.mcc_digit1, ctx->guti->gummei.plmn.mcc_digit2, ctx->guti->gummei.plmn.mcc_digit3,
            ctx->guti->gummei.plmn.mnc_digit1, ctx->guti->gummei.plmn.mnc_digit2, ctx->guti->gummei.plmn.mnc_digit3,
            _emm_data.conf.tai_list.tai[i].tac);
      }
    }
    ctx->tai_list.n_tais = j;
  }
  /*
   * The IMSI if provided by the UE
   */
  if (imsi) {
    if (!ctx->imsi) {
      ctx->imsi = (imsi_t *) MALLOC_CHECK (sizeof (imsi_t));
    }

    if (ctx->imsi) {
      memcpy (ctx->imsi, imsi, sizeof (imsi_t));
    } else {
      LOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }
  }

  /*
   * The IMEI if provided by the UE
   */
  if (imei) {
    if (!ctx->imei) {
      ctx->imei = (imei_t *) MALLOC_CHECK (sizeof (imei_t));
    }

    if (ctx->imei) {
      memcpy (ctx->imei, imei, sizeof (imei_t));
    } else {
      LOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }
  }

  /*
   * The ESM message contained within the attach request
   */
  if (esm_msg_pP->length > 0) {
    if (ctx->esm_msg.value) {
      FREE_CHECK (ctx->esm_msg.value);
      ctx->esm_msg.value = NULL;
      ctx->esm_msg.length = 0;
    }

    ctx->esm_msg.value = (uint8_t *) CALLOC_CHECK (esm_msg_pP->length, sizeof(uint8_t));

    if (ctx->esm_msg.value) {
      memcpy ((char *)ctx->esm_msg.value, (char *)esm_msg_pP->value, esm_msg_pP->length);
    } else {
      LOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }
  }

  ctx->esm_msg.length = esm_msg_pP->length;
  /*
   * Attachment indicator
   */
  ctx->is_attached = false;
  LOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

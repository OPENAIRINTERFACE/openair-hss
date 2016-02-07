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

#include "emm_proc.h"
#include "log.h"
#include "nas_timer.h"

#include "emmData.h"

#include "emm_sap.h"
#include "emm_cause.h"
#include "EpsUpdateResult.h"
#include "msc.h"

#include <string.h>             // memcmp, memcpy
#include <stdlib.h>             // MALLOC_CHECK, FREE_CHECK

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
     Internal data handled by the tracking area update procedure in the MME
   --------------------------------------------------------------------------
*/
/*
   Internal data used for attach procedure
*/
typedef struct tau_accept_data_s {
  unsigned int                            ueid; /* UE identifier        */
#define TAU_COUNTER_MAX  5
  unsigned int                            retransmission_count; /* Retransmission counter   */

} tau_accept_data_t;

int                                     emm_network_capability_have_changed (
  const emm_data_context_t * ctx,
  int eea,
  int eia,
  int ucs2,
  int uea,
  int uia,
  int gea,
  int umts_present,
  int gprs_present);

int                                     emm_proc_tracking_area_update_request (
  nas_ue_id_t ueid,
  const tracking_area_update_request_msg * msg,
  int *emm_cause,
  const nas_message_decode_status_t  * decode_status);

static int                              _emm_tracking_area_update (
    void *args);
static int                              _emm_tracking_area_update_security (
  void *args);
static int                              _emm_tracking_area_update_reject (
  void *args);
static int                              _emm_tracking_area_update_accept (
  emm_data_context_t * emm_ctx,
  tau_accept_data_t * data);
static int                              _emm_tracking_area_update_abort (
  void *args);


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/
int
emm_network_capability_have_changed (
  const emm_data_context_t * ctx,
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
   * Supported EPS encryption algorithms
   */
  if (eea != ctx->eea) {
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_network_capability_have_changed: eea 0x%x/0x%x (ctxt)", eea, ctx->eea);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  /*
   * Supported EPS integrity algorithms
   */
  if (eia != ctx->eia) {
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_network_capability_have_changed: eia 0x%x/0x%x (ctxt)", eia, ctx->eia);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if (umts_present != ctx->umts_present) {
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_network_capability_have_changed: umts_present %u/%u (ctxt)", umts_present, ctx->umts_present);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((ctx->umts_present) && (umts_present)) {
    if (ucs2 != ctx->ucs2) {
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_network_capability_have_changed: ucs2 %u/%u (ctxt)", ucs2, ctx->ucs2);
      LOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }

    /*
     * Supported UMTS encryption algorithms
     */
    if (uea != ctx->uea) {
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_network_capability_have_changed: uea 0x%x/0x%x (ctxt)", uea, ctx->uea);
      LOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }

    /*
     * Supported UMTS integrity algorithms
     */
    if (uia != ctx->uia) {
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_network_capability_have_changed: uia 0x%x/0x%x (ctxt)", uia, ctx->uia);
      LOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }

  if (gprs_present != ctx->gprs_present) {
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_network_capability_have_changed: gprs_present %u/%u (ctxt)", gprs_present, ctx->gprs_present);
    LOG_FUNC_RETURN (LOG_NAS_EMM, true);
  }

  if ((ctx->gprs_present) && (gprs_present)) {
    if (gea != ctx->gea) {
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  _emm_network_capability_have_changed: gea 0x%x/0x%x (ctxt)", gea, ctx->gea);
      LOG_FUNC_RETURN (LOG_NAS_EMM, true);
    }
  }
  LOG_FUNC_RETURN (LOG_NAS_EMM, false);
}


int
emm_proc_tracking_area_update_request (
  nas_ue_id_t ueid,
  const tracking_area_update_request_msg * msg,
  int *emm_cause,
  const nas_message_decode_status_t  * decode_status)
{
  LOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  emm_data_context_t                     *ue_ctx = NULL;

  /*
   * Get the UE's EMM context if it exists
   */

  ue_ctx = emm_data_context_get (&_emm_data, ueid);

  // May be the MME APP module did not find the context, but if we have the GUTI, we may find it
  if (! ue_ctx) {
    if (EPS_MOBILE_IDENTITY_GUTI == msg->oldguti.guti.typeofidentity) {
      GUTI_t guti;
      guti.m_tmsi = msg->oldguti.guti.mtmsi;
      guti.gummei.MMEgid = msg->oldguti.guti.mmegroupid;
      guti.gummei.MMEcode = msg->oldguti.guti.mmecode;
      guti.gummei.plmn.MCCdigit1 = msg->oldguti.guti.mccdigit1;
      guti.gummei.plmn.MCCdigit2 = msg->oldguti.guti.mccdigit2;
      guti.gummei.plmn.MCCdigit3 = msg->oldguti.guti.mccdigit3;
      guti.gummei.plmn.MNCdigit1 = msg->oldguti.guti.mncdigit1;
      guti.gummei.plmn.MNCdigit2 = msg->oldguti.guti.mncdigit2;
      guti.gummei.plmn.MNCdigit3 = msg->oldguti.guti.mncdigit3;

      ue_ctx = emm_data_context_get_by_guti (&_emm_data, &guti);

      if ( ue_ctx) {
        mme_api_notify_ue_id_changed(ue_ctx->ueid, ueid);
        // ue_id has changed
        emm_data_context_remove (&_emm_data, ue_ctx);
        // ue_id changed
        ue_ctx->ueid = ueid;
        // put context with right key
        emm_data_context_add (&_emm_data, ue_ctx);

        // update decode_status
        //if ( ue_ctx->security) {
        //  decode_status->security_context_available = 1;
        //}
        // ...anyway now for simplicity we reject the TAU (else have to re-do integrity checking on NAS msg)
        rc = emm_proc_tracking_area_update_reject (ueid, EMM_CAUSE_IMPLICITLY_DETACHED);
        LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
      } else {
        // NO S10
        rc = emm_proc_tracking_area_update_reject (ueid, EMM_CAUSE_IMPLICITLY_DETACHED);
        LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
      }
    }
  }
  /*
   * Requirements MME24.301R10_5.5.3.2.4_2
   */
  if (msg->presencemask & TRACKING_AREA_UPDATE_REQUEST_UE_NETWORK_CAPABILITY_IEI) {
    if ( ue_ctx) {
      if (! ue_ctx->ue_network_capability_ie) {
        ue_ctx->ue_network_capability_ie = MALLOC_CHECK(sizeof(UeNetworkCapability));
      }
      memcpy(ue_ctx->ue_network_capability_ie, &msg->uenetworkcapability, sizeof(UeNetworkCapability));
    }
  }
  if (msg->presencemask & TRACKING_AREA_UPDATE_REQUEST_MS_NETWORK_CAPABILITY_IEI) {
    if ( ue_ctx) {
      if (! ue_ctx->ms_network_capability_ie) {
        ue_ctx->ms_network_capability_ie = MALLOC_CHECK(sizeof(MsNetworkCapability));
      } else {
        FREE_OCTET_STRING(ue_ctx->ms_network_capability_ie->msnetworkcapabilityvalue);
      }
      DUP_OCTET_STRING(msg->msnetworkcapability.msnetworkcapabilityvalue, ue_ctx->ms_network_capability_ie->msnetworkcapabilityvalue);
    }
  }
  /*
   * Requirements MME24.301R10_5.5.3.2.4_3
   */
  if (msg->presencemask & TRACKING_AREA_UPDATE_REQUEST_UE_RADIO_CAPABILITY_INFORMATION_UPDATE_NEEDED_IEI) {
    if ( ue_ctx) {
      if (0 != msg->ueradiocapabilityinformationupdateneeded) {
        // delete the stored UE radio capability information if any
//#pragma message  "TODO: Actually UE radio capability information is not stored in EPC"
      }
    }
  }
  /*
   * Requirements MME24.301R10_5.5.3.2.4_4
   */
  if (msg->presencemask & TRACKING_AREA_UPDATE_REQUEST_DRX_PARAMETER_IEI) {
    if ( ue_ctx) {
      if (! ue_ctx->drx_parameter) {
        ue_ctx->drx_parameter = MALLOC_CHECK(sizeof(DrxParameter));
      }
      memcpy(ue_ctx->drx_parameter, &msg->drxparameter, sizeof(DrxParameter));
    }
  }
  /*
   * Requirement MME24.301R10_5.5.3.2.4_5a
   */
  if (msg->presencemask & TRACKING_AREA_UPDATE_REQUEST_EPS_BEARER_CONTEXT_STATUS_IEI) {
    if ( ue_ctx) {
      if (! ue_ctx->eps_bearer_context_status) {
        ue_ctx->eps_bearer_context_status = MALLOC_CHECK(sizeof(EpsBearerContextStatus));
      }
      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - TAU Request (ueid=" NAS_UE_ID_FMT ") IE EPS_BEARER_CONTEXT_STATUS 0x%04X", ueid, msg->epsbearercontextstatus);
      memcpy(ue_ctx->eps_bearer_context_status, &msg->epsbearercontextstatus, sizeof(EpsBearerContextStatus));
//#pragma message  "TODO Requirement MME24.301R10_5.5.3.2.4_5a: TAU Request: Deactivate EPS bearers if necessary (S11 Modify Bearer Request)"
    }
  }
  /*
   * Requirement MME24.301R10_5.5.3.2.4_6
   */
  if ( EPS_UPDATE_TYPE_PERIODIC_UPDATING == msg->epsupdatetype.epsupdatetypevalue) {
/*
 * MME24.301R10_5.5.3.2.4_6 Normal and periodic tracking area updating procedure accepted by the network UE - EPS update type
 * If the EPS update type IE included in the TRACKING AREA UPDATE REQUEST message indicates "periodic updating", and the UE was
 *  previously successfully attached for EPS and non-EPS services, subject to operator policies the MME should allocate a TAI
 *  list that does not span more than one location area.
 */
    //TODO

  }
  /*
   * Requirement MME24.301R10_5.5.3.2.4_12
   * MME24.301R10_5.5.3.2.4_12 Normal and periodic tracking area updating procedure accepted by the network UE - active flag
   * If the "active" flag is included in the TRACKING AREA UPDATE REQUEST message, the MME shall re-establish
   * the radio and S1 bearers for all active EPS bearer contexts..
   */
  if ( 0 < msg->epsupdatetype.activeflag) {
//#pragma message  "TODO Requirement MME24.301R10_5.5.3.2.4_12: Request: re-establish the radio and S1 bearers for all active EPS bearer contexts (S11 Modify Bearer Request)"

  } else {
    /*
     * Requirement MME24.301R10_5.5.3.2.4_13
     * MME24.301R10_5.5.3.2.4_13 Normal and periodic tracking area updating procedure accepted by the network UE - not active flag
     * If the "active" flag is not included in the TRACKING AREA UPDATE REQUEST message, the MME may also re-establish the radio and
     *  S1 bearers for all active EPS bearer contexts due to downlink pending data or downlink pending signalling.
     */
//#pragma message  "TODO Requirement MME24.301R10_5.5.3.2.4_13: Request: re-establish the radio and S1 bearers for all active EPS bearer contexts (S11 Modify Bearer Request)"
  }

  /*
   * MME24.301R10_5.5.3.2.3_3 TAU procedure: EMM common procedure initiation - emergency bearer
   * The MME may be configured to skip the authentication procedure even if no EPS security context is available and
   * proceed directly to the execution of the security mode control procedure as specified in subclause 5.4.3, during a
   * tracking area updating procedure for a UE that has only a PDN connection for emergency bearer services.
   */
  if ((! ue_ctx->security) &&
      (_emm_data.conf.eps_network_feature_support & EPS_NETWORK_FEATURE_SUPPORT_EMERGENCY_BEARER_SERVICES_IN_S1_MODE_SUPPORTED) &&
      (0 < ue_ctx->is_emergency)) {
    if ( ue_ctx->ue_network_capability_ie) {
      // overwrite previous values
      ue_ctx->eea = ue_ctx->ue_network_capability_ie->eea;
      ue_ctx->eia = ue_ctx->ue_network_capability_ie->eia;
      ue_ctx->ucs2 = ue_ctx->ue_network_capability_ie->ucs2;
      ue_ctx->uea = ue_ctx->ue_network_capability_ie->uea;
      ue_ctx->uia = ue_ctx->ue_network_capability_ie->uia;
//#pragma message  "TODO (clean gea code)"
      //ue_ctx->gea = ue_ctx->gea;
      ue_ctx->umts_present = ue_ctx->ue_network_capability_ie->umts_present;
      ue_ctx->gprs_present = ue_ctx->ue_network_capability_ie->gprs_present;
    }
    rc = emm_proc_security_mode_control (ue_ctx->ueid,
      0,        // TODO: eksi != 0
      ue_ctx->eea,
      ue_ctx->eia,
      ue_ctx->ucs2,
      ue_ctx->uea,
      ue_ctx->uia,
      ue_ctx->gea,
      ue_ctx->umts_present,
      ue_ctx->gprs_present,
      _emm_tracking_area_update_accept,
      _emm_tracking_area_update_reject,
      _emm_tracking_area_update_reject);
    LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  } /*else {
     *
     * MME24.301R10_5.5.3.2.3_4 TAU procedure: EMM common procedure initiation – no authentication procedure
     * The MME shall not initiate an EMM authentication procedure before completion of the tracking area updating
     * procedure, if the following conditions apply:
     * a) the UE initiated the tracking area updating procedure after handover or inter-system handover to S1 mode;
     * b) the target cell is a shared network cell; and
     *   -the UE has provided its GUTI in the Old GUTI IE or the Additional GUTI IE in the TRACKING AREA
     *   UPDATE REQUEST message, and the PLMN identity included in the GUTI is different from the selected
     *   PLMN identity of the target cell; or
     *   -the UE has mapped the P-TMSI and RAI into the Old GUTI IE and not included an Additional GUTI IE in
     *   the TRACKING AREA UPDATE REQUEST message, and the PLMN identity included in the RAI is
     *   different from the selected PLMN identity of the target cell.
     }
     */
  else {
    /*
     * MME24.301R10_5.5.3.2.3_2 TAU procedure: EMM common procedure initiation
     * During the tracking area updating procedure, the MME may initiate EMM common procedures, e.g. the EMM
     * authentication and security mode control procedures.
     */
    bool authentication_requested = false;
    authentication_requested = ((0 < decode_status->integrity_protected_message) && (0 == decode_status->mac_matched)) ||
        (0 == decode_status->integrity_protected_message) || (0 == decode_status->security_context_available);
    if (authentication_requested) {
      auth_vector_t                          *auth = &ue_ctx->vector;
      const OctetString                       loc_rand = { AUTH_RAND_SIZE, (uint8_t *) auth->rand };
      const OctetString                       autn = { AUTH_AUTN_SIZE, (uint8_t *) auth->autn };
      rc = emm_proc_authentication (ue_ctx, ue_ctx->ueid, 0,  // TODO: eksi != 0
                                  &loc_rand,
                                  &autn,
                                  _emm_tracking_area_update_security,
                                  _emm_tracking_area_update_reject,
                                  _emm_tracking_area_update_reject);
      LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
    } else {
      bool network_capability_have_changed = false;
      if ( ue_ctx->ue_network_capability_ie) {
        network_capability_have_changed = emm_network_capability_have_changed(ue_ctx,
            ue_ctx->ue_network_capability_ie->eea,
            ue_ctx->ue_network_capability_ie->eia,
            ue_ctx->ue_network_capability_ie->ucs2,
            ue_ctx->ue_network_capability_ie->uea,
            ue_ctx->ue_network_capability_ie->uia,
            ue_ctx->gea, // TODO
            ue_ctx->ue_network_capability_ie->umts_present,
            ue_ctx->ue_network_capability_ie->gprs_present);
        // overwrite previous values
        if (network_capability_have_changed) {
          ue_ctx->eea = ue_ctx->ue_network_capability_ie->eea;
          ue_ctx->eia = ue_ctx->ue_network_capability_ie->eia;
          ue_ctx->ucs2 = ue_ctx->ue_network_capability_ie->ucs2;
          ue_ctx->uea = ue_ctx->ue_network_capability_ie->uea;
          ue_ctx->uia = ue_ctx->ue_network_capability_ie->uia;
//#pragma message  "TODO (clean gea code)"
          //ue_ctx->gea = ue_ctx->gea;
          ue_ctx->umts_present = ue_ctx->ue_network_capability_ie->umts_present;
          ue_ctx->gprs_present = ue_ctx->ue_network_capability_ie->gprs_present;
        }
      }
      // keep things simple at the beginning
      //if (network_capability_have_changed)
      {
        rc = emm_proc_security_mode_control (ue_ctx->ueid,
            0,        // TODO: eksi != 0
            ue_ctx->eea,
            ue_ctx->eia,
            ue_ctx->ucs2,
            ue_ctx->uea,
            ue_ctx->uia,
            ue_ctx->gea,
            ue_ctx->umts_present,
            ue_ctx->gprs_present,
            _emm_tracking_area_update,
            _emm_tracking_area_update_reject,
            _emm_tracking_area_update_reject);
        LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
      } //else {
        // TAU Accept
      //}

    }
    LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }

  rc = emm_proc_tracking_area_update_reject (ueid, EMM_CAUSE_ILLEGAL_UE);
  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}
/****************************************************************************
 **                                                                        **
 ** Name:        emm_proc_tracking_area_update_reject()                    **
 **                                                                        **
 ** Description:                                                           **
 **                                                                        **
 ** Inputs:  ueid:              UE lower layer identifier                  **
 **                  emm_cause: EMM cause code to be reported              **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    RETURNok, RETURNerror                      **
 **                  Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_tracking_area_update_reject (
  nas_ue_id_t ueid,
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
  ue_ctx.ueid = ueid;
  /*
   * Update the EMM cause code
   */
  if (ueid > 0)
  {
    ue_ctx.emm_cause = emm_cause;
  } else {
    ue_ctx.emm_cause = EMM_CAUSE_ILLEGAL_UE;
  }

  /*
   * Do not accept attach request with protocol error
   */
  rc = _emm_tracking_area_update_reject (&ue_ctx);
  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

static int
_emm_tracking_area_update (
    void *args)
{
  LOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  emm_data_context_t                     *emm_ctx = (emm_data_context_t *) (args);
  tau_accept_data_t                      *data = (tau_accept_data_t *) CALLOC_CHECK (1, sizeof (tau_accept_data_t));

  if (data ) {
    /*
     * Setup ongoing EMM procedure callback functions
     */
    rc = emm_proc_common_initialize (emm_ctx->ueid, NULL, NULL, NULL, _emm_tracking_area_update_abort, data);

    if (rc != RETURNok) {
      LOG_WARNING (LOG_NAS_EMM, "Failed to initialize EMM callback functions");
      FREE_CHECK (data);
      LOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
    }

    /*
     * Set the UE identifier
     */
    data->ueid = emm_ctx->ueid;
    /*
     * Reset the retransmission counter
     */
    data->retransmission_count = 0;
    rc = RETURNok;

    rc = _emm_tracking_area_update_accept (emm_ctx, data);

  } else {
    /*
     * The TAU procedure failed
     */
    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to respond to TAU Request");
    emm_ctx->emm_cause = EMM_CAUSE_PROTOCOL_ERROR;
    rc = _emm_tracking_area_update_reject (emm_ctx);
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}
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
static void                            *
_emm_tau_t3450_handler (
  void *args)
{
  LOG_FUNC_IN (LOG_NAS_EMM);
  tau_accept_data_t                      *data = (tau_accept_data_t *) (args);

  // Requirement MME24.301R10_5.5.3.2.7_c Abnormal cases on the network side - T3450 time-out
  /*
   * Increment the retransmission counter
   */
  data->retransmission_count += 1;
  LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - T3450 timer expired, retransmission counter = %d", data->retransmission_count);
  /*
   * Get the UE's EMM context
   */
  emm_data_context_t *emm_ctx = emm_ctx = emm_data_context_get (&_emm_data, data->ueid);

  if (data->retransmission_count < TAU_COUNTER_MAX) {
    /*
     * Send attach accept message to the UE
     */
    _emm_tracking_area_update_accept (emm_ctx, data);
  } else {
    /*
     * Abort the attach procedure
     */
    _emm_tracking_area_update_abort (data);
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, NULL);
}

/** \fn void _emm_tracking_area_update_reject(void *args);
    \brief Performs the tracking area update procedure not accepted by the network.
     @param [in]args UE EMM context data
     @returns status of operation
*/
static int
_emm_tracking_area_update_security (
  void *args)
{
  LOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  emm_data_context_t                     *emm_ctx = (emm_data_context_t *) (args);

  LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Setup NAS security (ueid=" NAS_UE_ID_FMT ")", emm_ctx->ueid);

  /*
   * Create new NAS security context
   */
  if (emm_ctx->security == NULL) {
    emm_ctx->security = (emm_security_context_t *) MALLOC_CHECK (sizeof (emm_security_context_t));
  }

  if (emm_ctx->security) {
    memset (emm_ctx->security, 0, sizeof (emm_security_context_t));
    emm_ctx->security->type = EMM_KSI_NOT_AVAILABLE;
    emm_ctx->security->selected_algorithms.encryption = NAS_SECURITY_ALGORITHMS_EEA0;
    emm_ctx->security->selected_algorithms.integrity = NAS_SECURITY_ALGORITHMS_EIA0;
  } else {
    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to create security context");
    emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    /*
     * Do not accept the UE to attach to the network
     */
    rc = _emm_tracking_area_update_reject (emm_ctx);
    LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }

  /*
   * Initialize the security mode control procedure
   */
  rc = emm_proc_security_mode_control (emm_ctx->ueid, 0,        // TODO: eksi != 0
                                       emm_ctx->eea, emm_ctx->eia, emm_ctx->ucs2,
                                       emm_ctx->uea, emm_ctx->uia, emm_ctx->gea,
                                       emm_ctx->umts_present, emm_ctx->gprs_present,
                                       _emm_tracking_area_update,
                                       _emm_tracking_area_update_reject,
                                       _emm_tracking_area_update_reject);

  if (rc != RETURNok) {
    /*
     * Failed to initiate the security mode control procedure
     */
    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Failed to initiate security mode control procedure");
    emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    /*
     * Do not accept the UE to attach to the network
     */
    rc = _emm_tracking_area_update_reject (emm_ctx);
  }
  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/** \fn void _emm_tracking_area_update_reject(void *args);
    \brief Performs the tracking area update procedure not accepted by the network.
     @param [in]args UE EMM context data
     @returns status of operation
*/
static int
_emm_tracking_area_update_reject (
  void *args)
{
  LOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  emm_data_context_t                     *emm_ctx = (emm_data_context_t *) (args);

  if (emm_ctx) {
    emm_sap_t                               emm_sap = {0};

    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - EMM tracking area update procedure not accepted " "by the network (ueid=" NAS_UE_ID_FMT ", cause=%d)", emm_ctx->ueid, emm_ctx->emm_cause);
    /*
     * Notify EMM-AS SAP that Tracking Area Update Reject message has to be sent
     * onto the network
     */
    emm_sap.primitive = EMMAS_ESTABLISH_REJ;
    emm_sap.u.emm_as.u.establish.ueid = emm_ctx->ueid;
    emm_sap.u.emm_as.u.establish.UEid.guti = NULL;

    if (emm_ctx->emm_cause == EMM_CAUSE_SUCCESS) {
      emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    }

    emm_sap.u.emm_as.u.establish.emm_cause = emm_ctx->emm_cause;
    emm_sap.u.emm_as.u.establish.NASinfo = EMM_AS_NAS_INFO_TAU;
    emm_sap.u.emm_as.u.establish.NASmsg.length = 0;
    emm_sap.u.emm_as.u.establish.NASmsg.value = NULL;
    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data (&emm_sap.u.emm_as.u.establish.sctx, emm_ctx->security, false, true);
    rc = emm_sap_send (&emm_sap);
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/** \fn void _emm_tracking_area_update_accept (emm_data_context_t * emm_ctx,tau_accept_data_t * data);
    \brief Sends ATTACH ACCEPT message and start timer T3450.
     @param [in]emm_ctx UE EMM context data
     @param [in]data    UE TAU accept data
     @returns status of operation (RETURNok, RETURNerror)
*/
static int
_emm_tracking_area_update_accept (
  emm_data_context_t * emm_ctx,
  tau_accept_data_t * data)
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
    emm_sap.u.emm_as.u.establish.ueid = emm_ctx->ueid;

    emm_sap.u.emm_as.u.establish.eps_update_result = EPS_UPDATE_RESULT_TA_UPDATED;
    if (mme_config.nas_config.t3412_min > 0) {
      emm_sap.u.emm_as.u.establish.t3412 = &mme_config.nas_config.t3412_min;
    }
    // TODO Reminder
    emm_sap.u.emm_as.u.establish.guti = NULL;

    emm_sap.u.emm_as.u.establish.tai_list.list_type = emm_ctx->tai_list.list_type;
    emm_sap.u.emm_as.u.establish.tai_list.n_tais    = emm_ctx->tai_list.n_tais;
    for (i=0; i < emm_ctx->tai_list.n_tais; i++) {
      memcpy(&emm_sap.u.emm_as.u.establish.tai_list.tai[i], &emm_ctx->tai_list.tai[i], sizeof(tai_t));
    }
    emm_sap.u.emm_as.u.establish.NASinfo = EMM_AS_NAS_INFO_TAU;

    // TODO Reminder
    emm_sap.u.emm_as.u.establish.eps_bearer_context_status = NULL;
    emm_sap.u.emm_as.u.establish.location_area_identification = NULL;
    emm_sap.u.emm_as.u.establish.combined_tau_emm_cause = NULL;


    if (mme_config.nas_config.t3402_min > 0) {
      emm_sap.u.emm_as.u.establish.t3402 = &mme_config.nas_config.t3402_min;
    }
    emm_sap.u.emm_as.u.establish.t3423 = NULL;
    // TODO Reminder
    emm_sap.u.emm_as.u.establish.equivalent_plmns = NULL;
    emm_sap.u.emm_as.u.establish.emergency_number_list = NULL;

    emm_sap.u.emm_as.u.establish.eps_network_feature_support = &_emm_data.conf.eps_network_feature_support;
    emm_sap.u.emm_as.u.establish.additional_update_result = NULL;
    emm_sap.u.emm_as.u.establish.t3412_extended = NULL;


    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data (&emm_sap.u.emm_as.u.establish.sctx, emm_ctx->security, false, true);
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - encryption = 0x%X ", emm_sap.u.emm_as.u.establish.encryption);
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - integrity  = 0x%X ", emm_sap.u.emm_as.u.establish.integrity);
    emm_sap.u.emm_as.u.establish.encryption = emm_ctx->security->selected_algorithms.encryption;
    emm_sap.u.emm_as.u.establish.integrity = emm_ctx->security->selected_algorithms.integrity;
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - encryption = 0x%X (0x%X)", emm_sap.u.emm_as.u.establish.encryption, emm_ctx->security->selected_algorithms.encryption);
    LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - integrity  = 0x%X (0x%X)", emm_sap.u.emm_as.u.establish.integrity, emm_ctx->security->selected_algorithms.integrity);
    /*
     * Get the activate default EPS bearer context request message to
     * transfer within the ESM container of the attach accept message
     */
    //emm_sap.u.emm_as.u.establish.NASmsg = ...;
    //LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - NASmsg  src size = %d NASmsg  dst size = %d ", data->esm_msg.length, emm_sap.u.emm_as.u.establish.NASmsg.length);
    rc = emm_sap_send (&emm_sap);

    if (rc != RETURNerror) {
      if (emm_ctx->T3450.id != NAS_TIMER_INACTIVE_ID) {
        /*
         * Re-start T3450 timer
         */
        emm_ctx->T3450.id = nas_timer_restart (emm_ctx->T3450.id);
        MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 restarted UE " NAS_UE_ID_FMT " (TAU)", data->ueid);
      } else {
        /*
         * Start T3450 timer
         */
        emm_ctx->T3450.id = nas_timer_start (emm_ctx->T3450.sec, _emm_tau_t3450_handler, data);
        MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 started UE " NAS_UE_ID_FMT " (TAU)", data->ueid);
      }

      LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Timer T3450 (%d) expires in %ld seconds (TAU)", emm_ctx->T3450.id, emm_ctx->T3450.sec);
    }
  } else {
    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - emm_ctx NULL");
  }

  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

static int
_emm_tracking_area_update_abort (
  void *args)
{
  int                                     rc = RETURNerror;
  emm_data_context_t                     *ctx = NULL;
  tau_accept_data_t                      *data;

  LOG_FUNC_IN (LOG_NAS_EMM);
  data = (tau_accept_data_t *) (args);

  if (data) {
    unsigned int                            ueid = data->ueid;

    LOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - Abort the TAU procedure (ueid=" NAS_UE_ID_FMT ")", ueid);
    ctx = emm_data_context_get (&_emm_data, ueid);

    if (ctx) {
      /*
       * Stop timer T3450
       */
      if (ctx->T3450.id != NAS_TIMER_INACTIVE_ID) {
        LOG_INFO (LOG_NAS_EMM, "EMM-PROC  - Stop timer T3450 (%d)", ctx->T3450.id);
        ctx->T3450.id = nas_timer_stop (ctx->T3450.id);
        MSC_LOG_EVENT (MSC_NAS_EMM_MME, "T3450 stopped UE " NAS_UE_ID_FMT " (TAU)", data->ueid);
      }
    }

    /*
     * Release retransmission timer parameters
     */
    // no contained struct to free
    FREE_CHECK (data);

    /*
     * Notify EMM that EPS attach procedure failed
     */
    emm_sap_t                               emm_sap = {0};

    emm_sap.primitive = EMMREG_ATTACH_REJ;
    emm_sap.u.emm_reg.ueid = ueid;
    emm_sap.u.emm_reg.ctx = ctx;
    rc = emm_sap_send (&emm_sap);
  }
  LOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

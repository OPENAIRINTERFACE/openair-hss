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

  Source      EmmDeregistered.c

  Version     0.1

  Date        2012/10/03

  Product     NAS stack

  Subsystem   EPS Mobility Management

  Author      Frederic Maurel, Lionel GAUTHIER

  Description Implements the EPS Mobility Management procedures executed
        when the EMM-SAP is in EMM-DEREGISTERED state.

        In EMM-DEREGISTERED state, no EMM context has been established
        or the EMM context is marked as detached.
        The UE shall start the attach or combined attach procedure to
        establish an EMM context.

        The MME may answer to an attach or a combined attach procedure
        initiated by the UE. It may also answer to a tracking area
        updating procedure or combined tracking area updating procedure
        initiated by a UE if the EMM context is marked as detached.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <assert.h>

#include "bstrlib.h"

#include "log.h"
#include "common_defs.h"
#include "common_defs.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "networkDef.h"
#include "emm_proc.h"
#include "emm_fsm.h"
#include "emm_regDef.h"


/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    EmmDeregistered()                                         **
 **                                                                        **
 ** Description: Handles the behaviour of the UE and the MME while the     **
 **      EMM-SAP is in EMM-DEREGISTERED state.                     **
 **                                                                        **
 **              3GPP TS 24.301, section 5.2.2.2                           **
 **                                                                        **
 ** Inputs:  evt:       The received EMM-SAP event                 **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ***************************************************************************/
int EmmDeregistered (emm_reg_t * const evt)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  emm_data_context_t                          *emm_ctx = evt->ctx;

  if (emm_fsm_get_state (emm_ctx) != EMM_DEREGISTERED){
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state of UE " MME_UE_S1AP_ID_FMT " is not EMM_DEREGISTERED - instead (%d). \n", emm_ctx->ue_id, emm_fsm_get_state(emm_ctx));
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }

  switch (evt->primitive) {

  case _EMMREG_COMMON_PROC_REQ:
    /*
     * An EMM common procedure has been initiated;
     * enter state EMM-COMMON-PROCEDURE-INITIATED.
     */
    rc = emm_fsm_set_state (evt->ue_id, evt->ctx, EMM_COMMON_PROCEDURE_INITIATED);
    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "EMMREG_COMMON_PROC_REQ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_COMMON_PROC_CNF:
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Primitive _EMMREG_COMMON_PROC_CNF is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_COMMON_PROC_CNF ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_COMMON_PROC_REJ:
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Primitive _EMMREG_COMMON_PROC_REJ is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_COMMON_PROC_REJ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_COMMON_PROC_ABORT:
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Primitive _EMMREG_COMMON_PROC_ABORT is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_COMMON_PROC_ABORT ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_ATTACH_CNF:
    OAILOG_WARNING (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Primitive _EMMREG_ATTACH_CNF is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_ATTACH_CNF ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);

    break;

  case _EMMREG_ATTACH_REJ:
    /*
     * Attach procedure failed;
     * enter state EMM-DEREGISTERED.
     */
    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_ATTACH_REJ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_DEREGISTERED);

    if ((emm_ctx) && (evt->u.attach.proc) && (evt->u.attach.proc->emm_spec_proc.emm_proc.base_proc.fail_out)) {
      rc = (*evt->u.attach.proc->emm_spec_proc.emm_proc.base_proc.fail_out)(emm_ctx, &evt->u.attach.proc->emm_spec_proc.emm_proc.base_proc);
    }

    if ((emm_ctx) && (evt->notify) && (evt->u.attach.proc) && (evt->u.attach.proc->emm_spec_proc.emm_proc.base_proc.failure_notif)) {
      rc = (*evt->u.attach.proc->emm_spec_proc.emm_proc.base_proc.failure_notif)(emm_ctx);
    }
    if (evt->free_proc) {
      nas_delete_attach_procedure(emm_ctx);
    }
    break;

  case _EMMREG_ATTACH_ABORT:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Primitive _EMMREG_ATTACH_ABORT is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_ATTACH_ABORT ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_DETACH_INIT:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Primitive _EMMREG_DETACH_INIT is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_DETACH_INIT ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_DETACH_REQ:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Primitive _EMMREG_DETACH_REQ is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_DETACH_REQ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_DETACH_FAILED:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Primitive _EMMREG_DETACH_FAILED is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_DETACH_FAILED ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_DETACH_CNF:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Primitive _EMMREG_DETACH_CNF is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_DETACH_CNF ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_TAU_REQ:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Primitive _EMMREG_TAU_REQ is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_TAU_REQ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_TAU_CNF:
    OAILOG_INFO (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Setting UE to registered state via _EMMREG_TAU_CNF. \n");
    rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_REGISTERED);
	emm_ctx_set_valid_imsi(emm_ctx, &emm_ctx->_imsi, emm_ctx->_imsi64);
    mme_api_notify_new_guti(emm_ctx->ue_id, &emm_ctx->_guti);
	break;

  case _EMMREG_TAU_REJ:
    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_TAU_REJ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);

    if ((emm_ctx) && (evt->u.tau.proc) && (evt->u.tau.proc->emm_spec_proc.emm_proc.base_proc.fail_out)) {
      rc = (*evt->u.tau.proc->emm_spec_proc.emm_proc.base_proc.fail_out)(emm_ctx, &evt->u.tau.proc->emm_spec_proc.emm_proc.base_proc);
    }

    if ((emm_ctx) && (evt->notify) && (evt->u.tau.proc) && (evt->u.tau.proc->emm_spec_proc.emm_proc.base_proc.failure_notif)) {
      rc = (*evt->u.tau.proc->emm_spec_proc.emm_proc.base_proc.failure_notif)(emm_ctx);
    }
    if (evt->free_proc) {
      nas_delete_tau_procedure(emm_ctx);
    }
    break;

  case _EMMREG_TAU_ABORT:
     if (evt->u.tau.proc) {
       MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_TAU_ABORT ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
       rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_DEREGISTERED);

       /*
        * Stop timer T3450 (if exists).
        */
       void * unused = NULL;
       nas_stop_T3450(evt->u.tau.proc->ue_id, &evt->u.tau.proc->T3450, unused);

       if ((emm_ctx) && (evt->u.tau.proc->emm_spec_proc.emm_proc.base_proc.abort)) { /**< Currently, will perform IMPLICIT detach. */
         (*evt->u.tau.proc->emm_spec_proc.emm_proc.base_proc.abort)((nas_emm_base_proc_t*) emm_ctx, evt->u.tau.proc); // &evt->u.tau.proc->emm_spec_proc.emm_proc.base_proc);
       }
 //
 //      if ((rc != RETURNerror) && (emm_ctx) && (evt->notify) && (evt->u.tau.proc->emm_spec_proc.emm_proc.base_proc.failure_notif)) {
 //        (*evt->u.tau.proc->emm_spec_proc.emm_proc.base_proc.failure_notif)(emm_ctx);
 //      }
 //
 //      // todo: T3450 actually should be stopped here, since T3450 is activated for TAU only if GUTI is not sent & COMMON procedure will not be entered.
 //      if (evt->free_proc) {
 //        nas_delete_tau_procedure(emm_ctx);
 //      }
     } else {
       MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_TAU_ABORT ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
     }
     break;

  case _EMMREG_SERVICE_REQ:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Primitive _EMMREG_SERVICE_REQ is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_SERVICE_REQ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_SERVICE_CNF:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Primitive _EMMREG_SERVICE_CNF is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_SERVICE_CNF ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_SERVICE_REJ:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Primitive _EMMREG_SERVICE_REJ is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_SERVICE_REJ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_LOWERLAYER_SUCCESS:
    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_LOWERLAYER_SUCCESS ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    /*
     * Data successfully delivered to the network
     */
    if (emm_ctx) {
      nas_emm_proc_t * emm_proc = nas_emm_find_procedure_by_msg_digest(emm_ctx, (const char *)evt->u.ll_success.msg_digest,
          evt->u.ll_success.digest_len, evt->u.ll_success.msg_len);
      if (emm_proc) {
        if ((evt->notify) && (emm_proc->not_delivered)) {
          rc = (*emm_proc->delivered)(emm_ctx, emm_proc);
        }
      }
    }
    rc = RETURNok;
    break;

  case _EMMREG_LOWERLAYER_FAILURE:
    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_LOWERLAYER_FAILURE ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);

    if (emm_ctx) {
      nas_emm_proc_t * emm_proc = nas_emm_find_procedure_by_msg_digest(emm_ctx, (const char *)evt->u.ll_failure.msg_digest,
          evt->u.ll_failure.digest_len, evt->u.ll_failure.msg_len);
      if (emm_proc) {
        if ((evt->notify) && (emm_proc->not_delivered)) {
          rc = (*emm_proc->not_delivered)(emm_ctx, emm_proc);
        }
      }
    }
    break;

  case _EMMREG_LOWERLAYER_RELEASE:
    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_LOWERLAYER_RELEASE ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    nas_delete_all_emm_procedures(emm_ctx);
    /** The ESM procedures will handle the EMM session termination independently, checking the EMM state when transmitting a message. */
    rc = RETURNok;
    break;

  case  _EMMREG_LOWERLAYER_NON_DELIVERY:
    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_LOWERLAYER_NON_DELIVERY ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    if (emm_ctx) {
      nas_emm_proc_t * emm_proc = nas_emm_find_procedure_by_msg_digest(emm_ctx, (const char *)evt->u.non_delivery_ho.msg_digest,
          evt->u.non_delivery_ho.digest_len, evt->u.non_delivery_ho.msg_len);
      if (emm_proc) {
        if ((evt->notify) && (emm_proc->not_delivered)) {
          rc = (*emm_proc->not_delivered_ho)(emm_ctx, emm_proc);
        } else {
          rc = RETURNok;
        }
      }
    }
    break;
  default:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_DEREGISTERED - Primitive is not valid (%d)\n", evt->primitive);
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_UNKNOWN(primitive id %d) ue id " MME_UE_S1AP_ID_FMT " ", evt->primitive, evt->ue_id);
    break;
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

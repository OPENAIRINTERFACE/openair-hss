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

  Source      EmmCommonProcedureInitiated.c

  Version     0.1

  Date        2012/10/03

  Product     NAS stack

  Subsystem   EPS Mobility Management

  Author      Frederic Maurel, Lionel GAUTHIER

  Description Implements the EPS Mobility Management procedures executed
        when the EMM-SAP is in EMM-COMMON-PROCEDURE-INITIATED state.

        In EMM-COMMON-PROCEDURE-INITIATED state, the MME has started
        a common EMM procedure and is waiting for a response from the
        UE.

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
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"

#include "mme_app_procedures.h"
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
 ** Name:    EmmCommonProcedureInitiated()                             **
 **                                                                        **
 ** Description: Handles the behaviour of the MME while the EMM-SAP is in  **
 **      EMM_COMMON_PROCEDURE_INITIATED state.                     **
 **                                                                        **
 **              3GPP TS 24.301, section 5.1.3.4.2                         **
 **                                                                        **
 ** Inputs:  evt:       The received EMM-SAP event                 **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ***************************************************************************/
int EmmCommonProcedureInitiated (emm_reg_t * const evt)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  emm_data_context_t                     *emm_ctx = evt->ctx;

  assert (emm_fsm_get_state (emm_ctx) == EMM_COMMON_PROCEDURE_INITIATED);

  switch (evt->primitive) {
  case _EMMREG_COMMON_PROC_REQ:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_COMMON_PROCEDURE_INITIATED - Primitive _EMMREG_COMMON_PROC_REQ is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_COMMON_PROC_REQ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_COMMON_PROC_CNF:

    /*
     * An EMM common procedure successfully completed;
     * Get the type of the common procedure.
     */
    if (evt->u.common.common_proc) {
      MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_COMMON_PROC_CNF ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
      if (evt->u.common.common_proc->emm_proc.base_proc.parent) {
        rc = nas_unlink_emm_procedures(evt->u.common.common_proc->emm_proc.base_proc.parent, (nas_emm_base_proc_t*)&evt->u.common.common_proc->emm_proc.base_proc);
      }

      // todo: need to check if another common procedure is existing?
      // todo: state might have been changed meanwhile (Identity Response (IMEI)) comes late and out of context (after Attach) --> UE goes to DEREGISTERED.
      // todo: previous state --> checking if the UE has_already_been_attached or not and if any other common procedure is running should be enough!
      rc = emm_fsm_set_state (evt->ue_id, emm_ctx, ((nas_emm_proc_t*)evt->u.common.common_proc)->previous_emm_fsm_state);

      if ((rc != RETURNerror) && (emm_ctx) && (evt->notify)) {
        (*evt->u.common.common_proc->emm_proc.base_proc.success_notif)(emm_ctx); /**< Callback method called and job of the session structure is finished. */
      }
      if(emm_data_context_get(&_emm_data, evt->ue_id)){
        if (evt->free_proc) {
          nas_delete_common_procedure(emm_ctx, &evt->u.common.common_proc);
        }
      }else{
        /** No EMM context exist. Not freeing any procedures. */
      }
    } else {
      MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_COMMON_PROC_CNF ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    }
    break;

  case _EMMREG_COMMON_PROC_REJ:
    if (evt->u.common.common_proc) {
      MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_COMMON_PROC_REJ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
      /*
       * An EMM common procedure failed;
       * enter state EMM-DEREGISTERED.
       */
      rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_DEREGISTERED);

      if ((emm_ctx) && (evt->u.common.common_proc->emm_proc.base_proc.fail_out)) {
        rc = (*evt->u.common.common_proc->emm_proc.base_proc.fail_out)(emm_ctx, &evt->u.common.common_proc->emm_proc.base_proc);
      }

      if((emm_ctx = emm_data_context_get(&_emm_data, evt->ue_id))){
    	  if ((rc != RETURNerror) && (evt->u.common.common_proc->emm_proc.base_proc.failure_notif)) {
    		  rc = (*evt->u.common.common_proc->emm_proc.base_proc.failure_notif)(emm_ctx);
    	  }
      }
      /** Check if the emm context still exists. */
      if((emm_ctx = emm_data_context_get(&_emm_data, evt->ue_id))){
        if (evt->free_proc) {
          nas_delete_common_procedure(emm_ctx, &evt->u.common.common_proc);
        }
      }else{
        /** No EMM context exist. Not freeing any procedures. */
      }
    } else {
      MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_COMMON_PROC_REJ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    }
    break;

  case _EMMREG_COMMON_PROC_ABORT:
    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_COMMON_PROC_ABORT ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    if (evt->u.common.common_proc) {
      if (evt->u.common.common_proc->emm_proc.base_proc.parent) {
        rc = nas_unlink_emm_procedures(evt->u.common.common_proc->emm_proc.base_proc.parent, (nas_emm_base_proc_t*)&evt->u.common.common_proc->emm_proc.base_proc);
      }

      if ((emm_ctx) && (evt->u.common.common_proc->emm_proc.base_proc.abort)) { /**< Just stops the timer. */
        (*evt->u.common.common_proc->emm_proc.base_proc.abort)(emm_ctx, &evt->u.common.common_proc->emm_proc.base_proc);
      }

      /** Check if the emm context still exists. */
      if((emm_ctx = emm_data_context_get(&_emm_data, evt->ue_id))){
          rc = emm_fsm_set_state (evt->ue_id, emm_ctx, ((nas_emm_proc_t*)evt->u.common.common_proc)->previous_emm_fsm_state);
          /*
           *
           * We will always check the notification flag, depending on the severity of the error.
           * May perform an implicit detach (and removing all procedures).
           */
          if ((rc != RETURNerror) && (emm_ctx) && (evt->notify) && (evt->u.common.common_proc->emm_proc.base_proc.failure_notif)) {
            (*evt->u.common.common_proc->emm_proc.base_proc.failure_notif)(emm_ctx);
          }
          if((emm_ctx = emm_data_context_get(&_emm_data, evt->ue_id))){
        	  if (evt->free_proc) {
        		  nas_delete_common_procedure(emm_ctx, &evt->u.common.common_proc);
        	  }
          } else {
        	  OAILOG_INFO(LOG_NAS_EMM, "No EMM context for UE " MME_UE_S1AP_ID_FMT " exists after fail handler for common procedure. "
        			  "Not continuing further. \n", evt->ue_id);
          }
      }else{
        /** No EMM context exist. Not freeing any procedures. */
    	OAILOG_INFO(LOG_NAS_EMM, "No EMM context for UE " MME_UE_S1AP_ID_FMT " exists after aborting common procedure. "
    			"Not continuing further. \n", evt->ue_id);
      }
    } else {
      MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_COMMON_PROC_ABORT ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    }

    break;

  case _EMMREG_ATTACH_CNF:
    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_ATTACH_CNF ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);

    /*
     * Attach procedure successful and default EPS bearer
     * context activated;
     * enter state EMM-REGISTERED.
     */
    rc = emm_fsm_set_state (evt->ue_id, evt->ctx, EMM_REGISTERED);
    assert(rc == RETURNok);

    /*
     * Call this method after setting the state.
     * Initial Context Setup Response might have been received meanwhile.
     * todo: add locks
     */
    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_ATTACH_CNF ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    if ((emm_ctx) /*&& (evt->notify) */&& (evt->u.attach.proc) && (evt->u.attach.proc->emm_spec_proc.emm_proc.base_proc.success_notif)) {
      rc = (*evt->u.attach.proc->emm_spec_proc.emm_proc.base_proc.success_notif)(emm_ctx);
    }
    //    if (evt->free_proc) {
    nas_delete_attach_procedure(emm_ctx);
     //    }
    break;

  case _EMMREG_ATTACH_REJ:
    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_ATTACH_REJ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    /*
     * Attach procedure failed;
     * enter state EMM-DEREGISTERED.
     */
    rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_DEREGISTERED);
    nas_delete_attach_procedure(emm_ctx);
    break;

  case _EMMREG_ATTACH_ABORT:
    if (evt->u.attach.proc) {
      MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_ATTACH_ABORT ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
      rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_DEREGISTERED);

      if ((emm_ctx) && (evt->u.attach.proc->emm_spec_proc.emm_proc.base_proc.abort)) { /**< Currently, will perform IMPLICIT detach. For any case we will remove the procedures here. */
        (*evt->u.attach.proc->emm_spec_proc.emm_proc.base_proc.abort)((nas_emm_base_proc_t*) emm_ctx, evt->u.attach.proc); // &evt->u.attach.proc->emm_spec_proc.emm_proc.base_proc);
      }

//        nas_delete_attach_procedure(emm_ctx);
    } else {
      MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_ATTACH_ABORT ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    }

    break;

  case _EMMREG_DETACH_INIT:
    // todo: could happen in CommonProcedure! (common procedure started) // todo: need to stop all commons before detach ?
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_COMMON_PROCEDURE_INITIATED - Primitive _EMMREG_DETACH_INIT is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_DETACH_INIT ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_DETACH_REQ:
    // todo: could happen in CommonProcedure! (common procedure started) // todo: need to stop all commons before detach ?
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_COMMON_PROCEDURE_INITIATED - Primitive _EMMREG_DETACH_REQ is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_DETACH_REQ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_DETACH_FAILED:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_COMMON_PROCEDURE_INITIATED - Primitive _EMMREG_DETACH_FAILED is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_DETACH_FAILED ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_DETACH_CNF:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_COMMON_PROCEDURE_INITIATED - Primitive _EMMREG_DETACH_CNF is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_DETACH_CNF ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_TAU_REQ:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_COMMON_PROCEDURE_INITIATED - Primitive _EMMREG_TAU_REQ is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_TAU_REQ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_TAU_CNF:
    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_TAU_CNF ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);

    /*
     * Call this method after setting the state.
     * Initial Context Setup Response might have been received meanwhile.
     * todo: add locks
     */
    rc = emm_fsm_set_state (evt->ue_id, evt->ctx, EMM_REGISTERED);
    assert(rc == RETURNok);

    /*
     * TAU procedure successful
     * enter state EMM-REGISTERED.
     */
    if ((emm_ctx) && (evt->notify) && (evt->u.tau.proc) && (evt->u.tau.proc->emm_spec_proc.emm_proc.base_proc.success_notif)) {
      rc = (*evt->u.tau.proc->emm_spec_proc.emm_proc.base_proc.success_notif)(emm_ctx);
    }

    if (evt->free_proc) {
      nas_delete_tau_procedure(emm_ctx);
    }

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
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_COMMON_PROCEDURE_INITIATED - Primitive _EMMREG_SERVICE_REQ is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_SERVICE_REQ ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_SERVICE_CNF:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_COMMON_PROCEDURE_INITIATED - Primitive _EMMREG_SERVICE_CNF is not valid\n");
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_SERVICE_CNF ue id " MME_UE_S1AP_ID_FMT " ", evt->ue_id);
    break;

  case _EMMREG_SERVICE_REJ:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_COMMON_PROCEDURE_INITIATED - Primitive _EMMREG_SERVICE_REJ is not valid\n");
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
        if ((evt->notify) && (emm_proc->delivered)) {
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
      rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_DEREGISTERED);
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
        }
      }
      rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_DEREGISTERED);
    }
    break;

  default:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM state EMM_COMMON_PROCEDURE_INITIATED - Primitive is not valid (%d)\n", evt->primitive);
    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "_EMMREG_UNKNOWN(primitive id %d) ue id " MME_UE_S1AP_ID_FMT " ", evt->primitive, evt->ue_id);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

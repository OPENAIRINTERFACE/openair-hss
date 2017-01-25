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

  Author      Frederic Maurel

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
#include "emm_fsm.h"
#include "commonDef.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "mme_app_ue_context.h"
#include "emm_proc.h"
#include "mme_app_defs.h"


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
int
EmmCommonProcedureInitiated (
  const emm_reg_t * evt)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  emm_context_t                          *emm_ctx = evt->ctx;

  assert (emm_fsm_get_state (emm_ctx) == EMM_COMMON_PROCEDURE_INITIATED);

  switch (evt->primitive) {
  case _EMMREG_PROC_ABORT:
    /*
     * The EMM procedure that initiated EMM common procedure aborted
     */
    if (emm_ctx) {
      //tricky:
      if (emm_ctx->common_proc) { // assume cleaned if terminated
        rc = emm_proc_common_abort (&emm_ctx->common_proc);
        emm_fsm_set_state(evt->ue_id, emm_ctx, EMM_DEREGISTERED);
      } else if (emm_ctx->specific_proc) {
        rc = emm_proc_specific_abort (&emm_ctx->specific_proc);
      }
    }
    break;

  case _EMMREG_COMMON_PROC_CNF:

    /*
     * An EMM common procedure successfully completed;
     */
    if (evt->u.common.is_attached) {
      rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_REGISTERED);
    } else {
      rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_DEREGISTERED);
    }
    OAILOG_TRACE (LOG_NAS_EMM, "rc=%d, emm_ctx=%p, common_proc=%p\n", rc, emm_ctx, emm_ctx->common_proc);

    if ((rc != RETURNerror) && (emm_ctx) && (emm_ctx->common_proc)) {
      rc = emm_proc_common_success (&emm_ctx->common_proc);
    }

    break;

  case _EMMREG_COMMON_PROC_REJ:
    /*
     * An EMM common procedure failed;
     * enter state EMM-DEREGISTERED.
     */
    rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_DEREGISTERED);

    if ((rc != RETURNerror) && (emm_ctx) && (emm_ctx->common_proc)) {
      rc = emm_proc_common_reject (&emm_ctx->common_proc);
    }

    break;

  case _EMMREG_ATTACH_CNF:
    /*
     * Attach procedure successful and default EPS bearer
     * context activated;
     * enter state EMM-REGISTERED.
     */
    rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_REGISTERED);
    break;

  case _EMMREG_ATTACH_REJ:
    /*
     * Attach procedure failed;
     * enter state EMM-DEREGISTERED.
     */
    rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_DEREGISTERED);
    break;

  case _EMMREG_LOWERLAYER_SUCCESS:
    /*
     * Data successfully delivered to the network
     */
    rc = RETURNok;
    break;

  case _EMMREG_LOWERLAYER_RELEASE:
  case _EMMREG_LOWERLAYER_FAILURE:
    /*
     * Transmission failure occurred before the EMM common
     * procedure being completed
     */

    if ((emm_ctx) && (emm_ctx->common_proc)) {
      rc = emm_proc_common_ll_failure (&emm_ctx->common_proc);
    }

    rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_DEREGISTERED);

    break;

  case _EMMREG_LOWERLAYER_NON_DELIVERY:
    if ((emm_ctx) && (emm_ctx->common_proc)) {
      rc = emm_proc_common_non_delivered_ho (&emm_ctx->common_proc);
    }
    if (rc != RETURNerror) {
      rc = emm_fsm_set_state (evt->ue_id, emm_ctx, EMM_DEREGISTERED);
    }
    break;

  default:
    OAILOG_ERROR (LOG_NAS_EMM, "EMM-FSM   - Primitive is not valid (%d)\n", evt->primitive);
    break;
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

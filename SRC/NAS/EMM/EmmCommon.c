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
  Source      EmmCommon.h

  Version     0.1

  Date        2013/04/19

  Product     NAS stack

  Subsystem   EPS Mobility Management

  Author      Frederic Maurel

  Description Defines callback functions executed within EMM common procedures
        by the Non-Access Stratum running at the network side.

        Following EMM common procedures can always be initiated by the
        network whilst a NAS signalling connection exists:

        GUTI reallocation
        authentication
        security mode control
        identification
        EMM information

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "log.h"
#include "dynamic_memory_check.h"
#include "assertions.h"
#include "common_defs.h"
#include "commonDef.h"
#include "emm_proc.h"
#include "emm_data.h"

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_common_initialize()                              **
 **                                                                        **
 ** Description: Initialize EMM procedure callback functions executed for  **
 **      the UE with the given identifier                          **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      success:   EMM procedure executed upon successful EMM **
 **             common procedure completion                **
 **      reject:    EMM procedure executed if the EMM common   **
 **             procedure failed or is rejected            **
 **      failure:   EMM procedure executed upon transmission   **
 **             failure reported by lower layer            **
 **      abort:     EMM common procedure executed when the on- **
 **             going EMM procedure is aborted             **
 **      args:      EMM common procedure argument parameters   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_common_data                           **
 **                                                                        **
 ***************************************************************************/
int emm_proc_common_initialize (
  struct emm_context_s               * const emm_context,
  emm_common_proc_type_t        const type,
  emm_common_data_t           * const emm_common_data,
  emm_common_success_callback_t       _success,
  emm_common_reject_callback_t        _reject,
  emm_common_failure_callback_t       _failure,
  emm_common_ll_failure_callback_t    _ll_failure,
  emm_common_non_delivered_ho_callback_t _non_delivered_ho,
  emm_common_abort_callback_t         _abort)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  if (emm_common_data) {
    emm_common_data->container     = emm_context;
    emm_common_data->type          = type;
    emm_common_data->success       = _success;
    emm_common_data->reject        = _reject;
    emm_common_data->failure       = _failure;
    emm_common_data->ll_failure    = _ll_failure;
    emm_common_data->non_delivered_ho = _non_delivered_ho;
    emm_common_data->abort         = _abort;
    emm_common_data->cleanup       = false;
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_common_success()                                 **
 **                                                                        **
 ** Description: The EMM common procedure initiated between the UE with    **
 **      the specified identifier and the MME completed success-   **
 **      fully. The network performs required actions related to   **
 **      the ongoing EMM procedure.                                **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      Others:    _emm_common_data, _emm_data                **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_common_success (emm_common_data_t **emm_common_data)
{
  emm_common_success_callback_t           emm_callback = {0};
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  if (*emm_common_data ) {
    emm_callback = (*emm_common_data)->success;

    (*emm_common_data)->cleanup = true;
    if (emm_callback) {
      struct emm_context_s  *ctx = (*emm_common_data)->container;
      rc = (*emm_callback) (ctx);
    }
    // kind of garbage collector
    emm_common_cleanup (emm_common_data);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_common_reject()                                  **
 **                                                                        **
 ** Description: The EMM common procedure initiated between the UE with    **
 **      the specified identifier and the MME failed or has been   **
 **      rejected. The network performs required actions related   **
 **      to the ongoing EMM procedure.                             **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      Others:    _emm_common_data, _emm_data                **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_common_reject (emm_common_data_t **emm_common_data)
{
  int                                     rc = RETURNerror;
  emm_common_reject_callback_t            emm_callback;

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  if (*emm_common_data ) {
    emm_callback = (*emm_common_data)->reject;

    (*emm_common_data)->cleanup = true;
    if (emm_callback) {
      struct emm_context_s  *ctx = (*emm_common_data)->container;
      rc = (*emm_callback) (ctx);
    } else {
      rc = RETURNok;
    }

    emm_common_cleanup (emm_common_data);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
int emm_proc_common_failure (emm_common_data_t **emm_common_data)
{
  int                                     rc = RETURNerror;
  emm_common_reject_callback_t            emm_callback;

  OAILOG_FUNC_IN (LOG_NAS_EMM);
  if (*emm_common_data ) {
    emm_callback = (*emm_common_data)->failure;

    (*emm_common_data)->cleanup = true;
    if (emm_callback) {
      struct emm_context_s  *ctx = (*emm_common_data)->container;
      rc = (*emm_callback) (ctx);
    } else {
      rc = RETURNok;
    }

    emm_common_cleanup (emm_common_data);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_common_ll_failure()                                 **
 **                                                                        **
 ** Description: The EMM common procedure has been initiated between the   **
 **      UE with the specified identifier and the MME, and a lower **
 **      layer failure occurred before the EMM common procedure    **
 **      being completed. The network performs required actions    **
 **      related to the ongoing EMM procedure.                     **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      Others:    _emm_common_data, _emm_data                **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_common_ll_failure (emm_common_data_t **emm_common_data)
{
  emm_common_ll_failure_callback_t           emm_callback;
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_EMM);

  if (*emm_common_data) {
    emm_callback = (*emm_common_data)->ll_failure;

    (*emm_common_data)->cleanup = true;
    if (emm_callback) {
      struct emm_context_s  *ctx = (*emm_common_data)->container;
      rc = (*emm_callback) (ctx);
    } else {
      rc = RETURNok;
    }

    emm_common_cleanup (emm_common_data);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_common_non_delivered()                                 **
 **                                                                        **
 ** Description: The EMM common procedure has been initiated between the   **
 **      UE with the specified identifier and the MME, and a report **
 **      of lower layers stated that the EMM common procedure message   **
 **      could not be delivered. The network performs required actions    **
 **      related to the ongoing EMM procedure.                     **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      Others:    _emm_common_data, _emm_data                **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_common_non_delivered_ho (emm_common_data_t **emm_common_data)
{
  emm_common_non_delivered_ho_callback_t     emm_callback;
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_EMM);

  if (*emm_common_data) {
    emm_callback = (*emm_common_data)->non_delivered_ho;

    (*emm_common_data)->cleanup = true;
    if (emm_callback) {
      struct emm_context_s  *ctx = (*emm_common_data)->container;
      rc = (*emm_callback) (ctx);
    } else {
      rc = RETURNok;
    }

    emm_common_cleanup (emm_common_data);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_common_abort()                                   **
 **                                                                        **
 ** Description: The ongoing EMM procedure has been aborted. The network   **
 **      performs required actions related to the EMM common pro-  **
 **      cedure previously initiated between the UE with the spe-  **
 **      cified identifier and the MME.                            **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      Others:    _emm_common_data                           **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_common_abort (emm_common_data_t **emm_common_data)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_common_abort_callback_t             emm_callback;
  int                                     rc = RETURNerror;


  if (*emm_common_data) {
    emm_callback = (*emm_common_data)->abort;

    (*emm_common_data)->cleanup = true;
    if (emm_callback) {
      struct emm_context_s  *ctx = (*emm_common_data)->container;
      rc = (*emm_callback) (ctx);
    } else {
      rc = RETURNok;
    }

    emm_common_cleanup (emm_common_data);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_common_cleanup()                                 **
 **                                                                        **
 ** Description: Cleans EMM procedure callback functions upon completion   **
 **      of an EMM common procedure previously initiated within an **
 **      EMM procedure currently in progress between the network   **
 **      and the UE with the specified identifier.                 **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _emm_common_data                           **
 **                                                                        **
 ***************************************************************************/
void emm_common_cleanup (emm_common_data_t **emm_common_data)
{

  if (*emm_common_data) {
    if ((*emm_common_data)->cleanup) {
      emm_common_proc_type_t  type = (*emm_common_data)->type;
      free_wrapper((void**)emm_common_data);
      // nothing to do for all emm_common_arg_t
      switch (type) {
      case EMM_COMMON_PROC_TYPE_AUTHENTICATION:
        OAILOG_TRACE (LOG_NAS_EMM, "common procedure AUTHENTICATION cleaned\n");
        break;
      case EMM_COMMON_PROC_TYPE_IDENTIFICATION:
        OAILOG_TRACE (LOG_NAS_EMM, "common procedure IDENTIFICATION cleaned\n");
        break;
      case EMM_COMMON_PROC_TYPE_SECURITY_MODE_CONTROL:
        OAILOG_TRACE (LOG_NAS_EMM, "common procedure SECURITY_MODE_CONTROL cleaned\n");
        break;
      case EMM_COMMON_PROC_TYPE_GUTI_REALLOCATION:
        OAILOG_TRACE (LOG_NAS_EMM, "common procedure GUTI_REALLOCATION cleaned\n");
        break;
      case EMM_COMMON_PROC_TYPE_INFORMATION:
        OAILOG_TRACE (LOG_NAS_EMM, "common procedure INFORMATION cleaned\n");
        break;
     default:
       ;
      }
    }
  }
}


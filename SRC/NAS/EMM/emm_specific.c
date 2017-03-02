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
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "obj_hashtable.h"
#include "log.h"
#include "msc.h"
#include "gcc_diag.h"
#include "dynamic_memory_check.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "security_types.h"
#include "common_types.h"
#include "common_defs.h"
#include "securityDef.h"
#include "nas_message.h"
#include "as_message.h"
#include "emm_proc.h"
#include "networkDef.h"
#include "emm_sap.h"
#include "mme_api.h"
#include "emm_data.h"
#include "emm_specific.h"

//#include "emm_data.h"

//------------------------------------------------------------------------------
int emm_proc_specific_initialize (
  emm_context_t             * const emm_context,
  emm_specific_proc_type_t           const type,
  emm_specific_procedure_data_t    * const emm_specific_data,
  emm_specific_ll_failure_callback_t       _ll_failure,
  emm_specific_non_delivered_ho_callback_t _non_delivered_ho,
  emm_specific_abort_callback_t            _abort)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  if (emm_specific_data) {
    emm_specific_data->container     = emm_context;
    emm_specific_data->type          = type;
    emm_specific_data->ll_failure    = _ll_failure;
    emm_specific_data->non_delivered_ho = _non_delivered_ho;
    emm_specific_data->abort         = _abort;
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
}


//------------------------------------------------------------------------------
int emm_proc_specific_ll_failure (emm_specific_procedure_data_t **emm_specific_data)
{
  emm_specific_ll_failure_callback_t           emm_callback;
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_EMM);

  if (*emm_specific_data) {
    emm_callback = (*emm_specific_data)->ll_failure;

    if (emm_callback) {
      struct emm_context_s  *ctx = (*emm_specific_data)->container;
      rc = (*emm_callback) (ctx);
    }

    emm_proc_specific_cleanup (emm_specific_data);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}
//------------------------------------------------------------------------------
int emm_proc_specific_non_delivered_ho (emm_specific_procedure_data_t **emm_specific_data)
{
  emm_common_non_delivered_ho_callback_t     emm_callback;
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_EMM);

  if (*emm_specific_data) {
    emm_callback = (*emm_specific_data)->non_delivered_ho;

    if (emm_callback) {
      struct emm_context_s  *ctx = (*emm_specific_data)->container;
      rc = (*emm_callback) (ctx);
    }

    emm_proc_specific_cleanup (emm_specific_data);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}
//------------------------------------------------------------------------------
int emm_proc_specific_abort (emm_specific_procedure_data_t **emm_specific_data)
{
  emm_common_abort_callback_t             emm_callback;
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS_EMM);

  if (*emm_specific_data) {
    emm_callback = (*emm_specific_data)->abort;

    if (emm_callback) {
      struct emm_context_s  *ctx = (*emm_specific_data)->container;
      rc = (*emm_callback) (ctx);
    }

    emm_proc_specific_cleanup (emm_specific_data);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}
//------------------------------------------------------------------------------
void emm_proc_specific_cleanup (emm_specific_procedure_data_t **emm_specific_data)
{

  if (*emm_specific_data) {
    switch ((*emm_specific_data)->type) {
    case EMM_SPECIFIC_PROC_TYPE_ATTACH:
      bdestroy_wrapper (&(*emm_specific_data)->arg.u.attach_data.esm_msg);
      OAILOG_INFO (LOG_NAS_EMM, "Cleaned specific procedure ATTACH\n");
      break;
    case EMM_SPECIFIC_PROC_TYPE_NONE:
      OAILOG_INFO (LOG_NAS_EMM, "Cleaned specific procedure NONE\n");
      break;
    case EMM_SPECIFIC_PROC_TYPE_DETACH:
      OAILOG_INFO (LOG_NAS_EMM, "Cleaned specific procedure DETACH\n");
      break;
    case EMM_SPECIFIC_PROC_TYPE_TAU:
      OAILOG_INFO (LOG_NAS_EMM, "Cleaned specific procedure TAU\n");
      break;
    default:
      OAILOG_WARNING (LOG_NAS_EMM, "Cleaned specific procedure unknown type %d\n", (*emm_specific_data)->type);
      ;
    }
    free_wrapper((void**)emm_specific_data);
  }
}

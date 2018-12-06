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
  Source      EsmDeleteBearerCommandHdl.c

  Version     0.1

  Date        2018/12/04

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Dincer Beken

  Description Defines the bearer deletion related congestion procedure for the ESM triggered by the
        Access Stratum.

        Method handled when bearer could not be established in the eNB as well as when the congestion is UE triggered by
        DELETE_BEARER_COMMAND message.

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
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "3gpp_36.401.h"
#include "mme_app_ue_context.h"
#include "common_defs.h"
#include "esm_data.h"
#include "esm_ebr.h"
#include "esm_proc.h"
#include "esm_pt.h"
#include "esm_cause.h"
#include "esm_sapDef.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

int
esm_proc_bearer_removal_command(esm_context_t * esm_context, esm_eps_congestion_indicator_t * eps_congestion_indicator){
  ue_context_t      *ue_context = NULL;
  int rc =          RETURNok;

  OAILOG_FUNC_IN (LOG_NAS_ESM);

  // todo: just process the bearer removal command and forward it to the CN
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

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


/*! \file m2ap_mce_decoder.c
   \brief m2ap decode procedures for MME
   \author Dincer BEKEN <dbeken@blackned.de>
   \date 2019
   \version 0.1
*/
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>

#include "m2ap_common.h"
#include "m2ap_mce_handlers.h"
#include "bstrlib.h"

#include "log.h"
#include "assertions.h"
#include "common_defs.h"
#include "intertask_interface.h"
#include "dynamic_memory_check.h"

//-----------------------------------------------------------------------------
int m2ap_mce_decode_pdu(M2AP_M2AP_PDU_t *pdu, const_bstring const raw)
{
  asn_dec_rval_t dec_ret;
  DevAssert(pdu != NULL);
  DevAssert(blength(raw) != 0);
  dec_ret = aper_decode(NULL,
                        &asn_DEF_M2AP_M2AP_PDU,
                        (void **)&pdu,
                        bdata(raw),
                        blength(raw),
                        0,
                        0);

  if (dec_ret.code != RC_OK) {
    OAILOG_ERROR (LOG_S1AP, "Failed to decode PDU\n");
    return -1;
  }
  return 0;
}


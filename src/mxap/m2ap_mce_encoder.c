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

/*! \file m2ap_mce_encoder.c
   \brief m2ap encode procedures for MCE
   \author Dincer BEKEN <dbeken@blackned.de>
   \date 2019
*/

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>

#include "bstrlib.h"

#include "intertask_interface.h"
#include "mme_app_bearer_context.h"
#include "mme_api.h"
#include "assertions.h"
#include "log.h"
#include "m2ap_common.h"
#include "m2ap_mce_encoder.h"

static inline int                       m2ap_mce_encode_initiating (
  M2AP_M2AP_PDU_t * pdu,
  uint8_t ** buffer,
  uint32_t * length);
static inline int                       m2ap_mce_encode_successfull_outcome (
  M2AP_M2AP_PDU_t * pdu,
  uint8_t ** buffer,
  uint32_t * len);
static inline int                       m2ap_mce_encode_unsuccessfull_outcome (
  M2AP_M2AP_PDU_t * pdu,
  uint8_t ** buffer,
  uint32_t * len);

//------------------------------------------------------------------------------
int
m2ap_mce_encode_pdu (M2AP_M2AP_PDU_t *pdu, uint8_t **buffer, uint32_t *length)
{
  int ret = -1;
  DevAssert (pdu != NULL);
  DevAssert (buffer != NULL);
  DevAssert (length != NULL);


  switch (pdu->present) {
  case M2AP_M2AP_PDU_PR_initiatingMessage:
    ret = m2ap_mce_encode_initiating (pdu, buffer, length);
    break;

  case M2AP_M2AP_PDU_PR_successfulOutcome:
    ret = m2ap_mce_encode_successfull_outcome (pdu, buffer, length);
    break;

  case M2AP_M2AP_PDU_PR_unsuccessfulOutcome:
    ret = m2ap_mce_encode_unsuccessfull_outcome (pdu, buffer, length);
    break;

  default:
    OAILOG_NOTICE (LOG_MXAP, "Unknown message outcome (%d) or not implemented", (int)pdu->present);
    break;
  }
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_M2AP_M2AP_PDU, pdu);
  return ret;
}



//------------------------------------------------------------------------------
static inline int
m2ap_mce_encode_initiating (
  M2AP_M2AP_PDU_t *pdu,
  uint8_t ** buffer,
  uint32_t * length)
{
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch(pdu->choice.initiatingMessage.procedureCode) {
  case M2AP_ProcedureCode_id_sessionStart:
  case M2AP_ProcedureCode_id_sessionStop:
  case M2AP_ProcedureCode_id_sessionUpdate:
  case M2AP_ProcedureCode_id_mbmsSchedulingInformation:
  case M2AP_ProcedureCode_id_mbmsServiceCounting:
  case M2AP_ProcedureCode_id_mCEConfigurationUpdate:
	  break;

  default:
    OAILOG_NOTICE (LOG_MXAP, "Unknown procedure ID (%d) for initiating message_p\n", (int)pdu->choice.initiatingMessage.procedureCode);
    *buffer = NULL;
    *length = 0;
    return -1;
  }

  memset(&res, 0, sizeof(res));
  res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_M2AP_M2AP_PDU, pdu);
  *buffer = res.buffer;
  *length = res.result.encoded;
  return 0;
}

//------------------------------------------------------------------------------
static inline int
m2ap_mce_encode_successfull_outcome (
  M2AP_M2AP_PDU_t * pdu,
  uint8_t ** buffer,
  uint32_t * length)
{
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch (pdu->choice.successfulOutcome.procedureCode) {
  case M2AP_ProcedureCode_id_m2Setup:
  case M2AP_ProcedureCode_id_reset:
     break;

  default:
    OAILOG_DEBUG (LOG_MXAP, "Unknown procedure ID (%d) for successfull outcome message\n", (int)pdu->choice.successfulOutcome.procedureCode);
    *buffer = NULL;
    *length = 0;
    return -1;
  }

  memset(&res, 0, sizeof(res));
  res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_M2AP_M2AP_PDU, pdu);
  *buffer = res.buffer;
  *length = res.result.encoded;
  return 0;
}

//------------------------------------------------------------------------------
static inline int
m2ap_mce_encode_unsuccessfull_outcome (
  M2AP_M2AP_PDU_t * pdu,
  uint8_t ** buffer,
  uint32_t * length)
{

  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch(pdu->choice.unsuccessfulOutcome.procedureCode) {
  case M2AP_ProcedureCode_id_m2Setup:
    break;

  default:
    OAILOG_DEBUG (LOG_MXAP, "Unknown procedure ID (%d) for unsuccessfull outcome message\n", (int)pdu->choice.unsuccessfulOutcome.procedureCode);
    *buffer = NULL;
    *length = 0;
    return -1;
  }

  memset(&res, 0, sizeof(res));
  res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_M2AP_M2AP_PDU, pdu);
  *buffer = res.buffer;
  *length = res.result.encoded;
  return 0;
}

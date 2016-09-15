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
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "3gpp_24.301.h"
#include "security_types.h"
#include "common_types.h"
#include "common_defs.h"
#include "intertask_interface.h"
#include "itti_getters.h"
#include "mme_api.h"
#include "emm_data.h"

//------------------------------------------------------------------------------
// get value from message into var value attribute of type u64
void nas_dl_data_req_get_mme_ue_s1ap_id (const void * const message, void *value)
{
  *((uint64_t*)value) = (uint64_t)((const itti_nas_dl_data_req_t* const)message)->ue_id;
}

//------------------------------------------------------------------------------
// get value from message into var value attribute of type bstring
void mme_app_connection_establishment_cnf_get_kenb (const void * const message, void *value_bstring)
{
  // if previous value, free it
  if (value_bstring) {
    bdestroy_wrapper((bstring*)value_bstring);
  }
  *(bstring*)value_bstring = blk2bstr(((const itti_mme_app_connection_establishment_cnf_t* const)message)->kenb,AUTH_KENB_SIZE);
}
//------------------------------------------------------------------------------
// get value from message into var value attribute of type u64
void mme_app_connection_establishment_cnf_get_erab0_transport_layer_address (const void * const message, void *value_bstring)
{
  // if previous value, free it
  if (value_bstring) {
    bdestroy_wrapper((bstring*)value_bstring);
  }
  *(bstring*)value_bstring = bstrcpy(((const itti_mme_app_connection_establishment_cnf_t* const)message)->transport_layer_address[0]);
}
//------------------------------------------------------------------------------
// get value from message into var value attribute of type u64
void mme_app_connection_establishment_cnf_get_erab0_teid (const void * const message, void *value)
{
  *((uint64_t*)value) = (teid_t)((const itti_mme_app_connection_establishment_cnf_t* const)message)->gtp_teid[0];
}

//------------------------------------------------------------------------------
// get value from message into var value attribute of type u64
void mme_app_connection_establishment_cnf_get_nas_mac (const void * const message, void *value)
{
  nas_message_t                           nas_msg = {.security_protected.header = {0},
                                                     .security_protected.plain.emm.header = {0},
                                                     .security_protected.plain.esm.header = {0}};
  nas_message_decode_status_t             decode_status = {0};
  int                                     decoder_rc = 0;
  emm_security_context_t                  emm_security_context = {0};

  decoder_rc = nas_message_decode (
      (const unsigned char *)bdata(((const itti_mme_app_connection_establishment_cnf_t* const)message)->nas_pdu[0]),
      &nas_msg,
      blength(((const itti_mme_app_connection_establishment_cnf_t* const)message)->nas_pdu[0]),
      &emm_security_context, &decode_status);
  AssertFatal (RETURNok == decoder_rc, "Error in decoding NAS ATTACH_ACCEPT");
  // assume security protected
  *((uint64_t*)value) = nas_msg.security_protected.header.message_authentication_code;
#warning "TODO free nas_message_t content"
}

//------------------------------------------------------------------------------
// get value from message into var value attribute of type u64
void mme_app_connection_establishment_cnf_get_nas_downlink_sequence_number (const void * const message, void *value)
{
  nas_message_t                           nas_msg = {.security_protected.header = {0},
                                                     .security_protected.plain.emm.header = {0},
                                                     .security_protected.plain.esm.header = {0}};
  nas_message_decode_status_t             decode_status = {0};
  int                                     decoder_rc = 0;
  emm_security_context_t                  emm_security_context = {0};

  decoder_rc = nas_message_decode (
      (const unsigned char *)bdata(((const itti_mme_app_connection_establishment_cnf_t* const)message)->nas_pdu[0]),
      &nas_msg,
      blength(((const itti_mme_app_connection_establishment_cnf_t* const)message)->nas_pdu[0]),
      &emm_security_context, &decode_status);
  AssertFatal (RETURNok == decoder_rc, "Error in decoding NAS ATTACH_ACCEPT");
  // assume security protected
  *((uint64_t*)value) = nas_msg.security_protected.header.sequence_number;
#warning "TODO free nas_message_t content"
}

//------------------------------------------------------------------------------
// get value from message into var value attribute of type u64
void mme_app_connection_establishment_cnf_get_nas_emm_guti_mtmsi (const void * const message, void *value)
{
  // assume security protected
  nas_message_t                           nas_msg = {.security_protected.header = {0},
                                                     .security_protected.plain.emm.header = {0},
                                                     .security_protected.plain.esm.header = {0}};
  nas_message_decode_status_t             decode_status = {0};
  int                                     decoder_rc = 0;
  emm_security_context_t                  emm_security_context = {0};

  decoder_rc = nas_message_decode (
      (const unsigned char *)bdata(((const itti_mme_app_connection_establishment_cnf_t* const)message)->nas_pdu[0]),
      &nas_msg,
      blength(((const itti_mme_app_connection_establishment_cnf_t* const)message)->nas_pdu[0]),
      &emm_security_context, &decode_status);
  AssertFatal (RETURNok == decoder_rc, "Error in decoding NAS ATTACH_ACCEPT");
  // assume security protected
  AssertFatal (ATTACH_ACCEPT == nas_msg.security_protected.plain.emm.header.message_type, "No NAS ATTACH_ACCEPT found in mme_app_connection_establishment_cnf");
  AssertFatal (ATTACH_ACCEPT_GUTI_IEI == (nas_msg.security_protected.plain.emm.attach_accept.presencemask & ATTACH_ACCEPT_GUTI_IEI), "No GUTI in message");
  *((uint64_t*)value) = nas_msg.security_protected.plain.emm.attach_accept.guti.guti.m_tmsi;
#warning "TODO free nas_message_t content"
}


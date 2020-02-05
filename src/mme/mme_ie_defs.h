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

/*! \file mme_ie_defs.h
 * \brief
 * \author Dincer Beken
 * \company Blackned GmbH
 * \email: dbeken@blackned.de
 *
 */

#ifndef FILE_MME_IE_DEFS_SEEN
#define FILE_MME_IE_DEFS_SEEN
#include "3gpp_24.008.h"
#include "3gpp_24.301.h"
#include "3gpp_36.401.h"
#include "common_types.h"
#include "nas_timer.h"

/* Cause as defined in 3GPP TS 29.274 #8.4 */

typedef enum {
  FCAUSE_RANAP,
  FCAUSE_BSSGP,
  FCAUSE_S1AP,
} F_Cause_Type_t;

typedef enum {
  FCAUSE_S1AP_RNL = 0,  // Radio Network Layer
  FCAUSE_S1AP_TL = 1,   // Transport Layer
  FCAUSE_S1AP_NAS = 2,  // NAS Layer
  FCAUSE_S1AP_Protocol = 3,
  FCAUSE_S1AP_Misc = 4,
} F_Cause_S1AP_Type_t;

/** Only S1AP will be supported for RAN cause. */
typedef struct {
  F_Cause_Type_t fcause_type;
  F_Cause_S1AP_Type_t fcause_s1ap_type;
  uint8_t fcause_value;
} F_Cause_t;

typedef enum {
  COMPLETE_ATTACH_REQUEST_TYPE = 0,
  COMPLETE_TAU_REQUEST_TYPE = 1,
} Complete_Request_Message_Type_t;

typedef struct F_Container {
  bstring container_value;
  uint8_t container_type;
} F_Container_t;

typedef struct Complete_Request_Message {
  bstring request_value;
  Complete_Request_Message_Type_t request_type;
} Complete_Request_Message_t;

////-----------------
// typedef struct bearer_context_setup {
//  uint8_t      eps_bearer_id;       ///< EPS Bearer ID
//  // todo: rest is for indirect data forwarding!
//} bearer_context_setup_t;

#define MAX_SETUP_BEARERS 11

///*
// * Minimal and maximal value of an EPS bearer identity:
// * The EPS Bearer Identity (EBI) identifies a message flow
// */
//#define ESM_EBI_MIN     (EPS_BEARER_IDENTITY_FIRST)
//#define ESM_EBI_MAX     (EPS_BEARER_IDENTITY_LAST)
//
///* EPS bearer context states */
// typedef enum {
//  ESM_EBR_INACTIVE = 0,     /* No EPS bearer context exists     */
//  ESM_EBR_ACTIVE,           /* The EPS bearer context is active, in the UE, in
//  the network        */ ESM_EBR_INACTIVE_PENDING, /* The network has initiated
//  an EPS bearer context deactivation towards the UE  */
//  ESM_EBR_MODIFY_PENDING,   /* The network has initiated an EPS bearer context
//  modification towards the UE  */ ESM_EBR_ACTIVE_PENDING,   /* The network has
//  initiated an EPS bearer context activation towards the UE    */
//  ESM_EBR_STATE_MAX
//} esm_ebr_state;
//
///* ESM message timer retransmission data */
// typedef struct esm_ebr_timer_data_s {
//  struct emm_context_s  *ctx;
//  mme_ue_s1ap_id_t       ue_id;      /* Lower layers UE identifier       */
//  ebi_t                  ebi;        /* EPS bearer identity          */
//  unsigned int           count;      /* Retransmission counter       */
//  bstring                msg;        /* Encoded ESM message to re-transmit */
//} esm_ebr_timer_data_t;

//#define BEARER_STATE_NULL        0
//#define BEARER_STATE_SGW_CREATED (1 << 0)
//#define BEARER_STATE_MME_CREATED (1 << 1)
//#define BEARER_STATE_ENB_CREATED (1 << 2)
//#define BEARER_STATE_ACTIVE      (1 << 3)
//#define BEARER_STATE_S1_RELEASED (1 << 4)
//
// typedef uint8_t mme_app_bearer_state_t;
//
//
///** @struct bearer_context_t
// *  @brief Parameters that should be kept for an eps bearer.
// */

#endif /* FILE_MME_IE_DEFS_SEEN */

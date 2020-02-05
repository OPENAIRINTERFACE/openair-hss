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
Source      esmData.h

Version     0.1

Date        2012/12/04

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines internal private data handled by EPS Session
        Management sublayer.

*****************************************************************************/

#ifndef __ESMDATA_H__
#define __ESMDATA_H__
#include "tree.h"

#include "mme_api.h"
#include "nas_timer.h"
#include "networkDef.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/* Total number of active EPS bearers */
#define ESM_DATA_EPS_BEARER_TOTAL 11

/** ESM byte buffer size. */
#define ESM_SAP_BUFFER_SIZE 4096

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * Minimal and maximal value of an EPS bearer identity:
 * The EPS Bearer Identity (EBI) identifies a message flow
 */
#define ESM_EBI_MIN (EPS_BEARER_IDENTITY_FIRST)
#define ESM_EBI_MAX (EPS_BEARER_IDENTITY_LAST)

/* EPS bearer context states */
typedef enum {
  ESM_EBR_INACTIVE = 0, /* No EPS bearer context exists     */
  ESM_EBR_ACTIVE, /* The EPS bearer context is active, in the UE, in the network
                   */
  ESM_EBR_INACTIVE_PENDING, /* The network has initiated an EPS bearer context
                               deactivation towards the UE  */
  ESM_EBR_MODIFY_PENDING,   /* The network has initiated an EPS bearer context
                               modification towards the UE  */
  ESM_EBR_ACTIVE_PENDING,   /* The network has initiated an EPS bearer context
                               activation towards the UE    */
  ESM_EBR_STATE_MAX
} esm_ebr_state;

/*
 * -----------------------
 * EPS bearer context data
 * -----------------------
 */
typedef struct esm_ebr_context_s {
  esm_ebr_state status; /* EPS bearer context status        */
  traffic_flow_template_t* tft;
} esm_ebr_context_t;

/*
 * --------------------------------------------------------------------------
 * Structure of data handled by EPS Session Management sublayer in the UE
 * and in the MME
 * --------------------------------------------------------------------------
 */

/* ESM procedure transaction states */
typedef enum {
  ESM_PROCEDURE_TRANSACTION_INACTIVE = 0,
  ESM_PROCEDURE_TRANSACTION_PENDING,
  ESM_PROCEDURE_TRANSACTION_MAX
} esm_pt_state_e;

/*
 * --------------------------------------------------------------------------
 *  ESM internal data handled by EPS Session Management sublayer in the MME
 * --------------------------------------------------------------------------
 */

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

extern char ip_addr_str[100];

extern char* esm_data_get_ipv4_addr(const_bstring ip_addr);

extern char* esm_data_get_ipv6_addr(const_bstring ip_addr);

extern char* esm_data_get_ipv4v6_addr(const_bstring ip_addr);

#endif /* __ESMDATA_H__*/

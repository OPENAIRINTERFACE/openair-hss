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
#ifndef FILE_EMM_COMMON_SEEN
#define FILE_EMM_COMMON_SEEN
#include "common_types.h"
#include "3gpp_36.401.h"
#include "securityDef.h"
/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

typedef enum {
  EMM_COMMON_PROC_TYPE_MIN = 0,
  EMM_COMMON_PROC_TYPE_NONE = EMM_COMMON_PROC_TYPE_MIN,
  EMM_COMMON_PROC_TYPE_GUTI_REALLOCATION,
  EMM_COMMON_PROC_TYPE_AUTHENTICATION,
  EMM_COMMON_PROC_TYPE_SECURITY_MODE_CONTROL,
  EMM_COMMON_PROC_TYPE_IDENTIFICATION,
  EMM_COMMON_PROC_TYPE_INFORMATION,
  EMM_COMMON_PROC_TYPE_NUM
} emm_common_proc_type_t;
/*
 * Type of EMM procedure callback functions
 * ----------------------------------------
 * EMM procedure to be executed under certain conditions, when an EMM common
 * procedure has been initiated by the ongoing EMM procedure.
 * - The EMM common procedure successfully completed
 * - The EMM common procedure failed or is rejected
 * - Lower layer failure occured before the EMM common procedure completion
 */
typedef int (*emm_common_success_callback_t)(void *);
typedef int (*emm_common_reject_callback_t) (void *);
typedef int (*emm_common_failure_callback_t) (void *);
typedef int (*emm_common_ll_failure_callback_t)(void *);
typedef int (*emm_common_non_delivered_callback_t)(void *);
/* EMM common procedure to be executed when the ongoing EMM procedure is
 * aborted.
 */
typedef int (*emm_common_abort_callback_t)(void *);


/*
   Internal data used for authentication procedure
*/
typedef struct {
  mme_ue_s1ap_id_t                        ue_id; /* UE identifier        */
#define AUTHENTICATION_COUNTER_MAX  5
  unsigned int                            retransmission_count; /* Retransmission counter   */
  ksi_t                                   ksi;  /* NAS key set identifier   */
  uint8_t                                 rand[AUTH_RAND_SIZE]; /* Random challenge number  */
  uint8_t                                 autn[AUTH_AUTN_SIZE]; /* Authentication token     */
  bool                                    notify_failure;       /* Indicates whether the identification
                                                                 * procedure failure shall be notified
                                                                 * to the ongoing EMM procedure */
} authentication_data_t;

/*
   Internal data used for identification procedure
*/

typedef struct {
  unsigned int                            ue_id; /* UE identifier        */
#define IDENTIFICATION_COUNTER_MAX  5
  unsigned int                            retransmission_count; /* Retransmission counter   */
  emm_proc_identity_type_t                type; /* Type of UE identity      */
  bool                                    notify_failure;       /* Indicates whether the identification
                                                                 * procedure failure shall be notified
                                                                 * to the ongoing EMM procedure */
} identification_data_t;

/*
   Internal data used for security mode control procedure
*/
typedef struct security_data_s {
  unsigned int                            ue_id; /* UE identifier                         */
#define SECURITY_COUNTER_MAX    5
  unsigned int                            retransmission_count; /* Retransmission counter    */
  int                                     ksi;  /* NAS key set identifier                */
  int                                     eea;  /* Replayed EPS encryption algorithms    */
  int                                     eia;  /* Replayed EPS integrity algorithms     */
  int                                     ucs2; /* Replayed Alphabet                     */
  int                                     uea;  /* Replayed UMTS encryption algorithms   */
  int                                     uia;  /* Replayed UMTS integrity algorithms    */
  int                                     gea;  /* Replayed G encryption algorithms      */
  bool                                    umts_present;
  bool                                    gprs_present;
  int                                     selected_eea; /* Selected EPS encryption algorithms    */
  int                                     selected_eia; /* Selected EPS integrity algorithms     */
  int                                     saved_selected_eea; /* Previous selected EPS encryption algorithms    */
  int                                     saved_selected_eia; /* Previous selected EPS integrity algorithms     */
  int                                     saved_eksi; /* Previous ksi     */
  uint16_t                                saved_overflow; /* Previous dl_count overflow     */
  uint8_t                                 saved_seq_num; /* Previous dl_count seq_num     */
  emm_sc_type_t                           saved_sc_type;
  bool                                    notify_failure;       /* Indicates whether the identification
                                                                 * procedure failure shall be notified
                                                                 * to the ongoing EMM procedure */
} security_data_t;


typedef struct emm_common_arg_s {
  union {
    authentication_data_t authentication_data;
    identification_data_t identification_data;
    security_data_t       security_data;
  } u;
} emm_common_arg_t;

struct emm_context_s;

/* Ongoing EMM procedure callback functions */
typedef struct emm_common_data_s {
  struct emm_context_s                  * container;
  emm_common_success_callback_t           success;
  emm_common_reject_callback_t            reject;
  emm_common_failure_callback_t           failure;
  emm_common_ll_failure_callback_t        ll_failure;
  emm_common_non_delivered_callback_t     non_delivered;
  emm_common_abort_callback_t             abort;

  emm_common_arg_t                        common_arg;
  // At the end of the day, we have to be sure of the type of the common_arg
  emm_common_proc_type_t                  type;
  bool                                    cleanup;
} emm_common_data_t;



/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int emm_proc_common_initialize(struct emm_context_s        * const emm_ctx,
                               emm_common_proc_type_t        const type,
                               emm_common_data_t * const emm_common_data,
                               emm_common_success_callback_t success,
                               emm_common_reject_callback_t reject,
                               emm_common_failure_callback_t failure,
                               emm_common_ll_failure_callback_t ll_failure,
                               emm_common_non_delivered_callback_t non_delivered,
                               emm_common_abort_callback_t abort);

int emm_proc_common_success(emm_common_data_t **emm_common_data);
int emm_proc_common_reject(emm_common_data_t **emm_common_data);
int emm_proc_common_failure(emm_common_data_t **emm_common_data);
int emm_proc_common_ll_failure(emm_common_data_t **emm_common_data);
int emm_proc_common_non_delivered(emm_common_data_t **emm_common_data);
int emm_proc_common_abort(emm_common_data_t **emm_common_data);

void *emm_proc_common_get_args(mme_ue_s1ap_id_t ue_id);
void emm_common_cleanup (emm_common_data_t **emm_common_data);

//struct emm_common_data_s *emm_common_data_context_get (struct emm_common_data_head_s *root, mme_ue_s1ap_id_t _ueid);

#endif /* FILE_EMM_COMMON_SEEN*/

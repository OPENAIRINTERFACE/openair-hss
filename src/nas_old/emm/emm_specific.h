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
#ifndef FILE_EMM_SPECIFIC_SEEN
#define FILE_EMM_SPECIFIC_SEEN


typedef enum {
  EMM_SPECIFIC_PROC_TYPE_MIN = 0,
  EMM_SPECIFIC_PROC_TYPE_NONE = EMM_SPECIFIC_PROC_TYPE_MIN,
  EMM_SPECIFIC_PROC_TYPE_ATTACH,
  EMM_SPECIFIC_PROC_TYPE_DETACH,
  EMM_SPECIFIC_PROC_TYPE_TAU,
  EMM_SPECIFIC_PROC_TYPE_NUM
} emm_specific_proc_type_t;

/*
 * Type of EMM procedure callback functions
 * ----------------------------------------
 * EMM procedure to be executed under certain conditions, when an EMM specific
 * procedure has been initiated.
 * - The EMM specific procedure successfully completed
 * - The EMM specific procedure failed or is rejected
 * - Lower layer failure occured before the EMM specific procedure completion
 */
struct emm_context_s;

typedef int (*emm_specific_ll_failure_callback_t)(struct emm_context_s *emm_context);
typedef int (*emm_specific_non_delivered_ho_callback_t)(struct emm_context_s *emm_context);
/* EMM specific procedure to be executed when the ongoing EMM procedure is aborted.*/
typedef int (*emm_specific_abort_callback_t)(struct emm_context_s *emm_context);




/*
   Internal data used for attach procedure
*/
typedef struct attach_data_s {
  mme_ue_s1ap_id_t                        ue_id; /* UE identifier        */
#define ATTACH_COUNTER_MAX  5
  unsigned int                            retransmission_count; /* Retransmission counter   */
  bstring                                 esm_msg;      /* ESM message to be sent within
                                                         * the Attach Accept message    */
} attach_data_t;

/*
 * Internal data used for attach procedure
 */
typedef struct tau_data_s {
  mme_ue_s1ap_id_t                        ue_id; /* UE identifier        */
#define TAU_COUNTER_MAX  5
  unsigned int                            retransmission_count; /* Retransmission counter   */
} tau_data_t;


typedef struct emm_specific_procedure_arg_s {
  union {
    attach_data_t         attach_data;
    //detach_data_t         detach_data;
    tau_data_t            tau_data;
  } u;
} emm_specific_procedure_arg_t;

struct emm_context_s;

/* Ongoing EMM procedure callback functions */
typedef struct emm_specific_procedure_data_s {
  struct emm_context_s                    * container;
  emm_specific_ll_failure_callback_t        ll_failure;
  emm_specific_non_delivered_ho_callback_t  non_delivered_ho;
  emm_specific_abort_callback_t             abort;

  emm_specific_procedure_arg_t              arg;
  // At the end of the day, we have to be sure of the type of the specific_arg
  emm_specific_proc_type_t                  type;
} emm_specific_procedure_data_t;
/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int emm_proc_specific_initialize(struct emm_context_s           * const emm_context,
                               emm_specific_proc_type_t           const type,
                               emm_specific_procedure_data_t    * const emm_specific_data,
                               emm_specific_ll_failure_callback_t       ll_failure,
                               emm_specific_non_delivered_ho_callback_t non_delivered_ho,
                               emm_specific_abort_callback_t            abort);

int emm_proc_specific_ll_failure(emm_specific_procedure_data_t **emm_specific_data);
int emm_proc_specific_non_delivered_ho(emm_specific_procedure_data_t **emm_specific_data);
int emm_proc_specific_abort(emm_specific_procedure_data_t **emm_specific_data);
void emm_proc_specific_cleanup (emm_specific_procedure_data_t **emm_specific_data);

#endif /* FILE_EMM_SPECIFIC_SEEN*/

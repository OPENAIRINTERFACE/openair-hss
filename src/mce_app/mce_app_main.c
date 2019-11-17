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

/*! \file mce_app_main.c
  \brief
  \author Dincer Beken
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "m2ap_mce.h"
#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "log.h"
#include "assertions.h"
#include "intertask_interface.h"
#include "itti_free_defined_msg.h"
#include "mme_config.h"
#include "timer.h"
#include "mme_app_bearer_context.h"
#include "mce_app_extern.h"
#include "mce_app_defs.h"
#include "mce_app_statistics.h"
#include "common_defs.h"

mce_app_desc_t                          mce_app_desc = {.rw_lock = PTHREAD_RWLOCK_INITIALIZER, 0} ;

void *mce_app_thread (void *args);

//------------------------------------------------------------------------------
void *mce_app_thread (void *args)
{
  const struct mbms_service_s 							* mbms_service  = NULL;
  const mme_app_mbms_proc_t  					        * mbms_proc  = NULL;
  itti_mark_task_ready (TASK_MCE_APP);
  MSC_START_USE ();

  while (1) {
    MessageDef                             *received_message_p = NULL;

    /*
     * Trying to fetch a message from the message queue.
     * If the queue is empty, this function will block till a
     * message is sent to the task.
     */
    itti_receive_msg (TASK_MCE_APP, &received_message_p);
    DevAssert (received_message_p );

    switch (ITTI_MSG_ID (received_message_p)) {

    case MESSAGE_TEST:{
    	OAI_FPRINTF_INFO("TASK_MCE_APP received MESSAGE_TEST\n");
    }
    break;

    /** MBMS Session Request Messages. */
    case SM_MBMS_SESSION_START_REQUEST:{
    	mce_app_handle_mbms_session_start_request(
    			&SM_MBMS_SESSION_START_REQUEST(received_message_p)
    	);
    }
    break;

    case SM_MBMS_SESSION_UPDATE_REQUEST:{
    	mce_app_handle_mbms_session_update_request(
    			&SM_MBMS_SESSION_UPDATE_REQUEST(received_message_p)
    	);
    }
    break;

    case SM_MBMS_SESSION_STOP_REQUEST:{
    	mce_app_handle_mbms_session_stop_request(
    			&SM_MBMS_SESSION_STOP_REQUEST(received_message_p)
    	);
    }
    break;

    case M3AP_ENB_SETUP_REQUEST:{
    	mce_app_handle_m3ap_enb_setup_request(
			&M3AP_ENB_SETUP_REQUEST(received_message_p)
    	);
    }
    break;

    case TERMINATE_MESSAGE:{
        /*
         * Termination message received TODO -> release any data allocated
         */
        mce_app_exit();
        itti_free_msg_content(received_message_p);
        itti_free (ITTI_MSG_ORIGIN_ID (received_message_p), received_message_p);

        OAI_FPRINTF_INFO("TASK_MCE_APP terminated\n");
        itti_exit_task ();
      }
      break;

    case TIMER_HAS_EXPIRED:{
        /*
         * Check statistic timer
         */
        if (received_message_p->ittiMsg.timer_has_expired.timer_id == mce_app_desc.statistic_timer_id) {
          mce_app_statistics_display ();
          /** Display the ITTI buffer. */
          itti_print_DEBUG ();
        } else if (received_message_p->ittiMsg.timer_has_expired.arg != NULL) {
            mbms_service_index_t mbms_service_idx = ((mbms_service_index_t)(received_message_p->ittiMsg.timer_has_expired.arg));
            mbms_service_t * mbms_service = mce_mbms_service_exists_mbms_service_index(&mce_app_desc.mce_mbms_service_contexts, mbms_service_idx);
            if (mbms_service == NULL) {
              OAILOG_WARNING (LOG_MME_APP, "Timer expired but no associated MBMS Service for MBMS Service idx " MCE_MBMS_SERVICE_INDEX_FMT "\n", mbms_service_idx);
              break;
            }
            if(mbms_service->mbms_procedure) {
              OAILOG_WARNING (LOG_MME_APP, "TIMER_HAS_EXPIRED with ID %u and FOR MBMS Service with TMGI " TMGI_FMT ". \n",
            	received_message_p->ittiMsg.timer_has_expired.timer_id, TMGI_ARG(&mbms_service->privates.fields.tmgi));
              // MBMS Session timer expiry handler
              mce_app_handle_mbms_session_duration_timer_expiry(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id);
            }
        }
      }
      break;

    default:{
      OAILOG_WARNING(LOG_MCE_APP, "Unknown message ID %d:%s\n", ITTI_MSG_ID (received_message_p), ITTI_MSG_NAME (received_message_p));
        AssertFatal (0, "Unkwnon message ID %d:%s\n", ITTI_MSG_ID (received_message_p), ITTI_MSG_NAME (received_message_p));
      }
      break;
    }

    itti_free_msg_content(received_message_p);
    itti_free(ITTI_MSG_ORIGIN_ID (received_message_p), received_message_p);
    received_message_p = NULL;
  }
  return NULL;
}

//------------------------------------------------------------------------------
int mce_app_init (const mme_config_t * mme_config_p)
{
  OAILOG_FUNC_IN (LOG_MCE_APP);

  memset (&mce_app_desc, 0, sizeof (mce_app_desc));
  // todo: (from develop)   pthread_rwlock_init (&mce_app_desc.rw_lock, NULL); && where to unlock it?
  bstring b = bfromcstr("mce_app_mbms_service_id_mbms_service_htbl");
  mce_app_desc.mce_mbms_service_contexts.mbms_service_index_mbms_service_htbl = hashtable_uint64_ts_create (mme_config.mbms.max_mbms_services, NULL, b);
  btrunc(b, 0);
//  bassigncstr(b, "mce_app_enb_ue_s1ap_id_ue_context_htbl");
//  mce_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl = hashtable_uint64_ts_create (mme_config.max_ues, NULL, b);
//  btrunc(b, 0);
  bassigncstr(b, "mce_app_tunsm_mbms_service_htbl");
  mce_app_desc.mce_mbms_service_contexts.tunsm_mbms_service_htbl = hashtable_uint64_ts_create (mme_config.mbms.max_mbms_services, NULL, b);
  AssertFatal(sizeof(uintptr_t) >= sizeof(uint64_t), "Problem with tunsm_mbms_service_htbl in MCE_APP");
  btrunc(b, 0);
  bassigncstr(b, "mce_app_mbms_service_id_mbms_service_htbl");
  mce_app_desc.mce_mbms_service_contexts.mbms_service_index_mbms_service_htbl = hashtable_ts_create (mme_config.mbms.max_mbms_services, NULL, hash_free_int_func, b);
  btrunc(b, 0);
  bdestroy_wrapper (&b);

  /**
   * Initialize the UE contexts.
   */
  STAILQ_INIT(&mce_app_desc.mce_mbms_services_list);
  /** Iterate through the list of mbms services (each a context). */
  for(int num_ms = 0; num_ms < CHANGEABLE_VALUE; num_ms++) {
	/** Create a new mutex for each and put them into the list. */
	mbms_service_t * mbms_service = &mce_app_desc.mbms_services[num_ms];
	pthread_mutexattr_t mutexattr = {0};
	int rc = pthread_mutexattr_init(&mutexattr);
	if (rc) {
		OAILOG_ERROR (LOG_MCE_APP, "Cannot create MBMS service, failed to init mutex attribute: %s\n", strerror(rc));
		OAILOG_FUNC_RETURN (LOG_MCE_APP, RETURNerror);
	}
	rc = pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
	if (rc) {
		OAILOG_ERROR (LOG_MCE_APP, "Cannot create MBMS service, failed to set mutex attribute type: %s\n", strerror(rc));
		OAILOG_FUNC_RETURN (LOG_MCE_APP, RETURNerror);
	}
	rc = pthread_mutex_init(&mbms_service->privates.recmutex, &mutexattr);
	if (rc) {
		OAILOG_ERROR (LOG_MCE_APP, "Cannot create MBMS service, failed to init mutex: %s\n", strerror(rc));
		OAILOG_FUNC_RETURN (LOG_MCE_APP, RETURNerror);
	}
	//  rc = lock_ue_contexts(new_p);
	if (rc) {
		OAILOG_ERROR (LOG_MCE_APP, "Cannot create MBMS service, failed to lock mutex: %s\n", strerror(rc));
		OAILOG_FUNC_RETURN (LOG_MCE_APP, NULL);
	}
	STAILQ_INSERT_TAIL(&mce_app_desc.mce_mbms_services_list, &mce_app_desc.mbms_services[num_ms], entries);

  }

  /*
   * Create the thread associated with MME applicative layer
   */
  if (itti_create_task (TASK_MCE_APP, &mce_app_thread, NULL) < 0) {
    OAILOG_ERROR (LOG_MCE_APP, "MCE APP create task failed\n");
    OAILOG_FUNC_RETURN (LOG_MCE_APP, RETURNerror);
  }

  mce_app_desc.statistic_timer_period = mme_config_p->mme_statistic_timer;

  /*
   * Request for periodic timer
   */
  if (timer_setup (mme_config_p->mme_statistic_timer, 0, TASK_MCE_APP, INSTANCE_DEFAULT, TIMER_PERIODIC, NULL, &mce_app_desc.statistic_timer_id) < 0) {
    OAILOG_ERROR (LOG_MCE_APP, "Failed to request new timer for statistics with %ds " "of periodicity\n", mme_config_p->mme_statistic_timer);
    mce_app_desc.statistic_timer_id = 0;
  }
  // todo: unlock the mme_desc?!

  OAILOG_DEBUG (LOG_MCE_APP, "Initializing MCE applicative layer: DONE -- ASSERTING\n");
  OAILOG_FUNC_RETURN (LOG_MCE_APP, RETURNok);
}

//------------------------------------------------------------------------------
void mce_app_exit (void)
{
  // todo: also check other timers!
  timer_remove(mce_app_desc.statistic_timer_id, NULL);
  // todo: hashtable_uint64_ts_destroy (mce_app_desc.mce_mbms_service_contexts.mbms_service_id_mbms_service_htbl);
  // todo: hashtable_uint64_ts_destroy (mce_app_desc.mme_ue_contexts.enb_ue_s1ap_id_ue_context_htbl);
  hashtable_uint64_ts_destroy (mce_app_desc.mce_mbms_service_contexts.tunsm_mbms_service_htbl);
  hashtable_ts_destroy (mce_app_desc.mce_mbms_service_contexts.mbms_service_index_mbms_service_htbl);

  mme_config_exit(); // todo: could it stay?
}

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


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"

#include "log.h"
#include "assertions.h"
#include "msc.h"
#include "intertask_interface.h"
#include "gcc_diag.h"
#include "mme_config.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"

// todo: complete rework! @ MME_APP EMM layer should be completely unaware of ESM sessions, etc.. that should all handled in the NAS layer
//------------------------------------------------------------------------------
void
mme_app_handle_detach_req (
  const itti_nas_detach_req_t * const detach_req_p)
{
  struct ue_context_s *ue_context    = NULL;
  OAILOG_FUNC_IN (LOG_MME_APP);

  DevAssert(detach_req_p != NULL);
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, detach_req_p->ue_id);
  if (ue_context == NULL) {
    OAILOG_ERROR (LOG_MME_APP, "UE context doesn't exist for ueId "MME_UE_S1AP_ID_FMT "-> Nothing to do :-) \n", detach_req_p->ue_id);
    OAILOG_FUNC_OUT (LOG_MME_APP);
  }

  // todo: review detach again! When the state transits from EMM_REG to EMM_DEREG, which method is called
  /*
   * Reset the flags of the UE.
   */
//   ue_context->subscription_known = SUBSCRIPTION_UNKNOWN;

   /** We will remove the mme_app handover procedures. */

  /**
   * A Detach Request is sent by the EMM layer after all PDN sessions are removed.
   * So, we will not check the number of PDN sessions remaining. The rest would be to remove the S1AP signaling connnection and the UE Context (toget with the procedures).
   * The S11 bearers should be 0.
   *
   * The S10 tunnel endpoint will be removed together with the S10 related procedure (inter-mme handover or emm_cn_context_request) (together with the MME_APP context deregistration).
   *
   * We don't need to check for invalidated TEID fields!
   * Delete Session Response should invalidate the S11-TEID registration in the hashtable for MME_APP UE Context.
   * If the MME_APP UE context removal happens before the arrival of the delete session response, the existing TEIDs (not 0)
   * should be used to deregister the MME_APP UE context from the hashtables.
   * That is the reason why we send and wait for PDN_DISCONNECT_RSP --> To make it clear that !
   *
   * Check if If UE is already in idle state, skip asking eNB to release UE context and just clean up locally.
   */

  /**
   * UE was already in idle mode.
   * For UE triggered detach in idle mode, it had to be active again.
   * This is network triggered detach.
   *
   * The UE context must be in UNREGISTERED mode, since everything is controller via EMM,
   * and we enter this stage only if EMM has triggered detach (which sets EMM state to EMM_DEREGSITERED) --> todo: set to EMM_DEREGISTER initiated, if it is an implicit MME detach!
   *
   * todo: if it is an MME triggered implicit detach, the UE should be in EMM_DEREGISTER_INITIATED state --> from there determine if it is an implicit detach or not..
   * If it is concluded that it is an implicit detach --> perform paging, leave the state as it is in EMM_DEREGISTER_INITIATED
   */
  if (ECM_IDLE == ue_context->privates.fields.ecm_state) {
    // todo: perform paging, if the UE is in EMM_DEREGISTER_INITIATED! state (MME triggered detach).
    // Notify S1AP to release S1AP UE context locally.
    OAILOG_DEBUG (LOG_MME_APP, "ECM context for UE with IMSI: " IMSI_64_FMT " and MME_UE_S1AP_ID : " MME_UE_S1AP_ID_FMT " (already idle). \n",
         ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id);
    // Free MME UE Context
    if(ue_context->privates.s1_ue_context_release_cause != S1AP_INVALIDATE_NAS){
      /** No context release complete is expected, so directly remove the UE context, too. */
      mme_app_itti_ue_context_release (ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, S1AP_IMPLICIT_CONTEXT_RELEASE, ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id); /**< Set the signaling connection to ECM_IDLE when the Context-Removal-Completion has arrived. */
      mme_remove_ue_context (&mme_app_desc.mme_ue_contexts, ue_context);
    }
    /** UE Context already released from source eNodeB. */
    else if (ue_context->privates.s1_ue_context_release_cause == S1AP_SUCCESSFUL_HANDOVER){ /**< Wait for a response. */
      OAILOG_DEBUG (LOG_MME_APP, "UE context will be released and resources removed due HANDOVER for UE with IMSI: " IMSI_64_FMT " and MME_UE_S1AP_ID : %d. \n",
          ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id);
      /** Remove the UE context. */
      mme_remove_ue_context (&mme_app_desc.mme_ue_contexts, ue_context);
    }
    else{
      OAILOG_DEBUG (LOG_MME_APP, "UE context will not be released for UE with IMSI: " IMSI_64_FMT " and MME_UE_S1AP_ID : %d (already idle). \n",
           ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id);
      mme_app_itti_ue_context_release (ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, S1AP_IMPLICIT_CONTEXT_RELEASE, ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id); /**< Set the signaling connection to ECM_IDLE when the Context-Removal-Completion has arrived. */
      // todo: not released --> remove the handover procedure
      ue_context->privates.s1_ue_context_release_cause = S1AP_HANDOVER_CANCELLED;
      mme_app_delete_s10_procedure_mme_handover(ue_context);
      ue_context->privates.s1_ue_context_release_cause = S1AP_INVALID_CAUSE;

    }
  } else {  /**< UE has an active context. Setting NAS_DETACH as S1AP cause and sending the context removal command! */
    if (ue_context->privates.s1_ue_context_release_cause == S1AP_INVALID_CAUSE) {
      ue_context->privates.s1_ue_context_release_cause = S1AP_NAS_DETACH;
    }
    /**
     * Notify S1AP to send UE Context Release Command to eNB. Signaling connection will be set to idle after context completion complete.
     * This starts a timer to wait for the context removal completion. If timeout happens, the S1AP UE reference will be notified and the MMME_APP will be called.
     * If the S1AP context removal response does not arrive, the MME_APP UE context may already be removed. Not a problem for the MME_APP. The S1AP UE reference will be removed.
     */
    mme_app_itti_ue_context_release (ue_context->privates.mme_ue_s1ap_id, ue_context->privates.fields.enb_ue_s1ap_id, ue_context->privates.s1_ue_context_release_cause, ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id);
    /**
     * We may or may not expect a response from the eNodeB context removal depending on the set release cause, to remove the rest of the UE context and deregistrate the UE.
     * If the response does not arrive, the S1AP timeout will do the rest.
     * No further timeout in the NAS-EMM. NAS will send detach once. It will remove the EMM context immediately.
     * Nothing special to do for S10. Should be removed with mme_remove_ue_context!
     *
     * Not need to remove with successfully completing the handover on the target-MME side. Without removing a second handover from the target side should be possible!
     *
     * Just continue without waiting for a response from the eNodeB for the S1AP context removal, if its a general shutdown.
     */
    if (ue_context->privates.s1_ue_context_release_cause == S1AP_SCTP_SHUTDOWN_OR_RESET) {
      /**
       * Just cleanup the MME APP state associated with s1. Set the connection state to invalid.. Acting like the S1AP connection is already released.
       * This will also deregister the ENB_UE_S1AP_ID key from the registration of the MME_APP.
       * Getting back from IDLE will always be with an INITIAL_UE_REQUEST. There the new ENB_ID will be registered for the MME_APP UE context!
       *
       * Signaling Transition:
       * The signaling connection is activated, after the UE has checked the initial context request and permitted.
       * This may not be eventually directly with initial ctx setup request (from eNB), but later after MME verifies.
       * In between the MME_APP context, which is directly created might be needed for the enb_ID, that's why register before and separately.
       * That's why, don't provide the enb_id as an argument.
       * Getting back from IDLE_TO_ACTIVE with this method won't register a new key, just acknowledgement by the MME and stop the any idle-timers.
       */
      mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);
      // S1AP, nothing more to do. Free MME UE Context
      mme_remove_ue_context (&mme_app_desc.mme_ue_contexts, ue_context);
    }
    else if (ue_context->privates.s1_ue_context_release_cause == S1AP_SUCCESSFUL_HANDOVER){ /**< Wait for a response. */
      OAILOG_DEBUG (LOG_MME_APP, "UE context will be released and resources removed due HANDOVER for UE with IMSI: " IMSI_64_FMT " and MME_UE_S1AP_ID : %d. \n",
          ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id);

      mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);
      /** Remove the UE context. */
      mme_remove_ue_context (&mme_app_desc.mme_ue_contexts, ue_context);
    }
    else if (ue_context->privates.s1_ue_context_release_cause == S1AP_INVALIDATE_NAS){ /**< Wait for a response. */
      OAILOG_DEBUG (LOG_MME_APP, "UE context will not be released since only NAS is invalidated for IMSI: " IMSI_64_FMT " and MME_UE_S1AP_ID : %d. \n",
          ue_context->privates.fields.imsi, ue_context->privates.mme_ue_s1ap_id);
      mme_ue_context_update_ue_sig_connection_state (&mme_app_desc.mme_ue_contexts, ue_context, ECM_IDLE);
      // todo: manually release the mme_app_handover procedure // emm_cn_procedure?!
      // todo: not released --> remove the handover procedure
      ue_context->privates.s1_ue_context_release_cause = S1AP_HANDOVER_CANCELLED;
      mme_app_delete_s10_procedure_mme_handover(ue_context);
      ue_context->privates.s1_ue_context_release_cause = S1AP_INVALID_CAUSE;
    }
  }
  OAILOG_FUNC_OUT (LOG_MME_APP);
}

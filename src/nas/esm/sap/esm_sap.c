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
  Source      esm_sap.c

  Version     0.1

  Date        2012/11/22

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Frederic Maurel

  Description Defines the ESM Service Access Points at which the EPS
        Session Management sublayer provides procedures for the
        EPS bearer context handling and resources allocation.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "bstrlib.h"
#include "assertions.h"

#include "log.h"
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "nas_message.h"
#include "common_defs.h"
#include "TLVDecoder.h"
#include "esm_ebr.h"
#include "esm_proc.h"
#include "esm_cause.h"
#include "esm_msg.h"
#include "esm_recv.h"
#include "esm_send.h"
#include "esm_msgDef.h"
#include "esm_sap.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/
extern int _pdn_connectivity_delete (esm_context_t * esm_context, int pid,   ebi_t     default_ebi);
/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/


static int _esm_sap_recv (
  int msg_type,
  bool is_standalone,
  esm_context_t * esm_context,
  const_bstring req,
  bstring rsp,
  esm_sap_error_t * err);

static int _esm_sap_send (
  int msg_type,
  bool is_standalone,
  esm_context_t * esm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const esm_sap_data_t * data,
  bstring * rsp);

/*
   String representation of ESM-SAP primitives
*/
static const char                      *_esm_sap_primitive_str[] = {
  "ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REQ",
  "ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_CNF",
  "ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REJ",
  "ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_REQ",
  "ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_CNF",
  "ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_REJ",
  "ESM_EPS_BEARER_CONTEXT_MODIFY_REQ",
  "ESM_EPS_BEARER_CONTEXT_MODIFY_CNF",
  "ESM_EPS_BEARER_CONTEXT_MODIFY_REJ",
  "ESM_DEDICATED_EPS_BEARER_CONTEXT_DEACTIVATE_REQ",
  "ESM_DEDICATED_EPS_BEARER_CONTEXT_DEACTIVATE_CNF",

  "ESM_EPS_UPDATE_ESM_BEARER_CTXS_REQ",

  "ESM_PDN_CONFIG_RES",

  "ESM_PDN_CONNECTIVITY_REQ",
  "ESM_PDN_CONNECTIVITY_CNF",
  "ESM_PDN_CONNECTIVITY_REJ",
  "ESM_PDN_DISCONNECT_REQ",
  "ESM_PDN_DISCONNECT_CNF",
  "ESM_PDN_DISCONNECT_REJ",
  "ESM_BEARER_RESOURCE_ALLOCATE_REQ",
  "ESM_BEARER_RESOURCE_ALLOCATE_REJ",
  "ESM_BEARER_RESOURCE_MODIFY_REQ",
  "ESM_BEARER_RESOURCE_MODIFY_REJ",
  "ESM_UNITDATA_IND",
};


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    esm_sap_initialize()                                      **
 **                                                                        **
 ** Description: Initializes the ESM Service Access Point state machine    **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    NONE                                       **
 **                                                                        **
 ***************************************************************************/
void
esm_sap_initialize (
  void)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  /*
   * Initialize ESM state machine
   */
  //esm_fsm_initialize();
  OAILOG_FUNC_OUT (LOG_NAS_ESM);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_sap_send()                                            **
 **                                                                        **
 ** Description: Processes the ESM Service Access Point primitive          **
 **                                                                        **
 ** Inputs:  msg:       The ESM-SAP primitive to process           **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
esm_sap_send (esm_sap_t * msg)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     rc = RETURNerror;
  pdn_cid_t                               pid = MAX_APN_PER_UE;
  ebi_t                                   ebi = 0;

  /*
   * Check the ESM-SAP primitive
   */
  esm_primitive_t                         primitive   = msg->primitive;
  esm_cause_t                             esm_cause   = ESM_CAUSE_SUCCESS;

  pdn_context_t                          *pdn_context = NULL;
  pdn_context_t                          *established_pdn = NULL;
  ue_context_t                           *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, msg->ctx->ue_id);

  assert ((primitive > ESM_START) && (primitive < ESM_END));
  OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Received primitive %s (%d)\n", _esm_sap_primitive_str[primitive - ESM_START - 1], primitive);

  switch (primitive) {
  case ESM_PDN_CONNECTIVITY_REQ:
    /*
     * The MME received a PDN connectivity request message
     */
    rc = _esm_sap_recv (PDN_CONNECTIVITY_REQUEST, msg->is_standalone, msg->ctx, msg->recv, msg->send, &msg->err);
    break;

  case ESM_PDN_CONNECTIVITY_REJ:
    /*
     * PDN connectivity locally failed
     */
    rc = esm_proc_default_eps_bearer_context_failure (msg->ctx, &msg->data.pdn_connect.cid, &msg->data.pdn_connect.linked_ebi);

    if (rc != RETURNerror) {
      rc = esm_proc_pdn_connectivity_failure (msg->ctx, msg->data.pdn_connect.cid, ebi);
    }
    _esm_sap_send(PDN_CONNECTIVITY_REJECT,msg->is_standalone, msg->ctx, msg->ctx->esm_proc_data->pti, msg->data.pdn_connect.linked_ebi, &msg->data, &msg->send);

    break;

  case ESM_PDN_DISCONNECT_REQ:
    /*
     * MME (detach) or UE PDN disconnection procedure.
     * If MME initiated procedure, the PDN context will be removed locally before sending the delete session request, but one at a time.
     * At detach, also multiple PDN connections may exist.
     *
     *
     * Without waiting for UE confirmation, directly update the ESM context.
     * Send S11 Delete Session Request to the SAE-GW.
     *
     * Execute the PDN disconnect procedure requested by the UE.
     * Validating the message in the context of the UE and finding the PDN context to remove.
     */
    rc = esm_proc_eps_bearer_context_deactivate(msg->ctx, msg->data.pdn_disconnect.local_delete, msg->data.pdn_disconnect.default_ebi, msg->data.pdn_disconnect.cid, &esm_cause);
    if (rc != RETURNerror) {
      /** If no local pdn context deletion, directly continue with the NAS/S1AP message. */
      int pid = esm_proc_pdn_disconnect_request( msg->ctx, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, msg->data.pdn_disconnect.cid, msg->data.pdn_disconnect.default_ebi, &esm_cause);
      if (pid == RETURNerror) {
        rc = RETURNerror;
      }else{
        /** Delete the PDN connection locally (todo: also might do this together with removing all bearers/default bearer). */
        proc_tid_t pti = _pdn_connectivity_delete (msg->ctx, msg->data.pdn_disconnect.cid, msg->data.pdn_disconnect.default_ebi);
      }
    }else{
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
//      DevAssert(0);
    }
    break;

  case ESM_PDN_DISCONNECT_CNF:{
    /** Check if we do a local detach or not. */
    OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Continuing with standalone PDN disconnection for ueId " MME_UE_S1AP_ID_FMT ". \n",
          msg->ue_id);
    /*
     * Create a deactivate EPS bearer context request message, set the status as pending deactivation and continue.
     * The EBI should already be set when the ESM message has been received.
     */
    rc = _esm_sap_send(DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST,
        msg->is_standalone, msg->ctx, msg->ctx->esm_proc_data->pti, msg->ctx->esm_proc_data->ebi,
        &msg->data, &msg->send);
  }
  break;

  case ESM_PDN_CONFIG_RES:
    /*
     * Check if an APN is given, if so get the pdn configuration for it and set the pdn_connectivity flag as false.
     * Else, go through all PDNs.
     */
    if(msg->data.pdn_config_res.apn){
      rc = esm_proc_pdn_config_res(msg->ctx,
          &msg->data.pdn_config_res.pdn_cid,
          &msg->data.pdn_config_res.is_pdn_connectivity,
          msg->data.pdn_config_res.imsi,
          msg->data.pdn_config_res.apn,
          &msg->data.pdn_config_res.default_ebi, &esm_cause);
    }else{
      RB_FOREACH (established_pdn, PdnContexts, &ue_context->pdn_contexts) {
        msg->data.pdn_config_res.apn = established_pdn->apn_subscribed;
        /** Not setting the context identifier. */
        rc = esm_proc_pdn_config_res(msg->ctx,
            &msg->data.pdn_config_res.pdn_cid,
            &msg->data.pdn_config_res.is_pdn_connectivity,
            msg->data.pdn_config_res.imsi,
            msg->data.pdn_config_res.apn,
            &msg->data.pdn_config_res.default_ebi, &esm_cause);
      }
    }
    break;

  case ESM_PDN_DISCONNECT_REJ:
    break;

  case ESM_BEARER_RESOURCE_ALLOCATE_REQ:
    break;

  case ESM_BEARER_RESOURCE_ALLOCATE_REJ:
    /*
     * Reject the bearer, stop the timer and put it into the empty pool.
     * This might be due a failed establishment.
     * No need to inform the MME_APP layer again.
     */
    rc = esm_proc_eps_bearer_context_deactivate (msg->ctx, true, msg->data.esm_bearer_resource_allocate_rej.ebi,
        PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, NULL);
    /** If this was the default bearer, release the pdn connectivity. */
    break;

  case ESM_BEARER_RESOURCE_MODIFY_REQ:
    break;

  case ESM_BEARER_RESOURCE_MODIFY_REJ:
    /*
      * Reject the bearer, stop the timer and put it into the empty pool.
      * This might be due a failed establishment.
      */
     if(msg->data.esm_bearer_resource_modify_rej.remove){
       OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Removing bearer with ebi %d for ueId " MME_UE_S1AP_ID_FMT " due failed modification. \n",
           msg->data.esm_bearer_resource_modify_rej.ebi, msg->ue_id);
       /** Will return false, if the bearer is already removed. */
       rc = esm_proc_eps_bearer_context_deactivate (msg->ctx, true, msg->data.esm_bearer_resource_allocate_rej.ebi,
           PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, NULL);
     }else{
       OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Not removing bearer with ebi %d for ueId " MME_UE_S1AP_ID_FMT " due failed modification. \n",
           msg->data.esm_bearer_resource_modify_rej.ebi, msg->ue_id);

       /** Abort the ESM modification procedure, if existing and still pending.. */
       esm_cause =  ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED;
       rc = esm_proc_modify_eps_bearer_context_reject(msg->ctx, msg->data.esm_bearer_resource_modify_rej.ebi, &esm_cause, false);
     }
     /**
      * No need to to inform the MME_APP of the removal.
      * A CBResp/UBResp (incl. removing the count of unhandled bearers) might already have been triggered.
      */
     break;

  case ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REQ:{
    /** The PDN context should exist, set the received APN_AMBR and IP address, which would be necessary for retransmission. */

    rc = _esm_sap_send(ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST,
               msg->is_standalone, msg->ctx, msg->ctx->esm_proc_data->pti, msg->data.pdn_connect.linked_ebi, &msg->data, &msg->send);
    if(rc == RETURNerror) {
      OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Could not trigger default bearer context allocation for ebi %d with cxdId %d for UE " MME_UE_S1AP_ID_FMT "! "
          "Triggering PDN Context rejection. \n", msg->data.pdn_connect.linked_ebi, msg->data.pdn_connect.cid, msg->ctx->ue_id);
      /** Try sending a PDN Connectivity reject. */
      _esm_sap_send(PDN_CONNECTIVITY_REJECT,msg->is_standalone, msg->ctx, msg->ctx->esm_proc_data->pti, msg->data.pdn_connect.linked_ebi, &msg->data, &msg->send);

      rc = esm_proc_eps_bearer_context_deactivate(msg->ctx, msg->data.pdn_disconnect.local_delete, msg->data.pdn_disconnect.default_ebi, msg->data.pdn_disconnect.cid, &esm_cause);
      if (rc != RETURNerror) {

        /** If no local pdn context deletion, directly continue with the NAS/S1AP message. */
        int pid = esm_proc_pdn_disconnect_request( msg->ctx, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, msg->data.pdn_disconnect.cid, msg->data.pdn_disconnect.default_ebi, &esm_cause);
        if (pid == RETURNerror) {
          rc = RETURNerror;
        }else{
          /** Delete the PDN connection locally (todo: also might do this together with removing all bearers/default bearer). */
          proc_tid_t pti = _pdn_connectivity_delete (msg->ctx, msg->data.pdn_disconnect.cid, msg->data.pdn_disconnect.default_ebi);
        }

      }else{
        OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
     //      DevAssert(0);
      }
      OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - After failed sending ACTIVATE DEFAULT BEARER CONTEXT, triggering CN session removal for "
          "for pdnCtxId %d (ebi=%d) for ue " MME_UE_S1AP_ID_FMT "!\n", msg->data.pdn_connect.cid, msg->data.pdn_connect.linked_ebi, msg->ctx->ue_id);
    }else{
      OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Successfully completed triggering the allocation of default bearer context with ebi %d for pdnCtxId %d for ue " MME_UE_S1AP_ID_FMT "!\n",
          msg->data.pdn_connect.linked_ebi, msg->data.pdn_connect.cid, msg->ctx->ue_id);
    }
  }
  break;

  case ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_CNF:
    /*
     * The MME received activate default ESP bearer context accept
     */
    rc = _esm_sap_recv (ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT, msg->is_standalone, msg->ctx, msg->recv, msg->send, &msg->err);
    break;

  case ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REJ:
    /*
     * The MME received activate default ESP bearer context reject
     */
    rc = _esm_sap_recv (ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT, msg->is_standalone, msg->ctx, msg->recv, msg->send, &msg->err);
    break;

  case ESM_DEDICATED_EPS_BEARER_CONTEXT_ACTIVATE_REQ: {
    esm_eps_activate_eps_bearer_ctx_req_t* bearer_activate = &msg->data.eps_dedicated_bearer_context_activate;
    if (msg->is_standalone) {
      /** Handle each bearer context separately. */
      /** Get the bearer contexts to be created. */
      bearer_contexts_to_be_created_t * bcs_tbc = (bearer_contexts_to_be_created_t*)bearer_activate->bcs_to_be_created_ptr;
      for(int num_bc = 0; num_bc < bcs_tbc->num_bearer_context; num_bc++){
        rc = esm_proc_dedicated_eps_bearer_context (msg->ctx,     /**< Create an ESM procedure and store the bearers in the procedure as pending. */
            bearer_activate->linked_ebi,
            bearer_activate->pti,
            bearer_activate->cid,
            &bcs_tbc->bearer_contexts[num_bc],
            &esm_cause);
        /** For each bearer separately process with the bearer establishment. */
        if (rc != RETURNok) {   /**< We assume that no ESM procedure exists. */
          OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Failed to handle CN bearer context procedure due SYSTEM_FAILURE for num_bearer %d!\n",
              _esm_sap_primitive_str[primitive - ESM_START - 1], primitive, num_bc);
          /**
           * Send a NAS ITTI directly for the specific bearer. This will reduce the number of bearers to be processed.
           * No bearer should be allocated.
           */
          nas_itti_activate_eps_bearer_ctx_rej(msg->ue_id, bcs_tbc->bearer_contexts[num_bc].s1u_sgw_fteid.teid, esm_cause); /**< Assuming, no other CN bearer procedure will intervere. */
          /**<
           * We will check the remaining bearer requests and reject bearers individually (we might have a mix of rejected and accepted bearers).
           * The remaining must also be rejected, such that the procedure has no pending elements anymore.
           */
          continue;
        }
        OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Successfully allocated bearer with ebi %d for ue " MME_UE_S1AP_ID_FMT "!\n",
            bcs_tbc->bearer_contexts[num_bc].eps_bearer_id, ue_context->mme_ue_s1ap_id);
        /* Send Activate Dedicated Bearer Context Request */
        rc = _esm_sap_send(ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST,
            msg->is_standalone, msg->ctx, (proc_tid_t)bearer_activate->pti, bcs_tbc->bearer_contexts[num_bc].eps_bearer_id,
            &msg->data, &msg->send);
        /** Check the resulting error code. */
        if(rc == RETURNerror) {
          OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Could not trigger allocation of dedicated bearer context with ebi %d for ue " MME_UE_S1AP_ID_FMT "!\n",
              bcs_tbc->bearer_contexts[num_bc].eps_bearer_id, ue_context->mme_ue_s1ap_id);
          /** Removed the created EPS bearer context. */
          rc = esm_proc_eps_bearer_context_deactivate (msg->ctx, true, bcs_tbc->bearer_contexts[num_bc].eps_bearer_id,  /**< Set above. */
              PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, NULL);
          /** Inform the MME APP. */
          nas_itti_activate_eps_bearer_ctx_rej(msg->ue_id, bcs_tbc->bearer_contexts[num_bc].s1u_sgw_fteid.teid, esm_cause); /**< Assuming, no other CN bearer procedure will intervere. */
        }else{
          OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Successfully completed triggering the allocation of dedicated bearer context with ebi %d for ue " MME_UE_S1AP_ID_FMT "!\n",
              bcs_tbc->bearer_contexts[num_bc].eps_bearer_id, ue_context->mme_ue_s1ap_id);
        }
      }
    }
  }
  break;

  case ESM_EPS_BEARER_CONTEXT_MODIFY_REQ: {
     esm_eps_modify_eps_bearer_ctx_req_t* bearer_modify = &msg->data.eps_bearer_context_modify;
     if (msg->is_standalone) {
       /** Handle each bearer context separately. */
       bearer_contexts_to_be_updated_t * bcs_tbu = (bearer_contexts_to_be_updated_t*)bearer_modify->bcs_to_be_updated_ptr;
       for(int num_bc = 0; num_bc < bcs_tbu->num_bearer_context; num_bc++){
         rc = esm_proc_modify_eps_bearer_context (msg->ctx,     /**< Just setting the state as MODIFY_PENDING. */
             bearer_modify->pti,
             &bcs_tbu->bearer_contexts[num_bc],
             &esm_cause);
         if (rc != RETURNok) {   /**< We assume that no ESM procedure exists. */
           OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Failed to handle CN modify bearer context procedure due SYSTEM_FAILURE for num_bearer %d!\n", num_bc);
           /** Send a NAS ITTI directly for the specific bearer. This will reduce the number of bearers to be processed. */
           nas_itti_modify_eps_bearer_ctx_rej(msg->ue_id, bcs_tbu->bearer_contexts[num_bc].eps_bearer_id, esm_cause); /**< Assuming, no other CN bearer procedure will intervere. */
           continue; /**< The remaining must also be rejected, such that the procedure has no pending elements anymore. */
         }
         OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Successfully modified bearer context with ebi %d for ue " MME_UE_S1AP_ID_FMT "!\n",
             bcs_tbu->bearer_contexts[num_bc].eps_bearer_id, ue_context->mme_ue_s1ap_id);
         /* Send Modify Bearer Context Request */
         rc = _esm_sap_send(MODIFY_EPS_BEARER_CONTEXT_REQUEST,
             msg->is_standalone, msg->ctx, (proc_tid_t)bearer_modify->pti, bcs_tbu->bearer_contexts[num_bc].eps_bearer_id,
             &msg->data, &msg->send);
         /** Check the resulting error code. */
         if(rc == RETURNerror){
           OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Could not trigger allocation of dedicated bearer context with ebi %d for ue " MME_UE_S1AP_ID_FMT "!\n",
               bcs_tbu->bearer_contexts[num_bc].eps_bearer_id, ue_context->mme_ue_s1ap_id);
           /** Removed the created EPS bearer context. */
           rc = esm_proc_modify_eps_bearer_context_reject(msg->ctx, bcs_tbu->bearer_contexts[num_bc].eps_bearer_id,
               &esm_cause, false);
           /** Inform the MME APP. */
           nas_itti_modify_eps_bearer_ctx_rej(msg->ue_id, bcs_tbu->bearer_contexts[num_bc].eps_bearer_id, esm_cause); /**< Assuming, no other CN bearer procedure will intervere. */
         }else{
           OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Successfully completed triggering the allocation of dedicated bearer context with ebi %d for ue " MME_UE_S1AP_ID_FMT "!\n",
               bcs_tbu->bearer_contexts[num_bc].eps_bearer_id, ue_context->mme_ue_s1ap_id);
         }
       }
     }
   }
   break;

  case ESM_DEDICATED_EPS_BEARER_CONTEXT_DEACTIVATE_REQ:{
    esm_eps_deactivate_eps_bearer_ctx_req_t* bearer_deactivate = &msg->data.eps_dedicated_bearer_context_deactivate;
    if (msg->is_standalone) {
      esm_cause_t esm_cause;

      // todo: currently single messages, sum up to one big message
      for(int num_bc = 0; num_bc < bearer_deactivate->ebis.num_ebi; num_bc++){
        rc = esm_proc_eps_bearer_context_deactivate(msg->ctx, false,
            bearer_deactivate->ebis.ebis[num_bc],
            bearer_deactivate->cid, &esm_cause);

        if(rc != RETURNerror){
          /* Send Activate Dedicated Bearer Context Request */
          rc = _esm_sap_send(DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST,
              msg->is_standalone, msg->ctx, (proc_tid_t)bearer_deactivate->pti,  bearer_deactivate->ebis.ebis[num_bc],
              &msg->data, &msg->send);
        }else{
          OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Error deactivating bearer context for UE " MME_UE_S1AP_ID_FMT " and ebi %d. \n",
             ue_context->mme_ue_s1ap_id, bearer_deactivate->ebis.ebis[num_bc]);
          if(esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY){
            /** Inform the MME_APP about the missing EBI to continue with the DBResp. */
            nas_itti_dedicated_eps_bearer_deactivation_complete(ue_context->mme_ue_s1ap_id, bearer_deactivate->ebis.ebis[num_bc]);
          }
        }
      }
    }
  }
  break;

  case ESM_EPS_UPDATE_ESM_BEARER_CTXS_REQ: {
    /** Check that the bearers are active, and update the tft, qos, and apn-ambr values. */
    // todo: apn ambr to ue ambr calculation !
    esm_eps_update_esm_bearer_ctxs_req_t* update_esm_bearer_ctxs = &msg->data.eps_update_esm_bearer_ctxs;
    esm_cause_t esm_cause;
    /** Update the bearer level QoS TFT values. */
    for(int num_bc = 0; num_bc < msg->data.eps_update_esm_bearer_ctxs.bcs_to_be_updated->num_bearer_context; num_bc++){
      rc = esm_proc_update_eps_bearer_context(msg->ctx, &msg->data.eps_update_esm_bearer_ctxs.bcs_to_be_updated->bearer_contexts[num_bc]);
      if(rc != RETURNerror){
        /* Successfully updated the bearer context. */
        OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Successfully updated ebi %d to final values via UBR for UE " MME_UE_S1AP_ID_FMT ". \n",
            msg->data.eps_update_esm_bearer_ctxs.bcs_to_be_updated->bearer_contexts[num_bc].eps_bearer_id, ue_context->mme_ue_s1ap_id);
      }else{
        OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Error while updating ebi %d to final values via UBR for UE " MME_UE_S1AP_ID_FMT ". \n",
            msg->data.eps_update_esm_bearer_ctxs.bcs_to_be_updated->bearer_contexts[num_bc].eps_bearer_id, ue_context->mme_ue_s1ap_id);
        /**
         * Bearer may stay with old parameters and in active state.
         * We assume that the bearer got removed due some concurrency  issues.
         * We continue to handle the remaining bearers.
         */
      }
    }
  }
  break;

  case ESM_UNITDATA_IND:
    /** Check that an EMM context exists, if not disregard the message. */
    if(msg->ctx)
      rc = _esm_sap_recv (-1, msg->is_standalone, msg->ctx, msg->recv, msg->send, &msg->err);
    else{
      OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - No EMM context received for ESM_UNITDATA_IND. \n");
      rc = RETURNerror;
    }
    break;

  default:
    break;
  }

  if (rc != RETURNok) {
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Failed to process primitive %s (%d)\n", _esm_sap_primitive_str[primitive - ESM_START - 1], primitive);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    _esm_sap_recv()                                           **
 **                                                                        **
 ** Description: Processes ESM messages received from the network: Decodes **
 **      the message and checks whether it is of the expected ty-  **
 **      pe, checks the validity of the procedure transaction iden-**
 **      tity, checks the validity of the EPS bearer identity, and **
 **      parses the message content.                               **
 **      If no protocol error is found the ESM response message is **
 **      returned in order to be sent back onto the network upon   **
 **      the relevant ESM procedure completion.                    **
 **      If a protocol error is found the ESM status message is    **
 **      returned including the value of the ESM cause code.       **
 **                                                                        **
 ** Inputs:  msg_type:  Expected type of the received ESM message  **
 **      is_standalone: Indicates whether the ESM message has been **
 **             received standalone or together within EMM **
 **             attach related message                     **
 **      ue_id:      UE identifier within the MME               **
 **      req:       The encoded ESM request message to process **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     rsp:       The encoded ESM response message to be re- **
 **             turned upon ESM procedure completion       **
 **      err:       Error code of the ESM procedure            **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
static int
_esm_sap_recv (
  int msg_type,
  bool is_standalone,
  esm_context_t * esm_context,
  const_bstring req,
  bstring rsp,
  esm_sap_error_t * err)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_proc_procedure_t                    esm_procedure = NULL;
  esm_cause_t                             esm_cause = ESM_CAUSE_SUCCESS;
  int                                     rc = RETURNerror;
  int                                     decoder_rc;
  ESM_msg                                 esm_msg;

  /** An
  memset (&esm_msg, 0, sizeof (ESM_msg));
  /*
   * Decode the received ESM message
   */
  memset(&esm_msg, 0, sizeof(ESM_msg));

  decoder_rc = esm_msg_decode (&esm_msg, (uint8_t *)bdata(req), blength(req));

  /*
   * Process decoding errors
   */
  if (decoder_rc < 0) {
    /*
     * 3GPP TS 24.301, section 7.2
     * * * * Ignore received message that is too short to contain a complete
     * * * * message type information element
     */
    if (decoder_rc == TLV_BUFFER_TOO_SHORT) {
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Discard message too short to contain a complete message type IE\n");
      /*
       * Return indication that received message has been discarded
       */
      *err = ESM_SAP_DISCARDED;
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
    }
    /*
     * 3GPP TS 24.301, section 7.2
     * * * * Unknown or unforeseen message type
     */
    else if (decoder_rc == TLV_WRONG_MESSAGE_TYPE) {
      esm_cause = ESM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED;
    }
    /*
     * 3GPP TS 24.301, section 7.7.2
     * * * * Conditional IE errors
     */
    else if (decoder_rc == TLV_UNEXPECTED_IEI) {
      esm_cause = ESM_CAUSE_CONDITIONAL_IE_ERROR;
    } else {
      esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
    }
  }
  /*
   * Check the type of the ESM message actually received
   */
  else if ((msg_type > 0) && (msg_type != esm_msg.header.message_type)) {
    if (esm_msg.header.message_type != ESM_STATUS) {
      /*
       * Semantically incorrect ESM message
       */
      OAILOG_ERROR (LOG_NAS_ESM , "ESM-SAP   - Received ESM message 0x%x is not of the expected type (0x%x)\n", esm_msg.header.message_type, msg_type);
      esm_cause = ESM_CAUSE_SEMANTICALLY_INCORRECT;
    }
  }

  /*
   * Get the procedure transaction identity
   */
  pti_t                            pti = esm_msg.header.procedure_transaction_identity;

  /*
   * Get the EPS bearer identity
   */
  ebi_t                            ebi = esm_msg.header.eps_bearer_identity;

  /*
   * Indicate whether the ESM bearer context related procedure was triggered
   * * * * by the receipt of a transaction related request message from the UE or
   * * * * was triggered network-internally
   */
  int                                     triggered_by_ue = (pti != PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED);

  /*
   * Indicate whether the received message shall be ignored
   */
  bool                                     is_discarded = false;

  if (esm_cause != ESM_CAUSE_SUCCESS) {
    OAILOG_ERROR (LOG_NAS_ESM , "ESM-SAP   - Failed to decode expected ESM message 0x%x cause %d\n", msg_type, esm_cause);
  }
  /*
   * Process the received ESM message
   */
  else
    switch (esm_msg.header.message_type) {
    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
      /*
       * Process activate default EPS bearer context accept message
       * received from the UE
       */
      esm_cause = esm_recv_activate_default_eps_bearer_context_accept (esm_context, pti, ebi, &esm_msg.activate_default_eps_bearer_context_accept);

      if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
        /*
         * 3GPP TS 24.301, section 7.3.1, case f
         * * * * Ignore ESM message received with reserved PTI value
         * * * * 3GPP TS 24.301, section 7.3.2, case f
         * * * * Ignore ESM message received with reserved or assigned
         * * * * value that does not match an existing EPS bearer context
         */
        is_discarded = true;
      }

      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT:
      /*
       * Process activate default EPS bearer context reject message
       * received from the UE
       */
      esm_cause = esm_recv_activate_default_eps_bearer_context_reject (esm_context, pti, ebi, &esm_msg.activate_default_eps_bearer_context_reject);

      if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
        /*
         * 3GPP TS 24.301, section 7.3.1, case f
         * * * * Ignore ESM message received with reserved PTI value
         * * * * 3GPP TS 24.301, section 7.3.2, case f
         * * * * Ignore ESM message received with reserved or assigned
         * * * * value that does not match an existing EPS bearer context
         */
        is_discarded = true;
      }

      break;

    case DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT:
      /*
       * Process deactivate EPS bearer context accept message
       * received from the UE
       */
      esm_cause = esm_recv_deactivate_eps_bearer_context_accept (esm_context, pti, ebi, &esm_msg.deactivate_eps_bearer_context_accept);

      if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
        /*
         * 3GPP TS 24.301, section 7.3.1, case f
         * * * * Ignore ESM message received with reserved PTI value
         * * * * 3GPP TS 24.301, section 7.3.2, case f
         * * * * Ignore ESM message received with reserved or assigned
         * * * * value that does not match an existing EPS bearer context
         */
        is_discarded = true;
      }

      break;

    case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT:
      /*
       * Process activate dedicated EPS bearer context accept message
       * received from the UE
       */
      esm_cause = esm_recv_activate_dedicated_eps_bearer_context_accept (esm_context, pti, ebi, &esm_msg.activate_dedicated_eps_bearer_context_accept);

      if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
        /*
         * 3GPP TS 24.301, section 7.3.1, case f
         * * * * Ignore ESM message received with reserved PTI value
         * * * * 3GPP TS 24.301, section 7.3.2, case f
         * * * * Ignore ESM message received with reserved or assigned
         * * * * value that does not match an existing EPS bearer context
         */
        is_discarded = true;
      }

      break;

    case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT:
      /*
       * Process activate dedicated EPS bearer context reject message
       * received from the UE
       */
      esm_cause = esm_recv_activate_dedicated_eps_bearer_context_reject (esm_context, pti, ebi, &esm_msg.activate_dedicated_eps_bearer_context_reject);

      if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
        /*
         * 3GPP TS 24.301, section 7.3.1, case f
         * * * * Ignore ESM message received with reserved PTI value
         * * * * 3GPP TS 24.301, section 7.3.2, case f
         * * * * Ignore ESM message received with reserved or assigned
         * * * * value that does not match an existing EPS bearer context
         */
        is_discarded = true;
      }

      break;

    case MODIFY_EPS_BEARER_CONTEXT_ACCEPT:
      /*
       * Process activate dedicated EPS bearer context accept message
       * received from the UE
       */
      esm_cause = esm_recv_modify_eps_bearer_context_accept (esm_context, pti, ebi, &esm_msg.modify_eps_bearer_context_accept);

      if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
        /*
         * 3GPP TS 24.301, section 7.3.1, case f
         * * * * Ignore ESM message received with reserved PTI value
         * * * * 3GPP TS 24.301, section 7.3.2, case f
         * * * * Ignore ESM message received with reserved or assigned
         * * * * value that does not match an existing EPS bearer context
         */
        is_discarded = true;
      }

      break;

    case MODIFY_EPS_BEARER_CONTEXT_REJECT:
      /*
       * Process modify EPS bearer context reject message
       * received from the UE
       */
      esm_cause = esm_recv_modify_eps_bearer_context_reject (esm_context, pti, ebi, &esm_msg.modify_eps_bearer_context_reject);

      if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
        /*
         * 3GPP TS 24.301, section 7.3.1, case f
         * * * * Ignore ESM message received with reserved PTI value
         * * * * 3GPP TS 24.301, section 7.3.2, case f
         * * * * Ignore ESM message received with reserved or assigned
         * * * * value that does not match an existing EPS bearer context
         */
        is_discarded = true;
      }

      break;

    case PDN_CONNECTIVITY_REQUEST:{
        OAILOG_DEBUG (LOG_NAS_ESM, "ESM-SAP   - PDN_CONNECTIVITY_REQUEST pti %u ebi %u\n", pti, ebi);
        /*
         * Process PDN connectivity request message received from the UE
         */
        bool                                    is_pdn_connectivity = false;
        pdn_context_t                          *new_pdn_context     = NULL;
        esm_cause = esm_recv_pdn_connectivity_request (esm_context, pti, ebi, &esm_msg.pdn_connectivity_request, &ebi, &is_pdn_connectivity, NULL /** todo: imsi_t*/, &new_pdn_context);

        if (esm_cause != ESM_CAUSE_SUCCESS) {
          rc = esm_send_pdn_connectivity_reject (pti, &esm_msg.pdn_connectivity_reject, esm_cause);
          /*
           * Setup the callback function used to send PDN connectivity
           * * * * reject message onto the network
           */
          esm_procedure = esm_proc_pdn_connectivity_reject;
        } else {
#if ORIGINAL_CODE
          /*
           * Setup PDN type
           */
          int                                     pdn_type = -1;

          if (data.pdn_type == ESM_PDN_TYPE_IPV4) {
            pdn_type = PDN_VALUE_TYPE_IPV4;
          } else if (data.pdn_type == ESM_PDN_TYPE_IPV6) {
            pdn_type = PDN_VALUE_TYPE_IPV6;
          } else if (data.pdn_type == ESM_PDN_TYPE_IPV4V6) {
            pdn_type = PDN_VALUE_TYPE_IPV4V6;
          }

          /*
           * Setup EPS bearer level Quality of Service
           */
          EpsQualityOfService                     qos;

          qos.bitRatesPresent = 1;
          qos.bitRatesExtPresent = 0;
          qos.qci = data.qos.qci;
          qos.bitRates.maxBitRateForUL = data.qos.mbrUL;
          qos.bitRates.maxBitRateForDL = data.qos.mbrDL;
          qos.bitRates.guarBitRateForUL = data.qos.gbrUL;
          qos.bitRates.guarBitRateForDL = data.qos.gbrDL;
          /*
           * Return default EPS bearer context request message
           */
          rc = esm_send_activate_default_eps_bearer_context_request (pti, ebi, &esm_msg.activate_default_eps_bearer_context_request, &data.apn, pdn_type, &data.pdn_addr, &qos, esm_cause);
#  if 0
          PacketFilters                           pkfs;

          pkfs[0].identifier = 1;
          pkfs[0].direction = TRAFFIC_FLOW_TEMPLATE_DOWNLINK_ONLY;
          pkfs[0].eval_precedence = 2;
          pkfs[0].packetfilter.flags = (TRAFFIC_FLOW_TEMPLATE_IPV4_REMOTE_ADDR_FLAG | TRAFFIC_FLOW_TEMPLATE_PROTOCOL_NEXT_HEADER_FLAG | TRAFFIC_FLOW_TEMPLATE_SINGLE_LOCAL_PORT_FLAG | TRAFFIC_FLOW_TEMPLATE_SINGLE_REMOTE_PORT_FLAG);
          pkfs[0].packetfilter.ipv4remoteaddr[0].addr = 192;
          pkfs[0].packetfilter.ipv4remoteaddr[1].addr = 168;
          pkfs[0].packetfilter.ipv4remoteaddr[2].addr = 12;
          pkfs[0].packetfilter.ipv4remoteaddr[3].addr = 1;
          pkfs[0].packetfilter.ipv4remoteaddr[0].mask = 255;
          pkfs[0].packetfilter.ipv4remoteaddr[1].mask = 255;
          pkfs[0].packetfilter.ipv4remoteaddr[2].mask = 255;
          pkfs[0].packetfilter.ipv4remoteaddr[3].mask = 0;
          pkfs[0].packetfilter.protocolidentifier_nextheader = 17;
          pkfs[0].packetfilter.singlelocalport = 10001;
          pkfs[0].packetfilter.singleremoteport = 12001;
          pkfs[1].identifier = 2;
          pkfs[1].direction = TRAFFIC_FLOW_TEMPLATE_UPLINK_ONLY;
          pkfs[1].eval_precedence = 3;
          pkfs[1].packetfilter.flags = (TRAFFIC_FLOW_TEMPLATE_IPV4_REMOTE_ADDR_FLAG | TRAFFIC_FLOW_TEMPLATE_PROTOCOL_NEXT_HEADER_FLAG | TRAFFIC_FLOW_TEMPLATE_SINGLE_LOCAL_PORT_FLAG | TRAFFIC_FLOW_TEMPLATE_SINGLE_REMOTE_PORT_FLAG);
          pkfs[1].packetfilter.ipv4remoteaddr[0].addr = 192;
          pkfs[1].packetfilter.ipv4remoteaddr[1].addr = 168;
          pkfs[1].packetfilter.ipv4remoteaddr[2].addr = 12;
          pkfs[1].packetfilter.ipv4remoteaddr[3].addr = 1;
          pkfs[1].packetfilter.ipv4remoteaddr[0].mask = 255;
          pkfs[1].packetfilter.ipv4remoteaddr[1].mask = 255;
          pkfs[1].packetfilter.ipv4remoteaddr[2].mask = 255;
          pkfs[1].packetfilter.ipv4remoteaddr[3].mask = 0;
          pkfs[1].packetfilter.protocolidentifier_nextheader = 17;
          pkfs[1].packetfilter.singlelocalport = 10002;
          pkfs[1].packetfilter.singleremoteport = 12002;
          /*
           * Return dedicated EPS bearer context request message
           */
          rc = esm_send_activate_dedicated_eps_bearer_context_request (pti, ebi, &esm_msg.activate_dedicated_eps_bearer_context_request, ebi, &qos, &pkfs, 2);
#  endif
          esm_procedure = esm_proc_default_eps_bearer_context_request;
#else
          esm_cause = ESM_CAUSE_SUCCESS;
#endif
          if(!is_standalone){
            *err = ESM_SAP_SUCCESS;
             OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
          }
          ue_context_t * ue_context  = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, esm_context->ue_id);
          /** Get the configuration. */
          DevAssert(new_pdn_context);

          /** If a PDN connectivity exists, perform an activate default eps bearer context request. */
          if(is_pdn_connectivity){
            /*
             * Get the PDN context.
             */
            bearer_context_t * bearer_context_duplicate = mme_app_get_session_bearer_context(new_pdn_context, new_pdn_context->default_ebi);
            DevAssert(bearer_context_duplicate);

            memset (&esm_msg, 0, sizeof (ESM_msg));
            esm_proc_pdn_type_t                     esm_pdn_type = ESM_PDN_TYPE_IPV4;

            esm_sap_t esm_sap;
            memset(&esm_sap, 0, sizeof(esm_sap_t));
            /** Trigger default EPS Bearer Activation. */
            esm_sap.data.pdn_connect.cid = new_pdn_context->context_identifier;
            esm_sap.data.pdn_connect.pdn_type = new_pdn_context->pdn_type;
            esm_sap.data.pdn_connect.apn = new_pdn_context->apn_subscribed;
//            esm_sap.data.pdn_connect.pco = new_pdn_context->pco;
            esm_sap.data.pdn_connect.linked_ebi = new_pdn_context->default_ebi;
            /** The PDN Context PAA/AMBR values will be updated with the CSResp in the MME_APP bearer handler.*/
            esm_sap.data.pdn_connect.apn_ambr = new_pdn_context->subscribed_apn_ambr; /**< Updated with the value from the PCRF. */
            esm_sap.data.pdn_connect.pdn_addr = paa_to_bstring(new_pdn_context->paa); /**< For this, we need to store the IP information. */
            /*
             * Return default EPS bearer context request message
             */
            rc = _esm_sap_send(ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST, is_standalone,
                esm_context, pti, new_pdn_context->default_ebi, &esm_sap.data, &esm_sap.send);
          }else{
            OAILOG_INFO (LOG_NAS_EMM, "No PDN connectivity exists for the second APN \"%s\" for ueId " MME_UE_S1AP_ID_FMT ". "
                "Establishing PDN connection. \n", bdata(esm_msg.activate_default_eps_bearer_context_request.accesspointname), esm_context->ue_id);
            nas_itti_pdn_connectivity_req (esm_context->esm_proc_data->pti, esm_context->ue_id, new_pdn_context->context_identifier, new_pdn_context->default_ebi,
                esm_context->esm_proc_data, esm_context->esm_proc_data->request_type);
            /** No PDN connectivity exists yet. Leaving with PDN connectivity establishment. No procedure needed. */
            rc = RETURNok;
          }
        }
        break;
      }

    case PDN_DISCONNECT_REQUEST:
      /*
       * Process PDN disconnect request message received from the UE.
       * Get the Linked Default EBI.
       */
      esm_cause = esm_recv_pdn_disconnect_request (esm_context, pti, ebi, &esm_msg.pdn_disconnect_request, &ebi);

      if (esm_cause != ESM_CAUSE_SUCCESS) {
        /*
         * Return reject message
         */
        rc = esm_send_pdn_disconnect_reject (pti, &esm_msg.pdn_disconnect_reject, esm_cause);
        /*
         * Setup the callback function used to send PDN connectivity
         * * * * reject message onto the network
         */
        esm_procedure = esm_proc_pdn_disconnect_reject;
        /*
         * No ESM status message should be returned
         */
        esm_cause = ESM_CAUSE_SUCCESS;
      } else {
        /*
         * Return deactivate EPS bearer context request message
         */
//        rc = esm_send_deactivate_eps_bearer_context_request (pti, ebi, &esm_msg.deactivate_eps_bearer_context_request, ESM_CAUSE_REGULAR_DEACTIVATION);
        /*
         * Setup the callback function used to send deactivate EPS
         * * * * bearer context request message onto the network
         */
//        esm_procedure = esm_proc_eps_bearer_context_deactivate_request;
      }

      break;

    case BEARER_RESOURCE_ALLOCATION_REQUEST:
      break;

    case BEARER_RESOURCE_MODIFICATION_REQUEST:
      break;

    case ESM_INFORMATION_RESPONSE:
      esm_cause = esm_recv_information_response (esm_context, pti, ebi, &esm_msg.esm_information_response);
      break;

    case ESM_STATUS:
      /*
       * Process received ESM status message
       */
      esm_cause = esm_recv_status (esm_context, pti, ebi, &esm_msg.esm_status);
      break;

    default:
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Received unexpected ESM message " "0x%x\n", esm_msg.header.message_type);
      esm_cause = ESM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED;
      break;
    }

  if ((esm_cause != ESM_CAUSE_SUCCESS) && (esm_procedure == NULL)) {
    /*
     * ESM message processing failed
     */
    if (!is_discarded) {
      /*
       * 3GPP TS 24.301, section 7.1
       * * * * Handling of unknown, unforeseen, and erroneous protocol data
       */
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Received ESM message is not valid " "(cause=%d)\n", esm_cause);
      /*
       * Return an ESM status message
       */
      rc = esm_send_status (pti, ebi, &esm_msg.esm_status, esm_cause);
      /*
       * Setup the callback function used to send ESM status message
       * * * * onto the network
       */
      esm_procedure = esm_proc_status;
      /*
       * Discard received ESM message
       */
      is_discarded = true;
    }
  } else {
    /*
     * ESM message processing succeed
     */
    *err = ESM_SAP_SUCCESS;
    rc = RETURNok;
  }

  if ((rc != RETURNerror) && (esm_procedure )) {
    #define ESM_SAP_BUFFER_SIZE 4096
    uint8_t           esm_sap_buffer[ESM_SAP_BUFFER_SIZE];
    /*
     * Encode the returned ESM response message
     */
    int size = esm_msg_encode (&esm_msg, (uint8_t *) esm_sap_buffer, ESM_SAP_BUFFER_SIZE);

    if (size > 0) {
      rsp = blk2bstr(esm_sap_buffer, size);
    }

    /*
     * Complete the relevant ESM procedure
     */
    rc = (*esm_procedure) (is_standalone, esm_context, ebi, &rsp, triggered_by_ue);

    if (is_discarded) {
      /*
       * Return indication that received message has been discarded
       */
      *err = ESM_SAP_DISCARDED;
    } else if (rc != RETURNok) {
      /*
       * Return indication that ESM procedure failed
       */
      *err = ESM_SAP_FAILED;
    }
  } else if (is_discarded) {
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Silently discard message type 0x%x\n", esm_msg.header.message_type);
    /*
     * Return indication that received message has been discarded
     */
    *err = ESM_SAP_DISCARDED;
    rc = RETURNok;
  }

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _esm_sap_send()                                           **
 **                                                                        **
 ** Description: Processes ESM messages to send onto the network: Encoded  **
 **      the message and execute the relevant ESM procedure.       **
 **                                                                        **
 ** Inputs:  msg_type:  Type of the ESM message to be sent         **
 **      is_standalone: Indicates whether the ESM message has to   **
 **             be sent standalone or together within EMM  **
 **             attach related message                     **
 **      ue_id:      UE identifier within the MME               **
 **      pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      data:      Data required to build the message         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     rsp:       The encoded ESM response message to be re- **
 **             turned upon ESM procedure completion       **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
static int
_esm_sap_send (
  int msg_type,
  bool is_standalone,
  esm_context_t * esm_context,
  proc_tid_t pti,
  ebi_t ebi,
  const esm_sap_data_t * data,
  bstring * rsp)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_proc_procedure_t                    esm_procedure = NULL;
  int                                     rc = RETURNok;

  /*
   * Indicate whether the message is sent by the UE or the MME
   */
  bool                                    sent_by_ue = false;
  ESM_msg                                 esm_msg;

  memset (&esm_msg, 0, sizeof (ESM_msg));
  esm_proc_pdn_type_t esm_pdn_type = 0;
  /*
   * Process the ESM message to send
   */
  switch (msg_type) {
  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST: {
    switch (data->pdn_connect.pdn_type) {
    case IPv4:
      OAILOG_INFO (LOG_NAS_EMM, "EMM  -  esm_pdn_type = ESM_PDN_TYPE_IPV4\n");
      esm_pdn_type = ESM_PDN_TYPE_IPV4;
      break;

    case IPv6:
      OAILOG_INFO (LOG_NAS_EMM, "EMM  -  esm_pdn_type = ESM_PDN_TYPE_IPV6\n");
      esm_pdn_type = ESM_PDN_TYPE_IPV6;
      break;

    case IPv4_AND_v6:
      OAILOG_INFO (LOG_NAS_EMM, "EMM  -  esm_pdn_type = ESM_PDN_TYPE_IPV4V6\n");
      esm_pdn_type = ESM_PDN_TYPE_IPV4V6;
      break;

    default:
      OAILOG_INFO (LOG_NAS_EMM, "EMM  -  esm_pdn_type = ESM_PDN_TYPE_IPV4 (forced to default)\n");
      esm_pdn_type = ESM_PDN_TYPE_IPV4;
    }

    EpsQualityOfService eps_qos = {0};
    memset((void*)&eps_qos, 0, sizeof(eps_qos));
    eps_qos.qci = data->pdn_connect.qci;
    /*
     * Return default EPS bearer context request message.
     */
    rc = esm_send_activate_default_eps_bearer_context_request (pti, ebi,
        &esm_msg.activate_default_eps_bearer_context_request,
        data->pdn_connect.apn,
        data->pdn_connect.pco, &data->pdn_connect.apn_ambr,
        esm_pdn_type, data->pdn_connect.pdn_addr,
        &eps_qos, ESM_CAUSE_SUCCESS);
    /*
     * Complete the relevant ESM procedure
     */
    esm_procedure = esm_proc_default_eps_bearer_context_request;
  }
  break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST: {
      const   esm_eps_activate_eps_bearer_ctx_req_t *msg = &data->eps_dedicated_bearer_context_activate;
      EpsQualityOfService eps_qos = {0};
      /** Sending a EBR-Request per bearer context. */
      bearer_contexts_to_be_created_t *bcs_tbc = (bearer_contexts_to_be_created_t*)msg->bcs_to_be_created_ptr;
      for(int num_bc = 0; num_bc < bcs_tbc->num_bearer_context; num_bc++) {
        if(bcs_tbc->bearer_contexts[num_bc].eps_bearer_id == ebi){
          memset((void*)&eps_qos, 0, sizeof(eps_qos));
          /** Set the EPS QoS. */
          rc = qos_params_to_eps_qos(bcs_tbc->bearer_contexts[num_bc].bearer_level_qos.qci,
              bcs_tbc->bearer_contexts[num_bc].bearer_level_qos.mbr.br_dl,bcs_tbc->bearer_contexts[num_bc].bearer_level_qos.mbr.br_ul,
              bcs_tbc->bearer_contexts[num_bc].bearer_level_qos.gbr.br_dl, bcs_tbc->bearer_contexts[num_bc].bearer_level_qos.gbr.br_ul,
              &eps_qos, false);
          if (RETURNok == rc) {
            rc = esm_send_activate_dedicated_eps_bearer_context_request (
                pti, bcs_tbc->bearer_contexts[num_bc].eps_bearer_id,
                &esm_msg.activate_dedicated_eps_bearer_context_request,
                msg->linked_ebi, &eps_qos,
                &bcs_tbc->bearer_contexts[num_bc].tft,
                &bcs_tbc->bearer_contexts[num_bc].pco);
            esm_procedure = esm_proc_dedicated_eps_bearer_context_request; /**< Not the procedure. */
          }
          /** Exit the loop directly. */
          OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Successfully sent request to establish dedicated bearer for ebi %d for UE " MME_UE_S1AP_ID_FMT " . \n",
              ebi, esm_context->ue_id);
          break;
        }
      }
    }
    /** Exit the case always here. */
    break;

  case MODIFY_EPS_BEARER_CONTEXT_REQUEST:{
    const   esm_eps_modify_eps_bearer_ctx_req_t *msg = &data->eps_bearer_context_modify;

    EpsQualityOfService eps_qos = {0};
    memset((void*)&eps_qos, 0, sizeof(eps_qos));
    /** Sending a EBR-Request per bearer context. */
    bearer_contexts_to_be_updated_t *bcs_tbu = (bearer_contexts_to_be_updated_t*)msg->bcs_to_be_updated_ptr;
    for(int num_bc = 0; num_bc < bcs_tbu->num_bearer_context; num_bc++){
      if(bcs_tbu->bearer_contexts[num_bc].eps_bearer_id == ebi){
        if(bcs_tbu->bearer_contexts[num_bc].bearer_level_qos){
          memset((void*)&eps_qos, 0, sizeof(eps_qos));
          rc = qos_params_to_eps_qos(bcs_tbu->bearer_contexts[num_bc].bearer_level_qos->qci,
              bcs_tbu->bearer_contexts[num_bc].bearer_level_qos->mbr.br_dl,
              bcs_tbu->bearer_contexts[num_bc].bearer_level_qos->mbr.br_ul,
              bcs_tbu->bearer_contexts[num_bc].bearer_level_qos->gbr.br_dl,
              bcs_tbu->bearer_contexts[num_bc].bearer_level_qos->gbr.br_ul,
              &eps_qos, false);
        }
        if (RETURNok == rc) {
          ambr_t * ambr = !num_bc ? &msg->apn_ambr: NULL; /**< Send the new received AMBR (if exists) with the first bearer update message. */
          rc = esm_send_modify_eps_bearer_context_request (
              pti, bcs_tbu->bearer_contexts[num_bc].eps_bearer_id,
              &esm_msg.activate_dedicated_eps_bearer_context_request, &eps_qos,
              &bcs_tbu->bearer_contexts[num_bc].tft,
              ambr, &bcs_tbu->bearer_contexts[num_bc].pco);

          esm_procedure = esm_proc_modify_eps_bearer_context_request; /**< Not the procedure. */
        }
        /** Exit the loop directly. */
        OAILOG_INFO (LOG_NAS_ESM, "ESM-SAP   - Successfully sent request to modify bearer for ebi %d for UE " MME_UE_S1AP_ID_FMT " . \n",
            ebi, esm_context->ue_id);
        break;
      }
    }
  }
  break;

  case DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST:
    /**
     * Create and encode the message,
     * set the procedure to set the pending status and to start the timers.
     * The EBI should already be set when the ESM message has been received.
     * This is always non local, since else we don't send message sent.
     */
    rc = esm_send_deactivate_eps_bearer_context_request (pti, ebi,
        &esm_msg.deactivate_eps_bearer_context_request, ESM_CAUSE_REGULAR_DEACTIVATION);
    if (rc != RETURNerror) {
      /** If no local pdn context deletion, directly continue with the NAS/S1AP message.
       * Setup the callback function used to send deactivate EPS
       * * * * bearer context request message onto the network
       */
      esm_procedure = esm_proc_eps_bearer_context_deactivate_request;
    }
    break;

  case PDN_CONNECTIVITY_REJECT:
    rc = esm_send_pdn_connectivity_reject (pti, &esm_msg.pdn_connectivity_reject, data->pdn_connect.esm_cause);
    /*
     * Setup the callback function used to send PDN connectivity
     * * * * reject message onto the network
     */
    esm_procedure = esm_proc_pdn_connectivity_reject;
    break;

  case PDN_DISCONNECT_REJECT:
    break;

  case BEARER_RESOURCE_ALLOCATION_REJECT:
    break;

  case BEARER_RESOURCE_MODIFICATION_REJECT:
    break;

  default:
    OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Send unexpected ESM message 0x%x\n", msg_type);
    break;
  }

  if (rc != RETURNerror) {
    #define ESM_SAP_BUFFER_SIZE 4096
    uint8_t           esm_sap_buffer[ESM_SAP_BUFFER_SIZE];
    /*
     * Encode the returned ESM response message
     */
    int size = esm_msg_encode (&esm_msg, esm_sap_buffer, ESM_SAP_BUFFER_SIZE);

    if (size > 0) {
      *rsp = blk2bstr(esm_sap_buffer, size);
    } else{
      OAILOG_WARNING (LOG_NAS_ESM, "ESM-SAP   - Error encoding ESM message 0x%x\n", msg_type);
      OAILOG_FUNC_RETURN(LOG_NAS_ESM, RETURNerror);
    }

    /*
     * Execute the relevant ESM procedure
     */
    if (esm_procedure) {
      rc = (*esm_procedure) (is_standalone, esm_context, ebi, rsp, sent_by_ue);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
}

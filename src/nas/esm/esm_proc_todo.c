
//        case MODIFY_EPS_BEARER_CONTEXT_ACCEPT:
//          /*
//           * Process activate dedicated EPS bearer context accept message
//           * received from the UE
//           */
//          esm_cause = esm_recv_modify_eps_bearer_context_accept (esm_context, pti, ebi, &esm_msg.modify_eps_bearer_context_accept);
//
//          if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
//            /*
//             * 3GPP TS 24.301, section 7.3.1, case f
//             * * * * Ignore ESM message received with reserved PTI value
//             * * * * 3GPP TS 24.301, section 7.3.2, case f
//             * * * * Ignore ESM message received with reserved or assigned
//             * * * * value that does not match an existing EPS bearer context
//             */
//            is_discarded = true;
//          }
//
//          break;
//
//        case MODIFY_EPS_BEARER_CONTEXT_REJECT:
//          /*
//           * Process modify EPS bearer context reject message
//           * received from the UE
//           */
//          esm_cause = esm_recv_modify_eps_bearer_context_reject (esm_context, pti, ebi, &esm_msg.modify_eps_bearer_context_reject);
//
//          if ((esm_cause == ESM_CAUSE_INVALID_PTI_VALUE) || (esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY)) {
//            /*
//             * 3GPP TS 24.301, section 7.3.1, case f
//             * * * * Ignore ESM message received with reserved PTI value
//             * * * * 3GPP TS 24.301, section 7.3.2, case f
//             * * * * Ignore ESM message received with reserved or assigned
//             * * * * value that does not match an existing EPS bearer context
//             */
//            is_discarded = true;
//          }
//
//          break;


/////////////////////////////////////////// ESM-SAP

//  case ESM_PDN_DISCONNECT_REQ:
//    /*
//     * MME (detach) or UE PDN disconnection procedure.
//     * If MME initiated procedure, the PDN context will be removed locally before sending the delete session request, but one at a time.
//     * At detach, also multiple PDN connections may exist.
//     *
//     *
//     * Without waiting for UE confirmation, directly update the ESM context.
//     * Send S11 Delete Session Request to the SAE-GW.
//     *
//     * Execute the PDN disconnect procedure requested by the UE.
//     * Validating the message in the context of the UE and finding the PDN context to remove.
//     */
//    rc = esm_proc_eps_bearer_context_deactivate(msg->ctx, msg->data.pdn_disconnect.local_delete, msg->data.pdn_disconnect.default_ebi, msg->data.pdn_disconnect.cid, &esm_cause);
//    if (rc != RETURNerror) {
//      /** If no local pdn context deletion, directly continue with the NAS/S1AP message. */
//      int pid = esm_proc_pdn_disconnect_request( msg->ctx, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, msg->data.pdn_disconnect.cid, msg->data.pdn_disconnect.default_ebi, &esm_cause);
//      if (pid == RETURNerror) {
//        rc = RETURNerror;
//      }else{
//        /** Delete the PDN connection locally (todo: also might do this together with removing all bearers/default bearer). */
//        proc_tid_t pti = _pdn_connectivity_delete (msg->ctx, msg->data.pdn_disconnect.cid, msg->data.pdn_disconnect.default_ebi);
//      }
//    }else{
//      OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
////      DevAssert(0);
//    }
//    break;

//  case ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REQ:{
//    /** The PDN context should exist, set the received APN_AMBR and IP address, which would be necessary for retransmission. */
//
//    rc = _esm_sap_send(ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST,
//               msg->ctx, msg->ctx->esm_proc_data->pti, msg->data.pdn_connect.linked_ebi, &msg->data, &msg->send);
//    if(rc == RETURNerror) {
//      OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Could not trigger default bearer context allocation for ebi %d with cxdId %d for UE " MME_UE_S1AP_ID_FMT "! "
//          "Triggering PDN Context rejection. \n", msg->data.pdn_connect.linked_ebi, msg->data.pdn_connect.cid, msg->ctx->ue_id);
//      /** Try sending a PDN Connectivity reject. */
//      _esm_sap_send(PDN_CONNECTIVITY_REJECT, msg->ctx, msg->ctx->esm_proc_data->pti, msg->data.pdn_connect.linked_ebi, &msg->data, &msg->send);
//
//      rc = esm_proc_eps_bearer_context_deactivate(msg->ctx, msg->data.pdn_disconnect.local_delete, msg->data.pdn_disconnect.default_ebi, msg->data.pdn_disconnect.cid, &esm_cause);
//      if (rc != RETURNerror) {
//
//        /** If no local pdn context deletion, directly continue with the NAS/S1AP message. */
//        int pid = esm_proc_pdn_disconnect_request( msg->ctx, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, msg->data.pdn_disconnect.cid, msg->data.pdn_disconnect.default_ebi, &esm_cause);
//        if (pid == RETURNerror) {
//          rc = RETURNerror;
//        }else{
//          /** Delete the PDN connection locally (todo: also might do this together with removing all bearers/default bearer). */
//          proc_tid_t pti = _pdn_connectivity_delete (msg->ctx, msg->data.pdn_disconnect.cid, msg->data.pdn_disconnect.default_ebi);
//        }
//
//      }else{
//        OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
//     //      DevAssert(0);
//      }
//      OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - After failed sending ACTIVATE DEFAULT BEARER CONTEXT, triggering CN session removal for "
//          "for pdnCtxId %d (ebi=%d) for ue " MME_UE_S1AP_ID_FMT "!\n", msg->data.pdn_connect.cid, msg->data.pdn_connect.linked_ebi, msg->ctx->ue_id);
//    }else{
//      OAILOG_INFO(LOG_NAS_ESM, "ESM-SAP   - Successfully completed triggering the allocation of default bearer context with ebi %d for pdnCtxId %d for ue " MME_UE_S1AP_ID_FMT "!\n",
//          msg->data.pdn_connect.linked_ebi, msg->data.pdn_connect.cid, msg->ctx->ue_id);
//    }
//  }
//  break;
//
//  case ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_CNF:
//    /*
//     * The MME received activate default ESP bearer context accept
//     */
//    rc = _esm_sap_recv (ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT, msg->ctx, msg->recv, msg->send, &msg->err);
//    break;
//
//
//  case ESM_DEDICATED_EPS_BEARER_CONTEXT_DEACTIVATE_REQ:{
//    esm_eps_deactivate_eps_bearer_ctx_req_t* bearer_deactivate = &msg->data.eps_dedicated_bearer_context_deactivate;
//    esm_cause_t esm_cause;
//
//    // todo: currently single messages, sum up to one big message
//    for(int num_bc = 0; num_bc < bearer_deactivate->ebis.num_ebi; num_bc++){
//      rc = esm_proc_eps_bearer_context_deactivate(msg->ctx, false,
//          bearer_deactivate->ebis.ebis[num_bc],
//          bearer_deactivate->cid, &esm_cause);
//
//      if(rc != RETURNerror){
//        /* Send Activate Dedicated Bearer Context Request */
//        rc = _esm_sap_send(DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST, msg->ctx, (proc_tid_t)bearer_deactivate->pti,  bearer_deactivate->ebis.ebis[num_bc],
//            &msg->data, &msg->send);
//      }else{
//        OAILOG_ERROR (LOG_NAS_ESM, "ESM-SAP   - Error deactivating bearer context for UE " MME_UE_S1AP_ID_FMT " and ebi %d. \n",
//            ue_context->mme_ue_s1ap_id, bearer_deactivate->ebis.ebis[num_bc]);
//        if(esm_cause == ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY){
//          /** Inform the MME_APP about the missing EBI to continue with the DBResp. */
//          nas_itti_dedicated_eps_bearer_deactivation_complete(ue_context->mme_ue_s1ap_id, bearer_deactivate->ebis.ebis[num_bc]);
//        }
//      }
//    }
//  }
//  break;


/// PDN DISCONNECT
////------------------------------------------------------------------------------
//static int _esm_cn_pdn_disconnect_res(const esm_cn_pdn_disconnect_res_t * msg)
//{
//  OAILOG_FUNC_IN (LOG_NAS_ESM);
//  int                                     rc = RETURNok;
//  struct emm_data_context_s              *emm_context = NULL;
//  ESM_msg                                 esm_msg = {.header = {0}};
//  int                                     esm_cause;
//  esm_sap_t                               esm_sap = {0};
//  bool                                    pdn_local_delete = false;
//
//  ue_context_t *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, msg->ue_id);
//  DevAssert(ue_context); // todo:
//
//  emm_context = emm_data_context_get (&_emm_data, msg->ue_id);
//  if (emm_context == NULL) {
//    OAILOG_ERROR (LOG_NAS_ESM, "EMMCN-SAP  - " "Failed to find UE associated to id " MME_UE_S1AP_ID_FMT "...\n", msg->ue_id);
//    OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
//  }
//  memset (&esm_msg, 0, sizeof (ESM_msg));
//
//  /** todo: Inform the ESM layer about the PDN session removal. */
//
//  // todo: check if detach_proc is active or UE is in DEREGISTRATION_INITIATED state, dn's
//
//  /*
//   * Check if there is a specific detach procedure running.
//   * In that case, check if other PDN contexts exist. Delete those sessions, too.
//   * Finally send Detach Accept back to the UE.
//   */
//  nas_emm_detach_proc_t  *detach_proc = get_nas_specific_procedure_detach(emm_context);
//  // todo: implicit detach ? specific procedure?
//  if(detach_proc || EMM_DEREGISTERED_INITIATED == emm_fsm_get_state(emm_context)){
//    OAILOG_INFO (LOG_NAS_ESM, "Detach procedure ongoing for UE " MME_UE_S1AP_ID_FMT". Performing local PDN context deletion. \n", emm_context->ue_id);
//    pdn_local_delete = true;
//  }
//  /** Check for the number of pdn connections left. */
//  if(false /*!emm_context->esm_ctx.n_pdns */){
//    OAILOG_INFO (LOG_NAS_ESM, "No more PDN contexts exist for UE " MME_UE_S1AP_ID_FMT". Continuing with detach procedure. \n", emm_context->ue_id);
//    /** If a specific detach procedure is running, check if it is switch, off, if so, send detach accept back. */
//    if(detach_proc){
//      if (!detach_proc->ies->switch_off) {
//        /*
//         * Normal detach without UE switch-off
//         */
//        emm_sap_t                               emm_sap = {0};
//        emm_as_data_t                          *emm_as = &emm_sap.u.emm_as.u.data;
//
//        MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMM_AS_NAS_INFO_DETACH ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
//        /*
//         * Setup NAS information message to transfer
//         */
//        emm_as->nas_info = EMM_AS_NAS_INFO_DETACH;
//        emm_as->nas_msg = NULL;
//        /*
//         * Set the UE identifier
//         */
//        emm_as->ue_id = emm_context->ue_id;
//        /*
//         * Setup EPS NAS security data
//         */
//        emm_as_set_security_data (&emm_as->sctx, &emm_context->_security, false, true);
//        /*
//         * Notify EMM-AS SAP that Detach Accept message has to
//         * be sent to the network
//         */
//        emm_sap.primitive = EMMAS_DATA_REQ;
//        rc = emm_sap_send (&emm_sap);
//      }
//    }
//    /** Detach the UE. */
//    if (rc != RETURNerror) {
//
//      mme_ue_s1ap_id_t old_ue_id = emm_context->ue_id;
//
//      emm_sap_t                               emm_sap = {0};
//
//      /*
//       * Notify EMM FSM that the UE has been implicitly detached
//       */
//      MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0, "0 EMMREG_DETACH_CNF ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
//      emm_sap.primitive = EMMREG_DETACH_CNF;
//      emm_sap.u.emm_reg.ue_id = emm_context->ue_id;
//      emm_sap.u.emm_reg.ctx = emm_context;
//      emm_sap.u.emm_reg.free_proc = true;
//      rc = emm_sap_send (&emm_sap);
//      // Notify MME APP to remove the remaining MME_APP and S1AP contexts..
//      nas_itti_detach_req(old_ue_id);
//      // todo: review unlock
////      unlock_ue_contexts(ue_context);
//      OAILOG_FUNC_RETURN(LOG_NAS_ESM, rc);
//
//    } else{
//      // todo:
//      DevAssert(0);
//    }
//  }else{
//// todo:   OAILOG_INFO (LOG_NAS_ESM, "%d more PDN contexts exist for UE " MME_UE_S1AP_ID_FMT". Continuing with PDN Disconnect procedure. \n", emm_context->esm_ctx.n_pdns, emm_context->ue_id);
//    /*
//     * Send Disconnect Request to the ESM layer.
//     * It will disconnect the first PDN which is pending deactivation.
//     * This can be called at detach procedure with multiple PDNs, too.
//     *
//     * If detach procedure, or MME initiated PDN context deactivation procedure, we will first locally remove the PDN context.
//     * The remaining PDN contexts will not be those who need NAS messaging to be purged, but the ones, who were not purged locally at all.
//     */
//    if(!pdn_local_delete){
//      esm_sap.primitive = ESM_PDN_DISCONNECT_CNF;
//      esm_sap.ue_id = emm_context->ue_id;
//      esm_sap.ctx = emm_context;
//      esm_sap.recv = NULL;
//      esm_sap.data.pdn_disconnect.local_delete = pdn_local_delete; // pdn_context->default_ebi; /**< Default Bearer Id of default APN. */
//      esm_sap_send(&esm_sap);
//      OAILOG_FUNC_RETURN(LOG_NAS_ESM, rc);
//    }else{
//      OAILOG_INFO (LOG_NAS_ESM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Non Standalone & local detach triggered PDN deactivation. \n", emm_context->ue_id);
//      esm_sap_t                               esm_sap = {0};
//      /** Send Disconnect Request to all PDNs. */
//      pdn_context_t * pdn_context = RB_MIN(PdnContexts, &ue_context->pdn_contexts);
//
//      esm_sap.primitive = ESM_PDN_DISCONNECT_REQ;
//      esm_sap.ue_id = emm_context->ue_id;
//      esm_sap.ctx = emm_context;
////   todo:   esm_sap.recv = emm_context->esm_msg;
//      esm_sap.data.pdn_disconnect.default_ebi = pdn_context->default_ebi; /**< Default Bearer Id of default APN. */
//      esm_sap.data.pdn_disconnect.cid         = pdn_context->context_identifier; /**< Context Identifier of default APN. */
//      esm_sap.data.pdn_disconnect.local_delete = true;                            /**< Local deletion of PDN contexts. */
//      esm_sap_send(&esm_sap);
//      /*
//       * Remove the contexts and send S11 Delete Session Request to the SAE-GW.
//       * Will continue with the detach procedure when S11 Delete Session Response arrives.
//       */
//
//      /**
//       * Not waiting for a response. Will assume that the session is correctly purged.. Continuing with the detach and assuming that the SAE-GW session is purged.
//       * Assuming, that in 5G AMF/SMF structure, it is like in PCRF, sending DELETE_SESSION_REQUEST and not caring about the response. Assuming the session is deactivated.
//       */
//      OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
//    }
//  }
//  OAILOG_FUNC_RETURN (LOG_NAS_ESM, rc);
//}


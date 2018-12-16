/*
 * mme_app_todo.c
 *
 *  Created on: Dec 4, 2018
 *      Author: root
 */
static int mme_app_reset_enb_fteids(ue_context_t * ue_context){
  OAILOG_FUNC_IN(LOG_MME_APP);

  // todo: go through all bearers of all pdn contexts and reset the enb fteid..
  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
}

// todo: do this with macros, such that it is always locked..
int
mme_app_modify_bearers(const mme_ue_s1ap_id_t mme_ue_s1ap_id, bearer_contexts_to_be_modified_t *bcs_to_be_modified){
  int rc = RETURNerror;

  OAILOG_FUNC_IN(LOG_MME_APP);

  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id);
  if(!ue_context){
    OAILOG_INFO(LOG_MME_APP, "No UE context is found" MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  mme_app_reset_enb_fteids(ue_context);
  // todo: checking on procedures of the function.. mme_app_is_ue_context_clean(ue_context)?!?
  LOCK_UE_CONTEXT(ue_context);
  /** Get the PDN Context. */
  for(int nb_bearer = 0; bcs_to_be_modified->num_bearer_context; nb_bearer++) {
    bearer_context_to_be_modified_t *bc_to_be_modified = &bcs_to_be_modified->bearer_contexts[nb_bearer];
    /** Get the bearer context. */
    bearer_context_t * bearer_context = NULL;
    mme_app_get_session_bearer_context_from_all(ue_context, bc_to_be_modified->eps_bearer_id, &bearer_context);
    DevAssert(bearer_context); // todo: remove this from results of the tests..
    /** Set to inactive. */
    bearer_context->bearer_state &= (~BEARER_STATE_ACTIVE);
    /** Update the FTEID of the bearer context and uncheck the established state. */
    bearer_context->enb_fteid_s1u.teid = bc_to_be_modified->s1_eNB_fteid.teid;
    bearer_context->enb_fteid_s1u.interface_type      = S1_U_ENODEB_GTP_U;
    /** Set the IP address from the FTEID. */
    if (bc_to_be_modified->s1_eNB_fteid.ipv4) {
      bearer_context->enb_fteid_s1u.ipv4 = 1;
      bearer_context->enb_fteid_s1u.ipv4_address.s_addr, bc_to_be_modified->s1_eNB_fteid.ipv4_address;
    }
    if (bc_to_be_modified->s1_eNB_fteid.ipv6) {
      bearer_context->enb_fteid_s1u.ipv6 = 1;
      memcpy(&bearer_context->enb_fteid_s1u.ipv6_address, &bc_to_be_modified->s1_eNB_fteid.ipv6_address, sizeof(bc_to_be_modified->s1_eNB_fteid.ipv6_address));
    }
    bearer_context->bearer_state |= BEARER_STATE_ENB_CREATED;
    bearer_context->bearer_state |= BEARER_STATE_MME_CREATED; // todo: remove this flag.. unnecessary
  }
  UNLOCK_UE_CONTEXT(ue_context);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}


// TODO: EMM ---> PDN CONTEXT UPDATE PROCEDURE!!
//mme_app_get_pdn_context(ue_context, apn_config->context_identifier, ESM_EBI_UNASSIGNED, apn, &pdn_context);
//  if(pdn_context){
//    OAILOG_INFO(LOG_NAS_EMM, "EMMCN-SAP  - " "PDN context was found for UE " MME_UE_S1AP_ID_FMT" already. "
//        "(Assuming PDN connectivity is already established before ULA). "
//        "Will update PDN/UE context information and continue with the accept procedure for id " MME_UE_S1AP_ID_FMT "...\n", ue_context->mme_ue_s1ap_id);
//    /** Check the context id of the PDN context. Set it to the correct one. */
//    if(pdn_context->context_identifier >= PDN_CONTEXT_IDENTIFIER_UNASSIGNED){
//      pdn_context_t *pdn_context_removed = RB_REMOVE(PdnContexts, &ue_context->pdn_contexts, pdn_context);
//      if(!pdn_context_removed){
//        OAILOG_ERROR(LOG_MME_APP,  "Could not find pdn context with pid %d for ue_id " MME_UE_S1AP_ID_FMT "! \n",
//            pdn_context->context_identifier, ue_context->mme_ue_s1ap_id);
//        OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
//      }
//      pdn_context->context_identifier = apn_config->context_identifier;
//      /** Set the context also to all the bearers. */
//      RB_FOREACH (bearer_context, SessionBearers, &pdn_context->session_bearers) {
//        // todo: better error handling
//        bearer_context->pdn_cx_id = apn_config->context_identifier;
//      }
//      DevAssert(!RB_INSERT (PdnContexts, &ue_context->pdn_contexts, pdn_context));
//
//    }
///** Set the state of the ESM bearer context as ACTIVE (not setting as active if no TAU has followed). */
//    rc = esm_ebr_set_status (esm_context, pdn_context->default_ebi, ESM_EBR_ACTIVE, false);
//    /** Set the context identifier when updating the pdn_context. */
//    OAILOG_INFO(LOG_NAS_EMM, "EMMCN-SAP  - " "Successfully updated PDN context for UE " MME_UE_S1AP_ID_FMT" which was established already. \n", ue_context->mme_ue_s1ap_id);
//    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);


//
//if (rc != RETURNerror) {
//  /*
//   * Create local default EPS bearer context
//   */
//  if ((!is_pdn_connectivity) || ((is_pdn_connectivity) /*&& (EPS_BEARER_IDENTITY_UNASSIGNED == pdn_context->default_ebi) */)) {
//    rc = esm_proc_default_eps_bearer_context (esm_context, esm_context->esm_proc_data->pti, pdn_context, esm_context->esm_proc_data->apn, &new_ebi, &esm_context->esm_proc_data->bearer_qos, esm_cause);


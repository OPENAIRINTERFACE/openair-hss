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
  Source      LowerLayer.c

  Version     0.1

  Date        2012/03/14

  Product     NAS stack

  Subsystem   EPS Mobility Management

  Author      Frederic Maurel, Lionel GAUTHIER

  Description Defines EMM procedures executed by the Non-Access Stratum
        upon receiving notifications from lower layers so that data
        transfer succeed or failed, or NAS signalling connection is
        released, or ESM unit data has been received from under layer,
        and to request ESM unit data transfer to under layer.

*****************************************************************************/

#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "log.h"
#include "msc.h"
#include "gcc_diag.h"
#include "common_defs.h"
#include "common_defs.h"
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"

#include "emm_data.h"
#include "emm_sap.h"
#include "esm_sap.h"

#include "mme_app_defs.h"
#include "LowerLayer.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
            Lower layer notification handlers
   --------------------------------------------------------------------------
*/

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_success()                                      **
 **                                                                        **
 ** Description: Notify the EPS Mobility Management entity that data have  **
 **      been successfully delivered to the network                **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_success (mme_ue_s1ap_id_t ue_id, bstring *nas_msg)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_sap_t                               emm_sap = {0};
  int                                     rc = RETURNok;

  emm_sap.primitive = EMMREG_LOWERLAYER_SUCCESS;
  emm_sap.u.emm_reg.ue_id = ue_id;
  emm_sap.u.emm_reg.ctx = NULL;
  emm_data_context_t                     *emm_data_context = emm_data_context_get(&_emm_data, ue_id);

  if (emm_data_context) {
    emm_sap.u.emm_reg.ctx = emm_data_context;
    emm_sap.u.emm_reg.notify = true;
    emm_sap.u.emm_reg.free_proc = false;

    if (*nas_msg) {
      emm_sap.u.emm_reg.u.ll_success.msg_len = blength(*nas_msg);
      emm_sap.u.emm_reg.u.ll_success.digest_len = EMM_REG_MSG_DIGEST_SIZE;
      nas_digest_msg((const unsigned char * const)bdata(*nas_msg), blength(*nas_msg),
          (char * const)emm_sap.u.emm_reg.u.ll_success.msg_digest, &emm_sap.u.emm_reg.u.ll_success.digest_len);
    }
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_MME, NULL, 0, "EMMREG_LOWERLAYER_SUCCESS ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    rc = emm_sap_send (&emm_sap);
//    unlock_ue_contexts(ue_context);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  } else {
    OAILOG_INFO (LOG_NAS_EMM, "Unknown ue id " MME_UE_S1AP_ID_FMT "\n",ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }
}

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_failure()                                      **
 **                                                                        **
 ** Description: Notify the EPS Mobility Management entity that lower la-  **
 **      yers failed to deliver data to the network                **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_failure (mme_ue_s1ap_id_t ue_id, STOLEN_REF bstring *nas_msg)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_sap_t                               emm_sap = {0};
  int                                     rc = RETURNok;

  emm_sap.primitive = EMMREG_LOWERLAYER_FAILURE;
  emm_sap.u.emm_reg.ue_id = ue_id;
  emm_data_context_t                     *emm_data_context = emm_data_context_get(&_emm_data, ue_id);

  if (emm_data_context) {
    emm_sap.u.emm_reg.ctx = emm_data_context;
    emm_sap.u.emm_reg.notify    = true;
    emm_sap.u.emm_reg.free_proc = false;

    if (*nas_msg) {
      emm_sap.u.emm_reg.u.ll_failure.msg_len = blength(*nas_msg);
      emm_sap.u.emm_reg.u.ll_failure.digest_len = EMM_REG_MSG_DIGEST_SIZE;
      nas_digest_msg((const unsigned char * const)bdata(*nas_msg), blength(*nas_msg),
          (char * const)emm_sap.u.emm_reg.u.ll_failure.msg_digest, &emm_sap.u.emm_reg.u.ll_failure.digest_len);
    }
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_MME, NULL, 0, "EMMREG_LOWERLAYER_FAILURE ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    rc = emm_sap_send (&emm_sap);
//    unlock_ue_contexts(ue_context);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  } else {
    OAILOG_INFO (LOG_NAS_EMM, "Unknown ue id " MME_UE_S1AP_ID_FMT "\n",ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }
}

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_non_delivery_indication()                          **
 **                                                                        **
 ** Description: Notify the EPS Mobility Management entity that lower la-  **
 **      yers failed to deliver data to the network                        **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                         **
 **      Others:    None                                                   **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                                  **
 **      Others:    None                                                   **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_non_delivery_indication (mme_ue_s1ap_id_t ue_id, STOLEN_REF bstring *nas_msg)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_sap_t                               emm_sap = {0};
  int                                     rc = RETURNok;

  emm_sap.primitive = EMMREG_LOWERLAYER_NON_DELIVERY;
  emm_sap.u.emm_reg.ue_id = ue_id;
  emm_data_context_t                     *emm_data_context = emm_data_context_get(&_emm_data, ue_id);

  if (emm_data_context) {
    emm_sap.u.emm_reg.ctx = emm_data_context;
    if (*nas_msg) {
      emm_sap.u.emm_reg.u.non_delivery_ho.msg_len = blength(*nas_msg);
      emm_sap.u.emm_reg.u.non_delivery_ho.digest_len = EMM_REG_MSG_DIGEST_SIZE;
      nas_digest_msg((const unsigned char * const)bdata(*nas_msg), blength(*nas_msg),
          (char * const)emm_sap.u.emm_reg.u.non_delivery_ho.msg_digest, &emm_sap.u.emm_reg.u.non_delivery_ho.digest_len);
    }
    MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_MME, NULL, 0, "EMMREG_LOWERLAYER_NON_DELIVERY ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    rc = emm_sap_send (&emm_sap);
//    unlock_ue_contexts(ue_context);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  } else {
    OAILOG_INFO (LOG_NAS_EMM, "Unknown ue id " MME_UE_S1AP_ID_FMT "\n",ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }
}

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_establish()                                    **
 **                                                                        **
 ** Description: Update the EPS connection management status upon recei-   **
 **      ving indication so that the NAS signalling connection is  **
 **      established                                               **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_establish (void)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_release()                                      **
 **                                                                        **
 ** Description: Notify the EPS Mobility Management entity that NAS signal-**
 **      ling connection has been released                         **
 **                                                                        **
 ** Inputs:  cause:     Release cause                              **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int lowerlayer_release (mme_ue_s1ap_id_t ue_id, int cause)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  emm_sap_t                               emm_sap = {0};
  int                                     rc = RETURNok;

  emm_sap.primitive = EMMREG_LOWERLAYER_RELEASE;
  emm_sap.u.emm_reg.ue_id = 0;
  emm_data_context_t                     *emm_data_context = emm_data_context_get(&_emm_data, ue_id);
  if (emm_data_context) {
    emm_sap.u.emm_reg.ctx = emm_data_context;
  } else {
    OAILOG_INFO (LOG_NAS_EMM, "Unknown ue id " MME_UE_S1AP_ID_FMT "\n",ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  }
  MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_MME, NULL, 0, "EMMREG_LOWERLAYER_RELEASE ue id " MME_UE_S1AP_ID_FMT " ", emm_sap.u.emm_reg.ue_id);
  rc = emm_sap_send (&emm_sap);
//  unlock_ue_contexts(ue_context);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

///****************************************************************************
// **                                                                        **
// ** Name:    lowerlayer_data_ind()                                     **
// **                                                                        **
// ** Description: Notify the EPS Session Management entity that data have   **
// **      been received from lower layers                           **
// **                                                                        **
// ** Inputs:  ue_id:      UE lower layer identifier                  **
// **      data:      Data transfered from lower layers          **
// **      Others:    None                                       **
// **                                                                        **
// ** Outputs:     None                                                      **
// **      Return:    RETURNok, RETURNerror                      **
// **      Others:    None                                       **
// **                                                                        **
// ***************************************************************************/
//int lowerlayer_data_ind (mme_ue_s1ap_id_t ue_id, const_bstring    data)
//{
//  esm_sap_t                               esm_sap = {0};
//  int                                     rc = RETURNok;
//
//  OAILOG_FUNC_IN (LOG_NAS_EMM);
//
//  emm_data_context_t                     *emm_data_context = emm_data_context_get(&_emm_data, ue_id);
//  esm_sap.primitive = ESM_UNITDATA_IND;
//  esm_sap.ue_id = ue_id;
//  esm_sap.ctx = emm_data_context;
//  esm_sap.recv = data;
//  data = NULL;
//  rc = esm_sap_send (&esm_sap);
////  unlock_ue_contexts(ue_context);
//  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
//}

/****************************************************************************
 **                                                                        **
 ** Name:    lowerlayer_data_req()                                     **
 **                                                                        **
 ** Description: Notify the EPS Mobility Management entity that data have  **
 **      to be transfered to lower layers                          **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **          data:      Data to be transfered to lower layers      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/

int lowerlayer_data_req (mme_ue_s1ap_id_t ue_id, bstring data)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNok;
  emm_sap_t                               emm_sap = {0};
  emm_security_context_t                 *sctx = NULL;
  emm_data_context_t                     *emm_data_context = emm_data_context_get(&_emm_data, ue_id);

  emm_sap.primitive = EMMAS_DATA_REQ;
  emm_sap.u.emm_as.u.data.guti = NULL;
  emm_sap.u.emm_as.u.data.ue_id = ue_id;


  if (emm_data_context) {
    sctx = &emm_data_context->_security;
  }

  emm_sap.u.emm_as.u.data.nas_info = 0;
  emm_sap.u.emm_as.u.data.nas_msg = data;
  /*
   * Setup EPS NAS security data
   */
  emm_as_set_security_data (&emm_sap.u.emm_as.u.data.sctx, sctx, false, true);
  MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_MME, NULL, 0, "EMMAS_DATA_REQ  (STATUS) ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
  rc = emm_sap_send (&emm_sap);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
int lowerlayer_activate_bearer_req (
    const mme_ue_s1ap_id_t ue_id,
    const ebi_t            ebi,
    const bitrate_t        mbr_dl,
    const bitrate_t        mbr_ul,
    const bitrate_t        gbr_dl,
    const bitrate_t        gbr_ul,
    bstring data)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNok;
  emm_sap_t                               emm_sap = {0};
  emm_security_context_t                 *sctx = NULL;
  emm_data_context_t                     *emm_data_context = emm_data_context_get(&_emm_data, ue_id);

  if (!emm_data_context) {
    // todo: check no (implicit) detach procedure is ongoing
    OAILOG_ERROR(LOG_NAS_EMM, "EMM context not existing UE " MME_UE_S1AP_ID_FMT ". "
        "Aborting the dedicated bearer activation. \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  } else if (emm_data_context->_emm_fsm_state != EMM_REGISTERED){
	  /** Check if a TAU procedure is ongoing. Set the pending flag for pending QoS. */
	  OAILOG_ERROR(LOG_NAS_EMM, "EMM context not in EMM_REGISTERED state for UE " MME_UE_S1AP_ID_FMT ". "
			  "Cannot activate bearers. \n", ue_id);
	  /** Check for an EMM (idle TAU) or S10 handover procedure. */
	  nas_emm_tau_proc_t * nas_proc_tau = get_nas_specific_procedure_tau(emm_data_context);
	  if(nas_proc_tau) {
		  OAILOG_INFO(LOG_NAS_EMM, "A TAU procedure is running for UE " MME_UE_S1AP_ID_FMT ". Triggering pending qos. \n", ue_id);
		  /** Only do this for DEREGISTERED state. */
		  if(emm_data_context->_emm_fsm_state != EMM_COMMON_PROCEDURE_INITIATED){
			  nas_proc_tau->pending_qos = true;
			  /** This may be an idle-TAU, so set the active flag (todo: here check glitches). */
			  if(!nas_proc_tau->ies->eps_update_type.active_flag) {
				  OAILOG_INFO(LOG_NAS_EMM, "Setting the active flag for TAU procedure for UE " MME_UE_S1AP_ID_FMT ". \n", ue_id);
				  nas_proc_tau->ies->eps_update_type.active_flag = true;
			  }
		  } else {
			  OAILOG_INFO(LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " is not in DEREGISTERED state, to late to set qos/active flag.. will do via paging. \n", ue_id);
		  }
	  } else {
		  OAILOG_INFO(LOG_NAS_EMM, "No S10-TAU procedure is running for UE " MME_UE_S1AP_ID_FMT ", which is not in registered state. "
				  "Not triggering any pending qos at this moment. \n", ue_id);
	  }
	  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
  }

  sctx = &emm_data_context->_security;
  emm_sap.primitive = EMMAS_ERAB_SETUP_REQ;
  emm_sap.u.emm_as.u.activate_bearer_context_req.ebi    = ebi;
  emm_sap.u.emm_as.u.activate_bearer_context_req.ue_id  = ue_id;
  emm_sap.u.emm_as.u.activate_bearer_context_req.mbr_dl = mbr_dl;
  emm_sap.u.emm_as.u.activate_bearer_context_req.mbr_ul = mbr_ul;
  emm_sap.u.emm_as.u.activate_bearer_context_req.gbr_dl = gbr_dl;
  emm_sap.u.emm_as.u.activate_bearer_context_req.gbr_ul = gbr_ul;
  emm_sap.u.emm_as.u.activate_bearer_context_req.nas_msg = data;
  /*
   * Setup EPS NAS security data
   */
  emm_as_set_security_data (&emm_sap.u.emm_as.u.activate_bearer_context_req.sctx, sctx, false, true);
  MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_MME, NULL, 0, "EMMAS_ERAB_SETUP_REQ  (STATUS) ue id " MME_UE_S1AP_ID_FMT " ebi %u gbr_dl %" PRIu64 " gbr_ul %" PRIu64 " ",
      ue_id, ebi, emm_sap.u.emm_as.u.activate_bearer_context_req.gbr_dl, emm_sap.u.emm_as.u.activate_bearer_context_req.gbr_ul);
  rc = emm_sap_send (&emm_sap);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
int lowerlayer_modify_bearer_req (
    const mme_ue_s1ap_id_t ue_id,
    const ebi_t            ebi,
    const bitrate_t        mbr_dl,
    const bitrate_t        mbr_ul,
    const bitrate_t        gbr_dl,
    const bitrate_t        gbr_ul,
    bstring data)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNok;
  emm_sap_t                               emm_sap = {0};
  emm_security_context_t                 *sctx = NULL;
  emm_data_context_t                     *emm_data_context = emm_data_context_get(&_emm_data, ue_id);


  if (!emm_data_context) {
    // todo: check no (implicit) detach procedure is ongoing
    OAILOG_ERROR(LOG_NAS_EMM, "EMM context not existing UE " MME_UE_S1AP_ID_FMT ". "
        "Aborting the dedicated bearer activation. \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  } else if (emm_data_context->_emm_fsm_state != EMM_REGISTERED){
	  /** Check if a TAU procedure is ongoing. Set the pending flag for pending QoS. */
	  OAILOG_ERROR(LOG_NAS_EMM, "EMM context not in EMM_REGISTERED state for UE " MME_UE_S1AP_ID_FMT ". "
				"Cannot modify bearers. \n", ue_id);
	  /** Only do this for DEREGISTERED state. */
	  /** Check for an EMM (idle TAU) or S10 handover procedure. */
	  nas_emm_tau_proc_t * nas_proc_tau = get_nas_specific_procedure_tau(emm_data_context);
	  if(nas_proc_tau) {
		  if(emm_data_context->_emm_fsm_state != EMM_COMMON_PROCEDURE_INITIATED){
			  nas_proc_tau->pending_qos = true;
			  /** This may be an idle-TAU, so set the active flag (todo: here check glitches). */
			  if(!nas_proc_tau->ies->eps_update_type.active_flag) {
				  OAILOG_INFO(LOG_NAS_EMM, "Setting the active flag for TAU procedure for UE " MME_UE_S1AP_ID_FMT ". \n", ue_id);
				  nas_proc_tau->ies->eps_update_type.active_flag = true;
			  }
		  } else {
			  OAILOG_INFO(LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " is not in DEREGISTERED state, to late to set qos/active flag.. will do via paging. \n", ue_id);
		  }
	  } else {
		  OAILOG_INFO(LOG_NAS_EMM, "No S10-TAU procedure is running for UE " MME_UE_S1AP_ID_FMT ", which is not in registered state. "
				  "Not triggering any pending qos at this moment. \n", ue_id);
	  }
	  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
  }

  if(data){
	  emm_sap.primitive = EMMAS_ERAB_MODIFY_REQ;
	  emm_sap.u.emm_as.u.modify_bearer_context_req.ebi    = ebi;
	  emm_sap.u.emm_as.u.modify_bearer_context_req.ue_id  = ue_id;
	  emm_sap.u.emm_as.u.modify_bearer_context_req.mbr_dl = mbr_dl;
	  emm_sap.u.emm_as.u.modify_bearer_context_req.mbr_ul = mbr_ul;
	  emm_sap.u.emm_as.u.modify_bearer_context_req.gbr_dl = gbr_dl;
	  emm_sap.u.emm_as.u.modify_bearer_context_req.gbr_ul = gbr_ul;

	  sctx = &emm_data_context->_security;
	  emm_sap.u.emm_as.u.modify_bearer_context_req.nas_msg = data;
	  /*
	   * Setup EPS NAS security data
	   */
	  emm_as_set_security_data (&emm_sap.u.emm_as.u.modify_bearer_context_req.sctx, sctx, false, true);
	  MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_MME, NULL, 0, "EMMAS_ERAB_MODIFY_REQ  (STATUS) ue id " MME_UE_S1AP_ID_FMT " ebi %u gbr_dl %" PRIu64 " gbr_ul %" PRIu64 " ",
	      ue_id, ebi, emm_sap.u.emm_as.u.modify_bearer_context_req.gbr_dl, emm_sap.u.emm_as.u.modify_bearer_context_req.gbr_ul);
	  rc = emm_sap_send (&emm_sap);
	  //  unlock_ue_contexts(ue_context);
	  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
  }
  OAILOG_WARNING(LOG_NAS_EMM, "No data exists ERAB-Modify for UE " MME_UE_S1AP_ID_FMT ". Returning. \n", ue_id);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
}

//------------------------------------------------------------------------------
int lowerlayer_deactivate_bearer_req (
    const mme_ue_s1ap_id_t ue_id,
    const ebi_t            ebi,
    bstring data)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNok;
  emm_sap_t                               emm_sap = {0};
  emm_security_context_t                 *sctx = NULL;
  emm_data_context_t                     *emm_data_context = emm_data_context_get(&_emm_data, ue_id);


  if (!emm_data_context) {
    // todo: check no (implicit) detach procedure is ongoing
    OAILOG_ERROR(LOG_NAS_EMM, "EMM context not existing UE " MME_UE_S1AP_ID_FMT ". "
        "Aborting the dedicated bearer activation. \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNerror);
  } else if (!emm_data_context->is_has_been_attached){
	  /** Check if a TAU procedure is ongoing. Set the pending flag for pending QoS. */
	  OAILOG_ERROR(LOG_NAS_EMM, "EMM context not in EMM_REGISTERED state for UE " MME_UE_S1AP_ID_FMT ". "
			  "Cannot delete bearers. \n", ue_id);
	  /** Check for an EMM (idle TAU) or S10 handover procedure. */
	  nas_emm_tau_proc_t * nas_proc_tau = get_nas_specific_procedure_tau(emm_data_context);
	  if(nas_proc_tau) {
		  if(emm_data_context->_emm_fsm_state != EMM_COMMON_PROCEDURE_INITIATED){
			  nas_proc_tau->pending_qos = true;
			  /** This may be an idle-TAU, so set the active flag (todo: here check glitches). */
			  if(!nas_proc_tau->ies->eps_update_type.active_flag) {
				  OAILOG_INFO(LOG_NAS_EMM, "Setting the active flag for TAU procedure for UE " MME_UE_S1AP_ID_FMT ". \n", ue_id);
				  nas_proc_tau->ies->eps_update_type.active_flag = true;
			  }
		  } else {
			  OAILOG_INFO(LOG_NAS_EMM, "UE " MME_UE_S1AP_ID_FMT " is not in DEREGISTERED state, to late to set qos/active flag.. will do via paging. \n", ue_id);
		  }
	  } else {
		  OAILOG_INFO(LOG_NAS_EMM, "No S10-TAU procedure is running for UE " MME_UE_S1AP_ID_FMT ", which is not in registered state. "
				  "Not triggering any pending qos at this moment. \n", ue_id);
	  }
	  OAILOG_FUNC_RETURN (LOG_NAS_EMM, RETURNok);
  }

  if(data){
	emm_sap.primitive = EMMAS_ERAB_RELEASE_REQ;
	emm_sap.u.emm_as.u.deactivate_bearer_context_req.ebi    = ebi;
	emm_sap.u.emm_as.u.deactivate_bearer_context_req.ue_id  = ue_id;
	sctx = &emm_data_context->_security;
	emm_sap.u.emm_as.u.deactivate_bearer_context_req.nas_msg = data;
	/*
	 * Setup EPS NAS security data
	 */
	emm_as_set_security_data (&emm_sap.u.emm_as.u.deactivate_bearer_context_req.sctx, sctx, false, true);
	MSC_LOG_TX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_MME, NULL, 0, "EMMAS_ERAB_RELEASE_REQ  (STATUS) ue id " MME_UE_S1AP_ID_FMT " ebi %u ",
			ue_id, ebi);
	rc = emm_sap_send (&emm_sap);
	//  unlock_ue_contexts(ue_context);
  }
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}

/*
   --------------------------------------------------------------------------
                EMM procedure handlers
   --------------------------------------------------------------------------
*/

/****************************************************************************
 **                                                                        **
 ** Name:    emm_as_set_security_data()                                    **
 **                                                                        **
 ** Description: Setup security data according to the given EPS security   **
 **      context when data transfer to lower layers is requested   **
 **                                                                        **
 ** Inputs:  args:      EPS security context currently in use      **
 **      is_new:    Indicates whether a new security context   **
 **             has just been taken into use               **
 **      is_ciphered:   Indicates whether the NAS message has to   **
 **             be sent ciphered                           **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     data:      EPS NAS security data to be setup          **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void emm_as_set_security_data (
  emm_as_security_data_t * data,
  const void *args,
  bool is_new,
  bool is_ciphered)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  const emm_security_context_t           *context = (emm_security_context_t *) (args);

  memset (data, 0, sizeof (emm_as_security_data_t));

  if (context && ((context->sc_type == SECURITY_CTX_TYPE_FULL_NATIVE) || (context->sc_type == SECURITY_CTX_TYPE_MAPPED))) {
    /*
     * 3GPP TS 24.301, sections 5.4.3.3 and 5.4.3.4
     * * * * Once a valid EPS security context exists and has been taken
     * * * * into use, UE and MME shall cipher and integrity protect all
     * * * * NAS signalling messages with the selected NAS ciphering and
     * * * * NAS integrity algorithms
     */
    OAILOG_INFO (LOG_NAS_EMM, "EPS security context exists is new %u KSI %u SQN %u count %u\n",
        is_new, context->eksi, context->ul_count.seq_num, *(uint32_t *) (&context->ul_count));
    OAILOG_STREAM_HEX (OAILOG_LEVEL_DEBUG, LOG_NAS_EMM, "knas_int:", context->knas_int, AUTH_KNAS_INT_SIZE);
    OAILOG_STREAM_HEX (OAILOG_LEVEL_DEBUG, LOG_NAS_EMM, "knas_enc:", context->knas_enc, AUTH_KNAS_ENC_SIZE);
    data->is_new = is_new;
    data->ksi = context->eksi;
    data->sqn = context->dl_count.seq_num; //TODO AT check whether we need to increment this by one or it is already incremented after sending last NAS mssage. 
    data->count = 0x00000000 | ((context->dl_count.overflow & 0x0000FFFF) << 8) | (context->dl_count.seq_num & 0x000000FF);
    /*
     * NAS integrity and cyphering keys may not be available if the
     * * * * current security context is a partial EPS security context
     * * * * and not a full native EPS security context
     */
    data->is_knas_int_present = true;
    memcpy(data->knas_int, context->knas_int, sizeof(data->knas_int));

    if (is_ciphered) {
      /*
       * 3GPP TS 24.301, sections 4.4.5
       * * * * When the UE establishes a new NAS signalling connection,
       * * * * it shall send initial NAS messages integrity protected
       * * * * and unciphered
       */
      /*
       * 3GPP TS 24.301, section 5.4.3.2
       * * * * The MME shall send the SECURITY MODE COMMAND message integrity
       * * * * protected and unciphered
       */
      OAILOG_DEBUG (LOG_NAS_EMM, "EPS security context exists knas_enc\n");
      data->is_knas_enc_present = true;
      memcpy(data->knas_enc, context->knas_enc, sizeof(data->knas_enc));
    }
  } else {
    OAILOG_DEBUG (LOG_NAS_EMM, "NO Valid Security Context Available\n");
    /*
     * No valid EPS security context exists
     */
    data->ksi = KSI_NO_KEY_AVAILABLE;
  }

  OAILOG_FUNC_OUT (LOG_NAS_EMM);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

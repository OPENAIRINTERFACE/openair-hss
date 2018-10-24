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
  Source      esm_ebr_context.h

  Version     0.1

  Date        2013/05/28

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Frederic Maurel

  Description Defines functions used to handle EPS bearer contexts.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "obj_hashtable.h"
#include "log.h"
#include "msc.h"
#include "gcc_diag.h"
#include "dynamic_memory_check.h"
#include "assertions.h"
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "mme_app_ue_context.h"
#include "mme_app_bearer_context.h"
#include "common_defs.h"
#include "commonDef.h"
#include "emm_data.h"
#include "esm_ebr.h"
#include "esm_ebr_context.h"
#include "esm_sapDef.h"
#include "emm_sap.h"
#include "mme_app_defs.h"


/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_context_create()                                  **
 **                                                                        **
 ** Description: Creates a new EPS bearer context to the PDN with the spe- **
 **      cified PDN connection identifier                          **
 **                                                                        **
 ** Inputs:  ue_id:      UE identifier                              **
 **      pid:       PDN connection identifier                  **
 **      ebi:       EPS bearer identity                        **
 **      is_default:    true if the new bearer is a default EPS    **
 **             bearer context                             **
 **      esm_qos:   EPS bearer level QoS parameters            **
 **      tft:       Traffic flow template parameters           **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The EPS bearer identity of the default EPS **
 **             bearer associated to the new EPS bearer    **
 **             context if successfully created;           **
 **             UNASSIGN EPS bearer value otherwise.       **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ***************************************************************************/
ebi_t
esm_ebr_context_create (
  emm_data_context_t * emm_context,
  const proc_tid_t pti,
  void *pdn_context_v,
  ebi_t ebi,
  struct fteid_set_s * fteid_set,
  bool is_default,
  const bearer_qos_t *bearer_level_qos,
  traffic_flow_template_t * tft,
  protocol_configuration_options_t * pco)
{
  int                                     bidx = 0;
  esm_context_t                          *esm_ctx = NULL;
  esm_pdn_t                              *pdn = NULL;
  pdn_context_t                          *pdn_context = (pdn_context_t*)pdn_context_v;

  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_ctx = &emm_context->esm_ctx;
  ue_context_t                           *ue_context  = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);

  if(!pdn_context){
    OAILOG_ERROR(LOG_NAS_ESM , "ESM-PROC  - PDN connection has not been " "allocated for UE " MME_UE_S1AP_ID_FMT". \n", emm_context->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
  }

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Create new %s EPS bearer context (ebi=%d) " "for PDN connection (pid=%d)\n",
      (is_default) ? "default" : "dedicated", ebi, pdn_context->context_identifier);

  /** Get the PDN Session for the UE. */
  /*
   * Check the total number of active EPS bearers
   * No maximum number of bearers per UE.
   */
  if (esm_ctx->n_active_ebrs > BEARERS_PER_UE) {
    OAILOG_WARNING (LOG_NAS_ESM , "ESM-PROC  - The total number of active EPS" "bearers is exceeded\n");
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
  }
  /*
   * Check that the EBI is available (no already allocated).
   */

  /*
   * Get the PDN connection entry
   */
  pdn = &pdn_context->esm_data;

  /** Get the already registered bearer context. */
  bearer_context_t * bearer_context = mme_app_get_session_bearer_context(pdn_context, ebi);
  if(bearer_context){
    MSC_LOG_EVENT (MSC_NAS_ESM_MME, "0 Registered Bearer ebi %u cid %u pti %u", ebi, pdn_context->context_identifier, pti);
    bearer_context->transaction_identifier = pti;
    /*
     * Increment the total number of active EPS bearers
     */
    esm_ctx->n_active_ebrs += 1;
    /*
     * Increment the number of EPS bearer for this PDN connection
     */
    pdn->n_bearers += 1;
    /*
     * Setup the EPS bearer data with verified TFT and QoS.
     */
    bearer_context->qci = bearer_level_qos->qci;
    bearer_context->esm_ebr_context.gbr_dl = bearer_level_qos->gbr.br_dl;
    bearer_context->esm_ebr_context.gbr_ul = bearer_level_qos->gbr.br_ul;
    bearer_context->esm_ebr_context.mbr_dl = bearer_level_qos->mbr.br_dl;
    bearer_context->esm_ebr_context.mbr_ul = bearer_level_qos->mbr.br_ul;
    /** Set the priority values. */
    bearer_context->preemption_capability    = bearer_level_qos->pci == 0 ? 1 : 0;
    bearer_context->preemption_vulnerability = bearer_level_qos->pvi == 0 ? 1 : 0;
    bearer_context->priority_level           = bearer_level_qos->pl;
    if (bearer_context->esm_ebr_context.tft) {
      free_traffic_flow_template(&bearer_context->esm_ebr_context.tft);
    }
    /* TFT bitmaps (precedence, identifier) will be updated for CREATE_NEW_TFT @ validation. */
    if(tft){
      bearer_context->esm_ebr_context.tft = (traffic_flow_template_t *) calloc (1, sizeof (traffic_flow_template_t));
      memcpy(bearer_context->esm_ebr_context.tft, tft, sizeof (traffic_flow_template_t));  /**< Should have the processed bitmap in the validation . */
    }
    if(pco){
      if (bearer_context->esm_ebr_context.pco) {
        free_protocol_configuration_options(&bearer_context->esm_ebr_context.pco);
      }
      /** Make it with memcpy, don't use bearer.. */
      bearer_context->esm_ebr_context.pco = (protocol_configuration_options_t *) calloc (1, sizeof (protocol_configuration_options_t));
      memcpy(bearer_context->esm_ebr_context.pco, pco, sizeof (protocol_configuration_options_t));  /**< Should have the processed bitmap in the validation . */
    }

    /** We can set the FTEIDs right before the CBResp is set. */
    if(fteid_set){
      memcpy((void*)&bearer_context->s_gw_fteid_s1u , fteid_set->s1u_fteid, sizeof(fteid_t));
      memcpy((void*)&bearer_context->p_gw_fteid_s5_s8_up , fteid_set->s5_fteid, sizeof(fteid_t));
      bearer_context->bearer_state |= BEARER_STATE_SGW_CREATED;
    }
    /** Set the MME_APP states (todo: may be with Activate Dedicated Bearer Response). */
    bearer_context->bearer_state   |= BEARER_STATE_SGW_CREATED;
    bearer_context->bearer_state   |= BEARER_STATE_MME_CREATED;

    if (is_default) {
      /*
       * Set the PDN connection activation indicator
       */
      pdn_context->is_active = true;

      pdn_context->default_ebi = ebi;
      /*
       * Update the emergency bearer services indicator
       */
      if (pdn->is_emergency) {
        esm_ctx->is_emergency = true;
      }

    }
    /** Set the default ebi. */
    bearer_context->linked_ebi = pdn_context->default_ebi;

    /*
     * Return the EPS bearer identity of the default EPS bearer
     * * * * associated to the new EPS bearer context
     */
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, bearer_context->ebi);
  }
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_context_update()                                  **
 **                                                                        **
 ** Description: Updates an existing EPS session bearer context with
 **     new QoS and TFT values.                                            **
 **                                                                        **
 ** Inputs:  ue_id:      UE identifier                              **
 **      esm_qos:   EPS bearer level QoS parameters            **
 **      tft:       Traffic flow template parameters           **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The EPS bearer identity of the default EPS **
 **             bearer associated to the new EPS bearer    **
 **             context if successfully created;           **
 **             UNASSIGN EPS bearer value otherwise.       **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ***************************************************************************/
int
esm_ebr_context_update (
  const emm_data_context_t * emm_context,
  bearer_context_t * bearer_context,
  const bearer_qos_t *bearer_level_qos,
  traffic_flow_template_t * tft,
  protocol_configuration_options_t * pco)
{
  int                                     bidx = 0;
  esm_context_t                          *esm_ctx = NULL;
  esm_pdn_t                              *pdn = NULL;

  OAILOG_FUNC_IN (LOG_NAS_ESM);
  esm_ctx = &emm_context->esm_ctx;
  ue_context_t                           *ue_context  = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Updating EPS bearer context (ebi=%d) " "for UE with ueId " MME_UE_S1AP_ID_FMT ". \n", bearer_context->ebi, emm_context->ue_id);

  if(!bearer_context){
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - EPS bearer context (ebi=%d) could not be found for UE with ueId " MME_UE_S1AP_ID_FMT ". \n", bearer_context->ebi, emm_context->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
  }

  // todo: check why esm_proc_data is needed and does it needs to be updated?! only for UE triggered qos update?
  // todo: esm_ctx->esm_proc_data

  /** Update EPS level QoS. */
  if(bearer_level_qos){
    OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Updating EPS QoS for EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n", bearer_context->ebi, emm_context->ue_id);

    /** Update the QCI. */
    bearer_context->qci = bearer_level_qos->qci;

    /** Update the GBR and MBR values (if existing). */
    bearer_context->esm_ebr_context.gbr_dl = bearer_level_qos->gbr.br_dl;
    bearer_context->esm_ebr_context.gbr_ul = bearer_level_qos->gbr.br_ul;
    bearer_context->esm_ebr_context.mbr_dl = bearer_level_qos->mbr.br_dl;
    bearer_context->esm_ebr_context.mbr_ul = bearer_level_qos->mbr.br_ul;

    /** Set the priority values. */
    bearer_context->preemption_capability    = bearer_level_qos->pci == 0 ? 1 : 0;
    bearer_context->preemption_vulnerability = bearer_level_qos->pvi == 0 ? 1 : 0;
    bearer_context->priority_level           = bearer_level_qos->pl;
  }

  if(pco){
    if (bearer_context->esm_ebr_context.pco) {
      free_protocol_configuration_options(&bearer_context->esm_ebr_context.pco);
    }
    /** Make it with memcpy, don't use bearer.. */
    bearer_context->esm_ebr_context.pco = (protocol_configuration_options_t *) calloc (1, sizeof (protocol_configuration_options_t));
    memcpy(bearer_context->esm_ebr_context.pco, pco, sizeof (protocol_configuration_options_t));  /**< Should have the processed bitmap in the validation . */
  }

  /** Update TFT. */
  if(tft){
    OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Updating TFT for EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n", bearer_context->ebi, emm_context->ue_id);

    if(tft->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_ADD_PACKET_FILTER_TO_EXISTING_TFT){
      /**
       * todo: (6.1.3.3.4) check at the beginning that all the packet filters exist, before continuing (check syntactical errors).
       * & check that the bearer context in question has tft (and if delete, at least one TFT remains.
       */
      for(int num_pf = 0 ; num_pf < tft->numberofpacketfilters; num_pf++){
        /**
         * Assume that the identifier will be used as the ordinal.
         */
        packet_filter_t *new_packet_filter = NULL;
        int num_pf1 = 0;
        for(; num_pf1 < TRAFFIC_FLOW_TEMPLATE_NB_PACKET_FILTERS_MAX; num_pf1++) {
          /** Update the packet filter with the correct identifier. */
          if(bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1].length == 0){
            new_packet_filter = &bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1];
            break;
          }
        }
        DevAssert(new_packet_filter); /**< todo: make synctactical check before. */
        /** Clean up the packet filter. */
        memset((void*)new_packet_filter, 0, sizeof(*new_packet_filter));
        // todo: any variability in size?
        memcpy((void*)new_packet_filter, (void*)&tft->packetfilterlist.addpacketfilter[num_pf], sizeof(tft->packetfilterlist.addpacketfilter[num_pf]));
        OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Added new packet filter with packet filter id %d to EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n",
            tft->packetfilterlist.addpacketfilter[num_pf].identifier, bearer_context->ebi, emm_context->ue_id);
        bearer_context->esm_ebr_context.tft->numberofpacketfilters++;
        /** Update the flags. */
        bearer_context->esm_ebr_context.tft->packet_filter_identifier_bitmap |= tft->packet_filter_identifier_bitmap;
        for(int prec = 0; prec < sizeof(tft->precedence_set); prec ++){
          bearer_context->esm_ebr_context.tft->precedence_set[prec] = bearer_context->esm_ebr_context.tft->precedence_set[prec] + tft->precedence_set[prec];
        }
      }
      OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Successfully added %d new packet filters to EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". "
          "Current number of packet filters of bearer %d. \n", tft->numberofpacketfilters, bearer_context->ebi, emm_context->ue_id, bearer_context->esm_ebr_context.tft->numberofpacketfilters);
    } else if(tft->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_DELETE_PACKET_FILTERS_FROM_EXISTING_TFT){
      // todo: check that at least one TFT exists, if not signal error !
      /** todo: (6.1.3.3.4) check at the beginning that all the packet filters exist, before continuing (check syntactical errors). */
      for(int num_pf = 0 ; num_pf < tft->numberofpacketfilters; num_pf++){
        /**
         * Assume that the identifier will be used as the ordinal.
         */
        int num_pf1 = 0;
        for(; num_pf1 < bearer_context->esm_ebr_context.tft->numberofpacketfilters; num_pf1++) {
          /** Update the packet filter with the correct identifier. */
          if(bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1].identifier == tft->packetfilterlist.deletepacketfilter[num_pf].identifier)
            break;
        }
        DevAssert(num_pf1 < bearer_context->esm_ebr_context.tft->numberofpacketfilters); /**< todo: make synctactical check before. */
        /** Remove the packet filter. */
        OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Removed the packet filter with packet filter id %d of EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n",
            tft->packetfilterlist.deletepacketfilter[num_pf].identifier, bearer_context->ebi, emm_context->ue_id);
        memset((void*)&bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1], 0,
            sizeof(bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1]));
        bearer_context->esm_ebr_context.tft->numberofpacketfilters--;
      }
      OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Successfully deleted %d existing packet filters of EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". "
          "Current number of packet filters of bearer context %d. \n", tft->numberofpacketfilters, bearer_context->ebi, emm_context->ue_id, bearer_context->esm_ebr_context.tft->numberofpacketfilters);
      /** Update the flags. */
      bearer_context->esm_ebr_context.tft->packet_filter_identifier_bitmap &= ~tft->packet_filter_identifier_bitmap;
      for(int prec = 0; prec < sizeof(tft->packetfilterlist.replacepacketfilter); prec ++){
        for(int prec1 = 0; prec1 < sizeof(bearer_context->esm_ebr_context.tft->precedence_set); prec1 ++){
          if(bearer_context->esm_ebr_context.tft->precedence_set[prec1] == tft->packetfilterlist.replacepacketfilter[prec].identifier){
            bearer_context->esm_ebr_context.tft->precedence_set[prec1] = 0;
            bearer_context->esm_ebr_context.tft->precedence_set[tft->packetfilterlist.replacepacketfilter[prec].eval_precedence] = tft->packetfilterlist.replacepacketfilter[prec].identifier; /**< Remove the precedences. */
          }
        }
      }
    } else if(tft->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_REPLACE_PACKET_FILTERS_IN_EXISTING_TFT){
      /** todo: (6.1.3.3.4) check at the beginning that all the packet filters exist, before continuing (check syntactical errors). */
      for(int num_pf = 0 ; num_pf < tft->numberofpacketfilters; num_pf++){
        /**
         * Assume that the identifier will be used as the ordinal.
         */
        int num_pf1 = 0;
        for(; num_pf1 < bearer_context->esm_ebr_context.tft->numberofpacketfilters; num_pf1++) {
          /** Update the packet filter with the correct identifier. */
          if(bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1].identifier == tft->packetfilterlist.replacepacketfilter[num_pf].identifier)
            break;
        }
        DevAssert(num_pf1 < bearer_context->esm_ebr_context.tft->numberofpacketfilters); /**< todo: make synctactical check before. */
        /** Remove the packet filter. */
        memset((void*)&bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1], 0,
            sizeof(bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1]));
        OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Removed the packet filter with packet filter id %d of EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n",
            tft->packetfilterlist.replacepacketfilter[num_pf].identifier, bearer_context->ebi, emm_context->ue_id);
        // todo: use length?
        memcpy((void*)&bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1], &tft->packetfilterlist.replacepacketfilter[num_pf],
            sizeof(tft->packetfilterlist.replacepacketfilter[num_pf]));
      }
      OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Successfully replaced %d existing packet filters of EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". "
          "Current number of packet filter of bearer %d. \n", tft->numberofpacketfilters, bearer_context->ebi, emm_context->ue_id, bearer_context->esm_ebr_context.tft->numberofpacketfilters);
      /** No need to update the identifier bitmap. */
      for(int prec = 0; prec < sizeof(tft->packetfilterlist.replacepacketfilter); prec ++){
        for(int prec1 = 0; prec1 < sizeof(bearer_context->esm_ebr_context.tft->precedence_set); prec1 ++){
          if(bearer_context->esm_ebr_context.tft->precedence_set[prec1] == tft->packetfilterlist.replacepacketfilter[prec].identifier){
            bearer_context->esm_ebr_context.tft->precedence_set[prec1] = 0;
            bearer_context->esm_ebr_context.tft->precedence_set[tft->packetfilterlist.replacepacketfilter[prec].eval_precedence] = tft->packetfilterlist.replacepacketfilter[prec].identifier; /**< Remove the precedences. */
          }
        }
      }
    } else {
      // todo: check that no other operation code is permitted, bearers without tft not permitted and default bearer may not have tft
      OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Received invalid TFT operation code %d for bearer with (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n",
          tft->tftoperationcode, bearer_context->ebi, emm_context->ue_id);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
   }
  }

  /*
   * Return the EPS bearer identity of the default EPS bearer
   * * * * associated to the new EPS bearer context
   */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
}

//------------------------------------------------------------------------------
void esm_ebr_context_init (esm_ebr_context_t *esm_ebr_context)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  if (esm_ebr_context)  {
    memset(esm_ebr_context, 0, sizeof(*esm_ebr_context));
    /*
     * Set the EPS bearer context status to INACTIVE
     */
    esm_ebr_context->status = ESM_EBR_INACTIVE;
    /*
     * Disable the retransmission timer
     */
    esm_ebr_context->timer.id = NAS_TIMER_INACTIVE_ID;
  }
  OAILOG_FUNC_OUT (LOG_NAS_ESM);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_context_release()                                 **
 **                                                                        **
 ** Description: Releases EPS bearer context entry previously allocated    **
 **      to the EPS bearer with the specified EPS bearer identity  **
 **                                                                        **
 ** Inputs:  ue_id:      UE identifier                              **
 **      ebi:       EPS bearer identity                        **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     pid:       Identifier of the PDN connection entry the **
 **             EPS bearer context belongs to              **
 **      bid:       Identifier of the released EPS bearer con- **
 **             text entry                                 **
 **      Return:    The EPS bearer identity associated to the  **
 **             EPS bearer context if successfully relea-  **
 **             sed; UNASSIGN EPS bearer value otherwise.  **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ***************************************************************************/
ebi_t
esm_ebr_context_release (
  emm_data_context_t * emm_context,
  ebi_t ebi,
  pdn_cid_t *pid,
  bool ue_requested)
{
  OAILOG_FUNC_IN (LOG_NAS_ESM);
  int                                     found = false;
  esm_pdn_t                              *pdn = NULL;

  ue_context_t                        *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, emm_context->ue_id);
  pdn_context_t                       *pdn_context = NULL;
  bearer_context_t                    *bearer_context = NULL;

  if (ebi != ESM_EBI_UNASSIGNED) {
    if ((ebi < ESM_EBI_MIN) || (ebi > ESM_EBI_MAX)) {
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
    }
  }else{
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
  }

  if(*pid != NULL && *pid != MAX_APN_PER_UE){
    mme_app_get_pdn_context(ue_context, *pid, ebi, NULL, &pdn_context);
  }else{
    /** Get the bearer context from all session bearers. */
    mme_app_get_session_bearer_context_from_all(ue_context, ebi, &bearer_context);
    if(!bearer_context){
      OAILOG_ERROR(LOG_NAS_ESM , "ESM-PROC  - Could not find bearer context from ebi %d. \n", ebi);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
    }

    mme_app_get_pdn_context(ue_context, bearer_context->pdn_cx_id, 5/*bearer_context->linked_ebi*/, NULL, &pdn_context);
    *pid = bearer_context->pdn_cx_id;
  }

  /** At this point, we must have found the pdn_context. */
  if(!pdn_context){
    OAILOG_ERROR(LOG_NAS_ESM , "ESM-PROC  - PDN connection identifier %d " "is not valid\n", *pid);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
  }

  pdn = &pdn_context->esm_data;
  /*
   * Check if it is a default ebi, if so release all the bearer contexts.
   * If not, just release a single bearer context.
   */
  if (ebi == pdn_context->default_ebi || ebi == ESM_SAP_ALL_EBI) {
    /*
     * 3GPP TS 24.301, section 6.4.4.3, 6.4.4.6
     * * * * If the EPS bearer identity is that of the default bearer to a
     * * * * PDN, the UE shall delete all EPS bearer contexts associated to
     * * * * that PDN connection.
     */
    RB_FOREACH(bearer_context, SessionBearers, &pdn_context->session_bearers){
      OAILOG_WARNING (LOG_NAS_ESM , "ESM-PROC  - Release EPS bearer context " "(ebi=%d, pid=%d)\n", bearer_context->ebi, *pid);
      /*
       * Delete the TFT
       */
      // TODO Look at "free_traffic_flow_template"
      //free_traffic_flow_template(&pdn->bearer[i]->tft);

      /*
       * Set the EPS bearer context state to INACTIVE
       */
      int rc  = esm_ebr_release (emm_context, bearer_context, pdn_context, ue_requested);
      if (rc != RETURNok) {
         /*
          * Failed to update ESM bearer status.
          */
         OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Failed to update ESM bearer status. \n");
         OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
      }

//      pdn->n_bearers -= 1;
//
//      // todo: update esm context
//      emm_context->esm_ctx.n_active_ebrs--;
//
//      // todo: fix:
//      break;
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, ebi);
    }

    /*
     * Reset the PDN connection activation indicator
     */
    // TODO Look at "Reset the PDN connection activation indicator"
    pdn_context->is_active = false;

  }else{
    /*
      * The identity of the EPS bearer to released is given;
      * Release the EPS bearer context entry that match the specified EPS
      * bearer identity
      */
     bearer_context = mme_app_get_session_bearer_context(pdn_context, ebi);
     if(!bearer_context) {
       OAILOG_ERROR(LOG_NAS_ESM , "ESM-PROC  - PDN connection identifier %d " "is not valid\n", *pid);
       OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
     }
     /*
      * Delete the TFT
      */
     // TODO Look at "free_traffic_flow_template"
     //free_traffic_flow_template(&pdn->bearer[i]->tft);

     /*
      * Set the EPS bearer context state to INACTIVE
      */
     int rc  = esm_ebr_release (emm_context, bearer_context, pdn_context, ue_requested);
     if (rc != RETURNok) {
        /*
         * Failed to update ESM bearer status.
         */
        OAILOG_WARNING (LOG_NAS_ESM, "ESM-PROC  - Failed to update ESM bearer status. \n");
        OAILOG_FUNC_RETURN (LOG_NAS_ESM, ESM_EBI_UNASSIGNED);
     }

     pdn->n_bearers -= 1;

     // todo: update esm context
     emm_context->esm_ctx.n_active_ebrs--;
  }

  /*
   * Update the emergency bearer services indicator
   */
  if (pdn->is_emergency) {
    pdn->is_emergency = false;
  }
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, ebi);
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

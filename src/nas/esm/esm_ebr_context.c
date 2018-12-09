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
#include "mme_app_defs.h"


#include "emm_data.h"
#include "emm_sap.h"
#include "esm_ebr.h"
#include "esm_sapDef.h"
#include "esm_ebr_context.h"

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
  mme_ue_s1ap_id_t ue_id,
  bearer_context_t * bearer_context,
  const bearer_qos_t *bearer_level_qos,
  traffic_flow_template_t * tft,
  protocol_configuration_options_t * pco)
{
  int                                     bidx = 0;
  esm_pdn_t                              *pdn = NULL;

  OAILOG_FUNC_IN (LOG_NAS_ESM);
  ue_context_t                           *ue_context  = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, esm_context->ue_id);

  OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Updating EPS bearer context (ebi=%d) " "for UE with ueId " MME_UE_S1AP_ID_FMT ". \n", bearer_context->ebi, esm_context->ue_id);

  if(!bearer_context){
    OAILOG_ERROR (LOG_NAS_ESM, "ESM-PROC  - EPS bearer context (ebi=%d) could not be found for UE with ueId " MME_UE_S1AP_ID_FMT ". \n", bearer_context->ebi, esm_context->ue_id);
    OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
  }

  // todo: check why esm_proc_data is needed and does it needs to be updated?! only for UE triggered qos update?
  // todo: esm_ctx->esm_proc_data

  /** Update EPS level QoS. */
  if(bearer_level_qos){
    OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Updating EPS QoS for EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n", bearer_context->ebi, esm_context->ue_id);

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
    OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Updating TFT for EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n", bearer_context->ebi, esm_context->ue_id);

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
            tft->packetfilterlist.addpacketfilter[num_pf].identifier, bearer_context->ebi, esm_context->ue_id);
        bearer_context->esm_ebr_context.tft->numberofpacketfilters++;
        /** Update the flags. */
        bearer_context->esm_ebr_context.tft->packet_filter_identifier_bitmap |= tft->packet_filter_identifier_bitmap;
        DevAssert(!bearer_context->esm_ebr_context.tft->precedence_set[new_packet_filter->eval_precedence]);
        bearer_context->esm_ebr_context.tft->precedence_set[new_packet_filter->eval_precedence] = new_packet_filter->identifier + 1;
        OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Set precedence %d as pfId %d to EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n",
            new_packet_filter->eval_precedence, bearer_context->esm_ebr_context.tft->precedence_set[new_packet_filter->eval_precedence], bearer_context->ebi, esm_context->ue_id);
      }
      OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Successfully added %d new packet filters to EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". "
          "Current number of packet filters of bearer %d. \n", tft->numberofpacketfilters, bearer_context->ebi, esm_context->ue_id, bearer_context->esm_ebr_context.tft->numberofpacketfilters);
    } else if(tft->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_DELETE_PACKET_FILTERS_FROM_EXISTING_TFT){
      // todo: check that at least one TFT exists, if not signal error !
      /** todo: (6.1.3.3.4) check at the beginning that all the packet filters exist, before continuing (check syntactical errors). */
      for(int num_pf = 0 ; num_pf < tft->numberofpacketfilters; num_pf++){
        /**
         * Assume that the identifier will be used as the ordinal.
         */
        int num_pf1 = 0;
        for(; num_pf1 < TRAFFIC_FLOW_TEMPLATE_NB_PACKET_FILTERS_MAX; num_pf1++) {
          /** Update the packet filter with the correct identifier. */
          if(bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1].identifier == tft->packetfilterlist.deletepacketfilter[num_pf].identifier)
            break;
        }
        DevAssert(num_pf1 < TRAFFIC_FLOW_TEMPLATE_NB_PACKET_FILTERS_MAX); /**< todo: make syntactical check before. */
        /** Remove the packet filter. */
        OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Removed the packet filter with packet filter id %d of EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n",
            tft->packetfilterlist.deletepacketfilter[num_pf].identifier, bearer_context->ebi, esm_context->ue_id);
        /** Remove from precedence list. */
        bearer_context->esm_ebr_context.tft->precedence_set[bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1].eval_precedence] = 0;
        memset((void*)&bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1], 0,
            sizeof(bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1]));
        bearer_context->esm_ebr_context.tft->numberofpacketfilters--;
      }
      OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Successfully deleted %d existing packet filters of EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". "
          "Current number of packet filters of bearer context %d. \n", tft->numberofpacketfilters, bearer_context->ebi, esm_context->ue_id, bearer_context->esm_ebr_context.tft->numberofpacketfilters);
      /** Update the flags. */
      bearer_context->esm_ebr_context.tft->packet_filter_identifier_bitmap &= ~tft->packet_filter_identifier_bitmap;
    } else if(tft->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_REPLACE_PACKET_FILTERS_IN_EXISTING_TFT){
      /** todo: (6.1.3.3.4) check at the beginning that all the packet filters exist, before continuing (check syntactical errors). */
      for(int num_pf = 0 ; num_pf < tft->numberofpacketfilters; num_pf++){
        /**
         * Assume that the identifier will be used as the ordinal.
         */
        int num_pf1 = 0;
        for(; num_pf1 < TRAFFIC_FLOW_TEMPLATE_NB_PACKET_FILTERS_MAX; num_pf1++) {
          /** Update the packet filter with the correct identifier. */
          if(bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1].identifier == tft->packetfilterlist.replacepacketfilter[num_pf].identifier)
            break;
        }
        DevAssert(num_pf1 < TRAFFIC_FLOW_TEMPLATE_NB_PACKET_FILTERS_MAX); /**< todo: make syntactical check before. */
        /** Clean the old packet filter and the precedence map entry. The identifier will stay.. */
        bearer_context->esm_ebr_context.tft->precedence_set[bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1].eval_precedence] = 0;
        memset((void*)&bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1], 0, sizeof(create_new_tft_t));
        OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Removed the packet filter with packet filter id %d of EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n",
            tft->packetfilterlist.replacepacketfilter[num_pf].identifier, bearer_context->ebi, esm_context->ue_id);
        // todo: use length?
        memcpy((void*)&bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1], &tft->packetfilterlist.replacepacketfilter[num_pf], sizeof(replace_packet_filter_t));
      }
      OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Successfully replaced %d existing packet filters of EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". "
          "Current number of packet filter of bearer %d. \n", tft->numberofpacketfilters, bearer_context->ebi, esm_context->ue_id, bearer_context->esm_ebr_context.tft->numberofpacketfilters);
      /** Removed the precedences, set them again, should be all zero. */
      for(int num_pf = 0 ; num_pf < tft->numberofpacketfilters; num_pf++){
        DevAssert(!bearer_context->esm_ebr_context.tft->precedence_set[tft->packetfilterlist.replacepacketfilter[num_pf].eval_precedence]);
        OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Successfully set the new precedence of packet filter id (%d +1) to %d of EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT "\n. ",
                  tft->packetfilterlist.replacepacketfilter[num_pf].identifier, tft->packetfilterlist.replacepacketfilter[num_pf].eval_precedence, bearer_context->ebi, esm_context->ue_id);
        bearer_context->esm_ebr_context.tft->precedence_set[tft->packetfilterlist.replacepacketfilter[num_pf].eval_precedence] = tft->packetfilterlist.replacepacketfilter[num_pf].identifier + 1;
      }
      /** No need to update the identifier bitmap. */
    } else {
      // todo: check that no other operation code is permitted, bearers without tft not permitted and default bearer may not have tft
      OAILOG_INFO (LOG_NAS_ESM, "ESM-PROC  - Received invalid TFT operation code %d for bearer with (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n",
          tft->tftoperationcode, bearer_context->ebi, esm_context->ue_id);
      OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNerror);
   }
  }

  /*
   * Return the EPS bearer identity of the default EPS bearer
   * * * * associated to the new EPS bearer context
   */
  OAILOG_FUNC_RETURN (LOG_NAS_ESM, RETURNok);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

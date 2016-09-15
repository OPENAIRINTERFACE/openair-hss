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
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "mme_scenario_player.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "3gpp_24.301.h"
#include "security_types.h"
#include "common_types.h"
#include "emm_msg.h"
#include "esm_msg.h"
#include "intertask_interface.h"
#include "common_defs.h"
#include "itti_comp.h"
#include "securityDef.h"

//------------------------------------------------------------------------------
int itti_msg_comp_sctp_new_association(const sctp_new_peer_t * const itti_msg1, const sctp_new_peer_t * const itti_msg2)
{
  if (itti_msg1->instreams != itti_msg2->instreams) return RETURNerror;
  if (itti_msg1->outstreams != itti_msg2->outstreams) return RETURNerror;
  if (itti_msg1->assoc_id != itti_msg2->assoc_id) return RETURNerror;
  return RETURNok;
}

//------------------------------------------------------------------------------
int itti_msg_comp_sctp_close_association(const sctp_close_association_t * const itti_msg1, const sctp_close_association_t * const itti_msg2)
{
  if (itti_msg1->assoc_id != itti_msg2->assoc_id) return RETURNerror;
  return RETURNok;
}

//------------------------------------------------------------------------------
int itti_msg_comp_s1ap_ue_context_release_req(const itti_s1ap_ue_context_release_req_t * const itti_msg1, const itti_s1ap_ue_context_release_req_t * const itti_msg2)
{
  if (itti_msg1->enb_ue_s1ap_id != itti_msg2->enb_ue_s1ap_id) return RETURNerror;
  if (itti_msg1->mme_ue_s1ap_id != itti_msg2->mme_ue_s1ap_id) return RETURNerror;
  return RETURNok;
}

//------------------------------------------------------------------------------
int itti_msg_comp_mme_app_connection_establishment_cnf(const itti_mme_app_connection_establishment_cnf_t * const itti_msg1,
    const itti_mme_app_connection_establishment_cnf_t * const itti_msg2)
{

  if (itti_msg1->ue_id != itti_msg2->ue_id) return RETURNerror;
  if (itti_msg1->no_of_e_rabs != itti_msg2->no_of_e_rabs) return RETURNerror;

  for (int i=0; i < itti_msg1->no_of_e_rabs; i++) {
    if (itti_msg1->e_rab_id[i] != itti_msg2->e_rab_id[i]) return RETURNerror;
    if (itti_msg1->e_rab_level_qos_qci[i] != itti_msg2->e_rab_level_qos_qci[i]) return RETURNerror;
    if (itti_msg1->e_rab_level_qos_priority_level[i] != itti_msg2->e_rab_level_qos_priority_level[i]) return RETURNerror;
    if (itti_msg1->e_rab_level_qos_preemption_capability[i] != itti_msg2->e_rab_level_qos_preemption_capability[i]) return RETURNerror;
    if (itti_msg1->e_rab_level_qos_preemption_vulnerability[i] != itti_msg2->e_rab_level_qos_preemption_vulnerability[i]) return RETURNerror;
    //itti_msg->transport_layer_address[i]
    if (itti_msg1->gtp_teid[i] != itti_msg2->gtp_teid[i]) return RETURNerror;
    if (blength(itti_msg1->nas_pdu[i]) != blength(itti_msg2->nas_pdu[i])) return RETURNerror;
    if (memcmp(itti_msg1->nas_pdu[i]->data, itti_msg2->nas_pdu[i]->data, blength(itti_msg2->nas_pdu[i]))) return RETURNerror;
  }
  if (itti_msg1->ue_security_capabilities_encryption_algorithms != itti_msg2->ue_security_capabilities_encryption_algorithms) return RETURNerror;
  if (itti_msg1->ue_security_capabilities_integrity_algorithms != itti_msg2->ue_security_capabilities_integrity_algorithms) return RETURNerror;
  if (memcmp(itti_msg1->kenb, itti_msg2->kenb, AUTH_KENB_SIZE)) return RETURNerror;
  return RETURNok;
}


//------------------------------------------------------------------------------
int itti_msg_comp_nas_downlink_data_req(const itti_nas_dl_data_req_t * const itti_msg1,
    const itti_nas_dl_data_req_t * const itti_msg2)
{
  if (itti_msg1->enb_ue_s1ap_id != itti_msg2->enb_ue_s1ap_id) {
    OAILOG_DEBUG(LOG_MME_SCENARIO_PLAYER, "NAS_DOWNLINK_DATA_REQ message differs with itti_nas_dl_data_req_t.enb_ue_s1ap_id: " ENB_UE_S1AP_ID_FMT " != " ENB_UE_S1AP_ID_FMT "\n",
        itti_msg1->enb_ue_s1ap_id, itti_msg2->enb_ue_s1ap_id );
    return RETURNerror;
  }
  if (itti_msg1->ue_id != itti_msg2->ue_id) {
    OAILOG_DEBUG(LOG_MME_SCENARIO_PLAYER, "NAS_DOWNLINK_DATA_REQ message differs with itti_nas_dl_data_req_t.ue_id: " MME_UE_S1AP_ID_FMT " != " MME_UE_S1AP_ID_FMT "\n",
        itti_msg1->ue_id, itti_msg2->ue_id );
    return RETURNerror;
  }
  if (blength(itti_msg1->nas_msg) != blength(itti_msg2->nas_msg)) {
    OAILOG_DEBUG(LOG_MME_SCENARIO_PLAYER, "NAS_DOWNLINK_DATA_REQ message differs with itti_nas_dl_data_req_t.nas_msg length: %d != %d\n",
        blength(itti_msg1->nas_msg), blength(itti_msg2->nas_msg) );
    return RETURNerror;
  }
  if (0 < blength(itti_msg2->nas_msg)) {
    if (memcmp(itti_msg1->nas_msg->data, itti_msg2->nas_msg->data, blength(itti_msg2->nas_msg))) {
      OAILOG_DEBUG(LOG_MME_SCENARIO_PLAYER, "NAS_DOWNLINK_DATA_REQ message differs with itti_nas_dl_data_req_t.nas_msg\n");
      return RETURNerror;
    }
  }
  return RETURNok;
}



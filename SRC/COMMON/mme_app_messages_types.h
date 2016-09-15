/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
#ifndef FILE_MME_APP_MESSAGES_TYPES_SEEN
#define FILE_MME_APP_MESSAGES_TYPES_SEEN

#define MME_APP_INITIAL_UE_MESSAGE(mSGpTR)               (mSGpTR)->ittiMsg.mme_app_initial_ue_message
#define MME_APP_CONNECTION_ESTABLISHMENT_CNF(mSGpTR)     (mSGpTR)->ittiMsg.mme_app_connection_establishment_cnf
#define MME_APP_INITIAL_CONTEXT_SETUP_RSP(mSGpTR)        (mSGpTR)->ittiMsg.mme_app_initial_context_setup_rsp
#define MME_APP_DELETE_SESSION_RSP(mSGpTR)	         (mSGpTR)->ittiMsg.mme_app_delete_session_rsp

typedef struct itti_mme_app_initial_ue_message_s {
  sctp_assoc_id_t     sctp_assoc_id; // key stored in MME_APP for MME_APP forward NAS response to S1AP
  mme_ue_s1ap_id_t    mme_ue_s1ap_id;
  enb_ue_s1ap_id_t    enb_ue_s1ap_id;
  bstring             nas;
  tai_t               tai;               /* Indicating the Tracking Area from which the UE has sent the NAS message.                         */
  ecgi_t              cgi;               /* Indicating the cell from which the UE has sent the NAS message.                         */
  rrc_establishment_cause_t      rrc_establishment_cause;          /* Establishment cause                     */

  bool                is_s_tmsi_valid;
  bool                is_csg_id_valid;
  bool                is_gummei_valid;
  s_tmsi_t            opt_s_tmsi;
  csg_id_t            opt_csg_id;
  gummei_t            opt_gummei;
  //void                opt_cell_access_mode;
  //void                opt_cell_gw_transport_address;
  //void                opt_relay_node_indicator;
  /* Transparent message from s1ap to be forwarded to MME_APP or
   * to S1AP if connection establishment is rejected by NAS.
   */
  itti_s1ap_initial_ue_message_t transparent;
} itti_mme_app_initial_ue_message_t;

typedef struct itti_mme_app_connection_establishment_cnf_s {
  mme_ue_s1ap_id_t        ue_id;


  ambr_t                  ue_ambr;

  // E-RAB to Be Setup List
  uint16_t                no_of_e_rabs; // spec says max 256, actually stay with BEARERS_PER_UE
  //     >>E-RAB ID
  ebi_t                   e_rab_id[BEARERS_PER_UE];
  //     >>E-RAB Level QoS Parameters
  qci_t                   e_rab_level_qos_qci[BEARERS_PER_UE];
  //       >>>Allocation and Retention Priority
  priority_level_t        e_rab_level_qos_priority_level[BEARERS_PER_UE];
  //       >>>Pre-emption Capability
  pre_emp_capability_t    e_rab_level_qos_preemption_capability[BEARERS_PER_UE];
  //       >>>Pre-emption Vulnerability
  pre_emp_vulnerability_t e_rab_level_qos_preemption_vulnerability[BEARERS_PER_UE];
  //     >>Transport Layer Address
  bstring                 transport_layer_address[BEARERS_PER_UE];
  //     >>GTP-TEID
  teid_t                  gtp_teid[BEARERS_PER_UE];
  //     >>NAS-PDU (optional)
  bstring                 nas_pdu[BEARERS_PER_UE];
  //     >>Correlation ID TODO? later...

  // UE Security Capabilities
  uint16_t                ue_security_capabilities_encryption_algorithms;
  uint16_t                ue_security_capabilities_integrity_algorithms;

  // Security key
  uint8_t                 kenb[AUTH_KENB_SIZE];

  // Trace Activation (optional)
  // Handover Restriction List (optional)
  // UE Radio Capability (optional)
  // Subscriber Profile ID for RAT/Frequency priority (optional)
  // CS Fallback Indicator (optional)
  // SRVCC Operation Possible (optional)
  // CSG Membership Status (optional)
  // Registered LAI (optional)
  // GUMMEI ID (optional)
  // MME UE S1AP ID 2  (optional)
  // Management Based MDT Allowed (optional)

  //itti_nas_conn_est_cnf_t nas_conn_est_cnf;
} itti_mme_app_connection_establishment_cnf_t;

typedef struct itti_mme_app_initial_context_setup_rsp_s {
  uint32_t                mme_ue_s1ap_id;
  uint16_t                no_of_e_rabs;
  ebi_t                   e_rab_id[BEARERS_PER_UE];
  bstring                 transport_layer_address[BEARERS_PER_UE];
  s1u_teid_t              gtp_teid[BEARERS_PER_UE];
} itti_mme_app_initial_context_setup_rsp_t;

typedef struct itti_mme_app_delete_session_rsp_s {
  /* UE identifier */
  mme_ue_s1ap_id_t	  ue_id;
} itti_mme_app_delete_session_rsp_t;

#endif /* FILE_MME_APP_MESSAGES_TYPES_SEEN */

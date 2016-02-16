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
#ifndef MME_APP_MESSAGES_TYPES_H_
#define MME_APP_MESSAGES_TYPES_H_


#define MME_APP_CONNECTION_ESTABLISHMENT_IND(mSGpTR)     (mSGpTR)->ittiMsg.mme_app_connection_establishment_ind
#define MME_APP_CONNECTION_ESTABLISHMENT_CNF(mSGpTR)     (mSGpTR)->ittiMsg.mme_app_connection_establishment_cnf
#define MME_APP_INITIAL_CONTEXT_SETUP_RSP(mSGpTR)        (mSGpTR)->ittiMsg.mme_app_initial_context_setup_rsp

typedef struct itti_mme_app_connection_establishment_ind_s {
  uint32_t            mme_ue_s1ap_id;
  nas_establish_ind_t nas;

  /* Transparent message from s1ap to be forwarded to MME_APP or
   * to S1AP if connection establishment is rejected by NAS.
   */
  s1ap_initial_ue_message_t transparent;
} itti_mme_app_connection_establishment_ind_t;

typedef struct itti_mme_app_connection_establishment_cnf_s {

  ebi_t                   eps_bearer_id;
  FTeid_t                 bearer_s1u_sgw_fteid;
  qci_t                   bearer_qos_qci;
  priority_level_t        bearer_qos_prio_level;
  pre_emp_vulnerability_t bearer_qos_pre_emp_vulnerability;
  pre_emp_capability_t    bearer_qos_pre_emp_capability;
  ambr_t                  ambr;

  /* Key eNB */
  uint8_t                 keNB[32];
  uint16_t                security_capabilities_encryption_algorithms;
  uint16_t                security_capabilities_integrity_algorithms;

  itti_nas_conn_est_cnf_t nas_conn_est_cnf;
} itti_mme_app_connection_establishment_cnf_t;

typedef struct itti_mme_app_initial_context_setup_rsp_s {
  uint32_t                mme_ue_s1ap_id;
  ebi_t                   eps_bearer_id;
  FTeid_t                 bearer_s1u_enb_fteid;
} itti_mme_app_initial_context_setup_rsp_t;


#endif /* MME_APP_MESSAGES_TYPES_H_ */

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



/*! \file mme_app_ue_context.h
 *  \brief MME applicative layer
 *  \author Sebastien ROUX
 *  \date 2013
 *  \version 1.0
 *  @defgroup _mme_app_impl_ MME applicative layer
 *  @ingroup _ref_implementation_
 *  @{
 */

#ifndef MME_APP_UE_CONTEXT_H
#define MME_APP_UE_CONTEXT_H


#include "common_types.h"
#include "sgw_ie_defs.h"
#include "s1ap_messages_types.h"
#include "nas_messages_types.h"
#include "s6a_messages_types.h"
#include "security_types.h"
#include "tree.h"
#include "hashtable.h"
#include "obj_hashtable.h"

#include <stdint.h>
#include <inttypes.h>   /* For sscanf formats */
#include <time.h>       /* to provide time_t */


typedef enum {
  ECM_IDLE,
  ECM_CONNECTED,
  ECM_DEREGISTERED,
} mm_state_t;

#define IMSI_DIGITS_MAX 15

typedef struct {
  uint32_t length;
  char data[IMSI_DIGITS_MAX + 1];
} mme_app_imsi_t;

#define IMSI_FORMAT "s"
#define IMSI_DATA(MME_APP_IMSI) (MME_APP_IMSI.data)

/* Convert the IMSI contained by a char string NULL terminated to uint64_t */

bool mme_app_is_imsi_empty(mme_app_imsi_t const * imsi);
bool mme_app_imsi_compare(mme_app_imsi_t const * imsi_a, mme_app_imsi_t const * imsi_b);
void mme_app_copy_imsi(mme_app_imsi_t * imsi_dst, mme_app_imsi_t const * imsi_src);

void mme_app_string_to_imsi(mme_app_imsi_t * const imsi_dst, char const * const imsi_string_src);
void mme_app_imsi_to_string(char * const imsi_dst, mme_app_imsi_t const * const imsi_src);

uint64_t mme_app_imsi_to_u64 (mme_app_imsi_t imsi_src);
void mme_app_ue_context_uint_to_imsi(uint64_t imsi_src, mme_app_imsi_t *imsi_dst);
void mme_app_convert_imsi_to_imsi_mme (mme_app_imsi_t * imsi_dst, const imsi_t *imsi_src);

/** @struct bearer_context_t
 *  @brief Parameters that should be kept for an eps bearer.
 */
typedef struct bearer_context_s {
  /* S-GW Tunnel Endpoint for User-Plane */
  s1u_teid_t       s_gw_teid;

  /* S-GW IP address for User-Plane */
  ip_address_t s_gw_address;

  /* P-GW Tunnel Endpoint for User-Plane */
  Teid_t       p_gw_teid;

  /* P-GW IP address for User-Plane */
  ip_address_t p_gw_address;

  /* QoS for this bearer */
  qci_t                   qci;
  priority_level_t        prio_level;
  pre_emp_vulnerability_t pre_emp_vulnerability;
  pre_emp_capability_t    pre_emp_capability;

  /* TODO: add TFT */
} bearer_context_t;

/** @struct ue_context_t
 *  @brief Useful parameters to know in MME application layer. They are set
 * according to 3GPP TS.23.401 #5.7.2
 */
typedef struct ue_context_s {
  /* Tree entry */
  //RB_ENTRY(ue_context_s) rb_entry;

  /* Basic identifier for ue. IMSI is encoded on maximum of 15 digits of 4 bits,
   * so usage of an unsigned integer on 64 bits is necessary.
   */
  mme_app_imsi_t         imsi;                        // set by nas_auth_param_req_t
#define IMSI_UNAUTHENTICATED  (0x0)
#define IMSI_AUTHENTICATED    (0x1)
  /* Indicator to show the IMSI authentication state */
  unsigned               imsi_auth:1;                 // set by nas_auth_resp_t

  enb_ue_s1ap_id_t       enb_ue_s1ap_id:24;
  mme_ue_s1ap_id_t       mme_ue_s1ap_id;
  sctp_assoc_id_t        sctp_assoc_id_key;

  uint8_t                nb_of_vectors;               // updated by S6A AUTHENTICATION ANSWER
  /* List of authentication vectors for E-UTRAN */
  eutran_vector_t       *vector_list;                 // updated by S6A AUTHENTICATION ANSWER
  // pointer in vector_list
  eutran_vector_t       *vector_in_use;               // updated by S6A AUTHENTICATION ANSWER

#define SUBSCRIPTION_UNKNOWN    0x0
#define SUBSCRIPTION_KNOWN      0x1
  unsigned               subscription_known:1;        // set by S6A UPDATE LOCATION ANSWER
  uint8_t                msisdn[MSISDN_LENGTH+1];     // set by S6A UPDATE LOCATION ANSWER
  uint8_t                msisdn_length;               // set by S6A UPDATE LOCATION ANSWER

  mm_state_t             mm_state;                    // not set/read
  /* Globally Unique Temporary Identity */
  guti_t                 guti;                        // guti.gummei.plmn set by nas_auth_param_req_t
  // read by S6A UPDATE LOCATION REQUEST
  me_identity_t          me_identity;                 // not set/read except read by display utility

  /* TODO: Add TAI list */

  /* Last known cell identity */
  ecgi_t                  e_utran_cgi;                 // set by nas_attach_req_t
  // read for S11 CREATE_SESSION_REQUEST
  /* Time when the cell identity was acquired */
  time_t                 cell_age;                    // set by nas_auth_param_req_t

  /* TODO: add csg_id */
  /* TODO: add csg_membership */

  network_access_mode_t  access_mode;                  // set by S6A UPDATE LOCATION ANSWER

  /* TODO: add ue radio cap, ms classmarks, supported codecs */

  /* TODO: add ue network capability, ms network capability */
  /* TODO: add selected NAS algorithm */

  /* TODO: add DRX parameter */

  apn_config_profile_t   apn_profile;                  // set by S6A UPDATE LOCATION ANSWER
  ard_t                  access_restriction_data;      // set by S6A UPDATE LOCATION ANSWER
  subscriber_status_t    sub_status;                   // set by S6A UPDATE LOCATION ANSWER
  ambr_t                 subscribed_ambr;              // set by S6A UPDATE LOCATION ANSWER
  ambr_t                 used_ambr;

  rau_tau_timer_t        rau_tau_timer;               // set by S6A UPDATE LOCATION ANSWER

  /* Store the radio capabilities as received in S1AP UE capability indication
   * message.
   */
  //char                  *ue_radio_capabilities;       // not set/read
  //int                    ue_radio_cap_length;         // not set/read

  Teid_t                 mme_s11_teid;                // set by mme_app_send_s11_create_session_req
  Teid_t                 sgw_s11_teid;                // set by S11 CREATE_SESSION_RESPONSE
  PAA_t                  paa;                         // set by S11 CREATE_SESSION_RESPONSE

  // temp
  char                   pending_pdn_connectivity_req_imsi[16];
  uint8_t                pending_pdn_connectivity_req_imsi_length;
  OctetString            pending_pdn_connectivity_req_apn;
  OctetString            pending_pdn_connectivity_req_pdn_addr;
  int                    pending_pdn_connectivity_req_pti;
  unsigned               pending_pdn_connectivity_req_ue_id;
  network_qos_t          pending_pdn_connectivity_req_qos;
  pco_flat_t             pending_pdn_connectivity_req_pco;
  void                  *pending_pdn_connectivity_req_proc_data;
  int                    pending_pdn_connectivity_req_request_type;

  ebi_t                  default_bearer_id;
  bearer_context_t       eps_bearers[BEARERS_PER_UE];
} ue_context_t;

typedef struct {
  uint32_t nb_ue_managed;
  uint32_t nb_ue_idle;

  uint32_t nb_bearers_managed;

  uint32_t nb_ue_since_last_stat;
  uint32_t nb_bearers_since_last_stat;

  ///* Entry to the root */
  //RB_HEAD(ue_context_map, ue_context_s) ue_context_tree;
  hash_table_ts_t  *imsi_ue_context_htbl;
  hash_table_ts_t  *tun11_ue_context_htbl;
  hash_table_ts_t  *mme_ue_s1ap_id_ue_context_htbl;
  hash_table_ts_t  *enb_ue_s1ap_id_ue_context_htbl;
  obj_hash_table_t  *guti_ue_context_htbl;
} mme_ue_context_t;

/** \brief Retrieve an UE context by selecting the provided IMSI
 * \param imsi Imsi to find in UE map
 * @returns an UE context matching the IMSI or NULL if the context doesn't exists
 **/
ue_context_t *mme_ue_context_exists_imsi(mme_ue_context_t * const mme_ue_context,
    const mme_app_imsi_t imsi);

/** \brief Retrieve an UE context by selecting the provided S11 teid
 * \param teid The tunnel endpoint identifier used between MME and S-GW
 * @returns an UE context matching the teid or NULL if the context doesn't exists
 **/
ue_context_t *mme_ue_context_exists_s11_teid(mme_ue_context_t * const mme_ue_context,
    const s11_teid_t teid);

/** \brief Retrieve an UE context by selecting the provided mme_ue_s1ap_id
 * \param mme_ue_s1ap_id The UE id identifier used in S1AP MME (and NAS)
 * @returns an UE context matching the mme_ue_s1ap_id or NULL if the context doesn't exists
 **/
ue_context_t *mme_ue_context_exists_mme_ue_s1ap_id(mme_ue_context_t * const mme_ue_context,
    const mme_ue_s1ap_id_t mme_ue_s1ap_id);

/** \brief Retrieve an UE context by selecting the provided enb_ue_s1ap_id
 * \param enb_ue_s1ap_id The UE id identifier used in S1AP MME
 * @returns an UE context matching the enb_ue_s1ap_id or NULL if the context doesn't exists
 **/
ue_context_t *mme_ue_context_exists_enb_ue_s1ap_id (
  mme_ue_context_t * const mme_ue_context_p,
  const enb_ue_s1ap_id_t enb_ue_s1ap_id);

/** \brief Retrieve an UE context by selecting the provided guti
 * \param guti The GUTI used by the UE
 * @returns an UE context matching the guti or NULL if the context doesn't exists
 **/
ue_context_t *mme_ue_context_exists_guti(mme_ue_context_t * const mme_ue_context,
    const guti_t * const guti);


/** \brief Create the association between mme_ue_s1ap_id and an UE context (enb_ue_s1ap_id key)
 * \param mme_ue_context_p The MME context
 * \param ue_context_p The UE context
 * \param enb_ue_s1ap_id The UE id identifier used in S1AP and MME_APP
 * \param mme_ue_s1ap_id The UE id identifier used in MME_APP and NAS
 **/
void
mme_ue_context_notified_new_ue_s1ap_id_association (
  mme_ue_context_t * const mme_ue_context_p,
  ue_context_t   * const ue_context_p,
  const enb_ue_s1ap_id_t enb_ue_s1ap_id,
  const mme_ue_s1ap_id_t mme_ue_s1ap_id);

/** \brief Update an UE context by selecting the provided guti
 * \param mme_ue_context_p The MME context
 * \param ue_context_p The UE context
 * \param mme_ue_s1ap_id The UE id identifier used in S1AP MME (and NAS)
 * \param imsi
 * \param mme_s11_teid The tunnel endpoint identifier used between MME and S-GW
 * \param nas_ue_id The UE id identifier used in S1AP MME and NAS
 * \param guti_p The GUTI used by the UE
 **/
void mme_ue_context_update_coll_keys(
    mme_ue_context_t * const mme_ue_context_p,
    ue_context_t     * const ue_context_p,
    const enb_ue_s1ap_id_t   enb_ue_s1ap_id,
    const mme_ue_s1ap_id_t   mme_ue_s1ap_id,
    const mme_app_imsi_t     imsi,
    const s11_teid_t         mme_s11_teid,
    const guti_t     * const guti_p);

/** \brief Insert a new UE context in the tree of known UEs.
 * At least the IMSI should be known to insert the context in the tree.
 * \param ue_context_p The UE context to insert
 * @returns 0 in case of success, -1 otherwise
 **/
int mme_insert_ue_context(mme_ue_context_t * const mme_ue_context,
                         const struct ue_context_s * const ue_context_p);


/** \brief TODO WORK HERE Remove UE context unnecessary information.
 *  mark it as released. It is necessary to keep track of the association (s_tmsi (guti), mme_ue_s1ap_id)
 * \param ue_context_p The UE context to remove
 **/
void mme_notify_ue_context_released (
    mme_ue_context_t * const mme_ue_context_p,
    struct ue_context_s *ue_context_p);

/** \brief Remove a UE context of the tree of known UEs.
 * \param ue_context_p The UE context to remove
 **/
void mme_remove_ue_context(mme_ue_context_t * const mme_ue_context,
		                   struct ue_context_s * const ue_context_p);


/** \brief Allocate memory for a new UE context
 * @returns Pointer to the new structure, NULL if allocation failed
 **/
ue_context_t *mme_create_new_ue_context(void);

/** \brief Dump the UE contexts present in the tree
 **/
void mme_app_dump_ue_contexts(const mme_ue_context_t * const mme_ue_context);


void mme_app_handle_s1ap_ue_context_release_req(const itti_s1ap_ue_context_release_req_t const *s1ap_ue_context_release_req);

#endif /* MME_APP_UE_CONTEXT_H */

/* @} */

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



/*! \file mme_app_session_context.h
 *  \brief MME applicative layer
 *  \author Dincer Beken, Lionel Gauthier
 *  \date 2013
 *  \version 1.0
 *  \email: dbeken@blackned.de
 *  @defgroup _mme_app_impl_ MME applicative layer
 *  @ingroup _ref_implementation_
 *  @{
 */

#ifndef FILE_MME_APP_SESSION_CONTEXT_SEEN
#define FILE_MME_APP_SESSION_CONTEXT_SEEN
#include <stdint.h>
#include <inttypes.h>   /* For sscanf formats */
#include <time.h>       /* to provide time_t */

#include "tree.h"
#include "queue.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "bstrlib.h"
#include "common_types.h"
#include "mme_app_messages_types.h"
#include "esm_data.h"

typedef int ( *mme_app_ue_callback_t) (void*);

// TODO: (amar) only used in testing
#define IMSI_FORMAT "s"
#define IMSI_DATA(MME_APP_IMSI) (MME_APP_IMSI.data)

#define BEARER_STATE_NULL        0
#define BEARER_STATE_SGW_CREATED (1 << 0)
#define BEARER_STATE_MME_CREATED (1 << 1)
#define BEARER_STATE_ENB_CREATED (1 << 2)
#define BEARER_STATE_ACTIVE      (1 << 3)
#define BEARER_STATE_S1_RELEASED (1 << 4)

#define MAX_APN_PER_UE        5 /**< Maximum number of bearers. */
#define MAX_NUM_BEARERS_UE    11 /**< Maximum number of bearers. */

typedef uint8_t mme_app_bearer_state_t;

/*
 * @struct bearer_context_new_t
 * @brief Parameters that should be kept for an eps bearer. Used for stacked memory
 *
 * Structure of an EPS bearer
 * --------------------------
 * An EPS bearer is a logical concept which applies to the connection
 * between two endpoints (UE and PDN Gateway) with specific QoS attri-
 * butes. An EPS bearer corresponds to one Quality of Service policy
 * applied within the EPC and E-UTRAN.
 */
typedef struct bearer_context_new_s {
  // EPS Bearer ID: An EPS bearer identity uniquely identifies an EP S bearer for one UE accessing via E-UTRAN
  ebi_t                       ebi;
  ebi_t                       linked_ebi;

  // S-GW IP address for S1-u: IP address of the S-GW for the S1-u interfaces.
  // S-GW TEID for S1u: Tunnel Endpoint Identifier of the S-GW for the S1-u interface.
  fteid_t                      s_gw_fteid_s1u;            // set by S11 CREATE_SESSION_RESPONSE

  // PDN GW TEID for S5/S8 (user plane): P-GW Tunnel Endpoint Identifier for the S5/S8 interface for the user plane. (Used for S-GW change only).
  // NOTE:
  // The PDN GW TEID is needed in MME context as S-GW relocation is triggered without interaction with the source S-GW, e.g. when a TAU
  // occurs. The Target S-GW requires this Information Element, so it must be stored by the MME.
  // PDN GW IP address for S5/S8 (user plane): P GW IP address for user plane for the S5/S8 interface for the user plane. (Used for S-GW change only).
  // NOTE:
  // The PDN GW IP address for user plane is needed in MME context as S-GW relocation is triggered without interaction with the source S-GW,
  // e.g. when a TAU occurs. The Target S GW requires this Information Element, so it must be stored by the MME.
  fteid_t                      		p_gw_fteid_s5_s8_up;

  // EPS bearer QoS: QCI and ARP, optionally: GBR and MBR for GBR bearer

  // extra 23.401 spec members
  pdn_cid_t                         pdn_cx_id;

  /*
   * Two bearer states, one mme_app_bearer_state (towards SAE-GW) and one towards eNodeB (if activated in RAN).
   * todo: setting one, based on the other is possible?
   */
  mme_app_bearer_state_t            bearer_state;     /**< Need bearer state to establish them. */
  esm_ebr_context_t                 esm_ebr_context;  /**< Contains the bearer level QoS parameters. */
  fteid_t                           enb_fteid_s1u;

  /* QoS for this bearer */
  bearer_qos_t                		bearer_level_qos;

  /** Add an entry field to make it part of a list (session or UE, no need to save more lists). */
  LIST_ENTRY(bearer_context_new_s) 	entries;
}__attribute__((__packed__)) bearer_context_new_t;

/** @struct subscribed_apn_t
 *  @brief Parameters that should be kept for a subscribed apn by the UE.
 */
// For each active PDN connection:
typedef struct pdn_context_s {
  context_identifier_t      context_identifier; // key

  //APN in Use: The APN currently used. This APN shall be composed of the APN Network
  //            Identifier and the default APN Operator Identifier, as specified in TS 23.003 [9],
  //            clause 9.1.2 (EURECOM: "mnc<MNC>.mcc<MCC>.gprs"). Any received value in the APN OI Replacement field is not applied
  //            here.
  bstring                     apn_in_use;        // an ID for P-GW through which a user can access the Subscribed APN

  // APN Restriction: Denotes the restriction on the combination of types of APN for the APN associated
  //                  with this EPS bearer Context.

  // APN Subscribed: The subscribed APN received from the HSS (APN-NI // ULA:APN Service Selection).
  bstring                     apn_subscribed;

  // APN-OI Replacement: APN level APN-OI Replacement which has same role as UE level APN-OI
  // Replacement but with higher priority than UE level APN-OI Replacement. This is
  // an optional parameter. When available, it shall be used to construct the PDN GW
  // FQDN instead of UE level APN-OI Replacement.
  bstring         apn_oi_replacement;

  // PDN Type: IPv4, IPv6 or IPv4v6
  pdn_type_t                  pdn_type; /**< Set by UE/ULR. */

  // IP Address(es): IPv4 address and/or IPv6 prefix
  //                 NOTE:
  //                 The MME might not have information on the allocated IPv4 address.
  //                 Alternatively, following mobility involving a pre-release 8 SGSN, this
  //                 IPv4 address might not be the one allocated to the UE.
  paa_t             *paa ;                         // set by S11 CREATE_SESSION_RESPONSE


  // EPS PDN Charging Characteristics: The charging characteristics of this PDN connection, e.g. normal, prepaid, flat-rate
  // and/or hot billing.

  // SIPTO permissions: Indicates whether the traffic associated with this APN is allowed or prohibited for SIPTO
  // LIPA permissions: Indicates whether the PDN can be accessed via Local IP Access. Possible values
  //                   are: LIPA-prohibited, LIPA-only and LIPA-conditional.

  // VPLMN Address Allowed: Specifies whether the UE is allowed to use the APN in the domain of the HPLMN
  //                        only, or additionally the APN in the domain of the VPLMN.

  // PDN GW Address in Use(control plane): The IP address of the PDN GW currently used for sending control plane signalling.
  ip_address_t                p_gw_address_s5_s8_cp;

  // PDN GW TEID for S5/S8 (control plane): PDN GW Tunnel Endpoint Identifier for the S5/S8 interface for the control plane.
  //                                        (For GTP-based S5/S8 only).
  teid_t                      p_gw_teid_s5_s8_cp;

  // MS Info Change Reporting Action: Need to communicate change in User Location Information to the PDN GW with this EPS bearer Context.

  // CSG Information Reporting Action: Need to communicate change in User CSG Information to the PDN GW with this
  //                                   EPS bearer Context.
  //                                   This field denotes separately whether the MME/SGSN are requested to send
  //                                   changes in User CSG Information for (a) CSG cells, (b) hybrid cells in which the
  //                                   subscriber is a CSG member and (c) hybrid cells in which the subscriber is not a
  //                                   CSG member.

  // EPS subscribed QoS profile: The bearer level QoS parameter values for that APN's default bearer (QCI and
  // ARP) (see clause 4.7.3).

  // Subscribed APN-AMBR: The Maximum Aggregated uplink and downlink MBR values to be shared across
  //                      all Non-GBR bearers, which are established for this APN, according to the
  //                      subscription of the user.
  ambr_t                       subscribed_apn_ambr;

  // PDN GW GRE Key for uplink traffic (user plane): PDN GW assigned GRE Key for the S5/S8 interface for the user plane for uplink traffic. (For PMIP-based S5/S8 only)

  // Default bearer: Identifies the EPS Bearer Id of the default bearer within the given PDN connection.
  ebi_t                       default_ebi;

  //bstring                     pgw_id;            // an ID for P-GW through which a user can access the Subscribed APN

  /**
   * Lists per APN, to keep bearer contexts and UE/EMM context separated.
   * We will store the bearers with the linked ebi, because the context id may change with S10 handovers.
   */
  LIST_HEAD(session_bearers_s, bearer_context_new_s) session_bearers;

  /* S-GW IP address for Control-Plane */
  union {
	  struct sockaddr_in     ipv4_addr;
	  struct sockaddr_in6    ipv6_addr;
  } s_gw_addr_s11_s4;

  teid_t                      s_gw_teid_s11_s4;            // set by S11 CREATE_SESSION_RESPONSE
  protocol_configuration_options_t *pco; // temp storage of information waiting for activation of required procedure
  RB_ENTRY (pdn_context_s)    pdnCtxRbtNode;            /**< RB Tree Data Structure Node        */
} pdn_context_t;

/**
 * Bearer Pool elements.
 * Contains list bearer elements and PDN sessions.
 * A lock should be kept for all of it.
 */
typedef struct ue_session_pool_s {
	mme_ue_s1ap_id_t		mme_ue_s1ap_id;
	teid_t					mme_teid_s11;
	teid_t					saegw_teid_s11;

	bearer_context_new_t 	bcs_ue[MAX_NUM_BEARERS_UE];
	pdn_context_t 			pdn_ue[MAX_APN_PER_UE];
	/*
	 * List of empty bearer context.
	 * Take the bearer contexts from here and put them into the PDN context.
	 */
	LIST_HEAD(free_bearers_s, bearer_context_new_s) free_bearers;

	/**
	 * Map of PDN
	 */
	RB_HEAD(PdnContexts, pdn_context_s) pdn_contexts;

	/** ESM Procedures */
	struct esm_procedures_s {
		LIST_HEAD(esm_pdn_connectivity_procedures_s, nas_esm_proc_pdn_connectivity_s) *pdn_connectivity_procedures;
		LIST_HEAD(esm_bearer_context_procedures_s, nas_esm_proc_bearer_context_s)   *bearer_context_procedures;
	}esm_procedures;

	LIST_HEAD(s11_procedures_s, mme_app_s11_proc_s) *s11_procedures;

	// todo: remove later
	ebi_t                        next_def_ebi_offset;

	/** Point to the next free session pool. */
	struct ue_session_pool_s*     					 next_free_sp;
}ue_session_pool_t;

/* Declaration (prototype) of the function to store pdn and bearer contexts. */
RB_PROTOTYPE(PdnContexts, pdn_context_s, pdn_ctx_rbt_Node, mme_app_compare_pdn_context)

typedef struct mme_ue_session_pool_s {
  uint32_t               nb_ue_session_pools_managed;
  uint32_t               nb_ue_session_pools_since_last_stat;
  uint32_t               nb_bearers_managed;
  uint32_t               nb_bearers_since_last_stat;
  // todo: check if necessary hash_table_uint64_ts_t  *tun11_ue_context_htbl;// data is mme_ue_s1ap_id_t
  hash_table_ts_t 		  *mme_ue_s1ap_id_ue_session_pool_htbl;
  hash_table_uint64_ts_t  *tun11_ue_session_pool_htbl;// data is mme_ue_s1ap_id_t
} mme_ue_session_pool_t;

ue_session_pool_t * get_new_session_pool();
void release_session_pool(ue_session_pool_t ** ue_session_pool);

void
mme_ue_session_pool_update_coll_keys (
  mme_ue_session_pool_t * const mme_ue_session_pool_p,
  ue_session_pool_t     * const ue_session_pool,
  const mme_ue_s1ap_id_t   mme_ue_s1ap_id,
  const s11_teid_t         mme_teid_s11);

void mme_ue_session_pool_dump_coll_keys(void);

int mme_insert_ue_session_pool (
  mme_ue_session_pool_t * const mme_ue_session_pool_p,
  const struct ue_session_pool_s *const ue_session_pool);
void mme_remove_ue_session_pool(
  mme_ue_session_pool_t * const mme_ue_session_pool_p,
  struct ue_session_pool_s *ue_session_pool);

void mme_app_esm_detach (mme_ue_s1ap_id_t ue_id);
int mme_app_pdn_process_session_creation(mme_ue_s1ap_id_t ue_id, imsi64_t imsi, mm_state_t mm_state, ambr_t subscribed_ue_ambr, fteid_t * saegw_s11_fteid, gtpv2c_cause_t *cause,
    bearer_contexts_created_t * bcs_created, ambr_t *ambr, paa_t ** paa, protocol_configuration_options_t * pco);

/** \brief Retrieve an UE context by selecting the provided mme_ue_s1ap_id
 * \param mme_ue_s1ap_id The UE id identifier used in S1AP MME (and NAS)
 * @returns an UE context matching the mme_ue_s1ap_id or NULL if the context doesn't exists
 **/
ue_session_pool_t *mme_ue_session_pool_exists_mme_ue_s1ap_id(mme_ue_session_pool_t * const mme_ue_context, const mme_ue_s1ap_id_t mme_ue_s1ap_id);
struct ue_session_pool_s * mme_ue_session_pool_exists_s11_teid ( mme_ue_session_pool_t * const mme_ue_session_pool_p,  const s11_teid_t teid);

ebi_t mme_app_get_free_bearer_id(ue_session_pool_t * const ue_session_pool);

void mme_app_ue_session_pool_s1_release_enb_informations(mme_ue_s1ap_id_t ue_id);

ambr_t mme_app_total_p_gw_apn_ambr(ue_session_pool_t *ue_session_pool);

ambr_t mme_app_total_p_gw_apn_ambr_rest(ue_session_pool_t *ue_session_pool, pdn_cid_t pci);

#endif /* FILE_MME_APP_UE_CONTEXT_SEEN */

/* @} */

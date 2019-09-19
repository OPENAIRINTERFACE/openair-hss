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
 *  \author Sebastien ROUX, Lionel Gauthier
 *  \date 2013
 *  \version 1.0
 *  \email: lionel.gauthier@eurecom.fr
 *  @defgroup _mme_app_impl_ MME applicative layer
 *  @ingroup _ref_implementation_
 *  @{
 */

#ifndef FILE_MME_APP_UE_CONTEXT_SEEN
#define FILE_MME_APP_UE_CONTEXT_SEEN
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
#include "s1ap_messages_types.h"
#include "nas_messages_types.h"
#include "s6a_messages_types.h"
#include "s10_messages_types.h"
#include "s11_messages_types.h"
#include "security_types.h"
#include "emm_data.h"
#include "esm_data.h"

typedef enum {
  ECM_IDLE = 0,
  ECM_CONNECTED,
} ecm_state_t;


#define IMSI_DIGITS_MAX 15

typedef struct {
  uint32_t length;
  char data[IMSI_DIGITS_MAX + 1];
} mme_app_imsi_t;

typedef int ( *mme_app_ue_callback_t) (void*);

// TODO: (amar) only used in testing
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

/*
 * Timer identifier returned when in inactive state (timer is stopped or has
 * failed to be started)
 */
#define MME_APP_TIMER_INACTIVE_ID   (-1)
#define MME_APP_DELTA_T3412_REACHABILITY_TIMER 4 // in minutes
#define MME_APP_DELTA_REACHABILITY_IMPLICIT_DETACH_TIMER 0 // in minutes
#define MME_APP_INITIAL_CONTEXT_SETUP_RSP_TIMER_VALUE 2 // In seconds

// todo: #define MME_APP_INITIAL_CONTEXT_SETUP_RSP_TIMER_VALUE 2 // In seconds
/* Timer structure */
struct mme_app_timer_t {
  long id;         /* The timer identifier                 */
  long sec;       /* The timer interval value in seconds  */
};

/** @struct ue_context_t
 *  @brief Useful parameters to know in MME application layer. They are set
 * according to 3GPP TS.23.401 #5.7.2
 */
typedef struct ue_context_s {
  /* S10 procedures. */
  LIST_HEAD(s10_procedures_s, mme_app_s10_proc_s) *s10_procedures;
  struct {
	  pthread_mutex_t recmutex;  // mutex on the ue_context_t
	  /** UE Identifiers. */
	  enb_s1ap_id_key_t      enb_s1ap_id_key; // key uniq among all connected eNBs

	  // MME UE S1AP ID, Unique identity of the UE within MME.
	  mme_ue_s1ap_id_t       mme_ue_s1ap_id;

	  // Mobile Reachability Timer-Start when UE moves to idle state. Stop when UE moves to connected state
	  struct mme_app_timer_t       mobile_reachability_timer;
	  // Implicit Detach Timer-Start at the expiry of Mobile Reachability timer. Stop when UE moves to connected state
	  struct mme_app_timer_t       implicit_detach_timer;
	  // Initial Context Setup Procedure Guard timer
	  struct mme_app_timer_t       initial_context_setup_rsp_timer;
	  /** Custom timer to remove UE at the source-MME side after a timeout. */
	  //  struct mme_app_timer_t       mme_mobility_completion_timer; // todo: not TXXXX value found for this.
	  // todo: (2) timers necessary for handover?
	  struct mme_app_timer_t       s1ap_handover_req_timer;
	  enum s1cause           	   s1_ue_context_release_cause;
	  struct {
		  /* Basic identifier for ue. IMSI is encoded on maximum of 15 digits of 4 bits,
		   * so usage of an unsigned integer on 64 bits is necessary.
		   */
		  imsi64_t         		 imsi;                      // set by nas_auth_param_req_t
		  bstring                msisdn;                    // The basic MSISDN of the UE. The presence is dictated by its storage in the HSS.
		                                                    // set by S6A UPDATE LOCATION ANSWER
		  ecm_state_t            ecm_state;                 // ECM state ECM-IDLE, ECM-CONNECTED.
		                                                    // not set/read
		  // todo: enum s1cause
		  mm_state_t             mm_state;

		  /* Last known cell identity */
		  ecgi_t                  e_utran_cgi;                 // Last known E-UTRAN cell, set by nas_attach_req_t
		  // read for S11 CREATE_SESSION_REQUEST
		  /* Time when the cell identity was acquired */
		  time_t                 cell_age;                    // Time elapsed since the last E-UTRAN Cell Global Identity was acquired. set by nas_auth_param_req_t

		  /* TODO: add csg_id */
		  /* TODO: add csg_membership */
		  /* TODO Access mode: Access mode of last known ECGI when the UE was active */

		  /* Store the radio capabilities as received in S1AP UE capability indication message. */
		  // UE Radio Access Capability UE radio access capabilities.
		  //  bstring                 ue_radio_capabilities;       // not set/read


		  // UE Network Capability  // UE network capabilities including security algorithms and key derivation functions supported by the UE

		  // MS Network Capability  // For a GERAN and/or UTRAN capable UE, this contains information needed by the SGSN.

		  /* TODO: add DRX parameter */
		  // UE Specific DRX Parameters   // UE specific DRX parameters for A/Gb mode, Iu mode and S1-mode

		  // Selected NAS Algorithm       // Selected NAS security algorithm
		  // eKSI                         // Key Set Identifier for the main key K ASME . Also indicates whether the UE is using
		                                  // security keys derived from UTRAN or E-UTRAN security association.

		  // K ASME                       // Main key for E-UTRAN key hierarchy based on CK, IK and Serving network identity

		  // NAS Keys and COUNT           // K NASint , K_ NASenc , and NAS COUNT parameter.

		  // Selected CN operator id      // Selected core network operator identity (to support network sharing as defined in TS 23.251 [24]).

		  // Recovery                     // Indicates if the HSS is performing database recovery.

		  ard_t                  access_restriction_data;      // The access restriction subscription information. set by S6A UPDATE LOCATION ANSWER

		  // ODB for PS parameters        // Indicates that the status of the operator determined barring for packet oriented services.

		  // APN-OI Replacement           // Indicates the domain name to replace the APN-OI when constructing the PDN GW
		                                  // FQDN upon which to perform a DNS resolution. This replacement applies for all
		                                  // the APNs in the subscriber's profile. See TS 23.003 [9] clause 9.1.2 for more
		                                  // information on the format of domain names that are allowed in this field.
		  bstring      apn_oi_replacement; // example: "province1.mnc012.mcc345.gprs"

		  // MME IP address for S11       // MME IP address for the S11 interface (used by S-GW)
		  // LOCATED IN mme_config_t.ipv4.s11

		  // MME TEID for S11             // MME Tunnel Endpoint Identifier for S11 interface.
		  // LOCATED IN THIS.subscribed_apns[MAX_APN_PER_UE].mme_teid_s11

		  // S-GW IP address for S11/S4   // S-GW IP address for the S11 and S4 interfaces
		  // LOCATED IN THIS.subscribed_apns[MAX_APN_PER_UE].s_gw_address_s11_s4

		  // S-GW TEID for S11/S4         // S-GW Tunnel Endpoint Identifier for the S11 and S4 interfaces.
		  // LOCATED IN THIS.subscribed_apns[MAX_APN_PER_UE].s_gw_teid_s11_s4

		  // SGSN IP address for S3       // SGSN IP address for the S3 interface (used if ISR is activated for the GERAN and/or UTRAN capable UE)

		  // SGSN TEID for S3             // SGSN Tunnel Endpoint Identifier for S3 interface (used if ISR is activated for the E-UTRAN capable UE)

		  // eNodeB Address in Use for S1-MME // The IP address of the eNodeB currently used for S1-MME.
		  // implicit with use of SCTP through the use of sctp_assoc_id_key
		  sctp_assoc_id_t        sctp_assoc_id_key; // link with eNB id

		  // eNB UE S1AP ID,  Unique identity of the UE within eNodeB.
		  enb_ue_s1ap_id_t       enb_ue_s1ap_id:24;

		  // MME TEID for S11             // MME Tunnel Endpoint Identifier for S11 interface.
		  // LOCATED IN THIS.subscribed_apns[MAX_APN_PER_UE].mme_teid_s11
		  teid_t                      local_mme_teid_s10;                // needed to get the UE context from S10 messages
		  teid_t                      mme_teid_s11;                	// set by mme_app_send_s11_create_session_req
		  teid_t                      saegw_teid_s11;                // set by mme_app_send_s11_create_session_req

		  // EPS Subscribed Charging Characteristics: The charging characteristics for the MS e.g. normal, prepaid, flat rate and/or hot billing.
		  // Subscribed RFSP Index: An index to specific RRM configuration in the E-UTRAN that is received from the HSS.
		  // RFSP Index in Use: An index to specific RRM configuration in the E-UTRAN that is currently in use.
		  // Trace reference: Identifies a record or a collection of records for a particular trace.
		  // Trace type: Indicates the type of trace
		  // Trigger id: Identifies the entity that initiated the trace
		  // OMC identity: Identifies the OMC that shall receive the trace record(s).
		  // URRP-MME: URRP-MME indicating that the HSS has requested the MME to notify the HSS regarding UE reachability at the MME.
		  // CSG Subscription Data: The CSG Subscription Data is a list of CSG IDs for the visiting PLMN and for each
		  //   CSG ID optionally an associated expiration date which indicates the point in time when the subscription to the CSG ID
		  //   expires; an absent expiration date indicates unlimited subscription. For a CSG ID that can be used to access specific PDNs via Local IP Access, the
		  //   CSG ID entry includes the corresponding APN(s).
		  // LIPA Allowed: Specifies whether the UE is allowed to use LIPA in this PLMN.

		  // Subscribed Periodic RAU/TAU Timer: Indicates a subscribed Periodic RAU/TAU Timer value.
		  rau_tau_timer_t        rau_tau_timer;               // set by S6A UPDATE LOCATION ANSWER

		  // MPS CS priority: Indicates that the UE is subscribed to the eMLPP or 1x RTT priority service in the CS domain.

		  // MPS EPS priority: Indicates that the UE is subscribed to MPS in the EPS domain.

		  network_access_mode_t  access_mode;                  // set by S6A UPDATE LOCATION ANSWER

		  network_access_mode_t  network_access_mode;       // set by S6A UPDATE LOCATION ANSWER

		  /* Time when the cell identity was acquired */
		  bstring                 ue_radio_capability;

		  /* Globally Unique Temporary Identity */
		  bool                   is_guti_set;                 // is guti has been set
		  guti_t                 guti;                        // guti.gummei.plmn set by nas_auth_param_req_t
		  // read by S6A UPDATE LOCATION REQUEST
		  me_identity_t          me_identity;                 // not set/read except read by display utility
	  }fields;
  }privates;
  /** Entries for UE context pool. */
  STAILQ_ENTRY (ue_context_s)		entries;
} ue_context_t;

typedef struct mme_ue_context_s {
  uint32_t               nb_ue_context_managed;
  uint32_t               nb_ue_context_idle;

  uint32_t               nb_ue_context_since_last_stat;

  uint32_t               nb_apn_configuration;

  hash_table_uint64_ts_t  *imsi_ue_context_htbl; // data is mme_ue_s1ap_id_t
  hash_table_uint64_ts_t  *tun10_ue_context_htbl;// data is mme_ue_s1ap_id_t
  hash_table_uint64_ts_t  *tun11_ue_context_htbl;// data is mme_ue_s1ap_id_t
  hash_table_ts_t 		  *mme_ue_s1ap_id_ue_context_htbl;
  hash_table_uint64_ts_t  *enb_ue_s1ap_id_ue_context_htbl; // data is enb_s1ap_id_key_t
  obj_hash_table_uint64_t *guti_ue_context_htbl;// data is mme_ue_s1ap_id_t
  /** Subscription profiles saved by IMSI. */
  hash_table_ts_t         *imsi_subscription_profile_htbl; // data is Subscription profile (not uint64)
} mme_ue_context_t;

/** \brief Retrieve an UE context by selecting the provided IMSI
 * \param imsi Imsi to find in UE map
 * @returns an UE context matching the IMSI or NULL if the context doesn't exists
 **/
ue_context_t *mme_ue_context_exists_imsi(mme_ue_context_t * const mme_ue_context,
    const imsi64_t imsi);

/** \brief Retrieve an UE context by selecting the provided S11 teid
 * \param teid The tunnel endpoint identifier used between MME and S-GW
 * @returns an UE context matching the teid or NULL if the context doesn't exists
 **/
ue_context_t *mme_ue_context_exists_s11_teid(mme_ue_context_t * const mme_ue_context,
    const s11_teid_t teid);

/** \brief Retrieve an UE context by selecting the provided S10 teid
 * \param teid The tunnel endpoint identifier used between MME and S-GW
 * @returns an UE context matching the teid or NULL if the context doesn't exists
 **/
ue_context_t *mme_ue_context_exists_s10_teid(mme_ue_context_t * const mme_ue_context,
    const s11_teid_t teid);


/** \brief Retrieve a subscription profile by imsi.
 * \param teid imsi
 * @returns the subscription profile received from the HSS.
 **/
subscription_data_t *mme_ue_subscription_data_exists_imsi ( mme_ue_context_t * const mme_ue_context_p,
    const imsi64_t imsi);

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
  const enb_s1ap_id_key_t enb_key);

/** \brief Retrieve an UE context by selecting the provided guti
 * \param guti The GUTI used by the UE
 * @returns an UE context matching the guti or NULL if the context doesn't exists
 **/
ue_context_t *mme_ue_context_exists_guti(mme_ue_context_t * const mme_ue_context,
    const guti_t * const guti);

/** \brief Update an UE context by selecting the provided guti
 * \param mme_ue_context_p The MME context
 * \param ue_context_p The UE context
 * \param enb_s1ap_id_key The eNB UE id identifier
 * \param mme_ue_s1ap_id The UE id identifier used in S1AP MME (and NAS)
 * \param imsi
 * \param mme_s11_teid The tunnel endpoint identifier used between MME and S-GW
 * \param nas_ue_id The UE id identifier used in S1AP MME and NAS
 * \param guti_p The GUTI used by the UE
 **/
void mme_ue_context_update_coll_keys(
    mme_ue_context_t * const mme_ue_context_p,
    ue_context_t     * const ue_context_p,
    const enb_s1ap_id_key_t  enb_s1ap_id_key,
    const mme_ue_s1ap_id_t   mme_ue_s1ap_id,
    const imsi64_t     imsi,
    const s11_teid_t         mme_teid_s11,
    const s10_teid_t         local_mme_teid_s10,
    const guti_t     * const guti_p);

/** \brief dump MME associative collections
 **/

void mme_ue_context_dump_coll_keys(void);

/** \brief Allocate a new UE context in the tree of known UEs.
 * Allocates a new MME_UE_S1AP_ID.
 * At least the IMSI should be known to insert the context in the tree.
 * \param ue_context_p The UE context to insert
 * @returns the ue_context in case of success, NULL otherwise
 **/
ue_context_t * get_new_ue_context(void);

void test_ue_context_extablishment(void);

/** \brief Remove a UE context of the tree of known UEs.
 * \param ue_context_p The UE context to remove
 **/
void mme_remove_ue_context(mme_ue_context_t * const mme_ue_context,
		                   struct ue_context_s * const ue_context_p);

/** \brief Insert a subscription profile received from the HSS for a given IMSI.
 * \param imsi
 * @returns 0 in case of success, -1 otherwise
 **/
int mme_insert_subscription_profile(mme_ue_context_t * const mme_ue_context,
                         const imsi64_t imsi,
                         const subscription_data_t * subscription_data);

/** \brief Remove subscription data of an IMSI cached from the HSS.
 * \param imsi
 **/
subscription_data_t * mme_remove_subscription_profile(mme_ue_context_t * const mme_ue_context_p, imsi64_t imsi);

/** \brief Update the UE context based on the subscription profile.
 * \param ue_id, subscription data
 **/
int mme_app_update_ue_subscription(mme_ue_s1ap_id_t ue_id, subscription_data_t * subscription_data);

/** \brief Dump the UE contexts present in the tree
 **/
void mme_app_dump_ue_contexts(const mme_ue_context_t * const mme_ue_context);

int mme_app_mobility_complete(const mme_ue_s1ap_id_t mme_ue_s1ap_id, const bool activate);

void mme_app_handle_s1ap_ue_context_release_req(const itti_s1ap_ue_context_release_req_t * const s1ap_ue_context_release_req);

#endif /* FILE_MME_APP_UE_CONTEXT_SEEN */

/* @} */

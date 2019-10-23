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

/*! \file m2ap_mce.h
  \brief
  \author Dincer BEKEN
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#ifndef FILE_MXAP_MCE_SEEN
#define FILE_MXAP_MCE_SEEN

#if MME_CLIENT_TEST == 0
# include "intertask_interface.h"
#endif

#include "hashtable.h"

// Forward declarations
struct m2ap_enb_description_s;

#define M2AP_TIMER_INACTIVE_ID   (-1)
#define M2AP_MBMS_CONTEXT_REL_COMP_TIMER 1 // in seconds

/* Timer structure */
struct mxap_timer_t {
  long id;           /* The timer identifier                 */
  long sec;          /* The timer interval value in seconds  */
};

// The current m2 state of the MCE relating to the specific eNB.
enum mce_m2_enb_state_s {
  M2AP_INIT,          /// The sctp association has been established but m2 hasn't been setup.
  M2AP_RESETING,      /// The m2state is resetting due to an SCTP reset on the bound association.
  M2AP_READY,         ///< MCE and eNB are M2 associated, MBMS contexts can be added
  M2AP_SHUTDOWN       /// The M2 state is being torn down due to sctp shutdown.
};

//enum s1_mbms_state_s {
//  S1AP_UE_INVALID_STATE,
//  S1AP_UE_WAITING_CSR,    ///< Waiting for Initial Context Setup Response
//  S1AP_UE_CONNECTED,      ///< MBMS context ready
//  S1AP_UE_WAITING_CRR,   /// MBMS Context release Procedure initiated , waiting for MBMS context Release Complete
//};

/** Main structure representing MBMS Bearer service association over m2ap per eNB.
 * Created upon a successful
 **/
typedef struct mbms_description_s {
  enb_mbms_m2ap_id_t enb_mbms_m2ap_id:24;    ///< Unique MBMS id over eNB (24 bits wide)
  struct mbms_enb_description_s *enb;         	 ///< Which eNB this MBMS is attached to

  /** Set the release cause, since we must now differentiate between them. */
  enum m2cause              m2_release_cause;  ///< M2AP Release Cause

  mce_mbms_m2ap_id_t 		mce_mbms_m2ap_id;       	///< Unique MBMS id over MCE (32 bits wide)

  /** SCTP stream on which M2 message will be sent/received.
   *  During an MBMS M2 connection, a pair of streams is
   *  allocated and is used during all the connection.
   *  Stream 0 is reserved for non MBMS signalling.
   *  @name sctp stream identifier
   **/
  /*@{*/
  sctp_stream_id_t sctp_stream_recv; ///< eNB -> MCE stream
  sctp_stream_id_t sctp_stream_send; ///< MCE -> eNB stream
  /*@}*/

  /* Timer for procedure outcome issued by MCE that should be answered */
  long outcome_response_timer_id;

  // MBMS Context Release procedure guard timer
  struct mxap_timer_t       mxap_mbms_context_rel_timer;
} mbms_description_t;

/* Main structure representing eNB association over m2ap
 * Generated (or updated) every time a new M2SetupRequest is received.
 */
typedef struct m2ap_enb_description_s {

  enum mce_m2_enb_state_s m2_state;         ///< State of the eNB specific M2AP association

  /** eNB related parameters **/
  /*@{*/
  char     				enb_name[150];      ///< Printable eNB Name
  uint32_t 				enb_id;             ///< Unique eNB ID
  mbms_service_area_t   mbms_sa_list;      ///< Tracking Area Identifiers signaled by the eNB (for each cell - used for paging.).
  /*@}*/

  /** MBMS list for this eNB **/
  /*@{*/
  uint32_t nb_mbms_associated; ///< Number of associated MBMS on this eNB
  hash_table_ts_t  mbms_coll; // contains mbms_description_s, key is mbms_description_s.?;
  /*@}*/

  /** SCTP stuff **/
  /*@{*/
  sctp_assoc_id_t  sctp_assoc_id;    ///< SCTP association id on this machine
  sctp_stream_id_t next_sctp_stream; ///< Next SCTP stream
  sctp_stream_id_t instreams;        ///< Number of streams avalaible on eNB -> MCE
  sctp_stream_id_t outstreams;       ///< Number of streams avalaible on MCE -> eNB
  /*@}*/
} m2ap_enb_description_t;

extern uint32_t         nb_enb_associated;
extern struct mme_config_s    *global_mme_config_p;

/** \brief MXAP layer top init
 * @returns -1 in case of failure
 **/
int mxap_mce_init(void);

/** \brief MXAP layer top exit
 **/
void mxap_mce_exit (void);

/** \brief Look for given eNB id in the list. Another method is necessary, since another map will be looked at.
 * \param enb_id The unique eNB id to search in list
 * @returns NULL if no eNB matchs the eNB id, or reference to the eNB element in list if matches
 **/
m2ap_enb_description_t* mxap_is_enb_id_in_list(const uint32_t enb_id);
//
///** \brief Look for given TAC in the list.
// * \param tac TAC is not uniqueue and used for the search in the list.
// * @returns All matched eNBs in the enb_list.
// **/
//void m2ap_is_tac_in_list (
//  const tac_t tac,
//  int *num_enbs,
//  m2ap_enb_description_t ** enbs);

/** \brief Look for given eNB SCTP assoc id in the list
 * \param enb_id The unique sctp assoc id to search in list
 * @returns NULL if no eNB matchs the sctp assoc id, or reference to the eNB element in list if matches
 **/
m2ap_enb_description_t* m2ap_is_enb_assoc_id_in_list(const sctp_assoc_id_t sctp_assoc_id);

/** \brief Look for given mbms eNB id in the list
 * \param enb_id The unique mbms_eNB id to search in list
 * @returns NULL if no MBMS matchs the mbms_enb_id, or reference to the mbms element in list if matches
 **/
mbms_description_t* m2ap_is_mbms_enb_id_in_list(m2ap_enb_description_t *enb_ref,
    const enb_mbms_m2ap_id_t enb_mbms_m2ap_id);

/** \brief Look for given mbms mce id in the list
 * \param enb_id The unique mbms_mce_id to search in list
 * @returns NULL if no MBMS matchs the mbms_mce_id, or reference to the mbms element in list if matches
 **/
mbms_description_t* m2ap_is_mbms_mce_id_in_list(const mce_mbms_m2ap_id_t mbms_mce_id);

/** \brief Look for given mbms enb m2ap id in the list of UEs for a particular enb.
 * \param enb_id The unique mbms_enb_id to search in list
 * @returns NULL if no MBMS matchs the mbms_enb_id, or reference to the mbms element in list if matches
 **/
mbms_description_t* m2ap_is_enb_mbms_m2ap_id_in_list_per_enb ( const enb_mbms_m2ap_id_t enb_mbms_m2ap_id, const uint32_t  enb_id);

///** \brief associate mainly 2(3) identifiers in M2AP layer: {mme_mbms_m2ap_id_t, sctp_assoc_id (,enb_mbms_m2ap_id)}
// **/
//void mxap_notified_new_mbms_mce_m2ap_id_association (
//    const sctp_assoc_id_t  sctp_assoc_id,
//    const enb_mbms_m2ap_id_t enb_mbms_m2ap_id,
//    const mce_mbms_m2ap_id_t mme_mbms_m2ap_id);

/** \brief Allocate and add to the list a new eNB descriptor
 * @returns Reference to the new eNB element in list
 **/
m2ap_enb_description_t* m2ap_new_enb(void);

/** \brief Dump the eNB related information.
 * hashtable callback. It is called by hashtable_ts_apply_funct_on_elements()
 * Calls m2ap_dump_enb
 **/
bool m2ap_dump_enb_hash_cb (const hash_key_t keyP,
               void * const enb_void,
               void *unused_parameterP,
               void **unused_resultP);

/** \brief Dump the eNB list
 * Calls dump_enb for each eNB in list
 **/
void m2ap_dump_enb_list(void);

/** \brief Dump eNB related information.
 * Calls dump_ue for each MBMS in list
 * \param enb_ref eNB structure reference to dump
 **/
void m2ap_dump_enb(const m2ap_enb_description_t * const enb_ref);

/** \brief Dump the MBMS related information.
 * hashtable callback. It is called by hashtable_ts_apply_funct_on_elements()
 * Calls m2ap_dump_mbms
 **/
bool m2ap_dump_mbms_hash_cb (const hash_key_t keyP,
               void * const mbms_void,
               void *unused_parameterP,
               void **unused_resultP);

 /** \brief Dump MBMS related information.
 * \param mbms_ref MBMS structure reference to dump
 **/
void m2ap_dump_mbms(const mbms_description_t * const mbms_ref);

bool m2ap_enb_compare_by_enb_id_cb (const hash_key_t keyP,
                                    void * const elementP, void * parameterP, void __attribute__((unused)) **unused_resultP);

/** \brief Remove target MBMS from the list
 * \param mbms_ref MBMS structure reference to remove
 **/
void m2ap_remove_mbms(mbms_description_t *mbms_ref);

#endif /* FILE_M2AP_MCE_SEEN */

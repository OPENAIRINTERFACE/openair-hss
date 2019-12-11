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

#ifndef FILE_M2AP_MCE_SEEN
#define FILE_M2AP_MCE_SEEN

#if MME_CLIENT_TEST == 0
# include "intertask_interface.h"
#endif

#include "hashtable.h"
#include "m2ap_common.h"
#include "mme_app_bearer_context.h"

// Forward declarations
struct m2ap_enb_description_s;

#define M2AP_TIMER_INACTIVE_ID   (-1)
#define M2AP_MBMS_CONTEXT_REL_COMP_TIMER 1 // in seconds

#define M2AP_ENB_SERVICE_SCTP_STREAM_ID         0
#define MBMS_SERVICE_SCTP_STREAM_ID          	1

/* Timer structure */
struct m2ap_timer_t {
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

/** Main structure representing MBMS Bearer service association over m2ap per eNB.
 * Created upon a successful
 **/
typedef struct mbms_description_s {
  mce_mbms_m2ap_id_t 			mce_mbms_m2ap_id:24;    ///< Unique MBMS id over MCE (24 bits wide)
  /** List of SCTP associations pointing to the eNBs. */
  hash_table_uint64_ts_t	g_m2ap_assoc_id2mce_enb_id_coll; // key is enb_mbms_m2ap_id, key is sctp association id;

  /** MBMS Parameters. */
  tmgi_t					    tmgi;
  mbms_service_area_id_t		mbms_service_area_id;

  /** SCTP stream on which M2 message will be sent/received.
   *  During an MBMS M2 connection, a pair of streams is
   *  allocated and is used during all the connection.
   *  Stream 0 is reserved for non MBMS signalling.
   *  @name sctp stream identifier
   **/

  /*@{*/
//  sctp_stream_id_t sctp_stream_recv; ///< eNB -> MCE stream
//  sctp_stream_id_t sctp_stream_send; ///< MCE -> eNB stream
//  /*@}*/
//

  /** MBMS Bearer. */
  struct mbms_bearer_context_s		mbms_bc;

  // MBMS Action timer
  struct m2ap_timer_t       m2ap_action_timer;
} mbms_description_t;

/*
 * Used to search by TMGI and MBMS SAI.
 */
typedef struct m2ap_tmgi_s{
  tmgi_t					tmgi;
  mbms_service_area_id_t 	mbms_service_area_id_t;
}m2ap_tmgi_t;

/* Main structure representing eNB association over m2ap
 * Generated (or updated) every time a new M2SetupRequest is received.
 */
typedef struct m2ap_enb_description_s {

  enum mce_m2_enb_state_s m2_state;         ///< State of the eNB specific M2AP association

  /** eNB related parameters **/
  /*@{*/
  char     				m2ap_enb_name[150];      ///< Printable eNB Name
  uint32_t 				m2ap_enb_id;             ///< Unique eNB ID
  /** Received MBMS SA list. */
  mbms_service_area_t   mbms_sa_list;      ///< Tracking Area Identifiers signaled by the eNB (for each cell - used for paging.).
  /** Configured MBSFN Area Id . */
  mbsfn_area_ids_t		  mbsfn_area_ids;

  uint8_t								local_mbms_area;
  /*@}*/

  /** MBMS Services for this eNB **/
  /*@{*/
  uint32_t nb_mbms_associated; ///< Number of NAS associated UE on this eNB
  /*@}*/

  /** SCTP stuff **/
  /*@{*/
  sctp_assoc_id_t  sctp_assoc_id;    ///< SCTP association id on this machine
  sctp_stream_id_t instreams;        ///< Number of streams avalaible on eNB -> MCE
  sctp_stream_id_t outstreams;       ///< Number of streams avalaible on MCE -> eNB
  /*@}*/
} m2ap_enb_description_t;

extern uint32_t         nb_enb_associated;
extern struct mme_config_s    *global_mme_config_p;

/** \brief M2AP layer top init
 * @returns -1 in case of failure
 **/
int m2ap_mce_init(void);

/** \brief M2AP layer top exit
 **/
void m2ap_mce_exit (void);

/** \brief Look for given local MBMS area in the list.
 * \param local MBMS area is not unique and used for the search in the list.
 * @returns All matched M2AP eNBs in the m2ap_enb_list.
 **/
void m2ap_is_mbms_area_list (
  const uint8_t local_mbms_area,
  int *num_m2ap_enbs,
  m2ap_enb_description_t ** m2ap_enbs);

/** \brief Look for given MBMS Service Area Id in the list.
 * \param MBMS Service Area Id is not unique and used for the search in the list.
 * @returns All matched M2AP eNBs in the m2ap_enb_list.
 **/
void m2ap_is_mbms_sai_in_list (
  const mbms_service_area_id_t mbms_sai,
  int *num_m2ap_enbs,
	const m2ap_enb_description_t ** m2ap_enbs);

/** \brief Look for given MBMS Service Area Id in the list.
 * \param tac MBMS Service Area Id is not unique and used for the search in the list.
 * @returns All M2AP eNBs in the m2ap_enb_list, which don't support the given MBMS SAI.
 **/
void m2ap_is_mbms_sai_not_in_list (
  const mbms_service_area_id_t mbms_sai,
  int *num_m2ap_enbs,
  m2ap_enb_description_t ** m2ap_enbs);

/** \brief Look for given eNB SCTP assoc id in the list
 * \param enb_id The unique sctp assoc id to search in list
 * @returns NULL if no eNB matchs the sctp assoc id, or reference to the eNB element in list if matches
 **/
m2ap_enb_description_t* m2ap_is_enb_assoc_id_in_list(const sctp_assoc_id_t sctp_assoc_id);

/** \brief Look for given mbms mce id in the list
 * \param mbms_mce_id The unique mbms_mce_id to search in list
 * @returns NULL if no MBMS matchs the mbms_mce_id, or reference to the mbms element in list if matches
 **/
mbms_description_t* m2ap_is_mbms_mce_m2ap_id_in_list(const mce_mbms_m2ap_id_t mbms_mce_id);

/** \brief Perform an eNB reset (cleaning SCTP associations in the MBMS Service References) without removing the M2AP eNB.
 * \param m2ap_enb_ref : eNB to reset fully
 **/
void m2ap_enb_full_reset (const void *m2ap_enb_ref_p);

/** \brief Look for given mbms via TMGI is in the list
 * \param tmgi
 * \param mbms_sai
 * @returns NULL if no MBMS matchs the mbms_mce_id, or reference to the mbms element in list if matches
 **/
mbms_description_t* m2ap_is_mbms_tmgi_in_list (
  const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_sai);

///** \brief associate mainly 2(3) identifiers in M2AP layer: {mme_mbms_m2ap_id_t, sctp_assoc_id (,enb_mbms_m2ap_id)}
// **/
//void m2ap_notified_new_mbms_mce_m2ap_id_association (
//    const sctp_assoc_id_t  sctp_assoc_id,
//    const enb_mbms_m2ap_id_t enb_mbms_m2ap_id,
//    const mce_mbms_m2ap_id_t mme_mbms_m2ap_id);

/** \brief Allocate and add to the list a new eNB descriptor
 * @returns Reference to the new eNB element in list
 **/
m2ap_enb_description_t* m2ap_new_enb(void);

/** \brief Allocate a new MBMS Service Description. Will allocate a new MCE MBMS M2AP Id (24).
 * \param tmgi_t
 * \param mbms_service_are_id
 * @returns Reference to the new MBMS Service element in list
 **/
mbms_description_t* m2ap_new_mbms(const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_sai);

//------------------------------------------------------------------------------
void
m2ap_set_embms_cfg_item (m2ap_enb_description_t * const m2ap_enb_ref,
	M2AP_MBMS_Service_Area_ID_List_t * mbms_service_areas);

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
void m2ap_dump_enb(const m2ap_enb_description_t * const m2ap_enb_ref);

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

bool m2ap_enb_compare_by_m2ap_enb_id_cb (const hash_key_t keyP,
      void * const elementP, void * parameterP, void __attribute__((unused)) **unused_resultP);

m2ap_enb_description_t                      *
m2ap_is_enb_id_in_list ( const uint32_t m2ap_enb_id);

#endif /* FILE_M2AP_MCE_SEEN */

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

/*! \file m3ap_message_types.h
* \brief
* \author Dincer Beken
* \company Blackned GmbH
* \email: dbeken@blackned.de
*
*/

#ifndef FILE_M3AP_MESSAGES_TYPES_SEEN
#define FILE_M3AP_MESSAGES_TYPES_SEEN

#define M3AP_MBMS_SESSION_START_REQUEST(mSGpTR)                  (mSGpTR)->ittiMsg.m3ap_mbms_session_start_req
#define M3AP_MBMS_SESSION_UPDATE_REQUEST(mSGpTR)                 (mSGpTR)->ittiMsg.m3ap_mbms_session_update_req
#define M3AP_MBMS_SESSION_STOP_REQUEST(mSGpTR)                   (mSGpTR)->ittiMsg.m3ap_mbms_session_stop_req

#define M3AP_ERROR_INDICATION(mSGpTR)                			 (mSGpTR)->ittiMsg.m3ap_error_ind
#define MCE_APP_M3_MBMS_SERVICE_COUNTING_REQ(mSGpTR)			 (mSGpTR)->ittiMsg.m3ap_mbms_service_counting_req
#define M3AP_ENB_INITIATED_RESET_REQ(mSGpTR)                	 (mSGpTR)->ittiMsg.m3ap_initiated_reset_req
#define M3AP_ENB_INITIATED_RESET_ACK(mSGpTR)                	 (mSGpTR)->ittiMsg.m3ap_enb_initiated_reset_ack

// List of possible causes for MME generated UE context release command towards eNB
enum m2cause {
  M2AP_INVALID_CAUSE = 0,
  M2AP_NAS_NORMAL_RELEASE,
  M2AP_NAS_DETACH,
  M2AP_RADIO_EUTRAN_GENERATED_REASON,
  M2AP_IMPLICIT_CONTEXT_RELEASE,
  M2AP_INITIAL_CONTEXT_SETUP_FAILED,
  M2AP_SCTP_SHUTDOWN_OR_RESET,

  M2AP_HANDOVER_CANCELLED,
  M2AP_HANDOVER_FAILED,
  M2AP_NETWORK_ERROR,
  M2AP_SYSTEM_FAILURE,

  M2AP_INVALIDATE_NAS,  /**< Removing the NAS layer only. */

  M2AP_SUCCESS
};

typedef enum m2ap_reset_type_e {
  M2AP_RESET_ALL = 0,
  M2AP_RESET_PARTIAL
} m2ap_reset_type_t;

typedef struct m2_sig_conn_id_s {
  mce_mbms_m2ap_id_t*  mme_mbms_m2ap_id;
  enb_mbms_m2ap_id_t*  enb_mbms_m2ap_id;
} m2_sig_conn_id_t;

//-----------------
typedef struct mbms_bearer_context_to_be_created_s {
  struct bearer_context_to_be_created_s 		bc_tbc;
  struct mbms_ip_multicast_distribution_s 		mbms_ip_mc_dist;
} mbms_bearer_context_to_be_created_t;

//------------------------------------------------------------------------------
typedef struct itti_m3ap_mbms_session_start_req_s {
  tmgi_t									tmgi;
  mbms_service_area_id_t					mbms_service_area_id;
  mbms_bearer_context_to_be_created_t	    mbms_bearer_tbc;
  // todo: handle timer better --> Removal of current time should be done in M2AP layer
  uint32_t									time_to_start_in_sec;  /**< Will be handled in the MCE_APP layer. */
  uint32_t									time_to_start_in_usec;  /**< Will be handled in the MCE_APP layer. */
} itti_m3ap_mbms_session_start_req_t;

//------------------------------------------------------------------------------
typedef struct itti_m3ap_mbms_session_update_req_s {
  tmgi_t									tmgi;
  mbms_service_area_id_t					new_mbms_service_area_id;
  mbms_service_area_id_t					old_mbms_service_area_id;
  mbms_bearer_context_to_be_created_t	    mbms_bearer_tbc;
  uint32_t									time_to_update_in_sec;  /**< Will be handled in the MCE_APP layer. */
} itti_m3ap_mbms_session_update_req_t;

//------------------------------------------------------------------------------
typedef struct itti_m3ap_mbms_session_stop_req_s {
  tmgi_t 				    		tmgi;           //< MBMS Service TMGI.
  mbms_service_area_id_t			mbms_service_area_id;
  bool								inform_enbs;
} itti_m3ap_mbms_session_stop_req_t;

//------------------------------------------------------------------------------
typedef struct itti_m3ap_enb_initiated_reset_req_s {
  uint32_t          sctp_assoc_id;
  uint16_t          sctp_stream_id;
  uint32_t          enb_id;
  s1ap_reset_type_t  s1ap_reset_type;
  uint32_t          num_mbms;
  s1_sig_conn_id_t  *ue_to_reset_list;
} itti_m3ap_enb_initiated_reset_req_t;

//------------------------------------------------------------------------------
typedef struct itti_m3ap_enb_initiated_reset_ack_s {
  uint32_t          sctp_assoc_id;
  uint16_t          sctp_stream_id;
  m2ap_reset_type_t m2ap_reset_type;
  uint32_t          num_mbms;
  m2_sig_conn_id_t  *mbsm_to_reset_list;
} itti_m3ap_enb_initiated_reset_ack_t;

/** M3AP Error Indication. */
typedef struct itti_m3ap_error_ind_s {
  mce_mbms_m2ap_id_t      mce_mbms_m2ap_id:24;
  enb_mbms_m2ap_id_t      enb_mbms_m2ap_id;
  sctp_assoc_id_t         assoc_id;
  uint32_t                enb_id;
  enum m2cause            cause;
}itti_m3ap_error_ind_t;

/** M3AP MBMS Service Counting Request. */
typedef struct itti_m3ap_mbms_service_counting_req_s {
  mce_mbms_m2ap_id_t      mce_mbms_m2ap_id:24;
  enb_mbms_m2ap_id_t      enb_mbms_m2ap_id;
  sctp_assoc_id_t         assoc_id;
  uint32_t                enb_id;
  enum m2cause            cause;
}itti_m3ap_mbms_service_counting_req_t;


#endif /* FILE_M3AP_MESSAGES_TYPES_SEEN */

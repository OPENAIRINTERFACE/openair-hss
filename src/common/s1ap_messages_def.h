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

//WARNING: Do not include this header directly. Use intertask_interface.h instead.

/*! \file s1ap_messages_def.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

/* Messages for S1AP logging */
MESSAGE_DEF(S1AP_UPLINK_NAS_LOG            , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_uplink_nas_log)
MESSAGE_DEF(S1AP_UE_CAPABILITY_IND_LOG     , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_ue_capability_ind_log)
MESSAGE_DEF(S1AP_INITIAL_CONTEXT_SETUP_LOG , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_initial_context_setup_log)
MESSAGE_DEF(S1AP_NAS_NON_DELIVERY_IND_LOG  , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_nas_non_delivery_ind_log)
MESSAGE_DEF(S1AP_DOWNLINK_NAS_LOG          , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_downlink_nas_log)
MESSAGE_DEF(S1AP_S1_SETUP_LOG              , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_s1_setup_log)
MESSAGE_DEF(S1AP_S1_SETUP_FAILURE_LOG      , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_s1_setup_failure_log)
MESSAGE_DEF(S1AP_INITIAL_UE_MESSAGE_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_initial_ue_message_log)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_REQ_LOG, MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_ue_context_release_req_log)
MESSAGE_DEF(S1AP_INITIAL_CONTEXT_SETUP_FAILURE_LOG, MESSAGE_PRIORITY_MED, IttiMsgText               , s1ap_initial_context_setup_failure_log)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_COMMAND_LOG, MESSAGE_PRIORITY_MED, IttiMsgText                  , s1ap_ue_context_release_command_log)
MESSAGE_DEF(S1AP_PATH_SWITCH_REQUEST_LOG   , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_path_switch_request_log)
MESSAGE_DEF(S1AP_PATH_SWITCH_ACK_LOG       , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_path_switch_ack_log)
MESSAGE_DEF(S1AP_PATH_SWITCH_FAILURE_LOG   , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_path_switch_failure_log)
MESSAGE_DEF(S1AP_HANDOVER_REQUIRED_LOG     , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_handover_required_log)
MESSAGE_DEF(S1AP_HANDOVER_REQUEST_LOG      , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_handover_request_log)
MESSAGE_DEF(S1AP_HANDOVER_COMMAND_LOG      , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_handover_command_log)
MESSAGE_DEF(S1AP_HANDOVER_CANCEL_LOG       , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_handover_cancel_log)
MESSAGE_DEF(S1AP_HANDOVER_CANCEL_ACK_LOG   , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_handover_cancel_ack_log)
MESSAGE_DEF(S1AP_ENB_STATUS_TRANSFER_LOG   , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_enb_status_transfer_log)
MESSAGE_DEF(S1AP_MME_STATUS_TRANSFER_LOG   , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_mme_status_transfer_log)
MESSAGE_DEF(S1AP_HANDOVER_NOTIFY_LOG       , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_handover_notify_log)
MESSAGE_DEF(S1AP_HANDOVER_REQUEST_ACKNOWLEDGE_LOG       , MESSAGE_PRIORITY_MED, IttiMsgText         , s1ap_handover_request_acknowledge_log)
MESSAGE_DEF(S1AP_HANDOVER_FAILURE_LOG      , MESSAGE_PRIORITY_MED, IttiMsgText         , s1ap_handover_failure_log)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_ue_context_release_log)

MESSAGE_DEF(S1AP_E_RABSETUP_LOG             , MESSAGE_PRIORITY_MED, IttiMsgText                     , s1ap_e_rabsetup_log)
MESSAGE_DEF(S1AP_E_RABMODIFY_LOG            , MESSAGE_PRIORITY_MED, IttiMsgText                     , s1ap_e_rabmodify_log)
MESSAGE_DEF(S1AP_E_RABRELEASE_LOG           , MESSAGE_PRIORITY_MED, IttiMsgText                     , s1ap_e_rabrelease_log)
MESSAGE_DEF(S1AP_E_RABRELEASE_IND_LOG       , MESSAGE_PRIORITY_MED, IttiMsgText                     , s1ap_e_rabrelease_ind_log)

MESSAGE_DEF(S1AP_E_RABSETUP_RESPONSE_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                     , s1ap_e_rabsetup_response_log)
MESSAGE_DEF(S1AP_E_RABMODIFY_RESPONSE_LOG   , MESSAGE_PRIORITY_MED, IttiMsgText                    , s1ap_e_rabmodify_response_log)
MESSAGE_DEF(S1AP_E_RABRELEASE_RESPONSE_LOG  , MESSAGE_PRIORITY_MED, IttiMsgText                     , s1ap_e_rabrelease_response_log)
MESSAGE_DEF(S1AP_PAGING_LOG                 , MESSAGE_PRIORITY_MED, IttiMsgText                     , s1ap_paging_log)

MESSAGE_DEF(S1AP_ENB_RESET_LOG             , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_enb_reset_log)

MESSAGE_DEF(S1AP_UE_CAPABILITIES_IND       ,  MESSAGE_PRIORITY_MED, itti_s1ap_ue_cap_ind_t                ,  s1ap_ue_cap_ind)
MESSAGE_DEF(S1AP_ENB_DEREGISTERED_IND      ,  MESSAGE_PRIORITY_MED, itti_s1ap_eNB_deregistered_ind_t      ,  s1ap_eNB_deregistered_ind)
MESSAGE_DEF(S1AP_DEREGISTER_UE_REQ         ,  MESSAGE_PRIORITY_MED, itti_s1ap_deregister_ue_req_t         ,  s1ap_deregister_ue_req)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_REQ    ,  MESSAGE_PRIORITY_MED, itti_s1ap_ue_context_release_req_t    ,  s1ap_ue_context_release_req)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_COMMAND,  MESSAGE_PRIORITY_MED, itti_s1ap_ue_context_release_command_t,  s1ap_ue_context_release_command)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_COMPLETE, MESSAGE_PRIORITY_MED, itti_s1ap_ue_context_release_complete_t, s1ap_ue_context_release_complete)
MESSAGE_DEF(S1AP_INITIAL_UE_MESSAGE         , MESSAGE_PRIORITY_MED, itti_s1ap_initial_ue_message_t  ,        s1ap_initial_ue_message)
MESSAGE_DEF(S1AP_E_RAB_SETUP_REQ            , MESSAGE_PRIORITY_MED, itti_s1ap_e_rab_setup_req_t  ,           s1ap_e_rab_setup_req)
MESSAGE_DEF(S1AP_E_RAB_SETUP_RSP            , MESSAGE_PRIORITY_MED, itti_s1ap_e_rab_setup_rsp_t  ,           s1ap_e_rab_setup_rsp)
MESSAGE_DEF(S1AP_E_RAB_MODIFY_REQ           , MESSAGE_PRIORITY_MED, itti_s1ap_e_rab_modify_req_t  ,          s1ap_e_rab_modify_req)
MESSAGE_DEF(S1AP_E_RAB_MODIFY_RSP           , MESSAGE_PRIORITY_MED, itti_s1ap_e_rab_modify_rsp_t  ,          s1ap_e_rab_modify_rsp)
MESSAGE_DEF(S1AP_E_RAB_RELEASE_REQ          , MESSAGE_PRIORITY_MED, itti_s1ap_e_rab_release_req_t  ,         s1ap_e_rab_release_req)
MESSAGE_DEF(S1AP_E_RAB_RELEASE_RSP          , MESSAGE_PRIORITY_MED, itti_s1ap_e_rab_release_rsp_t  ,         s1ap_e_rab_release_rsp)
MESSAGE_DEF(S1AP_E_RAB_RELEASE_IND          , MESSAGE_PRIORITY_MED, itti_s1ap_e_rab_release_ind_t  ,         s1ap_e_rab_release_ind)

MESSAGE_DEF(S1AP_ERROR_INDICATION           , MESSAGE_PRIORITY_MED, itti_s1ap_error_indication_t         ,  s1ap_error_indication)

MESSAGE_DEF(S1AP_ENB_INITIATED_RESET_REQ   ,  MESSAGE_PRIORITY_MED, itti_s1ap_enb_initiated_reset_req_t   ,  s1ap_enb_initiated_reset_req)
MESSAGE_DEF(S1AP_ENB_INITIATED_RESET_ACK   ,  MESSAGE_PRIORITY_MED, itti_s1ap_enb_initiated_reset_ack_t   ,  s1ap_enb_initiated_reset_ack)

/** Path Switch. */
MESSAGE_DEF(S1AP_PATH_SWITCH_REQUEST       , MESSAGE_PRIORITY_MED, itti_s1ap_path_switch_request_t    ,    s1ap_path_switch_request)
MESSAGE_DEF(S1AP_PATH_SWITCH_REQUEST_FAILURE, MESSAGE_PRIORITY_MED, itti_s1ap_path_switch_request_failure_t,    s1ap_path_switch_request_failure)
MESSAGE_DEF(S1AP_PATH_SWITCH_REQUEST_ACKNOWLEDGE, MESSAGE_PRIORITY_MED, itti_s1ap_path_switch_request_ack_t  ,    s1ap_path_switch_request_ack)


/** Handover Required. */
MESSAGE_DEF(S1AP_HANDOVER_REQUIRED         , MESSAGE_PRIORITY_MED, itti_s1ap_handover_required_t        , s1ap_handover_required)


/** Handover Preparation Failure. */
MESSAGE_DEF(S1AP_HANDOVER_PREPARATION_FAILURE, MESSAGE_PRIORITY_MED, itti_s1ap_handover_preparation_failure_t,    s1ap_handover_preparation_failure)
/** Handover Cancel. */
MESSAGE_DEF(S1AP_HANDOVER_CANCEL             , MESSAGE_PRIORITY_MED, itti_s1ap_handover_cancel_t         , s1ap_handover_cancel)
MESSAGE_DEF(S1AP_HANDOVER_CANCEL_ACKNOWLEDGE , MESSAGE_PRIORITY_MED, itti_s1ap_handover_cancel_acknowledge_t         , s1ap_handover_cancel_acknowledge)

/** Handover Request. */
MESSAGE_DEF(S1AP_HANDOVER_REQUEST          , MESSAGE_PRIORITY_MED, itti_s1ap_handover_request_t        , s1ap_handover_request)
/** Handover Command. */
MESSAGE_DEF(S1AP_HANDOVER_COMMAND          , MESSAGE_PRIORITY_MED, itti_s1ap_handover_command_t        , s1ap_handover_command)

/** Handover Request Acknowledge/Failure. */
MESSAGE_DEF(S1AP_HANDOVER_REQUEST_ACKNOWLEDGE  , MESSAGE_PRIORITY_MED, itti_s1ap_handover_request_acknowledge_t,    s1ap_handover_request_acknowledge)
MESSAGE_DEF(S1AP_HANDOVER_FAILURE              , MESSAGE_PRIORITY_MED, itti_s1ap_handover_failure_t    ,    s1ap_handover_failure)

/** eNB/MME status transfer. */
MESSAGE_DEF(S1AP_ENB_STATUS_TRANSFER       , MESSAGE_PRIORITY_MED, itti_s1ap_status_transfer_t      , s1ap_enb_status_transfer)
MESSAGE_DEF(S1AP_MME_STATUS_TRANSFER       , MESSAGE_PRIORITY_MED, itti_s1ap_status_transfer_t      , s1ap_mme_status_transfer)
/** Handover Notify. */
MESSAGE_DEF(S1AP_HANDOVER_NOTIFY           , MESSAGE_PRIORITY_MED, itti_s1ap_handover_notify_t      ,    s1ap_handover_notify)

/** Paging. */
MESSAGE_DEF(S1AP_PAGING                    , MESSAGE_PRIORITY_MED, itti_s1ap_paging_t               ,    s1ap_paging)

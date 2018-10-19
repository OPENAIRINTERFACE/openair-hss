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

#ifndef FILE_SGI_FORWARD_MESSAGES_TYPES_SEEN
#define FILE_SGI_FORWARD_MESSAGES_TYPES_SEEN

#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SGI_CREATE_ENDPOINT_REQUEST(mSGpTR)       ((itti_sgi_create_end_point_request_t*)(mSGpTR)->itti_msg)
#define SGI_CREATE_ENDPOINT_RESPONSE(mSGpTR)      ((itti_sgi_create_end_point_response_t*)(mSGpTR)->itti_msg)
#define SGI_UPDATE_ENDPOINT_REQUEST(mSGpTR)       ((itti_sgi_update_end_point_request_t*)(mSGpTR)->itti_msg)
#define SGI_UPDATE_ENDPOINT_RESPONSE(mSGpTR)      ((itti_sgi_update_end_point_response_t*)(mSGpTR)->itti_msg)
#define SGI_DELETE_ENDPOINT_REQUEST(mSGpTR)       ((itti_sgi_delete_end_point_request_t*)(mSGpTR)->itti_msg)
#define SGI_DELETE_ENDPOINT_RESPONSE(mSGpTR)      ((itti_sgi_delete_end_point_response_t*)(mSGpTR)->itti_msg)

typedef enum SGIStatus_e {
  SGI_STATUS_OK                                           = 16,
  SGI_STATUS_ERROR_CONTEXT_NOT_FOUND                      = 64,
  SGI_STATUS_ERROR_INVALID_MESSAGE_FORMAT                 = 65,
  SGI_STATUS_ERROR_SERVICE_NOT_SUPPORTED                  = 68,
  SGI_STATUS_ERROR_SYSTEM_FAILURE                         = 72,
  SGI_STATUS_ERROR_NO_RESOURCES_AVAILABLE                 = 73,
  SGI_STATUS_ERROR_ALL_DYNAMIC_ADDRESSES_OCCUPIED         = 84,
  SGI_STATUS_ERROR_NO_MEMORY_AVAILABLE                    = 91,
  SGI_STATUS_MAX,
} SGIStatus_t;


typedef struct {
  teid_t           context_teid;        ///< Tunnel Endpoint Identifier S11
  teid_t           sgw_S1u_teid;        ///< Tunnel Endpoint Identifier S1-U
  ebi_t            eps_bearer_id;       ///< EPS bearer identifier
  pdn_type_t       pdn_type;            ///< PDN Type
  paa_t            paa;                 ///< PDN Address Allocation
} itti_sgi_create_end_point_request_t;

typedef struct {
  SGIStatus_t      status;              ///< Status of  endpoint creation (Failed = 0xFF or Success = 0x0)
  teid_t           context_teid;        ///< Tunnel Endpoint Identifier S11
  teid_t           sgw_S1u_teid;        ///< Tunnel Endpoint Identifier S1-U
  ebi_t            eps_bearer_id;       ///< EPS bearer identifier
  pdn_type_t       pdn_type;            ///< PDN Type
  paa_t            paa;                 ///< PDN Address Allocation
  protocol_configuration_options_t       pco;                 ///< Protocol configuration options
} itti_sgi_create_end_point_response_t;

typedef struct {
  teid_t           context_teid;        ///< Tunnel Endpoint Identifier S11
  teid_t           sgw_S1u_teid;        ///< Tunnel Endpoint Identifier S1-U
  teid_t           enb_S1u_teid;        ///< Tunnel Endpoint Identifier S1-U
  ebi_t            eps_bearer_id;       ///< EPS bearer identifier
} itti_sgi_update_end_point_request_t;

typedef struct {
  SGIStatus_t      status;              ///< Status of  endpoint creation (Failed = 0xFF or Success = 0x0)
  teid_t           context_teid;        ///< Tunnel Endpoint Identifier S11
  teid_t           sgw_S1u_teid;        ///< Tunnel Endpoint Identifier S1-U
  teid_t           enb_S1u_teid;        ///< Tunnel Endpoint Identifier S1-U
  ebi_t            eps_bearer_id;       ///< EPS bearer identifier
} itti_sgi_update_end_point_response_t;


typedef struct {
  teid_t           context_teid;        ///< Tunnel Endpoint Identifier S11
  teid_t           sgw_S1u_teid;        ///< Tunnel Endpoint Identifier S1-U
  ebi_t            eps_bearer_id;       ///< EPS bearer identifier
  pdn_type_t       pdn_type;            ///< PDN Type
  paa_t            paa;                 ///< PDN Address Allocation
} itti_sgi_delete_end_point_request_t;

typedef struct {
  SGIStatus_t      status;              ///< Status of  endpoint deletion (Failed = 0xFF or Success = 0x0)
  teid_t           context_teid;        ///< Tunnel Endpoint Identifier S11
  teid_t           sgw_S1u_teid;        ///< Tunnel Endpoint Identifier S1-U
  ebi_t            eps_bearer_id;       ///< EPS bearer identifier
  pdn_type_t       pdn_type;            ///< PDN Type
  paa_t            paa;                 ///< PDN Address Allocation
} itti_sgi_delete_end_point_response_t;

#ifdef __cplusplus
}
#endif
#endif /* FILE_SGI_FORWARD_MESSAGES_TYPES_SEEN */

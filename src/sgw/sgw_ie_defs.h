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

/*! \file sgw_ie_defs.h
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_SGW_IE_DEFS_SEEN
#define FILE_SGW_IE_DEFS_SEEN
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t  DelayValue_t;
typedef uint32_t SequenceNumber_t;


/* 3GPP TS 29.274 Figure 8.12 */



typedef struct {
  uint32_t uplink_ambr;
  uint32_t downlink_ambr;
} AMBR_t;


#ifdef __cplusplus
}
#endif



#endif  /* FILE_SGW_IE_DEFS_SEEN */


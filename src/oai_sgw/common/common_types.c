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

/*! \file common_types.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "security_types.h"
#include "common_types.h"
#include "common_defs.h"
#include "3gpp_29.274.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Clear GUTI without free it */
void clear_guti(guti_t * const guti) {memset(guti, 0, sizeof(guti_t));guti->m_tmsi = INVALID_TMSI;}
/* Clear IMSI without free it */
void clear_imsi(imsi_t * const imsi){memset(imsi, 0, sizeof(imsi_t));}
/* Clear IMEI without free it */
void clear_imei(imei_t * const imei){memset(imei, 0, sizeof(imei_t));}
/* Clear IMEISV without free it */
void clear_imeisv(imeisv_t * const imeisv){memset(imeisv, 0, sizeof(imeisv_t));}





#ifdef __cplusplus
}
#endif


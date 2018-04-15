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

/*! \file gtpv2c_ie_formatter.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_GTPV2C_IE_FORMATTER_SEEN
#define FILE_GTPV2C_IE_FORMATTER_SEEN

#include <stdbool.h>

#include "NwTypes.h"
#include "NwGtpv2c.h"
#include "3gpp_23.003.h"
#include "3gpp_29.274.h"

/* Imsi Information Element
 * 3GPP TS.29.274 #8.3
 * NOTE: Imsi is TBCD coded
 * octet 5   | Number digit 2 | Number digit 1   |
 * octet n+4 | Number digit m | Number digit m-1 |
 */
nw_rc_t gtpv2c_imsi_ie_get(uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

int gtpv2c_imsi_ie_set(nw_gtpv2c_msg_handle_t *msg, const imsi_t *imsi);

/* Cause Information Element */
nw_rc_t gtpv2c_cause_ie_get(uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

int gtpv2c_cause_ie_set(nw_gtpv2c_msg_handle_t *msg, const gtpv2c_cause_t  *cause);

/* Fully Qualified TEID (F-TEID) Information Element */
int gtpv2c_fteid_ie_set (nw_gtpv2c_msg_handle_t * msg, const fteid_t * fteid, const uint8_t   instance);
nw_rc_t gtpv2c_fteid_ie_get(uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);


/* PDN Address Allocation Information Element */
nw_rc_t gtpv2c_paa_ie_get(
  uint8_t ieType, uint16_t ieLength, uint8_t ieInstance, uint8_t *ieValue, void *arg);

int gtpv2c_paa_ie_set(nw_gtpv2c_msg_handle_t *msg, const paa_t *paa);


#endif /* FILE_GTPV2C_IE_FORMATTER_SEEN */

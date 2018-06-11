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

/*! \file s11_ie_formatter.c
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include "../../gtpv2c_ie_formatter/shared/gtpv2c_ie_formatter.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <pthread.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "common_defs.h"
#include "gcc_diag.h"
#include "log.h"
#include "assertions.h"
#include "conversions.h"
#include "3gpp_33.401.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_24.007.h"
#include "3gpp_29.274.h"
#include "3gpp_36.413.h"
#include "NwGtpv2c.h"
#include "NwGtpv2cIe.h"
#include "NwGtpv2cMsg.h"
#include "NwGtpv2cMsgParser.h"
#include "security_types.h"
#include "common_types.h"
#include "sgw_ie_defs.h"
#include "PdnType.h"

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_imsi_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  imsi_t                                 *imsi = (imsi_t *) arg;
  uint8_t                                 decoded = 0;

  DevAssert (arg );
  DevAssert (ieLength <= IMSI_BCD_DIGITS_MAX);
  imsi->length = 0;
  while (decoded < ieLength) {
    uint8_t tmp = ieValue[decoded++];
    imsi->u.value[imsi->length++] = (tmp >> 4) | (tmp << 4);
  }
  for (int i = imsi->length; i < IMSI_BCD8_SIZE; i++) {
    imsi->u.value[i] = 0xff;
  }

  OAILOG_DEBUG (LOG_GTPV2C, "\t- IMSI (l=%d) %u%u%u%u%u%u%u%u%u%u%u%u%u%u%u\n",  imsi->length,
      imsi->u.num.digit1, imsi->u.num.digit2, imsi->u.num.digit3, imsi->u.num.digit4,
      imsi->u.num.digit5, imsi->u.num.digit6, imsi->u.num.digit7, imsi->u.num.digit8,
      imsi->u.num.digit9, imsi->u.num.digit10, imsi->u.num.digit11, imsi->u.num.digit12,
      imsi->u.num.digit13, imsi->u.num.digit14, imsi->u.num.digit15);
  return NW_OK;
}

//------------------------------------------------------------------------------
int
gtpv2c_imsi_ie_set (
  nw_gtpv2c_msg_handle_t * msg,
  const imsi_t * imsi)
{
  nw_rc_t                                   rc;
  imsi_t                                  imsi_nbo = {.length = 0, .u.value = {0}};

  DevAssert (msg );
  DevAssert (imsi );

//  memcpy(&imsi_nbo, imsi, sizeof (imsi_nbo));

//  hexa_to_ascii((uint8_t *)imsi_pP->u.value,
//                NAS_PDN_CONNECTIVITY_REQ(message_p).imsi,
//                8);

//  NAS_PDN_CONNECTIVITY_REQ(message_p).pti             = ptiP;
//  NAS_PDN_CONNECTIVITY_REQ(message_p).ue_id           = ue_idP;
//
//
//  NAS_PDN_CONNECTIVITY_REQ(message_p).imsi[15]        = '\0';
//
//  if (isdigit(NAS_PDN_CONNECTIVITY_REQ(message_p).imsi[14])) {
//    NAS_PDN_CONNECTIVITY_REQ(message_p).imsi_length = 15;
//  } else {
//    NAS_PDN_CONNECTIVITY_REQ(message_p).imsi_length = 14;
//    NAS_PDN_CONNECTIVITY_REQ(message_p).imsi[14] = '\0';
//  }

  for (int i = 0; i < IMSI_BCD8_SIZE; i++) {
    uint8_t tmp = imsi->u.value[i];
    imsi_nbo.u.value[i] = (tmp >> 4) | (tmp << 4);
  }
  imsi_nbo.length = imsi->length;

//  uint8_t digits[IMSI_BCD_DIGITS_MAX+1];
//  int j = 0;
//  for (int i = 0; i < imsi->length; i++) {
//    if ((9 >= (imsi->u.value[i] & 0x0F)) && (j < IMSI_BCD_DIGITS_MAX)){
//      imsi_nbo.u.value[j++]=imsi_nbo.u.value[i] & 0x0F;
//    }
//    if ((0x90 >= (imsi_nbo.u.value[i] & 0xF0)) && (j < IMSI_BCD_DIGITS_MAX)){
//      imsi_nbo.u.value[j++]=(imsi_nbo.u.value[i] & 0xF0) >> 4;
//    }
//  }
//    hexa_to_ascii((uint8_t *)imsi->u.value,
//        imsi_nbo.u.value,
//        8);

  rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_IMSI, imsi_nbo.length, 0, (uint8_t*)imsi_nbo.u.value);
  DevAssert (NW_OK == rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_cause_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  gtpv2c_cause_t                             *cause = (gtpv2c_cause_t *) arg;

  DevAssert (cause );
  cause->cause_value = ieValue[0];
  cause->cs          = ieValue[1] & 0x01;
  cause->bce         = (ieValue[1] & 0x02) >> 1;
  cause->pce         = (ieValue[1] & 0x04) >> 2;
  if (6 == ieLength) {
    cause->offending_ie_type     = ieValue[2];
    cause->offending_ie_length   = ((uint16_t)ieValue[3]) << 8;
    cause->offending_ie_length  |= ((uint16_t)ieValue[4]);
    cause->offending_ie_instance = ieValue[5] & 0x0F;
  }
  OAILOG_DEBUG (LOG_GTPV2C, "\t- Cause value %u\n", cause->cause_value);
  return NW_OK;
}

//------------------------------------------------------------------------------
int
gtpv2c_cause_ie_set (
  nw_gtpv2c_msg_handle_t * msg,
  const gtpv2c_cause_t * cause)
{
  nw_rc_t                                   rc;
  uint8_t                                 value[6];

  DevAssert (msg );
  DevAssert (cause );
  value[0] = cause->cause_value;
  value[1] = ((cause->pce & 0x1) << 2) | ((cause->bce & 0x1) << 1) | (cause->cs & 0x1);

  if (cause->offending_ie_type ) {
    value[2] = cause->offending_ie_type;
    value[3] = (cause->offending_ie_length & 0xFF00) >> 8;
    value[4] = cause->offending_ie_length & 0x00FF;
    value[5] = cause->offending_ie_instance & 0x0F;
    rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_CAUSE, 6, 0, value);
  } else {
    rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_CAUSE, 2, 0, value);
  }

  DevAssert (NW_OK == rc);
  return rc == NW_OK ? 0 : -1;
}


//------------------------------------------------------------------------------
int
gtpv2c_fteid_ie_set (
  nw_gtpv2c_msg_handle_t * msg,
  const fteid_t * fteid,
  const uint8_t   instance)
{
  nw_rc_t                                   rc;
  uint8_t                                 value[25];

  DevAssert (msg );
  DevAssert (fteid );
  /*
   * MCC Decimal | MCC Hundreds
   */
  value[0] = (fteid->ipv4 << 7) | (fteid->ipv6 << 6) | (fteid->interface_type & 0x3F);
  value[1] = (fteid->teid >> 24 );
  value[2] = (fteid->teid >> 16 ) & 0xFF;
  value[3] = (fteid->teid >>  8 ) & 0xFF;
  value[4] = (fteid->teid >>  0 ) & 0xFF;

  int offset = 5;
  if (fteid->ipv4 == 1) {
    uint32_t hbo = ntohl(fteid->ipv4_address.s_addr);
    value[offset++] = (uint8_t)(hbo >> 24);
    value[offset++] = (uint8_t)(hbo >> 16);
    value[offset++] = (uint8_t)(hbo >> 8);
    value[offset++] = (uint8_t)hbo;
  }
  if (fteid->ipv6 == 1) {
    /*
     * IPv6 present: copy the 16 bytes
     */
    memcpy (&value[offset], fteid->ipv6_address.__in6_u.__u6_addr8, 16);
    offset += 16;
  }

  rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_FTEID, offset, instance, value);
  DevAssert (NW_OK == rc);
  return RETURNok;
}


//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_fteid_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  uint8_t                                 offset = 0;
   fteid_t                                *fteid = (fteid_t *) arg;

   DevAssert (fteid );
   fteid->ipv4 = (ieValue[0] & 0x80) >> 7;
   fteid->ipv6 = (ieValue[0] & 0x40) >> 6;
   fteid->interface_type = ieValue[0] & 0x1F;
   OAILOG_DEBUG (LOG_GTPV2C, "\t- F-TEID type %d\n", fteid->interface_type);
   /*
    * Copy the TEID or GRE key
    */
   fteid->teid = ntoh_int32_buf (&ieValue[1]);
   OAILOG_DEBUG (LOG_GTPV2C, "\t- TEID/GRE    %08x\n", fteid->teid);

   if (fteid->ipv4 == 1) {
     /*
      * IPv4 present: copy the 4 bytes
      */
     uint32_t hbo = (((uint32_t)ieValue[5]) << 24) |
                    (((uint32_t)ieValue[6]) << 16) |
                    (((uint32_t)ieValue[7]) << 8) |
                    (uint32_t)ieValue[8];
     fteid->ipv4_address.s_addr = htonl(hbo);
     offset = 4;
     OAILOG_DEBUG (LOG_GTPV2C, "\t- IPv4 addr   " IN_ADDR_FMT "\n", PRI_IN_ADDR (fteid->ipv4_address));
   }

   if (fteid->ipv6 == 1) {
     char                                    ipv6_ascii[INET6_ADDRSTRLEN];

     /*
      * IPv6 present: copy the 16 bytes
      * * * * WARNING: if Ipv4 is present, 4 bytes of offset should be applied
      */
     memcpy (fteid->ipv6_address.__in6_u.__u6_addr8, &ieValue[5 + offset], 16);
     inet_ntop (AF_INET6, (void*)&fteid->ipv6_address, ipv6_ascii, INET6_ADDRSTRLEN);
     OAILOG_DEBUG (LOG_GTPV2C, "\t- IPv6 addr   %s\n", ipv6_ascii);
   }

   return NW_OK;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_paa_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  uint8_t                                 offset = 0;
  paa_t                                  *paa = (paa_t *) arg;

  DevAssert (paa );
  paa->pdn_type = ieValue[0] & 0x07;
  OAILOG_DEBUG (LOG_S11, "\t- PAA type  %d\n", paa->pdn_type);

  if (paa->pdn_type & 0x2) {
    char                                    ipv6_ascii[INET6_ADDRSTRLEN];

    /*
     * IPv6 present: copy the 16 bytes
     * * * * WARNING: if both ipv4 and ipv6 are present,
     * * * *          17 bytes of offset should be applied for ipv4
     * * * * NOTE: an ipv6 prefix length is prepend
     * * * * NOTE: in Rel.8 the prefix length has a default value of /64
     */
    paa->ipv6_prefix_length = ieValue[1];

    memcpy (paa->ipv6_address.__in6_u.__u6_addr8, &ieValue[2], 16);
    inet_ntop (AF_INET6, &paa->ipv6_address, ipv6_ascii, INET6_ADDRSTRLEN);
    OAILOG_DEBUG (LOG_S11, "\t- IPv6 addr %s/%u\n", ipv6_ascii, paa->ipv6_prefix_length);
  }

  if (paa->pdn_type == 3) {
    offset = 17;
  }

  if (paa->pdn_type & 0x1) {
    uint32_t ip = (((uint32_t)ieValue[1 + offset]) << 24) |
                  (((uint32_t)ieValue[2 + offset]) << 16) |
                  (((uint32_t)ieValue[3 + offset]) << 8) |
                   ((uint32_t)ieValue[4 + offset]);

    paa->ipv4_address.s_addr = htonl(ip);
    char ipv4[INET_ADDRSTRLEN];
    inet_ntop (AF_INET, (void*)&paa->ipv4_address, ipv4, INET_ADDRSTRLEN);
    OAILOG_DEBUG (LOG_S11, "\t- IPv4 addr %s\n", ipv4);
  }

  paa->pdn_type -= 1;
  return NW_OK;
}

//------------------------------------------------------------------------------
int
gtpv2c_paa_ie_set (
  nw_gtpv2c_msg_handle_t * msg,
  const paa_t * paa)
{
  /*
   * ipv4 address = 4 + ipv6 address = 16 + ipv6 prefix length = 1
   * * * * + pdn_type = 1
   * * * * = maximum of 22 bytes
   */
  uint8_t                                 temp[22];
  uint8_t                                 pdn_type;
  uint8_t                                 offset = 0;
  nw_rc_t                                   rc;

  DevAssert (paa );
  pdn_type = paa->pdn_type + 1;
  temp[offset] = pdn_type;
  offset++;

  if (pdn_type & 0x2) {
    /*
     * If ipv6 or ipv4v6 present
     */
    temp[1] = paa->ipv6_prefix_length;
    memcpy (&temp[2], paa->ipv6_address.__in6_u.__u6_addr8, 16);
    offset += 17;
  }

  if (pdn_type & 0x1) {
    uint32_t hbo = ntohl(paa->ipv4_address.s_addr);
    temp[offset++] = (uint8_t)(hbo >> 24);
    temp[offset++] = (uint8_t)(hbo >> 16);
    temp[offset++] = (uint8_t)(hbo >> 8);
    temp[offset++] = (uint8_t)hbo;
  }

  rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_PAA, offset, 0, temp);
  DevAssert (NW_OK == rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_ambr_ie_set (
    nw_gtpv2c_msg_handle_t * msg, ambr_t * ambr)
{
  nw_rc_t                                   rc;

  DevAssert (msg );
  DevAssert (ambr );

  uint8_t                                 ambr_br[16];
  uint8_t                                 *p_ambr;
  p_ambr = ambr_br;

  memset(ambr_br, 0, 16);

  INT64_TO_BUFFER(ambr->br_ul, p_ambr);
  p_ambr+=8;

  INT64_TO_BUFFER(ambr->br_dl, p_ambr);
  // todo: byte order?

  rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_AMBR, 16, 0, p_ambr);
  DevAssert (NW_OK == rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_ambr_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  ambr_t                                 *ambr = (ambr_t *) arg;

  DevAssert (ambr );
  ambr->br_ul = ntoh_int32_buf (&ieValue[0]);
  ambr->br_dl = ntoh_int32_buf (&ieValue[4]);
  OAILOG_DEBUG (LOG_S11, "\t- AMBR UL %" PRIu64 "\n", ambr->br_ul);
  OAILOG_DEBUG (LOG_S11, "\t- AMBR DL %" PRIu64 "\n", ambr->br_dl);
  return NW_OK;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_arp_ie_set (
    nw_gtpv2c_msg_handle_t * msg, arp_t * arp)
{
  DevAssert (msg );
  DevAssert (arp );
  uint8_t  arp8 = ((arp->pre_emp_capability & 0x01) << 6) | ((arp->priority_level & 0x0f) << 6) | (arp->pre_emp_vulnerability & 0x01);

  nw_rc_t rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_ARP, 1, 0, &arp8);
  DevAssert (NW_OK == rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_arp_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  arp_t                                 *arp = (arp_t *) arg;

  DevAssert (arp);
  arp->pre_emp_capability = (ieValue[0] & 0x40) >> 6;
  arp->priority_level     = (ieValue[0] & 0x3C) >> 2;
  arp->pre_emp_vulnerability     = ieValue[0] & 0x01;
  OAILOG_DEBUG (LOG_S11, "\t- ARP PCI  %d\n", arp->pre_emp_capability);
  OAILOG_DEBUG (LOG_S11, "\t- ARP PL   %d\n", arp->priority_level);
  OAILOG_DEBUG (LOG_S11, "\t- ARP PVI  %d\n", arp->pre_emp_vulnerability);
  return NW_OK;
}


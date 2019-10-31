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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <pthread.h>

#include "bstrlib.h"

//#include "intertask_interface.h"

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
#include "NwGtpv2c.h"
#include "NwGtpv2cIe.h"
#include "NwGtpv2cMsg.h"
#include "NwGtpv2cMsgParser.h"
#include "common_types.h"
#include "mme_ie_defs.h"
#include "mme_config.h"
#include "sm_common.h"
#include "sm_ie_formatter.h"

nw_rc_t
sm_tmgi_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  tmgi_t                *tmgi 	 = (tmgi_t*) arg;
  uint8_t                decoded = 0;

  DevAssert (tmgi);

  /** Set the MBMS Service ID. */
  DECODE_U24(ieValue, tmgi->mbms_service_id, decoded);
  ieValue+=decoded;
  OAILOG_DEBUG (LOG_SM, "\t- MBMS Service ID %d\n", tmgi->mbms_service_id);
  /** Convert to TBCD and add to TMGI. */
  tmgi->plmn.mcc_digit2 = (*ieValue >> 4) & 0xf;
  tmgi->plmn.mcc_digit1 = *ieValue & 0xf;
  ieValue++;
  tmgi->plmn.mnc_digit3 = (*ieValue >> 4) & 0xf;
  tmgi->plmn.mcc_digit3 = *ieValue & 0xf;
  ieValue++;
  tmgi->plmn.mnc_digit2 = (*ieValue >> 4) & 0xf;
  tmgi->plmn.mnc_digit2 = *ieValue & 0xf;
  ieValue++;
  return NW_OK;
}

nw_rc_t
sm_mbms_session_duration_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  DevAssert (arg );
  mbms_session_duration_t * msd = (mbms_session_duration_t *) arg;
  msd->seconds = (ieValue[0] << 17) + (ieValue[1] << 9) + (ieValue[2] & 0x80);
  msd->days = (ieValue[3] & 0x7F);
  ieValue+=3;
  return NW_OK;
}

nw_rc_t
sm_mbms_service_area_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  mbms_service_area_t                             *mbms_service_area = (mbms_service_area_t *) arg;

  DevAssert (mbms_service_area);

  mbms_service_area->num_service_area = *ieValue;
  ieValue++;
  int decoded = 0;
  while (decoded < mbms_service_area->num_service_area){
	  mbms_service_area->serviceArea[decoded] = *(uint16_t*)ieValue;
	  ieValue+=2;
  }

  return NW_OK;
}

nw_rc_t
sm_mbms_flow_identifier_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  uint16_t                                *flow_id = (uint16_t *) arg;

  DevAssert (flow_id);
  *flow_id = *(uint16_t*)ieValue;
  ieValue+=2;
  OAILOG_DEBUG (LOG_SM, "\t- Flow-ID %d\n", *flow_id);
  return NW_OK;
}

nw_rc_t
sm_mbms_ip_multicast_distribution_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  mbms_ip_multicast_distribution_t           *mbms_ip_mc_addr = (mbms_ip_multicast_distribution_t *) arg;
  DevAssert (mbms_ip_mc_addr );

  /*
   * Copy the CTEID
   */
  mbms_ip_mc_addr->cteid = ntoh_int32_buf (ieValue);
  OAILOG_DEBUG (LOG_SM, "\t- CTEID    %08x\n", mbms_ip_mc_addr->cteid);
  ieValue+=4;

  /*
   * Distibution Address
   */
  int da_type = (*ieValue & 0xC0) >> 6;
  int da_length = (*ieValue & 0x3F);
  ieValue++;

  if (da_type == 0) {
    /*
     * IPv4 present: copy the 4 bytes
     */
	uint32_t hbo = (((uint32_t)ieValue[0]) << 24) |
			(((uint32_t)ieValue[1]) << 16) |
			(((uint32_t)ieValue[2]) << 8) |
			(uint32_t)ieValue[3];
    mbms_ip_mc_addr->distribution_address.pdn_type = IPv4;
	mbms_ip_mc_addr->distribution_address.address.ipv4_address.s_addr = htonl(hbo);
	OAILOG_DEBUG (LOG_SM, "\t- MC Distribution IPv4 addr   " IN_ADDR_FMT "\n", PRI_IN_ADDR (mbms_ip_mc_addr->distribution_address.address.ipv4_address));
	ieValue+=4;
  }
  else if (da_type == 1) {
	if(da_length != 16){
		OAILOG_ERROR (LOG_SM, "\t- Received invalid IPv6 length for IP Multicast addr  %d\n", da_length);
		return NW_FAILURE;
	}
    char                                    ipv6_ascii[INET6_ADDRSTRLEN];
    /*
     * IPv6 present: copy the 16 bytes
     * * * * WARNING: if Ipv4 is present, 4 bytes of offset should be applied
     */
    mbms_ip_mc_addr->distribution_address.pdn_type = IPv6;
    memcpy (mbms_ip_mc_addr->distribution_address.address.ipv6_address.__in6_u.__u6_addr8, ieValue, da_length);
    inet_ntop (AF_INET6, (void*)&mbms_ip_mc_addr->distribution_address.address.ipv6_address, ipv6_ascii, INET6_ADDRSTRLEN);
    OAILOG_DEBUG (LOG_SM, "\t- IPv6 MC distribution addr   %s\n", ipv6_ascii);
    ieValue+=16;
  } else {
	  OAILOG_ERROR (LOG_SM, "\t- Received invalid IP type %d for IP Multicast distribution addr. \n", mbms_ip_mc_addr->distribution_address.pdn_type);
	  return NW_FAILURE;
  }

  /*
   * Source Address
   */
  int sa_type = (*ieValue & 0xC0) >> 6;
  int sa_length = (*ieValue & 0x3F);
  ieValue++;

  if (sa_type == 0) {
    /*
     * IPv4 present: copy the 4 bytes
     */
	uint32_t hbo = (((uint32_t)ieValue[0]) << 24) |
			(((uint32_t)ieValue[1]) << 16) |
			(((uint32_t)ieValue[2]) << 8) |
			(uint32_t)ieValue[3];
	mbms_ip_mc_addr->source_address.pdn_type = IPv4;
	mbms_ip_mc_addr->source_address.address.ipv4_address.s_addr = htonl(hbo);
	OAILOG_DEBUG (LOG_SM, "\t- MC Source IPv4 addr   " IN_ADDR_FMT "\n", PRI_IN_ADDR (mbms_ip_mc_addr->source_address.address.ipv4_address));
	ieValue+=4;
  }
  else if (sa_type == 1) {
	if(sa_length != 16){
	  OAILOG_ERROR (LOG_SM, "\t- Received invalid IPv6 length for IP Multicast source addr  %d\n", sa_length);
	  return NW_FAILURE;
	}
    char                                    ipv6_ascii[INET6_ADDRSTRLEN];
    /*
     * IPv6 present: copy the 16 bytes
     * * * * WARNING: if Ipv4 is present, 4 bytes of offset should be applied
     */
	mbms_ip_mc_addr->source_address.pdn_type = IPv6;
	memcpy (mbms_ip_mc_addr->source_address.address.ipv6_address.__in6_u.__u6_addr8, ieValue, sa_length);
    inet_ntop (AF_INET6, (void*)&mbms_ip_mc_addr->source_address.address.ipv6_address, ipv6_ascii, INET6_ADDRSTRLEN);
    OAILOG_DEBUG (LOG_SM, "\t- IPv6 MC source addr   %s\n", ipv6_ascii);
    ieValue+=16;
  } else {
	  OAILOG_ERROR (LOG_SM, "\t- Received invalid IP type %d for IP Multicast source addr.\n", mbms_ip_mc_addr->source_address.pdn_type);
	  return NW_FAILURE;
  }

  mbms_ip_mc_addr->hc_indication = *ieValue;
  ieValue++;

  return NW_OK;
}

nw_rc_t sm_mbms_data_transfer_start_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  mbms_abs_time_data_transfer_t                       *abs_time = (mbms_abs_time_data_transfer_t *) arg;
  uint8_t		b_seconds[4];
  uint8_t		b_useconds[4];
  DevAssert (abs_time);

  memcpy((void*)b_seconds, ieValue, 4);
  memcpy((void*)b_useconds, ieValue+4, 4);

  /** Get the number of seconds since EPOCH. */
  uint32_t seconds  = (((uint32_t)b_seconds[0]) << 24) |
  			(((uint32_t)b_seconds[1]) << 16) |
  			(((uint32_t)b_seconds[2]) << 8) |
  			(uint32_t)b_seconds[3];

  abs_time->sec_since_epoch = seconds - JAN_1970;
  /** Calculate microseconds. */
  long double rest;
  for(int i = 0; i < 4; i ++){
	  for(int j = 0; j < 8; j ++){
		  rest = (b_useconds[i] & (0x01 << j)) * ((1/(2^32)) << (i*8 + j));
	  }
  }
  /** Divide it to a million to get useconds. */
  abs_time->usec = rest / 1000000;
  ieValue+=8;
  return NW_OK;
}

/**
 * MBMS Bearer Flags
 */
nw_rc_t
sm_mbms_flags_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  mbms_flags_t                         *bearer_flags = (mbms_flags_t *) arg;

  DevAssert (arg );

  if (ieLength != 1) {
    return NW_GTPV2C_IE_INCORRECT;
  }

  bearer_flags->msri = ieValue[0] & 0x01;
  bearer_flags->lmri = ieValue[0] & 0x02;
  return NW_OK;
}

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

/*
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "bstrlib.h"
#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "RemoteUEIPInformation.h"

//------------------------------------------------------------------------------
int decode_remote_ue_ip_information(
    remote_ue_ip_information_t *remoteueipinformation, 
    uint8_t *buffer, 
    uint8_t iei,
    uint32_t len)
    {
    int                                     decoded = 0;
    uint8_t                                 ielen = 0;
    if (iei > 0) {
    CHECK_IEI_DECODER (iei, *buffer);
    decoded++;
  }
    DECODE_U8(buffer + decoded, ielen, decoded);
    memset(remoteueipinformation, 0, sizeof(remote_ue_ip_information_t));
    CHECK_LENGTH_DECODER(len - decoded, ielen);
    remoteueipinformation ->spare = (*(buffer + decoded) >> 3) & 0xf;
    remoteueipinformation ->addresstype = *(buffer + decoded) & 0xf;
     decoded ++ ;
     if (iei >1)
    {
        decoded ++;
    }
    if ((ielen + 2) != decoded){
        decoded = ielen + 1 + (iei > 0 ? 1 : 0);
    } 
    return decoded ;
    } 
 //------------------------------------------------------------------------------
  int encode_remote_ue_ip_information(
    remote_ue_ip_information_t *remoteueipinformation, 
    uint8_t *buffer,
    uint8_t iei, 
    uint32_t len)      
    {
        uint8_t                                  *lenPtr ;
        uint32_t                                encoded = 0;
        int                                     encode_result;

  /*
   * Checking IEI and pointer
   */
  //CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, REMOTE_UE_IP_INFORMATION_MINIMUM_LENGTH, len);
  
  /*
  if (iei > 0)
{
    *buffer = iei;
    encoded++ ;

}
   
  lenPtr = (buffer + encoded);
  encoded++;
  *(buffer + encoded) = 0x00 | (((remoteueipinformation ->spare) & 0x1f) << 3) |
  ((remoteueipinformation ->addresstype = ADDRESS_TYPE_IPV4)& 0x7);
  encoded++;
  *(buffer + encoded) = (&remoteueipinformation->remoteueipaddress);
  memcpy((void*)(buffer + encoded), (const void*)(&remoteueipinformation->remoteueipaddress.ipv4), 4);
  encoded += 4;

  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
  
return encoded;
   }remote
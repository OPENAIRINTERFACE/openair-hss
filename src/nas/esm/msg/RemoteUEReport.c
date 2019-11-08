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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "bstrlib.h"

#include "log.h"
#include "3gpp_24.007.h"
#include "3gpp_24.301.h"
#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "assertions.h"
#include "common_types.h"
#include "RemoteUEReport.h"
#include "PKMFAddress.h"
#include "RemoteUEContext.h"


int decode_remote_ue_report(remote_ue_report_msg *remoteuereport, uint8_t *buffer, uint32_t len)
{
uint32_t decoded = 0;
int decoded_result = 0;
// Check if we got a NULL pointer and if buffer length is >= minimum length expected for the message.
CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, REMOTE_UE_REPORT_MINIMUM_LENGTH, len);
/*
* Decoding optional fields
*/
while(len - decoded >0){
    uint8_t ieiDecoded = *(buffer + decoded);
/*
* Type | value iei are below 0x80 so just return the first 4 bits
*/

if (ieiDecoded >= 0x80)
ieiDecoded = ieiDecoded & 0xf0;

switch (ieiDecoded){
case REMOTE_UE_REPORT_PKMF_ADDRESS_IEI:
if ((decoded_result = decode_pkmf_address(&remoteuereport->pkmfaddress, buffer+decoded, REMOTE_UE_REPORT_PKMF_ADDRESS_IEI,  len - decoded)) < 0 )
return decoded_result;

decoded += decoded_result;

/*
* Set corresponding mask to 1 in presencemask
*/
    remoteuereport -> presencemask |= REMOTE_UE_REPORT_PKMF_ADDRESS_PRESENT;
    break;
}
}
return decoded;
}

int encode_remote_ue_report(remote_ue_report_msg *remoteuereport, uint8_t *buffer, uint32_t len)
{
int encoded = 0 ;
int encode_result = 0;
/*
   * Checking IEI and pointer
   */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, REMOTE_UE_REPORT_MINIMUM_LENGTH, len);  

  //if ((remoteuereport->presencemask & REMOTE_UE_CONTEXT_PRESENT) == REMOTE_UE_CONTEXT_PRESENT)
  //{
   //   if((encode_result = encode_remote_ue_context (&remoteuereport ->remoteuecontext, REMOTE_UE_REPORT_UE_CONTEXT_IEI, buffer+encoded, len-encoded)) < 0)
   //   return encode_result;

  //} 
  if ((remoteuereport->presencemask & REMOTE_UE_REPORT_PKMF_ADDRESS_PRESENT) == REMOTE_UE_REPORT_PKMF_ADDRESS_PRESENT)
  {
  if((encode_result = encode_pkmf_address (&remoteuereport ->pkmfaddress, REMOTE_UE_REPORT_PKMF_ADDRESS_IEI, buffer+encoded, len-encoded)) < 0)
      return encode_result;    
  else
  encoded += encode_result;
      }
return encoded;
}
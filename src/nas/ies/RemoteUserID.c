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
#include "RemoteUserID.h"
#include "common_defs.h"


static int remoteue_decode_imsi (imsi_identity_t * imsi, uint8_t *  buffer, const uint8_t ie_len);
static int remoteue_encode_imsi (imsi_identity_t * imsi, uint8_t * buffer);
//------------------------------------------------------------------------------

int decode_remote_user_id(remote_user_id_t *remoteuserid, 
uint8_t iei, uint8_t *buffer, 
uint32_t len)
{
    int                                     decoded = 0;
    uint8_t                                 ielen = 0;
    if (iei > 0) {
    CHECK_IEI_DECODER (iei, *buffer);
    decoded++;
  }
  DECODE_U8 (buffer + decoded, ielen, decoded);
   memset (remoteuserid, 0, sizeof (remote_user_id_t));
   //OAILOG_TRACE (LOG_NAS_EMM, "decode_remote_user_id = %d\n", ielen);
   CHECK_LENGTH_DECODER (len - decoded, ielen);
   remoteuserid ->spare = (*(buffer + decoded) << 4) & 0x1;
   remoteuserid->instance = *(buffer + decoded) & 0x1;
      decoded++;
      remoteuserid->spare_instance = 1;
      //OAILOG_TRACE (LOG_NAS_EMM, "remoteuserid decoded spare_instance\n");


      if (iei > 1){
      remoteuserid ->spare1 = (*(buffer + decoded) << 2) & 0x5f;
      remoteuserid->imeif = (*(buffer + decoded) << 1) & 0x1;
      remoteuserid->msisdnf = *(buffer + decoded ) & 0x1;
      decoded++;
      remoteuserid->flags_present = 1;
     // OAILOG_TRACE (LOG_NAS_EMM, "remoteuserid decoded flags_present\n");

      if (iei > 2){
      remoteuserid->imsi_identity->num_digits = *(buffer + decoded ) & 0xf;
      decoded++;

      if (iei > 3){
      decoded += remoteue_decode_imsi(&remoteuserid->imsi_identity, buffer + decoded, len-decoded);
      decoded++;
              }
            }
        }

      //OAILOG_TRACE (LOG_NAS_EMM, "remoteuserid decoded=%u\n", decoded);

       if ((ielen + 2) != decoded) {
       decoded = ielen + 1 + (iei > 0 ? 1 : 0) /* Size of header for this IE */ ;
          //OAILOG_TRACE (LOG_NAS_EMM, "remoteuserid then decoded=%u\n", decoded);
      /* }
       return decoded;
      }

//----------------------------------------------------------------

int encode_remote_user_id(
remote_user_id_t *remoteuserid,
uint8_t iei,
uint8_t *buffer,
uint32_t len)
{
uint8_t                                *lenPtr;
uint32_t                                encoded = 0;

if (iei > 0)
{
*buffer = iei;
encoded++;
}

lenPtr = (buffer + encoded);
encoded++;
if(remoteuserid->spare_instance){
*(buffer + encoded) = 0x00 | ((remoteuserid->spare & 0x1f) << 4) | (remoteuserid->instance & 0x1f);
   encoded++;

}

if(remoteuserid->flags_present){
*(buffer + encoded) =  ((remoteuserid->spare1 & 0x3f) << 2) | // spare coded as zero
((remoteuserid->imeif  & 0x1) << 1) |
(remoteuserid->msisdnf & 0x1);
encoded++;

}
lenPtr = (buffer + encoded);
 encoded++;
 *(buffer + encoded) = remoteuserid->imsi_identity->num_digits;
 encoded++;
 encoded += remoteue_encode_imsi(&remoteuserid->imsi_identity, buffer + encoded);
 encoded++;

*lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
       return encoded;
}

//--------------------------------------------------------------------------

 int remoteue_decode_imsi (imsi_identity_t * imsi, 
 uint8_t *  buffer, 
 const uint8_t ie_len)

{

//OAILOG_FUNC_IN (LOG_NAS_EMM);
 int                             decoded = 0;
 imsi->instanceimsi =            *(buffer + decoded) & 0xf;
 imsi->spareimsi =               (*(buffer + decoded) >> 4) & 0xf;

 //imsi->typeofidentity = *(buffer + decoded) & 0x7;
 //  if (imsi->typeofidentity != EPS_MOBILE_IDENTITY_IMSI)
 //{
 //return (TLV_VALUE_DOESNT_MATCH);
 //}
 //imsi->oddeven = (*(buffer + decoded) >> 3) & 0x1;
 //imsi->identity_digit1 = (*(buffer + decoded) >> 4) & 0xf;

    imsi->num_digits = 1;
    decoded++;
    if (decoded < ie_len)
    {
    imsi->identity_digit1 = *(buffer + decoded) & 0xf;
    imsi->identity_digit2 = (*(buffer + decoded) >> 4) & 0xf;
    decoded++;
    imsi->num_digits += 2;
    if (decoded < ie_len)
    {
    imsi->identity_digit3 = *(buffer + decoded) & 0xf;
    imsi->num_digits++;
    imsi->identity_digit4 = (*(buffer + decoded) >> 4) & 0xf;
    if ((IMSI_ODD == imsi->oddeven)  && (imsi->identity_digit5 != 0x0f))
    {
    return (TLV_VALUE_DOESNT_MATCH);
    }
    else
    {
    imsi->num_digits++;
    }
    decoded++;
    if (decoded < ie_len)
    {
    imsi->identity_digit5 = *(buffer + decoded) & 0xf;
    imsi->num_digits++;
    imsi->identity_digit6 = (*(buffer + decoded) >> 4) & 0xf;
    if ((IMSI_ODD == imsi->oddeven)  && (imsi->identity_digit7 != 0x0f))
    {
    return (TLV_VALUE_DOESNT_MATCH);
    }
    else
    {
    imsi->num_digits++;
    }
    decoded++;
    if (decoded < ie_len)
    {
    imsi->identity_digit7 = *(buffer + decoded) & 0xf;
    imsi->num_digits++;
    imsi->identity_digit8 = (*(buffer + decoded) >> 4) & 0xf;
    if ((IMSI_ODD == imsi->oddeven)  && (imsi->identity_digit9 != 0x0f))
    {
    return (TLV_VALUE_DOESNT_MATCH);
    }
          else
    {
    imsi->num_digits++;
    }
    decoded++;
    if (decoded < ie_len)
    {
    imsi->identity_digit9 = *(buffer + decoded) & 0xf;
    imsi->num_digits++;
    imsi->identity_digit10 = (*(buffer + decoded) >> 4) & 0xf;
    if ((IMSI_ODD == imsi->oddeven)  && (imsi->identity_digit11 != 0x0f)) {
    return (TLV_VALUE_DOESNT_MATCH);
    }
    else
    {
    imsi->num_digits++;
    }
    decoded++;
    if (decoded < ie_len)
    {
    imsi->identity_digit11 = *(buffer + decoded) & 0xf;
    imsi->num_digits++;
    imsi->identity_digit12 = (*(buffer + decoded) >> 4) & 0xf;
    if ((IMSI_ODD == imsi->oddeven)  && (imsi->identity_digit13 != 0x0f))
    {
    return (TLV_VALUE_DOESNT_MATCH);
    }
    else
    {
    imsi->num_digits++;
    }
    decoded++;
    if (decoded < ie_len)
    {
    imsi->identity_digit13 = *(buffer + decoded) & 0xf;
    imsi->num_digits++;
    imsi->identity_digit14 = (*(buffer + decoded) >> 4) & 0xf;
    if ((IMSI_ODD == imsi->oddeven)  && (imsi->identity_digit15 != 0x0f))
    {
    return (TLV_VALUE_DOESNT_MATCH);
    }
    else
    {
    imsi->num_digits++;
    }
    decoded++;
    }
    }
    }
    }
    }
    }
    }
    return decoded;
     //OAILOG_FUNC_RETURN (LOG_NAS_EMM, decoded);
   }
//---------------------------------------------------------------------

 int remoteue_encode_imsi (imsi_identity_t * imsi, 
                           uint8_t * buffer)
 {
  uint32_t                                encoded = 0;

    *(buffer + encoded) = 0x00 | (imsi->spareimsi << 4) | (imsi->instanceimsi);
    encoded++;
    *(buffer + encoded) = 0x00 | (imsi->identity_digit2 << 4) | imsi->identity_digit1;
    encoded++;
  // Quick fix, should do a loop, but try without modifying struct!
  
    if (imsi->num_digits > 2) {
    if (imsi->oddeven != IMSI_ODD) {
    *(buffer + encoded) = 0x00 | (imsi->identity_digit4 << 4) | imsi->identity_digit3;
    } else {
    *(buffer + encoded) = (0xf0 << 4) | imsi->identity_digit3;
    encoded++;

    if (imsi->num_digits > 4) {
    if (imsi->oddeven != IMSI_ODD) {
        *(buffer + encoded) = 0x00 | (imsi->identity_digit6 << 4) | imsi->identity_digit5;
    } else {
    *(buffer + encoded) = (0xf0 << 4) | imsi->identity_digit5;
    }
    encoded++;
    
    if (imsi->num_digits > 6) {
    if (imsi->oddeven != IMSI_ODD) {
    *(buffer + encoded) = 0x00 | (imsi->identity_digit8 << 4) | imsi->identity_digit7;
    } else {
    *(buffer + encoded) = (0xf0 << 4) | imsi->identity_digit7;
    }
    encoded++;
    
    if (imsi->num_digits > 8) {
    if (imsi->oddeven != IMSI_ODD) {
            *(buffer + encoded) = 0x00 | (imsi->identity_digit10 << 4) | imsi->identity_digit9;
    } else {
    *(buffer + encoded) = (0xf0 << 4) | imsi->identity_digit9;
    }
    encoded++;
    
    if (imsi->num_digits > 10) {
    if (imsi->oddeven != IMSI_ODD) {
    *(buffer + encoded) = 0x00 | (imsi->identity_digit12 << 4) | imsi->identity_digit11;
    } else {
    *(buffer + encoded) = (0xf0 << 4) | imsi->identity_digit11;
    }
    encoded++;
    
    if (imsi->num_digits > 12) {
    if (imsi->oddeven != IMSI_ODD) {
    *(buffer + encoded) = 0x00 | (imsi->identity_digit14 << 4) | imsi->identity_digit13;
    } else {
    *(buffer + encoded) = (0xf0 << 4) | imsi->identity_digit13;
    }
    encoded++;
    
    if (imsi->num_digits > 14) {
    *(buffer + encoded) = (0xf0 << 4) | imsi->identity_digit15;
    }
    encoded++;
              }
            }
          }
        }
      }
    }
    }
    if (imsi->oddeven == IMSI_ODD) {
    *(buffer + encoded - 1) = 0xf0 | *(buffer + encoded - 1);
   }

   return encoded;

    }
*/


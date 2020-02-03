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
#include <stdint.h>
#include <stdbool.h>

#include "bstrlib.h"

#include "log.h"
#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "RemoteUEContext.h"


static int decode_imsi_remote_ue_context (imsi_remote_ue_context_t * imsi, uint8_t * buffer, uint8_t ie_len);
//static int decode_encrypted_imsi_remote_ue_context (encrypted_imsi_remote_ue_context_t * imsi, uint8_t * buffer, uint8_t ie_len);
int decode_remote_ip_remote_ue_context(remote_ue_ip_remote_ue_context_t *remoteueip, uint8_t *buffer, uint8_t iei, uint32_t len);

static int encode_imsi_remote_ue_context (imsi_remote_ue_context_t * imsi, uint8_t * buffer);
//static int encode_encrypted_imsi_remote_ue_context (encrypted_imsi_remote_ue_context_t * imsi, uint8_t * buffer);
static encode_remote_ip_remote_ue_context(remote_ue_ip_remote_ue_context_t *remoteueip, uint8_t *buffer);


//-------------------------------------------------------------------

int decode_remote_ue_context(
    remote_ue_context_t *remoteuecontext, 
    uint8_t iei, uint8_t *buffer,
    uint32_t len)
     
{
    int                                     decoded_rc = TLV_VALUE_DOESNT_MATCH;
    int                                     decoded = 0;
    uint8_t                                 ielen = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER (iei, *buffer);
    decoded++;
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);
  uint8_t              typeofidentity = *(buffer + decoded) & 0x7;

    if (typeofidentity == REMOTE_UE_CONTEXT_IDENTITY_ENCRYPTED_IMSI) {
    decoded_rc = decode_encrypted_imsi_remote_ue_context (&remoteuecontext->encrypted_imsi, buffer + decoded, ielen);
    }
    else if (typeofidentity == REMOTE_UE_CONTEXT_IDENTITY_IMSI) {
    decoded_rc = decode_imsi_remote_ue_context (&remoteuecontext->imsi, buffer + decoded, ielen);
    }
    if (decoded < ielen) {
        remoteuecontext->remoteueipaddress.spare = (*(buffer + decoded) >> 3) & 0x7;
        remoteuecontext->remoteueipaddress.addresstype = (*(buffer + decoded)) & 0x7;
        
    }
    if (decoded_rc < 0) {
    return decoded_rc;
  }
  return (decoded + decoded_rc);
}

//---------------------------------------------------
int encode_remote_ue_context(
    remote_ue_context_t *remoteuecontext,
    uint8_t iei, uint8_t *buffer,
    uint32_t len)
    {
   uint8_t                                *lenPtr;
   int                                     encoded_rc = TLV_VALUE_DOESNT_MATCH;
   uint32_t                                encoded = 0;

  /*
   * Checking IEI and pointer
   */

  CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, REMOTE_UE_CONTEXT_MINIMUM_LENGTH, len);

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr = (buffer + encoded);
  encoded++;

  if (remoteuecontext->encrypted_imsi.typeofidentity == REMOTE_UE_CONTEXT_IDENTITY_ENCRYPTED_IMSI) {
    encoded_rc = encode_encrypted_imsi_remote_ue_context (&remoteuecontext->encrypted_imsi, buffer + encoded);
  } else if (remoteuecontext->imsi.typeofidentity == REMOTE_UE_CONTEXT_IDENTITY_IMSI) {
    encoded_rc = encode_imsi_remote_ue_context (&remoteuecontext->imsi, buffer + encoded);
  }
    
  if (encoded_rc < 0) {
  return encoded_rc;
  }

  *lenPtr = encoded + encoded_rc - 1 - ((iei > 0) ? 1 : 0);
  return (encoded + encoded_rc);
}

//---------------------------------------------------------

static int decode_imsi_remote_ue_context (imsi_remote_ue_context_t * imsi, uint8_t * buffer, uint8_t ie_len)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     decoded = 0;

  imsi->typeofidentity = *(buffer + decoded) & 0x7;

  if (imsi->typeofidentity != REMOTE_UE_CONTEXT_IDENTITY_IMSI) {
    return (TLV_VALUE_DOESNT_MATCH);
  }

  imsi->oddeven = (*(buffer + decoded) >> 3) & 0x1;
  imsi->identity_digit1 = (*(buffer + decoded) >> 4) & 0xf;
  imsi->num_digits = 1;
  decoded++;
  if (decoded < ie_len) {
    imsi->identity_digit2 = *(buffer + decoded) & 0xf;
    imsi->identity_digit3 = (*(buffer + decoded) >> 4) & 0xf;
    decoded++;
    imsi->num_digits += 2;
    if (decoded < ie_len) {
      imsi->identity_digit4 = *(buffer + decoded) & 0xf;
      imsi->num_digits++;
      imsi->identity_digit5 = (*(buffer + decoded) >> 4) & 0xf;
      if ((REMOTE_UE_CONTEXT_IDENTITY_EVEN == imsi->oddeven)  && (imsi->identity_digit5 != 0x0f)) {
        return (TLV_VALUE_DOESNT_MATCH);
      } else {
        imsi->num_digits++;
      }
      decoded++;
      if (decoded < ie_len) {
        imsi->identity_digit6 = *(buffer + decoded) & 0xf;
        imsi->num_digits++;
        imsi->identity_digit7 = (*(buffer + decoded) >> 4) & 0xf;
        if ((REMOTE_UE_CONTEXT_IDENTITY_EVEN == imsi->oddeven)  && (imsi->identity_digit7 != 0x0f)) {
          return (TLV_VALUE_DOESNT_MATCH);
        } else {
          imsi->num_digits++;
        }
        decoded++;
        if (decoded < ie_len) {
          imsi->identity_digit8 = *(buffer + decoded) & 0xf;
          imsi->num_digits++;
          imsi->identity_digit9 = (*(buffer + decoded) >> 4) & 0xf;
          if ((REMOTE_UE_CONTEXT_IDENTITY_EVEN == imsi->oddeven)  && (imsi->identity_digit9 != 0x0f)) {
            return (TLV_VALUE_DOESNT_MATCH);
          } else {
            imsi->num_digits++;
          }
          decoded++;
          if (decoded < ie_len) {
            imsi->identity_digit10 = *(buffer + decoded) & 0xf;
            imsi->num_digits++;
            imsi->identity_digit11 = (*(buffer + decoded) >> 4) & 0xf;
            if ((REMOTE_UE_CONTEXT_IDENTITY_EVEN == imsi->oddeven)  && (imsi->identity_digit11 != 0x0f)) {
              return (TLV_VALUE_DOESNT_MATCH);
            } else {
              imsi->num_digits++;
            }
            decoded++;
            if (decoded < ie_len) {
              imsi->identity_digit12 = *(buffer + decoded) & 0xf;
              imsi->num_digits++;
              imsi->identity_digit13 = (*(buffer + decoded) >> 4) & 0xf;
              if ((REMOTE_UE_CONTEXT_IDENTITY_EVEN == imsi->oddeven)  && (imsi->identity_digit13 != 0x0f)) {
                return (TLV_VALUE_DOESNT_MATCH);
              } else {
                imsi->num_digits++;
              }
              decoded++;
              if (decoded < ie_len) {
                imsi->identity_digit14 = *(buffer + decoded) & 0xf;
                imsi->num_digits++;
                imsi->identity_digit15 = (*(buffer + decoded) >> 4) & 0xf;
                if ((REMOTE_UE_CONTEXT_IDENTITY_EVEN == imsi->oddeven)  && (imsi->identity_digit15 != 0x0f)) {
                  return (TLV_VALUE_DOESNT_MATCH);
                } else {
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

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, decoded);
}
//------------------------------------------------------------------------------
static int encode_imsi_remote_ue_context (imsi_remote_ue_context_t * imsi, uint8_t * buffer)
{
  uint32_t                                encoded = 0;

  *(buffer + encoded) = 0x00 | (imsi->identity_digit1 << 4) | (imsi->oddeven << 3) | (imsi->typeofidentity);
  encoded++;
  *(buffer + encoded) = 0x00 | (imsi->identity_digit3 << 4) | imsi->identity_digit2;
  encoded++;
  // Quick fix, should do a loop, but try without modifying struct!
  if (imsi->num_digits > 3) {
    if (imsi->oddeven != REMOTE_UE_CONTEXT_IDENTITY_EVEN) {
      *(buffer + encoded) = 0x00 | (imsi->identity_digit5 << 4) | imsi->identity_digit4;
    } else {
      *(buffer + encoded) = 0xf0 | imsi->identity_digit4;
    }
    encoded++;
    if (imsi->num_digits > 5) {
      if (imsi->oddeven != REMOTE_UE_CONTEXT_IDENTITY_EVEN) {
        *(buffer + encoded) = 0x00 | (imsi->identity_digit7 << 4) | imsi->identity_digit6;
      } else {
        *(buffer + encoded) = 0xf0 | imsi->identity_digit6;
      }
      encoded++;
      if (imsi->num_digits > 7) {
        if (imsi->oddeven != REMOTE_UE_CONTEXT_IDENTITY_EVEN) {
          *(buffer + encoded) = 0x00 | (imsi->identity_digit9 << 4) | imsi->identity_digit8;
        } else {
          *(buffer + encoded) = 0xf0 | imsi->identity_digit8;
        }
        encoded++;
        if (imsi->num_digits > 9) {
          if (imsi->oddeven != REMOTE_UE_CONTEXT_IDENTITY_EVEN) {
            *(buffer + encoded) = 0x00 | (imsi->identity_digit11 << 4) | imsi->identity_digit10;
          } else {
            *(buffer + encoded) = 0xf0 | imsi->identity_digit10;
          }
          encoded++;
          if (imsi->num_digits > 11) {
            if (imsi->oddeven != REMOTE_UE_CONTEXT_IDENTITY_EVEN) {
              *(buffer + encoded) = 0x00 | (imsi->identity_digit13 << 4) | imsi->identity_digit12;
            } else {
              *(buffer + encoded) = 0xf0 | imsi->identity_digit12;
            }
            encoded++;
            if (imsi->num_digits > 13) {
              if (imsi->oddeven != REMOTE_UE_CONTEXT_IDENTITY_EVEN) {
                *(buffer + encoded) = 0x00 | (imsi->identity_digit15 << 4) | imsi->identity_digit14;
              } else {
                *(buffer + encoded) = 0xf0 | imsi->identity_digit14;
              }
              encoded++;
            }
          }
        }
      }
    }
  }
  return encoded;
}

//------------------------------------------------------------------------------


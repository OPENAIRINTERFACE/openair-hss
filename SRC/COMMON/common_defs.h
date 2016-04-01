/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */



#ifndef FILE_COMMON_DEFS_SEEN
#define FILE_COMMON_DEFS_SEEN
#include <arpa/inet.h>  // htonl, htons
#include <stdint.h>

//------------------------------------------------------------------------------
#define STOLEN_REF
#define CLONE_REF

//------------------------------------------------------------------------------

typedef enum {
  /* Fatal errors - received message should not be processed */
  TLV_MAC_MISMATCH                        = -14,
  TLV_BUFFER_NULL                         = -13,
  TLV_BUFFER_TOO_SHORT                    = -12,
  TLV_PROTOCOL_NOT_SUPPORTED              = -11,
  TLV_WRONG_MESSAGE_TYPE                  = -10,
  TLV_OCTET_STRING_TOO_LONG_FOR_IEI       = -9,

  TLV_VALUE_DOESNT_MATCH                  = -4,
  TLV_MANDATORY_FIELD_NOT_PRESENT         = -3,
  TLV_UNEXPECTED_IEI                      = -2,

  RETURNerror                             = -1,
  RETURNok                                = 0,

  TLV_ERROR_OK                            =  RETURNok,
  /* Defines error code limit below which received message should be discarded
   * because it cannot be further processed */
  TLV_FATAL_ERROR                         = TLV_VALUE_DOESNT_MATCH

} error_code_e;
//------------------------------------------------------------------------------
#define DECODE_U8(bUFFER, vALUE, sIZE)    \
    vALUE = *(uint8_t*)(bUFFER);    \
    sIZE += sizeof(uint8_t)

#define DECODE_U16(bUFFER, vALUE, sIZE)   \
    vALUE = ntohs(*(uint16_t*)(bUFFER));  \
    sIZE += sizeof(uint16_t)

#define DECODE_U24(bUFFER, vALUE, sIZE)   \
    vALUE = ntohl(*(uint32_t*)(bUFFER)) >> 8; \
    sIZE += sizeof(uint8_t) + sizeof(uint16_t)

#define DECODE_U32(bUFFER, vALUE, sIZE)   \
    vALUE = ntohl(*(uint32_t*)(bUFFER));  \
    sIZE += sizeof(uint32_t)

#if (BYTE_ORDER == LITTLE_ENDIAN)
# define DECODE_LENGTH_U16(bUFFER, vALUE, sIZE)          \
    vALUE = ((*(bUFFER)) << 8) | (*((bUFFER) + 1));      \
    sIZE += sizeof(uint16_t)
#else
# define DECODE_LENGTH_U16(bUFFER, vALUE, sIZE)          \
    vALUE = (*(bUFFER)) | (*((bUFFER) + 1) << 8);        \
    sIZE += sizeof(uint16_t)
#endif

#define ENCODE_U8(buffer, value, size)    \
    *(uint8_t*)(buffer) = value;    \
    size += sizeof(uint8_t)

#define ENCODE_U16(buffer, value, size)   \
    *(uint16_t*)(buffer) = htons(value);  \
   size += sizeof(uint16_t)

#define ENCODE_U24(buffer, value, size)   \
    *(uint32_t*)(buffer) = htonl(value);  \
    size += sizeof(uint8_t) + sizeof(uint16_t)

#define ENCODE_U32(buffer, value, size)   \
    *(uint32_t*)(buffer) = htonl(value);  \
    size += sizeof(uint32_t)


#endif /* FILE_COMMON_DEFS_SEEN */

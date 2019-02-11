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

/*! \file common_defs.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_COMMON_DEFS_SEEN
#define FILE_COMMON_DEFS_SEEN

//------------------------------------------------------------------------------
#define STOLEN_REF
#define CLONE_REF

#define OFFSET_OF(TyPe, MeMBeR) ((size_t) &((TyPe *)0)->MeMBeR)


#define PARENT_STRUCT(cOnTaiNeD, TyPe, MeMBeR) ({                      \
        const typeof( ((TyPe *)0)->MeMBeR ) *__MemBeR_ptr = (cOnTaiNeD);    \
        (TyPe *)( (char *)__MemBeR_ptr - OFFSET_OF(TyPe,MeMBeR) );})


 #define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

 #define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })


/*
 * A list of PLMNs
 */
#define PLMN_LIST_T(SIZE) struct {uint8_t n_plmns; plmn_t plmn[SIZE];}

/*
 * A list of TACs
 */
#define TAC_LIST_T(SIZE) struct {uint8_t n_tacs; TAC_t tac[SIZE];}

//------------------------------------------------------------------------------
#define INTERNAL_FLAG_NULL        0
#define INTERNAL_FLAG_X2_HANDOVER   (1 << 0)
#define INTERNAL_FLAG_SKIP_RESPONSE (2 << 0)
#define INTERNAL_FLAG_TRIGGERED_ACK (3 << 0)
//#define INTERNAL_FLAG_TRIGGERED_REJECT (4 << 0)

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




#define IPV4_STR_ADDR_TO_INT_NWBO(AdDr_StR,NwBo,MeSsAgE ) do {\
            struct in_addr inp;\
            if ( inet_aton(AdDr_StR, &inp ) < 0 ) {\
                AssertFatal (0, MeSsAgE);\
            } else {\
                NwBo = inp.s_addr;\
            }\
        } while (0)

#define IPV4_STR_ADDR_TO_INADDR(AdDr_StR,InAdDr,MeSsAgE ) do {\
            if ( inet_aton(AdDr_StR, &InAdDr ) <= 0 ) {\
                AssertFatal (0, MeSsAgE);\
            }\
        } while (0)

#define NIPADDR(addr) \
  (uint8_t)(addr & 0x000000FF), \
  (uint8_t)((addr & 0x0000FF00) >> 8), \
  (uint8_t)((addr & 0x00FF0000) >> 16), \
  (uint8_t)((addr & 0xFF000000) >> 24)

#define HIPADDR(addr) \
  (uint8_t)((addr & 0xFF000000) >> 24),\
  (uint8_t)((addr & 0x00FF0000) >> 16),\
  (uint8_t)((addr & 0x0000FF00) >> 8), \
  (uint8_t)(addr & 0x000000FF)

#define NIP6ADDR(addr) \
  ntohs((addr)->s6_addr16[0]), \
  ntohs((addr)->s6_addr16[1]), \
  ntohs((addr)->s6_addr16[2]), \
  ntohs((addr)->s6_addr16[3]), \
  ntohs((addr)->s6_addr16[4]), \
  ntohs((addr)->s6_addr16[5]), \
  ntohs((addr)->s6_addr16[6]), \
  ntohs((addr)->s6_addr16[7])

#define IN6_ARE_ADDR_MASKED_EQUAL(a,b,m) \
  (((((__const uint32_t *) (a))[0] & (((__const uint32_t *) (m))[0])) == (((__const uint32_t *) (b))[0] & (((__const uint32_t *) (m))[0])))  \
   && ((((__const uint32_t *) (a))[1] & (((__const uint32_t *) (m))[1])) == (((__const uint32_t *) (b))[1] & (((__const uint32_t *) (m))[1])))  \
   && ((((__const uint32_t *) (a))[2] & (((__const uint32_t *) (m))[2])) == (((__const uint32_t *) (b))[2] & (((__const uint32_t *) (m))[2])))  \
   && ((((__const uint32_t *) (a))[3] & (((__const uint32_t *) (m))[3])) == (((__const uint32_t *) (b))[3] & (((__const uint32_t *) (m))[3]))))

#define EBI_TO_INDEX(eBi) (eBi-5)
#define INDEX_TO_EBI(iNdEx) (iNdEx+5)

#endif /* FILE_COMMON_DEFS_SEEN */

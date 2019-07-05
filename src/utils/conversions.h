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

/*! \file conversions.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_CONVERSIONS_SEEN
#define FILE_CONVERSIONS_SEEN

#include "common_types.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "EpsQualityOfService.h"

/* Endianness conversions for 16 and 32 bits integers from host to network order */
#if (BYTE_ORDER == LITTLE_ENDIAN)
# define hton_int32(x)   \
    (((x & 0x000000FF) << 24) | ((x & 0x0000FF00) << 8) |  \
    ((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24))

# define hton_int16(x)   \
    (((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8)

# define ntoh_int32_buf(bUF)        \
    ((*(bUF)) << 24) | ((*((bUF) + 1)) << 16) | ((*((bUF) + 2)) << 8)   \
  | (*((bUF) + 3))
#else
# define hton_int32(x) (x)
# define hton_int16(x) (x)
#endif

#define IN_ADDR_TO_BUFFER(X,bUFF) INT32_TO_BUFFER((X).s_addr,(char*)bUFF)

#define IN6_ADDR_TO_BUFFER(X,bUFF)                     \
    do {                                               \
        ((uint8_t*)(bUFF))[0]  = (X).s6_addr[0];  \
        ((uint8_t*)(bUFF))[1]  = (X).s6_addr[1];  \
        ((uint8_t*)(bUFF))[2]  = (X).s6_addr[2];  \
        ((uint8_t*)(bUFF))[3]  = (X).s6_addr[3];  \
        ((uint8_t*)(bUFF))[4]  = (X).s6_addr[4];  \
        ((uint8_t*)(bUFF))[5]  = (X).s6_addr[5];  \
        ((uint8_t*)(bUFF))[6]  = (X).s6_addr[6];  \
        ((uint8_t*)(bUFF))[7]  = (X).s6_addr[7];  \
        ((uint8_t*)(bUFF))[8]  = (X).s6_addr[8];  \
        ((uint8_t*)(bUFF))[9]  = (X).s6_addr[9];  \
        ((uint8_t*)(bUFF))[10] = (X).s6_addr[10]; \
        ((uint8_t*)(bUFF))[11] = (X).s6_addr[11]; \
        ((uint8_t*)(bUFF))[12] = (X).s6_addr[12]; \
        ((uint8_t*)(bUFF))[13] = (X).s6_addr[13]; \
        ((uint8_t*)(bUFF))[14] = (X).s6_addr[14]; \
        ((uint8_t*)(bUFF))[15] = (X).s6_addr[15]; \
    } while(0)

#define BUFFER_TO_INT8(buf, x) (x = ((buf)[0]))

#define INT8_TO_BUFFER(x, buf) ((buf)[0] = (x))

/* Convert an integer on 16 bits to the given bUFFER */
#define INT16_TO_BUFFER(x, buf) \
do {                            \
    (buf)[0] = (x) >> 8;        \
    (buf)[1] = (x);             \
} while(0)

/* Convert an array of char containing vALUE to x */
#define BUFFER_TO_INT16(buf, x) \
do {                            \
    x = ((buf)[0] << 8)  |      \
        ((buf)[1]);             \
} while(0)

/* Convert an integer on 32 bits to the given bUFFER */
#define INT32_TO_BUFFER(x, buf) \
do {                            \
    (buf)[0] = (x) >> 24;       \
    (buf)[1] = (x) >> 16;       \
    (buf)[2] = (x) >> 8;        \
    (buf)[3] = (x);             \
} while(0)


/* Convert an integer on 64 bits to the given bUFFER */
#define INT64_TO_BUFFER(x, buf) \
do {                            \
    (buf)[0] = (x) >> 56;       \
    (buf)[1] = (x) >> 48;       \
    (buf)[2] = (x) >> 40;        \
    (buf)[3] = (x) >> 32;       \
    (buf)[4] = (x) >> 24;       \
    (buf)[5] = (x) >> 16;        \
    (buf)[6] = (x) >> 8;        \
    (buf)[7] = (x);             \
} while(0)

/* Convert an array of char containing vALUE to x */
#define BUFFER_TO_INT32(buf, x) \
do {                            \
    x = ((buf)[0] << 24) |      \
        ((buf)[1] << 16) |      \
        ((buf)[2] << 8)  |      \
        ((buf)[3]);             \
} while(0)

/* Convert an array of char containing vALUE to x */
#define BUFFER_TO_INT64(buf, x) \
do {                            \
    x = ((buf)[0] << 56) |      \
        ((buf)[1] << 48) |      \
        ((buf)[2] << 40)  |      \
        ((buf)[3] << 32)  |      \
        ((buf)[4] << 24)  |      \
        ((buf)[5] << 16)  |      \
        ((buf)[6] << 8)  |      \
        ((buf)[7]);             \
} while(0)

/* Convert an integer on 32 bits to an octet string from aSN1c tool */
#define INT32_TO_OCTET_STRING(x, aSN)           \
do {                                            \
    (aSN)->buf = calloc(4, sizeof(uint8_t));    \
    INT32_TO_BUFFER(x, ((aSN)->buf));           \
    (aSN)->size = 4;                            \
} while(0)

#define INT32_TO_BIT_STRING(x, aSN) \
do {                                \
    INT32_TO_OCTET_STRING(x, aSN);  \
    (aSN)->bits_unused = 0;         \
} while(0)

#define INT16_TO_OCTET_STRING(x, aSN)           \
do {                                            \
    (aSN)->buf = calloc(2, sizeof(uint8_t));    \
    (aSN)->size = 2;              \
    INT16_TO_BUFFER(x, (aSN)->buf);             \
} while(0)

#define INT8_TO_OCTET_STRING(x, aSN)            \
do {                                            \
    (aSN)->buf = calloc(1, sizeof(uint8_t));    \
    (aSN)->size = 1;                            \
    INT8_TO_BUFFER(x, (aSN)->buf);              \
} while(0)

#define MME_CODE_TO_OCTET_STRING INT8_TO_OCTET_STRING
#define M_TMSI_TO_OCTET_STRING   INT32_TO_OCTET_STRING
#define MME_GID_TO_OCTET_STRING  INT16_TO_OCTET_STRING

#define OCTET_STRING_TO_INT8(aSN, x)    \
do {                                    \
    DevCheck((aSN)->size == 1, (aSN)->size, 0, 0);           \
    BUFFER_TO_INT8((aSN)->buf, x);    \
} while(0)

#define OCTET_STRING_TO_INT16(aSN, x)   \
do {                                    \
    DevCheck((aSN)->size == 2, (aSN)->size, 0, 0);           \
    BUFFER_TO_INT16((aSN)->buf, x);    \
} while(0)

#define OCTET_STRING_TO_INT32(aSN, x)   \
do {                                    \
    DevCheck((aSN)->size == 4, (aSN)->size, 0, 0);           \
    BUFFER_TO_INT32((aSN)->buf, x);    \
} while(0)

#define BIT_STRING_TO_INT32(aSN, x)     \
do {                                    \
    DevCheck((aSN)->bits_unused == 0, (aSN)->bits_unused, 0, 0);    \
    OCTET_STRING_TO_INT32(aSN, x);      \
} while(0)

#define BIT_STRING_TO_CELL_IDENTITY(aSN, vALUE)                     \
do {                                                                \
    DevCheck((aSN)->bits_unused == 4, (aSN)->bits_unused, 4, 0);    \
    vALUE.enb_id = ((uint32_t)(aSN)->buf[0] << 16) | ((uint32_t)(aSN)->buf[1] << 8) |   \
        ((uint32_t)(aSN)->buf[2]);                                       \
    vALUE.cell_id = ((aSN)->buf[3]);    \
} while(0)

#define MCC_HUNDREDS(vALUE) \
    ((vALUE) / 100)
/* When MNC is only composed of 2 digits, set the hundreds unit to 0xf */
#define MNC_HUNDREDS(vALUE, mNCdIGITlENGTH) \
    ( mNCdIGITlENGTH == 2 ? 15 : (vALUE) / 100)
#define MCC_MNC_DECIMAL(vALUE) \
    (((vALUE) / 10) % 10)
#define MCC_MNC_DIGIT(vALUE) \
    ((vALUE) % 10)

#define MCC_TO_BUFFER(mCC, bUFFER)      \
do {                                    \
    DevAssert(bUFFER != NULL);          \
    (bUFFER)[0] = MCC_HUNDREDS(mCC);    \
    (bUFFER)[1] = MCC_MNC_DECIMAL(mCC); \
    (bUFFER)[2] = MCC_MNC_DIGIT(mCC);   \
} while(0)

#define MCC_MNC_TO_PLMNID(mCC, mNC, mNCdIGITlENGTH, oCTETsTRING)               \
do {                                                                           \
    (oCTETsTRING)->buf = calloc(3, sizeof(uint8_t));                           \
    (oCTETsTRING)->buf[0] = (MCC_MNC_DECIMAL(mCC) << 4) | MCC_HUNDREDS(mCC);   \
    (oCTETsTRING)->buf[1] = (MNC_HUNDREDS(mNC,mNCdIGITlENGTH) << 4) | MCC_MNC_DIGIT(mCC);     \
    (oCTETsTRING)->buf[2] = (MCC_MNC_DIGIT(mNC) << 4) | MCC_MNC_DECIMAL(mNC);  \
    (oCTETsTRING)->size = 3;                                                   \
} while(0)

#define MCC_MNC_TO_TBCD(mCC, mNC, mNCdIGITlENGTH, tBCDsTRING)        \
do {                                                                 \
    char _buf[3];                                                    \
     DevAssert((mNCdIGITlENGTH == 3) || (mNCdIGITlENGTH == 2));      \
    _buf[0] = (MCC_MNC_DECIMAL(mCC) << 4) | MCC_HUNDREDS(mCC);       \
    _buf[1] = (MNC_HUNDREDS(mNC,mNCdIGITlENGTH) << 4) | MCC_MNC_DIGIT(mCC);\
    _buf[2] = (MCC_MNC_DIGIT(mNC) << 4) | MCC_MNC_DECIMAL(mNC);      \
    OCTET_STRING_fromBuf(tBCDsTRING, _buf, 3);                       \
} while(0)

#define TBCD_TO_MCC_MNC(tBCDsTRING, mCC, mNC, mNCdIGITlENGTH)    \
do {                                                             \
    int mNC_hundred;                                             \
    DevAssert((tBCDsTRING)->size == 3);                          \
    mNC_hundred = (((tBCDsTRING)->buf[1] & 0xf0) >> 4);          \
    if (mNC_hundred == 0xf) {                                    \
        mNC_hundred = 0;                                         \
        mNCdIGITlENGTH = 2;                                      \
    } else {                                                     \
            mNCdIGITlENGTH = 3;                                  \
    }                                                            \
    mCC = (((((tBCDsTRING)->buf[0]) & 0xf0) >> 4) * 10) +        \
        ((((tBCDsTRING)->buf[0]) & 0x0f) * 100) +                \
        (((tBCDsTRING)->buf[1]) & 0x0f);                         \
    mNC = (mNC_hundred * 100) +                                  \
        ((((tBCDsTRING)->buf[2]) & 0xf0) >> 4) +                 \
        ((((tBCDsTRING)->buf[2]) & 0x0f) * 10);                  \
} while(0)

#define TBCD_TO_PLMN_T(tBCDsTRING, pLMN)                            \
do {                                                                \
    DevAssert((tBCDsTRING)->size == 3);                             \
    (pLMN)->mcc_digit2 = (((tBCDsTRING)->buf[0] & 0xf0) >> 4);      \
    (pLMN)->mcc_digit1 = ((tBCDsTRING)->buf[0]  & 0x0f);            \
    (pLMN)->mnc_digit3 = (((tBCDsTRING)->buf[2] & 0xf0) >> 4);      \
    (pLMN)->mcc_digit3 = ((tBCDsTRING)->buf[1]  & 0x0f);            \
    (pLMN)->mnc_digit2 = ((tBCDsTRING)->buf[2]  & 0x0f);      		\
    (pLMN)->mnc_digit1 = (((tBCDsTRING)->buf[1]  & 0xf0) >> 4);     \
    if((pLMN)->mnc_digit1 == 0x0F) {							    \
    	(pLMN)->mnc_digit1 = (pLMN)->mnc_digit2;					\
    	(pLMN)->mnc_digit2 = (pLMN)->mnc_digit3;					\
    	(pLMN)->mnc_digit3 = 0x0F;									\
    }																\
} while(0)

#define PLMN_T_TO_TBCD(pLMN, tBCDsTRING, mNClENGTH)                 \
do {                                                                \
    tBCDsTRING[0] = (pLMN.mcc_digit2 << 4) | pLMN.mcc_digit1;         \
    /* ambiguous (think about len 2) */                             \
    if (mNClENGTH == 2) {                                      \
        tBCDsTRING[1] = (0x0F << 4) | pLMN.mcc_digit3;               \
        tBCDsTRING[2] = (pLMN.mnc_digit2 << 4) | pLMN.mnc_digit1;     \
    } else {                                                        \
        tBCDsTRING[1] = (pLMN.mnc_digit3 << 4) | pLMN.mcc_digit3;     \
        tBCDsTRING[2] = (pLMN.mnc_digit2 << 4) | pLMN.mnc_digit1;     \
    }                                                               \
} while(0)
//
//#define PLMN_T_TO_MCC_MNC(pLMN, mCC, mNC, mNCdIGITlENGTH)               \
//do {                                                                    \
//    mCC = pLMN.mcc_digit3 * 100 + pLMN.mcc_digit2 * 10 + pLMN.mcc_digit1;  \
//    mNCdIGITlENGTH = (pLMN.mnc_digit3 == 0xF ? 2 : 3);                   \
//    mNC = (mNCdIGITlENGTH == 2 ? 0 : pLMN.mnc_digit3 * 100)              \
//          + pLMN.mnc_digit2 * 10 + pLMN.mnc_digit1;                       \
//} while(0)

#define PLMN_T_TO_MCC_MNC(pLMN, mCC, mNC, mNCdIGITlENGTH)                 \
do {                                                                      \
    mCC = pLMN.mcc_digit1 * 100 + pLMN.mcc_digit2 * 10 + pLMN.mcc_digit3; \
    mNCdIGITlENGTH = (pLMN.mnc_digit3 == 0xF ? 2 : 3);                    \
    mNC = (mNCdIGITlENGTH == 2 ? pLMN.mnc_digit1 * 10 + pLMN.mnc_digit2 : \
        pLMN.mnc_digit1 * 100 + pLMN.mnc_digit2 * 10 + pLMN.mnc_digit3);  \
} while(0)

/*
 * TS 36.413 v10.9.0 section 9.2.1.37:
 * Macro eNB ID:
 * Equal to the 20 leftmost bits of the Cell
 * Identity IE contained in the E-UTRAN CGI
 * IE (see subclause 9.2.1.38) of each cell
 * served by the eNB.
 */
#define MACRO_ENB_ID_TO_BIT_STRING(mACRO, bITsTRING)    \
do {                                                    \
    (bITsTRING)->buf = calloc(3, sizeof(uint8_t));      \
    (bITsTRING)->buf[0] = ((mACRO) >> 12);              \
    (bITsTRING)->buf[1] = (mACRO) >> 4;                 \
    (bITsTRING)->buf[2] = ((mACRO) & 0x0f) << 4;        \
    (bITsTRING)->size = 3;                              \
    (bITsTRING)->bits_unused = 4;                       \
} while(0)
/*
 * TS 36.413 v10.9.0 section 9.2.1.38:
 * E-UTRAN CGI/Cell Identity
 * The leftmost bits of the Cell
 * Identity correspond to the eNB
 * ID (defined in subclause 9.2.1.37).
 */
#define MACRO_ENB_ID_TO_CELL_IDENTITY(mACRO, cELL_iD, bITsTRING) \
do {                                                    \
    (bITsTRING)->buf = calloc(4, sizeof(uint8_t));      \
    (bITsTRING)->buf[0] = ((mACRO) >> 12);              \
    (bITsTRING)->buf[1] = (mACRO) >> 4;                 \
    (bITsTRING)->buf[2] = (((mACRO) & 0x0f) << 4) | ((cELL_iD) >> 4);        \
    (bITsTRING)->buf[3] = ((cELL_iD) & 0x0f) << 4;        \
    (bITsTRING)->size = 4;                              \
    (bITsTRING)->bits_unused = 4;                       \
} while(0)

/* Used to format an uint32_t containing an ipv4 address */
#define IN_ADDR_FMT    "%u.%u.%u.%u"
#define PRI_IN_ADDR(aDDRESS)               \
    (uint8_t)((aDDRESS.s_addr)  & 0x000000ff),         \
    (uint8_t)(((aDDRESS.s_addr) & 0x0000ff00) >> 8 ),  \
    (uint8_t)(((aDDRESS.s_addr) & 0x00ff0000) >> 16),  \
    (uint8_t)(((aDDRESS.s_addr) & 0xff000000) >> 24)

#define IPV4_ADDR_DISPLAY_8(aDDRESS)            \
    (aDDRESS)[0], (aDDRESS)[1], (aDDRESS)[2], (aDDRESS)[3]

#define TAC_TO_ASN1 INT16_TO_OCTET_STRING
#define GTP_TEID_TO_ASN1 INT32_TO_OCTET_STRING
#define OCTET_STRING_TO_TAC OCTET_STRING_TO_INT16
#define OCTET_STRING_TO_MME_CODE OCTET_STRING_TO_INT8
#define OCTET_STRING_TO_M_TMSI   OCTET_STRING_TO_INT32
#define OCTET_STRING_TO_MME_GID  OCTET_STRING_TO_INT16
#define OCTET_STRING_TO_CSG_ID   OCTET_STRING_TO_INT27

/* Convert the IMSI contained by a char string NULL terminated to uint64_t */
#define IMSI_STRING_TO_IMSI64(sTRING, iMSI64_pTr) sscanf(sTRING, IMSI_64_FMT, iMSI64_pTr)
#define IMSI64_TO_STRING(iMSI64, sTRING) snprintf(sTRING, IMSI_BCD_DIGITS_MAX+1, IMSI_64_FMT, iMSI64)
imsi64_t imsi_to_imsi64(const imsi_t * const imsi);

#define IMSI_TO_STRING(iMsI_t_PtR,iMsI_sTr, MaXlEn) \
        do { \
          int l_i = 0; \
          int l_j = 0; \
          while((l_i < IMSI_BCD8_SIZE) && (l_j < MaXlEn - 1)){ \
            if((((iMsI_t_PtR)->u.value[l_i] & 0xf0) >> 4) > 9) \
              break; \
            sprintf(((iMsI_sTr) + l_j), "%u",(((iMsI_t_PtR)->u.value[l_i] & 0xf0) >> 4)); \
            l_j++; \
            if(((iMsI_t_PtR)->u.value[l_i] & 0xf) > 9 || (l_j >= MaXlEn - 1)) \
              break; \
            sprintf(((iMsI_sTr) + l_j), "%u", ((iMsI_t_PtR)->u.value[l_i] & 0xf)); \
            l_j++; \
            l_i++; \
          } \
          for(; l_j < MaXlEn; l_j++) \
              (iMsI_sTr)[l_j] = '\0'; \
        } while (0);\

#define IMEI_TO_STRING(iMeI_t_PtR,iMeI_sTr, MaXlEn) \
        {\
          int l_offset = 0;\
          int l_ret    = 0;\
          l_ret = snprintf(iMeI_sTr + l_offset, MaXlEn - l_offset, "%u%u%u%u%u%u%u%u",\
                  (iMeI_t_PtR)->u.num.tac1, (iMeI_t_PtR)->u.num.tac2,\
                  (iMeI_t_PtR)->u.num.tac3, (iMeI_t_PtR)->u.num.tac4,\
                  (iMeI_t_PtR)->u.num.tac5, (iMeI_t_PtR)->u.num.tac6,\
                  (iMeI_t_PtR)->u.num.tac7, (iMeI_t_PtR)->u.num.tac8);\
          if (l_ret > 0) {\
            l_offset += l_ret;\
            l_ret = snprintf(iMeI_sTr + l_offset, MaXlEn - l_offset, "%u%u%u%u%u%u",\
                (iMeI_t_PtR)->u.num.snr1, (iMeI_t_PtR)->u.num.snr2,\
                (iMeI_t_PtR)->u.num.snr3, (iMeI_t_PtR)->u.num.snr4,\
                (iMeI_t_PtR)->u.num.snr5, (iMeI_t_PtR)->u.num.snr6);\
          }\
          if (((iMeI_t_PtR)->u.num.parity != 0x0)   && (l_ret > 0)) {\
            l_offset += l_ret;\
            l_ret = snprintf(iMeI_sTr + l_offset, MaXlEn - l_offset, "%u", (iMeI_t_PtR)->u.num.cdsd);\
          }\
        }


void hexa_to_ascii(uint8_t *from, char *to, size_t length);

int ascii_to_hex(uint8_t *dst, const char *h);

#define UINT8_TO_BINARY_FMT "%c%c%c%c%c%c%c%c"
#define UINT8_TO_BINARY_ARG(bYtE) \
    ((bYtE) & 0x80 ? '1':'0'),\
    ((bYtE) & 0x40 ? '1':'0'),\
    ((bYtE) & 0x20 ? '1':'0'),\
    ((bYtE) & 0x10 ? '1':'0'),\
    ((bYtE) & 0x08 ? '1':'0'),\
    ((bYtE) & 0x04 ? '1':'0'),\
    ((bYtE) & 0x02 ? '1':'0'),\
    ((bYtE) & 0x01 ? '1':'0')

void tai_to_Tai(const tai_t * const tai, Tai_t * const Tai);

#endif /* FILE_CONVERSIONS_SEEN */

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

Source      commonDef.h

Version     0.1

Date        2012/02/27

Product     NAS stack

Subsystem   include

Author      Frederic Maurel

Description Contains global common definitions

*****************************************************************************/
#ifndef FILE_COMMONDEF_SEEN
#define FILE_COMMONDEF_SEEN

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>


/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

#define RETURNok        (0)
#define RETURNerror     (-1)


/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
-----------------------------------------------------------------------------
            Standard data type definitions
-----------------------------------------------------------------------------
*/
//-----------------------------------------------------------------------------
// GENERIC TYPES
//-----------------------------------------------------------------------------

#define BOOL_NOT(b) (b^true)


/*
-----------------------------------------------------------------------------
            Common NAS data type definitions
-----------------------------------------------------------------------------
*/

typedef uint8_t     Stat_t;     /* Registration status  */
typedef uint8_t     AcT_t;      /* Access Technology    */
typedef bool        ksi_t;      /* Key set identifier   */





#define NAS_IMSI2STR(iMsI_t_PtR,iMsI_sTr, MaXlEn) \
        { \
          int i = 0; \
          int j = 0; \
          while((i < IMSI_BCD8_SIZE) && (j < MaXlEn - 1)){ \
            if(((iMsI_t_PtR->u.value[i] & 0xf0) >> 4) > 9) \
              break; \
            sprintf((iMsI_sTr + j), "%u",((iMsI_t_PtR->u.value[i] & 0xf0) >> 4)); \
            j++; \
            if((iMsI_t_PtR->u.value[i] & 0xf) > 9 || (j >= MaXlEn - 1)) \
              break; \
            sprintf((iMsI_sTr + j), "%u", (iMsI_t_PtR->u.value[i] & 0xf)); \
            j++; \
            i++; \
          } \
          for(; j < MaXlEn; j++) \
              iMsI_sTr[j] = '\0'; \
        } \


#define NAS_IMSI2U64(iMsI_t_PtR,iMsI_u64) \
        {\
          uint64_t mUlT = 1; \
          iMsI_u64 = iMsI_t_PtR->u.num.digit1; \
          if (iMsI_t_PtR->u.num.digit15 != 0xf) { \
            iMsI_u64 = iMsI_t_PtR->u.num.digit15 + \
                       iMsI_t_PtR->u.num.digit14 *10 + \
                       iMsI_t_PtR->u.num.digit13 *100 + \
                       iMsI_t_PtR->u.num.digit12 *1000 + \
                       iMsI_t_PtR->u.num.digit11 *10000 + \
                       iMsI_t_PtR->u.num.digit10 *100000 + \
                       iMsI_t_PtR->u.num.digit9  *1000000 + \
                       iMsI_t_PtR->u.num.digit8  *10000000 + \
                       iMsI_t_PtR->u.num.digit7  *100000000;  \
            mUlT = 1000000000; \
          } else { \
            iMsI_u64 = iMsI_t_PtR->u.num.digit14  + \
                       iMsI_t_PtR->u.num.digit13 *10 + \
                       iMsI_t_PtR->u.num.digit12 *100 + \
                       iMsI_t_PtR->u.num.digit11 *1000 + \
                       iMsI_t_PtR->u.num.digit10 *10000 + \
                       iMsI_t_PtR->u.num.digit9  *100000 + \
                       iMsI_t_PtR->u.num.digit8  *1000000 + \
                       iMsI_t_PtR->u.num.digit7  *10000000;  \
            mUlT = 100000000; \
          } \
          if (iMsI_t_PtR->u.num.digit6 != 0xf) {\
              iMsI_u64 += iMsI_t_PtR->u.num.digit6 *mUlT;   \
              iMsI_u64 += iMsI_t_PtR->u.num.digit5 *mUlT*10; \
              iMsI_u64 += iMsI_t_PtR->u.num.digit4 *mUlT*100; \
              iMsI_u64 += iMsI_t_PtR->u.num.digit3 *mUlT*1000; \
              iMsI_u64 += iMsI_t_PtR->u.num.digit2 *mUlT*10000; \
              iMsI_u64 += iMsI_t_PtR->u.num.digit1 *mUlT*100000; \
          } else { \
              iMsI_u64 += iMsI_t_PtR->u.num.digit5 *mUlT;    \
              iMsI_u64 += iMsI_t_PtR->u.num.digit4 *mUlT*10;  \
              iMsI_u64 += iMsI_t_PtR->u.num.digit3 *mUlT*100;  \
              iMsI_u64 += iMsI_t_PtR->u.num.digit2 *mUlT*1000;  \
              iMsI_u64 += iMsI_t_PtR->u.num.digit1 *mUlT*10000;  \
          } \
        }

#define NAS_IMEI2STR(iMeI_t_PtR,iMeI_sTr, MaXlEn) \
        {\
          int l_offset = 0;\
          int l_ret    = 0;\
          l_ret = snprintf(iMeI_sTr + l_offset, MaXlEn - l_offset, "%u%u%u%u%u%u%u%u",\
                  iMeI_t_PtR->u.num.tac1, iMeI_t_PtR->u.num.tac2,\
                  iMeI_t_PtR->u.num.tac3, iMeI_t_PtR->u.num.tac4,\
                  iMeI_t_PtR->u.num.tac5, iMeI_t_PtR->u.num.tac6,\
                  iMeI_t_PtR->u.num.tac7, iMeI_t_PtR->u.num.tac8);\
          if (l_ret > 0) {\
            l_offset += l_ret;\
            l_ret = snprintf(iMeI_sTr + l_offset, MaXlEn - l_offset, "%u%u%u%u%u%u",\
                  iMeI_t_PtR->u.num.snr1, iMeI_t_PtR->u.num.snr2,\
                  iMeI_t_PtR->u.num.snr3, iMeI_t_PtR->u.num.snr4,\
                  iMeI_t_PtR->u.num.snr5, iMeI_t_PtR->u.num.snr6);\
          }\
          if ((iMeI_t_PtR->u.num.parity != 0x0)   && (l_ret > 0)) {\
            l_offset += l_ret;\
            l_ret = snprintf(iMeI_sTr + l_offset, MaXlEn - l_offset, "%u", iMeI_t_PtR->u.num.cdsd);\
          }\
        }


#define GUTI2STR_MAX_LENGTH 25
#define GUTI2STR(GuTi_PtR, GuTi_StR, MaXlEn) \
        {\
          int l_offset = 0;\
          int l_ret    = 0;\
          l_ret += snprintf((GuTi_StR) + l_offset,MaXlEn-l_offset, "%03u.",\
                  (GuTi_PtR)->gummei.plmn.mcc_digit3 * 100 +\
                  (GuTi_PtR)->gummei.plmn.mcc_digit2 * 10 +\
                  (GuTi_PtR)->gummei.plmn.mcc_digit1);\
          if (l_ret > 0) {\
            l_offset += l_ret;\
          }  else {\
            l_offset = MaXlEn;\
          }\
          if ((GuTi_PtR)->gummei.plmn.mnc_digit1 != 0xf) {\
              l_ret += snprintf((GuTi_StR) + l_offset,MaXlEn-l_offset, "%03u|%04x|%02x|%08x",\
                      (GuTi_PtR)->gummei.plmn.mnc_digit3 * 100 +\
                      (GuTi_PtR)->gummei.plmn.mnc_digit2 * 10 +\
                      (GuTi_PtR)->gummei.plmn.mnc_digit1,\
                      (GuTi_PtR)->gummei.mme_gid,\
                      (GuTi_PtR)->gummei.mme_code,\
                      (GuTi_PtR)->m_tmsi);\
          } else {\
              l_ret += snprintf((GuTi_StR) + l_offset,MaXlEn-l_offset, "%02u|%04x|%02x|%08x",\
                      (GuTi_PtR)->gummei.plmn.mnc_digit2 * 10 +\
                      (GuTi_PtR)->gummei.plmn.mnc_digit1,\
                      (GuTi_PtR)->gummei.mme_gid,\
                      (GuTi_PtR)->gummei.mme_code,\
                      (GuTi_PtR)->m_tmsi);\
          }\
        }




/* Checks PLMN validity */
#define PLMN_IS_VALID(plmn) (((plmn).mcc_digit1 &    \
                              (plmn).mcc_digit2 &    \
                              (plmn).mcc_digit3) != 0x0F)

/* Checks TAC validity */
#define TAC_IS_VALID(tac)   (((tac) != 0x0000) && ((tac) != 0xFFF0))

/* Checks TAI validity */
#define TAI_IS_VALID(tai)   (PLMN_IS_VALID((tai).plmn) &&   \
                             TAC_IS_VALID((tai).tac))
/*
 * A list of PLMNs
 */
#define PLMN_LIST_T(SIZE) struct {uint8_t n_plmns; plmn_t plmn[SIZE];}

/*
 * A list of TACs
 */
#define TAC_LIST_T(SIZE) struct {uint8_t n_tacs; TAC_t tac[SIZE];}

/*
 * A list of TAIs
 */
#define TAI_LIST_T(SIZE) struct {uint8_t list_type; uint8_t n_tais; tai_t tai[SIZE];}

/*
 * User notification callback, executed whenever a change of data with
 * respect of network information (e.g. network registration and/or
 * location change, new PLMN becomes available) is notified by the
 * EPS Mobility Management sublayer
 */
//typedef int (*emm_indication_callback_t) (Stat_t, tac_t, eci_t, AcT_t, const char*, size_t);



/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

#endif /* FILE_COMMONDEF_SEEN*/

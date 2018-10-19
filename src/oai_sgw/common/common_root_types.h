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

/*! \file common_root_types.h
  \brief
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_COMMON_ROOT_TYPES_SEEN
#define FILE_COMMON_ROOT_TYPES_SEEN

#include <stdint.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "bstrlib.h"
#include "common_dim.h"

#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
#define PRIORITY_LEVEL_MAX (15)
#define PRIORITY_LEVEL_MIN (1)
#define BEARERS_PER_UE     (11)
#define MSISDN_LENGTH      (15)
#define IMEI_DIGITS_MAX    (15)
#define IMEISV_DIGITS_MAX  (16)
#define MAX_APN_PER_UE     (5)

typedef uint16_t                 sctp_stream_id_t;
typedef uint32_t                 sctp_assoc_id_t;
typedef uint64_t                 enb_s1ap_id_key_t ;
#define MME_APP_ENB_S1AP_ID_KEY(kEy, eNb_Id, eNb_Ue_S1Ap_Id) do { kEy = (((enb_s1ap_id_key_t)eNb_Id) << 24) | eNb_Ue_S1Ap_Id; } while(0);
#define MME_APP_ENB_S1AP_ID_KEY2ENB_S1AP_ID(kEy) (enb_ue_s1ap_id_t)(((enb_s1ap_id_key_t)kEy) & ENB_UE_S1AP_ID_MASK)
#define MME_APP_ENB_S1AP_ID_KEY_FORMAT "0x%16"PRIx64

#define M_TMSI_BIT_MASK          UINT32_MAX


//------------------------------------------------------------------------------
// UE S1AP IDs

#define INVALID_ENB_UE_S1AP_ID_KEY   UINT32_MAX
#define ENB_UE_S1AP_ID_MASK          0x00FFFFFF
#define ENB_UE_S1AP_ID_FMT           "%06"PRIx32

#define MME_UE_S1AP_ID_FMT           "%"PRIx32
#define INVALID_MME_UE_S1AP_ID       0xFFFFFFFF          // You can pick any value between 0..2^32-1,
                                                     // all values are allowed. try to find another way (boolean is_valid for example)

//------------------------------------------------------------------------------
// TEIDs

typedef uint32_t                 teid_t;
#define TEID_FMT                "0x%"PRIx32
#define TEID_SCAN_FMT            SCNx32
typedef teid_t                   s11_teid_t;
typedef teid_t                   s10_teid_t;
typedef teid_t                   s1u_teid_t;
#define INVALID_TEID             0x00000000

//------------------------------------------------------------------------------
// IMSI

typedef uint64_t                 imsi64_t;
#define IMSI_64_FMT              "%"SCNu64
#define INVALID_IMSI64           (imsi64_t)0

//------------------------------------------------------------------------------
// PLMN

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GUTI
#define GUTI_FMT PLMN_FMT"|%04x|%02x|%08x"
#define GUTI_ARG(GuTi_PtR) \
  PLMN_ARG(&(GuTi_PtR)->gummei.plmn), \
  (GuTi_PtR)->gummei.mme_gid,\
  (GuTi_PtR)->gummei.mme_code,\
  (GuTi_PtR)->m_tmsi

//------------------------------------------------------------------------------
typedef uint8_t       ksi_t;
#define KSI_NO_KEY_AVAILABLE     0x07

typedef struct count_s{
  uint32_t spare:8;
  uint32_t overflow:16;
  uint32_t seq_num:8;
} count_t;

typedef uint8_t     AcT_t;      /* Access Technology    */


typedef enum {
  RAT_WLAN           = 0,
  RAT_VIRTUAL        = 1,
  RAT_UTRAN          = 1000,
  RAT_GERAN          = 1001,
  RAT_GAN            = 1002,
  RAT_HSPA_EVOLUTION = 1003,
  RAT_EUTRAN         = 1004,
  RAT_CDMA2000_1X    = 2000,
  RAT_HRPD           = 2001,
  RAT_UMB            = 2002,
  RAT_EHRPD          = 2003,
} rat_type_t;

typedef enum {
 MME_UPDATE_PROCEDURE       = 0,
 SGSN_UPDATE_PROCEDURE      = 1,
 SUBSCRIPTION_WITHDRAWAL    = 2,
 UPDATE_PROCEDURE_IWF       = 3,
 INITIAL_ATTACH_PROCEDURE   = 4,
}cancellation_type_t;

#define NUMBER_OF_RAT_TYPE 11

#define MEI_IMEI    0x0
#define MEI_IMEISV  0x1

typedef struct {
  uint8_t present;
  union {
    unsigned imei:15;
    unsigned imeisv:16;
  } choice;
} Mei_t;

typedef enum {
  SS_SERVICE_GRANTED = 0,
  SS_OPERATOR_DETERMINED_BARRING = 1,
  SS_MAX,
} subscriber_status_t;

typedef enum {
  NAM_PACKET_AND_CIRCUIT = 0,
  NAM_RESERVED           = 1,
  NAM_ONLY_PACKET        = 2,
  NAM_MAX,
} network_access_mode_t;

/* Only one type of address can be present at the same time
 * This type is applicable to IP address Information Element defined
 * in 3GPP TS 29.274 #8.9
 */
typedef struct {
#define GTP_IP_ADDR_v4  0x0
#define GTP_IP_ADDR_v6  0x1
  unsigned present:1;
  union {
    uint8_t v4[4];
    uint8_t v6[16];
  } address;
} gtp_ip_address_t;

typedef uint64_t bitrate_t;

typedef char*    APN_t;
typedef uint8_t  APNRestriction_t;
typedef uint8_t  DelayValue_t;
typedef uint8_t  priority_level_t;
#define PRIORITY_LEVEL_FMT                "0x%"PRIu8
#define PRIORITY_LEVEL_SCAN_FMT            SCNu8

typedef uint32_t SequenceNumber_t;
typedef uint32_t access_restriction_t;
typedef uint32_t context_identifier_t;
typedef uint32_t rau_tau_timer_t;


typedef uint32_t ard_t;
typedef int      pdn_cid_t;  // pdn connexion identity, related to esm protocol, sometimes type is mixed with int return code!!...
typedef uint8_t  proc_tid_t; // procedure transaction identity, related to esm protocol
#define ARD_UTRAN_NOT_ALLOWED               (1U)
#define ARD_GERAN_NOT_ALLOWED               (1U << 1)
#define ARD_GAN_NOT_ALLOWED                 (1U << 2)
#define ARD_I_HSDPA_EVO_NOT_ALLOWED         (1U << 3)
#define ARD_E_UTRAN_NOT_ALLOWED             (1U << 4)
#define ARD_HO_TO_NON_3GPP_NOT_ALLOWED      (1U << 5)
#define ARD_MAX                             (1U << 6)


typedef union {
  uint8_t imei[IMEI_DIGITS_MAX-1]; // -1 =  remove CD/SD digit
  uint8_t imeisv[IMEISV_DIGITS_MAX];
} me_identity_t;

typedef struct {
  bitrate_t br_ul;
  bitrate_t br_dl;
} ambr_t;


//-----------------
typedef enum {
  QCI_1 = 1,
  QCI_2 = 2,
  QCI_3 = 3,
  QCI_4 = 4,
  QCI_5 = 5,
  QCI_6 = 6,
  QCI_7 = 7,
  QCI_8 = 8,
  QCI_9 = 9,
  /* Values from 128 to 254 are operator specific.
   * Other are reserved.
   */
  QCI_MAX,
} qci_e;

typedef uint8_t qci_t;
#define QCI_FMT                "0x%"PRIu8
#define QCI_SCAN_FMT            SCNu8

typedef enum {
  PRE_EMPTION_CAPABILITY_ENABLED  = 0,
  PRE_EMPTION_CAPABILITY_DISABLED = 1,
  PRE_EMPTION_CAPABILITY_MAX,
} pre_emption_capability_t;

#define PRE_EMPTION_CAPABILITY_FMT                "0x%"PRIu8
#define PRE_EMPTION_CAPABILITY_SCAN_FMT            SCNu8

typedef enum {
  PRE_EMPTION_VULNERABILITY_ENABLED  = 0,
  PRE_EMPTION_VULNERABILITY_DISABLED = 1,
  PRE_EMPTION_VULNERABILITY_MAX,
} pre_emption_vulnerability_t;

#define PRE_EMPTION_VULNERABILITY_FMT                "0x%"PRIu8
#define PRE_EMPTION_VULNERABILITY_SCAN_FMT            SCNu8

typedef struct {
  priority_level_t            priority_level;
  pre_emption_vulnerability_t pre_emp_vulnerability;
  pre_emption_capability_t    pre_emp_capability;
} allocation_retention_priority_t;
typedef allocation_retention_priority_t arp_t;

typedef struct eps_subscribed_qos_profile_s {
  qci_t qci;
  allocation_retention_priority_t allocation_retention_priority;
} eps_subscribed_qos_profile_t;



typedef enum {
  DIAMETER_AUTHENTICATION_DATA_UNAVAILABLE = 4181,
  DIAMETER_ERROR_USER_UNKNOWN              = 5001,
  DIAMETER_ERROR_ROAMING_NOT_ALLOWED       = 5004,
  DIAMETER_ERROR_UNKNOWN_EPS_SUBSCRIPTION  = 5420,
  DIAMETER_ERROR_RAT_NOT_ALLOWED           = 5421,
  DIAMETER_ERROR_EQUIPMENT_UNKNOWN         = 5422,
  DIAMETER_ERROR_UNKOWN_SERVING_NODE       = 5423,
} s6a_experimental_result_t;

typedef enum {
  DIAMETER_SUCCESS = 2001,
} s6a_base_result_t;

typedef struct {
#define S6A_RESULT_BASE         0x0
#define S6A_RESULT_EXPERIMENTAL 0x1
  unsigned present:1;

  union {
    /* Experimental result as defined in 3GPP TS 29.272 */
    s6a_experimental_result_t experimental;
    /* Diameter basics results as defined in RFC 3588 */
    s6a_base_result_t         base;
  } choice;
} s6a_result_t;

#ifdef __cplusplus
}
#endif
#endif /* FILE_COMMON_ROOT_TYPES_SEEN */

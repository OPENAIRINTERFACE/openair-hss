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

/*****************************************************************************
Source      emmData.h

Version     0.1

Date        2012/10/18

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines internal private data handled by EPS Mobility
        Management sublayer.

*****************************************************************************/
#ifndef FILE_EMMDATA_SEEN
#define FILE_EMMDATA_SEEN

#include "commonDef.h"
#include "networkDef.h"
#include "securityDef.h"

#include "OctetString.h"
#include "nas_timer.h"

#include "esmData.h"

#include "emm_fsm.h"
#include "mme_api.h"
#include "obj_hashtable.h"
#include "hashtable.h"

#include "UeNetworkCapability.h"
#include "MsNetworkCapability.h"
#include "DrxParameter.h"
#include "EpsBearerContextStatus.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/


/* Checks Mobile Country Code equality */
#define MCCS_ARE_EQUAL(n1, n2)  (((n1).mcc_digit1 == (n2).mcc_digit1) && \
                                 ((n1).mcc_digit2 == (n2).mcc_digit2) && \
                                 ((n1).mcc_digit3 == (n2).mcc_digit3))

/* Checks Mobile Network Code equality */
#define MNCS_ARE_EQUAL(n1, n2)  (((n1).mnc_digit1 == (n2).mnc_digit1) &&  \
                                 ((n1).mnc_digit2 == (n2).mnc_digit2) &&  \
                                 ((n1).mnc_digit3 == (n2).mnc_digit3))

/* Checks PLMNs equality */
#define PLMNS_ARE_EQUAL(p1, p2) ((MCCS_ARE_EQUAL((p1),(p2))) && \
                                 (MNCS_ARE_EQUAL((p1),(p2))))

/* Checks TAIs equality */
#define TAIS_ARE_EQUAL(t1, t2)  ((PLMNS_ARE_EQUAL((t1).plmn,(t2).plmn)) && \
                                 ((t1).tac == (t2).tac))

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 * EPS NAS security context handled by EPS Mobility Management sublayer in
 * the UE and in the MME
 * --------------------------------------------------------------------------
 */
/* Type of security context */
typedef enum {
  EMM_KSI_NOT_AVAILABLE = 0,
  EMM_KSI_NATIVE,
  EMM_KSI_MAPPED
} emm_ksi_t;

/* EPS NAS security context structure */
typedef struct emm_security_context_s {
  emm_ksi_t type;     /* Type of security context        */
  int eksi;           /* NAS key set identifier for E-UTRAN      */
  OctetString kasme;      /* ASME security key (native context)      */
  //OctetString ksgsn;    /* SGSN security key (mapped context)      */
  OctetString knas_enc;   /* NAS cyphering key               */
  OctetString knas_int;   /* NAS integrity key               */
  struct count_s{
    uint32_t spare:8;
    uint32_t overflow:16;
    uint32_t seq_num:8;
  } dl_count, ul_count;   /* Downlink and uplink count parameters    */
  struct {
    uint8_t eps_encryption;   /* algorithm used for ciphering            */
    uint8_t eps_integrity;    /* algorithm used for integrity protection */
    uint8_t umts_encryption;  /* algorithm used for ciphering            */
    uint8_t umts_integrity;   /* algorithm used for integrity protection */
    uint8_t gprs_encryption;  /* algorithm used for ciphering            */
    uint8_t umts_present:1;
    uint8_t gprs_present:1;
  } capability;       /* UE network capability           */
  struct {
    uint8_t encryption:4;   /* algorithm used for ciphering           */
    uint8_t integrity:4;    /* algorithm used for integrity protection */
  } selected_algorithms;       /* MME selected algorithms                */

  // Requirement MME24.301R10_4.4.4.3_2 (DETACH REQUEST (if sent before security has been activated);)
  uint8_t   activated;
} emm_security_context_t;


/*
 * --------------------------------------------------------------------------
 *  EMM internal data handled by EPS Mobility Management sublayer in the MME
 * --------------------------------------------------------------------------
 */
/*
 * Structure of the EMM context established by the network for a particular UE
 * ---------------------------------------------------------------------------
 */
typedef struct emm_data_context_s {
  mme_ue_s1ap_id_t ue_id;        /* UE identifier                                   */
  bool             is_dynamic;  /* Dynamically allocated context indicator         */
  bool             is_attached; /* Attachment indicator                            */
  bool             is_emergency;/* Emergency bearer services indicator             */

  imsi_t          *imsi;        /* The IMSI provided by the UE or the MME          */
  imei_t          *imei;        /* The IMEI provided by the UE                     */
  imeisv_t        *imeisv;      /* The IMEISV provided by the UE                   */
  int              guti_is_new; /* New GUTI indicator                              */
  guti_t          *guti;        /* The GUTI assigned to the UE                     */
  guti_t          *old_guti;    /* The old GUTI                                    */
  tai_list_t       tai_list;    /* TACs the the UE is registered to                */
  tai_t            last_visited_registered_tai;
  /*int            n_tacs;       * Number of consecutive tracking areas the UE is
                             * registered to                                   /
  tac_t            tac;           * Code of the first tracking area the UE is
                             * registered to                                   */

  int             ksi;          /* Security key set identifier provided by the UE  */
  int             eea;          /* EPS encryption algorithms supported by the UE   */
  int             eia;          /* EPS integrity algorithms supported by the UE    */
  int             ucs2;         /* UCS2 Alphabet*/
  int             uea;          /* UMTS encryption algorithms supported by the UE  */
  int             uia;          /* UMTS integrity algorithms supported by the UE   */
  int             gea;          /* GPRS encryption algorithms supported by the UE  */
  int             umts_present; /* For encoding ue network capabilities (variable size)*/
  int             gprs_present; /* For encoding ue network capabilities (variable size)*/

  UeNetworkCapability    *ue_network_capability_ie; /* stored TAU Request IE Requirement MME24.301R10_5.5.3.2.4_2*/
  MsNetworkCapability    *ms_network_capability_ie; /* stored TAU Request IE Requirement MME24.301R10_5.5.3.2.4_2*/
  DrxParameter           *drx_parameter;            /* stored TAU Request IE Requirement MME24.301R10_5.5.3.2.4_4*/
  EpsBearerContextStatus *eps_bearer_context_status;/* stored TAU Request IE Requirement MME24.301R10_5.5.3.2.4_5*/

  auth_vector_t vector;/* EPS authentication vector                            */
  emm_security_context_t *security;    /* Current EPS NAS security context     */

  // Requirement MME24.301R10_4.4.2.1_2
  emm_security_context_t *non_current_security;    /* Non current EPS NAS security context     */

  OctetString esm_msg;      /* ESM message contained within the initial request*/
  int         emm_cause;    /* EMM failure cause code                          */

  emm_fsm_state_t    _emm_fsm_status;

  struct nas_timer_t T3450; /* EMM message retransmission timer */
  struct nas_timer_t T3460; /* Authentication timer         */
  struct nas_timer_t T3470; /* Identification timer         */

  esm_data_context_t esm_data_ctx;

#  define EMM_CN_SAP_BUFFER_SIZE 4096
  char                             emm_cn_sap_buffer[EMM_CN_SAP_BUFFER_SIZE];

} emm_data_context_t;

/*
 * Structure of the EMM data
 * -------------------------
 */
typedef struct emm_data_s {
  /*
   * MME configuration
   * -----------------
   */
  mme_api_emm_config_t conf;
  /*
   * EMM contexts
   * ------------
   */
  hash_table_ts_t    *ctx_coll_ue_id;// key is emm ue id, data is struct emm_data_context_s
  obj_hash_table_t    *ctx_coll_guti; // key is guti, data is emm ue id (unsigned int)
} emm_data_t;

struct emm_data_context_s *emm_data_context_get(
  emm_data_t *emm_data, const mme_ue_s1ap_id_t ueid);

struct emm_data_context_s *emm_data_context_get_by_guti(
  emm_data_t *emm_data, guti_t *guti);

struct emm_data_context_s *emm_data_context_remove(
  emm_data_t *_emm_data, struct emm_data_context_s *elm);

int  emm_data_context_add(emm_data_t *emm_data, struct emm_data_context_s *elm);
void free_emm_data_context(struct emm_data_context_s * const emm_ctx);
void emm_data_context_dump(const struct emm_data_context_s * const elm_pP);

void emm_data_context_dump_all(void);


/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *      EPS mobility management data (used within EMM only)
 * --------------------------------------------------------------------------
 */
emm_data_t _emm_data;



/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

#endif /* FILE_EMMDATA_SEEN*/

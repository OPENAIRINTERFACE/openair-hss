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
  Source      mme_api.c

  Version     0.1

  Date        2013/02/28

  Product     NAS stack

  Subsystem   Application Programming Interface

  Author      Frederic Maurel

  Description Implements the API used by the NAS layer running in the MME
        to interact with a Mobility Management Entity

*****************************************************************************/


#include "common_types.h"
#include "mme_api.h"
#include "assertions.h"
#include "sgw_ie_defs.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "log.h"

#include <string.h>             // memcpy

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/
extern mme_app_desc_t                   mme_app_desc;

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* Maximum number of PDN connections the MME may simultaneously support */
#define MME_API_PDN_MAX         10

/* MME group identifier */
#define MME_API_MME_GID         0x0102

/* MME code */
#define MME_API_MME_CODE        0x12

/* Default APN */
static const OctetString                mme_api_default_apn = {
  /*
   * LW: apn seems to be coded using a one byte size field before each part of the name
   */
#if 1
  15, (uint8_t *) ("\x0e" "www.eurecom.fr")
#else
  35, (uint8_t *) ("\x08" "internet" "\x02" "v4" "\x03" "pft" "\x06" "mnc092" "\x06" "mcc208" "\x04" "gprs")
#endif
};

/* APN configured for emergency bearer services */
static const OctetString                mme_api_emergency_apn = {
  19, (uint8_t *) ("\x12" "www.eurecom_sos.fr")
};

/* Public Land Mobile Network identifier */
static const plmn_t                     mme_api_plmn = { 0, 2, 0xf, 8, 0, 1 };  // 20810

/* Number of concecutive tracking areas */
#define MME_API_NB_TACS     4
/* Code of the first tracking area belonging to the PLMN */
#define MME_API_FIRST_TAC   0x0001


/* Authentication parameter RAND */
static const uint8_t                    _mme_api_rand[AUTH_RAND_SIZE] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x02, 0x03, 0x04
};

/* Authentication parameter AUTN */
static const uint8_t                    _mme_api_autn[AUTH_AUTN_SIZE] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x05, 0x04, 0x03, 0x02, 0x00,
  0x00, 0x00, 0x00, 0x00
};

/* Authentication response parameter */
static const uint8_t                    _mme_api_xres[AUTH_XRES_SIZE] = {
  0x67, 0x70, 0x3a, 0x31, 0xf2, 0x2a, 0x2d, 0x51, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

static mme_api_ip_version_t             _mme_api_ip_capability = MME_API_IPV4V6_ADDR;

/* Pool of IPv4 addresses */
static uint8_t                          _mme_api_ipv4_addr[MME_API_PDN_MAX][4] = {
  {0xC0, 0xA8, 0x02, 0x3C},     /* 192.168.02.60    */
  {0xC0, 0xA8, 0x0C, 0xBB},     /* 192.168.12.187   */
  {0xC0, 0xA8, 0x0C, 0xBC},     /* 192.168.12.188   */
  {0xC0, 0xA8, 0x0C, 0xBD},     /* 192.168.12.189   */
  {0xC0, 0xA8, 0x0C, 0xBE},     /* 192.168.12.190   */
  {0xC0, 0xA8, 0x0C, 0xBF},     /* 192.168.12.191   */
  {0xC0, 0xA8, 0x0C, 0xC0},     /* 192.168.12.192   */
  {0xC0, 0xA8, 0x0C, 0xC1},     /* 192.168.12.193   */
  {0xC0, 0xA8, 0x0C, 0xC2},     /* 192.168.12.194   */
  {0xC0, 0xA8, 0x0C, 0xC3},     /* 192.168.12.195   */
};

/* Pool of IPv6 addresses */
static uint8_t                          _mme_api_ipv6_addr[MME_API_PDN_MAX][8] = {
  /*
   * FE80::221:70FF:C0A8:023C/64
   */
  {0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x02, 0x3C},
  /*
   * FE80::221:70FF:C0A8:0CBB/64
   */
  {0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xBB},
  /*
   * FE80::221:70FF:C0A8:0CBC/64
   */
  {0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xBC},
  /*
   * FE80::221:70FF:C0A8:0CBD/64
   */
  {0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xBD},
  /*
   * FE80::221:70FF:C0A8:0CBE/64
   */
  {0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xBE},
  /*
   * FE80::221:70FF:C0A8:0CBF/64
   */
  {0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xBF},
  /*
   * FE80::221:70FF:C0A8:0CC0/64
   */
  {0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xC0},
  /*
   * FE80::221:70FF:C0A8:0CC1/64
   */
  {0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xC1},
  /*
   * FE80::221:70FF:C0A8:0CC2/64
   */
  {0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xC2},
  /*
   * FE80::221:70FF:C0A8:0CC3/64
   */
  {0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xC3},
};

/* Pool of IPv4v6 addresses */
static uint8_t                          _mme_api_ipv4v6_addr[MME_API_PDN_MAX][12] = {
  /*
   * 192.168.02.60, FE80::221:70FF:C0A8:023C/64
   */
  {0xC0, 0xA8, 0x02, 0x3C, 0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x02, 0x3C},
  /*
   * 192.168.12.187, FE80::221:70FF:C0A8:0CBB/64
   */
  {0xC0, 0xA8, 0x0C, 0xBB, 0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xBB},
  /*
   * 192.168.12.188, FE80::221:70FF:C0A8:0CBC/64
   */
  {0xC0, 0xA8, 0x0C, 0xBC, 0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xBC},
  /*
   * 192.168.12.189, FE80::221:70FF:C0A8:0CBD/64
   */
  {0xC0, 0xA8, 0x0C, 0xBD, 0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xBD},
  /*
   * 192.168.12.189, FE80::221:70FF:C0A8:0CBE/64
   */
  {0xC0, 0xA8, 0x0C, 0xBD, 0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xBE},
  /*
   * 192.168.12.189, FE80::221:70FF:C0A8:0CBF/64
   */
  {0xC0, 0xA8, 0x0C, 0xBD, 0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xBF},
  /*
   * 192.168.12.189, FE80::221:70FF:C0A8:0CC0/64
   */
  {0xC0, 0xA8, 0x0C, 0xBD, 0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xC0},
  /*
   * 192.168.12.189, FE80::221:70FF:C0A8:0CC1/64
   */
  {0xC0, 0xA8, 0x0C, 0xBD, 0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xC1},
  /*
   * 192.168.12.189, FE80::221:70FF:C0A8:0CC2/64
   */
  {0xC0, 0xA8, 0x0C, 0xBD, 0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xC2},
  /*
   * 192.168.12.189, FE80::221:70FF:C0A8:0CC3/64
   */
  {0xC0, 0xA8, 0x0C, 0xBD, 0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xC3},
};

static const OctetString                _mme_api_pdn_addr[MME_API_ADDR_MAX][MME_API_PDN_MAX] = {
  {
   /*
    * IPv4 network capability
    */
   {4, _mme_api_ipv4_addr[0]},
   {4, _mme_api_ipv4_addr[1]},
   {4, _mme_api_ipv4_addr[2]},
   {4, _mme_api_ipv4_addr[3]},
   {4, _mme_api_ipv4_addr[4]},
   {4, _mme_api_ipv4_addr[5]},
   {4, _mme_api_ipv4_addr[6]},
   {4, _mme_api_ipv4_addr[7]},
   {4, _mme_api_ipv4_addr[8]},
   {4, _mme_api_ipv4_addr[9]},
   },
  {
   /*
    * IPv6 network capability
    */
   {8, _mme_api_ipv6_addr[0]},
   {8, _mme_api_ipv6_addr[1]},
   {8, _mme_api_ipv6_addr[2]},
   {8, _mme_api_ipv6_addr[3]},
   {8, _mme_api_ipv6_addr[4]},
   {8, _mme_api_ipv6_addr[5]},
   {8, _mme_api_ipv6_addr[6]},
   {8, _mme_api_ipv6_addr[7]},
   {8, _mme_api_ipv6_addr[8]},
   {8, _mme_api_ipv6_addr[9]},
   },
  {
   /*
    * IPv4v6 network capability
    */
   {12, _mme_api_ipv4v6_addr[0]},
   {12, _mme_api_ipv4v6_addr[1]},
   {12, _mme_api_ipv4v6_addr[2]},
   {12, _mme_api_ipv4v6_addr[3]},
   {12, _mme_api_ipv4v6_addr[4]},
   {12, _mme_api_ipv4v6_addr[5]},
   {12, _mme_api_ipv4v6_addr[6]},
   {12, _mme_api_ipv4v6_addr[7]},
   {12, _mme_api_ipv4v6_addr[8]},
   {12, _mme_api_ipv4v6_addr[9]},
   },
};

/* Subscribed QCI */
#define MME_API_QCI     3

/* Data bit rates */
#define MME_API_BIT_RATE_64K    0x40
#define MME_API_BIT_RATE_128K   0x48
#define MME_API_BIT_RATE_512K   0x78
#define MME_API_BIT_RATE_1024K  0x87

/* Total number of PDN connections (should not exceed MME_API_PDN_MAX) */
static int                              _mme_api_pdn_id = 0;

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    mme_api_get_emm_config()                                  **
 **                                                                        **
 ** Description: Retreives MME configuration data related to EPS mobility  **
 **      management                                                **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
mme_api_get_emm_config (
  mme_api_emm_config_t * config,
  mme_config_t * mme_config_p)
{
  int                                     i;
  OAILOG_FUNC_IN (LOG_NAS);
  AssertFatal (mme_config_p->served_tai.nb_tai >= 1, "No PLMN configured");
  AssertFatal (mme_config_p->gummei.nb_mmec >= 1, "No MME Code configured");
  AssertFatal (mme_config_p->gummei.nb_mme_gid >= 1, "No MME Group ID configured");
  config->gummei.plmn.mcc_digit1 = (mme_config_p->served_tai.plmn_mcc[0] / 100) % 10;
  config->gummei.plmn.mcc_digit2 = (mme_config_p->served_tai.plmn_mcc[0] / 10) % 10;
  config->gummei.plmn.mcc_digit3 = mme_config_p->served_tai.plmn_mcc[0] % 10;

  if (mme_config_p->served_tai.plmn_mnc_len[0] == 2) {
    config->gummei.plmn.mnc_digit1 = (mme_config_p->served_tai.plmn_mnc[0] / 10) % 10;
    config->gummei.plmn.mnc_digit2 = mme_config_p->served_tai.plmn_mnc[0] % 10;
    config->gummei.plmn.mnc_digit3 = 0xf;
  } else if (mme_config_p->served_tai.plmn_mnc_len[0] == 3) {
    config->gummei.plmn.mnc_digit1 = (mme_config_p->served_tai.plmn_mnc[0] / 100) % 10;
    config->gummei.plmn.mnc_digit2 = (mme_config_p->served_tai.plmn_mnc[0] / 10) % 10;
    config->gummei.plmn.mnc_digit3 = mme_config_p->served_tai.plmn_mnc[0] % 10;
  } else {
    AssertFatal ((mme_config_p->served_tai.plmn_mnc_len[0] >= 2) && (mme_config_p->served_tai.plmn_mnc_len[0] <= 3), "BAD MNC length for GUMMEI");
  }

  config->tai_list.n_tais = 0;
  for (i = 0; i < mme_config_p->served_tai.nb_tai; i++) {
    config->tai_list.tai[i].plmn.mcc_digit1 = (mme_config_p->served_tai.plmn_mcc[i] / 100) % 10;
    config->tai_list.tai[i].plmn.mcc_digit2 = (mme_config_p->served_tai.plmn_mcc[i] / 10) % 10;
    config->tai_list.tai[i].plmn.mcc_digit3 = mme_config_p->served_tai.plmn_mcc[i] % 10;

    if (mme_config_p->served_tai.plmn_mnc_len[0] == 2) {
      config->tai_list.tai[i].plmn.mnc_digit1 = (mme_config_p->served_tai.plmn_mnc[0] / 10) % 10;
      config->tai_list.tai[i].plmn.mnc_digit2 = mme_config_p->served_tai.plmn_mnc[0] % 10;
      config->tai_list.tai[i].plmn.mnc_digit3 = 0xf;
    } else if (mme_config_p->served_tai.plmn_mnc_len[0] == 3) {
      config->tai_list.tai[i].plmn.mnc_digit1 = (mme_config_p->served_tai.plmn_mnc[0] / 100) % 10;
      config->tai_list.tai[i].plmn.mnc_digit2 = (mme_config_p->served_tai.plmn_mnc[0] / 10) % 10;
      config->tai_list.tai[i].plmn.mnc_digit3 = mme_config_p->served_tai.plmn_mnc[0] % 10;
    } else {
      AssertFatal ((mme_config_p->served_tai.plmn_mnc_len[0] >= 2) && (mme_config_p->served_tai.plmn_mnc_len[0] <= 3), "BAD MNC length for GUMMEI");
    }
    config->tai_list.tai[i].tac            = mme_config_p->served_tai.tac[i];
    config->tai_list.n_tais += 1;
  }
  config->tai_list.list_type = mme_config_p->served_tai.list_type;
  /*
   * SR: this config param comes from MME global config
   */
  // hardcoded
  config->eps_network_feature_support = EPS_NETWORK_FEATURE_SUPPORT_CS_LCS_LOCATION_SERVICES_VIA_CS_DOMAIN_NOT_SUPPORTED;
  //config->eps_network_feature_support = EPS_NETWORK_FEATURE_SUPPORT_CS_LCS_LOCATION_SERVICES_VIA_CS_DOMAIN_NOT_SUPPORTED
  if (mme_config_p->eps_network_feature_support.emergency_bearer_services_in_s1_mode != 0) {
    config->eps_network_feature_support |= EPS_NETWORK_FEATURE_SUPPORT_EMERGENCY_BEARER_SERVICES_IN_S1_MODE_SUPPORTED;
  }
  if (mme_config_p->eps_network_feature_support.ims_voice_over_ps_session_in_s1 != 0) {
    config->eps_network_feature_support |= EPS_NETWORK_FEATURE_SUPPORT_IMS_VOICE_OVER_PS_SESSION_IN_S1_SUPPORTED;
  }
  if (mme_config_p->eps_network_feature_support.location_services_via_epc != 0) {
    config->eps_network_feature_support |= EPS_NETWORK_FEATURE_SUPPORT_LOCATION_SERVICES_VIA_EPC_SUPPORTED;
  }
  if (mme_config_p->eps_network_feature_support.extended_service_request != 0) {
    config->eps_network_feature_support |= EPS_NETWORK_FEATURE_SUPPORT_EXTENDED_SERVICE_REQUEST_SUPPORTED;
  }

  if (mme_config_p->unauthenticated_imsi_supported != 0) {
    config->features |= MME_API_UNAUTHENTICATED_IMSI;
  }

  for (i = 0; i < 8; i++) {
    config->prefered_integrity_algorithm[i] = mme_config_p->nas_config.prefered_integrity_algorithm[i];
    config->prefered_ciphering_algorithm[i] = mme_config_p->nas_config.prefered_ciphering_algorithm[i];
  }
  OAILOG_FUNC_RETURN (LOG_NAS, RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    mme_api_get_config()                                      **
 **                                                                        **
 ** Description: Retreives MME configuration data related to EPS session   **
 **      management                                                **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
mme_api_get_esm_config (
  mme_api_esm_config_t * config)
{
  OAILOG_FUNC_IN (LOG_NAS);

  if (_mme_api_ip_capability == MME_API_IPV4_ADDR) {
    config->features = MME_API_IPV4;
  } else if (_mme_api_ip_capability == MME_API_IPV6_ADDR) {
    config->features = MME_API_IPV6;
  } else if (_mme_api_ip_capability == MME_API_IPV4V6_ADDR) {
    config->features = MME_API_IPV4 | MME_API_IPV6;
  } else {
    config->features = 0;
  }

  OAILOG_FUNC_RETURN (LOG_NAS, RETURNok);
}

/*
 *
 *  Name:    mme_api_notify_new_guti()
 *
 *  Description: Notify the MME of a generated GUTI for a UE(not spec).
 *
 *  Inputs:
 *         ueid:      nas_ue id
 *         guti:      EPS Globally Unique Temporary UE Identity
 *  Return:    RETURNok, RETURNerror
 *
 */
int
mme_api_notify_new_guti (
  const mme_ue_s1ap_id_t id,
  guti_t * const guti)
{
  ue_context_t                           *ue_context = NULL;

  OAILOG_FUNC_IN (LOG_NAS);
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, id);


  if ( ue_context) {
    mme_ue_context_update_coll_keys (&mme_app_desc.mme_ue_contexts,
        ue_context,
        ue_context->enb_ue_s1ap_id,
        id,
        ue_context->imsi,
        ue_context->mme_s11_teid,
        guti);
    OAILOG_FUNC_RETURN (LOG_NAS, RETURNok);
  }

  OAILOG_FUNC_RETURN (LOG_NAS, RETURNerror);
}

/*
 *
 *  Name:    mme_api_notify_end_ue_s1ap_id_changed()
 *
 *  Description: Notify the MME of a change in ue id (reconnection).
 *
 *  Inputs:
 *         old_ueid:      old nas_ue id
 *         new_ueid:      new nas_ue id
 *         mme_ue_s1ap_id:   nas ue id
 *  Return:    RETURNok, RETURNerror
 *
 */
int
mme_api_notified_new_ue_s1ap_id_association (
    const enb_ue_s1ap_id_t enb_ue_s1ap_id,
    const mme_ue_s1ap_id_t mme_ue_s1ap_id)
{
  ue_context_t                           *ue_context = NULL;

  OAILOG_FUNC_IN (LOG_NAS);
  ue_context = mme_ue_context_exists_enb_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, enb_ue_s1ap_id);

  if ( ue_context) {
    mme_ue_context_notified_new_ue_s1ap_id_association(&mme_app_desc.mme_ue_contexts, ue_context, enb_ue_s1ap_id, mme_ue_s1ap_id);
    // optimistic return:
    // TODO return value from mme_ue_context_notified_new_ue_s1ap_id_association
    OAILOG_FUNC_RETURN (LOG_NAS, RETURNok);
  }

  OAILOG_FUNC_RETURN (LOG_NAS, RETURNerror);
}


/*
 *
 *  Name:    mme_api_identify_guti()
 *
 *  Description: Requests the MME to identify the UE using the specified
 *       GUTI. If the UE is known by the MME (a Mobility Management context exists for this  UE  in  the  MME),
 *       its security context is returned.
 *
 * Inputs:  guti:      EPS Globally Unique Temporary UE Identity
 *      Others:    None
 *
 * Outputs:     vector:    The EPS authentication vector of the UE if known by the network; NULL otherwise.
 *      Return:    RETURNok, RETURNerror
 *      Others:    None
 */
int
mme_api_identify_guti (
  const guti_t * guti,
  auth_vector_t * vector)
{
  ue_context_t                           *ue_context = NULL;

  OAILOG_FUNC_IN (LOG_NAS);
  ue_context = mme_ue_context_exists_guti (&mme_app_desc.mme_ue_contexts, guti);

  if ( ue_context) {
    if ( ue_context->vector_in_use) {
      memcpy (vector->rand, ue_context->vector_in_use->rand, AUTH_RAND_SIZE);
      memcpy (vector->autn, ue_context->vector_in_use->autn, AUTH_AUTN_SIZE);
      memcpy (vector->xres, ue_context->vector_in_use->xres.data, ue_context->vector_in_use->xres.size);
      OAILOG_FUNC_RETURN (LOG_NAS, RETURNok);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS, RETURNerror);
}

/*
 * Name:    mme_api_identify_imsi()
 *
 * Description: Requests the MME to identify the UE using the specified IMSI.
 *      If the UE is known by the MME (a Mobility Management context exists for
 *      this  UE  in  the  MME), its security context is returned.
 *
 * Inputs:  imsi:      International Mobile Subscriber Identity
 *   Others:    None
 *
 * Outputs:     vector:    The EPS authentication vector of the UE if known by the network; NULL otherwise.
 *   Return:    RETURNok, RETURNerror
 *   Others:    None
 */
int
mme_api_identify_imsi (
  const imsi_t * imsi,
  auth_vector_t * vector)
{
  ue_context_t                           *ue_context = NULL;
  mme_app_imsi_t                          mme_imsi = {.length = 0};

  OAILOG_FUNC_IN (LOG_NAS);

  mme_app_convert_imsi_to_imsi_mme (&mme_imsi, imsi);

  ue_context = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, mme_imsi);

  if ( ue_context) {
    if ( ue_context->vector_in_use) {
      memcpy (vector->rand, ue_context->vector_in_use->rand, AUTH_RAND_SIZE);
      memcpy (vector->autn, ue_context->vector_in_use->autn, AUTH_AUTN_SIZE);
      memcpy (vector->xres, ue_context->vector_in_use->xres.data, ue_context->vector_in_use->xres.size);
      OAILOG_FUNC_RETURN (LOG_NAS, RETURNok);
    }
  }

  OAILOG_FUNC_RETURN (LOG_NAS, RETURNerror);
}

/*
 *
 * Name:    mme_api_identify_imei()
 *
 * Description: Requests the MME to identify the UE using the specified
 *      IMEI. If the UE is known by the MME (a Mobility Manage-
 *      ment context exists for this  UE  in  the  MME), its se-
 *      curity context is returned.
 *
 * Inputs:  imei:      International Mobile Equipment Identity
 *      Others:    None
 *
 * Outputs:     vector:    The EPS authentication vector of the UE if
 *             known by the network; NULL otherwise.
 *      Return:    RETURNok, RETURNerror
 *      Others:    None
 *
 */
int
mme_api_identify_imei (
  const imei_t * imei,
  auth_vector_t * vector)
{
  int                                     rc = RETURNerror;

  OAILOG_FUNC_IN (LOG_NAS);
  OAILOG_FUNC_RETURN (LOG_NAS, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    mme_api_new_guti()                                        **
 **                                                                        **
 ** Description: Requests the MME to assign a new GUTI to the UE identi-   **
 **      fied by the given IMSI.                                   **
 **                                                                        **
 ** Description: Requests the MME to assign a new GUTI to the UE identi-   **
 **      fied by the given IMSI and returns the list of consecu-   **
 **      tive tracking areas the UE is registered to.              **
 **                                                                        **
 ** Inputs:  imsi:      International Mobile Subscriber Identity   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     guti:      The new assigned GUTI                      **
 **      tai_list:       TAIs belonging to the PLMN                                **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
mme_api_new_guti (
  const imsi_t * imsi,
  guti_t * guti,
  tai_list_t * tai_list)
{
  ue_context_t                           *ue_context = NULL;
  mme_app_imsi_t                          mme_imsi = {.length = 0};

  OAILOG_FUNC_IN (LOG_NAS);

  mme_app_convert_imsi_to_imsi_mme (&mme_imsi, imsi);

  ue_context = mme_ue_context_exists_imsi (&mme_app_desc.mme_ue_contexts, mme_imsi);

  if ( ue_context) {
    char                                    guti_str[GUTI2STR_MAX_LENGTH];
    int                                     i,j;
    plmn_t                                  plmn;

    memcpy (guti, &ue_context->guti, sizeof (*guti));

    GUTI2STR (guti, guti_str, GUTI2STR_MAX_LENGTH);

    plmn.mcc_digit1 = guti->gummei.plmn.mcc_digit1;
    plmn.mcc_digit2 = guti->gummei.plmn.mcc_digit2;
    plmn.mcc_digit3 = guti->gummei.plmn.mcc_digit3;
    plmn.mnc_digit1 = guti->gummei.plmn.mnc_digit1;
    plmn.mnc_digit2 = guti->gummei.plmn.mnc_digit2;
    plmn.mnc_digit3 = guti->gummei.plmn.mnc_digit3;
    j = 0;
    for (i=0; i < _emm_data.conf.tai_list.n_tais; i++) {
      if ((_emm_data.conf.tai_list.tai[i].plmn.mcc_digit1 == plmn.mcc_digit1) &&
          (_emm_data.conf.tai_list.tai[i].plmn.mcc_digit2 == plmn.mcc_digit2) &&
          (_emm_data.conf.tai_list.tai[i].plmn.mcc_digit3 == plmn.mcc_digit3) &&
          (_emm_data.conf.tai_list.tai[i].plmn.mnc_digit1 == plmn.mnc_digit1) &&
          (_emm_data.conf.tai_list.tai[i].plmn.mnc_digit2 == plmn.mnc_digit2) &&
          (_emm_data.conf.tai_list.tai[i].plmn.mnc_digit3 == plmn.mnc_digit3) ) {

      }
      tai_list->tai[j].plmn.mcc_digit1 = plmn.mcc_digit1;
      tai_list->tai[j].plmn.mcc_digit2 = plmn.mcc_digit2;
      tai_list->tai[j].plmn.mcc_digit3 = plmn.mcc_digit3;
      tai_list->tai[j].plmn.mnc_digit1 = plmn.mnc_digit1;
      tai_list->tai[j].plmn.mnc_digit2 = plmn.mnc_digit2;
      tai_list->tai[j].plmn.mnc_digit3 = plmn.mnc_digit3;
      tai_list->tai[j].tac            = _emm_data.conf.tai_list.tai[i].tac;
      j += 1;
      if (15 == plmn.mnc_digit3) { // mnc length 2
        OAILOG_INFO (LOG_NAS, "mme_api_new_guti - UE registered to TAC %d%d%d.%d%d:%d\n",
            plmn.mcc_digit1, plmn.mcc_digit2, plmn.mcc_digit3, plmn.mnc_digit1, plmn.mnc_digit2, _emm_data.conf.tai_list.tai[i].tac);
      } else {
        OAILOG_INFO (LOG_NAS, "mme_api_new_guti - UE registered to TAC %d%d%d.%d%d%d:%d\n",
            plmn.mcc_digit1, plmn.mcc_digit2, plmn.mcc_digit3, plmn.mnc_digit1, plmn.mnc_digit2, plmn.mnc_digit3, _emm_data.conf.tai_list.tai[i].tac);
      }
    }
    tai_list->n_tais = j;

    OAILOG_INFO (LOG_NAS, "mme_api_new_guti - Got GUTI %s\n", guti_str);
    OAILOG_FUNC_RETURN (LOG_NAS, RETURNok);
  }

  OAILOG_FUNC_RETURN (LOG_NAS, RETURNerror);
}

/****************************************************************************
 **                                                                        **
 ** Name:        mme_api_subscribe()                                       **
 **                                                                        **
 ** Description: Requests the MME to check whether connectivity with the   **
 **              requested PDN can be established using the specified APN. **
 **              If accepted the MME returns PDN subscription context con- **
 **              taining EPS subscribed QoS profile, the default APN if    **
 **              required and UE's IPv4 address and/or the IPv6 prefix.    **
 **                                                                        **
 ** Inputs:  apn:               If not NULL, Access Point Name of the PDN  **
 **                             to connect to                              **
 **              is_emergency:  true if the PDN connectivity is requested  **
 **                             for emergency bearer services              **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:         apn:       If NULL, default APN or APN configured for **
 **                             emergency bearer services                  **
 **                  pdn_addr:  PDN connection IPv4 address or IPv6 inter- **
 **                             face identifier to be used to build the    **
 **                             IPv6 link local address                    **
 **                  qos:       EPS subscribed QoS profile                 **
 **                  Return:    RETURNok, RETURNerror                      **
 **                  Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
mme_api_subscribe (
  OctetString * apn,
  mme_api_ip_version_t mme_pdn_index,
  OctetString * pdn_addr,
  int is_emergency,
  mme_api_qos_t * qos)
{
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_NAS);

  if (apn && (apn->length == 0)) {
    /*
     * PDN connectivity to default APN
     */
    if (is_emergency) {
      apn->length = mme_api_emergency_apn.length;
      apn->value = mme_api_emergency_apn.value;
    } else {
      apn->length = mme_api_default_apn.length;
      apn->value = mme_api_default_apn.value;
    }
  }

  /*
   * Assign PDN address
   */
  if (pdn_addr && (_mme_api_pdn_id < MME_API_PDN_MAX)) {
    pdn_addr->length = _mme_api_pdn_addr[mme_pdn_index][_mme_api_pdn_id].length;
    pdn_addr->value = _mme_api_pdn_addr[mme_pdn_index][_mme_api_pdn_id].value;
    /*
     * Increment the total number of PDN connections
     */
    _mme_api_pdn_id += 1;
  } else {
    /*
     * Maximum number of PDN connections exceeded
     */
    rc = RETURNerror;
  }

  /*
   * Setup EPS subscribed QoS profile
   */
  if (qos) {
    qos->qci = MME_API_QCI;
    /*
     * Uplink bit rate
     */
    qos->gbr[MME_API_UPLINK] = MME_API_BIT_RATE_64K;
    qos->mbr[MME_API_UPLINK] = MME_API_BIT_RATE_128K;
    /*
     * Downlink bit rate
     */
    qos->gbr[MME_API_DOWNLINK] = MME_API_BIT_RATE_512K;
    qos->mbr[MME_API_DOWNLINK] = MME_API_BIT_RATE_1024K;
  } else {
    rc = RETURNerror;
  }

  OAILOG_FUNC_RETURN (LOG_NAS, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:        mme_api_unsubscribe()                                     **
 **                                                                        **
 ** Description: Requests the MME to release connectivity with the reques- **
 **              ted PDN using the specified APN.                          **
 **                                                                        **
 ** Inputs:  apn:               Access Point Name of the PDN to disconnect **
 **                             from                                       **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    RETURNok, RETURNerror                      **
 **                  Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
mme_api_unsubscribe (
  OctetString * apn)
{
  OAILOG_FUNC_IN (LOG_NAS);
  int                                     rc = RETURNok;

  /*
   * Decrement the total number of PDN connections
   */
  _mme_api_pdn_id -= 1;
  OAILOG_FUNC_RETURN (LOG_NAS, rc);
}

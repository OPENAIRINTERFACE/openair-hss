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

/*! \file mme_app_ue_context.c
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <pthread.h>

#include "bstrlib.h"
#include "assertions.h"

#include "log.h"
#include "conversions.h"
#include "common_types.h"
#include "common_defs.h"
#include "common_types.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "3gpp_24.301.h"
#include "mme_app_ue_context.h"
#include "mme_app_bearer_context.h"

static mme_ue_s1ap_id_t mme_app_ue_s1ap_id_generator = 1;

/*---------------------------------------------------------------------------
   Bearer Context RBTree Search Data Structure
  --------------------------------------------------------------------------*/

/**
  Comparator funtion for comparing two ebis.

  @param[in] a: Pointer to bearer context a.
  @param[in] b: Pointer to bearer context b.
  @return  An integer greater than, equal to or less than zero according to whether the
  object pointed to by a is greater than, equal to or less than the object pointed to by b.
*/

static
inline int32_t                    mme_app_compare_bearer_context(
    struct bearer_context_s *a,
    struct bearer_context_s *b) {
    if (a->ebi > b->ebi )
      return 1;

    if (a->ebi < b->ebi)
      return -1;

    /* Not more field to compare. */
    return 0;
}

//RB_GENERATE (BearerPool, bearer_context_s, bearer_ctx_rbt_Node, mme_app_compare_bearer_context)


/*---------------------------------------------------------------------------
   PDN Context RBTree Search Data Structure
  --------------------------------------------------------------------------*/

/**
  Comparator funtion for comparing two apns.

  @param[in] a: Pointer to pdn context a.
  @param[in] b: Pointer to pdn context b (received .
  @return  An integer greater than, equal to or less than zero according to whether the
  object pointed to by a is greater than, equal to or less than the object pointed to by b.
*/

static
inline int32_t                    mme_app_compare_pdn_context(
    struct pdn_context_s *a,
    struct pdn_context_s *b) {


  /** Check that default ebis not 0. */
  if(a->default_ebi && b->default_ebi){
    /** Compare Default EBI. */
    if (a->default_ebi > b->default_ebi)
      return 1;

    if (a->default_ebi < b->default_ebi)
      return -1;

    return 0; /**< We found it. */
  }

  /** The context identifier may or may not be set. It may also be null. So check it latest. */
  if(a->context_identifier < PDN_CONTEXT_IDENTIFIER_UNASSIGNED && b->context_identifier < PDN_CONTEXT_IDENTIFIER_UNASSIGNED){
    if (a->context_identifier > b->context_identifier)
      return 1;

    if (a->context_identifier < b->context_identifier)
      return -1;
  }

  /** Compare APN selection. */
  if(a->apn_subscribed && b->apn_subscribed){
    int res = bstricmp (a->apn_subscribed, b->apn_subscribed);
//    if(res != 0){
      return res; /**< If they match, no need to check further. */
//    }
  }

  /* Compare the APN names. */
//  return bstricmp (a->apn_in_use, b->apn_in_use);
//    OAILOG_DEBUG (LOG_MME_APP, "Selected APN %s for UE " IMSI_64_FMT "\n",
//        ue_context->apn_config_profile.apn_configuration[index].service_selection,
//        ue_context->imsi);
//         return &ue_context->apn_config_profile.apn_configuration[index];
//       }
//  return 1; // todo bstrcmp(apn_network_identifier(a->apn_in_use),
      //apn_network_identifier(b->apn_in_use));
  return 0;
}

RB_GENERATE (PdnContexts, pdn_context_s, pdnCtxRbtNode, mme_app_compare_pdn_context)

/**
 * @brief mme_app_convert_imsi_to_imsi_mme: converts the imsi_t struct to the imsi mme struct
 * @param imsi_dst
 * @param imsi_src
 */
// TODO: (amar) This and below functions are only used in testing possibly move
// these to the testing module
void
mme_app_convert_imsi_to_imsi_mme (mme_app_imsi_t * imsi_dst,
                                  const imsi_t *imsi_src)
{
  memset(imsi_dst->data,  (uint8_t) '\0', sizeof(imsi_dst->data));
  IMSI_TO_STRING(imsi_src, imsi_dst->data, IMSI_BCD_DIGITS_MAX + 1);
  imsi_dst->length = strlen(imsi_dst->data);
}

/**
 * @brief mme_app_copy_imsi: copies an mme imsi to another mme imsi
 * @param imsi_dst
 * @param imsi_src
 */

void
mme_app_copy_imsi (mme_app_imsi_t * imsi_dst,
                   const mme_app_imsi_t *imsi_src)
{
  strncpy(imsi_dst->data, imsi_src->data, IMSI_BCD_DIGITS_MAX + 1);
  imsi_dst->length = imsi_src->length;
}

/**
 * @brief mme_app_imsi_compare: compares to imsis returns true if the same else false
 * @param imsi_a
 * @param imsi_b
 * @return
 */

bool
mme_app_imsi_compare (mme_app_imsi_t const * imsi_a,
                      mme_app_imsi_t const * imsi_b)
{
  if((strncmp(imsi_a->data, imsi_b->data, IMSI_BCD_DIGITS_MAX) == 0)
          && imsi_a->length == imsi_b->length) {
    return true;
  }else
    return false;
}

/**
 * @brief mme_app_string_to_imsi converst the a string to the imsi mme structure
 * @param imsi_dst
 * @param imsi_string_src
 */

void
mme_app_string_to_imsi (mme_app_imsi_t * const imsi_dst,
                       char const * const imsi_string_src)
{
    strncpy(imsi_dst->data, imsi_string_src, IMSI_BCD_DIGITS_MAX + 1);
    imsi_dst->length = strlen(imsi_dst->data);
    return;
}

/**
 * @brief mme_app_imsi_to_string converts imsi structure to a string
 * @param imsi_dst
 * @param imsi_src
 */

void
mme_app_imsi_to_string (char * const imsi_dst,
                       mme_app_imsi_t const * const imsi_src)
{
    strncpy(imsi_dst, imsi_src->data, IMSI_BCD_DIGITS_MAX + 1);
    return;
}

/**
 * @brief mme_app_is_imsi_empty: checks if an imsi struct is empty returns true if it is empty
 * @param imsi
 * @return
 */
bool
mme_app_is_imsi_empty (mme_app_imsi_t const * imsi)
{
  return (imsi->length == 0) ? true : false;
}

/**
 * @brief mme_app_imsi_to_u64: converts imsi to uint64 (be careful leading 00 will be cut off)
 * @param imsi_src
 * @return
 */

uint64_t
mme_app_imsi_to_u64 (mme_app_imsi_t imsi_src)
{
  uint64_t uint_imsi;
  sscanf(imsi_src.data, "%" SCNu64, &uint_imsi);
  return uint_imsi;
}

//------------------------------------------------------------------------------
void mme_app_ue_context_s1_release_enb_informations(ue_context_t *ue_context)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  pdn_context_t * registered_pdn_ctx = NULL;
  /** Update all bearers and get the pdn context id. */
  RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_context->pdn_contexts) {
    DevAssert(registered_pdn_ctx);

    /*
     * Get the first PDN whose bearers are not established yet.
     * Do the MBR just one PDN at a time.
     */
    bearer_context_t * bearer_context_to_set_idle = NULL;
    RB_FOREACH (bearer_context_to_set_idle, SessionBearers, &registered_pdn_ctx->session_bearers) {
      DevAssert(bearer_context_to_set_idle);
      /** Add them to the bearears list of the MBR. */
      mme_app_bearer_context_s1_release_enb_informations(bearer_context_to_set_idle);
    }
  }
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
ambr_t mme_app_total_p_gw_apn_ambr(ue_context_t *ue_context){
  pdn_context_t * registered_pdn_ctx = NULL;
  ambr_t apn_ambr_sum= {0, 0};
  RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_context->pdn_contexts) {
    DevAssert(registered_pdn_ctx);
    apn_ambr_sum.br_dl += registered_pdn_ctx->subscribed_apn_ambr.br_dl;
    apn_ambr_sum.br_ul += registered_pdn_ctx->subscribed_apn_ambr.br_ul;
  }
  return apn_ambr_sum;
}

//------------------------------------------------------------------------------
ambr_t mme_app_total_p_gw_apn_ambr_rest(ue_context_t *ue_context, pdn_cid_t pci){
  /** Get the total APN AMBR excluding the given PCI. */
  pdn_context_t * registered_pdn_ctx = NULL;
  ambr_t apn_ambr_sum= {0, 0};
  RB_FOREACH (registered_pdn_ctx, PdnContexts, &ue_context->pdn_contexts) {
    DevAssert(registered_pdn_ctx);
    if(registered_pdn_ctx->context_identifier == pci)
      continue;
    apn_ambr_sum.br_dl += registered_pdn_ctx->subscribed_apn_ambr.br_dl;
    apn_ambr_sum.br_ul += registered_pdn_ctx->subscribed_apn_ambr.br_ul;
  }
  return apn_ambr_sum;
}

//------------------------------------------------------------------------------
mme_ue_s1ap_id_t mme_app_ctx_get_new_ue_id(void)
{
  mme_ue_s1ap_id_t tmp = 0;
  tmp = __sync_fetch_and_add (&mme_app_ue_s1ap_id_generator, 1);
  return tmp;
}

/*
 * Generate the functions to operate inside the bearer pool.
 */
RB_GENERATE (SessionBearers, bearer_context_s, bearerContextRbtNode, mme_app_compare_bearer_context)

RB_GENERATE (BearerPool, bearer_context_s, bearerContextRbtNode, mme_app_compare_bearer_context)

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
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "log.h"
#include "common_defs.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "xml_load.h"
#include "3gpp_24.008_xml.h"
#include "xml2_wrapper.h"

//******************************************************************************
// 10.5.5 GPRS mobility management information elements
//******************************************************************************

//------------------------------------------------------------------------------
// 10.5.5.4 TMSI status
//------------------------------------------------------------------------------
bool tmsi_status_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, tmsi_status_t * const tmsistatus)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bstring xpath_expr = bformat("./%s",TMSI_STATUS_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)tmsistatus, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}
//------------------------------------------------------------------------------
void tmsi_status_to_xml (const tmsi_status_t * const tmsistatus, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, TMSI_STATUS_IE_XML_STR, "%"PRIx8, *tmsistatus);
}

//------------------------------------------------------------------------------
// 10.5.5.6 DRX parameter
//------------------------------------------------------------------------------
bool drx_parameter_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, drx_parameter_t * const drxparameter)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bool res = false;
  bstring xpath_expr_drx = bformat("./%s",DRX_PARAMETER_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_drx);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));
      if (res) {
        uint8_t  splitpgcyclecode = 0;
        bstring xpath_expr = bformat("./%s",SPLIT_PG_CYCLE_CODE_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&splitpgcyclecode, NULL);
        bdestroy_wrapper (&xpath_expr);
        drxparameter->splitpgcyclecode = splitpgcyclecode;
      }
      if (res) {
        uint8_t  cnspecificdrxcyclelengthcoefficientanddrxvaluefors1mode = 0;
        bstring xpath_expr = bformat("./%s",CN_SPECIFIC_DRX_CYCLE_LENGTH_COEFFICIENT_AND_DRX_VALUE_FOR_S1_MODE_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&cnspecificdrxcyclelengthcoefficientanddrxvaluefors1mode, NULL);
        bdestroy_wrapper (&xpath_expr);
        drxparameter->cnspecificdrxcyclelengthcoefficientanddrxvaluefors1mode = cnspecificdrxcyclelengthcoefficientanddrxvaluefors1mode;
      }
      if (res) {
        uint8_t  splitonccch = 0;
        bstring xpath_expr = bformat("./%s",SPLIT_ON_CCCH_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&splitonccch, NULL);
        bdestroy_wrapper (&xpath_expr);
        drxparameter->splitonccch = splitonccch;
      }
      if (res) {
        uint8_t  nondrxtimer = 0;
        bstring xpath_expr = bformat("./%s",NON_DRX_TIMER_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&nondrxtimer, NULL);
        bdestroy_wrapper (&xpath_expr);
        drxparameter->nondrxtimer = nondrxtimer;
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
  }
  bdestroy_wrapper (&xpath_expr_drx);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------
void drx_parameter_to_xml (const drx_parameter_t * const drxparameter, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, DRX_PARAMETER_IE_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, SPLIT_PG_CYCLE_CODE_ATTR_XML_STR, "%"PRIu8, drxparameter->splitpgcyclecode);
  XML_WRITE_FORMAT_ELEMENT(writer, CN_SPECIFIC_DRX_CYCLE_LENGTH_COEFFICIENT_AND_DRX_VALUE_FOR_S1_MODE_ATTR_XML_STR, "%"PRIu8, drxparameter->cnspecificdrxcyclelengthcoefficientanddrxvaluefors1mode);
  XML_WRITE_FORMAT_ELEMENT(writer, SPLIT_ON_CCCH_ATTR_XML_STR, "%"PRIu8, drxparameter->splitonccch);
  XML_WRITE_FORMAT_ELEMENT(writer, NON_DRX_TIMER_ATTR_XML_STR, "%"PRIu8, drxparameter->nondrxtimer);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
// 10.5.5.8 P-TMSI signature
//------------------------------------------------------------------------------
bool p_tmsi_signature_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, p_tmsi_signature_t * ptmsisignature)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bstring xpath_expr = bformat("./%s",P_TMSI_SIGNATURE_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx32, (void*)ptmsisignature, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------

void p_tmsi_signature_to_xml ( const p_tmsi_signature_t * const ptmsisignature, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, P_TMSI_SIGNATURE_IE_XML_STR, "%"PRIx32, *ptmsisignature);
}


//------------------------------------------------------------------------------
// 10.5.5.9 Identity type 2
//------------------------------------------------------------------------------
bool identity_type_2_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, identity_type2_t * const identitytype2)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bstring xpath_expr = bformat("./%s",IDENTITY_TYPE_2_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)identitytype2, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}
//------------------------------------------------------------------------------
void identity_type_2_to_xml (const identity_type2_t * const identitytype2, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, IDENTITY_TYPE_2_IE_XML_STR, "%"PRIu8, *identitytype2);
}

//------------------------------------------------------------------------------
// 10.5.5.10 IMEISV request
//------------------------------------------------------------------------------
bool imeisv_request_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, imeisv_request_t * const imeisvrequest)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bstring xpath_expr = bformat("./%s",IMEISV_REQUEST_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)imeisvrequest, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}
//------------------------------------------------------------------------------
void imeisv_request_to_xml (const imeisv_request_t * const imeisvrequest, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, IMEISV_REQUEST_IE_XML_STR, "%"PRIu8, *imeisvrequest);
}

//------------------------------------------------------------------------------
// 10.5.5.12 MS network capability
//------------------------------------------------------------------------------
bool ms_network_capability_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, ms_network_capability_t * const msnetworkcapability)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bool res = false;
  bstring xpath_expr = bformat("./%s",MS_NETWORK_CAPABILITY_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));
      if (res) {
        uint8_t  gea1 = 0;
        bstring xpath_expr = bformat("./%s",GEA1_BITS_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&gea1, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->gea1 = gea1;
      }
      if (res) {
        uint8_t  smdc = 0;
        bstring xpath_expr = bformat("./%s",SM_CAPABILITIES_VIA_DEDICATED_CHANNELS_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&smdc, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->smdc = smdc;
      }
      if (res) {
        uint8_t  smgc = 0;
        bstring xpath_expr = bformat("./%s",SM_CAPABILITIES_VIA_GPRS_CHANNELS_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&smgc, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->smgc = smgc;
      }
      if (res) {
        uint8_t  ucs2 = 0;
        bstring xpath_expr = bformat("./%s",UCS2_SUPPORT_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&ucs2, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->ucs2 = ucs2;
      }
      if (res) {
        uint8_t  sssi = 0;
        bstring xpath_expr = bformat("./%s",SS_SCREENING_INDICATOR_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&sssi, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->sssi = sssi;
      }
      if (res) {
        uint8_t  solsa = 0;
        bstring xpath_expr = bformat("./%s",SOLSA_CAPABILITY_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&solsa, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->solsa = solsa;
      }
      if (res) {
        uint8_t  revli = 0;
        bstring xpath_expr = bformat("./%s",REVISION_LEVEL_INDICATOR_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&revli, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->revli = revli;
      }
      if (res) {
        uint8_t  pfc = 0;
        bstring xpath_expr = bformat("./%s",PFC_FEATURE_MODE_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&pfc, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->pfc = pfc;
      }
      if (res) {
        uint8_t  egea = 0;
        bstring xpath_expr = bformat("./%s",EXTENDED_GEA_BITS_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&egea, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->egea = egea;
      }
      if (res) {
        uint8_t  lcs = 0;
        bstring xpath_expr = bformat("./%s",LCS_VA_CAPABILITY_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&lcs, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->lcs = lcs;
      }
      if (res) {
        uint8_t  ps_ho_utran = 0;
        bstring xpath_expr = bformat("./%s",PS_INTER_RAT_HO_FROM_GERAN_TO_UTRAN_IU_MODE_CAPABILITY_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&ps_ho_utran, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->ps_ho_utran = ps_ho_utran;
      }
      if (res) {
        uint8_t  ps_ho_eutran = 0;
        bstring xpath_expr = bformat("./%s",PS_INTER_RAT_HO_FROM_GERAN_TO_E_UTRAN_S1_MODE_CAPABILITY_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&ps_ho_eutran, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->ps_ho_eutran = ps_ho_eutran;
      }
      if (res) {
        uint8_t  emm_cpc = 0;
        bstring xpath_expr = bformat("./%s",EMM_COMBINED_PROCEDURES_CAPABILITY_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&emm_cpc, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->emm_cpc = emm_cpc;
      }
      if (res) {
        uint8_t  isr = 0;
        bstring xpath_expr = bformat("./%s",ISR_SUPPORT_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&isr, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->isr = isr;
      }
      if (res) {
        uint8_t  srvcc = 0;
        bstring xpath_expr = bformat("./%s",SRVCC_TO_GERAN_UTRAN_CAPABILITY_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&srvcc, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->srvcc = srvcc;
      }
      if (res) {
        uint8_t  epc_cap = 0;
        bstring xpath_expr = bformat("./%s",EPC_CAPABILITY_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&epc_cap, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->epc_cap = epc_cap;
      }
      if (res) {
        uint8_t  nf_cap = 0;
        bstring xpath_expr = bformat("./%s",NF_CAPABILITY_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&nf_cap, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->nf_cap = nf_cap;
      }
      if (res) {
        uint8_t  geran_ns = 0;
        bstring xpath_expr = bformat("./%s",SPARE_BITS_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&geran_ns, NULL);
        bdestroy_wrapper (&xpath_expr);
        msnetworkcapability->geran_ns = geran_ns;
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}
//------------------------------------------------------------------------------
void ms_network_capability_to_xml (const ms_network_capability_t  * const msnetworkcapability, xmlTextWriterPtr writer)
{

  XML_WRITE_START_ELEMENT(writer, MS_NETWORK_CAPABILITY_IE_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, GEA1_BITS_ATTR_XML_STR, "%x", msnetworkcapability->gea1);
  XML_WRITE_FORMAT_ELEMENT(writer, SM_CAPABILITIES_VIA_DEDICATED_CHANNELS_ATTR_XML_STR, "%x", msnetworkcapability->smdc);
  XML_WRITE_FORMAT_ELEMENT(writer, SM_CAPABILITIES_VIA_GPRS_CHANNELS_ATTR_XML_STR, "%x", msnetworkcapability->smgc);
  XML_WRITE_FORMAT_ELEMENT(writer, UCS2_SUPPORT_ATTR_XML_STR, "%x", msnetworkcapability->ucs2);
  XML_WRITE_FORMAT_ELEMENT(writer, SS_SCREENING_INDICATOR_ATTR_XML_STR, "%x", msnetworkcapability->sssi);
  XML_WRITE_FORMAT_ELEMENT(writer, SOLSA_CAPABILITY_ATTR_XML_STR, "%x", msnetworkcapability->solsa);
  XML_WRITE_FORMAT_ELEMENT(writer, REVISION_LEVEL_INDICATOR_ATTR_XML_STR, "%x", msnetworkcapability->revli);
  XML_WRITE_FORMAT_ELEMENT(writer, PFC_FEATURE_MODE_ATTR_XML_STR, "%x", msnetworkcapability->pfc);
  XML_WRITE_FORMAT_ELEMENT(writer, EXTENDED_GEA_BITS_ATTR_XML_STR, "%x", msnetworkcapability->egea);
  XML_WRITE_FORMAT_ELEMENT(writer, LCS_VA_CAPABILITY_ATTR_XML_STR, "%x", msnetworkcapability->lcs);
  XML_WRITE_FORMAT_ELEMENT(writer, PS_INTER_RAT_HO_FROM_GERAN_TO_UTRAN_IU_MODE_CAPABILITY_ATTR_XML_STR, "%x", msnetworkcapability->ps_ho_utran);
  XML_WRITE_FORMAT_ELEMENT(writer, PS_INTER_RAT_HO_FROM_GERAN_TO_E_UTRAN_S1_MODE_CAPABILITY_ATTR_XML_STR, "%x", msnetworkcapability->ps_ho_eutran);
  XML_WRITE_FORMAT_ELEMENT(writer, EMM_COMBINED_PROCEDURES_CAPABILITY_ATTR_XML_STR, "%x", msnetworkcapability->emm_cpc);
  XML_WRITE_FORMAT_ELEMENT(writer, ISR_SUPPORT_ATTR_XML_STR, "%x", msnetworkcapability->isr);
  XML_WRITE_FORMAT_ELEMENT(writer, SRVCC_TO_GERAN_UTRAN_CAPABILITY_ATTR_XML_STR, "%x", msnetworkcapability->srvcc);
  XML_WRITE_FORMAT_ELEMENT(writer, EPC_CAPABILITY_ATTR_XML_STR, "%x", msnetworkcapability->epc_cap);
  XML_WRITE_FORMAT_ELEMENT(writer, NF_CAPABILITY_ATTR_XML_STR, "%x", msnetworkcapability->nf_cap);
  XML_WRITE_FORMAT_ELEMENT(writer, SPARE_BITS_ATTR_XML_STR, "%x", msnetworkcapability->geran_ns);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
// 10.5.5.15 Routing area identification
//------------------------------------------------------------------------------
bool routing_area_code_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, routing_area_code_t * const rac)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bstring xpath_expr = bformat("./%s",ROUTING_AREA_CODE_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)rac, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}
//------------------------------------------------------------------------------
void routing_area_code_to_xml (const routing_area_code_t * const rac, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, ROUTING_AREA_CODE_IE_XML_STR, "%"PRIx8, *rac);
}

//------------------------------------------------------------------------------
bool routing_area_identification_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, routing_area_identification_t * const rai)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bool res = false;
  bstring xpath_expr = bformat("./%s",ROUTING_AREA_IDENTIFICATION_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));
      if (res) {res= location_area_identification_from_xml(xml_doc, xpath_ctx, &rai->lai);}
      if (res) {res= routing_area_code_from_xml(xml_doc, xpath_ctx, &rai->rac);}
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
  }
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}
//------------------------------------------------------------------------------
void routing_area_identification_to_xml (const routing_area_identification_t * const rai, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, ROUTING_AREA_IDENTIFICATION_IE_XML_STR);
  location_area_identification_to_xml (&rai->lai, writer);
  routing_area_code_to_xml (&rai->rac, writer);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
// 10.5.5.28 Voice domain preference and UE's usage setting
//------------------------------------------------------------------------------
bool voice_domain_preference_and_ue_usage_setting_from_xml (
    xmlDocPtr xml_doc,
    xmlXPathContextPtr xpath_ctx,
    voice_domain_preference_and_ue_usage_setting_t * const voicedomainpreferenceandueusagesetting)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bool res = false;
  bstring xpath_expr_vd = bformat("./%s",VOICE_DOMAIN_PREFERENCE_AND_UE_USAGE_SETTING_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_vd);
  if (xpath_obj) {
    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));
      if (res) {
        uint8_t  ue_usage_setting = 0;
        bstring xpath_expr = bformat("./%s",UE_USAGE_SETTING_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&ue_usage_setting, NULL);
        bdestroy_wrapper (&xpath_expr);
        voicedomainpreferenceandueusagesetting->ue_usage_setting = ue_usage_setting;
      }
      if (res) {
        uint8_t  voice_domain_for_eutran = 0;
        bstring xpath_expr = bformat("./%s",VOICE_DOMAIN_PREFERENCE_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&voice_domain_for_eutran, NULL);
        bdestroy_wrapper (&xpath_expr);
        voicedomainpreferenceandueusagesetting->voice_domain_for_eutran = voice_domain_for_eutran;
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
  }
  bdestroy_wrapper (&xpath_expr_vd);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}
//------------------------------------------------------------------------------
void voice_domain_preference_and_ue_usage_setting_to_xml(
    const voice_domain_preference_and_ue_usage_setting_t * const voicedomainpreferenceandueusagesetting,
    xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, VOICE_DOMAIN_PREFERENCE_AND_UE_USAGE_SETTING_IE_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, UE_USAGE_SETTING_ATTR_XML_STR, "%"PRIx8, voicedomainpreferenceandueusagesetting->ue_usage_setting);
  XML_WRITE_FORMAT_ELEMENT(writer, VOICE_DOMAIN_PREFERENCE_ATTR_XML_STR, "%"PRIx8, voicedomainpreferenceandueusagesetting->voice_domain_for_eutran);
  XML_WRITE_END_ELEMENT(writer);
}


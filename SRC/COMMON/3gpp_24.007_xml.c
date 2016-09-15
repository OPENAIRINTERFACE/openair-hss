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
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "3gpp_24.007_xml.h"

#include "xml_load.h"
#include "xml2_wrapper.h"
#include "log.h"

//------------------------------------------------------------------------------
bool protocol_discriminator_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    eps_protocol_discriminator_t * const pd)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bool res = false;
  bstring xpath_expr = bformat("./%s",PROTOCOL_DISCRIMINATOR_IE_XML_STR);
  char val[128] = {0};
  res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", val, NULL);
  if (!strcasecmp(val, GROUP_CALL_CONTROL_VAL_XML_STR)) {
    *pd = GROUP_CALL_CONTROL;
    res = true;
  } else if (!strcasecmp(val, BROADCAST_CALL_CONTROL_VAL_XML_STR)) {
    *pd = BROADCAST_CALL_CONTROL;
    res = true;
  } else  if (!strcasecmp(val, EPS_SESSION_MANAGEMENT_MESSAGE_VAL_XML_STR)) {
    *pd = EPS_SESSION_MANAGEMENT_MESSAGE;
    res = true;
  } else  if (!strcasecmp(val, CALL_CONTROL_CC_RELATED_SS_MESSAGE_VAL_XML_STR)) {
    *pd = CALL_CONTROL_CC_RELATED_SS_MESSAGE;
    res = true;
  } else  if (!strcasecmp(val, GPRS_TRANSPARENT_TRANSPORT_PROTOCOL_VAL_XML_STR)) {
    *pd = GPRS_TRANSPARENT_TRANSPORT_PROTOCOL;
    res = true;
  } else  if (!strcasecmp(val, MOBILITY_MANAGEMENT_MESSAGE_VAL_XML_STR)) {
    *pd = MOBILITY_MANAGEMENT_MESSAGE;
    res = true;
  } else  if (!strcasecmp(val, RADIO_RESOURCES_MANAGEMENT_MESSAGE_VAL_XML_STR)) {
    *pd = RADIO_RESOURCES_MANAGEMENT_MESSAGE;
    res = true;
  } else  if (!strcasecmp(val, EPS_MOBILITY_MANAGEMENT_MESSAGE_VAL_XML_STR)) {
    *pd = EPS_MOBILITY_MANAGEMENT_MESSAGE;
    res = true;
  } else  if (!strcasecmp(val, GPRS_MOBILITY_MANAGEMENT_MESSAGE_VAL_XML_STR)) {
    *pd = GPRS_MOBILITY_MANAGEMENT_MESSAGE;
    res = true;
  } else  if (!strcasecmp(val, SMS_MESSAGE_VAL_XML_STR)) {
    *pd = SMS_MESSAGE;
    res = true;
  } else  if (!strcasecmp(val, GPRS_SESSION_MANAGEMENT_MESSAGE_VAL_XML_STR)) {
    *pd = GPRS_SESSION_MANAGEMENT_MESSAGE;
    res = true;
  } else  if (!strcasecmp(val, NON_CALL_RELATED_SS_MESSAGE_VAL_XML_STR)) {
    *pd = NON_CALL_RELATED_SS_MESSAGE;
    res = true;
  } else
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------
void protocol_discriminator_to_xml(const eps_protocol_discriminator_t * const pd, xmlTextWriterPtr writer)
{
  switch (*pd) {
  case GROUP_CALL_CONTROL:
    XML_WRITE_FORMAT_ELEMENT(writer, PROTOCOL_DISCRIMINATOR_IE_XML_STR, "%s", GROUP_CALL_CONTROL_VAL_XML_STR);
    break;
  case BROADCAST_CALL_CONTROL:
    XML_WRITE_FORMAT_ELEMENT(writer, PROTOCOL_DISCRIMINATOR_IE_XML_STR, "%s", BROADCAST_CALL_CONTROL_VAL_XML_STR);
    break;
  case EPS_SESSION_MANAGEMENT_MESSAGE:
    XML_WRITE_FORMAT_ELEMENT(writer, PROTOCOL_DISCRIMINATOR_IE_XML_STR, "%s", EPS_SESSION_MANAGEMENT_MESSAGE_VAL_XML_STR);
    break;
  case CALL_CONTROL_CC_RELATED_SS_MESSAGE:
    XML_WRITE_FORMAT_ELEMENT(writer, PROTOCOL_DISCRIMINATOR_IE_XML_STR, "%s", CALL_CONTROL_CC_RELATED_SS_MESSAGE_VAL_XML_STR);
    break;
  case GPRS_TRANSPARENT_TRANSPORT_PROTOCOL:
    XML_WRITE_FORMAT_ELEMENT(writer, PROTOCOL_DISCRIMINATOR_IE_XML_STR, "%s", GPRS_TRANSPARENT_TRANSPORT_PROTOCOL_VAL_XML_STR);
    break;
  case MOBILITY_MANAGEMENT_MESSAGE:
    XML_WRITE_FORMAT_ELEMENT(writer, PROTOCOL_DISCRIMINATOR_IE_XML_STR, "%s", MOBILITY_MANAGEMENT_MESSAGE_VAL_XML_STR);
    break;
  case RADIO_RESOURCES_MANAGEMENT_MESSAGE:
    XML_WRITE_FORMAT_ELEMENT(writer, PROTOCOL_DISCRIMINATOR_IE_XML_STR, "%s", RADIO_RESOURCES_MANAGEMENT_MESSAGE_VAL_XML_STR);
    break;
  case EPS_MOBILITY_MANAGEMENT_MESSAGE:
    XML_WRITE_FORMAT_ELEMENT(writer, PROTOCOL_DISCRIMINATOR_IE_XML_STR, "%s", EPS_MOBILITY_MANAGEMENT_MESSAGE_VAL_XML_STR);
    break;
  case GPRS_MOBILITY_MANAGEMENT_MESSAGE:
    XML_WRITE_FORMAT_ELEMENT(writer, PROTOCOL_DISCRIMINATOR_IE_XML_STR, "%s", GPRS_MOBILITY_MANAGEMENT_MESSAGE_VAL_XML_STR);
    break;
  case SMS_MESSAGE:
    XML_WRITE_FORMAT_ELEMENT(writer, PROTOCOL_DISCRIMINATOR_IE_XML_STR, "%s", SMS_MESSAGE_VAL_XML_STR);
    break;
  case GPRS_SESSION_MANAGEMENT_MESSAGE:
    XML_WRITE_FORMAT_ELEMENT(writer, PROTOCOL_DISCRIMINATOR_IE_XML_STR, "%s", GPRS_SESSION_MANAGEMENT_MESSAGE_VAL_XML_STR);
    break;
  case NON_CALL_RELATED_SS_MESSAGE:
    XML_WRITE_FORMAT_ELEMENT(writer, PROTOCOL_DISCRIMINATOR_IE_XML_STR, "%s", NON_CALL_RELATED_SS_MESSAGE_VAL_XML_STR);
    break;
  default:
    XML_WRITE_FORMAT_ELEMENT(writer, PROTOCOL_DISCRIMINATOR_IE_XML_STR, "%s", "UNKNOWN");
  }
}

//------------------------------------------------------------------------------
bool sequence_number_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    uint8_t                   * const sn)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bstring xpath_expr = bformat("./%s",SEQUENCE_NUMBER_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)sn, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------
void sequence_number_to_xml(const uint8_t * const sn, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, SEQUENCE_NUMBER_IE_XML_STR, "%"PRIu8, *sn);
}

//------------------------------------------------------------------------------
bool eps_bearer_identity_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    ebi_t                     * const ebi)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bstring xpath_expr = bformat("./%s",EBI_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)ebi, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------
void eps_bearer_identity_to_xml(const ebi_t * const ebi, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, EBI_IE_XML_STR, "%"PRIu8, *ebi);
}

//------------------------------------------------------------------------------
bool procedure_transaction_identity_from_xml (
    xmlDocPtr                         xml_doc,
    xmlXPathContextPtr                xpath_ctx,
    pti_t                     * const pti)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bstring xpath_expr = bformat("./%s",PTI_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)pti, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------
void procedure_transaction_identity_to_xml(const pti_t * const pti, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, PTI_IE_XML_STR, "%"PRIx8, *pti);
}


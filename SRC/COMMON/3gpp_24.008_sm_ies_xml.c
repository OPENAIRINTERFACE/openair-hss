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
#include <pthread.h>

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
#include "3gpp_24.008_xml.h"
#include "xml_load.h"
#include "xml2_wrapper.h"
#include "conversions.h"

//******************************************************************************
// 10.5.6 Session management information elements
//******************************************************************************
//------------------------------------------------------------------------------
// 10.5.6.1 Access Point Name
//------------------------------------------------------------------------------
bool access_point_name_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, access_point_name_t * access_point_name)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  char apn_str[ACCESS_POINT_NAME_MAX_LENGTH+1]  = {0};
  bstring xpath_expr = bformat("./%s",ACCESS_POINT_NAME_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%s", (void*)apn_str, NULL);
  if (res) {
    *access_point_name = blk2bstr((const void *)apn_str, strlen(apn_str));
  }
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}

//------------------------------------------------------------------------------
void access_point_name_to_xml (access_point_name_t access_point_name, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, ACCESS_POINT_NAME_IE_XML_STR, "%s", bdata(access_point_name));
}

//------------------------------------------------------------------------------
// 10.5.6.3 Protocol configuration options
//------------------------------------------------------------------------------
bool protocol_configuration_options_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, protocol_configuration_options_t * const pco, bool ms2network_direction)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  memset(pco, 0, sizeof(*pco));
  bool res = false;
  bstring xpath_expr_pco = bformat("./%s",PROTOCOL_CONFIGURATION_OPTIONS_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj_pco = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_pco);
  if (xpath_obj_pco) {
    xmlNodeSetPtr nodes_pco = xpath_obj_pco->nodesetval;
    int size = (nodes_pco) ? nodes_pco->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {

      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes_pco->nodeTab[0], xpath_ctx));
      if (res) {
        uint8_t  extension = 0;
        bstring xpath_expr = bformat("./%s",EXTENSION_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&extension, NULL);
        bdestroy_wrapper (&xpath_expr);
        pco->ext = extension;
      }
      if (res) {
        uint8_t  configuration_protocol = 0;
        bstring xpath_expr = bformat("./%s",CONFIGURATION_PROTOCOL_ATTR_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&configuration_protocol, NULL);
        bdestroy_wrapper (&xpath_expr);
        pco->configuration_protocol = configuration_protocol;
      }

      if (res) {
        bstring xpath_expr_cid = bformat("./%s",PROTOCOL_ATTR_XML_STR);
        xmlXPathObjectPtr xpath_obj_cid = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_cid);
        if (xpath_obj_cid) {
          xmlNodeSetPtr nodes_cid = xpath_obj_cid->nodesetval;
          int size = (nodes_cid) ? nodes_cid->nodeNr : 0;
          for (int i = 0; i < size; i++) {
            xmlChar *key = xmlNodeListGetString(xml_doc, nodes_cid->nodeTab[i]->xmlChildrenNode, 1);
            OAILOG_TRACE (LOG_UTIL, "Found %s=%s\n", bdata(xpath_expr_cid), key);
            if (!strcasecmp(PROTOCOL_ID_LCP_VAL_XML_STR, (const char *)key)) {
              pco->protocol_or_container_ids[i].id = PCO_PI_LCP;
            } else if (!strcasecmp(PROTOCOL_ID_PAP_VAL_XML_STR, (const char *)key)) {
              pco->protocol_or_container_ids[i].id = PCO_PI_PAP;
            }  else if (!strcasecmp(PROTOCOL_ID_CHAP_VAL_XML_STR, (const char *)key)) {
              pco->protocol_or_container_ids[i].id = PCO_PI_CHAP;
            }  else if (!strcasecmp(PROTOCOL_ID_IPCP_VAL_XML_STR, (const char *)key)) {
              if (res) {
                pco->protocol_or_container_ids[i].id = PCO_PI_IPCP;
                xmlNodePtr saved_node_ptr2 = xpath_ctx->node;
                res = (RETURNok == xmlXPathSetContextNode(nodes_pco->nodeTab[0], xpath_ctx));
                if (res) {
                  bstring xpath_expr_raw = bformat("./%s",RAW_CONTENT_ATTR_XML_STR);
                  res = xml_load_hex_stream_leaf_tag(xml_doc,xpath_ctx, xpath_expr_raw, &pco->protocol_or_container_ids[i].contents);
                  bdestroy_wrapper (&xpath_expr_raw);
                }
                res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr2, xpath_ctx)) & res;
              }
            }  else {
              res = false;
            }
            xmlFree(key);
          }
        }
      }
      if (saved_node_ptr) {
        res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
      }
    }
  }
  bdestroy_wrapper (&xpath_expr_pco);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}
//------------------------------------------------------------------------------
void protocol_configuration_options_to_xml (const protocol_configuration_options_t * const pco, xmlTextWriterPtr writer, bool ms2network_direction)
{
  int                                     i = 0;

  if (pco) {
    XML_WRITE_START_ELEMENT(writer, PROTOCOL_CONFIGURATION_OPTIONS_IE_XML_STR);
    XML_WRITE_FORMAT_ELEMENT(writer, EXTENSION_ATTR_XML_STR, "%"PRIx8, pco->ext);
    XML_WRITE_FORMAT_ELEMENT(writer, CONFIGURATION_PROTOCOL_ATTR_XML_STR, "%"PRIx8, pco->configuration_protocol);
    while (i < pco->num_protocol_or_container_id) {
      switch (pco->protocol_or_container_ids[i].id) {
      case PCO_PI_LCP:
        XML_WRITE_START_ELEMENT(writer, PROTOCOL_ATTR_XML_STR);
        XML_WRITE_ATTRIBUTE(writer, PROTOCOL_ID_ATTR_XML_STR, PROTOCOL_ID_LCP_VAL_XML_STR);
        XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
        XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
        XML_WRITE_END_ELEMENT(writer);
        break;
      case PCO_PI_PAP:
        XML_WRITE_START_ELEMENT(writer, PROTOCOL_ATTR_XML_STR);
        XML_WRITE_ATTRIBUTE(writer, PROTOCOL_ID_ATTR_XML_STR, PROTOCOL_ID_PAP_VAL_XML_STR);
        XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
        XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
        XML_WRITE_END_ELEMENT(writer);
        break;
      case PCO_PI_CHAP:
        XML_WRITE_START_ELEMENT(writer, PROTOCOL_ATTR_XML_STR);
        XML_WRITE_ATTRIBUTE(writer, PROTOCOL_ID_ATTR_XML_STR, PROTOCOL_ID_CHAP_VAL_XML_STR);
        XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
        XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
        XML_WRITE_END_ELEMENT(writer);
        break;
      case PCO_PI_IPCP:
        XML_WRITE_START_ELEMENT(writer, PROTOCOL_ATTR_XML_STR);
        XML_WRITE_ATTRIBUTE(writer, PROTOCOL_ID_ATTR_XML_STR, PROTOCOL_ID_IPCP_VAL_XML_STR);
        XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
        XML_WRITE_HEX_ELEMENT(writer, RAW_CONTENT_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
        XML_WRITE_COMMENT(writer, "TODO IPCP (replace raw content by human readable tags)");
        XML_WRITE_END_ELEMENT(writer);
        break;

      default:
        if (ms2network_direction) {
          switch (pco->protocol_or_container_ids[i].id) {
          case PCO_CI_P_CSCF_IPV6_ADDRESS_REQUEST:
            /* When the container identifier indicates P-CSCF IPv6 Address Request, DNS Server
             * IPv6 Address Request, or MSISDN Request, the container identifier contents field is
             * empty and the length of container identifier contents indicates a length equal to
             * zero. If the container identifier contents field is not empty, it shall be ignored.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, P_CSCF_IPV6_ADDRESS_REQUEST_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_DNS_SERVER_IPV6_ADDRESS_REQUEST:
            /* When the container identifier indicates P-CSCF IPv6 Address Request, DNS Server
             * IPv6 Address Request, or MSISDN Request, the container identifier contents field is
             * empty and the length of container identifier contents indicates a length equal to
             * zero. If the container identifier contents field is not empty, it shall be ignored.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, DNS_SERVER_IPV6_ADDRESS_REQUEST_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_MS_SUPPORT_OF_NETWORK_REQUESTED_BEARER_CONTROL_INDICATOR:
            /* When the container identifier indicates MS Support of Network Requested Bearer
             * Control indicator, the container identifier contents field is empty and the length of
             * container identifier contents indicates a length equal to zero. If the container
             * identifier contents field is not empty, it shall be ignored.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, MS_SUPPORT_OF_NETWORK_REQUESTED_BEARER_CONTROL_INDICATOR_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_DSMIPV6_HOME_AGENT_ADDRESS_REQUEST:
            /* When the container identifier indicates DSMIPv6 Home Agent Address Request, the
             * container identifier contents field is empty and the length of container identifier
             * contents indicates a length equal to zero. If the container identifier contents field is
             * not empty, it shall be ignored.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, DSMIPV6_HOME_AGENT_ADDRESS_REQUEST_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_DSMIPV6_HOME_NETWORK_PREFIX_REQUEST:
            /* When the container identifier indicates DSMIPv6 Home Network Prefix Request, the
             * container identifier contents field is empty and the length of container identifier
             * contents indicates a length equal to zero. If the container identifier contents field is
             * not empty, it shall be ignored.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, DSMIPV6_HOME_NETWORK_PREFIX_REQUEST_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_DSMIPV6_IPV4_HOME_AGENT_ADDRESS_REQUEST:
            /* When the container identifier indicates DSMIPv6 IPv4 Home Agent Address
             * Request, the container identifier contents field is empty and the length of container
             * identifier contents indicates a length equal to zero. If the container identifier contents
             * field is not empty, it shall be ignored.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, DSMIPV6_IPV4_HOME_AGENT_ADDRESS_REQUEST_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_IP_ADDRESS_ALLOCATION_VIA_NAS_SIGNALLING:
            /* When the container identifier indicates IP address allocation via NAS signalling, the
             * container identifier contents field is empty and the length of container identifier
             * contents indicates a length equal to zero. If the container identifier contents field is
             * not empty, it shall be ignored.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, IP_ADDRESS_ALLOCATION_VIA_NAS_SIGNALLING_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_IPV4_ADDRESS_ALLOCATION_VIA_DHCPV4:
            /* When the container identifier indicates IP address allocation via DHCPv4, the
             * container identifier contents field is empty and the length of container identifier
             * contents indicates a length equal to zero. If the container identifier contents field is
             * not empty, it shall be ignored.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, IPV4_ADDRESS_ALLOCATION_VIA_DHCPV4_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_P_CSCF_IPV4_ADDRESS_REQUEST:
            /* When the container identifier indicates P-CSCF IPv4 Address Request, the
             * container identifier contents field is empty and the length of container identifier
             * contents indicates a length equal to zero. If the container identifier contents field is
             * not empty, it shall be ignored.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, P_CSCF_IPV4_ADDRESS_REQUEST_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_DNS_SERVER_IPV4_ADDRESS_REQUEST:
            /* When the container identifier indicates DNS Server IPv4 Address Request, the
             * container identifier contents field is empty and the length of container identifier
             * contents indicates a length equal to zero. If the container identifier contents field is
             * not empty, it shall be ignored.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, DNS_SERVER_IPV4_ADDRESS_REQUEST_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_MSISDN_REQUEST:
            /* When the container identifier indicates P-CSCF IPv6 Address Request, DNS Server
             * IPv6 Address Request, or MSISDN Request, the container identifier contents field is
             * empty and the length of container identifier contents indicates a length equal to
             * zero. If the container identifier contents field is not empty, it shall be ignored.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, MSISDN_REQUEST_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_IFOM_SUPPORT_REQUEST:
            /* When the container identifier indicates IFOM Support Request (see
             * 3GPP TS 24.303 [124] and 3GPP TS 24.327 [125]), the container identifier contents
             * field is empty and the length of container identifier contents indicates a length equal
             * to zero. If the container identifier contents field is not empty, it shall be ignored.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, IFOM_SUPPORT_REQUEST_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_IPV4_LINK_MTU_REQUEST:
            /* When the container identifier indicates IPv4 Link MTU Request, the container
             * identifier contents field is empty and the length of container identifier contents
             * indicates a length equal to zero. If the container identifier contents field is not empty,
             * it shall be ignored.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, IPV4_LINK_MTU_REQUEST_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_IM_CN_SUBSYSTEM_SIGNALING_FLAG:
            /* When the container identifier indicates IM CN Subsystem Signaling Flag (see 3GPP
             * TS 24.229 [95]), the container identifier contents field is empty and the length of
             * container identifier contents indicates a length equal to zero. If the container
             * identifier contents field is not empty, it shall be ignored. In Network to MS direction
             * this information may be used by the MS to indicate to the user whether the
             * requested dedicated signalling PDP context was successfully established.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, IM_CN_SUBSYSTEM_SIGNALING_FLAG_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          default:
            XML_WRITE_FORMAT_COMMENT(writer, "Unhandled container id %u length %d\n", pco->protocol_or_container_ids[i].id, blength(pco->protocol_or_container_ids[i].contents));
          }
        }  else {
          switch (pco->protocol_or_container_ids[i].id) {
          case PCO_CI_P_CSCF_IPV6_ADDRESS:
            /* When the container identifier indicates P-CSCF IPv6 Address, the container
             * identifier contents field contains one IPv6 address corresponding to a P-CSCF
             * address (see 3GPP TS 24.229 [95]). This IPv6 address is encoded as an 128-bit
             * address according to RFC 3513 [99]. When there is need to include more than one
             * P-CSCF address, then more logical units with container identifier indicating P-CSCF
             * Address are used.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, P_CSCF_IPV6_ADDRESS_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_DNS_SERVER_IPV6_ADDRESS:
            /* When the container identifier indicates DNS Server IPv6 Address, the container
             * identifier contents field contains one IPv6 DNS server address (see 3GPP TS
             * 27.060 [36a]). This IPv6 address is encoded as an 128-bit address according to
             * RFC 3513 [99]. When there is need to include more than one DNS server address,
             * then more logical units with container identifier indicating DNS Server Address are
             * used.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, DNS_SERVER_IPV6_ADDRESS_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_POLICY_CONTROL_REJECTION_CODE:
            /* When the container identifier indicates Policy Control rejection code, the container
             * identifier contents field contains a Go interface related cause code from the GGSN
             * to the MS (see 3GPP TS 29.207 [100]). The length of container identifier contents
             * indicates a length equal to one. If the container identifier contents field is empty or
             * its actual length is greater than one octect, then it shall be ignored by the receiver.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, POLICY_CONTROL_REJECTION_CODE_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_SELECTED_BEARER_CONTROL_MODE:
            /* When the container identifier indicates Selected Bearer Control Mode, the container
             * identifier contents field contains the selected bearer control mode, where ‘01H’
             * indicates that ‘MS only’ mode has been selected and ‘02H’ indicates that ‘MS/NW’
             * mode has been selected. The length of container identifier contents indicates a
             * length equal to one. If the container identifier contents field is empty or its actual
             * length is greater than one octect, then it shall be ignored by the receiver.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, SELECTED_BEARER_CONTROL_MODE_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_DSMIPV6_HOME_AGENT_ADDRESS:
            /* When the container identifier indicates DSMIPv6 Home Agent Address, the
             * container identifier contents field contains one IPv6 address corresponding to a
             * DSMIPv6 HA address (see 3GPP TS 24.303 [124] and 3GPP TS 24.327 [125]).
             * This IPv6 address is encoded as an 128-bit address according to
             * IETF RFC 3513 [99].
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, DSMIPV6_HOME_AGENT_ADDRESS_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_DSMIPV6_HOME_NETWORK_PREFIX:
            /* When the container identifier indicates DSMIPv6 Home Network Prefix, the
             * container identifier contents field contains one IPv6 Home Network Prefix (see
             * 3GPP TS 24.303 [124] and 3GPP TS 24.327 [125]). This IPv6 prefix is encoded as
             * an IPv6 address according to RFC 3513 [99] followed by 8 bits which specifies the
             * prefix length.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, DSMIPV6_HOME_NETWORK_PREFIX_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_DSMIPV6_IPV4_HOME_AGENT_ADDRESS:
            /* When the container identifier indicates DSMIPv6 IPv4 Home Agent Address, the
             * container identifier contents field contains one IPv4 address corresponding to a
             * DSMIPv6 IPv4 Home Agent address (see 3GPP TS 24.303 [124] and
             * 3GPP TS 24.327 [125]).
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, DSMIPV6_IPV4_HOME_AGENT_ADDRESS_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_P_CSCF_IPV4_ADDRESS:
            /* When the container identifier indicates P-CSCF IPv4 Address, the container
             * identifier contents field contains one IPv4 address corresponding to the P-CSCF
             * address to be used.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, P_CSCF_IPV4_ADDRESS_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            if (4 == pco->protocol_or_container_ids[i].length) {
              XML_WRITE_FORMAT_COMMENT(writer, "%s=%u.%u.%u.%u\n", P_CSCF_IPV4_ADDRESS_VAL_XML_STR,
                  pco->protocol_or_container_ids[i].contents->data[0], pco->protocol_or_container_ids[i].contents->data[1],
                  pco->protocol_or_container_ids[i].contents->data[2], pco->protocol_or_container_ids[i].contents->data[3]);
            }
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_DNS_SERVER_IPV4_ADDRESS:
            /* When the container identifier indicates DNS Server IPv4 Address, the container
             * identifier contents field contains one IPv4 address corresponding to the DNS server
             * address to be used.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, DNS_SERVER_IPV4_ADDRESS_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            if (4 == pco->protocol_or_container_ids[i].length) {
              XML_WRITE_FORMAT_COMMENT(writer, "%s=%u.%u.%u.%u\n", DNS_SERVER_IPV4_ADDRESS_VAL_XML_STR,
                  pco->protocol_or_container_ids[i].contents->data[0], pco->protocol_or_container_ids[i].contents->data[1],
                  pco->protocol_or_container_ids[i].contents->data[2], pco->protocol_or_container_ids[i].contents->data[3]);
            }
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_MSISDN:
            /* When the container identifier indicates MSISDN, the container identifier contents
             * field contains the MSISDN (see 3GPP TS 23.003 [10]) assigned to the MS. Use of
             * the MSISDN provided is defined in subclause 6.4.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, MSISDN_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_IFOM_SUPPORT:
            /* When the container identifier indicates IFOM Support, the container identifier
             * contents field is empty and the length of container identifier contents indicates a
             * length equal to zero. If the container identifier contents field is not empty, it shall be
             * ignored. This information indicates that the Home Agent supports IFOM.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, IFOM_SUPPORT_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_IPV4_LINK_MTU:
            /* When the container identifier indicates IPv4 Link MTU, the length of container
             * identifier contents indicates a length equal to two. The container identifier contents
             * field contains the binary coded representation of the IPv4 link MTU size in octets. Bit
             * 8 of the first octet of the container identifier contents field contains the most
             * significant bit and bit 1 of the second octet of the container identifier contents field
             * contains the least significant bit. If the length of container identifier contents is
             * different from two octets, then it shall be ignored by the receiver.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, IPV4_LINK_MTU_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            if (2 == pco->protocol_or_container_ids[i].length) {
              XML_WRITE_FORMAT_COMMENT(writer, "%s=%u\n", IPV4_LINK_MTU_VAL_XML_STR,
                  (uint16_t)(pco->protocol_or_container_ids[i].contents->data[0])*256 + (uint16_t)pco->protocol_or_container_ids[i].contents->data[1]);
            }
            XML_WRITE_END_ELEMENT(writer);
            break;
          case PCO_CI_IM_CN_SUBSYSTEM_SIGNALING_FLAG:
            /* When the container identifier indicates IM CN Subsystem Signaling Flag (see 3GPP
             * TS 24.229 [95]), the container identifier contents field is empty and the length of
             * container identifier contents indicates a length equal to zero. If the container
             * identifier contents field is not empty, it shall be ignored. In Network to MS direction
             * this information may be used by the MS to indicate to the user whether the
             * requested dedicated signalling PDP context was successfully established.
             */
            XML_WRITE_START_ELEMENT(writer, CONTAINER_ATTR_XML_STR);
            XML_WRITE_ATTRIBUTE(writer, CONTAINER_ID_ATTR_XML_STR, IM_CN_SUBSYSTEM_SIGNALING_FLAG_VAL_XML_STR);
            XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%d", pco->protocol_or_container_ids[i].length);
            XML_WRITE_HEX_ATTRIBUTE(writer, HEX_DATA_ATTR_XML_STR, bdata(pco->protocol_or_container_ids[i].contents), blength(pco->protocol_or_container_ids[i].contents));
            XML_WRITE_END_ELEMENT(writer);
            break;
          default:
            XML_WRITE_FORMAT_COMMENT(writer, "Unhandled container id %u length %d\n", pco->protocol_or_container_ids[i].id, blength(pco->protocol_or_container_ids[i].contents));
          }
        }
      }
      i++;
    }
    XML_WRITE_END_ELEMENT(writer);
  }
}

//------------------------------------------------------------------------------
// 10.5.6.5 Quality of service
//------------------------------------------------------------------------------
bool quality_of_service_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, quality_of_service_t * const qualityofservice)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bool res = false;
  bstring xpath_expr_qos = bformat("./%s",QUALITY_OF_SERVICE_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj_qos = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_qos);
  if (xpath_obj_qos) {
    xmlNodeSetPtr nodes = xpath_obj_qos->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));

      if (res) {
        uint8_t  delayclass = 0;
        bstring xpath_expr = bformat("./%s",DELAY_CLASS_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&delayclass, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->delayclass = delayclass;
      }
      if (res) {
        uint8_t  reliabilityclass       = 0;
        bstring xpath_expr = bformat("./%s",RELIABILITY_CLASS_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&reliabilityclass, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->reliabilityclass = reliabilityclass;
      }
      if (res) {
        uint8_t  peakthroughput       = 0;
        bstring xpath_expr = bformat("./%s",PEAK_THROUGHPUT_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&peakthroughput, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->peakthroughput = peakthroughput;
      }
      if (res) {
        uint8_t  precedenceclass       = 0;
        bstring xpath_expr = bformat("./%s",PRECEDENCE_CLASS_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&precedenceclass, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->precedenceclass = precedenceclass;
      }
      if (res) {
        uint8_t  meanthroughput       = 0;
        bstring xpath_expr = bformat("./%s",MEAN_THROUGHPUT_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&meanthroughput, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->meanthroughput = meanthroughput;
      }
      if (res) {
        uint8_t  trafficclass       = 0;
        bstring xpath_expr = bformat("./%s",TRAFIC_CLASS_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&trafficclass, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->trafficclass = trafficclass;
      }
      if (res) {
        uint8_t  deliveryorder       = 0;
        bstring xpath_expr = bformat("./%s",DELIVERY_ORDER_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&deliveryorder, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->deliveryorder = deliveryorder;
      }
      if (res) {
        uint8_t  deliveryoferroneoussdu       = 0;
        bstring xpath_expr = bformat("./%s",DELIVERY_OF_ERRONEOUS_SDU_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&deliveryoferroneoussdu, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->deliveryoferroneoussdu = deliveryoferroneoussdu;
      }
      if (res) {
        uint8_t  maximumsdusize       = 0;
        bstring xpath_expr = bformat("./%s",MAXIMUM_SDU_SIZE_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&maximumsdusize, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->maximumsdusize = maximumsdusize;
      }
      if (res) {
        uint8_t  maximumbitrateuplink       = 0;
        bstring xpath_expr = bformat("./%s",MAXIMUM_BIT_RATE_UPLINK_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&maximumbitrateuplink, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->maximumbitrateuplink = maximumbitrateuplink;
      }
      if (res) {
        uint8_t  residualber       = 0;
        bstring xpath_expr = bformat("./%s",RESIDUAL_BER_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&residualber, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->residualber = residualber;
      }
      if (res) {
        uint8_t  sduratioerror       = 0;
        bstring xpath_expr = bformat("./%s",SDU_RATIO_ERROR_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&sduratioerror, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->sduratioerror = sduratioerror;
      }
      if (res) {
        uint8_t  transferdelay       = 0;
        bstring xpath_expr = bformat("./%s",TRANSFER_DELAY_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&transferdelay, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->transferdelay = transferdelay;
      }
      if (res) {
        uint8_t  traffichandlingpriority       = 0;
        bstring xpath_expr = bformat("./%s",TRAFFIC_HANDLING_PRIORITY_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&traffichandlingpriority, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->traffichandlingpriority = traffichandlingpriority;
      }
      if (res) {
        uint8_t  guaranteedbitrateuplink       = 0;
        bstring xpath_expr = bformat("./%s",GUARANTEED_BITRATE_UPLINK_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&guaranteedbitrateuplink, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->guaranteedbitrateuplink = guaranteedbitrateuplink;
      }
      if (res) {
        uint8_t  guaranteedbitratedownlink       = 0;
        bstring xpath_expr = bformat("./%s",GUARANTEED_BITRATE_DOWNLINK_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&guaranteedbitratedownlink, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->guaranteedbitratedownlink = guaranteedbitratedownlink;
      }
      if (res) {
        uint8_t  signalingindication       = 0;
        bstring xpath_expr = bformat("./%s",SIGNALING_INDICATION_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&signalingindication, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->signalingindication = signalingindication;
      }
      if (res) {
        uint8_t  sourcestatisticsdescriptor       = 0;
        bstring xpath_expr = bformat("./%s",SOURCE_STATISTICS_DESCRIPTOR_VAL_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&sourcestatisticsdescriptor, NULL);
        bdestroy_wrapper (&xpath_expr);
        qualityofservice->sourcestatisticsdescriptor = sourcestatisticsdescriptor;
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
  }
  bdestroy_wrapper (&xpath_expr_qos);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}
//------------------------------------------------------------------------------
void quality_of_service_to_xml ( const quality_of_service_t * const qualityofservice, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, QUALITY_OF_SERVICE_IE_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, DELAY_CLASS_VAL_XML_STR, "%"PRIu8, qualityofservice->delayclass);
  XML_WRITE_FORMAT_ELEMENT(writer, RELIABILITY_CLASS_VAL_XML_STR, "%"PRIu8, qualityofservice->reliabilityclass);
  XML_WRITE_FORMAT_ELEMENT(writer, PEAK_THROUGHPUT_VAL_XML_STR, "%"PRIu8, qualityofservice->peakthroughput);
  XML_WRITE_FORMAT_ELEMENT(writer, PRECEDENCE_CLASS_VAL_XML_STR, "%"PRIu8, qualityofservice->precedenceclass);
  XML_WRITE_FORMAT_ELEMENT(writer, MEAN_THROUGHPUT_VAL_XML_STR, "%"PRIu8, qualityofservice->meanthroughput);
  XML_WRITE_FORMAT_ELEMENT(writer, TRAFIC_CLASS_VAL_XML_STR, "%"PRIu8, qualityofservice->trafficclass);
  XML_WRITE_FORMAT_ELEMENT(writer, DELIVERY_ORDER_VAL_XML_STR, "%"PRIu8, qualityofservice->deliveryorder);
  XML_WRITE_FORMAT_ELEMENT(writer, DELIVERY_OF_ERRONEOUS_SDU_VAL_XML_STR, "%"PRIu8, qualityofservice->deliveryoferroneoussdu);
  XML_WRITE_FORMAT_ELEMENT(writer, MAXIMUM_SDU_SIZE_VAL_XML_STR, "%"PRIu8, qualityofservice->maximumsdusize);
  XML_WRITE_FORMAT_ELEMENT(writer, MAXIMUM_BIT_RATE_UPLINK_VAL_XML_STR, "%"PRIu8, qualityofservice->maximumbitrateuplink);
  XML_WRITE_FORMAT_ELEMENT(writer, RESIDUAL_BER_VAL_XML_STR, "%"PRIu8, qualityofservice->residualber);
  XML_WRITE_FORMAT_ELEMENT(writer, SDU_RATIO_ERROR_VAL_XML_STR, "%"PRIu8, qualityofservice->sduratioerror);
  XML_WRITE_FORMAT_ELEMENT(writer, TRANSFER_DELAY_VAL_XML_STR, "%"PRIu8, qualityofservice->transferdelay);
  XML_WRITE_FORMAT_ELEMENT(writer, TRAFFIC_HANDLING_PRIORITY_VAL_XML_STR, "%"PRIu8, qualityofservice->traffichandlingpriority);
  XML_WRITE_FORMAT_ELEMENT(writer, GUARANTEED_BITRATE_UPLINK_VAL_XML_STR, "%"PRIu8, qualityofservice->guaranteedbitrateuplink);
  XML_WRITE_FORMAT_ELEMENT(writer, GUARANTEED_BITRATE_DOWNLINK_VAL_XML_STR, "%"PRIu8, qualityofservice->guaranteedbitratedownlink);
  XML_WRITE_FORMAT_ELEMENT(writer, SIGNALING_INDICATION_VAL_XML_STR, "%"PRIu8, qualityofservice->signalingindication);
  XML_WRITE_FORMAT_ELEMENT(writer, SOURCE_STATISTICS_DESCRIPTOR_VAL_XML_STR, "%"PRIu8, qualityofservice->sourcestatisticsdescriptor);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
// 10.5.6.7 Linked TI
//------------------------------------------------------------------------------
bool linked_ti_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, linked_ti_t * const linkedti)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bool res = false;
  bstring xpath_expr_lti = bformat("./%s",LINKED_TI_IE_XML_STR);
  xmlXPathObjectPtr xpath_obj_lti = xml_find_nodes(xml_doc, &xpath_ctx, xpath_expr_lti);
  if (xpath_obj_lti) {
    xmlNodeSetPtr nodes = xpath_obj_lti->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if ((1 == size) && (xml_doc)) {
      xmlNodePtr saved_node_ptr = xpath_ctx->node;
      res = (RETURNok == xmlXPathSetContextNode(nodes->nodeTab[0], xpath_ctx));

      if (res) {
        uint8_t  tiflag = 0;
        bstring xpath_expr = bformat("./%s",TI_FLAG_IE_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)&tiflag, NULL);
        bdestroy_wrapper (&xpath_expr);
        linkedti->tiflag = tiflag;
      }
      if (res) {
        uint8_t  tivalue       = 0;
        bstring xpath_expr = bformat("./%s",TIO_IE_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&tivalue, NULL);
        bdestroy_wrapper (&xpath_expr);
        linkedti->tivalue = tivalue;
      }
      if (res) {
        uint8_t  ext       = 0;
        bstring xpath_expr = bformat("./%s",TI_EXT_IE_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&ext, NULL);
        bdestroy_wrapper (&xpath_expr);
        linkedti->ext = ext;
      }
      if (res) {
        uint8_t  tivalue_cont       = 0;
        bstring xpath_expr = bformat("./%s",TIE_IE_XML_STR);
        res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNu8, (void*)&tivalue_cont, NULL);
        bdestroy_wrapper (&xpath_expr);
        linkedti->tivalue_cont = tivalue_cont;
      }
      res = (RETURNok == xmlXPathSetContextNode(saved_node_ptr, xpath_ctx)) & res;
    }
  }
  bdestroy_wrapper (&xpath_expr_lti);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}
//------------------------------------------------------------------------------
void linked_ti_to_xml ( const linked_ti_t * const linkedti, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, LINKED_TI_IE_XML_STR);
  XML_WRITE_FORMAT_ELEMENT(writer, TI_FLAG_IE_XML_STR, "%"PRIx8, linkedti->tiflag);
  XML_WRITE_FORMAT_ELEMENT(writer, TIO_IE_XML_STR, "%"PRIu8, linkedti->tivalue);
  XML_WRITE_FORMAT_ELEMENT(writer, TI_EXT_IE_XML_STR, "%"PRIu8, linkedti->ext);
  XML_WRITE_FORMAT_ELEMENT(writer, TIE_IE_XML_STR, "%"PRIu8, linkedti->tivalue_cont);
  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
// 10.5.6.9 LLC service access point identifier
//------------------------------------------------------------------------------
bool llc_service_access_point_identifier_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, llc_service_access_point_identifier_t * const llc_sap_id)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bstring xpath_expr = bformat("./%s",LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)llc_sap_id, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}
//------------------------------------------------------------------------------
void llc_service_access_point_identifier_to_xml (const llc_service_access_point_identifier_t * const llc_sap_id, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IE_XML_STR, "%"PRIu8, *llc_sap_id);
}

//------------------------------------------------------------------------------
// 10.5.6.11 Packet Flow Identifier
//------------------------------------------------------------------------------
bool packet_flow_identifier_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, packet_flow_identifier_t * const packetflowidentifier)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bstring xpath_expr = bformat("./%s",PACKET_FLOW_IDENTIFIER_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)packetflowidentifier, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}
//------------------------------------------------------------------------------
void packet_flow_identifier_to_xml (const packet_flow_identifier_t * const packetflowidentifier, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FLOW_IDENTIFIER_IE_XML_STR, "%"PRIx8, *packetflowidentifier);
}

//------------------------------------------------------------------------------
// 10.5.6.12 Traffic Flow Template
//------------------------------------------------------------------------------
bool packet_filter_content_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, packet_filter_contents_t * const pfc)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  AssertFatal(0, "TODO");
  OAILOG_FUNC_RETURN (LOG_UTIL, false);
}
//------------------------------------------------------------------------------
void packet_filter_content_to_xml (const packet_filter_contents_t * const pfc, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, PACKET_FILTER_CONTENTS_IE_XML_STR);
  XML_WRITE_FORMAT_ATTRIBUTE(writer, COMPONENT_TYPE_IDENTIFIER_ATTR_XML_STR, "%x", pfc->flags);

  if (pfc->flags & TRAFFIC_FLOW_TEMPLATE_IPV4_REMOTE_ADDR_FLAG) {
    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_IPV4_REMOTE_ADDRESS_IE_XML_STR, "%u.%u.%u.%u",
        pfc->ipv4remoteaddr[0].addr, pfc->ipv4remoteaddr[1].addr,
        pfc->ipv4remoteaddr[2].addr, pfc->ipv4remoteaddr[3].addr);

    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_IPV4_REMOTE_ADDRESS_MASK_IE_XML_STR, "%u.%u.%u.%u",
        pfc->ipv4remoteaddr[0].mask, pfc->ipv4remoteaddr[1].mask,
        pfc->ipv4remoteaddr[2].mask, pfc->ipv4remoteaddr[3].mask);
  }

  if (pfc->flags & TRAFFIC_FLOW_TEMPLATE_IPV6_REMOTE_ADDR_FLAG) {
    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_IPV6_REMOTE_ADDRESS_IE_XML_STR,
        "%x%.2x:%x%.2x:%x%.2x:%x%.2x:%x%.2x:%x%.2x:%x%.2x:%x%.2x",
        pfc->ipv6remoteaddr[0].addr, pfc->ipv6remoteaddr[1].addr, pfc->ipv6remoteaddr[2].addr, pfc->ipv6remoteaddr[3].addr,
        pfc->ipv6remoteaddr[4].addr, pfc->ipv6remoteaddr[5].addr, pfc->ipv6remoteaddr[6].addr, pfc->ipv6remoteaddr[7].addr,
        pfc->ipv6remoteaddr[8].addr, pfc->ipv6remoteaddr[9].addr, pfc->ipv6remoteaddr[10].addr, pfc->ipv6remoteaddr[11].addr,
        pfc->ipv6remoteaddr[12].addr, pfc->ipv6remoteaddr[13].addr, pfc->ipv6remoteaddr[14].addr, pfc->ipv6remoteaddr[15].addr);

    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_IPV6_REMOTE_ADDRESS_MASK_IE_XML_STR,
        "%x%.2x:%x%.2x:%x%.2x:%x%.2x:%x%.2x:%x%.2x:%x%.2x:%x%.2x",
        pfc->ipv6remoteaddr[0].mask, pfc->ipv6remoteaddr[1].mask, pfc->ipv6remoteaddr[2].mask, pfc->ipv6remoteaddr[3].mask,
        pfc->ipv6remoteaddr[4].mask, pfc->ipv6remoteaddr[5].mask, pfc->ipv6remoteaddr[6].mask, pfc->ipv6remoteaddr[7].mask,
        pfc->ipv6remoteaddr[8].mask, pfc->ipv6remoteaddr[9].mask, pfc->ipv6remoteaddr[10].mask, pfc->ipv6remoteaddr[11].mask,
        pfc->ipv6remoteaddr[12].mask, pfc->ipv6remoteaddr[13].mask, pfc->ipv6remoteaddr[14].mask, pfc->ipv6remoteaddr[15].mask);
  }

  if (pfc->flags & TRAFFIC_FLOW_TEMPLATE_PROTOCOL_NEXT_HEADER_FLAG) {
    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_PROTOCOL_IDENTIFIER_NEXT_HEADER_TYPE_IE_XML_STR,
        "%u",
        pfc->protocolidentifier_nextheader);
  }

  if (pfc->flags & TRAFFIC_FLOW_TEMPLATE_SINGLE_LOCAL_PORT_FLAG) {
    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_SINGLE_LOCAL_PORT_IE_XML_STR,
        "%u", pfc->singlelocalport);
  }

  if (pfc->flags & TRAFFIC_FLOW_TEMPLATE_LOCAL_PORT_RANGE_FLAG) {
    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_LOCAL_PORT_RANGE_LOW_LIMIT_IE_XML_STR,
        "%u", pfc->localportrange.lowlimit);
    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_LOCAL_PORT_RANGE_HIGH_LIMIT_IE_XML_STR,
        "%u", pfc->localportrange.highlimit);
  }

  if (pfc->flags & TRAFFIC_FLOW_TEMPLATE_SINGLE_REMOTE_PORT_FLAG) {
    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_SINGLE_REMOTE_PORT_IE_XML_STR,
        "%u", pfc->singleremoteport);
  }

  if (pfc->flags & TRAFFIC_FLOW_TEMPLATE_REMOTE_PORT_RANGE_FLAG) {
    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_REMOTE_PORT_RANGE_LOW_LIMIT_IE_XML_STR,
        "%u", pfc->remoteportrange.lowlimit);
    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_REMOTE_PORT_RANGE_HIGH_LIMIT_IE_XML_STR,
        "%u", pfc->remoteportrange.highlimit);
  }

  if (pfc->flags & TRAFFIC_FLOW_TEMPLATE_SECURITY_PARAMETER_INDEX_FLAG) {
    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_SECURITY_PARAMETER_INDEX_IE_XML_STR,
        "%u", pfc->securityparameterindex);
  }

  if (pfc->flags & TRAFFIC_FLOW_TEMPLATE_TYPE_OF_SERVICE_TRAFFIC_CLASS_FLAG) {
    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_TYPE_OF_SERVICE_TRAFFIC_CLASS_IE_XML_STR,
        "%u", pfc->typdeofservice_trafficclass.value);
    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_TYPE_OF_SERVICE_TRAFFIC_CLASS_MASK_IE_XML_STR,
        "%u", pfc->typdeofservice_trafficclass.mask);
  }

  if (pfc->flags & TRAFFIC_FLOW_TEMPLATE_FLOW_LABEL_FLAG) {
    XML_WRITE_FORMAT_ELEMENT(writer, PACKET_FILTER_CONTENTS_FLOW_LABEL_IE_XML_STR,
        "%u", pfc->flowlabel);
  }

  XML_WRITE_END_ELEMENT(writer);
}

//------------------------------------------------------------------------------
bool packet_filter_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, packet_filter_t * const packetfilter)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  AssertFatal(0, "TODO");
  OAILOG_FUNC_RETURN (LOG_UTIL, false);
}
//------------------------------------------------------------------------------
void packet_filter_to_xml (const packet_filter_t * const packetfilter, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, PACKET_FILTER_IE_XML_STR);
  XML_WRITE_FORMAT_ATTRIBUTE(writer, IDENTIFIER_ATTR_XML_STR, "%x", packetfilter->identifier);
  XML_WRITE_FORMAT_ATTRIBUTE(writer, PACKET_FILTER_EVALUATION_PRECEDENCE_ATTR_XML_STR, "%x", packetfilter->eval_precedence);
  switch (packetfilter->direction) {
  case TRAFFIC_FLOW_TEMPLATE_PRE_REL7_TFT_FILTER:
    XML_WRITE_FORMAT_COMMENT(writer, "%s = %s", PACKET_FILTER_DIRECTION_ATTR_XML_STR, PACKET_FILTER_DIRECTION_PRE_REL_7_VAL_XML_STR);
    break;
  case TRAFFIC_FLOW_TEMPLATE_DOWNLINK_ONLY:
    XML_WRITE_FORMAT_COMMENT(writer, "%s = %s", PACKET_FILTER_DIRECTION_ATTR_XML_STR, PACKET_FILTER_DIRECTION_DOWNLINK_ONLY_VAL_XML_STR);
    break;
  case TRAFFIC_FLOW_TEMPLATE_UPLINK_ONLY:
    XML_WRITE_FORMAT_COMMENT(writer, "%s = %s", PACKET_FILTER_DIRECTION_ATTR_XML_STR, PACKET_FILTER_DIRECTION_UPLINK_ONLY_VAL_XML_STR);
    break;
  case TRAFFIC_FLOW_TEMPLATE_BIDIRECTIONAL:
    XML_WRITE_FORMAT_COMMENT(writer, "%s = %s", PACKET_FILTER_DIRECTION_ATTR_XML_STR, PACKET_FILTER_DIRECTION_BIDIRECTIONAL_VAL_XML_STR);
    break;
  default:
    XML_WRITE_FORMAT_COMMENT(writer, "%s = unknown", PACKET_FILTER_DIRECTION_ATTR_XML_STR);
  }
  XML_WRITE_FORMAT_ATTRIBUTE(writer, PACKET_FILTER_DIRECTION_ATTR_XML_STR, "%x", packetfilter->direction);
  XML_WRITE_FORMAT_ATTRIBUTE(writer, LENGTH_ATTR_XML_STR, "%x", packetfilter->length);
  packet_filter_content_to_xml(&packetfilter->packetfiltercontents, writer);
  XML_WRITE_END_ELEMENT(writer);
}
//------------------------------------------------------------------------------
bool packet_filter_identifier_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, packet_filter_identifier_t * const packetfilteridentifier)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  bstring xpath_expr = bformat("./%s",IDENTIFIER_IE_XML_STR);
  bool res = xml_load_leaf_tag(xml_doc, xpath_ctx, xpath_expr, "%"SCNx8, (void*)packetfilteridentifier, NULL);
  bdestroy_wrapper (&xpath_expr);
  OAILOG_FUNC_RETURN (LOG_UTIL, res);
}
//------------------------------------------------------------------------------
void packet_filter_identifier_to_xml (const packet_filter_identifier_t * const packetfilteridentifier, xmlTextWriterPtr writer)
{
  XML_WRITE_FORMAT_ELEMENT(writer, IDENTIFIER_IE_XML_STR, "%"PRIx8, packetfilteridentifier->identifier);
}

//------------------------------------------------------------------------------
bool traffic_flow_template_from_xml (xmlDocPtr xml_doc, xmlXPathContextPtr xpath_ctx, traffic_flow_template_t * const trafficflowtemplate)
{
  OAILOG_FUNC_IN (LOG_UTIL);
  AssertFatal(0, "TODO");
  OAILOG_FUNC_RETURN (LOG_UTIL, false);
}
//------------------------------------------------------------------------------
void traffic_flow_template_to_xml (const traffic_flow_template_t * const trafficflowtemplate, xmlTextWriterPtr writer)
{
  XML_WRITE_START_ELEMENT(writer, TRAFFIC_FLOW_TEMPLATE_IE_XML_STR);
  XML_WRITE_FORMAT_ATTRIBUTE(writer, TFT_OPERATION_CODE_ATTR_XML_STR, "%x", trafficflowtemplate->tftoperationcode);
  XML_WRITE_FORMAT_ATTRIBUTE(writer, E_ATTR_XML_STR, "%x", trafficflowtemplate->ebit);
  XML_WRITE_FORMAT_ATTRIBUTE(writer, NUMBER_OF_PACKET_FILTERS_ATTR_XML_STR, "%u", trafficflowtemplate->numberofpacketfilters);
  switch (trafficflowtemplate->tftoperationcode) {
    case TRAFFIC_FLOW_TEMPLATE_OPCODE_CREATE_NEW_TFT:
      for (int i = 0; i < trafficflowtemplate->numberofpacketfilters; i++) {
        packet_filter_to_xml(&trafficflowtemplate->packetfilterlist.createnewtft[i], writer);
      }
      break;
    case TRAFFIC_FLOW_TEMPLATE_OPCODE_ADD_PACKET_FILTER_TO_EXISTING_TFT:
      for (int i = 0; i < trafficflowtemplate->numberofpacketfilters; i++) {
        packet_filter_to_xml(&trafficflowtemplate->packetfilterlist.addpacketfilter[i], writer);
      }
      break;
    case TRAFFIC_FLOW_TEMPLATE_OPCODE_REPLACE_PACKET_FILTERS_IN_EXISTING_TFT:
      for (int i = 0; i < trafficflowtemplate->numberofpacketfilters; i++) {
        packet_filter_to_xml(&trafficflowtemplate->packetfilterlist.replacepacketfilter[i], writer);
      }
      break;
    case TRAFFIC_FLOW_TEMPLATE_OPCODE_DELETE_PACKET_FILTERS_FROM_EXISTING_TFT:
      for (int i = 0; i < trafficflowtemplate->numberofpacketfilters; i++) {
        packet_filter_identifier_to_xml(&trafficflowtemplate->packetfilterlist.deletepacketfilter[i], writer);
      }
      break;
    case TRAFFIC_FLOW_TEMPLATE_OPCODE_SPARE:
      XML_WRITE_COMMENT(writer, "warning TFT operation code spare");
      break;
    case TRAFFIC_FLOW_TEMPLATE_OPCODE_DELETE_EXISTING_TFT:
    case TRAFFIC_FLOW_TEMPLATE_OPCODE_NO_TFT_OPERATION:
      break;
    case TRAFFIC_FLOW_TEMPLATE_OPCODE_RESERVED:
      XML_WRITE_COMMENT(writer, "warning TFT operation code reserved");
      break;
    default:
      // not possible
      XML_WRITE_FORMAT_COMMENT(writer, "warning unknown TFT operation code %d", trafficflowtemplate->tftoperationcode);
      ;
  }
  if (TRAFFIC_FLOW_TEMPLATE_PARAMETER_LIST_IS_INCLUDED == trafficflowtemplate->ebit) {
    XML_WRITE_COMMENT(writer, "TODO parameters list");
  }
  XML_WRITE_END_ELEMENT(writer);
}

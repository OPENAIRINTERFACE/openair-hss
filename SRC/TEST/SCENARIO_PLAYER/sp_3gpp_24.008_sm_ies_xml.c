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
#include "common_defs.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "log.h"
#include "mme_scenario_player.h"
#include "xml_load.h"
#include "3gpp_24.008_xml.h"
#include "sp_3gpp_24.008_xml.h"
#include "xml2_wrapper.h"


//******************************************************************************
// 10.5.6 Session management information elements
//******************************************************************************
//------------------------------------------------------------------------------
// 10.5.6.1 Access Point Name
//------------------------------------------------------------------------------
bool sp_access_point_name_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, access_point_name_t * access_point_name)
{

  bstring xpath_expr = bformat("./%s",ACCESS_POINT_NAME_XML_STR);
  bool res = sp_xml_load_ascii_stream_leaf_tag (scenario, msg, xpath_expr, access_point_name);
  bdestroy_wrapper (&xpath_expr);
  return res;
}


//------------------------------------------------------------------------------
// 10.5.6.3 Protocol configuration options
//------------------------------------------------------------------------------
bool sp_protocol_configuration_options_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, protocol_configuration_options_t * const pco, bool ms2network_direction)
{
  AssertFatal(0, "TODO");
}

//------------------------------------------------------------------------------
// 10.5.6.5 Quality of service
//------------------------------------------------------------------------------
bool sp_quality_of_service_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, quality_of_service_t * const qualityofservice)
{
  AssertFatal(0, "TODO");
}


//------------------------------------------------------------------------------
// 10.5.6.7 Linked TI
//------------------------------------------------------------------------------
bool sp_linked_ti_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, linked_ti_t * const linkedti)
{
  AssertFatal(0, "TODO");
}


//------------------------------------------------------------------------------
// 10.5.6.9 LLC service access point identifier
//------------------------------------------------------------------------------
bool sp_llc_service_access_point_identifier_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, llc_service_access_point_identifier_t * const llc_sap_id)
{
  AssertFatal(0, "TODO");
}

//------------------------------------------------------------------------------
// 10.5.6.11 Packet Flow Identifier
//------------------------------------------------------------------------------
bool sp_packet_flow_identifier_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, packet_flow_identifier_t * const packetflowidentifier)
{
  AssertFatal(0, "TODO");
}

//------------------------------------------------------------------------------
// 10.5.6.12 Traffic Flow Template
//------------------------------------------------------------------------------
bool sp_packet_filter_content_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, packet_filter_contents_t * const pfc)
{
  AssertFatal(0, "TODO");
}

//------------------------------------------------------------------------------
bool sp_packet_filter_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, packet_filter_t * const packetfilter)
{
  AssertFatal(0, "TODO");
}

//------------------------------------------------------------------------------
bool sp_packet_filter_identifier_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, packet_filter_identifier_t * const packetfilteridentifier)
{
  AssertFatal(0, "TODO");
}

//------------------------------------------------------------------------------
bool sp_traffic_flow_template_from_xml (scenario_t * const scenario, scenario_player_msg_t * const msg, traffic_flow_template_t * const trafficflowtemplate)
{
  AssertFatal(0, "TODO");
}


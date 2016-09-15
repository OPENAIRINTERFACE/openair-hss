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
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "3gpp_24.301.h"
#include "security_types.h"
#include "common_defs.h"
#include "common_types.h"
#include "ApnAggregateMaximumBitRate.h"
#include "EpsQualityOfService.h"
#include "EsmCause.h"
#include "EsmInformationTransferFlag.h"
#include "LinkedEpsBearerIdentity.h"
#include "PdnAddress.h"
#include "PdnType.h"
#include "NasRequestType.h"
#include "RadioPriority.h"
#include "mme_scenario_player.h"
#include "3gpp_24.301_esm_ies_xml.h"
#include "sp_3gpp_24.301_esm_ies_xml.h"

//------------------------------------------------------------------------------
bool sp_apn_aggregate_maximum_bit_rate_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    ApnAggregateMaximumBitRate * const apnaggregatemaximumbitrate)
{
  AssertFatal (0, "TODO if necessary");
}

//------------------------------------------------------------------------------
bool sp_eps_quality_of_service_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    EpsQualityOfService   * const epsqualityofservice)
{
  AssertFatal (0, "TODO if necessary");
}

//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( esm_cause, ESM_CAUSE);

//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( esm_information_transfer_flag, ESM_INFORMATION_TRANSFER_FLAG);
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( linked_eps_bearer_identity, LINKED_EPS_BEARER_IDENTITY);
//------------------------------------------------------------------------------
bool sp_pdn_address_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    PdnAddress            * const pdnaddress)
{
  AssertFatal (0, "TODO if necessary");
}

//------------------------------------------------------------------------------
bool sp_pdn_type_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    pdn_type_t               * const pdntype)
{
  AssertFatal (0, "TODO if necessary");
}


//------------------------------------------------------------------------------
SP_NUM_FROM_XML_GENERATE( request_type, REQUEST_TYPE);



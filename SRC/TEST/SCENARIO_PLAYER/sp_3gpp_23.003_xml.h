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

#ifndef FILE_SP_3GPP_23_003_XML_SEEN
#define FILE_SP_3GPP_23_003_XML_SEEN
#include "sp_xml_load.h"
//==============================================================================
// 2 Identification of mobile subscribers
//==============================================================================

//------------------------------------------------------------------------------
// 2.2 Composition of IMSI
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// 2.4 Structure of TMSI
//------------------------------------------------------------------------------
SP_NUM_FROM_XML_PROTOTYPE(tmsi);

//------------------------------------------------------------------------------
// 2.8 Globally Unique Temporary UE Identity (GUTI)
//------------------------------------------------------------------------------
bool sp_m_tmsi_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    tmsi_t * const m_tmsi);
SP_NUM_FROM_XML_PROTOTYPE(mme_code);
SP_NUM_FROM_XML_PROTOTYPE(mme_gid);
bool sp_gummei_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    gummei_t * const gummei);
bool sp_guti_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    guti_t * const guti);

//------------------------------------------------------------------------------
// 2.9 Structure of the S-Temporary Mobile Subscriber Identity (S-TMSI)
//------------------------------------------------------------------------------
bool sp_s_tmsi_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    s_tmsi_t * const s_tmsi);

//==============================================================================
// 4 Identification of location areas and base stations
//==============================================================================

//------------------------------------------------------------------------------
// 4.7  Closed Subscriber Group
//------------------------------------------------------------------------------
bool sp_csg_id_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    csg_id_t * const csg_id);


//==============================================================================
// 6 International Mobile Station Equipment Identity and Software Version Number
//==============================================================================

//------------------------------------------------------------------------------
// 6.2.1 Composition of IMEI
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// 6.2.2 Composition of IMEISV
//------------------------------------------------------------------------------


//==============================================================================
// 19  Numbering, addressing and identification for the Evolved Packet Core (EPC)
//==============================================================================

//------------------------------------------------------------------------------
// 19.6  E-UTRAN Cell Identity (ECI) and E-UTRAN Cell Global Identification (ECGI)
//------------------------------------------------------------------------------

bool sp_eci_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    eci_t                 * const eci);
bool sp_ecgi_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    ecgi_t                * const ecgi);

#endif /* FILE_SP_3GPP_23_003_XML_SEEN */

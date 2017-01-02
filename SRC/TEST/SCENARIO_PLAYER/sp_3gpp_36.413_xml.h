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

#ifndef FILE_SP_3GPP_36_413_XML_SEEN
#define FILE_SP_3GPP_36_413_XML_SEEN

#include "sp_xml_load.h"

bool sp_e_rab_setup_item_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    e_rab_setup_item_t * const item);

bool sp_e_rab_setup_list_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    e_rab_setup_list_t        * const list);

bool sp_e_rab_to_be_setup_list_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    e_rab_to_be_setup_list_t * const list);

bool sp_e_rab_to_be_setup_item_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    e_rab_to_be_setup_item_t * const item);

SP_NUM_FROM_XML_PROTOTYPE(e_rab_id);

bool sp_e_rab_level_qos_parameters_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    e_rab_level_qos_parameters_t * const params);

bool sp_allocation_and_retention_priority_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    allocation_and_retention_priority_t * const arp);

bool sp_gbr_qos_information_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    gbr_qos_information_t * const gqi);

bool sp_ue_aggregate_maximum_bit_rate_from_xml (
    scenario_t            * const scenario,
    scenario_player_msg_t * const msg,
    ue_aggregate_maximum_bit_rate_t * const ue_ambr);

#endif /* FILE_SP_3GPP_36_413_XML_SEEN */

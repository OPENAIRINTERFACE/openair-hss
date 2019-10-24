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

/*! \file 3gpp_requirements_23.246.h
   \brief
   \author Dincer BEKEN
   \date   2019
   \email: dbeken@blackned.de
*/
#ifndef FILE_3GPP_REQUIREMENTS_23_246_SEEN
#define FILE_3GPP_REQUIREMENTS_23_246_SEEN

#include "3gpp_requirements.h"
#include "log.h"

#define REQUIREMENT_3GPP_23_246(rElEaSe_sEcTiOn__OaImark) REQUIREMENT_3GPP_SPEC(LOG_NAS, "Hit 3GPP TS 23_246"#rElEaSe_sEcTiOn__OaImark" : "rElEaSe_sEcTiOn__OaImark##_BRIEF"\n")
#define NO_REQUIREMENT_3GPP_23_246(rElEaSe_sEcTiOn__OaImark) REQUIREMENT_3GPP_SPEC(LOG_NAS, "#NOT IMPLEMENTED 3GPP TS 23_246"#rElEaSe_sEcTiOn__OaImark" : "rElEaSe_sEcTiOn__OaImark##_BRIEF"\n")
#define NOT_REQUIREMENT_3GPP_23_246(rElEaSe_sEcTiOn__OaImark) REQUIREMENT_3GPP_SPEC(LOG_NAS, "#NOT ASSERTED 3GPP TS 23_246"#rElEaSe_sEcTiOn__OaImark" : "rElEaSe_sEcTiOn__OaImark##_BRIEF"\n")

//-----------------------------------------------------------------------------------------------------------------------
// MBMS Session Start Request
//-----------------------------------------------------------------------------------------------------------------------
#define R8_3_2__6 "MME23.246R10_8.3.2_6: MBMS Session Start procedure\
                                                                                                                        \
    The MME may return an MBMS Session Start Response to the MBMS-GW as soon as the session request is             \
    accepted by one E-UTRAN node."                                                                                \

#define R8_3_2__6_BRIEF "MME Response to MBMS Start Request after first successful eNB"

//-----------------------------------------------------------------------------------------------------------------------

#endif /* FILE_3GPP_REQUIREMENTS_23_246_SEEN */

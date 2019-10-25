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

/*! \file 3gpp_requirements_29.468.h
   \brief
   \author Dincer BEKEN
   \date   2019
   \email: dbeken@blackned.de
*/
#ifndef FILE_3GPP_REQUIREMENTS_29_468_SEEN
#define FILE_3GPP_REQUIREMENTS_29_468_SEEN

#include "3gpp_requirements.h"
#include "log.h"

#define REQUIREMENT_3GPP_29_468(rElEaSe_sEcTiOn__OaImark) REQUIREMENT_3GPP_SPEC(LOG_NAS, "Hit 3GPP TS 29_468"#rElEaSe_sEcTiOn__OaImark" : "rElEaSe_sEcTiOn__OaImark##_BRIEF"\n")
#define NO_REQUIREMENT_3GPP_29_468(rElEaSe_sEcTiOn__OaImark) REQUIREMENT_3GPP_SPEC(LOG_NAS, "#NOT IMPLEMENTED 3GPP TS 29_468"#rElEaSe_sEcTiOn__OaImark" : "rElEaSe_sEcTiOn__OaImark##_BRIEF"\n")
#define NOT_REQUIREMENT_3GPP_29_468(rElEaSe_sEcTiOn__OaImark) REQUIREMENT_3GPP_SPEC(LOG_NAS, "#NOT ASSERTED 3GPP TS 29_468"#rElEaSe_sEcTiOn__OaImark" : "rElEaSe_sEcTiOn__OaImark##_BRIEF"\n")

//-----------------------------------------------------------------------------------------------------------------------
// Modify MBMS Bearer Procedure
//-----------------------------------------------------------------------------------------------------------------------
#define R5_3_4 "MME29.468R15_5.3.4: MBMS Session Start procedure\
                                                                                                                        \
    The MBMS-StartStop-Indication AVP set to \"UPDATE\", the TMGI AVP and the MBMS-Flow-Identifier AVP					\
	to designate the bearer to be modified."																		    \

#define R5_3_4_BRIEF "MBMS Flow-Id in MBMS Update Bearer Request"
//-----------------------------------------------------------------------------------------------------------------------

#endif /* FILE_3GPP_REQUIREMENTS_29_468_SEEN */

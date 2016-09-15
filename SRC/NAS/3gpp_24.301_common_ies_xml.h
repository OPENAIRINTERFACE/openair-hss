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

#ifndef FILE_3GPP_24_301_COMMON_IES_XML_SEEN
#define FILE_3GPP_24_301_COMMON_IES_XML_SEEN

#include "EpsBearerContextStatus.h"

//==============================================================================
// 9 General message format and information elements coding
//==============================================================================

//------------------------------------------------------------------------------
// 9.9.2 Common information elements
//------------------------------------------------------------------------------
// 9.9.2.0A Device properties
// See subclause 10.5.7.8 in 3GPP TS 24.008 [13].

// 9.9.2.1 EPS bearer context status
#define EPS_BEARER_CONTEXT_STATUS_IE_XML_STR        "eps_bearer_context_status"
#define EPS_BEARER_CONTEXT_STATUS_XML_SCAN_FMT      "%"SCNx16
NUM_FROM_XML_PROTOTYPE(eps_bearer_context_status);
void eps_bearer_context_status_to_xml (eps_bearer_context_status_t * epsbearercontextstatus, xmlTextWriterPtr writer);

// 9.9.2.2 Location area identification
// See subclause 10.5.1.3 in 3GPP TS 24.008 [13].
// 9.9.2.3 Mobile identity
// See subclause 10.5.1.4 in 3GPP TS 24.008 [13].
// 9.9.2.4 Mobile station classmark 2
// See subclause 10.5.1.6 in 3GPP TS 24.008 [13].
// 9.9.2.5Mobile station classmark 3
// See subclause 10.5.1.7 in 3GPP TS 24.008 [13].

// 9.9.2.6 NAS security parameters from E-UTRA
// 9.9.2.7 NAS security parameters to E-UTRA

// 9.9.2.8 PLMN list
// See subclause 10.5.1.13 in 3GPP TS 24.008 [13].

// 9.9.2.9 Spare half octet

//9.9.2.10 Supported codec list
//See subclause 10.5.4.32 in 3GPP TS 24.008 [13].


#endif /* FILE_3GPP_24_301_COMMON_IES_XML_SEEN */

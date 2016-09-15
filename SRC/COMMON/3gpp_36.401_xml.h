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

#ifndef FILE_3GPP_36_401_XML_SEEN
#define FILE_3GPP_36_401_XML_SEEN

#define ENB_UE_S1AP_ID_IE_XML_STR         "enb_ue_s1ap_id"
#define MME_UE_S1AP_ID_IE_XML_STR         "mme_ue_s1ap_id"
#define ENB_UE_S1AP_ID_XML_FMT            "%06"PRIX32
#define ENB_UE_S1AP_ID_XML_SCAN_FMT       "%"SCNx32
#define MME_UE_S1AP_ID_XML_FMT            "%08"PRIX32
#define MME_UE_S1AP_ID_XML_SCAN_FMT       "%"SCNx32

#include "xml_load.h"

NUM_FROM_XML_PROTOTYPE(enb_ue_s1ap_id);
NUM_FROM_XML_PROTOTYPE(mme_ue_s1ap_id);

void enb_ue_s1ap_id_to_xml (const enb_ue_s1ap_id_t * const enb_ue_s1ap_id, xmlTextWriterPtr writer);
void mme_ue_s1ap_id_to_xml (const mme_ue_s1ap_id_t * const mme_ue_s1ap_id, xmlTextWriterPtr writer);

#endif /* FILE_3GPP_36_401_XML_SEEN */

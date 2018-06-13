/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file 3gpp_36.401.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_3GPP_36_401_SEEN
#define FILE_3GPP_36_401_SEEN

//------------------------------------------------------------------------------
// 6.2 E-UTRAN Identifiers
//------------------------------------------------------------------------------



typedef uint32_t                 enb_ue_s1ap_id_t;         /*!< \brief  An eNB UE S1AP ID shall be allocated so as to uniquely identify the UE over the S1 interface within an eNB.
                                                                        When an MME receives an eNB UE S1AP ID it shall store it for the duration of the UE-associated logical S1-connection for this UE.
                                                                        Once known to an MME this IE is included in all UE associated S1-AP signalling.
                                                                        The eNB UE S1AP ID shall be unique within the eNB logical node. */

typedef uint32_t                 mme_ue_s1ap_id_t;         /*!< \brief  A MME UE S1AP ID shall be allocated so as to uniquely identify the UE over the S1 interface within the MME.
                                                                        When an eNB receives MME UE S1AP ID it shall store it for the duration of the UE-associated logical S1-connection for this UE.
                                                                        Once known to an eNB this IE is included in all UE associated S1-AP signalling.
                                                                        The MME UE S1AP ID shall be unique within the MME logical node.*/


#endif /* FILE_3GPP_36_401_SEEN */

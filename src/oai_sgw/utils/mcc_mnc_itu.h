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

/*! \file mcc_mnc_itu.h
  \brief  Defines the MCC/MNC list delivered by the ITU
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#ifndef FILE_MCC_MNC_SEEN
#define FILE_MCC_MNC_SEEN

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mcc_mnc_list_s {
  uint16_t mcc;
  char     mnc[4];
} mcc_mnc_list_t;

__attribute__ ((pure)) int find_mnc_length(const char mcc_digit1P,
                    const char mcc_digit2P,
                    const char mcc_digit3P,
                    const char mnc_digit1P,
                    const char mnc_digit2P,
                    const char mnc_digit3P);

#ifdef __cplusplus
}
#endif

#endif

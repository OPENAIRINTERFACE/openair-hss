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

#ifndef FILE_ETSI_TS_135_206_V10_0_0_ANNEX3_SEEN
#define FILE_ETSI_TS_135_206_V10_0_0_ANNEX3_SEEN
/*--------------------------- prototypes --------------------------*/
void f1 ( uint8_t k[16], uint8_t rand[16], uint8_t sqn[6], uint8_t amf[2], uint8_t mac_a[8] );
void f2345 ( uint8_t k[16], uint8_t rand[16], uint8_t res[8], uint8_t ck[16], uint8_t ik[16], uint8_t ak[6] );
void f1star( uint8_t k[16], uint8_t rand[16], uint8_t sqn[6], uint8_t amf[2], uint8_t mac_s[8] );
void f5star( uint8_t k[16], uint8_t rand[16], uint8_t ak[6] );
void ComputeOPc( uint8_t op_c[16] );
void RijndaelKeySchedule( uint8_t key[16] );
void RijndaelEncrypt( uint8_t input[16], uint8_t output[16] );
#endif /* FILE_ETSI_TS_135_206_V10_0_0_ANNEX3_SEEN */

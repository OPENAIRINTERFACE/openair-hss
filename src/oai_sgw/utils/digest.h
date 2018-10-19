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

/*! \file digest.h
   \brief: Digest function
   \author  Lionel GAUTHIER
   \date 2017
   \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_DIGEST_SEEN
#define FILE_DIGEST_SEEN
#include <openssl/evp.h>

#ifdef __cplusplus
extern "C" {
#endif

int digest_buffer(const EVP_MD *(*evp_x)(void), const unsigned char *buffer, size_t buffer_len, unsigned char **digest, unsigned int *digest_len);

#ifdef __cplusplus
}
#endif

#endif /* FILE_DIGEST_SEEN */

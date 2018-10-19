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

/*! \file digest.c
   \brief
   \author  Lionel GAUTHIER
   \date 2017
   \email: lionel.gauthier@eurecom.fr
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "log.h"
#include "common_defs.h"
#include "digest.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// evp_x can be EVP_sha256, ...
int digest_buffer(const EVP_MD *(*evp_x)(void), const unsigned char *buffer, size_t buffer_len, unsigned char **digest, unsigned int *digest_len)
{
  EVP_MD_CTX * mdctx = NULL;

  if((mdctx = EVP_MD_CTX_create())) {
    if(1 == EVP_DigestInit_ex(mdctx, (*evp_x)(), NULL)) {
      if(1 == EVP_DigestUpdate(mdctx, buffer, buffer_len)) {
        if((*digest = (unsigned char *)OPENSSL_malloc(EVP_MD_size((*evp_x)())))) {
          if(1 == EVP_DigestFinal_ex(mdctx, *digest, digest_len)) {
            EVP_MD_CTX_destroy(mdctx);
            return RETURNok;
          } else {
            OPENSSL_free(*digest);
            OAILOG_ERROR (LOG_UTIL, "Digest EVP_DigestFinal_ex()\n");
          }
        } else {
          OAILOG_ERROR (LOG_UTIL, "Digest OPENSSL_malloc()\n");
        }
      } else {
        OAILOG_ERROR (LOG_UTIL, "Digest EVP_DigestUpdate()\n");
      }
    } else {
      OAILOG_ERROR (LOG_UTIL, "Digest EVP_DigestInit_ex()\n");
    }
    EVP_MD_CTX_destroy(mdctx);
  } else {
    OAILOG_ERROR (LOG_UTIL, "Digest EVP_MD_CTX_create()\n");
  }
  *digest = NULL;
  *digest_len = 0;
  return RETURNerror;
}

#ifdef __cplusplus
}
#endif

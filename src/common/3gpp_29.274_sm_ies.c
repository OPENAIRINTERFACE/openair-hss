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

/*! \file 3gpp_24.008_sm_ies.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>

#include "bstrlib.h"

#include "log.h"
#include "dynamic_memory_check.h"
#include "common_defs.h"
#include "assertions.h"
#include "3gpp_29.274.h"
#include "3gpp_24.008.h"


//------------------------------------------------------------------------------
void free_bearer_contexts_to_be_created(bearer_contexts_to_be_created_t **bcs_tbc){
  bearer_contexts_to_be_created_t * bctbc = *bcs_tbc;
  // nothing to do for packet filters
  for (int i = 0; i < bctbc->num_bearer_context; i++) {
    for (int j = 0; j < bctbc->bearer_contexts[i].tft.parameterslist.num_parameters; j++) {
      bdestroy_wrapper(&bctbc->bearer_contexts[i].tft.parameterslist.parameter[j]);
    }
  }
  free_wrapper((void**)bcs_tbc);
}

//------------------------------------------------------------------------------
// 7.3.1 PDN Connections IE
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
static void free_pdn_connection(pdn_connection_t * pdn_connection)
{
  /** APN String. */
  bdestroy_wrapper (&pdn_connection->apn_str);

  /** Bearer Contexts to be Created. */
  free_bearer_contexts_to_be_created(&pdn_connection->bearer_context_list);
}

//------------------------------------------------------------------------------
void free_mme_ue_eps_pdn_connections(mme_ue_eps_pdn_connections_t ** pdn_connections)
{
  mme_ue_eps_pdn_connections_t * pdnconnections = *pdn_connections;
  // nothing to do for packet filters
  for (int i = 0; i < pdnconnections->num_pdn_connections; i++) {
    free_pdn_connection(&pdnconnections->pdn_connection[i]);
  }
  free_wrapper((void**)pdn_connections);
}

//------------------------------------------------------------------------------
// 8.38 MM Context IE
//------------------------------------------------------------------------------

void free_mm_context_eps(mm_context_eps_t ** ue_eps_mm_context){
  mm_context_eps_t * ueepsmmcontext = *ue_eps_mm_context;

  for(int i = 0; i < ueepsmmcontext->num_quad; i++){
    if(ueepsmmcontext->auth_quadruplet[i])
      free_wrapper((void**)&ueepsmmcontext->auth_quadruplet[i]);
  }

  for(int i = 0; i < ueepsmmcontext->num_quit; i++){
    if(ueepsmmcontext->auth_quintuplet[i])
      free_wrapper((void**)&ueepsmmcontext->auth_quintuplet[i]);
  }

  free_wrapper((void**)ue_eps_mm_context);
}



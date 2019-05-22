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

/*! \file mme_app_edns_emulation.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "bstrlib.h"

#include "log.h"
#include "obj_hashtable.h"
#include "mme_config.h"
#include "common_defs.h"
#include "dynamic_memory_check.h"
#include "mme_app_edns_emulation.h"



static obj_hash_table_t * sgw_e_dns_entries = NULL;
static obj_hash_table_t * mme_e_dns_entries = NULL;

//------------------------------------------------------------------------------
struct sockaddr* mme_app_edns_get_wrr_entry(bstring id, const interface_type_t interface_type)
{
  struct sockaddr *sockaddr = NULL;
  switch(interface_type){
  case S10_MME_GTP_C:
	  obj_hashtable_get (mme_e_dns_entries, bdata(id), blength(id),
	      (void **)&sockaddr);
	  break;
  case S11_SGW_GTP_C:
  	  obj_hashtable_get (sgw_e_dns_entries, bdata(id), blength(id),
  	      (void **)&sockaddr);
  	  break;
  default :
	  break;
  }

  return sockaddr;
}

//------------------------------------------------------------------------------
int mme_app_edns_add_wrr_entry(bstring id, struct sockaddr *edns_ip_addr, const interface_type_t interface_type)
{
  char * cid = calloc(1, blength(id)+1);
  if (cid) {
    strncpy(cid, (const char *)id->data, blength(id));

    struct sockaddr_in *data;
    if(edns_ip_addr->sa_family == AF_INET) {
    	data = malloc(sizeof(struct sockaddr_in));
    	memcpy((struct sockaddr_in*)data, (struct sockaddr_in*)edns_ip_addr, sizeof(struct sockaddr_in));
    } else if (edns_ip_addr->sa_family == AF_INET6) {
    	data = malloc(sizeof(struct sockaddr_in6));
    	memcpy((struct sockaddr_in6*)data, (struct sockaddr_in6*)edns_ip_addr, sizeof(struct sockaddr_in6));
    } else {
    	return RETURNerror;
    }
    if (data) {
      hashtable_rc_t rc;
      switch(interface_type){
      case S10_MME_GTP_C:
    	  rc = obj_hashtable_insert (mme_e_dns_entries, cid, strlen(cid), data);
    	  break;
      case S11_SGW_GTP_C:
          rc = obj_hashtable_insert (sgw_e_dns_entries, cid, strlen(cid), data);
      	  break;
      default :
    	  return RETURNerror;
      }
      /** Key is copied inside. */
      free_wrapper(&cid);
      if (HASH_TABLE_OK == rc) return RETURNok;
    }
  }
  return RETURNerror;
}

//------------------------------------------------------------------------------
int  mme_app_edns_init (const mme_config_t * mme_config_p)
{
  int rc = RETURNok;
  sgw_e_dns_entries = obj_hashtable_create (min(64, MME_CONFIG_MAX_SERVICE), NULL, free_wrapper, free_wrapper, NULL);
  mme_e_dns_entries = obj_hashtable_create (min(64, MME_CONFIG_MAX_SERVICE), NULL, free_wrapper, free_wrapper, NULL);
  if (sgw_e_dns_entries && mme_e_dns_entries) {
    /** Add the service (s10 or s11). */
    for (int i = 0; i < mme_config_p->e_dns_emulation.nb_service_entries; i++) {
    	rc |= mme_app_edns_add_wrr_entry(mme_config_p->e_dns_emulation.service_id[i], &mme_config_p->e_dns_emulation.sockaddr[i], mme_config_p->e_dns_emulation.interface_type[i]);
    }
//    /** Add the neighboring MMEs. */
//    for (int i = 0; i < mme_config_p->e_dns_emulation.nb_mme_entries; i++) {
//      rc |= mme_app_edns_add_mme_entry(mme_config_p->e_dns_emulation.mme_id[i], mme_config_p->e_dns_emulation.mme_ip_addr[i]);
//    }
    return rc;
  }
  return RETURNerror;
}

//------------------------------------------------------------------------------
void  mme_app_edns_exit (void)
{
  obj_hashtable_destroy (sgw_e_dns_entries);
  obj_hashtable_destroy (mme_e_dns_entries);
}

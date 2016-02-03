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

/*! \file sgw_lite_context_manager.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#define SGW_LITE
#define SGW_LITE_CONTEXT_MANAGER_C

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "tree.h"
#include "hashtable.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "sgw_lite_defs.h"
#include "sgw_lite_context_manager.h"
#include "sgw_lite.h"
#include "dynamic_memory_check.h"
#include "log.h"

extern sgw_app_t                        sgw_app;


//-----------------------------------------------------------------------------
static void
sgw_lite_display_s11teid2mme_mapping (
  uint64_t keyP,
  void *dataP,
  void *unusedParameterP)
//-----------------------------------------------------------------------------
{
  mme_sgw_tunnel_t                       *mme_sgw_tunnel = NULL;

  if (dataP != NULL) {
    mme_sgw_tunnel = (mme_sgw_tunnel_t *) dataP;
    LOG_DEBUG (LOG_SPGW_APP, "| %u\t<------------->\t%u\n", mme_sgw_tunnel->remote_teid, mme_sgw_tunnel->local_teid);
  } else {
    LOG_DEBUG (LOG_SPGW_APP, "INVALID S11 TEID MAPPING FOUND\n");
  }
}

//-----------------------------------------------------------------------------
void
sgw_lite_display_s11teid2mme_mappings (
  void)
//-----------------------------------------------------------------------------
{
  LOG_DEBUG (LOG_SPGW_APP, "+--------------------------------------+\n");
  LOG_DEBUG (LOG_SPGW_APP, "| MME <--- S11 TE ID MAPPINGS ---> SGW |\n");
  LOG_DEBUG (LOG_SPGW_APP, "+--------------------------------------+\n");
  hashtable_ts_apply_funct_on_elements (sgw_app.s11teid2mme_hashtable, sgw_lite_display_s11teid2mme_mapping, NULL);
  LOG_DEBUG (LOG_SPGW_APP, "+--------------------------------------+\n");
}

//-----------------------------------------------------------------------------
static void
sgw_lite_display_pdn_connection_sgw_eps_bearers (
  uint64_t keyP,
  void *dataP,
  void *unusedParameterP)
//-----------------------------------------------------------------------------
{
  sgw_eps_bearer_entry_t                 *eps_bearer_entry = NULL;

  if (dataP != NULL) {
    eps_bearer_entry = (sgw_eps_bearer_entry_t *) dataP;
    LOG_DEBUG (LOG_SPGW_APP, "|\t\t\t\t%" PRId64 "\t<-> ebi: %u, enb_teid_for_S1u: %u, s_gw_teid_for_S1u_S12_S4_up: %u (tbc)\n",
                    keyP, eps_bearer_entry->eps_bearer_id, eps_bearer_entry->enb_teid_for_S1u, eps_bearer_entry->s_gw_teid_for_S1u_S12_S4_up);
  } else {
    LOG_DEBUG (LOG_SPGW_APP, "\t\t\t\tINVALID eps_bearer_entry FOUND\n");
  }
}

//-----------------------------------------------------------------------------
static void
sgw_lite_display_s11_bearer_context_information (
  uint64_t keyP,
  void *dataP,
  void *unusedParameterP)
//-----------------------------------------------------------------------------
{
  s_plus_p_gw_eps_bearer_context_information_t *sp_context_information = NULL;
  hashtable_rc_t                          hash_rc;

  if (dataP != NULL) {
    sp_context_information = (s_plus_p_gw_eps_bearer_context_information_t *) dataP;
    LOG_DEBUG (LOG_SPGW_APP, "| KEY %" PRId64 ":      \n", keyP);
    LOG_DEBUG (LOG_SPGW_APP, "|\tsgw_eps_bearer_context_information:     |\n");
    //Imsi_t               imsi;                           ///< IMSI (International Mobile Subscriber Identity) is the subscriber permanent identity.
    LOG_DEBUG (LOG_SPGW_APP, "|\t\timsi_unauthenticated_indicator:\t%u\n", sp_context_information->sgw_eps_bearer_context_information.imsi_unauthenticated_indicator);
    //char                 msisdn[MSISDN_LENGTH];          ///< The basic MSISDN of the UE. The presence is dictated by its storage in the HSS.
    LOG_DEBUG (LOG_SPGW_APP, "|\t\tmme_teid_for_S11:              \t%u\n", sp_context_information->sgw_eps_bearer_context_information.mme_teid_for_S11);
    //ip_address_t         mme_ip_address_for_S11;         ///< MME IP address the S11 interface.
    LOG_DEBUG (LOG_SPGW_APP, "|\t\ts_gw_teid_for_S11_S4:          \t%u\n", sp_context_information->sgw_eps_bearer_context_information.s_gw_teid_for_S11_S4);
    //ip_address_t         s_gw_ip_address_for_S11_S4;     ///< S-GW IP address for the S11 interface and the S4 Interface (control plane).
    //cgi_t                last_known_cell_Id;             ///< This is the last location of the UE known by the network
    LOG_DEBUG (LOG_SPGW_APP, "|\t\tpdn_connection:\n");
    LOG_DEBUG (LOG_SPGW_APP, "|\t\t\tapn_in_use:        %s\n", sp_context_information->sgw_eps_bearer_context_information.pdn_connection.apn_in_use);
    LOG_DEBUG (LOG_SPGW_APP, "|\t\t\tdefault_bearer:    %u\n", sp_context_information->sgw_eps_bearer_context_information.pdn_connection.default_bearer);
    LOG_DEBUG (LOG_SPGW_APP, "|\t\t\teps_bearers:\n");
    hash_rc = hashtable_ts_apply_funct_on_elements (sp_context_information->sgw_eps_bearer_context_information.pdn_connection.sgw_eps_bearers, sgw_lite_display_pdn_connection_sgw_eps_bearers, NULL);

    if (hash_rc != HASH_TABLE_OK) {
      LOG_DEBUG (LOG_SPGW_APP, "Invalid sgw_eps_bearers hashtable for display\n");
    }
    //void                  *trxn;
    //uint32_t               peer_ip;
  } else {
    LOG_DEBUG (LOG_SPGW_APP, "INVALID s_plus_p_gw_eps_bearer_context_information FOUND\n");
  }
}

//-----------------------------------------------------------------------------
void
sgw_lite_display_s11_bearer_context_information_mapping (
  void)
//-----------------------------------------------------------------------------
{
  LOG_DEBUG (LOG_SPGW_APP, "+-----------------------------------------+\n");
  LOG_DEBUG (LOG_SPGW_APP, "| S11 BEARER CONTEXT INFORMATION MAPPINGS |\n");
  LOG_DEBUG (LOG_SPGW_APP, "+-----------------------------------------+\n");
  hashtable_ts_apply_funct_on_elements (sgw_app.s11_bearer_context_information_hashtable, sgw_lite_display_s11_bearer_context_information, NULL);
  LOG_DEBUG (LOG_SPGW_APP, "+--------------------------------------+\n");
}

//-----------------------------------------------------------------------------
void
pgw_lite_cm_free_apn (
  pgw_apn_t * apnP)
//-----------------------------------------------------------------------------
{
  if (apnP != NULL) {
    if (apnP->pdn_connections != NULL) {
      obj_hashtable_ts_destroy (apnP->pdn_connections);
    }
  }
}

//-----------------------------------------------------------------------------
Teid_t
sgw_lite_get_new_S11_tunnel_id (
  void)
//-----------------------------------------------------------------------------
{
  // TO DO: RANDOM
  static Teid_t                           tunnel_id = 0;

  tunnel_id += 1;
  return tunnel_id;
}

//-----------------------------------------------------------------------------
mme_sgw_tunnel_t                       *
sgw_lite_cm_create_s11_tunnel (
  Teid_t remote_teid,
  Teid_t local_teid)
//-----------------------------------------------------------------------------
{
  mme_sgw_tunnel_t                       *new_tunnel;

  new_tunnel = MALLOC_CHECK (sizeof (mme_sgw_tunnel_t));

  if (new_tunnel == NULL) {
    /*
     * Malloc failed, may be ENOMEM error
     */
    LOG_ERROR (LOG_SPGW_APP, "Failed to create tunnel for remote_teid %u\n", remote_teid);
    return NULL;
  }

  new_tunnel->remote_teid = remote_teid;
  new_tunnel->local_teid = local_teid;
  /*
   * Trying to insert the new tunnel into the tree.
   * * * * If collision_p is not NULL (0), it means tunnel is already present.
   */
  hashtable_ts_insert (sgw_app.s11teid2mme_hashtable, local_teid, new_tunnel);
  return new_tunnel;
}

//-----------------------------------------------------------------------------
int
sgw_lite_cm_remove_s11_tunnel (
  Teid_t local_teid)
//-----------------------------------------------------------------------------
{
  int                                     temp;

  temp = hashtable_ts_free (sgw_app.s11teid2mme_hashtable, local_teid);
  return temp;
}

//-----------------------------------------------------------------------------
sgw_eps_bearer_entry_t                 *
sgw_lite_cm_create_eps_bearer_entry (
  void)
//-----------------------------------------------------------------------------
{
  sgw_eps_bearer_entry_t                 *eps_bearer_entry;

  eps_bearer_entry = MALLOC_CHECK (sizeof (sgw_eps_bearer_entry_t));

  if (eps_bearer_entry == NULL) {
    /*
     * Malloc failed, may be ENOMEM error
     */
    LOG_ERROR (LOG_SPGW_APP, "Failed to create new EPS bearer object\n");
    return NULL;
  }

  return eps_bearer_entry;
}


//-----------------------------------------------------------------------------
sgw_pdn_connection_t                   *
sgw_lite_cm_create_pdn_connection (
  void)
//-----------------------------------------------------------------------------
{
  sgw_pdn_connection_t                   *pdn_connection;

  pdn_connection = MALLOC_CHECK (sizeof (sgw_pdn_connection_t));

  if (pdn_connection == NULL) {
    /*
     * Malloc failed, may be ENOMEM error
     */
    LOG_ERROR (LOG_SPGW_APP, "Failed to create new PDN connection object\n");
    return NULL;
  }

  memset (pdn_connection, 0, sizeof (sgw_pdn_connection_t));
  pdn_connection->sgw_eps_bearers = hashtable_ts_create (12, NULL, NULL, "sgw_eps_bearers");

  if (pdn_connection->sgw_eps_bearers == NULL) {
    LOG_ERROR (LOG_SPGW_APP, "Failed to create eps bearers collection object\n");
    FREE_CHECK (pdn_connection);
    pdn_connection = NULL;
    return NULL;
  }

  return pdn_connection;
}

//-----------------------------------------------------------------------------
void
sgw_lite_cm_free_pdn_connection (
  sgw_pdn_connection_t * pdn_connectionP)
//-----------------------------------------------------------------------------
{
  if (pdn_connectionP != NULL) {
    if (pdn_connectionP->sgw_eps_bearers != NULL) {
      hashtable_ts_destroy (pdn_connectionP->sgw_eps_bearers);
    }
  }
}

//-----------------------------------------------------------------------------
void
sgw_lite_cm_free_s_plus_p_gw_eps_bearer_context_information (
  s_plus_p_gw_eps_bearer_context_information_t * contextP)
//-----------------------------------------------------------------------------
{
  if (contextP == NULL) {
    return;
  }

  /*
   * if (contextP->sgw_eps_bearer_context_information.pdn_connections != NULL) {
   * obj_hashtable_ts_destroy(contextP->sgw_eps_bearer_context_information.pdn_connections);
   * }
   */
  if (contextP->sgw_eps_bearer_context_information.pdn_connection.sgw_eps_bearers != NULL) {
    hashtable_ts_destroy (contextP->sgw_eps_bearer_context_information.pdn_connection.sgw_eps_bearers);
  }

  if (contextP->pgw_eps_bearer_context_information.apns != NULL) {
    obj_hashtable_ts_destroy (contextP->pgw_eps_bearer_context_information.apns);
  }

  FREE_CHECK (contextP);
}

//-----------------------------------------------------------------------------
s_plus_p_gw_eps_bearer_context_information_t *
sgw_lite_cm_create_bearer_context_information_in_collection (
  Teid_t teid)
//-----------------------------------------------------------------------------
{
  s_plus_p_gw_eps_bearer_context_information_t *new_bearer_context_information;

  new_bearer_context_information = MALLOC_CHECK (sizeof (s_plus_p_gw_eps_bearer_context_information_t));

  if (new_bearer_context_information == NULL) {
    /*
     * Malloc failed, may be ENOMEM error
     */
    LOG_ERROR (LOG_SPGW_APP, "Failed to create new bearer context information object for S11 remote_teid %u\n", teid);
    return NULL;
  }

  memset (new_bearer_context_information, 0, sizeof (s_plus_p_gw_eps_bearer_context_information_t));
  LOG_DEBUG (LOG_SPGW_APP, "sgw_lite_cm_create_bearer_context_information_in_collection %d\n", teid);
  /*
   * new_bearer_context_information->sgw_eps_bearer_context_information.pdn_connections = obj_hashtable_ts_create(32, NULL, NULL, sgw_lite_cm_free_pdn_connection);
   * 
   * if ( new_bearer_context_information->sgw_eps_bearer_context_information.pdn_connections == NULL) {
   * SPGW_APP_ERROR("Failed to create PDN connections collection object entry for EPS bearer teid %u \n", teid);
   * sgw_lite_cm_free_s_plus_p_gw_eps_bearer_context_information(new_bearer_context_information);
   * return NULL;
   * }
   */
  new_bearer_context_information->pgw_eps_bearer_context_information.apns = obj_hashtable_ts_create (32, NULL, NULL, pgw_lite_cm_free_apn, "pgw_eps_bearer_ctxt_info_apns");

  if (new_bearer_context_information->pgw_eps_bearer_context_information.apns == NULL) {
    LOG_ERROR (LOG_SPGW_APP, "Failed to create APN collection object entry for EPS bearer S11 teid %u \n", teid);
    sgw_lite_cm_free_s_plus_p_gw_eps_bearer_context_information (new_bearer_context_information);
    return NULL;
  }

  /*
   * Trying to insert the new tunnel into the tree.
   * * * * If collision_p is not NULL (0), it means tunnel is already present.
   */
  hashtable_ts_insert (sgw_app.s11_bearer_context_information_hashtable, teid, new_bearer_context_information);
  LOG_DEBUG (LOG_SPGW_APP, "Added new s_plus_p_gw_eps_bearer_context_information_t in s11_bearer_context_information_hashtable key teid %u\n", teid);
  return new_bearer_context_information;
}

int
sgw_lite_cm_remove_bearer_context_information (
  Teid_t teid)
{
  int                                     temp;

  temp = hashtable_ts_free (sgw_app.s11_bearer_context_information_hashtable, teid);
  return temp;
}

//--- EPS Bearer Entry

//-----------------------------------------------------------------------------
sgw_eps_bearer_entry_t                 *
sgw_lite_cm_create_eps_bearer_entry_in_collection (
  hash_table_t * eps_bearersP,
  ebi_t eps_bearer_idP)
//-----------------------------------------------------------------------------
{
  sgw_eps_bearer_entry_t                 *new_eps_bearer_entry;
  hashtable_rc_t                          hash_rc = HASH_TABLE_OK;

  if (eps_bearersP == NULL) {
    LOG_ERROR (LOG_SPGW_APP, "Failed to create EPS bearer entry for EPS bearer id %u. reason eps bearer hashtable is NULL \n", eps_bearer_idP);
    return NULL;
  }

  new_eps_bearer_entry = MALLOC_CHECK (sizeof (sgw_eps_bearer_entry_t));

  if (new_eps_bearer_entry == NULL) {
    /*
     * Malloc failed, may be ENOMEM error
     */
    LOG_ERROR (LOG_SPGW_APP, "Failed to create EPS bearer entry for EPS bearer id %u \n", eps_bearer_idP);
    return NULL;
  }

  memset (new_eps_bearer_entry, 0, sizeof (sgw_eps_bearer_entry_t));
  new_eps_bearer_entry->eps_bearer_id = eps_bearer_idP;
  hash_rc = hashtable_ts_insert (eps_bearersP, eps_bearer_idP, new_eps_bearer_entry);
  LOG_DEBUG (LOG_SPGW_APP, "Inserted new EPS bearer entry for EPS bearer id %u status %s\n", eps_bearer_idP, hashtable_rc_code2string (hash_rc));
  hash_rc = hashtable_ts_apply_funct_on_elements (eps_bearersP, sgw_lite_display_pdn_connection_sgw_eps_bearers, NULL);

  if (hash_rc != HASH_TABLE_OK) {
    LOG_DEBUG (LOG_SPGW_APP, "Invalid sgw_eps_bearers hashtable for display\n");
  }

  /*
   * CHECK DUPLICATES IN HASH TABLES ? if (temp == 1) {
   * SPGW_APP_WARN("This EPS bearer entry already exists: %u\n", eps_bearer_idP);
   * FREE_CHECK(new_eps_bearer_entry);
   * new_eps_bearer_entry = collision_p;
   * }
   */
  return new_eps_bearer_entry;
}

//-----------------------------------------------------------------------------
int
sgw_lite_cm_remove_eps_bearer_entry (
  hash_table_t * eps_bearersP,
  ebi_t eps_bearer_idP)
//-----------------------------------------------------------------------------
{
  int                                     temp;

  if (eps_bearersP == NULL) {
    return -1;
  }

  temp = hashtable_ts_free (eps_bearersP, eps_bearer_idP);
  return temp;
}

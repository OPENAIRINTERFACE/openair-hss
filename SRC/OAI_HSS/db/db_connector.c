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


#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <inttypes.h>
#include <cassandra.h>
#include <mysql/mysql.h>

#include "hss_config.h"
#include "db_proto.h"
#include "log.h"

extern void                             ComputeOPc (
  const uint8_t const kP[16],
  const uint8_t const opP[16],
  uint8_t opcP[16]);


db_cassandra_t                             *db_desc;

static void
print_buffer (
  const char *prefix,
  uint8_t * buffer,
  int length)
{
  int                                     i;

  fprintf (stdout, "%s", prefix);

  for (i = 0; i < length; i++) {
    fprintf (stdout, "%02x.", buffer[i]);
  }

  fprintf (stdout, "\n");
}

#if 0
int
hss_mysql_connect (
  const hss_config_t * hss_config_p)
{
  const int                               mysql_reconnect_val = 1;

  if ((hss_config_p->mysql_server == NULL) || (hss_config_p->mysql_user == NULL) || (hss_config_p->mysql_password == NULL) || (hss_config_p->mysql_database == NULL)) {
    FPRINTF_ERROR ( "An empty name is not allowed\n");
    return EINVAL;
  }

  FPRINTF_DEBUG ("Initializing db layer\n");
  db_desc = malloc (sizeof (database_t));

  if (db_desc == NULL) {
    FPRINTF_DEBUG ("An error occured on MALLOC\n");
    return errno;
  }

  pthread_mutex_init (&db_desc->db_cs_mutex, NULL);
  /*
   * Copy database configuration from static hss config
   */
  db_desc->server = strdup (hss_config_p->mysql_server);
  db_desc->user = strdup (hss_config_p->mysql_user);
  db_desc->password = strdup (hss_config_p->mysql_password);
  db_desc->database = strdup (hss_config_p->mysql_database);
  /*
   * Init mySQL client
   */
  db_desc->db_conn = mysql_init (NULL);
  mysql_options (db_desc->db_conn, MYSQL_OPT_RECONNECT, &mysql_reconnect_val);

  /*
   * Try to connect to database
   */
  if (!mysql_real_connect (db_desc->db_conn, db_desc->server, db_desc->user, db_desc->password, db_desc->database, 0, NULL, 0)) {
    FPRINTF_ERROR ("An error occured while connecting to db: %s\n", mysql_error (db_desc->db_conn));
    mysql_thread_end();
    return -1;
  }

  /*
   * Set the multi statement ON
   */
  mysql_set_server_option (db_desc->db_conn, MYSQL_OPTION_MULTI_STATEMENTS_ON);
  FPRINTF_DEBUG ("Initializing db layer: DONE\n");
  return 0;
}

#endif

int cassandra_real_connect(const char* cassandra_server)
{
    /* Setup and connect to cluster */
    CassCluster* cluster = cass_cluster_new();
    CassSession* session = cass_session_new();

    /* Add contact points */
    cass_cluster_set_contact_points(cluster, cassandra_server);
    CassFuture* connect_future = cass_session_connect(session, cluster);
    CassError rc = cass_future_error_code(connect_future);

    db_desc->db_conn = session;
    db_desc->db_cluster = cluster;
    db_desc->db_connect_future = connect_future;
    FPRINTF_DEBUG("Connect result: %s\n", cass_error_desc(rc));
    if(rc == CASS_OK){
        FPRINTF_DEBUG("Successfully connected!\n");
    }
    else{
        /*Handle error*/
        const char* message;
        size_t message_length;
        cass_future_error_message(connect_future,&message,&message_length);
        FPRINTF_ERROR( "Unable to connect: '%.*s'\n", (int)message_length, message);
	cass_future_free(connect_future);
	//Look for reconnect options in Cassandra
        return -1;
    }
    return 0;
}


int
hss_cassandra_connect (
  const hss_config_t * hss_config_p)
{
  const int                               mysql_reconnect_val = 1;

  if ((hss_config_p->mysql_server == NULL) || (hss_config_p->mysql_user == NULL) || (hss_config_p->mysql_password == NULL) || (hss_config_p->mysql_database == NULL)) {
    FPRINTF_ERROR ( "An empty name is not allowed\n");
    return EINVAL;
  }

  FPRINTF_DEBUG ("Initializing db layer\n");
  db_desc = malloc (sizeof (db_cassandra_t));

  if (db_desc == NULL) {
    FPRINTF_DEBUG ("An error occured on MALLOC\n");
    return errno;
  }

  pthread_mutex_init (&db_desc->db_cs_mutex, NULL);
  /*
   * Copy database configuration from static hss config
   */
  db_desc->server = strdup (hss_config_p->mysql_server);
  db_desc->user = strdup (hss_config_p->mysql_user);
  db_desc->password = strdup (hss_config_p->mysql_password);
  db_desc->database = strdup (hss_config_p->mysql_database);
  /*
   * Init mysql client
   */
  //db_desc->db_conn = mysql_init (NULL);
  //mysql_options (db_desc->db_conn, MYSQL_OPT_RECONNECT, &mysql_reconnect_val);

  /*
   * Try to connect to database
   */
  if (cassandra_real_connect (hss_config_p->mysql_server) != 0) {
    FPRINTF_ERROR ("An error occured while connecting to db: %s\n",hss_config_p->mysql_server);
    //mysql_thread_end();
    return -1;
  }

  /*
   * Set the multi statement ON
   */
  //mysql_set_server_option (db_desc->db_conn, MYSQL_OPTION_MULTI_STATEMENTS_ON);
  FPRINTF_DEBUG ("Initializing db layer: DONE\n");
  return 0;
}



void
hss_mysql_disconnect (
  void)
{
  mysql_close (db_desc->db_conn);
  mysql_thread_end();
}

int
hss_mysql_update_loc (
  const char *imsi,
  mysql_ul_ans_t * mysql_ul_ans)
{
  MYSQL_RES                              *res;
  MYSQL_ROW                               row;
  char                                    query[1000];
  int                                     ret = 0;

  if ((db_desc->db_conn == NULL) || (mysql_ul_ans == NULL)) {
    return EINVAL;
  }

  if (strlen (imsi) > 15) {
    return EINVAL;
  }

  sprintf (query, "SELECT `access_restriction`,`mmeidentity_idmmeidentity`," "`msisdn`,`ue_ambr_ul`,`ue_ambr_dl`,`rau_tau_timer` " "FROM `users` WHERE `users`.`imsi`='%s' ", imsi);
  memcpy (mysql_ul_ans->imsi, imsi, strlen (imsi) + 1);
  FPRINTF_DEBUG ("Query: %s\n", query);
  pthread_mutex_lock (&db_desc->db_cs_mutex);

  if (mysql_query (db_desc->db_conn, query)) {
    pthread_mutex_unlock (&db_desc->db_cs_mutex);
    FPRINTF_ERROR ("Query execution failed: %s\n", mysql_error (db_desc->db_conn));
    return EINVAL;
  }

  res = mysql_store_result (db_desc->db_conn);
  pthread_mutex_unlock (&db_desc->db_cs_mutex);

  if ( res == NULL )
    return EINVAL;

  if ((row = mysql_fetch_row (res)) != NULL) {
    int                                     mme_id;

    /*
     * MSISDN may be NULL
     */
    mysql_ul_ans->access_restriction = atoi (row[0]);

    if ((mme_id = atoi (row[1])) > 0) {
      ret = hss_mysql_query_mmeidentity (mme_id, &mysql_ul_ans->mme_identity);
    } else {
      mysql_ul_ans->mme_identity.mme_host[0] = '\0';
      mysql_ul_ans->mme_identity.mme_realm[0] = '\0';
    }

    if (row[2] != 0) {
      memcpy (mysql_ul_ans->msisdn, row[2], strlen (row[2]));
    }

    mysql_ul_ans->aggr_ul = atoi (row[3]);
    mysql_ul_ans->aggr_dl = atoi (row[4]);
    mysql_ul_ans->rau_tau = atoi (row[5]);
  }

  mysql_free_result (res);
  return ret;
}

int
hss_cassandra_update_loc (
  const char *imsi,
  cassandra_ul_ans_t * cass_ul_ans)
{
  char                                    query[1000];
  int                                     ret = 0;
  const CassRow                           *row;
  CassStatement                           *statement;
  CassFuture                              *query_future;
  CassError                               rc;
  const CassResult                        *result = NULL;
  CassValue                               *cass_access_res_value,*cass_idmme_value,*cass_msisdn_value,*cass_aggr_ul_value,*cass_aggr_dl_value,*cass_rau_tau_value;
  const char				  *msisdn;
  size_t			 	  msisdn_length;
  cass_int32_t                            access_res,idmme,rau_tau;
  cass_int64_t			          aggr_ul,aggr_dl;
  
  if ((db_desc->db_conn == NULL) || (cass_ul_ans == NULL)) {
    return EINVAL;
  }

  if (strlen (imsi) > 15) {
    return EINVAL;
  }

  sprintf (query, "SELECT access_restriction,mmeidentity_idmmeidentity, msisdn,ue_ambr_ul,ue_ambr_dl,rau_tau_timer FROM vhss.users_imsi WHERE imsi='%s' ", imsi);
  statement = cass_statement_new(query,0);
  memcpy (cass_ul_ans->imsi, imsi, strlen (imsi) + 1);
  FPRINTF_DEBUG ("Query: %s\n", query);
  pthread_mutex_lock (&db_desc->db_cs_mutex);
  query_future = cass_session_execute(db_desc->db_conn,statement);
  cass_statement_free(statement);
  rc = cass_future_error_code(query_future);
  if ( rc != CASS_OK) {
        pthread_mutex_unlock(&db_desc->db_cs_mutex);
        const char* message;
        size_t message_length;
        cass_future_error_message(query_future,&message,&message_length);
        FPRINTF_ERROR( "Query execution failed: '%.*s'\n", (int)message_length, message);
        return EINVAL;
  }

  result = cass_future_get_result(query_future);
  /* If there was an error then the result won't be available */
  if (result == NULL) {
     /* Handle error */
     FPRINTF_DEBUG("Select query result is NULL\n");
     pthread_mutex_unlock(&db_desc->db_cs_mutex);
     cass_future_free(query_future);
     return EINVAL;
  }
  pthread_mutex_unlock(&db_desc->db_cs_mutex);
  cass_future_free(query_future);
  row = cass_result_first_row(result);  
  cass_access_res_value = cass_row_get_column_by_name(row,"access_restriction");
  cass_idmme_value = cass_row_get_column_by_name(row,"mmeidentity_idmmeidentity");
  cass_msisdn_value = cass_row_get_column_by_name(row,"msisdn");
  cass_aggr_ul_value = cass_row_get_column_by_name(row,"ue_ambr_ul");
  cass_aggr_dl_value = cass_row_get_column_by_name(row,"ue_ambr_dl");
  cass_rau_tau_value = cass_row_get_column_by_name(row,"rau_tau_timer");

  if( row != NULL ){
	int 	mme_id;
        cass_value_get_int32(cass_access_res_value,&access_res);
        cass_value_get_int32(cass_idmme_value,&idmme);
        cass_value_get_string(cass_msisdn_value,&msisdn,&msisdn_length);
        cass_value_get_int64(cass_aggr_ul_value,&aggr_ul);
        cass_value_get_int64(cass_aggr_dl_value,&aggr_dl);
        cass_value_get_int32(cass_rau_tau_value,&rau_tau);
        
        cass_ul_ans->access_restriction =  access_res ;
      
        if ((idmme) > 0 ){
	    ret = hss_cassandra_query_mmeidentity (idmme, &cass_ul_ans->mme_identity);
	}else {
            cass_ul_ans->mme_identity.mme_host[0] = '\0';
            cass_ul_ans->mme_identity.mme_realm[0] = '\0';
        }

        if (msisdn != NULL) {
	    memcpy(cass_ul_ans->msisdn, msisdn, strlen(msisdn));
        }

        cass_ul_ans->aggr_ul = aggr_ul;
        cass_ul_ans->aggr_dl = aggr_dl;
        cass_ul_ans->rau_tau = rau_tau;
	
	printf("access_restriction = %d \n",cass_ul_ans->access_restriction);
	printf("mmeidentity_idmmeidentity = %d \n",idmme);
	printf("msisdn = %s \n",cass_ul_ans->msisdn);
	printf("aggr_ul = %d\n",cass_ul_ans->aggr_ul);
	printf("rau_tau = %d\n",cass_ul_ans->rau_tau);
  }
  FPRINTF_DEBUG("At the end of hss_cassandra_update_loc\n");

  cass_result_free(result);
  return ret;
}

int
hss_mysql_purge_ue (
  mysql_pu_req_t * mysql_pu_req,
  mysql_pu_ans_t * mysql_pu_ans)
{
  MYSQL_RES                              *res;
  MYSQL_ROW                               row;
  char                                    query[1000];
  int                                     ret = 0;

  if ((db_desc->db_conn == NULL) || (mysql_pu_req == NULL) || (mysql_pu_ans == NULL)) {
    return EINVAL;
  }

  if (strlen (mysql_pu_req->imsi) > 15) {
    return EINVAL;
  }


  sprintf (query, "UPDATE `users` SET `users`.`ms_ps_status`=\"PURGED\" " "WHERE `users`.`imsi`='%s'; " "SELECT `users`.`mmeidentity_idmmeidentity` FROM `users` " "WHERE `users`.`imsi`='%s' ", mysql_pu_req->imsi, mysql_pu_req->imsi);
  FPRINTF_DEBUG ("Query: %s\n", query);
  pthread_mutex_lock (&db_desc->db_cs_mutex);

  if (mysql_query (db_desc->db_conn, query)) {
    pthread_mutex_unlock (&db_desc->db_cs_mutex);
    FPRINTF_ERROR ("Query execution failed: %s\n", mysql_error (db_desc->db_conn));
    return EINVAL;
  }

  res = mysql_store_result (db_desc->db_conn);
  pthread_mutex_unlock (&db_desc->db_cs_mutex);

  if ( res == NULL )
    return EINVAL;

  if ((row = mysql_fetch_row (res)) != NULL) {
    int                                     mme_id;

    if ((mme_id = atoi (row[0])) > 0) {
      ret = hss_mysql_query_mmeidentity (mme_id, mysql_pu_ans);
    } else {
      mysql_pu_ans->mme_host[0] = '\0';
      mysql_pu_ans->mme_realm[0] = '\0';
    }

    mysql_free_result (res);
    return ret;
  }

  mysql_free_result (res);
  return EINVAL;
}

int
hss_cassandra_purge_ue (
  cassandra_pu_req_t * cass_pu_req,
  cassandra_pu_ans_t * cass_pu_ans)
{
  CassStatement				  *statement;
  CassFuture				  *query_future;
  CassError				  rc;
  const CassResult                        *result = NULL;
  CassValue				  *cass_idmme_value;
  const CassRow                           *row;
  const char				  *idmme;
  size_t				  idmme_length;
  char                                    query[1000];
  int                                     ret = 0,mme_id;
  
  if ((db_desc->db_conn == NULL) || (cass_pu_req == NULL) || (cass_pu_ans == NULL)) {
    return EINVAL;
  }

  if (strlen (cass_pu_req->imsi) > 15) {
    FPRINTF_DEBUG("IMSI length cannot be greater than 15\n");
    return EINVAL;
  }


  sprintf (query, "UPDATE `vhss.users_imsi` SET `users_imsi`.`ms_ps_status`=\"PURGED\" " "WHERE `users_imsi`.`imsi`='%s'; " "SELECT `users_imsi`.`mmeidentity_idmmeidentity` FROM `users_imsi` " "WHERE `users_imsi`.`imsi`='%s' ", cass_pu_req->imsi, cass_pu_req->imsi);
  FPRINTF_DEBUG ("Query: %s\n", query);

  statement = cass_statement_new(query,0);
  pthread_mutex_lock (&db_desc->db_cs_mutex);
  query_future = cass_session_execute(db_desc->db_conn,statement);
  cass_statement_free(statement);
  rc = cass_future_error_code(query_future);
  if ( rc != CASS_OK) {
        pthread_mutex_unlock(&db_desc->db_cs_mutex);
        const char* message;
        size_t message_length;
        cass_future_error_message(query_future,&message,&message_length);
        FPRINTF_ERROR( "Query execution failed: '%.*s'\n", (int)message_length, message);
        return EINVAL;
  }

  result = cass_future_get_result(query_future);
  /* If there was an error then the result won't be available */
  if (result == NULL) {
     /* Handle error */
     FPRINTF_DEBUG("Select query result is NULL\n");
     pthread_mutex_unlock(&db_desc->db_cs_mutex);
     cass_future_free(query_future);
     return EINVAL;
  }
  pthread_mutex_unlock(&db_desc->db_cs_mutex);
  cass_future_free(query_future);

  row = cass_result_first_row(result);
  cass_idmme_value = cass_row_get_column_by_name(row,"mmeidentity_idmmeidentity");
  cass_result_free(result);
  if( cass_idmme_value == NULL )
     return EINVAL;

  cass_value_get_string(cass_idmme_value,&idmme,&idmme_length);
  if ((mme_id = atoi(idmme)) > 0 ){
            ret = hss_cassandra_query_mmeidentity (mme_id, cass_pu_ans);
        }else {
            cass_pu_ans->mme_host[0] = '\0';
            cass_pu_ans->mme_realm[0] = '\0';
        }

  return ret;

}

int
hss_mysql_get_user (
  const char *imsi)
{
  MYSQL_RES                              *res;
  MYSQL_ROW                               row;
  char                                    query[1000];

  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }


  sprintf (query, "SELECT `imsi` FROM `users` WHERE `users`.`imsi`='%s' ", imsi);
  FPRINTF_DEBUG ("Query: %s\n", query);
  pthread_mutex_lock (&db_desc->db_cs_mutex);

  if (mysql_query (db_desc->db_conn, query)) {
    pthread_mutex_unlock (&db_desc->db_cs_mutex);
    FPRINTF_ERROR ("Query execution failed: %s\n", mysql_error (db_desc->db_conn));
    return EINVAL;
  }

  res = mysql_store_result (db_desc->db_conn);
  pthread_mutex_unlock (&db_desc->db_cs_mutex);

  if ( res == NULL )
    return EINVAL;

  if ((row = mysql_fetch_row (res)) != NULL) {
    mysql_free_result (res);
    return 0;
  }

  mysql_free_result (res);
  return EINVAL;
}

int
mysql_push_up_loc (
  mysql_ul_push_t * ul_push_p)
{
  MYSQL_RES                              *res;
  char                                    query[1000];
  int                                     query_length = 0;
  int                                     status;

  if ((db_desc->db_conn == NULL) || (ul_push_p == NULL)) {
    return EINVAL;
  }
  // TODO: multi-statement check results

  if (ul_push_p->mme_identity_present == MME_IDENTITY_PRESENT) {
    query_length += sprintf (&query[query_length], "INSERT INTO `mmeidentity`"
                             " (`mmehost`,`mmerealm`) SELECT '%s','%s' FROM `mmeidentity` WHERE NOT"
                             " EXISTS (SELECT * FROM `mmeidentity` WHERE `mmehost`='%s'"
                             " AND `mmerealm`='%s') LIMIT 1;", ul_push_p->mme_identity.mme_host, ul_push_p->mme_identity.mme_realm, ul_push_p->mme_identity.mme_host, ul_push_p->mme_identity.mme_realm);
  }

  query_length += sprintf (&query[query_length], "UPDATE `users`%s SET ", ul_push_p->mme_identity_present == MME_IDENTITY_PRESENT ? ",`mmeidentity`" : "");

  if (ul_push_p->imei_present == IMEI_PRESENT) {
    query_length += sprintf (&query[query_length], "`imei`='%s',", ul_push_p->imei);
  }

  if (ul_push_p->sv_present == SV_PRESENT) {
    query_length += sprintf (&query[query_length], "`imeisv`='%*s',", 2, ul_push_p->software_version);
  }

  if (ul_push_p->mme_identity_present == MME_IDENTITY_PRESENT) {
    query_length += sprintf (&query[query_length], "`users`.`mmeidentity_idmmeidentity`=`mmeidentity`.`idmmeidentity`, " "`users`.`ms_ps_status`=\"NOT_PURGED\"");
  }

  query_length += sprintf (&query[query_length], " WHERE `users`.`imsi`='%s'", ul_push_p->imsi);

  if (ul_push_p->mme_identity_present == MME_IDENTITY_PRESENT) {
    query_length += sprintf (&query[query_length], " AND `mmeidentity`.`mmehost`='%s'" " AND `mmeidentity`.`mmerealm`='%s'", ul_push_p->mme_identity.mme_host, ul_push_p->mme_identity.mme_realm);
  }

  FPRINTF_DEBUG ("Query: %s\n", query);
  pthread_mutex_lock (&db_desc->db_cs_mutex);

  if (mysql_query (db_desc->db_conn, query)) {
    pthread_mutex_unlock (&db_desc->db_cs_mutex);
    fprintf (stderr, "Query execution failed: %s\n", mysql_error (db_desc->db_conn));
    return EINVAL;
  }

  /*
   * process each statement result
   */
  do {
    /*
     * did current statement return data?
     */
    res = mysql_store_result (db_desc->db_conn);

    if (res) {
      /*
       * yes; process rows and free the result set
       */
      mysql_free_result (res);
    } else {                    /* no result set or error */
      if (mysql_field_count (db_desc->db_conn) == 0) {
        FPRINTF_ERROR ( "%lld rows affected\n", mysql_affected_rows (db_desc->db_conn));
      } else {                  /* some error occurred */
        FPRINTF_ERROR ( "Could not retrieve result set\n");
        break;
      }
    }

    /*
     * more results? -1 = no, >0 = error, 0 = yes (keep looping)
     */
    if ((status = mysql_next_result (db_desc->db_conn)) > 0)
      FPRINTF_ERROR ( "Could not execute statement\n");
  } while (status == 0);

  pthread_mutex_unlock (&db_desc->db_cs_mutex);
  return 0;
}

int
cassandra_push_up_loc (
  cassandra_ul_push_t * ul_push_p)
{
  CassStatement                           *statement;
  CassFuture				  *query_future;
  CassError 			          rc;
  char                                    query[1000];
  int                                     query_length = 0;
  int                                     status;

  if ((db_desc->db_conn == NULL) || (ul_push_p == NULL)) {
    return EINVAL;
  }
  // TODO: multi-statement check results

  FPRINTF_DEBUG("Inside cassandra_push_up_loc function\n");
  if (ul_push_p->mme_identity_present == MME_IDENTITY_PRESENT) {
    query_length += sprintf (&query[query_length], "INSERT INTO vhss.mmeidentity"
                             " (mmehost,mmerealm) SELECT mmehost,mmerealm FROM vhss.mmeidentity WHERE NOT"
                             " EXISTS (SELECT * FROM vhss.mmeidentity WHERE mmehost='%s'"
                             " AND mmerealm='%s') LIMIT 1;", ul_push_p->mme_identity.mme_host, ul_push_p->mme_identity.mme_realm, ul_push_p->mme_identity.mme_host, ul_push_p->mme_identity.mme_realm);
  }

  query_length += sprintf (&query[query_length], "UPDATE vhss.users_imsi %s SET ", ul_push_p->mme_identity_present == MME_IDENTITY_PRESENT ? ",`mmeidentity`" : "");

  if (ul_push_p->imei_present == IMEI_PRESENT) {
    query_length += sprintf (&query[query_length], "imei='%s',", ul_push_p->imei);
  }

  if (ul_push_p->sv_present == SV_PRESENT) {
    query_length += sprintf (&query[query_length], "imeisv='%*s',", 2, ul_push_p->software_version);
  }

  if (ul_push_p->mme_identity_present == MME_IDENTITY_PRESENT) {
    query_length += sprintf (&query[query_length], "users_imsi.mmeidentity_idmmeidentity=mmeidentity.idmmeidentity, " "users_imsi.ms_ps_status=\"NOT_PURGED\"");
  }

  query_length += sprintf (&query[query_length], " WHERE users_imsi.imsi='%s'", ul_push_p->imsi);

  if (ul_push_p->mme_identity_present == MME_IDENTITY_PRESENT) {
    query_length += sprintf (&query[query_length], " AND mmeidentity.mmehost='%s'" " AND mmeidentity.mmerealm='%s'", ul_push_p->mme_identity.mme_host, ul_push_p->mme_identity.mme_realm);
  }

  FPRINTF_DEBUG ("Query: %s\n", query);

  statement = cass_statement_new(query,0);
  pthread_mutex_lock (&db_desc->db_cs_mutex);
  query_future = cass_session_execute(db_desc->db_conn,statement);
  cass_statement_free(statement);
  rc = cass_future_error_code(query_future);
  if( rc != CASS_OK ){
        pthread_mutex_unlock(&db_desc->db_cs_mutex);
        const char* message;
        size_t message_length;
        cass_future_error_message(query_future,&message,&message_length);
        FPRINTF_ERROR( "Query execution failed: '%.*s'\n", (int)message_length, message);
        return EINVAL;
  }

  #if 0
   
  /*
   * process each statement result
   */
  do {
    /*
     * did current statement return data?
     */
    res = mysql_store_result (db_desc->db_conn);

    if (res) {
      /*
       * yes; process rows and free the result set
       */
      mysql_free_result (res);
    } else {                    /* no result set or error */
      if (mysql_field_count (db_desc->db_conn) == 0) {
        FPRINTF_ERROR ( "%lld rows affected\n", mysql_affected_rows (db_desc->db_conn));
      } else {                  /* some error occurred */
        FPRINTF_ERROR ( "Could not retrieve result set\n");
        break;
      }
    }

    /*
     * more results? -1 = no, >0 = error, 0 = yes (keep looping)
     */
    if ((status = mysql_next_result (db_desc->db_conn)) > 0)
      FPRINTF_ERROR ( "Could not execute statement\n");
  } while (status == 0);

  #endif

  pthread_mutex_unlock (&db_desc->db_cs_mutex);
  cass_future_free(query_future);
  FPRINTF_DEBUG("At the end of cassandra_push_up_loc function\n");
  return 0;
}

int
hss_mysql_push_rand_sqn (
  const char *imsi,
  uint8_t * rand_p,
  uint8_t * sqn)
{
  int                                     status = 0,
    i;
  MYSQL_RES                              *res;
  char                                    query[1000];
  int                                     query_length = 0;
  uint64_t                                sqn_decimal = 0;

  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }

  if (rand_p == NULL || sqn == NULL) {
    return EINVAL;
  }

  sqn_decimal = ((uint64_t) sqn[0] << 40) | ((uint64_t) sqn[1] << 32) | ((uint64_t) sqn[2] << 24) | (sqn[3] << 16) | (sqn[4] << 8) | sqn[5];
  query_length = sprintf (query, "UPDATE `users` SET `rand`=UNHEX('");

  for (i = 0; i < RAND_LENGTH; i++) {
    query_length += sprintf (&query[query_length], "%02x", rand_p[i]);
  }

  query_length += sprintf (&query[query_length], "'),`sqn`=%" PRIu64, sqn_decimal);
  query_length += sprintf (&query[query_length], " WHERE `users`.`imsi`='%s'", imsi);
  FPRINTF_DEBUG ("Query: %s\n", query);
  pthread_mutex_lock (&db_desc->db_cs_mutex);

  if (mysql_query (db_desc->db_conn, query)) {
    pthread_mutex_unlock (&db_desc->db_cs_mutex);
    FPRINTF_ERROR ("Query execution failed: %s\n", mysql_error (db_desc->db_conn));
    return EINVAL;
  }

  /*
   * process each statement result
   */
  do {
    /*
     * did current statement return data?
     */
    res = mysql_store_result (db_desc->db_conn);

    if (res) {
      /*
       * yes; process rows and free the result set
       */
      mysql_free_result (res);
    } else {                    /* no result set or error */
      if (mysql_field_count (db_desc->db_conn) == 0) {
        FPRINTF_ERROR ( "%lld rows affected\n", mysql_affected_rows (db_desc->db_conn));
      } else {                  /* some error occurred */
        FPRINTF_ERROR ( "Could not retrieve result set\n");
        break;
      }
    }

    /*
     * more results? -1 = no, >0 = error, 0 = yes (keep looping)
     */
    if ((status = mysql_next_result (db_desc->db_conn)) > 0)
      FPRINTF_ERROR ( "Could not execute statement\n");
  } while (status == 0);

  pthread_mutex_unlock (&db_desc->db_cs_mutex);
  return 0;
}

int
hss_cassandra_push_rand_sqn (
  const char *imsi,
  uint8_t * rand_p,
  uint8_t * sqn)
{
  int                                     status = 0,
    i;
  char                                    query[1000];
  int                                     query_length = 0;
  uint64_t                                sqn_decimal = 0;
  const CassResult			  *result = NULL;
  CassStatement				  *statement;
  CassFuture				  *query_future;
  CassError				  rc;

  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }

  if (rand_p == NULL || sqn == NULL) {
    return EINVAL;
  }

  sqn_decimal = ((uint64_t) sqn[0] << 40) | ((uint64_t) sqn[1] << 32) | ((uint64_t) sqn[2] << 24) | (sqn[3] << 16) | (sqn[4] << 8) | sqn[5];
  query_length = sprintf (query, "UPDATE vhss.users_imsi SET rand='");

  for (i = 0; i < RAND_LENGTH; i++) {
    query_length += sprintf (&query[query_length], "%02x", rand_p[i]);
  }

  query_length += sprintf (&query[query_length], "', sqn=%" PRIu64, sqn_decimal);
  query_length += sprintf (&query[query_length], " WHERE imsi='%s'", imsi);
  FPRINTF_DEBUG ("Query: %s\n", query);

  statement = cass_statement_new(query,0);
  pthread_mutex_lock (&db_desc->db_cs_mutex);
  query_future = cass_session_execute(db_desc->db_conn,statement);
  cass_statement_free(statement);
  rc = cass_future_error_code(query_future);
  if( rc != CASS_OK ){
	pthread_mutex_unlock(&db_desc->db_cs_mutex);
        const char* message;
        size_t message_length;
        cass_future_error_message(query_future,&message,&message_length);
        FPRINTF_ERROR( "Query execution failed: '%.*s'\n", (int)message_length, message);
        return EINVAL;
  }


  
  #if 0 
  /*
   * process each statement result
   */
  do {
    /*
     * did current statement return data?
     */
    res = mysql_store_result (db_desc->db_conn);

    if (res) {
      /*
       * yes; process rows and free the result set
       */
      mysql_free_result (res);
    } else {                    /* no result set or error */
      if (mysql_field_count (db_desc->db_conn) == 0) {
        FPRINTF_ERROR ( "%lld rows affected\n", mysql_affected_rows (db_desc->db_conn));
      } else {                  /* some error occurred */
        FPRINTF_ERROR ( "Could not retrieve result set\n");
        break;
      }
    }

    /*
     * more results? -1 = no, >0 = error, 0 = yes (keep looping)
     */
    if ((status = mysql_next_result (db_desc->db_conn)) > 0)
      FPRINTF_ERROR ( "Could not execute statement\n");
  } while (status == 0);
  #endif

  pthread_mutex_unlock (&db_desc->db_cs_mutex);
  cass_future_free(query_future);
  
  return 0;
}



int
hss_mysql_increment_sqn (
  const char *imsi)
{
  int                                     status;
  MYSQL_RES                              *res;
  char                                    query[1000];

  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }

  if (imsi == NULL) {
    return EINVAL;
  }

  /*
   * + 32 = 2 ^ sizeof(IND) (see 3GPP TS. 33.102)
   */
  sprintf (query, "UPDATE `users` SET `sqn` = `sqn` + 32 WHERE `users`.`imsi`='%s'", imsi);
  FPRINTF_DEBUG ("Query: %s\n", query);

  pthread_mutex_lock(&db_desc->db_cs_mutex);

  if (mysql_query (db_desc->db_conn, query)) {
    pthread_mutex_unlock (&db_desc->db_cs_mutex);
    FPRINTF_ERROR ("Query execution failed: %s\n", mysql_error (db_desc->db_conn));
    return EINVAL;
  }

  /*
   * process each statement result
   */
  do {
    /*
     * did current statement return data?
     */
    res = mysql_store_result (db_desc->db_conn);

    if (res) {
      /*
       * yes; process rows and free the result set
       */
      mysql_free_result (res);
    } else {                    /* no result set or error */
      if (mysql_field_count (db_desc->db_conn) == 0) {
        FPRINTF_ERROR ( "%lld rows affected\n", mysql_affected_rows (db_desc->db_conn));
      } else {                  /* some error occurred */
        FPRINTF_ERROR ( "Could not retrieve result set\n");
        break;
      }
    }

    /*
     * more results? -1 = no, >0 = error, 0 = yes (keep looping)
     */
    if ((status = mysql_next_result (db_desc->db_conn)) > 0)
      FPRINTF_ERROR ( "Could not execute statement\n");
  } while (status == 0);

  pthread_mutex_unlock (&db_desc->db_cs_mutex);
  return 0;
}

int
hss_cassandra_increment_sqn (
  const char *imsi, uint8_t *sqn)
{
  int                                     status;
  char                                    query[1000];
  CassStatement                           *statement;
  CassFuture                              *query_future;
  CassError                               rc;
  uint64_t				  sqn_decimal = 0,value;

  sqn_decimal = ((uint64_t) sqn[0] << 40) | ((uint64_t) sqn[1] << 32) | ((uint64_t) sqn[2] << 24) | (sqn[3] << 16) | (sqn[4] << 8) | sqn[5];
  value = sqn_decimal+32;
  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }

  if (imsi == NULL) {
    return EINVAL;
  }

  
  /*
   * + 32 = 2 ^ sizeof(IND) (see 3GPP TS. 33.102)
   */
  sprintf (query, "UPDATE vhss.users_imsi SET sqn = %"PRIu64 " WHERE imsi='%s'", value, imsi);
  FPRINTF_DEBUG ("Query: %s\n", query);
  
  statement = cass_statement_new(query,0);
  pthread_mutex_lock(&db_desc->db_cs_mutex);
  query_future = cass_session_execute(db_desc->db_conn,statement);
  cass_statement_free(statement);
  rc = cass_future_error_code(query_future);
  if( rc != CASS_OK ){
        pthread_mutex_unlock(&db_desc->db_cs_mutex);
        const char* message;
        size_t message_length;
        cass_future_error_message(query_future,&message,&message_length);
        FPRINTF_ERROR( "Query execution failed: '%.*s'\n", (int)message_length, message);
        return EINVAL;
  }

  #if 0
  /*
   * process each statement result
   */
  do {
    /*
     * did current statement return data?
     */
    res = mysql_store_result (db_desc->db_conn);

    if (res) {
      /*
       * yes; process rows and free the result set
       */
      mysql_free_result (res);
    } else {                    /* no result set or error */
      if (mysql_field_count (db_desc->db_conn) == 0) {
        FPRINTF_ERROR ( "%lld rows affected\n", mysql_affected_rows (db_desc->db_conn));
      } else {                  /* some error occurred */
        FPRINTF_ERROR ( "Could not retrieve result set\n");
        break;
      }
    }

    /*
     * more results? -1 = no, >0 = error, 0 = yes (keep looping)
     */
    if ((status = mysql_next_result (db_desc->db_conn)) > 0)
      FPRINTF_ERROR ( "Could not execute statement\n");
  } while (status == 0);

  #endif

  pthread_mutex_unlock (&db_desc->db_cs_mutex);
  cass_future_free(query_future);
  return 0;
}

int
hss_mysql_auth_info (
  mysql_auth_info_req_t * auth_info_req,
  mysql_auth_info_resp_t * auth_info_resp)
{
  int                                     ret = 0;
  MYSQL_RES                              *res;
  MYSQL_ROW                               row;
  char                                    query[1000];

  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }

  if ((auth_info_req == NULL) || (auth_info_resp == NULL)) {
    return EINVAL;
  }

  sprintf (query, "SELECT `key`,`sqn`,`rand`,`OPc` FROM `users` WHERE `users`.`imsi`='%s' ", auth_info_req->imsi);
  FPRINTF_DEBUG ("Query: %s\n", query);
  pthread_mutex_lock (&db_desc->db_cs_mutex);

  if (mysql_query (db_desc->db_conn, query)) {
    pthread_mutex_unlock (&db_desc->db_cs_mutex);
    FPRINTF_ERROR ("Query execution failed: %s\n", mysql_error (db_desc->db_conn));
    return EINVAL;
  }

  res = mysql_store_result (db_desc->db_conn);
  pthread_mutex_unlock (&db_desc->db_cs_mutex);

  if ( res == NULL )
    return EINVAL;

  ret = 0;

  if ((row = mysql_fetch_row (res)) != NULL) {
    if (row[0] == NULL || row[1] == NULL || row[2] == NULL || row[3] == NULL) {
      ret = EINVAL;
    }

    if (row[0] != NULL) {
      print_buffer ("Key: ", (uint8_t *) row[0], KEY_LENGTH);
      memcpy (auth_info_resp->key, row[0], KEY_LENGTH);
    }

    if (row[1] != NULL) {
      uint64_t                                sqn = 0;

      sqn = atoll (row[1]);
      printf ("Received SQN %s converted to %" PRIu64 "\n", row[1], sqn);
      auth_info_resp->sqn[0] = (sqn & (255UL << 40)) >> 40;
      auth_info_resp->sqn[1] = (sqn & (255UL << 32)) >> 32;
      auth_info_resp->sqn[2] = (sqn & (255UL << 24)) >> 24;
      auth_info_resp->sqn[3] = (sqn & (255UL << 16)) >> 16;
      auth_info_resp->sqn[4] = (sqn & (255UL << 8)) >> 8;
      auth_info_resp->sqn[5] = (sqn & 0xFF);
      print_buffer ("SQN: ", auth_info_resp->sqn, SQN_LENGTH);
    }

    if (row[2] != NULL) {
      print_buffer ("RAND: ", (uint8_t *) row[2], RAND_LENGTH);
      memcpy (auth_info_resp->rand, row[2], RAND_LENGTH);
    }

    if (row[3] != NULL) {
      print_buffer ("OPc: ", (uint8_t *) row[3], KEY_LENGTH);
      memcpy (auth_info_resp->opc, row[3], KEY_LENGTH);
    }

  }

  mysql_free_result (res);
  return ret;
}

int
hss_cassandra_auth_info (
  cassandra_auth_info_req_t * auth_info_req,
  cassandra_auth_info_resp_t * auth_info_resp)
{
  int                                     ret = 0;
  const CassRow                          *row;
  char                                    query[1000];
  CassStatement 			  *statement;
  CassFuture				  *query_future;
  CassError                               rc;
  const CassResult                        *result = NULL;
  CassValue				  *cass_sqn_value,*cass_key_value,*cass_rand_value,*cass_opc_value;
  cass_int64_t				  sqn;
  const char 				  *key,*rand,*opc;
  size_t 				  key_length,rand_length,opc_length;
  

  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }

  if ((auth_info_req == NULL) || (auth_info_resp == NULL)) {
    return EINVAL;
  }

  sprintf (query, "SELECT key,sqn,rand,OPc FROM vhss.users_imsi WHERE imsi='%s' ", auth_info_req->imsi);
  FPRINTF_DEBUG ("Query: %s\n", query);

  statement = cass_statement_new(query,0);
  pthread_mutex_lock (&db_desc->db_cs_mutex);
  query_future = cass_session_execute(db_desc->db_conn,statement);
  cass_statement_free(statement);
  rc = cass_future_error_code(query_future);
  if ( rc != CASS_OK) {
	pthread_mutex_unlock(&db_desc->db_cs_mutex);
    	const char* message;
    	size_t message_length;
    	cass_future_error_message(query_future,&message,&message_length);
    	FPRINTF_ERROR( "Query execution failed: '%.*s'\n", (int)message_length, message);
	return EINVAL;
  }

  result = cass_future_get_result(query_future);
  /* If there was an error then the result won't be available */
  if (result == NULL) {
     /* Handle error */
     FPRINTF_DEBUG("Select query result is NULL\n");
     pthread_mutex_unlock(&db_desc->db_cs_mutex);
     cass_future_free(query_future);
     return EINVAL;
  }
  pthread_mutex_unlock(&db_desc->db_cs_mutex);
  cass_future_free(query_future);
  
  row = cass_result_first_row(result);
  cass_key_value = cass_row_get_column_by_name(row,"key");
  cass_sqn_value = cass_row_get_column_by_name(row,"sqn");
  cass_rand_value = cass_row_get_column_by_name(row,"rand");
  cass_opc_value = cass_row_get_column_by_name(row,"opc");
  
  cass_result_free(result);


  if( cass_key_value == NULL || cass_sqn_value == NULL || cass_rand_value == NULL || cass_opc_value == NULL ){
	ret = EINVAL;
  }

  if( cass_key_value != NULL ){
	cass_value_get_string(cass_key_value,&key,&key_length);
	if(key != NULL){
		FPRINTF_DEBUG("key value is '%s'\n",key);
		memcpy (auth_info_resp->key, (uint8_t *)key, KEY_LENGTH);
	}
  }
  if( cass_sqn_value != NULL ){
	cass_value_get_int64(cass_sqn_value,&sqn);
	if(sqn != NULL ){
		FPRINTF_DEBUG("retrieved sqn value is %" PRIu64 "\n",sqn);
		auth_info_resp->sqn[0] = (sqn & (255UL << 40)) >> 40;
		auth_info_resp->sqn[1] = (sqn & (255UL << 32)) >> 32;
		auth_info_resp->sqn[2] = (sqn & (255UL << 24)) >> 24;
		auth_info_resp->sqn[3] = (sqn & (255UL << 16)) >> 16;
		auth_info_resp->sqn[4] = (sqn & (255UL << 8)) >> 8;
		auth_info_resp->sqn[5] = (sqn & 0xFF);
	}
  }
  if(cass_rand_value != NULL ){
	cass_value_get_string(cass_rand_value,&rand,&rand_length);
	if(rand != NULL){
		FPRINTF_DEBUG("Rand value is %s\n",rand);
		memcpy(auth_info_resp->rand,(uint8_t *)rand, RAND_LENGTH);
	}
  }
 if(cass_opc_value != NULL ){
	cass_value_get_string(cass_opc_value,&opc,&opc_length);
	if(opc != NULL){
		FPRINTF_DEBUG("opc value is %s\n",opc);
		memcpy(auth_info_resp->opc,(uint8_t *)opc, KEY_LENGTH);
	}
  }

  FPRINTF_DEBUG("At the end of function hss_cassandra_auth_info\n");
  return ret;
}

#if 0
int
hss_mysql_check_opc_keys (
  const uint8_t const opP[16])
{
  int                                     ret = 0;
  MYSQL_RES                              *res = NULL;
  MYSQL_RES                              *res2 = NULL;
  MYSQL_ROW                               row;
  char                                    query[1000];
  char                                    update[1000];
  uint8_t                                 k[16];
  uint8_t                                 opc[16];
  int                                     update_length = 0;
  int                                     status = 0;
  int                                     i;

  FPRINTF_DEBUG("Inside hss_mysql_check_opc_keys\n"); 
  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }

  sprintf (query, "SELECT `imsi`,`key`,`OPc` FROM `users` ");
  FPRINTF_DEBUG ("Query: %s\n", query);
  pthread_mutex_lock (&db_desc->db_cs_mutex);

  if (mysql_query (db_desc->db_conn, query)) {
    pthread_mutex_unlock (&db_desc->db_cs_mutex);
    FPRINTF_ERROR ( "Query execution failed: %s\n", mysql_error (db_desc->db_conn));
    mysql_thread_end ();
    return EINVAL;
  }

  res = mysql_store_result (db_desc->db_conn);
  pthread_mutex_unlock (&db_desc->db_cs_mutex);

  while ((row = mysql_fetch_row (res))) {
    if (row[0] == NULL || row[1] == NULL) {
      FPRINTF_ERROR ( "Query execution failed: %s\n", mysql_error (db_desc->db_conn));
      ret = EINVAL;
    } else {
      if (row[0] != NULL) {
        printf ("IMSI: %s", (uint8_t *) row[0]);
      }

      if (row[1] != NULL) {
        print_buffer ("Key: ", (uint8_t *) row[1], KEY_LENGTH);
        memcpy (k, row[1], KEY_LENGTH);
      }
      //if (row[3] != NULL)
      {
        print_buffer ("OPc: ", (uint8_t *) row[2], KEY_LENGTH);
        //} else {
        ComputeOPc (k, opP, opc);
	update_length = sprintf (update, "UPDATE `users` SET `OPc`=UNHEX('");
        for (i = 0; i < KEY_LENGTH; i++) {
          update_length += sprintf (&update[update_length], "%02x", opc[i]);
        }

	update_length += sprintf (&update[update_length], "') WHERE `users`.`imsi`='%s'", (uint8_t *) row[0]);
        FPRINTF_DEBUG ("Query: %s\n", update);

        if (mysql_query (db_desc->db_conn, update)) {
          FPRINTF_ERROR ( "Query execution failed: %s\n", mysql_error (db_desc->db_conn));
        } else {
          printf ("IMSI %s Updated OPc ", (uint8_t *) row[0]);

          for (i = 0; i < KEY_LENGTH; i++) {
            printf ("%02x", (uint8_t) (row[2][i]));
          }

          printf (" -> ");

          for (i = 0; i < KEY_LENGTH; i++) {
            printf ("%02x", opc[i]);
          }

          printf ("\n");

          /*
           * process each statement result
           */
          do {
            /*
             * did current statement return data?
             */
            res2 = mysql_store_result (db_desc->db_conn);

            if (res2) {
              /*
               * yes; process rows and free the result set
               */
              mysql_free_result (res2);
            } else {            /* no result set or error */
              if (mysql_field_count (db_desc->db_conn) == 0) {
                FPRINTF_ERROR ( "%lld rows affected\n", mysql_affected_rows (db_desc->db_conn));
              } else {          /* some error occurred */
                FPRINTF_ERROR ( "Could not retrieve result set\n");
                break;
              }
            }

            /*
             * more results? -1 = no, >0 = error, 0 = yes (keep looping)
             */
            if ((status = mysql_next_result (db_desc->db_conn)) > 0)
              FPRINTF_ERROR ( "Could not execute statement\n");
          } while (status == 0);
        }
      }
    }
  }

  mysql_free_result (res);
  mysql_thread_end ();
  FPRINTF_DEBUG("At the end of function hss_mysql_check_opc_keys\n");
  return ret;
}
#endif


int
hss_cassandra_check_opc_keys (
  const uint8_t const opP[16])
{
  int                                     ret = 0;
  const CassResult 			  *result = NULL;
  CassStatement			          *statement;
  CassFuture				  *query_future;
  CassError				  rc;
  char					  update[1000];
  uint8_t                                 k[16];
  uint8_t                                 opc[16];
  int                                     update_length = 0;
  int                                     i;
  const CassRow 			  *row;
  const CassValue			  *imsi_value,*key_value,*OPc_value;
  const char				  *imsi, *opc_char,*key;
  size_t 				  imsi_length,key_length,opc_length;

  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }
  statement = cass_statement_new("SELECT imsi,key,OPc from vhss.users_imsi",0);
  pthread_mutex_lock (&db_desc->db_cs_mutex);
  query_future = cass_session_execute(db_desc->db_conn,statement);
  cass_statement_free(statement);
  rc = cass_future_error_code(query_future);
  if(rc != CASS_OK){
    pthread_mutex_unlock(&db_desc->db_cs_mutex);
    const char* message;
    size_t message_length;
    cass_future_error_message(query_future,&message,&message_length);
    FPRINTF_ERROR( "Query execution failed: '%.*s'\n", (int)message_length, message);
    return EINVAL;
  }

  result = cass_future_get_result(query_future);
  /* If there was an error then the result won't be available */
  if (result == NULL) {
     /* Handle error */
     FPRINTF_DEBUG("Select query result is NULL\n");
     pthread_mutex_unlock(&db_desc->db_cs_mutex);
     cass_future_free(query_future);
     return EINVAL;
  }
  pthread_mutex_unlock(&db_desc->db_cs_mutex);

  cass_future_free(query_future);
  CassIterator* iterator = cass_iterator_from_result(result);
  while(cass_iterator_next(iterator)){

    row = cass_iterator_get_row(iterator);
    imsi_value = cass_row_get_column_by_name(row,"imsi");
    key_value = cass_row_get_column_by_name(row,"key");
    OPc_value = cass_row_get_column_by_name(row,"opc");
    if( imsi_value == NULL || key_value == NULL){
       FPRINTF_ERROR("Imsi value or key_value is NULL\n");
       ret=EINVAL;
    }
    else{
    	if( imsi_value != NULL){
    		cass_value_get_string(imsi_value,&imsi,&imsi_length);
    		FPRINTF_DEBUG("IMSI value is '%.*s'\n",(int)imsi_length,imsi);
    	}
    	if( key_value != NULL){
        	cass_value_get_string(key_value,&key,&key_length);
		if(key != NULL){
        		FPRINTF_DEBUG("key value is '%s'\n",key);
			memcpy (k, (uint8_t *)key, KEY_LENGTH);
		}
		else{
		FPRINTF_DEBUG("Key value is NULL\n");
		}
    	}
	//if( OPc_value != NULL)
	//{
       		cass_value_get_string(OPc_value,&opc_char,&opc_length);
		//if(opc_char != NULL){
			//FPRINTF_DEBUG("OPc value is '%.*s'\n",(int)opc_length,opc_char);
       			FPRINTF_DEBUG("OPc value is %s\n",opc_char);
			ComputeOPc (k, opP, opc);
			update_length = sprintf (update, "UPDATE vhss.users_imsi SET OPc='");
			if( update_length < 0 ){
				FPRINTF_DEBUG("Cannot copy the query into c-string!\n");
				goto out;
			}

        		for (i = 0; i < KEY_LENGTH; i++) {
          			update_length += sprintf (&update[update_length], "%02x", opc[i]);
        		}

        		update_length += sprintf (&update[update_length], "' WHERE imsi='%s'",imsi);
        		FPRINTF_DEBUG ("Query: %s\n", update);
                	CassStatement* statement = cass_statement_new(update,0);
			query_future = cass_session_execute(db_desc->db_conn,statement);
			cass_statement_free(statement);
			rc = cass_future_error_code(query_future);
			if ( rc != CASS_OK) {
				FPRINTF_ERROR ( "Query execution failed: %s\n");
			}
			else {
          			printf ("IMSI %s Updated OPc \n",imsi);
			}
		//}
		//else{
		//	FPRINTF_DEBUG("opc_char is NULL\n");
		//}
	//}
    }
    }
out:
    cass_iterator_free(iterator);
    cass_result_free(result);
    return ret;
}

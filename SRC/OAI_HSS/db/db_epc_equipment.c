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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <error.h>

#include <mysql/mysql.h>

#include "hss_config.h"
#include "db_proto.h"
#include "log.h"

int
hss_mysql_query_mmeidentity (
  const int id_mme_identity,
  mysql_mme_identity_t * mme_identity_p)
{
  MYSQL_RES                              *res;
  MYSQL_ROW                               row;
  char                                    query[1000];

  if ((db_desc->db_conn == NULL) || (mme_identity_p == NULL)) {
    return EINVAL;
  }

  memset (mme_identity_p, 0, sizeof (mysql_mme_identity_t));
  sprintf (query, "SELECT mmehost,mmerealm FROM mmeidentity WHERE " "mmeidentity.idmmeidentity='%d' ", id_mme_identity);
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
    if (row[0] != NULL) {
      memcpy (mme_identity_p->mme_host, row[0], strlen (row[0]));
    } else {
      mme_identity_p->mme_host[0] = '\0';
    }

    if (row[1] != NULL) {
      memcpy (mme_identity_p->mme_realm, row[1], strlen (row[1]));
    } else {
      mme_identity_p->mme_realm[0] = '\0';
    }

    mysql_free_result (res);
    return 0;
  }

  mysql_free_result (res);
  return EINVAL;
}

int
hss_cassandra_query_mmeidentity (
  const int id_mme_identity,
  cassandra_mme_identity_t * mme_identity_p)
{
  const CassResult			 *result=NULL;
  const CassRow	                         *row;
  char                                   query[1000];
  CassStatement 			 *statement;
  CassFuture				 *query_future;
  CassError                              rc;
  CassValue				 *cass_mmehost_value,*cass_mmerealm_value;
  const char 				 *mmehost = NULL, *mmerealm = NULL;
  size_t 				 mmehost_length,mmerealm_length;

  if ((db_desc->db_conn == NULL) || (mme_identity_p == NULL)) {
    return EINVAL;
  }
  memset (mme_identity_p, 0, sizeof (cassandra_mme_identity_t)); 
  sprintf (query, "SELECT mmehost,mmerealm FROM vhss.mmeidentity WHERE idmmeidentity=%d ", id_mme_identity);
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
  pthread_mutex_unlock (&db_desc->db_cs_mutex);
  
/* If there was an error then the result won't be available */
  if (result == NULL) {
     /* Handle error */
     FPRINTF_DEBUG("Select query result is NULL\n");
     cass_future_free(query_future);
     return EINVAL;
  }
  cass_future_free(query_future);

  
  row = cass_result_first_row(result);
  cass_result_free(result);
 
  if( row == NULL ){
	return EINVAL;
  }
  cass_mmehost_value = cass_row_get_column_by_name(row,"mmehost");
  cass_mmerealm_value = cass_row_get_column_by_name(row,"mmerealm");
  
  if( cass_mmehost_value != NULL ){
	cass_value_get_string(cass_mmehost_value,&mmehost,&mmehost_length);
	if( mmehost != NULL ){
		memcpy(mme_identity_p->mme_host,mmehost,strlen(mmehost));
		mme_identity_p->mme_host[strlen(mmehost)] = '\0';
	}
	else{
		mme_identity_p->mme_host[0] = '\0';
	}
  }
  if( cass_mmerealm_value != NULL){
	cass_value_get_string(cass_mmerealm_value,&mmerealm,&mmerealm_length);
	if( mmerealm != NULL ){
		memcpy(mme_identity_p->mme_realm,mmerealm,strlen(mmerealm));
		mme_identity_p->mme_realm[strlen(mmerealm)] = '\0';
	}
	else{
		mme_identity_p->mme_realm[0] = '\0';
	}
  }
  
  return 0;

}




int
hss_mysql_check_epc_equipment (
  mysql_mme_identity_t * mme_identity_p)
{
  MYSQL_RES                              *res;
  MYSQL_ROW                               row;
  char                                    query[1000];

  if ((db_desc->db_conn == NULL) || (mme_identity_p == NULL)) {
    return EINVAL;
  }

  sprintf (query, "SELECT idmmeidentity FROM mmeidentity WHERE mmeidentity.mmehost='%s' ", mme_identity_p->mme_host);
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
hss_cassandra_check_epc_equipment (
  cassandra_mme_identity_t * mme_identity_p)
{
  const CassResult			 *result=NULL;
  const CassRow	                         *row;
  char                                    query[1000];
  CassStatement 			 *statement;
  CassFuture				 *query_future;
  CassError                               rc;
  CassValue				 *cass_idmmeidentity_value;
  int 					 *idmmeidentity;

  if ((db_desc->db_conn == NULL) || (mme_identity_p == NULL)) {
    return EINVAL;
  }

  sprintf (query, "SELECT idmmeidentity FROM vhss.mmeidentity_host WHERE mmehost='%s' ", mme_identity_p->mme_host);
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
     pthread_mutex_unlock(&db_desc->db_cs_mutex);
     FPRINTF_DEBUG("Select query result is NULL\n");
     cass_future_free(query_future);
     return EINVAL;
  }
  cass_future_free(query_future);
  row = cass_result_first_row(result);

  if( row == NULL ){
	cass_result_free(result);
	pthread_mutex_unlock (&db_desc->db_cs_mutex);
	return EINVAL;
  }
  cass_idmmeidentity_value = cass_row_get_column_by_name(row,"idmmeidentity");
  cass_result_free(result);
  if( cass_idmmeidentity_value != NULL ){
	cass_value_get_int32(cass_idmmeidentity_value,&idmmeidentity);
	if(idmmeidentity != NULL ){
  		pthread_mutex_unlock (&db_desc->db_cs_mutex);
		return idmmeidentity;
	}
  }
  
  pthread_mutex_unlock (&db_desc->db_cs_mutex);
  return EINVAL;
}

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
#include <stdint.h>
#include "hss_config.h"
#include "db_proto.h"
#include "log.h"

int
hss_mysql_query_pdns (
  const char *imsi,
  mysql_pdn_t ** pdns_p,
  uint8_t * nb_pdns)
{
  int                                     ret;
  MYSQL_RES                              *res = NULL;
  MYSQL_ROW                               row;
  char                                    query[1000];
  mysql_pdn_t                            *pdn_array = NULL;

  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }

  if (nb_pdns == NULL || pdns_p == NULL) {
    return EINVAL;
  }

  sprintf (query, "SELECT * FROM `pdn` WHERE " "`pdn`.`users_imsi`=%s LIMIT 10; ", imsi);
  FPRINTF_DEBUG ("Query: %s\n", query);
  pthread_mutex_lock (&db_desc->db_cs_mutex);

  if (mysql_query (db_desc->db_conn, query)) {
    pthread_mutex_unlock (&db_desc->db_cs_mutex);
    fprintf (stderr, "Query execution failed: %s\n", mysql_error (db_desc->db_conn));
    return EINVAL;
  }

  res = mysql_store_result (db_desc->db_conn);
  pthread_mutex_unlock (&db_desc->db_cs_mutex);

  if ( res == NULL )
    return EINVAL;

  *nb_pdns = 0;

  while ((row = mysql_fetch_row (res)) != NULL) {
    mysql_pdn_t                            *pdn_elm;    /* Local PDN element in array */
    unsigned long                          *lengths;

    lengths = mysql_fetch_lengths (res);
    *nb_pdns += 1;

    if (*nb_pdns == 1) {
      /*
       * First row, must malloc
       */
      pdn_array = malloc (sizeof (mysql_pdn_t));
    } else {
      pdn_array = realloc (pdn_array, *nb_pdns * sizeof (mysql_pdn_t));
    }

    if (pdn_array == NULL) {
      /*
       * Error on malloc
       */
      ret = ENOMEM;
      goto err;
    }

    pdn_elm = &pdn_array[*nb_pdns - 1];
    /*
     * Copying the APN
     */
    memset (pdn_elm, 0, sizeof (mysql_pdn_t));
    memcpy (pdn_elm->apn, row[1], lengths[1]);

    /*
     * PDN Type + PDN address
     */
    if (strcmp (row[2], "IPv6") == 0) {
      pdn_elm->pdn_type = IPV6;
      memcpy (pdn_elm->pdn_address.ipv6_address, row[4], lengths[4]);
      pdn_elm->pdn_address.ipv6_address[lengths[4]] = '\0';
    } else if (strcmp (row[2], "IPv4v6") == 0) {
      pdn_elm->pdn_type = IPV4V6;
      memcpy (pdn_elm->pdn_address.ipv4_address, row[3], lengths[3]);
      pdn_elm->pdn_address.ipv4_address[lengths[3]] = '\0';
      memcpy (pdn_elm->pdn_address.ipv6_address, row[4], lengths[4]);
      pdn_elm->pdn_address.ipv6_address[lengths[4]] = '\0';
    } else if (strcmp (row[2], "IPv4_or_IPv6") == 0) {
      pdn_elm->pdn_type = IPV4_OR_IPV6;
      memcpy (pdn_elm->pdn_address.ipv4_address, row[3], lengths[3]);
      pdn_elm->pdn_address.ipv4_address[lengths[3]] = '\0';
      memcpy (pdn_elm->pdn_address.ipv6_address, row[4], lengths[4]);
      pdn_elm->pdn_address.ipv6_address[lengths[4]] = '\0';
    } else {
      pdn_elm->pdn_type = IPV4;
      memcpy (pdn_elm->pdn_address.ipv4_address, row[3], lengths[3]);
      pdn_elm->pdn_address.ipv4_address[lengths[3]] = '\0';
    }

    pdn_elm->aggr_ul = atoi (row[5]);
    pdn_elm->aggr_dl = atoi (row[6]);
    pdn_elm->qci = atoi (row[9]);
    pdn_elm->priority_level = atoi (row[10]);

    if (strcmp (row[11], "ENABLED") == 0) {
      pdn_elm->pre_emp_cap = 0;
    } else {
      pdn_elm->pre_emp_cap = 1;
    }

    if (strcmp (row[12], "DISABLED") == 0) {
      pdn_elm->pre_emp_vul = 1;
    } else {
      pdn_elm->pre_emp_vul = 0;
    }
  }

  mysql_free_result (res);

  /*
   * We did not find any APN for the requested IMSI
   */
  if (*nb_pdns == 0) {
    return EINVAL;
  } else {
    *pdns_p = pdn_array;
    return 0;
  }

err:

  if (pdn_array) {
    free (pdn_array);
  }

  pdn_array = NULL;
  *pdns_p = pdn_array;
  *nb_pdns = 0;
  mysql_free_result (res);
  return ret;
}

void get_mcc_mnc_from_imsi(char *mcc, char *mnc, char * imsi)
{
        int length=strlen(imsi);
	memcpy(mcc,imsi,3);
	memcpy(mnc,&imsi[3],3);
}

int
hss_cassandra_query_pdns (
  const char *imsi,
  cassandra_pdn_t *pdns_p)
{
  int                                     ret=0;
  const CassRow				  *row;
  const CassResult			  *result = NULL;
  char                                    query[1000];
  CassStatement				  *statement;
  CassFuture				  *query_future;
  CassError				  rc;
  const CassValue			  *cass_apn_value, *cass_pdn_type_value, *cass_pdn_ipv6_value,
					  *cass_pdn_ipv4_value, *cass_aggr_ambr_ul_value, *cass_aggr_ambr_dl_value,
					  *cass_qci_value, *cass_priority_level_value, *cass_pre_emp_cap_value,
					  *cass_pre_emp_vul_value; 
  const char 				  *apn, *pdn_type, *pre_emp_cap, *pre_emp_vul;
  CassInet		                  pdn_ipv6_addr, pdn_ipv4_addr;
  size_t			 	  apn_length, pdn_type_length, pre_emp_cap_length, pre_emp_vul_length, pdn_ipv6_len, pdn_ipv4_len;
  cass_int32_t				  aggr_ambr_ul = 0, aggr_ambr_dl = 0, qci = 0, priority_level = 0;
  char                                    mcc[4], mnc[4], mcc_label[7], mnc_label[7], apn_oi[20];


  printf("Inside hss_cassandra_query_pdns \n");
  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }

  sprintf (query, "SELECT apn, pdn_type, pdn_ipv6, pdn_ipv4, aggregate_ambr_ul,"
                   "aggregate_ambr_dl, qci, priority_level, pre_emp_cap,"
                   "pre_emp_vul FROM vhss.users_imsi WHERE imsi='%s'; ", imsi);
  statement = cass_statement_new(query,0);
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
  
  if( row != NULL ){
	cass_apn_value = cass_row_get_column_by_name(row,"apn");
	cass_pdn_type_value = cass_row_get_column_by_name(row,"pdn_type");
	cass_pdn_ipv6_value = cass_row_get_column_by_name(row,"pdn_ipv6");
	cass_pdn_ipv4_value = cass_row_get_column_by_name(row,"pdn_ipv4");
	cass_aggr_ambr_ul_value = cass_row_get_column_by_name(row,"aggregate_ambr_ul");
	cass_aggr_ambr_dl_value = cass_row_get_column_by_name(row,"aggregate_ambr_dl");
	cass_qci_value = cass_row_get_column_by_name(row,"qci");
	cass_priority_level_value = cass_row_get_column_by_name(row,"priority_level");
	cass_pre_emp_cap_value = cass_row_get_column_by_name(row,"pre_emp_cap");
	cass_pre_emp_vul_value = cass_row_get_column_by_name(row,"pre_emp_vul");

	memset(mcc, 0, sizeof(mcc));
	memset(mnc, 0, sizeof(mnc));
 	if( cass_apn_value != NULL ){ 
  		cass_value_get_string(cass_apn_value, &apn, &apn_length);
		get_mcc_mnc_from_imsi(&mcc,&mnc,imsi);
    			
                sprintf(apn_oi,"mnc%s.mcc%s.gprs",mnc,mcc);
		//sprintf(apn,apn_oi);
        }
	if( cass_pdn_type_value != NULL )
	{
		cass_value_get_string(cass_pdn_type_value, &pdn_type, &pdn_type_length);
	}
	if( cass_pdn_ipv6_value != NULL )
	{
  		cass_value_get_inet (cass_pdn_ipv6_value, &pdn_ipv6_addr);
  	}
	if( cass_pdn_ipv4_value != NULL )
	{
		cass_value_get_inet (cass_pdn_ipv4_value, &pdn_ipv4_addr);
  	}
	if( cass_aggr_ambr_ul_value != NULL )
	{
		cass_value_get_int32(cass_aggr_ambr_ul_value, &aggr_ambr_ul);
	}
	if( cass_aggr_ambr_dl_value != NULL )
	{
  		cass_value_get_int32(cass_aggr_ambr_dl_value, &aggr_ambr_dl);
	}
	if( cass_qci_value != NULL )
	{
  		cass_value_get_int32(cass_qci_value, &qci);
	}
	if( cass_priority_level_value != NULL )
	{
  		cass_value_get_int32(cass_priority_level_value, &priority_level);
	}
	if( cass_pre_emp_cap_value != NULL )
	{
  		cass_value_get_string(cass_pre_emp_cap_value, &pre_emp_cap, &pre_emp_cap_length);
	}
	if( cass_pre_emp_vul_value != NULL )
	{
  		cass_value_get_string(cass_pre_emp_vul_value, &pre_emp_vul, &pre_emp_vul_length);
	}
 
    	memcpy (pdns_p->apn, apn, strlen(apn));
  	/*
         * We did not find any APN for the requested IMSI
         */  

   	if( pdns_p->apn == NULL ){
		return EINVAL;
    	}
    
    	/*
     	 * PDN Type + PDN address
         */
   	printf("pdn_type --> %s\n",pdn_type); 
	if( strcmp (pdn_type, "IPv6") == 0 ) {
      		pdns_p->pdn_type = IPV6;
		sprintf(pdns_p->pdn_address.ipv6_address, "%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X", pdn_ipv6_addr.address[0], pdn_ipv6_addr.address[1], pdn_ipv6_addr.address[2], pdn_ipv6_addr.address[3], pdn_ipv6_addr.address[4], pdn_ipv6_addr.address[5], pdn_ipv6_addr.address[6], pdn_ipv6_addr.address[7], pdn_ipv6_addr.address[8], pdn_ipv6_addr.address[9], pdn_ipv6_addr.address[10], pdn_ipv6_addr.address[11], pdn_ipv6_addr.address[12], pdn_ipv6_addr.address[13], pdn_ipv6_addr.address[14], pdn_ipv6_addr.address[15]);
    	} else if( strcmp (pdn_type, "IPv4v6") == 0 ) {
       		pdns_p->pdn_type = IPV4V6;
		sprintf(pdns_p->pdn_address.ipv4_address, "%u.%u.%u.%u", pdn_ipv4_addr.address[0],pdn_ipv4_addr.address[1],pdn_ipv4_addr.address[2],pdn_ipv4_addr.address[3]);
		sprintf(pdns_p->pdn_address.ipv6_address, "%X%X:%X%X:%X%X:%X%X:%X%X:%X%X:%X%X:%X%X", pdn_ipv6_addr.address[0], pdn_ipv6_addr.address[1], pdn_ipv6_addr.address[2], pdn_ipv6_addr.address[3], pdn_ipv6_addr.address[4], pdn_ipv6_addr.address[5], pdn_ipv6_addr.address[6], pdn_ipv6_addr.address[7], pdn_ipv6_addr.address[8], pdn_ipv6_addr.address[9], pdn_ipv6_addr.address[10], pdn_ipv6_addr.address[11], pdn_ipv6_addr.address[12], pdn_ipv6_addr.address[13], pdn_ipv6_addr.address[14], pdn_ipv6_addr.address[15]);
    	} else if( strcmp (pdn_type, "IPv4_or_IPv6") == 0 ) {
      		pdns_p->pdn_type = IPV4_OR_IPV6;
      		sprintf(pdns_p->pdn_address.ipv4_address, "%u.%u.%u.%u", pdn_ipv4_addr.address[0],pdn_ipv4_addr.address[1],pdn_ipv4_addr.address[2],pdn_ipv4_addr.address[3]);
                sprintf(pdns_p->pdn_address.ipv6_address, "%X%X:%X%X:%X%X:%X%X:%X%X:%X%X:%X%X:%X%X", pdn_ipv6_addr.address[0], pdn_ipv6_addr.address[1], pdn_ipv6_addr.address[2], pdn_ipv6_addr.address[3], pdn_ipv6_addr.address[4], pdn_ipv6_addr.address[5], pdn_ipv6_addr.address[6], pdn_ipv6_addr.address[7], pdn_ipv6_addr.address[8], pdn_ipv6_addr.address[9], pdn_ipv6_addr.address[10], pdn_ipv6_addr.address[11], pdn_ipv6_addr.address[12], pdn_ipv6_addr.address[13], pdn_ipv6_addr.address[14], pdn_ipv6_addr.address[15]);
    	} else {
      		pdns_p->pdn_type = IPV4;
		printf("pdn_ipv4_addr length = %d \n", pdn_ipv4_addr.address_length);
		sprintf(pdns_p->pdn_address.ipv4_address, "%u.%u.%u.%u", pdn_ipv4_addr.address[0],pdn_ipv4_addr.address[1],pdn_ipv4_addr.address[2],pdn_ipv4_addr.address[3]);
    	}

    	if( aggr_ambr_ul != NULL )
		pdns_p->aggr_ul = aggr_ambr_ul;
    	if( aggr_ambr_dl != NULL )
		pdns_p->aggr_dl = aggr_ambr_dl;
    	if( qci != NULL )
		pdns_p->qci = qci;
    	if( priority_level != NULL )	
		pdns_p->priority_level = priority_level;
	if( pre_emp_cap != NULL ){
		if (strcmp (pre_emp_cap, "ENABLED") == 0) {
      			pdns_p->pre_emp_cap = 0;
    		} else {
      			pdns_p->pre_emp_cap = 1;
    		}
	}
	if( pre_emp_vul != NULL ){
    		if (strcmp (pre_emp_vul, "DISABLED") == 0) {
      			pdns_p->pre_emp_vul = 1;
    		} else {
      			pdns_p->pre_emp_vul = 0;
    		}
	}
  	cass_result_free(result);
 	return 0;
  }
	
  	cass_result_free(result);
  return EINVAL;
}


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


#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <mysql/mysql.h>
#include <cassandra.h>
#include <netinet/in.h> /* To provide internet addresses strings helpers */

#ifndef DB_PROTO_H_
#define DB_PROTO_H_

typedef struct {
  /* The mysql reference connector object */
  MYSQL *db_conn;
  char  *server;
  char  *user;
  char  *password;
  char  *database;

  pthread_mutex_t db_cs_mutex;
} database_t;

typedef struct {
  /*Cassandra session object*/
  CassSession* db_conn;
  CassCluster* db_cluster;
  CassFuture* db_connect_future;
  char *server;
  char *user;
  char *password;
  char *database;
 
  pthread_mutex_t db_cs_mutex;
} db_cassandra_t;

extern db_cassandra_t *db_desc;

typedef uint32_t pre_emp_vul_t;
typedef uint32_t pre_emp_cap_t;
typedef uint8_t  access_restriction_t;
typedef uint32_t ambr_t;
typedef uint8_t  qci_t;
typedef uint8_t  prio_level_t;
typedef uint32_t rau_tau_t;

#define IMSI_LENGTH_MAX (15)
#define IMEI_LENGTH_MAX (15)

typedef struct {
  char imsi[IMSI_LENGTH_MAX + 1];
} mysql_auth_info_req_t;

typedef struct {
  char imsi[IMSI_LENGTH_MAX + 1];
} cassandra_auth_info_req_t;

/* Expressed in bytes */
#define KEY_LENGTH  (16)
#define SQN_LENGTH  (6)
#define RAND_LENGTH (16)

typedef struct mysql_auth_info_resp_s{
  uint8_t key[KEY_LENGTH];
  uint8_t sqn[SQN_LENGTH];
  /* RAND should not be here... */
  uint8_t rand[RAND_LENGTH];
  uint8_t opc[KEY_LENGTH];
} mysql_auth_info_resp_t;

typedef struct cassandra_auth_info_resp_s{
  uint8_t key[KEY_LENGTH];
  uint8_t sqn[SQN_LENGTH];
  /* RAND should not be here... */
  uint8_t rand[RAND_LENGTH];
  uint8_t opc[KEY_LENGTH];
} cassandra_auth_info_resp_t;

typedef struct mysql_opc_push_s{
  char imsi[IMSI_LENGTH_MAX + 1];
  /* New computed SQN that will be used on next auth info req */
  uint8_t sqn[SQN_LENGTH];
} mysql_opc_push_t;

typedef struct mysql_sqn_push_s{
  char imsi[IMSI_LENGTH_MAX + 1];
  /* New computed SQN that will be used on next auth info req */
  uint8_t sqn[SQN_LENGTH];
} mysql_sqn_push_t;

typedef struct mysql_mme_identity_s{
  /* An MME may have already been registered as serving the UE. */
  char mme_host[255];
  char mme_realm[200];
} mysql_mme_identity_t;

typedef struct cassandra_mme_identity_s{
  /* An MME may have already been registered as serving the UE. */
  char mme_host[255];
  char mme_realm[200];
} cassandra_mme_identity_t;

typedef struct mysql_ul_ans_s{
  char imsi[IMSI_LENGTH_MAX + 1];
  /* MSISDN this parameter may be NULL */
  char msisdn[16];

  /* Maximum aggregated bitrates for the user */
  ambr_t aggr_ul;
  ambr_t aggr_dl;

  /* Subscribed RAU-TAU timer */
  rau_tau_t rau_tau;

  access_restriction_t access_restriction;
  mysql_mme_identity_t mme_identity;
} mysql_ul_ans_t;

typedef struct cassandra_ul_ans_s{
  char imsi[IMSI_LENGTH_MAX + 1];
  /* MSISDN this parameter may be NULL */
  char msisdn[16];

  /* Maximum aggregated bitrates for the user */
  ambr_t aggr_ul;
  ambr_t aggr_dl;

  /* Subscribed RAU-TAU timer */
  rau_tau_t rau_tau;

  access_restriction_t access_restriction;
  cassandra_mme_identity_t mme_identity;
  char *subscription_data;
} cassandra_ul_ans_t;

typedef struct cassandra_ul_push_s{
  /* Bit masks indicating presence of optional fields */
#define MME_IDENTITY_PRESENT           (0x1)
#define MME_SUPPORTED_FEATURES_PRESENT (0x1)
#define IMEI_PRESENT                   (0x1)
#define SV_PRESENT                     (0x1)
#define UE_SRVCC_PRESENT               (0x1)

  unsigned mme_identity_present:1;
  unsigned mme_supported_features_present:1;
  unsigned imei_present:1;
  unsigned sv_present:1;
  unsigned ue_srvcc_present:1;

  /* IMSI */
  char imsi[IMSI_LENGTH_MAX + 1];
  /* Origin host and realm */
  cassandra_mme_identity_t mme_identity;
  /* IMEISV */
  char imei[IMEI_LENGTH_MAX+1];
  char software_version[2+1];

  uint32_t ue_srvcc;
  uint32_t mme_supported_features;
} cassandra_ul_push_t;

typedef struct mysql_ul_push_s{
  /* Bit masks indicating presence of optional fields */
#define MME_IDENTITY_PRESENT           (0x1)
#define MME_SUPPORTED_FEATURES_PRESENT (0x1)
#define IMEI_PRESENT                   (0x1)
#define SV_PRESENT                     (0x1)
#define UE_SRVCC_PRESENT               (0x1)

  unsigned mme_identity_present:1;
  unsigned mme_supported_features_present:1;
  unsigned imei_present:1;
  unsigned sv_present:1;
  unsigned ue_srvcc_present:1;

  /* IMSI */
  char imsi[IMSI_LENGTH_MAX + 1];
  /* Origin host and realm */
  mysql_mme_identity_t mme_identity;
  /* IMEISV */
  char imei[IMEI_LENGTH_MAX+1];
  char software_version[2+1];

  uint32_t ue_srvcc;
  uint32_t mme_supported_features;
} mysql_ul_push_t;

typedef enum {
  IPV4         = 0,
  IPV6         = 1,
  IPV4V6       = 2,
  IPV4_OR_IPV6 = 3,
} pdn_type_t;

typedef struct pdn_address_s{
  char ipv4_address[INET_ADDRSTRLEN];
  char ipv6_address[INET6_ADDRSTRLEN];
} pdn_address_t;

typedef struct mysql_pdn_s{
  char          apn[61];
  pdn_type_t    pdn_type;
  pdn_address_t pdn_address;
  ambr_t        aggr_ul;
  ambr_t        aggr_dl;
  qci_t         qci;
  prio_level_t  priority_level;
  pre_emp_cap_t pre_emp_cap;
  pre_emp_vul_t pre_emp_vul;
} mysql_pdn_t;

typedef struct cassandra_pdn_s{
  char          apn[61];
  pdn_type_t    pdn_type;
  pdn_address_t pdn_address;
  ambr_t        aggr_ul;
  ambr_t        aggr_dl;
  qci_t         qci;
  prio_level_t  priority_level;
  pre_emp_cap_t pre_emp_cap;
  pre_emp_vul_t pre_emp_vul;
} cassandra_pdn_t;

typedef struct mysql_pu_req_s{
  /* IMSI */
  char imsi[IMSI_LENGTH_MAX + 1];
} mysql_pu_req_t;

typedef struct cassandra_pu_req_s{
  /* IMSI */
  char imsi[IMSI_LENGTH_MAX + 1];
} cassandra_pu_req_t;

typedef mysql_mme_identity_t mysql_pu_ans_t;

typedef cassandra_mme_identity_t cassandra_pu_ans_t;

int hss_mysql_connect(const hss_config_t *hss_config_p);

void hss_mysql_disconnect(void);

int hss_mysql_get_user(const char *imsi);

int hss_mysql_update_loc(const char *imsi, mysql_ul_ans_t *mysql_ul_ans);

int hss_cassandra_update_loc(const char *imsi, cassandra_ul_ans_t *cass_ul_ans);

int hss_mysql_query_mmeidentity(const int id_mme_identity,
                                mysql_mme_identity_t *mme_identity_p);

int hss_cassandra_query_mmeidentity(const int id_mme_identity,
                                cassandra_mme_identity_t *mme_identity_p);

int hss_mysql_check_epc_equipment(mysql_mme_identity_t *mme_identity_p);

int hss_cassandra_check_epc_equipment(cassandra_mme_identity_t *mme_identity_p);

int mysql_push_up_loc(mysql_ul_push_t *ul_push_p);

int cassandra_push_up_loc(cassandra_ul_push_t *ul_push_p);

int hss_mysql_purge_ue(mysql_pu_req_t *mysql_pu_req,
                       mysql_pu_ans_t *mysql_pu_ans);

int hss_cassandra_purge_ue(cassandra_pu_req_t *cass_pu_req,
                       cassandra_pu_ans_t *cass_pu_ans);

int hss_mysql_query_pdns(const char   *imsi,
                         mysql_pdn_t **pdns_p,
                         uint8_t      *nb_pdns);

int hss_cassandra_query_pdns(const char   *imsi,
                         cassandra_pdn_t *pdns_p);

int hss_mysql_auth_info(mysql_auth_info_req_t  *auth_info_req,
                        mysql_auth_info_resp_t *auth_info_resp);

int hss_cassandra_auth_info(cassandra_auth_info_req_t  *auth_info_req,
                        cassandra_auth_info_resp_t *auth_info_resp);

int hss_mysql_push_rand_sqn(const char *imsi, uint8_t *rand_p, uint8_t *sqn);

int hss_cassandra_push_rand_sqn(const char *imsi, uint8_t *rand_p, uint8_t *sqn);

int hss_mysql_increment_sqn(const char *imsi);

int hss_cassandra_increment_sqn(const char *imsi, uint8_t *sqn);

int hss_mysql_check_opc_keys(const uint8_t const opP[16]);

uint64_t get_latest_identity(const char *table_name);

int update_identity_table(const char* table_name);

void convert_ascii_to_binary(unsigned char *dest, unsigned char *src, int length);

#endif /* DB_PROTO_H_ */

/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the terms found in the LICENSE file in the root of this source tree.
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

#ifndef HSS_CONFIG_H_
#define HSS_CONFIG_H_

typedef struct hss_config_s {
  char* cassandra_server;
  char* cassandra_user;
  char* cassandra_password;
  char* cassandra_database;

  char* operator_key;
  unsigned char operator_key_bin[16];
  int valid_op;

  /* The freediameter configuration file */
  char* freediameter_config;

  /* THe HSS global configuration file */
  char* config;

  char* random;
  char random_bool;
} hss_config_t;

// int hss_config_init(int argc, char *argv[], hss_config_t *hss_config_p);

#endif /* HSS_CONFIG_H_ */

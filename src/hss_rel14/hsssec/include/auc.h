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

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "aucpp.h"

#ifndef AUC_H_
#define AUC_H_

#define SQN_LENGTH_BITS (48)
#define SQN_LENGTH_OCTEST (SQN_LENGTH_BITS / 8)
#define IK_LENGTH_BITS (128)
#define IK_LENGTH_OCTETS (IK_LENGTH_BITS / 8)
#define RAND_LENGTH_OCTETS (16)
#define RAND_LENGTH_BITS (RAND_LENGTH_OCTETS * 8)
#define XRES_LENGTH_OCTETS (8)
#define AUTN_LENGTH_OCTETS (16)
#define KASME_LENGTH_OCTETS (32)
#define MAC_S_LENGTH (8)

extern uint8_t opc[16];
typedef uint8_t uint8_t;

void RijndaelKeySchedule(uint8_t const key[16]);
void RijndaelEncrypt(uint8_t const in[16], uint8_t out[16]);

/* Sequence number functions */
struct sqn_ue_s;
struct sqn_ue_s* sqn_exists(uint64_t imsi);
void sqn_insert(struct sqn_ue_s* item);
void sqn_init(struct sqn_ue_s* item);
struct sqn_ue_s* sqn_new(uint64_t imsi);
void sqn_list_init(void);
void sqn_get(uint64_t imsi, uint8_t sqn[6]);

/* Random number functions */
struct random_state_s;
void generate_random(uint8_t* random, ssize_t length);

// void SetOP(char *opP);

void ComputeOPc(uint8_t const kP[16], uint8_t const opP[16], uint8_t opcP[16]);

void f1(
    uint8_t const kP[16], uint8_t const k[16], uint8_t const rand[16],
    uint8_t const sqn[6], uint8_t const amf[2], uint8_t mac_a[8]);
void f1star(
    uint8_t const kP[16], uint8_t const k[16], uint8_t const rand[16],
    uint8_t const sqn[6], uint8_t const amf[2], uint8_t mac_s[8]);
void f2345(
    uint8_t const kP[16], uint8_t const k[16], uint8_t const rand[16],
    uint8_t res[8], uint8_t ck[16], uint8_t ik[16], uint8_t ak[6]);
void f5star(
    uint8_t const kP[16], uint8_t const k[16], uint8_t const rand[16],
    uint8_t ak[6]);

void generate_autn(
    uint8_t const sqn[6], uint8_t const ak[6], uint8_t const amf[2],
    uint8_t const mac_a[8], uint8_t autn[16]);
int generate_vector(
    uint8_t const opc[16], uint64_t imsi, uint8_t key[16], uint8_t plmn[3],
    uint8_t sqn[6], auc_vector_t* vector);

void kdf(
    uint8_t* key, uint16_t key_len, uint8_t* s, uint16_t s_len, uint8_t* out,
    uint16_t out_len);

void derive_kasme(
    uint8_t ck[16], uint8_t ik[16], uint8_t plmn[3], uint8_t sqn[6],
    uint8_t ak[6], uint8_t kasme[32]);

uint8_t* sqn_ms_derive(
    uint8_t const opc[16], uint8_t* key, uint8_t* auts, uint8_t* rand);

static inline void print_buffer(
    const char* prefix, uint8_t* buffer, int length) {
#ifdef NODEBUG
  return;
#else
  int i;

  fprintf(stdout, "%s", prefix);

  for (i = 0; i < length; i++) {
    fprintf(stdout, "%02x.", buffer[i]);
  }

  fprintf(stdout, "\n");
#endif
}

#endif /* AUC_H_ */

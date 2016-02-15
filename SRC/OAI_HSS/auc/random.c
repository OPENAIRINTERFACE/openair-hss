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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <gmp.h>
#include <sys/time.h>

#include "auc.h"
#include "hss_config.h"

typedef struct random_state_s {
  pthread_mutex_t                         lock;
  gmp_randstate_t                         state;
} random_state_t;

random_state_t                          random_state;
extern hss_config_t                     hss_config;

void
random_init (
  void)
{
  //    mpz_t number;
  //    pthread_mutex_init(&random_state.lock, NULL);
  //    mpz_init(number);
  //    gmp_randinit_default(random_state.state);
  //    srand(time(NULL));
  struct timeval                          t1;

  gettimeofday (&t1, NULL);
  srand (t1.tv_usec * t1.tv_sec);
}

/* Generate a random number between 0 and 2^length - 1 where length is expressed
   in bits.
*/
void
generate_random (
  uint8_t * random_p,
  ssize_t length)
{
  //    random_t random_nb;
  //    mpz_init_set_ui(random_nb, 0);
  //    pthread_mutex_lock(&random_state.lock);
  //    mpz_urandomb(random_nb, random_state.state, 8 * length);
  //    pthread_mutex_unlock(&random_state.lock);
  //    mpz_export(random_p, NULL, 1, length, 0, 0, random_nb);
  int                                     i;    //r = 0,  mask = 0, shift;

  if (hss_config.random_bool > 0) {
    for (i = 0; i < length; i++) {
      //        if ((i % sizeof(i)) == 0)
      //            r = rand();
      //        shift = 8 * (i % sizeof(i));
      //        mask = 0xFF << shift;
      //        random_p[i] = (r & mask) >> shift;
      random_p[i] = rand ();
    }
  } else {
    for (i = 0; i < length; i++) {
      random_p[i] = i; // no random value
    }
  }
}

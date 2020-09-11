/*
 * Copyright (c) 2017 Sprint
 *
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
 */

#ifndef __SATOMIC_H
#define __SATOMIC_H

#include <ext/atomicity.h>

#define atomic_dec_fetch(a) __sync_sub_and_fetch(&a, 1)
#define atomic_inc_fetch(a) __sync_add_and_fetch(&a, 1)
#define atomic_fetch_dec(a) __sync_fetch_and_sub(&a, 1)
#define atomic_fetch_inc(a) __sync_fetch_and_add(&a, 1)

#define atomic_add_fetch(a, b) __sync_add_and_fetch(&a, b)
#define atomic_sub_fetch(a, b) __sync_sub_and_fetch(&a, b)
#define atomic_fetch_add(a, b) __sync_fetch_and_add(&a, b)
#define atomic_fetch_sub(a, b) __sync_fetch_and_sub(&a, b)

#define atomic_fetch_and(a, b) __sync_fetch_and_and(&a, b)
#define atomic_fetch_or(a, b) __sync_fetch_and_or(&a, b)
#define atomic_and_fetch(a, b) __sync_and_and_fetch(&a, b)
#define atomic_or_fetch(a, b) __sync_or_and_fetch(&a, b)

#define atomic_cas(a, b, c) __sync_val_compare_and_swap(&a, b, c)
#define atomic_swap(a, b) __sync_lock_test_and_set(&a, b)

#endif  // #define __SATOMIC_H

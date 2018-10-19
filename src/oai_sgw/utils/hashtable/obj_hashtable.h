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

/*! \file obj_hashtable.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#ifndef FILE_OBJ_HASHTABLE_SEEN
#define FILE_OBJ_HASHTABLE_SEEN

#include "hashtable.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct obj_hash_node_s {
    int                 key_size;
    void               *key;
    void               *data;
    struct obj_hash_node_s *next;
} obj_hash_node_t;

typedef struct obj_hash_node_uint64_s {
    int                 key_size;
    void               *key;
    uint64_t            data;
    struct obj_hash_node_uint64_s *next;
} obj_hash_node_uint64_t;

typedef struct obj_hash_table_s {
    pthread_mutex_t     mutex;
    hash_size_t         size;
    hash_size_t         num_elements;
    struct obj_hash_node_s **nodes;
    pthread_mutex_t     *lock_nodes;
    hash_size_t       (*hashfunc)(const void*, int);
    void              (*freekeyfunc)(void**);
    void              (*freedatafunc)(void**);
    bstring             name;
    bool                log_enabled;
} obj_hash_table_t;
typedef struct obj_hash_table_uint64_s {
    pthread_mutex_t     mutex;
    hash_size_t         size;
    hash_size_t         num_elements;
    struct obj_hash_node_uint64_s **nodes;
    pthread_mutex_t     *lock_nodes;
    hash_size_t       (*hashfunc)(const void*, int);
    void              (*freekeyfunc)(void**);
    bstring             name;
    bool                log_enabled;
} obj_hash_table_uint64_t;

void                obj_hashtable_no_free_key_callback(void* param);
obj_hash_table_t   *obj_hashtable_init (obj_hash_table_t * const hashtblP, const hash_size_t sizeP, hash_size_t (*hashfuncP) (const void *,int),void (*freekeyfuncP) (void **),void (*freedatafuncP) (void **), bstring display_name_pP);
obj_hash_table_t   *obj_hashtable_create  (const hash_size_t   size, hash_size_t (*hashfunc)(const void*, int ), void (*freekeyfunc)(void**), void (*freedatafunc)(void**), bstring display_name_pP);
hashtable_rc_t      obj_hashtable_destroy (obj_hash_table_t * const hashtblP);
hashtable_rc_t      obj_hashtable_is_key_exists (const obj_hash_table_t * const hashtblP, const void* const keyP, const int key_sizeP) __attribute__ ((hot, warn_unused_result));
hashtable_rc_t      obj_hashtable_insert  (obj_hash_table_t * const hashtblP,       const void* const keyP, const int key_sizeP, void *dataP);
hashtable_rc_t      obj_hashtable_dump_content (const obj_hash_table_t * const hashtblP, bstring str);
hashtable_rc_t      obj_hashtable_free  (obj_hash_table_t *hashtblP, const void* keyP, const int key_sizeP);
hashtable_rc_t      obj_hashtable_remove(obj_hash_table_t *hashtblP, const void* keyP, const int key_sizeP, void** dataP);
hashtable_rc_t      obj_hashtable_get     (const obj_hash_table_t * const hashtblP, const void* const keyP, const int key_sizeP, void ** dataP) __attribute__ ((hot));
hashtable_rc_t      obj_hashtable_get_keys(const obj_hash_table_t * const hashtblP, void ** keysP, unsigned int * sizeP);
hashtable_rc_t      obj_hashtable_resize  (obj_hash_table_t * const hashtblP, const hash_size_t sizeP);

// Thread-safe functions
obj_hash_table_t   *obj_hashtable_ts_init (obj_hash_table_t * const hashtblP, const hash_size_t sizeP, hash_size_t (*hashfuncP) (const void *,int),void (*freekeyfuncP) (void **),void (*freedatafuncP) (void **),bstring display_name_pP);
obj_hash_table_t   *obj_hashtable_ts_create  (const hash_size_t   size, hash_size_t (*hashfunc)(const void*, int ), void (*freekeyfunc)(void**), void (*freedatafunc)(void**), bstring display_name_pP);
hashtable_rc_t      obj_hashtable_ts_destroy (obj_hash_table_t * const hashtblP);
hashtable_rc_t      obj_hashtable_ts_is_key_exists (const obj_hash_table_t * const hashtblP, const void* const keyP, const int key_sizeP) __attribute__ ((hot, warn_unused_result));
hashtable_rc_t      obj_hashtable_ts_insert  (obj_hash_table_t * const hashtblP,       const void* const keyP, const int key_sizeP, void *dataP);
hashtable_rc_t      obj_hashtable_ts_dump_content (const obj_hash_table_t * const hashtblP, bstring str);
hashtable_rc_t      obj_hashtable_ts_free  (obj_hash_table_t *hashtblP, const void* keyP, const int key_sizeP);
hashtable_rc_t      obj_hashtable_ts_remove(obj_hash_table_t *hashtblP, const void* keyP, const int key_sizeP, void** dataP);
hashtable_rc_t      obj_hashtable_ts_get     (const obj_hash_table_t * const hashtblP, const void* const keyP, const int key_sizeP, void ** dataP) __attribute__ ((hot));
hashtable_rc_t      obj_hashtable_ts_get_keys(const obj_hash_table_t * const hashtblP, void ** keysP, unsigned int * sizeP);
hashtable_rc_t      obj_hashtable_ts_resize  (obj_hash_table_t * const hashtblP, const hash_size_t sizeP);
obj_hash_table_uint64_t   *obj_hashtable_uint64_init (obj_hash_table_uint64_t * const hashtblP, const hash_size_t sizeP, hash_size_t (*hashfuncP) (const void *,int),void (*freekeyfuncP) (void **), bstring display_name_pP);
obj_hash_table_uint64_t   *obj_hashtable_uint64_create  (const hash_size_t   size, hash_size_t (*hashfunc)(const void*, int ), void (*freekeyfunc)(void**), bstring display_name_pP);
hashtable_rc_t      obj_hashtable_uint64_destroy (obj_hash_table_uint64_t * const hashtblP);
hashtable_rc_t      obj_hashtable_uint64_is_key_exists (const obj_hash_table_uint64_t * const hashtblP, const void* const keyP, const int key_sizeP) __attribute__ ((hot, warn_unused_result));
hashtable_rc_t      obj_hashtable_uint64_insert  (obj_hash_table_uint64_t * const hashtblP, const void* const keyP, const int key_sizeP, const uint64_t dataP);
hashtable_rc_t      obj_hashtable_uint64_dump_content (const obj_hash_table_uint64_t * const hashtblP, bstring str);
hashtable_rc_t      obj_hashtable_uint64_free  (obj_hash_table_uint64_t *hashtblP, const void* keyP, const int key_sizeP);
hashtable_rc_t      obj_hashtable_uint64_remove(obj_hash_table_uint64_t *hashtblP, const void* keyP, const int key_sizeP);
hashtable_rc_t      obj_hashtable_uint64_get     (const obj_hash_table_uint64_t * const hashtblP, const void* const keyP, const int key_sizeP, uint64_t * const dataP) __attribute__ ((hot));
hashtable_rc_t      obj_hashtable_uint64_get_keys(const obj_hash_table_uint64_t * const hashtblP, void ** keysP, unsigned int * sizeP);
hashtable_rc_t      obj_hashtable_uint64_resize  (obj_hash_table_uint64_t * const hashtblP, const hash_size_t sizeP);

// Thread-safe functions
obj_hash_table_uint64_t   *obj_hashtable_uint64_ts_init (obj_hash_table_uint64_t * const hashtblP, const hash_size_t sizeP, hash_size_t (*hashfuncP) (const void *,int),void (*freekeyfuncP) (void **), bstring display_name_pP);
obj_hash_table_uint64_t   *obj_hashtable_uint64_ts_create  (const hash_size_t   size, hash_size_t (*hashfunc)(const void*, int ), void (*freekeyfunc)(void**), bstring display_name_pP);
hashtable_rc_t      obj_hashtable_uint64_ts_destroy (obj_hash_table_uint64_t * const hashtblP);
hashtable_rc_t      obj_hashtable_uint64_ts_is_key_exists (const obj_hash_table_uint64_t * const hashtblP, const void* const keyP, const int key_sizeP) __attribute__ ((hot, warn_unused_result));
hashtable_rc_t      obj_hashtable_uint64_ts_insert  (obj_hash_table_uint64_t * const hashtblP, const void* const keyP, const int key_sizeP, const uint64_t dataP);
hashtable_rc_t      obj_hashtable_uint64_ts_dump_content (const obj_hash_table_uint64_t * const hashtblP, bstring str);
hashtable_rc_t      obj_hashtable_uint64_ts_free  (obj_hash_table_uint64_t *hashtblP, const void* keyP, const int key_sizeP);
hashtable_rc_t      obj_hashtable_uint64_ts_remove(obj_hash_table_uint64_t *hashtblP, const void* keyP, const int key_sizeP);
hashtable_rc_t      obj_hashtable_uint64_ts_get     (const obj_hash_table_uint64_t * const hashtblP, const void* const keyP, const int key_sizeP, uint64_t * const dataP) __attribute__ ((hot));
hashtable_rc_t      obj_hashtable_uint64_ts_get_keys(const obj_hash_table_uint64_t * const hashtblP, void ** keysP, unsigned int * sizeP);
hashtable_rc_t      obj_hashtable_uint64_ts_resize  (obj_hash_table_uint64_t * const hashtblP, const hash_size_t sizeP);

#ifdef __cplusplus
}
#endif

#endif


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

/*! \file hashtable.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#ifndef FILE_HASH_TABLE_SEEN
#define FILE_HASH_TABLE_SEEN

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t   hash_size_t;
typedef uint64_t hash_key_t;

#define HASHTABLE_NOT_A_KEY_VALUE ((uint64_t)-1)

typedef enum hashtable_return_code_e {
    HASH_TABLE_OK                  = 0,
    HASH_TABLE_INSERT_OVERWRITTEN_DATA,
    HASH_TABLE_KEY_NOT_EXISTS         ,
    HASH_TABLE_SEARCH_NO_RESULT       ,
    HASH_TABLE_KEY_ALREADY_EXISTS     ,
    HASH_TABLE_BAD_PARAMETER_HASHTABLE,
    HASH_TABLE_BAD_PARAMETER_KEY,
    HASH_TABLE_SYSTEM_ERROR           ,
    HASH_TABLE_CODE_MAX
} hashtable_rc_t;

#define HASH_TABLE_DEFAULT_HASH_FUNC NULL
#define HASH_TABLE_DEFAULT_free_wrapper_FUNC NULL


typedef struct hash_node_s {
    hash_key_t          key;
    void               *data;
    struct hash_node_s *next;

} hash_node_t;

typedef struct hash_node_uint64_s {
    hash_key_t                 key;
    uint64_t                   data;
    struct hash_node_uint64_s *next;
} hash_node_uint64_t;

typedef struct hash_table_s {
    hash_size_t         size;
    hash_size_t         num_elements;
    struct hash_node_s **nodes;
    hash_size_t       (*hashfunc)(const hash_key_t);
    void              (*freefunc)(void**);
    bstring             name;
    bool                is_allocated_by_malloc;
    bool                log_enabled;
} hash_table_t;

typedef struct hash_table_ts_s {
    pthread_mutex_t     mutex;
    hash_size_t         size;
    hash_size_t         num_elements;
    struct hash_node_s **nodes;
    pthread_mutex_t     *lock_nodes;
    hash_size_t       (*hashfunc)(const hash_key_t);
    void              (*freefunc)(void**);
    bstring             name;
    bool                is_allocated_by_malloc;
    bool                log_enabled;
} hash_table_ts_t;
typedef struct hash_table_uint64_s {
    hash_size_t         size;
    hash_size_t         num_elements;
    struct hash_node_uint64_s **nodes;
    hash_size_t       (*hashfunc)(const hash_key_t);
    bstring             name;
    bool                is_allocated_by_malloc;
    bool                log_enabled;
} hash_table_uint64_t;

typedef struct hash_table_uint64_ts_s {
    pthread_mutex_t     mutex;
    hash_size_t         size;
    hash_size_t         num_elements;
    struct hash_node_uint64_s **nodes;
    pthread_mutex_t     *lock_nodes;
    hash_size_t       (*hashfunc)(const hash_key_t);
    bstring             name;
    bool                is_allocated_by_malloc;
    bool                log_enabled;
} hash_table_uint64_ts_t;

typedef struct hashtable_key_array_s {
    int                 num_keys;
    hash_key_t         *keys;
} hashtable_key_array_t;

typedef struct hashtable_element_array_s {
    int                 num_elements;
    void              **elements;
} hashtable_element_array_t;

typedef struct hashtable_uint64_element_array_s {
    int                 num_elements;
    uint64_t           *elements;
} hashtable_uint64_element_array_t;

char*           hashtable_rc_code2string(hashtable_rc_t rc);
void            hash_free_int_func(void** memory);
hash_table_t * hashtable_init (hash_table_t * const hashtbl,const hash_size_t size,hash_size_t (*hashfunc) (const hash_key_t),void (*freefunc) (void **),bstring display_name_p);
__attribute__ ((malloc)) hash_table_t   *hashtable_create (const hash_size_t   size, hash_size_t (*hashfunc)(const hash_key_t ), void (*freefunc)(void**), bstring name_p);
hashtable_rc_t  hashtable_destroy(hash_table_t * hashtbl);
hashtable_rc_t  hashtable_is_key_exists (const hash_table_t * const hashtbl, const hash_key_t key)                                              __attribute__ ((hot, warn_unused_result));
hashtable_rc_t  hashtable_apply_callback_on_elements (hash_table_t * const hashtbl,
                                                   bool func_cb(hash_key_t key, void* element, void* parameter, void**result),
                                                   void* parameter,
                                                   void**result);
hashtable_rc_t  hashtable_dump_content (const hash_table_t * const hashtbl, bstring str);
hashtable_rc_t  hashtable_insert (hash_table_t * const hashtbl, const hash_key_t key, void *element);
hashtable_rc_t  hashtable_free (hash_table_t * const hashtbl, const hash_key_t key);
hashtable_rc_t  hashtable_remove(hash_table_t * const hashtbl, const hash_key_t key, void** element);
hashtable_rc_t  hashtable_get    (const hash_table_t * const hashtbl, const hash_key_t key, void **element) __attribute__ ((hot));
hashtable_rc_t  hashtable_resize (hash_table_t * const hashtbl, const hash_size_t size);

// Thread-safe functions
hash_table_ts_t * hashtable_ts_init (hash_table_ts_t * const hashtbl,const hash_size_t size,hash_size_t (*hashfunc) (const hash_key_t),void (*freefunc) (void **),bstring display_name_p);
__attribute__ ((malloc)) hash_table_ts_t   *hashtable_ts_create (const hash_size_t   size, hash_size_t (*hashfunc)(const hash_key_t ), void (*freefunc)(void **), bstring name_p);
hashtable_rc_t  hashtable_ts_destroy(hash_table_ts_t * hashtbl);
hashtable_rc_t  hashtable_ts_is_key_exists (const hash_table_ts_t * const hashtbl, const hash_key_t key) __attribute__ ((hot, warn_unused_result));
hashtable_key_array_t * hashtable_ts_get_keys (hash_table_ts_t * const hashtblP);
hashtable_element_array_t* hashtable_ts_get_elements (hash_table_ts_t * const hashtblP);
hashtable_rc_t  hashtable_ts_apply_callback_on_elements (hash_table_ts_t * const hashtbl,
                                                      bool func_cb(const hash_key_t key, void* const element, void* parameter, void**result),
                                                      void* parameter,
                                                      void**result);
hashtable_rc_t  hashtable_ts_dump_content (const hash_table_ts_t * const hashtbl, bstring str);
hashtable_rc_t  hashtable_ts_insert (hash_table_ts_t * const hashtbl, const hash_key_t key, void *element);
hashtable_rc_t  hashtable_ts_free (hash_table_ts_t * const hashtbl, const hash_key_t key);
hashtable_rc_t  hashtable_ts_remove(hash_table_ts_t * const hashtbl, const hash_key_t key, void** element);
hashtable_rc_t  hashtable_ts_get    (const hash_table_ts_t * const hashtbl, const hash_key_t key, void **element) __attribute__ ((hot));
hashtable_rc_t  hashtable_ts_resize (hash_table_ts_t * const hashtbl, const hash_size_t size);
hash_table_uint64_ts_t * hashtable_uint64_ts_init (hash_table_uint64_ts_t * const hashtbl, const hash_size_t size, hash_size_t (*hashfunc) (const hash_key_t),bstring display_name_p);
__attribute__ ((malloc)) hash_table_uint64_ts_t   *hashtable_uint64_ts_create (const hash_size_t   size, hash_size_t (*hashfunc)(const hash_key_t ), bstring name_p);
hashtable_rc_t  hashtable_uint64_ts_destroy(hash_table_uint64_ts_t * hashtbl);
hashtable_rc_t  hashtable_uint64_ts_is_key_exists (const hash_table_uint64_ts_t * const hashtbl, const hash_key_t key) __attribute__ ((hot, warn_unused_result));
hashtable_key_array_t * hashtable_uint64_ts_get_keys (hash_table_uint64_ts_t * const hashtblP);
hashtable_uint64_element_array_t * hashtable_uint64_ts_get_elements (hash_table_uint64_ts_t * const hashtblP);
hashtable_rc_t  hashtable_uint64_ts_apply_callback_on_elements (hash_table_uint64_ts_t * const hashtbl,
                                                      bool func_cb(const hash_key_t key, const uint64_t element, void* parameter, void**result),
                                                      void* parameter,
                                                      void**result);
hashtable_rc_t  hashtable_uint64_ts_dump_content (const hash_table_uint64_ts_t * const hashtbl, bstring str);
hashtable_rc_t  hashtable_uint64_ts_insert (hash_table_uint64_ts_t * const hashtbl, const hash_key_t key, const uint64_t dataP);
hashtable_rc_t  hashtable_uint64_ts_free (hash_table_uint64_ts_t * const hashtbl, const hash_key_t key);
hashtable_rc_t  hashtable_uint64_ts_remove(hash_table_uint64_ts_t * const hashtbl, const hash_key_t key);
hashtable_rc_t  hashtable_uint64_ts_get    (const hash_table_uint64_ts_t * const hashtbl, const hash_key_t key, uint64_t * const dataP) __attribute__ ((hot));
hashtable_rc_t  hashtable_uint64_ts_resize (hash_table_uint64_ts_t * const hashtbl, const hash_size_t size);

#ifdef __cplusplus
}
#endif

#endif


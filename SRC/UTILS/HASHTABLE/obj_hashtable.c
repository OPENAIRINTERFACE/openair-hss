/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "obj_hashtable.h"
#include "dynamic_memory_check.h"

#if TRACE_HASHTABLE
#  define PRINT_HASHTABLE(...)  do { printf(##__VA_ARGS__);} while (0);
#else
#  define PRINT_HASHTABLE(...)
#endif

//------------------------------------------------------------------------------
// Free function selected if we do not want to FREE_CHECK the key when removing an entry
void
obj_hashtable_no_free_key_callback (
  void *param)
{
  ;                             // volountary do nothing
}

//------------------------------------------------------------------------------
/*
   Default hash function
   def_hashfunc() is the default used by hashtable_create() when the user didn't specify one.
   This is a simple/naive hash function which adds the key's ASCII char values. It will probably generate lots of collisions on large hash tables.
*/

static                                  hash_size_t
def_hashfunc (
  const void *const keyP,
  int key_sizeP)
{
  hash_size_t                             hash = 0;

  while (key_sizeP)
    hash ^= ((unsigned char *)keyP)[--key_sizeP];

  return hash;
}

//------------------------------------------------------------------------------
/*
 *    Initialization
 *    obj_hashtable_init() sets up the initial structure of the hash table. The user specified size will be allocated and initialized to NULL.
 *    The user can also specify a hash function. If the hashfunc argument is NULL, a default hash function is used.
 *    If an error occurred, NULL is returned. All other values in the returned obj_hash_table_t pointer should be released with hashtable_destroy().
 *
 */
obj_hash_table_t *obj_hashtable_init (
  obj_hash_table_t * const hashtblP,
  const hash_size_t sizeP,
  hash_size_t (*hashfuncP) (const void *,int),
  void (*freekeyfuncP) (void *),
  void (*freedatafuncP) (void *),
  char *display_name_pP)
{

  if (!(hashtblP->nodes = CALLOC_CHECK (sizeP, sizeof (obj_hash_node_t *)))) {
    FREE_CHECK (hashtblP);
    return NULL;
  }

  hashtblP->size = sizeP;

  if (hashfuncP)
    hashtblP->hashfunc = hashfuncP;
  else
    hashtblP->hashfunc = def_hashfunc;

  if (freekeyfuncP)
    hashtblP->freekeyfunc = freekeyfuncP;
  else
    hashtblP->freekeyfunc = FREE_CHECK;

  if (freedatafuncP)
    hashtblP->freedatafunc = freedatafuncP;
  else
    hashtblP->freedatafunc = FREE_CHECK;

  if (display_name_pP) {
    hashtblP->name = STRDUP_CHECK(display_name_pP);
  } else {
    hashtblP->name = MALLOC_CHECK (64);
    snprintf (hashtblP->name, 64, "obj_hashtable@%p", hashtblP);
  }

  return hashtblP;
}

//------------------------------------------------------------------------------
/*
   Initialization
   obj_hashtable_create() allocate and set up the initial structure of the hash table. The user specified size will be allocated and initialized to NULL.
   The user can also specify a hash function. If the hashfunc argument is NULL, a default hash function is used.
   If an error occurred, NULL is returned. All other values in the returned obj_hash_table_t pointer should be released with hashtable_destroy().
*/
obj_hash_table_t                       *
obj_hashtable_create (
  const hash_size_t sizeP,
  hash_size_t (*hashfuncP) (const void *,
                            int),
  void (*freekeyfuncP) (void *),
  void (*freedatafuncP) (void *),
  char *display_name_pP)
{
  obj_hash_table_t                       *hashtbl;

  if (!(hashtbl = MALLOC_CHECK (sizeof (obj_hash_table_t))))
    return NULL;

  return obj_hashtable_init(hashtbl, sizeP, hashfuncP, freekeyfuncP, freedatafuncP, display_name_pP);
}

//------------------------------------------------------------------------------
/*
   Initialization
   obj_hashtable_ts_init() sets up the initial structure of the hash table. The user specified size will be allocated and initialized to NULL.
   The user can also specify a hash function. If the hashfunc argument is NULL, a default hash function is used.
   If an error occurred, NULL is returned. All other values in the returned obj_hash_table_t pointer should be released with hashtable_destroy().
*/
obj_hash_table_t                       *
obj_hashtable_ts_init (
  obj_hash_table_t * const hashtblP,
  const hash_size_t sizeP,
  hash_size_t (*hashfuncP) (const void *,
                            int),
  void (*freekeyfuncP) (void *),
  void (*freedatafuncP) (void *),
  char *display_name_pP)
{
  if (!(hashtblP->lock_nodes = CALLOC_CHECK (sizeP, sizeof (pthread_mutex_t)))) {
    FREE_CHECK (hashtblP->nodes);
    FREE_CHECK (hashtblP->name);
    FREE_CHECK (hashtblP);
    return NULL;
  }

  pthread_mutex_init(&hashtblP->mutex, NULL);
  for (int i = 0; i < sizeP; i++) {
    pthread_mutex_init(&hashtblP->lock_nodes[i], NULL);
  }

  return hashtblP;
}

//------------------------------------------------------------------------------
/*
   Initialisation
   obj_hashtable_ts_create() allocate and sets up the initial structure of the hash table. The user specified size will be allocated and initialized to NULL.
   The user can also specify a hash function. If the hashfunc argument is NULL, a default hash function is used.
   If an error occurred, NULL is returned. All other values in the returned obj_hash_table_t pointer should be released with hashtable_destroy().
*/
obj_hash_table_t                       *
obj_hashtable_ts_create (
  const hash_size_t sizeP,
  hash_size_t (*hashfuncP) (const void *,
                            int),
  void (*freekeyfuncP) (void *),
  void (*freedatafuncP) (void *),
  char *display_name_pP)
{
  obj_hash_table_t                       *hashtbl = NULL;

  if (!(hashtbl = obj_hashtable_create (sizeP,hashfuncP,freekeyfuncP,freedatafuncP,display_name_pP))) {
    return NULL;
  }

  return obj_hashtable_ts_init(hashtbl, sizeP, hashfuncP, freekeyfuncP, freedatafuncP, display_name_pP);
}

//------------------------------------------------------------------------------
/*
   Cleanup
   The hashtable_destroy() walks through the linked lists for each possible hash value, and releases the elements. It also releases the nodes array and the obj_hash_table_t.
*/
hashtable_rc_t
obj_hashtable_destroy (
  obj_hash_table_t * const hashtblP)
{
  hash_size_t                             n;
  obj_hash_node_t                        *node,
                                         *oldnode;

  for (n = 0; n < hashtblP->size; ++n) {
    node = hashtblP->nodes[n];

    while (node) {
      oldnode = node;
      node = node->next;
      hashtblP->freekeyfunc (oldnode->key);
      hashtblP->freedatafunc (oldnode->data);
      FREE_CHECK (oldnode);
    }
  }

  FREE_CHECK (hashtblP->nodes);
  FREE_CHECK (hashtblP);
  return HASH_TABLE_OK;
}

//------------------------------------------------------------------------------
/*
   Cleanup
   The hashtable_destroy() walks through the linked lists for each possible hash value, and releases the elements. It also releases the nodes array and the obj_hash_table_t.
*/
hashtable_rc_t
obj_hashtable_ts_destroy (
  obj_hash_table_t * const hashtblP)
{
  hash_size_t                             n;
  obj_hash_node_t                        *node,
                                         *oldnode;

  for (n = 0; n < hashtblP->size; ++n) {
    pthread_mutex_lock (&hashtblP->lock_nodes[n]);
    node = hashtblP->nodes[n];

    while (node) {
      oldnode = node;
      node = node->next;
      hashtblP->freekeyfunc (oldnode->key);
      hashtblP->freedatafunc (oldnode->data);
      FREE_CHECK (oldnode);
    }
    pthread_mutex_unlock (&hashtblP->lock_nodes[n]);
    pthread_mutex_destroy (&hashtblP->lock_nodes[n]);
  }

  FREE_CHECK (hashtblP->nodes);
  FREE_CHECK (hashtblP);
  return HASH_TABLE_OK;
}

//------------------------------------------------------------------------------
hashtable_rc_t
obj_hashtable_is_key_exists (
  const obj_hash_table_t * const hashtblP,
  const void *const keyP,
  const int key_sizeP)
{
  obj_hash_node_t                        *node;
  hash_size_t                             hash;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  hash = hashtblP->hashfunc (keyP, key_sizeP) % hashtblP->size;
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      PRINT_HASHTABLE (stderr, "%s(%s,key %p) hash %lx return OK\n", __FUNCTION__, hashtblP->name, keyP, hash);
      return HASH_TABLE_OK;
    } else if (node->key_size == key_sizeP) {
      if (memcmp (node->key, keyP, key_sizeP) == 0) {
        PRINT_HASHTABLE (stderr, "%s(%s,key %p) hash %lx return OK\n", __FUNCTION__, hashtblP->name, keyP, hash);
        return HASH_TABLE_OK;
      }
    }

    node = node->next;
  }

  PRINT_HASHTABLE (stderr, "%s(%s,key %p) hash %lx return KEY_NOT_EXISTS\n", __FUNCTION__, hashtblP->name, keyP, hash);
  return HASH_TABLE_KEY_NOT_EXISTS;
}
//------------------------------------------------------------------------------
hashtable_rc_t
obj_hashtable_ts_is_key_exists (
  const obj_hash_table_t * const hashtblP,
  const void *const keyP,
  const int key_sizeP)
{
  obj_hash_node_t                        *node;
  hash_size_t                             hash;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  hash = hashtblP->hashfunc (keyP, key_sizeP) % hashtblP->size;
  pthread_mutex_lock (&hashtblP->lock_nodes[hash]);
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      pthread_mutex_unlock (&hashtblP->lock_nodes[hash]);
      PRINT_HASHTABLE (stderr, "%s(%s,key %p) hash %lx return OK\n", __FUNCTION__, hashtblP->name, keyP, hash);
      return HASH_TABLE_OK;
    } else if (node->key_size == key_sizeP) {
      if (memcmp (node->key, keyP, key_sizeP) == 0) {
        pthread_mutex_unlock (&hashtblP->lock_nodes[hash]);
        PRINT_HASHTABLE (stderr, "%s(%s,key %p) hash %lx return OK\n", __FUNCTION__, hashtblP->name, keyP, hash);
        return HASH_TABLE_OK;
      }
    }

    node = node->next;
  }
  pthread_mutex_unlock (&hashtblP->lock_nodes[hash]);

  PRINT_HASHTABLE (stderr, "%s(%s,key %p) hash %lx return KEY_NOT_EXISTS\n", __FUNCTION__, hashtblP->name, keyP, hash);
  return HASH_TABLE_KEY_NOT_EXISTS;
}

//------------------------------------------------------------------------------
hashtable_rc_t
obj_hashtable_dump_content (
  const obj_hash_table_t * const hashtblP,
  char *const buffer_pP,
  int *const remaining_bytes_in_buffer_pP)
{
  obj_hash_node_t                        *node = NULL;
  unsigned int                            i = 0;
  int                                     rc;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    rc = snprintf (buffer_pP, *remaining_bytes_in_buffer_pP, "HASH_TABLE_BAD_PARAMETER_HASHTABLE");
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  while ((i < hashtblP->size) && (*remaining_bytes_in_buffer_pP > 0)) {
    if (hashtblP->nodes[i] != NULL) {
      node = hashtblP->nodes[i];

      while (node) {
        rc = snprintf (buffer_pP, *remaining_bytes_in_buffer_pP, "Hash %x Key %p Element %p\n", i, node->key, node->data);
        node = node->next;

        if ((0 > rc) || (*remaining_bytes_in_buffer_pP < rc)) {
          PRINT_HASHTABLE (stderr, "Error while dumping hashtable content");
        } else {
          *remaining_bytes_in_buffer_pP -= rc;
        }
      }
    }

    i += 1;
  }

  return HASH_TABLE_OK;
}
//------------------------------------------------------------------------------
hashtable_rc_t
obj_hashtable_ts_dump_content (
  const obj_hash_table_t * const hashtblP,
  char *const buffer_pP,
  int *const remaining_bytes_in_buffer_pP)
{
  obj_hash_node_t                        *node = NULL;
  unsigned int                            i = 0;
  int                                     rc = 0;
  int                                     rc2 = 0;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    rc = snprintf (buffer_pP, *remaining_bytes_in_buffer_pP, "HASH_TABLE_BAD_PARAMETER_HASHTABLE");
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  while ((i < hashtblP->size) && (*remaining_bytes_in_buffer_pP > 0)) {
    if (hashtblP->nodes[i] != NULL) {
      pthread_mutex_lock(&hashtblP->lock_nodes[i]);
      node = hashtblP->nodes[i];

      while (node) {
        rc2 = snprintf (&buffer_pP[rc], *remaining_bytes_in_buffer_pP, "Hash %x Key %p Key length %d Element %p\n", i, node->key, node->key_size, node->data);
        if ((0 > rc2) || (*remaining_bytes_in_buffer_pP < rc2)) {
          PRINT_HASHTABLE (stderr, "Error while dumping hashtable content");
        } else {
          *remaining_bytes_in_buffer_pP -= rc2;
          rc += rc2;
        }
        node = node->next;
      }
      pthread_mutex_unlock(&hashtblP->lock_nodes[i]);
    }
    i += 1;
  }
  buffer_pP[rc] = '\0';

  return HASH_TABLE_OK;
}

//------------------------------------------------------------------------------
/*
   Adding a new element
   To make sure the hash value is not bigger than size, the result of the user provided hash function is used modulo size.
*/
hashtable_rc_t
obj_hashtable_insert (
  obj_hash_table_t * const hashtblP,
  const void *const keyP,
  const int key_sizeP,
  void *dataP)
{
  obj_hash_node_t                        *node;
  hash_size_t                             hash;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  hash = hashtblP->hashfunc (keyP, key_sizeP) % hashtblP->size;
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      if (node->data) {
        hashtblP->freedatafunc (node->data);
      }

      node->data = dataP;
      node->key_size = key_sizeP;
      // waste of memory here (keyP is lost) we should FREE_CHECK it now
      PRINT_HASHTABLE (stderr, "%s(%s,key %p) hash %lx return INSERT_OVERWRITTEN_DATA\n", __FUNCTION__, hashtblP->name, keyP, hash);
      return HASH_TABLE_INSERT_OVERWRITTEN_DATA;
    }

    node = node->next;
  }

  if (!(node = MALLOC_CHECK (sizeof (obj_hash_node_t)))) {
    PRINT_HASHTABLE (stderr, "%s(%s,key %p) hash %lx return SYSTEM_ERROR\n", __FUNCTION__, hashtblP->name, keyP, hash);
    return HASH_TABLE_SYSTEM_ERROR;
  }

  if (!(node->key = MALLOC_CHECK (key_sizeP))) {
    PRINT_HASHTABLE (stderr, "%s(%s,key %p) hash %lx return SYSTEM_ERROR\n", __FUNCTION__, hashtblP->name, keyP, hash);
    FREE_CHECK (node);
    return -1;
  }

  memcpy (node->key, keyP, key_sizeP);
  node->data = dataP;
  node->key_size = key_sizeP;

  if (hashtblP->nodes[hash]) {
    node->next = hashtblP->nodes[hash];
  } else {
    node->next = NULL;
  }

  hashtblP->nodes[hash] = node;
  __sync_fetch_and_add (&hashtblP->num_elements, 1);
  PRINT_HASHTABLE (stdout, "%s(%s,key %p) hash %lx return OK\n", __FUNCTION__, hashtblP->name, keyP, hash);
  return HASH_TABLE_OK;
}

//------------------------------------------------------------------------------
/*
   Adding a new element
   To make sure the hash value is not bigger than size, the result of the user provided hash function is used modulo size.
*/
hashtable_rc_t
obj_hashtable_ts_insert (
  obj_hash_table_t * const hashtblP,
  const void *const keyP,
  const int key_sizeP,
  void *dataP)
{
  obj_hash_node_t                        *node;
  hash_size_t                             hash;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  hash = hashtblP->hashfunc (keyP, key_sizeP) % hashtblP->size;
  pthread_mutex_lock(&hashtblP->lock_nodes[hash]);
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      if (node->data) {
        hashtblP->freedatafunc (node->data);
      }

      node->data = dataP;
      node->key_size = key_sizeP;
      // no waste of memory here because if node->key == keyP, it is a reuse of the same key
      pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
      PRINT_HASHTABLE (stderr, "%s(%s,key %p) hash %lx return INSERT_OVERWRITTEN_DATA\n", __FUNCTION__, hashtblP->name, keyP, hash);
      return HASH_TABLE_INSERT_OVERWRITTEN_DATA;
    }

    node = node->next;
  }

  if (!(node = MALLOC_CHECK (sizeof (obj_hash_node_t)))) {
    pthread_mutex_unlock (&hashtblP->lock_nodes[hash]);
    PRINT_HASHTABLE (stderr, "%s(%s,key %p) hash %lx return SYSTEM_ERROR\n", __FUNCTION__, hashtblP->name, keyP, hash);
    return HASH_TABLE_SYSTEM_ERROR;
  }

  if (!(node->key = MALLOC_CHECK (key_sizeP))) {
    FREE_CHECK (node);
    pthread_mutex_unlock (&hashtblP->lock_nodes[hash]);
    PRINT_HASHTABLE (stderr, "%s(%s,key %p) hash %lx return SYSTEM_ERROR\n", __FUNCTION__, hashtblP->name, keyP, hash);
    return HASH_TABLE_SYSTEM_ERROR;
  }


  memcpy (node->key, keyP, key_sizeP);
  node->data = dataP;
  node->key_size = key_sizeP;

  if (hashtblP->nodes[hash]) {
    node->next = hashtblP->nodes[hash];
  } else {
    node->next = NULL;
  }

  hashtblP->nodes[hash] = node;
  __sync_fetch_and_add (&hashtblP->num_elements, 1);
  pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
  PRINT_HASHTABLE (stdout, "%s(%s,key %p) hash %lx return OK\n", __FUNCTION__, hashtblP->name, keyP, hash);
  return HASH_TABLE_OK;
}

//------------------------------------------------------------------------------
/*
   To remove an element from the hash table, we just search for it in the linked list for that hash value,
   and remove it if it is found. If it was not found, it is an error and -1 is returned.
*/
hashtable_rc_t
obj_hashtable_free (
  obj_hash_table_t * const hashtblP,
  const void *const keyP,
  const int key_sizeP)
{
  obj_hash_node_t                        *node,
                                         *prevnode = NULL;
  hash_size_t                             hash;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  hash = hashtblP->hashfunc (keyP, key_sizeP) % hashtblP->size;
  node = hashtblP->nodes[hash];

  while (node) {
    if ((node->key == keyP) || ((node->key_size == key_sizeP) && (memcmp (node->key, keyP, key_sizeP) == 0))) {
      if (prevnode) {
        prevnode->next = node->next;
      } else {
        hashtblP->nodes[hash] = node->next;
      }

      hashtblP->freekeyfunc (node->key);
      hashtblP->freedatafunc (node->data);
      FREE_CHECK (node);
      hashtblP->num_elements -= 1;
      PRINT_HASHTABLE (stdout, "%s(%s,key %p) hash %lx return OK\n", __FUNCTION__, hashtblP->name, keyP, hash);
      return HASH_TABLE_OK;
    }

    prevnode = node;
    node = node->next;
  }

  return HASH_TABLE_KEY_NOT_EXISTS;
}

//------------------------------------------------------------------------------
/*
   To remove an element from the hash table, we just search for it in the linked list for that hash value,
   and remove it if it is found. If it was not found, it is an error and -1 is returned.
*/
hashtable_rc_t
obj_hashtable_ts_free (
  obj_hash_table_t * const hashtblP,
  const void *const keyP,
  const int key_sizeP)
{
  obj_hash_node_t                        *node,
                                         *prevnode = NULL;
  hash_size_t                             hash;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  hash = hashtblP->hashfunc (keyP, key_sizeP) % hashtblP->size;
  pthread_mutex_lock(&hashtblP->lock_nodes[hash]);
  node = hashtblP->nodes[hash];

  while (node) {
    if ((node->key == keyP) || ((node->key_size == key_sizeP) && (memcmp (node->key, keyP, key_sizeP) == 0))) {
      if (prevnode) {
        prevnode->next = node->next;
      } else {
        hashtblP->nodes[hash] = node->next;
      }

      hashtblP->freekeyfunc (node->key);
      hashtblP->freedatafunc (node->data);
      FREE_CHECK (node);
      __sync_fetch_and_sub (&hashtblP->num_elements, 1);
      pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
      PRINT_HASHTABLE (stdout, "%s(%s,key %p) hash %lx return OK\n", __FUNCTION__, hashtblP->name, keyP, hash);
      return HASH_TABLE_OK;
    }

    prevnode = node;
    node = node->next;
  }
  pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);

  return HASH_TABLE_KEY_NOT_EXISTS;
}

//------------------------------------------------------------------------------
/*
   To remove an element from the hash table, we just search for it in the linked list for that hash value,
   and remove it if it is found. If it was not found, it is an error and -1 is returned.
*/
hashtable_rc_t
obj_hashtable_remove (
  obj_hash_table_t * const hashtblP,
  const void *const keyP,
  const int key_sizeP,
  void **dataP)
{
  obj_hash_node_t                        *node,
                                         *prevnode = NULL;
  hash_size_t                             hash;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  hash = hashtblP->hashfunc (keyP, key_sizeP) % hashtblP->size;
  node = hashtblP->nodes[hash];

  while (node) {
    if ((node->key == keyP) || ((node->key_size == key_sizeP) && (memcmp (node->key, keyP, key_sizeP) == 0))) {
      if (prevnode) {
        prevnode->next = node->next;
      } else {
        hashtblP->nodes[hash] = node->next;
      }

      hashtblP->freekeyfunc (node->key);
      *dataP = node->data;
      FREE_CHECK (node);
      hashtblP->num_elements -= 1;
      PRINT_HASHTABLE (stdout, "%s(%s,key %p) hash %lx return OK\n", __FUNCTION__, hashtblP->name, keyP, hash);
      return HASH_TABLE_OK;
    }

    prevnode = node;
    node = node->next;
  }

  return HASH_TABLE_KEY_NOT_EXISTS;
}

//------------------------------------------------------------------------------
/*
   To remove an element from the hash table, we just search for it in the linked list for that hash value,
   and remove it if it is found. If it was not found, it is an error and -1 is returned.
*/
hashtable_rc_t
obj_hashtable_ts_remove (
  obj_hash_table_t * const hashtblP,
  const void *const keyP,
  const int key_sizeP,
  void **dataP)
{
  obj_hash_node_t                        *node,
                                         *prevnode = NULL;
  hash_size_t                             hash;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  hash = hashtblP->hashfunc (keyP, key_sizeP) % hashtblP->size;
  pthread_mutex_lock(&hashtblP->lock_nodes[hash]);
  node = hashtblP->nodes[hash];

  while (node) {
    if ((node->key == keyP) || ((node->key_size == key_sizeP) && (memcmp (node->key, keyP, key_sizeP) == 0))) {
      if (prevnode) {
        prevnode->next = node->next;
      } else {
        hashtblP->nodes[hash] = node->next;
      }

      hashtblP->freekeyfunc (node->key);
      *dataP = node->data;
      FREE_CHECK (node);
      __sync_fetch_and_sub (&hashtblP->num_elements, 1);
      pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
      PRINT_HASHTABLE (stdout, "%s(%s,key %p) hash %lx return OK\n", __FUNCTION__, hashtblP->name, keyP, hash);
      return HASH_TABLE_OK;
    }

    prevnode = node;
    node = node->next;
  }
  pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);

  return HASH_TABLE_KEY_NOT_EXISTS;
}

//------------------------------------------------------------------------------
/*
   Searching for an element is easy. We just search through the linked list for the corresponding hash value.
   NULL is returned if we didn't find it.
*/
hashtable_rc_t
obj_hashtable_get (
  const obj_hash_table_t * const hashtblP,
  const void *const keyP,
  const int key_sizeP,
  void **dataP)
{
  obj_hash_node_t                        *node;
  hash_size_t                             hash;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    *dataP = NULL;
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  hash = hashtblP->hashfunc (keyP, key_sizeP) % hashtblP->size;
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      *dataP = node->data;
      PRINT_HASHTABLE (stdout, "%s(%s,key %p) hash %lx return OK\n", __FUNCTION__, hashtblP->name, keyP, hash);
      return HASH_TABLE_OK;
    } else if (node->key_size == key_sizeP) {
      if (memcmp (node->key, keyP, key_sizeP) == 0) {
        *dataP = node->data;
        PRINT_HASHTABLE (stdout, "%s(%s,key %p) hash %lx return OK\n", __FUNCTION__, hashtblP->name, keyP, hash);
        return HASH_TABLE_OK;
      }
    }

    node = node->next;
  }

  *dataP = NULL;
  PRINT_HASHTABLE (stderr, "%s(%s,key %p) hash %lx return KEY_NOT_EXISTS\n", __FUNCTION__, hashtblP->name, keyP, hash);
  return HASH_TABLE_KEY_NOT_EXISTS;
}

//------------------------------------------------------------------------------
/*
   Searching for an element is easy. We just search through the linked list for the corresponding hash value.
   NULL is returned if we didn't find it.
*/
hashtable_rc_t
obj_hashtable_ts_get (
  const obj_hash_table_t * const hashtblP,
  const void *const keyP,
  const int key_sizeP,
  void **dataP)
{
  obj_hash_node_t                        *node;
  hash_size_t                             hash;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    *dataP = NULL;
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  hash = hashtblP->hashfunc (keyP, key_sizeP) % hashtblP->size;
  pthread_mutex_lock(&hashtblP->lock_nodes[hash]);
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      *dataP = node->data;
      pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
      PRINT_HASHTABLE (stdout, "%s(%s,key %p) hash %lx return OK\n", __FUNCTION__, hashtblP->name, keyP, hash);
      return HASH_TABLE_OK;
    } else if (node->key_size == key_sizeP) {
      if (memcmp (node->key, keyP, key_sizeP) == 0) {
        *dataP = node->data;
        pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
        PRINT_HASHTABLE (stdout, "%s(%s,key %p) hash %lx return OK\n", __FUNCTION__, hashtblP->name, keyP, hash);
        return HASH_TABLE_OK;
      }
    }

    node = node->next;
  }

  *dataP = NULL;
  pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
  PRINT_HASHTABLE (stderr, "%s(%s,key %p) hash %lx return KEY_NOT_EXISTS\n", __FUNCTION__, hashtblP->name, keyP, hash);
  return HASH_TABLE_KEY_NOT_EXISTS;
}

//------------------------------------------------------------------------------
/*
   Function to return all keys of an object hash table
*/
hashtable_rc_t
obj_hashtable_get_keys (
  const obj_hash_table_t * const hashtblP,
  void **keysP,
  unsigned int *sizeP)
{
  size_t                                  n = 0;
  obj_hash_node_t                        *node = NULL;
  obj_hash_node_t                        *next = NULL;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    keysP = NULL;
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  *sizeP = 0;
  keysP = CALLOC_CHECK (hashtblP->num_elements, sizeof (void *));

  if (keysP) {
    for (n = 0; n < hashtblP->size; ++n) {
      for (node = hashtblP->nodes[n]; node; node = next) {
        keysP[*sizeP++] = node->key;
        next = node->next;
      }
    }

    PRINT_HASHTABLE (stdout, "%s return OK\n", __FUNCTION__);
    return HASH_TABLE_OK;
  }
  PRINT_HASHTABLE (stderr, "%s return SYSTEM_ERROR\n", __FUNCTION__);
  return HASH_TABLE_SYSTEM_ERROR;
}

//------------------------------------------------------------------------------
/*
   Function to return all keys of an object hash table
*/
hashtable_rc_t
obj_hashtable_ts_get_keys (
  const obj_hash_table_t * const hashtblP,
  void **keysP,
  unsigned int *sizeP)
{
  size_t                                  n = 0;
  obj_hash_node_t                        *node = NULL;
  obj_hash_node_t                        *next = NULL;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    keysP = NULL;
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  *sizeP = 0;
  keysP = CALLOC_CHECK (hashtblP->num_elements,  sizeof (void *));

  if (keysP) {
    for (n = 0; n < hashtblP->size; ++n) {
      pthread_mutex_lock (&hashtblP->lock_nodes[n]);
      for (node = hashtblP->nodes[n]; node; node = next) {
        keysP[*sizeP++] = node->key;
        next = node->next;
      }
      pthread_mutex_unlock (&hashtblP->lock_nodes[n]);
    }

    PRINT_HASHTABLE (stdout, "%s return OK\n", __FUNCTION__);
    return HASH_TABLE_OK;
  }
  PRINT_HASHTABLE (stderr, "%s return SYSTEM_ERROR\n", __FUNCTION__);
  return HASH_TABLE_SYSTEM_ERROR;
}

//------------------------------------------------------------------------------
/*
   Resizing
   The number of elements in a hash table is not always known when creating the table.
   If the number of elements grows too large, it will seriously reduce the performance of most hash table operations.
   If the number of elements are reduced, the hash table will waste memory. That is why we provide a function for resizing the table.
   Resizing a hash table is not as easy as a realloc(). All hash values must be recalculated and each element must be inserted into its new position.
   We create a temporary obj_hash_table_t object (newtbl) to be used while building the new hashes.
   This allows us to reuse hashtable_insert() and hashtable_free(), when moving the elements to the new table.
   After that, we can just FREE_CHECK the old table and copy the elements from newtbl to hashtbl.
*/
hashtable_rc_t
obj_hashtable_resize (
  obj_hash_table_t * const hashtblP,
  const hash_size_t sizeP)
{
  obj_hash_table_t                        newtbl = {.mutex = PTHREAD_MUTEX_INITIALIZER, 0};
  hash_size_t                             n;
  obj_hash_node_t                        *node,
                                         *next;
  void                                   *dummy = NULL;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  newtbl.size = sizeP;
  newtbl.hashfunc = hashtblP->hashfunc;

  if (!(newtbl.nodes = CALLOC_CHECK (sizeP, sizeof (obj_hash_node_t *))))
    return HASH_TABLE_SYSTEM_ERROR;

  for (n = 0; n < hashtblP->size; ++n) {
    for (node = hashtblP->nodes[n]; node; node = next) {
      next = node->next;
      obj_hashtable_remove (hashtblP, node->key, node->key_size, &dummy);
      obj_hashtable_insert (&newtbl, node->key, node->key_size, node->data);
    }
  }

  FREE_CHECK (hashtblP->nodes);
  hashtblP->size = newtbl.size;
  hashtblP->nodes = newtbl.nodes;
  PRINT_HASHTABLE (stdout, "%s return OK\n", __FUNCTION__);
  return HASH_TABLE_OK;
}

//------------------------------------------------------------------------------
/*
   Resizing
   The number of elements in a hash table is not always known when creating the table.
   If the number of elements grows too large, it will seriously reduce the performance of most hash table operations.
   If the number of elements are reduced, the hash table will waste memory. That is why we provide a function for resizing the table.
   Resizing a hash table is not as easy as a realloc(). All hash values must be recalculated and each element must be inserted into its new position.
   We create a temporary obj_hash_table_t object (newtbl) to be used while building the new hashes.
   This allows us to reuse hashtable_insert() and hashtable_free(), when moving the elements to the new table.
   After that, we can just FREE_CHECK the old table and copy the elements from newtbl to hashtbl.
*/
hashtable_rc_t
obj_hashtable_ts_resize (
  obj_hash_table_t * const hashtblP,
  const hash_size_t sizeP)
{
  obj_hash_table_t                        newtbl = {.mutex = PTHREAD_MUTEX_INITIALIZER, 0};
  hash_size_t                             n;
  obj_hash_node_t                        *node,
                                         *next;
  void                                   *dummy = NULL;

  if (hashtblP == NULL) {
    PRINT_HASHTABLE (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  newtbl.size = sizeP;
  newtbl.hashfunc = hashtblP->hashfunc;

  if (!(newtbl.nodes = CALLOC_CHECK (sizeP, sizeof (obj_hash_node_t *))))
    return HASH_TABLE_SYSTEM_ERROR;

  if (!(newtbl.lock_nodes = CALLOC_CHECK (sizeP, sizeof (pthread_mutex_t)))) {
    FREE_CHECK (newtbl.nodes);
    return HASH_TABLE_SYSTEM_ERROR;
  }
  for (n = 0; n < hashtblP->size; ++n) {
    pthread_mutex_init(&newtbl.lock_nodes[n], NULL);
  }

  pthread_mutex_lock(&hashtblP->mutex);
  for (n = 0; n < hashtblP->size; ++n) {
    for (node = hashtblP->nodes[n]; node; node = next) {
      next = node->next;
      obj_hashtable_ts_remove (hashtblP, node->key, node->key_size, &dummy);
      obj_hashtable_insert (&newtbl, node->key, node->key_size, node->data);
    }
  }

  FREE_CHECK (hashtblP->nodes);
  FREE_CHECK (hashtblP->nodes);
  hashtblP->size = newtbl.size;
  hashtblP->nodes = newtbl.nodes;
  hashtblP->lock_nodes = newtbl.lock_nodes;
  pthread_mutex_unlock(&hashtblP->mutex);
  PRINT_HASHTABLE (stdout, "%s return OK\n", __FUNCTION__);
  return HASH_TABLE_OK;
}

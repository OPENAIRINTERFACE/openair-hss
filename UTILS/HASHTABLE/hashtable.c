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
#include "dynamic_memory_check.h"
#include "hashtable.h"
#include "assertions.h"
#include "dynamic_memory_check.h"


//-------------------------------------------------------------------------------------------------------------------------------
char                                   *
hashtable_rc_code2string (
  hashtable_rc_t rcP)
//-------------------------------------------------------------------------------------------------------------------------------
{
  switch (rcP) {
  case HASH_TABLE_OK:
    return "HASH_TABLE_OK";
    break;

  case HASH_TABLE_INSERT_OVERWRITTEN_DATA:
    return "HASH_TABLE_INSERT_OVERWRITTEN_DATA";
    break;

  case HASH_TABLE_KEY_NOT_EXISTS:
    return "HASH_TABLE_KEY_NOT_EXISTS";
    break;

  case HASH_TABLE_KEY_ALREADY_EXISTS:
    return "HASH_TABLE_KEY_ALREADY_EXISTS";
    break;

  case HASH_TABLE_BAD_PARAMETER_HASHTABLE:
    return "HASH_TABLE_BAD_PARAMETER_HASHTABLE";
    break;

  default:
    return "UNKNOWN hashtable_rc_t";
  }
}

//-------------------------------------------------------------------------------------------------------------------------------
/*
   FREE_CHECK int function
   hash_free_int_func() is used when this hashtable is used to store int values as data (pointer = value).
*/

void
hash_free_int_func (
  void *memoryP)
{
}

//-------------------------------------------------------------------------------------------------------------------------------
/*
   Default hash function
   def_hashfunc() is the default used by hashtable_create() when the user didn't specify one.
   This is a simple/naive hash function which adds the key's ASCII char values. It will probably generate lots of collisions on large hash tables.
*/

static                                  hash_size_t
def_hashfunc (
  const uint64_t keyP)
{
  return (hash_size_t) keyP;
}

//-------------------------------------------------------------------------------------------------------------------------------
/*
   Initialisation
   hashtable_create() sets up the initial structure of the hash table. The user specified size will be allocated and initialized to NULL.
   The user can also specify a hash function. If the hashfunc argument is NULL, a default hash function is used.
   If an error occurred, NULL is returned. All other values in the returned hash_table_t pointer should be released with hashtable_destroy().
*/
hash_table_t                           *
hashtable_create (
  const hash_size_t sizeP,
  hash_size_t (*hashfuncP) (const hash_key_t),
  void (*freefuncP) (void *),
  char *display_name_pP)
{
  hash_table_t                           *hashtbl = NULL;

  if (!(hashtbl = CALLOC_CHECK (1, sizeof (hash_table_t)))) {
    return NULL;
  }

  if (!(hashtbl->nodes = CALLOC_CHECK (sizeP, sizeof (hash_node_t *)))) {
    FREE_CHECK (hashtbl);
    return NULL;
  }

  hashtbl->size = sizeP;

  if (hashfuncP)
    hashtbl->hashfunc = hashfuncP;
  else
    hashtbl->hashfunc = def_hashfunc;

  if (freefuncP)
    hashtbl->freefunc = freefuncP;
  else
    hashtbl->freefunc = FREE_CHECK;

  if (display_name_pP) {
    hashtbl->name = STRDUP_CHECK(display_name_pP);
  } else {
    hashtbl->name = MALLOC_CHECK(64);
    snprintf (hashtbl->name, 64, "hastable@%p", hashtbl);
  }

  return hashtbl;
}
//-------------------------------------------------------------------------------------------------------------------------------
/*
   Initialisation
   hashtable_create() sets up the initial structure of the hash table. The user specified size will be allocated and initialized to NULL.
   The user can also specify a hash function. If the hashfunc argument is NULL, a default hash function is used.
   If an error occurred, NULL is returned. All other values in the returned hash_table_t pointer should be released with hashtable_destroy().
*/
hash_table_t                           *
hashtable_ts_create (
  const hash_size_t sizeP,
  hash_size_t (*hashfuncP) (const hash_key_t),
  void (*freefuncP) (void *),
  char *display_name_pP)
{
  hash_table_t                           *hashtbl = NULL;

  if (!(hashtbl = hashtable_create (sizeP,hashfuncP,freefuncP,display_name_pP))) {
    return NULL;
  }


  if (!(hashtbl->lock_nodes = CALLOC_CHECK (sizeP, sizeof (pthread_mutex_t)))) {
    FREE_CHECK (hashtbl->nodes);
    FREE_CHECK (hashtbl);
    FREE_CHECK (hashtbl->name);
    return NULL;
  }

  for (int i = 0; i < sizeP; i++) {
    pthread_mutex_init (&hashtbl->lock_nodes[i], NULL);
  }

  return hashtbl;
}

//-------------------------------------------------------------------------------------------------------------------------------
/*
   Cleanup
   The hashtable_destroy() walks through the linked lists for each possible hash value, and releases the elements. It also releases the nodes array and the hash_table_t.
*/
hashtable_rc_t
hashtable_destroy (
  hash_table_t * const hashtblP)
{
  hash_size_t                             n;
  hash_node_t                            *node,
                                         *oldnode;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  for (n = 0; n < hashtblP->size; ++n) {
    node = hashtblP->nodes[n];

    while (node) {
      oldnode = node;
      node = node->next;

      if (oldnode->data) {
        hashtblP->freefunc (oldnode->data);
      }

      FREE_CHECK (oldnode);
    }
  }

  FREE_CHECK (hashtblP->nodes);
  FREE_CHECK(hashtblP->name);
  AssertFatal(NULL == hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");
  FREE_CHECK (hashtblP);
  return HASH_TABLE_OK;
}



/*
   Cleanup
   The hashtable_destroy() walks through the linked lists for each possible hash value, and releases the elements. It also releases the nodes array and the hash_table_t.
*/
hashtable_rc_t
hashtable_ts_destroy (
  hash_table_t * const hashtblP)
{
  hash_size_t                             n;
  hash_node_t                            *node,
                                         *oldnode;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }

  for (n = 0; n < hashtblP->size; ++n) {
    pthread_mutex_lock (&hashtblP->lock_nodes[n]);
    node = hashtblP->nodes[n];

    while (node) {
      oldnode = node;
      node = node->next;

      if (oldnode->data) {
        hashtblP->freefunc (oldnode->data);
      }

      FREE_CHECK (oldnode);
    }

    pthread_mutex_unlock (&hashtblP->lock_nodes[n]);
    pthread_mutex_destroy (&hashtblP->lock_nodes[n]);
  }

  FREE_CHECK (hashtblP->nodes);
  FREE_CHECK(hashtblP->name);
  FREE_CHECK (hashtblP);
  return HASH_TABLE_OK;
}



//-------------------------------------------------------------------------------------------------------------------------------
hashtable_rc_t
hashtable_is_key_exists (
  const hash_table_t * const hashtblP,
  const hash_key_t keyP)
//-------------------------------------------------------------------------------------------------------------------------------
{
  hash_node_t                            *node = NULL;
  hash_size_t                             hash = 0;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL == hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  hash = hashtblP->hashfunc (keyP) % hashtblP->size;
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
#if HASHTABLE_DEBUG
      fprintf (stdout, "%s(%s,key 0x%"PRIx64") return OK\n", __FUNCTION__, hashtblP->name, keyP);
#endif
      return HASH_TABLE_OK;
    }

    node = node->next;
  }
#if HASHTABLE_DEBUG
  fprintf (stdout, "%s(%s,key 0x%"PRIx64") return KEY_NOT_EXISTS\n", __FUNCTION__, hashtblP->name, keyP);
#endif
  return HASH_TABLE_KEY_NOT_EXISTS;
}



//-------------------------------------------------------------------------------------------------------------------------------
hashtable_rc_t
hashtable_ts_is_key_exists (
  const hash_table_t * const hashtblP,
  const hash_key_t keyP)
//-------------------------------------------------------------------------------------------------------------------------------
{
  hash_node_t                            *node = NULL;
  hash_size_t                             hash = 0;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL != hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  hash = hashtblP->hashfunc (keyP) % hashtblP->size;
  pthread_mutex_lock (&hashtblP->lock_nodes[hash]);
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      pthread_mutex_unlock (&hashtblP->lock_nodes[hash]);
#if HASHTABLE_DEBUG
      fprintf (stdout, "%s(%s,key 0x%"PRIx64") return OK\n", __FUNCTION__, hashtblP->name, keyP);
#endif
      return HASH_TABLE_OK;
    }

    node = node->next;
  }
  pthread_mutex_unlock (&hashtblP->lock_nodes[hash]);
#if HASHTABLE_DEBUG
  fprintf (stdout, "%s(%s,key 0x%"PRIx64") return KEY_NOT_EXISTS\n", __FUNCTION__, hashtblP->name, keyP);
#endif
  return HASH_TABLE_KEY_NOT_EXISTS;
}



//-------------------------------------------------------------------------------------------------------------------------------
hashtable_rc_t
hashtable_apply_funct_on_elements (
  hash_table_t * const hashtblP,
  void functP (hash_key_t keyP,
               void *dataP,
               void *parameterP),
  void *parameterP)
//-------------------------------------------------------------------------------------------------------------------------------
{
  hash_node_t                            *node = NULL;
  unsigned int                            i = 0;
  unsigned int                            num_elements = 0;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL == hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  while ((num_elements < hashtblP->num_elements) && (i < hashtblP->size)) {
    if (hashtblP->nodes[i] != NULL) {
      node = hashtblP->nodes[i];

      while (node) {
        num_elements += 1;
        functP (node->key, node->data, parameterP);
        node = node->next;
      }
    }

    i += 1;
  }

  return HASH_TABLE_OK;
}



//-------------------------------------------------------------------------------------------------------------------------------
hashtable_rc_t
hashtable_ts_apply_funct_on_elements (
  hash_table_t * const hashtblP,
  void functP (hash_key_t keyP,
               void *dataP,
               void *parameterP),
  void *parameterP)
//-------------------------------------------------------------------------------------------------------------------------------
{
  hash_node_t                            *node = NULL;
  unsigned int                            i = 0;
  unsigned int                            num_elements = 0;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL != hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  while ((num_elements < hashtblP->num_elements) && (i < hashtblP->size)) {
    pthread_mutex_lock(&hashtblP->lock_nodes[i]);
    if (hashtblP->nodes[i] != NULL) {
      node = hashtblP->nodes[i];

      while (node) {
        num_elements += 1;
        functP (node->key, node->data, parameterP);
        node = node->next;
      }
    }
    pthread_mutex_unlock(&hashtblP->lock_nodes[i]);
    i += 1;
  }

  return HASH_TABLE_OK;
}



//-------------------------------------------------------------------------------------------------------------------------------
hashtable_rc_t
hashtable_dump_content (
  const hash_table_t * const hashtblP,
  char *const buffer_pP,
  int *const remaining_bytes_in_buffer_pP)
//-------------------------------------------------------------------------------------------------------------------------------
{
  hash_node_t                            *node = NULL;
  unsigned int                            i = 0;
  int                                     rc;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    rc = snprintf (buffer_pP, *remaining_bytes_in_buffer_pP, "HASH_TABLE_BAD_PARAMETER_HASHTABLE");
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL == hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  while ((i < hashtblP->size) && (*remaining_bytes_in_buffer_pP > 0)) {
    if (hashtblP->nodes[i] != NULL) {
      node = hashtblP->nodes[i];

      while (node) {
        rc = snprintf (buffer_pP, *remaining_bytes_in_buffer_pP, "Key 0x%"PRIx64" Element %p\n", node->key, node->data);
        node = node->next;

        if ((0 > rc) || (*remaining_bytes_in_buffer_pP < rc)) {
          fprintf (stderr, "Error while dumping hashtable content");
        } else {
          *remaining_bytes_in_buffer_pP -= rc;
        }
      }
    }

    i += 1;
  }

  return HASH_TABLE_OK;
}



hashtable_rc_t
hashtable_ts_dump_content (
  const hash_table_t * const hashtblP,
  char *const buffer_pP,
  int *const remaining_bytes_in_buffer_pP)
//-------------------------------------------------------------------------------------------------------------------------------
{
  hash_node_t                            *node = NULL;
  unsigned int                            i = 0;
  int                                     rc;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    rc = snprintf (buffer_pP, *remaining_bytes_in_buffer_pP, "HASH_TABLE_BAD_PARAMETER_HASHTABLE");
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL != hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  while ((i < hashtblP->size) && (*remaining_bytes_in_buffer_pP > 0)) {
    if (hashtblP->nodes[i] != NULL) {
      pthread_mutex_lock(&hashtblP->lock_nodes[i]);
      node = hashtblP->nodes[i];

      while (node) {
        rc = snprintf (buffer_pP, *remaining_bytes_in_buffer_pP, "Key 0x%"PRIx64" Element %p\n", node->key, node->data);
        node = node->next;

        if ((0 > rc) || (*remaining_bytes_in_buffer_pP < rc)) {
          fprintf (stderr, "Error while dumping hashtable content");
        } else {
          *remaining_bytes_in_buffer_pP -= rc;
        }
      }
      pthread_mutex_unlock(&hashtblP->lock_nodes[i]);
    }
    i += 1;
  }
  return HASH_TABLE_OK;
}



//-------------------------------------------------------------------------------------------------------------------------------
/*
   Adding a new element
   To make sure the hash value is not bigger than size, the result of the user provided hash function is used modulo size.
*/
hashtable_rc_t
hashtable_insert (
  hash_table_t * const hashtblP,
  const hash_key_t keyP,
  void *dataP)
{
  hash_node_t                            *node = NULL;
  hash_size_t                             hash = 0;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL == hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  hash = hashtblP->hashfunc (keyP) % hashtblP->size;
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      if (node->data) {
        hashtblP->freefunc (node->data);
      }

      node->data = dataP;
#if HASHTABLE_DEBUG
      fprintf (stderr, "%s(%s,key 0x%"PRIx64") return INSERT_OVERWRITTEN_DATA\n", __FUNCTION__, hashtblP->name, keyP);
#endif
      return HASH_TABLE_INSERT_OVERWRITTEN_DATA;
    }

    node = node->next;
  }

  if (!(node = MALLOC_CHECK (sizeof (hash_node_t))))
    return -1;

  node->key = keyP;
  node->data = dataP;

  if (hashtblP->nodes[hash]) {
    node->next = hashtblP->nodes[hash];
  } else {
    node->next = NULL;
  }

  hashtblP->nodes[hash] = node;
  hashtblP->num_elements += 1;

#if HASHTABLE_DEBUG
  fprintf (stdout, "%s(%s,key 0x%"PRIx64") return OK\n", __FUNCTION__, hashtblP->name, keyP);
#endif
  return HASH_TABLE_OK;
}


/*
   Adding a new element
   To make sure the hash value is not bigger than size, the result of the user provided hash function is used modulo size.
*/
hashtable_rc_t
hashtable_ts_insert (
  hash_table_t * const hashtblP,
  const hash_key_t keyP,
  void *dataP)
{
  hash_node_t                            *node = NULL;
  hash_size_t                             hash = 0;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL != hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  hash = hashtblP->hashfunc (keyP) % hashtblP->size;
  pthread_mutex_lock(&hashtblP->lock_nodes[hash]);
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      if (node->data) {
        hashtblP->freefunc (node->data);
      }
      node->data = dataP;
      pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
#if HASHTABLE_DEBUG
      fprintf (stderr, "%s(%s,key 0x%"PRIx64") return INSERT_OVERWRITTEN_DATA\n", __FUNCTION__, hashtblP->name, keyP);
#endif
      return HASH_TABLE_INSERT_OVERWRITTEN_DATA;
    }

    node = node->next;
  }

  if (!(node = MALLOC_CHECK (sizeof (hash_node_t))))
    return -1;

  node->key = keyP;
  node->data = dataP;

  if (hashtblP->nodes[hash]) {
    node->next = hashtblP->nodes[hash];
  } else {
    node->next = NULL;
  }

  hashtblP->nodes[hash] = node;
  __sync_fetch_and_add (&hashtblP->num_elements, 1);
  pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
#if HASHTABLE_DEBUG
  fprintf (stdout, "%s(%s,key 0x%"PRIx64") return OK\n", __FUNCTION__, hashtblP->name, keyP);
#endif
  return HASH_TABLE_OK;
}



//-------------------------------------------------------------------------------------------------------------------------------
/*
   To FREE_CHECK an element from the hash table, we just search for it in the linked list for that hash value,
   and FREE_CHECK it if it is found. If it was not found, it is an error and -1 is returned.
*/
hashtable_rc_t
hashtable_free (
  hash_table_t * const hashtblP,
  const hash_key_t keyP)
{
  hash_node_t                            *node,
                                         *prevnode = NULL;
  hash_size_t                             hash = 0;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL == hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  hash = hashtblP->hashfunc (keyP) % hashtblP->size;
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      if (prevnode)
        prevnode->next = node->next;
      else
        hashtblP->nodes[hash] = node->next;

      if (node->data) {
        hashtblP->freefunc (node->data);
      }

      FREE_CHECK (node);
      __sync_fetch_and_sub (&hashtblP->num_elements, 1);
#if HASHTABLE_DEBUG
      fprintf (stdout, "%s(%s,key 0x%"PRIx64") return OK\n", __FUNCTION__, hashtblP->name, keyP);
#endif
      return HASH_TABLE_OK;
    }

    prevnode = node;
    node = node->next;
  }

#if HASHTABLE_DEBUG
  fprintf (stderr, "%s(%s,key 0x%"PRIx64") return KEY_NOT_EXISTS\n", __FUNCTION__, hashtblP->name, keyP);
#endif
  return HASH_TABLE_KEY_NOT_EXISTS;
}



/*
   To FREE_CHECK an element from the hash table, we just search for it in the linked list for that hash value,
   and FREE_CHECK it if it is found. If it was not found, it is an error and -1 is returned.
*/
hashtable_rc_t
hashtable_ts_free (
  hash_table_t * const hashtblP,
  const hash_key_t keyP)
{
  hash_node_t                            *node,
                                         *prevnode = NULL;
  hash_size_t                             hash = 0;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL != hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  hash = hashtblP->hashfunc (keyP) % hashtblP->size;
  pthread_mutex_lock(&hashtblP->lock_nodes[hash]);
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      if (prevnode)
        prevnode->next = node->next;
      else
        hashtblP->nodes[hash] = node->next;

      if (node->data) {
        hashtblP->freefunc (node->data);
      }

      FREE_CHECK (node);
      __sync_fetch_and_sub (&hashtblP->num_elements, 1);
      pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
#if HASHTABLE_DEBUG
      fprintf (stdout, "%s(%s,key 0x%"PRIx64") return OK\n", __FUNCTION__, hashtblP->name, keyP);
#endif
      return HASH_TABLE_OK;
    }

    prevnode = node;
    node = node->next;
  }

   pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
#if HASHTABLE_DEBUG
  fprintf (stderr, "%s(%s,key 0x%"PRIx64") return KEY_NOT_EXISTS\n", __FUNCTION__, hashtblP->name, keyP);
#endif
  return HASH_TABLE_KEY_NOT_EXISTS;
}



//-------------------------------------------------------------------------------------------------------------------------------
/*
   To remove an element from the hash table, we just search for it in the linked list for that hash value,
   and remove it if it is found. If it was not found, it is an error and -1 is returned.
*/
hashtable_rc_t
hashtable_remove (
  hash_table_t * const hashtblP,
  const hash_key_t keyP,
  void **dataP)
{
  hash_node_t                            *node,
                                         *prevnode = NULL;
  hash_size_t                             hash = 0;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL == hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  hash = hashtblP->hashfunc (keyP) % hashtblP->size;
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      if (prevnode)
        prevnode->next = node->next;
      else
        hashtblP->nodes[hash] = node->next;

      *dataP = node->data;
      FREE_CHECK (node);
      __sync_fetch_and_sub (&hashtblP->num_elements, 1);
#if HASHTABLE_DEBUG
      fprintf (stdout, "%s(%s,key 0x%"PRIx64") return OK\n", __FUNCTION__, hashtblP->name, keyP);
#endif
      return HASH_TABLE_OK;
    }

    prevnode = node;
    node = node->next;
  }

#if HASHTABLE_DEBUG
  fprintf (stderr, "%s(%s,key 0x%"PRIx64") return KEY_NOT_EXISTS\n", __FUNCTION__, hashtblP->name, keyP);
#endif
  return HASH_TABLE_KEY_NOT_EXISTS;
}


/*
   To remove an element from the hash table, we just search for it in the linked list for that hash value,
   and remove it if it is found. If it was not found, it is an error and -1 is returned.
*/
hashtable_rc_t
hashtable_ts_remove (
  hash_table_t * const hashtblP,
  const hash_key_t keyP,
  void **dataP)
{
  hash_node_t                            *node,
                                         *prevnode = NULL;
  hash_size_t                             hash = 0;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL != hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  hash = hashtblP->hashfunc (keyP) % hashtblP->size;
  pthread_mutex_lock(&hashtblP->lock_nodes[hash]);
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      if (prevnode)
        prevnode->next = node->next;
      else
        hashtblP->nodes[hash] = node->next;

      *dataP = node->data;
      FREE_CHECK (node);
      __sync_fetch_and_sub (&hashtblP->num_elements, 1);
      pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
#if HASHTABLE_DEBUG
      fprintf (stdout, "%s(%s,key 0x%"PRIx64") return OK\n", __FUNCTION__, hashtblP->name, keyP);
#endif
      return HASH_TABLE_OK;
    }

    prevnode = node;
    node = node->next;
  }
  pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);

#if HASHTABLE_DEBUG
  fprintf (stderr, "%s(%s,key 0x%"PRIx64") return KEY_NOT_EXISTS\n", __FUNCTION__, hashtblP->name, keyP);
#endif
  return HASH_TABLE_KEY_NOT_EXISTS;
}



//-------------------------------------------------------------------------------------------------------------------------------
/*
   Searching for an element is easy. We just search through the linked list for the corresponding hash value.
   NULL is returned if we didn't find it.
*/
hashtable_rc_t
hashtable_get (
  const hash_table_t * const hashtblP,
  const hash_key_t keyP,
  void **dataP)
{
  hash_node_t                            *node = NULL;
  hash_size_t                             hash = 0;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    *dataP = NULL;
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL == hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  hash = hashtblP->hashfunc (keyP) % hashtblP->size;
  /*
   * fprintf(stderr, "hashtable_get() key=%s, hash=%d\n", key, hash);
   */
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      *dataP = node->data;
#if HASHTABLE_DEBUG
      fprintf (stdout, "%s(%s,key 0x%"PRIx64") return OK\n", __FUNCTION__, hashtblP->name, keyP);
#endif
      return HASH_TABLE_OK;
    }

    node = node->next;
  }

  *dataP = NULL;
#if HASHTABLE_DEBUG
  fprintf (stderr, "%s(%s,key 0x%"PRIx64") return KEY_NOT_EXISTS\n", __FUNCTION__, hashtblP->name, keyP);
#endif
  return HASH_TABLE_KEY_NOT_EXISTS;
}



/*
   Searching for an element is easy. We just search through the linked list for the corresponding hash value.
   NULL is returned if we didn't find it.
*/
hashtable_rc_t
hashtable_ts_get (
  const hash_table_t * const hashtblP,
  const hash_key_t keyP,
  void **dataP)
{
  hash_node_t                            *node = NULL;
  hash_size_t                             hash = 0;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    *dataP = NULL;
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL != hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  hash = hashtblP->hashfunc (keyP) % hashtblP->size;
  /*
   * fprintf(stderr, "hashtable_get() key=%s, hash=%d\n", key, hash);
   */
  pthread_mutex_lock(&hashtblP->lock_nodes[hash]);
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      *dataP = node->data;
      pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
#if HASHTABLE_DEBUG
      fprintf (stdout, "%s(%s,key 0x%"PRIx64") return OK\n", __FUNCTION__, hashtblP->name, keyP);
#endif
      return HASH_TABLE_OK;
    }

    node = node->next;
  }

  *dataP = NULL;
  pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
#if HASHTABLE_DEBUG
  fprintf (stderr, "%s(%s,key 0x%"PRIx64") return KEY_NOT_EXISTS\n", __FUNCTION__, hashtblP->name, keyP);
#endif
  return HASH_TABLE_KEY_NOT_EXISTS;
}



//-------------------------------------------------------------------------------------------------------------------------------
/*
   Resizing
   The number of elements in a hash table is not always known when creating the table.
   If the number of elements grows too large, it will seriously reduce the performance of most hash table operations.
   If the number of elements are reduced, the hash table will waste memory. That is why we provide a function for resizing the table.
   Resizing a hash table is not as easy as a realloc(). All hash values must be recalculated and each element must be inserted into its new position.
   We create a temporary hash_table_t object (newtbl) to be used while building the new hashes.
   This allows us to reuse hashtable_insert() and hashtable_free(), when moving the elements to the new table.
   After that, we can just FREE_CHECK the old table and copy the elements from newtbl to hashtbl.
*/

hashtable_rc_t
hashtable_resize (
  hash_table_t * const hashtblP,
  const hash_size_t sizeP)
{
  hash_table_t                            newtbl;
  hash_size_t                             n;
  hash_node_t                            *node,
                                         *next;
  void                                   *dummy = NULL;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL == hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  newtbl.size = sizeP;
  newtbl.hashfunc = hashtblP->hashfunc;

  if (!(newtbl.nodes = CALLOC_CHECK (sizeP, sizeof (hash_node_t *))))
    return -1;

  for (n = 0; n < hashtblP->size; ++n) {
    for (node = hashtblP->nodes[n]; node; node = next) {
      next = node->next;
      hashtable_remove (hashtblP, node->key, &dummy);
      hashtable_insert (&newtbl, node->key, node->data);
    }
  }

  FREE_CHECK (hashtblP->nodes);
  hashtblP->size = newtbl.size;
  hashtblP->nodes = newtbl.nodes;
  return HASH_TABLE_OK;
}



/*
   Resizing
   The number of elements in a hash table is not always known when creating the table.
   If the number of elements grows too large, it will seriously reduce the performance of most hash table operations.
   If the number of elements are reduced, the hash table will waste memory. That is why we provide a function for resizing the table.
   Resizing a hash table is not as easy as a realloc(). All hash values must be recalculated and each element must be inserted into its new position.
   We create a temporary hash_table_t object (newtbl) to be used while building the new hashes.
   This allows us to reuse hashtable_insert() and hashtable_free(), when moving the elements to the new table.
   After that, we can just FREE_CHECK the old table and copy the elements from newtbl to hashtbl.
   Dangerous not really thread safe.
*/

hashtable_rc_t
hashtable_ts_resize (
  hash_table_t * const hashtblP,
  const hash_size_t sizeP)
{
  hash_table_t                            newtbl = {0};
  hash_size_t                             n      = 0;
  hash_node_t                            *node   = NULL,
                                         *next   = NULL;
  void                                   *dummy  = NULL;

  if (hashtblP == NULL) {
#if HASHTABLE_DEBUG
    fprintf (stderr, "%s return BAD_PARAMETER_HASHTABLE\n", __FUNCTION__);
#endif
    return HASH_TABLE_BAD_PARAMETER_HASHTABLE;
  }
  AssertFatal(NULL != hashtblP->lock_nodes, "Mismatch Thread-Safe hashtable or memory coruption");

  newtbl.size = sizeP;
  newtbl.hashfunc = hashtblP->hashfunc;

  if (!(newtbl.nodes = CALLOC_CHECK (sizeP, sizeof (hash_node_t *))))
    return -1;

  for (n = 0; n < hashtblP->size; ++n) {
    for (node = hashtblP->nodes[n]; node; node = next) {
      next = node->next;
      hashtable_ts_remove (hashtblP, node->key, &dummy);
      hashtable_insert (&newtbl, node->key, node->data);
    }
  }

  FREE_CHECK (hashtblP->nodes);
  hashtblP->size = newtbl.size;
  hashtblP->nodes = newtbl.nodes;
  return HASH_TABLE_OK;
}

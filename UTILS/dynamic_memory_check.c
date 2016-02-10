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
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include "assertions.h"
#include "backtrace.h"
#include "dynamic_memory_check.h"


//-------------------------------------------------------------------------------------------------------------------------------
typedef struct dmc_malloc_info_s {
  pthread_t                alloc_tid;
  pthread_t                free_tid;
  size_t                   size;
} dmc_malloc_info_t;

//-------------------------------------------------------------------------------------------------------------------------------
typedef struct dmc_hash_node_s {
    uintptr_t               key;
    dmc_malloc_info_t      *data;
    struct dmc_hash_node_s *next;
} dmc_hash_node_t;

typedef struct dmc_hash_table_s {
    size_t                   size;
    size_t                   num_elements;
    struct dmc_hash_node_s **nodes;
    pthread_mutex_t         *lock_nodes;
} dmc_hash_table_t;

typedef enum dmc_hashtable_return_code_e {
    DMC_HASH_TABLE_OK                      = 0,
    DMC_HASH_TABLE_INSERT_OVERWRITTEN_DATA = 1,
    DMC_HASH_TABLE_KEY_NOT_EXISTS          = 2,
    DMC_HASH_TABLE_SYSTEM_ERROR            = 5,
    DMC_HASH_TABLE_CODE_MAX
} dmc_hashtable_rc_t;

//-------------------------------------------------------------------------------------------------------------------------------
#define DYNAMIC_MEMORY_CHECK_HASH_SIZE 1024
// keep track of memory allocated
// use of "already there" hashtable collection, we may think of better suitable collection
dmc_hash_table_t g_dma_htbl = {0};
dmc_hash_table_t g_dma_free_htbl = {0};

inline static void dma_register_pointer(void* ptr, size_t size);
inline static void dma_deregister_pointer(void* ptr);
static dmc_hashtable_rc_t dmc_hashtable_ts_is_key_exists (const dmc_hash_table_t * const hashtblP, const uintptr_t keyP);
static dmc_hashtable_rc_t dmc_hashtable_ts_insert (dmc_hash_table_t * const hashtblP, const uintptr_t keyP, dmc_malloc_info_t *dataP);
static dmc_hashtable_rc_t dmc_hashtable_ts_free (dmc_hash_table_t * const hashtblP, const uintptr_t keyP);
static dmc_hashtable_rc_t dmc_hashtable_ts_remove (dmc_hash_table_t * const hashtblP, const uintptr_t keyP, dmc_malloc_info_t **dataP);
//------------------------------------------------------------------------------
dmc_hashtable_rc_t
dmc_hashtable_ts_is_key_exists (
  const dmc_hash_table_t * const hashtblP,
  const uintptr_t keyP)
{
  dmc_hash_node_t                    *node = NULL;
  size_t                              hash = 0;


  hash = (size_t)(keyP) % hashtblP->size;
  pthread_mutex_lock (&hashtblP->lock_nodes[hash]);
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      pthread_mutex_unlock (&hashtblP->lock_nodes[hash]);
      return DMC_HASH_TABLE_OK;
    }
    node = node->next;
  }
  pthread_mutex_unlock (&hashtblP->lock_nodes[hash]);
  return DMC_HASH_TABLE_KEY_NOT_EXISTS;
}

//------------------------------------------------------------------------------
dmc_hashtable_rc_t
dmc_hashtable_ts_insert (
  dmc_hash_table_t * const hashtblP,
  const uintptr_t keyP,
  dmc_malloc_info_t *dataP)
{
  dmc_hash_node_t                        *node = NULL;
  size_t                                  hash = 0;

  hash = (size_t)(keyP) % hashtblP->size;
  pthread_mutex_lock(&hashtblP->lock_nodes[hash]);
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      if (node->data) {
        free(node->data);
      }
      node->data = dataP;
      pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
      return DMC_HASH_TABLE_INSERT_OVERWRITTEN_DATA;
    }

    node = node->next;
  }

  if (!(node = malloc (sizeof (dmc_hash_node_t))))
    return DMC_HASH_TABLE_SYSTEM_ERROR;

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
  return DMC_HASH_TABLE_OK;
}

//------------------------------------------------------------------------------
dmc_hashtable_rc_t
dmc_hashtable_ts_free (
    dmc_hash_table_t * const hashtblP,
  const uintptr_t keyP)
{
  dmc_hash_node_t                        *node,
                                         *prevnode = NULL;
  size_t                                  hash = 0;

  hash = (size_t)(keyP) % hashtblP->size;
  pthread_mutex_lock(&hashtblP->lock_nodes[hash]);
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      if (prevnode)
        prevnode->next = node->next;
      else
        hashtblP->nodes[hash] = node->next;

      if (node->data) {
        free (node->data);
      }

      free (node);
      __sync_fetch_and_sub (&hashtblP->num_elements, 1);
      pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
      return DMC_HASH_TABLE_OK;
    }

    prevnode = node;
    node = node->next;
  }

  pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
  return DMC_HASH_TABLE_KEY_NOT_EXISTS;
}

//------------------------------------------------------------------------------
dmc_hashtable_rc_t
dmc_hashtable_ts_remove (
  dmc_hash_table_t * const hashtblP,
  const uintptr_t keyP,
  dmc_malloc_info_t **dataP)
{
  dmc_hash_node_t                        *node,
                                         *prevnode = NULL;
  size_t                                  hash = 0;

  hash = (size_t)(keyP) % hashtblP->size;
  pthread_mutex_lock(&hashtblP->lock_nodes[hash]);
  node = hashtblP->nodes[hash];

  while (node) {
    if (node->key == keyP) {
      if (prevnode)
        prevnode->next = node->next;
      else
        hashtblP->nodes[hash] = node->next;

      *dataP = node->data;
      free (node);
      __sync_fetch_and_sub (&hashtblP->num_elements, 1);
      pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);
      return DMC_HASH_TABLE_OK;
    }

    prevnode = node;
    node = node->next;
  }
  pthread_mutex_unlock(&hashtblP->lock_nodes[hash]);

  return DMC_HASH_TABLE_KEY_NOT_EXISTS;
}

//------------------------------------------------------------------------------
void dyn_mem_check_init(void)
{
  fprintf (stdout, "Initializing Dynamic memory check\n");
  g_dma_htbl.nodes = calloc (1, DYNAMIC_MEMORY_CHECK_HASH_SIZE * sizeof (dmc_hash_node_t *));
  AssertFatal(NULL != g_dma_htbl.nodes , "Could not allocate memory");
  g_dma_free_htbl.nodes = calloc (1, DYNAMIC_MEMORY_CHECK_HASH_SIZE * sizeof (dmc_hash_node_t *));
  AssertFatal(NULL != g_dma_free_htbl.nodes , "Could not allocate memory");

  g_dma_htbl.lock_nodes = calloc (1, DYNAMIC_MEMORY_CHECK_HASH_SIZE * sizeof (pthread_mutex_t));
  AssertFatal(NULL != g_dma_htbl.lock_nodes , "Could not allocate memory");
  g_dma_free_htbl.lock_nodes = calloc (1, DYNAMIC_MEMORY_CHECK_HASH_SIZE * sizeof (pthread_mutex_t));
  AssertFatal(NULL != g_dma_free_htbl.lock_nodes , "Could not allocate memory");

  for (int i = 0; i < DYNAMIC_MEMORY_CHECK_HASH_SIZE; i++) {
    pthread_mutex_init (&g_dma_htbl.lock_nodes[i], NULL);
    pthread_mutex_init (&g_dma_free_htbl.lock_nodes[i], NULL);
  }

  g_dma_htbl.size = DYNAMIC_MEMORY_CHECK_HASH_SIZE;
  g_dma_free_htbl.size = DYNAMIC_MEMORY_CHECK_HASH_SIZE;

  fprintf (stdout, "Initializing Dynamic memory check done\n");
}

//------------------------------------------------------------------------------
void dyn_mem_check_exit(void)
{

  fprintf (stdout, "Exiting Dynamic memory check\n");
  //TODO display remaining allocated pointers, their source


  //TODO free remaining pointers

  // now free internal memory
  for (int i = 0; i < DYNAMIC_MEMORY_CHECK_HASH_SIZE; i++) {
    pthread_mutex_destroy (&g_dma_htbl.lock_nodes[i]);
    pthread_mutex_destroy (&g_dma_free_htbl.lock_nodes[i]);
  }

  // free all internal structures
  free(g_dma_htbl.lock_nodes);
  free(g_dma_free_htbl.lock_nodes);

  // free all internal structures
  free(g_dma_htbl.nodes);
  free(g_dma_free_htbl.nodes);
  fprintf (stdout, "Exiting Dynamic memory check done\n");
}

//------------------------------------------------------------------------------
void dma_register_pointer(void* ptr, size_t size)
{
  dmc_hashtable_rc_t  rc = dmc_hashtable_ts_is_key_exists (&g_dma_htbl, (const uintptr_t)ptr);
  AssertFatal(DMC_HASH_TABLE_KEY_NOT_EXISTS == rc , "Pointer not marked free  %p rc = %d\n", ptr, rc);
  AssertFatal(DMC_HASH_TABLE_OK != rc , "Pointer not marked free ? %p rc = %d\n", ptr, rc);
  dmc_malloc_info_t *info = malloc(sizeof(dmc_malloc_info_t));
  info->size = size;
  info->alloc_tid  = pthread_self();
  info->free_tid   = 0;
  fprintf (stdout, "Registering  %p alloc tid %08lX free tid %08lX\n", ptr, info->alloc_tid, info->free_tid);
  rc = dmc_hashtable_ts_insert(&g_dma_htbl, (const uintptr_t)ptr, info);
  AssertFatal(DMC_HASH_TABLE_OK == rc, "Dynamic Memory Alloc Error, could not register pointer %p", ptr);

  rc = dmc_hashtable_ts_is_key_exists (&g_dma_free_htbl, (const uintptr_t)ptr);
  if (DMC_HASH_TABLE_OK == rc) {
    // may reuse free memory
    fprintf (stdout, "Flushing     %p alloc tid %08lX free tid %08lX\n", ptr, info->alloc_tid, info->free_tid);
    rc = dmc_hashtable_ts_free(&g_dma_free_htbl, (const uintptr_t)ptr);
  }
}

//------------------------------------------------------------------------------
void dma_deregister_pointer(void* ptr)
{
  dmc_malloc_info_t *info = NULL;
  dmc_hashtable_rc_t  rc = dmc_hashtable_ts_remove(&g_dma_htbl, (const uintptr_t)ptr, &info);
  if (DMC_HASH_TABLE_OK == rc) {
    info->free_tid = pthread_self();
    fprintf (stdout, "Freeing      %p alloc tid %08lX free tid %08lX\n", ptr, info->alloc_tid, info->free_tid);
    rc = dmc_hashtable_ts_insert(&g_dma_free_htbl, (const uintptr_t)ptr, info);
    AssertFatal(DMC_HASH_TABLE_OK == rc, "Memory Free Error ???");
  } else {
    // may be already free
    rc = dmc_hashtable_ts_is_key_exists (&g_dma_free_htbl, (const uintptr_t)ptr);
    AssertFatal(DMC_HASH_TABLE_OK != rc, "Pointer %p already free", ptr);
    display_backtrace();
    AssertFatal(0, "Trying to free a non allocated pointer %p tid %08lX", ptr, pthread_self());
  }
}

//------------------------------------------------------------------------------
void *malloc_wrapper(size_t size)
{
  void *ptr = malloc(size);
  if (ptr) {
    dma_register_pointer(ptr, size);
  }
  return ptr;
}

//------------------------------------------------------------------------------
void free_wrapper(void *ptr)
{
  if (ptr) {
    dma_deregister_pointer(ptr);
  }
  free(ptr);
  ptr = NULL;
}

//------------------------------------------------------------------------------
void *calloc_wrapper(size_t nmemb, size_t size)
{
  void *ptr = calloc(nmemb, size);
  if (ptr) {
    dma_register_pointer(ptr, size);
  }
  return ptr;
}
/*
 * The realloc() function changes the size of the memory block pointed to by ptr to size bytes.
 * The contents will be unchanged in the range from the start of the region up to the minimum of
 * the old and new sizes.  If the new size is larger than the old size, the added memory will
 * not be initialized.
 * If ptr is NULL, then the call is equivalent to malloc(size), for all values of size;
 *  if size is equal to zero, and ptr is not NULL, then the call is equivalent to free(ptr).
 * Unless ptr is NULL, it must have been returned by an earlier call to malloc(), calloc() or  realloc().
 * If the area pointed to was moved, a free(ptr) is done.
 *
 * The realloc() function returns a pointer to the newly allocated memory, which is suitably aligned
 * for any kind of variable and may be different from ptr, or NULL if the request fails.
 * If size was equal to 0, either NULL or a pointer suitable to be passed to free() is returned.
 * If realloc() fails the original block is left untouched; it is not freed or moved.
 *
 */
void *realloc_wrapper(void *ptr, size_t size)
{
  void *ptr2 = realloc(ptr, size);
  if ((ptr2) && (ptr2 != ptr)) {
    dma_deregister_pointer(ptr);
    dma_register_pointer(ptr2, size);
  } else if ((0 == size) && (ptr)) {
    dma_deregister_pointer(ptr);
  }
  return ptr2;
}


char *strdup_wrapper(const char *s)
{
  char * ptr = strdup(s);
  if (ptr) {
    dma_register_pointer(ptr, strlen(s)+1);
  }
  return ptr;
}

char *strndup_wrapper(const char *s, size_t n)
{
  char * ptr = strndup(s, n);
  if (ptr) {
    dma_register_pointer(ptr, n+1);
  }
  return ptr;
}

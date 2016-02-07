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
#include <stdlib.h>

#include "hashtable.h"
#include "assertions.h"


#define DYNAMIC_MEMORY_CHECK_HASH_SIZE 1024
// keep track of memory allocated
// use of "already there" hashtable collection, we may think of better suitable collection
hash_table_t g_dma_htbl = {0};
hash_table_t g_dma_free_htbl = {0};

static  hash_size_t def_hashfunc (const uint64_t keyP);


static  hash_size_t def_hashfunc (const uint64_t keyP)
{
  return (hash_size_t) keyP;
}

void dyn_mem_check_init(void) {
  g_dma_htbl.nodes = calloc (DYNAMIC_MEMORY_CHECK_HASH_SIZE, sizeof (hash_node_t *));
  AssertFatal(NULL != g_dma_htbl.nodes , "Could not allocate memory");
  g_dma_free_htbl.nodes = calloc (DYNAMIC_MEMORY_CHECK_HASH_SIZE, sizeof (hash_node_t *));
  AssertFatal(NULL != g_dma_free_htbl.nodes , "Could not allocate memory");

  g_dma_htbl.lock_nodes = calloc (DYNAMIC_MEMORY_CHECK_HASH_SIZE, sizeof (pthread_mutex_t));
  AssertFatal(NULL != g_dma_htbl.lock_nodes , "Could not allocate memory");
  g_dma_free_htbl.lock_nodes = calloc (DYNAMIC_MEMORY_CHECK_HASH_SIZE, sizeof (pthread_mutex_t));
  AssertFatal(NULL != g_dma_free_htbl.lock_nodes , "Could not allocate memory");

  for (int i = 0; i < DYNAMIC_MEMORY_CHECK_HASH_SIZE; i++) {
    pthread_mutex_init (&g_dma_htbl.lock_nodes[i], NULL);
    pthread_mutex_init (&g_dma_free_htbl.lock_nodes[i], NULL);
  }

  g_dma_htbl.size = DYNAMIC_MEMORY_CHECK_HASH_SIZE;
  g_dma_htbl.hashfunc = def_hashfunc;
  g_dma_htbl.freefunc = free_wrapper;

  g_dma_free_htbl.size = DYNAMIC_MEMORY_CHECK_HASH_SIZE;
  g_dma_free_htbl.hashfunc = def_hashfunc;
  g_dma_free_htbl.freefunc = free_wrapper;

  snprintf (g_dma_htbl.name, 64, "DYNAMIC_MEMORY_CHECK_HASHTBL@%p", &g_dma_htbl);
  snprintf (g_dma_htbl.name, 64, "DYNAMIC_MEMORY_CHECK_FREE_HASHTBL@%p", &g_dma_free_htbl);
}

void dma_register_pointer(void* ptr) {
  hashtable_rc_t  rc = hashtable_ts_is_key_exists (&g_dma_htbl, (const uint64_t)ptr);
  AssertFatal(HASH_TABLE_KEY_ALREADY_EXISTS != rc , "Memory check error ???");
  rc = hashtable_ts_insert(&g_dma_htbl, (const uint64_t)ptr, NULL);
  AssertFatal(HASH_TABLE_OK == rc, "Memory Free Error ???");

  rc = hashtable_ts_is_key_exists (&g_dma_free_htbl, (const uint64_t)ptr);
  if (HASH_TABLE_OK == rc) {
    // may reuse free memory
    rc = hashtable_ts_free(&g_dma_free_htbl, (const uint64_t)ptr);
  }

}

void dma_deregister_pointer(void* ptr) {
  hashtable_rc_t  rc = hashtable_ts_is_key_exists (&g_dma_htbl, (const uint64_t)ptr);
  if (HASH_TABLE_OK != rc) {
    // may be already free
    rc = hashtable_ts_is_key_exists (&g_dma_free_htbl, (const uint64_t)ptr);
    AssertFatal(HASH_TABLE_OK != rc, "Pointer %p already free", ptr);
    AssertFatal(HASH_TABLE_OK == rc, "Trying to free a non allocated pointer %p", ptr);
  }
  rc = hashtable_ts_free(&g_dma_htbl, (const uint64_t)ptr);
  AssertFatal(HASH_TABLE_OK == rc, "Memory Free Error ???");
  rc = hashtable_ts_insert(&g_dma_free_htbl, (const uint64_t)ptr, NULL);
  AssertFatal(HASH_TABLE_OK == rc, "Memory Free Error ???");
}


void *malloc_wrapper(size_t size)
{
  void *ptr = malloc(size);
  if (ptr) {
    dma_register_pointer(ptr);
  }
  return ptr;
}

void free_wrapper(void *ptr)
{
  if (ptr) {
    dma_deregister_pointer(ptr);
  }
  free(ptr);
  ptr = NULL;
}

void *calloc_wrapper(size_t nmemb, size_t size)
{
  void *ptr = calloc(nmemb, size);
  if (ptr) {
    dma_register_pointer(ptr);
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
    dma_register_pointer(ptr2);
  } else if ((0 == size) && (ptr)) {
    dma_deregister_pointer(ptr);
  }
  return ptr2;
}


char *strdup_wrapper(const char *s)
{
  char * ptr = strdup(s);
  if (ptr) {
    dma_register_pointer(ptr);
  }
  return ptr;
}

char *strndup_wrapper(const char *s, size_t n)
{
  char * ptr = strndup(s, n);
  if (ptr) {
    dma_register_pointer(ptr);
  }
  return ptr;
}

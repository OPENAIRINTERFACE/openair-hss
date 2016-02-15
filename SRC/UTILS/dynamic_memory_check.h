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

#ifndef DYNAMIC_MEMORY_CHECK_H_
#define DYNAMIC_MEMORY_CHECK_H_
#if MEMORY_CHECK
#include <stdlib.h>

void dyn_mem_check_init(void);
void dyn_mem_check_exit(void);
void *malloc_wrapper(size_t size)                 __attribute__ ((hot, warn_unused_result));
void free_wrapper(void *ptr)                      __attribute__ ((hot));
void *calloc_wrapper(size_t nmemb, size_t size)   __attribute__ ((hot, warn_unused_result));
void *realloc_wrapper(void *ptr, size_t size)     __attribute__ ((warn_unused_result));
char *strdup_wrapper(const char *s)               __attribute__ ((warn_unused_result));
char *strndup_wrapper(const char *s, size_t n)    __attribute__ ((warn_unused_result));

#define DYN_MEM_CHECK_INIT    dyn_mem_check_init
#define DYN_MEM_CHECK_EXIT    dyn_mem_check_exit
#define MALLOC_CHECK          malloc_wrapper
#define FREE_CHECK            free_wrapper
#define CALLOC_CHECK          calloc_wrapper
#define REALLOC_CHECK         realloc_wrapper
#define STRDUP_CHECK          strdup_wrapper
#define STRNDUP_CHECK         strndup_wrapper
#else
#include <stdlib.h>

void free_wrapper(void *ptr) __attribute__((hot));

#define DYN_MEM_CHECK_INIT()
#define DYN_MEM_CHECK_EXIT()
#define MALLOC_CHECK          malloc
#define FREE_CHECK            free_wrapper
#define CALLOC_CHECK          calloc
#define REALLOC_CHECK         realloc
#define STRDUP_CHECK          strdup
#define STRNDUP_CHECK         strndup
#endif
#endif /* DYNAMIC_MEMORY_CHECK_H_ */

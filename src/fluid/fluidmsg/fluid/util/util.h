/* Copyright (c) 2008, 2009 The Board of Trustees of The Leland Stanford
 * Junior University
 *
 * We are making the OpenFlow specification and associated documentation
 * (Software) available for public use and benefit with the expectation
 * that others will use, modify and enhance the Software and contribute
 * those enhancements back to the community. However, since we would
 * like to make the Software available for broadest use, with as few
 * restrictions as possible permission is hereby granted, free of
 * charge, to any person obtaining a copy of this Software to deal in
 * the Software under the copyrights without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * The name and trademarks of copyright holder(s) may NOT be used in
 * advertising or publicity pertaining to the Software or any
 * derivatives without specific, written prior permission.
 */

#ifndef UTIL_H
#define UTIL_H 1

#include <arpa/inet.h>
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#else
#define va_copy(dst, src) ((dst) = (src))
#endif
#endif

#ifndef __cplusplus
/* Build-time assertion for use in a statement context. */
#define BUILD_ASSERT(EXPR) \
  sizeof(struct { unsigned int build_assert_failed : (EXPR) ? 1 : -1; })

/* Build-time assertion for use in a declaration context. */
#define BUILD_ASSERT_DECL(EXPR) \
  extern int(*build_assert(void))[BUILD_ASSERT(EXPR)]
#else  /* __cplusplus */
#endif /* __cplusplus */

#define NO_RETURN __attribute__((__noreturn__))
#define UNUSED __attribute__((__unused__))
#define PACKED __attribute__((__packed__))
#define PRINTF_FORMAT(FMT, ARG1) __attribute__((__format__(printf, FMT, ARG1)))
#define STRFTIME_FORMAT(FMT) __attribute__((__format__(__strftime__, FMT, 0)))
#define MALLOC_LIKE __attribute__((__malloc__))
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#define ARRAY_SIZE(ARRAY) (sizeof ARRAY / sizeof *ARRAY)
#define ROUND_UP(X, Y) (((X) + ((Y)-1)) / (Y) * (Y))
#define ROUND_DOWN(X, Y) ((X) / (Y) * (Y))
#define IS_POW2(X) ((X) && !((X) & ((X)-1)))

#ifndef MIN
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

#ifndef MAX
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#endif

#define NOT_REACHED() abort()
#define NOT_IMPLEMENTED() abort()
#define NOT_TESTED() ((void)0) /* XXX should print a message. */

/* Given POINTER, the address of the given MEMBER in a STRUCT object, returns
   the STRUCT object. */
#define CONTAINER_OF(POINTER, STRUCT, MEMBER) \
  ((STRUCT*)((char*)(POINTER)-offsetof(STRUCT, MEMBER)))

/* Check endianness on OS X. */
#ifndef __BYTE_ORDER
#define __BYTE_ORDER __BYTE_ORDER__
#endif
#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN __ORDER_BIG_ENDIAN__
#endif
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline uint16_t hton16(uint16_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
  return n;
#else
  return htons(n);
#endif
}

static inline uint16_t ntoh16(uint16_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
  return n;
#else
  return ntohs(n);
#endif
}

static inline uint32_t hton32(uint32_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
  return n;
#else
  return htonl(n);
#endif
}

static inline uint32_t ntoh32(uint32_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
  return n;
#else
  return ntohl(n);
#endif
}

static inline uint64_t hton64(uint64_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
  return n;
#else
  return (((uint64_t)hton32(n)) << 32) + hton32(n >> 32);
#endif
}

static inline uint64_t ntoh64(uint64_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
  return n;
#else
  return (((uint64_t)ntoh32(n)) << 32) + ntoh32(n >> 32);
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* util.h */

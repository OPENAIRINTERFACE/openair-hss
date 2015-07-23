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

/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * General Public License for more details.
 *
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <stdarg.h>

#ifndef __attribute__
#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#define __attribute__(Spec)     /* empty */
#endif
#endif

extern int debug;
extern int error_count;
extern int break_on_error;

extern uint8_t * decode_hex_dup(const char *hex);
extern int decode_hex(uint8_t *dst, const char *h);
extern unsigned decode_hex_length(const char *h);

#define H(x)  decode_hex_dup(x)
#define HL(x) decode_hex_dup(x), decode_hex_length(x)

extern void fail (const char *format, ...)
__attribute__ ((format (printf, 1, 2)));
extern void success (const char *format, ...)
__attribute__ ((format (printf, 1, 2)));

extern void escapeprint (const char *str, size_t len);
extern void hexprint (const void *str, size_t len);
extern void binprint (const void *str, size_t len);

extern int
compare_buffer(uint8_t *buffer,  uint32_t length_buffer,
               uint8_t *pattern, uint32_t length_pattern);

/* This must be implemented elsewhere. */
extern void doit (void);

#endif /* UTILS_H */

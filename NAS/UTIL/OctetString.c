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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "OctetString.h"
#include "dynamic_memory_check.h"

#define DUMP_OUTPUT_SIZE 1024
static char                             _dump_output[DUMP_OUTPUT_SIZE];

OctetString                            *
dup_octet_string (
  OctetString * octetstring)
{
  OctetString                            *os_p = NULL;

  if (octetstring) {
    os_p = CALLOC_CHECK (1, sizeof (OctetString));
    os_p->length = octetstring->length;
    os_p->value = MALLOC_CHECK (octetstring->length + 1);
    memcpy (os_p->value, octetstring->value, octetstring->length);
    os_p->value[octetstring->length] = '\0';
  }

  return os_p;
}


void
free_octet_string (
  OctetString * octetstring)
{
  if (octetstring) {
    if (octetstring->value)
      FREE_CHECK (octetstring->value);

    octetstring->value = NULL;
    octetstring->length = 0;
    FREE_CHECK (octetstring);
  }
}


int
encode_octet_string (
  OctetString * octetstring,
  uint8_t * buffer,
  uint32_t buflen)
{
  if (octetstring != NULL) {
    if ((octetstring->value != NULL) && (octetstring->length > 0)) {
      CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, octetstring->length, buflen);
      memcpy ((void *)buffer, (void *)octetstring->value, octetstring->length);
      return octetstring->length;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

int
decode_octet_string (
  OctetString * octetstring,
  uint16_t pdulen,
  uint8_t * buffer,
  uint32_t buflen)
{
  if (buflen < pdulen)
    return -1;

  if ((octetstring != NULL) && (buffer != NULL)) {
    octetstring->length = pdulen;
    octetstring->value = MALLOC_CHECK (sizeof (uint8_t) * (pdulen + 1));
    memcpy ((void *)octetstring->value, (void *)buffer, pdulen);
    octetstring->value[pdulen] = '\0';
    return octetstring->length;
  } else {
    return -1;
  }
}

char                                   *
dump_octet_string_xml (
  const OctetString * const octetstring)
{
  int                                     i;
  int                                     remaining_size = DUMP_OUTPUT_SIZE;
  int                                     size = 0;
  int                                     size_print = 0;

  size_print = snprintf (_dump_output, remaining_size, "<Length>%u</Length>\n\t<values>", octetstring->length);
  size += size_print;
  remaining_size -= size_print;

  for (i = 0; i < octetstring->length; i++) {
    size_print = snprintf (&_dump_output[size], remaining_size, "0x%x ", octetstring->value[i]);
    size += size_print;
    remaining_size -= size_print;
  }

  size_print = snprintf (&_dump_output[size], remaining_size, "</values>\n");
  return _dump_output;
}

char                                   *
dump_octet_string (
  const OctetString * const octetstring)
{
  int                                     i;
  int                                     remaining_size = DUMP_OUTPUT_SIZE;
  int                                     size = 0;
  int                                     size_print = 0;

  for (i = 0; i < octetstring->length; i++) {
    size_print = snprintf (&_dump_output[size], remaining_size, "0x%x ", octetstring->value[i]);
    size += size_print;
    remaining_size -= size_print;
  }

  return _dump_output;
}

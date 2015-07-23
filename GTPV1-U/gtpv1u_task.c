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
/*! \file gtpv1u_task.c
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include "mme_config.h"

#include "assertions.h"
#include "intertask_interface.h"

#include "gtpv1u.h"
#include "gtpv1u_sgw_defs.h"
#include "msc.h"

static gtpv1u_data_t                    gtpv1u_sgw_data;

static void                            *gtpv1u_thread (
  void *args);

//-----------------------------------------------------------------------------
void
gtpu_print_hex_octets (
  unsigned char *dataP,
  unsigned long sizeP)
//-----------------------------------------------------------------------------
{
  unsigned long                           octet_index = 0;
  unsigned long                           buffer_marker = 0;
  unsigned char                           aindex;

#define GTPU_2_PRINT_BUFFER_LEN 8000
  char                                    gtpu_2_print_buffer[GTPU_2_PRINT_BUFFER_LEN];
  struct timeval                          tv;
  struct timezone                         tz;
  char                                    timeofday[64];
  unsigned int                            h,
                                          m,
                                          s;

  if (dataP == NULL) {
    return;
  }

  gettimeofday (&tv, &tz);
  h = tv.tv_sec / 3600 / 24;
  m = (tv.tv_sec / 60) % 60;
  s = tv.tv_sec % 60;
  snprintf (timeofday, 64, "%02d:%02d:%02d.%06d", h, m, s, tv.tv_usec);
  GTPU_DEBUG ("%s------+-------------------------------------------------|\n", timeofday);
  GTPU_DEBUG ("%s      |  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f |\n", timeofday);
  GTPU_DEBUG ("%s------+-------------------------------------------------|\n", timeofday);

  for (octet_index = 0; octet_index < sizeP; octet_index++) {
    if (GTPU_2_PRINT_BUFFER_LEN < (buffer_marker + 32)) {
      buffer_marker += snprintf (&gtpu_2_print_buffer[buffer_marker], GTPU_2_PRINT_BUFFER_LEN - buffer_marker, "... (print buffer overflow)");
      GTPU_DEBUG ("%s%s", timeofday, gtpu_2_print_buffer);
      return;
    }

    if ((octet_index % 16) == 0) {
      if (octet_index != 0) {
        buffer_marker += snprintf (&gtpu_2_print_buffer[buffer_marker], GTPU_2_PRINT_BUFFER_LEN - buffer_marker, " |\n");
        GTPU_DEBUG ("%s%s", timeofday, gtpu_2_print_buffer);
        buffer_marker = 0;
      }

      buffer_marker += snprintf (&gtpu_2_print_buffer[buffer_marker], GTPU_2_PRINT_BUFFER_LEN - buffer_marker, " %04ld |", octet_index);
    }

    /*
     * Print every single octet in hexadecimal form
     */
    buffer_marker += snprintf (&gtpu_2_print_buffer[buffer_marker], GTPU_2_PRINT_BUFFER_LEN - buffer_marker, " %02x", dataP[octet_index]);
    /*
     * Align newline and pipes according to the octets in groups of 2
     */
  }

  /*
   * Append enough spaces and put final pipe
   */
  for (aindex = octet_index; aindex < 16; ++aindex)
    buffer_marker += snprintf (&gtpu_2_print_buffer[buffer_marker], GTPU_2_PRINT_BUFFER_LEN - buffer_marker, "   ");

  //GTPU_DEBUG("   ");
  buffer_marker += snprintf (&gtpu_2_print_buffer[buffer_marker], GTPU_2_PRINT_BUFFER_LEN - buffer_marker, " |\n");
  GTPU_DEBUG ("%s%s", timeofday, gtpu_2_print_buffer);
}


int
gtpv1u_send_udp_msg (
  uint8_t * buffer,
  uint32_t buffer_len,
  uint32_t buffer_offset,
  uint32_t peerIpAddr,
  uint32_t peerPort)
{
  // Create and alloc new message
  MessageDef                             *message_p;
  udp_data_req_t                         *udp_data_req_p;

  message_p = itti_alloc_new_message (TASK_GTPV1_U, UDP_DATA_REQ);
  udp_data_req_p = &message_p->ittiMsg.udp_data_req;
  udp_data_req_p->peer_address = peerIpAddr;
  udp_data_req_p->peer_port = peerPort;
  udp_data_req_p->buffer = buffer;
  udp_data_req_p->buffer_length = buffer_len;
  udp_data_req_p->buffer_offset = buffer_offset;
  return itti_send_msg_to_task (TASK_UDP, INSTANCE_DEFAULT, message_p);
}


static void                            *
gtpv1u_thread (
  void *args)
{
  itti_mark_task_ready (TASK_GTPV1_U);
  MSC_START_USE ();

  while (1) {
    /*
     * Trying to fetch a message from the message queue.
     * * * * If the queue is empty, this function will block till a
     * * * * message is sent to the task.
     */
    MessageDef                             *received_message_p = NULL;

    itti_receive_msg (TASK_GTPV1_U, &received_message_p);
    DevAssert (received_message_p != NULL);

    switch (ITTI_MSG_ID (received_message_p)) {
    case TERMINATE_MESSAGE:{
        itti_exit_task ();
      }
      break;


    default:{
        GTPU_ERROR ("Unkwnon message ID %d:%s\n", ITTI_MSG_ID (received_message_p), ITTI_MSG_NAME (received_message_p));
      }
      break;
    }

    itti_free (ITTI_MSG_ORIGIN_ID (received_message_p), received_message_p);
    received_message_p = NULL;
  }

  return NULL;
}

int
gtpv1u_init (
  const mme_config_t * mme_config_p)
{
  GTPU_DEBUG ("Initializing GTPV1U interface\n");
  memset (&gtpv1u_sgw_data, 0, sizeof (gtpv1u_sgw_data));
  gtpv1u_sgw_data.sgw_ip_address_for_S1u_S12_S4_up = mme_config_p->ipv4.sgw_ip_address_for_S1u_S12_S4_up;

  if (itti_create_task (TASK_GTPV1_U, &gtpv1u_thread, NULL) < 0) {
    GTPU_ERROR ("gtpv1u phtread_create: %s", strerror (errno));
    return -1;
  }

  GTPU_DEBUG ("Initializing GTPV1U interface: DONE\n");
  return 0;
}

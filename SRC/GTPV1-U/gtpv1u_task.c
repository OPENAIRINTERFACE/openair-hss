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
#include "log.h"

static gtpv1u_data_t                    gtpv1u_sgw_data;

static void                            *gtpv1u_thread (
  void *args);

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
  OAILOG_START_USE ();
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
        OAILOG_ERROR (LOG_GTPV1U , "Unkwnon message ID %d:%s\n", ITTI_MSG_ID (received_message_p), ITTI_MSG_NAME (received_message_p));
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
  OAILOG_DEBUG (LOG_GTPV1U , "Initializing GTPV1U interface\n");
  memset (&gtpv1u_sgw_data, 0, sizeof (gtpv1u_sgw_data));
  gtpv1u_sgw_data.sgw_ip_address_for_S1u_S12_S4_up = mme_config_p->ipv4.sgw_ip_address_for_s1u_s12_s4_up;

  if (itti_create_task (TASK_GTPV1_U, &gtpv1u_thread, NULL) < 0) {
    OAILOG_ERROR (LOG_GTPV1U , "gtpv1u phtread_create: %s", strerror (errno));
    return -1;
  }

  OAILOG_DEBUG (LOG_GTPV1U , "Initializing GTPV1U interface: DONE\n");
  return 0;
}

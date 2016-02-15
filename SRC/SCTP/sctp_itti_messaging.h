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

#ifndef SCTP_ITTI_MESSAGING_H_
#define SCTP_ITTI_MESSAGING_H_

int sctp_itti_send_new_association(uint32_t assoc_id, uint16_t instreams,
                                   uint16_t outstreams);

int sctp_itti_send_new_message_ind(int n, uint8_t *buffer, uint32_t assoc_id,
                                   uint16_t stream,
                                   uint16_t instreams, uint16_t outstreams);

int sctp_itti_send_com_down_ind(uint32_t assoc_id);

#endif /* SCTP_ITTI_MESSAGING_H_ */

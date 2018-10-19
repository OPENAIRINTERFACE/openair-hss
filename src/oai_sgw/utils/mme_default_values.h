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

#ifndef FILE_MME_DEFAULT_VALUES_SEEN
#define FILE_MME_DEFAULT_VALUES_SEEN

/*******************************************************************************
 * Timer Constants
 ******************************************************************************/
#define MME_STATISTIC_TIMER_S  (60)

/*******************************************************************************
 * GTPV1 User Plane Constants
 ******************************************************************************/
#define GTPV1_U_PORT_NUMBER    (2152)

/*******************************************************************************
 * S1AP Constants
 ******************************************************************************/

#define S1AP_PORT_NUMBER (36412) ///< S1AP SCTP IANA ASSIGNED Port Number
#define S1AP_SCTP_PPID   (18)    ///< S1AP SCTP Payload Protocol Identifier (PPID)

#define S1AP_OUTCOME_TIMER_DEFAULT (5)     ///< S1AP Outcome drop timer (s)

/*******************************************************************************
 * S6A Constants
 ******************************************************************************/

#define S6A_CONF_FILE "../S6A/freediameter/s6a.conf"

/*******************************************************************************
 * SCTP Constants
 ******************************************************************************/

#define SCTP_RECV_BUFFER_SIZE (1 << 16)
#define SCTP_OUT_STREAMS      (32)
#define SCTP_IN_STREAMS       (32)
#define SCTP_MAX_ATTEMPTS     (5)

/*******************************************************************************
 * MME global definitions
 ******************************************************************************/

#define MMEC                    (0)
#define MMEGID                  (0)
#define PLMN_MCC                (208)
#define PLMN_MNC                (34)
#define PLMN_MNC_LEN            (2)
#define PLMN_TAC                (1)

#define RELATIVE_CAPACITY       (15)


#endif /* FILE_MME_DEFAULT_VALUES_SEEN */

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
/*! \file gtpv1u.h
* \brief
* \author Sebastien ROUX, Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#ifndef GTPV1_U_H_
#define GTPV1_U_H_

/* When gtpv1u is compiled for eNB use MACRO from UTILS/log.h,
 * otherwise use standard fprintf as logger.
 */

# define GTPU_DEBUG(x, args...)   fprintf(stdout, "[GTPU][D]"x, ##args)
# define GTPU_INFO(x, args...)    fprintf(stdout, "[GTPU][I]"x, ##args)
# define GTPU_WARNING(x, args...) fprintf(stdout, "[GTPU][W]"x, ##args)
# define GTPU_ERROR(x, args...)   fprintf(stderr, "[GTPU][E]"x, ##args)

#warning "TO BE REFINED"
# define GTPU_HEADER_OVERHEAD_MAX 64

uint32_t gtpv1u_new_teid(void);

#endif /* GTPV1_U_H_ */

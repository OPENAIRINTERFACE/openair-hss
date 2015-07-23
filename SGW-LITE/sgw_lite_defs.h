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

/*! \file sgw_lite_defs.h
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#ifndef SGW_LITE_DEFS_H_
#define SGW_LITE_DEFS_H_



#if defined(ENB_MODE)
# include "UTIL/LOG/log.h"
# define SPGW_APP_ERROR(x, args...) LOG_E(SPGW, x, ##args)
# define SPGW_APP_WARN(x, args...)  LOG_W(SPGW, x, ##args)
# define SPGW_APP_TRACE(x, args...)  LOG_T(SPGW, x, ##args)
# define SPGW_APP_INFO(x, args...) LOG_I(SPGW, x, ##args)
# define SPGW_APP_DEBUG(x, args...) LOG_I(SPGW, x, ##args)
#else
# define SPGW_APP_ERROR(x, args...) do { fprintf(stdout, "[SPGW-APP][E]"x, ##args); } while(0)
# define SPGW_APP_WARN(x, args...)  do { fprintf(stdout, "[SPGW-APP][W]"x, ##args); } while(0)
# define SPGW_APP_TRACE(x, args...)  do { fprintf(stdout, "[SPGW-APP][T]"x, ##args); } while(0)
# define SPGW_APP_INFO(x, args...) do { fprintf(stdout, "[SPGW-APP][I]"x, ##args); } while(0)
# define SPGW_APP_DEBUG(x, args...) do { fprintf(stdout, "[SPGW-APP][D]"x, ##args); } while(0)
#endif


int sgw_lite_init(char* config_file_name_pP);

#endif /* SGW_LITE_DEFS_H_ */

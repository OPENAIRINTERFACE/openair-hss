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


#include <stdlib.h>
#include <stdio.h>

#include "intertask_interface.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "mme_app_statistics.h"
#include "log.h"

int
mme_app_statistics_display (
  void)
{
  OAILOG_DEBUG (LOG_MME_APP, "================== Statistics ==================\n");
  OAILOG_DEBUG (LOG_MME_APP, "        |   Global   | Since last display |\n");
  OAILOG_DEBUG (LOG_MME_APP, "UE      | %10u |     %10u     |\n", mme_app_desc.mme_ue_contexts.nb_ue_managed, mme_app_desc.mme_ue_contexts.nb_ue_since_last_stat);
  OAILOG_DEBUG (LOG_MME_APP, "Bearers | %10u |     %10u     |\n", mme_app_desc.mme_ue_contexts.nb_bearers_managed, mme_app_desc.mme_ue_contexts.nb_bearers_since_last_stat);
  mme_app_desc.mme_ue_contexts.nb_ue_since_last_stat = 0;
  mme_app_desc.mme_ue_contexts.nb_bearers_since_last_stat = 0;
  return 0;
}

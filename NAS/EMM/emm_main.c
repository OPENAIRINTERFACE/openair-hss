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

/*****************************************************************************
  Source      emm_main.c

  Version     0.1

  Date        2012/10/10

  Product     NAS stack

  Subsystem   EPS Mobility Management

  Author      Frederic Maurel

  Description Defines the EPS Mobility Management procedure call manager,
        the main entry point for elementary EMM processing.

*****************************************************************************/

#include "emm_main.h"
#include "nas_log.h"
#include "emmData.h"


#if NAS_BUILT_IN_EPC
#  include "mme_config.h"
#  include "obj_hashtable.h"
#endif
#include <string.h>


/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/


/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_initialize()                                     **
 **                                                                        **
 ** Description: Initializes EMM internal data                             **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
#if NAS_BUILT_IN_EPC
void
emm_main_initialize (
  mme_config_t * mme_config_p)
#else
void
emm_main_initialize (
  void)
#endif
{
  LOG_FUNC_IN;
  /*
   * Retreive MME supported configuration data
   */
#if NAS_BUILT_IN_EPC
  memset(&_emm_data.conf, 0, sizeof(_emm_data.conf));
  if (mme_api_get_emm_config (&_emm_data.conf, mme_config_p) != RETURNok)
#else
  if (mme_api_get_emm_config (&_emm_data.conf) != RETURNok)
#endif
  {
    LOG_TRACE (ERROR, "EMM-MAIN  - Failed to get MME configuration data");
  }
#if NAS_BUILT_IN_EPC
  _emm_data.ctx_coll_ue_id = hashtable_create (MAX_NUMBER_OF_UE * 2, NULL, NULL, "emm_data.ctx_coll_ue_id");
  _emm_data.ctx_coll_guti = obj_hashtable_create (MAX_NUMBER_OF_UE * 2, NULL, NULL, NULL, "emm_data.ctx_coll_guti");
#endif
  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_cleanup()                                        **
 **                                                                        **
 ** Description: Performs the EPS Mobility Management clean up procedure   **
 **                                                                        **
 ** Inputs:  None                                                      **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **          Return:    None                                       **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void
emm_main_cleanup (
  void)
{
  LOG_FUNC_IN;
  LOG_FUNC_OUT;
}



/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

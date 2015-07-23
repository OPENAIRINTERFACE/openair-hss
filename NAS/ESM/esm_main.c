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
  Source      esm_main.c

  Version     0.1

  Date        2012/12/04

  Product     NAS stack

  Subsystem   EPS Session Management

  Author      Frederic Maurel

  Description Defines the EPS Session Management procedure call manager,
        the main entry point for elementary ESM processing.

*****************************************************************************/

#include "esm_main.h"
#include "commonDef.h"
#include "nas_log.h"

#include "emmData.h"
#include "esmData.h"
#include "esm_pt.h"
#include "esm_ebr.h"

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
 ** Name:    esm_main_initialize()                                     **
 **                                                                        **
 ** Description: Initializes ESM internal data                             **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void
esm_main_initialize (
  void)
{
  int                                     i;

  LOG_FUNC_IN;

  /*
   * Retreive MME supported configuration data
   */
  if (mme_api_get_esm_config (&_esm_data.conf) != RETURNok) {
    LOG_TRACE (ERROR, "ESM-MAIN  - Failed to get MME configuration data");
  }
#if !defined(NAS_BUILT_IN_EPC)

  /*
   * Initialize ESM contexts
   */
  for (i = 0; i < ESM_DATA_NB_UE_MAX; i++) {
    _esm_data.ctx[i] = NULL;
  }

#endif
  /*
   * Initialize the EPS bearer context manager
   */
  esm_ebr_initialize ();
  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:        esm_main_cleanup()                                        **
 **                                                                        **
 ** Description: Performs the EPS Session Management clean up procedure    **
 **                                                                        **
 ** Inputs:      None                                                      **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    None                                       **
 **                  Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void
esm_main_cleanup (
  void)
{
  LOG_FUNC_IN;
  LOG_FUNC_OUT;
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

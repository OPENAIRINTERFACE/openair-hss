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

#include "assertions.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "log.h"
#include "mme_config.h"
#include "msc.h"
#include "s6a_defs.h"
#include "s6a_messages.h"

int s6a_add_result_code_mme(struct msg *ans, struct avp *failed_avp,
                            int result_code, int experimental) {
  struct avp *avp;
  union avp_value value;

  if (DIAMETER_ERROR_IS_VENDOR(result_code) && experimental != 0) {
    struct avp *experimental_result;

    CHECK_FCT(fd_msg_avp_new(s6a_fd_cnf.dataobj_s6a_experimental_result, 0,
                             &experimental_result));
    CHECK_FCT(fd_msg_avp_new(s6a_fd_cnf.dataobj_s6a_vendor, 0, &avp));
    value.u32 = VENDOR_3GPP;
    CHECK_FCT(fd_msg_avp_setvalue(avp, &value));
    CHECK_FCT(fd_msg_avp_add(experimental_result, MSG_BRW_LAST_CHILD, avp));
    CHECK_FCT(
        fd_msg_avp_new(s6a_fd_cnf.dataobj_s6a_experimental_result, 0, &avp));
    value.u32 = result_code;
    CHECK_FCT(fd_msg_avp_setvalue(avp, &value));
    CHECK_FCT(fd_msg_avp_add(experimental_result, MSG_BRW_LAST_CHILD, avp));
    CHECK_FCT(fd_msg_avp_add(ans, MSG_BRW_LAST_CHILD, experimental_result));
    /*
     * Add Origin_Host & Origin_Realm AVPs
     */
    CHECK_FCT(fd_msg_add_origin(ans, 0));
  } else {
    /*
     * This is a code defined in the base protocol: result-code AVP should
     * * * * be used.
     */
    CHECK_FCT(fd_msg_rescode_set(ans, retcode_2_string(result_code), NULL,
                                 failed_avp, 1));
  }

  return 0;
}

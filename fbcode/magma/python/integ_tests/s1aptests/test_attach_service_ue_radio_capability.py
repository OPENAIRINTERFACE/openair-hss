"""
Copyright (c) 2016-present, Facebook, Inc.
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree. An additional grant
of patent rights can be found in the PATENTS file in the same directory.
"""

import unittest

import gpp_types
import s1ap_types
import test_wrapper


class TestAttachServiceUeRadioCapability(unittest.TestCase):

    def setUp(self):
        self._test_wrapper = test_wrapper.TestWrapper()

    def tearDown(self):
        self._test_wrapper.cleanup()

    def test_attach_service_ue_radio_capability(self):
        """ Test with a single UE attach, UE context release, service request,
        check for UE radio capability """
        self._test_wrapper.configUEDevice(1)
        req = self._test_wrapper.ue_req
        ue_id = req.ue_id
        print("************************* Running End to End attach for UE id ",
              ue_id)
        # Now actually complete the attach
        self._test_wrapper._s1_util.attach(
            ue_id, s1ap_types.tfwCmd.UE_END_TO_END_ATTACH_REQUEST,
            s1ap_types.tfwCmd.UE_ATTACH_ACCEPT_IND)

        print("************************* Sending UE context release request ",
              "for UE id ", ue_id)
        # Send UE context release request to move UE to idle mode
        req = s1ap_types.ueCntxtRelReq_t()
        req.ue_Id = ue_id
        req.cause = gpp_types.CauseRadioNetwork.USER_INACTIVITY.value
        self._test_wrapper.s1_util.issue_cmd(
            s1ap_types.tfwCmd.UE_CNTXT_REL_REQUEST, req)
        response = self._test_wrapper.s1_util.get_response()
        self.assertEqual(
            response.msg_type, s1ap_types.tfwCmd.UE_CTX_REL_IND.value)

        print("************************* Sending Service request for UE id ",
              ue_id)
        # Send service request to reconnect UE
        req = s1ap_types.ueserviceReq_t()
        req.ue_Id = ue_id
        req.ueMtmsi = s1ap_types.ueMtmsi_t()
        req.ueMtmsi.pres = False
        req.rrcCause = s1ap_types.Rrc_Cause.TFW_MO_DATA.value
        self._test_wrapper.s1_util.issue_cmd(
            s1ap_types.tfwCmd.UE_SERVICE_REQUEST, req)
        response = self._test_wrapper.s1_util.get_response()
        self.assertEqual(
            response.msg_type, s1ap_types.tfwCmd.INT_CTX_SETUP_IND.value)

        print("************************* Running UE detach for UE id ",
              ue_id)
        # Now detach the UE
        self._test_wrapper.s1_util.detach(
            ue_id, s1ap_types.ueDetachType_t.UE_SWITCHOFF_DETACH.value, False)


if __name__ == "__main__":
    unittest.main()

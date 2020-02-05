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

#include <stdio.h>

#include "BaseApplication.h"
#if SERVICE303
#include "service303.h"
#endif

extern "C" {
#include "log.h"
}

using namespace fluid_msg;
namespace openflow {

void BaseApplication::event_callback(const ControllerEvent& ev,
                                     const OpenflowMessenger& messenger) {
  if (ev.get_type() == EVENT_SWITCH_UP) {
    remove_all_flows(ev.get_connection(), messenger);
    install_default_flow(ev.get_connection(), messenger);
  } else if (ev.get_type() == EVENT_ERROR) {
    handle_error_message(static_cast<const ErrorEvent&>(ev));
  }
}

void BaseApplication::install_default_flow(fluid_base::OFConnection* ofconn,
                                           const OpenflowMessenger& messenger) {
  of13::FlowMod fm = messenger.create_default_flow_mod(
      OF_TABLE_SWITCH, of13::OFPFC_ADD, OF_PRIO_SWITCH_LOWER_PRIORITY);
  // Output to next table
  of13::GoToTable inst(OF_TABLE_SGI_OUT);
  fm.add_instruction(inst);
  OAILOG_INFO(LOG_GTPV1U, "Setting default switch flow for Base Application\n");
  messenger.send_of_msg(fm, ofconn);

  of13::FlowMod fml = messenger.create_default_flow_mod(
      OF_TABLE_LOOP, of13::OFPFC_ADD, OF_PRIO_SWITCH_LOWER_PRIORITY);
  // Output to next table
  fml.add_instruction(inst);
  OAILOG_INFO(LOG_GTPV1U, "Setting default loop flow for Base Application\n");
  messenger.send_of_msg(fml, ofconn);
}

void BaseApplication::remove_all_flows(fluid_base::OFConnection* ofconn,
                                       const OpenflowMessenger& messenger) {
  of13::FlowMod fm = messenger.create_default_flow_mod(
      OF_TABLE_SWITCH, of13::OFPFC_DELETE, OF_PRIO_SWITCH_LOWER_PRIORITY);
  // match all
  fm.out_port(of13::OFPP_ANY);
  fm.out_group(of13::OFPG_ANY);
  messenger.send_of_msg(fm, ofconn);
  return;
}

void BaseApplication::handle_error_message(const ErrorEvent& ev) {
  static std::vector<std::string> error_type{
      std::string("OFPET_HELLO_FAILED"),
      std::string("OFPET_BAD_REQUEST"),
      std::string("OFPET_BAD_ACTION"),
      std::string("OFPET_BAD_INSTRUCTION"),
      std::string("OFPET_BAD_MATCH"),
      std::string("OFPET_FLOW_MOD_FAILED"),
      std::string("OFPET_GROUP_MOD_FAILED"),
      std::string("OFPET_PORT_MOD_FAILED"),
      std::string("OFPET_TABLE_MOD_FAILED"),
      std::string("OFPET_QUEUE_OP_FAILED"),
      std::string("OFPET_SWITCH_CONFIG_FAILED"),
      std::string("OFPET_ROLE_REQUEST_FAILED"),
      std::string("OFPET_METER_MOD_FAILED"),
      std::string("OFPET_TABLE_FEATURES_FAILED")};

  static std::vector<std::string> hello_failed_code = {
      std::string("OFPHFC_INCOMPATIBLE"), std::string("OFPHFC_EPERM")};

  static std::vector<std::string> bad_request_code = {
      std::string("OFPBRC_BAD_VERSION"),
      std::string("OFPBRC_BAD_TYPE"),
      std::string("OFPBRC_BAD_STAT"),
      std::string("OFPBRC_BAD_VENDOR"),
      std::string("OFPBRC_BAD_SUBTYPE"),
      std::string("OFPBRC_EPERM"),
      std::string("OFPBRC_BAD_LEN"),
      std::string("OFPBRC_BUFFER_EMPTY"),
      std::string("OFPBRC_BUFFER_UNKNOWN"),
      std::string("OFPBRC_BAD_TABLE_ID"),
      std::string("OFPBRC_IS_SLAVE"),
      std::string("OFPBRC_BAD_PORT"),
      std::string("OFPBRC_BAD_PACKET"),
      std::string("OFPBRC_MULTIPART_BUFFER_OVERFLOW")};

  static std::vector<std::string> bad_action_code = {
      std::string("OFPBAC_BAD_TYPE"),
      std::string("OFPBAC_BAD_LEN"),
      std::string("OFPBAC_BAD_VENDOR"),
      std::string("OFPBAC_BAD_VENDOR_TYPE"),
      std::string("OFPBAC_BAD_OUT_PORT"),
      std::string("OFPBAC_BAD_ARGUMENT"),
      std::string("OFPBAC_EPERM"),
      std::string("OFPBAC_TOO_MANY"),
      std::string("OFPBAC_BAD_QUEUE"),
      std::string("OFPBAC_BAD_OUT_GROUP"),
      std::string("OFPBAC_MATCH_INCONSISTENT"),
      std::string("OFPBAC_UNSUPPORTED_ORDER"),
      std::string("OFPBAC_BAD_TAG"),
      std::string("OFPBAC_BAD_SET_TYPE"),
      std::string("OFPBAC_BAD_SET_LEN"),
      std::string("OFPBAC_BAD_SET_ARGUMENT")};

  static std::vector<std::string> bad_instruction_code = {
      std::string("OFPBIC_UNKNOWN_INST"),
      std::string("OFPBIC_UNSUP_INST"),
      std::string("OFPBIC_BAD_TABLE_ID"),
      std::string("OFPBIC_UNSUP_METADATA"),
      std::string("OFPBIC_UNSUP_METADATA_MASK"),
      std::string("OFPBIC_BAD_EXPERIMENTER"),
      std::string("OFPBIC_BAD_EXP_TYPE"),
      std::string("OFPBIC_BAD_LEN"),
      std::string("OFPBIC_EPERM")};

  static std::vector<std::string> bad_match_code = {
      std::string("OFPBMC_BAD_TYPE"),
      std::string("OFPBMC_BAD_LEN"),
      std::string("OFPBMC_BAD_TAG"),
      std::string("OFPBMC_BAD_DL_ADDR_MASK"),
      std::string("OFPBMC_BAD_NW_ADDR_MASK"),
      std::string("OFPBMC_BAD_WILDCARDS"),
      std::string("OFPBMC_BAD_FIELD"),
      std::string("OFPBMC_BAD_VALUE"),
      std::string("OFPBMC_BAD_MASK"),
      std::string("OFPBMC_BAD_PREREQ"),
      std::string("OFPBMC_DUP_FIELD"),
      std::string("OFPBMC_EPERM")};

  static std::vector<std::string> flow_mod_failed_code = {
      std::string("OFPFMFC_UNKNOWN"),      std::string("OFPFMFC_TABLE_FULL"),
      std::string("OFPFMFC_BAD_TABLE_ID"), std::string("OFPFMFC_OVERLAP"),
      std::string("OFPFMFC_EPERM"),        std::string("OFPFMFC_BAD_TIMEOUT"),
      std::string("OFPFMFC_BAD_COMMAND"),  std::string("OFPFMFC_BAD_FLAGS")};

  static std::vector<std::string> group_mod_failed_code = {
      std::string("OFPGMFC_GROUP_EXISTS"),
      std::string("OFPGMFC_INVALID_GROUP"),
      std::string("OFPGMFC_UNKNOWN2"),
      std::string("OFPGMFC_OUT_OF_GROUPS"),
      std::string("OFPGMFC_OUT_OF_BUCKETS"),
      std::string("OFPGMFC_CHAINING_UNSUPPORTED"),
      std::string("OFPGMFC_WATCH_UNSUPPORTED"),
      std::string("OFPGMFC_LOOP"),
      std::string("OFPGMFC_UNKNOWN_GROUP"),
      std::string("OFPGMFC_CHAINED_GROUP"),
      std::string("OFPGMFC_BAD_TYPE"),
      std::string("OFPGMFC_BAD_COMMAND"),
      std::string("OFPGMFC_BAD_BUCKET"),
      std::string("OFPGMFC_BAD_WATCH"),
      std::string("OFPGMFC_EPERM")};

  static std::vector<std::string> port_mod_failed_code = {
      std::string("OFPPMFC_BAD_PORT"), std::string("OFPPMFC_BAD_HW_ADDR"),
      std::string("OFPPMFC_BAD_CONFIG"), std::string("OFPPMFC_BAD_ADVERTISE"),
      std::string("OFPPMFC_EPERM")};

  static std::vector<std::string> table_mod_failed_code = {
      std::string("OFPTMFC_BAD_TABLE"), std::string("OFPTMFC_BAD_CONFIG"),
      std::string("OFPTMFC_EPERM")};

  static std::vector<std::string> queue_op_failed_code = {
      std::string("OFPQOFC_BAD_PORT"), std::string("OFPQOFC_BAD_QUEUE"),
      std::string("OFPQOFC_EPERM")};

  static std::vector<std::string> switch_config_failed_code = {
      std::string("OFPSCFC_BAD_FLAGS"), std::string("OFPSCFC_BAD_LEN"),
      std::string("OFPQCFC_EPERM")};

  static std::vector<std::string> role_request_failed_code = {
      std::string("OFPRRFC_STALE"),
      std::string("OFPRRFC_UNSUP"),
      std::string("OFPRRFC_BAD_ROLE"),
  };

  static std::vector<std::string> meter_mod_failed_code = {
      std::string("OFPMMFC_UNKNOWN"),
      std::string("OFPMMFC_METER_EXISTS"),
      std::string("OFPMMFC_INVALID_METER"),
      std::string("OFPMMFC_UNKNOWN_METER"),
      std::string("OFPMMFC_BAD_COMMAND"),
      std::string("OFPMMFC_BAD_FLAGS"),
      std::string("OFPMMFC_BAD_RATE"),
      std::string("OFPMMFC_BAD_BURST"),
      std::string("OFPMMFC_BAD_BAND"),
      std::string("OFPMMFC_BAD_BAND_VALUE"),
      std::string("OFPMMFC_OUT_OF_METERS"),
      std::string("OFPMMFC_OUT_OF_BAND")};

  static std::vector<std::string> table_features_failed_code = {
      std::string("OFPTFFC_BAD_TABLE"),    std::string("OFPTFFC_BAD_METADATA"),
      std::string("OFPTFFC_BAD_TYPE"),     std::string("OFPTFFC_BAD_LEN"),
      std::string("OFPTFFC_BAD_ARGUMENT"), std::string("OFPTFFC_EPERM")};
  static std::vector<std::vector<std::string>> error_code = {
      hello_failed_code,
      bad_request_code,
      bad_action_code,
      bad_instruction_code,
      bad_match_code,
      flow_mod_failed_code,
      group_mod_failed_code,
      port_mod_failed_code,
      table_mod_failed_code,
      queue_op_failed_code,
      switch_config_failed_code,
      role_request_failed_code,
      meter_mod_failed_code,
      table_features_failed_code};

  // First 16 bits of error message are the type, second 16 bits are the code
  OAILOG_ERROR(LOG_GTPV1U,
               "Openflow error received - type: %s(0x%02x), code: %s(0x%02x)\n",
               error_type[ev.get_error_type()].c_str(), ev.get_error_type(),
               error_code[ev.get_error_type()][ev.get_error_code()].c_str(),
               ev.get_error_code());
#if SERVICE303
  char type_str[50];
  char code_str[50];
  sprintf(type_str, "0x%02x", ev.get_error_type());
  sprintf(code_str, "0x%02x", ev.get_error_code());
  increment_counter("openflow_error_msg", 1, 2, "error_type", type_str,
                    "error_code", code_str);
#endif
}
}  // namespace openflow

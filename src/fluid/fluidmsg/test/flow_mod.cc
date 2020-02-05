#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "constants.hh"

class FlowMod13 : public ::testing::Test {
 protected:
  virtual void SetUp() {
    src = of13::FlowMod(xid, cookie, cookie_mask, table_id, of13::OFPFC_ADD,
                        hard_timeout, idle_timeout, priority, OFP_NO_BUFFER,
                        of13::OFPP_ANY, of13::OFPG_ANY,
                        of13::OFPFF_SEND_FLOW_REM);
  }

  of13::GoToTable *go_to;
  of13::WriteMetadata *write_meta;
  of13::WriteActions *write_actions;
  of13::ApplyActions *apply_actions;
  of13::ClearActions *clear;
  of13::Meter *meter;
  of13::FlowMod src;
};

TEST_F(FlowMod13, AllOXMMatch) {
  /* Crazy match created just to test lots of actions
   * and instructions without match */
  write_actions = new of13::WriteActions();
  write_actions->add_action(new of13::GroupAction(1));
  write_actions->add_action(new of13::SetNWTTLAction(25));
  write_actions->add_action(
      new of13::SetFieldAction(new of13::EthSrc("00:00:00:00:00:02")));
  write_actions->add_action(
      new of13::SetFieldAction(new of13::EthDst("00:00:00:00:00:04")));
  write_actions->add_action(new of13::PopVLANAction());
  write_actions->add_action(new of13::PushMPLSAction(0x8848));
  go_to = new of13::GoToTable(2);
  meter = new of13::Meter(1);
  apply_actions = new of13::ApplyActions();
  apply_actions->add_action(new of13::PopPBBAction());
  apply_actions->add_action(new of13::SetMPLSTTLAction(50));
  apply_actions->add_action(new of13::CopyTTLOutAction());
  apply_actions->add_action(new of13::CopyTTLInAction());
  clear = new of13::ClearActions();
  src.add_instruction(write_actions);
  src.add_instruction(apply_actions);
  src.add_instruction(go_to);
  src.add_instruction(meter);
  src.add_instruction(clear);
  uint8_t *buffer = src.pack();
  of13::FlowMod dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST_F(FlowMod13, OXMMasked) {
  src.add_oxm_field(new of13::EthSrc("12:34:56:78:90", "ff:ff:ff:ff:ff:00"));
  src.add_oxm_field(new of13::EthDst("12:34:56:78:91", "ff:ff:ff:ff:ff:00"));
  src.add_oxm_field(new of13::EthType(0x800));
  src.add_oxm_field(new of13::IPv4Src("192.168.1.1", "255.255.255.0"));
  src.add_oxm_field(new of13::IPv4Dst("192.168.2.1", "255.255.255.0"));
  src.add_instruction(new of13::Meter(1));
  uint8_t *buffer = src.pack();
  of13::FlowMod dst;
  of_error err = dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST_F(FlowMod13, BAD_PRE_REQ) {
  src.add_oxm_field(new of13::IPv4Src("192.168.1.1"));
  uint8_t *buffer = src.pack();
  of13::FlowMod dst;
  of_error err = dst.unpack(buffer);
  ASSERT_EQ(openflow_error(of13::OFPET_BAD_MATCH, of13::OFPBMC_BAD_PREREQ),
            err);
  OFMsg::free_buffer(buffer);
  of13::FlowMod dst2;
  src.add_oxm_field(new of13::EthType(0x800));
  buffer = src.pack();
  err = dst2.unpack(buffer);
  ASSERT_EQ(0, err);
  OFMsg::free_buffer(buffer);
}

TEST_F(FlowMod13, DupSetFieldAction) {
  write_actions = new of13::WriteActions();
  write_actions->add_action(
      new of13::SetFieldAction(new of13::EthSrc("00:00:00:00:00:02")));
  uint16_t l1 = write_actions->length();
  write_actions->add_action(
      new of13::SetFieldAction(new of13::EthSrc("00:00:00:00:00:04")));

  uint16_t l2 = write_actions->length();

  // Action set size should not change
  ASSERT_EQ(write_actions->actions().action_set().size(), 1);

  // Inserting a duplicate field should not increase the size
  ASSERT_EQ(l1, l2);
}

TEST_F(FlowMod13, MultipleSetFieldAction) {
  write_actions = new of13::WriteActions();

  of13::SetFieldAction *src_action =
      new of13::SetFieldAction(new of13::EthSrc("00:00:00:00:00:02"));
  write_actions->add_action(src_action);

  uint16_t l1 = write_actions->length();

  of13::SetFieldAction *dst_action =
      new of13::SetFieldAction(new of13::EthDst("00:00:00:00:00:04"));
  write_actions->add_action(dst_action);

  uint16_t l2 = write_actions->length();

  // Primary set ordering should be the same for both actions
  ASSERT_EQ(src_action->set_order(), dst_action->set_order());
  // Secondary set ordering should not be the same
  ASSERT_NE(src_action->set_sub_order(), dst_action->set_sub_order());

  // Inserting a different field should increase the write action size
  ASSERT_GT(l2, l1);

  // Action set size should have both actions
  ASSERT_EQ(write_actions->actions().action_set().size(), 2);
}

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "constants.hh"

using namespace fluid_msg;

class MultipartFlow : public ::testing::Test {
 protected:
  virtual void SetUp() {
    req_match.add_oxm_field(new of13::InPort(1));
    req_match.add_oxm_field(new of13::EthSrc("00:00:00:00:00:01"));

    match1 = match2 = req_match;
    match1.add_oxm_field(new of13::IPv4Src("192.168.1.2"));
    match1.add_oxm_field(new of13::EthType(0x0800));
    match1.add_oxm_field(new of13::VLANVid(2));
    of13::OutputAction* act1 = new of13::OutputAction(1, send_len);
    of13::SetQueueAction* act12 = new of13::SetQueueAction(2);
    a1.add_action(act1);
    a1.add_action(act12);
    fs1 = of13::FlowStats(table_id, 300, 20, 1360, 0, 500, 0, cookie, 36, 1756);
    fs1.match(match1);
    of13::ApplyActions* inst1 = new of13::ApplyActions(a1);
    fs1.add_instruction(inst1);

    match2.add_oxm_field(new of13::EthType(0x8847));
    match2.add_oxm_field(new of13::MPLSLabel(21));
    of13::OutputAction* act2 = new of13::OutputAction(2, send_len);
    of13::PopMPLSAction* act21 = new of13::PopMPLSAction(0x8847);
    a2.add_action(act2);
    a2.add_action(act21);
    fs2 =
        of13::FlowStats(table_id, 659, 96, 2000, 100, 0, 0x0, cookie, 42, 2352);
    fs2.match(match2);
    of13::ApplyActions* inst2 = new of13::ApplyActions(a2);
    fs2.add_instruction(inst2);
  }
  // virtual void TearDown() {}
  of13::Match req_match;
  of13::FlowStats fs1;
  of13::FlowStats fs2;
  of13::Match match1;
  of13::Match match2;
  of13::InstructionSet i1;
  of13::InstructionSet i2;
  ActionList a1;
  ActionList a2;
};

class TableFeaturesTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    instructions =
        new of13::TableFeaturePropInstruction(of13::OFPTFPT_INSTRUCTIONS);
    instructions_miss =
        new of13::TableFeaturePropInstruction(of13::OFPTFPT_INSTRUCTIONS_MISS);
    write_actions =
        new of13::TableFeaturePropActions(of13::OFPTFPT_WRITE_ACTIONS);
    write_actions_miss =
        new of13::TableFeaturePropActions(of13::OFPTFPT_WRITE_ACTIONS_MISS);
    apply_actions =
        new of13::TableFeaturePropActions(of13::OFPTFPT_APPLY_ACTIONS);
    apply_actions_miss =
        new of13::TableFeaturePropActions(of13::OFPTFPT_APPLY_ACTIONS_MISS);
    static const uint32_t oxm_arr[] = {
        OXM_OF_IN_PORT,  OXM_OF_IN_PHY_PORT, OXM_OF_METADATA, OXM_OF_METADATA_W,
        OXM_OF_ETH_DST,  OXM_OF_ETH_DST_W,   OXM_OF_ETH_SRC,  OXM_OF_ETH_SRC_W,
        OXM_OF_ETH_TYPE, OXM_OF_IP_PROTO,    OXM_OF_IPV4_SRC, OXM_OF_IPV4_SRC_W,
        OXM_OF_IPV4_DST, OXM_OF_IPV4_DST_W,  OXM_OF_TCP_SRC,  OXM_OF_TCP_DST,
        OXM_OF_UDP_SRC,  OXM_OF_UDP_DST};
    std::vector<uint32_t> oxm_ids(
        oxm_arr, oxm_arr + sizeof(oxm_arr) / sizeof(oxm_arr[0]));
    match = new of13::TableFeaturePropOXM(of13::OFPTFPT_WILDCARDS, oxm_ids);
    wildcards = new of13::TableFeaturePropOXM(of13::OFPTFPT_WILDCARDS, oxm_ids);
    write_set_field =
        new of13::TableFeaturePropOXM(of13::OFPTFPT_WRITE_SETFIELD, oxm_ids);
    write_set_field_miss = new of13::TableFeaturePropOXM(
        of13::OFPTFPT_WRITE_SETFIELD_MISS, oxm_ids);
    apply_set_field =
        new of13::TableFeaturePropOXM(of13::OFPTFPT_APPLY_SETFIELD, oxm_ids);
    apply_set_field_miss = new of13::TableFeaturePropOXM(
        of13::OFPTFPT_APPLY_SETFIELD_MISS, oxm_ids);

    tf = of13::TableFeatures(0, "Table 0", 0xFFFFFFFFFFFFFFFF,
                             0xFFFFFFFFFFFFFFFF, 0x0, 65000);
    tf.add_table_prop(instructions);
    tf.add_table_prop(instructions_miss);
    tf.add_table_prop(write_actions);
    tf.add_table_prop(write_actions_miss);
    tf.add_table_prop(apply_actions);
    tf.add_table_prop(apply_actions_miss);
    tf.add_table_prop(match);
    tf.add_table_prop(wildcards);
    tf.add_table_prop(write_set_field);
    tf.add_table_prop(write_set_field_miss);
    tf.add_table_prop(apply_set_field);
    tf.add_table_prop(apply_set_field_miss);
  }

  virtual ~TableFeaturesTest() {}
  of13::TableFeatures tf;
  of13::TableFeaturePropInstruction* instructions;
  of13::TableFeaturePropInstruction* instructions_miss;
  of13::TableFeaturePropActions* write_actions;
  of13::TableFeaturePropActions* write_actions_miss;
  of13::TableFeaturePropActions* apply_actions;
  of13::TableFeaturePropActions* apply_actions_miss;
  of13::TableFeaturePropOXM* match;
  of13::TableFeaturePropOXM* wildcards;
  of13::TableFeaturePropOXM* write_set_field;
  of13::TableFeaturePropOXM* write_set_field_miss;
  of13::TableFeaturePropOXM* apply_set_field;
  of13::TableFeaturePropOXM* apply_set_field_miss;
  std::vector<of13::Instruction> instruction_ids;
  std::vector<Action> action_ids;
  std::vector<uint8_t> next_table_ids;
};

TEST(MultipartRequest, MultipartRequestDesc) {
  of13::MultipartRequestDesc src(xid, 0);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestDesc dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartReply, MultipartReplyDesc) {
  SwitchDesc desc("Fluid Test", "Not Switch", "OpenFlow Driver", "0000032",
                  "Just a test");
  of13::MultipartReplyDesc src(xid, of13::OFPMPF_REPLY_MORE, desc);
  uint8_t* buffer = src.pack();
  of13::MultipartReplyDesc dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST_F(MultipartFlow, MultipartRequestFlow) {
  of13::MultipartRequestFlow src(xid, 0, table_id, of13::OFPP_ANY,
                                 of13::OFPG_ANY, cookie, cookie_mask,
                                 req_match);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestFlow dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST_F(MultipartFlow, MultipartReplyFlow) {
  of13::MultipartReplyFlow src(xid, 0);
  src.add_flow_stats(fs1);
  src.add_flow_stats(fs2);
  uint8_t* buffer = src.pack();
  of13::MultipartReplyFlow dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST_F(MultipartFlow, MultipartRequestAggregateAllOXM) {
  of13::MultipartRequestAggregate src(xid, 0, table_id, of13::OFPP_ANY,
                                      of13::OFPG_ANY, cookie, cookie_mask);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestAggregate dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST_F(MultipartFlow, MultipartRequestAggregateMatch) {
  of13::MultipartRequestAggregate src(xid, 0, table_id, of13::OFPP_ANY,
                                      of13::OFPG_ANY, cookie, cookie_mask,
                                      req_match);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestAggregate dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartReply, MultipartReplyAggregate) {
  of13::MultipartReplyAggregate src(xid, 0, 78, 4368, 2);
  uint8_t* buffer = src.pack();
  of13::MultipartReplyAggregate dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartRequest, MultipartRequestTable) {
  of13::MultipartRequestTable src(xid, 0);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestTable dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartReply, MultipartReplyTable) {
  of13::TableStats stats(table_id, 2, 136, 78);
  of13::MultipartReplyTable src(xid, 0);
  src.add_table_stat(stats);
  uint8_t* buffer = src.pack();
  of13::MultipartReplyTable dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartRequest, MultipartRequestPortStats) {
  of13::MultipartRequestPortStats src(xid, 0, 1);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestPortStats dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartReply, MultipartReplyPortStats) {
  struct port_rx_tx_stats rx_tx = {64, 32, 3584, 1792, 0, 0};
  struct port_err_stats err = {2, 4, 0, 0, 0};
  of13::PortStats stats(1, rx_tx, err, 0, 300, 30);
  of13::MultipartReplyPortStats src(xid, 0);
  src.add_port_stat(stats);
  uint8_t* buffer = src.pack();
  of13::MultipartReplyPortStats dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartRequest, MultipartRequestQueue) {
  of13::MultipartRequestQueue src(xid, 0, 1, 22);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestQueue dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartReplyQueue, MultipartReplyQueue) {
  of13::QueueStats stats(1, 22, 1904, 34, 0, 300, 50);
  of13::MultipartReplyQueue src(xid, 0);
  src.add_queue_stat(stats);
  uint8_t* buffer = src.pack();
  of13::MultipartReplyQueue dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartRequest, MultipartRequestGroup) {
  of13::MultipartRequestGroup src(xid, 0, group_id);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestGroup dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartReply, MultipartReplyGroup) {
  of13::BucketStats bucket_stats(46, 2576);
  of13::GroupStats stat(group_id, 2, 46, 2576, 60, 34);
  stat.add_bucket_stat(bucket_stats);
  of13::MultipartReplyGroup src(xid, 0);
  src.add_group_stats(stat);
  uint8_t* buffer = src.pack();
  of13::MultipartReplyGroup dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartRequest, MultipartRequestGroupDesc) {
  of13::MultipartRequestGroupDesc src(xid, 0);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestGroupDesc dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartReply, MultipartReplyGroupDesc) {
  of13::OutputAction* act = new of13::OutputAction(1, send_len);
  of13::DecNWTTLAction* actw = new of13::DecNWTTLAction();
  of13::PushMPLSAction* actv = new of13::PushMPLSAction(0x8847);
  of13::Bucket b(0x0, 1, of13::OFPG_ANY);
  b.add_action(actw);
  b.add_action(act);
  b.add_action(actv);
  of13::GroupDesc desc(of13::OFPGT_FF, group_id);
  desc.add_bucket(b);
  of13::Bucket b2(0x0, of13::OFPP_ANY, of13::OFPG_ANY);
  of13::GroupAction* act2 = new of13::GroupAction(2);
  of13::PushVLANAction* actvlan = new of13::PushVLANAction(0x8100);
  of13::SetFieldAction* set =
      new of13::SetFieldAction(new of13::IPv4Src("192.168.1.1"));
  b2.add_action(act2);
  b2.add_action(actvlan);
  b2.add_action(set);
  desc.add_bucket(b2);
  of13::MultipartReplyGroupDesc src(xid, 0x0);
  src.add_group_desc(desc);
  uint8_t* buffer = src.pack();
  of13::MultipartReplyGroupDesc dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartRequest, MultipartRequestGroupFeatures) {
  of13::MultipartRequestGroupFeatures src(xid, 0);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestGroupFeatures dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartReply, MultipartReplyGroupFeatures) {
  uint32_t actions[4] = {OFPAT_ALL, OFPAT_ALL, OFPAT_ALL, OFPAT_ALL};
  uint32_t max_groups[4] = {16, 32, 64, 128};
  of13::GroupFeatures features(OFPGT_ALL, of13::OFPGFC_SELECT_LIVENESS,
                               max_groups, actions);
  of13::MultipartReplyGroupFeatures src(xid, 0, features);
  uint8_t* buffer = src.pack();
  of13::MultipartReplyGroupFeatures dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartRequest, MultipartRequestMeter) {
  of13::MultipartRequestMeter src(xid, 0, 1);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestMeter dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartReply, MultipartReplyMeter) {
  of13::BandStats band_st(43, 2408);
  of13::MeterStats stats(1, 3, 43, 2408, 300, 20);
  stats.add_band_stats(band_st);
  of13::MultipartReplyMeter src(xid, 0);
  src.add_meter_stats(stats);
  uint8_t* buffer = src.pack();
  of13::MultipartReplyMeter dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartRequest, MultipartRequestMeterConfig) {
  of13::MultipartRequestMeterConfig src(xid, 0, 1);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestMeterConfig dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartReplyMeterConfig, MultipartReplyMeterConfig) {
  of13::MeterConfig mc1(of13::OFPMF_KBPS, 1);
  of13::MeterConfig mc2(of13::OFPMF_BURST, 2);
  of13::MeterBandDrop* bd = new of13::MeterBandDrop(10000, 20000);
  mc1.add_band(bd);
  of13::MeterBandDSCPRemark* dr =
      new of13::MeterBandDSCPRemark(10000, 20000, 2);
  mc2.add_band(dr);
  of13::MultipartReplyMeterConfig src(xid, 0);
  src.add_meter_config(mc1);
  src.add_meter_config(mc2);
  uint8_t* buffer = src.pack();
  of13::MultipartReplyMeterConfig dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartRequest, MultipartRequestMeterFeatures) {
  of13::MultipartRequestMeterFeatures src(xid, 0);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestMeterFeatures dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

uint32_t max_meter_;
uint32_t band_types_;
uint32_t capabilities_;
uint8_t max_bands_;
uint8_t max_color_;

TEST(MultipartReply, MultipartReplyMeterFeatures) {
  of13::MeterFeatures features(10, OFPMBT_ALL, OFPMF_ALL, 2, 3);
  of13::MultipartReplyMeterFeatures src(xid, 0, features);
  uint8_t* buffer = src.pack();
  of13::MultipartReplyMeterFeatures dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST_F(TableFeaturesTest, MultipartRequestTableFeatures) {
  of13::MultipartRequestTableFeatures src(xid, 0);
  src.add_table_features(tf);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestTableFeatures dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST_F(TableFeaturesTest, MultipartReplyTableFeatures) {
  of13::MultipartReplyTableFeatures src(xid, 0);
  src.add_table_features(tf);
  uint8_t* buffer = src.pack();
  of13::MultipartReplyTableFeatures dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartRequest, MultipartRequestPortDescription) {
  of13::MultipartRequestPortDescription src(xid, 0);
  uint8_t* buffer = src.pack();
  of13::MultipartRequestPortDescription dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MultipartReply, MultipartReplyPortDescription) {
  of13::Port p1(1, "00:00:00:00:00:01", "Port 1", 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                10000, 100000);
  of13::Port p2(2, "00:00:00:00:00:02", "Port 2", 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                10000, 100000);
  of13::Port p3(3, "00:00:00:00:00:03", "Port 3", 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                10000, 100000);
  of13::Port p4(4, "00:00:00:00:00:04", "Port 4", 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                10000, 100000);
  of13::MultipartReplyPortDescription src(xid, 0);
  src.add_port(p1);
  src.add_port(p2);
  src.add_port(p3);
  src.add_port(p4);
  uint8_t* buffer = src.pack();
  of13::MultipartReplyPortDescription dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

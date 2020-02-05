#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "constants.hh"

using namespace fluid_msg;

class StatsFlow : public ::testing::Test {
 protected:
  virtual void SetUp() {
    req_match.in_port(1);
    req_match.dl_src("00:00:00:00:00:01");
    match1 = match2 = req_match;
    match1.nw_src("192.168.1.0");
    match1.dl_vlan(2);
    of10::OutputAction* act1 = new of10::OutputAction(1, send_len);
    of10::SetDLSrcAction* act12 = new of10::SetDLSrcAction("00:00:00:00:00:02");
    a1.add_action(act1);
    a1.add_action(act12);
    fs1 = of10::FlowStats(table_id, 300, 20, 1360, 0, 500, cookie, 36, 1756,
                          match1, a1);
    match2.nw_src("192.168.1.2");
    match2.dl_type(0x8100);
    of10::OutputAction* act2 = new of10::OutputAction(2, send_len);
    of10::SetNWSrcAction* act21 = new of10::SetNWSrcAction("192.168.2.1");
    a1.add_action(act2);
    a1.add_action(act21);
    fs2 = of10::FlowStats(table_id, 659, 96, 2000, 100, 0, cookie, 42, 2352,
                          match2, a2);
  }
  // virtual void TearDown() {}
  of10::Match req_match;
  of10::FlowStats fs1;
  of10::FlowStats fs2;
  of10::Match match1;
  of10::Match match2;
  ActionList a1;
  ActionList a2;
};

TEST(StatsRequest, StatsRequestDesc) {
  of10::StatsRequestDesc src(xid, 0);
  uint8_t* buffer = src.pack();
  of10::StatsRequestDesc dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(StatsReply, StatsReplyDesc) {
  SwitchDesc desc("Fluid Test", "Not Switch", "OpenFlow Driver", "0000032",
                  "Just a test");
  of10::StatsReplyDesc src(xid, of10::OFPSF_REPLY_MORE, desc);
  uint8_t* buffer = src.pack();
  of10::StatsReplyDesc dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST_F(StatsFlow, StatsRequestFlow) {
  of10::StatsRequestFlow src(xid, 0, req_match, table_id, of10::OFPP_NONE);
  uint8_t* buffer = src.pack();
  of10::StatsRequestFlow dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST_F(StatsFlow, StatsReplyFlow) {
  of10::StatsReplyFlow src(xid, 0);
  src.add_flow_stats(fs1);
  src.add_flow_stats(fs2);
  uint8_t* buffer = src.pack();
  of10::StatsReplyFlow dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST_F(StatsFlow, StatsRequestAggregate) {
  of10::StatsRequestAggregate src(xid, 0, req_match, table_id, of10::OFPP_NONE);
  uint8_t* buffer = src.pack();
  of10::StatsRequestAggregate dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(StatsReply, StatsReplyAggregate) {
  of10::StatsReplyAggregate src(xid, 0, 78, 4368, 2);
  uint8_t* buffer = src.pack();
  of10::StatsReplyAggregate dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(StatsRequest, StatsRequestTable) {
  of10::StatsRequestTable src(xid, 0);
  uint8_t* buffer = src.pack();
  of10::StatsRequestTable dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(StatsReply, StatsReplyTable) {
  of10::TableStats stats(table_id, "Table 0", of10::OFPFW_ALL, 65536, 2, 136,
                         78);
  of10::StatsReplyTable src(xid, 0);
  src.add_table_stat(stats);
  uint8_t* buffer = src.pack();
  of10::StatsReplyTable dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(StatsRequest, StatsRequestPort) {
  of10::StatsRequestPort src(xid, 0, 1);
  uint8_t* buffer = src.pack();
  of10::StatsRequestPort dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(StatsReply, StatsReplyPort) {
  struct port_rx_tx_stats rx_tx = {64, 32, 3584, 1792, 0, 0};
  struct port_err_stats err = {2, 4, 0, 0, 0};
  of10::PortStats stats(1, rx_tx, err, 0);
  of10::StatsReplyPort src(xid, 0);
  src.add_port_stat(stats);
  uint8_t* buffer = src.pack();
  of10::StatsReplyPort dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(StatsRequest, StatsRequestQueue) {
  of10::StatsRequestQueue src(xid, 0, 1, 22);
  uint8_t* buffer = src.pack();
  of10::StatsRequestQueue dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(StatsReplyQueue, StatsReplyQueue) {
  of10::QueueStats stats(1, 22, 1904, 34, 0);
  of10::StatsReplyQueue src(xid, 0);
  src.add_queue_stat(stats);
  uint8_t* buffer = src.pack();
  of10::StatsReplyQueue dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

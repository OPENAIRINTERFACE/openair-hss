#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "constants.hh"

using namespace fluid_msg;

/*Create Ports for Features Reply*/
class FeaturesReplyTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    this->p1 = of10::Port(1, "00:00:00:00:00:01", "Port 1", 0x0, 0x0, 0x0, 0x0,
                          0x0, 0x0);
    this->p2 = of10::Port(2, "00:00:00:00:00:02", "Port 2", 0x0, 0x0, 0x0, 0x0,
                          0x0, 0x0);
    this->p3 = of10::Port(3, "00:00:00:00:00:03", "Port 3", 0x0, 0x0, 0x0, 0x0,
                          0x0, 0x0);
    this->p4 = of10::Port(4, "00:00:00:00:00:04", "Port 4", 0x0, 0x0, 0x0, 0x0,
                          0x0, 0x0);
    this->ports.push_back(this->p1);
    this->ports.push_back(this->p2);
    this->ports.push_back(this->p3);
    this->ports.push_back(this->p4);
    src = of10::FeaturesReply(xid, dp_id, 32, 1, 0x0, 0x0, ports);
    src2 = of10::FeaturesReply(xid, dp_id, 32, 1, 0x0, 0x0);
    src2.add_port(this->p1);
    src2.add_port(this->p2);
    src2.add_port(this->p3);
    src2.add_port(this->p4);
  }
  // virtual void TearDown() {}
  of10::FeaturesReply src, src2;
  of10::Port p1, p2, p3, p4;
  std::vector<of10::Port> ports;
};

TEST(HelloTest, of10Hello) {
  // Construct Hello message with a xid
  of10::Hello src(xid);
  // pack the message
  uint8_t* buffer = src.pack();
  // Creates a hello message with data
  of10::Hello dst;
  dst.unpack(buffer);
  // compare the structures
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(HelloTest, of13Hello) {
  // Construct Hello message with a xid and HelloElem
  std::list<of13::HelloElemVersionBitmap> elements;
  of13::HelloElemVersionBitmap hl;
  hl.add_bitmap(0xC);
  elements.push_back(hl);
  of13::Hello src(xid, elements);
  // pack the message
  uint8_t* data = src.pack();
  // Creates a hello message with data
  of13::Hello dst;
  dst.unpack(data);
  // compare the structures
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(data);
}

TEST(ErrorTest, of10Error) {
  // Construct an OpenFlow 1.3 Hello message that will be part of the
  // error message
  of13::Hello hello(xid);
  // pack the message
  uint8_t* data = hello.pack();
  of10::Error src(hello.xid(), of10::OFPET_HELLO_FAILED,
                  of10::OFPHFC_INCOMPATIBLE, (uint8_t*)data, hello.length());
  uint8_t* buffer = src.pack();
  // unpack error
  of10::Error dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  // Compare data
  ASSERT_EQ(0, memcmp(src.data(), dst.data(), src.data_len()));
  OFMsg::free_buffer(data);
  OFMsg::free_buffer(buffer);
}

TEST(ErrorTest, of13Error) {
  // Construct an OpenFlow 1.0 Hello message that will be part of the
  // error message
  of10::Hello hello(xid);
  // pack the message
  uint8_t* data = hello.pack();
  of13::Error src(hello.xid(), of13::OFPET_HELLO_FAILED,
                  of13::OFPHFC_INCOMPATIBLE, (uint8_t*)data, hello.length());
  uint8_t* buffer = src.pack();
  // unpack error
  of13::Error dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(data);
  OFMsg::free_buffer(buffer);
}

TEST(EchoRequestTest, of10Error) {
  // Creates echo request with some data
  char echo_data[30] = "Hey you, OpenFlow 1.0 Switch?";
  of10::EchoRequest src(xid);
  src.data((uint8_t*)echo_data, 30);
  // Pack echo request
  uint8_t* buffer = src.pack();
  of10::EchoRequest dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(EchoRequestTest, of13Error) {
  // Creates echo request with some data
  char echo_data[30] = "Hey you, OpenFlow 1.3 Switch?";
  of13::EchoRequest src(xid);
  src.data((uint8_t*)echo_data, 30);
  // Pack echo request
  uint8_t* buffer = src.pack();
  of13::EchoRequest dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(EchoReplyTest, of10Error) {
  // Creates echo request with some data
  char echo_data[45] = "Hello, I'm here OpenFlow 1.0 Controller!";
  of10::EchoRequest src(xid);
  src.data((uint8_t*)echo_data, 45);
  // Pack echo request
  uint8_t* buffer = src.pack();
  of10::EchoReply dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(EchoReplyTest, of13Error) {
  // Creates echo request with some data
  char echo_data[45] = "Hello, I'm here OpenFlow 1.0 Controller!";
  of13::EchoReply src(xid);
  src.data((uint8_t*)echo_data, 45);
  // Pack echo request
  uint8_t* buffer = src.pack();
  of13::EchoReply dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(FeaturesRequestTest, of10FeaturesRequest) {
  of10::FeaturesRequest src(xid);
  uint8_t* buffer = src.pack();
  of10::FeaturesRequest dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(FeaturesRequestTest, of13FeaturesRequest) {
  of13::FeaturesRequest src(xid);
  uint8_t* buffer = src.pack();
  of13::FeaturesRequest dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST_F(FeaturesReplyTest, of10FeaturesReply) {
  // Test inserting the port constructor
  uint8_t* buffer = src.pack();
  of10::FeaturesReply dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
  buffer = src2.pack();
  // Use a different features reply dst
  // because if we use the same, the ports
  // will be duplicated from the previous
  // test
  of10::FeaturesReply dst2;
  dst2.unpack(buffer);
  ASSERT_EQ(src2, dst2);
  OFMsg::free_buffer(buffer);
}

TEST_F(FeaturesReplyTest, of13FeaturesReply) {
  of13::FeaturesReply src(xid, dp_id, 32, n_tables, 1, 0x0);
  uint8_t* buffer = src.pack();
  of13::FeaturesReply dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(GetConfigRequestTest, of10GetConfigRequest) {
  of10::GetConfigRequest src(xid);
  uint8_t* buffer = src.pack();
  of10::GetConfigRequest dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}
TEST(GetConfigRequestTest, of13GetConfigRequest) {
  of13::GetConfigRequest src(xid);
  uint8_t* buffer = src.pack();
  of13::GetConfigRequest dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(GetConfigReplyTest, of10GetConfigReply) {
  of10::GetConfigReply src(xid, of10::OFPC_FRAG_NORMAL, send_len);
  uint8_t* buffer = src.pack();
  of10::GetConfigReply dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}
TEST(GetConfigRequestTest, of13GetConfigReply) {
  of13::GetConfigReply src(xid, of10::OFPC_FRAG_NORMAL, send_len);
  uint8_t* buffer = src.pack();
  of13::GetConfigReply dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(SetConfigTest, of10SetConfig) {
  of10::SetConfig src(xid, of10::OFPC_FRAG_NORMAL, send_len);
  uint8_t* buffer = src.pack();
  of10::SetConfig dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}
TEST(SetConfigTest, of13SetConfig) {
  of13::SetConfig src(xid, of10::OFPC_FRAG_NORMAL, send_len);
  uint8_t* buffer = src.pack();
  of13::SetConfig dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(PortStatusTest, of10PortStatus) {
  of10::Port p(1, "00:00:00:00:00:01", "Port 1", 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
  of10::PortStatus src(xid, of10::OFPPR_ADD, p);
  uint8_t* buffer = src.pack();
  of10::PortStatus dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(PortStatusTest, of13PortStatus) {
  of13::Port p(1, "00:00:00:00:00:01", "Port 1", 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
               1000, 100000);
  of13::PortStatus src(xid, of10::OFPPR_ADD, p);
  uint8_t* buffer = src.pack();
  of13::PortStatus dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(PortModTest, of10PortMod) {
  of10::PortMod src(xid, 1, "00:00:00:00:00:01", of10::OFPPC_PORT_DOWN,
                    0xffffffff, of10::OFPPF_10MB_HD);
  uint8_t* buffer = src.pack();
  of10::PortMod dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(PortModTest, of13PortMod) {
  of13::PortMod src(xid, 1, "00:00:00:00:00:01", of13::OFPPC_PORT_DOWN,
                    0xffffffff, of13::OFPPF_10MB_HD);
  uint8_t* buffer = src.pack();
  of13::PortMod dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(QueueGetConfigRequest, of10ueueGetConfigRequest) {
  of10::QueueGetConfigRequest src(xid, 1);
  uint8_t* buffer = src.pack();
  of10::QueueGetConfigRequest dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(QueueGetConfigRequest, of13QueueGetConfigRequest) {
  of13::QueueGetConfigRequest src(xid, 1);
  uint8_t* buffer = src.pack();
  of13::QueueGetConfigRequest dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(BarrierRequestTest, of10BarrierRequest) {
  of10::BarrierRequest src(xid);
  uint8_t* buffer = src.pack();
  of10::BarrierRequest dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}
TEST(BarrierRequestTest, of13BarrierRequest) {
  of13::BarrierRequest src(xid);
  uint8_t* buffer = src.pack();
  of13::BarrierRequest dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(BarrierReplyTest, of10BarrierReply) {
  of10::BarrierReply src(xid);
  uint8_t* buffer = src.pack();
  // EXPECT_EQ(0, memcmp(buffer, barrier_reply, src.length()));
  of10::BarrierReply dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(BarrierReplyTest, of13BarrierReply) {
  of13::BarrierReply src(xid);
  uint8_t* buffer = src.pack();
  of13::BarrierReply dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(GroupModTest, of13GroupMod) {
  of13::OutputAction* act = new of13::OutputAction(1, send_len);
  // of13::DecNWTTLAction* actw = new of13::DecNWTTLAction();
  // of13::PopVLANAction* actv = new of13::PopVLANAction();
  of13::Bucket b(0x0, 1, of13::OFPG_ANY);
  // b.add_action(actw);
  b.add_action(act);
  // b.add_action(actv);
  of13::GroupMod src(xid, of13::OFPGC_ADD, of13::OFPGT_FF, group_id);
  src.add_bucket(b);
  uint8_t* buffer = src.pack();
  of13::GroupMod dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(TableModTest, of13TableMod) {
  of13::TableMod src(xid, table_id, 0x0);
  uint8_t* buffer = src.pack();
  of13::TableMod dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(RoleRequestTest, of13RoleRequest) {
  of13::RoleRequest src(xid, OFPCR_ROLE_MASTER, generation_id);
  uint8_t* buffer = src.pack();
  of13::RoleRequest dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(RoleReplyTest, of13RoleReply) {
  of13::RoleReply src(xid, OFPCR_ROLE_MASTER, generation_id);
  uint8_t* buffer = src.pack();
  of13::RoleReply dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(GetAsyncRequestTest, of13GetAsyncRequest) {
  of13::GetAsyncRequest src(xid);
  uint8_t* buffer = src.pack();
  of13::GetAsyncRequest dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(GetAsyncReplyTest, of13GetAsyncReply) {
  of13::GetAsyncReply src(xid);
  src.master_equal_packet_in_mask(2);
  src.master_equal_port_status_mask(4);
  src.master_equal_flow_removed_mask(1);
  src.slave_packet_in_mask(1);
  src.slave_port_status_mask(2);
  src.slave_flow_removed_mask(4);
  uint8_t* buffer = src.pack();
  of13::GetAsyncReply dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(SetAsyncTest, of13SetAsync) {
  of13::SetAsync src(xid);
  src.master_equal_packet_in_mask(2);
  src.master_equal_port_status_mask(4);
  src.master_equal_flow_removed_mask(1);
  src.slave_packet_in_mask(1);
  src.slave_port_status_mask(2);
  src.slave_flow_removed_mask(4);
  uint8_t* buffer = src.pack();
  of13::SetAsync dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(QueueGetConfigReply, of10QueueGetConfigReply) {
  of10::QueuePropMinRate* min_rate = new of10::QueuePropMinRate(100);
  of10::PacketQueue pq(1);
  pq.add_property(min_rate);
  of10::QueueGetConfigReply src(xid, 1);
  src.add_queue(pq);
  uint8_t* buffer = src.pack();
  of10::QueueGetConfigReply dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(QueueGetConfigReply, of13QueueGetConfigReply) {
  of13::QueuePropMinRate* min_rate = new of13::QueuePropMinRate(100);
  of13::QueuePropMaxRate* max_rate = new of13::QueuePropMaxRate(1000);
  of13::PacketQueue pq(1, 1);
  pq.add_property(min_rate);
  pq.add_property(max_rate);
  of13::QueueGetConfigReply src(xid, 1);
  src.add_queue(pq);
  uint8_t* buffer = src.pack();
  of13::QueueGetConfigReply dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

TEST(MeterModTest, of13MeterMod) {
  of13::MeterMod src(xid, of13::OFPMC_ADD, of13::OFPMF_KBPS, 1);
  of13::MeterBandDrop* bd = new of13::MeterBandDrop(10000, 20000);
  src.add_band(bd);
  of13::MeterBandDSCPRemark* dr =
      new of13::MeterBandDSCPRemark(10000, 20000, 2);
  src.add_band(dr);
  uint8_t* buffer = src.pack();
  of13::MeterMod dst;
  dst.unpack(buffer);
  ASSERT_EQ(src, dst);
  OFMsg::free_buffer(buffer);
}

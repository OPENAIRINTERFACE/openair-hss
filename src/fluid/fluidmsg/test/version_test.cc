#include <gtest/gtest.h>
#include "of10msg.hh"
#include "of13msg.hh"

using namespace fluid_msg;

template <class T>
OFMsg* CreateMsg();

template <>
OFMsg* CreateMsg<of10::Hello>() {
  return new of10::Hello();
}

template <>
OFMsg* CreateMsg<of10::EchoRequest>() {
  return new of10::EchoRequest();
}

template <>
OFMsg* CreateMsg<of10::Error>() {
  return new of10::Error();
}

template <>
OFMsg* CreateMsg<of10::EchoReply>() {
  return new of10::EchoReply();
}

template <>
OFMsg* CreateMsg<of10::Vendor>() {
  return new of10::Vendor();
}

template <>
OFMsg* CreateMsg<of10::FeaturesRequest>() {
  return new of10::FeaturesRequest();
}

template <>
OFMsg* CreateMsg<of10::FeaturesReply>() {
  return new of10::FeaturesReply();
}

template <>
OFMsg* CreateMsg<of10::GetConfigRequest>() {
  return new of10::GetConfigRequest();
}

template <>
OFMsg* CreateMsg<of10::GetConfigReply>() {
  return new of10::GetConfigReply();
}

template <>
OFMsg* CreateMsg<of10::SetConfig>() {
  return new of10::SetConfig();
}

template <>
OFMsg* CreateMsg<of10::FlowMod>() {
  return new of10::FlowMod();
}

template <>
OFMsg* CreateMsg<of10::PacketOut>() {
  return new of10::PacketOut();
}

template <>
OFMsg* CreateMsg<of10::PacketIn>() {
  return new of10::PacketIn();
}

template <>
OFMsg* CreateMsg<of10::FlowRemoved>() {
  return new of10::FlowRemoved();
}

template <>
OFMsg* CreateMsg<of10::PortStatus>() {
  return new of10::PortStatus();
}

template <>
OFMsg* CreateMsg<of10::PortMod>() {
  return new of10::PortMod();
}

template <>
OFMsg* CreateMsg<of10::StatsRequest>() {
  return new of10::StatsRequest();
}

template <>
OFMsg* CreateMsg<of10::StatsReply>() {
  return new of10::StatsReply();
}

template <>
OFMsg* CreateMsg<of10::QueueGetConfigRequest>() {
  return new of10::QueueGetConfigRequest();
}

template <>
OFMsg* CreateMsg<of10::QueueGetConfigReply>() {
  return new of10::QueueGetConfigReply();
}

template <>
OFMsg* CreateMsg<of10::BarrierRequest>() {
  return new of10::BarrierRequest();
}

template <>
OFMsg* CreateMsg<of10::BarrierReply>() {
  return new of10::BarrierReply();
}

template <>
OFMsg* CreateMsg<of13::Hello>() {
  return new of13::Hello();
}

template <>
OFMsg* CreateMsg<of13::EchoRequest>() {
  return new of13::EchoRequest();
}

template <>
OFMsg* CreateMsg<of13::Error>() {
  return new of13::Error();
}

template <>
OFMsg* CreateMsg<of13::EchoReply>() {
  return new of13::EchoReply();
}

template <>
OFMsg* CreateMsg<of13::FeaturesRequest>() {
  return new of13::FeaturesRequest();
}

template <>
OFMsg* CreateMsg<of13::FeaturesReply>() {
  return new of13::FeaturesReply();
}

template <>
OFMsg* CreateMsg<of13::GetConfigRequest>() {
  return new of13::GetConfigRequest();
}

template <>
OFMsg* CreateMsg<of13::GetConfigReply>() {
  return new of13::GetConfigReply();
}

template <>
OFMsg* CreateMsg<of13::SetConfig>() {
  return new of13::SetConfig();
}

template <>
OFMsg* CreateMsg<of13::FlowMod>() {
  return new of13::FlowMod();
}

template <>
OFMsg* CreateMsg<of13::PacketOut>() {
  return new of13::PacketOut();
}

template <>
OFMsg* CreateMsg<of13::PacketIn>() {
  return new of13::PacketIn();
}

template <>
OFMsg* CreateMsg<of13::FlowRemoved>() {
  return new of13::FlowRemoved();
}

template <>
OFMsg* CreateMsg<of13::PortStatus>() {
  return new of13::PortStatus();
}

template <>
OFMsg* CreateMsg<of13::PortMod>() {
  return new of13::PortMod();
}

template <>
OFMsg* CreateMsg<of13::MultipartRequest>() {
  return new of13::MultipartRequest();
}

template <>
OFMsg* CreateMsg<of13::MultipartReply>() {
  return new of13::MultipartReply();
}

template <>
OFMsg* CreateMsg<of13::QueueGetConfigRequest>() {
  return new of13::QueueGetConfigRequest();
}

template <>
OFMsg* CreateMsg<of13::QueueGetConfigReply>() {
  return new of13::QueueGetConfigReply();
}

template <>
OFMsg* CreateMsg<of13::BarrierRequest>() {
  return new of13::BarrierRequest();
}

template <>
OFMsg* CreateMsg<of13::BarrierReply>() {
  return new of13::BarrierReply();
}

template <>
OFMsg* CreateMsg<of13::GroupMod>() {
  return new of13::GroupMod();
}

template <>
OFMsg* CreateMsg<of13::TableMod>() {
  return new of13::TableMod();
}

template <>
OFMsg* CreateMsg<of13::RoleRequest>() {
  return new of13::RoleRequest();
}

template <>
OFMsg* CreateMsg<of13::RoleReply>() {
  return new of13::RoleReply();
}

template <>
OFMsg* CreateMsg<of13::GetAsyncRequest>() {
  return new of13::GetAsyncRequest();
}

template <>
OFMsg* CreateMsg<of13::GetAsyncReply>() {
  return new of13::GetAsyncReply();
}

template <>
OFMsg* CreateMsg<of13::SetAsync>() {
  return new of13::SetAsync();
}

template <>
OFMsg* CreateMsg<of13::MeterMod>() {
  return new of13::MeterMod();
}

// Then we define a test fixture class template.
template <class T>
class OF10MsgVersionTest : public testing::Test {
 protected:
  // The ctor calls the factory function to create a prime table
  // implemented by T.
  OF10MsgVersionTest() : msg_(CreateMsg<T>()) {}
  virtual ~OF10MsgVersionTest() { delete msg_; }

  OFMsg* const msg_;
};

// Then we define a test fixture class template.
template <class T>
class OF13MsgVersionTest : public testing::Test {
 protected:
  // The ctor calls the factory function to create a prime table
  // implemented by T.
  OF13MsgVersionTest() : msg_(CreateMsg<T>()) {}

  virtual ~OF13MsgVersionTest() { delete msg_; }

  OFMsg* const msg_;
};

#if GTEST_HAS_TYPED_TEST

using testing::Types;

typedef Types<
    of10::Hello, of10::EchoRequest, of10::Error, of10::EchoReply, of10::Vendor,
    of10::FeaturesRequest, of10::FeaturesReply, of10::GetConfigRequest,
    of10::GetConfigReply, of10::SetConfig, of10::FlowMod, of10::PacketOut,
    of10::PacketIn, of10::FlowRemoved, of10::PortStatus, of10::PortMod,
    of10::StatsRequest, of10::StatsReply, of10::QueueGetConfigRequest,
    of10::QueueGetConfigReply, of10::BarrierRequest, of10::BarrierReply>
    OF10Implementations;

typedef Types<of13::Hello, of13::EchoRequest, of13::Error, of13::EchoReply,
              of13::FeaturesRequest, of13::FeaturesReply,
              of13::GetConfigRequest, of13::GetConfigReply, of13::SetConfig,
              of13::FlowMod, of13::PacketOut, of13::PacketIn, of13::FlowRemoved,
              of13::PortStatus, of13::PortMod, of13::GroupMod, of13::TableMod,
              of13::MultipartRequest, of13::MultipartReply,
              of13::QueueGetConfigRequest, of13::QueueGetConfigReply,
              of13::BarrierRequest, of13::BarrierReply, of13::RoleRequest,
              of13::RoleReply, of13::GetAsyncRequest, of13::GetAsyncReply,
              of13::SetAsync, of13::MeterMod>
    OF13Implementations;

TYPED_TEST_CASE(OF10MsgVersionTest, OF10Implementations);

TYPED_TEST_CASE(OF13MsgVersionTest, OF13Implementations);

TYPED_TEST(OF10MsgVersionTest, VersionAssert) {
  ASSERT_EQ(of10::OFP_VERSION, this->msg_->version());
}

TYPED_TEST(OF13MsgVersionTest, VersionAssert) {
  ASSERT_EQ(of13::OFP_VERSION, this->msg_->version());
}
#endif  // GTEST_HAS_TYPED_TEST

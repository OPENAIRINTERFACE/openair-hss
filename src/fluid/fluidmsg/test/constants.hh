#include "of10msg.hh"
#include "of13msg.hh"

using namespace fluid_msg;

const uint32_t xid = 42;

const uint64_t dp_id = 0x0000000000000001;

const uint8_t table_id = 2;

const uint32_t group_id = 1;

const uint8_t n_tables = 32;

const uint16_t send_len = 1024;

const uint64_t generation_id = 12345;

const uint64_t cookie = 0x1000000000000001;

const uint64_t cookie_mask = 0xffffffffffffffff;

const uint16_t priority = 1000;

const uint16_t hard_timeout = 300;

const uint16_t idle_timeout = 60;

const uint32_t OXM_OF_IN_PORT = of13::OXMTLV::make_header(0x8000, 0, false, 4);

const uint32_t OXM_OF_IN_PHY_PORT =
    of13::OXMTLV::make_header(0x8000, 1, false, 4);

/* Metadata passed btw tables. */
const uint32_t OXM_OF_METADATA = of13::OXMTLV::make_header(0x8000, 2, false, 8);
const uint32_t OXM_OF_METADATA_W =
    of13::OXMTLV::make_header(0x8000, 2, true, 8);

/* Ethernet destination address.*/
const uint32_t OXM_OF_ETH_DST = of13::OXMTLV::make_header(0x8000, 3, false, 6);
const uint32_t OXM_OF_ETH_DST_W = of13::OXMTLV::make_header(0x8000, 3, true, 6);

/* Ethernet source address.*/
const uint32_t OXM_OF_ETH_SRC = of13::OXMTLV::make_header(0x8000, 4, false, 6);
const uint32_t OXM_OF_ETH_SRC_W = of13::OXMTLV::make_header(0x8000, 4, true, 6);

/* Ethernet frame type. */
const uint32_t OXM_OF_ETH_TYPE = of13::OXMTLV::make_header(0x8000, 5, false, 2);

/* IP protocol. */
const uint32_t OXM_OF_IP_PROTO =
    of13::OXMTLV::make_header(0x8000, 10, false, 1);

/* IP source address. */
const uint32_t OXM_OF_IPV4_SRC =
    of13::OXMTLV::make_header(0x8000, 11, false, 4);
const uint32_t OXM_OF_IPV4_SRC_W =
    of13::OXMTLV::make_header(0x8000, 11, true, 4);

/* IP destination address. */
const uint32_t OXM_OF_IPV4_DST =
    of13::OXMTLV::make_header(0x8000, 12, false, 4);
const uint32_t OXM_OF_IPV4_DST_W =
    of13::OXMTLV::make_header(0x8000, 12, true, 4);

/* TCP source port. */
const uint32_t OXM_OF_TCP_SRC = of13::OXMTLV::make_header(0x8000, 13, false, 2);

/* TCP destination port. */
const uint32_t OXM_OF_TCP_DST = of13::OXMTLV::make_header(0x8000, 14, false, 2);

/* UDP source port. */
const uint32_t OXM_OF_UDP_SRC = of13::OXMTLV::make_header(0x8000, 15, false, 2);

/* UDP destination port. */
const uint32_t OXM_OF_UDP_DST = of13::OXMTLV::make_header(0x8000, 16, false, 2);

const uint32_t OFPAT_ALL =
    (1 << of13::OFPAT_OUTPUT) | (1 << of13::OFPAT_COPY_TTL_OUT) |
    (1 << of13::OFPAT_COPY_TTL_IN) | (1 << of13::OFPAT_SET_MPLS_TTL) |
    (1 << of13::OFPAT_DEC_MPLS_TTL) | (1 << of13::OFPAT_PUSH_VLAN) |
    (1 << of13::OFPAT_POP_VLAN) | (1 << of13::OFPAT_PUSH_MPLS) |
    (1 << of13::OFPAT_POP_MPLS) | (1 << of13::OFPAT_SET_QUEUE) |
    (1 << of13::OFPAT_GROUP) | (1 << of13::OFPAT_SET_NW_TTL) |
    (1 << of13::OFPAT_DEC_NW_TTL);

const uint32_t OFPGT_ALL = (1 << of13::OFPGT_ALL) | (1 << of13::OFPGT_SELECT) |
                           (1 << of13::OFPGT_INDIRECT) | (1 << of13::OFPGT_FF);

const uint32_t OFPMBT_ALL =
    (1 << of13::OFPMBT_DROP) | (1 << of13::OFPMBT_DSCP_REMARK);

const uint32_t OFPMF_ALL = of13::OFPMF_KBPS | of13::OFPMF_PKTPS |
                           of13::OFPMF_BURST | of13::OFPMF_STATS;

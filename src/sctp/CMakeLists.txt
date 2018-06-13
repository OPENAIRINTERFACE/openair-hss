
set(ASN1RELDIR r10.5)
include_directories(${SRC_TOP_DIR}/common)
include_directories(${SRC_TOP_DIR}/common/itti)
include_directories(${CMAKE_BINARY_DIR}/s1ap/${ASN1RELDIR})
include_directories(${SRC_TOP_DIR}/utils)
include_directories(${SRC_TOP_DIR}/utils/bstr)
include_directories(${SRC_TOP_DIR}/utils/hashtable)
include_directories(${SRC_TOP_DIR}/utils/msc)

# nas_messages_types pulls in everything.
# TODO (amar) fix include leak
include_directories("${SRC_TOP_DIR}/nas")
include_directories("${SRC_TOP_DIR}/nas/emm")
include_directories("${SRC_TOP_DIR}/nas/emm/msg")
include_directories("${SRC_TOP_DIR}/nas/emm/sap")
include_directories("${SRC_TOP_DIR}/nas/ies")
include_directories("${SRC_TOP_DIR}/nas/util")
include_directories("${SRC_TOP_DIR}/nas/esm")
include_directories("${SRC_TOP_DIR}/nas/esm/msg")
include_directories("${SRC_TOP_DIR}/nas/api/network")
include_directories("${SRC_TOP_DIR}/nas/api/mme")
include_directories(${SRC_TOP_DIR}/mme)
include_directories("${SRC_TOP_DIR}/mme_app")
include_directories("${SRC_TOP_DIR}/s1ap/messages/asn1/${ASN1RELDIR}")
include_directories("${SRC_TOP_DIR}/s1ap")
include_directories(${SRC_TOP_DIR}/sgw)

add_library(SCTP_SERVER
    sctp_common.c
    sctp_itti_messaging.c
    sctp_primitives_server.c
    )

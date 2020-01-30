set(ASN1RELDIR r10.5)
include_directories(${SRC_TOP_DIR}/common)
include_directories(${SRC_TOP_DIR}/common/itti)
include_directories(${CMAKE_BINARY_DIR}/s1ap/${ASN1RELDIR})
include_directories(${SRC_TOP_DIR}/utils)
include_directories(${SRC_TOP_DIR}/utils/bstr)
include_directories(${SRC_TOP_DIR}/utils/hashtable)
include_directories(${SRC_TOP_DIR}/utils/msc)
include_directories(${SRC_TOP_DIR}/gtpv2-c/gtpv2c_ie_formatter/shared)
include_directories(${SRC_TOP_DIR}/gtpv2-c/nwgtpv2c-0.11/include)
include_directories(${SRC_TOP_DIR}/gtpv2-c/nwgtpv2c-0.11/shared)
include_directories(${SRC_TOP_DIR}/mme)

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
include_directories("${SRC_TOP_DIR}/mme_app")
include_directories("${SRC_TOP_DIR}/s1ap/messages/asn1/${ASN1RELDIR}")
include_directories("${SRC_TOP_DIR}/s1ap")

add_library(S11_MME
    s11_common.c
    s11_ie_formatter.c
    s11_mme_task.c
    s11_mme_bearer_manager.c
    s11_mme_session_manager.c
    )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

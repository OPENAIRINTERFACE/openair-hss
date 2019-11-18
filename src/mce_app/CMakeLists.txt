set(ASN1RELDIR r10.5)
include_directories(${SRC_TOP_DIR}/common)
include_directories(${SRC_TOP_DIR}/common/itti)
include_directories(${CMAKE_BINARY_DIR}/s1ap/${ASN1RELDIR})
include_directories(${SRC_TOP_DIR}/utils)
include_directories(${SRC_TOP_DIR}/utils/bstr)
include_directories(${SRC_TOP_DIR}/utils/hashtable)
include_directories(${SRC_TOP_DIR}/utils/msc)
include_directories(${SRC_TOP_DIR}/gtpv2-c/gtpv2c_ie_formatter/shared)
include_directories(${SRC_TOP_DIR}/mme)
include_directories(${SRC_TOP_DIR}/m2ap)

# TODO (amar) fix include leak
include_directories("${SRC_TOP_DIR}/mme_app")

add_library(MCE_APP
    mce_app_mbms_service.c
    mce_app_itti_messaging.c
    mce_app_main.c
    mce_app_statistics.c
		mce_app_mbms_service_context.c
  	mce_app_mbsfn_context.c
    )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

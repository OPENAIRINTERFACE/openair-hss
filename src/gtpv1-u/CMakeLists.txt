# gtpv1u_sgw_defs.h is inculded from sgw.h and hence is retained in this
# directory as in in OAI.
set(ASN1RELDIR r10.5)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${SRC_TOP_DIR}/common)
include_directories(${SRC_TOP_DIR}/common/itti)
include_directories(${CMAKE_BINARY_DIR}/s1ap/${ASN1RELDIR})
include_directories(${SRC_TOP_DIR}/utils)
include_directories(${SRC_TOP_DIR}/utils/bstr)
include_directories(${SRC_TOP_DIR}/utils/hashtable)
include_directories(${SRC_TOP_DIR}/utils/msc)

# Header leaks of spgw_config.h, sgw_config.h, pgw_config.h, sgw.h
include_directories(${SRC_TOP_DIR}/sgw)

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

if(ENABLE_LIBGTPNL)
pkg_search_module(GTPNL libgtpnl REQUIRED)
include_directories(${GTPNL_INCLUDE_DIRS})
endif(ENABLE_LIBGTPNL)


set (GTPV1U_SRC
    gtpv1u_task.c
    gtpv1u_teid_pool.c
    )

if(ENABLE_LIBGTPNL)
list(APPEND GTPV1U_SRC gtp_tunnel_libgtpnl_marking_bearer.c)
endif(ENABLE_LIBGTPNL)

if(ENABLE_OPENFLOW)
include_directories("${SRC_TOP_DIR}/openflow/controller")
list(APPEND GTPV1U_SRC gtp_tunnel_openflow.c)
endif(ENABLE_OPENFLOW)

if(ENABLE_OPENFLOW_MOSAIC)
include_directories("${SRC_TOP_DIR}/openflow/mosaic/core/eps")
list(APPEND GTPV1U_SRC gtp_tunnel_openflow_mosaic.cc)
endif(ENABLE_OPENFLOW_MOSAIC)

add_library(GTPV1U ${GTPV1U_SRC})

if(ENABLE_LIBGTPNL)
target_link_libraries(GTPV1U ${GTPNL_LIBRARIES})
endif(ENABLE_LIBGTPNL)


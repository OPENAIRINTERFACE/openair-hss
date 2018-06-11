###############################################################################
# Utilities Library
###############################################################################

# libbstr
add_library(BSTR
    ${CMAKE_CURRENT_SOURCE_DIR}/bstr/bstraux.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bstr/bstrlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bstr/buniutil.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bstr/utf8util.c
    )
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/bstr)

# libhashtable
add_library(HASHTABLE
    ${CMAKE_CURRENT_SOURCE_DIR}/hashtable/hashtable.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hashtable/hashtable_uint64.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hashtable/obj_hashtable.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hashtable/obj_hashtable_uint64.c
    )
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/hashtable)
# pulls in dynamic_memory_checker.h
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# log.h pulls in gcc_diag.h
set(ASN1RELDIR r10.5)
#include_directories(${SRC_TOP_DIR}/common)
include_directories(${SRC_TOP_DIR}/oai_sgw/common)
include_directories(${CMAKE_BINARY_DIR}/s1ap/${ASN1RELDIR})


###############################################################################
# Core Network Utils
###############################################################################

# log.c pulls in intertask_interface.h
include_directories(${SRC_TOP_DIR}/oai_sgw/common/itti)

# log.c pulls in nas_messages_types.h which brings in the entire universe.
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

if (MESSAGE_CHART_GENERATOR)
  add_library(MSC
      ${CMAKE_CURRENT_SOURCE_DIR}/msc/msc.c
      )
  set(MSC_LIB MSC PARENT_SCOPE)
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/msc)

set(CN_UTILS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/async_system.c
    ${CMAKE_CURRENT_SOURCE_DIR}/conversions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/digest.c
    ${CMAKE_CURRENT_SOURCE_DIR}/enum_string.c
    ${CMAKE_CURRENT_SOURCE_DIR}/if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcc_mnc_itu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dynamic_memory_check.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pid_file.c
    ${CMAKE_CURRENT_SOURCE_DIR}/shared_ts_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/TLVEncoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/TLVDecoder.c
    )

if (LOG_OAI)
  set(CN_UTILS_SRC ${CN_UTILS_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/log.c)
endif (LOG_OAI)

add_library(CN_UTILS ${CN_UTILS_SRC})



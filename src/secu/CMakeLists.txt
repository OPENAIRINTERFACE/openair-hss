###############################################################################
# Security library
###############################################################################

# security_types.h
include_directories(${SRC_TOP_DIR}/common)
# liblfds611.h
include_directories(${SRC_TOP_DIR}/common/ITTI)
# log.h
include_directories(${SRC_TOP_DIR}/utils)
# bstrlib.h
include_directories(${SRC_TOP_DIR}/utils/bstr)
include_directories(${SRC_TOP_DIR}/utils/msc)

include_directories(${SRC_TOP_DIR}/nas/ies)
include_directories(${SRC_TOP_DIR}/nas)

set(SECU_CN_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/kdf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/rijndael.c
    ${CMAKE_CURRENT_SOURCE_DIR}/snow3g.c
    ${CMAKE_CURRENT_SOURCE_DIR}/key_nas_deriver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/nas_stream_eea1.c
    ${CMAKE_CURRENT_SOURCE_DIR}/nas_stream_eia1.c
    ${CMAKE_CURRENT_SOURCE_DIR}/nas_stream_eea2.c
    ${CMAKE_CURRENT_SOURCE_DIR}/nas_stream_eia2.c
    )
add_library(SECU_CN ${SECU_CN_SRC})

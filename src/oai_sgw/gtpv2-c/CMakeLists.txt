set(GTPV2C_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/nwgtpv2c-0.11/src)
set(GTPV2C_IE_FORMATTER_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/gtpv2c_ie_formatter/src)
add_library(GTPV2C
    ${GTPV2C_DIR}/NwGtpv2cTrxn.c
    ${GTPV2C_DIR}/NwGtpv2cTunnel.c
    ${GTPV2C_DIR}/NwGtpv2cMsg.c
    ${GTPV2C_DIR}/NwGtpv2cMsgIeParseInfo.c
    ${GTPV2C_DIR}/NwGtpv2cMsgParser.c
    ${GTPV2C_DIR}/NwGtpv2c.c
    ${GTPV2C_IE_FORMATTER_DIR}/gtpv2c_ie_formatter.c 
    )


# common_defs.h for return values
include_directories(${SRC_TOP_DIR}/oai_sgw/common)
# log.h
include_directories(${SRC_TOP_DIR}/oai_sgw/utils)
include_directories(${SRC_TOP_DIR}/oai_sgw/utils/msc)
include_directories(${SRC_TOP_DIR}/oai_sgw/utils/bstr)

# conversions.h
include_directories(${SRC_TOP_DIR}/nas/ies)

# sgw_ie_defs.h
include_directories(${SRC_TOP_DIR}/sgw)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/nwgtpv2c-0.11/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/nwgtpv2c-0.11/shared/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/gtpv2c_ie_formatter/shared/)

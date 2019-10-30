add_list1_option(M2AP_VERSION R15 "M2AP Asn.1 grammar version" R15)
add_definitions(-DASN1_MINIMUM_VERSION=923)

# Common headers
include_directories("${SRC_TOP_DIR}/common")
include_directories("${SRC_TOP_DIR}/common/itti")
include_directories("${SRC_TOP_DIR}/utils")
include_directories("${SRC_TOP_DIR}/mme")

# This is a fork of https://github.com/websnarf/bstrlib
# Evaluate if we want to pull the src directly from github.
include_directories("${SRC_TOP_DIR}/utils/bstr")
include_directories("${SRC_TOP_DIR}/utils/hashtable")

# Possible header leak
include_directories("${SRC_TOP_DIR}/mce_app")

###############################################################################
# A difficulty: asn1c generates C code of a un-predictable list of files
# so, generate the c from asn1c once at cmake run time
# So, if someone modify the asn.1 source file in such as way that it will create
# (so creating new asn.1 objects instead of modifying the object attributes)
# New C code source file, cmake must be re-run (instead of re-running make only)
###############################################################################

set(M2AP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
include_directories("${M2AP_DIR}")

execute_process(COMMAND ${asn1c_call} ${M2AP_C_DIR} ${M2AP_ASN_FILES})
file(GLOB M2AP_source ${M2AP_C_DIR}/*.c)

set(M2AP_OAI_generated
    ${M2AP_C_DIR}/m2ap_decoder.c
    ${M2AP_C_DIR}/m2ap_encoder.c
    ${M2AP_C_DIR}/m2ap_xer_print.c
    ${M2AP_C_DIR}/m2ap_ies_defs.h
    )

file(GLOB m2ap_h ${M2AP_C_DIR}/*.h)
set(m2ap_h ${m2ap_h})

add_custom_command(
    OUTPUT ${M2AP_OAI_generated}
    COMMAND ${asn1c_call} ${M2AP_C_DIR} ${M2AP_ASN_FILES}
    COMMAND python ${M2AP_DIR}/messages/asn1/asn1tostruct.py -f${M2AP_ASN_DIR}/M2AP-PDU-Contents.asn -o${M2AP_C_DIR}
    DEPENDS ${M2AP_ASN_FILES}
)

add_library(M2AP_LIB
    ${M2AP_OAI_generated}
    ${M2AP_source}
    m2ap_common.c
    )


add_library(M2AP_EPC
    ${M2AP_DIR}/m2ap_mce_encoder.c
    ${M2AP_DIR}/m2ap_mce_decoder.c
    ${M2AP_DIR}/m2ap_mce_handlers.c
    ${M2AP_DIR}/m2ap_mce_procedures.c
    ${M2AP_DIR}/m2ap_mce.c
    ${M2AP_DIR}/m2ap_mce_itti_messaging.c
    ${M2AP_DIR}/m2ap_mce_retransmission.c
    ${M2AP_DIR}/m2ap_mce_mbms_sa.c
    )

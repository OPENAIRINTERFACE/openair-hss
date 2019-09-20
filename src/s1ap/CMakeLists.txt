add_list1_option(S1AP_VERSION R10 "S1AP Asn.1 grammar version" R8 R9 R10)
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
# Possibly unused.
include_directories("${SRC_TOP_DIR}/utils/msc")

# Nas message headers
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

# Possible header leak
include_directories("${SRC_TOP_DIR}/mme_app")
include_directories("${SRC_TOP_DIR}/sgw")

###############################################################################
# A difficulty: asn1c generates C code of a un-predictable list of files
# so, generate the c from asn1c once at cmake run time
# So, if someone modify the asn.1 source file in such as way that it will create
# (so creating new asn.1 objects instead of modifying the object attributes)
# New C code source file, cmake must be re-run (instead of re-running make only)
###############################################################################

set(ASN1RELDIR r10.5)
set(S1AP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(asn1_generated_dir ${CMAKE_CURRENT_BINARY_DIR})
set(asn1c_call "${CMAKE_CURRENT_SOURCE_DIR}/generate_asn1")

set(S1AP_ASN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/messages/asn1/${ASN1RELDIR})
set(S1AP_ASN_FILES
    ${S1AP_ASN_DIR}/S1AP-CommonDataTypes.asn
    ${S1AP_ASN_DIR}/S1AP-Constants.asn
    ${S1AP_ASN_DIR}/S1AP-IEs.asn
    ${S1AP_ASN_DIR}/S1AP-PDU.asn
    )
set(S1AP_C_DIR ${asn1_generated_dir}/${ASN1RELDIR})

include_directories("${S1AP_C_DIR}")
include_directories("${S1AP_DIR}")

execute_process(COMMAND ${asn1c_call} ${S1AP_C_DIR} ${S1AP_ASN_FILES})
file(GLOB S1AP_source ${S1AP_C_DIR}/*.c)

set(S1AP_OAI_generated
    ${S1AP_C_DIR}/s1ap_decoder.c
    ${S1AP_C_DIR}/s1ap_encoder.c
    ${S1AP_C_DIR}/s1ap_xer_print.c
    ${S1AP_C_DIR}/s1ap_ies_defs.h
    )

file(GLOB s1ap_h ${S1AP_C_DIR}/*.h)
set(s1ap_h ${s1ap_h})

add_custom_command(
    OUTPUT ${S1AP_OAI_generated}
    COMMAND ${asn1c_call} ${S1AP_C_DIR} ${S1AP_ASN_FILES}
    COMMAND python ${S1AP_DIR}/messages/asn1/asn1tostruct.py -f${S1AP_ASN_DIR}/S1AP-PDU-Contents.asn -o${S1AP_C_DIR}
    DEPENDS ${S1AP_ASN_FILES}
)

add_library(S1AP_LIB
    ${S1AP_OAI_generated}
    ${S1AP_source}
    s1ap_common.c
    )


add_library(S1AP_EPC
    ${S1AP_DIR}/s1ap_mme_encoder.c
    ${S1AP_DIR}/s1ap_mme_decoder.c
    ${S1AP_DIR}/s1ap_mme_handlers.c
    ${S1AP_DIR}/s1ap_mme_nas_procedures.c
    ${S1AP_DIR}/s1ap_mme.c
    ${S1AP_DIR}/s1ap_mme_itti_messaging.c
    ${S1AP_DIR}/s1ap_mme_retransmission.c
    ${S1AP_DIR}/s1ap_mme_ta.c
    )

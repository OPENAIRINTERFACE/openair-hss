add_definitions("-DPACKAGE_NAME=\"MME\"")
#TODO: TBD
# set in parent CMakeLists.txt: add_definitions("-DPACKAGE_VERSION=\"0.1\"")
# set in parent CMakeLists.txt: add_definitions("-DPACKAGE_BUGREPORT=\"TBD\"")

# Dependencies
pkg_search_module(OPENSSL openssl REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIRS})

pkg_search_module(CRYPTO libcrypto REQUIRED)
include_directories(${CRYPTO_INCLUDE_DIRS})


set(ASN1RELDIR r10.5)
include_directories(${SRC_TOP_DIR}/common)
include_directories(${SRC_TOP_DIR}/common/itti)
include_directories(${SRC_TOP_DIR}/common/message_utils)
include_directories(${CMAKE_BINARY_DIR}/s1ap/${ASN1RELDIR})
include_directories(${SRC_TOP_DIR}/secu)
include_directories(${SRC_TOP_DIR}/sgw)
include_directories(${SRC_TOP_DIR}/s1ap)
include_directories(${SRC_TOP_DIR}/utils)
include_directories(${SRC_TOP_DIR}/utils/bstr)
include_directories(${SRC_TOP_DIR}/utils/hashtable)
include_directories(${SRC_TOP_DIR}/utils/msc)
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
include_directories("${SRC_TOP_DIR}/mce_app")
include_directories("${SRC_TOP_DIR}/mme_app")
include_directories("${SRC_TOP_DIR}/s1ap/messages/asn1/${ASN1RELDIR}")
include_directories("${SRC_TOP_DIR}/s1ap")
include_directories("${SRC_TOP_DIR}/s10")
add_library(MME_APP
    mme_app_capabilities.c
    mme_app_apn_selection.c
    mme_app_bearer.c
    mme_app_bearer_context.c
    mme_app_capabilities.c
    mme_app_context.c
    mme_app_detach.c
    mme_app_edns_emulation.c
    mme_app_itti_messaging.c
    mme_app_location.c
    mme_app_main.c
    mme_app_pdn_context.c
    mme_app_procedures.c
    mme_app_esm_procedures.c
    mme_app_sgw_selection.c
    mme_app_statistics.c
    mme_app_transport.c
    mme_app_ue_context.c
    mme_app_session_context.c
    mme_app_wrr_selection.c
    mme_config.c
    )

include_directories(${SRC_TOP_DIR}/sctp)
include_directories(${SRC_TOP_DIR}/udp)
include_directories(${SRC_TOP_DIR}/s11)
include_directories(${SRC_TOP_DIR}/s6a)

# MME executable
################################
add_executable(mme
    ../oai_mme/oai_mme_log.c
    ../oai_mme/oai_mme.c
    ../common/common_types.c
    ../common/itti_free_defined_msg.c
    ../nas/nas_mme_task.c
    )

target_link_libraries (mme
    -Wl,--start-group
    LIB_NAS_MME S1AP_LIB S1AP_EPC S11_MME S10 GTPV2C SCTP_SERVER UDP_SERVER SECU_CN  S6A MME_APP MCE_APP
            ${MSC_LIB} ITTI  3GPP_TYPES CN_UTILS
            HASHTABLE BSTR
    -Wl,--end-group
    pthread m sctp  rt crypt ${LFDS} ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES}
      ${NETTLE_LIBRARIES} ${CONFIG_LIBRARIES} gnutls fdproto fdcore 
    )

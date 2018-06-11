
set(ASN1RELDIR r10.5)
include_directories(${SRC_TOP_DIR}/common)
include_directories(${SRC_TOP_DIR}/common/itti)
include_directories(${CMAKE_BINARY_DIR}/s1ap/${ASN1RELDIR})
include_directories(${SRC_TOP_DIR}/gtpv1-u)
include_directories(${SRC_TOP_DIR}/rpc_client)
include_directories(${SRC_TOP_DIR}/sgw)
include_directories(${SRC_TOP_DIR}/utils)
include_directories(${SRC_TOP_DIR}/utils/bstr)
include_directories(${SRC_TOP_DIR}/utils/hashtable)
include_directories(${SRC_TOP_DIR}/utils/msc)

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

add_definitions("-DPACKAGE_NAME=\"S/P-GW\"")
#TODO: TBD
# set in parent CMakeLists.txt: add_definitions("-DPACKAGE_VERSION=\"0.1\"")
# set in parent CMakeLists.txt: add_definitions("-DPACKAGE_BUGREPORT=\"TBD\"")

add_library (SGW
  pgw_config.c
  pgw_lite_paa.c
  pgw_pcef_emulation.c
  pgw_pco.c
  pgw_procedures.c
  pgw_ue_ip_address_alloc.c
  s11_causes.c
  sgw_config.c
  sgw_context_manager.c
  sgw_downlink_data_notification.c
  sgw_handlers.c
  sgw_handler_gtpu.c
  sgw_task.c
  spgw_config.c
  )


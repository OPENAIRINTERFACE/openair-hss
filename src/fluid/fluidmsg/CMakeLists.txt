macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
  else ()
    set (CMAKE_CXX_STANDARD 11)
  endif ()
endmacro(use_cxx11)

use_cxx11()


#OF MSG DRIVER
set(FLUIDMSG_MOD_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(${FLUIDMSG_MOD_DIR})
include_directories(${FLUIDMSG_MOD_DIR}/fluid)
include_directories(${FLUIDMSG_MOD_DIR}/fluid/ofcommon)
include_directories(${FLUIDMSG_MOD_DIR}/fluid/of10)
include_directories(${FLUIDMSG_MOD_DIR}/fluid/of13)
include_directories(${FLUIDMSG_MOD_DIR}/fluid/util)


set(FLUIDMSG_MOD_include_files
  ${FLUIDMSG_MOD_DIR}
  ${FLUIDMSG_MOD_DIR}/fluid
  ${FLUIDMSG_MOD_DIR}/fluid/ofcommon
  ${FLUIDMSG_MOD_DIR}/fluid/of10
  ${FLUIDMSG_MOD_DIR}/fluid/of13
  ${FLUIDMSG_MOD_DIR}/fluid/util
  )
set(FLUIDMSG_MOD_src_files
  ${FLUIDMSG_MOD_DIR}/fluid/util/ethaddr.cc
  ${FLUIDMSG_MOD_DIR}/fluid/util/ipaddr.cc
  ${FLUIDMSG_MOD_DIR}/fluid/ofcommon/msg.cc
  ${FLUIDMSG_MOD_DIR}/fluid/ofcommon/action.cc
  ${FLUIDMSG_MOD_DIR}/fluid/ofcommon/common.cc
  ${FLUIDMSG_MOD_DIR}/fluid/of10/of10action.cc
  ${FLUIDMSG_MOD_DIR}/fluid/of10/of10common.cc
  ${FLUIDMSG_MOD_DIR}/fluid/of10/of10match.cc
  ${FLUIDMSG_MOD_DIR}/fluid/of13/of13instruction.cc
  ${FLUIDMSG_MOD_DIR}/fluid/of13/of13action.cc
  ${FLUIDMSG_MOD_DIR}/fluid/of13/of13meter.cc
  ${FLUIDMSG_MOD_DIR}/fluid/of13/of13common.cc
  ${FLUIDMSG_MOD_DIR}/fluid/of13/of13match.cc
  ${FLUIDMSG_MOD_DIR}/fluid/of10msg.cc
  ${FLUIDMSG_MOD_DIR}/fluid/of13msg.cc
  )

add_library(FLUIDMSG_MOD ${FLUIDMSG_MOD_src_files})

target_link_libraries (FLUIDMSG_MOD
  event
  pthread
  ssl
  crypto
  event_openssl
  event_pthreads
)



macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
  else ()
    set (CMAKE_CXX_STANDARD 11)
  endif ()
endmacro(use_cxx11)

use_cxx11()

set(FLUIDBASE_MOD_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(${FLUIDBASE_MOD_DIR})
include_directories(${FLUIDBASE_MOD_DIR}/fluid)
include_directories(${FLUIDBASE_MOD_DIR}/fluid/base)

set(FLUIDBASE_MOD_include_files
  "${FLUIDBASE_MOD_DIR}"
  "${FLUIDBASE_MOD_DIR}/fluid"
  "${FLUIDBASE_MOD_DIR}/fluid/base"
  )
  
set(FLUIDBASE_MOD_src_files
  ${FLUIDBASE_MOD_DIR}/fluid/base/EventLoop.cc
  ${FLUIDBASE_MOD_DIR}/fluid/base/BaseOFConnection.cc
  ${FLUIDBASE_MOD_DIR}/fluid/base/BaseOFServer.cc
  ${FLUIDBASE_MOD_DIR}/fluid/OFConnection.cc
  ${FLUIDBASE_MOD_DIR}/fluid/OFServerSettings.cc
  ${FLUIDBASE_MOD_DIR}/fluid/OFServer.cc
  ${FLUIDBASE_MOD_DIR}/fluid/TLS.cc
  )

add_library(FLUIDBASE_MOD ${FLUIDBASE_MOD_src_files})



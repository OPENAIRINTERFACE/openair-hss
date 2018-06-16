# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.

# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

add_compile_options(-std=c++11)

#set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set (CMAKE_CXX_FLAGS "-Wno-write-strings -Wno-literal-suffix")
add_definitions(-DDEBUG_IS_ON=1)

include_directories("${SRC_TOP_DIR}/oai_sgw/common")
include_directories("${SRC_TOP_DIR}/oai_sgw/utils")
include_directories("${SRC_TOP_DIR}/oai_sgw/utils/bstr")
include_directories("${SRC_TOP_DIR}/oai_sgw/utils/msc")
include_directories("${SRC_TOP_DIR}/sgw")
include_directories("${SRC_TOP_DIR}/openflow/controller")
include_directories("${SRC_TOP_DIR}/fluid/fluidbase")
include_directories("${SRC_TOP_DIR}/fluid/fluidmsg")

add_library(OPENFLOW_CONTROLLER
  ArpApplication.h
  ArpApplication.cpp
  ControllerMain.h
  ControllerMain.cpp
  OpenflowController.h
  OpenflowController.cpp
  PacketInSwitchApplication.h
  PacketInSwitchApplication.cpp
  PagingApplication.h
  PagingApplication.cpp
  ControllerEvents.h
  ControllerEvents.cpp
  BaseApplication.h
  BaseApplication.cpp
  OpenflowMessenger.h
  OpenflowMessenger.cpp
  GTPApplication.h
  GTPApplication.cpp
  IMSIEncoder.h
  IMSIEncoder.cpp
  )

target_link_libraries (OPENFLOW_CONTROLLER FLUIDBASE_MOD FLUIDMSG_MOD)

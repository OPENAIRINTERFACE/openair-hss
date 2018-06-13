################################################################################
# Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The OpenAirInterface Software Alliance licenses this file to You under 
# the Apache License, Version 2.0  (the "License"); you may not use this file
# except in compliance with the License.  
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#-------------------------------------------------------------------------------
# For more information about the OpenAirInterface (OAI) Software Alliance:
#      contact@openairinterface.org
################################################################################
cmake_minimum_required (VERSION 3.0.2)
# Override options for MME
set (  BYTE_ORDER=LITTLE_ENDIAN )
set (  DISABLE_ITTI_DETECT_SUB_TASK_ID True )
set (  DISPLAY_LICENCE_INFO            False )
set (  ITTI_TASK_STACK_SIZE            2097152 )
set (  ITTI_LITE                       False )
set (  LOG_OAI                         True )
set (  LOG_OAI_CLEAN_HARD              False )
set (  MESSAGE_CHART_GENERATOR         False )
set (  MEMORY_CHECK                    False )
set (  MME_BUILD                       True )
set (  S1AP_DEBUG_LIST                 False )
set (  SECU_DEBUG                      False )
set (  SCTP_DUMP_LIST                  False )
set (  TRACE_HASHTABLE                 False )
set (  TRACE_3GPP_SPEC                 False )

#############################################
# Base directories, compatible with legacy OAI building
################################################
set (OPENAIRCN_DIR   $ENV{OPENAIRCN_DIR})
set (BUILD_TOP_DIR   ${OPENAIRCN_DIR}/build)
set (SRC_TOP_DIR     $ENV{OPENAIRCN_DIR}/src)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../src/CMakeLists.txt)

pkg_search_module(NETTLE nettle REQUIRED)
if(NOT ${NETTLE_FOUND})
  message( FATAL_ERROR "PACKAGE nettle not found: some targets will fail. Run SCRIPTS/build_mme -i or SCRIPTS/build_hss -i  again!")
else()
  include_directories(${NETTLE_INCLUDE_DIRS})
endif()

string(REGEX REPLACE "([0-9]+).*" "\\1" NETTLE_VERSION_MAJOR ${NETTLE_VERSION})
string(REGEX REPLACE "[0-9]+\\.([0-9]+).*" "\\1" NETTLE_VERSION_MINOR ${NETTLE_VERSION})
message ("NETTLE_VERSION_INSTALLED  = ${NETTLE_VERSION}")
message ("NETTLE_VERSION_MAJOR = ${NETTLE_VERSION_MAJOR}")
message ("NETTLE_VERSION_MINOR = ${NETTLE_VERSION_MINOR}")
if ("${NETTLE_VERSION_MAJOR}" STREQUAL "" OR "${NETTLE_VERSION_MINOR}" STREQUAL "")
    message( FATAL_ERROR "The nettle version is not detected properly..." )
endif()

add_definitions("-DNETTLE_VERSION_MAJOR=${NETTLE_VERSION_MAJOR}")
add_definitions("-DNETTLE_VERSION_MINOR=${NETTLE_VERSION_MINOR}")

pkg_search_module(LIBXML2 libxml-2.0 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIRS})

pkg_search_module(OPENSSL openssl REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIRS})

pkg_search_module(CRYPTO libcrypto REQUIRED)
include_directories(${CRYPTO_INCLUDE_DIRS})

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../src/gtpv1-u ${CMAKE_CURRENT_BINARY_DIR}/gtpv1-u)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../src/mme_app ${CMAKE_CURRENT_BINARY_DIR}/mme_app)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../src/nas ${CMAKE_CURRENT_BINARY_DIR}/nas)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../src/sctp ${CMAKE_CURRENT_BINARY_DIR}/sctp)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../src/secu ${CMAKE_CURRENT_BINARY_DIR}/secu)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../src/s1ap ${CMAKE_CURRENT_BINARY_DIR}/s1ap)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../src/s6a ${CMAKE_CURRENT_BINARY_DIR}/s6a)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../src/s10 ${CMAKE_CURRENT_BINARY_DIR}/s10)


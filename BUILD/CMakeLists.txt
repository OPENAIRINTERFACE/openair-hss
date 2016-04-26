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
# Author: laurent THOMAS, Lionel GAUTHIER
###############################################################################

cmake_minimum_required (VERSION 2.8)

#############################################
# Base directories, compatible with legacy OAI building
################################################
set (OPENAIRCN_DIR   $ENV{OPENAIRCN_DIR})
set (BUILD_TOP_DIR   ${OPENAIRCN_DIR}/BUILD)
set (OPENAIRCN_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY})

project (OpenAirInterface)

###########################################
# macros to define options as there is numerous options in oai
################################################
macro(add_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  add_definitions("-D${name}=${value}")
endmacro(add_option)

macro(add_boolean_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  set_property(CACHE ${name} PROPERTY TYPE BOOL)
  if (${value})
    add_definitions("-D${name}=1")
  else (${value})
    add_definitions("-D${name}=0")
  endif (${value})
endmacro(add_boolean_option)

macro(add_integer_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  add_definitions("-D${name}=${value}")
endmacro(add_integer_option)

macro(add_list1_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  set_property(CACHE ${name} PROPERTY STRINGS ${ARGN})
  if(NOT "${value}" STREQUAL "False")
    add_definitions("-D${name}=${value}")
  endif()
endmacro(add_list1_option)

macro(add_list2_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  set_property(CACHE ${name} PROPERTY STRINGS ${ARGN})
  if(NOT "${value}" STREQUAL "False")
    add_definitions("-D${value}=1")
  endif()
endmacro(add_list2_option)

macro(add_list_string_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  set_property(CACHE ${name} PROPERTY STRINGS ${ARGN})
  if(NOT "${value}" STREQUAL "False")
    add_definitions("-D${name}=\"${value}\"")
  endif()
endmacro(add_list_string_option)
####################################################
# compilation flags
#############################################
if (CMAKE_BUILD_TYPE STREQUAL "")
   set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()
add_list_string_option(CMAKE_BUILD_TYPE "RelWithDebInfo" "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." Debug Release RelWithDebInfo MinSizeRel)
Message("Build type is ${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  add_boolean_option(LOG_OAI True         "Thread safe logging API")
  add_boolean_option(LOG_OAI_MINIMAL True "Thread safe logging API, log only levels above NOTICE")
endif()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_boolean_option(LOG_OAI True "Thread safe logging API")
endif()


Message("Architecture is ${CMAKE_SYSTEM_PROCESSOR}")
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
  set(C_FLAGS_PROCESSOR "-gdwarf-2 -mfloat-abi=hard -mfpu=neon -lgcc -lrt")
else (CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
  set(C_FLAGS_PROCESSOR "-msse4.2")
endif()
#
set(CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} ${C_FLAGS_PROCESSOR} -std=gnu99 -Wall -Wstrict-prototypes -fno-strict-aliasing -rdynamic -funroll-loops -Wno-packed-bitfield-compat -fPIC ")
# add autoTOOLS definitions that were maybe used!
set(CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_FCNTL_H=1 -DHAVE_ARPA_INET_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_SYS_SOCKET_H=1 -DHAVE_STRERROR=1 -DHAVE_SOCKET=1 -DHAVE_MEMSET=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_STDLIB_H=1 -DHAVE_MALLOC=1 -DHAVE_LIBSCTP"
)
# set a flag for changes in the source code
# these changes are related to hardcoded path to include .h files
add_definitions(-DCMAKER)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -fstack-protector-all -g -DMALLOC_CHECK_=3 -DDEBUG_IS_ON=1 -DTRACE_IS_ON=1 -O0")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS} -g -DMALLOC_CHECK_=3  -DDEBUG_IS_ON -O1") 

set(GIT_BRANCH        "UNKNOWN")
set(GIT_COMMIT_HASH   "UNKNOWN")
set(GIT_COMMIT_DATE   "UNKNOWN")

find_package(Git)
if(GIT_FOUND)
  message("git found: ${GIT_EXECUTABLE}")
  # Get the current working branch
  execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # Get the latest abbreviated commit hash of the working branch
  execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  # Get the latest commit date of the working branch
  execute_process(
    COMMAND git log -1 --format=%cd
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()


add_list_string_option(PACKAGE_NAME "NotDefined" "As per attribute name")
add_definitions("-DPACKAGE_VERSION=\"Branch: ${GIT_BRANCH} Abrev. Hash: ${GIT_COMMIT_HASH} Date: ${GIT_COMMIT_DATE}\"")
add_definitions("-DPACKAGE_BUGREPORT=\"openaircn-user@lists.eurecom.fr\"")



#########################################
# Debug and build related options
#########################################
add_boolean_option( DAEMONIZE                       False    "Fork executable if true")

add_boolean_option( DISABLE_ITTI_DETECT_SUB_TASK_ID False    " ")
add_boolean_option( DISPLAY_LICENCE_INFO            False    "If a module has a licence banner to show")
add_boolean_option( ENABLE_ITTI                     True     "ITTI is internal messaging, should remain enabled for most targets")
add_boolean_option( ENABLE_ITTI_ANALYZER            False    "ITTI Analyzer is a GUI based on GTK that displays the ITTI messages exchanged between tasks")
add_integer_option( ITTI_TASK_STACK_SIZE            0        "pthread allocated stack size in bytes of an ITTI task, if 0, use default stack size ") 
add_integer_option( ITTI_LITE                       0        "Do not use ITTI systematically for each message exchanged between layer modules") 
add_boolean_option( MESSAGE_CHART_GENERATOR         False    "For generating sequence diagrams")
# NAS LAYER OPTIONS
##########################
add_boolean_option( EPC_BUILD                       False    "BUILD MME-xGW executable")
add_boolean_option( GTPV1U_LINEAR_TEID_ALLOCATION   False    "Teid allocation id mode versus pseudo random")
# S1AP LAYER OPTIONS
##########################
add_boolean_option(S1AP_DEBUG_LIST                  False    "Traces, option to be removed soon")
# SCTP LAYER OPTIONS
##########################
add_boolean_option(SCTP_DUMP_LIST                   False    "Traces, option to be removed soon")

add_boolean_option( TRACE_HASHTABLE                 False    "Trace hashtables operations ")
add_boolean_option( LOG_OAI                         False    "Thread safe logging utility")
add_boolean_option( LOG_OAI_CLEAN_HARD              False    "Thread safe logging utility option for cleaning inner structs")
add_boolean_option( SECU_DEBUG                      False    "Traces, option to be removed soon")

add_boolean_option( NAS_FORCE_REJECT_SR             True     "TEMP, TO be disabled for community after development and test: Force the EMM NAS layer to respond to any SR by reject")
add_boolean_option( NAS_FORCE_REJECT_TAU            True     "TEMP, TO be disabled for community after development and test: Force the EMM NAS layer to respond to any TAU Request by reject")


set (ITTI_DIR ${OPENAIRCN_DIR}/SRC/COMMON/ITTI)
if (${ENABLE_ITTI})
  add_library(ITTI
    # add .h files if depend on (this one is generated)
    ${ITTI_DIR}/intertask_interface.h
    ${ITTI_DIR}/intertask_interface.c
    ${ITTI_DIR}/backtrace.c
    ${ITTI_DIR}/memory_pools.c
    ${ITTI_DIR}/signals.c
    ${ITTI_DIR}/timer.c
    )
if (${ENABLE_ITTI_ANALYZER})
add_library(ITTI
    ${ITTI_DIR}/intertask_interface_dump.c )
endif (${ENABLE_ITTI_ANALYZER})
    
  set(ITTI_LIB ITTI)
endif (${ENABLE_ITTI})


#############################
# ASN.1 grammar C code generation & dependancies
################################
# A difficulty: asn1c generates C code of a un-predictable list of files
# so, generate the c from asn1c once at cmake run time
# So, if someone modify the asn.1 source file in such as way that it will create
# (so creating new asn.1 objects instead of modifying the object attributes)
# New C code source file, cmake must be re-run (instead of re-running make only)
#############
set(asn1c_call "${BUILD_TOP_DIR}/TOOLS/generate_asn1")
set(asn1_generated_dir ${OPENAIRCN_BIN_DIR})


# S1AP
# Same limitation as described in RRC: unknown generated file list
# so we generate it at cmake time
##############
add_list1_option(S1AP_VERSION R10 "S1AP Asn.1 grammar version" R8 R9 R10)

set(S1AP_DIR ${OPENAIRCN_DIR}/SRC/S1AP)
set(ASN1RELDIR R10.5)

set(S1AP_ASN_DIR ${S1AP_DIR}/MESSAGES/ASN1/${ASN1RELDIR})
set(S1AP_ASN_FILES
  ${S1AP_ASN_DIR}/S1AP-CommonDataTypes.asn
  ${S1AP_ASN_DIR}/S1AP-Constants.asn
  ${S1AP_ASN_DIR}/S1AP-IEs.asn
  ${S1AP_ASN_DIR}/S1AP-PDU.asn
  )
set(S1AP_C_DIR ${asn1_generated_dir}/${ASN1RELDIR})

execute_process(COMMAND ${asn1c_call} ${S1AP_C_DIR} ${S1AP_ASN_FILES})
file(GLOB S1AP_source ${S1AP_C_DIR}/*.c)

set(S1AP_OAI_generated
  ${S1AP_C_DIR}/s1ap_decoder.c
  ${S1AP_C_DIR}/s1ap_encoder.c
  ${S1AP_C_DIR}/s1ap_xer_print.c
  ${S1AP_C_DIR}/s1ap_ies_defs.h
  )
file(GLOB s1ap_h ${S1AP_C_DIR}/*.h)
set(s1ap_h ${s1ap_h} )

add_custom_command (
  OUTPUT ${S1AP_OAI_generated}
  COMMAND ${asn1c_call} ${S1AP_C_DIR} ${S1AP_ASN_FILES}
  COMMAND python ${S1AP_DIR}/MESSAGES/ASN1/asn1tostruct.py -f${S1AP_ASN_DIR}/S1AP-PDU-Contents.asn -o${S1AP_C_DIR}
  DEPENDS ${S1AP_ASN_FILES}
  )
add_library(S1AP_LIB
  ${S1AP_OAI_generated}
  ${S1AP_source}
  ${S1AP_DIR}/s1ap_common.c
  )

include_directories ("${S1AP_C_DIR}")
include_directories ("${S1AP_DIR}")


add_library(S1AP_EPC
  ${S1AP_C_DIR}/s1ap_ies_defs.h
  ${S1AP_DIR}/s1ap_mme_encoder.c
  ${S1AP_DIR}/s1ap_mme_decoder.c
  ${S1AP_DIR}/s1ap_mme_handlers.c
  ${S1AP_DIR}/s1ap_mme_nas_procedures.c
  ${S1AP_DIR}/s1ap_mme.c
  ${S1AP_DIR}/s1ap_mme_itti_messaging.c
  ${S1AP_DIR}/s1ap_mme_retransmission.c
  ${S1AP_DIR}/s1ap_mme_ta.c
  )




 # add the binary tree to the search path for include files
#######################################################
# We will find ConfigOAI.h after generation in target directory
include_directories("${OPENAIRCN_BIN_DIR}")
# add directories to find all include files
# the internal rule is to use generic names such as defs.h
# but to make it uniq name as adding the relative path in the include directtive
# example: #include "RRC/LITE/defs.h"
#find_path (include_dirs_all *.h ${OPENAIRCN_DIR}/SRC)
#find_path (include_dirs_all *.h PATHS /usr/include NO_CMAKE_PATH)
#include_directories("${include_dirs_all}")


include_directories("${OPENAIRCN_DIR}/SRC/COMMON")
include_directories("${OPENAIRCN_DIR}/SRC/UTILS")

include_directories("${OPENAIRCN_DIR}/SRC/COMMON/ITTI")
include_directories("${OPENAIRCN_DIR}/SRC/NAS/")
include_directories("${OPENAIRCN_DIR}/SRC/NAS/API/MME")
include_directories("${OPENAIRCN_DIR}/SRC/NAS/API/NETWORK")
include_directories("${OPENAIRCN_DIR}/SRC/NAS/EMM/MSG")
include_directories("${OPENAIRCN_DIR}/SRC/NAS/ESM/MSG")
include_directories("${OPENAIRCN_DIR}/SRC/NAS/IES")
include_directories("${OPENAIRCN_DIR}/SRC/NAS/UTIL")
include_directories("${OPENAIRCN_DIR}/SRC/SECU")
include_directories("${OPENAIRCN_DIR}/SRC/SCTP")
include_directories("${OPENAIRCN_DIR}/SRC/S1AP")
include_directories("${OPENAIRCN_DIR}/SRC/UDP")
include_directories("${OPENAIRCN_DIR}/SRC/GTPV1-U")
include_directories("${OPENAIRCN_DIR}/SRC/UTILS/HASHTABLE")
include_directories("${OPENAIRCN_DIR}/SRC/UTILS/LFDS/liblfds6.1.1/liblfds611/inc")
include_directories("${OPENAIRCN_DIR}/SRC/UTILS/LFDS/liblfds7.0.0/liblfds700/inc")
include_directories("${OPENAIRCN_DIR}/SRC/UTILS/MSC")
include_directories("${OPENAIRCN_DIR}/SRC/SGW")
include_directories("${OPENAIRCN_DIR}/SRC/MME_APP")
include_directories("${OPENAIRCN_DIR}/SRC/S6A")
include_directories("${OPENAIRCN_DIR}/SRC")

# Utilities Library
################
add_library(HASHTABLE
  ${OPENAIRCN_DIR}/SRC/UTILS/HASHTABLE/hashtable.c
  ${OPENAIRCN_DIR}/SRC/UTILS/HASHTABLE/obj_hashtable.c
)
include_directories(${OPENAIRCN_DIR}/SRC/UTILS/HASHTABLE)

if (MESSAGE_CHART_GENERATOR)
  add_library(MSC  
    ${OPENAIRCN_DIR}/SRC/UTILS/MSC/msc.c
  )
  set(MSC_LIB MSC)
endif()
include_directories(${OPENAIRCN_DIR}/SRC/UTILS/MSC/msc)


set(SECU_CN_SRC
  ${OPENAIRCN_DIR}/SRC/SECU/kdf.c
  ${OPENAIRCN_DIR}/SRC/SECU/rijndael.c
  ${OPENAIRCN_DIR}/SRC/SECU/snow3g.c
  ${OPENAIRCN_DIR}/SRC/SECU/key_nas_deriver.c
  ${OPENAIRCN_DIR}/SRC/SECU/nas_stream_eea1.c
  ${OPENAIRCN_DIR}/SRC/SECU/nas_stream_eia1.c
  ${OPENAIRCN_DIR}/SRC/SECU/nas_stream_eea2.c
  ${OPENAIRCN_DIR}/SRC/SECU/nas_stream_eia2.c
  )
add_library(SECU_CN ${SECU_CN_SRC})


set(CN_UTILS_SRC
  ${OPENAIRCN_DIR}/SRC/UTILS/conversions.c
  ${OPENAIRCN_DIR}/SRC/UTILS/enum_string.c
  ${OPENAIRCN_DIR}/SRC/UTILS/mme_config.c
  ${OPENAIRCN_DIR}/SRC/UTILS/mcc_mnc_itu.c
  ${OPENAIRCN_DIR}/SRC/UTILS/dynamic_memory_check.c
  ${OPENAIRCN_DIR}/SRC/UTILS/pid_file.c
  )

if (LOG_OAI)
  set(CN_UTILS_SRC   ${CN_UTILS_SRC}   ${OPENAIRCN_DIR}/SRC/UTILS/log.c )
endif(LOG_OAI)

add_library(CN_UTILS ${CN_UTILS_SRC})


# CN libs
##########################
set(GTPV1U_DIR ${OPENAIRCN_DIR}/SRC/GTPV1-U)
set (GTPV1U_SRC
  ${GTPV1U_DIR}/gtpv1u_teid_pool.c
)
add_library(GTPV1U ${GTPV1U_SRC})

set(GTPV2C_DIR  ${OPENAIRCN_DIR}/SRC/GTPV2-C/nwgtpv2c-0.11/src)
add_library(GTPV2C
  ${GTPV2C_DIR}/NwGtpv2cTrxn.c
  ${GTPV2C_DIR}/NwGtpv2cTunnel.c
  ${GTPV2C_DIR}/NwGtpv2cMsg.c
  ${GTPV2C_DIR}/NwGtpv2cMsgIeParseInfo.c
  ${GTPV2C_DIR}/NwGtpv2cMsgParser.c
  ${GTPV2C_DIR}/NwGtpv2c.c
  )
include_directories(${OPENAIRCN_DIR}/SRC/GTPV2-C/nwgtpv2c-0.11/include/)
include_directories(${OPENAIRCN_DIR}/SRC/GTPV2-C/nwgtpv2c-0.11/shared/)


add_library(SCTP_SERVER
  ${OPENAIRCN_DIR}/SRC/SCTP/sctp_common.c
  ${OPENAIRCN_DIR}/SRC/SCTP/sctp_itti_messaging.c
  ${OPENAIRCN_DIR}/SRC/SCTP/sctp_primitives_server.c
  )


add_library(UDP_SERVER ${OPENAIRCN_DIR}/SRC/UDP/udp_primitives_server.c)

set(S11_DIR ${OPENAIRCN_DIR}/SRC/S11)
add_library(S11
  ${S11_DIR}/s11_common.c
  ${S11_DIR}/s11_ie_formatter.c
  ${S11_DIR}/s11_mme_task.c
  ${S11_DIR}/s11_sgw.c
  ${S11_DIR}/s11_mme_session_manager.c
  ${S11_DIR}/s11_sgw_session_manager.c
  ${S11_DIR}/s11_sgw_bearer_manager.c
)
include_directories(${S11_DIR})

set(S6A_DIR ${OPENAIRCN_DIR}/SRC/S6A)
add_library(S6A
  ${S6A_DIR}/s6a_auth_info.c
  ${S6A_DIR}/s6a_dict.c
  ${S6A_DIR}/s6a_error.c
  ${S6A_DIR}/s6a_peer.c
  ${S6A_DIR}/s6a_subscription_data.c
  ${S6A_DIR}/s6a_task.c
  ${S6A_DIR}/s6a_up_loc.c
  )

set(SGW_DIR ${OPENAIRCN_DIR}/SRC/SGW)
add_library (SGW
  ${SGW_DIR}/spgw_config.c
  ${SGW_DIR}/s11_causes.c
  ${SGW_DIR}/sgw_task.c
  ${SGW_DIR}/sgw_handlers.c
  ${SGW_DIR}/sgw_context_manager.c
  ${SGW_DIR}/pgw_lite_paa.c
  )

set(MME_DIR ${OPENAIRCN_DIR}/SRC/MME_APP)
add_library(MME_APP
  ${MME_DIR}/mme_app_capabilities.c
  ${MME_DIR}/mme_app_context.c
  ${MME_DIR}/mme_app_if_nas_transport.c
  ${MME_DIR}/mme_app_main.c
  ${MME_DIR}/mme_app_bearer.c
  ${MME_DIR}/mme_app_authentication.c
  ${MME_DIR}/mme_app_detach.c
  ${MME_DIR}/mme_app_location.c
  ${MME_DIR}/mme_app_transport.c
  ${MME_DIR}/mme_app_statistics.c
  ${MME_DIR}/s6a_2_nas_cause.c
  )


set(NAS_SRC ${OPENAIRCN_DIR}/SRC/NAS/)
set(libnas_api_OBJS
  ${NAS_SRC}API/NETWORK/as_message.c
  ${NAS_SRC}API/NETWORK/nas_message.c
  ${NAS_SRC}API/NETWORK/network_api.c
  )
  
set(libnas_emm_msg_OBJS
  ${NAS_SRC}EMM/MSG/AttachAccept.c
  ${NAS_SRC}EMM/MSG/AttachComplete.c
  ${NAS_SRC}EMM/MSG/AttachReject.c
  ${NAS_SRC}EMM/MSG/AttachRequest.c
  ${NAS_SRC}EMM/MSG/AuthenticationFailure.c
  ${NAS_SRC}EMM/MSG/AuthenticationReject.c
  ${NAS_SRC}EMM/MSG/AuthenticationRequest.c
  ${NAS_SRC}EMM/MSG/AuthenticationResponse.c
  ${NAS_SRC}EMM/MSG/CsServiceNotification.c
  ${NAS_SRC}EMM/MSG/DetachAccept.c
  ${NAS_SRC}EMM/MSG/DetachRequest.c
  ${NAS_SRC}EMM/MSG/DownlinkNasTransport.c
  ${NAS_SRC}EMM/MSG/EmmInformation.c
  ${NAS_SRC}EMM/MSG/emm_msg.c
  ${NAS_SRC}EMM/MSG/EmmStatus.c
  ${NAS_SRC}EMM/MSG/ExtendedServiceRequest.c
  ${NAS_SRC}EMM/MSG/GutiReallocationCommand.c
  ${NAS_SRC}EMM/MSG/GutiReallocationComplete.c
  ${NAS_SRC}EMM/MSG/IdentityRequest.c
  ${NAS_SRC}EMM/MSG/IdentityResponse.c
  ${NAS_SRC}EMM/MSG/SecurityModeCommand.c
  ${NAS_SRC}EMM/MSG/SecurityModeComplete.c
  ${NAS_SRC}EMM/MSG/SecurityModeReject.c
  ${NAS_SRC}EMM/MSG/ServiceReject.c
  ${NAS_SRC}EMM/MSG/ServiceRequest.c
  ${NAS_SRC}EMM/MSG/TrackingAreaUpdateAccept.c
  ${NAS_SRC}EMM/MSG/TrackingAreaUpdateComplete.c
  ${NAS_SRC}EMM/MSG/TrackingAreaUpdateReject.c
  ${NAS_SRC}EMM/MSG/TrackingAreaUpdateRequest.c
  ${NAS_SRC}EMM/MSG/UplinkNasTransport.c
)
  
set(libnas_esm_msg_OBJS
  ${NAS_SRC}ESM/MSG/ActivateDedicatedEpsBearerContextAccept.c
  ${NAS_SRC}ESM/MSG/ActivateDedicatedEpsBearerContextReject.c
  ${NAS_SRC}ESM/MSG/ActivateDedicatedEpsBearerContextRequest.c
  ${NAS_SRC}ESM/MSG/ActivateDefaultEpsBearerContextAccept.c
  ${NAS_SRC}ESM/MSG/ActivateDefaultEpsBearerContextReject.c
  ${NAS_SRC}ESM/MSG/ActivateDefaultEpsBearerContextRequest.c
  ${NAS_SRC}ESM/MSG/BearerResourceAllocationReject.c
  ${NAS_SRC}ESM/MSG/BearerResourceAllocationRequest.c
  ${NAS_SRC}ESM/MSG/BearerResourceModificationReject.c
  ${NAS_SRC}ESM/MSG/BearerResourceModificationRequest.c
  ${NAS_SRC}ESM/MSG/DeactivateEpsBearerContextAccept.c
  ${NAS_SRC}ESM/MSG/DeactivateEpsBearerContextRequest.c
  ${NAS_SRC}ESM/MSG/EsmInformationRequest.c
  ${NAS_SRC}ESM/MSG/EsmInformationResponse.c
  ${NAS_SRC}ESM/MSG/esm_msg.c
  ${NAS_SRC}ESM/MSG/EsmStatus.c
  ${NAS_SRC}ESM/MSG/ModifyEpsBearerContextAccept.c
  ${NAS_SRC}ESM/MSG/ModifyEpsBearerContextReject.c
  ${NAS_SRC}ESM/MSG/ModifyEpsBearerContextRequest.c
  ${NAS_SRC}ESM/MSG/PdnConnectivityReject.c
  ${NAS_SRC}ESM/MSG/PdnConnectivityRequest.c
  ${NAS_SRC}ESM/MSG/PdnDisconnectReject.c
  ${NAS_SRC}ESM/MSG/PdnDisconnectRequest.c
)

set(libnas_ies_OBJS
  ${NAS_SRC}IES/AccessPointName.c
  ${NAS_SRC}IES/AdditionalUpdateResult.c
  ${NAS_SRC}IES/AdditionalUpdateType.c
  ${NAS_SRC}IES/ApnAggregateMaximumBitRate.c
  ${NAS_SRC}IES/AuthenticationFailureParameter.c
  ${NAS_SRC}IES/AuthenticationParameterAutn.c
  ${NAS_SRC}IES/AuthenticationParameterRand.c
  ${NAS_SRC}IES/AuthenticationResponseParameter.c
  ${NAS_SRC}IES/CipheringKeySequenceNumber.c
  ${NAS_SRC}IES/Cli.c
  ${NAS_SRC}IES/CsfbResponse.c
  ${NAS_SRC}IES/DaylightSavingTime.c
  ${NAS_SRC}IES/DetachType.c
  ${NAS_SRC}IES/DrxParameter.c
  ${NAS_SRC}IES/EmergencyNumberList.c
  ${NAS_SRC}IES/EmmCause.c
  ${NAS_SRC}IES/EpsAttachResult.c
  ${NAS_SRC}IES/EpsAttachType.c
  ${NAS_SRC}IES/EpsBearerContextStatus.c
  ${NAS_SRC}IES/EpsBearerIdentity.c
  ${NAS_SRC}IES/EpsMobileIdentity.c
  ${NAS_SRC}IES/EpsNetworkFeatureSupport.c
  ${NAS_SRC}IES/EpsQualityOfService.c
  ${NAS_SRC}IES/EpsUpdateResult.c
  ${NAS_SRC}IES/EpsUpdateType.c
  ${NAS_SRC}IES/EsmCause.c
  ${NAS_SRC}IES/EsmInformationTransferFlag.c
  ${NAS_SRC}IES/EsmMessageContainer.c
  ${NAS_SRC}IES/GprsTimer.c
  ${NAS_SRC}IES/GutiType.c
  ${NAS_SRC}IES/IdentityType2.c
  ${NAS_SRC}IES/ImeisvRequest.c
  ${NAS_SRC}IES/KsiAndSequenceNumber.c
  ${NAS_SRC}IES/LcsClientIdentity.c
  ${NAS_SRC}IES/LcsIndicator.c
  ${NAS_SRC}IES/LinkedEpsBearerIdentity.c
  ${NAS_SRC}IES/LlcServiceAccessPointIdentifier.c
  ${NAS_SRC}IES/LocationAreaIdentification.c
  ${NAS_SRC}IES/MessageType.c
  ${NAS_SRC}IES/MobileIdentity.c
  ${NAS_SRC}IES/MobileStationClassmark2.c
  ${NAS_SRC}IES/MobileStationClassmark3.c
  ${NAS_SRC}IES/MsNetworkCapability.c
  ${NAS_SRC}IES/MsNetworkFeatureSupport.c
  ${NAS_SRC}IES/NasKeySetIdentifier.c
  ${NAS_SRC}IES/NasMessageContainer.c
  ${NAS_SRC}IES/NasRequestType.c
  ${NAS_SRC}IES/NasSecurityAlgorithms.c
  ${NAS_SRC}IES/NetworkName.c
  ${NAS_SRC}IES/Nonce.c
  ${NAS_SRC}IES/PacketFlowIdentifier.c
  ${NAS_SRC}IES/PagingIdentity.c
  ${NAS_SRC}IES/PdnAddress.c
  ${NAS_SRC}IES/PdnType.c
  ${NAS_SRC}IES/PlmnList.c
  ${NAS_SRC}IES/ProcedureTransactionIdentity.c
  ${NAS_SRC}IES/ProtocolConfigurationOptions.c
  ${NAS_SRC}IES/ProtocolDiscriminator.c
  ${NAS_SRC}IES/PTmsiSignature.c
  ${NAS_SRC}IES/QualityOfService.c
  ${NAS_SRC}IES/RadioPriority.c
  ${NAS_SRC}IES/SecurityHeaderType.c
  ${NAS_SRC}IES/ServiceType.c
  ${NAS_SRC}IES/ShortMac.c
  ${NAS_SRC}IES/SsCode.c
  ${NAS_SRC}IES/SupportedCodecList.c
  ${NAS_SRC}IES/TimeZoneAndTime.c
  ${NAS_SRC}IES/TimeZone.c
  ${NAS_SRC}IES/TmsiStatus.c
  ${NAS_SRC}IES/TrackingAreaIdentity.c
  ${NAS_SRC}IES/TrackingAreaIdentityList.c
  ${NAS_SRC}IES/TrafficFlowAggregateDescription.c
  ${NAS_SRC}IES/TrafficFlowTemplate.c
  ${NAS_SRC}IES/TransactionIdentifier.c
  ${NAS_SRC}IES/UeNetworkCapability.c
  ${NAS_SRC}IES/UeRadioCapabilityInformationUpdateNeeded.c
  ${NAS_SRC}IES/UeSecurityCapability.c
  ${NAS_SRC}IES/VoiceDomainPreferenceAndUeUsageSetting.c
)

set (libnas_utils_OBJS
  ${NAS_SRC}UTIL/nas_timer.c
  ${NAS_SRC}UTIL/socket.c
  ${NAS_SRC}UTIL/TLVEncoder.c
  ${NAS_SRC}UTIL/TLVDecoder.c
  ${NAS_SRC}UTIL/OctetString.c
)


set(libnas_mme_api_OBJS
  ${NAS_SRC}API/MME/mme_api.c
)

set(libnas_mme_emm_OBJS
  ${NAS_SRC}EMM/Attach.c
  ${NAS_SRC}EMM/Authentication.c
  ${NAS_SRC}EMM/Detach.c
  ${NAS_SRC}EMM/EmmCommon.c
  ${NAS_SRC}EMM/emm_data_ctx.c
  ${NAS_SRC}EMM/emm_main.c
  ${NAS_SRC}EMM/EmmStatusHdl.c
  ${NAS_SRC}EMM/Identification.c
  ${NAS_SRC}EMM/LowerLayer.c
  ${NAS_SRC}EMM/SecurityModeControl.c
  ${NAS_SRC}EMM/ServiceRequestHdl.c
  ${NAS_SRC}EMM/TrackingAreaUpdate.c
)
  
set(libnas_mme_emm_sap_OBJS
  ${NAS_SRC}EMM/SAP/emm_as.c
  ${NAS_SRC}EMM/SAP/emm_cn.c
  ${NAS_SRC}EMM/SAP/EmmCommonProcedureInitiated.c
  ${NAS_SRC}EMM/SAP/EmmDeregistered.c
  ${NAS_SRC}EMM/SAP/EmmDeregisteredInitiated.c
  ${NAS_SRC}EMM/SAP/emm_esm.c
  ${NAS_SRC}EMM/SAP/emm_fsm.c
  ${NAS_SRC}EMM/SAP/EmmNull.c
  ${NAS_SRC}EMM/SAP/emm_recv.c
  ${NAS_SRC}EMM/SAP/emm_reg.c
  ${NAS_SRC}EMM/SAP/EmmRegistered.c
  ${NAS_SRC}EMM/SAP/emm_sap.c
  ${NAS_SRC}EMM/SAP/emm_send.c
  )

set (libnas_mme_esm_OBJS
  ${NAS_SRC}ESM/DedicatedEpsBearerContextActivation.c
  ${NAS_SRC}ESM/DefaultEpsBearerContextActivation.c
  ${NAS_SRC}ESM/EpsBearerContextDeactivation.c
  ${NAS_SRC}ESM/esm_data_context.c
  ${NAS_SRC}ESM/esm_ebr.c
  ${NAS_SRC}ESM/esm_ebr_context.c
  ${NAS_SRC}ESM/esm_ip.c
  ${NAS_SRC}ESM/esm_main.c
  ${NAS_SRC}ESM/esm_pt.c
  ${NAS_SRC}ESM/EsmStatusHdl.c
  ${NAS_SRC}ESM/PdnConnectivity.c
  ${NAS_SRC}ESM/PdnDisconnect.c
  )

set(libnas_mme_esm_sap_OBJS
  ${NAS_SRC}ESM/SAP/esm_recv.c
  ${NAS_SRC}ESM/SAP/esm_send.c
  ${NAS_SRC}ESM/SAP/esm_sap.c
  )
  
add_library(LIB_NAS_MME
  ${NAS_SRC}nas_itti_messaging.c
  ${NAS_SRC}nas_if_s1.c
  ${NAS_SRC}nas_if_s6a.c
  ${NAS_SRC}nas_network.c
  ${NAS_SRC}nas_proc.c
  ${libnas_api_OBJS}
  ${libnas_mme_api_OBJS}
  ${libnas_emm_msg_OBJS}
  ${libnas_esm_msg_OBJS}
  ${libnas_ies_OBJS}
  ${libnas_utils_OBJS}
  ${libnas_mme_emm_OBJS}
  ${libnas_mme_emm_sap_OBJS}
  ${libnas_mme_esm_OBJS}
  ${libnas_mme_esm_sap_OBJS} 
  )
  
include_directories(${NAS_SRC})
include_directories(${NAS_SRC}/API/MME)
include_directories(${NAS_SRC}/EMM)
include_directories(${NAS_SRC}/EMM/SAP)
include_directories(${NAS_SRC}/ESM)
include_directories(${NAS_SRC}/ESM/SAP)


# Make lfds as a own source code (even if it is a outside library)
# For better intergration with compilation flags & structure of cmake
###################################################################
set(lfds ${OPENAIRCN_DIR}/SRC/UTILS/LFDS/liblfds6.1.1/liblfds611/src/)
file(GLOB lfds_queue ${lfds}/lfds611_queue/*.c)
file(GLOB lfds_ring ${lfds}/lfds611_ringbuffer/*.c)
file(GLOB lfds_slist ${lfds}/lfds611_slist/*.c)
file(GLOB lfds_stack ${lfds}/lfds611_stack/*.c)
file(GLOB lfds_freelist ${lfds}/lfds611_freelist/*.c)

include_directories(${lfds})
add_library(LFDS
  ${lfds_queue} ${lfds_ring} ${lfds_slist} ${lfds_stack} ${lfds_freelist}
  ${lfds}/lfds611_liblfds/lfds611_liblfds_abstraction_test_helpers.c
  ${lfds}/lfds611_liblfds/lfds611_liblfds_aligned_free.c
  ${lfds}/lfds611_liblfds/lfds611_liblfds_aligned_malloc.c
  ${lfds}/lfds611_abstraction/lfds611_abstraction_free.c
  ${lfds}/lfds611_abstraction/lfds611_abstraction_malloc.c
)

# Make lfds as a own source code (even if it is a outside library)
# For better intergration with compilation flags & structure of cmake
###################################################################
set(lfds7 ${OPENAIRCN_DIR}/SRC/UTILS/LFDS/liblfds7.0.0/liblfds700/src/)
file(GLOB lfds7_queue ${lfds7}/lfds700_queue/*.c)
file(GLOB lfds7_misc ${lfds7}/lfds700_misc/*.c)
file(GLOB lfds7_ring ${lfds7}/lfds700_ringbuffer/*.c)
file(GLOB lfds7_stack ${lfds7}/lfds700_stack/*.c)
file(GLOB lfds7_freelist ${lfds7}/lfds700_freelist/*.c)

include_directories(${lfds7})
add_library(LFDS7
  ${lfds7_queue} ${lfds7_misc} ${lfds7_ring} ${lfds7_stack} ${lfds7_freelist}
)

# System packages that are required
# We use either the cmake buildin, in ubuntu are in: /usr/share/cmake*/Modules/
# or cmake provide a generic interface to pkg-config that widely used
###################################
set(CMAKE_MODULE_PATH "${OPENAIRCN_DIR}/BUILD/MODULES" "${CMAKE_MODULE_PATH}")

include(FindPkgConfig)

pkg_search_module(LIBXML2 libxml-2.0 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIRS})

pkg_search_module(OPENSSL openssl REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIRS})

pkg_search_module(CONFIG libconfig REQUIRED)
include_directories(${CONFIG_INCLUDE_DIRS})

pkg_search_module(CRYPTO libcrypto REQUIRED)
include_directories(${CRYPTO_INCLUDE_DIRS})


pkg_search_module(OPENPGM openpgm-5.1)
if(NOT ${OPENPGM_FOUND})
  message("PACKAGE openpgm-5.1 is required by binaries such as oaisim: will fail later if this target is built")
else()
  include_directories(${OPENPGM_INCLUDE_DIRS})
endif()

pkg_search_module(NETTLE nettle)
if(NOT ${NETTLE_FOUND})
  message("PACKAGE nettle not found: some targets will fail")
else()
  include_directories(${NETTLE_INCLUDE_DIRS})
endif()

pkg_search_module(XPM xpm)
if(NOT ${XPM_FOUND})
  message("PACKAGE xpm not found: some targets will fail")
else()
  include_directories(${XPM_INCLUDE_DIRS})
endif()




# Hack on a test of asn1c version (already dirty)
add_definitions(-DASN1_MINIMUM_VERSION=924)

#################################
# add executables for operation
#################################""



# MME + S/P-GW implementation in one process
################################
add_executable(mme_gw
  ${OPENAIRCN_DIR}/SRC/OAI_EPC/oai_epc_log.c
  ${OPENAIRCN_DIR}/SRC/OAI_EPC/oai_epc.c
  ${GTPV1U_DIR}/gtpv1u_task.c
  ${OPENAIRCN_DIR}/SRC/SCTP/sctp_primitives_server.c
  ${OPENAIRCN_DIR}/SRC/NAS/nas_mme_task.c
  )
if( ITTI_ANALYZER )
add_executable(mme_gw
  ${OPENAIRCN_BIN_DIR}/messages_xml.h )
endif( ITTI_ANALYZER )
target_link_libraries (mme_gw
#SECU_OSA
  -Wl,--start-group
   LIB_NAS_MME S1AP_LIB S1AP_EPC GTPV1U GTPV2C SCTP_SERVER UDP_SERVER SECU_CN  S6A SGW MME_APP LFDS LFDS7 ${MSC_LIB} ${ITTI_LIB} CN_UTILS HASHTABLE
  -Wl,--end-group
  pthread m sctp  rt crypt ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} ${NETTLE_LIBRARIES} ${CONFIG_LIBRARIES} gnutls fdproto fdcore
  )
  
# MME standalone in one process
################################
add_executable(mme
  ${OPENAIRCN_DIR}/SRC/OAI_MME/oai_mme_log.c
  ${OPENAIRCN_DIR}/SRC/OAI_MME/oai_mme.c
  ${OPENAIRCN_DIR}/SRC/SCTP/sctp_primitives_server.c
  ${OPENAIRCN_DIR}/SRC/NAS/nas_mme_task.c
  )
if( ITTI_ANALYZER )
add_executable(mme
  ${OPENAIRCN_BIN_DIR}/messages_xml.h )
endif( ITTI_ANALYZER )
target_link_libraries (mme
#SECU_OSA
  -Wl,--start-group
   LIB_NAS_MME S1AP_LIB S1AP_EPC S11 GTPV2C SCTP_SERVER UDP_SERVER SECU_CN  S6A MME_APP LFDS LFDS7 ${MSC_LIB} ${ITTI_LIB} CN_UTILS HASHTABLE
  -Wl,--end-group
  pthread m sctp  rt crypt ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} ${NETTLE_LIBRARIES} ${CONFIG_LIBRARIES} gnutls fdproto fdcore
  )


# oai_sgw is SGW  node implementation
################################
add_executable(spgw
  ${OPENAIRCN_DIR}/SRC/OAI_SGW/oai_sgw.c
  ${GTPV1U_DIR}/gtpv1u_task.c
  )
if( ITTI_ANALYZER )
add_executable(spgw
  ${OPENAIRCN_BIN_DIR}/messages_xml.h )
endif( ITTI_ANALYZER )
target_link_libraries (spgw
  -Wl,--start-group
  GTPV1U SGW S11 GTPV1U GTPV2C UDP_SERVER LFDS LFDS7 ${MSC_LIB} ${ITTI_LIB} CN_UTILS HASHTABLE
  -Wl,--end-group
  pthread m rt ${CONFIG_LIBRARIES}  
  )


# Default parameters
# Does not work on simple install (fqdn in /etc/hosts 127.0.1.1)
#cmake_host_system_information(RESULT HSS_FQDN_STR QUERY FQDN)
#cmake_host_system_information(RESULT MME_FQDN_STR QUERY FQDN)
IF( EPC_BUILD OR MME_BUILD )
  INCLUDE(FindFreeDiameter)
  # if standalone eNB or UE no need for FreeDiameter
  IF( FREEDIAMETER_FOUND )
    IF( NOT FREEDIAMETER_HSS_S6A_ENABLED )
      MESSAGE( SEND_ERROR "FreeDiameter is not enabled for OPENAIRHSS" )
    ENDIF( NOT FREEDIAMETER_HSS_S6A_ENABLED )
  ENDIF( FREEDIAMETER_FOUND )


  execute_process(COMMAND hostname --fqdn OUTPUT_VARIABLE HSS_FQDN_STR TIMEOUT 3)
  execute_process(COMMAND hostname --fqdn OUTPUT_VARIABLE MME_FQDN_STR TIMEOUT 3)
  string(STRIP ${HSS_FQDN_STR} HSS_FQDN_STR)
  string(STRIP ${MME_FQDN_STR} MME_FQDN_STR)

  set(HSS_IP                       127.0.1.1                 CACHE STRING     "HSS IP address, default is local")
  set(MME_FQDN                     ${MME_FQDN_STR}           CACHE STRING     "MME Fully qualified domain name")
  set(HSS_FQDN                     ${HSS_FQDN_STR}           CACHE STRING     "HSS Fully qualified domain name")
  set(TRANSPORT_option             "#No_TCP"                 CACHE STRING     "No_TCP or No_SCTP or comment string, FreeDiameter config option")
  set(TRANSPORT_PREFER_TCP_option  "#Prefer_TCP"             CACHE STRING     "Prefer_TCP or comment string, FreeDiameter config option")
  IF( NOT DEFINED REALM )
    string(FIND ${MME_FQDN} "." pos)
    math(EXPR pos1 "${pos}+1")
    string(SUBSTRING ${MME_FQDN} ${pos1} -1 realm_string)
    message("REALM extracted from MME FQDN is ${realm_string}")
    set(REALM                      ${realm_string}           CACHE STRING     "EPC realm")
  ENDIF( NOT DEFINED REALM )
  message("REALM is ${REALM}")
  IF( NOT DEFINED MME_HOSTNAME )
    string(FIND ${MME_FQDN} "." pos)
    string(SUBSTRING ${MME_FQDN} 0 ${pos} mme_hostname_string)
    message("MME_HOSTNAME extracted from MME FQDN is ${mme_hostname_string}")
    set(MME_HOSTNAME         ${mme_hostname_string}           CACHE STRING     "MME hostname")
  ENDIF( NOT DEFINED MME_HOSTNAME )
  IF( NOT DEFINED HSS_HOSTNAME )
    string(FIND ${HSS_FQDN} "." pos)
    string(SUBSTRING ${HSS_FQDN} 0 ${pos} hss_hostname_string)
    message("HSS_HOSTNAME extracted from HSS FQDN is ${hss_hostname_string}")
    set(HSS_HOSTNAME         ${hss_hostname_string}           CACHE STRING     "HSS hostname")
  ENDIF( NOT DEFINED HSS_HOSTNAME )

  if ( ${MME_HOSTNAME} STREQUAL ${HSS_HOSTNAME} )
    set(MME_S6A_IS_SERVER      1 )
  endif( ${MME_HOSTNAME} STREQUAL ${HSS_HOSTNAME} )

  IF( MME_S6A_IS_SERVER )
    set(ConnectPeer        "#ConnectPeer"     CACHE STRING   "HSS connect to MME")
    set(DIAMETER_PORT      "Port = 3870"      CACHE STRING   "Diameter bind port, 3868 is always used by default by HSS, even if HSS client")
    set(DIAMETER_SEC_PORT  "SecPort = 3871"   CACHE STRING   "Diameter bind sec port")
  ELSE( MME_S6A_IS_SERVER )
    set(ConnectPeer        "#ConnectPeer"     CACHE STRING   "HSS act as a server")
    set(DIAMETER_PORT      "#Port = 3870"     CACHE STRING   "Diameter bind port")
    set(DIAMETER_SEC_PORT  "#SecPort = 3871"  CACHE STRING   "Diameter bind sec port")
  ENDIF( MME_S6A_IS_SERVER )

  execute_process(COMMAND sudo ${OPENAIRCN_DIR}/SCRIPTS/check_mme_s6a_certificate ${FREEDIAMETER_PATH}/.. ${MME_FQDN} WORKING_DIRECTORY ${BUILD_TOP_DIR})


ENDIF( EPC_BUILD OR MME_BUILD )

IF( EPC_BUILD )
  configure_file(
    ${BUILD_TOP_DIR}/EPC/epc.conf.in
    ${CMAKE_CURRENT_BINARY_DIR}/epc.conf)

  configure_file(
    ${BUILD_TOP_DIR}/EPC/s6a.conf.in
    ${CMAKE_CURRENT_BINARY_DIR}/mme_fd.conf)
ENDIF( EPC_BUILD )

IF( MME_BUILD )
  configure_file(
    ${BUILD_TOP_DIR}/MME/mme.conf.in
    ${CMAKE_CURRENT_BINARY_DIR}/mme.conf)
    
  configure_file(
    ${BUILD_TOP_DIR}/MME/s6a.conf.in
    ${CMAKE_CURRENT_BINARY_DIR}/mme_fd.conf)
ENDIF( MME_BUILD )

IF( SPGW_BUILD )
  configure_file(
    ${BUILD_TOP_DIR}/SPGW/spgw.conf.in
    ${CMAKE_CURRENT_BINARY_DIR}/spgw.conf)
ENDIF( SPGW_BUILD )



###################################"
# Add executables for tests
####################################



# TODO
#unitary tests for Core NEtwork pieces
#################################
#foreach(myExe s1ap
#    secu_knas_encrypt_eia1
#    secu_kenb
#    aes128_ctr_encrypt
#    aes128_ctr_decrypt
#    secu_knas_encrypt_eea2
#    secu_knas secu_knas_encrypt_eea1
#    kdf
#    aes128_cmac_encrypt
#    secu_knas_encrypt_eia2)
#    
#  add_executable(test_${myExe}
#    ${OPENAIRCN_DIR}/SRC/TEST/test_util.c
#    ${OPENAIRCN_DIR}/SRC/TEST/test_${myExe}.c
#    )
#  target_link_libraries (test_${myExe}
#    -Wl,--start-group SECU_CN UTIL LFDS LFDS7 -Wl,--end-group m rt crypt ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} ${NETTLE_LIBRARIES} ${CONFIG_LIBRARIES} fdproto fdcore
#    )
#endforeach(myExe)


##################################################
# Generated specific cases is not regular code
###############################################

##################""
# itti symbolic debug print require to generate a specific include file
########################################

# retrieve the compiler options to send it to gccxml
get_directory_property( DirDefs COMPILE_DEFINITIONS )
foreach( d ${DirDefs} )
    list(APPEND itti_compiler_options "-D${d}")
endforeach()
get_directory_property( DirDefs INCLUDE_DIRECTORIES )
foreach( d ${DirDefs} )
    list(APPEND itti_compiler_options "-I${d}")
endforeach()

set (ITTI_H ${ITTI_DIR}/intertask_interface_types.h)
if( ITTI_ANALYZER )
add_custom_command (
  OUTPUT ${OPENAIRCN_BIN_DIR}/messages.xml
  COMMAND gccxml ${itti_compiler_options} -fxml=${OPENAIRCN_BIN_DIR}/messages.xml ${ITTI_H}
  DEPENDS ${S1AP_OAI_generated} 
  )

add_custom_command (
  OUTPUT ${OPENAIRCN_BIN_DIR}/messages_xml.h
  COMMAND sed -e 's/ *//'   -e 's/\"/\\\\\"/g' -e 's/^/\"/' -e 's/$$/\\\\n\"/' ${OPENAIRCN_BIN_DIR}/messages.xml  > ${OPENAIRCN_BIN_DIR}/messages_xml.h
  DEPENDS ${OPENAIRCN_BIN_DIR}/messages.xml 
  )
endif(ITTI_ANALYZER)




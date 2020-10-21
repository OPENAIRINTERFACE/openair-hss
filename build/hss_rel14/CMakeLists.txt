################################################################################
# Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The OpenAirInterface Software Alliance licenses this file to You under
# the terms found in the LICENSE file in the root of this source tree.
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
cmake_minimum_required(VERSION 2.8)
set(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_CXX_FLAGS "-std=c++11")

project(HSS_UTIL)


ENABLE_LANGUAGE(C)
#Sends the -std=c99 flag to the gcc compiler
#add_definitions(-std=c99)

ADD_DEFINITIONS(-D_GNU_SOURCE)

if( NOT DEFINED OPENAIRCN_DIR)
  message(SEND_ERROR "OPENAIRCN_DIR not set: ${OPENAIRCN_DIR}")
endif( NOT DEFINED OPENAIRCN_DIR)



set(OPENAIR_CMAKE_SCRIPTS ${OPENAIRCN_DIR}/build/tools)
set(OAI_HSS_DIR           ${OPENAIRCN_DIR}/src/hss_rel14)
set(OAI_HSS_UTIL_DIR           ${OPENAIRCN_DIR}/src/hss_rel14/util)
set(OPENAIR_BIN_DIR       ${OPENAIRCN_DIR}/src/hss_rel14/)


set(CMAKE_MODULE_PATH "${OPENAIRCN_DIR}/build/cmake_modules" "${CMAKE_MODULE_PATH}")

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

macro(add_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  add_definitions("-D${name}=${value}")
endmacro(add_option)


# compilation flags
# added CMAKER conditional compilation to not bother legacy building system
#############################################
if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)

set(SECURITY_FLAGS                  "-D_FORTIFY_SOURCE=2 -fasynchronous-unwind-tables -fexceptions"
set(SECURITY_FLAGS                  "${SECURITY_FLAGS} -fstack-protector-all -fstack-protector-strong -Wall"
set(SECURITY_FLAGS                  "${SECURITY_FLAGS} -Werror=format-security -Werror=implicit-function-declaration"

SET(CMAKE_CXX_FLAGS "-std=c++11 -Wreturn-type -g -pthread -lrt ${SECURITY_FLAGS}")

#
set(CMAKE_C_FLAGS                  "${CMAKE_C_FLAGS} std=c++11 -Wreturn-type -g -pthread -lrt $(SECURITY_FLAGS) -DCMAKER")
set(CMAKE_C_FLAGS_DEBUG            "${CMAKE_C_FLAGS_DEBUG} -std=c++11 -Wreturn-type -g -pthread -lrt $(SECURITY_FLAGS)-ggdb -DMALLOC_CHECK_=3")
set(CMAKE_C_FLAGS_RELWITHDEBINFO   "${CMAKE_C_FLAGS_RELWITHDEBINFO} std=c++11 -Wreturn-type -g -pthread -lrt $(SECURITY_FLAGS) -ggdb -DMALLOC_CHECK_=3 -O2")


set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

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


# Below is a hard-coded info
add_definitions("-DPACKAGE_VERSION=\"Branch: ${GIT_BRANCH} Abrev. Hash: ${GIT_COMMIT_HASH} Date: ${GIT_COMMIT_DATE}\"")
add_definitions("-DPACKAGE_BUGREPORT=\"openaircn-user@lists.eurecom.fr\"")

##TTN
################################################################################

################################################################################
# EXECUTABLE oai_hss
################################################################################
link_directories(${OAI_HSS_DIR}/util)
link_directories(${OAI_HSS_DIR}/hsssec)

include_directories( ${OAI_HSS_DIR}/../git_submodules/rapidjson/include ${OAI_HSS_DIR}/util/include ${OAI_HSS_DIR}/include /usr/local/include/freeDiameter ${OAI_HSS_DIR}/hsssec/include /usr/local/include ${OAI_HSS_DIR}/../git_submodules/cpp-driver/include ${OAI_HSS_DIR}/../git_submodules/spdlog/include)
ADD_EXECUTABLE(oai_hss  ${OAI_HSS_DIR}/src/main.cpp)
target_link_libraries (oai_hss
                       -Wl,-whole-archive
                       hss_modules
                       -Wl,-no-whole-archive
                       gmp
                       /usr/local/lib/
                       /usr/local/lib/x86_64-linux-gnu
                       ${MySQL_LIBRARY}
                       ${NETTLE_LIBRARIES}
                       ${FREEDIAMETER_LIBRARIES}
                       ${CONFIG_LIBRARIES}
                       ${CMAKE_THREAD_LIBS_INIT}
                       gnutls)

# Default parameters
# Does not work on simple install (fqdn in /etc/hosts 127.0.1.1)
add_boolean_option(DAEMONIZE         false          "If true, HSS execute like a daemon (fork).")


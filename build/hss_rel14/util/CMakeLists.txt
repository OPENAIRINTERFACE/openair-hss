cmake_minimum_required(VERSION 2.4.0)

project(c3po)


SET(CMAKE_CXX_FLAGS "-std=c++0x")

if( NOT DEFINED OPENAIRCN_DIR)
  message(SEND_ERROR "OPENAIRCN_DIR not set: ${OPENAIRCN_DIR}")
endif( NOT DEFINED OPENAIRCN_DIR)

set(OAI_HSS_DIR           ${OPENAIRCN_DIR}/src/hss_rel14)
set(OAI_MODULES_DIR           ${OPENAIRCN_DIR}/build/hss_rel14)

# Find source files
file(GLOB SOURCES $OAI_HSS_DIR/util/*.cpp)

# Include header files
include_directories(OAI_HSS_DIR/util/include)
include_directories($OAI_MODULES_DIR/modules/rapidjson/include/)
include_directories($OAI_MODULES_DIR/modules/freeDiameter/include/)
include_directories($OAI_MODULES_DIR/modules/c-ares)
# Create shared library
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Install library
install(TARGETS ${PROJECT_NAME} DESTINATION lib/${PROJECT_NAME})

# Install library headers
file(GLOB HEADERS $OAI_HSS_DIR/util/include/*.h)
install(FILES ${HEADERS} DESTINATION $OAI_HSS_DIR/util/include/${PROJECT_NAME})
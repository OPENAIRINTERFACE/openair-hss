cmake_minimum_required(VERSION 2.4.0)

project(c3po)


SET(CMAKE_CXX_FLAGS "-std=c++0x")


# Find source files
file(GLOB SOURCES src/*.cpp)

# Include header files
include_directories(include)
include_directories(../modules/rapidjson/include/)
include_directories(../modules/freeDiameter/include/)
include_directories(../modules/c-ares)
# Create shared library
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Install library
install(TARGETS ${PROJECT_NAME} DESTINATION lib/${PROJECT_NAME})

# Install library headers
file(GLOB HEADERS include/*.h)
install(FILES ${HEADERS} DESTINATION include/${PROJECT_NAME})

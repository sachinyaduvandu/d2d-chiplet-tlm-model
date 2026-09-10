cmake_minimum_required(VERSION 3.16)
project(d2d_chiplet_tlm_model CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall")

# Allow environment variable or CLI override, fallback to standard locations
if(DEFINED SYSTEMC_PREFIX)
    set(SYSTEMC_DIR ${SYSTEMC_PREFIX})
elseif(DEFINED ENV{SYSTEMC_HOME})
    set(SYSTEMC_DIR $ENV{SYSTEMC_HOME})
elseif(EXISTS "/usr/local/systemc-3.0.2")
    set(SYSTEMC_DIR "/usr/local/systemc-3.0.2")
elseif(EXISTS "/opt/systemc")
    set(SYSTEMC_DIR "/opt/systemc")
else()
    set(SYSTEMC_DIR "/usr/local")
endif()

message(STATUS "Using SystemC from: ${SYSTEMC_DIR}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SYSTEMC_DIR}/include
)

link_directories(
    ${SYSTEMC_DIR}/lib-linux64
    ${SYSTEMC_DIR}/lib
)

add_executable(d2d_sim src/sc_main.cpp)
target_link_libraries(d2d_sim systemc pthread)

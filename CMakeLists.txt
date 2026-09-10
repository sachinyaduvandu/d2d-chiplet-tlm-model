cmake_minimum_required(VERSION 3.10)
project(d2d_tlm_model CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DSC_ALLOW_DEPRECATED_IEEE_API)

set(SYSTEMC_PREFIX "/opt/systemc")
include_directories(${SYSTEMC_PREFIX}/include include)
link_directories(${SYSTEMC_PREFIX}/lib)

add_executable(d2d_sim
    src/sc_main.cpp
)

target_link_libraries(d2d_sim systemc)
set_target_properties(d2d_sim PROPERTIES INSTALL_RPATH "${SYSTEMC_PREFIX}/lib")

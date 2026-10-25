# saintsrow - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(saintsrow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(SAINTSROW_SOURCES
    src/main.cpp
)

if(WIN32)
    add_executable(saintsrow WIN32 ${SAINTSROW_SOURCES})
else()
    add_executable(saintsrow ${SAINTSROW_SOURCES})
endif()

rexglue_setup_target(saintsrow GPU_PLUGINS xenos)

# the ports' common part: linker map, -march=native on the player's PC, the FidelityFX runtime (port.cmake)
include(port.cmake)
port_setup(saintsrow)

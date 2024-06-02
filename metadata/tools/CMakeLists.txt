# CMake build settings for all of the metadata tools

# (c) 2013-2021 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
# All rights reserved--use subject to compliance with end user license agreement.

cmake_minimum_required(VERSION 3.19)
project(tools)

set(ROOT ${CMAKE_SOURCE_DIR}/../..)
set(EXTERNAL ${ROOT}/external)
set(METADATA ${ROOT}/metadata)

include_directories(${METADATA}/common/include ${EXTERNAL}/argparse)
link_directories(${EXTERNAL}/argparse/build)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

file(GLOB bintool_sources ${METADATA}/tools/bintool/*.c)
add_executable(bintool ${bintool_sources})
target_link_libraries(bintool argparse)

file(GLOB dpxdump_sources ${METADATA}/tools/dpxdump/*.c)
add_executable(dpxdump ${dpxdump_sources})
target_link_libraries(dpxdump argparse)

file(GLOB exrdump_sources ${METADATA}/tools/exrdump/*.c)
add_executable(exrdump ${exrdump_sources})
target_link_libraries(exrdump argparse)

file(GLOB klvdump_sources ${METADATA}/tools/klvdump/*.c)
add_executable(klvdump ${klvdump_sources})
target_link_libraries(klvdump argparse)

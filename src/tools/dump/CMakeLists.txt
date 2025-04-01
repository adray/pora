# CMakeLists to build dump

cmake_minimum_required (VERSION 3.8)

project("podump")

include_directories("../../core")

set (PODUMP_SOURCES
    "poDump.h"
    "poDump.cpp"
)


# Add source to this project's executable.
add_executable(dump ${PODUMP_SOURCES})
target_link_libraries(dump poracore)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET dump PROPERTY CXX_STANDARD 20)
endif()


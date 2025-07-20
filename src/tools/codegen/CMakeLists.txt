# CMakeLists to build codegen

cmake_minimum_required (VERSION 3.8)

project("pocodegen")

include_directories("../../core")
include_directories("../../backend")

set (PODUMP_SOURCES
    "poCodeGen.h"
    "poCodeGen.cpp"
)


# Add source to this project's executable.
add_executable(codegen ${PODUMP_SOURCES})
target_link_libraries(codegen poracore)
target_link_libraries(codegen porabackend)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET codegen PROPERTY CXX_STANDARD 20)
endif()


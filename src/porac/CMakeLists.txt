# Porac

cmake_minimum_required (VERSION 3.8)

set (PORAC_SOURCES
    "poApp.h"
    "poApp.cpp"
    "poAsm.h"
    "poAsm.cpp"
    "poCompiler.h"
    "poCompiler.cpp"
    "poEmit.h"
    "poEmit.cpp"

    "../Examples/Example.po"
)

project ("porac")

include_directories("../core")

# Add source to this project's executable.
add_executable(porac ${PORAC_SOURCES})
target_link_libraries(porac poracore)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET porac PROPERTY CXX_STANDARD 20)
endif()



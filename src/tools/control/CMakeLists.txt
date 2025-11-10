# CMakeLists to build dump

cmake_minimum_required (VERSION 3.8)

project("control")

include_directories("../../core")

set (POCONTROL_SOURCES
    "poControl.h"
    "poControl.cpp"
)


# Add source to this project's executable.
add_executable(control ${POCONTROL_SOURCES})
target_link_libraries(control poracore)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET control PROPERTY CXX_STANDARD 20)
endif()


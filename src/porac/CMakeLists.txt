# Porac

cmake_minimum_required (VERSION 3.8)

set (PORAC_SOURCES
    "poApp.h"
    "poApp.cpp"
    "poCompiler.h"
    "poCompiler.cpp"
    "poEmit.h"
    "poEmit.cpp"
)

set (PORAC_EXAMPLES
    "../examples/Example.po"
    "../examples/Example2.po"
    "../examples/Example3.po"
    "../examples/Example4.po"
    "../examples/Example5.po"
    "../examples/Example6.po"
    "../examples/Example7.po"
    "../examples/Example8.po"
)

project ("porac")

include_directories("../core")
include_directories("../backend")

# Add source to this project's executable.
add_executable(porac ${PORAC_SOURCES} ${PORAC_EXAMPLES})
target_link_libraries(porac poracore)
target_link_libraries(porac porabackend)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET porac PROPERTY CXX_STANDARD 20)
endif()



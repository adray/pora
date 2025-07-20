# Pora Backend

cmake_minimum_required (VERSION 3.8)

set (PORABACKEND_SOURCES
    "poAsm.h"
    "poAsm.cpp"
    "po_x86_64.h"
    "po_x86_64.cpp"
)

project ("porabackend")

include_directories("../core")

# Add source to this project's executable.
add_library(porabackend STATIC ${PORABACKEND_SOURCES})

target_link_libraries(porabackend poracore)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET porabackend PROPERTY CXX_STANDARD 20)
endif()




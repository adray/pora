# Pora Tests

cmake_minimum_required (VERSION 3.8)

set (PORA_TEST_SOURCES
    "poTestApp.h"
    "poTestApp.cpp"
    "poSyntaxTests.h"
    "poSyntaxTests.cpp"
    "poDominatorTest.h"
    "poDominatorTest.cpp"
    "poSSATests.h"
    "poSSATests.cpp"
    "poLiveTests.h"
    "poLiveTests.cpp"
    "poRegLinearTests.h"
    "poRegLinearTests.cpp"
    "poNLFTests.h"
    "poNLFTests.cpp"
)

project ("poratest")

include_directories("../core")

# Add source to this project's executable.
add_executable(poratest ${PORA_TEST_SOURCES})
target_link_libraries(poratest poracore)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET poratest PROPERTY CXX_STANDARD 20)
endif()


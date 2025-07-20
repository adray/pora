# CMakeLists to build Pora

cmake_minimum_required (VERSION 3.8)

project("pora")

add_subdirectory("porac")
add_subdirectory("backend")
add_subdirectory("core")
add_subdirectory("tools")
add_subdirectory("test")

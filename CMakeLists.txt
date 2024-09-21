cmake_minimum_required(VERSION 3.15)

project(jrobots
  LANGUAGES CXX
  VERSION 1.0
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
  Release
  Debug
)

add_subdirectory(thirdparty)
add_subdirectory(src)

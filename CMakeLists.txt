cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(BWEB)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BWEM_DIR ${PROJECT_SOURCE_DIR}/external/bwem/BWEM)
set(BWAPI_DIR ${PROJECT_SOURCE_DIR}/external/bwem/external/openbw-bwapi)

add_subdirectory(${PROJECT_SOURCE_DIR}/external/bwem/BWEM)

set(GOOGLETEST_SOURCES
  ${PROJECT_SOURCE_DIR}/${GOOGLETEST_ROOT}/src/gtest-all.cc
  ${PROJECT_SOURCE_DIR}/${GOOGLETEST_ROOT}/src/gtest_main.cc
)

set(BWEB_SOURCES
  ${PROJECT_SOURCE_DIR}/src/Block.cpp
  ${PROJECT_SOURCE_DIR}/src/BWEB.cpp
  ${PROJECT_SOURCE_DIR}/src/BWEBUtil.cpp
  ${PROJECT_SOURCE_DIR}/src/PathFind.cpp
  ${PROJECT_SOURCE_DIR}/src/Station.cpp
  ${PROJECT_SOURCE_DIR}/src/Wall.cpp
)

include_directories(
  ${PROJECT_SOURCE_DIR}/external/bwem/BWEM/include
  ${PROJECT_SOURCE_DIR}/external/bwem/external/openbw-bwapi/bwapi/include
)

add_library(bweb
  ${BWEB_SOURCES}
)
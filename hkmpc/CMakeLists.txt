# Authors: Giulio Romualdi
# CopyPolicy: Released under the terms of the LGPLv2.1 or later

cmake_minimum_required(VERSION 3.1)

set (CMAKE_CXX_STANDARD 11)

project(OsqpEigen-HKMPC)

find_package(OsqpEigen)
find_package(Eigen3)

include_directories(SYSTEM ${EIGEN3_INCLUDE_DIR})

#MPCExample
add_executable(HKMPC src/hkmpc.cpp)
target_link_libraries(HKMPC OsqpEigen::OsqpEigen)

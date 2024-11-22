cmake_minimum_required(VERSION 3.12)
project(charge_schedule_for_mobile_robot)

find_package(PkgConfig REQUIRED)

add_library(nsgaii src/nsgaii.cpp)
include_directories(nsgaii ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nsgaii yaml-cpp)

add_library(charge_schedule_core src/charge_schedule_core.cpp)
include_directories(charge_schedule_core ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(charge_schedule_core yaml-cpp)

add_executable(charge_schedule src/charge_schedule.cpp)
target_link_libraries(charge_schedule nsgaii charge_schedule_core yaml-cpp)
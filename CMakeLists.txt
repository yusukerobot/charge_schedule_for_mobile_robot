cmake_minimum_required(VERSION 3.12)
project(charge_schedule_for_mobile_robot)

find_package(PkgConfig REQUIRED)

add_library(nsgaii src/details/nsgaii.cpp)
include_directories(nsgaii ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nsgaii yaml-cpp)

add_library(two_point_trans_schedule src/details/two_point_trans_schedule.cpp)
include_directories(two_point_trans_schedule ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(two_point_trans_schedule nsgaii yaml-cpp)

add_library(three_point_trans_schedule src/details/three_point_trans_schedule.cpp)
include_directories(three_point_trans_schedule ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(three_point_trans_schedule nsgaii yaml-cpp)

add_executable(two_main src/two_main.cpp)
target_link_libraries(two_main nsgaii two_point_trans_schedule three_point_trans_schedule)

add_executable(three_main src/two_main.cpp)
target_link_libraries(three_main nsgaii two_point_trans_schedule three_point_trans_schedule)
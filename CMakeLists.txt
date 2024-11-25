cmake_minimum_required(VERSION 3.12)
project(charge_schedule_for_mobile_robot)

find_package(PkgConfig REQUIRED)

# 基本のインクルードディレクトリとリンクライブラリの設定
set(COMMON_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(COMMON_LINK_LIBRARIES yaml-cpp)

# nsgaiiライブラリ
add_library(nsgaii src/details/nsgaii.cpp)
target_include_directories(nsgaii PUBLIC ${COMMON_INCLUDE_DIRS})
target_link_libraries(nsgaii PUBLIC ${COMMON_LINK_LIBRARIES})

# two_point_trans_scheduleライブラリ
add_library(two_point_trans_schedule src/details/two_point_trans_schedule.cpp)
target_include_directories(two_point_trans_schedule PUBLIC ${COMMON_INCLUDE_DIRS})
target_link_libraries(two_point_trans_schedule PUBLIC nsgaii ${COMMON_LINK_LIBRARIES})

# three_point_trans_scheduleライブラリ
add_library(three_point_trans_schedule src/details/three_point_trans_schedule.cpp)
target_include_directories(three_point_trans_schedule PUBLIC ${COMMON_INCLUDE_DIRS})
target_link_libraries(three_point_trans_schedule PUBLIC nsgaii ${COMMON_LINK_LIBRARIES})

# two_main実行ファイル
add_executable(two_main src/two_main.cpp)
target_link_libraries(two_main PUBLIC nsgaii two_point_trans_schedule)

# three_main実行ファイル
add_executable(three_main src/three_main.cpp)
target_link_libraries(three_main PUBLIC nsgaii three_point_trans_schedule)

# インストール設定: ライブラリと実行ファイルのインストール
install(TARGETS 
   nsgaii 
   two_point_trans_schedule 
   three_point_trans_schedule 
DESTINATION lib)

# include ディレクトリと params ディレクトリをまとめてインストール
install(DIRECTORY 
   ${CMAKE_CURRENT_SOURCE_DIR}/include/ 
   ${CMAKE_CURRENT_SOURCE_DIR}/params/
DESTINATION lib
FILES_MATCHING PATTERN "*.h"  # 必要に応じてヘッダーファイルだけをインストール
)

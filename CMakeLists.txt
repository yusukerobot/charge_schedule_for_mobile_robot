cmake_minimum_required(VERSION 3.22)
project(charge_schedule_for_mobile_robot)

# デフォルトビルドタイプ（Debugを推奨）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# デバッグ用フラグの設定
set(CMAKE_CXX_FLAGS_DEBUG "-g")

# 基本設定
set(COMMON_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(COMMON_LINK_LIBRARIES yaml-cpp)

# ---------------------------------
# nsgaiiライブラリ
# ---------------------------------
add_library(nsgaii src/details/nsgaii.cpp)
target_include_directories(nsgaii PUBLIC ${COMMON_INCLUDE_DIRS})
target_link_libraries(nsgaii PUBLIC ${COMMON_LINK_LIBRARIES})

# ---------------------------------
# two_point_trans_scheduleライブラリ
# ---------------------------------
add_library(two_point_trans_schedule src/details/two_point_trans_schedule.cpp)
target_include_directories(two_point_trans_schedule PUBLIC ${COMMON_INCLUDE_DIRS})
target_link_libraries(two_point_trans_schedule PUBLIC nsgaii ${COMMON_LINK_LIBRARIES})


# ---------------------------------
# 実行ファイル設定
# ---------------------------------

# two_main実行ファイル
add_executable(two_main src/two_main.cpp)
target_link_libraries(two_main PUBLIC nsgaii two_point_trans_schedule)

# sbx_test実行ファイル
add_executable(sbx_test src/sbx_test.cpp)
target_link_libraries(sbx_test PUBLIC nsgaii two_point_trans_schedule)

# sbx_test実行ファイル
add_executable(sbx_test2 src/sbx_test2.cpp)
target_link_libraries(sbx_test2 PUBLIC nsgaii two_point_trans_schedule)

# sbx_test実行ファイル
add_executable(sbx_test3 src/sbx_test3.cpp)
target_link_libraries(sbx_test3 PUBLIC nsgaii two_point_trans_schedule)

# ---------------------------------
# インストール設定
# ---------------------------------

# ライブラリのインストール
install(TARGETS 
    nsgaii 
    two_point_trans_schedule 
    two_main
    sbx_test
RUNTIME DESTINATION bin   # 実行ファイル
LIBRARY DESTINATION lib   # 動的ライブラリ
ARCHIVE DESTINATION lib   # 静的ライブラリ
)

# include ディレクトリと params ディレクトリをまとめてインストール
install(DIRECTORY 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ 
    ${CMAKE_CURRENT_SOURCE_DIR}/params/
DESTINATION include       # include ディレクトリにコピー
FILES_MATCHING PATTERN "*.h"
)

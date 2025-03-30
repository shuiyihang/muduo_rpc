cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)

# set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -stdlib=libc++")

# 启用 CMake 的选项来生成 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(rewrite_muduo)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 打开调试信息
add_definitions(-DMUDEBUG)

aux_source_directory(. SRC_LIST)
add_library(mymuduo SHARED ${SRC_LIST})
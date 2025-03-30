cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(myrpc)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/rpc/inc)
include_directories(${PROJECT_SOURCE_DIR}/example)
include_directories(/usr/include/mymuduo)

# 库文件搜索路径
link_libraries(${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(rpc/src)
add_subdirectory(example)
cmake_minimum_required(VERSION 2.8)
project(chat)

#配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)
SET(CMAKE_CXX_FLAGS "-std=c++11 -O3")
#设置可执行文件最终存储的路劲
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
 
#配置头文件的搜索路径
include_directories($(PROJECT_SOURCE_DIR)/include)
include_directories($(PROJECT_SOURCE_DIR)/include/server)
include_directories($(PROJECT_SOURCE_DIR)/include/server/model)
include_directories($(PROJECT_SOURCE_DIR)/thirdparty)
include_directories($(PROJECT_SOURCE_DIR)/include/server/db)
include_directories($(PROJECT_SOURCE_DIR)/include/server/redis)
#添加mysql动态库的搜索路径 ，该动态库不在默认搜索路劲
link_directories(/usr/lib64/mysql)
add_subdirectory(src)

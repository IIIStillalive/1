cmake_minimum_required(VERSION 3.16)
project(high_perf_server VERSION 0.1 LANGUAGES CXX)

# ---- 语言标准：C++20（gs 时代，充分支持） ----

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- 默认 Debug，方便 GDB 调试 ----
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# ---- 打开警告，把问题暴露在编译期 ----
add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)

# ---- 目标：server 可执行文件 ----
add_executable(server
  src/main.cpp
)

target_compile_features(server PRIVATE cxx_std_20)

# 头文件搜索路径:让 #include "server/xxx.hpp" 能找到
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/include)

# ---- 目标：epoll 探针(独立验证用,不参与 server 构建) ----
add_executable(epoll_probe
  src/epoll_probe.cpp
)

target_compile_features(epoll_probe PRIVATE cxx_std_20)
target_include_directories(epoll_probe PRIVATE ${CMAKE_SOURCE_DIR}/include)

# ---- 目标：线程池冒烟测试(独立验证 ThreadPool,不参与 server 构建) ----
add_executable(thread_pool_smoke
  src/thread_pool_smoke.cpp
)

target_compile_features(thread_pool_smoke PRIVATE cxx_std_20)
target_include_directories(thread_pool_smoke PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(thread_pool_smoke PRIVATE pthread)

cmake_minimum_required(VERSION 3.16)
project(trajectory_prediction CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Eigen3 REQUIRED NO_MODULE)
find_package(Threads REQUIRED)

add_executable(server main.cpp)

target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(server PRIVATE Eigen3::Eigen Threads::Threads)

# httplib needs _GLIBCXX_USE_CXX11_ABI on some Linux distros; Threads for blocking listen
target_compile_options(server PRIVATE -O2 -Wall -Wextra)

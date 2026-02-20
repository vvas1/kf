cmake_minimum_required(VERSION 3.16)
project(trajectory_prediction CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Eigen3 REQUIRED NO_MODULE)
find_package(Threads REQUIRED)

# ── libdatachannel (WebRTC data channels) ─────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    libdatachannel
    GIT_REPOSITORY https://github.com/paullouisageneau/libdatachannel.git
    GIT_TAG        v0.21.2
    GIT_SUBMODULES_RECURSE TRUE)

set(NO_MEDIA    ON  CACHE BOOL "" FORCE)
set(NO_EXAMPLES ON  CACHE BOOL "" FORCE)
set(NO_TESTS    ON  CACHE BOOL "" FORCE)
set(USE_GNUTLS  OFF CACHE BOOL "" FORCE)
set(USE_MBEDTLS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(libdatachannel)

# ── server binary ─────────────────────────────────────────────────────────────
add_executable(server main.cpp)

target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(server PRIVATE
    Eigen3::Eigen
    Threads::Threads
    datachannel)

target_compile_options(server PRIVATE -O2 -Wall -Wextra)

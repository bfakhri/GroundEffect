cmake_minimum_required(VERSION 3.12)
project(GroundEffect VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

include_directories(
    include
    ${SDL2_INCLUDE_DIRS}
)

file(GLOB SOURCES "src/*.cpp")

add_executable(GroundEffect ${SOURCES})

target_link_libraries(GroundEffect
    PRIVATE
    ${SDL2_LIBRARIES}
)

target_compile_options(GroundEffect PRIVATE -O3 -Wall -Wextra)

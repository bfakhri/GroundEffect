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
set(LIB_SOURCES ${SOURCES})
list(REMOVE_ITEM LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_executable(GroundEffect ${SOURCES})

target_link_libraries(GroundEffect
    PRIVATE
    ${SDL2_LIBRARIES}
)

target_compile_options(GroundEffect PRIVATE -O3 -Wall -Wextra)

enable_testing()
add_executable(GroundEffectTests ${LIB_SOURCES} tests/test_main.cpp)
target_link_libraries(GroundEffectTests PRIVATE ${SDL2_LIBRARIES})
target_compile_options(GroundEffectTests PRIVATE -O3 -Wall -Wextra)
add_test(NAME GroundEffectTests COMMAND GroundEffectTests)
set_tests_properties(GroundEffectTests PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

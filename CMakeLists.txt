cmake_minimum_required(VERSION 3.16)

project(uai_station LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(uai-station
    src/main.cpp
    src/emulator_manager.cpp
    src/chip8.cpp
    src/ps1_libretro.cpp
)

target_include_directories(uai-station PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
)

target_compile_options(uai-station PRIVATE
    ${SDL2_CFLAGS_OTHER}
)

target_link_libraries(uai-station PRIVATE
    ${SDL2_LIBRARIES}
    dl
)

if (UNIX AND NOT APPLE)
    target_link_libraries(uai-station PRIVATE pthread)
endif()
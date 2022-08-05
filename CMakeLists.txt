cmake_minimum_required(VERSION 3.16)
project(fast_life)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_$<CONFIG> ${CMAKE_BINARY_DIR})
enable_testing()

# TODO: Download these at configure time
include_directories(SDL2/include)
include_directories(SDL2_ttf/include)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SDL_DLLS SDL2/*/x64/*.dll SDL2_ttf/*/x64/*.dll)

find_library(SDL2_LIBRARIES SDL2 PATHS SDL2/lib/x64/ REQUIRED)
find_library(SDL2_TTF_LIBRARIES SDL2_ttf PATHS SDL2_ttf/lib/x64/ REQUIRED)
install(PROGRAMS ${SDL_DLLS} DESTINATION ${CMAKE_BINARY_DIR})
install(DIRECTORY fonts media DESTINATION ${CMAKE_BINARY_DIR})

add_subdirectory(src)

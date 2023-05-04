cmake_minimum_required(VERSION 3.14)
project(LastStop)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/SDL2)
add_subdirectory(external/SDL2_ttf)

include_directories(include external/SDL2/include external/SDL2_ttf)

# Collect all source files in the 'src' directory
file(GLOB SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} SDL2-static SDL2_ttf)

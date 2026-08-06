cmake_minimum_required(VERSION 3.25)

project(Shoot)

set(CMAKE_CXX_STANDARD 26)

find_package(SDL2 REQUIRED)


add_executable(
        Shoot
        Shoot.cpp
)


target_include_directories(
        Shoot
        PRIVATE
        ${SDL2_INCLUDE_DIRS}
)


target_link_libraries(
        Shoot
        ${SDL2_LIBRARIES}
)
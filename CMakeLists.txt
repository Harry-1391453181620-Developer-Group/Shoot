cmake_minimum_required(VERSION 3.25)

project(Shoot_Game)

set(CMAKE_CXX_STANDARD 26)

find_package(SDL2 REQUIRED)


add_executable(
        Shoot_Game
        Shoot.cpp
)


target_include_directories(
        Shoot_Game
        PRIVATE
        ${SDL2_INCLUDE_DIRS}
)


target_link_libraries(
        Shoot_Game
        ${SDL2_LIBRARIES}
)
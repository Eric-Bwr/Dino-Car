cmake_minimum_required(VERSION 3.0)
project(Cluster)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
find_package(Boost REQUIRED COMPONENTS system filesystem)

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
link_directories(${SDL2_LIBRARY_DIRS})

add_executable(Cluster Cluster.cpp Arduino.cpp)

target_link_libraries(Cluster
        ${SDL2_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        ${Boost_LIBRARIES}
)
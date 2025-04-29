cmake_minimum_required(VERSION 3.10)
project(Cluster)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_GFX REQUIRED SDL2_gfx)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
find_package(Boost REQUIRED COMPONENTS system filesystem)
find_library(WIRINGPI_LIB wiringPi REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(Cluster ${SOURCES})

target_compile_options(Cluster PRIVATE -O2 -Wall)

target_include_directories(Cluster PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
        ${SDL2_GFX_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
)

target_link_libraries(Cluster
        ${SDL2_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        ${SDL2_GFX_LIBRARIES}
        ${SDL2_IMAGE_LIBRARIES}
        ${Boost_LIBRARIES}
        wiringPi
)

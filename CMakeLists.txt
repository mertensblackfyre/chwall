cmake_minimum_required(VERSION 3.20)
project(mpv_discord_rpc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)

add_library(mpv_discord_rpc SHARED
    discord.cc
    plugin.cc
)

target_include_directories(mpv_discord_rpc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mpv_discord_rpc PRIVATE
    fmt::fmt
    nlohmann_json::nlohmann_json
    PkgConfig::MPV
)

target_compile_options(mpv_discord_rpc PRIVATE -Wall -Wextra -Wpedantic)

set_target_properties(mpv_discord_rpc PROPERTIES
    OUTPUT_NAME "discord_rpc"
    PREFIX ""
)

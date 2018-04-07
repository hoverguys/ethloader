# Project setup
cmake_minimum_required(VERSION 3.1)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/devkitPPC.cmake)
set(CMAKE_CXX_STANDARD 14)
project(ethloader C CXX ASM)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(SetupOGC)

# Sources
file(GLOB_RECURSE SOURCE_FILES src/*)
include_directories(include)
add_multi_target(ethloader "${SOURCE_FILES}")

# Dependencies
add_default_libraries(ethloader db bba ogc asnd)

# Create .dol
add_dol_targets(ethloader)

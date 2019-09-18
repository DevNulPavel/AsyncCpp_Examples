# CMake version
cmake_minimum_required (VERSION 3.9)


# Project name
project(cpp-channel VERSION 2.2.0 LANGUAGES CXX)

# Turn on the verbose
set(CMAKE_VERBOSE_MAKEFILE ON)

# Compiler vendors
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /W3")
    add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2")
endif()

# CXX target properties
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include files
set(INCLUDE_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/channel.h")

# Sources
set(SOURCE_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/channel.cpp")

# Include folders
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/")

# Output variables
# Стиль подключения библиотек: https://jira.game-insight.tech/browse/XGEN-1693
set("${PROJECT_NAME}_LIB" "${PROJECT_NAME}" PARENT_SCOPE)
set("${PROJECT_NAME}_INCLUDE_DIRS" "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)

# Make library target
add_library("${PROJECT_NAME}" STATIC ${INCLUDE_FILES} ${SOURCE_FILES})

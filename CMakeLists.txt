cmake_minimum_required(VERSION 3.15)

project(MediaLibrary)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(TAGLIB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TAGLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_definitions(-DTAGLIB_STATIC)

add_subdirectory(vendor/taglib)

add_library(MediaLibrary STATIC
	"include/MediaLibrary/MediaLibrary.h"
	"include/MediaLibrary/types.h"

	"src/MediaLibrary.cpp"
	"src/types.cpp"
)

target_link_libraries(MediaLibrary PRIVATE tag)

target_include_directories(MediaLibrary PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	vendor/taglib
    vendor/taglib/taglib/toolkit
    ${CMAKE_CURRENT_BINARY_DIR}/vendor/taglib
    ${CMAKE_CURRENT_BINARY_DIR}/vendor/taglib/taglib/toolkit
)
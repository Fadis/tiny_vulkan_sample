cmake_minimum_required(VERSION 3.30)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
PROJECT(
  tiny_vulkan_sample
  LANGUAGES CXX C
  HOMEPAGE_URL https://github.com/Fadis/tiny_vulkan_sample
  DESCRIPTION "Tiny Vulkan Sample"
  VERSION 0.1.0
)
cmake_policy( SET CMP0074 NEW )
include(CheckIncludeFiles)
include(CheckCXXSourceCompiles)
include(GNUInstallDirs)

# build type
#set(CMAKE_BUILD_TYPE Debug)

set( ADDITIONAL_COMPILE_FLAGS )
option( ENABLE_ADDRSAN "build with address sanitizer" OFF )
if( ENABLE_ADDRSAN )
  list(APPEND ADDITIONAL_COMPILE_FLAGS "-fsanitize=address" )
  if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    list(APPEND ADDITIONAL_COMPILE_FLAGS "-static-libasan" )
  endif()
endif()
option( ENABLE_PCOMPSAN "build with pointer compare sanitizer" OFF )
if( ENABLE_PCOMPSAN )
  list(APPEND ADDITIONAL_COMPILE_FLAGS "-fsanitize=pointer-compare" )
endif()
option( ENABLE_PSUBSAN "build with pointer subtract sanitizer" OFF )
if( ENABLE_PSUBSAN )
  list(APPEND ADDITIONAL_COMPILE_FLAGS "-fsanitize=pointer-subtract" )
endif()
option( ENABLE_THREADSAN "build with thread sanitizer" OFF )
if( ENABLE_THREADSAN )
  list(APPEND ADDITIONAL_COMPILE_FLAGS "-fsanitize=thread" )
endif()
option( ENABLE_LEAKSAN "build with leak sanitizer" OFF )
if( ENABLE_LEAKSAN )
  list(APPEND ADDITIONAL_COMPILE_FLAGS "-fsanitize=leak" )
endif()
option( ENABLE_UBSAN "build with undefined behavior sanitizer" OFF )
if( ENABLE_UBSAN )
  list(APPEND ADDITIONAL_COMPILE_FLAGS "-fsanitize=undefined" )
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list( APPEND ADDITIONAL_COMPILE_FLAGS "-Wno-unknown-warning-option" )
endif()
option( ENABLE_WERROR "build with -Werror" ON )
list( APPEND ADDITIONAL_COMPILE_FLAGS "-Wall" )
if( ENABLE_WERROR )
  list( APPEND ADDITIONAL_COMPILE_FLAGS "-Werror" )
endif()
set( CMAKE_CXX_STANDARD "20" )
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )
option( ENABLE_EXAMPLES "build examples" OFF )

# compile option (Debug) 
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g ${ADDITIONAL_COMPILER_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g ${ADDITIONAL_COMPILER_FLAGS}")

# compile option (Release)
set(CMAKE_CXX_FLAGS_RELEASE "-march=armv7-a -mtune=cortex-a7 -Os ${ADDITIONAL_COMPILER_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "-march=armv7-a -mtune=cortex-a7 -Os ${ADDITIONAL_COMPILER_FLAGS}")

#if you don't want the full compiler output, remove the following line
set(CMAKE_VERBOSE_MAKEFILE OFF)

option( ENABLE_CLANG_TIDY "use clang tidy" OFF )
if(ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY NAMES "clang-tidy" REQUIRED)
  set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY} -checks=-*,clang-analyzer-*,boost-*,-boost-use-ranges,bugprone-*,cert-*,concurrency-*,cppcoreguidelines-*,-cppcoreguidelines-special-member-functions,-cppcoreguidelines-macro-usage,-cppcoreguidelines-owning-memory,modernize-*,-modernize-use-trailing-return-type,portability-* -header-filter=${CMAKE_SOURCE_DIR}/include)
else()
  unset(CMAKE_CXX_CLANG_TIDY)
endif()

find_package(PkgConfig REQUIRED)

find_package(Threads REQUIRED)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.68.0 CONFIG COMPONENTS program_options system REQUIRED )
pkg_check_modules(Vulkan REQUIRED vulkan)
find_package(GLSLC REQUIRED)
find_package(SPIRVOPT)
find_package(OpenImageIO)
find_package(SDL3)
pkg_check_modules( LIBDRM libdrm )

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/external/VulkanMemoryAllocator/include/vk_mem_alloc.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/tiny_vulkan_sample/vk_mem_alloc.h
  COPYONLY
)
INSTALL(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/include/tiny_vulkan_sample/vk_mem_alloc.h
  DESTINATION
  include/tiny_vulkan_sample/
)
set( TVS_INCURE_DIRS "" )
list( APPEND TVS_INCURE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include" )
list( APPEND TVS_INCURE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/include" )
list( APPEND TVS_INCURE_DIRS "${Boost_INCLUDE_DIRS}" )
list( APPEND TVS_INCURE_DIRS "${Vulkan_INCLUDE_DIRS}" )
if( OpenImageIO_FOUND )
  list( APPEND TVS_INCURE_DIRS "${OIIO_INCLUDE_DIR}" )
endif()

set( TVS_LINK_DIRS "" )
list( APPEND TVS_LINK_DIRS "${Boost_LIBRARY_DIRS}" )
list( APPEND TVS_LINK_DIRS "${Vulkan_LIBRARY_DIRS}" )
if( OpenImageIO_FOUND )
  list( APPEND TVS_LINK_DIRS "${OIIO_LIBRARY_DIR}" )
endif()

INCLUDE_DIRECTORIES(
  ${TVS_INCURE_DIRS}
)
link_directories(
  ${TVS_LINK_DIRS}
)

add_definitions( -DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 )

if( EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json" )
  execute_process(
    COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
  )
endif()

set( EASYPACK_VENDOR "fadis" )
set( EASYPACK_RELEASE "1" )
set( EASYPACK_MAINTAINER "Naomasa Matsubayashi <foo@exmple.com>" )
set( EASYPACK_COMPRESSION_TYPE "xz" )
set( EASYPACK_LICENSE_NAME "MIT" )
include(EasyPack)

include(AddShader)

subdirs( include src )


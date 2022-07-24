cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

enable_language(CXX)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) # don't let OPTION erase non-cache variables
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # enable PIC
set(CMAKE_DEBUG_POSTFIX "-d")
set(CMAKE_CXX_EXTENSIONS OFF) # disable compiler extensions
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug) # ensure CMAKE_BUILD_TYPE is not empty
endif()

# build version
include(cmake/build_version.cmake)

# project parameters
set(project_name vulkify)
set(${project_name}_version_file vulkify_version.hpp)
set(${project_name}_version ${build_version})
set(cxx_standard cxx_std_20)

project(${project_name} VERSION "${${project_name}_version}")
set(${project_name}_soversion ${PROJECT_VERSION_MAJOR})
set(is_root_project OFF) # indicate if this is the top-level project

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(is_root_project ON)
  message(STATUS "${PROJECT_NAME} is top level")
endif()

# options
option(VULKIFY_BUILD_EXAMPLES "Build vulkify examples" ${is_root_project})
option(VULKIFY_DEBUG_TRACE "Enable debug trace messages" ${is_root_project})
option(VULKIFY_INSTALL "Install vulkify" ${is_root_project})
option(VULKIFY_USE_PCH "Enable PCH" ON)

# includes
include(CheckPIESupported)
check_pie_supported()
include(cmake/platform.cmake) # set up helper cache variables

# ktl
if(VULKIFY_INSTALL)
  set(KTL_INSTALL ON)

  if(NOT KTL_INSTALL)
    message(WARNING "Invalid INSTALL flags")
  endif()
endif()

include(FetchContent)
FetchContent_Declare(ktl
  GIT_REPOSITORY https://github.com/karnkaul/ktl
  GIT_TAG v1.4.1
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/src/ktl"
)
FetchContent_MakeAvailable(ktl)

# git operations
find_package(Git QUIET)

if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  git_get_commit_hash(${PROJECT_NAME}_GIT_COMMIT_HASH) # store commit hash in ${PROJECT_NAME}_GIT_COMIT_HASH
endif()

# external
add_subdirectory(ext)
add_subdirectory(cmake/interface) # import interface library

# header text
if(is_root_project)
  set(build_config ${CMAKE_BUILD_TYPE})

  if(CMAKE_CONFIGURATION_TYPES)
    set(build_config "Multi-config")
  endif()

  message(STATUS "[${CMAKE_SYSTEM_PROCESSOR}] [${PLATFORM}] [${CMAKE_GENERATOR}] [${CMAKE_CXX_COMPILER_ID}] [${build_config}]")
endif()

# compile commands
if(is_root_project AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  message(STATUS "Exporting compile commands")
endif()

# library
add_subdirectory(vulkify)

# examples
if(VULKIFY_BUILD_EXAMPLES)
  message(STATUS "Adding vulkify examples to build tree")
  add_subdirectory(examples)

  enable_testing()
  message(STATUS "Adding vulkify test target")
  add_test(NAME vulkify-headless COMMAND ${PROJECT_NAME}-quick-start --headless)
endif()

if(VULKIFY_INSTALL)
  install_package_config(PACKAGE ${PROJECT_NAME} IN vulkify/config.cmake.in)
  install_package_version(PACKAGE ${PROJECT_NAME} VERSION ${${PROJECT_NAME}_version})
  export_package_to_build_tree(PACKAGE ${PROJECT_NAME})
endif()

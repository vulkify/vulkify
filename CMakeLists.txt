cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

enable_language(CXX)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) # don't let OPTION erase non-cache variables
set(CMAKE_CXX_EXTENSIONS OFF CACHE INTERNAL "" FORCE) # disable compiler extensions
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # enable PIC
set(CMAKE_DEBUG_POSTFIX "-d")

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
option(VULKIFY_UBSAN "Enable UBSan" OFF)
option(VULKIFY_DEBUG_TRACE "Enable debug trace messages" ${is_root_project})
option(VULKIFY_INSTALL "Install vulkify" ${is_root_project})
option(VULKIFY_PCH "Enable PCH" ON)

# ktl
if(VULKIFY_INSTALL)
  set(KTL_INSTALL ON)

  if(NOT KTL_INSTALL)
    message(WARNING "Invalid INSTALL flags")
  endif()
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
  ktl
  GIT_REPOSITORY https://github.com/karnkaul/ktl
  GIT_TAG a952dbc
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/src/ktl"
)
FetchContent_MakeAvailable(ktl)

# includes
include(CheckPIESupported)
check_pie_supported()
include(cmake/platform.cmake) # set up helper cache variables

# git operations
find_package(Git QUIET)

if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  git_get_commit_hash(${PROJECT_NAME}_GIT_COMMIT_HASH) # store commit hash in ${PROJECT_NAME}_GIT_COMIT_HASH

  if(is_root_project)
    option(GIT_UPDATE_SUBMODULES "Init/Update submodules during generation" ${is_root_project})

    if(GIT_UPDATE_SUBMODULES)
      git_update_submodules(WARNING) # update git submodules and warn on failure
    endif()
  endif()
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

# version
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/${${project_name}_version_file}.in")
  configure_src_bin(
    IN src/${${project_name}_version_file}.in
    OUT ${project_name}/${${project_name}_version_file}
    TARGET ${project_name}
  )
endif()

if(VULKIFY_INSTALL)
  install(TARGETS ${PROJECT_NAME} interface options EXPORT ${PROJECT_NAME}-targets)
  install(EXPORT ${PROJECT_NAME}-targets
    FILE ${PROJECT_NAME}-targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" "${CMAKE_CURRENT_BINARY_DIR}/include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
  )
  install_package_config(PACKAGE ${PROJECT_NAME})
  install_package_version(PACKAGE ${PROJECT_NAME} VERSION ${${PROJECT_NAME}_version})
  export_package_to_build_tree(PACKAGE ${PROJECT_NAME})
endif()

# examples
if(VULKIFY_BUILD_EXAMPLES)
  message(STATUS "Adding vulkify examples to build tree")
  add_subdirectory(examples)
endif()

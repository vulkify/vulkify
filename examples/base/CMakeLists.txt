cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(base)

if(NOT TARGET vulkify)
  find_package(vulkify REQUIRED)
endif()

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PUBLIC vulkify::vulkify vulkify::options)
target_sources(${PROJECT_NAME} PRIVATE
  base.hpp
  base.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC .)

cmake_minimum_required(VERSION 3.17)

project(vulkify-quick-start)

if(NOT TARGET vulkify)
  find_package(vulkify REQUIRED)
endif()

add_executable(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE vulkify::vulkify vulkify::options)
target_sources(${PROJECT_NAME} PRIVATE quick_start.cpp)

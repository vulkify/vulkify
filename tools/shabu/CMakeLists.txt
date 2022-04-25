cmake_minimum_required(VERSION 3.14)

project(shabu)

add_executable(shabu)
target_compile_features(shabu PRIVATE cxx_std_20)

if(UNIX)
  target_compile_options(shabu PRIVATE -Wall -Wextra -Werror)
endif()

target_sources(shabu PRIVATE shabu.cpp)

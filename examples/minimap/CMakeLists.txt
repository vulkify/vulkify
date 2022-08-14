project(minimap)

if(NOT TARGET base)
  message(FATAL_ERROR "Missing base target")
endif()

add_executable(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE base::base)
target_sources(${PROJECT_NAME} PRIVATE minimap.cpp)

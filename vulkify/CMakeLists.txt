add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY VERSION ${${PROJECT_NAME}_version})
set_property(TARGET ${PROJECT_NAME} PROPERTY SOVERSION ${${PROJECT_NAME}_soversion})
set_property(TARGET ${PROJECT_NAME} PROPERTY INTERFACE_${PROJECT_NAME}_MAJOR_VERSION ${${PROJECT_NAME}_soversion})
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPATIBLE_INTERFACE_STRING ${PROJECT_NAME}_MAJOR_VERSION)

set(system SYSTEM)

if(${is_root_project})
  set(system "")
endif()

target_include_directories(${PROJECT_NAME} ${system} INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_include_directories(${PROJECT_NAME} PRIVATE
  include
  src
  "${CMAKE_CURRENT_BINARY_DIR}/include"
)
target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ktl::ktl
  glm
  ${PROJECT_NAME}::interface
  PRIVATE
  glfw
  dyvk::dyvk
  vma
  stb-image
  freetype
  ${PROJECT_NAME}::options
)
target_compile_definitions(${PROJECT_NAME} PUBLIC
  $<$<BOOL:${VULKIFY_DEBUG}>:VULKIFY_DEBUG>
  $<$<BOOL:${VULKIFY_DEBUG_TRACE}>:VULKIFY_DEBUG_TRACE>
  $<$<BOOL:${VULKIFY_REQUIRE_VULKAN_1_1_0}>:VULKIFY_REQUIRE_VULKAN_1_1_0>
)

if(APPLE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE VULKIFY_VK_PORTABILITY)
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_XYZW_ONLY GLM_FORCE_RADIANS GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_SILENT_WARNINGS GLM_ENABLE_EXPERIMENTAL GLM_EXT_INCLUDED)

include(vulkify_headers.cmake)
add_subdirectory(src)
target_source_group(TARGET ${PROJECT_NAME})

if(VULKIFY_USE_PCH)
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    <vulkan/vulkan.hpp>
    <vector>
    <string>
    <functional>
  )
endif()

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
endif()

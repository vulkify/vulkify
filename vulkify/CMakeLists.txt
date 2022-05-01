add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY VERSION ${${PROJECT_NAME}_version})
set_property(TARGET ${PROJECT_NAME} PROPERTY SOVERSION ${${PROJECT_NAME}_soversion})
set_property(TARGET ${PROJECT_NAME} PROPERTY INTERFACE_${PROJECT_NAME}_MAJOR_VERSION ${${PROJECT_NAME}_soversion})
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPATIBLE_INTERFACE_STRING ${PROJECT_NAME}_MAJOR_VERSION)

target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE
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
  vk-bootstrap::vk-bootstrap
  vma
  stb-image
  ${PROJECT_NAME}::options
)
target_compile_definitions(${PROJECT_NAME} PUBLIC
  $<$<BOOL:${VULKIFY_DEBUG}>:VULKIFY_DEBUG>
  $<$<BOOL:${VULKIFY_DEBUG_TRACE}>:VULKIFY_DEBUG_TRACE>
  $<$<BOOL:${VULKIFY_REQUIRE_VULKAN_1_1_0}>:VULKIFY_REQUIRE_VULKAN_1_1_0>
)
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

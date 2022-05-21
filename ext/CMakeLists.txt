set(ext_targets "")

execute_process(COMMAND
  ${CMAKE_COMMAND} -E tar -xf src.zip
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# glfw
set(GLFW_VULKAN_STATIC OFF)
set(GLFW_INSTALL ${VULKIFY_INSTALL})
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
add_subdirectory(src/glfw)

if(VULKIFY_INSTALL)
  export(EXPORT glfwTargets)
endif()

# dyvk
set(DYVK_INSTALL ${VULKIFY_INSTALL})
add_subdirectory(src/dyvk)

# vk-bootstrap
set(VK_BOOTSTRAP_VULKAN_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/dyvk/include" CACHE STRING "" FORCE)

if(NOT EXISTS "${VK_BOOTSTRAP_VULKAN_HEADER_DIR}/vulkan/vulkan.h")
  message(FATAL_ERROR "Missing vulkan headers")
endif()

add_subdirectory(src/vk-bootstrap)

if(VULKIFY_INSTALL)
  install(EXPORT vk-bootstrap-targets
    FILE vk-bootstrap-targets.cmake
    NAMESPACE vk-bootstrap::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )
  export(EXPORT vk-bootstrap-targets)
endif()

# glm
set(GLM_INSTALL ${VULKIFY_INSTALL})
add_subdirectory(src/glm)
target_compile_options(glm INTERFACE
  $<$<OR:$<BOOL:${LINUX_GCC}>,$<BOOL:${WINDOWS_GCC}>>:-Wno-volatile>
  $<$<OR:$<BOOL:${LINUX_CLANG}>,$<BOOL:${WINDOWS_CLANG}>>:-Wno-deprecated-volatile>
)
list(APPEND ext_targets glm)

# vma
add_subdirectory(src/vma)
list(APPEND ext_targets vma)

# stb
add_subdirectory(src/stb)
list(APPEND ext_targets stb-image)

# freetype
add_subdirectory(src/freetype)

if(VULKIFY_INSTALL)
  export(EXPORT freetype-targets)
endif()

if(VULKIFY_INSTALL)
  install(TARGETS ${ext_targets} EXPORT vulkify-ext-targets)
  install(EXPORT vulkify-ext-targets
    FILE vulkify-ext-targets.cmake
    NAMESPACE vulkify::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vulkify
  )
  export(EXPORT vulkify-ext-targets)
endif()
